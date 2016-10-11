/*
 *  usbmgr  --- USB Manager
 *  
 *  Copyright (c) by Shuu Yamaguchi <shuu@wondernetworkresources.com>
 *
 *  $Id: main.c,v 4.13 2001/01/04 14:57:05 shuu Exp $
 *
 *  Can be freely distributed and used under the terms of the GNU GPL.
 *  The polling code from usbd.c which is written by Thomas Sailer.
 *
 *  Added to -p option not to create pid file.
 *	Olaf Hering
 */
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<signal.h>
#include	<limits.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/wait.h>

#include	"usbmgr.h"
#include	"version.h"

#define	UPDATE_CMD	"update_usbdb"
#define	DOING_TIME	10

char *conf_dir = CONF_DIR;
char *vendor_dir = USBMGR_VENDOR_DIR;
char *class_dir = USBMGR_CLASS_DIR;
static char *conf_file = CONF_DIR "/" CONF_FILE ;
static char *preload_file = CONF_DIR "/" PRELOAD ;
static char *update_dbcmd = UPDATE_CMD;
static char *pid_file = USB_DIR_PID "/" USB_FILE_PID ;
static pid_t child_pid;
int poll_fd;

static void func_alarm(int);

/****** option flag ******/
int nodaemon = 0;
int debug = 0;
int sleep_time = -1;	/* default: wait a event */
int use_pid_file = 1;	/* default: make pid file */
int make_beep = 1;		/* default: make a beep */

static void
bye(void)
{
	close(poll_fd);
	if (use_pid_file)
		unlink(pid_file);
	do_mount(USB_UNMOUNT);
	closelog();
	syslog(LOG_INFO,mesg[MSG_BYE]);
}

static void
new_configuration(char *fname,int doing)
{
	char *cmd[3];
	int i = 0;

	switch(child_pid = fork()) {
	case 0:
		cmd[i++] = update_dbcmd;
		if (doing == USBMGR_DO_FORCE)
			cmd[i++] = "-f";	/* force option */
		cmd[i++] = fname;
		cmd[i] = NULL;
		execvp(update_dbcmd,cmd);
		syslog(LOG_ERR,mesg[MSG_EXEC_ERR],update_dbcmd);
		exit(1);
		break;
	case -1:	/* ignore error */
		break;
	default:	/* parent */
		signal(SIGALRM,func_alarm);
		alarm(DOING_TIME);
		wait(NULL);
		alarm(0);	/* clear SIGALRM */
		break;
	}
}

static void
daemonize(void)
{
	switch(fork()) {
	case 0:		/* child */
		break;	/* do nothing */
	case -1:	/* error */
		exit(1);
	default:	/* parent */
		exit(0);
	}
}

static int
create_pidfile()
{
	int fd,len;
	char pid_buf[10];	/* 8 + '\n' + '\0' 8:enough width ,
						 * because MAX_PID is 0x8000(32765).
						 */

	/* todo: if pid file exist, print warning */
	if((fd = creat(pid_file,0644)) == -1)
		return -1;
	sprintf(pid_buf,"%d\n",getpid());
	len = strlen(pid_buf);
	if(write(fd,pid_buf,len) != len)
		return -1;
	close(fd);

	return 0;
}

/*
 * When SIGHUP come, all memories which are used device, modlink,
 * config and module are freed.
 */
static void
func_hup(int dummy)
{
	delete_device(USBMGR_DO_FORCE);	/* force to delete all devices */
	new_configuration(conf_file,USBMGR_DO_FORCE);
}

static void
func_term(int dummy)
{
	exit(0);
}

static void
func_alarm(int dummy)
{
	kill((pid_t)child_pid,SIGTERM);
	kill((pid_t)child_pid,SIGKILL);
}

static void
init_signal(void)
{
	struct sigaction sa;

	sa.sa_handler = func_hup;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&(sa.sa_mask));
	sigaction(SIGHUP,&sa,NULL);

	sa.sa_handler = func_term;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&(sa.sa_mask));
	sigaction(SIGTERM,&sa,NULL);
}

static void
print_version(void)
{
	printf("usbmgr: version %s\n",usbmgr_version);
}

static void
print_usage(void)
{
	printf("usbmgr [-Vdnphb] [-D #n] [-c file] [-t time]\n");
	printf("\tV: print version\n");
	printf("\td: debug & no daemon mode\n");
	printf("\tn: no daemon mode\n");
	printf("\tp: no pid file\n");
	printf("\th: print this usage\n");
	printf("\tb: no beep\n");
	printf("\tD: debug mode\n\t\t#n: debug number\n");
	printf("\tc: specify configuration file\n\t\tfile: configuration file\n");
	printf("\tt: specify polling time\n\t\ttime: micro second\n");
}

int
main(int argc,char **argv)
{
	int opt;

	while((opt = getopt(argc,argv,"D:Vc:dnt:phb")) != EOF) {
		switch(opt) {
		case 'D':
			debug = (unsigned int)strtoul(optarg,NULL,0);
			break;
		case 'V':
			print_version();
			exit(0);
		case 'c':
			conf_file = optarg;
			break;
		case 'd':	/* -d: set debug & nodaemon */
			debug++;	/* through */
		case 'n':
			nodaemon = 1;
			break;
		case 't':
			sleep_time = (unsigned int)strtoul(optarg,NULL,0);
			break;
		case 'p':
			use_pid_file = 0;
			break;
		case 'h':
			print_version();
			print_usage();
			exit(0);
		case 'b':
			make_beep = 0;
			break;
		}
	}
#ifdef	DEBUG
	openlog("usbmgr",LOG_PID|LOG_PERROR,LOG_DAEMON);
#else
	openlog("usbmgr",LOG_PID|LOG_CONS,LOG_DAEMON);
	close(0);
	close(1);
	close(2);
#endif
	syslog(LOG_INFO,mesg[MSG_START],usbmgr_version);
	DPRINTF(LOG_INFO,"sleep time %d micro sec\n",sleep_time);
	if (chdir(conf_dir) == -1) {
		syslog(LOG_ERR,mesg[MSG_CANT_CD],conf_dir);
		exit(1);
	}
	new_configuration(conf_file,USBMGR_DO_AUTO);
	if (access(USBMGR_NOBEEP_FILE,R_OK) == 0)
		make_beep = 0;
	if (!nodaemon) {
		daemonize();
	}
	if (use_pid_file && create_pidfile() == -1) {
		syslog(LOG_ERR,mesg[MSG_CANT_PID]);
		exit(1);
	}
	init_signal();
	if (access(preload_file,R_OK) == 0)
		load_from_file(NULL,preload_file,MODULE_LOAD|MODULE_PRELOAD);
	atexit(&bye);
	while(1) {	/* prevent memomy from being large */
		observe();
		DPRINTF(LOG_DEBUG, "main: refresh or error");
	}

	return 0;
}
