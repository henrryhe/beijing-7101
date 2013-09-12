
/******************************************************************************\
 *
 * File Name   : TComp_2_2.c
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



static int NotCB = 0;
static int NotDevCB = 0;

ST_ErrorCode_t TCase_2_2_1(STEVT_SubscribeParams_t SubscrParam);
ST_ErrorCode_t TCase_2_2_2(STEVT_SubscribeParams_t);
ST_ErrorCode_t TCase_2_2_4( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam);
ST_ErrorCode_t TCase_2_2_5( STEVT_SubscribeParams_t SubscrParam);
ST_ErrorCode_t TCase_2_2_6( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam);
ST_ErrorCode_t TCase_2_2_7( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam);
ST_ErrorCode_t TCase_2_2_8( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam);
ST_ErrorCode_t TCase_2_2_9( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam);
ST_ErrorCode_t TCase_2_2_10( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam);


void NotifyCall(STEVT_CallReason_t Reason,
             STEVT_EventConstant_t Event,
             const void *EventData)
{
/*     STEVT_Print(("NotifyCall:\n")); */

    NotCB++;
}

void NotifyDevCall(STEVT_CallReason_t Reason,
                const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event,
                const void *EventData,
                const void *SubscriberData_p)
{
    NotDevCB++;
}

void NotifyDevCallEcho(STEVT_CallReason_t Reason,
                const ST_DeviceName_t RegistrantName,
                STEVT_EventConstant_t Event,
                const void *EventData,
                const void *SubscriberData_p)
{
    NotDevCB++;

    if (RegistrantName == NULL )
    {
        STEVT_Print(("\t\t\tNotifyDevCall: Null device name\n"));
    }
    else
    {
        STEVT_Print(("\t\t\tNotifyDevCall: Device name %s\n",RegistrantName));
    }

    STEVT_Print(("\t\t\tEvent data   : %s\n",(char *)EventData));


}

ST_ErrorCode_t TComp_2_2()
{
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_SubscribeParams_t SubscrParam;
    STEVT_DeviceSubscribeParams_t SubDevParam;
    STEVT_DeviceSubscribeParams_t SubDevParamEcho;
    STEVT_TermParams_t TermParam;
    U32 ErrorNum = 0;

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

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)NotifyCall;

    SubDevParam.NotifyCallback = (STEVT_DeviceCallbackProc_t)NotifyDevCall;

    SubDevParamEcho.NotifyCallback = (STEVT_DeviceCallbackProc_t)NotifyDevCallEcho;

    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"))
        return Error;
    }

    /*
     * Registering  EVENT1
     * Registrant is also a subscriber.
     */
    Error = TCase_2_2_1(SubscrParam);
    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
          ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
   }




    /*
     * Registering EVENT2 with 2 subscribers
     *
     * An EVENT can be registered only once using STEVT_Register()
     * also in case of different instances of the same driver
     */
    Error = TCase_2_2_2(SubscrParam);
    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    STEVT_Print(( "^^^Removed Test Case 3\n"));

    Error = TCase_2_2_4(SubscrParam, SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
    }

    Error = TCase_2_2_5(SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 5\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 5\n"));
    }

    Error = TCase_2_2_6(SubscrParam, SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 6\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 6\n"));
    }

    Error = TCase_2_2_7(SubscrParam, SubDevParamEcho);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 7\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 7\n"));
    }

    Error = TCase_2_2_8(SubscrParam, SubDevParamEcho);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 8\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 8\n"));
    }

    Error = TCase_2_2_9(SubscrParam, SubDevParamEcho);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 9\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 9\n"));
    }

    Error = TCase_2_2_10(SubscrParam, SubDevParamEcho);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 10\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 10\n"));
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

/* All works well when Registrant is equal to Subscriber */
ST_ErrorCode_t TCase_2_2_1(STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle1, Handle2;

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

    /*
     * Two subscribers to EVENT1
     */
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

    /*
     * Handle is both subscriber then registrant
     */
    Error = STEVT_Register(Handle, EVENT1, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Test Case 1: Fails if the registrant is the same of the subscriber"));
        return Error;
    }


    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle closing in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Close(Handle1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle1 closing in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Close(Handle2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle2 closing in Dev1 failed"))
        return Error;
    }

    return Error;
}

/*
 *  This test checks that if the Event is already
 *  registered corresponding error code is returned
 */
ST_ErrorCode_t TCase_2_2_2( STEVT_SubscribeParams_t SubscrParam)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle, Handle4, Handle5, Handle6;
    STEVT_EventID_t EventID, EventID1;


    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle5);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle5 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle6);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle6 opening in Dev1 failed"))
        return Error;
    }

    /*
     * Two subscribers for EVENT2
     */
    Error = STEVT_Subscribe(Handle4, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 Subscription by Handle4 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle6, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 Subscription by Handle6 failed"))
        return Error;
    }

    /*
     * Handle register EVENT2
     */
    Error = STEVT_Register(Handle, EVENT2, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 registering by Handle failed"))
        return Error;
    }

    /*
     *  Here is cheked if the Error is returned also when
     *  the registration is made by the same handle
     */
    Error = STEVT_Register(Handle, EVENT2, &EventID1);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"No more than one Register() is allowed in a single instance"));
        return 1;
    }

   /*
    * Also Handle5 try to register EVENT2, but fails
    */
    Error = STEVT_Register(Handle5, EVENT2, &EventID1);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"No more than one anonymous Register() is allowed also in different instances"));
        return 1;
    }

    /*
     * This call should be formally the same of the previous one
     */
    Error = STEVT_RegisterDeviceEvent(Handle5, NULL, EVENT2, &EventID1);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"RegisterDevice() with a NULL devicename is formally a Register()"));
        return 1;
    }

    /*
     * But all should work with a register device specifing a registrant name
     */
    Error = STEVT_RegisterDeviceEvent(Handle5, "Pippo", EVENT2, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 registe device by Handle5 failed"))
        return Error;
    }
    /*
     * Two register callbacks should be called also
     * if subscribed using Subscribe()
     */

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle closing failed"))
        return Error;
    }


    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle4 closing failed"))
        return Error;
    }


    Error = STEVT_Close(Handle6);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle6 closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle5);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 Handle4 closing failed"))
        return Error;
    }

    return ST_NO_ERROR;
}


ST_ErrorCode_t TCase_2_2_4( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle4;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle4 opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle4, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 subscription by Handle4 failed"))
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent(Handle4, "Pippo", EVENT3, &SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 subscription by Handle5 failed"))
        return Error;
    }

    Error = STEVT_RegisterDeviceEvent(Handle, "Pippo", EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT2 registering by Handle failed"))
        return Error;
    }

    Error = STEVT_Notify( Handle, EventID, "Test Notify CallBack");
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT2 Notifying failed"))
        return Error;
    }

    if (NotCB !=1)

    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Notify callbacks has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",1,NotCB));

        return 1;
    }

    if (NotDevCB !=1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device Event Notify callbacks has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",1,NotDevCB));
        return 1;
    }
    NotCB = 0;
    NotDevCB = 0;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 Handle4 closing failed"))
        return Error;
    }

    return Error;
}



/*
 * Backward compatibilty:
 * a register device event after two subscribe causes
 * the call of the callbacks
 */
ST_ErrorCode_t TCase_2_2_5( STEVT_SubscribeParams_t SubscrParam)

{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle4, Handle5;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle opening in Dev1 failed"))
        return Error;
    }

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

    Error = STEVT_Subscribe(Handle4, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT3 subscription by Handle4 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle5, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT3 subscription by Handle5 failed"))
        return Error;
    }

    Error = STEVT_RegisterDeviceEvent(Handle, "Pippo", EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 registering by Handle failed"))
        return Error;
    }

    Error = STEVT_Notify( Handle, EventID, "Test Notify CallBack");
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 EVENT2 Notifying failed"))
        return Error;
    }

    if (NotCB !=2)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Notify callbacks has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",2,NotCB));
        return 1;
    }
    NotCB = 0;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle4 closing failed"))
        return Error;
    }


    Error = STEVT_Close(Handle5);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 5 Handle5 closing failed"))
        return Error;
    }

    return Error;
}



ST_ErrorCode_t TCase_2_2_6( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_Handle_t Handle;

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 6 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent(Handle, "Pippo", EVENT3, &SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 6 EVENT3 subscription by Handle5 failed"))
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent(Handle, "Pippo", EVENT3, &SubDevParam);
    if (Error != STEVT_ERROR_ALREADY_SUBSCRIBED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 6 EVENT3 subscription by Handle5 failed"))
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 6 EVENT3 subscription by Handle failed"))
        return Error;
    }

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 6 Handle closing failed"))
        return Error;
    }

    return Error;
}

/*
 * Test for DDTS GNBvd05383
 *
 */
ST_ErrorCode_t TCase_2_2_7( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle4;
    char EventData[]="TEST 7 Notify Event data";

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 Handle4 opening in Dev1 failed"))
        return Error;
    }

    /*
     * Old event registered by Handle
     */
    Error = STEVT_Register(Handle, EVENT4, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 EVENT4 registering by Handle failed"))
        return Error;
    }

    /*
     * New Subscription to an old event using a NULL name
     */

    Error = STEVT_SubscribeDeviceEvent(Handle4, NULL, EVENT4, &SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 EVENT4 subscription by Handle5 failed"))
        return Error;
    }

#if defined(ST_OSLINUX)
    /* We must know the size of the event data for linux. */
    /* For most cases ity is ok to use the macro:
       #define STEVT_Notify(a,b,c) STEVT_NotifyWithSize(a,b,c,sizeof(c))
       This works when the event data points to a type that is the size
       of the data. In this case it does not and we have to pass the size explicitly.*/
    Error = STEVT_NotifyWithSize( Handle, EventID, (const void *)EventData,strlen(EventData) );
#else
    Error = STEVT_Notify( Handle, EventID, (const void *)EventData);
#endif
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 EVENT2 Notifying failed"))
        return Error;
    }

    if (NotDevCB !=1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device Event Notify callbacks has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",1,NotDevCB));
        return 1;
    }
    NotDevCB = 0;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 7 Handle4 closing failed"))
        return Error;
    }

    return Error;
}

/*
 * Miscellanous test: new subscribe device and old registrant
 * Register call back should be called saying that the registrant
 * is anonymous.
 * Registered before subscription.
 */
ST_ErrorCode_t TCase_2_2_8( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle4;
    char EventData[]="TEST 8 Notify Event data";

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 Handle4 opening in Dev1 failed"))
        return Error;
    }

    /*
     * Old event registered by Handle
     */
    Error = STEVT_Register(Handle, EVENT4, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 EVENT4 registering by Handle failed"))
        return Error;
    }

    /*
     * New Subscription to an old event using a NULL name
     */
    Error = STEVT_SubscribeDeviceEvent(Handle4, "Pippo", EVENT4, &SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 EVENT4 subscription by Handle5 failed"))
        return Error;
    }

    NotDevCB = 0;

#if defined(ST_OSLINUX)
    /* We must know the size of the event data for linux. */
    /* For most cases ity is ok to use the macro:
       #define STEVT_Notify(a,b,c) STEVT_NotifyWithSize(a,b,c,sizeof(c))
       This works when the event data points to a type that is the size
       of the data. In this case it does not and we have to pass the size explicitly.*/
    Error = STEVT_NotifyWithSize( Handle, EventID, (const void *)EventData,strlen(EventData) );
#else
    Error = STEVT_Notify( Handle, EventID, (const void *)EventData);
#endif

    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 EVENT2 Notifying failed"))
        return Error;
    }

    if (NotDevCB !=0)

    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device Event Notify callbacks has been erroneously called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",0,NotDevCB));
        return 1;
    }
    NotDevCB = 0;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 8 Handle4 closing failed"))
        return Error;
    }

    return Error;
}
/*
 * Miscellanous test: new subscribe device and old registrant
 * Register call bach should be called saying that the registrant
 * is anonymous.
 * Registered after subscription.
 */
ST_ErrorCode_t TCase_2_2_9( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle4;
    char EventData[]="TEST 9 Notify Event data";

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 Handle4 opening in Dev1 failed"))
        return Error;
    }

    /*
     * New Subscription to an old event
     */
    Error = STEVT_SubscribeDeviceEvent(Handle4, "REG#1", EVENT4, &SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 EVENT4 subscription by Handle5 failed"))
        return Error;
    }

    /*
     * Old event registered by Handle
     */
    Error = STEVT_Register(Handle, EVENT4, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 EVENT4 registering by Handle failed"))
        return Error;
    }

    NotDevCB = 0;

#if defined(ST_OSLINUX)
    /* We must know the size of the event data for linux. */
    /* For most cases ity is ok to use the macro:
       #define STEVT_Notify(a,b,c) STEVT_NotifyWithSize(a,b,c,sizeof(c))
       This works when the event data points to a type that is the size
       of the data. In this case it does not and we have to pass the size explicitly.*/
    Error = STEVT_NotifyWithSize( Handle, EventID, (const void *)EventData,strlen(EventData) );
#else
    Error = STEVT_Notify( Handle, EventID, (const void *)EventData);
#endif
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 EVENT2 Notifying failed"))
        return Error;
    }

    if (NotDevCB !=0)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device Event Notify callbacks has been erroneously called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",0,NotDevCB));
        return 1;
    }
    NotDevCB = 0;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 9 Handle4 closing failed"))
        return Error;
    }

    return Error;
}

/*
 * Test for DDTS GNBvd05383
 *
 * New subscriber specifying NULL as registrant name and new registrant
 * For backward compatibility the subscriber should be called in any case.
 *
 */
ST_ErrorCode_t TCase_2_2_10( STEVT_SubscribeParams_t SubscrParam,
                            STEVT_DeviceSubscribeParams_t SubDevParam)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventID_t EventID;
    STEVT_Handle_t Handle, Handle4;
    char EventData[]="TEST 10 DDTS GNBvd05383";

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 Handle opening in Dev1 failed"))
        return Error;
    }

    Error = STEVT_Open("Dev1", NULL, &Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 Handle4 opening in Dev1 failed"))
        return Error;
    }

    /*
     * Old event registered by Handle
     */
    Error = STEVT_RegisterDeviceEvent(Handle, "TC_10", EVENT4, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 EVENT4 registering by Handle failed"))
        return Error;
    }

    /*
     * New Subscription to an old event using a NULL name
     */
    Error = STEVT_SubscribeDeviceEvent(Handle4, NULL, EVENT4, &SubDevParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 EVENT4 subscription by Handle5 failed"))
        return Error;
    }

    NotDevCB = 0;

#if defined(ST_OSLINUX)
    /* We must know the size of the event data for linux. */
    /* For most cases ity is ok to use the macro:
       #define STEVT_Notify(a,b,c) STEVT_NotifyWithSize(a,b,c,sizeof(c))
       This works when the event data points to a type that is the size
       of the data. In this case it does not and we have to pass the size explicitly.*/
    Error = STEVT_NotifyWithSize( Handle, EventID, (const void *)EventData,strlen(EventData) );
#else
    Error = STEVT_Notify( Handle, EventID, (const void *)EventData);
#endif

    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 EVENT2 Notifying failed"))
        return Error;
    }

    if (NotDevCB !=1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Device Event Notify callbacks has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Expected %d, Received %d",1,NotDevCB));
        return 1;
    }
    NotDevCB = 0;

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 Handle closing failed"))
        return Error;
    }

    Error = STEVT_Close(Handle4);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 10 Handle4 closing failed"))
        return Error;
    }

    return Error;
}
