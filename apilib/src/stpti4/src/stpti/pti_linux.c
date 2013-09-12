/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_linux.c
      begin : Tue Apr 5 2005
 Description: STPTI Linux specific interface implementation.

Notes:

******************************************************************************/
#include "stos.h"
#include "pti_linux.h"
#include "memget.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_evt.h"

#include "tchal.h"

/* Returns the result of the test. */
#define STPTI_OSLINUX_EVT_IN_RANGE(e) ( ((e) >= STPTI_EVENT_BASE) && ((e) < STPTI_EVENT_END) )


/* Process the specified event */
/* Get the device (virtual pti supporting this session and send event).*/

ST_ErrorCode_t STPTI_OSLINUX_SendSessionEvent(STPTI_Handle_t Session, EventData_t *evt_event_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;

    FullSessionHandle.word = Session;

    STOS_SemaphoreWait( stpti_MemoryLock );    

    Error = stpti_LinuxSendDeviceEvent(FullSessionHandle,evt_event_p);
    
    STOS_SemaphoreSignal( stpti_MemoryLock );
    
    return Error;
}

/* Process the specified event */
/* Get the device (virtual pti supporting this session and send event).*/
/* Can pass any full handle */
/* Assumes stpti_MemoryLock locked. */
ST_ErrorCode_t stpti_LinuxSendDeviceEvent( FullHandle_t FullHandle, EventData_t *evt_event_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int evt_index = evt_event_p->Event - STPTI_EVENT_BASE;
    STPTI_OSLINUX_evt_struct_t *ptr_p;
    Device_t *dev_p;

    STTBX_Print((KERN_ALERT"pticore: STPTI_OSLINUX_SignalEvent %d\n",evt_event_p->Event));

    if( STPTI_OSLINUX_EVT_IN_RANGE(evt_event_p->Event) ){
        
        dev_p = stptiMemGet_Device( FullHandle );
        
        ptr_p = dev_p->STPTI_Linux_evt_array_p[evt_index];
    
        /* Add to queues and wake all waiting tasks. */
        while(ptr_p){
            STTBX_Print((KERN_ALERT"pticore: Waking queue %x\n",(int)ptr_p));

            /* Add event */
            /* Don't like this because its a copy. But no worse than STEVT. */
            ptr_p->evt_event_p[ptr_p->evt_write_index] = *evt_event_p;
            
            /* Update index */

            ptr_p->evt_write_index++;
            ptr_p->evt_write_index %= ptr_p->evt_queue_size;
    
            /* wake waiting task. */
            wake_up_interruptible(&ptr_p->wait_queue);
            
            ptr_p = ptr_p->next_p;  /* Get next structure. */

        }
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;
 
    return ST_NO_ERROR;
}

/* One event structure is required per task that will wait for events. */
/* This function allocates and initialises the structure. */
/* kmalloc is used for allocation. It is wise to make sure that
   Qsize * 4 is a multiple of 32 bytes.
*/
STPTI_OSLINUX_evt_struct_t *stpti_LinuxAllocateEvtStruct(unsigned int Qsize)
{
    STPTI_OSLINUX_evt_struct_t *evt_p = kmalloc( sizeof( STPTI_OSLINUX_evt_struct_t ), GFP_KERNEL );

    evt_p->evt_queue_size = Qsize;
    evt_p->evt_read_index = 0;
    evt_p->evt_write_index = 0;
    evt_p->next_p = NULL;
    init_waitqueue_head(&evt_p->wait_queue);

    evt_p->evt_event_p = (EventData_t*)kmalloc( sizeof( EventData_t ) * Qsize, GFP_KERNEL );
    if( evt_p->evt_event_p == NULL){
        kfree( evt_p );
        evt_p = NULL;
        STTBX_Print(( "STPTI: Failed to allocate for Event data queue\n"));
    }

    STTBX_Print((KERN_ALERT"pticore: Allocated evt struct 0x%08x\n",(int)evt_p));

    return evt_p;
}

/* This function deallocates the event structure. */
/* All events should be unregistered before this function is called. */
ST_ErrorCode_t stpti_LinuxDeallocateEvtStruct(STPTI_OSLINUX_evt_struct_t *evt_p)
{
    STTBX_Print((KERN_ALERT"pticore: Deallocate evt struct 0x%08x\n",(int)evt_p));

    kfree( (void*)evt_p->evt_event_p );
    kfree( evt_p );

    return ST_NO_ERROR;
}


/* Want an event. */
ST_ErrorCode_t STPTI_OSLINUX_SubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STPTI_OSLINUX_evt_struct_t *old_p;
    int evt_index = Event_ID - STPTI_EVENT_BASE;
    FullHandle_t FullSessionHandle;
    Device_t *dev_p;
    STPTI_OSLINUX_evt_struct_t *evt_p=NULL;
    
    STTBX_Print((KERN_ALERT"pticore: STPTI_OSLINUX_SubscribeEvent %d\n",Event_ID));

    if( STPTI_OSLINUX_EVT_IN_RANGE(Event_ID) ){

        STTBX_Print((KERN_ALERT"pticore: STPTI_OSLINUX_SubscribeEvent %d 0x%08x\n",Event_ID,(int)evt_p));

        STOS_SemaphoreWait( stpti_MemoryLock );

        FullSessionHandle.word = Session;
        dev_p = stptiMemGet_Device( FullSessionHandle );

        evt_p = stptiMemGet_Session( FullSessionHandle )->evt_p;
        
        /* Hook event structure on at the head of the events. */
        old_p = dev_p->STPTI_Linux_evt_array_p[evt_index];
        dev_p->STPTI_Linux_evt_array_p[evt_index] = evt_p;
        evt_p->next_p = old_p;

        STTBX_Print((KERN_ALERT"pticore: STPTI_OSLINUX_SubscribeEvent %d Next: 0x%08x\n",Event_ID,(int)old_p));

        STOS_SemaphoreSignal( stpti_MemoryLock );

    }
    else
        Error = ST_ERROR_BAD_PARAMETER;

    return Error;
}

/* Unregister anm event structure for an event. */
ST_ErrorCode_t STPTI_OSLINUX_UnsubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );

    FullSessionHandle.word = Session;
    Error = stpti_LinuxUnsubscribeEvent(FullSessionHandle, Event_ID);
    
    STOS_SemaphoreSignal( stpti_MemoryLock );

    return Error;
}


/* This allows CDI to wait using poll and select. */
wait_queue_head_t *STPTI_OSLINUX_GetWaitQueueHead(STPTI_Handle_t Session)
{
    FullHandle_t FullSessionHandle;
    Session_t *Session_p;
    
    FullSessionHandle.word = Session;

    STOS_SemaphoreWait( stpti_MemoryLock );

    Session_p = stptiMemGet_Session( FullSessionHandle );
    
    STOS_SemaphoreSignal( stpti_MemoryLock );

    return &Session_p->evt_p->wait_queue;
}

/*=============================================================================

   STPTI_OSLINUX_WaitForEvent()

   Must only be called once per session.
   Cannot have multiple threads waiting on session event.

   Return 0 Good
   return -ve for fail.

  ===========================================================================*/
ST_ErrorCode_t STPTI_OSLINUX_WaitForEvent(STPTI_Handle_t Session, STPTI_Event_t *Event_ID, STPTI_EventData_t *EventData)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;

    FullHandle_t FullSessionHandle;
    STPTI_OSLINUX_evt_struct_t *evt_p;

    FullSessionHandle.word = Session;

    STOS_SemaphoreWait( stpti_MemoryLock );

    evt_p = stptiMemGet_Session( FullSessionHandle )->evt_p;

    STOS_SemaphoreSignal( stpti_MemoryLock );
            
     /* Wait for data */
     while ( evt_p->evt_read_index ==
             evt_p->evt_write_index) {

            /*** Block waiting for data ***/
            if (wait_event_interruptible(evt_p->wait_queue,
                  (evt_p->evt_read_index != evt_p->evt_write_index)))
            {
                /* Call interrupted */
                STTBX_Print((KERN_ALERT"pticore: STPTI_ERROR_SIGNAL_ABORTED\n"));
                Error = STPTI_ERROR_SIGNAL_ABORTED;
                /* The session has probably been aborted.
                   So unregister all events for this session.
                   This is done because the process is terminated
                   without alowing the process to execute its unregistration code.
                   This is only true if a signal handler is not installed. */
                stpti_RemoveSessionFromEvtArray(FullSessionHandle);
                break;
            }
        }

     /* Get event ID */
     *Event_ID = evt_p->evt_event_p[evt_p->evt_read_index].Event;
     *EventData = evt_p->evt_event_p[evt_p->evt_read_index].EventData;
     
     /* Update index */
     evt_p->evt_read_index++;
     evt_p->evt_read_index %= evt_p->evt_queue_size;

     
     STTBX_Print((KERN_ALERT"pticore: Exiting STPTI_OSLINUX_WaitForEvt\n"));
     return (Error);
}

/* Return TRUE if there are any events in the event queue. */
BOOL STPTI_OSLINUX_IsEvent( STPTI_Handle_t Session )
{
    BOOL retval = FALSE;
    FullHandle_t FullSessionHandle;
    STPTI_OSLINUX_evt_struct_t *evt_p;

    FullSessionHandle.word = Session;

    STOS_SemaphoreWait( stpti_MemoryLock );

    evt_p = stptiMemGet_Session( FullSessionHandle )->evt_p;

    if( evt_p->evt_read_index != evt_p->evt_write_index )
        retval = TRUE;

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return retval;
}

/* Return the next event in the queue. */
EventData_t *STPTI_OSLINUX_GetNextEvent(STPTI_Handle_t Session)
{
    FullHandle_t FullSessionHandle;
    STPTI_OSLINUX_evt_struct_t *evt_p;

    FullSessionHandle.word = Session;

    STOS_SemaphoreWait( stpti_MemoryLock );

    evt_p = stptiMemGet_Session( FullSessionHandle )->evt_p;

    /* Update index */
    evt_p->evt_read_index++;
    evt_p->evt_read_index %= evt_p->evt_queue_size;

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return NULL;
}

/******************************************************************************
Function Name : stpti_LinuxEventQueueInit
  Description : Initialise the Linux event structures.
                Assumes stpti_MemoryLock already locked.
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_LinuxEventQueueInit(FullHandle_t DeviceHandle)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    U32 ArraySize = STPTI_OSLINUX_EVT_ARRAY_SIZE;
    Device_t *dev_p;
 
    dev_p = stptiMemGet_Device( DeviceHandle );

    /* Initialise event array */
    while( ArraySize ){
        dev_p->STPTI_Linux_evt_array_p[ArraySize-1] = NULL;
        ArraySize--;
    }

    return Error;
}

/******************************************************************************
Function Name : stpti_LinuxEventTerm
  Description : Assumes stpti_MemoryLock already locked.
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_LinuxEventQueueTerm(FullHandle_t DeviceHandle)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    U32 ArraySize = STPTI_OSLINUX_EVT_ARRAY_SIZE;
    Device_t *dev_p;
 
    dev_p = stptiMemGet_Device( DeviceHandle );
    
    /* Initialise event array */
    while( ArraySize ){
        dev_p->STPTI_Linux_evt_array_p[ArraySize-1] = NULL;
        ArraySize--;
    }
    
    return Error;
}

ST_ErrorCode_t stpti_RemoveSessionFromEvtArray(FullHandle_t FullSessionHandle)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    U32 ArraySize = STPTI_OSLINUX_EVT_ARRAY_SIZE;

    /* Find all references to the event structure and remove it from event array */
    while( ArraySize ){
        stpti_LinuxUnsubscribeEvent(FullSessionHandle, (ArraySize-1) + STPTI_EVENT_BASE );
        ArraySize--;
    }

    return Error;    
}

/* Unsubscribe an event structure for an event. */
ST_ErrorCode_t stpti_LinuxUnsubscribeEvent(FullHandle_t FullSessionHandle, STPTI_Event_t Event_ID)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    int evt_index = Event_ID - STPTI_EVENT_BASE;
    Device_t *dev_p;
    STPTI_OSLINUX_evt_struct_t **ptr_p;
    STPTI_OSLINUX_evt_struct_t *evt_p;

    if( STPTI_OSLINUX_EVT_IN_RANGE(Event_ID) ){

        dev_p = stptiMemGet_Device( FullSessionHandle );

        evt_p = stptiMemGet_Session( FullSessionHandle )->evt_p;

        ptr_p = (STPTI_OSLINUX_evt_struct_t **)&(dev_p->STPTI_Linux_evt_array_p[evt_index]);

        /* Find event structure */
        while(*ptr_p){
            STTBX_Print((KERN_ALERT"pticore: STPTI_OSLINUX_UnregisterEvent %x %x\n",(int)*ptr_p,(int)evt_p));

            if( *ptr_p == evt_p ){
                STTBX_Print((KERN_ALERT"pticore: STPTI_OSLINUX_UnregisterEvent %d - FOUND\n",Event_ID));
                /* Unlink event structure */
                *ptr_p = evt_p->next_p;
            }
            else
                ptr_p = &evt_p->next_p;
        }
    }
    else
        Error = ST_ERROR_BAD_PARAMETER;


    return Error;
}

/* Process the specified event */
/* This puts the event on the event queue. */
/* This is primaraly for debug */
ST_ErrorCode_t STPTI_OSLINUX_QueueSessionEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID, STPTI_EventData_t *EventData)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    
    FullSessionHandle.word = Session;

    Error = stpti_DirectQueueEvent(FullSessionHandle, Event_ID, EventData);

    return Error;
}

/* Implemented to the Linux Architecture document specification */
ST_ErrorCode_t STPTI_WaitEventList( const STPTI_Handle_t SessionHandle,
                                  U32 NbEvents,
                                  U32 *Events_p,
                                  U32 *Event_That_Has_Occurred_p,
                                    U32 EventDataSize, /* Size of Event data pointed to by EventData_p */ 
                                  void *EventData_p )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    int i;
    
    for( i = 0;i < NbEvents;i++ ){
        error = STPTI_OSLINUX_SubscribeEvent(SessionHandle, Events_p[i] );
        if( error != ST_NO_ERROR ){
            printk(KERN_ALERT"STPTI_SubscribeEvent failed\n");
        }
    }

    if( error == ST_NO_ERROR ){
        error = STPTI_OSLINUX_WaitForEvent( SessionHandle, Event_That_Has_Occurred_p, EventData_p );
        if( error != ST_NO_ERROR ){
            printk(KERN_ALERT"Failed to wait for event\n");
        }
    }

    if( error == ST_NO_ERROR ){    
        for( i = 0;i < NbEvents;i++ ){
            error = STPTI_OSLINUX_UnsubscribeEvent(SessionHandle, Events_p[i] );
            if( error != ST_NO_ERROR ){
                printk(KERN_ALERT"STPTI_SubscribeEvent failed\n");
            }
        }
    }
    
    return error;
}

/* User space mapping helpers */
ST_ErrorCode_t stpti_GetUserSpaceMap(STPTI_Buffer_t BufferHandle, U32 *AddrDiff)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if ( Error == ST_NO_ERROR )
    {
        Buffer_t *Buffer_p = stptiMemGet_Buffer( FullBufferHandle );

        if (Buffer_p->UserSpaceAddressDiff == (U32)-1)
        {
            STTBX_Print((KERN_ALERT"pticore: stpti_GetUserSpaceMap: buffer not mapped\n"));
            Error = ST_ERROR_NO_MEMORY;
            *AddrDiff = 0;
        }
        else
            *AddrDiff = Buffer_p->UserSpaceAddressDiff;
    }
    else
        STTBX_Print(( "STPTI: invalid buffer handle 0x%08x\n", BufferHandle));

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

ST_ErrorCode_t stpti_SetUserSpaceMap(STPTI_Buffer_t BufferHandle, U32 AddrDiff)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if ( Error == ST_NO_ERROR )
    {
        Buffer_t *Buffer_p = stptiMemGet_Buffer( FullBufferHandle );
        Buffer_p->UserSpaceAddressDiff = AddrDiff;
    }
    else
        STTBX_Print(( "STPTI: invalid buffer handle 0x%08x\n", BufferHandle));

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

void stpti_BufferCacheInvalidate(STPTI_Buffer_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if ( Error == ST_NO_ERROR )
    {
        Buffer_t             *Buffer_p = stptiMemGet_Buffer( FullBufferHandle );
        TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
        STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
        U32                   DMAIdent = Buffer_p->TC_DMAIdent;
        TCDMAConfig_t        *DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent);
        
        U32 Phys2VirtOffset = (U32)Buffer_p->MappedStart_p - (U32)Buffer_p->Start_p;
                
        U32 Write_p    =  DMAConfig_p->DMAWrite_p + Phys2VirtOffset;
        U32 Read_p     =  DMAConfig_p->DMARead_p + Phys2VirtOffset;
        U32 Top_p      =  (DMAConfig_p->DMATop_p & ~0xf) + 16 + Phys2VirtOffset;
        U32 Base_p     =  DMAConfig_p->DMABase_p + Phys2VirtOffset;

        if (Read_p > Write_p) /* Wrap around case */
        {
            stpti_InvalidateRegion((void*) Read_p, Top_p-Read_p);
            stpti_InvalidateRegion((void*) Base_p, Write_p - Base_p);
        }
        else
        {
            stpti_InvalidateRegion((void*) Read_p, Write_p-Read_p);
        }
    }
    else
        STTBX_Print(( "STPTI: invalid buffer handle 0x%08x\n", BufferHandle));

    STOS_SemaphoreSignal( stpti_MemoryLock );
}
