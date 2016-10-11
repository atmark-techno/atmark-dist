#ifndef _AJ_SERIAL_H_
#define _AJ_SERIAL_H_

#define SOCK_PROTO_KEY_STR		"SOCKPROTO"
#define CONNECT_ADDR_KEY_STR	"CONNECTADDR"
#define BAUDRATE_KEY_STR		"BAUDRATE"
#define PARITY_KEY_STR			"PARITY"
#define STOPBIT_KEY_STR			"STOPBIT"
#define FLOWCTRL_KEY_STR		"FLOWCTRL"
#define PORTNO_KEY_STR			"PORTNO"

#ifdef CONFIG_USER_FLATFSD_FLATFSD
#ifdef CONFIG_USER_FLATFSD_DISABLE_USR1
#define SAVE_CONFIG			"flatfsd -s"
#else
#define SAVE_CONFIG			"killall -USR1 flatfsd"
#endif
#define SERIAL_CONFIG_FILE	"/etc/config/serial.conf"
#else
#define SERIAL_CONFIG_FILE	"/etc/serial.conf"
#endif

#define PORTNO_RANGE_MIN                (1024)
#define PORTNO_RANGE_MAX                (65535)
#define DEFAULT_PORT_NO			(21347)


#ifdef  CONFIG_USER_SERI2ETH_SERI2ETH
#define RESET_PROG			"killall -USR1 seri2eth"
#endif

typedef enum __socket_proto_e
{
	TCPSERVER = 0,
	TCPCLIENT = 1,
	UDP = 2,
} sock_proto_e;

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
	parity_e parity;
	stop_bit_e stop_bit;
	flow_ctrl_e flow_ctrl;
} serial_setting_s;

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)

extern void init_aj_serial(void);
extern FindVarMethod var_aj_serial;

#endif
