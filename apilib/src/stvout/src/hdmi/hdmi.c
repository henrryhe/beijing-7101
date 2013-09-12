/*******************************************************************************
File name   : hdmi.c

Description : VOUT driver body file for hdmi functions (free version)

COPYRIGHT (C) STMicroelectronics 2004.

Date               Modification                                     Name
----               ------------                                     ----
3  Apr 2004        Created                                          AC
17 Nov 2004        Add Support of ST40/OS21                         AC
27 Dec 2005        Add info frame transmission for HDMI interface   AC
*******************************************************************************/


/* Includes ----------------------------------------------------------------- */
#if !defined (ST_OSLINUX)
#include <stdio.h>
#include <string.h>
#endif

#include "hdmi.h"
#include "hdmi_int.h"
#include "hdmi_ip.h"

#if defined (STVOUT_TRACE)
#include "..\..\..\dvdgr-prj-stapigat\src\trace.h"
#endif

/* Private Types ---------------------------------------------------------- */

/* Private Constants ------------------------------------------------------ */
#define MAX_VSYNC_DURATION               (ST_GetClocksPerSecond()/30)
/* Max time waiting for the cell reset */
#define MAX_WAIT_RESET_TIME              (MAX_VSYNC_DURATION * 2)


/* Private variables (static) --------------------------------------------- */

/* Global Variables ------------------------------------------------------- */
#if defined(STVOUT_STATE_MACHINE_TRACE)
extern U32 PIO_Int_Time;
U32 Send_To_Check_Time;
#endif


/* Private Function prototypes--------------------------------------------- */
static void InternalTaskFunction (HDMI_Handle_t const Handle);
static STVOUT_State_t HDMI_ChangeState (const HDMI_Handle_t Handle);

/* Exported Functions------------------------------------------------------ */

/*-----------------------------Push/Pop Commands----------------------------*/
/*****************************************************************************
Name        : Hdmicom_PopCommand
Description : Pop a command from a circular buffer of commands
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
ST_ErrorCode_t Hdmicom_PopCommand(CommandsBuffer_t * const Buffer_p, U8 * const Data_p)
{
    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    /* Check that buffer is not empty. */
    if (Buffer_p->UsedSize != 0)
    {
        /* Read character */
        *Data_p = *(Buffer_p->BeginPointer_p);

        if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
        {
            Buffer_p->BeginPointer_p = Buffer_p->Data_p;
        }
        else
        {
            (Buffer_p->BeginPointer_p)++;
        }

        /* Update size */
        (Buffer_p->UsedSize)--;
    }
    else
    {
        /* Un-protect access to BeginPointer_p and UsedSize */
        STOS_InterruptUnlock();
        /* Can't pop command: buffer empty. So go out after interrupt_unlock() */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

    return(ST_NO_ERROR);
} /* End of Hdmicom_PopCommand() function */

/*******************************************************************************
Name        : Hdmicom_PushCommand
Description : Push a command into a circular buffer of commands
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void Hdmicom_PushCommand(CommandsBuffer_t * const Buffer_p, const U8 Data)
{

    /* Protect access to BeginPointer_p and UsedSize */
    STOS_InterruptLock();

    if (Buffer_p->UsedSize + 1 <= Buffer_p->TotalSize)
    {
        /* Write character */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - Buffer_p->TotalSize) = Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize) = Data;
        }

        /* Update size */
        (Buffer_p->UsedSize)++;

        /* Monitor max used size in commands buffer */
        if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
        {
            Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
        }
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
    STOS_InterruptUnlock();

    /* Can't push command: buffer full */
    return;
} /* End of Hdmicom_PushCommand() function */
/*******************************************************************************
Name        : Hdmicom_PopCommand32
Description : Pop a command from a circular buffer of commands
Parameters  : Buffer, pointer to data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't pop if empty
Returns     : ST_NO_ERROR if success,
              ST_ERROR_NO_MEMORY if buffer empty,
              ST_ERROR_BAD_PARAMETER if bad params
*******************************************************************************/
ST_ErrorCode_t Hdmicom_PopCommand32(CommandsBuffer32_t * const Buffer_p, U32 * const Data_p)
{

#ifdef ST_OS21
    U32 Interrupt_Mask;
#endif
    if (Data_p == NULL)
    {
        /* No param pointer provided: cannot return data ! */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Protect access to BeginPointer_p and UsedSize */
#ifdef ST_OS21
    Interrupt_Mask = interrupt_mask_all();
#else
    STOS_InterruptLock();
#endif
    /* Check that buffer is not empty. */

    if (Buffer_p->UsedSize != 0)
    {
        /* Read character */
        *Data_p = *(Buffer_p->BeginPointer_p);



        if ((Buffer_p->BeginPointer_p) >= (Buffer_p->Data_p + Buffer_p->TotalSize - 1))
        {
            Buffer_p->BeginPointer_p = Buffer_p->Data_p;
        }
        else
        {
            (Buffer_p->BeginPointer_p)++;
        }

        /* Update size */
        (Buffer_p->UsedSize)--;
    }
    else
    {
        /* Un-protect access to BeginPointer_p and UsedSize */
#ifdef ST_OS21
        interrupt_unmask(Interrupt_Mask);
#else
        STOS_InterruptUnlock();
#endif
        /* Can't pop command: buffer empty. So go out after interrupt_unlock() */
        return(ST_ERROR_NO_MEMORY);
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
#ifdef ST_OS21
        interrupt_unmask(Interrupt_Mask);
#else
        STOS_InterruptUnlock();
#endif
    return(ST_NO_ERROR);
} /* end of Hdmicom_PopCommand32() function*/
/*******************************************************************************
Name        : hdmicom_PushCommand32
Description : Push a command into a circular buffer of commands
Parameters  : Buffer, data
Assumptions : Buffer_p is not NULL
Limitations : Doesn't push if full
Returns     : Nothing
*******************************************************************************/
void Hdmicom_PushCommand32(CommandsBuffer32_t * const Buffer_p, const U32 Data)
{
    /* Protect access to BeginPointer_p and UsedSize */
#ifdef ST_OS21
    U32 Interrupt_Mask;

    Interrupt_Mask = interrupt_mask_all();
#else
    STOS_InterruptLock();
#endif
    if (Buffer_p->UsedSize + 1 <= Buffer_p->TotalSize)
    {
        /* Write character */
        if ((Buffer_p->BeginPointer_p + Buffer_p->UsedSize) >= (Buffer_p->Data_p + Buffer_p->TotalSize))
        {
            /* Wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize - Buffer_p->TotalSize) = Data;
        }
        else
        {
            /* No wrap around Data_p buffer */
            *(Buffer_p->BeginPointer_p + Buffer_p->UsedSize) = Data;
        }

        /* Update size */
        (Buffer_p->UsedSize)++;

        /* Monitor max used size in commands buffer */
        if (Buffer_p->UsedSize > Buffer_p->MaxUsedSize)
        {
            Buffer_p->MaxUsedSize = Buffer_p->UsedSize;
        }
    }

    /* Un-protect access to BeginPointer_p and UsedSize */
#ifdef ST_OS21
    interrupt_unmask(Interrupt_Mask);
#else
    STOS_InterruptUnlock();
#endif

    /* Can't push command: buffer full */
    return;
} /* end of hdmicom_PushCommand32() function*/
/*******************************************************************************
Name        : StartInternnalTask
Parameters  : Start the internal task

Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success,
              ST_ERROR_ALREADY_INITIALIZED if task already running
              ST_ERROR_BAD_PARAMETER if problem of creation
*******************************************************************************/
ST_ErrorCode_t StartInternalTask(const HDMI_Handle_t Handle)
{
    HDMI_Task_t * const task_p  = &(((HDMI_Data_t *) Handle)->Internaltask);
    HDMI_Task_t * const task2_p = &(((HDMI_Data_t *) Handle)->InterruptTask);
    ST_ErrorCode_t          Error = ST_NO_ERROR;

    if (task_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    if (task2_p->IsRunning)
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* Start the task for the internal state machine */
    task_p->ToBeDeleted = FALSE;
    task2_p->ToBeDeleted = FALSE;
    Error = STOS_TaskCreate ((void (*) (void*))InternalTaskFunction,
                             (void*)Handle,
                             (((HDMI_Data_t *) Handle)->CPUPartition_p),
                             1024*16,
                             &(task_p->TaskStack),
                             (((HDMI_Data_t *) Handle)->CPUPartition_p),
                             &(task_p->Task_p),
                             &(task_p->TaskDesc),
                              STVOUT_TASK_HDCP_PRIORITY,
                              "STVOUT_STATE_MACHINE_INTERNAL_TASK",
                               0 /*task_flags_high_priority_process*/);
    if(Error != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    Error = STOS_TaskCreate ((void (*) (void*))stvout_InterruptProcessTask,
                        (void*)Handle,
                        (((HDMI_Data_t *) Handle)->CPUPartition_p),
                         1024*16,
                        &(task2_p->TaskStack),
                        (((HDMI_Data_t *) Handle)->CPUPartition_p),
                        &(task2_p->Task_p),
                        &(task2_p->TaskDesc),
                        STVOUT_TASK_INTERRUPT_PROCESS_PRIORITY,
                        "STVOUT_INFOFRAME_TASK",
                        0 /*task_flags_high_priority_process*/);
    if(Error != ST_NO_ERROR)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    task_p->IsRunning  = TRUE;
    task2_p->IsRunning  = TRUE;
    STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Internal task created.\n"));
    return(ST_NO_ERROR);
} /* End of StartInternalTask() function */
/*******************************************************************************
Name        : StopInternalTask
Description : Stop the internal task
Parameters  : Hdmi handle
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if no problem,
              ST_ERROR_ALREADY_INITIALIZED if task not running
*******************************************************************************/
ST_ErrorCode_t StopInternalTask(const HDMI_Handle_t Handle)
{
    HDMI_Task_t * const task_p  = &(((HDMI_Data_t *) Handle)->Internaltask);
    HDMI_Task_t * const task2_p = &(((HDMI_Data_t *) Handle)->InterruptTask);
    ST_ErrorCode_t      Err = ST_NO_ERROR;    if (!(task_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* End the function of the task here... */
    task_p->ToBeDeleted = TRUE;
/* Signal semaphore to release task that may wait infinitely if stopped */
    STOS_SemaphoreSignal(((HDMI_Data_t *)Handle)->ChangeState_p);
    STOS_TaskWait( &task_p->Task_p, TIMEOUT_INFINITY );
    Err = STOS_TaskDelete ( task_p->Task_p,
            (((HDMI_Data_t *) Handle)->CPUPartition_p),
            task_p->TaskStack,
            (((HDMI_Data_t *) Handle)->CPUPartition_p));
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    task_p->IsRunning  = FALSE;
    if (!(task2_p->IsRunning))
    {
        return(ST_ERROR_ALREADY_INITIALIZED);
    }
    /* End the function of the task here... */
    task2_p->ToBeDeleted = TRUE;
    STOS_SemaphoreSignal(((HDMI_Data_t *)Handle)->InterruptSem_p);
    STOS_TaskWait( &task2_p->Task_p, TIMEOUT_INFINITY );
    Err = STOS_TaskDelete ( task2_p->Task_p,
            (((HDMI_Data_t *) Handle)->CPUPartition_p),
            task2_p->TaskStack,
            (((HDMI_Data_t *) Handle)->CPUPartition_p));
    if(Err != ST_NO_ERROR)
    {
        return(Err);
    }
    task2_p->IsRunning  = FALSE;
  /* </STFAE - Ben>*/
    return(ST_NO_ERROR);
} /* End of StopInternalTask() function */
/*******************************************************************************
Name        : InternalTaskFunction
Description : Internal task function used for the state machine
Parameters  : Hdmi handle
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
static void  InternalTaskFunction (HDMI_Handle_t const Handle)
{
  STVOUT_State_t NextState;
#ifdef ST_OS21
  osclock_t         MaxWaitOrderTime;
#endif
#ifdef ST_OS20
  clock_t           MaxWaitOrderTime;
#endif
#ifdef ST_OSLINUX
  clock_t           MaxWaitOrderTime;
#endif
  U8                Tmp8;
  HDMI_Data_t *     const HDMI_Data_p = (HDMI_Data_t *)Handle;
  STOS_TaskEnter(NULL);
  HDMI_HANDLE_LOCK(Handle);
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("VM State","Task Start");
#endif

  do
  {

#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("VM State","Task Loop Do");
        TraceMessage("VM State","CellE %c; IsKey %c",
                (HDMI_Data_p->EnableCell)?'Y':'N',
                (HDMI_Data_p->IsKeysTransfered)?'Y':'N');
#endif
        HDMI_HANDLE_UNLOCK(Handle);
        /* At each loop: first check if there are interrupts to process. */
        MaxWaitOrderTime = time_plus(time_now(), ST_GetClocksPerSecond() * 2);
        /* The state machine should wait if no transition was made */
       if(STOS_SemaphoreWaitTimeOut(HDMI_Data_p->ChangeState_p, &MaxWaitOrderTime)!=0)
        {
#if defined(STVOUT_STATE_MACHINE_TRACE)
            TraceState("Sem","Timeout");
#endif
        }
        else
        {
#if defined(STVOUT_STATE_MACHINE_TRACE)
            TraceState("Sem","Signaled");
#endif
        }
#if defined(STVOUT_STATE_MACHINE_TRACE)
if(PIO_Int_Time!=0)
    {
        PIO_Int_Time = time_now() - PIO_Int_Time;
        TraceMessage("Info","PIO Sem Reaction %lu",PIO_Int_Time);
        PIO_Int_Time=0;
    }
#endif
		HDMI_HANDLE_LOCK(Handle);
        if (!(HDMI_Data_p->InternalTaskStarted))
        {
          /* the state machine has not been activated so far */
          continue;
        }
        /* Process all user commands*/
        if (PopControllerCommand(Handle, &Tmp8)==ST_NO_ERROR)
        {
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("VM State","Command Popped");
#endif
          /* For the moment, the task manage the reset controller command only...*/
          switch ((ControllerCommandID_t)Tmp8)
          {
            case CONTROLLER_COMMAND_HPD : /* no break */
#ifdef STVOUT_CEC_PIO_COMPARE
                /* Notify the CEC that the current Adresses are lost, in anycase plug or unplug */
                HDMI_Data_p->CEC_Params.IsLogicalAddressAllocated = FALSE;
                HDMI_Data_p->CEC_Params.IsValidPhysicalAddressObtained = FALSE;
#endif
              break;
            case CONTROLLER_COMMAND_PIX_CAPTURE:
              /* Wake up the state machine */
              HDMI_Data_p->PixelCaptured = TRUE;
              break;
            case CONTROLLER_COMMAND_RESET:
              /* Re-set the software */
              HDMI_SoftReset(Handle);
              break;
            default :
              break;
           }
        }
        NextState = HDMI_ChangeState(Handle);
        if (NextState != HDMI_Data_p->CurrentState)
        {
            /* Update Curent State. */
            HDMI_Data_p->CurrentState = NextState;
            switch (NextState)  /* Check the next state */
            {
                case STVOUT_NO_RECEIVER :
                    /* Receiver is no more detected, reset internal task and cell.  */
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("Exec State","No Receiver");
#endif
                    HDMI_SoftReset(Handle);
                    break;
                case STVOUT_RECEIVER_CONNECTED :
                    /* Retrieve Receiver E-DID tables information , store sink's capabilities */
					/* Check wether DVI/HDMI interface was disabled */
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("Exec State","Receiver Connected");
#endif
                   /* nothing to do....*/
                   break;
				case STVOUT_NO_ENCRYPTION :
                    /* The output is effectively allowed and no HDCP encryption will be managed
                     *  with the currently connected device. */
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("Exec State","No HDCP Re-Enc");
#endif
                   /* nothing to do ...*/
                    break;
                  default :
                    break;
            }
			HDMI_HANDLE_UNLOCK(Handle); /* Allow event handlers to call API functions */
            /* each transition from one state to other implies the sending the following evt to the application*/
            #if defined ST_OSLINUX
            STEVT_Notify(HDMI_Data_p->EventsHandle,
                         HDMI_Data_p->RegisteredEventsID[HDMI_CHANGE_STATE_EVT_ID],
                        STEVT_EVENT_DATA_TYPE_CAST &NextState);
            #else

            STEVT_Notify(HDMI_Data_p->EventsHandle,
                         HDMI_Data_p->RegisteredEventsID[HDMI_CHANGE_STATE_EVT_ID],
                        (const void*) &NextState);
            #endif
    		HDMI_HANDLE_LOCK(Handle);
        }
   }
   while(!(HDMI_Data_p->Internaltask.ToBeDeleted));
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("VM State","Task Stopped");
#endif
   HDMI_HANDLE_UNLOCK(Handle);
   STOS_TaskExit(NULL);
} /* end of InternalTaskFunction()*/
/*******************************************************************************
Name        : HDMI_ChangeState
Description : Manage the different transition from one state to another state
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     : The next state of the internal task
*******************************************************************************/
static STVOUT_State_t HDMI_ChangeState (const HDMI_Handle_t Handle)
{
    HDMI_Data_t * const HDMI_Data_p = (HDMI_Data_t *)Handle;
    STVOUT_State_t      NextState;

    /* Default case, next state is the same as the current one. */
    NextState = HDMI_Data_p->CurrentState;

    /* At first, and whatever the current state of task, check the sink connection. */
    if ( (HDMI_Data_p->CurrentState != STVOUT_NO_RECEIVER) &&
         (!HAL_IsReceiverConnected(HDMI_Data_p->HalHdmiHandle) ) )
    {
        NextState = STVOUT_NO_RECEIVER;
    }
    /* This state is reached from any another state as soon as the HDMI interface was disabled. */
    else if((HDMI_Data_p->CurrentState != STVOUT_NO_RECEIVER) &&
            (!HDMI_Data_p->EnableCell))
    {
        NextState = STVOUT_RECEIVER_CONNECTED;
    }
    else
    {
        switch (HDMI_Data_p->CurrentState)
        {
            case STVOUT_NO_RECEIVER :
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("Change State","No Receiver");
#else
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STVOUT_NO_RECEIVER"));
#endif
                if (HAL_IsReceiverConnected(HDMI_Data_p->HalHdmiHandle))
                {
                    /* Sink's connection. Change state. */
                    NextState = STVOUT_RECEIVER_CONNECTED;

					HDMI_Data_p->HdmiInterrupt.Mask = HDMI_ITX_ENABLE | HDMI_ITX_SW_RESET_ENABLE |
                                  HDMI_ITX_PIX_CAPTURE_ENABLE | HDMI_ITX_HOTPLUG_ENABLE;

					HAL_SetInterruptMask(HDMI_Data_p->HalHdmiHandle, HDMI_Data_p->HdmiInterrupt.Mask);

                }
                break;

           case STVOUT_RECEIVER_CONNECTED :
#if defined(STVOUT_STATE_MACHINE_TRACE)
        TraceState("Change State","Receiver Connected");
#else
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STVOUT_RECEIVER_CONNECTED\n"));
#endif
               /* Sink deconnection is managed at first in this function.  */
               /* Next stage should be :                                   */
               /*      - Wait for enable of stvout (via STVOUT_Enable()    */
               if (HDMI_Data_p->EnableCell)
               {

                   NextState = STVOUT_NO_ENCRYPTION;

               }
               else
               {
                   /* wait until interface will be enabled */
                   NextState = STVOUT_RECEIVER_CONNECTED;
               }
               break;
                  case STVOUT_NO_ENCRYPTION :
			   /* Check Whether there is a transition to the Encryption mode or not*/
#if defined(STVOUT_STATE_MACHINE_TRACE)
                   TraceState("Change State","No Encryption");
#else
                   STTBX_Report((STTBX_REPORT_LEVEL_INFO,"STVOUT_NO_ENCRYPTION\n"));
#endif
               if (HDMI_Data_p->EnableCell)
               {

                   NextState = STVOUT_NO_ENCRYPTION;

               }
               else
               {
                   /* wait until interface will be enabled */
                   NextState = STVOUT_RECEIVER_CONNECTED;
               }
               break;
                  default :
                break;
       }
    }
    if (HDMI_Data_p->CurrentState!= NextState)
    {
        /* Wake up the state machine */
         STOS_SemaphoreSignal(HDMI_Data_p->ChangeState_p);
    }
    /* Return status of state changes.      */
    return(NextState);

} /* end of HDMI_ChangeState()*/
/*******************************************************************************
Name        : HDMI_SoftReset
Description : Force the state machine to reset state, init driver software
              and resets all DVI/HDMI registers ans wait the it after soft reset
              completetion.
Parameters  : HDMI handle
Assumptions :
Limitations :
Returns     :nothing
*******************************************************************************/
void HDMI_SoftReset (const HDMI_Handle_t Handle)
{
        HDMI_Data_t * const Data_p =  (HDMI_Data_t*) Handle;

        /* Disable all interrupts*/
        HAL_SetInterruptMask(Data_p->HalHdmiHandle, HDMI_ITX_DISABLE_ALL);

        /* Clear all interrupts */
        HAL_ClearInterrupt(Data_p->HalHdmiHandle, HDMI_INTERRUPT_CLEAR_ALL);

        /* Force the state machine to reset state*/
        Data_p->InternalState = HDMI_RUNNING;
        Data_p->CurrentState = STVOUT_NO_RECEIVER;
        Data_p->Internaltask.IsRunning = TRUE;
        Data_p->Internaltask.ToBeDeleted = FALSE;

        /* Initialise the interrupts  */
        Data_p->HdmiInterrupt.Mask = 0;
        Data_p->HdmiInterrupt.IsInside = FALSE;
        Data_p->HdmiInterrupt.EnableCount = 0;
        Data_p->IsInfoFramePending = FALSE;
        Data_p->InfoFrameFuse = 0;
        Data_p->InfoFrameStatus = 0;

        /* Initialise interrupts queue */
        Data_p->InterruptsBuffer.Data_p = Data_p->InterruptsData;
        Data_p->InterruptsBuffer.TotalSize = sizeof(Data_p->InterruptsData) / sizeof(U32);
        Data_p->InterruptsBuffer.BeginPointer_p = Data_p->InterruptsBuffer.Data_p;
        Data_p->InterruptsBuffer.UsedSize = 0;
        Data_p->InterruptsBuffer.MaxUsedSize = 0;

        /* Initialise Commands queue */
        Data_p->CommandsBuffer.Data_p = Data_p->CommandsData;
        Data_p->CommandsBuffer.TotalSize = sizeof(Data_p->CommandsData);
        Data_p->CommandsBuffer.BeginPointer_p = Data_p->CommandsBuffer.Data_p;
        Data_p->CommandsBuffer.UsedSize = 0;
        Data_p->CommandsBuffer.MaxUsedSize = 0;

        Data_p->AVMute = TRUE; /* at reset mute AV */

#if defined(STVOUT_HDCP_PROTECTION)
        Data_p->IsKeysTransfered  =FALSE;
        memset((void*)Data_p->RepeaterKSVFifo, 0, sizeof(U8)*64*11);
	    memset((void*)&Data_p->HDCPSinkParams, 0, sizeof(STVOUT_HDCPSinkParams_t));
	    memset((void*)Data_p->HashResult, 0 , sizeof(U32)*5);
	    Data_p->RepeaterBStatus =0;
#endif

        /* Enable the hotplug interrupt*/
        Data_p->HdmiInterrupt.Mask = HDMI_ITX_HOTPLUG_ENABLE | HDMI_ITX_ENABLE;
		HAL_SetInterruptMask(Data_p->HalHdmiHandle, Data_p->HdmiInterrupt.Mask);

} /* end of HDMI_SoftReset() function*/
/* end of hdmi.c file */
/* ------------------------------- End of file ---------------------------- */

