#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include "legacy.h"
#include "common.h"

#define DEVICE "/dev/led"

static int legacy_probe(void)
{
	int fd;
	fd = open(DEVICE, O_RDWR);
	if (fd == -1)
		return -ENXIO;
	close(fd);
	return 0;
}

static int led_action(int id)
{
	int fd;
	int ret;
	char buf[16];

	fd = open(DEVICE, O_RDWR);
	if (fd == -1) {
		perror("ledctrl: " DEVICE " open");
		return -1;
	}

	switch (id) {
	case LED_RED_ON:
	case LED_RED_OFF:
	case LED_RED_BLINKON:
	case LED_RED_BLINKOFF:
	case LED_GREEN_ON:
	case LED_GREEN_OFF:
	case LED_GREEN_BLINKON:
	case LED_GREEN_BLINKOFF:
		ret = ioctl(fd, id);
		if (ret == -1) {
			perror("ledctrl: ioctl");
			close(fd);
			return -1;
		}
		break;
	case (LED_RED_STATUS | LED_GREEN_STATUS):
		ret = ioctl(fd, id, buf);
		if (ret == -1) {
			perror("ledctrl: ioctl");
			close(fd);
			return -1;
		}
		info("RED(%s),GREEN(%s)\n",
		     buf[0] & LED_RED ? "on":"off",
		     buf[0] & LED_GREEN ? "on":"off");
		break;
	default:
		close(fd);
		return 0;
	}
	close(fd);
	return 0;
}

static int legacy_parse_option(int argc, char **argv,
			       struct ledctrl_info *info)
{
	int i, j;
	char *ptr;
	int id, index;

	for (i=1; i<argc; i++) {
		if (strncmp(argv[i], "--status", 8) == 0) {
			for (j=0; info[j].id; j++) {
				if (info[j].id == LED_GREEN)
					info[j].command = LED_LEGACY_STATUS;
				if (info[j].id == LED_RED)
					info[j].command = LED_LEGACY_STATUS;
			}
			return 0;
		}
		if (strncmp(argv[i], "--green", 7) == 0)
			id = LED_GREEN;
		else if (strncmp(argv[i], "--red", 5) == 0)
			id = LED_RED;
		else
			return -EINVAL;

		for (j=0, index=-1; info[j].id; j++) {
			if (info[j].id == id) {
				index = j;
				break;
			}
		}
		if (index == -1)
			return -EINVAL;

		ptr = strstr(argv[i], "blinkoff");
		if (ptr) {
			info[index].command = LED_BLINK_OFF;
			continue;
		}
		ptr = strstr(argv[i], "blinkon");
		if (ptr) {
			info[index].command = LED_BLINK_ON;
			info[index].delay_on = 200;
			info[index].delay_off = 200;
			continue;
		}
		ptr = strstr(argv[i], "off");
		if (ptr) {
			info[index].command = LED_OFF;
			continue;
		}
		ptr = strstr(argv[i], "on");
		if (ptr) {
			info[index].command = LED_ON;
			continue;
		}
		return -EINVAL;
	}

	return 0;
}

static int legacy_operation(struct ledctrl_info *info)
{
	int i;

	for (i=0; info[i].id; i++) {
		if (info[i].command == LED_LEGACY_STATUS ||
		    info[i].command == LED_STATUS) {
			led_action(LED_RED_STATUS | LED_GREEN_STATUS);
			return 0;
		}
	}

	for (i=0; info[i].id; i++) {
		switch (info[i].command) {
		case LED_ON:
			if (info[i].id == LED_RED)
				led_action(LED_RED_ON);
			if (info[i].id == LED_GREEN)
				led_action(LED_GREEN_ON);
			break;
		case LED_OFF:
			if (info[i].id == LED_RED)
				led_action(LED_RED_OFF);
			if (info[i].id == LED_GREEN)
				led_action(LED_GREEN_OFF);
			break;
		case LED_BLINK_ON:
			if (info[i].id == LED_RED)
				led_action(LED_RED_BLINKON);
			if (info[i].id == LED_GREEN)
				led_action(LED_GREEN_BLINKON);
			break;
		case LED_BLINK_OFF:
			if (info[i].id == LED_RED)
				led_action(LED_RED_BLINKOFF);
			if (info[i].id == LED_GREEN)
				led_action(LED_GREEN_BLINKOFF);
			break;
		default:
			continue;
		}
	}

	return 0;
}

struct ledctrl_ops legacy_ops = {
	.probe = legacy_probe,
	.parse_option = legacy_parse_option,
	.operation = legacy_operation,
};
