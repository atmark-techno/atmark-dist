#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <linux/input.h>

#if defined(DEBUG)
#define DPRINT(args...) printf(args)
#else
#define DPRINT(args...)
#endif

void
usage(void)
{
	printf("Usage: %s SWITCH LOOP [COMMAND]... \n", EXEC_NAME);
}

int main(int argc, char **argv)
{
	struct input_event event;
	char *event_dev = "/dev/input/event0";
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
		/* using default device */
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
