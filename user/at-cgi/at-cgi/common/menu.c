/*
 * at-cgi-core/memu.c - very very simple menus for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "menu.h"

static menuItem main_menu_items[] = {
	{"Overview",	INDEX_CGI_PATH},
#ifdef USBDATA
	{"USB Data",	USBDATA_CGI_PATH},
#endif
#ifdef PACKETSCAN
	{"Packet Scan",	PACKETSCAN_CGI_PATH},
#endif
#ifdef SYSTEM
	{"System",		SYSTEM_CGI_PATH},
#endif
};

void print_main_menu(const char *selected_link)
{
	int i, count, width;

	count = sizeof(main_menu_items)/sizeof(menuItem);
	width = 94 / count;

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\" width=\"100%%\" id=\"main_menu\"><tr>\n");
	printf("<td class=\"left\" width=\"3%%\"></td>\n");

	for (i = 0; i < count; i++) {
		if (strcmp(main_menu_items[i].link, selected_link) == 0) {
			printf("<td class=\"selected\" align=\"center\" width=\"%d%%\"> <a href=\"%s\">%s</a></td>\n",
				width,
				main_menu_items[i].link,
				main_menu_items[i].name);
		} else {
			printf("<td align=\"center\" width=\"%d%%\"> <a href=\"%s\">%s</a></td>\n",
				width,
				main_menu_items[i].link,
				main_menu_items[i].name);
		}
	}

	printf("<td class=\"right\" width=\"3%%\"></td>\n</tr></table>\n");
}


void print_sub_menu(const menuItem *menu_items, int count, const char *selected_link)
{
	int i, width = 100 / count;
	char class[] = {"right_border"};

	printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\" width=\"98%%\" class=\"sub_menu\"><tr>\n");
	printf("<td class=\"left\" width=\"3%%\"></td>\n");

	for (i = 0; i < count; i++) {

		if (i == count - 1) {
			class[0] = '\0';
		}

		if (count > 4) {
			printf("<td align=\"center\" class=\"%s\">", class);
		} else {
			printf("<td align=\"center\" width=\"%d%%\" class=\"%s\">", width, class);
		}
		printf("<a href=\"\" onclick=\"linkregevent('%s'); ", menu_items[i].link);		

		if (strcmp(menu_items[i].link, selected_link) == 0) {
			printf("return false\" class=\"selected\">%s</a></td>\n", menu_items[i].name);
		} else {
			printf("return false\">%s</a></td>\n", menu_items[i].name);
		}

	}

	printf("<td class=\"right\" width=\"3%%\"></td>\n</tr></table>\n");
}
