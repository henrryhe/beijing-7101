/******************************************************************************\
 *
 * File Name   : TComp_2_1.c
 *
 * Description : Test STEVT_Register Interface
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

ST_ErrorCode_t TCase_2_1_1(void);
ST_ErrorCode_t TCase_2_1_2(STEVT_Handle_t Handle, STEVT_Handle_t Handle4);
ST_ErrorCode_t TCase_2_1_3(void);


ST_ErrorCode_t TComp_2_1()
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_Handle_t Handle, Handle1, Handle2, Handle4;
    STEVT_EventID_t EventID;
    STEVT_TermParams_t TermParam;
    U32 ErrorNum=0;

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

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed in Dev1"))
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


    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle4 opening failed in Dev1"))
        return Error;
    }

/*     Error = TCase_2_1_1(); */
/*     if (Error != ST_NO_ERROR) */
/*     { */
/*         STEVT_Print(( "^^^Failed Test Case 1\n")); */
/*         ErrorNum++; */
/*     } */
/*     else */
/*     { */
/*         STEVT_Print(( "^^^Passed Test Case 1\n")); */
/*     } */
    STEVT_Print(( "^^^Removed Test Case 1\n"));

    Error = TCase_2_1_2(Handle, Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_2_1_3();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    /*
     * Test case 2_1_4
     */
    Error = STEVT_Register(Handle1, EVENT4, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

    Error = STEVT_Term("Dev2", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev2 termination failed"))
        return Error;
    }

    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 termination failed"))
        return Error;
    }

    return (ST_ErrorCode_t)ErrorNum;
}

/*
 *  This test checks that when the Event List is full
 *  STEVT_ERROR_NO_MORE_SPACE Error is returned
 */
ST_ErrorCode_t TCase_2_1_1(void)
{
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID1, EventID2, EventID3, EventID4, EventID;
    STEVT_Handle_t Handle2;

    Error = STEVT_Open("Dev2", NULL, &Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Handle2 opening in Dev2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle2, EVENT1, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT1 registering by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle2, EVENT2, &EventID2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT2 registering by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle2, EVENT3, &EventID3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT3 registering by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle2, EVENT4, &EventID4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT4 registering by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle2, EVENT5, &EventID);
    if (Error != STEVT_ERROR_NO_MORE_SPACE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 More event than EventMaxNum has been registered"));
        return ST_ERROR_BAD_PARAMETER; /* Any value != 0 is ok */
    }

    return ST_NO_ERROR;
}

/*
 * An Event can be registered only once using STEVT_Register()
 * Different instances can register the same event using
 * STEVT_RegisterDeviceEvent().
 */
ST_ErrorCode_t TCase_2_1_2(STEVT_Handle_t Handle,STEVT_Handle_t Handle4)
{
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID, EventID1;

    Error = STEVT_Register(Handle, EVENT2, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 First instance registering failed"))
        return Error;
    }

    /*
     *  A second handle try to register EVENT2
     *  This is an error; two different instances of the
     *  same driver can register the same event only using
     *  STEVT_RegistereDeviceEvent()
     */
    Error = STEVT_Register(Handle4, EVENT2, &EventID1);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Second instance registering the same event"))
        return ST_ERROR_BAD_PARAMETER; /* Any value != 0 is ok */
    }

    /*
     *  Here is cheked if the STEVT_ERROR_ALREADY_REGISTERED is
     *  returned when the registration is made by the same
     *  handle
     */
    Error = STEVT_Register(Handle, EVENT2, &EventID1);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Error: the same event has been registered twice by the same registrant"))
        return ST_ERROR_BAD_PARAMETER; /* Any value != 0 is ok */
    }

    /*
     * Here is checked if a register device is allowed for the same event
     * by a different registrant
     */
    Error = STEVT_RegisterDeviceEvent( Handle4, "Pippo", EVENT2, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Second instance register device failed"))
        return Error;
    }

    return ST_NO_ERROR;
}

/*
 *  Check if ST_ERROR_INVALID_HANDLE is returned
 *  after a close
 */
ST_ErrorCode_t TCase_2_1_3()
{
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed in Dev1"))
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 handle close failed"))
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT4, &EventID);
    if (Error != ST_ERROR_INVALID_HANDLE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                     "Test Case 3 Error: EH accept an handle after the close"));
        return ST_ERROR_BAD_PARAMETER; /* Any value != 0 is ok */
    }

    return ST_NO_ERROR;
}
