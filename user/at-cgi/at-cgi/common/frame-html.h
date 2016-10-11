/*
 * at-cgi-core/frame-html.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef FRAME_HTML_H_
#define FRAME_HTML_H_

#define FORM_NAME	"main"
#define STYLE_SHEET	"/style.css"

extern void print_content_type(void);

extern void print_html_head(const char *post_taget);
extern void print_html_tail(void);

extern void print_frame_top(const char *post_taget);
extern void print_frame_bottom(void);

#endif /*FRAME_HTML_H_*/
