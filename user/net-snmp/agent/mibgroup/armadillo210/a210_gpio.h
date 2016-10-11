#ifndef _A210_GPIO_H_
#define _A210_GPIO_H_

#define GPIO_CONFIG_FILE_WITH_FLATFSD "/etc/config/gpio.conf"
#define GPIO_CONFIG_FILE_DEFAULT      "/etc/gpio.conf"

config_require(util_funcs)
config_add_mib(ATMARKTECHNO-MIB)
config_add_mib(ARMADILLO-210-MIB)

extern void init_a210_gpio(void);
extern FindVarMethod var_a210_gpio;

#endif
