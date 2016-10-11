#ifndef __COMMON_H_
#define __COMMON_H_

#define RESOURCE_DIR "/usr/share/functester/"
#define TEMPORARY_DIR "/tmp/"

#define DEFAULT_BUTTON_WIDTH  (80)
#define DEFAULT_BUTTON_HEIGHT (40)

#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])

typedef void (*activate_callback)(gpointer *data);
typedef void (*deactivate_callback)(gpointer *data);

struct task {
	char *name;
	char *name_long;
	GtkWidget *button;
	GtkWidget *box;	
	int (*setup)(struct task *task, GtkWidget *box);
	activate_callback activate;
	gpointer *activate_data;
	deactivate_callback deactivate;
	gpointer *deactivate_data;
	int usable;
	int active;
};

extern GtkWidget *window;
extern void error_msg(gchar *err_brief, gchar *err_detail_fmt, ...);

#endif
