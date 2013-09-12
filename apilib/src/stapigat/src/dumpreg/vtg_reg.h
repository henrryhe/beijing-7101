/*****************************************************************************

File name   : vtg_reg.h

Description : VTG register addresses.
			  Includes VTG1/2, DSP_CFG, DHDO, C656

COPYRIGHT (C) ST-Microelectronics 1999.

Date        Modification					       Name
----        ------------		         		       ----
08-Dec-99   Created from output_reg.h			 	       	FC
04-Jan-00   Added xxx_BASE_ADDRESS defines  	 			FC
07-Apr-00   Changed ITS/STA register address (hardware bug)             FC
27-Avr-01   Use new base address system                                 XD

*****************************************************************************/

#ifndef __VTG_REG_H
#define __VTG_REG_H

/* Includes --------------------------------------------------------------- */

#include "stdevice.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register base address */
#if defined (ST_7015)
#define VTG1          (STI7015_BASE_ADDRESS+ST7015_VTG1_OFFSET)
#define VTG2          (STI7015_BASE_ADDRESS+ST7015_VTG2_OFFSET)
#else
#define VTG1          (STI7020_BASE_ADDRESS+ST7020_VTG1_OFFSET)
#define VTG2          (STI7020_BASE_ADDRESS+ST7020_VTG2_OFFSET)
#endif

/* Register offsets */
#define VTG_HVDRIVE 	0x20        /* H/Vsync output enable */
#define VTG_CLKLN   	0x28        /* Line length (Pixclk) */
#define VTG_HDO   		0x2C        /* HSync start relative to Href (Pixclk) */
#define VTG_HDS   		0x30        /* Hsync end relative to Href (Pixclk) */
#define VTG_HLFLN   	0x34        /* Half-Lines per Field*/
#define VTG_VDO   		0x38        /* Vertical Drive Output start relative to Vref (half lines) */
#define VTG_VDS   		0x3C        /* Vertical Drive Output end relative to Vref (half lines) */
#define VTG_VTGMOD   	0x40        /* VTG Mode */
#define VTG_VTIMER   	0x44        /* Vsync Timer (Cycles) */
#define VTG_DRST   		0x48        /* VTG Reset */
#define VTG_RG1				0x4C				/* Start of range which generates a bottom field (in slave mode) */
#define VTG_RG2				0x50				/* End of range which generates a bottom field (in slave mode) */
#define VTG_ITM   		0x98        /* Interrupt Mask */
#define VTG_ITS   		0x9C       /* Interrupt Status */
#define VTG_STA   		0xA0       /* Status register */


/* Register Bits and Masks --------------------------------------------------- */
#define VTG_VTGMOD_SLAVEMODE_MASK					0x1f
#define VTG_VTGMOD_SLAVE_MODE_INTERNAL		0x0
#define VTG_VTGMOD_SLAVE_MODE_EXTERNAL		0x1
#define VTG_VTGMOD_SLAVE_MODE_EXT_VREF		0x2
#define VTG_VTGMOD_EXT_VSYNC_BNOTT				0x4
#define VTG_VTGMOD_EXT_VSYNC_HIGH					0x8
#define VTG_VTGMOD_EXT_HSYNC_HIGH					0x10
#define VTG_VTGMOD_FORCE_INTERLACED				0x20

#define VTG_VTGMOD_HVSYNC_CMD_MASK				0x3C0
#define VTG_VTGMOD_VSYNC_PIN				      0x00    /* for VTG1 */
#define VTG_VTGMOD_VSYNC_VTG1				      0x00    /* for VTG2 */
#define VTG_VTGMOD_VSYNC_SDIN1			      0x40
#define VTG_VTGMOD_VSYNC_SDIN2			      0x80
#define VTG_VTGMOD_VSYNC_HDIN				      0xC0
#define VTG_VTGMOD_HSYNC_PIN				      0x00    /* for VTG1 */
#define VTG_VTGMOD_HSYNC_VTG1				      0x00    /* for VTG2 */
#define VTG_VTGMOD_HSYNC_SDIN1			      0x100
#define VTG_VTGMOD_HSYNC_SDIN2			      0x200
#define VTG_VTGMOD_HSYNC_HDIN				      0x300

#define VTG_HVDRIVE_MASK					0x3
#define VTG_HVDRIVE_VSYNC					0x1
#define	VTG_HVDRIVE_HSYNC					0x2

/* VTG interrupts */
#define VTG_IT_MASK							  0x3f
#define VTG_IT_CIFD							  0x01
#define VTG_IT_YIFD							  0x02
#define VTG_IT_OFD							  0x04
#define VTG_IT_VSB							  0x08
#define VTG_IT_VST							  0x10
#define VTG_IT_PDVS							  0x20

#ifdef __cplusplus
}
#endif

#endif /* __VTG_REG_H */
/* ------------------------------- End of file ---------------------------- */


