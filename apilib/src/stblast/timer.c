/*****************************************************************************
File Name   : timer.c

Description : Timer support functions for UART.

Copyright (C) 2000 STMicroelectronics

History     :

    08/07/03  Added TaskName Identificable(DDTS 20059)

    30/11/99  Migrated timer functions to separate module.

    07/12/99  Bug fix to blocking mode of timer - the TIMER_RUNNING flag
              was not being set.

    9/12/99   Added task priority option for creating timer.
    13/1/00   Fixed race condition on write accesses to the timer
              status flags.

    30/3/00   Renamed functions to avoid conflicts with global namespace.

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if !defined(ST_OSLINUX) || defined(LINUX_FULL_USER_VERSION)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#if defined(ST_OSLINUX)
#include <linux/sched.h>
#include <linux/param.h>
#else
#include "stlite.h"                     /* Standard includes */
#endif

#include "stddefs.h"
#include "stos.h"

#if !defined(ST_OSLINUX)
#include "sttbx.h"                      /* STAPI includes */
#endif
#include "stcommon.h"

#include "timer.h"                      /* Private includes */
#include "blastint.h"

/* Private types/constants ------------------------------------------------ */

/* Private Functions Prototypes ------------------------------------------- */

#if defined(ST_OSLINUX)
static void stblast_TimerTask(void *Timer_pp);
#else
static void stblast_TimerTask(Timer_t *Timer_p);
#endif
extern void BLAST_QueueDump(void);

/* Routines (alphabetical order) ------------------------------------------ */

/*****************************************************************************
stblast_TimerTask()
Description:
    This routine is passed to task_create() when creating a timer task that
    provides the timer/callback functionality.
    The timer task may be in two states:

    TIMER_IDLE: the timer is blocked on the guard semaphore.
    TIMER_RUNNING: the timer is blocked on the timeout semaphore.

    NOTE: A timer may only be deleted when in TIMER_IDLE.

Parameters:
    param,  pointer to task function's parameters - this should be a pointer
            to a Timer_t object.

Return Value:
    Nothing.
See Also:
    stblast_TimerCreate()
    stblast_TimerDelete()
*****************************************************************************/
#if defined(ST_OSLINUX)
static void stblast_TimerTask(void *Timer_pp)
#else
static void stblast_TimerTask(Timer_t *Timer_p)
#endif
{
    STOS_Clock_t clk;

#if defined(ST_OSLINUX)
    Timer_t *Timer_p;
    Timer_p = (Timer_t *)Timer_pp;

#endif /* ST_OSLINUX && LINUX_FULL_KERNEL_VERSION */
    
    STOS_TaskEnter(Timer_p);
    /* Flag the timer as idle, as it blocks on the idle semaphore */
    Timer_p->Status = TIMER_IDLE;

    for(;;)
    {

        /* The guard semaphore allows a task to sleep and represents
         * the timer's idle state.  Whenever the timer is started or
         * is to be deleted, this semaphore will be signalled.
         */
        STOS_SemaphoreWait(Timer_p->GuardSemaphore_p);

        /* If timer delete has been called, then this flag will
         * be set to true and we should exit from this routine to
         * allow the task to be cleaned up.
         */
        if(Timer_p->DeleteTimer)
        {
            break;
        }
        do
        {
            /* If the timeout period is zero, then the user wishes
             * to wait indefinately.
             */
            if(*Timer_p->Timeout == 0)
            {
                /* Block until signalled */
                Timer_p->TimeoutError =
                   STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                          TIMEOUT_INFINITY);
            }
            else
            {
                /* Block for defined period */
                clk = STOS_time_plus(STOS_time_now(), *Timer_p->Timeout);
                Timer_p->TimeoutError =
                   STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                          &clk);
            }

            /* The timer may be retriggered in two circumstances:
             *
             * a) The timer has been paused - no further activity should
             *    take place until the timer is resumed.
             * b) A restart has been requested on the timer.
             */
            if((Timer_p->Status & TIMER_PAUSED) != 0)
            {
                STOS_SemaphoreWait(Timer_p->GuardSemaphore_p);
            }
            if(Timer_p->TimeoutError == TIMER_TIMEDOUT)
            {
                break;
            }
        }while((Timer_p->Status & TIMER_ABORT) == 0);

        if((Timer_p->Status & TIMER_ABORT) != 0)
        {
            Timer_p->TimeoutError = TIMER_SIGNALLED;
        }
        /* Set the timer to idle - this allows back-to-back
         * operations to take place from inside a callback.
         */
        Timer_p->Status = TIMER_IDLE;

        /* Call the callback, if there is one */
        if(Timer_p->CallbackRoutine)
        {
            Timer_p->CallbackRoutine(Timer_p->CallbackParam);
        }
    }
    STOS_TaskExit(Timer_p); 

} /* stblast_TimerTask() */

/*****************************************************************************
stblast_TimerCreate()
Description:
    This routine creates two semaphores and a task, required for operating
    the timer mechanism, as the basis for initializing the user's
    timer structure.
    Once initialized, the user may use their timer with TimerWait,
    TimerSignal.  TimerDelete is used to free up resources tied
    with the timer.
Parameters:
    Timer_p,        pointer to a timer structure to initialize.
    TaskPriority,   OS-specific priority for timer task.
Return Value:
    FALSE,                  timer successfully created.
    TRUE,                   error creating the timer task.
See Also:
    stblast_TimerDelete()
*****************************************************************************/

BOOL stblast_TimerCreate(const ST_DeviceName_t DeviceName,Timer_t *Timer_p, U32 TaskPriority)
{
    BOOL error = FALSE;
    BLAST_ControlBlock_t *Blaster_p;
     
    char TaskName[18+ST_MAX_DEVICE_NAME+1]; /*strlen(STBLAST_TimerTask_)=18 and max(strlen (device name)=16 */
  
    sprintf(TaskName,"STBLAST_TimerTask_%s",DeviceName);

    /* Create the timer idle semaphore required to control the timer task */
	Timer_p->GuardSemaphore_p   =  STOS_SemaphoreCreateFifo(NULL,0);
    /* Create the timeout semaphore */
    Timer_p->TimeoutSemaphore_p =  STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    
    /* Obtain the control block associated with the device name */
    Blaster_p = BLAST_GetControlBlockFromName(DeviceName);
   	
   	/* Create and start the timer task */
	error =  STOS_TaskCreate((void(*)(void *))stblast_TimerTask,
	                                       Timer_p,
	                                       Blaster_p->InitParams.DriverPartition,
	                                       STBLAST_TIMER_STACK_SIZE,
	                                       &Timer_p->TaskStack_p,
	                                       Blaster_p->InitParams.DriverPartition,
	                                   	   &Timer_p->TimerTask,
	                                       &Timer_p->TimerTaskDescriptor,
	                                       (int)TaskPriority,
	                                       TaskName,
	                                       (task_flags_t)0 );
    if(error != 0)
    {
        error = TRUE;
    }
    else
    {
        /* Ensure the delete timer flag is false, otherwise the
         * timer task may end prematurely.
         */
        Timer_p->DeleteTimer = FALSE;
    }
    return error;
} /* stblast_TimerCreate() */

/*****************************************************************************
stblast_TimerWait()
Description:
    This routine starts a time out (blocking or non-blocking) for a given
    period of clock ticks.

    NOTE: This routine does nothing unless the timer is idle.

Parameters:
    Timer_p,            pointer to an initialized timer structure.
    CallbackRoutine,    pointer to a function that take's a "void *" parameter
                        to be called when the timer times-out or is signalled.
                        NULL indicates no callback is required.
    CallbackParam,      the parameter passed to the callback routine.
                        Use NULL if NULL was used above.
    Timeout,            period to delay in clock ticks (0 => wait forever).
    IsBlocking,         if TRUE the calling task will block during the timeout
                        period.
Return Value:
    Nothing.
See Also:
    stblast_TimerSignal()
*****************************************************************************/

void stblast_TimerWait(Timer_t *Timer_p,
                      void (*CallbackRoutine)(void *),
                      void *CallbackParam,
                      const STOS_Clock_t *Timeout,
                      BOOL IsBlocking
                      )
{
    /* Copy the timeout pointer to the timer */
    Timer_p->Timeout = Timeout;

    /* Only begin a timer if the timer is idle - the signal routine can
     * be used to restart timers.
     */
    if((Timer_p->Status & TIMER_RUNNING) == 0)
    {
        /* This may be a blocking or non-blocking timeout */
        if(IsBlocking)
        {
            /* The user wishes to block, so we call wait on the timed
             * semaphore in this routine instead of the the timer
             * task.
             */

            STOS_InterruptLock(); /* Must be atomic to avoid race conditions */

            Timer_p->Status |= TIMER_RUNNING; /* Flag timer as running */

            STOS_InterruptUnlock();

            do
            {
                STOS_Clock_t clk;

                /* If the timeout period is zero, then the user wishes
                 * to wait indefinately.
                 */
                if(*Timer_p->Timeout == 0)
                {
                    /* Block until signalled */
                    Timer_p->TimeoutError =
                       STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                              TIMEOUT_INFINITY);
                }
                else
                {
                    /* Block for defined period */
                    clk = STOS_time_plus(STOS_time_now(), *Timer_p->Timeout);
                    Timer_p->TimeoutError =
                       STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                              &clk);
                }

                /* The timer may be retriggered in two circumstances:
                 *
                 * a) The timer has been paused - no further activity should
                 *    take place until the timer is resumed.
                 * b) A restart has been requested on the timer (default).
                 */
                if((Timer_p->Status & TIMER_PAUSED) != 0)
                {
                    STOS_SemaphoreWait(Timer_p->GuardSemaphore_p);
                }
                if(Timer_p->TimeoutError == TIMER_TIMEDOUT)
                {
                    break;
                }
            }while((Timer_p->Status & TIMER_ABORT) == 0);

            if((Timer_p->Status & TIMER_ABORT) != 0)
                Timer_p->TimeoutError = TIMER_SIGNALLED;

            /* Set the timer to idle - this allows back-to-back
             * operations to take place from inside a callback.
             */
            Timer_p->Status = TIMER_IDLE;

            /* Call the callback, if there is one */
            if(CallbackRoutine)
            {
                CallbackRoutine(CallbackParam);
            }
        }
        else
        {
            /* The timer task should take care of the rest of this request.
             * Signal the idle semaphore to wake it up and return to the
             * caller.
             */
            Timer_p->CallbackRoutine = CallbackRoutine;
            Timer_p->CallbackParam = CallbackParam;

            STOS_InterruptLock(); /* Update to status must be atomic */

            Timer_p->Status |= TIMER_RUNNING; /* Flag timer as running */

            STOS_InterruptUnlock();

            STOS_SemaphoreSignal(Timer_p->GuardSemaphore_p);
        }
    }
} /* stblast_TimerWait() */

/*****************************************************************************
stblast_TimerSignal()
Description:
    This routine will signal a running timer i.e., a task blocked on the
    timer may unblock.

    If the signal routine is called RestartTimer set to true, then the
    timeout will restart for another period, otherwise (if specified) the
    callback routine will be invoked.

    NOTE: if the timer is not in the Timer_RUNNING state then this
    routine does nothing.

Parameters:
    Timer_p,        pointer to an initialized timer structure.
    RestartTimer,   if set to TRUE then the timer will restart for another
                    period.
Return Value:
    Nothing.
See Also:
    stblast_TimerWait()
*****************************************************************************/

void stblast_TimerSignal(Timer_t *Timer_p, BOOL RestartTimer)
{
    /* If this is not a restart request, then we should set the abort
     * flag on the timer.  This ensures it will end.
     */
    if(!RestartTimer)
    {
        STOS_InterruptLock();  /* Update to status must be atomic */

        Timer_p->Status |= TIMER_ABORT;

        STOS_InterruptUnlock();
        STOS_SemaphoreSignal(Timer_p->TimeoutSemaphore_p);
    } else

    /* The timer must be running in order to signal it */
    if((Timer_p->Status & TIMER_RUNNING) != 0)
    {
        /* The timer is running, so wakeup the timer */
        STOS_SemaphoreSignal(Timer_p->TimeoutSemaphore_p);
    }
} /* stblast_TimerSignal() */

/*****************************************************************************
stblast_TimerPause()
Description:
    This routine allows a timer to be paused for an indefinite period
    of time.
    Whilst the timer is paused, no further activity can take place on
    the timer e.g., calls to signal/delete will do nothing.
    The timer will resume normal operation after a call to this routine
    with the Pause flag set to FALSE.

    NOTE: if the timer is not in the Timer_RUNNING state then this
    routine does nothing.

Parameters:
    Timer_p,        pointer to an initialized timer structure.
    Pause,          if set to TRUE then the timer will be paused otherwise
                    it will be resumed.
Return Value:
    Nothing.
See Also:
    Nothing.
*****************************************************************************/

void stblast_TimerPause(Timer_t *Timer_p, BOOL Pause)
{
    /* The timer must be running in order to pause it */
    if(Pause && (Timer_p->Status & TIMER_RUNNING) != 0)
    {
        /* This forces the timer to pause when it wakes up */

        STOS_InterruptLock();   /* Update to status must be atomic */

        Timer_p->Status |= TIMER_PAUSED;

        STOS_InterruptUnlock();

        /* The timer is running, so wakeup the timer */
        STOS_SemaphoreSignal(Timer_p->TimeoutSemaphore_p);
    }
    else if(!Pause && (Timer_p->Status & TIMER_PAUSED) != 0)
    {
        /* The timer is sleeping on the guard semaphore, so wakeup the
         * guard - the timer should resume as normal now.
         */
        STOS_InterruptLock();   /* Update to status must be atomic */

        Timer_p->Status &= ~TIMER_PAUSED;

        STOS_InterruptUnlock();
        STOS_SemaphoreSignal(Timer_p->GuardSemaphore_p);
    }
} /* stblast_TimerPause() */

/*****************************************************************************
stblast_TimerDelete()
Description:
    This routine cleans up all resources associated with an initialized
    timer.

    NOTE: this routine will block until the timer task has been deleted.
    It is advised that a call to signal is made prior to calling delete,
    otherwise you could potentially end up blocking forever e.g., a timer
    whose timeout is zero.

Parameters:
    Timer_p, pointer to an initialized timer structure to delete.
Return Value:
    FALSE,      no errors
    TRUE,       problem deleting timer.
See Also:
    stblast_TimerCreate()
*****************************************************************************/

BOOL stblast_TimerDelete(const ST_DeviceName_t DeviceName,Timer_t *Timer_p)
{
    BOOL error = FALSE;
    BLAST_ControlBlock_t *Blaster_p;
    /* Flag the timer as waiting to be deleted */
    Timer_p->DeleteTimer = TRUE;
 
    STOS_SemaphoreSignal(Timer_p->GuardSemaphore_p);
    /* Obtain the control block associated with the device name */
    Blaster_p = BLAST_GetControlBlockFromName(DeviceName);
    /* The task should be in the process of returning, but wait for
     * it first before deleting it. */
    if(STOS_TaskWait(&Timer_p->TimerTask,TIMEOUT_INFINITY) == 0)
    {
        STOS_TaskDelete(Timer_p->TimerTask,
                        (partition_t *)Blaster_p->InitParams.DriverPartition,
                         Timer_p->TaskStack_p,
                        (partition_t *)Blaster_p->InitParams.DriverPartition);
        
        /* Delete all semaphores associated with the timer */
        STOS_SemaphoreDelete(NULL,Timer_p->GuardSemaphore_p);
        STOS_SemaphoreDelete(NULL,Timer_p->TimeoutSemaphore_p);
    }

    /* Common exit point */
    return error;
} /* stblast_TimerDelete() */

/*****************************************************************************
Debug/test routines
*****************************************************************************/

#ifdef stblast_DEBUG

/*****************************************************************************
Test functions
*****************************************************************************/

/*****************************************************************************
stblast_TestCallback()
Description:
    Callback routine invoked by the timer tests.

Parameters:
    Param,  a pointer to a semaphore for notifying completion of this
            callback routine.
Return Value:
    Nothing.
See Also:
    stblast_TestTimer().
*****************************************************************************/

static void stblast_TestCallback(semaphore_t *Param)
{
    STBLAST_Print(("Callback stblast_TestCallback() invoked!"));
    STOS_SemaphoreSignal(Param);
}

/*****************************************************************************
stblast_TestTimer()
Description:
    Runs through a sequence of tests for stblast_TimerXXX() routines.
Parameters:
    Ref,    a unique section number for this test.
Return Value:
    Pass/fail counts via UART_TestResult_t.
See Also:
    Nothing.
*****************************************************************************/

void TestTimer(void)
{
    BOOL Error;
    Timer_t Timer;
    STOS_Clock_t Timeout;

    STBLAST_Print(("stblast_TimerCreate()\n"));
    STBLAST_Print(("Purpose: to ensure a timer can be created."));

    /* Create the timer to perform our tests with */
    Error = TimerCreate(&Timer);

    if(!Error)
    {
        Timeout = (STOS_Clock_t)ST_GetClocksPerSecond()*5;

        /* Test blocking timer */
        STBLAST_Print(("stblast_TimerWait()\n"));
        STBLAST_Print(("Blocking timer (5s)\n"));
        STBLAST_Print(("Params: timeout=5s blocking=true callback=null"));
        STBLAST_Print(("Purpose: To ensure a timer wait will block for"
                          " the designated number of clocks."));
        TimerWait(&Timer,NULL,NULL,&Timeout,TRUE);

        /* Test non-blocking timer */
        STBLAST_Print(("Non-blocking timer (5s)\n"));
        STBLAST_Print(("Params: timeout=5s blocking=false"
                          " callback=stblast_TestCallback()"));
        STBLAST_Print(("Purpose: To ensure a non-blocking timer invokes"
                          " its callback after the designated number of clocks."));

        /* Kick off the timer */
        TimerWait(&Timer,
                       (void(*)(void *))TestCallback,
                       &Timer.GuardSemaphore,
                       &Timeout,
                       FALSE);

        /* Wait to allow the callback to complete */
        STBLAST_Print(("Waiting for stblast_TestCallback()..."));
        STOS_SemaphoreWait(Timer.GuardSemaphore_p);

        /* Test non-blocking timer signalled */
        STBLAST_Print(("stblast_TimerSignal()\n"));
        STBLAST_Print(("Signal without restart (after 5s)\n"));
        STBLAST_Print(("Params: timeout=10s blocking=false"
                          " callback=stblast_TestCallback()"));
        STBLAST_Print(("Purpose: To ensure a timer is awoken when"
                          " it is signalled."));

        /* 10s delay callback  */
        Timeout = (STOS_Clock_t)ST_GetClocksPerSecond()*10;

        /* Kick of the timer */
        TimerWait(&Timer,
                  (void(*)(void *))TestCallback,
                  &Timer.GuardSemaphore,
                  &Timeout,
                  FALSE);

        /* Sleep for a while to allow the callback to complete */
        STBLAST_Print(("Waiting 5s and then signalling"
                          " stblast_TestCallback()..."));
        STOS_TaskDelay(ST_GetClocksPerSecond()*5);

        STBLAST_Print(("Calling stblast_TimerSignal()."));
        TimerSignal(&Timer, FALSE);
        STOS_SemaphoreWait(Timer.GuardSemaphore_p);

        /* Testing signal with restart */
        STBLAST_Print(("Signal with restart (after 5s)\n"));
        STBLAST_Print(("Params: timeout=10s blocking=false"
                          " callback=stblast_TestCallback()"));
        STBLAST_Print(("Purpose: To ensure a timer is restarted when"
                          " it is signalled with restart."));

        /* Kick off the timer */
        TimerWait(&Timer,
                  (void(*)(void *))TestCallback,
                  &Timer.GuardSemaphore,
                  &Timeout,
                  FALSE);

        /* Sleep for a while to allow the callback to complete */
        STBLAST_Print(("Waiting 5s and then restarting"
                          " stblast_TestCallback()..."));
        STOS_TaskDelay(ST_GetClocksPerSecond()*5);

        STBLAST_Print(("Calling stblast_TimerSignal() with restart."));
        TimerSignal(&Timer, TRUE);
        STOS_SemaphoreWait(Timer.GuardSemaphore_p);

        /* Testing pause facility */
        STBLAST_Print(("stblast_TimerPause()\n"));
        STBLAST_Print(("Params: timeout=10s blocking=false"
                          " callback=stblast_TestCallback()"));
        STBLAST_Print(("Purpose: To ensure a timer pauses and"
                          " does not timeout whilst paused."));

        /* Kick off the timer */
        TimerWait(&Timer,
                  (void(*)(void *))TestCallback,
                  &Timer.GuardSemaphore,
                  &Timeout,
                  FALSE);

    /* Sleep for a while to allow the callback to complete */
        STBLAST_Print(("Waiting 5s and then pausing"
                          " stblast_TestCallback()..."));
        STOS_TaskDelay(ST_GetClocksPerSecond()*5);

        STBLAST_Print(("Calling stblast_TimerPause() to pause."));
        TimerPause(&Timer, TRUE);

        STBLAST_Print(("Waiting 10s and then resuming"
                          " stblast_TestCallback()..."));
        STOS_TaskDelay(ST_GetClocksPerSecond()*10);

        STBLAST_Print(("Calling stblast_TimerPause() to resume."));
        TimerPause(&Timer, FALSE);
        STOS_SemaphoreWait(Timer.GuardSemaphore_p);

        /* Testing delete timer */
        STBLAST_Print(("stblast_TimerDelete()\n"));
        STBLAST_Print(("Purpose: To ensure a timer can be deleted and"
                          " that all resources are freed."));
        Error = TimerDelete(&Timer);
        if(Error)
        {
            STBLAST_Print(("stblast_TimerDelete() failed"));
        }
    }
    else
    {
        /* Couldn't create a timer */
        STBLAST_Print(("stblast_TimerCreate() failed"));
    }
}

#endif /* stblast_DEBUG */

/* End of timer.c */
