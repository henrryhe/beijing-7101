
#include <Msgqueue.h> // Windows CE message queues

#include "OS21WrapperMessageQueue.h"


/****************************************************/
/*                MESSAGES							*/
/****************************************************/


message_queue_t* message_create_queue(size_t ElementSize, unsigned int NumElements)
{
    message_queue_t* theNewQueue = NULL;
    int index, aa;
    MSGQUEUEOPTIONS mSGQOptions = { sizeof(mSGQOptions), 
	                                0,              // allocate memory at creation
                                    NumElements,    // max messages = no limit
	                                sizeof(void*),  // size of each message in queue
                                    TRUE // reader side
                                   }; 

    WCE_MSG(MDL_OSCALL,"-- message_create_queue()");

    theNewQueue = (message_queue_t*) malloc(sizeof(*theNewQueue));
    WCE_ASSERT(theNewQueue != NULL);
    if (theNewQueue == NULL)
        return NULL;

    theNewQueue->itemNumber = NumElements;
    theNewQueue->itemSize = ElementSize;

    theNewQueue->messages = (void**) malloc(NumElements * sizeof(*(theNewQueue->messages)));
    WCE_ASSERT(theNewQueue->messages != NULL);
    if (theNewQueue->messages == NULL)
    {
        free(theNewQueue);
        return NULL;
    }
    for (index = 0; index < NumElements; index++)
    {
        theNewQueue->messages[index] = (void*) malloc(ElementSize);
        WCE_ASSERT(theNewQueue->messages[index] != NULL)
        if (theNewQueue->messages[index] == NULL)
        {
            for (aa = 0; aa < index; aa++)
                free(theNewQueue->messages[aa]);
            free(theNewQueue->messages);
            free(theNewQueue);
            return NULL;
        }
    }

    // initialize read sides
    theNewQueue->freeMessagesRead = CreateMsgQueue(NULL, &mSGQOptions);
    WCE_VERIFY(theNewQueue->freeMessagesRead != NULL);
    theNewQueue->sentMessagesRead = CreateMsgQueue(NULL, &mSGQOptions);
    WCE_VERIFY(theNewQueue->sentMessagesRead != NULL);

    // initialize write sides
    mSGQOptions.bReadAccess = FALSE; // switch to write-side
    theNewQueue->freeMessagesWrite =
        OpenMsgQueue(GetCurrentProcess(),           // queue owner process
                     theNewQueue->freeMessagesRead, // the queue to re-open                                      
                     &mSGQOptions);
    WCE_VERIFY(theNewQueue->freeMessagesWrite != NULL);
    theNewQueue->sentMessagesWrite =
        OpenMsgQueue(GetCurrentProcess(),           // queue owner process
                     theNewQueue->sentMessagesRead, // the queue to re-open                                      
                     &mSGQOptions);
    WCE_VERIFY(theNewQueue->sentMessagesWrite != NULL);

    // put all messages into free queue
    for (index = 0; index < NumElements; index++)
    {
        BOOL ret = WriteMsgQueue(theNewQueue->freeMessagesWrite, theNewQueue->messages + index, sizeof(void*), 0, 0);
        WCE_VERIFY(ret);
    }

    // nobody is currently waiting on this queue
    theNewQueue->waitingTasks = 0;

    // initialize debug infos
    theNewQueue->free = NumElements;
    theNewQueue->busy = 0;
    theNewQueue->sent = theNewQueue->received = theNewQueue->claimed = theNewQueue->released = 0;

    return theNewQueue;
}


int message_delete_queue(message_queue_t* MessageQueue)
{
    int index;

    WCE_MSG(MDL_OSCALL,"-- message_delete_queue()");

    // best effort check that no task is waiting on the queue
    if (MessageQueue->waitingTasks != 0)
    {
        WCE_TRACE("message_delete_queue() failed because %d tasks are waiting on %p\n",
                  MessageQueue->waitingTasks, MessageQueue);
        return OS21_FAILURE;
    }

    WCE_ASSERT(CloseHandle(MessageQueue->freeMessagesRead));
    WCE_ASSERT(CloseHandle(MessageQueue->freeMessagesWrite));
    WCE_ASSERT(CloseHandle(MessageQueue->sentMessagesRead));
    WCE_ASSERT(CloseHandle(MessageQueue->sentMessagesWrite));

    for (index = 0; index < MessageQueue->itemNumber; index++)
        free(MessageQueue->messages[index]);
    free(MessageQueue->messages);
    free(MessageQueue);
    return OS21_SUCCESS;
}

void* message_claim(message_queue_t * MessageQueue)
{
    WCE_MSG(MDL_OSCALL,"-- message_claim()");
    return message_claim_timeout(MessageQueue, TIMEOUT_INFINITY);
}

void* message_claim_timeout(message_queue_t *queue, osclock_t * time_end_p)
{
    DWORD dwTicks;
    void* message;
    BOOL ret;
    DWORD sizeRead;
    DWORD flags;

    WCE_MSG(MDL_OSCALL,"-- message_claim_timeout()");

    WCE_ASSERT(queue != NULL);
    if (queue == NULL)
        return NULL;

    // translate OS21 timeout into WinCE timeout
    if (time_end_p == TIMEOUT_IMMEDIATE)
        dwTicks = 0;
    else if (time_end_p == TIMEOUT_INFINITY)
        dwTicks = INFINITE;
    else
    {
        osclock_t now = time_now();
        WCE_ASSERT(time_end_p != NULL);
        if (time_end_p == NULL)
            return NULL;

        if (time_after(*time_end_p, now)) //timeout not yet reached
            dwTicks = (DWORD)_WCE_Tick2Millisecond(time_minus(*time_end_p, now));
        else
            dwTicks = 0;
    }

    WCE_ASSERT(queue->freeMessagesRead != NULL);
    InterlockedIncrement(&queue->waitingTasks);
    ret = ReadMsgQueue(queue->freeMessagesRead, 
                       &message,
                       sizeof(message),
                       &sizeRead,
                       dwTicks,
                       &flags);
    P_ADDSYSTEMCALL(P_READ_QUEUE,0);
    InterlockedDecrement(&queue->waitingTasks);
    if (!ret) // timeout, or destroyed queue
        return NULL;
    WCE_ASSERT(sizeRead == sizeof(message));
    WCE_ASSERT(message != NULL);

    InterlockedDecrement(&queue->free);
    InterlockedIncrement(&queue->claimed);

    return message;
}

void message_send(message_queue_t *queue, void *message)
{
    BOOL ret;
    DWORD sizeRead;
    DWORD flags;

    WCE_MSG(MDL_OSCALL,"-- message_send()");

    WCE_ASSERT(queue != NULL);
    if (queue == NULL)
        return;

    WCE_ASSERT(queue->sentMessagesWrite!= NULL);

    ret = WriteMsgQueue(queue->sentMessagesWrite, &message, sizeof(message), 0, 0);
    WCE_ASSERT(ret);
	P_ADDSYSTEMCALL(P_WRITE_QUEUE,0);

    InterlockedIncrement(&queue->busy);
    InterlockedIncrement(&queue->sent);
}

void *message_receive(message_queue_t* MessageQueue)
{
    WCE_MSG(MDL_OSCALL,"-- message_receive()");
    return message_receive_timeout(MessageQueue, TIMEOUT_INFINITY);
}

void *message_receive_timeout (message_queue_t* queue, osclock_t * ticks)
{
    DWORD dwTicks;
    void* message;
    BOOL ret;
    DWORD sizeRead;
    DWORD flags;

    WCE_MSG(MDL_OSCALL,"-- message_receive_timeout()");

    WCE_ASSERT(queue != NULL);
    if (queue == NULL)
        return NULL;

    // translate OS21 timeout into WinCE timeout
    if (ticks == TIMEOUT_IMMEDIATE)
        dwTicks = 0;
    else if (ticks == TIMEOUT_INFINITY)
        dwTicks = INFINITE;
    else
    {
        osclock_t now = time_now();
        WCE_ASSERT(ticks != NULL);
        if (ticks == NULL)
            return NULL;

        if (time_after(*ticks, now)) //timeout not yet reached
            dwTicks = (DWORD)_WCE_Tick2Millisecond(time_minus(*ticks, now));
        else
            dwTicks = 0;
    }

    WCE_ASSERT(queue->sentMessagesRead != NULL);
    InterlockedIncrement(&queue->waitingTasks);
    ret = ReadMsgQueue(queue->sentMessagesRead, 
                       &message,
                       sizeof(message),
                       &sizeRead,
                       dwTicks,
                       &flags);
    InterlockedDecrement(&queue->waitingTasks);
    if (!ret) // timeout, or destroyed queue
        return NULL;
    WCE_ASSERT(sizeRead == sizeof(message));
    WCE_ASSERT(message != NULL);
	P_ADDSYSTEMCALL(P_READ_QUEUE,0);

    InterlockedDecrement(&queue->busy);
    InterlockedIncrement(&queue->received);

    return message;
}

void message_release(message_queue_t* queue, void* message)
{
    BOOL ret;
    DWORD sizeRead;
    DWORD flags;

    WCE_MSG(MDL_OSCALL,"-- message_release()");

    WCE_ASSERT(queue != NULL);
    if (queue == NULL)
        return;

    WCE_ASSERT(queue->freeMessagesWrite!= NULL);

    ret = WriteMsgQueue(queue->freeMessagesWrite, &message, sizeof(message), 0, 0);
    WCE_ASSERT(ret);
	P_ADDSYSTEMCALL(P_WRITE_QUEUE,0);

    InterlockedIncrement(&queue->free);
    InterlockedIncrement(&queue->released);
}


/*********************************************************************
        U N I T    T E S T S
 *********************************************************************/

#define INVARIANT(Q) WCE_ASSERT(Q != NULL && Q->itemNumber == Q->free + Q->busy + (Q->claimed - Q->sent) + (Q->received - Q->released));

typedef struct
{
    int action; /* 0 = claim+send, 1 = receive+release, 2 = try receiving */
    message_queue_t* queue;
    osclock_t ticks;
} DelayedAction;

static osclock_t startTS;

static void start(void)
{
    startTS = time_now();
}

static osclock_t stop(void)
{
    return time_now() - startTS;
}

static DWORD WAD(LPVOID lpParameter)
{
    DelayedAction* da = (DelayedAction*)lpParameter;
    short* message;

    if (da->action == 0) // send
    {
        task_delay(da->ticks);
        message = message_claim(da->queue);
        WCE_ASSERT(message != NULL);
        *message = 888;
        message_send(da->queue, message);
    }
    else if (da->action == 1)
    {
        task_delay(da->ticks);
        message = message_receive(da->queue);
        WCE_ASSERT(message != NULL);
        WCE_ASSERT(*message == 888);
        message_release(da->queue, message);
    }
    else
    {
        osclock_t timeout = da->ticks;
        message = message_receive_timeout(da->queue, &timeout);
    }

    free(lpParameter);
    return 0;
}

static void WaitAndDo(int action, message_queue_t* queue, osclock_t ticks)
{
    DelayedAction* param;

    param = (DelayedAction*) malloc(sizeof(*param));
    param->action = action;
    param->queue = queue;
    param->ticks = ticks;
    
    CreateThread(NULL, 0, WAD, (LPVOID)param, 0, NULL);
    // yuck: handle is never closed
}

void message_unit_test(void)
{
    message_queue_t* queue = NULL;
    short* message = NULL;
    short* omessage;
    int i;
    osclock_t timeout;

    // basic functionality
    printf("message_unit_test: basic tests...\n");
    queue = message_create_queue(sizeof(short), 20);
    INVARIANT(queue);

    message = message_claim(queue);
    WCE_ASSERT(message != NULL);
    INVARIANT(queue);

    *message = 666;
    message_send(queue, message);
    INVARIANT(queue);
        
    message = NULL;
    message = message_receive(queue);
    INVARIANT(queue);
    WCE_ASSERT(*message == 666);

    message_release(queue, message);
    INVARIANT(queue);

    WCE_ASSERT(message_delete_queue(queue) == OS21_SUCCESS);

    // order is FIFO
    printf("message_unit_test: FIFO check ...\n");

    queue = message_create_queue(sizeof(short), 10);
    for (i = 0; i < 10; i++)
    {
        message = message_claim(queue);
        *message = i;
        message_send(queue, message);
    }
    INVARIANT(queue);
    for (i = 0; i < 10; i++)
    {
        message = message_receive(queue);
        WCE_ASSERT(message != NULL);
        WCE_ASSERT(*message == i);
    }
    INVARIANT(queue);

    // queue full
    printf("message_unit_test: queue full handling ...\n");

    queue = message_create_queue(sizeof(short), 10);
    for (i = 0; i < 10; i++)
        message = message_claim(queue);
    omessage = message;
    WCE_ASSERT(message != NULL);
    INVARIANT(queue);

    message = message_claim_timeout(queue, TIMEOUT_IMMEDIATE);
    WCE_ASSERT(message == NULL);
    INVARIANT(queue);

    message_release(queue, omessage);
    message = message_claim(queue);
    WCE_ASSERT(message != NULL);

    INVARIANT(queue);
    WCE_ASSERT(message_delete_queue(queue) == OS21_SUCCESS);

    // time-outs
    printf("message_unit_test: time-outs...\n");

    queue = message_create_queue(sizeof(short), 10);

    start();
    message = message_receive_timeout(queue, TIMEOUT_IMMEDIATE);
    WCE_ASSERT(message == NULL);
    WCE_ASSERT(stop() < time_ticks_per_sec() / 1000);

    timeout = time_plus(time_now(), time_ticks_per_sec());
    start();
    message = message_receive_timeout(queue, &timeout);
    WCE_ASSERT(message == NULL);
    WCE_ASSERT(stop() >= time_ticks_per_sec());

    WaitAndDo(0, queue, time_ticks_per_sec());
    timeout = time_plus(time_now(), 10 * time_ticks_per_sec());
    start();
    message = message_receive_timeout(queue, &timeout);
    WCE_ASSERT(stop() <= 1.1 * time_ticks_per_sec());
    WCE_ASSERT(*message == 888);
    message_release(queue, message);

    for (i = 0; i < 10; i++)
    {
        message = message_claim(queue);
        *message = 888;
        message_send(queue, message);
    }

    WaitAndDo(1, queue, time_ticks_per_sec());
    timeout = time_plus(time_now(), 10 * time_ticks_per_sec());
    start();
    message = message_claim_timeout(queue, &timeout);
    WCE_ASSERT(stop() <= 1.1 * time_ticks_per_sec());
    message_release(queue, message);

    message_delete_queue(queue);

    // do not delete while task is waiting
    printf("message_unit_test: delete while tasks waiting...\n");

    queue = message_create_queue(sizeof(short), 10);
    WaitAndDo(2, queue, time_ticks_per_sec());
    Sleep(500);
    WCE_ASSERT(message_delete_queue(queue) == OS21_FAILURE);
    Sleep(1000);
    WCE_ASSERT(message_delete_queue(queue) == OS21_SUCCESS);

    printf("message_unit_test: end of test.\n");
}

