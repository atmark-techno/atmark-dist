/************************************************************************
 file name :
 summary   :
 coded by  :
 copyright :
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

#include <sys/reboot.h>
#include <linux/reboot.h>

#include "util_funcs.h"
#include "a210_common.h"
#include "a210_command.h"

#define MODULE_NAME "a210_command"

// MAGIC NUMBER
#define A210_REBOOT		1
#define A210_FLATFSD_SAVE	2

oid a210_command_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 4, 1 };

struct variable2 a210_command_variables[] = {
	{A210_REBOOT, ASN_INTEGER, RONLY, var_a210_command, 1, {1}},
	{A210_FLATFSD_SAVE, ASN_INTEGER, RONLY, var_a210_command, 1, {2}},
};

/******************************************************************
 * Initializes the module
 *****************************************************************/
void init_a210_command(void)
{
	DEBUGMSGTL((MODULE_NAME, "%s()\n", __FUNCTION__));
	REGISTER_MIB(
		     MODULE_NAME,
		     a210_command_variables,
		     variable2,
		     a210_command_variables_oid
		     );
}

/******************************************************************
 * reboot
 *****************************************************************/
int rebootCommand(
		  int    action,
		  u_char *var_val,
		  u_char var_val_type,
		  size_t var_val_len,
		  u_char *statP,
		  oid    *name,
		  size_t name_len
		  )
{
	if(action == COMMIT){
		system_reboot(REBOOT_RESTART);
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * save configuration to flash rom
 *****************************************************************/
int saveConfig(
	       int    action,
	       u_char *var_val,
	       u_char var_val_type,
	       size_t var_val_len,
	       u_char *statP,
	       oid    *name,
	       size_t name_len
	       )
{
	if(action == COMMIT){
		system_save_config();
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * Handler when accessed by oid
 *****************************************************************/
unsigned char* var_a210_command(
			      struct variable *vp,
			      oid             *name,
			      size_t          *length,
			      int             exact,
			      size_t          *var_len,
			      WriteMethod     **write_method
			      )
{
	/*
	 * variables we may use later 
	 */
	static unsigned long long_ret;
	int ret;

   	DEBUGMSGTL((MODULE_NAME, "var_a210_command()\n"));
	ret = header_generic(vp, name, length, exact, var_len, write_method);
	if(ret == MATCH_FAILED) return NULL;

	/*
	 * this is where we do the value assignments for the mib results.
	 */
   	DEBUGMSGTL((MODULE_NAME, "magic : %d\n", vp->magic));
	switch (vp->magic) {
	case A210_REBOOT:
		long_ret = 0;
		*write_method = rebootCommand;
		return (u_char *)&long_ret;

	case A210_FLATFSD_SAVE:
		long_ret = 0;
		*write_method = saveConfig;
		return (u_char *)&long_ret;

	default:
		DEBUGMSGTL((MODULE_NAME, "invalid request received\n"));
		break;
	}

	return NULL;
}
