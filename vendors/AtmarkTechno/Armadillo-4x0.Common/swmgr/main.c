#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>

#if defined(DEBUG)
#define DPRINT(args...) printf(args)
#else
#define DPRINT(args...)
#endif

#if defined(_SC_ARG_MAX)
# if defined(ARG_MAX)
# undef ARG_MAX
# endif
# define ARG_MAX (sysconf(_SC_ARG_MAX))
#endif

#ifndef FALSE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

#define INPUT_DIR_PATH	"/dev/input"
#define EVENT_NAME	"gpio-keys"

struct _sw {
	char *name;
	unsigned short code;
	char *desc;
};

struct _sw sw[] = {
	{"sw0", KEY_ENTER, "SW1 on Armadillo-4x0 board"},
	{"sw1", KEY_BACK, "SW1 on LCD extension board"},
	{"sw2", KEY_MENU, "SW2 on LCD extension board"},
	{"sw3", KEY_HOME, "SW3 on LCD extension board"},
};

static int
is_gpio_keys(char *path)
{
	char buf[sizeof(EVENT_NAME)];
	int fd;
	int ret;

	fd = open(path, O_RDONLY);
	if (fd < 0){
		perror("open");
		return FALSE;
	}

	ret = ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
	close(fd);
	if (ret < 0)
		return FALSE;

	ret = strcmp(buf, EVENT_NAME);
	if (ret !=  0)
		return FALSE;

	return TRUE;
}

static int
get_input_device(char *path, size_t path_length)
{
	DIR *dir;
	struct dirent *entry;
	int gpio_keys_found = FALSE;

	dir = opendir(INPUT_DIR_PATH);
	if (dir == NULL) {
		perror("opendir");
		return -1;
	}

	while ((entry = readdir(dir))) {
		snprintf(path, path_length, INPUT_DIR_PATH "/%s", entry->d_name);
		gpio_keys_found = is_gpio_keys(path);
		if (gpio_keys_found)
			break;
	}

	closedir(dir);
	return (gpio_keys_found) ? 0 : -1;
}

static void
usage(const char *prg)
{
	unsigned int i;

	printf("Usage: %s SWITCH LOOP [COMMAND]... \n", prg);
	printf("SWITCH\n");
	for (i = 0; i < (sizeof(sw) / sizeof(sw[0])); i++)
		printf("\t%s: %s\n", sw[i].name, sw[i].desc);

}

int
main(int argc, char **argv)
{
	struct input_event event;
	char device[PATH_MAX];
	char *cmd, *cmd_ptr;
	int fd;
	unsigned int sw_index;
	int ret;
	int i;
	int loop;

	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}

	for (sw_index = 0; sw_index < (sizeof(sw) / sizeof(sw[0])); sw_index++)
		if (strcmp(argv[1], sw[sw_index].name) == 0)
			break;

	if (sw_index == (sizeof(sw) / sizeof(sw[0]))) {
		usage(argv[0]);
		return -1;
	}

	ret = get_input_device(device, sizeof(device));
	if (ret < 0) {
		fprintf(stderr, "input device not found.\n");
		return -1;
	}

	loop = strtol(argv[2], NULL, 0);
	if (loop < 0) {
		usage(argv[0]);
		return -1;
	}

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	if (argc > 3) {
		cmd = malloc(ARG_MAX);
		if (cmd == NULL) {
			perror("malloc");
			close(fd);
			return -1;
		}
	}

	cmd_ptr = cmd;
	for (i = 3; i < argc; i++)
		cmd_ptr += sprintf(cmd_ptr, "%s ", argv[i]);

	for (i = 0; i < loop || loop == 0;) {
		ret = read(fd, &event, sizeof(event));
		if (ret != sizeof(event)) {
			perror("read");
			close(fd);
			return -1;
		}

		DPRINT("Event-type: %d\n", event.type);
		DPRINT("Event-code: %d\n", event.code);
		DPRINT("Event-val : %d\n", event.value);

		if ((event.type == EV_KEY) &&
		    (event.code == sw[sw_index].code) &&
		    (event.value != 0)) {
			if (argc > 3)
				system(cmd);
			i++;
		}
	}
	free(cmd);
	close(fd);

	return 0;
}
