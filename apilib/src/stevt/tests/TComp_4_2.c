/******************************************************************************\
 *
 * File Name   : TComp_4_2.c
 *
 * Description : Test STEVT_NotifySubscriber Interface
 *
 * Copyright STMicroelectronics - February 2000
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
static int NotCB2 = 0;
static int RegCB = 0;
static int UnregCB = 0;

ST_ErrorCode_t TCase_4_2_1(void);
ST_ErrorCode_t TCase_4_2_2(void);
ST_ErrorCode_t TCase_4_2_3(void);
ST_ErrorCode_t TCase_4_2_4(void);

void NotCB4_2(STEVT_CallReason_t Reason,
              STEVT_EventConstant_t Event,
              const void *EventData)
{
    NotCB++;
}

void RegCB4_2(STEVT_CallReason_t Reason,
                STEVT_EventConstant_t Event,
                const void *EventData)
{
    RegCB++;
}

void UnregCB4_2(STEVT_CallReason_t Reason,
                  STEVT_EventConstant_t Event,
                  const void *EventData)
{
   UnregCB++;
}

void NotDevCB4_2(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    NotCB++;
}

void NotDev2CB4_2(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    NotCB2++;
}

void RegDevCB4_2(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    RegCB++;
}

void UnregDevCB4_2(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
   UnregCB++;
}

ST_ErrorCode_t TComp_4_2()
{
    U32 ErrorNum =0;
    ST_ErrorCode_t Error;
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

    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"))
        return Error;
    }

    /*************************************************************************/
    /*
     * Case 1) Old Subscriber - Old Registrant
     */
    Error = TCase_4_2_1();
    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }
    NotCB = 0;
    RegCB = 0;
    UnregCB = 0;

    /*************************************************************************/
    /*
     * Case 2) Old Subscriber - New Registrant
     */
    Error = TCase_4_2_2();
    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }
    NotCB = 0;
    RegCB = 0;
    UnregCB = 0;

    /*************************************************************************/
    /*
     * Case 3) New Subscriber - New Registrant
     */
    Error = TCase_4_2_3();
    if (Error != 0)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        ErrorNum++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }
    NotCB = 0;
    RegCB = 0;
    UnregCB = 0;

    /*************************************************************************/
    /*
     * Case 4)
     */
    Error = TCase_4_2_4();
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
    RegCB = 0;
    UnregCB = 0;

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
 * Notify subscriber test
 * ----------------------
 *
 * Case 1) Old Subscriber - Old Registrant
 *
 */
ST_ErrorCode_t TCase_4_2_1()
{
    STEVT_Handle_t HandleReg, HandleSub;
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;
    STEVT_SubscriberID_t SubID;
    STEVT_SubscribeParams_t SubscrParam;

    NotCB = 0;
    SubscrParam.NotifyCallback     = (STEVT_CallbackProc_t)NotCB4_2;

    /* Open session for the registrant "REG1" (HandleReg) */
    Error = STEVT_Open("Dev1", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 1: Opening Registrant Handle failed\n"));
        return Error;
    }

    /* Open session for the subscriber "SUB1" (HandleSub) */
    Error = STEVT_Open("Dev1", NULL, &HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 1: Opening Subscriber Handle failed\n"));
        return Error;
    }

    Error = STEVT_Register(HandleReg, EVENT1, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT1 registration failed"));
        return Error;
    }

    Error = STEVT_Subscribe(HandleSub, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 1 EVENT1 subscription failed"));
        return Error;
    }

    Error = STEVT_GetSubscriberID( HandleSub, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error));
        return Error;
    }

    Error = STEVT_NotifySubscriber(HandleReg, EventID, "Notify CALLBACK", SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify a specified subscriber "));
        return Error;
    }

    if (NotCB != 1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
        return Error;
    }

    Error = STEVT_Close(HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    Error = STEVT_Close(HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    return ST_NO_ERROR;
}

/*
 * Notify subscriber test
 * ----------------------
 *
 * Case 2) Old Subscriber - New Registrant
 *
 */
ST_ErrorCode_t TCase_4_2_2()
{
    STEVT_Handle_t HandleReg, HandleSub;
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;
    STEVT_SubscriberID_t SubID;
    STEVT_SubscribeParams_t SubscrParam;

    NotCB = 0;
    SubscrParam.NotifyCallback     = (STEVT_CallbackProc_t)NotCB4_2;

    /* Open session for the registrant "REG1" (HandleReg) */
    Error = STEVT_Open("Dev1", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 2: Opening Registrant Handle failed\n"));
        return Error;
    }

    /* Open session for the subscriber "SUB1" (HandleSub) */
    Error = STEVT_Open("Dev1", NULL, &HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 2: Opening Subscriber Handle failed\n"));
        return Error;
    }

    Error = STEVT_RegisterDeviceEvent (HandleReg, "REG1", EVENT2, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 registration failed"));
        return Error;
    }

    Error = STEVT_Subscribe(HandleSub, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 2 EVENT2 subscription failed"));
        return Error;
    }

    Error = STEVT_GetSubscriberID( HandleSub, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error));
        return Error;
    }

    Error = STEVT_NotifySubscriber(HandleReg, EventID, "Notify CALLBACK", SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify a specified subscriber "));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    if (NotCB != 1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
        return Error;
    }

    Error = STEVT_Close(HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    Error = STEVT_Close(HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    return ST_NO_ERROR;
}

/*
 * Notify subscriber test
 * ----------------------
 *
 * Case 3) New Subscriber - New Registrant
 *
 */
ST_ErrorCode_t TCase_4_2_3()
{
    STEVT_Handle_t HandleReg, HandleSub;
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;
    STEVT_SubscriberID_t SubID;
    STEVT_DeviceSubscribeParams_t SubscrParam;

    NotCB = 0;
    SubscrParam.NotifyCallback     = (STEVT_DeviceCallbackProc_t)NotDevCB4_2;
    SubscrParam.SubscriberData_p   = (void *)NULL;

    /* Open session for the registrant "REG1" (HandleReg) */
    Error = STEVT_Open("Dev1", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 3: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open session for the subscriber "SUB1" (HandleSub) */
    Error = STEVT_Open("Dev1", NULL, &HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 3: Opening Subscriber Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_RegisterDeviceEvent (HandleReg, "REG1", EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 EVENT3 registration failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent (HandleSub, "REG1", EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 3 EVENT3 subscription failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_GetSubscriberID( HandleSub, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error));
        return Error;
    }

    Error = STEVT_NotifySubscriber(HandleReg, EventID, "Notify CALLBACK", SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify a specified subscriber "));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    if (NotCB != 1)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify callBack has not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected %d notify calls received %d",1,NotCB));
        return Error;
    }

    Error = STEVT_Close(HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    Error = STEVT_Close(HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    return ST_NO_ERROR;
}

/*
 * Notify subscriber test
 * ----------------------
 *
 * Case 4) A new subscriber suscribed twice to the same event with different
 *         registrants - New Registrant
 *
 */
ST_ErrorCode_t TCase_4_2_4()
{
    STEVT_Handle_t HandleReg, HandleSub;
    ST_ErrorCode_t Error;
    STEVT_EventID_t EventID;
    STEVT_SubscriberID_t SubID;
    STEVT_DeviceSubscribeParams_t SubscrParam;
    STEVT_DeviceSubscribeParams_t SubscrParam2;

    NotCB = 0;
    SubscrParam.NotifyCallback     = (STEVT_DeviceCallbackProc_t)NotDevCB4_2;
    SubscrParam.SubscriberData_p   = (void *)NULL;

    NotCB2 = 0;
    SubscrParam2.NotifyCallback     = (STEVT_DeviceCallbackProc_t)NotDev2CB4_2;
    SubscrParam2.SubscriberData_p   = (void *)NULL;

    /* Open session for the registrant "REG1" (HandleReg) */
    Error = STEVT_Open("Dev1", NULL, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 4: Opening Registrant Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    /* Open session for the subscriber "SUB1" (HandleSub) */
    Error = STEVT_Open("Dev1", NULL, &HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,"Test Case 4: Opening Subscriber Handle failed\n"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_RegisterDeviceEvent (HandleReg, "REG1", EVENT3, &EventID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 registration failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent (HandleSub, "REG2", EVENT3, &SubscrParam2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 subscription failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent (HandleSub, "REG1", EVENT3, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Test Case 4 EVENT3 subscription failed"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Received error 0x%08X",Error));
        return Error;
    }

    Error = STEVT_GetSubscriberID( HandleSub, &SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to obtain subscription ID"));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error received 0x%08X",Error));
        return Error;
    }

    /*
     * HandleReg is owned by REG1 so only the callback of the second subscribe
     * should be invoked => only NotCB should be incremented.
     */
    Error = STEVT_NotifySubscriber(HandleReg, EventID, "Notify CALLBACK", SubID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Unable to Notify a specified subscriber "));
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    if ((NotCB != 1) ||
        (NotCB2 != 0))
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Notify CallBacks have not been correctly called"));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected NotCB = %d notify calls received %d",1,NotCB));
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                     "Expected NotCB2 = %d notify calls received %d",0,NotCB2));
        return Error;
    }

    Error = STEVT_Close(HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    Error = STEVT_Close(HandleSub);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Connection close failed"))
        return Error;
    }

    return ST_NO_ERROR;
}
