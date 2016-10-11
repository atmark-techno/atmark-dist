#include <gtk/gtk.h>

#include "common.h"

static gboolean touchscreen_callback(GtkWidget *widget, GdkEventMotion *event)
{
	GdkGC *gc;
	PangoLayout *layout;
	char buf[64];

	sprintf(buf, "<span size=\"30000\">[%d, %d]</span>",
		(int)event->x, (int)event->y);
	gdk_window_clear(widget->window);
	gc = gdk_gc_new(widget->window);

	layout = gtk_widget_create_pango_layout(widget, "");
	pango_layout_set_markup(layout, buf, -1);
	gdk_draw_layout(widget->window, gc, 80, 80, layout);
	g_object_unref(layout);
	gdk_gc_destroy(gc);

	return TRUE;
}

int task_touchscreen_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *mainbox, *label;

	mainbox = gtk_frame_new("Area");
	label = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(box), mainbox);
	gtk_container_add(GTK_CONTAINER(mainbox), label);
	gtk_widget_show_all(box);

	g_signal_connect(G_OBJECT(label), "motion_notify_event",
			   (GtkSignalFunc)touchscreen_callback, label);
	gtk_widget_set_events(label, GDK_POINTER_MOTION_MASK);

	return 0;
}
