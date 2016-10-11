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

#define INPUT_DIR_PATH "/dev/input"
#define SW1_DEVICE_NAME_OLD "Tact-SW Port1"
#define SW2_DEVICE_NAME_OLD "Tact-SW Port2"
#define SW1_DEVICE_NAME_NEW "tactsw1"
#define SW2_DEVICE_NAME_NEW "tactsw2"

static int
check_input_device_name(char *name, char *path)
{
	char buf[256];
	int fd;
	int ret;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -1;

	ret = ioctl(fd, EVIOCGNAME(256), buf);
	close(fd);

	if (ret != -1)
		ret = strncmp(buf, name, strlen(name));

	return ret;
}

static int
get_input_device(char *name, char *path, int size)
{
	DIR *dir;
	struct dirent *entry;
	int ret = -1;

	dir = opendir(INPUT_DIR_PATH);
	if (dir == NULL) {
		perror("opendir");
		return -1;
	}

	while ((entry = readdir(dir))) {
		snprintf(path, size, INPUT_DIR_PATH "/%s", entry->d_name);
		ret = check_input_device_name(name, path);
		if (ret == 0)
			break;
		path[0] = '\0';
	}

	closedir(dir);
	return ret;
}

void
usage(void)
{
	printf("Usage: %s SWITCH LOOP [COMMAND]... \n", EXEC_NAME);
}

int main(int argc, char **argv)
{
	struct input_event event;
	char device[PATH_MAX];
	char *event_dev;
	char cmd[8096], *cmd_ptr;
	int fd;
	int ret;
	int i;
	int loop;

	if (argc < 3) {
		usage();
		return -1;
	}

	if (strcmp("sw1", argv[1]) == 0) {
		ret = get_input_device(SW1_DEVICE_NAME_NEW, device, PATH_MAX);
		if (ret)
			ret = get_input_device(SW1_DEVICE_NAME_OLD,
					       device, PATH_MAX);
		if (ret) {
			fprintf(stderr, "no such input device(`%s'or`%s').\n",
				SW1_DEVICE_NAME_NEW, SW1_DEVICE_NAME_OLD);
			return ret;
		}
		event_dev = device;
	} else if (strcmp("sw2", argv[1]) == 0) {
		ret = get_input_device(SW2_DEVICE_NAME_NEW, device, PATH_MAX);
		if (ret)
			ret = get_input_device(SW2_DEVICE_NAME_OLD,
					       device, PATH_MAX);
		if (ret) {
			fprintf(stderr, "no such input device(`%s'or`%s').\n",
				SW2_DEVICE_NAME_NEW, SW2_DEVICE_NAME_OLD);
			return ret;
		}
		event_dev = device;
	} else {
		event_dev = argv[1];
	}

	loop = (int)strtol(argv[2], NULL, 0);
	if (loop < 0)
		return -1;

	fd = open(event_dev, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return fd;
	}

	cmd_ptr = cmd;
	for (i=3; i<argc; i++)
		cmd_ptr += sprintf(cmd_ptr, "%s ", argv[i]);

	for (i=0; i<loop || loop==0;) {
		ret = read(fd, &event, sizeof(event));
		if (ret != sizeof(event))
			return -1;

		DPRINT("Event-type: %d\n", event.type);
		DPRINT("Event-code: %d\n", event.code);
		DPRINT("Event-val : %d\n", event.value);

		if (event.type && !event.value) {
			system(cmd);
			i++;
		}
	}

	close(fd);
	return 0;
}
