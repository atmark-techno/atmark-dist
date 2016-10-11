/*
 * simple-cgi-app/simple-cgi-app-html-parts.c - html parts
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdio.h>
#include <string.h>

#include "simple-cgi-app-html-parts.h"

#define STD_TEXTBOX_LEN			(20)
#define STD_TEXTBOX_MAX_LEN		(100)

void cgi_print_button(const char *name, const char *id, const char *onclick)
{
	printf("<input type=\"submit\" value=\"");
	printf(name);
	if (strcmp(onclick, "") == 0) {
		printf("\" name=\"%s\" onclick=\"regevent(this)\" />", id);
	} else {
		printf("\" name=\"%s\" onclick=\"%s; regevent(this)\" />", id, onclick);
	}
}

void cgi_print_button_confirm(const char *name, const char *id, const char *onclick, const char *messsage)
{
	printf("<input type=\"submit\" value=\"%s", name);
	if (strcmp(onclick, "") == 0) {
		printf("\" name=\"%s\" onclick=\"regeventconfirm(this, '%s'); return false\" />", id, messsage);
	} else {
		printf("\" name=\"%s\" onclick=\"%s; regeventconfirm(this, '%s'); return false\" />", id, onclick, messsage);
	}
}

void cgi_print_select_box(
	const char *id,
	int size,
	const cgiHtmlOption *options,
	int option_count,
	const char *selected_val,
	const char *on_change)
{
	int i;

	printf("<select name=\"%s\" size=\"%d\" onchange=\"%s\">\n", id, size, on_change);

	for (i = 0; i < option_count; i++) {
		if ((strcmp(options[i].value, selected_val)) == 0 &&
			(strlen(options[i].value) == strlen(selected_val))) {
			printf("<option value=\"%s\" selected=\"selected\">%s</option>\n",
				options[i].value, options[i].label);
		} else {
			printf("<option value=\"%s\">%s</option>\n",
				options[i].value, options[i].label);
		}
	}

	printf("</select>\n");
}

void cgi_print_select_box_from_str_list(const char *id, int size, cgiStrList *options,
		const char *selected_val, const char *on_change)
{
	cgiStrList *next_list;

	printf("<select name=\"%s\" size=\"%d\" onchange=\"%s\">\n", id, size, on_change);

	next_list = options;
	while (next_list) {

		if (strcmp(next_list->str, selected_val) == 0) {
			printf("<option value=\"%s\" selected=\"selected\">%s</option>\n",
				next_list->str, next_list->str);
		} else {
			printf("<option value=\"%s\">%s</option>\n",
				next_list->str, next_list->str);
		}

		next_list = next_list->next_list;

	}

	printf("</select>\n");
}

void cgi_print_select_box_from_str_pair_list(const char *id, int size, cgiStrPairList *options,
		const char *selected_val, const char *on_change)
{
	cgiStrPairList *next_list;

	printf("<select name=\"%s\" size=\"%d\" onchange=\"%s\">\n", id, size, on_change);

	next_list = options;
	while (next_list) {

		if (strcmp(next_list->val, selected_val) == 0) {
			printf("<option value=\"%s\" selected=\"selected\">%s</option>\n",
				next_list->val, next_list->name);
		} else {
			printf("<option value=\"%s\">%s</option>\n",
				next_list->val, next_list->name);
		}

		next_list = next_list->next_list;

	}

	printf("</select>\n");
}

void cgi_print_text_box(const char *id, const char *value, int size, int maxlength)
{
	int s = STD_TEXTBOX_LEN, max_len = STD_TEXTBOX_MAX_LEN;

	if (size != 0) {
		s = size;
	}
	if (maxlength != 0) {
		max_len = maxlength;
	}

	printf("<input type=\"text\" name=\"%s\" value=\"%s\" size=\"%d\" maxlength=\"%d\" />\n",
		 id, value, s, max_len);
}

void cgi_print_password_box(const char *id, const char *value, int size, int maxlength)
{
	int s = STD_TEXTBOX_LEN, max_len = STD_TEXTBOX_MAX_LEN;

	if (size != 0) {
		s = size;
	}
	if (maxlength != 0) {
		max_len = maxlength;
	}

	printf("<input type=\"password\" name=\"%s\" value=\"%s\" size=\"%d\" maxlength=\"%d\" />\n",
		 id, value, s, max_len);
}

void cgi_print_checkbox(const char *id, int checked, const char *onclick)
{
	if (checked) {
		printf("<input type=\"checkbox\" name=\"%s\" onclick=\"%s\" checked=\"checked\" />\n", id, onclick);
	} else {
		printf("<input type=\"checkbox\" name=\"%s\" onclick=\"%s\" />\n", id, onclick);
	}
}

void cgi_print_radio_button_group(const char *group_id, const cgiRadioButton radio_buttons[], int count)
{
	int i;

	for (i = 0; i < count; i++) {

		if (radio_buttons[i].checked) {
			printf("<input type=\"radio\" class=\"radio\" name=\"%s\" value=\"%s\" checked=\"checked\" />\n",
				group_id, radio_buttons[i].id);
		} else {
			printf("<input type=\"radio\" class=\"radio\" name=\"%s\" value=\"%s\" />\n",
				group_id, radio_buttons[i].id);
		}

	}
}

void cgi_print_radio_button(const char *group_id, const char *id, int checked, const char *onclick)
{
	if (checked) {
		printf("<input type=\"radio\" class=\"radio\" name=\"%s\" value=\"%s\" checked=\"checked\" onclick=\"%s\" />\n",
			group_id, id, onclick);
	} else {
		printf("<input type=\"radio\" class=\"radio\" name=\"%s\" value=\"%s\" onclick=\"%s\" />\n",
			group_id, id, onclick);
	}
}

void cgi_print_text_area(const char *id, const char *text, int rows, int cols)
{
	printf("<textarea name=\"%s\" rows=\"%d\" cols=\"%d\">%s</textarea>\n",
		id, rows, cols, text);
}

void cgi_print_hidden_field(const char *id, const char *value)
{
	printf("<input type=\"hidden\" name=\"%s\" value=\"%s\" />\n",
		id, value);
}

void cgi_print_row_from_tab_line(char *lines, int start_row)
{
	int i = 0, temp, row_pos = 1;

	while (row_pos < start_row) {
		while (lines[i] != '\n') {
			i++;
		}
		row_pos++;
		i++;
	}

	printf("<tr>");

	while (lines[i]) {

		printf("<td>");
		temp = i;
		while (lines[temp] != '\t' && lines[temp] != '\n') {
			temp++;
		}
		if (lines[temp] == '\n') {
			lines[temp] = '\0';
			printf("%s</td></tr>\n", &lines[i]);
			i = temp+1;
			if (lines[i]) {
				printf("<tr>");
			}
			continue;
		}
		lines[temp] = '\0';
		temp++;
		printf("%s</td>", &lines[i]);
		while (lines[temp] == '\t') {
			temp++;
		}
		i = temp;

	}
}
