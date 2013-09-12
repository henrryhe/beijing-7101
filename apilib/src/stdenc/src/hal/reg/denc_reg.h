/*******************************************************************************
File Name   : denc_reg.h

Description : DENC V3/V5/V6/V7/V8/V9/V10/V11/V12 registers

COPYRIGHT (C) STMicroelectronics 2003

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DENC_REG_H
#define __DENC_REG_H

/* Includes ----------------------------------------------------------------- */
#include "stdenc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

/*
  register map for DENC V3/ V5/ V6/ V7/ V8/ V9/ V10/ V11/ V12 version
*/
#ifndef DENC_CFG0_RESET_VALUE
#define DENC_CFG0_RESET_VALUE  0x92
#endif

#ifndef DENC_CFG1_RESET_VALUE
#define DENC_CFG1_RESET_VALUE  0x44
#endif

#ifndef DENC_CFG2_RESET_VALUE
#define DENC_CFG2_RESET_VALUE  0x20
#endif

#ifndef DENC_CFG3_RESET_VALUE
#define DENC_CFG3_RESET_VALUE  0x00
#endif

#ifndef DENC_CFG4_RESET_VALUE
#define DENC_CFG4_RESET_VALUE  0x00
#endif

#ifndef DENC_CFG5_RESET_VALUE
#define DENC_CFG5_RESET_VALUE  0x00
#endif

#ifndef DENC_CFG6_RESET_VALUE
#define DENC_CFG6_RESET_VALUE  0x10
#endif

#ifndef DENC_CFG7_RESET_VALUE
#define DENC_CFG7_RESET_VALUE  0x00
#endif

#ifndef DENC_CFG8_RESET_VALUE
#define DENC_CFG8_RESET_VALUE  0x20
#endif

#ifndef DENC_CFG9_RESET_VALUE
#define DENC_CFG9_RESET_VALUE  0x22
#endif

#ifndef DENC_CFG10_RESET_VALUE
#define DENC_CFG10_RESET_VALUE 0x48
#endif

#ifndef DENC_CFG11_RESET_VALUE
#define DENC_CFG11_RESET_VALUE 0x2c /*0x28*/ /*yxl 2007-09-06 modify*/
#endif

#ifndef DENC_CFG12_RESET_VALUE
#define DENC_CFG12_RESET_VALUE 0x00
#endif

#ifndef DENC_CFG13_RESET_VALUE
#define DENC_CFG13_RESET_VALUE 0x82
#endif
#if defined(ST_5162)
#define DENC_HUE_CONTROL           0x69
#define DENC_CFG3_CLK_FROM_PAD     0x04 /* VIDPIX_2XCLK_from_pad being input     */
#define DENC_CFG8_422_FROM_PAD     0x10 /* YCrCb_422_from_pad being input        */
#define DENC_CFG10_SEC_IN_MAIN     0x00 /* main video input is SECAM encoded     */
#define DENC_CFG10_RGB_SAT         0x00 /* more saturated colors on RGBoutputs   */
#define DENC_CFG8_BLK_ALL          0x08 /* Blanking of all video lines           */
#endif

/* DEN - Digital encoder registers - standard 8 bit calls     --------_----- */
#define DENC_CFG0   0x00  /* Configuration register 0                [7:0]   */
#define DENC_CFG1   0x01  /* Configuration register 1                [7:0]   */
#define DENC_CFG2   0x02  /* Configuration register 2                [7:0]   */
#define DENC_CFG3   0x03  /* Configuration register 3                [7:0]   */
#define DENC_CFG4   0x04  /* Configuration register 4                [7:0]   */
#define DENC_CFG5   0x05  /* Configuration register 5                [7:0]   */
#define DENC_CFG6   0x06  /* Configuration register 6                [7:0]   */
/* register macrocell V6/V7/V8/V9/V10/V11/V12 */
#define DENC_CFG7   0x07  /* Configuration register 7, SECAM         [7:0]   */
#define DENC_CFG8   0x08  /* Configuration register 8                [7:0]   */
/* end 'register macrocell V6/V7/V8/V9/V10/V11/V12' */
/* register macrocell V10/V11/V12 */
#define DENC_CFG9   0x51  /* Configuration register 9                [7:0]   */
/* end 'register macrocell V10/V11/V12' */
/* register macrocell V12 */
#define DENC_CFG10  0x5C  /* Configuration register 10               [7:0]   */
#define DENC_CFG11  0x5D  /* Configuration register 11               [7:0]   */
#define DENC_CFG12  0x5E  /* Configuration register 12               [7:0]   */
#define DENC_CFG13  0x5F  /* Configuration register 13               [7:0]   */
/* end 'register macrocell V12' */

#define DENC_IDFS2  0x0A  /* Increment digital frequency synthesiser [23:16] */
#define DENC_IDFS1  0x0B  /* Increment digital frequency synthesiser [15:8]  */
#define DENC_IDFS0  0x0C  /* Increment digital frequency synthesiser [7:0]   */

/* register macrocell V3/V5 */
#define DENC_CH_ID  0x11  /* Denc Chip Identification                        */
/* end 'register macrocell V3/V5' */
/* register macrocell V6/V7/V8/V9/V10/V11/V12 */
#define DENC_DAC13  0x11  /* Denc gain for tri-dac 1-3               [7:0]   */
#define DENC_CID    0x18  /* Denc macro-cell identification number   [7:0]   */
/* end 'register macrocell V6/V7/V8/V9/V10/V11/V12' */

#define DENC_ISCID  DENC_CH_ID  /* test for chid : DENC_CH_ID R  for v3 v5   */
                                /*                 DENC_DAC13 RW for v6 v12  */

/* DENC_CFG0 - Configuration register 0  (8-bit)---------------------------- */
#define DENC_CFG0_MASK_STD   0x3F /* Mask for standard selected              */
#define DENC_CFG0_PAL_BDGHI  0x00 /* PAL B, D, G, H or I standard selected   */
#define DENC_CFG0_PAL_N      0x40 /* PAL N standard selected                 */
#define DENC_CFG0_NTSC_M     0x80 /* NTSC M  standard selected               */
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
/* register macrocell V3/V5/V6/V7/V8/V9/V10/V11 */
#define DENC_CFG2_SEL_444      0x10 /* Select 444 input for RGB tri-dacs     */
/* end register macrocell V3/V5/V6/V7/V8/V9/V10/V11 */
#define DENC_CFG2_SEL_RST      0x08 /* Reset DDFS with value on DNC_IFx reg. */
#define DENC_CFG2_RST_OSC      0x04 /* Software phase reset of DDFS          */
#define DENC_CFG2_MASK_RST     0xFC /* Mask for reset DDFS mode              */
#define DENC_CFG2_RST_8F       0x03 /* Reset DDFS every 8 fields             */
#define DENC_CFG2_RST_4F       0x02 /* Reset DDFS every 4 fields             */
#define DENC_CFG2_RST_2F       0x01 /* Reset DDFS every 2 fields             */
#define DENC_CFG2_RST_EVE      0x00 /* Reset DDFS every line                 */


/* DENC_CFG3 - Configuration register 3  (8-bit) --------------------------- */
#define DENC_CFG3_ENA_TRFLT    0x80 /* Enable Trap filter                    */
#define DENC_CFG3_PAL_TRFLT    0x40 /* select Trap filter 4,43 MHz           */
#define DENC_CFG3_ENA_CGMS     0x20 /* Enable CGMS encoding                  */
#define DENC_CFG3_VAL_422_CK_MUX 0x04 /* Enable external clock for debug and validation */
/* register macrocell V3/V5/V6/V7/V8/V9 */
#define DENC_CFG3_MASK_DELAY   0xF1 /* Mask for delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P2     0x04 /* +2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P1     0x02 /* +1 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_0      0x00 /* +0 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M1     0x0E /* -1 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M2     0x0C /* -2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_ENA_WSS      0x01 /* wide screen signalling enable         */
/* end register macrocell V3/V5/V6/V7/V8/V9 */
/* register macrocell V3/V5 */
#define DENC_CFG3_NOSD         0x10 /*choice of active edge  of 'denc_ref_ck'*/
/* end register macrocell V3/V5 */
/* register macrocell V7/V8/V9/V10/V11/V12 */
#define DENC_CFG3_CK_IN_PHASE  0x10 /*choice of active edge  of 'denc_ref_ck'*/
/* end register macrocell V7/V8/V9/V10/V11/V12 */
/* register macrocell V10/V11/V12 */
#define DENC_CFG3_DELAY_ENABLE 0x08 /* enable of chroma to luma delay        */
/* end register macrocell V10/V11/V12 */


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
#define DENC_CFG4_ALINE        0x08 /* Video active line duration control    */
/* register macrocell V3/V5/V6 */
#define DENC_CFG4_MASK_TXD     0xF8 /* Mask for teletext data latency        */
#define DENC_CFG4_TXT_LAT_2    0x00 /* teletext data latency 2=2+0           */
#define DENC_CFG4_TXT_LAT_3    0x01 /* teletext data latency 3=2+1           */
#define DENC_CFG4_TXT_LAT_4    0x02 /* teletext data latency 4=2+2           */
#define DENC_CFG4_TXT_LAT_5    0x03 /* teletext data latency 5=2+3           */
#define DENC_CFG4_TXT_LAT_6    0x04 /* teletext data latency 6=2+4           */
#define DENC_CFG4_TXT_LAT_7    0x05 /* teletext data latency 7=2+5           */
#define DENC_CFG4_TXT_LAT_8    0x06 /* teletext data latency 8=2+6           */
#define DENC_CFG4_TXT_LAT_9    0x07 /* teletext data latency 9=2+7           */
/* end register macrocell V3/V5/V6 */
/* register macrocell V7/V8/V9/10/V11*/
#define DENC_CFG4_MASK_DELAY   0xF8 /* Mask for delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_P2     0x02 /* +2 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_P1     0x01 /* +1 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_0      0x00 /* +0 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_M1     0x07 /* -1 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_M2     0x06 /* -2 pixel delay on luma 4:4:4 inputs   */
/* end register macrocell V7/V8/V9/10/V11 */


/* DENC_CFG5 - Configuration register 5  (8-bit) --------------------------- */
/* register macrocell V3 */
#define DENC_CFG5_MASK_CONF    0x7F /* mask for configuration of outputs     */
#define DENC_CFG5_RGB          0x80 /* outputs selection : R-G-B-CVBS1       */
#define DENC_CFG5_NYC          0x00 /* outputs selection : Y-C-CVBS-CVBS1    */
#define DENC_CFG5_DIS_CVBS1    0x40 /*  */
#define DENC_CFG5_DIS_YS_V3    0x08 /*  */
#define DENC_CFG5_DIS_C_V3     0x04 /*  */
#define DENC_CFG5_DIS_CVBS     0x02 /*  */
/* end 'register macrocell V3' */
/* register macrocell V5 */
/*#define DENC_CFG5_DIS_CVBS1    0x40 already defined */
#define DENC_CFG5_DIS_YS_V5    0x20 /*  */
#define DENC_CFG5_DIS_C_V5     0x10 /*  */
#define DENC_CFG5_DIS_R        0x08 /*  */
#define DENC_CFG5_DIS_G        0x04 /*  */
#define DENC_CFG5_DIS_B        0x02 /*  */
/* end 'register macrocell V5' */
/* register macrocell V7/V8/V9/10/V11 */
#define DENC_CFG5_SEL_INC      0x80 /* Choice of Dig Freq Synthe increment   */
/* end 'register macrocell V7/V8/V9/10/V11' */
/* register macrocell V6/V7/V8/V9/10/V11 */
#define DENC_CFG5_DIS_DAC1     0x40 /* DAC 1 input forced to 0               */
#define DENC_CFG5_DIS_DAC2     0x20 /* DAC 2 input forced to 0               */
#define DENC_CFG5_DIS_DAC3     0x10 /* DAC 3 input forced to 0               */
#define DENC_CFG5_DIS_DAC4     0x08 /* DAC 4 input forced to 0               */
#define DENC_CFG5_DIS_DAC5     0x04 /* DAC 5 input forced to 0               */
#define DENC_CFG5_DIS_DAC6     0x02 /* DAC 6 input forced to 0               */
/* end 'register macrocell V6/V7/V8/V9/10/V11' */
#define DENC_CFG5_DAC_INV      0x01 /* Enable DAC input data inversion       */
#define DENC_CFG5_DAC_NONINV   0x00 /* Enable DAC input data non inversion   */


/* DENC_CFG6 - Configuration register 6 ------------------------------------ */
#define DENC_CFG6_RST_SOFT     0x80 /* Denc soft reset                       */
#define DENC_CFG6_MASK_LSKP    0x8F /* mask for line skip configuration      */
#define DENC_CFG6_NORM_MODE    0x00 /* normal mode, no insert/skip capable   */
#define DENC_CFG6_MAN_MODE     0x10 /* same as normal, unless skip specified */
#define DENC_CFG6_AUTO_INS     0x40 /* automatic line insert mode            */
#define DENC_CFG6_AUTO_SKP     0x60 /* automatic line skip mode              */
#define DENC_CFG6_FORBIDDEN    0x70 /* Reserved, don't write this value      */
#define DENC_CFG6_MAX_DYN      0x01 /* Maximum dynamic range 1-254 ( 16-240) */
/* register macrocell V3 */
#define DENC_CFG6_CHGI2C_0     0x02 /* Chip add select; write=0x40,read=0x41 */
#define DENC_CFG6_CHGI2C_1     0x00 /* Chip add select; write=0x42,read=0x43 */
/* end register macrocell V3 */
/* register macrocell V7/V8/V9/10/V11 */
#define DENC_CFG6_TTX_ENA      0x02 /* Teletexte enable bit                  */
#define DENC_CFG6_MASK_CFC     0x0C /* Color frequency control mask          */
#define DENC_CFG6_CFC_OFF      0x00 /* Update of increment for DDFS disabled */
#define DENC_CFG6_CFC_IMM      0x04 /* Update immediately after loading / CFC*/
#define DENC_CFG6_CFC_HSYNC    0x08 /* Update on next active edge of HSYNC   */
#define DENC_CFG6_CFC_COLBUR   0x0C /* Update just before next color burst   */
/* end register macrocell V7/V8/V9/10/V11 */


/* DENC_CFG7 - Configuration register 7 ( SECAM mainly ) ------------------- */
#define DENC_CFG7_SECAM        0x80 /* Select SECAM chroma encoding on top   */
                                    /* of config selected in DENC_CFG0       */
#define DENC_CFG7_PHI12_SEC    0x40 /* sub carrier phase sequence start      */
#define DENC_CFG7_INV_PHI_SEC  0x20 /* invert phases on second field         */
/* register macrocell V6/V7/V8/V9/V10/V11 */
#define DENC_CFG7_SETUP_YUV    0x08 /* Control of pedestal enabled for YUV   */
#define DENC_CFG7_UV_LEV       0x04 /* UV output level control               */
/* end register macrocell V6/V7/V8/V9/V10/V11 */
/* register macrocell V12 */
#define DENC_CFG7_SETUP_AUX    0x08 /* Control of pedestal enabled for AUX   */
/* end register macrocell V12 */

#define DENC_CFG7_ENA_VPS      0x02 /* enable video programming system       */
#define DENC_CFG7_SQ_PIX       0x01 /* enable square pixel mode (PAL/NTSC)   */


/* DENC_CFG7 - Configuration register 8 (only for activating MUX422*/
#define DENC_CFG8_VAL_422_MUX  0x10 /* Enable video output in STi4629 */

/* register macrocell V10/V11/V12 */
#define DENC_CFG9_FLT_YS       0x01 /* Enable software luma coeffs           */
#define DENC_CFG9_PLG_DIV_Y_0  0x02 /* Sum of coefficients                   */
#define DENC_CFG9_PLG_DIV_Y_1  0x04 /* Sum of coefficients                   */
#define DENC_CFG9_MASK_PLG_DIV 0xF9 /* Mask for sum of coefficients          */
/* end register macrocell V10/V11/V12 */
/* register macrocell V10/V11 */
#define DENC_CFG9_444_CVBS     0x08 /* Set 4:4:4 for CVBS                    */
/* end register macrocell V10/V11 */
/* register macrocell V10/V11/V12 */
#define DENC_CFG9_MASK_DELAY   0x0F /* Mask for delay on chroma path         */
#define DENC_CFG9_DELAY_P2_5   0xC0 /* +2.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_P2     0xD0 /* +2 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_P1_5   0xE0 /* +1.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_P1     0xF0 /* +1 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_0_5    0x00 /* +0.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_0      0x10 /* +0 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_M0_5   0x20 /* -0.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_M1     0x30 /* -1 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_M1_5   0x40 /* -1.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_M2     0x50 /* -2 pixel delay on chroma path         */
/* end register macrocell V10/V11/V12 */

/* register macrocell V12 */
/* DENC_CFG10 - Configuration register 10  (8-bit)-------------------------- */
#define DENC_CFG10_AUX_MSK_FLT 0x9F /* Mask for U/V Chroma filter bandwith   */
                                     /* selection on AUX                     */
#define DENC_CFG10_AUX_FLT_11  0x00 /* AUX FLT Low definition NTSC filter    */
#define DENC_CFG10_AUX_FLT_13  0x20 /* AUX FLT Low definition PAL filter     */
#define DENC_CFG10_AUX_FLT_16  0x40 /* AUX FLT High definition NTSC filter   */
#define DENC_CFG10_AUX_FLT_19  0x60 /* AUX FLT High definition PAL filter    */
#define DENC_CFG10_AUX_CO_KIL  0x10 /* Color suppressed on CVBS AUX          */
#define DENC_CFG10_RGB_SAT_EN  0x08 /* RGB outputs saturated to real colors. */
#define DENC_CFG10_SECAM_IN    0x04 /* Secam input video select.             */

/* DENC_CFG11 - Configuration register 11  (8-bit)-------------------------- */
#define DENC_CFG11_AUX_MASK_DEL 0x0F /* Mask for delay on aux chroma path    */
#define DENC_CFG11_AUX_DEL_P2_5 0xC0 /* +2.5 pixel delay on aux chroma path  */
#define DENC_CFG11_AUX_DEL_P2   0xD0 /* +2 pixel delay on aux chroma path    */
#define DENC_CFG11_AUX_DEL_P1_5 0xE0 /* +1.5 pixel delay on aux chroma path  */
#define DENC_CFG11_AUX_DEL_P1   0xF0 /* +1 pixel delay on aux chroma path    */
#define DENC_CFG11_AUX_DEL_0_5  0x00 /* +0.5 pixel delay on aux chroma path  */
#define DENC_CFG11_AUX_DEL_0    0x10 /* +0 pixel delay on aux chroma path    */
#define DENC_CFG11_AUX_DEL_M0_5 0x20 /* -0.5 pixel delay on aux chroma path  */
#define DENC_CFG11_AUX_DEL_M1   0x30 /* -1 pixel delay on aux chroma path    */
#define DENC_CFG11_AUX_DEL_M1_5 0x40 /* -1.5 pixel delay on aux chroma path  */
#define DENC_CFG11_AUX_DEL_M2   0x50 /* -2 pixel delay on aux chroma path    */
#define DENC_CFG11_MAIN_IF_DEL  0x04 /* delay on luma vs chroma in CVBS_main */

/* DENC_CFG12 - Configuration register 12  (8-bit)-------------------------- */
#define DENC_CFG12_AUX_ENTRAP   0x80 /* Enable trap filter in CVBS_aux       */
#define DENC_CFG12_AUX_DEL_EN   0x08 /* Enable luma to chroma delay on aux   */
#define DENC_CFG12_MAIN_ENNOTCH 0x04 /* Notch filtering on main luma input   */
#define DENC_CFG12_AUX_MAX_DYN  0x02 /* max dynamic magnitude allowed on aux */
/* end 'register macrocell V12' */
#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107) || defined(ST_5162)
#define DENC_CFG13_CVBS_MAIN    0x0     /* enable CVBS SD output on dac3  dac 4,5 and 6 not defined*/
#else
#define DENC_CFG13_CVBS_MAIN    0x7     /* enable CVBS SD output on dac6 */
#endif
#define DENC_CFG13_YCCVBS_MAIN  0x7     /* enable CVBS SD output on dac3 */
/* STi7015/20 specific DENC registers, out of DENC cell registers */
/* As addressing is 32bits, register offsets are given divided by 4 */
#define DENC_CFG_7015          0x70 /* 0x1C0>>2 DENC configuration           */
#define DENC_TTX_7015          0x71 /* 0x1C4>>2 Start address of Txt file    */

#define DENC_CFG_7015_ON       0x01  /* DENC On. To be set to use DENC       */
#define DENC_CFG_7015_CKR      0x02  /* Insertion of CLKRUN for Teletext     */
#define DENC_TTX_7015_MASK     0x3FFFF00 /* Mask of address in the 32bits    */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* ----------------------------- End of file ------------------------------ */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __DENC_REG_H */

/* End of denc_reg.h */


