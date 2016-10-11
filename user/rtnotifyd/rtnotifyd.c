/*
 * Copyright (c) 2014, Internet Initiative Japan, Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <asm/types.h>
#include <errno.h>
#include <linux/netlink.h> 
#include <linux/rtnetlink.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

static int watch(int);
static void logit(int, const char *, ...);
static void touch(void);

int daemonize = 1;
const char *touch_file = NULL;
const char *pid_file = NULL;

static void
usage(void)
{
	printf("Usage: rtnotifyd [options]\n"
	       "\n"
	       "Options:\n"
	       "-f		run foreground\n"
	       "-t <path>	touch file if address changed\n"
	       "-p <path>	pid file\n"
	);
	exit(1);
}

int
main(int argc, char *argv[])
{
	int ch;
	int rtsock;
	struct sockaddr_nl sa;

	while ((ch = getopt(argc, argv, "ft:p:h")) != -1) {
		switch(ch) {
		case 'f':
			daemonize = 0;
			break;
		case 't':
			touch_file = optarg;
			break;
		case 'p':
			pid_file = optarg;
			break;
		case 'h':
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (daemonize) {
		openlog("rtnotifyd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
		if (daemon(0, 0) < 0) {
			logit(LOG_ERR, "daemon() failed: %s", strerror(errno));
			exit(1);
		}
	}

	if (pid_file) {
		FILE *fp;
		if ((fp = fopen(pid_file, "w")) == NULL)
			logit(LOG_ERR, "cannot create pidfile: %s: %s\n",
			    pid_file, strerror(errno));
		else {
			fprintf(fp, "%lu\n", (unsigned long)getpid());
			fclose(fp);
		}
	}
		

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

	if ((rtsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) < 0) {
		logit(LOG_ERR, "failed to create socket: %s", strerror(errno));
		return -1;
	}
	if (bind(rtsock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		logit(LOG_ERR, "failed to bind socket: %s", strerror(errno));
		return -1;
	}

	watch(rtsock);

	if (pid_file)
		unlink(pid_file);
	return 0;
}

static int
watch(int rtsock)
{
	int len;
	struct nlmsghdr *nh;
	char buf [4096];
	struct iovec iov = {buf, sizeof(buf)};
	struct sockaddr_nl nladdr;
	struct msghdr msg = {
		&nladdr, sizeof(nladdr), &iov, 1, NULL, 0, 0
	};

	logit(LOG_INFO, "waiting for netlink message");

	while(1) {
		len = recvmsg(rtsock, &msg, 0);
		if(len < 0) {
			if (errno == EINTR || errno == EAGAIN ||
			    errno == ENOBUFS)
				continue;
			logit(LOG_ERR, "recvmsg() returned error: %s(%d)\n",
			    strerror(errno), errno);
			return -1;
		} else if (len == 0) {
			logit(LOG_ERR, "reached EOF");
			return -1;
		}
	
		for(nh = (struct nlmsghdr *)buf; NLMSG_OK(nh, len);
		    nh = NLMSG_NEXT(nh, len)) {
			if(nh->nlmsg_type == RTM_NEWADDR ||
			   nh->nlmsg_type == RTM_DELADDR) {
				logit(LOG_INFO, "detect address changed");
				touch();
			} else {
				logit(LOG_WARNING,
				    "unknown routing message: %d",
				    nh->nlmsg_type);
				continue;
			}
		}
	}
	return -1;
}

static void
touch(void)
{
	int fd;

	if (touch_file == NULL) {
		logit(LOG_WARNING, "file path is empty");
		return;
	}

	fd = open(touch_file, O_WRONLY | O_CREAT,
	    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if  (fd < 0 || close(fd))
		logit(LOG_ERR, "failed to touch file: %s(%d)",
		    strerror(errno), errno);
}

static const char *
log_priority_string(int priority)
{
	switch (priority) {
	case LOG_EMERG:         return "EMERG";
	case LOG_ERR:           return "ERROR";
	case LOG_WARNING:       return "WARN ";
	case LOG_INFO:          return "INFO ";
	case LOG_DEBUG:         return "DEBUG";
	default:                return "?????";
	}
}

static void
logit(int priority, const char *msg, ...)
{
	va_list ap, ap_cp;
	struct tm *tm;
	time_t now;
	char timebuf[20];

	va_start(ap, msg);
	va_copy(ap_cp, ap);

	if (daemonize)
		vsyslog(priority, msg, ap);
	else {
		time(&now);
		tm = localtime(&now);
		strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S", tm);
		fprintf(stderr, "%s [%u] %s ",
			timebuf,
			(unsigned int)getpid(),
			log_priority_string(priority));
		vfprintf(stderr, msg, ap);
		fputc('\n', stderr);
	}
	va_end(ap_cp);
	va_end(ap);
}
