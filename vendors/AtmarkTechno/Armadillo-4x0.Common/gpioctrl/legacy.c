#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "common.h"

#define DEVICE "/dev/gpio"

static int legacy_probe(void)
{
	int fd;
	fd = open(DEVICE, O_RDWR);
	if (fd == -1)
		return -ENXIO;

	close(fd);
	return 0;
}

static int legacy_set(struct gpio_param *paramlist)
{
	int fd;
	int ret;

	fd = open(DEVICE, O_RDWR);
	if (fd == -1) {
		perror(DEVICE);
		return -errno;
	}

	ret = ioctl(fd, PARAM_SET, paramlist);
	if (ret != 0)
		return ret;

	close(fd);

	return ret;
}

static int legacy_get(struct gpio_param *paramlist)
{
	int fd;
	int ret;

	fd = open(DEVICE, O_RDWR);
	if (fd == -1) {
		perror(DEVICE);
		return -errno;
	}

	ret = ioctl(fd, PARAM_GET, paramlist);
	if (ret != 0)
		return ret;

	close(fd);

	return ret;
}

static int legacy_interrupt_wait(struct gpio_param *paramlist,
				 struct wait_param *waitparam)
{
	int fd;
	int ret;

	fd = open(DEVICE, O_RDWR);
	if (fd == -1) {
		perror(DEVICE);
		return -errno;
	}

	ret = ioctl(fd, INTERRUPT_WAIT, waitparam);
	if (ret != 0)
		return ret;

	close(fd);

	return ret;
}

struct gpioctrl_ops legacy_ops = {
	.probe = legacy_probe,
	.set = legacy_set,
	.get = legacy_get,
	.interrupt_wait = legacy_interrupt_wait,
};
