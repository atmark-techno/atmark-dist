/*
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: usbmgr_msg.c,v 4.3 2000/12/24 15:47:58 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */

#include	<stdio.h>

/****** message *******/
char *mesg[] = {
/* 0 */ "start %s",
"Can't change directory %s",
"Can't create pid file",
"fork error",
"exec error %s",
/* 5 */ "\"%s\" isn't loaded",
"Can't get module loader",
"\"%s\" was %s",
"script path is too long",
"trying to %s \"%s\"",
/* 10 */ "stop %s",
"no change",
"new device",
"USB device isn't matched the configuration",
"USB device is matched the configuration",
/* 15 */ "open error \"%s\"",
"allocate error",
"read error \"%s\"",
"bye!",
"vendor:0x%x product:0x%x",
/* 20 */ "class:0x%x subclass:0x%x protocol:0x%x",
"%s %s",
NULL};
