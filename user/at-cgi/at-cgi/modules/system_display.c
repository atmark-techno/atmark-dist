/*
 * modules/system_display.c - system module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <ctype.h>

#include <simple-cgi-app-html-parts.h>
#include <simple-cgi-app-io.h>
#include <simple-cgi-app-alloc.h>
#include <simple-cgi-app-misc.h>

#include <frame-html.h>
#include <misc-utils.h>
#include <core.h>
#include <menu.h>
#include <system-config.h>
#include <common.h>

#include "system_display.h"

void display_sub_menu(char *selected_link)
{
	menuItem menu_items[] = {
		{"System Overview", 	ACTION_OVERVIEW},
		{"Network",				ACTION_NETWORK},
#ifdef FIREWALL
		{"Firewall",			ACTION_FIREWALL},
#endif
#ifdef BRIDGE
		{"Bridge",				ACTION_BRIDGE_STATUS},
#endif
		{"Password", 			ACTION_PASSWORD},
		{"Firmware", 			ACTION_UPDATES},
		{"Save &amp; Load",		ACTION_SYSTEM_STATE},
	};

	print_sub_menu(menu_items, sizeof(menu_items)/sizeof(menuItem),
		selected_link);
}

void display_system_overview(void)
{
	int ret;
	float avg_load5;
	struct sysinfo info;
	char *hostname_args[] = {HOSTNAME_PATH,  NULL};
	char *kernel_ver_args[] = {UNAME_PATH, "-r", NULL};
	char user_error_msg[] = "An error occured while obtaining system info";
	networkInfo network_info;

	print_frame_top(SYSTEM_CGI_PATH);

	ret = sysinfo(&info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get sysinfo");
		return;
	}
	ret = get_current_network_info(config_get_primary_if(), &network_info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get current network info");
		return;
	}
	ret = get_sys_load(NULL, &avg_load5, NULL);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get current network info");
		return;
	}

	display_sub_menu(ACTION_OVERVIEW);

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Network Info</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">IP Address</td><td>");
	print_current_effective_ip_address(config_get_primary_if());
	printf(" (");
	if (network_info.use_dhcp) {
		printf("auto");
	} else {
		printf("static");
	}
	printf(")</td></tr>\n");
	printf("<tr><td class=\"left\">MAC Address</td><td>");
	print_ifconfig_part(config_get_primary_if(), "HWaddr ");
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Host name</td><td>");
	cgi_print_command(hostname_args[0], hostname_args);
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Traffic</td><td>");
	print_ifconfig_part(config_get_primary_if(), "RX packets:");
	printf(" packets received</td></tr>\n");
	printf("<tr><td class=\"left\"></td><td>");
	print_ifconfig_part(config_get_primary_if(), "TX packets:");
	printf(" packets sent</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Zeroconf</td><td>");
	print_process_state("avahi-daemon");
	printf("</td></tr>");

	printf("</table>\n");

	printf("<p><a href=\"\" onclick=\"linkregevent('%s'); return false\">show ifconfig</a></p>\n",
		ACTION_OVERVIEW_IFCONFIG);

	printf("<hr />\n");

	printf("<h3>System State</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">Load</td>");
	printf("<td>%.2f (5min average)</td></tr>\n", avg_load5);

	printf("<tr class=\"new\"><td class=\"left\">Memory</td>");
	printf("<td>%ldK available, %ldK free</td></tr>\n",
		info.totalram / 1024, info.freeram / 1024);

	printf("<tr class=\"new\"><td class=\"left\">Uptime</td><td>");
	print_uptime();
	printf("</td></tr>\n");

	printf("</table>\n");

	printf("<p><a href=\"\" onclick=\"linkregevent('%s'); return false\">show meminfo</a><br />\n",
		ACTION_OVERVIEW_MEMINFO);
#ifdef SYSLOGD
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">show syslog</a></p>\n",
		ACTION_OVERVIEW_MESSAGES);
#endif

	printf("<hr />\n");

	printf("<h3>Firmware</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">Version</td><td>");
	cgi_print_file(DISTNAME_PATH);
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Kernel</td><td>");
	cgi_print_command(kernel_ver_args[0], kernel_ver_args);
	printf("</td></tr>\n");

	printf("</table>\n");

	printf("<hr />\n");
	printf("</div>\n");
}

static void display_network_settings_jb(void)
{
	printf("<script type=\"text/javascript\">\n");
	printf("function setoptions() {\n");
	printf("state = false;\n");
	printf("if (document.%s.%s[0].checked == true) {\n",
		FORM_NAME, ELE_ID_IP_ASSIGN_TYPE);
	printf("state = true;\n");
	printf("}\n");
	printf("document.%s.%s.disabled = state;\n", FORM_NAME, ELE_ID_IP_ADDRESS);
	printf("document.%s.%s.disabled = state;\n", FORM_NAME, ELE_ID_IP_NETMASK);
	printf("document.%s.%s.disabled = state;\n", FORM_NAME, ELE_ID_IP_GATEWAY);
	printf("document.%s.%s.disabled = state;\n", FORM_NAME, ELE_ID_IP_DNSSERVER);
	printf("}\n");
	printf("window.onload = setoptions;\n");
	printf("</script>\n");
}

void display_network_settings(
	networkInfo *network_info,
	char *hostname,
	char *err1,
	char *message)
{

	int ret;
	int static_ip = 0, dhcp_ip = 0;
	char buffer[HOST_NAME_MAX];
	char user_error_msg[] = "An error occured while displaying configuration";
	networkInfo got_network_info;

	print_frame_top(SYSTEM_CGI_PATH);

	if (network_info == NULL) {
		ret = get_current_network_info(config_get_primary_if(), &got_network_info);
		if (ret < 0) {
			report_error(user_error_msg, "Failed to get current network info");
			return;
		}
		network_info = &got_network_info;
	}
	if (network_info->use_dhcp) {
		dhcp_ip = 1;
	} else {
		static_ip = 1;
	}

	if (hostname == NULL) {
		ret = gethostname(buffer, sizeof(buffer));
		if (ret < 0) {
			report_error(user_error_msg, "Failed to get hostname");
			return;
		}
		hostname = buffer;
	}

	display_sub_menu(ACTION_NETWORK);

	if (message) {
		print_top_info_message(message);
	}

	display_network_settings_jb();

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Network Settings</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");
	printf("<tr class=\"new\"><td class=\"left\">AUTO IP</td><td colspan=\"2\">");
	cgi_print_radio_button(ELE_ID_IP_ASSIGN_TYPE, ELE_ID_IP_DHCP, dhcp_ip, "setoptions();");
	printf("</td></tr></table>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");
	printf("<tr><td valign=\"top\" class=\"left\">STATIC IP</td><td valign=\"top\" width=\"5%%\">");
	cgi_print_radio_button(ELE_ID_IP_ASSIGN_TYPE, ELE_ID_IP_STATIC, static_ip, "setoptions();");
	printf("</td><td>\n");

	printf("<table border=\"0\" cellpadding=\"2\" cellspacing=\"0\" width=\"100%%\">\n");

	printf("<tr><td class=\"left_sub\">Address</td><td>");
	cgi_print_text_box(ELE_ID_IP_ADDRESS, network_info->address, 15, 15);
	printf("</td></tr>\n<tr><td class=\"left_sub\">Netmask</td><td>");
	cgi_print_text_box(ELE_ID_IP_NETMASK, network_info->netmask, 15, 15);
	printf("</td></tr>\n<tr><td class=\"left_sub\">Gateway</td><td>");
	cgi_print_text_box(ELE_ID_IP_GATEWAY, network_info->gateway, 15, 15);
	printf(" <span class=\"small\">(optional)</span></td></tr><tr><td class=\"left_sub\">DNS Server</td><td>");
	cgi_print_text_box(ELE_ID_IP_DNSSERVER, network_info->nameserver, 15, 15);
	printf(" <span class=\"small\">(optional)</span></td></tr>\n");

	printf("</table>\n");

	printf("</td></tr>\n");

	printf("</table>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">Hostname</td><td>");
	cgi_print_text_box(ELE_ID_HOSTNAME, hostname, 30, 30);
	printf("</td></tr>\n");

	printf("</table>\n");

	printf("<p class=\"inline_error\">%s</p>\n", err1);

	printf("<hr />\n");

	cgi_print_button("Update", ACTION_NETWORK_SAVE, "");
	printf(" ");
	cgi_print_button("Cancel", ACTION_NETWORK_CANCEL, "");

	printf("</div>\n");
}

void display_password_settings(
	char *username,
	char *curr_pass,
	char *new_pass,
	char *pass_check,
	char *err1,
	char *message)
{
	int ret;
	char usr_name[USERNAME_MAX];

	if (cgi_is_null_or_blank(username)) {
		ret = get_username(usr_name, sizeof(usr_name));
		if (ret < 0) {
			report_error("An error occured displaying password settings",
				"Failed to get username");
				return;
		}
		username = usr_name;
	}

	print_frame_top(SYSTEM_CGI_PATH);

	display_sub_menu(ACTION_PASSWORD);

	if (message) {
		print_top_info_message(message);
	}

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>User and Password Details</h3>\n");

	printf("<div class=\"h_gap_small\"></div>\n");

	printf("<table border=\"0\" cellpadding=\"5\" cellspacing=\"0\" width=\"100%%\" align=\"center\" class=\"details\">\n");

	printf("<tr><td class=\"left\">Username</td><td>");
	cgi_print_text_box(ELE_ID_USERNAME, username, 20, 20);
	printf("</td></tr>\n");

	printf("<tr><td class=\"left\">Current password</td><td>");
	cgi_print_password_box(ELE_ID_CURR_PASSWORD, curr_pass, 20, 20);
	printf("</td></tr>\n");

	printf("<tr><td class=\"left\">New password</td><td>");
	cgi_print_password_box(ELE_ID_PASSWORD1, new_pass, 20, 20);
	printf("</td></tr>\n");

	printf("<tr><td class=\"left\">Confirm new password</td><td>");
	cgi_print_password_box(ELE_ID_PASSWORD2, pass_check, 20, 20);
	printf("</td></tr>\n");

	printf("</table>\n");

	printf("<p class=\"inline_error\">%s</p>\n", err1);

	printf("<hr />\n");

	cgi_print_button("Update", ACTION_PASSWORD_SAVE, "");
	printf(" ");
	cgi_print_button("Cancel", ACTION_PASSWORD_CANCEL, "");

	printf("<div class=\"h_gap_big\"></div>\n");

	printf("</div>\n");
}

void display_update_in_progress(void)
{
	printf("<div class=\"h_gap_verybig\"></div>");
	printf("<div align=\"center\">\
		<img src=\"%s\" alt=\"Admin\" class=\"title_img\"/>\
		</div>",
		TITLE_IMG);
	printf("<div class=\"h_gap_verybig\"></div>");

	printf("<div align=\"center\">\n");

	printf("<script type=\"text/javascript\">\n \
		function autocheck(){\n");

	printf("document.%s.req_area.value = '%s';\n \
		document.%s.submit();\n",
		FORM_NAME,
		ACTION_UPDATES_CHECK,
		FORM_NAME);

	printf("}\n \
		window.onload = setTimeout('autocheck()', 5000); \
		</script>\n");

	printf("<p class=\"update_title\">Updating Firmware</p>\n");

	printf("<div class=\"h_gap_med\"></div>");

	printf("<p align=\"center\">更新が終わるまで%sの電源を切らないでください。<br />\
		更新中に電源が切断された場合、%sが起動できなくなる可能性がありますので、ご注意ください。</p>\n",
		config_get_product_name(), config_get_product_name());

	printf("</div>\n");

	printf("<div class=\"h_gap_verybig\"></div>");
}

static void display_software_update_jb(void)
{
	printf("<script type=\"text/javascript\">\n");

	printf("function setpaths() {\n\
		setuserlandpath();\n\
		setkernelpath();\n\
		updateuserlandeditstate();\n\
		updatekerneleditstate();\n\
		}\n");

	printf("function setuserlandpath(){\n\
		document.%s.%s.value = document.%s.%s.value;\n\
		}\n",
		FORM_NAME,
		ELE_ID_USERLAND_PATH,
		FORM_NAME,
		ELE_ID_USERLAND_SELECT);

	printf("function setkernelpath(){\n\
		document.%s.%s.value = document.%s.%s.value;\n\
		}\n",
		FORM_NAME,
		ELE_ID_KERNEL_PATH,
		FORM_NAME,
		ELE_ID_KERNEL_SELECT);

	printf("function updateuserlandeditstate(){\n\
		if (document.%s.%s.checked == true) {\n\
			state = false;\n\
		} else {\n\
			state = true;\n\
		}\n\
		document.%s.%s.disabled = state;\n\
		}",
		FORM_NAME,
		ELE_ID_USERLAND_EDIT,
		FORM_NAME,
		ELE_ID_USERLAND_PATH);

	printf("function updatekerneleditstate(){\n\
		if (document.%s.%s.checked == true) {\n\
			state = false;\n\
		} else {\n\
			state = true;\n\
		}\n\
		document.%s.%s.disabled = state;\n\
		}",
		FORM_NAME,
		ELE_ID_KERNEL_EDIT,
		FORM_NAME,
		ELE_ID_KERNEL_PATH);

	printf("function setpaths() {\n\
		setuserlandpath();\n\
		setkernelpath();\n\
		updateuserlandeditstate();\n\
		updatekerneleditstate();\n\
		}\n");

	printf("function copypaths(){\n\
		document.%s.%s.value = document.%s.%s.value;\n\
		document.%s.%s.value = document.%s.%s.value;\n\
		}\n",
		FORM_NAME, ELE_ID_USERLAND_PATH_H,
		FORM_NAME, ELE_ID_USERLAND_PATH,
		FORM_NAME, ELE_ID_KERNEL_PATH_H,
		FORM_NAME, ELE_ID_KERNEL_PATH);

	printf("window.onload = setpaths; \
		</script>\n");
}

static void display_software_update(
	cgiStrPairList *userland_options,
	cgiStrPairList *kernel_options,
	softwareUpdateMsgType msg_type,
	char *message)
{
	int netflash_state;
	char user_error_msg[] = "An error occured when displaying update progress";
	cgiStrPairList *blank_options = NULL;

	cgi_str_pair_list_add(&blank_options, "", "");

	netflash_state = is_process_alive("netflash");
	if (netflash_state < 0) {
		report_error(user_error_msg, "Failed to get netflash process state");
		return;
	} else if (netflash_state == PROCESS_STATE_ACTIVE) {
		display_update_in_progress();
		return;
	}

	if (!userland_options) {
		userland_options = blank_options;
	}
	if (!kernel_options) {
		kernel_options = blank_options;
	}

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu("updates");
	display_software_update_jb();

	if (msg_type == SU_MSG_UPDATE_INFO) {
		print_top_info_message(message);
	}

	cgi_print_hidden_field(ELE_ID_USERLAND_PATH_H, "");
	cgi_print_hidden_field(ELE_ID_KERNEL_PATH_H, "");

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Firmware updates</h3>\n");

	printf("<p>イメージファイルのダウンロード完了後、更新完了まで数分間程度かかります。<br />\
		更新中に電源が切断された場合、%sが起動できなくなる可能性がありますので、ご注意ください。<br />\
		また、現在と異なる種類のユーザランドイメージに更新する場合、「Allow all image types」にチェックを入れてください。<br />\
		その際、システム設定の初期化(System : Save &amp; Load -> Restore Defaults)が必要になる場合があります。</p>",
		config_get_product_name());

	printf("<p>\n");
	cgi_print_button("Get firmware options", ACTION_UPDATES_GET_OPT, "");
	if (msg_type == SU_MSG_GET_OPTIONS) {
		printf(" <span class=\"inline_error\">%s</span>", message);
	}
	printf("</p>\n");

	printf("<hr />");

	printf("<h4>Applications (Userland)</h4>\n");

	printf("<div class=\"h_gap_small\"></div>");

	printf("<p>\n");

	cgi_print_select_box_from_str_pair_list(ELE_ID_USERLAND_SELECT, 1,
		userland_options, "", "setuserlandpath()");
	printf("</p>\n");

	printf("<p>\n");
	cgi_print_text_box(ELE_ID_USERLAND_PATH, "", 80, 256);
	printf(" ");
	cgi_print_checkbox(ELE_ID_USERLAND_EDIT, 0, "updateuserlandeditstate()");
	printf(" Edit</p>\n");

	printf("<div class=\"h_gap_small\"></div>");

	if (msg_type == SU_MSG_USERLAND_PATH) {
		printf("<p class=\"inline_error\">%s</p>", message);
	}

	printf("<p>\n");
	cgi_print_button_confirm("Update userland", ACTION_UPDATES_USERLAND, "copypaths()",
		"選択したイメージファイルでApplications (Userland)を更新しますか？");
	cgi_print_checkbox(ELE_ID_ALLOW_OTHER_PRODUCTS, 0, "");
	printf("Allow all image types");
	printf("</p>\n");

	printf("<hr />");

	printf("<h4>Kernel</h4>\n");

	printf("<div class=\"h_gap_small\"></div>");

	printf("<p>\n");
	cgi_print_select_box_from_str_pair_list(ELE_ID_KERNEL_SELECT, 1,
		kernel_options, "", "setkernelpath()");
	printf("</p>\n");

	printf("<p>\n");
	cgi_print_text_box(ELE_ID_KERNEL_PATH, "", 80, 256);
	printf(" ");
	cgi_print_checkbox(ELE_ID_KERNEL_EDIT, 0, "updatekerneleditstate()");
	printf(" Edit</p>\n");

	printf("<div class=\"h_gap_small\"></div>");

	if (msg_type == SU_MSG_KERNEL_PATH) {
		printf("<p class=\"inline_error\">%s</p>", message);
	}

	printf("<p>\n");
	cgi_print_button_confirm("Update kernel", ACTION_UPDATES_KERNEL, "copypaths()",
		"選択したイメージファイルでKernelを更新しますか？");
	printf("</p>\n");

	printf("<hr />");

	printf("<p class=\"small\">Current firmware download location: %s ", config_get_firmware_url());
	printf("<a href=\"\" onclick=\"linkregevent('%s'); \
		return false\">change</a></p>\n",
		ACTION_UPDATES_URL);

	printf("</div>");

	cgi_str_pair_list_free(blank_options);
}

void display_software_update_clear(void)
{
	display_software_update(NULL, NULL, SU_MSG_NONE, NULL);
}

void display_software_update_options(cgiStrPairList *userland_options,
	cgiStrPairList *kernel_options)
{
	display_software_update(userland_options, kernel_options, SU_MSG_NONE,
		NULL);
}

void display_software_update_msg(softwareUpdateMsgType msg_type, char *message)
{
	display_software_update(NULL, NULL, msg_type, message);
}

void display_url_change(char *url, char *err1)
{
	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu("updates");

	if (url == NULL) {
		url = config_get_firmware_url();
	}

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Firmware download location</h3>\n");

	printf("<div class=\"h_gap_small\"></div>");

	printf("<p>\n");
	cgi_print_text_box(ELE_ID_FIRMWARE_URL, url, 90, 128);
	printf("</p>\n");

	printf("<!-- fix for ie one text box only submit problem -->\n");
	printf("<input type=\"text\" style=\"visibility:hidden; display:none;\" />\n");

	printf("<p class=\"inline_error\">%s</p>", err1);

	printf("<div class=\"h_gap_small\"></div>");

	printf("<hr />");

	printf("<p>\n");
	cgi_print_button("Update", ACTION_UPDATES_URL_SAVE, "");
	printf(" ");
	cgi_print_button("Cancel", ACTION_UPDATES_URL_CANCEL, "");
	printf("</p>\n");

	printf("</div>");
}

void display_system_state_settings(char * message)
{
	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu(ACTION_SYSTEM_STATE);

	if (message) {
		print_top_info_message(message);
	}

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Save and Load System Settings</h3>\n");

	printf("<div class=\"h_gap_small\"></div>");

	printf("<p>現在のシステム設定をフラッシュに保存する</p>");
	cgi_print_button_confirm("Save", ACTION_SYSTEM_STATE_SAVE, "",
		"設定をフラッシュに保存しますか？");
	printf("<div class=\"h_gap_small\"></div>");
	printf("<p>現在のシステム設定を破棄し、フラッシュに保存されている元の設定に戻す</p>\n");
	cgi_print_button_confirm("Reload", ACTION_SYSTEM_STATE_LOAD, "",
		"以前の設定に戻してよろしいですか？");
	printf("<div class=\"h_gap_small\"></div>");
	printf("<p>現在のシステム設定を破棄し、初期状態の設定にする (システムの再起動が必要です)<br />\n");
	printf("システム情報を新しく生成しなおすため、再起動時が完了するまでに数分必要です</p>\n");
	cgi_print_button_confirm("Restore Defaults", ACTION_SYSTEM_STATE_INIT, "",
		"設定を初期化してよろしいですか？");
	printf("<div class=\"h_gap_small\"></div>");

	printf("<hr />");

	printf("<h3>System Reboot</h3>\n");

	printf("<p>システムを再起動する</p><p>\n");
	cgi_print_button_confirm("Reboot", ACTION_SYSTEM_STATE_REBOOT, "",
		"システムを再起動してよろしいですか？");
	printf("</p>\n");

	printf("<hr />");

	printf("</div>");
}

void display_system_state_settings_change(void)
{
	int ret;
	networkInfo network_info;
	char *hostname_args[] = {HOSTNAME_PATH, NULL};
	char user_error_msg[] = "An error occured when changing system settings";

	ret = get_current_network_info(config_get_primary_if(), &network_info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get current network info");
		return;
	}

	printf("<div class=\"h_gap_big\"></div>");

	printf("<div align=\"center\" width=\"80%%\" style=\"padding: 0 30px 0 30px;\">\
		<img src=\"%s\" alt=\"Admin\" class=\"title_img\"/>",
		TITLE_IMG);

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p class=\"update_title\">システム設定が変更されました</p>\n");

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p align=\"center\">\
	ネットワーク設定が変更された可能性があります。<br/>\
	ネットワーク接続を切断し、再接続します。<br />\
	WEBブラウザ画面を閉じ、");

	if (strcmp(config_get_product_name(), PRODUCT_NAME_A9) == 0)
		printf("しばらく待ってから");
	else
		printf("%sの赤LEDが消灯するまで待ってから", \
		       config_get_product_name());

	printf("<br />");

	printf("Bonjourなどを利用してトップページにアクセスし直してください。");

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
	print_ifconfig_part(config_get_primary_if(), "HWaddr ");
	printf("</p>");

	printf("</div>");
	printf("<div class=\"h_gap_big\"></div>");
}

void display_network_change(networkInfo *network_info)
{
	char *hostname_args[] = {HOSTNAME_PATH, NULL};

	printf("<div class=\"h_gap_big\"></div>");

	printf("<div align=\"center\" width=\"80%%\" style=\"padding: 0 30px 0 30px;\">\
		<img src=\"%s\" alt=\"Admin\" class=\"title_img\"/>",
		TITLE_IMG);

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p class=\"update_title\">ネットワーク設定が変更されました</p>\n");

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p align=\"center\">\
		ネットワーク接続を切断し、再接続します。<br />\
		WEBブラウザ画面を閉じ、");

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
	if (network_info->use_dhcp) {
		printf("auto");
	} else {
		printf("static (%s)", network_info->address);
	}
	printf("</p>");

	printf("<p align=\"center\">MAC address: ");
	print_ifconfig_part(config_get_primary_if(), "HWaddr ");
	printf("</p>");

	printf("</div>");
	printf("<div class=\"h_gap_big\"></div>");
}

void display_ifconfig(void)
{
	int ret;
	cgiStr *ifconfig_stdout;
	char *args[] = {"ifconfig", "-a", NULL};
	char user_error_msg[] = "Could not obtain system information";

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu("");

	ifconfig_stdout = cgi_str_new(NULL);

	ret = cgi_read_command(&ifconfig_stdout, "/sbin/ifconfig", args);
	if (ret < 0) {
		report_error(user_error_msg, "command_to_str() failed");
		cgi_str_free(ifconfig_stdout);
		return;
	}

	printf("<div class=\"contents_frame\">\n");
	printf("<h3>ifconfig</h3><pre>\n");
	cgi_str_print(ifconfig_stdout);
	printf("</pre>\n");

	printf("<hr />\n");

	cgi_print_button("Back", ACTION_OVERVIEW, "");
	printf(" ");
	cgi_print_button("Refresh", ACTION_OVERVIEW_IFCONFIG, "");
	printf("</div>\n");

	cgi_str_free(ifconfig_stdout);
}

void display_meminfo(void)
{
	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu("");

	printf("<div class=\"contents_frame\">\n");
	printf("<h3>meminfo</h3><pre>\n");
	cgi_print_file(MEMINFO_PATH);
	printf("</pre>\n");

	printf("<hr />\n");

	cgi_print_button("Back", ACTION_OVERVIEW, "");
	printf(" ");
	cgi_print_button("Refresh", ACTION_OVERVIEW_MEMINFO, "");
	printf("</div>\n");
}

#ifdef SYSLOGD
void display_messages(void)
{
	int ret;
	cgiStr *cat_output;
	char *args[] = {SUDO_PATH, "/bin/cat", SYSLOG_PATH, NULL};
	char user_error_msg[] = "Could not obtain system information";

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu("");

	cat_output = cgi_str_new(NULL);

	ret = cgi_read_command(&cat_output, args[0], args);
	if (ret < 0) {
		report_error(user_error_msg, "flex_mem_cat_command() failed");
		cgi_str_free(cat_output);
		return;
	}

	printf("<div class=\"contents_frame\">\n");
	printf("<h3>syslog</h3>");
	cgi_print_button("Back", ACTION_OVERVIEW, "");
	printf(" ");
	cgi_print_button("Refresh", ACTION_OVERVIEW_MESSAGES, "");
	printf("<pre class=\"auto\">\n%s", cgi_str_get(cat_output));
	printf("</pre>\n");
	cgi_print_button("Back", ACTION_OVERVIEW, "");
	printf(" ");
	cgi_print_button("Refresh", ACTION_OVERVIEW_MESSAGES, "");
	printf("</div>\n");

	cgi_str_free(cat_output);
}
#endif

# ifdef FIREWALL

void firewall_config_add_rule_from_line(cgiStrPairLL **rules_list, char *line)
{
	char *first_part, *second_part;
	cgiStrPairList *str_pair_list = NULL;

	while (line && line[0] == '<') {

		first_part = ++line;
		line = strchr(line, ',');
		if (!line) {
			break;
		}
		*line = '\0';
		second_part = line + 1;
		line = strchr(second_part, '>');
		if (!line) {
			break;
		}
		*line = '\0';
		cgi_str_pair_list_add(&str_pair_list, first_part, second_part);

		line++;

	}

	if (str_pair_list) {
		cgi_str_pair_ll_add(rules_list, str_pair_list);
	}

}

firewallConfig *firewall_config_load(char *config_path)
{
	int ret;
	cgiStr *config_str;
	firewallConfig *firewall_config;
	char *line_end, *line_start, *temp;
	char *name_temp;

	config_str = cgi_str_new(NULL);
	ret = cgi_read_file(&config_str, config_path);
	if (ret < 0) {
		cgi_str_free(config_str);
		return NULL;
	}
	if (!config_str->str[0]) {
		cgi_str_free(config_str);
		return NULL;
	}
	firewall_config = cgi_calloc(sizeof(firewallConfig), 1);

	line_start = config_str->str;

	/* each line */
	while ((line_end = strchr(line_start, '\n')) != NULL) {
		line_end[0] = '\0';

		/* only if its data */
		if (line_start[0] != ' ' && line_start[0] != '#') {

			/* a pre line */
			if (strncmp(line_start, FIREWALL_CONF_ELE_PRE, strlen(FIREWALL_CONF_ELE_PRE)) == 0) {

				temp = line_start + strlen(FIREWALL_CONF_ELE_PRE);
				if (temp) {
					cgi_str_list_add(&firewall_config->pre, temp);
				}
			/* an interface match line */
			} else if (strncmp(line_start, FIREWALL_CONF_ELE_IFACE_MATCH, strlen(FIREWALL_CONF_ELE_IFACE_MATCH)) == 0) {

				temp = line_start + strlen(FIREWALL_CONF_ELE_IFACE_MATCH);
				if (temp) {
					cgi_str_list_add(&firewall_config->iface_matches, temp);
				}

			/* an option line */
			} else if (strncmp(line_start, FIREWALL_CONF_ELE_OPTION, strlen(FIREWALL_CONF_ELE_OPTION)) == 0) {

				temp = line_start + strlen(FIREWALL_CONF_ELE_OPTION);
				name_temp = temp;
				temp = strchr(temp, ':');
				if (temp && (temp + 1)) {
					*temp++ = '\0';
					cgi_str_pair_list_add(&firewall_config->options, name_temp, temp);
				}
			/* a policy line */
			} else if (strncmp(line_start, FIREWALL_CONF_ELE_POLICY, strlen(FIREWALL_CONF_ELE_POLICY)) == 0) {

				temp = line_start + strlen(FIREWALL_CONF_ELE_POLICY);
				name_temp = temp;
				temp = strchr(temp, ':');
				if (temp && (temp + 1)) {
					*temp++ = '\0';
					cgi_str_pair_list_add(&firewall_config->policies, name_temp, temp);
				}

			/* a default rule line */
			} else if (strncmp(line_start, FIREWALL_CONF_ELE_DEFAULT_RULE, strlen(FIREWALL_CONF_ELE_DEFAULT_RULE)) == 0) {

				temp = line_start + strlen(FIREWALL_CONF_ELE_DEFAULT_RULE);
				name_temp = temp;
				temp = strchr(temp, ':');
				if (temp && (temp + 1)) {
					*temp++ = '\0';
					if (strcmp(name_temp, "INPUT") == 0) {
						firewall_config_add_rule_from_line(&firewall_config->def_rules_in, temp);
					} else if (strcmp(name_temp, "OUTPUT") == 0) {
						firewall_config_add_rule_from_line(&firewall_config->def_rules_out, temp);
					} else if (strcmp(name_temp, "FORWARD") == 0) {
						firewall_config_add_rule_from_line(&firewall_config->def_rules_forwd, temp);
					}
				}

			/* a user rule line */
			} else if (strncmp(line_start, FIREWALL_CONF_ELE_USER_RULE, strlen(FIREWALL_CONF_ELE_USER_RULE)) == 0) {

				temp = line_start + strlen(FIREWALL_CONF_ELE_USER_RULE);
				name_temp = temp;
				temp = strchr(temp, ':');
				if (temp && (temp + 1)) {
					*temp++ = '\0';
					if (strcmp(name_temp, "INPUT") == 0) {
						firewall_config_add_rule_from_line(&firewall_config->user_rules_in, temp);
					} else if (strcmp(name_temp, "OUTPUT") == 0) {
						firewall_config_add_rule_from_line(&firewall_config->user_rules_out, temp);
					} else if (strcmp(name_temp, "FORWARD") == 0) {
						firewall_config_add_rule_from_line(&firewall_config->user_rules_forwd, temp);
					}
				}

			}

		}
		line_start = ++line_end;
		if (!line_start) {
			break;
		}

	}

	cgi_str_free(config_str);
	return firewall_config;
}

firewallConfig *firewall_config_load_auto(void)
{
	if (cgi_file_exists(FIREWALL_TEMP_CONF_PATH)) {
		return firewall_config_load(FIREWALL_TEMP_CONF_PATH);
	} else {
		return firewall_config_load(FIREWALL_CONF_PATH);
	}
}

void firewall_config_free(firewallConfig* firewall_config)
{
	cgi_str_pair_list_free(firewall_config->options);
	cgi_str_list_free(firewall_config->pre);
	cgi_str_pair_list_free(firewall_config->policies);
	cgi_str_pair_ll_free(firewall_config->def_rules_in);
	cgi_str_pair_ll_free(firewall_config->user_rules_in);
	cgi_str_pair_ll_free(firewall_config->def_rules_out);
	cgi_str_pair_ll_free(firewall_config->user_rules_out);
	cgi_str_pair_ll_free(firewall_config->def_rules_forwd);
	cgi_str_pair_ll_free(firewall_config->user_rules_forwd);
	cgi_str_list_free(firewall_config->iface_matches);
	free(firewall_config);
}

static void firewall_config_print_rule(FILE *fp, cgiStrPairLL *rules, char *chain)
{
	cgiStrPairList *temp_pair_list;
	cgiStrPairLL *temp_pair_ll;

	if (!rules) {
		return;
	}

	temp_pair_ll = rules;
	while (temp_pair_ll) {

		fprintf(fp, "$IPT -A %s ", chain);

		temp_pair_list = temp_pair_ll->str_pair_list;
		while (temp_pair_list) {

			if (strcmp(temp_pair_list->name, "port-sel-typ") != 0) {
				fprintf(fp, "%s %s ", temp_pair_list->name, temp_pair_list->val);
			}

			temp_pair_list = temp_pair_list->next_list;

		}
		fprintf(fp, "\n");
		temp_pair_ll = temp_pair_ll->next_list;

	}
}

void firewall_config_print_script(FILE *fp, firewallConfig *firewall_config)
{
	cgiStrPairList *temp_pair_list;
	cgiStrList *temp_str_list;
	int print_in_rules = 0, print_out_rules = 0, print_forwd_rules = 0;

	fprintf(fp, "#\n");
	fprintf(fp, "# iptables firewall script (filter table)\n");
	fprintf(fp, "# Produced by at-cgi\n");
	fprintf(fp, "#\n\n");

	fprintf(fp, "# Options\n\n");
	temp_pair_list = firewall_config->options;
	while (temp_pair_list) {
		fprintf(fp, "%s=%s\n", temp_pair_list->name, temp_pair_list->val);
		temp_pair_list = temp_pair_list->next_list;
	}
	fprintf(fp, "\n");

	fprintf(fp, "# Pre-rule configuration\n\n");
	temp_str_list = firewall_config->pre;
	while (temp_str_list) {
		fprintf(fp, "%s\n", temp_str_list->str);
		temp_str_list = temp_str_list->next_list;
	}
	fprintf(fp, "\n");

	fprintf(fp, "# Filter table policies\n\n");
	temp_pair_list = firewall_config->policies;
	while (temp_pair_list) {
		int print_rules = strcmp(temp_pair_list->val, "DROP") == 0 ? 1 : 0;
		if (strcmp(temp_pair_list->name, "INPUT") == 0) {
			print_in_rules = print_rules;
		} else if (strcmp(temp_pair_list->name, "OUTPUT") == 0) {
			print_out_rules = print_rules;
		} else if (strcmp(temp_pair_list->name, "FORWARD") == 0) {
			print_forwd_rules = print_rules;
		}
		fprintf(fp, "$IPT -P %s %s\n", temp_pair_list->name, temp_pair_list->val);
		temp_pair_list = temp_pair_list->next_list;
	}
	fprintf(fp, "\n");

	if (print_in_rules) {
		fprintf(fp, "# Input chain rules - default\n\n");
		firewall_config_print_rule(fp, firewall_config->def_rules_in, "INPUT");
		fprintf(fp, "\n");
		fprintf(fp, "# Input chain rules - user set\n\n");
		firewall_config_print_rule(fp, firewall_config->user_rules_in, "INPUT");
		fprintf(fp, "\n");
	}

	if (print_out_rules) {
		fprintf(fp, "# Output chain rules - default\n\n");
		firewall_config_print_rule(fp, firewall_config->def_rules_out, "OUTPUT");
		fprintf(fp, "\n");
		fprintf(fp, "# Output chain rules - user set\n\n");
		firewall_config_print_rule(fp, firewall_config->user_rules_out, "OUTPUT");
		fprintf(fp, "\n");
	}

	if (print_forwd_rules) {
		fprintf(fp, "# Forward chain rules - default\n\n");
		firewall_config_print_rule(fp, firewall_config->def_rules_forwd, "FORWARD");
		fprintf(fp, "\n");
		fprintf(fp, "# Forward chain rules - user set\n\n");
		firewall_config_print_rule(fp, firewall_config->user_rules_forwd, "FORWARD");
		fprintf(fp, "\n");
	}
}

static void firewall_print_grey_out_js(void)
{
	printf("<script type=\"text/javascript\">\n");
	printf("function grey_out_id(id)\n");
	printf("{\n");
	/* printf("document.getElementById(id).style.color = 'grey';\n"); */
	printf("}\n");
	printf("function black_out_id(id)\n");
	printf("{\n");
	/* printf("document.getElementById(id).style.color = 'black';\n"); */
	printf("}\n");
	printf("</script>\n");
}

static void firewall_display_rule_config_screen_js(void)
{
	firewall_print_grey_out_js();
	printf("<script type=\"text/javascript\">\n");

	printf("function set_service_sub_state() {\n");
	printf("if (document.%s.%s[0].checked == true) {\n", FORM_NAME, ELE_ID_FIREWALL_RULE_SERVICE);
	printf("black_out_id('know_service_area');\n");
	printf("grey_out_id('spec_port_area');\n");
	printf("document.%s.%s.disabled = false;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_KNOWN);
	printf("document.%s.%s.disabled = true;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_PORT);
	printf("} else if (document.%s.%s[1].checked == true) {\n", FORM_NAME, ELE_ID_FIREWALL_RULE_SERVICE);
	printf("grey_out_id('know_service_area');\n");
	printf("black_out_id('spec_port_area');\n");
	printf("document.%s.%s.disabled = true;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_KNOWN);
	printf("document.%s.%s.disabled = false;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_PORT);
	printf("}\n");
	printf("}\n");

	printf("function set_service_state() {\n");
	printf("if (document.%s.%s[0].checked == true || document.%s.%s[1].checked == true) {\n",
			FORM_NAME, ELE_ID_FIREWALL_RULE_PROTOCOL, FORM_NAME, ELE_ID_FIREWALL_RULE_PROTOCOL);
	printf("black_out_id('service_area');\n");
	printf("set_service_sub_state()\n");
	printf("document.%s.%s[0].disabled = false;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_SERVICE);
	printf("document.%s.%s[1].disabled = false;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_SERVICE);
	printf("} else {\n");
	printf("grey_out_id('know_service_area');\n");
	printf("grey_out_id('spec_port_area');\n");
	printf("document.%s.%s[0].disabled = true;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_SERVICE);
	printf("document.%s.%s[1].disabled = true;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_SERVICE);
	printf("document.%s.%s.disabled = true;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_KNOWN);
	printf("document.%s.%s.disabled = true;\n", FORM_NAME, ELE_ID_FIREWALL_RULE_PORT);
	printf("}\n");
	printf("}\n");

	printf("function set_rule_state() {\n");
	printf("set_service_sub_state()\n");
	printf("set_service_state()\n");
	printf("}\n");

	printf("window.onload = set_rule_state()\n;");

	printf("</script>\n");
}

static cgiStrList *get_well_know_services(void)
{
	struct servent *service;
	cgiStrList *found_services = NULL;

	while ((service = getservent())) {
		cgi_str_list_add_unique_ordered(&found_services, service->s_name);
	}

	endservent();

	return found_services;
}

void firewall_display_rule_config_screen(char *chain, char *ok_action, cgiStrPairList *rule_list,
		int rule_number, cgiStrList *iface_matches, char *err1)
{
	int tcp_select = 1, udp_select = 0, icmp_select = 0;
	int well_know_select = 1, port_select = 0;
	char *service = "", *port = "";
	char *in_iface = "all", *out_iface = "all";
	char *temp_val, *temp_val2;
	char print_buf[64];
	cgiStrList *well_know_services, *interfaces;
	char user_error_msg[] = "An error occured while obtaining firewall info";

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu(ACTION_FIREWALL);

	well_know_services = get_well_know_services();
	if (!well_know_services) {
		report_error(user_error_msg, "failed to load well known services");
		return;
	}

	interfaces = interface_list_get(iface_matches);
	if (!interfaces) {
		report_error(user_error_msg, "failed to get interface list");
		cgi_str_list_free(well_know_services);
		return;
	}
	/* Add standard option */
	cgi_str_list_add_to_start(&interfaces, "Any");

	if (rule_list) {

		temp_val = cgi_str_pair_list_get_val(rule_list, "-p");
		if (strcasecmp(temp_val, "udp") == 0) {
			udp_select = 1;
			tcp_select = 0;
		} else if (strcasecmp(temp_val, "icmp") == 0) {
			icmp_select = 1;
			tcp_select = 0;
		}
		temp_val = cgi_str_pair_list_get_val(rule_list, "--dport");
		temp_val2 = cgi_str_pair_list_get_val(rule_list, "port-sel-typ");
		if  (temp_val) {
			if (strcmp(temp_val2, FIREWALL_CONF_LINE_WELL_KNOWN) == 0) {
				service = temp_val;
			} else {
				port_select = 1;
				well_know_select = 0;
				port = temp_val;
			}
		}
		temp_val = cgi_str_pair_list_get_val(rule_list, "-i");
		if  (temp_val) {
			in_iface = temp_val;
		}

		temp_val = cgi_str_pair_list_get_val(rule_list, "-o");
		if  (temp_val) {
			out_iface = temp_val;
		}

	}

	cgi_print_hidden_field(ELE_ID_FIREWALL_RULE_CHAIN, chain);
	sprintf(print_buf, "%d", rule_number);
	cgi_print_hidden_field(ELE_ID_FIREWALL_RULE_RULE_NUMBER, print_buf);

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Traffic to allow through firewall</h3>\n");

	printf("<h4>Protocol</h4>\n");

	printf("<p><label>");
	cgi_print_radio_button(ELE_ID_FIREWALL_RULE_PROTOCOL, ELE_ID_FIREWALL_RULE_TCP, tcp_select, "set_service_state()");
	printf(" TCP</label> <label>");
	cgi_print_radio_button(ELE_ID_FIREWALL_RULE_PROTOCOL, ELE_ID_FIREWALL_RULE_UDP, udp_select, "set_service_state()");
	printf(" UDP</label> <label>");
	cgi_print_radio_button(ELE_ID_FIREWALL_RULE_PROTOCOL, ELE_ID_FIREWALL_RULE_ICMP, icmp_select, "set_service_state()");
	printf(" ICMP</label></p>");

	printf("<div id=\"service_area\">\n");

	printf("<h4>Service</h4>\n");

	printf("<p><label>");
	cgi_print_radio_button(ELE_ID_FIREWALL_RULE_SERVICE, ELE_ID_FIREWALL_RULE_SERVICE_KNOWN, well_know_select, "set_service_sub_state()");
	printf(" <span id=\"know_service_area\">Well known service: </span></label>");
	cgi_print_select_box_from_str_list(ELE_ID_FIREWALL_RULE_KNOWN, 1, well_know_services, service, "");
	printf("</p>");
	printf("<p><label>");
	cgi_print_radio_button(ELE_ID_FIREWALL_RULE_SERVICE, ELE_ID_FIREWALL_RULE_SERVICE_PORT, port_select, "set_service_sub_state()");
	printf(" <span id=\"spec_port_area\">Specific port: </span></label>");
	cgi_print_text_box(ELE_ID_FIREWALL_RULE_PORT, port, 20, 20);
	printf("<span class=\"small\">use : to specify port ranges</span></p>");

	printf("</div>\n");

	printf("<!-- fix for ie one text box only submit problem -->\n");
	printf("<input type=\"text\" style=\"visibility:hidden; display:none;\" />\n");

	printf("<h4>Interfaces</h4>\n");

	printf("<p>");
	if (strcmp(chain, "IN") == 0) {
		printf("In: ");
		cgi_print_select_box_from_str_list(ELE_ID_FIREWALL_RULE_IFACE_IN, 1, interfaces, in_iface, "");
	} else if (strcmp(chain, "OUT") == 0) {
		printf(" Out: ");
		cgi_print_select_box_from_str_list(ELE_ID_FIREWALL_RULE_IFACE_OUT, 1, interfaces, out_iface, "");
	} else {
		printf("In: ");
		cgi_print_select_box_from_str_list(ELE_ID_FIREWALL_RULE_IFACE_IN, 1, interfaces, in_iface, "");
		printf(" Out: ");
		cgi_print_select_box_from_str_list(ELE_ID_FIREWALL_RULE_IFACE_OUT, 1, interfaces, out_iface, "");
	}
	printf("</p>");

	printf("<p class=\"inline_error\">%s</p>\n", err1);

	printf("<hr />\n");

	cgi_print_button("OK", ok_action, "");
	printf(" ");
	cgi_print_button("Cancel", ACTION_FIREWALL_RULE_CANCEL, "");

	printf("</div>\n");

	firewall_display_rule_config_screen_js();

	cgi_str_list_free(interfaces);
	cgi_str_list_free(well_know_services);
}

static void firewall_config_print_rule_rows(cgiStrPairLL *rules, int def_rule, char *label)
{
	cgiStrPairLL *temp_pair_ll;
	char *val;
	int row = 0;
	char print_buf[128];

	temp_pair_ll = rules;
	while (temp_pair_ll) {

		printf("<tr><td width=\"15px\">");
		if (!def_rule) {
			snprintf(print_buf, sizeof(print_buf), "%s%d", label, row);
			cgi_print_checkbox(print_buf, 0, "");
			row++;
		}
		printf("</td><td>");
		val = cgi_str_pair_list_get_val(temp_pair_ll->str_pair_list, "--dport");
		if (val) {
			printf("%s", cgi_str_toupper(val));
		}
		val = cgi_str_pair_list_get_val(temp_pair_ll->str_pair_list, "-p");
		if (val) {
			printf(" %s", cgi_str_toupper(val));
		}
		val = cgi_str_pair_list_get_val(temp_pair_ll->str_pair_list, "-i");
		if (val) {
			printf(", In interface: %s", val);
		}
		val = cgi_str_pair_list_get_val(temp_pair_ll->str_pair_list, "-o");
		if (val) {
			printf(", Out interface: %s", val);
		}
		printf("</td></tr>\n");

		temp_pair_ll = temp_pair_ll->next_list;

	}
}

static void firewall_print_config_screen_js(void)
{
	firewall_print_grey_out_js();

	printf("<script type=\"text/javascript\">\n");

	printf("function set_traffic_sel_state_update() {\n");
	printf("if (document.%s.%s[0].checked == true) {\n", FORM_NAME, ELE_ID_FIREWALL_IN_POLICY);
	printf("grey_out_id('%s');\n", ELE_ID_FIREWALL_IN_OK_SERVICES);
	printf("}\n");
	printf("if (document.%s.%s[0].checked == true) {\n", FORM_NAME, ELE_ID_FIREWALL_OUT_POLICY);
	printf("grey_out_id('%s');\n", ELE_ID_FIREWALL_OUT_OK_SERVICES);
	printf("}\n");
	printf("if (document.%s.%s[0].checked == true) {\n", FORM_NAME, ELE_ID_FIREWALL_FORWD_POLICY);
	printf("grey_out_id('%s');\n", ELE_ID_FIREWALL_FORWD_OK_SERVICES);
	printf("}\n");
	printf("}\n");

	printf("window.onload = set_traffic_sel_state_update;\n");

	printf("</script>\n");
}


void firewall_display_config_screen(char *top_message)
{
	char print_buf[128];
	char *val;
	int selected;
	char user_error_msg[] = "An error occured while obtaining firewall info";
	firewallConfig *firewall_config;

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu(ACTION_FIREWALL);

	firewall_config = firewall_config_load_auto();
	if (!firewall_config) {
		report_error(user_error_msg, "failed to load firewall config");
		return;
	}

	if (top_message) {
		print_top_info_message(top_message);
	} else if (cgi_file_exists(FIREWALL_TEMP_CONF_PATH)) {
		print_top_info_message("Editing...");
	}

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Incoming Traffic</h3>\n");

	val = cgi_str_pair_list_get_val(firewall_config->policies, "INPUT");
	selected = (strcmp(val, "ACCEPT") == 0) ? 1 : 0;
	printf("<p><label>");
	snprintf(print_buf, sizeof(print_buf), "grey_out_id('%s')", ELE_ID_FIREWALL_IN_OK_SERVICES);
	cgi_print_radio_button(ELE_ID_FIREWALL_IN_POLICY, ELE_ID_FIREWALL_IN_POLICY_ACCEPT, selected, print_buf);
	printf(" <strong>Allow all traffic</strong></label></p>\n");

	selected = (strcmp(val, "DROP") == 0) ? 1 : 0;
	printf("<p><label>");
	snprintf(print_buf, sizeof(print_buf), "black_out_id('%s')", ELE_ID_FIREWALL_IN_OK_SERVICES);
	cgi_print_radio_button(ELE_ID_FIREWALL_IN_POLICY, ELE_ID_FIREWALL_IN_POLICY_DROP, selected, print_buf);
	printf(" <strong>Allow only the following traffic</strong></label></p>\n");

	printf("<div id=\"%s\">\n", ELE_ID_FIREWALL_IN_OK_SERVICES);

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" class=\"firewall_services\">\n");
	firewall_config_print_rule_rows(firewall_config->def_rules_in, 1, NULL);
	firewall_config_print_rule_rows(firewall_config->user_rules_in, 0, FIREWALL_ROW_IN);
	printf("</table>\n");

	printf("<div class=\"firewall_services\">\n");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Delete selected</a> \n",
		ACTION_FIREWALL_IN_DEL);
	printf("<span style=\"padding: 0 5px 0 5px;\"></span>");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Edit selected</a> \n",
		ACTION_FIREWALL_IN_EDIT);
	printf("<span style=\"padding: 0 10px 0 10px;\"></span>");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Add new</a> \n",
		ACTION_FIREWALL_IN_ADD);
	printf("</div>\n");

	printf("</div>\n");

	printf("<div class=\"h_gap_small\"></div>\n");
	printf("<hr />\n");

	printf("<h3>Outgoing Traffic</h3>\n");

	val = cgi_str_pair_list_get_val(firewall_config->policies, "OUTPUT");
	selected = (strcmp(val, "ACCEPT") == 0) ? 1 : 0;
	printf("<p><label>");
	snprintf(print_buf, sizeof(print_buf), "grey_out_id('%s')", ELE_ID_FIREWALL_OUT_OK_SERVICES);
	cgi_print_radio_button(ELE_ID_FIREWALL_OUT_POLICY, ELE_ID_FIREWALL_OUT_POLICY_ACCEPT, selected, print_buf);
	printf(" <strong>Allow all traffic</strong></label></p>\n");

	selected = (strcmp(val, "DROP") == 0) ? 1 : 0;
	printf("<p><label>");
	snprintf(print_buf, sizeof(print_buf), "black_out_id('%s')", ELE_ID_FIREWALL_OUT_OK_SERVICES);
	cgi_print_radio_button(ELE_ID_FIREWALL_OUT_POLICY, ELE_ID_FIREWALL_OUT_POLICY_DROP, selected, print_buf);
	printf(" <strong>Allow only the following traffic</strong></label></p>\n");

	printf("<div id=\"%s\">\n", ELE_ID_FIREWALL_OUT_OK_SERVICES);

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" class=\"firewall_services\">\n");
	firewall_config_print_rule_rows(firewall_config->def_rules_out, 1, NULL);
	firewall_config_print_rule_rows(firewall_config->user_rules_out, 0, FIREWALL_ROW_OUT);
	printf("</table>\n");

	printf("<div class=\"firewall_services\">\n");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Delete selected</a> \n",
		ACTION_FIREWALL_OUT_DEL);
	printf("<span style=\"padding: 0 5px 0 5px;\"></span>");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Edit selected</a> \n",
		ACTION_FIREWALL_OUT_EDIT);
	printf("<span style=\"padding: 0 10px 0 10px;\"></span>");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Add new</a> \n",
		ACTION_FIREWALL_OUT_ADD);
	printf("</div>\n");

	printf("</div>\n");

	printf("<div class=\"h_gap_small\"></div>\n");
	printf("<hr />\n");

	printf("<h3>Forwarded Traffic</h3>\n");

	val = cgi_str_pair_list_get_val(firewall_config->options, "IP_FORWARD");
	selected = (strcmp(val, "yes") == 0) ? 1 : 0;
	printf("<p>");
	cgi_print_checkbox(ELE_ID_FIREWALL_IP_FORWD, selected, "");
	printf(" Forward IP traffic</p>");

	val = cgi_str_pair_list_get_val(firewall_config->policies, "FORWARD");
	selected = (strcmp(val, "ACCEPT") == 0) ? 1 : 0;
	printf("<p><label>");
	snprintf(print_buf, sizeof(print_buf), "grey_out_id('%s')", ELE_ID_FIREWALL_FORWD_OK_SERVICES);
	cgi_print_radio_button(ELE_ID_FIREWALL_FORWD_POLICY, ELE_ID_FIREWALL_FORWD_POLICY_ACCEPT, selected, print_buf);
	printf(" <strong>Allow all traffic</strong></label></p>\n");

	selected = (strcmp(val, "DROP") == 0) ? 1 : 0;
	printf("<p><label>");
	snprintf(print_buf, sizeof(print_buf), "black_out_id('%s')", ELE_ID_FIREWALL_FORWD_OK_SERVICES);
	cgi_print_radio_button(ELE_ID_FIREWALL_FORWD_POLICY, ELE_ID_FIREWALL_FORWD_POLICY_DROP, selected, print_buf);
	printf(" <strong>Allow only the following traffic</strong></label></p>\n");

	printf("<div id=\"%s\">\n", ELE_ID_FIREWALL_FORWD_OK_SERVICES);

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" class=\"firewall_services\">\n");
	firewall_config_print_rule_rows(firewall_config->def_rules_forwd, 1, NULL);
	firewall_config_print_rule_rows(firewall_config->user_rules_forwd, 0, FIREWALL_ROW_FORWD);
	printf("</table>\n");

	printf("<div class=\"firewall_services\">\n");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Delete selected</a> \n",
		ACTION_FIREWALL_FORWD_DEL);
	printf("<span style=\"padding: 0 5px 0 5px;\"></span>");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Edit selected</a> \n",
		ACTION_FIREWALL_FORWD_EDIT);
	printf("<span style=\"padding: 0 10px 0 10px;\"></span>");
	printf("<a href=\"\" onclick=\"linkregevent('%s'); return false\">Add new</a> \n",
		ACTION_FIREWALL_FORWD_ADD);
	printf("</div>\n");

	printf("</div>\n");

	printf("<div class=\"h_gap_small\"></div>\n");
	printf("<hr />\n");

	cgi_print_button("Update", ACTION_FIREWALL_UPDATE, "");
	printf(" ");
	cgi_print_button("Cancel", ACTION_FIREWALL_CANCEL, "");

	printf("<div class=\"h_gap_small\"></div>\n");
	printf("<hr />\n");

	printf("<span class=\"small\"><a href=\"\" onclick=\"linkregevent('%s'); return false\">Show firewall script</a></span>\n",
		ACTION_FIREWALL_SHOW_SCRIPT);

	printf("</div>\n");

	firewall_config_free(firewall_config);

	firewall_print_config_screen_js();
}

void firewall_display_script(void)
{
	firewallConfig *firewall_config;
	char user_error_msg[] = "An error occured while obtaining firewall info";

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu(ACTION_FIREWALL);

	firewall_config = firewall_config_load_auto();
	if (!firewall_config) {
		report_error(user_error_msg, "failed to load firewall config");
		return;
	}

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Firewall Script</h3>\n");

	printf("<pre>\n");
	firewall_config_print_script(stdout, firewall_config);
	printf("</pre>\n");

	printf("<hr />\n");

	cgi_print_button("Back", ACTION_FIREWALL, "");

	printf("</div>\n");

	firewall_config_free(firewall_config);
}

# endif

# ifdef BRIDGE

static void print_bridge_settings_js(cgiStrList *iface_matches, int iface_count)
{
	cgiStrList *iface_list_next = iface_matches;

	printf("<script type=\"text/javascript\">\n");

	printf("function enable_ele(textareaname) {\n");
	printf("document.%s.elements[textareaname].disabled = false;\n", FORM_NAME);
	printf("}\n");

	printf("function disable_ele(textareaname) {\n");
	printf("document.%s.elements[textareaname].disabled = true;\n", FORM_NAME);
	printf("}\n");

	printf("function update_textbox_state(checkboxname, textareaname) {\n");
	printf("if (document.%s.elements[checkboxname].checked == true) {\n", FORM_NAME);
	printf("disable_ele(textareaname);\n");
	printf("} else {\n");
	printf("enable_ele(textareaname);\n");
	printf("}");
	printf("}\n");

	printf("function update_port_stp_select_state(checked, id)\n");
	printf("{\n");
	printf("if (checked == false) {\n");
	printf("disable_ele('%s' + id + '%s');\n", ELE_ID_PORT_USE_BASE, ELE_ID_PRIO_AUTO_TAIL);
	printf("disable_ele('%s' + id + '%s');\n", ELE_ID_PORT_USE_BASE, ELE_ID_PRIO_TEXT_TAIL);
	printf("disable_ele('%s' + id + '%s');\n", ELE_ID_PORT_USE_BASE, ELE_ID_COST_AUTO_TAIL);
	printf("disable_ele('%s' + id + '%s');\n", ELE_ID_PORT_USE_BASE, ELE_ID_COST_TEXT_TAIL);
	printf("} else {\n");
	printf("enable_ele('%s' + id + '%s');\n", ELE_ID_PORT_USE_BASE, ELE_ID_PRIO_AUTO_TAIL);
	printf("enable_ele('%s' + id + '%s');\n", ELE_ID_PORT_USE_BASE, ELE_ID_COST_AUTO_TAIL);
	printf("update_textbox_state('%s' + id + '%s', '%s' + id + '%s');\n",
		 ELE_ID_PORT_USE_BASE, ELE_ID_PRIO_AUTO_TAIL, ELE_ID_PORT_USE_BASE, ELE_ID_PRIO_TEXT_TAIL);
	printf("update_textbox_state('%s' + id + '%s', '%s' + id + '%s');\n",
		 ELE_ID_PORT_USE_BASE, ELE_ID_COST_AUTO_TAIL, ELE_ID_PORT_USE_BASE, ELE_ID_COST_TEXT_TAIL);
	printf("}\n");
	printf("}\n");

	printf("function update_all_ports_stp_select_state(){\n");
	while (iface_list_next) {
		if (iface_count != 2) {
			printf("update_port_stp_select_state(document.%s.elements['port_%s'].checked, '%s');\n",
				FORM_NAME, iface_list_next->str, iface_list_next->str);
		} else {
			printf("update_port_stp_select_state(true, '%s');\n", iface_list_next->str);
		}
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("}\n");

	printf("function update_macageing_state() {\n");
	printf("update_textbox_state('%s', '%s');\n", ELE_ID_MAC_AGEING_AUTO, ELE_ID_MAC_AGEING_TEXT);
	printf("}\n");

	printf("function update_bridge_prio_state() {\n");
	printf("update_textbox_state('%s', '%s');\n", ELE_ID_BRIDGE_PRIO_AUTO, ELE_ID_BRIDGE_PRIO_TEXT);
	printf("}\n");

	printf("function update_forwarding_state() {\n");
	printf("update_textbox_state('%s', '%s');\n", ELE_ID_FORWARDING_AUTO, ELE_ID_FORWARDING_TEXT);
	printf("}\n");

	printf("function update_hellotime_state() {\n");
	printf("update_textbox_state('%s', '%s');\n", ELE_ID_HELLO_TIME_AUTO, ELE_ID_HELLO_TIME_TEXT);
	printf("}\n");

	printf("function update_maxage_state() {\n");
	printf("update_textbox_state('%s', '%s');\n", ELE_ID_MAX_AGE_AUTO, ELE_ID_MAX_AGE_TEXT);
	printf("}\n");

	printf("function disable_stp_detail() {\n");
	printf("disable_ele('%s');\n", ELE_ID_BRIDGE_PRIO_AUTO);
	printf("disable_ele('%s');\n", ELE_ID_BRIDGE_PRIO_TEXT);
	printf("disable_ele('%s');\n", ELE_ID_FORWARDING_AUTO);
	printf("disable_ele('%s');\n", ELE_ID_FORWARDING_TEXT);
	printf("disable_ele('%s');\n", ELE_ID_HELLO_TIME_AUTO);
	printf("disable_ele('%s');\n", ELE_ID_HELLO_TIME_TEXT);
	printf("disable_ele('%s');\n", ELE_ID_MAX_AGE_AUTO);
	printf("disable_ele('%s');\n", ELE_ID_MAX_AGE_TEXT);
	iface_list_next = iface_matches;
	while (iface_list_next) {
		printf("disable_ele('%s%s%s');\n", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_AUTO_TAIL);
		printf("disable_ele('%s%s%s');\n", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_TEXT_TAIL);
		printf("disable_ele('%s%s%s');\n", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_AUTO_TAIL);
		printf("disable_ele('%s%s%s');\n", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_TEXT_TAIL);
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("}\n");

	printf("function enable_stp_detail() {\n");
	printf("enable_ele('%s');\n", ELE_ID_BRIDGE_PRIO_AUTO);
	printf("enable_ele('%s');\n", ELE_ID_FORWARDING_AUTO);
	printf("enable_ele('%s');\n", ELE_ID_HELLO_TIME_AUTO);
	printf("enable_ele('%s');\n", ELE_ID_MAX_AGE_AUTO);
	printf("update_all_ports_stp_select_state();");
	printf("}\n");

	printf("function update_stp_state() {\n");
	printf("if (document.%s.%s.checked == false) {\n", FORM_NAME, ELE_ID_ENABLE_STP);
	printf("disable_stp_detail();\n");
	printf("} else {\n");
	printf("enable_stp_detail();\n");
	printf("update_bridge_prio_state();\n");
	printf("update_forwarding_state();\n");
	printf("update_hellotime_state();\n");
	printf("update_maxage_state();\n");
	iface_list_next = iface_matches;
	while (iface_list_next) {
		printf("update_textbox_state('%s%s%s', '%s%s%s');\n",
			ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_AUTO_TAIL,
			ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_TEXT_TAIL);
		printf("update_textbox_state('%s%s%s', '%s%s%s');\n",
			ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_AUTO_TAIL,
			ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_TEXT_TAIL);
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("}\n");
	printf("}\n");

	printf("function update_interfaces_state() {\n");
	printf("if (document.%s.%s.checked == false) {\n", FORM_NAME, ELE_ID_ACTIVE_BRIDGE);
	iface_list_next = iface_matches;
	while (iface_list_next) {
		printf("disable_ele('%s%s');\n", ELE_ID_PORT_USE_BASE, iface_list_next->str);
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("} else {\n");
	iface_list_next = iface_matches;
	while (iface_list_next) {
		printf("enable_ele('%s%s');\n", ELE_ID_PORT_USE_BASE, iface_list_next->str);
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("}");
	printf("}\n");

	printf("function set_overall_bridge_state(){\n");
	printf("update_interfaces_state();\n");
	printf("if (document.%s.%s.checked == false) {\n", FORM_NAME, ELE_ID_ACTIVE_BRIDGE);
	printf("disable_ele('%s');\n", ELE_ID_ENABLE_STP);
	printf("disable_ele('%s');\n", ELE_ID_MAC_AGEING_AUTO);
	printf("disable_ele('%s');\n", ELE_ID_MAC_AGEING_TEXT);
	printf("disable_stp_detail();\n");
	printf("} else {\n");
	printf("enable_ele('%s');\n", ELE_ID_ENABLE_STP);
	printf("enable_ele('%s');\n", ELE_ID_MAC_AGEING_AUTO);
	printf("enable_ele('%s');\n", ELE_ID_MAC_AGEING_TEXT);
	printf("update_macageing_state();\n");
	printf("update_all_ports_stp_select_state();\n");
	printf("update_stp_state();\n");
	printf("}\n");
	printf("}\n");

	printf("window.onload = set_overall_bridge_state;\n");

	printf("</script>\n");
}

static void bridge_config_default2blank(char *value)
{
	if (strcmp(value, "DEFAULT") == 0) {
		value[0] = '\0';
	}
}

int bridge_config_is_entry_default(const char *bridge_config_entry)
{
	if (strcmp(bridge_config_entry, BR_DEFAULT) == 0) {
		return 1;
	} else {
		return 0;
	}
}

void display_bridge_settings(bridgeConfig *bridge_config, char *err1, char *err2)
{
	int macageing_chk, br_prio_chk, forward_chk, chk, iface_count = 0;
	int hello_chk, max_age_chk;
	char user_error_msg[] = "An error occured when aquiring bridge settings";
	char print_buf[128], print_buf2[128], print_buf3[128];
	cgiStrList *iface_list, *iface_list_next;

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu(ACTION_BRIDGE_STATUS);

	if (!bridge_config) {
		bridge_config = bridge_config_load();
		if (!bridge_config) {
			report_error(user_error_msg, "failed to load bridge info");
			return;
		}
	}

	iface_list = interface_list_get(bridge_config->iface_matches);
	iface_list_next = iface_list;
	while (iface_list_next) {
		iface_count++;
		iface_list_next = cgi_str_list_next(iface_list_next);
	}

	macageing_chk = 	bridge_config_is_entry_default(bridge_config->ageing);
	br_prio_chk = 		bridge_config_is_entry_default(bridge_config->br_prio);
	forward_chk = 		bridge_config_is_entry_default(bridge_config->forward);
	hello_chk = 		bridge_config_is_entry_default(bridge_config->hello);
	max_age_chk = 		bridge_config_is_entry_default(bridge_config->max_age);

	bridge_config_default2blank(bridge_config->ageing);
	bridge_config_default2blank(bridge_config->br_prio);
	bridge_config_default2blank(bridge_config->forward);
	bridge_config_default2blank(bridge_config->hello);
	bridge_config_default2blank(bridge_config->max_age);

	printf("<div class=\"contents_frame\">\n");

	printf("<h3>Basic Configuration</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">Activate Bridge</td><td>");
	cgi_print_checkbox(ELE_ID_ACTIVE_BRIDGE, bridge_config->create_br, "set_overall_bridge_state()");
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Interfaces to bridge</td><td><table><tr>");
	iface_list_next = iface_list;
	while (iface_list_next) {
		printf("<td>%s ", iface_list_next->str);
		if (iface_count == 2) {
			snprintf(print_buf, sizeof(print_buf), "%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str);
			cgi_print_hidden_field(print_buf, "1");
		} else {
			int chk = cgi_str_list_exists(bridge_config->bridged_ifaces, iface_list_next->str);
			snprintf(print_buf, sizeof(print_buf), "update_port_stp_select_state(this.checked, '%s')", iface_list_next->str);
			snprintf(print_buf2, sizeof(print_buf2), "%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str);
			cgi_print_checkbox(print_buf2, chk, print_buf);
		}
		printf("</td>");
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("</tr></table></td></tr>\n");

	printf("</table>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\"></td>");
	printf("<td width=\"8%%\"><span class=\"small\"> Auto</span></td>\n");
	printf("<td align=\"left\"><span class=\"small\">Defined</span></td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">MAC table ageing time</td><td>");
	cgi_print_checkbox(ELE_ID_MAC_AGEING_AUTO, macageing_chk, "update_macageing_state()");
	printf("</td><td>");
	cgi_print_text_box(ELE_ID_MAC_AGEING_TEXT, bridge_config->ageing, 10, 10);
	printf(" <span class=\"small\">seconds</span></td></tr>\n");

	printf("</table>\n");

	printf("<p class=\"inline_error\">%s</p>\n", err1);

	printf("<hr />\n");
	printf("<h3>STP Configuration</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">Enable STP</td><td>");
	cgi_print_checkbox(ELE_ID_ENABLE_STP, bridge_config->stp_on, "update_stp_state()");
	printf("</td></tr>\n");

	printf("</table>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\"></td>");
	printf("<td width=\"8%%\"><span class=\"small\"> Auto</span></td>\n");
	printf("<td align=\"left\"><span class=\"small\">Defined</span></td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Bridge Priority</td><td>");
	cgi_print_checkbox(ELE_ID_BRIDGE_PRIO_AUTO, br_prio_chk, "update_bridge_prio_state()");
	printf("</td><td>");
	cgi_print_text_box(ELE_ID_BRIDGE_PRIO_TEXT, bridge_config->br_prio, 10, 10);
	printf(" <span class=\"small\">(0 ~ 65,535)</span></td></td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Forwarding Delay</td><td>");
	cgi_print_checkbox(ELE_ID_FORWARDING_AUTO, forward_chk, "update_forwarding_state()");
	printf("</td><td>");
	cgi_print_text_box(ELE_ID_FORWARDING_TEXT, bridge_config->forward, 10, 10);
	printf(" <span class=\"small\">seconds (4 ~ 30)</span></td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Hello time</td><td>");
	cgi_print_checkbox(ELE_ID_HELLO_TIME_AUTO, hello_chk, "update_hellotime_state()");
	printf("</td><td>");
	cgi_print_text_box(ELE_ID_HELLO_TIME_TEXT, bridge_config->hello, 10, 10);
	printf(" <span class=\"small\">seconds (1 ~ 10)</span></td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Max message age</td><td>");
	cgi_print_checkbox(ELE_ID_MAX_AGE_AUTO, max_age_chk, "update_maxage_state()");
	printf("</td><td>");
	cgi_print_text_box(ELE_ID_MAX_AGE_TEXT, bridge_config->max_age, 10, 10);
	printf(" <span class=\"small\">seconds (6 ~ 40)</span></td></tr>\n");

	printf("</table>\n");

	iface_list_next = iface_list;

	while (iface_list_next) {

		char *text_val = cgi_str_pair_list_get_val(bridge_config->port_prios, iface_list_next->str);
		if (text_val) {
			chk = bridge_config_is_entry_default(text_val);
			bridge_config_default2blank(text_val);
		} else {
			chk = 0;
			text_val = "";
		}

		snprintf(print_buf, sizeof(print_buf), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_AUTO_TAIL);
		snprintf(print_buf2, sizeof(print_buf2), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_PRIO_TEXT_TAIL);
		snprintf(print_buf3, sizeof(print_buf3), "update_textbox_state('%s', '%s')", print_buf, print_buf2);
		printf("<div id=\"%s%s\">\n", ELE_ID_PORT_USE_BASE, iface_list_next->str);
		printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");
		printf("<tr class=\"new\"><td class=\"left\">%s port priority</td><td width=\"8%%\">", iface_list_next->str);
		cgi_print_checkbox(print_buf, chk, print_buf3);
		printf("</td><td>");
		cgi_print_text_box(print_buf2, text_val, 10, 10);
		printf(" <span class=\"small\">(0 ~ 63)</span></td></tr>\n");

		text_val = cgi_str_pair_list_get_val(bridge_config->port_costs, iface_list_next->str);
		if (text_val) {
			chk = bridge_config_is_entry_default(text_val);
			bridge_config_default2blank(text_val);
		} else {
			chk = 0;
			text_val = "";
		}

		snprintf(print_buf, sizeof(print_buf), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_AUTO_TAIL);
		snprintf(print_buf2, sizeof(print_buf2), "%s%s%s", ELE_ID_PORT_USE_BASE, iface_list_next->str, ELE_ID_COST_TEXT_TAIL);
		snprintf(print_buf3, sizeof(print_buf3), "update_textbox_state('%s', '%s')", print_buf, print_buf2);
		printf("<tr class=\"new\"><td class=\"left\">%s path cost</td><td width=\"8%%\">", iface_list_next->str);
		cgi_print_checkbox(print_buf, chk, print_buf3);
		printf("</td><td>");
		cgi_print_text_box(print_buf2, text_val, 10, 10);
		printf(" <span class=\"small\">(1 ~ 65,535)</span></td></tr>\n");
		printf("</table>\n");
		printf("</div>\n");

		iface_list_next = cgi_str_list_next(iface_list_next);

	}

	printf("<p class=\"inline_error\">%s</p>\n", err2);

	printf("<hr />\n");

	printf("<p>ブリッジ設定を変更した場合、ただちに設定保存とシステム再起動がおこなわれます。</p>");

	cgi_print_button_confirm("Update", ACTION_BRIDGE_SAVE, "",
		"新しいブリッジ設定を保存し、システムを再起動します。よろしいですか?");
	printf(" ");
	cgi_print_button("Cancel", ACTION_BRIDGE_CANCEL, "");

	printf("</div>\n");

	print_bridge_settings_js(iface_list, iface_count);

	cgi_str_list_free(iface_list);

	bridge_config_free(bridge_config);
}

void display_bridge_state(void)
{
	int ret, stp;
	cgiStr *cgi_str;
	char *bridge_id, *stp_enabled, *temp;
	char *show_args[] 		= {BRCTL_PATH, "show", BRIDGE_NAME, NULL};
	char *showmacs_args[] 	= {BRCTL_PATH, "showmacs", BRIDGE_NAME, NULL};
	char *showstp_args[] 	= {BRCTL_PATH, "showstp", BRIDGE_NAME, NULL};
	char user_error_msg[] 	= "An error occured when obtaining bridge information";
	bridgeConfig *bridge_config;
	cgiStrList *iface_list_next;

	bridge_config = bridge_config_load();
	if (!bridge_config) {
		report_error(user_error_msg, "Failed to load bridge config");
		return;
	}

	cgi_str = cgi_str_new(NULL);

	ret = cgi_read_command(&cgi_str, show_args[0], show_args);
	if (ret < 0) {
		report_error(user_error_msg, "failed to read brctl show output");
		cgi_str_free(cgi_str);
		return;
	}

	print_frame_top(SYSTEM_CGI_PATH);
	display_sub_menu(ACTION_BRIDGE_STATUS);

	printf("<div class=\"contents_frame\">\n");

	if (strstr(cgi_str_get(cgi_str), BRIDGE_NAME) == NULL) {
		printf("<p align=\"center\" class=\"big_message\">Bridge is not active</p>\n");
		printf("<p align=\"center\">");
		cgi_print_button("Configure", ACTION_BRIDGE_CONFIG, "");
		printf("</p></div>\n");
		return;
	}

	bridge_id = strstr(cgi_str_get(cgi_str), BRIDGE_NAME) + strlen(BRIDGE_NAME);
	while (*bridge_id == '\t') {
		bridge_id++;
	}
	stp_enabled = bridge_id;
	while (*stp_enabled != '\t') {
		stp_enabled++;
	}
	*stp_enabled++ = '\0';
	temp = stp_enabled;
	while (*temp != '\t') {
		temp++;
	}
	*temp = '\0';

	if (strcmp(stp_enabled, "yes") == 0) {
		stp = 1;
	} else {
		stp = 0;
	}

	printf("<h3>Bridge Overview</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\"	width=\"100%%\" class=\"details\">\n");

	printf("<tr class=\"new\"><td class=\"left\">Bridge name</td><td>");
	printf(BRIDGE_NAME);
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Bridge ID</td><td>");
	printf(bridge_id);
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">STP enabled</td><td>");
	printf(stp_enabled);
	printf("</td></tr>\n");

	printf("<tr class=\"new\"><td class=\"left\">Interfaces</td><td>");
	iface_list_next = bridge_config->bridged_ifaces;
	while (iface_list_next) {
		printf("%s ", iface_list_next->str);
		iface_list_next = cgi_str_list_next(iface_list_next);
	}
	printf("</td></tr>\n");

	printf("</table>\n");

	cgi_str_reset(&cgi_str);
	ret = cgi_read_command(&cgi_str, showmacs_args[0], showmacs_args);
	if (ret < 0) {
		report_error(user_error_msg, "failed to read brctl showmacs output");
		cgi_str_free(cgi_str);
		return;
	}

	cgi_print_button("Configure", ACTION_BRIDGE_CONFIG, "");

	printf("<hr />\n");

	printf("<h3>Stored MAC Addresses</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"60%%\" class=\"small_out\">\n");

	printf("<tr><td><strong>Port</strong></td><td><strong>MAC Address</strong></td>");
	printf("<td><strong>Local Address?</strong></td><td><strong>Ageing Timer</strong></td></tr>\n");

	cgi_print_row_from_tab_line(cgi_str_get(cgi_str), 2);

	printf("</table>\n");
	printf("<div class=\"h_gap_small\"></div>\n");

	printf("<hr />\n");

	printf("<h3>STP Details</h3>\n");

	if (stp) {
		cgi_str_reset(&cgi_str);
		ret = cgi_read_command(&cgi_str, showstp_args[0], showstp_args);
		if (ret < 0) {
			report_error(user_error_msg, "failed to read brctl showstp output");
			cgi_str_free(cgi_str);
			return;
		}
		printf("<pre>%s</pre>\n", cgi_str_get(cgi_str));
	} else {
		printf("<p>STP not enabled</p>\n");
	}

	printf("<hr />\n");
	printf("</div>\n");

	bridge_config_free(bridge_config);
	cgi_str_free(cgi_str);
}
/*
void display_bridge_config_change(void)
{
	int ret;
	networkInfo network_info;
	char *hostname_args[] = {HOSTNAME_PATH, NULL};
	char user_error_msg[] = "An error occured when changing system settings";

	ret = get_current_network_info(config_get_primary_if(), &network_info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get current network info");
		return;
	}

	printf("<div class=\"h_gap_big\"></div>");

	printf("<div align=\"center\" width=\"80%%\" style=\"padding: 0 30px 0 30px;\">\
		<img src=\"%s\" alt=\"Admin\" class=\"title_img\"/>",
		TITLE_IMG);

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p class=\"update_title\">ブリッジ設定が変更されました<br />システムを再起動してください</p>\n");

	printf("<div class=\"h_gap_big\"></div>");

	printf("<p align=\"center\">\
	ネットワーク接続を切断します。<br />\
	%sを再起動し、WEBブラウザ画面を閉じ、", config_get_product_name());

	if (strcmp(config_get_product_name(), PRODUCT_NAME_A9) == 0)
		printf("しばらく待ってから");
	else
		printf("%sの赤LEDが消灯するまで待ってから", config_get_product_name());

	printf(<br />);
	printf("Bonjourなどを利用してトップページにアクセスし直してください。");

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
	print_ifconfig_part(config_get_primary_if(), "HWaddr ");
	printf("</p>");

	printf("</div>");
	printf("<div class=\"h_gap_big\"></div>");
}
*/
# endif

