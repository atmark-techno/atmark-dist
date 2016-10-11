#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "common.h"

#define GPIOCLASS "/sys/class/gpio"

static char *gpio_num2id(unsigned long num)
{
	switch (num) {
	case GPIO0: return "CON9_21";
	case GPIO1: return "CON9_22";
	case GPIO2: return "CON9_23";
	case GPIO3: return "CON9_24";
	case GPIO4: return "CON9_25";
	case GPIO5: return "CON9_26";
	case GPIO6: return "CON9_27";
	case GPIO7: return "CON9_28";
	case GPIO8: return "CON9_11";
	case GPIO9: return "CON9_12";
	case GPIO10: return "CON9_13";
	case GPIO11: return "CON9_14";
	case GPIO12: return "CON9_15";
	case GPIO13: return "CON9_16";
	case GPIO14: return "CON9_17";
	case GPIO15: return "CON9_18";
	default:
		return NULL;
	}
}

static int gpiolib_probe(void)
{
	int fd;

	fd = open(GPIOCLASS "/export", O_WRONLY);
	if (fd == -1)
		return -ENXIO;

	close(fd);
	return 0;
}

static int gpiolib_set(struct gpio_param *paramlist)
{
	char path[PATH_MAX];
	char *gpio;
	int fd;

	gpio = gpio_num2id(paramlist->no);
	if (gpio == NULL)
		return -EINVAL;

	snprintf(path, PATH_MAX, "%s/%s/direction", GPIOCLASS, gpio);
	fd = open(path, O_WRONLY);
	if (paramlist->mode == MODE_OUTPUT)
		write(fd, "out", 3);
	else
		write(fd, "in", 2);
	close(fd);

	if (paramlist->mode == MODE_OUTPUT) {
		snprintf(path, PATH_MAX, "%s/%s/value", GPIOCLASS, gpio);
		fd = open(path, O_WRONLY);
		if (paramlist->data.o.value)
			write(fd, "1", 1);
		else
			write(fd, "0", 1);
		close(fd);
	}

	return 0;
}

static int gpiolib_get(struct gpio_param *paramlist)
{
	char path[PATH_MAX];
	char *gpio;
	char buf[256];
	int fd;

	while (paramlist) {
		gpio = gpio_num2id(paramlist->no);
		if (gpio == NULL) {
			paramlist = paramlist->next;
			continue;
		}

		snprintf(path, PATH_MAX, "%s/%s/direction", GPIOCLASS, gpio);
		fd = open(path, O_RDONLY);
		read(fd, buf, 256);
		close(fd);
		if (strncmp(buf, "in", 2) == 0)
		    paramlist->mode = MODE_INPUT;
		else
		    paramlist->mode = MODE_OUTPUT;

		snprintf(path, PATH_MAX, "%s/%s/value", GPIOCLASS, gpio);
		fd = open(path, O_RDONLY);
		read(fd, buf, 256);
		close(fd);
		if (strncmp(buf, "1", 1) == 0)
			paramlist->data.i.value = 1;
		else
			paramlist->data.i.value = 0;

		paramlist->data.i.int_type = 0;

		paramlist = paramlist->next;
	}
	return 0;
}

static int gpiolib_interrupt_wait(struct gpio_param *paramlist,
				  struct wait_param *waitparam)
{
	fd_set fds;
	char path[PATH_MAX];
	char *gpio;
	char buf[256];
	int fd;
	int ret;

	if (paramlist->data.i.int_enable == 0)
		return -EINVAL;

	gpio = gpio_num2id(paramlist->no);
	if (gpio == NULL)
		return -EINVAL;

	snprintf(path, PATH_MAX, "%s/%s/value", GPIOCLASS, gpio);
	fd = open(path, O_RDONLY);
	read(fd, buf, 256);
	if (paramlist->data.i.int_type == TYPE_LOW_LEVEL)
		if (strncmp(buf, "0", 1) == 0)
			return 0;
	if (paramlist->data.i.int_type == TYPE_HIGH_LEVEL)
		if (strncmp(buf, "1", 1) == 0)
			return 0;
	close(fd);

	snprintf(path, PATH_MAX, "%s/%s/edge", GPIOCLASS, gpio);
	fd = open(path, O_WRONLY);
	if (paramlist->data.i.int_type == TYPE_FALLING_EDGE)
		write(fd, "falling", 7);
	else
		write(fd, "rising", 6);
	close(fd);

	snprintf(path, PATH_MAX, "%s/%s/value", GPIOCLASS, gpio);
	fd = open(path, O_RDONLY);
	read(fd, buf, 1);
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	ret = select(fd + 1, &fds, NULL, NULL, NULL);
	if (ret == -1)
		return -errno;
	read(fd, buf, 256);
	close(fd);

	return 0;
}

struct gpioctrl_ops gpiolib_ops = {
	.probe = gpiolib_probe,
	.set = gpiolib_set,
	.get = gpiolib_get,
	.interrupt_wait = gpiolib_interrupt_wait,
};
