/*
 * at-cgi-core/at-cgi-html-parts.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef HTMLPARTS_H_
#define HTMLPARTS_H_

#include "simple-cgi-app-llist.h"

#define HTML_OPTION_LEN_MAX		(256)
typedef struct {
	char label[HTML_OPTION_LEN_MAX];
	char value[HTML_OPTION_LEN_MAX];
} cgiHtmlOption;

#define RADIO_BUTTON_ID_LEN_MAX	(256)
typedef struct {
	char id[RADIO_BUTTON_ID_LEN_MAX];
	int checked;
} cgiRadioButton;

extern void cgi_print_select_box(const char *id, int size, const cgiHtmlOption *options, int option_count, const char *selected_val, const char *on_change);
extern void cgi_print_select_box_from_str_list(const char *id, int size, cgiStrList *options, const char *selected_val, const char *on_change);
extern void cgi_print_select_box_from_str_pair_list(const char *id, int size, cgiStrPairList *options, const char *selected_val, const char *on_change);
extern void cgi_print_button(const char *name, const char *id, const char *onclick);
extern void cgi_print_button_confirm(const char *name, const char *id, const char *onclick, const char *messsage);
extern void cgi_print_text_box(const char *id, const char *value, int size, int maxlength);
extern void cgi_print_password_box(const char *id, const char *value, int size, int maxlength);
extern void cgi_print_checkbox(const char *id, int checked, const char *onclick);
extern void cgi_print_radio_button_group(const char *group_id, const cgiRadioButton radio_buttons[], int count);
extern void cgi_print_radio_button(const char *group_id, const char *id, int checked, const char *onclick);
extern void cgi_print_text_area(const char *id, const char *text, int rows, int cols);
extern void cgi_print_hidden_field(const char *id, const char *value);

extern void cgi_print_row_from_tab_line(char *lines, int start_row);

#endif /*HTMLPARTS_H_*/
