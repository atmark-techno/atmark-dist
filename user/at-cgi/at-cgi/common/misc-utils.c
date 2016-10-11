/*
 * at-cgi-core/misc-utils.c - utils for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <simple-cgi-app-str.h>
#include <simple-cgi-app-io.h>
#include <simple-cgi-app-alloc.h>
#include <simple-cgi-app-misc.h>

#include "misc-utils.h"
#include "core.h"
#include "system-config.h"

#define STANDARD_REBOOT_TIME	(40)

#define MAX_STR_LEN			(1024)
#define MAX_STR_LEN_S		(128)

#ifdef DEBUG
#define SERIAL_PORT			"/dev/ttyS0"
#endif

#define SSH_KEY_GEN_TIME		(90)

int sudo_string_to_file(char *path, const char *string)
{
	int ret, temp_fd;
	FILE *temp_fp;
	char template[] = "atcgi_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, CP_PATH, template, "", NULL};

	copy_args[3] = path;

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		syslog(LOG_ERR, "at-cgi: failed to open temp file");
		return -1;
	}

	ret = fprintf(temp_fp, string);
	if (ret != strlen(string)) {
		syslog(LOG_ERR, "at-cgi: failed to write to temp file");
		fclose(temp_fp);
		unlink(template);
		return -1;
	}
	fclose(temp_fp);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to copy temp file");
		return -1;
	}
	unlink(template);

	return 0;
}

void print_uptime(void)
{
	int ret, min, hours, days, secs;
	struct sysinfo info;

	ret = sysinfo(&info);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: sysinfo() failed");
		return;
	}

	days = info.uptime / (24 * 60 * 60);
	info.uptime -= (days * (24 * 60 * 60));
	hours = info.uptime / (60 * 60);
	info.uptime -= hours * 60 * 60;
	min = info.uptime / 60;
	info.uptime -= min * 60;
	secs = info.uptime;

	printf("%d days, %d hours, %d min, %d sec", days, hours, min, secs);
}

void print_ifconfig_part(const char *interf, const char *part_label)
{
	int ret, i;
	char *interf_start, *part_label_start, *interf_end;
	cgiStr *if_info;
	char *args[] = {"ifconfig", "-a", NULL};

	if_info = cgi_str_new(NULL);

	ret = cgi_read_command(&if_info, "/sbin/ifconfig", args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: command_to_str() failed");
		cgi_str_free(if_info);
		return;
	}

	interf_start = strstr(cgi_str_get(if_info), interf);
	if (interf_start == NULL) {
		syslog(LOG_ERR, "at-cgi: parse failure");
		return;
	}

	interf_end = strstr(interf_start, "\n\n");
	if (interf_end != NULL) {
		interf_end[0] = '\0';
	}
	part_label_start = strstr(interf_start, part_label);
	if (part_label_start == NULL) {
		syslog(LOG_ERR, "at-cgi: parse failure");
		return;
	}
	i = strlen(part_label);
	while (part_label_start[i] != ' ' && if_info->str[i] != '\n') {
		i++;
	}
	part_label_start[i] = '\0';

	printf("%s", part_label_start + strlen(part_label));

	cgi_str_free(if_info);
}

int is_process_alive(char *process_name)
{
	int ret;
	char *args[] = {"pidof", "", NULL};
	cgiStr *pidof_output;

	pidof_output = cgi_str_new(NULL);

	args[1] = process_name;

	ret = cgi_read_command(&pidof_output, PIDOF_PATH, args);
	if (ret < 0) {
		cgi_str_free(pidof_output);
		return PROCESS_STATE_INACTIVE;
	}

	if (strcmp(cgi_str_get(pidof_output), "\n") == 0) {
		cgi_str_free(pidof_output);
		return PROCESS_STATE_INACTIVE;
	} else {
		cgi_str_free(pidof_output);
		return PROCESS_STATE_ACTIVE;
	}
}

void print_process_state(char *process_name)
{
	int ret = is_process_alive(process_name);

	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: get_process_state() failed");
		return;
	} else if (ret == PROCESS_STATE_ACTIVE) {
		printf("Active");
	} else {
		printf("Inactive");
	}
}

static int get_first_nameserver(char *nameserver_buf, int len)
{
	int found_no;
	const char format[] = "nameserver %s";
	char buffer[MAX_STR_LEN_S];
	char value[MAX_STR_LEN_S];
	FILE *fp;

	if (!buffer) {
		return -1;
	}

	nameserver_buf[0] = '\0';

	fp = fopen(RESOLV_CONF_PATH, "r");
	if (fp == NULL) {
		return 0;
	}

	while (fgets(buffer, sizeof(buffer), fp)) {

		if (buffer[0] == '#') {
			continue;
		}

		found_no = sscanf(buffer, format, value);
		if(found_no != 1){
			continue;
		}

		strncpy(nameserver_buf, value, len);
		break;

	}

	fclose(fp);
	return 0;
}

int get_current_network_info(const char *iface, networkInfo *network_info)
{
	int in_options = 0, finished_options = 0, found_no, ret;
	int got_addr = 0, got_net = 0;
	char buffer[MAX_STR_LEN_S];
	const char* format = "%s %s";
	char key[MAX_STR_LEN_S];
	char value[MAX_STR_LEN_S];
	FILE *fp;
	memset(network_info, 0, sizeof(networkInfo));

	fp = fopen(INTERFACES_PATH, "r");
	if (fp == NULL) {
		return -1;
	}
	while (fgets(buffer, sizeof(buffer), fp)) {

		if (buffer[0] == '#') {
			continue;
		}

		if (!in_options) {

			if (strncmp(buffer, "iface", 5) == 0 && strstr(buffer, iface)) {
				if (strstr(buffer, "dhcp")) {
					network_info->use_dhcp = 1;
					fclose(fp);
					return 0;
				} else if (strstr(buffer, "static")) {
					in_options = 1;
				}
			}
		} else {

			if (buffer[0] == '\n') {
				finished_options++;
			}

			found_no = sscanf(buffer, format, key, value);
			if(found_no != 2){
				continue;
			}

			if (strcmp(key, "address") == 0) {
				strncpy(network_info->address, value, sizeof(network_info->address)-1);
				got_addr = 1;
			} else if (strcmp(key, "netmask") == 0) {
				strncpy(network_info->netmask, value, sizeof(network_info->netmask)-1);
				got_net = 1;
			} else if (strcmp(key, "gateway") == 0) {
				if (strcmp(value, "0.0.0.0") != 0) {
					strncpy(network_info->gateway, value, sizeof(network_info->gateway)-1);
				}
			}

		}
		if (finished_options) {
			break;
		}

	}

	fclose(fp);
	if (got_addr && got_net) {
		ret = get_first_nameserver(network_info->nameserver,
			sizeof(network_info->nameserver)-1);
		if (ret < 0) {
			return -1;
		}
		return 0;
	}

	return -1;
}

static int save_first_nameserver(const char *nameserver)
{
	int ret;
	char line_buffer[MAX_STR_LEN_S];

	if (!nameserver) {
		return -1;
	}

	if (nameserver[0] == '\0') {
		line_buffer[0] = '\0';
	} else {
		snprintf(line_buffer, sizeof(line_buffer), "search local-network\nnameserver %s\n", nameserver);
	}

	ret = sudo_string_to_file(RESOLV_CONF_PATH, line_buffer);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

char *get_broadcast_address(char *address_s, char *netmask_s)
{
	struct in_addr inp;
	unsigned long address;
	unsigned long netmask;
	unsigned long broadcast;

	inet_aton(address_s, &inp);
	address = inp.s_addr;
	inet_aton(netmask_s, &inp);
	netmask = inp.s_addr;

	broadcast = address | ~netmask;
	inp.s_addr = broadcast;

	return inet_ntoa(inp);
}

int save_network_info(const char *iface, networkInfo *network_info)
{
	int ret, temp_fd, updated = 0;
	FILE *fp, *temp_fp;
	char buffer[MAX_STR_LEN_S+1];
	char temp_buffer[MAX_STR_LEN_S+1];
	char p_buffer[MAX_STR_LEN_S*NETWORK_INFO_CHAR_MEMBERS+1];
	char template[] = "zero_tmp_XXXXXX";
	char copy_args[MAX_STR_LEN_S];
	cgiStr *temp_store;

	fp = fopen(INTERFACES_PATH, "r");
	if (fp == NULL) {
		return -1;
	}

	temp_store = cgi_str_new(NULL);

	if (network_info->use_dhcp) {
		snprintf(p_buffer, sizeof(p_buffer), "iface %s inet dhcp\n\n", iface);
	} else {
		if (!network_info->broadcast[0] && network_info->address[0] && network_info->netmask[0]) {
			strncpy(network_info->broadcast,
				get_broadcast_address(network_info->address, network_info->netmask),
				MAX_NET_INFO_TEXT_LEN);
		}
		snprintf(p_buffer, sizeof(p_buffer), "iface %s inet static\n", iface);
		snprintf(temp_buffer, sizeof(temp_buffer), "address %s\n", network_info->address);
		strcat(p_buffer, temp_buffer);
		snprintf(temp_buffer, sizeof(temp_buffer), "netmask %s\n", network_info->netmask);
		strcat(p_buffer, temp_buffer);
		snprintf(temp_buffer, sizeof(temp_buffer), "broadcast %s\n", network_info->broadcast);
		strcat(p_buffer, temp_buffer);
		if (network_info->gateway[0] == '\0') {
			snprintf(temp_buffer, sizeof(temp_buffer), "gateway 0.0.0.0\n");
		} else {
			snprintf(temp_buffer, sizeof(temp_buffer), "gateway %s\n\n", network_info->gateway);
		}
		strcat(p_buffer, temp_buffer);
	}

	while (fgets(buffer, sizeof(buffer), fp)) {

		if (strncmp(buffer, "iface", 5) == 0 && strstr(buffer, iface)) {

			cgi_str_add(&temp_store, p_buffer);

			while (fgets(buffer, sizeof(buffer), fp)) {
				if (buffer[0] == '\n' || strstr(buffer, iface)) {
					break;
				}
			}

			updated = 1;

		} else {

			cgi_str_add(&temp_store, buffer);

		}

	}

	fclose(fp);

	if (!updated) {
		snprintf(temp_buffer, sizeof(temp_buffer), "\nauto %s\n", iface);
		cgi_str_add(&temp_store, temp_buffer);
		cgi_str_add(&temp_store, p_buffer);
	}

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		cgi_str_free(temp_store);
		return -1;
	}

	ret = fputs(cgi_str_get(temp_store), temp_fp);
	if (ret < 0) {
		cgi_str_free(temp_store);
		return -1;
	}
	fclose(temp_fp);

	sprintf(copy_args, "sudo cp %s %s", template, INTERFACES_PATH);
	ret = system(copy_args);
	if (ret != 0) {
		cgi_str_free(temp_store);
		return -1;
	}

	unlink(template);
	cgi_str_free(temp_store);

	ret = save_first_nameserver(network_info->nameserver);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

int get_sys_load(float *avg1, float *avg2, float *avg3)
{
	FILE *fp = fopen(LOADAVG_PATH, "r");
	float tmp_arvg1, tmp_arvg2, tmp_arvg3;

	if (fp == NULL) {
		return -1;
	}

	if (fscanf(fp, "%f %f %f", &tmp_arvg1, &tmp_arvg2, &tmp_arvg3) != 3) {
		return -1;
	}

	if (avg1 != NULL) {
		*avg1 = tmp_arvg1;
	}
	if (avg2 != NULL) {
		*avg2 = tmp_arvg2;
	}
	if (avg3 != NULL) {
		*avg3 = tmp_arvg3;
	}

	fclose(fp);
	return 0;
}

int get_username(char *username, int len)
{
	char *ret;
	char *end;
	FILE *fp;

	fp = fopen(HTPASSWD_FILE_NAME, "r");
	if (fp == NULL) {
		username[0] = '\0';
		return 0;
	}

	ret = fgets(username, len, fp);
	if (ret) {
		end = strstr(username, ":");
		if (end) {
			end[0] = '\0';
		}
	}
	fclose(fp);

	return 0;
}

static int get_a230_reboot_time(void)
{
	FILE *in_fp;
	char buffer[MAX_STR_LEN_S];
	char *key, *value;
	int create_br = 0, stp_on = 0;
	char forward[MAX_STR_LEN_S];
	int reboot_time = STANDARD_REBOOT_TIME;

	in_fp = fopen(BRCONFIG_PATH, "r");
	if (in_fp == NULL) {
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), in_fp) != NULL) {

		if(buffer[0] == '#' || buffer[0] == ' '){
			continue;
		}

		value = strstr(buffer, "=");
		if (value == NULL) {
			continue;
		}
		buffer[strlen(buffer)-1] = '\0';
		*value++ = '\0';
		key = buffer;

		if (strcmp(key, "CREATE_BR0") == 0) {
			create_br = atoi(value);
		} else if (strcmp(key, "STP_ON") == 0) {
			stp_on = atoi(value);
		} else if (strcmp(key, "SETFD") == 0) {
			strncpy(forward, value, sizeof(forward));
		}
	}
	fclose(in_fp);

	if (create_br) {

		if (stp_on) {
			if (strcmp(forward, "DEFAULT") == 0) {
				reboot_time += 30;
			} else {
				reboot_time += atoi(forward)*2;
			}
		} else {
			reboot_time += 15;
		}

	}

	return reboot_time;
}

int required_reboot_secs(const char *product_name, int first_boot)
{
	int reboot_secs;

	if (strcmp(product_name, PRODUCT_NAME_A220) == 0) {
		reboot_secs = STANDARD_REBOOT_TIME;
	} else if (strcmp(product_name, PRODUCT_NAME_A230) == 0) {
		reboot_secs = get_a230_reboot_time();
	} else if (strcmp(product_name, PRODUCT_NAME_A240) == 0) {
		reboot_secs = STANDARD_REBOOT_TIME;
	} else if (strcmp(product_name, PRODUCT_NAME_A9) == 0) {
		reboot_secs = STANDARD_REBOOT_TIME;
	} else {
		syslog(LOG_ERR, "at-cgi: unrecognized product name encountered");
		reboot_secs = STANDARD_REBOOT_TIME;
	}

	if (first_boot) {
		reboot_secs += SSH_KEY_GEN_TIME;
	}

	return reboot_secs;
}

int is_hostname(char *hostname)
{
	if (!hostname) {
		return 0;
	}

	for (; *hostname; hostname++) {
		if (!isalnum(*hostname)) {
			if (*hostname != '-' && *hostname != '.') {
				return 0;
			}
		}
	}
	return 1;
}

static char *get_ifconfig_part(cgiStr **if_info, char *interf,  char *part_label)
{
	int ret, i;
	char *interf_start, *part_label_start, *interf_end;
	char *args[] = {IFCONFIG_PATH, "-a", NULL};

	ret = cgi_read_command(if_info, args[0], args);
	if (ret < 0) {
		return NULL;
	}

	interf_start = strstr(cgi_str_get(*if_info), interf);
	if (interf_start == NULL) {
		return NULL;
	}

	interf_end = strstr(interf_start, "\n\n");
	if (interf_end != NULL) {
		interf_end[0] = '\0';
	}
	part_label_start = strstr(interf_start, part_label);
	if (part_label_start == NULL) {
		return NULL;
	}
	i = strlen(part_label);
	while (part_label_start[i] != ' ' && (*if_info)->str[i] != '\n') {
		i++;
	}
	part_label_start[i] = '\0';

	return  part_label_start + strlen(part_label);
}

void print_current_effective_ip_address(char *current_iface)
{
	char *address;
	cgiStr *if_info;
	char iface_buf[MAX_STR_LEN_S];

	if_info = cgi_str_new(NULL);

	strncpy(iface_buf, current_iface, sizeof(iface_buf));

	address = get_ifconfig_part(&if_info, iface_buf, "inet addr:");
	if (!address) {
		cgi_str_reset(&if_info);
		strncat(iface_buf, ":0", sizeof(iface_buf));
		address = get_ifconfig_part(&if_info, iface_buf, "inet addr:");
		if (!address) {
			cgi_str_free(if_info);
			printf("unknown");
			return;
		}
	}

	printf("%s", address);

	cgi_str_free(if_info);
}

#define IP_INT(a,b,c,d)	(((a)<<24)|((b)<<16)|((c)<<8)|((d)<<0))
#define PRIVATE_BLOCK_24BIT_ADDR	IP_INT( 10,  0,  0,  0)
#define PRIVATE_BLOCK_24BIT_MASK	IP_INT(255,  0,  0,  0)
#define PRIVATE_BLOCK_20BIT_ADDR	IP_INT(172, 16,  0,  0)
#define PRIVATE_BLOCK_20BIT_MASK	IP_INT(255,240,  0,  0)
#define PRIVATE_BLOCK_16BIT_ADDR	IP_INT(192,168,  0,  0)
#define PRIVATE_BLOCK_16BIT_MASK	IP_INT(255,255,  0,  0)
#define IPV4LL_BLOCK_ADDR		IP_INT(169,254,  0,  0)
#define IPV4LL_BLOCK_MASK		IP_INT(255,255,  0,  0)

typedef struct {
	unsigned long addr;
	unsigned long mask;
} ipBlock;

int is_valid_private_ipaddr(char *addr_s, char *mask_s)
{
	struct in_addr inp;
	unsigned long addr, mask, host_addr;
	int i;

	static ipBlock blocks[] = {
		{PRIVATE_BLOCK_24BIT_ADDR,	PRIVATE_BLOCK_24BIT_MASK},
		{PRIVATE_BLOCK_20BIT_ADDR,	PRIVATE_BLOCK_20BIT_MASK},
		{PRIVATE_BLOCK_16BIT_ADDR,	PRIVATE_BLOCK_16BIT_MASK},
		{IPV4LL_BLOCK_ADDR,		IPV4LL_BLOCK_MASK},
	};

	inet_aton(addr_s, &inp);
	addr = htonl(inp.s_addr);
	inet_aton(mask_s, &inp);
	mask = htonl(inp.s_addr);

	for (i = 0; i < ARRAY_SIZE(blocks); i++) {
		if ((addr & blocks[i].mask) == blocks[i].addr) {
			if (mask >= blocks[i].mask) {
				host_addr = addr & ~mask;
				if (host_addr != 0 && host_addr != ~mask)
					return 1;
			}
		}
	}

	return 0;
}

int is_same_net_segment(char *address_1_s, char *address_2_s, char *netmask_s)
{
	struct in_addr inp;
	unsigned long address_1;
	unsigned long address_2;
	unsigned long netmask;

	inet_aton(address_1_s, &inp);
	address_1 = inp.s_addr;
	inet_aton(address_2_s, &inp);
	address_2 = inp.s_addr;
	inet_aton(netmask_s, &inp);
	netmask = inp.s_addr;

	if ((address_1 & netmask) == (address_2 & netmask)) {
		return 1;
	}

	return 0;
}

#define IFACE_POLL_WAIT_USEC		(1000*500)
#define IFACE_POLL_MAX_WAIT_USEC	(1000*1000*3)
static void wait_for_iface_down(char *iface)
{
	int ret, wait = 0;
	cgiStr *ifconfig_output;
	char *ifconfig_args[] = {SUDO_PATH, IFCONFIG_PATH,  NULL};
	char iface_buf[64];

	snprintf(iface_buf, sizeof(iface_buf), "%s ", iface);

	ifconfig_output = cgi_str_new(NULL);

	while (wait < IFACE_POLL_MAX_WAIT_USEC) {

		ret = cgi_read_command(&ifconfig_output, ifconfig_args[0], ifconfig_args);
		if (ret < 0) {
			cgi_str_free(ifconfig_output);
			syslog(LOG_ERR, "at-cgi: failed to read ifconfig output");
			return;
		}

		if (!strstr(ifconfig_output->str, iface_buf)) {
			cgi_str_free(ifconfig_output);
			return;
		}

		cgi_str_reset(&ifconfig_output);

		usleep(IFACE_POLL_WAIT_USEC);
		wait += IFACE_POLL_WAIT_USEC;

	}

	cgi_str_free(ifconfig_output);
	syslog(LOG_ERR, "at-cgi: iface not downed");
}

void networking_down(char *iface, int wait_for_down)
{
	char *if_down[] 		= {SUDO_PATH, IFDOWN_PATH, "", NULL};
	char *avahi_stop[] 		= {SUDO_PATH, AVAHI_PATH, "-k", NULL};
	char *led_ctrl_red_on[] = {SUDO_PATH, LED_CTRL_PATH, "--red=on", NULL};
#ifdef PACKETSCAN
	char *stop_snort[] 		= {SUDO_PATH, SNORT_INIT_PATH, "stop", iface, NULL};

	cgi_exec(stop_snort[0], stop_snort);
#endif

	if_down[2] = iface;

	if (strcmp(config_get_product_name(), PRODUCT_NAME_A9))
		cgi_exec(led_ctrl_red_on[0], led_ctrl_red_on);

	cgi_exec(avahi_stop[0], avahi_stop);

	cgi_exec(if_down[0], if_down);

	if (wait_for_down == WAIT_FOR_IFACE_DOWN) {
		wait_for_iface_down(iface);
	}
}

#ifdef BRIDGE

static void row_to_str_list(cgiStrList **list, char *row)
{
	char *val_start;
	char *val_end;

	val_start = strchr(row, '\'') + 1;
	if ((val_end = strrchr(val_start, '\''))) {
		val_end[0] = '\0';
	}

	while (val_start && val_start[0] != '\'') {

		if ((val_end = strchr(val_start, ' '))) {
			while (*val_end == ' ') {
				*val_end++ = '\0';
			}
		}

		cgi_str_list_add(list, val_start);

		val_start = val_end;

	}
}

bridgeConfig *bridge_config_load(void)
{
	FILE *in_fp;
	bridgeConfig *bridge_config;
	char buffer[BRIDGE_CONFIG_ENTRY_LEN_MAX];
	char *key, *value;

	in_fp = fopen(BRCONFIG_PATH, "r");
	if (in_fp == NULL) {
		return NULL;
	}

	bridge_config = cgi_calloc(sizeof(bridgeConfig), 1);

	while (fgets(buffer, sizeof(buffer), in_fp) != NULL) {

		if(buffer[0] == '#' || buffer[0] == ' '){
			continue;
		}

		value = strstr(buffer, "=");
		if (value == NULL) {
			continue;
		}
		buffer[strlen(buffer)-1] = '\0';
		*value++ = '\0';
		key = buffer;

		if (strcmp(key, "CREATE_BRIDGE") == 0) {
			bridge_config->create_br = atoi(value);
		} else if (strcmp(key, "BRIDGE_NAME") == 0) {
			cgi_strncpy(bridge_config->bridge_name, value, sizeof(bridge_config->bridge_name));
		} else if (strcmp(key, "BRIDGED_IFACES") == 0) {
			row_to_str_list(&bridge_config->bridged_ifaces, value);
		} else if (strcmp(key, "IFACE_MATCHES") == 0) {
			row_to_str_list(&bridge_config->iface_matches, value);
		} else if (strcmp(key, "SETAGEING") == 0) {
			cgi_strncpy(bridge_config->ageing, value, sizeof(bridge_config->ageing));
		} else if (strcmp(key, "STP_ON") == 0) {
			bridge_config->stp_on = atoi(value);
		} else if (strcmp(key, "SETBRIDGEPRIO") == 0) {
			cgi_strncpy(bridge_config->br_prio, value, sizeof(bridge_config->br_prio));
		} else if (strcmp(key, "SETFD") == 0) {
			cgi_strncpy(bridge_config->forward, value, sizeof(bridge_config->forward));
		} else if (strcmp(key, "SETGCINT") == 0) {
			cgi_strncpy(bridge_config->gc_int, value, sizeof(bridge_config->gc_int));
		} else if (strcmp(key, "SETHELLO") == 0) {
			cgi_strncpy(bridge_config->hello, value, sizeof(bridge_config->hello));
		} else if (strcmp(key, "SETMAXAGE") == 0) {
			cgi_strncpy(bridge_config->max_age, value, sizeof(bridge_config->max_age));
		} else if (strncmp(key, "SETPATHCOST_", 12) == 0) {
			cgi_str_pair_list_add(&bridge_config->port_costs, key+12, value);
		} else if (strncmp(key, "SETPATHPRIO_", 12) == 0) {
			cgi_str_pair_list_add(&bridge_config->port_prios, key+12, value);
		}

	}

	fclose(in_fp);
	return bridge_config;
}

void bridge_config_free(bridgeConfig *bridge_config)
{
		cgi_str_list_free(bridge_config->bridged_ifaces);
		cgi_str_list_free(bridge_config->iface_matches);
		cgi_str_pair_list_free(bridge_config->port_costs);
		cgi_str_pair_list_free(bridge_config->port_prios);
		free(bridge_config);
}

#endif

void networking_up(char *iface)
{
	char *ip_up[] 				= {SUDO_PATH, IFUP_PATH, "", NULL};
	char *avahi_start[] 		= {SUDO_PATH, AVAHI_PATH, "-D", NULL};
	char *led_ctrl_red_off[]	= {SUDO_PATH, LED_CTRL_PATH, "--red=off", NULL};
#ifdef PACKETSCAN
	char *start_snort[] 		= {SUDO_PATH, SNORT_INIT_PATH, "start", iface, NULL};
#endif
#ifdef BRIDGE
	char *ipconfig_up[] 		= {SUDO_PATH, IFCONFIG_PATH, "br0", "0.0.0.0", NULL};
	bridgeConfig *bridge_config;
	int up_wait;
	
	bridge_config = bridge_config_load();
	if (!bridge_config) {
		syslog(LOG_ERR, "at-cgi: failed to load bridge info");
	} else {
		if (bridge_config->stp_on) {
			up_wait = atoi(bridge_config->forward) * 2;
		} else {
			up_wait = 15;
		}
		bridge_config_free(bridge_config);
		cgi_exec(ipconfig_up[0], ipconfig_up);
		sleep(up_wait);
	}
#endif

	ip_up[2] = iface;

	cgi_exec(ip_up[0], ip_up);

	cgi_exec(avahi_start[0], avahi_start);

	if (strcmp(config_get_product_name(), PRODUCT_NAME_A9))
		cgi_exec(led_ctrl_red_off[0], led_ctrl_red_off);

#ifdef PACKETSCAN
	cgi_exec(start_snort[0], start_snort);
#endif
}

cgiStrList *interface_list_get(cgiStrList *poss_iface_matches)
{
	int ret;
	cgiStrList *found_ifaces = NULL, *poss_iface_next;
	cgiStr *ifconfig_output;
	char *line_start, *line_end, *temp;
	char *ifconfig_args[] = {SUDO_PATH, IFCONFIG_PATH, "-a", NULL};

	if (!poss_iface_matches) {
		return NULL;
	}

	ifconfig_output = cgi_str_new(NULL);
	ret = cgi_read_command(&ifconfig_output, ifconfig_args[0], ifconfig_args);
	if (ret < 0) {
		cgi_str_free(ifconfig_output);
		syslog(LOG_ERR, "at-cgi: failed to read ifconfig output");
		return NULL;
	}

	line_start = cgi_str_get(ifconfig_output);

	while ((line_end = strchr(line_start, '\n')) != NULL) {

		line_end[0] = '\0';

		poss_iface_next = poss_iface_matches;
		while (poss_iface_next) {

			if (strstr(line_start, poss_iface_next->str)) {

				temp = strchr(line_start, ' ');
				temp[0] = '\0';
				cgi_str_list_add(&found_ifaces, line_start);
				break;

			}

			poss_iface_next = cgi_str_list_next(poss_iface_next);

		}

		line_start = ++line_end;
		if (!line_start) {
			break;
		}

	}

	cgi_str_free(ifconfig_output);

	return found_ifaces;
}

#ifdef DEBUG
void serial_debug(const char *message)
{
	FILE *file_out = fopen(SERIAL_PORT, "w");
	if (file_out) {
		fprintf(file_out, "%s (debug) : %s\n", SOFTWARE_NAME, message);
		fclose(file_out);
	}
}
#endif
