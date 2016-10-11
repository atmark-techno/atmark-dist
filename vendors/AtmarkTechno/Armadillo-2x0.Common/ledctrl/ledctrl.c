
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <asm/arch/armadillo2x0_led.h>

#define VERSION "v1.01"
#define COPYRIGHT "2005-2006 Atmark Techno, Inc."
#define DEVICE "/dev/led"

struct option_info{
  unsigned long id;
  char *name;
  char *help;
};

struct option_info opt_list[]={
  {LED_RED_ON,         "--red=on",         ""},
  {LED_RED_OFF,        "--red=off",        ""},
  {LED_RED_BLINKON,    "--red=blinkon",    ""},
  {LED_RED_BLINKOFF,   "--red=blinkoff",   ""},
  {LED_GREEN_ON,       "--green=on",       ""},
  {LED_GREEN_OFF,      "--green=off",      ""},
  {LED_GREEN_BLINKON,  "--green=blinkon",  ""},
  {LED_GREEN_BLINKOFF, "--green=blinkoff", ""},
  {LED_RED_STATUS | LED_GREEN_STATUS,"--status","display led status"},
  {0,NULL},
};

int led_action(int id){
  int fd;
  int ret;
  char buf[16];

  fd = open(DEVICE,O_RDWR);
  if(fd == -1){
    perror("ledctrl: " DEVICE " open");
    return -1;
  }
  
  switch(id){
  case LED_RED_ON:
  case LED_RED_OFF:
  case LED_RED_BLINKON:
  case LED_RED_BLINKOFF:
  case LED_GREEN_ON:
  case LED_GREEN_OFF:
  case LED_GREEN_BLINKON:
  case LED_GREEN_BLINKOFF:
    ret = ioctl(fd,id);
    if(ret == -1){
      perror("ledctrl: ioctl");
      close(fd);
      return -1;
    }
    break;
  case (LED_RED_STATUS | LED_GREEN_STATUS):
    ret = ioctl(fd,id,buf);
    if(ret == -1){
      perror("ledctrl: ioctl");
      close(fd);
      return -1;
    }
    printf("RED(%s),GREEN(%s)\n",
	   buf[0] & LED_RED ? "on":"off",
	   buf[0] & LED_GREEN ? "on":"off"
	   );
    break;
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
