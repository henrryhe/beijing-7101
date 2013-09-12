/******************************************************************************\
 *
 * File Name   : TComp_8_1.c
 *
 * Description : Test stress memory alloation/free
 *
 * Copyright STMicroelectronics - March 2001
 *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "stlite.h"
#include "stcommon.h"

#include "evt_test.h"


#define MAXEVENTS  16
#define MAXITERATIONS 300
#define STEVT_DEVICE_NAME "Dev8_1"

static int NotCB = 0;
STEVT_EventConstant_t Event[MAXEVENTS];
STEVT_DeviceSubscribeParams_t DevSubscrParam;


static void NotifyDevCB(STEVT_CallReason_t Reason,
                 const ST_DeviceName_t RegistrantName,
                 STEVT_EventConstant_t Event,
                 const void *EventData,
                 const void *SubscriberData_p)
{
    NotCB++;
}

static ST_ErrorCode_t EVT_StressMem(void);


ST_ErrorCode_t TComp_8_1()
{
    U32 ErrorNum =0, i=0;
    ST_ErrorCode_t Error;
    STEVT_InitParams_t InitParam;
    STEVT_TermParams_t TermParam;


    NotCB = 0;
    DevSubscrParam.NotifyCallback     = (STEVT_DeviceCallbackProc_t)NotifyDevCB;
    DevSubscrParam.SubscriberData_p   = (void *)NULL;

    TermParam.ForceTerminate = TRUE;

    InitParam.EventMaxNum   = MAXEVENTS;
    InitParam.ConnectMaxNum = 16;
    InitParam.SubscrMaxNum  = 256;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_USER_DEFINED;
    InitParam.MemoryPoolSize  = 2500;

    for (i=0; i<MAXEVENTS; i++)
    {
        Event[i]=(STEVT_DRIVER_BASE+i);
    }

    Error = STEVT_Init(STEVT_DEVICE_NAME, &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Dev8_1 initialisation failed"))
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Received error 0x%08X",Error));
        return Error;
    }

    for (i=0; i< MAXITERATIONS; i++)
    {
        Error = EVT_StressMem();
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"EVT_StressMem failed after %d iterations",i));
            return Error;
        }
    }


    /*************************************************************************/
    Error = STEVT_Term(STEVT_DEVICE_NAME, &TermParam);
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


/*-------------------------------------------------------------------------
 * Function : EVT_StressMem
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t EVT_StressMem(void)
{
    ST_ErrorCode_t RetErr = ST_NO_ERROR;
    U32 i;
    STEVT_Handle_t EvtSubHandle;
    STEVT_OpenParams_t OpenParams;

    /* Open the event handler for the subscriber */
    RetErr = STEVT_Open(STEVT_DEVICE_NAME, &OpenParams, &EvtSubHandle);
    if (RetErr!= ST_NO_ERROR )
    {
        STEVT_Print(("EVT_StressMem() : STEVT_Open() error 0x%08X for subscriber !\n", RetErr));
        return RetErr;
    }

    if (RetErr == ST_NO_ERROR)
    {
        for (i =0; i<MAXEVENTS; i++)
        {

            RetErr |= STEVT_SubscribeDeviceEvent(EvtSubHandle, "VID00",
                        Event[i], &DevSubscrParam);
            if (RetErr != ST_NO_ERROR )
            {
                STEVT_Print(("EVT_StressMem() : subscribe events error 0x%08X ! event #%d \n", RetErr,i));
                return RetErr;
            }
        }
    }

    /* STEVT_Print(("Allocated memory is %d\n",STEVT_GetAllocatedMemory(EvtSubHandle)-32)); */

    /* Close */
    if (RetErr == ST_NO_ERROR)
    {
        RetErr = STEVT_Close(EvtSubHandle);
        if (RetErr!= ST_NO_ERROR )
        {
            STEVT_Print(("EVT_StressMem() : close subscriber evt. handler error %d !\n", RetErr));
            return RetErr;
        }
    }


    return(RetErr);

} /* end of EVT_StressMem() */

