#ifndef _A210_MISC_H_
#define _A210_MISC_H_

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)
config_add_mib(ARMADILLO-210-MIB)

extern void init_a210_misc(void);
extern FindVarMethod var_a210_misc;

#endif
