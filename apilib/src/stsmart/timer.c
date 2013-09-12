/*****************************************************************************
File Name   : timer.c

Description : Timer support functions for UART.

Copyright (C) 2006 STMicroelectronics

History     :

Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#if defined(ST_OS20) || defined(ST_OS21)
#include <string.h>                     /* C libs */
#include <stdio.h>
#endif

#ifdef ST_OSLINUX
#include "linux/kthread.h"
#include "compat.h"
#else
#include "stlite.h"
#endif
#include "sttbx.h"
#include "stsys.h"

#include "stos.h"
#include "stcommon.h"
#include "stddefs.h"

#include "timer.h"                      /* Private includes */

#if defined( ST_OSLINUX )

#endif

/* Private types/constants ------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */

/* Private Functions Prototypes ------------------------------------------- */
#ifdef ST_OSLINUX
static void stuart_TimerTask(void *Timer_pp);
#else
static void stuart_TimerTask(Timer_t *Timer_p);
#endif
static int timerTaskName = 0;
/* Routines (alphabetical order) ------------------------------------------ */

/*****************************************************************************
stuart_TimerTask()
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
    stuart_TimerCreate()
    stuart_TimerDelete()
*****************************************************************************/
#if defined(ST_OSLINUX)&&defined(LINUX_FULL_KERNEL_VERSION)
static void stuart_TimerTask(void *Timer_pp)
#else
static void stuart_TimerTask(Timer_t *Timer_p)
#endif
{
#ifdef ST_OSLINUX
    Timer_t *Timer_p;

    Timer_p = (Timer_t *)Timer_pp;
    if(timerTaskName==0){
        strcpy(current->comm, "stuart_TimerTask_0");
        timerTaskName++;
    }else{
        strcpy(current->comm, "stuart_TimerTask_1");
    }
#endif

    STOS_TaskEnter(NULL);
    for (;;)
    {
        STOS_Clock_t clk;

        /* The guard semaphore allows a task to sleep and represents
         * the timer's idle state.  Whenever the timer is started or
         * is to be deleted, this semaphore will be signalled.
         */
        STOS_SemaphoreWait(Timer_p->GuardSemaphore_p);

        /* If timer delete has been called, then this flag will
         * be set to true and we should exit from this routine to
         * allow the task to be cleaned up.
         */
        if (Timer_p->DeleteTimer)
        {
            break;
        }



        do
        {
            /* If the timeout period is zero, then the user wishes
             * to wait indefinately.
             */
            if (*Timer_p->Timeout == 0)
            {
                /* Block until signalled */
                Timer_p->TimeoutError =
                   STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                                    TIMEOUT_INFINITY
                    );
            }
            else
            {
                /* Block for defined period */
                clk = STOS_time_plus(STOS_time_now(), *Timer_p->Timeout);
                Timer_p->TimeoutError =
                   STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                                    &clk
                    );
            }

            /* The timer may be retriggered in two circumstances:
             *
             * a) The timer has been paused - no further activity should
             *    take place until the timer is resumed.
             * b) A restart has been requested on the timer.
             */
            if ((Timer_p->Status & TIMER_PAUSED) != 0)
            {
                STOS_SemaphoreWait(Timer_p->GuardSemaphore_p);
            }
            if (Timer_p->TimeoutError == TIMER_TIMEDOUT)
            {
                break;
            }

        } while ((Timer_p->Status & TIMER_ABORT) == 0);

        if ((Timer_p->Status & TIMER_ABORT) != 0)
        {
            Timer_p->TimeoutError = TIMER_SIGNALLED;
        }

        /* Set the timer to idle - this allows back-to-back
         * operations to take place from inside a callback.
         */
        Timer_p->Status = TIMER_IDLE;

        /* Call the callback, if there is one */
        if (Timer_p->CallbackRoutine)
        {
            Timer_p->CallbackRoutine(Timer_p->CallbackParam);
        }
    }
    /* Don't stop without permission */
    STOS_TaskExit(NULL);

} /* stuart_TimerTask() */

/*****************************************************************************
stuart_TimerCreate()
Description:
    This routine creates two semaphores and a task, required for operating
    the timer mechanism, as the basis for initializing the user's
    timer structure.
    Once initialized, the user may use their timer with TimerWait,
    TimerSignal.  TimerDelete is used to free up resources tied
    with the timer.
Parameters:
    DeviceName,     DeviceName specifies which STUART is being used.
    Timer_p,        pointer to a timer structure to initialize.
    TaskPriority,   OS-specific priority for timer task.
Return Value:
    FALSE,                  timer successfully created.
    TRUE,                   error creating the timer task.
See Also:
    stuart_TimerDelete()
*****************************************************************************/

BOOL stuart_TimerCreate(ST_DeviceName_t DeviceName,ST_Partition_t *DriverPartition,Timer_t *Timer_p, U32 TaskPriority)
{
    BOOL error = FALSE;

    /* To identify the owning STUART instance.Action against the DDTS 20066 */
    char TimerDeviceName[MAX_TIMER_DEVICE_NAME];

    /* Create the timer idle semaphore required to control the timer task */
    Timer_p->GuardSemaphore_p = STOS_SemaphoreCreateFifo(NULL,0);
    /* Create the timeout semaphore */
    Timer_p->TimeoutSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);

    /* Set up data members that are used immediately */
    Timer_p->Status = TIMER_IDLE;
    Timer_p->DeleteTimer = FALSE;

    strcpy(TimerDeviceName,DeviceName);
    strcat(TimerDeviceName,TimerDeviceSuffix);

    error = STOS_TaskCreate((void(*)(void *))stuart_TimerTask,
                                        Timer_p,
                                        DriverPartition,
                                        TIMER_STACK_SIZE,
                                        &Timer_p->TimerTaskStack_p,
                                        DriverPartition,
                                        &Timer_p->TimerTask_p,
                                        &Timer_p->TimerTaskDescriptor,
                                        TaskPriority,
                                        TimerDeviceName,
                                        (task_flags_t)0);
    if(error != ST_NO_ERROR)
    {
        error = TRUE;
    }

    return error;
} /* stuart_TimerCreate() */

/*****************************************************************************
stuart_TimerWait()
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
    stuart_TimerSignal()
*****************************************************************************/

void stuart_TimerWait(Timer_t *Timer_p,
                      void (*CallbackRoutine)(void *),
                      void *CallbackParam,
                      const STOS_Clock_t *Timeout,
                      BOOL IsBlocking
                      )
{
    /* Copy the timeout pointer to the timer (JF: outside condition?) */
    Timer_p->Timeout = Timeout;

    /* Only begin a timer if the timer is idle - the signal routine can
     * be used to restart timers.
     */
    if ((Timer_p->Status & TIMER_RUNNING) == 0)
    {
        /* This may be a blocking or non-blocking timeout */
        if (IsBlocking)
        {
            /* The user wishes to block, so we call wait on the timed
             * semaphore in this routine instead of the timer task.
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
                if (*Timer_p->Timeout == 0)
                {
                    /* Block until signalled */
                    Timer_p->TimeoutError =
                       STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                                 TIMEOUT_INFINITY
                        );
                }
                else
                {
                    /* Block for defined period */
                    clk = STOS_time_plus(STOS_time_now(), *Timer_p->Timeout);
                    Timer_p->TimeoutError =
                       STOS_SemaphoreWaitTimeOut(Timer_p->TimeoutSemaphore_p,
                                                 &clk
                        );
                }

                /* The timer may be retriggered in two circumstances:
                 *
                 * a) The timer has been paused - no further activity should
                 *    take place until the timer is resumed.
                 * b) A restart has been requested on the timer (default).
                 */
                if ((Timer_p->Status & TIMER_PAUSED) != 0)
                {
                    STOS_SemaphoreWait(Timer_p->GuardSemaphore_p);
                }

                if (Timer_p->TimeoutError == TIMER_TIMEDOUT)
                {
                    break;
                }

            } while ((Timer_p->Status & TIMER_ABORT) == 0);

            if ((Timer_p->Status & TIMER_ABORT) != 0)
            {
                Timer_p->TimeoutError = TIMER_SIGNALLED;
            }

            /* Set the timer to idle - this allows back-to-back
             * operations to take place from inside a callback.
             * JF: Note assumption that a new timerWait won't be
             * called at least until we enter the callback
             */
            Timer_p->Status = TIMER_IDLE;

            /* Call the callback, if there is one */
            if (CallbackRoutine)
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
            Timer_p->CallbackParam   = CallbackParam;

            STOS_InterruptLock(); /* Update to status must be atomic */
            Timer_p->Status |= TIMER_RUNNING; /* Flag timer as running */
            STOS_InterruptUnlock();

            STOS_SemaphoreSignal(Timer_p->GuardSemaphore_p);
        }
    }
} /* stuart_TimerWait() */

/*****************************************************************************
stuart_TimerSignal()
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
    stuart_TimerWait()
*****************************************************************************/

void stuart_TimerSignal(Timer_t *Timer_p, BOOL RestartTimer)
{

     /* If this is not a restart request, then we should set the abort
     * flag on the timer.  This ensures it will end.
     */
    if (!RestartTimer)
    {
        STOS_InterruptLock();  /* Update to status must be atomic */
        Timer_p->Status |= TIMER_ABORT;
        STOS_InterruptUnlock();

        /* The timer is running, so wakeup the timer */
        STOS_SemaphoreSignal(Timer_p->TimeoutSemaphore_p);

    }

    /*The timer must be running in order to signal it */
    if ((Timer_p->Status & TIMER_RUNNING) != 0)
    {
        /*The timer is running, so wakeup the timer */
        STOS_SemaphoreSignal(Timer_p->TimeoutSemaphore_p);
    }

} /* stuart_TimerSignal() */

/*****************************************************************************
stuart_TimerPause()
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

void stuart_TimerPause(Timer_t *Timer_p, BOOL Pause)
{
    /* The timer must be running in order to pause it */
    if (Pause && (Timer_p->Status & TIMER_RUNNING) != 0)
    {
        /* This forces the timer to pause when it wakes up */
        STOS_InterruptLock();   /* Update to status must be atomic */
        Timer_p->Status |= TIMER_PAUSED;
        STOS_InterruptUnlock();

        /* The timer is running, so wakeup the timer */
        STOS_SemaphoreSignal(Timer_p->TimeoutSemaphore_p);
    }
    else if (!Pause && (Timer_p->Status & TIMER_PAUSED) != 0)
    {
        /* The timer is sleeping on the guard semaphore, so wakeup the
         * guard - the timer should resume as normal now.
         */
        STOS_InterruptLock();   /* Update to status must be atomic */
        Timer_p->Status &= ~(U32)TIMER_PAUSED;
        STOS_InterruptUnlock();

        STOS_SemaphoreSignal(Timer_p->GuardSemaphore_p);
    }

    /* JF: During Pause, signals of TimeoutSemaphore will queue up, but response
      is delayed because the task is actually waiting on GuardSemaphore */
} /* stuart_TimerPause() */

/*****************************************************************************
stuart_TimerDelete()
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
    stuart_TimerCreate()
*****************************************************************************/

BOOL stuart_TimerDelete(Timer_t *Timer_p,ST_Partition_t *DriverPartition)
{
    BOOL error = FALSE;

    /* Flag the timer as waiting to be deleted */
    Timer_p->DeleteTimer = TRUE;
    STOS_SemaphoreSignal(Timer_p->GuardSemaphore_p);

    /* The task should be in the process of returning, but wait for
     * it first before deleting it. */
    error = STOS_TaskWait( &Timer_p->TimerTask_p, TIMEOUT_INFINITY);
    if(error == ST_NO_ERROR)
    {
        error = STOS_TaskDelete(Timer_p->TimerTask_p,
                                DriverPartition,
                                Timer_p->TimerTaskStack_p,
                                DriverPartition);
        if(error == ST_NO_ERROR)
        {
            STOS_SemaphoreDelete(NULL,Timer_p->GuardSemaphore_p);
            STOS_SemaphoreDelete(NULL,Timer_p->TimeoutSemaphore_p);
        }
    }
    /* Common exit point */
    return error;
} /* stuart_TimerDelete() */

/* End of timer.c */

