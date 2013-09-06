#ifndef __CHTIMER_H__
#define __CHTIMER_H__

/*****************************************************************************
  include
 *****************************************************************************/
#include       "ChSys.h"




/*****************************************************************************
 *BASIC DEFINITIONS
 *****************************************************************************/
/* Timer status */
#define             CHCA_TIMER_IDLE                              0              /* The timer is idle */
#define             CHCA_TIMER_RUNNING                       1              /* The timer is running */
#define             CHCA_TIMER_PAUSED                         (1<<1)      /* The timer is paused */
#define             CHCA_TIMER_ABORT                           (1<<2)      /* The timer has been aborted */

/* These values specify the cause for a timer timing out */
#define             CHCA_TIMER_TIMEDOUT                     -1
#define             CHCA_TIMER_SIGNALLED                    0

typedef CHCA_UINT     ChCA_TimerStatus_t;


/* Timer task's stack size */
typedef struct
{
        CHCA_CALLBACK_FN                CallbackRoutine;
        CHCA_ULONG                           TimeDelay;
        CHCA_ULONG                           TimeOut;
        CHCA_TICKTIME                       StartTime;	  
        volatile ChCA_TimerStatus_t      TimerStatus;
        CHCA_BOOL                             UsedStatus;
        ChCA_Timer_Handle_t               hTimerHandle;	  
} CHCA_Timer_t;


#endif                 /* __CHTIMER_H__ */


