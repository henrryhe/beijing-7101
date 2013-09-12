/******************************************************************************

    File Name   : ReEnter.c

    Description : Re-entrancy test for STEVT

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "evt_test.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

enum
{
    EVENT1 = STEVT_DRIVER_BASE,
    EVENT2
};

/* Private Variables ------------------------------------------------------- */

/* Communications which we would normally pass through EventData */
static STEVT_Handle_t EVTHandle;
static STEVT_EventID_t EventID1;
static U32 ErrorCount, CBCount;

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : NullCB
  Description : callback which does nothing
   Parameters :
******************************************************************************/
static void NullCB(STEVT_CallReason_t Reason, 
                   STEVT_EventConstant_t Event,
                   const void *EventData)
{
}


/******************************************************************************
Function Name : ReEnterCB
  Description : callback which attempts various re-entrant actions
   Parameters :
******************************************************************************/
static void ReEnterCB(STEVT_CallReason_t Reason, 
                      STEVT_EventConstant_t Event,
                      const void *EventData)
{
    ST_ErrorCode_t                 Error;
    STEVT_EventID_t                EventID2;
    STEVT_InitParams_t             InitParam;
    STEVT_TermParams_t             TermParam;
    STEVT_SubscriberID_t           SubscriberID;
    STEVT_SubscribeParams_t        SubscrParam = {NullCB};
        
    TermParam.ForceTerminate = TRUE;
    
    InitParam.EventMaxNum   = 1;
    InitParam.ConnectMaxNum = 1;
    InitParam.SubscrMaxNum  = 1;

#ifdef ST_OS21
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = (ST_Partition_t *)SystemPartition;
#endif

    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    
    STEVT_Print(("%s callback received\n",
        Reason == CALL_REASON_NOTIFY_CALL ? "NOTIFY" :
        "UNRECOGNISED"));
    
    /* confirm we can call the non-event-specific API */
    Error = STEVT_GetSubscriberID (EVTHandle, &SubscriberID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested GetSubscriberID failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }
    
    /* Subscribe a different event */
    Error = STEVT_Subscribe (EVTHandle, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested subscription failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }
    
    /* Unsubscribing the event tests that code and frees it for next time */
    Error = STEVT_Unsubscribe (EVTHandle, EVENT2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested unsubscription failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }

   
    /* Register a different event constant */
    Error = STEVT_Register (EVTHandle, EVENT2, &EventID2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested registration failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }

    /* Notify that different event constant */
    Error = STEVT_Notify (EVTHandle, EventID2, NULL);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested notification failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }
    
    /* Subscribe that different event now it is referenced */
    Error = STEVT_Subscribe (EVTHandle, EVENT2, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested subscription failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }

    /* Confirm NotifySubscriber works too */
    Error = STEVT_NotifySubscriber (EVTHandle, EventID2, NULL, SubscriberID);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested notification failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }

    Error = STEVT_Unsubscribe (EVTHandle, EVENT2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested unsubscription failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }
    
    /* Unregister the different event constant */
    Error = STEVT_Unregister (EVTHandle, EVENT2);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: nested unregistration failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
    }
  
    ++CBCount;
}


/******************************************************************************
Function Name : ReEnterTest
  Description : Inner part of STEVT re-entrancy test (separated out for
                convenience of error handling)
   Parameters :
******************************************************************************/
void ReEnterTest( void )
{
    ST_ErrorCode_t            Error;
    STEVT_SubscribeParams_t   SubscrParam = {ReEnterCB};

    /* Open registrant handle */
    Error = STEVT_Open("DevReEnter", NULL, &EVTHandle);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: Opening Handle failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
        return;
    }
    
    /* Register EVENT1 */
    Error = STEVT_Register (EVTHandle, EVENT1, &EventID1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: event registration failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
        return;
    }
    
    /* Subscribe ReEnterCB (leads to RegisterCallback) */
    Error = STEVT_Subscribe (EVTHandle, EVENT1, &SubscrParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: event subscription failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
        return;
    }
    
      
    /* Notify it and try various things inside the callback */
    Error = STEVT_Notify (EVTHandle, EventID1, NULL);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: event notification failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
        return;
    }
    
    /* Explicitly Unregister EVENT1 */
    Error = STEVT_Unregister(EVTHandle, EVENT1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: event unregistration failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
        return;
    }
    
    /* Remove the subscription to prevent a second UnregisterCallback at Close
      for the anonymous registration. In that case, all calls should uniformly
      return ST_ERROR_INVALID_HANDLE */
    Error = STEVT_Unsubscribe (EVTHandle, EVENT1);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "ReEnter: event unsubscription failed\n"
                      "Received error 0x%08X",Error));
        ++ErrorCount;
        return;
    }
}

/******************************************************************************
Function Name : TESTEVT_ReEnter
  Description : Check STEVT re-entrancy
   Parameters :
******************************************************************************/
U32 TESTEVT_ReEnter( void )
{
    ST_ErrorCode_t          Error;
    STEVT_InitParams_t      InitParam;
    STEVT_TermParams_t      TermParam;
    
    ErrorCount = 0;
    CBCount = 0;
    
    TermParam.ForceTerminate = TRUE;
    
    InitParam.EventMaxNum   = 4;
    InitParam.ConnectMaxNum = 1;
    InitParam.SubscrMaxNum  = 4;
#ifdef ST_OS21
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = (ST_Partition_t *)SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    

    /* Initialise EVT device */
    Error = STEVT_Init("DevReEnter", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "DevReEnter initialisation failed\n"
                      "Received error 0x%08X",Error));
        return ++ErrorCount;
    }

    /* Do the main test */
    ReEnterTest();
    task_delay(1000);
    
    /* Clean up */
    Error = STEVT_Term("DevReEnter", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Device termination failed\n"
                      "Received error 0x%08X",Error));
        return ++ErrorCount;
    }

    /* nb STEVT_Term calls omitted on previous error cases */
    
    /* expect Register (at Subscribe), Notify, Unregister for EVENT1: anonymous,
      plus Register for EVENT1: "Second" */
    if(CBCount != 1)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Wrong number of EVENT1 callbacks received:\n"
                      "\texpected 5, got %u", CBCount));
        ++ErrorCount;
    }
    
    if(ErrorCount == 0)
    {
        STEVT_Print(("Re-entrancy test passed\n"));
    }

    return ErrorCount;
}
