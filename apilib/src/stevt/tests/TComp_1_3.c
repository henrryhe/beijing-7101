/******************************************************************************\
 *
 * File Name   : TComp_1_3.c
 *
 * Description : Test STEVT_Subscribe Interface
 *
 * Author      : Manu Jeewani
 *
 * Copyright STMicroelectronics - February 1999
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "evt_test.h"

#define EVENT2  2
#define EVENT3  3

STEVT_SubscribeParams_t         SubscrParam;
STEVT_DeviceSubscribeParams_t   SubscrDevParam;

static ST_ErrorCode_t TCase_1_3_1(void);
static ST_ErrorCode_t TCase_1_3_2(void);
static ST_ErrorCode_t TCase_1_3_3(void);
static ST_ErrorCode_t TCase_1_3_4(void);

void Notify_CallBack(STEVT_CallReason_t Reason,
               STEVT_EventConstant_t Event,
               const void *EventData)
{
    printf("%s %d\n", (char *)EventData, Reason);
}

void Notify_DevCallBack(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    printf("%s %s %d\n", (char *)RegistrantName,(char *)EventData, Reason);
}



U32 TComp_1_3()
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

    Error = STEVT_Init("Dev1", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report(( STTBX_REPORT_LEVEL_ERROR,
                      "Dev1 initialisation failed"));
        NumErrors++;
        return NumErrors;
    }

    Error = TCase_1_3_1();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 1\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 1\n"));
    }

    Error = TCase_1_3_2();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 2\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 2\n"));
    }

    Error = TCase_1_3_3();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 3\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 3\n"));
    }

    Error = TCase_1_3_4();
    if (Error != ST_NO_ERROR)
    {
        STEVT_Print(( "^^^Failed Test Case 4\n"));
        NumErrors++;
    }
    else
    {
        STEVT_Print(( "^^^Passed Test Case 4\n"));
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


/*  Register CallBack is not NULL with Old Subscription */
ST_ErrorCode_t TCase_1_3_1(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_Handle_t Handle;

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)Notify_CallBack;
    /*SubscrParam.RegisterCallback = (STEVT_CallbackProc_t)Notify_CallBack ;
    SubscrParam.UnregisterCallback = NULL;*/

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed"));
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);
   /* if (Error != STEVT_ERROR_REGISTER_CALLBACK_NOT_NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Register Callback is not NULL"));
        return Error;
    } */

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "first close failed"));
        return Error;
    }
    return Error;
}

/*  Unregister Call Back is not NULL With Old Subscription */
ST_ErrorCode_t TCase_1_3_2(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle;

    SubscrParam.NotifyCallback = (STEVT_CallbackProc_t)Notify_CallBack;
    /* SubscrParam.RegisterCallback = NULL;
    SubscrParam.UnregisterCallback = (STEVT_CallbackProc_t)Notify_CallBack; */

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed"));
        return Error;
    }

    Error = STEVT_Subscribe(Handle, EVENT2, &SubscrParam);
    /*if (Error != STEVT_ERROR_UNREGISTER_CALLBACK_NOT_NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Register Callback is not NULL"));
        return Error;
    }*/

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "first close failed"));
        return Error;
    }

    return Error;
}

/*  Register CallBack is not NULL with New Subscription */
ST_ErrorCode_t TCase_1_3_3(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_Handle_t Handle;

    SubscrDevParam.NotifyCallback = (STEVT_DeviceCallbackProc_t)Notify_DevCallBack;
    /* SubscrDevParam.RegisterCallback = (STEVT_DeviceCallbackProc_t)Notify_DevCallBack ;
    SubscrDevParam.UnregisterCallback = NULL; */

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed"));
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent(Handle,"REG1", EVENT2, &SubscrDevParam);

    /*if (Error != STEVT_ERROR_REGISTER_CALLBACK_NOT_NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Register Callback is not NULL"));
        return Error;
    }*/

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "first close failed"));
        return Error;
    }
    return Error;
}

/*  Unregister Call Back is not NULL With New Subscription */
ST_ErrorCode_t TCase_1_3_4(void)
{
    ST_ErrorCode_t Error;
    STEVT_Handle_t Handle;

    SubscrDevParam.NotifyCallback = (STEVT_DeviceCallbackProc_t)Notify_DevCallBack;
    /* SubscrDevParam.RegisterCallback = NULL ;
    SubscrDevParam.UnregisterCallback = (STEVT_DeviceCallbackProc_t)Notify_DevCallBack; */

    Error = STEVT_Open("Dev1", NULL, &Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Handle opening failed"));
        return Error;
    }

    Error = STEVT_SubscribeDeviceEvent(Handle,"REG1", EVENT2, &SubscrDevParam);
    /*if (Error != STEVT_ERROR_UNREGISTER_CALLBACK_NOT_NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Register Callback is not NULL"));
        return Error;
    }*/

    Error = STEVT_Close(Handle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "first close failed"));
        return Error;
    }

    return Error;
}




