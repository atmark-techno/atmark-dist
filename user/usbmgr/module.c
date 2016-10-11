/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: module.c,v 4.1 2001/01/04 15:01:14 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<stdlib.h>

#include	"usbmgr.h"

/*
 * points to link in first usbmodule
 */
struct node *module_ring = NULL;

#ifdef	NOT_EXPAND_MACRO

struct node *
create_module(char *name)
{
	return create_node_data_link(&module_ring,name,strlen(name)+1);
}

/*
 * search module in module_ring
 * should use #define
 */
struct node *
find_string_module(char *name)
{
	return find_string_node(module_ring,name);
}

#endif
