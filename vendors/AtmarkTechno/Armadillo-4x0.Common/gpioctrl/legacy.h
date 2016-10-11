/*
 * armadillo2x0_gpio.h - definitions for the Armadillo-2x0 compatible
 * GPIO driver
 *
 * Copyright (C) 2010 Atmark Techno, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _LINUX_ARMADILLO2X0_GPIO_H
#define _LINUX_ARMADILLO2X0_GPIO_H

struct wait_param {
	unsigned long list;    // wait GPIO list
	unsigned long timeout; // wait timeout (msec)
};

//ioctl command
#define ARMADILLO2X0_GPIO_IOCTL_BASE	'G'
#define PARAM_SET	_IOW (ARMADILLO2X0_GPIO_IOCTL_BASE, 0, struct gpio_param *)
#define PARAM_GET	_IOR (ARMADILLO2X0_GPIO_IOCTL_BASE, 1, struct gpio_param *)
#define INTERRUPT_WAIT	_IOW (ARMADILLO2X0_GPIO_IOCTL_BASE, 2, struct wait_param *)

#ifndef BIT
#define BIT(x)    (1<<(x))
#endif

#define GPIO_NUM      16
#define GPIO_NUM_A210 8

typedef enum __gpio_no_e {
	GPIO0    = BIT(0),
	GPIO1    = BIT(1),
	GPIO2    = BIT(2),
	GPIO3    = BIT(3),
	GPIO4    = BIT(4),
	GPIO5    = BIT(5),
	GPIO6    = BIT(6),
	GPIO7    = BIT(7),
	GPIO8    = BIT(8),
	GPIO9    = BIT(9),
	GPIO10   = BIT(10),
	GPIO11   = BIT(11),
	GPIO12   = BIT(12),
	GPIO13   = BIT(13),
	GPIO14   = BIT(14),
	GPIO15   = BIT(15),
} gpio_no_e;

typedef enum __int_type_e {
	//bit[0-1]:interrupt type
	TYPE_LOW_LEVEL    = 0,
	TYPE_HIGH_LEVEL   = 1,
	TYPE_FALLING_EDGE = 2,
	TYPE_RISING_EDGE  = 3,
	//bit[2]:debounce enable
	TYPE_DEBOUNCE     = 4,
} int_type_e;

#define MODE_OUTPUT 0x00
#define MODE_INPUT  0x01
#define MODE_GET    0x02

struct output_param {
	unsigned long value;       // read/write value
	unsigned long reserved[3];
};

struct input_param {
	unsigned long value;       // read value
	unsigned long int_enable;  // {0:disable/1:enable}
	unsigned long int_type;    // {int_type_e}
	unsigned long resetved[1];
};

struct gpio_param {
	struct gpio_param *next;  // {NULL:end of list}
	unsigned long no;         // {gpio_no_e}
	unsigned long mode;       // {MODE_OUTPUT/MODE_INPUT}
	union {
		struct output_param o;
		struct input_param i;
	} data;
};

struct armadillo2x0_gpio_info {
	unsigned no;
	unsigned gpio;

	unsigned direction : 1;
	unsigned can_interrupt : 1;
	unsigned interrupt_enabled : 1;
	unsigned irq_type : 2;

#if defined(CONFIG_ARCH_EP93XX)
	unsigned debounce_enabled : 1;
	unsigned int db_reg;
	unsigned int db_bit;
#endif
};

struct armadillo2x0_gpio_platform_data {
	int gpio_num;
	struct armadillo2x0_gpio_info *gpio_info;
};

#endif /* _LINUX_ARMADILLO2X0_GPIO_H */
