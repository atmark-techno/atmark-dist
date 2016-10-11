/*
 * simple-cgi-app/simple-cgi-app-str.c - flexible string handling
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simple-cgi-app-alloc.h"

#include "simple-cgi-app-str.h"

#define CGI_STR_INIT_SIZE	(512)

cgiStr *cgi_strn_new(const char *string, int size)
{
	int new_cap;
	cgiStr *cgi_str;

	if ((string != NULL) && strlen(string) < size) {
		size = strlen(string);
	}

	if (CGI_STR_INIT_SIZE > size) {
		new_cap = CGI_STR_INIT_SIZE;
	} else {
		new_cap = size + 1;
	}

	cgi_str = cgi_malloc(sizeof(cgiStr) + new_cap);

	if (string) {
		strncpy(cgi_str->str, string, size);
		cgi_str->str_len = size;
		cgi_str->str[cgi_str->str_len] = '\0';		
	} else {
		cgi_str->str_len = 0;
		cgi_str->str[0] = '\0';
	}

	cgi_str->cap = new_cap;

	return cgi_str;
}

cgiStr *cgi_str_new(const char *string)
{
	int size = 0;

	if (string) {
		size = strlen(string);
	}

	return cgi_strn_new(string, size);
}

void cgi_strn_add(cgiStr **cgi_str, char *string, int len)
{
	int tmp_len, tmp_cap;

	if (!cgi_str || !string || len < 0) {
		return;
	}

	if (strlen(string) < len) {
		len = strlen(string);
	}

	if ((*cgi_str)->str_len + len >= (*cgi_str)->cap - 1) {

		while ((*cgi_str)->str_len + len >= (*cgi_str)->cap - 1) {
			(*cgi_str)->cap <<= 1;
		}

		tmp_len = (*cgi_str)->str_len;
		tmp_cap = (*cgi_str)->cap;
		*cgi_str = cgi_realloc(*cgi_str, sizeof(cgiStr) + (*cgi_str)->cap);
		(*cgi_str)->cap = tmp_cap;
		(*cgi_str)->str_len = tmp_len;

	}

	strncpy((*cgi_str)->str + (*cgi_str)->str_len, string, len);

	(*cgi_str)->str_len += len;
	(*cgi_str)->str[(*cgi_str)->str_len] = '\0';
}

void cgi_str_add(cgiStr **cgi_str, char *string)
{
	if (!string) {
		return;
	}

	cgi_strn_add(cgi_str, string, strlen(string));
}

void cgi_str_reset(cgiStr **cgi_str)
{
	if (!cgi_str) {
		return;
	}

	(*cgi_str)->str_len = 0;
	(*cgi_str)->str[0] = '\0';
}

void cgi_str_free(cgiStr *cgi_str)
{
	if (!cgi_str) {
		return;
	}

	free(cgi_str);
}

void cgi_str_print(const cgiStr *cgi_str)
{
	if (!cgi_str) {
		return;
	}

	printf(cgi_str->str);
}

char *cgi_str_get(cgiStr *cgi_str)
{
	if (!cgi_str) {
		return NULL;
	}

	return cgi_str->str;
}
