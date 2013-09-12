/*****************************************************************************

File Name  : event.c

Description: Event notification functions of STDVM

COPYRIGHT (C) STMicroelectronics 2005

*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvmfs.h"
#include "dvmindex.h"
#include "handle.h"

#include "stevt.h"


/* Private Constants ------------------------------------------------------ */
#define STDVMi_EVENT_TASK_NAME          "STDVM_EventTask"

#ifdef ST_OS21
    #define STDVMi_EVENT_TASK_STACK_SIZE    ((OS21_DEF_MIN_STACK_SIZE)+(2*1024))
#else
    #define STDVMi_EVENT_TASK_STACK_SIZE    (2*1024)
#endif

#define STDVMi_EVENT_TASK_MQ_SIZE       (2)

#ifndef STDVM_EVENT_TASK_PRIORITY
    #define STDVM_EVENT_TASK_PRIORITY       MIN_USER_PRIORITY
#endif

/* Private Types ---------------------------------------------------------- */
typedef struct STDVMi_Event_s
{
    STDVM_Event_t       Event;

    union
    {
        STDVM_EventData_t   EventData;
        STDVM_EventError_t  EventError;
    } U;

} STDVMi_Event_t;


typedef enum STDVMi_TaskState_e
{
    TASK_RUNNING,
    TASK_STOPPED
} STDVMi_TaskState_t;



/* Private Variables ------------------------------------------------------ */
static  STEVT_Handle_t      STDVMi_EVTHandle;
static  STEVT_EventID_t     STDVMi_EVTIDs[STDVM_MAX_EVTS];

static  task_t             *STDVMi_EventTask_p;
static  message_queue_t    *STDVMi_EventTaskMQ_p;

static  STDVMi_TaskState_t  STDVMi_EventTaskState;

static  U32                 STDVMi_ReadWriteThresholdInMs;
static  U32                 STDVMi_ReadWriteThresholdOKInMs;
static  U32                 STDVMi_MinDiskSpace;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */
static ST_ErrorCode_t STDVMi_RegisterAndSubscribeEvents(ST_DeviceName_t DVMDeviceName,
                                                        ST_DeviceName_t PRMDeviceName,
                                                        ST_DeviceName_t MP3PBDeviceName,
                                                        ST_DeviceName_t EVTDeviceName);
static ST_ErrorCode_t STDVMi_UnRegisterAndUnSubscribeEvents(ST_DeviceName_t DVMDeviceName,
                                                            ST_DeviceName_t PRMDeviceName,
                                                            ST_DeviceName_t MP3PBDeviceName);

static void STDVMi_EventCallback(   STEVT_CallReason_t    Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void           *EventData,
                                    const void           *SubscriberData_p);

static void STDVMi_EventTask(void *NULL_p);

/* Global Variables ------------------------------------------------------- */

/* Prototypes ------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */


/*******************************************************************************
Name         : STDVMi_EventInit()

Description  : Creates task for STDVM event notifications

Parameters   : STDVM_InitParams_t   *InitParams_p       Init params passed to STDVM_Init

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_EventInit(ST_DeviceName_t     DVMDeviceName,
                                ST_DeviceName_t     PRMDeviceName,
                                ST_DeviceName_t     MP3PBDeviceName,
                                STDVM_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t  ErrorCode;


    ErrorCode = STDVMi_RegisterAndSubscribeEvents(DVMDeviceName, PRMDeviceName, MP3PBDeviceName, InitParams_p->EVTDeviceName);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_RegisterAndSubscribeEvents()=%08X\n", ErrorCode));
        return ErrorCode;
    }

    STDVMi_EventTaskMQ_p = message_create_queue(sizeof(STDVMi_Event_t), STDVMi_EVENT_TASK_MQ_SIZE);
    if(STDVMi_EventTaskMQ_p == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "message_create_queue failed\n"));
        return ST_ERROR_NO_MEMORY;
    }

    STDVMi_EventTaskState = TASK_RUNNING;

    STDVMi_EventTask_p = task_create( STDVMi_EventTask,
                               NULL,
                               STDVMi_EVENT_TASK_STACK_SIZE,
                               STDVM_EVENT_TASK_PRIORITY,
                               STDVMi_EVENT_TASK_NAME,
                               task_flags_no_min_stack_size );
    if(STDVMi_EventTask_p == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "task_create failed\n"));
        return ST_ERROR_NO_MEMORY;
    }

    STDVMi_ReadWriteThresholdInMs   = InitParams_p->ReadWriteThresholdInMs;
    STDVMi_ReadWriteThresholdOKInMs = InitParams_p->ReadWriteThresholdOKInMs;
    STDVMi_MinDiskSpace             = InitParams_p->MinDiskSpace;

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_EventTerm()

Description  : Terminates STDVM event notification task

Parameters   : BOOL     ForceTerminate

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_EventTerm(ST_DeviceName_t     DVMDeviceName,
                                ST_DeviceName_t     PRMDeviceName,
                                ST_DeviceName_t     MP3PBDeviceName,
                                BOOL                ForceTerminate)
{
    ST_ErrorCode_t  ErrorCode;
    STDVMi_Event_t *Event_p;


    ErrorCode = STDVMi_UnRegisterAndUnSubscribeEvents(DVMDeviceName, PRMDeviceName, MP3PBDeviceName);
    if(ErrorCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_UnRegisterAndUnSubscribeEvents()=%08X\n", ErrorCode));
        return ErrorCode;
    }


    Event_p = (STDVMi_Event_t *)message_claim(STDVMi_EventTaskMQ_p);

    STDVMi_EventTaskState = TASK_STOPPED;

    message_send(STDVMi_EventTaskMQ_p, Event_p);

    task_wait(&STDVMi_EventTask_p, 1, TIMEOUT_INFINITY);
    if(task_delete(STDVMi_EventTask_p) != 0 )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "task_delete failed\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    message_delete_queue(STDVMi_EventTaskMQ_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_RegisterAndSubscribeEvents()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
static ST_ErrorCode_t STDVMi_RegisterAndSubscribeEvents(ST_DeviceName_t DVMDeviceName,
                                                        ST_DeviceName_t PRMDeviceName,
                                                        ST_DeviceName_t MP3PBDeviceName,
                                                        ST_DeviceName_t EVTDeviceName)
{
    ST_ErrorCode_t                  ErrCode;
    STEVT_OpenParams_t              EVT_OpenParams;
    U32                             i;
    STEVT_DeviceSubscribeParams_t   SubscribeParams = { 0 };


    memset(&EVT_OpenParams,0,sizeof(STEVT_OpenParams_t));

    ErrCode = STEVT_Open(EVTDeviceName, &EVT_OpenParams, &STDVMi_EVTHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Open : %08X\n", ErrCode));
        return ST_ERROR_BAD_PARAMETER;
    }

    for(i = STDVM_EVT_END_OF_PLAYBACK_ID; i < STDVM_MAX_EVTS; i++)
    {
        ErrCode = STEVT_RegisterDeviceEvent(STDVMi_EVTHandle, DVMDeviceName, STDVM_EVT_END_OF_PLAYBACK+i, &STDVMi_EVTIDs[i]);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_RegisterDeviceEvent : %08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    SubscribeParams.NotifyCallback      = STDVMi_EventCallback;
    SubscribeParams.SubscriberData_p    = NULL;

    for(i = STPRM_EVT_END_OF_PLAYBACK; i <= STPRM_EVT_ERROR; i++)
    {
        ErrCode = STEVT_SubscribeDeviceEvent(STDVMi_EVTHandle, PRMDeviceName, i, &SubscribeParams);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Subscribe : %08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

#ifdef ENABLE_MP3_PLAYBACK
    for(i = MP3PB_EVT_END_OF_PLAYBACK; i <= MP3PB_EVT_ERROR; i++)
    {
        ErrCode = STEVT_SubscribeDeviceEvent(STDVMi_EVTHandle, MP3PBDeviceName, i, &SubscribeParams);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Subscribe : %08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }
#endif

    return ST_NO_ERROR;
}


static ST_ErrorCode_t STDVMi_UnRegisterAndUnSubscribeEvents(ST_DeviceName_t DVMDeviceName,
                                                            ST_DeviceName_t PRMDeviceName,
                                                            ST_DeviceName_t MP3PBDeviceName)
{
    ST_ErrorCode_t                  ErrCode;
    U32                             i;


#ifdef ENABLE_MP3_PLAYBACK
    for(i = MP3PB_EVT_END_OF_PLAYBACK; i <= MP3PB_EVT_ERROR; i++)
    {
        ErrCode = STEVT_UnsubscribeDeviceEvent(STDVMi_EVTHandle, MP3PBDeviceName, i);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Unsubscribe : %08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }
#endif

    for(i = STPRM_EVT_END_OF_PLAYBACK; i <= STPRM_EVT_ERROR; i++)
    {
        ErrCode = STEVT_UnsubscribeDeviceEvent(STDVMi_EVTHandle, PRMDeviceName, i);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Unsubscribe : %08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    for(i = STDVM_EVT_END_OF_PLAYBACK_ID; i < STDVM_MAX_EVTS; i++)
    {
        ErrCode = STEVT_UnregisterDeviceEvent(STDVMi_EVTHandle, DVMDeviceName, STDVM_EVT_END_OF_PLAYBACK+i);
        if(ErrCode != ST_NO_ERROR)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_RegisterDeviceEvent : %08X\n", ErrCode));
            return ST_ERROR_BAD_PARAMETER;
        }
    }

    ErrCode = STEVT_Close(STDVMi_EVTHandle);
    if(ErrCode != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Close : %08X\n", ErrCode));
        return ST_ERROR_BAD_PARAMETER;
    }

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_EventCallback()

Description  :

Parameters   :

Return Value :
*******************************************************************************/
static void STDVMi_EventCallback(   STEVT_CallReason_t    Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void           *EventData,
                                    const void           *SubscriberData_p)
{
    ST_ErrorCode_t      ErrCode;

    switch(Event)
    {
        case STPRM_EVT_END_OF_PLAYBACK :
            {
                STDVM_EventData_t   DVMEventData;

                DVMEventData.Handle = (STDVM_Handle_t)STDVMi_GetHandleFromPRMHandle(((STPRM_EventData_t *)EventData)->Handle);
                DVMEventData.Type   = ((STPRM_EventData_t *)EventData)->Type;

                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_END_OF_PLAYBACK_ID], &DVMEventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
            }
            break;

        case STPRM_EVT_END_OF_FILE :
            {
                STDVM_EventData_t   DVMEventData;

                DVMEventData.Handle = (STDVM_Handle_t)STDVMi_GetHandleFromPRMHandle(((STPRM_EventData_t *)EventData)->Handle);
                DVMEventData.Type   = ((STPRM_EventData_t *)EventData)->Type;

                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_END_OF_FILE_ID], &DVMEventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
            }
            break;

        case STPRM_EVT_ERROR :
            {
                STDVM_EventError_t  DVMEventError;

                DVMEventError.Handle = (STDVM_Handle_t)STDVMi_GetHandleFromPRMHandle(((STPRM_EventError_t *)EventData)->Handle);
                DVMEventError.Type   = ((STPRM_EventError_t *)EventData)->Type;
                DVMEventError.Error  = ((STPRM_EventError_t *)EventData)->Error;

                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_ERROR_ID], &DVMEventError);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
            }
            break;

#ifdef ENABLE_MP3_PLAYBACK
        case MP3PB_EVT_END_OF_PLAYBACK :
            {
                STDVM_EventData_t   DVMEventData;

                DVMEventData.Handle = (STDVM_Handle_t)STDVMi_GetHandleFromMP3PBHandle(*((MP3PB_Handle_t *)EventData));
                DVMEventData.Type   = STDVM_OBJECT_PLAYBACK;

                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_END_OF_PLAYBACK_ID], &DVMEventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
            }
            break;

        case MP3PB_EVT_END_OF_FILE :
            {
                STDVM_EventData_t   DVMEventData;

                DVMEventData.Handle = (STDVM_Handle_t)STDVMi_GetHandleFromMP3PBHandle(*((MP3PB_Handle_t *)EventData));
                DVMEventData.Type   = STDVM_OBJECT_PLAYBACK;

                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_END_OF_FILE_ID], &DVMEventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
            }
            break;

        case MP3PB_EVT_ERROR :
            {
                STDVM_EventError_t  DVMEventError;

                DVMEventError.Handle = (STDVM_Handle_t)STDVMi_GetHandleFromMP3PBHandle(*((MP3PB_Handle_t *)EventData));
                DVMEventError.Type   = STDVM_OBJECT_PLAYBACK;
                DVMEventError.Error  = STDVM_PLAY_READ_ERROR;

                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_ERROR_ID], &DVMEventError);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
            }
            break;
#endif

        default:
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unknown event from STPRM %08X\n", Event));
            break;
    }
}


/*******************************************************************************
Name         : STDVMi_EventTask()

Description  : STDVM Event task notifies STDVM internal events

Parameters   : void     *NULL_p     NOT USED

Return Value :
*******************************************************************************/
static void STDVMi_EventTask(void *NULL_p)
{
    ST_ErrorCode_t      ErrCode;
    STDVMi_Event_t     *Event_p;


    while(STDVMi_EventTaskState == TASK_RUNNING)
    {
        Event_p = message_receive(STDVMi_EventTaskMQ_p);

        if(STDVMi_EventTaskState != TASK_RUNNING)
        {
            message_release(STDVMi_EventTaskMQ_p, Event_p);
            break;
        }

        switch(Event_p->Event)
        {
            case STDVM_EVT_END_OF_FILE:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_END_OF_FILE_ID], &Event_p->U.EventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            case STDVM_EVT_END_OF_PLAYBACK:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_END_OF_PLAYBACK_ID], &Event_p->U.EventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            case STDVM_EVT_WRITE_CLOSE_TO_READ:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_WRITE_CLOSE_TO_READ_ID], &Event_p->U.EventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            case STDVM_EVT_WRITE_TO_READ_MARGIN_OK:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_WRITE_TO_READ_MARGIN_OK_ID],&Event_p->U.EventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            case STDVM_EVT_DISK_SPACE_LOW:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_DISK_SPACE_LOW_ID], &Event_p->U.EventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            case STDVM_EVT_DISK_FULL:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_DISK_FULL_ID], &Event_p->U.EventData);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            case STDVM_EVT_ERROR:
                ErrCode = STEVT_Notify(STDVMi_EVTHandle, STDVMi_EVTIDs[STDVM_EVT_ERROR_ID], &Event_p->U.EventError);
                if(ErrCode != ST_NO_ERROR)
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STEVT_Notify : %08X\n", ErrCode));
                break;

            default:
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Unknown event from STDVM %08X\n", Event_p->Event));
                break;
        }

        message_release(STDVMi_EventTaskMQ_p, Event_p);
    }
}


/*******************************************************************************
Name         : STDVMi_NotifyEvent()

Description  : STDVM internal events notification function

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_NotifyEvent(STDVM_Event_t Event, STDVMi_Handle_t *STDVMi_Handle_p)
{
    STDVMi_Event_t     *Event_p;

    Event_p = (STDVMi_Event_t *)message_claim(STDVMi_EventTaskMQ_p);

    Event_p->Event              = Event;
    Event_p->U.EventData.Type   = STDVMi_Handle_p->ObjectType;
    Event_p->U.EventData.Handle = (STDVM_Handle_t)STDVMi_Handle_p;

    message_send(STDVMi_EventTaskMQ_p, Event_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_NotifyErrorEvent()

Description  : STDVM internal events notification function

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_NotifyErrorEvent(STDVM_Event_t Event, STDVM_Errors_t Error, STDVMi_Handle_t *STDVMi_Handle_p)
{
    STDVMi_Event_t     *Event_p;

    Event_p = (STDVMi_Event_t *)message_claim(STDVMi_EventTaskMQ_p);

    Event_p->Event               = Event;
    Event_p->U.EventError.Type   = STDVMi_Handle_p->ObjectType;
    Event_p->U.EventError.Handle = (STDVM_Handle_t)STDVMi_Handle_p;
    Event_p->U.EventError.Error  = Error;

    message_send(STDVMi_EventTaskMQ_p, Event_p);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_GetReadWriteThresholdInMs()

Description  : STDVM internal events notification function

Parameters   :

Return Value :
*******************************************************************************/
U32 STDVMi_GetReadWriteThresholdInMs(void)
{
    return STDVMi_ReadWriteThresholdInMs;
}


/*******************************************************************************
Name         : STDVMi_GetReadWriteThresholdOKInMs()

Description  : STDVM internal events notification function

Parameters   :

Return Value :
*******************************************************************************/
U32 STDVMi_GetReadWriteThresholdOKInMs(void)
{
    return STDVMi_ReadWriteThresholdOKInMs;
}


/*******************************************************************************
Name         : STDVMi_GetMinDiskSpace()

Description  : STDVM internal events notification function

Parameters   :

Return Value :
*******************************************************************************/
U32 STDVMi_GetMinDiskSpace(void)
{
    return STDVMi_MinDiskSpace;
}


/* EOF ---------------------------------------------------------------------- */

