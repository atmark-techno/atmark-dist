/*
 * Copyright 2001 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"
RCSID("$OpenBSD: monitor_fdpass.c,v 1.6 2004/08/13 02:51:48 djm Exp $");

#include <sys/uio.h>
#include <sys/utsname.h>

#include "log.h"
#include "monitor_fdpass.h"

static int
cmsg_type_is_broken(void)
{
	static int broken_cmsg_type = -1;

	if (broken_cmsg_type != -1)
		return broken_cmsg_type;
	else {
		struct utsname uts;
		/* If uname() fails, play safe and assume that cmsg_type
		 * isn't broken.
		 */
		if (!uname(&uts) &&
		    strcmp(uts.sysname, "Linux") == 0 &&
		    strncmp(uts.release, "2.0.", 4) == 0)
			broken_cmsg_type = 1;
		else
			broken_cmsg_type = 0;
	}

	return broken_cmsg_type;
}

void
mm_send_fd(int sock, int fd)
{
#if defined(HAVE_SENDMSG) && (defined(HAVE_ACCRIGHTS_IN_MSGHDR) || defined(HAVE_CONTROL_IN_MSGHDR))
	struct msghdr msg;
	struct iovec vec;
	char ch = '\0';
	ssize_t n;
#ifndef HAVE_ACCRIGHTS_IN_MSGHDR
	char tmp[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;
#endif

	memset(&msg, 0, sizeof(msg));
#ifdef HAVE_ACCRIGHTS_IN_MSGHDR
	msg.msg_accrights = (caddr_t)&fd;
	msg.msg_accrightslen = sizeof(fd);
#else
	msg.msg_control = (caddr_t)tmp;
	msg.msg_controllen = CMSG_LEN(sizeof(int));
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*(int *)CMSG_DATA(cmsg) = fd;
#endif

	vec.iov_base = &ch;
	vec.iov_len = 1;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;

	if ((n = sendmsg(sock, &msg, 0)) == -1)
		fatal("%s: sendmsg(%d): %s", __func__, fd,
		    strerror(errno));
	if (n != 1)
		fatal("%s: sendmsg: expected sent 1 got %ld",
		    __func__, (long)n);
#else
	fatal("%s: UsePrivilegeSeparation=yes not supported",
	    __func__);
#endif
}

int
mm_receive_fd(int sock)
{
#if defined(HAVE_RECVMSG) && (defined(HAVE_ACCRIGHTS_IN_MSGHDR) || defined(HAVE_CONTROL_IN_MSGHDR))
	struct msghdr msg;
	struct iovec vec;
	ssize_t n;
	char ch;
	int fd;
#ifndef HAVE_ACCRIGHTS_IN_MSGHDR
	char tmp[CMSG_SPACE(sizeof(int))];
	struct cmsghdr *cmsg;
#endif

	memset(&msg, 0, sizeof(msg));
	vec.iov_base = &ch;
	vec.iov_len = 1;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
#ifdef HAVE_ACCRIGHTS_IN_MSGHDR
	msg.msg_accrights = (caddr_t)&fd;
	msg.msg_accrightslen = sizeof(fd);
#else
	msg.msg_control = tmp;
	msg.msg_controllen = sizeof(tmp);
#endif

	if ((n = recvmsg(sock, &msg, 0)) == -1)
		fatal("%s: recvmsg: %s", __func__, strerror(errno));
	if (n != 1)
		fatal("%s: recvmsg: expected received 1 got %ld",
		    __func__, (long)n);

#ifdef HAVE_ACCRIGHTS_IN_MSGHDR
	if (msg.msg_accrightslen != sizeof(fd))
		fatal("%s: no fd", __func__);
#else
	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL)
		fatal("%s: no message header", __func__);
	if (!cmsg_type_is_broken() && cmsg->cmsg_type != SCM_RIGHTS)
		fatal("%s: expected type %d got %d", __func__,
		    SCM_RIGHTS, cmsg->cmsg_type);
	fd = (*(int *)CMSG_DATA(cmsg));
#endif
	return fd;
#else
	fatal("%s: UsePrivilegeSeparation=yes not supported",
	    __func__);
#endif
}
