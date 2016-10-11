#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include <sys/time.h>

#include "common.h"

struct linux_rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

#ifndef RTC_RD_TIME
#define RTC_RD_TIME       _IOR('p', 0x09, struct linux_rtc_time)
#endif
#ifndef RTC_SET_TIME
#define RTC_SET_TIME      _IOW('p', 0x0a, struct linux_rtc_time)
#endif

struct mySpinButton {
	GtkWidget *entry;
	GtkWidget *bt_up, *bt_down, *bt_current;
	gint timer;
	int interval;
	int max, min;
};

static struct mySpinButton hspin_info = {
	.max = 23,
	.min = 0,
};
static struct mySpinButton mspin_info = {
	.max = 59,
	.min = 0,
};
static struct mySpinButton sspin_info = {
	.max = 59,
	.min = 0,
};

static GtkWidget *calendar;

static void update_sysclk(struct tm *tm)
{
	struct timeval tv;
	memset(&tv, 0, sizeof(struct timeval));

	tv.tv_sec = mktime(tm);
	settimeofday(&tv, NULL);
}

static int rtc_write(struct tm *tm)
{
	struct tm utc_tm;
	time_t local_time;
	int fd, ret, err;

#ifdef DEBUG
	g_print("Set RTC: %04d/%02d/%02d %02d:%02d:%02d\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
#endif

	tzset();
	local_time = mktime(tm);
	if (local_time == -1)
		return -EINVAL;
	gmtime_r(&local_time, &utc_tm);

	fd = open("/dev/rtc", O_WRONLY);
	if (fd == -1)
		return -errno;
	ret = ioctl(fd, RTC_SET_TIME, &utc_tm);
	err = errno;
	close(fd);
	if (ret == -1)
		return err;

	return 0;
}

static int rtc_read(struct tm *tm)
{
	struct tm rtc_tm, local_tm;
	time_t utc_time;
	int fd, ret, err;

	memset(&rtc_tm, 0, sizeof(struct tm));
	memset(&local_tm, 0, sizeof(struct tm));

	fd = open("/dev/rtc", O_RDONLY);
	if (fd == -1)
		return -errno;
	ret = ioctl(fd, RTC_RD_TIME, &rtc_tm);
	err = errno;
	close(fd);
	if (ret == -1)
		return err;

	tzset();
	utc_time = timegm(&rtc_tm);
	localtime_r(&utc_time, &local_tm);
	memcpy(tm, &local_tm, sizeof(struct tm));

#ifdef DEBUG
	g_print("Get RTC: %04d/%02d/%02d %02d:%02d:%02d\n",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
#endif
	return 0;
}

static gboolean myspin_timeout(gpointer data)
{
	struct mySpinButton *spin = data;

	g_source_remove(spin->timer);
	gtk_signal_emit_by_name(GTK_OBJECT(spin->bt_current), "pressed", data);

	return TRUE;
}

static void myspin_pressed_callback(GtkWidget *widget, gpointer data)
{
	struct mySpinButton *spin = data;
	const gchar *text;
	gchar buf[64];
	int increment = 0;
	int val;

	if (widget == spin->bt_up)
		increment = 1;
	else if (widget == spin->bt_down)
		increment = -1;

	text = gtk_entry_get_text(GTK_ENTRY(spin->entry));
	val = (int)strtol(text, NULL, 10) + increment;
	if (val < spin->min || spin->max < val)
		return;
	sprintf(buf, "%02d", val);
	gtk_entry_set_text(GTK_ENTRY(spin->entry), buf);

	spin->bt_current = widget;
	spin->timer = g_timeout_add(spin->interval,
				    myspin_timeout,
				    data);
	spin->interval = 50;
}

static void myspin_released_callback(GtkWidget *widget, gpointer data)
{
	struct mySpinButton *spin = data;

	if (spin->timer)
		g_source_remove(spin->timer);
	spin->bt_current = NULL;
	spin->interval = 200;
}

static void set_rtc(GtkWidget *widget, gpointer data)
{
	struct tm local_tm;
	int ret;

	memset(&local_tm, 0, sizeof(struct tm));

	local_tm.tm_hour = (int)strtol(
		gtk_entry_get_text(GTK_ENTRY(hspin_info.entry)),
		NULL, 10);
	local_tm.tm_min = (int)strtol(
		gtk_entry_get_text(GTK_ENTRY(mspin_info.entry)),
		NULL, 10);
	local_tm.tm_sec = (int)strtol(
		gtk_entry_get_text(GTK_ENTRY(sspin_info.entry)),
		NULL, 10);
	gtk_calendar_get_date(GTK_CALENDAR(calendar),
			      (guint *)&local_tm.tm_year,
			      (guint *)&local_tm.tm_mon,
			      (guint *)&local_tm.tm_mday);

	if (local_tm.tm_year < 2000 || 2099 < local_tm.tm_year) {
		error_msg("Can not set date!", "invalied date.");
		return;
	}
	local_tm.tm_year -= 1900;

	update_sysclk(&local_tm);
	ret = rtc_write(&local_tm);
	if (ret < 0)
		error_msg("failed set rtc", strerror(-ret));
}

static void get_rtc(GtkWidget *widget, gpointer data)
{
	struct tm local_tm;
	char thour[4], tmin[4], tsec[4];
	int ret;

	ret = rtc_read(&local_tm);
	if (ret < 0) {
		error_msg("failed get rtc", strerror(-ret));
		return;
	}

	snprintf(thour, 3, "%02d", local_tm.tm_hour);
	gtk_entry_set_text(GTK_ENTRY(hspin_info.entry), thour);
	snprintf(tmin, 3, "%02d", local_tm.tm_min);
	gtk_entry_set_text(GTK_ENTRY(mspin_info.entry), tmin);
	snprintf(tsec, 3, "%02d", local_tm.tm_sec);
	gtk_entry_set_text(GTK_ENTRY(sspin_info.entry), tsec);

	gtk_calendar_select_month(GTK_CALENDAR(calendar),
				  local_tm.tm_mon,
				  local_tm.tm_year + 1900);
	gtk_calendar_select_day(GTK_CALENDAR(calendar), local_tm.tm_mday);
}

static GtkWidget *gtk_myspin_button_new(struct mySpinButton *spin)
{
	GtkWidget *box;
	GtkWidget *arrow_up, *arrow_down;

	box = gtk_hbox_new(FALSE, 0);
	spin->interval = 200,

	/* entry */
	spin->entry = gtk_entry_new();
	gtk_widget_set_usize(GTK_WIDGET(spin->entry), 30, 30);
	gtk_entry_set_editable(GTK_ENTRY(spin->entry), FALSE);
	gtk_entry_set_alignment(GTK_ENTRY(spin->entry), 0.5);
	gtk_entry_set_text(GTK_ENTRY(spin->entry), "00");

	/* button */
	spin->bt_up = gtk_button_new();
	spin->bt_down = gtk_button_new();
	arrow_up = gtk_arrow_new(GTK_ARROW_UP, 0);
	arrow_down = gtk_arrow_new(GTK_ARROW_DOWN, 0);
	gtk_button_set_relief(GTK_BUTTON(spin->bt_up), GTK_RELIEF_NONE);
	gtk_button_set_relief(GTK_BUTTON(spin->bt_down), GTK_RELIEF_NONE);
	gtk_button_set_image(GTK_BUTTON(spin->bt_up), arrow_up);
	gtk_button_set_image(GTK_BUTTON(spin->bt_down), arrow_down);

	/* pack */
	gtk_box_pack_start(GTK_BOX(box), spin->bt_up, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), spin->entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), spin->bt_down, FALSE, FALSE, 0);

	/* signal */
	gtk_signal_connect(GTK_OBJECT(spin->bt_up),
			   "pressed",
			   G_CALLBACK(myspin_pressed_callback),
			   (gpointer)spin);
	gtk_signal_connect(GTK_OBJECT(spin->bt_down),
			   "pressed",
			   G_CALLBACK(myspin_pressed_callback),
			   (gpointer)spin);
	gtk_signal_connect(GTK_OBJECT(spin->bt_up),
			   "released",
			   G_CALLBACK(myspin_released_callback),
			   (gpointer)spin);
	gtk_signal_connect(GTK_OBJECT(spin->bt_down),
			   "released",
			   G_CALLBACK(myspin_released_callback),
			   (gpointer)spin);

	return box;
}

int task_rtc_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *vbox, *timebox, *btbox;
	GtkWidget *sep;
	GtkWidget *timealign;
	GtkWidget *hspin, *mspin, *sspin;
	GtkWidget *timel, *colon1l, *colon2l;
	GtkWidget *setbt, *getbt;

	vbox = gtk_vbox_new(FALSE, 0);
	timebox = gtk_hbox_new(FALSE, 0);
	btbox = gtk_hbox_new(FALSE, 0);

	calendar = gtk_calendar_new();
	sep = gtk_hseparator_new();
	timealign = gtk_alignment_new(0.5, 0.5, 0, 0);

	hspin = gtk_myspin_button_new(&hspin_info);
	mspin = gtk_myspin_button_new(&mspin_info);
	sspin = gtk_myspin_button_new(&sspin_info);

	timel = gtk_label_new("Time");
	colon1l = gtk_label_new(":");
	colon2l = gtk_label_new(":");
	setbt = gtk_button_new_with_label("Set");
	getbt = gtk_button_new_with_label("Get");

	gtk_widget_set_size_request(setbt, DEFAULT_BUTTON_WIDTH, -1);
	gtk_widget_set_size_request(getbt, DEFAULT_BUTTON_WIDTH, -1);

	gtk_box_pack_start(GTK_BOX(box), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), calendar, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), timealign, FALSE, FALSE, 3);

	gtk_container_add(GTK_CONTAINER(timealign), timebox);
	gtk_box_pack_start(GTK_BOX(timebox), timel, TRUE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(timebox), hspin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timebox), colon1l, TRUE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(timebox), mspin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(timebox), colon2l, TRUE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(timebox), sspin, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), sep, TRUE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbox), btbox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(btbox), setbt, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(btbox), getbt, TRUE, FALSE, 0);

	gtk_signal_connect(GTK_OBJECT(setbt),
			   "clicked",
			   G_CALLBACK(set_rtc), (gpointer)NULL);
	gtk_signal_connect(GTK_OBJECT(getbt),
			   "clicked",
			   G_CALLBACK(get_rtc), (gpointer)NULL);

	get_rtc(NULL, 0);

	gtk_widget_show_all(box);

	return 0;
}
