/*
 * armadillo2x0_led.h - definitions for the Armadillo-2x0 compatible
 * LED driver
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

#ifndef _LINUX_ARMADILLO2X0_LED_H
#define _LINUX_ARMADILLO2X0_LED_H

//read/write parameter
#define LED_RED               (1 << 0)
#define LED_GREEN             (1 << 1)
#define LED_RED_BLINK         (1 << 2)
#define LED_GREEN_BLINK       (1 << 3)

//ioctl command
#define LED_IOCTL_BASE        'l'
#define LED_RED_ON            _IO(LED_IOCTL_BASE, 0x01)
#define LED_RED_OFF           _IO(LED_IOCTL_BASE, 0x02)
#define LED_RED_STATUS        _IOR(LED_IOCTL_BASE, 0x03, char *)
#define LED_RED_BLINKON       _IO(LED_IOCTL_BASE, 0x04)
#define LED_RED_BLINKOFF      _IO(LED_IOCTL_BASE, 0x05)
#define LED_RED_BLINKSTATUS   _IOR(LED_IOCTL_BASE, 0x06, char *)
#define LED_GREEN_ON          _IO(LED_IOCTL_BASE, 0x07)
#define LED_GREEN_OFF         _IO(LED_IOCTL_BASE, 0x08)
#define LED_GREEN_STATUS      _IOR(LED_IOCTL_BASE, 0x09, char *)
#define LED_GREEN_BLINKON     _IO(LED_IOCTL_BASE, 0x10)
#define LED_GREEN_BLINKOFF    _IO(LED_IOCTL_BASE, 0x11)
#define LED_GREEN_BLINKSTATUS _IOR(LED_IOCTL_BASE, 0x12, char *)

struct armadillo2x0_led_data {
	unsigned gpio;
	unsigned active_low;
};

struct armadillo2x0_led_platform_data {
	struct armadillo2x0_led_data led_red_data;
	struct armadillo2x0_led_data led_green_data;
};

#endif /* _LINUX_ARMADILLO2X0_LED_H */
