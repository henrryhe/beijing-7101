/******************************************************************************\
 *
 * File Name   : stevtint.h
 *
 * Copyright STMicroelectronics - February 1999
 *
\******************************************************************************/
#ifndef __STEVT_INTERNAL_H
#define __STEVT_INTERNAL_H

#if !defined( ST_OSLINUX )
#include <stdio.h>

#include "stlite.h"
#endif
#include "stevt.h"
#include "mem_handler.h"

typedef struct Connection_s  *UserHandle_t;

/* For locking */
#define INSIDE_TASK() (task_context(NULL, NULL) == task_context_task)
#define TASK_DELAY_VALUE    400

#if !defined(STEVT_NO_TBX)
#define STEVT_Report(x)          STTBX_Report(x);
#else
#define STEVT_Report(x)                {};
#endif

typedef union stevt_CallbackProc_u
{
    STEVT_CallbackProc_t       Callback;
    STEVT_DeviceCallbackProc_t DevCallback;
}stevt_CallbackProc_t;

struct DevName_s
{
    ST_DeviceName_t        Name;
    U32                    Counter;
    struct DevName_s      *Next;
};

struct EventHandler_s
{
    struct Connection_s *ConnectionList;
    struct Event_s      *EventList;
    struct DevName_s    *NameList;
    U32                  EventMaxNum;
    U32                  SubscrMaxNum;
    U32                  ConnectMaxNum;
    MemHandle_t         *MemHandle;
    U32                  MemAddr;
    /* Control semaphores */
#if defined(ST_OS21) || defined(ST_OSWINCE)
    semaphore_t          *LockSemaphore_p; /* For exclusive access to private data */
#else
    semaphore_t          LockSemaphore;
    semaphore_t          *LockSemaphore_p;
#endif
};

struct Registrant_s
{
    UserHandle_t         Registrant;
    struct Event_s      *Event;
    struct DevName_s    *RegName;
    struct Registrant_s *Next;
};

struct Subscription_s
{
    UserHandle_t           Subscriber;
    struct DevName_s      *RegName;
    stevt_CallbackProc_t   Notify;
    void                  *SubscrData_p;
    BOOL                   MultiInstance;
    U32			           OverFlow;
    struct Subscription_s *Next;
#if defined(ST_OSLINUX)
    BOOL WithSize; /* Indicates callback called with event data size. */
#endif
};

struct Connection_s {
    struct EventHandler_s *EH_p;
    U32                    MagicNumber; /* Set to define a valid Handle */
    STEVT_SubscriberID_t   ID;
    struct Connection_s   *Next;
};


typedef struct Subscription_s Subscription_t;
typedef struct EventHandler_s EventHandler_t;
typedef struct Registrant_s   Registrant_t;
typedef struct DevName_s      DevName_t;
typedef struct Connection_s   Connection_t;
typedef struct SubID_s     SubID_t;

#ifdef ST_OSLINUX
int stevt_dump_state(char *buf, int count);
int stevt_dump_events(char *buf, int count, int offset, int *eof);
#endif
#endif /* #ifndef __STEVT_INTERNAL_H */

