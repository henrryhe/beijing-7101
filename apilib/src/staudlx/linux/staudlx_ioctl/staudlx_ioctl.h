/*****************************************************************************
 *
 *  Module      : staudlx_ioctl
 *  Date        : 11-07-2005
 *  Author      : TAYLORD
 *  Description :
 *  Copyright   : STMicroelectronics (c)2005
 *****************************************************************************/


#ifndef STAUDLX_IOCTL_H
#define STAUDLX_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */


#include "staudlx.h"
#include "stclkrv.h"


/*** IOCtl defines ***/

#define STAUDLX_IOCTL_MAGIC_NUMBER   0X16     /* Unique module id - See 'ioctl-number.txt' */


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_InitParams_t      InitParams;
} STAUD_Ioctl_Init_t;

#define STAUD_IOC_INIT                    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 0, STAUD_Ioctl_Init_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_TermParams_t      TermParams;
} STAUD_Ioctl_Term_t;

#define STAUD_IOC_TERM               _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 1, STAUD_Ioctl_Term_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_OpenParams_t      OpenParams;
    STAUD_Handle_t          Handle;
} STAUD_Ioctl_Open_t;

#define STAUD_IOC_OPEN               _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 2, STAUD_Ioctl_Open_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t         Handle;
} STAUD_Ioctl_Close_t;

#define STAUD_IOC_CLOSE              _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 3, STAUD_Ioctl_Close_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;

} STAUD_Ioctl_Play_t;

#define STAUD_IOC_PLAY              _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 4, STAUD_Ioctl_Play_t *)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    STAUD_Object_t         InputSource;

} STAUD_Ioctl_DRConnectSource_t;

#define STAUD_IOC_DRCONNECTSOURCE     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 5, STAUD_Ioctl_DRConnectSource_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         OutputObject;

} STAUD_Ioctl_PPDisableDownsampling_t;

#define STAUD_IOC_PPDISABLEDOWNSAMPLING     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 6, STAUD_Ioctl_PPDisableDownsampling_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         OutputObject;
    U32                    Value;

} STAUD_Ioctl_PPEnableDownsampling_t;

#define STAUD_IOC_PPENABLEDOWNSAMPLING     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 7, STAUD_Ioctl_PPEnableDownsampling_t *)

 typedef STAUD_Ioctl_PPDisableDownsampling_t STAUD_Ioctl_OPDisableSynchronization_t;

#define STAUD_IOC_OPDISABLESYNCHRONIZATION     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 8, STAUD_Ioctl_OPDisableSynchronization_t *)

typedef STAUD_Ioctl_PPDisableDownsampling_t STAUD_Ioctl_OPEnableSynchronization_t;

#define STAUD_IOC_OPENABLESYNCHRONIZATION     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 9, STAUD_Ioctl_OPEnableSynchronization_t *)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         InputObject;
    STAUD_BroadcastProfile_t BroadcastProfile;
} STAUD_Ioctl_IPGetBroadcastProfile_t;

#define STAUD_IOC_IPGETBROADCASTPROFILE     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 10, STAUD_Ioctl_IPGetBroadcastProfile_t *)

typedef struct
    {
        ST_ErrorCode_t  ErrorCode;
        ST_DeviceName_t DeviceName;
        STAUD_Object_t         DecoderObject;
        STAUD_DRCapability_t Capability;
} STAUD_Ioctl_DRGetCapability_t;

#define STAUD_IOC_DRGETCAPABILITY     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 11, STAUD_Ioctl_DRGetCapability_t *)

typedef struct
    {
        ST_ErrorCode_t  ErrorCode;
        STAUD_Handle_t         Handle;
        STAUD_Object_t         DecoderObject;
        STAUD_DynamicRange_t  DynamicRange;
} STAUD_Ioctl_DRGetDynamicRangeControl_t;

#define STAUD_IOC_DRGETDYNAMICRANGECONTROL     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 12, STAUD_Ioctl_DRGetDynamicRangeControl_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    U32                    SamplingFrequency;

} STAUD_Ioctl_IPGetSamplingFrequency_t;

#define STAUD_IOC_IPGETSAMPLINGFREQUENCY     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 13, STAUD_Ioctl_IPGetSamplingFrequency_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    S32                    Speed;

} STAUD_Ioctl_DRGetSpeed_t;

#define STAUD_IOC_DRGETSPEED     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 14, STAUD_Ioctl_DRGetSpeed_t *)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    STAUD_StreamInfo_t     StreamInfo;

} STAUD_Ioctl_IPGetStreamInfo_t;

#define STAUD_IOC_IPGETSTREAMINFO     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 15, STAUD_Ioctl_IPGetStreamInfo_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    S32                    Offset;

} STAUD_Ioctl_DRGetSyncOffset_t;

#define STAUD_IOC_DRGETSYNCOFFSET     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 16, STAUD_Ioctl_DRGetSyncOffset_t *)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    STAUD_Fade_t           Fade;

} STAUD_Ioctl_DRPause_t;

#define STAUD_IOC_DRPAUSE     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 17, STAUD_Ioctl_DRPause_t *)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         InputObject;
    U32                    Delay;

} STAUD_Ioctl_IPPauseSynchro_t;

#define STAUD_IOC_IPPAUSESYNCHRO     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 18, STAUD_Ioctl_IPPauseSynchro_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    STAUD_StreamParams_t   StreamParams;

} STAUD_Ioctl_DRPrepare_t;

#define STAUD_IOC_DRPREPARE     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 19, STAUD_Ioctl_DRPrepare_t *)

typedef STAUD_Ioctl_PPDisableDownsampling_t STAUD_Ioctl_DRResume_t;

#define STAUD_IOC_DRRESUME     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 20, STAUD_Ioctl_DRResume_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_Object_t      Object;
    STCLKRV_Handle_t    ClkSource;
} STAUD_Ioctl_DRSetClockRecoverySource_t;

#define STAUD_IOC_DRSETCLOCKRECOVERYSOURCE     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 21, STAUD_Ioctl_DRSetClockRecoverySource_t *)

typedef STAUD_Ioctl_DRGetDynamicRangeControl_t STAUD_Ioctl_DRSetDynamicRangeControl_t;

#define STAUD_IOC_DRSETDYNAMICRANGECONTROL     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 23, STAUD_Ioctl_DRSetDynamicRangeControl_t *)

typedef STAUD_Ioctl_DRGetSpeed_t STAUD_Ioctl_DRSetSpeed_t;

#define STAUD_IOC_DRSETSPEED     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 24, STAUD_Ioctl_DRSetSpeed_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    U8                     StreamID;

} STAUD_Ioctl_IPSetStreamID_t;

#define STAUD_IOC_IPSETSTREAMID     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 25, STAUD_Ioctl_IPSetStreamID_t* )

typedef STAUD_Ioctl_IPPauseSynchro_t STAUD_Ioctl_IPSkipSynchro_t ;

#define STAUD_IOC_IPSKIPSYNCHRO     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 26, STAUD_Ioctl_IPSkipSynchro_t *)

typedef STAUD_Ioctl_DRGetSyncOffset_t STAUD_Ioctl_DRSetSyncOffset_t;

#define STAUD_IOC_DRSETSYNCOFFSET     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 27, STAUD_Ioctl_DRSetSyncOffset_t *)

#define MAX_REVISION_STRING_LEN 256

#define STAUD_IOC_GETREVISION     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 28, char* )

typedef STAUD_Ioctl_DRPrepare_t  STAUD_Ioctl_DRStart_t;

#define STAUD_IOC_DRSTART     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 29, STAUD_Ioctl_DRStart_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    STAUD_Stop_t           StopMode;
    STAUD_Fade_t           Fade;
} STAUD_Ioctl_DRStop_t;

#define STAUD_IOC_DRSTOP     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 30, STAUD_Ioctl_DRStop_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_Capability_t      Capability;
} STAUD_Ioctl_GetCapability_t;

#define STAUD_IOC_GETCAPABILITY    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 31, STAUD_Ioctl_GetCapability_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         OutputObject;
    STAUD_DigitalMode_t    OutputMode;
} STAUD_Ioctl_OPSetDigitalMode_t;

#define STAUD_IOC_OPSETDIGITALMODE    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 32, STAUD_Ioctl_OPSetDigitalMode_t*)

typedef STAUD_Ioctl_OPSetDigitalMode_t STAUD_Ioctl_OPGetDigitalMode_t;

#define STAUD_IOC_OPGETDIGITALMODE    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 33, STAUD_Ioctl_OPGetDigitalMode_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_Object_t          InputObject;
    STAUD_IPCapability_t    IPCapability;
} STAUD_Ioctl_IPGetCapability_t;

#define STAUD_IOC_IPGETCAPABILITY    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 34, STAUD_Ioctl_IPGetCapability_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t              Handle;
    STAUD_Object_t              DecoderObject;
    STAUD_DataInterfaceParams_t DMAParams;
} STAUD_Ioctl_IPGetDataInterfaceParams_t;

#define STAUD_IOC_IPGETDATAINTERFACEPARAMS    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 35, STAUD_Ioctl_IPGetDataInterfaceParams_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t          Handle;
    STAUD_Object_t          InputObject;
    STAUD_BufferParams_t    DataParams;
} STAUD_Ioctl_IPGetInputBufferParams_t;

#define STAUD_IOC_IPGETINPUTBUFFERPARAMS    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 36, STAUD_Ioctl_IPGetInputBufferParams_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    STAUD_InputParams_t InputParams;

} STAUD_Ioctl_IPGetParams_t;

#define STAUD_IOC_IPGETPARAMS    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 37, STAUD_Ioctl_IPGetParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    STAUD_PCMBuffer_t PCMBuffer;
    U32 NumBuffers;
    U32 NumQueued ;
} STAUD_Ioctl_IPQueuePCMBuffer_t;

#define STAUD_IOC_IPQUEUEPCMBUFFER    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 38, STAUD_Ioctl_IPQueuePCMBuffer_t*)

#define MAX_NUMBER_OF_STAUD_MIXERINPUTPARAMS (5)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    U8 Level ;

} STAUD_Ioctl_IPSetLowDataLevelEvent_t;

#define STAUD_IOC_IPSETLOWDATALEVELEVENT    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 40, STAUD_Ioctl_IPSetLowDataLevelEvent_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    STAUD_InputParams_t InputParams;

} STAUD_Ioctl_IPSetParams_t;

#define STAUD_IOC_IPSETPARAMS    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 41, STAUD_Ioctl_IPSetParams_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t MixerObject;
    U32 NumInputs;
    STAUD_Object_t InputSources[MAX_NUMBER_OF_STAUD_MIXERINPUTPARAMS];
    STAUD_MixerInputParams_t InputParams[MAX_NUMBER_OF_STAUD_MIXERINPUTPARAMS];
} STAUD_Ioctl_MXConnectSource_t;

#define STAUD_IOC_MXCONNECTSOURCE    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 42, STAUD_Ioctl_MXConnectSource_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    ST_DeviceName_t      DeviceName;
    STAUD_Object_t       MixerObject;
    STAUD_MXCapability_t MXCapability;

} STAUD_Ioctl_MXGetCapability_t;

#define STAUD_IOC_MXGETCAPABILITY    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 43, STAUD_Ioctl_MXGetCapability_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       MixerObject;
    STAUD_Object_t       InputSource;
    STAUD_MixerInputParams_t MXMixerInputParams;

} STAUD_Ioctl_MXSetInputParams_t;

#define STAUD_IOC_MXSETINPUTPARAMS    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 44, STAUD_Ioctl_MXSetInputParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_Attenuation_t  Attenuation;

} STAUD_Ioctl_PPGetAttenuation_t;

#define STAUD_IOC_PPGETATTENUATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 48, STAUD_Ioctl_PPGetAttenuation_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    ST_DeviceName_t DeviceName;
    STAUD_Object_t       OutputObject;
    STAUD_OPCapability_t OPCapability;

} STAUD_Ioctl_OPGetCapability_t;

#define STAUD_IOC_OPGETCAPABILITY   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 49, STAUD_Ioctl_OPGetCapability_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_SpeakerEnable_t SpeakerEnable;

} STAUD_Ioctl_PPSetSpeakerEnable_t;

#define STAUD_IOC_PPSETSPEAKERENABLE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 51, STAUD_Ioctl_PPSetSpeakerEnable_t*)

typedef STAUD_Ioctl_PPSetSpeakerEnable_t STAUD_Ioctl_PPGetSpeakerEnable_t;

#define STAUD_IOC_PPGETSPEAKERENABLE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 52, STAUD_Ioctl_PPGetSpeakerEnable_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_Attenuation_t  Attenuation;

} STAUD_Ioctl_PPSetAttenuation_t;

#define STAUD_IOC_PPSETATTENUATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 55, STAUD_Ioctl_PPSetAttenuation_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       OutputObject;
    STAUD_OutputParams_t Params;

} STAUD_Ioctl_OPSetParams_t;

#define STAUD_IOC_OPSETPARAMS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 56, STAUD_Ioctl_OPSetParams_t*)


typedef STAUD_Ioctl_OPSetParams_t STAUD_Ioctl_OPGetParams_t;

#define STAUD_IOC_OPGETPARAMS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 57, STAUD_Ioctl_OPGetParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_Delay_t        Delay;

} STAUD_Ioctl_PPSetDelay_t;

#define STAUD_IOC_PPSETDELAY   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 60, STAUD_Ioctl_PPSetDelay_t*)

typedef STAUD_Ioctl_PPSetDelay_t STAUD_Ioctl_PPGetDelay_t;

#define STAUD_IOC_PPGETDELAY   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 61, STAUD_Ioctl_PPGetDelay_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_Effect_t       Effect;

} STAUD_Ioctl_PPGetEffect_t;

/* #define STAUD_IOC_PPGETEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 62, STAUD_Ioctl_PPGetEffect_t*) */

typedef STAUD_Ioctl_PPGetEffect_t STAUD_Ioctl_PPSetEffect_t;

#define STAUD_IOC_PPSETEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 63, STAUD_Ioctl_PPSetEffect_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_Karaoke_t      Karaoke;

} STAUD_Ioctl_PPSetKaraoke_t;

#define STAUD_IOC_PPSETKARAOKE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 64, STAUD_Ioctl_PPSetKaraoke_t*)

typedef STAUD_Ioctl_PPSetKaraoke_t STAUD_Ioctl_PPGetKaraoke_t;

#define STAUD_IOC_PPGETKARAOKE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 65, STAUD_Ioctl_PPGetKaraoke_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_SpeakerConfiguration_t SpeakerConfig;
    STAUD_BassMgtType_t BassType;
} STAUD_Ioctl_PPSetSpeakerConfig_t;

#define STAUD_IOC_PPSETSPEAKERCONFIG   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 68, STAUD_Ioctl_PPSetSpeakerConfig_t*)

typedef STAUD_Ioctl_PPSetSpeakerConfig_t STAUD_Ioctl_PPGetSpeakerConfig_t;

#define STAUD_IOC_PPGETSPEAKERCONFIG   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 69, STAUD_Ioctl_PPGetSpeakerConfig_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PostProcObject;
    STAUD_Object_t       InputSource;

} STAUD_Ioctl_PPConnectSource_t;

#define STAUD_IOC_PPCONNECTSOURCE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 70, STAUD_Ioctl_PPConnectSource_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       OutputObject;

} STAUD_Ioctl_OPMute_t;

#define STAUD_IOC_OPMUTE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 71, STAUD_Ioctl_OPMute_t*)

typedef STAUD_Ioctl_OPMute_t STAUD_Ioctl_OPUnMute_t;

#define STAUD_IOC_OPUNMUTE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 72, STAUD_Ioctl_OPUnMute_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_Object_t          PostProcObject;
    STAUD_PPCapability_t    PPCapability;

} STAUD_Ioctl_PPGetCapability_t;

#define STAUD_IOC_PPGETCAPABILITY   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 73, STAUD_Ioctl_PPGetCapability_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t InputObject;
    GetWriteAddress_t   GetWriteAddress;
    InformReadAddress_t InformReadAddress;
    void * BufferHandle;

} STAUD_Ioctl_IPSetDataInputInterface_t;


#define STAUD_IOC_IPSETDATAINPUTINTERFACE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 74, STAUD_Ioctl_IPSetDataInputInterface_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
} STAUD_Ioctl_DisableDeEmphasis_t;

#define STAUD_IOC_DISABLEDEEMPHASIS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 76, STAUD_Ioctl_DisableDeEmphasis_t*)

typedef STAUD_Ioctl_DisableDeEmphasis_t STAUD_Ioctl_DisableSynchronisation_t;

#define STAUD_IOC_DISABLESYNCHRONISATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 77, STAUD_Ioctl_DisableSynchronisation_t*)

typedef STAUD_Ioctl_DisableDeEmphasis_t STAUD_Ioctl_EnableDeEmphasis_t;

#define STAUD_IOC_ENABLEDEEMPHASIS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 78, STAUD_Ioctl_EnableDeEmphasis_t*)

typedef STAUD_Ioctl_DisableDeEmphasis_t STAUD_Ioctl_EnableSynchronisation_t;

#define STAUD_IOC_ENABLESYNCHRONISATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 79, STAUD_Ioctl_EnableSynchronisation_t*)

typedef STAUD_Ioctl_DisableDeEmphasis_t STAUD_Ioctl_Resume_t;

#define STAUD_IOC_RESUME   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 80, STAUD_Ioctl_Resume_t*)

typedef STAUD_Ioctl_DisableDeEmphasis_t STAUD_Ioctl_SetPrologic_t;

#define STAUD_IOC_SETPROLOGIC   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 81, STAUD_Ioctl_SetPrologic_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Attenuation_t  Attenuation;
} STAUD_Ioctl_GetAttenuation_t;

#define STAUD_IOC_GETATTENUATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 82, STAUD_Ioctl_GetAttenuation_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Delay_t        Delay;
} STAUD_Ioctl_GetChannelDelay_t;

#define STAUD_IOC_GETCHANNELDELAY   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 83, STAUD_Ioctl_GetChannelDelay_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_DigitalOutputConfiguration_t Mode;
} STAUD_Ioctl_GetDigitalOutput_t;

#define STAUD_IOC_GETDIGITALOUTPUT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 84, STAUD_Ioctl_GetDigitalOutput_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    U8                   Control;
} STAUD_Ioctl_GetDynamicRangeControl_t;

#define STAUD_IOC_GETDYNAMICRANGECONTROL   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 85, STAUD_Ioctl_GetDynamicRangeControl_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_StartParams_t  Params;
} STAUD_Ioctl_Start_t;

#define STAUD_IOC_START   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 86, STAUD_Ioctl_Start_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    BOOL                   AnalogueOutput;
    BOOL                   DigitalOutput;
} STAUD_Ioctl_Mute_t;

#define STAUD_IOC_MUTE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 87, STAUD_Ioctl_Mute_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Effect_t       EffectMode;
} STAUD_Ioctl_GetEffect_t;

#define STAUD_IOC_GETEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 88, STAUD_Ioctl_GetEffect_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Karaoke_t      KaraokeMode;
} STAUD_Ioctl_GetKaraokeOutput_t;

#define STAUD_IOC_GETKARAOKEOUTPUT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 89, STAUD_Ioctl_GetKaraokeOutput_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    BOOL                 PrologicState;
} STAUD_Ioctl_GetPrologic_t;

#define STAUD_IOC_GETPROLOGIC   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 90, STAUD_Ioctl_GetPrologic_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Speaker_t      Speaker;
} STAUD_Ioctl_GetSpeaker_t;

#define STAUD_IOC_GETSPEAKER   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 91, STAUD_Ioctl_GetSpeaker_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_SpeakerConfiguration_t Speaker;
    STAUD_BassMgtType_t BassType;
} STAUD_Ioctl_GetSpeakerConfiguration_t;

#define STAUD_IOC_GETSPEAKERCONFIGURATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 92, STAUD_Ioctl_GetSpeakerConfiguration_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Stereo_t       Mode;
} STAUD_Ioctl_GetStereoOutput_t;

#define STAUD_IOC_GETSTEREOOUTPUT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 93, STAUD_Ioctl_GetStereoOutput_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_SynchroUnit_t  SynchroUnit;
} STAUD_Ioctl_GetSynchroUnit_t;

#define STAUD_IOC_GETSYNCHROUNIT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 94, STAUD_Ioctl_GetSynchroUnit_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_Fade_t        Fade;
} STAUD_Ioctl_Pause_t;

#define STAUD_IOC_PAUSE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 95, STAUD_Ioctl_Pause_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    U32                 Unit;
} STAUD_Ioctl_PauseSynchro_t;

#define STAUD_IOC_PAUSESYNCHRO   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 96, STAUD_Ioctl_PauseSynchro_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_Prepare_t     Params;
} STAUD_Ioctl_Prepare_t;

#define STAUD_IOC_PREPARE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 97, STAUD_Ioctl_Prepare_t*)

typedef STAUD_Ioctl_GetAttenuation_t STAUD_Ioctl_SetAttenuation_t;

#define STAUD_IOC_SETATTENUATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 98, STAUD_Ioctl_SetAttenuation_t*)

typedef STAUD_Ioctl_GetChannelDelay_t STAUD_Ioctl_SetChannelDelay_t;

#define STAUD_IOC_SETCHANNELDELAY   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 99, STAUD_Ioctl_SetChannelDelay_t*)

typedef  STAUD_Ioctl_GetDigitalOutput_t STAUD_Ioctl_SetDigitalOutput_t;

#define STAUD_IOC_SETDIGITALOUTPUT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 100, STAUD_Ioctl_SetDigitalOutput_t*)

typedef  STAUD_Ioctl_GetDynamicRangeControl_t STAUD_Ioctl_SetDynamicRangeControl_t;

#define STAUD_IOC_SETDYNAMICRANGECONTROL   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 101, STAUD_Ioctl_SetDynamicRangeControl_t*)

typedef STAUD_Ioctl_GetEffect_t STAUD_Ioctl_SetEffect_t;

#define STAUD_IOC_SETEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 102, STAUD_Ioctl_SetEffect_t*)

typedef STAUD_Ioctl_GetKaraokeOutput_t STAUD_Ioctl_SetKaraokeOutput_t;

#define STAUD_IOC_SETKARAOKEOUTPUT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 103, STAUD_Ioctl_SetKaraokeOutput_t*)

typedef STAUD_Ioctl_GetSpeaker_t STAUD_Ioctl_SetSpeaker_t;

#define STAUD_IOC_SETSPEAKER   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 104, STAUD_Ioctl_SetSpeaker_t*)

typedef STAUD_Ioctl_GetSpeakerConfiguration_t STAUD_Ioctl_SetSpeakerConfiguration_t;

#define STAUD_IOC_SETSPEAKERCONFIGURATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 105, STAUD_Ioctl_SetSpeakerConfiguration_t*)

typedef STAUD_Ioctl_GetStereoOutput_t STAUD_Ioctl_SetStereoOutput_t;

#define STAUD_IOC_SETSTEREOOUTPUT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 106, STAUD_Ioctl_GetStereoOutput_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_Stop_t StopMode;
    STAUD_Fade_t Fade;
} STAUD_Ioctl_Stop_t;

#define STAUD_IOC_STOP   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 107, STAUD_Ioctl_Stop_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_AudioPlayerID_t PlayerID;
    STAUD_BufferParams_t DataParams;
} STAUD_Ioctl_GetInputBufferParams_t;

#define STAUD_IOC_GETINPUTBUFFERPARAMS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 108, STAUD_Ioctl_GetInputBufferParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    int                 InterruptNumber;
    int                 InterruptLevel;
} STAUD_Ioctl_IntHandlerInstall_t;

#define STAUD_IOC_INTHANDLERINSTALL   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 109, STAUD_Ioctl_IntHandlerInstall_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t        ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t        Handle;
    STAUD_AudioPlayerID_t PlayerID;
    U32                   Delay;
} STAUD_Ioctl_SkipSynchro_t;

#define STAUD_IOC_SKIPSYNCHRO   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 110, STAUD_Ioctl_SkipSynchro_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    BOOL                 Emphasis;

} STAUD_Ioctl_DRSetDeEmphasisFilter_t;

#define STAUD_IOC_DRSETDEEMPHASISFILTER   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 111, STAUD_Ioctl_DRSetDeEmphasisFilter_t*)

typedef STAUD_Ioctl_DRSetDeEmphasisFilter_t STAUD_Ioctl_DRGetDeEmphasisFilter_t;

#define STAUD_IOC_DRGETDEEMPHASISFILTER   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 112, STAUD_Ioctl_DRGetDeEmphasisFilter_t*)

typedef STAUD_Ioctl_PPSetEffect_t STAUD_Ioctl_DRSetEffect_t;

#define STAUD_IOC_DRSETEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 113, STAUD_Ioctl_DRSetEffect_t*)

typedef STAUD_Ioctl_DRSetEffect_t STAUD_Ioctl_DRGetEffect_t;

#define STAUD_IOC_DRGETEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 114, STAUD_Ioctl_DRGetEffect_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    BOOL                 Prologic;

} STAUD_Ioctl_DRSetPrologic_t;

#define STAUD_IOC_DRSETPROLOGIC   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 115, STAUD_Ioctl_DRSetPrologic_t*)

typedef STAUD_Ioctl_DRSetPrologic_t STAUD_Ioctl_DRGetPrologic_t;

#define STAUD_IOC_DRGETPROLOGIC   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 116, STAUD_Ioctl_DRGetPrologic_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_StereoMode_t   StereoMode;

} STAUD_Ioctl_DRSetStereoMode_t;

#define STAUD_IOC_DRSETSTEREOMODE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 117, STAUD_Ioctl_DRSetStereoMode_t*)

typedef STAUD_Ioctl_DRSetStereoMode_t STAUD_Ioctl_DRGetStereoMode_t;

#define STAUD_IOC_DRGETSTEREOMODE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 118, STAUD_Ioctl_DRGetStereoMode_t*)

typedef STAUD_Ioctl_OPMute_t STAUD_Ioctl_DRMute_t;

#define STAUD_IOC_DRMUTE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 119, STAUD_Ioctl_DRMute_t*)

typedef STAUD_Ioctl_DRMute_t STAUD_Ioctl_DRUnMute_t;

#define STAUD_IOC_DRUNMUTE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 120, STAUD_Ioctl_DRUnMute_t*)

typedef STAUD_Ioctl_Play_t STAUD_Ioctl_PlayerStart_t;

#define STAUD_IOC_PLAYERSTART   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 121, STAUD_Ioctl_PlayerStart_t*)

typedef STAUD_Ioctl_PlayerStart_t STAUD_Ioctl_PlayerStop_t;

#define STAUD_IOC_PLAYERSTOP   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 122, STAUD_Ioctl_PlayerStop_t*)

typedef STAUD_Ioctl_DRSetStereoMode_t STAUD_Ioctl_PPSetStereoModeNow_t;

#define STAUD_IOC_PPSETSTEREOMODENOW   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 123, STAUD_Ioctl_PPSetStereoModeNow_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PPObject;
    STAUD_DCRemove_t     Params;
} STAUD_Ioctl_PPDcRemove_t;

#define STAUD_IOC_PPDCREMOVE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 124, STAUD_Ioctl_PPDcRemove_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_CompressionMode_t CompressionMode;

} STAUD_Ioctl_DRSetCompressionMode_t;

#define STAUD_IOC_DRSETCOMPRESSIONMODE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 125, STAUD_Ioctl_DRSetCompressionMode_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_AudioCodingMode_t AudioCodingMode;

} STAUD_Ioctl_DRSetAudioCodingMode_t;

#define STAUD_IOC_DRSETAUDIOCODINGMODE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 126, STAUD_Ioctl_DRSetAudioCodingMode_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       InputObject;
    STAUD_PCMInputParams_t InputParams;

} STAUD_Ioctl_IPSetPcmParams_t;

#define STAUD_IOC_IPSETPCMPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 127, STAUD_Ioctl_IPSetPcmParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    STAUD_PCMBuffer_t PCMBuffer_p;
} STAUD_Ioctl_IPGetPCMBuffer_t;

#define STAUD_IOC_IPGETPCMBUFFER    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 128, STAUD_Ioctl_IPGetPCMBuffer_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    U32 BufferSize;
} STAUD_Ioctl_IPGetPCMBufferSize_t;

#define STAUD_IOC_IPGETPCMBUFFERSIZE    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 129, STAUD_Ioctl_IPGetPCMBufferSize_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_EffectSRSParams_t ParamType;
    S16 				Value_p;

} STAUD_Ioctl_DRGetSrsEffectParam_t;

#define STAUD_IOC_DRGETSRSEFFECTPARAM   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 130, STAUD_Ioctl_DRGetSrsEffectParam_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_DownmixParams_t DownMixParam;

} STAUD_Ioctl_DRSetDownMix_t;

#define STAUD_IOC_DRSETDOWNMIX   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 131, STAUD_Ioctl_DRSetDownMix_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_EffectSRSParams_t ParamsType;
    S16 				Value;
} STAUD_Ioctl_DRSetSrsEffect_t;

#define STAUD_IOC_DRSETSRSEFFECT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 132, STAUD_Ioctl_DRSetSrsEffect_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t MixerObject;
    U16 InputID;
    BOOL PTSStatus;
} STAUD_IOCTL_MXUpdatePTSStatus_t;

#define STAUD_IOC_MXUPDATEPTSSTATUS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 133, STAUD_IOCTL_MXUpdatePTSStatus_t*)



typedef struct
{    /* Error code retrieved by STAPI function */
     ST_ErrorCode_t          ErrorCode;
     /* Parameters to the function */
     STAUD_Handle_t       Handle;
     STCLKRV_Handle_t ClkSource;
} STAUD_Ioctl_SetClockRecoverySource_t;

#define STAUD_IOC_SETCLOCKRECOVERYSOURCE _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 134, STAUD_Ioctl_SetClockRecoverySource_t*)


typedef struct
{    /* Error code retrieved by STAPI function */
     ST_ErrorCode_t          ErrorCode;
     /* Parameters to the function */
     STAUD_Handle_t       Handle;
     STAUD_Object_t OutputObject;
} STAUD_Ioctl_OPEnableHDMIOutput_t;

#define STAUD_IOC_OPENABLEHDMIOUTPUT _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 135, STAUD_Ioctl_OPEnableHDMIOutput_t*)

typedef struct
{
	/* Error code retrieved by STAPI function */
	ST_ErrorCode_t	ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t	Handle;
	STAUD_Object_t 	DecoderObject;
	U32				Freq;
} STAUD_Ioctl_DRGetBeepToneFreq_t;

#define STAUD_IOC_DRGETBEEPTONEFREQ _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 136, STAUD_Ioctl_DRGetBeepToneFreq_t*)

typedef struct
{
	/* Error code retrieved by STAPI function */
	ST_ErrorCode_t	ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t	Handle;
	STAUD_Object_t 	DecoderObject;
	U32				BeepToneFrequency;
} STAUD_Ioctl_DRSetBeepToneFreq_t;

#define STAUD_IOC_DRSETBEEPTONEFREQ _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 137, STAUD_Ioctl_DRSetBeepToneFreq_t*)

typedef struct
{
	/* Error code retrieved by STAPI function */
	ST_ErrorCode_t	ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t	Handle;
	STAUD_Object_t 	MixerObject;
	U32				InputID;
	U16				MixLevel;
} STAUD_Ioctl_MXSetMixerLevel_t;

#define STAUD_IOC_MXSETMIXERLEVEL _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 138, STAUD_Ioctl_MXSetMixerLevel_t*)

typedef struct
{
	/* Error code retrieved by STAPI function */
	ST_ErrorCode_t			ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			InputObject;
	STAUD_BroadcastProfile_t 	BroadcastProfile;
	} STAUD_Ioctl_IPSetBroadcastProfile_t;

#define STAUD_IOC_IPSETBROADCASTPROFILE _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 139, STAUD_Ioctl_IPSetBroadcastProfile_t*)

typedef enum
{
    STAUD_CB_WRITE_PTR,
    STAUD_CB_READ_PTR
} STAUD_Ioctl_CallbackType_t;

typedef struct
{
    /* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Ioctl_CallbackType_t CBType;
    void*                Handle;
    void*                Address;
} STAUD_Ioctl_CallbackData_t;

#define STAUD_IOC_AWAIT_CALLBACK  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 140, STAUD_Ioctl_CallbackData_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t		ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t		Handle;
	STAUD_Object_t 		InputObject;
	U32   				Size;
} STAUD_Ioctl_IPGetBitBufferFreeSize_t;

#define STAUD_IOC_IPGETBITBUFFERFREESIZE  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 141, STAUD_Ioctl_IPGetBitBufferFreeSize_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			InputObject;
	STAUD_PCMReaderConf_t   ReaderParams;
} STAUD_Ioctl_IPSetPCMReaderParams_t;

#define STAUD_IOC_IPSETPCMREADERPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 142, STAUD_Ioctl_IPSetPCMReaderParams_t*)


typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			DecoderObject;
	BOOL 				DolbyDigitalEx;
} STAUD_Ioctl_DRSetDolbyDigitalEx_t;


#define STAUD_IOC_DRSETDOLBYDIGITALEX  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 143, STAUD_Ioctl_DRSetDolbyDigitalEx_t*)


typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			DecoderObject;
	BOOL 				DolbyDigitalEx_p;
} STAUD_Ioctl_DRGetDolbyDigitalEx_t;

#define STAUD_IOC_DRGETDOLBYDIGITALEX  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 144, STAUD_Ioctl_DRGetDolbyDigitalEx_t*)


typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			PostProcObject;
	STAUD_Equalization_t 		Equalization_p;
} STAUD_Ioctl_PPGetEqualizationParams_t;

#define STAUD_IOC_PPGETEQUALIZATIONPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 145, STAUD_Ioctl_PPGetEqualizationParams_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			PostProcObject;
	STAUD_Equalization_t 		Equalization_p;
} STAUD_Ioctl_PPSetEqualizationParams_t;

#define STAUD_IOC_PPSETEQUALIZATIONPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 146, STAUD_Ioctl_PPSetEqualizationParams_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			DecoderObject;
	STAUD_Omni_t 			Omni;
} STAUD_Ioctl_DRSetOmniParams_t;

#define STAUD_IOC_DRSETOMNIPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 147, STAUD_Ioctl_DRSetOmniParams_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			DecoderObject;
	STAUD_Omni_t 			Omni_p;
} STAUD_Ioctl_DRGetOmniParams_t;


#define STAUD_IOC_DRGETOMNIPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 148, STAUD_Ioctl_DRGetOmniParams_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			ModuleObject;
	STAUD_StreamParams_t  	StreamParams_p;
} STAUD_Ioctl_ModuleStart_t;


#define STAUD_IOC_MODULESTART  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 149, STAUD_Ioctl_ModuleStart_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
} STAUD_Ioctl_GenericStart_t;


#define STAUD_IOC_GENERICSTART  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 150, STAUD_Ioctl_GenericStart_t*)


typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 			InputObject;
	STAUD_PCMReaderConf_t   	PCMReaderParams;
} STAUD_Ioctl_IPGetPCMReaderParams_t;

#define STAUD_IOC_IPGETPCMREADERPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 151, STAUD_Ioctl_IPGetPCMReaderParams_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t		ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
	STAUD_Object_t 		ModuleObject;
} STAUD_Ioctl_ModuleStop_t;

#define STAUD_IOC_MODULESTOP  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 152, STAUD_Ioctl_ModuleStop_t*)

typedef struct
{
	/* Error code returned by the callback function */
    ST_ErrorCode_t		ErrorCode;
    /* Parameters to the function */
	STAUD_Handle_t			Handle;
} STAUD_Ioctl_GenericStop_t;

#define STAUD_IOC_GENERICSTOP  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 153, STAUD_Ioctl_GenericStop_t*)

typedef struct
{
	/* Error code returned by the callback function */
	ST_ErrorCode_t          ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t		Handle;
	STAUD_Object_t 		MixerObject;
	U32					InputID;
	BOOL					FreeRun;
} STAUD_Ioctl_MXUpdateMaster_t;

#define STAUD_IOC_MXUPDATEMASTER  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 154, STAUD_Ioctl_MXUpdateMaster_t*)

typedef struct
{
	/* Error code returned by the callback function */
	ST_ErrorCode_t          ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t		Handle;
	STAUD_Object_t 		OutputObject;
	U32					Latency;
} STAUD_Ioctl_OPSetLatency_t;

#define STAUD_IOC_OPSETLATENCY  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 155, STAUD_Ioctl_OPSetLatency_t*)

typedef struct
{
	/* Error code retrieved by STAPI function */
	ST_ErrorCode_t          	ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t       	Handle;
	STAUD_Object_t 		DecoderObject;
	STAUD_PLIIParams_t 	PLIIParams;
} STAUD_Ioctl_DRSetAdvancePLII_t;

#define STAUD_IOC_DRSETADVPLII   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 156, STAUD_Ioctl_DRSetAdvancePLII_t*)

typedef STAUD_Ioctl_DRSetAdvancePLII_t STAUD_Ioctl_DRGetAdvancePLII_t;

#define STAUD_IOC_DRGETADVPLII   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 157, STAUD_Ioctl_DRGetAdvancePLII_t*)

typedef struct
{
	/* Error code retrieved by STAPI function */
	ST_ErrorCode_t          	ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t       	Handle;
	STAUD_Object_t 		inputObject;
	STAUD_SynchroUnit_t SynchroUnit;
} STAUD_Ioctl_IPGetSynchroUnit_t;

#define STAUD_IOC_IPGETSYNCHROUNIT   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 158, STAUD_Ioctl_IPGetSynchroUnit_t*)

typedef STAUD_Ioctl_OPEnableHDMIOutput_t STAUD_Ioctl_OPDisableHDMIOutput_t;

#define STAUD_IOC_OPDISABLEHDMIOUTPUT _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 159, STAUD_Ioctl_OPDisableHDMIOutput_t*)

typedef struct
{
	/* Error code returned by the callback function */
	ST_ErrorCode_t          ErrorCode;
	/* Parameters to the function */
	STAUD_Handle_t		Handle;
	STAUD_Object_t 		OutputObject;
	BOOL 				EnableDisableEncOutput;
} STAUD_Ioctl_OPEncSetDigital_t;

#define STAUD_IOC_OPENCSETDIGITAL  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 160, STAUD_Ioctl_OPEncSetDigital_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_CircleSurrondII_t       Csii;

} STAUD_Ioctl_DRCircleSrround_t;

typedef STAUD_Ioctl_DRCircleSrround_t  STAUD_Ioctl_DRGetCircleSrround_t;
#define STAUD_IOC_DRGETCIRCLE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 161, STAUD_Ioctl_DRGetCircleSrround_t*)

typedef STAUD_Ioctl_DRGetCircleSrround_t STAUD_Ioctl_DRSetCircleSrround_t;
#define  STAUD_IOC_DRSETCIRCLE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 162, STAUD_Ioctl_DRSetCircleSrround_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       InputObject;
} STAUD_Ioctl_IPHandleEOF_t;

#define STAUD_IOC_IPHANDLEEOF		_IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 163, STAUD_Ioctl_IPHandleEOF_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_Object_t      DecoderObject;
	 U32                 DDPOPSetting;
} STAUD_Ioctl_DRSetDDPOP_t;

#define STAUD_IOC_DRSETDDPOP		_IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 164, STAUD_Ioctl_DRSetDDPOP_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STAUD_Object_t          EncoderObject;
    STAUD_ENCCapability_t   Capability;
} STAUD_Ioctl_ENCGetCapability_t;
#define STAUD_IOC_ENCGETCAPABILITY  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 165, STAUD_Ioctl_ENCGetCapability_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t          Handle;
    STAUD_Object_t          SrcObject;
	 U32                     SrcId;
    STAUD_Object_t          DstObject;
	 U32                     DstId;
} STAUD_Ioctl_ConnectSrcDst_t;
#define STAUD_IOC_CONNECTSRCDST  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 166, STAUD_Ioctl_ConnectSrcDst_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t          Handle;
    STAUD_Object_t          Object;
	 U32                     InputId;
} STAUD_Ioctl_DisconnectInput_t;
#define STAUD_IOC_DISCONNECTINPUT  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 167, STAUD_Ioctl_DisconnectInput_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t          EncoderObject;
    STAUD_ENCOutputParams_s   EncoderOPParams;
} STAUD_Ioctl_ENCSetOutputParams_t;
#define STAUD_IOC_ENCSETOUTPUTPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 168, STAUD_Ioctl_ENCSetOutputParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t          EncoderObject;
    STAUD_ENCOutputParams_s   EncoderOPParams;
} STAUD_Ioctl_ENCGetOutputParams_t;
#define STAUD_IOC_ENCGETOUTPUTPARAMS  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 169, STAUD_Ioctl_ENCGetOutputParams_t*)

typedef struct
{

    ST_ErrorCode_t   ErrorCode;
    /* Parameters to the function */

    STAUD_Handle_t Handle;
    STAUD_Object_t  inputObject;
    FrameDeliverFunc_t  Func_fp;
    void * clientInfo;
} STAUD_Ioctl_DPSetCallback;

#define STAUD_IOC_DPSETCALLBACK  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 170, STAUD_Ioctl_DPSetCallback*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    STAUD_Handle_t Handle;
    STAUD_Object_t InputObject;
    STAUD_GetBufferParam_t BufferParam;
} STAUD_Ioctl_GetBufferParam_t;

#define STAUD_IOC_GETBUFFERPARAM    _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 171, STAUD_Ioctl_GetBufferParam_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    FrameDeliverFunc_t FrameDeliverFunc_fp;
    U8 		*MemoryStart;
    U32 	Size;
	BufferMetaData_t bufferMetaData;
    void 	*clientInfo;
} STAUD_Ioctl_FPCallbackData_t;

#define STAUD_IOC_AWAIT_FPCALLBACK  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 172, STAUD_Ioctl_FPCallbackData_t*)


typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       Object;
    STAUD_Attenuation_t  Attenuation;

} STAUD_Ioctl_SetModuleAttenuation_t;

#define STAUD_IOC_SETMODULEATTENUATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 173, STAUD_Ioctl_SetModuleAttenuation_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       Object;
    STAUD_Attenuation_t  Attenuation;

} STAUD_Ioctl_GetModuleAttenuation_t;

#define STAUD_IOC_GETMODULEATTENUATION   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 174, STAUD_Ioctl_GetModuleAttenuation_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_DTSNeo_t   DTSNeo;

} STAUD_Ioctl_DRSetDTSNeo_t;

#define STAUD_IOC_DRSETDTSNEO  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 175, STAUD_Ioctl_DRSetDTSNeo_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
      STAUD_DTSNeo_t   DTSNeo;

} STAUD_Ioctl_DRGetDTSNeo_t;

#define STAUD_IOC_DRGETDTSNEO   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 176, STAUD_Ioctl_DRGetDTSNeo_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PPObject;
    STAUD_BTSC_t   	BTSC;

} STAUD_Ioctl_PPSetBTSC_t;

#define STAUD_IOC_DRSETBTSC  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 177, STAUD_Ioctl_PPSetBTSC_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       PPObject;
    STAUD_BTSC_t   	BTSC;

} STAUD_Ioctl_PPGetBTSC_t;

#define STAUD_IOC_DRGETBTSC  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 178, STAUD_Ioctl_PPGetBTSC_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t      ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t      Handle;
    STAUD_Scenario_t    Scenario;

} STAUD_Ioctl_SetScenario_t;

#define STAUD_IOC_SETSCENARIO  _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 179, STAUD_Ioctl_SetScenario_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t       Handle;
    STAUD_Object_t       DecoderObject;
    STAUD_DRInitParams_t   DRInitParams;

} STAUD_Ioctl_DRSetInitParams_t;

#define STAUD_IOC_DRSETINITPARAMS   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 180, STAUD_Ioctl_DRSetInitParams_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;
    /* Parameters to the function */
    STAUD_Handle_t         Handle;
    STAUD_Object_t         DecoderObject;
    STAUD_StreamInfo_t     StreamInfo;

} STAUD_Ioctl_DRGetStreamInfo_t;

#define STAUD_IOC_DRGETSTREAMINFO     _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 181, STAUD_Ioctl_DRGetStreamInfo_t *)

typedef STAUD_Ioctl_DRSetStereoMode_t STAUD_Ioctl_DRGetInStereoMode_t;
#define STAUD_IOC_DRGETINSTEREOMODE   _IOWR(STAUDLX_IOCTL_MAGIC_NUMBER, 182, STAUD_Ioctl_DRGetInStereoMode_t*)

#endif
