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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/statvfs.h>

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

struct cpustat {
	time_t t;
	uint64_t idle;
	uint64_t interrupt;
	uint64_t user;
	uint64_t system;
	uint64_t other;
	uint64_t total;
};

struct trafficstat {
	time_t t;
	unsigned long long in_octet;
	unsigned long long out_octet;
	unsigned long long in_packet;
	unsigned long long out_packet;
	unsigned long long in_error;
	unsigned long long out_error;
};

static struct cpustat cpu_prev = { 0 };
static struct trafficstat traffic_prev[3] = { {0}, {0}, {0} };

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

	snprintf(buf, sizeof(buf), "traceroute %s -n -m %d -q %d %s > %s",
		 (af == AF_INET ? "-4" : "-6"),
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
	FILE *fp;
	struct cpustat cur;
	unsigned long long x[8], total, difftotal;
	int i, n;
	char cpu[5], linebuf[80];
	uint8_t v_idle, v_interrupt, v_user, v_system, v_other;

	fp = fopen("/proc/stat", "r");
	if (fp == NULL) {
		logit(LOG_WARNING, "cannot open /proc/stat: %s",
		      strerror(errno));
		return;
	}
	for (i = 0; i < 8; i++)
		x[i] = 0;
	total = 0;
	while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		n = sscanf(linebuf,
			   "%4s %llu %llu %llu %llu %llu %llu %llu %llu",
			   (char *)&cpu, &x[0], &x[1], &x[2], &x[3], &x[4],
			   &x[5], &x[6], &x[7]);
		if (n >= 5 && strcmp(cpu, "cpu") == 0) {
			for (i = 0; i < 8; i++)
				total += x[i]; 
			break;
		}
	}
	fclose(fp);
	if (total == 0)
		return;

	cur.t = sys_heartbeat_gettime();
	cur.idle = x[3];
	cur.interrupt = (x[5] + x[6]);
	cur.user = (x[0] + x[1]);
	cur.system = x[2];
	cur.other = (x[4] + x[7]);
	cur.total = total;
	if (cpu_prev.t == 0) {
		cpu_prev = cur;
		return;
	}

	/* TODO: 4 sya 5 nyuu */
	difftotal = cur.total - cpu_prev.total;
	v_idle = (cur.idle - cpu_prev.idle) * 100 / difftotal;
	v_interrupt = (cur.interrupt - cpu_prev.interrupt) * 100 / difftotal;
	v_user = (cur.user - cpu_prev.user) * 100 / difftotal;
	v_system = (cur.system - cpu_prev.system) * 100 / difftotal;
	v_other = (cur.other - cpu_prev.other) * 100 / difftotal;

	arms_hb_set_cpu_detail_usage(ctx, 0, v_idle, v_interrupt, v_user,
		v_system, v_other);
	if (opt_verbose)
		logit(LOG_INFO, "heartbeat cpu_detail_usage: idx=%u, idle=%u, "
		      "interrupt=%u, user=%u, system=%u, other=%u",
		      0, v_idle, v_interrupt, v_user, v_system, v_other);

	arms_hb_set_cpu_usage(ctx, 0, 100 - v_idle);

	cpu_prev = cur;
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
	FILE *fp;
	unsigned long long v_free, v_used;
	unsigned long long b, c, f, t, v;
	int n;
	char name[17], linebuf[80];

	fp = fopen("/proc/meminfo", "r");
	if (fp == NULL) {
		logit(LOG_WARNING, "cannot open /proc/meminfo: %s",
		      strerror(errno));
		return;
	}
	f = t = b = c = 0;
	while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		n = sscanf(linebuf, "%16s %llu kB", (char *)&name, &v);
		if (n < 2)
			continue;
		if (strcmp(name, "MemTotal:") == 0)
			t = v * 1024;
		else if (strcmp(name, "MemFree:") == 0)
			f = v * 1024;
		else if (strcmp(name, "Buffers:") == 0)
			b = v * 1024;
		else if (strcmp(name, "Cached:") == 0)
			c = v * 1024;
	}
	fclose(fp);
	if (t < f + b + c)
		return;		/* should not happen */
	v_free = f + b + c;
	v_used = t - v_free;
	arms_hb_set_mem_usage(ctx, 0, v_used, v_free);
	if (opt_verbose)
		logit(LOG_INFO, "heartbeat mem_usage: "
				"idx=%u, used=%llu, free=%llu",
				0, v_used, v_free);
}

static void
sys_heartbeat_traffic(arms_context_t *ctx)
{
	FILE *fp;
	struct trafficstat cur, prev;
	unsigned long long dur, v_in_octet, v_out_octet, v_in_packet,
		           v_out_packet, v_in_error, v_out_error;
	int ifidx, n;
	char ifname[8], linebuf[1024], *p;

	fp = fopen("/proc/net/dev", "r");
	if (fp == NULL) {
		logit(LOG_WARNING, "cannot open /proc/net/dev: %s",
		      strerror(errno));
		return;
	}
	while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
		if ((p = strchr(linebuf, ':')) == NULL)
			continue;
		*p = ' ';
		n = sscanf(linebuf,
			   "%7s "
			   "%llu %llu %llu %*u %*u %*u %*u %*u"
			   "%llu %llu %llu %*u %*u %*u %*u %*u",
			   ifname,
			   &cur.in_octet,
			   &cur.in_packet,
			   &cur.in_error,
			   &cur.out_octet,
			   &cur.out_packet,
			   &cur.out_error);
		if (n < 7)
			continue;
		if (strcmp(ifname, "eth0") == 0)
			ifidx = 0;
		else if (strcmp(ifname, "eth1") == 0)
			ifidx = 1;
		else if (strcmp(ifname, "eth2") == 0)
			ifidx = 2;
		else
			continue;

		cur.t = sys_heartbeat_gettime();
		if (traffic_prev[ifidx].t == 0) {
			traffic_prev[ifidx] = cur;
			continue;
		}

		prev = traffic_prev[ifidx];
		if (cur.t <= prev.t)
			continue;	/* should not happen */
		dur = cur.t - prev.t;
		v_in_octet   = (cur.in_octet   - prev.in_octet)   / dur;
		v_out_octet  = (cur.out_octet  - prev.out_octet)  / dur;
		v_in_packet  = (cur.in_packet  - prev.in_packet)  / dur;
		v_out_packet = (cur.out_packet - prev.out_packet) / dur;
		v_in_error   = (cur.in_error   - prev.in_error)   / dur;
		v_out_error  = (cur.out_error  - prev.out_error)  / dur;
		arms_hb_set_traffic_rate(ctx, ifidx, v_in_octet, v_out_octet,
			v_in_packet, v_out_packet, v_in_error, v_out_error);
		if (opt_verbose)
			logit(LOG_INFO, "heartbeat traffic_rate: ifidx=%u, "
			      "in_octet=%llu, out_octet=%llu, in_packet=%llu, "
			      "out_packet=%llu, in_error=%llu, out_error=%llu",
			      ifidx, v_in_octet, v_out_octet, v_in_packet,
			      v_out_packet, v_in_error, v_out_error);
		traffic_prev[ifidx] = cur;
	}
	fclose(fp);
}

void
sys_heartbeat_prepare(arms_context_t *ctx)
{
	sys_heartbeat_cpu(ctx);
	sys_heartbeat_mem(ctx);
	sys_heartbeat_disk(ctx);
	sys_heartbeat_traffic(ctx);
}
