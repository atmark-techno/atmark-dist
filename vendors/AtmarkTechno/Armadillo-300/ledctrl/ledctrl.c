
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <asm/arch/armadillo3x0_led.h>

#define VERSION "v1.00"
#define COPYRIGHT "2006 Atmark Techno, Inc."
#define DEVICE "/dev/led"

#define OPT_LED_ON     1
#define OPT_LED_OFF    2
#define OPT_LED_STATUS 3

struct option_info{
  unsigned long id;
  char *name;
  char *help;
};

struct option_info opt_list[]={
  {OPT_LED_ON, "--on", ""},
  {OPT_LED_OFF, "--off", ""},
  {OPT_LED_STATUS, "--status", "display led status"},
  {0,NULL},
};

int led_action(int id){
  int fd;
  int ret;
  //char buf[16];
  struct a3x0_led_param param;

  memset(&param, 0, sizeof(param));

  fd = open(DEVICE,O_RDWR);
  if(fd == -1){
    perror("ledctrl: " DEVICE " open");
    return -1;
  }
  
  switch(id){
  case OPT_LED_ON:
  case OPT_LED_OFF:
    param.buf = (id == OPT_LED_ON) ? 1 : 0;
    ret = ioctl(fd, A3X0_LED_SET, &param);
    if(ret == -1){
      perror("ledctrl: ioctl");
      close(fd);
      return -1;
    }
    break;
  case OPT_LED_STATUS:
    ret = ioctl(fd, A3X0_LED_GET, &param);
    if(ret == -1){
      perror("ledctrl: ioctl");
      close(fd);
      return -1;
    }
    printf("%s\n", param.buf ? "on":"off");
  default:
    close(fd);
    return 0;
  }
  close(fd);
  return 0;
}

void usage(int status){
  int i;
  printf("\n");
  printf("usage ledctrl [option]\n\n");
  printf("  %s (Copyright (C) %s)\n\n",VERSION,COPYRIGHT);
  printf("option:\n");
  for(i=0;;i++){
    if(!opt_list[i].name) break;

    printf("  %-20s %s\n",opt_list[i].name,opt_list[i].help);
  }
  printf("\n");
  exit(status);
}

int main(int argc, char **argv){
  int flag=0;
  int ret;
  int i,j;

  if(argc < 2) usage(1);

  for(i=1;i<argc;i++){
    for(j=0,flag=0;;j++){
      if(!opt_list[j].name) break;

      if(!strcasecmp(argv[i],opt_list[j].name)){
	flag++;
	ret=led_action(opt_list[j].id);
	if(ret) return ret;
	break;
      }
    }

    if(!flag){
      printf("unknown option : %s\n",argv[i]);
      usage(1);
    }
  }

  return 0;
}
