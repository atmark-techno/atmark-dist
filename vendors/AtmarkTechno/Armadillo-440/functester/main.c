#include <gtk/gtk.h>

#include "common.h"

/* build-time configuartion */
//#define CONFIG_DISPLAY_CLOSE_BUTTON
//#define CONFIG_FULL_SCREEN

#define PROGRAM_NAME "Armadillo-440 Function Tester"
#define DEFAULT_XRES (480)
#define DEFAULT_YRES (272)

GtkWidget *window;

/* max error message detail length: 512 */
void error_msg(gchar *err_brief, gchar *err_detail_fmt, ...)
{
	va_list ap;
	char fmt_buf[512];

	va_start(ap, err_detail_fmt);

	vsnprintf(fmt_buf, sizeof(fmt_buf)-1, err_detail_fmt, ap);

	GtkWidget* dialog =
		gtk_message_dialog_new_with_markup(
			GTK_WINDOW(window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			"<span size=\"20000\">%s</span>",
			err_brief);

	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
						 "%s", fmt_buf);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error");
	gtk_dialog_run(GTK_DIALOG (dialog));

	gtk_widget_destroy(dialog);

	va_end(ap);
}

extern int task_led_setup(struct task *task, GtkWidget *box);
extern int task_button_setup(struct task *task, GtkWidget *box);
extern int task_touchscreen_setup(struct task *task, GtkWidget *box);
extern int task_backlight_setup(struct task *task, GtkWidget *box);
extern int task_sound_setup(struct task *task, GtkWidget *box);
extern int task_rtc_setup(struct task *task, GtkWidget *box);

#define DEFAULT_ACTIVATE_TASK 0

struct task tasks[] = {
	{
		.name		= "LED",
		.name_long	= "LED Control",
		.setup		= task_led_setup,
	},
	{
		.name		= "Button",
		.name_long	= "Button Monitor",
		.setup		= task_button_setup,
	},
	{
		.name		= "Touch Screen",
		.name_long	= "Touch Screen Test",
		.setup		= task_touchscreen_setup,
	},
	{
		.name		= "Backlight",
		.name_long	= "Backlight Control",
		.setup		= task_backlight_setup,
	},
	{
		.name		= "Sound",
		.name_long	= "Sound Test",
		.setup		= task_sound_setup,
	},
	{
		.name		= "RTC",
		.name_long	= "Real Time Clock Test",
		.setup		= task_rtc_setup,
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

	gtk_container_set_border_width(GTK_CONTAINER(window), 5);

	gtk_window_set_default_size(GTK_WINDOW(window),
				    DEFAULT_XRES, DEFAULT_YRES);

#if !defined(__i386__) && defined(CONFIG_FULL_SCREEN)
	gtk_window_fullscreen(GTK_WINDOW(window));
#endif
}

int main(int argc, char *argv[])
{
	GtkWidget *main_box, *menu_box, *task_box;
	GtkWidget *menu_sep;
	GtkWidget *title;
	char *markup;
	int ret, i;

	gtk_init(&argc, &argv);

	window_setup();

	/* setup main box */
	main_box = gtk_hbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(window), main_box);
	gtk_widget_show(main_box);

	/* setup menu box */
	menu_box = gtk_vbox_new(FALSE, 0);
	gtk_widget_set_size_request(menu_box, 100, -1);
	gtk_box_pack_start(GTK_BOX(main_box), menu_box, FALSE, FALSE, 0);
	gtk_widget_show(menu_box);

	/* setup menu box separator */
	menu_sep = gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(main_box), menu_sep, FALSE, FALSE, 2);
	gtk_widget_show(menu_sep);

	/* setup all tasks */

	for (i = 0; i < ARRAY_SIZE(tasks); i++) {
		/* menu button */
		tasks[i].button =
			gtk_toggle_button_new_with_label(tasks[i].name);
		gtk_box_pack_start(GTK_BOX(menu_box), tasks[i].button, FALSE,
				   FALSE, 5);
		g_signal_connect(G_OBJECT(tasks[i].button), "pressed",
				 G_CALLBACK(menu_callback),
				 (gpointer)&tasks[i]);
		gtk_widget_show(tasks[i].button);

		/* outer box */
		tasks[i].box = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(main_box), tasks[i].box,
				   TRUE, TRUE, 10);

		/* title */
		title = gtk_label_new(NULL);
		markup = g_markup_printf_escaped(
			"<span size=\"20000\">%s</span>",
			tasks[i].name_long);
		gtk_label_set_markup(GTK_LABEL(title), markup);
		gtk_box_pack_start(GTK_BOX(tasks[i].box),title,
				   FALSE, FALSE, 0);
		gtk_widget_show(title);

		/* task box */
		task_box = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(tasks[i].box), task_box,
				   TRUE, TRUE, 0);
		gtk_widget_show(task_box);

		/* box setup */
		if (tasks[i].setup) {
			ret = tasks[i].setup(&tasks[i], task_box);
			if (ret == 0)
				tasks[i].usable = 1;
			else {
				error_msg("Setup Error",
					  "An error occured while setting up "
					  "%s", tasks[i].name_long);
			}
		}
	}

	close_button_setup(menu_box);

	/* display first function */
	task_activate(&tasks[DEFAULT_ACTIVATE_TASK]);
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(tasks[DEFAULT_ACTIVATE_TASK].button), TRUE);

	gtk_widget_show(window);

	gtk_main();

	return 0;
}
