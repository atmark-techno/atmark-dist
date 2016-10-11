/************************************************************************
 file name :
 summary   :
 coded by  :
 copyright :
************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <signal.h>

#include "a210_common.h"

int system_check_executable(char *command){
  char *prefix[4] = {"/bin", "/sbin", "/usr/bin", "/usr/sbin"};
  struct stat info;
  char path[256];
  int ret;
  int i;

  for(i=0; i<4; i++){
    sprintf(path, "%s/%s", prefix[i], command);
    ret = stat(path, &info);
    if(ret == -1){
      continue;
    }
    
    if(info.st_mode & S_IXUSR){
      return 0;
    }
  }
  return -1;
}

void system_reboot(int flag){
	pid_t pid;
	if((pid = fork()) == 0){
		sleep(1);
		reboot(flag);
		_exit(0);
	}
}

int system_save_config(void){
	pid_t pid;
	char *param[]={"/bin/flatfsd","-s",0};

	if((pid = fork()) == 0){
		execv(param[0], param);
		_exit(0);
	}

	waitpid(pid, NULL, 0);
	return 0;
}

int system_get_processid(char *pidfile){
	pid_t pid;
	FILE *fp;
	
	fp = fopen(pidfile, "r");
	if(fp == NULL) return -1;
	fscanf(fp, "%d", &pid);
	fclose(fp);
	
	return pid;
}


int system_reset_seri2eth(void){
	pid_t pid;

	pid = system_get_processid(SERI2ETH_PID_FILE);
	if(pid == -1) return -1;
	
	kill(pid, SIGUSR1);
	return 0;
}

