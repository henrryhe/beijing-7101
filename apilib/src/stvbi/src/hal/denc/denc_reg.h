/*******************************************************************************
File Name   : denc_reg.h

Description : DENC V3/V5/V6/V7/V8/V9/V10/V11 registers

COPYRIGHT (C) STMicroelectronics 2000

Date               Modification                                      Name
----               ------------                                      ----
06 Jul 2000        Created                                           JG
...
11 Dec 2000        Use register device access instead                HSdLM
                   of register memory access
22 Feb 2001        Let only used defines, rename some regs           HSdLM
04 Jul 2001        Remove inclusion of stsys.h                       HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VBI_DENC_REG_H
#define __VBI_DENC_REG_H

/* Includes ----------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

/*
  register map for DENC V3/ V5/ V6/ V7/ V8/ V9/ V10/ V11 version
*/

/* DEN - Digital encoder registers - standard 8 bit calls     --------_----- */
#define DENC_CFG1   0x01  /* Configuration register 1                [7:0]   */
#define DENC_CFG3   0x03  /* Configuration register 3                [7:0]   */
#define DENC_CFG4   0x04  /* Configuration register 4                [7:0]   */
#define DENC_CFG6   0x06  /* Configuration register 6                [7:0]   */
/* not for V3/V5 */
#define DENC_CFG7   0x07  /* Configuration register 7                [7:0]   */
#define DENC_CFG8   0x08  /* Configuration register 8                [7:0]   */
/* end 'not for V3/V5' */
#define DENC_STA    0x09  /* Status register                         [7:0]   */

/* not for V3/V5 */
#define DENC_WSS1   0x0F  /* Wide Screen Signalling                  [15:8]  */
#define DENC_WSS0   0x10  /* Wide Screen Signalling                  [7:0]   */
#define DENC_VPS0   0x19  /* Video Programming Sys s[1-0] cni[3:0] ->[7:0]   */
#define DENC_VPS1   0x1A  /* Video Programming Sys np[7:6] d[4:0] m3->[7:0]  */
#define DENC_VPS2   0x1B  /* Video Programming Sys m2[2:0] h[4:0]   ->[7:0]  */
#define DENC_VPS3   0x1C  /* Video Programming Sys min[4:0] c[3:2]  ->[7:0]  */
#define DENC_VPS4   0x1D  /* Video Programming Sys c[1:0] np[5:0]   ->[7:0]  */
#define DENC_VPS5   0x1E  /* Video Programming Sys pt[7:0]          ->[7:0]  */
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
    /* register macrocell V7/V8/V9/V10/V11*/
#define DENC_TTXL1  0x22  /* Teletext Configuration Line 1                   */
#define DENC_TTXL2  0x23  /* Teletext Configuration Line 2                   */
#define DENC_TTXL3  0x24  /* Teletext Configuration Line 3                   */
#define DENC_TTXL4  0x25  /* Teletext Configuration Line 4                   */
#define DENC_TTXL5  0x26  /* Teletext Configuration Line 5                   */
    /* end register macrocell V7/V8/V9/V10/V11 */

#define DENC_CC11   0x27  /* Closed Caption first character for field 1      */
#define DENC_CC12   0x28  /* Closed Caption second character for field 1     */
#define DENC_CC21   0x29  /* Closed Caption first character for field 2      */
#define DENC_CC22   0x2A  /* Closed Caption second character for field 2     */
#define DENC_CFL1   0x2B  /* Closed Caption line insertion for field 1 [4:0] */
#define DENC_CFL2   0x2C  /* Closed Caption line insertion for field 2       */

    /* register macrocell V7/V8/V9/V10/V11 */
#define DENC_TTXCFG 0x40  /* Teletext Configuration                          */
#define DENC_DAC2   0x41  /* DAC2 Gain, Mask of TTX, En/Disab reg 0x42-0x44  */
    /* end register macrocell V7/V8/V9/V10/V11 */

/* DEN - Digital encoder registers - specific 16 bit calls    -------------- */
#define DENC_WSS16   0x0F  /* Wide Screen Signalling                  [15:0] */
#define DENC_CC116   0x27  /* Closed Caption first character for field 1/2   */
#define DENC_CC216   0x29  /* Closed Caption first character for field 1/2   */
#define DENC_CFL16   0x2B  /* Closed Caption line insertion for field 1/2    */

/* DEN - Digital encoder registers - specific 24 bit calls    -------------- */
#define DENC_CGMS24  0x1F  /* CGMS data byte 0 [1:4]  -> [3:0]               */

/* DEN - Digital encoder registers - specific 32 bit calls    -------------- */
#define DENC_TTX32  0x22    /* Teletext Block Definition byte 1  [31:0]      */



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
    /* register macrocell V7/V8/V9/10/V11 */
#define DENC_CFG3_CK_IN_PHASE  0x10 /*choice of active edge  of 'denc_ref_ck'*/
    /* end register macrocell V7/V8/V9/10/V11 */
    /* register macrocell V10/V11 */
#define DENC_CFG3_DELAY_ENABLE 0x08 /* enable of chroma to luma delay        */
    /* end register macrocell V10/V11 */


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
#define DENC_CFG4_MASK_TXD     0x07 /* Mask for teletext data latency        */
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
#define DENC_CFG6_FP_TTXT      0x04 /* Full page teletext                    */
#define DENC_CFG6_TTX_ALL      0x08 /* start line 7/320 or 6/318             */
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
#define DENC_CFG7_SETUP_YUV    0x08 /* Control of pedestal enabled           */
#define DENC_CFG7_UV_LEV       0x04 /* UV output level control               */
#define DENC_CFG7_ENA_VPS      0x02 /* enable video programming system       */
#define DENC_CFG7_SQ_PIX       0x01 /* enable square pixel mode (PAL/NTSC)   */


/* DENC_CFG8 - Configuration register 8 ( 4 MSB only used ) ---------------- */
  /* specific configuration of outputs  dac1  dac2  dac3  dac4  dac5  dac6   */
#define DENC_CFG8_CONF_OUT_0   0x00 /*   Y     C    CVBS   C     Y    CVBS   */
#define DENC_CFG8_CONF_OUT_1   0x10 /*   Y     C    CVBS   V     Y     U     */
#define DENC_CFG8_CONF_OUT_2   0x20 /*   Y     C    CVBS   R     G     B     */
#define DENC_CFG8_MASK_CONF    0xCF /* mask for configuration of outputs     */
    /* register macrocell V6 */
#define DENC_CFG8_FP_TTXT      0x80 /* Full page teletext                    */
#define DENC_CFG8_FP_TTXT_ALL  0x40 /* +Teletext also on lines 6,318,319     */
    /* end register macrocell V6 */
    /* register macrocell V7/V8/V9/10/V11 */
#define DENC_CFG8_PH_RST_1     0x80 /* sub-carrier phase reset mode          */
#define DENC_CFG8_PH_RST_0     0x40 /* sub-carrier phase reset mode          */
#define DENC_CFG8_MASK_PH_RST  0x3F /* mask for sub-carrier phase reset mode */
#define DENC_CFG8_TTX_NOTMV    0x04 /* ancillary data priority on a VBI line */
#define DENC_CFG8_BLK_ALL      0x08 /* blanking of all video lines           */
    /* end register macrocell V7/V8/V9/10/V11 */

#define DENC_STA_BUF1_FREE     0x10 /* cc reg access condition for field 1   */
#define DENC_STA_BUF2_FREE     0x20 /* cc reg access condition for field 2   */

/* not for V3/V5 */
#define DENC_WSS1_MASK         0x1F /* Wide Screen Signalling 1 mask         */
#define DENC_WSS0_MASK         0x1C /* Wide Screen Signalling 0 mask         */
#define DENC_VPS0_MASK         0xCF /* Video Programming System 0 mask       */
/* end 'not for V3/V5' */



/* register macrocell V7/V8/V9/V10/V11*/
#define DENC_TTXL1_MASK_CONF  0xF0  /* mask for config bits                  */
#define DENC_TTXL1_MASK_DEL   0x70  /* mask for teletext data latency        */
#define DENC_TTXL1_FULL_PAGE  0x80  /* full page teletext mode enable        */
#define DENC_TTXL3_MASK_F1    0xFC  /* mask for field1 lines 18 -> 23        */

#define DENC_TTXCFG_MASK_STD  0x60  /* teletext standard selection           */
#define DENC_TTXCFG_STD_A     0x00  /* teletext standard A                   */
#define DENC_TTXCFG_STD_B     0x20  /* teletext standard B                   */
#define DENC_TTXCFG_STD_C     0x40  /* teletext standard C                   */
#define DENC_TTXCFG_STD_D     0x60  /* teletext standard D                   */
#define DENC_TTXCFG_100IRE    0x80  /* amplitude of teletext waveform        */
/* end of 'register macrocell V7/V8/V9/V10/V11' */


#define DENC_DAC2_MASK_DAC2    0x0F /* mask for DAC2 configuration           */
#define DENC_DAC2_TTX_MASK_OFF 0x08 /* Teletext mask disable                 */
#define DENC_DAC2_MASK_BRIGHT4 0xFD /* mask for enable brightness 4:4:4      */
#define DENC_DAC2_MASK_BRIGHT2 0xFE /* mask for enable brightness 4:2:2      */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __VBI_DENC_REG_H */

/* End of denc_reg.h */


