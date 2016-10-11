/*
 * dmeo-ad - demo application of the A/D board
 *
 *  Copyright (C) 2006-2008 Atmark Techno, Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

#include <asm/suzaku_sid.h>

#define VERSION					("1.1.0")
#define SID_DEVICE				("/dev/sid")

#define CHANNEL_BASE				(1)
#define CHANNEL_NUM				(16)
#define RESOLUTION_MIN				(9)
#define RESOLUTION_MAX				(16)
#define SAMPLE_RATE_MAX				(150000) /* 150 ksps */

static char *bin_name = NULL;

static unsigned int sid_channels;
static unsigned int sid_resolution;
static unsigned int sid_sample_size;
static int sid_value_digits;

static unsigned int sample_rate;
static unsigned int nr_sample;
static int nr_sample_digits;
static int print_ch[CHANNEL_NUM];
static int enable_print_index;

static void usage(FILE *fp)
{
	fprintf(fp,
		"Usage: %s [OPTION]... SAMPLE_RATE [NR_SAMPLE]\n", bin_name);
	fprintf(fp,
		"\n");
	fprintf(fp,
		"  SAMPLE_RATE       sample rate [Hz]\n");
	fprintf(fp,
		"  NR_SAMPLE         number of sample\n"
		"                    "
		"(default: the number for one second of samples)\n");
	fprintf(fp,
		"\n");
	fprintf(fp,
		"Options:\n");
	fprintf(fp,
		"  -c CHANNEL[,...]  only display the specified channel\n");
	fprintf(fp,
		"  -i                enable index display\n");
	fprintf(fp,
		"  -u, -?            display this usage\n");
	fprintf(fp,
		"  -v                display version\n");
}

#define DELIMITER " "
static void output_samples(unsigned short *value, int count)
{
	static unsigned int index = 1;
	int i, j;

	for (i = 0; i < count ; i++) {
		if (enable_print_index)
			printf("%*u:", nr_sample_digits, index++);

		for (j = 0; j < sid_channels; j++, value++)
			if (print_ch[j])
				printf(DELIMITER "%*hu",
				       sid_value_digits, *value);
		printf("\n");
	}
}

static int print_ad(int fd)
{
	size_t size = sid_sample_size * ((sample_rate + 9) / 10); /* 100msec */
	unsigned char *sample_buf;
	size_t remain, len;
	unsigned int overrun;

	sample_buf = malloc(size);
	if (!sample_buf) {
		fprintf(stderr, "Out of memory.\n");
		return -1;
	}
	if (ioctl(fd, SID_IOC_SET_FREQ, sample_rate) < 0) {
		fprintf(stderr, "Failed to set sample rate.\n");
		return -1;
	}
	for (remain = sid_sample_size * nr_sample; remain > 0; remain -= len) {
		if (size > remain)
			size = remain;
		len = read(fd, sample_buf, size);
		if (len < 0) {
			fprintf(stderr, "Read error.\n");
			free(sample_buf);
			return -1;
		}
		if (ioctl(fd, SID_IOC_GET_OVERRUN, &overrun) < 0) {
			fprintf(stderr, "Failed to get overrun state.\n");
			free(sample_buf);
			return -1;
		}
		if (overrun > 0) {
			fprintf(stderr, "Overrun occurred.\n");
			if (overrun < 0xffffffffUL)
				printf("\n(Overrun: "
				       "approx. %u samples dropped)\n\n",
				       overrun);
			else
				printf("\n(Overrun: "
				       "many samples dropped)\n\n");
		}
		output_samples((unsigned short *)sample_buf,
			       len / sid_sample_size);
	}
	free(sample_buf);

	return 0;
}

static int get_sid_info(int fd)
{
	unsigned short value = 0xffff;
	char tmp_buf[16];

	if (ioctl(fd, SID_IOC_GET_CHANNELS, &sid_channels) < 0) {
		fprintf(stderr, "Failed to get number of channels.\n");
		return -1;
	}
	if (!sid_channels || CHANNEL_NUM < sid_channels) {
		fprintf(stderr, "Number of channels not supported.\n");
		return -1;
	}

	if (ioctl(fd, SID_IOC_GET_RESOLUTION, &sid_resolution) < 0) {
		fprintf(stderr, "Failed to get resolution.\n");
		return -1;
	}
	if (sid_resolution < RESOLUTION_MIN ||
	    RESOLUTION_MAX < sid_resolution) {
		fprintf(stderr, "Resolution not supported.\n");
		return -1;
	}
	sid_sample_size = ((sid_resolution + 7) / 8) * sid_channels;
	value >>= 16 - sid_resolution;
	sprintf(tmp_buf, "%hu", value);
	sid_value_digits = strlen(tmp_buf);

	return 0;
}

static int parse_channel_options(char *options)
{
	unsigned int ch;

	for (ch = 0; ch < sid_channels; ch++)
		print_ch[ch] = 0;

	while (options) {
		char *comma = strchr(options, ',');

		if (comma)
			*comma = '\0';

		ch = strtoul(options, NULL, 0);
		if (ch < CHANNEL_BASE || CHANNEL_BASE + sid_channels <= ch) {
			fprintf(stderr, "Channel %u not supported.\n", ch);
			return -1;
		}
		print_ch[ch - CHANNEL_BASE] = 1;

		if (comma) {
			*comma = ',';
			options = ++comma;
		}
		else
			break;
	}

	return 0;
}

static int check_arg(int fd)
{
	if ((sample_rate <= 0) || (SAMPLE_RATE_MAX < sample_rate)) {
		fprintf(stderr, "Illegal sample rate.\n");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int fd;
	int opt;
	int ret;
	int i;
	char tmp_buf[16];

	bin_name = basename(argv[0]);

	fd = open(SID_DEVICE, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s.\n", SID_DEVICE);
		exit(EXIT_FAILURE);
	}

	ret = get_sid_info(fd);
	if (ret < 0) {
		close(fd);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < sid_channels; i++)
		print_ch[i] = 1;
	enable_print_index = 0;
	sample_rate = 0;
	nr_sample = 0;

	while ((opt = getopt(argc, argv, "c:iuv")) != -1) {
		switch (opt) {
		case 'c':
			if (parse_channel_options(optarg) < 0) {
				close(fd);
				exit(EXIT_FAILURE);
			}
			break;
		case 'i':
			enable_print_index = 1;
			break;
		case 'v':
			printf("%s %s\n", bin_name, VERSION);
			close(fd);
			exit(EXIT_SUCCESS);
		case 'u':
		case '?':
			usage(stdout);
			close(fd);
			exit(EXIT_SUCCESS);
		default:
			usage(stderr);
			close(fd);
			exit(EXIT_FAILURE);
		}
	}

	for (; optind < argc; optind++) {
		if (!sample_rate)
			sample_rate = strtoul(argv[optind], NULL, 0);
		else if (!nr_sample)
			nr_sample = strtoul(argv[optind], NULL, 0);
	}
	if (!sample_rate) {
		usage(stdout);
		close(fd);
		exit(EXIT_SUCCESS);
	}
	if (!nr_sample)
		nr_sample = sample_rate;
	sprintf(tmp_buf, "%u", nr_sample);
	nr_sample_digits = strlen(tmp_buf);

	ret = check_arg(fd);
	if (ret < 0) {
		close(fd);
		exit(EXIT_FAILURE);
	}

	ret = print_ad(fd);
	if (ret < 0) {
		close(fd);
		exit(EXIT_FAILURE);
	}

	close(fd);

	exit(EXIT_SUCCESS);
}
