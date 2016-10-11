/*
 * Copyright (c) 2012, Internet Initiative Japan, Inc.
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

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/sysctl.h>
#include <sys/dkstat.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <netinet/in.h>

#include <netdb.h>
#include <unistd.h>
#include <syslog.h>

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libarms.h"

#include "armsd.h"


struct trafficstat {
	time_t t;
	unsigned long long in_octet;
	unsigned long long out_octet;
	unsigned long long in_packet;
	unsigned long long out_packet;
	unsigned long long in_error;
	unsigned long long out_error;
};

long cpu_prev[CPUSTATES];
struct trafficstat traffic_prev[4] = { {0}, {0}, {0}, {0} };

static time_t sys_heartbeat_gettime(void);
static void sys_heartbeat_cpu(arms_context_t *);
static void sys_heartbeat_disk(arms_context_t *);
static void sys_heartbeat_mem(arms_context_t *);
static void sys_heartbeat_traffic(arms_context_t *);

int
sys_ping(const arms_ping_arg_t *arg, struct arms_ping_report *rep)
{
	FILE *fp;
	struct addrinfo hints, *res, *res0;
	int af, n, pkts, success, timeout;
	const char *argv[4 + 3];		/* 3 for safety... */
	char buf[300], *tmpfile;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST;
	n = getaddrinfo(arg->dst, NULL, &hints, &res0);
	if (n) {
		logit(LOG_WARNING, "cannot resolve %s: %s",
			arg->dst, gai_strerror(n));
		return ARMS_ESYSTEM;
	}
	af = AF_UNSPEC;
        for (res = res0; res; res = res->ai_next)
		if (res->ai_family == AF_INET || res->ai_family == AF_INET6) {
			af = res->ai_family;
			break;
		}
	freeaddrinfo(res0);
	if (af == AF_UNSPEC) {
		logit(LOG_WARNING, "no IPv4/IPv6 address: %s", arg->dst);
		return ARMS_ESYSTEM;
	}

	fclose(armsdir_create_tmpfile(&tmpfile));

	snprintf(buf, sizeof(buf), "%s -nq -c %d -s %d %s > %s",
		 (af == AF_INET ? "ping" : "ping6"),
		 arg->count, arg->size, arg->dst, tmpfile);
	logit(LOG_DEBUG, "ping command exec: %s", buf);

	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = buf;
	argv[3] = NULL;
	timeout = dconf_get_int("timeout");
	n = script_exec(argv, timeout);
	if (n == -1) {
		logit(LOG_WARNING, "ping command failed");
		unlink(tmpfile);
		return ARMS_ESYSTEM;
	}

	fp = fopen(tmpfile, "r");
	if (fp == NULL) {
		logit(LOG_ERR, "ping command succeeded but cannot open %s: %s",
			       tmpfile, strerror(errno));
		unlink(tmpfile);
		return ARMS_ESYSTEM;
	}
	n = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		n = sscanf(buf, "%d packets transmitted, %d received",
			   &pkts, &success);
		if (n == 2) {
			rep->success = success;
			rep->failure = pkts - success;
			break;
		}
	}
	fclose(fp);
	unlink(tmpfile);
	if (n == 2)
		return 0;
	else
		return ARMS_ESYSTEM;
}

int
sys_traceroute(const arms_traceroute_arg_t *arg,
	       struct arms_traceroute_info *tr, int trcount)
{
	FILE *fp;
	struct addrinfo hints, *res, *res0;
	int af, hop, n, timeout;
	const char *argv[4 + 3];		/* 3 for safety... */
	char buf[300], host[64], *tmpfile;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST;
	n = getaddrinfo(arg->addr, NULL, &hints, &res0);
	if (n) {
		logit(LOG_WARNING, "cannot resolve %s: %s",
			arg->addr, gai_strerror(n));
		return ARMS_ESYSTEM;
	}
	af = AF_UNSPEC;
        for (res = res0; res; res = res->ai_next)
		if (res->ai_family == AF_INET || res->ai_family == AF_INET6) {
			af = res->ai_family;
			break;
		}
	freeaddrinfo(res0);
	if (af == AF_UNSPEC) {
		logit(LOG_WARNING, "no IPv4/IPv6 address: %s", arg->addr);
		return ARMS_ESYSTEM;
	}

	fclose(armsdir_create_tmpfile(&tmpfile));

	snprintf(buf, sizeof(buf), "%s -nH -m %d -q %d %s > %s",
		 (af == AF_INET ? "traceroute" : "traceroute6"),
		 arg->maxhop, arg->count, arg->addr, tmpfile);
	logit(LOG_DEBUG, "traceroute command exec: %s", buf);

	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = buf;
	argv[3] = NULL;
	timeout = dconf_get_int("timeout");
	n = script_exec(argv, timeout);
	if (n == -1) {
		logit(LOG_WARNING, "traceroute command failed");
		unlink(tmpfile);
		return ARMS_ESYSTEM;
	}

	fp = fopen(tmpfile, "r");
	if (fp == NULL) {
		logit(LOG_ERR, "traceroute command succeeded but cannot open "
			       "%s: %s",
			       tmpfile, strerror(errno));
		unlink(tmpfile);
		return ARMS_ESYSTEM;
	}
	n = 0;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		n = sscanf(buf, " %d %63s", &hop, (char *)&host);
		if (n == 2) {
			if (trcount <= 0)
				break;
			tr->hop = hop;
			snprintf(tr->addr, sizeof(tr->addr), "%s", host);
			tr++;
			trcount--;
		}
	}
	fclose(fp);
	unlink(tmpfile);
	if (n == 2)
		return 0;
	else
		return ARMS_ESYSTEM;
}

static time_t
sys_heartbeat_gettime(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec;
}

static void
sys_heartbeat_cpu(arms_context_t *ctx)
{
	size_t size;
	unsigned long d_idle, d_interrupt, d_user, d_system, d_other, total;
	uint8_t v_idle, v_interrupt, v_user, v_system, v_other;
	uint64_t cpu_cur[CPUSTATES];
	int i;
	int mib[] = { CTL_KERN, KERN_CP_TIME };

	size = sizeof(cpu_cur);
	if (sysctl(mib, 2, cpu_cur, &size, NULL, 0) == -1) {
		logit(LOG_WARNING, "sysctl() failed: %s", strerror(errno));
		return;
	}

	if (cpu_prev[CP_SYS] == 0) {
		for (i = 0; i < CPUSTATES; i++)
			cpu_prev[i] = cpu_cur[i];
		return;
	}

	d_idle      = cpu_cur[CP_IDLE] - cpu_prev[CP_IDLE];
	d_interrupt = cpu_cur[CP_INTR] - cpu_prev[CP_INTR];
	d_user      = cpu_cur[CP_USER] - cpu_prev[CP_USER];
	d_system    = cpu_cur[CP_SYS]  - cpu_prev[CP_SYS];
	d_other     = cpu_cur[CP_NICE] - cpu_prev[CP_NICE];
	total = d_idle + d_interrupt + d_user + d_system + d_other;

	v_interrupt = d_interrupt * 100 / total;
	v_user = d_user * 100 / total;
	v_system = d_system * 100 / total;
	v_other = d_other * 100 / total;
	v_idle = 100 - v_interrupt - v_user - v_system - v_other;

	if (!opt_hbdebug)
		arms_hb_set_cpu_detail_usage(ctx, 0, v_idle, v_interrupt,
			v_user, v_system, v_other);
	if (opt_verbose)
		logit(LOG_INFO, "heartbeat cpu_detail_usage: idx=%u, idle=%u, "
		      "interrupt=%u, user=%u, system=%u, other=%u",
		      0, v_idle, v_interrupt, v_user, v_system, v_other);

	if (!opt_hbdebug)
		arms_hb_set_cpu_usage(ctx, 0, 100 - v_idle);

	for (i = 0; i < CPUSTATES; i++)
		cpu_prev[i] = cpu_cur[i];
}

static void
sys_heartbeat_disk(arms_context_t *ctx)
{
	struct statvfs fs;
	uint64_t v_used, v_free;
	int fsidx;
	char var[15];
	const char *path;

	for (fsidx = 0; fsidx < 3; fsidx++) {
		snprintf(var, sizeof(var), "hb-disk-usage%d", fsidx);
		path = dconf_get_string(var);
		if (path == NULL || path[0] == '\0')
			continue;

		if (statvfs(path, &fs) == -1) {
			logit(LOG_WARNING, "statvfs(\"%s\") failed: %s",
					   path, strerror(errno));
			return;
		}

		v_used = (uint64_t)fs.f_frsize * (fs.f_blocks - fs.f_bfree);
		v_free = (uint64_t)fs.f_frsize * fs.f_bavail;

		if (!opt_hbdebug)
			arms_hb_set_disk_usage(ctx, fsidx, v_used, v_free);
		if (opt_verbose)
			logit(LOG_INFO, "heartbeat disk_usage: "
					"idx=%u, path=%s, used=%llu, free=%llu",
					fsidx, path,
					(unsigned long long)v_used,
					(unsigned long long)v_free);
	}
}

static void
sys_heartbeat_mem(arms_context_t *ctx)
{
	struct vmtotal vmtotal;
	size_t size;
	unsigned long long v_free, v_used;
	int pagesize;
	int mib_hwpagesize[]  = { CTL_HW, HW_PAGESIZE };
	int mib_vmmeter[]     = { CTL_VM, VM_METER };

	size = sizeof(pagesize);
	if (sysctl(mib_hwpagesize, 2, &pagesize, &size, NULL, 0) == -1) {
		logit(LOG_WARNING, "sysctl hw.pagesize: %s", strerror(errno));
		return;
	}

	size = sizeof(vmtotal);
	if (sysctl(mib_vmmeter, 2, &vmtotal, &size, NULL, 0) == -1) {
		logit(LOG_WARNING, "sysctl vm.meter: %s", strerror(errno));
		return;
	}

	v_used = (unsigned long long)vmtotal.t_avm * pagesize;
	v_free = (unsigned long long)
		  (vmtotal.t_vm - vmtotal.t_avm + vmtotal.t_free) * pagesize;

	if (!opt_hbdebug)
		arms_hb_set_mem_usage(ctx, 0, v_used, v_free);
        if (opt_verbose)
                logit(LOG_INFO, "heartbeat mem_usage: "
                                "idx=%u, used=%llu, free=%llu",
                                0, v_used, v_free);
}

static void
sys_heartbeat_traffic(arms_context_t *ctx)
{
	struct ifdatareq ifdr;
	struct trafficstat cur, prev;
	unsigned long long dur, v_in_octet, v_out_octet, v_in_packet,
			   v_out_packet, v_in_error, v_out_error;
	int ifidx, s;
	char var[15];

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1) {
		logit(LOG_WARNING, "socket failed: %s", strerror(errno));
		return;
	}

	for (ifidx = 0;
	     ifidx < sizeof(traffic_prev) / sizeof(traffic_prev[0]);
	     ifidx++) {
		snprintf(var, sizeof(var), "hb-traffic-if%d", ifidx);

		memset(&ifdr, 0, sizeof(ifdr));
		strncpy(ifdr.ifdr_name, dconf_get_string(var),
			sizeof(ifdr.ifdr_name));

		if (ioctl(s, SIOCGIFDATA, &ifdr) == -1) {
			if (errno == ENXIO)
				break;		/* no more */
			logit(LOG_WARNING, "ioctl(SIOCGIFDATA, ) failed: %s",
					   strerror(errno));
			continue;
		}
		cur.in_octet = ifdr.ifdr_data.ifi_ibytes;
		cur.out_octet = ifdr.ifdr_data.ifi_obytes;
		cur.in_packet = ifdr.ifdr_data.ifi_ipackets;
		cur.out_packet = ifdr.ifdr_data.ifi_opackets;
		cur.in_error = ifdr.ifdr_data.ifi_ierrors;
		cur.out_error = ifdr.ifdr_data.ifi_oerrors;

		cur.t = sys_heartbeat_gettime();
		if (traffic_prev[ifidx].t == 0) {
			traffic_prev[ifidx] = cur;
			continue;
		}

		prev = traffic_prev[ifidx];
		if (cur.t <= prev.t)
			continue;       /* should not happen */

		if (cur.in_octet < prev.in_octet ||
		    cur.out_octet < prev.out_octet ||
		    cur.in_packet < prev.in_packet ||
		    cur.out_packet < prev.out_packet ||
		    cur.in_error < prev.in_error ||
		    cur.out_error < prev.out_error) {
			logit(LOG_INFO, "counter was reset (%s)",
					dconf_get_string(var));
			traffic_prev[ifidx] = cur;
			continue;
		}

		dur = cur.t - prev.t;
		v_in_octet   = (cur.in_octet   - prev.in_octet)   / dur;
		v_out_octet  = (cur.out_octet  - prev.out_octet)  / dur;
		v_in_packet  = (cur.in_packet  - prev.in_packet)  / dur;
		v_out_packet = (cur.out_packet - prev.out_packet) / dur;
		v_in_error   = (cur.in_error   - prev.in_error)   / dur;
		v_out_error  = (cur.out_error  - prev.out_error)  / dur;


		if (!opt_hbdebug)
			arms_hb_set_traffic_rate(ctx, ifidx,
						 v_in_octet, v_out_octet,
						 v_in_packet, v_out_packet,
						 v_in_error, v_out_error);
		if (opt_verbose)
			logit(LOG_INFO, "heartbeat traffic_rate: ifidx=%u, "
			      "in_octet=%llu, out_octet=%llu, in_packet=%llu, "
			      "out_packet=%llu, in_error=%llu, out_error=%llu",
			      ifidx, v_in_octet, v_out_octet, v_in_packet,
			      v_out_packet, v_in_error, v_out_error);

		traffic_prev[ifidx] = cur;
	}
	close(s);
}

void
sys_heartbeat_prepare(arms_context_t *ctx)
{
	sys_heartbeat_cpu(ctx);
	sys_heartbeat_mem(ctx);
	sys_heartbeat_disk(ctx);
	sys_heartbeat_traffic(ctx);
}
