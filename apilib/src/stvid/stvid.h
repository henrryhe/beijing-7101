/*******************************************************************************
File Name   : stvid.h

Description : Video driver header file

Copyright (C) 2007 STMicroelectronics

*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVID_H
#define __STVID_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stos.h"

#ifdef ST_OSLINUX
#include "stdevice.h"
#elif !defined(ST_OSWINCE)
#include "stlite.h"
#endif

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"
#endif /* def STVID_STVAPI_ARCHITECTURE */

#include "stevt.h"

#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* !!! temporary definition, will be added later to common */
typedef U32 STTS_t;


/* Exported Constants ------------------------------------------------------- */

#define STVID_DRIVER_ID       21
#define STVID_DRIVER_BASE     (STVID_DRIVER_ID << 16)

#define STVID_IGNORE_ID       0xFF

#define STVID_OPTIMISED_NB_FRAME_STORE_ID  80u
#define STVID_VARIABLE_IN_FIXED_SIZE_NB_FRAME_STORE_ID 81u
#define STVID_VARIABLE_IN_FULL_PARTITION_NB_FRAME_STORE_ID 82u
#define STVID_MAX_NUMBER_OF_PAN_AND_SCAN 3
enum
{
    STVID_ERROR_DECODER_RUNNING = STVID_DRIVER_BASE,
    STVID_ERROR_DECODER_RUNNING_IN_RT_MODE,
    STVID_ERROR_DECODER_PAUSING,
    STVID_ERROR_DECODER_STOPPED,
    STVID_ERROR_DECODER_NOT_PAUSING,
    STVID_ERROR_NOT_AVAILABLE,
    STVID_ERROR_DECODER_FREEZING,
    STVID_ERROR_EVENT_REGISTRATION,
    STVID_ERROR_SYSTEM_CLOCK,
    STVID_ERROR_IMPOSSIBLE_WITH_MEM_PROFILE,
    STVID_ERROR_MEMORY_ACCESS

    /* Add new error codes at last place, until numeric values are removed from tests scripts */
};

enum
{
    /* This event passes a (STVID_DisplayAspectRatio_t) as parameter */
    STVID_ASPECT_RATIO_CHANGE_EVT = STVID_DRIVER_BASE,
    /* This event passes no parameter */
    STVID_BACK_TO_SYNC_EVT,
    /* This event passes no parameter */
    STVID_DATA_ERROR_EVT,
    /* This event passes no parameter */
    STVID_DATA_OVERFLOW_EVT,
    /* This event passes a (STVID_DataUnderflow_t *) as parameter */
    STVID_DATA_UNDERFLOW_EVT,
    /* This event passes a (STVID_DigitalInputWindows_t *) as parameter */
    STVID_DIGINPUT_WIN_CHANGE_EVT,
    /* This event passes a (STVID_PictureParams_t *) as parameter */
    STVID_DISPLAY_NEW_FRAME_EVT,
    /* This event passes a (STVID_PictureParams_t *) as parameter */
    STVID_FRAME_RATE_CHANGE_EVT,
    /* This event passes a (STVID_MemoryProfile_t *) as parameter */
    STVID_IMPOSSIBLE_WITH_MEM_PROFILE_EVT,
    /* This event passes a (STVID_PictureInfos_t *) as parameter */
    STVID_NEW_PICTURE_TO_BE_DISPLAYED_EVT,
    /* This event passes a (STVID_PictureInfos_t *) as parameter */
    STVID_NEW_PICTURE_DECODED_EVT,
    /* This event passes no parameter */
    STVID_OUT_OF_SYNC_EVT,
    /* This event passes a (STVID_PictureParams_t *) as parameter */
    STVID_PICTURE_DECODING_ERROR_EVT,
    /* This event passes a (STVID_PictureParams_t *) as parameter */
    STVID_RESOLUTION_CHANGE_EVT,
    /* This event passes a (STVID_PictureParams_t *) as parameter */
    STVID_SCAN_TYPE_CHANGE_EVT,
    /* This event passes a (STVID_SequenceInfo_t *) as parameter */
    STVID_SEQUENCE_INFO_EVT,
    /* This event passes a (STVID_SpeedDriftThreshold_t *) as parameter */
    STVID_SPEED_DRIFT_THRESHOLD_EVT,
    /* This event passes a (STVID_PictureParams_t *) as parameter */
    STVID_STOPPED_EVT,
    /* This event passes a (STVID_SynchroInfo_t *) as parameter */
    STVID_SYNCHRO_EVT,
    /* This event passes a (STVID_UserData_t *) as parameter */
    STVID_USER_DATA_EVT,
    /* This event passes a (STVID_SynchronisationInfo_t *) as parameter */
    STVID_SYNCHRONISATION_CHECK_EVT
#ifdef STVID_HARDWARE_ERROR_EVENT
    , /* This is an un-documented event to inform the test application of the
    hardware errors, so that it doesn't have bad conclusions concerning the test failure.
    This event exports a STVID_HardwareError_t data. */
    STVID_HARDWARE_ERROR_EVT /* Injection by application can start again after the notification of this event */
#endif /* STVID_HARDWARE_ERROR_EVENT */
    /* This event passes a (STVID_ProvidedDataToBeInjected_t *) as parameter */
    , STVID_PROVIDED_DATA_TO_BE_INJECTED_EVT,
    /* This event passes no parameter */
    STVID_SEQUENCE_END_CODE_EVT,
    /* This event passes a (STVID_PictureInfos_t *) as parameter */
    STVID_NEW_PICTURE_ORDERED_EVT,
    /* This event passes a (STVID_ParsedData_t *) as parameter */
    STVID_PARSED_DATA_EVT,
    /* This event passes a (STVID_CRCCheckResult_t *) as parameter */
    STVID_END_OF_CRC_CHECK_EVT,  /* Debug only */
     /* This event passes no parameter */
    STVID_NEW_CRC_QUEUED_EVT,
    /* This event passes a (STVID_FieldInfos_t *) as parameter */
    STVID_NEW_FIELD_TO_BE_DISPLAYED_EVT
};

typedef enum STVID_BroadcastProfile_e
{
    STVID_BROADCAST_DVB     = 1,
    STVID_BROADCAST_DIRECTV = 2,
    STVID_BROADCAST_ATSC    = 4,
    STVID_BROADCAST_DVD     = 8,
    STVID_BROADCAST_ARIB    = 16
} STVID_BroadcastProfile_t;

typedef enum STVID_Clear_e
{
    STVID_CLEAR_DISPLAY_BLACK_FILL            = 1,
    STVID_CLEAR_DISPLAY_PATTERN_FILL          = 2,
    STVID_CLEAR_FREEING_DISPLAY_FRAME_BUFFER  = 4
} STVID_Clear_t;

typedef enum STVID_CodingMode_e
{
    STVID_CODING_MODE_MB    = 1
} STVID_CodingMode_t;

typedef enum STVID_ColorType_e
{
    STVID_COLOR_TYPE_YUV420 = 1,
    STVID_COLOR_TYPE_YUV422 = 2,
    STVID_COLOR_TYPE_YUV444 = 4
} STVID_ColorType_t;

typedef enum STVID_CompressionLevel_e
{
    STVID_COMPRESSION_LEVEL_NONE    = 1,
    STVID_COMPRESSION_LEVEL_1       = 2,
    STVID_COMPRESSION_LEVEL_2       = 4
} STVID_CompressionLevel_t;

typedef enum STVID_DecimationFactor_e
{
    STVID_DECIMATION_FACTOR_NONE    = 0,
    STVID_DECIMATION_FACTOR_H2      = 1,
    STVID_DECIMATION_FACTOR_V2      = 2,
    STVID_DECIMATION_FACTOR_H4      = 4,
    STVID_DECIMATION_FACTOR_V4      = 8,
    STVID_DECIMATION_FACTOR_H4_ADAPTATIVE_H2 = 16,
    STVID_DECIMATION_FACTOR_2       = (STVID_DECIMATION_FACTOR_H2 | STVID_DECIMATION_FACTOR_V2),
    STVID_DECIMATION_FACTOR_4       = (STVID_DECIMATION_FACTOR_H4 | STVID_DECIMATION_FACTOR_V4),
    STVID_MPEG_DEBLOCK_RECONSTRUCTION = 32,
    STVID_POSTPROCESS_RECONSTRUCTION = 64
} STVID_DecimationFactor_t;

typedef enum STVID_DecodedPictures_e
{
    STVID_DECODED_PICTURES_ALL,
    STVID_DECODED_PICTURES_IP,
    STVID_DECODED_PICTURES_I,
    STVID_DECODED_PICTURES_FIRST_I
} STVID_DecodedPictures_t;

typedef enum STVID_DeviceType_e
{
    STVID_DEVICE_TYPE_5508_MPEG,
    STVID_DEVICE_TYPE_5510_MPEG,
    STVID_DEVICE_TYPE_5512_MPEG,
    STVID_DEVICE_TYPE_5514_MPEG,
    STVID_DEVICE_TYPE_5516_MPEG,
    STVID_DEVICE_TYPE_5517_MPEG,
    STVID_DEVICE_TYPE_5518_MPEG,
    STVID_DEVICE_TYPE_7015_MPEG,
    STVID_DEVICE_TYPE_7015_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_7020_MPEG,
    STVID_DEVICE_TYPE_7020_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_GENERIC_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_5528_MPEG,
    STVID_DEVICE_TYPE_5100_MPEG,
    STVID_DEVICE_TYPE_5525_MPEG,
    STVID_DEVICE_TYPE_7100_MPEG,
    STVID_DEVICE_TYPE_7100_H264,
    STVID_DEVICE_TYPE_7100_MPEG4P2,
    STVID_DEVICE_TYPE_7100_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_7109_MPEG,
    STVID_DEVICE_TYPE_7109D_MPEG, /* test mode for stvid driver unitary tests only */
    STVID_DEVICE_TYPE_7109_H264,
    STVID_DEVICE_TYPE_7109_VC1,
    STVID_DEVICE_TYPE_7109_MPEG4P2,
    STVID_DEVICE_TYPE_7109_AVS,
    STVID_DEVICE_TYPE_7109_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_7200_MPEG,
    STVID_DEVICE_TYPE_7200_H264,
    STVID_DEVICE_TYPE_7200_VC1,
    STVID_DEVICE_TYPE_7200_MPEG4P2,
    STVID_DEVICE_TYPE_7200_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_7710_MPEG,
    STVID_DEVICE_TYPE_7710_DIGITAL_INPUT,
    STVID_DEVICE_TYPE_5105_MPEG,
    STVID_DEVICE_TYPE_5107_MPEG = STVID_DEVICE_TYPE_5105_MPEG, /* Caution that doing this re-affects a value define, so next re-starts with +1 as normal. So those = affectations must follow each other, to prevent 2 other value to be equal although not wanted */
    STVID_DEVICE_TYPE_5188_MPEG = STVID_DEVICE_TYPE_5105_MPEG, /* Caution that doing this re-affects a value define, so next re-starts with +1 as normal. So those = affectations must follow each other, to prevent 2 other value to be equal although not wanted */
	STVID_DEVICE_TYPE_5162_MPEG = STVID_DEVICE_TYPE_5105_MPEG, /* Caution that doing this re-affects a value define, so next re-starts with +1 as normal. So those = affectations must follow each other, to prevent 2 other value to be equal although not wanted */
	STVID_DEVICE_TYPE_STD2000_MPEG,
    STVID_DEVICE_TYPE_5301_MPEG,
    STVID_DEVICE_TYPE_ZEUS_MPEG,
    STVID_DEVICE_TYPE_ZEUS_H264,
    STVID_DEVICE_TYPE_ZEUS_VC1
} STVID_DeviceType_t;

typedef enum STVID_DisplayAspectRatio_e
{
    STVID_DISPLAY_ASPECT_RATIO_16TO9  = 1,
    STVID_DISPLAY_ASPECT_RATIO_4TO3   = 2,
    STVID_DISPLAY_ASPECT_RATIO_221TO1 = 4,
    STVID_DISPLAY_ASPECT_RATIO_SQUARE = 8,
    STVID_DISPLAY_ASPECT_RATIO_EXTENDED_PAR = 16
} STVID_DisplayAspectRatio_t;

typedef enum STVID_DisplayAspectRatioConversion_e
{
    STVID_DISPLAY_AR_CONVERSION_PAN_SCAN   = 1,
    STVID_DISPLAY_AR_CONVERSION_LETTER_BOX = 2,
    STVID_DISPLAY_AR_CONVERSION_COMBINED   = 4,
    STVID_DISPLAY_AR_CONVERSION_IGNORE     = 8
} STVID_DisplayAspectRatioConversion_t;

typedef enum STVID_ErrorRecoveryMode_e
{
    STVID_ERROR_RECOVERY_FULL       = 1,
    STVID_ERROR_RECOVERY_PARTIAL    = 2,
    STVID_ERROR_RECOVERY_NONE       = 4,
    STVID_ERROR_RECOVERY_HIGH       = 8
} STVID_ErrorRecoveryMode_t;

typedef enum STVID_ForceMode_e 
{
    STVID_FORCE_MODE_DISABLED,
    STVID_FORCE_MODE_ENABLED
} STVID_ForceMode_t;

typedef enum STVID_FreezeField_e
{
    STVID_FREEZE_FIELD_TOP     = 1,
    STVID_FREEZE_FIELD_BOTTOM  = 2,
    STVID_FREEZE_FIELD_CURRENT = 4,
    STVID_FREEZE_FIELD_NEXT    = 8
} STVID_FreezeField_t;

typedef enum STVID_FreezeMode_e
{
    STVID_FREEZE_MODE_NONE       = 1,
    STVID_FREEZE_MODE_FORCE      = 2,
    STVID_FREEZE_MODE_NO_FLICKER = 4
} STVID_FreezeMode_t;

typedef enum STVID_MPEGFrame_e
{
    STVID_MPEG_FRAME_I      = 1,
    STVID_MPEG_FRAME_P      = 2,
    STVID_MPEG_FRAME_B      = 4
} STVID_MPEGFrame_t;

typedef enum STVID_MPEGStandard_e
{
    STVID_MPEG_STANDARD_ISO_IEC_11172 = 1, /* MPEG 1 */
    STVID_MPEG_STANDARD_ISO_IEC_13818 = 2, /* MPEG 2 */
    STVID_MPEG_STANDARD_ISO_IEC_14496 = 4, /* MPEG-4 Part 10 - H264 */
    STVID_MPEG_STANDARD_SMPTE_421M    = 8,  /* VC1 */
    STVID_MPEG_STANDARD_ISO_IEC_14496_2 = 16,  /* MPEG-4 Part 2 - DivX */
    STVID_AVS_STANDARD_GB_T_20090_2 = 32  /* China AVS */
} STVID_MPEGStandard_t;

typedef enum STVID_ParsedDataType_e
{
    STVID_PARSED_DATA_TYPE_STARTCODE,
    STVID_PARSED_DATA_TYPE_SERIES_BYTES
} STVID_ParsedDataType_t;

typedef enum STVID_Picture_e
{
    STVID_PICTURE_LAST_DECODED = 1,
    STVID_PICTURE_DISPLAYED    = 2
} STVID_Picture_t;

typedef enum STVID_PictureStructure_e
{
    STVID_PICTURE_STRUCTURE_TOP_FIELD,
    STVID_PICTURE_STRUCTURE_BOTTOM_FIELD,
    STVID_PICTURE_STRUCTURE_FRAME
} STVID_PictureStructure_t;

typedef enum STVID_ScanType_e
{
    STVID_SCAN_TYPE_PROGRESSIVE = 1,
    STVID_SCAN_TYPE_INTERLACED  = 2
} STVID_ScanType_t;

typedef enum STVID_SetupObject_e
{
    STVID_SETUP_FRAME_BUFFERS_PARTITION                 = 0x0001,
    STVID_SETUP_DECODER_INTERMEDIATE_BUFFER_PARTITION   = 0x0002,
    STVID_SETUP_DECIMATED_FRAME_BUFFERS_PARTITION       = 0x0004,
    STVID_SETUP_FDMA_NODES_PARTITION                    = 0x0008,
    STVID_SETUP_PICTURE_PARAMETER_BUFFERS_PARTITION     = 0x0010,
    STVID_SETUP_ES_COPY_BUFFER_PARTITION                = 0x0020,
    STVID_SETUP_ES_COPY_BUFFER                          = 0x0040,
    STVID_SETUP_PARSING_RESULTS_BUFFER_PARTITION        = 0x0080,   /* SCList buffer            */
    STVID_SETUP_DATA_INPUT_BUFFER_PARTITION             = 0x0100,   /* PES buffer partition     */
    STVID_SETUP_BIT_BUFFER_PARTITION                    = 0x0200    /* ES bit buffer partition  */
} STVID_SetupObject_t;

typedef enum STVID_Stop_e
{
    STVID_STOP_WHEN_NEXT_REFERENCE = 1,
    STVID_STOP_WHEN_END_OF_DATA    = 2,
    STVID_STOP_NOW                 = 4,
    STVID_STOP_WHEN_NEXT_I         = 8
} STVID_Stop_t;

typedef enum STVID_StreamType_e
{
    STVID_STREAM_TYPE_ES           = 1,
    STVID_STREAM_TYPE_PES          = 2,
    STVID_STREAM_TYPE_MPEG1_PACKET = 4,
    STVID_STREAM_TYPE_UNCOMPRESSED = 8
} STVID_StreamType_t;

typedef enum STVID_SynchroAction_e
{
    STVID_SYNCHRO_ACTION_PAUSE,
    STVID_SYNCHRO_ACTION_SKIP
} STVID_SynchroAction_t;

typedef enum STVID_UserDataPosition_e
{
    STVID_USER_DATA_AFTER_SEQUENCE,
    STVID_USER_DATA_AFTER_GOP,
    STVID_USER_DATA_AFTER_PICTURE
} STVID_UserDataPosition_t;

typedef enum STVID_WinAlign_e
{
    STVID_WIN_ALIGN_TOP_LEFT        = 1,
    STVID_WIN_ALIGN_VCENTRE_LEFT    = 2,
    STVID_WIN_ALIGN_BOTTOM_LEFT     = 4,
    STVID_WIN_ALIGN_TOP_RIGHT       = 8,
    STVID_WIN_ALIGN_VCENTRE_RIGHT   = 16,
    STVID_WIN_ALIGN_BOTTOM_RIGHT    = 32,
    STVID_WIN_ALIGN_BOTTOM_HCENTRE  = 64,
    STVID_WIN_ALIGN_TOP_HCENTRE     = 128,
    STVID_WIN_ALIGN_VCENTRE_HCENTRE = 256
} STVID_WinAlign_t;

typedef enum STVID_WinSize_e
{
    STVID_WIN_SIZE_FIXED     = 1,
    STVID_WIN_SIZE_DONT_CARE = 2,
    STVID_WIN_SIZE_INCREASE  = 4,
    STVID_WIN_SIZE_DECREASE  = 8
} STVID_WinSize_t;

typedef enum STVID_CodecMode_e
{
    STVID_CODEC_MODE_DECODE    = 0,
    STVID_CODEC_MODE_TRANSCODE = 1
} STVID_CodecMode_t;

/* Exported Types ----------------------------------------------------------- */

typedef struct STVID_ScalingFactors_s
{
    U16 N;
    U16 M;
} STVID_ScalingFactors_t;

typedef struct STVID_ScalingCapability_s
{
    BOOL Continuous;                                       /* TRUE if continuous, FALSE if discreet*/
    U16  NbScalingFactors;                          /* # of elem for in the buffer. 2 if Continuous */
    STVID_ScalingFactors_t *ScalingFactors_p;                     /* Supported scaling, N/M format */
} STVID_ScalingCapability_t;

typedef struct STVID_Capability_s
{
    STVID_BroadcastProfile_t             SupportedBroadcastProfile;
    STVID_CodingMode_t                   SupportedCodingMode;            /* OR of supported ones */
    STVID_ColorType_t                    SupportedColorType;             /* OR of supported ones */
    STVID_CompressionLevel_t             SupportedCompressionLevel;      /* OR of supported ones */
    STVID_DecimationFactor_t             SupportedDecimationFactor;      /* OR of supported ones */
    STVID_DisplayAspectRatioConversion_t SupportedDisplayARConversion;   /* OR of supported ones */
    STVID_DisplayAspectRatio_t           SupportedDisplayAspectRatio;    /* OR of supported ones */
    STVID_ErrorRecoveryMode_t            SupportedErrorRecoveryMode;     /* OR of supported ones */
    STVID_FreezeMode_t                   SupportedFreezeMode;            /* OR of supported ones */
    STVID_FreezeField_t                  SupportedFreezeField;           /* OR of supported ones */
    STVID_Picture_t                      SupportedPicture;               /* OR of supported ones */
    STVID_ScanType_t                     SupportedScreenScanType;        /* OR of supported ones */
    STVID_Stop_t                         SupportedStop;                  /* OR of supported ones */
    STVID_StreamType_t                   SupportedStreamType;            /* OR of supported ones */
    BOOL                                 ProfileCapable;                 /* TRUE if supported */
    BOOL                                 StillPictureCapable;            /* Duplicate/Show/Hide/GetPictureAllocParams
                                                                            function, NonRealTime Decode supported */
    BOOL                                 ManualInputWindowCapable;       /* TRUE if manual       */
    BOOL                                 ManualOutputWindowCapable;      /* windowing supported  */
    BOOL                                 ColorKeyingCapable;             /* TRUE if supported */
    BOOL                                 PSICapable;                     /* TRUE if supported */
    BOOL                                 HDPIPCapable;                   /* TRUE if supported */
    STVID_WinAlign_t                     SupportedWinAlign;              /* OR of supported ones */
    STVID_WinSize_t                      SupportedWinSize;               /* OR of supported ones */
    U8                                   InputWindowHeightMin;           /* value in pixels */
    U8                                   InputWindowWidthMin;            /* value in pixels */
    U8                                   InputWindowPositionXPrecision;  /* value in pixels */
    U8                                   InputWindowPositionYPrecision;  /* value in pixels */
    U8                                   InputWindowWidthPrecision;      /* value in pixels */
    U8                                   InputWindowHeightPrecision;     /* value in pixels */
    U8                                   OutputWindowHeightMin;          /* value in pixels */
    U8                                   OutputWindowWidthMin;           /* value in pixels */
    U8                                   OutputWindowPositionXPrecision; /* value in pixels */
    U8                                   OutputWindowPositionYPrecision; /* value in pixels */
    U8                                   OutputWindowWidthPrecision;     /* value in pixels */
    U8                                   OutputWindowHeightPrecision;    /* value in pixels */
    STVID_ScalingCapability_t            SupportedVerticalScaling;
    STVID_ScalingCapability_t            SupportedHorizontalScaling;
    U8                                   DecoderNumber;
    STVID_SetupObject_t                  SupportedSetupObject;

} STVID_Capability_t;

typedef struct STVID_ConfigureEventParams_s
{
    BOOL    Enable;
    U32     NotificationsToSkip;
} STVID_ConfigureEventParams_t;

typedef struct STVID_ClearParams_s
{
    STVID_Clear_t ClearMode;
    void * PatternAddress1_p;
    U32 PatternSize1;
    void * PatternAddress2_p;
    U32 PatternSize2;
} STVID_ClearParams_t;

typedef struct STVID_DataInjectionCompletedParams_s
{
    BOOL TransferRelatedToPrevious;
} STVID_DataInjectionCompletedParams_t;

typedef struct STVID_DataUnderflow_s
{
    U32     BitBufferFreeSize;
    U32     BitRateValue;          /* in units of 400 bits/second */
    S32     RequiredTimeJump;
    U32     RequiredDuration;
    BOOL    TransferRelatedToPrevious;
} STVID_DataUnderflow_t;

typedef struct STVID_DigitalInputWindows_s
{
    STGXOBJ_Rectangle_t InputRectangle;  /* x, y, w, h         */
    U32                 OutputWidth;     /* bufferised x=0,y=0 */
    U32                 OutputHeight;    /* (implicit)         */
} STVID_DigitalInputWindows_t;

typedef struct STVID_ForceDecimationFactorParams_s 
{
    STVID_ForceMode_t           ForceMode;
    STVID_DecimationFactor_t    DecimationFactor;
} STVID_ForceDecimationFactorParams_t;

typedef struct STVID_Freeze_s
{
    STVID_FreezeMode_t  Mode;
    STVID_FreezeField_t Field;
} STVID_Freeze_t;

typedef struct STVID_HDPIPParams_s{
    BOOL    Enable;
    U32     WidthThreshold;
    U32     HeightThreshold;
} STVID_HDPIPParams_t;

typedef struct STVID_ProvidedDataToBeInjected_s
{
    void *              Address_p;
    U32                 SizeInBytes;
} STVID_ProvidedDataToBeInjected_t;

typedef U32 STVID_Handle_t;

typedef struct STVID_InitParams_s
{
    STVID_DeviceType_t      DeviceType;
    void *                  BaseAddress_p;
    void *                  DeviceBaseAddress_p;
    BOOL                    BitBufferAllocated;
    void *                  BitBufferAddress_p;
    U32                     BitBufferSize;  /* in byte */
    STEVT_EventConstant_t   InterruptEvent;
    ST_DeviceName_t         InterruptEventName;
    BOOL                    InstallVideoInterruptHandler;
    U32                     InterruptNumber;
    U32                     InterruptLevel;
#ifdef STVID_STVAPI_ARCHITECTURE
    U32                     MB2RInterruptNumber;
    U32                     MB2RInterruptLevel;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */
    ST_Partition_t *        CPUPartition_p;
    void *                  SharedMemoryBaseAddress_p;
    STAVMEM_PartitionHandle_t AVMEMPartition;
    U32                     MaxOpen;
    U32                     UserDataSize;
    ST_DeviceName_t         VINName;
    ST_DeviceName_t         EvtHandlerName;
    ST_DeviceName_t         ClockRecoveryName;
    U32                     AVSYNCDriftThreshold;
    void *                  BaseAddress2_p;
    void *                  BaseAddress3_p;
} STVID_InitParams_t;



typedef struct STVID_MemoryProfile_s
{
    U32                         MaxWidth;
    U32                         MaxHeight;
    U8                          NbFrameStore;
    STVID_CompressionLevel_t    CompressionLevel;
    STVID_DecimationFactor_t    DecimationFactor;
    union
    {
        struct
        {
            U8    Main;
            U8    Decimated;
        } OptimisedNumber;
        struct
		{
            U32  Main;
            U32  Decimated;
		} VariableInFixedSize;
        struct
		{
            STAVMEM_PartitionHandle_t Main;
            STAVMEM_PartitionHandle_t Decimated;
		} VariableInFullPartition;
    } FrameStoreIDParams;
} STVID_MemoryProfile_t;

typedef struct STVID_OpenParams_s
{
    U32 SyncDelay;
} STVID_OpenParams_t;

typedef struct STVID_ParsedData_s
{
    STVID_ParsedDataType_t ParsedDataType;
    union
    {
         struct
         {
               U8   Value;
         } StartCode;
         struct
         {
               U32  SeriesSizeInBytes;
         } ByteSeries;
    } WantedData;
    void *  ParsedDataAddress1;
    U32     ParsedDataSize1;
    void *  ParsedDataAddress2;
    U32     ParsedDataSize2;
} STVID_ParsedData_t;

typedef struct STVID_PTS_s
{
    U32  PTS;
    BOOL PTS33;
    BOOL Interpolated;
    BOOL IsValid;
} STVID_PTS_t;

typedef struct STVID_SetupParams_s
{
    STVID_SetupObject_t SetupObject;
    union
    {
        STAVMEM_PartitionHandle_t AVMEMPartition;
        struct
        {
            U32 BufferSize;
            BOOL BufferAllocated;
            void * BufferAddress_p;
        } SingleBuffer;
        struct
        {
            U32 BufferSize;
            BOOL BufferAllocated;
            void * BufferAddress1_p;
            void * BufferAddress2_p;
        } DoubleBuffer;
        struct
        {
            U8 NumberOfBuffers;
            U32 * BufferSizesTable;
            BOOL BufferAllocated;
            void ** BufferAddressesTable;
        } AnyBuffer;
    } SetupSettings;
} STVID_SetupParams_t;

typedef struct STVID_TimeCode_s
{
    U8 Hours;
    U8 Minutes;
    U8 Seconds;
    U8 Frames;
    BOOL Interpolated;
} STVID_TimeCode_t;

typedef struct STVID_VideoParams_s
{
    U32                         FrameRate;
    STGXOBJ_ScanType_t          ScanType;
    STVID_MPEGFrame_t           MPEGFrame;
    STVID_PictureStructure_t    PictureStructure;
    BOOL                        TopFieldFirst;
    STVID_TimeCode_t            TimeCode;
    STVID_PTS_t                 PTS;
    STVID_CompressionLevel_t    CompressionLevel;
    STVID_DecimationFactor_t    DecimationFactors;
    BOOL                        DoesNonDecimatedExist;      /* = TRUE when the non-decimated picture does exist */
    struct
    {
        void *                        Data1_p;
        U32                           Size1;
        void *                        Data2_p;
        U32                           Size2;
    } DecimatedBitmapParams;
#ifdef STVID_USE_CRC /* Debug only */
    U32                         ForceFieldCRC;
    BOOL                        IsMonochrome;
#endif /* STVID_USE_CRC */
} STVID_VideoParams_t;

typedef struct STVID_PictureDescriptors_s
{
    U32     PictureMeanQP;    /* mean of quatisation parameters */
    U32     PictureVarianceQP; /* variance of quatisation parameters */
} STVID_PictureDescriptors_t;

typedef struct STVID_PictureParams_s
{
    STVID_CodingMode_t          CodingMode;
    STVID_ColorType_t           ColorType;
    U32                         FrameRate;
    void *                      Data;
    U32                         Size;
    U32                         Height;
    U32                         Width;
    STVID_DisplayAspectRatio_t  Aspect;
    STVID_ScanType_t            ScanType;
    STVID_MPEGFrame_t           MPEGFrame;
    BOOL                        TopFieldFirst;
    STVID_TimeCode_t            TimeCode;
    STVID_PTS_t                 PTS;

    /* Private Data */
    U32 pChromaOffset;
    S32 pTemporalReference;
} STVID_PictureParams_t;

/* Added to export picture buffer */
typedef void * STVID_PictureBufferHandle_t;

typedef struct STVID_PictureInfos_s
{
    STGXOBJ_Bitmap_t    BitmapParams;
    STVID_VideoParams_t VideoParams;
    STVID_PictureBufferHandle_t PictureBufferHandle;
    STVID_PictureBufferHandle_t DecimatedPictureBufferHandle;
    STVID_PictureDescriptors_t   PictureDescriptors;
} STVID_PictureInfos_t;

typedef struct STVID_FieldInfos_s
{
    BOOL                        DisplayTopNotBottom; /* TRUE if the picture will be displayed in top type */
    STVID_PictureBufferHandle_t PictureBufferHandle; /* Handle to Picture Buffer, to be used by application throw API calls */
} STVID_FieldInfos_t;


typedef struct STVID_DisplayPictureInfos_s
{
    U8 NumberOfPanAndScan;
    struct
    {
        S32     FrameCentreHorizontalOffset;
        S32     FrameCentreVerticalOffset;
        U32     DisplayHorizontalSize;
        U32     DisplayVerticalSize;
        BOOL    HasDisplaySizeRecommendation;
    } PanAndScanIn16thPixel [STVID_MAX_NUMBER_OF_PAN_AND_SCAN];

    struct
    {
        U32     LeftOffset;
        U32     RightOffset;
        U32     TopOffset;
        U32     BottomOffset;
    } FrameCropInPixel;

} STVID_DisplayPictureInfos_t;


#ifdef STVID_STVAPI_ARCHITECTURE
typedef struct STVID_NewDecodedPictureEvtParams_s
{
    STGVOBJ_PictureId_t               MainTopPictureId;
    STGVOBJ_PictureId_t               MainBottomPictureId;
    STGVOBJ_PictureId_t               MainRepeatPictureId;
    STGVOBJ_PictureId_t               DecimatedTopPictureId;
    STGVOBJ_PictureId_t               DecimatedBottomPictureId;
    STGVOBJ_PictureId_t               DecimatedRepeatPictureId;
    DVRBM_BufferPoolId_t              BufferPoolId;
} STVID_NewDecodedPictureEvtParams_t;
#endif /* end of def STVID_STVAPI_ARCHITECTURE */


typedef struct STVID_SequenceInfo_s
{
    U32                         Height;
    U32                         Width;
    STVID_DisplayAspectRatio_t  Aspect;
    STVID_ScanType_t            ScanType;
    U32                         FrameRate;
    U32                         BitRate;
    STVID_MPEGStandard_t        MPEGStandard;
    BOOL                        IsLowDelay;
    U32                         VBVBufferSize;
    U8                          StreamID;
    U32                         ProfileAndLevelIndication;
    U8                          VideoFormat;

    /* Valid only if MPEGStandard is STVID_MPEG_STANDARD_ISO_IEC_13818 */
    U8                          FrameRateExtensionN; /*  2 bits */
    U8                          FrameRateExtensionD; /*  5 bits */

} STVID_SequenceInfo_t;

typedef struct STVID_SpeedDriftThreshold_s
{
    S32 DriftTime;
    U32 BitRateValue;          /* in units of 400 bits/second */
    U32 SpeedRatio;
} STVID_SpeedDriftThreshold_t;

typedef struct STVID_StartParams_s
{
    BOOL                        RealTime;
    BOOL                        UpdateDisplay;
    STVID_BroadcastProfile_t    BrdCstProfile;
    STVID_StreamType_t          StreamType;
    U8                          StreamID;
    BOOL                        DecodeOnce;
} STVID_StartParams_t;

typedef struct STVID_SynchroInfo_s
{
    STVID_SynchroAction_t   Action;
    STTS_t                  EffectiveTime;
} STVID_SynchroInfo_t;

typedef struct STVID_SynchronisationInfo_s
{
    S32             ClocksDifference;
    BOOL            IsSynchronisationOk;
    BOOL            IsLoosingSynchronisation;
    BOOL            IsBackToSynchronisation;
} STVID_SynchronisationInfo_t;

typedef struct STVID_TermParams_s
{
    BOOL ForceTerminate;
} STVID_TermParams_t;


typedef struct STVID_UserData_s
{
    STVID_BroadcastProfile_t    BroadcastProfile;
    STVID_UserDataPosition_t    PositionInStream;
    U32                         Length;
    BOOL                        BufferOverflow;
    void *                      Buff_p;
    STVID_PTS_t                 PTS;    /* valid for pictures only */

    /* Private Data */
    S32  pTemporalReference; /* Picture user data only */
    BOOL IsRegistered;      /* the 3 fields following are only available if IsReference==TRUE */
    U8 itu_t_t35_country_code;
    U8 itu_t_t35_country_code_extension_byte;
    U16 itu_t_t35_provider_code;
} STVID_UserData_t;

typedef void * STVID_ViewPortHandle_t;

typedef struct STVID_ViewPortParams_s
{
    ST_DeviceName_t LayerName;
} STVID_ViewPortParams_t;

typedef struct STVID_WindowParams_s
{
    STVID_WinAlign_t Align;
    STVID_WinSize_t Size;
} STVID_WindowParams_t;

/* Add buffer parameter structure to map PES buffer in user space */
#ifdef ST_OSLINUX
#define STVID_CALLBACK_DIRECT_INTERFACE_PTI     (NULL)
#endif

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

ST_Revision_t  STVID_GetRevision(void);
ST_ErrorCode_t STVID_GetCapability(const ST_DeviceName_t DeviceName, STVID_Capability_t  * const Capability_p);
ST_ErrorCode_t STVID_Init(const ST_DeviceName_t DeviceName, const STVID_InitParams_t * const InitParams_p);
ST_ErrorCode_t STVID_Open(const ST_DeviceName_t DeviceName, const STVID_OpenParams_t * const OpenParams_p, STVID_Handle_t * const Handle_p);
ST_ErrorCode_t STVID_Close(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_Term(const ST_DeviceName_t DeviceName, const STVID_TermParams_t * const TermParams_p);

#if defined ST_OSLINUX
#if !defined MODULE
/* Preprocess events only for user space under Linux. */
ST_ErrorCode_t STVID_PreprocessEvent(const ST_DeviceName_t DeviceName, STEVT_EventConstant_t Event, void * EventData);
#else /* ! MODULE */
/* For kernel space under linux: dummy function. */
#define STVID_PreprocessEvent(DeviceName, Event, EventData)     (ST_NO_ERROR)
#endif /* MODULE */

#else /* ST_OSLINUX */

/* For other OSes: dummy function. */
#define STVID_PreprocessEvent(DeviceName, Event, EventData)     (ST_NO_ERROR)
#endif /* Not ST_OSLINUX */

ST_ErrorCode_t STVID_Clear(const STVID_Handle_t Handle, const STVID_ClearParams_t * const Params_p);
ST_ErrorCode_t STVID_ConfigureEvent(const STVID_Handle_t Handle, const STEVT_EventID_t Event, const STVID_ConfigureEventParams_t * const Params_p);
ST_ErrorCode_t STVID_DataInjectionCompleted(const STVID_Handle_t Handle, const STVID_DataInjectionCompletedParams_t * const Params_p);
ST_ErrorCode_t STVID_DeleteDataInputInterface(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_DisableDeblocking(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_DisableFrameRateConversion(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_DisableSynchronisation(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_DuplicatePicture(const STVID_PictureParams_t * const Source_p, STVID_PictureParams_t * const Dest_p);
ST_ErrorCode_t STVID_EnableDeblocking(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_EnableFrameRateConversion(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_EnableSynchronisation(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_ForceDecimationFactor(const STVID_Handle_t Handle, const STVID_ForceDecimationFactorParams_t * const Params_p);
ST_ErrorCode_t STVID_Freeze(const STVID_Handle_t Handle, const STVID_Freeze_t * const Freeze_p);
ST_ErrorCode_t STVID_GetBitBufferFreeSize(const STVID_Handle_t Handle, U32 * const FreeSize_p);
ST_ErrorCode_t STVID_GetBitBufferParams(const STVID_Handle_t Handle, void ** const BaseAddress_p, U32 * const InitSize_p);
ST_ErrorCode_t STVID_GetDataInputBufferParams(const STVID_Handle_t
     Handle, void ** const BaseAddress_p, U32 * const Size_p);
ST_ErrorCode_t STVID_GetDecimationFactor(const STVID_Handle_t Handle, STVID_DecimationFactor_t * const DecimationFactor_p);
ST_ErrorCode_t STVID_GetDecodedPictures(const STVID_Handle_t Handle, STVID_DecodedPictures_t * const DecodedPictures_p);
ST_ErrorCode_t STVID_GetErrorRecoveryMode(const STVID_Handle_t Handle, STVID_ErrorRecoveryMode_t * const Mode_p);
ST_ErrorCode_t STVID_GetHDPIPParams(const STVID_Handle_t Handle, STVID_HDPIPParams_t * const HDPIPParams_p);
ST_ErrorCode_t STVID_GetMemoryProfile(const STVID_Handle_t Handle, STVID_MemoryProfile_t * const MemoryProfile_p);
ST_ErrorCode_t STVID_GetPictureAllocParams(const STVID_Handle_t Handle, const STVID_PictureParams_t * const Params_p, STAVMEM_AllocBlockParams_t * const AllocParams_p);
ST_ErrorCode_t STVID_GetPictureParams(const STVID_Handle_t Handle, const STVID_Picture_t PictureType, STVID_PictureParams_t * const Params_p);
ST_ErrorCode_t STVID_GetPictureInfos(const STVID_Handle_t Handle, const STVID_Picture_t PictureType, STVID_PictureInfos_t * const PictureInfos_p);
ST_ErrorCode_t STVID_GetDisplayPictureInfo(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t PictureBufferHandle, STVID_DisplayPictureInfos_t * const DisplayPictureInfos_p);
ST_ErrorCode_t STVID_GetSpeed(const STVID_Handle_t Handle, S32 * const Speed_p);
ST_ErrorCode_t STVID_InjectDiscontinuity(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_Pause(const STVID_Handle_t Handle, const STVID_Freeze_t * const Freeze_p);
ST_ErrorCode_t STVID_PauseSynchro(const STVID_Handle_t Handle, const STTS_t Time);
ST_ErrorCode_t STVID_Resume(const STVID_Handle_t Handle);
/* This function called by user autorizes video to call external functions */
ST_ErrorCode_t STVID_SetDataInputInterface(const STVID_Handle_t Handle,
     ST_ErrorCode_t (*GetWriteAddress)  (void * const Handle, void ** const Address_p),
     void (*InformReadAddress)(void * const Handle, void * const Address),
     void * const FunctionsHandle);
#ifdef VIDEO_DEBLOCK_DEBUG
ST_ErrorCode_t STVID_SetDeblockingStrength(const STVID_Handle_t VideoHandle, const U8 DeblockingStrength);
#endif /* VIDEO_DEBLOCK_DEBUG */
ST_ErrorCode_t STVID_SetDecodedPictures(const STVID_Handle_t Handle, const STVID_DecodedPictures_t DecodedPictures);
ST_ErrorCode_t STVID_SetErrorRecoveryMode(const STVID_Handle_t Handle, const STVID_ErrorRecoveryMode_t Mode);
#ifdef STVID_STVAPI_ARCHITECTURE
ST_ErrorCode_t STVID_SetExternalRasterBufferManagerPoolId(const STVID_Handle_t VideoHandle, const DVRBM_BufferPoolId_t BufferPoolId);
#endif /* STVID_STVAPI_ARCHITECTURE */
ST_ErrorCode_t STVID_SetHDPIPParams(const STVID_Handle_t Handle, const STVID_HDPIPParams_t * const HDPIPParams_p);
ST_ErrorCode_t STVID_SetMemoryProfile(const STVID_Handle_t Handle, const STVID_MemoryProfile_t * const MemoryProfile_p);
ST_ErrorCode_t STVID_SetSpeed(const STVID_Handle_t Handle, const S32 Speed);
ST_ErrorCode_t STVID_Setup(const STVID_Handle_t Handle, const STVID_SetupParams_t * const SetupParams_p);
ST_ErrorCode_t STVID_SkipSynchro(const STVID_Handle_t Handle, const STTS_t Time);
ST_ErrorCode_t STVID_Start(const STVID_Handle_t Handle, const STVID_StartParams_t * const Params_p);
ST_ErrorCode_t STVID_StartUpdatingDisplay(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_Step(const STVID_Handle_t Handle);
ST_ErrorCode_t STVID_Stop(const STVID_Handle_t Handle, const STVID_Stop_t StopMode, const STVID_Freeze_t * const Freeze_p);

typedef struct STVID_GetPictureBufferParams_s
{
    STVID_PictureStructure_t    PictureStructure; /* Picture structure (Frame, Top field, Bottom field)*/
    BOOL                        TopFieldFirst; /* If PictureStructure is frame, indicates the first (decoded) field in the frame */
    BOOL                        ExpectingSecondField; /* Set TRUE if the picture is a field picture, the first of the 2 fields (FALSE if second field) */
    U32                         ExtendedTemporalReference; /* Extended temporal reference: temporal_reference, but with no loop back to 0 and no "hole" in value incrementation */
    U32                         PictureWidth;
    U32                         PictureHeight;
} STVID_GetPictureBufferParams_t;


typedef struct STVID_PictureBufferDataParams_s
{
    void *                  Data1_p;
    U32                     Size1;
    void *                  Data2_p;
    U32                     Size2;

} STVID_PictureBufferDataParams_t;



ST_ErrorCode_t STVID_GetPictureBuffer(const STVID_Handle_t Handle, const STVID_GetPictureBufferParams_t * const Params_p,
                                      STVID_PictureBufferDataParams_t * const PictureBufferParams_p,
                                      STVID_PictureBufferHandle_t * const PictureBufferHandle_p);
ST_ErrorCode_t STVID_ReleasePictureBuffer(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t  PictureBufferHandle);
ST_ErrorCode_t STVID_DisplayPictureBuffer(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t  PictureBufferHandle,
                                          const STVID_PictureInfos_t*  const PictureInfos_p);
ST_ErrorCode_t STVID_TakePictureBuffer(const STVID_Handle_t Handle, const STVID_PictureBufferHandle_t PictureBufferHandle);

ST_ErrorCode_t STVID_DisableDisplay(const STVID_Handle_t VideoHandle);
ST_ErrorCode_t STVID_EnableDisplay(const STVID_Handle_t VideoHandle);

ST_ErrorCode_t STVID_PrintPictureBuffersStatus(const STVID_Handle_t Handle);

#if ! defined STVID_NO_COMPOSITION
/* These functions can be removed if driver composition is not needed ------- */
ST_ErrorCode_t STVID_CloseViewPort(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_DisableBorderAlpha(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_DisableColorKey(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_DisableOutputWindow(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_EnableBorderAlpha(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_EnableColorKey(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_EnableOutputWindow(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_GetAlignIOWindows(const STVID_ViewPortHandle_t ViewPortHandle,
                    S32 * const InputWinX_p,  S32 * const InputWinY_p,  U32 * const InputWinWidth_p,  U32 * const InputWinHeight_p,
                    S32 * const OutputWinX_p, S32 * const OutputWinY_p, U32 * const OutputWinWidth_p, U32 * const OutputWinHeight_p);
ST_ErrorCode_t STVID_GetDisplayAspectRatioConversion(const STVID_ViewPortHandle_t ViewPortHandle, STVID_DisplayAspectRatioConversion_t * const Conversion_p);
ST_ErrorCode_t STVID_GetInputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, BOOL * const AutoMode_p, STVID_WindowParams_t * const WinParams_p);
ST_ErrorCode_t STVID_GetIOWindows(const STVID_ViewPortHandle_t ViewPortHandle,
                    S32 * const InputWinX_p,  S32 * const InputWinY_p,  U32 * const InputWinWidth_p,  U32 * const InputWinHeight_p,
                    S32 * const OutputWinX_p, S32 * const OutputWinY_p, U32 * const OutputWinWidth_p, U32 * const OutputWinHeight_p);
ST_ErrorCode_t STVID_GetOutputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, BOOL * const AutoMode_p, STVID_WindowParams_t * const WinParams_p);
ST_ErrorCode_t STVID_GetViewPortAlpha(const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_GlobalAlpha_t * const GlobalAlpha_p);
ST_ErrorCode_t STVID_GetViewPortColorKey(const STVID_ViewPortHandle_t ViewPortHandle, STGXOBJ_ColorKey_t * const ColorKey_p);
ST_ErrorCode_t STVID_GetViewPortPSI(const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_PSI_t * const VPPSI_p);
ST_ErrorCode_t STVID_GetViewPortSpecialMode (const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_OutputMode_t * const OuputMode_p, STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
ST_ErrorCode_t STVID_HidePicture(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_OpenViewPort(const STVID_Handle_t VideoHandle,
                                  const STVID_ViewPortParams_t * const ViewPortParams_p,
                                  STVID_ViewPortHandle_t * const ViewPortHandle_p);
ST_ErrorCode_t STVID_SetDisplayAspectRatioConversion(const STVID_ViewPortHandle_t ViewPortHandle, const STVID_DisplayAspectRatioConversion_t Mode);
ST_ErrorCode_t STVID_SetInputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, const BOOL AutoMode, const STVID_WindowParams_t * const WinParams_p);
ST_ErrorCode_t STVID_SetIOWindows(const STVID_ViewPortHandle_t ViewPortHandle,
                    const S32 InputWinX,  const S32 InputWinY,  const U32 InputWinWidth,  const U32 InputWinHeight,
                    const S32 OutputWinX, const S32 OutputWinY, const U32 OutputWinWidth, const U32 OutputWinHeight);
ST_ErrorCode_t STVID_SetOutputWindowMode(const STVID_ViewPortHandle_t ViewPortHandle, const BOOL AutoMode, const STVID_WindowParams_t * const WinParams_p);
ST_ErrorCode_t STVID_SetViewPortAlpha(const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_GlobalAlpha_t * const GlobalAlpha_p);
ST_ErrorCode_t STVID_SetViewPortColorKey(const STVID_ViewPortHandle_t ViewPortHandle, const STGXOBJ_ColorKey_t * const ColorKey_p);
ST_ErrorCode_t STVID_SetViewPortPSI(const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_PSI_t * const VPPSI_p);
ST_ErrorCode_t STVID_SetViewPortSpecialMode (const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_OutputMode_t OuputMode, const STLAYER_OutputWindowSpecialModeParams_t * const Params_p);
ST_ErrorCode_t STVID_ShowPicture(const STVID_ViewPortHandle_t ViewPortHandle, STVID_PictureParams_t * const Params_p, const STVID_Freeze_t * const Freeze_p);
#endif /* no STVID_NO_COMPOSITION */

#if defined(DVD_SECURED_CHIP)
/* This function needs to be called only on secure platforms in order to use the DMU preprocessor hardware work around. */
ST_ErrorCode_t STVID_SetupReservedPartitionForH264PreprocWA_GNB42332(const STVID_Handle_t Handle, const STAVMEM_PartitionHandle_t AVMEMPartition);
#endif /* DVD_SECURED_CHIP */




/* Un-supported extensions (debug only) ------------------------------------- */

/*******************************************************************************
WARNING (do not remove)
=======
Every user of this code must be made aware of the following limitations.
This section's API's are extensions to the STVID API, for debugging and testing
purposes only. They are not part of the STVID product delivery, they don't
contain driver features used in production. They are not compiled by default.
They may change without notice. They may contain errors, are not tested. They
may be not supported on some platforms, may change the driver normal behaviour.
Application code such as reference software or customer application may use
them at their own risks and not for production. There will be no support.
*******************************************************************************/



#ifdef STVID_DEBUG_GET_STATISTICS
typedef struct STVID_Statistics_s
{
    /* Syntax for the naming of statistics variables: <module>[Pb]<variable_name>
        . <module> tells which module is responsible for the variable,
        . [Pb] is present if in the normal operations the variable should remain 0,
        . <variable_name> is the meaning of the variable */
    U32 ApiPbLiveResetWaitForFirstPictureDetected; /* Counts number of LiveReset while waiting for a 1st picture parsed (automatic Stop()/Start() when idle for too long in real-time) */
    U32 ApiPbLiveResetWaitForFirstPictureDecoded;  /* Counts number of LiveReset while waiting for a 1st picture decoded (automatic Stop()/Start() when idle for too long in real-time) */
    U32 ApiPbLiveResetWaitForNextPicture;   /* Counts number of LiveReset while waiting for the next picture decoded (automatic Stop()/Start() when idle for too long in real-time) */
    U32 AvsyncSkippedFields;                /* Counts number of fields skipped because of AVSYNC module */
    U32 AvsyncRepeatedFields;               /* Counts number of fields repeated because of AVSYNC module */
    U32 AvsyncMaxRepeatedFields;            /* Counts max number of fields required to be repeated at the display */
    U32 AvsyncFailedToSkipFields;           /* Counts number of fields that could not be skipped because display could not */
    U32 AvsyncExtendedSTCAvailable;         /* Counts number of time an extended STC was obtained successfully from STCLKRV */
    U32 AvsyncPictureWithNonInterpolatedPTS;/* Counts number of pictures with PTS's in the stream (non interpolated) */
    U32 AvsyncPictureCheckedSynchronizedOk; /* Counts number of pictures that were checked to be syncronized OK (need PTS+STC+enablesync+speed100) */
    U32 AvsyncPTSInconsistency;             /* Counts number of time the PTS of the current picture is not greater than the PTS of the previous picture */
    U32 DecodeHardwareSoftReset;            /* Counts number of soft reset of the hardware decoder */
    U32 DecodeStartCodeFound;               /* Counts number of start code found in bit buffer */
    U32 DecodeSequenceFound;                /* Counts number of sequence start code found in bit buffer */
    U32 DecodeUserDataFound;                /* Counts number of user data start code found in bit buffer */
    U32 DecodePictureFound;                 /* Counts number of picture srat code found in bit buffer */
    U32 DecodePictureFoundMPEGFrameI;       /* Counts number of I pictures found in bit buffer */
    U32 DecodePictureFoundMPEGFrameP;       /* Counts number of P pictures found in bit buffer */
    U32 DecodePictureFoundMPEGFrameB;       /* Counts number of B pictures found in bit buffer */
    U32 DecodePictureSkippedRequested;      /* Counts number of times the deccode of a picture was skipped following a request from Avsync or trickmode */
    U32 DecodePictureSkippedNotRequested;   /* Counts number of times the deccode of a picture was skipped without request from Avsync or trickmode */
    U32 DecodePictureDecodeLaunched;        /* Counts number of times the deccode of a picture is launched */
    U32 DecodeStartConditionVbvDelay;       /* Counts number of times the 1st decode after STVID_Start occurs according to the Vbv_Delay information. */
    U32 DecodeStartConditionPtsTimeComparison; /* Counts number of times the 1st decode after STVID_Start occurs according to the comparison PTS - STC. */
    U32 DecodeStartConditionVbvBufferSize;  /* Counts number of times the 1st decode after STVID_Start occurs according to the Vbv_Buffer_Size information. */
    U32 DecodeInterruptStartDecode;         /* Counts number of StartDecode interrupts */
    U32 DecodeInterruptPipelineIdle;        /* Counts number of PipelineIdle interrupts */
    U32 DecodeInterruptDecoderIdle;         /* Counts number of DecoderIdle interrupts */
    U32 DecodeInterruptBitBufferEmpty;      /* Counts number of BitBufferEmpty interrupts */
    U32 DecodeInterruptBitBufferFull;       /* Counts number of BitBufferFull interrupts */
    U32 DecodePbStartCodeFoundInvalid;      /* Counts number of invalid start code found in bit buffer */
    U32 DecodePbStartCodeFoundVideoPES;     /* Counts number of invalid start code found in bit buffer that are MPEG video PES start codes */
    U32 DecodePbMaxNbInterruptSyntaxErrorPerPicture;    /* Counts number of SyntaxError interrupts */
    U32 DecodePbInterruptSyntaxError;       /* Counts number of SyntaxError interrupts */
    U32 DecodePbInterruptDecodeOverflowError;/* Counts number of decode overflow interrupts */
    U32 DecodePbInterruptDecodeUnderflowError;/* Counts number of decode underflow interrupts */
    U32 DecodePbDecodeTimeOutError;         /* Counts number of decode time out */
    U32 DecodePbInterruptMisalignmentError; /* Counts number of decode misalignments */
    U32 DecodePbInterruptQueueOverflow;     /* Counts number of times the decode queue of interrupts overflows. Consequence: no IT flag is lost, but multiplicity can be lost because new IT status are ORed. */
    U32 DecodePbHeaderFifoEmpty;            /* Counts number of times the header fifo was empty when poping start code or bits. */
    U32 DecodePbVbvSizeGreaterThanBitBuffer;/* Counts number of times vbv decode level from the stream is greater than the bit buffer size. (Bit buffer is theroritically too small to decode the stream) */
    U32 DecodeMinBitBufferLevelReached;     /* Tells min bit buffer level of occupancy reached according to software polling. */
    U32 DecodeMaxBitBufferLevelReached;     /* Tells max bit buffer level of occupancy reached according to software polling. */
    U32 DecodePbSequenceNotInMemProfileSkipped; /* Counts number of sequence skipped at decode level because they don't fit in the memory profile */
#ifdef STVID_HARDWARE_ERROR_EVENT
    U32 DecodePbHardwareErrorMissingPID;    /* Counts number of hardware error 'missing PID' (on STi7015) */
    U32 DecodePbHardwareErrorSyntaxError;   /* Counts number of hardware error 'syntax error' (on STi7015) */
    U32 DecodePbHardwareErrorTooSmallPicture;/* Counts number of hardware error 'too small picture' (on STi7015) */
#endif /* STVID_HARDWARE_ERROR_EVENT */
    U32 DecodePbParserError;                /* Counts number of errors reported by parser driver (delta IP) */
    U32 DecodePbPreprocError;               /* Counts number of errors reported by proprocessor driver (delta IP) */
    U32 DecodePbFirmwareError;              /* Counts number of errors reported by Firmware (delta IP) */
    U32 DecodeGNBvd42696Error;              /* Counts number of GNBvd42696 (DMC) errors reported by firmware */
/* Decode time statistics from VIDPROD in MicroSeconds */
    U32                     DecodeTimeNbPictures;
    U32                     DecodeTimeMinPreprocessTime;
    U32                     DecodeTimeMaxPreprocessTime;
    U32                     DecodeTimeSumOfPreprocessTime;
    U32                     DecodeTimeMinBufferSearchTime;
    U32                     DecodeTimeMaxBufferSearchTime;
    U32                     DecodeTimeSumOfBufferSearchTime;
    U32                     DecodeTimeMinDecodeTime;
    U32                     DecodeTimeMaxDecodeTime;
    U32                     DecodeTimeSumOfDecodeTime;
    U32                     DecodeTimeMinFullDecodeTime;
    U32                     DecodeTimeMaxFullDecodeTime;
    U32                     DecodeTimeSumOfFullDecodeTime;
#ifdef STVID_VALID_MEASURE_TIMING
    U32                     DecodeTimeMinLXDecodeTime;
    U32                     DecodeTimeMaxLXDecodeTime;
    U32                     DecodeTimeSumOfLXDecodeTime;
#endif
/* End of Decode time statistics from VIDPROD */
    U32 DisplayPictureInsertedInQueue;      /* Counts number of pictures inserted in display queue */
    U32 DisplayPictureInsertedInQueueDecimated;      /* Counts number of pictures inserted in display queue */
    U32 DisplayPictureDisplayedByMain;            /* Counts number of pictures displayed (not decimated) */
    U32 DisplayPictureDisplayedByAux;            /* Counts number of pictures displayed (not decimated) */
    U32 DisplayPictureDisplayedBySec;            /* Counts number of pictures displayed (not decimated) */
    U32 DisplayPictureDisplayedDecimatedByMain;   /* Counts number of decimated pictures displayed */
    U32 DisplayPictureDisplayedDecimatedByAux;   /* Counts number of decimated pictures displayed */
    U32 DisplayPictureDisplayedDecimatedBySec;   /* Counts number of decimated pictures displayed */
    U32 DisplayPbQueueLockedByLackOfPicture;/* Counts number of time the display queue is locked by lack of picture to display */
    U32 DisplayPbQueueOverflow;             /* Counts number of overflow of the display queue */
    U32 DisplayPbPictureTooLateRejectedByMain;    /* Counts number of pictures rejected by display because too late */
    U32 DisplayPbPictureTooLateRejectedByAux;    /* Counts number of pictures rejected by display because too late */
    U32 DisplayPbPictureTooLateRejectedBySec;    /* Counts number of pictures rejected by display because too late */
    U32 DisplayPbPicturePreparedAtLastMinuteRejected;    /* Counts number of pictures prepared at last minute that are rejected by display (feature not available anymore)*/

    U32 QueuePictureInsertedInQueue;      /* Counts number of pictures inserted in ordering queue */
    U32 QueuePictureRemovedFromQueue;      /* Counts number of pictures removed from ordering queue */
    U32 QueuePicturePushedToDisplay;            /* Counts number of pictures pushed to display */
    U32 QueuePbPictureTooLateRejected;    /* Counts number of pictures rejected because too late */
    U32 SpeedDisplayBFramesNb;
    U32 SpeedDisplayPFramesNb;
    U32 SpeedDisplayIFramesNb;
    U32 MaxBitRate;
    U32 MinBitRate;
    U32 LastBitRate;
    S32 MaxPositiveDriftRequested;
    S32 MaxNegativeDriftRequested;
    U32 NbDecodedPicturesB;
    U32 NbDecodedPicturesP;
    U32 NbDecodedPicturesI;
    U32 SpeedSkipReturnNone;
    U32 SpeedRepeatReturnNone;
    /* Inject statistics ---------------------------------------------------- */
    U32 InjectFdmaTransfers;                /* Counts the number of FDMA transfers performed */
    void * InjectDataInputReadPointer;      /* Tells current read pointer in the data input buffer */
    void * InjectDataInputWritePointer;     /* Tells current write pointer in the data input buffer */
} STVID_Statistics_t;

ST_ErrorCode_t STVID_GetStatistics(const ST_DeviceName_t DeviceName, STVID_Statistics_t * const Statistics_p);
ST_ErrorCode_t STVID_ResetStatistics(const ST_DeviceName_t DeviceName, const STVID_Statistics_t * const Statistics_p);
#endif /* STVID_DEBUG_GET_STATISTICS */

#ifdef STVID_DEBUG_GET_STATUS
typedef struct STVID_Statu_s
{
    /* Syntax for the naming of statistics variables: <module>[Pb]<variable_name>
        . <module> tells which module is responsible for the variable,
        . [Pb] is present if in the normal operations the variable should remain 0,
        . <variable_name> is the meaning of the variable */
    /* Driver status (API variables) */
    void * BitBufferAddress_p;
    U32    BitBufferSize;                    /* in byte */
    void * DataInputBufferAddress_p;
    U32    DataInputBufferSize;              /* in byte */
    void * SCListBufferAddress_p;
    U32    SCListBufferSize;                 /* in byte */
	void * IntermediateBuffersAddress_p;
	U32    IntermediateBuffersSize;          /* in byte */
	U32    FdmaNodesSize;                    /* in byte */
	U32    InjectFlushBufferSize;            /* in byte */

	/* TODO Calculate PictureParameterBuffersSize */
	/* U32    PictureParameterBuffersSize; */     /* in byte */


    STVID_MemoryProfile_t  MemoryProfile;    /* Current memory profile */
    S32   SyncDelay;
    enum StateMachine_e                      /* TDB like in api.h */
    {
      VIDEO_STATE_STOPPED,
      VIDEO_STATE_RUNNING,
      VIDEO_STATE_PAUSING,
      VIDEO_STATE_FREEZING
    } StateMachine;                          /* State of the instance */
	BOOL VideoAlreadyStarted;                /* TRUE when STVID_Start is called */
	STVID_StartParams_t LastStartParams;     /* Last start parameters */
	BOOL VideoAlreadyFreezed;                /* TRUE when STVID_Freeze is called */
	STVID_Freeze_t LastFreezeParams;         /* Last freeze parameters */

    /* Driver status (internal variables) */
	/* TODO Calculate BitRateValue, DisplayLatency and PresentationDelay */
	/* U32   BitRateValue; */                     /* Last computed bit rate value */
    /* U8    DisplayLatency; */                   /* # of field latency introduced by the display */
    /* U32   PresentationDelay; */                /* */
    void * BitBufferReadPointer;             /* Current read pointer in the bit buffer */
    void * BitBufferWritePointer;            /* Current write pointer in the bit buffer */
    void * DataInputBufferReadPointer;       /* Current read pointer in the data input buffer */
    void * DataInputBufferWritePointer;      /* Current write pointer in the data input buffer */
    void * SCListBufferReadPointer;          /* Current read pointer in the SC List buffer */
    void * SCListBufferWritePointer;         /* Current write pointer in the SC List buffer */


    /* Stream status / characteristics */
    STVID_SequenceInfo_t* SequenceInfo_p;  /* Last sequence information, NULL if none or stopped */
	/* TODO Calculate LastDecodedPictureInfos_p and LastDisplayedPictureInfos_p */
	/* STVID_PictureInfos_t * LastDecodedPictureInfos_p; */ /* Last picture information, NULL if none */
    /* STVID_PictureInfos_t * LastDisplayedPictureInfos_p; */ /* Last picture information, NULL if none */

    /* Environment status */
	/* TODO Calculate DisplayMasterVTGFrameRate, DisplaySlaveVTGFrameRate and STC */
	/* U32   DisplayMasterVTGFrameRate; */       /* Frame rate of the VTG driving the master display */
    /* U32   DisplaySlaveVTGFrameRate; */        /* Frame rate of the VTG driving the slave display */
    /* STVID_PTS_t STC; */                      /* Last captured STC */
} STVID_Status_t;

ST_ErrorCode_t STVID_GetStatus(const ST_DeviceName_t DeviceName, STVID_Status_t * const Status_p);
#endif /* STVID_DEBUG_GET_STATUS */


ST_ErrorCode_t STVID_SetViewPortQualityOptimizations(const STVID_ViewPortHandle_t ViewPortHandle,
                                                     const STLAYER_QualityOptimizations_t * Params_p);
ST_ErrorCode_t STVID_GetViewPortQualityOptimizations(const STVID_ViewPortHandle_t ViewPortHandle,
                                                     STLAYER_QualityOptimizations_t * const Params_p);


#if ! defined STVID_NO_COMPOSITION
#ifdef VIDEO_DEBUG_DEINTERLACING_MODE
ST_ErrorCode_t STVID_GetRequestedDeinterlacingMode(const STVID_ViewPortHandle_t ViewPortHandle,
        STLAYER_DeiMode_t * const RequestedMode_p);
ST_ErrorCode_t STVID_SetRequestedDeinterlacingMode(const STVID_ViewPortHandle_t ViewPortHandle,
        const STLAYER_DeiMode_t RequestedMode);
#endif /* VIDEO_DEBUG_DEINTERLACING_MODE */
#endif /* no STVID_NO_COMPOSITION */



#ifdef STVID_USE_FMD

ST_ErrorCode_t STVID_SetFMDParams(const STVID_ViewPortHandle_t ViewPortHandle, const STLAYER_FMDParams_t * Params_p);
ST_ErrorCode_t STVID_GetFMDParams(const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_FMDParams_t * Params_p);
ST_ErrorCode_t STVID_EnableFMDReporting(const STVID_ViewPortHandle_t ViewPortHandle);
ST_ErrorCode_t STVID_DisableFMDReporting(const STVID_ViewPortHandle_t ViewPortHandle);

#endif /* STVID_USE_FMD */

#ifdef STVID_USE_CRC
typedef enum STVID_CRCFieldInversion_e
{
    STVID_CRC_INVFIELD_NONE,
    STVID_CRC_INVFIELD_WITH_PREVIOUS,
    STVID_CRC_INVFIELD_WITH_NEXT
} STVID_CRCFieldInversion_t;

typedef struct STVID_RefCRCEntry_s
{
    U32                         IDExtension;    /* MSB for PictureID, incremented at each "reset" of ID */
    S32                         ID;             /* LSB for PictureID */
    U32                         LumaCRC;
    U32                         ChromaCRC;
    BOOL                        FieldPicture;
} STVID_RefCRCEntry_t;

typedef struct STVID_CompCRCEntry_s
{
    U32                         IDExtension;    /* MSB for PictureID, incremented at each "reset" of ID */
    S32                         ID;             /* LSB for PictureID */
    U32                         DecodingOrderFrameID; /* FrameID with same value for both fields of field picture */
    U32                         LumaCRC;
    U32                         ChromaCRC;
    U32                         NbRepeatedLastPicture;
    U32                         NbRepeatedLastButOnePicture;
    BOOL                        FieldPicture;
    U32                         FieldCRC;
    BOOL                        IsMonochrome;
} STVID_CompCRCEntry_t;

typedef struct STVID_CRCErrorLogEntry_s
{
    U32                         RefCRCIndex;
    U32                         CompCRCIndex;
    U32                         IDExtension;    /* MSB for PictureID, incremented at each "reset" of ID */
    S32                         ID;             /* LSB for PictureID */
    U32                         DecodingOrderFrameID; /* FrameID with same value for both fields of field picture */
    U32                         LumaCRC;
    U32                         ChromaCRC;
    STVID_CRCFieldInversion_t   InvertedFields;
    BOOL                        LumaError;
    BOOL                        ChromaError;
    BOOL                        FieldPicture;
    U32                         FieldCRC;
    BOOL                        IsMonochrome;
} STVID_CRCErrorLogEntry_t;

typedef struct STVID_CRCStartParams_s
{
    U32                         NbPictureInRefCRC;
    STVID_RefCRCEntry_t         *RefCRCTable;
    U32                         MaxNbPictureInCompCRC;
    STVID_CompCRCEntry_t        *CompCRCTable;
    U32                         MaxNbPictureInErrorLog;
    STVID_CRCErrorLogEntry_t    *ErrorLogTable;
    U32                         NbPicturesToCheck;
    BOOL                        DumpFailingFrames;
} STVID_CRCStartParams_t;

typedef struct STVID_CRCCheckResult_s
{
    U32 NbRefCRCChecked;     /* Number of reference CRC checked (repeated pictures excluded */
    U32 NbCompCRC;           /* Number of Computed CRC stored in CompCRCTable including overwritten ones */
    U32 NbErrorsAndWarnings; /* Number of Error/warnings log stored in ErrorLogTable including overwritten ones */
    U32 NbErrors;            /* Number of Error only stored in ErrorLogTable including overwritten ones */
} STVID_CRCCheckResult_t;

typedef struct STVID_CRCDataMessage_s
{
    U32 IsReferencePicture;
    S32 PictureOrderCount ;
    U32 LumaCRC ;
    U32 ChromaCRC;
    U32 IsFieldCRC;
    U32 IsFieldBottomNotTop;
} STVID_CRCDataMessage_t;

typedef struct STVID_CRCReadMessages_s
{
    U8 NbToRead;
    STVID_CRCDataMessage_t *Messages_p;
    U8 NbReturned;
} STVID_CRCReadMessages_t;


ST_ErrorCode_t STVID_CRCStartCheck(const STVID_Handle_t VideoHandle, STVID_CRCStartParams_t *StartParams_p);
BOOL STVID_IsCRCCheckRunning(const STVID_Handle_t VideoHandle);
BOOL STVID_IsCRCModeField(const STVID_Handle_t VideoHandle,
                          STVID_PictureInfos_t *PictureInfos_p,
                          STVID_MPEGStandard_t MPEGStandard);
ST_ErrorCode_t STVID_CheckCRC(const STVID_Handle_t VideoHandle,
                                STVID_PictureInfos_t *PictureInfos_p,
                                U32 ExtendedPresentationOrderPictureIDExtension, U32 ExtendedPresentationOrderPictureID,
                                U32 DecodingOrderFrameID, U32 PictureNumber,
                                BOOL IsRepeatedLastPicture, BOOL IsRepeatedLastButOnePicture,
                                void *LumaAddress, void *ChromaAddress,
                                U32 LumaCRC, U32 ChromaCRC);
ST_ErrorCode_t STVID_CRCStopCheck(const STVID_Handle_t VideoHandle);
ST_ErrorCode_t STVID_CRCGetCurrentResults(const STVID_Handle_t Handle, STVID_CRCCheckResult_t *CRCCheckResult_p);

ST_ErrorCode_t STVID_CRCStartQueueing(const STVID_Handle_t VideoHandle);
ST_ErrorCode_t STVID_CRCStopQueueing(const STVID_Handle_t VideoHandle);
ST_ErrorCode_t STVID_CRCGetQueue(const STVID_Handle_t VideoHandle, STVID_CRCReadMessages_t *ReadCRC_p);
#endif /* STVID_USE_CRC */

#ifdef STVID_ENABLE_SYNCHRONIZATION_DELAY
ST_ErrorCode_t STVID_GetSynchronizationDelay(const STVID_Handle_t VideoHandle, S32 * const SyncDelay_p);
ST_ErrorCode_t STVID_SetSynchronizationDelay(const STVID_Handle_t VideoHandle, const S32 SyncDelay);
#endif /* STVID_ENABLE_SYNCHRONIZATION_DELAY */

#ifdef STVID_HARDWARE_ERROR_EVENT
typedef enum STVID_HardwareError_e
{
    STVID_HARDWARE_NO_ERROR_DETECTED       = 0,
    STVID_HARDWARE_ERROR_MISSING_PID       = 1,
    STVID_HARDWARE_ERROR_SYNTAX_ERROR      = 2,
    STVID_HARDWARE_ERROR_TOO_SMALL_PICTURE = 4,
    STVID_HARDWARE_ERROR_UNDEFINED         = 8
} STVID_HardwareError_t;
#endif /* STVID_HARDWARE_ERROR_EVENT */


#ifdef STVID_DEBUG_GET_DISPLAY_PARAMS
ST_ErrorCode_t STVID_GetVideoDisplayParams(const STVID_ViewPortHandle_t ViewPortHandle, STLAYER_VideoDisplayParams_t * Params_p);
#endif /*STVID_DEBUG_GET_DISPLAY_PARAMS*/

/* END OF Un-supported extensions (debug only) - see warning on top of section */

#ifdef ST_OSLINUX
#if !defined MODULE && !defined STVID_MMAP_PESBUFFERS_IN_USERWRAPPER
ST_ErrorCode_t STVID_MapPESBufferToUserSpace(STVID_Handle_t Handle, void ** Address_pp, U32 Size);
ST_ErrorCode_t STVID_UnmapPESBufferFromUserSpace(STVID_Handle_t Handle);
#endif
#endif  /* LINUX */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVID_H */

/* End of stvid.h */
