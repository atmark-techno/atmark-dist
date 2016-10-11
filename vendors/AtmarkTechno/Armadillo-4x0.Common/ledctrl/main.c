#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "common.h"

extern struct ledctrl_ops ledclass_ops;
extern struct ledctrl_ops legacy_ops;

struct ledctrl_info led_info[] = {
	{ LED_RED, "red", 0, 0, },
	{ LED_GREEN, "green", 0, 0, },
	{ LED_YELLOW, "yellow", 0, 0, },
	{ },
};

static void usage(int retval)
{
	FILE *output;
	int i;
	output = retval ? stderr : stdout;
	fprintf(output,
		"Usage: ledctrl ledname [command]\n"
		"Control LEDs.\n"
		"\n"
		"LED names:\n"
		"\n"
		"\tAvailable led names: ");
	for (i=0; led_info[i].id; i++)
		fprintf(output, "%s%s", i ? " ":"", led_info[i].name);
	fprintf(output,
		"\n"
		"\n"
		"\tSeparate multiple entries with a comma, "
		"ie ledname[,ledname,...]\n"
		"\tor specify \"all\" for all leds\n"
		"\n"
		"Commands:\n"
		"\n"
		"\ton                : turn led(s) on\n"
		"\toff               : turn led(s) off\n"
		"\tblink_on [delay]  : blink led(s) every \"delay\" msecs\n"
		"\tblink_off         : stop blinking leds (and return to "
		"previous state)\n"
		"\tstatus            : show current led status\n"
		"\n"
		"Example usage:\n"
		"\n"
		"\tledctrl %s status\n"
		"\n", led_info[0].name);
	exit(retval);
}

int main(int argc, char *argv[])
{
	struct ledctrl_ops *ops;
	int ret;

	if (argc < 2)
		usage(-EINVAL);

	if (strncmp(argv[1], "--", 2) == 0)
		ret = legacy_ops.parse_option(argc, argv, led_info);
	else
		ret = ledclass_ops.parse_option(argc, argv, led_info);
	if (ret)
		usage(ret);

	ops = &legacy_ops;
	ret = ops->probe();
	if (ret) {
		ops = &ledclass_ops;
		ret = ops->probe();
	}
	if (ret) {
		err("no found led-interface.\n");
		return -ENXIO;
	}

#if defined(LEDCTRL_DEBUG)
	{
		int i;
		for (i=0; led_info[i].id; i++) {
			dbg("led: %s, %d, %d, %d\n",
			    led_info[i].name,
			    led_info[i].command,
			    led_info[i].delay_on,
			    led_info[i].delay_off);
		}
	}
#endif

	return ops->operation(led_info);
}
