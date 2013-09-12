/******************************************************************************\
 *
 * File Name   : TComp_6_1.c
 *
 * Description : Test STEVT_Notify/NotifySubscriber performances
 *
 * Copyright STMicroelectronics - February 2000
 *
\******************************************************************************/

 /*MJ-> in case 3 ,4, 5 and 6 there are 400 subscription of two Events( 0 and
 1)  So in 400 connection_t structure (One for each open) and 2 event_t sturcture(for Event 0
 and Event 1 each have 400 subscription.

 Before DDTS 12227 a perticular subscriber could be find out in perticular Connection_t strcuture
 is easy because there are only 2 subscription for each connection_t structure

 But After DDTS 12227 a perticular suscriber is found by Event_t structure
 which is having 400 subscriber for each event . so it takes time to find out perticular sbscriber, As
 stated in DDTS.

 But if the situation is reverse it will help to find out perticular suscriber.
 As if any Event(say 1) is subcribed by only perticular subscriber then In that
 Event_t structure the list Subscription_t will have length 1 but that
 perticulat subscriber also subscribe for many events like 2,3,4,5 BLA BLA  BLA BLA
 then it is easy to find that subscriber from Event_t structure.

 We can see the performance improvment if

#define MAXEVENTS  400
#define MAXSUBSCR  2

 */

#include <stdio.h>
#include <stdlib.h>
#include "stlite.h"
#include "stcommon.h"

#include "evt_test.h"


static int NotCB = 0;

#define MAXEVENTS  2
#define MAXSUBSCR  400

static void NotifyCB(STEVT_CallReason_t Reason,
              STEVT_EventConstant_t Event,
              const void *EventData)
{
    NotCB++;
}

static void NotifyDevCB(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    NotCB++;
}

void report(clock_t time1, clock_t time2);
void report2(clock_t time1, clock_t time2, int event, int subs);

ST_ErrorCode_t TComp_6_1()
{
    U32 ErrorNum =0, i=0, subs=0;
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;
    STEVT_Handle_t        HandleReg, HandleSub[MAXSUBSCR];
    STEVT_EventConstant_t Event[MAXEVENTS];
    STEVT_EventID_t       EventID[MAXEVENTS];
    STEVT_SubscriberID_t  SubID[MAXSUBSCR];
    STEVT_SubscribeParams_t       SubscrParam;
    STEVT_DeviceSubscribeParams_t DevSubscrParam;
    clock_t               time1;
    clock_t               time2;


    NotCB = 0;
    DevSubscrParam.NotifyCallback     = (STEVT_DeviceCallbackProc_t)NotifyDevCB;
    DevSubscrParam.SubscriberData_p   = (void *)NULL;

    SubscrParam.NotifyCallback     = (STEVT_CallbackProc_t)NotifyCB;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum   = MAXEVENTS;
    InitParam.ConnectMaxNum = MAXSUBSCR+1;
    InitParam.SubscrMaxNum  = MAXSUBSCR*MAXEVENTS;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    for (i=0; i<MAXEVENTS; i++)
    {
        Event[i]=(STEVT_DRIVER_BASE+i);
    }

    Error = STEVT_Init("Dev6_1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev6_1 initialisation failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open session for the registrant "REG1" (HandleReg) */
    Error = STEVT_Open("Dev6_1", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 3: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open sessions for MAXSUBSCR subscribers */
    for ( subs = 0; subs<MAXSUBSCR; subs++)
    {
        Error = STEVT_Open("Dev6_1", NULL, &(HandleSub[subs]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 3: Opening Subscriber %1d  Handle failed\n",subs));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }


    /* The registrant register MAXEVENTS events receiving back MAXEVENTS EventID*/
    for (i = 0; i<MAXEVENTS/2; i++)
    {
        Error = STEVT_RegisterDeviceEvent (HandleReg, "REG1", Event[i], &(EventID[i]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Test Case 3 EVENT[%1d] registration failed",i));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }

    /* The registrant register MAXEVENTS events receiving back MAXEVENTS EventID*/
    for (i = MAXEVENTS/2; i<MAXEVENTS; i++)
    {
        Error = STEVT_Register (HandleReg, Event[i], &(EventID[i]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Test Case EVENT[%1d] registration failed",i));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Received error 0x%08X",Error));
            return Error;
        }
    }

    /*
     *  The first MAXSUBSCR/2  subscribers subscribe all the events
     *  using a subsrcribe device
     */
    for (subs =0 ; subs<MAXSUBSCR/2; subs++)
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
        Error = STEVT_GetSubscriberID( HandleSub[subs], &(SubID[subs]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Unable to obtain subscription ID"));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Error received 0x%08X",Error));
            return Error;
        }
    }

    /*
     *  The second MAXSUBSCR/2  subscribers subscribe all the events
     *  using a subsrcribe anonymous
     */
    for (subs =( MAXSUBSCR/2) ; subs<MAXSUBSCR; subs++)
    {
        for (i=0; i<MAXEVENTS; i++)
        {
            Error = STEVT_Subscribe (HandleSub[subs], Event[i], &SubscrParam);
            if (Error != ST_NO_ERROR)
            {
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Test Case  EVENT[%2d] subscription failed",i));
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Received error 0x%08X",Error));
                return Error;
            }

        }
        Error = STEVT_GetSubscriberID( HandleSub[subs], &(SubID[subs]));
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Unable to obtain subscription ID"));
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Error received 0x%08X",Error));
            return Error;
        }
    }


    /*************************************************************************/
    /*
     * Case 1) Notify performance for an  Event registered as device event
     */

    time2 = time_now();
    Error = STEVT_Notify(HandleReg, EventID[0], "Notify CALLBACK");
    time1 = time_now();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify "));
        ErrorNum++;
        return ErrorNum;
    }

    if (NotCB != MAXSUBSCR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",MAXSUBSCR,NotCB));
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1"));
        report(time1,time2);
    }


    /*
     * Case 2) Notify performance for an  Event registered as anonymous
     */

    NotCB =0;
    time2 = time_now();
    Error = STEVT_Notify(HandleReg, EventID[MAXEVENTS-1], "Notify CALLBACK");
    time1 = time_now();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify "));
        ErrorNum++;
        return ErrorNum;
    }

    if (NotCB != MAXSUBSCR/2)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",MAXSUBSCR/2,NotCB));
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2"));
        report(time1,time2);
    }

    /*
     * Case 3) NotifySubscribers performance for an  Event registered as device event
     *         to a device event suscribers
     */

    NotCB =0;
    i= MAXSUBSCR/2-1; /* last device event subscriber */
    time2 = time_now();
    Error = STEVT_NotifySubscriber(HandleReg, EventID[0], "Notify CALLBACK", SubID[i]);
    time1 = time_now();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify "));
        ErrorNum++;
        return ErrorNum;
    }

    if (NotCB != 1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3"));
        report2(time1,time2, 0, i);
    }

    /*
     * Case 4) NotifySubscribers performance for an  Event registered as anonymous event
     *         to an anonymous suscriber
     */

    NotCB =0;
    i= MAXSUBSCR-1; /* last anonymous subscriber */
    time2 = time_now();
    Error = STEVT_NotifySubscriber(HandleReg, EventID[MAXEVENTS-1], "Notify CALLBACK", SubID[i]);
    time1 = time_now();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify "));
        ErrorNum++;
        return ErrorNum;
    }

    if (NotCB != 1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4"));
        report2(time1,time2, MAXEVENTS-1, i);
    }

    /*
     * Case 5) NotifySubscribers performance for an  Event registered as anonymous event
     *         to a device event suscriber
     */

    NotCB =0;
    i= MAXSUBSCR/2-1;
    time2 = time_now();
    Error = STEVT_NotifySubscriber(HandleReg, EventID[MAXEVENTS-1], "Notify CALLBACK", SubID[i]);
    time1 = time_now();
    if (Error != STEVT_ERROR_NOT_SUBSCRIBED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to Notify "));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Expected error 0x%08X received 0x%08X", STEVT_ERROR_NOT_SUBSCRIBED,Error));
        ErrorNum++;
    }

    if (NotCB != 0)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has been erroneously called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",0,NotCB));
        STEVT_Print(( "^^^Failed Test Case 5\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 5\n"));
        report2(time1,time2, MAXEVENTS-1, i);
    }

    /*
     * Case 6) NotifySubscribers performance for an  Event registered as device event
     *         to an anonymous suscriber
     */

    NotCB =0;
    i= MAXSUBSCR-1; /* last anonymous subscriber */
    time2 = time_now();
    Error = STEVT_NotifySubscriber(HandleReg, EventID[0], "Notify CALLBACK", SubID[i]);
    time1 = time_now();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Unable to Notify "));
        ErrorNum++;
    }

    if (NotCB != 1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
        STEVT_Print(( "^^^Failed Test Case 6\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 6"));
        report2(time1,time2, 0, i);
    }

    /*************************************************************************/
    Error = STEVT_Term("Dev6_1", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed"))
        return Error;
    }

    return ErrorNum;
}

void report(clock_t time1, clock_t time2)
{
    clock_t el_time;

    el_time=(time_minus(time1,time2)*1000000)/ST_GetClocksPerSecond();

    STEVT_Print(("\n----------------------------------------\n"));
    STEVT_Print(("\tChip "));
#if defined(ST_5510)
    STEVT_Print(("STi5510"));
#elif defined(ST_5512)
    STEVT_Print(("STi5512"));
#endif
    STEVT_Print((" board "));
#if defined(mb282b)
    STEVT_Print(("MB282B\n"));
#elif defined(mb275)
    STEVT_Print(("MB275\n"));
#elif defined(mb231)
    STEVT_Print(("MB231\n"));
#endif
    STEVT_Print(("\tCPU Clock Speed = %d\n", ST_GetClockSpeed() ));
    STEVT_Print(("\tNumber of events      = %d\n",MAXEVENTS));
    STEVT_Print(("\tNumber of subscribers = %d\n",MAXSUBSCR));
    STEVT_Print(("\tElapsed time %lu us\n",el_time));
    STEVT_Print(("----------------------------------------\n"));
}


void report2(clock_t time1, clock_t time2, int event, int subs)
{
    clock_t el_time;

    el_time=(time_minus(time1,time2)*1000000)/ST_GetClocksPerSecond();

    STEVT_Print(("\n----------------------------------------\n"));
    STEVT_Print(("\tChip "));
#if defined(ST_5510)
    STEVT_Print(("STi5510"));
#elif defined(ST_5512)
    STEVT_Print(("STi5512"));
#endif
    STEVT_Print((" board "));
#if defined(mb282b)
    STEVT_Print(("MB282B\n"));
#elif defined(mb275)
    STEVT_Print(("MB275\n"));
#elif defined(mb231)
    STEVT_Print(("MB231\n"));
#endif
    STEVT_Print(("\tCPU Clock Speed = %d\n", ST_GetClockSpeed() ));
    STEVT_Print(("\tNumber of the event      = %d\n",event));
    STEVT_Print(("\tNumber of the subscriber = %d\n",subs));
    STEVT_Print(("\tElapsed time %lu us\n",el_time));
    STEVT_Print(("----------------------------------------\n"));
}
