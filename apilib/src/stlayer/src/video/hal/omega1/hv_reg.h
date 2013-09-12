/******************************************************************************

File name   : Hv_reg.h

Description : VIDEO registers

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                 Name
----               ------------                 ----
05/15/00           Created                      Philippe Legeard
08/03/00           Adapted for Omega1           Alexandre Nabais
******************************************************************************/

#ifndef __HAL_LAYER_REG_H
#define __HAL_LAYER_REG_H

/* Includes ---------------------------------------------------------------- */
#include "stddefs.h"
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************/
/*- Auxillary display registers ---------------------------------------------*/
/*****************************************************************************/

/* None for Omega1 */

/*****************************************************************************/
/*- Main display ------------------------------------------------------------*/
/*****************************************************************************/

/* VID - Video registers specific 16 bit calls ----------------------------- */
#define VID_VBG16       0x14  /* Video Bit buffer Start [13:0]                  */
#define VID_VBL16       0x16  /* Video Bit buffer Level [13:0]                  */
#define VID_VBS16       0x18  /* Video Bit buffer Stop [13:0]                   */
#define VID_VBT16       0x1A  /* Video Bit buffer Threshold [13:0]              */
#define VID_ABG16       0x1C  /* Video Bit buffer Start [13:0]                  */
#define VID_PTH16       0x2E  /* Video Panic Threshold [13:0]                   */
#define VID_TRF16       0x56  /* Video temporal reference [11:0]                */
#define VID_DCF16       0x74  /* Video Display Configuration [13:0]             */
/*- interrupt registers ------------------------------------------------*/
#define VID_ITM_16                          0x60            /* Interrupt Mask                                      */
#define VID_ITM_MASK                        0xFFFF
#define VID_ITS_16                          0x62            /* Interrupt Status                                    */
#define VID_ITS_MASK                        0xFFFF
#define VID_STA_16                          0x64            /* Status Register                                     */
#define VID_STA_MASK                        0xFFFF

#define VID_STA_SCH
#define VID_STA_BFF
#define VID_STA_HFE
#define VID_STA_BBF
#define VID_STA_BBE
#define VID_STA_VSB
#define VID_STA_VST
#define VID_STA_PSD
#define VID_STA_PII
#define VID_STA_DEI
#define VID_STA_ERC
#define VID_STA_PNC
#define VID_STA_HFF
#define VID_STA_BMI
#define VID_STA_SER
#define VID_STA_PDE
#define VID_STA_SCH_EMP 0
#define VID_STA_BFF_EMP 1
#define VID_STA_HFE_EMP 2
#define VID_STA_BBF_EMP 3
#define VID_STA_BBE_EMP 4
#define VID_STA_VSB_EMP 5
#define VID_STA_VST_EMP 6
#define VID_STA_PSD_EMP 7
#define VID_STA_PII_EMP 8
#define VID_STA_DEI_EMP 9
#define VID_STA_ERC_EMP 10
#define VID_STA_PNC_EMP 11
#define VID_STA_HFF_EMP 12
#define VID_STA_BMI_EMP 13
#define VID_STA_SER_EMP 14
#define VID_STA_PDE_EMP 15

/*- Backward ----------------------------------------------------------------*/
#define VID_BFP_16      0x12  /* Backward Frame pointer [13:0] luma */
#define VID_BFC_16      0x5E  /* Backward Frame pointer [13:0] chroma */

/*- Current -----------------------------------------------------------------*/
#define VID_DFP_16      0x0C  /* Display Frame pointer [13:0] luma */
#define VID_DFC_16      0x58  /* Display Frame pointer [13:0] chroma */

/*- Forward -----------------------------------------------------------------*/
#define VID_FFP_16      0x10  /* Forward Frame pointer [13:0] luma */
#define VID_FFC_16      0x5C  /* Forward Frame pointer [13:0] chroma */

/*- Reconstructed -----------------------------------------------------------*/
#define VID_RFP_16      0x0E  /* Reconstructed Frame pointer [13:0] luma */
#define VID_RFC_16      0x5A  /* Reconstructed Frame pointer [13:0] chroma */

/*- CCIR 656 / 601 Modes ----------------------------------------------------*/
#define VID_656         0x32  /* Enable 656 Mode [0:0] */
#define VID_656_MASK    0x01

/*- Window size -------------------------------------------------------------*/
#define VID_DFS_16      0x24  /* Video decoded frame size, cyclic [14:0] */
#define VID_DFS_MASK1   0x7F  /* First cycle. */
#define VID_DFS_MASK2   0xFF  /* Second cycle. */
#define VID_DFS_CIF     0x4000
#define VID_DFW         0x25  /* Video decoded frame width, [7:0] */

/*- Pan & Scan --------------------------------------------------------------*/
#define VID_PAN_16      0x2C  /* Video Pan display [10:0] */
#define VID_PAN_MASK    0x7FF
#define VID_SCN_8       0x87  /* Video Scan vector [5:0] rows of macroblocs */
#define VID_SCN_MASK    0x3F
#define VID_VSCAN_8     0x79  /* Video Scan vector [4:0] inside a macrobloc */
#define VID_VSCAN_MASK  0x1F

/*- Horizontal Offset -------------------------------------------------------*/
#define VID_XDO_16      0x70  /* Video X axis Start [9:0] */
#define VID_XDS_16      0x72  /* Video X axis Stop  [9:0] */
#define VID_XD0_MASK     0x3FF
#define VID_XDS_MASK     0x3FF

/*-Vertical Offset ----------------------------------------------------------*/
#define VID_YDO_16      0x6E  /* Video Y axis Start [8:0] */
#define VID_YDO0_8      0x6F  /* Video Y axis Start [7:0] */
#define VID_YDO1_8      0x6E  /* Video Y axis Start [8:8] */
#define VID_YDS_16      0x46  /* Video Display Stop [8:0] */
#define VID_YD00_MASK    0xFF

/* Masks for X/YDO/S are defined in level below as they differ depending on chip ! */

/*---------------------------------------------------------------------------*/

/*- Display configuration ---------------------------------------------------*/
#define VID_DCF_16      0x74  /* Video Display Configuration [13:0] */
#if defined(ST_5510) || defined(ST_5512)
#define VID_DCF_MASK    0x3F2F
#endif
#if defined(ST_5508) || defined(ST_5518)
#define VID_DCF_MASK    0x7F2C
#endif
#if defined(ST_5514)
#define VID_DCF_MASK    0x392C
#endif
#if defined(ST_5516) || defined(ST_5517)
#define VID_DCF_MASK    0x39EC
#endif
/* YC PAD IN on 5514 5516 and 5517 */
#define VID_DCF_YCPAD_MASK  1
#define VID_DCF_YCPAD_EMP   1
/* B2R Polarity on 5514 5516 and 5517 */
#define VID_DCF_B2RPOL_MASK 1
#define VID_DCF_B2RPOL_EMP  2
/* Disable Sample Rate Convertor */
#define VID_DCF_DSR_MASK    1
#define VID_DCF_DSR_EMP     3
/* enable/disable display */
#define VID_DCF_EVD_MASK    1
#define VID_DCF_EVD_EMP     5
/* On the fly */
#define VID_DCF_FLY_MASK    1
#define VID_DCF_FLY_EMP     10
/* Frame not field */
#define VID_DCF_FNF_MASK    1
#define VID_DCF_FNF_EMP     11
/* Blank first line */
#define VID_DCF_BFL_MASK    1
#define VID_DCF_BFL_EMP     12
/* Blank last line */
#define VID_DCF_BLL_MASK    1
#define VID_DCF_BLL_EMP     13
/*---------------------------------------------------------------------------*/

/*- Display configuration (8 bit) ------------------------------------------ */
#define VID_DIS_8           0xD6  /* Video Display Configuration [7:0] */
#define VID_DIS_MASK        0x7F
/* line drop : zoom out x2 -> x4 */
#define VID_DIS_ZOX4_MASK   1
#define VID_DIS_ZOX4_EMP    3
/* CPU Priority */
#define VID_DIS_CPU_MASK    2
#define VID_DIS_CPU_EMP     5
/* display Priority */
#define VID_DIS_PRI_MASK    2
#define VID_DIS_PRI_EMP     3
/* frame for L&C. Before called CIF. */
#define VID_DIS_CIF_MASK    2
#define VID_DIS_CIF_EMP     0
#define VID_DIS_CIF         0x03
/* Luma and chroma polarity */
#define VID_DIS_CFB         0x01
#define VID_DIS_LFB         0x02
#define VID_DIS_ZOOM4       0x04


/*- SRC -------------------------------------------------------------------- */
#define VID_LSO_8       0x6A  /* Video SRC luma sub pixel offset [7:0]       */
#define VID_CSO_8       0x6C  /* Video SRC chroma sub pixel offset [7:0]     */
#define VID_LSR0_8      0x6B  /* Video LUMA and CHROMA SRC control [7:0]     */
#define VID_LSR0_MASK   0xFF  /* Video LUMA and CHROMA SRC control [7:0]     */
#define VID_LSR1_8      0x6D  /* Video LUMA and CHROMA SRC control [8]       */
#define VID_LSR1_MASK   0x01  /* Video LUMA and CHROMA SRC control [8]       */


/*- Mix Weight ------------------------------------------------------------- */
#define VID_MWSV_8      0x9D  /* Mix Weight Still / Video                    */
#define VID_MWSV_MASK   0xFF  /* Mix Weight Still / Video                    */
#define VID_MWV_8       0x9B  /* Mix Weight Video                            */
#define VID_MWV_MASK    0xFF  /* Mix Weight Video                            */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VI_HAL_DIS_REG_H */



