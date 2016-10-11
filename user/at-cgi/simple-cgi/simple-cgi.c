/*
 * simple-cgi/simple-cgi.c - a very basic convenience library for c cgis
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "simple-cgi.h"

typedef struct {
	char *name;
	char *value;
} cgiNameValPair;

typedef struct {
	cgiNameValPair pairs[CGI_QUERY_PAIRS_MAX];
	int pair_count;
	char query[0];
} cgiHtmlQuery;

static cgiHtmlQuery *cgi_html_query;

int cgi_get_content_length(void)
{
	int content_length = 0;
	char *content_len_s;
	
	if (strcasecmp(cgi_get_request_method(), "POST") == 0) {

		content_len_s = getenv("CONTENT_LENGTH");
		if (content_len_s == NULL){
			content_length = 0;
		} else {
			content_length = atoi(content_len_s);
		}

	} else if (strcasecmp(cgi_get_request_method(), "GET") == 0) {

		char *query_string = getenv("QUERY_STRING");
		
		if (!query_string) {
			content_length = 0;
		} else {
			content_length = strlen(query_string);
		}

	}

	return content_length;
}

/* Parses the query string to name/value pairs.
 * Query pairs more than CGI_QUERY_PAIRS_MAX will just be ignored. */
static void cgi_parse_query(void)
{
	int p_count = 0, q_pos;
	char *last;

	if (!cgi_html_query) {
		return;
	}

	cgi_html_query->pairs[p_count].name = cgi_html_query->query;

	for (q_pos = 0; cgi_html_query->query[q_pos]; q_pos++) {

		if (cgi_html_query->query[q_pos] == '=') {

			cgi_html_query->query[q_pos] = '\0';
			cgi_html_query->pairs[p_count].value = &cgi_html_query->query[q_pos + 1];
			p_count++;

			if (p_count >= CGI_QUERY_PAIRS_MAX) {
				last = strchr(&cgi_html_query->query[q_pos + 1], '&');
				if (last) {
					*last = '\0';
				}
				cgi_html_query->pair_count = p_count;
				return;
			}

		} else if (cgi_html_query->query[q_pos] == '&') {

			cgi_html_query->query[q_pos] = '\0';
			cgi_html_query->pairs[p_count].name = &cgi_html_query->query[q_pos + 1];

		}
	}

	cgi_html_query->pair_count = p_count;
}

static void cgi_decode_query(void)
{
 	char *src_c, *dst_c;
  	int code;

	if (!cgi_html_query) {
		return;
	}

	src_c = cgi_html_query->query;

 	for (dst_c = src_c; *src_c; src_c++) {

		if (*src_c == '%') {
			if (sscanf(src_c + 1, "%2x", &code) != 1) {
				code = '?';
			}
			*dst_c++ = code;
			src_c += 2;
		} else if (*src_c == '+') {
			*dst_c++ = ' ';
		} else {
			*dst_c++ = *src_c;
		}

	}
	*dst_c = '\0';
}

int static set_query_from_post(int content_length)
{
	int read_in = 0, read_total = 0;

	if (!cgi_html_query) {
		return -1;
	}

	while (read_total < content_length) {

		read_in = fread(&cgi_html_query->query[read_total], 1,
			content_length - read_total, stdin);
		if (read_in != (content_length - read_total)) {
			if (feof(stdin) || ferror(stdin)) {
				free(cgi_html_query);
				return -1;
			}
		}
		read_total += read_in;

	}
	cgi_html_query->query[read_in] = '\0';

	return 0;
}

int static set_query_from_get(int content_length)
{
	char *query_string;

	if (!cgi_html_query) {
		return -1;
	}

	query_string = getenv("QUERY_STRING");
	if (!query_string) {
		return -1;	
	}

	strncpy(cgi_html_query->query, query_string, content_length);
	cgi_html_query->query[content_length] = '\0';

	return 0;
}

char *cgi_get_request_method(void)
{
	return getenv("REQUEST_METHOD");
}

int cgi_query_process(void)
{
	int ret, content_length;
	
	content_length = cgi_get_content_length();

	cgi_html_query = calloc(sizeof(cgiHtmlQuery) + content_length + 1, 1);
	if (!cgi_html_query) {
		return -1;
	}
	
	if (content_length == 0) {
		return 0;
	}

	if (strcasecmp(cgi_get_request_method(), "POST") == 0) {

		ret = set_query_from_post(content_length);
		if (ret < 0) {
			free(cgi_html_query);
			return -1;
		}

	} else if (strcasecmp(cgi_get_request_method(), "GET") == 0) {

		ret = set_query_from_get(content_length);
		if (ret < 0) {
			free(cgi_html_query);
			return -1;
		}

	} else {

		free(cgi_html_query);
		return -1;

	}

	cgi_decode_query();

	cgi_parse_query();

	return 0;
}

void cgi_query_free(void)
{
	free(cgi_html_query);
}

char *cgi_query_get_val(const char *pair_name)
{
	int i;

	if (!cgi_html_query) {
		return NULL;
	}

	for (i = 0; i < cgi_html_query->pair_count; i++) {

		if (strcmp(cgi_html_query->pairs[i].name, pair_name) == 0) {
			return cgi_html_query->pairs[i].value;
		}

	}

	return NULL;
}

void cgi_query_get_pairs(const char *pair_name_head,
		char *matches[][2], int max_matches)
{
	int i, match_no = 0;

	if (!cgi_html_query || !matches) {
		return;
	}
	
	memset(matches, 0, sizeof(char *) * max_matches);

	for (i = 0; i < cgi_html_query->pair_count; i++) {

		if (strncmp(cgi_html_query->pairs[i].name, pair_name_head, strlen(pair_name_head)) == 0) {

			matches[match_no][0] = cgi_html_query->pairs[i].name;
			matches[match_no][1] = cgi_html_query->pairs[i].value;
			match_no++;
			if (match_no >= max_matches) {
				return;
			}

		}

	}
}

#ifdef DEBUG
void cgi_query_dump_html(void)
{
	int i;

	if (!cgi_html_query) {
		return;
	}

	for (i = 0; i < cgi_html_query->pair_count; i++) {

		printf("%s: [%s]<br />\n",
			cgi_html_query->pairs[i].name,
			cgi_html_query->pairs[i].value
			);

	}
}
#endif

char *cgi_get_script_path(void)
{
	return getenv("SCRIPT_NAME");
}

char *cgi_get_script_name(void)
{
	char *script_name = getenv("SCRIPT_NAME");
	if (!script_name) {
		return NULL;
	}
	while (strstr(script_name, "/")) {
		script_name++;
	}
	return script_name;
}

char *cgi_get_auth_type(void)
{
	return getenv("AUTH_TYPE");
}

char *cgi_get_content_type(void)
{
	return getenv("CONTENT_TYPE");
}

char *cgi_get_gateway_interface(void)
{
	return getenv("GATEWAY_INTERFACE");
}

char *cgi_get_path_info(void)
{
	return getenv("PATH_INFO");
}

char *cgi_get_path_translated(void)
{
	return getenv("PATH_TRANSLATED");
}

char *cgi_get_query_string(void)
{
	return getenv("QUERY_STRING");
}

char *cgi_get_remote_addr(void)
{
	return getenv("REMOTE_ADDR");
}

char *cgi_get_remote_host(void)
{
	return getenv("REMOTE_HOST");
}

char *cgi_get_remote_ident(void)
{
	return getenv("REMOTE_IDENT");
}

char *cgi_get_remote_user(void)
{
	return getenv("REMOTE_USER");
}

char *cgi_get_server_name(void)
{
	return getenv("SERVER_NAME");
}

char *cgi_get_server_port(void)
{
	return getenv("SERVER_PORT");
}

char *cgi_get_server_protocol(void)
{
	return getenv("SERVER_PROTOCOL");
}

char *cgi_get_server_software(void)
{
	return getenv("SERVER_SOFTWARE");
}

char *cgi_get_http_accept(void)
{
	return getenv("HTTP_ACCEPT");
}

char *cgi_get_http_user_agent(void)
{
	return getenv("HTTP_USER_AGENT");
}

char *cgi_get_http_refferer(void)
{
	return getenv("HTTP_REFERER");
}

char *cgi_get_http_cookie(void)
{
	return getenv("HTTP_COOKIE");
}
