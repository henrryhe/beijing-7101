/*****************************************************************************

File name   : hdin_reg.h

Description : HD input register addresses

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 Name
----               ------------                 ----
01-Sep-99          Created                      FC
30-Nov-99          Updated                      FC
27-Avr-01          Use new base address system  XD

*****************************************************************************/

#ifndef __HDIN_REG_H
#define __HDIN_REG_H

/* Includes --------------------------------------------------------------- */

#include "stdevice.h"
#include "dumpreg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Registers base address */
#if defined (ST_7015)
#define HDIN          (STI7015_BASE_ADDRESS+ST7015_HDIN_OFFSET)
#else
#define HDIN          (STI7020_BASE_ADDRESS+ST7020_HDIN_OFFSET)
#endif

/* Registers offsets */
#define HDIN_RFP   		     0x08           /* Reconstruction luma frame pointer */
#define HDIN_RCHP   	     0x0c           /* Reconstruction chroma frame pointer */
#define HDIN_PFP   		     0x28           /* Presentation luma frame pointer */
#define HDIN_PCHP   	     0x2c           /* Presentation chroma frame pointer */
#define HDIN_PFLD   	     0x38           /* Presentation field Main display */
#define HDIN_DFW   		     0x60           /* Input frame width */
#define HDIN_PFW   		     0x6c           /* Presentation frame width */
#define HDIN_PFH   		     0x70           /* Presentation frame height */
#define HDIN_DMOD   	     0x80           /* Reconstruction memory mode */
#define HDIN_PMOD   	     0x84           /* Presentation memory mode */

#define HDIN_ITM   		     0x98           /* Interrupt mask */
#define HDIN_ITS   		     0x9c           /* Interrupt status */
#define HDIN_STA   		     0xa0           /* Status register */

#define HDIN_CTRL   	     0xb0          /* Control register for HD pixel port */
#define HDIN_HSIZE    	     0xd4          /* Nbr of samples per input line */
#define HDIN_TOP_VEND  	     0xd8          /* Last line of the top field/frame */
#define HDIN_LNCNT    	     0xbc          /* Line count */
#define HDIN_HBLANK    	     0xd0          /* Time between HSync and active input video */
#define HDIN_HOFF    	     0xc0          /* Time between HSync and rising edge of HSyncOut */
#define HDIN_VBLANK    	     0xcc          /* Verical offset for active video */
#define HDIN_VLOFF    	     0xc4          /* Verical offset for VSyncOut */
#define HDIN_VHOFF    	     0xc8          /* Horizontal offset for VSyncOut */
#define HDIN_LL    		     0xb4          /* Lower horizontal limit */
#define HDIN_UL    		     0xb8          /* Upper horizontal limit */
#define HDIN_BOT_VEND  	     0xdc          /* Last line of the bottom field */

/* Register masks and bits */

#define HDIN_LL_MASK		0xfff           /* Mask for lower pixel count limit to start search for HSync */
#define HDIN_UL_MASK		0xfff           /* Mask for upper pixel count limit to start search for HSync */
#define HDIN_LNCNT_MASK 	0x7ff           /* Mask for line count at most recent VSync */
#define HDIN_HBLANK_MASK	0xfff           /* Mask for vertical blanking following trailing edge of Vsync */
#define HDIN_HOFF_MASK  	0xfff           /* Mask for horizontal offset from HSync to HSyncout */
#define HDIN_VLOFF_MASK		0x7ff           /* Mask for vertical lines of offset from VSync to VSyncout */
#define HDIN_VHOFF_MASK		0xfff           /* Mask for horizontal offset from VSync to VSyncout */
#define HDIN_VBLANK_MASK	0x7ff           /* Mask for vertical blanking following trailing edge of Vsync */
#define HDIN_HSIZE_MASK  	0xfff           /* Mask for horizontal active video size */
#define HDIN_TOP_VEND_MASK  0xfff           /* Mask for vertical to field last active video line */
#define HDIN_BOT_VEND_MASK  0xfff           /* Mask for vertical to field last active video line */

/* HDIN_PFLD - Presentation Field Selection ----------------------------------------*/
#define HDIN_PFLD_TCEN		 0x01			/* Toggle on Chroma Enable 				*/
#define HDIN_PFLD_TYEN		 0x02			/* Toggle on Luma Enable 				*/
#define HDIN_PFLD_BTC 		 0x04			/* Bottom not Top for chroma			*/
#define HDIN_PFLD_BTY 		 0x08			/* Bottom not Top for luma				*/

/* HDIN_DMOD - Reconstruction memory mode ------------------------------------------*/
#define HDIN_DMOD_DEC		 0x3C			/* H and V decimation modes */
#define HDIN_DMOD_NO_VDEC	 0x00			/* no vertical decimation				*/
#define HDIN_DMOD_V2_VDEC	 0x04			/* V/2 decimation 						*/
#define HDIN_DMOD_V4_VDEC	 0x08			/* V/4 decimation 						*/
#define HDIN_DMOD_NO_HDEC	 0x00			/* no horizontal decimation 			*/
#define HDIN_DMOD_V2_HDEC	 0x10			/* H/2 decimation 						*/
#define HDIN_DMOD_V4_HDEC	 0x20			/* H/4 decimation 						*/
#define HDIN_DMOD_PI		 0x40			/* Progressive input					*/

/* HDIN_PMOD - Presentation memory mode --------------------------------------------*/
#define HDIN_PMOD_DEC		 0x3C			/* H and V decimation modes */
#define HDIN_PMOD_NO_VDEC	 0x00			/* no vertical decimation				*/
#define HDIN_PMOD_V2_VDEC	 0x04			/* V/2 decimation 						*/
#define HDIN_PMOD_V4_VDEC	 0x08			/* V/4 decimation 						*/
#define HDIN_PMOD_NO_HDEC	 0x00			/* no horizontal decimation 			*/
#define HDIN_PMOD_V2_HDEC	 0x10			/* H/2 decimation 						*/
#define HDIN_PMOD_V4_HDEC	 0x20			/* H/4 decimation 						*/
#define HDIN_PMOD_PDL		 0x40			/* Progressive display luma				*/
#define HDIN_PMOD_PDC		 0x80			/* Progressive display chroma			*/

/* HDIN_ITX - Interrupt bits -------------------------------------------------------*/
#define HDIN_ITX_VSB		 0x0001			/* Vsync bottom							*/
#define HDIN_ITX_VST		 0x0002			/* Vsync top							*/

/* HDIN_CTRL - Control register for HD pixel port ----------------------------------*/
#define HDIN_CTRL_MODE	     0x0003			/* mode MASK   					        */
#define HDIN_CTRL_MODE_EMB	 0x0000			/* Embedded sync mode 					*/
#define HDIN_CTRL_MODE_RGB	 0x0001			/* RGB mode 							*/
#define HDIN_CTRL_MODE_YC	 0x0002			/* Luma/Chroma mode 					*/
#define HDIN_CTRL_MODE_SD	 0x0003			/* SD pixel interface 					*/
#define HDIN_CTRL_HAE	 	 0x0004			/* Hsync active edge, 1: falling edge 	*/
#define HDIN_CTRL_VAE	 	 0x0008			/* Vsync active edge, 1: falling edge 	*/
#define HDIN_CTRL_CAE	 	 0x0010			/* Clock active edge, 1: falling edge 	*/
#define HDIN_CTRL_UFP	 	 0x0020			/* Upper field output signal polarity 	*/
#define HDIN_CTRL_TBM	 	 0x0040			/* Top/Bottom field selection method 	*/
#define HDIN_CTRL_CNV	 	 0x0080			/* RGB to YCbCr conversion			 	*/
#define HDIN_CTRL_EN	 	 0x0100			/* Enable interface					 	*/


#ifdef __cplusplus
}
#endif

#endif /* __HDIN_REG_H */
/* ------------------------------- End of file ---------------------------- */

