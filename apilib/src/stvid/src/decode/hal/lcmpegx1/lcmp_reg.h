/*******************************************************************************

File name   : lcmp_reg.h

Description : Video decoder module HAL registers for lcmpegs1 and lcmpegh1
              IP header file

COPYRIGHT (C) STMicroelectronics 2003

Date               Modification                                     Name
----               ------------                                     ----
 May 2003            Created                                      Michel Bruant
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __DECODE_REGISTERS_LCMPEGX1_H
#define __DECODE_REGISTERS_LCMPEGX1_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Register Map Summary (Offsets) */
/*--------------------------------*/

/* Configuration */
#define VID_EXE     0x008
#define CFG_VIDIC   0x010

/* Debug */
#define VID_REQS    0x040
#define VID_REQC    0x044
#define VID_DBG2    0x064
#define VID_VLDS    0x068
#define VID_VMBN    0x06c
#define VID_MBE0    0x070
#define VID_MBE1    0x074
#define VID_MBE2    0x078
#define VID_MBE3    0x07c
#define VID_FRN     0x014   /* Specific lcmpegh1 */

/* Q Matrix Tables */
#define VID_QMWIp   0x100
#define VID_QMWNIp  0x180

/* Decoding Task */
#define VID_TIS     0x300
#define VID_PPR     0x304
#define VID_SRS     0x30c

/* Interrupts */
#define VID_ITM     0x3f0
#define VID_ITS     0x3f4
#define VID_STA     0x3f8

/* Bit Buffers / Frame Buffers */
#define VID_DFH     0x400
#define VID_DFS     0x404
#define VID_DFW     0x408
#define VID_BBS     0x40c
#define VID_BBE     0x414
#define VID_VLDRL   0x448
#define VID_VLDPTR  0x45c
#define VID_MBNM    0x480
#define VID_BCHP    0x488
#define VID_BFP     0x48c
#define VID_FCHP    0x490
#define VID_FFP     0x494
#define VID_RCHP    0x498
#define VID_RFP     0x49c

#define VID_MBNS    0x484   /* Specific lcmpegh1 */
#define VID_RCM     0x4ac   /* Specific lcmpegh1 */
#define VID_SRCHP   0x4a0   /* Specific lcmpegh1 */
#define VID_SRFP    0x4a4   /* Specific lcmpegh1 */


/* Registers description : bits positions */
/*----------------------------------------*/

/* CFG_VIDIC : Video Decoder Interconnect Config */
#define CFG_VIDIC_LP    0  /* 3 bits */
#define CFG_VIDIC_PRDL  3  /* 2 bits */
#define CFG_VIDIC_PRDC  5  /* 2 bits */

/* VID_ITM : Interrupt Mask */
#define VID_ITM_DID     0  /* 1 bit  */
#define VID_ITM_DOE     1  /* 1 bit  */
#define VID_ITM_DUE     2  /* 1 bit  */
#define VID_ITM_MSE     3  /* 1 bit  */
#define VID_ITM_DSE     4  /* 1 bit  */
#define VID_ITM_VLDRL   5  /* 1 bit  */
#define VID_ITM_R_OPC   6  /* 1 bit  */

/* VID_ITM : Interrupt Status */
#define VID_STA_DID     0  /* 1 bit  */
#define VID_STA_DOE     1  /* 1 bit  */
#define VID_STA_DUE     2  /* 1 bit  */
#define VID_STA_MSE     3  /* 1 bit  */
#define VID_STA_DSE     4  /* 1 bit  */
#define VID_STA_VLDRL   5  /* 1 bit  */
#define VID_STA_R_OPC   6  /* 1 bit  */

/* VID_PPR : Picture Parameters */
#define VID_PPR_FFH     0  /* 4 bits */
#define VID_PPR_BFH     4  /* 4 bits */
#define VID_PPR_FFV     8  /* 4 bits */
#define VID_PPR_BFV     12 /* 4 bits */
#define VID_PPR_PST     16 /* 2 bits */
#define VID_PPR_DCP     18 /* 2 bits */
#define VID_PPR_PCT     20 /* 2 bits */
#define VID_PPR_AZZ     22 /* 1 bit  */
#define VID_PPR_IVF     23 /* 1 bit  */
#define VID_PPR_QST     24 /* 1 bit  */
#define VID_PPR_CMV     25 /* 1 bit  */
#define VID_PPR_FRM     26 /* 1 bit  */
#define VID_PPR_TFF     27 /* 1 bit  */
#define VID_PPR_FFN     28 /* 2 bits */
#define VID_PPR_MP2     30 /* 1 bit  */

/* VID_TIS : Task Instruction */
#define VID_TIS_OVW     0 /* 1 bit  */
#define VID_TIS_SBD     1 /* 1 bit  */
#define VID_TIS_MVC     2 /* 1 bit  */
#define VID_TIS_RMM     3 /* 1 bit  */

/* VID_RCM : Reconstruction mode  */    /* Specific lcmpegh1 */
#define VID_RCM_PS      0 /* 1 bits */
#define VID_RCM_ENS     1 /* 1 bits */
#define VID_RCM_ENM     2 /* 1 bits */
#define VID_RCM_HDEC    3 /* 2 bits */
#define VID_RCM_VDEC    5 /* 2 bits */

#define VID_RCM_DEC0    0
#define VID_RCM_DEC2    1
#define VID_RCM_DEC4    2



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __DECODE_REGISTERS_LCMPEGX1_H*/

/* End of lcmp_reg.h */
