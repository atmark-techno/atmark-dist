#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "udpip.h"
#include "discover_proto.h"

#define ETH_PROTOCOL_IP 0x0800
#define IP_PROTOCOL_UDP 0x11

//static unsigned char _zero_ipaddr[] = {0x00, 0x00, 0x00, 0x00};
static unsigned char _broadcast_ipaddr[] = {0xff, 0xff, 0xff, 0xff};
static unsigned char _broadcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/**********************************************************************
 * summary : 
 * return  : 
 *********************************************************************/
int mac_packet_check(packet_info *info, const unsigned char *mac, 
		     const void *_buf, const size_t len){
	eth_header *eth_hdr = (eth_header *)_buf;
  
	//check packet_length
	if(ETH_HEADER_LEN > len){
		return -1;
	}

	//check destination
	if(!(
	     (maccmp(_broadcast_mac, eth_hdr->dst_mac) == 0) ||
	     (maccmp(info->mac, eth_hdr->dst_mac) == 0)
	    )
	  ){
		return -1;
	}

	//check protocol
	if(ETH_PROTOCOL_IP != ntohs(eth_hdr->protocol)){
		return -1;
	}
	
	info->packet_len = len;
	info->eth_hdr    = eth_hdr;
	info->ip_hdr     = (ip_header *)((unsigned long)eth_hdr +
					 ETH_HEADER_LEN);

	return 0;
}

/**********************************************************************
 * summary : 
 * return  : 
 *********************************************************************/
static unsigned short ip_checksum(const void *_buf,
                                  const unsigned int buflen){
	unsigned short *buf = (unsigned short *)_buf;
	unsigned long sum;
	int i;

	for(sum = 0, i = 0; i < (buflen / 2); i++){
		sum += buf[i];
	}

	if(buflen % 2){
		sum += (buf[i] & 0x00ff);
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	
	return (unsigned short)~sum;
}

/**********************************************************************
 * summary : 
 * return  : 
 *********************************************************************/
int ip_packet_check(packet_info *info){
	ip_header *ip_hdr = info->ip_hdr;

	unsigned short header_len;
	unsigned short checksum;

	//check packet length
	if(IP_HEADER_LEN > (info->packet_len - ETH_HEADER_LEN)){
		return -1;
	}

	//check protocol
	if(IP_PROTOCOL_UDP != ip_hdr->protocol){
		return -1;
	}

	//check destination ipaddr
	if(!(
	     (ipaddrcmp(_broadcast_ipaddr, ip_hdr->dst_ipaddr) == 0) ||
	     (ipaddrcmp(info->ipaddr, ip_hdr->dst_ipaddr) == 0)
	    )
	  ){
		return -1;
	}
	
	//check sum
	header_len = (ip_hdr->header_len << 2);
	checksum         = ip_hdr->checksum;
	ip_hdr->checksum = 0;
	if(checksum != ip_checksum(ip_hdr, header_len)){
		return -1;
	}

	if(header_len != IP_HEADER_LEN){
		info->ip_extopt     = (void *)((unsigned long)ip_hdr +
					       (header_len - IP_HEADER_LEN));
		info->ip_extopt_len = (header_len - IP_HEADER_LEN);
	}
	info->udp_hdr = (udp_header *)((unsigned long)ip_hdr + header_len);
	
	return 0;
}

/**********************************************************************
 * summary : 
 * return  : 
 *********************************************************************/
static unsigned short udp_checksum(const ip_pseudo_header *__pheader,
                                   const void *__buf,
                                   const unsigned int buflen){
	unsigned short *buf;
	unsigned long sum;
	int i;

	buf = (unsigned short *)__pheader;
	for(sum = 0, i = 0; i < 6; i++){
		sum += buf[i];
	}

	buf = (unsigned short *)__buf;
	for(i = 0; i < (buflen / 2); i++){
		sum += buf[i];
	}

	if(buflen % 2){
		sum += (buf[i] & 0x00ff);
	}

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	
	return (unsigned short)~sum;
}

/**********************************************************************
 * summary : 
 * return  : 
 *********************************************************************/
int udp_packet_check(packet_info *info){
	udp_header *udp_hdr = info->udp_hdr;
	ip_pseudo_header pheader;
	unsigned short checksum;

	//check packet length
	if(UDP_HEADER_LEN > (info->packet_len - ETH_HEADER_LEN -
			     IP_HEADER_LEN - info->ip_extopt_len)){
		return -1;
	}

	//check destination port
	if(SERVER_PORT != ntohs(udp_hdr->dst_port)){
		return -1;
	}

	//create pseudo header
	ipaddrcpy(pheader.dst_ipaddr, info->ip_hdr->dst_ipaddr);
	ipaddrcpy(pheader.src_ipaddr, info->ip_hdr->src_ipaddr);
	pheader.zero     = 0;
	pheader.protocol = IP_PROTOCOL_UDP;
	pheader.data_len = udp_hdr->data_len;

	//check sum
	checksum          = udp_hdr->checksum;
	udp_hdr->checksum = 0;
	if(checksum != udp_checksum(&pheader, udp_hdr,
				    ntohs(udp_hdr->data_len))){
		return -1;
	}

	info->data     = (void *)((unsigned long)udp_hdr + UDP_HEADER_LEN);
	info->data_len = info->packet_len - (ETH_HEADER_LEN + IP_HEADER_LEN - 
					info->ip_extopt_len + UDP_HEADER_LEN);
	
	return 0;
}

/**********************************************************************
 * summary : 
 * return  : 
 *********************************************************************/
int gen_send_packet(void *_buf, const unsigned int buflen,
		    packet_info *info,
		    const void *packet, const unsigned int packetlen){
	static unsigned short packet_id = 0;
	eth_header *eth_hdr = (eth_header *)_buf;
	ip_header  *ip_hdr;
	udp_header *udp_hdr;
	ip_pseudo_header pheader;
	unsigned char *data;

	//create eth header
	maccpy(eth_hdr->dst_mac, _broadcast_mac);
	maccpy(eth_hdr->src_mac, info->mac);
	eth_hdr->protocol = htons(ETH_PROTOCOL_IP);

	//create ip header
	ip_hdr  = (ip_header *)((unsigned long)eth_hdr + ETH_HEADER_LEN);
	ip_hdr->version = 4;
	ip_hdr->diffserv = 0;
	ip_hdr->header_len = IP_HEADER_LEN >> 2;
	ip_hdr->data_len = htons(IP_HEADER_LEN + UDP_HEADER_LEN + packetlen);
	ip_hdr->id = htons(packet_id++);
	ip_hdr->flag = htons(0x4000);
	ip_hdr->ttl = 64;
	ip_hdr->protocol = IP_PROTOCOL_UDP;
	ipaddrcpy(ip_hdr->src_ipaddr, info->ipaddr);
	ipaddrcpy(ip_hdr->dst_ipaddr, _broadcast_ipaddr);  
	ip_hdr->checksum = 0;
	ip_hdr->checksum = ip_checksum(ip_hdr, IP_HEADER_LEN);

	udp_hdr = (udp_header *)((unsigned long)ip_hdr + IP_HEADER_LEN);
	data    = (unsigned char *)((unsigned long)udp_hdr + UDP_HEADER_LEN);

	//data copy
	memcpy(data, packet, packetlen);

	//create pseudo header
	ipaddrcpy(pheader.dst_ipaddr, ip_hdr->dst_ipaddr);
	ipaddrcpy(pheader.src_ipaddr, ip_hdr->src_ipaddr);
	pheader.zero = 0;
	pheader.protocol = ip_hdr->protocol;
	pheader.data_len = htons(UDP_HEADER_LEN + packetlen);
	
	//create udp header
	udp_hdr->src_port = htons(SERVER_PORT);
	udp_hdr->dst_port = htons(CLIENT_PORT);
	udp_hdr->data_len = htons(UDP_HEADER_LEN + packetlen);
	udp_hdr->checksum = 0;
  
	udp_hdr->checksum = udp_checksum(&pheader, udp_hdr,
					 UDP_HEADER_LEN + packetlen);

	return ETH_HEADER_LEN + IP_HEADER_LEN + UDP_HEADER_LEN + packetlen;  
}


