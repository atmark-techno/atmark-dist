/*
 * modules/packetscan_display.h - packet scan module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef PACKETSCAN_H_
#define PACKETSCAN_H_

#include <simple-cgi-app-str.h>

typedef struct {
	int packetscan_on;
	/* int winny_scan_on; */
	int port_scan_on;
	int content_scan_on;
	cgiStr *content_scan_content;
	cgiStr *alert_recipient;
	cgiStr *alert_smtp_host;
	cgiStr *alert_send_freq_max;
} packetscanConfig;

#define ACTION_CONFIG				"packetscan_config"
#define ACTION_CONFIG_UPDATE		"packetscan_config_update"

#define ELE_ID_PACKETSCAN_ON		"packetscan_on"
/* #define ELE_ID_WINNY_SCAN_ON		"winny_scan_on" */
#define ELE_ID_PORT_SCAN_ON			"port_scan_on"
#define ELE_ID_CONTENT_SCAN_ON		"content_scan_on"
#define ELE_ID_CONTENT_SCAN_CONTENT	"content_scan_content"
#define ELE_ID_ALERT_RECIPIENT		"alert_recipient"
#define ELE_ID_ALERT_SMTP_HOST		"alert_smtp_host"
#define ELE_ID_ALERT_SEND_FREQ_MAX	"alert_send_freq_max"

#define PACKETSCAN_CONFIG_PATH			CONFIG_PATH "/at-admin-packetscan"

extern void display_packetscan_config(packetscanConfig *packetscan_config,
		char *top_message, char *inline_err1, char *inline_err2);

extern packetscanConfig *packetscan_config_load(void);
extern void packetscan_config_free(packetscanConfig *packetscan_config);

#endif /*PACKETSCAN_H_*/
