/*
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: beep.c,v 4.0 2000/12/21 09:50:56 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 *
 *  beep function is based on beep() in cardmgr.c(pcmcia-cs)
 */
#include	<sys/kd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/ioctl.h>
#include	<unistd.h>

#include	"usbmgr.h"

#define	SLEEP_TIME	100
#define	BEEP_GOOD	800
#define	BEEP_ERROR	3200

void
beep(int status)
{
	int fd;
	int freq,btime;

	if (make_beep == 0)
		return;

	if (status == 0) {
		freq = BEEP_GOOD;
		btime = 100;
	} else {
		freq = BEEP_ERROR;
		btime = 300;
	}

	fd = open("/dev/console",O_WRONLY);
	if (fd < 0)
		return;
	ioctl(fd,KDMKTONE,(btime << 16) | freq);
	close(fd);
	usleep(SLEEP_TIME * 1000);	/* to identify each beep sound */
}
