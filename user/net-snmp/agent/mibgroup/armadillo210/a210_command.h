/************************************************************************
 file name :
 summary   :
 coded by  :
 copyright :
************************************************************************/

#ifndef _A210_COMMAND_H_
#define _A210_COMMAND_H_

/*
 * difinitions
 */

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)
config_add_mib(ARMADILLO-210-MIB)

/*
 * function declarations 
 */
extern void init_a210_command(void);
extern FindVarMethod var_a210_command;

extern WriteMethod rebootCommand;
extern WriteMethod saveConfig;

#endif
