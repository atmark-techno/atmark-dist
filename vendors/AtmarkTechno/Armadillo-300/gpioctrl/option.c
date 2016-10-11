
#include <stdio.h>
#include <string.h>

#include "gpioctrl.h"
#include "option.h"

struct option_param opt_param[] = {
  {OPT_COM,      OPT_STR_command, 1, "gpio[0-7],all",""},
  {OPT_MODE,     OPT_STR_mode, 1, "input/output",""},
  {OPT_TYPE,     OPT_STR_type, 1, "output:<low>/high,input:(none)",""},
  {OPT_END,      NULL, 0, NULL, NULL},
};

struct sub_param com_sub_param[] = {
  {COM_SUB_GPIO0, "gpio0"},
  {COM_SUB_GPIO1, "gpio1"},
  {COM_SUB_GPIO2, "gpio2"},
  {COM_SUB_GPIO3, "gpio3"},
  {COM_SUB_GPIO4, "gpio4"},
  {COM_SUB_GPIO5, "gpio5"},
  {COM_SUB_GPIO6, "gpio6"},
  {COM_SUB_GPIO7, "gpio7"},
  {COM_SUB_ALL  , "all"},
  {COM_SUB_END, ""},
};
struct sub_param mode_sub_param[] = {
  {MODE_SUB_INPUT, "input"},
  {MODE_SUB_OUTPUT, "output"},
  {MODE_SUB_END, ""},
};

struct sub_param type_sub_param[] = {
  {TYPE_SUB_LOW, "low"},
  {TYPE_SUB_HIGH, "high"},
  {TYPE_SUB_END, ""},
};

#define OPT_CHECK(sub,arg,info)    \
({                                 \
  int ret;                         \
  ret = opt_check_##sub(arg,info); \
  switch(ret){                     \
  case  0: continue;               \
  case -2: LOG_ERR("%s: option multiple defained\n",OPT_STR_##sub); return -2; \
  default: break;                  \
  }                                \
})

#define IGNORE_OPT(opt,info)       \
({                                 \
  LOG_ERR("ignore option: %s\n",OPT_STR_##opt); \
  info->opt = 0;                   \
})

#define NOT_MATCH_OPT(opt)         \
({                                 \
  LOG_ERR("improper option: %s\n",OPT_STR_##opt); \
})

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int opt_check_command(char *arg, struct option_info *info){
  int i;
  int flag = 0;
  char *sub = NULL;

  if(strncasecmp(arg, "--set=", strlen("--set=")) == 0){
    if(info->command != 0) return -2;
    info->command = OPT_SET;
    flag++;
  }
  if(strncasecmp(arg, "--get=", strlen("--get=")) == 0){
    if(info->command != 0) return -2;
    info->command = OPT_GET;
    flag++;
  }
  if(flag != 1) return -1;

  sub = strstr(arg, "=");
  if(sub == NULL) return -1;
  sub++;

  for(i=0; com_sub_param[i].id != COM_SUB_END; i++){
    if(strcasecmp(sub, com_sub_param[i].name) == 0){
      info->gpio_no = com_sub_param[i].id;
      return 0;
    }
  }
  return -1;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int opt_check_mode(char *arg, struct option_info *info){
  int i;
  char *sub = NULL;

  if(strncasecmp(arg, "--mode=", strlen("--mode=")) == 0){
    if(info->mode != 0) return -2;
    
    sub = strstr(arg, "=");
    if(sub == NULL) return -1;
    sub++;

    for(i=0; mode_sub_param[i].id != MODE_SUB_END; i++){
      if(strcasecmp(sub, mode_sub_param[i].name) == 0){
	info->mode = mode_sub_param[i].id;
	return 0;
      }
    }
    return -1;
  }
  return -1;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int opt_check_type(char *arg, struct option_info *info){
  int i;
  char *sub = NULL;

  if(strncasecmp(arg, "--type=", strlen("--type=")) == 0){
    if(info->type != 0) return -2;

    sub = strstr(arg, "=");
    if(sub == NULL) return -1;
    sub++;

    for(i=0; type_sub_param[i].id != TYPE_SUB_END; i++){
      if(strcasecmp(sub, type_sub_param[i].name) == 0){
	info->type = type_sub_param[i].id;
	return 0;
      }
    }
    return -1;
  }
  return -1;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
//+ 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
int option_detect(int argc, char **argv, struct option_info *info){
  int i;

  for(i=1;i<argc;i++){
    OPT_CHECK(command, argv[i], info);
    OPT_CHECK(mode, argv[i], info);
    OPT_CHECK(type, argv[i], info);
    printf("unknown option: %s\n",argv[i]);
    return -1;
  }

  switch(info->command){
  case OPT_SET:
    switch(info->mode){
    case MODE_SUB_INPUT:
      switch(info->type){
      case TYPE_SUB_LOW:
      case TYPE_SUB_HIGH:
	NOT_MATCH_OPT(type);
	return -1;
      }
      break;
    case MODE_SUB_OUTPUT:
      break;
    }
    break;
  case OPT_GET:
    if(info->mode){ IGNORE_OPT(mode, info); }
    if(info->type){ IGNORE_OPT(type, info); }
    break;
  }
  return 0;
}

