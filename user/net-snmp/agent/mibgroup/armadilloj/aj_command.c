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
#include <config/autoconf.h>

#include "util_funcs.h"
#include "aj_command.h"

oid aj_command_variables_oid[] = { 1, 3, 6, 1, 4, 1, 16031, 1, 2, 1 };

#ifdef CONFIG_USER_FLATFSD_DISABLE_USR1
#define SAVE_CONFIG          "flatfsd -s"
#else
#define SAVE_CONFIG          "killall -USR1 flatfsd"
#endif
#define REBOOT_CMD          "killall -HUP flatfsd"

// MAGIC NUMBER
#define AJREBOOTCMD	1
#define AJSAVECONFIG	2

struct variable2 aj_command_variables[] = {
    {AJREBOOTCMD, ASN_INTEGER, RWRITE, var_aj_command, 1, {1}},
//    {AJSAVECONFIG, ASN_INTEGER, RWRITE, var_aj_command, 1, {2}},
};

/******************************************************************
 * Initializes the module
 *****************************************************************/
void init_aj_command(void)
{
    REGISTER_MIB(
		"aj_command",
		aj_command_variables,
		variable2,
	    aj_command_variables_oid
		);
}

/******************************************************************
 * reboot armadillo-j
 *****************************************************************/
int rebootCommand(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	if(action == COMMIT){
		system(REBOOT_CMD);
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * save configuration to flash rom
 *****************************************************************/
int saveConfig(
	int   action,
	u_char *var_val,
	u_char var_val_type,
	size_t var_val_len,
	u_char *statP,
	oid    *name,
	size_t name_len
)
{
	if(action == COMMIT){
		system(SAVE_CONFIG);
	}

	return SNMP_ERR_NOERROR;
}

/******************************************************************
 * Hander when accessed by oid
 *****************************************************************/
unsigned char* var_aj_command(
	struct variable *vp,
	oid * name,
	size_t * length,
	int exact,
	size_t * var_len,
	WriteMethod ** write_method
)
{
    /*
     * variables we may use later 
     */
    static unsigned long long_ret;

   	DEBUGMSGTL(("aj_command", "var_aj_command()\n"));
    if(header_generic(vp, name, length, exact, var_len, write_method)
        == MATCH_FAILED)
        return NULL;

    /*
     * this is where we do the value assignments for the mib results.
     */
   	DEBUGMSGTL(("aj_command", "magic : %d\n", vp->magic));
    switch (vp->magic) {
	case AJREBOOTCMD:
		long_ret = 0;
		*write_method = rebootCommand;
		return (u_char *)&long_ret;

#if 0
	case AJSAVECONFIG:
		long_ret = 0;
		*write_method = saveConfig;
		return (u_char *)&long_ret;
#endif

	default:
    	DEBUGMSGTL(("aj_command", "invalid request received\n"));
		break;
	}

    return NULL;
}
