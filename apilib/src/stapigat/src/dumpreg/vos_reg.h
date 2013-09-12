/*****************************************************************************

File name   : vos_reg.h

Description : Video Output Stage register addresses.
			  Includes DSP_CFG, DHDO, C656

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 			Name
----               ------------                 			----
01-Sep-99          Created                      			FC
08-Dec-99          Renamed to vos_reg.h,				FC
		   removed VTG registers
04-Jan-00          added xxx_BASE_ADDRESS defines 			FC
27-Avr-01          Use new base address system                          XD

*****************************************************************************/

#ifndef __VOS_REG_H
#define __VOS_REG_H

/* Includes --------------------------------------------------------------- */

#include "stdevice.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register base address */
#if defined (ST_7015)
#define DSPCFG     (STI7015_BASE_ADDRESS+ST7015_DSPCFG_OFFSET)
#define DHDO       (STI7015_BASE_ADDRESS+ST7015_DHDO_OFFSET)
#define C656       (STI7015_BASE_ADDRESS+ST7015_C656_OFFSET)
#define SVM        (STI7015_BASE_ADDRESS+ST7015_SVM_OFFSET)
#else
#define DSPCFG     (STI7020_BASE_ADDRESS+ST7020_DSPCFG_OFFSET)
#define DHDO       (STI7020_BASE_ADDRESS+ST7020_DHDO_OFFSET)
#define C656       (STI7020_BASE_ADDRESS+ST7020_C656_OFFSET)
#define SVM        (STI7020_BASE_ADDRESS+ST7020_SVM_OFFSET)
#endif

/* Register offsets */
#define DSPCFG_CLK		0x00				/* Clocks Configuration */
#define DSPCFG_DIG		0x04				/* Digital Output Configuration */
#define DSPCFG_ANA		0x08				/* Analog Output Configuration */
#define DSPCFG_CAPT		0x0c				/* Capture Port Configuration */
#define DSPCFG_TST		0x2000			/* Test configuration register */
#define DSPCFG_SVM		0x2004			/* SVM Static Value */
#define DSPCFG_REF		0x2008			/* RGB DAC reference value */
#define DSPCFG_DAC		0x200c			/* HD DAC Configuration */

#define DHDO_ACFG			0x00				/* Analog output configuration */
#define DHDO_ABROAD		0x04				/* Sync pulse width */
#define DHDO_BBROAD		0x08				/* Start of broad pulse */
#define DHDO_CBROAD		0x0c				/* End of broad pulse */
#define DHDO_BACTIVE	0x10				/* Start of Active video */
#define DHDO_CACTIVE	0x14				/* End of Active video */
#define DHDO_BROADL		0x18				/* Broad Length */
#define DHDO_BLANKL		0x1c				/* Blanking Length */
#define DHDO_ACTIVEL	0x20				/* Active Length */
#define DHDO_ZEROL		0x24				/* Zero Level */
#define DHDO_MIDL			0x28				/* Sync pulse Mid Level */
#define DHDO_HIGHL		0x2c				/* Sync pulse High Level */
#define DHDO_MIDLOW		0x30				/* Sync pulse Mid-Low Level */
#define DHDO_LOWL			0x34				/* Broad pulse Low Level */
#define DHDO_COLOR		0x38				/* RGB2YCbCr Mode */
#define DHDO_YMLT			0x3C				/* Luma multiplication factor */
#define DHDO_CMLT			0x40				/* Chroma multiplication factor */
#define DHDO_YOFF			0x44				/* Luma offset */
#define DHDO_COFF			0x48				/* Chroma offset */

#define C656_BACT			0x00				/* Begin of active line in pixels */
#define C656_EACT			0x04				/* End of active line in pixels */
#define C656_BLANKL		0x08				/* Blank Length in lines */
#define C656_ACTL			0x0c				/* Active Length in lines */
#define C656_PAL			0x10				/* PAL/notNTSC Mode */

/* Main Output Scan Velocity Modulator */
#define SVM_DELAY_COMP	  0x30     		/* Delay compensation for analog video out */
#define SVM_PIX_MAX		    0x34       /* Nbr of pixel per line */
#define SVM_LIN_MAX		    0x38       /* Nbr of lines per field */
#define SVM_SHAPE		    	0x3c       /* Modulation shape control */
#define SVM_GAIN   		    0x40       /* Gain control */
#define SVM_OSD_FACTOR	  0x44       /* OSD gain factor */
#define SVM_FILTER		    0x48       /* SVM freq response selection for Video and OSD */
#define SVM_EDGE_CNT	    0x4c       /* Edge count for current frame */


/* Register Bits and Masks --------------------------------------------------- */
#define DHDO_PROG295M_MASK			0x01		    /* SMPTE295 Progressive/notInterlace */
#define DHDO_ABROAD_MASK			  0xfff       /* Sync pulse width */
#define DHDO_BBROAD_MASK			  0xfff       /* Start of broad pulse */
#define DHDO_CBROAD_MASK			  0xfff       /* End of broad pulse */
#define DHDO_BACTIVE_MASK			  0xfff       /* Start of Active video */
#define DHDO_CACTIVE_MASK			  0xfff       /* End of Active video */
#define DHDO_BROADL_MASK			  0x7ff       /* Broad Length */
#define DHDO_BLANKL_MASK			  0x7ff       /* Blanking Length */
#define DHDO_ACTIVEL_MASK			  0x7ff       /* Active Length */
#define DHDO_ZEROL_MASK				  0x03ff03ff  /* Zero Level */
#define DHDO_MIDL_MASK				  0x03ff03ff  /* Sync pulse Mid Level */
#define DHDO_HIGHL_MASK				  0x03ff03ff  /* Sync pulse High Level */
#define DHDO_MIDLOW_MASK			  0x03ff03ff  /* Sync pulse Mid-Low Level */
#define DHDO_LOWL_MASK				  0x03ff03ff  /* Broad pulse Low Level */
#define DHDO_COLOR_MASK				  0x03    		/* RGB2YCbCr Mode */

#define VOS_CLK_PIP_NOT_RECORD	0x01		    /* Clocks Configuration */
#define VOS_CLK_AUX_NOT_MAIN		0x02		    /* Clocks Configuration */
#define VOS_CLK_SDMAIN_ON_DENC	0x04		    /* Clocks Configuration */
#define VOS_CLK_INVCLK					0x08		    /* Clocks Configuration */
#define VOS_CLK_NUP							0x10		    /* Clocks Configuration */
#define VOS_CLK_GFX2						0x20		    /* Clocks Configuration */
#define VOS_DIG_RGB_NOT_YCBCR		0x01		    /* Digital Output Configuration */
#define VOS_DIG_HD_SYNCIN				0x02		    /* Digital Output Configuration */
#define VOS_DIG_HD_NOT_SD				0x04		    /* Digital Output Configuration */
#define VOS_DIG_SD_SYNCIN				0x08		    /* Digital Output Configuration */
#define VOS_ANA_RGB_NOT_YCBCR		0x01		    /* Analog Output Configuration */
#define VOS_ANA_HD_SYNCIN				0x02		    /* Analog Output Configuration */
#define VOS_ANA_RESCALE 				0x04		    /* Analog Output Configuration */
#define VOS_CAPT_MAIN_DISP			0x00		    /* Capture Port Configuration */
#define VOS_CAPT_AUX_DISP				0x01		    /* Capture Port Configuration */
#if 0	/* not implemented in 7015 */
#define VOS_CAPT_SUBPICTURE			0x02		    /* Capture Port Configuration */
#endif

/* Scan Velocity Modulation */
#define SVM_DELAY_COMP_MASK	    0x0f     		/* Mask for delay compensation for analog video out */
#define SVM_PIX_MAX_MASK		    0x7ff      	/* Mask for nbr of pixel per line */
#define SVM_LIN_MAX_MASK		    0x7ff       /* Mask for nbr of lines per field */
#define SVM_SHAPE_MASK			    0x3        	/* Mask for modulation shape control */
#define SVM_SHAPE_MOD_OFF				0x0        	/* Modulation shape off */
#define SVM_GAIN_MASK   		    0x1f        /* Mask for gain control */
#define SVM_OSD_FACTOR_MASK	    0x3        	/* Mask for OSD gain factor */
#define SVM_OSD_FACTOR_0	    	0x0        	/* Null OSD gain factor */
#define SVM_OSD_FACTOR_0_5	    0x1        	/* Null OSD gain factor */
#define SVM_OSD_FACTOR_0_75	    0x2        	/* Null OSD gain factor */
#define SVM_OSD_FACTOR_1	    	0x3        	/* Null OSD gain factor */
#define SVM_FILTER_MASK		    	0x1        	/* Mask for SVM 2D freq response */
#define SVM_FILTER_OSD_2D		    0x1        	/* SVM 2D freq response OSD */
#define SVM_FILTER_VID_2D		    0x2        	/* SVM 2D freq response Video */
#define SVM_EDGE_CNT_MASK				0xfffff			/* Mask for Edge count for current frame */

#ifdef __cplusplus
}
#endif

#endif /* __VOS_REG_H */
/* ------------------------------- End of file ---------------------------- */


