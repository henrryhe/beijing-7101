/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlb_connection.c                                       */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport                            */
/*    transport handle port creation and connection methods        */
/*                                                                 */
/*******************************************************************/

#include "embx_osinterface.h"
#include "embxP.h"
#include "embx_debug.h"

#include "embxlbP.h"

/***************************************************************************
 * Helper functions
 */

static EMBXLB_LocalPortHandle_t *find_port(EMBXLB_Transport_t *tplb, const EMBX_CHAR *portName)
{
unsigned int i;

    for(i=0;i<tplb->portTableSize;i++)
    {
        if( tplb->portTable[i] != 0 &&
            !strcmp(portName, tplb->portTable[i]->port.portName) )
        {
            return tplb->portTable[i];
        }
    }

    return 0;
}



static EMBX_ERROR create_remote_port(EMBXLB_LocalPortHandle_t *localPort, EMBX_RemotePortHandle_t **port)
{
EMBXLB_RemotePortHandle_t *remotePort;

    remotePort = (EMBXLB_RemotePortHandle_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_RemotePortHandle_t));
    if(remotePort != 0)
    {
        memcpy(remotePort, &_embxlb_remoteport_handle_template, sizeof(EMBXLB_RemotePortHandle_t));

        remotePort->destination    = localPort;
        remotePort->nextConnection = localPort->remote_connections;

        if(localPort->remote_connections != 0)
            localPort->remote_connections->prevConnection = remotePort;


        localPort->remote_connections = remotePort;

        *port = &remotePort->port;
        return EMBX_SUCCESS;
    }

    *port = 0;
    return EMBX_NOMEM;
}



static void make_connections(EMBXLB_Transport_t *tplb, EMBXLB_LocalPortHandle_t *phlb, const EMBX_CHAR *portName)
{
EMBXLB_ConnectBlockList_t *cbl = tplb->connectionRequests;
EMBX_BOOL bRefuseConnection    = EMBX_FALSE;

    /* In this transport we can directly access the connect block list,
     * without any additional locking, as all access to it is via API
     * calls which are serialized by the driver lock in the API
     * framework. If this transport had to use another thread to
     * manage connections then a transport level lock on the block list
     * would be required for all accesses to the list.
     */
    while(cbl != 0)
    {
    EMBXLB_ConnectBlockList_t *next = cbl->next;
        
        /* We cannot use the phlb->port.portName member yet as it
         * hasn't been initialized.
         */
        if(!strcmp(portName, cbl->portName))
        {

            if(bRefuseConnection)
            {
                cbl->ev->result = EMBX_CONNECTION_REFUSED;
            }
            else
            {
                cbl->ev->result = create_remote_port(phlb, cbl->port);

                /* If we only support single connections then we must refuse any other
                 * connection request for the same port name, assuming the 
                 * connection we just tried succeeded.
                 */
                if( cbl->ev->result == EMBX_SUCCESS &&
                    !tplb->transport.transportInfo.allowsMultipleConnections )
                {
                    bRefuseConnection = EMBX_TRUE;
                }
            } 
        
            EMBX_OS_EVENT_POST(&cbl->ev->event);

            if(cbl->prev != 0)
                cbl->prev->next = next;
            else
                tplb->connectionRequests = next;

                
            if(next != 0)
                next->prev = cbl->prev;

                
            EMBX_OS_MemFree(cbl);
            
        }

        cbl = next;          
    }
}



static EMBX_ERROR find_free_port_table_entry(EMBXLB_Transport_t *tplb, const EMBX_CHAR *portName, EMBX_INT *freeSlot)
{
unsigned int i;
EMBX_BOOL bFound = EMBX_FALSE;

    /*
     * Search for a free port table entry. While we are going through
     * the table anyway then make sure the port name provided is
     * unique.
     */
    for(i=0;i<tplb->portTableSize;i++)
    {
        if(tplb->portTable[i] == 0)
        {
            if(!bFound)
            {
                *freeSlot = i;
                bFound = EMBX_TRUE;
            }
        }
        else
        {
            if(!strcmp(portName, tplb->portTable[i]->port.portName))
            {
                /* Port name has already been used in this transport */
                return EMBX_ALREADY_BIND;
            }
        }
    }

    return bFound?EMBX_SUCCESS:EMBX_NOMEM;
}



static EMBX_ERROR grow_port_table(EMBXLB_Transport_t *tplb, EMBX_INT *freeSlot)
{
EMBXLB_LocalPortHandle_t **newTable;
EMBX_UINT                  newTableSize;
EMBX_UINT                  nBytes;

    *freeSlot = tplb->portTableSize;

    newTableSize = tplb->portTableSize * 2;

    if(newTableSize < tplb->portTableSize)
    {
        EMBX_DebugMessage(("EMBXLB::grow_port_table 'table size overflowed!!!'\n"));
        return EMBX_NOMEM;
    }


    nBytes   = newTableSize*sizeof(EMBXLB_LocalPortHandle_t *);
    newTable = (EMBXLB_LocalPortHandle_t **)EMBX_OS_MemAlloc(nBytes);
    if(newTable != 0)
    {
        memset(newTable, 0, nBytes);
        memcpy(newTable, tplb->portTable, tplb->portTableSize*sizeof(EMBXLB_LocalPortHandle_t *));

        EMBX_OS_MemFree(tplb->portTable);

        tplb->portTable     = newTable;
        tplb->portTableSize = newTableSize;

        EMBX_DebugMessage(("EMBXLB::grow_port_table 'new port table size = %u'\n", newTableSize));

        return EMBX_SUCCESS;
    }

    EMBX_DebugMessage(("EMBXLB::grow_port_table 'could not grow port table'\n"));
    return EMBX_NOMEM;
}



/***************************************************************************
 * Transport handle methods
 */

static EMBX_ERROR create_port(EMBX_TransportHandle_t *tp, const EMBX_CHAR *portName, EMBX_LocalPortHandle_t **port)
{
EMBX_ERROR res;
EMBX_INT   freeSlot;
EMBXLB_LocalPortHandle_t *phlb;
EMBXLB_Transport_t       *tplb;


    tplb = (EMBXLB_Transport_t *)tp->transport;

    res = find_free_port_table_entry(tplb, portName, &freeSlot);
    
    switch (res)
    {
        case EMBX_SUCCESS:
        {
            break;
        } /* EMBX_SUCCESS */
        case EMBX_NOMEM:
        {
            /*
             * If we have got to this point then there is no fixed
             * transport limit to number of ports (this is checked
             * in the shell) so try and grow the port table.
             */
            if(grow_port_table(tplb, &freeSlot) != EMBX_SUCCESS)
                return EMBX_NOMEM;


            break;
        } /* EMBX_NOMEM */
        default:
        {
            return res;
        }
    }


    phlb = (EMBXLB_LocalPortHandle_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_LocalPortHandle_t));
    if(phlb != 0)
    {
        memcpy(phlb, &_embxlb_localport_handle_template, sizeof(EMBXLB_LocalPortHandle_t));

        tplb->portTable[freeSlot] = phlb;
        *port = &phlb->port;

        /*
         * The transportHandle,port name and state members of the local port base
         * structure are filled in by the framework.
         */

        /*
         * Finally, make connections for any blocked connection requests waiting
         * for this new port name.
         */
        make_connections(tplb, phlb, portName);

        return EMBX_SUCCESS;
    }

    return EMBX_NOMEM;
}



static EMBX_ERROR connect_async(EMBX_TransportHandle_t *tp, const EMBX_CHAR *portName, EMBX_RemotePortHandle_t **port)
{
EMBXLB_LocalPortHandle_t *localPort;
EMBXLB_Transport_t       *tplb;


    tplb = (EMBXLB_Transport_t *)tp->transport;

    localPort = find_port(tplb, portName);

    if(localPort == 0)
        return EMBX_PORT_NOT_BIND;


    if( !tplb->transport.transportInfo.allowsMultipleConnections &&
        localPort->remote_connections != 0 )
    {
        return EMBX_CONNECTION_REFUSED;
    }

    return create_remote_port(localPort, port);
}



static EMBX_ERROR connect_block(EMBX_TransportHandle_t *tp, const EMBX_CHAR *portName, EMBX_EventState_t *es, EMBX_RemotePortHandle_t **port)
{
EMBX_ERROR res;

    res = connect_async(tp,portName,port);
    if(res == EMBX_PORT_NOT_BIND)
    {
    EMBXLB_ConnectBlockList_t *node;
    EMBXLB_Transport_t        *tplb;

        /* Portname not found so try and block the task until it is created */
        node = (EMBXLB_ConnectBlockList_t *)EMBX_OS_MemAlloc(sizeof(EMBXLB_ConnectBlockList_t));
        if(node == 0)
            return EMBX_NOMEM;


        strcpy(node->portName, portName);

        /* In this transport we can directly access the connect block list
         * without any additional locking.
         */
        tplb = (EMBXLB_Transport_t *)tp->transport;

        node->ev   = es;
        node->port = port;
        node->requestingHandle = tp; /* Needed for close_handle to clean up its blocked connections */
        node->prev = 0;
        node->next = tplb->connectionRequests;
        
        if(node->next != 0)
            node->next->prev = node;


        tplb->connectionRequests = node;

        return EMBX_BLOCK;
    }
    else
    {
        return res;
    }
}



static EMBX_VOID connect_interrupt(EMBX_TransportHandle_t *tp, EMBX_EventState_t *es)
{
EMBXLB_Transport_t        *tplb = (EMBXLB_Transport_t *)tp->transport;
EMBXLB_ConnectBlockList_t *cpl  = tplb->connectionRequests;

    /* In this transport we can directly access the connect block list
     * without any additional locking.
     */
    while(cpl != 0)
    {
    EMBXLB_ConnectBlockList_t *next = cpl->next;

        if(cpl->ev == es)
        {
            /* Found the interrupted entry, so unhook it from the blocked list
             * and destroy the structure. This can be the only entry with this
             * eventState pointer, so return immediately.
             */
            if(cpl->prev != 0)
                cpl->prev->next = next;
            else
                tplb->connectionRequests = next;


            if(next != 0)
                next->prev = cpl->prev;


            EMBX_OS_MemFree(cpl);

            return;
        }

        cpl = next;
    }
}


/***************************************************************************
 * Transport handle structures
 */

static EMBX_TransportHandleMethods_t transport_handle_methods = {
    create_port,
    connect_async, /* the global name 'connect' clashes with windows headers */
    connect_block,
    connect_interrupt
};

EMBXLB_TransportHandle_t _embxlb_transport_handle_template = {
    &transport_handle_methods,
    EMBX_HANDLE_INVALID
};
