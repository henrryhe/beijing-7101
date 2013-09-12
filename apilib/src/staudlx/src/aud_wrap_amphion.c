/************************************************************************
COPYRIGHT STMicroelectronics (C) 2000
Source file name : aud_wrap.c
************************************************************************/

/*****************************************************************************
Includes
*****************************************************************************/
#ifndef ST_OSLINUX
#include <string.h>
#endif
#include "stos.h"
#include "stddefs.h"
#include "staudlx.h"
#include "aud_pcmplayer.h"
#include "aud_spdifplayer.h"
#include "aud_utils.h"
#ifndef STB
#define STB
#endif
#ifndef STAUD_NO_WRAPPER_LAYER

extern U32  GetClassHandles(STAUD_Handle_t Handle,U32   ObjectClass, STAUD_Object_t *ObjectArray);

/*-----------------------------------------------------------------------------
                Decoding control functions
  ---------------------------------------------------------------------------*/
ST_ErrorCode_t  STAUD_DisableDeEmphasis(
                STAUD_Handle_t Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        //ErrorCode = STAUD_DRSetDeEmphasisFilter ( Handle, ObjectArray[ObjectCount], FALSE );
    }
     return ErrorCode;
}

#if defined (STB)
ST_ErrorCode_t  STAUD_DisableSynchronisation(
                STAUD_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_OUTPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        // Call to be modified to STAUD_DRDisableSynchronization
        ErrorCode = STAUD_OPDisableSynchronization( Handle, ObjectArray[ObjectCount] );
    }

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_INPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        // Call to be modified to STAUD_DRDisableSynchronization
        ErrorCode = STAUD_OPDisableSynchronization( Handle, ObjectArray[ObjectCount] );
    }


    return ErrorCode;
}
#endif


#if defined (STB)
ST_ErrorCode_t  STAUD_EnableSynchronisation(
                STAUD_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_OUTPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        // Call to be modified to STAUD_DREnableSynchronization
        ErrorCode = STAUD_OPEnableSynchronization( Handle, ObjectArray[ObjectCount] );
    }

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_INPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        // Call to be modified to STAUD_DREnableSynchronization
        ErrorCode = STAUD_OPEnableSynchronization( Handle, ObjectArray[ObjectCount] );
    }

    return ErrorCode;
}
#endif

#ifndef STAUD_REMOVE_CLKRV_SUPPORT

ST_ErrorCode_t STAUD_SetClockRecoverySource (STAUD_Handle_t Handle, STCLKRV_Handle_t ClkSource)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_OUTPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRSetClockRecoverySource( Handle, ObjectArray[ObjectCount], ClkSource);
    }

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_INPUT, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRSetClockRecoverySource( Handle, ObjectArray[ObjectCount], ClkSource);
    }


    return ErrorCode;

}

#endif

ST_ErrorCode_t  STAUD_GetAttenuation(
                STAUD_Handle_t  Handle,
                STAUD_Attenuation_t  *Attenuation_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_GetModuleAttenuation( Handle, ObjectArray[ObjectCount], Attenuation_p);
    }
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_GetChannelDelay(
                STAUD_Handle_t  Handle,
                STAUD_Delay_t   *Delay_p)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_GetDigitalOutput(
                STAUD_Handle_t      Handle,
                STAUD_DigitalOutputConfiguration_t *Mode_p)
{
    ST_ErrorCode_t       ErrorCode;
    STAUD_OutputParams_t OutputParams;

    ErrorCode = STAUD_OPGetParams ( Handle, STAUD_OBJECT_OUTPUT_SPDIF0, &OutputParams);//???

    if( ErrorCode == ST_NO_ERROR )
    {
        ErrorCode = STAUD_OPGetDigitalMode (Handle, STAUD_OBJECT_OUTPUT_SPDIF0, &Mode_p->DigitalMode);

    }

    /* parse structure OutputParams, generate STAUD_DigitalOutputConfiguration_t */
    Mode_p->Copyright = OutputParams.SPDIFOutParams.CopyPermitted;

    if (OutputParams.SPDIFOutParams.AutoLatency)
    {
        Mode_p->Latency = 0;
    }
    else
    {
        Mode_p->Latency = (U32)(OutputParams.SPDIFOutParams.Latency);
    }

    return ErrorCode;
}






ST_ErrorCode_t  STAUD_GetEffect(
                STAUD_Handle_t  Handle,
                STAUD_Effect_t  *Mode_p)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return ErrorCode;
}

ST_ErrorCode_t  STAUD_GetStereoOutput(
                STAUD_Handle_t  Handle,
                STAUD_Stereo_t  *Mode_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;
    STAUD_StereoMode_t  StereoMode;



    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRGetStereoMode( Handle, ObjectArray[ObjectCount],&StereoMode );
    }

    /* Convert STAUD_StereoMode_t to STAUD_Stereo_t */
    switch (StereoMode)
    {
        default:
        case STAUD_STEREO_MODE_STEREO:
            *Mode_p = STAUD_STEREO_STEREO;
            break;
        case STAUD_STEREO_MODE_PROLOGIC:
            *Mode_p = STAUD_STEREO_PROLOGIC;
            break;
        case STAUD_STEREO_MODE_DUAL_LEFT:
            *Mode_p = STAUD_STEREO_DUAL_LEFT;
            break;
        case STAUD_STEREO_MODE_DUAL_RIGHT:
            *Mode_p = STAUD_STEREO_DUAL_RIGHT;
            break;
        case STAUD_STEREO_MODE_DUAL_MONO:
            *Mode_p = STAUD_STEREO_DUAL_MONO;
            break;
        case STAUD_STEREO_MODE_SECOND_STEREO:
            *Mode_p = STAUD_STEREO_SECOND_STEREO;
            break;
    }

    return ErrorCode;
}
ST_ErrorCode_t  STAUD_Mute(
                STAUD_Handle_t  Handle,
                BOOL            AnalogueOutput,
                BOOL            DigitalOutput)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0, tempObjectCount = 0;

#if MUTE_AT_DECODER
    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);

    if (AnalogueOutput)
    {
        while (ObjectCount)
        {
            ObjectCount --;
            //ErrorCode = STAUD_DRMute( Handle, ObjectArray[ObjectCount] );
        }
    }
    else
    {
        while (ObjectCount)
        {
            ObjectCount --;
            //ErrorCode = STAUD_DRUnMute( Handle, ObjectArray[ObjectCount] );
        }
    }
#else
    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_OUTPUT, ObjectArray);

    if (AnalogueOutput)
    {
        tempObjectCount = ObjectCount;
        while (tempObjectCount)
        {
            tempObjectCount --;
            if ((ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP0) || (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP1)
                || (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP2)|| (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP3))
                ErrorCode = STAUD_OPMute( Handle, ObjectArray[tempObjectCount] );
        }

    }
    else
    {
        tempObjectCount = ObjectCount;
        while (tempObjectCount)
        {
            tempObjectCount --;
            if ((ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP0) || (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP1)
                || (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP2) || (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_PCMP3))
                ErrorCode = STAUD_OPUnMute( Handle, ObjectArray[tempObjectCount] );
        }
    }

#endif

    if(DigitalOutput)
    {

        tempObjectCount = ObjectCount;
        while (tempObjectCount)
        {
            tempObjectCount --;
            if (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_SPDIF0)
                ErrorCode = STAUD_OPMute( Handle, ObjectArray[tempObjectCount] );
        }
    }
    else
    {
        tempObjectCount = ObjectCount;
        while (tempObjectCount)
        {
            tempObjectCount --;
            if (ObjectArray[tempObjectCount] == STAUD_OBJECT_OUTPUT_SPDIF0)
                ErrorCode = STAUD_OPUnMute( Handle, ObjectArray[tempObjectCount] );
        }
    }

    return ErrorCode;
}


ST_ErrorCode_t  STAUD_Pause(
                STAUD_Handle_t  Handle,
                STAUD_Fade_t   *Fade_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_INPUT, ObjectArray);
    while (ObjectCount)
    {
        ObjectCount --;
            ErrorCode = STAUD_DRPause( Handle, ObjectArray[ObjectCount], Fade_p);
    }
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_Play(
                STAUD_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);
    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRStart( Handle,  ObjectArray[ObjectCount], NULL );
    }
    return ErrorCode;
}


#if defined (DVD)
ST_ErrorCode_t  STAUD_PauseSynchro(
                STAUD_Handle_t  Handle,
                U32             Unit)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_INPUT, ObjectArray);
    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_IPPauseSynchro( Handle, ObjectArray[ObjectCount], Unit );
    }
    return ErrorCode;
}
#endif

#if defined (DVD)
ST_ErrorCode_t  STAUD_Prepare(
                STAUD_Handle_t Handle,
                STAUD_Prepare_t *Params)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;
    STAUD_StreamParams_t StreamParams;

    StreamParams.StreamContent = Params->StreamContent;
    StreamParams.StreamType = Params->StreamType;
    StreamParams.SamplingFrequency = Params->SamplingFrequency;
    StreamParams.StreamID = STAUD_IGNORE_ID;


    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);
    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRPrepare(Handle, ObjectArray[ObjectCount], &StreamParams);
    }
    return ErrorCode;
}
#endif


ST_ErrorCode_t  STAUD_Resume(
                STAUD_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_INPUT, ObjectArray);
    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRResume( Handle, ObjectArray[ObjectCount] );
    }
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_SetAttenuation(
                STAUD_Handle_t  Handle,
                STAUD_Attenuation_t  Attenuation)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_SetModuleAttenuation( Handle, ObjectArray[ObjectCount], &Attenuation );
    }

    return ErrorCode;
}


ST_ErrorCode_t  STAUD_SetChannelDelay(
                STAUD_Handle_t  Handle,
                STAUD_Delay_t   Delay)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_SetDigitalOutput(
                STAUD_Handle_t      Handle,
                STAUD_DigitalOutputConfiguration_t Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_OutputParams_t OutputParams;

    if ( Mode.DigitalMode == STAUD_DIGITAL_MODE_OFF )
    {
        ErrorCode |= STAUD_OPSetDigitalMode (Handle, STAUD_OBJECT_OUTPUT_SPDIF0, STAUD_DIGITAL_MODE_OFF);
    }
    else
    {
        if ( Mode.Latency == 0 )     /*autolatency*/
        {
            OutputParams.SPDIFOutParams.AutoLatency = TRUE;
        }
        else                       /* only relevant if Autolatency is false */
        {
            OutputParams.SPDIFOutParams.Latency = (U16)(Mode.Latency);
        }

        /* Configure SPDIF output */
        OutputParams.SPDIFOutParams.CopyPermitted = Mode.Copyright;
        OutputParams.SPDIFOutParams.AutoCategoryCode = TRUE;
        OutputParams.SPDIFOutParams.CategoryCode = 0;
        OutputParams.SPDIFOutParams.AutoDTDI = TRUE;
        OutputParams.SPDIFOutParams.DTDI = 0;
        OutputParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 256;
        OutputParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode       = STAUD_SPDIF_DATA_PRECISION_24BITS;
        OutputParams.SPDIFOutParams.Emphasis                = STAUD_SPDIF_EMPHASIS_NOT_INDICATED;

        ErrorCode = STAUD_OPSetParams (Handle, STAUD_OBJECT_OUTPUT_SPDIF0, &OutputParams);

        if ( ErrorCode == ST_NO_ERROR )
        {
            /* Connect SPDIF output to it's source */
            if ( Mode.DigitalMode == STAUD_DIGITAL_MODE_COMPRESSED )
            {
                ErrorCode |= STAUD_OPSetDigitalMode (Handle, STAUD_OBJECT_OUTPUT_SPDIF0, STAUD_DIGITAL_MODE_COMPRESSED);
            }
            else /* ( Mode.DigitalMode == STAUD_DIGITAL_MODE_UNCOMPRESSED ) */
            {
                ErrorCode |= STAUD_OPSetDigitalMode (Handle, STAUD_OBJECT_OUTPUT_SPDIF0, STAUD_DIGITAL_MODE_NONCOMPRESSED);
            }
        }
    }
    return ErrorCode;
}




ST_ErrorCode_t  STAUD_SetSpeaker(
                STAUD_Handle_t  Handle,
                STAUD_Speaker_t Speaker)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_SetSpeakerConfiguration(
                STAUD_Handle_t                  Handle,
                STAUD_SpeakerConfiguration_t    Speaker,
                STAUD_BassMgtType_t BassType)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return ErrorCode;
}


ST_ErrorCode_t  STAUD_SetStereoOutput(
                STAUD_Handle_t  Handle,
                STAUD_Stereo_t  Mode)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    STAUD_Object_t ObjectArray[MAX_OBJECTS_PER_CLASS];
    U32 ObjectCount = 0;
    STAUD_StereoMode_t StereoMode;

    /* Convert STAUD_Stereo_t to STAUD_StereoMode_t */
    switch (Mode)
    {
        default:
        case STAUD_STEREO_MONO:
        case STAUD_STEREO_STEREO:
            StereoMode = STAUD_STEREO_MODE_STEREO;
            break;
        case STAUD_STEREO_PROLOGIC:
            StereoMode = STAUD_STEREO_MODE_PROLOGIC;
            break;
        case STAUD_STEREO_DUAL_LEFT:
            StereoMode = STAUD_STEREO_MODE_DUAL_LEFT;
            break;
        case STAUD_STEREO_DUAL_RIGHT:
            StereoMode = STAUD_STEREO_MODE_DUAL_RIGHT;
            break;
        case STAUD_STEREO_DUAL_MONO:
            StereoMode = STAUD_STEREO_MODE_DUAL_MONO;
            break;
        case STAUD_STEREO_SECOND_STEREO:
            StereoMode = STAUD_STEREO_MODE_SECOND_STEREO;
            break;
    }

    ObjectCount = GetClassHandles(Handle,   STAUD_CLASS_DECODER, ObjectArray);

    while (ObjectCount)
    {
        ObjectCount --;
        ErrorCode = STAUD_DRSetStereoMode( Handle, ObjectArray[ObjectCount], StereoMode );
    }

    return ErrorCode;
}


#if defined (DVD)
ST_ErrorCode_t  STAUD_SkipSynchro(
                STAUD_Handle_t  Handle,
                U32             Unit)
{
    ST_ErrorCode_t ErrorCode = ST_ERROR_FEATURE_NOT_SUPPORTED;
    return ErrorCode;
}
#endif


#if defined (STB)
ST_ErrorCode_t  STAUD_Start(
                STAUD_Handle_t  Handle,
                STAUD_StartParams_t  *Params_p)
{
    ST_ErrorCode_t ErrorCode;
    STAUD_StreamParams_t StreamParams;

    StreamParams.StreamContent      = Params_p->StreamContent;
    StreamParams.StreamType         = Params_p->StreamType;
    StreamParams.SamplingFrequency  = Params_p->SamplingFrequency;
    StreamParams.StreamID           = Params_p->StreamID;

    ErrorCode = STAUD_DRStart( Handle, STAUD_OBJECT_DECODER_COMPRESSED0, &StreamParams );

    return ErrorCode;
}
#endif


ST_ErrorCode_t  STAUD_Stop(
                STAUD_Handle_t  Handle,
                STAUD_Stop_t    StopMode,
                STAUD_Fade_t   *Fade_p)
{
    ST_ErrorCode_t ErrorCode;
    ErrorCode = STAUD_DRStop( Handle, STAUD_OBJECT_DECODER_COMPRESSED0, StopMode, Fade_p );
    return ErrorCode;
}

extern STPCMPLAYER_Handle_t pcmPlayerHandle;
extern STSPDIFPLAYER_Handle_t   spdifPlayerHandle;
//extern STAud_DecHandle_t      DecoderHandle;
extern STDataProcesser_Handle_t dataProcesserHandle;

//extern BOOL pcmplayerstopped;
BOOL decmute = FALSE;

extern STAUD_Handle_t audHandle;
extern STAUD_StreamParams_t audStreamParams;

ST_ErrorCode_t  STAUD_PlayerStart(STAUD_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    ErrorCode = STAUD_Start(audHandle, (STAUD_StartParams_t *)&audStreamParams);

    return ErrorCode;
}

ST_ErrorCode_t  STAUD_PlayerStop(STAUD_Handle_t  Handle)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
     STAUD_Fade_t   AUD_Fading;

    ErrorCode = STAUD_Stop(audHandle, STAUD_STOP_NOW,&AUD_Fading);

    return ErrorCode;
}
#endif

/* ------------------------------- End of file ---------------------------- */
