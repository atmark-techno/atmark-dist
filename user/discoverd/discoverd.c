/*********************************************************************
 *
 * file name : discoverd.c
 * summary   : program to reply discover packet
 * coded by  : mori
 * copyright : Atmark Techno
 * date(init): 2003.12.04
 * modified  : 2005.12.07 raw socket support. <by nakai>
 * 
 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <net/route.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <netpacket/packet.h>
#include <net/ethernet.h> 
#include <arpa/inet.h>

#include "discover_proto.h"
#include "udpip.h"

/* MACRO define */
#define TO_INADDR(x)	(*(struct in_addr*)&x)

/* constant value */
#define DEVICE_NAME                     "eth0"
#define DEVICE_NAME_LEN                 4
#define PROC_ROUTE_PATH                 "/proc/net/route"

#if defined(ENABLE_FLATFSD) || defined(ENABLE_FLATFSD_USR1)
#define NETWORK_FILE_NAME               "/etc/config/interface.eth0"
#else
#define NETWORK_FILE_NAME               "/etc/network.d/interface.eth0"
#endif

#define DEFAULT_IP                      0
#define DEFAULT_RT_METRIC               3
#define MAC_ADDRESS_SIZE                6

#define DHCP_CMD                        "/sbin/dhcpcd"
char* const DHCP_UP_PARAM[]             = {DHCP_CMD, "-t", "30", NULL};
char* const DHCP_DOWN_PARAM[]           = {DHCP_CMD, "-k", NULL};

/* network config file format */
#define	DHCP_KEY_STR                    "DHCP"
#define	IPADDRESS_KEY_STR               "IPADDRESS"
#define	NETMASK_KEY_STR                 "NETMASK"
#define	GATEWAY_KEY_STR                 "GATEWAY"

static pid_t g_dhcp_pid                 = 0;

#if defined(ENABLE_FLATFSD)
int save_config(void){
	pid_t pid;
	int ret;
	char *param[]={"/bin/flatfsd", "-s", 0};
	
	if((pid = vfork()) == 0){
		execv(param[0], param);
		_exit(0);
	}
	
	for(;;){
		ret = waitpid(pid, NULL, 0);
		if(ret == -1 && errno == EINTR){
			continue;
		}
		break;
	}

	return 0;
}
#elif defined(ENABLE_FLATFSD_USR1)
int save_config(void){
	pid_t pid;
	FILE *fp;
	fp = fopen("/var/run/flatfsd.pid", "r");
	if(fp == NULL){
		return -1;
	}
	fscanf(fp, "%d", &pid);
	fclose(fp);
	
	kill(pid, SIGUSR1);
	sleep(5);
	
	return 0;
}
#else //unuse flatfsd
#define save_config()
#endif

#if defined(ENABLE_SERI2ETH)
int restart_seri2eth(void){
	pid_t pid;
	FILE *fp;
	fp = fopen("/var/run/seri2eth.pid", "r");
	if(fp == NULL){
		return -1;
	}
	fscanf(fp, "%d", &pid);
	fclose(fp);
	
	kill(pid, SIGUSR1);
	
	return 0;
}
#else
#define restart_seri2eth()
#endif

#ifdef __DEBUG__
#define DEBUG_LINE()           LOG("DEBUG: L_%d\n", __LINE__)
#define DUMP_PACKET(x)         dump_packet(x)
#define DUMP_NETWORK_INFO(x)   dump_network_info(x)	

/**********************************************************************
 * summary : dump packet data
 * return  : -
 *********************************************************************/
static void dump_network_info(const network_info_s* net_info)
{
	unsigned long broadcast;

	LOG("--------------- NETWORK START --------------\n\n");
	/* use dhcp */
	if(net_info->use_dhcp){
		LOG("DHCP\tUSING DHCP\n");
	}
	else{
		LOG("DHCP\tSTATIC IP\n");
	}

	/* ip, broadcast, netmask, gateway */
	LOG("ip address\t: %s\n", inet_ntoa(TO_INADDR(net_info->ip_address)));
	LOG("net mask\t: %s\n", inet_ntoa(TO_INADDR(net_info->netmask)));
	broadcast = (net_info->ip_address | ~net_info->netmask);
	LOG("broad cast\t: %s\n", inet_ntoa(TO_INADDR(broadcast)));
	LOG("gate way\t: %s\n", inet_ntoa(TO_INADDR(net_info->gateway)));
	LOG("--------------- NETWORK END ----------------\n\n");
}

static void dump_packet(const discover_packet_s* packet)
{
	char* str;
	const network_info_s* net_info = &packet->net_info;

	LOG("--------------- PACKET START --------------\n\n");

	/* message type */
	switch(ntohl(packet->message_type)){
	case DISCOVER_REQ:
		str = "DISCOVER_REQ";
		break;
	case DISCOVER_REP:
		str = "DISCOVER_REP";
		break;
	case IP_OFFER_REQ:
		str = "IP_OFFER_REQ";
		break;
	case IP_OFFER_REP:
		str = "IP_OFFER_REP";
		break;
	default:
		str = "INVALID_MESSAGE_TYPE";
		break;
	}
	LOG("message type\t: %s\n", str);

	/* transaction ID */
	LOG("transaction ID\t: %d\n", packet->transaction_id);

	/* source mac address */
	LOG("mac addr\t: %02x:%02x:%02x:%02x:%02x:%02x\n",
		(unsigned char)packet->mac_addr[0],
		(unsigned char)packet->mac_addr[1],
		(unsigned char)packet->mac_addr[2],
		(unsigned char)packet->mac_addr[3],
		(unsigned char)packet->mac_addr[4],
		(unsigned char)packet->mac_addr[5]
	);

	/* result */
	LOG("result (reply)\t: %d\n", packet->result);

	dump_network_info(net_info);
	LOG("--------------- PACKET END ----------------\n\n");
}

#else
#define DUMP_NETWORK_INFO(x)
#define DUMP_PACKET(x)
#endif

/**********************************************************************
 * summary : return temprary socket descriptor
 * return  : socket desc
 *********************************************************************/
static int get_temp_socket(void)
{
	static int sock = -1;

	if(sock < 0){
		sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(sock < 0){
    		ERR("failed to create socket\n");
		}
	}
	
	return sock;
}

/**********************************************************************
 * summary : return local interface device mac address
 * return  : mac address
 *********************************************************************/
static const char* get_mac_address(void)
{
	static char local_mac_address[6];
	static int init = 0;
	if(!init){
		struct ifreq request;
		strcpy(request.ifr_name, DEVICE_NAME);
    	if(ioctl(get_temp_socket(), SIOCGIFHWADDR, &request) < 0){
        	PERROR(__FUNCTION__);
    	}else{
		memcpy(local_mac_address, request.ifr_hwaddr.sa_data, 6);
	}
	init = 1;
	}
	return local_mac_address;
}

/**********************************************************************
 * summary : get indicated network address by ioctl
 * return  : network address (failed -> 0)
 *********************************************************************/
static unsigned long get_address(
int cmd /* command of ioctl ex. SIOC... */
)
{
	struct sockaddr_in* addr_info;
	struct ifreq request;
	strcpy(request.ifr_name, DEVICE_NAME);

	/* ip address */
	if(ioctl(get_temp_socket(), cmd, &request) < 0){
		//PERROR(__FUNCTION__);
		return DEFAULT_IP;
	}

	addr_info = (struct sockaddr_in*)&request.ifr_addr;
	return addr_info->sin_addr.s_addr;
}

/**********************************************************************
 * summary : set indicated network address by ioctl
 * return  : success -> 0, failed -> -1
 *********************************************************************/
static int set_address(
	int cmd,			/* command of ioctl ex. SIOC... */
	unsigned long addr	/* network address to set */
)
{
	struct ifreq request;
	struct sockaddr_in* addr_info = (struct sockaddr_in*)&request.ifr_addr;
	addr_info->sin_family	= AF_INET;
	addr_info->sin_port		= 0;
	addr_info->sin_addr.s_addr = addr;

	strcpy(request.ifr_name, DEVICE_NAME);

	/* ip address */
	if(ioctl(get_temp_socket(), cmd, &request) < 0){
		PERROR(__FUNCTION__);
		return -1;
	}

	return 0;
}

/**********************************************************************
 * summary : get gateway address from proc file system and return it
 * return  : default gateway address (failed -> 0)
 *********************************************************************/
static unsigned long get_default_gateway(void)
{
	char buff[1024];

	/* open proc file system */
	FILE *fp = fopen(PROC_ROUTE_PATH, "r");
	if(!fp){
		ERR("failed to open proc file : %s\n", PROC_ROUTE_PATH);
		return DEFAULT_IP;
	}

	/* ignore first line (only format string) */
	if(!fgets(buff, sizeof(buff), fp)){
		ERR("EOF found\n");
		fclose(fp);
		return DEFAULT_IP;
	}

	/* find line it discribes default gateway */
	while(fgets(buff, sizeof(buff), fp)){
		const char* fmt = "%s %x %x";
		struct in_addr ip, gw;
		char name[64];
		int num = sscanf(buff, fmt, name, &ip, &gw);
		if(num != 3){
			ERR("sscanf() failed\n");
			continue;
		}

		if(!strncmp(name, DEVICE_NAME, DEVICE_NAME_LEN) &&
		gw.s_addr != 0){
			fclose(fp);
			return gw.s_addr;
		}
	}

	ERR("no default gw on : %s\n", DEVICE_NAME);

	fclose(fp);
	return DEFAULT_IP;
}

/**********************************************************************
 * summary : set default gateway address by ioctl
 * return  : -1 - failed, 0 - success, 1 : already exist
 *********************************************************************/
static int set_default_gateway(
	int cmd,                    /* command of ioctl ex. SIOC... */
	unsigned long default_gw    /* default gateway address */
)
{
	struct rtentry route;
	struct sockaddr_in address;

	if(!default_gw){
		// not indicated is same as OK
		return 0;
	}

	/* set parameter */
	bzero(&route, sizeof(route));
	route.rt_dev		= DEVICE_NAME;
	route.rt_flags		= RTF_UP | RTF_GATEWAY;
	address.sin_family	= AF_INET;
	address.sin_port	= 0;

	address.sin_addr.s_addr = INADDR_ANY;
	memcpy(&route.rt_dst, &address, sizeof(address));

	address.sin_addr.s_addr = default_gw;
	memcpy(&route.rt_gateway, &address, sizeof(address));

	if(ioctl(get_temp_socket(), cmd, &route) < 0){
		if(errno == EEXIST){
			return 1;
		}
		PERROR(__FUNCTION__);
		return -1;
	}

	return 0;
}

/**********************************************************************
 * summary : wake up dhcp client daemon
 * return  : process id of dhcp
 *********************************************************************/
static pid_t start_dhcp()
{
	pid_t dhcp_pid = vfork();

	if(dhcp_pid < 0){
		ERR("vfork() failed\n");
		return -1;
	}
	if(!dhcp_pid){
		LOG("Child process to wake dhcp started, cmd : %s %s\n",
			DHCP_CMD, DHCP_UP_PARAM);
		execv(DHCP_CMD, DHCP_UP_PARAM);
		_exit(0);
	}

	LOG("Child process ID : %d\n", dhcp_pid);
	return dhcp_pid;
}

/**********************************************************************
 * summary : kill dhcp client process
 * return  :
 *********************************************************************/
static void stop_dhcp(pid_t dhcp_pid)
{
	struct sigaction new, old;
	LOG("stop_dhcp()...: %d\n", dhcp_pid);
	memset(&new, 0, sizeof(new));
	sigemptyset(&new.sa_mask);
	sigaction(SIGCHLD, &new, &old);

	//kill(dhcp_pid, SIGTERM);
	system("killall dhcpcd");
	wait(NULL);
	sigaction(SIGCHLD, &old, 0);

	remove("/var/run/dhcpcd-eth0.pid");
}

/**********************************************************************
 * summary : get local network information then set to parameter
 * return  : success -> 0, failed -> -1
 *********************************************************************/
static int load_local_network_info(
	network_info_s* net_info
)
{
	char buff[256];
	FILE *fp;

	LOG("in load_local_network_info()\n");
	/* set default */
	memset(net_info, 0, sizeof(network_info_s));
	net_info->use_dhcp = 1;

	/* load setting from config file */
	fp = fopen(NETWORK_FILE_NAME, "r");
 	if(!fp){
		PERROR(__FUNCTION__);
		return -1;
	}

	while(fgets(buff, sizeof(buff), fp)){
		const char* fmt = "%s %s";
		char key[64];
		char value[64];
		
		int num = sscanf(buff, fmt, key, value);
		if(num != 2){
			continue;
		}

		LOG("setting, key : %s, value : %s\n", key, value);
		if(!strcmp(key, DHCP_KEY_STR)){
			net_info->use_dhcp = strcmp(value, "yes")? 0 : 1;
		}
		else if(!strcmp(key, IPADDRESS_KEY_STR)){
			inet_aton(value, (struct in_addr*)&net_info->ip_address);
		}
		else if(!strcmp(key, NETMASK_KEY_STR)){
			inet_aton(value, (struct in_addr*)&net_info->netmask);
		}
		else if(!strcmp(key, GATEWAY_KEY_STR)){
			inet_aton(value, (struct in_addr*)&net_info->gateway);
		}
	}

	fclose(fp);

	DUMP_NETWORK_INFO((const network_info_s*)net_info);

	return 0;
}

/**********************************************************************
 * summary : get local network information then set to parameter
 * return  : success -> 0, failed -> -1
 *********************************************************************/
static int update_local_network_info(
	network_info_s* net_info
)
{
	/* ip, net mask, default gw */
	net_info->ip_address    = get_address(SIOCGIFADDR);
	net_info->netmask       = get_address(SIOCGIFNETMASK);
	net_info->gateway       = get_default_gateway();

	return 0;
}

/**********************************************************************
 * summary : set default route address
 * return  : success -> 0, failed -> -1
 *********************************************************************/
static int set_default_route(void)
{
	struct rtentry route;
	struct sockaddr_in address;

	/* set parameter */
	bzero(&route, sizeof(route));
	address.sin_family	= AF_INET;
	address.sin_port	= 0;

	address.sin_addr.s_addr = INADDR_ANY;
	memcpy(&route.rt_dst, &address, sizeof(address));
	memcpy(&route.rt_genmask, &address, sizeof(address));

	route.rt_dev	= DEVICE_NAME;
	route.rt_flags	= RTF_UP;
	route.rt_metric = DEFAULT_RT_METRIC; /* default gw must be 0 or 1 */

	if(ioctl(get_temp_socket(), SIOCADDRT, &route) < 0){
		PERROR(__FUNCTION__);
		return -1;
	}

	return 0;
}

/**********************************************************************
 * summary : backup flash rom 
 * return  :
 *********************************************************************/
static void flash_backup(void)
{
	save_config();
	restart_seri2eth();
}
						    
/**********************************************************************
 * summary : save network information to file
 * return  : success -> 0, failed -> -1
 *********************************************************************/
static int save_network_info(
	const network_info_s* net_info	/* network information to save */
)
{
	char work_str[256];
	const char* fmt = "%s %s\n";

	FILE *fp = fopen(NETWORK_FILE_NAME, "w");
 	if(!fp){
		PERROR(__FUNCTION__);
		return -1;
	}

	/** error of fwrite() is not cared... */

	/* DHCP */
	if(net_info->use_dhcp){
		sprintf(work_str, fmt, DHCP_KEY_STR, "yes");
		fwrite(work_str, 1, strlen(work_str), fp);
	}
	else{

		sprintf(work_str, fmt, DHCP_KEY_STR, "no");
		fwrite(work_str, 1, strlen(work_str), fp);

		/* ip address */
		sprintf(work_str, fmt, IPADDRESS_KEY_STR,
					inet_ntoa(TO_INADDR(net_info->ip_address)));
		fwrite(work_str, 1, strlen(work_str), fp);

		/* netmask */
		sprintf(work_str, fmt, NETMASK_KEY_STR,
					inet_ntoa(TO_INADDR(net_info->netmask)));
		fwrite(work_str, 1, strlen(work_str), fp);

		/* gateway address */
		sprintf(work_str, fmt, GATEWAY_KEY_STR,
					inet_ntoa(TO_INADDR(net_info->gateway)));
		fwrite(work_str, 1, strlen(work_str), fp);
	}

	fclose(fp);

	flash_backup();

	return 0;
}

/**********************************************************************
 * summary : wake up network interface
 * return  : -
 *********************************************************************/
static void up_interface_dev(void)
{
	struct ifreq request;
	memset(&request, 0, sizeof(request));
	strcpy(request.ifr_name, DEVICE_NAME);

	/*
	 * up interface
	 * need IFF_PROMISC otherwise cann't reply response
	 * after using DHCP client
	 */
	request.ifr_flags |= IFF_UP | IFF_BROADCAST | IFF_RUNNING |
       						IFF_NOTRAILERS | IFF_MULTICAST;
	usleep(100000);
   	if(ioctl(get_temp_socket(), SIOCSIFFLAGS, &request) < 0){
		PERROR(__FUNCTION__);
   	}
	return;
}

/**********************************************************************
 * summary : initialize local network
 * return  : -
 *********************************************************************/
static void init_local_network(const network_info_s* local_net_info)
{
	if(local_net_info->use_dhcp){
		/* call dhcp client , set address -> 0.0.0.0 */
		set_address(SIOCSIFADDR, 0);
		g_dhcp_pid = start_dhcp(DHCP_UP_PARAM);
		LOG("dhcp started\n");
	}
	else{
		unsigned long broadcast = 
			local_net_info->ip_address | ~local_net_info->netmask;
		set_address(SIOCSIFADDR, local_net_info->ip_address);
		set_address(SIOCSIFNETMASK, local_net_info->netmask);
		set_address(SIOCSIFBRDADDR, broadcast);
		set_default_gateway(SIOCADDRT, local_net_info->gateway);
	}

	return;
}

/**********************************************************************
 * summary : change local network configuration by indicated info
 * return  : result_e (see discover_proto.h)
 *********************************************************************/
static result_e	change_network_setting(
	const network_info_s* new_net_info,  /* requested network conf */
	network_info_s* local_net_info       /* local network conf (IN/OUT) */
)
{
	unsigned long broadcast;

	if(new_net_info->use_dhcp){
		/*
		 * DHCP (call DHCP client)
		 */
		if(local_net_info->use_dhcp){
			return SUCCESS;
		}
		
		/* set address -> 0.0.0.0 */
		set_address(SIOCSIFADDR, 0);
		g_dhcp_pid = start_dhcp(DHCP_UP_PARAM);
		LOG("dhcp started\n");
	}
	else{
		/*
		 * STATIC IP
		 */
		if(set_address(SIOCSIFADDR, new_net_info->ip_address) < 0){
			ERR("failed to change ip address : %s\n",
				inet_ntoa(TO_INADDR(new_net_info->ip_address)));
			return SET_IP_ADDRESS_FAILED;
		}

		if(set_address(SIOCSIFNETMASK, new_net_info->netmask) < 0){
			ERR("failed to change net mask : %s\n",
				inet_ntoa(TO_INADDR(new_net_info->netmask)));
			init_local_network(local_net_info);
			return SET_SUBNETMASK_FAILED;
		}

		broadcast = new_net_info->ip_address | ~new_net_info->netmask;
		if(set_address(SIOCSIFBRDADDR, broadcast) < 0){
			ERR("failed to broadcast address : %s\n",
				inet_ntoa(TO_INADDR(broadcast)));
			init_local_network(local_net_info);
			return SET_BROAD_CAST_FAILED;
		}

		/* set default gw */
		switch(set_default_gateway(SIOCADDRT, new_net_info->gateway)){
		case 0:
			/* remove old gateway if any */
			if(local_net_info->gateway){
				LOG("removing old gateway\n");
				set_default_gateway(SIOCDELRT, local_net_info->gateway);
			}
			break;
		case 1:
			/* already set */
			LOG("default gateway already exist\n");
			break;
		default:
			ERR("failed to set gw address : %s\n",
				inet_ntoa(TO_INADDR(new_net_info->gateway)));
			init_local_network(local_net_info);
			return SET_DEFAULT_GW_FAILED;
		}

		/* kill if dhcp is exist, but all 'route' has gone after kill dhcp, 
		   need to set again in signal handler */
		if(g_dhcp_pid > 0){
			LOG("send kill message to child process\n");
			stop_dhcp(g_dhcp_pid); /* wait until killed */
			g_dhcp_pid = 0;
			up_interface_dev();
			init_local_network(new_net_info);
		}
	}

	/* set setting and save setting to file */
	memcpy(local_net_info, new_net_info, sizeof(network_info_s));
	save_network_info(local_net_info);

	return SUCCESS;
}

/**********************************************************************
 * summary : reply network configuration data
 * return  : -
 *********************************************************************/
static int send_raw_packet(int sock, packet_info *info,
			   void *packet, unsigned int packetlen){
	unsigned char sendbuf[256];
	int sendlen;
	int ret;
	int retry;
	
	sendlen = gen_send_packet(sendbuf, 256, info, packet, packetlen);

	/* send packet */
	for(retry = 10; retry > 0; retry--){
		ret = send(sock, sendbuf, sendlen, 0);
		if(ret < 0){
			if(retry > 1){
				up_interface_dev();
				sleep(1);
				continue;
			}
			PERROR(__FUNCTION__);
			ERR("sendto() failed\n");
			return -1;
		}
		break;
	}
	return 0;
}

/**********************************************************************
 * summary : reply network configuration data
 * return  : -
 *********************************************************************/
static void discover_req(
	int sock,                            /* UDP protocol socket */
	packet_info *info, 
	network_info_s* local_net_info,      /* local net work conf */
	const discover_packet_s* recv_packet /* received packet */
)
{
	discover_packet_s send_packet;

	DUMP_PACKET(recv_packet);

	update_local_network_info(local_net_info);

	/* make packet to send */
	bzero((char *)&send_packet, sizeof(send_packet));
	send_packet.message_type = htonl(DISCOVER_REP);
	memcpy(
			send_packet.mac_addr,
			get_mac_address(),
			MAC_ADDRESS_SIZE
			);
	send_packet.transaction_id = recv_packet->transaction_id;
	memcpy(&send_packet.net_info, local_net_info, sizeof(network_info_s));
	
	LOG("SEND PACKET DUMP\n");
	DUMP_PACKET(&send_packet);

	set_default_route();

	send_raw_packet(sock, info, &send_packet, sizeof(send_packet));
}

/**********************************************************************
 * summary : change network config by request and send reply
 * return  : -
 *********************************************************************/
static void ip_offer_req(
	int sock,
	packet_info *info,
	network_info_s* local_net_info,      /* local net work conf */
	const discover_packet_s* recv_packet /* received packet */
)
{
	discover_packet_s send_packet;
	memset(&send_packet, 0, sizeof(discover_packet_s));

	DUMP_PACKET(recv_packet);

	/* check if request is mine */
	if(memcmp(
		(void*)recv_packet->mac_addr,
		(void*)get_mac_address(),
		MAC_ADDRESS_SIZE
		)){
		return;
	}

	/* if mac address is same then setting */
	send_packet.result = 
		htonl(change_network_setting(&recv_packet->net_info,
					     local_net_info));

	/* make reply packet */
	send_packet.message_type = htonl(IP_OFFER_REP);
	send_packet.transaction_id = recv_packet->transaction_id;
	memcpy(send_packet.mac_addr, get_mac_address(), MAC_ADDRESS_SIZE);
	memcpy(&send_packet.net_info, local_net_info, sizeof(network_info_s));

	LOG("SEND PACKET DUMP\n");
	DUMP_PACKET(&send_packet);
	set_default_route();

	send_raw_packet(sock, info, &send_packet, sizeof(send_packet));
}

/**********************************************************************
 * summary : check received packet type to know request
 * return  : -
 *********************************************************************/
static int analysis_discover_packet(
	int sock,
	packet_info *info, /* raw packet info */
	network_info_s* local_net_info,/* local net work conf */
	const void *packet /* received packet data */
)
{
	discover_packet_s recv_packet;
	memcpy(&recv_packet, packet, sizeof(discover_packet_s));

	switch(ntohl(recv_packet.message_type)){
	case DISCOVER_REQ:
		discover_req(sock, info, local_net_info, &recv_packet);
		break;
	case IP_OFFER_REQ:
		ip_offer_req(sock, info, local_net_info, &recv_packet);
		return 1;
		break;
	default:
		ERR("invalid message type, id : %d\n",
			ntohl(recv_packet.message_type));
		break;
	}
	return 0;
}

/**********************************************************************
 * summary : check received packet type to know request
 * return  : DISCOVER_REQ detected -> 0, another -> -1
 *********************************************************************/
static int analysis_raw_packet(packet_info *info,
			       const void *_buf, const size_t len){
	unsigned char *buf = (unsigned char *)_buf;
	int ret;
	unsigned long ipaddr = get_address(SIOCGIFADDR);
  
	maccpy(info->mac, (unsigned char *)get_mac_address());
	ipaddrcpy(info->ipaddr, (unsigned char *)&ipaddr);

	ret = mac_packet_check(info, get_mac_address(), buf, len);
	if(ret == -1){
		return -1;
	}

	ret = ip_packet_check(info);
	if(ret == -1){
		return -1;
	}

	ret = udp_packet_check(info);
	if(ret == -1){
		return -1;
	}

	return 0;
}

/**********************************************************************
 * summary : set up raw-mode-socket desc
 * return  : socket desc. (failed -> -1)
 *********************************************************************/
static int setup_raw_socket(void){
	int raw_socket;
	struct sockaddr_ll sap;
	struct ifreq ifr;
	int flag = 1;
	int ret;

	memset(&ifr, 0, sizeof(struct ifreq));
	strcpy(ifr.ifr_name, DEVICE_NAME);
	
	raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(raw_socket == -1){
		ERR("Can't create socket: \n");
		return -1;
	}

	ret = ioctl(raw_socket, SIOCGIFHWADDR, &ifr);
	if(ret){
		ERR("ioctl SIOCGIFHWADDR: \n");
		return -1;
	}

	ret = setsockopt(raw_socket, SOL_SOCKET, SO_BROADCAST, 
			 (char *)&flag,sizeof(flag));
	if(ret == -1){
		ERR("Can't set SO_BROADCAST option on raw_socket\n");
		return -1;
	}
	
	ret = ioctl(raw_socket, SIOCGIFFLAGS, &ifr);
	if(ret == -1){
		ERR("ioctl SIOCGIFFLAGS: \n");
		return -1;
	}

	ifr.ifr_flags |= IFF_UP | IFF_BROADCAST | IFF_NOTRAILERS | IFF_RUNNING;
	ret = ioctl(raw_socket, SIOCSIFFLAGS, &ifr);
	if(ret == -1){
		ERR("ioctl SIOCSIFFLAGS: \n");
		return -1;
	}

	memset(&sap, 0, sizeof(struct sockaddr_ll));
	ret = ioctl(raw_socket, SIOCGIFINDEX, &ifr);
	if(ret == -1){
		ERR("ioctl SIOCGIFINDEX: \n");
		return -1;
	}
	sap.sll_family = AF_PACKET;
	sap.sll_protocol = 0;
	sap.sll_ifindex = ifr.ifr_ifindex;
	
	ret = bind(raw_socket, (void*)&sap, sizeof(struct sockaddr_ll));
	if(ret == -1){
		ERR("failes to bind address\n");
		return -1;
	}

	return raw_socket;
}

/**********************************************************************
 * summary : receive signal from dhcpcd 
 * return  :
 *********************************************************************/
void fin_notify(int sig)
{
	LOG("signal received, sig : %d\n", sig);
	if(sig == SIGCHLD){
		int tmp;
		tmp = waitpid(-1, NULL, WNOHANG);
		if(tmp <= 0) return;
		LOG("PID: %d child exited\n", tmp);
		if(tmp == g_dhcp_pid){
			up_interface_dev();
		}
	}
}

/**********************************************************************
 * summary : usage
 * return  :
 *********************************************************************/
void usage(void)
{
	printf("Usage: discoverd\n\n"
	       "  Configuration:\n    "
#if defined(__DEBUG__)
	       "__DEBUG__, "
#endif
#if defined(ENABLE_FLATFSD)
	       "ENABLE_FLATFSD, "
#endif
#if defined(ENABLE_FLATFSD_USR1)
	       "ENABLE_FLATFSD_USR1, "
#endif
#if defined(ENABLE_SERI2ETH)
	       "ENABLE_SERI2ETH, "
#endif
	       "\n");
}

/**********************************************************************
 * summary : main routine (run itself as daemon)
 * return  : success -> 0, failed -> !0
 *********************************************************************/
int main(int argc, char **argv){
	int raw_sock;
	network_info_s local_net_info;
	struct sigaction act;

	if(argc > 1){
		if(strcmp(argv[1], "--help") == 0){
			usage();
			return 0;
		}
	}

	// set signal to get from dhcpcd
	act.sa_handler = fin_notify;
	act.sa_flags   = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGCHLD, &act, 0);
	
	// work infinite loop-start
	for(;;){
	up_interface_dev();
	  
	raw_sock = setup_raw_socket();
	if(raw_sock == -1){
		ERR("setup_raw_socket() failed : %d\n", raw_sock);
		continue;
	}
	
	// call after interface up
	load_local_network_info(&local_net_info);
	init_local_network(&local_net_info);
	
	// in wait routine loop-start
	for(;;){
		int ret;      
		fd_set readfds;

		FD_ZERO(&readfds);
		FD_SET(raw_sock, &readfds);
		
		ret = select(raw_sock + 1, &readfds, NULL, NULL, NULL);
		if(ret < 0){
			ERR("select() failed\n");
			up_interface_dev();
			continue;
		}
		
		if(FD_ISSET(raw_sock, &readfds)){
		static char buf[256];
		int data_len;
		struct sockaddr_in from;
		int fromlen = sizeof(struct sockaddr_in);
		packet_info info;
		
		data_len = recvfrom(raw_sock, buf, 256, 0,
				    (struct sockaddr *)&from, &fromlen);
		if(data_len == -1){
			ERR("recvfrom() failed, result : %d\n", data_len);
			break;
		}
		
		memset(&info, 0 ,sizeof(packet_info));
		ret = analysis_raw_packet(&info, buf, data_len);
		
		if(ret == 0){
		ret = analysis_discover_packet(raw_sock, &info, 
					       &local_net_info, info.data);
		}
		} // FD_ISSET(raw_sock)
      
	} // in wait routine loop-end
    
	close(raw_sock);
    
	} // work infinite loop-end
 
	return 0;
}
