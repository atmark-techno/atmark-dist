/*
 * at-cgi-core/at-cgi-utils.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <simple-cgi-app-str.h>
#include <simple-cgi-app-llist.h>

#ifndef ATCGIUTILS_H_
#define ATCGIUTILS_H_

#define IP_TYPE_DHCP			(1)
#define IP_TYPE_STATIC		(2)

#define PROCESS_STATE_ACTIVE	(1)
#define PROCESS_STATE_INACTIVE	(0)

#define MAX_NET_INFO_TEXT_LEN	(128)
typedef struct {
	int use_dhcp;
	char address[MAX_NET_INFO_TEXT_LEN];
	char netmask[MAX_NET_INFO_TEXT_LEN];
	char broadcast[MAX_NET_INFO_TEXT_LEN];
	char gateway[MAX_NET_INFO_TEXT_LEN];
	char nameserver[MAX_NET_INFO_TEXT_LEN];
} networkInfo;
#define NETWORK_INFO_CHAR_MEMBERS	(5)

extern void print_uptime(void);
extern void print_5min_load(void);
extern void print_total_ram(void);

extern void print_ifconfig_part(const char *interf, const char *part_label);
extern int is_process_alive(char *process_name);
extern void print_process_state(char *process_name);
extern int get_username(char *username, int len);
extern int get_current_network_info(const char *iface, networkInfo *nework_info);
extern int save_network_info(const char *iface, networkInfo *network_info);
extern int get_sys_load(float *avg1, float *avg2, float *avg3);

extern int sudo_string_to_file(char *path, const char *string);

extern int required_reboot_secs(const char *product_name, int first_boot);

extern int is_hostname(char *hostname);

extern void print_current_effective_ip_address(char *current_iface);

extern char *get_broadcast_address(char *address_s, char *netmask_s);
extern int is_same_net_segment(char *address_1_s, char *address_2_s, char *netmask_s);
extern int is_valid_ip_class(char *address_s, char *netmask_s);

#define WAIT_FOR_IFACE_DOWN		(1)
#define NO_WAIT_FOR_IFACE_DOWN	(0)
extern void networking_down(char *iface, int wait_for_down);
extern void networking_up(char *iface);

extern cgiStrList *interface_list_get(cgiStrList *possible_matches);

#ifdef BRIDGE

#define BRIDGE_CONFIG_ENTRY_LEN_MAX		(64)

typedef struct {
	int create_br;
	char bridge_name[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	cgiStrList *bridged_ifaces;
	cgiStrList *iface_matches;
	char ageing[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	int stp_on;
	char br_prio[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	char forward[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	char gc_int[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	char hello[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	char max_age[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	cgiStrPairList *port_costs;
	cgiStrPairList *port_prios;
} bridgeConfig;

extern bridgeConfig *bridge_config_load(void);
extern void bridge_config_free(bridgeConfig *bridge_config);

#endif

#ifdef DEBUG
extern void serial_debug(const char *message);
#define at_cgi_debug(message) serial_debug(message)
#else
#define at_cgi_debug(message)
#endif

#endif /*ATCGIUTILS_H_*/
