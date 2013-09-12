/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb_remoteport.c                                       */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport,remote port implementation */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embxP.h"

#include "embxlbP.h"

/***************************************************************************
 * Remote port handle methods
 */

static EMBX_ERROR close_port(EMBX_RemotePortHandle_t *ph)
{
EMBXLB_RemotePortHandle_t *phlb = (EMBXLB_RemotePortHandle_t *)ph;

    if(phlb->destination != 0)
    {
        /* This port is still connected to a valid destination
         * so remove the port for the destination's remote
         * connection list.
         */
        if(phlb->prevConnection != 0)
            phlb->prevConnection->nextConnection = phlb->nextConnection;
        else
            phlb->destination->remote_connections = phlb->nextConnection;


        if(phlb->nextConnection != 0)
            phlb->nextConnection->prevConnection = phlb->prevConnection;


        phlb->destination    = 0;
        phlb->nextConnection = 0;
        phlb->prevConnection = 0;
    }

    phlb->port.state           = EMBX_HANDLE_CLOSED;
    phlb->port.transportHandle = 0;

    EMBX_OS_MemFree(phlb);

    return EMBX_SUCCESS;
}



static EMBX_ERROR do_send(EMBXLB_RemotePortHandle_t *phlb  ,
                          EMBX_RECEIVE_TYPE          evtype,
                          EMBX_HANDLE                handle,
                          EMBX_VOID                 *data  ,
                          EMBX_UINT                  offset,
                          EMBX_UINT                  size   )
{
    if(phlb->destination->blocked_receivers != 0)
    {
    EMBXLB_RecBlockList_t *receiver;

        /* Pick the first blocked receiver and put the object event info
         * directly into its receive event structure, then signal it to
         * wake up.
         */
        receiver = phlb->destination->blocked_receivers;
        phlb->destination->blocked_receivers = receiver->next;

        receiver->recev->handle = handle;
        receiver->recev->data   = data;
        receiver->recev->offset = offset;
        receiver->recev->size   = size;
        receiver->recev->type   = evtype;

        receiver->ev->result    = EMBX_SUCCESS;
        EMBX_OS_EVENT_POST(&receiver->ev->event);

        EMBX_OS_MemFree(receiver);
    }
    else
    {
    EMBXLB_RecEventList_t *node;

        /* Nobody is waiting for the message so we have to queue it
         * on the destination
         */
        node = (EMBXLB_RecEventList_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_RecEventList_t));
        if(node == 0)
            return EMBX_NOMEM;


        node->recev.handle = handle;
        node->recev.data   = data;
        node->recev.offset = offset;
        node->recev.size   = size;
        node->recev.type   = evtype;
        node->next         = 0;

        if(phlb->destination->pending_head != 0)
        {
            phlb->destination->pending_tail->next = node;
            phlb->destination->pending_tail       = node;
        }
        else
        {
            phlb->destination->pending_tail = node;
            phlb->destination->pending_head = node;
        }
    }

    return EMBX_SUCCESS;
}



static EMBX_ERROR send_message(EMBX_RemotePortHandle_t *ph, EMBX_Buffer_t *buf, EMBX_UINT size)
{
EMBXLB_RemotePortHandle_t *phlb = (EMBXLB_RemotePortHandle_t *)ph;

    return do_send(phlb, EMBX_REC_MESSAGE, EMBX_INVALID_HANDLE_VALUE, buf->data, 0, size);
}



static EMBX_ERROR send_object(EMBX_RemotePortHandle_t *ph, EMBX_HANDLE handle, EMBX_UINT offset, EMBX_UINT size)
{
EMBXLB_Transport_t *tplb;
EMBXLB_Object_t    *obj;

    tplb = (EMBXLB_Transport_t *)ph->transportHandle->transport;

    obj = (EMBXLB_Object_t *)EMBX_HANDLE_GETOBJ(&tplb->objectHandleManager, handle);

    if( (obj != 0) && ((offset+size) <= obj->size) )
    {
    EMBXLB_RemotePortHandle_t *phlb = (EMBXLB_RemotePortHandle_t *)ph;

        return do_send(phlb, EMBX_REC_OBJECT, handle, obj->data, offset, size);
    }
    else
    {
        return EMBX_INVALID_ARGUMENT;
    }
}



static EMBX_ERROR update_object(EMBX_RemotePortHandle_t *ph, EMBX_HANDLE handle, EMBX_UINT offset, EMBX_UINT size)
{
EMBXLB_Transport_t *tplb;
EMBXLB_Object_t    *obj;

    /* As this is a purely shared memory transport the only thing to
     * do is validate that the given object handle is actually a valid
     * object and the offset/size were within the object. 
     */

    tplb = (EMBXLB_Transport_t *)ph->transportHandle->transport;

    obj = (EMBXLB_Object_t *)EMBX_HANDLE_GETOBJ(&tplb->objectHandleManager, handle);

    if( (obj != 0) && ((offset+size) <= obj->size) )
        return EMBX_SUCCESS;
    else
        return EMBX_INVALID_ARGUMENT;
}


/***************************************************************************
 * Remote port handle structures
 */

static EMBX_RemotePortMethods_t remoteport_methods = {
    close_port,
    send_message,
    send_object,
    update_object
};
 
EMBXLB_RemotePortHandle_t _embxlb_remoteport_handle_template = {
    {
        &remoteport_methods,
        EMBX_HANDLE_INVALID
    }
};
