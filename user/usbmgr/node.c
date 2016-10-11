/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: node.c,v 4.2 2001/01/04 15:14:38 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	"node.h"

/*
 * create node , allocate data and link to base
 */
struct node *
create_node_data_link(struct node **base,void *data,int size)
{
	struct node *np;
	unsigned int alloc_size;

	if (size > 0)
		alloc_size = sizeof(struct node) + size + 1;
	else
		alloc_size = sizeof(struct node);
	np = (struct node *)Malloc(alloc_size);
	if (np == NULL)
		return NULL;
	INIT_NODE(np);
	link_node(base,np);
	if (size > 0) {
		np->data = (struct node *)(&(np->data)+1);
		memcpy((char *)np->data,(char *)data,size);
	} else {
		np->data = data;
	}

	return np;
}

void
delete_node_data_link(struct node **base,struct node *np)
{
	unlink_node(base,np);
	Free(np);
}

/*
 * link node
 */
void
link_node(struct node **linkp,struct node *np)
{
	if (*linkp == NULL)
		*linkp = np;
	else
		add_node(*linkp,np);
}

/*
 * unlink node
 */
void
unlink_node(struct node **base,struct node *np)
{
	if (np == *base) {	/* np is first node */
		if (np->prev == np)		/* np is only one node */
			*base = NULL;
		else
			*base = np->next;
	}
	np->next->prev = np->prev;
	np->prev->next = np->next;
}


/*
 * add node
 */
void
add_node(struct node *link,struct node *np)
{
	np->next = link;
	np->prev = link->prev;
	link->prev->next = np;
	link->prev = np;
}

/*
 * find data in node
 */
struct node *
find_string_node(struct node *start,char *name)
{
	struct node *np;

	if (start == NULL)
		return NULL;
	for (np = start; ;np = np->next) {
		if (strcmp((char *)np->data,name) == 0)
			return np;
		if (np->next == start)
			break;
	}
	return NULL;
}
