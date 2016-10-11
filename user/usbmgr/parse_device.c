/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: parse_device.c,v 4.17 2003/08/17 07:05:08 shuu Exp shuu $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 *
 */
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/poll.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<limits.h>
#include	<linux/limits.h>
#include	<asm/limits.h>
#include	<string.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/wait.h>
#include	<a.out.h>

#include	"usbmgr.h"

extern int poll_fd;	/* main.c */

#ifndef	DEVFILE
#define	DEVFILE		USB_PROCDIR "/devices"
#endif
#ifndef	USBMGR_BLOCK_SIZE
#define	USBMGR_BLOCK_SIZE	PAGE_SIZE		/* unit of allocation */
#endif
#ifndef USBMGR_BLOCK_COUNT
#define	USBMGR_BLOCK_COUNT		3		/* count of units */
#endif
#ifndef	FLUSH_TIME
#define	FLUSH_TIME	(500 * 1000)
#endif

#define	DEVICE_LINE_SIZE	128	/* enough size for a line on devices */
/* 
 * refreshing memory cycle
 * It should be greater then 2.
 */
#define REFRESH_COUNT	8

#define	ST_NONE			0
#define	ST_NEW			1
#define	ST_PRODUCT		(1<<1)
#define	ST_INTERFACE	(1<<2)
#define	ST_ENDPOINT		(1<<3)
#define	ST_TOPOLOGY		(1<<4)
#define	ST_END			-1

/* 
 * using I(interface descriptor) instead of D(Device descriptor)
 */

static void
parse_line(char *linep,int status,struct usbmgr_device *dev)
{
	int i;

	switch(status) {
	case ST_PRODUCT:
		/* P:  Vendor=xxxx ProdID=xxxx Rev=xx.xx */
		linep = strchr(linep,'=');
		linep++;
		DEVICE_VENDOR(dev) = strtoul(linep,NULL,16);
		linep = strchr(linep,'=');
		linep++;
		DEVICE_PRODUCT(dev) = strtoul(linep,NULL,16);
		if (debug)
			syslog(LOG_INFO,"parsing vender=0x%x product=0x%x",DEVICE_VENDOR(dev),DEVICE_PRODUCT(dev));
		break;
	case ST_INTERFACE:	
		/* I:  If#=dd Alt=dd #EPs=dd Cls=xx(sssss) Sub=xx Prot=xx Driver=ssss
	 	 */
		for(i=0;i < 4;i++) {
			linep = strchr(linep,'=');
			linep++;
		}
		DEVICE_CLASS(dev) = strtoul(linep,NULL,16);
		linep++;
		linep = strchr(linep,'=');
		linep++;
		DEVICE_SUBCLASS(dev) = strtoul(linep,NULL,16);
		linep = strchr(linep,'=');
		linep++;
		DEVICE_PROTOCOL(dev) = strtoul(linep,NULL,16);
		if (debug)
			syslog(LOG_INFO,"parsing class=0x%x subclass=0x%x protocol=0x%x",DEVICE_CLASS(dev),DEVICE_SUBCLASS(dev),DEVICE_PROTOCOL(dev));
		break;
	case ST_TOPOLOGY:
		/* T:  Bus=dd Lev=dd Prnt=dd Port=dd Cnt=dd Dev#=ddd Spd=ddd MxCh=dd
		 */
		linep = strchr(linep,'=');
		linep++;
		DEVICE_BUS(dev) = strtoul(linep,NULL,10);
		if (debug)
			syslog(LOG_INFO,"parseing bus=%03d\n", DEVICE_BUS(dev));
		linep = strchr(linep,'#');
		linep += 2;
		DEVICE_DEVNO(dev) = strtoul(linep,NULL,10);
		if (debug)
			syslog(LOG_INFO,"parsing dev#=%03d\n", DEVICE_DEVNO(dev));
		break;
	}
}

static int
parse_devices(char **curp,char *endp,struct usbmgr_device *dev) {
	char line[DEVICE_LINE_SIZE];
	int status = ST_NEW;
	int i = 0;	/* verbose */
	char *ptr;
	int first = 1;

	if (*curp == endp)
		return ST_END;
	ptr = *curp;

	if (*ptr == '\n')	/* for new devices format */
		ptr++;
	if (*ptr != 'T') {
		syslog(LOG_INFO,"invalid buffer line");
		return ST_END;
	}
	for(;ptr < endp;ptr++) {
		if (status == ST_NEW) {
			i=0;
			switch(*ptr) {
			case 'P':
				status = ST_PRODUCT;
				break;
			case 'I':
				status = ST_INTERFACE;
				break;
			case 'T':
				status = ST_TOPOLOGY;
				if (!first)
					goto loop_end;
				first = 0;
				break;
			default:
				status = ST_NONE;
				break;
			}
		} else if (status & (ST_PRODUCT|ST_INTERFACE|ST_TOPOLOGY) ) {
			line[i++] = *ptr;
		}
		/* parse the line */
		if (*ptr == '\n') {
			line[i] = '\0';
			if (status & (ST_PRODUCT|ST_INTERFACE|ST_TOPOLOGY))
				parse_line(line,status,dev);
			status = ST_NEW;
		}
	}
loop_end:
	if (debug)
		syslog(LOG_DEBUG,"left %d",endp - ptr);
	*curp = ptr;

	return 0;
}

void
observ_devices(char *bufp,ssize_t size)
{
	struct node *np;
	struct usbmgr_device dev_tmp;
	char *curp;
	char path[PATH_MAX+1];

	curp = bufp;
	while(parse_devices(&curp,bufp+size-1,&dev_tmp) != ST_END) {
		/*
		 * check if dev is loaded on memory 
		 */
		if (find_device(&dev_tmp) == NULL) {	/* find New device */
			if (debug)
				syslog(LOG_DEBUG,mesg[MSG_NEWDEVICE]);
			np = create_device(&dev_tmp);
			validate_desc(np);
			print_device(np);
			NODE_STATUS(np) |= USBMGR_ST_ACTIVE;
			/* search module file */
			if (check_vendor_file(np,path,BUILD_MODULE) == -1) {
				if (search_class_file(np,path,BUILD_MODULE) == INVALID) {
					beep(INVALID);
					syslog(LOG_INFO,mesg[MSG_NOT_MATCH]);
					continue;
				}
			}
			beep(GOOD);
			syslog(LOG_INFO,mesg[MSG_MATCH]);
			load_from_file(np,path,MODULE_LOAD);

			/* search script file */
			if (check_vendor_file(np,path,BUILD_SCRIPT) == -1) {
				if (search_class_file(np,path,BUILD_SCRIPT) == INVALID) {
					continue;
				}
			}
			NODE_TYPE(np) |= USBMGR_TYPE_SCRIPT;
			if (load_from_file(np,path,SCRIPT_START) == 0)
				syslog(LOG_INFO,mesg[MSG_START],path);
		} else {	/* devices are not changed */
			if (debug)
				syslog(LOG_DEBUG,mesg[MSG_NOCHANGE]);
		}
	}
	delete_device(USBMGR_DO_AUTO);	/* not force */
}

void
observe(void)
{
	struct pollfd pd;
	char *devfile = DEVFILE;
	char *buf,*bufp;
	int i,len;
	int bnum = USBMGR_BLOCK_COUNT;	/* count of block */
	int sum,refresh;

	buf = malloc(bnum * USBMGR_BLOCK_SIZE);
	if (buf == NULL)
		return;
	if ((pd.fd = open(devfile,O_RDONLY)) == -1) {
		load_from_file(NULL,USBMGR_HOST_FILE,MODULE_LOAD);
		do_mount(USB_MOUNT);
		if ((pd.fd = open(devfile,O_RDONLY)) == -1)
			exit(1);
	}
	pd.events = POLLIN;
	poll_fd = pd.fd;	/* global */
	for(refresh = 0;refresh < REFRESH_COUNT;refresh++) {
		poll(&pd,1,sleep_time); 	/* Hey poll! :-) */
#ifdef	WAIT_FLUSH_PROC
		usleep(FLUSH_TIME);	/* wait for flushing proc data */
#endif
		lseek(poll_fd,0,SEEK_SET);
		bufp = buf;
		for(i = 0,sum = 0;(len = read(poll_fd,bufp,USBMGR_BLOCK_SIZE)) != 0;i++) {
			if (i + 1 >= bnum) {	/* filled */
				bnum *= 2;
				buf = realloc(buf,bnum * USBMGR_BLOCK_SIZE);
				if (buf == NULL) {
					syslog(LOG_ERR,mesg[MSG_ALLOC_ERR]);
					goto return_close;	/* error */
				}
			}
			sum += len;
			if (len < USBMGR_BLOCK_SIZE)	/* maybe end of file */
				break;
			bufp = buf + sum;
		}
		observ_devices(buf,sum);
		if (debug)
			syslog(LOG_DEBUG,"------- size %d refresh %d/%d",sum,refresh,REFRESH_COUNT);
	} 

return_close:
	close(poll_fd);
	free(buf);

	return;
}

/*
 * mount -t usbdevfs /proc/bus/usb /proc/bus/usb
 *                    and 
 * umount /proc/bus/usb
 *
 * Don't use mount(2) and umount(2),because /etc/mtab is unnecessary.
 */
void
do_mount(int action)
{
	char *mount_arg[] = {"mount","-t","usbdevfs",USB_PROCDIR,USB_PROCDIR,NULL};
	char *unmount_arg[] = {"umount",USB_PROCDIR,NULL};
	char **mnt_arg;
	static int mnt_flag = 0;	/* verbose */

	DPRINTF(LOG_INFO,"do_mount action %d",action);
	/* usbmgr didn't mount /proc/bus/usb */
	if (action == USB_UNMOUNT && mnt_flag == 0) {
		DPRINTF(LOG_INFO,"do_mount need not to unmount");
		return;
	}

	switch(fork()){
	case 0:		/* child */
		if (action == USB_MOUNT)
			mnt_arg = mount_arg;
		else
			mnt_arg = unmount_arg;
		syslog(LOG_INFO,mesg[MSG_2STRING],mnt_arg[0],USB_PROCDIR);
		execvp(mnt_arg[0],mnt_arg);
		syslog(LOG_ERR,mesg[MSG_EXEC_ERR],mnt_arg[0]);
		exit(1);
		break;
	case -1:	/* error */
		DPRINTF(LOG_INFO,"do_mount fork error");
		break;
	default:	/* parent */
		DPRINTF(LOG_INFO,"do_mount mnt_flag %d",mnt_flag);
		if (action == USB_MOUNT)
			mnt_flag = 1;
		else
			mnt_flag = 0;
		DPRINTF(LOG_INFO,"do_mount mnt_flag -> %d",mnt_flag);
		wait(NULL);
		break;
	}
}

/*
 * search class file
 */
int
search_class_file(struct node *np,char *path,int flag)
{
	char *file;

	if (!(NODE_TYPE(np) & USBMGR_TYPE_CLASS))
		return INVALID;

	if (flag & BUILD_MODULE)
		file = USBMGR_MODULE_FILE;
	else if (flag & BUILD_SCRIPT)
		file = USBMGR_SCRIPT_FILE;
	else
		return INVALID;

	/* class/subclass/protocol/XXX */
	sprintf(path,"%s/%s/%02x/%02x/%02x/%s",
		conf_dir,class_dir,
		NODE_CLASS(np),NODE_SUBCLASS(np),NODE_PROTOCOL(np),file);
	DPRINTF(LOG_DEBUG,"try %s",path);
	if (access(path,R_OK) == 0)
		return GOOD;

	/* class/subclass/XXX */
	sprintf(path,"%s/%s/%02x/%02x/%s",
		conf_dir,class_dir,
		NODE_CLASS(np),NODE_SUBCLASS(np),file);
	DPRINTF(LOG_DEBUG,"try %s",path);
	if (access(path,R_OK) == 0)
		return GOOD;

	/* class/XXX */
	sprintf(path,"%s/%s/%02x/%s",
		conf_dir,class_dir,NODE_CLASS(np), file);
	DPRINTF(LOG_DEBUG,"try %s",path);
	if (access(path,R_OK) == 0)
		return GOOD;

	/* XXX */
	sprintf(path,"%s/%s/%s",
		conf_dir,class_dir, file);
	DPRINTF(LOG_DEBUG,"try %s",path);
	if (access(path,R_OK) == 0)
		return GOOD;
	return -1;
}



