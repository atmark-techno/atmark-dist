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
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>

#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <syslog.h>
#include <unistd.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libarms.h"

#include "armsd.h"
#include "line.h"
#include "compat.h"

#define ARMSDIRPERM	0755		/* 0755 for debug, 0700 for prod */
#define HEARTBEAT_MAX	5
#define PATHLEN		256

#define PATHMARMSDCONF	"/etc/armsd/armsd.conf"
#define PATHICONFIG	"/etc/armsd/initial-config"
#define PATHSTATECACHE	"/var/cache/armsd/state"
#define PATHPIDFILE	"/var/run/armsd.pid"

const char *opt_basedir = NULL;
int opt_verbose = 0;
int opt_nodaemon = 0;
int opt_hbdebug = 0;
const char *opt_distid = NULL;
const char *opt_logfile = NULL;
int opt_wait_pulldone = 0;

const char armsd_progname[] = "armsd";
const char armsd_version[] = "0.9.0";

const char arms_root_ca_certificate[] =
"-----BEGIN CERTIFICATE-----\n"
"MIICxjCCAa6gAwIBAgIBADANBgkqhkiG9w0BAQUFADAXMRUwEwYDVQQDEwxBUk1T\n"
"IFJvb3QgQ0EwHhcNMDcwNTIzMDQ0MjEyWhcNMzcwNjE0MDQ0MjEyWjAXMRUwEwYD\n"
"VQQDEwxBUk1TIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIB\n"
"AQDJRayC6SYg897wpwf56UffftVpGGtiYOi8LO4/L9lnPaMw7b9P8r30UbXy0AfX\n"
"rHzQHSBpdHTVopFONT6dMhd2kIyvFAw3r8H/lRK0VmIDV+dE3Y/rRFdB1P3TYQoC\n"
"rpUEsDJeQ/3bc0f/UwyCgm9laigMD6sGqEuFBFm7ovFfp/y2RtN1QKgCBGGKbz6z\n"
"dSgfqG0BwISICjqVEBM2ltOr93pBHrlI9btER1OP7CwCvahHhwUhgsUj7US3mPrd\n"
"AHTlWb9iyd3+EdstxsSZZRZa4iJB+f2OIVcb02o+Q9baHb8IZWwnY0Ei+rgZQZDN\n"
"WLOMXLLsEuPGlj6DHgRTOXR5AgMBAAGjHTAbMAwGA1UdEwQFMAMBAf8wCwYDVR0P\n"
"BAQDAgEGMA0GCSqGSIb3DQEBBQUAA4IBAQBPeMgeVIIiipIMypT7aPcfyaDuAJDy\n"
"MFrAEBGUCQDVwt1g6OtlJb5At/N3lXc8U149P11Uu7+lMescESassXnUVV2DNOrX\n"
"3uv6Jw2H3fhqaVvnPO3hlgJqjjqE1bVZJBA7I7H8Z0k2BvFAu3EvmT9dWabyWtqW\n"
"3UW/fhXV5byJ//0JEcGoWv1sxjG3/jNiZ3YhX0ZxzITAmWbRIpaljEq/mgzgAj7G\n"
"EIBaFnH6rpYsISaEAsFQilIWOfZIGHz5DVRjqQ70IAglxDRER7bdx91312chamxK\n"
"o+dGih8xsWZ7ntkKAEwZzsmgn+ZCDOJQK5t2pAe+8kdl4oVMpOMb78hM\n"
"-----END CERTIFICATE-----\n";

distribution_id_t distid;
arms_context_t *arms_context;
int arms_callback_udata;
int arms_listen_port;

FILE *log_fp = NULL;
const char *pidfile = NULL;
int pidfilecreated = 0;
pid_t armsd_pid = -1;
pid_t script_pid = -1;
int script_exitcode = -1;
int daemonized = 0;
struct timespec hb_lastsent = { 0 };

const char *configfile = PATHMARMSDCONF;

char *dconf_text = NULL;
struct dconf {
	const char *name;
	const char *def;
	char *val;
} dconf_list[] = {
	{ "app-event-interval", "300", NULL },
	{ "binary",		"0", NULL },
	{ "distribution-id",	NULL, NULL  },
	{ "hb-disk-usage0",	"/", NULL },
	{ "hb-disk-usage1",	NULL, NULL },
	{ "hb-disk-usage2",	NULL, NULL },
	{ "hb-dummy",		"0", NULL  },
	{ "hb-traffic-if0",	"eth0", NULL  },
	{ "hb-traffic-if1",	"eth1", NULL  },
	{ "hb-traffic-if2",	"eth2", NULL  },
	{ "hb-traffic-if3",	NULL, NULL  },
	{ "https-proxy-url",	NULL, NULL },
	{ "https-proxy-url-ls",	NULL, NULL },
	{ "https-proxy-url-rs",	NULL, NULL },
	{ "ls-sa-key",		"", NULL  },
	{ "path-iconfig",	NULL, NULL  },
	{ "path-root-ca",	NULL, NULL  },
	{ "path-pid",		PATHPIDFILE, NULL  },
	{ "path-state-cache",	NULL, NULL  },
	{ "script-app-event",	NULL, NULL  },
	{ "script-clear",	NULL, NULL  },
	{ "script-command",	NULL, NULL  },
	{ "script-debug",	NULL, NULL  },
	{ "script-reconfig",	NULL, NULL  },
	{ "script-start",	NULL, NULL  },
	{ "script-post-pull",	NULL, NULL  },
	{ "script-status",	NULL, NULL  },
	{ "script-stop",	NULL, NULL  },
	{ "script-reboot",	NULL, NULL  },
	{ "script-line-ctrl",	NULL, NULL  },
	{ "script-state-changed", NULL, NULL  },
	{ "timeout",		"30", NULL  },
	{ NULL, NULL, NULL }
};

struct callback_state {
	FILE *in;
	FILE *out;
	char *infile;
	char *outfile;
	int nul;
};

struct module {
	unsigned long id;
	char *idstr;
	char *version;
	char *infostring;

	struct config_state {
		FILE *fp;
		char *tmpfile;
		char *nextversion;
		char *nextinfo;
	} cstate;

	struct callback_state clear_state;
	struct callback_state get_status_state;
	struct callback_state read_config_state;
	struct callback_state md_command_state;

	struct module *next;		/* linked list */
};
struct module *module_list = NULL;

struct scriptlog {
	unsigned int bufsize;
	unsigned int readptr;
	unsigned int writeptr;
	char buf[1000];
};
static struct scriptlog scriptlog;

struct timeout {
	struct timespec ts;
	unsigned int sec;
};

static struct armsdir {
	/* directory */
	char *basedir;

	char *backupdir;
	char *candidatedir;
	char *runningdir;
	char *startupdir;
	char *tmpdir;

	/* temporary directory counter */
	unsigned int tmpnum;
} armsdir = { NULL, };

static int ihcw;		/* i hate (useless) compiler warnings */

static const char *arms_config_string(int);
static const char *arms_error_string(int);
static const char *arms_state_string(int);

static int callback_app_event(void *);
static int callback_clear_status(uint32_t, const char *, size_t, char *,
				 size_t, int *, void *);
static int callback_command(uint32_t, int, const char *, size_t, char *,
			    size_t, int *, void *);
static int callback_config(uint32_t, const char *, const char *, int,
			   const char *, size_t, int, void *);
static int callback_dump_debug(char *, size_t, int *, void *);
static int callback_heartbeat(arms_context_t *, void *);
static int callback_generic(uint32_t, const char *, size_t, char *, size_t,
			    int *, void *, const char *,
			    struct callback_state *, int);
static int callback_get_status(uint32_t, const char *, size_t, char *, size_t,
			       int *, void *);
static int callback_line_ctrl(int, int, void *, void *);
static int callback_log(int, const char *, void *);
static int callback_md_command(uint32_t, const char *, size_t, char *,
			       size_t, int *, void *);
static int callback_ping(const arms_ping_arg_t *, char *, size_t, int *,
			 void *);
static int callback_read_config(uint32_t, int, char *, size_t, int *, void *);
static int callback_state(int, int, void *);
static void callback_state_set_proxy(const char *);
static int callback_traceroute(const arms_traceroute_arg_t *, char *, size_t,
			       int *, void *);

static int armsdir_create(void);
static int armsdir_create_sub(const char *, char **);
static int armsdir_remove(void);
static void armsdir_remove_tmpfiles(void);
static int armsdir_set_base(void);

static struct module *module_new(unsigned long, const char *, const char *);
static int module_update(struct module *, const char *, const char *);
static void module_delete(struct module *);
static void module_all_remove_files(void);
static int module_config_exist(const struct module *, int);
static const char *module_config_path(const struct module *, int);
static int module_config_pathcpy(const struct module *, int, char *,
				   unsigned int);
static void module_copy_backup2running(unsigned long);
static void module_copy_candidate2running(unsigned long);
static void module_copy_candidate(unsigned long, const char *);
static void module_copy_running(unsigned long, const char *);
static void module_copy_running2backup(unsigned long);
static void module_copy_running2startup(unsigned long);
static void module_copy_sub_ln(unsigned long, const char *, const char *);
static void module_copy_sub_mv(unsigned long, const char *, const char *);
static struct module *module_find(unsigned long);
static int module_list_append(struct module *);
static struct module *module_list_iter_first(void);
static struct module *module_list_iter_next(struct module *);
static int module_list_remove(struct module *);
static void module_remove_candidate(struct module *);
static void module_remove_files(const struct module *);

static void heartbeat_prepare_dummy(arms_context_t *);

static int dconf_load(void);
static int dconf_parse(char *);
static char *dconf_trim(char *);

static int log_open_file(const char *);
static const char *log_priority_string(int);
static void log_close(void);

static void pidfile_create(void);
static void pidfile_remove(void);

void script_clearlog(void);
void script_errlog(const char *, int);
void script_initenv(void);
void script_logcpy(char *, const char *, unsigned int);

struct timeout *timeout_new(unsigned int);
void timeout_free(struct timeout *);
int timeout_remaining(struct timeout *);

static int is_xml_ascii(int);

void sighandler_init(void);
void sighandler_child(int);
void sighandler_exit(int);
void sighandler_reload(int);

static void show_version(void);
static const char *make_version_log(void);
static void usage(void);
static void cleanup(void);
static int str2distid(const char *, distribution_id_t *);
static int distid2str(const distribution_id_t *, char *, unsigned int);
static int load_from_file(const char *, char **);

static void load_state_cache(void);
static void save_state_cache(void);

static void sa_stop_modules(void);
static void sa_reboot(void);
static int sa_post_pull(void);

static arms_callback_tbl_t arms_callback_table = {
	ARMS_LIBPULL_VERSION,
	callback_config,
	callback_line_ctrl,
	callback_state,
	callback_log,
	callback_read_config,
	callback_get_status,
	callback_command,
	callback_app_event,
	callback_heartbeat,
};


static const char *
arms_config_string(int type)
{
	switch (type) {
	case ARMS_CONFIG_BACKUP:	return "backup-backup";
	case ARMS_CONFIG_CANDIDATE:	return "candidate-config";
	case ARMS_CONFIG_RUNNING:	return "running-config";
	default:			return "(unknown-config)";
	}
}

static const char *
arms_error_string(int e)
{
	static char ebuf[80];

	switch (e) {
	default:
		snprintf(ebuf, sizeof(ebuf), "(unknown error code %d)", e);
		return ebuf;
	}
}

static const char *
arms_state_string(int state)
{
	const char *s;

	switch (state) {
	case ARMS_ST_INITIAL:		s = "INITIAL"; break;
	case ARMS_ST_LSPULL:		s = "LSPULL"; break;
	case ARMS_ST_RSPULL:		s = "RSPULL"; break;
	case ARMS_ST_PULLDONE:		s = "PULLDONE"; break;
	case ARMS_ST_BOOT_FAIL:		s = "BOOT_FAIL"; break;
	case ARMS_ST_PUSH_INITIAL:	s = "PUSH_INITIAL"; break;
	case ARMS_ST_PUSH_SENDREADY:	s = "PUSH_SENDREADY"; break;
	case ARMS_ST_PUSH_WAIT:		s = "PUSH_WAIT"; break;
	case ARMS_ST_PUSH_REBOOT:	s = "PUSH_REBOOT"; break;
	default:			s = "(unknown)"; break;
	}
	return s;
};


static int
armsdir_create(void)
{
	int e = 0;

	/* create base directory */
	if (mkdir(armsdir.basedir, ARMSDIRPERM) == -1) {
		logit(LOG_ERR, "mkdir(\"%s\") failed: %s",
			       armsdir.basedir, strerror(errno));
		return -1;
	}

	e += armsdir_create_sub("backup-config", &armsdir.backupdir);
	e += armsdir_create_sub("candidate-config", &armsdir.candidatedir);
	e += armsdir_create_sub("running-config", &armsdir.runningdir);
	e += armsdir_create_sub("startup-config", &armsdir.startupdir);
	e += armsdir_create_sub("tmp", &armsdir.tmpdir);

	armsdir.tmpnum = 0;
	return e;
}

/* NOTE: returns 0 for success, 1 for error. */
static int
armsdir_create_sub(const char *name, char **ptr)
{
	unsigned int n;
	char *p;

	n = strlen(armsdir.basedir) + 1 + strlen(name) + 1;
	p = malloc(n);
	if (p == NULL) {
		logit(LOG_EMERG, "malloc(3) failed");
		return 1;
	}
	snprintf(p, n, "%s/%s", armsdir.basedir, name);
	if (mkdir(p, ARMSDIRPERM) == -1) {
		logit(LOG_ERR, "mkdir(\"%s\") failed: %s",
			       name, strerror(errno));
		free(p);
		return 1;
	}
	*ptr = p;
	return 0;
}

FILE *
armsdir_create_tmpfile(char **ptr)
{
	FILE *fp;
	int fd;
	char *p;
	char buf[128];

	snprintf(buf, sizeof(buf), "%s/%u", armsdir.tmpdir, armsdir.tmpnum);
	armsdir.tmpnum++;

	fd = open(buf, O_RDWR|O_CREAT|O_EXCL, 0600);
	if (fd == -1)
		err(1, "cannot create tmpfile: %s", buf);

	if (ptr != NULL) {
		p = strdup(buf);
		if (p == NULL)
			err(1, "strdup() failed");
		*ptr = p;
	}

	fp = fdopen(fd, "r+");
	if (fp == NULL)
		err(1, "fopen failed: %s", buf);

	return fp;
}

static int
armsdir_remove(void)
{
	if (armsdir.basedir == NULL)
		return 0;

	module_all_remove_files();
	armsdir_remove_tmpfiles();

	if (rmdir(armsdir.backupdir) != 0)
		warn("cannot remove %s", armsdir.backupdir);
	if (rmdir(armsdir.candidatedir) != 0)
		warn("cannot remove %s", armsdir.candidatedir);
	if (rmdir(armsdir.runningdir) != 0)
		warn("cannot remove %s", armsdir.runningdir);
	if (rmdir(armsdir.startupdir) != 0)
		warn("cannot remove %s", armsdir.startupdir);
	if (rmdir(armsdir.tmpdir) != 0)
		warn("cannot remove %s", armsdir.tmpdir);

	if (rmdir(armsdir.basedir) != 0)
		warn("cannot remove %s", armsdir.basedir);
	return 0;
}

static void
armsdir_remove_tmpfiles(void)
{
	DIR *dirp;
	struct dirent *dp;
	char buf[256];

	dirp = opendir(armsdir.tmpdir);
	if (dirp == NULL) {
		logit(LOG_ERR, "cannot opendir %s: %s",
			       armsdir.tmpdir, strerror(errno));
		return;
	}

	while ((dp = readdir(dirp)) != NULL) {
		/* will not remove ".", ".." and ".hoge" */
		if (dp->d_name[0] == '.')
			continue;
		snprintf(buf, sizeof(buf),
			 "%s/%s", armsdir.tmpdir, dp->d_name);
		(void)unlink(buf);
	}
	closedir(dirp);
}

static int
armsdir_set_base(void)
{
	struct stat st;
	unsigned int n;
	char buf[256], *p;

	if (opt_basedir != NULL) {
		if (*opt_basedir == '/') {
			n = strlen(opt_basedir);
			p = strdup(opt_basedir);
		} else {
			/* convert relative path to absolute path */
			if (getcwd(buf, sizeof(buf)) == NULL) {
				logit(LOG_ERR, "getcwd(3) failed...");
				return -1;
			}
			n = strlen(buf) + 1 + strlen(opt_basedir) + 1;
			p = malloc(n);
			if (p != NULL)
				snprintf(p, n, "%s/%s", buf, opt_basedir);
		}
	} else {
		snprintf(buf, sizeof(buf), "/tmp/armsd.%lu",
			 (unsigned long)getpid());
		n = strlen(buf);
		p = strdup(buf);
	}
	if (p == NULL) {	/* really? */
		logit(LOG_ERR, "no memory (cannot allocate %u bytes)", n);
		return -1;
	}
	if (stat(p, &st) == 0) {
		logit(LOG_ERR, "base directory already exists: %s", p);
		free(p);
		return -1;
	}
	armsdir.basedir = p;
	return 0;
}

static struct module *
module_new(unsigned long id, const char *version, const char *infostring)
{
	struct module *mod;
	char idbuf[16];

	mod = malloc(sizeof(struct module));
	if (mod == NULL)
		return NULL;
	mod->id = id;

	snprintf(idbuf, sizeof(idbuf), "%lu", id);
	mod->idstr = strdup(idbuf);
	mod->version = strdup((version != NULL) ? version : "-");
	mod->infostring = strdup((infostring != NULL) ? infostring : "-");
	if (mod->idstr == NULL ||
	    mod->version == NULL ||
	    mod->infostring == NULL) {
		logit(LOG_ERR, "memory exausted");
		free(mod->idstr);
		free(mod->version);
		free(mod->infostring);
		free(mod);
		return NULL;
	}

	memset(&mod->cstate, 0, sizeof(mod->cstate));
	memset(&mod->clear_state, 0, sizeof(mod->clear_state));
	memset(&mod->get_status_state, 0, sizeof(mod->get_status_state));
	memset(&mod->read_config_state, 0, sizeof(mod->read_config_state));
	memset(&mod->md_command_state, 0, sizeof(mod->md_command_state));
	mod->next = NULL;

	module_list_append(mod);
	return mod;
}

static int
module_update(struct module *mod, const char *version, const char *infostring)
{
	free(mod->version);
	free(mod->infostring);
	mod->version = strdup((version != NULL) ? version : "-");
	if (mod->version == NULL)
		return -1;
	mod->infostring = strdup((infostring != NULL) ? infostring : "-");
	if (mod->infostring == NULL)
		return -1;

	return 0;
}

static void
module_delete(struct module *mod)
{
	module_list_remove(mod);
	module_remove_candidate(mod);
	module_remove_files(mod);
	free(mod->idstr);
	free(mod->version);
	free(mod->infostring);
	free(mod);
}

static void
module_all_remove_files(void)
{
	struct module *m;

	for (m = module_list; m != NULL; m = m->next)
		module_remove_files(m);
}

static int
module_config_exist(const struct module *mod, int type)
{
	char buf[128];

	if (module_config_pathcpy(mod, type, buf, sizeof(buf)) > 0)
		return (access(buf, F_OK) == 0);
	else
		return 0;
}

/* return a pointer to a static varaible - use with care */
static const char *
module_config_path(const struct module *mod, int type)
{
	static char buf[256];

	if (module_config_pathcpy(mod, type, buf, sizeof(buf)) != -1)
		return buf;
	else
		return NULL;
}

static int
module_config_pathcpy(const struct module *mod, int type, char *buf,
		       unsigned int len)
{
	const char *dir;

	switch (type) {
	case ARMS_CONFIG_BACKUP:	dir = armsdir.backupdir;	break;
	case ARMS_CONFIG_CANDIDATE:	dir = armsdir.candidatedir;	break;
	case ARMS_CONFIG_RUNNING:	dir = armsdir.runningdir;	break;
	default:
		return -1;
	}
	return snprintf(buf, len, "%s/%lu", dir, mod->id);
}

static void
module_copy_backup2running(unsigned long mid)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.backupdir, mid);
	module_copy_sub_ln(mid, buf, armsdir.runningdir);
}

static void
module_copy_candidate2running(unsigned long mid)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.candidatedir, mid);
	module_copy_sub_ln(mid, buf, armsdir.runningdir);
}

static void
module_copy_candidate(unsigned long mid, const char *filename)
{
	module_copy_sub_mv(mid, filename, armsdir.candidatedir);
}

static void
module_copy_running2backup(unsigned long mid)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.runningdir, mid);
	module_copy_sub_ln(mid, buf, armsdir.backupdir);
}

static void
module_copy_running2startup(unsigned long mid)
{
	const struct module *mod;
	char buf[128];

	if ((mod = module_find(mid)) == NULL)
		return;
	if (module_config_pathcpy(mod, ARMS_CONFIG_RUNNING,
				   buf, sizeof(buf)) == -1)
		return;
	module_copy_sub_ln(mid, buf, armsdir.startupdir);
}

static void
module_copy_running(unsigned long mid, const char *filename)
{
	module_copy_sub_mv(mid, filename, armsdir.runningdir);
}

static void
module_copy_sub_ln(unsigned long mid, const char *filename, const char *dir)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "%s/%lu", dir, mid);
	if (unlink(buf) == -1 && errno != ENOENT)
		err(1, "unlink %s", buf);
	if (link(filename, buf) == -1)
		err(1, "link %s -> %s", filename, buf);
}

static void
module_copy_sub_mv(unsigned long mid, const char *filename, const char *dir)
{
	module_copy_sub_ln(mid, filename, dir);
	if (unlink(filename) == -1)
		err(1, "unlink %s", filename);
}

static struct module *
module_find(unsigned long mid)
{
	struct module *m;

	for (m = module_list; m != NULL; m = m->next) {
		if (m->id == mid)
			return m;
	}
	return NULL;
}

static int
module_list_append(struct module *mod)
{
	struct module *m;

	if (module_list == NULL)
		module_list = mod;
	else {
		for (m = module_list; m->next != NULL; m = m->next)
			;
		m->next = mod;
	}
	mod->next = NULL;
	return 0;
}

static struct module *
module_list_iter_first(void)
{
	return module_list;
}

static struct module *
module_list_iter_next(struct module *mod)
{
	return mod->next;
}

static int
module_list_remove(struct module *mod)
{
	struct module *m;

	if (module_list == NULL)
		return -1;
	if (module_list == mod)
		module_list = mod->next;
	else {
		for (m = module_list; m != NULL; m = m->next)
			if (m->next == mod) {
				m->next = mod->next;
				break;
			}
		if (m == NULL)
			return -1;
	}
	return 0;
}

static void
module_remove_candidate(struct module *mod)
{
	char buf[128];

	module_config_pathcpy(mod, ARMS_CONFIG_CANDIDATE, buf, sizeof(buf));
	unlink(buf);
}

static void
module_remove_files(const struct module *mod)
{
	char buf[128];

	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.backupdir, mod->id);
	unlink(buf);
	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.candidatedir, mod->id);
	unlink(buf);
	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.runningdir, mod->id);
	unlink(buf);
	snprintf(buf, sizeof(buf), "%s/%lu", armsdir.startupdir, mod->id);
	unlink(buf);
}

static void
heartbeat_prepare_dummy(arms_context_t *ctx)
{
	uint64_t q0, q1, q2, q3, q4, q5, qq;
	uint8_t c0, c1, c2, c3, c4, cq;
	int i, j;
	static uint64_t t[6][3];
	static const uint64_t tmax[3] = {
		1000 * 1000 * 1000,	/* if#0 is 1000Base-T */
		10 * 1000 * 1000,	/* if#1 is 10Base-T */
		64 * 1024		/* if#2 is ISDN :D */
	};

	/* cpu: 4-core cpu */
	for (i = 0; i < 4; i++) {
		c0 = (uint8_t)random() % 101;	/* idle */
		cq = 100 - c0;
		arms_hb_set_cpu_usage(ctx, i, cq);

		c1 = c2 = c3 = 0;
		if (cq > 0) {
			c1 = (uint8_t)random() % (cq + 1);
			cq -= c1;
		}
		if (cq > 0) {
			c2 = (uint8_t)random() % (cq + 1);
			cq -= c2;
		}
		if (cq > 0) {
			c3 = (uint8_t)random() % (cq + 1);
			cq -= c3;
		}
		c4 = cq;
		arms_hb_set_cpu_detail_usage(ctx, i, c0, c1, c2, c3, c4);
	}

	/* memory: #0 is 256MB */
	qq = 256 * 1024 * 1024;
	q0 = (uint64_t)random() % qq;
	q1 = qq - q0;
	arms_hb_set_mem_usage(ctx, 0, q0, q1);

	/* memory: #1 is 3GB */
	qq = 3ULL * 1024 * 1024 * 1024;
	q0 = (uint64_t)random() * 3 % qq;
	q1 = qq - q0;
	arms_hb_set_mem_usage(ctx, 1, q0, q1);

	/* disk: #0 is 32GB SSD */
	qq = 32ULL * 1024 * 1024 * 1024;
	q0 = (uint64_t)random() * 16 % qq;
	q1 = qq - q0;
	arms_hb_set_disk_usage(ctx, 0, q0, q1);

	/* disk: #1 is 4TB SATA HDD */
	qq = 4ULL * 1024 * 1024 * 1024 * 1024;
	q0 = (uint64_t)random() * 2048 % qq;
	q1 = qq - q0;
	arms_hb_set_disk_usage(ctx, 1, q0, q1);

	/* traffic */
	for (i = 0; i < 3; i++) {
		qq = tmax[i] / 8;	/* bit-per-sec -> byte-per-sec */
		q0 = (uint64_t)random() % qq;
		q1 = (uint64_t)random() % qq;
		q2 = q0 / (random() % 1458 + 64);
		q3 = q1 / (random() % 1458 + 64);
		q4 = q2 / 5;
		q5 = q3 / 5;
		arms_hb_set_traffic_rate(ctx, i, q0, q1, q2, q3, q4, q5);
		if (random() % 36 == 0) {
			for (j = 0; j < 6; j++)
				t[j][i] = 0;	/* reset! */
		}
		t[0][i] += q0;
		t[1][i] += q1;
		t[2][i] += q2;
		t[3][i] += q3;
		t[4][i] += q4;
		t[5][i] += q5;
		arms_hb_set_traffic_count(ctx, i, t[0][i], t[1][i], t[2][i],
				     t[3][i], t[4][i], t[5][i]);
	}

	/* radiowave */
	c0 = random() % 101;
	c1 = random() % 101;
	c2 = random() % 101;
	c3 = random() % 101;
	arms_hb_set_radiowave(ctx, 0, c0, c1, c2, c3);
}

static void
dconf_clear(void)
{
	int i;

	for (i = 0; dconf_list[i].name != NULL; i++) {
		free(dconf_list[i].val);
		dconf_list[i].val = NULL;
	}
}

int
dconf_get_int(const char *name)
{
	int i;
	const char *v;

	for (i = 0; dconf_list[i].name != NULL; i++) {
		if (strcmp(name, dconf_list[i].name) == 0) {
			v = dconf_list[i].val;
			if (v == NULL)
				v = dconf_list[i].def;

			/* XXX: strtol? */
			return atoi(v);
		}
	}
	return -1;
}

const char *
dconf_get_string(const char *name)
{
	int i;

	for (i = 0; dconf_list[i].name != NULL; i++) {
		if (strcmp(name, dconf_list[i].name) == 0) {
			if (dconf_list[i].val == NULL)
				return dconf_list[i].def;
			else
				return dconf_list[i].val;
		}
	}
	return NULL;
}

static int
dconf_load(void)
{
	FILE *fp;
	int errs;
	char linebuf[1000];

	dconf_clear();

	fp = fopen(configfile, "r");
	if (fp == NULL)
		err(1, "no config file %s", configfile);
	errs = 0;
	while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
		errs += dconf_parse(linebuf);
	fclose(fp);

	if (errs > 0)
		return -1;
	return 0;
}

static int
dconf_parse(char *line)
{
	int i;
	char *p, *q;

	/* NOTE: line is not "const". */

	p = dconf_trim(line);
	if (*p == '\0') /* empty line */
		return 0;
	if (*p == '#')  /* comment */
		return 0;

	q = strchr(p, ':');
	if (q == NULL) {
		/* no separator */
		return 1;
	}
	*q++ = '\0';

	p = dconf_trim(p);
	q = dconf_trim(q);

	for (i = 0; dconf_list[i].name != NULL; i++) {
		if (strcmp(p, dconf_list[i].name) == 0) {
			q = strdup(q);
			if (q == NULL)
				return 1;
			dconf_list[i].val = q;
			return 0;
		}
	}
	fprintf(stderr, "warning: unknown configuration parameter: %s=%s\n",
			p, q);
	return 1;
}

static char *
dconf_trim(char *p)
{
	char *q;

	while (*p != '\0' && isspace((int)*p))
		p++;
	q = p + strlen(p);
	while (p < q && (*q == '\0' || isspace((int)*q)))
		*q-- = '\0';
	return p;
}

static int
log_open_file(const char *filename)
{
	if (log_fp != NULL)
		fclose(log_fp);
	log_fp = fopen(filename, "a");
	if (log_fp == NULL)
		return -1;
	setvbuf(log_fp, NULL, _IOLBF, 0);
	return 0;
}

static const char *
log_priority_string(int priority)
{
	switch (priority) {
	case LOG_EMERG:		return "EMERG";
	case LOG_ERR:		return "ERROR";
	case LOG_WARNING:	return "WARN ";
	case LOG_INFO:		return "INFO ";
	case LOG_DEBUG:		return "DEBUG";
	default:		return "?????";
	}
}

void
logit(int priority, const char *msg, ...)
{
	va_list ap, ap_cp;
	struct tm *tm;
	time_t now;
	char timebuf[20];

	va_start(ap, msg);
	va_copy(ap_cp, ap);

	if (log_fp == NULL)
		vsyslog(priority, msg, ap);
	else {
		time(&now);
		tm = localtime(&now);
		strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S", tm);
		fprintf(log_fp, "%s %s [%u] %s ",
			timebuf,
			armsd_progname,
			(unsigned int)getpid(),
			log_priority_string(priority));
		vfprintf(log_fp, msg, ap);
		fputc('\n', log_fp);
	}

	if (opt_verbose && !daemonized) {
		time(&now);
		tm = localtime(&now);
		strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tm);
		fprintf(stdout, "%s %s ",
			timebuf, log_priority_string(priority));
		vfprintf(stdout, msg, ap_cp);
		fputc('\n', stdout);
		fflush(stdout);
	}

	va_end(ap_cp);
	va_end(ap);
}

static void
log_close(void)
{
	if (log_fp == NULL)
		return;		/* double close - ignore it */
	fclose(log_fp);
	log_fp = NULL;
}

static void
pidfile_create(void)
{
	FILE *fp;

	armsd_pid = getpid();

	if (pidfile == NULL)
		pidfile = dconf_get_string("path-pid");
	fp = fopen(pidfile, "w");
	if (fp == NULL) {
		/* something wrong but not fatal */
		fprintf(stderr, "cannot write pidfile %s: %s\n",
			pidfile, strerror(errno));
		return;
	}
	fprintf(fp, "%lu\n", (unsigned long)getpid());
	fclose(fp);
	pidfilecreated = 1;
}

static void
pidfile_remove(void)
{
	if (pidfilecreated)
		(void)unlink(pidfile);
	pidfilecreated = 0;
}


static void
show_version(void)
{
	printf("%s version %s\n", armsd_progname, armsd_version);
	printf("libarms %s\n", arms_library_ver_string());
}

static const char *
make_version_log(void)
{
	static char buf[80];

	snprintf(buf, sizeof(buf),
		 "%s version %s, with libarms %s",
	         armsd_progname,
		 armsd_version,
	         arms_library_ver_string());

	return buf;
}

static void
usage(void)
{
	printf("usage: armsd [-dV] [-f config] [-i distid] [-l logfile] [-p port]\n");
}

static void
cleanup(void)
{
	armsdir_remove();
	pidfile_remove();
	log_close();
}

static int
callback_app_event(void *u)
{
	const char *script;
	const char *argv[2];
	int timeout;
	int ret;

	script = dconf_get_string("script-app-event");
	timeout = dconf_get_int("timeout");

	if (script != NULL) {
		argv[0] = script;
		argv[1] = NULL;
		ret = script_exec(argv, timeout);
		if (ret != 0) {
			logit(LOG_INFO, "line status changed.");
			return ARMS_EPUSH;
		}
	}

	return 0;
}

static int
callback_heartbeat(arms_context_t *ctx, void *u)
{
	if (dconf_get_int("hb-dummy") == 0)
		sys_heartbeat_prepare(ctx);
	else
		heartbeat_prepare_dummy(ctx);

#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &hb_lastsent);
#else
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	hb_lastsent.tv_sec = tv.tv_sec;
	hb_lastsent.tv_nsec = tv.tv_usec * 1000;
}
#endif

	return 0;
}

static int
callback_clear_status(uint32_t id,
		      const char *request_buff, size_t request_len,
		      char *result_buff, size_t result_len,
		      int *next, void *u)
{
	struct module *mod;
	int ret;

	if (opt_verbose)
		logit(LOG_INFO, "clear-status callback called: id=%lu",
				 (unsigned long)id);
	if ((mod = module_find(id)) == NULL) {
		logit(LOG_WARNING, "no such module: id=%lu",
				   (unsigned long)id);
		return -1;
	}
	ret = callback_generic(id, request_buff, request_len, result_buff,
				result_len, next, u,
				"script-clear", &mod->clear_state, 1);
	return (ret == ARMS_EAPPEXEC ? ARMS_ESYSTEM : ret); /* XXX */
}

static int
callback_command(uint32_t id, int action,
		 const char *request_buff, size_t request_len,
		 char *result_buff, size_t result_len,
		 int *next, void *u)
{
	switch (action) {
	case ARMS_PUSH_CLEAR_STATUS:
		return callback_clear_status(id, request_buff, request_len,
					     result_buff, result_len, next, u);
	case ARMS_PUSH_DUMP_DEBUG:
		return callback_dump_debug(result_buff, result_len, next, u);
	case ARMS_PUSH_MD_COMMAND:
		return callback_md_command(id, request_buff, request_len,
					   result_buff, result_len, next, u);
	case ARMS_PUSH_PING:
		return callback_ping((const arms_ping_arg_t *)request_buff,
				     result_buff, result_len, next, u);
	case ARMS_PUSH_TRACEROUTE:
		return callback_traceroute((arms_traceroute_arg_t *)
					       request_buff,
				           result_buff, result_len, next, u);
	default:
		/* XXX */
		break;
	}

	return 0;
}

static int
callback_generic(uint32_t id,
		 const char *request_buff, size_t request_len,
		 char *result_buff, size_t result_len,
		 int *next, void *u,
		 const char *cmd, struct callback_state *state, int hasrequest)
{
	FILE *fp;
	size_t n, toread;
	int binary, r, timeout, ret = 0;
	const char *script;
	char idbuf[16];

	script = dconf_get_string(cmd);
	if (script == NULL)
		return -1;
	binary = (dconf_get_int("binary") > 0);
	timeout = dconf_get_int("timeout");

	snprintf(idbuf, sizeof(idbuf), "%lu", (unsigned long)id);

	if (state->out == NULL) {
		if (state->in != NULL) {
			fclose(state->in);
			state->in = NULL;
		}

		if (hasrequest) {
			const char *argv[5];

			fp = armsdir_create_tmpfile(&state->infile);
			ihcw = fwrite(request_buff, 1, request_len, fp);
			fclose(fp);

			fp = armsdir_create_tmpfile(&state->outfile);
			fclose(fp);

			argv[0] = script;
			argv[1] = idbuf;
			argv[2] = state->infile;
			argv[3] = state->outfile;
			argv[4] = NULL;
			r = script_exec(argv, timeout);

			unlink(state->infile);
			free(state->infile);
			state->infile = NULL;
		} else {
			const char *argv[4];

			fp = armsdir_create_tmpfile(&state->outfile);
			fclose(fp);

			argv[0] = script;
			argv[1] = idbuf;
			argv[2] = state->outfile;
			argv[3] = NULL;
			r = script_exec(argv, timeout);
		}

		if (r == 1) {
			logit(LOG_WARNING, "script timed out (%u sec)",
			      timeout);
			ret = ARMS_ESYSTEM;
		} else if (r == 2) {
			logit(LOG_DEBUG, "script exit with code %d",
			      script_exitcode);
			ret = ARMS_EAPPEXEC;
		} else if (r == -1) {
			logit(LOG_WARNING,
			      "callback_generic: script_exec failed");
			ret = ARMS_ESYSTEM;
		}

		state->out = fopen(state->outfile, "r");
		if (state->out == NULL)
			err(1, "fopen: %s", state->outfile);
		if (next != NULL)
			*next = ARMS_FRAG_FIRST;
	} else {
		if (next != NULL)
			*next = ARMS_FRAG_CONTINUE;
	}

	toread = result_len;
	if (!binary)
		toread--;	/* for NUL */

	n = fread(result_buff, 1, toread, state->out);
	if (!binary) {
		int i, c;
		for (i = 0; i < n; i++) {
			c = result_buff[i];
			if (!is_xml_ascii(c))
				result_buff[i] = '@';
		}
		result_buff[n] = '\0';
	}

	/* XXX: if (n < toread && !feof) */

	if (feof(state->out) || next == NULL || ret != 0) {
		fclose(state->out);
		state->out = NULL;
		unlink(state->outfile);
		free(state->outfile);
		state->outfile = NULL;

		if (next != NULL)
			*next |= ARMS_FRAG_FINISHED;
	} else {
		if (next != NULL)
			*next |= ARMS_FRAG_CONTINUE;
	}

	if (ret != 0)
		return ret;
	else if (binary)
		return ARMS_RESULT_BYTES(n);
	else
		return 0;
}

static int
callback_config(uint32_t id, const char *version, const char *infostring,
		int action, const char *buf, size_t len, int next, void *u)
{
	struct module *mod;
	int timeout;
	const char *script;
	const char *argv[6];
	int err, ret;

	if (opt_verbose)
		logit(LOG_INFO, "config callback called: id=%lu",
		      (unsigned long)id);

	timeout = dconf_get_int("timeout");

	switch (action) {
	case ARMS_PUSH_EXEC_STORED_CONFIG:
		script = dconf_get_string("script-reconfig");

		err = 0;
		for (mod = module_list_iter_first();
		     mod != NULL;
		     mod = module_list_iter_next(mod)) {
			if (opt_verbose)
				logit(LOG_INFO, "commiting config: id=%s",
				      (unsigned long)mod->idstr);

			if (!module_config_exist(mod, ARMS_CONFIG_CANDIDATE))
				continue;

			if (script != NULL) {
				argv[0] = script;
				argv[1] = mod->idstr;
				argv[2] = mod->cstate.nextversion;
				argv[3] = mod->cstate.nextinfo;
				argv[4] = module_config_path(mod,
						ARMS_CONFIG_CANDIDATE);
				argv[5] = NULL;
				ret = script_exec(argv, timeout);
				if (script_exitcode == 128) {
					logit(LOG_INFO,
					      "module syncing with reboot");
					return ARMS_EMODSYNC;
				}
				if (ret != 0)
					err++;
				else
					module_update(mod,
					    mod->cstate.nextversion,
					    mod->cstate.nextinfo);
				free(mod->cstate.nextversion);
				free(mod->cstate.nextinfo);
				mod->cstate.nextversion = NULL;
				mod->cstate.nextinfo = NULL;
			}

			/* when appending new module, RUNNING does NOT exist */
			if (module_config_exist(mod, ARMS_CONFIG_RUNNING))
				module_copy_running2backup(mod->id);
			module_copy_candidate2running(mod->id);
			module_remove_candidate(mod);
		}

		if (err != 0)
			return -1;

		return 0;

	case ARMS_PUSH_REVERT_CONFIG:
		mod = module_find(id);
		if (mod == NULL)
			return -1;

		script = dconf_get_string("script-reconfig");
		if (script != NULL) {
			argv[0] = script;
			argv[1] = mod->idstr;
			argv[2] = mod->version;
			argv[3] = mod->infostring;
			argv[4] = module_config_path(mod, ARMS_CONFIG_BACKUP);
			argv[5] = NULL;
			script_exec(argv, timeout);
		}

		module_copy_backup2running(id);
		return 0;

	case ARMS_REMOVE_MODULE:
		mod = module_find(id);
		if (mod == NULL)
			return -1;

		script = dconf_get_string("script-stop");
		if (script != NULL) {
			argv[0] = script;
			argv[1] = mod->idstr;
			argv[2] = NULL;
			script_exec(argv, timeout);
		}

		module_delete(mod);
		return 0;

	case ARMS_PULL_STORE_CONFIG:
	case ARMS_PUSH_STORE_CONFIG:
		break;

	default: /* unknown action */
		return -1;
	}

	mod = module_find(id);
	if (mod == NULL)
		mod = module_new(id, version, infostring);

	if (next & ARMS_FRAG_FIRST) {
		memset(&mod->cstate, 0, sizeof(mod->cstate));
		mod->cstate.fp = armsdir_create_tmpfile(&mod->cstate.tmpfile);
		if (action == ARMS_PUSH_STORE_CONFIG) {
			mod->cstate.nextversion = strdup((version != NULL) ? version : "-");
			mod->cstate.nextinfo = strdup((infostring != NULL) ? infostring : "-");
		}
	}

	if (len > 0)
		ihcw = fwrite(buf, 1, len, mod->cstate.fp);

	if (next & ARMS_FRAG_FINISHED) {
		fclose(mod->cstate.fp);
		mod->cstate.fp = NULL;

		if (action == ARMS_PULL_STORE_CONFIG) {
			script = dconf_get_string("script-start");
			if (script != NULL) {
				argv[0] = script;
				argv[1] = mod->idstr;
				argv[2] = mod->version;
				argv[3] = mod->infostring;
				argv[4] = mod->cstate.tmpfile;
				argv[5] = NULL;
				script_exec(argv, timeout);
			}

			module_copy_running(id, mod->cstate.tmpfile);
			module_copy_running2backup(id);
			module_copy_running2startup(id);
			module_remove_candidate(mod);
		} else if (action == ARMS_PUSH_STORE_CONFIG)
			module_copy_candidate(id, mod->cstate.tmpfile);

		free(mod->cstate.tmpfile);
		mod->cstate.tmpfile = NULL;
	}
	return 0;
}

static int
callback_dump_debug(char *result_buff, size_t result_len, int *next, void *u)
{
	arms_connection_info_t conninfo;
	arms_hbt_info_t hbinfo[5];
	arms_rs_info_t rsinfo[5];
	arms_url_t urlinfo[5];
	struct timespec now;
	char buf[100];
	int i, ret;

	if (opt_verbose)
		logit(LOG_INFO, "dump-debug callback called");

	result_buff[0] = '\0';

	/* My Info */
	distid2str(&distid, buf, sizeof(buf));
	strlcat(result_buff, "Distribution-ID: ", result_len);
	strlcat(result_buff, buf, result_len);
	strlcat(result_buff, "\n", result_len);

	strlcat(result_buff, "Library Version: ", result_len);
	strlcat(result_buff, arms_library_ver_string(), result_len);
	strlcat(result_buff, "\n\n", result_len);

	/* Push Status */
	ret = arms_get_connection_info(arms_context, &conninfo,
	    sizeof(conninfo));
	if (ret == 0) {
		strlcat(result_buff, "Confirmed Protocol: ", result_len);
		switch (conninfo.af) {
		case AF_INET:
			strlcat(result_buff, "IPv4", result_len);
			break;
		case AF_INET6:
			strlcat(result_buff, "IPv6", result_len);
			break;
		default:
			strlcat(result_buff, "unknown", result_len);
			break;
		}
		strlcat(result_buff, "\n", result_len);
	
		switch (conninfo.method) {
		case ARMS_PUSH_METHOD_SIMPLE:
			strlcat(result_buff, "Push Method: simple\n",
			    result_len);
			strlcat(result_buff, "SA Address: ", result_len);
			strlcat(result_buff, conninfo.un.simple_info.sa_address,
			    result_len);
			strlcat(result_buff, "\n", result_len);
			break;
		case ARMS_PUSH_METHOD_TUNNEL:
			strlcat(result_buff, "Push Method: tunnel\n",
			    result_len);
			strlcat(result_buff, "Active Tunnel List: ",
			    result_len);
			for (i = 0; i < MAX_RS_INFO; i++) {
				if (conninfo.un.tunnel_info[i]
				    == ARMS_TUNNEL_ACTIVE) {
					snprintf(buf, sizeof(buf), "%d ", i);
					strlcat(result_buff, buf, result_len);
				}
			}
			strlcat(result_buff, "\n", result_len);
			break;
		default:
			strlcat(result_buff, "Push Method: unkown\n",
			    result_len);
			break;
		}
	}

	strlcat(result_buff, "Proposed Push Port: ", result_len);
	if ((ret = arms_get_proposed_push_port(arms_context)) > 0) {
		snprintf(buf, sizeof(buf), "%d", ret);
		strlcat(result_buff, buf, result_len);
	} else {
		strlcat(result_buff, "(none)", result_len);
	}
	strlcat(result_buff, "\n", result_len);

	strlcat(result_buff, "Proposed Push Timeout: ", result_len);
	if ((ret = arms_get_proposed_push_timeout(arms_context)) > 0) {
		snprintf(buf, sizeof(buf), "%d", ret);
		strlcat(result_buff, buf, result_len);
	} else {
		strlcat(result_buff, "(none)", result_len);
	}
	strlcat(result_buff, "\n", result_len);

#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &now);
#else
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	now.tv_sec = tv.tv_sec;
	now.tv_nsec = tv.tv_usec * 1000;
}
#endif
	snprintf(buf, sizeof(buf), "Heartbeat Last Sent: %d sec ago\n\n",
	    (int)(now.tv_sec - hb_lastsent.tv_sec));
	strlcat(result_buff, buf, result_len);

	/* Server Address*/
	ret = arms_get_rsinfo(arms_context, rsinfo, sizeof(rsinfo));
	for (i = 0; i < ret; i++) {
		snprintf(buf, sizeof(buf), "Push-Address.%d: ", i);
		strlcat(result_buff, buf, result_len);
		strlcat(result_buff, rsinfo[i].host, result_len);
		strlcat(result_buff, "\n", result_len);
	}

	ret = arms_get_rs_tunnel_url(arms_context, urlinfo, sizeof(urlinfo));
	for (i = 0; i < ret; i++) {
		snprintf(buf, sizeof(buf), "Tunnel-Url.%d: ", i);
		strlcat(result_buff, buf, result_len);
		strlcat(result_buff, urlinfo[i].url, result_len);
		strlcat(result_buff, "\n", result_len);
	}

	ret = arms_get_rs_url(arms_context, urlinfo, sizeof(urlinfo));
	for (i = 0; i < ret; i++) {
		snprintf(buf, sizeof(buf), "Pull-URL.%d: ", i);
		strlcat(result_buff, buf, result_len);
		strlcat(result_buff, urlinfo[i].url, result_len);
		strlcat(result_buff, "\n", result_len);
	}
	strlcat(result_buff, "\n", result_len);

	/* Heartbeat */
	ret = arms_get_hbtinfo(arms_context, hbinfo, sizeof(hbinfo));
	if (ret > 0) {
		snprintf(buf, sizeof(buf), "Heartbeat Interval: %d\n",
		    hbinfo[0].interval);
		strlcat(result_buff, buf, result_len);

		for (i = 0;  i < ret; i++) {
			snprintf(buf, sizeof(buf), "HB-Server.%d: ", i);
			strlcat(result_buff, buf, result_len);
			strlcat(result_buff, hbinfo[i].host, result_len);

			snprintf(buf, sizeof(buf), "#%d\n", hbinfo[i].port);
			strlcat(result_buff, buf, result_len);
		}

		/* XXX assume all hb server use same algorithm */
		strlcat(result_buff, "HB-Algorithm: ", result_len);
		for (i = 0; i < hbinfo[0].numalg; i++) {
			strlcat(result_buff, hbinfo[0].algorithm[i],
			    result_len);
		}
		strlcat(result_buff, "\n", result_len);
	}

	return 0;
}

static int
callback_get_status(uint32_t id, const char *request_buff, size_t request_len,
		    char *result_buff, size_t result_len, int *next, void *u)
{
	struct module *mod;
	int ret;

	if (opt_verbose)
		logit(LOG_INFO, "get-status callback called: %u", (unsigned)id);
	if ((mod = module_find(id)) == NULL)
		return -1;
	ret = callback_generic(id, request_buff, request_len,
				result_buff, result_len, next, u,
				"script-status", &mod->get_status_state, 1);
	return (ret == ARMS_EAPPEXEC ? ARMS_ESYSTEM : ret); /* XXX */
}

static int
callback_line_ctrl(int line_action, int line_type, void *line_conf, void *u)
{
	const char *actionstr, *script;
	const char *argv[11];
	char ifidxstr[11], cidstr[11];
	int i, timeout, ret, script_ret;

	script = dconf_get_string("script-line-ctrl");
	timeout = dconf_get_int("timeout");

	switch (line_action) {
	case ARMS_LINE_ACT_CONNECT:
		actionstr = "connect";
		ret = ARMS_LINE_CONNECTED;
		break;
	case ARMS_LINE_ACT_DISCONNECT:
		actionstr = "disconnect";
		ret = ARMS_LINE_DISCONNECTED;
		break;
	case ARMS_LINE_ACT_STATUS:
		actionstr = "status";
		ret = ARMS_LINE_CONNECTED;
		break;
	default:
		logit(LOG_ERR, "line-ctrl callback called: unknown action");
		actionstr = "(unknown)";
		ret = -1;
		break;
	}

	i = 0;
	argv[i++] = script;
	argv[i++] = actionstr;
	if (line_type == ARMS_LINE_PPPOE) {
		arms_line_conf_pppoe_t *pppoe = line_conf;
		snprintf(ifidxstr, sizeof(ifidxstr), "%d", pppoe->ifindex);
		argv[i++] = "pppoe";
		argv[i++] = ifidxstr;
		argv[i++] = pppoe->id;
		argv[i++] = pppoe->pass;
	} else if (line_type == ARMS_LINE_PPPOE_IPV6) {
		arms_line_conf_pppoe_t *pppoe = line_conf;
		snprintf(ifidxstr, sizeof(ifidxstr), "%d", pppoe->ifindex);
		argv[i++] = "pppoe6";
		argv[i++] = ifidxstr;
		argv[i++] = pppoe->id;
		argv[i++] = pppoe->pass;
	} else if (line_type == ARMS_LINE_DHCP) {
		arms_line_conf_dhcp_t *dhcp = line_conf;
		snprintf(ifidxstr, sizeof(ifidxstr), "%d", dhcp->ifindex);
		argv[i++] = "dhcp";
		argv[i++] = ifidxstr;
	} else if (line_type == ARMS_LINE_MOBILE) {
		arms_line_conf_mobile_t *mobile = line_conf;
		snprintf(ifidxstr, sizeof(ifidxstr), "%d", mobile->ifindex);
		snprintf(cidstr, sizeof(cidstr), "%d", mobile->cid);
		argv[i++] = "mobile";
		argv[i++] = ifidxstr;
		argv[i++] = mobile->id;
		argv[i++] = mobile->pass;
		argv[i++] = mobile->telno;
		argv[i++] = cidstr;
		argv[i++] = mobile->apn;
		argv[i++] = mobile->pdp;
	} else if (line_type == ARMS_LINE_STATIC) {
		;
	} else if (line_type == ARMS_LINE_RA) {
		arms_line_conf_ra_t *ra = line_conf;
		snprintf(ifidxstr, sizeof(ifidxstr), "%d", ra->ifindex);
		argv[i++] = "ra";
		argv[i++] = ifidxstr;
	} else {
		logit(LOG_ERR, "line-ctrl callback called: unknown type");
		argv[i++] = "(unknown)";
		ret = -1;
	}
	argv[i++] = NULL;

	if (ret < 0)
		return ret;

	if (opt_verbose)
		logit(LOG_INFO, "line-ctrl callback called: "
				"action=%s, type=%s", actionstr, argv[2]);
	if (script == NULL || line_type == ARMS_LINE_STATIC || ret < 0)
		return ret;

	script_ret = script_exec(argv, timeout);
	if (script_ret == 1) {
		logit(LOG_WARNING, "script-line-ctrl timed out");
		return -1;
	}

	if (script_exitcode == 128)
		ret = ARMS_LINE_NEEDPOLL;
	else if (script_exitcode == 129)
		ret = ARMS_LINE_CONNECTED;
	else if (script_exitcode == 130)
		ret = ARMS_LINE_DISCONNECTED;
	else if (script_exitcode == 131)
		ret = ARMS_LINE_TIMEOUT;
	else if (script_exitcode == 132)
		ret = ARMS_LINE_AUTHFAIL;
	else {
		logit(LOG_ERR, "script-line-ctrl return unknown exit status");
		ret = -1;
	}

	return ret;
}

static int
callback_log(int log_code, const char *str, void *u)
{
	if (ARMS_LOG_TYPE(log_code) == ARMS_LOG_DEBUG)
		logit(LOG_DEBUG, "log callback: %d - %s", log_code, str); 
	else
		logit(LOG_INFO, "log callback: %d - %s", log_code, str); 

	return 0;
}

static int
callback_md_command(uint32_t id,
		    const char *request_buff, size_t request_len,
		    char *result_buff, size_t result_len,
		    int *next, void *u)
{
	struct module *mod;

	if (opt_verbose)
		logit(LOG_INFO, "md-command callback: %lu", (unsigned long)id); 
	if ((mod = module_find(id)) == NULL)
		return -1;
	/* ARMS_EAPPEXEC defined since libarms 5.20 */
	return callback_generic(id, request_buff, request_len, result_buff,
				result_len, next, u,
				"script-command", &mod->md_command_state, 1);
}

static int
callback_ping(const arms_ping_arg_t *arg,
	      char *result_buff, size_t result_len,
	      int *next, void *u)
{
	struct arms_ping_report *rep = (struct arms_ping_report *)result_buff;

	if (opt_verbose)
		logit(LOG_INFO,
		      "ping callback called: dst=%s, count=%d, size=%d",
		      arg->dst, arg->count, arg->size);

	if (result_len < sizeof(*rep))
		return -1;

	return sys_ping(arg, rep);
}

static int
callback_read_config(uint32_t id, int type,
		     char *result_buff, size_t result_len,
		     int *next, void *u)
{
	struct callback_state *state;
	struct module *mod;
	size_t n, toread;
	int binary;
	const char *dir, *typestr;
	char filename[PATHLEN];

	mod = module_find(id);
	if (!module_config_exist(mod, type)) {
		logit(LOG_ERR, "read-config (module #%lu, %s) requested "
			       "but we don't have that.",
			       mod->id, arms_config_string(type));
		return -1;
	}

	state = &mod->read_config_state;
	binary = (dconf_get_int("binary") > 0);

	if (*next == ARMS_FRAG_FIRST) {
		state->in = NULL;
		state->infile = NULL;
		state->out = NULL;
		state->outfile = NULL;
		state->nul = 0;		/* not used */

		switch (type) {
		case ARMS_CONFIG_BACKUP: typestr = "backup-config"; break;
		case ARMS_CONFIG_CANDIDATE: typestr = "candidate-config"; break;
		case ARMS_CONFIG_RUNNING: typestr = "running-config"; break;
		default: typestr = "(unknown)"; break;
		}
		if (opt_verbose)
			logit(LOG_INFO, "read-config callback: id=%lu, type=%s",
					(unsigned long)id, typestr);

		if (type == ARMS_CONFIG_BACKUP)
			dir = armsdir.backupdir;
		else if (type == ARMS_CONFIG_CANDIDATE)
			dir = armsdir.candidatedir;
		else /* ARMS_CONFIG_RUNNING */
			dir = armsdir.runningdir;

		snprintf(filename, sizeof(filename),
			 "%s/%lu", dir, (unsigned long)id);
		state->outfile = strdup(filename);
		if (state->outfile == NULL)
			err(1, "no memory");

		state->out = fopen(state->outfile, "r");
		if (state->out == NULL)
			return -1;

		*next = ARMS_FRAG_FIRST;

	} else /* ARMS_FRAG_CONTINUE */ {
		if (state->out == NULL)
			return -1;

		*next = 0;
	}

	toread = result_len;
	if (!binary)
		toread--;	/* for NUL */

	n = fread(result_buff, 1, toread, state->out);
	if (!binary)
		result_buff[n] = '\0';

	/* XXX: ferror */
	if (feof(state->out)) {
		fclose(state->out);
		state->out = NULL;
		state->outfile = NULL;
		*next |= ARMS_FRAG_FINISHED;
	} else
		*next |= ARMS_FRAG_CONTINUE;

	if (binary)
		return ARMS_RESULT_BYTES(n);
	else
		return 0;
}

static int
callback_state(int old, int new, void *u)
{
	const char *argv[4];
	const char *script;
	int timeout;

	logit(LOG_INFO, "state changed: %s -> %s",
			arms_state_string(old), arms_state_string(new));

	script = dconf_get_string("script-state-changed");
	timeout = dconf_get_int("timeout");

	if (script != NULL) {
		argv[0] = script;
		argv[1] = arms_state_string(old);
		argv[2] = arms_state_string(new);
		argv[3] = NULL;
		(void)script_exec(argv, timeout);
	}

	if (new == ARMS_ST_LSPULL)
		callback_state_set_proxy("https-proxy-url-ls");
	else if (new == ARMS_ST_RSPULL)
		callback_state_set_proxy("https-proxy-url-rs");

	return 0;
}

static void
callback_state_set_proxy(const char *var)
{
	const char *proxy;

	proxy = dconf_get_string(var);
	if (proxy == NULL)
		proxy = dconf_get_string("https-proxy-url");
	if (proxy == NULL)
		return;

	if (proxy[0] == '\0') {
		logit(LOG_INFO, "use no https proxy");
		arms_set_https_proxy(arms_context, NULL);
	} else {
		logit(LOG_INFO, "use https proxy: %s", proxy);
		arms_set_https_proxy(arms_context, proxy);
	}
}

static int
callback_traceroute(const arms_traceroute_arg_t *arg,
		    char *result_buff, size_t result_len,
		    int *next, void *u)
{
	struct arms_traceroute_info *tr = (void *)result_buff;
	int n;

	if (opt_verbose)
		logit(LOG_INFO, "traceroute callback called: "
				"addr=%s, count=%d, maxhop=%d",
				arg->addr, arg->count, arg->maxhop);

	n = result_len / sizeof(*tr);
	if (n < 0)
		return -1;

	return sys_traceroute(arg, tr, n);
}

void
script_clearlog(void)
{
	scriptlog.bufsize = sizeof(scriptlog.buf);
	scriptlog.readptr = 0;
	scriptlog.writeptr = 0;
}

void
script_errlog(const char *errmsg, int len)
{
	struct scriptlog *l = &scriptlog;
	char *p;

	if (l->writeptr + 1 >= l->bufsize)
		return;

	if (l->writeptr + len > l->bufsize)
		len = l->bufsize - l->writeptr;
	script_logcpy(l->buf + l->writeptr, errmsg, len);
	l->writeptr += len;
	if (l->writeptr == l->bufsize)
		l->buf[l->bufsize - 1] = '\0';

	while (l->readptr < l->writeptr) {
		p = memchr(l->buf + l->readptr, '\0',
			   l->writeptr - l->readptr);
		if (p == NULL)
			break;
		logit(LOG_WARNING, "script unhandled error: %s",
		      l->buf + l->readptr);
		l->readptr = (p + 1) - l->buf;
	}
}

void
script_initenv(void)
{
	char buf[100];

	distid2str(&distid, buf, sizeof(buf));
	setenv("ARMS_DISTRIBUTION_ID", buf, 1);
}

void
script_logcpy(char *dst, const char *src, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		if (src[i] == '\n')
			dst[i] = '\0';
		else if (src[i] == '\0' || !isgraph((int)src[i]))
			dst[i] = ' ';
		else
			dst[i] = src[i];
	}
}

/*
 * return: 0 = success, 1 = timeout, 2 = non-zero exit code, -1 = error.
 */
int
script_exec(const char **argv, unsigned int timeout)
{
	struct timeout *timo;
	pid_t pid;
	int e, pipefds[2];

	if (pipe(pipefds) == -1) {
		warn("pipe");
		return -1;
	}

	timo = timeout_new(timeout);
	if (timo == NULL) {
		close(pipefds[0]);
		close(pipefds[1]);
		return -1;
	}

	e = 0;
	script_pid = -1;
	script_exitcode = -1;
	script_clearlog();

	pid = fork();
	if (pid == 0) {
		/* child */
		signal(SIGTERM, SIG_DFL);
		signal(SIGHUP, SIG_DFL);
		signal(SIGINT, SIG_DFL);
		close(pipefds[0]);
		if (dup2(pipefds[1], STDERR_FILENO) == -1) {
			logit(LOG_ERR, "cannot exec script - dup2: %s",
				       strerror(errno));
			_exit(1);
		}
		close(pipefds[1]);
		/* we are sure argv[i] is on heap or stack... */
		execv(argv[0], (char **)argv);
		_exit(127);
		/* NOTREACHED */
	} else if (pid != -1) {
		struct pollfd pollfd, *fds;
		int n, nfds, t;
		char buf[256];

		script_pid = pid;
		close(pipefds[1]);

		pollfd.fd = pipefds[0];
		pollfd.events = POLLIN;
		fds = &pollfd;
		nfds = 1;

		while (1) {
			if (kill(pid, 0) == -1) {
				/*
				 * script has exited, but we don't know its
				 * succeeded or not.
				 */
				pid = -1;
				break;
			}

			t = timeout_remaining(timo);
			if (t <= 0) {
				logit(LOG_WARNING,
				       "executing script timed out");
				e = 1; /* timed out */
				break;
			}

			n = poll(fds, nfds, t);
			if (n == 0) {
				e = 1; /* timed out */
				break;

			} else if (n == -1) {
				if (errno == EINTR)
					continue;	/* retry */
				else {
					logit(LOG_ERR, "poll: %s",
						       strerror(errno));
					e = -1;	/* error */
					break;
				}
			}
			if (fds->revents & POLLIN) {
				n = read(fds->fd, buf, sizeof(buf));
				if (n > 0)
					script_errlog(buf, n);
				else if (n == 0) {
					/* child process closed stderr */
					fds = NULL;
					nfds = 0;
				}
			} else if (fds->revents & ~(POLLIN|POLLHUP)) {
				logit(LOG_ERR, "unknown poll event (0x%x)",
				      fds->revents);
				e = -1;
				break;
			}
		}
		if (pid != -1)
			kill(pid, SIGTERM);
		close(pipefds[0]);
		timeout_free(timo);
	} else {
		close(pipefds[0]);
		close(pipefds[1]);
		timeout_free(timo);
		return -1;
	}

	if (e != 0)
		return e;

	if (script_exitcode == -1)
		sleep(1);		/* waiting SIGCHLD */
	if (script_exitcode == 0)
		return 0;
	else
		return 2;
}

struct timeout *
timeout_new(unsigned int sec)
{
	struct timeout *timo;

	timo = malloc(sizeof(struct timeout));
	if (timo == NULL)
		return NULL;
#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &timo->ts);
#else
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	timo->ts.tv_sec = tv.tv_sec;
	timo->ts.tv_nsec = tv.tv_usec * 1000;
}
#endif
	timo->sec = sec;
	if (timo->ts.tv_sec + sec < timo->ts.tv_sec) {
		/* overflow!  sec is too large? */
		free(timo);
		return NULL;
	}
	return timo;
}

void
timeout_free(struct timeout *timo)
{
	free(timo);
}

int
timeout_remaining(struct timeout *timo)
{
	struct timespec now;
	long sec, nsec;

#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &now);
	if (now.tv_sec < timo->ts.tv_sec)
		return -1;	/* is it really a monotonic clock? */
#else
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	now.tv_sec = tv.tv_sec;
	now.tv_nsec = tv.tv_usec * 1000;
	if (now.tv_sec < timo->ts.tv_sec) {
		/*
		 * It should not happen but it happens when system clock
		 * goes back (by date(1) or NTP?).  We restart the timer
		 * so the actual duration of timeout will be at most twice
		 * as long as intended.  If system clock goes back again
		 * in the duration, the system is broken and we don't
		 * care about it.
		 */
		timo->ts = now;
		return timo->sec;
	}
}
#endif

	sec = timo->ts.tv_sec + timo->sec - now.tv_sec;
	nsec = timo->ts.tv_nsec - now.tv_nsec;
	if (nsec < 0) {
		sec -= 1;
		nsec += 1000000000L;
	}
	if (sec < 0)
		return 0;	/* timed out */
	if (sec + 1 >= INT_MAX / 1000)
		return 0;	/* too large - should be timed out */

	/* remaining time in milliseconds */
	return (int)(sec * 1000 + nsec / 1000000);
}

static int
is_xml_ascii(int c)
{
	const char xml_ascii_ctype[] = {
		0,
		0, 0, 0, 0, 0, 0, 0, 0, /* 00 */
		0, 1, 1, 0, 0, 1, 0, 0, /* 08 */
		0, 0, 0, 0, 0, 0, 0, 0, /* 10 */
		0, 0, 0, 0, 0, 0, 0, 0, /* 18 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 20 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 28 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 30 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 38 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 40 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 48 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 50 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 58 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 60 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 68 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 70 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 78 */
		0, 0, 0, 0, 0, 0, 0, 0, /* 80 */
		0, 0, 0, 0, 0, 0, 0, 0, /* 88 */
		0, 0, 0, 0, 0, 0, 0, 0, /* 90 */
		0, 0, 0, 0, 0, 0, 0, 0, /* 98 */
		0, 0, 0, 0, 0, 0, 0, 0, /* a0 */
		0, 0, 0, 0, 0, 0, 0, 0, /* a8 */
		0, 0, 0, 0, 0, 0, 0, 0, /* b0 */
		0, 0, 0, 0, 0, 0, 0, 0, /* b8 */
		0, 0, 0, 0, 0, 0, 0, 0, /* c0 */
		0, 0, 0, 0, 0, 0, 0, 0, /* c8 */
		0, 0, 0, 0, 0, 0, 0, 0, /* d0 */
		0, 0, 0, 0, 0, 0, 0, 0, /* d8 */
		0, 0, 0, 0, 0, 0, 0, 0, /* e0 */
		0, 0, 0, 0, 0, 0, 0, 0, /* e8 */
		0, 0, 0, 0, 0, 0, 0, 0, /* f0 */
		0, 0, 0, 0, 0, 0, 0, 0  /* f8 */
	};

	return (c == -1 ? 0 : ((xml_ascii_ctype + 1)[(unsigned char)c]));
}


static int
load_from_file(const char *fname, char **buffp)
{
	FILE *fp;
	char *buff;
	size_t l;

	fp = fopen(fname, "r");
	if (fp == NULL) {
		fprintf(stderr, "fopen(%s) failed: %s\n",
		    fname, strerror(errno));
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	l = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	buff = malloc(l + 1);
	if (buff == NULL) {
		fprintf(stderr, "malloc(%ld) failed: %s\n",
		    (long)l + 1, strerror(errno));
		fclose(fp);
		return -1;
	}

	l = fread(buff, 1, l, fp);
	if (l == 0 && !feof(fp)) {
		fprintf(stderr, "fread() failed: %s\n", strerror(errno));
		free(buff);
		fclose(fp);
		return -1;
	}
	*(buff + l) = '\0';

	*buffp = buff;
	fclose(fp);
	return l;
}

static void
load_state_cache(void)
{
	FILE *fp;
	size_t cachesize;
	const char *cachefile;
	char *buf;

	cachesize = arms_size_of_state();
	fp = NULL;
	buf = NULL;

	cachefile = dconf_get_string("path-state-cache");
	if (cachefile == NULL)
		cachefile = PATHSTATECACHE;

	fp = fopen(cachefile, "r");
	if (fp == NULL) {
		logit(LOG_INFO, "no state cache file - try LS-PULL");
		return;
	}

	if (fseek(fp, 0, SEEK_END) != 0) {
		logit(LOG_WARNING, "fseek() failed: %s", strerror(errno));
		goto eret;
	}

	if (ftell(fp) != (long)cachesize) {
		logit(LOG_WARNING, "size of state cache is not expected - "
				   "ignore it");
		goto eret;
	}
	if (fseek(fp, 0, SEEK_SET) != 0) {
		logit(LOG_WARNING, "fseek() failed: %s", strerror(errno));
		goto eret;
	}

	buf = malloc(cachesize);
	if (buf == NULL) {
		/* really? */
		goto eret;
	}
	if (fread(buf, 1, cachesize, fp) != cachesize) {
		logit(LOG_WARNING, "fread() failed: %s", strerror(errno));
		goto eret;
	}
	fclose(fp);
	fp = NULL;

	if (arms_restore_state(arms_context, buf, cachesize) != 0)
		logit(LOG_WARNING, "cannot restore state cache");

	free(buf);
	buf = NULL;
	return;

eret:
	if (fp != NULL)
		fclose(fp);
	if (buf != NULL)
		free(buf);
}

static void
save_state_cache(void)
{
	FILE *fp;
	size_t cachesize;
	const char *cachefile;
	char *cache;

	cachefile = dconf_get_string("path-state-cache");
	if (cachefile == NULL)
		cachefile = PATHSTATECACHE;

	cachesize = arms_size_of_state();
	cache = malloc(cachesize);
	if (cache == NULL)
		return;

	if (arms_dump_state(arms_context, cache, cachesize) != 0) {
		logit(LOG_WARNING, "cannot save state cache (ignored): "
				   "arms_dump_state() failed");
		free(cache);
		return;
	}

	fp = fopen(cachefile, "w");
	if (fp == NULL) {
		logit(LOG_WARNING, "cannot save state cache (ignored): "
				   "%s: %s", cachefile, strerror(errno));
		free(cache);
		return;
	}

	ihcw = fwrite(cache, 1, cachesize, fp);
	fclose(fp);
	free(cache);

	logit(LOG_INFO, "state cache saved");
}

static int
load_cert(arms_context_t *ctx)
{
	const char *filename;
	const char *cert;
	char *ca = NULL;
	int ret = -1;

	filename = dconf_get_string("path-root-ca");
	if (filename == NULL)
		cert = arms_root_ca_certificate;
	else {
		ret = load_from_file(filename, &ca);
		if (ret < 0) {
			fprintf(stderr, "cannot read file %s\n", "cacert.pem");
			ret = -1;
			goto failure;
		}
		cert = ca;
	}

	ret = arms_register_cert(ctx, cert);
	if (ret != 0) {
		fprintf(stderr, "arms_register_cert() failed\n");
		goto failure;
	}

failure:
	free(ca);
	return ret;
}

static int
load_initial_config(arms_context_t *ctx)
{
	const char *filename;
	char *config = NULL;
	int len;
	int err = -1;

	filename = dconf_get_string("path-iconfig");
	if (filename == NULL)
		/* use libarms embedded initial config */
		return 0;

	len = load_from_file(filename, &config);
	if (len < 0) {
		fprintf(stderr, "cannot read file %s\n", filename);
		return -1;
	}

	err = arms_load_config(ctx, config, len);
	if (err != 0) {
		fprintf(stderr, "cannot load config %s (%d)\n",
			filename, err);
		free(config);
		return -1;
	}

	if (config)
		free(config);

	return 0;
}

static int
str2distid(const char *s, distribution_id_t *distid)
{
	uint64_t code;
	unsigned int x[8];
	int n;

	n = sscanf(s, "%x-%x-%x-%x-%x-%x-%x-%x",
	           &x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &x[6], &x[7]);
	if (n != 8)
		return -1;

	distid->version = x[0];
	distid->vendor_code = (x[1] << 16) + x[2];
	distid->sa_type = x[3];

	code = x[4];
	code = (code << 16) + x[5];
	code = (code << 16) + x[6];
	code = (code << 16) + x[7];
	distid->sa_code = code;

	return 0;
}

static int
distid2str(const distribution_id_t *distid, char *s, unsigned int n)
{
	int k;

	k = snprintf(s, n, "%04X-%04X-%04X-%04X-%04X-%04X-%04X-%04X",
		/* version (16bit) */
		distid->version & 0xffff,

		/* vendor (32bit) */
		(distid->vendor_code >> 16) & 0xffff,
		distid->vendor_code & 0xffff,

		/* sa_type (16bit) */
		distid->sa_type & 0xffff,

		/* sa_code (64bit) */
		(unsigned int)((distid->sa_code >> 48) & 0xffff),
		(unsigned int)((distid->sa_code >> 32) & 0xffff),
		(unsigned int)((distid->sa_code >> 16) & 0xffff),
		(unsigned int)(distid->sa_code & 0xffff));

	return (k == 39) ? 0 : -1;
}

void
sighandler_init(void)
{
	struct sigaction act;

	/* XXX: other signals */
	signal(SIGHUP, sighandler_reload);
	signal(SIGINT, sighandler_exit);
	signal(SIGTERM, sighandler_exit);
	signal(SIGPIPE, SIG_IGN);

	memset(&act, 0, sizeof(act));
	act.sa_handler = sighandler_child;
	act.sa_flags = 0;			/* unset SA_RESTART */
	sigemptyset(&act.sa_mask);
	sigaction(SIGCHLD, &act, NULL);
}

void
sighandler_child(int sig)
{
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) != -1) {
		if (pid == script_pid && WIFEXITED(status))
			script_exitcode = WEXITSTATUS(status);
	}
}

void
sighandler_exit(int signo)
{
	if (getpid() != armsd_pid) {
		logit(LOG_INFO, "signal %d caught by child process", signo);
		return;
	}
	logit(LOG_INFO, "signal %d caught. exit.", signo);
	cleanup();
	exit(1);
}

void
sighandler_reload(int signo)
{
	if (getpid() != armsd_pid) {
		logit(LOG_INFO, "signal %d caught by child process", signo);
		return;
	}
	logit(LOG_INFO, "sighup received: reload configuration");
	if (dconf_load() == -1)
		logit(LOG_WARNING, "some errors in configuration detected");
}

static void
sa_stop_modules(void)
{
	struct module *mod;
	int timeout;
	const char *argv[3];
	
	timeout = dconf_get_int("timeout");
	argv[0] = dconf_get_string("script-stop");
	argv[2] = NULL;

	while ((mod = module_list_iter_first()) != NULL) {
		argv[1] = mod->idstr;
		(void)script_exec(argv, timeout);
		/* ignore error */

		module_delete(mod);
	}
}

static void
sa_reboot(void)
{
	int timeout;
	const char *argv[2];

	timeout = dconf_get_int("timeout");
	argv[0] = dconf_get_string("script-reboot");
	argv[1] = NULL;

printf("sa_rebooting: %s...\n", argv[0]);
	(void)script_exec(argv, timeout);
}

static int
sa_post_pull(void)
{
	int timeout, ret;
	const char *script;
	const char *argv[2];

	timeout = dconf_get_int("timeout");
	script = dconf_get_string("script-post-pull");
	if (script == NULL)
		return 0;

	argv[0] = script;
	argv[1] = NULL;

	ret = script_exec(argv, timeout);
	if (ret != 0)
		return -1;

	return 0;
}

#ifdef TEST
#include <check.h>
#include "test/test_dir.c"
#include "test/test_log.c"
#include "test/test_misc.c"
#include "test/test_module.c"
#include "test/test_timeout.c"
#include "test/test_main.c"
#endif

int
main(int argc, char **argv)
{
	int ch, n, ret;

#ifdef TEST
	if (argc == 1)
		return test_main(argc, argv);
#endif

	/* set default values */
	arms_listen_port = 0;

	while ((ch = getopt(argc, argv, "b:dDf:Hhi:l:p:vVw")) != -1) {
		switch(ch) {
		case 'b':
			opt_basedir = optarg;
			break;
		case 'd': /* deprecated */
			opt_verbose++;
			opt_nodaemon++;
			break;
		case 'D':
			opt_nodaemon++;
			break;
		case 'f':
			configfile = optarg;
			break;
		case 'H':
			opt_hbdebug++;
			opt_verbose++;
			opt_nodaemon++;
			break;
		case 'i':
			opt_distid = optarg;
			break;
		case 'l':
			opt_logfile = optarg;
			break;
		case 'p':
			arms_listen_port = atoi(optarg);
			break;
		case 'v':
			opt_verbose++;
			break;
		case 'V':
			show_version();
			exit(0);
			/* NOTREACHED */
		case 'w':
			opt_wait_pulldone++;
			break;
		case 'h':
		default:
			usage();
			exit(0);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	/* armsd_pid will be overwrote after daemonized  */
	armsd_pid = getpid();

	dconf_load();

	/* check mandatory settings */
	if (dconf_get_string("ls-sa-key") == NULL) {
		fprintf(stderr, "no ls-sa-key found on configuration\n");
		return 1;
	}

	if (opt_distid != NULL)
		str2distid(opt_distid, &distid);
	else if (dconf_get_string("distribution-id") != NULL)
		str2distid(dconf_get_string("distribution-id"), &distid);
	else {
		fprintf(stderr, "no distribution-id\n");
		return 1;
	}

	if ((ret = arms_init(&distid, &arms_context)) != 0) {
		fprintf(stderr, "arms_init() failed: %s\n",
			arms_error_string(ret));
		return 1;
	}

	if (load_cert(arms_context) != 0) {
		printf("Cannot load certificate.\n");
		return 1;
	}

	if (load_initial_config(arms_context) != 0) {
		printf("Cannot load configuration.\n");
		return 1;
	}

	if (arms_register_authkey(arms_context,
				  dconf_get_string("ls-sa-key"))) {
		fprintf(stderr, "invalid ls-sa key\n");
		return 1;
	}

	if (arms_register_description(arms_context,
				      armsd_progname,
				      armsd_version) != 0) {
		fprintf(stderr, "arms_register_description() failed.\n");
		return 1;
	}

	if (dconf_get_string("https-proxy-url") != NULL &&
	    arms_set_https_proxy(arms_context,
	                         dconf_get_string("https-proxy-url")) != 0) {
		fprintf(stderr, "arms_set_https_proxy() failed.\n");
		return 1;
	}

	openlog("armsd", LOG_PID | LOG_NDELAY, LOG_DAEMON);
	if (opt_logfile != NULL) {
		if (log_open_file(opt_logfile) == -1)
			logit(LOG_ERR, "cannot open logfile: %s",
				       strerror(errno));
	}

	if (opt_hbdebug > 0) {
		while (1) {
			sys_heartbeat_prepare(arms_context);
			sleep(5);
		}
	}

	if (armsdir_set_base() != 0)
		exit(1);

	/* Note: daemon() changes CWD. */
	if (!opt_nodaemon && !opt_wait_pulldone) {
		if (daemon(0, 0) == -1) {
			logit(LOG_ERR, "daemon(3) failed: %s\n",
				       strerror(errno));
			cleanup();
			exit(1);
		}
		pidfile_create();
		daemonized = 1;
		logit(LOG_DEBUG, "daemonized");
	}

	logit(LOG_INFO, "%s", make_version_log());
	script_initenv();
	sighandler_init();

	if (armsdir_create() == -1) {
		logit(LOG_ERR, "cannot make resource directories");
		cleanup();
		exit(1);
	}
	load_state_cache();

pull:
	ret = arms_pull(arms_context, 0, 0, &arms_callback_table, lines,
			&arms_callback_udata);
	if (ret != 0) {
		logit(LOG_ERR, "arms_pull failed");
		goto failure;
	}
	logit(LOG_INFO, "initial configuration succeeded");
	save_state_cache();

	ret = sa_post_pull();
	if (ret != 0) {
		ret = ARMS_EREBOOT;
		goto failure;
	}

	ret = arms_push_method_query(arms_context,
				     &arms_callback_table,
				     &arms_callback_udata);
	if (ret != 0) {
		logit(LOG_INFO, "push-method-query failed: %d", ret);
		goto failure;
	}

	/* Note: daemon() changes CWD. */
	if (!opt_nodaemon && !daemonized) {
		if (daemon(0, 0) == -1) {
			logit(LOG_ERR, "daemon(3) failed: %s\n",
				       strerror(errno));
			cleanup();
			exit(1);
		}
		pidfile_create();
		daemonized = 1;
		logit(LOG_DEBUG, "daemonized");
	}

	n = dconf_get_int("app-event-interval");
	if (n != -1) {
		struct timeval tv;

		tv.tv_sec = n;
		tv.tv_usec = 0;
		ret = arms_set_app_event_interval(arms_context, &tv);
		if (ret == -1) {
			logit(LOG_WARNING,
			      "cannot set app-event-interval (%d)", n);
		}
	}

	ret = arms_event_loop(arms_context, arms_listen_port, 0,
				&arms_callback_table,
				&arms_callback_udata);

failure:
	switch (ret) {
	case 0:
	case ARMS_EREBOOT:
		logit(LOG_INFO, "reboot requested. exit.");
		break;
	case ARMS_EPULL:
		logit(LOG_INFO, "reconfigure by ARMS pull requested");
		sa_stop_modules();
		goto pull;
	case ARMS_EDONTRETRY:
		logit(LOG_ERR, "LS/RS denied pull access. exit.");
		break;
	default:
		logit(LOG_ERR, "arms system failed: "
		               "level = %d, type = %d, code = %d\n",
			       ARMS_ERR_LVL(ret), ARMS_ERR_TYPE(ret), ret);
		break;
	}
	sa_stop_modules();
	arms_end(arms_context);
	cleanup();

	if (ret != ARMS_EDONTRETRY)
		sa_reboot();

	return 0;
}
