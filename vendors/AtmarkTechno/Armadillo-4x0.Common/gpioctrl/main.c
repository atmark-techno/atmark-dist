#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"
#include "option.h"

#ifndef GPIO_ALL
#define GPIO_ALL (0xffffffff)
#endif

#define gen_set_output_param(value)		value, 0, 0, 0
#define gen_set_input_param(enable, type)	0, enable, type, 0
#define gen_get_param()				0, 0, 0, 0

extern struct gpioctrl_ops gpiolib_ops;
extern struct gpioctrl_ops legacy_ops;

void usage(int status)
{
	int i;
	printf("\n");
	printf("usage gpioctrl [option]\n\n");
	printf("  %s (Copyright (C) %s)\n\n", VERSION, COPYRIGHT);

	printf("option:\n");
	for (i=0; opt_param[i].id != OPT_END; i++)
		printf("  %s : %s\n", opt_param[i].name, opt_param[i].sub);
	printf("\n");

	exit(status);
}

void display_param_list(struct gpio_param *head)
{
	struct gpio_param *current;
	int i = 0;

	if (!head)
		return;

	current = head;
	do {
		i++;
		dbg("list[%2d]   : 0x%08lx\n", i, (unsigned long)current);

		info("GPIO No.   : %ld ", current->no);
		switch (current->no) {
		case GPIO0: info("(GPIO0)\n"); break;
		case GPIO1: info("(GPIO1)\n"); break;
		case GPIO2: info("(GPIO2)\n"); break;
		case GPIO3: info("(GPIO3)\n"); break;
		case GPIO4: info("(GPIO4)\n"); break;
		case GPIO5: info("(GPIO5)\n"); break;
		case GPIO6: info("(GPIO6)\n"); break;
		case GPIO7: info("(GPIO7)\n"); break;
		case GPIO8: info("(GPIO8)\n"); break;
		case GPIO9: info("(GPIO9)\n"); break;
		case GPIO10: info("(GPIO10)\n"); break;
		case GPIO11: info("(GPIO11)\n"); break;
		case GPIO12: info("(GPIO12)\n"); break;
		case GPIO13: info("(GPIO13)\n"); break;
		case GPIO14: info("(GPIO14)\n"); break;
		case GPIO15: info("(GPIO15)\n"); break;
		default: info("(unknown)\n"); return;
		}

		info("MODE       : %ld ", current->mode);
		switch (current->mode) {
		case MODE_OUTPUT: info("(MODE_OUTPUT)\n"); break;
		case MODE_INPUT: info("(MODE_INPUT)\n"); break;
		case MODE_GET: info("(MODE_GET)\n"); break;
		default: info("(unknown)\n"); return;
		}

		switch (current->mode) {
		case MODE_OUTPUT:
			info("VALUE      : %ld ", current->data.o.value);
			info("(%s)\n", (current->data.o.value) ? "HIGH":"LOW");
			break;
		case MODE_INPUT:
			info("VALUE      : %ld ", current->data.i.value);
			info("(%s)\n", (current->data.i.value) ? "HIGH":"LOW");
			info("INTERRUPT  : %ld ", current->data.i.int_enable);
			info("(%s)\n", (current->data.i.int_enable == 1) ?
			     "ENABLE" : "DISABLE");

			if (current->data.i.int_enable == 1) {
				info("INT-TYPE   : %ld (",
				     current->data.i.int_type);
				switch (current->data.i.int_type & 0x0003) {
				case TYPE_LOW_LEVEL:
					info("TYPE_LOW_LEVEL"); break;
				case TYPE_HIGH_LEVEL:
					info("TYPE_HIGH_LEVEL"); break;
				case TYPE_FALLING_EDGE:
					info("TYPE_FALLING_EDGE"); break;
				case TYPE_RISING_EDGE:
					info("TYPE_RISING_EDGE"); break;
				}
				if (current->data.i.int_type & TYPE_DEBOUNCE)
					info(",TYPE_DEBOUNCE");
				info(")\n");
			}
			break;
		}

		info("\n");
		current = current->next;
	} while (current);
}

int add_param_list(struct gpio_param **head,
		   unsigned long no, unsigned long mode,
		   unsigned long arg1, unsigned long arg2,
		   unsigned long arg3, unsigned long arg4)
{
	struct gpio_param *add;
	struct gpio_param *tmp;

	add = (struct gpio_param *)malloc(sizeof(struct gpio_param));
	if (!add)
		return -1;

	memset(add, 0, sizeof(struct gpio_param));

	add->no = no;
	add->mode = mode;

	switch (mode) {
	case MODE_OUTPUT:
		add->data.o.value = arg1;
		break;
	case MODE_INPUT:
		add->data.i.int_enable = arg2;
		add->data.i.int_type = arg3;
		break;
	case MODE_GET:
		break;
	default:
		return -1;
	}

	if (*head) {

		tmp = *head;
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = add;
	} else {
		*head = add;
	}

	return 0;
}

int free_param_list(struct gpio_param **head)
{
	struct gpio_param *tmp;
	struct gpio_param *current;
	int i = 0;

	if (!*head)
		return 0;

	current = *head;
	do {
		i++;
		tmp = current;
		current = current->next;
		dbg("free[%2d] : 0x%08lx\n", i, (unsigned long)tmp);
		free(tmp);

	} while (current);
	*head = 0;

	return 0;
}

int main(int argc, char *argv[])
{
	struct gpioctrl_ops *ops;
	struct gpio_param *param_list = NULL;
	struct wait_param wait_param;
	struct option_info opt_info;
	unsigned long gpio = 0;
	unsigned long mode = 0;
	unsigned long value = 0;
	unsigned long int_enable = 0;
	unsigned long int_type = 0;
	int ret;

	init_option_info(&opt_info);
	ret = option_detect(argc, argv, &opt_info);
	if (ret)
		usage(ret);

	switch (opt_info.gpio_no) {
	case COM_SUB_GPIO0: gpio = GPIO0; break;
	case COM_SUB_GPIO1: gpio = GPIO1; break;
	case COM_SUB_GPIO2: gpio = GPIO2; break;
	case COM_SUB_GPIO3: gpio = GPIO3; break;
	case COM_SUB_GPIO4: gpio = GPIO4; break;
	case COM_SUB_GPIO5: gpio = GPIO5; break;
	case COM_SUB_GPIO6: gpio = GPIO6; break;
	case COM_SUB_GPIO7: gpio = GPIO7; break;
	case COM_SUB_GPIO8: gpio = GPIO8; break;
	case COM_SUB_GPIO9: gpio = GPIO9; break;
	case COM_SUB_GPIO10: gpio = GPIO10; break;
	case COM_SUB_GPIO11: gpio = GPIO11; break;
	case COM_SUB_GPIO12: gpio = GPIO12; break;
	case COM_SUB_GPIO13: gpio = GPIO13; break;
	case COM_SUB_GPIO14: gpio = GPIO14; break;
	case COM_SUB_GPIO15: gpio = GPIO15; break;
	case COM_SUB_ALL: gpio = GPIO_ALL; break;
	}

	ops = &legacy_ops;
	ret = ops->probe();
	if (ret) {
		ops = &gpiolib_ops;
		ret = ops->probe();
	}
	if (ret) {
		err("no found gpio-interface.\n");
		return -ENXIO;
	}

	if (opt_info.command == OPT_SET) {
		switch (opt_info.mode) {
		case MODE_SUB_INPUT:
			mode = MODE_INPUT;
			if (opt_info.handler) {
				int_enable = 1;
				int_type = 0;
				switch (opt_info.type) {
				case TYPE_SUB_LOW_LEVEL:
					int_type |= TYPE_LOW_LEVEL; break;
				case TYPE_SUB_HIGH_LEVEL:
					int_type |= TYPE_HIGH_LEVEL; break;
				case TYPE_SUB_FALLING_EDGE:
					int_type |= TYPE_FALLING_EDGE; break;
				case TYPE_SUB_RISING_EDGE:
					int_type |= TYPE_RISING_EDGE; break;
				default:
					int_enable = 0; break;
				}
				if (opt_info.debounce)
					int_type |= TYPE_DEBOUNCE;
			}

			add_param_list(&param_list, gpio, mode,
				       gen_set_input_param(int_enable,
							   int_type));

			ret = ops->set(param_list);
			if (ret < 0) {
				err("failed set parameters\n");
				free_param_list(&param_list);
				return ret;
			}

			if (opt_info.handler) {
				wait_param.list = gpio;
				wait_param.timeout = 0;

				ret = ops->interrupt_wait(param_list,
							  &wait_param);
				if (ret < 0) {
					err("failed wait interrupt\n");
					free_param_list(&param_list);
					return ret;
				}

				if (wait_param.list & gpio)
					system(opt_info.handler);
			}
			free_param_list(&param_list);
			break;
		case MODE_SUB_OUTPUT:
			mode = MODE_OUTPUT;

			switch (opt_info.type) {
			case TYPE_SUB_LOW:
				value = 0; break;
			case TYPE_SUB_HIGH:
				value = 1; break;
			}

			add_param_list(&param_list, gpio, mode,
				       gen_set_output_param(value));

			ret = ops->set(param_list);
			if (ret < 0) {
				err("failed set parameters\n");
				free_param_list(&param_list);
				return ret;
			}

			free_param_list(&param_list);

			break;
		}

	} else if (opt_info.command == OPT_GET) {
		if (gpio == GPIO_ALL) {
			add_param_list(&param_list, GPIO0, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO1, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO2, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO3, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO4, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO5, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO6, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO7, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO8, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO9, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO10, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO11, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO12, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO13, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO14, MODE_GET,
				       gen_get_param());
			add_param_list(&param_list, GPIO15, MODE_GET,
				       gen_get_param());
		} else {
			add_param_list(&param_list, gpio, MODE_GET,
				       gen_get_param());
		}

		ret = ops->get(param_list);
		if (ret < 0) {
			err("failed get parameters\n");
			free_param_list(&param_list);
			return ret;
		}

		display_param_list(param_list);

		free_param_list(&param_list);
	}

	return 0;
}
