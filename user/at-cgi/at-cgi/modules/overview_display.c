/*
 * modules/overview_display.c - system module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>
#include <string.h>

#include <simple-cgi-app-io.h>

#include <misc-utils.h>
#include <core.h>
#include <menu.h>
#include <frame-html.h>
#include <system-config.h>
#include <common.h>

#include "system_display.h"
#include "packetscan.h"

#ifdef USBDATA
static void display_usbdata_overview(void)
{
	printf("<div class=\"standard_function_box\">\n");
	printf("<p><a href=\"%s\" class=\"big_link\">Show USB Data</a></p>", USBDATA_CGI_PATH);
	printf("</div>\n");
}
#endif

#ifdef PACKETSCAN
static int display_packetscan_overview(void)
{
	packetscanConfig *packetscan_config;

	packetscan_config = packetscan_config_load();
	if (!packetscan_config) {
		return -1;
	}

	printf("<div class=\"standard_function_box\">\n");
	printf("<h3>Packet Scan</h3>\n");

	if (!packetscan_config->packetscan_on) {

		printf("<p><strong>Packet scanning is not active</strong></p>\n");

	} else {

		printf("<p><strong>Packet scanning is active with the following detection types:</strong> ");

		printf("<ul>");
		/*
		if (packetscan_config->winny_scan_on) {
			printf("<li>Winny usage detection</li>");	
		}
		*/
		if (packetscan_config->port_scan_on) {
			printf("<li>Portscan detection</li>");	
		}
		if (packetscan_config->content_scan_on) {
			printf("<li>Content detection</li>");	
		}
		printf("</ul>");	
	}

	printf("</div>\n");
	
	packetscan_config_free(packetscan_config);
	
	return 0;
}
#endif

void display_overview(void)
{
	int ret;
	char *hostname_args[] = {HOSTNAME_PATH, NULL};
	char *kernel_ver_args[] = {UNAME_PATH, "-r", NULL};
	char user_error_msg[] = "An error occured when obtaining system info";
	networkInfo network_info;

	print_content_type();
	print_html_head(INDEX_CGI_PATH);

	ret = get_current_network_info(config_get_primary_if(), &network_info);
	if (ret < 0) {
		report_error(user_error_msg, "Failed to get current network info");
		return;
	}

#ifndef INDEX_ONLY

	print_frame_top(INDEX_CGI_PATH);

#else

	printf("<div class=\"h_gap_med\"></div>\n");
	printf("<div align=\"center\">\n");
	printf("<img src=\"%s\" alt=\"Admin\" class=\"title_img\"/>\n", TITLE_IMG);
	printf("</div>\n");

#endif

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\" width=\"80%%\" class=\"overview_frame\">\n");
	printf("<tr><td>\n");

#ifdef USBDATA
	display_usbdata_overview();
#endif

#ifdef PACKETSCAN
	ret = display_packetscan_overview();
	if (ret < 0) {
		report_error(user_error_msg, "Failed to display packet scan overview");
		return;
	}
#endif

	printf("<div class=\"h_gap_small\"></div>\n");

	printf("<div class=\"system_box\">");
	printf("<h4>Network</h4>\n<p><strong>IP Address: </strong>");

	print_current_effective_ip_address(config_get_primary_if());

	printf(" (");
	if (network_info.use_dhcp) {
		printf("auto");
	} else {
		printf("static");
	}
	printf(")</p>\n<p><strong>MAC Address: </strong>");
	print_ifconfig_part(config_get_primary_if(), "HWaddr ");
	printf("</p>\n<p><strong>Host name: </strong>");
	cgi_print_command(hostname_args[0], hostname_args);
	printf("</p>\n</div>\n");

	printf("<div class=\"system_box\">\n");
	printf("<h4>Uptime</h4><p>\n");
	print_uptime();
	printf("</p></div>\n");

	printf("<div class=\"system_box\">\n");
	printf("<h4>Firmware</h4>\n");
	printf("<p><strong>Dist: </strong>");
	cgi_print_file(DISTNAME_PATH);
	printf("</p>\n<p><strong>Kernel: </strong>");
	cgi_print_command(kernel_ver_args[0], kernel_ver_args);
	printf("</p></div>\n");

	printf("</td></tr></table>\n");

	print_frame_bottom();
	print_html_tail();
}
