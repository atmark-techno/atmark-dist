/*
 * at-cgi/common.c - common, system wide actions and views.
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>

#include <simple-cgi-app-html-parts.h>
#include <simple-cgi-app-str.h>
#include <simple-cgi-app-io.h>

#include "frame-html.h"
#include "core.h"
#include "misc-utils.h"
#include "common.h"

void report_error(const char *user_error_msg, const char *error_details)
{
	at_cgi_debug(user_error_msg);
	at_cgi_debug(error_details);
		
	syslog(LOG_ERR, "at-cgi: %s : %s", user_error_msg, error_details);

	printf("<div class=\"h_gap_big\"></div>");
	printf("<div align=\"center\" class=\"error\"><h3>Error</h3><p>\n");
	printf(user_error_msg);
	printf("</p></div>\n");
	printf("<div class=\"h_gap_verybig\"></div>");	
}

void return_crit_error(const char *user_error_msg, const char *error_details)
{
	at_cgi_debug(user_error_msg);
	at_cgi_debug(error_details);

	syslog(LOG_ERR, "at-cgi: %s : %s", user_error_msg, error_details);

	print_content_type();
	print_html_head("");
	
	printf("<div style=\"margin: 150px 0 0 0\"></div>\n");
	printf("<div align=\"center\" class=\"error\"><p>\n");
	printf(user_error_msg);
	printf("</p></div>\n");
	
	print_html_tail();
}

void print_top_info_message(const char *message)
{
	printf("<div class=\"top_info_message\"><div><span class=\"message\">\n");
	printf(message);
	printf("</span></div></div>\n");
}

int set_flag(char *path)
{
	int ret;
	ret = cgi_dump_to_file(path, FLAG_NOTCHECKED);
	if (ret < 0) {
		return -1;
	}
	return 0;
}
int flag_exists(char *path, const char *state)
{
	int ret;
	cgiStr *flag_contents;

	if (!cgi_file_exists(path)) {
		return 0;
	}

	flag_contents = cgi_str_new(NULL);

	ret = cgi_read_file(&flag_contents, path);
	if (ret < 0) {
		cgi_str_free(flag_contents);
		return 0;
	}
	
	if (strcmp(cgi_str_get(flag_contents), state) == 0) {
		cgi_str_free(flag_contents);
		return 1;	
	}
	
	cgi_str_free(flag_contents);
	return 0;
}

void print_system_messages(void)
{
	printf("<script type=\"text/javascript\">\n \
		function tosystemstate(){\n \
		document.%s.req_area.value = '%s';\n \
		document.%s.action = '%s';\n \
		document.%s.submit();\n \
		}\n \
		</script>\n",
		FORM_NAME, ACTION_SETTINGS_DETAILS,
		FORM_NAME, ACTION_SETTINGS_PATH,
		FORM_NAME);

	if (flag_exists(UNSAVED_SETTINGS_FLAG_PATH, FLAG_NOTCHECKED)) {	
		printf("<div>設定が変更されていますが、まだ保存されていません -> \
			<a href=\"%s\" onclick=\"tosystemstate(); \
			return false\">System : Save &amp; Load</a></div>\n",
			ACTION_SETTINGS_PATH);
	}

	if (flag_exists(UPDATED_SETTINGS_INIT_FLAG_PATH, FLAG_NOTCHECKED)) {	
		printf("<div>設定が初期化されましたが、まだ再起動されていません -> \
			<a href=\"%s\" onclick=\"tosystemstate(); \
			return false\">System : Save &amp; Load</a></div>\n",
			ACTION_SETTINGS_PATH);
	}

	if (flag_exists(UPDATED_FW_FLAG_PATH, FLAG_NOTCHECKED)) {	
		printf("<div>ソフトウェア更新を行いましたが、まだ再起動されていません -> \
			<a href=\"%s\" onclick=\"tosystemstate(); \
			return false\">System : Save &amp; Load</a></div>\n",
			ACTION_SETTINGS_PATH);
	}
}

int update_unsaved_settings_state(char *path)
{
	int ret;
	
	ret = cgi_dump_to_file(path, FLAG_CHECKED);
	if (ret < 0) {
		return -1;
	}
	return 0;
}

int system_wide_handling(void)
{
	/* Do nothing for now */
	
	return LOCAL_HANDLING_REQUIRED;
}

