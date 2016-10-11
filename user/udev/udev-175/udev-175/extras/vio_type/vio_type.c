/*
 * vio-type - identify type of a VIO device
 *
 * Copyright © 2006 Canonical Ltd.
 * Author: Scott James Remnant <scott@ubuntu.com>
 * Copyright © 2010 Marco d'Itri <md@linux.it>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>

#include "libudev.h"
#include "libudev-private.h"

static int debug;

static void log_fn(struct udev *udev, int priority,
		   const char *file, int line, const char *fn,
		   const char *format, va_list args)
{
	if (debug) {
		fprintf(stderr, "%s: ", fn != NULL ? fn : file);
		vfprintf(stderr, format, args);
	} else {
		vsyslog(priority, format, args);
	}
}

int main(int argc, char *argv[])
{
	static const struct option options[] = {
		{ "export", no_argument, NULL, 'e' },
		{ "debug", no_argument, NULL, 'd' },
		{ "help", no_argument, NULL, 'h' },
		{}
	};
	struct udev *udev;
	struct stat buf;
	const char *devpath = NULL;
	char filename[UTIL_PATH_SIZE], devspec[256], devtype[256];
	int export = 0;
	int rc = 1;
	int i, fd, len;

	udev = udev_new();
	if (udev == NULL)
	    goto exit;

	udev_log_init("vio_type");
	udev_set_log_fn(udev, log_fn);

	while (1) {
		int option;

		option = getopt_long(argc, argv, "edh", options, NULL);
		if (option == -1)
			break;

		switch (option) {
		case 'e':
			export = 1;
			break;
		case 'd':
			debug = 1;
			if (udev_get_log_priority(udev) < LOG_INFO)
				udev_set_log_priority(udev, LOG_INFO);
			break;
		case 'h':
			printf("Usage: vio_type [--debug] [--help] <devpath>\n"
			       "  --debug    print debug information\n"
			       "  --help     print this help text\n\n");
		default:
			goto exit;
		}
	}

	devpath = argv[optind];
	if (devpath == NULL) {
		err(udev, "No device specified");
		rc = 2;
		goto exit;
	}

	util_strscpyl(filename, sizeof(filename), udev_get_sys_path(udev),
	    devpath, "/devspec", NULL);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		err(udev, "unable to open '%s'", filename);
		goto exit;
	}

	len = read(fd, devspec, sizeof(devspec));
	if (len <= 0) {
#if !defined(__sparc__) && !defined(__sparc64__)
		err(udev, "unable to read from '%s'", filename);
#endif
		goto close;
	}

	devspec[len] = '\0';
	if (devspec[len-1] == '\n')
		devspec[len-1] = '\0';

	close(fd);

#if !defined(__sparc__) && !defined(__sparc64__)
	/* now we look in /proc */

	util_strscpyl(filename, sizeof(filename), "/proc/device-tree",
	    devspec, "/device_type", NULL);

	/* hang around for /proc to catch up */
	for (i = 100; i; i--) {
		if (stat(filename, &buf) == 0)
			break;

		usleep(30000);
	}

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		err(udev, "unable to open '%s'", filename);
		goto exit;
	}

	len = read(fd, devtype, sizeof(devtype));
	if (len <= 0) {
		err(udev, "unable to read from '%s'", filename);
		goto close;
	}

	devtype[len] = '\0';
	if (devtype[len-1] == '\n')
		devtype[len-1] = '\0';
#else
	strncpy(devtype,devspec,strlen(devspec)+1);
#endif

	if (export) {
		printf("VIO_TYPE=%s\n", devtype);
	} else {
		printf("%s\n", devtype);
	}

	rc = 0;

close:
	close(fd);
exit:
	udev_unref(udev);
	udev_log_close();
	return rc;
}
