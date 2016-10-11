/************************************************************************
 file name :
 summary   :
 coded by  :
 copyright :
************************************************************************/

#ifndef _AJ_COMMAND_H_
#define _AJ_COMMAND_H_

/*
 * difinitions
 */

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)

/*
 * function declarations 
 */
extern void init_aj_command(void);
extern FindVarMethod var_aj_command;

extern WriteMethod rebootCommand;
extern WriteMethod saveConfig;

#endif                          /* _AJ_MISC_H_ */
