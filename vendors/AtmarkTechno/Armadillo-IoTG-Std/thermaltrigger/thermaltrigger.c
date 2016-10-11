#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/file.h>

#define GPIO_PATH	"/sys/class/gpio/TEMP_ALERT_N/"
#define LM75B_PATH	"/sys/devices/platform/i2c-gpio.3/i2c-3/3-0048/"

#define LM75B_VALUE_MIN	-55000
#define LM75B_VALUE_MAX	125000

#define LOCKFILE 	"/var/run/thermaltrigger.lock"
static int lock = -1;

static int terminated;

#define __stringify_1(x)	#x
#define stringify(x)		__stringify_1(x)

#define UL_BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define __must_be_array(a) \
	UL_BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(__typeof__(a), __typeof__(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#define __unused __attribute__((unused))

#define logging_init()							\
({									\
	openlog("thermaltrigger", LOG_PID | LOG_NOWAIT, LOG_DAEMON);	\
})

#define logging_close()	\
({			\
	closelog();	\
})

#define log_error(doexit, ...)		\
({					\
	syslog(LOG_ERR, __VA_ARGS__);	\
	logging_close();		\
	if (doexit)			\
		exit(EXIT_FAILURE);	\
})

#define log_message( ...)		\
({					\
	syslog(LOG_INFO, __VA_ARGS__);	\
})

enum {
	ALT_UNKNOWN,
	ALT_ABOVE,
	ALT_BELOW,
};
static int alteration = ALT_UNKNOWN;
static char *threshold;

static char **params;

static ssize_t _xwrite(int fd, const void *buf, size_t count)
{
	ssize_t ret;

	do {
		ret = write(fd, buf, count);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

ssize_t xwrite(int fd, const void *buf, size_t count)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (count) {
		cc = _xwrite(fd, buf, count);

		if (cc < 0) {
			if (total)
				return total;
			return cc;
		}

		total += cc;
		buf = ((const char *)buf) + cc;
		count -= cc;
	}

	return total;
}

static ssize_t _xread(int fd, void *buf, size_t count)
{
	ssize_t ret;

	do {
		ret = read(fd, buf, count);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

static ssize_t xread(int fd, void *buf, size_t count)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (count) {
		cc = _xread(fd, buf, count);

		if (cc < 0) {
			if (total)
				return total;
			return cc;
		}
		if (cc == 0)
			break;
		buf = ((char *)buf) + cc;
		total += cc;
		count -= cc;
	}

	return total;
}


static int xpoll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	int ret;

	do {
		if (terminated)
			return -1;
		ret = poll(fds, nfds, timeout);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

static int xflock(int fd, int operation)
{
	int ret;

	do {
		ret = flock(fd, operation);
	} while (ret < 0 && errno == EINTR);

	return ret;
}

static int xstrtol(const char *str, long *val)
{
	const char *ptr;
	char *endptr;

	ptr = str;

	while (*ptr != '\0') {
		if (isspace((int)*ptr)) {
			ptr++;
			continue;
		}
		break;
	}

	if (*ptr == '\0')
		return -1;

	errno = 0;
	*val = strtol(ptr, &endptr, 0);

	if (errno || (*endptr == *ptr))
		return -1;

	return 0;
}

static void set_signal_edge(void)
{
        int fd;
	int ret;

        fd = open(GPIO_PATH "edge", O_RDWR);
	if (fd < 0)
		log_error(1, "cannot open %s", GPIO_PATH "edge");

	if (alteration == ALT_ABOVE)
		ret = xwrite(fd, "falling", 7);
	else
		ret = xwrite(fd, "rising", 6);
	if (ret < 0)
		log_error(1, "cannot setting signal edge");

        close(fd);
}

static void wait_interrupt(void)
{
        int fd;
	struct pollfd pfd;
	char val;
	int ret;

	fd = open(GPIO_PATH "value", O_RDONLY);
	if (fd < 0)
		log_error(1, "cannot open %s", GPIO_PATH "value");

	ret = xread(fd, &val, 1);
	if (ret < 0)
		log_error(1, "cannot read %s", GPIO_PATH "value");

	pfd.fd = fd;
	pfd.events = POLLPRI | POLLERR;
	pfd.revents = 0;

	ret = xpoll(&pfd, 1, -1);
	if (ret < 0)
		log_error(1, "cannot poll %s", GPIO_PATH "value");

	close(fd);
}

static int is_valid_temp(char *arg)
{
	long temp;
	int ret;

	ret = xstrtol(arg, &temp);
	if (ret)
		return ret;

	if ((temp < LM75B_VALUE_MIN) || (LM75B_VALUE_MAX < temp))
		return -1;

	return 0;
}

#define TEMP_LEN_MAX	(7) /* strlen(stringify(LM75B_VALUE_MAX)) + 1 */
static char temp1_max[TEMP_LEN_MAX];
static char temp1_max_hyst[TEMP_LEN_MAX];

static void write_temp_file(const char *file, const char *val, size_t count, int exit_on_err)
{
	char path[MAXPATHLEN];
        int fd;
	int ret;

	snprintf(path, sizeof(path), "%s%s", LM75B_PATH, file);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		log_error(exit_on_err, "cannot open %s", path);
		return;
	}

	ret = xwrite(fd, val, count);
	if (ret < 0) {
		log_error(exit_on_err, "cannot write %s", path);
		return;
	}

	close(fd);
}

static void read_temp_file(const char *file, char *val, size_t count, int exit_on_err)
{
	char path[MAXPATHLEN];
        int fd;
	int ret;

	snprintf(path, sizeof(path), "%s%s", LM75B_PATH, file);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		log_error(exit_on_err, "cannot open %s", path);
		return;
	}

	ret = xread(fd, val, count);
	if (ret < 0)
		log_error(exit_on_err, "cannot read %s", path);

	close(fd);
}

static void restore_sensor(void)
{
	if (strlen(temp1_max))
		write_temp_file("temp1_max", temp1_max, strlen(temp1_max), 0);

	if (strlen(temp1_max_hyst))
		write_temp_file("temp1_max_hyst", temp1_max_hyst, strlen(temp1_max_hyst), 0);
}

static void backup_sensor(void)
{
	read_temp_file("temp1_max", temp1_max, TEMP_LEN_MAX, 1);
	read_temp_file("temp1_max_hyst", temp1_max_hyst, TEMP_LEN_MAX, 1);
}

static void setup_sensor(void)
{
	if (alteration == ALT_ABOVE) {
		write_temp_file("temp1_max_hyst", stringify(LM75B_VALUE_MAX), TEMP_LEN_MAX, 1);
		write_temp_file("temp1_max", threshold, strlen(threshold), 1);
	} else {
		write_temp_file("temp1_max", stringify(LM75B_VALUE_MIN), TEMP_LEN_MAX, 1);
		write_temp_file("temp1_max_hyst", threshold, strlen(threshold), 1);
	}
}

static void term_handler(__unused int signum)
{
	terminated = 1;
}

static int setup_signal(void)
{
	int term_signals[] = {
		SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTERM
	};
	struct sigaction sa;
	unsigned int i;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = term_handler;

	for(i = 0; i < ARRAY_SIZE(term_signals); i++) {
		if (sigaction(term_signals[i], &sa, NULL) < 0) {
			fprintf(stderr, "cannot set signal handler\n");
			return -1;
		}
	}

	return 0;
}

static void exec_cmd(void)
{
	int pid;
	int status;

	if (!params)
		return;

	pid = fork();
	if (pid < 0)
		log_error(1, "cannot fork");

	if (!pid) {
		execvp(params[0], params);
		log_error(1, "execvp failure");
	} else
		waitpid(pid, &status, 0);
}

static void usage(void)
{
	fprintf(stderr, "Usage: thermaltrigger -a|-b THRESHOLD COMMAND [ARGS]\n"
		"Options:\n"
		"  -a, --above=THRESHOLD\n"
		"      Execute the program COMMAND when the detected temperature is equal\n"
		"      to or above the THRESHOLD.\n"
		"  -b, --below=THRESHOLD\n"
		"      Execute the program COMMAND when the detected temperature is equal\n"
		"      to or below the THRESHOLD.\n"
		"TEMPERATURE: Range: %d - %d\n", LM75B_VALUE_MIN, LM75B_VALUE_MAX);
}

static int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"above",required_argument, NULL, 'a' },
		{"below",required_argument, NULL, 'b' },
		{0,0,0,0},
	};
	int c;
	int ret;

	while ((c = getopt_long(argc, argv, "+a:b:",longopts, NULL)) != EOF) {
		switch (c) {
		case 'a':
			alteration = ALT_ABOVE;
			ret = is_valid_temp(optarg);
			if (ret)
				return -1;
			threshold = optarg;
			break;
		case 'b':
			alteration = ALT_BELOW;
			ret = is_valid_temp(optarg);
			if (ret)
				return -1;
			threshold = optarg;
			break;
		default:
			return -1;
		}
	}

	if (alteration == ALT_UNKNOWN)
		return -1;

	if (optind < argc)
		params = &argv[optind];

	return 0;
}

static int place_exclusive_lock(void)
{
	int ret;

	lock = open(LOCKFILE, O_RDONLY|O_CREAT|O_CLOEXEC,
		  S_IWUSR|S_IRUSR|S_IRGRP|S_IROTH);
	if (lock < 0)
		fprintf(stderr, "cannot open %s\n", LOCKFILE);

	ret = xflock(lock, LOCK_EX | LOCK_NB);
	if (ret < 0) {
		if (errno == EWOULDBLOCK)
			fprintf(stderr, "thermaltrigger is already running\n");
		else
			fprintf(stderr, "cannot flock %s\n", LOCKFILE);
	}

	return ret;
}

static void remove_exclusive_lock(void)
{
	if (lock < 0)
		return;

	xflock(lock, LOCK_UN | LOCK_NB);
	close(lock);
	unlink(LOCKFILE);
}

int main(int argc, char *argv[])
{
	int ret;

	ret = parse_args(argc, argv);
	if (ret) {
		usage();
		return EXIT_FAILURE;
	}

	atexit(remove_exclusive_lock);
	ret = place_exclusive_lock();
	if (ret)
		return EXIT_FAILURE;

	ret = setup_signal();
	if (ret)
		return EXIT_FAILURE;

	logging_init();

	backup_sensor();

	atexit(restore_sensor);
	setup_sensor();

	set_signal_edge();

	log_message("Waiting for %s millidegrees Celsius or %s.",
		    threshold, (alteration == ALT_ABOVE) ? "above" : "below");
	wait_interrupt();
	log_message("exceeded the threshold. executing command.");

	exec_cmd();

	restore_sensor();
	remove_exclusive_lock();
	logging_close();

	return EXIT_SUCCESS;
}
