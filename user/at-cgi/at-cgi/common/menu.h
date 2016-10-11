/*
 * at-cgi-core/menu.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */
 
#ifndef MENU_H_
#define MENU_H_

#define INDEX_CGI_PATH			"/index.cgi"
#define USBDATA_CGI_PATH		"/usbdata.cgi"
#define SYSTEM_CGI_PATH			"/admin/system.cgi"
#define PACKETSCAN_CGI_PATH		"/admin/packetscan.cgi"

#define MAX_MENU_ENTRY_LEN		(50)

typedef struct {
	char name[MAX_MENU_ENTRY_LEN];
	char link[MAX_MENU_ENTRY_LEN];
} menuItem;

extern void print_main_menu(const char *selected);
extern void print_sub_menu(const menuItem *menu_items, int count, const char *selected_link);

#endif /*MENU_H_*/
