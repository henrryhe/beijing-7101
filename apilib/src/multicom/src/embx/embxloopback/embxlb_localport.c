/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb_localport.c                                        */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport, local port implementation */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embxP.h"

#include "embxlbP.h"


/***************************************************************************
 * Helper routines
 */

static void invalidate_remote_connections(EMBXLB_LocalPortHandle_t *phlb)
{
EMBXLB_RemotePortHandle_t *rpl;

    /* Invalidate all remote connections to the given port */
    rpl = phlb->remote_connections;
    phlb->remote_connections = 0;

    while(rpl != 0)
    {
    EMBXLB_RemotePortHandle_t *next = rpl->nextConnection;

        rpl->port.state     = EMBX_HANDLE_INVALID;
        rpl->destination    = 0;
        rpl->prevConnection = 0;
        rpl->nextConnection = 0;

        rpl = next;
    }
}



static void destroy_event_queue(EMBXLB_LocalPortHandle_t *phlb)
{
EMBXLB_Transport_t    *tplb;
EMBXLB_RecEventList_t *evl;

    /*
     * Destroy any pending event structures and queued message buffers
     */
    tplb = (EMBXLB_Transport_t *)phlb->port.transportHandle->transport;
    evl  = phlb->pending_head;

    phlb->pending_head = 0;
    phlb->pending_tail = 0;

    while(evl != 0)
    {
    EMBXLB_RecEventList_t *next = evl->next;
    
        if(evl->recev.type == EMBX_REC_MESSAGE)
        {
        EMBX_Buffer_t *buf = EMBX_CAST_TO_BUFFER_T(evl->recev.data);

            /* This is convenient for this transport as there is no transport
             * level locking, but you would have to be carefull doing this
             * if free_buffer did need a transport lock that you have already
             * got, ending up in deadlock or a damaged mutex depending on
             * the mutex implementation.
             */
            tplb->transport.methods->free_buffer(&tplb->transport, buf);
        }

        EMBX_OS_MemFree(evl);        
        evl = next;
    }
}



static void wakeup_blocked_receivers(EMBXLB_LocalPortHandle_t *phlb, EMBX_ERROR blockedResult)
{
EMBXLB_RecBlockList_t *rbl;

    /* Wake up blocked receivers with the supplied error code. 
     *
     * In this transport we can directly access the receive block list
     * without any additional locking as all access to it is via API
     * calls which are serialized by the driver lock in the API
     * framework. If this transport had to use another thread to
     * manage delivery then a transport level lock on the block list
     * would be required for all accesses to the list.
     */
    rbl = phlb->blocked_receivers;
    phlb->blocked_receivers = 0;

    while(rbl != 0)
    {
    EMBXLB_RecBlockList_t *next = rbl->next;

        rbl->ev->result = blockedResult;
        EMBX_OS_EVENT_POST(&rbl->ev->event);

        EMBX_OS_MemFree(rbl);
        rbl = next;
    }
}



static void cleanup_port(EMBXLB_LocalPortHandle_t *phlb, EMBX_ERROR blockedResult)
{
unsigned int i;
EMBXLB_Transport_t *tplb;

    tplb = (EMBXLB_Transport_t *)phlb->port.transportHandle->transport;

    invalidate_remote_connections(phlb);

    destroy_event_queue(phlb);

    wakeup_blocked_receivers(phlb, blockedResult);

    /*
     * Invalidate the port reference in the transport's port table
     * this effectively unbinds the port name so any connection
     * attempt will return EMBX_PORT_NOT_BIND. 
     */
    for(i=0;i<tplb->portTableSize;i++)
    {
        if(tplb->portTable[i] == phlb)
        {
            tplb->portTable[i] = 0;
            break;
        }
    }
}


/***************************************************************************
 * Local port handle methods
 */

static EMBX_ERROR invalidate_port(EMBX_LocalPortHandle_t *ph)
{
EMBXLB_LocalPortHandle_t *phlb = (EMBXLB_LocalPortHandle_t *)ph;

    cleanup_port(phlb, EMBX_PORT_INVALIDATED);
    ph->state = EMBX_HANDLE_INVALID;
    
    return EMBX_SUCCESS;
}



static EMBX_ERROR close_port(EMBX_LocalPortHandle_t *ph)
{
EMBXLB_LocalPortHandle_t *phlb = (EMBXLB_LocalPortHandle_t *)ph;

    cleanup_port(phlb, EMBX_PORT_CLOSED);
    ph->state = EMBX_HANDLE_CLOSED;

    EMBX_OS_MemFree(ph);

    return EMBX_SUCCESS;
}



static EMBX_ERROR receive(EMBX_LocalPortHandle_t *ph, EMBX_RECEIVE_EVENT *recev)
{
EMBXLB_LocalPortHandle_t *phlb;
EMBXLB_RecEventList_t    *next;

    phlb = (EMBXLB_LocalPortHandle_t *)ph;

    if(phlb->pending_head == 0)
        return EMBX_INVALID_STATUS;


    *recev = phlb->pending_head->recev;

    next = phlb->pending_head->next;

    EMBX_OS_MemFree(phlb->pending_head);

    phlb->pending_head = next;

    if(next == 0)
        phlb->pending_tail = 0;


    return EMBX_SUCCESS;
}



static EMBX_ERROR receive_block(EMBX_LocalPortHandle_t *ph, EMBX_EventState_t *es, EMBX_RECEIVE_EVENT *recev)
{
EMBXLB_RecBlockList_t    *node;
EMBXLB_LocalPortHandle_t *phlb;

    if(receive(ph, recev) == EMBX_SUCCESS)
        return EMBX_SUCCESS;


    node = (EMBXLB_RecBlockList_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_RecBlockList_t));
    if(node != 0)
    {       
        phlb = (EMBXLB_LocalPortHandle_t *)ph;

        /*
         * In this transport we can directly access the receive block list
         * without any additional locking.
         */
        node->ev    = es;
        node->recev = recev;
        node->next  = phlb->blocked_receivers;

        phlb->blocked_receivers = node;

        return EMBX_BLOCK;
    }

    return EMBX_NOMEM;
}



static EMBX_VOID receive_interrupt(EMBX_LocalPortHandle_t *ph, EMBX_EventState_t *ev)
{
EMBXLB_LocalPortHandle_t *phlb        = (EMBXLB_LocalPortHandle_t *)ph;
EMBXLB_RecBlockList_t    *currentNode = phlb->blocked_receivers;
EMBXLB_RecBlockList_t    *lastNode    = 0;

    /* Find the blocked receiver with the given event state pointer
     * and remove its entry from the list. Its task has just been
     * interrupted by something other than EMBX and is about to
     * disappear from under us.
     *
     * In this transport we can directly access the receive block list
     * without any additional locking.
     */
    while(currentNode)
    {
        if(currentNode->ev == ev)
        {
            if(lastNode != 0)
                lastNode->next = currentNode->next;
            else
                phlb->blocked_receivers = currentNode->next;


            EMBX_OS_MemFree(currentNode);

            return;
        }

        lastNode = currentNode;
        currentNode = currentNode->next;
    }
}



/***************************************************************************
 * Local port handle structures
 */

static EMBX_LocalPortMethods_t localport_methods = {
    invalidate_port,
    close_port,
    receive,
    receive_block,
    receive_interrupt
};


EMBXLB_LocalPortHandle_t  _embxlb_localport_handle_template = {
    {
        &localport_methods,
        EMBX_HANDLE_INVALID
    }
};
