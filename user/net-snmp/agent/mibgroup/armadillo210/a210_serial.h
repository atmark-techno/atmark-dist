#ifndef _A210_SERIAL_H_
#define _A210_SERIAL_H_

#define SOCK_PROTO_KEY_STR              "SOCKPROTO"
#define CONNECT_ADDR_KEY_STR            "CONNECTADDR"
#define BAUDRATE_KEY_STR                "BAUDRATE"
#define DATALEN_KEY_STR                 "DATALEN"
#define PARITY_KEY_STR                  "PARITY"
#define STOPBIT_KEY_STR                 "STOPBIT"
#define FLOWCTRL_KEY_STR                "FLOWCTRL"
#define PORTNO_KEY_STR                  "PORTNO"

#define SERIAL_CONFIG_FILE_WITH_FLATFSD "/etc/config/serial.conf"
#define SERIAL_CONFIG_FILE_DEFAULT      "/etc/serial.conf"

#define PORTNO_RANGE_MIN                (1024)
#define PORTNO_RANGE_MAX                (65535)
#define DEFAULT_PORT_NO			(21347)

typedef enum __socket_proto_e
{
	TCPSERVER = 0,
	TCPCLIENT = 1,
	UDP = 2,
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
	NO_PARITY	= 0,
	ODD_PARITY	= 1,
	EVEN_PARITY	= 2,
} parity_e;

typedef enum __stop_bit_e
{
	ONE_BIT		= 0,
	TWO_BIT		= 1,
} stop_bit_e;

typedef enum __flow_ctrl_e
{
	NO_FLOW		= 0,
	HARDWARE	= 1,
} flow_ctrl_e;

typedef struct __serial_setting_s
{
	unsigned long baudrate;
	datalen_e datalen;
	parity_e parity;
	stop_bit_e stop_bit;
	flow_ctrl_e flow_ctrl;
} serial_setting_s;

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)
config_add_mib(ARMADILLO-210-MIB)

extern void init_a210_serial(void);
extern FindVarMethod var_a210_serial;

#endif
