/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: usbmgr.h,v 4.6 2001/01/04 14:49:41 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#ifndef	__USBMGR_H
#define	__USBMGR_H

#include	<sys/types.h>
#include	<syslog.h>

#include	"node.h"
#include	"common.h"

#define	GOOD	0
#define	INVALID	-1

#ifdef  DEBUG
#define DPRINTF( args... )  syslog(args)
#else
#define DPRINTF( args... )
#endif

typedef unsigned char __u8;
typedef unsigned short __u16;

struct usbmgr_device {
	__u8  bDeviceClass;
	__u8  bDeviceSubClass;
	__u8  bDeviceProtocol;
	__u16 idVendor;
	__u16 idProduct;
	__u8 type;		/* added */
	__u8 status;	/* added */
	__u8 bus;	/* added */
	__u8 devno;	/* added */
};

#ifndef	USB_PROCDIR
#define	USB_PROCDIR		"/proc/bus/usb"
#endif
#define	USB_FILE_PID		"usbmgr.pid"
#define	USB_DIR_PID		"/var/run"
#define	USB_NOMODULE		"none"
#define	USB_DEVICE_MAX		127

#define	MODULE_LOAD			1
#define	MODULE_UNLOAD		(1<<1)
#define	SCRIPT_START		(1<<2)
#define	SCRIPT_STOP			(1<<3)
#define	MODULE_PRELOAD		(1<<4)  /* can add to MODULE_[UN]LOAD */

#define	USB_MOUNT		1
#define	USB_UNMOUNT		2

#define	USBMGR_DO_AUTO		0
#define	USBMGR_DO_FORCE		1

/* For device */
#define	NODE2DEV(node)		((struct usbmgr_device *)node->data)
#define	DEVICE_STATUS(dev)	((dev)->status)
#define	DEVICE_TYPE(dev)	((dev)->type)
#define	DEVICE_VENDOR(dev)	((dev)->idVendor)
#define	DEVICE_PRODUCT(dev)	((dev)->idProduct)
#define	DEVICE_CLASS(dev)	((dev)->bDeviceClass)
#define	DEVICE_SUBCLASS(dev)	((dev)->bDeviceSubClass)
#define	DEVICE_PROTOCOL(dev)	((dev)->bDeviceProtocol)
#define	DEVICE_BUS(dev)		((dev)->bus)
#define	DEVICE_DEVNO(dev)	((dev)->devno)

#define	NODE_STATUS(node)	DEVICE_STATUS(NODE2DEV(node))
#define	NODE_TYPE(node)		DEVICE_TYPE(NODE2DEV(node))
#define	NODE_VENDOR(node)	DEVICE_VENDOR(NODE2DEV(node))
#define	NODE_PRODUCT(node)	DEVICE_PRODUCT(NODE2DEV(node))
#define	NODE_CLASS(node)	DEVICE_CLASS(NODE2DEV(node))
#define	NODE_SUBCLASS(node)	DEVICE_SUBCLASS(NODE2DEV(node))
#define	NODE_PROTOCOL(node)	DEVICE_PROTOCOL(NODE2DEV(node))
#define	NODE_BUS(node)		DEVICE_BUS(NODE2DEV(node))
#define	NODE_DEVNO(node)	DEVICE_DEVNO(NODE2DEV(node))
/* class */
#define	CLASS_NONE		0
/* vendor */
#define	VENDOR_NONE		0
/* status */
#define	USBMGR_ST_UNKNOWN	0
#define	USBMGR_ST_ACTIVE	1
/* type */
#define	USBMGR_TYPE_NONE	0
#define	USBMGR_TYPE_SCRIPT	0x1
#define USBMGR_TYPE_VENDOR	0x2
#define USBMGR_TYPE_CLASS	0x4

/* for build_path() */
#define	BUILD_DIR		0x1
#define	BUILD_MODULE	0x2
#define	BUILD_SCRIPT	0x4
#define	BUILD_VENDOR	0x8
#define	BUILD_CLASS		0x10

/* message */
#define	MSG_START	0
#define	MSG_CANT_CD	1
#define	MSG_CANT_PID	2
#define	MSG_FORK_ERR	3
#define	MSG_EXEC_ERR	4
#define	MSG_NOT_LOAD	5
#define	MSG_CANT_GET_LD	6
#define	MSG_DID		7
#define	MSG_TOO_LONG	8
#define	MSG_TRY_LD	9
#define	MSG_STOP	10
#define	MSG_NOCHANGE	11
#define	MSG_NEWDEVICE	12
#define	MSG_NOT_MATCH	13
#define	MSG_MATCH	14
#define	MSG_OPEN_ERR	15
#define	MSG_ALLOC_ERR	16
#define	MSG_READ_ERR	17
#define	MSG_BYE			18
#define	MSG_VENDOR_S	19
#define	MSG_CLASS_S		20
#define	MSG_2STRING		21

extern char *conf_dir;
extern char *vendor_dir;
extern char *class_dir;
extern char *host_module;
extern int debug;
extern int sleep_time;
extern int make_beep;
extern char *mesg[];
extern struct node * netdev_ring;
extern struct node * netdev_ring2;

extern struct node * create_device(struct usbmgr_device *);
extern void observ_devices(char *,ssize_t);
extern void delete_device(int);
extern struct node * find_device(struct usbmgr_device *);
extern void print_device(struct node *);
extern void do_mount(int );
extern int search_class_file(struct node * ,char *,int );
extern int load_from_file(struct node *,char *,int );
extern void observe(void);
extern int check_vendor_file(struct node * ,char *,int );
extern void validate_desc(struct node *);
extern int get_netdev(struct node ** );

#ifndef	NOT_EXPAND_MACRO
#define	find_string_netdev(ring,name)	find_string_node((ring),(name))
#endif	/* NOT_EXPAND_MACRO */

#endif	/* __USBMGR_H */
