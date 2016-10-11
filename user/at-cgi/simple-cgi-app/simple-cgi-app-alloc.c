/*
 * simple-cgi-app/simple-cgi-app-alloc.c - wrappers for malloc etc
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define OOM_DIE_EXIT_VALUE	(EXIT_FAILURE)
#define OOM_ERROR_HTML	"<span style=\"color: red;\">Out of memory</span>\n"

static void oom_msg_and_die(void)
{
	printf(OOM_ERROR_HTML);
	exit(OOM_DIE_EXIT_VALUE);
}

void *cgi_malloc(size_t size)
{
	void *ptr;

	if (size == 0) {
		return NULL;
	}

	ptr = malloc(size);
	if (!ptr) {
		oom_msg_and_die();
	}

	return ptr;
}

void *cgi_calloc(size_t nmemb, size_t size)
{
	void *ptr;

	if (size == 0 || nmemb == 0) {
		return NULL;
	}

	ptr = calloc(nmemb, size);
	if (!ptr) {
		oom_msg_and_die();
	}

	return ptr;
}

void *cgi_realloc(void *ptr, size_t size)
{
	if (size == 0) {
		free(ptr);
		return NULL;
	}

	ptr = realloc(ptr, size);
	if (!ptr) {
		oom_msg_and_die();
	}
	
	return ptr;
}

char *cgi_strdup(const char *s)
{
	char *r;
	
	if (!s) {
		return NULL;	
	}
	
	r = strdup(s);
	if (!r) {
		oom_msg_and_die();
	}
	
	return r;
}

char *cgi_strndup(const char *s, size_t n)
{
	char *r;
	
	if (!s) {
		return NULL;	
	}
	
	r = calloc(++n, 1);
	if (!r) {
		oom_msg_and_die();
	}
	
	strncpy(r, s, n);
	
	return r;
}
