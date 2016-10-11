/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: dump_usbdev.c,v 4.0 2001/01/04 15:20:33 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<unistd.h>
#include	<limits.h>
#include	<string.h>
#include	<dirent.h>

#include	"version.h"

typedef unsigned char __u8;
typedef unsigned short __u16;

struct usb_device_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u16 bcdUSB;
	__u8  bDeviceClass;
	__u8  bDeviceSubClass;
	__u8  bDeviceProtocol;
	__u8  bMaxPacketSize0;
	__u16 idVendor;
	__u16 idProduct;
	__u16 bcdDevice;
	__u8  iManufacturer;
	__u8  iProduct;
	__u8  iSerialNumber;
	__u8  bNumConfigurations;
};

#ifndef USB_PROCDIR
#define USB_PROCDIR     "/proc/bus/usb"
#endif

int flag_v = 0;
int flag_d = 0;
int flag_s = 0;

#define USB_TYPE_CLASS          (0x01 << 5)
/*
 * Descriptor types
 */
#define USB_DT_DEVICE           0x01
#define USB_DT_CONFIG           0x02
#define USB_DT_STRING           0x03
#define USB_DT_INTERFACE        0x04
#define USB_DT_ENDPOINT         0x05

#define USB_DT_HID          (USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT           (USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL         (USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB          (USB_TYPE_CLASS | 0x09)

void
print_length(__u8 len)
{
	printf("data size %d\n",len);
}

void
print_desc_type(__u8 dt)
{
	switch(dt) {
	case USB_DT_DEVICE:	
		printf("device");
		break;
	case USB_DT_CONFIG:
		printf("config");
		break;
	case USB_DT_STRING:
		printf("string");
		break;
	case USB_DT_INTERFACE:
		printf("interface");
		break;
	case USB_DT_ENDPOINT:
		printf("endpoint");
		break;
	case USB_DT_HID:
		printf("HID");
		break;
	case USB_DT_REPORT:
		printf("report");
		break;
	case USB_DT_PHYSICAL:
		printf("physical");
		break;
	case USB_DT_HUB:
		printf("hub");
		break;
	}
	putchar('\n');
}
	
void
print_version(__u16 ver)
{
	printf("version %x.%02x\n",ver >> 8, ver &0xff);
}

/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE     0   /* for DeviceClass */
#define USB_CLASS_AUDIO         1
#define USB_CLASS_COMM          2
#define USB_CLASS_HID           3
#define USB_CLASS_PRINTER       7
#define USB_CLASS_MASS_STORAGE      8
#define USB_CLASS_HUB           9
#define USB_CLASS_DATA          10
#define USB_CLASS_VENDOR_SPEC       0xff

void
print_class(__u8 class)
{
	switch(class) {
	case USB_CLASS_PER_INTERFACE:	
		printf("device");
		break;
	case USB_CLASS_AUDIO:
		printf("audio");
		break;
	case USB_CLASS_COMM:
		printf("comm");
		break;
	case USB_CLASS_HID:
		printf("HID");
		break;
	case USB_CLASS_PRINTER:
		printf("printer");
		break;
	case USB_CLASS_MASS_STORAGE:
		printf("mass storage");
		break;
	case USB_CLASS_HUB:
		printf("hub");
		break;
	case USB_CLASS_DATA:
		printf("data");
		break;
	case USB_CLASS_VENDOR_SPEC:
		printf("vendor spec");
		break;
	}
	putchar('\n');
}

void
dump_proc_nnn(char *nnn)
{
	struct usb_device_descriptor dev;
	int fd;

	if((fd = open(nnn,O_RDONLY)) == -1)
		return;
	if(read(fd,&dev,sizeof(struct usb_device_descriptor)) <= 0)
		return;
	close(fd);

	printf("bLength = %u\n",dev.bLength);
	if (flag_v)
		print_length(dev.bLength);

	printf("bDescriptorType = 0x%x\n",dev.bDescriptorType);
	if (flag_v)
		print_desc_type(dev.bDescriptorType);

	printf("bcdUSB = 0x%x\n",dev.bcdUSB);
	if (flag_v)
		print_version(dev.bcdUSB);

	printf("bDeviceClass = 0x%x\n",dev.bDeviceClass);
	if (flag_v)
		print_class(dev.bDeviceClass);

	printf("bDeviceSubClass = 0x%x\n",dev.bDeviceSubClass);
	printf("bDeviceProtocol = 0x%x\n",dev.bDeviceProtocol);
	printf("bMaxPacketSize0 = %u\n",dev.bMaxPacketSize0);
	printf("idVendor = 0x%x\n",dev.idVendor);
	printf("idProduct = 0x%x\n",dev.idProduct);
	printf("bcdDevice = 0x%x\n",dev.bcdDevice);
	if (flag_v)
		print_version(dev.bcdDevice);
	printf("iManufacturer = 0x%x\n",dev.iManufacturer);
	printf("iProduct = 0x%x\n",dev.iProduct);
	printf("iSerialNumber = 0x%x\n",dev.iSerialNumber);
	printf("bNumConfigurations = 0x%x\n",dev.bNumConfigurations);
}

void
dump_config_nnn(char *nnn)
{
	struct usb_device_descriptor dev;
	int fd;
	static int device_count = 0;

	if((fd = open(nnn,O_RDONLY)) == -1)
		return;
	if(read(fd,&dev,sizeof(struct usb_device_descriptor)) <= 0)
		return;
	close(fd);

	if (dev.idVendor != 0) {
		if (flag_s) {
			printf("%03d 0x%x 0x%x\n",
				device_count,dev.idVendor,dev.idProduct);
		} else {
			printf("vendor 0x%x product 0x%x module <module_name>\n",
				dev.idVendor,dev.idProduct);
		}
	}
	if (dev.bDeviceClass != 0 && (flag_s || dev.idVendor == 0)) {
		if (flag_s) {
			printf("%03d 0x%x 0x%x 0x%x\n",
				device_count,dev.bDeviceClass,dev.bDeviceSubClass,dev.bDeviceProtocol);
		} else {
			printf("class 0x%x subclass 0x%x protocol 0x%x module <module_name>\n",
				dev.bDeviceClass,dev.bDeviceSubClass,dev.bDeviceProtocol);
		}
	}			
	device_count++;
}

main(int argc,char **argv)
{
	int opt;
	int index;
	char path[PATH_MAX+1];
	DIR *dp;
	struct dirent *de;
	char *usb_dir = USB_PROCDIR "/001";

	while((opt = getopt(argc,argv,"Vvds")) != EOF) {
		switch(opt) {
		case 'V':
			printf("usbmgr: version %s\n",usbmgr_version);
			break;
		case 'v':
			flag_v++;
			break;
		case 'd':
			flag_d++;
			break;
		case 's':
			flag_s++;
			break;
		}
	}
		
		
	if (optind == argc) {	/* auto detect */
		if((dp = opendir(usb_dir)) == NULL) {
			fprintf(stderr,"%s is not mounted\n",USB_PROCDIR);
			exit(1);
		}
		while((de = readdir(dp)) != NULL) {
			if (strlen(de->d_name) != 3 || 
				!isdigit(de->d_name[0]) ||
				!isdigit(de->d_name[1]) ||
				!isdigit(de->d_name[2]))
				continue;
			sprintf(path,"%s/%s",usb_dir,de->d_name);
			if (flag_d)
				dump_proc_nnn(path);
			else
				dump_config_nnn(path);
		}
		closedir(dp);
	} else {
		char *pathp;

		for (; optind < argc;optind++) {
			pathp = argv[optind];
			if (strchr(argv[optind],'/') == NULL) {
				sprintf(path,"%s/001/%s",USB_PROCDIR,argv[optind]);
				pathp = path;
			}
			if (flag_d)
				dump_proc_nnn(pathp);
			else
				dump_config_nnn(pathp);
		}
	}
}
	

