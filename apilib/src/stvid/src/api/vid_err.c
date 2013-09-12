/*******************************************************************************

File name   : vid_err.c

Description : Video error recovery feature source file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
 6 Oct 2000        Created                                           HG
 8 Oct 2001        Modification about the entrance in the task       AN
22 Jul 2003        Added error recovery mode HIGH                    HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */

/* Define below to suppress automatic stop/start when nothing in real time. Should not be defined by default ! */
/*#define STVID_NO_AUTO_STOP_START 1*/

/* Define to add debug info and traces */
/*#define TRACE_ERROR_RECOVERY*/

#ifdef TRACE_ERROR_RECOVERY
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */
#endif /* TRACE_ERROR_RECOVERY */

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdlib.h>
#include <string.h>
#endif

#include "stddefs.h"
#include "stos.h"

#include "stvid.h"
#include "api.h"
#include "vid_com.h"
#include "vid_err.h"
#include "producer.h"
#include "sttbx.h"
#include "stevt.h"
#include "stos.h"

#ifndef STVID_STVAPI_ARCHITECTURE
#include "stvtg.h"
#endif

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
/* ------------                 TIME-OUT constants               ------------ */
/* -------------------------------------------------------------------------- */

/* Max number of VSync between a start and a first picture data */
/* detected by the decode parse.                                */
/* i.e. 1800/60 = 30s                                           */
#define MAX_VSYNC_UNTIL_FIRST_PICTURE_DETECTED          (1800)

/* Max number of VSync between the first picture data are       */
/* parsed and its decode launch. This is equal to vbv_buffer    */
/* size to feed before launching the first decode (vbv_delay).  */
/* Let's make the assumption the min bit rate is 24kb/s.        */
/* Take a time of (max vbv_buffer_size/min bit rate)            */
/*              = 112*2048*8/(24*1024) = 74.7s                  */
/* i.e. 74.7 * 60 = 4480 VSyncs                                 */
#define MAX_VSYNC_UNTIL_FIRST_PICTURE_DECODE_LAUNCHED   (4500)

/* Max number of VSync between two picture decode               */
/* e.g. (at speed 100 and for NTSC) : 15/60 = 250ms             */
#define MAX_VSYNC_BETWEEN_TWO_PICTURE_DECODE            (15)



/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */

/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */

#ifdef ST_producer
static void DecodeOrSkipLaunchedErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
static void DecodeRestartedErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* ST_producer */

#ifdef ST_display
static void FieldsRepeatedErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* ST_display */

static ST_ErrorCode_t StartErrorRecoveryTask(VideoDevice_t * const Device_p, const U8 DecoderNumber);
static ST_ErrorCode_t StopErrorRecoveryTask (VideoDevice_t * const Device_p);

#ifndef STVID_STVAPI_ARCHITECTURE
static void VSyncErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p);
#endif /* end of ndef STVID_STVAPI_ARCHITECTURE */

static void ErrorRecoveryTaskFunc(VideoDevice_t * const Device_p);


/* External Function prototypes ---------------------------------------------- */

/* API Functions ------------------------------------------------------------ */


/*
--------------------------------------------------------------------------------
Retrieves the current error recovery mode for the specified video driver.
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_GetErrorRecoveryMode(const STVID_Handle_t VideoHandle, STVID_ErrorRecoveryMode_t * const Mode_p)
{
    VideoDevice_t * Device_p;

    /* Exit now if parameters are bad */
    if (Mode_p == NULL)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid, point onto the structure. */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error recovery mode is %d.", *Mode_p));

    *Mode_p = Device_p->DeviceData_p->ErrorRecoveryMode;

    return(ST_NO_ERROR);
} /* End of STVID_GetErrorRecoveryMode() function */


/*
--------------------------------------------------------------------------------
Defines the behavior of the video driver regarding error recovery.
--------------------------------------------------------------------------------
*/
ST_ErrorCode_t STVID_SetErrorRecoveryMode(const STVID_Handle_t VideoHandle, const STVID_ErrorRecoveryMode_t Mode)
{
    VideoDevice_t * Device_p;
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if (!(IsValidHandle(VideoHandle)))
    {
        /* Handle given is invalid */
        return(ST_ERROR_INVALID_HANDLE);
    }

    /* Handle is valid, point onto the structure. */
    Device_p = ((VideoUnit_t *)VideoHandle)->Device_p;

    switch(Mode)
    {
        case STVID_ERROR_RECOVERY_FULL :
        case STVID_ERROR_RECOVERY_HIGH :
        case STVID_ERROR_RECOVERY_PARTIAL :
        case STVID_ERROR_RECOVERY_NONE :
            break;

        default :
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STVID_SetErrorRecoveryMode : Invalid mode"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
            break;
    }

#ifdef ST_producer
    if ((ErrorCode == ST_NO_ERROR) && (Device_p->DeviceData_p->ProducerHandleIsValid))
    {
        /* If a decoder exist then inform modules of the new error recovery mode. */
        ErrorCode = VIDPROD_SetErrorRecoveryMode(Device_p->DeviceData_p->ProducerHandle, Mode);

        if (ErrorCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "VIDPROD_SetErrorRecoveryMode failed"));
            ErrorCode = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error recovery mode set to %d.", Mode));
        }
    }
#else /* ST_producer */
    /* Nothing to do: error recovery mode has no meaning but is not an error */
#endif /* not ST_producer */

    if (ErrorCode == ST_NO_ERROR)
    {
        Device_p->DeviceData_p->ErrorRecoveryMode = Mode;
    }

    return(ErrorCode);
} /* End of STVID_SetErrorRecoveryMode() function */



/* Exported functions ------------------------------------------------------- */

/*******************************************************************************
Name        : stvid_InitErrorRecovery
Description : Launch error recovery feature
Parameters  : Initialised device
Assumptions : Device_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if failed
*******************************************************************************/
ST_ErrorCode_t stvid_InitErrorRecovery(VideoDevice_t * const Device_p,
        const STVID_ErrorRecoveryMode_t Mode, const ST_DeviceName_t VideoName,
        const U8 DecoderNumber)
{
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    /* Basic test of device pointer.    */
    if ( (Device_p == NULL) || (Device_p->DeviceData_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Prepare subscribe parameters for all subscribes */
    STEVT_SubscribeParams.SubscriberData_p = (void *) (Device_p);

    Device_p->DeviceData_p->CurrentAbsoluteSpeed        = 100;
    Device_p->DeviceData_p->ErrorRecoveryMode           = Mode;
    Device_p->DeviceData_p->LiveResetState              = WAIT_FOR_VIDEO_START;
    (Device_p->DeviceData_p->ErrorRecoveryVTGName)[0]   = '\0'; /* No VTG connected yet */
    Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = 0;
    Device_p->DeviceData_p->IsErrorRecoveryInRealTime   = FALSE;
    Device_p->DeviceData_p->ErrorRecoveryTask.IsRunning  = FALSE;

#ifdef ST_producer
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        ErrorCode = VIDPROD_SetErrorRecoveryMode(Device_p->DeviceData_p->ProducerHandle, Mode);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: function call failed, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery init: could not set the Error Recovery Mode !"));
            return(ErrorCode);
        }
    }

    /* Subscribe to ready to decode new picture EVT */
    if (Device_p->DeviceData_p->ProducerHandleIsValid)
    {
        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) DecodeRestartedErrorRecovery;
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_RESTARTED_EVT, &STEVT_SubscribeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: subscribe failed, undo initialisations done */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery init: could not subscribe to VIDPROD_RESTARTED_EVT !"));

            return(ST_ERROR_BAD_PARAMETER);
        }

        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) DecodeOrSkipLaunchedErrorRecovery;
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_DECODE_LAUNCHED_EVT, &STEVT_SubscribeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: subscribe failed, undo initialisations done */
            STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_RESTARTED_EVT);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery init: could not subscribe to VIDPROD_DECODE_LAUNCHED_EVT !"));

            return(ST_ERROR_BAD_PARAMETER);
        }

        /* Subscribe with the same callback function.   */
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_SKIP_LAUNCHED_EVT, &STEVT_SubscribeParams);
        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: subscribe failed, undo initialisations done */
            STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_RESTARTED_EVT);
            STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_DECODE_LAUNCHED_EVT);

            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery init: could not subscribe to VIDPROD_SKIP_LAUNCHED_EVT !"));

            return(ST_ERROR_BAD_PARAMETER);
        }
    }

#endif /* ST_producer */

#ifdef ST_display
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) FieldsRepeatedErrorRecovery;
    ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDDISP_FIELDS_REPEATED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
#ifdef ST_producer
        /* Error: subscribe failed, undo initialisations done */
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_RESTARTED_EVT);
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_DECODE_LAUNCHED_EVT);
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_SKIP_LAUNCHED_EVT);
#endif /* ST_producer */

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery init: could not subscribe to VIDDISP_FIELDS_REPEATED_EVT !"));

        return(ST_ERROR_BAD_PARAMETER);
    }
#endif /* ST_display */

#ifdef ST_avsync
    STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) FieldsRepeatedErrorRecovery;
    ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT, &STEVT_SubscribeParams);
    if (ErrorCode != ST_NO_ERROR)
    {
#ifdef ST_producer
        /* Error: subscribe failed, undo initialisations done */
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_RESTARTED_EVT);
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_DECODE_LAUNCHED_EVT);
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_SKIP_LAUNCHED_EVT);
#endif /* ST_producer */
#ifdef ST_display
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDDISP_FIELDS_REPEATED_EVT);
#endif /* ST_display */

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery init: could not subscribe to VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT !"));

        return(ST_ERROR_BAD_PARAMETER);
    }
#endif /* ST_avsync */

    /* Init the semaphores used throughout Signaling semaphores*/
    Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p = STOS_SemaphoreCreateFifo(Device_p->DeviceData_p->CPUPartition_p,0);
    Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p = STOS_SemaphoreCreatePriority(Device_p->DeviceData_p->CPUPartition_p,1);

    /* Start error recovery task */
    ErrorCode = StartErrorRecoveryTask(Device_p, DecoderNumber);
    if ( (ErrorCode != ST_NO_ERROR) &&
         (ErrorCode != ST_ERROR_ALREADY_INITIALIZED) )
    {
        /* Error: task creation failed, undo initialisations done */
         STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p,Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p);
         STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p,Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

#ifdef ST_producer
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_RESTARTED_EVT);
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_DECODE_LAUNCHED_EVT);
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDPROD_SKIP_LAUNCHED_EVT);
#endif /* ST_producer */
#ifdef ST_display
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDDISP_FIELDS_REPEATED_EVT);
#endif /* ST_display */
#ifdef ST_avsync
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VideoName, VIDAVSYNC_NEXT_PICTURE_DISPLAY_DELAYED_EVT);
#endif /* ST_display */

        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery cannot start task ! (%s)", (ErrorCode == ST_ERROR_ALREADY_INITIALIZED?"already exists":"failed")));
    }

    return(ErrorCode);
} /* End of stvid_InitErrorRecovery() function */


#ifndef STVID_STVAPI_ARCHITECTURE
/*******************************************************************************
Name        : stvid_NewVTGErrorRecovery
Description : To be called when a new VTG is connected
Parameters  : Initialised device, VTG name
Assumptions : Device_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if failed
*******************************************************************************/
ST_ErrorCode_t stvid_NewVTGErrorRecovery(VideoDevice_t * const Device_p, const ST_DeviceName_t VTGName)
{
    STEVT_DeviceSubscribeParams_t STEVT_SubscribeParams;
    ST_ErrorCode_t ErrorCode;

    if ( (Device_p == NULL) || (Device_p->DeviceData_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    if ((Device_p->DeviceData_p->ErrorRecoveryVTGName)[0] != '\0')
    {
        /* There was a VTG previously subscribed, un-subscribe it */
        STEVT_UnsubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, Device_p->DeviceData_p->ErrorRecoveryVTGName, STVTG_VSYNC_EVT);
    }

    /* Subscribe to new VTG name if required, nothing to do otherwise. */
    if ((VTGName)[0] != '\0')
    {
        /* Subscribe to VSYNC EVT */
        STEVT_SubscribeParams.NotifyCallback = (STEVT_DeviceCallbackProc_t) VSyncErrorRecovery;
        STEVT_SubscribeParams.SubscriberData_p = (void *) (Device_p);
        ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, VTGName, STVTG_VSYNC_EVT, &STEVT_SubscribeParams);

        if (ErrorCode != ST_NO_ERROR)
        {
            /* Error: subscribe failed */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery: could not subscribe to STVTG_VSYNC_EVT !"));
            /* Try to re-subscribe to old VSync, to avoid being blocked ? !!! */
            if ((Device_p->DeviceData_p->ErrorRecoveryVTGName)[0] != '\0')
            {
                ErrorCode = STEVT_SubscribeDeviceEvent(Device_p->DeviceData_p->EventsHandle, Device_p->DeviceData_p->ErrorRecoveryVTGName, STVTG_VSYNC_EVT, &STEVT_SubscribeParams);
                if (ErrorCode != ST_NO_ERROR)
                {
                    (Device_p->DeviceData_p->ErrorRecoveryVTGName)[0] = '\0'; /* No VTG connected */
                }
            }
            return(ST_ERROR_BAD_PARAMETER);
        }
    }

    strcpy(Device_p->DeviceData_p->ErrorRecoveryVTGName, VTGName);

    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    return(ST_NO_ERROR);
} /* End of stvid_NewVTGErrorRecovery() function */
#endif /* end of ndef STVID_STVAPI_ARCHITECTURE */

/*******************************************************************************
Name        : stvid_SetSpeedErrorRecovery
Description : To be called in STVID_SetSpeed() to inform the current speed
Parameters  : Initialised device
              current speed
Assumptions : Device_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER is any input parameter is bad
*******************************************************************************/
ST_ErrorCode_t stvid_SetSpeedErrorRecovery(VideoDevice_t * const Device_p, const S32 Speed)
{
    if ( (Device_p == NULL) || (Device_p->DeviceData_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    /* Save the absolute current trickmode speed.   */
    if (Speed < 0)
    {
        Device_p->DeviceData_p->CurrentAbsoluteSpeed = - (Speed);
    }
    else
    {
        Device_p->DeviceData_p->CurrentAbsoluteSpeed = Speed;
    }

    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    return(ST_NO_ERROR);

} /* end of stvid_SetSpeedErrorRecovery() function */


/*******************************************************************************
Name        : stvid_StartVideoErrorRecovery
Description : To be called in STVID_Start() to inform error  recovery feature
Parameters  : Initialised device
              BOOL IsStartInRealTime.
Assumptions : Device_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER is any input parameter is bad
*******************************************************************************/
ST_ErrorCode_t stvid_StartVideoErrorRecovery(VideoDevice_t * const Device_p, BOOL IsStartInRealTime)
{
    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL))
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    /* Reset the live reset state.  */
    Device_p->DeviceData_p->LiveResetState = WAIT_FOR_VIDEO_START;
    /* No time out is defined for this state (no need). */
    Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = 0;
    /* Save the mode of start Real time or not.         */
    Device_p->DeviceData_p->IsErrorRecoveryInRealTime = IsStartInRealTime;

#ifdef TRACE_UART
    TraceBuffer(("\r\n   (Start) state WAIT_FOR_VIDEO_START"));
#endif /* TRACE_UART */

    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    return(ST_NO_ERROR);
} /* End of stvid_StartVideoErrorRecovery() function */



/*******************************************************************************
Name        : stvid_TermErrorRecovery
Description : Terminate error recovery feature
Parameters  : Initialised device
Assumptions : Device_p is not NULL
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER is any input parameter is bad
*******************************************************************************/
ST_ErrorCode_t stvid_TermErrorRecovery(VideoDevice_t * const Device_p)
{
    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL) )
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Reset automaton's status.    */
    Device_p->DeviceData_p->LiveResetState = WAIT_FOR_VIDEO_START;
    StopErrorRecoveryTask(Device_p);
    STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p,Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p);
    STOS_SemaphoreDelete(Device_p->DeviceData_p->CPUPartition_p,Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    return(ST_NO_ERROR);
} /* End of stvid_TermErrorRecovery() function */




/* Private functions -------------------------------------------------------- */


/*******************************************************************************
Name        : ErrorRecoveryTaskFunc
Description : Function of the error recovery mechanism
              Set Device_p->DeviceData_p->ErrorRecoveryTask.ToBeDeleted to end
              the task.
Parameters  : Opened decoder
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void ErrorRecoveryTaskFunc(VideoDevice_t * const VideoDevice_p)
{
    STVID_Freeze_t Freeze;
    VideoDevice_t*    Device_p;
#ifdef ST_producer
    VIDPROD_DecoderOperatingMode_t  ProducerOperatingMode;
    ST_ErrorCode_t ErrorCode;
#endif /* ST_producer */
#ifdef STVID_DEBUG_GET_STATISTICS
    LiveResetState_t LiveResetState;
#endif /* STVID_DEBUG_GET_STATISTICS */

    STOS_TaskEnter(NULL);

    Device_p = VideoDevice_p;

    do
    { /* While not ToBeDeleted */

        /* Check if an order is available, but don't wait too long without looping */
#ifndef STVID_STVAPI_ARCHITECTURE
        semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p);
#else
        Device_p->DeviceData_p->ErrorRecoveryTaskMaxWaitOrderTime = time_plus(time_now(), STVID_MAX_VSYNC_DURATION);
        semaphore_wait_timeout(Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p,
          &(Device_p->DeviceData_p->ErrorRecoveryTaskMaxWaitOrderTime));
#endif

        stvid_EnterCriticalSection(Device_p);
        if(Device_p->DeviceData_p->Status == VIDEO_STATUS_RUNNING)
        {
            /* Not needed, double-ckeck counter value */
            if (Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset == 0)
            {
                /* ******************** Time-out detected ! ******************* */

#ifdef TRACE_UART
                TraceBuffer(("\r\n\r\n!!! TIME-OUT detected  !!!"));
#endif /* TRACE_UART */

                semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

#ifdef STVID_DEBUG_GET_STATISTICS
                /* Store the LiveReset State */
                LiveResetState = Device_p->DeviceData_p->LiveResetState;
#endif /* STVID_DEBUG_GET_STATISTICS */

                /* Reset the state of the live reset automaton.     */
                Device_p->DeviceData_p->LiveResetState = WAIT_FOR_VIDEO_START;
                /* No time out is defined for this state (no need). */
/*                Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = 0;*/

                semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

#ifdef TRACE_UART
                TraceBuffer(("\r\n   (Task ) state WAIT_FOR_VIDEO_START  (VSyncBeforeReset : %d)"
                        , Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset));
#endif /* TRACE_UART */

#ifdef ST_producer
                ErrorCode = VIDPROD_GetDecoderSpecialOperatingMode (Device_p->DeviceData_p->ProducerHandle, &ProducerOperatingMode);
                if ( (ProducerOperatingMode.IsLowDelay) && (ErrorCode == ST_NO_ERROR) )
                {
                    /* The current decoded stream include  a "low_delay" flag. In this case, don't apply    */
                    /* The stop/start operation because in this mode, picture may arrive very, very slowly  */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery detected error: LowDelay detected : no action."));

                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery detected error: notify STVID_DATA_ERROR_EVT !"));
                    VIDPROD_NotifyDecodeEvent(Device_p->DeviceData_p->ProducerHandle, STVID_DATA_ERROR_EVT, (const void *) NULL);
#ifdef TRACE_UART
                    TraceBuffer(("\r\n!!! Time-out but Low-delay detected (no action)."));
#endif /* TRACE_UART */
                }
                else
#endif /* ST_producer */
                {
#ifdef TRACE_UART
                    TraceBuffer(("\r\n!!! Error recovery error: automatic stop()/start()"));
#endif /* TRACE_UART */
                    /* React to error : stop/re-start with same parameters ! */
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Error recovery detected error: automatic stop()/start() !"));

                    Freeze.Mode     = STVID_FREEZE_MODE_FORCE; /* !!! ? this */
                    Freeze.Field    = STVID_FREEZE_FIELD_CURRENT; /* !!! ? this */
                    stvid_ActionStop(Device_p, STVID_STOP_NOW, &Freeze);
                    /* Re-start */
                    stvid_ActionStart(Device_p, &(Device_p->DeviceData_p->StartParams));

#ifdef STVID_DEBUG_GET_STATISTICS
                    switch (LiveResetState)
                    {
                        case WAIT_FOR_FIRST_PICTURE_DETECTED :  /* Decoder just started by STVID_Start() (or restarted).        */
                            Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDetected ++;
                            break;
                        case WAIT_FOR_FIRST_PICTURE_DECODE :    /* The first picture is parsed, and sufficient bit buffer level */
                                                                /* is expected to launch the very first decode.                 */
                            Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForFirstPictureDecoded ++;
                            break;
                        case WAIT_FOR_NEXT_PICTURE_DECODE :     /* Decode is going on. The next decode is expected.             */
                        default :     /* Decode is going on. The next decode is expected.             */
                            Device_p->DeviceData_p->StatisticsApiPbLiveResetWaitForNextPicture ++;
                            break;
                    }
#endif /* STVID_DEBUG_GET_STATISTICS */
                }
            }
        }
        stvid_LeaveCriticalSection(Device_p);
    } while (!(Device_p->DeviceData_p->ErrorRecoveryTask.ToBeDeleted));

    STOS_TaskExit(NULL);
} /* End of ErrorRecoveryTaskFunc() function */

#ifdef ST_producer
/*******************************************************************************
Name        : DecodeOrSkipLaunchedErrorRecovery
Description : Function to subscribe the launched of decode or skip of a picture
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void DecodeOrSkipLaunchedErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL))
    {
#ifdef TRACE_UART
        TraceBuffer(("\r\n!!! DecodeOrSkipLaunchedErrorRecovery NULL pointer error"));
#endif /* TRACE_UART */
        return;
    }

    semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    /* Test the current state of the live reset.    */
    switch (Device_p->DeviceData_p->LiveResetState)
    {
        case WAIT_FOR_NEXT_PICTURE_DECODE :
            /* Test the current number of VSync before a live reset     */
            if (Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset < MAX_VSYNC_BETWEEN_TWO_PICTURE_DECODE)
            {
                /* It's less than MAX_VSYNC_BETWEEN_TWO_PICTURE_DECODE, */
                /* no pending repeat action, update the current time-out*/
                Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = MAX_VSYNC_BETWEEN_TWO_PICTURE_DECODE;
            }
            /* Else, do not touch the current value. It takes into      */
            /* account the time-out of the repeated fields.             */
#ifdef TRACE_UART
            else
            {
                if (Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset != MAX_VSYNC_BETWEEN_TWO_PICTURE_DECODE)
                {
                    TraceBuffer(("\r\n   DecodeOrSkipLaunchedErrorRecovery (VSyncBeforeReset : %4d)"
                            , Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset));
                }
            }
#endif /* TRACE_UART */
            break;

        case WAIT_FOR_FIRST_PICTURE_DECODE :
            /* Second step after the start. We were waiting for the     */
            /* second decode order. This means that the vbv_buffer_size */
            /* is reached, and that decode is now in normal behavior.   */
            Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = MAX_VSYNC_BETWEEN_TWO_PICTURE_DECODE;
            Device_p->DeviceData_p->LiveResetState = WAIT_FOR_NEXT_PICTURE_DECODE;
#ifdef TRACE_UART
            TraceBuffer(("\r\n   (Dc/Sk) state WAIT_FOR_NEXT_PICTURE_DECODE  (VSyncBeforeReset : %d)"
                    , Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset));
#endif /* TRACE_UART */
            break;

        default :
            break;
    }

    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

} /* End of DecodeOrSkipLaunchedErrorRecovery() function */
#endif /* ST_producer */

#ifdef ST_producer
/*******************************************************************************
Name        : DecodeRestartedErrorRecovery
Description : Function to subscribe the decode SOFT RESET event
Parameters  : EVT decode
Assumptions :
Limitations :
Returns     : EVT decode
*******************************************************************************/
static void DecodeRestartedErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL))
    {
#ifdef TRACE_UART
        TraceBuffer(("\r\n!!! DecodeRestartedErrorRecovery NULL pointer error"));
#endif /* TRACE_UART */
        return;
    }

    /* Test if the decoder is running in REAL TIME mode.    */
    if (Device_p->DeviceData_p->IsErrorRecoveryInRealTime)
    {
        /* YES, the state machine can be initialized.   */
        semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

        /* The live reset automaton is started : next stage.*/
        Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = MAX_VSYNC_UNTIL_FIRST_PICTURE_DETECTED;
        Device_p->DeviceData_p->LiveResetState = WAIT_FOR_FIRST_PICTURE_DETECTED;

#ifdef TRACE_UART
TraceBuffer(("\r\n   (ReStr) state WAIT_FOR_FIRST_PICTURE_DETECTED (VSyncBeforeReset : %d)"
        , Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset));
#endif /* TRACE_UART */

        semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);
    }
#ifdef TRACE_UART
    else
    {
        /* NO, the live reset should have no action.    */
        TraceBuffer(("\r\n   (ReStr) state WAIT_FOR_VIDEO_START (not real time)"));
    }
#endif /* TRACE_UART */

} /* End of DecodeRestartedErrorRecovery() function */
#endif /* ST_producer */

#ifdef ST_display
/*******************************************************************************
Name        : FieldsRepeatedErrorRecovery
Description : Function to subscribe fields have been repeated by on display, or
              next picture display has been delayed by synchronisation
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
static void FieldsRepeatedErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);

    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL))
    {
#ifdef TRACE_UART
        TraceBuffer(("\r\n!!! FieldsRepeatedErrorRecovery NULL pointer error"));
#endif /* TRACE_UART */
        return;
    }

    /* No watchdog in non realtime mode, so no need to update timeout counter. */
    if (!(Device_p->DeviceData_p->IsErrorRecoveryInRealTime))
    {
        return;
    }

    semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    /* Test the current state of the live reset.    */
    if (Device_p->DeviceData_p->LiveResetState == WAIT_FOR_NEXT_PICTURE_DECODE)
    {
        /* Update the time-out in order to take into account repeated fields or
         * delay from synchronisation.   */
        Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset += (U32) *((U32 *)(EventData_p));

#ifdef TRACE_UART
        TraceBuffer(("\r\n   FieldsRepeatedErrorRecovery (%04d VSync -> VSyncBeforeReset : %4d)"
                , (U32) *((U32 *)(EventData_p))
                , Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset));
#endif /* TRACE_UART */
    }

    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

} /* End of FieldsRepeatedErrorRecovery() function */
#endif /* ST_display */

#ifdef ST_producer
/*******************************************************************************
Name        : ReadyToDecodeNewPictureErrorRecovery
Description : Function called for each new picture event
Parameters  : EVT API
Assumptions :
Limitations :
Returns     : EVT API
*******************************************************************************/
ST_ErrorCode_t stvid_ReadyToDecodeNewPictureErrorRecovery(VideoDevice_t * const Device_p)
{
    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL))
    {
#ifdef TRACE_UART
        TraceBuffer(("\r\n!!! ReadyToDecodeNewPictureErrorRecovery NULL pointer error"));
#endif /* TRACE_UART */
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    semaphore_wait(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    /* Test the current state of the live reset.    */
    if (Device_p->DeviceData_p->LiveResetState == WAIT_FOR_FIRST_PICTURE_DETECTED)
    {
        /* First step after the start. We were waiting for the  */
        /* first picture. Now, we have to wait for its decode.  */
        Device_p->DeviceData_p->LiveResetState = WAIT_FOR_FIRST_PICTURE_DECODE;
        Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset = MAX_VSYNC_UNTIL_FIRST_PICTURE_DECODE_LAUNCHED;

#ifdef TRACE_UART
        TraceBuffer(("\r\n   (NewPi) state WAIT_FOR_FIRST_PICTURE_DECODE (VSyncBeforeReset : %d)"
                , Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset));
#endif /* TRACE_UART */
    }

    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryDataAccessSemaphore_p);

    return(ST_NO_ERROR);
} /* End of ReadyToDecodeNewPictureErrorRecovery() function */
#endif /* ST_producer */

/*******************************************************************************
Name        : StartErrorRecoveryTask
Description : Start the task of error recovery mechanism
Parameters  : Opened decoder
Assumptions : Device_p is not NULL.
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
static ST_ErrorCode_t StartErrorRecoveryTask(VideoDevice_t * const Device_p, const U8 DecoderNumber)
{
    VIDCOM_Task_t * const Task_p = &(Device_p->DeviceData_p->ErrorRecoveryTask);
    ST_ErrorCode_t Error ;
    char TaskName[27] = "STVID[?].ErrorRecoveryTask";

    if (Task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    /* In TaskName string, replace '?' with decoder number */
    TaskName[6] = (char) ((U8) '0' + DecoderNumber);
    Task_p->ToBeDeleted = FALSE;

    Error = STOS_TaskCreate((void (*) (void*)) ErrorRecoveryTaskFunc, (void *) Device_p,
                    Device_p->DeviceData_p->CPUPartition_p ,
                    STVID_TASK_STACK_SIZE_ERROR_RECOVERY,   /* ? */
                    &Task_p->TaskStack,
                    Device_p->DeviceData_p->CPUPartition_p ,
                    &(Task_p->Task_p),
                    &(Task_p->TaskDesc),
                    STVID_TASK_PRIORITY_ERROR_RECOVERY,
                    TaskName,
                    task_flags_no_min_stack_size);

    if (Error != ST_NO_ERROR)
    {
        return(Error);
    }

    Task_p->IsRunning  = TRUE;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error recovery task created."));
    return(ST_NO_ERROR);
} /* End of StartErrorRecoveryTask() function */


/*******************************************************************************
Name        : StopErrorRecoveryTask
Description : Stop the task of error recovery mechanism
Parameters  : Opened decoder
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
static ST_ErrorCode_t StopErrorRecoveryTask(VideoDevice_t * const Device_p)
{
    VIDCOM_Task_t * const Task_p = &(Device_p->DeviceData_p->ErrorRecoveryTask);
    task_t *TaskList_p ;

    if (!(Task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }

    TaskList_p = Task_p->Task_p;

    /* End the function of the task here... */
    Task_p->ToBeDeleted = TRUE;

    /* And awake the task.                  */
    semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p);

    STOS_TaskWait(&TaskList_p, TIMEOUT_INFINITY);
    STOS_TaskDelete ( Task_p->Task_p,
                                  Device_p->DeviceData_p->CPUPartition_p,
                                  Task_p->TaskStack,
                                  Device_p->DeviceData_p->CPUPartition_p );

    Task_p->IsRunning  = FALSE;

    return(ST_NO_ERROR);
} /* End of StopErrorRecoveryTask() function */


#ifndef STVID_STVAPI_ARCHITECTURE
/*******************************************************************************
Name        : VSyncErrorRecovery
Description : Function to subscribe the VSYNC event
Parameters  : EVT API
Assumptions : Called under interrupt context
Limitations :
Returns     : EVT API
*******************************************************************************/
static void VSyncErrorRecovery(STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event, void *EventData_p, void *SubscriberData_p)
{
    VideoDevice_t * Device_p = (VideoDevice_t *) SubscriberData_p;

    UNUSED_PARAMETER(Reason);
    UNUSED_PARAMETER(RegistrantName);
    UNUSED_PARAMETER(Event);
    UNUSED_PARAMETER(EventData_p);

    if ((Device_p == NULL) || (Device_p->DeviceData_p == NULL))
    {
#ifdef TRACE_UART
        TraceBuffer(("\r\n!!! VSyncErrorRecovery NULL pointer error"));
#endif /* TRACE_UART */
        return;
    }

#ifdef STVID_NO_AUTO_STOP_START
        return;
#endif /* STVID_NO_AUTO_STOP_START */

    /* Check if error recovery live reset is in place. */
    /* Test of the real-time, the state, the speed, the error recovery mode. */
    /* The error recovery task may be schedulded only if the video decoder is in conditions ! (optimization : no task switching) */
    if ((Device_p->DeviceData_p->IsErrorRecoveryInRealTime) &&
        (Device_p->DeviceData_p->LiveResetState != WAIT_FOR_VIDEO_START) &&
        (Device_p->DeviceData_p->CurrentAbsoluteSpeed != 0) &&
        (Device_p->DeviceData_p->ErrorRecoveryMode != STVID_ERROR_RECOVERY_NONE)
       )
    {
        /* As we are in interrupt context, we can manipulate MaxNumberOfVSyncToWaitBeforeLiveReset with no risk that it is overwritten */
        if (Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset > 0)
        {
            /* An other VSync occured. Decrement time-out */
            Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset --;
            if (Device_p->DeviceData_p->MaxNumberOfVSyncToWaitBeforeLiveReset == 0)
            {
                /* Counter reached 0, trigger task to perform live reset... */
                semaphore_signal(Device_p->DeviceData_p->ErrorRecoveryTriggerSemaphore_p);
            }
        }
    }
} /* End of VSyncErrorRecovery() function */
#endif /* end of ndef STVID_STVAPI_ARCHITECTURE */

/* End of vid_err.c */
