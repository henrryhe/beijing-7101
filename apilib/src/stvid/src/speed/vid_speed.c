/*******************************************************************************

File name   : vid_speed.c

Description : Video Speed source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
12 Jan 2006       Created                                          RH

*******************************************************************************/
/* Private preliminary definitions (internal use only) ---------------------- */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#ifdef TRACE_UART
#include "trace.h"
#endif

#include "stddefs.h"
#include "stos.h"

#include "stsys.h"

#include "sttbx.h"

#include "vid_ctcm.h"

#include "speed.h"
#include "vid_speed.h"


#include "display.h"

#include "stevt.h"
#include "stvtg.h"
#include "stavmem.h"

enum
{
	NO_DEGRADED_MODE = 0,
	DEGRADED_BSLICES,
	DEGRADED_PSLICES
};

/* Private Constants -------------------------------------------------------- */
/* default bitrate if none has been computed */
#define  VIDSPEED_DEFAULT_BITRATE                         37500
#define  VIDSPEED_DEFAULT_FRAME_RATE                      25

#define  VIDSPEED_LEVEL_SKIP_CURRENT_PICTURE              3
#define  VIDSPEED_LEVEL_SKIP_ALL_B                        5
#define  VIDSPEED_LEVEL_SKIP_ALL_B_AND_DISP_P             6
#define  VIDSPEED_LEVEL_SKIP_ALL_P                        14
#define  VIDSPEED_LEVEL_SKIP_ON_HDD                       500
#define  VIDSPEED_LEVEL_TO_REACH_AFTER_SKIP_ON_HDD        20
#define  VIDSPEED_BETWEEN_2_I_MIN_PIC_NB                  2

#define VIDSPEED_SPEED_MIN_SPEED_FOR_NO_NULL_COUNTER      200
#define VIDSPEED_MAX_SPEED_FOR_SMALL_SKIP                 400 /*1000*/
#define  MAX_WAIT_ORDER_TIME_WATCHDOG                     (3 * STVID_MAX_VSYNC_DURATION)

#ifdef STFAE
#include "stapp_trace.h"
#define TraceMessage TRACE_Message
#define TraceState   TRACE_State
#define TraceVal     TRACE_Val
#endif

#ifdef SPEED_DEBUG
#include "vid_trc.h"
/*#include "trace.h"*/
#define TraceDebug(x) TraceMessage x
#define TraceGraph(x) TraceVal x
#define TraceFrameBuffer(x) TraceState x
#endif /* SPEED_DEBUG */

#define TraceSkipRepeat(x) /* TraceVal x */
#define TraceSkipMode(x) /* TraceVal x */
#define TraceDrifts(x) /* TraceVal x */
#define TraceSpeed(x) /* TraceMessage x */
#define TraceGOPStructure(x) /* TraceMessage x */

/* Dummy value that we set to check that a handle is valid, only one chance out of 2^32 to get it by accident ! */
#define  VIDSPEED_VALID_HANDLE 0x01851850

#define  IsStvidPictureInfosExpectingSecondField(PictureInfos_p) \
           ((((PictureInfos_p)->VideoParams.TopFieldFirst) && ((PictureInfos_p)->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD))  || \
            ((!((PictureInfos_p)->VideoParams.TopFieldFirst)) && ((PictureInfos_p)->VideoParams.PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD)))

/* Private Function prototypes ---------------------------------------------- */
static void SpeedInitialize(const VIDSPEED_Handle_t SpeedHandle);
static ST_ErrorCode_t StartSpeedTask(const VIDSPEED_Handle_t SpeedHandle);
static ST_ErrorCode_t StopSpeedTask(const VIDSPEED_Handle_t SpeedHandle);
static void SpeedTaskFunc(const VIDSPEED_Handle_t SpeedHandle);
static void ReadyToDecodeNewPictureSpeed(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void NewPictureSkippedByDecode(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName, STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void NewPictureSkippedByDisplayWhithoutSpeedRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p);
#if (defined(ST_7200) && defined(ST_smoothbackward)) || (!defined(ST_7200))
static void NoNextPictureToDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* (defined(ST_7200) && defined(ST_smoothbackward)) || (!defined(ST_7200)) */
#ifdef STVID_TRICKMODE_BACKWARD
static void ComputeIOnlyBackwardDrift(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void ReceiveFirstDecodedPictureInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void UnderflowEventSent(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* STVID_TRICKMODE_BACKWARD */
static void VTGScanTypeChanged(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static ST_ErrorCode_t GetBitRateValue(const VIDSPEED_Handle_t SpeedHandle, U32 * const BitRateValue_p);
static void InitSpeedStructure(const VIDSPEED_Handle_t SpeedHandle);

#ifdef SPEED_DEBUG
static void SpeedErrorLine(U32 Line);
#define SpeedError()       SpeedErrorLine(__LINE__)
#endif /* SPEED_DEBUG */

/* Macros --------------------------------------------------------------------- */
#define CAST_CONST_HANDLE_TO_DATA_TYPE( DataType, Handle )  \
    ((DataType)(*(DataType*)((void*)&(Handle))))

/*******************************************************************************
Name        : VIDSPEED_Init
Description :
Parameters  : Init params, returns Speed handle
Assumptions : InitParams_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if already initialised
              ST_ERROR_BAD_PARAMETER if bad parameter
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_Init(const VIDSPEED_InitParams_t * const InitParams_p, VIDSPEED_Handle_t * const SpeedHandle_p)
{
    VIDSPEED_Data_t *                   VIDSPEED_Data_p;
    STEVT_OpenParams_t                  STEVT_OpenParams;
    STEVT_DeviceSubscribeParams_t       STEVT_SubscribeParams;
    ST_ErrorCode_t                      ErrorCode;

    /* Allocate a HALTRICK structure */
    SAFE_MEMORY_ALLOCATE(VIDSPEED_Data_p, VIDSPEED_Data_t * ,InitParams_p->CPUPartition_p, sizeof(VIDSPEED_Data_t));
    if (VIDSPEED_Data_p == NULL)
    {
        /* Error: allocation failed */
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_NO_MEMORY);
    }

    /* Remember partition */
    VIDSPEED_Data_p->CPUPartition_p = InitParams_p->CPUPartition_p;

    /* Allocation succeeded: make handle point on structure */
    *SpeedHandle_p = (VIDSPEED_Handle_t *) VIDSPEED_Data_p;
    VIDSPEED_Data_p->ValidityCheck = VIDSPEED_VALID_HANDLE;

    /* Store parameters */
    VIDSPEED_Data_p->DecoderNumber = InitParams_p->DecoderNumber;
    VIDSPEED_Data_p->DisplayHandle = InitParams_p->DisplayHandle;

    VIDSPEED_Data_p->ProducerHandle = (VIDPROD_Handle_t *) InitParams_p->ProducerHandle;

    VIDSPEED_Data_p->AvmemPartitionHandle = InitParams_p->AvmemPartitionHandle;
#ifdef ST_ordqueue
    VIDSPEED_Data_p->OrderingQueueHandle = InitParams_p->OrderingQueueHandle;
#endif /* ST_ordqueue */
    VIDSPEED_Data_p->MainParams.Speed = 100;
    VIDSPEED_Data_p->MainParams.RequiredSpeed = 100;
    VIDSPEED_Data_p->MainParams.BitRateValue = VIDSPEED_DEFAULT_BITRATE;
    VIDSPEED_Data_p->MainParams.FrameRateValue = VIDSPEED_DEFAULT_FRAME_RATE * 1000 ; /* Hz -> mHz */
    VIDSPEED_Data_p->MainParams.VTGScanType = STGXOBJ_INTERLACED_SCAN;
    VIDSPEED_Data_p->Flags.SpeedControlEnabled = FALSE;
    VIDSPEED_Data_p->Flags.DisplayManagedBySpeed = TRUE;
    VIDSPEED_Data_p->WaitFirstDecode = FALSE;
    InitSpeedStructure(*SpeedHandle_p);

    VIDSPEED_Data_p->EventDisplayChangeToBeNotifiedInTask = FALSE;
#ifdef STVID_DEBUG_GET_STATISTICS
    VIDSPEED_Data_p->Statistics.SpeedSkipReturnNone = 0;
    VIDSPEED_Data_p->Statistics.SpeedRepeatReturnNone = 0;
#endif /*STVID_DEBUG_GET_STATISTICS*/

    /* Semaphore protection init */
    VIDSPEED_Data_p->SpeedParamsSemaphore_p = STOS_SemaphoreCreatePriority(VIDSPEED_Data_p->CPUPartition_p, 1);
    VIDSPEED_Data_p->WakeSpeedTaskUp_p = STOS_SemaphoreCreateFifoTimeOut(VIDSPEED_Data_p->CPUPartition_p, 0);

    /* Open EVT handle */
    ErrorCode = STEVT_Open(InitParams_p->EventsHandlerName, &STEVT_OpenParams, &(VIDSPEED_Data_p->EventsHandle));
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: open failed, undo initialisations done */
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        VIDSPEED_Term(*SpeedHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }

    /* Register event to reached asked speed if necessary (skip on HDD) */
    ErrorCode = STEVT_RegisterDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, STVID_SPEED_DRIFT_THRESHOLD_EVT, &(VIDSPEED_Data_p->RegisteredEventsID[VIDSPEED_SPEED_DRIFT_THRESHOLD_EVT_ID]));
    if (ErrorCode == ST_NO_ERROR)
    {
        ErrorCode = STEVT_RegisterDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT, &(VIDSPEED_Data_p->RegisteredEventsID[VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT_ID]));
    }
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: registration failed, undo initialisations done */
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        VIDSPEED_Term(*SpeedHandle_p);
        return(STVID_ERROR_EVENT_REGISTRATION);
    }
    /* Start Trick Mode task */
    VIDSPEED_Data_p->SpeedTask.IsRunning = FALSE;
    ErrorCode = StartSpeedTask(*SpeedHandle_p);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: task creation failed, undo initialisations done */
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Module Trick mode init: could not start Trick mode task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
        VIDSPEED_Term(*SpeedHandle_p);
        if (ErrorCode == ST_ERROR_ALREADY_INITIALIZED)
        {
            /* Change error code because ST_ERROR_ALREADY_INITIALIZED is not allowed for _Init() functions */
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ErrorCode);
    }

    /* Prepare subscribe parameters for all subscribes */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p);

    /* Subscribe to ready to decode new picture */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)ReadyToDecodeNewPictureSpeed;
	ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_DECODE_LAUNCHED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Subscribe to new picture skipped by decode   */

    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)NewPictureSkippedByDecode;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_PICTURE_SKIPPED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

	/* Subscribe to new picture skipped by display      */
    STEVT_SubscribeParams.NotifyCallback = NewPictureSkippedByDisplayWhithoutSpeedRequest;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
#ifdef STVID_TRICKMODE_BACKWARD
    /* Subscribe to buffer fully parsed      */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)ComputeIOnlyBackwardDrift;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_BUFFER_FULLY_PARSED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Subscribe to first picture decoded in bit buffer      */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)ReceiveFirstDecodedPictureInBitBuffer;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDPROD_DECODE_FIRST_PICTURE_BACKWARD_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    /* Subscribe to underflow event send */
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)UnderflowEventSent;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, STVID_DATA_UNDERFLOW_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

#endif /* STVID_TRICKMODE_BACKWARD */


#if (defined(ST_7200) && defined(ST_smoothbackward)) || (!defined(ST_7200))
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)NoNextPictureToDisplay;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_NO_NEXT_PICTURE_TO_DISPLAY_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
#endif /* (defined(ST_7200) && defined(ST_smoothbackward)) || (!defined(ST_7200)) */

    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t)VTGScanTypeChanged;
    /* STEVT_SubscribeParams.SubscriberData_p = (void *) (*SpeedHandle_p); */
    ErrorCode = STEVT_SubscribeDeviceEvent(VIDSPEED_Data_p->EventsHandle, InitParams_p->VideoName, VIDDISP_VTG_SCANTYPE_CHANGE_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
        /* Error: subscribe failed, undo initialisations done */
        VIDSPEED_Term(*SpeedHandle_p);
#ifdef SPEED_DEBUG
        SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    return(ErrorCode);
} /* end of VIDSPEED_Init() */
/*******************************************************************************
Name        : InitSpeedStructure
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
static void InitSpeedStructure(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDSPEED_Data_t   * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;

    VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat = 0;
    TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    VIDSPEED_Data_p->MainParams.VirtualClockCounter = 0;
    VIDSPEED_Data_p->MainParams.BitRateValue = VIDSPEED_DEFAULT_BITRATE;
    VIDSPEED_Data_p->MainParams.FrameRateValue = VIDSPEED_DEFAULT_FRAME_RATE * 1000 ; /* Hz -> mHz */
    VIDSPEED_Data_p->Flags.SecondFieldHasToBeSkipped = FALSE;
    VIDSPEED_Data_p->Flags.WaitingForASecondField = FALSE;
    VIDSPEED_Data_p->Flags.SpeedReinitNeeded = FALSE;
    VIDSPEED_Data_p->AverageFieldsNb = 0;
	VIDSPEED_Data_p->ZeroCounter = 0;

    /* Display parameters init */
    VIDSPEED_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
    VIDSPEED_Data_p->DisplayParams.DisplayForwardNotBackward = TRUE;
    VIDSPEED_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = TRUE;
}
/*******************************************************************************
Name        : VIDSPEED_SetSpeed
Description : Sets speed of video
Parameters  : Video Speed handle, Speed
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_SetSpeed(const VIDSPEED_Handle_t SpeedHandle, const S32 Speed)
{
    VIDSPEED_Data_t   * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
#ifdef STVID_TRICKMODE_BACKWARD
    S32 SpeedBeforeChange = VIDSPEED_Data_p->MainParams.Speed;
#endif

#ifndef STVID_TRICKMODE_BACKWARD

    if (Speed < 0)
    {
        VIDSPEED_Data_p->MainParams.RequiredSpeed = 100;
        VIDSPEED_Data_p->MainParams.LastSpeed = 100;
        STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Negative speed is not supported"));
    }
    else
#endif /* STVID_TRICKMODE_BACKWARD */
    {
        /* Required Speed init */
        VIDSPEED_Data_p->MainParams.RequiredSpeed = Speed;
        VIDSPEED_Data_p->MainParams.LastSpeed = Speed;
    }
    /* Lock Access to some parameters of Trick mode datas */
    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
    VIDPROD_SkipData(VIDSPEED_Data_p->ProducerHandle, VIDPROD_SKIP_NO_PICTURE);
    VIDSPEED_Data_p->AverageFieldsNb = 0;
	VIDSPEED_Data_p->ZeroCounter = 0;
    VIDSPEED_Data_p->WaitFirstDecode = TRUE;
    VIDSPEED_Data_p->MainParams.VirtualClockCounter = 0;
#ifdef STVID_TRICKMODE_BACKWARD
    VIDSPEED_Data_p->BackwardIOnly.NbDisplayedPictures = 0;
    VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField = 0;
    VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField = 2;
    VIDSPEED_Data_p->BackwardIOnly.NbVsyncToDisplayEachField = 0;
    VIDSPEED_Data_p->BackwardIOnly.IsBufferParsed = FALSE;
    VIDSPEED_Data_p->BackwardIOnly.IsDisplayEnable = FALSE;
    VIDSPEED_Data_p->BackwardIOnly.IsBufferReadyForDisplay = FALSE;
    VIDSPEED_Data_p->BackwardIOnly.IsNewBufferStartedDecoded = FALSE;
    VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer = 0;
    VIDSPEED_Data_p->BackwardIOnly.VSyncUsed = 0;
    VIDSPEED_Data_p->BackwardIOnly.PreviousSkip = 0;
    VIDSPEED_Data_p->BackwardIOnly.PreviousBitBufferUsedForDisplay_p = NULL;
    VIDSPEED_Data_p->BackwardIOnly.IsDisplayedBufferChanged = FALSE;
    VIDSPEED_Data_p->BackwardIOnly.DoesUnderflowEventOccur = FALSE;
#endif /* STVID_TRICKMODE_BACKWARD */
    VIDSPEED_Data_p->Flags.SecondFieldHasToBeSkipped = FALSE;
    VIDSPEED_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_CARE_MPEG_FRAME;
    VIDPROD_ResetDriftTime(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle);

#ifdef STVID_TRICKMODE_BACKWARD
    if ((Speed < 0) && (VIDSPEED_Data_p->MainParams.Speed >= 0))
    {
        VIDPROD_ChangeDirection(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle, TRICKMODE_CHANGING_BACKWARD);
    }
    if ((Speed >= 0) && (VIDSPEED_Data_p->MainParams.Speed < 0))
    {
        VIDPROD_ChangeDirection(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle, TRICKMODE_CHANGING_FORWARD);
    }

#endif /*STVID_TRICKMODE_BACKWARD*/

    SpeedInitialize(SpeedHandle);
    VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat = 0;
    TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    /* Un lock Access to some parameters of Trick mode data */
    STOS_SemaphoreSignal(VIDSPEED_Data_p->SpeedParamsSemaphore_p);

#ifdef STVID_TRICKMODE_BACKWARD
    if (((Speed < 0) && (SpeedBeforeChange >= 0)) ||
        ((Speed >= 0) && (SpeedBeforeChange < 0)))
        {
            VIDQUEUE_CancelAllPictures(VIDSPEED_Data_p->OrderingQueueHandle);
            VIDDISP_FreeNotDisplayedPicturesFromDisplayQueue(VIDSPEED_Data_p->DisplayHandle);
        }
#endif /* STVID_TRICKMODE_BACKWARD */

#ifdef SPEED_DEBUG
    TraceBuffer(("Speed set  %d \r\n", Speed));
#endif
    TraceSpeed(("SP", "%d", Speed));

    return (ErrorCode);

} /* End of VIDSPEED_SetSpeed() function */
/*******************************************************************************
Name        : SpeedInitialize
Description : Sets real speed of video and a lot of parameters
Parameters  : Video Speed handle, Speed
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
*******************************************************************************/
static void SpeedInitialize(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDSPEED_Data_t   * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    U32   PositiveOfRequiredSpeed;
    U32   PositiveOfCurrentSpeed;

    PositiveOfRequiredSpeed =  (VIDSPEED_Data_p->MainParams.RequiredSpeed >= 0)
                             ? (VIDSPEED_Data_p->MainParams.RequiredSpeed)
                             : (-VIDSPEED_Data_p->MainParams.RequiredSpeed);
    PositiveOfCurrentSpeed =  (VIDSPEED_Data_p->MainParams.Speed >= 0)
                            ? (VIDSPEED_Data_p->MainParams.Speed)
                            : (-VIDSPEED_Data_p->MainParams.Speed);

    /* Ask the producer not to skip any pictures */
    VIDPROD_SkipData(VIDSPEED_Data_p->ProducerHandle, VIDPROD_SKIP_NO_PICTURE);
    VIDPROD_SetDecodingMode(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle, NO_DEGRADED_MODE);

    /* Compute New value of MainParams.NbFieldsIfPosToSkipIfNegToRepeat */
    if (PositiveOfRequiredSpeed < VIDSPEED_SPEED_MIN_SPEED_FOR_NO_NULL_COUNTER)
    {
        VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat = 0;
    }
    else /*if (PositiveOfRequiredSpeed < VIDSPEED_MAX_SPEED_FOR_SMALL_SKIP)*/
    {
        VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat = 0;/*PositiveOfRequiredSpeed / 100;*/
    }
    TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    VIDSPEED_Data_p->AverageFieldsNb = 0;
	VIDSPEED_Data_p->ZeroCounter = 0;

    /* Initialize new Speed */
    VIDSPEED_Data_p->MainParams.Speed = VIDSPEED_Data_p->MainParams.RequiredSpeed;

    /* forward init params */
    if (VIDSPEED_Data_p->MainParams.Speed >= 0)
    {
        VIDSPEED_Data_p->DisplayParams.DisplayForwardNotBackward = TRUE;

        VIDSPEED_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = FALSE;
        /* Speed 100%: care fields polarity matching when displaying */
        if (VIDSPEED_Data_p->MainParams.Speed == 100)
        {
            VIDSPEED_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = TRUE;
        }
    }
	else
	{
		if (VIDSPEED_Data_p->MainParams.Speed == -100)
        {
            VIDSPEED_Data_p->DisplayParams.DisplayWithCareOfFieldsPolarity = TRUE;
        }
		VIDSPEED_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
	}
    /* Set display params */
    /* Cannot notify event VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT here because we may be in API call, notify it right after in task */
    VIDSPEED_Data_p->EventDisplayChangeToBeNotifiedInTask = TRUE;
    /* Trick Mode Task has to be waked up so that Fields have to be skipped if necessary. */
    STOS_SemaphoreSignal(VIDSPEED_Data_p->WakeSpeedTaskUp_p);

} /* End of SpeedInitialize() function */

/*******************************************************************************
Name        : VIDSPEED_Term
Description : Terminate Speed, undo all what was done at init
Parameters  : Speed handle
Assumptions : Term can be called in init when init process fails: function to undo
              things done at init should not cause trouble to the system
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_INVALID_HANDLE if not initialised
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_Term(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDSPEED_Data_t  * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    ST_ErrorCode_t     ErrorCode = ST_NO_ERROR;

    if (VIDSPEED_Data_p->ValidityCheck != VIDSPEED_VALID_HANDLE)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Stop Trick Mode Task */
    StopSpeedTask(SpeedHandle);

    /* Close instances opened at init */
    /* Un-subscribe and un-register events: this is done automatically by STEVT_Close() */
    ErrorCode = STEVT_Close(VIDSPEED_Data_p->EventsHandle);

    /* Lock Speed params before delete */
    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);

    /* semaphore delete */
    STOS_SemaphoreDelete(VIDSPEED_Data_p->CPUPartition_p, VIDSPEED_Data_p->SpeedParamsSemaphore_p);
    STOS_SemaphoreDelete(VIDSPEED_Data_p->CPUPartition_p, VIDSPEED_Data_p->WakeSpeedTaskUp_p);

    /* De-validate structure */
    VIDSPEED_Data_p->ValidityCheck = 0; /* not VIDSPEED_VALID_HANDLE */

    /* De-allocate last: data inside cannot be used after ! */
    SAFE_MEMORY_DEALLOCATE(VIDSPEED_Data_p, VIDSPEED_Data_p->CPUPartition_p, sizeof(VIDSPEED_Data_t));

    return(ErrorCode);

} /* end of VIDSPEED_Term() */
/*******************************************************************************
Name        : StopSpeedTask
Description : Stop the task of Speed control
Parameters  : Speed handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopSpeedTask(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDSPEED_Data_t *) SpeedHandle)->SpeedTask);
    VIDSPEED_Data_t * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    task_t *TaskList_p ;

    if (!(Task_p->IsRunning))
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    TaskList_p = Task_p->Task_p;

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* Wake the task up so that it can terminate */
    STOS_SemaphoreSignal(VIDSPEED_Data_p->WakeSpeedTaskUp_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                                  VIDSPEED_Data_p->CPUPartition_p,
                                  Task_p->TaskStack,
                                  VIDSPEED_Data_p->CPUPartition_p);

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopSpeedTask() function */
/*******************************************************************************
Name        : StartSpeedTask
Description : Start the task of Trick Mode control
Parameters  : Trick Mode handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartSpeedTask(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDCOM_Task_t * const Task_p = &(((VIDSPEED_Data_t *) SpeedHandle)->SpeedTask);
    VIDSPEED_Data_t * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR ;
    char TaskName[23] = "STVID[?].SpeedTask";

    if (Task_p->IsRunning)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + VIDSPEED_Data_p->DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    ErrorCode = STOS_TaskCreate((void (*) (void*)) SpeedTaskFunc,
                                (void *) SpeedHandle,
                                ((VIDSPEED_Data_t *) SpeedHandle)->CPUPartition_p,
                                STVID_TASK_STACK_SIZE_SPEED,
                                &(Task_p->TaskStack),
                                ((VIDSPEED_Data_t *) SpeedHandle)->CPUPartition_p,
                                &(Task_p->Task_p),
                                &(Task_p->TaskDesc),
                                STVID_TASK_PRIORITY_SPEED,
                                TaskName,
                                task_flags_no_min_stack_size);
    if (ErrorCode != ST_NO_ERROR)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ErrorCode);
    }

    Task_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Speed task created."));
    return(ST_NO_ERROR);
} /* End of StartSpeedTask() function */
/*******************************************************************************
Name        : SpeedTaskFunc
Description : Function of the Speed task
              Set VIDSPEED_Data_p->SpeedTask.ToBeDeleted to end the task
Parameters  : Speed handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void SpeedTaskFunc(const VIDSPEED_Handle_t VideoSpeedHandle)
{
    VIDSPEED_Data_t * VIDSPEED_Data_p;

    VIDSPEED_Handle_t SpeedHandle;
    clock_t         MaxWaitOrderTimeWatchDog;

    STOS_TaskEnter(NULL);

#ifdef REAL_OS40
    SpeedHandle = GlobalContext;
    semV(&ContextAccess);
#else /* REAL_OS40 */
    SpeedHandle = VideoSpeedHandle;
#endif /* not REAL_OS40 */

     VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;

    do
    {
        if (VIDSPEED_Data_p->MainParams.Speed == 100)
        {
            /* There is nothing to do, so semaphore waits infinitely. */
            STOS_SemaphoreWait(VIDSPEED_Data_p->WakeSpeedTaskUp_p);
        }
        else
        {
            /* Trick Mode Task has to wait for VSYNC subscribe callback */
            MaxWaitOrderTimeWatchDog = time_plus(time_now(), MAX_WAIT_ORDER_TIME_WATCHDOG);
            STOS_SemaphoreWaitTimeOut(VIDSPEED_Data_p->WakeSpeedTaskUp_p, &MaxWaitOrderTimeWatchDog);
#ifdef SPEED_DEBUG
        /*TraceGraph(("NB fields to skip or repeat", "%d ", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));*/
#endif /* SPEED_DEBUG */
            while (STOS_SemaphoreWaitTimeOut(VIDSPEED_Data_p->WakeSpeedTaskUp_p, TIMEOUT_IMMEDIATE) == 0)
            {
                /* Just pop all semaphore signals, so that in case of multiple signal there is only one wait, and not many for nothing... */;
            }
            if (VIDSPEED_Data_p->EventDisplayChangeToBeNotifiedInTask)
            {
                /* Notify event so that API level can choose to change display settings or not. */
                VIDSPEED_Data_p->EventDisplayChangeToBeNotifiedInTask = FALSE;
                STEVT_Notify(VIDSPEED_Data_p->EventsHandle, VIDSPEED_Data_p->RegisteredEventsID[VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDSPEED_Data_p->DisplayParams));
            }
        }
    } while (!(((VIDSPEED_Data_t *) SpeedHandle)->SpeedTask.ToBeDeleted));


    STOS_TaskExit(NULL);
} /* End of SpeedTaskFunc() function */
/*******************************************************************************
Name        : NewPictureSkippedByDecode
Description : Function to subscribe VIDPROD_PICTURE_SKIPPED_EVT.
              Removes the number of fields from NbFieldsIfPosToSkipIfNegToRepeat
              when decode Error recovery skips one picture.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void NewPictureSkippedByDecode(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                      STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{

    VIDSPEED_Data_t *              VIDSPEED_Data_p;
    U32                            NbFieldsShouldBeDisplayed;
    VIDSPEED_Handle_t 			   SpeedHandle ;

    SpeedHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Handle_t, SubscriberData_p);
    VIDSPEED_Data_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Data_t *, SpeedHandle);
    NbFieldsShouldBeDisplayed = *((U32*)EventData_p);

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    /* Avoid semaphore locks when speed is x1 */
    if(VIDSPEED_Data_p->MainParams.Speed == 100)
    {
        return;
    }

    /* Update the Skip/Repeat variable */
    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
    if((VIDSPEED_Data_p->MainParams.Speed >= 0) && (VIDSPEED_Data_p->Flags.SpeedControlEnabled) && (!VIDSPEED_Data_p->WaitFirstDecode))
    {
        VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat -= (S32) NbFieldsShouldBeDisplayed;
        TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    }
    STOS_SemaphoreSignal(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
} /* end of NewPictureSkippedByDecode() function */

/*******************************************************************************
Name        : NewPictureSkippedByDisplayWhithoutSpeedRequest
Description : Function to subscribe VIDDISP_NEW_PICTURE_SKIPPED_NOT_REQUESTED_EVT.
              Removes the number of fields from NbFieldsIfPosToSkipIfNegToRepeat
              when display skips one too late picture because.
Parameters  :
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void NewPictureSkippedByDisplayWhithoutSpeedRequest(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                      STEVT_EventConstant_t Event, const void *EventData_p, const void *SubscriberData_p)
{
    VIDSPEED_Data_t *        VIDSPEED_Data_p;
	VIDSPEED_Handle_t        SpeedHandle;

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);
	UNUSED_PARAMETER(EventData_p);

    SpeedHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Handle_t, SubscriberData_p);
    VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;

	if(VIDSPEED_Data_p->MainParams.Speed == 100)
    {
        return;
    }
    /* Update the Skip/Repeat variable */
    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
    if((VIDSPEED_Data_p->MainParams.Speed >= 0) && (VIDSPEED_Data_p->Flags.SpeedControlEnabled))
    {
        VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat--;
        TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    }
    STOS_SemaphoreSignal(VIDSPEED_Data_p->SpeedParamsSemaphore_p);

} /* end of NewPictureSkippedByDisplayWhithoutSpeedRequest() function */

/********************************************************************************
* Name        : VIDSPEED_GetSpeed
* Description : Gets speed of video
* Parameters  : Video Speed handle, Speed
* Assumptions :
* Limitations :
* Returns     : ST_NO_ERROR if success,
* *******************************************************************************/
ST_ErrorCode_t VIDSPEED_GetSpeed(const VIDSPEED_Handle_t SpeedHandle, S32 * const Speed_p)
{
    VIDSPEED_Data_t * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;

    *Speed_p = VIDSPEED_Data_p->MainParams.RequiredSpeed;

    return(ST_NO_ERROR);
} /* end of VIDSPEED_GetSpeed() function */
/*******************************************************************************
 * Name        : VIDSPEED_DisableSpeedControl
 * Description : Disables Speed control when video is stopped or paused
 * Parameters  : Speed handle
 * Assumptions : MUST be called by VID_Stop !
 * Limitations :
 * Returns     : ST_NO_ERROR
 * *******************************************************************************/
ST_ErrorCode_t VIDSPEED_DisableSpeedControl(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDSPEED_Data_t * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    /* disable control speed */
    VIDSPEED_Data_p->Flags.SpeedControlEnabled = FALSE;
    return(ST_NO_ERROR);
} /* end of VIDSPEED_DisableSpeedControl() function */

/*******************************************************************************
* Name        : VIDSPEED_EnableSpeedControl
* Description : Enables Speed control when video is started
* Parameters  : Speed handle
* Assumptions : MUST be called by VID_Start !
* Limitations :
* Returns     : ST_NO_ERROR
* *******************************************************************************/
ST_ErrorCode_t VIDSPEED_EnableSpeedControl(const VIDSPEED_Handle_t SpeedHandle)
{
    VIDSPEED_Data_t * VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;

    /* enable control speed */
    VIDSPEED_Data_p->Flags.SpeedControlEnabled = TRUE;

    return(ST_NO_ERROR);
} /* end of VIDSPEED_EnableSpeedControl() function */

/*******************************************************************************
Name        : VIDSPEED_Stop
Description : Stops Speed
Parameters  :
Assumptions : should be called if STVID_Stop required when playing smooth backward. Linear bit buffers should
              be deallocated
Limitations :
Returns     : STAVMEM deallocation Errors, ST_NO_ERROR
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_Stop(const VIDSPEED_Handle_t SpeedHandle, const STVID_Stop_t StopMode, BOOL * const SpeedWillStopDecode_p, const STVID_Freeze_t * const Freeze_p)
{
    VIDSPEED_Data_t    * VIDSPEED_Data_p;
    ST_ErrorCode_t       ErrorCode;

	ErrorCode = ST_NO_ERROR;
	VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
	/* unused parameters */
	UNUSED_PARAMETER(Freeze_p);

    /* if stop now, disable control speed now */
    if (StopMode == STVID_STOP_NOW)
    {
        ErrorCode = VIDSPEED_DisableSpeedControl(SpeedHandle);
    }

    if (VIDSPEED_Data_p->MainParams.Speed >= 0)
    {
        /* if speed > 0, Nothing to do. */
        *SpeedWillStopDecode_p = FALSE;

        return(ST_NO_ERROR);
    }

    *SpeedWillStopDecode_p = TRUE;
    return(ErrorCode);
} /* end of VIDSPEED_Stop() function */
/*******************************************************************************
Name        : VIDSPEED_VsyncActionSpeed
Description : Function which has to be called every Vsync occurence. It computes the number of fileds to
              be skipped or repeated, Skipd or Repeats fields in display module in order to reach teh required speed.
Parameters  :
Assumptions : None
Limitations : None
Returns     : None
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_VsyncActionSpeed(const VIDSPEED_Handle_t SpeedHandle, BOOL IsReferenceDisplay)
{
    VIDSPEED_Data_t  * VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;

    S32    VirtualClockCounter;
    S32    TimeElapsed = 0;
    /* slow motion */
    S32    NbFieldsToRepeat = 0;
    U32    NbFieldsRepeated = 0;
    /* Fast forward */
    U32    NbFieldsToSkip = 0;
    U32    NbFieldsSkipped = 0;
    ST_ErrorCode_t                ErrorCode;
    STVID_SpeedDriftThreshold_t   SpeedDriftThresholdData;
#ifdef STVID_TRICKMODE_BACKWARD
    /*VIDBUFF_PictureBuffer_t       CurrentlyDisplayedPictureBuffer;*/
    S32                           SkipThatShouldHaveBeenDone;
#endif

    ErrorCode = ST_NO_ERROR;
    if (VIDSPEED_Data_p->ValidityCheck != VIDSPEED_VALID_HANDLE)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }
    /* Lock semaphore protecting Trick mode private params. This will prohibit the Speed task from accessing to
     * the Speed context when the display task is woken up. The Speed context will be unlocked
     * at the end of the display processing. It is important to lock the Speed context while display task is acting
     * to avoid deadlocks. */
    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);

    /* Speed actions are done only on the reference display (but SpeedParamsMutex_p should be locked anyway) */
    if (!IsReferenceDisplay)
    {
        return(ST_NO_ERROR);
    }
    if (VIDSPEED_Data_p->Flags.SpeedReinitNeeded)
    {
        return(ST_NO_ERROR);
    }
#ifdef STVID_TRICKMODE_BACKWARD
    if (VIDSPEED_Data_p->MainParams.Speed < 0)
    {
        VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer ++;
        if(VIDDISP_GetDisplayedPictureExtendedInfos(VIDSPEED_Data_p->DisplayHandle, &(VIDSPEED_Data_p->BackwardIOnly.CurrentlyDisplayedPictureBuffer)) == ST_NO_ERROR)
        {
            if((VIDSPEED_Data_p->BackwardIOnly.PreviousBitBufferUsedForDisplay_p != NULL)&&
               (VIDSPEED_Data_p->BackwardIOnly.PreviousBitBufferUsedForDisplay_p != VIDSPEED_Data_p->BackwardIOnly.CurrentlyDisplayedPictureBuffer.SmoothBackwardPictureInfo_p))
            {
                /* New linear bitbuffer for displayed frame buffer */

                /* Compute skips */
                SkipThatShouldHaveBeenDone = (VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer * (-VIDSPEED_Data_p->MainParams.Speed)) / 100;
                SkipThatShouldHaveBeenDone = (SkipThatShouldHaveBeenDone * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / ((VIDSPEED_Data_p->MainParams.FrameRateValue ) / 1000);

                VIDSPEED_Data_p->BackwardIOnly.IsDisplayedBufferChanged = TRUE;

                VIDSPEED_Data_p->BackwardIOnly.VSyncUsed = VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer;
                /* Reinit counter */
                VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer = 0;

            }
            VIDSPEED_Data_p->BackwardIOnly.PreviousBitBufferUsedForDisplay_p = VIDSPEED_Data_p->BackwardIOnly.CurrentlyDisplayedPictureBuffer.SmoothBackwardPictureInfo_p;
        }
    }
#endif /* STVID_TRICKMODE_BACKWARD */
    if (VIDSPEED_Data_p->Flags.DisplayManagedBySpeed)
    {
        if ((!VIDSPEED_Data_p->Flags.SpeedControlEnabled) || (VIDSPEED_Data_p->MainParams.Speed == 100))
        {
            return(ST_NO_ERROR);
        }
#ifdef STVID_TRICKMODE_BACKWARD

        /* Here we compute how many times we need to skip or repeat a field to achieve the required speed */
        /*VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer ++;*/
        if (VIDSPEED_Data_p->MainParams.Speed < 0)
        {
            if (VIDSPEED_Data_p->BackwardIOnly.IsBufferReadyForDisplay)
            {
                if ((VIDSPEED_Data_p->BackwardIOnly.NbDisplayedPictures < (VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInDisplayedBuffer + 1)) ||
                    ((VIDSPEED_Data_p->BackwardIOnly.NbDisplayedPictures >= (VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInDisplayedBuffer + 1)
                    && (!VIDSPEED_Data_p->BackwardIOnly.IsNewBufferStartedDecoded))))
                {
                    if (VIDSPEED_Data_p->MainParams.VirtualClockCounter > 1)
                    {
                        NbFieldsToRepeat = 1;
                        VIDDISP_RepeatFields(VIDSPEED_Data_p->DisplayHandle, (U32)NbFieldsToRepeat, &NbFieldsRepeated);
                        VIDSPEED_Data_p->MainParams.VirtualClockCounter --;
                    }
                    else
                    {
                        VIDSPEED_Data_p->MainParams.VirtualClockCounter += 2*VIDSPEED_Data_p->BackwardIOnly.NbVsyncToDisplayEachField;
                        VIDSPEED_Data_p->BackwardIOnly.NbDisplayedPictures ++;
                    }
                }
                else
                {
                    if (VIDSPEED_Data_p->BackwardIOnly.IsNewBufferStartedDecoded)
                    {
                        VIDSPEED_Data_p->BackwardIOnly.NbDisplayedPictures = 0;
                        VIDSPEED_Data_p->BackwardIOnly.NbVsyncToDisplayEachField = VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField;
                        VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInDisplayedBuffer = VIDSPEED_Data_p->BackwardIOnly.NbIPicturesForNextDisplayedBuffer;
                        VIDSPEED_Data_p->BackwardIOnly.IsNewBufferStartedDecoded = FALSE;
#ifdef SPEED_DEBUG
                        TraceBuffer(("Each field displayed for %d, before %d, nb picture %d, display from time %d \r\n",
                             VIDSPEED_Data_p->BackwardIOnly.NbVsyncToDisplayEachField,
                             VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField,
                             VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInDisplayedBuffer,
                             VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField));
#endif /* SPEED_DEBUG */
                    }
                }
            }
        }
        else
#endif /* STVID_TRICKMODE_BACKWARD */
        {
            if(VIDSPEED_Data_p->MainParams.VTGScanType == STGXOBJ_INTERLACED_SCAN)
            {
                TimeElapsed = VIDSPEED_Data_p->MainParams.Speed ;
            }
            else
            {
                TimeElapsed = VIDSPEED_Data_p->MainParams.Speed * 2;
            }

            /* Compute MainParams.NbFieldsIfPosToSkipIfNegToRepeat and skip fields if necessary */
            VirtualClockCounter = VIDSPEED_Data_p->MainParams.VirtualClockCounter;

            /* Compute slow motion (backward or forward) */
            /* MAJ of Trick Mode Fields to skip or to repeat counter */
            if (TimeElapsed < 100)
            {
                VirtualClockCounter = VirtualClockCounter - 100 + TimeElapsed;
                if (VirtualClockCounter < 0)
                {
                    /* the current field displayed has to be displayed one more time */
                    VirtualClockCounter += 100;
                    VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat --;
                }
            }
            /* If Fast (forward or backward) */
            else
            {
                /* Compute number of fields to skip */
                NbFieldsToSkip = (TimeElapsed + VirtualClockCounter) / 100 - 1;

                /* MAJ Virtual clock */
                VirtualClockCounter = (TimeElapsed + VirtualClockCounter) % 100;
                if (VirtualClockCounter > TimeElapsed)
                {
                    VirtualClockCounter = VirtualClockCounter - TimeElapsed;
                }

                if (NbFieldsToSkip)
                {
                    VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat += (S32) NbFieldsToSkip;
                }
            }
            VIDSPEED_Data_p->MainParams.VirtualClockCounter = VirtualClockCounter ;
            /* End of computing MainParams.NbFieldsIfPosToSkipIfNegToRepeat against VirtualClockCounter */

            /* If Fields to repeat */
            if (VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat < 0)
            {
                /* NbFieldsToRepeat = -(VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat); */
                /* NbFieldsToRepeat should not be > 1 ! */
                NbFieldsToRepeat = 1;

                /* Repeat displayed field */
                VIDDISP_RepeatFields(VIDSPEED_Data_p->DisplayHandle, (U32)NbFieldsToRepeat, &NbFieldsRepeated);
                VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat += (S32) NbFieldsRepeated;
#ifdef STVID_DEBUG_GET_STATISTICS
                if (NbFieldsRepeated == 0)
                {
                    VIDSPEED_Data_p->Statistics.SpeedRepeatReturnNone ++;
                }
#endif /*STVID_DEBUG_GET_STATISTICS*/

            }
            /* if Fields to skip */
            else if((VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat > 0) && (VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat <= VIDSPEED_LEVEL_SKIP_ON_HDD))
            {
                NbFieldsToSkip = (U32)(VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat);

                /* Skip fields */
                VIDDISP_SkipFields(VIDSPEED_Data_p->DisplayHandle, (U32)NbFieldsToSkip, &NbFieldsSkipped, FALSE, VIDDISP_SKIP_STARTING_FROM_CURRENTLY_DISPLAYED_PICTURE);
                VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat -= (S32) NbFieldsSkipped;

#ifdef STVID_DEBUG_GET_STATISTICS
                if (NbFieldsSkipped == 0)
                {
                    VIDSPEED_Data_p->Statistics.SpeedSkipReturnNone ++;
                }
#endif /*STVID_DEBUG_GET_STATISTICS*/
            }

            /* Manage decode skips */
            if(VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat == 0)
            {
                VIDPROD_SkipData(VIDSPEED_Data_p->ProducerHandle, VIDPROD_SKIP_NO_PICTURE);
            }
            /* Display skip was not enough to keep the right speed ? Try to skip decode of some B pictures */
            else if((VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat > 0)
                    && (VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat <= VIDSPEED_LEVEL_SKIP_ON_HDD))
            {
                VIDPROD_SkipData(VIDSPEED_Data_p->ProducerHandle, VIDPROD_SKIP_ALL_B_PICTURES);
            }
            /* Decode skips are still not enough... Ask for HDD skips */
            else if(VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat > VIDSPEED_LEVEL_SKIP_ON_HDD)
            {
                /* Decode only I pictures */
                VIDPROD_SkipData(VIDSPEED_Data_p->ProducerHandle, VIDPROD_SKIP_ALL_P_PICTURES);

                if(VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat > (VIDSPEED_LEVEL_SKIP_ON_HDD + 1))
                {
                    U32 FieldsToSkip;
                    /* Skip fields and keep the SkipRepeat counter at a level to continue decode of only I pictures */
                    FieldsToSkip = (VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat - (VIDSPEED_LEVEL_SKIP_ON_HDD + 1));
                    VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat -= FieldsToSkip;
                    /* Prepare Data for drift event */
                    GetBitRateValue(SpeedHandle, &SpeedDriftThresholdData.BitRateValue);
                    SpeedDriftThresholdData.DriftTime = (FieldsToSkip * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / ((VIDSPEED_Data_p->MainParams.FrameRateValue * 2) / 1000);
                    /* Send drift event */
                    VIDSPEED_Data_p->DriftTime += SpeedDriftThresholdData.DriftTime;
                    /*STEVT_Notify(VIDSPEED_Data_p->EventsHandle, VIDSPEED_Data_p->RegisteredEventsID[VIDSPEED_SPEED_DRIFT_THRESHOLD_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&SpeedDriftThresholdData));*/
                    TraceDrifts(("DRIFT", SpeedDriftThresholdData.DriftTime));

                }
            }
        }
        TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    }

    return(ST_NO_ERROR);
} /* end of VIDSPEED_VsyncActionSpeed() function */

/*******************************************************************************
Name        : VIDSPEED_PostVsyncAction
Description : Function which has to be called every Vsync occurence. It computes the number of fileds to
              be skipped or repeated, Skipd or Repeats fields in display module in order to reach teh required speed.
Parameters  :
Assumptions : None
Limitations : None
Returns     : None
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_PostVsyncAction(const VIDSPEED_Handle_t SpeedHandle, BOOL IsReferenceDisplay)
{
    VIDSPEED_Data_t 	 *VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;
    UNUSED_PARAMETER(IsReferenceDisplay);
      if (VIDSPEED_Data_p->ValidityCheck != VIDSPEED_VALID_HANDLE)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* unlock semaphore protecting Trick mode private params */
    STOS_SemaphoreSignal(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
    /* Speed actions are done only on the reference display
    if (!IsReferenceDisplay)
    {
        return(ST_NO_ERROR);
    }
    *** Not needed for the moment because no action is done on Post VSync Event ***/
    return(ST_NO_ERROR);
} /* end of VIDSPEED_PostVsyncAction() function */

#if (defined(ST_7200) && defined(ST_smoothbackward)) || (!defined(ST_7200))
/*******************************************************************************
Name        : NoNextPictureToDisplay
Description : Call back of NO_NEXT_PICTURE_TO_DISPLAY evt. This event is notified
              from the display task context, so the Speed context is already
              protected (in VIDSPEED_VsyncActionSpeed).
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void NoNextPictureToDisplay(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VIDSPEED_Data_t *        VIDSPEED_Data_p;

    VIDSPEED_Handle_t SpeedHandle = (VIDSPEED_Handle_t) SubscriberData_p;
    /* Find structure */
    VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);
	UNUSED_PARAMETER(EventData_p);

    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);

    if(VIDSPEED_Data_p->Flags.SpeedControlEnabled)
    {
        if(VIDSPEED_Data_p->MainParams.VTGScanType == STGXOBJ_INTERLACED_SCAN)
        {
            /* The display is freezing, so to keep the speed
            correct, we need to skip 1 field */
            VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat += 1;
        }
        else
        {
            VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat += 2;
        }
        TraceSkipRepeat(("SK/RP", VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat));
    }
    STOS_SemaphoreSignal(VIDSPEED_Data_p->SpeedParamsSemaphore_p);


} /* end of NoNextPictureToDisplay() function */
#endif /*(defined(ST_7200) && defined(ST_smoothbackward)) || (!defined(ST_7200))*/
#ifdef STVID_TRICKMODE_BACKWARD
/*******************************************************************************
Name        : ComputeIOnlyBackwardDrift
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ComputeIOnlyBackwardDrift(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VIDSPEED_Handle_t             SpeedHandle;
    VIDSPEED_Data_t *             VIDSPEED_Data_p;
    U32                           TotalNbOfFields, NbFieldsToJump ;
    U32                           TimeToDisplayEachField;
    S32                           DriftTime;
    U32 AverageNbFieldsPerIPicture;
    U32 TotalVsyncNeededToDisplayTheBuffer;
    U32 MaxTreatmentTime;

    SpeedHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Handle_t, SubscriberData_p);
    VIDSPEED_Data_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Data_t *, SpeedHandle);

    /* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);


    DriftTime = 0;
    /* =================================================================== */
    /* Compute the jump to perform */
    /* to be use that at least one buffer has been parsed since we changed speed */


    if (( VIDSPEED_Data_p->BackwardIOnly.IsBufferParsed ) && (VIDSPEED_Data_p->BackwardIOnly.IsDisplayedBufferChanged))
    {
        TotalVsyncNeededToDisplayTheBuffer = ((VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields + VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields) * 100)
                 / (-VIDSPEED_Data_p->MainParams.Speed);
        if(TotalVsyncNeededToDisplayTheBuffer < VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields)
        {
            TotalVsyncNeededToDisplayTheBuffer = VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields;
        }
        if ((VIDSPEED_Data_p->BackwardIOnly.VSyncUsed ) > ((TotalVsyncNeededToDisplayTheBuffer ) ))
        {
            NbFieldsToJump = ((VIDSPEED_Data_p->BackwardIOnly.VSyncUsed) - TotalVsyncNeededToDisplayTheBuffer ) * (-VIDSPEED_Data_p->MainParams.Speed) /100;
#ifdef SPEED_DEBUG
            TraceBuffer(("display delay %d, used %d, new used %d,  needed %d \r\n", NbFieldsToJump, VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer,
                    VIDSPEED_Data_p->BackwardIOnly.VSyncUsed, TotalVsyncNeededToDisplayTheBuffer));
#endif /* SPEED_DEBUG */
            /* Frame rate value is 25000, 30000 ..., that's why we divide by 1000 */
            DriftTime = (NbFieldsToJump * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / ((VIDSPEED_Data_p->MainParams.FrameRateValue ) / 1000);
        }

    }

    VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInParsedBuffer = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->NbIPicturesInBuffer;
    VIDSPEED_Data_p->BackwardIOnly.NbNotIPicturesInParsedBuffer = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->NbNotIPicturesInBuffer;
    VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->TotalNbIFields;
    VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->TotalNbNotIFields;
    VIDSPEED_Data_p->BackwardIOnly.BufferTreatmentTime = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->BufferTreatmentTime;
    VIDSPEED_Data_p->BackwardIOnly.BufferDecodeTime = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->BufferDecodeTime;
    VIDSPEED_Data_p->BackwardIOnly.TimeToDecodeIPicture = ((VIDPROD_BitBUfferInfos_t *)EventData_p)->TimeToDecodeIPicture;

    VIDSPEED_Data_p->BackwardIOnly.IsBufferParsed = TRUE;

    TotalNbOfFields = VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields + VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields;

    if ((TotalNbOfFields < (((-VIDSPEED_Data_p->MainParams.Speed)/100) * VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields)) && (VIDSPEED_Data_p->BackwardIOnly.IsDisplayedBufferChanged))
    {
        NbFieldsToJump = VIDSPEED_Data_p->BackwardIOnly.VSyncUsed * ((-VIDSPEED_Data_p->MainParams.Speed) /100);
        if (NbFieldsToJump > TotalNbOfFields)
        {
            NbFieldsToJump = NbFieldsToJump - TotalNbOfFields;
            /* Frame rate value is 25000, 30000 ..., that's why we divide by 1000 */
            DriftTime = (NbFieldsToJump * STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) / ((VIDSPEED_Data_p->MainParams.FrameRateValue ) / 1000);
#ifdef SPEED_DEBUG
            TraceBuffer(( "high speed jump %d, total fields %d, total I %d  \r\n", NbFieldsToJump,  VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields, VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields));
#endif /* SPEED_DEBUG */
        }
    }
    if (DriftTime != 0)
    {
        if ( VIDSPEED_Data_p->MainParams.FrameRateValue <= 30000)
        {
            DriftTime = DriftTime / 2;
        }

        VIDSPEED_Data_p->BackwardIOnly.PreviousSkip = DriftTime ;


        VIDPROD_UpdateDriftTime(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle, DriftTime);
    }
    else if (VIDSPEED_Data_p->BackwardIOnly.DoesUnderflowEventOccur)
    {
        VIDPROD_UpdateDriftTime(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle, VIDSPEED_Data_p->BackwardIOnly.PreviousSkip);
    }
    VIDSPEED_Data_p->BackwardIOnly.IsDisplayedBufferChanged = FALSE;
    VIDSPEED_Data_p->BackwardIOnly.DoesUnderflowEventOccur = FALSE;

    /*VIDSPEED_Data_p->BackwardIOnly.NbVsyncUsedToDisplayTheBuffer = 0;*/
    /* ====================================================================== */
    /* Compute the number of vsync to display each I picture */
    /* The time to display each I picture is equal to the buffer treatment time divided by the number of I in the buffer, in units of time. */
    /* To translate this number in multiple of Vsync to display each picture, this number has to be multiplied by twice the frame rate */
    MaxTreatmentTime = ( 1000000 * (VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields + VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields)) / ( 2 * (VIDSPEED_Data_p->MainParams.FrameRateValue/1000));

    if ((VIDSPEED_Data_p->BackwardIOnly.BufferDecodeTime <= MaxTreatmentTime) && (VIDSPEED_Data_p->BackwardIOnly.BufferDecodeTime > 0))
    {
        TimeToDisplayEachField = VIDSPEED_Data_p->BackwardIOnly.BufferTreatmentTime / (VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields );
        TimeToDisplayEachField = TimeToDisplayEachField / 1000;
        TimeToDisplayEachField = TimeToDisplayEachField * 2 * VIDSPEED_Data_p->MainParams.FrameRateValue / 1000;
        VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField = TimeToDisplayEachField / 1000;
    }
    else
    {
        VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField = 2;

    }
#ifdef SPEED_DEBUG
    TraceBuffer(("Buffer treatment %d, FR %d, Nb I %d, nb not I %d, display %d, max treatment %d \r\n", VIDSPEED_Data_p->BackwardIOnly.BufferTreatmentTime,
                VIDSPEED_Data_p->MainParams.FrameRateValue, VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields, VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields,
                VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField, MaxTreatmentTime));
#endif /* SPEED_DEBUG */
    AverageNbFieldsPerIPicture = VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields / VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInParsedBuffer;

    if (TotalNbOfFields > (((-VIDSPEED_Data_p->MainParams.Speed) * VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields) /100))
    {
        TimeToDisplayEachField = (TotalNbOfFields * 100) / ((-VIDSPEED_Data_p->MainParams.Speed) * VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields);

        if (( TimeToDisplayEachField < VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField))
        {
            VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField = (VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField
                    + VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField) /2;
        }
        else
        {
            VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField = (VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField + TimeToDisplayEachField)/2;
        }
#ifdef SPEED_DEBUG
        TraceBuffer(("Small speed, display for %d, local %d, global %d \r\n", VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField, TimeToDisplayEachField, VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField));
#endif
    }
    else
    {
        VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField = (VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField + VIDSPEED_Data_p->BackwardIOnly.TimeToDisplayEachField) /2;
#ifdef SPEED_DEBUG
        TraceBuffer(("High speed, display for %d, Buffer treatment %d, max %d  \r\n", VIDSPEED_Data_p->BackwardIOnly.NewNbVsyncToDisplayEachField, VIDSPEED_Data_p->BackwardIOnly.BufferTreatmentTime, MaxTreatmentTime));
        TraceBuffer(("Nb I pictures %d, not I %d \r\n", VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields, VIDSPEED_Data_p->BackwardIOnly.TotalNbNotIFields));
#endif
    }
}
/*******************************************************************************
Name        : ReceiveFirstDecodedPictureInBitBuffer
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void ReceiveFirstDecodedPictureInBitBuffer(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VIDSPEED_Handle_t             SpeedHandle;
    VIDSPEED_Data_t *             VIDSPEED_Data_p;

    SpeedHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Handle_t, SubscriberData_p);
    VIDSPEED_Data_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Data_t *, SpeedHandle);

    /* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    if ( !VIDSPEED_Data_p->BackwardIOnly.IsBufferReadyForDisplay)
    {
        VIDSPEED_Data_p->BackwardIOnly.NbIPicturesInDisplayedBuffer = VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields;
    }
    else
    {
        VIDSPEED_Data_p->BackwardIOnly.IsNewBufferStartedDecoded = TRUE;
    }
    VIDSPEED_Data_p->BackwardIOnly.IsBufferReadyForDisplay = TRUE;
    VIDSPEED_Data_p->BackwardIOnly.NbIPicturesForNextDisplayedBuffer = VIDSPEED_Data_p->BackwardIOnly.TotalNbIFields;

}
/*******************************************************************************
Name        : UnderflowEventSent
Description :
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void UnderflowEventSent(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VIDSPEED_Handle_t             SpeedHandle;
    VIDSPEED_Data_t *             VIDSPEED_Data_p;

    SpeedHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Handle_t, SubscriberData_p);
    VIDSPEED_Data_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Data_t *, SpeedHandle);

    /* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    VIDSPEED_Data_p->BackwardIOnly.DoesUnderflowEventOccur = TRUE;
}
#endif /* STVID_TRICKMODE_BACKWARD */
/*******************************************************************************
Name        : VTGScanTypeChanged
Description : Call back of VIDDISP_VTG_SCANTYPE_CHANGE evt. This event is notified
              from the VTG interrupt context.
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void VTGScanTypeChanged(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VIDSPEED_Data_t *        VIDSPEED_Data_p;

    VIDSPEED_Handle_t SpeedHandle = (VIDSPEED_Handle_t) SubscriberData_p;
    /* Find structure */
    VIDSPEED_Data_p = (VIDSPEED_Data_t *) SpeedHandle;

	/* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
	UNUSED_PARAMETER(Event);

    VIDSPEED_Data_p->MainParams.VTGScanType = *((STGXOBJ_ScanType_t*) EventData_p);
} /* end of VTGScanTypeChanged() function */

/*******************************************************************************
Name        : ReadyToDecodeNewPictureSpeed
Description : Function to subscribe the VIDDEC_READY_TO_DECODE_NEW_PICTURE_EVT event
Parameters  : PictureInfos_p, temporal_reference, vbv_delay.
              According to the NbOfFieldsToSkipOrToRepeat counter value, the decision-making of
              skipping or decoding it is made in this call-back.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ReadyToDecodeNewPictureSpeed(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
	VIDSPEED_Handle_t       	  SpeedHandle;
    STVID_PictureInfos_t *        PictureInfos_p;
    VIDSPEED_Data_t *             VIDSPEED_Data_p;
    BOOL                          UnderflowOccured;
    STVID_SpeedDriftThreshold_t   SpeedDriftThresholdData;
    ST_ErrorCode_t       		  ErrorCode;

    SpeedHandle = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Handle_t, SubscriberData_p);
    VIDSPEED_Data_p = CAST_CONST_HANDLE_TO_DATA_TYPE(VIDSPEED_Data_t *, SpeedHandle);

    /* unused parameters */
	UNUSED_PARAMETER(Reason);
	UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    /* Nothing to do if speed is 100 */
    if (VIDSPEED_Data_p->MainParams.Speed == 100)
    {
        return;
    }

    PictureInfos_p = ((VIDPROD_NewPictureDecodeOrSkip_t *)EventData_p)->PictureInfos_p;
    UnderflowOccured = ((VIDPROD_NewPictureDecodeOrSkip_t *)EventData_p)->UnderflowOccured;
	ErrorCode = ST_NO_ERROR;


    if ((PictureInfos_p == NULL) ||
        (!VIDSPEED_Data_p->Flags.DisplayManagedBySpeed)
        )
    {
        VIDSPEED_Data_p->Flags.SpeedReinitNeeded = TRUE;
        return;
    }
    if (VIDSPEED_Data_p->Flags.SpeedReinitNeeded)
    {
        VIDSPEED_Data_p->MainParams.Speed = VIDSPEED_Data_p->MainParams.LastSpeed;
        VIDSPEED_Data_p->MainParams.RequiredSpeed = VIDSPEED_Data_p->MainParams.LastSpeed;
        SpeedInitialize(SpeedHandle);

        VIDSPEED_Data_p->Flags.SpeedReinitNeeded = FALSE;
    }
    /* This check is to avoid having NbFieldsIfPosToSkipIfNegToRepeat already big when the first decoded picture arrived after a */
    /* speed change. It may be long since first RAP is found and NbFieldsIfPosToSkipIfNegToRepeat is incremented at each Vsync. */
    /* Without this check we can be in I only at speed 150, because we are already above the threshold for skipping all P. */
    if (VIDSPEED_Data_p->WaitFirstDecode)
    {
        VIDSPEED_Data_p->NbFieldsIfPosToSkipIfNegToRepeat = 0;
        VIDSPEED_Data_p->AverageFieldsNb = 0;
        VIDSPEED_Data_p->ZeroCounter = 0;
        VIDSPEED_Data_p->WaitFirstDecode = FALSE;
    }

    /* Lock semaphore protecting Trick mode private params */
    STOS_SemaphoreWait(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
    if((VIDSPEED_Data_p->MainParams.Speed >= 0) && (VIDSPEED_Data_p->Flags.SpeedControlEnabled))
    {
		/* Update the FrameRate value */
        if ((PictureInfos_p->VideoParams.FrameRate > 60000) || (PictureInfos_p->VideoParams.FrameRate == 0))
        {
            VIDSPEED_Data_p->MainParams.FrameRateValue = VIDSPEED_DEFAULT_FRAME_RATE * 1000 ; /* Hz -> mHz */
        }
        else
        {
            VIDSPEED_Data_p->MainParams.FrameRateValue = PictureInfos_p->VideoParams.FrameRate;
        }

        /* TODO_OB : This event notification is not yet important, until we need
           to managed bwk->fwd switch and beeing picture accurate */
        if (VIDSPEED_Data_p->DisplayParams.DisplayStart != VIDDISP_DISPLAY_START_AS_AVAILABLE)
        {
            VIDSPEED_Data_p->DisplayParams.DisplayStart = VIDDISP_DISPLAY_START_AS_AVAILABLE;
            /* Notify event so that API level can choose to change display settings or not. */
            STEVT_Notify(VIDSPEED_Data_p->EventsHandle, VIDSPEED_Data_p->RegisteredEventsID[VIDSPEED_DISPLAY_PARAMS_CHANGE_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDSPEED_Data_p->DisplayParams));
        }
        if (UnderflowOccured)
        {
            GetBitRateValue(SpeedHandle, &SpeedDriftThresholdData.BitRateValue);
            SpeedDriftThresholdData.DriftTime = VIDSPEED_Data_p->DriftTime;
            VIDSPEED_Data_p->DriftTime = 0;
            /* Ask for a HDD skip */
            STEVT_Notify(VIDSPEED_Data_p->EventsHandle, VIDSPEED_Data_p->RegisteredEventsID[VIDSPEED_SPEED_DRIFT_THRESHOLD_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST (&SpeedDriftThresholdData));
        }

    }

    /* Un-Lock semaphore protecting Trick mode private params */
    STOS_SemaphoreSignal(VIDSPEED_Data_p->SpeedParamsSemaphore_p);
} /* end of ReadyToDecodeNewPictureTrickMode() function */

/*******************************************************************************
Name        : GetBitRateValue
Description :
Parameters  :
Assumptions :
Limitations :
Returns     : ST_NO_ERROR, ST_ERROR_BAD_PARAMETER
*******************************************************************************/
static ST_ErrorCode_t GetBitRateValue(const VIDSPEED_Handle_t SpeedHandle, U32 * const BitRateValue_p)
{
    VIDSPEED_Data_t  * VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;
    ST_ErrorCode_t     ErrorCode;

    /* Get Bit Rate Value */
    *BitRateValue_p = VIDSPEED_Data_p->MainParams.BitRateValue;
    /*ErrorCode = VIDPROD_GetBitRateValue(VIDSPEED_Data_p->ProducerHandle, BitRateValue_p);*/
    ErrorCode = VIDPROD_GetBitRateValue(((VIDPROD_Properties_t *)(VIDSPEED_Data_p->ProducerHandle))->InternalProducerHandle, BitRateValue_p);

    if (ErrorCode != ST_NO_ERROR)
    {
        *BitRateValue_p = VIDSPEED_Data_p->MainParams.BitRateValue;
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return (ErrorCode);
    }

    VIDSPEED_Data_p->MainParams.BitRateValue = *BitRateValue_p;
    return (ST_NO_ERROR);
} /* end of GetBitRateValue() function */

#ifdef STVID_DEBUG_GET_STATISTICS
/*******************************************************************************
Name        : VIDSPEED_GetStatistics
Description : Get the statistics of the speed module.
Parameters  : .
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_GetStatistics(const VIDSPEED_Handle_t SpeedHandle, STVID_Statistics_t * const Statistics_p)
{
    VIDSPEED_Data_t  * VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;

    if (Statistics_p == NULL)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
    Statistics_p->SpeedSkipReturnNone   = VIDSPEED_Data_p->Statistics.SpeedSkipReturnNone;
    Statistics_p->SpeedRepeatReturnNone = VIDSPEED_Data_p->Statistics.SpeedRepeatReturnNone;

    return(ST_NO_ERROR);
} /* End of VIDSPEED_GetStatistics() function */
/*******************************************************************************
Name        : VIDSPEED_ResetStatistics
Description : Reset the statistics of the speed module.
Parameters  : .
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if Statistics_p == NULL
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_ResetStatistics(const VIDSPEED_Handle_t SpeedHandle, const STVID_Statistics_t * const Statistics_p)
{
    VIDSPEED_Data_t  * VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;

    if (Statistics_p == NULL)
    {
#ifdef SPEED_DEBUG
		SpeedError();
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset parameters that are said to be reset (giving value != 0) */
    VIDSPEED_Data_p->Statistics.SpeedSkipReturnNone = 0;
    VIDSPEED_Data_p->Statistics.SpeedRepeatReturnNone = 0;

    return(ST_NO_ERROR);
} /* End of VIDSPEED_ResetStatistics() function */
#endif /* STVID_DEBUG_GET_STATISTICS */
/*******************************************************************************
Name        : VIDSPEED_DisplayManagedBySpeed
Description :
Parameters  : .
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
*******************************************************************************/
ST_ErrorCode_t VIDSPEED_DisplayManagedBySpeed(const VIDSPEED_Handle_t SpeedHandle, const BOOL DisplayManagedBySpeed)
{
    VIDSPEED_Data_t  * VIDSPEED_Data_p = (VIDSPEED_Data_t *)SpeedHandle;
    VIDSPEED_Data_p->Flags.DisplayManagedBySpeed = DisplayManagedBySpeed;
    if (!DisplayManagedBySpeed)
    {
        VIDSPEED_Data_p->MainParams.LastSpeed = VIDSPEED_Data_p->MainParams.Speed;
        InitSpeedStructure(SpeedHandle);
        VIDSPEED_Data_p->MainParams.Speed = 100;
        VIDSPEED_Data_p->MainParams.RequiredSpeed = 100;
        SpeedInitialize(SpeedHandle);

    }
    return(ST_NO_ERROR);
}
#ifdef SPEED_DEBUG
/*******************************************************************************
Name        : SpeedError
Description : Function to help in debug : put a breakpoint here
              and you will know by the callers where it has been called
Parameters  :
Assumptions :
Limitations :
Returns     :
*******************************************************************************/
static void SpeedErrorLine(U32 Line)
{
    U8 Error;
    Error++;
    TraceBuffer(("ERROR - %d - ", Line));       /* Informs user about Error line */
} /* end of SpeedErrorLine() function */
#endif /* SPEED_DEBUG */


