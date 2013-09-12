/******************************************************************************

    File Name   : ConcNotify.c

    Description : Evaluate STEVT performance when multiple threads are
                  notifying simultaneously

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "ThreadTests.h"
#if !defined( ST_OSLINUX )
#include "stlite.h"
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif
#include "stcommon.h"
#include "evt_test.h"

/* Private Types ----------------------------------------------------------- */

typedef struct NotifyThreadInfo_s
{
    STEVT_Handle_t  Handle;
    STEVT_EventID_t EventId;
    ST_ErrorCode_t  Error;

} NotifyThreadInfo_t;



/* Private Constants ------------------------------------------------------- */

enum
{
    TESTEVT_ERROR_FAIL = STEVT_DRIVER_BASE+100, /* STEVT bug found */
    TESTEVT_ERROR_TASK /* OS20 task call failed */
};

#define NUM_THREADS     4       /* aiming for a realistic number */
#define NUM_SUBSCR      10
#define NUM_NOTIFIES    100
#if defined(ST_OS21) || defined(ST_OSWINCE)
#define STACK_SIZE      17*1024 /* 16kB is min */
#else
#define STACK_SIZE      4096
#endif
#define THREAD_PRIORITY 10

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : NotifyCB
  Description : callback that does nothing
   Parameters :
******************************************************************************/
static void NotifyCB(STEVT_CallReason_t Reason,
                     STEVT_EventConstant_t Event,
                     const void *EventData)
{
    /* By default, we do nothing, and so you get best-case performance */

#if 0 /* change the balance a bit by doing some work */
#endif
}


/******************************************************************************
Function Name : NotifyThread
  Description : Perform NUM_NOTIFIES notifications
   Parameters :
******************************************************************************/
#if defined(ST_OSLINUX)
static void* NotifyThread(void *Param)
#else
static void NotifyThread(void *Param)
#endif

{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    NotifyThreadInfo_t  *Info_p = (NotifyThreadInfo_t*) Param;
    int                 i;
  
    /* Note the following loop never yields - which could prevent buffered
      STTBX output from getting out till we've finished */

    /*STEVT_Print(("NotifyThread...\n"));*/
    for (i = 0; i < NUM_NOTIFIES; ++i)
    {
        Error = STEVT_Notify(Info_p->Handle, Info_p->EventId, NULL);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ConcNotify: STEVT_Notify failed"));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            break;
        }
    }

    Info_p->Error = Error;
    /*STEVT_Print(("NotifyThread returning\n"));*/
    /* obscure behaviour: only two of these get out */
#if defined(ST_OSLINUX)
    return NULL;
#endif
}


/******************************************************************************
Function Name : TESTEVT_ConcNotify
  Description : Evaluate simultaneous STEVT_Notify performance, to see how much
                locking harms it. There are many scenarios we could try, but as
                the locking so far proposed is at the driver level, one
                registrant being simultaneously used by many threads to notify
                the same event is what we do.
   Parameters :
******************************************************************************/
ST_ErrorCode_t TESTEVT_ConcNotify( void )
{
    ST_ErrorCode_t          Error, LastError = ST_NO_ERROR;

#if defined( ST_OSLINUX )
	 pthread_attr_t attr;
        pthread_t notify_th[NUM_THREADS];
        struct sched_param sparam;
        int ret;
#endif

    STEVT_InitParams_t      InitParam;
    STEVT_TermParams_t      TermParam;
    STEVT_Handle_t          HandleReg;
    STEVT_Handle_t          HandleSubs[NUM_SUBSCR];
    STEVT_EventID_t         EventID;
    STEVT_SubscribeParams_t SubscrParam;
    task_t                  *NotifyTasks[NUM_THREADS];
    NotifyThreadInfo_t      NotifyInfo[NUM_THREADS];
    clock_t                 t1, t2, dt;
    S32                     i;


    SubscrParam.NotifyCallback     = NotifyCB;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum   = 1;
    InitParam.SubscrMaxNum  = NUM_SUBSCR;
    InitParam.ConnectMaxNum = NUM_SUBSCR+1;
#ifdef ST_OS21
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = (ST_Partition_t *)SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;


    /* Initialise EVT device */
    Error = STEVT_Init("DevConcNotify", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "DevConcNotify initialisation failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open registrant handle */
    Error = STEVT_Open("DevConcNotify", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ConcNotify: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Register the event */
    Error = STEVT_Register (HandleReg, STEVT_DRIVER_BASE, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ConcNotify: event registration failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* subscribe NUM_SUBSCR */
    for (i = 0; i < NUM_SUBSCR; ++i)
    {
        Error = STEVT_Open("DevConcNotify", NULL, &(HandleSubs[i]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ConcNotify: Opening subscriber %d failed\n",i));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }

        Error = STEVT_Subscribe (HandleSubs[i], STEVT_DRIVER_BASE, &SubscrParam);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ConcNotify: subscription %d failed",i));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }

    /* set up the notification tasks - because we pre-empt them, we will
      set them off together. Note they're all at the same priority */
#if defined( ST_OSLINUX )
    pthread_attr_init(&attr);
    sparam.sched_priority = THREAD_PRIORITY;
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    pthread_attr_setschedparam(&attr, &sparam);
#endif
  
    for (i = 0; i < NUM_THREADS; ++i)
    {
        NotifyInfo[i].Handle = HandleReg;
        NotifyInfo[i].EventId = EventID;
		
#if defined( ST_OSLINUX )
        ret = pthread_create(&notify_th[i],&attr,NotifyThread, (void*)&NotifyInfo[i]);
      if (ret != 0)	  	
#else
        NotifyTasks[i] = task_create(NotifyThread, (void*)&NotifyInfo[i],
                                     STACK_SIZE, THREAD_PRIORITY,
                                     "NotifyThread", 0);
#endif

        if (NotifyTasks[i] == NULL)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ConcNotify: couldn't create NotifyTask %i",i));
            return TESTEVT_ERROR_TASK;
        }

    }

    /* wait for tasks to complete */

    /* obscure behaviour: just adding the following halts GX1, unless you also
      add "NotifyThread returning" above. Without either, zero time is reported,
      with both 80 ms */
    /*STEVT_Print(("Releasing %i tasks...\n", NUM_THREADS));*/
    t1 = time_now();
    for (i = 0; i < NUM_THREADS; ++i)
    {
        Error = STOS_TaskWait (&NotifyTasks[i], TIMEOUT_INFINITY );
        if ( Error != ST_NO_ERROR )
        {       
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ConcNotify: error %d calling task_wait (NotifyTasks[%i])",Error,i));
            LastError = TESTEVT_ERROR_TASK;
        }
    }
    t2 = time_now();
    dt = time_minus(t2, t1);

    for (i = 0; i < NUM_THREADS; ++i)
    {
    
#if defined( ST_OSLINUX )
	if (0 != pthread_join(notify_th[i],NULL))
#else
        if (0 != task_delete(NotifyTasks[i]))
#endif

        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ConcNotify: error deleting NotifyTask %i",i));
            LastError = TESTEVT_ERROR_TASK;
        }
    }

    Error = STEVT_Term("DevConcNotify", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        LastError = Error;
    }

    /* nb STEVT_Term calls omitted on previous error cases */
    for (i = 0; i < NUM_THREADS; ++i)
    {
        /* propogate errors from notification tasks */
        if (NotifyInfo[i].Error != ST_NO_ERROR)
        {
            LastError = NotifyInfo[i].Error;
        }
    }

    if (LastError == ST_NO_ERROR)
    {
        STEVT_Print(("%i threads concurrently performaing %i notifications each"
                     " to %i subscribers:\n  Total time %lu ms (%lu ticks)\n",
                     NUM_THREADS, NUM_NOTIFIES, NUM_SUBSCR,
                     (dt*1000)/ST_GetClocksPerSecond(), dt));
    }
    return LastError;
}
