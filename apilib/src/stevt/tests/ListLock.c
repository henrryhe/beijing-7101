/******************************************************************************

    File Name   : ListLock.c

    Description : List-locking test for STEVT

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "ThreadTests.h"
#if !defined(ST_OSLINUX)
#include "stlite.h"
#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif
#endif
#include "evt_test.h"

/* Private Types ----------------------------------------------------------- */

typedef struct NotifyThreadInfo_s
{
    STEVT_Handle_t  Handle;
    STEVT_EventID_t EventId;
    volatile BOOL   Continue;
    ST_ErrorCode_t  Error;
    U32             NotifyCnt;

} NotifyThreadInfo_t;

/* Private Constants ------------------------------------------------------- */

enum
{
    TESTEVT_ERROR_FAIL = STEVT_DRIVER_BASE+100, /* STEVT bug found */
    TESTEVT_ERROR_TASK /* OS20 task call failed */
};

#define NUM_FILLERS      200

#if defined(ST_OS21) || defined(ST_OSWINCE)
#define STACK_SIZE      17*1024 /* min is 16kB */
#else
#define STACK_SIZE      4096
#endif

#define NOTIFY_PRIORITY         10
#define UNSUBSCRIBE_PRIORITY    11

#define WAIT        500
#define TIMEOUT     (WAIT * 3 * NUM_FILLERS)

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
/******************************************************************************
Function Name : FillCB
  Description : callback that does nothing
   Parameters :
******************************************************************************/
static void FillCB(STEVT_CallReason_t Reason,
                   STEVT_EventConstant_t Event,
                   const void *EventData)
{
}

/******************************************************************************
Function Name : ActiveCB
  Description : Callback that increments a count
   Parameters :
******************************************************************************/
static void ActiveCB(STEVT_CallReason_t Reason,
                     STEVT_EventConstant_t Event,
                     const void *EventData)
{
    ++ *(int*)EventData;
}


/******************************************************************************
Function Name : NotifyThread
  Description : Keep notifying ACTIVE_EVENT and check the notification is
                received (and no nasty crash occurs)
   Parameters :
******************************************************************************/
static void NotifyThread(void *Param)
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    NotifyThreadInfo_t  *Info_p = (NotifyThreadInfo_t*) Param;
    int                 CBCount;

    /* For maximum coverage, the following loop never yields - note this could
      prevent buffered STTBX output from getting out till we've finished */

    Info_p->NotifyCnt = 0;
    while (Info_p->Continue)
    {
        CBCount = 0;

        Error = STEVT_Notify(Info_p->Handle, Info_p->EventId, (void*)&CBCount);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: notify failed"));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            break;
        }
        if (CBCount != 1)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: wrong number of callbacks from notify"));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received %d, should get 1",CBCount));
            STEVT_Print((", %u notifies %d incident and then failed\n",
                     NUM_FILLERS, Info_p->NotifyCnt));

            Error = TESTEVT_ERROR_FAIL;
            break;
        }
        ++(Info_p->NotifyCnt);
    }

    Info_p->Error = Error;
}


/******************************************************************************
Function Name : UnsubscribeThread
  Description : Periodically delete a FILL_EVENT subscription, attempting to
                upset NotifyThread
   Parameters : STEVT_Handle_t array with NUM_FILLERS elements
******************************************************************************/
static void UnsubscribeThread(void *Param)
{
    STEVT_Handle_t          *Handles_p = (STEVT_Handle_t*) Param;
    STEVT_SubscribeParams_t SubscrParam;
    ST_ErrorCode_t          Error;
    U32                     subs;

    SubscrParam.NotifyCallback     = FillCB;

    STEVT_Print(("\nPlease wait"));
    for (subs = 0; subs < NUM_FILLERS; ++subs)
    {
        task_delay(WAIT);

        if (subs%20 == 0)
        {
            STEVT_Print(("."));
        }
        Error = STEVT_Unsubscribe(Handles_p[subs], STEVT_DRIVER_BASE);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: unsubscription %d failed",subs));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return;
        }

        /* That is sufficient to make STEVT_Notify read deallocated memory.
          However, stevt_RemoveSubscriber doesn't NULL out Current->Next
          before freeing it, so STEVT_Notify is still able to get back on
          the chain. To really prove the point, we need to reuse the
          memory. Putting the same subscription back now goes at the end of
          the chain, NULLing Next as required */

        Error = STEVT_Subscribe (Handles_p[subs], STEVT_DRIVER_BASE, &SubscrParam);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: fill re-subscription %d failed",subs));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return;
        }
    }

    task_delay(WAIT);
}

#endif
/******************************************************************************
Function Name : TESTEVT_ListLock
  Description : Demonstrate that lack of locking on subscriber list reads can
                cause us to miss later elements of the list
   Parameters :
******************************************************************************/
ST_ErrorCode_t TESTEVT_ListLock( void )
{
    ST_ErrorCode_t          Error = ST_NO_ERROR;
#if defined( ST_OSLINUX )
    printf("TESTEVT_ListLock - Not Implemented\n");
    return ST_ERROR_FEATURE_NOT_SUPPORTED; /* Any old error */
#else
    STEVT_InitParams_t      InitParam;
    STEVT_TermParams_t      TermParam;
    STEVT_Handle_t          HandleReg;
    STEVT_Handle_t          HandleFillerSub[NUM_FILLERS], HandleActiveSub;
    STEVT_EventID_t         EventID;
    STEVT_SubscribeParams_t SubscrParam;
    task_t                  *NotifyTask, *UnsubscribeTask;
    NotifyThreadInfo_t      NotifyInfo;

    U32                     subs;
    S32                     i;


    SubscrParam.NotifyCallback     = FillCB;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum   = 1;
    InitParam.ConnectMaxNum = NUM_FILLERS+2; /* NUM_FILLERS+1 subscribers */
    InitParam.SubscrMaxNum  = NUM_FILLERS+1;
#ifdef ST_OS21
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = (ST_Partition_t *)SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;


    /* Initialise EVT device */
    Error = STEVT_Init("DevListLock", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "DevListLock initialisation failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open registrant handle */
    Error = STEVT_Open("DevListLock", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Register the event */
    Error = STEVT_Register (HandleReg, STEVT_DRIVER_BASE, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: event registration failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* subscribe NUM_FILLERS first, then the active one */
    for (subs = 0; subs < NUM_FILLERS; subs++)
    {
        Error = STEVT_Open("DevListLock", NULL, &(HandleFillerSub[subs]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: Opening fill subscriber %d failed\n",subs));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }

        Error = STEVT_Subscribe (HandleFillerSub[subs], STEVT_DRIVER_BASE, &SubscrParam);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: fill subscription %d failed",subs));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }

    Error = STEVT_Open("DevListLock", NULL, &(HandleActiveSub));
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: Opening active subscriber failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }
    SubscrParam.NotifyCallback = ActiveCB;
    Error = STEVT_Subscribe (HandleActiveSub, STEVT_DRIVER_BASE, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: active subscription failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* set a low-priority task repeatedly notifying
      and a higher-priority one deleting subscriptions */

    NotifyInfo.Handle = HandleReg;
    NotifyInfo.EventId = EventID;
    NotifyInfo.Continue = 1;

    NotifyTask = task_create(NotifyThread, (void*)&NotifyInfo, STACK_SIZE,
                             NOTIFY_PRIORITY, "NotifyThread",(task_flags_t)0);
    if (NotifyTask == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: couldn't create NotifyTask"));
        return TESTEVT_ERROR_TASK;
    }

    UnsubscribeTask = task_create(UnsubscribeThread, (void*)HandleFillerSub,
                                  STACK_SIZE, UNSUBSCRIBE_PRIORITY,
                                  "UnsubscribeThread",(task_flags_t)0);
    if (UnsubscribeTask == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: couldn't create UnsubscribeTask"));
        return TESTEVT_ERROR_TASK;
    }

    /* wait for tasks to complete */
    i = task_wait(&UnsubscribeTask, 1, TIMEOUT_INFINITY); /* &Timeout */
    if (i == 0)
    {
        if (0 != task_delete(UnsubscribeTask))
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: error deleting UnsubscribeTask"));
            return TESTEVT_ERROR_TASK;
        }
    }
    else if (i == -1)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: timed out waiting for UnsubscribeTask to complete"));
        return TESTEVT_ERROR_TASK;
    }
    else
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: error %d calling task_wait (UnsubscribeTask)", i));
        return TESTEVT_ERROR_TASK;
    }

    NotifyInfo.Continue = 0;
    i = task_wait(&NotifyTask, 1, TIMEOUT_INFINITY); /* &Timeout */
    if (i == 0)
    {
        if (0 != task_delete(NotifyTask))
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "ListLock: error deleting NotifyTask"));
            return TESTEVT_ERROR_TASK;
        }
    }
    else if (i == -1)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: timed out waiting for NotifyTask to complete"));
        return TESTEVT_ERROR_TASK;
    }
    else
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ListLock: error %d calling task_wait (NotifyTask)", i));
        return TESTEVT_ERROR_TASK;
    }

    Error = STEVT_Term("DevListLock", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* nb STEVT_Term calls omitted on previous error cases */

    Error = NotifyInfo.Error; /* message already given by NotifyTask */
    if (Error == ST_NO_ERROR)
    {
        STEVT_Print(("\nPerformed %u terminations, %u notifies without incident\n",
                     NUM_FILLERS, NotifyInfo.NotifyCnt));
    }
#endif
    return Error;
}
