/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: device.c,v 4.2 2001/01/04 14:31:32 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<string.h>
#include	<limits.h>
#include	<linux/limits.h>
#include	<unistd.h>
#include	<stdlib.h>

#include	"usbmgr.h"

static struct node *device_ring = NULL;


/*
 * create usbdevice
 */
#if 0
struct usbdev *
create_device(struct usbmgr_device_descriptor desc)
{
	struct usbdev *dev;

	dev = (struct usbdev *)Malloc(sizeof(struct usbdev));
	if (dev == NULL)
		return NULL;
	INIT_NODE(&(dev->link));
	dev->desc = desc;
	dev->status = USBMGR_ST_UNKNOWN;
	dev->type = USBMGR_TYPE_NONE;
	link_node(&device_ring,&(dev->link));

	return dev;
}
#else
struct node *
create_device(struct usbmgr_device *dev)
{
	struct node *np;

	np = create_node_data_link(&device_ring,dev,sizeof(struct usbmgr_device)); 
	if (np == NULL)
		return NULL;
	NODE_STATUS(np) = USBMGR_ST_UNKNOWN;
	NODE_TYPE(np) = USBMGR_TYPE_NONE;

	return np;
}
#endif

/*
 * search device using config
 */
#if 0
struct usbdev *
find_device(struct usbdev target)
{
	struct usbdev *dev;

	if (device_ring == NULL)
		return NULL;
	for(dev = (struct usbdev *)device_ring; ;dev = (struct usbdev *)dev->link.next) {
		if ((dev->desc.bDeviceClass == target.desc.bDeviceClass) &&
			(dev->desc.bDeviceSubClass == target.desc.bDeviceSubClass) &&
			(dev->desc.bDeviceProtocol == target.desc.bDeviceProtocol) &&
			(dev->desc.idVendor == target.desc.idVendor) &&
			(dev->desc.idProduct == target.desc.idProduct) ) {
			dev->status |= USBMGR_ST_ACTIVE;
			return dev;
		}
		if (dev->link.next == (struct node *)device_ring)
			break;
	}
	return NULL;
}
#else
struct node *
find_device(struct usbmgr_device *dev)
{
	struct node *np;

	if (device_ring == NULL)
		return NULL;
	for (np = device_ring; ;np = np->next) { 
		if ((DEVICE_CLASS(dev) == NODE_CLASS(np)) &&
			(DEVICE_SUBCLASS(dev) == NODE_SUBCLASS(np)) &&
			(DEVICE_PROTOCOL(dev) == NODE_PROTOCOL(np)) &&
			(DEVICE_VENDOR(dev) == NODE_VENDOR(np)) &&
			(DEVICE_PRODUCT(dev) == NODE_PRODUCT(np)) ) {

			NODE_STATUS(np) |= USBMGR_ST_ACTIVE;

			return np;
		}
		if (np->next == device_ring)
			break;
	}
	return NULL;
}
#endif

/*
 * delete device
 */
void
delete_device(int force)
{
	struct node *np,*next,*end;

	if (device_ring == NULL)
		return;
	end = device_ring->prev;
	for(np = device_ring; ;np = next) {
		next = np->next;
		if (force || !(NODE_STATUS(np) & USBMGR_ST_ACTIVE)) {
			char path[PATH_MAX+1];

			if (NODE_TYPE(np) & USBMGR_TYPE_SCRIPT) {
				if (check_vendor_file(np,path,BUILD_SCRIPT) == INVALID) {
					search_class_file(np,path,BUILD_SCRIPT);
				}
				if (load_from_file(np,path,SCRIPT_STOP) == 0)
					syslog(LOG_INFO,mesg[MSG_STOP],path);
			}
			if ((check_vendor_file(np,path,BUILD_MODULE) == INVALID)  &&
				(search_class_file(np,path,BUILD_MODULE) == INVALID)) {
				beep(INVALID);
				DPRINTF(LOG_DEBUG,"not identify device -> beep fail");
			} else {
				beep(GOOD);
				DPRINTF(LOG_DEBUG,"identify device -> beep good");
				load_from_file(np,path,MODULE_UNLOAD);
			}
			delete_node_data_link(&device_ring,np);
		} else {
			NODE_STATUS(np) &= ~USBMGR_ST_ACTIVE;
		}
		if (np == end)
			break;
	}
#ifdef	DEBUG
	if (force)
		printf("device_ring %p\n",device_ring);
#endif
}

#if 0
int
check_vendor_file(struct usbdev dev,char *path,int flag)
{
	char *file;

	if (flag & BUILD_MODULE)
		file = USBMGR_MODULE_FILE;
	else if (flag & BUILD_SCRIPT)
		file = USBMGR_SCRIPT_FILE;
	else /* BUILD_DIR */
		file = "";

	if (!(dev.type & USBMGR_TYPE_VENDOR))
		return -1;
	sprintf(path,"%s/%s/%04x/%04x/%s",
		conf_dir,vendor_dir,dev.desc.idVendor,dev.desc.idProduct,file);
	if (access(path,R_OK) == -1)
		return -1;
	return 0;
}
#else
int
check_vendor_file(struct node *np,char *path,int flag)
{
	char *file;

	if (flag & BUILD_MODULE)
		file = USBMGR_MODULE_FILE;
	else if (flag & BUILD_SCRIPT)
		file = USBMGR_SCRIPT_FILE;
	else /* BUILD_DIR */
		file = "";

	if (!(NODE_TYPE(np) & USBMGR_TYPE_VENDOR))
		return INVALID;
	sprintf(path,"%s/%s/%04x/%04x/%s",
		conf_dir,vendor_dir,
		NODE_VENDOR(np),NODE_PRODUCT(np),file);
	if (access(path,R_OK) == -1)
		return INVALID;
	return GOOD;
}
#endif

#if 0
void
validate_desc(struct usbmgr_device *dev)
{
	dev->type = USBMGR_TYPE_NONE;	
	if (dev->desc.idVendor != 0)
		dev->type |= USBMGR_TYPE_VENDOR;
	if (dev->desc.bDeviceClass != 0) 
		dev->type |= USBMGR_TYPE_CLASS;
}
#else
void
validate_desc(struct node *np)
{
	NODE_TYPE(np) = USBMGR_TYPE_NONE;	
	if (NODE_VENDOR(np) != VENDOR_NONE)
		NODE_TYPE(np) |= USBMGR_TYPE_VENDOR;
	if (NODE_CLASS(np) != CLASS_NONE) 
		NODE_TYPE(np) |= USBMGR_TYPE_CLASS;
}
#endif


void
print_device(struct node *np)
{
	if (NODE_TYPE(np) & USBMGR_TYPE_VENDOR)
		syslog(LOG_INFO,mesg[MSG_VENDOR_S],
			NODE_VENDOR(np),NODE_PRODUCT(np));
	if (NODE_TYPE(np) & USBMGR_TYPE_CLASS)
		syslog(LOG_INFO,mesg[MSG_CLASS_S],
			NODE_CLASS(np),NODE_SUBCLASS(np),NODE_PROTOCOL(np));
}

