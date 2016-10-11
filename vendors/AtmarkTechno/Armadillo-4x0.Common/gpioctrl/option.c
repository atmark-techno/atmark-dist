#include <stdio.h>
#include <string.h>
#include "common.h"
#include "option.h"

struct option_param opt_param[] = {
	{ OPT_COM,	OPT_STR_command, 1, "gpio[0-15],all","" },
	{ OPT_MODE,	OPT_STR_mode, 1, "input/output","" },
	{ OPT_TYPE,	OPT_STR_type, 1,
	  "output:<low>/high,input:<low-level>"
	  "/high-level/falling-edge/rising-edge","" },
	{ OPT_DEBOUNCE,	OPT_STR_debounce, 0, "(none)","" },
	{ OPT_HUNDLER,	OPT_STR_handler, 1, "exec...","" },
	{ OPT_END,	NULL, 0, NULL, NULL },
};

struct sub_param com_sub_param[] = {
	{ COM_SUB_GPIO0, "gpio0" },
	{ COM_SUB_GPIO1, "gpio1" },
	{ COM_SUB_GPIO2, "gpio2" },
	{ COM_SUB_GPIO3, "gpio3" },
	{ COM_SUB_GPIO4, "gpio4" },
	{ COM_SUB_GPIO5, "gpio5" },
	{ COM_SUB_GPIO6, "gpio6" },
	{ COM_SUB_GPIO7, "gpio7" },
	{ COM_SUB_GPIO8, "gpio8" },
	{ COM_SUB_GPIO9, "gpio9" },
	{ COM_SUB_GPIO10, "gpio10" },
	{ COM_SUB_GPIO11, "gpio11" },
	{ COM_SUB_GPIO12, "gpio12" },
	{ COM_SUB_GPIO13, "gpio13" },
	{ COM_SUB_GPIO14, "gpio14" },
	{ COM_SUB_GPIO15, "gpio15" },
	{ COM_SUB_ALL  , "all" },
	{ COM_SUB_END, "" },
};
struct sub_param mode_sub_param[] = {
	{ MODE_SUB_INPUT, "input" },
	{ MODE_SUB_OUTPUT, "output" },
	{ MODE_SUB_END, "" },
};

struct sub_param type_sub_param[] = {
	{ TYPE_SUB_LOW, "low" },
	{ TYPE_SUB_HIGH, "high" },
	{ TYPE_SUB_LOW_LEVEL, "low-level" },
	{ TYPE_SUB_HIGH_LEVEL, "high-level" },
	{ TYPE_SUB_FALLING_EDGE, "falling-edge" },
	{ TYPE_SUB_RISING_EDGE, "rising-edge" },
	{ TYPE_SUB_END, "" },
};

#define OPT_CHECK(sub,arg,info)						\
({									\
	int ret;							\
	ret = opt_check_##sub(arg,info);				\
	switch (ret) {							\
	case 0:								\
		continue;						\
	case -2:							\
		err("%s: option multiple defained\n", OPT_STR_##sub);	\
		return -2;						\
	default:							\
		 break;							\
	}								\
})

#define IGNORE_OPT(opt,info)				\
({							\
	err("ignore option: %s\n", OPT_STR_##opt);	\
	info->opt = 0;					\
})

#define NOT_MATCH_OPT(opt)				\
({							\
	err("invalid option: %s\n", OPT_STR_##opt);	\
})

int opt_check_command(char *arg, struct option_info *info)
{
	int i;
	int flag = 0;
	char *sub = NULL;

	if (strncasecmp(arg, "--set=", strlen("--set=")) == 0) {
		if (info->command != 0)
			return -2;
		info->command = OPT_SET;
		flag++;
	}
	if (strncasecmp(arg, "--get=", strlen("--get=")) == 0) {
		if (info->command != 0)
			return -2;
		info->command = OPT_GET;
		flag++;
	}
	if (flag != 1)
		return -1;

	sub = strstr(arg, "=");
	if (sub == NULL)
		return -1;
	sub++;

	for (i=0; com_sub_param[i].id != COM_SUB_END; i++) {
		if (strcasecmp(sub, com_sub_param[i].name) == 0) {
			info->gpio_no = com_sub_param[i].id;
			return 0;
		}
	}
	return -1;
}

int opt_check_mode(char *arg, struct option_info *info)
{
	int i;
	char *sub = NULL;

	if (strncasecmp(arg, "--mode=", strlen("--mode=")) == 0) {
		if (info->mode != 0)
			return -2;

		sub = strstr(arg, "=");
		if (sub == NULL)
			return -1;
		sub++;

		for (i=0; mode_sub_param[i].id != MODE_SUB_END; i++) {
			if (strcasecmp(sub, mode_sub_param[i].name) == 0) {
				info->mode = mode_sub_param[i].id;
				return 0;
			}
		}
		return -1;
	}
	return -1;
}

int opt_check_type(char *arg, struct option_info *info)
{
	int i;
	char *sub = NULL;

	if (strncasecmp(arg, "--type=", strlen("--type=")) == 0) {
		if (info->type != 0)
			return -2;

		sub = strstr(arg, "=");
		if (sub == NULL)
			return -1;
		sub++;

		for (i=0; type_sub_param[i].id != TYPE_SUB_END; i++) {
			if (strcasecmp(sub, type_sub_param[i].name) == 0) {
				info->type = type_sub_param[i].id;
				return 0;
			}
		}
		return -1;
	}
	return -1;
}

int opt_check_debounce(char *arg, struct option_info *info)
{
	if (strcasecmp(arg, "--debounce") == 0) {
		if (info->debounce != 0)
			return -2;

		info->debounce = 1;
		return 0;
	}
	return -1;
}

int opt_check_handler(char *arg, struct option_info *info)
{
	char *sub = NULL;

	if (strncasecmp(arg, "--handler=", strlen("--handler")) == 0) {
		if (info->handler != NULL)
			return -2;

		sub = strstr(arg, "=");
		if (sub == NULL)
			return -1;
		sub++;

		info->handler = sub;
		return 0;
	}
	return -1;
}

int option_detect(int argc, char **argv, struct option_info *info)
{
	int i;

	for (i=1; i<argc; i++) {
		OPT_CHECK(command, argv[i], info);
		OPT_CHECK(mode, argv[i], info);
		OPT_CHECK(type, argv[i], info);
		OPT_CHECK(debounce, argv[i], info);
		OPT_CHECK(handler, argv[i], info);
		err("unknown option: %s\n", argv[i]);
		return -1;
	}

	switch (info->command) {
	case OPT_SET:
		if (info->gpio_no == COM_SUB_ALL)
			NOT_MATCH_OPT(command);
		switch (info->mode) {
		case MODE_SUB_INPUT:
			switch (info->type) {
			case TYPE_SUB_LOW:
			case TYPE_SUB_HIGH:
				NOT_MATCH_OPT(type);
				return -1;
			}
			if (info->handler == NULL) {
				if (info->type)
					IGNORE_OPT(type, info);
				if (info->debounce)
					IGNORE_OPT(debounce, info);
			}
			break;
		case MODE_SUB_OUTPUT:
			switch (info->type) {
			case TYPE_SUB_LOW_LEVEL:
			case TYPE_SUB_HIGH_LEVEL:
			case TYPE_SUB_FALLING_EDGE:
			case TYPE_SUB_RISING_EDGE:
				NOT_MATCH_OPT(type);
				return -1;
			}
			if (info->debounce)
				IGNORE_OPT(debounce, info);
			if (info->handler)
				IGNORE_OPT(handler, info);
			break;
		}
		break;
	case OPT_GET:
		if (info->mode)
			IGNORE_OPT(mode, info);
		if (info->type)
			IGNORE_OPT(type, info);
		if (info->debounce)
			IGNORE_OPT(debounce, info);
		if (info->handler)
			IGNORE_OPT(handler, info);
		break;
	}
	return 0;
}
