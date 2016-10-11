/*
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@dotAster.com>
 *
 *  $Id: loader.c,v 4.6 2003/08/17 07:07:38 shuu Exp shuu $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	<limits.h>
#include	<linux/limits.h>
#include	<string.h>
#include	<sys/wait.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<sys/utsname.h>	/* for uname */

#include	"usbmgr.h"

#define	MODPROBLE_FILE "/proc/sys/kernel/modprobe"
#define	START_ARGV	2

#define	INVALID_TYPE	0
#define	OLD_TYPE	1
#define	NEW_TYPE	2

static char *module_loader = NULL;

static int
get_loader(void)
{
	int fd;
	char ld[PATH_MAX+1];
	ssize_t len;

	if ((fd = open(MODPROBLE_FILE,O_RDONLY)) == -1)
		return -1;
	if ((len = read(fd,ld,PATH_MAX)) <= 0)
		return -1;
	close(fd);
	if (ld[len-1] == '\n')
		ld[len-1] = '\0';
	else
		ld[len] = '\0';
	if((module_loader = strdup(ld)) == NULL)
		return -1;

	return 0;
}

static int
get_modutils_type(void)
{
	struct utsname uts;
	static int type = INVALID_TYPE;

	if (type != INVALID_TYPE)
		return type;
	if (uname(&uts) == -1)
		return type;	/* oops */
	if (uts.release[2] >= '5')
		type = NEW_TYPE;
	else
		type = OLD_TYPE;
	return type;
}

static int
execute(char *path,char **argv)
{
	pid_t pid;
	int status;

	pid = fork();
	switch(pid) {
	case -1:
		syslog(LOG_ERR,mesg[MSG_FORK_ERR]);
		break;
	case 0:
		execv(path,argv);
		syslog(LOG_ERR,mesg[MSG_EXEC_ERR],path);
		exit(1);
	default:	/* parent */
		wait(&status);
		break;
	}
	if (WIFEXITED(status))	/* normal exit */
		return WEXITSTATUS(status);
	return 1;

}
	
static void
load(char *loader,int argc,char **argv,int action)
{
	int modutil_type,i;
	char *new_command[4];	/* 0: loader 1: option 2: module 3: NULL */

	if ((modutil_type = get_modutils_type()) == OLD_TYPE) {
		DPRINTF(LOG_DEBUG,"load:kernel <= 2.4\n");
		argv[0] = loader;
		if (action & MODULE_LOAD)
			argv[1] = "-as";
		else
			argv[1] = "-asr";
		DPRINTF(LOG_DEBUG,"load:execute %s %s",argv[0],argv[1]);
#ifdef	DEBUG
		for (i = 2;argv[i] != NULL;i++) {
			DPRINTF(LOG_DEBUG," %s",argv[i]);
		}
		DPRINTF(LOG_DEBUG,"\n");
#endif
		execute(loader,argv);
	} else if (modutil_type == NEW_TYPE) {
		DPRINTF(LOG_DEBUG,"load:kernel >= 2.5\n");
		new_command[0] = loader;
		new_command[3] = NULL;
		if (action & MODULE_LOAD)
			new_command[1] = "-s";
		else
			new_command[1] = "-sr";
		for (i = START_ARGV;argv[i] != NULL;i++) {
			new_command[2] = argv[i];
			DPRINTF(LOG_DEBUG,"load:execute %s %s %s\n",
				new_command[0], new_command[1], new_command[2]);
			execute(loader,new_command);
		}
	}
}

/*
 * load module
 * USB_NOMODULE is not load
 */
static int 
module_load(int argc,char **name,int action)
{
	int count;

	/* name[0]:loader, name[1]:loader option */
	if (!strcmp(name[START_ARGV],USB_NOMODULE)) {
		syslog(LOG_INFO,mesg[MSG_NOT_LOAD],name[START_ARGV]);
		return GOOD;
	}
	if (module_loader == NULL) {
		if (get_loader() == -1)	{
			syslog(LOG_ERR,mesg[MSG_CANT_GET_LD]);
			return INVALID;
		}
	}
	load(module_loader,argc,name,action);
	for(count = START_ARGV;count < argc;count++) {
		syslog(LOG_INFO,mesg[MSG_DID],
			name[count],(action & MODULE_LOAD) ? "loaded" : "unloaded");
	}

	return GOOD;
}

/*
 * execute script in struct config
 */
static int
execute_script(struct node *np,char **cmd,int action)
{
	char path[PATH_MAX+1];		/* for full path of command */
	char buf[256],*bufp;
	char *envp[3];
	int status,i = 0;
	int ret = 0;

	bufp = buf;
	envp[i++] = bufp;
	bufp += sprintf(bufp,"DEVICE=/proc/bus/usb/%03d/%03d",
		NODE_BUS(np),NODE_DEVNO(np)) + 1;
	envp[i++] = bufp;
	if (action == SCRIPT_START) {
		bufp += sprintf(bufp,"ACTION=add") + 1;
	} else {
		bufp += sprintf(bufp,"ACTION=remove") + 1;
	}
	envp[i] = NULL;
	DPRINTF(LOG_DEBUG,"execute_script:used buf length %d\n",bufp - buf);
	for(i = START_ARGV;cmd[i] != NULL;i++) {
		switch(fork()) {
		case -1:
			syslog(LOG_ERR,mesg[MSG_FORK_ERR]);
			return INVALID;
		case 0:	/* child */
			if (strlen(cmd[i]) + sizeof(CONF_DIR) + 1 > PATH_MAX) {
				syslog(LOG_ERR,mesg[MSG_TOO_LONG]);
				return INVALID;
			}
			sprintf(path,"%s/%s",conf_dir,cmd[i]);
			if (debug) {
				syslog(LOG_DEBUG,"execle \"%s\":cmd[%d] \"%s\"\n",
					path,i,cmd[i]);
			}
			execle(path,cmd[i],NULL,envp);
			syslog(LOG_ERR,mesg[MSG_EXEC_ERR],path);
			exit(1);
		default:	/* parent */
			wait(&status);
			break;
		}
		if (WIFEXITED(status))	/* normal exit */
			ret |= WEXITSTATUS(status);
	}
	return ret;
}


int
load_from_file(struct node *np,char *fname,int action)
{
	char *name[USB_DEVICE_MAX+3];	/* 3:modprobe,option,NULL */
	int count = START_ARGV;	/* 0:loader 1:loader option */
	int len;
	struct stat st;
	char *mod_string,*sep,*prev;
	int fd;
	int ret = GOOD;
	int preload_count = 0;

	if (stat(fname,&st) != 0) {
		syslog(LOG_ERR,mesg[MSG_OPEN_ERR],fname);
		return INVALID;
	}
	len = st.st_size;
	if (len == 0)	/* do nothing */
		return GOOD;
	if ((fd = open(fname,O_RDONLY)) == -1) {
		syslog(LOG_ERR,mesg[MSG_OPEN_ERR],fname);
		ret = INVALID;
		goto err_close;
	}
	if ((mod_string = malloc(len + 1)) == NULL)	{
		syslog(LOG_ERR,mesg[MSG_ALLOC_ERR]);
		ret = INVALID;
		goto err_free;
	}
	if (read(fd,mod_string,len) != len) {
		syslog(LOG_ERR,mesg[MSG_OPEN_ERR],fname);
		ret = INVALID;
		goto err_free;
	}
	for (prev = mod_string;sep = strchr(prev,'\n');prev = sep+1) {
		*sep = '\0';
		/* if MODULE_PRELOAD and MODULE_LOAD is ON, 
		 * 		register module name to list
		 * else module name is found in list,
		 *		it is removed from name array
		 */
		if (action == (MODULE_LOAD|MODULE_PRELOAD)) {
			DPRINTF(LOG_DEBUG,"load_from_file:append %s preload list",prev);
			create_module(prev);
			if (debug) {
				syslog(LOG_DEBUG,mesg[MSG_TRY_LD],
					(action & MODULE_LOAD) ? "load" : "unload",prev);
			}
		} else {
			if (action & (MODULE_LOAD|MODULE_UNLOAD)) {
				if (find_string_module(prev) != NULL) {
					preload_count++;
					DPRINTF(LOG_DEBUG,"load_from_file:%s is in preload list",prev);
					continue;
				}
				if (debug) {
					syslog(LOG_DEBUG,mesg[MSG_TRY_LD],
						(action & MODULE_LOAD) ? "load" : "unload",prev);
				}
			} else {	/* SCRIPT_START or SCRIPT_STOP */
				if (debug) {
					syslog(LOG_DEBUG,mesg[MSG_TRY_LD],prev,
						(action & SCRIPT_START) ? "start" : "stop");
				}
			}
		}
		name[count++] = prev;
	}
	name[count] = NULL;

	if (action & (MODULE_LOAD|MODULE_UNLOAD)) {
		if (count == START_ARGV) {
			if (preload_count) {	/* make a beep even if no module */
				DPRINTF(LOG_DEBUG,"load device(nothing) -> beep good");
				beep(GOOD);
			}
			DPRINTF(LOG_DEBUG,"There are nothing to load");
			goto err_free;
		}
		if (module_load(count,name,action) == GOOD) {
			beep(GOOD);
			DPRINTF(LOG_DEBUG,"load device -> beep good");
		} else {
			beep(INVALID);
			DPRINTF(LOG_DEBUG,"load device -> beep fail");
		}
	} else {	/* SCRIPT_START or SCRIPT_STOP */
		if (count == 0) {
			DPRINTF(LOG_DEBUG,"There are nothing to execute");
			goto err_free;
		}
		if (execute_script(np,name,action) == GOOD) {
			beep(GOOD);
			DPRINTF(LOG_DEBUG,"execute script -> beep good");
		} else {
			beep(INVALID);
			DPRINTF(LOG_DEBUG,"execute script -> beep fail");
		}
	}

err_free:
	free(mod_string);
err_close:
	close(fd);

	return ret;
}


