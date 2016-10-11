/*
 * modules/packetscan_display.c - packet scan module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <simple-cgi-app-html-parts.h>

#include <frame-html.h>
#include <menu.h>
#include <common.h>

#include "packetscan.h"

static void print_packetscan_config_js(void)
{
	printf("<script type=\"text/javascript\">\n");

	printf("function disable_ele(ele_name) {\n");
	printf("document.%s.elements[ele_name].disabled = true;\n", FORM_NAME);
	printf("}\n");

	printf("function enable_ele(ele_name) {\n");
	printf("document.%s.elements[ele_name].disabled = false;\n", FORM_NAME);
	printf("}\n");

	printf("function set_contentdetect_state() {\n");
	printf("if (document.%s.elements['%s'].checked == false) {\n", FORM_NAME, ELE_ID_CONTENT_SCAN_ON);
	printf("disable_ele('%s');\n", ELE_ID_CONTENT_SCAN_CONTENT);
	printf("} else {\n");
	printf("enable_ele('%s');\n", ELE_ID_CONTENT_SCAN_CONTENT);
	printf("}\n");
	printf("}\n");

	printf("function set_packetscan_state() {\n");

	printf("if (document.%s.elements['%s'].checked == false) {\n", FORM_NAME, ELE_ID_PACKETSCAN_ON);
	/* printf("disable_ele('%s');\n", ELE_ID_WINNY_SCAN_ON); */
	printf("disable_ele('%s');\n", ELE_ID_PORT_SCAN_ON);
	printf("disable_ele('%s');\n", ELE_ID_CONTENT_SCAN_ON);
	printf("disable_ele('%s');\n", ELE_ID_CONTENT_SCAN_CONTENT);
	printf("disable_ele('%s');\n", ELE_ID_ALERT_RECIPIENT);
	printf("disable_ele('%s');\n", ELE_ID_ALERT_SMTP_HOST);
	printf("disable_ele('%s');\n", ELE_ID_ALERT_SEND_FREQ_MAX);
	printf("} else {\n");
	/* printf("enable_ele('%s');\n", ELE_ID_WINNY_SCAN_ON); */
	printf("enable_ele('%s');\n", ELE_ID_PORT_SCAN_ON);
	printf("enable_ele('%s');\n", ELE_ID_CONTENT_SCAN_ON);
	printf("set_contentdetect_state()\n");
	printf("enable_ele('%s');\n", ELE_ID_ALERT_RECIPIENT);
	printf("enable_ele('%s');\n", ELE_ID_ALERT_SMTP_HOST);
	printf("enable_ele('%s');\n", ELE_ID_ALERT_SEND_FREQ_MAX);
	printf("}\n");
	printf("}\n");

	printf("window.onload = set_packetscan_state;\n");

	printf("</script>\n");
}

void display_packetscan_config(packetscanConfig *packetscan_config,
		char *top_message, char *inline_err1, char *inline_err2)
{
	char user_error_msg[] = "An error occured when displaying packet scan settings";
	cgiHtmlOption alert_freq_options[] = {
		{"1 minute", 	"1"},
		{"5 minutes", 	"5"},
		{"15 minutes", 	"15"},
		{"30 minutes", 	"30"},
		{"60 minutes", 	"60"}
	};
	int alert_freq_options_size = sizeof(alert_freq_options)/sizeof(cgiHtmlOption);

	print_frame_top(PACKETSCAN_CGI_PATH);

	if (!packetscan_config) {
		packetscan_config = packetscan_config_load();
		if (!packetscan_config) {
			report_error(user_error_msg, "failed to load packet scan configuration");
			return;
		}
	}

	if (top_message) {
		print_top_info_message(top_message);
	}

	printf("<div class=\"contents_frame\">\n");

	printf("<h2>Packet Scan Configuration</h2>\n");

	printf("<div class=\"h_gap_small\"></div>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");
	printf("<tr><td class=\"left\">Enable packet scanning</td><td>");
	cgi_print_checkbox(ELE_ID_PACKETSCAN_ON, packetscan_config->packetscan_on, "set_packetscan_state()");
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("<div class=\"h_gap_small\"></div>\n");

	printf("<hr />\n");

	printf("<h3>Detection Types</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");
	/* printf("<tr class=\"new\"><td class=\"left\">Winny usage detection</td><td>");
	cgi_print_checkbox(ELE_ID_WINNY_SCAN_ON, packetscan_config->winny_scan_on, "set_packetscan_state()");
	printf("</td></tr>\n"); */
	printf("<tr class=\"new\"><td class=\"left\">Portscan detection</td><td>");
	cgi_print_checkbox(ELE_ID_PORT_SCAN_ON, packetscan_config->port_scan_on, "set_packetscan_state()");
	printf("</td></tr>\n");
	printf("<tr class=\"new\"><td class=\"left\">Content detection</td><td>");
	cgi_print_checkbox(ELE_ID_CONTENT_SCAN_ON, packetscan_config->content_scan_on, "set_contentdetect_state()");
	printf("<span style=\"padding: 0 10px 0 0;\"></span>");
	cgi_print_text_box(ELE_ID_CONTENT_SCAN_CONTENT, cgi_str_get(packetscan_config->content_scan_content), 50, 50);
	printf("</td></tr>\n");
	printf("</table>\n");

	printf("<p class=\"inline_error\">%s</p>\n", inline_err1);

	printf("<hr />\n");

	printf("<h3>Alerts</h3>\n");

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"100%%\" class=\"details\">\n");
	printf("<tr class=\"new\"><td class=\"left\">Email address</td><td>");
	cgi_print_text_box(ELE_ID_ALERT_RECIPIENT, cgi_str_get(packetscan_config->alert_recipient), 40, 128);
	printf("</td></tr>\n");
	printf("<tr class=\"new\"><td class=\"left\">SMTP Server</td><td>");
	cgi_print_text_box(ELE_ID_ALERT_SMTP_HOST, cgi_str_get(packetscan_config->alert_smtp_host), 40, 128);
	printf("</td></tr>\n");
	printf("<tr class=\"new\"><td class=\"left\">Send alert notices every </td><td>");
	cgi_print_select_box(ELE_ID_ALERT_SEND_FREQ_MAX, 1, alert_freq_options, alert_freq_options_size, cgi_str_get(packetscan_config->alert_send_freq_max),"");
	printf("<span style=\"padding: 0 0 0 5px;\"> at max</span></td></tr>\n");
	printf("</table>\n");

	printf("<p class=\"inline_error\">%s</p>\n", inline_err2);

	printf("<hr />\n");

	cgi_print_button("Update", ACTION_CONFIG_UPDATE, "");

	printf("</div>\n");

	print_packetscan_config_js();

	packetscan_config_free(packetscan_config);
}
