#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include "common.h"

#define LEDCLASS "/sys/class/leds"
#define LEDPRESTAT "/tmp/ledctrl_pre_stat_"

#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])

static int legacy_display = 0;

static int ledclass_probe(void)
{
	int fd;

	fd = open(LEDCLASS, O_RDONLY);
	if (fd == -1)
		return -ENXIO;
	close(fd);
	return 0;
}

static int ledclass_status(struct ledctrl_info *info)
{
	char path[PATH_MAX];
	char buf[256];
	int fd;
	int brightness;

	snprintf(path, PATH_MAX, LEDCLASS"/%s/brightness", info->name);
	fd = open(path, O_RDONLY);
	if (fd == -1)
		return -errno;
	read(fd, buf, 256);
	close(fd);
	brightness = strtol(buf, NULL, 0);

	snprintf(path, PATH_MAX, LEDCLASS"/%s/delay_on", info->name);
	fd = open(path, O_RDONLY);
	if ((fd == -1) && errno == ENOENT) {
		if (brightness)
			info->command = LED_ON;
		else
			info->command = LED_OFF;
	} else if (fd != -1) {
		read(fd, buf, 256);
		close(fd);
		info->delay_on = strtol(buf, NULL, 0);

		snprintf(path, PATH_MAX, LEDCLASS"/%s/delay_off", info->name);
		fd = open(path, O_RDONLY);
		if (fd == -1)
			return -errno;
		read(fd, buf, 256);
		close(fd);
		info->delay_off = strtol(buf, NULL, 0);

		if (info->delay_on && info->delay_off)
			info->command = LED_BLINK_ON;
		else if (brightness)
			info->command = LED_ON;
		else
			info->command = LED_OFF;
	}

	return 0;
}

static int ledclass_onoff(struct ledctrl_info *info, int onoff)
{
	char path[PATH_MAX];
	int fd;

	snprintf(path, PATH_MAX, LEDPRESTAT "%s", info->name);
	remove(path);

	snprintf(path, PATH_MAX, LEDCLASS "/%s/trigger", info->name);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		perror(path);
		return -errno;
	}
	write(fd, "none", 4);
	close(fd);

	snprintf(path, PATH_MAX, LEDCLASS "/%s/brightness", info->name);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		perror(path);
		return -errno;
	}
	if (onoff)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);
	close(fd);

	return 0;
}

static int ledclass_blink(struct ledctrl_info *info, int onoff)
{
	char path[PATH_MAX];
	char buf[256];
	int fd;

	if (onoff == 0) {
		snprintf(path, PATH_MAX, LEDPRESTAT "%s", info->name);
		fd = open(path, O_RDONLY);
		if ((fd == -1) && (errno == ENOENT)) {
			return 0;
		} else if (fd == -1) {
			perror(path);
			return -errno;
		}
		read(fd, buf, 256);
		close(fd);
		remove(path);

		sscanf(buf, "%d, %d, %d", &info->command,
		       &info->delay_on, &info->delay_off);
		if (info->command) {
			info->command = LED_ON;
			return ledclass_onoff(info, 1);
		} else if (info->delay_on && info->delay_off) {
			info->command = LED_BLINK_ON;
		} else {
			info->command = LED_OFF;
			return ledclass_onoff(info, 0);
		}
	} else {
		snprintf(path, PATH_MAX, LEDPRESTAT "%s", info->name);
		fd = open(path, O_RDONLY);
		if (fd == 0) {
			close(fd);
		} else if ((fd == -1) && (errno == ENOENT)) {
			struct ledctrl_info cur_status;
			memcpy(&cur_status, info, sizeof(struct ledctrl_info));
			cur_status.delay_on = cur_status.delay_off = 0;
			ledclass_status(&cur_status);
			fd = open(path, O_WRONLY | O_CREAT);
			if (fd != -1) {
				snprintf(buf, 256, "%d, %d, %d\n",
					 cur_status.command == LED_ON ? 1 : 0,
					 cur_status.delay_on,
					 cur_status.delay_off);
				write(fd, buf, strlen(buf));
				close(fd);
			}
		}
	}

	snprintf(path, PATH_MAX, LEDCLASS "/%s/trigger", info->name);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		perror(path);
		return -errno;
	}
	write(fd, "timer", 5);
	close(fd);

	snprintf(path, PATH_MAX, LEDCLASS "/%s/delay_on", info->name);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		perror(path);
		return -errno;
	}
	snprintf(buf, 256, "%d", info->delay_on);
	write(fd, buf, strlen(buf));
	close(fd);

	snprintf(path, PATH_MAX, LEDCLASS "/%s/delay_off", info->name);
	fd = open(path, O_WRONLY);
	if (fd == -1) {
		perror(path);
		return -errno;
	}
	snprintf(buf, 256, "%d", info->delay_off);
	write(fd, buf, strlen(buf));
	close(fd);

	return 0;
}

static int ledclass_display(struct ledctrl_info *info, int count)
{
	if (count == 0)
		info("        brightness  delay_on  delay_off\n");

	info("%-7s %10d  %8d  %9d\n", info->name,
	     info->command == LED_ON ? 1 : 0,
	     info->delay_on,
	     info->delay_off);
	return 0;
}

static int ledclass_parse_option(int argc, char *argv[],
				 struct ledctrl_info *info)
{
	char buf[256], *ptr, *list;
	int i;
	if (argc < 2)
		return -EINVAL;
	if (strcmp(argv[1], "all") == 0) {
		ptr = buf;
		for (i=0; info[i].id; i++)
			ptr += sprintf(ptr, "%s%s", i ? ",":"", info[i].name);
		list = buf;
	} else
		list = argv[1];

	ptr = list;
	while (1) {
		char led[256], *sep;
		int len;
		int index;
		sep = strchr(ptr, ',');
		if (sep == NULL)
			len = strlen(ptr);
		else
			len = (int)(sep - ptr);
		strncpy(led, ptr, len);
		led[len] = '\0';

		for (i=0, index=-1; info[i].id; i++) {
			if (strcmp(info[i].name, led) == 0) {
				index = i;
				break;
			}
		}
		if (index == -1)
			return -EINVAL;

		if (strcmp(argv[2], "on") == 0) {
			info[index].command = LED_ON;
		} else if (strcmp(argv[2], "off") == 0) {
			info[index].command = LED_OFF;
		} else if (strcmp(argv[2], "blink_on") == 0) {
			char *eptr;
			info[index].command = LED_BLINK_ON;
			if (argc > 3) {
				info[index].delay_on = (int)strtol(argv[3],
								&eptr, 0);
				if (eptr[0] != '\0')
					return -EINVAL;
				info[index].delay_off = info[index].delay_on;
			} else {
				info[index].delay_on = 200;
				info[index].delay_off = 200;
			}
		} else if (strcmp(argv[2], "blink_off") == 0) {
			info[index].command = LED_BLINK_OFF;
		} else if (strcmp(argv[2], "status") == 0) {
			info[index].command = LED_STATUS;
		} else
			return -EINVAL;

		if (sep == NULL)
			break;
		ptr = sep + 1;
	}
	return 0;
}

static int ledclass_operation(struct ledctrl_info *info)
{
	int status = 0;
	int ret;
	int i;
	for (i=0; info[i].id; i++) {
		switch (info[i].command) {
		case LED_ON:
			ret = ledclass_onoff(&info[i], 1);
			if (ret)
				return ret;
			break;
		case LED_OFF:
			ret = ledclass_onoff(&info[i], 0);
			if (ret)
				return ret;
			break;
		case LED_BLINK_ON:
			ret = ledclass_blink(&info[i], 1);
			if (ret)
				return ret;
			break;
		case LED_BLINK_OFF:
			ret = ledclass_blink(&info[i], 0);
			if (ret)
				return ret;
			break;
		case LED_LEGACY_STATUS:
			ledclass_status(&info[i]);
			legacy_display = 1;
			break;
		case LED_STATUS:
			ledclass_status(&info[i]);
			ret = ledclass_display(&info[i], status++);
			if (ret)
				return ret;
			break;
		default:
			break;
		}
	}

	if (legacy_display) {
		for (i=0; info[i].id; i++)
			if (info[i].id == LED_RED) {
				info("RED(%s),",
				     info[i].command == LED_ON ? "ON" :
				     info[i].command == LED_OFF ? "OFF" :
				     "BLINK");
			}
		for (i=0; info[i].id; i++)
			if (info[i].id == LED_GREEN) {
				info("GREEN(%s)\n",
				     info[i].command == LED_ON ? "ON" :
				     info[i].command == LED_OFF ? "OFF" :
				     "BLINK");
			}
	}
	return 0;
}

struct ledctrl_ops ledclass_ops = {
	.probe = ledclass_probe,
	.parse_option = ledclass_parse_option,
	.operation = ledclass_operation,
};
