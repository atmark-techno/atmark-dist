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

#ifndef _CAMCTRL_OV7725_H
#define _CAMCTRL_OV7725_H

#include "camctrl.h"

#define OV7725_GAIN        (0x00) /* AGC - Gain control gain setting */
#define		OV7725_GAIN_MASK	(0xff)
#define OV7725_BLUE        (0x01) /* AWB - Blue channel gain setting */
#define		OV7725_BLUE_MASK	(0xff)
#define OV7725_RED         (0x02) /* AWB - Red   channel gain setting */
#define		OV7725_RED_MASK		(0xff)
#define OV7725_GREEN       (0x03) /* AWB - Green channel gain setting */
#define		OV7725_GREEN_MASK	(0xff)
#define OV7725_AECH        (0x08) /* Exposure Value - AEC MSBs */
#define		OV7725_AECH_MASK	(0xff)
#define OV7725_COM3        (0x0c) /* Common control 3 */
		/* Sensor color bar test pattern */
#define OV7725_COM4        (0x0d) /* Common control 4 */
#define 	OV7725_COM4_PLL_MASK	(0xc0)
#define 	OV7725_COM4_PLL_BYPASS	(0x00)	/*  00: Bypass PLL */
#define 	OV7725_COM4_PLL_4x	(0x40)	/*  01: PLL 4x */
#define 	OV7725_COM4_PLL_6x	(0x80)	/*  10: PLL 6x */
#define 	OV7725_COM4_PLL_8x	(0xc0)	/*  11: PLL 8x */
#define 	OV7725_COM3SCOLOR_TEST	(1<<0)
#define OV7725_AEC         (0x10) /* Exposure Value */
#define		OV7725_AEC_MASK		(0xff)
#define OV7725_COM7        (0x12) /* Common control 7 */
#define 	OV7725_COM7SLCT_QVGA	(0x40)	/* 0:VGA 1:QVGA */
#define OV7725_COM8        (0x13) /* Common control 8 */
#define 	OV7725_COM8_AGC_ON	(1<<2)	/* AGC Enable */
#define 	OV7725_COM8_AWB_ON	(1<<1)	/* AWB Enable */
#define 	OV7725_COM8_AEC_ON	(1<<0)	/* AEC Enable */
#define OV7725_DSP_CTRL3   (0x66) /* DSP control byte 3 */
#define 	OV7725_CBAR_MASK	(1<<5)	/* DSP Color bar mask */
#define OV7725_AWB_CTRL1   (0x69) /* AWB control  1 */
#define 	OV7725_AWB_CTRL1_GAIN_4x	(1<<2)

#define OV7725_T_LINE_VGA	(784)
#define OV7725_T_LINE_QVGA	(576)

struct camctrl_param *ov7725_params(void);

#endif /* _CAMCTRL_OV7725_H */
