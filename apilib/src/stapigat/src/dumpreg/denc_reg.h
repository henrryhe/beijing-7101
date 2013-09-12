/*****************************************************************************

File name   : denc_reg.h

Description : DENC register addresses

COPYRIGHT (C) ST-Microelectronics 1999.

Date               Modification                 			Name
----               ------------                 			----
13-Sep-99          Created                      			FC
04-Jan-00	   Added xxx_BASE_ADDRESS defines 			FC
29-Feb-00	   Update, solved conflict with CFG			FC
27-Avr-01          Use new base address system                          XD
TODO:
-----
*****************************************************************************/

#ifndef __DENC_REG_H
#define __DENC_REG_H

/* Includes --------------------------------------------------------------- */

#include "stdevice.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

#if defined (ST_7015)
/* Register base address */
#define DENC (STI7015_BASE_ADDRESS+ST7015_DENC_OFFSET)
#else
/* Register base address */
#define DENC (STI7020_BASE_ADDRESS+ST7020_DENC_OFFSET)
#endif

/* Register offsets */
/* Configuration registers */
#define DENC_CFG0   		0x00           	/* Configuration 0 */
#define DENC_CFG1   		0x04           	/* Configuration 1 */
#define DENC_CFG2   		0x08           	/* Configuration 2 */
#define DENC_CFG3   		0x0C           	/* Configuration 3 */
#define DENC_CFG4   		0x10           	/* Configuration 4 */
#define DENC_CFG5   		0x14           	/* Configuration 5 */
#define DENC_CFG6   		0x18           	/* Configuration 6 */
#define DENC_CFG7   		0x1C           	/* Configuration 7 */
#define DENC_CFG8   		0x20           	/* Configuration 8 */
#define DENC_CFG9   		0x144          	/* Configuration 9 */
#define DENC_STATUS   		0x24           	/* Status */

/* Digital Frequency Synthesizer */
#define DENC_DFS_INC2   	0x28           	/* Increment for DFS, byte 2 */
#define DENC_DFS_INC1   	0x2C           	/* Increment for DFS, byte 1 */
#define DENC_DFS_INC0   	0x30           	/* Increment for DFS, byte 0 */
#define DENC_DFS_PHASE1		0x34           	/* Phase DFS, byte 1 */
#define DENC_DFS_PHASE0		0x38           	/* Phase DFS, byte 0 */

#define DENC_WSS_DATA1  	0x3C           	/* WSS Data, byte 1 */
#define DENC_WSS_DATA0  	0x40           	/* WSS Data, byte 0 */

#define DENC_DAC13_MULT		0x44            /* DAC1 & DAC3 Multiplication factor */
#define DENC_DAC45_MULT		0x48            /* DAC4 & DAC5 Multiplication factor */
#define DENC_DAC6C_MULT		0x4C            /* DAC6 & C Multiplication factor */

#define DENC_LINE2   		0x54            /* Target and Reference lines, byte 2 */
#define DENC_LINE1   		0x58            /* Target and Reference lines, byte 1 */
#define DENC_LINE0   		0x5C            /* Target and Reference lines, byte 0 */

#define DENC_CHIPID   		0x60            /* DENC Identification Number */

#define DENC_VPS5   		0x64            /* VPS Data Register, byte 5 */
#define DENC_VPS4   		0x68            /* VPS Data Register, byte 4 */
#define DENC_VPS3   		0x6C            /* VPS Data Register, byte 3 */
#define DENC_VPS2   		0x70            /* VPS Data Register, byte 2 */
#define DENC_VPS1   		0x74            /* VPS Data Register, byte 1 */
#define DENC_VPS0   		0x78            /* VPS Data Register, byte 0 */

#define DENC_CGMS0   		0x7C            /* CGMS Register, byte 0 */
#define DENC_CGMS1   		0x80            /* CGMS Register, byte 1 */
#define DENC_CGMS2   		0x84            /* CGMS Register, byte 2 */

#define DENC_TTX1   		0x88            /* TTX Conf/Lines 1 */
#define DENC_TTX2   		0x8C            /* TTX Lines 2 */
#define DENC_TTX3   		0x90            /* TTX Lines 3 */
#define DENC_TTX4   		0x94            /* TTX Lines 4 */
#define DENC_TTX5   		0x98            /* TTX Lines 5 */
#define DENC_TTX_CONF  		0x100           /* TTX configuration */
#define DENC_D2M_TTX   		0x104           /* DAC2Mult & TTX */

#define DENC_CCCF10   		0x9C            /* Closed Caption Characters/Extended Data for Field 1, byte 0 */
#define DENC_CCCF11   		0xA0            /* Closed Caption Characters/Extended Data for Field 1, byte 1 */
#define DENC_CCCF20   		0xA4            /* Closed Caption Characters/Extended Data for Field 2, byte 0 */
#define DENC_CCCF21   		0xA8            /* Closed Caption Characters/Extended Data for Field 2, byte 1 */
#define DENC_CCLIF1   		0xAC            /* Closed Caption/Extended Data line insertion for Field 1 */
#define DENC_CCLIF2   		0xB0            /* Closed Caption/Extended Data line insertion for Field 2 */


#define DENC_BRIGHT   		0x114           /* Brightness */
#define DENC_CONTRAST   	0x118           /* Contrast */
#define DENC_SATURATION 	0x11C           /* Saturation */

#define DENC_CCOEF0			0x120			/* Chroma Coef 0 */
#define DENC_CCOEF1			0x124			/* Chroma Coef 1 */
#define DENC_CCOEF2			0x128			/* Chroma Coef 2 */
#define DENC_CCOEF3			0x12C			/* Chroma Coef 3 */
#define DENC_CCOEF4			0x130			/* Chroma Coef 4 */
#define DENC_CCOEF5			0x134			/* Chroma Coef 5 */
#define DENC_CCOEF6			0x138			/* Chroma Coef 6 */
#define DENC_CCOEF7			0x13C			/* Chroma Coef 7 */
#define DENC_CCOEF8			0x140			/* Chroma Coef 8 */

#define DENC_LCOEF0			0x148			/* Luma Coef 0 */
#define DENC_LCOEF1			0x14C			/* Luma Coef 1 */
#define DENC_LCOEF2			0x150			/* Luma Coef 2 */
#define DENC_LCOEF3			0x154			/* Luma Coef 3 */
#define DENC_LCOEF4			0x158			/* Luma Coef 4 */
#define DENC_LCOEF5			0x15C			/* Luma Coef 5 */
#define DENC_LCOEF6			0x160			/* Luma Coef 6 */
#define DENC_LCOEF7			0x164			/* Luma Coef 7 */
#define DENC_LCOEF8			0x168			/* Luma Coef 8 */
#define DENC_LCOEF9			0x16C			/* Luma Coef 9 */

#define DENC_CFG   			0x1C0           /* Denc Output Stage Configuration */
#define DENC_TTX   			0x1C4           /* Denc TTX */



/* DENC_CFG0 - Configuration register 0  (8-bit)---------------------------- */
#define DENC_CFG0_MASK_STD   0x3F /* Mask for standard selected              */
#define DENC_CFG0_PAL_BDGHI  0x00 /* PAL B, D, G, H or I standard selected   */
#define DENC_CFG0_PAL_N      0x40 /* PAL N standard selected                 */
#define DENC_CFG0_NTSC_M     0x80 /* PAL NTSC standard selected              */
#define DENC_CFG0_PAL_M      0xC0 /* PAL M standard selected                 */
#define DENC_CFG0_MASK_SYNC  0xC7 /* Mask for synchro configuration          */
#define DENC_CFG0_ODDE_SLV   0x00 /* ODDEVEN based slave mode (frame lock)   */
#define DENC_CFG0_FRM_SLV    0x08 /* Frame only based slave mode(frame lock) */
#define DENC_CFG0_ODHS_SLV   0x10 /* ODDEVEN + HSYNC based slave mode(line l)*/
#define DENC_CFG0_FRHS_SLV   0x18 /* Frame + HSYNC based slave mode(line l)  */
#define DENC_CFG0_VSYNC_SLV  0x20 /* VSYNC only based slave mode(frame l   ) */
#define DENC_CFG0_VSHS_SLV   0x28 /* VSYNC + HSYNC based slave mode(line l  )*/
#define DENC_CFG0_MASTER     0x30 /* Master mode selected                    */
#define DENC_CFG0_COL_BAR    0x38 /* Test color bar pattern enabled          */
#define DENC_CFG0_HSYNC_POL  0x04 /* HSYNC positive pulse                    */
#define DENC_CFG0_ODD_POL    0x02 /* Synchronisation polarity selection      */
#define DENC_CFG0_FREE_RUN   0x01 /* Freerun On                              */

/* DENC_CFG1 - Configuration register 1  (8-bit)---------------------------- */
#define DENC_CFG1_VBI_SEL      0x80 /* Full VBI selected                     */
#define DENC_CFG1_MASK_FLT     0x9F /* Mask for U/V Chroma filter bandwith   */
                                    /* selection                             */
#define DENC_CFG1_MASK_SYNC_OK 0xEF /* mask for sync in case of frame loss   */
#define DENC_CFG1_FLT_11       0x00 /* FLT Low definition NTSC filter        */
#define DENC_CFG1_FLT_13       0x20 /* FLT Low definition PAL filter         */
#define DENC_CFG1_FLT_16       0x40 /* FLT High definition NTSC filter       */
#define DENC_CFG1_FLT_19       0x60 /* FLT High definition PAL filter        */
#define DENC_CFG1_SYNC_OK      0x10 /* Synchronisation avaibility            */
#define DENC_CFG1_COL_KILL     0x08 /* Color suppressed on CVBS              */
#define DENC_CFG1_SETUP        0x04 /* Pedestal setup (7.5 IRE)              */
#define DENC_CFG1_MASK_CC      0xFC /* Mask for Closed caption encoding mode */
#define DENC_CFG1_CC_DIS       0x00 /* Closed caption data encoding disabled */
#define DENC_CFG1_CC_ENA_F1    0x01 /* Closed caption enabled in field 1     */
#define DENC_CFG1_CC_ENA_F2    0x02 /* Closed caption enabled in field 2     */
#define DENC_CFG1_CC_ENA_BOTH  0x03 /* Closed caption enabled in both fields */
#define DENC_CFG1_DAC_INV      0x80 /* Enable DAC input data inversion       */

/* DENC_CFG2 - Configuration register 2  (8-bit)---------------------------- */
#define DENC_CFG2_NO_INTER     0x80 /* Non-interlaced mode selected          */
#define DENC_CFG2_ENA_RST      0x40 /* Cyclic phase reset enabled            */
#define DENC_CFG2_ENA_BURST    0x20 /* Chrominance burst enabled             */
#define DENC_CFG2_SEL_444      0x10 /* Select 444 input for RGB tri-dacs     */
#define DENC_CFG2_SEL_RST      0x08 /* Reset DDFS with value on DNC_IFx reg. */
#define DENC_CFG2_RST_OSC      0x04 /* Software phase reset of DDFS          */
#define DENC_CFG2_RST_4F       0x02 /* Reset DDFS every 4 fields             */
#define DENC_CFG2_RST_2F       0x01 /* Reset DDFS every 2 fields             */
#define DENC_CFG2_RST_EVE      0x00 /* Reset DDFS every line                 */
#define DENC_CFG2_RST_8F       0x03 /* Reset DDFS every 8 fields             */
#define DENC_CFG2_MASK_RST     0xFC /* Mask for reset DDFS mode              */

/* DENC_CFG3 - Configuration register 3  (8-bit) --------------------------- */
#define DENC_CFG3_ENA_TRFLT    0x80 /* Enable Trap filter                    */
#define DENC_CFG3_PAL_TRFLT    0x40 /* select Trap filter for PAL            */
#define DENC_CFG3_ENA_CGMS     0x20 /* Enable CGMS endoding                  */
#define DENC_CFG3_DELAY_EN     0x04 /* +2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_ENA_WSS      0x01 /* wide screen signalling enable         */

/* DENC_CFG4 - Configuration register 4  (8-bit) --------------------------- */
#define DENC_CFG4_MASK_SYIN    0x3F /* Mask for adjustment of incoming       */
                                    /* synchro signals                       */
#define DENC_CFG4_SYIN_0       0x00 /* nominal delay                         */
#define DENC_CFG4_SYIN_P1      0x40 /* delay = +1 ckref                      */
#define DENC_CFG4_SYIN_P2      0x80 /* delay = +2 ckref                      */
#define DENC_CFG4_SYIN_P3      0xC0 /* delay = +3 ckref                      */
#define DENC_CFG4_MASK_SYOUT   0xCF /* Mask for adjustment of outgoing       */
                                   /* synchro signals                       */
#define DENC_CFG4_SYOUT_0      0x00 /* nominal delay                         */
#define DENC_CFG4_SYOUT_P1     0x10 /* delay = +1 ckref                      */
#define DENC_CFG4_SYOUT_P2     0x20 /* delay = +2 ckref                      */
#define DENC_CFG4_SYOUT_P3     0x30 /* delay = +3 ckref                      */
#define DENC_CFG4_SYIN_P1      0x40 /* delay = +1 ckref                      */
#define DENC_CFG4_SYIN_P2      0x80 /* delay = +2 ckref                      */
#define DENC_CFG4_SYIN_P3      0xC0 /* delay = +3 ckref                      */
#define DENC_CFG4_ALINE        0x08 /* Video active line duration control    */
#define DENC_CFG4_MASK_DELAY   0xF8 /* Mask for delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_P2     0x02 /* +2 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_P1     0x01 /* +1 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_0      0x00 /* +0 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_M1     0x07 /* -1 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_M2     0x06 /* -2 pixel delay on luma 4:4:4 inputs   */

/* DENC_CFG5 - Configuration register 5  (8-bit) --------------------------- */
#define DENC_CFG5_SEL_INC      0x80 /* Choice of Dig Freq Synthe increment   */
#define DENC_CFG5_DIS_DAC1     0x40 /* DAC 1 input forced to 0               */
#define DENC_CFG5_DIS_DAC2     0x20 /* DAC 2 input forced to 0               */
#define DENC_CFG5_DIS_DAC3     0x10 /* DAC 3 input forced to 0               */
#define DENC_CFG5_DIS_DAC4     0x08 /* DAC 4 input forced to 0               */
#define DENC_CFG5_DIS_DAC5     0x04 /* DAC 5 input forced to 0               */
#define DENC_CFG5_DIS_DAC6     0x02 /* DAC 6 input forced to 0               */
#define DENC_CFG5_DAC_INV      0x01 /* Enable DAC input data inversion       */
#define DENC_CFG5_DAC_NONINV   0x00 /* Enable DAC input data non inversion   */

/* DENC_CFG6 - Configuration register 6 ( 5 bits, 3 unused bits ) ---------- */
#define DENC_CFG6_RST_SOFT     0x80 /* Denc soft reset                       */
#define DENC_CFG6_MASK_LSKP    0x8F /* mask for line skip configuration      */
#define DENC_CFG6_NORM_MODE    0x00 /* normal mode, no insert/skip capable   */
#define DENC_CFG6_MAN_MODE     0x10 /* same as normal, unless skip specified */
#define DENC_CFG6_AUTO_INS     0x40 /* automatic line insert mode            */
#define DENC_CFG6_AUTO_SKP     0x60 /* automatic line skip mode              */
#define DENC_CFG6_FORBIDDEN    0x70 /* Reserved, don't write this value      */
#define DENC_CFG6_MAX_DYN      0x01 /* Maximum dynamic range 1-254 ( 16-240) */
#define DENC_CFG6_TTX_ENA      0x02 /* Teletexte enable bit                  */

/* DENC_CFG7 - Configuration register 7 ( SECAM mainly ) ------------------- */
#define DENC_CFG7_SECAM        0x80 /* Select SECAM chroma encoding on top   */
                                    /* of config selected in DENC_CFG0       */
#define DENC_CFG7_PHI12_SEC    0x40 /* sub carrier phase sequence start      */
#define DENC_CFG7_INV_PHI_SEC  0x20 /* invert phases on second field         */
#define DENC_CFG7_SETUP_YUV    0x08 /* Control of pedestal enabled           */
#define DENC_CFG7_UV_LEV       0x04 /* UV output level control               */
#define DENC_CFG7_ENA_VPS      0x02 /* enable video programming system       */
#define DENC_CFG7_SQ_PIX       0x01 /* enable square pixel mode (PAL/NTSC)   */

/* DENC_CFG8 - Configuration register 8 ( 5 MSB only used ) ---------------- */
  /* specific configuration of outputs  dac1  dac2  dac3  dac4  dac5  dac6   */
#define DENC_CFG8_CONF_OUT_0   0x00 /*   Y     C    CVBS   C     Y    CVBS   */
#define DENC_CFG8_CONF_OUT_1   0x10 /*   Y     C    CVBS   V     Y     U     */
#define DENC_CFG8_CONF_OUT_2   0x20 /*   Y     C    CVBS   R     G     B     */
#define DENC_CFG8_MASK_CONF    0xCF /* mask for configuration of outputs     */
#define DENC_CFG8_PH_RST_0     0x40 /* sub-carrier phase reset mode          */
#define DENC_CFG8_PH_RST_1     0x80 /* sub-carrier phase reset mode          */
#define DENC_CFG8_MASK_PH_RST  0x3F /* mask for sub-carrier phase reset mode */
#define DENC_CFG8_BLK_ALL      0x08 /* blanking of all video lines           */

/* DENC_CFG9 - Configuration register 9 (status, ReadOnly) ----------------- */
#define DENC_CFG9_FLT_YS  	   0x01 /* Enable software luma coeffs			 */
#define DENC_CFG9_PLG_DIV_Y_0  0x02 /* Sum of coefficients					 */
#define DENC_CFG9_PLG_DIV_Y_1  0x04 /* Sum of coefficients					 */
#define DENC_CFG9_MASK_PLG_DIV 0xF9 /* Mask for sum of coefficients			 */
#define DENC_CFG9_444_CVBS     0x08 /* Set 4:4:4 for CVBS 					 */
#define DENC_CFG9_DEL_0        0x10 /* Delay bit 0							 */
#define DENC_CFG9_DEL_1        0x20 /* Delay bit 1							 */
#define DENC_CFG9_DEL_2        0x40 /* Delay bit 2							 */
#define DENC_CFG9_DEL_3        0x80 /* Delay bit 3							 */
#define DENC_CFG3_MASK_DELAY   0x0F /* Mask for delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P4     0x40 /* +4 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P2     0x20 /* +2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P1     0x10 /* +1 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_0      0x00 /* +0 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M1     0xF0 /* -1 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M2     0xE0 /* -2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M4     0xC0 /* -4 pixel delay on luma 4:2:2 inputs   */

/* DENC_CCOEF0 - Chroma Coef 0 and config ---------------------------------- */
#define DENC_CCOEF0_FLT_S	   0x80	/* Chroma Filter Selection               */
#define DENC_CCOEF0_DIV_MASK   0x9F	/* Mask for chroma Filter Division       */
#define DENC_CCOEF0_DIV1   	   0x40	/* Chroma Filter Division bit 1          */
#define DENC_CCOEF0_DIV0   	   0x20	/* Chroma Filter Division bit 0          */
#define DENC_CCOEF0_MASK   	   0x1F	/* Mask for Chroma Filter Coef 0         */

/* DENC_CCOEF1 - Chroma Coef 1 and config ---------------------------------- */
#define DENC_CCOEF1_MASK   	   0x3F	/* Mask for Chroma Filter Coef 1         */

/* DENC_CCOEF2 - Chroma Coef 2 and config ---------------------------------- */
#define DENC_CCOEF2_MASK   	   0x7F	/* Mask for Chroma Filter Coef 2         */

/* DENC_CCOEF3 - Chroma Coef 3 and config ---------------------------------- */
#define DENC_CCOEF3_MASK   	   0x7F	/* Mask for Chroma Filter Coef 3         */

/* DENC_CCOEF4 - Chroma Coef 4 and config ---------------------------------- */
#define DENC_CCOEF4_MASK   	   0xFF	/* Mask for Chroma Filter Coef 4         */

/* DENC_CCOEF5 - Chroma Coef 5 and config ---------------------------------- */
#define DENC_CCOEF5_MASK   	   0xFF	/* Mask for Chroma Filter Coef 5         */

/* DENC_CCOEF6 - Chroma Coef 6 and config ---------------------------------- */
#define DENC_CCOEF6_MASK   	   0xFF	/* Mask for Chroma Filter Coef 6         */

/* DENC_CCOEF7 - Chroma Coef 7 and config ---------------------------------- */
#define DENC_CCOEF7_MASK   	   0xFF	/* Mask for Chroma Filter Coef 7         */

/* DENC_CCOEF8 - Chroma Coef 8 and config ---------------------------------- */
#define DENC_CCOEF8_MASK   	   0xFF	/* Mask for Chroma Filter Coef 8         */



/* DENC_LCOEF0 - Luma Coef 0 and config ---------------------------------- */
#define DENC_LCOEF0_MASK   	   0x1F	/* Mask for Luma Filter Coef 0         */

/* DENC_LCOEF1 - Luma Coef 1 and config ---------------------------------- */
#define DENC_LCOEF1_MASK   	   0x3F	/* Mask for Luma Filter Coef 1         */

/* DENC_LCOEF2 - Luma Coef 2 and config ---------------------------------- */
#define DENC_LCOEF2_MASK   	   0x7F	/* Mask for Luma Filter Coef 2         */

/* DENC_LCOEF3 - Luma Coef 3 and config ---------------------------------- */
#define DENC_LCOEF3_MASK   	   0x7F	/* Mask for Luma Filter Coef 3         */

/* DENC_LCOEF4 - Luma Coef 4 and config ---------------------------------- */
#define DENC_LCOEF4_MASK   	   0xFF	/* Mask for Luma Filter Coef 4         */

/* DENC_LCOEF5 - Luma Coef 5 and config ---------------------------------- */
#define DENC_LCOEF5_MASK   	   0xFF	/* Mask for Luma Filter Coef 5         */

/* DENC_LCOEF6 - Luma Coef 6 and config ---------------------------------- */
#define DENC_LCOEF6_MASK   	   0xFF	/* Mask for Luma Filter Coef 6         */

/* DENC_LCOEF7 - Luma Coef 7 and config ---------------------------------- */
#define DENC_LCOEF7_MASK   	   0xFF	/* Mask for Luma Filter Coef 7         */

/* DENC_LCOEF8 - Luma Coef 8 and config ---------------------------------- */
#define DENC_LCOEF8_MASK   	   0xFF	/* Mask for Luma Filter Coef 8         */

/* DENC_LCOEF9 - Luma Coef 9 and config ---------------------------------- */
#define DENC_LCOEF9_MASK   	   0xFF	/* Mask for Luma Filter Coef 9         */

#ifdef __cplusplus
}
#endif

#endif /* __DENC_REG_H */
/* ------------------------------- End of file ---------------------------- */


