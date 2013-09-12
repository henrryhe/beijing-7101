/*******************************************************************************

File name   : blt_mem.h

Description : header for file which writes nodes in shared memory

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
20 Jun 2000        Created                                           TM
******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __BLT_MEM_H
#define __BLT_MEM_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stgxobj.h"
#include "blt_com.h"
#include "blt_init.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STBLIT_MAX_WIDTH  0xFFF   /* 12 bit HW constraint */
#define STBLIT_MAX_HEIGHT 0xFFF   /* 12 bit HW constraint */

/* Next Instruction Pointer */
/*#ifdef STBLIT_EMULATOR  */
#define STBLIT_NIP_POINTER_MASK                 0xFFFFFFFF    /* TBD */
/*#else*/
/*#define STBLIT_NIP_POINTER_MASK                 0x03FFFFFF*/
/*#endif*/

#define STBLIT_NIP_POINTER_SHIFT                0
#define STBLIT_NIP_BANK_MASK                    0x3F
#define STBLIT_NIP_BANK_SHIFT                   26

/* User specific  */
#define STBLIT_USR_MASK                         0xFFFFFFFF
#define STBLIT_USR_SHIFT                        0

/* Instruction  */
#define STBLIT_INS_SRC1_MODE_MASK               7
#define STBLIT_INS_SRC1_MODE_SHIFT              0
#define STBLIT_INS_SRC1_MODE_DISABLED           0
#define STBLIT_INS_SRC1_MODE_MEMORY             1
#define STBLIT_INS_SRC1_MODE_COLOR_FILL         3
#define STBLIT_INS_SRC1_MODE_DIRECT_COPY        4
#define STBLIT_INS_SRC1_MODE_DIRECT_FILL        7

#define STBLIT_INS_SRC2_MODE_MASK               3
#define STBLIT_INS_SRC2_MODE_SHIFT              3
#define STBLIT_INS_SRC2_MODE_DISABLED           0
#define STBLIT_INS_SRC2_MODE_MEMORY             1
#define STBLIT_INS_SRC2_MODE_COLOR_FILL         3

#define STBLIT_INS_ENABLE_IN_CSC_MASK           1
#define STBLIT_INS_ENABLE_IN_CSC_SHIFT          6
#define STBLIT_INS_ENABLE_CLUT_MASK             1
#define STBLIT_INS_ENABLE_CLUT_SHIFT            7
#define STBLIT_INS_ENABLE_RESCALE_MASK          1
#define STBLIT_INS_ENABLE_RESCALE_SHIFT         8
#define STBLIT_INS_ENABLE_FLICKER_FILTER_MASK   1
#define STBLIT_INS_ENABLE_FLICKER_FILTER_SHIFT  9
#define STBLIT_INS_ENABLE_RECT_CLIPPING_MASK    1
#define STBLIT_INS_ENABLE_RECT_CLIPPING_SHIFT   10
#define STBLIT_INS_ENABLE_COLOR_KEY_MASK        1
#define STBLIT_INS_ENABLE_COLOR_KEY_SHIFT       11
#define STBLIT_INS_ENABLE_OUT_CSC_MASK          1
#define STBLIT_INS_ENABLE_OUT_CSC_SHIFT         12
#define STBLIT_INS_ENABLE_PLANE_MASK            1
#define STBLIT_INS_ENABLE_PLANE_SHIFT           14

#define STBLIT_INS_ENABLE_IRQ_MASK              7
#define STBLIT_INS_ENABLE_IRQ_SHIFT             18
#define STBLIT_INS_ENABLE_IRQ_COMPLETED         1
#define STBLIT_INS_ENABLE_IRQ_READY2START       2
#define STBLIT_INS_ENABLE_IRQ_SUSPENDED         4

#define STBLIT_INS_XYL_MODE_MASK                1
#define STBLIT_INS_XYL_MODE_SHIFT               23
#define STBLIT_INS_XYL_MODE_ENABLE              1

#define STBLIT_INS_TRIGGER_CONTROL_MASK         0x7F
#define STBLIT_INS_TRIGGER_CONTROL_SHIFT        24
#define STBLIT_INS_TRIGGER_VTG_SELECT           1
#define STBLIT_INS_TRIGGER_RASTER_TOP           2
#define STBLIT_INS_TRIGGER_RASTER_BOT           4
#define STBLIT_INS_TRIGGER_CAPTURE_TOP          8
#define STBLIT_INS_TRIGGER_CAPTURE_BOT          16
#define STBLIT_INS_TRIGGER_VSYNC_TOP            32
#define STBLIT_INS_TRIGGER_VSYNC_BOT            64

/* Source1 Base Address */
/*#ifdef STBLIT_EMULATOR */
#define STBLIT_S1BA_POINTER_MASK                0xFFFFFFFF  /* TBD */
/*#else*/
/*#define STBLIT_S1BA_POINTER_MASK                0x3FFFFFF*/
/*#endif*/

#define STBLIT_S1BA_POINTER_SHIFT               0
#define STBLIT_S1BA_BANK_MASK                   0x3F
#define STBLIT_S1BA_BANK_SHIFT                  26

/* Source1 Type */
#define STBLIT_S1TY_PITCH_MASK                  0xFFFF
#define STBLIT_S1TY_PITCH_SHIFT                 0
#define STBLIT_S1TY_COLOR_MASK                  0x1F
#define STBLIT_S1TY_COLOR_SHIFT                 16
#define STBLIT_S1TY_ALPHA_RANGE_MASK            1
#define STBLIT_S1TY_ALPHA_RANGE_SHIFT           21
#define STBLIT_S1TY_SCAN_ORDER_MASK             3
#define STBLIT_S1TY_SCAN_ORDER_SHIFT            24
#define STBLIT_S1TY_SUB_BYTE_MASK               1
#define STBLIT_S1TY_SUB_BYTE_SHIFT              28
#define STBLIT_S1TY_RGB_EXPANSION_MASK          1
#define STBLIT_S1TY_RGB_EXPANSION_SHIFT         29
#define STBLIT_S1TY_BIG_NOT_LITTLE_MASK         1
#define STBLIT_S1TY_BIG_NOT_LITTLE_SHIFT        30

/* Source1 XY Location */
#define STBLIT_S1XY_X_MASK                      0xFFFF
#define STBLIT_S1XY_X_SHIFT                     0
#define STBLIT_S1XY_Y_MASK                      0xFFFF
#define STBLIT_S1XY_Y_SHIFT                     16

/* Source1 Size */
#define STBLIT_S1SZ_WIDTH_MASK                  0xFFF
#define STBLIT_S1SZ_WIDTH_SHIFT                 0
#define STBLIT_S1SZ_HEIGHT_MASK                 0xFFF
#define STBLIT_S1SZ_HEIGHT_SHIFT                16

/* Source1 Color Fill */
#define STBLIT_S1CF_COLOR_MASK                  0xFFFFFFFF
#define STBLIT_S1CF_COLOR_SHIFT                 0

/* Source2 Base Address */
/*#ifdef STBLIT_EMULATOR  */
#define STBLIT_S2BA_POINTER_MASK                0xFFFFFFFF  /* TBD */
/*#else*/
/*#define STBLIT_S2BA_POINTER_MASK                0x3FFFFFF*/
/*#endif*/

#define STBLIT_S2BA_POINTER_SHIFT               0
#define STBLIT_S2BA_BANK_MASK                   0x3F
#define STBLIT_S2BA_BANK_SHIFT                  26

/* Source2 Type */
#define STBLIT_S2TY_PITCH_MASK                  0xFFFF
#define STBLIT_S2TY_PITCH_SHIFT                 0
#define STBLIT_S2TY_COLOR_MASK                  0x1F
#define STBLIT_S2TY_COLOR_SHIFT                 16
#define STBLIT_S2TY_ALPHA_RANGE_MASK            1
#define STBLIT_S2TY_ALPHA_RANGE_SHIFT           21
#define STBLIT_S2TY_SCAN_ORDER_MASK             3
#define STBLIT_S2TY_SCAN_ORDER_SHIFT            24
#define STBLIT_S2TY_CHROMA_EXPANSION_MASK       1
#define STBLIT_S2TY_CHROMA_EXPANSION_SHIFT      26
#define STBLIT_S2TY_CHROMA_LINE_REPEAT_MASK     1
#define STBLIT_S2TY_CHROMA_LINE_REPEAT_SHIFT    27
#define STBLIT_S2TY_SUB_BYTE_MASK               1
#define STBLIT_S2TY_SUB_BYTE_SHIFT              28
#define STBLIT_S2TY_RGB_EXPANSION_MASK          1
#define STBLIT_S2TY_RGB_EXPANSION_SHIFT         29
#define STBLIT_S2TY_BIG_NOT_LITTLE_MASK         1
#define STBLIT_S2TY_BIG_NOT_LITTLE_SHIFT        30

/* Source2 XY Location */
#define STBLIT_S2XY_X_MASK                      0xFFFF
#define STBLIT_S2XY_X_SHIFT                     0
#define STBLIT_S2XY_Y_MASK                      0xFFFF
#define STBLIT_S2XY_Y_SHIFT                     16

/* Source2 Size */
#define STBLIT_S2SZ_WIDTH_MASK                  0xFFF
#define STBLIT_S2SZ_WIDTH_SHIFT                 0
#define STBLIT_S2SZ_HEIGHT_MASK                 0xFFF
#define STBLIT_S2SZ_HEIGHT_SHIFT                16

/* Source2 Color Fill */
#define STBLIT_S2CF_COLOR_MASK                  0xFFFFFFFF
#define STBLIT_S2CF_COLOR_SHIFT                 0

/* Target Base Address */
/*#ifdef STBLIT_EMULATOR */
#define STBLIT_TBA_POINTER_MASK                 0xFFFFFFFF /* TBD */
/*#else*/
/*#define STBLIT_TBA_POINTER_MASK                 0x3FFFFFF*/
/*#endif*/

#define STBLIT_TBA_POINTER_SHIFT                0
#define STBLIT_TBA_BANK_MASK                    0x3F
#define STBLIT_TBA_BANK_SHIFT                   26

/* Target Type */
#define STBLIT_TTY_PITCH_MASK                  0xFFFF
#define STBLIT_TTY_PITCH_SHIFT                 0
#define STBLIT_TTY_COLOR_MASK                  0x1F
#define STBLIT_TTY_COLOR_SHIFT                 16
#define STBLIT_TTY_ALPHA_RANGE_MASK            1
#define STBLIT_TTY_ALPHA_RANGE_SHIFT           21
#define STBLIT_TTY_SCAN_ORDER_MASK             3
#define STBLIT_TTY_SCAN_ORDER_SHIFT            24
#define STBLIT_TTY_ENABLE_2D_DITHER_MASK       1
#define STBLIT_TTY_ENABLE_2D_DITHER_SHIFT      26
#define STBLIT_TTY_CHROMA_NOT_LUMA_MASK        1
#define STBLIT_TTY_CHROMA_NOT_LUMA_SHIFT       27
#define STBLIT_TTY_SUB_BYTE_MASK               1
#define STBLIT_TTY_SUB_BYTE_SHIFT              28
#define STBLIT_TTY_BIG_NOT_LITTLE_MASK         1
#define STBLIT_TTY_BIG_NOT_LITTLE_SHIFT        30

/* Target XY Location */
#define STBLIT_TXY_X_MASK                      0xFFFF
#define STBLIT_TXY_X_SHIFT                     0
#define STBLIT_TXY_Y_MASK                      0xFFFF
#define STBLIT_TXY_Y_SHIFT                     16

/* Clipping Window Offset */
#define STBLIT_CWO_XDO_MASK                    0x7FFF
#define STBLIT_CWO_XDO_SHIFT                   0
#define STBLIT_CWO_YDO_MASK                    0x7FFF
#define STBLIT_CWO_YDO_SHIFT                   16
#define STBLIT_CWO_INT_NOT_EXT_MASK            1
#define STBLIT_CWO_INT_NOT_EXT_SHIFT           31

/* Clipping Window Stop */
#define STBLIT_CWS_XDS_MASK                    0x7FFF
#define STBLIT_CWS_XDS_SHIFT                   0
#define STBLIT_CWS_YDS_MASK                    0x7FFF
#define STBLIT_CWS_YDS_SHIFT                   16

/* Color Conversion Operators */

#define STBLIT_CCO_IN_MASK                     7
#define STBLIT_CCO_IN_SHIFT                    0
#define STBLIT_CCO_IN_COLORIMETRY_MASK         1
#define STBLIT_CCO_IN_COLORIMETRY_SHIFT        0
#define STBLIT_CCO_IN_CHROMA_FORMAT_MASK       1
#define STBLIT_CCO_IN_CHROMA_FORMAT_SHIFT      1
#define STBLIT_CCO_IN_DIRECTION_MASK           1
#define STBLIT_CCO_IN_DIRECTION_SHIFT          2
#define STBLIT_CCO_OUT_MASK                    3
#define STBLIT_CCO_OUT_SHIFT                   8
#define STBLIT_CCO_OUT_COLORIMETRY_MASK        1
#define STBLIT_CCO_OUT_COLORIMETRY_SHIFT       8
#define STBLIT_CCO_OUT_CHROMA_FORMAT_MASK      1
#define STBLIT_CCO_OUT_CHROMA_FORMAT_SHIFT     9

#define STBLIT_CCO_CLUT_OPMODE_MASK               3
#define STBLIT_CCO_CLUT_OPMODE_SHIFT              16
#define STBLIT_CCO_CLUT_OPMODE_EXPANSION          0
#define STBLIT_CCO_CLUT_OPMODE_CORRECTION         1
#define STBLIT_CCO_CLUT_OPMODE_REDUCTION          2

#define STBLIT_CCO_ENABLE_CLUT_UPDATE_MASK        1
#define STBLIT_CCO_ENABLE_CLUT_UPDATE_SHIFT       18

#define STBLIT_CCO_ERROR_DIFF_WEIGHT_MASK         0xF
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_SHIFT        19
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_DISABLED     0
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_100          1
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_75           2
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_50           3
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_25           4
#define STBLIT_CCO_ERROR_DIFF_WEIGHT_ADAPT        8

/* CLUT Memory Location */
/*#ifdef STBLIT_EMULATOR */
#define STBLIT_CML_POINTER_MASK                 0xFFFFFFFF  /* TBD */
/*#else*/
/*#define STBLIT_CML_POINTER_MASK                 0x3FFFFFF*/
/*#endif*/
#define STBLIT_CML_POINTER_SHIFT                0
#define STBLIT_CML_BANK_MASK                    0x3F
#define STBLIT_CML_BANK_SHIFT                   26

/* 2D Resize Control */

#define STBLIT_RZC_2DHFILTER_MODE_MASK           3
#define STBLIT_RZC_2DHFILTER_MODE_SHIFT          0
#define STBLIT_RZC_2DHFILTER_INACTIVE            0
#define STBLIT_RZC_2DHFILTER_ACTIV_COLOR         1
#define STBLIT_RZC_2DHFILTER_ACTIV_ALPHA         2
#define STBLIT_RZC_2DHFILTER_ACTIV_BOTH          3

#define STBLIT_RZC_2DHRESIZER_MODE_MASK           1
#define STBLIT_RZC_2DHRESIZER_MODE_SHIFT          2
#define STBLIT_RZC_2DHRESIZER_ENABLE              1

#define STBLIT_RZC_2DVFILTER_MODE_MASK            3
#define STBLIT_RZC_2DVFILTER_MODE_SHIFT           4
#define STBLIT_RZC_2DVFILTER_INACTIVE             0
#define STBLIT_RZC_2DVFILTER_ACTIV_COLOR          1
#define STBLIT_RZC_2DVFILTER_ACTIV_ALPHA          2
#define STBLIT_RZC_2DVFILTER_ACTIV_BOTH           3

#define STBLIT_RZC_2DVRESIZER_MODE_MASK           1
#define STBLIT_RZC_2DVRESIZER_MODE_SHIFT          6
#define STBLIT_RZC_2DVRESIZER_ENABLE              1

#define STBLIT_RZC_FFILTER_MODE_MASK              3
#define STBLIT_RZC_FFILTER_MODE_SHIFT             8
#define STBLIT_RZC_FFILTER_MODE_FORCE0            0
#define STBLIT_RZC_FFILTER_MODE_ADAPTATIVE        1
#define STBLIT_RZC_FFILTER_MODE_TEST              2

#define STBLIT_RZC_ENABLE_ALPHA_H_BORDER_MASK     1
#define STBLIT_RZC_ENABLE_ALPHA_H_BORDER_SHIFT    12
#define STBLIT_RZC_ENABLE_ALPHA_V_BORDER_MASK     1
#define STBLIT_RZC_ENABLE_ALPHA_V_BORDER_SHIFT    13
#define STBLIT_RZC_INITIAL_SUBPOS_HSRC_MASK       7
#define STBLIT_RZC_INITIAL_SUBPOS_HSRC_SHIFT      16
#define STBLIT_RZC_INITIAL_SUBPOS_VSRC_MASK       7
#define STBLIT_RZC_INITIAL_SUBPOS_VSRC_SHIFT      20

/* H Filter Coefficients Pointer */
/*#ifdef STBLIT_EMULATOR*/
#define STBLIT_HFP_POINTER_MASK                 0xFFFFFFFF  /* TBD */
/*#else*/
/*#define STBLIT_HFP_POINTER_MASK                 0x3FFFFFF*/
/*#endif*/
#define STBLIT_HFP_POINTER_SHIFT                0
#define STBLIT_HFP_BANK_MASK                    0x3F
#define STBLIT_HFP_BANK_SHIFT                   26

/* V Filter Coefficients Pointer */

/*#ifdef STBLIT_EMULATOR */
#define STBLIT_VFP_POINTER_MASK                 0xFFFFFFFF   /* TBD */
/*#else*/
/*#define STBLIT_VFP_POINTER_MASK                 0x3FFFFFF*/
/*#endif*/
#define STBLIT_VFP_POINTER_SHIFT                0
#define STBLIT_VFP_BANK_MASK                    0x3F
#define STBLIT_VFP_BANK_SHIFT                   26

/* Resize Scaling Factor */
#define STBLIT_RSF_INCREMENT_HSRC_MASK                 0xFFFF
#define STBLIT_RSF_INCREMENT_HSRC_SHIFT                0
#define STBLIT_RSF_INCREMENT_VSRC_MASK                 0xFFFF
#define STBLIT_RSF_INCREMENT_VSRC_SHIFT                16

/* Flicker Filter 0 */
#define STBLIT_FF0_N_MINUS_1_COEFF_MASK                0xFF
#define STBLIT_FF0_N_MINUS_1_COEFF_SHIFT               0
#define STBLIT_FF0_N_COEFF_MASK                        0xFF
#define STBLIT_FF0_N_COEFF_SHIFT                       8
#define STBLIT_FF0_N_PLUS_1_COEFF_MASK                 0xFF
#define STBLIT_FF0_N_PLUS_1_COEFF_SHIFT                16
#define STBLIT_FF0_THRESHOLD_MASK                      0xF
#define STBLIT_FF0_THRESHOLD_SHIFT                     24

/* Flicker Filter 1 */
#define STBLIT_FF1_N_MINUS_1_COEFF_MASK                0xFF
#define STBLIT_FF1_N_MINUS_1_COEFF_SHIFT               0
#define STBLIT_FF1_N_COEFF_MASK                        0xFF
#define STBLIT_FF1_N_COEFF_SHIFT                       8
#define STBLIT_FF1_N_PLUS_1_COEFF_MASK                 0xFF
#define STBLIT_FF1_N_PLUS_1_COEFF_SHIFT                16
#define STBLIT_FF1_THRESHOLD_MASK                      0xF
#define STBLIT_FF1_THRESHOLD_SHIFT                     24

/* Flicker Filter 2 */
#define STBLIT_FF2_N_MINUS_1_COEFF_MASK                0xFF
#define STBLIT_FF2_N_MINUS_1_COEFF_SHIFT               0
#define STBLIT_FF2_N_COEFF_MASK                        0xFF
#define STBLIT_FF2_N_COEFF_SHIFT                       8
#define STBLIT_FF2_N_PLUS_1_COEFF_MASK                 0xFF
#define STBLIT_FF2_N_PLUS_1_COEFF_SHIFT                16
#define STBLIT_FF2_THRESHOLD_MASK                      0xF
#define STBLIT_FF2_THRESHOLD_SHIFT                     24

/* Flicker Filter 3 */
#define STBLIT_FF3_N_MINUS_1_COEFF_MASK                0xFF
#define STBLIT_FF3_N_MINUS_1_COEFF_SHIFT               0
#define STBLIT_FF3_N_COEFF_MASK                        0xFF
#define STBLIT_FF3_N_COEFF_SHIFT                       8
#define STBLIT_FF3_N_PLUS_1_COEFF_MASK                 0xFF
#define STBLIT_FF3_N_PLUS_1_COEFF_SHIFT                16

/* ALU and Color Key Control */
#define STBLIT_ACK_ALU_MODE_MASK                       0xF
#define STBLIT_ACK_ALU_MODE_SHIFT                      0
#define STBLIT_ACK_ALU_MODE_BYPASS_SRC1                0
#define STBLIT_ACK_ALU_MODE_LOGICAL                    1
#define STBLIT_ACK_ALU_MODE_BLEND_UNWEIGHT             2
#define STBLIT_ACK_ALU_MODE_BLEND_WEIGHT               3
#define STBLIT_ACK_ALU_MODE_MASK_LOGICAL_FIRST         4
#define STBLIT_ACK_ALU_MODE_MASK_BLEND                 5
#define STBLIT_ACK_ALU_MODE_420_CONCATENATION          6
#define STBLIT_ACK_ALU_MODE_BYPASS_SRC2                7
#define STBLIT_ACK_ALU_MODE_MASK_LOGICAL_SECOND        8
#define STBLIT_ACK_ALU_MODE_MASK_XYL_LOGICAL           9
#define STBLIT_ACK_ALU_MODE_MASK_XYL_BLEND_UNWEIGHT    10
#define STBLIT_ACK_ALU_MODE_MASK_XYL_BLEND_WEIGHT      11

#define STBLIT_ACK_ALU_GLOBAL_ALPHA_MASK               0xFF
#define STBLIT_ACK_ALU_GLOBAL_ALPHA_SHIFT              8

#define STBLIT_ACK_ALU_ROP_MODE_MASK                   0xF
#define STBLIT_ACK_ALU_ROP_MODE_SHIFT                  8


#define STBLIT_ACK_COLOR_KEY_CFG_MASK                  0x3F
#define STBLIT_ACK_COLOR_KEY_CFG_SHIFT                 16
#define STBLIT_ACK_COLOR_KEY_CFG_BLUE_MASK             3
#define STBLIT_ACK_COLOR_KEY_CFG_BLUE_SHIFT            16
#define STBLIT_ACK_COLOR_KEY_CFG_GREEN_MASK            3
#define STBLIT_ACK_COLOR_KEY_CFG_GREEN_SHIFT           18
#define STBLIT_ACK_COLOR_KEY_CFG_RED_MASK              3
#define STBLIT_ACK_COLOR_KEY_CFG_RED_SHIFT             20
#define STBLIT_ACK_COLOR_KEY_IGNORED                   0
#define STBLIT_ACK_COLOR_KEY_INSIDE                    1
#define STBLIT_ACK_COLOR_KEY_OUTSIDE                   3


#define STBLIT_ACK_COLOR_KEY_IN_SELECT_MASK            3
#define STBLIT_ACK_COLOR_KEY_IN_SELECT_SHIFT           22
#define STBLIT_ACK_COLOR_KEY_IN_SELECT_DST             0
#define STBLIT_ACK_COLOR_KEY_IN_SELECT_SRC_BEFORE      1
#define STBLIT_ACK_COLOR_KEY_IN_SELECT_SRC_AFTER       2

/* Color Key 1 */
#define STBLIT_KEY1_BLUE_MASK                          0xFF
#define STBLIT_KEY1_BLUE_SHIFT                         0
#define STBLIT_KEY1_GREEN_MASK                         0xFF
#define STBLIT_KEY1_GREEN_SHIFT                        8
#define STBLIT_KEY1_RED_MASK                           0xFF
#define STBLIT_KEY1_RED_SHIFT                          16

/* Color Key 2 */
#define STBLIT_KEY2_BLUE_MASK                          0xFF
#define STBLIT_KEY2_BLUE_SHIFT                         0
#define STBLIT_KEY2_GREEN_MASK                         0xFF
#define STBLIT_KEY2_GREEN_SHIFT                        8
#define STBLIT_KEY2_RED_MASK                           0xFF
#define STBLIT_KEY2_RED_SHIFT                          16

/* Plane Mask */
#define STBLIT_PMK_VALUE_MASK                          0xFFFFFFFF
#define STBLIT_PMK_VALUE_SHIFT                         0

/* Raster Scan Trigger */
#define STBLIT_RST_VIDEO_LINE_MASK                     0x7FF
#define STBLIT_RST_VIDEO_LINE_SHIFT                    0
#define STBLIT_RST_VTG_MASK                            1
#define STBLIT_RST_VTG_SHIFT                           16

/* XYL */
#define STBLIT_XYL_MODE_MASK                           1
#define STBLIT_XYL_MODE_SHIFT                          0
#define STBLIT_XYL_MODE_STANDARD                       0
#define STBLIT_XYL_MODE_TRANSFORM                      1

#define STBLIT_XYL_BUFFER_FORMAT_MASK                  3
#define STBLIT_XYL_BUFFER_FORMAT_SHIFT                 8

#define STBLIT_XYL_NB_SUBINST_MASK                     0xFFFF
#define STBLIT_XYL_NB_SUBINST_SHIFT                    16

/* XYP */
#define STBLIT_XYP_SUBINST_PTR_MASK                    0xFFFFFFFF
#define STBLIT_XYP_SUBINST_PTR_SHIFT                   0


/* Exported Types ----------------------------------------------------------- */
typedef enum
{
    STBLIT_ARRAY_TYPE_XY    = 0,
    STBLIT_ARRAY_TYPE_XYL   = 1,
    STBLIT_ARRAY_TYPE_XYC   = 2,
    STBLIT_ARRAY_TYPE_XYLC  = 3
} stblit_ArrayType_t;


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
#ifdef STBLIT_LINUX_FULL_USER_VERSION
U32 stblit_CpuToDevice(void* Address);
#endif /* STBLIT_LINUX_FULL_USER_VERSION */
ST_ErrorCode_t stblit_FormatColorForInputFormatter (STGXOBJ_Color_t* ColorIn_p, U32* ValueOut_p);
ST_ErrorCode_t stblit_FormatColorForOutputFormatter (STGXOBJ_Color_t* ColorIn_p, U32* ValueOut_p);
ST_ErrorCode_t stblit_Connect2Nodes(stblit_Device_t* Device_p, stblit_Node_t* FirstNode_p, stblit_Node_t* SecondNode_p);
void stblit_WriteCommonFieldInNode(stblit_Node_t*          Node_p,
                                   stblit_CommonField_t*   CommonField_p);
ST_ErrorCode_t stblit_OnePassBlitOperation( stblit_Device_t*        Device_p,
                                        STBLIT_Source_t*        Src1_p,
                                        STBLIT_Source_t*        Src2_p,
                                        STBLIT_Destination_t*   Dst_p,
                                        STBLIT_BlitContext_t*   BlitContext_p,
                                        U16                     Code,
                                        stblit_MaskMode_t       MaskMode,
                                        stblit_NodeHandle_t*    NodeHandle_p,
                                        BOOL                    SrcMB,
                                        stblit_OperationConfiguration_t*  Config_p,
                                        stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassBlitSliceRectangleOperation(stblit_Device_t*   Device_p,
                                                        STBLIT_SliceData_t*   Src1_p,
                                                        STBLIT_SliceData_t*   Src2_p,
                                                        STBLIT_SliceData_t*   Dst_p,
                                                        STBLIT_SliceRectangle_t*  SliceRectangle_p,
                                                        STBLIT_BlitContext_t*   BlitContext_p,
                                                        U16                     Code,
                                                        stblit_MaskMode_t       MaskMode,
                                                        stblit_NodeHandle_t*    NodeHandle_p,
                                                        stblit_OperationConfiguration_t*  Config_p,
                                                        stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassCopyOperation( stblit_Device_t*      Device_p,
                                            STGXOBJ_Bitmap_t*     SrcBitmap_p,
                                            STGXOBJ_Rectangle_t*  SrcRectangle_p,
                                            STGXOBJ_Bitmap_t*     DstBitmap_p,
                                            S32                   DstPositionX,
                                            S32                   DstPositionY,
                                            STBLIT_BlitContext_t* BlitContext_p,
                                            stblit_NodeHandle_t*  NodeHandle_p,
                                            stblit_OperationConfiguration_t*  Config_p,
                                            stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassFillOperation( stblit_Device_t*      Device_p,
                                            STGXOBJ_Bitmap_t*     Bitmap_p,
                                            STGXOBJ_Rectangle_t*  Rectangle_p,
                                            STGXOBJ_Rectangle_t*  MaskRectangle_p,
                                            STGXOBJ_Color_t*      Color_p,
                                            STBLIT_BlitContext_t* BlitContext_p,
                                            stblit_NodeHandle_t*  NodeHandle_p,
                                            stblit_OperationConfiguration_t*  Config_p,
                                            stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassDrawLineOperation( stblit_Device_t*      Device_p,
                                                STGXOBJ_Bitmap_t*     Bitmap_p,
                                                S32                   PositionX,
                                                S32                   PositionY,
                                                U32                   Length,
                                                BOOL                  HorNotVer,
                                                STGXOBJ_Color_t*      Color_p,
                                                STBLIT_BlitContext_t* BlitContext_p,
                                                stblit_NodeHandle_t*  NodeHandle_p,
                                                stblit_OperationConfiguration_t*  Config_p,
                                                stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassDrawHLineOperation( stblit_Device_t*      Device_p,
                                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                                 S32                   PositionX,
                                                 S32                   PositionY,
                                                 U32                   Length,
                                                 STGXOBJ_Color_t*      Color_p,
                                                 STBLIT_BlitContext_t* BlitContext_p,
                                                 stblit_NodeHandle_t*  NodeHandle_p,
                                                 stblit_OperationConfiguration_t*  Config_p,
                                                 stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassDrawArrayOperation( stblit_Device_t*      Device_p,
                                                 STGXOBJ_Bitmap_t*     Bitmap_p,
                                                 stblit_ArrayType_t    ArrayType,
                                                 void*                 Array_p,
                                                 STGXOBJ_Color_t*      Color_p,
                                                 U32                   NbElem,
                                                 STBLIT_BlitContext_t* BlitContext_p,
                                                 stblit_NodeHandle_t*  NodeHandle_p,
                                                 stblit_OperationConfiguration_t*  Config_p,
                                                 stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_OnePassConcatOperation( stblit_Device_t*      Device_p,
                                              STGXOBJ_Bitmap_t*     SrcAlphaBitmap_p,
                                              STGXOBJ_Rectangle_t*  SrcAlphaRectangle_p,
                                              STGXOBJ_Bitmap_t*     SrcColorBitmap_p,
                                              STGXOBJ_Rectangle_t*  SrcColorRectangle_p,
                                              STGXOBJ_Bitmap_t*     DstBitmap_p,
                                              STGXOBJ_Rectangle_t*  DstRectangle_p,
                                              STBLIT_BlitContext_t* BlitContext_p,
                                              stblit_NodeHandle_t*  NodeHandle_p,
                                              stblit_OperationConfiguration_t*  Config_p,
                                              stblit_CommonField_t*             CommonField_p);

ST_ErrorCode_t stblit_UpdateDestinationBitmap(stblit_Device_t*      Device_p,
                                              stblit_JobBlit_t*     JBlit_p,
                                              STGXOBJ_Bitmap_t*     Bitmap_p);

ST_ErrorCode_t stblit_UpdateBackgroundBitmap(stblit_Device_t*      Device_p,
                                              stblit_JobBlit_t*     JBlit_p,
                                              STGXOBJ_Bitmap_t*     Bitmap_p);

ST_ErrorCode_t stblit_UpdateForegroundBitmap(stblit_Device_t*      Device_p,
                                              stblit_JobBlit_t*     JBlit_p,
                                              STGXOBJ_Bitmap_t*     Bitmap_p);

ST_ErrorCode_t stblit_UpdateForegroundColor(stblit_JobBlit_t*     JBlit_p,
                                            STGXOBJ_Color_t*      Color_p);

ST_ErrorCode_t stblit_UpdateMaskBitmap(stblit_Device_t*      Device_p,
                                       stblit_JobBlit_t*     JBlit_p,
                                       STGXOBJ_Bitmap_t*     Bitmap_p);

ST_ErrorCode_t stblit_UpdateDestinationRectangle(stblit_JobBlit_t*     JBlit_p,
                                                 STGXOBJ_Rectangle_t*  Rectangle_p);

ST_ErrorCode_t stblit_UpdateBackgroundRectangle(stblit_JobBlit_t*     JBlit_p,
                                                STGXOBJ_Rectangle_t*  Rectangle_p);

ST_ErrorCode_t stblit_UpdateForegroundRectangle(stblit_JobBlit_t*     JBlit_p,
                                                STGXOBJ_Rectangle_t*  Rectangle_p);

ST_ErrorCode_t stblit_UpdateClipRectangle(stblit_JobBlit_t*     JBlit_p,
                                          BOOL                  WriteInsideClipRectangle,
                                          STGXOBJ_Rectangle_t*  ClipRectangle_p);

ST_ErrorCode_t stblit_UpdateMaskWord(stblit_JobBlit_t*     JBlit_p,
                                     U32                   MaskWord);

ST_ErrorCode_t stblit_UpdateColorKey(stblit_JobBlit_t*         JBlit_p,
                                     STBLIT_ColorKeyCopyMode_t ColorKeyCopyMode,
                                     STGXOBJ_ColorKey_t*       ColorKey_p);

#ifdef WA_WIDTH_8176_BYTES_LIMITATION
ST_ErrorCode_t stblit_WA_CheckWidthByteLimitation(STGXOBJ_ColorType_t ColorType, U32 Width);
#endif

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __BLT_MEM_H  */

/* End of blt_mem.h */
