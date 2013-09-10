/************************************************************************
File Name   : sthdmi.h

Description : Exported types and functions for 'HDMI' driver

COPYRIGHT (C) STMicroelectronics 2005

Date               Modification                                     Name
----               ------------                                     ----
04 Feb 2005        Created                                          AC
***********************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STHDMI_H
#define __STHDMI_H


/* Includes --------------------------------------------------------------- */
#include "stddefs.h"
#include "stvid.h"
#include "stvtg.h"
#include "stgxobj.h"
#include "stvout.h"
#ifdef ST_7710
#include "staud.h"
#else
#include "staudlx.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ----------------------------------------------------- */
#define STHDMI_DRIVER_ID      7
#define STHDMI_DRIVER_BASE    (STHDMI_DRIVER_ID << 16)


enum
{
   STHDMI_CEC_COMMAND_EVT = STHDMI_DRIVER_BASE
};

enum
{
    STHDMI_ERROR_NOT_AVAILABLE = STHDMI_DRIVER_BASE,
    STHDMI_ERROR_INVALID_PACKET_FORMAT,
    STHDMI_ERROR_INFOMATION_NOT_AVAILABLE,
    STHDMI_ERROR_VIDEO_UNKOWN,
    STHDMI_ERROR_VTG_UNKOWN,
    STHDMI_ERROR_VMIX_UNKOWN,
    STHDMI_ERROR_AUDIO_UNKOWN,
    STHDMI_ERROR_VOUT_UNKOWN,
    STHDMI_ERROR_EVT_UNKNOWN
};

typedef enum STHDMI_DeviceType_e
{
    STHDMI_DEVICE_TYPE_7710,
    STHDMI_DEVICE_TYPE_7100,
    STHDMI_DEVICE_TYPE_7200
} STHDMI_DeviceType_t;

typedef enum STHDMI_InfoFrameType_e
{
    STHDMI_INFOFRAME_TYPE_VENDOR_SPEC  = 1,
    STHDMI_INFOFRAME_TYPE_AVI          = 2,
    STHDMI_INFOFRAME_TYPE_SPD          = 3,
    STHDMI_INFOFRAME_TYPE_AUDIO        = 4,
    STHDMI_INFOFRAME_TYPE_MPEG_SOURCE  = 5

}STHDMI_InfoFrameType_t;

typedef enum STHDMI_AVIInfoFrameFormat_e
{
   STHDMI_AVI_INFOFRAME_FORMAT_NONE =1,
   STHDMI_AVI_INFOFRAME_FORMAT_VER_ONE=2,
   STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO=4
}STHDMI_AVIInfoFrameFormat_t;

typedef enum STHDMI_SPDInfoFrameFormat_e
{
   STHDMI_SPD_INFOFRAME_FORMAT_NONE   =1,
   STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE=2
}STHDMI_SPDInfoFrameFormat_t;

typedef enum STHDMI_MSInfoFrameFormat_e
{
    STHDMI_MS_INFOFRAME_FORMAT_NONE   =1,
    STHDMI_MS_INFOFRAME_FORMAT_VER_ONE=2
}STHDMI_MSInfoFrameFormat_t;

typedef enum STHDMI_AUDIOInfoFrameFormat_e
{
    STHDMI_AUDIO_INFOFRAME_FORMAT_NONE    =1,
    STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE =2
}STHDMI_AUDIOInfoFrameFormat_t;

typedef enum STHDMI_VSInfoFrameFormat_e
{
    STHDMI_VS_INFOFRAME_FORMAT_NONE    =1,
    STHDMI_VS_INFOFRAME_FORMAT_VER_ONE =2
}STHDMI_VSInfoFrameFormat_t;

typedef enum STHDMI_BarInfo_e
{
   STHDMI_BAR_INFO_NOT_VALID,      /* Bar Data not valid */
   STHDMI_BAR_INFO_V,              /* Vertical bar data valid */
   STHDMI_BAR_INFO_H,              /* Horizental bar data valid */
   STHDMI_BAR_INFO_VH             /* Horizental and Vertical bar data valid */
}STHDMI_BarInfo_t;

typedef enum STHDMI_ScanInfo_e
{
    STHDMI_SCAN_INFO_NO_DATA,       /* No Scan information*/
    STHDMI_SCAN_INFO_OVERSCANNED,   /* Scan information, Overscanned (for television) */
    STHDMI_SCAN_INFO_UNDERSCANNED   /* Scan information, Underscanned (for computer) */
}STHDMI_ScanInfo_t;

typedef enum STHDMI_PictureScaling_e
{
    STHDMI_PICTURE_NON_UNIFORM_SCALING,      /* No Known, non-uniform picture scaling  */
    STHDMI_PICTURE_SCALING_H,                /* Picture has been scaled horizentally */
    STHDMI_PICTURE_SCALING_V,                /* Picture has been scaled Vertically */
    STHDMI_PICTURE_SCALING_HV                /* Picture has been scaled Horizentally and Vertically   */
 }STHDMI_PictureScaling_t;

typedef enum STHDMI_SPDDeviceInfo_e
{
    STHDMI_SPD_DEVICE_UNKNOWN,
    STHDMI_SPD_DEVICE_DIGITAL_STB,
    STHDMI_SPD_DEVICE_DVD,
    STHDMI_SPD_DEVICE_D_VHS,
    STHDMI_SPD_DEVICE_HDD_VIDEO,
    STHDMI_SPD_DEVICE_DVC,
    STHDMI_SPD_DEVICE_DSC,
    STHDMI_SPD_DEVICE_VIDEO_CD,
    STHDMI_SPD_DEVICE_GAME,
    STHDMI_SPD_DEVICE_PC_GENERAL
}STHDMI_SPDDeviceInfo_t;

typedef enum STHDMI_EDIDDisplayType_e
{
    STHDMI_EDID_DISPLAY_TYPE_MONOCHROME,
    STHDMI_EDID_DISPLAY_TYPE_RGB,
    STHDMI_EDID_DISPLAY_TYPE_NON_RGB,
    STHDMI_EDID_DISPLAY_TYPE_UNDEFINED
}STHDMI_EDIDDisplayType_t;

typedef enum STHDMI_EDIDEstablishedTimingI_e
{
    STHDMI_EDID_TIMING_720_400_70 =1,         /* 720*400@70Hz, IBM VGA */
    STHDMI_EDID_TIMING_720_400_88 =2,         /* 720*400@88Hz, IBM XGA2 */
    STHDMI_EDID_TIMING_640_480_60 =4,         /* 640*480@60Hz, IBM VGA */
    STHDMI_EDID_TIMING_640_480_67 =8,         /* 640*480@67Hz, Apple, MacII */
    STHDMI_EDID_TIMING_640_480_72 =16,        /* 640*480@72Hz, VESA */
    STHDMI_EDID_TIMING_640_480_75 =32,        /* 640*480@75Hz, VESA */
    STHDMI_EDID_TIMING_800_600_56 =64,        /* 800*600@56Hz, VESA */
    STHDMI_EDID_TIMING_800_600_60 =128       /* 800*600@60Hz, VESA */
}STHDMI_EDIDEstablishedTimingI_t;

typedef enum STHDMI_EDIDEstablishedTimingII_e
{
    STHDMI_EDID_TIMING_800_600_72 =1,         /* 800*600@72Hz, VESA */
    STHDMI_EDID_TIMING_800_600_75 =2,         /* 800*600@75Hz, VESA */
    STHDMI_EDID_TIMING_832_624_75 =4,         /* 832*624@75Hz, Apple, MacII */
    STHDMI_EDID_TIMING_1024_768_87 =8,        /* 1024*768@87Hz, Apple, MacII */
    STHDMI_EDID_TIMING_1024_768_60 =16,       /* 1024*768@60Hz, VESA */
    STHDMI_EDID_TIMING_1024_768_70 =32,       /* 1024*768@70Hz, VESA */
    STHDMI_EDID_TIMING_1024_768_75 =64,       /* 1024*768@75Hz, VESA */
    STHDMI_EDID_TIMING_1280_1024_75 =128     /* 1280*1024@75Hz, VESA */
}STHDMI_EDIDEstablishedTimingII_t;

typedef enum STHDMI_EDIDDecodeStereo_e
{
    STHDMI_EDID_NO_STEREO,
    STHDMI_EDID_STEREO_RIGHT_IMAGE,           /* Field Sequentiel stereo, right image when stereo sync=1 */
    STHDMI_EDID_STEREO_LEFT_IMAGE,            /* Field Sequentiel stereo, left image when stereo sync =1 */
    STHDMI_EDID_STEREO_2WAY_INTERLEAVED_RIGHT,   /* 2-way interleaved stereo, right image on even lines */
    STHDMI_EDID_STEREO_2WAY_INTERLEAVED_LEFT,   /* 2-way interleaved stereo, left image on even lines  */
    STHDMI_EDID_STEREO_4WAY_INTERLEAVED,      /* 4-way interleaved stereo */
    STHDMI_EDID_STEREO_SBS_INTERLEAVED        /* Side-by-side interleaved stereo */

}STHDMI_EDIDDecodeStereo_t;

typedef enum STHDMI_EDIDSyncSignalType_e
{
    STHDMI_EDID_SYNC_ANALOG_COMPOSITE,       /* Analog composite */
    STHDMI_EDID_SYNC_BIP_ANALOG_COMPOSITE,   /* Bipolar Analog Composite */
    STHDMI_EDID_SYNC_DIGITAL_COMPOSITE,      /* Digital Composite */
    STHDMI_EDID_SYNC_DIGITAL_SEPARATE       /* Digital Separate */
}STHDMI_EDIDSyncSignalType_t;


typedef enum STHDMI_CEC_Opcode_e
{
    STHDMI_CEC_OPCODE_FEATURE_ABORT                        = 0x00,
    STHDMI_CEC_OPCODE_ABORT_MESSAGE                        = 0xFF,
    STHDMI_CEC_OPCODE_ACTIVE_SOURCE                        = 0x82,
    STHDMI_CEC_OPCODE_INACTIVE_SOURCE                      = 0x9D,
    STHDMI_CEC_OPCODE_REQUEST_ACTIVE_SOURCE                = 0x85,
    STHDMI_CEC_OPCODE_CLEAR_ANALOGUE_TIMER                 = 0x33,
    STHDMI_CEC_OPCODE_CLEAR_DIGITAL_TIMER                  = 0x99,
    STHDMI_CEC_OPCODE_CLEAR_EXTERNAL_TIMER                 = 0xA1,
    STHDMI_CEC_OPCODE_SET_ANALOGUE_TIMER                   = 0x34,
    STHDMI_CEC_OPCODE_SET_DIGITAL_TIMER                    = 0x97,
    STHDMI_CEC_OPCODE_SET_EXTERNAL_TIMER                   = 0xA2,
    STHDMI_CEC_OPCODE_SET_TIMER_PROGRAM_TITLE              = 0x67,
    STHDMI_CEC_OPCODE_TIMER_CLEARED_STATUS                 = 0x43,
    STHDMI_CEC_OPCODE_TIMER_STATUS                         = 0x35,
    STHDMI_CEC_OPCODE_CEC_VERSION                          = 0x9E,
    STHDMI_CEC_OPCODE_GET_CEC_VERSION                      = 0x9F,
    STHDMI_CEC_OPCODE_GIVE_PHYSICAL_ADDRESS                = 0x83,
    STHDMI_CEC_OPCODE_REPORT_PHYSICAL_ADDRESS              = 0x84,
    STHDMI_CEC_OPCODE_GET_MENU_LANGUAGE                    = 0x91,
    STHDMI_CEC_OPCODE_SET_MENU_LANGUAGE                    = 0x32,
    STHDMI_CEC_OPCODE_DECK_CONTROL                         = 0x42,
    STHDMI_CEC_OPCODE_DECK_STATUS                          = 0x1B,
    STHDMI_CEC_OPCODE_GIVE_DECK_STATUS                     = 0x1A,
    STHDMI_CEC_OPCODE_PLAY                                 = 0x41,
    STHDMI_CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS             = 0x08,
    STHDMI_CEC_OPCODE_SELECT_ANALOGUE_SERVICE              = 0x92,
    STHDMI_CEC_OPCODE_SELECT_DIGITAL_SERVICE               = 0x93,
    STHDMI_CEC_OPCODE_TUNER_DEVICE_STATUS                  = 0x07,
    STHDMI_CEC_OPCODE_TUNER_STEP_DECREMENT                 = 0x06,
    STHDMI_CEC_OPCODE_TUNER_STEP_INCREMENT                 = 0x05,
    STHDMI_CEC_OPCODE_DEVICE_VENDOR_ID                     = 0x87,
    STHDMI_CEC_OPCODE_GIVE_DEVICE_VENDOR_ID                = 0x8C,
    STHDMI_CEC_OPCODE_VENDOR_COMMAND                       = 0x89,
    STHDMI_CEC_OPCODE_VENDOR_COMMAND_WITH_ID               = 0xA0,
    STHDMI_CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN            = 0x8A,
    STHDMI_CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP              = 0x8B,
    STHDMI_CEC_OPCODE_SET_OSD_STRING                       = 0x64,
    STHDMI_CEC_OPCODE_GIVE_OSD_NAME                        = 0x46,
    STHDMI_CEC_OPCODE_SET_OSD_NAME                         = 0x47,
    STHDMI_CEC_OPCODE_IMAGE_VIEW_ON                        = 0x04,
    STHDMI_CEC_OPCODE_TEXT_VIEW_ON                         = 0x0D,
    STHDMI_CEC_OPCODE_ROUTING_CHANGE                       = 0x80,
    STHDMI_CEC_OPCODE_ROUTING_INFORMATION                  = 0x81,
    STHDMI_CEC_OPCODE_SET_STREAM_PATH                      = 0x86,
    STHDMI_CEC_OPCODE_STANDBY                              = 0x36,
    STHDMI_CEC_OPCODE_RECORD_OFF                           = 0x0B,
    STHDMI_CEC_OPCODE_RECORD_ON                            = 0x09,
    STHDMI_CEC_OPCODE_RECORD_STATUS                        = 0x0A,
    STHDMI_CEC_OPCODE_RECORD_TV_SCREEN                     = 0x0F,
    STHDMI_CEC_OPCODE_MENU_REQUEST                         = 0x8D,
    STHDMI_CEC_OPCODE_MENU_STATUS                          = 0x8E,
    STHDMI_CEC_OPCODE_USER_CONTROL_PRESSED                 = 0x44,
    STHDMI_CEC_OPCODE_USER_CONTROL_RELEASED                = 0x45,
    STHDMI_CEC_OPCODE_GIVE_DEVICE_POWER_STATUS             = 0x8F,
    STHDMI_CEC_OPCODE_REPORT_POWER_STATUS                  = 0x90,
    STHDMI_CEC_OPCODE_GIVE_AUDIO_STATUS                    = 0x71,
    STHDMI_CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS        = 0x7D,
    STHDMI_CEC_OPCODE_REPORT_AUDIO_STATUS                  = 0x7A,
    STHDMI_CEC_OPCODE_SET_SYSTEM_AUDIO_MODE                = 0x72,
    STHDMI_CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST            = 0x70,
    STHDMI_CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS             = 0x7E,
    STHDMI_CEC_OPCODE_SET_AUDIO_RATE                       = 0x9A,
    STHDMI_CEC_OPCODE_POLLING_MESSAGE
}STHDMI_CEC_Opcode_t;

    typedef enum STHDMI_CEC_AbortReason_e
    {
        STHDMI_CEC_ABORT_REASON_UNRECOGNIZED_OPCODE             = 0,
        STHDMI_CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND  = 1,
        STHDMI_CEC_ABORT_REASON_CANNOT_PROVIDE_SOURCE           = 2,
        STHDMI_CEC_ABORT_REASON_INVALID_OPERAND                 = 3,
        STHDMI_CEC_ABORT_REASON_REFUSED                         = 4
    }STHDMI_CEC_AbortReason_t;

    typedef enum STHDMI_CEC_AnalogueBroadcastType_e
    {
        STHDMI_CEC_ANALOGUE_BROADCAST_TYPE_CABLE       = 0x00,
        STHDMI_CEC_ANALOGUE_BROADCAST_TYPE_SATELLITE   = 0x01,
        STHDMI_CEC_ANALOGUE_BROADCAST_TYPE_TERRESTRIAL = 0x02
    }STHDMI_CEC_AnalogueBroadcastType_t;

    typedef enum STHDMI_CEC_AudioRate_e
    {
        STHDMI_CEC_AUDIO_RATE_RATE_CONTROL_OFF     = 0,
        STHDMI_CEC_AUDIO_RATE_WIDE_STANDARD_RATE   = 1,
        STHDMI_CEC_AUDIO_RATE_WIDE_FAST_RATE       = 2,
        STHDMI_CEC_AUDIO_RATE_WIDE_SLOW_RATE       = 3,
        STHDMI_CEC_AUDIO_RATE_NARROW_STANDARD_RATE = 4,
        STHDMI_CEC_AUDIO_RATE_NARROW_FAST_RATE     = 5,
        STHDMI_CEC_AUDIO_RATE_NARROW_SLOW_RATE     = 6
    }STHDMI_CEC_AudioRate_t;

            typedef enum STHDMI_CEC_AudioVolumeStatus_e
            {
                STHDMI_CEC_AUDIO_VOLUME_STATUS_UNKNOWN   = 0x7F
            }STHDMI_CEC_AudioVolumeStatus_Unknown_t;

        typedef enum STHDMI_CEC_AudioMuteStatus_e
        {
            STHDMI_CEC_AUDIO_MUTE_STATUS_OFF     = 0x00,
            STHDMI_CEC_AUDIO_MUTE_STATUS_ON      = 0x80
        }STHDMI_CEC_AudioMuteStatus_t;

   typedef enum STHDMI_CEC_SystemAudioStatus_e
   {
         STHDMI_CEC_SYSTEM_AUDIO_STATUS_OFF                = 0x00,
         STHDMI_CEC_SYSTEM_AUDIO_STATUS_ON                 = 0x01
   }STHDMI_CEC_SystemAudioStatus_t;

   typedef enum STHDMI_CEC_BroadcastSystem_e
   {
        STHDMI_CEC_BROADCAST_SYSTEM_PAL_BG          = 0,
        STHDMI_CEC_BROADCAST_SYSTEM_SECAM_L_PRIME   = 1,
        STHDMI_CEC_BROADCAST_SYSTEM_PAL_M           = 2,
        STHDMI_CEC_BROADCAST_SYSTEM_NTSC_M          = 3,
        STHDMI_CEC_BROADCAST_SYSTEM_PAL_I           = 4,
        STHDMI_CEC_BROADCAST_SYSTEM_SECAM_DK        = 5,
        STHDMI_CEC_BROADCAST_SYSTEM_SECAM_BG        = 6,
        STHDMI_CEC_BROADCAST_SYSTEM_SECAM_L         = 7,
        STHDMI_CEC_BROADCAST_SYSTEM_PAL_DK          = 8,
        STHDMI_CEC_BROADCAST_SYSTEM_OTHER_SYSTEM    = 31
   }STHDMI_CEC_BroadcastSystem_t;

   typedef enum STHDMI_CEC_CECVersion_e
   {
         STHDMI_CEC_CEC_VERSION_1_1        = 0x00,
         STHDMI_CEC_CEC_VERSION_1_2        = 0x01,
         STHDMI_CEC_CEC_VERSION_1_2_A      = 0x02,
         STHDMI_CEC_CEC_VERSION_1_3        = 0x03,
         STHDMI_CEC_CEC_VERSION_1_3_A      = 0x04
   }STHDMI_CEC_CECVersion_t;

        typedef enum STHDMI_CEC_ChannelNumberFormat_e
        {
                STHDMI_CEC_CHANNEL_NUMBER_FORMAT_ONE_PART        = 0x400,  /*ONE_PART_CHANNEL_NUMBER*/
                STHDMI_CEC_CHANNEL_NUMBER_FORMAT_TWO_PART        = 0x800   /*TWO_PART_CHANNEL_NUMBER*/
        }STHDMI_CEC_ChannelNumberFormat_t;

   typedef enum STHDMI_CEC_DeckControlMode_e
   {
        STHDMI_CEC_DECK_CONTROL_MODE_SKIP_FORWARD_WIND       = 1,
        STHDMI_CEC_DECK_CONTROL_MODE_SKIP_REVERSE_REWIND     = 2,
        STHDMI_CEC_DECK_CONTROL_MODE_STOP                    = 3,
        STHDMI_CEC_DECK_CONTROL_MODE_EJECT                   = 4
   }STHDMI_CEC_DeckControlMode_t;

   typedef enum STHDMI_CEC_DeckInfo_e
   {
        STHDMI_CEC_DECK_INFO_PLAY                     = 0x11,
        STHDMI_CEC_DECK_INFO_RECORD                   = 0x12,
        STHDMI_CEC_DECK_INFO_PLAY_REVERSE             = 0x13,
        STHDMI_CEC_DECK_INFO_STILL                    = 0x14,
        STHDMI_CEC_DECK_INFO_SLOW                     = 0x15,
        STHDMI_CEC_DECK_INFO_SLOW_REVERSE             = 0x16,
        STHDMI_CEC_DECK_INFO_FAST_FORWARD             = 0x17,
        STHDMI_CEC_DECK_INFO_FAST_REVERSE             = 0x18,
        STHDMI_CEC_DECK_INFO_NO_MEDIA                 = 0x19,
        STHDMI_CEC_DECK_INFO_STOP                     = 0x1A,
        STHDMI_CEC_DECK_INFO_SKIP_FROWARD_WIND        = 0x1B,
        STHDMI_CEC_DECK_INFO_SKIP_REVERSE_REWIND      = 0x1C,
        STHDMI_CEC_DECK_INFO_INDEX_SEARCH_FORWARD     = 0x1D,
        STHDMI_CEC_DECK_INFO_INDEX_SEARCH_REVERSE     = 0x1E,
        STHDMI_CEC_DECK_INFO_OTHER_STATUS             = 0x1F
   }STHDMI_CEC_DeckInfo_t;

   typedef enum STHDMI_CEC_DeviceType_e
   {
        STHDMI_CEC_DEVICE_TYPE_TV                       = 0,
        STHDMI_CEC_DEVICE_TYPE_RECORDING_DEVICE         = 1,
        STHDMI_CEC_DEVICE_TYPE_TUNER                    = 3,
        STHDMI_CEC_DEVICE_TYPE_PLAYBACK_DEVICE          = 4,
        STHDMI_CEC_DEVICE_TYPE_AUDIO_SYSTEM             = 5
   }STHDMI_CEC_DeviceType_t;

        typedef enum STHDMI_CEC_ServiceIdentificationMethod_e
        {
                STHDMI_CEC_SERVICE_IDENTIFICATION_METHOD_BY_DIGITAL_IDS    = 0x00,   /*SERVICE_IDENTIFIED_BY_DIGITAL_IDS*/
                STHDMI_CEC_SERVICE_IDENTIFICATION_METHOD_BY_CHANNEL        = 0x80    /*SERVICE_IDENTIFIED_BY_CHANNEL*/
        }STHDMI_CEC_ServiceIdentificationMethod_t;

        typedef enum STHDMI_CEC_DigitalBroadcastSystem_e
        {
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_GENERIC         = 0x00,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_GENERIC         = 0x01,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_GENERIC          = 0x02,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_BS              = 0x08,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_CS              = 0x09,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ARIB_T               = 0x0A,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_CABLE           = 0x10,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_SATELLITE       = 0x11,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_ATSC_TERRESTRIAL     = 0x12,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_C                = 0x18,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_S                = 0x19,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_S2               = 0x1A,
                STHDMI_CEC_DIGITAL_BROADCAST_SYSTEM_DVB_T                = 0x1B
        }STHDMI_CEC_DigitalBroadcastSystem_t;

  typedef enum STHDMI_CEC_PowerStatus_e
  {
        STHDMI_CEC_POWER_STATUS_ON                           = 0x00,
        STHDMI_CEC_POWER_STATUS_STANDBY                      = 0x01,
        STHDMI_CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON  = 0x02,
        STHDMI_CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY  = 0x03
  }STHDMI_CEC_PowerStatus_t;

  typedef enum STHDMI_CEC_UICommand_Opcode_e /*User Interface Command Opcode */
  {
        STHDMI_CEC_UICOMMAND_OPCODE_SELECT                      = 0x00,
        STHDMI_CEC_UICOMMAND_OPCODE_UP                          = 0x01,
        STHDMI_CEC_UICOMMAND_OPCODE_DOWN                        = 0x02,
        STHDMI_CEC_UICOMMAND_OPCODE_LEFT                        = 0x03,
        STHDMI_CEC_UICOMMAND_OPCODE_RIGHT                       = 0x04,
        STHDMI_CEC_UICOMMAND_OPCODE_RIGHT_UP                    = 0x05,
        STHDMI_CEC_UICOMMAND_OPCODE_RIGHT_DOWN                  = 0x06,
        STHDMI_CEC_UICOMMAND_OPCODE_LEFT_UP                     = 0x07,
        STHDMI_CEC_UICOMMAND_OPCODE_LEFT_DOWN                   = 0x08,
        STHDMI_CEC_UICOMMAND_OPCODE_ROOT_MENU                   = 0x09,
        STHDMI_CEC_UICOMMAND_OPCODE_SETUP_MENU                  = 0x0A,
        STHDMI_CEC_UICOMMAND_OPCODE_CONTENTS_MENU               = 0x0B,
        STHDMI_CEC_UICOMMAND_OPCODE_FAVORITE_MENU               = 0x0C,
        STHDMI_CEC_UICOMMAND_OPCODE_EXIT                        = 0x0D,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_0                       = 0x20,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_1                       = 0x21,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_2                       = 0x22,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_3                       = 0x23,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_4                       = 0x24,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_5                       = 0x25,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_6                       = 0x26,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_7                       = 0x27,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_8                       = 0x28,
        STHDMI_CEC_UICOMMAND_OPCODE_NUM_9                       = 0x29,
        STHDMI_CEC_UICOMMAND_OPCODE_DOT                         = 0x2A,
        STHDMI_CEC_UICOMMAND_OPCODE_ENTER                       = 0x2B,
        STHDMI_CEC_UICOMMAND_OPCODE_CLEAR                       = 0x2C,
        STHDMI_CEC_UICOMMAND_OPCODE_NEXT_FAVORITE               = 0x2F,
        STHDMI_CEC_UICOMMAND_OPCODE_CHANNEL_UP                  = 0x30,
        STHDMI_CEC_UICOMMAND_OPCODE_CHANNEL_DOWN                = 0x31,
        STHDMI_CEC_UICOMMAND_OPCODE_PREVIOUS_CHANNEL            = 0x32,
        STHDMI_CEC_UICOMMAND_OPCODE_SOUND_SELECT                = 0x33,
        STHDMI_CEC_UICOMMAND_OPCODE_INPUT_SELECT                = 0x34,
        STHDMI_CEC_UICOMMAND_OPCODE_DISPLAY_INFORMATION         = 0x35,
        STHDMI_CEC_UICOMMAND_OPCODE_HELP                        = 0x36,
        STHDMI_CEC_UICOMMAND_OPCODE_PAGE_UP                     = 0x37,
        STHDMI_CEC_UICOMMAND_OPCODE_PAGE_DOWN                   = 0x38,
        STHDMI_CEC_UICOMMAND_OPCODE_POWER                       = 0x40,
        STHDMI_CEC_UICOMMAND_OPCODE_VOLUME_UP                   = 0x41,
        STHDMI_CEC_UICOMMAND_OPCODE_VOLUME_DOWN                 = 0x42,
        STHDMI_CEC_UICOMMAND_OPCODE_MUTE                        = 0x43,
        STHDMI_CEC_UICOMMAND_OPCODE_PLAY                        = 0x44,
        STHDMI_CEC_UICOMMAND_OPCODE_STOP                        = 0x45,
        STHDMI_CEC_UICOMMAND_OPCODE_PAUSE                       = 0x46,
        STHDMI_CEC_UICOMMAND_OPCODE_RECORD                      = 0x47,
        STHDMI_CEC_UICOMMAND_OPCODE_REWIND                      = 0x48,
        STHDMI_CEC_UICOMMAND_OPCODE_FAST_FORWARD                = 0x49,
        STHDMI_CEC_UICOMMAND_OPCODE_EJECT                       = 0x4A,
        STHDMI_CEC_UICOMMAND_OPCODE_FORWARD                     = 0x4B,
        STHDMI_CEC_UICOMMAND_OPCODE_BACKWARD                    = 0x4C,
        STHDMI_CEC_UICOMMAND_OPCODE_STOP_RECORD                 = 0x4D,
        STHDMI_CEC_UICOMMAND_OPCODE_PAUSE_RECORD                = 0x4E,
        STHDMI_CEC_UICOMMAND_OPCODE_ANGLE                       = 0x50,
        STHDMI_CEC_UICOMMAND_OPCODE_SUBPICTURE                  = 0x51,
        STHDMI_CEC_UICOMMAND_OPCODE_VIDEO_ON_DEMAND             = 0x52,
        STHDMI_CEC_UICOMMAND_OPCODE_ELECTRONIC_PROGRAM_GUIDE    = 0x53,
        STHDMI_CEC_UICOMMAND_OPCODE_TIMER_PROGRAMMING           = 0x54,
        STHDMI_CEC_UICOMMAND_OPCODE_INITIAL_CONFIGURATION       = 0x55,
        STHDMI_CEC_UICOMMAND_OPCODE_PLAY_FUNCTION               = 0x60,
        STHDMI_CEC_UICOMMAND_OPCODE_PAUSE_PLAY_FUNCTION         = 0x61,
        STHDMI_CEC_UICOMMAND_OPCODE_RECORD_FUNCTION             = 0x62,
        STHDMI_CEC_UICOMMAND_OPCODE_PAUSE_RECORD_FUNCTION       = 0x63,
        STHDMI_CEC_UICOMMAND_OPCODE_STOP_FUNCTION               = 0x64,
        STHDMI_CEC_UICOMMAND_OPCODE_MUTE_FUNCTION               = 0x65,
        STHDMI_CEC_UICOMMAND_OPCODE_RESTORE_VOLUME_FUNCTION     = 0x66,
        STHDMI_CEC_UICOMMAND_OPCODE_TUNE_FUNCTION               = 0x67,
        STHDMI_CEC_UICOMMAND_OPCODE_SELECT_MEDIA_FUNCTION       = 0x68,
        STHDMI_CEC_UICOMMAND_OPCODE_SELECT_AV_INPUT_FUNCTION    = 0x69,
        STHDMI_CEC_UICOMMAND_OPCODE_SELECT_AUDIO_INPUT_FUNCTION = 0x6A,
        STHDMI_CEC_UICOMMAND_OPCODE_POWER_TOGGLE_FUNCTION       = 0x6B,
        STHDMI_CEC_UICOMMAND_OPCODE_POWER_OFF_FUNCTION          = 0x6C,
        STHDMI_CEC_UICOMMAND_OPCODE_POWER_ON_FUNCTION           = 0x6D,
        STHDMI_CEC_UICOMMAND_OPCODE_F1_BLUE                     = 0x71,
        STHDMI_CEC_UICOMMAND_OPCODE_F2_RED                      = 0x72,
        STHDMI_CEC_UICOMMAND_OPCODE_F3_GREEN                    = 0x73,
        STHDMI_CEC_UICOMMAND_OPCODE_F4_YELLOW                   = 0x74,
        STHDMI_CEC_UICOMMAND_OPCODE_F5                          = 0x75,
        STHDMI_CEC_UICOMMAND_OPCODE_DATA                        = 0x76
}STHDMI_CEC_UICommand_Opcode_t; /*User Interface Command Opcode */

  typedef enum STHDMI_CEC_PlayMode_e
  {
        STHDMI_CEC_PLAY_MODE_FORWARD                         = 0x24, /*PLAY_FORWARD*/
        STHDMI_CEC_PLAY_MODE_REVERSE                         = 0x20, /*PLAY_REVERSE*/
        STHDMI_CEC_PLAY_MODE_STILL                           = 0x25, /*PLAY_STILL*/
        STHDMI_CEC_PLAY_MODE_FAST_FORWARD_MIN_SPEED          = 0x05,
        STHDMI_CEC_PLAY_MODE_FAST_FORWARD_MEDUIM_SPEED       = 0x06,
        STHDMI_CEC_PLAY_MODE_FAST_FORWARD_MAX_SPEED          = 0x07,
        STHDMI_CEC_PLAY_MODE_FAST_REVERSE_MIN_SPEED          = 0x09,
        STHDMI_CEC_PLAY_MODE_FAST_REVERSE_MEDUIM_SPEED       = 0x0A,
        STHDMI_CEC_PLAY_MODE_FAST_REVERSE_MAX_SPEED          = 0x0B,
        STHDMI_CEC_PLAY_MODE_SLOW_FORWARD_MIN_SPEED          = 0x15,
        STHDMI_CEC_PLAY_MODE_SLOW_FORWARD_MEDUIM_SPEED       = 0x16,
        STHDMI_CEC_PLAY_MODE_SLOW_FORWARD_MAX_SPEED          = 0x17,
        STHDMI_CEC_PLAY_MODE_SLOW_REVERSE_MIN_SPEED          = 0x19,
        STHDMI_CEC_PLAY_MODE_SLOW_REVERSE_MEDUIM_SPEED       = 0x1A,
        STHDMI_CEC_PLAY_MODE_SLOW_REVERSE_MAX_SPEED          = 0x1B
}STHDMI_CEC_PlayMode_t;

  typedef enum STHDMI_CEC_MenuState_e
  {
        STHDMI_CEC_MENU_STATE_ACTIVATED                = 0x00,
        STHDMI_CEC_MENU_STATE_DEACTIVATED              = 0x01
  }STHDMI_CEC_MenuState_t;

  typedef enum STHDMI_CEC_MenuRequestType_e
  {
        STHDMI_CEC_MENU_REQUEST_TYPE_ACTIVATE           = 0x00,
        STHDMI_CEC_MENU_REQUEST_TYPE_DEACTIVATE         = 0x01,
        STHDMI_CEC_MENU_REQUEST_TYPE_QUERY              = 0x02
  }STHDMI_CEC_MenuRequestType_t;

  typedef enum STHDMI_CEC_RecordStatusInfo_e
  {
        STHDMI_CEC_RECORD_STATUS_INFO_RECORDING_CURRENTLY_SELECTED_SOURCE           = 0x01,
        STHDMI_CEC_RECORD_STATUS_INFO_RECORDING_DIGITAL_SERVICE                     = 0x02,
        STHDMI_CEC_RECORD_STATUS_INFO_RECORDING_ANALOGUE_SERVICE                    = 0x03,
        STHDMI_CEC_RECORD_STATUS_INFO_RECORDING_EXTERNAL_INPUT                      = 0x04,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_UNABLE_TO_RECORD_DIG_SERVICE     = 0x05,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_UNABLE_TO_RECORD_ANA_SERVICE     = 0x06,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_UNABLE_TO_SELECT_REQ_SERVICE     = 0x07,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_INVALID_EXT_PLUG_NUMBER          = 0x09,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_INVALID_EXT_PHYSICAL_ADDRESS     = 0x0A,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_CA_SYSTEM_NOT_SUPPORTED          = 0x0B,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_INSUFFICIENT_CA_ENTIT         = 0x0C,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_NOT_ALLOWED_TO_COPY_SOURCE       = 0x0D,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_FURTHER_COPIES_ALLOWED        = 0x0E,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_MEDIA                         = 0x10,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_PLAYING                          = 0x11,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_ALREADY_RECORDING                = 0x12,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_MEDIA_PROTECTED                  = 0x13,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_SOURCE_SIGNAL                 = 0x14,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_MEDIA_PROBLEM                    = 0x15,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_NOT_ENOUGH_SPACE_AVAILABLE       = 0x16,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_PARENTAL_LOCK_ON                 = 0x17,
        STHDMI_CEC_RECORD_STATUS_INFO_RECORDING_TERMINATED_NORMALLY                 = 0x1A,
        STHDMI_CEC_RECORD_STATUS_INFO_RECORDING_HAS_ALREADY_TERMINATED              = 0x1B,
        STHDMI_CEC_RECORD_STATUS_INFO_NO_RECORDING_OTHER_REASON                     = 0x1F
  }STHDMI_CEC_RecordStatusInfo_t;

        typedef enum STHDMI_CEC_RecordSourceType_e
        {
            STHDMI_CEC_RECORD_SOURCE_TYPE_OWN_SOURCE                   = 0x01,
            STHDMI_CEC_RECORD_SOURCE_TYPE_DIGITAL_SERVICE              = 0x02,
            STHDMI_CEC_RECORD_SOURCE_TYPE_ANALOGUE_SERVICE             = 0x03,
            STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PLUG                = 0x04,
            STHDMI_CEC_RECORD_SOURCE_TYPE_EXTERNAL_PHYSCIAL_ADDRESS    = 0x05
        }STHDMI_CEC_RecordSourceType_t;

    typedef enum STHDMI_CEC_ExternalSourceSpecifier_e
    {
            STHDMI_CEC_EXTERNAL_SOURCE_SPECIFIER_PLUG                           = 4, /*EXTERNAL_PLUG*/
            STHDMI_CEC_EXTERNAL_SOURCE_SPECIFIER_PHYSICAL_ADDRESS               = 5  /*EXTERNAL_PHYSICAL_ADDRESS*/
    }STHDMI_CEC_ExternalSourceSpecifier_t;

  typedef enum STHDMI_CEC_TimerClearedStatusData_e
  {
        STHDMI_CEC_TIMER_CLEARED_STATUS_DATA_TIMER_NOT_CLEARED_RECORDING              = 0x00,
        STHDMI_CEC_TIMER_CLEARED_STATUS_DATA_TIMER_NOT_CLEARED_NO_MATCHING            = 0x01,
        STHDMI_CEC_TIMER_CLEARED_STATUS_DATA_TIMER_NOT_CLEARED_NO_INFO_AVAILABLE      = 0x02,
        STHDMI_CEC_TIMER_CLEARED_STATUS_DATA_TIMER_CLEARED                            = 0x80
  }STHDMI_CEC_TimerClearedStatusData_t;

                typedef enum STHDMI_CEC_ProgrammedInfo_e
                {
                    STHDMI_CEC_PROGRAMMED_INFO_ENOUGH_SPACE_AVAILABLE_FOR_RECORDING        = 0x08,
                    STHDMI_CEC_PROGRAMMED_INFO_NOT_ENOUGH_SPACE_AVAILABLE_FOR_RECORDING    = 0x09,
                    STHDMI_CEC_PROGRAMMED_INFO_MAY_NOT_BE_ENOUGH_SPACE_AVAILABLE           = 0x0B,
                    STHDMI_CEC_PROGRAMMED_INFO_NO_MEDIA_INFO_AVAILABLE                     = 0x0A
                }STHDMI_CEC_ProgrammedInfo_t;

                typedef enum STHDMI_CEC_NotProgrammedErrorInfo_e
                {
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_NO_FREE_TIMER_AVAILABLE                      = 0x01,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_DATE_OUT_OF_RANGE                            = 0x02,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_RECORDING_SEQUENCE_ERROR                     = 0x03,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_INVALID_EXTERNAL_PLUG_NUMBER                 = 0x04,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_INVALID_EXTERNAL_PHYSICAL_ADDRESS            = 0x05,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_CA_SYSTEM_NOT_SUPPORTED                      = 0x06,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_NO_OR_INSUFFICIENT_CA_ENTITLEMENTS           = 0x07,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_DOES_NOT_SUPPORT_RESOLUTION                  = 0x08,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_PARENTAL_LOCK_ON                             = 0x09,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_CLOCK_FAILURE                                = 0x0A,
                    STHDMI_CEC_NOT_PROGRAMMED_ERROR_INFO_DUPLICATE                                    = 0x0E   /* ALREADY PROGRAMMED     */
                }STHDMI_CEC_NotProgrammedErrorInfo_t;

            typedef enum STHDMI_CEC_ProgrammedIndicator_e
            {
                STHDMI_CEC_PROGRAMMED_INDICATOR_NOT_PROGRAMMED           = 0x00,
                STHDMI_CEC_PROGRAMMED_INDICATOR_PROGRAMMED               = 0x10
            }STHDMI_CEC_ProgrammedIndicator_t;

        typedef enum STHDMI_CEC_TimerOverlapWarning_e
        {
            STHDMI_CEC_TIMER_OVERLAP_WARNING_NO_OVERLAP                = 0x00,
            STHDMI_CEC_TIMER_OVERLAP_WARNING_TIMER_BLOCK_OVERLAP       = 0x80
        }STHDMI_CEC_TimerOverlapWarning_t;

        typedef enum STHDMI_CEC_MediaInfo_e
        {
            STHDMI_CEC_MEDIA_INFO_PRESENT_NOT_PROTECTED     = 0x00,   /*MEDIA_PRESENT_NOT_PROTECTED*/
            STHDMI_CEC_MEDIA_INFO_PRESENT_PROTECTED         = 0x20,   /*MEDIA_PRESENT_PROTECTED*/
            STHDMI_CEC_MEDIA_INFO_NOT_PRESENT               = 0x40    /*MEDIA_NOT_PRESENT*/
        }STHDMI_CEC_MediaInfo_t;

        typedef enum STHDMI_CEC_DisplayControl_e
        {
                STHDMI_CEC_DISPLAY_CONTROL_DISPLAY_FOR_DEFAULT_TIME                = 0x00,
                STHDMI_CEC_DISPLAY_CONTROL_DISPLAY_UNTIL_CLEARED                   = 0x40,
                STHDMI_CEC_DISPLAY_CONTROL_CLEAR_PREVIOUS_MESSAGE                  = 0x80
        }STHDMI_CEC_DisplayControl_t;

  typedef enum STHDMI_CEC_StatusRequest_e
  {
        STHDMI_CEC_STATUS_REQUEST_ON                              = 0x01,
        STHDMI_CEC_STATUS_REQUEST_OFF                             = 0x02,
        STHDMI_CEC_STATUS_REQUEST_ONCE                            = 0x03
  }STHDMI_CEC_StatusRequest_t;


        typedef enum STHDMI_CEC_RecordingFlag_e
        {
                STHDMI_CEC_RECORDING_FLAG_NOT_BEING_USED_FOR_RECORDING                 = 0x00,
                STHDMI_CEC_RECORDING_FLAG_BEING_USED_FOR_RECORDING                     = 0x80
        }STHDMI_CEC_RecordingFlag_t;

        typedef enum STHDMI_CEC_TunerDisplayInfo_e
        {
                STHDMI_CEC_TUNER_DISPLAY_INFO_DISPLAYING_DIGITAL_TUNER                 = 0x00,
                STHDMI_CEC_TUNER_DISPLAY_INFO_NOT_DISPLAYING_TUNER                     = 0x01,
                STHDMI_CEC_TUNER_DISPLAY_INFO_DISPLAYING_ANALOGUE_TUNER                = 0x02
        }STHDMI_CEC_TunerDisplayInfo_t;

    typedef enum STHDMI_CEC_RecordingSequence_e
    {
        STHDMI_CEC_RECORDING_SEQUENCE_SUNDAY      = 0x01,
        STHDMI_CEC_RECORDING_SEQUENCE_MONDAY      = 0x02,
        STHDMI_CEC_RECORDING_SEQUENCE_TUESDAY     = 0x04,
        STHDMI_CEC_RECORDING_SEQUENCE_WEDNESDAY   = 0x08,
        STHDMI_CEC_RECORDING_SEQUENCE_THURSDAY    = 0x10,
        STHDMI_CEC_RECORDING_SEQUENCE_FRIDAY      = 0x20,
        STHDMI_CEC_RECORDING_SEQUENCE_SATURDAY    = 0x40,
        STHDMI_CEC_RECORDING_SEQUENCE_ONCE_ONLY   = 0x00
    }STHDMI_CEC_RecordingSequence_t;



/* Exported Types --------------------------------------------------------- */

typedef struct STHDMI_AVIInfoFrameVer1_s
{
    STHDMI_InfoFrameType_t   FrameType;
    U32                      FrameVersion ;
    U32                      FrameLength;
    STVOUT_OutputType_t      OutputType;
    BOOL                     HasActiveFormatInformation;
    STHDMI_BarInfo_t         BarInfo;
    STHDMI_ScanInfo_t        ScanInfo;
    STVOUT_ColorSpace_t      Colorimetry;
    STGXOBJ_AspectRatio_t    AspectRatio;
    STGXOBJ_AspectRatio_t    ActiveAspectRatio;
    STHDMI_PictureScaling_t  PictureScaling;
    U32                      LineNEndofTopLower;
    U32                      LineNEndofTopUpper;
    U32                      LineNStartofBotLower;
    U32                      LineNStartofBotUpper;
    U32                      PixelNEndofLeftLower;
    U32                      PixelNEndofLeftUpper;
    U32                      PixelNStartofRightLower;
    U32                      PixelNStartofRightUpper;

}STHDMI_AVIInfoFrameVer1_t;

typedef struct STHDMI_AVIInfoFrameVer2_s
{

    STHDMI_InfoFrameType_t   FrameType;
    U32                      FrameVersion ;
    U32                      FrameLength;
    STVOUT_OutputType_t      OutputType;
    BOOL                     HasActiveFormatInformation;
    STHDMI_BarInfo_t         BarInfo;
    STHDMI_ScanInfo_t        ScanInfo;
    STVOUT_ColorSpace_t      Colorimetry;
    STGXOBJ_AspectRatio_t    AspectRatio;
    STGXOBJ_AspectRatio_t    ActiveAspectRatio;
    STHDMI_PictureScaling_t  PictureScaling;
    U32                      PixelRepetition;
    U32                      VideoFormatIDCode;
    U32                      LineNEndofTopLower;
    U32                      LineNEndofTopUpper;
    U32                      LineNStartofBotLower;
    U32                      LineNStartofBotUpper;
    U32                      PixelNEndofLeftLower;
    U32                      PixelNEndofLeftUpper;
    U32                      PixelNStartofRightLower;
    U32                      PixelNStartofRightUpper;

}STHDMI_AVIInfoFrameVer2_t;

typedef union STHDMI_AVIInfoFrame_u
{
  STHDMI_AVIInfoFrameVer1_t   AVI861A;  /* AVI info frame version1 for EIA/CEA 861A Support */
  STHDMI_AVIInfoFrameVer2_t   AVI861B;  /* AVI Info frame verison2 for EIA/CEA 861B Support*/

}STHDMI_AVIInfoFrame_t;

typedef struct STHDMI_SPDInfoFrame_s
{
    STHDMI_InfoFrameType_t   FrameType;
    U32                      FrameVersion ;
    U32                      FrameLength;
    char                     VendorName[8];
    char                     ProductDescription[16];
    STHDMI_SPDDeviceInfo_t   SourceDeviceInfo;

}STHDMI_SPDInfoFrame_t;

typedef struct STHDMI_AUDIOInfoFrame_s
{
    STHDMI_InfoFrameType_t       FrameType;
    U32                          FrameVersion ;
    U32                          FrameLength;
    U32                          ChannelCount;
    STAUD_StreamContent_t        CodingType;
    STAUD_DACDataPrecision_t     SampleSize;
    STAUD_Object_t               DecoderObject;
    U32                          SamplingFrequency;
    U32                          BitRate;
    U32                          ChannelAlloc;
    U32                          LevelShift;
    BOOL                         DownmixInhibit;
}STHDMI_AUDIOInfoFrame_t;

typedef struct STHDMI_MPEGSourceInfoFrame_s
{
    STHDMI_InfoFrameType_t   FrameType;
    U32                      FrameVersion ;
    U32                      FrameLength;
    U32                      MPEGBitRate;
    BOOL                     IsFieldRepeated;
    STVID_MPEGFrame_t        MepgFrameType;
}STHDMI_MPEGSourceInfoFrame_t;

typedef struct STHDMI_VendorSpecificInfoFrame_s
{
    STHDMI_InfoFrameType_t   FrameType;
    U32                      FrameVersion ;
    U32                      FrameLength;
    U32                      RegistrationId;
    char*                    VSPayload_p;

}STHDMI_VendorSpecInfoFrame_t;

typedef struct STHDMI_ShortVideoDescriptor_s
{
    BOOL  IsNative;
    U32   VideoIdCode;
    struct STHDMI_ShortVideoDescriptor_s*  NextVideoDescriptor_p;
}STHDMI_ShortVideoDescriptor_t;

typedef struct STHDMI_VideoDataBlock_s
{
    U8   TagCode;
    U8   VideoLength;
    STHDMI_ShortVideoDescriptor_t*  VideoDescriptor_p;
}STHDMI_VideoDataBlock_t;

typedef struct STHDMI_ShortAudioDescriptor_s
{
    STAUD_StreamContent_t    AudioFormatCode;
    U32                      NumofAudioChannels;
    U32                      AudioSupported;
    U32                      MaxBitRate;
    struct STHDMI_ShortAudioDescriptor_s*  NextAudioDescriptor_p;
}STHDMI_ShortAudioDescriptor_t;

typedef struct STHDMI_AudioDataBlock_s
{
    U8   TagCode;
    U8   AudioLength;
    STHDMI_ShortAudioDescriptor_t*  AudioDescriptor_p;
}STHDMI_AudioDataBlock_t;

typedef struct STHDMI_SpeakerAllocation_s
{
    U8   TagCode;
    U8   SpeakerLength;
    U8   SpeakerData[3];
}STHDMI_SpeakerAllocation_t;

typedef struct STHDMI_CEC_PhysicalAddress_s
{
    U8 A;
    U8 B;
    U8 C;
    U8 D;
}STHDMI_CEC_PhysicalAddress_t;

typedef struct STHDMI_VendorDataBlock_s
{
    U8  TagCode;
    U8  VendorLength;
    U8 RegistrationId[3];
    STHDMI_CEC_PhysicalAddress_t PhysicalAddress;
    U8* VendorData_p;
}STHDMI_VendorDataBlock_t;

typedef struct STHDMI_DataBlockCollection_s
{
  STHDMI_VideoDataBlock_t     VideoData;
  STHDMI_AudioDataBlock_t     AudioData;
  STHDMI_SpeakerAllocation_t  SpeakerData;
  STHDMI_VendorDataBlock_t    VendorData;
}STHDMI_DataBlockCollection_t;


typedef struct STHDMI_EDIDTimingDescriptor_s
{
    U32    PixelClock;
    U32    HorActivePixel;
    U32    HorBlankingPixel;
    U32    VerActiveLine;
    U32    VerBlankingLine;
    U32    HSyncOffset;
    U32    HSyncWidth;
    U32    VSyncOffset;
    U32    VSyncWidth;
    U32    HImageSize;
    U32    VImageSize;
    U32    HBorder;
    U32    VBorder;
    BOOL   IsInterlaced;
    BOOL   IsHPolarityPositive;
    BOOL   IsVPolarityPositive;
    STHDMI_EDIDDecodeStereo_t  Stereo;
    STHDMI_EDIDSyncSignalType_t SyncDescription;
}STHDMI_EDIDTimingDescriptor_t;

typedef struct STHDMI_EDIDSinkParams_s
{
    BOOL                        IsUnderscanSink;
    BOOL                        IsBasicAudioSupported;
    BOOL                        IsYCbCr444;
    BOOL                        IsYCbCr422;
    U32                         NativeFormatNum;
}STHDMI_EDIDSinkParams_t;

typedef struct STHDMI_EDIDTimingExtVer1_s
{
    U32                            Tag;
    U32                            RevisionNumber;
    U32                            Offset;
    U32                            Checksum;
    U8*                            DataBlock_p;
    U32                            NumberOfTiming;
    STHDMI_EDIDTimingDescriptor_t* TimingDesc_p;
}STHDMI_EDIDTimingExtVer1_t;

typedef struct STHDMI_EDIDTimingExtVer2_s
{
    U32                            Tag;
    U32                            RevisionNumber;
    U32                            Offset;
    U32                            Checksum;
    U8*                            DataBlock_p;
    STHDMI_EDIDSinkParams_t        FormatDescription;
    U32                            NumberOfTiming;
    STHDMI_EDIDTimingDescriptor_t* TimingDesc_p;
}STHDMI_EDIDTimingExtVer2_t;

typedef struct STHDMI_EDIDTimingExtVer3_s
{
    U32    Tag;
    U32    RevisionNumber;
    U32    Offset;
    U32    Checksum;
    STHDMI_EDIDSinkParams_t        FormatDescription;
    STHDMI_DataBlockCollection_t   DataBlock;
    U32                            NumberOfTiming;
    STHDMI_EDIDTimingDescriptor_t* TimingDesc_p;
}STHDMI_EDIDTimingExtVer3_t;

typedef struct STHDMI_EDIDMapBlockExt_s
{
   U32 Tag;
   U32 ExtensionTag[126];
   U32 CheckSum;
}STHDMI_EDIDMapBlockExt_t;

typedef union STHDMI_EDIDTimingExt_u
{
    STHDMI_EDIDTimingExtVer1_t      EDID861;
    STHDMI_EDIDTimingExtVer2_t      EDID861A;
    STHDMI_EDIDTimingExtVer3_t      EDID861B;

}STHDMI_EDIDTimingExt_t;

typedef struct STHDMI_MonitorData_s
{
   U8*  Data_p;
   U8   Length;
}STHDMI_MonitorData_t;

typedef struct STHDMI_GTFParams_s
{
    U32 C;    /* 0=<C=<127*/
    U32 M;    /* 0=<M=<65535*/
    U32 K;    /* 0=< K=<255*/
    U32 J;    /* 0=< J=<127*/
}STHDMI_GTFParams_t;

typedef struct STHDMI_MonitorRangeLimit_s
{
    U32                 MinVertRate;
    U32                 MaxVertRate;
    U32                 MinHorRate;
    U32                 MaxHorRate;
    U32                 MaxPixelClock;
    BOOL                IsSecondGTFSupported;
    U32                 StartFrequency;
    STHDMI_GTFParams_t  GTFParams;
}STHDMI_MonitorRangeLimit_t;

typedef struct STHDMI_MonitorColorPoint_s
{
    U32 White_x[2];
    U32 White_y[2];
    U32 WhiteGamma[2];
}STHDMI_MonitorColorPoint_t;

typedef struct STHDMI_EDIDStandardTiming_s
{
    BOOL                   IsStandardTimingUsed;
    U32                    HorizontalActive;
    U32                    VerticalActive;
    U32                    RefreshRate;
}STHDMI_EDIDStandardTiming_t;

typedef struct STHDMI_MonitorTimingId_s
{
    STHDMI_EDIDStandardTiming_t    StdTiming[6];

}STHDMI_MonitorTimingId_t;

typedef union STHDMI_MonitorDataType_u
{
    STHDMI_MonitorData_t        MonitorData;
    STHDMI_MonitorRangeLimit_t  RangeLimit;
    STHDMI_MonitorColorPoint_t  ColorPoint;
    STHDMI_MonitorTimingId_t    TmingId;
    /* reserved for future use ...*/
}STHDMI_MonitorDataType_t;

typedef struct STHDMI_MonitorDescriptorExt_s
{
    U8                             DataTag;
    STHDMI_MonitorDataType_t       DataDescriptor;

}STHDMI_MonitorDescriptorExt_t;

typedef struct STHDMI_EDIDBlockExtension_s
{
    STHDMI_EDIDTimingExt_t         TimingData;
    STHDMI_EDIDMapBlockExt_t       MapBlock;
    STHDMI_MonitorDescriptorExt_t  MonitorBlock;
    struct STHDMI_EDIDBlockExtension_s*  NextBlockExt_p;
}STHDMI_EDIDBlockExtension_t;

typedef struct STHDMI_EDIDProdDescription_s
{
    char IDManufactureName[3];
    U32 IDProductCode;
    U32 IDSerialNumber;
    U32 WeekOfManufacture;
    U32 YearOfManufacture;
}STHDMI_EDIDProdDescription_t;

typedef struct STHDMI_EDIDStructInfos_t /* This have to change to end with s not t */
{
    U32 Version;
    U32 Revision;
}STHDMI_EDIDStructInfos_t;

typedef struct STHDMI_VideoInputParams_s
{
    BOOL IsDigitalSignal;
    BOOL IsSetupExpected;
    BOOL IsSepSyncSupported;
    BOOL IsCompSyncSupported;
    BOOL IsSyncOnGreenSupported;
    BOOL IsPulseRequired;
    BOOL IsVesaDFPCompatible;
    U32 SignalLevelMax;
    U32 SignalLevelMin;
}STHDMI_VideoInputParams_t;

typedef struct STHDMI_FeatureSupported_s
{
   BOOL                       IsStandby;
   BOOL                       IsSuspend;
   BOOL                       IsActiveOff;
   BOOL                       IsRGBDefaultColorSpace; /* RGB standard default color space as its primary color space */
   BOOL                       IsPreffredTimingMode;    /* The display uses the sRGB standard default color space
                                                       * as its primary color space */
   BOOL                       IsDefaultGTFSupported;
   STHDMI_EDIDDisplayType_t   DisplayType;
}STHDMI_FeatureSupported_t;

typedef struct STHDMI_EDIDDisplayParams_s
{
    U32                        MaxHorImageSize;
    U32                        MaxVerImageSize;
    U32                        DisplayGamma;
    STHDMI_VideoInputParams_t  VideoInput;
    STHDMI_FeatureSupported_t  FeatureSupport;
}STHDMI_EDIDDisplayParams_t;

typedef struct STHDMI_EDIDColorParams_s
{
    U32 Red_x;
    U32 Red_y;
    U32 Green_x;
    U32 Green_y;
    U32 Blue_x;
    U32 Blue_y;
    U32 White_x;
    U32 White_y;

}STHDMI_EDIDColorParams_t;

typedef struct STHDMI_EDIDEstablishedTiming_s
{
    STHDMI_EDIDEstablishedTimingI_t   Timing1;
    STHDMI_EDIDEstablishedTimingII_t  Timing2;
    BOOL                 IsManTimingSupported;

}STHDMI_EDIDEstablishedTiming_t;

typedef struct STHDMI_EDIDStructure_s
{
   U8                              EDIDHeader[8];
   U32                             EDIDExtensionFlag;
   U32                             EDIDChecksum;
   STHDMI_EDIDProdDescription_t    EDIDProdDescription;
   STHDMI_EDIDStructInfos_t        EDIDInfos;
   STHDMI_EDIDDisplayParams_t      EDIDDisplayParams;
   STHDMI_EDIDColorParams_t        EDIDColorParams;
   STHDMI_EDIDEstablishedTiming_t  EDIDEstablishedTiming;
   STHDMI_EDIDStandardTiming_t     EDIDStdTiming[8];
   STHDMI_EDIDTimingDescriptor_t   EDIDDetailedDesc[4];
}STHDMI_EDIDStructure_t;

typedef struct STHDMI_EDIDSink_s
{
    STHDMI_EDIDStructure_t          EDIDBasic;
    STHDMI_EDIDBlockExtension_t*    EDIDExtension_p;
}STHDMI_EDIDSink_t;


typedef U32 STHDMI_Handle_t;

typedef struct STHDMI_OnChip_s
{
    ST_DeviceName_t             VTGDeviceName;
    ST_DeviceName_t             VOUTDeviceName;
    ST_DeviceName_t             EvtDeviceName;
}STHDMI_OnChip_t;


typedef struct STHDMI_InitParams_s
{
    STHDMI_DeviceType_t         DeviceType;
    ST_Partition_t*             CPUPartition_p;
    U32                         MaxOpen;
    union
    {
        STHDMI_OnChip_t         OnChipHdmiCell;
        /* TBD... reserved for future use */   /* for target on selicon image chip(e.g : SIL9190) */
    }Target;

    STVOUT_OutputType_t            OutputType;
    STHDMI_AVIInfoFrameFormat_t    AVIType;
    STHDMI_SPDInfoFrameFormat_t    SPDType;
    STHDMI_MSInfoFrameFormat_t     MSType;
    STHDMI_AUDIOInfoFrameFormat_t  AUDIOType;
    STHDMI_VSInfoFrameFormat_t     VSType;
}STHDMI_InitParams_t;

typedef struct STHDMI_OpenParams_s
{
    U8 NotUsed;
}STHDMI_OpenParams_t;

typedef struct STHDMI_TermParams_s
{
    BOOL                       ForceTerminate;
}STHDMI_TermParams_t;

typedef struct STHDMI_Capability_s
{
    STVOUT_OutputType_t        SupportedOutputs;
    BOOL                       IsEDIDDataSupported;
    BOOL                       IsDataIslandSupported;
    BOOL                       IsVideoSupported;
    BOOL                       IsAudioSupported;
}STHDMI_Capability_t;

typedef struct STHDMI_SinkCapability_s
{
    U32                         EDIDBasicVersion;
    U32                         EDIDBasicRevision;
    U32                         EDIDExtRevision;
    BOOL                        IsBasicEDIDSupported;
    BOOL                        IsAdditionalDataSupported;
    BOOL                        IsLCDTimingDataSupported;
    BOOL                        IsColorInfoSupported;
    BOOL                        IsDviDataSupported;
    BOOL                        IsTouchScreenDataSupported;
    BOOL                        IsBlockMapSupported;

}STHDMI_SinkCapability_t;

typedef struct STHDMI_SourceCapability_s
{
   STHDMI_AVIInfoFrameFormat_t    AVIInfoFrameSupported;
   STHDMI_AVIInfoFrameFormat_t    AVIInfoFrameSelected;
   STHDMI_SPDInfoFrameFormat_t    SPDInfoFrameSupported;
   STHDMI_SPDInfoFrameFormat_t    SPDInfoFrameSelected;
   STHDMI_MSInfoFrameFormat_t     MSInfoFrameSupported;
   STHDMI_MSInfoFrameFormat_t     MSInfoFrameSelected;
   STHDMI_AUDIOInfoFrameFormat_t  AudioInfoFrameSupported;
   STHDMI_AUDIOInfoFrameFormat_t  AudioInfoFrameSelected;
   STHDMI_VSInfoFrameFormat_t     VSInfoFrameSupported;
   STHDMI_VSInfoFrameFormat_t     VSInfoFrameSelected;
}STHDMI_SourceCapability_t;

typedef struct STHDMI_OutputWindows_s
{
   S32  OutputWinX;
   S32  OutputWinY;
   U32  OutputWinWidth;
   U32  OutputWinHeight;
}STHDMI_OutputWindows_t;


    typedef struct STHDMI_CEC_Data_s
    {
        U8      Data[14];
        U8      DataLength;
    }STHDMI_CEC_Data_t;

    typedef struct STHDMI_CEC_VendorCommandWithID_s
    {
        U32                  VendorID;
        STHDMI_CEC_Data_t    VendorSpecificData;
    }STHDMI_CEC_VendorCommandWithID_t;


        typedef union STHDMI_CEC_AudioVolumeStatus_u
        {
            U8                                      KnownAudioVolumeStatus;
            STHDMI_CEC_AudioVolumeStatus_Unknown_t  UnknownAudioVolumeStatus;
        }STHDMI_CEC_AudioVolumeStatus_t;

   typedef struct STHDMI_CEC_AudioStatus_s
    {
        STHDMI_CEC_AudioMuteStatus_t   AudioMuteStatus;
        STHDMI_CEC_AudioVolumeStatus_t AudioVolumeStatus; /* 0x00 <= n <= 0x64 Compare Mask - error report */
    }STHDMI_CEC_AudioStatus_t;                              /* 0x65 <= n <= 0x7e : reserved Compare Mask - error report */
                                                               /* 0x7f : unknown Audio volume status - error report */

   typedef struct STHDMI_CEC_ChannelIdentifier_s
   {
        STHDMI_CEC_ChannelNumberFormat_t  ChannelNumberFormat;
        U16                               MajorChannelNumber; /*Mask 10 bits - Error report if the 3 digits overtake 10 bits */
        U16                               MinorChannelNumber;
   }STHDMI_CEC_ChannelIdentifier_t;

            typedef struct STHDMI_CEC_ARIB_DVBData_s
            {
                    U16    TransportStreamID ;
                    U16    ServiceID;
                    U16    OriginalNetworkID;
            }STHDMI_CEC_ARIB_DVBData_t;

            typedef struct STHDMI_CEC_ATSCData_s
            {
                    U16    TransportStreamID ;
                    U16    ProgramNumber;
            }STHDMI_CEC_ATSCData_t;

        typedef union STHDMI_CEC_ServiceIdentification_u
        {
                STHDMI_CEC_ARIB_DVBData_t       ARIBData ;
                STHDMI_CEC_ATSCData_t           ATSCData;
                STHDMI_CEC_ARIB_DVBData_t       DVBData;
                STHDMI_CEC_ChannelIdentifier_t  ChannelData;
        }STHDMI_CEC_ServiceIdentification_t;

   typedef struct STHDMI_CEC_AnalogueServiceIdentification_s
   {
        STHDMI_CEC_AnalogueBroadcastType_t   AnalogueBroadcastType;
        U16                                  AnalogueFrequency;
        STHDMI_CEC_BroadcastSystem_t         BroadcastSystem;
   }STHDMI_CEC_AnalogueServiceIdentification_t;

   typedef struct STHDMI_CEC_DigitalServiceIdentification_s
   {
        STHDMI_CEC_ServiceIdentificationMethod_t ServiceIdentificationMethod;
        STHDMI_CEC_DigitalBroadcastSystem_t      DigitalBroadcastSystem;
        STHDMI_CEC_ServiceIdentification_t       ServiceIdentification;
   }STHDMI_CEC_DigitalServiceIdentification_t;

  typedef struct STHDMI_CEC_RoutingChange_s
  {
        STHDMI_CEC_PhysicalAddress_t     OriginalAddress;
        STHDMI_CEC_PhysicalAddress_t     NewAddress;
  }STHDMI_CEC_RoutingChange_t;

  typedef struct STHDMI_CEC_ReportPhysicalAddress_s
  {
        STHDMI_CEC_PhysicalAddress_t     PhysicalAddress;
        STHDMI_CEC_DeviceType_t          DeviceType;
  }STHDMI_CEC_ReportPhysicalAddress_t;

  typedef STHDMI_CEC_Opcode_t STHDMI_CEC_FeatureOpcode_t;

  typedef struct STHDMI_CEC_FeatureAbort_s
  {
        STHDMI_CEC_FeatureOpcode_t       FeatureOpcode;
        STHDMI_CEC_AbortReason_t         AbortReason;
  }STHDMI_CEC_FeatureAbort_t;


    typedef union STHDMI_CEC_UIC_Operand_u
    {
        STHDMI_CEC_PlayMode_t            PlayMode;
        STHDMI_CEC_ChannelIdentifier_t   ChannelIdentifier;
        U8                               UIFunctionMedia;
        U8                               UIFunctionSelectAVInput;
        U8                               UIFunctionSelectAudioInput;
    }STHDMI_CEC_UIC_Operand_t;


  typedef struct STHDMI_CEC_UICommand_s /*User Interface Command */
  {
        BOOL                            UseAdditionalOperands;
        STHDMI_CEC_UIC_Operand_t        Operand;
        STHDMI_CEC_UICommand_Opcode_t   Opcode;
  }STHDMI_CEC_UICommand_t; /*User Interface Command */



        typedef struct STHDMI_CEC_AnalogueService_s
        {
            STHDMI_CEC_RecordSourceType_t       RecordSourceType;
            STHDMI_CEC_AnalogueBroadcastType_t  AnalogueBroadcastType;
            U16                                 AnalogueFrequency;
            STHDMI_CEC_BroadcastSystem_t        BroadcastSystem;
        }STHDMI_CEC_AnalogueService_t;

        typedef struct STHDMI_CEC_DigitalService_s
        {
            STHDMI_CEC_RecordSourceType_t               RecordSourceType;
            STHDMI_CEC_DigitalServiceIdentification_t   DigitalServiceIdentification;
        }STHDMI_CEC_DigitalService_t;

        typedef struct STHDMI_CEC_ExternalPlug_s
        {
            STHDMI_CEC_RecordSourceType_t             RecordSourceType;
            U8                                        ExternalPlug;
        }STHDMI_CEC_ExternalPlug_t;

        typedef struct STHDMI_CEC_ExternalPhysicalAddress_s
        {
            STHDMI_CEC_RecordSourceType_t             RecordSourceType;
            STHDMI_CEC_PhysicalAddress_t              ExternalPhysicalAddress;
        }STHDMI_CEC_ExternalPhysicalAddress_t;


  typedef union STHDMI_CEC_RecordSource_u
  {
         STHDMI_CEC_RecordSourceType_t             OwnSource;
         STHDMI_CEC_DigitalService_t               DigitalService;
         STHDMI_CEC_AnalogueService_t              AnalogueService;
         STHDMI_CEC_ExternalPlug_t                 ExternalPlug;
         STHDMI_CEC_ExternalPhysicalAddress_t      ExternalPhysicalAddress;
  }STHDMI_CEC_RecordSource_t;



  typedef struct STHDMI_CEC_Time_s
  {
        U8                  Hour;   /* 00 <= n <= 23 Decimal NOT BCD Compare Mask - error report */
        U8                  Minute; /* 00 <= n <= 59 Decimal NOT BCD Compare Mask - error report */
  }STHDMI_CEC_Time_t;

  typedef struct STHDMI_CEC_Duration_s
  {
        U8                  DurationHour;   /* 00 <= n <= 99 Decimal NOT BCD Compare Mask - error report */
        U8                  Minute;         /* 00 <= n <= 59 Decimal NOT BCD Compare Mask - error report */
  }STHDMI_CEC_Duration_t;

  typedef struct STHDMI_CEC_SetAnalogueTimer_s
  {
        U8                                           DayOfMonth;                /* 00 <= n <= 31 Decimal  Compare Mask - error report */
        U8                                           MonthOfYear;               /* 00 <= n <= 12 Decimal  Compare Mask - error report */
        STHDMI_CEC_Time_t                            StartTime;
        STHDMI_CEC_Duration_t                        Duration;                 /* 00 <= n <= 99 Decimal NOT BCD Compare Mask - error report */
        STHDMI_CEC_RecordingSequence_t               RecordingSequence;
        STHDMI_CEC_AnalogueBroadcastType_t           AnalogueBroadcastType;
        U16                                          AnalogueFrequency;
        STHDMI_CEC_BroadcastSystem_t                 BroadcastSystem;
  }STHDMI_CEC_SetAnalogueTimer_t;

  typedef struct STHDMI_CEC_SetDigitalTimer_s
  {
        U8                                           DayOfMonth;                /* 00 <= n <= 31 Decimal  Compare Mask - error report */
        U8                                           MonthOfYear;               /* 00 <= n <= 12 Decimal  Compare Mask - error report */
        STHDMI_CEC_Time_t                            StartTime;
        STHDMI_CEC_Duration_t                        Duration;              /* 00 <= n <= 99 Decimal NOT BCD Compare Mask - error report */
        U8                                           RecordingSequence;
        STHDMI_CEC_DigitalServiceIdentification_t    DigitalServiceIdentification;
  }STHDMI_CEC_SetDigitalTimer_t;

    typedef union STHDMI_CEC_ExternalPlugType_u
    {
        U8                                           ExternalPlug;
        STHDMI_CEC_PhysicalAddress_t                 ExternalPhysicalAddress;
    }STHDMI_CEC_ExternalPlugType_t;

  typedef struct STHDMI_CEC_SetExternalTimer_s
  {
        U8                                        DayOfMonth;                /* 00 <= n <= 31 Decimal  Compare Mask - error report */
        U8                                        MonthOfYear;               /* 00 <= n <= 12 Decimal  Compare Mask - error report */
        STHDMI_CEC_Time_t                         StartTime;
        STHDMI_CEC_Duration_t                     Duration;              /* 00 <= n <= 99 Decimal NOT BCD Compare Mask - error report */
        U8                                        RecordingSequence;
        STHDMI_CEC_ExternalSourceSpecifier_t      ExternalSourceSpecifier;
        STHDMI_CEC_ExternalPlugType_t             ExternalPlugType;
  }STHDMI_CEC_SetExternalTimer_t;


            typedef union STHDMI_CEC_ProgrammedInfoType_u
            {
                STHDMI_CEC_ProgrammedInfo_t          ProgrammedInfo;
                STHDMI_CEC_NotProgrammedErrorInfo_t  NotProgrammedErrorInfo;
            }STHDMI_CEC_ProgrammedInfoType_t;

        typedef struct STHDMI_CEC_TimerProgrammedInfo_s
        {
            STHDMI_CEC_ProgrammedIndicator_t   ProgrammedIndicator;
            STHDMI_CEC_ProgrammedInfoType_t    ProgrammedInfoType;
            BOOL                               IsDurationAvailable;
            STHDMI_CEC_Duration_t              Duration;
        }STHDMI_CEC_TimerProgrammedInfo_t;

  typedef struct STHDMI_CEC_TimerStatusData_s
  {
        STHDMI_CEC_TimerOverlapWarning_t      TimerOverlapWarning;
        STHDMI_CEC_MediaInfo_t                MediaInfo;
        STHDMI_CEC_TimerProgrammedInfo_t      TimerProgrammedInfo;
  }STHDMI_CEC_TimerStatusData_t;

  typedef struct STHDMI_CEC_SetOSDString_s
  {
        STHDMI_CEC_DisplayControl_t   DisplayControl;
        char                          OSDString[13];                /* null terminated string or max 13 */
  }STHDMI_CEC_SetOSDString_t;


        typedef union STHDMI_CEC_A_DSystem_u
        {
                STHDMI_CEC_AnalogueService_t              AnalogueSystem;
                STHDMI_CEC_DigitalServiceIdentification_t    DigitalSystem;
        }STHDMI_CEC_A_DSystem_t;

  typedef struct STHDMI_CEC_TunerDeviceInfo_s
  {
        STHDMI_CEC_RecordingFlag_t    RecordingFlag;
        STHDMI_CEC_TunerDisplayInfo_t TunerDisplayInfo;
        STHDMI_CEC_A_DSystem_t        A_DSystem;
  }STHDMI_CEC_TunerDeviceInfo_t;

typedef union STHDMI_CEC_Operand_u
{
  STHDMI_CEC_AbortReason_t                   AbortReason;
  char                                       ASCIIdigit;             /* 0x30 <= n <= 0x39 Compare Mask - error report */
  char                                       ASCII;                  /* 0x20 <= n <= 0x7F Compare Mask - error report */
  char                                       ProgramTitleString[14];     /* null terminated string or max 14 */
  char                                       OSDName[14];                /* null terminated string or max 14 */
  char                                       OSDString[13];              /* null terminated string or max 13 */
  char                                       Language[3];
  BOOL                                       Boolean;
  U32                                        VendorID;
  STHDMI_CEC_Data_t                          VendorSpecificData;
  STHDMI_CEC_Data_t                          VendorSpecificRCCode;
  STHDMI_CEC_VendorCommandWithID_t           VendorCommandWithID;
  U16                                        AnalogueFrequency;      /* Analogue frequency parameter : 0x0000 <= n <= 0xFFFF Compare Mask - error report */
  STHDMI_CEC_AudioRate_t                     AudioRate;              /* Wide Range Control IEEE 1394 compatible + Narrow Range Control (HDMI transparent)*/
  STHDMI_CEC_AudioStatus_t                   AudioStatus;
  STHDMI_CEC_SystemAudioStatus_t             SystemAudioStatus;
  STHDMI_CEC_BroadcastSystem_t               BroadcastSystem;        /* 0x00 <= n <= 0x1F Compare Mask - error report */
  STHDMI_CEC_CECVersion_t                    CECVersion;
  STHDMI_CEC_ChannelIdentifier_t             ChannelIdentifier;
  U8                                         DayOfMonth;             /* 0x00 <= n <= 0x1F Compare Mask - error report */
  STHDMI_CEC_DeckControlMode_t               DeckControlMode;
  STHDMI_CEC_DeckInfo_t                      DeckInfo;
  STHDMI_CEC_DeviceType_t                    DeviceType;
  STHDMI_CEC_PhysicalAddress_t               PhysicalAddress;
  STHDMI_CEC_PhysicalAddress_t               NewAddress;
  STHDMI_CEC_PhysicalAddress_t               OriginalAddress;
  STHDMI_CEC_PhysicalAddress_t               ExternalPhysicalAddress;
  STHDMI_CEC_RoutingChange_t                 RoutingChange;
  STHDMI_CEC_ReportPhysicalAddress_t         ReportPhysicalAddress;
  STHDMI_CEC_PowerStatus_t                   PowerStatus;
  STHDMI_CEC_FeatureAbort_t                  FeatureAbort;
  STHDMI_CEC_UICommand_t                     UICommand; /*User Interface Command */
  STHDMI_CEC_MenuState_t                     MenuState;
  STHDMI_CEC_MenuRequestType_t               MenuRequestType;
  STHDMI_CEC_RecordSource_t                  RecordSource;
  STHDMI_CEC_RecordStatusInfo_t              RecordStatusInfo;
  STHDMI_CEC_Time_t                          Time;
  STHDMI_CEC_TimerStatusData_t               TimerStatusData;
  STHDMI_CEC_SetAnalogueTimer_t              SetAnalogueTimer;
  STHDMI_CEC_SetDigitalTimer_t               SetDigitalTimer;
  STHDMI_CEC_SetExternalTimer_t              SetExternalTimer;
  U8                                         ExternalPlug;
  STHDMI_CEC_SetOSDString_t                  SetOSDString;
  STHDMI_CEC_StatusRequest_t                 StatusRequest;
  STHDMI_CEC_PlayMode_t                      PlayMode;
  STHDMI_CEC_TunerDeviceInfo_t               TunerDeviceInfo;
  STHDMI_CEC_AnalogueBroadcastType_t         AnalogueBroadcastType;
  STHDMI_CEC_AnalogueServiceIdentification_t AnalogueServiceIdentification;
  STHDMI_CEC_DigitalServiceIdentification_t  DigitalServiceIdentification;
  STHDMI_CEC_ExternalSourceSpecifier_t       ExternalSourceSpecifier; /* 0x00 <= n <= 0xFF Compare Mask - error report */
  STHDMI_CEC_TimerClearedStatusData_t        TimerClearedStatusData;
  STHDMI_CEC_FeatureOpcode_t                 FeatureOpcode;
}STHDMI_CEC_Operand_t;

typedef struct STHDMI_CEC_Command_s
{
  U8                                Retries;
  U8                                Source;
  U8                                Destination;
  STHDMI_CEC_Operand_t              CEC_Operand;
  STHDMI_CEC_Opcode_t               CEC_Opcode;
}STHDMI_CEC_Command_t;

typedef struct STHDMI_CEC_CommandInfo_s
{
    STVOUT_CECStatus_t      Status;
    STHDMI_CEC_Command_t    Command;
}STHDMI_CEC_CommandInfo_t;


/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

ST_Revision_t  STHDMI_GetRevision (
                void
                    );


ST_ErrorCode_t STHDMI_GetCapability (
                const ST_DeviceName_t               DeviceName,
                STHDMI_Capability_t*                const Capability_p
                );

ST_ErrorCode_t STHDMI_GetSourceCapability (
                const ST_DeviceName_t               DeviceName,
                STHDMI_SourceCapability_t*          const Capability_p
                );


ST_ErrorCode_t STHDMI_GetSinkCapability (
                const ST_DeviceName_t               DeviceName,
                STHDMI_SinkCapability_t*            const Capability_p
                );


ST_ErrorCode_t STHDMI_Init (
                const ST_DeviceName_t               DeviceName,
                const STHDMI_InitParams_t*          const InitParams_p
                );

ST_ErrorCode_t STHDMI_Open (
                const ST_DeviceName_t               DeviceName,
                const STHDMI_OpenParams_t*          const OpenParams_p,
                STHDMI_Handle_t*                    const Handle_p
                );

ST_ErrorCode_t STHDMI_Close (
                const STHDMI_Handle_t               Handle
                );

ST_ErrorCode_t STHDMI_Term (
                const ST_DeviceName_t               DeviceName,
                const STHDMI_TermParams_t*          const TermParams_p
                );


ST_ErrorCode_t STHDMI_FillAVIInfoFrame(
               const STHDMI_Handle_t                Handle,
               STHDMI_AVIInfoFrame_t *              const  AVIBuffer_p
               );

ST_ErrorCode_t STHDMI_FillSPDInfoFrame(
               const STHDMI_Handle_t                Handle,
               STHDMI_SPDInfoFrame_t *              const  SPDBuffer_p
               );

ST_ErrorCode_t STHDMI_FillAudioInfoFrame(
               const STHDMI_Handle_t                Handle,
               STHDMI_AUDIOInfoFrame_t*             const AudioBuffer_p
               );


ST_ErrorCode_t STHDMI_FillMSInfoFrame(
               const STHDMI_Handle_t                Handle,
               STHDMI_MPEGSourceInfoFrame_t*        const   MSBuffer_p
               );

ST_ErrorCode_t STHDMI_FillVSInfoFrame(
               const STHDMI_Handle_t                 Handle,
               STHDMI_VendorSpecInfoFrame_t*         const  VSBuffer_p
               );

ST_ErrorCode_t STHDMI_FillSinkEDID(
               const STHDMI_Handle_t                 Handle,
               STHDMI_EDIDSink_t*                    const   EdidBuffer_p
               );
ST_ErrorCode_t STHDMI_SetOutputWindows(
               const STHDMI_Handle_t                 Handle,
               const STHDMI_OutputWindows_t*         const   OutputWindows_p
               );

ST_ErrorCode_t  STHDMI_FreeEDIDMemory (
                const STHDMI_Handle_t                 Handle
                );


ST_ErrorCode_t STHDMI_CECSendCommand   (
                const STHDMI_Handle_t                 Handle,
                STHDMI_CEC_Command_t*                 const CEC_Command_p
                );

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STHDMI_H */
/* End of sthdmi.h */






