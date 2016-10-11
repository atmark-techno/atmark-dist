/*
 * at-cgi-core/system-config.c- config handling for at-cgi
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 * Author: Chris McHarg <chris (at) atmark-techno.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>

#include <simple-cgi-app-io.h>

#include "core.h"
#include "misc-utils.h"

#include "system-config.h"

#define ATCGI_CONFIG_PATH		"/etc/config/at-admin"
#define SUDO_PATH			"/usr/bin/sudo"

#define CONFIG_LINE_LEN_MAX		(1024)

static atcgiConfig *atcgi_config;

static void row_to_config_values(configValues *config_values, char *row)
{
	int val_count = 0;
	char *val_start;
	char *val_end;

	strncpy(config_values->row, row, CONFIG_VAL_ROW_LEN_MAX);
	val_start = config_values->row;

	while (val_start) {

		config_values->values[val_count] = val_start;
		
		if ((val_end = strchr(val_start, ','))) {
			*val_end++ = '\0';
			while (*val_end == ' ') {
				*val_end++ = '\0';
			}
		}

		val_count++;
		if (val_count >= MAX_VALUES_PER_ROW) {
			return;
		}
		val_start = val_end;

	}
}

static void fprint_config_values(FILE *fp, configValues *config_values)
{
	int i;

	fprintf(fp, "%s", config_values->values[0]);

	for (i = 1; config_values->values[i]; i++) {
		fprintf(fp, ", %s", config_values->values[i]);

	}
	
	fprintf(fp, "\"\n");
}

int config_load(void)
{
	FILE *in_fp;
	char buffer[CONFIG_LINE_LEN_MAX];
	char *key, *value, *end;
	int not_found_flag = 0, len;

	atcgi_config = calloc(sizeof(atcgiConfig), 1);
	if (!atcgi_config) {
		syslog(LOG_ERR, "at-cgi: malloc for config failed");
		return -1;
	}

	in_fp = fopen(ATCGI_CONFIG_PATH, "r");
	if (in_fp == NULL) {
		free(atcgi_config);
		syslog(LOG_ERR, "at-cgi: failed to open config file");
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), in_fp) != NULL) {

		if (buffer[0] == '#' || buffer[0] == ' ') {
			continue;
		}

		value = strstr(buffer, "=\"");
		if (value == NULL) {
			continue;
		}
		*value = '\0';
		value += 2;
		end = strstr(value, "\"");
		if (value == NULL) {
			continue;
		}
		*end = '\0';
		key = buffer;

		if (strcmp(key, "VERSION") == 0) {
			strncpy(atcgi_config->version, value, VERSION_LEN_MAX);
			atcgi_config->version[VERSION_LEN_MAX] = '\0';
		} else if (strcmp(key, "PRODUCT_NAME") == 0) {
			strncpy(atcgi_config->product_name, value,
				PRODUCT_NAME_LEN_MAX);
			atcgi_config->product_name[PRODUCT_NAME_LEN_MAX] =
			  '\0';
		} else  if (strcmp(key, "PRIMARY_IF") == 0) {
			strncpy(atcgi_config->primary_if, value,
				PRIMARY_IF_LEN_MAX);
			atcgi_config->primary_if[PRIMARY_IF_LEN_MAX] = '\0';
		} else  if (strcmp(key, "FIRMWARE_URL") == 0) {
			len = strlen(value);
			if (len < FIRMWARE_URL_LEN_MAX) {
			  memcpy(atcgi_config->firmware_url, value, len + 1);
			  if (value[len - 1] != '/' &&
			      len < FIRMWARE_URL_LEN_MAX) {
			    atcgi_config->firmware_url[len] = '/';
			    atcgi_config->firmware_url[len + 1] = '\0';
			  }
			}
		} else  if (strcmp(key, "FIRMWARE_KERNEL_MATCH") == 0) {
			row_to_config_values(&atcgi_config->firmware_kernel_matches, value);
		} else  if (strcmp(key, "FIRMWARE_USERLAND_MATCH") == 0) {
			row_to_config_values(&atcgi_config->firmware_userland_matches, value);
		}
	}

	if (atcgi_config->version[0] == '\0') {
		not_found_flag++;
	} else if (atcgi_config->product_name[0] == '\0') {
		not_found_flag++;
	} else if (atcgi_config->primary_if[0] == '\0') {
		not_found_flag++;
	} else if (atcgi_config->firmware_url[0] == '\0') {
		not_found_flag++;
	} else if (atcgi_config->firmware_kernel_matches.row[0] == '\0') {
		not_found_flag++;
	} else if (atcgi_config->firmware_userland_matches.row[0] == '\0') {
		not_found_flag++;
	}

	if (not_found_flag) {
		free(atcgi_config);
		syslog(LOG_ERR, "at-cgi: failed to get all options");
		return -1;
	}

	fclose(in_fp);
	return 0;
}

int config_dump(void)
{
	int ret, temp_fd;
	FILE *temp_fp;
	char template[] = "atcgi_conf_tmp_XXXXXX";
	char *copy_args[] = {SUDO_PATH, CP_PATH, template, ATCGI_CONFIG_PATH, NULL};

	temp_fd = mkstemp(template);
	temp_fp = fdopen(temp_fd, "w");
	if (temp_fp == NULL) {
		syslog(LOG_ERR, "at-cgi: failed to open temp file");
		return -1;
	}

	fprintf(temp_fp, "VERSION=\"%s\"\n", atcgi_config->version);
	fprintf(temp_fp, "PRODUCT_NAME=\"%s\"\n", atcgi_config->product_name);
	fprintf(temp_fp, "PRIMARY_IF=\"%s\"\n", atcgi_config->primary_if);
	fprintf(temp_fp, "FIRMWARE_URL=\"%s\"\n", atcgi_config->firmware_url);
	fprintf(temp_fp, "FIRMWARE_KERNEL_MATCH=\"");
	fprint_config_values(temp_fp, &atcgi_config->firmware_kernel_matches);
	fprintf(temp_fp, "FIRMWARE_USERLAND_MATCH=\"");
	fprint_config_values(temp_fp, &atcgi_config->firmware_userland_matches);
	fprintf(temp_fp, "\n");
	fclose(temp_fp);

	ret = cgi_exec(copy_args[0], copy_args);
	if (ret < 0) {
		syslog(LOG_ERR, "at-cgi: failed to copy temp file");
		return -1;
	}
	unlink(template);

	return 0;
}

void config_free(void)
{
	free(atcgi_config);
}

char *config_get_version(void)
{
	return atcgi_config->version;
}

char *config_get_primary_if(void)
{
	return atcgi_config->primary_if;
}

void config_set_primary_if(const char *new_primary_if)
{
	strncpy(atcgi_config->primary_if, new_primary_if, PRIMARY_IF_LEN_MAX);
}

char *config_get_firmware_url(void)
{
	return atcgi_config->firmware_url;
}

void config_set_firmware_url(const char *new_firmware_url)
{
	strncpy(atcgi_config->firmware_url, new_firmware_url, FIRMWARE_URL_LEN_MAX);
}

char *config_get_product_name(void)
{
	return atcgi_config->product_name;
}

configValues *config_get_firmware_kernel_matches(void)
{
	return &atcgi_config->firmware_kernel_matches;
}

configValues *config_get_firmware_userland_matches(void)
{
	return &atcgi_config->firmware_userland_matches;
}
