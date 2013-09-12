/******************************************************************************\
 *
 * File Name   : stevt.c
 *
 * Description : API functions implementation for Event Handler
 *
 * Copyright STMicroelectronics - 1999 2000
 *
\******************************************************************************/
#if !defined( ST_OSLINUX )
#include <string.h>
#include <stdlib.h>
#include "stlite.h"

#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif

#endif

#include "stos.h"
#include "stevtint.h"
#include "event.h"

/*
******************************************************************************
Private Macros
******************************************************************************
*/
#define EVT_VALID_HANDLE          0xFEDCBA98

/* CHECK_WORD_PTR confirms it is okay to read a word from a given address */
#ifdef OS40

/* avoid being halted by exception if pointer is bad */
#include <mem/chMem.h>
static int MemCheckBuf;
#define CHECK_WORD_PTR(p) \
    (svMemRead((void *)(p),(void *)&MemCheckBuf,sizeof(int)) != K_EFAULT)

#elif defined(OS21)
#define CHECK_WORD_PTR(p) ((p > sizeof(U32))? 1:0)

#else
#define CHECK_WORD_PTR(p) TRUE
  /* ((p) != NULL) - won't help when we're passed &(Unit_p->MagicNumber),
    and the magic number check does all we need */
#endif

#ifdef ARCHITECTURE_ST20
#define IS_VALID_EVT_HANDLE(d)  ((d)?((d)->MagicNumber == EVT_VALID_HANDLE):0)
#elif defined(ARCHITECTURE_ST40) && (defined(ST_OS21) || defined(ST_OSWINCE))
#define IS_VALID_EVT_HANDLE(d)  ((d)?((d)->MagicNumber == EVT_VALID_HANDLE):0)
#elif defined(OS40)
#define IS_VALID_EVT_HANDLE(d)  (CHECK_WORD_PTR(d)?((d)->MagicNumber == EVT_VALID_HANDLE):0)
#elif defined(ST_OSLINUX)
#define IS_VALID_EVT_HANDLE(d)      ((d)?((d)->MagicNumber == EVT_VALID_HANDLE):0)
#elif defined(ARCHITECTURE_ST200) && defined(ST_OS21) /*5525*/
#define IS_VALID_EVT_HANDLE(d)      ((d)?((d)->MagicNumber == EVT_VALID_HANDLE):0)
#else
#error OS/ARCHITECTURE NOT RECOGNIZED !!!
#endif

/*  MORE_SPACE is the number of new entries in the dictionary
 *  database */
#define MORE_SPACE 5



/*  Dictionary database entry */
struct EHDevice_s
{
    ST_DeviceName_t    EHName;
    EventHandler_t    *EH;
    ST_Partition_t    *MemoryPartition;
};

typedef struct EHDevice_s EHDevice_t;

/*  EH Dictionary. Shouldn't rely on compiler initialising, really. */
static EHDevice_t *EHNameDict = NULL;

/*  Number of initialized EH */
static U32 EHNameDictNum = 0;

/*  Number of Dictionary Elements */
static U32 EHNameDictSize = 0;

static EHDevice_t *GetDeviceNamed(const ST_DeviceName_t EventHandlerName);

/* To ensure parts of STEVT Init are atomic */

#if defined(ST_OS21) || defined(ST_OSWINCE)
static semaphore_t *InitSemaphore_p;
#elif defined ST_OSLINUX
static semaphore_t *InitSemaphore_p = NULL;
#else
static semaphore_t InitSemaphore;
static semaphore_t *InitSemaphore_p = NULL;
#endif /* ST_OS21 || ST_OSWINCE*/

ST_ErrorCode_t STEVT_Init (const ST_DeviceName_t EventHandlerName,
                           const STEVT_InitParams_t *InitParams)
{
    U32             MemAddr,MemSize;
    EventHandler_t *EHPtr;
#if !defined(STEVT_NO_PARAMETER_CHECK)
    EHDevice_t     *EHDevice;
#endif
    MemHandle_t    *MemHandle;
    ST_ErrorCode_t  Error;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Init()"));

#if !defined(STEVT_NO_PARAMETER_CHECK)
    /*
     * Check if an Event Handler as been already initialized with this name
     */
    EHDevice = GetDeviceNamed ( EventHandlerName );
    if (EHDevice != NULL )
    {

        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Named Event Handler has been already initialized"))
        return (ST_ERROR_ALREADY_INITIALIZED);
    }

    /*
     * Check initialization parameters
     */
    if (InitParams == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Bad Initialization Parameters"))
        return (ST_ERROR_BAD_PARAMETER);
    }

    if (InitParams->EventMaxNum == 0)
        return ST_ERROR_BAD_PARAMETER;

    if (InitParams->SubscrMaxNum == 0)
        return ST_ERROR_BAD_PARAMETER;

    if (InitParams->ConnectMaxNum == 0)
        return ST_ERROR_BAD_PARAMETER;
#endif

    /* Make sure the semaphore has been created */
#if !(defined(ST_OSLINUX) && defined(MODULE))
    task_lock();
#endif
    if(InitSemaphore_p == NULL)
    {

#if defined(ST_OS21) || defined(ST_OSLINUX) || defined(ST_OSWINCE)
        InitSemaphore_p = STOS_SemaphoreCreateFifo(NULL ,1);
#else
        InitSemaphore_p = &InitSemaphore;
        semaphore_init_fifo( InitSemaphore_p,1);
#endif
    }
#if !(defined(ST_OSLINUX) && defined(MODULE))
    task_unlock();
#endif

    /* Enter criticial region */
    STOS_SemaphoreWait(InitSemaphore_p);

    /* no more slots available to initialize a new Event Handler */
    if (EHNameDictNum == EHNameDictSize)
    {
        EHNameDictSize += MORE_SPACE;
        if (EHNameDictNum == 0)
        {
            /*  First time a block of memory is allocated from partition */
            EHNameDict = (EHDevice_t *)STOS_MemoryAllocate(InitParams->MemoryPartition,
                                                       sizeof(EHDevice_t)*EHNameDictSize);
        }
        else
        {
#ifdef ST_OSLINUX
            EHNameDict = (EHDevice_t *)STOS_MemoryReallocate(InitParams->MemoryPartition,
                                                         EHNameDict,
                                                         sizeof(EHDevice_t)*EHNameDictSize,
                                                         sizeof(EHDevice_t)*(EHNameDictSize-MORE_SPACE));
#else
    /*reallocation of memory block */
    #if defined(ST_OSWINCE)
    		EHDevice_t *temp;
			temp = (EHDevice_t *)memory_allocate(InitParams->MemoryPartition,
                                                       sizeof(EHDevice_t)*EHNameDictSize);
			memcpy(temp,EHNameDict,(sizeof(EHDevice_t)*EHNameDictSize));
			memory_deallocate(InitParams->MemoryPartition,EHNameDict);
			EHNameDict=temp;

    #else
            /* realloc increase the memory allocated for MORE_SPACE entries */
            EHNameDict = (EHDevice_t *)STOS_MemoryReallocate(InitParams->MemoryPartition,
                                                         EHNameDict,
                                                         sizeof(EHDevice_t)*EHNameDictSize,
                                                         sizeof(EHDevice_t)*(EHNameDictSize-MORE_SPACE));
    #endif
#endif
        }
        if (EHNameDict == NULL)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                          "EH Dictionary initialization failed"));

            STOS_SemaphoreSignal(InitSemaphore_p);
            return (ST_ERROR_NO_MEMORY);
        }
    }


    /*
     *  Allocate the Event Handler
     */
    EHPtr = (EventHandler_t *)STOS_MemoryAllocate(InitParams->MemoryPartition,
                                              sizeof(EventHandler_t));
    if (EHPtr == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EH initialization failed: no memory"));

        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_NO_MEMORY);
    }

    /*
     * Allocate memory handler and the memory to be used in the event handler
     */
    MemHandle = (MemHandle_t *)STOS_MemoryAllocate(InitParams->MemoryPartition,
                                               sizeof(MemHandle_t) );
    if (MemHandle == (MemHandle_t *)NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EH memory initialization failed: no memory"));

        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_NO_MEMORY);
    }
    memset((void *)MemHandle, 0, sizeof(MemHandle_t));

    /*
     *  memory size needed for this EH
     *  if not defined in the init params use an heuristic formula
     *  to evaluate it.
     */
    if ((InitParams->MemorySizeFlag == STEVT_USER_DEFINED ) && (InitParams->MemoryPoolSize!=0))
    {
        MemSize = InitParams->MemoryPoolSize;
    }
    else
    {
        MemSize =  stevt_MemSize( InitParams );
    }

    MemAddr = (U32)STOS_MemoryAllocate(InitParams->MemoryPartition, MemSize );
    if (MemAddr == (U32)NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "EH memory initialization failed: no memory"));

        STOS_SemaphoreSignal(InitSemaphore_p);
        return (ST_ERROR_NO_MEMORY);
    }

    /*
     * Initialize the internal memory handler
     */
    Error = stevt_InitMem( MemAddr, MemSize, MemHandle);
    if (Error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Error in internal memory handle"));
        STOS_SemaphoreSignal(InitSemaphore_p);
        return ST_ERROR_NO_MEMORY;
    }

    /*
     * Initialize fields in the Event Handler structure.
     */
    EHPtr->ConnectionList = NULL;
    EHPtr->EventList      = NULL;
    EHPtr->NameList       = NULL;
    EHPtr->EventMaxNum    = InitParams->EventMaxNum;
    EHPtr->SubscrMaxNum   = InitParams->SubscrMaxNum;
    EHPtr->ConnectMaxNum  = InitParams->ConnectMaxNum;
    EHPtr->MemHandle      = MemHandle;
    EHPtr->MemAddr        = MemAddr  ;

    /* Used for ensuring certain things are atomic,
       So create a semaphore for control block. */
#if defined(ST_OS21) || defined(ST_OSWINCE)
    EHPtr->LockSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
#else
    EHPtr->LockSemaphore_p = &EHPtr->LockSemaphore;
    semaphore_init_fifo(EHPtr->LockSemaphore_p,1);
#endif

    /*
     * Set the STEVT Driver and corresponding memory
     * partition in the dictionary slot
     */
    EHNameDict[EHNameDictNum].EH = EHPtr;
    EHNameDict[EHNameDictNum].MemoryPartition = InitParams->MemoryPartition;

    strcpy((EHNameDict[EHNameDictNum].EHName),EventHandlerName);

    EHNameDictNum++;

    STOS_SemaphoreSignal(InitSemaphore_p);
    return ST_NO_ERROR;
}

ST_ErrorCode_t STEVT_Open (const ST_DeviceName_t     EventHandlerName,
                           const STEVT_OpenParams_t *OpenParams,
                           STEVT_Handle_t           *Handle)
{
    EHDevice_t     *EHDevice;
    EventHandler_t *EHPtr;
    Connection_t   *ConnectSlot_p;
    const STEVT_OpenParams_t *OpenParams_p;
    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Open()"));
    /* Searches the referenced event handler by name
     * to check if it has been initialized
     */

    EHDevice = GetDeviceNamed ( EventHandlerName );
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (EHDevice == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Named EH has not been initialized"));
        return ST_ERROR_UNKNOWN_DEVICE;
    }
    if (Handle == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                     "NULL handle pointer"));
        return ST_ERROR_BAD_PARAMETER;
    }
#endif
    OpenParams_p = OpenParams;
    EHPtr = EHDevice->EH;

    /* Make sure we don't get conflicts */
    STOS_SemaphoreWait(EHPtr->LockSemaphore_p);
    /*
     * Add a new user connection
     */
    ConnectSlot_p = stevt_AddConnection( EHPtr );
    if (ConnectSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Maximum number of connections has been reached"))

        STOS_SemaphoreSignal(EHPtr->LockSemaphore_p);

        return (ST_ERROR_NO_FREE_HANDLES);
    }
    /*
     * Validate the handle
     */
    ConnectSlot_p->MagicNumber    = EVT_VALID_HANDLE;
    *Handle = (STEVT_Handle_t) ConnectSlot_p;

    /* Release the control block semaphore */
    STOS_SemaphoreSignal(EHPtr->LockSemaphore_p);

    return ST_NO_ERROR;
}

ST_ErrorCode_t STEVT_Close (STEVT_Handle_t Handle)
{
    UserHandle_t    TheUser;
    EventHandler_t *EHPtr;
    ST_ErrorCode_t  Error;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Close()"));
    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
#endif


    EHPtr = TheUser->EH_p;
    STOS_SemaphoreWait(EHPtr->LockSemaphore_p);

    stevt_Close(TheUser);
    Error = stevt_RemoveConnection( EHPtr, TheUser);
    if (Error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Error removing user 0x%08X",TheUser));
        STOS_SemaphoreSignal(EHPtr->LockSemaphore_p);
        return Error;
    }

    /* Release the control block semaphore */
    STOS_SemaphoreSignal(EHPtr->LockSemaphore_p);

    return ST_NO_ERROR;
}

ST_ErrorCode_t STEVT_Term (const ST_DeviceName_t     EventHandlerName,
                           const STEVT_TermParams_t *TermParams)
{
    EventHandler_t *EHPtr;
    ST_Partition_t *MemoryPartition;
    EHDevice_t     *EHDevice;
    ST_ErrorCode_t  Error=ST_NO_ERROR;
    Connection_t   *Current=NULL, *This=NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Term()"));


    /*
     * Check if an Event Handler as been initialized with this name
     */
    EHDevice = GetDeviceNamed ( EventHandlerName );
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (EHDevice == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "Named EH has not been opened"))
        return (ST_ERROR_UNKNOWN_DEVICE);
    }
    if (TermParams == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                     "NULL termparams"));
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    if (InitSemaphore_p != NULL)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }

    /* Driver name found! */
    EHPtr = EHDevice->EH;
    MemoryPartition = EHDevice->MemoryPartition;

    /*
     *  If ForceTerminate is set all connections
     *  are closed before doing the Termination.
     *  In this way all the register callback of all
     *  events jet registered are called.
     */
    if (TermParams->ForceTerminate == TRUE)
    {
        Current = EHPtr->ConnectionList;

        while (Current != NULL )
        {
            stevt_Close( (UserHandle_t)Current );
            This = Current;
            Current = Current->Next;
            Error = stevt_RemoveConnection( EHPtr, (UserHandle_t)This);
            if (Error != ST_NO_ERROR )
            {
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Error removing user 0x%08X",This));

                if (InitSemaphore_p != NULL)
                {
                    STOS_SemaphoreSignal(InitSemaphore_p);
                }
                return Error;
            }
        }
    }
    if (EHPtr->ConnectionList != NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Still open connections, no term is possible"));
        if (InitSemaphore_p != NULL)
        {
            STOS_SemaphoreSignal(InitSemaphore_p);
        }
        return ST_ERROR_OPEN_HANDLE;
    }

    /* Destroy MemHandle */
    Error = stevt_DeinitMem(EHPtr->MemHandle);
    if (Error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Error in internal memory handle"));
        if (InitSemaphore_p != NULL)
        {
            STOS_SemaphoreSignal(InitSemaphore_p);
        }
        return ST_ERROR_NO_MEMORY;
    }
    STOS_MemoryDeallocate (MemoryPartition, EHPtr->MemHandle);
    STOS_MemoryDeallocate (MemoryPartition, (void *)EHPtr->MemAddr);

    /* Clean the Event Handler memory */
    EHPtr->ConnectionList= NULL;
    EHPtr->EventList     = NULL;
    EHPtr->NameList      = NULL;
    EHPtr->EventMaxNum   = 0;
    EHPtr->SubscrMaxNum  = 0;
    EHPtr->ConnectMaxNum = 0;
    EHPtr->MemHandle     = NULL;
    EHPtr->MemAddr       = 0;

    /* delete the lock semaphore, before releasing memory */
    semaphore_delete(EHPtr->LockSemaphore_p);

    /* Disposes the Event handler */
    STOS_MemoryDeallocate (MemoryPartition, (EventHandler_t *)EHPtr);

    /* Adjust the external Dictionary */
    EHNameDictNum--;
    *EHDevice = EHNameDict[EHNameDictNum];
    EHNameDict[EHNameDictNum].EH     = NULL;
    EHNameDict[EHNameDictNum].MemoryPartition = NULL;
    strcpy((char *)(EHNameDict[EHNameDictNum].EHName), "");

    if(EHNameDictNum == 0)
    {
        STOS_MemoryDeallocate(MemoryPartition, EHNameDict);

        /* Set the dictionary size to 0 as well */
        EHNameDictSize=0;

        /* Point to NULL so that no inadvertent usage occurs */
        EHNameDict = NULL;
    }
    if (InitSemaphore_p != NULL)
    {
        STOS_SemaphoreSignal(InitSemaphore_p);
    }

    return ST_NO_ERROR;
}

#if defined(ST_OSLINUX)
ST_ErrorCode_t STEVT_NotifyWithSize(STEVT_Handle_t Handle,
                             STEVT_EventID_t EventID,
                             const void *EventData,
                             U32 EventDataSize)
#else
ST_ErrorCode_t STEVT_Notify(STEVT_Handle_t Handle,
                             STEVT_EventID_t EventID,
                             const void *EventData)
#endif
{
    UserHandle_t    TheUser;
    Registrant_t   *RegSlot_p=NULL;
    Event_t        *Event;
    DevName_t      *NameSlot_p=NULL;

#if defined(ST_OSLINUX)
    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Notify()"));
#else
    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_NotifyWithSize()"));
#endif

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    /*
     * Check if the EventID is associated to an event registered by TheUser
     */
    RegSlot_p = stevt_CheckID( EventID, TheUser);
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (RegSlot_p == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Event ID"));
        return STEVT_ERROR_INVALID_EVENT_ID;
    }
#endif
    Event          = RegSlot_p->Event;
    NameSlot_p     = RegSlot_p->RegName;

    /* For Locking */
    if (INSIDE_TASK())
    {
        if((Event->ReadCnt == 0) || (Event->WriteCnt > 0))
        {
            STOS_SemaphoreWait(Event->WriteSemaphore_p);
            interrupt_lock();
            Event->ReadCnt++;
            interrupt_unlock();
        }
        else
        {
            interrupt_lock();
            Event->ReadCnt++;
            interrupt_unlock();
        }
    }
    Event->NotifyCnt++;

    if (NULL != NameSlot_p)
    {
        /*
         * Notify all the user subscribed to the pair Event/NameSlot_p
         * and all the anonymous subscribers for backward compatibility
         */

#if defined(ST_OSLINUX)
        stevt_CallBothNotifyCB( Event, NameSlot_p, EventData, EventDataSize);
#else
        stevt_CallBothNotifyCB( Event, NameSlot_p, EventData);
#endif
    }
    else
    {
        /*
         * Notify only the anonymous subscribers
         */
#if defined(ST_OSLINUX)
        stevt_CallNotifyCB( Event, EventData, EventDataSize);
#else
        stevt_CallNotifyCB( Event, EventData);
#endif
    }

    /* For Locking */
    if (INSIDE_TASK())
    {
        interrupt_lock();
        Event->ReadCnt--;
        interrupt_unlock();

        if(Event->ReadCnt == 0)
        {
            STOS_SemaphoreSignal(Event->WriteSemaphore_p);

        }
    }

    return ST_NO_ERROR;

}

#if defined(ST_OSLINUX)
ST_ErrorCode_t STEVT_NotifySubscriberWithSize(STEVT_Handle_t Handle,
                                       STEVT_EventID_t EventID,
                                       const void *EventData,
                                       STEVT_SubscriberID_t SubscriberID,
                                       U32 EventDataSize)
#else
ST_ErrorCode_t STEVT_NotifySubscriber (STEVT_Handle_t Handle,
                                       STEVT_EventID_t EventID,
                                       const void *EventData,
                                       STEVT_SubscriberID_t SubscriberID)
#endif
{
    UserHandle_t    TheUser, Subscriber;
    EventHandler_t *EHPtr;
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    Registrant_t   *RegSlot_p=NULL;
    DevName_t      *RegName_p;
    Subscription_t *SubsSlot_p;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_NotifySubscriber()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    EHPtr = TheUser->EH_p;

    RegSlot_p = stevt_CheckID( EventID, TheUser);
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (RegSlot_p == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Event ID"));
        return STEVT_ERROR_INVALID_EVENT_ID;
    }
#endif

    Subscriber = stevt_Decrypt( EHPtr, SubscriberID);
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(Subscriber))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Subscriber Handle = 0x%08X",
                      Subscriber));
        return STEVT_ERROR_INVALID_SUBSCRIBER_ID;
    }
#endif

    /* For GNBvd12227(SubID List seems superflous) */
    SubsSlot_p = stevt_SearchSubscriberID(Subscriber,RegSlot_p->Event,RegSlot_p->RegName);

    if (SubsSlot_p == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Subscriber Handle = 0x%08X",
                      Subscriber));
        return STEVT_ERROR_NOT_SUBSCRIBED;
    }
    RegSlot_p->Event->NotifyCnt++;

    if (SubsSlot_p->MultiInstance == TRUE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Calling Notify.DevCallback"));
        RegName_p = RegSlot_p->RegName;
#if defined(ST_OSLINUX)
        if( SubsSlot_p->WithSize ){
            ((STEVT_DeviceCallbackProcWithSize_t)SubsSlot_p->Notify.DevCallback)(CALL_REASON_NOTIFY_CALL,
                                      RegName_p == NULL ? NULL : RegName_p->Name,
                                      RegSlot_p->Event->EventConst,
                                      EventData,
                                      SubsSlot_p->SubscrData_p,
                                      EventDataSize);
        }
        else{
            SubsSlot_p->Notify.DevCallback(CALL_REASON_NOTIFY_CALL,
                                      RegName_p == NULL ? NULL : RegName_p->Name,
                                      RegSlot_p->Event->EventConst,
                                      EventData,
                                      SubsSlot_p->SubscrData_p);
        }

#else
            SubsSlot_p->Notify.DevCallback(CALL_REASON_NOTIFY_CALL,
                                      RegName_p == NULL ? NULL : RegName_p->Name,
                                      RegSlot_p->Event->EventConst,
                                      EventData,
                                      SubsSlot_p->SubscrData_p);
#endif
    }
    else
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Calling Notify.Callback"));
        SubsSlot_p->Notify.Callback(CALL_REASON_NOTIFY_CALL,
                                   RegSlot_p->Event->EventConst,
                                   EventData);
    }

    return Error;
}

ST_ErrorCode_t STEVT_RegisterInt (STEVT_Handle_t        Handle,
                               STEVT_EventConstant_t EventConst,
                               STEVT_EventID_t      *EventID,
			     const  char		    *EventName)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Register()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)

    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (EventID == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "NULL EventID"));
        return ST_ERROR_BAD_PARAMETER;
    }

    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }

#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));

    Error = stevt_EventRegister ( EventConst, TheUser, NULL, EventID,
		EventName);

    if (Error != ST_NO_ERROR)
    {
        *EventID = (STEVT_EventID_t)NULL;
        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return Error;
    }

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));

    return Error;
}

ST_ErrorCode_t STEVT_RegisterDeviceEventInt  (STEVT_Handle_t Handle,
                                           const ST_DeviceName_t RegistrantName,
                                           STEVT_EventConstant_t EventConst,
                                           STEVT_EventID_t *EventID,
					const char *EventName)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_RegisterDeviceEvent()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
    if (EventID == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "NULL EventID"));
        return ST_ERROR_BAD_PARAMETER;
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));

    if (RegistrantName == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Null device name"));
    }
    Error = stevt_EventRegister ( EventConst, TheUser, RegistrantName,
			EventID, EventName);
    if (Error != ST_NO_ERROR)
    {
        *EventID = (STEVT_EventID_t)NULL;
        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return Error;
    }

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
    return Error;
}

#if 1 /*yxl 2008-09-17 add below section*/
ST_ErrorCode_t STEVT_RegisterDeviceEvent  (STEVT_Handle_t Handle,
                                           const ST_DeviceName_t RegistrantName,
                                           STEVT_EventConstant_t EventConst,
                                           STEVT_EventID_t *EventID)
{

	STEVT_RegisterDeviceEventInt(Handle,
		RegistrantName,
		EventConst,
		EventID,(char*)EventConst);

}
#endif /*yxl 2008-09-17 add below section*/



ST_ErrorCode_t STEVT_Unregister (STEVT_Handle_t Handle,
                                 STEVT_EventConstant_t Event)
{
    UserHandle_t    TheUser;
    EventHandler_t *EH;
    Event_t        *EventSlot_p;
    ST_ErrorCode_t  Error;
    BOOL            RemovedEvent;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Unregister()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));
    EH = TheUser->EH_p;

    /* Searches the event */
    EventSlot_p = stevt_SearchEvent( EH, Event );
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (EventSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "The Event is not in the Event List"))

        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return (STEVT_ERROR_INVALID_EVENT_NAME);
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    /*
     * Search if TheUser is a registrant of this event,
     * if found, remove it and sent unregister callbacks
     */
    Error = stevt_EventUnregister(EventSlot_p, TheUser, NULL);
    if (Error != ST_NO_ERROR )
    {
        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return Error;
    }

    /*
     * If no more referenced (both subscribers and
     * registrants) remove the event.
     */
    Error = stevt_RemoveEvent(EH, Event,&RemovedEvent);

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
    return Error;
}

/*
 * Only keeped for simmetry, but useless because only one handle can register a
 * given Event/Name combination. Directly calls STEVT_Unregister()
 */
ST_ErrorCode_t STEVT_UnregisterDeviceEvent (STEVT_Handle_t Handle,
                                           const ST_DeviceName_t RegistrantName,
                                           STEVT_EventConstant_t Event)
{
    UserHandle_t    TheUser;
    EventHandler_t *EH;
    Event_t        *EventSlot_p;
    ST_ErrorCode_t  Error;
    BOOL            RemovedEvent;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Unregister()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));
    EH = TheUser->EH_p;

    /* Searches the event */
    EventSlot_p = stevt_SearchEvent( EH, Event );
#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (EventSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                      "The Event is not in the Event List"))

        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return (STEVT_ERROR_INVALID_EVENT_NAME);
    }
    if (RegistrantName == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Null device name"));
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    /*
     * Search if TheUser is a registrant of this event,
     * if found, remove it and sent unregister callbacks
     */
    Error = stevt_EventUnregister(EventSlot_p, TheUser, RegistrantName);
    if (Error != ST_NO_ERROR )
    {
        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return Error;
    }

    /*
     * If no more referenced (both subscribers and
     * registrants) remove the event.
     */
    Error = stevt_RemoveEvent(EH, Event,&RemovedEvent);

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
    return Error;

    /*ST_ErrorCode_t  Error;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_UnregisterDeviceEvent()"));
    Error = STEVT_Unregister( Handle, Event );
    return Error;*/
}

ST_ErrorCode_t STEVT_Subscribe (STEVT_Handle_t Handle,
                                STEVT_EventConstant_t EventConst,
                                const STEVT_SubscribeParams_t *SubscribeParams)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Subscribe()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
    if (SubscribeParams == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "NULL subscribeparams"));
        return ST_ERROR_BAD_PARAMETER;
    }
    if (SubscribeParams->NotifyCallback == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Missing NotifyCallback"));
        return STEVT_ERROR_MISSING_NOTIFY_CALLBACK;
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    STOS_SemaphoreWait(TheUser->EH_p->LockSemaphore_p);

#if defined(ST_OSLINUX) && defined(MODULE)
    Error = stevt_EventSubscribe( EventConst,
                        (const STEVT_DeviceSubscribeParams_t *)SubscribeParams,
                        TheUser,
                        NULL,
                        FALSE,
                        FALSE); /* Do not call callback with size. */
#else
    Error = stevt_EventSubscribe( EventConst,
                        (const STEVT_DeviceSubscribeParams_t *)SubscribeParams,
                        TheUser,
                        NULL,
                        FALSE);
#endif
    if (Error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to subscribe event"));
        STOS_SemaphoreSignal(TheUser->EH_p->LockSemaphore_p);
        return Error;
    }

    STOS_SemaphoreSignal(TheUser->EH_p->LockSemaphore_p);
    return Error;
}

#if defined (ST_OSLINUX) && defined(MODULE)
/* This function duplicates STEVT_SubscribeDeviceEvent() and instructs evt to
   call the "WithSize" callback with the size of the event data.
   This is required for the linux stevt ioctl driver.

   This function should only be called from kernel space.
*/

ST_ErrorCode_t STEVT_SubscribeDeviceEventWithSize(STEVT_Handle_t Handle,
                                           const ST_DeviceName_t RegistrantName,
                                           STEVT_EventConstant_t EventConst,
                                           const STEVT_DeviceSubscribeParamsWithSize_t *SubscribeParams)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_SubscribeDeviceEventReqSize()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
    if (SubscribeParams == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "NULL subscribeparams"));
        return ST_ERROR_BAD_PARAMETER;
    }
    if (SubscribeParams->NotifyCallback == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Missing NotifyCallback"));
        return STEVT_ERROR_MISSING_NOTIFY_CALLBACK;
    }

    if (RegistrantName == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Null device name"));
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));

#if defined(ST_OSLINUX) && defined(MODULE)
    Error = stevt_EventSubscribe( EventConst,
                        (const STEVT_DeviceSubscribeParams_t *)SubscribeParams,
                        TheUser,
                        (char*) RegistrantName,
                        TRUE,
                        TRUE); /* Call callback with event data size. */
#else
    Error = stevt_EventSubscribe( EventConst,
                        (const STEVT_DeviceSubscribeParams_t *)SubscribeParams,
                        TheUser,
                        (char*) RegistrantName,
                        TRUE);
#endif

    if (Error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to subscribe event"));

        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));

        return Error;
    }

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
    return Error;
}

#endif

ST_ErrorCode_t STEVT_SubscribeDeviceEvent (STEVT_Handle_t Handle,
                                           const ST_DeviceName_t RegistrantName,
                                           STEVT_EventConstant_t EventConst,
                                           const STEVT_DeviceSubscribeParams_t *SubscribeParams)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_SubscribeDeviceEvent()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
    if (SubscribeParams == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "NULL subscribeparams"));
        return ST_ERROR_BAD_PARAMETER;
    }
    if (SubscribeParams->NotifyCallback == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Missing NotifyCallback"));
        return STEVT_ERROR_MISSING_NOTIFY_CALLBACK;
    }

    if (RegistrantName == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Null device name"));
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));

#if defined(ST_OSLINUX) && defined(MODULE)
    Error = stevt_EventSubscribe( EventConst,
                        SubscribeParams,
                        TheUser,
                        (char*) RegistrantName,
                        TRUE,
                        FALSE); /* Do not call callback with size. */
#else
    Error = stevt_EventSubscribe( EventConst,
                        SubscribeParams,
                        TheUser,
                        RegistrantName,
                        TRUE);
#endif
    if (Error != ST_NO_ERROR )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Unable to subscribe event"));

        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));

        return Error;
    }

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
    return Error;
}

ST_ErrorCode_t STEVT_Unsubscribe (STEVT_Handle_t Handle,
                                  STEVT_EventConstant_t EventConst)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_Unsubscribe()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));

    Error = stevt_EventUnsubscribe( EventConst, TheUser, NULL);
    if (Error != ST_NO_ERROR )
    {

        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));

        return Error;
    }

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));

    return ST_NO_ERROR;
}

ST_ErrorCode_t STEVT_UnsubscribeDeviceEvent (STEVT_Handle_t Handle,
                                             const ST_DeviceName_t RegistrantName,
                                             STEVT_EventConstant_t EventConst)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_UnsubscribeDeviceEvent()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }

    if (RegistrantName == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Null device name"));
    }
    if (!INSIDE_TASK())
    {
        return STEVT_ERROR_INTERRUPT_HANDLER;
    }
#endif

    STOS_SemaphoreWait((TheUser->EH_p->LockSemaphore_p));

    Error = stevt_EventUnsubscribe( EventConst, TheUser, RegistrantName);
    if (Error != ST_NO_ERROR )
    {
        STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
        return Error;
    }

    STOS_SemaphoreSignal((TheUser->EH_p->LockSemaphore_p));
    return ST_NO_ERROR;
}

ST_ErrorCode_t STEVT_GetSubscriberID (STEVT_Handle_t Handle,
                                      STEVT_SubscriberID_t *SubscriberID)
{
    UserHandle_t    TheUser;
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_GetSubscriberID()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
    if (SubscriberID == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR, "NULL subscriber ID"));
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    *SubscriberID = TheUser->ID;

    return Error;
}

ST_Revision_t STEVT_GetRevision(void)
{
    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_GetRevision()"));

    return STEVT_REVISION;
}

U32 STEVT_GetAllocatedMemory(STEVT_Handle_t Handle)
{
    UserHandle_t    TheUser;
    EventHandler_t *EHPtr;
    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"STEVT_GetAllocatedMemory()"));

    TheUser = (UserHandle_t)Handle;

#if !defined(STEVT_NO_PARAMETER_CHECK)
    if (! IS_VALID_EVT_HANDLE(TheUser))
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Invalid Handle : TheUser = 0x%08X",TheUser));
        return ST_ERROR_INVALID_HANDLE;
    }
#endif

    EHPtr = TheUser->EH_p;
    return (EHPtr->MemHandle->AllocatedMemory+32);
}

static EHDevice_t *GetDeviceNamed(const ST_DeviceName_t EventHandlerName)
{
    U32 i;
    EHDevice_t *EventHandler_p = NULL;

    /* Searches the referenced event handler */
    for (i = 0; i < EHNameDictNum; i++)
    {
        if (strcmp(EHNameDict[i].EHName, EventHandlerName) == 0)
        {
            EventHandler_p = &(EHNameDict[i]);
            break;
        }
    }

    return EventHandler_p;
}

#ifdef ST_OSLINUX
Event_t *stevt_FindEvent(STEVT_EventConstant_t EventConst)
{
	Event_t		*Event = NULL;
	int		DictNum;

	for (DictNum=0; (DictNum<EHNameDictNum) && !Event; DictNum++)
		Event = stevt_SearchEvent(EHNameDict[DictNum].EH, EventConst);
	return(Event);
}
EXPORT_SYMBOL(stevt_FindEvent);

/*
 * stevt_dump_state: dump some status info to buf, which is count bytes.
 * In Linux, count is 1K.
 */
int stevt_dump_state(char *buf, int count)
{
	int len = 0;

	len += sprintf(buf+len,"Dictionary size    = %d\n", EHNameDictSize);
	len += sprintf(buf+len,"Initialized handles = %d\n", EHNameDictNum);

	return(len);
}

/*
 * stevt_dump_events: dump event details into buf, which is count bytes.
 * In Linux, count initially is 1K but could be less on subsequent calls.
 * This function will get called repeatedly to gather date until it sets eof
 * (or until it returns length 0 twice in a row). It keeps track of where
 * it left off last time.
 * The offset parameter is 0 when we want to start reading anew.
 */
int stevt_dump_events(char *buf, int count, int offset, int *eof)
{
	int			len = 0;
	static int		DictNum = 0;	/* where we left off */
	static Event_t		*Event = NULL;	/* where we left off */

	if (offset == 0) {	/* First call */
		DictNum = 0;
		Event = NULL;
	}
	if (count < 80) return(len);		/* Not enough room */

/*printk(KERN_INFO "%s: %d %d 0x%lx %d\n", __FUNCTION__, (int)offset, DictNum, (long)Event, count);*/

	if (DictNum < EHNameDictNum)
	{
		if (Event == NULL)	/* New event handler */
		{
			len += sprintf(buf+len,"\nEvent handler %d = %s\n",
					DictNum, EHNameDict[DictNum].EHName);
			len += sprintf(buf+len,"\n%-42sEvent Constant  Trig Oflow Subscr\n", "EventName");
			len += sprintf(buf+len,"===========================================================================\n");
			Event = EHNameDict[DictNum].EH->EventList;
		}

		/* Fill the buffer as much as possible. We need 1 line for
		 * the event plus optionally an unknown number of subscriber
		 * names.
		 * Lets hope those do not go over 1 line, or they will get
		 * dropped silently.
		 */
		while ((Event) && ((len+160) < count))
		{
			len += stevt_dump_event(Event, &buf[len], count-len);
			Event = Event->Next;
		}
		if (Event == NULL)
			DictNum++;
	}
	else
		*eof = 1;

/*printk(KERN_INFO "%s: %d bytes, next 0x%lx\n", __FUNCTION__, len, (long)Event);*/
	return(len);
}
#endif

