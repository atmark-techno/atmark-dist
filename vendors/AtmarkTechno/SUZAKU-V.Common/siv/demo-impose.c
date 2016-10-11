#include <fcntl.h>
#include <linux/fb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <asm/suzaku_siv.h>

#include "logo.h"

#define FB_NUM 3

#define DEV_FB0        "/dev/fb0"
#define DEV_FB1        "/dev/fb1"
#define DEV_FB2        "/dev/fb2"

#define X_ACTIVE 640
#define Y_ACTIVE 432

#define OFFSET_ACTIVE   (16)

int main(void)
{
	const char dev[FB_NUM][16] = {DEV_FB0, DEV_FB1, DEV_FB2};
	int fd[FB_NUM];
	unsigned char *fb[FB_NUM];
	struct fb_var_screeninfo var;
	unsigned long fb_x, fb_y;
	unsigned long logo_x, logo_y;
	unsigned long offset_x, offset_y;
	unsigned long fb_byte_per_pixel;
	unsigned long fb_size;
	int ret;
	int next;
	int i, j;

	for (i = 0; i < FB_NUM; i++) {
		fd[i] = open(dev[i], O_RDWR);
		if (fd[i] < 0) {
			fprintf(stderr, "fb%d open error\n", i);
			return -1;
		}
	}

	ret =ioctl(fd[0], FBIOGET_VSCREENINFO, &var);
	if (ret < 0) {
		perror("ioctl err");
		return 1;
	}

	fb_x = var.xres_virtual;
	fb_y = var.yres_virtual;
	fb_byte_per_pixel = var.bits_per_pixel / 8;
	fb_size = fb_x * fb_y * fb_byte_per_pixel;

	for (i = 0; i < FB_NUM; i++) {
		fb[i] = mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED,
			     fd[i], 0);
		if (fb[i] == MAP_FAILED) {
			fprintf(stderr, "fb%d mmap error\n", i);
			return -1;
		}
	}

	logo_x = sizeof(logo[0]) / fb_byte_per_pixel;
	logo_y = sizeof(logo) / sizeof(logo[0]);

	offset_x = (fb_x - X_ACTIVE) / 2 + OFFSET_ACTIVE;
	offset_y = (fb_y - Y_ACTIVE) / 2 + OFFSET_ACTIVE;

	ioctl(fd[0], FBIO_CAPTURE_MODE_SELECT, CAPTURE_STOP_MODE);

	ioctl(fd[0], FBIO_DISPLAY_SELECT);
	ioctl(fd[1], FBIO_CAPTURE_SELECT);

	for (i = 1; ; i = next) {
		next = (i < FB_NUM - 1) ? (i + 1) : 0;
		ioctl(fd[i], FBIO_WAIT_CAPTURE);
		ioctl(fd[next], FBIO_CAPTURE_SELECT);

		for (j = 0; j < logo_y; j++)
			memcpy(fb[i] +  fb_byte_per_pixel * (fb_x * (offset_y + j) + offset_x),
			       logo[j], fb_byte_per_pixel * logo_x);
			
		ioctl(fd[i], FBIO_DISPLAY_SELECT);
	}
		
	for (i = 0; i < FB_NUM; i++) {
		munmap(fb[i], fb_size);
		close(fd[i]);
	}

	return 0;
}
