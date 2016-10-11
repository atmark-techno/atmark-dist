#include <gtk/gtk.h>
#include <cairo.h>
#include <gdk/gdkkeysyms.h>

#include "common.h"

#define BT_BG_IMG RESOURCE_DIR "a440_lcd.png"
#define BT_PUSH_IMG RESOURCE_DIR "push.png"

static void task_set_activate_callback(struct task *task,
				       activate_callback callback,
				       void *data)
{
	task->activate = callback;
	task->activate_data = data;
}

static void task_set_deactivate_callback(struct task *task,
					 deactivate_callback callback,
					 void *data)
{
	task->deactivate = callback;
	task->deactivate_data = data;
}

struct button_key {
	char *label;
	int key;
	int x, y;
	int state;
	GtkWidget *button;
};

static struct button_key button_keys[] = {
	{.label = "Enter", .key = GDK_Return, .x = 165, .y = 185, },

	/*For xfree86 keycodes*/
	{.label = "Home",  .key = GDK_Next, .x = 225, .y =  30, },
	{.label = "Menu",  .key = GDK_Menu, .x = 255, .y =  30, },
	{.label = "Back",  .key = GDK_Begin, .x = 285, .y =  30, },

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 18
	/*For evdev keycodes*/
	{.label = "Home",  .key = GDK_Home, .x = 225, .y =  30, },
	{.label = "Menu",  .key = GDK_MenuKB, .x = 255, .y =  30, },
	{.label = "Back",  .key = GDK_Back, .x = 285, .y =  30, },
#endif
};

static gboolean key_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GtkWidget *draw_area = data;
	int i;

	for (i = 0; i < ARRAY_SIZE(button_keys); i++) {
		if (event->keyval != button_keys[i].key)
			continue;
		if (event->type == GDK_KEY_PRESS)
			button_keys[i].state = 1;
		else
			button_keys[i].state = 0;
	}

	gdk_window_invalidate_rect(draw_area->window, NULL, FALSE);

	return TRUE;
}

static void button_activate(gpointer *data)
{
	g_signal_connect(G_OBJECT(window), "key-press-event",
			 G_CALLBACK(key_event), data);
	g_signal_connect(G_OBJECT(window), "key-release-event",
			 G_CALLBACK(key_event), data);
}

static void button_deactivate(gpointer *data)
{
	g_signal_handlers_disconnect_by_func(G_OBJECT(window),
					     G_CALLBACK(key_event), NULL);
}

static gboolean button_expose_event(GtkWidget *widget, GdkEventExpose *event,
				    gpointer data)
{
	static int init = 0;
	static cairo_surface_t *bg, *img;
	cairo_t *cr;
	int i;

	if (init == 0) {
		bg = cairo_image_surface_create_from_png(BT_BG_IMG);
		img = cairo_image_surface_create_from_png(BT_PUSH_IMG);
		init++;
	}

	cr = gdk_cairo_create(widget->window);

	cairo_set_source_surface(cr, bg, 0, 0);
	cairo_paint(cr);

	for (i=0; i<ARRAY_SIZE(button_keys); i++) {
#if !defined(__arm__)
		button_keys[i].state = 1;
#endif
		if (button_keys[i].state) {
			cairo_set_source_surface(cr, img,
						 button_keys[i].x,
						 button_keys[i].y);
			cairo_paint(cr);
		}
	}

	cairo_destroy(cr);

	return TRUE;
}

int task_button_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *draw_area;

	draw_area = gtk_drawing_area_new();
	gtk_box_pack_start(GTK_BOX(box), draw_area, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(draw_area), "expose-event",
			 G_CALLBACK(button_expose_event), NULL);

	task_set_activate_callback(task, button_activate, draw_area);
	task_set_deactivate_callback(task, button_deactivate, draw_area);

	gtk_widget_show_all(box);

	return 0;
}
