#ifndef _AJ_GPIO_H_
#define _AJ_GPIO_H_

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)

extern void init_aj_gpio(void);
extern FindVarMethod var_aj_gpio;

#endif
