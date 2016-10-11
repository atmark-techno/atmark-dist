#ifndef __LEDCTRL_COMMON_H_
#define __LEDCTRL_COMMON_H_

#include "legacy.h"

#define PROGNAME "ledctrl"

struct ledctrl_info {
	int id; /* LED_RED, LED_GREEN, LED_YELLOW */
/*      LED_RED		(1 << 1)	See. legacy.h */
/*      LED_GREEN	(1 << 0)	See. legacy.h */
#define LED_YELLOW	(1 << 2)
	char *name;
	int command;
#define LED_ON			(1)
#define LED_OFF			(2)
#define LED_BLINK_ON		(3)
#define LED_BLINK_OFF		(4)
#define LED_STATUS		(5)
#define LED_LEGACY_STATUS	(6)
	int delay_on;
	int delay_off;
};

struct ledctrl_ops {
	int (*probe)(void);
	int (*parse_option)(int argc, char **argv, struct ledctrl_info *info);
	int (*operation)(struct ledctrl_info *info);
};

#define err(format, arg...) fprintf(stderr, PROGNAME ": " format, ## arg)
#define info(format, arg...) fprintf(stdout, format, ## arg)

#if defined(LEDCTRL_DEBUG)
#define dbg(format, arg...) fprintf(stdout, "debug: " format, ## arg)
#else
#define dbg(format, arg...)
#endif

#endif
