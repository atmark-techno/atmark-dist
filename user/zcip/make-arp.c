

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

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




int output_to_syslog = 0;




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




void usage(int exit_code) {
    printf("\n\
make-arp -- sends arbitrary ARP Requests\n\
\n\
usage: make-arp [OPTIONS] [sea=SOURCE-EA] [sip=SOURCE-IP] tea=TARGET-EA tip=TARGET-IP\n\
\n\
OPTIONS:\n\
\n\
    -h          Print this help.\n\
\n\
\n\
Only ARP Requests for now.  In the Ethernet header, the source EA is\n\
mine and the target EA is the broadcast address FF:FF:FF:FF:FF:FF.\n\
\n\
");

    exit (exit_code);
}




int main(int argc, char *argv[]) {
    int r;
    int i;
    int c;
    char ebuf[LIBNET_ERRBUF_SIZE];

    struct ether_addr broadcast_ea;

    struct ether_addr *sea;
    struct ether_addr tea;
    struct in_addr sip, tip;

    uint8_t *arp_probe_packet;

    int sock_fd;
    struct ifreq ifr;

    char *if_name = "eth0";
    struct libnet_link_int *lin;

    uint32_t tmp[6];


    broadcast_ea.ether_addr_octet[0] = 0xFF;
    broadcast_ea.ether_addr_octet[1] = 0xFF;
    broadcast_ea.ether_addr_octet[2] = 0xFF;
    broadcast_ea.ether_addr_octet[3] = 0xFF;
    broadcast_ea.ether_addr_octet[4] = 0xFF;
    broadcast_ea.ether_addr_octet[5] = 0xFF;


    while ((c = getopt(argc, argv, "hi:")) != EOF) {
        switch (c) {
            case 'h':
                usage(0);

            case 'i':
                if_name = optarg;
                break;

            default:
                print("unknown argument: %s\n", optarg);
                usage(1);
        }
    }

    
    // initialize libnet link-level access to the network interface
    lin = libnet_open_link_interface(if_name, ebuf);
    if (!lin) die("libnet_open_link_interface(\"%s\"): %s\n", if_name, ebuf);


    // get the Ethernet address of this interface
    sea = libnet_get_hwaddr(lin, if_name, ebuf);
    if (!sea) die("libnet_get_hwaddr(\"%s\"): %s\n", if_name, ebuf);


    // get the IP address of this interface
    sock_fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock_fd < 0) die("cannot create socket: %s\n", strerror(errno));

    strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = (char)NULL;


    ifr.ifr_addr.sa_family = AF_INET;
    r = ioctl(sock_fd, SIOCGIFADDR, &ifr);
    if (r < 0) die("SIOCSIFADDR error: %s\n", strerror(errno));
    sip = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;

    close(sock_fd);


    for (i = optind; i < argc; i ++) {
        if (strncmp(argv[i], "sip=", 4) == 0) {
            r = inet_aton(&argv[i][4], &sip);
            if (r == 0) die("invalid sip\n");
        } else if (strncmp(argv[i], "sea=", 4) == 0) {
            r = sscanf(
                argv[i],
                "sea=%X:%X:%X:%X:%X:%X",
                &tmp[0],
                &tmp[1],
                &tmp[2],
                &tmp[3],
                &tmp[4],
                &tmp[5]
            );
            sea->ether_addr_octet[0] = tmp[0];
            sea->ether_addr_octet[1] = tmp[1];
            sea->ether_addr_octet[2] = tmp[2];
            sea->ether_addr_octet[3] = tmp[3];
            sea->ether_addr_octet[4] = tmp[4];
            sea->ether_addr_octet[5] = tmp[5];
            if (r != 6) die("invalid sea\n");
        } else if (strncmp(argv[i], "tip=", 4) == 0) {
            r = inet_aton(&argv[i][4], &tip);
            if (r == 0) die("invalid tip\n");
        } else if (strncmp(argv[i], "tea=", 4) == 0) {
            r = sscanf(
                argv[i],
                "tea=%X:%X:%X:%X:%X:%X",
                &tmp[0],
                &tmp[1],
                &tmp[2],
                &tmp[3],
                &tmp[4],
                &tmp[5]
            );
            tea.ether_addr_octet[0] = tmp[0];
            tea.ether_addr_octet[1] = tmp[1];
            tea.ether_addr_octet[2] = tmp[2];
            tea.ether_addr_octet[3] = tmp[3];
            tea.ether_addr_octet[4] = tmp[4];
            tea.ether_addr_octet[5] = tmp[5];
            if (r != 6) die("invalid tea\n");
        } else {
            die("unknown arg: %s\n", argv[i]);
        }
    }


    print("interface %s\n", if_name);

    print("    sip: %s\n", inet_ntoa(sip));
    print("    sea: %02X:%02X:%02X:%02X:%02X:%02X\n",
        sea->ether_addr_octet[0],
        sea->ether_addr_octet[1],
        sea->ether_addr_octet[2],
        sea->ether_addr_octet[3],
        sea->ether_addr_octet[4],
        sea->ether_addr_octet[5]
    );

    print("    tip: %s\n", inet_ntoa(tip));
    print("    tea: %02X:%02X:%02X:%02X:%02X:%02X\n",
        tea.ether_addr_octet[0],
        tea.ether_addr_octet[1],
        tea.ether_addr_octet[2],
        tea.ether_addr_octet[3],
        tea.ether_addr_octet[4],
        tea.ether_addr_octet[5]
    );


    // allocate memory for the ARP probe packet
    r = libnet_init_packet(LIBNET_ETH_H + LIBNET_ARP_H, &arp_probe_packet);
    if (r == -1) die("libnet_init_packet(): error\n");


    // build the Ethernet header of the ARP probe packet
    r = libnet_build_ethernet(
        broadcast_ea.ether_addr_octet,
        sea->ether_addr_octet,
        ETHERTYPE_ARP,
        NULL, 0,  // payload pointer and size
        arp_probe_packet
    );
    if (r == -1) die("libnet_build_ethernet(): error\n");


    // build the ARP header
    r = libnet_build_arp(
        ARPHRD_ETHER,
        ETHERTYPE_IP,
        ETH_ALEN,
        4,
        ARPOP_REQUEST,
        sea->ether_addr_octet, (uint8_t *)&sip.s_addr,
        tea.ether_addr_octet, (uint8_t *)&tip.s_addr,
        NULL, 0,  // payload pointer and size
        arp_probe_packet + LIBNET_ETH_H
    );
    if (r == -1) die("libnet_build_arp(): error\n");


    r = libnet_write_link_layer(
        lin,
        if_name,
        arp_probe_packet,
        LIBNET_ARP_H + LIBNET_ETH_H
    );
    if (r == -1) die("libnet_write_link_layer(): error\n");


    return 0;
}


