/******************************************************************************
File name   : aud_cmd.c

Description : Audio driver related functions

COPYRIGHT (C) STMicroelectronics 2007

References  :

*******************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "stddefs.h"
#include "stlite.h"
#include "staudlx.h"
#include "stdevice.h"
#include "stsys.h"
#include "stcommon.h"
#include "sttbx.h"
#include "stvout.h"
#include "aud_cmd.h"
#include "initterm.h"
#include "ch_hdmi.h"

#define TESTAPP_AUDIO_INTERNAL_DAC
#define TESTAPP_AUDIO_DUAL_DECODE
#define DEBUG_PRINT STTBX_Print
/* Private Types ------------------------------------------------------------ */
typedef struct AUD_EvtHolder_s
{
    STEVT_CallReason_t      Reason;
    STEVT_EventConstant_t   Event;
    U32                     EventData;
} AUD_EvtHolder_t;

/* Private Constants -------------------------------------------------------- */
#define UNMUTED  FALSE
#define MUTED    TRUE

/* Private Variables -------------------------------------------------------- */
ST_DeviceName_t AUDIO_Names[AUDIO_DEVICE_NUMBER]={"AUDIO0","AUDIO1"};
STAUD_Handle_t  AUDIO_Handler[AUDIO_DEVICE_NUMBER];
BOOL AudioMuteState = UNMUTED;

extern ST_Partition_t              *SystemPartition;
extern STAVMEM_PartitionHandle_t    AVMEMHandle[];
extern STHDMI_Handle_t          HDMIHandle;
extern ST_DeviceName_t CLKRVDeviceName;
extern ST_DeviceName_t EVTDeviceName;
extern STEVT_Handle_t EVTHandle;
extern STVOUT_Handle_t VOUTHandle[3];

extern ST_DeviceName_t AUDDeviceName;
extern STAUD_Handle_t AUDHandle;


static message_queue_t             *AUD_EvtMsg_p  = NULL;
static task_t                      *AUD_EvtTaskHandle = NULL;
static BOOL                         AUD_EvtTaskDelete = FALSE;
semaphore_t                        *AUD_EvtTaskRemoved;

/* Private Macros ----------------------------------------------------------- */

/* Private Function prototypes ---------------------------------------------- */
static void AUD_EvtTask(void *v);

/* Functions -------------------------------------------------------------- */

/*------------------------------------------------------------------------------
 * Function    : AUD_Init
 * Description : Init audio device
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Init(AUD_DeviceId_t DeviceId)
{
    ST_ErrorCode_t           ErrCode = ST_NO_ERROR;
    STAUD_InitParams_t       AUD_InitParams;
    STAUD_OpenParams_t       AUD_OpenParams;
    STAUD_BroadcastProfile_t AUD_BroadcastProfile;
    STAUD_DynamicRange_t     AUD_DynamicRange;
    #ifndef TESTAPP_AUDIO_INTERNAL_DAC
        U32                  CutId = ST_GetCutRevision();
    #endif

    memset(&AUD_InitParams      ,0,sizeof(STAUD_InitParams_t));
    memset(&AUD_OpenParams      ,0,sizeof(STAUD_OpenParams_t));
    memset(&AUD_DynamicRange    ,0,sizeof(STAUD_DynamicRange_t));
    memset(&AUD_BroadcastProfile,0,sizeof(STAUD_BroadcastProfile_t));
    AUD_InitParams.DeviceType                                   = STAUD_DEVICE_STI7100;
    AUD_InitParams.Configuration                                = STAUD_CONFIG_DVB_COMPACT;
    AUD_InitParams.InterruptNumber                              = 0;
    AUD_InitParams.InterruptLevel                               = 0;

    AUD_InitParams.BufferPartition                              = 0;
    AUD_InitParams.AllocateFromEMBX                             = TRUE;
    AUD_InitParams.AVMEMPartition                               = AVMEMHandle[0];

    AUD_InitParams.NumChannels                                  = 2;
    #ifdef AUDIO_MULTICHANNEL
      #ifndef TESTAPP_AUDIO_DUAL_DECODE
        AUD_InitParams.NumChannels                              = 10;
      #endif
    #endif
    AUD_InitParams.DACClockToFsRatio                            = 256;
    AUD_InitParams.PCMOutParams.InvertWordClock                 = FALSE;
    AUD_InitParams.PCMOutParams.Format                          = STAUD_DAC_DATA_FORMAT_I2S;
    AUD_InitParams.PCMOutParams.InvertBitClock                  = FALSE;
    AUD_InitParams.PCMOutParams.Precision                       = STAUD_DAC_DATA_PRECISION_24BITS;
    AUD_InitParams.PCMOutParams.Alignment                       = STAUD_DAC_DATA_ALIGNMENT_LEFT;
    AUD_InitParams.PCMOutParams.MSBFirst                        = TRUE;
    AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier    = 256;

    /* To be able to give pcm to HDMI we need a value 128 */
    if(DeviceId == AUD_INT)
    {
        #ifdef TESTAPP_AUDIO_INTERNAL_DAC
            AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier    = 256;/*NEW*/
        #else
            if(CutId >= 0xC0)
            {
                AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier    = 256;/*NEW*/
            }
            else
            {
                AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier    = 128;/*OLD*/
            }
        #endif
    }
    else
    {
        AUD_InitParams.PCMOutParams.PcmPlayerFrequencyMultiplier    = 256;/*NEW*/
    }
    AUD_InitParams.SPDIFOutParams.AutoLatency                   = TRUE;
    AUD_InitParams.SPDIFOutParams.AutoCategoryCode              = TRUE;
    AUD_InitParams.SPDIFOutParams.CategoryCode                  = 0;/*NEW*/
    AUD_InitParams.SPDIFOutParams.AutoDTDI                      = TRUE;
    AUD_InitParams.SPDIFOutParams.CopyPermitted                 = STAUD_COPYRIGHT_MODE_NO_COPY;
    AUD_InitParams.SPDIFOutParams.SPDIFPlayerFrequencyMultiplier= 128;
    AUD_InitParams.SPDIFOutParams.SPDIFDataPrecisionPCMMode     = STAUD_SPDIF_DATA_PRECISION_24BITS;
    AUD_InitParams.SPDIFOutParams.Emphasis                      = STAUD_SPDIF_EMPHASIS_CD_TYPE;
    AUD_InitParams.MaxOpen                                      = 1;
    AUD_InitParams.CPUPartition_p                               = SystemPartition;
    AUD_InitParams.PCMMode                                      = PCM_ON;
    #ifdef AUDIO_SPDIF_COMRESSED
        AUD_InitParams.SPDIFMode                                = STAUD_DIGITAL_MODE_COMPRESSED;
    #else
        AUD_InitParams.SPDIFMode                                = STAUD_DIGITAL_MODE_NONCOMPRESSED;
    #endif
#if 0
    #ifdef TESTAPP_AUD_PCMMIXING
        AUD_InitParams.DriverIndex    =8;
    #elif TESTAPP_AUDIO_INTERNAL_DAC
        AUD_InitParams.DriverIndex    =1;
    #else
        AUD_InitParams.DriverIndex    =0;
    #endif
#else
AUD_InitParams.DriverIndex    =1;
#endif

    #ifdef TESTAPP_AUDIO_DUAL_DECODE
        if(DeviceId == AUD_INT)
        {
            #ifdef TESTAPP_AUD_PCMMIXING
                AUD_InitParams.DriverIndex    =8;
            #else
                AUD_InitParams.DriverIndex  =0;
            #endif
        }
        else
        {
            AUD_InitParams.DriverIndex  =4;
        }
    #endif

    #ifdef AAC_TO_DTS_TRANSCODING
        AUD_InitParams.DriverIndex  = 35;
    #endif


    strcpy(AUD_InitParams.EvtHandlerName, EVTDeviceName);
    #ifdef TESTAPP_AUDIO_DUAL_DECODE
        if(DeviceId == AUD_INT)
        {
            strcpy(AUD_InitParams.ClockRecoveryName, CLKRVDeviceName);
        }
        else
        {
            /* CLKRV_Names[1] will be set os clkrv source in aud_start()*/
            strcpy(AUD_InitParams.ClockRecoveryName, CLKRVDeviceName);
        }
    #else
        strcpy(AUD_InitParams.ClockRecoveryName, CLKRVDeviceName);
    #endif

    ErrCode = STAUD_Init(/*AUDIO_Names[DeviceId]*/AUDDeviceName, &AUD_InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_Init(%s, DrvIndx:%d) -> FAILED(%d)", /*AUDIO_Names[DeviceId]*/AUDDeviceName, AUD_InitParams.DriverIndex, ErrCode));
        return ErrCode;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_Init(%s, DrvIndx:%d) -> OK", /*AUDIO_Names[DeviceId]*/AUDDeviceName, AUD_InitParams.DriverIndex));
    }

    AUD_OpenParams.SyncDelay = 0;
    ErrCode = STAUD_Open(AUDDeviceName/*AUDIO_Names[DeviceId]*/,&AUD_OpenParams,&AUDHandle/*AUDIO_Handler[DeviceId]*/);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_Open(%s, Hdl:%d) -> FAILED(%d)", /*AUDIO_Names[DeviceId]*/AUDDeviceName, AUDIO_Handler[DeviceId], ErrCode));
        return ErrCode;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_Open(%s, Hdl:%d) -> OK", /*AUDIO_Names[DeviceId]*/AUDDeviceName, AUDIO_Handler[DeviceId]));
    }

#if 0
    AUD_BroadcastProfile = STAUD_BROADCAST_DVB;
    #ifndef TESTAPP_AUDIO_DUAL_DECODE
        ErrCode = STAUD_IPSetBroadcastProfile (AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, AUD_BroadcastProfile);
    #else
        if(DeviceId==AUD_INT)
        {
            ErrCode=STAUD_IPSetBroadcastProfile (AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, AUD_BroadcastProfile);
        }
        else
        {
            ErrCode=STAUD_IPSetBroadcastProfile (AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED1, AUD_BroadcastProfile);
        }
    #endif
#endif

    AUD_Configure_Event(DeviceId);

    if (ErrCode != ST_NO_ERROR)
    {
        return ErrCode;
    }

    /* Return no errors */
    /* ================ */
    return (ST_NO_ERROR);
} /* AUD_Init */

/*------------------------------------------------------------------------------
 * Function    : AUD_Setup
 * Description : Setup audio device
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Setup(AUD_DeviceId_t DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    char           dac[20]={0};
    #ifndef TESTAPP_AUD_PCMMIXING
    U32            CutId = ST_GetCutRevision();
	#endif

    #if !defined(TESTAPP_AUDIO_DUAL_DECODE)
        #if  defined(TESTAPP_AUDIO_INTERNAL_DAC)
            strcpy(dac, "INTERNAL DAC");
        #else
            strcpy(dac, "EXTERNAL DAC");
        #endif
    #endif

    /*init Audio*/
    ErrCode |= AUD_Init(DeviceId);
    #ifdef TESTAPP_AUDIO_DUAL_DECODE
        ErrCode |= AUD_Init(AUD_EXT);
    #endif
    #ifndef TESTAPP_AUD_PCMMIXING    /*DriverIndex:8 used for PCM mixing where SPDIF objectset not exist in the chain*/

        if (DeviceId == AUD_INT)
        {
            if(CutId >= 0xC0)
                ErrCode |= AUD_SetDigitalOutput(DeviceId, 2);
            else
                ErrCode |= AUD_SetDigitalOutput(DeviceId, 0);
        }
    #endif
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_Setup(%s) -> FAILED(%d)", dac, ErrCode));
        return ErrCode;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_Setup(%s) -> OK", dac));
    }

    return ErrCode;
} /* AUD_Setup */

/*------------------------------------------------------------------------------
 * Function    : AUD_Start
 * Description : Start audio driver
 * Input       : DeviceId, AUD_StartParams
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Start(AUD_DeviceId_t  DeviceId, STAUD_StreamContent_t StreamContent , U32 freq, STAUD_StreamType_t StreamType)
{
    STAUD_StreamParams_t    AUD_StreamParams;
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;

    AUD_StreamParams.SamplingFrequency = freq;
    AUD_StreamParams.StreamID          = STAUD_IGNORE_ID;
    AUD_StreamParams.StreamType        = StreamType;
    AUD_StreamParams.StreamContent     = StreamContent;
    U32 SamplingFrequency;
    U32            CutId = ST_GetCutRevision();

#ifndef TESTAPP_AUDIO_DUAL_DECODE

    /* HDMI audio is from PCM for cut2.0 because of fdma firmware */
    if(CutId < 0xC0)
    {
        #ifdef TESTAPP_AUDIO_INTERNAL_DAC
            /*Do nothing for cut2.0
            * Not Possible to redirect Internal DAC output to HDMI */
        #else
            ErrCode = STAUD_OPEnableHDMIOutput(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
        #endif
        if (ErrCode == ST_NO_ERROR)
        {
               STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPEnableHDMIOutput to PCMP (%s)  -> OK", AUDIO_Names[0]));
        }
        else
        {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPEnableHDMIOutput on PCMP(%s)  -> FAILED(%d)", AUDIO_Names[0], ErrCode));

        }
    }



    ErrCode = STAUD_DRStart(AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, &AUD_StreamParams);
    STAUD_IPGetSamplingFrequency(AUDIO_Handler[DeviceId], STAUD_OBJECT_INPUT_CD0, &SamplingFrequency );
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STAUD_DRStart(%s, %d, %d, %d, F:%dHz, Id: %02.2x) -> FAILED ",
                        AUDIO_Names[DeviceId],
                        AUDIO_Handler[DeviceId],
                        AUD_StreamParams.StreamContent,
                        AUD_StreamParams.StreamType,
                        SamplingFrequency,
                        AUD_StreamParams.StreamID));
        return ErrCode;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "STAUD_DRStart(%s, %d, %d, %d, F:%dHz, Id: %02.2x) -> OK ",
                      AUDIO_Names[DeviceId],
                      AUDIO_Handler[DeviceId],
                      AUD_StreamParams.StreamContent,
                      AUD_StreamParams.StreamType,
                      SamplingFrequency,
                      AUD_StreamParams.StreamID));
    }

    #ifndef TESTAPP_MEMORY_INJECT
        #ifndef TESTAPP_DISABLE_SYNC
            #ifdef TESTAPP_AUD_PCMMIXING        /*DriverIndex:8 used for PCM mixing where PCM0 player is used*/
                ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
            #elif TESTAPP_AUDIO_INTERNAL_DAC
                ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP1);
            #else
                ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
            #endif

            if (ErrCode == ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_DREnableSynchronization on  PCMP (%s)  -> OK",AUDIO_Names[0]));
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DREnableSynchronization on  PCMP(%s)  -> FAILED(%d)",AUDIO_Names[0], ErrCode));

            }

            #ifndef TESTAPP_AUD_PCMMIXING    /*DriverIndex:8 used for PCM mixing where SPDIF objectset not exist in the chain*/
                ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_SPDIF0);
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_DREnableSynchronization on  SPDIF (%s)  -> OK", AUDIO_Names[0]));
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DREnableSynchronization on SPDIF0(%s)  -> FAILED(%d)", AUDIO_Names[0], ErrCode));
                }
            #endif
        #else /* TESTAPP_DISABLE_SYNC */
            #ifdef TESTAPP_AUD_PCMMIXING        /*DriverIndex:8 used for PCM mixing where PCM0 player is used*/
                ErrCode = STAUD_OPDisableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
            #elif TESTAPP_AUDIO_INTERNAL_DAC
                ErrCode = STAUD_OPDisableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP1);
            #else
                ErrCode = STAUD_OPDisableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
            #endif

            if (ErrCode == ST_NO_ERROR)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPDisableSynchronization on  PCMP (%s)  -> OK",AUDIO_Names[0]));
            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPDisableSynchronization on  PCMP(%s)  -> FAILED(%d)",AUDIO_Names[0], ErrCode));

            }

            #ifndef TESTAPP_AUD_PCMMIXING    /*DriverIndex:8 used for PCM mixing where SPDIF objectset not exist in the chain*/
                ErrCode = STAUD_OPDisableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_SPDIF0);
                if (ErrCode == ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPDisableSynchronization on  SPDIF (%s)  -> OK", AUDIO_Names[0]));
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPDisableSynchronization on SPDIF0(%s)  -> FAILED(%d)", AUDIO_Names[0], ErrCode));
                }
            #endif /* TESTAPP_AUD_PCMMIXING */
        #endif /* TESTAPP_DISABLE_SYNC */
    #endif /* TESTAPP_MEMORY_INJECT */
#else /* TESTAPP_AUDIO_DUAL_DECODE */
    if(DeviceId == AUD_INT)
    {
        if(CutId < 0xC0)
        {
            ErrCode = STAUD_OPEnableHDMIOutput(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
            if (ErrCode == ST_NO_ERROR)
            {
                   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPEnableHDMIOutput to PCMP (%s)  -> OK", AUDIO_Names[0]));
            }
            else
            {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPEnableHDMIOutput on PCMP(%s)  -> FAILED(%d)", AUDIO_Names[0], ErrCode));

            }
        }

        ErrCode = STAUD_DRStart(AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, &AUD_StreamParams);
        STAUD_IPGetSamplingFrequency(AUDIO_Handler[DeviceId], STAUD_OBJECT_INPUT_CD0, &SamplingFrequency);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STAUD_DRStart(%s, %d, %d, %d, F:%dHz, Id: %02.2x) -> FAILED ",
                            AUDIO_Names[DeviceId],
                            AUDIO_Handler[DeviceId],
                            AUD_StreamParams.StreamContent,
                            AUD_StreamParams.StreamType,
                            SamplingFrequency,
                            AUD_StreamParams.StreamID));
            return ErrCode;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                          "STAUD_DRStart(%s, %d, %d, %d, F:%dHz, Id: %02.2x) -> OK ",
                          AUDIO_Names[DeviceId],
                          AUDIO_Handler[DeviceId],
                          AUD_StreamParams.StreamContent,
                          AUD_StreamParams.StreamType,
                          SamplingFrequency,
                          AUD_StreamParams.StreamID));
        }
        ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0);
        if (ErrCode == ST_NO_ERROR)
        {
               STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPEnableSynchronization on PCMP0 (%s)  -> OK", AUDIO_Names[DeviceId]));
        }
        else
        {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPEnableSynchronization on PCMP0 (%s)  -> FAILED(%d)", AUDIO_Names[DeviceId], ErrCode));

        }

        if(CutId >= 0xC0) /* cut3*/
        {
            ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_SPDIF0);
            if (ErrCode == ST_NO_ERROR)
            {
                   STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_DREnableSynchronization on SPDIF0 (%s)  -> OK", AUDIO_Names[0]));
            }
            else
            {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DREnableSynchronization on SPDIF0 (%s)  -> FAILED(%d)", AUDIO_Names[0], ErrCode));

            }
        }
    }
    else
    {

     /*   ErrCode = STAUD_DRSetClockRecoverySource(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP1, CLKRV_Handler[1]);*/
        if (ErrCode == ST_NO_ERROR)
        {
               STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_DRSetClockRecoverySource(AudHdl:%s, ClkHdl:%d)  -> OK", AUDIO_Names[DeviceId], CLKRV_Handler[1]));
        }
        else
        {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DRSetClockRecoverySource(AudHdl:%s, ClkHdl:%d)  -> FAILED(%d)", AUDIO_Names[DeviceId], CLKRV_Handler[1], ErrCode));
        }

        ErrCode = STAUD_DRStart(AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED1, &AUD_StreamParams);
        STAUD_IPGetSamplingFrequency(AUDIO_Handler[DeviceId], STAUD_OBJECT_INPUT_CD1, &SamplingFrequency );
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                            "STAUD_DRStart(%s, %d, %d, %d, F:%dHz, Id: %02.2x) -> FAILED ",
                            AUDIO_Names[DeviceId],
                            AUDIO_Handler[DeviceId],
                            AUD_StreamParams.StreamContent,
                            AUD_StreamParams.StreamType,
                            SamplingFrequency,
                            AUD_StreamParams.StreamID));
            return ErrCode;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                          "STAUD_DRStart(%s, %d, %d, %d, F:%dHz, Id: %02.2x) -> OK ",
                          AUDIO_Names[DeviceId],
                          AUDIO_Handler[DeviceId],
                          AUD_StreamParams.StreamContent,
                          AUD_StreamParams.StreamType,
                          SamplingFrequency,
                          AUD_StreamParams.StreamID));
        }

        ErrCode = STAUD_OPDisableSynchronization(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP1);
        if (ErrCode == ST_NO_ERROR)
        {
               STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPDisableSynchronization(%s)  -> OK", AUDIO_Names[DeviceId]));
        }
        else
        {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPDisableSynchronization(%s)  -> FAILED(%d)", AUDIO_Names[DeviceId], ErrCode));

        }
    }
#endif /* TESTAPP_AUDIO_DUAL_DECODE */

    return(ErrCode);
} /* AUD_Start */

/*------------------------------------------------------------------------------
 * Function    : AUD_Stop
 * Description : Stop audio driver
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Stop(AUD_DeviceId_t DeviceId)
{
    ST_ErrorCode_t      ErrCode = ST_NO_ERROR;
    STAUD_Fade_t        AUD_Fading;
    STAUD_Stop_t        StopMode;

    AUD_Fading.FadeType = STAUD_FADE_SOFT_MUTE;
    StopMode = STAUD_STOP_WHEN_END_OF_DATA;

    ErrCode = STAUD_DRStop(AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, StopMode, &AUD_Fading);
    ErrCode |= STAUD_DisableSynchronisation(AUDIO_Handler[DeviceId]);
    ErrCode |= STAUD_DRSetSyncOffset(AUDIO_Handler[DeviceId], STAUD_OBJECT_DECODER_COMPRESSED0, 0);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                  "STAUD_DRStop(%s, %d, %d) -> FAILED",
                  AUDIO_Names[DeviceId],
                  AUDIO_Handler[DeviceId],
                  StopMode));
        return ErrCode;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "STAUD_DRStop(%s, %d, %d) -> OK",
                      AUDIO_Names[DeviceId],
                      AUDIO_Handler[DeviceId],
                      StopMode));
    }
    return ErrCode;
} /* AUD_Stop */

/*------------------------------------------------------------------------------
 * Function    : AUD_Close
 * Description : Close audio
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Close(AUD_DeviceId_t DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    ErrCode = STAUD_Close(AUDIO_Handler[DeviceId]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STAUD_Close(%d)=%d", DeviceId, ErrCode));
        return ErrCode;
    }
    else
    {
        /* ErrCode == ST_NO_ERROR */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "STAUD_Close(%d)=%d", DeviceId, ErrCode));
    }
    return ErrCode;
}/* AUD_Close */

/*------------------------------------------------------------------------------
 * Function    : AUD_Term
 * Description : Term audio
 * Input       : DeviceId, ForceTerminate
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Term(AUD_DeviceId_t DeviceId, BOOL ForceTerminate)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STAUD_TermParams_t TermParams;

    memset((void*)&TermParams, 0, sizeof(STAUD_TermParams_t));
    TermParams.ForceTerminate = ForceTerminate;
    ErrCode = STAUD_Term(AUDIO_Names[DeviceId], &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "STAUD_Term(%d)=%d", DeviceId, ErrCode));
        return ErrCode;
    }
    else
    {
        /* ErrCode == ST_NO_ERROR                                         */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "STAUD_Term(%d)=%d", DeviceId, ErrCode));
    }
    return ErrCode;

} /* AUD_Term() */

/*-------------------------------------------------------------------------
 * Function    : AUD_CallbackProc
 * Description : Callback proc.
 * Input       : STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData
 * Output      :
 * Return      : ErrCode
 * ----------------------------------------------------------------------*/
static void AUD_CallbackProc(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData )
{
    AUD_EvtHolder_t         *AUD_EvtHolder_p;
    osclock_t               time;

    time = time_plus(time_now(), time_ticks_per_sec()/10);
    AUD_EvtHolder_p = message_claim_timeout (AUD_EvtMsg_p, &time);
    if (AUD_EvtHolder_p == NULL)
    {
        return;
    }

    AUD_EvtHolder_p->Event     = Event;
    AUD_EvtHolder_p->Reason    = Reason;
    memcpy(&(AUD_EvtHolder_p->EventData), (U32 *)EventData, sizeof(U32));

    message_send (AUD_EvtMsg_p, AUD_EvtHolder_p);


} /* AUD_CallbackProc() */

/*------------------------------------------------------------------------------
 * Function    : AUD_Configure_Event
 * Description : Stop audio driver
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_Configure_Event(AUD_DeviceId_t DeviceId)
{
    ST_ErrorCode_t             ErrCode = ST_NO_ERROR;
    STEVT_SubscribeParams_t    STEVT_SubscribeParams;
    U32                        event;

    AUD_EvtMsg_p = message_create_queue_timeout(sizeof(AUD_EvtHolder_t), 32);
    if(AUD_EvtMsg_p == NULL)
    {
        DEBUG_PRINT(("message_create(AUD_EvtMsg_p) -> FAILED\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    AUD_EvtTaskRemoved = semaphore_create_fifo(1);
    if(AUD_EvtTaskRemoved == NULL)
    {
        DEBUG_PRINT(("semaphore_create(AUD_EvtTaskRemoved) -> FAILED\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    AUD_EvtTaskDelete = FALSE;
    AUD_EvtTaskHandle = Task_Create(AUD_EvtTask, NULL, 8192, MIN_USER_PRIORITY+3, "AUD_EvtTask", 0);
    if(AUD_EvtTaskHandle == NULL)
    {
        DEBUG_PRINT(("task_create(AUD_EvtTask) -> FAILED\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    event = STAUD_NEW_FREQUENCY_EVT;
    STEVT_SubscribeParams.NotifyCallback = AUD_CallbackProc;
    ErrCode=STEVT_Subscribe(EVTHandle, event, &STEVT_SubscribeParams);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != STEVT_ERROR_ALREADY_SUBSCRIBED))
    {
        STTBX_Print(("EVT_Subscribe(%d, VIDEO)=%s\n", event, GetErrorText(ErrCode)));
        return ErrCode;
    }

    if(ErrCode == STEVT_ERROR_ALREADY_SUBSCRIBED)
    ErrCode = ST_NO_ERROR;

    return ErrCode;
} /*AUD_Configure_Event() */

/*------------------------------------------------------------------------------
 * Function    : AUD_EvtTask
 * Description : Handle Audio Events
 * Input       : Void *v
 * Output      : None
 * Return      : None
 * --------------------------------------------------------------------------- */
static void AUD_EvtTask(void *v)
{
    ST_ErrorCode_t              ErrCode = ST_ERROR_BAD_PARAMETER;
    STVOUT_OutputParams_t       OutParam;
    U32                         NewAudioFrequency;
    AUD_EvtHolder_t            *AUD_EvtHolder_p;
    osclock_t                   time;
    semaphore_t                *ProtectAudioEvtHandling;

    ProtectAudioEvtHandling = semaphore_create_fifo(1);
    if(ProtectAudioEvtHandling == NULL)
    {
        DEBUG_PRINT(("semaphore_create(ProtectAudioEvtHandling) -> FAILED\n"));
        return ;
    }

    semaphore_wait(AUD_EvtTaskRemoved);

    while(AUD_EvtTaskDelete == FALSE)
    {
        time = time_plus(time_now(), time_ticks_per_sec()/10);
        AUD_EvtHolder_p = message_receive_timeout(AUD_EvtMsg_p, &time);
        if(AUD_EvtHolder_p == NULL)
        {
            continue;
        }

        if (AUD_EvtHolder_p->Reason == CALL_REASON_NOTIFY_CALL)
        {
           switch(AUD_EvtHolder_p->Event)
           {
                case STAUD_NEW_FREQUENCY_EVT:
#if 0
                    /* Get new frequency */
                    NewAudioFrequency = AUD_EvtHolder_p->EventData;

                    /* If there is a real frequency change */
                    /* =================================== */
                    if (HDMIHandle.HDMI_AudioFrequency != NewAudioFrequency)
                    {
                         HDMIHandle.HDMI_AudioFrequency = NewAudioFrequency;
                         /* If HDMI is on, affect HDMI output */
                         /* ------------------------------------- */
                         if (HDMIHandle.HDMI_IsEnabled == TRUE)
                         {
                             ErrCode = STVOUT_GetOutputParams(VOUTHandle[2],&OutParam);
                             if (ErrCode != ST_NO_ERROR)
                             {
                                 STTBX_Print(("\nAUD_CallbackProc():**ERROR** !!! Unable to get output params !!!\n"));
                                 return;
                             }
                             OutParam.HDMI.AudioFrequency = HDMIHandle.HDMI_AudioFrequency;
                             ErrCode = STVOUT_SetOutputParams(VOUTHandle[2],&OutParam);
                             if (ErrCode != ST_NO_ERROR)
                             {
                                 STTBX_Print(("\nAUD_CallbackProc():**ERROR** !!! Unable to set output params !!!\n"));
                                 return;
                             }
                         }
                     }
#endif
                     break;

                default:
                     break;
            }
        }
        message_release(AUD_EvtMsg_p, AUD_EvtHolder_p);
    }

    semaphore_delete(ProtectAudioEvtHandling);

    semaphore_signal(AUD_EvtTaskRemoved);

} /* AUD_EvtTask */

/*-------------------------------------------------------------------------
 * Function    : AUD_Unsubscribe_Events
 * Description : Unsubscribe configured events, currently supported  events:
 *                  STAUD_NEW_FREQUENCY_EVT,
 * Input       : DECODER_t decoder_t :
 * Output      : None
 * Return      : ST_ErrorCode_t
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t AUD_Unsubscribe_Events(PATH_t path, DECODER_t decoder_t)
{
    U32             event;
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    AUD_EvtTaskDelete = TRUE;

    /* Wait task to be ended */
    semaphore_wait(AUD_EvtTaskRemoved);

    message_delete_queue(AUD_EvtMsg_p);
    semaphore_delete(AUD_EvtTaskRemoved);
    task_delete(AUD_EvtTaskHandle);

    /*Unsubscribe  STAUD_NEW_FREQUENCY_EVT  event*/
    event = STAUD_NEW_FREQUENCY_EVT;
    ErrCode = STEVT_Unsubscribe(EVTHandle, event);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != STEVT_ERROR_NOT_SUBSCRIBED))
    {
        DEBUG_PRINT(("STEVT_UnsubscribeDeviceEvent(%d, VIDEO)=%s\n", event, GetErrorText(ErrCode) ));
        return ErrCode;
    }

    return ErrCode;
} /* AUD_Unsubscribe_Events() */

/*------------------------------------------------------------------------------
 * Function    : AUD_DRSetSyncOffset
 * Description : Stop audio driver
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_DRSetSyncOffset(AUD_DeviceId_t DeviceId, U32 Offset)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    #ifndef TESTAPP_AUDIO_DUAL_DECODE
        #ifdef TESTAPP_AUD_PCMMIXING        /*DriverIndex:8 used for PCM mixing where PCM0 player is used*/
            ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0, Offset);
        #elif TESTAPP_AUDIO_INTERNAL_DAC
            ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP1, Offset);
        #else
            ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0, Offset);
        #endif
    #else
        ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[DeviceId], STAUD_OBJECT_OUTPUT_PCMP0, Offset);
    #endif
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DRSetSyncOffset on PCMP-> FAILED(%d)", ErrCode));
    }
    ErrCode |= STAUD_DRSetSyncOffset(AUDIO_Handler[DeviceId],STAUD_OBJECT_OUTPUT_SPDIF0, Offset);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DRSetSyncOffset on SPDIF-> FAILED(%d)", ErrCode));
    }
    return ErrCode;
} /* AUD_DRSetSyncOffset() */

/*------------------------------------------------------------------------------
 * Function    : AUD_STSetDigitalOutput
 * Description : set digital output
 * Input       : AUD_DeviceId_t DeviceId,
 *               U8 SpdifMode:
 *               0 -> STAUD_DIGITAL_MODE_OFF
 *               1 -> STAUD_DIGITAL_MODE_COMPRESSED
 *               2 -> STAUD_DIGITAL_MODE_NONCOMPRESSED
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_SetDigitalOutput(AUD_DeviceId_t DeviceId, U8 SpdifMode)
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;
    STAUD_DigitalOutputConfiguration_t spdifOutParams;

    switch(SpdifMode)
    {
        case 0:
            spdifOutParams.DigitalMode = STAUD_DIGITAL_MODE_OFF;
            break;
        case 1:
            spdifOutParams.DigitalMode = STAUD_DIGITAL_MODE_COMPRESSED;
            break;
        case 2:
            spdifOutParams.DigitalMode = STAUD_DIGITAL_MODE_NONCOMPRESSED;
            break;
    }
    ErrCode = STAUD_SetDigitalOutput(AUDIO_Handler[DeviceId], spdifOutParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_SetDigitalOutput(%s, %d) -> FAILED(%d)", AUDIO_Names[DeviceId], SpdifMode, ErrCode));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_SetDigitalOutput(%s, %d) -> OK", AUDIO_Names[DeviceId], SpdifMode));
    }

    return ErrCode;
}

/*------------------------------------------------------------------------------
 * Function    : AUD_PcmMixing
 * Description : PCM Mixing
 * Input       : DeviceId
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_PcmMixing(AUD_DeviceId_t DeviceId)
 {
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    ErrCode = STAUD_MXUpdatePTSStatus(AUDIO_Handler[DeviceId], STAUD_OBJECT_MIXER0, 0, FALSE);
    if(ErrCode != ST_NO_ERROR)
    {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_MXUpdatePTSStatus() -> FAILED(%d)", ErrCode));
           return ErrCode;
    }
    else
    {
           STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_MXUpdatePTSStatus() -> OK"));
    }

    return ErrCode;
}

/*------------------------------------------------------------------------------
 * Function    : AUD_PcmConfig
 * Description : Configuration
 * Input       : DeviceId, PCMInputParams, InputObject
 *
 * Output      :
 * Return      : ErrCode
 * --------------------------------------------------------------------------- */
ST_ErrorCode_t AUD_PcmConfig(AUD_DeviceId_t DeviceId, STAUD_PCMInputParams_t PCMInputParams, STAUD_Object_t InputObject)
{
    ST_ErrorCode_t  ErrCode = ST_NO_ERROR;

    ErrCode = STAUD_IPSetPCMParams (AUDIO_Handler[DeviceId], InputObject, &PCMInputParams);
    if(ErrCode != ST_NO_ERROR)
    {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_IPSetPCMParams() -> FAILED(%d)", ErrCode));
           return ErrCode;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_IPSetPCMParams() -> OK"));
    }

    return ErrCode;
}
/*------------------------------------------------------------------------------
 * Function    : ToggleAudioMuteState
 * Description : Toggle audio mute state
 * Input       : None
 * Output      : None
 * Return      : None
 * ---------------------------------------------------------------------------*/
void ToggleAudioMuteState(void)
{
    ST_ErrorCode_t ErrCode;

    if (AudioMuteState == UNMUTED)
    {
        ErrCode = STAUD_Mute(AUDIO_Handler[0], TRUE, TRUE); /*mute analog&digital outputs*/
        if (ErrCode == ST_NO_ERROR)
        {
            AudioMuteState = MUTED;
            STTBX_Print(("Audio muted\n"));
        }
    }
    else
    {
        ErrCode = STAUD_Mute(AUDIO_Handler[0], FALSE, FALSE); /*unmute analog&digital outputs*/
        if (ErrCode == ST_NO_ERROR)
        {
            AudioMuteState = UNMUTED;
            STTBX_Print(("Audio unmuted\n"));
        }
    }
} /* ToggleAudioMuteState */

#ifdef TESTTOOL_SUPPORT
/*------------------------------------------------------------------------------
 * Function    : TT_AUD_Mute
 * Description : Mute audio
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * ---------------------------------------------------------------------------*/
BOOL TT_AUD_Mute(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    ErrCode = STAUD_Mute(AUDIO_Handler[0], TRUE, TRUE);
    if (ErrCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine(pars_p, "STAUD_DRMute  FAILED");
        return TRUE;
    }
    return FALSE;
} /* TT_AUD_Mute */

/*------------------------------------------------------------------------------
 * Function    : TT_AUD_Mute
 * Description : Unmute audio
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * ---------------------------------------------------------------------------*/
BOOL TT_AUD_UnMute(parse_t *pars_p, char *result_sym_p)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    ErrCode = STAUD_Mute(AUDIO_Handler[0], FALSE, FALSE);
    if (ErrCode != ST_NO_ERROR)
    {
        STTST_TagCurrentLine(pars_p, "STAUD_DRMute  FAILED");
        return TRUE;
    }
    return FALSE;
} /* TT_AUD_UnMute */

/*-------------------------------------------------------------------------
 * Function    : TT_AUD_OPEnableSynchronization
 * Description : Enable Synchronization
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TT_AUD_OPEnableSynchronization (parse_t *pars_p, char *result_sym_p)
{
  S32                   LVar;
  BOOL                  RetErr;
  STAUD_Object_t        eDecoderObject;
  AUD_DeviceId_t        eDeviceId;
  ST_ErrorCode_t        ErrCode = ST_NO_ERROR;


    /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
    STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
    return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;


    /* Decoder Object */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
    STTST_TagCurrentLine(pars_p, "Expected: Decoder Object");
    return RetErr;
    }
    eDecoderObject=( STAUD_Object_t)LVar;


    ErrCode = STAUD_OPEnableSynchronization(AUDIO_Handler[eDeviceId], eDecoderObject);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_OPEnableSynchronization-> FAILED(%d)",  ErrCode));
        return TRUE;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "STAUD_OPEnableSynchronization -> OK"));
    }


return FALSE;
}/*TT_AUD_OPEnableSynchronization*/


/*-------------------------------------------------------------------------
 * Function    : TT_AUD_Stop
 * Description : Stop
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TT_AUD_Stop (parse_t *pars_p, char *result_sym_p)
{
    S32                   LVar;
    BOOL                  RetErr;
    AUD_DeviceId_t        eDeviceId;
    STAUD_Object_t        eObject;

    STAUD_Fade_t          stAUDFading;
    STAUD_Stop_t          eStopMode;
    U32                   uOffset=0;
    ST_ErrorCode_t        ErrCode = ST_NO_ERROR;


    /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;


    /* Object */
    RetErr = STTST_GetInteger( pars_p, STAUD_OBJECT_DECODER_COMPRESSED0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Object");
        return RetErr;
    }
    eObject=(STAUD_Object_t)LVar;


    /* Stop mode */
    RetErr = STTST_GetInteger( pars_p, STAUD_STOP_WHEN_END_OF_DATA, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Stop mode");
        return RetErr;
    }
    eStopMode=(STAUD_Stop_t)LVar;


    /* FadeType */
    RetErr = STTST_GetInteger( pars_p, STAUD_FADE_SOFT_MUTE, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: FadeType");
        return RetErr;
    }
    stAUDFading.FadeType=(STAUD_Fade_ID_t)LVar;

    /* Offset */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Offset");
        return RetErr;
    }
    uOffset=(U32)LVar;


    ErrCode = STAUD_DRStop(AUDIO_Handler[eDeviceId], eObject, eStopMode, &stAUDFading);
    ErrCode |= STAUD_DisableSynchronisation(AUDIO_Handler[eDeviceId]);
    ErrCode |= STAUD_DRSetSyncOffset(AUDIO_Handler[eDeviceId], eObject, uOffset);

    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_Stop(Device:%d) -> FAILED(%d)",eDeviceId,  ErrCode));
        return TRUE;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_Stop(Device:%d) -> OK",eDeviceId));
    }

return FALSE;
}/* TT_AUD_Stop */

/*-------------------------------------------------------------------------
 * Function    : TT_AUD_Start
 * Description : Start
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TT_AUD_Start (parse_t *pars_p, char *result_sym_p)
{
   S32                      LVar;
   BOOL                     RetErr;
   STAUD_StreamContent_t    eStreamContent;
   STAUD_StreamType_t       eStreamType;
   AUD_DeviceId_t           eDeviceId;
   ST_ErrorCode_t           ErrCode = ST_NO_ERROR;
   U32                      uFrequency;


    /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;


    /* Stream content */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Stream Content");
        return RetErr;
    }
    eStreamContent=(STAUD_StreamContent_t)LVar;


    /* Frequency */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Frequency");
        return RetErr;
    }
    uFrequency=(U32)LVar;


    /* Stream Type */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Stream Type");
        return RetErr;
    }
    eStreamType=(STAUD_StreamType_t)LVar;


    ErrCode=AUD_Start(eDeviceId,eStreamContent,uFrequency,eStreamType);

    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_Start -> FAILED(%d)",  ErrCode));
        return TRUE;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_Start -> OK"));
    }
    return FALSE;

}/* TT_AUD_Start*/




/*-------------------------------------------------------------------------
 * Function    : TT_AUD_SetDigitalOutput
 * Description : Set Digital Output
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL TT_AUD_SetDigitalOutput (parse_t *pars_p, char *result_sym_p)
{
    S32                   LVar;
    BOOL                  RetErr;
    AUD_DeviceId_t        eDeviceId;
    U8                    ucOutSpdifMode;
    ST_ErrorCode_t        ErrCode = ST_NO_ERROR;


    /*DeviceId*/
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;


    /* Output SPDIF mode */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: OutSpdifMode");
        return RetErr;
    }
    ucOutSpdifMode=( U8)LVar;



  /* Digital output */
    ErrCode = AUD_SetDigitalOutput(eDeviceId, ucOutSpdifMode);

    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_SetDigitalOutput-> FAILED(%d)",  ErrCode));
        return TRUE;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_SetDigitalOutput -> OK"));
    }
return FALSE;

}/* TT_AUD_SetDigitalOutput */


/*------------------------------------------------------------------------------
 * Function    : TT_AUD_Close
 * Description : Close
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------- */
BOOL TT_AUD_Close(parse_t *pars_p, char *result_sym_p)
{
    S32                   LVar;
    BOOL                  RetErr;
    AUD_DeviceId_t        eDeviceId;
    ST_ErrorCode_t        ErrCode = ST_NO_ERROR;


    /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;


    ErrCode = STAUD_Close(AUDIO_Handler[eDeviceId]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                        "STAUD_Close(%d)=%d", eDeviceId, ErrCode));
        return TRUE;
    }
    else
    {
        /* ErrCode == ST_NO_ERROR */
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                        "STAUD_Close(%d)=%d", eDeviceId, ErrCode));
    }
    return FALSE;

}/* TT_AUD_Close */

/*------------------------------------------------------------------------------
 * Function    : TT_AUD_DRSetSyncOffset
 * Description : Set Sync Offset
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------- */
BOOL TT_AUD_DRSetSyncOffset(parse_t *pars_p, char *result_sym_p)
{
    S32                   LVar;
    BOOL                  RetErr;
    AUD_DeviceId_t        eDeviceId;
    ST_ErrorCode_t        ErrCode = ST_NO_ERROR;
    U32                   uOffset=0;

    /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;


    /* Offset */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Offset");
        return RetErr;
    }
    uOffset=(U32)LVar;



   #ifdef TESTAPP_AUDIO_INTERNAL_DAC
        ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[eDeviceId],STAUD_OBJECT_OUTPUT_PCMP1,uOffset);
   #else
        ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[eDeviceId],STAUD_OBJECT_OUTPUT_PCMP0,uOffset);
   #endif
   if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DRSetSyncOffset on PCMP-> FAILED(%d)", ErrCode));
         }

   ErrCode|=STAUD_DRSetSyncOffset(AUDIO_Handler[eDeviceId],STAUD_OBJECT_OUTPUT_SPDIF0,uOffset);
   if(ErrCode != ST_NO_ERROR)
   {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STAUD_DRSetSyncOffset on SPDIF-> FAILED(%d)", ErrCode));
        return TRUE;
   }


   return FALSE;

}/* TT_AUD_DRSetSyncOffset */


/*------------------------------------------------------------------------------
 * Function    : TT_AUD_PcmConfig
 * Description : PCM Configuration
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------- */
BOOL TT_AUD_PcmConfig(parse_t *pars_p, char *result_sym_p)
{
    S32                       LVar;
    BOOL                      RetErr;
    AUD_DeviceId_t            eDeviceId;
    STAUD_PCMInputParams_t    stPCMInputParams;
    STAUD_Object_t            eInputObject;
    ST_ErrorCode_t            ErrCode = ST_NO_ERROR;


   /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;



   /* PCM Input params */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    stPCMInputParams.DataPrecision=(U8)LVar;


    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Frequency");
        return RetErr;
    }
    stPCMInputParams.Frequency=(U32)LVar;



    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: NumChannels");
        return RetErr;
    }
    stPCMInputParams.NumChannels=(U8)LVar;


    /* Input Object */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: Object");
        return RetErr;
    }
    eInputObject=(STAUD_Object_t)LVar;



    ErrCode = AUD_PcmConfig(eDeviceId, stPCMInputParams, eInputObject);

    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_PcmConfig -> FAILED(%d)", ErrCode));
        return TRUE;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_PcmConfig -> OK"));
    }

    return FALSE;

} /* TT_AUD_PcmConfig */


/*------------------------------------------------------------------------------
 * Function    : TT_AUD_PcmMixing
 * Description : PCM Mixing
 * Input       : *pars_p,*result_sym_p
 * Output      :
 * Return      : TRUE if error, FALSE if success
 * --------------------------------------------------------------------------- */
BOOL TT_AUD_PcmMixing(parse_t *pars_p, char *result_sym_p)
{
    S32                       LVar;
    BOOL                      RetErr;
    AUD_DeviceId_t            eDeviceId;
    ST_ErrorCode_t            ErrCode = ST_NO_ERROR;

    /* DeviceId */
    RetErr = STTST_GetInteger( pars_p, 0, (S32*)&LVar );
    if (RetErr)
    {
        STTST_TagCurrentLine(pars_p, "Expected: DeviceId");
        return RetErr;
    }
    eDeviceId=(AUD_DeviceId_t)LVar;



    ErrCode=AUD_PcmMixing(eDeviceId);

    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "AUD_PcmMixing -> FAILED(%d)", ErrCode));
        return TRUE;
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "AUD_PcmMixing-> OK"));
    }

    return FALSE;

}/* TT_AUD_PcmMixing */


/*------------------------------------------------------------------------------
 * Function    : AUDIO_RegisterCommands
 * Description : Register audio testtool commands
 * Input       : None
 * Output      : None
 * Return      : None
 * ---------------------------------------------------------------------------*/
void AUDIO_RegisterCommands(void)
{
    ST_ErrorCode_t        ErrCode = ST_NO_ERROR;

    ErrCode = STTST_RegisterCommand( "aud_mute", TT_AUD_Mute, "Mute Audio");
    ErrCode |= STTST_RegisterCommand( "aud_unmute", TT_AUD_UnMute, "Unmute Audio");
    ErrCode |= STTST_RegisterCommand( "aud_start", TT_AUD_Start, "");
    ErrCode |= STTST_RegisterCommand( "aud_Stop", TT_AUD_Stop, "");
    ErrCode |= STTST_RegisterCommand( "aud_Close", TT_AUD_Close, "");
    ErrCode |= STTST_RegisterCommand( "aud_OPEnableSynchronization", TT_AUD_OPEnableSynchronization, "");
    ErrCode |= STTST_RegisterCommand( "aud_SetDigitalOutput", TT_AUD_SetDigitalOutput, "");
    ErrCode |= STTST_RegisterCommand( "aud_DRSetSyncOffset", TT_AUD_DRSetSyncOffset, "");
    ErrCode |= STTST_RegisterCommand( "aud_PcmConfig", TT_AUD_PcmConfig, "");
    ErrCode |= STTST_RegisterCommand( "aud_PcmMixing", TT_AUD_PcmMixing, "");


    ErrCode |= STTST_RegisterCommand( "aud_InjectStart", TT_AUD_InjectStart, "");
    ErrCode |= STTST_RegisterCommand( "aud_InjectStop", TT_AUD_InjectStop, "");


    ErrCode |= STTST_AssignInteger ("A_AC3"   , STAUD_STREAM_CONTENT_AC3, TRUE);
    ErrCode |= STTST_AssignInteger ("A_MPEG1" , STAUD_STREAM_CONTENT_MPEG1, TRUE);
    ErrCode |= STTST_AssignInteger ("A_MPEG2" , STAUD_STREAM_CONTENT_MPEG2, TRUE);
    ErrCode |= STTST_AssignInteger ("A_MP3"   , STAUD_STREAM_CONTENT_MP3, TRUE);
    ErrCode |= STTST_AssignInteger ("A_AAC"   , STAUD_STREAM_CONTENT_MPEG_AAC, TRUE);

    /* AUD_DeviceId_t */
    ErrCode |= STTST_AssignInteger ("AUD_INT"    , AUD_INT, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_EXT"    , AUD_EXT, TRUE);

    /* Decoder Object */
    ErrCode |= STTST_AssignInteger ("AUD_IN_CD0"   , STAUD_OBJECT_INPUT_CD0, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_IN_CD1"   , STAUD_OBJECT_INPUT_CD1, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_DEC_COMP0"   , STAUD_OBJECT_DECODER_COMPRESSED0, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_DEC_COMP1" , STAUD_OBJECT_DECODER_COMPRESSED1, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_POST_PROC0" , STAUD_OBJECT_POST_PROCESSOR0, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_POST_PROC1"   , STAUD_OBJECT_POST_PROCESSOR1, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_OUT_PCMP0"   , STAUD_OBJECT_OUTPUT_PCMP0, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_OUT_PCMP1"   , STAUD_OBJECT_OUTPUT_PCMP1, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_OUT_SPDIF0"   , STAUD_OBJECT_OUTPUT_SPDIF0, TRUE);

    /* Digital Modes */
    ErrCode |= STTST_AssignInteger ("AUD_MODE_COMP"        , STAUD_DIGITAL_MODE_COMPRESSED, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_MODE_UNCOMP"    , STAUD_DIGITAL_MODE_NONCOMPRESSED, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_MODE_OFF"        , STAUD_DIGITAL_MODE_OFF, TRUE);

    /* Brdcast profile */
    ErrCode |= STTST_AssignInteger ("AUD_DVB"   , STAUD_BROADCAST_DVB, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_DIRECTV" , STAUD_BROADCAST_DIRECTV, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_ATSC"     , STAUD_BROADCAST_ATSC, TRUE);
    ErrCode |= STTST_AssignInteger ("AUD_DVD"     , STAUD_BROADCAST_DVD, TRUE);


    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("Audio commands registration error\n"));
    }
    else
    {
        STTBX_Print(("Audio commands registered-> OK\n"));
    }

}
#endif

/* aud_cmd.c */
