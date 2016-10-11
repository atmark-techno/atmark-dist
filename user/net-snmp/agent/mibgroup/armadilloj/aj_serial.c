/************************************************************************
 file name : aj_serial.c
 summary   : serial setting for armadillo-j
 coded by  : F.Morishima
 copyright : Atmark Techno
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


#include <config/autoconf.h>

#include "util_funcs.h"
#include "aj_serial.h"

// MAGIC NUMBERS
#define AJSERISOCKPROTO		1
#define AJSERICONNECTADDR	2
#define AJSERIBITRATE		3
#define AJSERISTOPBIT		4
#define AJSERIPARITY		5
#define AJSERIFLOWCTRL		6
#define AJSERIPORTNO		7

oid aj_serial_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 2, 2 };

struct variable2 aj_serial_variables[] = {
    {AJSERISOCKPROTO, ASN_INTEGER, RWRITE, var_aj_serial, 1, {2}},
    {AJSERICONNECTADDR, ASN_IPADDRESS, RWRITE, var_aj_serial, 1, {4}},
    {AJSERIPORTNO, ASN_INTEGER, RWRITE, var_aj_serial, 1, {5}},
    {AJSERIBITRATE, ASN_INTEGER, RWRITE, var_aj_serial, 1, {10}},
    {AJSERISTOPBIT, ASN_INTEGER, RWRITE, var_aj_serial, 1, {15}},
    {AJSERIPARITY, ASN_INTEGER, RWRITE, var_aj_serial, 1, {20}},
    {AJSERIFLOWCTRL, ASN_INTEGER, RWRITE, var_aj_serial, 1, {25}},
};

static unsigned int g_save_flag = 0;
static unsigned long g_connect_addr = 0;
static unsigned int g_port_no = DEFAULT_PORT_NO;
static sock_proto_e g_sock_proto = TCPSERVER;
static serial_setting_s g_serial_setting = {
	9600, NO_PARITY, ONE_BIT, NO_FLOW
};

/******************************************************************
 * Check port no value is valid
 *****************************************************************/
static int check_portno(int value)
{
	if(PORTNO_RANGE_MIN <= value && value <= PORTNO_RANGE_MAX){
		return 1;
	}
	return 0;
}

/******************************************************************
 * Check baud rate value is valid
 *****************************************************************/
static int check_baudrate(int value)
{
	switch(value){
	case 600:
	case 1200:
	case 2400:
	case 1800:
	case 4800:
	case 9600:
	case 19200:
	case 38400:
	case 57600:
	case 115200:
	case 230400:
		return 1;
	default:
		return 0;
	}
}

/******************************************************************
 * Check parity value is valid
 *****************************************************************/
static int check_parity(int value)
{
	switch(value){
	case ODD_PARITY:
	case EVEN_PARITY:
	case NO_PARITY:
		return 1;
	default:
		return 0;
	}
}

/******************************************************************
 * Check flow control value is valid
 *****************************************************************/
static int check_flow_ctrl(int value)
{
	switch(value){
	case HARDWARE:
	case NO_FLOW:
		return 1;
	default:
		return 0;
	}
}

/******************************************************************
 * Check socket protocol is valied
 *****************************************************************/
static int check_sock_proto(int value)
{
	switch(value){
	case TCPSERVER:
	case TCPCLIENT:
	case UDP:
		return 1;
	default:
		return 0;
	}
}

/******************************************************************
 * Check stop bit value is valid
 *****************************************************************/
static int check_stop_bit(int value)
{
	switch(value){
	case ONE_BIT:
	case TWO_BIT:
		return 1;
	default:
		return 0;
	}
}

/******************************************************************
 * load serial setting
 *****************************************************************/
static void load_serial_setting(void)
{
    char buff[256];
    FILE* fp = fopen(SERIAL_CONFIG_FILE, "r");
    if(!fp){
        DEBUGMSGTL(("aj_serial", "init_serial_seting()\n"));
        return;
    }

    /* get value from config file */
    while(fgets(buff, sizeof(buff), fp)){
        const char* fmt = "%s %u";
        char key[64];
        unsigned long value;

        if(sscanf(buff, fmt, key, &value) != 2){
	        DEBUGMSGTL(("aj_serial", "format invalid %s\n", buff));
            continue;
        }

		if(!strcmp(key, SOCK_PROTO_KEY_STR)){
			if(check_sock_proto(value)){
				g_sock_proto = value;
			}
		}
		else if(!strcmp(key, CONNECT_ADDR_KEY_STR)){
			g_connect_addr = value;
		}
		else if(!strcmp(key, PORTNO_KEY_STR)){
			if(check_portno(value)){
				g_port_no = value;
			}else{
				g_port_no = DEFAULT_PORT_NO;
			}
		}
		else if(!strcmp(key, BAUDRATE_KEY_STR)){
			if(check_baudrate(value)){
				g_serial_setting.baudrate = value;
			}
		}
		else if(!strcmp(key, STOPBIT_KEY_STR)){
			if(check_stop_bit(value)){
				g_serial_setting.stop_bit = (stop_bit_e)value;
			}
		}
		else if(!strcmp(key, PARITY_KEY_STR)){
			if(check_parity(value)){
				g_serial_setting.parity = (parity_e)value;
			}
		}
		else if(!strcmp(key, FLOWCTRL_KEY_STR)){
			if(check_flow_ctrl(value)){
				g_serial_setting.flow_ctrl = (flow_ctrl_e)value;
			}
		}
		else{
   			DEBUGMSGTL(("aj_serial", "invalid key %s\n", key));
		}
	}

	fclose(fp);
	return;
}

/******************************************************************
 * serial setting
 *****************************************************************/
void init_aj_serial(void)
{
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));
	REGISTER_MIB(
				"aj_serial",
				aj_serial_variables,
				variable2,
				aj_serial_variables_oid
				);
	load_serial_setting();
}

/******************************************************************
 * save serial setting to config file
 *****************************************************************/
static int save_serial_config()
{
	char work_str[256];
	FILE *fp = fopen(SERIAL_CONFIG_FILE, "w");
	if(!fp){
		DEBUGMSGTL(("aj_serial", "fopen() failed : %s\n",
					SERIAL_CONFIG_FILE));
		return -1;
	}

	sprintf(work_str, "%s %lu\n", CONNECT_ADDR_KEY_STR, g_connect_addr);
	fwrite(work_str, 1, strlen(work_str), fp);
	sprintf(work_str, "%s %lu\n", PORTNO_KEY_STR, g_port_no);
	fwrite(work_str, 1, strlen(work_str), fp);
	sprintf(work_str, "%s %lu\n", SOCK_PROTO_KEY_STR, g_sock_proto);
	fwrite(work_str, 1, strlen(work_str), fp);
	sprintf(work_str, "%s %lu\n", BAUDRATE_KEY_STR, g_serial_setting.baudrate);
	fwrite(work_str, 1, strlen(work_str), fp);
	sprintf(work_str, "%s %u\n", PARITY_KEY_STR, g_serial_setting.parity);
	fwrite(work_str, 1, strlen(work_str), fp);
	sprintf(work_str, "%s %u\n", STOPBIT_KEY_STR, g_serial_setting.stop_bit);
	fwrite(work_str, 1, strlen(work_str), fp);
	sprintf(work_str, "%s %u\n", FLOWCTRL_KEY_STR, g_serial_setting.flow_ctrl);
	fwrite(work_str, 1, strlen(work_str), fp);

	fclose(fp);

#ifdef  CONFIG_USER_SERI2ETH_SERI2ETH
	system(RESET_PROG);
#endif
#ifdef CONFIG_USER_FLATFSD_FLATFSD
	system(SAVE_CONFIG);
#ifndef CONFIG_USER_FLATFSD_DISABLE_USR1
	sleep(3);
#endif
#endif
	return 0;
}

/******************************************************************
 * change ip address that connect to
 ******************************************************************/
static int writeConnectAddr(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_IPADDRESS){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		break;

	case ACTION:
		if(g_connect_addr != value){
			g_connect_addr = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			save_serial_config();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}
/******************************************************************
 * change port no
 ******************************************************************/
static int writePortNo(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if(var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_portno(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_port_no != value){
			g_port_no = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			save_serial_config();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change socket protocol
 ******************************************************************/
static int writeSockProto(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if(var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_sock_proto(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_sock_proto != value){
			g_sock_proto = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			save_serial_config();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change baudrate
 ******************************************************************/
static int writeBaudrate(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_baudrate(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_serial_setting.baudrate != value){
			g_serial_setting.baudrate = value;
			g_save_flag = 1;
		}
		break;
	
	case COMMIT:
		if(g_save_flag){
			save_serial_config();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change stop bit
 ******************************************************************/
static int writeStopbit(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_stop_bit(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_serial_setting.stop_bit != value){
			g_serial_setting.stop_bit = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			save_serial_config();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change flow control
 ******************************************************************/
static int writeFlowCtrl(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_flow_ctrl(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_serial_setting.flow_ctrl != value){
			g_serial_setting.flow_ctrl = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			save_serial_config();
			g_save_flag = 0;
		}
		break;
	default:
		break;
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * change parity
 ******************************************************************/
static int writeParity(
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
	DEBUGMSGTL(("aj_serial", "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL(("aj_serial", "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL(("aj_serial", "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_parity(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_serial_setting.parity != value){
			g_serial_setting.parity = value;
			g_save_flag = 1;
		}
		break;

	case COMMIT:
		if(g_save_flag){
			save_serial_config();
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
 *****************************************************************/
unsigned char* var_aj_serial(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
	unsigned long long_ret;

	DEBUGMSGTL(("aj_serial", "var_aj_serial()\n"));
	if(header_generic(vp, name, length, exact, var_len, write_method)
		== MATCH_FAILED){
		return NULL;
	}

	DEBUGMSGTL(("aj_serial", "magic : %d\n", vp->magic));

	switch(vp->magic){

	case AJSERICONNECTADDR:
		*write_method = writeConnectAddr;
		return (u_char *)&g_connect_addr;

	case AJSERIPORTNO:
		*write_method = writePortNo;
		return (u_char *)&g_port_no;

	case AJSERISOCKPROTO:
		*write_method = writeSockProto;
		return (u_char *)&g_sock_proto;

	case AJSERIBITRATE:
		*write_method = writeBaudrate;
		return (u_char *)&g_serial_setting.baudrate;

	case AJSERISTOPBIT:
		*write_method = writeStopbit;
		return (u_char *)&g_serial_setting.stop_bit;

	case AJSERIPARITY:
		*write_method = writeParity;
		return (u_char *)&g_serial_setting.parity;

	case AJSERIFLOWCTRL:
		*write_method = writeFlowCtrl;
		return (u_char *)&g_serial_setting.flow_ctrl;

	default:
		DEBUGMSGTL(("aj_serial", "invlid oid%d\n", vp->magic));
		break;
	}
	return NULL;
}

