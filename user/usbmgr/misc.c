/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: misc.c,v 3.1 2000/12/01 11:07:26 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */

#include	<stdio.h>
#include	<unistd.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<string.h>

#include	"usbmgr.h"


static int
Mkdir(char *name)
{
	struct stat st;

	if (stat(name,&st) == -1) {
		if (errno == ENOENT) {
			return mkdir(name,0755);
		} else 
			return -1;
	} else if (!S_ISDIR(st.st_mode)) {
		return -1;
	}
	return 0;
}

void
Mkdir_R(char *path)
{
	char *ptr;

	for(ptr = path;ptr = strchr(ptr,'/');ptr++) {
		*ptr = '\0';
		Mkdir(path);
		*ptr = '/';
	}
}
