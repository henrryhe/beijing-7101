/*****************************************************************************

File Name   : hdmi.c
Description : General hdmi functions

COPYRIGHT (C) STMicroelectronics 2006.

*****************************************************************************/

/* Includes ----------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stddefs.h"
#include "stdenc.h"
#include "stdevice.h"
#include "sthdmi.h"
#include "stsys.h"
#include "sttbx.h"
#include "stvout.h"
#include "..\main\initterm.h"
#include "ch_hdmi.h"
#define DEBUG_PRINT STTBX_Print
/* Private Types -------------------------------------------------------------------- */
#ifndef DU32
typedef volatile unsigned int DU32;
#endif

typedef struct HDMI_EvtHolder_s
{
    STEVT_CallReason_t      Reason;
    STEVT_EventConstant_t   Event;
    STVOUT_State_t          EventData;
} HDMI_EvtHolder_t;

/* Exported Variables ------------------------------------------------------------ */
extern ST_Partition_t              *SystemPartition;
extern ST_DeviceName_t             VOUTDeviceName[3];
extern STVOUT_Handle_t             VOUTHandle[3];
/* Private Variables --------------------------------------------------------------- */

STHDMI_Handle_t HDMI_Handler[HDMI_DEVICE_NUMBER];
ST_DeviceName_t HDMI_Names[HDMI_DEVICE_NUMBER] = {"HDMI"};

HDMI_DeviceHandle_t HdmiDeviceHandle[HDMI_MAXDEVICE] =
{
    {"HDMI",
     &HDMI_Handler[0],
     STHDMI_DEVICE_TYPE_7100,
     0,                      /* Event DeviceName */
     VOUT_HDMI,
     VTG_MAIN,
     "AUD_INT",              /* AUD_DeviceName */
     STVOUT_NO_RECEIVER,     /* VOUT_State */
     FALSE,                  /* HDMI_IsEnabled */
     48000                   /* HDMI_AudioFrequency */
    }
};


static message_queue_t      *HDMI_EvtMsg_p  = NULL;
static task_t               *HDMI_EvtTaskHandle = NULL;
static BOOL                  HDMI_EvtTaskDelete = FALSE;
semaphore_t                 *HDMI_EvtTaskRemoved;


#if defined (TESTAPP_HDCP_ON)

/* Private Constants -------------------------------------------------- */
/* keys must be loaded below, defaults are null since secret */
#ifndef HDMI_IV_0_KEY_VALUE
#define HDMI_IV_0_KEY_VALUE  0x00000000 /* Initialization value Key LSB */
#endif

#ifndef HDMI_IV_1_KEY_VALUE
#define HDMI_IV_1_KEY_VALUE  0x00000000 /* Initialization value Key MSB */
#endif

#ifndef HDMI_KSV_0_KEY_VALUE
#define HDMI_KSV_0_KEY_VALUE 0x00000000 /* Key selection vector */
#endif

#ifndef HDMI_KSV_1_KEY_VALUE
#define HDMI_KSV_1_KEY_VALUE 0x00000000 /* Key selection vector */
#endif

/* Private Variable --------------------------------------------------------------- */
/* Secret device keys of the HDCP transmitter from the digital content protection LLC */
static const U32 HDMI_DeviceKeys[80] =
{ 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000 };

#endif /* #if defined (TESTAPP_HDCP_ON) */


/* Functions ------------------------------------------------------- */
static void HDMI_EvtTask(void *v);

/*-------------------------------------------------------------------------
 * Function     : HDMI_HotplugCallback
 * Description : When vout state changed, this callback is called
 * Input          : Event Data
 * Output        : None
 * Return        : None
 * ----------------------------------------------------------------------*/
void HDMI_HotplugCallback(STEVT_CallReason_t Reason, STEVT_EventConstant_t Event, const void *EventData)
{
    HDMI_EvtHolder_t        *HDMI_EvtHolder_p;
    osclock_t                time;

    if (EventData == NULL)
    {
        STTBX_Print(("\nHDMI_HotplugCallback():**ERROR** !!! EventData pointer is NULL !!!\n"));
        return;
    }

    time = time_plus(time_now(), time_ticks_per_sec()/10);
    HDMI_EvtHolder_p = message_claim_timeout (HDMI_EvtMsg_p, &time);
    if (HDMI_EvtHolder_p == NULL)
    {
        return;
    }

    HDMI_EvtHolder_p->Event     = Event;
    HDMI_EvtHolder_p->Reason    = Reason;
    memcpy(&(HDMI_EvtHolder_p->EventData), (STVOUT_State_t *)EventData, sizeof(STVOUT_State_t));

    message_send (HDMI_EvtMsg_p, HDMI_EvtHolder_p);

}/* HDMI_HotplugCallback() */

/*------------------------------------------------------------------------------
 * Function    : HDMI_EvtTask
 * Description : Handle Audio Events
 * Input       : Void *v
 * Output      : None
 * Return      : None
 * --------------------------------------------------------------------------- */
static void HDMI_EvtTask(void *v)
{
    ST_ErrorCode_t              ErrCode = ST_ERROR_BAD_PARAMETER;
    HDMI_EvtHolder_t           *HDMI_EvtHolder_p;
    osclock_t                   time;
    semaphore_t                *ProtectHDMIEvtHandling;

    ProtectHDMIEvtHandling = semaphore_create_fifo(1);
    if(ProtectHDMIEvtHandling == NULL)
    {
        DEBUG_PRINT(("semaphore_create(ProtectHDMIEvtHandling) -> FAILED\n"));
        return ;
    }

    semaphore_wait(HDMI_EvtTaskRemoved);

    while(HDMI_EvtTaskDelete == FALSE)
    {
        time = time_plus(time_now(), time_ticks_per_sec()/10);
        HDMI_EvtHolder_p = message_receive_timeout(HDMI_EvtMsg_p, &time);
        if(HDMI_EvtHolder_p == NULL)
        {
            continue;
        }

        /* Look for state change */
        /* ===================== */
        switch (HDMI_EvtHolder_p->EventData)
        {
            case STVOUT_DISABLED:
            case STVOUT_ENABLED:
            case STVOUT_NO_ENCRYPTION:
            case STVOUT_NO_HDCP_RECEIVER:
            case STVOUT_AUTHENTICATION_IN_PROGRESS:
                break;

            case STVOUT_AUTHENTICATION_SUCCEEDED:
                break;

            case STVOUT_AUTHENTICATION_FAILED:
                #if defined (TESTAPP_HDCP_ON)
                    EnableDefaultOutput(&VOUTHandle[VOUT_HDMI]);
                #endif
                break;

            case STVOUT_AUTHENTICATION_PART3_SUCCEEDED:
                break;

            case STVOUT_NO_RECEIVER:
                HdmiDeviceHandle[HDMI].VOUT_State = STVOUT_NO_RECEIVER;
                if (HdmiDeviceHandle[HDMI].HDMI_IsEnabled == TRUE)
                {
                    ErrCode = HDMI_DisableOutput(HDMI);
                }
                break;

            case STVOUT_RECEIVER_CONNECTED:
                HdmiDeviceHandle[HDMI].VOUT_State = STVOUT_RECEIVER_CONNECTED;
                if (HdmiDeviceHandle[HDMI].HDMI_IsEnabled == FALSE)
                {
                    ErrCode = HDMI_EnableOutput(HDMI);
                }
                break;

            default:
                break;
        }
        message_release(HDMI_EvtMsg_p, HDMI_EvtHolder_p);
    }

    semaphore_delete(ProtectHDMIEvtHandling);

    semaphore_signal(HDMI_EvtTaskRemoved);

} /* HDMI_EvtTask */

#if defined(TESTAPP_HDCP_ON)
/*-------------------------------------------------------------------------
 * Function     : HDMI_Setup
 * Description : Setup for sthdmi driver
 * Input          : None
 * Output        : None
 * Return        : ST ErrorCode
  * ----------------------------------------------------------------------*/
ST_ErrorCode_t EnableDefaultOutput(STVOUT_Handle_t *VOUT_Handle)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STVOUT_DefaultOutput_t VOUT_DefaultOutput;

    /* 0,   0,   0  : Black
     * 255, 255, 255: White */
    VOUT_DefaultOutput.DataChannel0=0;
    VOUT_DefaultOutput.DataChannel1=0;
    VOUT_DefaultOutput.DataChannel2=0;

    ErrCode = STVOUT_EnableDefaultOutput(*VOUT_Handle, &VOUT_DefaultOutput);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "EnableDefaultOutput -> FAILED(%d)", ErrCode));
        return (ErrCode);
    }

    return(ErrCode);
} /* EnableDefaultOutput() */
#endif

/*-------------------------------------------------------------------------
 * Function     : HDMI_Setup
 * Description : Setup for sthdmi driver
 * Input          : None
 * Output        : None
 * Return        : ST ErrorCode
  * ----------------------------------------------------------------------*/
ST_ErrorCode_t HDMI_Setup(void)
{
    STHDMI_InitParams_t     HDMI_InitParams;
    STHDMI_OpenParams_t     HDMI_OpenParams;
    STEVT_SubscribeParams_t EVT_SubscribeParams;
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;

    /* Initialize the HDMI interface */
    /* ============================= */
    memset(&HDMI_InitParams, 0, sizeof(STHDMI_InitParams_t));
    memset(&HDMI_OpenParams, 0, sizeof(STHDMI_OpenParams_t));

    HDMI_InitParams.DeviceType     = STHDMI_DEVICE_TYPE_7100;
    HDMI_InitParams.OutputType     = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
    HDMI_InitParams.MaxOpen        = 1;
    HDMI_InitParams.CPUPartition_p = SystemPartition;
    HDMI_InitParams.AVIType        = STHDMI_AVI_INFOFRAME_FORMAT_VER_TWO;
    HDMI_InitParams.SPDType        = STHDMI_SPD_INFOFRAME_FORMAT_VER_ONE;
    HDMI_InitParams.MSType         = STHDMI_MS_INFOFRAME_FORMAT_VER_ONE;
    HDMI_InitParams.AUDIOType      = STHDMI_AUDIO_INFOFRAME_FORMAT_VER_ONE;
    HDMI_InitParams.VSType         = STHDMI_VS_INFOFRAME_FORMAT_VER_ONE;
#if 1
	strcpy(HDMI_InitParams.Target.OnChipHdmiCell.EvtDeviceName, EVTDeviceName);
#if 0 
	strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VOUTDeviceName, VOUTDeviceName[VOUT_MAIN]);
#else
	strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VOUTDeviceName, VOUTDeviceName[VOUT_HDMI]);
#endif
	strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VTGDeviceName, VTGDeviceName[VTG_MAIN]);
#else

    strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VTGDeviceName, VTG_Names[0]);
    strcpy(HDMI_InitParams.Target.OnChipHdmiCell.VOUTDeviceName, VOUT_Names[2]);
    strcpy(HDMI_InitParams.Target.OnChipHdmiCell.EvtDeviceName, EVT_Names[0]);
#endif
    ErrCode =  STHDMI_Init(HDMI_Names[0], &HDMI_InitParams);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HDMI_Init(%s) -> FAILED(%d)",
                        HDMI_Names[0], GetErrorText(ErrCode)));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "HDMI_Init(%s)=%s",
                      HDMI_Names[0], GetErrorText(ErrCode)));
    }

    /*ErrCode |= HDMI_GetCapability(HDMI_Names[0]);
    ErrCode |= HDMI_GetSinkCapability(HDMI_Names[0]);
    ErrCode |= HDMI_GetSourceCapability(HDMI_Names[0]);
    if (ErrCode != ST_NO_ERROR) return (ErrCode);*/

    ErrCode = STHDMI_Open(HDMI_Names[0], &HDMI_OpenParams, &HDMI_Handler[0]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "HDMI_Open(%s) -> FAILED(%d)", HDMI_Names[0], ErrCode));
        return (ErrCode);
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "HDMI_Open(%s) -> OK", HDMI_Names[0]));
    }

    /* Use this Poke to Enable HDMI audio    */
    /* ===================================== */
    *(DU32 *)(AUDIO_IF_BASE_ADDRESS + 0x204) = 0x1;


    /* Subscribe to STVOUT state change event */
    /* ====================================== */
    memset(&EVT_SubscribeParams, 0, sizeof(STEVT_DeviceSubscribeParams_t));
    EVT_SubscribeParams.NotifyCallback = HDMI_HotplugCallback;
    ErrCode = STEVT_Subscribe(/*EVT_Handler[0]*/EVTHandle, STVOUT_CHANGE_STATE_EVT, &EVT_SubscribeParams);
    if (ErrCode != ST_NO_ERROR)
        return (ErrCode);

    /* Start internal Task */
    if(HDMI_EvtTaskHandle == NULL)
    {
        HDMI_EvtMsg_p = message_create_queue_timeout(sizeof(HDMI_EvtHolder_t), 32);
        if(HDMI_EvtMsg_p == NULL)
        {
            DEBUG_PRINT(("message_create(HDMI_EvtMsg_p) -> FAILED\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        HDMI_EvtTaskRemoved = semaphore_create_fifo(1);
        if(HDMI_EvtTaskRemoved == NULL)
        {
            DEBUG_PRINT(("semaphore_create(HDMI_EvtTaskRemoved) -> FAILED\n"));
            return ST_ERROR_BAD_PARAMETER;
        }

        HDMI_EvtTaskDelete = FALSE;
        HDMI_EvtTaskHandle = Task_Create(HDMI_EvtTask, NULL, 1024, 6+3, "HDMI_EvtTask", 0);
        if(HDMI_EvtTaskHandle == NULL)
        {
            DEBUG_PRINT(("task_create(HDMI_EvtTask) -> FAILED\n"));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    /* Start the HDMI/HDCP state machine */
    /* ============================ */
    ErrCode = STVOUT_Start(VOUTHandle[VOUT_HDMI]);
    if (ErrCode != ST_NO_ERROR)
        return (ErrCode);


    return (ST_NO_ERROR);
}/* HDMI_Setup() */

/*-------------------------------------------------------------------------
 * Function     : Enable_HDMI
 * Description : Enable HDMI output
 * Input          : VoutHandler
 * Output        : None
 * Return        : ST ErrorCode
  * ----------------------------------------------------------------------*/
ST_ErrorCode_t Enable_HDMI(STVOUT_Handle_t VoutHandler)
{
    ST_ErrorCode_t        ErrCode = ST_NO_ERROR;
    STVOUT_OutputParams_t OutParam;


    OutParam.HDMI.ForceDVI = FALSE;
    OutParam.HDMI.IsHDCPEnable = FALSE;
    OutParam.HDMI.AudioFrequency = 48000;
    ErrCode |= STVOUT_SetOutputParams(VoutHandler, &OutParam);
    ErrCode |= STVOUT_Enable(VoutHandler);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Enable_HDMI -> FAILED(%d)", ErrCode));
        return (ErrCode);
    }
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Enable_HDMI -> OK"));

    return (ErrCode);

} /* Enable_HDMI() */


/*-------------------------------------------------------------------------
 * Function     : Clean_HDMI
 * Description : Clean for HDMI output
 * Input          : HDMI DeviceId and HandleId
 * Output        : None
 * Return        : ST ErrorCode
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t Clean_HDMI(HDMI_DeviceId_t HDMI_DeviceId, HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    /* Unsubscribe from STVOUT state change event */
    /* ======================================== */
    ErrCode = STEVT_Unsubscribe(/*EVT_Handler[0]*/EVTHandle, STVOUT_CHANGE_STATE_EVT);
    if (ErrCode != ST_NO_ERROR)
        return (ErrCode);

    /* Unsubscribe from STAUD new frequency event */
    /* ======================================== */
    ErrCode = STEVT_Unsubscribe(/*EVT_Handler[0]*/EVTHandle, STAUD_NEW_FREQUENCY_EVT);
    if (ErrCode != ST_NO_ERROR)
        return (ErrCode);

    /*********************** Close & Term HDMI ******************/
    ErrCode = THDMI_Close(HDMI_HandleId);
    if (ErrCode != ST_NO_ERROR)
        return (ErrCode);

    ErrCode = THDMI_Term(HDMI_DeviceId);

    return (ErrCode);
}/* Clean_HDMI() */

/*---------------------------------------------------------------------
 * Function     : HDMI_GetCapability
 * Description : Get HDMI Capabilities.
 * Input          : HDMI Device ID
 * Output        : None
 * Return        : ST ErrorCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_GetCapability(ST_DeviceName_t HDMI_DeviceName)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_Capability_t HDMI_Capability;

    memset((void *)&HDMI_Capability, 0, sizeof(STHDMI_Capability_t));

    ErrCode = STHDMI_GetCapability(HDMI_Names[0], &HDMI_Capability);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "HDMI_GetCapability(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "HDMI_GetCapability(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
        STTBX_Print(("HDMI Capability: IsAudioSupported: %d\n", HDMI_Capability.IsAudioSupported));
        STTBX_Print(("HDMI Capability: IsDataIslandSupported: %d\n", HDMI_Capability.IsDataIslandSupported));
        STTBX_Print(("HDMI Capability: IsEDIDDataSupported: %d\n", HDMI_Capability.IsEDIDDataSupported));
        STTBX_Print(("HDMI Capability: IsVideoSupported: %d\n", HDMI_Capability.IsVideoSupported));
        STTBX_Print(("HDMI Capability: SupportedOutputs: %d\n", HDMI_Capability.SupportedOutputs));
    }

    return (ErrCode);
}/* HDMI_GetCapability() */

/*---------------------------------------------------------------------
 * Function     : HDMI_GetSinkCapability
 * Description : Get HDMI Sink Capabilities.
 * Input          : HDMI Device ID
 * Output        : None
 * Return        : ST ErrorCode
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_GetSinkCapability(ST_DeviceName_t HDMI_DeviceName)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_SinkCapability_t HDMI_SinkCapability;

    memset((void *)&HDMI_SinkCapability, 0, sizeof(STHDMI_SinkCapability_t));

    ErrCode = STHDMI_GetSinkCapability(HDMI_Names[0], &HDMI_SinkCapability);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "HDMI_SinkCapability(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "HDMI_SinkCapability(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
        STTBX_Print(("HDMI Sink Capability: EDIDBasicVersion: %d\n", HDMI_SinkCapability.EDIDBasicVersion));
        STTBX_Print(("HDMI Sink Capability: EDIDBasicRevision: %d\n", HDMI_SinkCapability.EDIDBasicRevision));
        STTBX_Print(("HDMI Sink Capability: EDIDExtRevision: %d\n", HDMI_SinkCapability.EDIDExtRevision));
        STTBX_Print(("HDMI Sink Capability: IsBasicEDIDSupported: %d\n", HDMI_SinkCapability.IsBasicEDIDSupported));
        STTBX_Print(("HDMI Sink Capability: IsAdditionalDataSupported: %d\n", HDMI_SinkCapability.IsAdditionalDataSupported));
        STTBX_Print(("HDMI Sink Capability: IsLCDTimingDataSupported: %d\n", HDMI_SinkCapability.IsLCDTimingDataSupported));
        STTBX_Print(("HDMI Sink Capability: IsColorInfoSupported: %d\n", HDMI_SinkCapability.IsColorInfoSupported));
        STTBX_Print(("HDMI Sink Capability: IsDviDataSupported: %d\n", HDMI_SinkCapability.IsDviDataSupported));
        STTBX_Print(("HDMI Sink Capability: IsTouchScreenDataSupported: %d\n", HDMI_SinkCapability.IsTouchScreenDataSupported));
        STTBX_Print(("HDMI Sink Capability: IsBlockMapSupported: %d\n", HDMI_SinkCapability.IsBlockMapSupported));
    }

    return (ErrCode);
}/* HDMI_GetSinkCapability() */

/*---------------------------------------------------------------------
 * Function     : HDMI_GetSourceCapability
 * Description : Get HDMI Source Capabilities.
 * Input          : HDMI Device ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_GetSourceCapability(ST_DeviceName_t HDMI_DeviceName)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_SourceCapability_t HDMI_SourceCapability;

    memset((void *)&HDMI_SourceCapability, 0, sizeof(STHDMI_SourceCapability_t));

    ErrCode = STHDMI_GetSourceCapability(HDMI_Names[0], &HDMI_SourceCapability);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "HDMI_GetSourceCapability(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "HDMI_GetSourceCapability(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
        STTBX_Print(("HDMI Source Capability: AudioInfoFrameSupported: %d\n", HDMI_SourceCapability.AudioInfoFrameSupported));
        STTBX_Print(("HDMI Source Capability: AVIInfoFrameSupported: %d\n", HDMI_SourceCapability.AVIInfoFrameSupported));
        STTBX_Print(("HDMI Source Capability: MSInfoFrameSupported: %d\n", HDMI_SourceCapability.MSInfoFrameSupported));
        STTBX_Print(("HDMI Source Capability: SPDInfoFrameSupported: %d\n", HDMI_SourceCapability.SPDInfoFrameSupported));
        STTBX_Print(("HDMI Source Capability: VSInfoFrameSupported: %d\n", HDMI_SourceCapability.VSInfoFrameSupported));
    }

    return (ErrCode);
}/* HDMI_GetSourceCapability() */

/*---------------------------------------------------------------------
 * Function     : HDMI_IsScreenDVIOnly
 * Description : This function checks if the screen is DVI only
 * Input          : HDMI Handle ID, DVIOnlyNotHDMI as boolean
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_IsScreenDVIOnly(HDMI_HandleId_t HDMI_HandleId, BOOL *DVIOnlyNotHDMI)
{
    ST_ErrorCode_t    ErrCode = ST_NO_ERROR;
    STVOUT_State_t    VOUT_State;
    STHDMI_EDIDSink_t HDMI_EDIDSink;
    BOOL              SinkIsDVIOnly;

    /* Try to get HDMI state */
    /* ---------------------------- */
    ErrCode = STVOUT_GetState(VOUTHandle[VOUT_HDMI], &VOUT_State);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get state of the HDMI !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }
    if (VOUT_State == STVOUT_NO_RECEIVER)
    {
        STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! There is not screen on HDMI !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Get EDID of the sink */
    /* This call is required for proper operation of audio workaround */
    /* -------------------- */
    memset(&HDMI_EDIDSink, 0, sizeof(STHDMI_EDIDSink_t));
    ErrCode = STHDMI_FillSinkEDID(HDMI_Handler[HDMI_HandleId], &HDMI_EDIDSink);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to get sink informations !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    #if 1
    /* Analyse EDID */
    /* ------------ */
    SinkIsDVIOnly = TRUE;
    if (HDMI_EDIDSink.EDIDExtension_p != NULL)
    {
        if ((HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.Tag == 0x02) &&
                        (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.RevisionNumber == 3))
        {
            if ((HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[0] == 0x03)&&
                (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[1] == 0x0C)&&
                (HDMI_EDIDSink.EDIDExtension_p->TimingData.EDID861B.DataBlock.VendorData.RegistrationId[2] == 0x00))
            {
                SinkIsDVIOnly = FALSE;
            }
        }
    }
    #endif

    /* Free EDID */
    /* --------- */
    if (HDMI_EDIDSink.EDIDExtension_p != NULL)
    {
        ErrCode = STHDMI_FreeEDIDMemory(HDMI_Handler[HDMI_HandleId]);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(("HDMI_IsScreenDVIOnly():**ERROR** !!! Unable to deallocate EDID informations !!!\n"));
            return (ST_ERROR_BAD_PARAMETER);
        }
    }

    /* Return no errors */
    /* ---------------- */
    *DVIOnlyNotHDMI = SinkIsDVIOnly;
    return (ST_NO_ERROR);
}/* HDMI_IsScreenDVIOnly() */

/*---------------------------------------------------------------------
 * Function     : HDMI_EnableOutput
 * Description : Enable the HDMI output
 * Input          : HDMI Device ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_EnableOutput(U32 DeviceId)
{
    #if defined (TESTAPP_HDCP_ON)
        STVOUT_HDCPParams_t     VOUT_HDCPParams;
    #endif
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;
    STVOUT_OutputParams_t   VOUT_OutputParams;
    STVTG_TimingMode_t      VTG_TimingMode;
    STVTG_ModeParams_t      VTG_ModeParams;
    STHDMI_OutputWindows_t  HDMI_OutputWindows;

    #if defined (TESTAPP_HDCP_ON)
       /*if (VOUT_OutputParams.HDMI.IsHDCPEnable == TRUE)*/
      {
           memset(&VOUT_HDCPParams, 0, sizeof(STVOUT_HDCPParams_t));
           VOUT_HDCPParams.IV_0        = HDMI_IV_0_KEY_VALUE;
           VOUT_HDCPParams.IV_1        = HDMI_IV_1_KEY_VALUE;
           VOUT_HDCPParams.KSV_0       = HDMI_KSV_0_KEY_VALUE;
           VOUT_HDCPParams.KSV_1       = HDMI_KSV_1_KEY_VALUE;
           VOUT_HDCPParams.IRate       = 0;
           VOUT_HDCPParams.IsACEnabled = FALSE;
           memcpy(&VOUT_HDCPParams.DeviceKeys, HDMI_DeviceKeys, sizeof(HDMI_DeviceKeys));
           ErrCode = STVOUT_SetHDCPParams(VOUTHandle[VOUT_HDMI], &VOUT_HDCPParams);
           if (ErrCode != ST_NO_ERROR)
           {
               STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to setup HDCP keys !!!\n"));
               return (ST_ERROR_BAD_PARAMETER);
           }



      }
    #endif /* TESTAPP_HDCP_ON */

    /* Enable the output */
    /* ================= */
    ErrCode = STVOUT_Enable(VOUTHandle[VOUT_HDMI]);
    if (ErrCode != ST_NO_ERROR)
    {
       STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to enable the HDMI interface !!!\n"));
       return (ST_ERROR_BAD_PARAMETER);
    }
    HdmiDeviceHandle[DeviceId].HDMI_IsEnabled = TRUE; /* Get EDID of the sink */

    ErrCode = STVTG_GetMode(/*VTG_Handler[0]*/VTGHandle[VTG_MAIN], &VTG_TimingMode, &VTG_ModeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVTG_GetMode()=%s", GetErrorText(ErrCode)));
        return (ST_ERROR_BAD_PARAMETER);
    }

    memset(&HDMI_OutputWindows, 0, sizeof(STHDMI_OutputWindows_t));
    HDMI_OutputWindows.OutputWinX      = 0;
    HDMI_OutputWindows.OutputWinY      = 0;
    HDMI_OutputWindows.OutputWinWidth  = VTG_ModeParams.ActiveAreaWidth;
    HDMI_OutputWindows.OutputWinHeight = VTG_ModeParams.ActiveAreaHeight;
    ErrCode = STHDMI_SetOutputWindows(HDMI_Handler[0], &HDMI_OutputWindows);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STHDMI_SetOutputWindows()=%s", GetErrorText(ErrCode)));
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Get the output params*/
    /* ========================================= */
    ErrCode = STVOUT_GetOutputParams(VOUTHandle[VOUT_HDMI], &VOUT_OutputParams);
    if (ErrCode != ST_NO_ERROR)
    {
       STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to get output parameters !!!\n"));
       return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check if the screen is DVI only or not */
    /* ====================================== */
    #if defined(TESTAPP_HDMI_FORCE_DVI)
        VOUT_OutputParams.HDMI.ForceDVI = TRUE;
    #else
        {
            BOOL IsDVIOnly;
            ErrCode = HDMI_IsScreenDVIOnly(0, &IsDVIOnly);
            if (ErrCode != ST_NO_ERROR)
            {
                STTBX_Print(("HDMI_EnableOutput(%d):**ERROR** !!! Unable to detect if this is a DVI or HDMI screen !!!\n", DeviceId));
                return (ST_ERROR_BAD_PARAMETER);
            }
            VOUT_OutputParams.HDMI.ForceDVI = IsDVIOnly;
        }
    #endif /* TESTAPP_HDMI_FORCE_DVI */

    #if defined(TESTAPP_HDCP_ON)
        VOUT_OutputParams.HDMI.IsHDCPEnable = TRUE;
    #else
        VOUT_OutputParams.HDMI.IsHDCPEnable = FALSE;
    #endif

    VOUT_OutputParams.HDMI.AudioFrequency = HdmiDeviceHandle[0].HDMI_AudioFrequency;
    ErrCode = STVOUT_SetOutputParams(VOUTHandle[VOUT_HDMI], &VOUT_OutputParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to set output parameters !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Enable HDMI Info Frames */
    /* ======================= */
    ErrCode = HDMI_EnableInfoframe(HDMI);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableInfoframe():**ERROR** !!! Unable to enable info frames !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    return (ST_NO_ERROR);
}/* HDMI_EnableOutput() */

/*---------------------------------------------------------------------
 * Function     : HDMI_DisableOutput
 * Description : Disable the HDMI output
 * Input          : HDMI Device ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_DisableOutput(U32 DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    ErrCode = STVOUT_Disable(VOUTHandle[VOUT_HDMI]);
    HdmiDeviceHandle[DeviceId].HDMI_IsEnabled = FALSE;

    return (ErrCode);
}/* HDMI_DisableOutput() */

/*---------------------------------------------------------------------
 * Function     : HDMI_EnableInfoframe
 * Description : Enable HDMI info frame transmission
 * Input          : HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_EnableInfoframe(U32 HandleId)
{
    ST_ErrorCode_t          ErrCode = ST_NO_ERROR;

    /* Send the AVI info frame */
    /* ======================= */
    ErrCode = HDMI_FillAVI(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the AVI Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* This is mandatory for proper audio workaround operation */
    /* Send the audio info frame */
    /* ========================= */
    ErrCode = HDMI_FillAudio(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the AUDIO Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* This is not mondatory for HDMI certfication*/
    /* Send the MPEG Source info Frame*/
    /*================================*/
    ErrCode = HDMI_FillMS(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the MPEG SOURCE Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Send the SPD info Frame*/
    /*========================*/
    ErrCode = HDMI_FillSPD(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the SPD Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Send the SPD info Frame*/
    /*========================*/
    ErrCode = HDMI_FillVS(HandleId);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Print(("\nHDMI_EnableOutput():**ERROR** !!! Unable to send the VS Info frame !!!\n"));
        return (ST_ERROR_BAD_PARAMETER);
    }
    return (ST_NO_ERROR);
}/* HDMI_EnableInfoframe() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillAVI
 * Description : Fill AVI Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_FillAVI(HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_AVIInfoFrame_t HDMI_AVIInfoFrame;
    STVTG_TimingMode_t    VTGTimingMode;
    STVTG_ModeParams_t    VTGModeParams;

    memset((void *)&HDMI_AVIInfoFrame, 0, sizeof(STHDMI_AVIInfoFrame_t));

    HDMI_AVIInfoFrame.AVI861B.FrameType                  = STHDMI_INFOFRAME_TYPE_AVI;
    HDMI_AVIInfoFrame.AVI861B.FrameVersion               = 2;
    HDMI_AVIInfoFrame.AVI861B.FrameLength                = 13;
    HDMI_AVIInfoFrame.AVI861B.HasActiveFormatInformation = TRUE;
    HDMI_AVIInfoFrame.AVI861B.OutputType                 = STVOUT_OUTPUT_DIGITAL_HDMI_RGB888;
    HDMI_AVIInfoFrame.AVI861B.ScanInfo                   = STHDMI_SCAN_INFO_OVERSCANNED;
    HDMI_AVIInfoFrame.AVI861B.Colorimetry                = STVOUT_ITU_R_601;
    HDMI_AVIInfoFrame.AVI861B.ActiveAspectRatio          = STGXOBJ_ASPECT_RATIO_16TO9;
    HDMI_AVIInfoFrame.AVI861B.PictureScaling             = STHDMI_PICTURE_NON_UNIFORM_SCALING;

    ErrCode = STVTG_GetMode(/*VTG_Handler[MAIN_PATH]*/VTGHandle[VTG_MAIN], &VTGTimingMode, &VTGModeParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                       "STVTG_GetMode()=%s",
                       GetErrorText(ErrCode)));
    }
    else
    {
        switch (VTGTimingMode)
        {
            case STVTG_TIMING_MODE_480I60000_13514 :
            case STVTG_TIMING_MODE_480I59940_13500 :
            case STVTG_TIMING_MODE_480P60000_27027 :
            case STVTG_TIMING_MODE_480P30000_13514 :
            case STVTG_TIMING_MODE_480P24000_10811 :
            case STVTG_TIMING_MODE_480P59940_27000 :  /* no break */
            case STVTG_TIMING_MODE_480P29970_13500 :
            case STVTG_TIMING_MODE_480P23976_10800 :
            case STVTG_TIMING_MODE_480P60000_24570:  /* no break */
            case STVTG_TIMING_MODE_480P60000_25200:  /* no break */
            case STVTG_TIMING_MODE_480P59940_25175:  /* no break */
                 HDMI_AVIInfoFrame.AVI861B.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
                 break;
            case STVTG_TIMING_MODE_1080I60000_74250:  /* no break */
            case STVTG_TIMING_MODE_1080I59940_74176:  /* no break */
            case STVTG_TIMING_MODE_1080I50000_74250:  /* no break */
            case STVTG_TIMING_MODE_1080I50000_74250_1: /* no break */
            case STVTG_TIMING_MODE_720P60000_74250: /* no break */
            case STVTG_TIMING_MODE_720P59940_74176: /* no break */
            case STVTG_TIMING_MODE_720P50000_74250:
                 HDMI_AVIInfoFrame.AVI861B.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
                 break;
            default:
                 HDMI_AVIInfoFrame.AVI861B.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
                 break;
        }

        ErrCode = STHDMI_FillAVIInfoFrame(HDMI_Handler[HDMI_HandleId], &HDMI_AVIInfoFrame);
        if (ErrCode != ST_NO_ERROR)
        {
             STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                                  "THDMI_FillAVI(%s)=%s",
                                                  HDMI_Names[0],
                                                  GetErrorText(ErrCode)));
     }
    }

    return (ErrCode);
}/* HDMI_FillAVI() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillSPD
 * Description : Fill SPD Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_FillSPD(HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_SPDInfoFrame_t HDMI_SPDInfoFrame;

    memset((void *)&HDMI_SPDInfoFrame, 0, sizeof(STHDMI_SPDInfoFrame_t));

    HDMI_SPDInfoFrame.FrameLength = 25 ;
    HDMI_SPDInfoFrame.FrameType = STHDMI_INFOFRAME_TYPE_SPD;
    HDMI_SPDInfoFrame.FrameVersion = 2;

    strncpy(HDMI_SPDInfoFrame.ProductDescription, "ST7109", 8);
    strncpy(HDMI_SPDInfoFrame.VendorName, "STM", 16);
    HDMI_SPDInfoFrame.SourceDeviceInfo =  STHDMI_SPD_DEVICE_DIGITAL_STB;

    ErrCode = STHDMI_FillSPDInfoFrame(HDMI_Handler[HDMI_HandleId], &HDMI_SPDInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillSPD(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillSPD() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillAudio
 * Description : Fill Audio Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_FillAudio(HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_AUDIOInfoFrame_t HDMI_AUDIOInfoFrame;

    memset((void *)&HDMI_AUDIOInfoFrame, 0, sizeof(STHDMI_AUDIOInfoFrame_t));

    HDMI_AUDIOInfoFrame.FrameType         = STHDMI_INFOFRAME_TYPE_AUDIO;
    HDMI_AUDIOInfoFrame.FrameVersion      = 2;
    HDMI_AUDIOInfoFrame.CodingType        = STAUD_STREAM_CONTENT_NULL;
    HDMI_AUDIOInfoFrame.SampleSize        = 0xFF;
    HDMI_AUDIOInfoFrame.DownmixInhibit    = FALSE;

    ErrCode = STHDMI_FillAudioInfoFrame(HDMI_Handler[HDMI_HandleId], &HDMI_AUDIOInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillAudio(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillAudio() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillMS
 * Description : Fill MS Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_FillMS(HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_MPEGSourceInfoFrame_t HDMI_MPEGSourceInfoFrame;

    memset((void *)&HDMI_MPEGSourceInfoFrame, 0, sizeof(STHDMI_MPEGSourceInfoFrame_t));

    HDMI_MPEGSourceInfoFrame.FrameLength = 10;
    HDMI_MPEGSourceInfoFrame.FrameType = STHDMI_INFOFRAME_TYPE_MPEG_SOURCE;
    HDMI_MPEGSourceInfoFrame.FrameVersion = 2;
    HDMI_MPEGSourceInfoFrame.IsFieldRepeated = FALSE;
    HDMI_MPEGSourceInfoFrame.MepgFrameType = STVID_MPEG_FRAME_I;  /* Should be updated for every frame displayed. */
    HDMI_MPEGSourceInfoFrame.MPEGBitRate = 10000000;

    ErrCode = STHDMI_FillMSInfoFrame(HDMI_Handler[HDMI_HandleId], &HDMI_MPEGSourceInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillMS(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillMS() */

/*---------------------------------------------------------------------
 * Function     : HDMI_FillVS
 * Description : Fill VS Info Frame, WILL BE FULLY IMPLEMENT LATER
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t HDMI_FillVS(HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_VendorSpecInfoFrame_t HDMI_VendorSpecInfoFrame;
    char VSBuffer[3] = {'S','T','M'};

    memset((void *)&HDMI_VendorSpecInfoFrame, 0, sizeof(STHDMI_VendorSpecInfoFrame_t));

    HDMI_VendorSpecInfoFrame.FrameLength = 1;
    HDMI_VendorSpecInfoFrame.FrameType = STHDMI_INFOFRAME_TYPE_VENDOR_SPEC;
    HDMI_VendorSpecInfoFrame.FrameVersion = 2;
    HDMI_VendorSpecInfoFrame.RegistrationId = 0;
    HDMI_VendorSpecInfoFrame.VSPayload_p = "STM";
    memcpy(HDMI_VendorSpecInfoFrame.VSPayload_p, VSBuffer, 3);

    ErrCode = STHDMI_FillVSInfoFrame(HDMI_Handler[HDMI_HandleId], &HDMI_VendorSpecInfoFrame);

    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "THDMI_FillVS(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* HDMI_FillVS() */

/*---------------------------------------------------------------------
 * Function     : HDMI_Close
 * Description : Close HDMI handle.
 * Input          : HDMI Device ID, HDMI Handle ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t THDMI_Close(HDMI_HandleId_t HDMI_HandleId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;

    ErrCode = STHDMI_Close(HDMI_Handler[HDMI_HandleId]);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "HDMI_Close(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "HDMI_Close(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* THDMI_Close() */

/*---------------------------------------------------------------------
 * Function     : HDMI_Term
 * Description : Terminates HDMI
 * Input          : HDMI Device ID
 * Output        : None
 * Return        : ST Error Code
 * ----------------------------------------------------------------- */
ST_ErrorCode_t THDMI_Term(HDMI_DeviceId_t HDMI_DeviceId)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    STHDMI_TermParams_t HDMI_TermParams;

    memset((void *)&HDMI_TermParams, 0, sizeof(STHDMI_TermParams_t));

    HDMI_TermParams.ForceTerminate = TRUE;

    ErrCode = STHDMI_Term(HDMI_Names[0], &HDMI_TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "HDMI_Term(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }
    else
    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,
                      "HDMI_Term(%s)=%s",
                      HDMI_Names[0],
                      GetErrorText(ErrCode)));
    }

    return (ErrCode);
}/* THDMI_Term() */

/*End of hdmi.c */
