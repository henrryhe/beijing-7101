/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_evt.h
 Description: events for stpti

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stcommon.h"
#include "stevt.h"
#include "sttbx.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_mem.h"

#include "memget.h"


/* Public Variables -------------------------------------------------------- */
#if defined(ST_OSLINUX)
#include "pti_linux.h"

    #define EVT_TASK_NAME "stpti4_EvtTask"
    static DECLARE_COMPLETION(pticore_evttask_exited);
#else
#define EVT_TASK_NAME "stpti_EventNotifyTask"
#endif /* #endif defined(ST_OSLINUX) */

extern Driver_t STPTI_Driver;
/* pointer to event task stack GNBvd21068*/
void *EvtTaskStack;


/* Private Function Prototypes --------------------------------------------- */


static void stpti_EventNotifyTask(void);


/* Functions --------------------------------------------------------------- */

#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_EventQueueTerm
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_EventQueueTerm(void)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    EventData_t *event_p;
    FullHandle_t FullNullHandle;
    CLOCK time;


    FullNullHandle.word = STPTI_NullHandle();

    /* Send 'terminate' event to task */
    do
    {
        time = time_plus(time_now(), 10);
#if defined(ST_OSLINUX)
        event_p = (EventData_t *)message_claim_timeout(STPTI_Driver.EventQueue_p, &time);
#else
        event_p = message_claim_timeout(STPTI_Driver.EventQueue_p, &time);
#endif
    }
    while (event_p == NULL);

    event_p->Source = FullNullHandle;
    message_send(STPTI_Driver.EventQueue_p, (void *) event_p);

    /* Wait for task to exit */

    time = time_plus(time_now(), TICKS_PER_SECOND * 10);

    STOS_SemaphoreSignal(stpti_MemoryLock);
#if defined(ST_OSLINUX)
    wait_for_completion(&pticore_evttask_exited);       /* Wait for task to exit */
    STOS_SemaphoreWait(stpti_MemoryLock);

    message_delete_queue( STPTI_Driver.EventQueue_p );  /* delete the queue */
    STPTI_Driver.EventQueue_p = NULL;                   /* Make sure it can't be used */
    STPTI_Driver.EventTaskId_p = NULL;
#else
    if (STOS_TaskWait((task_t **) & STPTI_Driver.EventTaskId_p, &time) != ST_NO_ERROR)
    {
        STOS_SemaphoreWait(stpti_MemoryLock);
        Error = ST_ERROR_TIMEOUT;
    }
    else
    {
        STOS_SemaphoreWait(stpti_MemoryLock);
        
        Error = STOS_TaskDelete ( STPTI_Driver.EventTaskId_p,
                                  STPTI_Driver.MemCtl.Partition_p,
                                  EvtTaskStack,
                                  STPTI_Driver.MemCtl.Partition_p );
        
        if ( Error == ST_NO_ERROR )
        {
            message_delete_queue(STPTI_Driver.EventQueue_p);
            STPTI_Driver.EventQueue_p = NULL;    /* Make sure it can't be used */
            STPTI_Driver.EventTaskId_p = NULL;
	     }
    }
#endif /* #endif defined(ST_OSLINUX) */

    return Error;
}
#endif /* !STPTI_BSL_SUPPORT*/

/******************************************************************************
Function Name : stpti_EventQueueInit
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_EventQueueInit(ST_Partition_t *Partition_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    static tdesc_t TaskDesc;/* task descriptor 21068*/

    STPTI_Driver.EventQueue_p = STOS_MessageQueueCreate( sizeof(EventData_t), EVENT_QUEUE_SIZE );

    if (STPTI_Driver.EventQueue_p == NULL)
    {
        Error = ST_ERROR_NO_MEMORY;
    }
    else
    {
        Error = STOS_TaskCreate( (void (*)(void *))stpti_EventNotifyTask, 
                                 NULL,
                                 Partition_p,
                                 STPTI_Driver.EventTaskStackSize,
                                 &(EvtTaskStack),
                                 Partition_p,
                                 (task_t**)&(STPTI_Driver.EventTaskId_p), 
                                 &TaskDesc,
                                 STPTI_Driver.EventTaskPriority,
#if defined(ST_OSLINUX)
                                 EVT_TASK_NAME,
#else
                                 "stpti_EventNotifyTask",
#endif
                                 0 );
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_EventTerm
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_EventTerm(FullHandle_t DeviceHandle)
{
    ST_ErrorCode_t Error;
    Error = STEVT_Close(stptiMemGet_Device(DeviceHandle)->EventHandle);
    stptiMemGet_Device(DeviceHandle)->EventHandle = 0;
    return( Error );
}


/******************************************************************************
Function Name : stpti_EventOpen
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_EventOpen(FullHandle_t DeviceHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_OpenParams_t STEVT_OpenParams;
    STEVT_Handle_t EventHandle;


    /* Event Handler stuff - assume it has been initialised elsewhere */
    Error = STEVT_Open((void *) stptiMemGet_Device(DeviceHandle)->EventHandlerName, &STEVT_OpenParams, &EventHandle);
    if (Error == ST_NO_ERROR)
    {
        stptiMemGet_Device(DeviceHandle)->EventHandle = EventHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_EventRegister
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_EventRegister(FullHandle_t DeviceHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STEVT_EventConstant_t event;
    STEVT_Handle_t EventHandle;
    ST_DeviceName_t DeviceName;
    STEVT_EventID_t EventID;

    EventHandle = stptiMemGet_Device(DeviceHandle)->EventHandle;
    strncpy ( (char *)DeviceName, (char *)stptiMemGet_Device(DeviceHandle)->DeviceName, sizeof (ST_DeviceName_t) ); 

    /* Register all potential events */

    for (event = STPTI_EVENT_BASE; event < STPTI_EVENT_END; ++event)
    {
        Error = STEVT_RegisterDeviceEvent(EventHandle, DeviceName, event, &EventID);

        if (Error != ST_NO_ERROR)
            break;

        stptiMemGet_Device(DeviceHandle)->EventID[event - STPTI_EVENT_BASE] = EventID;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_QueueEvent
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_QueueEvent(FullHandle_t Handle, STEVT_EventConstant_t Event, STPTI_EventData_t * EventData)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
        
    STPTI_CRITICAL_SECTION_BEGIN {

        Error = stpti_DirectQueueEvent(Handle, Event, EventData);

    } STPTI_CRITICAL_SECTION_END;

    return Error;
}


/******************************************************************************
Function Name : stpti_DirectQueueEvent
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DirectQueueEvent(FullHandle_t Handle, STEVT_EventConstant_t Event, STPTI_EventData_t * EventData)
{
    EventData_t *event_p;

#if defined(ST_OSLINUX)
    event_p = (EventData_t *)message_claim_timeout(STPTI_Driver.EventQueue_p, TIMEOUT_IMMEDIATE);    
#else
    event_p = message_claim_timeout(STPTI_Driver.EventQueue_p, TIMEOUT_IMMEDIATE);
#endif
    if (event_p == NULL)
    {
        return ST_ERROR_NO_MEMORY;
    }
    else
    {
        event_p->Source = Handle;
        event_p->Event = Event;
        memcpy( (void *)&event_p->EventData, EventData, sizeof(STPTI_EventData_t) );

        message_send(STPTI_Driver.EventQueue_p, (void *) event_p);
    }

    return ST_NO_ERROR;
}

/******************************************************************************
Function Name : stpti_EventNotifyTask
  Description :
   Parameters :
******************************************************************************/
static void stpti_EventNotifyTask(void)
{
    EventData_t     *event_p;
    FullHandle_t     Handle;
    message_queue_t *EventQueue_p = STPTI_Driver.EventQueue_p;
    Slot_t          *SlotStructure;
    FullHandle_t     FullSlotHandle;

    STTBX_Print(("Event: stpti_EventNotifyTask started\n"));
    STOS_SemaphoreWait(stpti_MemoryLock);
    
    while (TRUE)
    {
        
        STOS_SemaphoreSignal(stpti_MemoryLock);
        
        event_p = message_receive(EventQueue_p);
        Handle = event_p->Source;

        if (Handle.word == STPTI_NullHandle())
        {
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            STTBX_Print(("\tEvent: Terminating the handler\n"));
#endif
            message_release(EventQueue_p, (void *) event_p);
            break;
        }
        
        
        STOS_SemaphoreWait(stpti_MemoryLock);
        
#if !defined ( STPTI_BSL_SUPPORT )
        if (ST_NO_ERROR != stpti_DeviceHandleCheck(Handle))
        {
            /* Bad device - ignore this message */
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
            STTBX_Print(("\tBad Device\n"));
#endif
            message_release(EventQueue_p, (void *) event_p);
            continue;
        }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        switch (event_p->Event)
        {
        case STPTI_EVENT_BUFFER_OVERFLOW_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_BUFFER_OVERFLOW_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

#ifdef STPTI_CAROUSEL_SUPPORT
        case STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_CAROUSEL_CYCLE_COMPLETE_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_CAROUSEL_ENTRY_TIMEOUT_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_CAROUSEL_TIMED_ENTRY_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;
#endif

        case STPTI_EVENT_CC_ERROR_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_CC_ERROR_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_CLEAR_TO_SCRAMBLED_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_DMA_COMPLETE_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_DMA_COMPLETE_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

#ifndef STPTI_NO_INDEX_SUPPORT
        case STPTI_EVENT_INDEX_MATCH_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_INDEX_MATCH_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;
#endif

        case STPTI_EVENT_INTERRUPT_FAIL_EVT:
            {
                STPTI_EventData_t *EventData = ((STPTI_EventData_t *) &event_p->EventData);
                STTBX_Print(("\tEvent: %s - %s  [0X%X:%d]\n", "STPTI_EVENT_INTERRUPT_FAIL_EVT",
                            stptiMemGet_Device(Handle)->DeviceName, EventData->u.ErrEventData.DMANumber, EventData->u.ErrEventData.BytesRemaining));
            }
            break;
            
        case STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_INVALID_DESCRAMBLE_KEY_EVT",
                         stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_INVALID_LINK_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_INVALID_LINK_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_INVALID_PARAMETER_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_INVALID_PARAMETER_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_PACKET_ERROR_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_PACKET_ERROR_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_PCR_RECEIVED_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_PCR_RECEIVED_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_SCRAMBLED_TO_CLEAR_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_SECTIONS_DISCARDED_ON_CRC_CHECK_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        case STPTI_EVENT_TC_CODE_FAULT_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_TC_CODE_FAULT_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;


	case STPTI_EVENT_PES_ERROR_EVT:
            STTBX_Print(("\tEvent: %s - %s\n", "STPTI_EVENT_PES_ERROR_EVT", stptiMemGet_Device(Handle)->DeviceName));
            break;

        default:
            STTBX_Print(("\tEvent: %s(0X%X) - %s\n", "UNKNOWN", event_p->Event, stptiMemGet_Device(Handle)->DeviceName));
            break;
        }

#endif  /* STPTI_INTERNAL_DEBUG_SUPPORT */

        /* if we have a PCR event don't return the slot handle in the event data return the PCRReturnHandle */

        if ( event_p->Event == STPTI_EVENT_PCR_RECEIVED_EVT )
        {
            FullSlotHandle.word = ((STPTI_EventData_t *) &event_p->EventData)->SlotHandle; /* get the slot handle that the event occurred on */            

#if !defined ( STPTI_BSL_SUPPORT )
            if(stpti_SlotHandleCheck(FullSlotHandle) == ST_NO_ERROR)
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
            {
                SlotStructure = stptiMemGet_Slot( FullSlotHandle );                       /* extract the SlotStructure for this slot */

                if(SlotStructure->PCRReturnHandle != STPTI_NullHandle())
                {
                    /* STEVT_SubscriberID_t    SubscriberID; */ 
                    /* set slot handle in event data to PCRReturnHandle from slot structure */
                    ((STPTI_EventData_t *) &event_p->EventData)->SlotHandle = SlotStructure->PCRReturnHandle;                     
                    
                    /* STEVT_GetSubscriberID(stptiMemGet_Device(Handle)->EventHandle, &SubscriberID);                    
                    STEVT_NotifySubscriber(stptiMemGet_Device(Handle)->EventHandle,stptiMemGet_Device(Handle)->EventID[event_p->Event - STPTI_EVENT_BASE],(void *) &event_p->EventData, SubscriberID);  */
                     STOS_SemaphoreSignal( stpti_MemoryLock );

#if defined(ST_OSLINUX)
                     STEVT_NotifyWithSize(   stptiMemGet_Device(Handle)->EventHandle, 
                                             stptiMemGet_Device(Handle)->EventID[event_p->Event - STPTI_EVENT_BASE], 
                                             (void *) &event_p->EventData, sizeof(event_p->EventData));
                     STOS_SemaphoreWait( stpti_MemoryLock );
                     stpti_LinuxSendDeviceEvent( Handle, event_p );
#else
                     STEVT_Notify(  stptiMemGet_Device(Handle)->EventHandle, 
                                    stptiMemGet_Device(Handle)->EventID[event_p->Event - STPTI_EVENT_BASE], 
                                    (void *) &event_p->EventData);
                     STOS_SemaphoreWait( stpti_MemoryLock );
#endif /* #endif defined(ST_OSLINUX) */
                }   
            }
        }
        else
        {         
            /* STEVT_SubscriberID_t    SubscriberID;
            
            STEVT_GetSubscriberID(stptiMemGet_Device(Handle)->EventHandle, &SubscriberID);
            STEVT_NotifySubscriber(stptiMemGet_Device(Handle)->EventHandle, stptiMemGet_Device(Handle)->EventID[event_p->Event - STPTI_EVENT_BASE], (void *) &event_p->EventData, SubscriberID); */ 
            STOS_SemaphoreSignal( stpti_MemoryLock );
#if defined(ST_OSLINUX)
            STEVT_NotifyWithSize(stptiMemGet_Device(Handle)->EventHandle, stptiMemGet_Device(Handle)->EventID[event_p->Event - STPTI_EVENT_BASE], (void *) &event_p->EventData, sizeof(event_p->EventData));  
            STOS_SemaphoreWait( stpti_MemoryLock );
            stpti_LinuxSendDeviceEvent( Handle, event_p );
#else
            STEVT_Notify(stptiMemGet_Device(Handle)->EventHandle, stptiMemGet_Device(Handle)->EventID[event_p->Event - STPTI_EVENT_BASE], (void *) &event_p->EventData);  
            STOS_SemaphoreWait( stpti_MemoryLock );
#endif /* #endif defined(ST_OSLINUX) */
        }

        message_release(EventQueue_p, (void *) event_p);
    }

#if defined(ST_OSLINUX)
    complete_and_exit(&pticore_evttask_exited ,0);
#endif
}



/* --- EOF --- */

