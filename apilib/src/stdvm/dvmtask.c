/*****************************************************************************

File Name  : dvmtask.c

Description: STDVM internal task manager

Copyright (C) 2005 STMicroelectronics

*****************************************************************************/

/* Includes ----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dvmtask.h"
#include "handle.h"
#include "event.h"


/* Private Constants ------------------------------------------------------ */
#define STDVMi_SERVICE_TASK_NAME        "STDVM_ServiceTask"

#ifdef ST_OS21
    #define STDVMi_SERVICE_TASK_STACK_SIZE  ((OS21_DEF_MIN_STACK_SIZE)+(2*1024))
#else
    #define STDVMi_SERVICE_TASK_STACK_SIZE  (2*1024)
#endif

#define STDVMi_SERVICE_TASK_MQ_SIZE     (2)

#ifndef STDVM_SERVICE_TASK_PRIORITY
    #define STDVM_SERVICE_TASK_PRIORITY     MIN_USER_PRIORITY
#endif

/* Private Types ---------------------------------------------------------- */
typedef enum STDVMi_TaskState_e
{
    TASK_RUNNING,
    TASK_STOPPED
}STDVMi_TaskState_t;

/* Private Variables ------------------------------------------------------ */

static  task_t             *STDVMi_ServiceTask_p;
static  message_queue_t    *STDVMi_ServiceTaskMQ_p;

static  STDVMi_TaskState_t  STDVMi_ServiceTaskState;

/* Private Function prototypes -------------------------------------------- */

static void STDVMi_ServiceTask(void *NULL_p);

/* Global Variables ------------------------------------------------------- */

/* Prototypes ------------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

/*******************************************************************************
Name         : STDVMi_ServiceTaskInit(void)

Description  :

Parameters   :

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_ServiceTaskInit(void)
{
    /* message queue for record change pid */
    STDVMi_ServiceTaskMQ_p = message_create_queue_timeout(sizeof(STDVMi_Handle_t **), STDVMi_SERVICE_TASK_MQ_SIZE);
    if(STDVMi_ServiceTaskMQ_p == NULL )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Service Task message_create_queue failed\n"));
        return ST_ERROR_NO_MEMORY;
    }

    STDVMi_ServiceTaskState = TASK_RUNNING;

    STDVMi_ServiceTask_p = task_create(STDVMi_ServiceTask,
                                NULL,
                                STDVMi_SERVICE_TASK_STACK_SIZE,
                                STDVM_SERVICE_TASK_PRIORITY,
                                STDVMi_SERVICE_TASK_NAME,
                                task_flags_no_min_stack_size );
    if(STDVMi_ServiceTask_p == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Service Task create failed\n"));
        return ST_ERROR_NO_MEMORY;
    }

    return ST_NO_ERROR;
}

/*******************************************************************************
Name         : STDVMi_ServiceTaskTerm()

Description  : Terminates STDVM event notification task

Parameters   : BOOL     ForceTerminate

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_ServiceTaskTerm(void)
{
    STDVMi_Handle_t   **Msg2Send;


    Msg2Send = (STDVMi_Handle_t **)message_claim(STDVMi_ServiceTaskMQ_p);

    STDVMi_ServiceTaskState = TASK_STOPPED;

    message_send(STDVMi_ServiceTaskMQ_p, Msg2Send);

    task_wait(&STDVMi_ServiceTask_p, 1, TIMEOUT_INFINITY);
    if(task_delete(STDVMi_ServiceTask_p) != 0 )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "task_delete failed\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    message_delete_queue(STDVMi_ServiceTaskMQ_p);

    return ST_NO_ERROR;
}

/*******************************************************************************
Name         : STDVMi_GetChangePidState()

Description  : STDVM internal state notification function

Parameters   : *Handle_p

Return Value :
*******************************************************************************/
STDVMi_PidChangeState_t STDVMi_GetChangePidState(STDVMi_Handle_t *Handle_p)
{
    return Handle_p->PidChangeState;
}

/*******************************************************************************
Name         : STDVMi_GetChangePidState()

Description  : STDVM internal state notification function

Parameters   : *Handle_p

Return Value :
*******************************************************************************/
void STDVMi_SetChangePidState(STDVMi_Handle_t *Handle_p, STDVMi_PidChangeState_t PidChangeState)
{
    Handle_p->PidChangeState = PidChangeState;
}

/*******************************************************************************
Name         : STDVMi_NofityPidChange()

Description  : STDVM internal state change function

Parameters   : *Handle_p

Return Value :
*******************************************************************************/
ST_ErrorCode_t STDVMi_NotifyPidChange(STDVMi_Handle_t *Handle_p)
{
    STDVMi_Handle_t   **Msg2Send;

    Handle_p->PidChangeState = PID_CHANGE_CHANGING;

    Msg2Send = (STDVMi_Handle_t **)message_claim(STDVMi_ServiceTaskMQ_p);

    *Msg2Send = Handle_p;

    message_send(STDVMi_ServiceTaskMQ_p, Msg2Send);

    return ST_NO_ERROR;
}


/*******************************************************************************
Name         : STDVMi_ServiceTask()

Description  : STDVM service task

Parameters   : void     *NULL_p     NOT USED

Return Value :
*******************************************************************************/
static void STDVMi_ServiceTask(void *NULL_p)
{
    ST_ErrorCode_t               ErrorCode;
    STDVMi_Handle_t             *Handle_p;
    STDVMi_Handle_t            **MsgReceived;
    clock_t                      Time2Wait;
    U32                          StartTimeInMs, CurrentTimeInMs, EndTimeInMs;
    S32                          RemainingTime;
    S32                          AudioTrickTime;
    U32                          i;


    while(STDVMi_ServiceTaskState == TASK_RUNNING)
    {
        /* wait on message queue to receive message otherwise check end of file for audio trick mode */
        Time2Wait = time_plus(time_now(), ST_GetClocksPerSecond()/10);

        MsgReceived = message_receive_timeout(STDVMi_ServiceTaskMQ_p, &Time2Wait);

        if(STDVMi_ServiceTaskState != TASK_RUNNING)
        {
            if(MsgReceived != NULL)
            {
                message_release(STDVMi_ServiceTaskMQ_p, MsgReceived);
            }
            break;
        }

        if(MsgReceived != NULL)
        {
            Handle_p = *MsgReceived;

        	message_release(STDVMi_ServiceTaskMQ_p, MsgReceived);

            if(Handle_p->PidChangeState == PID_CHANGE_CHANGING)
            {
                /* TODO: Get a copy of the PIDs list or protect Handle_p, since it may be invalid when we reach here */
                ErrorCode = STDVM_PlayChangePids((STDVM_Handle_t)Handle_p,
                                    Handle_p->StreamInfo_p->ProgramInfo->NbOfPids,
                                    Handle_p->StreamInfo_p->ProgramInfo->Pids);
                if(ErrorCode != ST_NO_ERROR)
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_ServiceTask: STDVM_PlayChangePids()=%08X\n", ErrorCode));
                }

                Handle_p->PidChangeState = PID_CHANGE_IDLE;
            }
        }
        else  /* Audio trick mode */
        {
            for(i = 0; i < STDVMi_NbOfHandles; i++)
            {
                Handle_p = &STDVMi_Handles_p[i];

                if((Handle_p->HandleInUse == TRUE) &&
                   (Handle_p->ObjectType == STDVM_OBJECT_PLAYBACK) &&
                   (Handle_p->AudioOnlyProg == TRUE) &&
                   (Handle_p->AudioInTrickMode == TRUE) &&
                   (Handle_p->AudioInPause == FALSE) &&
                   (Handle_p->AudioTrickEndOfFile == FALSE))
                {
                    ErrorCode = STPRM_PlayGetTime(Handle_p->PRM_Handle, &StartTimeInMs, &CurrentTimeInMs, &EndTimeInMs);
                    if(ErrorCode != ST_NO_ERROR)
                    {
                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetTime : %08X\n", ErrorCode));
                        continue;
                    }

                    AUDIO_TRICK_ACCESS_MUTEX_LOCK(Handle_p);

                    if(Handle_p->AudioTrickStartTime != 0)
                    {
                        AudioTrickTime = (S32)(((float)time_minus(time_now(), Handle_p->AudioTrickStartTime) * \
                                                Handle_p->CurrentSpeed * 10) / ST_GetClocksPerSecond());
                    }
                    else
                    {
                        AudioTrickTime = 0;
                    }
                    CurrentTimeInMs = Handle_p->CurrentTime + AudioTrickTime;

                    RemainingTime   = (Handle_p->CurrentSpeed > 0) ? (EndTimeInMs - CurrentTimeInMs) : (CurrentTimeInMs - StartTimeInMs);

                    AUDIO_TRICK_ACCESS_MUTEX_RELEASE(Handle_p);

                    if(RemainingTime <= 0)
                    {
                        STPRM_PlayStatus_t           PlayStatus;
                        STPRM_RecordStatus_t         RecordStatus;
                        U32                          HandleIndex;

                        ErrorCode = STPRM_PlayGetStatus(Handle_p->PRM_Handle, &PlayStatus);
                        if(ErrorCode != ST_NO_ERROR)
                        {
                            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayGetStatus : %08X\n", ErrorCode));
                            break;
                        }

                        for(HandleIndex = 0; HandleIndex < STDVMi_NbOfHandles; HandleIndex++)
                        {
                            if((STDVMi_Handles_p[HandleIndex].HandleInUse == TRUE) &&                       /* handle is allocated */
                               (STDVMi_Handles_p[HandleIndex].ObjectType == STDVM_OBJECT_RECORD) &&
                                (STDVMi_Handles_p[HandleIndex].StreamInfo_p->Signature == STDVMi_SIGNATURE))         /* object type is record */
                            {
                                ErrorCode = STPRM_RecordGetStatus(STDVMi_Handles_p[HandleIndex].PRM_Handle, &RecordStatus);
                                if(ErrorCode != ST_NO_ERROR)
                                {
                                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_RecordGetStatus : %08X\n", ErrorCode));
                                    continue;
                                }

                                if(!(strcmp(PlayStatus.SourceName,RecordStatus.DestinationName)))
                                {
                                    ErrorCode = STPRM_PlaySeek(Handle_p->PRM_Handle, EndTimeInMs, STPRM_PLAY_SEEK_SET);
                                    if(ErrorCode != ST_NO_ERROR)
                                    {
                                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlaySeek : %08X\n", ErrorCode));
                                        break;
                                    }

                                    ErrorCode = STPRM_PlayResume(Handle_p->PRM_Handle);
                                    if(ErrorCode != ST_NO_ERROR)
                                    {
                                        STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STPRM_PlayResume : %08X\n", ErrorCode));
                                        break;
                                    }

                                    Handle_p->CurrentSpeed = STDVM_PLAY_SPEED_ONE;
                                    Handle_p->AudioTrickStartTime = 0;
                                    Handle_p->AudioInTrickMode = FALSE;
                                    break;
                                }
                            }
                        }
                        if(HandleIndex == STDVMi_NbOfHandles)
                        {
                            Handle_p->CurrentTime         = (Handle_p->CurrentSpeed > 0) ? EndTimeInMs : StartTimeInMs;
                            Handle_p->AudioTrickStartTime = 0;
                            Handle_p->AudioTrickEndOfFile = TRUE;

                            ErrorCode = STDVMi_NotifyEvent(STDVM_EVT_END_OF_FILE, Handle_p);
                            if(ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_NotifyEvent : %08X\n", ErrorCode));
                            }

                            ErrorCode = STDVMi_NotifyEvent(STDVM_EVT_END_OF_PLAYBACK, Handle_p);
                            if(ErrorCode != ST_NO_ERROR)
                            {
                                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "STDVMi_NotifyEvent : %08X\n", ErrorCode));
                            }
                        }
                    }
                }
            }/* for */
        }
    }/* while */
}

/* EOF ---------------------------------------------------------------------- */

