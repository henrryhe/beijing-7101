/*******************************************************************************
File Name   : dnc_reg.h

Description : DENC V3/V5->V13 registers

COPYRIGHT (C) STMicroelectronics 2003

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __DNC_REG_H
#define __DNC_REG_H

/* Includes ----------------------------------------------------------------- */

#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

/*
  register map for DENC V3/ V5->V13 version
*/

/* DEN - Digital encoder registers - standard 8 bit calls     --------_----- */
#define DENC_CFG0   0x00  /* Configuration register 0                [7:0]   */
#define DENC_CFG1   0x01  /* Configuration register 1                [7:0]   */
#define DENC_CFG2   0x02  /* Configuration register 2                [7:0]   */
#define DENC_CFG3   0x03  /* Configuration register 3                [7:0]   */
#define DENC_CFG4   0x04  /* Configuration register 4                [7:0]   */
#define DENC_CFG5   0x05  /* Configuration register 5                [7:0]   */
#define DENC_CFG6   0x06  /* Configuration register 6                [7:0]   */
/* not for V3/V5 */
#define DENC_CFG7   0x07  /* Configuration register 7, SECAM         [7:0]   */
#define DENC_CFG8   0x08  /* Configuration register 8                [7:0]   */
/* end 'not for V3/V5' */
/* register macrocell V12 */
#define DENC_CFG10  0x5C  /* Configuration register 10               [7:0]   */
#define DENC_CFG11  0x5D  /* Configuration register 11               [7:0]   */
#define DENC_CFG12  0x5E  /* Configuration register 12               [7:0]   */
#define DENC_CFG13  0x5F  /* Configuration register 13               [7:0]   */
/* end 'register macrocell V12' */
#define DENC_STA    0x09  /* Status register                         [7:0]   */
#define DENC_IDFS2  0x0A  /* Increment digital frequency synthesiser [23:16] */
#define DENC_IDFS1  0x0B  /* Increment digital frequency synthesiser [15:8]  */
#define DENC_IDFS0  0x0C  /* Increment digital frequency synthesiser [7:0]   */
#define DENC_PDFS1  0x0D  /* Phase digital frequency synthesiser     [15:8]  */
#define DENC_PDFS0  0x0E  /* Phase digital frequency synthesiser     [7:0]   */
/* only for V3/V5 */
#define DENC_CH_ID  0x11  /* Denc Chip Identification                        */
#define DENC_REV    0x12  /* Denc Revision                                   */
/* end 'only for V3/V5' */
/* not for V3/V5 */
#define DENC_WSS1   0x0F  /* Wide Screen Signalling                  [15:8]  */
#define DENC_WSS0   0x10  /* Wide Screen Signalling                  [7:0]   */
#define DENC_DAC13  0x11  /* Denc gain for tri-dac 1-3               [7:0]   */

#if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
 || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)|| defined(ST_7111)|| defined (ST_7105)  /* -DencV13- || ST_7710*/
#define DENC_DAC34  0x12  /* Denc gain for tri-dac 3-4               [7:0]   */
#define DENC_DAC45  0x13  /* Denc gain for tri-dac 4-5               [7:0]   */
#define DENC_DAC2   0x6A  /* Denc gain for tri-dac 2                 [7:0]   */
#define DENC_DAC6   0x6B  /* Denc gain for tri-dac 6                 [7:0]   */
#if defined(ST_5188)|| defined(ST_5525)
#define DENC_DAC3   0x12  /* Denc gain for quad-dac 3                [7:0]   */
#define DENC_DAC2   0x6A  /* Denc gain for  quad-dac 2               [7:0]   */
#define DENC_DAC4   0x6B  /* Denc gain for  quad-dac 6               [7:0]   */
#endif
#else   /* end -DencV13- || ST_7710*/
#define DENC_DAC45  0x12  /* Denc gain for tri-dac 4-5               [7:0]   */
#define DENC_DAC6C  0x13  /* Denc gain for tri-dac 6-C               [7:0]   */
#endif
/* end 'not for V3/V5' */
#define DENC_LJMP2  0x15  /* line jump luma [8:1]                  ->[7:0]   */
#define DENC_LJMP1  0x16  /* line jump luma [0] chroma [8:2]       ->[7:0]   */
#define DENC_LJMP0  0x17  /* line jump chroma [1:0]                ->[7:6]   */
/* not for V3/V5 */
#define DENC_CID    0x18  /* Denc macro-cell identification number   [7:0]   */
#define DENC_VPS5   0x19  /* Video Programming Sys s[1-0] cni[3:0] ->[7:0]   */
#define DENC_VPS4   0x1A  /* Video Programming Sys np[7:6] d[4:0] m3->[7:0]  */
#define DENC_VPS3   0x1B  /* Video Programming Sys m2[2:0] h[4:0]   ->[7:0]  */
#define DENC_VPS2   0x1C  /* Video Programming Sys min[4:0] c[3:2]  ->[7:0]  */
#define DENC_VPS1   0x1D  /* Video Programming Sys c[1:0] np[5:0]   ->[7:0]  */
#define DENC_VPS0   0x1E  /* Video Programming Sys pt[7:0]          ->[7:0]  */
/* end 'not for V3/V5' */
#define DENC_CGMS2  0x1F  /* CGMS data byte 0 [1:4]  -> [3:0]                */
#define DENC_CGMS1  0x20  /* CGMS data byte 1 [5:12] -> [7:0]                */
#define DENC_CGMS0  0x21  /* CGMS data byte 2 [13:20]-> [7:0]                */

/* register macrocell V3/V5/V6 */
#define DENC_TTX1   0x22  /* Teletext Block Definition byte 1  [7:0]         */
#define DENC_TTX2   0x23  /* Teletext Block Definition byte 2  [7:0]         */
#define DENC_TTX3   0x24  /* Teletext Block Definition byte 3  [7:0]         */
#define DENC_TTX4   0x25  /* Teletext Block Definition byte 4  [7:0]         */
#define DENC_TTXM   0x26  /* Teletext Block Mapping [7:0]                    */
/* end register macrocell V3/V5/V6 */
/* register macrocell V7->V12 */
#define DENC_TTXL1  0x22  /* Teletext Configuration Line 1                   */
#define DENC_TTXL2  0x23  /* Teletext Configuration Line 2                   */
#define DENC_TTXL3  0x24  /* Teletext Configuration Line 3                   */
#define DENC_TTXL4  0x25  /* Teletext Configuration Line 4                   */
#define DENC_TTXL5  0x26  /* Teletext Configuration Line 5                   */
/* end register macrocell V7->V12 */

#define DENC_CC11   0x27  /* Closed Caption first character for field 1      */
#define DENC_CC12   0x28  /* Closed Caption second character for field 2     */
#define DENC_CC21   0x29  /* Closed Caption first character for field 1      */
#define DENC_CC22   0x2A  /* Closed Caption second character for field 2     */
#define DENC_CFL1   0x2B  /* Closed Caption line insertion for field 1 [4:0] */
#define DENC_CFL2   0x2C  /* Closed Caption line insertion for field 2       */



/* register macrocell V7->V12 */
#define DENC_TTXCFG 0x40  /* Teletext Configuration                          */
#if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109)\
 || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)|| defined (ST_7111)|| defined (ST_7105)
#define DENC_DACC   0x41  /* DACC Gain, Mask of TTX, En/Disab reg 0x42-0x44 -for DencV13- and ST_7710*/
#else
#define DENC_DAC2   0x41  /* DAC2 Gain, Mask of TTX, En/Disab reg 0x42-0x44  */
#endif
#define DENC_BRIGHT 0x45  /* Brightness                                      */
#define DENC_CONTRA 0x46  /* Contrast                                        */
#define DENC_SATURA 0x47  /* Saturation                                      */
/* end register macrocell V7->V12 */

/* register macrocell V10/V11 */
#define DENC_CHRM0  0x48  /* Chroma filter coefficient 0                     */
#define DENC_CHRM1  0x49  /* Chroma filter coefficient 1                     */
#define DENC_CHRM2  0x4A  /* Chroma filter coefficient 2                     */
#define DENC_CHRM3  0x4B  /* Chroma filter coefficient 3                     */
#define DENC_CHRM4  0x4C  /* Chroma filter coefficient 4                     */
#define DENC_CHRM5  0x4D  /* Chroma filter coefficient 5                     */
#define DENC_CHRM6  0x4E  /* Chroma filter coefficient 6                     */
#define DENC_CHRM7  0x4F  /* Chroma filter coefficient 7                     */
#define DENC_CHRM8  0x50  /* Chroma filter coefficient 8                     */
#define DENC_CFG9   0x51  /* Configuration register 9                 7:0]   */
#define DENC_LUMA0  0x52  /* Luma filter coefficient 0                       */
#define DENC_LUMA1  0x53  /* Luma filter coefficient 1                       */
#define DENC_LUMA2  0x54  /* Luma filter coefficient 2                       */
#define DENC_LUMA3  0x55  /* Luma filter coefficient 3                       */
#define DENC_LUMA4  0x56  /* Luma filter coefficient 4                       */
#define DENC_LUMA5  0x57  /* Luma filter coefficient 5                       */
#define DENC_LUMA6  0x58  /* Luma filter coefficient 6                       */
#define DENC_LUMA7  0x59  /* Luma filter coefficient 7                       */
#define DENC_LUMA8  0x5A  /* Luma filter coefficient 8                       */
#define DENC_LUMA9  0x5B  /* Luma filter coefficient 9                       */
/* end register macrocell V10/V11 */
/* register macrocell V12 */
#define DENC_AUX_CHRM0  0x60  /* Aux chroma filter coefficient 0             */
#define DENC_AUX_CHRM1  0x61  /* Aux Chroma filter coefficient 1             */
#define DENC_AUX_CHRM2  0x62  /* Aux Chroma filter coefficient 2             */
#define DENC_AUX_CHRM3  0x63  /* Aux Chroma filter coefficient 3             */
#define DENC_AUX_CHRM4  0x64  /* Aux Chroma filter coefficient 4             */
#define DENC_AUX_CHRM5  0x65  /* Aux Chroma filter coefficient 5             */
#define DENC_AUX_CHRM6  0x66  /* Aux Chroma filter coefficient 6             */
#define DENC_AUX_CHRM7  0x67  /* Aux Chroma filter coefficient 7             */
#define DENC_AUX_CHRM8  0x68  /* Aux Chroma filter coefficient 8             */
/* end register macrocell V12 */
#if defined (ST_5162)
#define DENC_HUE_CONTROL          0x69
#define DENC_CFG3_CLK_FROM_PAD    0x04 /* VIDPIX_2XCLK_from_pad being input     */
#define DENC_CFG8_422_FROM_PAD    0x10 /* YCrCb_422_from_pad being input        */
#define DENC_CFG10_SEC_IN_MAIN    0x00 /* main video input is SECAM encoded     */
#define DENC_CFG10_SEC_IN_AUX     0x04 /* aux video input is SECAM encoded      */
#define DENC_CFG10_RGB_SAT        0x00 /* more saturated colors on RGBoutputs   */
#endif
#define DENC_ISCID  17  /* test for chid */

/* DEN - Digital encoder registers - specific 16 bit calls    -------------- */
#define DENC_PDFS16  0x0D  /* Phase digital frequency synthesiser     [15:0] */
#define DENC_WSS16   0x0F  /* Wide Screen Signalling                  [15:0] */
#define DENC_CC116   0x27  /* Closed Caption first character for field 1/2   */
#define DENC_CC216   0x29  /* Closed Caption first character for field 1/2   */
#define DENC_CFL16   0x2B  /* Closed Caption line insertion for field 1/2    */

/* DEN - Digital encoder registers - specific 24 bit calls    -------------- */
#define DENC_IDFS24  0x0A  /* Increment digital frequency synthesiser [23:0 ]*/
#define DENC_LJMP24  0x15  /* line jump luma [8:1]              ->[7:0]      */
#define DENC_CGMS24  0x1F  /* CGMS data byte 0 [1:4]  -> [3:0]               */

/* DEN - Digital encoder registers - specific 32 bit calls    -------------- */
#define DENC_TTX32  0x22/* Teletext Block Definition byte 1  [31:0]      */

#if defined (ST_7109)||defined (ST_7200)||defined (ST_7111)|| defined (ST_7105)
#define DENC_CHRDEL                   0x6C  /*Configures delay on chroma path.*/
#define DENC_CHRDEL_EN                0x6D  /*Chroma delay enable*/
#define DENC_CHRDEL_DELAY_ENABLED     0x01 /* enable of chroma to luma delay        */
#define DENC_CHRDEL_MASK_DELAY        0xF0 /* Mask for delay on chroma path         */


#define DENC_CHRDEL_DELAY_P2_5        0xC /* +2.5 pixel delay on chroma path       */
#define DENC_CHRDEL_DELAY_P2          0xD /* +2 pixel delay on chroma path         */
#define DENC_CHRDEL_DELAY_P1_5        0xE /* +1.5 pixel delay on chroma path       */
#define DENC_CHRDEL_DELAY_P1          0xF /* +1 pixel delay on chroma path         */
#define DENC_CHRDEL_DELAY_0_5         0x0 /* +0.5 pixel delay on chroma path       */
#define DENC_CHRDEL_DELAY_0           0x1 /* +0 pixel delay on chroma path         */
#define DENC_CHRDEL_DELAY_M0_5        0x2 /* -0.5 pixel delay on chroma path       */
#define DENC_CHRDEL_DELAY_M1          0x3 /* -1 pixel delay on chroma path         */
#define DENC_CHRDEL_DELAY_M1_5        0x4 /* -1.5 pixel delay on chroma path       */
#define DENC_CHRDEL_DELAY_M2          0x5 /* -2 pixel delay on chroma path         */
#endif

#define DENC_CFG9_MASK_DELAY          0x0F /* Mask for delay on chroma path         */
#define DENC_CFG9_DELAY_P2_5          0xC0 /* +2.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_P2            0xD0 /* +2 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_P1_5          0xE0 /* +1.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_P1            0xF0 /* +1 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_0_5           0x00 /* +0.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_0             0x10 /* +0 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_M0_5          0x20 /* -0.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_M1            0x30 /* -1 pixel delay on chroma path         */
#define DENC_CFG9_DELAY_M1_5          0x40 /* -1.5 pixel delay on chroma path       */
#define DENC_CFG9_DELAY_M2            0x50 /* -2 pixel delay on chroma path         */


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
/* register macrocell V3/V5/V6/V7->V11 */
#define DENC_CFG2_SEL_444      0x10 /* Select 444 input for RGB tri-dacs     */
/* end register macrocell V3/V5/V6/V7->V11 */
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
#define DENC_CFG3_MASK_DELAY   0xF1 /* Mask for delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P2     0x04 /* +2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_P1     0x02 /* +1 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_0      0x00 /* +0 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M1     0x0E /* -1 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_DELAY_M2     0x0C /* -2 pixel delay on luma 4:2:2 inputs   */
#define DENC_CFG3_ENA_WSS      0x01 /* wide screen signalling enable         */
/* register macrocell V3/V5 */
#define DENC_CFG3_NOSD         0x10 /*choice of active edge  of 'denc_ref_ck'*/
/* end register macrocell V3/V5 */
/* register macrocell V7->V12 */
#define DENC_CFG3_CK_IN_PHASE  0x10 /*choice of active edge  of 'denc_ref_ck'*/
/* end register macrocell V7->V12 */
/* register macrocell V10->V12 */
#define DENC_CFG3_DELAY_ENABLE 0x08 /* enable of chroma to luma delay        */
/* end register macrocell V10->V12 */


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
/* register macrocell V7->V12*/
#define DENC_CFG4_MASK_DELAY   0xF8 /* Mask for delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_P2     0x02 /* +2 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_P1     0x01 /* +1 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_0      0x00 /* +0 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_M1     0x07 /* -1 pixel delay on luma 4:4:4 inputs   */
#define DENC_CFG4_DELAY_M2     0x06 /* -2 pixel delay on luma 4:4:4 inputs   */
/* end register macrocell V7->V12 */


/* DENC_CFG5 - Configuration register 5  (8-bit) --------------------------- */
/* V3 */
#define DENC_CFG5_MASK_CONF    0x7F /* mask for configuration of outputs     */
#define DENC_CFG5_RGB          0x80 /* outputs selection : R-G-B-CVBS1       */
#define DENC_CFG5_NYC          0x00 /* outputs selection : Y-C-CVBS-CVBS1    */
#define DENC_CFG5_DIS_CVBS1    0x40 /*  */
#define DENC_CFG5_DIS_YS_V3    0x08 /*  */
#define DENC_CFG5_DIS_C_V3     0x04 /*  */
#define DENC_CFG5_DIS_CVBS     0x02 /*  */
/* V5 */
/*#define DENC_CFG5_DIS_CVBS1    0x40 already define  */
#define DENC_CFG5_DIS_YS_V5    0x20 /*  */
#define DENC_CFG5_DIS_C_V5     0x10 /*  */
#define DENC_CFG5_DIS_R        0x08 /*  */
#define DENC_CFG5_DIS_G        0x04 /*  */
#define DENC_CFG5_DIS_B        0x02 /*  */
/* register macrocell V7->V12 */
#define DENC_CFG5_SEL_INC      0x80 /* Choice of Dig Freq Synthe increment   */
/* end register macrocell V6/V7->V12 */
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
/* register macrocell V3 */
#define DENC_CFG6_CHGI2C_0     0x02 /* Chip add select; write=0x40,read=0x41 */
#define DENC_CFG6_CHGI2C_1     0x00 /* Chip add select; write=0x42,read=0x43 */
/* end register macrocell V3 */
/* register macrocell V7->V12 */
#define DENC_CFG6_TTX_ENA      0x02 /* Teletexte enable bit                  */
/* end register macrocell V7->V12 */
/* register macrocell V5/V7->V12 */
#define DENC_CFG6_MASK_CFC     0x0C /* Color frequency control mask              */
#define DENC_CFG6_CFC_00       0x00 /* Color frequency  disable                  */
#define DENC_CFG6_CFC_01       0x04 /* increment of DDFS after serial loading    */
#define DENC_CFG6_CFC_02       0x08 /* increment of DDFS on next active edge     */
#define DENC_CFG6_CFC_03       0x0C /* increment of DDFS before next color burst */
/* end register macrocell V5/V7->V12 */


/* DENC_CFG7 - Configuration register 7 ( SECAM mainly ) ------------------- */
#define DENC_CFG7_SECAM        0x80 /* Select SECAM chroma encoding on top   */
                                    /* of config selected in DENC_CFG0       */
#define DENC_CFG7_PHI12_SEC    0x40 /* sub carrier phase sequence start      */
#define DENC_CFG7_INV_PHI_SEC  0x20 /* invert phases on second field         */
/* register macrocell V6->V11 */
#define DENC_CFG7_SETUP_YUV    0x08 /* Control of pedestal enabled           */
#define DENC_CFG7_UV_LEV       0x04 /* UV output level control               */
/* end register macrocell V6->V11 */
/* register macrocell V12 */
#define DENC_CFG7_SETUP_AUX    0x08 /* Control of pedestal enabled for AUX   */
/* end register macrocell V12 */
#define DENC_CFG7_ENA_VPS      0x02 /* enable video programming system       */
#define DENC_CFG7_SQ_PIX       0x01 /* enable square pixel mode (PAL/NTSC)   */

/* DENC_CFG8 - Configuration register 8 ------------------------------------ */
/* register macrocell V3/V5->V11 */
/* specific configuration of outputs  dac1  dac2  dac3  dac4  dac5  dac6     */
#define DENC_CFG8_CONF_OUT_0   0x00 /*   Y     C    CVBS   C     Y    CVBS   */
#define DENC_CFG8_CONF_OUT_1   0x10 /*   Y     C    CVBS   V     Y     U     */
#define DENC_CFG8_CONF_OUT_2   0x20 /*   Y     C    CVBS   R     G     B     */
#define DENC_CFG8_MASK_CONF    0xCF /* mask for configuration of outputs     */
/* end register macrocell V3/V5->V11 */
/* register macrocell V6 */
#define DENC_CFG8_FP_TTXT      0x80 /* Full page teletext                    */
#define DENC_CFG8_FP_TTXT_ALL  0x40 /* +Teletext also on lines 6,318,319     */
/* end register macrocell V6 */
/* register macrocell V7->V12 */
#define DENC_CFG8_PH_RST_1     0x80 /* sub-carrier phase reset mode          */
#define DENC_CFG8_PH_RST_0     0x40 /* sub-carrier phase reset mode          */
#define DENC_CFG8_MASK_PH_RST  0x3F /* mask for sub-carrier phase reset mode */
#define DENC_CFG8_BLK_ALL      0x08 /* blanking of all video lines           */
/* end register macrocell V7->V12 */

/* -DencV13- */
#if defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_5301)|| defined(ST_7109) \
 || defined(ST_5188)|| defined(ST_5525)|| defined(ST_5107)|| defined(ST_7200)|| defined(ST_5162)||defined (ST_7111)|| defined (ST_7105)
#define DENC_DAC13_MASK_DAC1   0x03 /* mask for DAC1 configuration           */
#define DENC_DAC13_MASK_DAC3   0xFC /* mask for DAC3 configuration           */
#define DENC_DAC34_MASK_DAC3   0x0F /* mask for DAC4 configuration           */
#define DENC_DAC34_MASK_DAC4   0xF0 /* mask for DAC4 configuration           */
#define DENC_DAC45_MASK_DAC4   0x3F /* mask for DAC4 configuration           */
#define DENC_DAC45_MASK_DAC5   0xC0 /* mask for DAC5 configuration           */
#define DENC_DAC6_MASK_DAC6    0xC0 /* mask for DAC6 configuration           */
#define DENC_DACC_MASK_DACC    0x0F /* mask for DACC configuration           */
#define DENC_DAC2_MASK_DAC2    0xC0 /* mask for DAC2 configuration           */
#define DENC_DACC_MASK_TTX_OFF 0xF7 /* mask for Teletext mask                */

/* register macrocell V13 */
#define DENC_DACC_BCS_EN_MAIN  0x01 /* brightness/contrast/saturation: main  */
#define DENC_DACC_BCS_EN_AUX   0x02 /* brightness/contrast/saturation: aux   */
/* end register macrocell V13 */
/* end -DencV13- */
#if defined (ST_5188)||defined (ST_5525)
#define DENC_DAC13_MASK_DAC1   0x03 /* mask for DAC1 configuration           */
#define DENC_DAC13_MASK_DAC3   0xFC /* mask for DAC3 configuration           */
#define DENC_DAC3_MASK_DAC3    0x0F /* mask for DAC4 configuration           */
#define DENC_DAC4_MASK_DAC4    0xC0 /* mask for DAC4 configuration           */
#define DENC_DAC2_MASK_DAC2    0xC0 /* mask for DAC2 configuration           */
#define DENC_DACC_MASK_DACC    0x0F /* mask for DACC configuration           */
#define DENC_DACC_MASK_TTX_OFF 0xF7 /* mask for Teletext mask                */

/* register macrocell V13 */
#define DENC_DACC_BCS_EN_MAIN  0x01 /* brightness/contrast/saturation: main  */
#define DENC_DACC_BCS_EN_AUX   0x02 /* brightness/contrast/saturation: aux   */
#endif
#else
#define DENC_DAC13_MASK_DAC1   0x0F /* mask for DAC1 configuration           */
#define DENC_DAC13_MASK_DAC3   0xF0 /* mask for DAC3 configuration           */
#define DENC_DAC45_MASK_DAC4   0x0F /* mask for DAC4 configuration           */
#define DENC_DAC45_MASK_DAC5   0xF0 /* mask for DAC5 configuration           */
#define DENC_DAC6C_MASK_DAC6   0x0F /* mask for DAC6 configuration           */
#define DENC_DAC6C_MASK_DACC   0xF0 /* mask for DAC2 configuration           */
#define DENC_DAC2_MASK_DAC2    0x0F /* mask for DAC2 configuration           */
#define DENC_DAC2_MASK_TTX_OFF 0xF7 /* mask for Teletext mask                */
/* register macrocell V12 */
#define DENC_DAC2_BCS_EN_MAIN  0x01 /* brightness/contrast/saturation: main  */
#define DENC_DAC2_BCS_EN_AUX   0x02 /* brightness/contrast/saturation: aux   */
/* end register macrocell V12 */
#endif
/* register macrocell V8->V11 */
#define DENC_DAC2_BCS_EN_444   0x02 /* enable brightness 4:4:4               */
#define DENC_DAC2_BCS_EN_422   0x01 /* enable brightness 4:2:2               */
/* end register macrocell V8->V11 */

/* register macrocell V10/V11/V12 */
/* DENC_CHRM0 - Chroma Coef 0 and config -------_--------------------------- */
#define DENC_CHRM0_FLT_S       0x80 /* Chroma Filter Selection               */
#define DENC_CHRM0_DIV_MASK    0x9F /* Mask for chroma Filter Division       */
#define DENC_CHRM0_DIV1        0x40 /* Chroma Filter Division bit 1          */
#define DENC_CHRM0_DIV0        0x20 /* Chroma Filter Division bit 0          */
#define DENC_CHRM0_MASK        0x1F /* Mask for Chroma Filter Coef 0         */

/* DENC_CHRM1-8 - Chroma Coef 1-8 and config ------------------------------- */
#define DENC_CHRM1_MASK        0x3F /* Mask for Chroma Filter Coef 1         */
#define DENC_CHRM2_MASK        0x7F /* Mask for Chroma Filter Coef 2         */
#define DENC_CHRM3_MASK        0x7F /* Mask for Chroma Filter Coef 3         */
#define DENC_CHRM4_MASK        0xFF /* Mask for Chroma Filter Coef 4         */
#define DENC_CHRM5_MASK        0xFF /* Mask for Chroma Filter Coef 5         */
#define DENC_CHRM6_MASK        0xFF /* Mask for Chroma Filter Coef 6         */
#define DENC_CHRM7_MASK        0xFF /* Mask for Chroma Filter Coef 7         */
#define DENC_CHRM8_MASK        0xFF /* Mask for Chroma Filter Coef 8         */

/* DENC_CFG9 - Configuration register 9 ------------------------------------ */
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
#define DENC_CFG9_DEL_0        0x10 /* Delay bit 0                           */
#define DENC_CFG9_DEL_1        0x20 /* Delay bit 1                           */
#define DENC_CFG9_DEL_2        0x40 /* Delay bit 2                           */
#define DENC_CFG9_DEL_3        0x80 /* Delay bit 3                           */
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

/* DENC_CFG13 - Configuration register 13  (8-bit)-------------------------- */
#if defined (ST_5188) || defined (ST_5525)
#define DENC_CFG13_DAC123_OUT0  0x00 /*  CVBS_main                           */
#define DENC_CFG13_DAC123_OUT1  0x10 /* R G B                                */
#define DENC_CFG13_DAC123_OUT2  0x18 /* Pr_main Y_main Pb_main               */
#else
#define DENC_CFG13_DAC123_OUT0  0x00 /* Y_main C_main CVBS_main              */
#define DENC_CFG13_DAC123_OUT1  0x08 /* Y_main C_main CVBS_aux               */
#define DENC_CFG13_DAC123_OUT2  0x10 /* R G B                                */
#endif
#define DENC_CFG13_DAC4_MASK    0xF8 /* DACs 4 configuration                 */
#define DENC_CFG13_DAC4_OUT0    0x01 /* CVBS_main                            */
#define DENC_CFG13_DAC4_OUT1    0x04 /* CVBS_main                            */
#define DENC_CFG13_DAC4_OUT2    0x07 /* CVBS_main                            */

#define DENC_CFG13_RGB_MAX_DYN  0x40 /* <> gains when RGB is being outputted */
#define DENC_CFG13_AUX_NMAI_RGB 0x80 /* either main video or aux video as RGB*/

#define DENC_CFG13_DAC123_MASK  0xC7 /* DACs 1, 2, 3 configuration           */

#define DENC_CFG13_DAC123_OUT3  0x18 /* Pr_main Y_main Pb_main               */
#define DENC_CFG13_DAC123_OUT4  0x20 /* CVBS_main CVBS_aux CVBS_aux          */
#define DENC_CFG13_DAC123_OUT5  0x28 /* CVBS_main CVBS_aux CVBS_main         */
#define DENC_CFG13_DAC123_OUT6  0x30 /* Y_aux C_aux CVBS_main                */
#define DENC_CFG13_DAC123_OUT7  0x38 /* Y_aux C_aux CVBS_aux                 */
#define DENC_CFG13_DAC456_MASK  0xF8 /* DACs 4, 5, 6 configuration           */
#define DENC_CFG13_DAC456_OUT0  0x00 /* Y_aux C_aux CVBS_aux                 */
#define DENC_CFG13_DAC456_OUT1  0x01 /* Y_aux C_aux CVBS_main                */
#define DENC_CFG13_DAC456_OUT2  0x02 /* R G B                                */
#define DENC_CFG13_DAC456_OUT3  0x03 /* Pr_aux Y_aux Pb_aux                  */
#define DENC_CFG13_DAC456_OUT4  0x04 /* CVBS_aux CVBS_main CVBS_main         */
#define DENC_CFG13_DAC456_OUT5  0x05 /* CVBS_aux CVBS_main CVBS_aux          */
#define DENC_CFG13_DAC456_OUT6  0x06 /* Y_main C_main CVBS_aux               */
#define DENC_CFG13_DAC456_OUT7  0x07 /* Y_main C_main CVBS_main              */

/* end register macrocell V12 */


/* DENC_LUMA0-9 - Luma Coef 0-9 and config --------------------------------- */
#define DENC_LUMA0_MASK        0x1F /* Mask for Luma Filter Coef 0           */
#define DENC_LUMA1_MASK        0x3F /* Mask for Luma Filter Coef 1           */
#define DENC_LUMA2_MASK        0x7F /* Mask for Luma Filter Coef 2           */
#define DENC_LUMA3_MASK        0x7F /* Mask for Luma Filter Coef 3           */
#define DENC_LUMA4_MASK        0xFF /* Mask for Luma Filter Coef 4           */
#define DENC_LUMA5_MASK        0xFF /* Mask for Luma Filter Coef 5           */
#define DENC_LUMA6_MASK        0xFF /* Mask for Luma Filter Coef 6           */
#define DENC_LUMA7_MASK        0xFF /* Mask for Luma Filter Coef 7           */
#define DENC_LUMA8_MASK        0xFF /* Mask for Luma Filter Coef 8           */
#define DENC_LUMA9_MASK        0xFF /* Mask for Luma Filter Coef 9           */
/* end register macrocell V10/V11 */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __DNC_REG_H */

/* End of dnc_reg.h */



