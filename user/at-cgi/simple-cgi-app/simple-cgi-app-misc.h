/*
 * simple-cgi-app/simple-cgi-app-misc.h - misc functions
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef CONV_H_
#define CONV_H_

extern char *cgi_strncpy(char *dest, const char *src, size_t n);

extern int cgi_is_null_or_blank(const char *string);
extern int cgi_is_a_number(const char *input);
extern int cgi_is_ascii(const char *input);
extern void cgi_chop_line_end(char *line);

extern int cgi_is_valid_url(char *url);

extern char *cgi_str_toupper(char *string);

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))

#endif /*CONV_H_*/
