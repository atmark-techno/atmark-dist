/*
 * simple-cgi-app/simple-cgi-app-llist.c - linked lists
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <string.h>
#include <stdlib.h>

#include "simple-cgi-app-alloc.h"

#include "simple-cgi-app-llist.h"

#include <misc-utils.h>

void cgi_str_list_add(cgiStrList **list_current, char *str_to_add)
{
	cgiStrList *list_new_end = *list_current;
	cgiStrList *list_pre = NULL;

	while (list_new_end) {
		list_pre = list_new_end;
		list_new_end = list_new_end->next_list;
	}

	list_new_end = cgi_calloc(sizeof(cgiStrList), 1);
	list_new_end->str = cgi_strdup(str_to_add);
	if (list_pre) {
		list_pre->next_list = list_new_end;
	} else {
		*list_current = list_new_end;
	}

	return;
}

void cgi_str_list_add_to_start(cgiStrList **list_current, char *str_to_add)
{
	cgiStrList *list_new;

	list_new = cgi_calloc(sizeof(cgiStrList), 1);
	list_new->str = cgi_strdup(str_to_add);
	list_new->next_list = *list_current;
	*list_current = list_new;
}

void cgi_str_list_add_unique(cgiStrList **list_current, char *str_to_add)
{
	cgiStrList *list_new_end = *list_current;
	cgiStrList *list_pre = NULL;

	/* find end */
	while (list_new_end) {
		/* or do nothing if duplicate */
		if (strcmp(list_new_end->str, str_to_add) == 0) {
			return;
		}
		list_pre = list_new_end;
		list_new_end = list_new_end->next_list;
	}

	list_new_end = cgi_calloc(sizeof(cgiStrList), 1);
	list_new_end->str = cgi_strdup(str_to_add);
	if (list_pre) {
		list_pre->next_list = list_new_end;
	} else {
		*list_current = list_new_end;
	}

	return;
}

void cgi_str_list_add_unique_ordered(cgiStrList **list_current, char *str_to_add)
{
	cgiStrList *list_next = *list_current;
	cgiStrList *list_pre = NULL, *list_new;

	/* either find end, placement before a greater string, or do nothing
	 * if the string is a duplicate */
	while (list_next) {
		/* duplicate */
		if (strcmp(list_next->str, str_to_add) == 0) {
			return;
		/* next list has a greater string */
		} else if (strcmp (str_to_add, list_next->str) < 0) {
			break;	
		}
		list_pre = list_next;
		list_next = list_next->next_list;
	}

	list_new = cgi_calloc(sizeof(cgiStrList), 1);
	list_new->str = cgi_strdup(str_to_add);
	list_new->next_list = list_next;
	
	if (list_pre) {
		list_pre->next_list = list_new;
	} else {
		*list_current = list_new;
	}

	return;
}

void cgi_str_list_free(cgiStrList *list_start)
{
	cgiStrList *list_to_free = list_start;
	cgiStrList *next_list_to_free;

	while (list_to_free) {
		next_list_to_free = list_to_free->next_list;
		free(list_to_free->str);
		free(list_to_free);
		list_to_free = next_list_to_free;
	}
}

cgiStrList *cgi_str_list_next(cgiStrList *list_current)
{
	return list_current->next_list;
}

int cgi_str_list_exists(cgiStrList *list_start, char *str)
{
	while (list_start) {
		if (strcmp(list_start->str, str) == 0) {
			return 1;
		}
		list_start = list_start->next_list;
	}

	return 0;
}

int cgi_str_list_length(cgiStrList *list_start)
{
	int count = 0;

	while (list_start) {
		count++;
		list_start = list_start->next_list;
	}

	return count;
}

void cgi_str_pair_list_add(cgiStrPairList **list_current,
		char *name_to_add, char *val_to_add)
{
	cgiStrPairList *list_new_end = *list_current;
	cgiStrPairList *list_pre = NULL;

	/* find end */
	while (list_new_end) {
		/* or replace if duplicate */
		if (strcmp(list_new_end->name, name_to_add) == 0) {
			free(list_new_end->val);
			list_new_end->val = cgi_strdup(val_to_add);
			return;
		}
		list_pre = list_new_end;
		list_new_end = list_new_end->next_list;
	}

	list_new_end = cgi_calloc(sizeof(cgiStrPairList), 1);
	list_new_end->name = cgi_strdup(name_to_add);
	list_new_end->val = cgi_strdup(val_to_add);
	if (list_pre) {
		list_pre->next_list = list_new_end;
	} else {
		*list_current = list_new_end;
	}

	return;
}

void cgi_str_pair_list_free(cgiStrPairList *list_start)
{
	cgiStrPairList *list_to_free = list_start;
	cgiStrPairList *next_list_to_free;

	while (list_to_free) {
		next_list_to_free = list_to_free->next_list;
		free(list_to_free->name);
		free(list_to_free->val);
		free(list_to_free);
		list_to_free = next_list_to_free;
	}
}

cgiStrPairList *cgi_str_pair_list_next(cgiStrPairList *list_current)
{
	return list_current->next_list;
}

char *cgi_str_pair_list_get_val(cgiStrPairList *list_start, char *name)
{
	while (list_start) {
		if (strcmp(list_start->name, name) == 0) {
			return list_start->val;
		}
		list_start = list_start->next_list;
	}

	return NULL;
}

cgiStrPairLL *cgi_str_pair_ll_add(cgiStrPairLL **list_current, cgiStrPairList *list_to_add)
{
	cgiStrPairLL *list_new_end = *list_current;
	cgiStrPairLL *list_pre = NULL;

	while (list_new_end) {
		list_pre = list_new_end;
		list_new_end = list_new_end->next_list;
	}

	list_new_end = cgi_calloc(sizeof(cgiStrPairLL), 1);
	list_new_end->str_pair_list = list_to_add;
	if (list_pre) {
		list_pre->next_list = list_new_end;
	} else {
		*list_current = list_new_end;
	}

	return list_new_end;
}

void cgi_str_pair_ll_free(cgiStrPairLL *list_start)
{
	cgiStrPairLL *list_to_free = list_start;
	cgiStrPairLL *next_list_to_free;

	while (list_to_free) {
		next_list_to_free = list_to_free->next_list;
		cgi_str_pair_list_free(list_to_free->str_pair_list);
		free(list_to_free);
		list_to_free = next_list_to_free;
	}
}

cgiStrPairLL *cgi_str_pair_ll_next(cgiStrPairLL *list_current)
{
	return list_current->next_list;
}

cgiStrPairList *cgi_str_pair_ll_get(cgiStrPairLL *list_start, int index)
{
	int pos = 0;

	while (list_start) {
		if (pos == index) {
			return list_start->str_pair_list;
		}
		pos++;
		list_start = list_start->next_list;
	}

	return NULL;
}

int cgi_str_pair_ll_delete(cgiStrPairLL **list_start, int index)
{
	int pos = 0;
	cgiStrPairLL *previous_list = NULL, *next_list;

	next_list = *list_start;
	while (next_list) {

		if (pos == index) {

			if (previous_list) {
				previous_list->next_list = next_list->next_list;
			} else {
				*list_start = next_list->next_list;
			}
			cgi_str_pair_list_free(next_list->str_pair_list);
			free(next_list);
			return 0;

		}

		pos++;
		previous_list = next_list;
		next_list = next_list->next_list;

	}

	return -1;
}

int cgi_str_pair_ll_replace(cgiStrPairLL **list_start, int index, cgiStrPairList *new_list)
{
	int pos = 0;
	cgiStrPairLL *previous_list = NULL, *next_list;

	next_list = *list_start;
	while (next_list) {

		if (pos == index) {

			cgi_str_pair_list_free(next_list->str_pair_list);
			next_list->str_pair_list = new_list;
			return 0;

		}

		pos++;
		previous_list = next_list;
		next_list = next_list->next_list;

	}

	return -1;
}
