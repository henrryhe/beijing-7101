/******************************************************************************\
 *
 * File Name   : TComp_3_3.c
 *
 * Description : Test STEVT_Unsubscribe Interface
 *
 * Author      : Manuela Ielo
 *
 * Copyright STMicroelectronics - March 1999
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

ST_ErrorCode_t TCase_3_3_1(STEVT_Handle_t , STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_3_2(STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_3_3(void);
ST_ErrorCode_t TCase_3_3_4(STEVT_SubscribeParams_t );
ST_ErrorCode_t TCase_3_3_5(STEVT_SubscribeParams_t );


void NotCB3_3(STEVT_CallReason_t Reason,
              STEVT_EventConstant_t Event,
              const void *EventData)
{
    printf("%s\n", (char *)EventData);
}

void RegCB3_3(STEVT_CallReason_t Reason,
                STEVT_EventConstant_t Event,
                const void *EventData)
{
    RegCB++;
}

void UnregCB3_3(STEVT_CallReason_t Reason,
                  STEVT_EventConstant_t Event,
                  const void *EventData)
{
    UnregCB++;
}

ST_ErrorCode_t TComp_3_3()
{
    ST_ErrorCode_t Error;
    U32 ErrorNum =0;
    STEVT_InitParams_t InitParam;
    STEVT_SubscribeParams_t SubscrParam;
    STEVT_Handle_t Handle;
    STEVT_TermParams_t TermParam;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum = 4;
    InitParam.ConnectMaxNum = 5;
    InitParam.SubscrMaxNum = 4;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotCB3_3;


    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening in Dev1 failed"))
        return Error;
    }

    Error = TCase_3_3_1(Handle, SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    Error = TCase_3_3_2(SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_3_3_3();
    if (Error != STEVT_ERROR_INVALID_EVENT_NAME)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    Error = TCase_3_3_4(SubscrParam);

    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }
    RegCB = 0;

    Error = TCase_3_3_5(SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 5\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 5\n"));
    }

    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 termination failed"))
        return Error;
    }

    return ErrorNum;

}

/*  This test check that ST_ERROR_INVALID_HANDLE is returned if the handle
 *  is previously closed */
ST_ErrorCode_t TCase_3_3_1(STEVT_Handle_t Handle,
                           STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;

    Error = STEVT_Subscribe(Handle, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT1 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Handle closing failed"));
        return Error;
    }

    Error = STEVT_Unsubscribe(Handle, EVENT1);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Expected error 0x%08X, received 0x%08X",ST_ERROR_INVALID_HANDLE,Error));
        return 1;
    }

    return ST_NO_ERROR;
}

/*  This test checks that if the Handle is not in the subscription
 *  list ST_ERROR_INVALID_HANDLE is returned */
ST_ErrorCode_t TCase_3_3_2(STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error, Error1;
    STEVT_Handle_t Handle1, Handle2, Handle3;

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle1 opening in Dev1 failed"))
         return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle2 opening in Dev1 failed"))
         return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle3 opening in Dev1 failed"))
         return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle2, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 subscription by Handle2 failed"))
        return Error;
    }

    Error1= STEVT_Unsubscribe(Handle3, EVENT2);
    if (Error1 != STEVT_ERROR_NOT_SUBSCRIBED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                     "Test Case 2 Expected error 0x%08X, received 0x%08X",
                     STEVT_ERROR_NOT_SUBSCRIBED, Error1));
        Error1 = 1;
    }
    else
    {
        Error1 = 0;
    }


    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle1 close failed"))
        return Error;
    }

    Error = STEVT_Close(Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle2 closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle3 closing failed"))
        return Error;
    }

    return Error1;
}

/*  If the Event is not in the Event List STEVT_ERROR_INVALID_EVENT_NAME
 *  is returned */
ST_ErrorCode_t TCase_3_3_3(void)
{
   ST_ErrorCode_t Error;
   STEVT_Handle_t Handle;

   Error = STEVT_Open("Dev1", NULL, &Handle);
   if (Error != ST_NO_ERROR)
   {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle opening in Dev1 failed"))
        return Error;
    }

   Error = STEVT_Unsubscribe(Handle, EVENT4);

   return Error;
}

/*  Unsubscribe from a registered event */
ST_ErrorCode_t TCase_3_3_4(STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error, Error1;
    STEVT_Handle_t Handle1, Handle2, Handle3;
    STEVT_EventID_t EventID;

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle1 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle2 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle3 opening in Dev1 failed"))
        return Error;
    }

    Error =  STEVT_Subscribe(Handle1, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT1 subscription by Handle1 failed"))
        return Error;
    }

    Error =  STEVT_Subscribe(Handle2, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT1 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle3, EVENT1, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT1 subscription by Handle3 failed"))
        return Error;
    }

    Error1 = STEVT_Unsubscribe(Handle2, EVENT1);

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle1 closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle2 closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle3 closing failed"))
        return Error;
    }

    return Error1;
}


/*  Unsubscribe from an unregistered event */
ST_ErrorCode_t TCase_3_3_5(STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle1, Handle2, Handle3;

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle1 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle2 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle3 opening in Dev1 failed"))
        return Error;
    }

    Error =  STEVT_Subscribe(Handle1, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 subscription by Handle1 failed"))
        return Error;
    }

    Error =  STEVT_Subscribe(Handle2, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 subscription by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Unsubscribe(Handle2, EVENT2);

    return Error;
}
