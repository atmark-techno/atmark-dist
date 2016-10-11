/*
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: module.h,v 4.2 2001/01/04 14:40:48 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#ifndef	__USBMGR_MODULE_H
#define	__USBMGR_MODULE_H

#include	"node.h"

extern struct node * find_string_module(char *);
extern struct node * create_module(char *);
extern struct node * module_ring;

#ifndef	NOT_EXPAND_MACRO
#define	create_module(name)		create_node_data_link(&module_ring,(name),strlen(name)+1)
#define	find_string_module(name)	find_string_node(module_ring,(name))
#endif

#endif	/* __USBMGR_MODULE_H */
