/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: parse_config.c,v 4.0 2000/12/21 09:58:28 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>	/* TODO:delete          for stderr */
#include	<unistd.h>
#include	<sys/stat.h>
#include	<sys/mman.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<string.h>
#include	<syslog.h>
#include	<stdlib.h>

#include	"update_usbdb.h"

/*
 * for make_nobeep()
 */
#define	MAKE	0
#define	REMOVE	1

#define	TOKEN_SIZE(ID)	(sizeof(USB_ ## ID) - 1)
#define	TOKEN_MEMBER(ID)	{USB_ ## ID, TOKEN_SIZE(ID)}
#define	TOKEN_MEMBER_END	{NULL, -1}

/* token in config file */
#define	USB_HOST		"host"
#define	USB_VENDOR		"vendor"
#define	USB_PRODUCT		"product"
#define	USB_CLASS		"class"
#define	USB_SUBCLASS	"subclass"
#define	USB_PROTOCOL	"protocol"
#define	USB_MODULE		"module"	/* must be last token in statement */
#define	USB_CONNECT		","			/* must be one character */
#define	USB_SCRIPT		"script"
#define	USB_BEEP		"beep"

#define	USB_HOST_ID		0
#define	USB_VENDOR_ID	1
#define	USB_PRODUCT_ID	2
#define	USB_CLASS_ID	3
#define	USB_SUBCLASS_ID	4
#define	USB_PROTOCOL_ID	5
#define	USB_MODULE_ID	6
#define	USB_CONNECT_ID	7
#define	USB_SCRIPT_ID	8
#define	USB_BEEP_ID		9

struct config_token {
	char *name;
	int size;
};

static struct config_token token[] = {
	TOKEN_MEMBER(HOST),
	TOKEN_MEMBER(VENDOR),
	TOKEN_MEMBER(PRODUCT),
	TOKEN_MEMBER(CLASS),
	TOKEN_MEMBER(SUBCLASS),
	TOKEN_MEMBER(PROTOCOL),
	TOKEN_MEMBER(MODULE),
	TOKEN_MEMBER(CONNECT),
	TOKEN_MEMBER(SCRIPT),
	TOKEN_MEMBER(BEEP),
	TOKEN_MEMBER_END
};

/* points to the position which is invalid statement in configuration file */
static char *error_ptr;		
static void write_host(char *);
static int make_nobeep(char *,int );

/*
 * search a token
 *
 * [return]
 *	index from the founded token[]
 *  -1: find no token
 *   *: index of token[]
 */ 
static int
find_token(char *ptr,char *endp)
{
	int i,left;

	left = endp - ptr;
	if (left <= 0)
		return -1;
	if (*ptr == *USB_CONNECT)		/* compare characters */
		return USB_CONNECT_ID;
	for(i = 0;token[i].name != NULL;i++) {
		if(left >= token[i].size &&
			!strncmp(ptr, token[i].name, token[i].size) &&
			isspace(*(ptr+token[i].size)))
			return i;
	}
	return -1;
}

/*
 * [return]
 * 	length of token which have no '\0'
 *  0: no token
 */
static int
find_arg(char *ptr,char *endp)
{
	char *ptr_orig;

	ptr_orig = ptr;
	for(;ptr < endp;ptr++) {
		if (isspace(*ptr) || *ptr == *USB_CONNECT)	/* arg1,<any> */
			return ptr - ptr_orig;
	}
	return 0;
}

#define		ST_NEW		1		/* new statement */
#define		ST_END		2		/* statement end */

static int
parse_token(char *ptr,char *endp)
{
	int id = -1,prev_id = -1;
	int get_arg_flag = 0;
	int status = ST_END;
	struct config *conf = NULL;

	while(ptr < endp) {
		/* delete white space */
		if(isspace(*ptr)) {
			ptr++;
			continue;
		}
		/* delete comments */
		if (*ptr == '#') {
			while(ptr < endp) {
				if (*ptr == '\n')
					break;
				ptr++;
			}
			continue;
		}
		if (!get_arg_flag) {
			if ((id = find_token(ptr,endp)) == -1) {
				error_ptr = ptr;
				return -1;
			}
			if (debug > 1)
				syslog(LOG_DEBUG,"real token \"%s\"",token[id].name);
			get_arg_flag = 1;
			ptr += token[id].size;
			if (id == USB_CONNECT_ID) {
				if (prev_id != USB_MODULE_ID)
					return -1;
				id = prev_id;
			}
			if (id == USB_MODULE_ID) {	/* config statement end ? */
				status = ST_END;
			} else {
				if(status == ST_END) {
					write_config(conf);
					conf = create_config();
					if (conf == NULL)
						return -1;
					link_config(conf);	/* link to config_ring */
				}
				status = ST_NEW;
			}
			if (debug > 1)
				syslog(LOG_DEBUG, "token \"%s\"",token[id].name);
		} else {	/* get argument of the founded token */
			int len;
			char *arg;
			struct node *mod;

			if ((len = find_arg(ptr,endp)) == 0) {
				error_ptr = ptr;
				return -1;	/* no argument */
			}
			if ((arg = (char *)Malloc(len+1)) == NULL)
				return -1;
			strncpy(arg,ptr,len);
			arg[len] = '\0';
			if (debug > 1)
				syslog(LOG_DEBUG,"arg \"%s\"",arg);
			switch(id) {
			case USB_HOST_ID:
				write_host(arg);	/* write host module name on "host" */
				break;
			case USB_VENDOR_ID:
				conf->id.vendor = (__uint16_t)strtoul(arg,NULL,0);
				conf->id.type |= CONFIG_TYPE_VENDOR;
				break;
			case USB_PRODUCT_ID:
				conf->id.product = (__uint16_t)strtoul(arg,NULL,0);
				conf->id.type |= CONFIG_TYPE_PRODUCT;
				break;
			case USB_CLASS_ID:
				conf->id.class = (__uint8_t)strtoul(arg,NULL,0);
				conf->id.type |= CONFIG_TYPE_CLASS;
				break;
			case USB_SUBCLASS_ID:
				conf->id.subclass = (__uint8_t)strtoul(arg,NULL,0);
				conf->id.type |= CONFIG_TYPE_SUBCLASS;
				break;
			case USB_PROTOCOL_ID:
				conf->id.protocol = (__uint8_t)strtoul(arg,NULL,0);
				conf->id.type |= CONFIG_TYPE_PROTOCOL;
				break;
			case USB_MODULE_ID:	/* must be last token */
				if((mod = find_string_module(arg)) == NULL) {
					mod = create_module(arg);
					if (mod == NULL)
						return -1;
				}
				/* config link to module */
				if (create_modlink(&(conf->link.data),mod) == NULL)
					return -1;
				break;
			case USB_SCRIPT_ID:
				if((conf->script = strdup(arg)) == NULL)
					return -1;
				break;
			case USB_BEEP_ID:
				if ((arg[0] == 'o' || arg[0] == 'O') &&
					(arg[1] == 'f' || arg[1] == 'F') &&
					(arg[2] == 'f' || arg[2] == 'F')) {
					make_nobeep(USBMGR_NOBEEP_FILE,MAKE);
				} else {
					make_nobeep(USBMGR_NOBEEP_FILE,REMOVE);
				}
				break;
			}
			Free(arg);
			prev_id = id;
			ptr += len;
			get_arg_flag = 0;
		}
	}	
	write_config(conf);

	return 0;
}

/*
 * Function: load_config
 * [input]
 *	fname: configuration file name
 * [output]
 *   0: success
 *	-1: error
 */
#define	ERRBUF_SIZE	32
int
load_config(char *fname)
{
	struct stat st;
	int fd;
	char *mp;
	char errbuf[ERRBUF_SIZE+1];

	if ((fd = open(fname,O_RDONLY)) == -1)
		return -1;
	if(fstat(fd,&st) == -1)
		return -1;
	
	if((mp = (char *)mmap(0,st.st_size,PROT_READ,MAP_SHARED,fd,0)) == MAP_FAILED)
		return -1;
	close(fd);

	if(parse_token(mp, mp+st.st_size) == -1) {
		if (error_ptr != NULL) {
			int left;

			left = st.st_size - (error_ptr - mp);
			syslog(LOG_ERR,"Syntax error near %d bytes offset in configuration file",error_ptr - mp);
			if (ERRBUF_SIZE < left) {
				left = ERRBUF_SIZE;
			}
			memcpy(errbuf,error_ptr,left);
			errbuf[left] = '\0';
			syslog(LOG_ERR,"\"%s\"",errbuf);
		}
		return -1;
	}

	return munmap(mp,st.st_size);
}

static void
write_host(char *host)
{
	int fd;
	char sep = '\n';
	static int first = 1;
	int open_flag;

	if (first) {
		open_flag = O_WRONLY|O_CREAT|O_TRUNC;
		first = 0;
	} else {
		open_flag = O_WRONLY|O_APPEND;
	}

	if ((fd = open(USBMGR_HOST_FILE,open_flag,0644)) == -1) {
		syslog(LOG_ERR,error_create,USBMGR_HOST_FILE);
		return;
	}
	if (write(fd,host,strlen(host)) == -1) {
		syslog(LOG_ERR,"Can't write %s",USBMGR_HOST_FILE);
		goto end;
	}
	write(fd,&sep,1);
end:
	close(fd);
}

static int
make_nobeep(char *fname,int action)
{
	int fd;

	if (action == MAKE) {
		if ((fd = open(fname,O_WRONLY|O_CREAT|O_TRUNC,0644)) == -1) {
			return -1;
		}
		close(fd);
	} else {/* REMOVE */
		return unlink(fname);
	}
}
