/*
 * modules/packetscan_main.c - packet scan module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <simple-cgi.h>

#include <simple-cgi-app-alloc.h>
#include <simple-cgi-app-html-parts.h>
#include <simple-cgi-app-misc.h>
#include <simple-cgi-app-io.h>

#include <frame-html.h>
#include <system-config.h>
#include <common.h>
#include <misc-utils.h>
#include <core.h>
#include <menu.h>

#include "packetscan.h"

#define CONTENTSCAN_RULE_PATH 	CONFIG_PATH "/snort/local.rules.1"
/*#define WINNY_RULE_PATH 		CONFIG_PATH "/snort/local.rules.2"*/
#define PORTSCAN_DEF_PATH	 	CONFIG_PATH "/snort/sfportscan.conf"

#define SEND_MAIL_ALERT_PATH	"/etc/snort/send-alert-email"

static int packetscan_config_save(packetscanConfig *packetscan_config)
{
	int ret, temp_fd;
	FILE *temp_fp;
	char template[] = "packetscan_conf_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, "cp", template, PACKETSCAN_CONFIG_PATH, NULL};

	if (!packetscan_config) {
		return -1;
	}

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		return -1;
	}

	fprintf(temp_fp, "#! /bin/sh\n\n");
	fprintf(temp_fp, "PACKETSCAN_ON=\"%d\"\n\n", packetscan_config->packetscan_on);
	/*fprintf(temp_fp, "WINNY_SCAN_ON=\"%d\"\n", packetscan_config->winny_scan_on);*/
	fprintf(temp_fp, "PORT_SCAN_ON=\"%d\"\n", packetscan_config->port_scan_on);
	fprintf(temp_fp, "CONTENT_SCAN_ON=\"%d\"\n", packetscan_config->content_scan_on);
	fprintf(temp_fp, "CONTENT_SCAN_CONTENT=\"%s\"\n", cgi_str_get(packetscan_config->content_scan_content));
	fprintf(temp_fp, "ALERT_RECIPIENT=\"%s\"\n", cgi_str_get(packetscan_config->alert_recipient));
	fprintf(temp_fp, "ALERT_SMTP_HOST=\"%s\"\n", cgi_str_get(packetscan_config->alert_smtp_host));
	fprintf(temp_fp, "ALERT_SEND_FREQ_MAX=\"%s\"\n\n", cgi_str_get(packetscan_config->alert_send_freq_max));

	fclose(temp_fp);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
		return -1;
	}
	unlink(template);

	return 0;
}

static void parse_user_input(packetscanConfig *packetscan_config)
{
	char *query_val;

	if (!packetscan_config) {
		return;
	}

	query_val = cgi_query_get_val(ELE_ID_PACKETSCAN_ON);
	if (cgi_is_null_or_blank(query_val)) {
		packetscan_config->packetscan_on = 0;
		return;
	} else {
		packetscan_config->packetscan_on = 1;
	}

	/*
	query_val = cgi_query_get_val(ELE_ID_WINNY_SCAN_ON);
	if (cgi_is_null_or_blank(query_val)) {
		packetscan_config->winny_scan_on = 0;
	} else {
		packetscan_config->winny_scan_on = 1;
	}
	*/

	query_val = cgi_query_get_val(ELE_ID_PORT_SCAN_ON);
	if (cgi_is_null_or_blank(query_val)) {
		packetscan_config->port_scan_on = 0;
	} else {
		packetscan_config->port_scan_on = 1;
	}

	query_val = cgi_query_get_val(ELE_ID_CONTENT_SCAN_ON);
	if (cgi_is_null_or_blank(query_val)) {
		packetscan_config->content_scan_on = 0;
	} else {
		packetscan_config->content_scan_on = 1;
		query_val = cgi_query_get_val(ELE_ID_CONTENT_SCAN_CONTENT);
		cgi_str_reset(&packetscan_config->content_scan_content);
		if (!cgi_is_null_or_blank(query_val)) {
			cgi_chop_line_end(query_val);
			cgi_str_add(&packetscan_config->content_scan_content, query_val);
		}
	}

	query_val = cgi_query_get_val(ELE_ID_ALERT_RECIPIENT);
	cgi_str_reset(&packetscan_config->alert_recipient);
	if (!cgi_is_null_or_blank(query_val)) {
		cgi_chop_line_end(query_val);
		cgi_str_add(&packetscan_config->alert_recipient, query_val);
	}

	query_val = cgi_query_get_val(ELE_ID_ALERT_SMTP_HOST);
	cgi_str_reset(&packetscan_config->alert_smtp_host);
	if (!cgi_is_null_or_blank(query_val)) {
		cgi_chop_line_end(query_val);
		cgi_str_add(&packetscan_config->alert_smtp_host, query_val);
	}

	query_val = cgi_query_get_val(ELE_ID_ALERT_SEND_FREQ_MAX);
	cgi_str_reset(&packetscan_config->alert_send_freq_max);
	if (!cgi_is_null_or_blank(query_val)) {
		cgi_str_add(&packetscan_config->alert_send_freq_max, query_val);
	}
}

static int validate_user_input(packetscanConfig *packetscan_config)
{
	int i;

	if (!packetscan_config) {
		return -1;
	}

	if (!packetscan_config->packetscan_on) {
		return 0;
	}

	if (/*!packetscan_config->winny_scan_on && */!packetscan_config->port_scan_on &&
			!packetscan_config->content_scan_on) {
		display_packetscan_config(packetscan_config, NULL, "Please select at least one detection type", "");
		return -1;
	}

	if (packetscan_config->content_scan_on) {

		if (cgi_is_null_or_blank(cgi_str_get(packetscan_config->content_scan_content))) {
			display_packetscan_config(packetscan_config, NULL, "Please enter a word or phrase for the content detection", "");
			return -1;
		}

		if (!cgi_is_ascii(packetscan_config->content_scan_content->str)) {
			display_packetscan_config(packetscan_config, NULL, "Invalid characters in content detection", "");
			return -1;
		}

		for (i = 0; cgi_str_get(packetscan_config->content_scan_content)[i]; i++) {
			if (packetscan_config->content_scan_content->str[i] == ':' ||
					packetscan_config->content_scan_content->str[i] == ';' ||
					packetscan_config->content_scan_content->str[i] == '\\' ||
					packetscan_config->content_scan_content->str[i] == '"') {
				display_packetscan_config(packetscan_config, NULL, "The characters : ; \\ \" cannot be used in content detection", "");
				return -1;
			}
		}

	}

	if (cgi_is_null_or_blank(cgi_str_get(packetscan_config->alert_recipient))) {
		display_packetscan_config(packetscan_config, NULL, "", "Please enter an email address to send alerts to");
		return -1;
	}
	if (!strchr(packetscan_config->alert_recipient->str, '@') ||
			!strchr(packetscan_config->alert_recipient->str, '.') ||
			!is_hostname(strchr(packetscan_config->alert_recipient->str, '@') + 1) ||
			packetscan_config->alert_recipient->str_len < 5) {
			display_packetscan_config(packetscan_config, NULL, "", "Email address is not valid");
			return -1;
	}

	if (cgi_is_null_or_blank(cgi_str_get(packetscan_config->alert_smtp_host))) {
		display_packetscan_config(packetscan_config, NULL, "", "Please enter a SMTP server");
		return -1;
	}
	if (!is_hostname(cgi_str_get(packetscan_config->alert_smtp_host))) {
		display_packetscan_config(packetscan_config, NULL, "", "SMTP server address is not valid");
		return -1;
	}

	return 0;
}

static int update_snort_content_scan_rule(packetscanConfig *packetscan_config)
{
	int ret, temp_fd;
	FILE *temp_fp;
	char template[] = "packetscan_content_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, "cp", template, CONTENTSCAN_RULE_PATH, NULL};

	if (!packetscan_config) {
		return -1;
	}

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		return -1;
	}

	fprintf(temp_fp, "#Content scan rule\n\n");
	if (!packetscan_config->content_scan_on) {
		fprintf(temp_fp, "#");
	}
	fprintf(temp_fp, "alert tcp any any -> any any (content: \"%s\"; nocase; sid: 1000001; rev: 1; msg: \"content scan hit\"; classtype: string-detect;)\n\n",
			cgi_str_get(packetscan_config->content_scan_content));

	fclose(temp_fp);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
		return -1;
	}
	unlink(template);

	return 0;
}

static int de_comment_file(char *path, int comment)
{
	int ret, temp_fd;
	FILE *temp_fp, *target_fp;
	char template[] = "packetscan_comment_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, "cp", template, path, NULL};
	char buffer[1024];

	if (!path) {
		return -1;
	}

	target_fp = fopen(path, "r");
	if (target_fp == NULL) {
		return -1;
	}

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		fclose(target_fp);
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), target_fp) != NULL) {

		if (comment) {
			fprintf(temp_fp, "#%s", buffer);
		} else {
			if (buffer[0] == '#') {
				fprintf(temp_fp, "%s", buffer+1);
			} else {
				fprintf(temp_fp, "%s", buffer);
			}
		}

	}

	fclose(temp_fp);
	fclose(target_fp);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
		return -1;
	}
	unlink(template);

	return 0;
}

int update_root_crontab_snort_entry(char *mins)
{
	int ret;
	char buffer[256];
	char *crontab_args[] = {SUDO_PATH, CRONTAB_PATH, CRONTAB_CONFIG_PATH, NULL};

	if (atoi(mins) < 1 || atoi(mins) > 60) {
		return -1;
	}

	sprintf(buffer, "*/%s * * * * " SEND_MAIL_ALERT_PATH "\n", mins);
	ret = sudo_string_to_file(CRONTAB_CONFIG_PATH, buffer);
	if (ret < 0) {
		return -1;
	}

	ret = cgi_exec(crontab_args[0], crontab_args);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

static int update_snort_config(packetscanConfig *packetscan_config_old,
		packetscanConfig *packetscan_config_new)
{
	int ret;

	if (!packetscan_config_old || !packetscan_config_new) {
		return -1;
	}

	ret = update_snort_content_scan_rule(packetscan_config_new);
	if (ret < 0) {
		return -1;
	}
	/*
	if (packetscan_config_new->winny_scan_on != packetscan_config_old->winny_scan_on) {
		if (packetscan_config_new->winny_scan_on) {
			ret = de_comment_file(WINNY_RULE_PATH, 0);
		} else {
			ret = de_comment_file(WINNY_RULE_PATH, 1);
		}
		if (ret <  0) {
			return -1;
		}
	}*/

	if (packetscan_config_new->port_scan_on != packetscan_config_old->port_scan_on) {
		if (packetscan_config_new->port_scan_on) {
			ret = de_comment_file(PORTSCAN_DEF_PATH, 0);
		} else {
			ret = de_comment_file(PORTSCAN_DEF_PATH, 1);
		}
		if (ret <  0) {
			return -1;
		}
	}

	if (packetscan_config_new->alert_send_freq_max != packetscan_config_old->alert_send_freq_max) {
		ret = update_root_crontab_snort_entry(cgi_str_get(packetscan_config_new->alert_send_freq_max));
		if (ret < 0) {
			return -1;
		}
	}

	return 0;
}

static int update_snort(packetscanConfig *packetscan_config_old,
		packetscanConfig *packetscan_config_new)
{
	int ret;
	char *stop_snort_args[] = {SUDO_PATH, SNORT_INIT_PATH, "stop", config_get_primary_if(), NULL};
	char *start_snort_args[] = {SUDO_PATH, SNORT_INIT_PATH, "start", config_get_primary_if(), NULL};

	if (!packetscan_config_old || !packetscan_config_new) {
		return -1;
	}

	if (packetscan_config_old->packetscan_on) {
		ret = cgi_exec(stop_snort_args[0], stop_snort_args);
		if (ret < 0) {
			syslog(LOG_ERR, "at-cgi: failed to stop snort");
		}
	}

	ret = update_snort_config(packetscan_config_old, packetscan_config_new);
	if (ret < 0) {
		return -1;
	}

	if (packetscan_config_new->packetscan_on) {
		ret = cgi_exec(start_snort_args[0], start_snort_args);
		if (ret < 0) {
			return -1;
		}
	}

	return 0;
}

int send_confirmation_email(packetscanConfig *packetscan_config)
{
	pid_t pid;
	int pipe_fds_stdin[2];
	int status, ret;
	char *hostname_args[] = {HOSTNAME_PATH,  NULL};
	cgiStr *hostname;
	FILE *mail_stdin;

	if (!packetscan_config) {
		return -1;
	}

	char subject[] = "AT-Admin packet scan configuration updated";
	char *mail_args[] = {SUDO_PATH, MAIL_PATH, "-L", "-S", "", "-H", "", "-s", subject, "", NULL};

	hostname = cgi_str_new(NULL);
	ret = cgi_read_command(&hostname, hostname_args[0], hostname_args);
	if (ret < 0) {
		cgi_str_free(hostname);
		return -1;
	}
	if (cgi_str_get(hostname)[hostname->str_len-1] == '\n') {
		cgi_str_get(hostname)[hostname->str_len-1] = '\0';
	}

	mail_args[4] = cgi_str_get(packetscan_config->alert_smtp_host);
	mail_args[6] = cgi_str_get(hostname);
	mail_args[9] = cgi_str_get(packetscan_config->alert_recipient);

	if (pipe(pipe_fds_stdin) != 0) {
		return -1;
	}

	pid = fork();

	if (pid == 0) {

		close(0);
		dup2(pipe_fds_stdin[0], 0);
		close(pipe_fds_stdin[0]);
		close(pipe_fds_stdin[1]);

		ret = execv(mail_args[0], mail_args);
		_exit(1);

	} else if (pid < 0) {

		return -1;

	}

	close(pipe_fds_stdin[0]);
	cgi_str_free(hostname);

	mail_stdin = fdopen(pipe_fds_stdin[1], "w");

	fprintf(mail_stdin, "AT-Admin packet scan configuration has been updated ");
	fprintf(mail_stdin, "to the following settings.\n\n");
	fprintf(mail_stdin, "Packet scanning: %s\n\n", packetscan_config->packetscan_on ? "enabled" : "disabled");
	fprintf(mail_stdin, "Detection Types\n");
	fprintf(mail_stdin, "---------------\n\n");
	/* fprintf(mail_stdin, "Winny usage detection: %s\n", packetscan_config->winny_scan_on ? "enabled" : "disabled"); */
	fprintf(mail_stdin, "Portscan detection: %s\n", packetscan_config->port_scan_on ? "enabled" : "disabled");
	if (packetscan_config->content_scan_on) {
		fprintf(mail_stdin, "Content detection: enabled (%s)\n\n", cgi_str_get(packetscan_config->content_scan_content));
	} else {
		fprintf(mail_stdin, "Content detection: disabled\n\n");
	}
	fprintf(mail_stdin, "Alerts\n");
	fprintf(mail_stdin, "------\n\n");
	/* fprintf(mail_stdin, "Email address: %s\n", cgi_str_get(packetscan_config->alert_recipient)); */
	/* fprintf(mail_stdin, "SMTP Server: %s\n", cgi_str_get(packetscan_config->alert_smtp_host)); */
	fprintf(mail_stdin, "Max alert frequency: every %s minute", cgi_str_get(packetscan_config->alert_send_freq_max));
	fprintf(mail_stdin, "%s", strcmp(cgi_str_get(packetscan_config->alert_send_freq_max), "1") == 0 ? "\n" : "s\n");

	fclose(mail_stdin);

	ret = waitpid(pid, &status, 0);
	if (ret < 0) {
		return -1;
	}

	if (!WIFEXITED(status)) {
		return -1;
	} else if (WEXITSTATUS(status)) {
		return -1;
	}

	return 0;
}

static void handle_packetscan_config_update(void)
{
	int ret;
	packetscanConfig *packetscan_config_new, *packetscan_config_old;
	char user_error_msg[] = "An error occured when updating packet scan configuration";

	packetscan_config_old = packetscan_config_load();
	if (!packetscan_config_old) {
		report_error(user_error_msg, "failed to load packet scan configuration");
		return;
	}

	packetscan_config_new = packetscan_config_load();
	if (!packetscan_config_new) {
		packetscan_config_free(packetscan_config_old);
		report_error(user_error_msg, "failed to load packet scan configuration");
		return;
	}

	parse_user_input(packetscan_config_new);

	ret = validate_user_input(packetscan_config_new);
	if (ret < 0) {
		return;
	}

	ret = packetscan_config_save(packetscan_config_new);
	if (ret < 0) {
		packetscan_config_free(packetscan_config_old);
		packetscan_config_free(packetscan_config_new);
		report_error(user_error_msg, "failed to save packet scan configuration");
		return;
	}

	ret = set_flag(UNSAVED_SETTINGS_FLAG_PATH);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to set unsaved settings flag");
	}

	ret = update_snort(packetscan_config_old, packetscan_config_new);
	if (ret < 0) {
		packetscan_config_free(packetscan_config_old);
		packetscan_config_free(packetscan_config_new);
		report_error(user_error_msg, "failed to update snort configuration");
		return;
	}

	if (packetscan_config_new->packetscan_on && 
			!cgi_is_null_or_blank(packetscan_config_new->alert_recipient->str) && 
			!cgi_is_null_or_blank(packetscan_config_new->alert_smtp_host->str)) {
		ret = send_confirmation_email(packetscan_config_new);
		if (ret < 0) {
			syslog(LOG_ERR, "at-cgi: failed to send confirmation email");
			display_packetscan_config(NULL, "Configuration updated but confirmation email could not be sent", "", "");
		} else {
			display_packetscan_config(NULL, "Configuration updated and confirmation email sent", "", "");
		}
	} else {
		display_packetscan_config(NULL, "Configuration updated", "", "");
	}

	packetscan_config_free(packetscan_config_old);
	packetscan_config_free(packetscan_config_new);
}

static void handle_local_request()
{
	char *req_area;
	char user_error_msg[] = "Request could not be processed";

	print_content_type();
	print_html_head(PACKETSCAN_CGI_PATH);

	req_area = cgi_query_get_val("req_area");

	if (!req_area) {
		/* No set req_area means a new get */
		display_packetscan_config(NULL, NULL, "", "");
	} else if (strcmp(req_area, ACTION_CONFIG) == 0) {
		display_packetscan_config(NULL, NULL, "", "");
	} else if (strcmp(req_area, ACTION_CONFIG_UPDATE) == 0) {
		handle_packetscan_config_update();
	} else {
		print_frame_top(PACKETSCAN_CGI_PATH);
		report_error(user_error_msg, "No action match");
	}

	print_frame_bottom();
	print_html_tail();
}

int main(int argc, char *argv[])
{
	int ret;

	at_cgi_debug("packetscan - up and running");

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

	at_cgi_debug("packetscan - finished");

	exit(EXIT_SUCCESS);
}
