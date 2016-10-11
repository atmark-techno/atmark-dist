/*
 * modules/system_main.c - system module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>

#include <simple-cgi.h>

#include <simple-cgi-app-str.h>
#include <simple-cgi-app-misc.h>
#include <simple-cgi-app-io.h>

#include <frame-html.h>
#include <core.h>
#include <passwd.h>
#include <common.h>
#include <misc-utils.h>
#include <system-config.h>
#include <menu.h>

#include "system_display.h"

#define MIN_PASSWD_LEN			(5)
#define MIN_USERNAME_LEN		(3)

#define FIRMWARE_SUFFIX_MATCH		".gz"

#define NETFLASH_WRONG_PRODUCT		(10)

static void handle_update_in_progress(void)
{
	int ret;
	int netflash_state;
	char user_error_msg[] = "An error occured when updating firmware";
	char user_error_msg2[] = "The firmware image is different from the current type";
	char *del_args[] = {RM_PATH, NETFLASH_RESULT_PATH, NULL};

	netflash_state = is_process_alive("netflash");

	if (netflash_state < 0) {

		report_error(user_error_msg, "Failed to get netflash process state");
		return;

	} else if (netflash_state == PROCESS_STATE_ACTIVE) {

		display_update_in_progress();

	} else {

		if (cgi_file_exists(NETFLASH_RESULT_PATH)) {
			if (flag_exists(NETFLASH_RESULT_PATH, FLAG_WRONG_PRODUCT)) 
				report_error(user_error_msg2, "netflash failed");
			else
				report_error(user_error_msg, "netflash failed");

			cgi_exec(del_args[0], del_args);

			return;
		}

		ret = set_flag(UPDATED_FW_FLAG_PATH);
		if (ret < 0) {
			syslog(LOG_ERR, "at-cgi: failed to set fw update flag");
		}

		display_software_update_msg(SU_MSG_UPDATE_INFO, "Firmware updated");

	}
}

static void process_password_change(char *username, char *password)
{
	pid_t pid;
	int pipe_fds[2], status, ret;
	char user_error_msg[] = {"An error occured while updating password\n"};
	char *passwd_args[] = {SUDO_PATH, "htpasswd",
		"-c", HTPASSWD_FILE_NAME, "", NULL};

	passwd_args[4] = username;

	if (pipe(pipe_fds) != 0) {
		report_error(user_error_msg, "Failed to create pipe");
		return;
	}

	pid = fork();

	if (pid == 0) {

		close(0);
		close(1);
		dup2(pipe_fds[0], 0);
		close(pipe_fds[0]);
		close(pipe_fds[1]);
		ret = execvp(passwd_args[0], passwd_args);

		if (ret < 0) {
			exit(-1);
		}

	} else if (pid < 0) {

		report_error(user_error_msg, "Failed to fork");
		return;

	}

	close(pipe_fds[0]);
	ret = write(pipe_fds[1], password, strlen(password));
	if (ret < 0) {
		report_error(user_error_msg, "Failed to write to child");
		return;
	}
	close(pipe_fds[1]);

	waitpid(pid, &status, 0);
	if (!WIFEXITED(status) || WEXITSTATUS(status)) {
		report_error(user_error_msg, "Child exited abnormally");
		return;
	}

	ret = set_flag(UNSAVED_SETTINGS_FLAG_PATH);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to set unsaved settings flag");
	}

	display_password_settings("", "", "", "", "", "Password changed");
}

static void change_password(void)
{
	int ret, i;
	char *pass1, *pass2, *curr_pass, *username;
	char user_error_msg[] = {"An error occured while updating password\n"};

	username 	= cgi_query_get_val(ELE_ID_USERNAME);
	curr_pass 	= cgi_query_get_val(ELE_ID_CURR_PASSWORD);
	pass1 		= cgi_query_get_val(ELE_ID_PASSWORD1);
	pass2 		= cgi_query_get_val(ELE_ID_PASSWORD2);

	if (cgi_is_null_or_blank(username)) {
		display_password_settings("", curr_pass, pass1, pass2,
			"Please enter a username", NULL);
		return;
	} else if (cgi_is_null_or_blank(curr_pass)) {
		display_password_settings(username, "", pass1, pass2,
			"Please enter the current password", NULL);
		return;
	} else if (cgi_is_null_or_blank(pass1)) {
		display_password_settings(username, curr_pass, "", pass2,
			"Please enter the new password", NULL);
		return;
	} else if (cgi_is_null_or_blank(pass2)) {
		display_password_settings(username, curr_pass, pass1, "",
			"Please enter the password twice", NULL);
		return;
	}

	cgi_chop_line_end(username);
	cgi_chop_line_end(curr_pass);
	cgi_chop_line_end(pass1);
	cgi_chop_line_end(pass2);

	if (cgi_file_exists(HTPASSWD_FILE_NAME)) {
		ret = check_current_password(curr_pass);
		if (ret < 0) {
			report_error(user_error_msg,
				"Failed to check current password");
			return;
		} else if (ret == 0) {
			display_password_settings(username, curr_pass, pass1, pass2,
				"Current password is incorrect", NULL);
			return;
		}
	}

	if (strcmp(pass1, pass2) != 0) {
		display_password_settings(username, curr_pass, pass1,
			pass2, "The new passwords are not the same", NULL);
		return;
	}

	if (strlen(username) < MIN_USERNAME_LEN) {
		display_password_settings(username, curr_pass,
			pass1, pass2, "The username is too short", NULL);
		return;
	}

	for (i = 0; i < strlen(username); i++) {
		if (!isalnum(username[i]) && username[i] != ' ') {
			display_password_settings(username, curr_pass, pass1,
				pass2, "Invalid character in username", NULL);
			return;
		}
	}

	if (strlen(pass1) < MIN_PASSWD_LEN) {
		display_password_settings(username, curr_pass,
			pass1, pass2, "The password is too short", NULL);
		return;
	}

	for (i = 0; i < strlen(pass1); i++) {
		if (!isalnum(pass1[i]) && pass1[i] != ' ') {
			display_password_settings(username, curr_pass, pass1,
				pass2, "Invalid character in password", NULL);
			return;
		}
	}

	process_password_change(username, pass1);
}

static void reboot_system(char *current_iface)
{
	int ret, reboot_secs, settings_inited;
	char *reboot_args[] = {SUDO_PATH, REBOOT_PATH, NULL};
	char *hostname_args[] = {HOSTNAME_PATH,  NULL};
	networkInfo network_info;
	char user_error_msg[] = {"An error occured while attempting to reboot"};

	ret = get_current_network_info(config_get_primary_if(), &network_info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get current network info");
		return;
	}

	settings_inited = flag_exists(UPDATED_SETTINGS_INIT_FLAG_PATH, FLAG_NOTCHECKED);

	reboot_secs = required_reboot_secs(config_get_product_name(), settings_inited);
	if (reboot_secs < 0) {
		report_error(user_error_msg, "Failed to get reboot time");
		return;
	}

	ret = cgi_exec_no_wait(reboot_args[0], reboot_args);
	if (ret < 0) {
		report_error(user_error_msg, "Execution of reboot command failed");
		return;
	}

	printf("<div class=\"h_gap_big\"></div>");

	printf("<div align=\"center\" width=\"80%%\" style=\"padding: 0 30px 0 30px;\">");
	printf("<img src=\"%s\" alt=\"Admin\" class=\"title_img\"/>",
		TITLE_IMG);

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p class=\"update_title\">%sを再起動しています</p>\n",
		config_get_product_name());

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p align=\"center\">");

	printf("WEBブラウザ画面を閉じ、");

	if (strcmp(config_get_product_name(), PRODUCT_NAME_A9) == 0)
		printf("しばらく待ってから");
	else
		printf("%sの赤LEDが消灯するまで待ってから", config_get_product_name());

	printf("<br />");

	printf("再度Bonjourなどを利用してトップページにアクセスし直してください。");

	printf("<div class=\"h_gap_med\"></div>");

	printf("<p align=\"center\">Hostname: ");
	cgi_print_command(hostname_args[0], hostname_args);
	printf("</p>");

	printf("<p align=\"center\">IP Address: ");
	if (network_info.use_dhcp) {
		printf("auto");
	} else {
		printf("static (%s)", network_info.address);
	}
	printf("</p>");

	printf("<p align=\"center\">MAC address: ");
	if (!current_iface) {
		print_ifconfig_part(config_get_primary_if(), "HWaddr ");
	} else {
		print_ifconfig_part(current_iface, "HWaddr ");
	}
	printf("</p>");

	printf("</div>");
	printf("<div class=\"h_gap_big\"></div>");
}

static void commit_settings(void)
{
	int ret;
	char *flatfsd_args[] = {SUDO_PATH, FLATFSD_PATH, "-s", NULL};
	char *rm_args[] = {RM_PATH, UNSAVED_SETTINGS_FLAG_PATH, NULL};
	char *rm_args2[] = {RM_PATH, UPDATED_SETTINGS_INIT_FLAG_PATH, NULL};
	char user_error_msg[] = {"An error occured while saving settings"};

	ret = cgi_exec(flatfsd_args[0], flatfsd_args);
	if (ret < 0) {
		report_error(user_error_msg, "flatfsd -s failed");
		return;
	}

	ret = cgi_exec(rm_args[0], rm_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to remove unsaved settings flag");
	}
	if (flag_exists(UPDATED_SETTINGS_INIT_FLAG_PATH, FLAG_NOTCHECKED)) {
		ret = cgi_exec(rm_args2[0], rm_args2);
		if (ret < 0) {
			syslog(LOG_ERR, "at-cgi: failed to remove inited settings flag");
		}
	}

	display_system_state_settings("Settings Saved");
}

static void process_software_update(char *flash_region, char *image_url)
{
	int ret;
	pid_t pid;
	char user_error_msg[] = {"An error occured while updating software"};
	char *netflash_args[] = {SUDO_PATH, NETFLASH_PATH,
		"-k", "-b", "-r", "", "", NULL};
	char *netflash2_args[] = {SUDO_PATH, NETFLASH_PATH,
		"-k", "-b", "-n", "-r", "", "", NULL};
	char *touch_args[] = {SUDO_PATH, TOUCH_PATH,
		NETFLASH_RESULT_PATH, NULL};
	char *query_val;

	netflash_args[5] = netflash2_args[6] = flash_region;
	netflash_args[6] = netflash2_args[7] = image_url;

	at_cgi_debug("updating software: parent: start");

	pid = fork();

	if (pid == 0) {

		at_cgi_debug("updating software: child: start");

		/* close stdin/out to allow http server to return a response */
		close(1);
		close(2);

		query_val = cgi_query_get_val(ELE_ID_ALLOW_OTHER_PRODUCTS);
		if (strcmp(flash_region, USERLAND_FLASH_PATH) == 0  &&
		    cgi_is_null_or_blank(query_val))
			ret = cgi_exec_return_exit_status(netflash_args[0], netflash_args);
		else
			ret = cgi_exec_return_exit_status(netflash2_args[0], netflash2_args);

		if (ret != 0) {
			at_cgi_debug("updating software: child: netflash failed");

			if (ret == NETFLASH_WRONG_PRODUCT) {
				ret = cgi_dump_to_file(NETFLASH_RESULT_PATH, FLAG_WRONG_PRODUCT);
				if (ret < 0)
					exit(-1);
			} else
				cgi_exec(touch_args[0], touch_args);
		}

		at_cgi_debug("updating software: child: finished");

		exit(0);

	} else if (pid < 0) {

		report_error(user_error_msg, "Fork failed");
		return;

	}

	usleep(500000);

	at_cgi_debug("updating software: parent: finished");

	display_update_in_progress();
}

static void update_software(void)
{
	char *req_area, *path;

	req_area = cgi_query_get_val("req_area");

	if (strcmp(req_area, ACTION_UPDATES_USERLAND) == 0) {

		path = cgi_query_get_val(ELE_ID_USERLAND_PATH_H);
		cgi_chop_line_end(path);
		if (cgi_is_null_or_blank(path)) {
			display_software_update_msg(SU_MSG_USERLAND_PATH,
				"Please select an image file");
			return;
		}
		if (!cgi_is_valid_url(path)) {
			display_software_update_msg(SU_MSG_USERLAND_PATH,
				"The entered URL was not valid");
			return;
		}
		process_software_update(USERLAND_FLASH_PATH, path);

	} else {

		path = cgi_query_get_val(ELE_ID_KERNEL_PATH_H);
		cgi_chop_line_end(path);
		if (cgi_is_null_or_blank(path)) {
			display_software_update_msg(SU_MSG_KERNEL_PATH,
				"Please select an image file");
			return;
		}
		if (!cgi_is_valid_url(path)) {
			display_software_update_msg(SU_MSG_KERNEL_PATH,
				"The entered URL was not valid");
			return;
		}
		process_software_update(KERNEL_FLASH_PATH, path);

	}
}

static int update_hostname_config(int *hname_change)
{
	int ret;
	FILE *config_hosts_fp;
	char buffer[1024];
	char current_hostname[HOST_NAME_MAX], *hostname;
	char *hostname_args[] = {SUDO_PATH, HOSTNAME_PATH, "-F", HOSTNAME_FILE_PATH, NULL};
	char user_error_msg[] = "An error occured while updating configuration";
	cgiStr *hosts_entry;

	hostname = cgi_query_get_val(ELE_ID_HOSTNAME);

	ret = gethostname(current_hostname, sizeof(current_hostname));
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get hostname");
		return -1;
	}

	if (strcmp(hostname, current_hostname) == 0) {
		*hname_change = 0;
		return 0;
	}
	*hname_change = 1;

	ret = sudo_string_to_file(HOSTNAME_FILE_PATH, hostname);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to write to hostname file");
		return -1;
	}

    ret = cgi_exec(hostname_args[0], hostname_args);
	if (ret != 0) {
		return -1;
	}

	if (!cgi_file_exists(HOSTS_CONF_PATH)) {
		return 0;
	}

	hosts_entry = cgi_str_new(NULL);

	config_hosts_fp = fopen(HOSTS_CONF_PATH, "r");
	if (config_hosts_fp == NULL) {
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), config_hosts_fp)) {

		if (strncmp(buffer, "127.0.0.1", 9) == 0) {
			cgi_str_add(&hosts_entry, "127.0.0.1 localhost ");
			cgi_str_add(&hosts_entry, hostname);
			cgi_str_add(&hosts_entry, "\n");
		} else {
			cgi_str_add(&hosts_entry, buffer);
		}

	}

	fclose(config_hosts_fp);

	ret = sudo_string_to_file(HOSTS_CONF_PATH, cgi_str_get(hosts_entry));
	if (ret < 0) {
		report_error(user_error_msg, "Failed to write to hosts file");
		return -1;
	}

	cgi_str_free(hosts_entry);

	return 0;
}

static int compare_if_config(networkInfo *new_network_info, int *if_change)
{
	int ret;
	char user_error_msg[] = "An error occured while updating configuration";
	networkInfo network_info;

	ret = get_current_network_info(config_get_primary_if(), &network_info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get network info");
		return -1;
	}

	if (new_network_info->use_dhcp && network_info.use_dhcp) {
		*if_change = 0;
		return 0;
	} else if ((strcmp(network_info.address, new_network_info->address) == 0) &&
		(strcmp(network_info.netmask, new_network_info->netmask) == 0) &&
		(strcmp(network_info.gateway, new_network_info->gateway) == 0) &&
		(strcmp(network_info.nameserver, new_network_info->nameserver) == 0)) {
		*if_change = 0;
		return 0;
	}

	*if_change = 1;

	return 0;
}

static int network_restart(networkInfo *new_network_info)
{
	int ret;
	pid_t pid;
#ifdef FIREWALL
	char *firewall_args[] = {SUDO_PATH, FIREWALL_SCRIPT_PATH, NULL};
#endif

	pid = fork();
	if (pid == 0) {

		/* close stdin/out to allow http server to return a response */
		close(1);
		close(2);

		networking_down(config_get_primary_if(), WAIT_FOR_IFACE_DOWN);

		if (new_network_info) {
			ret = save_network_info(config_get_primary_if(), new_network_info);
			if (ret < 0) {
				syslog(LOG_ERR, "at-cgi: counldn't save network info");
			}
		}

#ifdef FIREWALL
		ret = cgi_exec(firewall_args[0], firewall_args);
		if (ret < 0) {
			syslog(LOG_ERR, "failed to run firewall script");
		}
#endif

		networking_up(config_get_primary_if());

		exit(0);

	} else if (pid < 0) {

		return -1;

	}

	return 0;
}

static int avahi_restart(void)
{
	int ret;
	char *avahi_start[] = {"/usr/bin/sudo", "/sbin/avahi-daemon", "-D", NULL};
	char *avahi_stop[] = {"/usr/bin/sudo", "/sbin/avahi-daemon", "-k", NULL};

	ret = cgi_exec(avahi_stop[0], avahi_stop);
	if (ret < 0) {
		return -1;
	}
	ret = cgi_exec(avahi_start[0], avahi_start);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

static int process_network_settings_args(networkInfo *network_info)
{
	char *ip_type, *address, *netmask, *gateway, *hostname, *nameserver;
	struct in_addr inp;

	hostname = cgi_query_get_val(ELE_ID_HOSTNAME);

	ip_type = cgi_query_get_val(ELE_ID_IP_ASSIGN_TYPE);
	if (cgi_is_null_or_blank(ip_type)) {
		report_error("Invalid request", "Failed to get ip type arguments");
		return -1;
	}

	if (strcmp(ip_type, ELE_ID_IP_DHCP) == 0) {

		network_info->use_dhcp 	= 1;
		network_info->address[0] 	= '\0';
		network_info->netmask[0] 	= '\0';
		network_info->gateway[0] 	= '\0';
		network_info->nameserver[0]	= '\0';

	} else {

		network_info->use_dhcp = 0;

		address 	= cgi_query_get_val(ELE_ID_IP_ADDRESS);
		netmask 	= cgi_query_get_val(ELE_ID_IP_NETMASK);
		gateway 	= cgi_query_get_val(ELE_ID_IP_GATEWAY);
		nameserver 	= cgi_query_get_val(ELE_ID_IP_DNSSERVER);

		strncpy(network_info->address, address, sizeof(network_info->address));
		strncpy(network_info->netmask, netmask, sizeof(network_info->netmask));
		strncpy(network_info->gateway, gateway, sizeof(network_info->gateway));
		strncpy(network_info->nameserver, nameserver, sizeof(network_info->nameserver));

		if (cgi_is_null_or_blank(network_info->address)) {
			display_network_settings(network_info, hostname,
				"Please enter an IP address", NULL);
			return -1;
		} else if (cgi_is_null_or_blank(network_info->netmask)) {
			display_network_settings(network_info, hostname,
				"Please enter a netmask", NULL);
			return -1;
		}

		cgi_chop_line_end(network_info->address);
		cgi_chop_line_end(network_info->netmask);
		cgi_chop_line_end(network_info->gateway);
		cgi_chop_line_end(network_info->nameserver);

		if (inet_aton(network_info->address, &inp) == 0) {
			display_network_settings(network_info, hostname,
				"The entered IP address is not valid", NULL);
			return -1;
		}
		if (inet_aton(network_info->netmask, &inp) == 0) {
			display_network_settings(network_info, hostname,
				"The entered netmask is not valid", NULL);
			return -1;
		}

		if (!is_valid_private_ipaddr(network_info->address, network_info->netmask)) {
			display_network_settings(network_info, hostname,
				"Please enter a valid private network address and \
				netmask combination", NULL);
			return -1;
		}

		if (!cgi_is_null_or_blank(network_info->gateway)) {

			if (inet_aton(network_info->gateway, &inp) == 0) {
				display_network_settings(network_info, hostname,
					"The entered gateway is not valid", NULL);
				return -1;
			}

			if (is_same_net_segment(network_info->gateway, network_info->address, network_info->netmask) == 0) {
				display_network_settings(network_info, hostname,
					"IP address and gateway must be on the same \
					network segment", NULL);
				return -1;
			}

		}

		if (!cgi_is_null_or_blank(network_info->nameserver)) {
			if (inet_aton(network_info->nameserver, &inp) == 0) {
				display_network_settings(network_info, hostname,
					"The entered DNS server address is not valid", NULL);
				return -1;
			}
		}

		strncpy(network_info->broadcast,
			get_broadcast_address(network_info->address, network_info->netmask),
			MAX_NET_INFO_TEXT_LEN);

	}

	if (cgi_is_null_or_blank(hostname)) {
		display_network_settings(network_info, hostname,
				"Please enter hostname", NULL);
		return -1;
	}
	cgi_chop_line_end(hostname);
	if (!is_hostname(hostname)) {
		display_network_settings(network_info, hostname,
			"Hostname is not valid", NULL);
		return -1;
	}

	return 0;
}

static void save_network_settings(void)
{
	networkInfo network_info;
	int ret, if_change = 0, hname_change = 0;
	char user_error_msg[] = "An error occured when updating network settings";

	ret = process_network_settings_args(&network_info);
	if (ret < 0) {
		return;
	}

	ret = compare_if_config(&network_info, &if_change);
	if (ret < 0) {
		return;
	}

	ret = update_hostname_config(&hname_change);
	if (ret < 0) {
		return;
	}

	if (!if_change && !hname_change) {

		display_network_settings(NULL, NULL, "", "No changes made");
		return;

	}

	if (if_change) {
		ret = network_restart(&network_info);
		if (ret < 0) {
			report_error(user_error_msg,
				"Failed to update network interfaces");
			return;
		}
	} else {
		ret = avahi_restart();
		if (ret < 0) {
			report_error(user_error_msg,
				"Failed when restarting avahi");
			return;
		}
	}

	ret = set_flag(UNSAVED_SETTINGS_FLAG_PATH);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to set unsaved settings flag");
	}

	display_network_change(&network_info);
}

static void load_settings(void)
{
	int ret;
	char *flatfsd_args[] = {SUDO_PATH, FLATFSD_PATH, "-r", NULL};
	char *hostname_args[] = {SUDO_PATH, HOSTNAME_PATH, "-F", HOSTNAME_FILE_PATH, NULL};
	char *rm_args[] = {RM_PATH, UNSAVED_SETTINGS_FLAG_PATH, NULL};
	char *rm2_args[] = {RM_PATH, UPDATED_SETTINGS_INIT_FLAG_PATH, NULL};
	char user_error_msg[] = {"An error occured while undoing settings"};
#ifdef PACKETSCAN
	char *crontab_args[] = {SUDO_PATH, CRONTAB_PATH, CRONTAB_CONFIG_PATH, NULL};
#endif
	ret = cgi_exec(flatfsd_args[0], flatfsd_args);
	if (ret < 0) {
		report_error(user_error_msg, "flatfsd -r failed");
		return;
	}

	ret = cgi_exec(hostname_args[0], hostname_args);
	if (ret < 0) {
		return;
	}

	ret = network_restart(NULL);
	if (ret < 0) {
		report_error(user_error_msg,
			"Failed to update network interfaces");
		return;
	}

	ret = cgi_exec(rm_args[0], rm_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to remove unsaved settings flag");
	}
	ret = cgi_exec(rm2_args[0], rm2_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to remove init settings flag");
	}

#ifdef PACKETSCAN
		ret = cgi_exec(crontab_args[0], crontab_args);
		if (ret < 0) {
			syslog(LOG_ERR, "failed to update crontab");
		}
#endif

	display_system_state_settings_change();
}

static void init_settings(void)
{
	int ret;
	char *flatfsd_args[] = {SUDO_PATH, FLATFSD_PATH, "-w", NULL};
	char *rm_args[] = {SUDO_PATH, RM_PATH, UNSAVED_SETTINGS_FLAG_PATH, NULL};
	char *cp_args[] = {SUDO_PATH, CP_PATH, "-a", "/etc/config", "/tmp/at-admin/config", NULL};
	char *rm2_args[] = {SUDO_PATH, RM_PATH, "-rf", "/etc/config", NULL};
	char *cp2_args[] = {SUDO_PATH, CP_PATH, "-a", "/tmp/at-admin/config", "/etc/config", NULL};
	char *rm3_args[] = {SUDO_PATH, RM_PATH, "-rf", "/tmp/at-admin/config", NULL};
	char user_error_msg[] = {"An error occured while undoing settings"};

	ret = cgi_exec(cp_args[0], cp_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to cp");
	}

	ret = cgi_exec(flatfsd_args[0], flatfsd_args);
	if (ret < 0) {
		report_error(user_error_msg, "flatfsd -w failed");
		return;
	}

	ret = cgi_exec(rm2_args[0], rm2_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to rm");
	}
	ret = cgi_exec(cp2_args[0], cp2_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to cp");
	}
	ret = cgi_exec(rm3_args[0], rm3_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to rm");
	}
	ret = cgi_exec(rm_args[0], rm_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to remove unsaved settings flag");
	}

	ret = set_flag(UPDATED_SETTINGS_INIT_FLAG_PATH);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to set settings init flag");
	}

	display_system_state_settings("Settings restored");
}

static void get_update_options(void)
{
	int ret, i, userland_no = 0, kernel_no = 0;
	cgiStr *wget_output;
	char *wget_args[] = {WGET_PATH, "-O", "-", "", NULL};
	char *href_start, *href_end;
	cgiStrPairList *userland_options  = NULL;
	cgiStrPairList *kernel_options = NULL;
	configValues *kernel_matches, *userland_matches;
	char buf[512];

	kernel_matches = config_get_firmware_kernel_matches();
	userland_matches = config_get_firmware_userland_matches();

	wget_args[3] = config_get_firmware_url();

	wget_output = cgi_str_new(NULL);

	ret = cgi_read_command(&wget_output, wget_args[0], wget_args);
	if (ret < 0) {
		cgi_str_free(wget_output);
		display_software_update_msg(SU_MSG_GET_OPTIONS,
			"Failed to get update options");
		return;
	}

	href_end = cgi_str_get(wget_output);
	while ((href_start = strcasestr(href_end, "href=\"")) != NULL) {

		href_end = strstr(href_start+6, "\"");
		*href_end++ = '\0';

		/* check suffix matches */
		if (strcmp(href_end - strlen(FIRMWARE_SUFFIX_MATCH) - 1,
				FIRMWARE_SUFFIX_MATCH) != 0)
			continue;

		for (i = 0; userland_matches->values[i]; i++) {
			if (strstr(href_start, userland_matches->values[i]) != NULL) {
				snprintf(buf, sizeof(buf), "%s%s", config_get_firmware_url(), href_start+6);
				cgi_str_pair_list_add(&userland_options, href_start+6, buf);
				userland_no++;

				break;
			}
		}

		for (i = 0; kernel_matches->values[i]; i++) {
			if (strstr(href_start, kernel_matches->values[i]) != NULL) {
				snprintf(buf, sizeof(buf), "%s%s", config_get_firmware_url(), href_start+6);
				cgi_str_pair_list_add(&kernel_options, href_start+6, buf);
				kernel_no++;
				break;
			}
		}

	}

	if (userland_no == 0 && kernel_no == 0) {
		display_software_update_msg(SU_MSG_GET_OPTIONS,
			"No update options could be found");
	} else {
		display_software_update_options(userland_options, kernel_options);
	}

	cgi_str_pair_list_free(userland_options);
	cgi_str_pair_list_free(kernel_options);
	cgi_str_free(wget_output);
}

static void handle_url_change_save()
{
	int ret;
	char *url_new;
	char user_error_msg[] = "An error occured when saving changes";

	url_new = cgi_query_get_val(ELE_ID_FIRMWARE_URL);
	if (cgi_is_null_or_blank(url_new)) {
		display_url_change("", "Please enter a valid URL");
		return;
	}

	cgi_chop_line_end(url_new);

	if (!cgi_is_valid_url(url_new)) {
		display_url_change(url_new, "The URL is not valid");
		return;
	}

	config_set_firmware_url(url_new);
	ret = config_dump();
	if (ret < 0) {
		report_error(user_error_msg, "Failed to dump at-cgi info");
		return;
	}

	ret = set_flag(UNSAVED_SETTINGS_FLAG_PATH);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to set unsaved settings flag");
	}

	display_software_update_clear();
}

#ifdef FIREWALL

static void firewall_config_rule_dump(FILE *fp, cgiStrPairLL *rules,
		char *rule_type, char *chain)
{
	cgiStrPairList *temp_pair_list;
	cgiStrPairLL *temp_pair_ll;

	temp_pair_ll = rules;
	while (temp_pair_ll) {

		fprintf(fp, "%s%s:", rule_type, chain);

		temp_pair_list = temp_pair_ll->str_pair_list;
		while (temp_pair_list) {
			fprintf(fp, "<%s,%s>", temp_pair_list->name, temp_pair_list->val);
			temp_pair_list = temp_pair_list->next_list;
		}

		fprintf(fp, "\n");

		temp_pair_ll = temp_pair_ll->next_list;

	}
}

static int firewall_config_dump(char *path, firewallConfig* firewall_config, int sudo)
{
	cgiStrPairList *temp_pair_list;
	cgiStrList *temp_str_list;
	int temp_fd, ret;
	FILE *config_fp;
	char template[] = "atcgi_firewall_conf_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, CP_PATH, template, "", NULL};

	if (sudo) {
		copy_args[3] = path;
		temp_fd = mkstemp(template);
		config_fp = fdopen(temp_fd, "w");
	} else {
		config_fp = fopen(path, "w");
	}
	if (config_fp == NULL) {
		return -1;
	}

	fprintf(config_fp, "#\n");
	fprintf(config_fp, "# Firewall data store for at-cgi\n");
	fprintf(config_fp, "#\n\n");

	fprintf(config_fp, "# PRE - Always executed first\n");
	fprintf(config_fp, "# POLICY-<chain> - Chain policy definition\n");
	fprintf(config_fp, "# DEFAULT-ACCEPT-RULE-<chain> - An accept rule to always apply first\n");
	fprintf(config_fp, "# USER-ACCEPT-RULE-<chain> - A user defined accept rule\n");
	fprintf(config_fp, "# OPTION-<option> - A specific option\n");
	fprintf(config_fp, "# INTERFACE-MATCH - Strings that match interfaces that may be specified\n\n");

	temp_str_list = firewall_config->iface_matches;
	while (temp_str_list) {
		fprintf(config_fp, "%s%s\n", FIREWALL_CONF_ELE_IFACE_MATCH,
				temp_str_list->str);
		temp_str_list = temp_str_list->next_list;
	}
	fprintf(config_fp, "\n");


	temp_pair_list = firewall_config->options;
	while (temp_pair_list) {
		fprintf(config_fp, "%s%s:%s\n", FIREWALL_CONF_ELE_OPTION,
				temp_pair_list->name, temp_pair_list->val);
		temp_pair_list = temp_pair_list->next_list;
	}
	fprintf(config_fp, "\n");

	temp_str_list = firewall_config->pre;
	while (temp_str_list) {
		fprintf(config_fp, "%s%s\n", FIREWALL_CONF_ELE_PRE,
				temp_str_list->str);
		temp_str_list = temp_str_list->next_list;
	}
	fprintf(config_fp, "\n");

	temp_pair_list = firewall_config->policies;
	while (temp_pair_list) {
		fprintf(config_fp, "%s%s:%s\n", FIREWALL_CONF_ELE_POLICY,
				temp_pair_list->name, temp_pair_list->val);
		temp_pair_list = temp_pair_list->next_list;
	}
	fprintf(config_fp, "\n");

	firewall_config_rule_dump(config_fp, firewall_config->def_rules_in,
			FIREWALL_CONF_ELE_DEFAULT_RULE, "INPUT");
	fprintf(config_fp, "\n");

	firewall_config_rule_dump(config_fp, firewall_config->user_rules_in,
			FIREWALL_CONF_ELE_USER_RULE, "INPUT");
	fprintf(config_fp, "\n");

	firewall_config_rule_dump(config_fp, firewall_config->def_rules_out,
			FIREWALL_CONF_ELE_DEFAULT_RULE, "OUTPUT");
	fprintf(config_fp, "\n");

	firewall_config_rule_dump(config_fp, firewall_config->user_rules_out,
			FIREWALL_CONF_ELE_USER_RULE, "OUTPUT");
	fprintf(config_fp, "\n");

	firewall_config_rule_dump(config_fp, firewall_config->def_rules_forwd,
			FIREWALL_CONF_ELE_DEFAULT_RULE, "FORWARD");
	fprintf(config_fp, "\n");

	firewall_config_rule_dump(config_fp, firewall_config->user_rules_forwd,
			FIREWALL_CONF_ELE_USER_RULE, "FORWARD");
	fprintf(config_fp, "\n");

	fclose(config_fp);

	if (sudo) {
		ret = cgi_exec(copy_args[0], copy_args);
		if (ret < 0) {
			return -1;
		}
		unlink(template);
	}

	return 0;
}

static firewallConfig *firewall_config_parse_user_input(void)
{
	char *temp;
	firewallConfig *firewall_config;

	firewall_config = firewall_config_load_auto();
	if (!firewall_config) {
		return NULL;
	}

	temp = cgi_query_get_val(ELE_ID_FIREWALL_IN_POLICY);
	if (strcmp(temp, ELE_ID_FIREWALL_IN_POLICY_ACCEPT) == 0) {
		cgi_str_pair_list_add(&firewall_config->policies, "INPUT", "ACCEPT");
	} else {
		cgi_str_pair_list_add(&firewall_config->policies, "INPUT", "DROP");
	}

	temp = cgi_query_get_val(ELE_ID_FIREWALL_OUT_POLICY);
	if (strcmp(temp, ELE_ID_FIREWALL_OUT_POLICY_ACCEPT) == 0) {
		cgi_str_pair_list_add(&firewall_config->policies, "OUTPUT", "ACCEPT");
	} else {
		cgi_str_pair_list_add(&firewall_config->policies, "OUTPUT", "DROP");
	}

	temp = cgi_query_get_val(ELE_ID_FIREWALL_IP_FORWD);
	if (temp) {
		cgi_str_pair_list_add(&firewall_config->options, "IP_FORWARD", "yes");
	} else {
		cgi_str_pair_list_add(&firewall_config->options, "IP_FORWARD", "no");
	}

	temp = cgi_query_get_val(ELE_ID_FIREWALL_FORWD_POLICY);
	if (strcmp(temp, ELE_ID_FIREWALL_FORWD_POLICY_ACCEPT) == 0) {
		cgi_str_pair_list_add(&firewall_config->policies, "FORWARD", "ACCEPT");
	} else {
		cgi_str_pair_list_add(&firewall_config->policies, "FORWARD", "DROP");
	}

	return firewall_config;
}

cgiStrPairList *firewall_rule_parse_user_input(char **chain, int *rule_number)
{
	char *temp_val;
	cgiStrPairList *rule_list = NULL;

	*chain 			= cgi_query_get_val(ELE_ID_FIREWALL_RULE_CHAIN);
	temp_val 		= cgi_query_get_val(ELE_ID_FIREWALL_RULE_RULE_NUMBER);
	*rule_number 	= atoi(temp_val);

	temp_val = cgi_query_get_val(ELE_ID_FIREWALL_RULE_PROTOCOL);
	if (strcmp(temp_val, ELE_ID_FIREWALL_RULE_TCP) == 0) {
		cgi_str_pair_list_add(&rule_list, "-p", "tcp");
	} else if (strcmp(temp_val, ELE_ID_FIREWALL_RULE_UDP) == 0) {
		cgi_str_pair_list_add(&rule_list, "-p", "udp");
	} else {
		cgi_str_pair_list_add(&rule_list, "-p", "icmp");
	}

	if (strcmp(temp_val, ELE_ID_FIREWALL_RULE_ICMP) != 0) {
		temp_val = cgi_query_get_val(ELE_ID_FIREWALL_RULE_SERVICE);
		if (strcmp(temp_val, ELE_ID_FIREWALL_RULE_SERVICE_KNOWN) == 0) {
			cgi_str_pair_list_add(&rule_list, "port-sel-typ", FIREWALL_CONF_LINE_WELL_KNOWN);
			temp_val = cgi_query_get_val(ELE_ID_FIREWALL_RULE_KNOWN);
		} else {
			cgi_str_pair_list_add(&rule_list, "port-sel-typ", FIREWALL_CONF_LINE_DIRECT);
			temp_val = cgi_query_get_val(ELE_ID_FIREWALL_RULE_PORT);
		}
		cgi_str_pair_list_add(&rule_list, "--dport", temp_val);
	}

	temp_val = cgi_query_get_val(ELE_ID_FIREWALL_RULE_IFACE_IN);
	if (temp_val && strcmp(temp_val, "Any") != 0) {
		cgi_str_pair_list_add(&rule_list, "-i", temp_val);
	}
	temp_val = cgi_query_get_val(ELE_ID_FIREWALL_RULE_IFACE_OUT);
	if (temp_val && strcmp(temp_val, "Any") != 0) {
		cgi_str_pair_list_add(&rule_list, "-o", temp_val);
	}

	return rule_list;
}

int firewall_rule_check_user_input(cgiStrPairList *rule_list, cgiStr **error_msg)
{
	int i;
	char *temp_val;

	temp_val = cgi_str_pair_list_get_val(rule_list, "-p");

	if (strcmp(temp_val, "icmp") != 0) {

		temp_val = cgi_str_pair_list_get_val(rule_list, "port-sel-typ");

		if (strcmp(temp_val, FIREWALL_CONF_LINE_DIRECT) == 0) {

			temp_val = cgi_str_pair_list_get_val(rule_list, "--dport");
			if (cgi_is_null_or_blank(temp_val)) {
				cgi_str_add(error_msg, "Please enter a port number");
				return -1;
			}
			for (i = 0; temp_val[i]; i++) {
				if (!isdigit(temp_val[i]) && temp_val[i] != ':' && temp_val[i] != '!') {
					cgi_str_add(error_msg, "Please enter a valid port number \
						(: and ! may also be used)");
					return -1;
				}
			}

		}

	}

	return 0;
}

void firewall_handle_rule_update(char *req_area)
{
	int ret, rule_number;
	cgiStrPairList *rule_list;
	char *chain;
	cgiStr *error_msg;
	firewallConfig *firewall_config;
	char user_error_msg[] = "An error occured while updating configuration";

	firewall_config = firewall_config_load_auto();
	if (!firewall_config) {
		report_error(user_error_msg, "failed to load firewall config");
		return;
	}

	rule_list = firewall_rule_parse_user_input(&chain, &rule_number);
	 /* add default accept */
	cgi_str_pair_list_add(&rule_list, "-j", "ACCEPT");

	error_msg = cgi_str_new(NULL);
	ret = firewall_rule_check_user_input(rule_list, &error_msg);
	if (ret < 0) {
		firewall_display_rule_config_screen(chain, req_area, rule_list, rule_number,
				firewall_config->iface_matches, cgi_str_get(error_msg));
		cgi_str_free(error_msg);
		return;
	}
	cgi_str_free(error_msg);

	if (strcmp(req_area, ACTION_FIREWALL_RULE_OK_EDIT) == 0) {
		if (strcmp(chain, "IN") == 0) {
			cgi_str_pair_ll_replace(&firewall_config->user_rules_in, rule_number, rule_list);
		} else if (strcmp(chain, "OUT") == 0) {
			cgi_str_pair_ll_replace(&firewall_config->user_rules_out, rule_number, rule_list);
		} else {
			cgi_str_pair_ll_replace(&firewall_config->user_rules_forwd, rule_number, rule_list);
		}
	} else {
		if (strcmp(chain, "IN") == 0) {
			cgi_str_pair_ll_add(&firewall_config->user_rules_in, rule_list);
		} else if (strcmp(chain, "OUT") == 0) {
			cgi_str_pair_ll_add(&firewall_config->user_rules_out, rule_list);
		} else {
			cgi_str_pair_ll_add(&firewall_config->user_rules_forwd, rule_list);
		}
	}

	ret = firewall_config_dump(FIREWALL_TEMP_CONF_PATH, firewall_config, 0);
	if (ret < 0) {
		report_error(user_error_msg, "firewall_config_dump() failed");
	}

	firewall_config_free(firewall_config);

	firewall_display_config_screen(NULL);
}

static void firewall_handle_rule_edit(char *req_area)
{
	int row_no = 0, ret;
	char *pairs[1][2];
	firewallConfig *firewall_config;
	cgiStrPairList *rule_list;
	char user_error_msg[] = "An error occured while obtaining firewall info";

	/* check that a row was selected */
	if (strcmp(req_area, ACTION_FIREWALL_IN_EDIT) == 0) {
		cgi_query_get_pairs(FIREWALL_ROW_IN, pairs, 1);
		if (!pairs[0][0]) {
			firewall_display_config_screen(NULL);
			return;
		}
	} else if (strcmp(req_area, ACTION_FIREWALL_OUT_EDIT) == 0) {
		cgi_query_get_pairs(FIREWALL_ROW_OUT, pairs, 1);
		if (!pairs[0][0]) {
			firewall_display_config_screen(NULL);
			return;
		}
	} else if (strcmp(req_area, ACTION_FIREWALL_FORWD_EDIT) == 0) {
		cgi_query_get_pairs(FIREWALL_ROW_FORWD, pairs, 1);
		if (!pairs[0][0]) {
			firewall_display_config_screen(NULL);
			return;
		}
	}

	firewall_config = firewall_config_parse_user_input();
	if (!firewall_config) {
		report_error(user_error_msg, "failed to parse user input");
	}

	ret = firewall_config_dump(FIREWALL_TEMP_CONF_PATH, firewall_config, 0);
	if (ret < 0) {
		firewall_config_free(firewall_config);
		report_error(user_error_msg, "failed to dump firewall config");
		return;
	}

	if (strcmp(req_area, ACTION_FIREWALL_IN_ADD) == 0) {

		firewall_display_rule_config_screen("IN", ACTION_FIREWALL_RULE_OK_ADD,
				NULL, 0, firewall_config->iface_matches, "");

	} else if (strcmp(req_area, ACTION_FIREWALL_IN_EDIT) == 0) {

		sscanf(pairs[0][0], FIREWALL_ROW_IN"%d", &row_no);
		rule_list = cgi_str_pair_ll_get(firewall_config->user_rules_in, row_no);
		firewall_display_rule_config_screen("IN", ACTION_FIREWALL_RULE_OK_EDIT,
				rule_list, row_no, firewall_config->iface_matches, "");

	} else if (strcmp(req_area, ACTION_FIREWALL_OUT_ADD) == 0) {

		firewall_display_rule_config_screen("OUT", ACTION_FIREWALL_RULE_OK_ADD,
				NULL, 0, firewall_config->iface_matches, "");

	} else if (strcmp(req_area, ACTION_FIREWALL_OUT_EDIT) == 0) {

		sscanf(pairs[0][0], FIREWALL_ROW_OUT"%d", &row_no);
		rule_list = cgi_str_pair_ll_get(firewall_config->user_rules_out, row_no);
		firewall_display_rule_config_screen("OUT", ACTION_FIREWALL_RULE_OK_EDIT,
				rule_list, row_no, firewall_config->iface_matches, "");

	} else if (strcmp(req_area, ACTION_FIREWALL_FORWD_ADD) == 0) {

		firewall_display_rule_config_screen("FORWD", ACTION_FIREWALL_RULE_OK_ADD,
				NULL, 0, firewall_config->iface_matches, "");

	} else if (strcmp(req_area, ACTION_FIREWALL_FORWD_EDIT) == 0) {

		sscanf(pairs[0][0], FIREWALL_ROW_FORWD"%d", &row_no);
		rule_list = cgi_str_pair_ll_get(firewall_config->user_rules_forwd, row_no);
		firewall_display_rule_config_screen("FORWD", ACTION_FIREWALL_RULE_OK_EDIT,
				rule_list, row_no, firewall_config->iface_matches, "");

	}

	firewall_config_free(firewall_config);
}

#define SELECTED_PAIRS_MAX	(20)
static void firewall_handle_rule_delete(char *req_area)
{
	int ret = 0, row_no, i;
	char *pairs[SELECTED_PAIRS_MAX][2];
	char *row_no_s;
	cgiStrPairLL **rules;
	firewallConfig *firewall_config;
	char user_error_msg[] = "An error occured while deleting firewall rule";
	char buf[128];

	firewall_config = firewall_config_parse_user_input();
	if (!firewall_config) {
		report_error(user_error_msg, "failed to parse user input");
	}

	if (strcmp(req_area, ACTION_FIREWALL_IN_DEL) == 0) {
		row_no_s = FIREWALL_ROW_IN;
		rules = &firewall_config->user_rules_in;
	} else if (strcmp(req_area, ACTION_FIREWALL_OUT_DEL) == 0) {
		row_no_s = FIREWALL_ROW_OUT;
		rules = &firewall_config->user_rules_out;
	} else {
		row_no_s = FIREWALL_ROW_FORWD;
		rules = &firewall_config->user_rules_forwd;
	}

	cgi_query_get_pairs(row_no_s, pairs, SELECTED_PAIRS_MAX);
	if (!pairs[0][0]) {
		firewall_config_free(firewall_config);
		firewall_display_config_screen(NULL);
		return;
	}

	snprintf(buf, sizeof(buf), "%s%%d", row_no_s);
	for (i = 0; pairs[i][0]; i++) {
		sscanf(pairs[i][0], buf, &row_no);
		ret = cgi_str_pair_ll_delete(rules, row_no-i);
		if (ret < 0) {
			firewall_config_free(firewall_config);
			report_error(user_error_msg, "failed to delete rule list");
			return;
		}
	}

	ret = firewall_config_dump(FIREWALL_TEMP_CONF_PATH, firewall_config, 0);
	if (ret < 0) {
		firewall_config_free(firewall_config);
		report_error(user_error_msg, "firewall_config_dump() failed");
	}

	firewall_config_free(firewall_config);
	firewall_display_config_screen(NULL);
}

static void firewall_handle_config_cancel(void)
{
	int ret;
	char *rm_args[] = {RM_PATH, FIREWALL_TEMP_CONF_PATH, NULL};
	char user_error_msg[] = {"An error occured while clearing changes"};

	if (cgi_file_exists(FIREWALL_TEMP_CONF_PATH)) {
		ret = cgi_exec(rm_args[0], rm_args);
		if (ret < 0) {
			report_error(user_error_msg, "failed to remove temp firewall data store");
			return;
		}
	}

	firewall_display_config_screen(NULL);
}

static void firewall_handle_config_update(void)
{
	int ret, temp_fd;
	FILE *script_fp;
	firewallConfig *firewall_config;
	char template[] = "atcgi_firewall_tmp_XXXXXX";
	char user_error_msg[] = "An error occured while updating the firewall";
	char *rm_args[] = {RM_PATH, FIREWALL_TEMP_CONF_PATH, NULL};
	char *copy_args[] = {SUDO_PATH, CP_PATH, template, FIREWALL_SCRIPT_CONF_PATH, NULL};
	char *firewall_args[] = {SUDO_PATH, FIREWALL_SCRIPT_PATH, NULL};

	firewall_config = firewall_config_parse_user_input();
	if (!firewall_config) {
		report_error(user_error_msg, "failed to parse user input");
	}

	ret = firewall_config_dump(FIREWALL_CONF_PATH, firewall_config, 1);
	if (ret < 0) {
		firewall_config_free(firewall_config);
		report_error(user_error_msg, "failed to dump firewall config");
		return;
	}

	if (cgi_file_exists(FIREWALL_TEMP_CONF_PATH)) {
		ret = cgi_exec(rm_args[0], rm_args);
		if (ret < 0) {
			firewall_config_free(firewall_config);
			report_error(user_error_msg, "failed to remove temp firewall data store");
			return;
		}
	}

	temp_fd = mkstemp(template);
	script_fp = fdopen(temp_fd, "w");

	firewall_config_print_script(script_fp, firewall_config);
	fclose(script_fp);
	firewall_config_free(firewall_config);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
			firewall_config_free(firewall_config);
			report_error(user_error_msg, "failed to dump firewall script");
	}
	unlink(template);

	ret = cgi_exec(firewall_args[0], firewall_args);
	if (ret < 0) {
		report_error(user_error_msg, "failed to run firewall script");
		return;
	}

	ret = set_flag(UNSAVED_SETTINGS_FLAG_PATH);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to set unsaved settings flag");
	}
	firewall_display_config_screen("Firewall updated");
}

static void handle_firewall_request(char *req_area)
{
	if (strcmp(req_area, ACTION_FIREWALL) == 0)  {
		firewall_display_config_screen(NULL);
	} else if (strcmp(req_area, ACTION_FIREWALL_IN_ADD) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_IN_EDIT) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_OUT_ADD) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_OUT_EDIT) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_FORWD_ADD) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_FORWD_EDIT) == 0) {
		firewall_handle_rule_edit(req_area);
	} else if (strcmp(req_area, ACTION_FIREWALL_IN_DEL) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_OUT_DEL) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_FORWD_DEL) == 0) {
		firewall_handle_rule_delete(req_area);
	} else if (strcmp(req_area, ACTION_FIREWALL_RULE_OK_ADD) == 0 ||
			strcmp(req_area, ACTION_FIREWALL_RULE_OK_EDIT) == 0) {
		firewall_handle_rule_update(req_area);
	} else if (strcmp(req_area, ACTION_FIREWALL_RULE_CANCEL) == 0) {
		firewall_display_config_screen(NULL);
	} else if (strcmp(req_area, ACTION_FIREWALL_CANCEL) == 0) {
		firewall_handle_config_cancel();
	} else if (strcmp(req_area, ACTION_FIREWALL_SHOW_SCRIPT) == 0) {
		firewall_display_script();
	} else if (strcmp(req_area, ACTION_FIREWALL_UPDATE) == 0) {
		firewall_handle_config_update();
	} else {
		print_frame_top(SYSTEM_CGI_PATH);
		display_sub_menu(ACTION_FIREWALL);
		report_error("Request could not be processed", "No firewall action match");
	}
}

# endif

# ifdef BRIDGE

/* stp value ranges */
#define BR_PRIO_MIN				(0)
#define BR_PRIO_MAX				(65535)
#define FORWARD_DELAY_MIN		(4)
#define FORWARD_DELAY_MAX		(30)
#define HELLO_TIME_MIN			(1)
#define HELLO_TIME_MAX			(10)
#define MESSAGE_AGE_MIN			(6)
#define MESSAGE_AGE_MAX			(40)
#define PORT_PRIO_MIN			(0)
#define PORT_PRIO_MAX			(63) /* should be 255, but linux 2.6.12 is using 6bits */
#define PORT_COST_MIN			(1)
#define PORT_COST_MAX			(65535)

/* config of ageing time not in stp, so no known limits.
 * Just going with what works with brctl here. */
#define AGEING_MIN				(0)
#define AGEING_MAX				(42000000)

#define BRIDGED_IFACE_NUM_MIN	(2)

static void bridge_config_set_text_val(const char *auto_name, const char *text_name,
		char *bridge_config_text_entry)
{
	char *auto_val = cgi_query_get_val(auto_name);
	char *text_val = cgi_query_get_val(text_name);
	if (!cgi_is_null_or_blank(auto_val)) {
		cgi_strncpy(bridge_config_text_entry, BR_DEFAULT, BRIDGE_CONFIG_ENTRY_LEN_MAX);
	} else {
		if (!cgi_is_null_or_blank(text_val)) {
			cgi_chop_line_end(text_val);
			cgi_strncpy(bridge_config_text_entry, text_val, BRIDGE_CONFIG_ENTRY_LEN_MAX);
		} else {
			bridge_config_text_entry[0] = '\0';
		}
	}
}

static void bridge_config_parse_user_input(bridgeConfig *bridge_config)
{
	char *create_br, *stp_on;
	cgiStrList *iface_list, *iface_list_next;
	char query_name_buf[128];
	char *query_val, *query_val2;

	create_br = cgi_query_get_val(ELE_ID_ACTIVE_BRIDGE);
	if (cgi_is_null_or_blank(create_br)) {
		bridge_config->create_br = 0;
		return;
	} else {
		bridge_config->create_br = 1;
	}

	cgi_str_list_free(bridge_config->bridged_ifaces);
	bridge_config->bridged_ifaces = NULL;
	iface_list = interface_list_get(bridge_config->iface_matches);
	iface_list_next = iface_list;
	while (iface_list_next) {
		snprintf(query_name_buf, sizeof(query_name_buf), "%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str);
		query_val = cgi_query_get_val(query_name_buf);
		if (!cgi_is_null_or_blank(query_val)) {
			cgi_str_list_add(&bridge_config->bridged_ifaces, iface_list_next->str);
		}
		iface_list_next = cgi_str_list_next(iface_list_next);
	}

	bridge_config_set_text_val(ELE_ID_MAC_AGEING_AUTO, ELE_ID_MAC_AGEING_TEXT,
		bridge_config->ageing);

	stp_on = cgi_query_get_val(ELE_ID_ENABLE_STP);
	if (cgi_is_null_or_blank(stp_on)) {
		bridge_config->stp_on = 0;
		return;
	} else {
		bridge_config->stp_on = 1;
	}

	bridge_config_set_text_val(ELE_ID_BRIDGE_PRIO_AUTO, ELE_ID_BRIDGE_PRIO_TEXT,
		bridge_config->br_prio);

	bridge_config_set_text_val(ELE_ID_FORWARDING_AUTO, ELE_ID_FORWARDING_TEXT,
		bridge_config->forward);

	bridge_config_set_text_val(ELE_ID_HELLO_TIME_AUTO, ELE_ID_HELLO_TIME_TEXT,
		bridge_config->hello);

	bridge_config_set_text_val(ELE_ID_MAX_AGE_AUTO, ELE_ID_MAX_AGE_TEXT,
		bridge_config->max_age);

	iface_list_next = bridge_config->bridged_ifaces;
	while (iface_list_next) {

		snprintf(query_name_buf, sizeof(query_name_buf), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_AUTO_TAIL);
		query_val = cgi_query_get_val(query_name_buf);
		snprintf(query_name_buf, sizeof(query_name_buf), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_TEXT_TAIL);
		query_val2 = cgi_query_get_val(query_name_buf);

		if (!cgi_is_null_or_blank(query_val)) {
			cgi_str_pair_list_add(&bridge_config->port_costs, iface_list_next->str, BR_DEFAULT);
		} else {
			if (!cgi_is_null_or_blank(query_val2)) {
				cgi_chop_line_end(query_val2);
				cgi_str_pair_list_add(&bridge_config->port_costs, iface_list_next->str, query_val2);
			} else {
				cgi_str_pair_list_add(&bridge_config->port_costs, iface_list_next->str, '\0');
			}
		}

		snprintf(query_name_buf, sizeof(query_name_buf), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_AUTO_TAIL);
		query_val = cgi_query_get_val(query_name_buf);
		snprintf(query_name_buf, sizeof(query_name_buf), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_TEXT_TAIL);
		query_val2 = cgi_query_get_val(query_name_buf);

		if (!cgi_is_null_or_blank(query_val)) {
			cgi_str_pair_list_add(&bridge_config->port_prios, iface_list_next->str, BR_DEFAULT);
		} else {
			if (!cgi_is_null_or_blank(query_val2)) {
				cgi_chop_line_end(query_val2);
				cgi_str_pair_list_add(&bridge_config->port_prios, iface_list_next->str, query_val2);
			} else {
				cgi_str_pair_list_add(&bridge_config->port_prios, iface_list_next->str, '\0');
			}
		}

		iface_list_next = cgi_str_list_next(iface_list_next);
	}

	cgi_str_list_free(iface_list);
}

/*
 * Returns -1 if a user input error is found.
 */
static int bridge_config_check_user_input(bridgeConfig *bridge_config)
{
	cgiStrPairList *str_pair_list_next;
	char error_buf[128];

	if (!bridge_config->create_br) {
		return 0;
	}

	if (cgi_str_list_length(bridge_config->bridged_ifaces) < BRIDGED_IFACE_NUM_MIN) {
		snprintf(error_buf, sizeof(error_buf), "Please select at least %d interfaces", BRIDGED_IFACE_NUM_MIN);
		display_bridge_settings(bridge_config, error_buf, "");
		return -1;
	}

	/* add checks for stp limits according to spec */

	if (cgi_is_null_or_blank(bridge_config->ageing)) {
		display_bridge_settings(bridge_config, "Please specify an ageing time", "");
		return -1;
	} else if (strcmp(bridge_config->ageing, BR_DEFAULT) != 0) {
		if (!cgi_is_a_number(bridge_config->ageing)) {
			display_bridge_settings(bridge_config, "Ageing time must be a number", "");
			return -1;
		} else if (atoi(bridge_config->ageing) < AGEING_MIN || atoi(bridge_config->ageing) > AGEING_MAX) {
			snprintf(error_buf, sizeof(error_buf), "Ageing time must be a value between %d and %d", AGEING_MIN, AGEING_MAX);
			display_bridge_settings(bridge_config, error_buf, "");
			return -1;
		}
	}

	if (!bridge_config->stp_on) {
		return 0;
	}

	if (cgi_is_null_or_blank(bridge_config->br_prio)) {
		display_bridge_settings(bridge_config, "", "Please specify a bridge priority");
		return -1;
	} else if (strcmp(bridge_config->br_prio, BR_DEFAULT) != 0) {
		if (!cgi_is_a_number(bridge_config->br_prio)) {
			display_bridge_settings(bridge_config, "", "Bridge priority must be a number");
			return -1;
		} else if (atoi(bridge_config->br_prio) < BR_PRIO_MIN || atoi(bridge_config->br_prio) > BR_PRIO_MAX) {
			snprintf(error_buf, sizeof(error_buf), "Bridge priority must be a value between %d and %d", BR_PRIO_MIN, BR_PRIO_MAX);
			display_bridge_settings(bridge_config, "", error_buf);
			return -1;
		}
	}

	if (cgi_is_null_or_blank(bridge_config->forward)) {
		display_bridge_settings(bridge_config, "", "Please specify a forwarding delay");
		return -1;
	} else if (strcmp(bridge_config->forward, BR_DEFAULT) != 0) {
		if (!cgi_is_a_number(bridge_config->forward)) {
			display_bridge_settings(bridge_config, "", "Forwarding delay must be a number");
			return -1;
		} else if (atoi(bridge_config->forward) < FORWARD_DELAY_MIN || atoi(bridge_config->forward) > FORWARD_DELAY_MAX) {
			snprintf(error_buf, sizeof(error_buf), "Forwarding delay must be a value between %d and %d", FORWARD_DELAY_MIN, FORWARD_DELAY_MAX);
			display_bridge_settings(bridge_config, "", error_buf);
			return -1;
		}
	}

	if (cgi_is_null_or_blank(bridge_config->hello)) {
		display_bridge_settings(bridge_config, "", "Please specify a hello time");
		return -1;
	} else if (strcmp(bridge_config->hello, BR_DEFAULT) != 0) {
		if (!cgi_is_a_number(bridge_config->hello)) {
			display_bridge_settings(bridge_config, "", "Hello time must be a number");
			return -1;
		} else if (atoi(bridge_config->hello) < HELLO_TIME_MIN || atoi(bridge_config->hello) > HELLO_TIME_MAX) {
			snprintf(error_buf, sizeof(error_buf), "Hello time must be a value between %d and %d", HELLO_TIME_MIN, HELLO_TIME_MAX);
			display_bridge_settings(bridge_config, "", error_buf);
			return -1;
		}
	}

	if (cgi_is_null_or_blank(bridge_config->max_age)) {
		display_bridge_settings(bridge_config, "", "Please specify a max age");
		return -1;
	} else if (strcmp(bridge_config->max_age, BR_DEFAULT) != 0) {
		if (!cgi_is_a_number(bridge_config->max_age)) {
			display_bridge_settings(bridge_config, "", "Max age must be a number");
			return -1;
		} else if (atoi(bridge_config->max_age) < MESSAGE_AGE_MIN || atoi(bridge_config->max_age) > MESSAGE_AGE_MAX) {
			snprintf(error_buf, sizeof(error_buf), "Max message age must be a value between %d and %d", MESSAGE_AGE_MIN, MESSAGE_AGE_MAX);
			display_bridge_settings(bridge_config, "", error_buf);
			return -1;
		}
	}

	str_pair_list_next = bridge_config->port_prios;
	while (str_pair_list_next) {
		if (cgi_is_null_or_blank(str_pair_list_next->val)) {
			snprintf(error_buf, sizeof(error_buf), "Please specify a path priority for %s", str_pair_list_next->name);
			display_bridge_settings(bridge_config, "", error_buf);
			return -1;
		} else if (strcmp(str_pair_list_next->val, BR_DEFAULT) != 0) {
			if (!cgi_is_a_number(str_pair_list_next->val)) {
				snprintf(error_buf, sizeof(error_buf), "Path priority for %s must be a number", str_pair_list_next->name);
				display_bridge_settings(bridge_config, "", error_buf);
				return -1;
			} else if (atoi(str_pair_list_next->val) < PORT_PRIO_MIN || atoi(str_pair_list_next->val) > PORT_PRIO_MAX) {
				snprintf(error_buf, sizeof(error_buf), "Path priority for %s must be a value between %d and %d", str_pair_list_next->name, PORT_PRIO_MIN, PORT_PRIO_MAX);
				display_bridge_settings(bridge_config, "", error_buf);
				return -1;
			}
		}
		str_pair_list_next = str_pair_list_next->next_list;
	}

	str_pair_list_next = bridge_config->port_costs;
	while (str_pair_list_next) {
		if (cgi_is_null_or_blank(str_pair_list_next->val)) {
			snprintf(error_buf, sizeof(error_buf), "Please specify a path cost for %s", str_pair_list_next->name);
			display_bridge_settings(bridge_config, "", error_buf);
			return -1;
		} else if (strcmp(str_pair_list_next->val, BR_DEFAULT) != 0) {
			if (!cgi_is_a_number(str_pair_list_next->val)) {
				snprintf(error_buf, sizeof(error_buf), "Path cost for %s must be a number", str_pair_list_next->name);
				display_bridge_settings(bridge_config, "", error_buf);
				return -1;
			} else if (atoi(str_pair_list_next->val) < PORT_COST_MIN || atoi(str_pair_list_next->val) > PORT_COST_MAX) {
				snprintf(error_buf, sizeof(error_buf), "Path cost for %s must be a value between %d and %d", str_pair_list_next->name, PORT_COST_MIN, PORT_COST_MAX);
				display_bridge_settings(bridge_config, "", error_buf);
				return -1;
			}
		}
		str_pair_list_next = str_pair_list_next->next_list;
	}

	return 0;
}

static void bridge_config_blank2default(char *value)
{
	if (value[0] == '\0') {
		cgi_strncpy(value, BR_DEFAULT, BRIDGE_CONFIG_ENTRY_LEN_MAX);
	}
}

static void print_row_from_list(FILE *temp_fp, cgiStrList *list, char *row_name)
{
	cgiStrList *list_next = list;

	fprintf(temp_fp, "%s='%s", row_name, list_next->str);
	list_next = cgi_str_list_next(list_next);

	while (list_next) {
		fprintf(temp_fp, " %s", list_next->str);
		list_next = cgi_str_list_next(list_next);
	}

	fprintf(temp_fp, "'\n");
}

static int bridge_config_dump(bridgeConfig *bridge_config)
{
	int ret, temp_fd;
	FILE *temp_fp;
	char template[] = "br_conf_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, "cp", template, BRCONFIG_PATH, NULL};
	char user_error_msg[] = "An error occured while updating configuration";
	cgiStrPairList *pair_list_next;

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		report_error(user_error_msg, "Failed to make temp file");
		return -1;
	}

	bridge_config_blank2default(bridge_config->ageing);
	bridge_config_blank2default(bridge_config->br_prio);
	bridge_config_blank2default(bridge_config->forward);
	bridge_config_blank2default(bridge_config->hello);
	bridge_config_blank2default(bridge_config->gc_int);
	bridge_config_blank2default(bridge_config->max_age);

	fprintf(temp_fp, "#! /bin/sh\n\n");
	fprintf(temp_fp, "CREATE_BRIDGE=%d\n\n", bridge_config->create_br);
	fprintf(temp_fp, "BRIDGE_NAME=%s\n", bridge_config->bridge_name);
	fprintf(temp_fp, "SETAGEING=%s\n", bridge_config->ageing);
	print_row_from_list(temp_fp, bridge_config->bridged_ifaces, "BRIDGED_IFACES");
	print_row_from_list(temp_fp, bridge_config->iface_matches, "IFACE_MATCHES");
	fprintf(temp_fp, "STP_ON=%d\n", bridge_config->stp_on);
	fprintf(temp_fp, "SETBRIDGEPRIO=%s\n", bridge_config->br_prio);
	fprintf(temp_fp, "SETFD=%s\n", bridge_config->forward);
	fprintf(temp_fp, "SETHELLO=%s\n", bridge_config->hello);
	fprintf(temp_fp, "SETGCINT=%s\n", bridge_config->gc_int);
	fprintf(temp_fp, "SETMAXAGE=%s\n", bridge_config->max_age);

	pair_list_next = bridge_config->port_costs;
	while (pair_list_next) {
		if (pair_list_next->val[0] == '\0') {
			fprintf(temp_fp, "SETPATHCOST_%s=%s\n", pair_list_next->name, BR_DEFAULT);
		} else {
			fprintf(temp_fp, "SETPATHCOST_%s=%s\n", pair_list_next->name, pair_list_next->val);
		}
		pair_list_next = cgi_str_pair_list_next(pair_list_next);
	}

	pair_list_next = bridge_config->port_prios;
	while (pair_list_next) {
		if (pair_list_next->val[0] == '\0') {
			fprintf(temp_fp, "SETPATHPRIO_%s=%s\n", pair_list_next->name, BR_DEFAULT);
		} else {
			fprintf(temp_fp, "SETPATHPRIO_%s=%s\n", pair_list_next->name, pair_list_next->val);
		}
		pair_list_next = cgi_str_pair_list_next(pair_list_next);
	}

	fclose(temp_fp);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to update bridges config file");
		return -1;
	}
	unlink(template);

	return 0;
}

static int system_config_update(bridgeConfig *bridge_config_new,
		bridgeConfig *bridge_config_current)
{
	int ret;

	if (bridge_config_new->create_br) {
		config_set_primary_if(BRIDGE_NAME);
	} else {
		config_set_primary_if(bridge_config_current->bridged_ifaces->str);
	}

	ret = config_dump();
	if (ret < 0) {
		return -1;
	}

	return 0;
}

/* When enabling the bridge, the current primary interface ip config
 * is copied to the bridge interface. When disabling the bridge, the current
 * ip config of the bridge interface is copied to the first interface
 * previously bridged. */
static int network_info_update(bridgeConfig *bridge_config_new,
		bridgeConfig *bridge_config_current)
{
	int ret;
	networkInfo network_info;

	if (bridge_config_new->create_br) {
		ret = get_current_network_info(config_get_primary_if(), &network_info);
		if (ret < 0) {
			return -1;
		}
		ret = save_network_info(BRIDGE_NAME, &network_info);
		if (ret < 0) {
			return -1;
		}
	} else {
		ret = get_current_network_info(BRIDGE_NAME, &network_info);
		if (ret < 0) {
			return -1;
		}
		ret = save_network_info(bridge_config_current->bridged_ifaces->str, &network_info);
		if (ret < 0) {
			return -1;
		}
	}

	return 0;
}
/*
static int networking_restart(bridgeConfig *bridge_config_new,
		bridgeConfig *bridge_config_current)
{
	pid_t pid;
	char *br_remove[] = {SUDO_PATH, BRIDGE_INIT_PATH, "remove", NULL};
	char *br_create[] = {SUDO_PATH, BRIDGE_INIT_PATH, "create", NULL};

	pid = fork();
	if (pid == 0) {

		close(1);
		close(2);

		networking_down(config_get_primary_if(), WAIT_FOR_IFACE_DOWN);

		cgi_exec(br_remove[0], br_remove);

		if (bridge_config_new->create_br != bridge_config_current->create_br) {

			network_info_update(bridge_config_new, bridge_config_current);

			system_config_update(bridge_config_new, bridge_config_current);

		}

		cgi_exec(br_create[0], br_create);

		// wait for bridge up!!!! ??

		networking_up(config_get_primary_if());

		exit(0);

	} else if (pid < 0) {

		return -1;

	}

	return 0;
}
*/
static void handle_bridge_config_save()
{
	int ret;
	bridgeConfig *bridge_config_new, *bridge_config_current;
	char user_error_msg[] = "An error occured when saving bridge configuration";
	char *flatfsd_args[] = {SUDO_PATH, FLATFSD_PATH, "-s", NULL};
	char *current_iface = config_get_primary_if();

	bridge_config_current = bridge_config_load();
	if (!bridge_config_current) {
		return_crit_error(user_error_msg, "Failed to load bridge config");
		return;
	}

	bridge_config_new = bridge_config_load();
	if (!bridge_config_new) {
		return_crit_error(user_error_msg, "Failed to load bridge config");
		return;
	}

	bridge_config_parse_user_input(bridge_config_new);

	ret = bridge_config_check_user_input(bridge_config_new);
	if (ret < 0) {
		return;
	}

	ret = bridge_config_dump(bridge_config_new);
	if (ret < 0) {
		return_crit_error(user_error_msg, "Failed to dump bridge config");
		return;
	}

	ret = network_info_update(bridge_config_new, bridge_config_current);
	if (ret < 0) {
		return_crit_error(user_error_msg, "Failed to update network config");
		return;
	}

	ret = system_config_update(bridge_config_new, bridge_config_current);
	if (ret < 0) {
		return_crit_error(user_error_msg, "Failed to update system config");
		return;
	}

	ret = cgi_exec(flatfsd_args[0], flatfsd_args);
	if (ret < 0) {
		return_crit_error(user_error_msg, "Failed to save system config");
		return;
	}

	bridge_config_free(bridge_config_current);
	bridge_config_free(bridge_config_new);

	reboot_system(current_iface);
}

static void handle_bridge_request(char *req_area)
{
	char user_error_msg[] = "Request could not be processed";

	if (strcmp(req_area, ACTION_BRIDGE_STATUS) == 0) {
		display_bridge_state();
	} else if (strcmp(req_area, ACTION_BRIDGE_CONFIG) == 0) {
		display_bridge_settings(NULL, "", "");
	} else if (strcmp(req_area, ACTION_BRIDGE_SAVE) == 0) {
		handle_bridge_config_save();
	} else if (strcmp(req_area, ACTION_BRIDGE_CANCEL) == 0) {
		display_bridge_state();
	} else {
		print_frame_top(SYSTEM_CGI_PATH);
		display_sub_menu("");
		report_error(user_error_msg, "No action match");
	}
}

# endif

static void handle_local_request()
{
	char *req_area;
	char user_error_msg[] = "Request could not be processed";

	print_content_type();
	print_html_head(SYSTEM_CGI_PATH);

	req_area = cgi_query_get_val("req_area");

	if (!req_area) {

		/* No set req_area means a new get */
		display_system_overview();

	} else if (strcmp(req_area, ACTION_OVERVIEW) == 0)  {
		display_system_overview();
	} else if (strcmp(req_area, ACTION_OVERVIEW_IFCONFIG) == 0) {
		display_ifconfig();
	} else if (strcmp(req_area, ACTION_OVERVIEW_MEMINFO) == 0) {
		display_meminfo();
#ifdef SYSLOGD
	} else if (strcmp(req_area, ACTION_OVERVIEW_MESSAGES) == 0) {
		display_messages();
#endif
	} else if (strcmp(req_area, ACTION_NETWORK) == 0) {
		display_network_settings(NULL, NULL, "", NULL);
	} else if (strcmp(req_area, ACTION_NETWORK_SAVE) == 0) {
		save_network_settings();
	} else if (strcmp(req_area, ACTION_NETWORK_CANCEL) == 0) {
		display_network_settings(NULL, NULL, "", NULL);
# ifdef FIREWALL
	} else if (strncmp(req_area, ACTION_FIREWALL, strlen(ACTION_FIREWALL)) == 0) {
		handle_firewall_request(req_area);
# endif
# ifdef BRIDGE
	} else if (strncmp(req_area, ACTION_BRIDGE, strlen(ACTION_BRIDGE)) == 0) {
		handle_bridge_request(req_area);
# endif
	} else if (strcmp(req_area, ACTION_PASSWORD) == 0) {
		display_password_settings("", "", "", "", "", NULL);
	} else if (strcmp(req_area, ACTION_PASSWORD_SAVE) == 0) {
		change_password();
	} else if (strcmp(req_area, ACTION_PASSWORD_CANCEL) == 0) {
		display_password_settings("", "", "", "", "", NULL);
	} else if (strcmp(req_area, ACTION_UPDATES) == 0) {
		display_software_update_clear();
	} else if (strcmp(req_area, ACTION_UPDATES_GET_OPT) == 0) {
		get_update_options();
	} else if (strcmp(req_area, ACTION_UPDATES_USERLAND) == 0) {
		update_software();
	} else if (strcmp(req_area, ACTION_UPDATES_KERNEL) == 0) {
		update_software();
	} else if (strcmp(req_area, ACTION_UPDATES_URL) == 0) {
		display_url_change(NULL, "");
	} else if (strcmp(req_area, ACTION_UPDATES_URL_SAVE) == 0) {
		handle_url_change_save();
	} else if (strcmp(req_area, ACTION_UPDATES_URL_CANCEL) == 0) {
		display_software_update_clear();
	} else if (strcmp(req_area, ACTION_UPDATES_CHECK) == 0) {
		handle_update_in_progress();
	} else if (strcmp(req_area, ACTION_SYSTEM_STATE) == 0) {
		display_system_state_settings(NULL);
	} else if (strcmp(req_area, ACTION_SETTINGS_DETAILS) == 0) {
		display_system_state_settings(NULL);
	} else if (strcmp(req_area, ACTION_SYSTEM_STATE_SAVE) == 0) {
		commit_settings();
	} else if (strcmp(req_area, ACTION_SYSTEM_STATE_LOAD) == 0) {
		load_settings();
	} else if (strcmp(req_area, ACTION_SYSTEM_STATE_INIT) == 0) {
		init_settings();
	} else if (strcmp(req_area, ACTION_SYSTEM_STATE_REBOOT) == 0) {
		reboot_system(NULL);
	} else {
		print_frame_top(SYSTEM_CGI_PATH);
		display_sub_menu(ACTION_OVERVIEW);
		report_error(user_error_msg, "No action match");
	}

	print_frame_bottom();
	print_html_tail();
}

int main(int argc, char * argv[])
{
	int ret;

	at_cgi_debug("system - up and running");

 	if (cgi_get_content_length() > MAX_QUERY_DATA_SIZE) {
 		return_crit_error("Invalid query",
			"query content length too big");
		exit(EXIT_SUCCESS);
 	}

	ret = config_load();
	if (ret < 0) {
		return_crit_error("A required system file could not be loaded",
			"failed to load config");
		exit(EXIT_SUCCESS);
	}

	ret = cgi_query_process();
	if (ret < 0) {
		return_crit_error("Request could not be processed",
			"Error when processing request arguments");
		exit(EXIT_SUCCESS);
	}

	ret = system_wide_handling();

	if (ret == LOCAL_HANDLING_REQUIRED) {
		handle_local_request();
	}

	config_free();
	cgi_query_free();

	at_cgi_debug("system - finished");

	exit(EXIT_SUCCESS);
}
