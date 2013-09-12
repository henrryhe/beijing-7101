/******************************************************************************\
 *
 * File Name   : TComp_4_1.c
 *
 * Description : Test STEVT_Notify Interface
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

static int NotCB = 0;
static int RegCB = 0;
static int UnregCB = 0;
STEVT_SubscribeParams_t SubscrParam;

ST_ErrorCode_t TCase_4_1_2(STEVT_Handle_t Handle);
ST_ErrorCode_t TCase_4_1_8(void );

void NotCB4_1(STEVT_CallReason_t Reason,
              STEVT_EventConstant_t Event,
              const void *EventData)
{
    NotCB++;
}

void RegCB4_1(STEVT_CallReason_t Reason,
                STEVT_EventConstant_t Event,
                const void *EventData)
{
    RegCB++;
}

void UnregCB4_1(STEVT_CallReason_t Reason,
                  STEVT_EventConstant_t Event,
                  const void *EventData)
{
   UnregCB++;
}

ST_ErrorCode_t TComp_4_1()
{
    U32 ErrorNum =0;
    ST_ErrorCode_t Error;
    ST_ErrorCode_t TCError;
    STEVT_InitParams_t InitParam;
    STEVT_Handle_t Handle, Handle1, Handle2, Handle3, Handle4;
    STEVT_EventID_t Event1, Event2;
    STEVT_TermParams_t TermParam;
    STEVT_SubscriberID_t SubID;

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

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotCB4_1;

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

    Error = STEVT_Open("Dev1", NULL, &Handle3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle3 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle, EVENT1, &Event1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT1 registration by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle1, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 subscription by Handle1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle2, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 subscription by Handle2 failed"))
        return Error;
    }

    Error = STEVT_Register(Handle3, EVENT2, &Event2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 registration by Handle3 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle3, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT3 subscription by Handle3 failed"))
        return Error;
    }

    /*************************************************************************/
    /*
     * TCase_4_1_1
     *
     * The user of the notify is not the same of the registrant for that
     * event. Event2 has been registered by Handle3.
     */
    Error = STEVT_Notify(Handle, Event2, "Notify CALLBACK");
    if (Error != STEVT_ERROR_INVALID_EVENT_ID)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    /*************************************************************************/
    /*
     * TCase_4_1_2
     */
    TCError=0;
    Error = TCase_4_1_2(Handle2);

    if (TCError != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }
    RegCB = 0;
    UnregCB = 0;


    /*************************************************************************/
    /*
     * TCase_4_1_3
     *
     * Notify after a close
     */
    TCError=0;
    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Notify(Handle, Event1, "Notify CallBack");
    if (Error != ST_ERROR_INVALID_HANDLE)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    /*************************************************************************/
    /*
     * TCase_4_1_4
     *
     * Normal usage, all correct
     */
    TCError=0;
    Error = STEVT_Notify(Handle3, Event2, "Notify CALLBACK");

    if (NotCB != 2)
    {
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Not. callBack has not been correctly called"));
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",2,NotCB));
       Error++;
    }
    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }
    NotCB = 0;

    /*************************************************************************/
    /*
     * TCase_4_1_5
     *
     *  Test GetSubscriberID / NotifySubscriber
     */
    TCError=0;
    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle4, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 subscription by Handle4 failed"));
        return Error;
    }

    Error = STEVT_GetSubscriberID( Handle4, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        TCError++;
    }

    Error = STEVT_NotifySubscriber(Handle3, Event2, "Notify CALLBACK", SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify a specified subscriber "));
        TCError++;
    }

    if (NotCB != 1)
    {
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Not. callBack has not been correctly called"));
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
       TCError++;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
       TCError++;
    }

    if (TCError != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 5\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 5\n"));
    }
    NotCB = 0;

    /*************************************************************************/
    /*
     * TCase_4_1_6
     *
     *  Negative test GetSubscriberID / NotifySubscriber
     */
    TCError=0;
    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle4, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 subscription by Handle4 failed"));
        return Error;
    }

    Error = STEVT_GetSubscriberID( Handle4, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        TCError++;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    /*
     * At this point SubID should be invalid
     */
    Error = STEVT_NotifySubscriber(Handle3, Event2, "Notify CALLBACK", SubID);
    if (Error != STEVT_ERROR_INVALID_SUBSCRIBER_ID)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "NotifySubscriber expected error 0x%08X received 0x%08X",
                      STEVT_ERROR_INVALID_SUBSCRIBER_ID,Error));
        TCError++;
    }

    if (NotCB != 0)
    {
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Not. callBack has not been correctly called"));
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",0,NotCB));
       TCError++;
    }


    if (TCError != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 6\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 6\n"));
    }
    NotCB = 0;

    /*************************************************************************/
    /*
     * TCase_4_1_7
     *
     *  Negative test GetSubscriberID / NotifySubscriber
     */
    TCError=0;
    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle4, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT2 subscription by Handle4 failed"));
        return Error;
    }

    Error = STEVT_GetSubscriberID( Handle4, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        TCError++;
    }

    /*
     * Handle 4 is not a subscriber for event 2
     */
    Error = STEVT_NotifySubscriber(Handle3, Event2, "Notify CALLBACK", SubID);
    if (Error != STEVT_ERROR_NOT_SUBSCRIBED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "NotifySubscriber expected error 0x%08X received 0x%08X",
                      STEVT_ERROR_NOT_SUBSCRIBED, Error));
        TCError++;
    }
    if (NotCB != 0)
    {
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Not. callBack has not been correctly called"));
       STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",0,NotCB));
       TCError++;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
       TCError++;
    }

    if (TCError != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 7\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 7\n"));
    }
    NotCB = 0;

    /*
     * Invalid usage of a null EventID
     */
    Error = TCase_4_1_8();
    if (Error != ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Failed Test Case 8\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 8\n"));
    }


    /*************************************************************************/
    Error = STEVT_Term("Dev1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed"))
        return Error;
    }

    return ErrorNum;
}


/*
 *  Notify of an unregistered event
 *  This test checks that Unregister remove the registration
 *  and Notify doesn't allow to notify an unregistered event
 */
ST_ErrorCode_t TCase_4_1_2(STEVT_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;

    Error = STEVT_Register(Handle, EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT3 registration by Handle failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        return Error;
    }

    Error = STEVT_Unregister(Handle, EVENT3);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT3 Handle unregistration failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        return Error;
    }

    Error = STEVT_Notify(Handle, EventID, "Notify CALLBACK");
    if (Error != STEVT_ERROR_INVALID_EVENT_ID)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Invalid Event ID accepted"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        return 1;
    }

    return ST_NO_ERROR;
}

/*
 * Null event ID used to notify
 */
ST_ErrorCode_t TCase_4_1_8(void )
{
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID, FakeEventID=0;
    STEVT_Handle_t Handle1, Handle2;

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

    Error = STEVT_Register(Handle1, EVENT4, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT4 registration by Handle failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle2, EVENT4, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EVENT4 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Notify(Handle1, FakeEventID, "Notify CallBack");
    if (Error != STEVT_ERROR_INVALID_EVENT_ID)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Invalid Event ID accepted"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error))
        return 1;
    }
    Error = ST_NO_ERROR;

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    Error = STEVT_Close(Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    return Error;

}
