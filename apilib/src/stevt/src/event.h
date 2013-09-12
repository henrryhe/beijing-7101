/******************************************************************************\
 *
 * File Name   : event.h
 *
 * Description : Types and function for events management
 *
 * Copyright STMicroelectronics - 1999 - 2000
 *
\******************************************************************************/
#ifndef __EVENT_H
#define __EVENT_H


#include "stevtint.h"

struct Event_s
{
    EventHandler_t       *EH_p;
    STEVT_EventConstant_t EventConst;
    Subscription_t       *Subscription;
    Registrant_t         *RegistrantList;
    /* To keep certain operations atomic */
#if defined(ST_OS21) || defined(ST_OSWINCE)
    semaphore_t           *WriteSemaphore_p;
#else
    semaphore_t           WriteSemaphore;
    semaphore_t           *WriteSemaphore_p;
#endif
    U32                   ReadCnt;
    U32                   WriteCnt;

/* added for debugging support*/
    U32			          NotifyCnt;
    U32			          OverFlow;
    const char		          *EventName;

    struct Event_s       *Next;
};


typedef struct Event_s Event_t;

ST_ErrorCode_t stevt_EventRegister ( STEVT_EventConstant_t EventConst,
                                     UserHandle_t          Registrant,
                                     const ST_DeviceName_t RegistrantName,
                                     STEVT_EventID_t      *EventID,
                				const     char		  *EventName);

ST_ErrorCode_t stevt_EventUnregister (Event_t               *Event,
                                      UserHandle_t          Registrant, const ST_DeviceName_t  RegistrantName);
#if defined(ST_OSLINUX) && defined(MODULE)
ST_ErrorCode_t stevt_EventSubscribe( STEVT_EventConstant_t EventConst,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              UserHandle_t Subscriber,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs,
                                     BOOL WithSize);
#else
ST_ErrorCode_t stevt_EventSubscribe( STEVT_EventConstant_t EventConst,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              UserHandle_t Subscriber,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs);/* Call callback with size of event data */
#endif
ST_ErrorCode_t stevt_EventUnsubscribe (STEVT_EventConstant_t EventConst,
                                       UserHandle_t          Subscriber,
                                       const ST_DeviceName_t       RegistrantName);
void stevt_Close(UserHandle_t Handle);

Registrant_t *stevt_SearchRegistrantByHandle(Event_t      *Event,
                                             UserHandle_t  Registrant);
Registrant_t *stevt_SearchRegistrantByName(Event_t      *Event,
                                           const ST_DeviceName_t RegistrantName);
Registrant_t *stevt_CheckID( STEVT_EventID_t EvID, UserHandle_t TheUser);
ST_ErrorCode_t stevt_RemoveRegistrant(Event_t        *Event,
                                      UserHandle_t    Registrant, const ST_DeviceName_t  RegistrantName);
ST_ErrorCode_t stevt_AddRegistrant(Event_t        *Event,
                                   UserHandle_t    Registrant,
                                   const ST_DeviceName_t RegistrantName);
DevName_t *stevt_SearchName( EventHandler_t *EH_p, const ST_DeviceName_t Name);
DevName_t *stevt_AddName( EventHandler_t *EH_p, const ST_DeviceName_t Name);
ST_ErrorCode_t stevt_RemoveName( EventHandler_t *EH_p, const ST_DeviceName_t Name);

Subscription_t *stevt_SearchSubscriber (Event_t        *Event,
                                        UserHandle_t    Subscriber,
                                        const ST_DeviceName_t RegistrantName);
ST_ErrorCode_t stevt_RemoveSubscriber (Event_t        *Event,
                                       UserHandle_t    Subscriber,
                                       const ST_DeviceName_t RegistrantName,
                                       BOOL * RemovedEvent);
#if defined(ST_OSLINUX)
Subscription_t *stevt_AddSubscriber(Event_t        *Event,
                              UserHandle_t    Subscriber,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs,
                              BOOL WithSize);
#else
Subscription_t *stevt_AddSubscriber(Event_t        *Event,
                              UserHandle_t    Subscriber,
                              const STEVT_DeviceSubscribeParams_t *SubscrParams,
                              const ST_DeviceName_t RegistrantName,
                              BOOL IsMultiInstanceSubs);
#endif

Event_t *stevt_AddEvent(EventHandler_t *EH_p, STEVT_EventConstant_t EventConst,
			                                                    const char *EventName);
Event_t *stevt_SearchEvent(EventHandler_t *EH_p, STEVT_EventConstant_t Event);
ST_ErrorCode_t stevt_RemoveEvent(EventHandler_t *EH_p,
                                 STEVT_EventConstant_t EventConst,BOOL * RemovedEvent);

Connection_t *stevt_AddConnection (EventHandler_t *EH_p);
ST_ErrorCode_t stevt_RemoveConnection (EventHandler_t *EH_p, UserHandle_t User);

STEVT_SubscriberID_t stevt_Crypt( UserHandle_t TheUser);
UserHandle_t stevt_Decrypt(EventHandler_t *EH_p, STEVT_SubscriberID_t ID);
U32 stevt_MemSize(const STEVT_InitParams_t *Init);

#if defined(ST_OSLINUX)
void stevt_CallBothNotifyCB(Event_t *Event, DevName_t *NameSlot_p,
                            const void *EventData,
                            U32 EventDataSize);
void stevt_CallNotifyCB(Event_t *Event, const void *EventData,U32 EventDataSize);
#else
void stevt_CallBothNotifyCB(Event_t *Event, DevName_t *NameSlot_p,
                            const void *EventData);
void stevt_CallNotifyCB(Event_t *Event, const void *EventData);
#endif

SubID_t *stevt_AddSubID( UserHandle_t TheSubsc, Subscription_t *Subscriber,
                         STEVT_EventConstant_t EventConst);
Subscription_t *stevt_SearchSubID ( UserHandle_t TheSubsc,
                                    STEVT_EventConstant_t EventConst,
                                    DevName_t *RegName);
Subscription_t *stevt_SearchSubscriberID ( UserHandle_t TheSubsc,
                                    Event_t * Event,
                                    DevName_t *RegName);
ST_ErrorCode_t stevt_RemoveSubID( UserHandle_t TheSubsc,
                                  Subscription_t *Subscriber,
                                  STEVT_EventConstant_t EventConst);


#ifdef ST_OSLINUX
int stevt_dump_event(Event_t *Event, char *buf, int count);
#endif

#endif  /* #ifndef __EVENT_H */

