/*****************************************************************************

File name   : sdin_reg.h

Description : SD input register addresses

COPYRIGHT (C) ST-Microelectronics 1999.

Date               	Modification                 Name
----               	------------                 ----
26-Aug-99               Created                      FC
27-Avr-01               Use new base address system  XD

TODO:
=====
TODO: Solve conflict between ITM/ITS/STA registers and other registers
*****************************************************************************/

#ifndef __SDIN_REG_H
#define __SDIN_REG_H

/* Includes --------------------------------------------------------------- */

#include "stdevice.h"
#include "dumpreg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register base address */
#if defined (ST_7015)
#define SDIN1        (STI7015_BASE_ADDRESS+ST7015_SDIN1_OFFSET)
#define SDIN2        (STI7015_BASE_ADDRESS+ST7015_SDIN2_OFFSET)
#elif defined (ST_7020)
#define SDIN1        (STI7020_BASE_ADDRESS+ST7020_SDIN1_OFFSET)
#define SDIN2        (STI7020_BASE_ADDRESS+ST7020_SDIN2_OFFSET)
#endif

#if defined (ST_7015) || defined  (ST_7020)
/* Register offsets */
#define SDIN_RFP   			0x08            /* Reconstruction luma frame pointer */
#define SDIN_RCHP   		0x0c            /* Reconstruction chroma frame pointer */
#define SDIN_PFP   			0x28            /* Presentation luma frame pointer */
#define SDIN_PCHP   		0x2c            /* Presentation chroma frame pointer */
#define SDIN_PFLD   		0x38            /* Presentation field Main display */
#define SDIN_ANC			0x50			/* Ancilliary data buffer begin */
#define SDIN_DFW   			0x60            /* Input frame width */
#define SDIN_PFW   			0x6c            /* Presentation frame width */
#define SDIN_PFH   			0x70            /* Presentation frame height */
#define SDIN_DMOD   		0x80            /* Reconstruction memory mode */
#define SDIN_PMOD   		0x84            /* Presentation memory mode */

#define SDIN_ITM   			0x98            /* Interrupt mask */
#define SDIN_ITS   			0x9c            /* Interrupt status */
#define SDIN_STA   			0xa0            /* Status register */

#if defined (ST_7015)
#define SDIN_CTRL  			0xb0            /* Control register for SD pixel port */

#define SDIN_CTRL_MASK		 0x007f         /* Mask for control register       		*/
#define SDIN_CTRL_SDE	 	 0x0001			/* Enable 								*/
#define SDIN_CTRL_MDE		 0x0002         /* Mode: ExtSync not CCIR656 			*/
#define SDIN_CTRL_MDE_656	 0x0000			/* CCIR656 mode 						*/
#define SDIN_CTRL_MDE_EXT	 0x0002			/* External sync mode 					*/
#define SDIN_CTRL_DC		 0x0004         /* Data Capture mode 					*/
#define SDIN_CTRL_HED	 	 0x0008			/* HSI active edge, 1: rising edge 		*/
#define SDIN_CTRL_VED	 	 0x0010			/* VSI active edge, 1: falling edge 	*/
#define SDIN_CTRL_ANC	 	 0x0020			/* Ancillary data format (cf spec)	 	*/
#define SDIN_CTRL_TVS	 	 0x0040			/* Progressive mode (ignore field type)	*/

#define SDIN_LL             0xb4            /* Lower pixel count limit to start search for HSync */
#define SDIN_UL    			0xb8            /* Upper pixel count limit to start search for HSync */
#define SDIN_LNCNT    		0xbc            /* Line count at most recent VSync */
#define SDIN_HOFF    		0xc0            /* Horizontal offset from HSync to HSyncout */
#define SDIN_VLOFF    		0xc4            /* Vertical lines of offset from VSync to VSyncout */
#define SDIN_VHOFF    		0xc8            /* Horizontal offset from VSync to VSyncout */
#define SDIN_VBLANK    		0xcc            /* Vertical blanking following trailing edge of Vsync */
#define SDIN_HBLANK    	    0xd0            /* Time between HSync and active input video */
#define SDIN_LENGTH    	    0xd4            /* Number of ancillary data bytes in Data Capture mode */

#elif defined (ST_7020)

#define SDIN_CTL           0xb0            /* SD Pixel Port Control Register */

/* SDIN_CTRL - Control register ----------------------------------------------------*/
#define SDIN_CTL_MASK      0x07ff         /* Mask for control register            */

#define SDIN_CTL_SDE       0x0001
#define SDIN_CTL_ADE       0x0002
#define SDIN_CTL_ESP       0x0004
#define SDIN_CTL_VBH       0x0008
#define SDIN_CTL_MDE       0x0010
#define SDIN_CTL_HRP       0x0020
#define SDIN_CTL_VRP       0x0040
#define SDIN_CTL_DC        0x0080
#define SDIN_CTL_SPN       0x0100
#define SDIN_CTL_E254      0x0200
#define SDIN_CTL_OEV       0x0400

#define SDIN_TFO           0xb4            /* SD input top field offset */
#define SDIN_TFS           0xb8            /* SD input top field stop   */
#define SDIN_BFO           0xbc            /* SD input bottom field offset */
#define SDIN_BFS           0xc0            /* SD input bottom field stop   */
#define SDIN_VSD           0xc4            /* SD input vertical synchronization delay   */
#define SDIN_HSD           0xc8            /* SD input horizontal synchronization delay */
#define SDIN_HLL           0xcc            /* SD input half-line length                 */
#define SDIN_HLFLN         0xd0            /* SD input half-lines per verical           */
#define SDIN_APS           0xd4            /* SD input ancillary data page size         */

#endif /* ST_7020 */


/* Register masks and bits */

#define SDIN_LL_MASK		0x3ff           /* Mask for lower pixel count limit to start search for HSync */
#define SDIN_UL_MASK		0x3ff           /* Mask for upper pixel count limit to start search for HSync */
#define SDIN_LNCNT_MASK 	0x1ff           /* Mask for line count at most recent VSync */
#define SDIN_HBLANK_MASK	0x3ff           /* Mask for vertical blanking following trailing edge of Vsync */
#define SDIN_HOFF_MASK  	0x7ff           /* Mask for horizontal offset from HSync to HSyncout */
#define SDIN_VLOFF_MASK		0x1ff           /* Mask for vertical lines of offset from VSync to VSyncout */
#define SDIN_VHOFF_MASK		0x7ff           /* Mask for horizontal offset from VSync to VSyncout */
#define SDIN_VBLANK_MASK	0x00f           /* Mask for vertical blanking following trailing edge of Vsync */
#define SDIN_LENGTH_MASK	0x0ff           /* Mask for ancillary data capture length */

/* SDIN_PFLD - Presentation Field Selection ----------------------------------------*/
#define SDIN_PFLD_TCEN		 0x01			/* Toggle on Chroma Enable 				*/
#define SDIN_PFLD_TYEN		 0x02			/* Toggle on Luma Enable 				*/
#define SDIN_PFLD_BTC 		 0x04			/* Bottom not Top for chroma			*/
#define SDIN_PFLD_BTY 		 0x08			/* Bottom not Top for luma				*/

/* SDIN_DMOD - Reconstruction memory mode ------------------------------------------*/
#define SDIN_DMOD_DEC		 0x30			/* V decimation modes */
#define SDIN_DMOD_NO_HDEC	 0x00			/* no horizontal decimation 			*/
#define SDIN_DMOD_V2_HDEC	 0x10			/* H/2 decimation 						*/
#define SDIN_DMOD_V4_HDEC	 0x20			/* H/4 decimation 						*/

/* SDIN_PMOD - Presentation memory mode --------------------------------------------*/
#define SDIN_PMOD_DEC		 0x30			/* V decimation modes */
#define SDIN_PMOD_NO_HDEC	 0x00			/* no horizontal decimation 			*/
#define SDIN_PMOD_V2_HDEC	 0x10			/* H/2 decimation 						*/
#define SDIN_PMOD_V4_HDEC	 0x20			/* H/4 decimation 						*/

/* SDIN_ITX - Interrupt bits -------------------------------------------------------*/
#define SDIN_ITX_VSB		 0x0001			/* Vsync bottom							*/
#define SDIN_ITX_VST		 0x0002			/* Vsync top							*/

#endif /* ST_7015 || ST_7020 */

#ifdef __cplusplus
}
#endif

#endif /* __SDIN_REG_H */
/* ------------------------------- End of file ---------------------------- */


