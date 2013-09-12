/*******************************************************************************
File Name   : denc_reg.h

Description : DENC V3/V5/V6/V7/V8/V9/V10/V11 registers

COPYRIGHT (C) STMicroelectronics 2000

Date               Modification                                     Name
----               ------------                                     ----
22 Feb 2001       Created (imported from STDENC)                    HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __VTG_DENC_REG_H
#define __VTG_DENC_REG_H

/* Includes ----------------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

/*
  register map for DENC V3/ V5/ V6/ V7/ V8/ V9/ V10/ V11 version
*/

/* DEN - Digital encoder registers - standard 8 bit calls     --------_----- */
#define DENC_CFG0   0x00  /* Configuration register 0                [7:0]   */
#define DENC_CFG1   0x01  /* Configuration register 1                [7:0]   */
#define DENC_CFG4   0x04  /* Configuration register 4                [7:0]   */
#define DENC_CFG6   0x06  /* Configuration register 6                [7:0]   */

/* DENC_CFG0 - Configuration register 0  (8-bit)---------------------------- */
#define DENC_CFG0_MASK_STD     0x3F /* Mask for standard selected              */
#define DENC_CFG0_PAL_BDGHI    0x00 /* PAL B, D, G, H or I standard selected   */
#define DENC_CFG0_PAL_N        0x40 /* PAL N standard selected                 */
#define DENC_CFG0_NTSC_M       0x80 /* NTSC M  standard selected               */
#define DENC_CFG0_PAL_M        0xC0 /* PAL M standard selected                 */
#define DENC_CFG0_MASK_SYNC    0xC7 /* Mask for synchro configuration          */
#define DENC_CFG0_ODDE_SLV     0x00 /* ODDEVEN based slave mode (frame lock)   */
#define DENC_CFG0_FRM_SLV      0x08 /* Frame only based slave mode(frame lock) */
#define DENC_CFG0_ODHS_SLV     0x10 /* ODDEVEN + HSYNC based slave mode(line l)*/
#define DENC_CFG0_FRHS_SLV     0x18 /* Frame + HSYNC based slave mode(line l)  */
#define DENC_CFG0_VSYNC_SLV    0x20 /* VSYNC only based slave mode(frame l   ) */
#define DENC_CFG0_VSHS_SLV     0x28 /* VSYNC + HSYNC based slave mode(line l  )*/
#define DENC_CFG0_MASTER       0x30 /* Master mode selected                    */
#define DENC_CFG0_COL_BAR      0x38 /* Test color bar pattern enabled          */
#define DENC_CFG0_HSYNC_POL    0x04 /* HSYNC positive pulse                    */
#define DENC_CFG0_ODD_POL      0x02 /* Synchronisation polarity selection      */
#define DENC_CFG0_FREE_RUN     0x01 /* Freerun On                              */

/* DENC_CFG1 - Configuration register 1  (8-bit)----------------------------   */
#define DENC_CFG1_VBI_SEL      0x80 /* Full VBI selected                       */
#define DENC_CFG1_MASK_FLT     0x9F /* Mask for U/V Chroma filter bandwith     */
                                    /* selection                               */
#define DENC_CFG1_MASK_SYNC_OK 0xEF /* mask for sync in case of frame loss     */
#define DENC_CFG1_FLT_11       0x00 /* FLT Low definition NTSC filter          */
#define DENC_CFG1_FLT_13       0x20 /* FLT Low definition PAL filter           */
#define DENC_CFG1_FLT_16       0x40 /* FLT High definition NTSC filter         */
#define DENC_CFG1_FLT_19       0x60 /* FLT High definition PAL filter          */
#define DENC_CFG1_SYNC_OK      0x10 /* Synchronisation avaibility              */
#define DENC_CFG1_COL_KILL     0x08 /* Color suppressed on CVBS                */
#define DENC_CFG1_SETUP        0x04 /* Pedestal setup (7.5 IRE)                */
#define DENC_CFG1_MASK_CC      0xFC /* Mask for Closed caption encoding mode   */
#define DENC_CFG1_CC_DIS       0x00 /* Closed caption data encoding disabled   */
#define DENC_CFG1_CC_ENA_F1    0x01 /* Closed caption enabled in field 1       */
#define DENC_CFG1_CC_ENA_F2    0x02 /* Closed caption enabled in field 2       */
#define DENC_CFG1_CC_ENA_BOTH  0x03 /* Closed caption enabled in both fields   */
#define DENC_CFG1_DAC_INV      0x80 /* Enable DAC input data inversion         */

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
    /* end register macrocell V3 */
    /* register macrocell V7/V8/V9/10/V11 */
#define DENC_CFG6_TTX_ENA      0x02 /* Teletexte enable bit                  */
#define DENC_CFG6_MASK_CFC     0x0C /* Color frequency control mask          */
#define DENC_CFG6_CFC_OFF      0x00 /* Update of increment for DDFS disabled */
#define DENC_CFG6_CFC_IMM      0x04 /* Update immediately after loading / CFC*/
#define DENC_CFG6_CFC_HSYNC    0x08 /* Update on next active edge of HSYNC   */
#define DENC_CFG6_CFC_COLBUR   0x0C /* Update just before next color burst   */
    /* end register macrocell V7/V8/V9/10/V11 */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __VTG_DENC_REG_H */

/* End of denc_reg.h */


