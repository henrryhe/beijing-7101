/******************************************************************************\
 *
 * File Name   : event.c
 *
 * Copyright STMicroelectronics - 1999-2000
 *
\******************************************************************************/
#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <string.h>

#include "stlite.h"

#ifndef STEVT_NO_TBX
#include "sttbx.h"
#endif

#endif

#if defined(ST_OSLINUX)
    #include "stddefs.h"
    #include "stevt_core_proc.h"
#endif

#include "stos.h"
#include "event.h"
#include "stcommon.h"
#include "mem_handler.h"
#include "stevtint.h"

static ST_ErrorCode_t stevt_EventUnsubscribeAll (Event_t *Event,
                                                 UserHandle_t Subscriber);

static ST_ErrorCode_t stevt_EventUnregisterAll (Event_t *Event,
                                                 UserHandle_t Registrant);
static U32 GetRealSize (U32 ObjSize);

static void  EventLock(void);
static void  EventUnlock(void);

static void  EventLock()
{
    interrupt_lock();
}

static void  EventUnlock()
{
    interrupt_unlock();
}


/* RegistrantName NULL for anonymous registration */
ST_ErrorCode_t stevt_EventRegister ( STEVT_EventConstant_t EventConst,
                                     UserHandle_t          Registrant,
                                     const ST_DeviceName_t RegistrantName,
                                     STEVT_EventID_t      *EventID,
				     const char		  *EventName)
{
    Registrant_t   *RegSlot_p = NULL;
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    Event_t        *Event = NULL;
    EventHandler_t *EH = NULL;
    BOOL RemovedEvent = FALSE;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_EventRegister()"));

    EH = Registrant->EH_p;


    /* Searches if the event has been referenced */
    Event = stevt_SearchEvent(EH, EventConst );
    if (Event != NULL )
    {
        /*
         * The Event has been already created. See if this event has
         * already been registered by the same registrant.
         */

        RegSlot_p = stevt_SearchRegistrantByName(Event, RegistrantName);
        /* An Event with NULL registrant can be registered by only one
           what ever may be the handle */
        if (RegSlot_p != NULL && RegistrantName == NULL)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                         "Event already registered by a registrant with the same name or anonymous"));
            return STEVT_ERROR_ALREADY_REGISTERED;
        }
        else
        {
            /* To disallow multiple registration from
            same handle with diffrent REG name*/
            RegSlot_p = stevt_SearchRegistrantByHandle(Event, Registrant);
            if (RegSlot_p != NULL )
            {
                STEVT_Report((STTBX_REPORT_LEVEL_ERROR,
                            "Event already registered by the same Registrant"));
                return STEVT_ERROR_ALREADY_REGISTERED;
            }
	    /* The name is unset if it was Subscribed to first. */
	    Event->EventName = EventName;
        }
    }
    else
    {
        /*
         *  Add a new event slot because it is not yet referenced
         */

        Event = stevt_AddEvent( EH, EventConst, EventName );
        if (Event == NULL )
        {
           STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"No more space for a new event"));
           return STEVT_ERROR_NO_MORE_SPACE;
        }
    }

    /*
     * Create a new registrant slot for this event
     */
    /* Why two functions stevt_AddRegistrant and stevt_SearchRegistrantByHandle
     have been created when it can be done with one function as in
     stevt_AddSubscriber which will return RegSlot_p  */

    /* INSIDE_TASK() is redundunt, register should always called from task
     * before adding registrant get the Write semaphore which may be acquired
       by Notify API */
    if (INSIDE_TASK())
    {
    	STOS_SemaphoreWait(Event->WriteSemaphore_p);
        interrupt_lock();
        Event->WriteCnt++;
        interrupt_unlock();
    }

    EventLock();        /* Disable all interrupts: CRITICAL REGION */
    Error = stevt_AddRegistrant(Event, Registrant, RegistrantName);
    if (Error != ST_NO_ERROR )
    {
        EventUnlock();
        stevt_RemoveEvent( EH, EventConst, &RemovedEvent); /* remove if no other references */

        /* Release Write semaphore and decrement Write Count */
        if (INSIDE_TASK())
        {
            interrupt_lock();
            Event->WriteCnt--;
            interrupt_unlock();
            STOS_SemaphoreSignal(Event->WriteSemaphore_p);
        }
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Not possible to add a new Registrant"));
        return Error;
    }

    EventUnlock();  /* Enable all interrupts: END CRITICAL REGION */

    /* Release Write semaphore and decrement Write Count */
    if (INSIDE_TASK())
    {
        interrupt_lock();
        Event->WriteCnt--;
        interrupt_unlock();
        STOS_SemaphoreSignal(Event->WriteSemaphore_p);

    }


    /* If we Register the same event with diffrent registrant name
    then in Event_s->RegistrantList structure two diffrent Registrant_t structure
    will be added, but with the stevt_SearchRegistrantByHandle function
    we will get the first Registrant_t pointer because for both diffrent
    Registrant_t,Registrant_t->Registrant is same.I Think it should search either RegName
    or RegName and handle  both */
    RegSlot_p=stevt_SearchRegistrantByHandle(Event, Registrant);
    /*
     * EventID is a pointer to the registrant slot for this event.
     */
    *EventID=(STEVT_EventID_t)RegSlot_p;
    if (*EventID == 0 )
    {
        /*
         * This case should not been possible. Internal error. Call
         * Remove* might actually make things worse, so don't.
         */
        STEVT_Report((STTBX_REPORT_LEVEL_FATAL,"Registrant has been added but not found"));
        return ST_ERROR_INVALID_HANDLE;
    }

    return ST_NO_ERROR;
}

/*
 * Unregister a specified Event registered by a specified Registrant,
 * regardless its DeviceName. Unregister doesn't need the registrant
 * name as it is not allowed that an instance of a registrant could
 * register the same event more than once.
 */
ST_ErrorCode_t stevt_EventUnregister (Event_t        *Event,
                                      UserHandle_t    Registrant, const ST_DeviceName_t  RegistrantName )
{
    ST_ErrorCode_t  Error;
    Registrant_t   *RegSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_EventUnregister()"));

    RegSlot_p = stevt_SearchRegistrantByHandle( Event, Registrant);
    if (RegSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Registrant not found"));
        return ST_ERROR_INVALID_HANDLE;
    }

    /* INSIDE_TASK() is redundunt, register should always called from task
     * before adding registrant get the Write semaphore which may be acquired
     * by Notify API */
    if (INSIDE_TASK())
    {

        STOS_SemaphoreWait(Event->WriteSemaphore_p);
        interrupt_lock();
        Event->WriteCnt++;
        interrupt_unlock();
    }
    EventLock();        /* Disable all interrupts: CRITICAL REGION */

    /*
     * Search if this event has been registered by a Registrant.
     * If it exists, it removes the registrant slot and returns ST_NO_ERROR
     * otherwise it returns STEVT_ERROR_ALREADY_UNREGISTERED if there are not
     * registrant for this event or ST_ERROR_INVALID_HANDLE if the event has
     * been registered by an other user.
     */

    Error = stevt_RemoveRegistrant( Event, Registrant, RegistrantName);
    if (Error != ST_NO_ERROR)
    {
        EventUnlock();  /* Enable all interrupts before exit */
         /* Release Write semaphore and decrement Write Count */
        if (INSIDE_TASK())
        {
            interrupt_lock();
            Event->WriteCnt--;
            interrupt_unlock();
            STOS_SemaphoreSignal(Event->WriteSemaphore_p);

        }
        return Error;
    }

    EventUnlock();      /* Enable all interrupts: END CRITICAL REGION */

     /* Release Write semaphore and decrement Write Count */
    if (INSIDE_TASK())
    {
        interrupt_lock();
        Event->WriteCnt--;
        interrupt_unlock();
        STOS_SemaphoreSignal(Event->WriteSemaphore_p);
    }

    return ST_NO_ERROR;
}

#if defined(ST_OSLINUX) && defined(MODULE)
/* RegistrantName NULL for anonymous (receive all notifications) */
ST_ErrorCode_t stevt_EventSubscribe( STEVT_EventConstant_t EventConst,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              UserHandle_t Subscriber,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs,
                              BOOL WithSize) /* If true call callback with size of event data */
#else
/* RegistrantName NULL for anonymous (receive all notifications) */
ST_ErrorCode_t stevt_EventSubscribe( STEVT_EventConstant_t EventConst,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              UserHandle_t Subscriber,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs)
#endif
{
    Subscription_t *SubsSlot_p=NULL;
    EventHandler_t *EH;
    Event_t        *Event;
    BOOL           RemovedEvent;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_EventSubscribe()"));

    EH = Subscriber->EH_p;

    /* Searches if the event has been referenced */
    Event = stevt_SearchEvent( EH, EventConst );
    if (Event != NULL )
    {
        /*
         * The event has already been created.  Check if the Subscsriber
         * is already present in the subscription list for the pair
         * Event/RegistrantName.
         */
        SubsSlot_p = stevt_SearchSubscriber(Event, Subscriber, RegistrantName);
        if (SubsSlot_p != NULL)
        {
           return STEVT_ERROR_ALREADY_SUBSCRIBED;
        }
    }
    else
    {
        /* Add a new event slot if not yet referenced  */

        Event = stevt_AddEvent( EH, EventConst, NULL );
        if (Event == NULL )
        {
           STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"No more space for a new event"));
           return STEVT_ERROR_NO_MORE_SPACE;
        }
    }

    /* INSIDE_TASK() is redundunt, register should always called from task
     * before adding registrant get the Write semaphore which may be acquired
     * by Notify API */
    if (INSIDE_TASK())
    {

        STOS_SemaphoreWait(Event->WriteSemaphore_p);
        interrupt_lock();
        Event->WriteCnt++;
        interrupt_unlock();
    }

    EventLock();        /* Disable all interrupts: CRITICAL REGION */

    /*
     * Add a Subscriber slot for the pair Event/RegistrantName
     */
#if defined(ST_OSLINUX)
    SubsSlot_p = stevt_AddSubscriber(Event,
                                     Subscriber,
                                     SubscrParams,
                                     RegistrantName,
                                     IsMultiInstanceSubs,
                                     WithSize);
#else
    SubsSlot_p = stevt_AddSubscriber(Event,
                                     Subscriber,
                                     SubscrParams,
                                     RegistrantName,
                                     IsMultiInstanceSubs);
#endif
    if (SubsSlot_p == NULL)
    {
       EventUnlock();  /* Enable all interrupts before exit */
       stevt_RemoveEvent(EH, EventConst,&RemovedEvent);

       /* Release Write semaphore and decrement Write Count */
       if (INSIDE_TASK())
       {

            interrupt_lock();
            Event->WriteCnt--;
            interrupt_unlock();
            STOS_SemaphoreSignal(Event->WriteSemaphore_p);

        }
        return STEVT_ERROR_NO_MORE_SPACE;

    }

   /* Ques-> Why This Connetion_t->ID contains its own pointer*/
   /* Ans-> This will help in case of STEVT_NotifySubscriber */
    Subscriber->ID = stevt_Crypt( Subscriber);

    EventUnlock();      /* Enable all interrupts: END CRITICAL REGION */

       /* release that Event semaphore */

    /* Release Write semaphore and decrement Write Count */
    if (INSIDE_TASK())
    {
        interrupt_lock();
        Event->WriteCnt--;
        interrupt_unlock();
        STOS_SemaphoreSignal(Event->WriteSemaphore_p);

    }

    return ST_NO_ERROR;
}

/* RegistrantName NULL to remove an anonymous subscription */
ST_ErrorCode_t stevt_EventUnsubscribe (STEVT_EventConstant_t EventConst,
                                       UserHandle_t          Subscriber,
                                       const ST_DeviceName_t       RegistrantName)
{
    ST_ErrorCode_t  Error=ST_NO_ERROR;
    EventHandler_t *EH;
    Event_t        *Event;
    BOOL            RemovedEvent = FALSE;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_EventUnsubscribe()"));

    EH = Subscriber->EH_p;

    /* Searches the refered event */
    Event = stevt_SearchEvent( EH, EventConst );
    if (Event == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Event not found"));
        return STEVT_ERROR_INVALID_EVENT_NAME;
    }

    /* INSIDE_TASK() is redundunt, register should always called from task
     * before adding registrant get the Write semaphore which may be acquired
     * by Notify API */
    if (INSIDE_TASK())
    {
        STOS_SemaphoreWait(Event->WriteSemaphore_p);
        interrupt_lock();
        Event->WriteCnt++;
        interrupt_unlock();
    }

    Error = stevt_RemoveSubscriber( Event, Subscriber, RegistrantName,&RemovedEvent);
    if (Error != ST_NO_ERROR)
    {
        /* Release Write semaphore and decrement Write Count */
        if (INSIDE_TASK())
        {
            interrupt_lock();
            Event->WriteCnt--;
            interrupt_unlock();
            STOS_SemaphoreSignal(Event->WriteSemaphore_p);

        }
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Subscriber not found"));
        return Error;
    }


    if (RemovedEvent == FALSE)
    {
        /* Release Write semaphore and decrement Write Count */
        if (INSIDE_TASK())
        {
            interrupt_lock();
            Event->WriteCnt--;
            interrupt_unlock();
            STOS_SemaphoreSignal(Event->WriteSemaphore_p);
        }
    }

    return ST_NO_ERROR;
}


/*
 * For a given UserHandle_t, for each Event:
 * 1) it searchs if an event has been registered by TheUser ( regardless to the
 *    RegistrantName. If found it calls all the unregister callback of
 *    subscribers to this event.
 *
 * 2) it searchs if some subscriptions of TheUser exist and it unsubscribes them
 *
 */
void stevt_Close(UserHandle_t TheUser)
{
    EventHandler_t *EHPtr;
    STEVT_EventConstant_t EventConst;
    Event_t  *Current;
    BOOL   RemovedEvent = FALSE;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_Close()"));

    EHPtr = TheUser->EH_p;

    Current = EHPtr->EventList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Empty Event list"));
        return;
    }

    do
    {
        /*
         * Search if this event has been registered by a registrant specified
         * by the handle TheUser, if found the registrant is removed by the
         * registrant list for this event and the unregister callback are called
         */
        stevt_EventUnregisterAll(Current, TheUser);

        /*
         * Searchs if TheUser has performed some subscriptions, and unsubscribes
         */
        stevt_EventUnsubscribeAll(Current, TheUser);

        EventConst = Current->EventConst;
        Current = Current->Next;

        /*
         * If no more referenced (both subscribers and
         * registrants) remove current event.
         */
        stevt_RemoveEvent(EHPtr, EventConst,&RemovedEvent);

    } while (Current != NULL );

}


/************************************************************************
 *
 * Registrant's list
 *
 ************************************************************************/

/*
 * Search a Registrant by Handle in the
 * registrant list associated to an event.
 * Returns a pointer to the Registrant slot if found or NULL
 */
Registrant_t *stevt_SearchRegistrantByHandle (Event_t      *Event,
                                              UserHandle_t  Registrant)
{
    Registrant_t *RegSlot_p = NULL;
    Registrant_t *Current = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_SearchRegistrantByHandle()"));

    Current = Event->RegistrantList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Empty registrant list"));
        return NULL;
    }

    /* Search the registrant slot in list */
    do
    {
        if (Current->Registrant == Registrant )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found registrant"));
            RegSlot_p = Current;
            break;
        }
        Current = Current->Next;
    } while (Current != NULL );

    return RegSlot_p;
}

/*
 * Search a Registrant by Name in the
 * registrant list associated to an event.
 * Returns a pointer to the Registrant slot if found or NULL
 * Input name NULL looks for anonymous registration
 */
Registrant_t *stevt_SearchRegistrantByName(Event_t      *Event,
                                           const ST_DeviceName_t RegistrantName)
{
    Registrant_t *RegSlot_p = NULL;
    Registrant_t *Current = NULL;
    DevName_t    *NameSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_SearchRegistrantByName()"));


    if (RegistrantName != NULL)
    {
        /* Search the name in the list of EventHandler_t->NameList */
        NameSlot_p = stevt_SearchName(Event->EH_p, RegistrantName);
        if (NameSlot_p == NULL )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Name not found"));
            return (Registrant_t *)NULL;
        }
    }

    Current = Event->RegistrantList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Empty registrant list"));
        return NULL;
    }

    /* Search the registrant slot in list */
    do
    {
        if (Current->RegName == NameSlot_p )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found registrantName %s",RegistrantName));
            RegSlot_p = Current;
            break;
        }
        Current = Current->Next;
    } while (Current != NULL );

    return RegSlot_p;
}

/*
 * Allocate and add a registrant slot to a linked list of registrants for
 * this event. RegistrantName NULL for anonymous registration
 */
ST_ErrorCode_t stevt_AddRegistrant(Event_t        *Event,
                                   UserHandle_t    Registrant,
                                   const ST_DeviceName_t RegistrantName)
{
    Registrant_t *Current, *RegSlot_p;
    MemHandle_t    *MemHandle;
    DevName_t      *NameSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_AddRegistrant()"));

    MemHandle = Event->EH_p->MemHandle;
    /*
     * Allocate a new slot for a registrant
     */
    RegSlot_p = stevt_AllocMem(sizeof(Registrant_t), MemHandle);
    if (RegSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
        return STEVT_ERROR_NO_MORE_SPACE;
    }
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Registrant slot at 0x%08X",
                  RegSlot_p));

    /*
     * Fill the allocated slot
     */
    RegSlot_p->Registrant = Registrant;
    RegSlot_p->Event = Event;

    if (RegistrantName != NULL)
    {
        NameSlot_p = stevt_AddName(Event->EH_p, RegistrantName);
        if (NameSlot_p == NULL )
        {
            stevt_FreeMem((void *)RegSlot_p, MemHandle);
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
            return STEVT_ERROR_NO_MORE_SPACE;
        }
    }
    RegSlot_p->RegName = NameSlot_p;
    RegSlot_p->Next = (Registrant_t *)NULL;

    /*
     * Add this slot at the end of the linked list of
     * the registrants for this event
     */

    if (Event->RegistrantList == NULL )
    {
        Event->RegistrantList = RegSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Event->RegistrantList->Registrant  0x%08X",Event->RegistrantList->Registrant));
    }
    else
    {
        Current = Event->RegistrantList;

        /* Search the last registrant slot in list */
        /* JF: why not put it at the start of the list? */
        while (Current->Next != NULL )
        {
            Current = Current->Next;
        }

        Current->Next = RegSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Next->Registrant          0x%08X",Current->Next->Registrant));
    }

    return ST_NO_ERROR;
}

/*
 * Remove and free a registrant slot in a linked list of registrants for
 * this event. RemoveRegistrant doesn't need the registrant
 * name as it is not allowed that an instance of a registrant could
 * register the same event more than once.
 */
ST_ErrorCode_t stevt_RemoveRegistrant(Event_t        *Event,
                                      UserHandle_t    Registrant , const ST_DeviceName_t  RegistrantName )
{
    Registrant_t *Current   = NULL;
    Registrant_t *Predec    = NULL;
    BOOL    Found=FALSE;
    MemHandle_t    *MemHandle;
    DevName_t      *NameSlot_p = NULL;
    ST_ErrorCode_t error = ST_NO_ERROR;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_RemoveRegistrant()"));

    if (RegistrantName != NULL)
    {
        NameSlot_p = stevt_SearchName(Event->EH_p, RegistrantName);
        if (NameSlot_p == NULL)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Registrant Name not found"));
            return STEVT_ERROR_NOT_SUBSCRIBED;
        }
    }
	
    Current = Event->RegistrantList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty registrant list"));
        return STEVT_ERROR_ALREADY_UNREGISTERED;
    }

    MemHandle = Event->EH_p->MemHandle;

    /* Search and remove the registrant slot in list */
    do
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Registrant  0x%08X",Current->Registrant));
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Registrant           0x%08X",Registrant));
        if ((Current->Registrant == Registrant )&&
             (Current->RegName == NameSlot_p ))
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found registrant"));
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Registrant destroyed"));
            if (Predec == NULL )
            {
                Event->RegistrantList = Current->Next;
            }
            else
            {
                Predec->Next = Current->Next;
            }

            /*
             * Where a name is involved, remove it if no longer referenced
             */
            if (Current->RegName != NULL)
            {
                error = stevt_RemoveName(Event->EH_p,Current->RegName->Name);
            }

            Current->RegName = NULL; /* why when we're about to free? */
            Current->Registrant = NULL;
            Current->Event = NULL;
            stevt_FreeMem ( (void *)Current, MemHandle );
            Found = TRUE;
            break;
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec  = Current;
        }
        Current = Current->Next;
    } while (Current != NULL );

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Registrant not found"));
        return STEVT_ERROR_ALREADY_UNREGISTERED;
    }

    return error;
}

/*
 * Check the event ID and returns a pointer to the registrant slot
 * or NULL
 */
Registrant_t *stevt_CheckID( STEVT_EventID_t EvID, UserHandle_t TheUser)
{
    Registrant_t   *RegSlot_p;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_CheckID()"));

    if (EvID == 0)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Null EventID"));
        RegSlot_p = (Registrant_t *)NULL;
    }
    else
    {
        RegSlot_p = (Registrant_t *)EvID;
        if (RegSlot_p->Registrant != TheUser )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Wrong EventID"));
            RegSlot_p = (Registrant_t *)NULL;
        }
    }

    return RegSlot_p;
}

/************************************************************************
 *
 * Name's list
 *
 ************************************************************************/
/*
 * Search a Name in the Name list
 * Returns a pointer to the name slot if found or NULL
 * Name must not be NULL
 */
DevName_t *stevt_SearchName( EventHandler_t *EH_p, const ST_DeviceName_t Name)
{
    DevName_t *Current = NULL;
    DevName_t *NameSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_SearchName()"));

    Current = EH_p->NameList;
    if (Current == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Empty name list"));
        return NULL;
    }

    /* Search the name slot in list */
    do
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Name  %s",Current->Name));
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Name           %s",Name));
        if (strcmp(Current->Name,Name) == 0 )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found Name %s",Name));
            NameSlot_p = Current;
            break;
        }
        Current = Current->Next;
    } while (Current != NULL );

    return NameSlot_p;
}

/*
 * Allocate and add a name slot to a linked list of names for
 * this Event Handler if not yet registered.
 * Returns a pointer to the allocated slot.
 * Name must not be NULL. Caller provides locking
 */
DevName_t *stevt_AddName( EventHandler_t *EH_p, const ST_DeviceName_t Name)
{
    DevName_t *Current, *NameSlot_p=NULL;
    MemHandle_t    *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_AddName()"));

    MemHandle = EH_p->MemHandle;

    /*
     * Search if this Name has alraedy been registered
     */
    NameSlot_p = stevt_SearchName(EH_p,Name);
    if (NameSlot_p != NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Name already registered"));
        NameSlot_p->Counter++;
        return NameSlot_p;
    }

    /*
     * Allocate a new slot for the name
     */
    NameSlot_p = stevt_AllocMem(sizeof(DevName_t), MemHandle);
    if (NameSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
        return NULL;
    }
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Name slot at 0x%08X",
                  NameSlot_p));

    /*
     * Fill the allocated slot
     */
    /*Why the value of counter is 1 and why it is needed*/
    strcpy(NameSlot_p->Name, Name);
    NameSlot_p->Counter = 1;
    NameSlot_p->Next = (DevName_t *)NULL;

    /*
     * Add this slot at the end of the linked list of name
     */
    if (EH_p->NameList == NULL )
    {
        EH_p->NameList = NameSlot_p;
    }
    else
    {
        Current = EH_p->NameList;

        /* Search the last name slot in list */
        /* JF: why not put it at the start of the list? */
        while (Current->Next != NULL )
        {
            Current = Current->Next;
        }
        Current->Next = NameSlot_p;
    }
    return NameSlot_p;
}

/*
 * Remove and free a name slot in a linked list of names for
 * this Event Handler if no more needed (i.e. not more referenced
 * by any user). Name must not be NULL. Caller provides locking
 */
ST_ErrorCode_t stevt_RemoveName( EventHandler_t *EH_p, const ST_DeviceName_t Name)
{
    DevName_t   *Current   = NULL;
    DevName_t   *Predec    = NULL;
    BOOL         Found     = FALSE;
    MemHandle_t *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_RemoveName()"));

    MemHandle = EH_p->MemHandle;

    Current = EH_p->NameList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty name list"));
        return STEVT_ERROR_ALREADY_UNREGISTERED;
    }

    /* Search and remove the name slot in list */
    do
    {
        if (strcmp(Current->Name,Name) == 0 )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found name"));
            Current->Counter--;
            if (Current->Counter == 0)
            {
                /*
                 * No more referenced, we can destroy it.
                 */
                STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Name destroyed"));
                if (Predec == NULL )
                {
                    EH_p->NameList = Current->Next;
                }
                else
                {
                    Predec->Next = Current->Next;
                }
                strcpy(Current->Name,"" );
                stevt_FreeMem ( (void *)Current, MemHandle );
            }
            Found = TRUE;
            break;
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec  = Current;
        }
        Current = Current->Next;
    } while (Current != NULL );

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Name not found"));
        return ST_ERROR_INVALID_HANDLE;
    }

    return ST_NO_ERROR;
}

/************************************************************************
 *
 * Subscriber's list
 *
 ************************************************************************/
/*
 * Allocate and add a subscriber slot to a linked list of subscribers for
 * this Event Handler if not jet registered.
 * Returns a pointer to the allocated slot.
 * RegistrantName NULL to subscribe anonymous (get all notifications)
 */
#if defined(ST_OSLINUX)
Subscription_t *stevt_AddSubscriber(Event_t        *Event,
                              UserHandle_t    Subscriber,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs,
                              BOOL WithSize)
#else
Subscription_t *stevt_AddSubscriber(Event_t        *Event,
                              UserHandle_t    Subscriber,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs)
#endif
{
    Subscription_t *Current, *SubSlot_p=NULL;
    MemHandle_t    *MemHandle;
    DevName_t      *NameSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_AddSubscriber()"));

    MemHandle = Event->EH_p->MemHandle;

    /*
     * Allocate a new slot for a Subscription
     */
    SubSlot_p = stevt_AllocMem(sizeof(Subscription_t), MemHandle);
    if (SubSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
        return NULL;
    }
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Subscriber slot at 0x%08X",
                  SubSlot_p));

    /*
     * If a name is being used, see if it is already registered and allocate
     * a new name slot if not
     */
    if (RegistrantName != NULL)
    {
        NameSlot_p = stevt_AddName(Event->EH_p, RegistrantName);
        if (NameSlot_p == NULL )
        {
            stevt_FreeMem((void *)SubSlot_p, MemHandle);
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
            return NULL;
        }
    }

    /*
     * Fill the allocated slot
     */
    SubSlot_p->Subscriber = Subscriber;
    SubSlot_p->RegName = NameSlot_p;
    SubSlot_p->MultiInstance = IsMultiInstanceSubs;
    SubSlot_p->Next = (Subscription_t *)NULL;
    SubSlot_p->OverFlow = 0;
#if defined(ST_OSLINUX)
    SubSlot_p->WithSize = WithSize; /* Indicates callback called with event data size. */
#endif
    if (IsMultiInstanceSubs == TRUE )
    {
        SubSlot_p->Notify.DevCallback     = SubscrParams->NotifyCallback;
        SubSlot_p->SubscrData_p           = SubscrParams->SubscriberData_p;
    }
    else
    {
        SubSlot_p->Notify.Callback     = (STEVT_CallbackProc_t)SubscrParams->NotifyCallback;
        SubSlot_p->SubscrData_p        = NULL;
    }
    /*
     * Add this slot at the end of the linked list of
     * the subscribers for this event
     */

    if (Event->Subscription == NULL )
    {
        Event->Subscription = SubSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Event->Subscription 0x%08X",Event->Subscription));
    }
    else
    {
        Current = Event->Subscription;

        /* Search the last registrant slot in list */
        /* JF: why not put it at the start of the list? */
        while (Current->Next != NULL )
        {
            Current = Current->Next;
        }

        Current->Next = SubSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Next 0x%08X",Current->Next));
    }



    return SubSlot_p;
}

/*
 * Remove and free a subscriber slot in a linked list of subscribers for
 * this event. RegistrantName NULL to remove an anonymous subscription
 */
ST_ErrorCode_t stevt_RemoveSubscriber (Event_t        *Event,
                                       UserHandle_t    Subscriber,
                                       const ST_DeviceName_t RegistrantName,
                                       BOOL * RemovedEvent)
{
    Subscription_t *Current   = NULL;
    Subscription_t *Predec    = NULL;
    DevName_t      *NameSlot_p = NULL;
    BOOL    Found=FALSE;
    MemHandle_t    *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_RemoveSubscriber()"));
    MemHandle = Event->EH_p->MemHandle;

    if (RegistrantName != NULL)
    {
        NameSlot_p = stevt_SearchName(Event->EH_p, RegistrantName);
        if (NameSlot_p == NULL)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Registrant Name not found"));
            return STEVT_ERROR_NOT_SUBSCRIBED;
        }
    }

    Current = Event->Subscription;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty subscriber list"));
        return STEVT_ERROR_NOT_SUBSCRIBED;
    }

    EventLock();        /* Disable all interrupts: CRITICAL REGION */

    /* Search and remove the subscriber slot in list */
    do
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Subscriber  0x%08X",Current->Subscriber));
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Subscriber           0x%08X",Subscriber));
        if ((Current->Subscriber == Subscriber) &&
             (Current->RegName == NameSlot_p ))
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found Subscriber"));


            if (Predec == NULL )
            {
                Event->Subscription = Current->Next;
            }
            else
            {
                Predec->Next = Current->Next;
            }

            /*
             * Where a name is involved, remove it if no longer referenced
             */
            if (Current->RegName != NULL)
            {
                stevt_RemoveName(Event->EH_p,RegistrantName);
            }


            Current->RegName = NULL;
            Current->Subscriber = NULL;
            Current->SubscrData_p = NULL;
            stevt_FreeMem ( (void *)Current, MemHandle );
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Subscriber destroyed"));
            Found = TRUE;
            break;
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec = Current;
        }
        Current = Current->Next;
    } while (Current != NULL );

    EventUnlock();

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Subscriber not found"));
        return STEVT_ERROR_NOT_SUBSCRIBED;

    }

   /*
    * If no more referencied (both subscribers and
    * registrants) remove the event.
    */
   stevt_RemoveEvent(Event->EH_p, Event->EventConst,RemovedEvent);

    return ST_NO_ERROR;
}


/*
 * Search a subscriber/RegistrantName pair slot in a linked list of subscribers
 * for this event. NULL is searched directly like any other value, and will have
 * zero or one matches
 */
Subscription_t *stevt_SearchSubscriber (Event_t        *Event,
                                        UserHandle_t    Subscriber,
                                        const ST_DeviceName_t RegistrantName)
{
    Subscription_t *Current    = NULL;
    DevName_t      *NameSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_SearchSubscriber()"));

    if (RegistrantName != NULL)
    {
        NameSlot_p = stevt_SearchName(Event->EH_p, RegistrantName);
        if (NameSlot_p == NULL)
        {
            STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Registrant Name not found"));
            return NULL;
        }
    }

    Current = Event->Subscription;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty subscriber list"));
        return NULL;
    }

    /* Search the subscriber slot in list */
    do
    {
        if ((Current->Subscriber == Subscriber) &&
             (Current->RegName == NameSlot_p ))
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found Subscriber"));
            return Current;
        }
        Current = Current->Next;
    } while ( Current != NULL );

    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Subscriber not found"));
    return NULL;
}

/*
 * Remove all the subscriptions slot identified by subscriber handle
 */
static ST_ErrorCode_t stevt_EventUnsubscribeAll (Event_t *Event,
                                                 UserHandle_t Subscriber)
{
    Subscription_t *Current   = NULL;
    Subscription_t *Predec    = NULL;
    BOOL    Found=FALSE;
    MemHandle_t    *MemHandle;
    ST_DeviceName_t RegistrantName;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_EventUnsubscribeAll()"));
    MemHandle = Event->EH_p->MemHandle;

    Current = Event->Subscription;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty subscriber list"));
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Search and remove the subscriber's slots in list */
    do
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Subscriber  0x%08X",Current->Subscriber));
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Subscriber           0x%08X",Subscriber));
        if (Current->Subscriber == Subscriber)
        {
            /*
             * If we remove the Current we don't have to move the Predec.
             */
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found Subscriber"));
            if (Predec == NULL )
            {
                /*
                 * Current is the first element in the list
                 * we need to update Event->Subscription (Start list)
                 */
                Event->Subscription = Current->Next;
            }
            else
            {
                /*
                 * Current is not the first element but exists a previous one
                 * we need to update the Predec
                 */
                Predec->Next = Current->Next;
            }

            if (Current->RegName != NULL)
            {
                /* JF: why copy in this case when we don't in others? */
                strcpy (RegistrantName, Current->RegName->Name);
                /*
                 * If no more referenced remove the name
                 */
                stevt_RemoveName(Event->EH_p,RegistrantName);
            }

            Current->RegName = NULL;
            Current->Subscriber = NULL;
            Current->SubscrData_p = NULL;
            stevt_FreeMem ( (void *)Current, MemHandle );
            Found = TRUE;
            /*break;*/
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec  = Current;
        }
        Current = Current->Next;
    } while (Current != NULL );

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Subscriber not found"));
        return ST_ERROR_INVALID_HANDLE;
    }

#ifdef MYDEBUG
    stevt_CountSubscribers(Event);
#endif

    return ST_NO_ERROR;
}

#if (defined(MYDEBUG) || defined(ST_OSLINUX))
static U32 stevt_CountSubscribers(Event_t *Event)
{
    Subscription_t *Current   = NULL;
    Subscription_t *Predec    = NULL;
    U32 Counter=0;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_CountSubscribers()"));

    Current = Event->Subscription;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty subscriber list"));
        return Counter;
    }

    do
    {
        Counter++;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Counter #%2d Current->Subscriber  0x%08X",Counter,Current->Subscriber));
        Predec  = Current;
        Current = Current->Next;
    } while (Current != NULL );

    return Counter;
}
#endif

/************************************************************************
 *
 *   CALLBACKS handling
 *
 ************************************************************************/

/*
 * Notify all the user subscribed to the pair Event/NameSlot_p
 * and all the anonymous subscribers for backward compatibility
 */
#if defined(ST_OSLINUX)
void stevt_CallBothNotifyCB(Event_t    *Event,
                            DevName_t  *NameSlot_p,
                            const void *EventData,
                            U32 EventDataSize)
#else
void stevt_CallBothNotifyCB(Event_t    *Event,
                            DevName_t  *NameSlot_p,
                            const void *EventData)
#endif
{
    Subscription_t *Current   = NULL;
    char *RegistrantName = NULL;

    if (NameSlot_p != NULL)
    {
        /* (actually always called with NameSlot_p != NULL) */
        RegistrantName = NameSlot_p->Name;
    }

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_CallBothNotifyCB()"));
    Current = Event->Subscription;

    if (Current != NULL )
    {
        /* Search a subscriber to a device with this RegistrantName */
        do
        {
            if ((Current->RegName == NameSlot_p) ||
                 (Current->RegName == NULL))
            {
                /* I Think when Current->RegNam != NULL then
                Current->MultiInstance == TRUE  */

                if (Current->MultiInstance == TRUE )
                {
                    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Calling Notify.DevCallback"));
#if defined(ST_OSLINUX)
                    if( Current->WithSize ){

                        ((STEVT_DeviceCallbackProcWithSize_t)Current->Notify.DevCallback)(CALL_REASON_NOTIFY_CALL,
                                                (char*)Current->RegName,
                                                Event->EventConst,
                                                EventData,
                                                Current->SubscrData_p,
                                                EventDataSize);
                    }
                    else{
                        Current->Notify.DevCallback(CALL_REASON_NOTIFY_CALL,
                                                RegistrantName,
                                                Event->EventConst,
                                                EventData,
                                                Current->SubscrData_p);
                    }
#else
                    Current->Notify.DevCallback(CALL_REASON_NOTIFY_CALL,
                                                RegistrantName,
                                                Event->EventConst,
                                                EventData,
                                                Current->SubscrData_p);
#endif
                }
                else
                {

                    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Calling Notify.Callback"));
                    Current->Notify.Callback(CALL_REASON_NOTIFY_CALL,
                                             Event->EventConst,
                                             EventData);
                }
            }
            Current = Current->Next;
        } while ( Current != NULL );
    }
}

/*
 * Call the Notify call back of all the anonymous subscribers to
 * this event (from an anonymous registrant)
 */
#if defined(ST_OSLINUX)
void stevt_CallNotifyCB(Event_t    *Event,
                        const void *EventData,
                        U32 EventDataSize)
#else
void stevt_CallNotifyCB(Event_t    *Event,
                        const void *EventData)
#endif
{
    Subscription_t *Current   = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_CallNotifyCB()"));
    Current = Event->Subscription;

    if (Current != NULL )
    {
        /* Search a subscriber to a device with this RegistrantName */
        do
        {
            if (Current->RegName == NULL )
            {
                if (Current->MultiInstance == TRUE )
                {
#if defined(ST_OSLINUX)
                    if( Current->WithSize ){
                        ((STEVT_DeviceCallbackProcWithSize_t)Current->Notify.DevCallback)(CALL_REASON_NOTIFY_CALL,
                                                NULL,
                                                Event->EventConst,
                                                EventData,
                                                Current->SubscrData_p,
                                                EventDataSize);
                    }
                    else{
                        Current->Notify.DevCallback(CALL_REASON_NOTIFY_CALL,
                                                NULL,
                                                Event->EventConst,
                                                EventData,
                                                Current->SubscrData_p);
                    }
#else
                    Current->Notify.DevCallback(CALL_REASON_NOTIFY_CALL,
                                                NULL,
                                                Event->EventConst,
                                                EventData,
                                                Current->SubscrData_p);
#endif

                }
                else
                {
                    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Calling Notify.Callback"));
                    Current->Notify.Callback(CALL_REASON_NOTIFY_CALL,
                                             Event->EventConst,
                                             EventData);
                }
            }
            Current = Current->Next;
        } while ( Current != NULL );
    }
}


/************************************************************************
 *
 * Events linked list
 *
 ************************************************************************/
/*
 * Allocate and add an event slot to a linked list of events for
 * this Event Handler if not jet registered.
 * Returns a pointer to the allocated slot.
 */
Event_t *stevt_AddEvent (EventHandler_t *EH_p, STEVT_EventConstant_t EventConst,
	const char *EventName)
{
    Event_t        *Current, *EventSlot_p=NULL;
    MemHandle_t    *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_AddEvent()"));

    MemHandle = EH_p->MemHandle;

    /*
     * Otherwise allocate a new slot for a registrant
     */

    EventSlot_p = stevt_AllocMem(sizeof(Event_t), MemHandle);
    if (EventSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
        return NULL;
    }
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Event slot at 0x%08X", EventSlot_p));
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"EventConst    0x%08X", EventConst));

    /*
     * Fill the allocated slot
     */
    EventSlot_p->EH_p           = EH_p;
    EventSlot_p->EventConst     = EventConst;
    EventSlot_p->Subscription   = NULL;
    EventSlot_p->RegistrantList = NULL;
    EventSlot_p->EventName      = EventName;
    EventSlot_p->NotifyCnt      = 0;
    EventSlot_p->OverFlow       = 0;
    EventSlot_p->Next           = NULL;

    /* To provide locking for read operation */
#ifdef ST_OS21
    EventSlot_p->WriteSemaphore_p = STOS_SemaphoreCreateFifo(NULL ,1);
#else
    EventSlot_p->WriteSemaphore_p = &EventSlot_p->WriteSemaphore;
    semaphore_init_fifo(EventSlot_p->WriteSemaphore_p,1);
#endif

    EventSlot_p->ReadCnt              = 0;
    EventSlot_p->WriteCnt             = 0;
    /*
     * Add this slot at the end of the linked list of
     * the events for this Event Handler
     */
    if (EH_p->EventList == NULL )
    {
        EH_p->EventList = EventSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"EH_p->EventList 0x%08X",EH_p->EventList));
    }
    else
    {
        Current = EH_p->EventList;

        /* Search the last event slot in list */
        /* JF: why not put it at the start of the list? */
        while (Current->Next != NULL )
        {
            Current = Current->Next;
        }

        Current->Next = EventSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Next 0x%08X",Current->Next));
    }

    return EventSlot_p;
}

/*
 * Search an event in the specified Event Handler.
 * Returns a pointer to the Event slot if found or NULL
 */
Event_t *stevt_SearchEvent(EventHandler_t *EH_p,
                           STEVT_EventConstant_t EventConst)
{
    Event_t *Current = NULL;
    Event_t *EventSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_SearchEvent()"));

    Current = EH_p->EventList;
    if (Current == NULL)
    {
        return NULL;
    }

    /* Search the event slot in list */
    do
    {
        if (Current->EventConst == EventConst )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found EventConst %d",EventConst));
            EventSlot_p = Current;
            break;
        }
        Current = Current->Next;
    } while (Current != NULL );

    return EventSlot_p;
}


/*
 * Remove and free an event slot in a linked list of events for
 * this Event Handler if no more needed (i.e. not more referenced
 * by any user).
 */
ST_ErrorCode_t stevt_RemoveEvent  (EventHandler_t *EH_p, STEVT_EventConstant_t EventConst, BOOL *RemovedEvent)
{
    Event_t     *Current = NULL;
    Event_t     *Predec  = NULL;
    BOOL         Found   = FALSE;
    MemHandle_t *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_RemoveEvent()"));

    MemHandle = EH_p->MemHandle;

    Current = EH_p->EventList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty event list"));
        return STEVT_ERROR_INVALID_EVENT_NAME;
    }

    /* Search and remove the event slot in list */
    do
    {
        if (Current->EventConst == EventConst )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found event"));
            /*
             * No more subcribers and registrants for this Event
             * we can destroy it
             */
            if ((Current->Subscription == NULL ) &&
                 ( Current->RegistrantList == NULL ))
            {

                STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Event destroyed"));
                if (Predec == NULL )
                {
                    EH_p->EventList = Current->Next;
                }
                else
                {
                    Predec->Next = Current->Next;
                }
                Current->EH_p = NULL;
                Current->EventConst = 0;
                Current->Subscription = NULL;
                Current->RegistrantList = NULL;
#ifdef ST_OSLINUX
		/* Ugly hack for user-space registered events */
		if (Current->EventName && (Current->EventName[0] == 'u') &&
		    (Current->EventName[1] == ':')) {
			kfree(Current->EventName);
			Current->EventName = NULL;
		}
#else
			Current->EventName = NULL;
#endif
                /* For locking*/
                semaphore_delete(Current->WriteSemaphore_p);

                (*RemovedEvent) = TRUE;
                Current->ReadCnt        = 0;
                Current->WriteCnt       = 0;

                stevt_FreeMem ( (void *)Current, MemHandle );
            }
            Found = TRUE;
            break;
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec  = Current;
        }
        Current = Current->Next;
    } while (Current != NULL );

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Event not found"));
        return STEVT_ERROR_INVALID_EVENT_NAME;
    }

    return ST_NO_ERROR;
}

/************************************************************************
 *
 * Connections linked list
 *
 ************************************************************************/
/*
 * Allocate and add a connection slot to a linked list for
 * this Event Handler.
 * Returns a pointer to the allocated slot.
 */
Connection_t *stevt_AddConnection (EventHandler_t *EH_p)

{
    Connection_t   *Current, *ConnectSlot_p=NULL;
    MemHandle_t    *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_AddConnection()"));

    MemHandle = EH_p->MemHandle;


    /*
     * allocate a new slot for a registrant
     */
    ConnectSlot_p = stevt_AllocMem(sizeof(Connection_t), MemHandle);
    if (ConnectSlot_p == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Insufficient memory space"));
        return NULL;
    }
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Connect slot at 0x%08X", ConnectSlot_p));

    /*
     * Fill the allocated slot
     */
    ConnectSlot_p->EH_p           = EH_p;
    ConnectSlot_p->ID             = (U32)NULL;
    ConnectSlot_p->Next           = NULL;

    /*
     * Add this slot at the end of the linked list of
     * the events for this Connect Handler
     */
    if (EH_p->ConnectionList == NULL )
    {
        EH_p->ConnectionList = ConnectSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"EH_p->ConnectionList 0x%08X",EH_p->ConnectionList));
    }
    else
    {
        Current = EH_p->ConnectionList;

        /* Search the last event slot in list */
        /* JF: why not put it at the start of the list? */
        while (Current->Next != NULL )
        {
            Current = Current->Next;
        }

        Current->Next = ConnectSlot_p;
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Next 0x%08X",Current->Next));
    }
    return ConnectSlot_p;
}

/*
 * Remove and free a connection slot in a linked list  for
 * this Event Handler
 */
ST_ErrorCode_t stevt_RemoveConnection  (EventHandler_t *EH_p, UserHandle_t User)
{
    Connection_t *Current   = NULL;
    Connection_t *Predec    = NULL;
    BOOL          Found     = FALSE;
    MemHandle_t  *MemHandle;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_RemoveConnection()"));

    MemHandle = EH_p->MemHandle;

    Current = EH_p->ConnectionList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty connection list"));
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Search and remove the event slot in list */
    do
    {
        if (Current == (Connection_t *)User )
        {
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found connection"));
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Connection destroyed"));
            if (Predec == NULL )
            {
                EH_p->ConnectionList = Current->Next;
            }
            else
            {
                Predec->Next = Current->Next;
            }
            Current->EH_p = NULL;
            Current->ID   = 0;
            Current->Next = NULL;
            Current->MagicNumber = 0;
            stevt_FreeMem ( (void *)Current, MemHandle );
            Found = TRUE;
            break;
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec  = Current;
        }
        Current = Current->Next;
    } while ( Current != NULL );

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Connection not found"));
        return ST_ERROR_INVALID_HANDLE;
    }

    return ST_NO_ERROR;
}

STEVT_SubscriberID_t stevt_Crypt( UserHandle_t TheUser)
{
    U32 Offset;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_Crypt()"));

    Offset = (U32)TheUser;

    return (STEVT_SubscriberID_t)Offset;
}

/*MJ-> Why this function needs EventHandler Pointer*/
/*Ans->Because in DDTS 13054 the implementation
Was changed but not the function signature*/
UserHandle_t stevt_Decrypt(EventHandler_t *EH_p, STEVT_SubscriberID_t ID)
{
	EventHandler_t *EH_d;
	EH_d=EH_p;

	STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_Decrypt()"));

    return (UserHandle_t)ID;
}

U32 stevt_MemSize( const STEVT_InitParams_t *Init)
{
    U32 ConSize;            /* Size of a connection slot */
    U32 NameSize;           /* Size of a name slot */
    U32 EventSize;          /* Size of an event slot */
    U32 RegSize;            /* Size of a registrant slot */
    U32 SubSize;            /* Size of a subscriber slot */
    U32 EventMax, ConnectMax, SubscrMax;
    U32 Size=0;

    ConSize = GetRealSize (sizeof(Connection_t));
    NameSize = GetRealSize (sizeof(DevName_t));
    EventSize = GetRealSize (sizeof(Event_t));
    RegSize = GetRealSize (sizeof(Registrant_t));
    SubSize = GetRealSize (sizeof(Subscription_t));

    EventMax   = Init->EventMaxNum;
    ConnectMax = Init->ConnectMaxNum;
    SubscrMax  = Init->SubscrMaxNum;
    /*
     * We have almost ConnectMax names, connections
     *
     *     Size  = ConnectMax * (ConSize + NameSize);
     *
     * The maximum number of registrants will be ConnectMax
     * Each registrant could register all the events =>
     * regslots = )the minimum value will be EventMax
     *
     *     Size += EventMax * ConnectMax *RegSize;
     *     Size += SubscrMax  * ( SubSize+SubIDSize);
     *     Size += EventMax   * EventSize;
     */

    /*  heuristic formula */
    /*
     * We assume:
     * average number of registrants slots =  EventMax * ConnectMax;
     * average number of names = ConnectMax/2
     *
     */
    Size  = ConnectMax * (ConSize + NameSize/2 + RegSize*EventMax/2);
    /* Change for DDTs GNBvd12227*/
    /*Size += SubscrMax  * (SubSize+SubIDSize);*/
    Size += SubscrMax  * (SubSize);
    Size += EventMax   * EventSize;
    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Required size %d",Size));

    return Size;
}


static U32 GetRealSize ( U32 ObjSize)
{
    U32 AllocHdrSize = 16;  /* Size of the alloc header structure */

    U32 real_size=0;

    real_size = ObjSize + AllocHdrSize;

    if (real_size % AllocHdrSize)
    {
       real_size += (AllocHdrSize - (real_size % AllocHdrSize));
    }

    STEVT_Report((STTBX_REPORT_LEVEL_INFO,"ObjSize = %d  RealSize = %d",
                  ObjSize,real_size));
    return real_size;
}

/*
 */


/* Implementation for DDTS GNBvd12227*/

Subscription_t *stevt_SearchSubscriberID ( UserHandle_t TheSubsc, Event_t * Event, DevName_t *RegName)
{
    Subscription_t *Current=NULL;
    Subscription_t *SubsSlot_p = NULL;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_SearchSubscriberID()"));

    Current=Event->Subscription;
    if (Current == NULL)
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Empty Sunscription list"));
        return NULL;
    }

    do
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Event->EventConst  %X",Event->EventConst));
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->RegName %s",
                      Current->RegName == NULL ? "NULL"
                        : Current->RegName->Name));
        if (TheSubsc == (UserHandle_t)Current->Subscriber)
        {
            if ((RegName == Current->RegName) || (Current->RegName == NULL))
            {
                SubsSlot_p = Current;
                STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found Subscription %X",SubsSlot_p));
                break;
            }
        }
        Current = Current->Next;
    } while ( Current != NULL );

    return SubsSlot_p;
}
#ifdef ST_OSLINUX
/*
 * stevt_dump_event: dump event info to buf, which is count bytes.
 * Beware: We asume there is enough room for all subscriber details.
 * If there isnt we silently ignore those that do not fit.
 */
int stevt_dump_event(Event_t *Event, char *buf, int count)
{
	int	len = 0;
	U32	subcnt;

	if ((stevt_VerboseLevel() & STEVT_VERBOSE_NOZEROCOUNTS) &&
	    (Event->NotifyCnt == 0))
		return len;
	if (count < 80) return(len);	/* Not enough room */

	if (Event->EventName)
		len += sprintf(buf+len,"%-45s", Event->EventName);
	else
		len += sprintf(buf+len, "%-45s","unknown");
	subcnt = stevt_CountSubscribers(Event);
	len += sprintf(buf+len,"0x%08X %6d %6d %4d\n",
		Event->EventConst, Event->NotifyCnt, Event->OverFlow, subcnt);

	if ((stevt_VerboseLevel() & STEVT_VERBOSE_SUBSCRIBERS) && subcnt)
	{
		Subscription_t	*sub_p = Event->Subscription;
		while ((sub_p) && ((len+ST_MAX_DEVICE_NAME+1) < count))
		{
			if (sub_p->RegName && sub_p->RegName->Name)
				len += sprintf(buf+len, "%-15.15s ", sub_p->RegName->Name);
			else
				len += sprintf(buf+len, "unknown ");
//			len += sprintf(buf+len, "0x%08x ", sub_p->OverFlow);
			sub_p = sub_p->Next;
		}
		len += sprintf(buf+len,"\n");
	}

	return(len);
}
#endif

static ST_ErrorCode_t stevt_EventUnregisterAll (Event_t *Event,
                                                 UserHandle_t Registrant)
{
    Registrant_t *Current   = NULL;
    Registrant_t *Predec    = NULL;
    BOOL    Found=FALSE;
    MemHandle_t    *MemHandle;
    ST_DeviceName_t RegistrantName;

    STEVT_Report((STTBX_REPORT_LEVEL_ENTER_LEAVE_FN,"stevt_EventUnregisterAll()"));
    MemHandle = Event->EH_p->MemHandle;

    Current = Event->RegistrantList;
    if (Current == NULL )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Empty Registrant list"));
        return ST_ERROR_INVALID_HANDLE;
    }

    /* Search and remove the subscriber's slots in list */
    do
    {
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Current->Registrant  0x%08X",Current->Registrant));
        STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Registrant          0x%08X",Registrant));
        if (Current->Registrant == Registrant)
        {
            /*
             * If we remove the Current we don't have to move the Predec.
             */
            STEVT_Report((STTBX_REPORT_LEVEL_INFO,"Found Registrant"));
            if (Predec == NULL )
            {
                /*
                 * Current is the first element in the list
                 * we need to update Event->Subscription (Start list)
                 */
                Event->RegistrantList= Current->Next;
            }
            else
            {
                /*
                 * Current is not the first element but exists a previous one
                 * we need to update the Predec
                 */
                Predec->Next = Current->Next;
            }

            if (Current->RegName != NULL)
            {
                /* JF: why copy in this case when we don't in others? */
                strcpy (RegistrantName, Current->RegName->Name);
                /*
                 * If no more referenced remove the name
                 */
                stevt_RemoveName(Event->EH_p,RegistrantName);
            }

            Current->RegName = NULL;
            Current->Registrant= NULL;
            Current->Event= NULL;
            stevt_FreeMem ( (void *)Current, MemHandle );
            Found = TRUE;
            /*break;*/
        }
        else
        {
            /*
             * Current has not been removed so
             * we have to move Predec to point Current
             */
            Predec  = Current;
        }
        Current = Current->Next;
    } while (Current != NULL );

    if (Found == FALSE )
    {
        STEVT_Report((STTBX_REPORT_LEVEL_ERROR,"Registrant not found"));
        return ST_ERROR_INVALID_HANDLE;
    }

    return ST_NO_ERROR;
}

