/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb_transport.c                                        */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport and transport handle       */
/*    method implementations                                       */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embx_debug.h"

#include "embxP.h"

#include "embxlbP.h"

#define EMBXLB_DEFAULT_PORT_TABLE_SIZE 32

/*************************************************************
 * Transport initialization
 */
static EMBX_ERROR do_initialize(EMBXLB_Transport_t *tplb)
{

    if( tplb->transport.transportInfo.maxPorts > 0 )
    {
        tplb->portTableSize = tplb->transport.transportInfo.maxPorts;
    }
    else
    {
        tplb->portTableSize = EMBXLB_DEFAULT_PORT_TABLE_SIZE;
    }

    tplb->portTable = (EMBXLB_LocalPortHandle_t **)EMBX_OS_MemAlloc(tplb->portTableSize*sizeof(EMBXLB_LocalPortHandle_t *));
    if(tplb->portTable == 0)
        goto error_exit;


    memset(tplb->portTable, 0, tplb->portTableSize*sizeof(EMBXLB_LocalPortHandle_t *));

    tplb->objectTable = (EMBXLB_Object_t *)EMBX_OS_MemAlloc(tplb->objectHandleManager.size*sizeof(EMBXLB_Object_t));
    if(tplb->objectTable == 0)
        goto error_exit;


    if(EMBX_HandleManager_Init(&tplb->objectHandleManager) == EMBX_NOMEM)
        goto error_exit;


    return EMBX_SUCCESS;

    /*
     * Manage cleaning up the allocations on error in one place.
     */
error_exit:
    if(tplb->portTable != 0)
    {
        EMBX_OS_MemFree(tplb->portTable);
        tplb->portTable = 0;
    }

    if(tplb->objectTable != 0)
    {
        EMBX_OS_MemFree(tplb->objectTable);
        tplb->objectTable = 0;
    }

    return EMBX_NOMEM;
}



static void init_thread(void *param)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)param;

    /*
     * This is a helper thread to test asynchronous 
     * initialization behaviour in embxshell. It
     * waits for a short period of time (simulating
     * waiting for another CPU to become alive)
     * before continuing with the real initialization.
     */
    EMBX_OS_Delay(5000);

    EMBX_OS_MUTEX_TAKE(&tplb->tpLock);

    if(tplb->initEvent != 0)
    {
        /*
         * a task is still waiting for the initialize
         * so do it.
         */
        tplb->initEvent->result = do_initialize(tplb);

        EMBX_OS_EVENT_POST(&tplb->initEvent->event);

        tplb->initEvent = 0;
    }

    EMBX_OS_MUTEX_RELEASE(&tplb->tpLock);
}



static EMBX_ERROR block_initialize(EMBX_Transport_t *tp, EMBX_EventState_t *ev)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;

    /*
     * We must initialise the transport lock first as it is used
     * by the initialization thread.
     */
    if(!EMBX_OS_MUTEX_INIT(&tplb->tpLock))
        return EMBX_NOMEM;


    tplb->bLockInitialized = EMBX_TRUE;

    tplb->initEvent = ev;
    
    tplb->initThread = EMBX_OS_ThreadCreate(init_thread, (void *)tplb, EMBX_DEFAULT_THREAD_PRIORITY, "loopback-init");

    if(tplb->initThread != EMBX_INVALID_THREAD)
    {
        return EMBX_BLOCK;
    }
    else
    {
        EMBX_DebugMessage(("Unable to create thread\n"));
        return EMBX_SYSTEM_ERROR;
    }
}



static EMBX_ERROR initialize(EMBX_Transport_t *tp, EMBX_EventState_t *ev)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;

    if(!EMBX_OS_MUTEX_INIT(&tplb->tpLock))
        return EMBX_NOMEM;


    tplb->bLockInitialized = EMBX_TRUE;

    return do_initialize(tplb);

}



static EMBX_VOID init_interrupt(EMBX_Transport_t *tp)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;

    /*
     * For this simple transport, zeroing the initialization
     * event pointer is sufficient to cause the initialzation
     * thread to terminate without initializing the transport.
     */

    EMBX_OS_MUTEX_TAKE(&tplb->tpLock);

    tplb->initEvent = 0;

    EMBX_OS_MUTEX_RELEASE(&tplb->tpLock);
}



/*************************************************************
 * Transport closedown
 */
static EMBX_ERROR closedown(EMBX_Transport_t *tp, EMBX_EventState_t *ev)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;
    /*
     * If we had an initialization thread, wait for it to
     * terminate and destroy the thread context. Hopefully
     * it already has a long time ago, but it may still be clearing-up
     * after an init_interrupt call. On Linux the call may get
     * interrupted by an OS signal while waiting for the thread
     * to exit.
     */
    if(EMBX_OS_ThreadDelete(tplb->initThread) == EMBX_SYSTEM_INTERRUPT)
        return EMBX_SYSTEM_INTERRUPT;


    tplb->initThread = EMBX_INVALID_THREAD;

    if(tplb->portTable)
    {
        EMBX_OS_MemFree(tplb->portTable);
        tplb->portTable = 0;
    }

    EMBX_HandleManager_Destroy(&tplb->objectHandleManager);
    
    if(tplb->objectTable)
    {
        EMBX_OS_MemFree(tplb->objectTable);
        tplb->objectTable = 0;
    }

    /*
     * Remember not to destroy transport locks until all threads
     * using them have terminated.
     */
    if(tplb->bLockInitialized)
    {
        EMBX_OS_MUTEX_DESTROY(&tplb->tpLock);
        tplb->bLockInitialized = EMBX_FALSE;
    }

    /*
     * Do not free the tplb structure, this is done at the framework level
     */
    return EMBX_SUCCESS;
}



static EMBX_VOID close_interrupt(EMBX_Transport_t *tp)
{
    /*
     * This transport never returns EMBX_BLOCK from closedown
     * so there is nothing to do here
     */
}



static EMBX_VOID do_invalidate_handle(EMBXLB_TransportHandle_t *th)
{
EMBX_LocalPortList_t *phl;

    /*
     * In this transport we can simply call the invalidate
     * method on each local port. This will implicitly invalidate
     * any remote ports, which must be connected to a local port.
     * It will also wake up any tasks waiting to receive data. Transports
     * involving multiple CPUs may need to do more work to invalidate
     * remote ports connected to a different CPU. 
     */
    phl = th->localPorts;
    while(phl)
    {
        /*
         * Making this call assumes that invalidate_port does not
         * require any transport resources that we already have
         * locked. In this transport that is the case.
         */
        phl->port->methods->invalidate_port(phl->port);
        phl = phl->next;
    }

    th->state = EMBX_HANDLE_INVALID;
}



static EMBX_VOID invalidate(EMBX_Transport_t *tp)
{
EMBXLB_Transport_t         *tplb;
EMBX_TransportHandleList_t *thandles;
EMBXLB_ConnectBlockList_t  *cbl;

    tplb = (EMBXLB_Transport_t *)tp;

    /*
     * First of all wake up any pending connection requests
     */
    cbl = tplb->connectionRequests;
    while(cbl)
    {
    EMBXLB_ConnectBlockList_t *next = cbl->next;

        cbl->ev->result = EMBX_TRANSPORT_INVALIDATED;
        EMBX_OS_EVENT_POST(&cbl->ev->event);

        EMBX_OS_MemFree(cbl);

        cbl = next;
    }

    tplb->connectionRequests = 0;


    /*
     * Now go and deal with still open transport handles
     */
    thandles = tplb->transport.transportHandles;

    while(thandles)
    {
        do_invalidate_handle((EMBXLB_TransportHandle_t *)thandles->transportHandle);

        thandles = thandles->next;
    }

}


/*************************************************************
 * Transport handle API calls
 */
static EMBX_ERROR open_handle(EMBX_Transport_t *tp, EMBX_TransportHandle_t **tph)
{
EMBXLB_TransportHandle_t *handle;

    handle = (EMBXLB_TransportHandle_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_TransportHandle_t));
    if(handle != 0)
    {
        /*
         * Copy default structure contents. This is all we need to do as the
         * framework takes care of the state and transport members. More
         * complicated transports may need to initialize other data members
         * in their subclassed transport handle structure.
         */
        memcpy(handle, &_embxlb_transport_handle_template, sizeof(EMBXLB_TransportHandle_t));
        *tph = (EMBX_TransportHandle_t *)handle;

        EMBX_OS_MODULE_REF();

        return EMBX_SUCCESS;
    }

    return EMBX_NOMEM;
}



static EMBX_VOID interrupt_connection_requests(EMBXLB_Transport_t *tplb, EMBX_TransportHandle_t *tph)
{
EMBXLB_ConnectBlockList_t *cbl;

    cbl = tplb->connectionRequests;
    while(cbl)
    {
    EMBXLB_ConnectBlockList_t *next = cbl->next;

        if(cbl->requestingHandle == tph)
        {
            if(cbl->prev != 0)
                cbl->prev->next = next;
            else
                tplb->connectionRequests = next;


            if(next != 0)
                next->prev = cbl->prev;

            cbl->ev->result = EMBX_TRANSPORT_CLOSED;
            EMBX_OS_EVENT_POST(&cbl->ev->event);

            EMBX_OS_MemFree(cbl);
        }

        cbl = next;
    }
}



static EMBX_ERROR close_handle(EMBX_Transport_t *tp, EMBX_TransportHandle_t *tph)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;

    /*
     * Unblock processes waiting for a connection through the closing handle.
     */
    interrupt_connection_requests(tplb, tph);

    /*
     * Make key handle structures invalid to help catch use after close.
     */
    tph->state     = EMBX_HANDLE_CLOSED;
    tph->transport = 0;

    EMBX_OS_MemFree(tph);

    EMBX_OS_MODULE_UNREF();

    return EMBX_SUCCESS;
}



static EMBX_ERROR alloc_buffer(EMBX_Transport_t *tp, EMBX_UINT size, EMBX_Buffer_t **buf)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;

    /*
     * Check allocation restriction, this is a test mode 
     * for the embxshell framework only.
     */
    if( (tplb->maxAllocations > 0) &&
        (tplb->nAllocations == tplb->maxAllocations) )
    {
        return EMBX_NOMEM;
    }

    *buf = (EMBX_Buffer_t *)EMBX_OS_MemAlloc(size+sizeof(EMBX_Buffer_t));
    if(*buf != 0)
    {
        /*
         * In other transport implementations the size may be adjusted
         * upwards to meet alignment constraints, but this must always
         * be the size that is available for buffer data, it must not
         * include the buffer header or any other heap management metadata.
         */
        (*buf)->size  = size;
        (*buf)->state = EMBX_BUFFER_ALLOCATED;

        tplb->nAllocations++;

        return EMBX_SUCCESS;
    }

    return EMBX_NOMEM;
}



static EMBX_ERROR free_buffer(EMBX_Transport_t *tp, EMBX_Buffer_t *buf)
{
EMBXLB_Transport_t *tplb = (EMBXLB_Transport_t *)tp;

    buf->size  = EMBX_BUFFER_FREE;
    buf->htp   = EMBX_INVALID_HANDLE_VALUE;
    buf->state = EMBX_BUFFER_FREE;

    EMBX_OS_MemFree(buf);

    tplb->nAllocations--;
    
    return EMBX_SUCCESS;
}



static EMBX_ERROR register_object(EMBX_Transport_t *tp, EMBX_VOID *object, EMBX_UINT size, EMBX_HANDLE *handle)
{
EMBXLB_Transport_t *tplb;
EMBXLB_Object_t    *obj;

    tplb = (EMBXLB_Transport_t *)tp;

    *handle = EMBX_HANDLE_CREATE(&tplb->objectHandleManager, 0, EMBXLB_HANDLE_TYPE_OBJECT);
    if(*handle != EMBX_INVALID_HANDLE_VALUE)
    {
        /*
         * Keep a one to one mapping of object descriptors and handle entries
         * this avoids dynamically allocating object descriptors without having
         * to write the handle manager code all over again. C++ templates would
         * have offered a more general solution to this
         */
        obj = &(tplb->objectTable[(*handle) & EMBX_HANDLE_INDEX_MASK]);
        EMBX_HANDLE_ATTACHOBJ(&tplb->objectHandleManager, *handle, obj);

        obj->size = size;
        obj->data = object;

        return EMBX_SUCCESS;
    }

    return EMBX_NOMEM;

}



static EMBX_ERROR deregister_object(EMBX_Transport_t *tp, EMBX_HANDLE handle)
{
EMBXLB_Transport_t *tplb;
EMBXLB_Object_t    *obj;

    tplb = (EMBXLB_Transport_t *)tp;

    obj  = (EMBXLB_Object_t *)EMBX_HANDLE_GETOBJ(&tplb->objectHandleManager, handle);
    if(obj != 0)
    {
        EMBX_HANDLE_FREE(&tplb->objectHandleManager, handle);
        return EMBX_SUCCESS;
    }

    return EMBX_INVALID_ARGUMENT;
}



static EMBX_ERROR get_object(EMBX_Transport_t *tp, EMBX_HANDLE handle, EMBX_VOID **object, EMBX_UINT *size)
{
EMBXLB_Transport_t *tplb;
EMBXLB_Object_t    *obj;

    tplb = (EMBXLB_Transport_t *)tp;

    obj  = (EMBXLB_Object_t *)EMBX_HANDLE_GETOBJ(&tplb->objectHandleManager, handle);
    if(obj != 0)
    {
        *object = obj->data;
        *size   = obj->size;
        return EMBX_SUCCESS;
    }

    return EMBX_INVALID_ARGUMENT;
}


/***************************************************************************
 * Transport structures
 */

static EMBX_TransportMethods_t transport_methods = {
    initialize,
    init_interrupt,
    closedown,
    close_interrupt,
    invalidate,
    open_handle,
    close_handle,
    alloc_buffer,
    free_buffer,
    register_object,
    deregister_object,
    get_object
};

EMBXLB_Transport_t _embxlb_transport_template = {
    {
        &transport_methods,
        { "" , EMBX_FALSE, EMBX_TRUE, EMBX_FALSE }, /* EMBX_TPINFO */
        EMBX_TP_UNINITIALIZED
    } /* EMBX_Transport_t */
};


/* An example of using different transport templates to
 * configure different transport behaviour.
 */
static EMBX_TransportMethods_t blocktransport_methods = {
    block_initialize,
    init_interrupt,
    closedown,
    close_interrupt,
    invalidate,
    open_handle,
    close_handle,
    alloc_buffer,
    free_buffer,
    register_object,
    deregister_object,
    get_object
};

EMBXLB_Transport_t _embxlb_blocktransport_template = {
    {
        &blocktransport_methods,
        { "" , EMBX_FALSE, EMBX_TRUE, EMBX_FALSE }, /* EMBX_TPINFO */
        EMBX_TP_UNINITIALIZED
    } /* EMBX_Transport_t */
};

