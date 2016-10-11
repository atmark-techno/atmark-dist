/*
 * modules/system_display.c - system module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef SYSTEM_DISPLAY_H_
#define SYSTEM_DISPLAY_H_

#include <simple-cgi-app-html-parts.h>

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX			(256)
#endif
#define USERNAME_MAX			(128)

#define ELE_ID_USERNAME 		"username"
#define ELE_ID_CURR_PASSWORD	"curr_pass"
#define ELE_ID_PASSWORD1 		"password1"
#define ELE_ID_PASSWORD2 		"password2"
#define ELE_ID_ZEROCONF 		"zeroconf"
#define ELE_ID_HOSTNAME 		"hostname"

#define ELE_ID_IP_ASSIGN_TYPE	"ip_assign_type"
#define ELE_ID_IP_DHCP			"dhcp"
#define ELE_ID_IP_STATIC		"static"
#define ELE_ID_IP_ADDRESS		"ip_address"
#define ELE_ID_IP_NETMASK		"ip_netmask"
#define ELE_ID_IP_GATEWAY		"ip_gateway"
#define ELE_ID_IP_DNSSERVER		"ip_dnsserver"

#define ELE_ID_ACTIVE			"active"
#define ELE_ID_INACTIVE			"inactive"

#define ELE_ID_USERLAND_PATH	"userland_path"
#define ELE_ID_KERNEL_PATH		"kernel_path"
#define ELE_ID_USERLAND_PATH_H	"userland_path_h"
#define ELE_ID_KERNEL_PATH_H	"kernel_path_h"
#define ELE_ID_USERLAND_EDIT	"userland_edit"
#define ELE_ID_KERNEL_EDIT		"kernel_edit"
#define ELE_ID_USERLAND_SELECT	"userland_select"
#define ELE_ID_KERNEL_SELECT	"kernel_select"
#define ELE_ID_IMAGE_NEW		"new"
#define ELE_ID_IMAGE_SPECIFIED	"specified"
#define ELE_ID_FIRMWARE_URL		"firmware_url"
#define ELE_ID_ALLOW_OTHER_PRODUCTS	"allow_other_products"

#define ELE_ID_FIREWALL_IN_POLICY			"firewall_in_policy"
#define ELE_ID_FIREWALL_IN_POLICY_ACCEPT	"firewall_in_policy_accept"
#define ELE_ID_FIREWALL_IN_POLICY_DROP		"firewall_in_policy_drop"
#define ELE_ID_FIREWALL_OUT_POLICY			"firewall_out_policy"
#define ELE_ID_FIREWALL_OUT_POLICY_ACCEPT	"firewall_out_policy_accept"
#define ELE_ID_FIREWALL_OUT_POLICY_DROP		"firewall_out_policy_drop"
#define ELE_ID_FIREWALL_FORWD_POLICY		"firewall_forwd_policy"
#define ELE_ID_FIREWALL_FORWD_POLICY_ACCEPT	"firewall_forwd_policy_accept"
#define ELE_ID_FIREWALL_FORWD_POLICY_DROP	"firewall_forwd_policy_drop"
#define ELE_ID_FIREWALL_IP_FORWD			"firewall_ip_forwd"
#define ELE_ID_FIREWALL_IN_OK_SERVICES		"firewall_in_ok_services"
#define ELE_ID_FIREWALL_OUT_OK_SERVICES		"firewall_out_ok_services"
#define ELE_ID_FIREWALL_FORWD_OK_SERVICES	"firewall_forwd_ok_services"

#define ELE_ID_FIREWALL_RULE_PROTOCOL		"firewall_rule_protocol"
#define ELE_ID_FIREWALL_RULE_TCP			"firewall_rule_protocol_tcp"
#define ELE_ID_FIREWALL_RULE_UDP			"firewall_rule_protocol_udp"
#define ELE_ID_FIREWALL_RULE_ICMP			"firewall_rule_protocol_icmp"
#define ELE_ID_FIREWALL_RULE_SERVICE		"firewall_rule_service"
#define ELE_ID_FIREWALL_RULE_SERVICE_KNOWN	"firewall_rule_service_well_know"
#define ELE_ID_FIREWALL_RULE_SERVICE_PORT	"firewall_rule_service_port"
#define ELE_ID_FIREWALL_RULE_KNOWN			"firewall_rule_well_know"
#define ELE_ID_FIREWALL_RULE_PORT			"firewall_rule_port"
#define ELE_ID_FIREWALL_RULE_IFACE_IN		"firewall_rule_iface_in"
#define ELE_ID_FIREWALL_RULE_IFACE_OUT		"firewall_rule_iface_out"
#define ELE_ID_FIREWALL_RULE_RULE_NUMBER	"firewall_rule_rule_number"
#define ELE_ID_FIREWALL_RULE_CHAIN			"firewall_rule_chain"

#define ACTION_OVERVIEW				"overview"
#define ACTION_OVERVIEW_IFCONFIG	"overview_ifconfig"
#define ACTION_OVERVIEW_MEMINFO		"overview_meminfo"
#define ACTION_OVERVIEW_MESSAGES	"overview_messages"

#define ACTION_NETWORK				"network"
#define ACTION_NETWORK_SAVE			"network_save"
#define ACTION_NETWORK_CANCEL		"network_cancel"

#define ACTION_FIREWALL						"firewall"
#define ACTION_FIREWALL_UPDATE				"firewall_update"
#define ACTION_FIREWALL_CANCEL				"firewall_cancel"
#define ACTION_FIREWALL_SHOW_SCRIPT			"firewall_show_script"
#define ACTION_FIREWALL_IN_DEL				"firewall_in_del"
#define ACTION_FIREWALL_IN_EDIT				"firewall_in_edit"
#define ACTION_FIREWALL_IN_ADD				"firewall_in_add"
#define ACTION_FIREWALL_OUT_DEL				"firewall_out_del"
#define ACTION_FIREWALL_OUT_EDIT			"firewall_out_edit"
#define ACTION_FIREWALL_OUT_ADD				"firewall_out_add"
#define ACTION_FIREWALL_FORWD_DEL			"firewall_forwd_del"
#define ACTION_FIREWALL_FORWD_EDIT			"firewall_forwd_edit"
#define ACTION_FIREWALL_FORWD_ADD			"firewall_forwd_add"
#define ACTION_FIREWALL_RULE_CANCEL			"firewall_rule_cancel"
#define ACTION_FIREWALL_RULE_OK_ADD			"firewall_rule_ok_add"
#define ACTION_FIREWALL_RULE_OK_EDIT		"firewall_rule_ok_edit"

#define ACTION_BRIDGE				"bridge"

#define ACTION_PASSWORD				"password"
#define ACTION_PASSWORD_SAVE		"password_save"
#define ACTION_PASSWORD_CANCEL		"password_cancel"

#define ACTION_UPDATES				"updates"
#define ACTION_UPDATES_GET_OPT		"update_get_opt"
#define ACTION_UPDATES_USERLAND		"update_userland"
#define ACTION_UPDATES_KERNEL		"update_kernel"
#define ACTION_UPDATES_URL			"update_url"
#define ACTION_UPDATES_URL_SAVE		"update_url_save"
#define ACTION_UPDATES_URL_CANCEL	"update_url_cancel"

#define ACTION_UPDATES_CHECK		"update_check"
#define ACTION_UPDATES_REBOOT		"update_reboot"

#define ACTION_SYSTEM_STATE			"system_state"
#define ACTION_SYSTEM_STATE_SAVE	"system_state_save"
#define ACTION_SYSTEM_STATE_LOAD	"system_state_load"
#define ACTION_SYSTEM_STATE_INIT	"system_state_init"
#define ACTION_SYSTEM_STATE_REBOOT	"system_state_reboot"

#define FIREWALL_CONF_PATH			CONFIG_PATH"/at-admin-firewall"
#define FIREWALL_SCRIPT_CONF_PATH	"/etc/config/firewall"
#define FIREWALL_SCRIPT_PATH		"/etc/init.d/firewall"
#define FIREWALL_TEMP_CONF_PATH		"/tmp/at-admin/firewall"
#define WELL_KNOWN_SERVICES_PATH	"/etc/services"

#define FIREWALL_CONF_ELE_PRE			"PRE:"
#define FIREWALL_CONF_ELE_POLICY		"POLICY-"
#define FIREWALL_CONF_ELE_OPTION		"OPTION-"
#define FIREWALL_CONF_ELE_DEFAULT_RULE	"DEFAULT-RULE-"
#define FIREWALL_CONF_ELE_USER_RULE		"USER-RULE-"
#define FIREWALL_CONF_ELE_IFACE_MATCH	"INTERFACE-MATCH:"
#define FIREWALL_CONF_LINE_WELL_KNOWN	"wk"
#define FIREWALL_CONF_LINE_DIRECT		"direct"

#define FIREWALL_ROW_IN				"firewall_row_in_"
#define FIREWALL_ROW_OUT			"firewall_row_out_"
#define FIREWALL_ROW_FORWD			"firewall_row_forwd_"

typedef struct {
	cgiStrPairList *options;
	cgiStrList *pre;
	cgiStrPairList *policies;
	cgiStrPairLL *def_rules_in;
	cgiStrPairLL *user_rules_in;
	cgiStrPairLL *def_rules_out;
	cgiStrPairLL *user_rules_out;
	cgiStrPairLL *def_rules_forwd;
	cgiStrPairLL *user_rules_forwd;
	cgiStrList *iface_matches;
} firewallConfig;

extern void display_sub_menu(char *selected_link);

extern void display_system_overview(void);

extern void display_network_settings(
	networkInfo *network_info,
	char *hostname,
	char *err1,
	char *message);

extern void display_password_settings(
	char *username,
	char *curr_pass,
	char *new_pass,
	char *pass_check,
	char *err1,
	char *message);

extern void display_update_in_progress(void);

typedef enum {
	SU_MSG_NONE,
	SU_MSG_GET_OPTIONS,
	SU_MSG_USERLAND_PATH,
	SU_MSG_KERNEL_PATH,
	SU_MSG_UPDATE_INFO,
} softwareUpdateMsgType;

extern void display_software_update_clear(void);
extern void display_software_update_options(cgiStrPairList *userland_options, cgiStrPairList *kernel_options);
extern void display_software_update_msg(softwareUpdateMsgType msg_type, char *message);

extern void display_url_change(char *url, char *err1);

extern void display_system_state_settings(char * message);

extern void display_system_state_settings_change(void);

extern void display_network_change(networkInfo *network_info);

extern void display_ifconfig(void);

extern void display_meminfo(void);

#ifdef SYSLOGD
extern void display_messages(void);
#endif

extern firewallConfig *firewall_config_load_auto(void);
extern void firewall_config_free(firewallConfig* firewall_config);
extern void firewall_display_config_screen(char *top_message);
extern void firewall_display_rule_config_screen(char *chain, char *ok_action, cgiStrPairList *rule_list,
		int rule_number, cgiStrList *iface_matches, char *err1);
extern void firewall_config_print_script(FILE *fp, firewallConfig *firewall_config);
extern void firewall_display_script(void);

#ifdef BRIDGE

#include "misc-utils.h"

#define BRIDGE_NAME					"br0"
#define BR_DEFAULT					"DEFAULT"

#define ACTION_BRIDGE_STATUS		"bridge_status"
#define ACTION_BRIDGE_CONFIG		"bridge_config"
#define ACTION_BRIDGE_SAVE			"bridge_save"
#define ACTION_BRIDGE_CANCEL		"bridge_cancel"

#define ELE_ID_ACTIVE_BRIDGE		"activate_bridge"
#define ELE_ID_ENABLE_STP			"enable_stp"

#define ELE_ID_MAC_AGEING_AUTO		"mac_ageing_auto"
#define ELE_ID_MAC_AGEING_TEXT		"mac_ageing_text"

#define ELE_ID_BRIDGE_PRIO_AUTO		"bridge_prio_auto"
#define ELE_ID_BRIDGE_PRIO_TEXT		"bridge_prio_text"

#define ELE_ID_FORWARDING_AUTO		"forwarding_auto"
#define ELE_ID_FORWARDING_TEXT		"forwarding_text"

#define ELE_ID_GC_INT_AUTO			"gcint_auto"
#define ELE_ID_GC_INT_TEXT			"gcint_text"

#define ELE_ID_HELLO_TIME_AUTO		"hello_time_auto"
#define ELE_ID_HELLO_TIME_TEXT		"hello_time_text"

#define ELE_ID_MAX_AGE_AUTO			"max_age_auto"
#define ELE_ID_MAX_AGE_TEXT			"max_age_text"

#define ELE_ID_PORT_USE_BASE		"port_"
#define ELE_ID_COST_AUTO_TAIL		"_cost_auto"
#define ELE_ID_COST_TEXT_TAIL		"_cost_text"
#define ELE_ID_PRIO_AUTO_TAIL		"_prio_auto"
#define ELE_ID_PRIO_TEXT_TAIL		"_prio_text"

extern void display_sub_menu(char *selected_link);
extern void display_bridge_config_change(void);
extern void display_bridge_state(void);
extern void display_bridge_settings(bridgeConfig *bridge_config, char *err1, char *err2);
extern int bridge_config_is_entry_default(const char *bridge_config_entry);

#endif

#endif /*SYSTEM_DISPLAY_H_*/
