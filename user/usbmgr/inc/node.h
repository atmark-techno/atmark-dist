/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: node.h,v 4.2 2001/01/04 15:24:30 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#ifndef	__USBMGR_NODE_H
#define	__USBMGR_NODE_H

#include	"debug.h"

struct node {
	struct node *prev;
	struct node *next;
	struct node *data;
};

#define	INIT_NODE(np)	(np)->prev = (np)->next = (np);(np)->data = NULL

extern struct node * create_node_data_link(struct node **,void *,int);
extern void link_node(struct node **,struct node *);
extern void add_node(struct node *,struct node *);
extern struct node * find_string_node(struct node *,char *);
extern void unlink_node(struct node **,struct node *);
extern void delete_node_data_link(struct node **,struct node *);

#endif	/* __USBMGR_NODE_H */

