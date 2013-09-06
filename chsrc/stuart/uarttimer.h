/*****************************************************************************
File Name   : uarttimer.h

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
#ifndef __UARTTIMER_H
#define __UARTTIMER_H

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"                    /* STAPI includes */

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

#ifndef STUART_TIMER_STACK_SIZE
#if defined(ARCHITECTURE_ST40)
#define STUART_TIMER_STACK_SIZE           10*1024
#else
#define STUART_TIMER_STACK_SIZE           2*1024
#endif
#endif

/*Suffix is added on DeviceName to identify the owning UART instance.*/
#define TimerDeviceSuffix "Timer"
#define MAX_TIMER_DEVICE_NAME \
(ST_MAX_DEVICE_NAME + sizeof(TimerDeviceSuffix)/sizeof(TimerDeviceSuffix[0]) - 1)


/* Exported Types --------------------------------------------------------- */


typedef struct
{ 
 	void                			(*CallbackRoutine)(void *);
	void                			*CallbackParam;
	task_t              			*TimerTask_p;
	tdesc_t             			TimerTaskDescriptor;
	void               			*TimerTaskStack_p;
	semaphore_t         			*GuardSemaphore_p;
	semaphore_t         			*TimeoutSemaphore_p;
	const STOS_Clock_t       	*Timeout;
	BOOL                			DeleteTimer;
	S32                 			TimeoutError;
	volatile TimerStatus_t 		Status;
} Timer_t;

/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */
BOOL stuart_TimerCreate(ST_DeviceName_t DeviceName, ST_Partition_t *DriverPartition, Timer_t *Timer_p, U32 TaskPriority);
void stuart_TimerWait(Timer_t *Timer_p,
                      void (*CallbackRoutine)(void *),
                      void *CallbackParam,
                      const STOS_Clock_t *Timeout,
                      BOOL IsBlocking
                      );
void stuart_TimerSignal(Timer_t *Timer_p,BOOL RestartTimer);
void stuart_TimerPause(Timer_t *Timer_p, BOOL Pause);
BOOL stuart_TimerDelete(Timer_t *Timer_p, ST_Partition_t *DriverPartition);

#endif /* __UARTTIMER_H */

/* End of uarttimer.h */
