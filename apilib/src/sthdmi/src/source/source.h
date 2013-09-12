/*******************************************************************************
File name   : source.h

Description : HDMI driver header file for SOURCE block

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/

#ifndef __SOURCE_H
#define __SOURCE_H

/* Includes ----------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif        /* ST_OSLINUX */

#include "hdmi_drv.h"
#include "stcommon.h"
#include "stddefs.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */
#define MAX =10

/* Exported Constants ----------------------------------------------------- */

/* --------------------------------
 * AVI infoframe Version 1 and 2--
 * ------------------------------*/

/* -----AVI InfoFrame Data Byte 1---- */
#define STHDMI_AVI_DATABYTE1_MASK       0x7F

#define STHDMI_SOURCE_RGB888_OUTPUT     0x00  /* For RGB (default output) */
#define STHDMI_SOURCE_YCBCR422_OUTPUT   0x20  /* For YCbCr 4:2:2 output*/
#define STHDMI_SOURCE_YCBCR444_OUTPUT   0x40  /* For YCbCr 4:4:4 output*/
#define STHDMI_SOURCE_FUTUR_OUTPUT      0x60  /* Value Reserved for Futur use */


#define STHDMI_BAR_INFO_NOT_VALID       0x00  /* Bar Data not valid */
#define STHDMI_V_BAR_INFO_VALID         0x04  /* Vertical Bar info valid */
#define STHDMI_H_BAR_INFO_VALID         0x08  /* Horizontal Bar Info valid */
#define STHDMI_VH_BAR_INFO_VALID        0x0C  /* Vertical and Horizontal Bar Info Valid */

#define STHDMI_SCAN_INFO_NO_DATA        0x00  /* Scan information no data */
#define STHDMI_SCAN_INFO_OVERSCANNED    0x01  /* Scan information Overscanned */
#define STHDMI_SCAN_INFO_UNDERSCANNED   0x02  /* Scan information underscanned*/
#define STHDMI_SCAN_INFO_FUTURE_USE     0x03  /* Scan information future USE*/

#define STHDMI_ACTIVE_FORMAT_VALID      0x10  /* Active Format Information Valid */


/* -----AVI InfoFrame Data Byte 2---- */
#define STHDMI_AVI_DATABYTE2_MASK       0xFF

#define STHDMI_COLORIMETRY_NO_DATA      0x00  /* No Data */
#define STHDMI_COLORIMETRY_ITU601       0x40  /* SMPTE 170M ITU601 */
#define STHDMI_COLORIMETRY_ITU709       0x80  /* ITU709 */
#define STHDMI_COLORIMETRY_FUTURE       0xD0  /* Rserved for Future use */

#define STHDMI_PIC_ASPECTRATIO_NO_DATA  0x00  /* No data */
#define STHDMI_PIC_ASPECTRATIO_43       0x10  /* Picture aspect ratio: 4:3  */
#define STHDMI_PIC_ASPECTRATIO_169      0x20  /* Picture aspect ratio: 16:9 */
#define STHDMI_PIC_ASPECTRATIO_FUTURE   0x30  /* Reserved for Future Use */

#define STHDMI_FORMAT_SAME_AS_PICTURE   0x08 /* Active format Same as picture aspect ratio */
#define STHDMI_FORMAT_ASPECTRATIO_43    0x09 /* Active Format 4:3 (center) */
#define STHDMI_FORMAT_ASPECTRATIO_169   0x0A /* Active Format 16:9 (center) */
#define STHDMI_FORMAT_ASPECTRATIO_149   0x0B /* Active Format 14:9 (center) */


/* -----AVI InfoFrame Data Byte 3---- */
#define STHDMI_AVI_DATABYTE3_MASK      0x03

#define STHDMI_NO_KNOWN_PIC_SCALING    0x00  /* No Known Non-uniform picture scaling */
#define STHDMI_H_PIC_SCALING           0x01  /* Picture has been scaled horizontally */
#define STHDMI_V_PIC_SCALING           0x02  /* Picture has been scaled vertically */
#define STHDMI_H_V_PIC_SCALING         0x03  /* Picture has been scaled horizontally and vertically */

/*--------------------------------
 * AVI info frame Data Byte 4&5 --
 * -------------------------------*/
#define STHDMI_AVI_VER1_DATABYTE4_MASK 0x00   /* This mask is for AVI info frame version1 */
#define STHDMI_AVI_VER2_DATABYTE4_MASK 0x7F   /* This mask is for AVI info frame version2 */
#define STHDMI_AVI_VER1_DATABYTE5_MASK 0x00
#define STHDMI_AVI_VER2_DATABYTE5_MASK 0x0F
#define STHDMI_RESERVED_FOR_FUTURE_USE 0x00  /* Reserved for Future use sall be set at0*/

/*--------------------------------
 * AVI info frame Data Byte 6=>13 -
 * -------------------------------*/
#define STHDMI_AVI_DATABYTE_MASK       0xFF

/*---------------------------------
 * --SPD info Frame Version 1------
 * --------------------------------*/
#define STHDMI_SPD_INFOFRAME_MASK      0x7F /* The vendor Name and Product description should be coded in 7-bits ASCII */
#define STHDMI_SPD_INFOFRAME_NULL      0x00 /*All unused characters should be NULL*/
#define STHDMI_DEVICE_UNKNOWN          0x00 /* The Source Device is Unknown */
#define STHDMI_DEVICE_DIGITAL_STB      0x01 /* The Source Device is Digital STB */
#define STHDMI_DEVICE_DVD              0x02 /* The Source Device is DVD */
#define STHDMI_DEVICE_D_VHS            0x03 /* The Source Device is D-VHS */
#define STHDMI_DEVICE_HDD_VIDEO        0x04 /* The Source Device is HDD Video */
#define STHDMI_DEVICE_DVC              0x05 /* The Source Device is DVC */
#define STHDMI_DEVICE_DSC              0x06 /* The Source Device is DSC */
#define STHDMI_DEVICE_VIDEO_CD         0x07 /* The Source Device is Video CD */
#define STHDMI_DEVICE_GAME             0x08 /* The Source Device is a Game */
#define STHDMI_DEVICE_PC_GENERAL       0x09 /* The Source Device is a PC General*/

/*----------------------------------
 * ----MPEG Source Info Frame ver1--
 * ---------------------------------*/
#define STHDMI_MS_INFOFRAME_MASK       0x13
#define STHDMI_MS_INFOFRAME_NULL       0x00
#define STHDMI_MPEG_FRAME_UNKNOWN      0x00
#define STHDMI_MPEG_FRAME_I            0x01
#define STHDMI_MPEG_FRAME_B            0x02
#define STHDMI_MPEG_FRAME_P            0x03
#define STHDMI_MPEG_REP_FIELD          0x10 /* Field repeat bit is set */


/*------------------------------------
 * ----Audio Coding Type Ver1---------
 * ----------------------------------*/
#define STHDMI_AUDIO_INFOFRAME_MASK   0xFF
/* ------Audio Info Frame Data Byte 1-------- */
#define STHDMI_AUDIO_DATABYTE1_MASK    0xF7
#define STHDMI_AUDIO_NO_REF_CHANNEL    0x00        /* Refer to Stream Header*/
#define STHDMI_AUDIO_2_CHANNEL         0x01        /* 2 Audio Channel Count */
#define STHDMI_AUDIO_3_CHANNEL         0x02        /* 3 Audio Channel Count */
#define STHDMI_AUDIO_4_CHANNEL         0x03        /* 4 Audio Channel Count */
#define STHDMI_AUDIO_5_CHANNEL         0x04        /* 5 Audio Channel Count */
#define STHDMI_AUDIO_6_CHANNEL         0x05        /* 6 Audio Channel Count */
#define STHDMI_AUDIO_7_CHANNEL         0x06        /* 7 Audio Channel Count */
#define STHDMI_AUDIO_8_CHANNEL         0x07        /* 8 Audio Channel Count */


#define STHDMI_AUDIO_CODING_NO_REF     0x00       /* Ref to stream header */
#define STHDMI_AUDIO_CODING_PCM        0x10       /* IEC60958 PCM [26,27] */
#define STHDMI_AUDIO_CODING_AC3        0x20       /* AC3 */
#define STHDMI_AUDIO_CODING_MPEG1      0x30       /* MPEG1 */
#define STHDMI_AUDIO_CODING_MP3        0x40       /* MP3 */
#define STHDMI_AUDIO_CODING_MPEG2      0x50       /* MPEG2 */
#define STHDMI_AUDIO_CODING_AAC        0x60       /* AAC */
#define STHDMI_AUDIO_CODING_DTS        0x70       /* DTS */
#define STHDMI_AUDIO_CODING_ATRAC      0x80       /* ATRAC */

/* ------Audio Info Frame Data Byte 2-------- */
#define STHDMI_AUDIO_DATABYTE2_MASK    0x1F
#define STHDMI_AUDIO_SS_NO_REF         0x00       /* Refer to stream header */
#define STHDMI_AUDIO_SS_16_BIT         0x01       /* Sampling Size 16 Bits */
#define STHDMI_AUDIO_SS_20_BIT         0x02       /* Sampling Size 20 Bits */
#define STHDMI_AUDIO_SS_24_BIT         0x03       /* Sampling Size 24 Bits */

#define STHDMI_AUDIO_SF_NO_REF         0x00       /* Refer to stream Header */
#define STHDMI_AUDIO_SF_32KHZ          0x04       /* Sampling Frequency 32 KHz */
#define STHDMI_AUDIO_SF_44KHZ          0x08       /* Sampling Frequency 44.1KHz */
#define STHDMI_AUDIO_SF_48KHZ          0x0C       /* Sampling Frequency 48 KHz */
#define STHDMI_AUDIO_SF_88KHZ          0x10       /* Sampling Frequency 88.2 KHz */
#define STHDMI_AUDIO_SF_96KHZ          0x14       /* Sampling Frequency 96 KHz */
#define STHDMI_AUDIO_SF_176KHZ         0x18       /* Sampling Frequency 176.4 KHz */
#define STHDMI_AUDIO_SF_192KHZ         0x1C       /* Sampling Frequency 192 KHz */

/* ------Audio Info Frame Data Byte 3-------- */
#define STHDMI_AUDIO_DATABYTE3_NULL    0x00       /* This data byte is not used and shall be considered reserved (all :0s) */
                                                  /* only for LPCM (uncompressed audio) */
/* ------Audio Info Frame Data Byte 4-------- */
/* ------------------------------------
 * ---Channel Allocation Configuration
 * -----------------------------------*/
#define STHDMI_CHANNEL_ALLOCATION_CONF0   0x00  /* configuration channel allocation 0-31 */
#define STHDMI_CHANNEL_ALLOCATION_CONF1   0x01
#define STHDMI_CHANNEL_ALLOCATION_CONF2   0x02
#define STHDMI_CHANNEL_ALLOCATION_CONF3   0x03
#define STHDMI_CHANNEL_ALLOCATION_CONF4   0x04
#define STHDMI_CHANNEL_ALLOCATION_CONF5   0x05
#define STHDMI_CHANNEL_ALLOCATION_CONF6   0x06
#define STHDMI_CHANNEL_ALLOCATION_CONF7   0x07
#define STHDMI_CHANNEL_ALLOCATION_CONF8   0x08
#define STHDMI_CHANNEL_ALLOCATION_CONF9   0x09
#define STHDMI_CHANNEL_ALLOCATION_CONF10  0x0A
#define STHDMI_CHANNEL_ALLOCATION_CONF11  0x0B
#define STHDMI_CHANNEL_ALLOCATION_CONF12  0x0C
#define STHDMI_CHANNEL_ALLOCATION_CONF13  0x0D
#define STHDMI_CHANNEL_ALLOCATION_CONF14  0x0E
#define STHDMI_CHANNEL_ALLOCATION_CONF15  0x0F
#define STHDMI_CHANNEL_ALLOCATION_CONF16  0x10
#define STHDMI_CHANNEL_ALLOCATION_CONF17  0x11
#define STHDMI_CHANNEL_ALLOCATION_CONF18  0x12
#define STHDMI_CHANNEL_ALLOCATION_CONF19  0x13
#define STHDMI_CHANNEL_ALLOCATION_CONF20  0x14
#define STHDMI_CHANNEL_ALLOCATION_CONF21  0x15
#define STHDMI_CHANNEL_ALLOCATION_CONF22  0x16
#define STHDMI_CHANNEL_ALLOCATION_CONF23  0x17
#define STHDMI_CHANNEL_ALLOCATION_CONF24  0x18
#define STHDMI_CHANNEL_ALLOCATION_CONF25  0x19
#define STHDMI_CHANNEL_ALLOCATION_CONF26  0x1A
#define STHDMI_CHANNEL_ALLOCATION_CONF27  0x1B
#define STHDMI_CHANNEL_ALLOCATION_CONF28  0x1C
#define STHDMI_CHANNEL_ALLOCATION_CONF29  0x1D
#define STHDMI_CHANNEL_ALLOCATION_CONF30  0x1E
#define STHDMI_CHANNEL_ALLOCATION_CONF31  0x1F

/* ------Audio Info Frame Data Byte 5-------- */
#define STHDMI_AUDIO_DATABYTE5_MASK       0xF8
#define STHDMI_AUDIO_LEVEL_SHIFT_0_DB     0x00     /* Level Shift 0 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_1_DB     0x08     /* Level Shift 1 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_2_DB     0x10     /* Level Shift 2 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_3_DB     0x18     /* Level Shift 3 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_4_DB     0x20     /* Level Shift 4 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_5_DB     0x28     /* Level Shift 5 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_6_DB     0x30     /* Level Shift 6 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_7_DB     0x38     /* Level Shift 7 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_8_DB     0x40     /* Level Shift 8 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_9_DB     0x48     /* Level Shift 9 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_10_DB    0x50     /* Level Shift 10 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_11_DB    0x58     /* Level Shift 11 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_12_DB    0x60     /* Level Shift 12 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_13_DB    0x68     /* Level Shift 13 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_14_DB    0x70     /* Level Shift 14 db*/
#define STHDMI_AUDIO_LEVEL_SHIFT_15_DB    0x78     /* Level Shift 15 db*/

#define STHDMI_DOWN_MIX_PROHIBETED        0x80     /* The Down Mixed stereo output is not permitted */

/* ------Audio Info Frame Data Bytes 6-10-------- */
#define STHDMI_DATA_BYTE_RESERVED         0x00     /* data Byte reserved (shall be 0)*/

/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */

/* Exported Macros--------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

/* ------------------------------- End of file ---------------------------- */
#ifdef __cplusplus
}
#endif
#endif /* #ifndef __SOURCE_H */


