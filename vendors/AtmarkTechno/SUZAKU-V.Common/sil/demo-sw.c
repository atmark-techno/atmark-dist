/*
 * dmeo_sw - demo application of switch
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
#define DEV_NAME    "/dev/silsw"

#define SW_NMAX     (3)

static char *prompt;
static char *sw_nr = NULL;
static int  _sw_nr = 0;

static void usage(void)
{
	FILE *fp = stderr;

	fprintf(fp, "Usage: %s [options]\n", prompt);
	fprintf(fp, "Options:\n");
	fprintf(fp, "    -l {N}         choose switch number [1-%d]\n", SW_NMAX);
	fprintf(fp, "    -u             usage\n");
	fprintf(fp, "    -v             version\n");
}

static void version(void)
{
	printf("%s %s\n", prompt, VERSION);
}

static int check_arg(void)
{
	if (sw_nr != NULL) {
		_sw_nr = atoi(sw_nr);
		if (_sw_nr <= 0 || _sw_nr > SW_NMAX) {
			fprintf(stderr, "illegal switch number.\n");
			return 0;
		}
	}

	return 1;
}

static int read_sw(int fd)
{
	char buf[2];
	int i;

	while (read(fd, buf, 1) == 1) {
		buf[1] = '\0';
		if (_sw_nr > 0) {
			printf("SW%d:[%s]\n", _sw_nr, (buf[0] & 0x1) ? "*" : " ");
		} else {
			for (i = SW_NMAX; i > 0; i--)
				printf("SW%d:[%s] ", i,
				       ((buf[0] >> (i-1)) & 0x1) ? "*" : " ");
			printf("\n");
		}
	}
	return 1;
}

static int demo(void)
{
	int fd;
	char devname[32];

	sprintf(devname, "%s%s", DEV_NAME, sw_nr == NULL ? "" : sw_nr);

	if ((fd = open(devname, O_RDONLY)) < 0) {
		fprintf(stderr, "failed to open %s.\n", devname);
		return 0;
	}

	read_sw(fd);

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
			sw_nr = optarg;
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

	if (!check_arg())
		return 1;

	if (!demo())
		return 1;

	return 0;
}
