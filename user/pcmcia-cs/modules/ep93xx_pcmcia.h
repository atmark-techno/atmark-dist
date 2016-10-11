/*
 * ep93xx_pcmcia.h
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License
 * at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and
 * limitations under the License. 
 *
 * The initial developer of the original code is David A. Hinds
 * <dahinds@users.sourceforge.net>.  Portions created by David A. Hinds
 * are Copyright (C) 1999 David A. Hinds.  All Rights Reserved.
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License version 2 (the "GPL"), in
 * which case the provisions of the GPL are applicable instead of the
 * above.  If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL.  If you do not delete the
 * provisions above, a recipient may use your version of this file
 * under either the MPL or the GPL.
 */

#ifndef _LINUX_EP93XX_PCMCIA_H
#define _LINUX_EP93XX_PCMCIA_H

//
// PCMCIA memory mappings.
//
#define EP93XX_PHYS_ADDR_PCMCIAMEM 	(PCMCIA_BASE_PHYS + 0xC000000)
#define EP93XX_SIZE_PCMCIAMEM		0x04000000

#define EP93XX_PHYS_ADDR_PCMCIAATTR	(PCMCIA_BASE_PHYS + 0x8000000)
#define EP93XX_SIZE_PCMCIAATTR		0x04000000

#define EP93XX_PHYS_ADDR_PCMCIAIO	(PCMCIA_BASE_PHYS)
#define EP93XX_SIZE_PCMCIAIO		0x04000000

//
// EEprom control signals.
//
#define  GPIOA_EECLK                    0x01
#define  GPIOA_EEDAT                    0x02
#define  GPIOA_SLA0                     0x04

//
// PCMCIA status signals = GPIO port F.
// 
#define EP93XX_PCMCIA_WP          		0x01
#define EP93XX_PCMCIA_CD1         		0x02
#define EP93XX_PCMCIA_CD2         		0x04
#define EP93XX_PCMCIA_BVD1        		0x08
#define EP93XX_PCMCIA_BVD2        		0x10
#define EP93XX_PCMCIA_VS1         		0x20
#define EP93XX_PCMCIA_RDY         		0x40
#define EP93XX_PCMCIA_VS2				0x80

#define EP93XX_PCMCIA_INT_WP			IRQ_GPIO0	/* 19 */
#define EP93XX_PCMCIA_INT_CD1           IRQ_GPIO1	/* 20 */
#define EP93XX_PCMCIA_INT_CD2           IRQ_GPIO2	/* 21 */
#define EP93XX_PCMCIA_INT_BVD1          IRQ_GPIO3	/* 22 */
#define EP93XX_PCMCIA_INT_BVD2          IRQ_GPIO4	/* 47 */
#define EP93XX_PCMCIA_INT_VS1           IRQ_GPIO5	/* 48 */
#define EP93XX_PCMCIA_INT_RDY           IRQ_GPIO6	/* 49 */
#define EP93XX_PCMCIA_INT_VS2           IRQ_GPIO7	/* 50 */

//
// Definitions to program the TPS2202A PCMCIA controller.
//
#define AVCC_0V         			0x000
#define AVCC_33V        			0x004
#define AVCC_5V         			0x008
#define BVCC_0V         			0x000
#define BVCC_33V        			0x080
#define BVCC_5V         			0x040
#define AVPP_0V         			0x000
#define BVPP_0V         			0x000
#define ENABLE          			0x100
#define EE_ADDRESS      			(0x55<<9)

#define EP93XX_MAX_SOCK   (1)

#define EP93XX_PCMCIA_IO_ACCESS      (0)
#define EP93XX_PCMCIA_5V_MEM_ACCESS  (300)
#define EP93XX_PCMCIA_3V_MEM_ACCESS  (600)

#endif /* _LINUX_EP93XX_PCMCIA_H */
