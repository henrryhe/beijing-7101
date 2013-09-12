/*****************************************************************************
File Name   : timer.h

Description : Timer support functions for UART.

Copyright (C) 2006 STMicroelectronics

History     :

Reference   :

*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __TIMER_H
#define __TIMER_H

/* Includes --------------------------------------------------------------- */

#if !defined(ST_OSLINUX) /* || defined(LINUX_FULL_USER_VERSION)*/
#include "stddefs.h"                    /* STAPI includes */
#endif

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

#define TIMER_IDLE         0            /* The timer is idle */
#define TIMER_RUNNING      1            /* The timer is running */
#define TIMER_PAUSED       (1<<1)       /* The timer is paused */
#define TIMER_ABORT        (1<<2)       /* The timer has been aborted */

/* These values specify the cause for a timer timing out */
#define TIMER_TIMEDOUT     -1
#define TIMER_SIGNALLED    0

/* Timer task's stack size */
#ifdef ST_OS21
#define TIMER_STACK_SIZE   (1024 * 16)
#else
#define TIMER_STACK_SIZE   (1024 * 2)
#endif

/*Suffix is added on DeviceName to identify the owning UART instance.*/
#define TimerDeviceSuffix "Timer"
#define MAX_TIMER_DEVICE_NAME \
(ST_MAX_DEVICE_NAME + sizeof(TimerDeviceSuffix)/sizeof(TimerDeviceSuffix[0]) - 1)


/* Exported Types --------------------------------------------------------- */

typedef struct
{
    void                (*CallbackRoutine)(void *);
    void                *CallbackParam;
    task_t              *TimerTask_p;
    tdesc_t             TimerTaskDescriptor;
    void                *TimerTaskStack_p;
    semaphore_t         *GuardSemaphore_p;
    semaphore_t         *TimeoutSemaphore_p;
    const STOS_Clock_t       *Timeout;
    BOOL                DeleteTimer;
    S32                 TimeoutError;
    volatile TimerStatus_t Status;
} Timer_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */
BOOL stuart_TimerCreate(ST_DeviceName_t DeviceName,ST_Partition_t *DriverPartition,Timer_t *Timer_p, U32 TaskPriority);
void stuart_TimerWait(Timer_t *Timer_p,
                      void (*CallbackRoutine)(void *),
                      void *CallbackParam,
                      const STOS_Clock_t *Timeout,
                      BOOL IsBlocking
                      );
void stuart_TimerSignal(Timer_t *Timer_p,BOOL RestartTimer);
void stuart_TimerPause(Timer_t *Timer_p, BOOL Pause);
BOOL stuart_TimerDelete(Timer_t *Timer_p, ST_Partition_t *DriverPartition);

#endif /* __TIMER_H */

/* End of timer.h */
