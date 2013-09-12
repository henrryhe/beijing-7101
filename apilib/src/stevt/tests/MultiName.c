/******************************************************************************

    File Name   : MultiName.c

    Description : Test STEVT handling of the same EVT handle registering
                  events with multiple names

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "evt_test.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */

/******************************************************************************
Function Name : TESTEVT_MultiName
  Description : Wrapper for STEVT multiple-name-handling tests
   Parameters :
******************************************************************************/
U32 TESTEVT_MultiName ( void )
{
    ST_ErrorCode_t          Error;
    STEVT_InitParams_t      InitParam;
    STEVT_OpenParams_t      OpenParam; /* empty */
    STEVT_TermParams_t      TermParam;
    STEVT_Handle_t          HandleMemCheck, HandleDevReg, HandleReg;
    STEVT_EventID_t         EventID[3];
    ST_DeviceName_t         EventName[3] = { "A", "B", "C" };
    U32                     ErrorNum = 0;
    U32                     AllocStart, AllocEnd;

    InitParam.EventMaxNum   = 3; /* only one, but two names */
    InitParam.ConnectMaxNum = 5;
    InitParam.SubscrMaxNum  = 1;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    InitParam.MemoryPartition = system_partition;
#else
    InitParam.MemoryPartition = SystemPartition;
#endif
    InitParam.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    TermParam.ForceTerminate = TRUE;

    /* return whenever failure would compromise later check */

    /* Initialise EVT device */
    Error = STEVT_Init("DevMultiName", &InitParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error 0x%08X initialising DevMultiName",Error));
        return ++ErrorNum;
    }

    /* Open memory-checking handle */
    Error = STEVT_Open("DevMultiName", &OpenParam, &HandleMemCheck);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: Error 0x%08X opening memory-check handle",
                      Error));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }
    AllocStart = STEVT_GetAllocatedMemory(HandleMemCheck);

    /* Open registrant handle(DEV) */
    Error = STEVT_Open("DevMultiName", &OpenParam, &HandleDevReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: Error 0x%08X opening registrant handle",
                      Error));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    /* Open registrant handle */
    Error = STEVT_Open("DevMultiName", &OpenParam, &HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: Error 0x%08X opening registrant handle",
                      Error));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    /* test 1 */
    /*Registering a event with same handle with two diffrent reg name */
    /* Register instances of the event STEVT_DRIVER_BASE with reg name A */

    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[0],
                                        STEVT_DRIVER_BASE, &EventID[0]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                       Error,EventName[0]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }


    /* Register instances of the same event(STEVT_DRIVER_BASE) same handle  with reg name B */
    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[1],
                                       STEVT_DRIVER_BASE, &EventID[1]);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[1]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    /* unregister the first(STEVT_DRIVER_BASE) one */
    Error = STEVT_UnregisterDeviceEvent (HandleDevReg, EventName[0],
                                         STEVT_DRIVER_BASE);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X unregistering event \"%s\"",
                      Error,EventName[0]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    /* now register the second again, Now it should get registered */
    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[1],
                                       STEVT_DRIVER_BASE, &EventID[0]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[1]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    if (Error == ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Passed Test Case TESTEVT_MultiName-1\n"));
    }
    else
    {
        STEVT_Print(( "^^^Failed Test Case TESTEVT_MultiName-1\n"));
    }

    /* test2 */
    /*Registering a event annoymously first time and second time with reg name B */

    /* Register instances of the event (STEVT_DRIVER_BASE+1) annoymously  */
    Error = STEVT_Register(HandleDevReg,
                           (STEVT_DRIVER_BASE + 1), &EventID[0]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                       Error,EventName[0]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

 /* Register instances of the same event(STEVT_DRIVER_BASE +1) same handle  with reg name B */
    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[1],
                                       (STEVT_DRIVER_BASE+1), &EventID[1]);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[1]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    Error = STEVT_Unregister(HandleDevReg,(STEVT_DRIVER_BASE +1));
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X unregistering event \"%s\"",
                      Error,EventName[0]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

/* Register instances of the same event(STEVT_DRIVER_BASE +1) same handle  with reg name B */
    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[1],
                                       (STEVT_DRIVER_BASE+1), &EventID[1]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[1]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    if (Error == ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Passed Test Case TESTEVT_MultiName-2\n"));
    }
    else
    {
        STEVT_Print(( "^^^Failed Test Case TESTEVT_MultiName-2\n"));
    }

    /* test 3 */
    /* Registering a event with reg name B and second time anooymously */


    /* Register instances of the event (STEVT_DRIVER_BASE+2) annoymously  */
    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[2],
                                       (STEVT_DRIVER_BASE+2), &EventID[2]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[2]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    Error = STEVT_Register(HandleDevReg,
                           (STEVT_DRIVER_BASE + 2), &EventID[2]);
    if (Error != STEVT_ERROR_ALREADY_REGISTERED)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                       Error,EventName[0]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    Error = STEVT_UnregisterDeviceEvent (HandleDevReg, EventName[2],
                                         (STEVT_DRIVER_BASE+2));
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X unregistering event \"%s\"",
                      Error,EventName[2]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }


    Error = STEVT_Register(HandleDevReg,
                           (STEVT_DRIVER_BASE + 2), &EventID[2]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                       Error,EventName[0]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    if (Error == ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Passed Test Case TESTEVT_MultiName-3\n"));
    }
    else
    {
        STEVT_Print(( "^^^Failed Test Case TESTEVT_MultiName-3\n"));
    }

    /* test 4 */
    /*Registering a event(STEVT_DRIVER_BASE+3) with reg name B and second time Reg name B but with diffrent handle */

    /* Register instances of the event (STEVT_DRIVER_BASE+2) with reg name B  */
    Error = STEVT_RegisterDeviceEvent (HandleDevReg, EventName[1],
                                       (STEVT_DRIVER_BASE+3), &EventID[2]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[1]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

     /* Register instances of the event (STEVT_DRIVER_BASE+3) with reg name B but with
        diffrent handle */
    Error = STEVT_RegisterDeviceEvent (HandleReg, EventName[1],
                                       (STEVT_DRIVER_BASE+3), &EventID[1]);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: error 0x%08X registering event \"%s\"",
                      Error,EventName[1]));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    if (Error == ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Passed Test Case TESTEVT_MultiName-4\n"));
    }
    else
    {
        STEVT_Print(( "^^^Failed Test Case TESTEVT_MultiName-4\n"));
    }


    /* confirm the internal memory associated with both registrations
       is released when we Close */

    Error = STEVT_Close(HandleDevReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: Error 0x%08X closing registrant handle",
                      Error));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    Error = STEVT_Close(HandleReg);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "MultiName: Error 0x%08X closing registrant handle",
                      Error));
        STEVT_Term("DevMultiName", &TermParam);
        return ++ErrorNum;
    }

    AllocEnd = STEVT_GetAllocatedMemory(HandleMemCheck);

    if (AllocStart != AllocEnd)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
               "MultiName: STEVT_Close did not release all internal memory:"
               " drop is %i bytes", (int)AllocEnd-(int)AllocStart));
        ++ErrorNum;
    }

    Error = STEVT_Term("DevMultiName", &TermParam);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Error 0x%08X terminating DevMultiName",Error));
        ++ErrorNum;
    }

    if (Error == ST_NO_ERROR )
    {
        STEVT_Print(( "^^^Passed Test Case TESTEVT_MultiName-5\n"));
    }
    else
    {
        STEVT_Print(( "^^^Failed Test Case TESTEVT_MultiName-5\n"));
    }

    return (ErrorNum);
}
