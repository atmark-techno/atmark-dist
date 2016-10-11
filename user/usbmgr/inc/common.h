/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: common.h,v 4.2 2001/01/04 14:38:22 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#ifndef	__USBMGR_COMMON_H
#define	__USBMGR_COMMON_H

#define	USBMGR_HOST_FILE	"host"
#define	USBMGR_NOBEEP_FILE	"nobeep"
#define	USBMGR_MODULE_FILE	"module"
#define	USBMGR_SCRIPT_FILE	"script"
#define	USBMGR_CLASS_DIR	"class"
#define	USBMGR_VENDOR_DIR	"vendor"

#include	"node.h"

extern struct node * find_string_module(char *);
extern struct node * create_module(char *);
extern struct node * module_ring;
extern void beep(int );

#ifndef	NOT_EXPAND_MACRO
#define	create_module(name)		create_node_data_link(&module_ring,(name),strlen(name)+1)
#define	find_string_module(name)	find_string_node(module_ring,(name))
#endif

#endif	/* __USBMGR_COMMON_H */
