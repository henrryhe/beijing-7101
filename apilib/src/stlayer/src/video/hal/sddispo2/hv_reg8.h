/*******************************************************************************

File name   : hv_reg8.h

Description : Video display module (STLAYER video) HAL registers for sddispo2 header file

COPYRIGHT (C) STMicroelectronics 2006

Date               Modification                                     Name
----               ------------                                     ----
04 Nov 2002        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __LAYER_DISPLAY_REGISTERS_SDDISPO2_H
#define __LAYER_DISPLAY_REGISTERS_SDDISPO2_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Video plug Register offsets */

/*- DISP control register ----------------------------------------------------*/
#define DISP_CTL                0x000   /* Display Control register */
#define DISP_CTL_MASK      0xE0E00000   /* Mask to access available conf. bits. */
/* output format 4:2:2 or 4:4:4 (not implemented) */
#define DISP_CTL_422OUT_MASK    0x1
#define DISP_CTL_422OUT_EMP     20
/* Enable/disable luma HF */
#define DISP_CTL_ENAYHF_MASK    0x1
#define DISP_CTL_ENAYHF_EMP     21
/* Enable/disable chroma HF */
#define DISP_CTL_ENACHF_MASK    0x1
#define DISP_CTL_ENACHF_EMP     22
/* Enable/disable luma and chroma HF together */
#define DISP_CTL_ENAYCHF_MASK   0x2
#define DISP_CTL_ENAYCHF_EMP    21
/* Big (not little) bitmap */
#define DISP_CTL_BIGNOTLITTLE_MASK 0x1
#define DISP_CTL_BIGNOTLITTLE_EMP  23
/* Enable/disable loading of V filters */
#define DISP_CTL_ENA_VFILTER_UPDATE_MASK 0x1
#define DISP_CTL_ENA_VFILTER_UPDATE_EMP  29
/* Enable/disable loading of H filters */
#define DISP_CTL_ENA_HFILTER_UPDATE_MASK 0x1
#define DISP_CTL_ENA_HFILTER_UPDATE_EMP  30
/* Enable/disable DISP block */
#define DISP_CTL_ENA_DISP_MASK  0x1
#define DISP_CTL_ENA_DISP_EMP   31
/*----------------------------------------------------------------------------*/

#define DISP_LUMA_HSRC          0x004	/* Display Luma HSRC */
#define DISP_LUMA_VSRC          0x008	/* Display Luma VSRC */
#define DISP_CHR_HSRC           0x00c	/* Display Chroma HSRC */
#define DISP_CHR_VSRC           0x010	/* Display Chroma VSRC */
#define DISP_INITIAL_PHASE_MASK 0x1FFF  /* Display Initial Phase Mask */
#define DISP_TARGET_SIZE        0x014	/* Display Target Pixmap Size */
#define DISP_NLZZD_Y            0x018	/* Display Luma   Non Linear Zoom Zone Definition */
#define DISP_NLZZD_C            0x01c	/* Display Chroma Non Linear Zoom Zone Definition */
#define DISP_PDELTA             0x020	/* Display Non Linear Zoom - Increment Step Definition */
#define DISP_STATUS             0x07c	/* Display Status */
#define DISP_MA_CTL             0x080	/* Display Memory Access Control */
/* Chroma access mode */
#define DISP_MA_CTL_MB_FIELDC_MASK      0x1
#define DISP_MA_CTL_MB_FIELDC_EMP       26
/* Min Req Space */
#define DISP_MA_CTL_MINREQSPACE_MASK    0x3FF
#define DISP_MA_CTL_MINREQSPACE_EMP     16
/* PxLdLine */
#define DISP_MA_CTL_PXLDLINE_MASK       0x1F
#define DISP_MA_CTL_PXLDLINE_EMP        11
/* CfLdLine */
#define DISP_MA_CTL_CFLDLINE_MASK       0x1F
#define DISP_MA_CTL_CFLDLINE_EMP        6
/* Luma access mode */
#define DISP_MA_CTL_MB_FIELDY_MASK      0x1
#define DISP_MA_CTL_MB_FIELDY_EMP       5
/* InFormat */
#define DISP_MA_CTL_IN_FORMAT_MASK      0x1F
#define DISP_MA_CTL_IN_FORMAT_EMP       0
/******************************************************************************/
/* The 3 registers below are own by STVID. They should not be accessed within STLAYER ! */
/*#define DISP_LUMA_BA            0x084*/   /* Display Luma   Source Pixmap Memory Location */
/*#define DISP_CHR_BA             0x088*/   /* Display Chroma Source Pixmap Memory Location */
/*#define DISP_PMP                0x08c*/   /* Display Pixmap Memory Pitch */
/******************************************************************************/
#define DISP_LUMA_XY            0x090	/* Display Luma   First Pixel Source Position */
#define DISP_CHR_XY             0x094	/* Display Chroma First Pixel Source Position */
#define DISP_LUMA_SIZE          0x098	/* Display Luma   Source Window Memory Size */
#define DISP_CHR_SIZE           0x09c	/* Display Chroma Source Window Memory Size */
#define DISP_HFP                0x0a0	/* Display Horizontal Filters Pointer */
#define DISP_VFP                0x0a4	/* Display Vertical Filters Pointer */
#define DISP_PKZ                0x0fc	/* Display Maximum Packet Size */
#define DISP_LHF_COEF0          0x100	/* Display Luma   Hor. filter Coefficient 0  */
#define DISP_LHF_COEF34         0x188	/* Display Luma   Hor. filter Coefficient 34  */
#define DISP_LVF_COEF21         0x1E0	/* Display Luma   Ver. filter Coefficient 21  */

#define DISP_MAX_XY_MASK        0x7FF   /* Display Max Positions */
#define DISP_MAX_SIZE_MASK      0x7FF   /* Display Max Sizes */
#define DISP_X_EMP              0       /* Position of X bits in registers */
#define DISP_Y_EMP              16      /* Position of X bits in registers */
#define DISP_WIDTH_EMP          0       /* Position of width bits in registers */
#define DISP_HEIGHT_EMP         16      /* Position of height bits in registers */


/******************************************************************************/
/*- Specifics registers access for c8dhl cell embedded in STi7710 chip (low  -*/
/*- cost HD version of sddispo2 IP).                                         -*/
/******************************************************************************/

/* WARNING : To be defined : offset of register LMU base address !!!         */
#define DISP_LMU_CTRL           0x2000  /* LMU Control Register */
#define DISP_LMU_CTRL_FML_C_EMP 0       /* film compaison line number bits  */
#define DISP_LMU_CTRL_FML_Y_EMP 16      /* film compaison line number bits  */
#define DISP_LMU_CTRL_FMC_C_EMP 8       /* Film mode control bits           */
#define DISP_LMU_CTRL_FMC_Y_EMP 24      /* Film mode control bits           */
#define DISP_LMU_CTRL_CK_C_EMP  10      /* Clock control bits               */
#define DISP_LMU_CTRL_CK_Y_EMP  26      /* Clock control bits               */
#define DISP_LMU_CTRL_EN_C_EMP  12      /* luma ENABLE bit                  */
#define DISP_LMU_CTRL_EN_Y_EMP  28      /* luma ENABLE bit                  */
#define DISP_LMU_CTRL_DTI_C_EMP 13      /* temporal interpolation DIS. bit  */
#define DISP_LMU_CTRL_DTI_Y_EMP 29      /* temporal interpolation DIS. bit  */
#define DISP_LMU_CTRL_MC_C_EMP  14      /* motion comparison mode bit       */
#define DISP_LMU_CTRL_MC_Y_EMP  30      /* motion comparison mode bit       */

#define  DISP_LMU_LMP           0x2004  /* LMU Luma Memory Pointer  */
#ifdef  STI7710_CUT2x
#define  DISP_LMU_LMP_MASK      0x3FFFFu
#else
#define  DISP_LMU_LMP_MASK      0xFFFFFFu
#endif
#define  DISP_LMU_LMP_EMP       8
#define  DISP_LMU_CMP           0x2008  /* LMU Chroma Memory Pointer */
#ifdef  STI7710_CUT2x
#define  DISP_LMU_CMP_MASK      0x3FFFFu
#else
#define  DISP_LMU_CMP_MASK      0xFFFFFFu
#endif
#define  DISP_LMU_CMP_EMP       8
#define  DISP_LMU_BPPL          0x200C  /* LMU Input Picture Horizontal Size */

#define  DISP_LMU_CFG           0x2010  /* LMU Configuration Register */
#define  DISP_LMU_CFG_MASK      0x7
#define  DISP_LMU_CFG_422_EMP   0
#define  DISP_LMU_CFG_422_MASK  1
#define  DISP_LMU_CFG_TNB_EMP   1
#define  DISP_LMU_CFG_TNB_MASK  1
#define  DISP_LMU_CFG_TOG_EMP   2
#define  DISP_LMU_CFG_TOG_MASK  1

#define  DISP_LMU_VINL          0x2014  /* LMU Input Picture Vertical Size */
#define  DISP_LMU_MRS           0x2018  /* LMU Min STBus Request */

#define  DISP_LMU_CHK           0x201C  /* LMU Max Chunk Size  */
#define  DISP_LMU_CHK_RDY_EMP   0       /* STBus Luma Read     */
#define  DISP_LMU_CHK_WRY_EMP   4       /* STBus Luma Write    */
#define  DISP_LMU_CHK_RDC_EMP   2       /* STBus Chroma Read   */
#define  DISP_LMU_CHK_WRC_EMP   6       /* STBus Chroma Write  */

#define  DISP_LMU_STA           0x2020  /* LMU Status Register  */
#define  DISP_LMU_ITM           0x2024  /* LMU Interrupt Mask Register */
#define  DISP_LMU_ITS           0x2028  /* LMU Interrupt Status Register */
#define  DISP_LMU_AFD           0x202C  /* LMU Accumulated Field Difference */

/******************************************************************************/
/*- Video Gamma compositor registers -----------------------------------------*/
/******************************************************************************/
/* GAM_VIDn_CTL : control */
#define GAM_VIDn_CTL                             0x00
#define GAM_VIDn_CTL_MASK                        0xc63f7007
/* Bit 25 of GAM_VIDn_CTL that selects between 601 and 709 color space */
#define GAM_VIDn_CTL_709NOT601                   0x02000000

/* Bit 0 of GAM_VIDn_CTL: Enable brightness and contrast correction with PSI block */
#define GAM_VIDn_CTL_PSI_BC                      0x01
/* Bit 1 of GAM_VIDn_CTL: Enable tint correction with PSI block */
#define GAM_VIDn_CTL_PSI_TINT                    0x02
/* Bit 2 of GAM_VIDn_CTL: Enable chroma saturation with PSI block */
#define GAM_VIDn_CTL_PSI_SAT                     0x04

/* GAM_VIDn_ALP : global alpha value */
#define GAM_VIDn_ALP                             0x04
#define GAM_VIDn_ALP_MASK                        0x000000ff
#define GAM_VIDn_ALP_FULL_TRANSPARANCY           0x00
#define GAM_VIDn_ALP_DEFAULT                     0x80

/* GAM_VIDn_KEY1 : lower limit of the color range */
#define GAM_VIDn_KEY1                            0x28
#define GAM_VIDn_KEY1_MASK                       0x00ffffff

/* GAM_VIDn_KEY2 : upper limit of the color range */
#define GAM_VIDn_KEY2                            0x2C
#define GAM_VIDn_KEY2_MASK                       0x00ffffff

/* GAM_VIDn_VPO : Viewport Offset */
#define GAM_VIDn_VPO                             0x0C
#define GAM_VIDn_VPO_MASK                        0x07ff0fff

/* GAM_VIDn_VPS : Viewport stop */
#define GAM_VIDn_VPS                             0x10
#define GAM_VIDn_VPS_MASK                        0x07ff0fff

/* GAM_VIDn_MPR matrix programming registers */
#define GAM_VIDn_MPR1                            0x30
#define GAM_VIDn_MPR1_MASK                       0x0fff007f
#define GAM_VIDn_MPR2                            0x34
#define GAM_VIDn_MPR2_MASK                       0x0fff0fff
#define GAM_VIDn_MPR3                            0x38
#define GAM_VIDn_MPR3_MASK                       0x0fff0fff
#define GAM_VIDn_MPR4                            0x3C
#define GAM_VIDn_MPR4_MASK                       0x0fff0fff

/* PSI registers */
/* Brightness and contrast */
#define GAM_VIDn_BC                              0x70
#define GAM_VIDn_BC_RST                          0x00008000
#define GAM_VIDn_B_MASK                          0x000000FF
#define GAM_VIDn_C_MASK                          0x0000FF00
/* Tint (hue/chroma phase) */
#define GAM_VIDn_TINT                            0x74
#define GAM_VIDn_TINT_RST                        0x00000000
#define GAM_VIDn_TINT_MASK                       0x000000FC
/* Chroma saturation */
#define GAM_VIDn_SAT                             0x78
#define GAM_VIDn_SAT_RST                         0x00000080
#define GAM_VIDn_SAT_MASK                        0x000000FC

/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __LAYER_DISPLAY_REGISTERS_SDDISPO2_H */

/* End of hv_reg8.h */
