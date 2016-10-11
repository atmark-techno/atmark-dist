#ifndef __DISCOVER_PROTO_H__
#define __DISCOVER_PROTO_H__

/*
********* DISCOVER DATAGRAM **********
	DISCOVER_REQ
		message type : 4byte
		client mac   : 8byte
	DISCOVER_RES
		message type : 4byte
		client mac   : 8byte
		server mac   : 8byte
                use dhcp     : 4byte
		ip address   : 4byte
		broad cast   : 4byte
                gate way     : 4byte
	IP_OFFER_REQ
		message type : 4byte
		client mac   : 8byte
		server mac   : 8byte
                use dhcp     : 4byte
		ip address   : 4byte
		broad cast   : 4byte
                gate way     : 4byte
	IP_OFFER_RES
		message type : 4byte
		client mac   : 8byte
		server mac   : 8byte
                use dhcp     : 4byte
		ip address   : 4byte
		broad cast   : 4byte
                gate way     : 4byte
*/
		
#include <syslog.h>

#define SERVER_PORT	22222
#define CLIENT_PORT	22223

/* debug macro */
#ifdef __DEBUG__
#define LOG(args...)    printf(args);fflush(stdout)
#define ERR(args...)    printf(args);fflush(stdout)
#define PERROR(label)   perror(label);fflush(stdout)
#else
#define LOG(args...)
#define ERR(args...)
#define PERROR(label)
#endif

typedef enum result
{
	SUCCESS = 0,
	SET_IP_ADDRESS_FAILED = 1,
	SET_SUBNETMASK_FAILED = 2,
	SET_BROAD_CAST_FAILED = 3,
	SET_DEFAULT_GW_FAILED = 4,
}result_e;

typedef enum message_type
{
	DISCOVER_REQ = 0,
	DISCOVER_REP = 1,
	IP_OFFER_REQ = 2,
	IP_OFFER_REP = 3,	
	INVALID_MESSAGE_TYPE = 4,
}message_type_e;

typedef struct network_info
{
	unsigned long use_dhcp;
	unsigned long ip_address;
	unsigned long netmask;
	unsigned long gateway;
}network_info_s;

typedef struct discover_packet
{
	message_type_e message_type;
	unsigned long transaction_id;
	char mac_addr[8];
	unsigned long result;
	network_info_s net_info;
}discover_packet_s;

#endif
