/************************************************************************
 file name : aj_gpio.c
 summary   : gpio control
 coded by  : F.Morishima
 copyright : Atmark techno
************************************************************************/
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include <arpa/inet.h>
#include <config/autoconf.h>

#include "util_funcs.h"
#include "aj_gpio.h"

#ifndef TRUE
#define	TRUE	1
#endif
#ifndef FALSE
#define	FALSE	0
#endif
#define	LOW		0
#define	HIGH	1

#define GPIO_PORT_NUM	5
typedef enum __gpio_mode_e
{
    SERIAL_MODE = 0,
    INPUT_MODE = 1,
    OUTPUT_MODE = 2,
} gpio_mode_e;

typedef struct __port_status
{
	gpio_mode_e mode;
	unsigned int trap_time;
	unsigned int init_data;
	unsigned int timer_id;
	unsigned int old_status;
} port_status;

static unsigned int g_save_flag = 0;
static unsigned long g_manager_addr;
static unsigned long g_check_cycle = 100;	/* msec */
static port_status g_port_info[GPIO_PORT_NUM];

////
// register control
////

#ifdef __DEBUG__
	#define GPIO_PORTA_REG			tmp_ptr
	int* tmp_ptr;
#else
	#define GPIO_PORTA_REG			0xffb00020
#endif

#define DEFAULT_REG_VAL			0xffe00000	/* use serial */

#define PORT_MODE_BASE			0x01000000
#define PORT_DIR_BASE           0x00010000
#define PORT_DATA_BASE          0x00000001

#define REVERSE_STATUS(status)	((status)? 0 : 1)
#define REAL_PORT_NO(port)		((port < 3)? port : port + 2)
#define PORT_MODE_MAP(port)		(PORT_MODE_BASE << REAL_PORT_NO(port))
#define PORT_DIR_MAP(port)		(PORT_DIR_BASE << REAL_PORT_NO(port))
#define PORT_DATA_MAP(port)		(PORT_DATA_BASE << REAL_PORT_NO(port))

////
// configuration file
////
#ifdef CONFIG_USER_FLATFSD_DISABLE_USR1
#define SAVE_CONFIG		"flatfsd -s"
#else
#define SAVE_CONFIG		"killall -USR1 flatfsd"
#endif

#ifndef CONFIG_USER_FLATFSD_FLATFSD
#define GPIO_CONFIG_FILE	"/etc/gpio.conf"
#else
#define GPIO_CONFIG_FILE	"/etc/config/gpio.conf"
#endif

static const char* MANAGER_KEY_STR		= "MANAGER";
static const char* CHECK_CYCLE_KEY_STR	= "CHECK_CYCLE";
static const char* MODE_KEY_STR[] = {
	"GPIO_MODE0", "GPIO_MODE1", "GPIO_MODE2", "GPIO_MODE3", "GPIO_MODE4"
};
static const char* INIT_DATA_KEY_STR[] = {
	"GPIO_INIT0", "GPIO_INIT1", "GPIO_INIT2", "GPIO_INIT3", "GPIO_INIT4"
};
static const char* TRAPTIME_KEY_STR[] = {
	"GPIO_TIME0", "GPIO_TIME1", "GPIO_TIME2", "GPIO_TIME3", "GPIO_TIME4"
};

////
// ucd snmp format
////

// MAGIC NUMBERS
#define AJGPIOMANAGER		1
#define AJGPIOPOSITION		2
#define AJGPIOMODE			3
#define AJGPIODATA			4
#define AJGPIOINITDATA		5
#define AJGPIOTRAPTIME		6
#define AJGPIOCHECKCYCLE	7
#define AJGPIOALLDATA		8

oid aj_gpio_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 2, 3 };

struct variable4 aj_gpio_variables[] = {
	{AJGPIOMANAGER, ASN_IPADDRESS, RWRITE, var_aj_gpio, 1, {1}},
	{AJGPIOCHECKCYCLE, ASN_INTEGER, RWRITE, var_aj_gpio, 1, {5}},
	{AJGPIOALLDATA, ASN_INTEGER, RWRITE, var_aj_gpio, 1, {7}},
	{AJGPIOPOSITION, ASN_INTEGER, RONLY, var_aj_gpio, 3, {10,1,1}},
	{AJGPIOMODE, ASN_INTEGER, RWRITE, var_aj_gpio, 3, {10,1,2}},
	{AJGPIODATA, ASN_INTEGER, RWRITE, var_aj_gpio, 3, {10,1,3}},
	{AJGPIOINITDATA, ASN_INTEGER, RWRITE, var_aj_gpio, 3, {10,1,4}},
	{AJGPIOTRAPTIME, ASN_INTEGER, RWRITE, var_aj_gpio, 3, {10,1,5}},
};

////
// trap information
////
const char* objid_gpiochange = ".1.3.6.1.4.1.16031.1.2.100.0";
oid objid_gpiostatus[] = {1, 3, 6, 1, 4, 1, 16031, 1, 2, 3, 10, 1, 3, 0};
oid objid_sysuptime[]  = {1, 3, 6, 1, 2, 1, 1, 3, 0};
oid objid_snmptrap[]   = {1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0};

#define OID_GPIO_STATUS_LEN		14
#define DEFAULT_COMMUNITY_NAME	"public"

/******************************************************************
 * get/set gpio register value
 *****************************************************************/
inline static unsigned long get_gpio_reg(void)
{
	return *(volatile unsigned long*)GPIO_PORTA_REG;
}

inline static void set_gpio_reg(unsigned long val)
{
	*(volatile unsigned long*)GPIO_PORTA_REG = val;
}

/******************************************************************
 * return indicated GPIO data
 *****************************************************************/
inline static unsigned long GetPortData(unsigned int index)
{
	return (get_gpio_reg() & PORT_DATA_MAP(index))? 1 : 0;
}

/******************************************************************
 * Call back function by snmp trap
 *****************************************************************/
int armadillo_snmp_input(int operation,
	       struct snmp_session *session,
	       int reqid,
	       struct snmp_pdu *pdu,
	       void *magic)
{
  return 1;
}

/******************************************************************
 * Send trap
 *****************************************************************/
static int SendGpioTrap(unsigned long manager_addr, unsigned long portNo)
{
    struct snmp_session session, *ss;
    struct snmp_pdu *pdu;
    char *trap = NULL;
	long sysuptime;
	char csysuptime [20];
	char buff[20];
	int err = 0;

     netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DEFAULT_PORT, 
		        SNMP_TRAP_PORT);

	snmp_sess_init(&session);

    session.callback		= armadillo_snmp_input;
    session.callback_magic	= NULL;
	session.remote_port		= SNMP_TRAP_PORT;
	session.version			= SNMP_VERSION_2c;
	session.community		= DEFAULT_COMMUNITY_NAME;
	session.community_len	= strlen(DEFAULT_COMMUNITY_NAME);
	session.peername		= inet_ntoa(*(struct in_addr*)&manager_addr);

	DEBUGMSGTL(("aj_gpio", "trap to ip : %s, port : %d\n",
				session.peername, session.remote_port));
	
    ss = snmp_open(&session);
    if (ss == NULL){
		DEBUGMSGTL(("aj_gpio", "snmp_open() failed()\n"));
        err = 1;
	 goto err_out;
    }

	pdu = snmp_pdu_create(SNMP_MSG_TRAP2);

	/* up time */
   	sysuptime = get_uptime();
   	sprintf (csysuptime, "%ld", sysuptime);
   	trap = csysuptime;

	snmp_add_var(pdu, objid_sysuptime,
			sizeof(objid_sysuptime)/sizeof(oid), 't', trap);

	/* object ID */
	if(snmp_add_var (pdu, objid_snmptrap,
					sizeof(objid_snmptrap)/sizeof(oid),
					'o', objid_gpiochange) != 0) {
		DEBUGMSGTL(("aj_gpio", "snmp_add_var() failed()\n"));
    		err = 1;
		goto err_out;
	}

	/* add port status variable */
	objid_gpiostatus[OID_GPIO_STATUS_LEN - 1] = portNo + 1;
	sprintf(buff, "%lu", REVERSE_STATUS(g_port_info[portNo].old_status));
	if (snmp_add_var (pdu, objid_gpiostatus,
						sizeof(objid_gpiostatus)/sizeof(oid),
						'i', buff) != 0) {
		DEBUGMSGTL(("aj_gpio", "snmp_add_var() failed()\n"));
    		err = 1;
		goto err_out;
	}

	/* send trap packet */
	if(!snmp_send(ss, pdu)){
		DEBUGMSGTL(("aj_gpio", "send trap failed()\n"));
		snmp_free_pdu(pdu);
	}

    snmp_close(ss);

  err_out:
	netsnmp_ds_set_int(NETSNMP_DS_LIBRARY_ID, NETSNMP_DS_LIB_DEFAULT_PORT, 
			   SNMP_PORT);

	if( err )
		return -1;
	else
		return 0;
}

/******************************************************************
 * initialization of trap status
 *****************************************************************/
static void InitTrapStatus(void)
{
	int i;
	DEBUGMSGTL(("aj_gpio", "in InitTrapStatus()\n"));

	/* get init state */
	for(i = 0 ; i < GPIO_PORT_NUM ; i++){
		g_port_info[i].old_status = GetPortData(i);
	}
}

/******************************************************************
 * Reset timer for trap
 *****************************************************************/
inline void ResetTrapTimer(int portNo)
{
	snmp_alarm_unregister(g_port_info[portNo].timer_id);
	g_port_info[portNo].timer_id = 0;
}

/******************************************************************
 * thread function checking port status and send trap
 *****************************************************************/
static void TrapTimeout(unsigned int reg, void *param)
{
	int portNo = (int)(int*)param;
	if(g_port_info[portNo].mode == INPUT_MODE){
		SendGpioTrap(g_manager_addr, portNo);
		g_port_info[portNo].old_status = 
			REVERSE_STATUS(g_port_info[portNo].old_status);
	}

	ResetTrapTimer(portNo);
	return;
}

/******************************************************************
 * Set timer for trap
 *****************************************************************/
void SetTrapTimer(int portNo, unsigned int timeout)
{
	struct timeval to;
	if(!timeout) return;
	if(g_port_info[portNo].timer_id){
		ResetTrapTimer(portNo);
	}
	
	to.tv_sec = timeout / 1000;
	to.tv_usec = (timeout % 1000) * 1000;
	g_port_info[portNo].timer_id = snmp_alarm_register_hr(
										to,
										0,
										TrapTimeout,
										(void*)portNo
										);
}

/******************************************************************
 * thread function checking port status and send trap
 *****************************************************************/
static void StatusCheckFunc(unsigned int reg, void *param)
{
	int i;
	DEBUGMSGTL(("aj_gpio", "start status check thread\n"));

	for(i = 0 ; i < GPIO_PORT_NUM ; i++){
		/* no trap setting */
		if(!g_port_info[i].trap_time){
			continue;
		}

		/* not input port */
		if(g_port_info[i].mode != INPUT_MODE){
			continue;
		}

		/* check status and set/reset timer */
		if(g_port_info[i].old_status != GetPortData(i) &&
		   !g_port_info[i].timer_id)
		{
			SetTrapTimer(i, g_port_info[i].trap_time);
		}

		if(g_port_info[i].old_status == GetPortData(i) &&
		   g_port_info[i].timer_id)
		{
			ResetTrapTimer(i);
		}
	}
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/******************************************************************
 * create / destroy trap thread
 *****************************************************************/
void TrapControl()
{
	int i;
	static unsigned int timer_reg = 0;
	struct timeval timer_interval;
	timer_interval.tv_sec = 0;
	timer_interval.tv_usec = g_check_cycle * 1000;

	for(i = 0 ; i < GPIO_PORT_NUM ; i++){
		if(g_port_info[i].mode == INPUT_MODE &&
			g_port_info[i].trap_time != 0){
			break;
		}
	}

	if(i == GPIO_PORT_NUM && timer_reg){
		/* no INPUT port , so unregist timer */
		DEBUGMSGTL(("aj_gpio", "unregister trap timer\n"));
		snmp_alarm_unregister(timer_reg);
		timer_reg = 0;
		return;
	}

	if(i != GPIO_PORT_NUM && !timer_reg){
		/* INPUT port exist, so regist timer */
		DEBUGMSGTL(("aj_gpio", "register trap timer\n"));
		InitTrapStatus();
		timer_reg = snmp_alarm_register_hr(
						timer_interval,
						SA_REPEAT,
						StatusCheckFunc,
						NULL
						);
		return;
	}
}


/******************************************************************
 * save gpio setting to config file
 *****************************************************************/
static int SaveGpioConfig(void)
{
	int i;
	char work_str[256];
	FILE *fp = fopen(GPIO_CONFIG_FILE, "w");
	if(!fp){
		DEBUGMSGTL(("aj_gpio", "fopen() failed : %s\n",
					GPIO_CONFIG_FILE));
		return -1;
	}

	sprintf(work_str, "%s %u\n", MANAGER_KEY_STR, g_manager_addr);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %u\n", CHECK_CYCLE_KEY_STR, g_check_cycle);
	fwrite(work_str, 1, strlen(work_str), fp);

	for(i = 0 ; i < GPIO_PORT_NUM ; i++){
		// mode, data, traptime
		sprintf(work_str, "%s %u\n", MODE_KEY_STR[i], g_port_info[i].mode);
		fwrite(work_str, 1, strlen(work_str), fp);

		sprintf(work_str, "%s %u\n", INIT_DATA_KEY_STR[i], g_port_info[i].init_data);
		fwrite(work_str, 1, strlen(work_str), fp);

		sprintf(work_str, "%s %u\n", TRAPTIME_KEY_STR[i], g_port_info[i].trap_time);
		fwrite(work_str, 1, strlen(work_str), fp);
	}

	fclose(fp);

#ifdef CONFIG_USER_FLATFSD_FLATFSD
	system(SAVE_CONFIG);
#ifndef CONFIG_USER_FLATFSD_DISABLE_USR1
	sleep(3);
#endif
#endif

	return 0;
}

/******************************************************************
 * change gpio data
 *****************************************************************/
static int ChangePortData(unsigned int value, unsigned int index)
{
   	DEBUGMSGTL(("aj_gpio", "ChangePortData()\n"));

	if(value > 1){
		return FALSE;
	}

	if(value){
		set_gpio_reg(get_gpio_reg() | PORT_DATA_MAP(index));
	}
	else{
		set_gpio_reg(get_gpio_reg() & ~PORT_DATA_MAP(index));
	}
	DEBUGMSGTL(("aj_gpio", "register val : %lu\n", get_gpio_reg()));

	return TRUE;
}

/******************************************************************
 * change gpio mode
 * return 1 : OK, 0 : NG
 *****************************************************************/
static int ChangePortMode(gpio_mode_e mode, unsigned int index)
{
   	DEBUGMSGTL(("aj_gpio", "ChangePortMode()\n"));

	if(g_port_info[index].mode == mode){
		return TRUE;
	}

	switch(mode){
	case INPUT_MODE:
		set_gpio_reg(
			(get_gpio_reg() & ~PORT_MODE_MAP(index)) & ~PORT_DIR_MAP(index));
		g_port_info[index].old_status = GetPortData(index);
		break;
	case OUTPUT_MODE:
		// change mode and data to follow init value
		set_gpio_reg(
			(get_gpio_reg() & ~PORT_MODE_MAP(index)) | PORT_DIR_MAP(index));
		ChangePortData(g_port_info[index].init_data, index);
		break;
	case SERIAL_MODE:
		set_gpio_reg(get_gpio_reg() | PORT_MODE_MAP(index));
		switch(index){
		case 0:
		case 1:
		case 2:
			set_gpio_reg(get_gpio_reg() & ~PORT_DIR_MAP(index));
			break;
		default:
			set_gpio_reg(get_gpio_reg() | PORT_DIR_MAP(index));
			break;
		}
		DEBUGMSGTL(("aj_gpio", "register val : %04x\n", get_gpio_reg()));
		break;
	default:
		return FALSE;
	}
	g_port_info[index].mode = mode;

	// call TrapControl after change mode
	TrapControl();
	ResetTrapTimer(index);
	return TRUE;
}

/******************************************************************
 * Get threshold time to send trap when port status changed
 *****************************************************************/
static int LoadGpioSetting(void)
{
    char buff[256];
	FILE* fp;
	int i;
	port_status work_port_info[GPIO_PORT_NUM];

	// set default
	g_manager_addr = 0;
	g_check_cycle = 100;
	for(i = 0 ; i < GPIO_PORT_NUM ; i++){
		work_port_info[i].trap_time = 0;
		work_port_info[i].init_data = LOW;	// low is better as init
		work_port_info[i].mode			= SERIAL_MODE;
	}

    fp = fopen(GPIO_CONFIG_FILE, "r");
    if(!fp){
        DEBUGMSGTL(("aj_gpio", "init_gpio_seting()\n"));
        return -1;
    }

	while(fgets(buff, sizeof(buff), fp)){
		int i;
		const char* fmt = "%s %u";
		char key[64];
		unsigned long value;

		if(sscanf(buff, fmt, key, &value) != 2){;
			DEBUGMSGTL(("aj_gpio", "format invalid %s\n", buff));
		    continue;
		}

		if(!strcmp(key, MANAGER_KEY_STR)){
			g_manager_addr = value;
			continue;
		}

		if(!strcmp(key, CHECK_CYCLE_KEY_STR)){
			g_check_cycle = value;
			continue;
		}

		for(i = 0 ; i < GPIO_PORT_NUM ; i++){
			if(!strcmp(key, TRAPTIME_KEY_STR[i])){
				work_port_info[i].trap_time = value;
				break;
			}
			if(!strcmp(key, INIT_DATA_KEY_STR[i])){
				work_port_info[i].init_data = value;
				break;
			}
			if(!strcmp(key, MODE_KEY_STR[i])){
				work_port_info[i].mode = value;
				break;
			}
		}
	}

	fclose(fp);

	// set initalized value
	for(i = 0 ; i < GPIO_PORT_NUM ; i++){
		g_port_info[i].init_data = work_port_info[i].init_data;
		g_port_info[i].trap_time = work_port_info[i].trap_time;
		// (call after copy init_data)
		ChangePortMode(work_port_info[i].mode, i);
	}

	return 0;
}

/******************************************************************
 * initialize
 *****************************************************************/
void init_aj_gpio(void)
{
	#ifdef __DEBUG__
		tmp_ptr = malloc(4);
	#endif
	
	REGISTER_MIB(
				"aj_gpio",
				aj_gpio_variables,
				variable4,
				aj_gpio_variables_oid
				);
	LoadGpioSetting();

	/* start GPIO status check timer */
	TrapControl();
}

/******************************************************************
 * change manager ip address
 *****************************************************************/
static int writeManagerAddr(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	unsigned long value = *(unsigned long *)var_val;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_IPADDRESS){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		break;
	
	case ACTION:
		if(g_manager_addr != value){
			g_manager_addr = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			SaveGpioConfig();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change all gpio data
 *****************************************************************/
static int writeAllData(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	int i;
	unsigned long value = *(unsigned long *)var_val;
	unsigned long work;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(value > 31){
			return SNMP_ERR_WRONGVALUE;
		}
		break;
	
	case ACTION:
		work = 0;
		for(i = 0 ; i < 5 ; i++){
			work |= (value & (1 << i))? PORT_DATA_MAP(i) : 0;
		}
		set_gpio_reg((get_gpio_reg() & 0xffffff00) | work);
		break;

	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change status check cycle
 *****************************************************************/
static int writeCheckCycle(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	unsigned long value = *(unsigned long *)var_val;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(value > 1000 || value < 10){	// 10ms - 1s
			return SNMP_ERR_WRONGVALUE;
		}
		break;
	
	case ACTION:
		if(g_check_cycle != value){
			g_check_cycle = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			SaveGpioConfig();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change gpio mode by SET command
 *****************************************************************/
static int writeMode(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	unsigned portNo;
	unsigned long value = *(unsigned long *)var_val;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(value != SERIAL_MODE && value != INPUT_MODE && value != OUTPUT_MODE){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		portNo = name[name_len - 1] - 1;
		if(g_port_info[portNo].mode != value){
			ChangePortMode(value, portNo);
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			SaveGpioConfig();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * set output value of gpio data
 *****************************************************************/
static int writeData(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	unsigned portNo;
	unsigned long value = *(unsigned long *)var_val;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(value > 1){
			return SNMP_ERR_WRONGVALUE;
		}
		break;
	
	case ACTION:
		portNo = name[name_len - 1] - 1;
		ChangePortData(value, portNo);
		/* no need to save (only initial value) */
		break;
	
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * set output value of gpio init data
 *****************************************************************/
static int writeInitData(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	unsigned portNo;
	unsigned long value = *(unsigned long *)var_val;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(value > 1){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		portNo = name[name_len - 1] - 1;
		if(g_port_info[portNo].init_data != value){
			g_port_info[portNo].init_data = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			SaveGpioConfig();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change gpio trap threshold time
 *****************************************************************/
static int writeTrapTime(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	unsigned portNo;
	unsigned long value = *(unsigned long *)var_val;

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_gpio", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_gpio", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(value > 604800000 || (value > 0 && value < 30)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;
	
	case ACTION:
		portNo = name[name_len - 1] - 1;
		if(g_port_info[portNo].trap_time != value){
			g_port_info[portNo].trap_time = value;
			ResetTrapTimer(portNo);
			TrapControl();
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			SaveGpioConfig();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * hander when access by oid
 ******************************************************************/
unsigned char* var_aj_gpio(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
	int i;
	static unsigned long ret;
	unsigned long portNo;
	DEBUGMSGTL(("aj_gpio", "var_aj_gpio()\n"));

	DEBUGMSGTL(("aj_serial", "magic : %d\n", vp->magic));

	// generic type (manager addr, check cycle, all output data)
	switch(vp->magic){
	case AJGPIOMANAGER:
		if(header_generic(
			vp, name, length, exact, var_len, write_method)
			== MATCH_FAILED){
			return NULL;
		}
		*write_method = writeManagerAddr;
		return (u_char *)&g_manager_addr;
	
	case AJGPIOCHECKCYCLE:
		if(header_generic(
			vp, name, length, exact, var_len, write_method)
			== MATCH_FAILED){
			return NULL;
		}
		*write_method = writeCheckCycle;
		return (u_char *)&g_check_cycle;
	
	case AJGPIOALLDATA:
		if(header_generic(
			vp, name, length, exact, var_len, write_method)
			== MATCH_FAILED){
			return NULL;
		}
		*write_method = writeAllData;
		ret = 0;
		for(i = 0 ; i < 5 ; i++){
			ret |= GetPortData(i) << i;
		}
		return (u_char *)&ret;

	default:
		break;
	}

	// table type
	if(header_simple_table(
		vp, name, length, exact, var_len, write_method, GPIO_PORT_NUM)
			== MATCH_FAILED){
		return NULL;
	}
	portNo = name[*length - 1] - 1;

	switch(vp->magic){
	case AJGPIOPOSITION:
		ret = portNo;
		if(ret > 2){
			ret += 2;
		}
		return (u_char *)&ret;

	case AJGPIOMODE:
		*write_method = writeMode;
		return (u_char *)&g_port_info[portNo].mode;

	case AJGPIODATA:
		ret = GetPortData(portNo);
		*write_method = writeData;
		return (u_char *)&ret;

	case AJGPIOINITDATA:
		*write_method = writeInitData;
		return (u_char *)&g_port_info[portNo].init_data;

	case AJGPIOTRAPTIME:
		*write_method = writeTrapTime;
		return (u_char *)&g_port_info[portNo].trap_time;

	default:
		DEBUGMSGTL(("aj_gpio", "invlid oid%d\n", vp->magic));
		break;
	}

	return NULL;
}
