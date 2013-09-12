/*******************************************************************************
File name   : sink.h

Description : HDMI driver header file for SINK (receiver) block.

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/

#ifndef __SINK_H
#define __SINK_H

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif        /* ST_OSLINUX */

#include "hdmi_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */
#define MAX =10

/* ------------------------------------------
 * ---Id Manufacter Character Code ----------
 * -----------------------------------------*/
#define STHDMI_ID_MANUFACTER_CHAR_A_CODE    0x01
#define STHDMI_ID_MANUFACTER_CHAR_B_CODE    0x02
#define STHDMI_ID_MANUFACTER_CHAR_C_CODE    0x03
#define STHDMI_ID_MANUFACTER_CHAR_D_CODE    0x04
#define STHDMI_ID_MANUFACTER_CHAR_E_CODE    0x05
#define STHDMI_ID_MANUFACTER_CHAR_F_CODE    0x06
#define STHDMI_ID_MANUFACTER_CHAR_G_CODE    0x07
#define STHDMI_ID_MANUFACTER_CHAR_H_CODE    0x08
#define STHDMI_ID_MANUFACTER_CHAR_I_CODE    0x09
#define STHDMI_ID_MANUFACTER_CHAR_J_CODE    0x0A
#define STHDMI_ID_MANUFACTER_CHAR_K_CODE    0x0B
#define STHDMI_ID_MANUFACTER_CHAR_L_CODE    0x0C
#define STHDMI_ID_MANUFACTER_CHAR_M_CODE    0x0D
#define STHDMI_ID_MANUFACTER_CHAR_N_CODE    0x0E
#define STHDMI_ID_MANUFACTER_CHAR_O_CODE    0x0F
#define STHDMI_ID_MANUFACTER_CHAR_P_CODE    0x10
#define STHDMI_ID_MANUFACTER_CHAR_Q_CODE    0x11
#define STHDMI_ID_MANUFACTER_CHAR_R_CODE    0x12
#define STHDMI_ID_MANUFACTER_CHAR_S_CODE    0x13
#define STHDMI_ID_MANUFACTER_CHAR_T_CODE    0x14
#define STHDMI_ID_MANUFACTER_CHAR_U_CODE    0x15
#define STHDMI_ID_MANUFACTER_CHAR_V_CODE    0x16
#define STHDMI_ID_MANUFACTER_CHAR_W_CODE    0x17
#define STHDMI_ID_MANUFACTER_CHAR_X_CODE    0x18
#define STHDMI_ID_MANUFACTER_CHAR_Y_CODE    0x19
#define STHDMI_ID_MANUFACTER_CHAR_Z_CODE    0x1A

/*-----------------------------------------------
 * -------------Id Serial Number----------------
 * ----------------------------------------------*/
#define STHDMI_ID_SER_NUM_BYTE1_MASK1      0xF0000000
#define STHDMI_ID_SER_NUM_BYTE1_MASK2      0x0F000000
#define STHDMI_ID_SER_NUM_BYTE2_MASK1      0x00F00000
#define STHDMI_ID_SER_NUM_BYTE2_MASK2      0x000F0000
#define STHDMI_ID_SER_NUM_BYTE3_MASK1      0x0000F000
#define STHDMI_ID_SER_NUM_BYTE3_MASK2      0x00000F00
#define STHDMI_ID_SER_NUM_BYTE4_MASK1      0x000000F0
#define STHDMI_ID_SER_NUM_BYTE4_MASK2      0x0000000F

/* ------------------------------------------------
 * ------------Basic Display Parameters------------
 * ------------------------------------------------ */
#define STHDMI_DIGITAL_SIGNAL_LEVEL       0x80

#define STHDMI_SIGNAL_LEVEL_STANDARD_OP1  0x00
#define STHDMI_SIGNAL_LEVEL_STANDARD_OP2  0x20
#define STHDMI_SIGNAL_LEVEL_STANDARD_OP3  0x40
#define STHDMI_SIGNAL_LEVEL_STANDARD_OP4  0x60

#define STHDMI_VIDEO_BLANKTOBLACK_SETUP   0x10
#define STHDMI_VIDEO_SEPERATE_SYNC        0x08
#define STHDMI_VIDEO_COMPOSITE_SYNC       0x04
#define STHDMI_VIDEO_SYNC_ON_GREEN        0x02
#define STHDMI_VIDEO_SERRATION_OF_VSYNC   0x01
#define STHDMI_VIDEO_DFP_COMPATBLE        0x01

#define STHDMI_FEATURE_STANDBY_ENABLED    0x80
#define STHDMI_FEATURE_SUSPEND_ENABLED    0x40
#define STHDMI_FEATURE_ACTIVE_OFF         0x20
#define STHDMI_FEATURE_DISPLAY_TYPE_1     0x00
#define STHDMI_FEATURE_DISPLAY_TYPE_2     0x08
#define STHDMI_FEATURE_DISPLAY_TYPE_3     0x10
#define STHDMI_FEATURE_DISPLAY_TYPE_4     0x18
#define STHDMI_FEATURE_RGB_STD_DEFAULT    0x04
#define STHDMI_FEATURE_PREFERRED_MODE     0x02
#define STHDMI_FEATURE_GTF_SUPPORTED      0x01

/* --------------------------------------------------------------
 * --Color Characteristic: Chromaticity and default white point
 * -------------------------------------------------------------*/
#define STHDMI_COLOR_RED_X_LOW_BITS       0xC0
#define STHDMI_COLOR_RED_Y_LOW_BITS       0x30
#define STHDMI_COLOR_GREEN_X_LOW_BITS     0x0C
#define STHDMI_COLOR_GREEN_Y_LOW_BITS     0x03
#define STHDMI_COLOR_BLUE_X_LOW_BITS      0xC0
#define STHDMI_COLOR_BLUE_Y_LOW_BITS      0x30
#define STHDMI_COLOR_WHITE_X_LOW_BITS     0x0C
#define STHDMI_COLOR_WHITE_Y_LOW_BITS     0x03

#define STHDMI_COLOR_CHARACTERISTIC_BIT0  0x01
#define STHDMI_COLOR_CHARACTERISTIC_BIT1  0x02
#define STHDMI_COLOR_CHARACTERISTIC_BIT2  0x04
#define STHDMI_COLOR_CHARACTERISTIC_BIT3  0x08
#define STHDMI_COLOR_CHARACTERISTIC_BIT4  0x10
#define STHDMI_COLOR_CHARACTERISTIC_BIT5  0x20
#define STHDMI_COLOR_CHARACTERISTIC_BIT6  0x40
#define STHDMI_COLOR_CHARACTERISTIC_BIT7  0x80
#define STHDMI_COLOR_CHARACTERISTIC_BIT8  0x100
#define STHDMI_COLOR_CHARACTERISTIC_BIT9  0x200

/* --------------------------------------------
 * ---------Established Timing-----------------
 * -------------------------------------------- */

#define STHDMI_ESTABLISHED_TIMING_NONE   0x00
#define STHDMI_ESTABLISHED_TIMING_BIT_1  0x01
#define STHDMI_ESTABLISHED_TIMING_BIT_2  0x02
#define STHDMI_ESTABLISHED_TIMING_BIT_3  0x04
#define STHDMI_ESTABLISHED_TIMING_BIT_4  0x08
#define STHDMI_ESTABLISHED_TIMING_BIT_5  0x10
#define STHDMI_ESTABLISHED_TIMING_BIT_6  0x20
#define STHDMI_ESTABLISHED_TIMING_BIT_7  0x40
#define STHDMI_ESTABLISHED_TIMING_BIT_8  0x80

/* ----------------------------------------------
 * --------Standard Timing Identification--------
 * ---------------------------------------------- */

#define STHDMI_TIMING_ID_ASPECT_RATIO_16_10 0x00
#define STHDMI_TIMING_ID_ASPECT_RATIO_4_3   0x40
#define STHDMI_TIMING_ID_ASPECT_RATIO_5_4   0x80
#define STHDMI_TIMING_ID_ASPECT_RATIO_16_9  0xC0
#define STHDMI_TIMING_REFRESF_RATE_MASK     0x3F

/* ---------------------------------------------
 * -------Detailed Timing Descriptor------------
 * --------------------------------------------- */
#define STHDMI_TIMING_LOW_BITS_MASK           0x0F
#define STHDMI_TIMING_HIGH_BITS_MASK          0xF0

#define STHDMI_TIMING_BITS67_MASK             0xC0
#define STHDMI_TIMING_BITS45_MASK             0x30
#define STHDMI_TIMING_BITS23_MASK             0x0C
#define STHDMI_TIMING_BITS01_MASK             0x03

#define STHDMI_FLAG_DECODE_STEREO_BIT0_MASK   0x01
#define STHDMI_FLAG_DECODE_STEREO_BIT56_MASK  0x60
#define STHDMI_TIMING_DESC_FLAG_INTERLACED    0x80
#define STHDMI_TIMING_DESC_FLAG_ANA_COMP      0x00
#define STHDMI_TIMING_DESC_FLAG_BIP_ANA       0x08
#define STHDMI_TIMING_DESC_FLAG_DIG_COMP      0x10
#define STHDMI_TIMING_DESC_FLAG_DIG_SEP       0x18

#define STHDMI_TIMING_STEREO_RIGHT_IMAGE      0x02
#define STHDMI_TIMING_STEREO_LEFT_IMAGE       0x04
#define STHDMI_TIMING_STEREO_2W_RIGHT_IMAGE   0x03
#define STHDMI_TIMING_STEREO_2W_LEFT_IMAGE    0x05
#define STHDMI_TIMING_STEREO_4W               0x06
#define STHDMI_TIMING_STEREO_SIDE_BY_SIDE     0x07

#define STHDMI_FLAG_BIT1_MASK                 0x02
#define STHDMI_FLAG_BIT2_MASK                 0x04

/* -----------------------------------------------
 * ----------CEA EDID Timing Extension------------
 * ----------------------------------------------*/
#define STHDMI_EDID_SINK_UNDERSCAN            0x80
#define STHDMI_EDID_SINK_BASIC_AUDIO          0x40
#define STHDMI_EDID_SINK_YCBCR444             0x20
#define STHDMI_EDID_SINK_YCBCR422             0x10
#define STHDMI_EDID_SINK_NATIVE_MASK          0x0F

#define STHDMI_EDID_DATA_BLOCK_TAG_CODE       0xE0
#define STHDMI_EDID_DATA_BLOCK_LENGTH         0x1F

/* -----------------------------------------------
* ----------CEA Short Video Descriptor-----------
* ----------------------------------------------*/
#define STHDMI_CEA_SHORT_VIDEO_NATIVE         0x80
#define STHDMI_CEA_SHORT_VIDEO_CODE_MASK      0x7F

/* -----------------------------------------------
* ----------CEA Short Audio Descriptor-----------
* ----------------------------------------------*/
#define STHDMI_AUDIO_FORMAT_CODE_MASK         0x78
#define STHDMI_AUDIO_MAX_NUM_CHANNEL_MASK     0x07
#define STHDMI_AUDIO_FORMAT_CODE_PCM          0x08    /* Linear PCM (e.g: IEC60958)*/
#define STHDMI_AUDIO_FORMAT_CODE_AC3          0x10    /* AC-3*/
#define STHDMI_AUDIO_FORMAT_CODE_MPEG1        0x18    /* MPEG1 (Layers 1&2) */
#define STHDMI_AUDIO_FORMAT_CODE_MP3          0x20    /* MP3 (MPEG1 Layer3) */
#define STHDMI_AUDIO_FORMAT_CODE_MPEG2        0x28    /* MPEG2 (multichannel) */
#define STHDMI_AUDIO_FORMAT_CODE_AAC          0x30    /* AAC */
#define STHDMI_AUDIO_FORMAT_CODE_DTS          0x38    /* DTS */
#define STHDMI_AUDIO_FORMAT_CODE_ATRAC        0x40    /* ATRAC */

/* ----------------------------------------------
 * --------- Monitor Descriptor Block------------
 * ---------------------------------------------- */

#define STHDMI_MONITOR_DESCRIPTOR_TAG1        0xFF    /* Monitor serial Number-stored as ASCII*/
#define STHDMI_MONITOR_DESCRIPTOR_TAG2        0xFE    /* ASCII String stored as ASCII */
#define STHDMI_MONITOR_DESCRIPTOR_TAG3        0xFD    /* Monitor Range limits, binary coded */
#define STHDMI_MONITOR_DESCRIPTOR_TAG4        0xFC    /* Monitor name, stored as ASCII */
#define STHDMI_MONITOR_DESCRIPTOR_TAG5        0xFB    /* Descriptor contains additional color point data */
#define STHDMI_MONITOR_DESCRIPTOR_TAG6        0xFA    /* Descriptor contains additional standard timing identification */
#define STHDMI_MONITOR_DESCRIPTOR_TAG7        0x10    /* Dummy descriptor, used to indicate that the descriptor space is unused */
                                                      /* tag : 0x11->0xF9  Currently undefined*/
                                                      /* 0x00-> 0x0F descriptor defined bu manufacturer*/
#define STHDMI_SECONDRY_TIMING_SUPPORTED      0x02    /* Secondary Timing Formula supported */
#define STHDMI_NO_COLOR_POINT                 0x00    /* No color point data follows */



/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */


/* Private Macros --------------------------------------------------------- */

/* Exported Macros--------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* #ifndef __SINK_H */


