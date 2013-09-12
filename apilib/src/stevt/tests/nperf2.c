/******************************************************************************

    File Name   : nperf2.c

    Description : New STEVT_Notify performance tests, showing variation with
                  number of events, names, and subscribers per name

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "stcommon.h"

#include "evt_test.h"

/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */

#define MAX_EVENTS 40
#define MAX_NAMES  6
#define MAX_SUBSCR 8

/* EVT device name to use */
const ST_DeviceName_t EvtName = "nPerf2Dev";

/* registrant names to use (MAX_NAMES entries) */
const ST_DeviceName_t Names[MAX_NAMES] = { "A", "B", "C", "D", "E", "F" };

/* Private Variables ------------------------------------------------------- */

/* EVT handles */
static STEVT_Handle_t RegistrantHandle;
static STEVT_Handle_t SubscriberHandles[MAX_SUBSCR];

/* array of event IDs for notification */
static STEVT_EventID_t EventIDs[MAX_EVENTS][MAX_NAMES];

/* Private Macros ---------------------------------------------------------- */

#define TABLE_LEN(X) (sizeof(X) / sizeof(X[0]))

/* Private Function Prototypes --------------------------------------------- */

/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : NullDevCB
  Description : Device callback that does nothing - to demonstrate the STEVT
                mechanism overhead and nothing but the overhead
   Parameters :
******************************************************************************/
void NullDevCB(STEVT_CallReason_t     Reason, 
               const ST_DeviceName_t  RegistrantName,
               STEVT_EventConstant_t  Event,
               const void            *EventData,
               const void            *SubscriberData_p)
{
}


/******************************************************************************
Function Name : DoNotifies
  Description : Time notifying the indicated number of events and names
   Parameters : 
******************************************************************************/
static ST_ErrorCode_t DoNotifies(U32 NumEvents, U32 NumNames,
                                 clock_t *TicksTaken)
{
    ST_ErrorCode_t  Error;
    clock_t         t1, t2;
    int             EventIdx, NameIdx;
    
    /* loop over names then events so we traverse different subscriber lists
      on sucessive iterations */
      
    t1 = time_now();
    for (NameIdx = 0; NameIdx < NumNames; ++NameIdx)
    {
        for (EventIdx = 0; EventIdx < NumEvents; ++EventIdx)
        {
            Error = STEVT_Notify(RegistrantHandle,
                                 EventIDs[EventIdx][NameIdx], NULL);
            if (Error != ST_NO_ERROR)
            {
                return Error;
            }
        }
    }
    t2 = time_now();
    
    *TicksTaken = time_minus(t2, t1); /* t2 - t1 */
    
    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : AddSubscribers
  Description : Add new subscribers to each current event and name
   Parameters :
******************************************************************************/
static ST_ErrorCode_t AddSubscribers(U32 NumEvents, U32 NumNames,
                                     U32 OldSubscr, U32 NewSubcr)
{
    ST_ErrorCode_t                  Error;
    STEVT_OpenParams_t              OpenParams;
    STEVT_DeviceSubscribeParams_t   SubscrParams = { NullDevCB,
                                                     NULL, NULL, NULL };
    int                             EventIdx, NameIdx, SubscrIdx;
    
    for (SubscrIdx = OldSubscr; SubscrIdx < NewSubcr; ++SubscrIdx)
    {
        Error = STEVT_Open(EvtName, &OpenParams, &SubscriberHandles[SubscrIdx]);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Error 0x%08X"
                          " opening subscriber %u", Error, SubscrIdx));
            return Error;
        }

        for (EventIdx = 0; EventIdx < NumEvents; ++EventIdx)
        {
            for (NameIdx = 0; NameIdx < NumNames; ++NameIdx)
            {
                Error = STEVT_SubscribeDeviceEvent(SubscriberHandles[SubscrIdx],
                                                   Names[NameIdx],
                                                   STEVT_DRIVER_BASE + EventIdx,
                                                   &SubscrParams);
                if (Error != ST_NO_ERROR)
                {
                    STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Error 0x%08X"
                                  " on subscription %u to event %u %s",
                                  Error, SubscrIdx, EventIdx, Names[NameIdx]));
                    return Error;
                }
            }
        }
    }
    
    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : AddRegistrations
  Description : Add new registration to each current event
   Parameters :
******************************************************************************/
static ST_ErrorCode_t AddRegistrations(U32 NumEvents, U32 OldReg, U32 NewReg)
{
    ST_ErrorCode_t  Error;
    int             EventIdx, NameIdx;

    for (EventIdx = 0; EventIdx < NumEvents; ++EventIdx)
    {
        for (NameIdx = OldReg; NameIdx < NewReg; ++NameIdx)
        {
            Error = STEVT_RegisterDeviceEvent(RegistrantHandle, Names[NameIdx],
                                              STEVT_DRIVER_BASE + EventIdx,
                                              &EventIDs[EventIdx][NameIdx]);
            if (Error != ST_NO_ERROR)
            {
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                              "Error 0x%08X registering event %u %s",
                              Error, EventIdx, Names[NameIdx]));
                return Error;
            }
        }
    }
    
    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : CleanSubscribers
  Description : Remove current subscribers to start again with new numbers
   Parameters :
******************************************************************************/
static ST_ErrorCode_t CleanSubscribers(U32 NumSubscr)
{
    ST_ErrorCode_t  Error;
    int             SubscrIdx;

    for (SubscrIdx = 0; SubscrIdx < NumSubscr; ++SubscrIdx)
    {
        Error = STEVT_Close(SubscriberHandles[SubscrIdx]);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "Error 0x%08X closing subscriber %u", SubscrIdx));
            return Error; /* so we know our scheme is broken */
        }
    }
    
    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : Report
  Description : Display results from a given subtest
   Parameters :
******************************************************************************/
static void Report(U32 EventNum, U32 NameNum, U32 SubscrNum, clock_t TicksTaken)
{
#if 0 /* STEVT_Print outputs "[fp]" for floating point */
    float TicksPerSec = (float)ST_GetClocksPerSecond();
    float Ticks = (float)TicksTaken;
    
    STEVT_Print(("#####################################################\n"));
    STEVT_Print(("  %2u events, %u names, %u subscribers\n",
                 EventNum, NameNum, SubscrNum));
    STEVT_Print(("      Total time:            %6.3f ms (%3u ticks)\n",
                 (float)(Ticks/TicksPerSec), TicksTaken));
                 /* cast to avoid "Lower precision in wider context: '/'" */
                 
    /* we looped over all events and names */
    Ticks /= (float)(U32)(EventNum * NameNum);
    STEVT_Print(("      per STEVT_Notify call: %6.3f ms (%3.2f ticks)\n",
                 (float)(Ticks/TicksPerSec), Ticks));

    if (SubscrNum > 0)
    {
        /* each notify touched SubscrNum subscribers */
        Ticks /= (float)SubscrNum;
        STEVT_Print(("      per subscriber call:   %6.3f ms (%3.2f ticks)\n",
                     (float)(Ticks/TicksPerSec), Ticks));
    }
#else
    STEVT_Print(("  %2u events, %u names, %u subscribers: %5u us (%3u ticks)\n",
                 EventNum, NameNum, SubscrNum,
                 (10000*TicksTaken)/((clock_t)ST_GetClocksPerSecond()/100),
                 TicksTaken));
                 /* splitting of the 1E6 conversion factor is a compromise
                   between accuracy and avoiding overflow */
#endif
}
       
       
/******************************************************************************
Function Name : TESTEVT_NotifyPerf2
  Description :
   Parameters :
      Returns : number of unexpected errors encountered
******************************************************************************/
U32 TESTEVT_NotifyPerf2(void)
{
    const U32 EventNums[]  = { 10, 20, 40 };
    const U32 NameNums[]   = {  1,  3,  6 };
    const U32 SubscrNums[] = {  0,  2,  4,  8 };

    U32                 ErrorNum = 0;
    U32                 EventPos, NamePos, SubscrPos;
    U32                 PrevNameNum, PrevSubscrNum;
    clock_t             TicksTaken;
    ST_ErrorCode_t      Error;
    STEVT_InitParams_t  InitParams;
    STEVT_OpenParams_t  OpenParams;
    STEVT_TermParams_t  TermParams;

    TermParams.ForceTerminate = TRUE; /* for interim aborts */
    
    /* initialise STEVT device */
    InitParams.EventMaxNum = MAX_EVENTS*MAX_NAMES;
    InitParams.ConnectMaxNum = MAX_SUBSCR+1;
    InitParams.SubscrMaxNum = MAX_SUBSCR*MAX_NAMES*MAX_EVENTS;
    InitParams.MemoryPartition = system_partition;
    InitParams.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    Error = STEVT_Init(EvtName, &InitParams);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "%s initialisation failed"
                      " with error 0x%08X", EvtName, Error));
        return ++ErrorNum;
    }
    
    for (EventPos = 0; EventPos < TABLE_LEN(EventNums); ++EventPos)
    {
        /* open registrant handle */
        Error = STEVT_Open(EvtName, &OpenParams, &RegistrantHandle);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Error 0x%08X"
                          " opening registrant handle", Error));
            STEVT_Term(EvtName, &TermParams);
            return ++ErrorNum;
        }

        /*STEVT_Print(("%u bytes allocated with just registrant handle\n",
                     STEVT_GetAllocatedMemory(RegistrantHandle)));*/
        
        PrevNameNum = 0;
        for (NamePos = 0; NamePos < TABLE_LEN(NameNums); ++NamePos)
        {
            Error = AddRegistrations(EventNums[EventPos],
                                     PrevNameNum, NameNums[NamePos]);
            if (Error != ST_NO_ERROR)
            {
                STEVT_Term(EvtName, &TermParams);
                return ++ErrorNum; /* message already displayed */
            }
            
            PrevNameNum = NameNums[NamePos];
            
            PrevSubscrNum = 0;
            for (SubscrPos = 0; SubscrPos < TABLE_LEN(SubscrNums); ++SubscrPos)
            {
                Error = AddSubscribers(EventNums[EventPos], NameNums[NamePos],
                                       PrevSubscrNum, SubscrNums[SubscrPos]);
                if (Error != ST_NO_ERROR)
                {
                    STEVT_Term(EvtName, &TermParams);
                    return ++ErrorNum; /* message already displayed */
                }
                
                PrevSubscrNum = SubscrNums[SubscrPos];
                                       
                Error = DoNotifies(EventNums[EventPos], NameNums[NamePos],
                                   &TicksTaken);
                if (Error != ST_NO_ERROR)
                {
                    STEVT_Term(EvtName, &TermParams);
                    return ++ErrorNum; /* message already displayed */
                }
                                   
                Report(EventNums[EventPos], NameNums[NamePos],
                       SubscrNums[SubscrPos], TicksTaken);
            }
            
            /* start next NumEvents/Names with zero subscribers */
            Error = CleanSubscribers(PrevSubscrNum);
            if (Error != ST_NO_ERROR)
            {
                STEVT_Term(EvtName, &TermParams);
                return ++ErrorNum; /* message already displayed */
            }
        }
        
        /* close registrant handle to clean up names we don't yet want
          with the new NumEvents */
        Error = STEVT_Close(RegistrantHandle);
        if (Error != ST_NO_ERROR)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "Error 0x%08X"
                          " closing registrant handle", Error));
            STEVT_Term(EvtName, &TermParams);
            return ++ErrorNum;
        }
    }

    /*STEVT_Print(("#####################################################\n"));*/
        
    /* should have closed all handles above */
    TermParams.ForceTerminate = FALSE;
    Error = STEVT_Term(EvtName, &TermParams);
    if (Error != ST_NO_ERROR)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "%s termination failed"
                      " with error 0x%08X", EvtName, Error));
        return ++ErrorNum;
    }
    
    return ErrorNum;
}
