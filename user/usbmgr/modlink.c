/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: modlink.c,v 4.1 2001/01/04 14:59:50 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<syslog.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<string.h>

#include	"update_usbdb.h"

#ifdef	NOT_EXPAND_MACRO
struct node *
create_modlink(struct node **base,struct node *module)
{
	return create_node_data_link(base,module,0);
}
#endif

void
write_modlink(struct node *start,char *path)
{
	struct node *mlink;
	int fd;
	char sep = '\n';

	if ((fd = creat(path,0644)) == -1) {
		syslog(LOG_ERR,error_create,path);
		return;
	}
	for (mlink = start; ;mlink = mlink->next) {
		char *name = (char *)mlink->data->data;

		if (write(fd,name, strlen(name)) == -1) {
			syslog(LOG_ERR,"Can't write %s",name);
			goto end;
		}
		write(fd,&sep,1);
		if (mlink->next == start)
			break;
	}
end:
	close(fd);
}
