/*
 * simple-cgi/simple-cgi.h - a very basic convenience library for  c cgis
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef SIMPLE_CGI_H
#define SIMPLE_CGI_H

/* simple-cgi uses a fixed size array to store query
 * name/value pairs. Should change it to a linked list.
 * or something. */
#define CGI_QUERY_PAIRS_MAX	(100)

/* Prepares any posted or getted query data for use by the cgi by 
 * parsing and decoding it.
 * cgi_get_content_length() can be called first to check
 * if the query is too big.
 * Returns -1 on error */
extern int cgi_query_process(void);

/* Frees resources allocated after calling cgi_process_query() */
extern void cgi_query_free(void);

/* Returns the value of the query parameter if it exits.
 * Note: cgi_process_query() must be called before using this */
extern char *cgi_query_get_val(const char *pair_name);
/* Same as above, but returns an array of pairs where the pair names 
 * begin with pair_name_head */
extern void cgi_query_get_pairs(const char *pair_name_head, char *matches[][2], int max_matches);

#ifdef DEBUG
/* Prints out all query pairs */
extern void cgi_query_dump_html(void);
#endif

extern char *cgi_get_auth_type(void);
extern int cgi_get_content_length(void);
extern char *cgi_get_content_type(void);
extern char *cgi_get_gateway_interface(void);
extern char *cgi_get_path_info(void);
extern char *cgi_get_path_translated(void);
extern char *cgi_get_query_string(void);
extern char *cgi_get_remote_addr(void);
extern char *cgi_get_remote_host(void);
extern char *cgi_get_remote_ident(void);
extern char *cgi_get_remote_user(void);
extern char *cgi_get_request_method(void);
extern char *cgi_get_script_name(void);
extern char *cgi_get_script_path(void);
extern char *cgi_get_server_name(void);
extern char *cgi_get_server_port(void);
extern char *cgi_get_server_protocol(void);
extern char *cgi_get_server_software(void);

extern char *cgi_get_http_accept(void);
extern char *cgi_get_http_user_agent(void);
extern char *cgi_get_http_refferer(void);
extern char *cgi_get_http_cookie(void);

#endif /*SIMPLE_CGI_H*/

