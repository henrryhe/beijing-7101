/*****************************************************************************

File name   : display_reg.h

Description : Display register addresses

COPYRIGHT (C) ST-Microelectronics 1999.

Date        Modification                 	      			Name
----        ------------                 		     		----
06-Sep-99   Created                      			 	FC
04-Oct-99   Moved SVM block to output_reg.h              		FC
04-Jan-00   Added xxx_BASE_ADDRESS defines				FC
29-Feb-00   Added PSI bypass			     			FC
27-Avr-01   Use new base address system                                 XD
TODO:
-----

*****************************************************************************/

#ifndef __DISPLAY_REG_H
#define __DISPLAY_REG_H

/* Includes --------------------------------------------------------------- */

#include "stdevice.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Register base address */
#if defined (ST_7015)
#define DIS             (STI7015_BASE_ADDRESS+ST7015_DISPLAY_OFFSET)
#define DIP             (STI7015_BASE_ADDRESS+ST7015_PIP_DISPLAY_OFFSET)
#else
#define DIS             (STI7020_BASE_ADDRESS+ST7020_DISPLAY_OFFSET)
#define DIP             (STI7020_BASE_ADDRESS+ST7020_PIP_DISPLAY_OFFSET)
#endif

/* Register offsets */
/* Main Display */
#define	DIS_XCFG1				0x0000		  /* Multi-display configuration for picture 1 */
#define	DIS_XDOFF1				0x0004		  /* Multi-display X offset for picture 1 (in MB) */
#define	DIS_YDOFF1				0x0008		  /* Multi-display Y offset for picture 1 (in MB) */
#define	DIS_XCFG2				0x0010		  /* Multi-display configuration for picture 2 */
#define	DIS_XDOFF2				0x0014		  /* Multi-display X offset for picture 2 (in MB) */
#define	DIS_YDOFF2				0x0018		  /* Multi-display Y offset for picture 2 (in MB) */
#define	DIS_XCFG3				0x0020		  /* Multi-display configuration for picture 3 */
#define	DIS_XDOFF3				0x0024		  /* Multi-display X offset for picture 3 (in MB) */
#define	DIS_YDOFF3				0x0028		  /* Multi-display Y offset for picture 3 (in MB) */
#define	DIS_XCFG4				0x0030		  /* Multi-display configuration for picture 4 */
#define	DIS_XDOFF4				0x0034		  /* Multi-display X offset for picture 4 (in MB) */
#define	DIS_YDOFF4				0x0038		  /* Multi-display Y offset for picture 4 (in MB) */
#define	DIS_MXFH				0x0040		  /* Main display main window total height (in MB) */
#define	DIS_MXFW				0x0044		  /* Main display main window total width (in MB) */
#define	DIS_DMB					0x0048		  /* Dummy macroblock location for empty spaces */
#define	DIS_DCMB				0x004c		  /* Dummy macroblock chroma location */
#define	DIS_LMP					0x0050		  /* LMU luma buffer start pointer */
#define	DIS_LCMP				0x0054		  /* LMU chroma buffer start pointer */

/* Main Display Interrupts */
#define	DIS_ITM					0x0098		  /* Interrupt mask */
#define	DIS_ITS					0x009c		  /* Interrupt status */
#define	DIS_STA					0x00a0		  /* Status Register */

/* Main Display Block to Line */
#define DIS_BTL_BPL   	        0x1d00        /* Blocks per Line */

/* Main Display Vertical Block Correction */
#define DIS_VBC_VINP   	        0x1d80        /* Vertical Start Position */

/* Main Display Linear Median Upconverter */
#define DIS_LMU_CTRL	        0x1c80        /* LMU Control */
#define DIS_LMU_AFD		        0x1c84        /* Accumulated Frame Difference */

/* Main Display Vertical Format Control */
#define DIS_VFC_COEFFS	        0x1800        /* VFC Coeffs */
#define DIS_VFC_LDELTA1         0x1a00        /* Line Delta Zone 1 */
#define DIS_VFC_LDELTA2         0x1a04        /* Line Delta Zone 2 */
#define DIS_VFC_LDELTA4         0x1a08        /* Line Delta Zone 4 */
#define DIS_VFC_LDELTA5         0x1a0c        /* Line Delta Zone 5 */
#define DIS_VFC_VZONE1          0x1a10        /* End of Vertical Zone 1 */
#define DIS_VFC_VZONE2          0x1a14        /* End of Vertical Zone 2 */
#define DIS_VFC_VZONE3          0x1a18        /* End of Vertical Zone 3 */
#define DIS_VFC_VZONE4          0x1a1c        /* End of Vertical Zone 4 */
#define DIS_VFC_VZONE5          0x1a20        /* End of Vertical Zone 5 */
#define DIS_VFC_LSTEP	        0x1a24        /* Distance from 1st to 2nd progressive line */
#define DIS_VFC_LSTEP_INIT	    0x1a28        /* Sub-Line scanning from VINP position */
#define DIS_VFC_VINL		    0x1a2c        /* Nbr of progressive luma VFC input lines */
#define DIS_VFC_HINP		    0x1a30        /* Horizontal start position of VFC output line */
#define DIS_VFC_HSINL		    0x1a34        /* Horizontal size of stored image */
#define DIS_VFC_IID   		    0x1a38        /* Input image type */
#define DIS_VFC_THRU		    0x1a3c        /* Vertical bypass */

/* Main Display Horizontal Format Control */
#define DIS_HFC_LCOEFFS		    0x1000        /* HFC Luma coeffs */
#define DIS_HFC_CCOEFFS		    0x1400        /* HFC Chroma coeffs */
#define DIS_HFC_YHINL		    0x1600        /* Pixel nbr of VFC luma output line */
#define DIS_HFC_CHINL		    0x1604        /* Pixel nbr of VFC chroma output line */
#define DIS_HFC_PDELTA1		    0x1608        /* PSTEP delta per output pixel Zone 1 */
#define DIS_HFC_PDELTA2		    0x160c        /* PSTEP delta per output pixel Zone 2 */
#define DIS_HFC_PDELTA4		    0x1610        /* PSTEP delta per output pixel Zone 4 */
#define DIS_HFC_PDELTA5		    0x1614        /* PSTEP delta per output pixel Zone 5 */
#define DIS_HFC_HZONE1		    0x1618        /* End of Horizontal zone 1 */
#define DIS_HFC_HZONE2		    0x161c        /* End of Horizontal zone 2 */
#define DIS_HFC_HZONE3		    0x1620        /* End of Horizontal zone 3 */
#define DIS_HFC_HZONE4		    0x1624        /* End of Horizontal zone 4 */
#define DIS_HFC_HZONE5		    0x1628        /* End of Horizontal zone 5 */
#define DIS_HFC_PSTEP		    0x162c        /* Distance from 1st to 2nd output pixel */
#define DIS_HFC_PSTEP_INIT	    0x1630        /* Sub-pixel panning from HINP position */
#define DIS_HFC_THRU		    0x1634        /* Horizontal bypass */

/* Main Display Edge Replacement */
#define DIS_ER_CTRL 		    0x1b10        /* Gain control */
#define DIS_ER_DELAY		    0x1b58        /* Operation frequency range */

/* Main Display Peaking */
#define DIS_PK_COREV 		    0x1b00        /* Coring for vertical peaking */
#define DIS_PK_VGAIN		    0x1b04        /* Vertical peaking gain control */
#define DIS_PK_FILT			    0x1b08        /* Horizontal peaking filter selection */
#define DIS_PK_HGAIN		    0x1b0c        /* Horizontal peaking gain control */
#define DIS_PK_COREH   		    0x1b5c        /* Coring for horizontal peaking */
#define DIS_PK_SIEN   		    0x1b60        /* Si Compensation */

/* Main Display Auto-Flesh */
#define DIS_AF_QWIDTH		    0x1b40        /* Quadrature flesh width control */
#define DIS_AF_TINT			    0x1b48        /* Tint control */
#define DIS_AF_OFF 			    0x1b4c        /* Auto flesh on/off */
#define DIS_AF_AXIS   		    0x1b50        /* Flesh axis control */

/* Main Display Green-Boost */
#define DIS_GB_OFF 			    0x1b54        /* Green boost on/off */

/* Main Display Dynamic Contrast Improvement */
#define DIS_DCI_COR_MOD			0x1b14        /* Coring mode */
#define DIS_DCI_COR   			0x1b18        /* Coring level */
#define DIS_DCI_FILTER   		0x1b1c        /* Low-Pass filter selection */
#define DIS_DCI_ELINE   		0x1b20        /* Last line of the DCI analysis window */
#define DIS_DCI_GAIN_SEG2_FRC   0x1b24        /* Frac part of gain for 2nd seg of dual seg xfer function */
#define DIS_DCI_LSTHR		    0x1b28        /* Light sample threshold */
#define DIS_DCI_LSWF   		    0x1b2c        /* Light sample weighting factor */
#define DIS_DCI_DSTHR		    0x1b30        /* Dark sample threshold */
#define DIS_DCI_SLINE		    0x1b34        /* 1st line of DCI analysis */
#define DIS_DCI_SNR_COR   	    0x1b38        /* Signal to noise ratio estimation for coring */
#define DIS_DCI_TF_DPP		    0x1b3c        /* Transfer function dynamic pivot point */
#define DIS_DCI_SAT   		    0x1b44        /* Saturation control */
#define DIS_DCI_CSC_ON		    0x1b64        /* Color Saturation Compensation on/off */
#define DIS_DCI_DCI_ON		    0x1b68        /* DCI on/off */
#define DIS_DCI_FREEZE_ANLY	    0x1b6c        /* Freeze analysis on/off */
#define DIS_DCI_DLTHR		    0x1b70        /* Dark level threshold for DSDA */
#define DIS_DCI_DYTC		    0x1b74        /* Dark area size for DSDA */
#define DIS_DCI_PEAK_SIZE	    0x1b78        /* Peak area size */
#define DIS_DCI_SENSBS   	    0x1b80        /* Sensitivity of DSDA */
#define DIS_DCI_SENSWS   	    0x1b84        /* Sensitivity of ABA */
#define DIS_DCI_EPIXEL   	    0x1b88        /* Hor pel position of right edge of DCI analysis window */
#define DIS_DCI_ERR_COMP   	    0x1b90        /* Error compensation */
#define DIS_DCI_GAIN_SEG1_FRC   0x1b94        /* Frac part of gain for 1st seg of dual seg xfer function */
#define DIS_DCI_SPIXEL		    0x1b98        /* Hor pel position of left edge of DCI analysis window  */
#define DIS_DCI_PEAK_THR0	    0x1b9c        /* Range 0 upper limit for peak detection */
#define DIS_DCI_PEAK_THR1	    0x1ba0        /* Range 1 upper limit for peak detection */
#define DIS_DCI_PEAK_THR2	    0x1ba4        /* Range 2 upper limit for peak detection */
#define DIS_DCI_PEAK_THR3	    0x1ba8        /* Range 3 upper limit for peak detection */
#define DIS_DCI_PEAK_THR4	    0x1bac        /* Range 4 upper limit for peak detection */
#define DIS_DCI_PEAK_THR5	    0x1bb0        /* Range 5 upper limit for peak detection */
#define DIS_DCI_PEAK_THR6	    0x1bb4        /* Range 6 upper limit for peak detection */
#define DIS_DCI_PEAK_THR7	    0x1bb8        /* Range 7 upper limit for peak detection */
#define DIS_DCI_PEAK_THR8	    0x1bbc        /* Range 8 upper limit for peak detection */
#define DIS_DCI_PEAK_THR9	    0x1bc0        /* Range 9 upper limit for peak detection */
#define DIS_DCI_PEAK_THR10	    0x1bc4        /* Range 10 upper limit for peak detection */
#define DIS_DCI_PEAK_THR11	    0x1bc8        /* Range 11 upper limit for peak detection */
#define DIS_DCI_PEAK_THR12	    0x1bcc        /* Range 12 upper limit for peak detection */
#define DIS_DCI_PEAK_THR13	    0x1bd0        /* Range 13 upper limit for peak detection */
#define DIS_DCI_PEAK_THR14	    0x1bd4        /* Range 14 upper limit for peak detection */
#define DIS_DCI_PEAK_THR15	    0x1bd8        /* Range 15 upper limit for peak detection */
#define DIS_DCI_CBCR_PEAK	    0x1be0        /* Output of chroma peak detector */
#define DIS_DCI_DRKS_DIST	    0x1be4        /* Output of dark sample distribution */
#define DIS_DCI_AVRG_BR		    0x1be8        /* Output of the average brightness analysis */
#define DIS_DCI_PEAK_N		    0x1bec        /* Upper limit control for peak detection algorithm */
#define DIS_DCI_FR_PEAK0	    0x1bf0        /* Counter output for peak detection in range 0 */
#define DIS_DCI_FR_PEAK1	    0x1bf4        /* Counter output for peak detection in range 1 */
#define DIS_DCI_FR_PEAK2	    0x1bf8        /* Counter output for peak detection in range 2 */
#define DIS_DCI_FR_PEAK3	    0x1bfc        /* Counter output for peak detection in range 3 */
#define DIS_DCI_FR_PEAK4	    0x1c00        /* Counter output for peak detection in range 4 */
#define DIS_DCI_FR_PEAK5	    0x1c04        /* Counter output for peak detection in range 5 */
#define DIS_DCI_FR_PEAK6	    0x1c08        /* Counter output for peak detection in range 6 */
#define DIS_DCI_FR_PEAK7	    0x1c0c        /* Counter output for peak detection in range 7 */
#define DIS_DCI_FR_PEAK8	    0x1c10        /* Counter output for peak detection in range 8 */
#define DIS_DCI_FR_PEAK9	    0x1c14        /* Counter output for peak detection in range 9 */
#define DIS_DCI_FR_PEAK10  	    0x1c18        /* Counter output for peak detection in range 10 */
#define DIS_DCI_FR_PEAK11  	    0x1c1c        /* Counter output for peak detection in range 11 */
#define DIS_DCI_FR_PEAK12  	    0x1c20        /* Counter output for peak detection in range 12 */
#define DIS_DCI_FR_PEAK13  	    0x1c24        /* Counter output for peak detection in range 13 */
#define DIS_DCI_FR_PEAK14  	    0x1c28        /* Counter output for peak detection in range 14 */
#define DIS_DCI_FR_PEAK15  	    0x1c2c        /* Counter output for peak detection in range 15 */
#define DIS_DCI_BYPASS  	    0x1c30        /* Enable bypass of PSI process */


/* PIP/Record Display */
#define	DIP_XCFG				0x0000		  /* PIP Configuration */

/* PIP/Record Display Interrupts */
#define	DIP_ITM					0x0098		  /* Interrupt mask */
#define	DIP_ITS					0x009c		  /* Interrupt status */
#define	DIP_STA					0x00a0		  /* Status Register */

/* PIP/Record Display Interface*/
#define DIP_IF_YRCC   		    0x1cc0        /* Luma channel rate control configuration */
#define DIP_IF_CRCC   		    0x1cc4        /* Chroma channel rate control configuration */
#define DIP_IF_VINP   		    0x1d80        /* Display vertical start position */
#define DIP_IF_TST_YFDATA	    0x1f98        /* Data test register for the test of the luma FIFO */
#define DIP_IF_TST_YFCTRL      	0x1f9c        /* Control test register for the test of the luma FIFO */
#define DIP_IF_TST_CFDATA      	0x1f90        /* Data test register for the test of the chroma FIFO */
#define DIP_IF_TST_CFCTRL      	0x1f94        /* Control test register for the test of the chroma FIFO */

/* PIP/Record Display Horizontal Format Control */
#define DIP_HFC_LCOEFFS		    0x1000        /* HFC Luma coeffs */
#define DIP_HFC_CCOEFFS		    0x1400        /* HFC Chroma coeffs */
#define DIP_HFC_YHINL	 	    0x1600        /* Pixel nbr of a VFC luma output line */
#define DIP_HFC_CHINL   	    0x1604        /* Pixel nbr of a VFC chroma output line */
#define DIP_HFC_PDELTA1   	    0x1608        /* Delta of PSTEP per ouput pixel in zone 1 */
#define DIP_HFC_PDELTA2   	    0x160c        /* Delta of PSTEP per ouput pixel in zone 2 */
#define DIP_HFC_PDELTA4   	    0x1610        /* Delta of PSTEP per ouput pixel in zone 4 */
#define DIP_HFC_PDELTA5   	    0x1614        /* Delta of PSTEP per ouput pixel in zone 5 */
#define DIP_HFC_HZONE1   	    0x1618        /* End of horizontal zone 1 */
#define DIP_HFC_HZONE2   	    0x161c        /* End of horizontal zone 2 */
#define DIP_HFC_HZONE3   	    0x1620        /* End of horizontal zone 3 */
#define DIP_HFC_HZONE4   	    0x1624        /* End of horizontal zone 4 */
#define DIP_HFC_HZONE5   	    0x1628        /* End of horizontal zone 5 */
#define DIP_HFC_PSTEP   	    0x162c        /* Distance from 1st to 2nd output pixel */
#define DIP_HFC_PSTEP_INIT      0x1630        /* Subpixel panning from HINP position */
#define DIP_HFC_THRU	   	    0x1634        /* Horizontal bypass */

/* PIP/Record Display Vertical Format Control */
#define DIP_VFC_COEFFS	        0x1800        /* VFC Coeffs */
#define DIP_VFC_LDELTA1         0x1a00        /* Line Delta Zone 1 */
#define DIP_VFC_LDELTA2         0x1a04        /* Line Delta Zone 2 */
#define DIP_VFC_LDELTA4         0x1a08        /* Line Delta Zone 4 */
#define DIP_VFC_LDELTA5         0x1a0c        /* Line Delta Zone 5 */
#define DIP_VFC_VZONE1          0x1a10        /* End of Vertical Zone 1 */
#define DIP_VFC_VZONE2          0x1a14        /* End of Vertical Zone 2 */
#define DIP_VFC_VZONE3          0x1a18        /* End of Vertical Zone 3 */
#define DIP_VFC_VZONE4          0x1a1c        /* End of Vertical Zone 4 */
#define DIP_VFC_VZONE5          0x1a20        /* End of Vertical Zone 5 */
#define DIP_VFC_LSTEP	        0x1a24        /* Distance from 1st to 2nd progressive line */
#define DIP_VFC_LSTEP_INIT	    0x1a28        /* Sub-Line scanning from VINP position */
#define DIP_VFC_VINL		    0x1a2c        /* Nbr of progressive luma VFC input lines */
#define DIP_VFC_HINP		    0x1a30        /* Horizontal start position of VFC output line */
#define DIP_VFC_HSINL		    0x1a34        /* Horizontal size of stored image */
#define DIP_VFC_IID   		    0x1a38        /* Input image type */
#define DIP_VFC_THRU		    0x1a3c        /* Vertical bypass */

/* PIP/Record Display Tint & Saturation */
#define DIP_TS_TINT   		    0x1b00        /* Tint control */
#define DIP_TS_SAT   		    0x1b04        /* Saturation control */
#define DIP_TS_PK_DET_OUT	    0x1b08        /* Frame wise chroma peak detector */
#define DIP_TS_BYPASS  		    0x1b0c        /* Tint & Sat bypass */


/* bits and masks ----------------------------------------------------------*/
/* Display Cfg */
#define	DIS_XCFG_VID1			0x00		 	/* Multi-display src from Video 1 */
#define	DIS_XCFG_VID2			0x01		  	/* Multi-display src from Video 2 */
#define	DIS_XCFG_VID3			0x02		  	/* Multi-display src from Video 3 */
#define	DIS_XCFG_VID4			0x03		  	/* Multi-display src from Video 4 */
#define	DIS_XCFG_VID5			0x04		  	/* Multi-display src from Video 5 */
#define	DIS_XCFG_SDIN1			0x05		  	/* Multi-display src from SDIN 1 */
#define	DIS_XCFG_SDIN2			0x06		  	/* Multi-display src from SDIN 2 */
#define	DIS_XCFG_HDIN			0x07		  	/* Multi-display src from HDIN */

#define	DIS_XCFG_PRIORITY_MASK	0x30		 	/* Mask for multi-display src priority */
#define	DIS_XCFG_ACTIVE_MASK	0x40		 	/* Mask for multi-display image active */
#define	DIS_XCFG_SOURCE_MASK	0x0f		 	/* Mask for multi-display source video decoder */
#define	DIS_XCFG_ENABLE			0x40		 	/* Enable multi-display src */

#define	DIP_XCFG_SOURCE_MASK	0x0f		 	/* Mask for PIP multi-display source video decoder */

/* Display interrupt */
#define	DIS_INTERRUPT_EAW_MASK	0x400		 	/* Mask for interrupt EAW */
#define	DIS_INTERRUPT_YWFF_MASK	0x200		 	/* Mask for interrupt YWFF */
#define	DIS_INTERRUPT_YRFE_MASK	0x100		 	/* Mask for interrupt YRFE */
#define	DIS_INTERRUPT_CWFF_MASK	0x80		 	/* Mask for interrupt CWFF */
#define	DIS_INTERRUPT_CRFE_MASK	0x40		 	/* Mask for interrupt CRFE */
#define	DIS_INTERRUPT_YBFE_MASK	0x20		 	/* Mask for interrupt YBFE */
#define	DIS_INTERRUPT_YAFE_MASK	0x10		 	/* Mask for interrupt YAFE */
#define	DIS_INTERRUPT_CFE_MASK	0x8			 	/* Mask for interrupt CFE */
#define	DIS_INTERRUPT_PXFE_MASK	0x4			 	/* Mask for interrupt PXFE */
#define	DIS_INTERRUPT_YIFD_MASK	0x2			 	/* Mask for interrupt YIFD */
#define	DIS_INTERRUPT_CIFD_MASK	0x1			 	/* Mask for interrupt CIFD */

#define	DIP_INTERRUPT_YAFE_MASK	0x10		 	/* Mask for interrupt YAFE */
#define	DIP_INTERRUPT_CFE_MASK	0x08		 	/* Mask for interrupt CFE */
#define	DIP_INTERRUPT_PXFE_MASK	0x04		 	/* Mask for interrupt PXFE */
#define	DIP_INTERRUPT_YIFD_MASK	0x02		 	/* Mask for interrupt YIFD */
#define	DIP_INTERRUPT_CIFD_MASK	0x01		 	/* Mask for interrupt CIFD */

/* PIP Interface */
#define	DIP_IF_YRCC_MASK		0x0f		 	/* Mask for Luma channel rate control configuration */
#define	DIP_IF_CRCC_MASK		0x0f		 	/* Mask for Chroma channel rate control configuration */
#define	DIP_IF_VINP_MASK		0x7ff		 	/* Mask for Display vertical start position */
#define	DIP_IF_TST_YFCTRL_YRST_MASK	0x4	 		/* Mask for reset of luma fifo */
#define	DIP_IF_TST_YFCTRL_YSEL1_MASK 0x2	 	/* Mask for Upper 32 bits selection of luma fifo */
#define	DIP_IF_TST_YFCTRL_YSEL2_MASK 0x1	 	/* Mask for Lower 32 bits selection of luma fifo */
#define	DIP_IF_TST_CFCTRL_CRST_MASK	0x4	 		/* Mask for reset of Chroma fifo */
#define	DIP_IF_TST_CFCTRL_CSEL1_MASK 0x2	 	/* Mask for Upper 32 bits selection of Chroma fifo */
#define	DIP_IF_TST_CFCTRL_CSEL2_MASK 0x1	 	/* Mask for Lower 32 bits selection of Chroma fifo */


/* BTL */
#define	DIS_BTL_BPL_MASK		0x7f		 	/* Mask for Block to line */

/* VBC */
#define	DIS_VBC_VINP_MASK		0x7ff		 	/* Mask for Vertical Block Correction */

/* LMU */
#define DIS_LMU_MC_ON_LUMA_PAIR				0x40000000	/* Motion estimation calculated on maximum of luma pair */
#define DIS_LMU_DISABLE_LUMA_TEMP_INTERP		0x20000000	/* Disable luma temporal interpolation */
#define DIS_LMU_ENABLE_LUMA_LINE_DOUBLING		0x10000000	/* Enable luma line doubling */
#define DIS_LMU_ENABLE_LUMA_CLK_DIV_BY_1		0x00000000	/* Luma clock throttle */
#define DIS_LMU_ENABLE_LUMA_CLK_DIV_BY_2		0x04000000	/* Luma clock throttle */
#define DIS_LMU_ENABLE_LUMA_CLK_DIV_BY_3		0x08000000	/* Luma clock throttle */
#define DIS_LMU_ENABLE_LUMA_CLK_DIV_BY_4		0x0C000000	/* Luma clock throttle */
#define DIS_LMU_ENABLE_LUMA_CLK_DIV_MASK		0x0C000000	/* Mask for luma clock throttle */
#define DIS_LMU_LUMA_FILM_MODE_MCU		      	0x00000000	/* Luma film mode, motion-compensated de-interlacing */
#define DIS_LMU_LUMA_FILM_MODE_FOLLOW_FLD		0x01000000	/* Luma film mode, missing lines from following field */
#define DIS_LMU_LUMA_FILM_MODE_PRECED_FLD		0x02000000	/* Luma film mode, missing lines from preceding field */
#define DIS_LMU_LUMA_BLANK_MISSING_LINES		0x03000000	/* Luma film mode, blank missing lines */
#define DIS_LMU_LUMA_FILM_MODE_MASK	      		0x03000000	/* Mask for luma film mode */
#define DIS_LMU_LUMA_LN_PER_FLD_MASK	      		0x00ff0000	/* Luma film mode, last line used for Acc. Frame Diff. */

#define DIS_LMU_MC_ON_CHROMA_PAIR			0x00004000	/* Motion estimation calculated on maximum of chroma pair */
#define DIS_LMU_DISABLE_CHROMA_TEMP_INTERP		0x00002000	/* Disable chroma temporal interpolation */
#define DIS_LMU_ENABLE_CHROMA_LINE_DOUBLING		0x00001000	/* Enable chroma line doubling */
#define DIS_LMU_ENABLE_CHROMA_CLK_DIV_BY_1		0x00000000	/* Chroma clock throttle */
#define DIS_LMU_ENABLE_CHROMA_CLK_DIV_BY_2		0x00000400	/* Chroma clock throttle */
#define DIS_LMU_ENABLE_CHROMA_CLK_DIV_BY_3		0x00000800	/* Chroma clock throttle */
#define DIS_LMU_ENABLE_CHROMA_CLK_DIV_BY_4		0x00000C00	/* Chroma clock throttle */
#define DIS_LMU_ENABLE_CHROMA_CLK_DIV_MASK		0x00000C00	/* Mask for chroma clock throttle */
#define DIS_LMU_CHROMA_FILM_MODE_MCU			0x00000000	/* Chroma film mode, motion-compensated de-interlacing */
#define DIS_LMU_CHROMA_FILM_MODE_FOLLOW_FLD		0x00000100	/* Chroma film mode, missing lines from following field */
#define DIS_LMU_CHROMA_FILM_MODE_PRECED_FLD		0x00000200	/* Chroma film mode, missing lines from preceding field */
#define DIS_LMU_CHROMA_BLANK_MISSING_LINES		0x00000300	/* Chroma film mode, blank missing lines */
#define DIS_LMU_CHROMA_FILM_MODE_MASK			0x00000300	/* Mask for chroma film mode */
#define DIS_LMU_CHROMA_LN_PER_FLD_MASK 			0x000000ff	/* Chroma film mode, last line used for Acc. Frame Diff. */

#define DIS_SRC_COEFFS_MASK				0x1ff		/* Mask for HFC/VFC, Luma/Chroma coeffs */

#define DIS_LMU_MC_Y_MASK 				0x40000000	/* Mask for Luma Motion Calculation */
#define DIS_LMU_DTI_Y_MASK 				0x20000000	/* Mask for Luma disable temporal interpolation */
#define DIS_LMU_EN_Y_MASK 				0x10000000	/* Mask for Luma LMU enable disable */
#define DIS_LMU_FML_Y_MASK 				0x00ff0000	/* Mask for Luma film mode detection */
#define DIS_LMU_MC_C_MASK 				0x00040000	/* Mask for Chroma Motion Calculation */
#define DIS_LMU_DTI_C_MASK 				0x00020000	/* Mask for Chroma disable temporal interpolation */
#define DIS_LMU_EN_C_MASK 				0x00010000	/* Mask for Chroma LMU enable disable */
#define DIS_LMU_FML_C_MASK 				0x000000ff	/* Mask for Chroma film mode detection */

#define DIS_LMU_ACC_DIFF_Y_MASK 			0x0000ff00	/* Mask for Luma Accumulated difference */
#define DIS_LMU_ACC_DIFF_C_MASK 			0x000000ff	/* Mask for Chroma Accumulated difference */


/* VFC */
#define DIS_VFC_THRU_LUMA			0x1			/* VFC Luma bypass */
#define DIS_VFC_THRU_CHROMA			0x2			/* VFC Chroma bypass */
#define DIS_VFC_COEFFS_NBR      	    	128        	/* Number of VFC coeffs */
#define DIP_VFC_COEFFS_NBR	        	128        	/* Number of VFC coeffs */
#define DIS_VFC_COEFFS_MASK			0x1ff		/* Mask for VFC coeffs */
#define DIS_VFC_ZONE_NBR			5			/* Number of zones for Main VFC */
#define DIP_VFC_ZONE_NBR			5			/* Number of zones for PIP VFC */
#define DIS_VFC_IID_PROG			0x3			/* Progressive Luma and Chroma */
#define DIS_VFC_IID_INTL			0x0			/* Interlaced Luma and Chroma */
#define DIS_VFC_DELTA_MASK			0x1ff		/* Mask for VFC delta */
#define DIS_VFC_ZONE_MASK			0x7ff		/* Mask for VFC zones */
#define DIS_VFC_LSTEP_MASK			0xffff		/* Mask for VFC Line to line step size */
#define DIS_VFC_LSTEP_INIT_MASK	        	0x7fff		/* Mask for VFC vertical subline offset */
#define DIS_VFC_VINL_MASK			0x7ff		/* Mask for VFC number of input lines */
#define DIS_VFC_HINP_MASK			0x7ff		/* Mask for VFC horizontal start position */
#define DIS_VFC_HSINL_MASK			0x7ff		/* Mask for VFC horizontal size */

#define DIP_VFC_DELTA_MASK			0x1ff		/* Mask for VFC delta */
#define DIP_VFC_ZONE_MASK			0x7ff		/* Mask for VFC zones */
#define DIP_VFC_LSTEP_MASK			0xffff		/* Mask for VFC Line to line step size */
#define DIP_VFC_LSTEP_INIT_MASK	        	0x7fff		/* Mask for VFC vertical subline offset */
#define DIP_VFC_VINL_MASK			0x7ff		/* Mask for VFC number of input lines */
#define DIP_VFC_HINP_MASK			0x7ff		/* Mask for VFC horizontal start position */
#define DIP_VFC_HSINL_MASK			0x7ff		/* Mask for VFC horizontal size */

/* HFC */
#define DIS_HFC_LCOEFFS_NBR	    	256        	/* Number of HFC Luma coeffs */
#define DIS_HFC_CCOEFFS_NBR	    	128        	/* Number of HFC Chroma coeffs */

#define DIP_HFC_LCOEFFS_NBR	    	256        	/* Number of HFC Luma coeffs */
#define DIP_HFC_CCOEFFS_NBR	    	128        	/* Number of HFC Chroma coeffs */

#define DIS_HFC_LCOEFFS_MASK		0x1ff		/* Mask for HFC Luma coeffs */
#define DIS_HFC_CCOEFFS_MASK		0x1ff		/* Mask for HFC Chroma coeffs */
#define DIS_HFC_ZONE_NBR			5			/* Number of zones for Main HFC */

#define DIP_HFC_ZONE_NBR			5			/* Number of zones for PIP HFC */

#define DIS_HFC_THRU_MASK			1			/* Mask for HFC through bit */
#define DIS_HFC_YHINL_MASK			0x7ff		/* Mask for HFC number of luma pixels */
#define DIS_HFC_CHINL_MASK			0x7ff		/* Mask for HFC number of chroma pixels */
#define DIS_HFC_DELTA_MASK			0x1ff		/* Mask for HFC delta */
#define DIS_HFC_ZONE_MASK			0x7ff		/* Mask for HFC zones */
#define DIS_HFC_PSTEP_MASK			0x7fff		/* Mask for HFC distance between pixels */
#define DIS_HFC_PSTEP_INIT_MASK		0x3fff		/* Mask for HFC horizontal offset */

#define DIP_HFC_THRU_MASK			1			/* Mask for HFC through bit */
#define DIP_HFC_YHINL_MASK			0x7ff		/* Mask for HFC number of luma pixels */
#define DIP_HFC_CHINL_MASK			0x7ff		/* Mask for HFC number of chroma pixels */
#define DIP_HFC_DELTA_MASK			0x1ff		/* Mask for HFC delta */
#define DIP_HFC_ZONE_MASK			0x7ff		/* Mask for HFC zones */
#define DIP_HFC_PSTEP_MASK			0x7fff		/* Mask for HFC distance between pixels */
#define DIP_HFC_PSTEP_INIT_MASK		0x3fff		/* Mask for HFC horizontal offset */

/* ER */
#define DIS_ER_CTRL_MASK			0x3f		/* Mask for ER gain control */
#define DIS_ER_GAIN_MIN	   			0    		/* Min for ER gain control */
#define DIS_ER_GAIN_MAX 			63   		/* Max for ER gain control */
#define DIS_ER_DELAY_MASK			0x03		/* Mask for ER Delay */
#define DIS_ER_DELAY_MIN			0    		/* Mask for ER Delay */
#define DIS_ER_DELAY_MAX				3			/* Mask for ER Delay */

/* Peaking, Si Compensation */
#define DIS_PK_COREV_MASK 			0x0f        /* Mask for coring for vertical peaking */
#define DIS_PK_COREV_MIN 			0        	/* Min for coring for vertical peaking */
#define DIS_PK_COREV_MAX 			15        	/* Max for coring for vertical peaking */

#define DIS_PK_VGAIN_MASK			0x3f        /* Mask for vertical peaking gain control */
#define DIS_PK_VGAIN_MIN			0        	/* Min for vertical peaking gain control */
#define DIS_PK_VGAIN_MAX			63	        /* Max for vertical peaking gain control */

#define DIS_PK_FILT_MASK			0x07        /* Mask for horizontal peaking filter selection */
#define DIS_PK_FILT_MIN				0        	/* Min for horizontal peaking filter selection */
#define DIS_PK_FILT_MAX				7        	/* Max for horizontal peaking filter selection */

#define DIS_PK_HGAIN_MASK			0x3f        /* Mask for horizontal peaking gain control */
#define DIS_PK_HGAIN_MIN			0        	/* Min for horizontal peaking gain control */
#define DIS_PK_HGAIN_MAX			63        	/* Max for horizontal peaking gain control */

#define DIS_PK_COREH_MASK   		0x0f        /* Mask for coring for horizontal peaking */
#define DIS_PK_COREH_MIN   			0        	/* Min for coring for horizontal peaking */
#define DIS_PK_COREH_MAX   			15        	/* Max for coring for horizontal peaking */

#define DIS_PK_SIEN_ON   			0x01        /* Mask for Si Compensation */
#define DIS_PK_SIEN_OFF   			0        	/* Si Compensation OFF */

/* Auto-Flesh */
#define DIS_AF_QWIDTH_MASK			0x03        /* Mask for quadrature flesh width control */
#define DIS_AF_QWIDTH_MIN			0        	/* Min for quadrature flesh width control */
#define DIS_AF_QWIDTH_MAX			2        	/* Max for quadrature flesh width control */
#define DIS_AF_TINT_MASK			0x3f        /* Mask for tint control */
#define DIS_AF_OFF_MASK 			0x7f        /* Mask for auto flesh control */
#define DIS_AF_MIN		 			0x00        /* Min for auto flesh effect */
#define DIS_AF_MAX		 			0x7f        /* Max for auto flesh effect */
#define DIS_AF_AXIS_MASK   			0x03        /* Mask for flesh axis control */
#define DIS_AF_AXIS_MIN  			0        	/* Min for flesh axis control */
#define DIS_AF_AXIS_MAX   			3        	/* Max for flesh axis control */

/* Green Boost */
#define DIS_GB_OFF_MASK		    	0x3f        /* Mask for green boost gain */
#define DIS_GB_OFF_MIN		    	-32        	/* Min for green boost on/off */
#define DIS_GB_OFF_MAX		    	0        	/* Max for green boost on/off */

/* Tint & Sat */
#define DIP_TS_TINT_MASK   			0x3f        /* Mask for tint control */
#define DIP_TS_TINT_MAX   			31        	/* Max for tint control */
#define DIP_TS_TINT_MIN   			-32        	/* Min for tint control */
#define DIP_TS_SAT_MASK   			0x3f        /* Mask for saturation control */
#define DIP_TS_SAT_MAX   			31          /* Max for saturation control */
#define DIP_TS_SAT_MIN   			0           /* Min for saturation control */
#define DIP_TS_PK_DET_OUT_MASK   	0xff        /* Mask for Frame wise chroma peak detector */
#define DIP_TS_BYPASS_MASK   		0x01        /* Mask for Tint / Sat bypass */

/* DCI */
#define DIS_DCI_COR_MOD_MASK		0x01        /* Mask for DCI coring mode */
#define DIS_DCI_CSC_ON_MASK			0x01        /* Mask for Color Saturation Compensation on/off */
#define DIS_DCI_DCI_ON_MASK			0x01        /* Mask for DCI on/off */
#define DIS_DCI_FREEZE_ANLY_MASK 	0x01        /* Mask for freeze analysis on/off */
#define DIS_DCI_SAT_MASK	    	0x3f        /* Mask for saturation control */
#define DIS_DCI_FILTER_SD			0x00        /* Low-Pass filter for SD (fl/fs = 0.05) */
#define DIS_DCI_FILTER_HD			0x01        /* Low-Pass filter for HD (fl/fs = 0.025) */
#define DIS_DCI_FILTER_MASK			0x01        /* Low-Pass filter DCI mask */
#define DIS_DCI_COR_MOD_SNR_ADAPT 	0x01        /* Noise adaptive coring mode */
#define DIS_DCI_COR_MASK			0x3f        /* Mask for fixed coring level */
#define DIS_DCI_LSTHR_MASK	    	0xff        /* Mask for light sample threshold */
#define DIS_DCI_LSWF_MASK       	0x0f        /* Mask for light sample weighting factor */
#define DIS_DCI_DSTHR_MASK	    	0xff        /* Mask for dark sample threshold */
#define DIS_DCI_SENSWS_MASK	    	0x7fff      /* Mask for sensitivity of ABA */
#define DIS_DCI_ELINE_MASK	    	0x7ff       /* Mask for last line of DCI analysis window */
#define DIS_DCI_SLINE_MASK	    	0x7ff       /* Mask for First line of DCI analysis window */
#define DIS_DCI_EPIXEL_MASK	    	0x7ff       /* Mask for Last pixel of DCI analysis window */
#define DIS_DCI_SPIXEL_MASK	    	0x7ff       /* Mask for First pixel of DCI analysis window */
#define DIS_DCI_SNR_COR_MASK	   	0x3f        /* Mask for signal to noise ratio for coring */

#define DIS_DCI_DLTHR_MASK			0xff      	/* Mask for dark level threshold for DSDA */
#define DIS_DCI_DYTC_MASK			0x3f      	/* Mask for dark area size for DSDA */
#define DIS_DCI_ERR_COMP_MASK 		0x3fff     	/* Mask for DSD error compensation */
#define DIS_DCI_SENSBS_MASK   		0x3fff      /* Mask for sensitivity of DSDA */
#define DIS_DCI_DRKS_DIST_MASK	    0xfffff     /* Mask for output of dark sample distribution */
#define DIS_DCI_AVRG_BR_MASK	    0x3ffff     /* Mask for average picture brightness */

#define DIS_DCI_TF_DPP_MASK		    0x3ff       /* Mask for transfer function dynamic pivot point */
#define DIS_DCI_TF_DPP_MIN		    7       	/* Min (recommended) for transfer function dynamic pivot point */
#define DIS_DCI_TF_DPP_MAX		    409       	/* Max (recommended) for transfer function dynamic pivot point */
#define DIS_DCI_GAIN_SEG1_FRC_MASK  0xfff       /* Mask for frac part of gain for 1st seg of xfer function */
#define DIS_DCI_GAIN_FRC_MIN  		0       	/* Min for frac part of gain for 1st seg of xfer function */
#define DIS_DCI_GAIN_FRC_MAX  		2864       	/* Max for frac part of gain for 1st seg of xfer function */
#define DIS_DCI_GAIN_SEG2_FRC_MASK  0xfff       /* Mask for frac part of gain for 2nd seg of xfer function */

#define DIS_DCI_PEAK_SIZE_MASK	    0x0f        /* Mask for peak area size */

#define DIS_DCI_PEAK_N_MASK		    0x0f        /* Mask for upper limit control for peak detection algorithm */

#define DIS_DCI_CBCR_PEAK_MASK	    0x7ff       /* Mask for output of chroma peak detector */

#define DIS_DCI_FR_PEAK_MASK	    0x7fff      /* Mask for counter output for peak detection */

#define DIS_DCI_PEAK_THR_MASK	    0xff        /* Mask for peak detection thresholds */

#define DIS_DCI_BYPASS_EN  	    	0x1         /* Enable bypass of PSI process */
#define DIS_DCI_BYPASS_MASK  	   	0x1         /* Mask for bypass of PSI process */

/* Interrupts */
#define DIS_IT_CIFD                                                           0x001                           /* Chroma Input Field */
#define DIS_IT_YIFD                                                           0x002                           /* Luma Input Field */
#define DIS_IT_EAW                                                            0x400                           /* End of Analysis Window */

#ifdef __cplusplus
}
#endif

#endif /* __DISPLAY_REG_H */
/* ------------------------------- End of file ---------------------------- */


