#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#include "common.h"

#define BL_BRIGHTNESS_FILE "/sys/devices/platform/pwm-backlight/" \
			   "backlight/pwm-backlight/brightness"

#define BACKLIGHT_MIN (30)
#define BACKLIGHT_MAX (250)

static int get_backlight_value(void)
{
#if  defined(__arm__)
	int fd, ret, val;
	char buf[8];

	fd = open(BL_BRIGHTNESS_FILE, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if (ret < 0)
		return -1;

	ret = sscanf(buf, "%d\n", &val);
	if (ret != 1)
		return -1;

	return val;
#else
	return 128;
#endif
}

static void set_backlight_value(int val)
{
#if defined(__arm__)
	int fd;
	char buf[8];

	snprintf(buf, sizeof(buf)-1, "%d", val);

	fd = open(BL_BRIGHTNESS_FILE, O_WRONLY);
	if (fd < 0)
		return;

	write(fd, buf, strlen(buf));

	close(fd);
#endif
}

static void backlight_hscale_handler(GtkAdjustment *adj, void *data)
{
	int val = gtk_adjustment_get_value(adj);
	set_backlight_value(val);
}

int task_backlight_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *hscale;
	GtkAdjustment *adj;
	int val;

	val = get_backlight_value();

	hscale = gtk_hscale_new_with_range(BACKLIGHT_MIN, BACKLIGHT_MAX, 1);
	adj = gtk_range_get_adjustment((GtkRange *)hscale);
	gtk_adjustment_set_value(adj, val);

	g_signal_connect(G_OBJECT(adj), "value_changed",
			 G_CALLBACK(backlight_hscale_handler), NULL);

	gtk_box_pack_start(GTK_BOX(box), hscale, TRUE, TRUE, 0);
	gtk_widget_show(hscale);

	return 0;
}
