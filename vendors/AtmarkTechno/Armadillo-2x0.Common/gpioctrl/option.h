#ifndef _OPTION_H_
#define _OPTION_H_

typedef enum _opt_id_e{
  OPT_END = 0,
  OPT_COM,
  OPT_SET,
  OPT_GET,
  OPT_MODE,
  OPT_TYPE,
  OPT_DEBOUNCE,
  OPT_HUNDLER,
}opt_id_e;

typedef enum _com_sub_id_e{
  COM_SUB_END = 0,
  COM_SUB_GPIO0,
  COM_SUB_GPIO1,
  COM_SUB_GPIO2,
  COM_SUB_GPIO3,
  COM_SUB_GPIO4,
  COM_SUB_GPIO5,
  COM_SUB_GPIO6,
  COM_SUB_GPIO7,
  COM_SUB_GPIO8,
  COM_SUB_GPIO9,
  COM_SUB_GPIO10,
  COM_SUB_GPIO11,
  COM_SUB_GPIO12,
  COM_SUB_GPIO13,
  COM_SUB_GPIO14,
  COM_SUB_GPIO15,
  COM_SUB_ALL,
}com_sub_id_e;

typedef enum _mode_sub_id_e{
  MODE_SUB_END = 0,
  MODE_SUB_INPUT,
  MODE_SUB_OUTPUT,
}mode_sub_id_e;

typedef enum _type_sub_id_e{
  TYPE_SUB_END = 0,
  TYPE_SUB_LOW,
  TYPE_SUB_HIGH,
  TYPE_SUB_LOW_LEVEL,
  TYPE_SUB_HIGH_LEVEL,
  TYPE_SUB_FALLING_EDGE,
  TYPE_SUB_RISING_EDGE,
}type_sub_id_e;

struct option_param{
  int id;
  char *name;
  int sub_flag;
  char *sub;
  char *help;
};

struct sub_param{
  int id;
  char *name;
};

struct option_info{
  int command;
  int gpio_no;
  int mode;
  int type;
  int debounce;
  char *handler;
};

#define OPT_STR_command  "--set,--get"
#define OPT_STR_mode     "--mode"
#define OPT_STR_type     "--type"
#define OPT_STR_debounce "--debounce"
#define OPT_STR_handler  "--handler"

#define init_option_info(info) memset(info, 0, sizeof(struct option_info))
extern struct option_param opt_param[];
int option_detect(int argc, char **argv, struct option_info *info);
#endif
