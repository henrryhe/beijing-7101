/****************************************************************************

File Name   : stclkrv.c

Description : Clock Recovery API Routines

Copyright (C) 2007, STMicroelectronics

Revision History    :

    [...]

References  :

$ClearCase (VOB: stclkrv)S

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 5.0

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include <string.h>
#include <stdlib.h>
#include "stlite.h"        /* MUST be included BEFORE stpti.h */
#include "sttbx.h"
#include "stsys.h"
#endif

#include "stos.h"
#include "stevt.h"

/* our own includes */
#include "stclkrv.h"
#include "clkrvreg.h"
#include "clkrvdat.h"

/* Private Constants ------------------------------------------------------ */

#ifndef STCLKRV_FILTER_TASK_PRIORITY
#ifdef ST_OSLINUX
#define STCLKRV_FILTER_TASK_PRIORITY  99
#else
#define STCLKRV_FILTER_TASK_PRIORITY  MAX_USER_PRIORITY
#endif
#endif

#ifdef ST_OSLINUX
#define STCLKRV_FS_REGION_SIZE 0xff
#define STCLKRV_AUDFS_REGION_SIZE 0xff
#define STCLKRV_CRU_REGION_SIZE 0xff
#endif

/* Externs ---------------------------------------------------------------- */
extern stclkrv_Freqtable_t NominalFreqTable[MAX_NO_IN_FREQ_TABLE];

/* Clock recovery Filter & drift correction of slaves clocks Debug Array */
#ifdef CLKRV_FILTERDEBUG
#define CLKRV_MAX_SAMPLES  5000
extern U32 ClkrvDbg[13][CLKRV_MAX_SAMPLES];
extern U32 Clkrv_Slave_Dbg[13][CLKRV_MAX_SAMPLES];
extern U32 SampleCount;
#endif

/* Private Variables ------------------------------------------------------ */

/* Provides mutual exclusion on global data */
static semaphore_t *AccessSemaphore_p = NULL;

/* Initialization Context */
static stclkrv_Context_t InitContext =
{
    TRUE,       /* Reset */
    0,          /* PrevDiff */
    0,          /* PrevPcr*/
    0,          /* Out Standing Error */
    0,          /* ControlValue */
    NULL,       /* ErrorStore_p; */
    0,          /* ErrorStoreCount */
    0,          /* Head */
    0,          /* ErrorSum */
    (PCRTIMEFACTOR * STCLKRV_PCR_DRIFT_THRES) /* Maximum error before a Glitch event is thrown*/
};


static stclkrv_ReferencePCR_t InitRefPcr =
{
    TRUE,                       /* AwaitingPCR         */
    FALSE,                      /* Valid               */
    TRUE,                       /* PCRHandlingActive   */
    0,                          /* PcrBaseBit32        */
    0,                          /* PcrBaseValue        */
    0,                          /* PcrExtension        */
    0,                          /* PcrArrivalBaseBit32 */
    0,                          /* PcrArrvialBaseValue */
    0,                          /* PcrArrivalExtension */
    0,                          /* Total Value         */
    0,                          /* Time                */
    0,                          /* GlitchCount         */
    (U32) STCLKRV_STATE_PCR_UNKNOWN   /* MachineState        */
};

/* Initialise ptr to Control block list */
STCLKRV_ControlBlock_t *ClkrvList_p   = NULL;

#if defined (WA_GNBvd44290)|| defined (WA_GNBvd54182)|| defined(WA_GNBvd56512)
extern void stclkrv_GNBvd44290_ResetClkrvTracking(void);
#endif /* WA_GNBvd44290 || WA_GNBvd54182 || WA_GNBvd56512*/

/* Driver revision number */
/*static const ST_Revision_t stclkrv_DriverRev = "STCLKRV-REL_5.4.0";*/

/* Private Macros --------------------------------------------------------- */
#if defined (ST_5100)
#define AdjustHPTTicksToMicroSec(Value)                 \
{                                                       \
     /* With 64-bit arithmetic we can do the exact calculation:
        Value' = (Value * 1000000) / ST_GetClocksPerSecondHigh() */         \
    U32 HPTicksPerSec = ST_GetClocksPerSecondHigh();                        \
    __optasm                                                                \
    {                                                                       \
        ldabc Value, 1000000, 0;                                            \
        lmul;                   /* A*B + C (carry) -> A'B' */               \
                                /* should strictly add HPTicksPerSec/2 */   \
        ldl HPTicksPerSec;      /* NOT ld (might corrupt B, C */            \
        ldiv;                   /* B'C' / A -> A remainder B */             \
        st Value;                                                           \
    }                                                                       \
    /* We might want to change and use this code on all chips */            \
}
#endif
/* Private Function prototypes -------------------------------------------- */

#ifndef STCLKRV_NO_PTI
/* Callback Registered with STPTI to notify PCR,STC values */
#ifdef ST_5188
extern void STCLKRV_ActionPCR(STEVT_CallReason_t EventReason,
                       ST_DeviceName_t RegistrantName,
                       STEVT_EventConstant_t EventOccured,
                       STDEMUX_EventData_t *PcrData_p,
                       STCLKRV_Handle_t *InstanceHandle_p);
#else
extern void STCLKRV_ActionPCR(STEVT_CallReason_t EventReason,
                       ST_DeviceName_t RegistrantName,
                       STEVT_EventConstant_t EventOccured,
                       STPTI_EventData_t *PcrData_p,
                       STCLKRV_Handle_t *InstanceHandle_p);
#endif
extern void ClockRecoveryFilterTask(STCLKRV_ControlBlock_t *CB_p);
extern void UpdateStateMachine(STCLKRV_ControlBlock_t *Instance_p,BOOL DiscontinuousPCR);

static ST_ErrorCode_t SubscribePCR(STCLKRV_ControlBlock_t *Instance_p);
static ST_ErrorCode_t UnsubscribePCR(STCLKRV_ControlBlock_t *Instance_p);

/* STPTI */
#ifndef ST_5188
static BOOL GetExtendedPacketArrivalTime(STCLKRV_Handle_t Handle,
                                         U32 *BaseBit32,
                                         U32 *BaseValue,
                                         U32 *Extension);
static BOOL GetPacketArrivalTime(STCLKRV_Handle_t Handle, U32 *ArrivalTime);
#endif
static void AddTwoExtSTCValues(STCLKRV_ExtendedSTC_t ExtSTCA,STCLKRV_ExtendedSTC_t ExtSTCB,
                                                    STCLKRV_ExtendedSTC_t *ReturnExtSTC_p);
static void SubtractTwoExtSTCValues(STCLKRV_ExtendedSTC_t ExtSTCA,STCLKRV_ExtendedSTC_t ExtSTCB,
                                                    STCLKRV_ExtendedSTC_t *ReturnExtSTC_p);
#if defined (ST_5100)
static BOOL ConvertMicroSecsToExtSTC(clock_t MicroSecVal, STCLKRV_ExtendedSTC_t *ExtSTC_p);
#endif

#endif

extern STOS_INTERRUPT_DECLARE(ClkrvTrackingInterruptHandler, Data);

/* EVT Related */
static ST_ErrorCode_t RegisterEvents(STCLKRV_ControlBlock_t *Instance_p);
static ST_ErrorCode_t UnregisterEvents(STCLKRV_ControlBlock_t *Instance_p);

/* Utility Functions */
static void AddToInstanceList(STCLKRV_ControlBlock_t *NewItem_p);
static BOOL DelFromInstanceList(ST_DeviceName_t DeviceName);
static STCLKRV_ControlBlock_t *FindInstanceByHandle(STCLKRV_Handle_t ClkHandle);
static STCLKRV_ControlBlock_t *FindInstanceByName(const ST_DeviceName_t DeviceName);
static BOOL IsClockValid(STCLKRV_ControlBlock_t *CB_p, STCLKRV_ClockSource_t ClockSource );

/* LINUX Memory Mapping functions */
#ifdef ST_OSLINUX
static ST_ErrorCode_t clkrv_LinuxMapRegion( STCLKRV_ControlBlock_t *CB_p );
static void clkrv_LinuxUnmapRegion( STCLKRV_ControlBlock_t *CB_p );
#endif

/* Functions -------------------------------------------------------------- */

#if defined(CLKRV_TESTING)

/****************************************************************************
Name         : STCLKRV_GetClocksFrequency()

Description  : Returns the current freq. of various FS clocks.
               NOTE: This function is not documented in the
               API and is not exported in stclkrv.h as it
               is only needed by the test harness

Parameters   : STCLKRV_Handle_t Handle - Valid clock recovery handle
               U32 *                   - Pointer to returned FS0 freq.
               U32 *                   - Pointer to returned FS1 freq.
               U32 *                   - Pointer to returned FS2 freq.

Return Value : ST_ErrorCode_t
               ST_NO_ERROR               Successful completion
               ST_ERROR_INVALID_HANDLE   STCLKRV handle is not open

See Also     :
 ****************************************************************************/
ST_ErrorCode_t STCLKRV_GetClocksFrequency( STCLKRV_Handle_t Handle,
                                        U32 *pSTCFrequency,
                                        U32 *pFS1Frequency,
                                        U32 *pFS2Frequency,
                                        U32 *pFS3Frequency,
                                        U32 *pFS4Frequency,
                                        U32 *pFS5Frequency,
                                        U32 *pFS6Frequency)
{
    /* Test handle validity */
    STCLKRV_ControlBlock_t *TmpCB_p;

    TmpCB_p = FindInstanceByHandle(Handle);
    if(TmpCB_p == NULL)
    {
        return ST_ERROR_INVALID_HANDLE;
    }
    if (TmpCB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }

    /* set to zero all first */
    *pFS1Frequency = 0;
    *pFS2Frequency = 0;
    *pFS3Frequency = 0;
    *pFS4Frequency = 0;
    *pFS5Frequency = 0;

    *pSTCFrequency = TmpCB_p->HWContext.Clocks[0].Frequency;
    /* 5188 has only one slave until cut 1.0 */
    *pFS1Frequency = TmpCB_p->HWContext.Clocks[1].Frequency;

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)\
 || defined (ST_5188) || defined (ST_7710) || defined (ST_7100) || defined (ST_7109)\
 || defined (ST_7200)
    *pFS2Frequency = TmpCB_p->HWContext.Clocks[2].Frequency;
#endif

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    *pFS3Frequency = TmpCB_p->HWContext.Clocks[3].Frequency;
#endif

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    *pFS4Frequency = TmpCB_p->HWContext.Clocks[4].Frequency;
#endif

#if defined (ST_5525) || defined (ST_7200)
    *pFS5Frequency = TmpCB_p->HWContext.Clocks[5].Frequency;
#endif

#if defined (ST_5525) || defined (ST_7200)
    *pFS6Frequency = TmpCB_p->HWContext.Clocks[6].Frequency;
#endif

    return ST_NO_ERROR;                  /* Success */
}
#endif  /* CLKRV_TESTING */

/****************************************************************************
Name         : STCLKRV_GetRevision()

Description  : Returns a pointer to the Driver Revision String.
               May be called at any time.

Parameters   : None

Return Value : ST_Revision_t

See Also     : ST_Revision_t
 ****************************************************************************/

ST_Revision_t STCLKRV_GetRevision( void )
{
    return STCLKRV_GETREVISION;
}

/****************************************************************************
Name         : STCLKRV_Init()

Description  : Initializes the Clock Recovery before use.

Parameters   : ST_DeviceType_t name needs to be supplied - should not be NULL,
               STCLKRV_InitParams_t struct. pointer appropriately filled by app.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_FEATURE_NOT_SUPPORTED  No multiple instance support(5100)
               ST_ERROR_ALREADY_INITIALIZED    Device already initialized
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid
               ST_ERROR_NO_MEMORY              memory_allocate failed
               STCLKRV_ERROR_HANDLER_INSTALL   Unable to install STPTI Callback
               ST_ERROR_INTERRUPT_INSTALL      Unable to install internal ISR
               STCLKRV_ERROR_EVT_REGISTER      Problem registering events

See Also     : STCLKRV_InitParams_t
               STCLKRV_Term()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Init( const ST_DeviceName_t Name,
                             const STCLKRV_InitParams_t *InitParams )
{

    ST_ErrorCode_t         ReturnCode = ST_NO_ERROR;
    ST_ErrorCode_t         RetCode = ST_NO_ERROR;
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;
    STCLKRV_ControlBlock_t *NextElem = NULL;
    ST_ErrorCode_t         IntReturn = 0;
    U32                    ErrorStoreCnt = 0;
    char                   RecoveryName[20] = "STCLKRV_Recovery[?]";

    /* Perform parameter validity checks */
    if (( Name                == NULL)   ||           /* NULL Name ptr          */
        (InitParams           == NULL)   ||           /* NULL structure ptr     */
        (Name[0]              == '\0' )  ||           /* Device Name undefined  */
        (strlen( Name )       >= ST_MAX_DEVICE_NAME)) /* string too long?       */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Device of that name already init */
    if (FindInstanceByName(Name) != NULL)
    {
        return (ST_ERROR_ALREADY_INITIALIZED);
    }

    if((InitParams->DeviceType < STCLKRV_DEVICE_TYPE_5100) ||
       (InitParams->DeviceType > STCLKRV_DEVICE_TYPE_BASELINE_ONLY))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    if(InitParams->DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY )
    {
        /* required for all*/
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
        if( (InitParams->FSBaseAddress_p == NULL) ||
          (((InitParams->DeviceType == STCLKRV_DEVICE_TYPE_7100) ||
            (InitParams->DeviceType == STCLKRV_DEVICE_TYPE_7200))&&
            (InitParams->AUDCFGBaseAddress_p == NULL)) )
#else
        if( (InitParams->FSBaseAddress_p == NULL) ||
            ((InitParams->DeviceType == STCLKRV_DEVICE_TYPE_7710) &&
            (InitParams->ADSCBaseAddress_p == NULL)) )
#endif
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
        /* Check For EVT Device Name */
        if((InitParams->EVTDeviceName[0] == '\0')||
            ( strlen( InitParams->EVTDeviceName ) >= ST_MAX_DEVICE_NAME ))
        {
            return( ST_ERROR_BAD_PARAMETER );
        }
    }
    if( (InitParams->DeviceType == STCLKRV_DEVICE_TYPE_7200) &&
        (InitParams->CRUBaseAddress_p == NULL) )
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
#ifndef STCLKRV_NO_PTI
#if defined (ST_7710) || defined (ST_5105)|| defined (ST_5188) || defined (ST_5107)
    /* Check For PTI Device Name C1 core limitation
       STC needs to be read in free run  mode  */
    if((InitParams->PTIDeviceName[0] == '\0')||
    ( strlen( InitParams->PTIDeviceName ) >= ST_MAX_DEVICE_NAME ))
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif
#endif /* STCLKRV_NO_PTI */

    /* Check For Memory parition */
    if (InitParams->Partition_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    if(InitParams->DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY )
    {
        /* Check averaging window parameters */
        if ((InitParams->MaxWindowSize == 0) ||
            (InitParams->MinSampleThres > InitParams->MaxWindowSize))
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
    }

    if( (InitParams->DeviceType == STCLKRV_DEVICE_TYPE_5525 ) ||
        (InitParams->DeviceType == STCLKRV_DEVICE_TYPE_7200 ) )
    {
        /* Check averaging window parameters */
        if( !((InitParams->DecodeType == STCLKRV_DECODE_PRIMARY) ||
            (InitParams->DecodeType == STCLKRV_DECODE_SECONDARY)))
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
    }


    /* All parameters ok, so Start the initialization process
     * Create instance list, Set FS and DCO HW, Start Filter task, Install interrupt,
     * register events and callbacks
    */
    if (ClkrvList_p == NULL)
    {
       TmpCB_p = (STCLKRV_ControlBlock_t *) memory_allocate(InitParams->Partition_p,
                                                        sizeof(STCLKRV_ControlBlock_t));
    }
    else
    {
        /* check whether hardware has been initialized before?? */

        if(InitParams->DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY )
        {
            NextElem = ClkrvList_p;
            while(NextElem != NULL)
            {
                if ( NextElem->InitPars.DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY )
                {
                    if ( ( NextElem->InitPars.DeviceType != STCLKRV_DEVICE_TYPE_5525) &&
                         ( NextElem->InitPars.DeviceType != STCLKRV_DEVICE_TYPE_7200) )
                    {
                        return (ST_ERROR_FEATURE_NOT_SUPPORTED); /* only one hardware init */
                    }
                }
                NextElem = NextElem->Next;
            }
        }

        TmpCB_p = (STCLKRV_ControlBlock_t *) memory_allocate(InitParams->Partition_p,
                                                     sizeof(STCLKRV_ControlBlock_t));
    }
    if (TmpCB_p == NULL)
    {
         return ST_ERROR_NO_MEMORY;
    }
    memset(TmpCB_p, 0x00, sizeof(STCLKRV_ControlBlock_t));

    /* If no list already created, init global semap */
    STOS_TaskLock();

    if (AccessSemaphore_p == NULL)
    {
        AccessSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 1);
    }

    STOS_TaskUnlock();

    /* Take semap for duration of function - to block another simultaneous init */
    STOS_SemaphoreWait(AccessSemaphore_p);

    /* Create and take instance semaphore to protect private data write */
    TmpCB_p->InstanceSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 1);

    STOS_SemaphoreWait(TmpCB_p->InstanceSemaphore_p);

    /* Load init data to instance */
    TmpCB_p->SlotEnabled = FALSE;

    strcpy(TmpCB_p->DeviceName, Name);
    TmpCB_p->OpenCount   = 0;
    TmpCB_p->InitPars    = *InitParams;
    TmpCB_p->ActvContext = InitContext;
    TmpCB_p->ActvContext.MaxFrequencyError = (PCRTIMEFACTOR * TmpCB_p->InitPars.PCRDriftThres);
    TmpCB_p->ActvRefPcr  = InitRefPcr;
    TmpCB_p->SlaveCorrectionBeingApplied = FALSE;

#ifdef ST_OSLINUX
    /* ioremap on LINUX for different base addresses */
    if(InitParams->DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        RetCode = clkrv_LinuxMapRegion(TmpCB_p);
        if(RetCode != ST_NO_ERROR)
        {
            /* Memory Mapping failed. Cannot continue... clean up and exit */
            clkrv_LinuxUnmapRegion(TmpCB_p);
            STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);
            STOS_SemaphoreDelete(NULL,TmpCB_p->InstanceSemaphore_p);
            STOS_SemaphoreSignal(AccessSemaphore_p);
            memory_deallocate(InitParams->Partition_p,TmpCB_p);

            return ST_ERROR_NO_MEMORY;
        }
    }
#endif/* #ifdef ST_OSLINUX */

#ifndef STCLKRV_NO_PTI
    /* Initialise STC baseline inactive */
    TmpCB_p->STCBaseline.STC.BaseBit32 = 0;
    TmpCB_p->STCBaseline.STC.BaseValue = 0;
    TmpCB_p->STCBaseline.STC.Extension = 0;
    TmpCB_p->FreerunSTC.Timer = 0;
    TmpCB_p->FreerunSTC.Active         = FALSE;
    if(InitParams->DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        TmpCB_p->STCBaseline.STCReferenceControl = STCLKRV_STC_SOURCE_PCR;
    }
    else
    {
        TmpCB_p->STCBaseline.STCReferenceControl = STCLKRV_STC_SOURCE_BASELINE;
    }
    TmpCB_p->OffsetInSTC = 0;
#endif /* STCLKRV_NO_PTI */

    if(InitParams->DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        /* Create array for PCR-STC error storage in Filter*/
        TmpCB_p->ActvContext.ErrorStore_p = (S32*) memory_allocate(InitParams->Partition_p,
                                                    (InitParams->MaxWindowSize * 4));
        if (TmpCB_p->ActvContext.ErrorStore_p == NULL)
        {
            /* No memory for filter. Cannot continue... clean up and exit */
#ifdef ST_OSLINUX
            clkrv_LinuxUnmapRegion(TmpCB_p);
#endif
            STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);
            STOS_SemaphoreDelete(NULL,TmpCB_p->InstanceSemaphore_p);
            STOS_SemaphoreSignal(AccessSemaphore_p);
            memory_deallocate(InitParams->Partition_p,TmpCB_p);

            return ST_ERROR_NO_MEMORY;
        }
        else
        {
            for (ErrorStoreCnt=0; ErrorStoreCnt!=TmpCB_p->InitPars.MaxWindowSize; ErrorStoreCnt++)
            {
                /* Initialize to Zeroes */
                TmpCB_p->ActvContext.ErrorStore_p[ErrorStoreCnt] = 0;
            }
        }

        /* setup of STC/SD video frequency to nominal 27 MHz*/
        /* + Invalidate Slaves */

        RetCode = clkrv_InitHW(TmpCB_p);

#ifndef STCLKRV_NO_PTI
        /* Set PCR callback function for pcr event subscription from STPTI */
        if (RetCode == ST_NO_ERROR)
        {
            /* Set pcr event subscription for STPTI */
            if (SubscribePCR(TmpCB_p) != ST_NO_ERROR)
            {
                ReturnCode = STCLKRV_ERROR_HANDLER_INSTALL;
                RetCode = STCLKRV_ERROR_HANDLER_INSTALL;
            }
        }
#endif

        if (RetCode == ST_NO_ERROR)
        {
            /* Register Events */
            RetCode = RegisterEvents(TmpCB_p);
            if (RetCode != ST_NO_ERROR)
            {
                ReturnCode = STCLKRV_ERROR_EVT_REGISTER;
            }
        }

        RecoveryName[17] = (char)((U8)'0' + (TmpCB_p->HWContext.Clocks[0].ClockType
                                      == STCLKRV_CLOCK_SD_0 ? 0 : 1));
        strcpy(TmpCB_p->RecoveryName, RecoveryName);

#ifndef STCLKRV_NO_PTI
        /* Filter Task Creation */
        if (ReturnCode == ST_NO_ERROR)
        {
            /* Create the Filter idle semaphore required to control the timer task */
            TmpCB_p->FilterSemaphore_p = STOS_SemaphoreCreateFifo(NULL, 0);

            TmpCB_p->FilterTaskActive = TRUE;

            RetCode = STOS_TaskCreate((void(*)(void *))ClockRecoveryFilterTask,
                                        TmpCB_p,
                                        TmpCB_p->InitPars.Partition_p,
                                        STCLKRV_FILTER_STACK_SIZE,
                                        &TmpCB_p->FilterTaskStack_p,
                                        TmpCB_p->InitPars.Partition_p,
                                        &TmpCB_p->FilterTask_p,
                                        &TmpCB_p->FilterTaskDescriptor,
                                        STCLKRV_FILTER_TASK_PRIORITY,
                                        TmpCB_p->RecoveryName,
                                        (task_flags_t)0);
            if (RetCode != ST_NO_ERROR)
            {
                /* Flag the timer as waiting to be deleted */
                TmpCB_p->FilterTaskActive = FALSE;

                STOS_SemaphoreSignal(TmpCB_p->FilterSemaphore_p);

                /* The task should be in the process of returning, but wait for
                * it first before deleting it. */
                if (STOS_TaskWait(&TmpCB_p->FilterTask_p,TIMEOUT_INFINITY) == 0)
                {
                    /* Now it should be safe to delete the task */
                    if (STOS_TaskDelete(TmpCB_p->FilterTask_p,
                                        (partition_t *)TmpCB_p->InitPars.Partition_p,
                                        TmpCB_p->FilterTaskStack_p,
                                        (partition_t *)TmpCB_p->InitPars.Partition_p
                                         ) == 0)
                    {
                        /* Delete all semaphores associated with the task */
                        STOS_SemaphoreDelete(NULL,TmpCB_p->FilterSemaphore_p);
                    }
                }
                ReturnCode = ST_ERROR_NO_MEMORY;
            }
        }
#endif /* STCLKRV_NO_PTI */

        /* Install DCO Interrupt */

        EnterCriticalSection();
        if (ReturnCode == ST_NO_ERROR)
        {
            IntReturn = STOS_InterruptInstall( TmpCB_p->InitPars.InterruptNumber,
                                               TmpCB_p->InitPars.InterruptLevel,
                                               ClkrvTrackingInterruptHandler,
                                               TmpCB_p->RecoveryName,
                                               TmpCB_p);
            if (IntReturn == 0)
            {
                IntReturn = STOS_InterruptEnable(TmpCB_p->InitPars.InterruptNumber,
                                                           TmpCB_p->InitPars.InterruptLevel);
                if(IntReturn != 0)
                {
                    ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
                }
            }
            else
            {
                ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
            }
        }
        LeaveCriticalSection();
    }
    /* subscription and regsitration ok - insert in linked list - Already under protection */
    if (ReturnCode == ST_NO_ERROR)
    {
        AddToInstanceList(TmpCB_p);     /* also sets up global ptr */
        STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);
    }
    else
    {
        /* Init failed, so give up memory */
#ifdef ST_OSLINUX
        clkrv_LinuxUnmapRegion(TmpCB_p);
#endif
        STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);
        STOS_SemaphoreDelete(NULL,TmpCB_p->InstanceSemaphore_p);
        memory_deallocate(InitParams->Partition_p,TmpCB_p->ActvContext.ErrorStore_p);
        memory_deallocate(InitParams->Partition_p,TmpCB_p);
    }

    STOS_SemaphoreSignal(AccessSemaphore_p);

    return (ReturnCode);
}

/****************************************************************************
Name         : STCLKRV_Open()

Description  : Opens the Clock Recovery Interface for operation.
               MUST have been preceded by a successful Init call.
               Note that this routine must cater for multi-thread usage.
               The first call actually "opens" the device. Subsequent calls
               just return the same handle as the first call & increment an
               open count.

Parameters   : STCLKRV_DeviceName_t name (matching name given to Init)
               Pointer to STCLKRV_OpenParams_t,
               Pointer to STCLKRV_Handle_t for return of the Handle.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               ST_ERROR_UNKNOWN_DEVICE       Invalid Name/no prior Init
               ST_ERROR_BAD_PARAMETER        One or more invalid parameters

See Also     : STCLKRV_Handle_t
               STCLKRV_Init()
               STCLKRV_Close()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Open( const ST_DeviceName_t       Name,
                             const STCLKRV_OpenParams_t  *OpenParams,
                             STCLKRV_Handle_t            *Handle )
{

    STCLKRV_ControlBlock_t *TmpCB_p = NULL;     /* Temp ptr into instance list */

   /* Perform parameter validity checks */

    if ((Name   == NULL) ||                 /* Name   ptr uninitialised? */
        (Handle == NULL))                   /* Handle ptr uninitialised? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Device must already exist in list ie, had prior call to init */
    TmpCB_p = FindInstanceByName(Name);

    if (TmpCB_p == NULL)
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Return ptr to block */
    *Handle =  (STCLKRV_Handle_t )TmpCB_p;

    /* Protect and open status */
    EnterCriticalSection();

    TmpCB_p->OpenCount++;

    LeaveCriticalSection();

    return (ST_NO_ERROR);
}


/****************************************************************************
Name         : STCLKRV_Enable()

Description  : Enables the clock recovery handling of PCR's by setting
               the PCRHandlingActive flag in given instance ControlBlock.

Parameters   : STCLKRV_Handle_t Handle

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

See Also     : STCLKRV_Disable()
 ****************************************************************************/
#ifndef STCLKRV_NO_PTI
ST_ErrorCode_t STCLKRV_Enable( STCLKRV_Handle_t Handle )
{
    /* Temp ptr into instance list */
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p  == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    if (TmpCB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    /* Protect data and write to PCR control switch */
    EnterCriticalSection();
    TmpCB_p->ActvRefPcr.PCRHandlingActive = TRUE;
    LeaveCriticalSection();

    return (ST_NO_ERROR);

}

/****************************************************************************
Name         : STCLKRV_SetSTCSource

Description  : Sets thes STC source control switch of the clock recovery
               instance given to the given value.
               Allows selection of either user STC baseline or PCR derived
               STC reference value.

Parameters   : STCLKRV_Handle_t Handle
               STCLKRV_STCSource_t STCSource

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

 ****************************************************************************/
ST_ErrorCode_t STCLKRV_SetSTCSource( STCLKRV_Handle_t Handle,
                                     STCLKRV_STCSource_t STCSource)
{
    /* Temp ptr into instance list */
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;

    /* Check handle is valid */

    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p  == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    if (TmpCB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        if ((STCSource != STCLKRV_STC_SOURCE_BASELINE))
        {
            return (ST_ERROR_BAD_PARAMETER);
        }
    }
    /* Check valid source supplied */
    if ((STCSource != STCLKRV_STC_SOURCE_PCR) &&
        (STCSource != STCLKRV_STC_SOURCE_BASELINE))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    /* Protect and update instance data */
    EnterCriticalSection();

    TmpCB_p->STCBaseline.STCReferenceControl = STCSource;
    if (STCSource !=  STCLKRV_STC_SOURCE_BASELINE)
    {
        TmpCB_p->FreerunSTC.Active = FALSE;
    }

    LeaveCriticalSection();

    return (ST_NO_ERROR);
}

/****************************************************************************
Name         : STCLKRV_SetPCRSource

Description  : Sets source for the PCR collection required for clock recovery
               and invalidate the decode clock

Parameters   : STCLKRV_Handle_t ClkHandle :- Handle identifying instance to set
               STCLKRV_SourceParams_t :- PCRSource data regarding PCR
               collection


Return Value :  ST_NO_ERROR
                ST_ERROR_INVALID_HANDLE
                ST_ERROR_BAD_PARAMETER
 ****************************************************************************/
ST_ErrorCode_t STCLKRV_SetPCRSource( STCLKRV_Handle_t Handle,
                                     STCLKRV_SourceParams_t *PCRSource_p)
{
    STCLKRV_ControlBlock_t *TmpCB_p;

#ifdef ST_5188
    if ( PCRSource_p->Source_u.STPTI_s.Slot == (STDEMUX_Slot_t )NULL)
#else
    if ( PCRSource_p->Source_u.STPTI_s.Slot == (STPTI_Slot_t )NULL)
#endif
    {
        return(ST_ERROR_BAD_PARAMETER);
    }

    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    if (TmpCB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    EnterCriticalSection();

    TmpCB_p->Slot = PCRSource_p->Source_u.STPTI_s.Slot;
    TmpCB_p->SlotEnabled = TRUE;

    /* flag reset reference PCR */
    TmpCB_p->ActvRefPcr.Valid = FALSE;
    TmpCB_p->ActvRefPcr.AwaitingPCR = TRUE;

    LeaveCriticalSection();

    /* Update state machine and generate STCLKRV_PCR_DISCONTINUOUS_EVT */
    UpdateStateMachine(TmpCB_p, TRUE ); /* GNBvd06145 */

    return ST_NO_ERROR;

}

#endif /* STCLKRV_NO_PTI */

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
/****************************************************************************
Name         : STCLKRV_SetApplicationMode()

Description  : Sets the application mode available.

Parameters   : STCLKRV_Handle_t      Handle
               STCLKRV_ApplicationMode_t AppMode

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

See Also     : STCLKRV_Enable()
****************************************************************************/

ST_ErrorCode_t STCLKRV_SetApplicationMode( STCLKRV_Handle_t    Handle,
                                           STCLKRV_ApplicationMode_t AppMode)
{

    STSYS_DU32 *BaseAddress = NULL;
    STCLKRV_ControlBlock_t *CB_p = NULL;
    U32 RegValue;

    /* Check handle is valid */
    CB_p = FindInstanceByHandle(Handle);

    if (CB_p == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    if (CB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    if ( ( AppMode < STCLKRV_APPLICATION_MODE_NORMAL) ||
          (AppMode > STCLKRV_APPLICATION_MODE_SD_ONLY) )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    if (CB_p->HWContext.ApplicationMode == AppMode)
        return (ST_NO_ERROR);

#ifdef ST_OSLINUX
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
#endif

    if (AppMode == STCLKRV_APPLICATION_MODE_SD_ONLY )
    {
#ifdef ST_7710
        RegValue = 0;
        /* Set the EN_PRG bit to Zero */
        clkrv_readReg(BaseAddress, FS_CLOCKGEN_CFG_2, &RegValue);
        RegValue |= 0x20;
        RegValue = (RegValue & 0x0000FFFF) | UNLOCK_KEY;   /* key for registers access */
        clkrv_writeReg(BaseAddress, FS_CLOCKGEN_CFG_2, RegValue);
#elif defined(ST_7100) || defined (ST_7109)
        RegValue = 0;
        clkrv_readReg(BaseAddress, CKGB_CLK_SRC, &RegValue);
        RegValue &= 0xFFFFFFFD;
        clkrv_writeReg(BaseAddress, CKGB_CLK_SRC, RegValue);
        /* clk_pix_sd =clk_hd/4 */
        clkrv_readReg(BaseAddress, CKGB_DISP_CFG, &RegValue);
        RegValue &= 0xFFFFF7FF;
        RegValue |= 0x400;
        clkrv_writeReg(BaseAddress, CKGB_DISP_CFG, RegValue);
#endif
        CB_p->HWContext.ApplicationMode = AppMode;

    }
    else if (AppMode == STCLKRV_APPLICATION_MODE_NORMAL )
    {
#ifdef ST_7710
        RegValue = 0;
        /* Set the EN_PRG bit to Zero */
        clkrv_readReg(BaseAddress, FS_CLOCKGEN_CFG_2, &RegValue);
        RegValue &= 0xFFFFFFDF;
        RegValue = (RegValue & 0x0000FFFF) | UNLOCK_KEY;   /* key for registers access */
        clkrv_writeReg(BaseAddress, FS_CLOCKGEN_CFG_2, RegValue);
#elif defined(ST_7100) || defined (ST_7109)
        RegValue = 0;
        clkrv_readReg(BaseAddress, CKGB_CLK_SRC, &RegValue);
        RegValue |= 0x02;
        clkrv_writeReg(BaseAddress, CKGB_CLK_SRC, RegValue);
#endif
        CB_p->HWContext.ApplicationMode = AppMode;
    }
    return (ST_NO_ERROR);
}
#endif

/****************************************************************************
Name         : STCLKRV_SetNominalFreq()

Description  : Programs the FS registers to the Nominal Frequency
               It will take around 100-150 WC time to take effect
               for the new frequency.

Parameters   : STCLKRV_Handle_t      Handle
               STCLKRV_ClockSource_t ClockSource
               U32                   Frequency

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle
               STCLKRV_INVALID_FREQUENCY       Invalid Frequency
               STCLKRV_INVALID_SLAVE           Invalid Slave

See Also     : STCLKRV_Enable()
****************************************************************************/

ST_ErrorCode_t STCLKRV_SetNominalFreq( STCLKRV_Handle_t      Handle,
                                       STCLKRV_ClockSource_t ClockSource,
                                       U32                   Frequency )
{
    U32 i = 0;
    U32 SlaveClock;
    U16 CorrectedIPE = 0;
    S32 FreqOffsetInSTC = 0;
    S32 InitialOffsetInPE = 0;
    S8  ChangeInMD=0,CorrectedMD=0;
    U32 CorrectedFrequency = 0;
    STSYS_DU32 *BaseAddress = NULL;
    STCLKRV_ControlBlock_t *CB_p = NULL;
    U32 Index = 0;
    STSYS_DU32 *BaseAddressCRU = NULL;

    /* Check handle is valid */
    if (NULL == (CB_p = FindInstanceByHandle(Handle)))
        return (ST_ERROR_INVALID_HANDLE);

    /* No clock programming in base line mode */
    if (CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);

    /* Check if clock is valid on this chip */
    if( ! IsClockValid(CB_p, ClockSource) )
        return (STCLKRV_ERROR_INVALID_SLAVE);

    /* Check if clock is valid on this chip */

    /* Search Frequency in Freq. Table */
    for(i=0;((i<MAX_NO_IN_FREQ_TABLE)&&(Frequency!= NominalFreqTable[i].Frequency));i++);

    if(i == MAX_NO_IN_FREQ_TABLE)
    {
        /* Could not find the given frequency in the table */
        return STCLKRV_ERROR_INVALID_FREQUENCY;
    }

    /* Valid clock & freq. */
#ifdef ST_OSLINUX
    BaseAddress = (STSYS_DU32 *)CB_p->FSMappedAddress_p;
#else
    BaseAddress = (STSYS_DU32 *)CB_p->InitPars.FSBaseAddress_p;
#endif

    EnterCriticalSection();

    /* Reset DCO_CMD  to 0x01 so that no interrupt occurs */

    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
    {
#ifdef ST_OSLINUX
        BaseAddressCRU = (STSYS_DU32 *)CB_p->CRUMappedAddress_p;
#else
        BaseAddressCRU = (STSYS_DU32 *)CB_p->InitPars.CRUBaseAddress_p;
#endif
        clkrv_writeReg(BaseAddressCRU, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x01);
    }
    else
    {
        clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x01);
    }
    LeaveCriticalSection();

    /* Mutually exclusive with FILTER task */
    /* STFAE - To avoid lock up in STAUD on SFR interrupt */
#if defined(ST_OS20)||defined(ST_OS21)
    if (task_context(NULL,NULL)==task_context_task)
#endif
    {
        STOS_SemaphoreWait(CB_p->InstanceSemaphore_p);
    }

    Index = FindClockIndex(CB_p, ClockSource);


    /* Zero Offset in table for STC/SD clock*/
    FreqOffsetInSTC = (S32)(CB_p->HWContext.Clocks[0].Frequency
                                - NominalFreqTable[0].Frequency);

    /* Ratio is multiple of 1000 and Step is of 100 so there we placed 10 */
    InitialOffsetInPE = (S32)((FreqOffsetInSTC * (S32)NominalFreqTable[0].Ratio)
                                         / (S32)(NominalFreqTable[0].Step * 10));

    /* Calculate change in MD, IPE */
    CorrectedIPE = (U16)((S32)NominalFreqTable[i].IPE + (S32)InitialOffsetInPE);
    /* if roll over, md needs to be changed */
    if(CorrectedIPE&0x8000)
    {
        /* calculate change in md, +ve or -ve change */
        ChangeInMD = (InitialOffsetInPE >= 0 ? 1 : -1);
        /* MD change, +1, -1 or 0*/
        CorrectedMD = NominalFreqTable[i].MD - ChangeInMD;
    }
    else
    {
        CorrectedMD = NominalFreqTable[i].MD;
    }
    /* Corrected IPE value */
    CorrectedIPE = CorrectedIPE & 0x7fff;

    /* Reset the clocks in software, HW should take care of itself */
    /* Doing this will discard any previous drift of other slave
       It is like resetting Drfit correction process */
    /* Everything else for other slave must be untouched */

    /* Clock[0] - Always Pixel clock */
    for ( SlaveClock = 1; SlaveClock < Index; SlaveClock++)
    {
        CB_p->HWContext.Clocks[SlaveClock].PrevCounter = 0;
        CB_p->HWContext.Clocks[SlaveClock].DriftError  = 0;
    }

    /* update Slave clock */
    CorrectedFrequency = (S32)NominalFreqTable[i].Frequency +
                                (S32)((InitialOffsetInPE * (S32)NominalFreqTable[i].Step)/100);

    CB_p->HWContext.Clocks[Index].PrevCounter = 0;
    CB_p->HWContext.Clocks[Index].DriftError  = 0;
    CB_p->HWContext.Clocks[Index].MD    = CorrectedMD;
    CB_p->HWContext.Clocks[Index].IPE   = CorrectedIPE;
    CB_p->HWContext.Clocks[Index].SDIV  = NominalFreqTable[i].SDIV;
    CB_p->HWContext.Clocks[Index].Step  = NominalFreqTable[i].Step;
    CB_p->HWContext.Clocks[Index].Ratio = NominalFreqTable[i].Ratio;
    CB_p->HWContext.Clocks[Index].ClockType = ClockSource;
    CB_p->HWContext.Clocks[Index].Frequency = CorrectedFrequency;
    CB_p->HWContext.Clocks[Index].SlaveWithCounter = SlaveHaveCounter(ClockSource);
    CB_p->HWContext.Clocks[Index].TicksToElapse =
                      (NominalFreqTable[i].Frequency * STCLKRV_INTERRUPT_TIME_SPAN );

    /* Update HW to reflect glitch free new freq. */
    clkrv_setClockFreq(CB_p, ClockSource, Index);

    /* Load referenece Counter **********************************/
#if defined (WA_GNBvd44290)|| defined (WA_GNBvd54182)|| defined(WA_GNBvd56512)
    /* otherwise we will have some audio issues during vtg mode change */
    stclkrv_GNBvd44290_ResetClkrvTracking();
    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
        clkrv_writeReg( BaseAddressCRU, CLKRV_REF_MAX_CNT(GetRegIndex(CB_p)),
                        (U32)(ENCODER_FREQUENCY * STCLKRV_INTERRUPT_TIME_FIRST ));
    else
        clkrv_writeReg( BaseAddress, CLKRV_REF_MAX_CNT(GetRegIndex(CB_p)),
                        (U32)(ENCODER_FREQUENCY * STCLKRV_INTERRUPT_TIME_FIRST ));
#else
    /* Load referenece Counter **********************************/
    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
        clkrv_writeReg( BaseAddressCRU, CLKRV_REF_MAX_CNT(GetRegIndex(CB_p)),
                        (U32)(ENCODER_FREQUENCY * STCLKRV_INTERRUPT_TIME_SPAN ));
    else
        clkrv_writeReg( BaseAddress, CLKRV_REF_MAX_CNT(GetRegIndex(CB_p)),
                        (U32)(ENCODER_FREQUENCY * STCLKRV_INTERRUPT_TIME_SPAN ));
#endif /* WA_GNBvd44290 || WA_GNBvd54182 || WA_GNBvd56512*/

    EnterCriticalSection();

    /* ReSet DCO_CMD  to 0x01 */
    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
        clkrv_writeReg(BaseAddressCRU, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x01);
    else
        clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x01);

    LeaveCriticalSection();

    /* Delay not required for 7710 */
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) \
 || defined (ST_7100) || defined (ST_7109) || defined (ST_5188) \
 || defined (ST_5525) || defined (ST_7200)
    CLKRV_DELAY(10);
#endif

    EnterCriticalSection();

    if( CB_p->InitPars.DeviceType == STCLKRV_DEVICE_TYPE_7200)
        clkrv_writeReg(BaseAddressCRU, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x00);
    else
        clkrv_writeReg(BaseAddress, CLKRV_REC_CMD(GetRegIndex(CB_p)), 0x00);

    LeaveCriticalSection();

    /* END - Loading referenece Counter */

    /* This slave is valid from now ON */
    CB_p->HWContext.Clocks[Index].Valid = TRUE;

    if( Index == CB_p->HWContext.ClockIndex )
    {
        CB_p->HWContext.ClockIndex++;
    }

    /* STFAE - For semaphore on interrupt */
#if defined(ST_OS20)||defined(ST_OS21)
    if (task_context(NULL,NULL)==task_context_task)
#endif
    {
        STOS_SemaphoreSignal(CB_p->InstanceSemaphore_p);
    }
    return ST_NO_ERROR;
}


/****************************************************************************
Name         : STCLKRV_GetExtendedSTC()

Description  : If in Clock recovery mode returns the full precision STC value
               if a recent PCR update has been received. This is the addition
               of the PCR value and elapsed time since the last packet
               (non PCR) was received.
               If in STC baselining mode returns the given STC baseline value
               plus the time elapsed since the last update.

Parameters   : STCLKRV_Handle_t      Handle
               STCLKRV_ExtendedSTC_t Pointer to returned STC value.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               ST_ERROR_INVALID_HANDLE       Not open or invalid handle
               ST_ERROR_BAD_PARAMETER        One or more invalid parameters
               STCLKRV_ERROR_PCR_UNAVAILABLE Accurate Decode unavailable

See Also     : STCLKRV_Open()
 ****************************************************************************/
#ifndef STCLKRV_NO_PTI
ST_ErrorCode_t STCLKRV_GetExtendedSTC(STCLKRV_Handle_t Handle,
                                      STCLKRV_ExtendedSTC_t *ExtendedSTC )
{
    BOOL STCErrorRet = TRUE;
    STCLKRV_ControlBlock_t *TmpCB_p;
    ST_ErrorCode_t ReturnCode = STCLKRV_ERROR_PCR_UNAVAILABLE;
    S32 Temp1_s32, Temp2_s32;
    S32 Carry;
    U32 ArrivalTimeBaseBit32 = 0;
    U32 ArrivalTimeBaseValue = 0;
    U32 ArrivalTimeExtension = 0;
    U32 ElapsedTime;
    U32 RefSTCBaseValue;
    U32 RefSTCBaseBit32;
    U32 RefSTCExtension;
    U32 RefArrivalTimeBaseValue;
    U32 RefArrivalTimeBaseBit32;
    U32 RefArrivalTimeExtension;

    clock_t MicroSecsSinceLastCall;
    STCLKRV_ExtendedSTC_t FreerunIncSTC;
    STCLKRV_ExtendedSTC_t UpdatedSTC;

    U32 STCReferenceControl;
    BOOL FreerunSTCActive;
    BOOL ActvRefPCRValid;
    clock_t FreerunSTCTimer;
    STCLKRV_ExtendedSTC_t STCBaseline;
    STCLKRV_ExtendedSTC_t STCPcrSync;
    STCLKRV_ExtendedSTC_t OffsetInExtSTC;
#ifdef ST_5188
    STDEMUX_Slot_t STPTISlot;
#else
    STPTI_Slot_t STPTISlot;
#endif
#if defined (ST_5100)
    clock_t HPTTicks;
#else
#ifdef ST_5188
    STDEMUX_TimeStamp_t STCCurrentTime; /* @ 90 kHz*/
#else
    STPTI_TimeStamp_t STCCurrentTime; /* @ 90 kHz*/
#endif
    clock_t TimeIn90kHzUnits;
#endif
    S32 STCOffset;

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);
    if (TmpCB_p == NULL)
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* NULL variable ptr? */
    if( ExtendedSTC == NULL )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Protect access and get shared data */
    EnterCriticalSection();
    STCReferenceControl = (U32) TmpCB_p->STCBaseline.STCReferenceControl;
    FreerunSTCActive    = TmpCB_p->FreerunSTC.Active;
    FreerunSTCTimer     = TmpCB_p->FreerunSTC.Timer;
    STCBaseline         = TmpCB_p->STCBaseline.STC;
    ActvRefPCRValid     = TmpCB_p->ActvRefPcr.Valid;
    RefSTCBaseValue     = TmpCB_p->ActvRefPcr.PcrBaseValue;
    RefSTCBaseBit32     = TmpCB_p->ActvRefPcr.PcrBaseBit32;
    RefSTCExtension     = TmpCB_p->ActvRefPcr.PcrExtension;
    RefArrivalTimeBaseValue = TmpCB_p->ActvRefPcr.PcrArrivalBaseValue;
    RefArrivalTimeBaseBit32 = TmpCB_p->ActvRefPcr.PcrArrivalBaseBit32;
    RefArrivalTimeExtension = TmpCB_p->ActvRefPcr.PcrArrivalExtension;
    STCOffset = TmpCB_p->OffsetInSTC;
    STPTISlot = TmpCB_p->Slot;
    LeaveCriticalSection();

	STTBX_Print(("\nYxlInfo:CLKH=%ld %lx %d %d %d",Handle,TmpCB_p,STCReferenceControl,
		FreerunSTCActive,ActvRefPCRValid));

    /* When in baseline mode, always freerun */
    if ((STCReferenceControl == (U32) STCLKRV_STC_SOURCE_BASELINE) && (FreerunSTCActive == TRUE))
    {
#if defined (ST_5100)
        /* Get high priority timer ticks */
        GetHPTimer(HPTTicks);

        /* calc time elapsed in ticks */
        MicroSecsSinceLastCall = STOS_time_minus(HPTTicks, FreerunSTCTimer);

        /* Adjust to usecs */
        AdjustHPTTicksToMicroSec(MicroSecsSinceLastCall);

        /* Convert elapsed  time to ExtSTC format */
        ConvertMicroSecsToExtSTC(MicroSecsSinceLastCall, &FreerunIncSTC);

#else /* 7710, 5105, 5188,5107  C1 cores */
#ifdef ST_5188
        if (STDEMUX_GetCurrentTimer(TmpCB_p->InitPars.PTIDeviceName,
                                 &STCCurrentTime) != ST_NO_ERROR)
#else
        if (STPTI_GetCurrentPTITimer(TmpCB_p->InitPars.PTIDeviceName,
                                     &STCCurrentTime) != ST_NO_ERROR)
#endif
        {
			STTBX_Print(("\nYxlInfo: come from here"));
            return STCLKRV_ERROR_PCR_UNAVAILABLE;
        }

		STTBX_Print(("\n YxlInfo:PtiNameAA=%s Value=%ld %ld %ld",
			TmpCB_p->InitPars.PTIDeviceName,STCCurrentTime.Bit32,STCCurrentTime.LSW,FreerunSTCTimer));

        TimeIn90kHzUnits = STOS_time_minus(STCCurrentTime.LSW, FreerunSTCTimer);

        /* Convert elapsed  time to ExtSTC format */
        FreerunIncSTC.BaseValue = TimeIn90kHzUnits;
        FreerunIncSTC.BaseBit32 = 0;
        FreerunIncSTC.Extension = 0;

        /* Approx value will work */
        MicroSecsSinceLastCall = (TimeIn90kHzUnits * 11);

#endif

        /* Add elapsed time in ExtSTC format to baseline */
        AddTwoExtSTCValues(FreerunIncSTC, STCBaseline, &UpdatedSTC);

        if (MicroSecsSinceLastCall >= STC_REBASELINE_TIME)
        {
            /* Protect access and  update control values */
            EnterCriticalSection();
            TmpCB_p->STCBaseline.STC.BaseValue = UpdatedSTC.BaseValue;
            TmpCB_p->STCBaseline.STC.BaseBit32 = UpdatedSTC.BaseBit32;
            TmpCB_p->STCBaseline.STC.Extension = UpdatedSTC.Extension;
#if defined (ST_5100)
            TmpCB_p->FreerunSTC.Timer = HPTTicks;
#else
            TmpCB_p->FreerunSTC.Timer = (clock_t)STCCurrentTime.LSW;
#endif
            LeaveCriticalSection();
        }

        /* Add Offset in returned value */
        OffsetInExtSTC.BaseValue = abs(STCOffset);
        OffsetInExtSTC.BaseBit32 = 0;
        OffsetInExtSTC.Extension = 0;

        if( STCOffset == 0)
        {
            /* Copy result to passed locations */
            ExtendedSTC->BaseBit32 = UpdatedSTC.BaseBit32;
            ExtendedSTC->BaseValue = UpdatedSTC.BaseValue;
            ExtendedSTC->Extension = UpdatedSTC.Extension;
        }
        else if( STCOffset > 0)
        {
            /* Add elapsed time in ExtSTC format to baseline */
            AddTwoExtSTCValues(OffsetInExtSTC, UpdatedSTC, ExtendedSTC);
        }
        else if ( STCOffset < 0)
        {
            /* Subtract elapsed time in ExtSTC format to baseline */
            SubtractTwoExtSTCValues(OffsetInExtSTC, UpdatedSTC, ExtendedSTC);
        }

        ReturnCode = ST_NO_ERROR;

    }
    else     /* If not in baseline mode....*/
    {
        if ((STCReferenceControl == (U32) STCLKRV_STC_SOURCE_PCR) && (ActvRefPCRValid))
        {

#ifdef ST_5188
            STCErrorRet = STDEMUX_GetCurrentTimer(TmpCB_p->InitPars.PTIDeviceName,
                                                                   &STCCurrentTime);
            ArrivalTimeBaseBit32 = STCCurrentTime.Bit32;
            ArrivalTimeBaseValue = STCCurrentTime.LSW;
            ArrivalTimeExtension = 0;
#else
            STCErrorRet = GetExtendedPacketArrivalTime(STPTISlot,
                                                       &ArrivalTimeBaseBit32,
                                                       &ArrivalTimeBaseValue,
                                                       &ArrivalTimeExtension );
#endif
        }

        if (!STCErrorRet)
        {
            /* Calculate time elapsed between previous reference and current reference point */
            ElapsedTime = STOS_time_minus( ArrivalTimeBaseValue, RefArrivalTimeBaseValue );

            /* restrict elapsed time to reasonable level */
            if( ( ElapsedTime >> PCR_STALE_SHIFT ) == 0 )
            {
                /* In order to calculate the correct STC we use
                   the following formula:
                   ((Packet Arrival Time) - (PCR Arrival Time)) + (PCR)
                   or for STC baselining scenario....
                   ((Packet Arrival Time) - (time of last baseline update)) + (baseline)
                */

                /* Start with modulo 300, 27MHz part */
                Temp1_s32 = (((S32)ArrivalTimeExtension - (S32)(RefArrivalTimeExtension)) +
                             (S32)(RefSTCExtension) );

                /* Test for over/underflow */
                if( Temp1_s32 > CONV90K_TO_27M )
                {
                    Carry = 1;                     /* Overflow */
                    Temp1_s32 -= CONV90K_TO_27M;
                }
                else if( Temp1_s32 < 0 )
                {
                    Carry = -1;                    /* Underflow */
                    Temp1_s32 += CONV90K_TO_27M;
                }
                else
                {
                    Carry = 0;
                }
                ArrivalTimeExtension = (U32)Temp1_s32;


                /* Calculate 90KHz, 32 bit value in two stages
                   in order to allow for overflow Carry/Borrow
                   is brought forward from previous stage */
                Temp1_s32 = ( (S32)(ArrivalTimeBaseValue & 0xffff) -
                              (S32)(RefArrivalTimeBaseValue & 0xffff) +
                              (S32)(RefSTCBaseValue & 0xffff) +
                              Carry );

                Carry = ( Temp1_s32 > 0x0000ffffL ) ? 1 : 0;
                Carry = ( Temp1_s32 < 0 ) ? -1 : Carry;

                /* High 16 bits of BaseValue + carry from low 16 */
                Temp2_s32 = ( (S32)( (ArrivalTimeBaseValue >> 16) & 0xffff ) -
                              (S32)( (RefArrivalTimeBaseValue >> 16) & 0xffff ) +
                              (S32)( (RefSTCBaseValue >> 16) & 0xffff ) +
                              Carry );

                Carry = ( Temp2_s32 > 0x0000ffffL ) ? 1 : 0;
                Carry = ( Temp2_s32 < 0 ) ? -1 : Carry;

                ArrivalTimeBaseValue = ((Temp2_s32 & 0xffff) << 16 |
                                        (Temp1_s32 & 0xffff));


                /* Calculate top bit with carry from previous stage */
                ArrivalTimeBaseBit32 = (((S32)ArrivalTimeBaseBit32 -
                                         (S32)(RefArrivalTimeBaseBit32)) +
                                         ((S32)(RefSTCBaseBit32 + Carry ) & 1));

                /* Add Offset in returned value */
                OffsetInExtSTC.BaseValue = abs(STCOffset);
                OffsetInExtSTC.BaseBit32 = 0;
                OffsetInExtSTC.Extension = 0;

                STCPcrSync.BaseBit32 = ArrivalTimeBaseBit32;
                STCPcrSync.BaseValue = ArrivalTimeBaseValue;
                STCPcrSync.Extension = ArrivalTimeExtension;

                if( STCOffset == 0)
                {
                    /* Copy result to passed locations */
                    ExtendedSTC->BaseBit32 = ArrivalTimeBaseBit32;
                    ExtendedSTC->BaseValue = ArrivalTimeBaseValue;
                    ExtendedSTC->Extension = ArrivalTimeExtension;

                    ReturnCode = ST_NO_ERROR;       /* everything OK */
                    return ReturnCode;

                }
                else if( STCOffset > 0)
                {
                    /* Add elapsed time in ExtSTC format to baseline */
                    AddTwoExtSTCValues(OffsetInExtSTC, STCPcrSync, &UpdatedSTC);
                }
                else if ( STCOffset < 0)
                {
                    /* Subtract elapsed time in ExtSTC format to baseline */
                    SubtractTwoExtSTCValues(OffsetInExtSTC, STCPcrSync, &UpdatedSTC);
                }

                /* Copy result to passed locations */
                ExtendedSTC->BaseBit32 = UpdatedSTC.BaseBit32;
                ExtendedSTC->BaseValue = UpdatedSTC.BaseValue;
                ExtendedSTC->Extension = UpdatedSTC.Extension;

                ReturnCode = ST_NO_ERROR;       /* everything OK */
            }

        }/* else failed to obtain STC */

    } /* else not stc baseline mode */

    return ReturnCode;
}

/****************************************************************************
Name         : STCLKRV_GetSTC()

Description  : Returns the decoder clock time if a recent PCR update has
               been received. This is the addition of the PCR value and
               elapsed time since the last packet (non PCR) was received.
               If baselining is enabled the STCbaseline is incremented based
               on the time elapsed since last update.

Parameters   : STCLKRV_Handle_t Handle
               U32 *STC   pointer to caller's System Time Clock variable

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               ST_ERROR_BAD_PARAMETER        One or more invalid parameters
               ST_ERROR_INVALID_HANDLE       Not open or invalid handle
               STCLKRV_ERROR_PCR_UNAVAILABLE Accurate Decode unavailable

See Also     : STCLKRV_Open()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_GetSTC( STCLKRV_Handle_t Handle,
                               U32 *STC )
{
    ST_ErrorCode_t ReturnCode = STCLKRV_ERROR_PCR_UNAVAILABLE;
    U32  ArrivalTime;
    U32  ElapsedTime;
    BOOL STCErrorRet = TRUE;
    STCLKRV_ControlBlock_t *TmpCB_p;

    clock_t MicroSecsSinceLastCall;
    STCLKRV_ExtendedSTC_t FreerunIncSTC;
    STCLKRV_ExtendedSTC_t UpdatedSTC;

    U32 STCReferenceControl;
    BOOL FreerunSTCActive;
    BOOL ActvRefPCRValid;
    clock_t FreerunSTCTimer;
    STCLKRV_ExtendedSTC_t STCBaseline;
    U32 ActvRefPCRBaseValue;
    U32 ActvRefPCRTime;
#ifdef ST_5188
    STDEMUX_Slot_t STPTISlot;
#else
    STPTI_Slot_t STPTISlot;
#endif
#if defined (ST_5100)
    clock_t HPTTicks;
#else
#ifdef ST_5188
    STDEMUX_TimeStamp_t STCCurrentTime; /* @ 90 kHz*/
#else
    STPTI_TimeStamp_t STCCurrentTime; /* @ 90 kHz*/
#endif
    clock_t TimeIn90kHzUnits;
#endif
    S32 STCOffset;

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);
    if (TmpCB_p == NULL)
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    if( STC == NULL )      /* NULL variable ptr? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    /* Protect access and get shared data */
    EnterCriticalSection();

    STCReferenceControl = (U32)TmpCB_p->STCBaseline.STCReferenceControl;
    FreerunSTCActive    = TmpCB_p->FreerunSTC.Active;
    FreerunSTCTimer     = TmpCB_p->FreerunSTC.Timer;
    STCBaseline         = TmpCB_p->STCBaseline.STC;
    ActvRefPCRValid     = TmpCB_p->ActvRefPcr.Valid;
    ActvRefPCRBaseValue = TmpCB_p->ActvRefPcr.PcrBaseValue;
    ActvRefPCRTime      = TmpCB_p->ActvRefPcr.Time;
    STPTISlot           = TmpCB_p->Slot;
    STCOffset           = TmpCB_p->OffsetInSTC;
    LeaveCriticalSection();

    /* For STC basline mode, freerunning is forced  */
    if ((STCReferenceControl == (U32)STCLKRV_STC_SOURCE_BASELINE) && (FreerunSTCActive == TRUE))
    {
#if defined (ST_5100)
        /* Get high priority timer ticks */
        GetHPTimer(HPTTicks);

        /* calc time elapsed in ticks */
        MicroSecsSinceLastCall = STOS_time_minus(HPTTicks, FreerunSTCTimer);

        /* Adjust to usecs */
        AdjustHPTTicksToMicroSec(MicroSecsSinceLastCall);

        /* Convert elapsed  time to ExtSTC format */
        ConvertMicroSecsToExtSTC(MicroSecsSinceLastCall, &FreerunIncSTC);

#else
#ifdef ST_5188
        if (STDEMUX_GetCurrentTimer(TmpCB_p->InitPars.PTIDeviceName,
                                     &STCCurrentTime) != ST_NO_ERROR)
#else
        if (STPTI_GetCurrentPTITimer(TmpCB_p->InitPars.PTIDeviceName,
                                     &STCCurrentTime) != ST_NO_ERROR)
#endif
        {
			STTBX_Print(("\nYxlInfo: come from hereAA"));

            return STCLKRV_ERROR_PCR_UNAVAILABLE;
        }

		STTBX_Print(("\n YxlInfo:PtiName=%s Value=%ld %ld %ld",
			TmpCB_p->InitPars.PTIDeviceName,STCCurrentTime.Bit32,STCCurrentTime.LSW,FreerunSTCTimer));

	

	
        TimeIn90kHzUnits = STOS_time_minus(STCCurrentTime.LSW, FreerunSTCTimer);

        /* Convert elapsed  time to ExtSTC format */
        FreerunIncSTC.BaseValue = TimeIn90kHzUnits;
        FreerunIncSTC.BaseBit32 = 0;
        FreerunIncSTC.Extension = 0;

        /* Approx value will work */
        MicroSecsSinceLastCall = (TimeIn90kHzUnits * 11);

#endif

        /* Add elapsed time in ExtSTC format to baseline */
        AddTwoExtSTCValues(FreerunIncSTC, STCBaseline, &UpdatedSTC);

        if (MicroSecsSinceLastCall >= STC_REBASELINE_TIME)
        {
            /* Protect data and update control values */
            EnterCriticalSection();
            TmpCB_p->STCBaseline.STC.BaseValue = UpdatedSTC.BaseValue;
            TmpCB_p->STCBaseline.STC.BaseBit32 = UpdatedSTC.BaseBit32;
            TmpCB_p->STCBaseline.STC.Extension = UpdatedSTC.Extension;
#if defined (ST_5100)
            TmpCB_p->FreerunSTC.Timer = HPTTicks;
#else
            TmpCB_p->FreerunSTC.Timer = STCCurrentTime.LSW;
#endif
            LeaveCriticalSection();
        }

        /* Return updated 90kHz baseline value */
        *STC = UpdatedSTC.BaseValue + STCOffset;
/*yxl 2008-05-29 add below*/

		STTBX_Print(("\nYxlInfo: Value=%ld %ld",UpdatedSTC.BaseValue,STCOffset));


        ReturnCode = ST_NO_ERROR;

    }
    else
    {
        if ((STCReferenceControl == (U32)STCLKRV_STC_SOURCE_PCR) && (ActvRefPCRValid))
        {
#ifdef ST_5188
            STCErrorRet = STDEMUX_GetCurrentTimer(TmpCB_p->InitPars.PTIDeviceName,
                                                                   &STCCurrentTime);
            ArrivalTime = STCCurrentTime.LSW*CONV90K_TO_27M;
#else
            STCErrorRet = GetPacketArrivalTime(STPTISlot, &ArrivalTime);
#endif
            if (!STCErrorRet)
            {

                /* Calulate time elapsed since reference point */

                /*  correctly handles wrap */
                ElapsedTime = (STOS_time_minus(ArrivalTime, ActvRefPCRTime)
                                             / CONV90K_TO_27M);

#if defined (CLKRV_FILTERDEBUG)
                STCLKRV_Print(("STCLKRV_GetSTC %u-%u=%u*300\n",
                             ArrivalTime, ActvRefPCRTime, ElapsedTime));
#endif

                /* restrict elapsed time to reasonable level */
                if ((ElapsedTime >> PCR_STALE_SHIFT) == 0)
                {
                    *STC = ActvRefPCRBaseValue + ElapsedTime + STCOffset;
                    ReturnCode = ST_NO_ERROR;       /* everything OK */
                }
                else
                {
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                                  "%u-%u=%u*300 judged stale\n",
                                  ArrivalTime, ActvRefPCRTime, ElapsedTime));
                }

            }
            else
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                              "STCLKRV_GetSTC: GetPacketArrivalTime failed\n"));
            }
        }
        else
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                          "STCLKRV_GetSTC: no valid PCR reference\n"));
        }
    }

    return ReturnCode;
}

/****************************************************************************
Name         : STCLKRV_SetSTCBaseline

Description  : Loads given STC baseline value to UserBaseline storage area.
               Takes time stamp for reference.

Parameters   : STCLKRV_Handle_t Handle
               STCLKRV_ExtendedSTC_t STC comprising 90KHz base value, bit32
               and 27Mhz extension value.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

See Also     : STCLKRV_Enable()
 ****************************************************************************/
ST_ErrorCode_t STCLKRV_SetSTCBaseline(STCLKRV_Handle_t Handle,
                                      STCLKRV_ExtendedSTC_t *STC)
{
    /* Temp ptr into instance list */
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;
#if defined (ST_5100)
    clock_t HPTTicks;
#else
#ifdef ST_5188
    STDEMUX_TimeStamp_t STCCurrentTime; /* @ 90 kHz*/
#else
    STPTI_TimeStamp_t STCCurrentTime; /* @ 90 kHz*/
#endif
#endif
    ST_ErrorCode_t ReturnCode = ST_NO_ERROR;

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p  == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

#if !defined (ST_5100)
#ifdef ST_5188
    if (STDEMUX_GetCurrentTimer(TmpCB_p->InitPars.PTIDeviceName,
                                 &STCCurrentTime) != ST_NO_ERROR)
#else
    if (STPTI_GetCurrentPTITimer(TmpCB_p->InitPars.PTIDeviceName,
                                 &STCCurrentTime) != ST_NO_ERROR)
#endif
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
#endif


    /* Exclusive access to data */
    EnterCriticalSection();

    /* Write given value to user baseline */
    TmpCB_p->STCBaseline.STC.BaseBit32 = STC->BaseBit32;
    TmpCB_p->STCBaseline.STC.BaseValue = STC->BaseValue;
    TmpCB_p->STCBaseline.STC.Extension = STC->Extension;
    /* Free run control */
    TmpCB_p->FreerunSTC.Active = TRUE;
#if defined (ST_5100)
    GetHPTimer(HPTTicks);
    TmpCB_p->FreerunSTC.Timer = HPTTicks;
#else
    TmpCB_p->FreerunSTC.Timer = STCCurrentTime.LSW;
#endif

    LeaveCriticalSection();

    return (ReturnCode);
}

/****************************************************************************
Name         : STCLKRV_SetSTCOffset

Description  : Sets the offset to be added in base value of returned STC
               value from GET_XXXSTC APIs.

Parameters   : STCLKRV_Handle_t Handle
               S32 Offset given in 90kHz units

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

See Also     : STCLKRV_GetExtendedSTC()
 ****************************************************************************/
ST_ErrorCode_t STCLKRV_SetSTCOffset( STCLKRV_Handle_t Handle,
                                     S32 Offset)
{
    /* Temp ptr into instance list */
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;
    ST_ErrorCode_t ReturnCode = ST_NO_ERROR;

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p  == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }

    EnterCriticalSection();
    TmpCB_p->OffsetInSTC = Offset;
    LeaveCriticalSection();

    return ReturnCode;

}


/****************************************************************************
Name         : STCLKRV_InvDecodeClk()

Description  : Invalidates the PCR reference valid flag internally, causing
               rapid PCR re-basing.

Parameters   : STCLKRV_Handle_t Handle

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

See Also     : STCLKRV_Open()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_InvDecodeClk( STCLKRV_Handle_t Handle )
{
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;     /* Temp ptr into instance list */

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);
    if (TmpCB_p  == NULL)
    {
        return( ST_ERROR_INVALID_HANDLE );
    }
    if (TmpCB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        /* Protect and update data */
        EnterCriticalSection();

        /* Invalidate freerunning STC until next baseline set */
        TmpCB_p->FreerunSTC.Active = FALSE;

        LeaveCriticalSection();
    }
    else
    {
        /* Protect and update data */
        EnterCriticalSection();

        /* Invaildate PCRs until next PCR ok */
        TmpCB_p->ActvRefPcr.Valid = FALSE;

        /* Invalidate freerunning STC until next baseline set */
        TmpCB_p->FreerunSTC.Active = FALSE;

        /* flag reset reference PCR */
        TmpCB_p->ActvRefPcr.AwaitingPCR = TRUE;

        LeaveCriticalSection();

        /* Update state machine and generate STCLKRV_PCR_DISCONTINUOUS_EVT */
        UpdateStateMachine(TmpCB_p, TRUE );
    }

    return (ST_NO_ERROR);
}

/****************************************************************************
Name         : STCLKRV_Disable()

Description  : Disables the clock recovery handling of PCR's by setting
               the PCRHandlingActive flag in given instance ControlBlock.

Parameters   : STCLKRV_Handle_t Handle

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle

See Also     : STCLKRV_Enable()
 ****************************************************************************/
ST_ErrorCode_t STCLKRV_Disable(STCLKRV_Handle_t Handle)
{
    /* Temp ptr into instance list */
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;

    /* Check handle is valid */
    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p  == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    if (TmpCB_p->InitPars.DeviceType==STCLKRV_DEVICE_TYPE_BASELINE_ONLY)
    {
        return (ST_ERROR_FEATURE_NOT_SUPPORTED);
    }
    /* Protect data and write to PCR control switch */
    EnterCriticalSection();
    TmpCB_p->ActvRefPcr.PCRHandlingActive = FALSE;
    LeaveCriticalSection();

    return (ST_NO_ERROR);
}
#endif /* STCLKRV_NO_PTI */

/****************************************************************************
Name         : STCLKRV_Close()

Description  : Closes the Handle for re-use. Only decrements OpenCount.
               Device closure performed by terminate.

Parameters   : STCLKRV_Handle_t

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         No Open on this Handle

See Also     : STCLKRV_Open()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Close( STCLKRV_Handle_t Handle )
{
    ST_ErrorCode_t RetCode = ST_NO_ERROR;
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;

    TmpCB_p = FindInstanceByHandle(Handle);

    if (TmpCB_p  == NULL)
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Protect, read and  update data */
    EnterCriticalSection();

    if (TmpCB_p->OpenCount != 0)
    {
        TmpCB_p->OpenCount--;
    }
    else
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }

    LeaveCriticalSection();

    return (RetCode);
}


/****************************************************************************
Name         : STCLKRV_Term()

Description  : Terminates the named device driver, effectively
               reversing the Initialization call.

               (NB: Using forced terminate may not be completely safe, as it
                is theoretically possible for a task to be waiting on the
                AccessSemaphore, in which case the task gets 'lost'.)

Parameters   : ST_DeviceName_t name matching that supplied with Init,
               STCLKRV_TermParams_t pointer containing ForceTerminate flag.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_UNKNOWN_DEVICE         Invalid device name
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid
               ST_ERROR_OPEN_HANDLE            Close not called/unsuccessful
               STCLKRV_ERROR_HANDLER_INSTALL   Unable to uninstall internal ISR
               STCLKRV_ERROR_EVT_REGISTER      Problem unregistering events
               ST_ERROR_INTERRUPT_UNINSTALL    Error while uninstalling interrupt

See Also     : STCLKRV_Init()
               UnregisterEvents()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Term( const ST_DeviceName_t Name,
                             const STCLKRV_TermParams_t *TermParams )
{
    STCLKRV_ControlBlock_t *TmpCB_p = NULL;
    ST_ErrorCode_t TermRetCode =  ST_NO_ERROR;    /* GNBvd05993 */

    /* Perform parameter validity checks */
    if (( Name       == NULL )   ||             /* NULL Name ptr? */
        ( TermParams == NULL ))                /* NULL structure ptr? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    TmpCB_p = FindInstanceByName(Name);
    if (TmpCB_p == NULL)
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    if ((!TermParams->ForceTerminate) && (TmpCB_p->OpenCount > 0))
    {
        return (ST_ERROR_OPEN_HANDLE);     /* need to call Close */
    }

    STOS_SemaphoreWait(TmpCB_p->InstanceSemaphore_p);

    if(TmpCB_p->InitPars.DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY )
    {
#ifndef STCLKRV_NO_PTI
        /* Unregister callback, subscriptions and event handlers */
        if (UnsubscribePCR(TmpCB_p) != ST_NO_ERROR)
        {
            TermRetCode = STCLKRV_ERROR_HANDLER_INSTALL;
        }
        /* Unregister events & close event handler */
        else if (UnregisterEvents(TmpCB_p) != ST_NO_ERROR)
        {
            TermRetCode = STCLKRV_ERROR_EVT_REGISTER;
        }
        /* Task deletion */
        if (TermRetCode == ST_NO_ERROR)
        {
            /* Flag the timer as waiting to be deleted */
            TmpCB_p->FilterTaskActive = FALSE;
            STOS_SemaphoreSignal(TmpCB_p->FilterSemaphore_p);

            /* The task should be in the process of returning, but wait for
            * it first before deleting it. */
            STOS_TaskWait(&TmpCB_p->FilterTask_p,TIMEOUT_INFINITY);
            /* Now it should be safe to delete the task */
            STOS_TaskDelete(TmpCB_p->FilterTask_p,(partition_t *)TmpCB_p->InitPars.Partition_p,
                            TmpCB_p->FilterTaskStack_p,(partition_t *)TmpCB_p->InitPars.Partition_p);
            /* Delete all semaphores associated with the Task */
            STOS_SemaphoreDelete(NULL,TmpCB_p->FilterSemaphore_p);
        }
#else
        if (UnregisterEvents(TmpCB_p) != ST_NO_ERROR)
        {
            TermRetCode = STCLKRV_ERROR_EVT_REGISTER;
        }
#endif /* STCLKRV_NO_PTI */

        if (TermRetCode == ST_NO_ERROR)
        {
            /* Uninstall the interrupt handler */
            if(STOS_InterruptDisable(TmpCB_p->InitPars.InterruptNumber,
                                     TmpCB_p->InitPars.InterruptLevel) != 0)
            {
                TermRetCode = ST_ERROR_INTERRUPT_UNINSTALL;
            }
            EnterCriticalSection();
            if (STOS_InterruptUninstall(TmpCB_p->InitPars.InterruptNumber,
                                        TmpCB_p->InitPars.InterruptLevel,
                                        (void *) TmpCB_p) != 0)
            {
                TermRetCode = ST_ERROR_INTERRUPT_UNINSTALL;
            }
            LeaveCriticalSection();
        }
#ifdef ST_OSLINUX
        if (TermRetCode == ST_NO_ERROR)
        {
            /* Called to unmap memroy regions as DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY */
            clkrv_LinuxUnmapRegion(TmpCB_p);
        }
#endif
    }

    if (TermRetCode == ST_NO_ERROR)
    {
        /* Clean up memory use by instance */
        STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);
        STOS_SemaphoreDelete(NULL,TmpCB_p->InstanceSemaphore_p);
        STOS_SemaphoreWait(AccessSemaphore_p);
        DelFromInstanceList(TmpCB_p->DeviceName);
        STOS_SemaphoreSignal(AccessSemaphore_p);
        if(TmpCB_p->InitPars.DeviceType != STCLKRV_DEVICE_TYPE_BASELINE_ONLY )
        {
            memory_deallocate(TmpCB_p->InitPars.Partition_p,
                           TmpCB_p->ActvContext.ErrorStore_p);
        }
        /* Clean up memory use by list if necessary */
        memory_deallocate(TmpCB_p->InitPars.Partition_p,TmpCB_p);
    }
    else
    {
        STOS_SemaphoreSignal(TmpCB_p->InstanceSemaphore_p);
        memory_deallocate(TmpCB_p->InitPars.Partition_p,
                          TmpCB_p->ActvContext.ErrorStore_p);
        memory_deallocate(TmpCB_p->InitPars.Partition_p,TmpCB_p);
    }

    return (TermRetCode);
}

/********************** END OF API's ***************************************/

/*********************** Private Functions *********************************/





/****************************************************************************
Name         : GetPacketArrivalTime()

Description  : Get packet arrival time from pti or stpti function

Parameters   : STCLKRV_Handle_t Handle for stpti instance identity
               U32 *ArrivalTime for value to return

Return Value : Error value: TRUE for fail
****************************************************************************/
#ifndef STCLKRV_NO_PTI
#ifndef ST_5188
static BOOL GetPacketArrivalTime(STCLKRV_Handle_t Handle, U32 *ArrivalTime)
{
    BOOL RetError = TRUE;
    STPTI_TimeStamp_t STPTI_ArrivalTime;
    U16 STPTI_ArrivalTimeExtension;

    if (STPTI_GetPacketArrivalTime (Handle,
                                    &STPTI_ArrivalTime,  /* 90Khz value */
                                    &STPTI_ArrivalTimeExtension) == ST_NO_ERROR)
    {
        RetError = FALSE;
        /* Calculate 27MHz arrival time : GNBvd06277 */
        *ArrivalTime = (U32) ((STPTI_ArrivalTime.LSW * CONV90K_TO_27M)
                              + STPTI_ArrivalTimeExtension);
    }

    return RetError;
}

/****************************************************************************
Name         : GetExtendedPacketArrivalTime()

Description  : Get extended packet arrival time from pti or stpti function

Parameters   : STCLKRV_Handle_t (U32) Handle for stpti instance identity (slot)
               U32 *BaseBit32 return value
               U32 *BaseValue return value
               U32 *Extension return value

Return Value : Error value: TRUE for fail
****************************************************************************/
static BOOL GetExtendedPacketArrivalTime(STCLKRV_Handle_t Handle,
                                  U32 *BaseBit32,
                                  U32 *BaseValue,
                                  U32 *Extension)
{
    BOOL RetError = TRUE;
    STPTI_TimeStamp_t STPTI_ArrivalTime;
    U16 STPTI_ArrivalTimeExtension;

    if (STPTI_GetPacketArrivalTime(Handle,
                                   &STPTI_ArrivalTime,
                                   &STPTI_ArrivalTimeExtension) == ST_NO_ERROR)
    {
        RetError = FALSE;
        /* Load local variables with returned values */
        *BaseBit32 = (U32) STPTI_ArrivalTime.Bit32;
        *BaseValue = (U32) STPTI_ArrivalTime.LSW;       /* 90Khz */
        *Extension = (U32) STPTI_ArrivalTimeExtension;  /* 27MHz */
    }

    return RetError;

}
#endif /* ST_5188 */
#endif /* STCLKRV_NO_PTI */


/*********************** Utility Functions *********************************/

/****************************************************************************
Name         : RegisterEvents()

Description  : Makes a call to STEVT_Open() and the registers the
               PCR Valid, Invalid & Glitch events.

Parameters   : STCLKRV_ControlBlock_t *Instance_p: ptr to instance in list

Return Value : ST_NO_ERROR      Successful Open and Registers
               Error            Error code from Open or Resgiste

****************************************************************************/
static ST_ErrorCode_t RegisterEvents( STCLKRV_ControlBlock_t *Instance_p )
{
    ST_ErrorCode_t     Error = ST_NO_ERROR;
    STEVT_OpenParams_t STEVT_OpenParams;

    Error = STEVT_Open(Instance_p->InitPars.EVTDeviceName, &STEVT_OpenParams,
                      &(Instance_p->ClkrvEvtHandle));
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Open() failed"));
        return (Error);

    }

    Error = STEVT_RegisterDeviceEvent(Instance_p->ClkrvEvtHandle,
                                       Instance_p->DeviceName,
                                       (U32) STCLKRV_PCR_VALID_EVT,
                                       &(Instance_p->RegisteredEvents[STCLKRV_PCR_VALID_ID]));
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STCLKRV_PCR_VALID_EVT) failed"));
        return  (Error);
    }

    Error = STEVT_RegisterDeviceEvent(Instance_p->ClkrvEvtHandle,
                                      Instance_p->DeviceName,
                                      (U32) STCLKRV_PCR_DISCONTINUITY_EVT,
                                      &(Instance_p->RegisteredEvents[STCLKRV_PCR_INVALID_ID]));
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STCLKRV_PCR_DISCONTINUITY_EVT) failed"));
        return (Error);
    }

    Error = STEVT_RegisterDeviceEvent(Instance_p->ClkrvEvtHandle,
                                      Instance_p->DeviceName,
                                      (U32) STCLKRV_PCR_GLITCH_EVT,
                                      &(Instance_p->RegisteredEvents[STCLKRV_PCR_GLITCH_ID]));
   if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_RegisterDeviceEvent(STCLKRV_PCR_GLITCH_EVT) failed"));
        return (Error);
    }

    return (Error);
}

/****************************************************************************
Name         : UnregisterEvents()

Description  : Unregisters the PCR Valid, Invalid & Glitch events
               and then Closes() the event handler.

Parameters   : STCLKRV_ControlBlock_t *Instance_p : ptr to instance

Return Value : ST_NO_ERROR      Successful Unregisters() and Close()
               Error            Error code from Close() or Unregister()

****************************************************************************/

static ST_ErrorCode_t UnregisterEvents( STCLKRV_ControlBlock_t *Instance_p )
{
    ST_ErrorCode_t     Error = ST_NO_ERROR;

    Error = STEVT_UnregisterDeviceEvent(Instance_p->ClkrvEvtHandle,
                                        Instance_p->DeviceName,
                                        (U32)STCLKRV_PCR_VALID_EVT );
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_UnregisterDeviceEvent(STCLKRV_PCR_VALID_EVT) failed"));
        return (Error);
    }

    Error = STEVT_UnregisterDeviceEvent(Instance_p->ClkrvEvtHandle,
                                        Instance_p->DeviceName,
                                        (U32)STCLKRV_PCR_DISCONTINUITY_EVT );
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_UnregisterDeviceEvent(STCLKRV_PCR_DISCONTINUITY_EVT) failed"));
        return (Error);
    }

    Error = STEVT_UnregisterDeviceEvent(Instance_p->ClkrvEvtHandle,
                                        Instance_p->DeviceName,
                                        (U32)STCLKRV_PCR_GLITCH_EVT );
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                    "Call to STEVT_UnregisterDeviceEvent(STCLKRV_PCR_GLITCH_EVT) failed"));
        return (Error);
    }

    Error = STEVT_Close(Instance_p->ClkrvEvtHandle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Close() failed"));
        return (Error);
    }

    return (Error);
}

/****************************************************************************
Name         : SubscribePCR()

Description  : Register callback mechansim to PTI for PCR

Parameters   : STCLKRV_ControlBlock_t *Instance_p : Instance to operate on


Return Value : ST_ErrorCode_t
 ****************************************************************************/
#ifndef STCLKRV_NO_PTI
static ST_ErrorCode_t SubscribePCR( STCLKRV_ControlBlock_t *Instance_p )
{
    ST_ErrorCode_t EvtError = ST_NO_ERROR;

   /* Register callback according to event handler method as required by stpti */

    STEVT_OpenParams_t PcrEvtOpenParams; /* Unused, only to hold return vals */
    STEVT_DeviceSubscribeParams_t PcrDeviceSubscribeParams;

    /* Callback function */
    PcrDeviceSubscribeParams.NotifyCallback   = (STEVT_DeviceCallbackProc_t)STCLKRV_ActionPCR;
    PcrDeviceSubscribeParams.SubscriberData_p = (void *)Instance_p;

    EvtError = STEVT_Open(Instance_p->InitPars.PCREvtHandlerName,
                          &PcrEvtOpenParams,&Instance_p->PcrEvtHandle);

    if (EvtError != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Open() failed"));
        return EvtError;
    }

#ifdef ST_5188
    EvtError = STEVT_SubscribeDeviceEvent(Instance_p->PcrEvtHandle,NULL,
                           (U32) STDEMUX_EVENT_PCR_RECEIVED_EVT,&PcrDeviceSubscribeParams);
#else
    EvtError = STEVT_SubscribeDeviceEvent(Instance_p->PcrEvtHandle,NULL,
                           (U32) STPTI_EVENT_PCR_RECEIVED_EVT,&PcrDeviceSubscribeParams);
#endif
    if (EvtError != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_SubsrcibeDeviceEvent() failed"));
        return EvtError;
    }

    return EvtError;
}

/****************************************************************************
Name         : UnsubscribePCR()

Description  : Unregister callback function to STPTI for PCR

Parameters   : STCLKRV_ControlBlock_t *Instance_p : instance to operate on.

Return Value : Error Code: TRUE if  unsetting callback failed, FALSE if ok
 ****************************************************************************/
static ST_ErrorCode_t UnsubscribePCR( STCLKRV_ControlBlock_t *Instance_p )
{
    ST_ErrorCode_t EvtError = ST_NO_ERROR;

#ifdef ST_5188
    EvtError = STEVT_UnsubscribeDeviceEvent(Instance_p->PcrEvtHandle,NULL,
                                 (U32) STDEMUX_EVENT_PCR_RECEIVED_EVT);
#else
    EvtError = STEVT_UnsubscribeDeviceEvent(Instance_p->PcrEvtHandle,NULL,
                                 (U32) STPTI_EVENT_PCR_RECEIVED_EVT);
#endif
    if (EvtError != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Unsubsrcibe() failed"));
        return EvtError;
    }

    EvtError = STEVT_Close(Instance_p->PcrEvtHandle);

    if (EvtError != ST_NO_ERROR )
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Call to STEVT_Close() failed"));
        return EvtError;
    }

   return EvtError;

}
#endif  /* STCLKRV_NO_PTI */



/****************************************************************************
Name         : AddToInstanceList

Description  : Adds pre-created intance of clock recovery control block
               to instance linked list.
               Adds to the end of the list.

Parameters   : STCLKRV_ControlBlock_t *newItem_p : pointer to item to add
               (pre-created prior to function call using malloc type command.)

Return Value : None
 ****************************************************************************/
static void AddToInstanceList( STCLKRV_ControlBlock_t *NewItem_p )
{
    if (ClkrvList_p == NULL)
    {
        /* If empty list, make newItem last item by NULL */
        NewItem_p->Next = NULL;
    }
    else
    {
        /* Add to start of the list */
        NewItem_p->Next = ClkrvList_p;
    }
    /* Set global ptr new first item */
    ClkrvList_p = NewItem_p;
}


/****************************************************************************
Name         : DelFromInstanceList

Description  : Deletes intance of clock recovery control block from instance
               linked list.

Parameters   : ST_DeviceName_t DeviceName : identifying device to delete from list.
               Does not deallocate the memory used by the deleted control block,
               this operation is required after the call to this function.

Return Value : FALSE is no error, TRUE is error {list is empty, item not found}.
 ****************************************************************************/
static BOOL DelFromInstanceList( ST_DeviceName_t DeviceName )
{
    STCLKRV_ControlBlock_t *Search_p;
    STCLKRV_ControlBlock_t *Previous_p;

    Search_p = ClkrvList_p;
    Previous_p = ClkrvList_p;

    while (Search_p != NULL)
    {
        /* If device name found...*/
        if (strcmp(Search_p->DeviceName,DeviceName) == 0)
        {
            /* If only item in list..*/
            if (Search_p == ClkrvList_p)
            {
                /* kill global pointer */
                ClkrvList_p = NULL;
            }
            else
            {
                /* link out item to del */
                Previous_p->Next=(STCLKRV_ControlBlock_t *)Search_p->Next;
            }

            return  FALSE;   /* all ok */
        }

        /* Step ptr thru list */
        Previous_p=(STCLKRV_ControlBlock_t *)Search_p;
        Search_p=(STCLKRV_ControlBlock_t *)Search_p->Next;

    }
    return TRUE;   /* Item not found and/or list empty */

}

/****************************************************************************
Name         : FindInstanceByName

Description  : Searches the clock recovery instance linked list for the
               instance with the given name.

Parameters   : const ST_DeviceName_t DeviceName : to identify the instance to
               search for.


Return Value :  STCLKRV_ControlBlock_t *ptr : pointer to instance in the linked
                list, or to NULL if not found.
 ****************************************************************************/
static STCLKRV_ControlBlock_t *FindInstanceByName( const ST_DeviceName_t DeviceName )
{

    STCLKRV_ControlBlock_t *Search_p;

    Search_p = ClkrvList_p;

    while (Search_p != NULL)
    {
       if (strcmp(Search_p->DeviceName,DeviceName) == 0)
       {
          return  Search_p;
       }

       Search_p = (STCLKRV_ControlBlock_t *)Search_p->Next;
    }

    return NULL;

}

#if defined(ST_OSLINUX)
BOOL STCLKRV_IsAlreadyRegistered(const ST_DeviceName_t DeviceName)
{
    return (FindInstanceByName(DeviceName) != NULL);
}
#endif

/****************************************************************************
Name         : FindInstanceByHandle

Description  : Searches the clock recovery instance linked list for the
               instance with the given Handle

Parameters   : Handle


Return Value : STCLKRV_Handle_t clkHandle : Handle to find in Inst list
 ****************************************************************************/
static STCLKRV_ControlBlock_t *FindInstanceByHandle( STCLKRV_Handle_t ClkHandle )
{

    STCLKRV_ControlBlock_t *Search_p;

    Search_p = ClkrvList_p;

    while (Search_p != NULL)
    {
        /* Is handle given the same as the address of the instance ie, valid ? */
       if (ClkHandle ==  (STCLKRV_Handle_t) Search_p)
       {
          return  Search_p;
       }

       Search_p = (STCLKRV_ControlBlock_t *)Search_p->Next;

    }

    return NULL;

}

/****************************************************************************
Name         : AddTwoExtSTCValues

Description  : Adds two given ExtSTC values.

Parameters   : ExtSTCA - First Extended STC
               ExtSTCB - Second Extended STC
               *ReturnExtSTC_p - Pointer to return storage location

Return value: None.
****************************************************************************/
#ifndef STCLKRV_NO_PTI
static void AddTwoExtSTCValues( STCLKRV_ExtendedSTC_t ExtSTCA,
                         STCLKRV_ExtendedSTC_t ExtSTCB,
                         STCLKRV_ExtendedSTC_t *ReturnExtSTC_p)
{
    S32 Temp1_s32, Temp2_s32;
    S32 Carry;
    U32 LocalTimeBaseBit32;
    U32 LocalTimeBaseValue;
    U32 LocalTimeExtension;

    /* 27Mhz component first */
    Temp1_s32 = ((S32)ExtSTCA.Extension + (S32)(ExtSTCB.Extension));

    /* Test for over/underflow */
    if( Temp1_s32 > CONV90K_TO_27M )
    {
        Carry = 1;                     /* Overflow */
        Temp1_s32 -= CONV90K_TO_27M;
    }
    else if( Temp1_s32 < 0 )
    {
        Carry = -1;                    /* Underflow */
        Temp1_s32 += CONV90K_TO_27M;
    }
    else
    {
        Carry = 0;
    }
    LocalTimeExtension = (U32)Temp1_s32;


    /* Calculate 90KHz, 32 bit value in two stages
       in order to allow for overflow Carry/Borrow
       is brought forward from previous stage */
    Temp1_s32 = ((S32)(ExtSTCA.BaseValue & 0xffff) +
                 (S32)(ExtSTCB.BaseValue & 0xffff) +
                 Carry );

    Carry = ( Temp1_s32 > 0x0000ffffL ) ? 1 : 0;
    Carry = ( Temp1_s32 < 0 ) ? -1 : Carry;

    /* High 16 bits of BaseValue + carry from low 16 */
    Temp2_s32 = ((S32)((ExtSTCA.BaseValue >> 16) & 0xffff) +
                 (S32)((ExtSTCB.BaseValue  >> 16) & 0xffff ) +
                 Carry );

    Carry = ( Temp2_s32 > 0x0000ffffL ) ? 1 : 0;
    Carry = ( Temp2_s32 < 0 ) ? -1 : Carry;

    LocalTimeBaseValue = ((Temp2_s32 & 0xffff) << 16 |
                          (Temp1_s32 & 0xffff));

    /* Calculate top bit with carry from previous stage */
    LocalTimeBaseBit32 = (((S32)ExtSTCA.BaseBit32 +
                           (S32)(ExtSTCB.BaseBit32) +
                           Carry ) & 1);

    /* Update control values */
    ReturnExtSTC_p->BaseValue = LocalTimeBaseValue;
    ReturnExtSTC_p->BaseBit32 = LocalTimeBaseBit32;
    ReturnExtSTC_p->Extension = LocalTimeExtension;

}

/****************************************************************************
Name         : SubtractTwoExtSTCValues

Description  : Adds two given ExtSTC values.
               ReturnExtSTC_p = ExtSTCB - ExtSTCA

Parameters   : ExtSTCA - First Extended STC
               ExtSTCB - Second Extended STC
               *ReturnExtSTC_p - Pointer to return storage location

Return value: None.
****************************************************************************/
static void SubtractTwoExtSTCValues( STCLKRV_ExtendedSTC_t ExtSTCA,
                         STCLKRV_ExtendedSTC_t ExtSTCB,
                         STCLKRV_ExtendedSTC_t *ReturnExtSTC_p)
{

    if( ExtSTCB.BaseValue >= ExtSTCA.BaseValue )
    {
        ReturnExtSTC_p->BaseValue = ExtSTCB.BaseValue - ExtSTCA.BaseValue;
        ReturnExtSTC_p->BaseBit32 = ExtSTCB.BaseBit32;
        ReturnExtSTC_p->Extension = ExtSTCB.Extension;
    }
    else
    {
        ReturnExtSTC_p->BaseValue = ExtSTCB.BaseValue - ExtSTCA.BaseValue;
        /* Toggle the Bit32 Wrap around */
        ReturnExtSTC_p->BaseBit32 = (((ExtSTCB.BaseBit32)^0x01) & 0x01);
        ReturnExtSTC_p->Extension = ExtSTCB.Extension;
    }
}


/****************************************************************************
Name         : ConvertMicroSecsToExtSTC

Description  : Converts a value from micro second time to Extended STC format

Parameters   : MicroSecVal from timer
               ExtSTC_p pointer to Extended STC type return location

Return value: True if error occured.
****************************************************************************/
#if defined (ST_5100)
static BOOL  ConvertMicroSecsToExtSTC(clock_t MicroSecVal, STCLKRV_ExtendedSTC_t *ExtSTC_p)
{
    S32 Temp1_s32, Temp2_s32;
    S32 Carry;
    U32 LocalTimeBaseBit32;
    U32 LocalTimeBaseValue;
    U32 LocalTimeExtension;
    U32 ConvVal;

    /* Input parameters ok? */
    if ((MicroSecVal == 0) || (ExtSTC_p == NULL))
    {
        return TRUE;
    }

    /* Convert value to 27Mhz quantity */
    ConvVal = (MicroSecVal * 27);


    /* 27Mhz component first */
    Temp1_s32 = (S32)(ConvVal % CONV90K_TO_27M);

    /* Test for over/underflow */
    if( Temp1_s32 > CONV90K_TO_27M )
    {
        Carry = 1;                     /* Overflow */
        Temp1_s32 -= CONV90K_TO_27M;
    }
    else if( Temp1_s32 < 0 )
    {
        Carry = -1;                    /* Underflow */
        Temp1_s32 += CONV90K_TO_27M;
    }
    else
    {
        Carry = 0;
    }
    LocalTimeExtension = (U32)Temp1_s32;


    /* Calculate 90KHz, 32 bit value in two stages
       in order to allow for overflow Carry/Borrow
       is brought forward from previous stage */
    Temp1_s32 = (S32)(((ConvVal / CONV90K_TO_27M) & 0xffff) +
                      Carry);

    Carry = ( Temp1_s32 > 0x0000ffffL ) ? 1 : 0;
    Carry = ( Temp1_s32 < 0 ) ? -1 : Carry;

    /* High 16 bits of BaseValue + carry from low 16 */
    Temp2_s32 = (S32)((((ConvVal / CONV90K_TO_27M) >> 16) & 0xffff ) +
                 Carry);

    Carry = ( Temp2_s32 > 0x0000ffffL ) ? 1 : 0;
    Carry = ( Temp2_s32 < 0 ) ? -1 : Carry;

    LocalTimeBaseValue = ((Temp2_s32 & 0xffff) << 16 |
                            (Temp1_s32 & 0xffff));

    /* Calculate top bit with carry from previous stage */
    LocalTimeBaseBit32 = (Carry & 1);

    /* Update Arrival time reference values */
    ExtSTC_p->BaseValue = LocalTimeBaseValue;
    ExtSTC_p->BaseBit32 = LocalTimeBaseBit32;
    ExtSTC_p->Extension = LocalTimeExtension;

    return FALSE;

}
#endif

#endif  /* STCLKRV_NO_PTI */
/****************************************************************************
Name         : IsClockValid()

Description  : Checks if the clock specified is valid for a device or not

Parameters   : ClockSource

Return Value : TRUE  if clock is valid
               FALSE if clock is invalid
*****************************************************************************/
static BOOL IsClockValid(STCLKRV_ControlBlock_t *CB_p, STCLKRV_ClockSource_t ClockSource )
{

#if defined(ST_7100) || defined (ST_7109)

    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_PCM_1) ||
        (ClockSource == STCLKRV_CLOCK_SPDIF_0) ||
        (ClockSource == STCLKRV_CLOCK_HD_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }

#elif defined ST_7710
    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_SPDIF_0) ||
        (ClockSource == STCLKRV_CLOCK_HD_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#elif defined (ST_5525)

    /* all possible clocks */

    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_PCM_1) ||
        (ClockSource == STCLKRV_CLOCK_PCM_2) ||
        (ClockSource == STCLKRV_CLOCK_PCM_3) ||
        (ClockSource == STCLKRV_CLOCK_SPDIF_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#elif defined (ST_7200)

    /* all possible clocks */
    if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_PRIMARY)
    {
        if( (ClockSource == STCLKRV_CLOCK_PCM_0)        ||
            (ClockSource == STCLKRV_CLOCK_PCM_1)        ||
            (ClockSource == STCLKRV_CLOCK_PCM_2)        ||
            (ClockSource == STCLKRV_CLOCK_PCM_3)        ||
            (ClockSource == STCLKRV_CLOCK_SPDIF_HDMI_0) ||
            (ClockSource == STCLKRV_CLOCK_SPDIF_0)      ||
            (ClockSource == STCLKRV_CLOCK_HD_0) )
        {
            return( TRUE );
        }
        else
        {
            return( FALSE );
        }
    }
    else  if (CB_p->InitPars.DecodeType == STCLKRV_DECODE_SECONDARY)
    {
        if( (ClockSource == STCLKRV_CLOCK_PCM_0)        ||
            (ClockSource == STCLKRV_CLOCK_PCM_1)        ||
            (ClockSource == STCLKRV_CLOCK_PCM_2)        ||
            (ClockSource == STCLKRV_CLOCK_PCM_3)        ||
            (ClockSource == STCLKRV_CLOCK_SPDIF_HDMI_0) ||
            (ClockSource == STCLKRV_CLOCK_SPDIF_0)      ||
            (ClockSource == STCLKRV_CLOCK_HD_1) )
        {
            return( TRUE );
        }
        else
        {
            return( FALSE );
        }
    }
    else
    {
        return( FALSE );
    }
#else

    if( (ClockSource == STCLKRV_CLOCK_PCM_0) ||
        (ClockSource == STCLKRV_CLOCK_SPDIF_0))
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
#endif

} /* IsClockValid() */

#ifdef ST_OSLINUX
/****************************************************************************
Name         : clkrv_LinuxMapRegion()

Description  : Memory check and IOREMAP being done on FS addresses to be used


Parameters   : Pointer to STCLKRV_ControlBlock_t

Return Value : ST_NO_ERROR
               ST_ERROR_NO_MEMORY
*****************************************************************************/
static ST_ErrorCode_t clkrv_LinuxMapRegion( STCLKRV_ControlBlock_t *CB_p )
{
    CB_p->FSMappedAddress_p =
                      (unsigned long *)STLINUX_MapRegion( (void *)CB_p->InitPars.FSBaseAddress_p,
                               STCLKRV_FS_REGION_SIZE,"STCLKRV_FS" );
    if (CB_p->FSMappedAddress_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    CB_p->AUDFSMappedAddress_p =
                      (unsigned long *)STLINUX_MapRegion( (void *)CB_p->InitPars.AUDCFGBaseAddress_p,
                                       STCLKRV_AUDFS_REGION_SIZE,"STCLKRV_AUD_FS");
    if(CB_p->AUDFSMappedAddress_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
#elif defined(ST_7710)
    CB_p->AUDFSMappedAddress_p =
                      (unsigned long *)STLINUX_MapRegion( (void *)CB_p->InitPars.ADSCBaseAddress_p,
                                       STCLKRV_AUDFS_REGION_SIZE,"STCLKRV_AUD_FS");
    if(CB_p->AUDFSMappedAddress_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
#endif /* #if defined(ST_7100) || defined(ST_7109) || defined(ST_7200) */
#if defined(ST_7200)
    CB_p->CRUMappedAddress_p =
                      (unsigned long *)STLINUX_MapRegion( (void *)CB_p->InitPars.CRUBaseAddress_p,
                                       STCLKRV_CRU_REGION_SIZE,"STCLKRV_CRU" );

    if (CB_p->CRUMappedAddress_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
#endif /* #if defined(ST_7200) */

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : clkrv_LinuxUnapRegion()

Description  : IOUNMAP and FS memory being released


Parameters   : Pointer to STCLKRV_ControlBlock_t

Return Value : None
*****************************************************************************/
static void clkrv_LinuxUnmapRegion( STCLKRV_ControlBlock_t *CB_p )
{
    if(CB_p->FSMappedAddress_p != NULL)
    {
        STLINUX_UnmapRegion((void *)CB_p->FSMappedAddress_p, STCLKRV_FS_REGION_SIZE);
    }
#if defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    if(CB_p->AUDFSMappedAddress_p != NULL)
    {
        STLINUX_UnmapRegion((void *)CB_p->AUDFSMappedAddress_p, STCLKRV_AUDFS_REGION_SIZE);
    }
#endif
#if defined(ST_7200)
    if(CB_p->CRUMappedAddress_p != NULL)
    {
        STLINUX_UnmapRegion((void *)CB_p->CRUMappedAddress_p, STCLKRV_CRU_REGION_SIZE);
    }
#endif
}
#endif /* #ifdef ST_OSLINUX */


/* End of stclkrv.c */

