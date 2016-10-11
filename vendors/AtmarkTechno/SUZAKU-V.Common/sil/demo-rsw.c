/*
 * dmeo_rsw - demo application of rotary code switch
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
#define DEV_NAME    "/dev/silrsw"
static char *prompt;

static void usage(void)
{
	FILE *fp = stderr;
	fprintf(fp, "Usage: %s [options]\n", prompt);
	fprintf(fp, "Options:\n");
	fprintf(fp, "    -u             usage\n");
	fprintf(fp, "    -v             version\n");
}

static void version(void)
{
	printf("%s %s\n", prompt, VERSION);
}

static int read_rsw(int fd)
{
	char buf;

	while (read(fd, &buf, 1) == 1) {
		printf("%c\n", buf);
	}
	return 1;
}

static int demo(void)
{
	int fd;

	if ((fd = open(DEV_NAME, O_RDWR)) < 0) {
		fprintf(stderr, "failed to open %s.\n", DEV_NAME);
		return 0;
	}

	read_rsw(fd);

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

	while ((opt = getopt(argc, argv, "uv")) != -1) {
		switch (opt) {
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
	}

	if (!demo())
		return 1;

	return 0;
}
