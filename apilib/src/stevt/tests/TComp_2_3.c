/******************************************************************************\
 *
 * File Name   : TComp_2_3.c
 *
 * Description : Test STEVT_Unregister Interface
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

static int RegCB = 0;

static int UnregCB = 0;

ST_ErrorCode_t TCase_2_3_1(STEVT_Handle_t Handle);
ST_ErrorCode_t TCase_2_3_2(void);
ST_ErrorCode_t TCase_2_3_3(void);
ST_ErrorCode_t TCase_2_3_4(STEVT_Handle_t Handle);
ST_ErrorCode_t TCase_2_3_5(STEVT_Handle_t Handle);

void NotifCB2_3(STEVT_CallReason_t Reason,
              STEVT_EventConstant_t Event,
              const void *EventData)
{
    STEVT_Print(("%s\n", (char *)EventData));
}

void RegCB2_3(STEVT_CallReason_t Reason,
                STEVT_EventConstant_t Event,
                const void *EventData)
{
    RegCB++;
}

void UnregCB2_3(STEVT_CallReason_t Reason,
                  STEVT_EventConstant_t Event,
                  const void *EventData)
{
    UnregCB++;
}

ST_ErrorCode_t TComp_2_3()
{
    ST_ErrorCode_t Error;
    U32 ErrorNum=0;
    STEVT_InitParams_t InitParam;
    STEVT_SubscribeParams_t SubscrParam;
    STEVT_Handle_t Handle, Handle1, Handle2;
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

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotifCB2_3;

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

    Error = STEVT_Subscribe(Handle, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT1 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT1 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT4, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT4 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT4, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT4 subscription by Handle1 failed"))
        return Error;
    }

    /********************************************************************/
    Error = TCase_2_3_1(Handle1);
    if (Error!=0)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }
    RegCB = 0;

    /********************************************************************/
    Error = TCase_2_3_2();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    /********************************************************************/
    Error = TCase_2_3_3();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    /********************************************************************/
    Error = TCase_2_3_4(Handle2);
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
    UnregCB = 0;

    /********************************************************************/
    Error = TCase_2_3_5(Handle2);

    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 5\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 5\n"));
    }

    RegCB = 0;
    UnregCB = 0;
    /********************************************************************/

    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 termination failed"))
        return Error;
    }

    return ErrorNum;
}

/*  This test checks that ST_ERROR_INVALID_HANDLE is returned if
 *  the UserHandle is not the Event Registrant */
ST_ErrorCode_t TCase_2_3_1(STEVT_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle3;
    STEVT_EventID_t EventID;

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Handle3 opening in Dev1 failed"))
        return Error;
    }

    /*  The Event EVENT1 is not registered */
    Error = STEVT_Unregister(Handle, EVENT1);
    if (Error != ST_ERROR_INVALID_HANDLE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Invalid user to unregister"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Expected error 0x%08X, received 0x%08X",ST_ERROR_INVALID_HANDLE,Error));
        return 1;
    }

    Error = STEVT_Register(Handle3, EVENT1, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT1 registering by Handle3 failed"))
        return Error;
    }

    /*  EVENT1 Registrant is Handle3 not Handle */
    Error = STEVT_Unregister(Handle, EVENT1);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 Invalid user to unregister"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Expected error 0x%08X, received 0x%08X",ST_ERROR_INVALID_HANDLE,Error));
        return 1;
    }

    return ST_NO_ERROR;
}

/*  This test checks that ST_ERROR_INVALID_HANDLE is returned if
 *  Handle has been previously closed */
ST_ErrorCode_t TCase_2_3_2(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle4;

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle4 closing  failed"))
        return Error;
    }

    Error = STEVT_Unregister(Handle4, EVENT1);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle4 still valid after the closing "))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Expected error 0x%08X, received 0x%08X",ST_ERROR_INVALID_HANDLE,Error));
        return 1;
    }

    return ST_NO_ERROR;
}

/*  This test checks that STEVT_ERROR_INVALID_EVENT_NAME is returned
 *  if the named Event is not in the Event List */
ST_ErrorCode_t TCase_2_3_3(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle4;
    STEVT_EventID_t EventID;

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 Handle4 opening failed"))
        return Error;
    }

    Error = STEVT_Register(Handle4, EVENT2, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 EVENT2 registering by Handle4 failed"))
        return Error;
    }

    /*
     * Handle4 is a registrant for EVENT2 but try to unregister EVENT3
     * EVENT3 is not in the Event List
     */

    Error = STEVT_Unregister(Handle4, EVENT3);
    if (Error != STEVT_ERROR_INVALID_EVENT_NAME)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 EVENT3 unregistered but never registered"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Expected error 0x%08X, received 0x%08X",STEVT_ERROR_INVALID_EVENT_NAME,Error));
        return 1;
    }

    return ST_NO_ERROR;
}

/*
 *  If an Event is unregistered twice and is no more referenced
 *  return STEVT_ERROR_INVALID_EVENT_NAME otherwise returns
 *  ST_ERROR_INVALID_HANDLE
 */
ST_ErrorCode_t TCase_2_3_4(STEVT_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    STEVT_EventConstant_t EventID, EventID1;

    Error = STEVT_Register(Handle, EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 registering by Handle2 failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Error received # 0x%08X",Error));
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT4, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT4 registering by Handle2 failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Error received # 0x%08X",Error));
        return Error;
    }

    /* EVENT3 has not any subscription */
    Error = STEVT_Unregister(Handle, EVENT3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 first unregistering failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Error received # 0x%08X",Error));
        return Error;
    }

    /*
     * EVENT3 is already unregistered by this user
     */
    Error = STEVT_Unregister(Handle, EVENT3);
    if (Error != STEVT_ERROR_INVALID_EVENT_NAME)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Test Case 4 EVENT3 second unregister"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Expected error 0x%08X, received 0x%08X",STEVT_ERROR_INVALID_EVENT_NAME,Error));
        return 1;
    }

    /*  EVENT4 has subscription */
    Error = STEVT_Unregister(Handle, EVENT4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT4 first unregistering failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Error received # 0x%08X",Error));
        return Error;
    }

    Error = STEVT_Unregister(Handle, EVENT4);
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Test Case 4 EVENT4 second unregister"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Expected error 0x%08X, received 0x%08X",ST_ERROR_INVALID_HANDLE,Error));
        return 1;
    }

    return ST_NO_ERROR;
}

/*  All works well, for an Event without any subscription and for an Event
 *  with some subscriptions */
ST_ErrorCode_t TCase_2_3_5(STEVT_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    STEVT_EventConstant_t EventID, EventID1;

    Error = STEVT_Register(Handle, EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT3 registering by Handle failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Error received # 0x%08X",Error));
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT4, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT4 registering by Handle failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Error received # 0x%08X",Error));
        return Error;
    }

    /*  Unregister without any subscription */
    Error = STEVT_Unregister(Handle, EVENT3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT3 unregistering by Handle failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Error received # 0x%08X",Error));
        return Error;
    }

    /*  Unregister without some subscriptions */
    Error = STEVT_Unregister(Handle, EVENT4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT4 unregistering by Handle failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Error received # 0x%08X",Error));
        return Error;
    }

    return Error;
}


