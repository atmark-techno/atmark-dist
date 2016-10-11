
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>

#define PROGRAM_NAME "Armadillo-500 FX Demo"

/* build-time configuartion */
//#define CONFIG_DISPLAY_CLOSE_BUTTON
#define CONFIG_FULL_SCREEN

#define DEFAULT_XRES (640)
#define DEFAULT_YRES (480)

#define DEFAULT_BUTTON_WIDTH	(80)
#define DEFAULT_BUTTON_HEIGHT	(50)

#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])

#define JMP_GPIO_PATH "/sys/devices/platform/armadillo5x0_gpio.0/" \
			"ports/gpio1_8_data"

#define BL_BRIGHTNESS_FILE "/sys/devices/platform/mxc-bl.0/backlight/" \
		"mxc-bl/brightness"

#define SOUND_TEST_FILE "/usr/share/sounds/test.wav"

#define LED_DIR_BASE "/sys/devices/platform/armadillo5x0_led.0/leds/status/"
#define LED_BRIGHTNESS_FILE	LED_DIR_BASE "brightness"
#define LED_TRIGGER_FILE	LED_DIR_BASE "trigger"
#define LED_TRIGGER		"timer"

#define SSD_DEVICE_FILE "/dev/sda"

static GtkWidget *window;

enum {
	TASK_LED,
	TASK_JUMPER,
	TASK_KEYPAD,
	TASK_BACKLIGHT,
	TASK_SOUND,
	TASK_SSD,
};

typedef void (*activate_callback)(gpointer *data);
typedef void (*deactivate_callback)(gpointer *data);

struct task {
	char			*name;
	char			*name_long;
	GtkWidget		*button;
	GtkWidget		*box;	
	int (*setup)(struct task *task, GtkWidget *box);
	activate_callback	activate;
	gpointer		*activate_data;
	deactivate_callback	deactivate;
	gpointer		*deactivate_data;
	int			usable;
	int			active;
};

static void task_set_activate_callback(struct task *task,
	activate_callback callback, gpointer *data)
{
	task->activate = callback;
	task->activate_data = data;
}

static void task_set_deactivate_callback(struct task *task,
	deactivate_callback callback, gpointer *data)
{
	task->deactivate = callback;
	task->deactivate_data = data;
}

/* max error message detail length: 512 */
static void error_msg(gchar *err_brief, gchar *err_detail_fmt, ...)
{
	va_list ap;
	char fmt_buf[512];

	va_start(ap, err_detail_fmt);

	vsnprintf(fmt_buf, sizeof(fmt_buf)-1, err_detail_fmt, ap);

	GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s", err_brief);

	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
		"%s", fmt_buf);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG (dialog));

	gtk_widget_destroy(dialog);

	va_end(ap);
}

enum {
	LED_STATE_ON,
	LED_STATE_OFF,
};
static int led_state;
GtkWidget *led_buttons[2];

static gboolean led_expose_event(GtkWidget *widget, GdkEventExpose *event,
		gpointer data)
{
	cairo_t *cr;
	int width, height;

	gtk_widget_get_size_request(widget, &width, &height);

	cr = gdk_cairo_create(widget->window);

	cairo_set_line_width(cr, 1);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_arc(cr, width/2, height/2, 
		(width < height ? width : height) / 2 - 10, 0, 2 * M_PI);
	cairo_stroke_preserve(cr);

	if (led_state == LED_STATE_ON)
		cairo_set_source_rgb(cr, 1, 0, 0); 
	else
		cairo_set_source_rgb(cr, 0, 0, 0); 

	cairo_fill(cr);

	cairo_destroy(cr);

	return FALSE;
}

static void led_set_state(int state)
{
	int ret, fd;

	if (state == LED_STATE_ON) {
		fd = open(LED_TRIGGER_FILE, O_WRONLY);
		if (fd < 0) 
			return;
		ret = write(fd, LED_TRIGGER, strlen(LED_TRIGGER));
		close(fd);
		if (ret < strlen(LED_TRIGGER))
			return;
	}

	fd = open(LED_BRIGHTNESS_FILE, O_WRONLY);
	if (fd < 0) 
		return;

	if (state == LED_STATE_ON)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);

	close(fd);
}

static void led_button_callback(GtkWidget *widget, gpointer data)
{
	GtkWidget *drawarea = data;

	if (widget == led_buttons[LED_STATE_ON]) {
		led_set_state(LED_STATE_ON);
		led_state = LED_STATE_ON;
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(led_buttons[LED_STATE_OFF]),
			FALSE);
	} else {
		led_set_state(LED_STATE_OFF);
		led_state = LED_STATE_OFF;
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(led_buttons[LED_STATE_ON]),
			FALSE);
	}

	gdk_window_invalidate_rect(drawarea->window, NULL, FALSE);
}

static int led_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *drawarea;
	GtkWidget *draw_align, *control_align;
	GtkWidget *control_box;
	GtkWidget *b;
	char buf[8];
	int ret, fd, val;

	memset(buf, 0, sizeof(buf));

	fd = open(LED_BRIGHTNESS_FILE, O_RDONLY);
	if (fd < 0) 
		return -1;

	ret = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if (ret < 0)
		return -1;

	ret = sscanf(buf, "%d\n", &val);
	if (ret != 1)
		return -1;

	if (val == 1)
		led_state = LED_STATE_ON;
	else
		led_state = LED_STATE_OFF;

	/* alignment to center LED graphic area */
	draw_align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(box), draw_align, FALSE, FALSE, 0);
	gtk_widget_show(draw_align);

	/* LED graphic area */
	drawarea = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawarea, 100, 100);
	gtk_container_add(GTK_CONTAINER(draw_align), drawarea);
	gtk_container_set_border_width(GTK_CONTAINER(draw_align), 20);
	g_signal_connect(G_OBJECT(drawarea), "expose-event",
      		G_CALLBACK(led_expose_event), NULL);
	gtk_widget_show(drawarea);

	/* alignment to center on/off buttons */
	control_align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(box), control_align, FALSE, FALSE, 0);
	gtk_widget_show(control_align);

	/* horizontal box for on/off buttons */
	control_box = gtk_hbox_new(FALSE, 20);
	gtk_container_add(GTK_CONTAINER(control_align), control_box);
	gtk_widget_set_size_request(control_box, -1, DEFAULT_BUTTON_HEIGHT);
	gtk_widget_show(control_box);

	/* on toggle button  */
	led_buttons[LED_STATE_ON] = b = gtk_toggle_button_new_with_label("ON");
	gtk_box_pack_start(GTK_BOX(control_box), b, FALSE, FALSE, 0);
	gtk_widget_set_size_request(b, DEFAULT_BUTTON_WIDTH, -1);
	g_signal_connect(G_OBJECT(b), "pressed", G_CALLBACK(led_button_callback),
		drawarea);
	gtk_widget_show(b);

	/* off toggle button  */
	led_buttons[LED_STATE_OFF] = b = gtk_toggle_button_new_with_label("OFF");
	gtk_box_pack_start(GTK_BOX(control_box), b, FALSE, FALSE, 0);
	gtk_widget_set_size_request(b, DEFAULT_BUTTON_WIDTH, -1);
	g_signal_connect(G_OBJECT(b), "pressed", G_CALLBACK(led_button_callback),
		drawarea);
	gtk_widget_show(b);

	if (led_state == LED_STATE_ON) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(led_buttons[LED_STATE_ON]),
			TRUE);
	} else {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(led_buttons[LED_STATE_OFF]),
			TRUE);
	}

	return 0;
}

static void backlight_hscale_handler(GtkAdjustment *adj, void *data)
{
	char buf[8];
	int fd, val;

	val = gtk_adjustment_get_value(adj);

	memset(buf, 0, sizeof(buf));

	snprintf(buf, sizeof(buf)-1, "%d", val);

	fd = open(BL_BRIGHTNESS_FILE, O_WRONLY);
	if (fd < 0) 
		return;

	write(fd, buf, strlen(buf));

	close(fd);
}

#define BACKLIGHT_MIN	(50)
#define BACKLIGHT_MAX	(200)

static int backlight_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *hscale;
	GtkAdjustment *adj;
	char buf[8];
	int ret, fd, val;

	memset(buf, 0, sizeof(buf));

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

	hscale = gtk_hscale_new_with_range(BACKLIGHT_MIN, BACKLIGHT_MAX, 1);

	adj = gtk_range_get_adjustment((GtkRange *)hscale);
	gtk_adjustment_set_value(adj, val);

	g_signal_connect(G_OBJECT(adj), "value_changed",
		G_CALLBACK(backlight_hscale_handler), NULL);

	gtk_box_pack_start(GTK_BOX(box), hscale, TRUE, TRUE, 0);
	gtk_widget_show(hscale);

	return 0;

}

static void sound_callback(GtkWidget *widget, gpointer data)
{
	system("vplay " SOUND_TEST_FILE " > /dev/null");
}

static int sound_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *play_b;
	GtkWidget *align;

	/* alignment to center buttons */
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);
	gtk_widget_show(align);

	/* setup play button */
	play_b = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	gtk_container_add(GTK_CONTAINER(align), play_b);
	gtk_widget_set_size_request(play_b, DEFAULT_BUTTON_WIDTH,
		DEFAULT_BUTTON_HEIGHT);
	g_signal_connect(G_OBJECT(play_b), "clicked",
		G_CALLBACK(sound_callback), NULL);
	gtk_widget_show(play_b);

	return 0;
}

enum {
	JMP_STATE_UNKNOWN,
	JMP_STATE_ON,
	JMP_STATE_OFF,
};
static char *jumper_states[3];

static int jumper_update_state(GtkWidget *label)
{
	int fd, ret;
	char buf;

	fd = open(JMP_GPIO_PATH, O_RDONLY);
	if (fd < 0) 
		goto err;

	ret = read(fd, &buf, 1);
	if (ret < 1) {
		close(fd);
		goto err;
	}

	if (buf == '0') {
		gtk_label_set_markup(GTK_LABEL(label),
			jumper_states[JMP_STATE_ON]);
	} else if (buf == '1') {
		gtk_label_set_markup(GTK_LABEL(label),
			jumper_states[JMP_STATE_OFF]);
	} else
		goto err;

	close(fd);

	return 0;

 err:
	gtk_label_set_markup(GTK_LABEL(label), 
		jumper_states[JMP_STATE_UNKNOWN]);
	return -1;	
}

static gint jumper_timer(gpointer data)
{
	GtkWidget *state_label = (GtkWidget *)data;

	jumper_update_state(state_label);

	return TRUE;
}

static gint jumper_timer_handle;

static void jumper_activate(gpointer *data)
{
	GtkWidget *state_label = (GtkWidget *)data;

	jumper_update_state(state_label);

	jumper_timer_handle = g_timeout_add(1000, jumper_timer, data);
}

static void jumper_deactivate(gpointer *data)
{
	g_source_remove(jumper_timer_handle);
}

static int jumper_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *align;
	GtkWidget *state_label;
	int ret;

	/* check the jumper gpio file is usable */
	ret = access(JMP_GPIO_PATH, O_RDONLY);
	if (ret < 0) 
		return -1;

	jumper_states[JMP_STATE_ON] = g_markup_printf_escaped(
		"<span size=\"40000\">SHORTED</span>");
	jumper_states[JMP_STATE_OFF] = g_markup_printf_escaped(
		"<span size=\"40000\">OPEN</span>");
	jumper_states[JMP_STATE_UNKNOWN] = g_markup_printf_escaped(
		"<span size=\"40000\">UNKNOWN</span>");

	/* alignment to center buttons */
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);
	gtk_widget_show(align);

	state_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(align), state_label);
	gtk_widget_show(state_label);

	task_set_activate_callback(task, jumper_activate,
		(gpointer *)state_label);
	task_set_deactivate_callback(task, jumper_deactivate, NULL);

	return 0;
}

struct keypad_key {
	char		*label;
	int		key;
	GtkWidget	*button;
};

static struct keypad_key keypad_keys[24] = {
	{.label = "Home",	.key = 65366},
	{.label = "Menu",	.key = 65383},
	{.label = "Back",	.key = 65368},
	{.label = "F1",		.key = 65470},
	{.label = "Up",		.key = 65379},
	{.label = "F2",		.key = 65471},
	{.label = "Left",	.key = 65515},
	{.label = "Select",	.key = 65293},
	{.label = "Right",	.key = 65516},
	{.label = "Go",		.key = 65378},
	{.label = "Down",	.key = 65364},
	{.label = "End",	.key = 65367},
	{.label = "1",		.key = 49},
	{.label = "2",		.key = 50},
	{.label = "3",		.key = 51},
	{.label = "4",		.key = 52},
	{.label = "5",		.key = 53},
	{.label = "6",		.key = 54},
	{.label = "7",		.key = 55},
	{.label = "8",		.key = 56},
	{.label = "9",		.key = 57},
	{.label = "F3",		.key = 65472},
	{.label = "0",		.key = 48},
	{.label = "F4",		.key = 65473},
};

static gboolean key_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	int i;
	GdkColor color;

	gdk_color_parse("red", &color);

	for (i = 0; i < ARRAY_SIZE(keypad_keys); i++) {

		if (event->keyval != keypad_keys[i].key)
			continue;

		if (event->type == GDK_KEY_PRESS) {
			gtk_widget_modify_bg(keypad_keys[i].button,
				GTK_STATE_NORMAL, &color);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
				keypad_keys[i].button), TRUE);
			gtk_button_pressed(GTK_BUTTON(keypad_keys[i].button));
		} else {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
				keypad_keys[i].button), FALSE);
		}
		break;

	}

	return TRUE;
}

static void keypad_activate(gpointer *data)
{
	int i;
	GdkColor color;

	gdk_color_parse("lightgrey", &color);

	for (i = 0; i < ARRAY_SIZE(keypad_keys); i++) {
			gtk_widget_modify_bg(keypad_keys[i].button,
				GTK_STATE_NORMAL, &color);
	}

	g_signal_connect(G_OBJECT(window), "key-press-event", 
		G_CALLBACK(key_event), NULL);
	g_signal_connect(G_OBJECT(window), "key-release-event", 
		G_CALLBACK(key_event), NULL);
}

static void keypad_deactivate(gpointer *data)
{
	g_signal_handlers_disconnect_by_func(G_OBJECT(window),
		G_CALLBACK(key_event), NULL);
}


static void keypad_button_callback(GtkWidget *widget, gpointer data)
{
	/* ensure button isn't toggled */
	gtk_toggle_button_set_active((GtkToggleButton *)widget, TRUE);
}

static int keypad_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *key_b;
	GtkWidget *align;
	GtkWidget *vbox, *line_box;
	int i, j, b = 0;

	/* alignment to center buttons */
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);
	gtk_widget_show(align);

	vbox = gtk_vbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(align), vbox);
	gtk_widget_show(vbox);

	for (i = 0; i < 8; i++) {

		line_box = gtk_hbox_new(FALSE, 20);
		gtk_box_pack_start(GTK_BOX(vbox), line_box, FALSE,
				FALSE, 0);
		gtk_widget_show(line_box);

		for (j = 0; j < 3; j++) {
			key_b = gtk_toggle_button_new_with_label(
				keypad_keys[b].label);
			gtk_widget_set_size_request(key_b, 80, -1);
			gtk_box_pack_start(GTK_BOX(line_box), key_b, FALSE,
				FALSE, 0);
			g_signal_connect(G_OBJECT(key_b), "pressed",
				G_CALLBACK(keypad_button_callback), NULL);
			gtk_widget_show(key_b);
			keypad_keys[b].button = key_b;
			b++;

		}

	}

	task_set_activate_callback(task, keypad_activate, NULL);
	task_set_deactivate_callback(task, keypad_deactivate, NULL);

	return 0;
}

enum {
	SSD_STATE_CON,
	SSD_STATE_DISCON,
};
static char *ssd_states[2];

static int ssd_update_state(GtkWidget *label)
{
	int fd, connected = 0;
	char buf;

	fd = open(SSD_DEVICE_FILE, O_RDONLY);
	if (fd > 0) {
		if (read(fd, &buf, 1) == 1)
			connected = 1;
		close(fd);
	}

	if (connected) {
		gtk_label_set_markup(GTK_LABEL(label),
			ssd_states[SSD_STATE_CON]);
	} else {
		gtk_label_set_markup(GTK_LABEL(label),
			ssd_states[SSD_STATE_DISCON]);
	}

	return 0;
}

static gint ssd_timer(gpointer data)
{
	GtkWidget *state_label = (GtkWidget *)data;

	ssd_update_state(state_label);

	return TRUE;
}

static gint ssd_timer_handle;

static void ssd_activate(gpointer *data)
{
	GtkWidget *state_label = (GtkWidget *)data;

	ssd_update_state(state_label);

	ssd_timer_handle = g_timeout_add(1000, ssd_timer, data);
}

static void ssd_deactivate(gpointer *data)
{
	g_source_remove(ssd_timer_handle);
}

static int ssd_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *align;
	GtkWidget *state_label;

	ssd_states[SSD_STATE_CON] = g_markup_printf_escaped(
		"<span size=\"30000\">Connected</span>");
	ssd_states[SSD_STATE_DISCON] = g_markup_printf_escaped(
		"<span size=\"30000\">Disconnected</span>");

	/* alignment to center buttons */
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(box), align, FALSE, FALSE, 0);
	gtk_widget_show(align);

	state_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(align), state_label);
	gtk_widget_show(state_label);

	task_set_activate_callback(task, ssd_activate, (gpointer *)state_label);
	task_set_deactivate_callback(task, ssd_deactivate, NULL);

	return 0;
}


struct task tasks[] = {
	[TASK_LED]	= {
		.name		= "LED",
		.name_long	= "Status LED Control",
		.setup		= led_setup,
	},
	[TASK_JUMPER] = {
		.name		= "Jumper",
		.name_long	= "Jumper 2 Status",
		.setup		= jumper_setup,
	},
	[TASK_KEYPAD] = {
		.name		= "Keypad",
		.name_long	= "Keypad Monitor",
		.setup		= keypad_setup,
	},
	[TASK_BACKLIGHT] = {
		.name		= "Backlight",
		.name_long	= "Backlight Control",
		.setup		= backlight_setup,
	},
	[TASK_SOUND] = {
		.name		= "Sound",
		.name_long	= "Sound Test",
		.setup		= sound_setup,
	},
	[TASK_SSD] = {
		.name		= "SSD",
		.name_long	= "SSD Status",
		.setup		= ssd_setup,
	},
};

static void task_activate(struct task *task)
{
	if (task->activate)
		task->activate(task->activate_data);
	gtk_widget_show(task->box);
	task->active = 1;
}

static void menu_callback(GtkWidget *widget, gpointer data)
{
	struct task *task = (struct task *)data;
	int i;

	if (task->active) {
		/* ensure button isn't toggled */
		gtk_toggle_button_set_active((GtkToggleButton *)task->button,
			FALSE);
		/* reactivate selected task */
		if (task->activate)
			task->activate(task->activate_data);
		return;
	}

	if (!task->usable) {
		error_msg("Not usable",
			"This function is not currently usable");
		return;
	}

	/* deactivate current task */
	for (i = 0; i < ARRAY_SIZE(tasks); i++) {
		if (tasks[i].active) {
			gtk_toggle_button_set_active(
				(GtkToggleButton *)tasks[i].button,
				FALSE);
			gtk_widget_hide(tasks[i].box);
			if (tasks[i].deactivate)
				tasks[i].deactivate(tasks[i].deactivate_data);
			tasks[i].active = 0;
			break;
		}
	}

	task_activate(task);
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	int i;

	/* deactivate current task */
	for (i = 0; i < ARRAY_SIZE(tasks); i++) {
		if (tasks[i].active) {
			if (tasks[i].deactivate)
				tasks[i].deactivate(tasks[i].deactivate_data);
			break;
		}
	}

	gtk_main_quit();

	return FALSE;
}

static void close_button_setup(GtkWidget *menu_box)
{
#ifdef CONFIG_DISPLAY_CLOSE_BUTTON
	GtkWidget *quit_b, *close_sep;

	/* seperator */
	close_sep = gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(menu_box), close_sep, FALSE, FALSE, 0);
	gtk_widget_show(close_sep);

	/* close button */
	quit_b = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(quit_b), "clicked",
		      G_CALLBACK(delete_event), NULL);
	gtk_box_pack_start(GTK_BOX(menu_box), quit_b, TRUE, TRUE, 0);
	gtk_widget_show(quit_b);
#endif
}

static void window_setup(void)
{
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_title(GTK_WINDOW(window), PROGRAM_NAME);

	g_signal_connect(G_OBJECT(window), "delete_event",
		      G_CALLBACK(delete_event), NULL);

	gtk_container_set_border_width(GTK_CONTAINER(window), 15);

	gtk_window_set_default_size(GTK_WINDOW(window), DEFAULT_XRES,
		DEFAULT_YRES);

#ifdef CONFIG_FULL_SCREEN
	gtk_window_fullscreen(GTK_WINDOW(window));
#endif
}

int main(int argc, char *argv[])
{
	int ret, i;
	char *markup;
	GtkWidget *title;
	GtkWidget *inner_align;
	GtkWidget *inner_box, *vbox, *menu_box;
	GtkWidget *menu_sep;

	gtk_init(&argc, &argv);

	window_setup();

	/* setup main vertical box */
	vbox = gtk_vbox_new(FALSE, 15);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show(vbox);

	/* setup menu box */
	menu_box = gtk_hbox_new(FALSE, 15);
	gtk_widget_set_size_request(menu_box, -1, 60);
	gtk_box_pack_end(GTK_BOX(vbox), menu_box, FALSE, FALSE, 0);
	gtk_widget_show(menu_box);

	/* setup menu box separator */
	menu_sep = gtk_hseparator_new();
	gtk_box_pack_end(GTK_BOX(vbox), menu_sep, FALSE, FALSE, 0);
	gtk_widget_show(menu_sep);

	/* setup all tasks */

	for (i = 0; i < ARRAY_SIZE(tasks); i++) {

		/* menu button */
		tasks[i].button =
			gtk_toggle_button_new_with_label(tasks[i].name);
		gtk_box_pack_start(GTK_BOX(menu_box), tasks[i].button, TRUE,
			TRUE, 0);
		g_signal_connect(G_OBJECT(tasks[i].button), "pressed",
			G_CALLBACK(menu_callback), (gpointer)&tasks[i]);
		gtk_widget_show(tasks[i].button);

		/* outer box */
    		tasks[i].box = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), tasks[i].box, TRUE, TRUE, 10);

		/* title */
		title = gtk_label_new(NULL);
		markup = g_markup_printf_escaped(
			"<span size=\"20000\">%s</span>",
			tasks[i].name_long);
		gtk_label_set_markup(GTK_LABEL(title), markup);
		gtk_box_pack_start(GTK_BOX(tasks[i].box),title, FALSE, FALSE,
			0);
		gtk_widget_show(title);

		/* alignment for inner box */
		inner_align = gtk_alignment_new(0.5, 0.5, 1, 0);
		gtk_container_set_border_width(GTK_CONTAINER(inner_align), 20);
		gtk_box_pack_start(GTK_BOX(tasks[i].box), inner_align, TRUE,
			TRUE, 0);
		gtk_widget_show(inner_align);

		/* inner box */
    		inner_box = gtk_vbox_new(FALSE, 0);
		gtk_container_add(GTK_CONTAINER(inner_align), inner_box);
		gtk_widget_show(inner_box);

		/* box setup */
		if (tasks[i].setup) {
			ret = tasks[i].setup(&tasks[i], inner_box);
			if (ret == 0)
				tasks[i].usable = 1;
			else {
				error_msg("Setup Error",
					"An error occured while setting up %s",
					tasks[i].name_long);
			}
		}

	}

	close_button_setup(menu_box);

	/* display first function */
	task_activate(&tasks[TASK_LED]);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tasks[TASK_LED].button),
		TRUE);

	gtk_widget_show(window);

	gtk_main();

	return 0;
}

