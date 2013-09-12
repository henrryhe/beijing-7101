                                        /*******************************************************************************

File name   : hv_mb2r.h

Description : Video decoder module HAL registers for Macroblock to Raster

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
24 dec 2004        Created                                          PLe
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DECODE_REGISTERS_MB2R_H
#define __DECODE_REGISTERS_MB2R_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************/
/* definition of the base address */
/****************************************************/
#define MB2R  0xDB900000

/****************************************************/
/* definition of register :  MB2R_MODE */
/* MB2R_MODE : -[30:0] CRCB_NOT_CBCR  */
/****************************************************/
#define MB2R_MODE                               0x00000000
#define MB2R_MODE_CRCB_NOT_CBCR                 0x00000001

/****************************************************/
/* definition of register :  MB2R_RECONST_IT_MASK */
/* MB2R_RECONST_IT_MASK : -[29:0] END_OF_SRR END_OF_MRR  */
/****************************************************/
#ifdef MB2R_0_X_VERSION
#define MB2R_RECONST_IT_MASK                    0x00000004
#define MB2R_RECONST_IT_MASK_END_OF_SRR         0x00000002
#define MB2R_RECONST_IT_MASK_END_OF_MRR         0x00000001
#endif

#ifdef MB2R_1_X_VERSION
#define MB2R_RECONST_IT_MASK                    0x00000610
#define MB2R_RECONST_IT_MASK_END_OF_SRR_2       0x00000400
#define MB2R_RECONST_IT_MASK_END_OF_MRR_2       0x00000200
#define MB2R_RECONST_IT_MASK_END_OF_RR_2        0x00000100
#define MB2R_RECONST_IT_MASK_END_OF_SRR_1       0x00000004
#define MB2R_RECONST_IT_MASK_END_OF_MRR_1       0x00000002
#define MB2R_RECONST_IT_MASK_END_OF_RR_1        0x00000001
#endif

/****************************************************/
/* definition of register :  MB2R_RECONST_CFG */
/* MB2R_RECONST_CFG : -[29:0] MAIN_RASTER MAIN_MB  */
/****************************************************/
#define MB2R_RECONST_CFG                        0x00000020
#define MB2R_RECONST_CFG_MAIN_RASTER            0x00000002
#define MB2R_RECONST_CFG_MAIN_MB                0x00000001

/****************************************************/
/* definition of register :  MB2R_MAIN_LBA_CFG */
/* MB2R_MAIN_LBA_CFG : LBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_MAIN_LBA_CFG                       0x00000040
#define MB2R_MAIN_LBA_CFG_LBA_MASK              0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_MAIN_CBA_CFG */
/* MB2R_MAIN_CBA_CFG : CBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_MAIN_CBA_CFG                       0x00000044
#define MB2R_MAIN_CBA_CFG_CBA_MASK              0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_MAIN_PARAM_CFG */
/* MB2R_MAIN_PARAM_CFG : -[8:0] FRAME_WIDTH[6:0] -[5:0] PICT_STRUCT[1:0] - FRAME_HEIGHT[6:0]  */
/****************************************************/
#define MB2R_MAIN_PARAM_CFG                     0x00000048
#define MB2R_MAIN_PARAM_CFG_FRAME_WIDTH_MASK    0x007F0000
#define MB2R_MAIN_PARAM_CFG_FRAME_WIDTH_SHIFT   16
#define MB2R_MAIN_PARAM_CFG_PICT_STRUCT_MASK    0x00000300
#define MB2R_MAIN_PARAM_CFG_PICT_STRUCT_SHIFT   8
#define MB2R_MAIN_PARAM_CFG_FRAME_HEIGHT_MASK   0x0000007F
#define MB2R_MAIN_PARAM_CFG_FRAME_HEIGHT_SHIFT  0

/****************************************************/
/* definition of register :  MB2R_MAIN_MLL_CFG */
/* MB2R_MAIN_MLL_CFG : -[10:0] MLL[15:0] -[4:0]  */
/****************************************************/
#define MB2R_MAIN_MLL_CFG                       0x0000004c
#define MB2R_MAIN_MLL_CFG_MLL_MASK              0x001FFFE0

/****************************************************/
/* definition of register :  MB2R_SECOND_LBA_CFG */
/* MB2R_SECOND_LBA_CFG : LBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_SECOND_LBA_CFG                     0x00000060
#define MB2R_SECOND_LBA_CFG_LBA_MASK            0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_SECOND_CBA_CFG */
/* MB2R_SECOND_CBA_CFG : CBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_SECOND_CBA_CFG                     0x00000064
#define MB2R_SECOND_CBA_CFG_CBA_MASK            0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_SECOND_PARAM_CFG */
/* MB2R_SECOND_PARAM_CFG : -[24:0] VDEC[1:0] HDEC[1:0] -[2:0]  */
/****************************************************/
#define MB2R_SECOND_PARAM_CFG                   0x00000068
#define MB2R_SECOND_PARAM_CFG_VDEC_MASK         0x00000060
#define MB2R_SECOND_PARAM_CFG_VDEC_NONE         0x00000000
#define MB2R_SECOND_PARAM_CFG_VDEC_V2           0x00000020
#define MB2R_SECOND_PARAM_CFG_VDEC_V4           0x00000040
#define MB2R_SECOND_PARAM_CFG_HDEC_MASK         0x00000018
#define MB2R_SECOND_PARAM_CFG_HDEC_NONE         0x00000000
#define MB2R_SECOND_PARAM_CFG_HDEC_H2           0x00000008
#define MB2R_SECOND_PARAM_CFG_HDEC_H4           0x00000010

/****************************************************/
/* definition of register :  MB2R_SECOND_MLL_CFG */
/* MB2R_SECOND_MLL_CFG : -[10:0] MLL[15:0] -[4:0]  */
/****************************************************/
#define MB2R_SECOND_MLL_CFG                     0x0000006c
#define MB2R_SECOND_MLL_CFG_MLL_MASK            0x001FFFE0

/****************************************************/
/* definition of register :  MB2R_RECONST_CUR */
/* MB2R_RECONST_CUR : -[29:0] MAIN_RASTER MAIN_MB  */
/****************************************************/
#define MB2R_RECONST_CUR                        0x000000a0
#define MB2R_RECONST_CUR_MAIN_RASTER            0x00000002
#define MB2R_RECONST_CUR_MAIN_MB                0x00000001

/****************************************************/
/* definition of register :  MB2R_MAIN_LBA_CUR */
/* MB2R_MAIN_LBA_CUR : LBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_MAIN_LBA_CUR                       0x000000c0
#define MB2R_MAIN_LBA_CUR_LBA_MASK              0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_MAIN_CBA_CUR */
/* MB2R_MAIN_CBA_CUR : CBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_MAIN_CBA_CUR                       0x000000c4
#define MB2R_MAIN_CBA_CUR_CBA_MASK              0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_MAIN_PARAM_CUR */
/* MB2R_MAIN_PARAM_CUR : -[8:0] FRAME_WIDTH[6:0] -[5:0] PICT_STRUCT[1:0] - FRAME_HEIGHT[6:0]  */
/****************************************************/
#define MB2R_MAIN_PARAM_CUR                     0x000000c8
#define MB2R_MAIN_PARAM_CUR_FRAME_WIDTH_MASK    0x007F0000
#define MB2R_MAIN_PARAM_CUR_PICT_STRUCT_MASK    0x00000300
#define MB2R_MAIN_PARAM_CUR_FRAME_HEIGHT_MASK   0x0000007F

/****************************************************/
/* definition of register :  MB2R_MAIN_MLL_CUR */
/* MB2R_MAIN_MLL_CUR : -[10:0] MLL[15:0] -[4:0]  */
/****************************************************/
#define MB2R_MAIN_MLL_CUR                       0x000000cc
#define MB2R_MAIN_MLL_CUR_MLL_MASK              0x001FFFE0

/****************************************************/
/* definition of register :  MB2R_SECOND_LBA_CUR */
/* MB2R_SECOND_LBA_CUR : LBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_SECOND_LBA_CUR                     0x000000e0
#define MB2R_SECOND_LBA_CUR_LBA_MASK            0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_SECOND_CBA_CUR */
/* MB2R_SECOND_CBA_CUR : CBA[26:0] -[4:0]  */
/****************************************************/
#define MB2R_SECOND_CBA_CUR                     0x000000e4
#define MB2R_SECOND_CBA_CUR_CBA_MASK            0xFFFFFFE0

/****************************************************/
/* definition of register :  MB2R_SECOND_PARAM_CUR */
/* MB2R_SECOND_PARAM_CUR : -[24:0] VDEC[1:0] HDEC[1:0] -[2:0]  */
/****************************************************/
#define MB2R_SECOND_PARAM_CUR                   0x000000e8
#define MB2R_SECOND_PARAM_CUR_VDEC_MASK         0x00000060
#define MB2R_SECOND_PARAM_CUR_HDEC_MASK         0x00000018

/****************************************************/
/* definition of register :  MB2R_SECOND_MLL_CUR */
/* MB2R_SECOND_MLL_CUR : -[10:0] MLL[15:0] -[4:0]  */
/****************************************************/
#define MB2R_SECOND_MLL_CUR                     0x000000ec
#define MB2R_SECOND_MLL_CUR_MLL_MASK            0x001FFFE0

/****************************************************/
/* definition of register :  MB2R_RECONST_CTL */
/* MB2R_RECONST_CTL : -[30:0] START  */
/****************************************************/
#define MB2R_RECONST_CTL                        0x00000100
#define MB2R_RECONST_CTL_START                  0x00000001

/****************************************************/
/* definition of register :  MB2R_RECONST_SET_IT */
/* MB2R_RECONST_SET_IT : -[29:0] END_OF_SRR END_OF_MRR  */
/****************************************************/

#ifdef MB2R_0_X_VERSION
#define MB2R_RECONST_SET_IT                     0x00000110
#define MB2R_RECONST_SET_IT_END_OF_SRR          0x00000002
#define MB2R_RECONST_SET_IT_END_OF_MRR          0x00000001
#endif

#ifdef MB2R_1_X_VERSION
#define MB2R_RECONST_SET_IT                     0x00000630
#define MB2R_RECONST_SET_IT_END_OF_SRR_2        0x00000040
#define MB2R_RECONST_SET_IT_END_OF_MRR_2        0x00000020
#define MB2R_RECONST_SET_IT_END_OF_RR_2         0x00000010
#define MB2R_RECONST_SET_IT_END_OF_SRR_1        0x00000004
#define MB2R_RECONST_SET_IT_END_OF_MRR_1        0x00000002
#define MB2R_RECONST_SET_IT_END_OF_RR_1         0x00000001
#endif


/****************************************************/
/* definition of register :  MB2R_RECONST_CLEAR_IT */
/* MB2R_RECONST_CLEAR_IT : -[29:0] END_OF_SRR END_OF_MRR  */
/****************************************************/
#ifdef MB2R_0_X_VERSION
#define MB2R_RECONST_CLEAR_IT                   0x00000114
#define MB2R_RECONST_CLEAR_IT_END_OF_SRR        0x00000002
#define MB2R_RECONST_CLEAR_IT_END_OF_MRR        0x00000001
#endif

#ifdef MB2R_1_X_VERSION
#define MB2R_RECONST_CLEAR_IT                   0x00000634
#define MB2R_RECONST_CLEAR_IT_END_OF_SRR_2      0x00000040
#define MB2R_RECONST_CLEAR_IT_END_OF_MRR_2      0x00000020
#define MB2R_RECONST_CLEAR_IT_END_OF_RR_2       0x00000010
#define MB2R_RECONST_CLEAR_IT_END_OF_SRR_1      0x00000004
#define MB2R_RECONST_CLEAR_IT_END_OF_MRR_1      0x00000002
#define MB2R_RECONST_CLEAR_IT_END_OF_RR_1       0x00000001
#endif


/****************************************************/
/* definition of register :  MB2R_RECONST_STA */
/* MB2R_RECONST_STA : -[28:0] PR_MB_IDLE END_OF_SRR END_OF_MRR  */
/****************************************************/
#ifdef MB2R_0_X_VERSION
#define MB2R_RECONST_STA                        0x00000180
#define MB2R_RECONST_STA_PR_MB_IDLE             0x00000004
#define MB2R_RECONST_STA_END_OF_SRR             0x00000002
#define MB2R_RECONST_STA_END_OF_MRR             0x00000001
#endif

#ifdef MB2R_1_X_VERSION
#define MB2R_RECONST_STA                        0x00000600
#define MB2R_RECONST_STA_END_OF_SRR_2           0x00000040
#define MB2R_RECONST_STA_END_OF_MRR_2           0x00000020
#define MB2R_RECONST_STA_END_OF_RR_2            0x00000010
#define MB2R_RECONST_STA_END_OF_SRR_1           0x00000004
#define MB2R_RECONST_STA_END_OF_MRR_1           0x00000002
#define MB2R_RECONST_STA_END_OF_RR_1            0x00000001
#endif


/****************************************************/
/* definition of register :  MB2R_RECONST_IT_STA */
/* MB2R_RECONST_IT_STA : -[29:0] END_OF_SRR END_OF_MRR  */
/****************************************************/
#ifdef MB2R_0_X_VERSION
#define MB2R_RECONST_IT_STA                     0x00000190
#define MB2R_RECONST_IT_STA_END_OF_SRR          0x00000002
#define MB2R_RECONST_IT_STA_END_OF_MRR          0x00000001
#endif

#ifdef MB2R_1_X_VERSION
#define MB2R_RECONST_IT_STA                     0x00000620
#define MB2R_RECONST_IT_STA_END_OF_SRR_2        0x00000040
#define MB2R_RECONST_IT_STA_END_OF_MRR_2        0x00000020
#define MB2R_RECONST_IT_STA_END_OF_RR_2         0x00000010
#define MB2R_RECONST_IT_STA_END_OF_SRR_1        0x00000004
#define MB2R_RECONST_IT_STA_END_OF_MRR_1        0x00000002
#define MB2R_RECONST_IT_STA_END_OF_RR_1         0x00000001
#endif


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DECODE_REGISTERS_MB2R_H */

/* End of hv_mb2r.h */
