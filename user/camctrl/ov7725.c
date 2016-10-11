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
#include "camctrl.h"
#include "armadillo.h"
#include "ov7725.h"

#define bitcount8(b)			\
      (	(!!((b) & (1ULL << 0))) +	\
	(!!((b) & (1ULL << 1))) +	\
	(!!((b) & (1ULL << 2))) +	\
	(!!((b) & (1ULL << 3))) +	\
	(!!((b) & (1ULL << 4))) +	\
	(!!((b) & (1ULL << 5))) +	\
	(!!((b) & (1ULL << 6))) +	\
	(!!((b) & (1ULL << 7)))	)

#define GAIN_ARG_MIN (16)
static guint32 ov7725_gain_argmin(void)
{
	return GAIN_ARG_MIN;
}

#define GAIN_ARG_MAX (496)
static guint32 ov7725_gain_argmax(void)
{
	return GAIN_ARG_MAX;
}

static guint32 ov7725_gain_arg2val(guint32 arg)
{
	guint8 hval, lval, tmp;

	if (arg >= ov7725_gain_argmax())
		return 0xff;
	else if (arg >= 252)
		hval = 0xf0;
	else if (arg >= 126)
		hval = 0x70;
	else if (arg >= 63)
		hval = 0x30;
	else if (arg >= 32)
		hval = 0x10;
	else if (arg >= ov7725_gain_argmin())
		hval = 0x0;
	else
		return 0x00;

	tmp = (1 << bitcount8(hval));
	lval = ((arg + ((arg % tmp + 1) / 2)) / tmp);
	if (lval < 16)
		lval = 0;
	else
		lval -= 16;

	return (hval | lval);
}

static guint32 ov7725_gain_val2arg(guint32 val)
{
	guint8 hval, lval;

	hval = val & 0xf0;
	lval = val & 0x0f;

	return ((1 << bitcount8(hval)) * (lval + 16));
}

static gint32 ov7725_get_reg(gint daddr)
{
	struct sccb_data dat;
	gboolean ret;

	dat.daddr[0] = daddr;
	dat.daddr[1] = CAMCTRL_PARAM_DADDR_TERM;

	ret = sccb_read(&dat);
	if (ret == FALSE)
		return -1;

	return dat.val;
}

static void ov7725_set_reg(gint daddr, guint8 val)
{
	struct sccb_data dat;

	dat.daddr[0] = daddr;
	dat.daddr[1] = CAMCTRL_PARAM_DADDR_TERM;

	dat.val = val;

	sccb_write(&dat);
}

#define GAIN_RGB_ARG_MIN    (128)
#define GAIN_RGB_ARG_MAX_2x (255)
#define GAIN_RGB_ARG_MAX_4x (510)
static gboolean ov7725_is_max_gain_4x()
{
	guint32 dat;

	dat = ov7725_get_reg(OV7725_AWB_CTRL1);
	if (dat == -1)
		dat = 0;

	if (dat & OV7725_AWB_CTRL1_GAIN_4x)
		return TRUE;

	return FALSE;
}

static void ov7725_max_gain_4x_enable(gboolean enable)
{
	guint32 dat;

	dat = ov7725_get_reg(OV7725_AWB_CTRL1);
	if (dat == -1)
		return;

	if (enable)
		dat |= OV7725_AWB_CTRL1_GAIN_4x;
	else
		dat &= ~OV7725_AWB_CTRL1_GAIN_4x;

	ov7725_set_reg(OV7725_AWB_CTRL1, dat & 0xff);
}

static guint32 ov7725_gain_rgb_arg2val(guint32 val)
{
	guint32 div;

	if (val < GAIN_RGB_ARG_MIN)
		val = GAIN_RGB_ARG_MIN;
	if (val > GAIN_RGB_ARG_MAX_4x)
		val = GAIN_RGB_ARG_MAX_4x;

	if (val > GAIN_RGB_ARG_MAX_2x) {
		ov7725_max_gain_4x_enable(TRUE);
		div = 0x40;
	} else {
		ov7725_max_gain_4x_enable(FALSE);
		div = 0x80;
	}
		
	return (val / (0x80 / div));
}

static guint32 ov7725_gain_rgb_val2arg(guint32 val)
{
	guint32 div;
	guint32 gain_4x;

	gain_4x = ov7725_is_max_gain_4x();
	if (gain_4x)
		div = 0x40;
	else
		div = 0x80;

	return ((val & 0xff) * (0x80 / div));
}

static guint32 ov7725_exposure_arg2val(guint32 val)
{
	guint32 dat;
	unsigned long long pll;
	unsigned long long tline;
	unsigned long long iclk;	/* kHz */

	dat = ov7725_get_reg(OV7725_COM4);
	switch(dat & OV7725_COM4_PLL_MASK) {
	case OV7725_COM4_PLL_BYPASS:
		pll = 1;
		break;
	case OV7725_COM4_PLL_6x:
		pll = 6;
		break;
	case OV7725_COM4_PLL_8x:
		pll = 8;
		break;
	default:
		pll = 4;
	}

	iclk = ARMADILLO_CAM_CLK * pll / 1000;

	dat = ov7725_get_reg(OV7725_COM7);
	if (dat & OV7725_COM7SLCT_QVGA)
		tline = OV7725_T_LINE_QVGA;
	else
		tline = OV7725_T_LINE_VGA;

	return (guint32)((val * iclk) / (tline * 1000ULL));
}

static guint32 ov7725_exposure_val2arg(guint32 val)
{
	guint32 dat;
	unsigned long long pll;
	unsigned long long tline;
	unsigned long long iclk;	/* kHz */

	dat = ov7725_get_reg(OV7725_COM4);
	switch(dat & OV7725_COM4_PLL_MASK) {
	case OV7725_COM4_PLL_BYPASS:
		pll = 1;
		break;
	case OV7725_COM4_PLL_6x:
		pll = 6;
		break;
	case OV7725_COM4_PLL_8x:
		pll = 8;
		break;
	default:
		pll = 4;
	}

	iclk = ARMADILLO_CAM_CLK * pll / 1000;

	dat = ov7725_get_reg(OV7725_COM7);
	if (dat & OV7725_COM7SLCT_QVGA)
		tline = OV7725_T_LINE_QVGA;
	else
		tline = OV7725_T_LINE_VGA;

	return (guint32)(val * tline * 1000ULL / iclk);	/* us */
}

static struct camctrl_param ov7725_camctrl_params[NR_PARAMS] = {
	[AUTO_GAIN] = {
		.daddr = {OV7725_COM8, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_COM8_AGC_ON,
	},
	[GAIN] = {
		.daddr = {OV7725_GAIN, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_GAIN_MASK,
		.arg2val = ov7725_gain_arg2val,
		.val2arg = ov7725_gain_val2arg,
	},
	[GAIN_BLUE] = {
		.daddr = {OV7725_BLUE, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_BLUE_MASK,
		.arg2val = ov7725_gain_rgb_arg2val,
		.val2arg = ov7725_gain_rgb_val2arg,
	},
	[GAIN_RED] = {
		.daddr = {OV7725_RED, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_RED_MASK,
		.arg2val = ov7725_gain_rgb_arg2val,
		.val2arg = ov7725_gain_rgb_val2arg,
	},
	[GAIN_GREEN] = {
		.daddr = {OV7725_GREEN, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_GREEN_MASK,
		.arg2val = ov7725_gain_rgb_arg2val,
		.val2arg = ov7725_gain_rgb_val2arg,
	},
	[AUTO_EXPOSURE] = {
		.daddr = {OV7725_COM8, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_COM8_AEC_ON,
	},
	[EXPOSURE] = {
		.daddr = {OV7725_AEC, OV7725_AECH, CAMCTRL_PARAM_DADDR_TERM},
		.mask = ((OV7725_AECH_MASK << 8) | OV7725_AEC_MASK),
		.arg2val = ov7725_exposure_arg2val,
		.val2arg = ov7725_exposure_val2arg,
	},
	[AUTO_WHITE_BALANCE] = {
		.daddr = {OV7725_COM8, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_COM8_AWB_ON,
	},
	[COLORBAR] = {
		.daddr = {OV7725_COM3, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_COM3SCOLOR_TEST,
	},
	[DSP_COLORBAR] = {
		.daddr = {OV7725_DSP_CTRL3, CAMCTRL_PARAM_DADDR_TERM},
		.mask = OV7725_CBAR_MASK,
	},
};

struct camctrl_param *ov7725_params(void)
{
	return ov7725_camctrl_params;
}
