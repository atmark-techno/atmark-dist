/*
 *  update_usbdb  --- update USB DataBase
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: update_usbdb.c,v 4.1 2001/01/04 15:22:05 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<syslog.h>
#include	<stdlib.h>

#include	"update_usbdb.h"

#define		UPDATE_SIGN_FILE		".update"

char *error_create = "Can't create file %s";

static	char *update_sign = UPDATE_SIGN_FILE;
static char *conf_dir = CONF_DIR ;

int debug = 0;
int force = 0;
int syslog_on = 0;

static void
usage(void){
	syslog(LOG_INFO,"Usage:update_usbdb [-f][-d] config_file\n\t-f:force\n\t-d:debug");
}

int
main(int argc,char **argv)
{
	struct stat st_update,st_conf;
	int opt;
	char *fname;

	while((opt = getopt(argc,argv,"dfhs")) != EOF) {
		switch(opt){
		case 'd':
			debug = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'h':
			usage();
			exit(0);
		case 's':
			syslog_on = 1;
			break;
		}
	}
	if (!syslog_on)
		openlog("update_usbdb",LOG_PERROR,LOG_USER);
	if (argc != optind + 1) {
		usage();
		exit(1);
	}
	fname = argv[optind];
	if (chdir(conf_dir) == -1) {
		syslog(LOG_ERR,"Can't change directory %s",conf_dir);
		exit(1);
	}
	if (stat(fname,&st_conf) == -1) {
		syslog(LOG_ERR,"Can't open file %s",fname);
		exit(1);
	}
	/* need to update DB ? */
	if (!force && stat(update_sign,&st_update) == 0 &&
		st_conf.st_mtime <= st_update.st_mtime) {
		exit(0);	/* need not */
	}
	if (creat(update_sign,0644) == -1) {
		syslog(LOG_ERR,error_create,update_sign);
		exit(1);
	}
	if (load_config(fname) == -1) {
		syslog(LOG_ERR,"configuration error %s",fname);
		exit(1);
	}

	return 0;
}
