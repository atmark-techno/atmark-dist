/************************************************************************
 file name : a210_serial.c
 summary   : seri2eth setting for armadillo-210
 coded by  : 
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
#include "a210_common.h"
#include "a210_serial.h"

#define MODULE_NAME "a210_serial"

// MAGIC NUMBERS
#define A210_SERISOCKPROTO		1
#define A210_SERICONNECTADDR		2
#define A210_SERIBITRATE		3
#define A210_SERIDATALEN                4
#define A210_SERISTOPBIT		5
#define A210_SERIPARITY			6
#define A210_SERIFLOWCTRL		7
#define A210_SERIPORTNO			8

oid a210_serial_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 4, 2 };

struct variable2 a210_serial_variables[] = {
    {A210_SERISOCKPROTO, ASN_INTEGER, RWRITE, var_a210_serial, 1, {2}},
    {A210_SERICONNECTADDR, ASN_IPADDRESS, RWRITE, var_a210_serial, 1, {4}},
    {A210_SERIPORTNO, ASN_INTEGER, RWRITE, var_a210_serial, 1, {5}},
    {A210_SERIBITRATE, ASN_INTEGER, RWRITE, var_a210_serial, 1, {10}},
    {A210_SERIDATALEN, ASN_INTEGER, RWRITE, var_a210_serial, 1, {12}},
    {A210_SERISTOPBIT, ASN_INTEGER, RWRITE, var_a210_serial, 1, {15}},
    {A210_SERIPARITY, ASN_INTEGER, RWRITE, var_a210_serial, 1, {20}},
    {A210_SERIFLOWCTRL, ASN_INTEGER, RWRITE, var_a210_serial, 1, {25}},
};

static unsigned int g_save_flag = 0;
static unsigned long g_connect_addr = 0;
static unsigned int g_port_no = DEFAULT_PORT_NO;
static sock_proto_e g_sock_proto = TCPSERVER;
static serial_setting_s g_serial_setting = {
	9600, DATALEN8, NO_PARITY, ONE_BIT, NO_FLOW
};

static char *serial_config_file = NULL;

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
	case 300:
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
 * Check datalen value is valid
 *****************************************************************/
static int check_datalen(int value)
{
	switch(value){
	case DATALEN5:
	case DATALEN6:
	case DATALEN7:
	case DATALEN8:
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
    FILE* fp;

    fp = fopen(serial_config_file, "r");
    if(!fp){
        DEBUGMSGTL((MODULE_NAME, "init_serial_seting()\n"));
        return;
    }

    /* get value from config file */
    while(fgets(buff, sizeof(buff), fp)){
        const char* fmt = "%s %u";
        char key[64];
        unsigned long value;

        if(sscanf(buff, fmt, key, &value) != 2){
	        DEBUGMSGTL((MODULE_NAME, "format invalid %s\n", buff));
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
		else if(!strcmp(key, DATALEN_KEY_STR)){
			if(check_datalen(value)){
				g_serial_setting.datalen = value;
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
   			DEBUGMSGTL((MODULE_NAME, "invalid key %s\n", key));
		}
	}

	fclose(fp);
	return;
}

/******************************************************************
 * serial setting
 *****************************************************************/
void init_a210_serial(void)
{
	int ret;

	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));
	REGISTER_MIB(
				MODULE_NAME,
				a210_serial_variables,
				variable2,
				a210_serial_variables_oid
				);

	ret = system_check_executable("flatfsd");
	if(ret == 0){
		serial_config_file = SERIAL_CONFIG_FILE_WITH_FLATFSD;
	}else{
		serial_config_file = SERIAL_CONFIG_FILE_DEFAULT;
	}

	load_serial_setting();
}

/******************************************************************
 * save serial setting to config file
 *****************************************************************/
static int save_serial_config()
{
	char work_str[256];
	FILE *fp = fopen(serial_config_file, "w");
	if(!fp){
		DEBUGMSGTL((MODULE_NAME, "fopen() failed : %s\n",
					serial_config_file));
		return -1;
	}

	sprintf(work_str, "%s %lu\n",
		CONNECT_ADDR_KEY_STR, g_connect_addr);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %lu\n",
		PORTNO_KEY_STR, g_port_no);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %u\n", 
		SOCK_PROTO_KEY_STR, g_sock_proto);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %lu\n", 
		BAUDRATE_KEY_STR, g_serial_setting.baudrate);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %u\n", 
		DATALEN_KEY_STR, g_serial_setting.datalen);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %u\n", 
		PARITY_KEY_STR, g_serial_setting.parity);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %u\n",
		STOPBIT_KEY_STR, g_serial_setting.stop_bit);
	fwrite(work_str, 1, strlen(work_str), fp);

	sprintf(work_str, "%s %u\n", 
		FLOWCTRL_KEY_STR, g_serial_setting.flow_ctrl);
	fwrite(work_str, 1, strlen(work_str), fp);

	fclose(fp);

	system_reset_seri2eth();

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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_IPADDRESS){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if(var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
 * change datalen
 ******************************************************************/
static int writeDataLength(
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
			return SNMP_ERR_WRONGLENGTH;
		}
		if(!check_datalen(value)){
			return SNMP_ERR_WRONGVALUE;
		}
		break;

	case ACTION:
		if(g_serial_setting.datalen != value){
			g_serial_setting.datalen = value;
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));

	switch(action){
	case RESERVE1:
		if(var_val_type != ASN_INTEGER){
			DEBUGMSGTL((MODULE_NAME, "%x net inter type", var_val_type));
			return SNMP_ERR_WRONGTYPE;
		}
		if (var_val_len != sizeof(long)) {
			DEBUGMSGTL((MODULE_NAME, "wrong length %x", var_val_len));
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
unsigned char* var_a210_serial(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
	int ret;

	DEBUGMSGTL((MODULE_NAME, "var_a210_serial()\n"));
	ret = header_generic(vp, name, length, exact, var_len, write_method);
	if(ret == MATCH_FAILED) return NULL;

	DEBUGMSGTL((MODULE_NAME, "magic : %d\n", vp->magic));

	switch(vp->magic){

	case A210_SERICONNECTADDR:
		*write_method = writeConnectAddr;
		return (u_char *)&g_connect_addr;

	case A210_SERIPORTNO:
		*write_method = writePortNo;
		return (u_char *)&g_port_no;

	case A210_SERISOCKPROTO:
		*write_method = writeSockProto;
		return (u_char *)&g_sock_proto;

	case A210_SERIBITRATE:
		*write_method = writeBaudrate;
		return (u_char *)&g_serial_setting.baudrate;

	case A210_SERIDATALEN:
		*write_method = writeDataLength;
		return (u_char *)&g_serial_setting.datalen;

	case A210_SERISTOPBIT:
		*write_method = writeStopbit;
		return (u_char *)&g_serial_setting.stop_bit;

	case A210_SERIPARITY:
		*write_method = writeParity;
		return (u_char *)&g_serial_setting.parity;

	case A210_SERIFLOWCTRL:
		*write_method = writeFlowCtrl;
		return (u_char *)&g_serial_setting.flow_ctrl;

	default:
		DEBUGMSGTL((MODULE_NAME, "invlid oid%d\n", vp->magic));
		break;
	}
	return NULL;
}


