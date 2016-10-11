/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: update_usbdb.h,v 4.2 2001/01/04 14:46:29 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#ifndef	UPDATE_USBDB_H
#define	UPDATE_USBDB_H

#include	"node.h"
#include	"common.h"

struct config_id {
	__uint16_t	vendor;			/* vendor ID */
	__uint16_t	product;		/* product ID */
	__uint8_t	class;			/* class number */
	__uint8_t	subclass;		/* subclass number */
	__uint8_t	protocol;		/* protocol number */
	__uint8_t	type;			/* available ID type */
};

struct config {
	struct node link;
	struct config_id id;
	char *script;				/* starting executable file */
};

#define	USBMGR_HOST_FILE	"host"

#define CONFIG_TYPE_NONE        0
#define CONFIG_TYPE_VENDOR      1
#define CONFIG_TYPE_PRODUCT     (1<<1)
#define CONFIG_TYPE_CLASS       (1<<2)
#define CONFIG_TYPE_SUBCLASS    (1<<3)
#define CONFIG_TYPE_PROTOCOL    (1<<4)

extern int debug;
extern int syslog_on;
extern char *error_create;
extern struct node * create_modlink(struct node **,struct node *);
extern void write_modlink(struct node *,char *);
extern void write_script(char *,char *);
extern struct config * create_config(void);
extern void link_config(struct config *);
extern int load_config(char *);
extern void write_config(struct config *);
extern void Mkdir_R(char *);

#ifndef	NOT_EXPAND_MACRO
#define	create_modlink(base,module)	create_node_data_link((base),(module),0)
#endif

#endif	/* UPDATE_USBDB_H */
