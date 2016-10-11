#define _GNU_SOURCE /* for mempcpy() */
#include <asm/arch/armadillo2x0_led.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/input.h>
#include <linux/fb.h>
#include <magick/api.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wand/magick_wand.h>
#include <jpeglib.h>

#define EVENT_SW "/dev/input/event0"
#define DEVICE_FB "/dev/fb0"
#define DEVICE_LED "/dev/led"

#define MAX_JPEG_IMAGE_WIDTH 1280
#define MAX_JPEG_IMAGE_HEIGHT 1024

/* default resolution */
static int disp_image_width = 800;
static int disp_image_height = 600;
static int color_dept = 3; /* byte */

#define MAX_NR_CACHE 10

#define BGCOLOR_B 255 /* background color (blue) */
#define BGCOLOR_G 255 /* background color (green)*/
#define BGCOLOR_R 255 /* background color (red)*/

#define CONVERT_IMAGE_EXIT_SUCCESS 0
#define CONVERT_IMAGE_NORMAL_ERR -1
#define CONVERT_IMAGE_FATAL_ERR -2

/**
 * The previous image is displayed when the switch is held down for a
 * period of time greater than SW_ON_POLL_SEC.
 */
#define SW_ON_POLL_SEC 1

#define CACHE_FILE_EXT ".tmp"

#define  PRINT_ERR(...) ({\
	pthread_mutex_lock(&ss_print_message_lock);\
	fprintf(stderr, ss_program_name);\
	fprintf(stderr, ": ");\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\n");\
	pthread_mutex_unlock(&ss_print_message_lock);\
})

#define PRINT_ERR_AND_HALT(...) ({\
	pthread_mutex_lock(&ss_print_message_lock);\
	fprintf(stderr, ss_program_name);\
	fprintf(stderr, ": ");\
	fprintf(stderr, __VA_ARGS__);\
	fprintf(stderr, "\n");\
	pthread_mutex_unlock(&ss_print_message_lock);\
	halt();\
})

#define PERROR_AND_HALT(void) ({\
	pthread_mutex_lock(&ss_print_message_lock);\
	perror(ss_program_name);\
	pthread_mutex_unlock(&ss_print_message_lock);\
	halt();\
})

enum SS_DISP {
	SS_DISP_NEXT,
	SS_DISP_PREV
};

enum SS_CACHE_EXIST {
	SS_NOTEXIST = 0,
	SS_EXISTING
};

typedef struct ss_thread_arg_tag {
	int state;     /* not 0 : err  */
	enum SS_DISP direction;
	char *work_dir;
	char *image_dir;
}ss_thread_arg;

typedef struct ss_cache_tag {
	unsigned long width;
	unsigned long height;
	enum SS_CACHE_EXIST exist;
}ss_cache;

typedef struct ss_imagefile_tag {
	ss_cache cache;
	struct ss_imagefile_tag *next;
	struct ss_imagefile_tag *prev;
	char filename[0];
}ss_imagefile;

struct error_manager
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

typedef struct error_manager* error_ptr;

static ss_imagefile *ss_current;

static char *ss_program_name; /* argv[0] */
static char *ss_fbmem; /* pointer to the start of frame buffer */

static pthread_cond_t ss_switch_cond;
static pthread_mutex_t	ss_current_lock;
static pthread_mutex_t	ss_cache_manager_cancel_lock;
static pthread_mutex_t	ss_print_message_lock;


/****************************************************************************************
 *
 ***************************************************************************************/
METHODDEF(void) error_exit(j_common_ptr cinfo)
{
	error_ptr err = (error_ptr)cinfo->err;
	(*cinfo->err->output_message)(cinfo);

	longjmp(err->setjmp_buffer, 1);
}


/****************************************************************************************
 *
 ***************************************************************************************/
static void halt(void)
{
	int ret;
	int fd_led;

	fd_led = open(DEVICE_LED, O_RDWR);

	if (fd_led == -1) {
		perror(ss_program_name);
	} else {
		ret = ioctl(fd_led, LED_RED_ON);
		if (ret == -1)
			perror(ss_program_name);

		close(fd_led);
	}

	pause();
}


/****************************************************************************************
 *
 ***************************************************************************************/
static void *xmalloc(size_t size)
{
	void *ret;

	ret = malloc(size);

	if (!ret)
		PERROR_AND_HALT();

	return ret;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static int disp_image(ss_cache *cache, const char *cachefile)
{
	FILE *fp;
	unsigned long i;
	unsigned long bg_width, bg_height;
	unsigned long color;
	int odd_x, odd_y;
	char *pfbmem;
	char bgcolor[disp_image_width * color_dept];
	char buf[disp_image_width * color_dept];

	fp = fopen(cachefile, "r");
	if (!fp) {
		perror(ss_program_name);
		cache->exist = SS_NOTEXIST;
		return -1;
	}

	pfbmem = ss_fbmem;
	bg_width = disp_image_width - cache->width;
	bg_height = disp_image_height - cache->height;

	if (cache->width < disp_image_width) {
		odd_x = bg_width % 2;
		odd_y = 0;

		color = 0;
		for (i = 0; i < (bg_width / 2 + odd_x); i++) {
			bgcolor[color++] = BGCOLOR_B;
			bgcolor[color++] = BGCOLOR_G;
			bgcolor[color++] = BGCOLOR_R;
		}
	} else {
		odd_x = 0;
		odd_y = cache->height % 2;

		color = 0;
		for (i = 0; i < disp_image_width; i++) {
			bgcolor[color++] = BGCOLOR_B;
			bgcolor[color++] = BGCOLOR_G;
			bgcolor[color++] = BGCOLOR_R;
		}
	}

	for (i = 0; i < (bg_height / 2); i++)
		pfbmem = mempcpy(pfbmem, bgcolor, disp_image_width * color_dept);

	i = 0;
	while (fread(buf, cache->width * color_dept, 1, fp) > 0) {
		pfbmem = mempcpy(pfbmem, bgcolor, (bg_width / 2) * color_dept);
		pfbmem = mempcpy(pfbmem, buf, cache->width * color_dept);
		pfbmem = mempcpy(pfbmem, bgcolor, (bg_width / 2 + odd_x) * color_dept);
		i++;
	}
	if (i != cache->height) {
		PRINT_ERR("cache file is broken");
		cache->exist = SS_NOTEXIST;
		fclose(fp);
		return -1;
	}

	for (i = 0; i < (bg_height / 2 + odd_y); i++)
		pfbmem = mempcpy(pfbmem, bgcolor, disp_image_width * color_dept);

	if (ferror(fp)) {
		PRINT_ERR("image display err");
		cache->exist = SS_NOTEXIST;
		return -1;
	}

	fclose(fp);

	return 0;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static int get_jpeg_image_size(unsigned long *width, unsigned long *height,
			   const char *imagefile)
{
	int fd;
	int ret;
	char format_simbol[2];
	FILE* fp;
	struct jpeg_decompress_struct cinfo;
	struct error_manager jerr;

	fd = open(imagefile, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = read(fd, format_simbol, 2);
	if (ret < 0)
		return -1;

	close(fd);

	if ((format_simbol[0] == 0xFF) && (format_simbol[1] == 0xD8)) {
		fp = fopen(imagefile, "r");
		if(!fp)
			return -1;

		cinfo.err = jpeg_std_error(&jerr.pub);
		jerr.pub.error_exit = error_exit;

		if(setjmp(jerr.setjmp_buffer)){
			jpeg_destroy_decompress(&cinfo);
			fclose(fp);
			return 0;
		}

		jpeg_create_decompress(&cinfo);
		jpeg_stdio_src(&cinfo, fp);
		jpeg_read_header(&cinfo, TRUE);
		jpeg_start_decompress(&cinfo);

		*width = cinfo.output_width;
		*height = cinfo.output_height;

		jpeg_destroy_decompress(&cinfo);

		fclose(fp);
	}

	return 0;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static int convert_image(ss_cache *cache, const char *imagefile, const char *cachefile)
{
	MagickWand *magick_wand;
	MagickBooleanType magick_stat;
	char pixels[disp_image_width * disp_image_height * color_dept];
	FILE * fp;
	unsigned long height, width;
	int ret;

	height = width = 0;
	ret = get_jpeg_image_size(&width, &height, imagefile);
	if (ret) {
		PRINT_ERR("%s: read err", imagefile);
		return CONVERT_IMAGE_NORMAL_ERR;
	}
	if (width * height > MAX_JPEG_IMAGE_WIDTH * MAX_JPEG_IMAGE_HEIGHT) {
		PRINT_ERR("%s:  too large", imagefile);
		return CONVERT_IMAGE_NORMAL_ERR;
	}

	MagickWandGenesis();
	magick_wand = NewMagickWand();

	/* read an image. */
	magick_stat = MagickReadImage(magick_wand, imagefile);
	if (!magick_stat) {
		PRINT_ERR("%s: read err", imagefile);
		return CONVERT_IMAGE_NORMAL_ERR;
	}

	width  = MagickGetImageWidth(magick_wand);
	height = MagickGetImageHeight(magick_wand);

	if (width * disp_image_height < height * disp_image_width) {
		width = width * disp_image_height / height;
		height = disp_image_height;
	} else {
		height = height * disp_image_width / width;
		width = disp_image_width;
	}

	cache->width = width;
	cache->height = height;

	magick_stat = MagickSampleImage(magick_wand, width, height);
	if (!magick_stat) {
		PRINT_ERR("%s: resize err", imagefile);
		return CONVERT_IMAGE_NORMAL_ERR;
	}

	/* get pixel data in blue-green-red order */
	magick_stat = MagickGetImagePixels(magick_wand, 0, 0, width, height, "BGR",
					   CharPixel, pixels);
	if (!magick_stat) {
		PRINT_ERR("%s: can't get pixel data", imagefile);
		return CONVERT_IMAGE_NORMAL_ERR;
	}

	DestroyMagickWand(magick_wand);
	MagickWandTerminus();

	fp = fopen(cachefile, "w+");
	if (!fp) {
		perror(ss_program_name);
		return CONVERT_IMAGE_FATAL_ERR;
	}

	fwrite(pixels, 1, width * height * color_dept, fp);
	if (feof(fp)) {
		PRINT_ERR("cache file create err");
		return CONVERT_IMAGE_FATAL_ERR;
	}

	fclose(fp);

	cache->exist = SS_EXISTING;

	return CONVERT_IMAGE_EXIT_SUCCESS;
}

/****************************************************************************************
 *
 ***************************************************************************************/
static int delete_extra_cache(const char *work_dir, ss_imagefile *image)
{
	int i = 1;
	int ret;
	unsigned long image_count = 0;
	char cache_file[PATH_MAX];
	ss_imagefile *forward, *backward, *target;

	if (image->cache.exist)
		image_count++;

	forward = backward = image;
	while (1) {
		if (i++ % 3)
			target = forward = forward->next;
		else
			target = backward = backward->prev;

		if (forward == backward)
			return 0;

		if (target->cache.exist) {
			if (image_count >= MAX_NR_CACHE - 1) {
				ret = snprintf(cache_file, PATH_MAX, "%s%s%s",
					       work_dir, target->filename, CACHE_FILE_EXT);
				if (ret >= PATH_MAX || ret < 0) {
					PRINT_ERR("delete_extra_cache : path create err");
					return -1;
				}
				unlink(cache_file);
				target->cache.exist = SS_NOTEXIST;
			} else
				image_count++;
		}
	}

	return 0;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static ss_imagefile *do_unlink_image(ss_imagefile *image,
				     int direct)
{
	pthread_mutex_lock(&ss_cache_manager_cancel_lock);
	pthread_mutex_lock(&ss_current_lock);

	/* when there is only one imagefile */
	if (image == image->prev) {
		PRINT_ERR("no image");
		return NULL;
	}

	image->prev->next = image->next;
	image->next->prev = image->prev;

	if (image == ss_current) {
		if (direct == SS_DISP_NEXT)
			ss_current = ss_current->next;
		else
			ss_current = ss_current->prev;
	}

	pthread_mutex_unlock(&ss_cache_manager_cancel_lock);
	pthread_mutex_unlock(&ss_current_lock);

	return image;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static void cache_manager_cleanup()
{
	pthread_mutex_unlock(&ss_current_lock);
	pthread_mutex_unlock(&ss_print_message_lock);
	pthread_mutex_unlock(&ss_cache_manager_cancel_lock);
}


/****************************************************************************************
 *
 ***************************************************************************************/
static void *cache_manager(void *argument)
{
	int nr_cache = 0;
	int ret;
	char imagefile[PATH_MAX];
	char cachefile[PATH_MAX];
	ss_imagefile *forward;
	ss_imagefile *backward;
	ss_imagefile *target;
	ss_thread_arg *arg;

	pthread_cleanup_push(cache_manager_cleanup, NULL);

	arg = (ss_thread_arg *)argument;

	target = forward = backward = ss_current;

	do {
		pthread_testcancel();

		/* calculate next target */
		if (nr_cache) {
			if (nr_cache % 3)
				target = forward = forward->next;
			else
				target = backward = backward->prev;
		} else {
			pthread_mutex_lock(&ss_current_lock);
			if (ss_current == target)
				target = forward = backward = ss_current;
			pthread_mutex_unlock(&ss_current_lock);
		}

		/* build image file name and cache file name */
		ret = snprintf(imagefile, PATH_MAX, "%s%s",
			       arg->image_dir, target->filename);
		if (ret >= PATH_MAX || ret < 0) {
			target = do_unlink_image(target, arg->direction);
			if (!target) {
				arg->state = 1;
				pthread_exit(NULL);
			}
			continue ;
		}
		ret = snprintf(cachefile, PATH_MAX, "%s%s%s",
			       arg->work_dir, target->filename, CACHE_FILE_EXT);
		if (ret >= PATH_MAX || ret < 0) {
			target = do_unlink_image(target, arg->direction);
			if (!target) {
				arg->state = 1;
				pthread_exit(NULL);
			}
			continue;
		}

		pthread_testcancel();

		/* do the actual cache build and display the data if
		 * the target is the current */
		if (!target->cache.exist) {
			pthread_mutex_lock(&ss_cache_manager_cancel_lock);
			delete_extra_cache(arg->work_dir, ss_current);
			ret = convert_image(&(target->cache), imagefile, cachefile);
			pthread_mutex_unlock(&ss_cache_manager_cancel_lock);

			if (ret != CONVERT_IMAGE_EXIT_SUCCESS) {
				if (ret == CONVERT_IMAGE_NORMAL_ERR) {
					target = do_unlink_image(target, arg->direction);
					if (!target) {
						arg->state = 1;
						pthread_exit(NULL);
					}
				} else {
					arg->state = 1;
					pthread_exit(NULL);
				}
				continue;
			} else if (target == ss_current)
				disp_image(&(target->cache), cachefile);
		}

		nr_cache++;
	} while (nr_cache < MAX_NR_CACHE);


	pthread_cleanup_pop(0);
	pthread_exit(NULL);
}


/****************************************************************************************
 *
 ***************************************************************************************/
static int chk_dir_and_cpy(char *dest,  const char *src)
{
	int ret;
	char *term;
	unsigned long name_len;
	struct stat st;

	term = strrchr(src, '\0');
	name_len = term - src;

	if (!term)
		return -1;

	if ((name_len < PATH_MAX - 2) && (*(term - 1) == '/'))
		strcpy(dest, src);
	else if ((name_len < (PATH_MAX - 3)) && (*(term - 1) != '/')) {
		strcpy(dest, src);
		*(dest + name_len) = '/';
		*(dest + name_len + 1) = '\0';
	} else
		return -1;

	ret = lstat(dest, &st);

	if (!S_ISDIR(st.st_mode) || ret)
		return -1;

	return 0;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static void init_cache(const char *image_dir)
{
	DIR *dir;
	struct dirent *dp;
	ss_imagefile *head, *curr, *tail;
	ss_imagefile *s;
	unsigned long name_len;

	dir = opendir(image_dir);

	if (!dir)
		PERROR_AND_HALT();

	/* search first regular file */
	tail = head = NULL;
	while ((dp = readdir(dir))) {
		if (dp->d_type != DT_REG)
			continue;

		name_len = strlen(dp->d_name);
		head = xmalloc(sizeof(ss_imagefile) + name_len + 1);
		tail = head;

		strcpy(head->filename, dp->d_name);
		head->cache.exist = SS_NOTEXIST;

		break;
	}

	/* there is no regular file */
	if (!head)
		PRINT_ERR_AND_HALT("no image");

	while ((dp = readdir(dir))) {
		if (dp->d_type != DT_REG)
			continue;

		name_len = strlen(dp->d_name);
		curr = xmalloc(sizeof(ss_imagefile) + name_len + 1);

		strcpy(curr->filename, dp->d_name);
		curr->cache.exist = SS_NOTEXIST;

		s = head;
		while (1) {
			if (strverscmp(curr->filename, s->filename) < 0) {
				if (s != head) {
					s->prev->next = curr;
					curr->prev = s->prev;
				} else
					head = curr;
				curr->next = s;
				s->prev = curr;

				break;
			}

			if (s == tail) {
				s->next = curr;
				curr->prev = s;
				tail = curr;
				break;
			}
			s = s->next;
		}
	}

	closedir(dir);

	ss_current = head;
	head->prev = tail;
	tail->next = head;
}


/****************************************************************************************
 *
 ***************************************************************************************/
static void *tact_switch_manager(void *argument)
{
	int fd_sw;
	int ret, sw_state;
	char cachefile[PATH_MAX];
	struct input_event event, event_save;
	ss_thread_arg *arg;
	fd_set Mask;

	arg = (ss_thread_arg *)argument;
	timerclear(&(event_save.time));

	fd_sw = open(EVENT_SW, O_RDONLY); /* tact switch open */
	if (fd_sw < 0) {
		PRINT_ERR("%s open err", EVENT_SW);
		arg->state = 1;
		pthread_exit(NULL);
	}

	while (1) {
		FD_ZERO(&Mask);
		FD_SET(fd_sw, &Mask);

		ret = select(fd_sw + 1, &Mask, NULL, NULL, NULL);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			else {
				PRINT_ERR("%s select err", EVENT_SW);
				arg->state = 1;
				close(fd_sw);
				pthread_exit(NULL);
			}
		}

		ret = read(fd_sw, &event, sizeof(event));
		if (ret != sizeof(event)) {
			PRINT_ERR("%s read err", EVENT_SW);
			arg->state = 1;
			close(fd_sw);
			pthread_exit(NULL);
		}

		if ((event.type != EV_SW) || event.code)
			continue;

		if (event.value) { /* held down */
			event_save = event;
			continue;
		} else { /* release */
			event_save.time.tv_sec += SW_ON_POLL_SEC;
			if (timercmp(&(event.time), &(event_save.time), >))
				sw_state = 1;
			else
				sw_state = 0;
		}

		pthread_mutex_lock(&ss_current_lock);
		if (sw_state) {
			ss_current = ss_current->prev;
			arg->direction = SS_DISP_PREV;
		} else {
			ss_current = ss_current->next;
			arg->direction = SS_DISP_NEXT;
		}

		if (ss_current->cache.exist) {
			ret = snprintf(cachefile, PATH_MAX, "%s%s%s", arg->work_dir,
				       ss_current->filename, CACHE_FILE_EXT);
			if (ret >= PATH_MAX || ret < 0) {
				PRINT_ERR("tact_switch_manager : path create err");
				arg->state = 1;
				pthread_exit(NULL);
			}
			disp_image(&(ss_current->cache), cachefile);
		}

		pthread_cond_signal(&ss_switch_cond);
		pthread_mutex_unlock(&ss_current_lock);
	}
}

/****************************************************************************************
 *
 ***************************************************************************************/
static void sighandler()
{
	int ret;
	int fd_led;

	fd_led = open(DEVICE_LED, O_RDWR);

	if (fd_led == -1) {
		perror(ss_program_name);
	} else {
		ret = ioctl(fd_led, LED_RED_OFF);
		if (ret == -1)
			perror(ss_program_name);

		close(fd_led);
	}

	exit(EXIT_SUCCESS);
}


/****************************************************************************************
 *
 ***************************************************************************************/
int main(int argc, char *argv[])
{
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;
	int fd_fb;
	int ret;
	char work_dir[PATH_MAX];
	char image_dir[PATH_MAX];
	pthread_attr_t attr;
	pthread_t sw_thread_id, ca_thread_id;
	ss_thread_arg th_arg;

	signal(SIGTERM, sighandler);

	ss_program_name = argv[0];
	pthread_mutex_init(&ss_print_message_lock, NULL);

	if (argc < 3)
		PRINT_ERR_AND_HALT("Usage: %s work_dir image_dir", ss_program_name);

	ret = chk_dir_and_cpy(work_dir, argv[1]);
	if (ret)
		PRINT_ERR_AND_HALT("%s is incorrect", argv[1]);

	ret = chk_dir_and_cpy(image_dir, argv[2]);
	if (ret)
		PRINT_ERR_AND_HALT("%s is incorrect", argv[2]);

	fd_fb = open(DEVICE_FB, O_RDWR); /* frame buffer open */
	if (fd_fb < 0)
		PERROR_AND_HALT();
	
	ret = ioctl(fd_fb, FBIOGET_FSCREENINFO, &fb_fix);
	if (ret < 0)
		PRINT_ERR_AND_HALT("Could not get "
				   "fixed screen information!\n");

	ret = ioctl(fd_fb, FBIOGET_VSCREENINFO, &fb_var);
	if (ret < 0)
		PRINT_ERR_AND_HALT("Could not get variable "
				   "screen information!\n" );

	if ((color_dept * 8) != fb_var.bits_per_pixel)
		PRINT_ERR_AND_HALT("Not support color depth!\n");

	disp_image_width = fb_var.xres;
	disp_image_height = fb_var.yres;

	if ((disp_image_width * disp_image_height * color_dept) !=
	    fb_fix.smem_len)
		PRINT_ERR_AND_HALT("Not support framebuffer parameters!\n");

	ss_fbmem = mmap(NULL, fb_fix.smem_len,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
	if (ss_fbmem == MAP_FAILED)
		PRINT_ERR_AND_HALT("Could not mmap the framebuffer!\n");

	init_cache(image_dir);

	pthread_attr_init(&attr); /* init thread attribute */
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_cond_init(&ss_switch_cond, NULL);
	pthread_mutex_init(&ss_current_lock, NULL);
	pthread_mutex_init(&ss_cache_manager_cancel_lock, NULL);

	th_arg.state = 0;
	th_arg.direction = SS_DISP_NEXT;
	th_arg.work_dir = work_dir;
	th_arg.image_dir = image_dir;

	if (pthread_create(&sw_thread_id, &attr, tact_switch_manager, &th_arg))
		PRINT_ERR_AND_HALT("switch manager create err");

	while (!pthread_create(&ca_thread_id, &attr, cache_manager, &th_arg)) {
		if (th_arg.state) {
			pthread_cancel(sw_thread_id);
			pthread_cancel(ca_thread_id);
			halt();
		}

		pthread_mutex_lock(&ss_current_lock);
		pthread_cond_wait(&ss_switch_cond, &ss_current_lock);
		pthread_mutex_unlock(&ss_current_lock);

		pthread_mutex_lock(&ss_cache_manager_cancel_lock);
		pthread_cancel(ca_thread_id);
		pthread_mutex_unlock(&ss_cache_manager_cancel_lock);
	}

	PRINT_ERR_AND_HALT("cache manager create err");

	return 0;
}
