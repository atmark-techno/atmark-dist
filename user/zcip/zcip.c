

// 
//  This program does the Zeroconf ad-hoc link-local IP configuration trick.
//
//
//  The algorithm is:
//      0. seed rng with MAC address
//      1. look for remembered address, if present goto 3
//      2. pick a random address between 169.254.1.0 and 169.254.254.0 (inclusive)
//      3. arp probe for it a couple of times (source MAC=me IP=0.0.0.0, target MAC=0 IP=selected)
//      4. if we get a reply to the probe or we see another probe for the same target address, goto 2
//      5. claim the address by sending a couple of arps for it (source MAC=me IP=selected, target MAC=0, IP=selected)
//      6. remember address (store to nvram)
//      7. watch for address collisions, if found defend, if defense fails goto 2
// 

// this is needed for the 48-bit random number functions in stdlib.h
// and is the easiest official way.
#define _GNU_SOURCE 1

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <libnet.h>
#include <pcap.h>

#include <arpa/inet.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>

#include <netinet/in.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/types.h>

#define RELEASE "4"

#define MAX_CONFLICTS		(10)
#define RATE_LIMIT_INTERVAL	(60)

#include "lockfile.h"

int output_to_syslog = 0;

struct zc_if {
    char *if_name;                // string name of the interface (eg eth0, or eth1:0)
    int default_route;		  // Give this if a default route.
    struct libnet_link_int *lin;  // our libnet link to the interface we are using
    pcap_t *pcap;                 // the packet capture device
    int pcap_fd;                  // the file descriptor to listen for packets on
    struct ether_addr *my_ea;     // the hardware address for the interface
    struct in_addr ip;            // the IP we're probing for or are defending (network byte sex)
    uint8_t *arp_defense_packet;  // sent out as defense by handle_collision()
    struct timeval last_defense;  // time we last tried to defend on this interface
};



void open_log(void) {
    openlog("zcip", LOG_CONS | LOG_PID, LOG_DAEMON);

    fflush(NULL);

    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
}




void die(char *format, ...) {
    va_list args;
    char buffer[1024];

    va_start(args, format);

    if (output_to_syslog) {
        vsnprintf(buffer, sizeof(buffer), format, args);
        syslog(LOG_ERR, buffer);
    } else {
        vprintf(format, args);
    }

    va_end(args);

    exit(1);
}




void print(char *format, ...) {
    va_list args;
    char buffer[1024];

    va_start(args, format);

    if (output_to_syslog) {
        vsnprintf(buffer, sizeof(buffer), format, args);
        syslog(LOG_INFO, buffer);
    } else {
        vprintf(format, args);
    }

    va_end(args);
}




// like memcmp, but for timevals
// returns -1 if t0 < t1
//          0 if t0 = t1
//          1 if t0 > t1
int tvcmp(const struct timeval *t0, const struct timeval *t1) {
    if (t0->tv_sec < t1->tv_sec) return -1;
    if (t0->tv_sec > t1->tv_sec) return 1;
    if (t0->tv_usec < t1->tv_usec) return -1;
    if (t0->tv_usec > t1->tv_usec) return 1;
    return 0;
}




// t0 -= t1, but min is { 0, 0 }
void tvsub(struct timeval *t0, const struct timeval *t1) {
    if (tvcmp(t0, t1) <= 0) {
        t0->tv_sec = 0;
        t0->tv_usec = 0;
        return;
    }
    t0->tv_usec -= t1->tv_usec;
    if (t0->tv_usec < 0) {
        t0->tv_usec += 1000000;
        t0->tv_sec --;
    }
    t0->tv_sec -= t1->tv_sec;
}


#ifdef	linux
#	define CHECK_KERNEL_SOCKETFILTERS
	/* libpcap has user mode emulation of this, but it doesn't
	 * seem to be kicking in
	 */
#endif

#ifdef CHECK_KERNEL_SOCKETFILTERS
#include	<linux/filter.h>

//
//       NAME:  kernel_socketfilter()
//
//   FUNCTION:  Verifies that kernel socket filters are available.
//
//  ARGUMENTS:  None
//
//    GLOBALS:  None
//
//    RETURNS:  Only if it worked.  If there's a problem it prints an error
//              message and exits.
//  
static void kernel_socketfilter()
{
#ifdef SO_ATTACH_FILTER
    int	sock = socket (PF_INET, SOCK_DGRAM, 0);
    int err;
    struct sock_filter code = BPF_STMT (BPF_RET|BPF_K, 0);
    struct sock_fprog filt = { 1, &code };

    if (sock < 0)
    	die ("can't get a socket?\n");
    err = setsockopt (sock, SOL_SOCKET, SO_ATTACH_FILTER, &filt, sizeof filt);
    close (sock);
    if (err == 0)
	return;

#endif
    die ("kernel doesn't support BSD socket filters ... reconfigure\n");
}
#endif


//
//       NAME:  ifdown()
//
//   FUNCTION:  Brings down the interface named by the argument if_name.
//
//  ARGUMENTS:  The name of the interface being handled.
//
//    GLOBALS:  None
//
//    RETURNS:  Only if it worked.  If there's a problem it prints an error
//              message and exits.
//  
void ifdown(struct zc_if *this_if) {
    int r;

    int sock_fd;
    struct ifreq ifr;


    sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock_fd < 0) die("cannot create socket: %s\n", strerror(errno));

    strncpy(ifr.ifr_name, this_if->if_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = (char)NULL;


    // get flags
    r = ioctl(sock_fd, SIOCGIFFLAGS, &ifr);
    if (r < 0) die("SIOCGIFFLAGS error in ifdown: %s\n", strerror(errno));


    // set flags to bring interface down
    ifr.ifr_flags &= ~(IFF_UP | IFF_BROADCAST | IFF_MULTICAST | IFF_PROMISC);
    r = ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
    if (r < 0) die("SIOCSIFFLAGS error in ifdown2: %s\n", strerror(errno));


    close(sock_fd);
}




//
//       NAME:  ifup()
//
//   FUNCTION:  Brings up the interface named by the argument if_name.
//              The IP address is passed in.  Assumes that the interface is
//              currently down.
//
//  ARGUMENTS:  The IP address to set the interface to (a struct in_addr
//              with network byte sex).
//
//    GLOBALS:  None.
//
//    RETURNS:  Only if it worked.  If there's a problem it prints an error
//              message and exits.
//  
void ifup(struct zc_if *this_if) {
    int r;

    int sock_fd;
    struct ifreq ifr;


    sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock_fd < 0) die("cannot create socket: %s\n", strerror(errno));

    strncpy(ifr.ifr_name, this_if->if_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = (char)NULL;


    // set address
    ifr.ifr_addr.sa_family = AF_INET;
    ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr = this_if->ip;
    r = ioctl(sock_fd, SIOCSIFADDR, &ifr);
    if (r < 0) {
	// make a nice message for the most common error...
	if (errno == -EPERM) die("You need to run this program as root.\n");
	// ...but handle other errors as well
	die("SIOCSIFADDR error in ifup: %s\n", strerror(errno));
    }


    // get flags
    r = ioctl(sock_fd, SIOCGIFFLAGS, &ifr);
    if (r < 0) die("SIOCGIFFLAGS error in ifup: %s\n", strerror(errno));


    // set flags to bring interface up
    // Condition is for ethernet alias devices.
    // ?? Do we need MULTICAST ?
    if ((ifr.ifr_flags & (IFF_UP | IFF_BROADCAST /*| IFF_MULTICAST */)) !=
	                 (IFF_UP | IFF_BROADCAST /*| IFF_MULTICAST */)) {

        ifr.ifr_flags |= (IFF_UP | IFF_BROADCAST | IFF_MULTICAST);
        r = ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
        if (r < 0) 
	    die("SIOCSIFFLAGS error in ifup: %s(%i)\n", strerror(errno),errno);
    }

    close(sock_fd);
}




//
//       NAME:  seed_rng()
//
//   FUNCTION:  Seeds random-number generator with random # or the MAC address.
//
//  ARGUMENTS:  the MAC address for the applicable interface (if NULL, 
//              then random # is used for seed).
//
//    GLOBALS:  None.
//
//    RETURNS:  Only if it worked.  If there was a problem, writes an error
//              message to the syslog and exits with an exit code of 1.
//
void seed_rng(struct ether_addr *my_ea) {
    uint16_t seed[3];

    if (my_ea == NULL)   // use true random seed
    {
        int handle, length;

        handle = open ("/dev/urandom", O_RDONLY);
        if (handle == -1)
        {
           perror ("/dev/urandom not available!");
           exit (-1);
        }

        length = read (handle, seed, sizeof (seed));
        if (length != sizeof (seed))
        {
           perror ("/dev/urandom read error!");
           exit (-1);
        }
        close (handle);
    }
    else                // seed RNG with MAC address
    {
       seed[0] = (uint8_t)my_ea->ether_addr_octet[0];
       seed[0] += (uint8_t)my_ea->ether_addr_octet[1] << 8;

       seed[1] = (uint8_t)my_ea->ether_addr_octet[2];
       seed[1] += (uint8_t)my_ea->ether_addr_octet[3] << 8;

       seed[2] = (uint8_t)my_ea->ether_addr_octet[4];
       seed[2] += (uint8_t)my_ea->ether_addr_octet[5] << 8;
    }

    seed48(seed);
}




//
//       NAME:  pick_random_address()
//
//   FUNCTION:  Picks a random address of the form 169.254.X.Y, where X
//              is between 1 and 254 inclusive, and Y is between 0 and 255
//              inclusive.
//
//  ARGUMENTS:  The address structure to put the randomly-generated address
//              in.
//
//    GLOBALS:  None.
//
//    RETURNS:  If it worked.  If there's a problem, writes an error
//              message to the syslog and exits with an exit code of 1.
//
//      FIXME:  We should keep track of what addresses we've tried, so we
//              dont accidentally reuse already-probed addresses.  Use
//              something like a static 16 kilo-bit array, with each bit 0
//              if the address is untested and 1 if it's been tested.
//
void pick_random_address(struct in_addr *ip) {
    uint16_t suffix;

    do {
        suffix = ((uint32_t)mrand48() >> 16);
    } while (suffix < 0x0100 || suffix > 0xfeff);

    // network byte sex
    ip->s_addr = htonl(0xa9fe0000|suffix);
}




//
//       NAME:  build_arp_defense_packet()
//
//   FUNCTION:  Initializes the ARP packet in arp_defense_packet with the
//              address in probe_ip.
//
//  ARGUMENTS:  The interface to defend for.
//
//    GLOBALS:  None
//
//    RETURNS:  If all went well.  If there's a problem it prints an error
//              message and exits with an exit code of 1.
//
void build_arp_defense_packet(struct zc_if *this_if) {
    int r;

    struct ether_addr broadcast_ea = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
    struct ether_addr null_ea = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };


    //
    // build the arp packet, in case we need to defend our address later
    //

    // allocate memory for the ARP probe packet
    r = libnet_init_packet(LIBNET_ETH_H + LIBNET_ARP_H, &(this_if->arp_defense_packet));
    if (r == -1) die("libnet_init_packet(): error\n");

    // build the Ethernet header of the ARP probe packet
    r = libnet_build_ethernet(
        broadcast_ea.ether_addr_octet,  // destination ethernet address
        this_if->my_ea->ether_addr_octet,        // source ethernet address
        ETHERTYPE_ARP,                  // protocol type
        NULL, 0,                        // payload pointer and size (will be filled in later)
        this_if->arp_defense_packet
    );
    if (r == -1) die("libnet_build_ethernet(): error\n");

    // build the ARP header
    r = libnet_build_arp(
        ARPHRD_ETHER,
        ETHERTYPE_IP,
        ETH_ALEN,
        4,
        ARPOP_REQUEST,
        this_if->my_ea->ether_addr_octet,    // From: my EA
	(uint8_t *)(&(this_if->ip.s_addr)),  // From: my IP
        null_ea.ether_addr_octet,            // To:   00:00:00:00:00:00
	(uint8_t *)(&(this_if->ip.s_addr)),  // To: my IP
        NULL, 0,                             // payload pointer and size
        this_if->arp_defense_packet + LIBNET_ETH_H
    );
    if (r == -1) die("libnet_build_arp(): error\n");
}




void packet_handler(char *unused, struct pcap_pkthdr *h, uint8_t *packet) {
#if ZCIP_DEBUG
    struct tm *tm_p;
    int i;

    tm_p = localtime((time_t *)&(h->ts.tv_sec));

    print(
        "%02d:%02d:%02d.%06d\n",
	tm_p->tm_hour,
	tm_p->tm_min,
	tm_p->tm_sec,
	(int)h->ts.tv_usec
    );

    for (i = 0; i < h->caplen; i ++) {
        if (i % 16 == 0) print("   ");
        print(" %02X", packet[i]);
        if (i % 16 == 15) print("\n");
        else if (i % 8 == 7) print(" ");
    }

    print("\n");

    print(
        "source EA: %02X:%02X:%02X:%02X:%02X:%02X\n",
        packet[LIBNET_ETH_H + 8],
        packet[LIBNET_ETH_H + 9],
        packet[LIBNET_ETH_H + 10],
        packet[LIBNET_ETH_H + 11],
        packet[LIBNET_ETH_H + 12],
        packet[LIBNET_ETH_H + 13]
    );

    print(
        "uint32_t source EA: 0x%08X\n",
        *((uint32_t *)(&packet[LIBNET_ETH_H + 8]))
    );

    print(
        "uint16_t source EA: 0x%04hX\n",
        *((uint16_t *)(&packet[LIBNET_ETH_H + 12]))
    );

    print(
        "source IP: %d.%d.%d.%d\n",
        packet[LIBNET_ETH_H + 14],
        packet[LIBNET_ETH_H + 15],
        packet[LIBNET_ETH_H + 16],
        packet[LIBNET_ETH_H + 17]
    );

    print(
        "target EA: %02X:%02X:%02X:%02X:%02X:%02X\n",
        packet[LIBNET_ETH_H + 18],
        packet[LIBNET_ETH_H + 19],
        packet[LIBNET_ETH_H + 20],
        packet[LIBNET_ETH_H + 21],
        packet[LIBNET_ETH_H + 22],
        packet[LIBNET_ETH_H + 23]
    );

    print(
        "target IP: %d.%d.%d.%d\n",
        packet[LIBNET_ETH_H + 24],
        packet[LIBNET_ETH_H + 25],
        packet[LIBNET_ETH_H + 26],
        packet[LIBNET_ETH_H + 27]
    );
#else
    // make gcc/lint/.. be quiet
    unused = unused;
    h = h;
    packet = packet;
#endif
}




//
//       NAME:  handle_collision()
//
//   FUNCTION:  This function gets called when a collision is detected.
//              We check to see if we've had a collision in the recent
//              past, and if so just give up.  If this is the first
//              collision in recent history, we defend.
//
//  ARGUMENTS:  The interface name that the collision is on.
//
//    GLOBALS:  None
//
//    RETURNS:  0 if the address was successfully defended and we can go
//              on using it, and -1 if we need to stop using this address
//              and try a new one.
//
int handle_collision(struct zc_if *this_if) {
    int r;

    struct timeval ten_seconds = { 10, 0 };
    struct timeval now, t;


    // print("handling collision\n");

    r = gettimeofday(&now, NULL);
    if (r != 0) die("gettimeofday: %s\n", strerror(errno));

    t = now;

    tvsub(&t, &(this_if->last_defense));  // t -= last_defense
    if (tvcmp(&t, &ten_seconds) <= 0) {
        print("persistent collision or too many collisions, reconfiguring\n");
        return -1;
    }

    this_if->last_defense = now;

    // print("defending\n");

    r = libnet_write_link_layer(
        this_if->lin,
        this_if->if_name,
        this_if->arp_defense_packet,
        LIBNET_ARP_H + LIBNET_ETH_H
    );
    if (r == -1) die("libnet_write_link_layer(): error in handle_collision()\n");

    return 0;
}




// 
// set the default route to be through if_name
//
void set_default_route(struct zc_if *this_if) {
    int r;

    int sock_fd;
    struct in_addr tmp_ip;

    struct rtentry rte;

    if (!this_if->default_route) return;

    sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock_fd < 0) die("cannot create socket: %s\n", strerror(errno));

    tmp_ip.s_addr = htonl(0);

    memset(&rte, 0, sizeof(rte));

    // destination: 0.0.0.0
    rte.rt_dst.sa_family = AF_INET;
    ((struct sockaddr_in *)&rte.rt_dst)->sin_addr = tmp_ip;

    rte.rt_flags = RTF_UP;

    rte.rt_dev = this_if->if_name;
    
    r = ioctl(sock_fd, SIOCADDRT, &rte);
    if (r < 0) die("SIOCSADDRT error: %s\n", strerror(errno));

    close(sock_fd);
}




//
//       NAME:  claim_address()
//
//   FUNCTION:  Sends out ARPs claiming ownership of the specified address.
//
//  ARGUMENTS:  The address to claim (network byte sex), and the interface name.
//
//    GLOBALS:  None.
//
//    RETURNS:  If everything went well it returns 0.  If a collision was
//              detected and defense failed, it returns -1.  In this case,
//              we need to try a new address.  If there was some other
//              problem it prints an error message and exits with an exit
//              code of 1.
//
int claim_address(struct zc_if *this_if) {
    int r;

    uint8_t *arp_packet;
    int num_arps_done;


    struct ether_addr broadcast_ea = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
    struct ether_addr null_ea = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };


    print("claiming ownership of address %s\n", inet_ntoa(this_if->ip));

    //
    // send out a couple of gratuitous arps to claim this address
    //


    // allocate memory for the ARP probe packet
    r = libnet_init_packet(LIBNET_ETH_H + LIBNET_ARP_H, &arp_packet);
    if (r == -1) die("libnet_init_packet(): error\n");


    // build the Ethernet header of the ARP probe packet
    r = libnet_build_ethernet(
        broadcast_ea.ether_addr_octet,
        this_if->my_ea->ether_addr_octet,
        ETHERTYPE_ARP,
        NULL, 0,  // payload pointer and size
        arp_packet
    );
    if (r == -1) die("libnet_build_ethernet(): error\n");


    // build the ARP header
    r = libnet_build_arp(
        ARPHRD_ETHER,
        ETHERTYPE_IP,
        ETH_ALEN,
        4,
        ARPOP_REQUEST,
        this_if->my_ea->ether_addr_octet,  // From: my EA
	(uint8_t *)&(this_if->ip.s_addr), // From: my IP
	//	(uint8_t *)&(this_if->ip->s_addr), // From: my IP
        null_ea.ether_addr_octet,          // To:  00:00:00:00:00:00
	(uint8_t *)&(this_if->ip.s_addr), // To:  my IP
	//	(uint8_t *)&(this_if->ip->s_addr), // To:  my IP
        NULL, 0,  // payload pointer and size
        arp_packet + LIBNET_ETH_H
    );
    if (r == -1) die("libnet_build_arp(): error\n");


    for (num_arps_done = 0; num_arps_done < 2; num_arps_done ++) {
        struct timeval start;

        r = gettimeofday(&start, NULL);
        if (r < 0) die("gettimeofday(): %s\n", strerror(errno));

        // print("sending probe %d for %s\n", (num_probes_done + 1), inet_ntoa(probe_ip));

        r = libnet_write_link_layer(
            this_if->lin,
            this_if->if_name,
            arp_packet,
            LIBNET_ARP_H + LIBNET_ETH_H
        );
        if (r == -1) die("libnet_write_link_layer(): error in claim_address()\n");

        while (1) {
            fd_set readers;
            struct timeval now, timeout;

            FD_ZERO(&readers);
            FD_SET(this_if->pcap_fd, &readers);

            timeout = start;
            timeout.tv_sec += 2;

            r = gettimeofday(&now, NULL);
            if (r < 0) die("gettimeofday(): %s\n", strerror(errno));
            
            // tvsub is like a prefix -=, except that the answer is non-negative
            // ({ 0, 0 } if the result would have been negative)
            tvsub(&timeout, &now);

            if ((timeout.tv_sec == 0) && (timeout.tv_usec == 0)) break;

            r = select(this_if->pcap_fd + 1, &readers, NULL, NULL, &timeout);
            if (r == -1) die("select: %s\n", strerror(errno));

            if (r == 0) continue;  // timeout

            r = pcap_dispatch(this_if->pcap, 10, (pcap_handler)packet_handler, NULL);
            if (r == -1) die("pcap error: %s\n", pcap_geterr(this_if->pcap));

            // if we get here, then we've detected a collision
            r = handle_collision(this_if);
            if (r != 0) {
                libnet_destroy_packet(&arp_packet);
                return -1;
            }
        }
    }

    libnet_destroy_packet(&arp_packet);

    return 0;
}




//
//       NAME:  probe_for_address()
//
//   FUNCTION:  Sends ARP probes and listens for replies, to determine
//              if the address is currently in use by a host on the
//              link-local network.
//
//  ARGUMENTS:  The interface to probe on
//
//    GLOBALS:  None
//
//    RETURNS:  If any host claims the address or if any host other than
//              us is also probing for this address, it returns 0,
//              otherwise it returns -1.
//              If there's a problem, it exits printing an informative
//              error message and with an exit code of 1.
//
int probe_for_address(struct zc_if *this_if) {
    int r;
    char ebuf[LIBNET_ERRBUF_SIZE];

    struct ether_addr broadcast_ea = { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };
    struct ether_addr null_ea = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

    struct in_addr null_ip;

    char filter_code[128];
    struct bpf_program bpf_prog;

    uint8_t *arp_probe_packet;

    int max_probes = 4;  // how many ARP probes to send
    int num_probes_done;

    print("probing for %s\n", inet_ntoa(this_if->ip));

    null_ip.s_addr = htonl(0);  // 0.0.0.0

    // allocate memory for the ARP probe packet
    r = libnet_init_packet(LIBNET_ETH_H + LIBNET_ARP_H, &arp_probe_packet);
    if (r == -1) die("libnet_init_packet(): error\n");


    // build the Ethernet header of the ARP probe packet
    r = libnet_build_ethernet(
        broadcast_ea.ether_addr_octet,    // to: broadcast
        this_if->my_ea->ether_addr_octet, // from: me
        ETHERTYPE_ARP,                    // an ARP packet
        NULL, 0,                          // payload pointer and size
        arp_probe_packet
    );
    if (r == -1) die("libnet_build_ethernet(): error\n");


    // build the ARP header
    // an "ARP Probe" is an ARP Request with the sender IP set to 0.0.0.0
    r = libnet_build_arp(
        ARPHRD_ETHER,
        ETHERTYPE_IP,
        ETH_ALEN,
        4,
        ARPOP_REQUEST,                           // an ARP Request
        this_if->my_ea->ether_addr_octet,        // From: my EA
	(uint8_t *)&null_ip.s_addr,              // From: 0.0.0.0
        null_ea.ether_addr_octet,                // To:   00:00:00:00:00:00
	(uint8_t *)&(this_if->ip.s_addr),        // To:   169.254.X.Y
        NULL, 0,                                 // no payload
        arp_probe_packet + LIBNET_ETH_H
    );
    if (r == -1) die("libnet_build_arp(): error\n");


    // initialize the packet capture interface
    // read up to the first 100 bytes of each captured packet
    // put interface into promiscuous mode
    // wait 10 ms for packets to accumulate before returning
    // FIXME: does this need to be promiscuous?
    this_if->pcap = pcap_open_live(this_if->if_name, 100, 1, 10, ebuf);
    if (!(this_if->pcap)) die("pcap_open_live(\"%s\"): %s\n", this_if->if_name, ebuf);


    // From revision 4 of the IETF Internet Draft "Dynamic Configuration
    // of IPv4 Link-Local Addresses":
    //
    //  If during this period the host receives any ARP packet where the
    //  packet's 'sender IP address' is the address being probed for, then
    //  the host MUST treat this address as being in use by some other
    //  host, and MUST select a new pseudo-random address and repeat the
    //  process. In addition, if during this period the host receives any
    //  ARP probe where the packet's 'target IP address' is the address
    //  being probed for, and the packet's 'sender hardware address' is
    //  not the hardware address of any of the host's interfaces, then the
    //  host MUST similarly treat this as an address collision and select
    //  a new address as above. This can occur if two (or more) hosts
    //  attempt to configure the same link-local address at the same time.
    //
    // In an ARP packet, the source Ethernet address occupies bytes 8-13
    // (inclusive), source IP is in bytes 14-17, target Ethernet address
    // is in 18-23, and target IP is in 24-27.
    //
    // We need to match: 
    //     source ip = probe_ip
    //     or
    //     target ip = probe_ip and source ea != me
    //
    snprintf(
        filter_code, sizeof(filter_code),
        "arp and ((arp[14:4]=0x%02X%02X%02X%02X) or (arp[24:4]=0x%02X%02X%02X%02X and (arp[8:4] != 0x%02X%02X%02X%02X or arp[12:2]!=0x%02X%02X)))",

        ((uint8_t *)(&this_if->ip.s_addr))[0],  // convert from network byte-order (1/4)
        ((uint8_t *)(&this_if->ip.s_addr))[1],  // convert from network byte-order (2/4)
        ((uint8_t *)(&this_if->ip.s_addr))[2],  // convert from network byte-order (3/4)
        ((uint8_t *)(&this_if->ip.s_addr))[3],  // convert from network byte-order (4/4)

        ((uint8_t *)(&this_if->ip.s_addr))[0],  // convert from network byte-order (1/4)
        ((uint8_t *)(&this_if->ip.s_addr))[1],  // convert from network byte-order (2/4)
        ((uint8_t *)(&this_if->ip.s_addr))[2],  // convert from network byte-order (3/4)
        ((uint8_t *)(&this_if->ip.s_addr))[3],  // convert from network byte-order (4/4)

        this_if->my_ea->ether_addr_octet[0],
        this_if->my_ea->ether_addr_octet[1],
        this_if->my_ea->ether_addr_octet[2],
        this_if->my_ea->ether_addr_octet[3],
        this_if->my_ea->ether_addr_octet[4],
        this_if->my_ea->ether_addr_octet[5]
    );

    // print("filter code: [%s]\n", filter_code);

    r = pcap_compile(this_if->pcap, &bpf_prog, filter_code, 0, -1);
    if (r == -1) die("pcap_compile(): error\n");

    r = pcap_setfilter(this_if->pcap, &bpf_prog);
    if (r == -1) die("pcap_setfilter(): error\n");

    pcap_freecode(&bpf_prog);


    this_if->pcap_fd = pcap_fileno(this_if->pcap);
    if (this_if->pcap_fd < 0) die("pcap_fd < 0!\n");

    /* At this point we've set the filter, but we may have already
     * collected a packet or two ... */
    {
        struct timeval timeout;
	fd_set readers;
	while(1)
	{
	    FD_ZERO(&readers);
	    FD_SET(this_if->pcap_fd, &readers);
	    timeout.tv_sec  = 0;
	    timeout.tv_usec = 0;
	    r = select(this_if->pcap_fd + 1, &readers, NULL, NULL, &timeout);
	    if (r == -1) die("select: %s\n", strerror(errno));
	    if (!r) break;

            r = pcap_dispatch(this_if->pcap, 10, (pcap_handler)packet_handler, NULL);
	}
    }

    for (num_probes_done = 0; num_probes_done < max_probes; num_probes_done ++) {
        struct timeval start;

        r = gettimeofday(&start, NULL);
        if (r < 0) die("gettimeofday(): %s\n", strerror(errno));

        // print("sending probe %d for %s\n", (num_probes_done + 1), inet_ntoa(probe_ip));

        r = libnet_write_link_layer(
            this_if->lin,
            this_if->if_name,
            arp_probe_packet,
            LIBNET_ARP_H + LIBNET_ETH_H
        );
        if (r == -1) die("libnet_write_link_layer(): error in probe_for_address()\n");

        while (1) {
            fd_set readers;
            struct timeval now, timeout;

            FD_ZERO(&readers);
            FD_SET(this_if->pcap_fd, &readers);

            r = gettimeofday(&now, NULL);
            if (r < 0) die("gettimeofday(): %s\n", strerror(errno));

            timeout.tv_sec = (start.tv_sec + 2) - now.tv_sec;
            timeout.tv_usec = start.tv_usec - now.tv_usec;

            if (timeout.tv_usec < 0) {
                timeout.tv_usec += 1000000;
                timeout.tv_sec -= 1;
            }

            if (
                (timeout.tv_sec < 0) ||
                ((timeout.tv_sec == 0) && (timeout.tv_usec == 0))
            ) break;

            r = select(this_if->pcap_fd + 1, &readers, NULL, NULL, &timeout);
            if (r == -1) die("select: %s\n", strerror(errno));

            if (r == 0) break;

            print("avoiding collision - discarding address\n");

            // if we get here, then the packet filter has detected an ARP
            // packet indicating we shouldnt use this IP address
            r = pcap_dispatch(this_if->pcap, 10, (pcap_handler)packet_handler, NULL);
            if (r == -1) die("pcap error: %s\n", pcap_geterr(this_if->pcap));

            libnet_destroy_packet(&arp_probe_packet);
            pcap_close(this_if->pcap);

            return 0;
        }
    }

    //
    // if we get here, no matching packet was found during the ARP probing
    //

    libnet_destroy_packet(&arp_probe_packet);
    pcap_close(this_if->pcap);

    return -1;
}




//
//       NAME:  look_for_remembered_address()
//
//   FUNCTION:  Looks in non-volatile storage for an address that has
//              been used successfully in the past.
//
//  ARGUMENTS:  A pointer to a struct zc_if, if one was found. The ip field
//              is initialised.
//
//    GLOBALS:  None.
//
//    RETURNS:  0 if an address was found, and -1 if no address was found.
//              If there's a problem, the function writes an error message
//              to the syslog and exits with an exit code of 1.
//
//      FIXME:  This should probably shell out to an external script,
//              something like:
//
//                  /usr/lib/zcip/zcip-mem (store A.B.C.D|recall)
//
int look_for_remembered_address(struct zc_if *this_if) {
    // FIXME: temporary hack to "remember" IP address provided on the command line
    if (this_if->ip.s_addr == htonl(0)) return -1;
    return 0;
}




//
//       NAME:  remember_address()
//
//   FUNCTION:  Commits the address to non-volatile storage, so that next
//              time we can start out trying this one.
//
//  ARGUMENTS:  The address to remember.
//
//    GLOBALS:  None.
//
//    RETURNS:  0 if all went well and the address was committed to
//              non-volatile storage, -1 if there was a problem and the
//              address was not saved.
//
//      FIXME:  This should probably shell out to an external script,
//              something like:
//
//                  /usr/lib/zcip/zcip-mem (store A.B.C.D|recall)
//
int remember_address(struct zc_if *this_if) {
    print("not storing IP for %s (not implemented yet!)\n", this_if->if_name);
    return -1;
}




//
//       NAME:  wait_for_collision()
//
//   FUNCTION:  Watches the network for other hosts trying to claim our
//              address.  If someone else tries to take our address, we
//              defend it.  If the defense fails, this function returns.
//
//  ARGUMENTS:  The interface to watch
//
//    RETURNS:  Nothing.
//
void wait_for_collision(struct zc_if *this_if) {
    print("watching for collisions\n");

    while (1) {
        int r;
        fd_set readers;

        FD_ZERO(&readers);
        FD_SET(this_if->pcap_fd, &readers);

        r = select(this_if->pcap_fd + 1, &readers, NULL, NULL, NULL);
        if (r == -1) die("select: %s\n", strerror(errno));

        // if we get here, then the packet filter has detected a potential
        // collision that we need to defend agains
        r = pcap_dispatch(this_if->pcap, 10, (pcap_handler)packet_handler, NULL);
        if (r == -1) die("pcap error: %s\n", pcap_geterr(this_if->pcap));

        if (handle_collision(this_if) != 0) return;
    }
} 




//
//       NAME:  stop_listening_for_collisions()
//
//   FUNCTION:  This function undoes the initialization done by
//              start_listening_for_collisions().  Shuts down the packet
//              filter.  Destroys the ARP defense packet.
//
//  ARGUMENTS:  The interface to de-configure
//
//    GLOBALS:  None
//
//    RETURNS:  If it worked.  If there's a problem initializing the
//              packet filter, it prints an error message and exits.
//
void stop_listening_for_collisions(struct zc_if *this_if) {
    pcap_close(this_if->pcap);
    libnet_destroy_packet(&(this_if->arp_defense_packet));
}




//
//       NAME:  start_listening_for_collisions()
//
//   FUNCTION:  Initializes a packet filter to watch for address
//              collisions. Initializes the ARP defense packet.  Resets
//              the timestamp keeping track of the last collision to zero
//              (Jan 1, 1970).
//
//  ARGUMENTS:  The interface to listen on.
//
//    GLOBALS:  The IP address in probe_ip is read and assumed to be ours.
//              pcap and pcap_fd are initialized.  arp_defense_packet is
//              initialized.
//
//    RETURNS:  If it worked.  If there's a problem initializing the
//              packet filter, it prints an error message and exits.
//
void start_listening_for_collisions(struct zc_if *this_if) {
    int r;
    char ebuf[LIBNET_ERRBUF_SIZE];

    char filter_code[128];
    struct bpf_program bpf_prog;


    build_arp_defense_packet(this_if);


    // initialize the packet capture interface
    // read up to the first 100 bytes of each captured packet
    // DONT put interface into promiscuous mode
    // wait 10 ms for packets to accumulate before returning
    this_if->pcap = pcap_open_live(this_if->if_name, 100, 0, 10, ebuf);
    if (!this_if->pcap) die("pcap_open_live(\"%s\"): %s\n", this_if->if_name, ebuf);


    // From revision 4 of the IETF Internet Draft "Dynamic Configuration
    // of IPv4 Link-Local Addresses":
    //
    //     At any time, if a host receives an ARP packet where the 'sender
    //     IP address' is the host's own IP address, but the 'sender
    //     hardware address' does not match any of the host's own
    //     interface addresses, then this is a conflicting ARP packet,
    //     indicating an address collision. A host MUST respond to a
    //     conflicting ARP packet...
    //
    // In an ARP packet, the source Ethernet address occupies bytes 8-13
    // (inclusive), source IP is in bytes 14-17, target Ethernet address
    // is in 18-23, and target IP is in 24-27.
    //
    // We need to match: 
    //     source ip = probe_ip
    //     and
    //     source ea != me
    //
    snprintf(
        filter_code, sizeof(filter_code),
        "arp and arp[14:4]=0x%02X%02X%02X%02X and (arp[8:4] != 0x%02X%02X%02X%02X or arp[12:2]!=0x%02X%02X)",

        ((uint8_t *)(&this_if->ip.s_addr))[0],  // convert from network byte-order (1/4)
        ((uint8_t *)(&this_if->ip.s_addr))[1],  // convert from network byte-order (2/4)
        ((uint8_t *)(&this_if->ip.s_addr))[2],  // convert from network byte-order (3/4)
        ((uint8_t *)(&this_if->ip.s_addr))[3],  // convert from network byte-order (4/4)

        this_if->my_ea->ether_addr_octet[0],
        this_if->my_ea->ether_addr_octet[1],
        this_if->my_ea->ether_addr_octet[2],
        this_if->my_ea->ether_addr_octet[3],
        this_if->my_ea->ether_addr_octet[4],
        this_if->my_ea->ether_addr_octet[5]
    );

    // print("filter code: [%s]\n", filter_code);

    r = pcap_compile(this_if->pcap, &bpf_prog, filter_code, 0, -1);
    if (r == -1) die("pcap_compile(): error\n");

    r = pcap_setfilter(this_if->pcap, &bpf_prog);
    if (r == -1) die("pcap_setfilter(): error\n");

    pcap_freecode(&bpf_prog);


    this_if->pcap_fd = pcap_fileno(this_if->pcap);
    if (this_if->pcap_fd < 0) die("pcap_fd < 0!\n");


    this_if->last_defense.tv_sec = 0;
    this_if->last_defense.tv_usec = 0;
}




void background(void) {
    static int is_backgrounded = 0;
    int r;


    // only do this once
    if (is_backgrounded) return;
    is_backgrounded = 1;


    fflush(NULL);

    r = fork();
    if (r < 0) {
        print("failed to fork: %s", strerror(errno));
        exit(1);
    }

    if (r > 0) {
        // kill off the parent
        // the sleep is here so that the childs last tty output comes before the return of the shell prompt
        if (output_to_syslog == 0) sleep(1);
        exit(0);
    }


    //
    // here we are in the child
    //


    // from now on, all output needs to go to the syslog no matter what the user asked for
    if (output_to_syslog == 0) {
        print("successfully acquired an IP address, backgrounding\n");
        print("all subsequent output will go to the syslog\n");
        open_log();
        output_to_syslog = 1;
    }
}

void version(void) {
    printf("zcip release " RELEASE "\n\
Copyright (C) 2001-2002 Various authors\n\
zcip comes with NO WARRANTY,\n\
to the extent permitted by law.\n\
You may redistribute copies of zcip\n\
under the terms of the 3-clause BSD license.\n\
For more information about these matters,\n\
see the file named Copyright.\n\
");

    exit (0);
}


void usage(int exit_code) {
    printf("usage: zcip [OPTIONS]\n\
Zeroconf IPv4 Link-Local Address Configuration\n\
OPTIONS:\n\
    -h, --help               Print this help, and exit.\n\
    -i, --interface IFNAME   Use interface IFNAME.\n\
                             If not provided it uses 'eth0'.\n\
    -r, --randseed           Seeds RNG with random # rather than MAC address.\n\
    -s, --syslog             Output to syslog instead of stdout.\n\
    -v, --version            Print the version information, and exit.\n\
\n\
This program does the ad-hoc link-local IPv4 auto-configuration trick, as\n\
described in the IETF Internet Draft 'Dynamic Configuration of IPv4\n\
Link-Local Addresses'.\n\
");

    exit (exit_code);
}




int main(int argc, char *argv[]) {
    int r, rand_seed = 0;
    int c;
    int probe_denials = 0;
    char ebuf[LIBNET_ERRBUF_SIZE];

    struct option longopts[] = {
        {"help", no_argument, NULL, 'h' },
	{"version", no_argument, NULL, 'v' },
        {"interface",required_argument, NULL, 'i' },
        {"randseed",no_argument, NULL, 'r'},
        {"syslog", no_argument, NULL, 's' },
        {"default", no_argument, NULL, 'd' },
        {0,0,0,0},
    };

    // static allocation of the device, just for now
    struct zc_if interface0;

    memset(&interface0, 0, sizeof(interface0));
    interface0.if_name = "eth0";


    while ((c = getopt_long(argc, argv, "rshi:v",longopts, NULL)) != EOF) {
        switch (c) {
            case 'h':
                usage(0);

            case 'i':
                interface0.if_name = optarg;
                break;

            case 'd':
                interface0.default_route = 1;
                break;

            case 'r':
                rand_seed = 1;
                break;

            case 's':
                output_to_syslog = 1;
                open_log();
                break;

	    case 'v':
	        version();
            default:
                print("unknown argument: %s\n", optarg);
                usage(1);
        }
    }

#ifdef CHECK_KERNEL_SOCKETFILTERS
    // insist on kernel support for socket filters
    kernel_socketfilter();
#endif

    if (!lock_check(interface0.if_name))
        die("Interface is locked by another zcip\n");

    // bring the interface up to 0.0.0.0
    interface0.ip.s_addr = htonl(0);
    ifup(&interface0);

    // initialize libnet link-level access to the network interface
    interface0.lin = libnet_open_link_interface(interface0.if_name, ebuf);
    if (!(interface0.lin)) die("libnet_open_link_interface(\"%s\"): %s\n", interface0.if_name, ebuf);


    // get the Ethernet address of this interface
    // this will be used to seed the PRNG, and as the source MAC in the ARP probes
    interface0.my_ea = libnet_get_hwaddr(interface0.lin, interface0.if_name, ebuf);
    if (!(interface0.my_ea)) die("libnet_get_hwaddr(\"%s\"): %s\n", interface0.if_name, ebuf);


    print(
        "interface: %s (%02X:%02X:%02X:%02X:%02X:%02X)\n",
        interface0.if_name,
        interface0.my_ea->ether_addr_octet[0],
        interface0.my_ea->ether_addr_octet[1],
        interface0.my_ea->ether_addr_octet[2],
        interface0.my_ea->ether_addr_octet[3],
        interface0.my_ea->ether_addr_octet[4],
        interface0.my_ea->ether_addr_octet[5]
    );


    seed_rng(rand_seed ? NULL : interface0.my_ea);


    // FIXME: temporary hack to "remember" an IP address provided on the command line
    if (optind < argc) {
        r = inet_aton(argv[optind], &(interface0.ip));
        if (r == 0) die("error parsing address: %s\n", argv[optind]);
    }


    if (look_for_remembered_address(&interface0) == 0) goto PROBE;

PICK:
    if (probe_denials >= MAX_CONFLICTS) sleep(RATE_LIMIT_INTERVAL);
    pick_random_address(&(interface0.ip));

PROBE:
    if (probe_for_address(&interface0) == 0) {
        print("address in use\n");
	probe_denials++;
        goto PICK;
    }
    probe_denials = 0;


    // bring the interface up to the selected address
    ifup(&interface0);

    // initialize pcap to listen for address collisions
    start_listening_for_collisions(&interface0);

    // use if_name as the default route
    set_default_route(&interface0);

    if (claim_address(&interface0) != 0) {
        ifdown(&interface0);
        stop_listening_for_collisions(&interface0);
        goto PICK;
    }


    remember_address(&interface0);


    // fork and exit, to send the application to the background 
    lock_unlock(interface0.if_name);
    background();
    if (!lock_check(interface0.if_name))
        die("Problem re-locking interface.\n");


    wait_for_collision(&interface0);


    // if we get here, a collision has been detected and defense failed,
    // so we need to reconfigure the IP setup


    ifdown(&interface0);
    stop_listening_for_collisions(&interface0);

    // bring the interface up to 0.0.0.0
    interface0.ip.s_addr = htonl(0);
    ifup(&interface0);

    goto PICK;
}
