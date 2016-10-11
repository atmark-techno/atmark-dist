#ifndef _GPIOD_H_
#define _GPIOD_H_

#define VERSION "v1.00"
#define COPYRIGHT "2005 Atmark Techno, Inc."
#define DEVICE "/dev/gpio"

#define DEBUG 1
#undef DEBUG
#if defined(DEBUG)
#define LOG_DEBUG(arg...) fprintf(stdout,arg)
#else
#define LOG_DEBUG(arg...)
#endif

#define LOG_MSG(arg...) fprintf(stdout,arg)
#define LOG_ERR(arg...) fprintf(stderr,arg)

#endif
