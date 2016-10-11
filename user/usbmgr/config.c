/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: config.c,v 4.0 2000/12/21 09:52:27 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<limits.h>
#include	<linux/limits.h>
#include	<unistd.h>
#include	<syslog.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"update_usbdb.h"

#define	CONFIG_ID_INVALID	0x00

static struct node *config_ring = NULL;

/*
 * create config
 */
struct config *
create_config(void)
{
	struct config *conf;

	conf = (struct config *)Malloc(sizeof(struct config));
	if (conf == NULL)
		return NULL;
	INIT_NODE(&(conf->link));
	conf->id.vendor = conf->id.product = CONFIG_ID_INVALID;
	conf->id.class = conf->id.subclass = conf->id.protocol = CONFIG_ID_INVALID;
	conf->id.type = CONFIG_TYPE_NONE;
	conf->script = NULL;

	return conf;
}

/*
 * link to config_ring
 */
void
link_config(struct config *conf)
{
	link_node(&config_ring,&(conf->link));
}

/*
 *  validate type in config
 */ 
void
validate_config(struct config *conf)
{
	if (conf->id.vendor != CONFIG_ID_INVALID ||
		conf->id.product != CONFIG_ID_INVALID)
		conf->id.type |= CONFIG_TYPE_VENDOR;
	if (conf->id.class != CONFIG_ID_INVALID ||
		conf->id.subclass != CONFIG_ID_INVALID ||
		conf->id.protocol != CONFIG_ID_INVALID)
		conf->id.type |= CONFIG_TYPE_CLASS;
}

void
write_script(char *path,char *content)
{
	int fd;
	char sep = '\n';

	if ((fd = creat(path,0644)) == -1) {
		syslog(LOG_ERR,error_create,path);
		return;
	}
	if (write(fd,content,strlen(content)) == -1) {
		syslog(LOG_ERR,"Can't write %s",path);
		goto end;
	}
	write(fd,&sep,1);
end:
	close(fd);
}

/*
 * Todo: add error
 */
void
write_config(struct config *conf)
{
	char path[PATH_MAX+1];

	if (conf == NULL)
		return;
	if (conf->id.type & CONFIG_TYPE_VENDOR) {
		sprintf(path,"%s/%04x/%04x/%s",
			USBMGR_VENDOR_DIR,conf->id.vendor,conf->id.product,
			USBMGR_MODULE_FILE);
		if (access(path,W_OK) != 0)
			Mkdir_R(path);
		write_modlink(conf->link.data,path);
		if (conf->script != NULL) {
			strcpy(strrchr(path,'/')+1,USBMGR_SCRIPT_FILE);
			write_script(path,conf->script);
		}
	}
	if (conf->id.type & CONFIG_TYPE_CLASS) {
		if (conf->id.type & CONFIG_TYPE_SUBCLASS) {
			if (conf->id.type & CONFIG_TYPE_PROTOCOL) {
				sprintf(path,"%s/%02x/%02x/%02x/%s",
					USBMGR_CLASS_DIR,
					conf->id.class, conf->id.subclass,
					conf->id.protocol,USBMGR_MODULE_FILE);
			} else {
				sprintf(path,"%s/%02x/%02x/%s",
					USBMGR_CLASS_DIR,
					conf->id.class, conf->id.subclass,
					USBMGR_MODULE_FILE);
			}
		} else {
			sprintf(path,"%s/%02x/%s",
				USBMGR_CLASS_DIR,
				conf->id.class, USBMGR_MODULE_FILE);
		}
		if (access(path,W_OK) != 0)
			Mkdir_R(path);
		write_modlink(conf->link.data,path);
		if (conf->script != NULL) {
			strcpy(strrchr(path,'/')+1,USBMGR_SCRIPT_FILE);
			write_script(path,conf->script);
		}
	}
}


