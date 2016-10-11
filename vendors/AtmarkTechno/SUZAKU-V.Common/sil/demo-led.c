/*
 * dmeo_led - demo application of led
 *
 *  Copyright (C) 2006 Atmark Techno, Inc.
 *
 * Tetsuya OHKAWA <tetsuya@atmark-techno.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VERSION     "1.0.0"
#define DEV_NAME    "/dev/silled"

#define LED_NMAX    (4)

static char *prompt;
static char *led_nr = NULL;
static char *ctrl = NULL;
static int  _led_nr = 0;
static int  _ctrl;

static void usage(void)
{
	FILE *fp = stderr;

	fprintf(fp, "Usage: %s [options] [ctrl]\n", prompt);
	fprintf(fp, "Options:\n");
	fprintf(fp, "    -l {N}        choose led number [1-%d]\n", LED_NMAX);
	fprintf(fp, "    -u            usage\n");
	fprintf(fp, "    -v            version\n");
}

static void version(void)
{
	printf("%s %s\n", prompt, VERSION);
}

static int check_arg(void)
{
	char *endp = NULL;

	if (led_nr != NULL) {
		_led_nr = atoi(led_nr);
		if (_led_nr <= 0 || _led_nr > LED_NMAX) {
			fprintf(stderr, "illegal led number.\n");
			return 0;
		}
	}

	if (ctrl != NULL) {
		_ctrl = strtoul(ctrl, &endp, 16);
		if (strlen(ctrl) != 1 || *endp != '\0') {
			fprintf(stderr, "bad ctrl number.\n");
			return 0;
		}
	}

	return 1;
}

static int read_leds(int fd)
{
	char buf[2];

	if (read(fd, buf, 1) < 0)
		return 0;
	buf[1] = '\0';
	printf("%c\n", buf[0]);

	return 1;
}

static int write_leds(int fd)
{
	if (write(fd, ctrl, 1) < 0)
		return 0;
	return 1;
}

static int demo(void)
{
	int fd;
	char devname[32];

	sprintf(devname, "%s%s", DEV_NAME, led_nr == NULL ? "" : led_nr);

	if ((fd = open(devname, O_RDWR)) < 0) {
		fprintf(stderr, "failed to open %s.\n", devname);
		return 0;
	}

	if (ctrl == NULL)
		read_leds(fd);
	else
		write_leds(fd);

	close(fd);

	return 1;
}

int main(int argc, char *argv[])
{
	int opt;

	if ((prompt = strrchr(argv[0], '/')) != NULL)
		++prompt;
	else
		prompt = argv[0];

	while ((opt = getopt(argc, argv, "l:uv")) != -1) {
		switch (opt) {
		case 'l':
			led_nr = optarg;
			break;
		case 'v':
			version();
			exit(0);
		case 'u':
		case '?':
		default:
			usage();
			exit(0);
		}
	}

	for ( ; optind < argc; optind++) {
		if (ctrl == NULL)
			ctrl = argv[optind];
	}

	if (!check_arg())
		return 1;

	if (!demo())
		return 1;

	return 0;
}
