/*
 * simple-cgi-app/simple-cgi-app-str.h - flexible string handling
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef SIMPLE_CGI_APP_STR_H_
#define SIMPLE_CGI_APP_STR_H_

typedef struct {
	int cap;
	int str_len;
	char str[0];
} cgiStr;

extern cgiStr *cgi_str_new(const char *string);
extern cgiStr *cgi_strn_new(const char *string, int size);
extern void cgi_str_add(cgiStr **cgi_str, char *string);
extern void cgi_strn_add(cgiStr **cgi_str, char *string, int len);

extern void cgi_str_reset(cgiStr **cgi_str);
extern void cgi_str_free(cgiStr *cgi_str);

extern char *cgi_str_get(cgiStr *cgi_str);
extern void cgi_str_print(const cgiStr *cgi_str);

#endif /*SIMPLE_CGI_APP_STR_H_*/
