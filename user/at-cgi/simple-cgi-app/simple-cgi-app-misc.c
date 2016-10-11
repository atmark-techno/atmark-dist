/*
 * simple-cgi-app/simple-cgi-app-misc.c - misc functions
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <string.h>
#include <ctype.h>

char *cgi_strncpy(char *dest, const char *src, size_t n)
{
	dest[n-1] = '\0';
	return strncpy(dest, src, n-1);
}

void cgi_chop_line_end(char *line)
{
	int i;

	for (i = strlen(line) -1; i >= 0; i--) {
		if (line[i] != ' ' && line[i] != '\n' && line[i] != '\t') {
			break;
		}
		line[i] = '\0';
	}
}

int cgi_is_a_number(const char *input)
{
	int i;

	for (i = 0; input[i]; i++) {
		if (!isdigit(input[i])) {
			return 0;
		}
	}

	return 1;
}

int cgi_is_ascii(const char *input)
{
	int i;

	for (i = 0; input[i]; i++) {
		if (!isascii(input[i])) {
			return 0;
		}
	}

	return 1;
}

int cgi_is_null_or_blank(const char *string)
{
	int i;
	if (string == NULL) {
		return 1;
	}
	for (i = 0; string[i]; i++) {
		if (string[i] != ' ') {
			return 0;
		}
	}
	return 1;
}

/* A very very simple check... */
int cgi_is_valid_url(char *url)
{
	int i;

	if (strncmp(url, "http://", 7) != 0) {
		return 0;
	}

	for (i = 0; url[i]; i++) {
		if (url[i] == ' ') {
			return 0;
		}
	}

	return 1;
}

char *cgi_str_toupper(char *string)
{
	int i;

	if (!string) {
		return NULL;
	}

	for (i = 0; string[i]; i++) {
		string[i] = toupper(string[i]);
	}

	return string;
}
