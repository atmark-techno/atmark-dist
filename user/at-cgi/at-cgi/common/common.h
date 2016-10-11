/*
 * at-cgi-core/common.h
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#ifndef COMMON_H_
#define COMMON_H_

#define UNSAVED_SETTINGS_FLAG_PATH		"/tmp/at-admin/unsavedsettings"
#define UPDATED_FW_FLAG_PATH			"/tmp/at-admin/updatedfirmware"
#define UPDATED_SETTINGS_INIT_FLAG_PATH	"/tmp/at-admin/settingsinited"
#define FLAG_CHECKED					"checked"
#define FLAG_NOTCHECKED					"notyetchecked"
#define FLAG_WRONG_PRODUCT				"wrong_product"

#define ACTION_SETTINGS_DETAILS		"settings_details"
#define ACTION_SETTINGS_PATH		"/admin/system.cgi"

#define LOCAL_HANDLING_NOTREQUIRED	(0)
#define LOCAL_HANDLING_REQUIRED		(1)

extern void print_system_messages(void);
extern void report_error(const char *user_error_msg, const char *error_details);
extern void return_crit_error(const char *user_error_msg, const char *error_details);
extern void print_top_info_message(const char *message);

extern int flag_exists(char *path, const char *state);
extern int update_unsaved_settings_state(char *path);

extern int system_wide_handling(void);
extern int set_flag(char *path);

#endif /*COMMON_H_*/
