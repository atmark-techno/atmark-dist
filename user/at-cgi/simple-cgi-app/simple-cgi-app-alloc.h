/*
 * simple-cgi-app/simple-cgi-app-alloc.h - wrappers for malloc etc
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef SIMPLECGIAPPALLOC_H_
#define SIMPLECGIAPPALLOC_H_

/* The following will all print an error to STDOUT
 * and exit if memory cannot be allocated. */
 
extern void *cgi_malloc(size_t size);
extern void *cgi_calloc(size_t nmemb, size_t size);
extern void *cgi_realloc(void *ptr, size_t size);
extern char *cgi_strdup(const char *s);
extern char *cgi_strndup(const char *s, size_t n);

#endif /*SIMPLECGIAPPALLOC_H_*/
