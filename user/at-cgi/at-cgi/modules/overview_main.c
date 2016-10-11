/*
 * modules/overview_main.c - system module for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <simple-cgi.h>

#include <misc-utils.h>
#include <system-config.h>
#include <common.h>
#include <core.h>

#include "overview_display.h"

static void handle_local_request()
{
	char *req_area;
	char user_error_msg[] = "Request could not be processed";

	req_area = cgi_query_get_val("req_area");

	if (!req_area) {

		/* No set req_area means a new get */
		display_overview();

	} else if (strcmp(req_area, ACTION_OVERVIEW) == 0) {

		display_overview();

	} else {

		return_crit_error(user_error_msg, "No action match");

	}
}

int main(int argc, char * argv[])
{
	int ret;

	at_cgi_debug("overview - up and running");

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

	at_cgi_debug("overview - finished");

	exit(EXIT_SUCCESS);
}
