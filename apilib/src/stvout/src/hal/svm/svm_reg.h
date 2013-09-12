/*******************************************************************************
File Name   : svm_reg.h

Description : 7015/20 SVM registers

COPYRIGHT (C) STMicroelectronics 2000

*******************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __SVM_REG_H
#define __SVM_REG_H

/* Includes ----------------------------------------------------------------- */
#include "stsys.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ----------------------------------------------------------- */

/* Exported Constants ------------------------------------------------------- */

#define DSPCFG_TST  0x2000  /* Test configuration */
#define DSPCFG_SVM  0x2004  /* SVM static value */
#define DSPCFG_REF  0x2008  /* REF static value*/
#define DSPCFG_DAC  0x200C  /* Dac configuration */

#define DSPCFG_DAC_BLRGB       0x00010000  /* blank on RGB configuration */

/* Main Output Scan Velocity Modulator */
#define SVM_DELAY_COMP    0x2B30  /* Delay compensation for analog video out */
#define SVM_PIX_MAX       0x2B34  /* Nbr of pixel per line */
#define SVM_LIN_MAX       0x2B38  /* Nbr of lines per field */
#define SVM_SHAPE         0x2B3c  /* Modulation shape control */
#define SVM_GAIN          0x2B40  /* Gain control */
#define SVM_OSD_FACTOR    0x2B44  /* OSD gain factor */
#define SVM_FILTER        0x2B48  /* SVM freq response selection for Video and OSD */
#define SVM_EDGE_CNT      0x2B4c  /* Edge count for current frame */

#define DSPCFG_SVM_MASK        0x00FF  /* Mask for SVM static value */

#define SVM_DELAY_COMP_MASK    0x0f         /* Mask for delay compensation for analog video out */
#define SVM_PIX_MAX_MASK       0x7ff        /* Mask for nbr of pixel per line */
#define SVM_LIN_MAX_MASK       0x7ff        /* Mask for nbr of lines per field */
#define SVM_SHAPE_MASK         0x3          /* Mask for modulation shape control */
#define SVM_SHAPE_MOD_OFF      0x0          /* Modulation shape off */
#define SVM_GAIN_MASK          0x1f         /* Mask for gain control */
#define SVM_OSD_FACTOR_MASK    0x3          /* Mask for OSD gain factor */
#define SVM_OSD_FACTOR_0       0x0          /* Null OSD gain factor */
#define SVM_OSD_FACTOR_0_5     0x1          /* Null OSD gain factor */
#define SVM_OSD_FACTOR_0_75    0x2          /* Null OSD gain factor */
#define SVM_OSD_FACTOR_1       0x3          /* Null OSD gain factor */
#define SVM_FILTER_MASK        0x1          /* Mask for SVM 2D freq response */
#define SVM_FILTER_OSD_2D      0x1          /* SVM 2D freq response OSD */
#define SVM_FILTER_VID_2D      0x2          /* SVM 2D freq response Video */
#define SVM_EDGE_CNT_MASK      0xfffff      /* Mask for Edge count for current frame */

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */
#define stvout_rd32(a)           STSYS_ReadRegDev32LE((void*)(a))
#define stvout_wr32(a,v)         STSYS_WriteRegDev32LE((void*)(a),(v))


/* Exported Functions ------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __SVM_REG_H */

/* End of svm_reg.h */


