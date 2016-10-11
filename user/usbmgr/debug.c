/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: debug.c,v 3.1 2000/12/01 11:01:26 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdlib.h>
#include	<syslog.h>

void *
dmalloc(size_t sz)
{
	void *ptr;
	
	ptr = malloc(sz);
	syslog(LOG_DEBUG,"dmalloc address 0x%0x-0x%0x size %u",
		(unsigned long)ptr,(unsigned long)ptr+sz-1,sz);

	return ptr;
}

void
dfree(void *ptr)
{
	syslog(LOG_DEBUG,"dfree address 0x%0x",ptr);
	free(ptr);
}
