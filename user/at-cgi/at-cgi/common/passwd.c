/*
 * at-cgi-core/passwd.c - passwd checking for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "core.h"

#define USER_ENTRY_LEN_MAX	(256)

int check_current_password(const char *entered_passwd)
{
	char *ret;
	char *start;
	FILE *fp;
	char buffer[USER_ENTRY_LEN_MAX];

	fp = fopen(HTPASSWD_FILE_NAME, "r");
	if (fp == NULL) {
		return -1;
	}

	ret = fgets(buffer, sizeof(buffer), fp);
	if (ret == NULL) {
		return -1;
	}
	fclose(fp);
	buffer[strlen(buffer)-1] = '\0';
	start = strstr(buffer, ":") + 1;

	if (strcmp(crypt(entered_passwd, start), start) == 0) {
		return 1;
	} else {
		return 0;
	}
}
