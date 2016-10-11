#ifndef __GPIOCTRL_COMMON_H_
#define __GPIOCTRL_COMMON_H_

#include "legacy.h"

#define PROGNAME "gpioctrl"
#define VERSION "v1.01"
#define COPYRIGHT "2005-2010 Atmark Techno, Inc."


enum {
	GPIOIF_LEGACY,
	GPIOIF_GPIOLIB,
	GPIOIF_UNKNOWN = -1,
};


struct gpioctrl_ops {
	int (*probe)(void);
	int (*set)(struct gpio_param *);
	int (*get)(struct gpio_param *);
	int (*interrupt_wait)(struct gpio_param *, struct wait_param *);
};

#define err(format, arg...) fprintf(stderr, PROGNAME ": " format, ## arg)
#define info(format, arg...) fprintf(stdout, format, ## arg)

#if defined(GPIOCTRL_DEBUG)
#define dbg(format, arg...) fprintf(stdout, "debug: " format, ## arg)
#else
#define dbg(format, arg...)
#endif

#endif
