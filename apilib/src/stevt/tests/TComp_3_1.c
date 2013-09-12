/******************************************************************************\
 *
 * File Name   : TComp_3_1.c
 *
 * Description : Test STEVT_Subscribe Interface
 *
 * Author      : Manuela Ielo
 *
 * Copyright STMicroelectronics - February 1999
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "evt_test.h"

#define EVENT1 1
#define EVENT2 2
#define EVENT3 3
#define EVENT4 4
#define EVENT5 5

static int RegCB = 0;

static int UnregCB = 0;

ST_ErrorCode_t TCase_3_1_1(STEVT_Handle_t, STEVT_SubscribeParams_t);
ST_ErrorCode_t TCase_3_1_2(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_1_3(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_1_4(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_1_5(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_1_6(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_1_7(STEVT_Handle_t , STEVT_SubscribeParams_t );


void NotCB3_1(STEVT_CallReason_t Reason,
               STEVT_EventConstant_t Event,
               const void *EventData)
{
    printf("%s\n", (char *)EventData);
}

void RegCB3_1(STEVT_CallReason_t Reason,
                 STEVT_EventConstant_t Event,
                 const void *EventData)
{
    RegCB++;
}

void UnregCB3_1(STEVT_CallReason_t Reason,
                  STEVT_EventConstant_t Event,
                  const void *EventData)
{
    UnregCB++;
}

ST_ErrorCode_t TComp_3_1()
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_SubscribeParams_t SubscrParam;
    STEVT_SubscribeParams_t SubscrParam1;
    STEVT_Handle_t Handle1, Handle2, Handle3, Handle;
    STEVT_EventID_t EventID, EventID1, EventID2, EventID3;
    STEVT_TermParams_t TermParam;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum = 4;
    InitParam.ConnectMaxNum = 7;
    InitParam.SubscrMaxNum = 4;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotCB3_1;

    SubscrParam1.NotifyCallback = NULL;


    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle3 opening failed in Dev1"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle1 opening failed in Dev1"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle2 opening failed in Dev1"))
        return Error;
     }

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed in Dev1"))
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT1, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT1 registering by Handle failed"))
        return Error;
    }

    Error = STEVT_Register(Handle2, EVENT2, &EventID2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 registering by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle3, EVENT3, &EventID3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT3 registering by Handle3 failed"))
        return Error;
    }

    Error = TCase_3_1_1(Handle, SubscrParam);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    /*
     *  Test Case 1 close Handle, and unreg EVENT1 so EVENT1 is
     *  unregistered and is possible to regiter it
     */
    Error = STEVT_Register(Handle1, EVENT1, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT1 registering by Handle1 failed"))
        return Error;
    }

    /*
     *  Test Case 3.1.2
     *  This test check that if the handle for the subscription is the same
     *  that did the registration all works well
     */
    Error = STEVT_Subscribe(Handle1, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_3_1_3(Handle1, SubscrParam);
    if (Error != STEVT_ERROR_ALREADY_SUBSCRIBED)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }
    /*
     * Test Case 3_1_4
     *
     * (NotifyCB == NULL) => STEVT_ERROR_MISSING_NOTIFY_CALLBACK
     */
    Error = STEVT_Subscribe(Handle2, EVENT3, &SubscrParam1);
    if (Error != STEVT_ERROR_MISSING_NOTIFY_CALLBACK)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

/*     Error = TCase_3_1_5(Handle3, SubscrParam); */
/*     if (Error != STEVT_ERROR_NO_MORE_SPACE) */
/*     { */
/*         STEVT_Print(( "^^^Failed Test Case 5\n")); */
/*     } */
/*     else */
/*     { */
/*         STEVT_Print(( "^^^Passed Test Case 5\n")); */
/*     } */
/*     if (RegCB != 3) */
/*     { */
/*         STEVT_Report(( STTBX_REPORT_LEVEL_ERROR, */
/*                       "Reg callBack has not been correctly called")) */
/*         return 1; */
/*     } */
/*     RegCB = 0; */
    STEVT_Print(( "^^^Removed Test Case 5\n"));

    /*
     * Test case 3_1_6 : RegisterCB is called after Subscription of Handle2
     */
    Error = STEVT_Subscribe(Handle2, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 6\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 6\n"));
    }

    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 termination failed"))
        return Error;
    }

    return ST_NO_ERROR;
}

/*  This test check that if the handle has been previously closed
 *  ST_ERROR_INVALID_HANDLE is returned */
ST_ErrorCode_t TCase_3_1_1(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Handle close failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT1, &SubscrParam);

    return Error;
}


/*  If the same handle wants subscribe more than once STEVT_ERROR_ALREADY_
 *  SUBSCRIBED is returned */
ST_ErrorCode_t TCase_3_1_3(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 EVENT2 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);

    return Error;
}


/*  When there is no more space for a new subscription,
 *  STEVT_ERROR_NO_MORE_SPACE is returned */
ST_ErrorCode_t TCase_3_1_5(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle4, Handle5, Handle6;

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle5);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle5 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle6);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle6 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 subscription by Handle3 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle4, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 subscription by Handle4 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle5, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 subscription by Handle5 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle6, EVENT2, &SubscrParam);

    return Error;
}








