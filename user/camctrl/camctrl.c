/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 Atmark Techno, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <glib.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "camctrl.h"
#include "armadillo.h"
#include "ov7725.h"
#include "sccb.h"
#include "version.h"

static int camctrl_sccbbus;
static guint8 camctrl_sccbaddr;

struct camctrl_param *camctrl_params;

static gboolean opt_version = FALSE;
static gboolean opt_get = FALSE;
static gboolean opt_set = FALSE;

static GOptionEntry opt_entries[] = {
	{ "version", 0, 0, G_OPTION_ARG_NONE, &opt_version,
	  "Print version", NULL },
	{ "get", 0, 0, G_OPTION_ARG_NONE, &opt_get,
	  "Select getter operation [default]", NULL },
	{ "set", 0, 0, G_OPTION_ARG_NONE, &opt_set,
	  "Select setter operation", NULL },
	{ NULL },
};

static gboolean opt_all = FALSE;
static gboolean opt_gain = FALSE;
static gboolean opt_exposure = FALSE;
static gboolean opt_red = FALSE;
static gboolean opt_green = FALSE;
static gboolean opt_blue = FALSE;
static gboolean opt_agc = FALSE; /* Automatic Gain Control */
static gboolean opt_aec = FALSE; /* Automatic Exposure Control */
static gboolean opt_awb = FALSE; /* Automatic White Balance */
static gboolean opt_colorbar = FALSE;
static gboolean opt_dsp_colorbar = FALSE;
static gint opt_register_address = -1;

#define DESC_ALL	"All parameters"
#define DESC_GAIN	"Gain value"
#define DESC_EXPOSURE	"Exposure value"
#define DESC_RED	"Red channel gain value"
#define DESC_GREEN	"Green channel gain value"
#define DESC_BLUE	"Blue channel gain value"
#define DESC_AGC	"Automatic Gain Control status"
#define DESC_AEC	"Automatic Exposure Control"
#define DESC_AWB	"Automatic White Balance"
#define DESC_COLORBAR	"Colorbar test pattem output status"
#define DESC_DCOLORBAR	"DSP colorbar generation status"
#define DESC_REG_ADDR	"Register address(for debug)"
#define DESC_REG_VAL	"Register value(for debug)"

static GOptionEntry opt_entries_get[] = {
	{ "all", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_all, DESC_ALL, NULL },
	{ "gain", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_gain, DESC_GAIN, NULL },
	{ "exposure", 'e', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_exposure, DESC_EXPOSURE, NULL },
	{ "red", 'r', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_red, DESC_RED, NULL },
	{ "green", 'g', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_green, DESC_GREEN, NULL },
	{ "blue", 'b', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_blue, DESC_BLUE, NULL },
	{ "auto-gain", 'G', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_agc, DESC_AGC, NULL },
	{ "auto-exposure", 'E', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_aec, DESC_AEC, NULL },
	{ "auto-white-balance", 'W', G_OPTION_FLAG_NOALIAS,
	  G_OPTION_ARG_NONE, &opt_awb, DESC_AWB, NULL },
	{ "colorbar", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_colorbar, DESC_COLORBAR, NULL },
	{ "dsp-colorbar", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_NONE,
	  &opt_dsp_colorbar, DESC_DCOLORBAR, NULL },
	{ "address", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_register_address, DESC_REG_ADDR, "<address>"},
	{ NULL },
};

static gint opt_gain_value = -1;
static gint opt_exposure_value = -1;
static gint opt_red_value = -1;
static gint opt_green_value = -1;
static gint opt_blue_value = -1;

static gchar *opt_agc_status;
static gchar *opt_aec_status;
static gchar *opt_awb_status;

static gchar *opt_colorbar_status;
static gchar *opt_dsp_colorbar_status;

static gint opt_register_value = -1;

static GOptionEntry opt_entries_set[] = {
	{ "gain", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_gain_value, DESC_GAIN" [16 .. 496]", "<value>" },
	{ "exposure", 'e', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_exposure_value, DESC_EXPOSURE" [0 .. 1081217 us]",
	  "<value>" },
	{ "red", 'r', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_red_value, DESC_RED" [128 .. 510]", "<value>" },
	{ "green", 'g', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_green_value, DESC_GREEN" [128 .. 510]", "<value>" },
	{ "blue", 'b', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_blue_value, DESC_BLUE" [128 .. 510]", "<value>" },
	{ "auto-gain", 'G', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_STRING,
	  &opt_agc_status,  DESC_AGC, "<on|off>" },
	{ "auto-exposure", 'E', G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_STRING,
	  &opt_aec_status, DESC_AEC, "<on|off>" },
	{ "auto-white-balance", 'W', G_OPTION_FLAG_NOALIAS,
	  G_OPTION_ARG_STRING, &opt_awb_status, DESC_AWB, "<on|off>" },
	{ "colorbar", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_STRING,
	  &opt_colorbar_status, DESC_COLORBAR, "<on|off>" },
	{ "dsp-colorbar", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_STRING,
	  &opt_dsp_colorbar_status, DESC_DCOLORBAR, "<on|off>" },
	{ "address", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_register_address, DESC_REG_ADDR, "<address>"},
	{ "value", 0, G_OPTION_FLAG_NOALIAS, G_OPTION_ARG_INT,
	  &opt_register_value, DESC_REG_VAL, "<value>"},
	{ NULL },
};

static gboolean is_valid_param(gint param)
{
	gint *daddr;
	int i;

	if ((param < 0) || (NR_PARAMS <= param))
		return FALSE;

	daddr = camctrl_params[param].daddr;
	for (i = 0; i < (SCCB_DATA_BYTES + 1); i++) {
		if (daddr[i] == CAMCTRL_PARAM_DADDR_TERM)
			return TRUE;
	}

	return FALSE;
}

static gboolean store_status(gint param, gchar *status)
{
	struct camctrl_param *p;
	struct sccb_data dat;
	gboolean ret;

	ret = is_valid_param(param);
	if (ret == FALSE) {
		g_print("unsupported\n");
		return TRUE;
	}

	p = &camctrl_params[param];
	memcpy(&dat.daddr, p->daddr, sizeof(gint [SCCB_DATA_BYTES + 1]));

	ret = sccb_read(&dat);
	if (ret == FALSE)
		return FALSE;

	if (g_strcmp0(status, "on") == 0)
		dat.val |= p->mask;
	else if (g_strcmp0(status, "off") == 0)
		dat.val &= ~p->mask;
	else {
		g_printerr("invalid status: %s\n", status);
		return FALSE;
	}

	ret = sccb_write(&dat);
	if (ret == FALSE)
		return FALSE;

	return TRUE;
}

static gboolean show_status(gint param, gchar *prefix)
{
	struct camctrl_param *p;
	struct sccb_data dat;
	gboolean ret;

	ret = is_valid_param(param);
	if (ret == FALSE) {
		g_print("unsupported\n");
		return TRUE;
	}

	p = &camctrl_params[param];
	memcpy(&dat.daddr, p->daddr, sizeof(gint [SCCB_DATA_BYTES + 1]));

	ret = sccb_read(&dat);
	if (ret == FALSE)
		return FALSE;
	g_print("%s%s\n", prefix, (dat.val & p->mask) ? "on" : "off");

	return TRUE;
}

static gboolean store_value(gint param, gint value)
{
	struct camctrl_param *p;
	struct sccb_data dat;
	guint32 val;
	gboolean ret;

	ret = is_valid_param(param);
	if (ret == FALSE) {
		g_print("unsupported\n");
		return TRUE;
	}

	p = &camctrl_params[param];
	memcpy(&dat.daddr, p->daddr, sizeof(gint [SCCB_DATA_BYTES + 1]));

	val = value;
	if (p->arg2val)
		val = p->arg2val(value);

	ret = sccb_read(&dat);
	if (ret == FALSE)
		return FALSE;

	dat.val &= ~p->mask;
	dat.val |= (val & p->mask);

	ret = sccb_write(&dat);
	if (ret == FALSE)
		return FALSE;

	return TRUE;
}

static gboolean show_value(gint param, gchar *prefix)
{
	struct camctrl_param *p;
	struct sccb_data dat;
	guint32 val;
	gboolean ret;

	ret = is_valid_param(param);
	if (ret == FALSE) {
		g_print("unsupported\n");
		return TRUE;
	}

	p = &camctrl_params[param];
	memcpy(&dat.daddr, p->daddr, sizeof(gint [SCCB_DATA_BYTES + 1]));

	ret = sccb_read(&dat);
	if (ret == FALSE)
		return FALSE;

	val = dat.val;
	if (p->val2arg)
		val = p->val2arg(val);
	g_print("%s%d\n", prefix, val);

	return TRUE;
}

static inline gboolean show_register_value(void)
{
	struct sccb_data data;
	gboolean ret;

	data.daddr[0] = (opt_register_address & 0xff);
	data.daddr[1] = CAMCTRL_PARAM_DADDR_TERM;

	ret = sccb_read(&data);
	if (ret == FALSE)
		return FALSE;
	g_print("%d\n", data.val & 0xff);

	return TRUE;
}

static inline gboolean store_register_value(void)
{
	struct sccb_data data;
	gboolean ret;

	data.daddr[0] = (opt_register_address & 0xff);
	data.daddr[1] = CAMCTRL_PARAM_DADDR_TERM;

	data.val = opt_register_value;
	ret = sccb_write(&data);
	if (ret == FALSE)
		return FALSE;

	return TRUE;
}

static gboolean show_all(void)
{
	gboolean ret;

	ret = show_value(GAIN,                "                gain: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_value(EXPOSURE,            "            exposure: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_value(GAIN_RED,            "                 red: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_value(GAIN_GREEN,          "               green: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_value(GAIN_BLUE,           "                blue: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_status(AUTO_GAIN,          "           auto-gain: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_status(AUTO_EXPOSURE,      "       auto-exposure: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_status(AUTO_WHITE_BALANCE, "  auto-white-balance: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_status(COLORBAR,           "            colorbar: ");
	if (ret == FALSE)
		return FALSE;
	ret = show_status(DSP_COLORBAR,       "        dsp-colorbar: ");
	if (ret == FALSE)
		return FALSE;

	return TRUE;
}

static void show_version(void)
{
	g_printf("camctrl: version %s\n", CAMCTRL_VERSION);
}

static gboolean handle_get_option(GOptionContext *optctx, int argc,
				  char *argv[])
{
	GOptionGroup *optgrp;
	GError *error = NULL;
	gboolean ret;

	g_option_context_set_help_enabled(optctx, TRUE);

	optgrp = g_option_group_new("get", "Get Options:",
				    "Show getter help options", NULL, NULL);
	g_option_group_add_entries(optgrp, opt_entries_get);
	g_option_context_add_group(optctx, optgrp);

	optgrp = g_option_group_new("set", "Set Options:",
				    "Show setter help options", NULL, NULL);
	g_option_group_add_entries(optgrp, opt_entries_set);
	g_option_context_add_group(optctx, optgrp);

	ret = g_option_context_parse(optctx, &argc, &argv, &error);
	if (ret == FALSE) {
		g_printerr("Error parsing options: %s\n", error->message);
		return FALSE;
	}

	if (opt_all)
		return show_all();

	if (opt_agc) {
		ret = show_status(AUTO_GAIN, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_aec) {
		ret = show_status(AUTO_EXPOSURE, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_awb) {
		ret = show_status(AUTO_WHITE_BALANCE, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_gain) {
		ret = show_value(GAIN, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_exposure) {
		ret = show_status(EXPOSURE, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_red) {
		ret = show_value(GAIN_RED, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_green) {
		ret = show_value(GAIN_GREEN, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_blue) {
		ret = show_value(GAIN_BLUE, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_colorbar) {
		ret = show_status(COLORBAR, "");
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_dsp_colorbar) {
		ret = show_status(DSP_COLORBAR, "");
		if (ret == FALSE)
			return FALSE;
	}

	if (opt_register_address > 0) {
		ret = show_register_value();
		if (ret == FALSE)
			return FALSE;
	}

	return TRUE;
}

static gboolean handle_set_option(GOptionContext *optctx, int argc,
				  char *argv[])
{
	GOptionGroup *optgrp;
	GError *error = NULL;
	gboolean ret;

	optgrp = g_option_group_new("set", "Set Options:",
				    "Show setter help options", NULL, NULL);
	g_option_group_add_entries(optgrp, opt_entries_set);
	g_option_context_add_group(optctx, optgrp);

	ret = g_option_context_parse(optctx, &argc, &argv, &error);
	if (ret == FALSE) {
		g_printerr("Error parsing options: %s\n", error->message);
		return FALSE;
	}

	if (opt_agc_status && (opt_gain_value >= 0)) {
		g_printerr("`--auto-gain' and `--gain' are exclusive\n");
		return FALSE;
	}

	if (opt_aec_status && (opt_exposure_value >= 0)) {
		g_printerr("`--auto-exposure' and "
			   "`--exposure' are exclusive\n");
		return FALSE;
	}

	if (opt_awb_status) {
		if (opt_red_value >= 0) {
			g_printerr("`--auto-white-blance' and "
				   "`--red' are exclusive\n");
			return FALSE;
		}
		if (opt_green_value >= 0) {
			g_printerr("`--auto-white-blance' and "
				   "`--green' are exclusive\n");
			return FALSE;
		}
		if (opt_blue_value >= 0) {
			g_printerr("`--auto-white-blance' and "
				   "`--blue' are exclusive\n");
			return FALSE;
		}
	}

	if (opt_agc_status) {
		ret = store_status(AUTO_GAIN, opt_agc_status);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_aec_status) {
		ret = store_status(AUTO_EXPOSURE, opt_aec_status);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_awb_status) {
		ret = store_status(AUTO_WHITE_BALANCE, opt_awb_status);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_gain_value >= 0) {
		ret = store_status(AUTO_GAIN, "off");
		if (ret == FALSE)
			return FALSE;
		ret = store_value(GAIN, opt_gain_value);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_exposure_value >= 0) {
		ret = store_status(AUTO_EXPOSURE, "off");
		if (ret == FALSE)
			return FALSE;
		ret = store_value(EXPOSURE, opt_exposure_value);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_red_value >= 0) {
		ret = store_status(AUTO_WHITE_BALANCE, "off");
		if (ret == FALSE)
			return FALSE;
		ret = store_value(GAIN_RED, opt_red_value);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_green_value >= 0) {
		ret = store_status(AUTO_WHITE_BALANCE, "off");
		if (ret == FALSE)
			return FALSE;
		ret = store_value(GAIN_GREEN, opt_green_value);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_blue_value >= 0) {
		ret = store_status(AUTO_WHITE_BALANCE, "off");
		if (ret == FALSE)
			return FALSE;
		ret = store_value(GAIN_BLUE, opt_blue_value);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_colorbar_status) {
		ret = store_status(COLORBAR, opt_colorbar_status);
		if (ret == FALSE)
			return FALSE;
	}
	if (opt_dsp_colorbar_status) {
		ret = store_status(DSP_COLORBAR, opt_dsp_colorbar_status);
		if (ret == FALSE)
			return FALSE;
	}

	if (((opt_register_address >= 0) && (opt_register_value < 0)) ||
	    ((opt_register_address < 0) && (opt_register_value >= 0))) {
		g_printerr("`--address' and `--value' are relative\n");
		return FALSE;
	}

	if ((opt_register_address >= 0) && (opt_register_value >= 0)) {
		store_register_value();
		if (ret == FALSE)
			return FALSE;
	}

	return TRUE;
}

static gboolean camctrl_init(void)
{
	gboolean ret;

	camctrl_sccbbus = ARMADILLO_SCCB_BUS;
	camctrl_sccbaddr = ARMADILLO_SCCB_ADDR;

	ret = sccb_open(camctrl_sccbbus, camctrl_sccbaddr);
	if (ret == FALSE)
		return FALSE;

	camctrl_params = ov7725_params();

	return TRUE;
}

static gboolean camctrl_exit(void)
{
	return sccb_close();
}

static void usage(GOptionContext *optctx)
{
	gchar *help;
	GOptionGroup *optgrp;

	optgrp = g_option_group_new("set", "Set Options:",
				    "Show setter help options", NULL, NULL);
	g_option_group_add_entries(optgrp, opt_entries_set);
	g_option_context_add_group(optctx, optgrp);

	optgrp = g_option_group_new("get", "Get Options:",
				    "Show getter help options", NULL, NULL);
	g_option_group_add_entries(optgrp, opt_entries_set);
	g_option_context_add_group(optctx, optgrp);

	help = g_option_context_get_help(optctx, FALSE, NULL);
	g_printerr("%s", help);
	g_free(help);
}

int main(int argc, char *argv[])
{
	GOptionContext *optctx;
	GError *error = NULL;
	gboolean ret;

	optctx = g_option_context_new("-- camera control application");
	g_option_context_set_help_enabled(optctx, FALSE);
	g_option_context_set_ignore_unknown_options(optctx, TRUE);

	g_option_context_add_main_entries(optctx, opt_entries, NULL);

	ret = g_option_context_parse(optctx, &argc, &argv, &error);
	if (ret == FALSE) {
		g_printerr("Error parsing options: %s\n", error->message);
		return EXIT_FAILURE;
	}

	if (opt_version) {
		show_version();
		return EXIT_SUCCESS;
	}

	if (opt_get && opt_set) {
		g_printerr("`--get' and `--set' are exclusive\n");
		usage(optctx);
		return EXIT_FAILURE;
	}

	ret = camctrl_init();
	if (ret == FALSE)
		return EXIT_FAILURE;

	if (opt_set)
		ret = handle_set_option(optctx, argc, argv);
	else
		ret = handle_get_option(optctx, argc, argv);

	camctrl_exit();

	if (ret == FALSE)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
