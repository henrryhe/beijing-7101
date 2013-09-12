/******************************************************************************\
 *
 * File Name   : TComp_1_2.c
 *
 * Description : Test STEVT_Close Interface
 *
 * Author      : Manuela Ielo
 *
 * Copyright STMicroelectronics - February 1999
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "evt_test.h"

#define EVENT2  2
#define EVENT3  3


STEVT_SubscribeParams_t SubscrParam;

ST_ErrorCode_t TCase_1_2_1(void);
ST_ErrorCode_t TCase_1_2_2(void);
ST_ErrorCode_t TCase_1_2_3(void);
ST_ErrorCode_t TCase_1_2_4(void);
ST_ErrorCode_t TCase_1_2_5(void);


void NotifyCB(STEVT_CallReason_t Reason,
               STEVT_EventConstant_t Event,
               const void *EventData)
{
    printf("%s %d\n", (char *)EventData, Reason);
}


U32 TComp_1_2()
{
    ST_ErrorCode_t Error;
    U32 NumErrors=0;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum = 5;
    InitParam.ConnectMaxNum = 5;
    InitParam.SubscrMaxNum = 5;

#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotifyCB;
    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"));
        NumErrors++;
        return NumErrors;
    }

    Error = TCase_1_2_1();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }
    Error = TCase_1_2_2();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_1_2_3();

    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    Error = TCase_1_2_4();
    if (Error != ST_NO_ERROR)
    {
        NumErrors++;
    }
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

    Error = TCase_1_2_5();
    if (Error != ST_NO_ERROR)
    {
        NumErrors++;
    }

    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 5\n"));
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
        NumErrors++;
    }

    return NumErrors;
}



/*  If 2 successive close for the same handle are made an
 *  ST_ERROR_INVALID_HANDLE is returned */
ST_ErrorCode_t TCase_1_2_1(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_Handle_t Handle;
#if defined(ARCHITECTURE_ST200) && defined(ST_8010)
    STEVT_Handle_t FakeHandle = 0x50000000;
#elif defined(ARCHITECTURE_ST200) && defined(ST_5301)
    STEVT_Handle_t FakeHandle = 0xC0000000;
#else
    STEVT_Handle_t FakeHandle = 0x80000000;
#endif

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed"));
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "first close failed"));
        return Error;
    }

    /*  Second close for the same handle */
    Error = STEVT_Close(Handle);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Wrong error returned : 0x%X",Error));
        Error = ST_ERROR_INVALID_HANDLE; /* Any value different from 0 is ok */
        return Error;
    }
    else
    {
        Error = ST_NO_ERROR;
    }

    /* Try to close using a fake handle */
    Error = STEVT_Close(FakeHandle);

    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Wrong error returned : 0x%X",Error));
        Error = ST_ERROR_INVALID_HANDLE; /* Any value different from 0 is ok */
        return Error;
    }
    else
    {
        Error = ST_NO_ERROR;
    }
    return Error;
}

/*  Closing an handle without subscription */
ST_ErrorCode_t TCase_1_2_2(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed"));
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "first close failed"));
        return Error;
    }

    return Error;
}

/*  Closing an handle with subscription */
ST_ErrorCode_t TCase_1_2_3( void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle, Handle2, Handle1;
    STEVT_EventID_t EventID;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle opening failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Handle1 opening failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Handle2 opening failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Event2 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle2, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Event2 subscription by Handle2 failed"));
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT2, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle EVENT2 registering failed"))
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle close failed"))
        return Error;
    }

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle close failed"))
        return Error;
    }

    Error = STEVT_Close(Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle close failed"))
        return Error;
    }
    return Error;
}

/* This test check the STEVT_Close for a Subscriber, i.e. if the
 * Close remove the Handle from the Subscription List */
ST_ErrorCode_t TCase_1_2_4(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle, Handle1;
    STEVT_EventID_t Event2;
    STEVT_EventID_t Event3;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Opening Handle failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Handle1 opening failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Event2 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT2, &Event2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Registering Event2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT3, &Event3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Registering Event3 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Event3 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle1 close failed"))
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle close failed"))
        return Error;
    }
    return Error;

}

/* This test check the STEVT_Close with a NULL Handle */
ST_ErrorCode_t TCase_1_2_5(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle3;

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Opening Handle3 failed"))
        return Error;
    }

    Error = STEVT_Close((STEVT_Handle_t)NULL);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                       "Invalid error returned: 0x%X",Error));
        Error = ST_ERROR_INVALID_HANDLE; /* Only to return an error */
    }
    else
    {
        Error = ST_NO_ERROR;
    }

    return Error;

}
