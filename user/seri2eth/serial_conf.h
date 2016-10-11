#ifndef _SERIAL_CONF_H_
#define _SERIAL_CONF_H_

#include <config/autoconf.h>

#define SOCK_PROTO_KEY_STR              "SOCKPROTO"
#define CONNECT_ADDR_KEY_STR            "CONNECTADDR"
#define BAUDRATE_KEY_STR                "BAUDRATE"
#define DATALEN_KEY_STR                 "DATALEN"
#define PARITY_KEY_STR                  "PARITY"
#define STOPBIT_KEY_STR                 "STOPBIT"
#define FLOWCTRL_KEY_STR                "FLOWCTRL"
#define PORTNO_KEY_STR                  "PORTNO"

#ifndef CONFIG_USER_FLATFSD_FLATFSD
#define SERIAL_CONFIG_FILE              "/etc/serial.conf"
#else
#define SERIAL_CONFIG_FILE              "/etc/config/serial.conf"
#endif

/* Makes seri2eth FSSTND compliant. */
#define UUCPLOCK			"/var/lock"

#define PORTNO_RANGE_MIN                (1024)
#define PORTNO_RANGE_MAX                (65535)

typedef enum __socket_proto_e
{
	TCPSERVER = 0,
	TCPCLIENT = 1,
	UDP       = 2,
} sock_proto_e;

typedef enum __datalen_e
{
	DATALEN5 = 0,
	DATALEN6 = 1,
	DATALEN7 = 2,
	DATALEN8 = 3,
} datalen_e;

typedef enum __parity_e
{
	NO_PARITY   = 0,
	ODD_PARITY  = 1,
	EVEN_PARITY = 2,
} parity_e;

typedef enum __stop_bit_e
{
	ONE_BIT = 0,
	TWO_BIT = 1,
} stop_bit_e;

typedef enum __flow_ctrl_e
{
	NO_FLOW  = 0,
	HARDWARE = 1,
} flow_ctrl_e;

typedef struct __serial_setting_s
{
	unsigned long baudrate;
	datalen_e datalen;
	parity_e parity;
	stop_bit_e stop_bit;
	flow_ctrl_e flow_ctrl;
} serial_setting_s;

#endif
