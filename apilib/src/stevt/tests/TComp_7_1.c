/******************************************************************************\
 *
 * File Name   : TComp_7_1.c
 *
 * Description : Test memory allocation
 *
 * Copyright STMicroelectronics - March 2001
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "stlite.h"
#include "stcommon.h"

#include "evt_test.h"


static int NotCB = 0;


static void NotifyDevCB(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    NotCB++;
}


/*
 * 2 Registrants for 4 events
 * 2 Subscribers to all 4 events for both the registrants
 *
 * =>  4 connnections
 * =>  4 events
 * => 16 subscriptions (each subscriber performs 4 subscrptions for each registrant
 *                      => 8 subscrptions per subscriber)
 */
#define MAXEVENTS  4
#define MAXREGISTRANTS 2
#define MAXSUBSCRIBERS 2

ST_ErrorCode_t TComp_7_1()
{
    U32 ErrorNum =0, i=0, subs=0;
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;
    STEVT_Handle_t        HandleReg1, HandleReg2, HandleSub[MAXSUBSCRIBERS];
    STEVT_EventConstant_t Event[MAXEVENTS];
    STEVT_EventID_t       EventID[MAXEVENTS];
    STEVT_DeviceSubscribeParams_t DevSubscrParam;


    NotCB = 0;
    DevSubscrParam.NotifyCallback     = (STEVT_DeviceCallbackProc_t)NotifyDevCB;
    DevSubscrParam.SubscriberData_p   = (void *)NULL;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum   = MAXEVENTS;
    InitParam.ConnectMaxNum = MAXSUBSCRIBERS + MAXREGISTRANTS;
    InitParam.SubscrMaxNum  = MAXSUBSCRIBERS * MAXEVENTS;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_USER_DEFINED;
    InitParam.MemoryPoolSize  = 2064;

    for (i=0; i<MAXEVENTS; i++)
    {
        Event[i]=(STEVT_DRIVER_BASE+i);
    }

    Error = STEVT_Init("Dev7_1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev7_1 initialisation failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open session for the registrant "REG1" (HandleReg) */
    Error = STEVT_Open("Dev7_1", NULL, &HandleReg1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 1: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open session for the registrant "REG2" (HandleReg) */
    Error = STEVT_Open("Dev7_1", NULL, &HandleReg2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 1: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open sessions for MAXSUBSCR subscribers */
    for (subs = 0; subs<MAXSUBSCRIBERS; subs++)
    {
        Error = STEVT_Open("Dev7_1", NULL, &(HandleSub[subs]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 3: Opening Subscriber %1d  Handle failed\n",subs));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }


    /* The registrant REG1 registers MAXEVENTS events receiving back MAXEVENTS EventID*/
    for (i = 0; i<MAXEVENTS; i++)
    {
        Error = STEVT_RegisterDeviceEvent (HandleReg1, "REG1", Event[i], &(EventID[i]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Test Case 1 EVENT[%1d] registration failed",i));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }

    /* The registrant REG2 registeris MAXEVENTS events receiving back MAXEVENTS EventID*/
    for (i = 0; i<MAXEVENTS; i++)
    {
        Error = STEVT_RegisterDeviceEvent (HandleReg2, "REG2", Event[i], &(EventID[i]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Test Case 1 EVENT[%1d] registration failed",i));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }

    /*
     *  MAXSUBSCR  subscribers subscribe all the events of REG1
     */
    for (subs =0 ; subs<MAXSUBSCRIBERS; subs++)
    {
        for (i=0; i<MAXEVENTS; i++)
        {
            Error = STEVT_SubscribeDeviceEvent (HandleSub[subs], "REG1", Event[i], &DevSubscrParam);
            if (Error != ST_NO_ERROR)
            {
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Test Case  EVENT[%2d] subscription failed",i));
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Received error 0x%08X",Error));
                return Error;
            }

        }
    }


    /*
     *  MAXSUBSCR  subscribers subscribe all the events of REG2
     */
    for (subs =0 ; subs<MAXSUBSCRIBERS; subs++)
    {
        for (i=0; i<MAXEVENTS; i++)
        {
            Error = STEVT_SubscribeDeviceEvent (HandleSub[subs], "REG2", Event[i], &DevSubscrParam);
            if (Error != ST_NO_ERROR)
            {
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Test Case  EVENT[%2d] subscription failed",i));
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Received error 0x%08X",Error));
                return Error;
            }

        }
    }

    STEVT_Print(("Requested memory is %d\n",STEVT_GetAllocatedMemory(HandleReg1)));

    /*************************************************************************/
    Error = STEVT_Term("Dev7_1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed"))
        return Error;
    }

    if (Error == ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }
    else
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
    }

    return ErrorNum;
}
