#ifndef __MGDELAYDRV_H__
#define __MGDELAYDRV_H__

/*****************************************************************************
  include
 *****************************************************************************/
/*#include       "..\chcabase\ChTimer.h"*/
#include       "..\chcabase\ChSys.h"



#if          0
/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/
/* Timer status */
#define             MGMAX_NO_TIMER                8                /*The max number of the timer*/

#define             MGTIMER_IDLE                 0                /* The timer is idle */
#define             MGTIMER_RUNNING          1                /* The timer is running */
#define             MGTIMER_PAUSED            (1<<1)        /* The timer is paused */
#define             MGTIMER_ABORT              (1<<2)        /* The timer has been aborted */

/* These values specify the cause for a timer timing out */
#define             MGTIMER_TIMEDOUT       -1
#define             MGTIMER_SIGNALLED       0

typedef U32     MgTimerStatus_t;


/* Timer task's stack size */
#if 1
typedef struct
{
      CALLBACK                              CallbackRoutine;
      semaphore_t                           GuardSemaphore;
      semaphore_t                           TimeoutSemaphore;
      udword                                    Delay;
      unsigned long int                      TimeOut;
      clock_t                                    StartTime;	  
      BOOL                                      DeleteTimer;
      S32                                        TimeoutError;
      volatile MgTimerStatus_t          Status;
      boolean                                  UsedStatus;
      boolean                                  TimerRestart;	  
      TMGDDITimerHandle                hDdiTimerHandle;	   
} MgTimer_t;
#else
typedef struct
{
      CALLBACK                              CallbackRoutine;
      semaphore_t                           GuardSemaphore;
      semaphore_t                           TimeoutSemaphore;
      udword                                   Timeout;
      BOOL                                      DeleteTimer;
      S32                                        TimeoutError;
      volatile MgTimerStatus_t          Status;
      boolean                                  UsedStatus;
	  
} MgTimer_t;
#endif
#endif

#endif                                  /* __MGDELAYDRV_H__ */
