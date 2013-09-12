/******************************************************************************\
 *
 * File Name   : TComp_3_2.c
 *
 * Description : Test STEVT_Subscribe Interface
 *
 * Copyright STMicroelectronics - 1999 - 2000
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

ST_ErrorCode_t TCase_3_2_1(STEVT_Handle_t, STEVT_SubscribeParams_t);
ST_ErrorCode_t TCase_3_2_2(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_2_3(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_2_4(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_2_5(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_2_6(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_2_7(STEVT_Handle_t , STEVT_SubscribeParams_t );


void NotCB3_2(STEVT_CallReason_t Reason,
               STEVT_EventConstant_t Event,
               const void *EventData)
{
    printf("%s\n", (char *)EventData);
}

void RegCB3_2(STEVT_CallReason_t Reason,
                 STEVT_EventConstant_t Event,
                 const void *EventData)
{
    RegCB++;
}

void UnregCB3_2(STEVT_CallReason_t Reason,
                  STEVT_EventConstant_t Event,
                  const void *EventData)
{
    UnregCB++;
}

ST_ErrorCode_t TComp_3_2()
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_SubscribeParams_t SubscrParam;
    STEVT_SubscribeParams_t SubscrParam1;
    STEVT_Handle_t Handle1, Handle2, Handle3, Handle, Handle5;
    STEVT_TermParams_t TermParam;
    U32 ErrorNum=0;

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

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotCB3_2;

    SubscrParam1.NotifyCallback = NULL;

    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"))
        return Error;
    }

    Error = STEVT_Init("Dev2", &InitParam);
    if (Error != ST_NO_ERROR)
    {
       STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                     "Dev2 initialisation failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle3 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle1 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle2 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev2", NULL, &Handle5);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle5 opening in Dev2 failed"))
        return Error;
    }

    Error = TCase_3_2_1(Handle, SubscrParam);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;

    }
    else
    {
         STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    Error = TCase_3_2_2(Handle1, SubscrParam);
    if (Error != STEVT_ERROR_ALREADY_SUBSCRIBED)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
         STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_3_2_3(Handle2, SubscrParam1);
    if (Error != STEVT_ERROR_MISSING_NOTIFY_CALLBACK)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
         STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

/*     Error = TCase_3_2_4(Handle3, SubscrParam); */
/*     if (Error != STEVT_ERROR_NO_MORE_SPACE) */
/*     { */
/*         STEVT_Print(( "^^^Failed Test Case 4\n")); */
/*         ErrorNum++; */
/*     } */
/*     else */
/*     { */
/*          STEVT_Print(( "^^^Passed Test Case 4\n")); */
/*     } */
    STEVT_Print(( "^^^Removed Test Case 4\n"));

/*     Error = TCase_3_2_5(Handle2, SubscrParam); */
/*     if (Error != STEVT_ERROR_NO_MORE_SPACE) */
/*     { */
/*         STEVT_Print(( "^^^Failed Test Case 5\n")); */
/*         ErrorNum++; */
/*     } */
/*     else */
/*     { */
/*          STEVT_Print(( "^^^Passed Test Case 5\n")); */
/*     }    */
    STEVT_Print(( "^^^Removed Test Case 5\n"));

    Error = TCase_3_2_6(Handle5, SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 6\n"));
        ErrorNum++;
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


    Error = STEVT_Term("Dev2", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev2 termination failed"))
        return Error;
    }
    return ErrorNum;
}

/*  This test check that if the handle has been previously closed
 *  ST_ERROR_INVALID_HANDLE is returned */
ST_ErrorCode_t TCase_3_2_1(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT1, &SubscrParam);

    return Error;
}

/*  If the same handle wants subscribe more than once STEVT_ERROR_ALREADY_
 *  SUBSCRIBED is returned */
ST_ErrorCode_t TCase_3_2_2(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);

    return Error;
}

/*  When NotifyCB is NULL, STEVT_ERROR_MISSING_NOTIFY_CALLBACK is returned */
ST_ErrorCode_t TCase_3_2_3(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Subscribe(Handle, EVENT3, &SubscrParam);

    return Error;
}

/*  When there is no more space for a new subscription,
 *  STEVT_ERROR_NO_MORE_SPACE is returned */
ST_ErrorCode_t TCase_3_2_4(STEVT_Handle_t Handle,
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
                      "Test Case 5 EVENT2  subscription by Handle3 failed"))
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

/*  When there are no more space to add new event in the Event List,
 *  STEVT_ERROR_NO_MORE_SPACE is returned */
ST_ErrorCode_t TCase_3_2_5(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Subscribe(Handle, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT1 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT3 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT4, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT4 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT5, &SubscrParam);

    return Error;
}


/*  At Subscription time, RegisterCB is called */
ST_ErrorCode_t TCase_3_2_6(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle6;
    STEVT_EventID_t EventID;

    Error = STEVT_Open("Dev2", NULL, &Handle6);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 6 Handle6 opening failed"))
        return Error;
    }
    Error = STEVT_Subscribe(Handle, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }
    Error = STEVT_Register(Handle6, EVENT3, &EventID);

    return Error;
}

ST_ErrorCode_t TCase_3_2_7(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Subscribe(Handle, EVENT3, &SubscrParam);

    return Error;
}







