/*******************************************************************************

File name   : hv_vdpreg.h

Description : Video display module (STLAYER video) HAL registers for displaypipe header file

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
18 Oct 2005        Created                                          DG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __LAYER_DISPLAY_REGISTERS_DISPLAYPIPE_H
#define __LAYER_DISPLAY_REGISTERS_DISPLAYPIPE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Video plug Register offsets */


/*- Video Display Pipe Registers ---------------------------------------------*/


#define VDP_DEI_CTL					            0x000            /* Deinterlacer control register */
#define VDP_DEI_CTL_REG_MASK                    0x77FFFFFF    /* bits 27 and 31 are unused */

/* CVF Type */
#define VDP_DEI_CVF_TYPE_MASK 		            0x7
#define VDP_DEI_CVF_TYPE_EMP 		            28
#define VDP_DEI_CVF_4_2_0 		                0x0
#define VDP_DEI_CVF_4_2_2 		                0x1
#define VDP_DEI_CVF_EVEN_FIELD 	                0x0
#define VDP_DEI_CVF_ODD_FIELD 		            0x2
#define VDP_DEI_CVF_INTERLACED 		            0x0
#define VDP_DEI_CVF_PROGRESSIVE 		        0x4
/* FMD SWAP */
#define VDP_DEI_CTL_FMD_SWAP_MASK               0x01
#define VDP_DEI_CTL_FMD_SWAP_EMP                26
#define VDP_DEI_CTL_FMD_NO_SWAP                 0
#define VDP_DEI_CTL_FMD_SWAP                    1
/* FMD ON */
#define VDP_DEI_CTL_FMD_ON_MASK                 0x01
#define VDP_DEI_CTL_FMD_ON_EMP                  25
#define VDP_DEI_CTL_FMD_OFF                     0
#define VDP_DEI_CTL_FMD_ON                      1
/* KMOV_FACTOR : Defines the motion factor. */
#define VDP_DEI_CTL_KMOV_FACTOR_MASK            0x01
#define VDP_DEI_CTL_KMOV_FACTOR_EMP             24
#define VDP_DEI_CTL_KMOV_15_16                  0
#define VDP_DEI_CTL_KMOV_14_16                  1
/*T_DETAIL : */
#define VDP_DEI_CTL_T_DETAIL_MASK               0x0F
#define VDP_DEI_CTL_T_DETAIL_EMP                20
#define VDP_DEI_CTL_BEST_DETAIL_VALUE           0x0A            /* 10 is the best value to avoid noise sensibility */
/* KCOR : Correction factor used in the fader.Unsigned fractional value,   */
#define VDP_DEI_CTL_KCOR_MASK                   0x0F
#define VDP_DEI_CTL_KCOR_EMP                    16
#define VDP_DEI_CTL_BEST_KCOR_VALUE             8  /* 8 is the recommended value for KCOR */
/* DIR_REC_EN: */
#define VDP_DEI_CTL_DIR_REC_EN_MASK             0x01
#define VDP_DEI_CTL_DIR_REC_EN_EMP              15
#define VDP_DEI_CTL_DIR_RECURSIVITY_OFF         0
#define VDP_DEI_CTL_DIR_RECURSIVITY_ON          1
/* DIR3_EN: */
#define VDP_DEI_CTL_DIR3_EN_MASK                0x01
#define VDP_DEI_CTL_DIR3_EN_EMP                 14
#define VDP_DEI_CTL_3RD_DIR_NOT_USED            0
#define VDP_DEI_CTL_3RD_DIR_USED                1
/* CLAMP: */
#define VDP_DEI_CTL_CLAMP_MODE_MASK             0x01
#define VDP_DEI_CTL_CLAMP_MODE_EMP              13
#define VDP_DEI_CTL_VERTICAL_CLAMPING           0
#define VDP_DEI_CTL_DIRECTIONAL_CLAMPING        1
/* F_MODE : Defines the fader mode. */
#define VDP_DEI_CTL_F_MODE_MASK                 0x01
#define VDP_DEI_CTL_F_MODE_EMP                  12
#define VDP_DEI_CTL_FADER_MLD                   0
#define VDP_DEI_CTL_FADER_LMU                   1
/* DD: Detail Detector */
#define VDP_DEI_CTL_DD_MODE_MASK                0x03
#define VDP_DEI_CTL_DD_MODE_EMP                 10
#define VDP_DEI_CTL_DD_MODE_DIAG5               0x00
#define VDP_DEI_CTL_DD_MODE_DIAG7               0x01
#define VDP_DEI_CTL_DD_MODE_DIAG9               0x02
/* MD: Motion Detector */
#define VDP_DEI_CTL_MD_MODE_MASK                0x03
#define VDP_DEI_CTL_MD_MODE_EMP                 8
#define VDP_DEI_CTL_MD_MODE_OFF                 0x00
#define VDP_DEI_CTL_MD_MODE_INIT                0x01
#define VDP_DEI_CTL_MD_MODE_LOW                 0x02
#define VDP_DEI_CTL_MD_MODE_FULL                0x03
/* CHROMA_INTERP : Defines the interpolation algorithm used for chroma */
#define VDP_DEI_CTL_CHROMA_INTERP_MASK          0x03
#define VDP_DEI_CTL_CHROMA_INTERP_EMP           6
#define VDP_DEI_CTL_CHROMA_VI                   0x00
#define VDP_DEI_CTL_CHROMA_DI                   0x01
#define VDP_DEI_CTL_CHROMA_MEDIAN               0x02
#define VDP_DEI_CTL_CHROMA_3D                   0x03
/* LUMA_INTERP : Defines the interpolation algorithm used for luma */
#define VDP_DEI_CTL_LUMA_INTERP_MASK            0x03
#define VDP_DEI_CTL_LUMA_INTERP_EMP             4
#define VDP_DEI_CTL_LUMA_VI                     0x00
#define VDP_DEI_CTL_LUMA_DI                     0x01
#define VDP_DEI_CTL_LUMA_MEDIAN                 0x02
#define VDP_DEI_CTL_LUMA_3D                     0x03
/* MAIN_MODE : Selects a group of configurations for the deinterlacer */
#define VDP_DEI_CTL_MAIN_MODE_MASK              0x03
#define VDP_DEI_CTL_MAIN_MODE_EMP               2
#define VDP_DEI_CTL_FIELD_MERGING               0x01
#define VDP_DEI_CTL_DEINTERLACING               0x02
#define VDP_DEI_CTL_TIME_UP_CONV                0x03
/* BYPASS : */
#define VDP_DEI_CTL_BYPASS_MASK		            0x1
#define VDP_DEI_CTL_BYPASS_EMP		            1
#define VDP_DEI_CTL_BYPASS_OFF                  0
#define VDP_DEI_CTL_BYPASS_ON                   1
/* INACTIVE : */
#define VDP_DEI_CTL_INACTIVE_MASK	            0x1
#define VDP_DEI_CTL_INACTIVE_EMP	            0
#define VDP_DEI_CTL_ACTIVE                      0
#define VDP_DEI_CTL_INACTIVE                    1

/* DEI Modes Available: */
#define VDP_DEI_SET_MODE_OFF           (VDP_DEI_CTL_INACTIVE_MASK << VDP_DEI_CTL_INACTIVE_EMP)

#define VDP_DEI_SET_MODE_BYPASS        (VDP_DEI_CTL_BYPASS_MASK << VDP_DEI_CTL_BYPASS_EMP)

#define VDP_DEI_SET_MODE_YC_FM         (VDP_DEI_CTL_FIELD_MERGING << VDP_DEI_CTL_MAIN_MODE_EMP)

#define VDP_DEI_SET_MODE_YC_VI         ( (VDP_DEI_CTL_DEINTERLACING << VDP_DEI_CTL_MAIN_MODE_EMP)   | \
                                         (VDP_DEI_CTL_LUMA_VI << VDP_DEI_CTL_LUMA_INTERP_EMP)       | \
                                         (VDP_DEI_CTL_CHROMA_VI << VDP_DEI_CTL_CHROMA_INTERP_EMP) )

#define VDP_DEI_SET_MODE_YC_DI         ( (VDP_DEI_CTL_DEINTERLACING << VDP_DEI_CTL_MAIN_MODE_EMP)   | \
                                         (VDP_DEI_CTL_LUMA_DI << VDP_DEI_CTL_LUMA_INTERP_EMP)       | \
                                         (VDP_DEI_CTL_CHROMA_DI << VDP_DEI_CTL_CHROMA_INTERP_EMP) )

#define VDP_DEI_SET_MODE_YC_MF         ( (VDP_DEI_CTL_DEINTERLACING << VDP_DEI_CTL_MAIN_MODE_EMP)   | \
                                         (VDP_DEI_CTL_LUMA_MEDIAN << VDP_DEI_CTL_LUMA_INTERP_EMP)   | \
                                         (VDP_DEI_CTL_CHROMA_MEDIAN << VDP_DEI_CTL_CHROMA_INTERP_EMP) )

#define VDP_DEI_SET_MODE_YC_MLD        ( (VDP_DEI_CTL_DEINTERLACING << VDP_DEI_CTL_MAIN_MODE_EMP) | \
                                         (VDP_DEI_CTL_LUMA_3D << VDP_DEI_CTL_LUMA_INTERP_EMP)     | \
                                         (VDP_DEI_CTL_CHROMA_3D << VDP_DEI_CTL_CHROMA_INTERP_EMP) | \
                                         (VDP_DEI_CTL_MD_MODE_FULL << VDP_DEI_CTL_MD_MODE_EMP) | \
                                         (VDP_DEI_CTL_FADER_MLD << VDP_DEI_CTL_F_MODE_EMP) )

#define VDP_DEI_SET_MODE_YC_LMU        ( (VDP_DEI_CTL_DEINTERLACING << VDP_DEI_CTL_MAIN_MODE_EMP) | \
                                         (VDP_DEI_CTL_LUMA_3D << VDP_DEI_CTL_LUMA_INTERP_EMP)     | \
                                         (VDP_DEI_CTL_CHROMA_3D << VDP_DEI_CTL_CHROMA_INTERP_EMP) | \
                                         (VDP_DEI_CTL_MD_MODE_FULL << VDP_DEI_CTL_MD_MODE_EMP)    | \
                                         (VDP_DEI_CTL_FADER_LMU << VDP_DEI_CTL_F_MODE_EMP) )

#define VDP_DEI_SET_MODE_MASK   ( (VDP_DEI_CTL_INACTIVE_MASK << VDP_DEI_CTL_INACTIVE_EMP)           | \
                                  (VDP_DEI_CTL_BYPASS_MASK << VDP_DEI_CTL_BYPASS_EMP)               | \
                                  (VDP_DEI_CTL_MAIN_MODE_MASK << VDP_DEI_CTL_MAIN_MODE_EMP)         | \
                                  (VDP_DEI_CTL_LUMA_INTERP_MASK << VDP_DEI_CTL_LUMA_INTERP_EMP)     | \
                                  (VDP_DEI_CTL_CHROMA_INTERP_MASK << VDP_DEI_CTL_CHROMA_INTERP_EMP) | \
                                  (VDP_DEI_CTL_MD_MODE_MASK << VDP_DEI_CTL_MD_MODE_EMP)             | \
                                  (VDP_DEI_CTL_F_MODE_MASK << VDP_DEI_CTL_F_MODE_EMP) )

/* Mask for changing 3D parameters */
#define VDP_DEI_3D_PARAMS_MASK       ( (VDP_DEI_CTL_DD_MODE_MASK << VDP_DEI_CTL_DD_MODE_EMP)          | \
                                       (VDP_DEI_CTL_CLAMP_MODE_MASK << VDP_DEI_CTL_CLAMP_MODE_EMP)    | \
                                       (VDP_DEI_CTL_DIR3_EN_MASK << VDP_DEI_CTL_DIR3_EN_EMP)          | \
                                       (VDP_DEI_CTL_DIR_REC_EN_MASK << VDP_DEI_CTL_DIR_REC_EN_EMP)    | \
                                       (VDP_DEI_CTL_KCOR_MASK << VDP_DEI_CTL_KCOR_EMP)                | \
                                       (VDP_DEI_CTL_T_DETAIL_MASK << VDP_DEI_CTL_T_DETAIL_EMP)        | \
                                       (VDP_DEI_CTL_KMOV_FACTOR_MASK << VDP_DEI_CTL_KMOV_FACTOR_EMP) )

#define VDP_DEI_VIEWPORT_ORIG		        0x004	/* output viewport origin */

#define VDP_DEI_VIEWPORT_ORIG_Y_MASK 		0x7ff
#define VDP_DEI_VIEWPORT_ORIG_X_MASK 		0x7ff
#define VDP_DEI_VIEWPORT_ORIG_X_EMP		    0            /* Position of Orig_X */
#define VDP_DEI_VIEWPORT_ORIG_Y_EMP		    16           /* Position of Orig_Y */

#define VDP_DEI_VIEWPORT_SIZE		        0x008	 /* output viewport size */
#define VDP_DEI_VIEWPORT_SIZE_WIDTH_EMP	    0             /* Position of VP Width */
#define VDP_DEI_VIEWPORT_SIZE_HEIGHT_EMP	16           /* Position of VP Height */

#define VDP_DEI_VIEWPORT_SIZE_H_MASK	    0x7ff
#define VDP_DEI_VIEWPORT_SIZE_W_MASK	    0x7ff

#define VDP_DEI_VF_SIZE				        0x00C	/* width of video fields in MB format */

#define VDP_DEI_T3I_CTL			                0x010	    /* stbus if control */
#define VDP_DEI_T3I_PRIO_MASK		            0xf
#define VDP_DEI_T3I_PRIO_EMP		            26
#define VDP_DEI_T3I_MSBR_MASK		            0x3ff
#define VDP_DEI_T3I_MSBR_EMP		            16
#define VDP_DEI_T3I_YC_BUFFERS_EMP	            15
#define VDP_DEI_T3I_YC_ENDIANESS_EMP            14
#define VDP_DEI_T3I_MB_ENABLE_EMP	            13
#define VDP_DEI_T3I_WAVSYNC_MASK	            0x1f
#define VDP_DEI_T3I_WAVSYNC_EMP	                8
#define VDP_DEI_T3I_OPSIZE_MASK	                0x7
#define VDP_DEI_T3I_OPSIZE_EMP		            5
#define VDP_DEI_T3I_CHKSIZE_MASK	            0x1f
#define VDP_DEI_T3I_CHKSIZE_EMP	                0

/* these BA need to be seen by the video decoder ? as in SDDISP ? */
#define VDP_DEI_PYF_BA				0x014	/* luma previous field base address */
#define VDP_DEI_CYF_BA				0x018	/* luma current field base address */
#define VDP_DEI_NYF_BA				0x01C	/* luma next field base address */
#define VDP_DEI_PCF_BA				0x020	/* chroma previous field base address */
#define VDP_DEI_CCF_BA				0x024	/* chroma current field base address */
#define VDP_DEI_NCF_BA				0x028	/* chroma next field base address */
#define VDP_DEI_PMF_BA				0x02C	/* motion buffer previous field base address */
#define VDP_DEI_CMF_BA				0x030	/* motion buffer current field base address */
#define VDP_DEI_NMF_BA				0x034	/* motion buffer next field base address */

/* Offsets for BaseAddress registers */
#define LUMA_TOP_BA_OFFSET              8
#define LUMA_BOTTOM_BA_OFFSET           136         /* = 0x88 */
#define LUMA_PROGRESSIVE_BA_OFFSET      8
#define CHROMA_TOP_BA_OFFSET            8
#define CHROMA_BOTTOM_BA_OFFSET         72           /* = 0x48 */
#define CHROMA_PROGRESSIVE_BA_OFFSET    8

/* T3I memory access */
#define VDP_DEI_YF_FORMAT			0x038	/* luma buffer memory format */
#define VDP_DEI_YF_STACK_L0			0x03C	/* luma line address generation stack */
#define VDP_DEI_YF_STACK_L1			0x040
#define VDP_DEI_YF_STACK_L2			0x044
#define VDP_DEI_YF_STACK_L3			0x048
#define VDP_DEI_YF_STACK_L4			0x04C
#define VDP_DEI_YF_STACK_P0			0x050	/* luma pixel address generation stack */
#define VDP_DEI_YF_STACK_P1			0x054
#define VDP_DEI_YF_STACK_P2			0x058
#define VDP_DEI_YF_STACK_CEIL_EMP   0
#define VDP_DEI_YF_STACK_CEIL_MASK  0X3FF
#define VDP_DEI_YF_STACK_OFFSET_EMP 10
#define VDP_DEI_YF_STACK_OFFSET_MASK 0x3FFFFF
#define VDP_DEI_CF_FORMAT			0x05C	/* chroma buffer memory format */
#define VDP_DEI_CF_STACK_L0			0x060	/* chroma line address generation stack */
#define VDP_DEI_CF_STACK_L1			0x064
#define VDP_DEI_CF_STACK_L2			0x068
#define VDP_DEI_CF_STACK_L3			0x06C
#define VDP_DEI_CF_STACK_L4			0x070
#define VDP_DEI_CF_STACK_P0			0x074	/* chroma pixel address generation stack */
#define VDP_DEI_CF_STACK_P1			0x078
#define VDP_DEI_CF_STACK_P2			0x07C
#define VDP_DEI_CF_STACK_CEIL_EMP   0
#define VDP_DEI_CF_STACK_CEIL_MASK  0X3FF
#define VDP_DEI_CF_STACK_OFFSET_EMP   10
#define VDP_DEI_CF_STACK_OFFSET_MASK 0x3FFFFF
#define VDP_DEI_MF_STACK_L0			0x080   /* motion line address generation stack */
#define VDP_DEI_MF_STACK_P0			0x084	/* motion pixel address generation stack */
#define VDP_DEI_DEBUG1			        	0x088	/* */

#define VDP_DEI_STATUS1                     0x08C     /* Deinterlacer Status register */
#define VDP_DEI_STATUS_DEI_DONE    (U32) 0x00400000

/* scene change detection */
#define VDP_FMD_THRESHOLD_SCD		0x090	/* scene change detection algorithm threshold */
#define VDP_FMD_SCENE_EMP                      0
#define VDP_FMD_SCENE_MASK                    0xFFFFF
/* repeat field detection */
#define VDP_FMD_THRESHOLD_RFD		0x094	/* repeat field detection algorithm threshold */
#define VDP_FMD_REPEAT_EMP                    0
#define VDP_FMD_REPEAT_MASK                  0xFFFFF
/* move detection */
#define VDP_FMD_THRESHOLD_MOVE		0x098	/* move detection algorithm threshold */
#define VDP_FMD_MOVE_EMP                        0
#define VDP_FMD_MOVE_MASK                      0xFF
#define VDP_FMD_NUM_MOVE_PIX_EMP        16
#define VDP_FMD_NUM_MOVE_PIX_MASK      0xFFF
/* consecutive field difference */
#define VDP_FMD_THRESHOLD_CFD		0x09C	/* consecutive field difference algorithm threshold */
#define VDP_FMD_NOISE_EMP                        0
#define VDP_FMD_NOISE_MASK                      0x0F
/* BLOCK_NUMBER */
#define VDP_FMD_BLOCK_NUMBER		       0x0A0	/* number of blocks to be used for block based difference */
#define VDP_FMD_H_BLK_NB_EMP                 0
#define VDP_FMD_H_BLK_NB_MASK               0x0F
#define VDP_FMD_V_BLK_NB_EMP                 8
#define VDP_FMD_V_BLK_NB_MASK               0x0F
/* FMD_CFD_SUM */
#define VDP_FMD_CFD_SUM				0x0A4	/* sum of consecutive field difference pixels differences for one field */
/* FIELD_SUM */
#define VDP_FMD_FIELD_SUM			       0x0A8	/* sum of |A-D| pixels differences for one field */
/* FMD_STATUS */
#define VDP_FMD_STATUS				       0x0AC	/* status */
#define VDP_FMD_MOVE_STATUS			0x0200
#define VDP_FMD_REPEAT_STATUS			0x0400
#define VDP_FMD_SCENECOUNT_MASK		0x01FF

#define VDP_VHSRC_CTL				0x100	/* display Control register */

#define VDP_VHSRC_CTL_MASK          0x03FF003F
#define VDP_VHSRC_CTL_CFLDLINE_MASK	0x1f	/* coeff Load line */
#define VDP_VHSRC_CTL_CFLDLINE_EMP	16
#define VDP_VHSRC_CTL_PXLDLINE_MASK	0x1f	/* pix load line */
#define VDP_VHSRC_CTL_PXLDLINE_EMP	21
#define VDP_VHSRC_CTL_ENA_VHSRC_MASK 0x01	/* Enable VHSRC */
#define VDP_VHSRC_CTL_ENA_VHSRC_EMP	5

#define VDP_VHSRC_Y_HSRC			0x104	/* display Luma HSRC */
#define VDP_VHSRC_Y_VSRC			0x108	/* display Luma VSRC */
#define VDP_VHSRC_C_HSRC			0x10C   /* display Chroma HSRC */
#define VDP_VHSRC_C_VSRC			0x110   /* display Chroma VSRC */
#define VDP_VHSRC_TARGET_SIZE		0x114   /* display Target Pixmap Size */
#define VDP_VHSRC_NLZZD_Y			0x118   /* display Luma   Non Linear Zoom Zone Definition */
#define VDP_VHSRC_NLZZD_C			0x11C   /* display Chroma Non Linear Zoom Zone Definition */
#define VDP_VHSRC_PDELTA			0x120   /* display Non Linear Zoom - Increment Step Definition */
#define VDP_VHSRC_Y_SIZE			0x124   /* source pixmap width in luma pixel units */
#define VDP_VHSRC_C_SIZE			0x128   /* source pixmap width in chroma pixel units */
#define VDP_VHSRC_HCOEF_BA			0x12C   /* display horizontal filter coefficients address */
#define VDP_VHSRC_VCOEF_BA			0x130	/* display vertical filter coefficients address */

#define VDP_VHSRC_LHF_COEF0         0x100	/* Display Luma   Hor. filter Coefficient 0  */
#define VDP_VHSRC_LHF_COEF34        0x188	/* Display Luma   Hor. filter Coefficient 34  */
#define VDP_VHSRC_LVF_COEF21        0x1E0	/* Display Luma   Ver. filter Coefficient 21  */


#define VDP_P2I_PXF_IT_STATUS		0x300	/* P2I & pixel fifo status */
#define VDP_P2I_PXF_IT_MASK			0x304	/* P2I & pixelf fifo mask */
#define VDP_PXF_END_PROCESSING      (U32)  0x00000001
#define VDP_PXF_FIFO_EMPTY               (U32)  0x00000002

#define VDP_Y_CRC_CHECK_SUM			0x400	/* crc */
#define VDP_UV_CRC_CHECK_SUM		0x404	/* crc */


#define VDP_VHSRC_INITIAL_PHASE_MASK    0x1FFF
#define VDP_VHSRC_MAX_XY_MASK           0x7FF   /* Display Max Positions */
#define VDP_VHSRC_X_EMP                 0       /* Position of X bits in registers */
#define VDP_VHSRC_Y_EMP                 16      /* Position of X bits in registers */
#define VDP_VHSRC_MAX_SIZE_MASK         0x7FF   /* Display Max Sizes */
#define VDP_VHSRC_WIDTH_EMP             0       /* Position of width bits in registers */
#define VDP_VHSRC_HEIGHT_EMP            16      /* Position of height bits in registers */



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

#endif /* #ifndef __LAYER_DISPLAY_REGISTERS_DISPLAYPIPE_H */

/* End of hv_vdpreg.h */
