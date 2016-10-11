
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <asm/arch/armadillo3x0_gpio.h>

#include "gpioctrl.h"
#include "option.h"

#define gen_set_output_param(value)      value,0,0,0
#define gen_set_input_param()            0,0,0,0
#define gen_get_param()                  0,0,0,0

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ usage
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void usage(int status){
  int i;
  printf("\n");
  printf("usage gpioctrl [option]\n\n");
  printf("  %s (Copyright (C) %s)\n\n",VERSION,COPYRIGHT);

  printf("option:\n");
  for(i=0; opt_param[i].id != OPT_END; i++){
    printf("  %s : %s\n",opt_param[i].name,opt_param[i].sub);
  }
  printf("\n");
  
  exit(status);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ display_param_list
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
void display_param_list(struct gpio_param *head){
  struct gpio_param *current;
  int i = 0;

  if(!head) return;

  current = head;
  do{
    i++;
    LOG_DEBUG("list[%2d]   : 0x%08lx\n",i,(unsigned long)current);

    LOG_MSG("GPIO No.   : %ld ",current->no);
    switch(current->no){
    case GPIO0: LOG_MSG("(GPIO0)\n");break;
    case GPIO1: LOG_MSG("(GPIO1)\n");break;
    case GPIO2: LOG_MSG("(GPIO2)\n");break;
    case GPIO3: LOG_MSG("(GPIO3)\n");break;
    case GPIO4: LOG_MSG("(GPIO4)\n");break;
    case GPIO5: LOG_MSG("(GPIO5)\n");break;
    case GPIO6: LOG_MSG("(GPIO6)\n");break;
    case GPIO7: LOG_MSG("(GPIO7)\n");break;
    default:    LOG_MSG("(unknown)\n");return;
    }
    
    LOG_MSG("MODE       : %ld ",current->mode);
    switch(current->mode){
    case MODE_OUTPUT: LOG_MSG("(MODE_OUTPUT)\n");break;
    case MODE_INPUT:  LOG_MSG("(MODE_INPUT)\n");break;
    case MODE_GET:    LOG_MSG("(MODE_GET)\n");break;
    default:          LOG_MSG("(unknown)\n");return;
    }

    switch(current->mode){
    case MODE_OUTPUT:
      LOG_MSG("VALUE      : %ld ",current->data.o.value);
      LOG_MSG("(%s)\n",(current->data.o.value) ? "HIGH":"LOW");
      break;
    case MODE_INPUT:
      LOG_MSG("VALUE      : %ld ",current->data.i.value);
      LOG_MSG("(%s)\n",(current->data.i.value) ? "HIGH":"LOW");
      break;
    }

    LOG_MSG("\n");
    current = current->next;
  }while(current);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ add_param_list
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int add_param_list(struct gpio_param **head,
		   unsigned long no,unsigned long mode,
		   unsigned long arg1,unsigned long arg2,
		   unsigned long arg3,unsigned long arg4){
  struct gpio_param *add;
  struct gpio_param *tmp;

  add = (struct gpio_param *)malloc(sizeof(struct gpio_param));
  if(!add) return -1;

  memset(add,0,sizeof(struct gpio_param));

  add->no   = no;
  add->mode = mode;    

  switch(mode){
  case MODE_OUTPUT:
    add->data.o.value = arg1;
    break;
  case MODE_INPUT:
    break;
  case MODE_GET:
    break;
  default:
    return -1;
  }

  if(*head){

    tmp = *head;
    while(tmp->next){
      tmp = tmp->next;
    }
    tmp->next = add;
  }else{
    *head = add;
  }

  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ free_param_list
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int free_param_list(struct gpio_param **head){
  struct gpio_param *tmp;
  struct gpio_param *current;
  int i = 0;

  if(!*head) return 0;

  current = *head;
  do{
    i++;
    tmp = current;
    current = current->next;
    LOG_DEBUG("free[%2d] : 0x%08lx\n",i,(unsigned long)tmp);
    free(tmp);
    
  }while(current);
  *head=0;

  return 0;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ main
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int main(int argc, char **argv){
  struct gpio_param *param_list = NULL;
  struct option_info opt_info;

  unsigned long gpio = 0;
  unsigned long mode = 0;
  unsigned long value = 0;

  int fd;
  int ret;
  
  init_option_info(&opt_info);
  ret = option_detect(argc, argv ,&opt_info);
  if(ret) usage(ret);

  switch(opt_info.gpio_no){
  case COM_SUB_GPIO0: gpio = GPIO0;break;
  case COM_SUB_GPIO1: gpio = GPIO1;break;
  case COM_SUB_GPIO2: gpio = GPIO2;break;
  case COM_SUB_GPIO3: gpio = GPIO3;break;
  case COM_SUB_GPIO4: gpio = GPIO4;break;
  case COM_SUB_GPIO5: gpio = GPIO5;break;
  case COM_SUB_GPIO6: gpio = GPIO6;break;
  case COM_SUB_GPIO7: gpio = GPIO7;break;
  case COM_SUB_ALL:   gpio = GPIO_ALL;break;
  }

  if(opt_info.command == OPT_SET){
    switch(opt_info.mode){
    case MODE_SUB_INPUT:  
      mode = MODE_INPUT;

      if(gpio == GPIO_ALL){
      add_param_list(&param_list,GPIO0,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO1,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO2,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO3,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO4,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO5,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO6,mode,gen_set_input_param());
      add_param_list(&param_list,GPIO7,mode,gen_set_input_param());
      }else{
      add_param_list(&param_list,gpio,mode,gen_set_input_param());
      }
      
      fd=open(DEVICE,O_RDWR);
      ioctl(fd,PARAM_SET,param_list);

      free_param_list(&param_list);

      close(fd);
      
      break;
    case MODE_SUB_OUTPUT:
      mode = MODE_OUTPUT;
      
      switch(opt_info.type){
      case TYPE_SUB_LOW: value = 0;break;
      case TYPE_SUB_HIGH: value = 1;break;
      }

      if(gpio == GPIO_ALL){
      add_param_list(&param_list,GPIO0,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO1,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO2,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO3,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO4,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO5,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO6,mode,gen_set_output_param(value));
      add_param_list(&param_list,GPIO7,mode,gen_set_output_param(value));
      }else{
      add_param_list(&param_list,gpio,mode,gen_set_output_param(value));
      }

      fd=open(DEVICE,O_RDWR);
      ioctl(fd,PARAM_SET,param_list);

      free_param_list(&param_list);

      close(fd);

      break;
    }

  }else if(opt_info.command == OPT_GET){
    if(gpio == GPIO_ALL){
      add_param_list(&param_list,GPIO0,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO1,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO2,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO3,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO4,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO5,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO6,MODE_GET,gen_get_param());
      add_param_list(&param_list,GPIO7,MODE_GET,gen_get_param());
    }else{
      add_param_list(&param_list,gpio,MODE_GET,gen_get_param());
    }
    
    fd=open(DEVICE,O_RDWR);
    ioctl(fd,PARAM_GET,param_list);
    
    display_param_list(param_list);
    
    close(fd);
    
    free_param_list(&param_list);
  }
  
  return 0;
}

