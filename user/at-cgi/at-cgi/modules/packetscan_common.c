/*
 * modules/packetscan_common.c - packet scan module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Chris McHarg <chris (at) atmark-techno.com>
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <simple-cgi-app-alloc.h>

#include <core.h>

#include "packetscan.h"

#define PACKETSCAN_CONFIG_LINE_LEN_MAX	1024

void packetscan_config_free(packetscanConfig *packetscan_config)
{
	cgi_str_free(packetscan_config->content_scan_content);
	cgi_str_free(packetscan_config->alert_recipient);
	cgi_str_free(packetscan_config->alert_smtp_host);
	cgi_str_free(packetscan_config->alert_send_freq_max);
	free(packetscan_config);
}

packetscanConfig *packetscan_config_load(void)
{
	FILE *in_fp;
	packetscanConfig *packetscan_config;
	char buffer[PACKETSCAN_CONFIG_LINE_LEN_MAX];
	char *key, *value;
	int packetscan_on_flag = 0, /* winny_scan_on_flag = 0, */ port_scan_on_flag = 0;
	int content_scan_on_flag = 0, content_scan_content_flag = 0;
	int alert_recipient_flag = 0, alert_smtp_host_flag = 0;
	int alert_send_freq_max_flag = 0;

	in_fp = fopen(PACKETSCAN_CONFIG_PATH, "r");
	if (in_fp == NULL) {
		return NULL;
	}

	packetscan_config = cgi_calloc(sizeof(packetscanConfig), 1);

	while (fgets(buffer, sizeof(buffer), in_fp) != NULL) {

		if(buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '='){
			continue;
		}

		if (buffer[strlen(buffer)-2] != '\"') {
			continue;
		}
		buffer[strlen(buffer)-2] = '\0';

		value = strstr(buffer, "=\"");
		if (value == NULL) {
			continue;
		}
		*value = '\0';
		value += 2;
		key = buffer;

		if (strcmp(key, "PACKETSCAN_ON") == 0) {
			packetscan_config->packetscan_on = atoi(value);
			packetscan_on_flag = 1;
		/*
		} else if (strcmp(key, "WINNY_SCAN_ON") == 0) {
			packetscan_config->winny_scan_on = atoi(value);
			winny_scan_on_flag = 1;
		*/
		} else if (strcmp(key, "PORT_SCAN_ON") == 0) {
			packetscan_config->port_scan_on = atoi(value);
			port_scan_on_flag = 1;
		} else if (strcmp(key, "CONTENT_SCAN_ON") == 0) {
			packetscan_config->content_scan_on = atoi(value);
			content_scan_on_flag = 1;
		} else if (strcmp(key, "CONTENT_SCAN_CONTENT") == 0) {
			packetscan_config->content_scan_content = cgi_str_new(value);
			content_scan_content_flag = 1;
		} else if (strcmp(key, "ALERT_RECIPIENT") == 0) {
			packetscan_config->alert_recipient = cgi_str_new(value);
			alert_recipient_flag = 1;
		} else if (strcmp(key, "ALERT_SMTP_HOST") == 0) {
			packetscan_config->alert_smtp_host = cgi_str_new(value);
			alert_smtp_host_flag = 1;
		} else if (strcmp(key, "ALERT_SEND_FREQ_MAX") == 0) {
			packetscan_config->alert_send_freq_max = cgi_str_new(value);
			alert_send_freq_max_flag = 1;
		}

	}

	fclose(in_fp);

	if (!packetscan_on_flag || /*!winny_scan_on_flag ||*/ !port_scan_on_flag ||
			!content_scan_on_flag || !content_scan_content_flag ||
			!alert_recipient_flag || !alert_smtp_host_flag ||
			!alert_send_freq_max_flag) {
		packetscan_config_free(packetscan_config);
		return NULL;
	}

	return packetscan_config;
}
