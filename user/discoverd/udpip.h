#ifndef __DISCOVER_UDPIP_H__
#define __DISCOVER_UDPIP_H__

#include <sys/types.h>

typedef struct _ip_pseudo_header{
	unsigned char dst_ipaddr[4];
	unsigned char src_ipaddr[4];
	unsigned char zero;
	unsigned char protocol;
	unsigned short data_len;
}ip_pseudo_header;

#define UDP_HEADER_LEN 8
typedef struct _udp_header{
	unsigned short src_port;
	unsigned short dst_port;
	unsigned short data_len;
	unsigned short checksum;
}udp_header;

#define IP_HEADER_LEN 20
typedef struct _ip_header{
	unsigned char  header_len:4,
	               version:4;
	unsigned char  diffserv;
	unsigned short data_len;
	unsigned short id;
#if 0
	unsigned short flag_offset:13,
	               flag:3;
#endif
	unsigned short flag;
	unsigned char  ttl;//time to live
	unsigned char  protocol;
	unsigned short checksum;
	unsigned char  src_ipaddr[4];
	unsigned char  dst_ipaddr[4];
}ip_header;

#define ETH_HEADER_LEN 14
typedef struct _eth_header{
	unsigned char dst_mac[6];
	unsigned char src_mac[6];
	unsigned short protocol;
}eth_header;

typedef struct _udp_ip_eth_info{
	unsigned char mac[6];
	unsigned char ipaddr[4];
	unsigned int packet_len;
	eth_header   *eth_hdr;
	ip_header    *ip_hdr;
	void         *ip_extopt;
	unsigned int ip_extopt_len;
	udp_header   *udp_hdr;
	void         *data;
	unsigned int data_len;
}packet_info;


#define maccpy(__dst, __src)                         \
({                                                   \
	unsigned char *dst = (unsigned char *)__dst; \
	unsigned char *src = (unsigned char *)__src; \
	int i;                                       \
 	for(i=0;i<6;i++){                            \
		dst[i] = src[i];                     \
	}                                            \
	i;                                           \
})

#define ipaddrcpy(__dst, __src)                      \
({                                                   \
	unsigned char *dst = (unsigned char *)__dst; \
	unsigned char *src = (unsigned char *)__src; \
	int i;                                       \
	for(i=0;i<4;i++){                            \
		dst[i] = src[i];                     \
	}                                            \
	i;                                           \
})

#define maccmp(__dst, __src)                         \
({                                                   \
	unsigned char *dst = (unsigned char *)__dst; \
	unsigned char *src = (unsigned char *)__src; \
	int i;                                       \
	int err = 0;                                 \
	for(i=0;i<6;i++){                            \
		if(dst[i] != src[i]){                \
			err=1;                       \
			break;                       \
		}                                    \
	}                                            \
	err;                                         \
})

#define ipaddrcmp(__dst, __src)                      \
({                                                   \
	unsigned char *dst = (unsigned char *)__dst; \
	unsigned char *src = (unsigned char *)__src; \
	int i;                                       \
	int err = 0;                                 \
	for(i=0;i<4;i++){                            \
		if(dst[i] != src[i]){                \
			err=1;                       \
			break;                       \
		}                                    \
	}                                            \
	err;                                         \
})

int mac_packet_check(packet_info *info, const unsigned char *mac, 
		     const void *_buf, const size_t len);
int ip_packet_check(packet_info *info);
int udp_packet_check(packet_info *info);

int gen_send_packet(void *_buf, const unsigned int buflen,
		    packet_info *recv_info,
		    const void *packet, const unsigned int packetlen);
#endif
