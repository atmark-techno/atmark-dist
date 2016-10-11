#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/file.h>
#include <linux/iio/events.h>

#define IIO_DEVICE	"iio:device"
#define IIO_DIR		"/sys/bus/iio/devices/"
#define DEV_DIR		"/dev/"

#define DEV_NAME	"3-0054"
static int dev_num;

#define ADC_VOLTAGE_MIN	0
#define ADC_VOLTAGE_MAX	255	/* register value */

#define VOLTAGE_MIN	0	/* reg_to_vin(ADC_VOLTAGE_MIN) */
#define VOLTAGE_MAX	20223	/* reg_to_vin(ADC_VOLTAGE_MAX) */

#define ADC_VOLT_LEN_MAX	(4) /* strlen(stringify(ADC_VOLTAGE_MAX)) + 1 */

static char adc_rising_value[ADC_VOLT_LEN_MAX];
static char adc_falling_value[ADC_VOLT_LEN_MAX];

#define LOCKFILE 	"/var/run/vintrigger.lock"
static int lock = -1;

static int terminated;

#define __stringify_1(x)	#x
#define stringify(x)		__stringify_1(x)

#define UL_BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
#define __must_be_array(a) \
	UL_BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(__typeof__(a), __typeof__(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(divisor) __divisor = divisor;		\
	(((x) + ((__divisor) / 2)) / (__divisor));	\
}							\
)

#define __unused __attribute__((unused))

#define logging_init()							\
({									\
	openlog("vintrigger", LOG_PID | LOG_NOWAIT, LOG_DAEMON);	\
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


#define ALT_OVER	(1<<0)
#define ALT_UNDER	(1<<1)
#define ALT_BOTH	(ALT_OVER | ALT_UNDER)

static int alteration;
static char *rising_thresh;
static char *falling_thresh;

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
		if (terminated)
			return -1;
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

/*
 *  VIN
 *   |
 *   R1(200k)
 *   |
 *   |--VOUT(to ADC081C)
 *   |
 *   R2(39k)
 *   |
 *  GND
 */
#define R1	200
#define R2	39

static int vin_to_reg(int vin)
{
	int vout = DIV_ROUND_CLOSEST(R2 * vin, R1 + R2);
	return DIV_ROUND_CLOSEST(vout * 256, 3300);
}

static int __unused reg_to_vin(int vout)
{
	int vin = DIV_ROUND_CLOSEST((R1 + R2) * vout, R2);
	return DIV_ROUND_CLOSEST(vin * 3300, 256);
}

static int is_valid_voltage(char *arg)
{
	long volt;
	int ret;

	ret = xstrtol(arg, &volt);
	if (ret)
		return ret;

	if ((volt < VOLTAGE_MIN) || (VOLTAGE_MAX < volt))
		return -1;

	return 0;
}

#define IIO_MAX_NAME_LENGTH 30
static int find_device_number(void)
{
	const struct dirent *ent;
	FILE *fp;
	DIR *dp;
	int num;
	char path[MAXPATHLEN];
	char name[IIO_MAX_NAME_LENGTH];

	dp = opendir(IIO_DIR);
	if (dp == NULL)
		return -1;

	for (ent = readdir(dp); ent != NULL; ent = readdir(dp)) {
		if (ent->d_name[0] == '.')
			continue;
		if (strncmp(ent->d_name, IIO_DEVICE, strlen(IIO_DEVICE)))
			continue;

		sscanf(ent->d_name, IIO_DEVICE"%d", &num);
		sprintf(path, "%s%s%d/name", IIO_DIR, IIO_DEVICE, num);
		fp = fopen(path, "r");
		if (!fp)
			continue;
		fscanf(fp, "%s", name);

		fclose(fp);
		if (!strcmp(DEV_NAME, name)) {
			closedir(dp);
			return num;
		}
	}
	closedir(dp);

	return -1;
}

static void write_adc_event(const char *file, const char *val, size_t count, int exit_on_err)
{
	char path[MAXPATHLEN];
        int fd;
	int ret;

	snprintf(path, sizeof(path), "%s%s%d/events/%s",
		 IIO_DIR, IIO_DEVICE, dev_num, file);
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

static void read_adc_event(const char *file, char *val, size_t count, int exit_on_err)
{
	char path[MAXPATHLEN];
	char *c;
	int fd;
	int ret;

	snprintf(path, sizeof(path), "%s%s%d/events/%s",
		 IIO_DIR, IIO_DEVICE, dev_num, file);
	fd = open(path, O_RDWR);
	if (fd < 0) {
		log_error(exit_on_err, "cannot open %s", path);
		return;
	}

	ret = xread(fd, val, count);
	if (ret < 0)
		log_error(exit_on_err, "cannot read %s", path);

	close(fd);

	c = strchr(val, '\n');
	if (c)
		*c = '\0';
}

static void restore_adc(void)
{
	write_adc_event("in_voltage_thresh_either_en", "0", 1, 1);

	if (strlen(adc_rising_value))
		write_adc_event("in_voltage_thresh_rising_value",
			       adc_rising_value, strlen(adc_rising_value), 0);

	if (strlen(adc_falling_value))
		write_adc_event("in_voltage_thresh_falling_value",
			       adc_falling_value, strlen(adc_falling_value), 0);
}

static void backup_adc(void)
{
	read_adc_event("in_voltage_thresh_rising_value",
		       adc_rising_value, ADC_VOLT_LEN_MAX, 1);
	read_adc_event("in_voltage_thresh_falling_value",
		       adc_falling_value, ADC_VOLT_LEN_MAX, 1);
}

static void setup_adc(void)
{
	long thresh;
	char hthresh[ADC_VOLT_LEN_MAX];
	char lthresh[ADC_VOLT_LEN_MAX];

	if (alteration & ALT_OVER) {
		xstrtol(rising_thresh, &thresh);
		snprintf(hthresh, sizeof(hthresh), "%d", vin_to_reg(thresh));
	}
	if (alteration & ALT_UNDER) {
		xstrtol(falling_thresh, &thresh);
		snprintf(lthresh, sizeof(lthresh), "%d", vin_to_reg(thresh));
	}

	switch (alteration) {
	case ALT_OVER:
		write_adc_event("in_voltage_thresh_rising_value",
			       hthresh, strlen(hthresh), 1);
		write_adc_event("in_voltage_thresh_falling_value",
			       stringify(ADC_VOLTAGE_MIN), ADC_VOLT_LEN_MAX, 1);
		break;
	case ALT_UNDER:
		write_adc_event("in_voltage_thresh_rising_value",
			       stringify(ADC_VOLTAGE_MAX), ADC_VOLT_LEN_MAX, 1);
		write_adc_event("in_voltage_thresh_falling_value",
			       lthresh, strlen(lthresh), 1);
		break;
	case ALT_BOTH:
		write_adc_event("in_voltage_thresh_rising_value",
			       hthresh, strlen(hthresh), 1);
		write_adc_event("in_voltage_thresh_falling_value",
			       lthresh, strlen(lthresh), 1);
		break;
	default:
		log_error(1, "unsupported alteration 0x%x", alteration);
	}

	write_adc_event("in_voltage_thresh_either_en", "1", 1, 1);
}

static void wait_interrupt(void)
{
	char path[MAXPATHLEN];
	int fd, event_fd;
	struct iio_event_data event;
	char val;
	int ret;

	snprintf(path, sizeof(path), "%s%s%d", DEV_DIR, IIO_DEVICE, dev_num);
	fd = open(path, 0);
	if (fd < 0)
		log_error(1, "cannot open %s", path);

	ret = ioctl(fd, IIO_GET_EVENT_FD_IOCTL, &event_fd);
	if (ret < 0 || event_fd < 0)
		log_error(1, "cannot get event fd");

	close(fd);

	setup_adc();
	while (1) {
		enum iio_chan_type type;
		enum iio_event_type ev_type;

		ret = xread(event_fd, &event, sizeof(event));
		if (ret < 0) {
			if (errno == EAGAIN)
				continue;
			else
				log_error(1, "cannot read event fd");
		}
		type = IIO_EVENT_CODE_EXTRACT_CHAN_TYPE(event.id);
		ev_type = IIO_EVENT_CODE_EXTRACT_TYPE(event.id);

		if ((type == IIO_VOLTAGE) && (ev_type == IIO_EV_TYPE_THRESH))
			break;
	}

out:
	close(event_fd);
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
	fprintf(stderr, "Usage: vintrigger -o|-u VOLTAGE COMMAND [ARGS]\n"
		"Options:\n"
		"  -o, --over=VOLTAGE\n"
		"      Execute the program COMMAND when the detected voltage is equal\n"
		"      to or over the VOLTAGE.\n"
		"  -u, --under=VOLTAGE\n"
		"      Execute the program COMMAND when the detected voltage is equal\n"
		"      to or unver the VOLTAGE.\n"
		"VOLTAGE: Range: %d - %d\n", VOLTAGE_MIN, VOLTAGE_MAX);
}

static int parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"over",required_argument, NULL, 'o' },
		{"under",required_argument, NULL, 'u' },
		{0,0,0,0},
	};
	int c;
	int ret;

	while ((c = getopt_long(argc, argv, "+o:u:",longopts, NULL)) != EOF) {
		switch (c) {
		case 'o':
			alteration |= ALT_OVER;
			ret = is_valid_voltage(optarg);
			if (ret)
				return -1;
			rising_thresh = optarg;
			break;
		case 'u':
			alteration |= ALT_UNDER;
			ret = is_valid_voltage(optarg);
			if (ret)
				return -1;
			falling_thresh = optarg;
			break;
		default:
			return -1;
		}
	}

	if (!alteration)
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
			fprintf(stderr, "vintrigger is already running\n");
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

	dev_num = find_device_number();
	if (dev_num < 0) {
		fprintf(stderr, "cannot find %s\n", DEV_NAME);
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

	backup_adc();

	atexit(restore_adc);

	switch (alteration) {
	case ALT_OVER:
		log_message("waiting for an over range alert (%s mV).", rising_thresh);
		break;
	case ALT_UNDER:
		log_message("waiting for an under range alert (%s mV).", falling_thresh);
		break;
	case ALT_BOTH:
		log_message("waiting for an under range alert or a over range alert (%s-%s mV).",
			    falling_thresh, rising_thresh);
		break;
	default:
		log_error(1, "unsupported alteration 0x%x", alteration);
	}
	wait_interrupt();
	log_message("exceeded the limit. executing command.");

	exec_cmd();

	remove_exclusive_lock();
	logging_close();

	return EXIT_SUCCESS;
}
