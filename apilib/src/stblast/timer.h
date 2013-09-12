/*****************************************************************************
File Name   : timer.h

Description : Timer support functions for UART.

Copyright (C) 2000 STMicroelectronics

History     :

    30/11/99  Migrated timer functions to separate module.

    13/01/00  Added volatile identifier to timer status flags to ensure
              compiler optimization does not take place as the flags are
              used for inter-task communications.

    30/3/00   Renamed functions to avoid conflicts with global namespace.

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __TIMER_H
#define __TIMER_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */
#include "stos.h"

/* Exported Constants ----------------------------------------------------- */
/*****************************************************************************
Timer_t:
A general purpose timer that allows a callback routine to be invoked after
a specified number of milliseconds have elapsed.
Each timer is represented by a task pending on a timed semaphore, with a
number of utility functions for controlling the execution of the task.
*****************************************************************************/

/* Timer status */
typedef U32 TimerStatus_t;

#define TIMER_IDLE         0           /* The timer is idle */
#define TIMER_RUNNING      1           /* The timer is running */
#define TIMER_PAUSED       (1<<1)      /* The timer is paused */
#define TIMER_ABORT        (1<<2)      /* The timer has been aborted */

/* These values specify the cause for a timer timing out */
#define TIMER_TIMEDOUT     -1
#define TIMER_SIGNALLED    0

/* Timer task's stack size */
#ifndef STBLAST_TIMER_STACK_SIZE
#ifdef ST_OS21
#define STBLAST_TIMER_STACK_SIZE   (1024 * 16)
#else
#define STBLAST_TIMER_STACK_SIZE   (1024 * 2)
#endif
#endif

/* Exported Types --------------------------------------------------------- */

typedef struct Timer_s
{
    void                (*CallbackRoutine)(void *);
    void                *CallbackParam;
    tdesc_t             TimerTaskDescriptor;
    void                *TaskStack_p;
    task_t              *TimerTask;
    semaphore_t         *GuardSemaphore_p;
    semaphore_t         *TimeoutSemaphore_p;
    const STOS_Clock_t  *Timeout;
    BOOL                DeleteTimer;
    S32                 TimeoutError;
    volatile TimerStatus_t Status;
} Timer_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */

BOOL stblast_TimerCreate(const ST_DeviceName_t DeviceName,Timer_t *Timer_p, U32 TaskPriority);
void stblast_TimerWait(Timer_t *Timer_p,
                      void (*CallbackRoutine)(void *),
                      void *CallbackParam,
                      const STOS_Clock_t *Timeout,
                      BOOL IsBlocking
                      );
void stblast_TimerSignal(Timer_t *Timer_p, BOOL RestartTimer);
void stblast_TimerPause(Timer_t *Timer_p, BOOL Pause);
BOOL stblast_TimerDelete(const ST_DeviceName_t DeviceName,Timer_t *Timer_p);

#endif /* __TIMER_H */

/* End of timer.h */
