/*! Time-stamp: <@(#)OS21WrapperMessageQueue.h   07/04/2005 - 12:41:57   William Hennebois>
 *********************************************************************
 *  @file   : OS21WrapperMessageQueue.h
 *
 *  Project : STAPI Windows CE
 *
 *  Package : 
 *
 *  Company :  TeamLog for ST
 *
 *  Author  : William Hennebois            Date: 07/04/2005
 *
 *  Purpose : Implementation of methods for Message Queue
 *
 *********************************************************************
 * Version History:
 *
 * V 0.10  07/04/2005 WH : First Revision
 *
 *********************************************************************
 */

#ifndef __OS21WrapperMessageQueue__
#define __OS21WrapperMessageQueue__

//! Maximum number of Queue Entity
#define QUEUE_NB_MAX  20

#if defined(MESSAGE_QUEUE_DEBUG) || defined(MESSAGE_QUEUE_FULL_DEBUG)
#define PrintMessageQ_Debug(x)		printf x

#ifdef MESSAGE_QUEUE_FULL_DEBUG
#define PrintMessageQ_FullDebug(x)  printf x
#else
#define PrintMessageQ_FullDebug(x)
#endif

#else
#define PrintMessageQ_Debug(x)
#define PrintMessageQ_FullDebug(x)
#endif


////////////////////////////////////////////////////////
///////////////                 ////////////////////////
//////////////// Struct		    ////////////////////////
///////////////                 ////////////////////////
////////////////////////////////////////////////////////

//! A Message Queue
#ifndef WINCE_MESSAGE_QUEUE_T
#define WINCE_MESSAGE_QUEUE_T
typedef struct
{
    void ** messages;    // array of all the messages
    unsigned int itemNumber;
    size_t       itemSize;
    HANDLE freeMessagesRead;   // queue of free messages, read-side
    HANDLE freeMessagesWrite;  // queue of free messages, write-side
    HANDLE sentMessagesRead ;  // queue of sent messages, read-side
    HANDLE sentMessagesWrite ; // queue of sent messages, write-side
    LONG waitingTasks; // number of tasks waiting on this queue

    // for debug only
    DWORD free, busy;
    DWORD sent, received, claimed, released;
    // invariant: itemNumber == free + busy + (claimed - sent) + (received - released)
} message_queue_t;
#endif

/* Messages functions */

message_queue_t* message_create_queue(size_t ElementSize, unsigned int NumElements);
#define message_create_queue_timeout  message_create_queue
int   message_delete_queue(message_queue_t* MessageQueue);

void* message_claim(message_queue_t * MessageQueue);
void* message_claim_timeout(message_queue_t *queue, osclock_t * time_end_p);

void message_send(message_queue_t *queue, void *message);

void *message_receive (message_queue_t* MessageQueue);
void *message_receive_timeout (message_queue_t* queue, osclock_t * ticks);

void message_release(message_queue_t* MessageQueue, void* Message);

#endif //__OS21WrapperMessageQueue__
