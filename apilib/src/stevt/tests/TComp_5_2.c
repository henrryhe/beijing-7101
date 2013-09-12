/******************************************************************************\
 *
 * File Name   : TComp_5_2.c
 *
 * Description : Test STEVT_Term Interface
 *
 * Author      : Manuela Ielo
 *
 * Copyright STMicroelectronics - March 1999
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "evt_test.h"

ST_ErrorCode_t TCase_5_2_1(void);
ST_ErrorCode_t TCase_5_2_2(void);
ST_ErrorCode_t TCase_5_2_3(void);
ST_ErrorCode_t TCase_5_2_4(void);

#define EVENT1 1
#define EVENT2 2

void NotCB5_2(STEVT_CallReason_t Reason,
               STEVT_EventConstant_t Event,
               const void *EventData)
{
    printf("%s %d\n", (char *)EventData, Reason);
}

ST_ErrorCode_t TComp_5_2()
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;

    InitParam.EventMaxNum = 4;
    InitParam.ConnectMaxNum = 4;
    InitParam.SubscrMaxNum = 4;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

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

    Error = STEVT_Init("Dev3", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev3 initialisation failed"))
        return Error;
    }

    Error = TCase_5_2_1();
    if (Error != ST_ERROR_OPEN_HANDLE)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    Error = TCase_5_2_2();
    if (Error != ST_ERROR_UNKNOWN_DEVICE)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_5_2_3();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    Error = TCase_5_2_4();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

    TermParam.ForceTerminate = TRUE;
    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 forced termination failed"))
    }

    return ST_NO_ERROR;
}

/*  This test checks that ST_ERROR_OPEN_HANDLE is returned
 *  doing a Term when there are still open connections */
ST_ErrorCode_t TCase_5_2_1(void)
{
    ST_ErrorCode_t Error;
    STEVT_TermParams_t TermParams;
    STEVT_Handle_t Handle1, Handle2, Handle3;
    STEVT_SubscribeParams_t SubscrParam;

    Error = STEVT_Open("Dev1", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1: Handle1 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1: Handle2 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1: Handle3 opening in Dev1 failed"))
        return Error;
    }

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotCB5_2;

    Error = STEVT_Subscribe(Handle3, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1: EVENT2 subscription by Handle3 failed"))
        return Error;
    }

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1: Handle1 closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1: Handle3 closing failed"))
        return Error;
    }

    TermParams.ForceTerminate = FALSE;

    Error = STEVT_Term("Dev1",  &TermParams);

    return Error;
}

/*  This test checks that ST_ERROR_UNKNOWN_DEVICE is returned
 *  when the device to term is not in the dictionary, both ForceTerminate
 *  is TRUE or FALSE */
ST_ErrorCode_t TCase_5_2_2(void)
{
    ST_ErrorCode_t Error;
    STEVT_TermParams_t TermParams;

    TermParams.ForceTerminate = FALSE;
    Error = STEVT_Term("Dev4", &TermParams);
    if (Error != ST_ERROR_UNKNOWN_DEVICE)
        return Error;

    TermParams.ForceTerminate = TRUE;
    Error = STEVT_Term("Dev4", &TermParams);

    return Error;

}

/*  All works in a correct way when ForceTerminate is False */
ST_ErrorCode_t TCase_5_2_3(void)
{
    ST_ErrorCode_t Error;
    STEVT_TermParams_t TermParams;
    STEVT_Handle_t Handle1, Handle2, Handle3;
    STEVT_SubscribeParams_t SubscrParam;
    STEVT_EventID_t EventID;

    TermParams.ForceTerminate = FALSE;

    Error = STEVT_Open("Dev2", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: Handle1 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev2", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: Handle2 opening in Dev2 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev2", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: Handle2 opening in Dev2 failed"))
        return Error;
    }

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotCB5_2;

    Error = STEVT_Register(Handle1, EVENT1, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: EVENT1 registering by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle2, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: EVENT1 subscription by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle3, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: EVENT2 subscription by Handle3 failed"))
        return Error;
    }

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: Handle1 closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: Handle2 closing failed"))

        return Error;
    }

    Error = STEVT_Close(Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3: Handle3 closing failed"))

        return Error;
    }

    Error = STEVT_Term("Dev2", &TermParams);

    return Error;

}

ST_ErrorCode_t TCase_5_2_4(void)
{
    STEVT_TermParams_t TermParams;
    ST_ErrorCode_t Error, Error1;
    STEVT_Handle_t Handle1, Handle2, Handle3;

#if !defined(ST_OSLINUX)
	STEVT_EventID_t EventID;
#endif

    TermParams.ForceTerminate = TRUE;

    Error = STEVT_Open("Dev3", NULL, &Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4: opening Handle1 in Dev3 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev3", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4: opening Handle 2 in Dev3 failed"))
        return Error;
    }

    Error1 = STEVT_Term("Dev3", &TermParams);
    if (Error1 != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4: Dev3 termination failed"))
    }

#if defined(ST_OSLINUX)
    /* This function fails because the test in STPTI_Register dereferences an invalid pointer
      IS_VALID_EVT_HANDLE(TheUser)
      The MACRO needs to be re-written / replaced.
    */
    STEVT_Print(( "Test Case 4: Register with a termed device test - DISABLED\n"));
#else
    Error = STEVT_Register(Handle1, EVENT1, &EventID);

    if (Error == ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4: Can register with a termed device"))
    }
#endif

    Error = STEVT_Open("Dev3", NULL, &Handle3);
    if (Error == ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4: Can open a termed device"))
    }

    return Error1;
}
