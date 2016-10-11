#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include <cairo.h>

#include "common.h"

#define LED_BG_IMG RESOURCE_DIR "a440.png"
#define LED_R_G_IMG RESOURCE_DIR "led_r_g.png"
#define LED_R_x_IMG RESOURCE_DIR "led_r_x.png"
#define LED_x_G_IMG RESOURCE_DIR "led_x_g.png"
#define LED_x_x_IMG RESOURCE_DIR "led_x_x.png"

static int led_state[2];

struct my_toggle_button {
	gchar *label;
	gint id;
	gpointer data;
};

static struct my_toggle_button led_rd = {
	.label = "Red",
	.id = 0,
};

static struct my_toggle_button led_gr = {
	.label = "Green",
	.id = 1,
};

static void led_control(int id, int mode)
{
#if defined(__arm__)
	int fd;
	char *path = "/sys/class/leds/";
	char file[512];

	sprintf(file, "%s%s/trigger", path, id ? "green" : "red");
	fd = open(file, O_WRONLY);
	if (fd < 0)
		return;
	write(fd, "default-on", 10);
	close(fd);

	sprintf(file, "%s%s/brightness", path, id ? "green" : "red");
	fd = open(file, O_WRONLY);
	if (fd < 0)
		return;

	if (mode)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);

	close(fd);
#endif
}

static void led_toggle_callback(GtkWidget *widget, gpointer data)
{
	struct my_toggle_button *toggle = (struct my_toggle_button *)data;
	GtkWidget *draw_area = toggle->data;

	if (led_state[toggle->id])
		led_state[toggle->id] = 0;
	else
		led_state[toggle->id] = 1;

	led_control(toggle->id, led_state[toggle->id]);

	gdk_window_invalidate_rect(draw_area->window, NULL, FALSE);
}

static GtkWidget *led_toggle_button(struct my_toggle_button *toggle,
				    gpointer data)
{
	GtkWidget *button;

	button = gtk_toggle_button_new_with_label(toggle->label);
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   G_CALLBACK(led_toggle_callback), (gpointer)toggle);
	gtk_widget_set_size_request(button, DEFAULT_BUTTON_WIDTH, -1);

	toggle->data = data;

	return button;
}

static gboolean led_expose_event(GtkWidget *widget, GdkEventExpose *event,
				 gpointer data)
{
	static int init = 0;
	static cairo_surface_t *bg, *img[4];
	cairo_surface_t *image;
	cairo_t *cr;

	if (init == 0) {
		bg = cairo_image_surface_create_from_png(LED_BG_IMG);
		img[0] = cairo_image_surface_create_from_png(LED_x_x_IMG);
		img[1] = cairo_image_surface_create_from_png(LED_x_G_IMG);
		img[2] = cairo_image_surface_create_from_png(LED_R_x_IMG);
		img[3] = cairo_image_surface_create_from_png(LED_R_G_IMG);
		init++;
	}

	cr = gdk_cairo_create(widget->window);

	cairo_set_source_surface(cr, bg, 0, 0);
	cairo_paint(cr);

	image = img[(led_state[0] << 1) | led_state[1]];
	cairo_set_source_surface(cr, image, 230, 120);
	cairo_paint(cr);

	cairo_destroy(cr);

	return TRUE;
}

int task_led_setup(struct task *task, GtkWidget *box)
{
	GtkWidget *main_box, *draw_area, *ctrl_box;
	GtkWidget *bt_led_rd, *bt_led_gr;

	main_box = gtk_vbox_new(FALSE, 0);
	draw_area = gtk_drawing_area_new();
	ctrl_box = gtk_hbox_new(FALSE, 0);

	gtk_box_pack_start(GTK_BOX(box), main_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_box), draw_area, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(main_box), ctrl_box, FALSE, TRUE, 0);

	g_signal_connect(G_OBJECT(draw_area), "expose-event",
			 G_CALLBACK(led_expose_event), NULL);

	/* setup toggle button: LED(red), LED(green) */
	bt_led_rd = led_toggle_button(&led_rd, draw_area);
	bt_led_gr = led_toggle_button(&led_gr, draw_area);
	gtk_box_pack_start(GTK_BOX(ctrl_box), bt_led_rd, TRUE, FALSE, 3);
	gtk_box_pack_start(GTK_BOX(ctrl_box), bt_led_gr, TRUE, FALSE, 3);

	led_control(0, 0);
	led_control(1, 1);

	gtk_widget_show_all(main_box);

	return 0;
}
