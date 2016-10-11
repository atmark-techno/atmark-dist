/*
 * at-cgi-core/system-config.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef ATCGICONFIG_H_
#define ATCGICONFIG_H_

#define VERSION_LEN_MAX					(63)
#define PRODUCT_NAME_LEN_MAX			(127)
#define PRIMARY_IF_LEN_MAX				(127)
#define FIRMWARE_URL_LEN_MAX			(511)
#define FIRMWARE_KERNEL_MATCH_LEN_MAX	(127)
#define FIRMWARE_USERLAND_MATCH_LEN_MAX	(127)

#define CONFIG_VAL_ROW_LEN_MAX			(255)
#define MAX_VALUES_PER_ROW				(10)

typedef struct {
	char row[CONFIG_VAL_ROW_LEN_MAX+1];
	char *values[MAX_VALUES_PER_ROW];
} configValues;

typedef struct {
	char version[VERSION_LEN_MAX+1];
	char product_name[PRODUCT_NAME_LEN_MAX+1];
	char primary_if[PRIMARY_IF_LEN_MAX+1];
	char firmware_url[FIRMWARE_URL_LEN_MAX+1];
	configValues firmware_kernel_matches;
	configValues firmware_userland_matches;
} atcgiConfig;

extern int config_load(void);
extern int config_dump(void);
extern void config_free(void);

extern char *config_get_version(void);

extern char *config_get_product_name(void);

extern char *config_get_primary_if(void);
extern void config_set_primary_if(const char *new_primary_if);

extern char *config_get_firmware_url(void);
extern void config_set_firmware_url(const char *new_firmware_url);

extern configValues *config_get_firmware_kernel_matches(void);
extern configValues *config_get_firmware_userland_matches(void);

#endif /*ATCGICONFIG_H_*/
