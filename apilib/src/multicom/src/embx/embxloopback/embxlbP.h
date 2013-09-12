/*******************************************************************/
/* Copyright 2002 STMicroelectronics R&D Ltd. All rights reserved. */
/*                                                                 */
/* File: embxlbP.h                                                 */
/*                                                                 */
/* Description:                                                    */
/*    EMBX Inprocess loopback transport private structures         */
/*                                                                 */
/*******************************************************************/

#ifndef _EMBXLBP_H
#define _EMBXLBP_H

typedef struct EMBXLB_RecEventList_s EMBXLB_RecEventList_t;

struct EMBXLB_RecEventList_s {
    EMBX_RECEIVE_EVENT     recev;
    EMBXLB_RecEventList_t *next;
};



typedef struct EMBXLB_RecBlockList_s EMBXLB_RecBlockList_t;

struct EMBXLB_RecBlockList_s {
    EMBX_EventState_t     *ev;
    EMBX_RECEIVE_EVENT    *recev;
    EMBXLB_RecBlockList_t *next;
};



typedef struct EMBXLB_RemotePortHandle_s EMBXLB_RemotePortHandle_t;

typedef struct {
    EMBX_LocalPortHandle_t     port; /* Must be first (think C++ base class) */

    EMBXLB_RecEventList_t     *pending_head;
    EMBXLB_RecEventList_t     *pending_tail;

    EMBXLB_RecBlockList_t     *blocked_receivers;

    EMBXLB_RemotePortHandle_t *remote_connections;
} EMBXLB_LocalPortHandle_t;



struct EMBXLB_RemotePortHandle_s {
    EMBX_RemotePortHandle_t    port; /* Must be first (think C++ base class) */

    EMBXLB_LocalPortHandle_t  *destination;

    EMBXLB_RemotePortHandle_t *prevConnection;
    EMBXLB_RemotePortHandle_t *nextConnection;
}; 


#define EMBXLB_HANDLE_TYPE_OBJECT 0x0B000000

typedef struct {
    EMBX_UINT  size;
    EMBX_VOID *data;
} EMBXLB_Object_t;



typedef EMBX_TransportHandle_t EMBXLB_TransportHandle_t;


typedef struct EMBXLB_ConnectBlockList_s EMBXLB_ConnectBlockList_t;

struct EMBXLB_ConnectBlockList_s {
    EMBX_EventState_t         *ev;
    EMBX_CHAR                  portName[EMBX_MAX_PORT_NAME+1];
    EMBX_TransportHandle_t    *requestingHandle;
    EMBX_RemotePortHandle_t  **port;

    EMBXLB_ConnectBlockList_t *next;
    EMBXLB_ConnectBlockList_t *prev;
};



typedef struct {
    EMBX_Transport_t transport; /* Must be first (think C++ base class) */

    EMBX_MUTEX tpLock;
    EMBX_BOOL  bLockInitialized;

    EMBX_UINT  nAllocations;
    EMBX_UINT  maxAllocations;
    EMBX_UINT  portTableSize;

    EMBX_HandleManager_t       objectHandleManager;
    EMBXLB_Object_t           *objectTable;

    EMBXLB_LocalPortHandle_t **portTable;

    EMBXLB_ConnectBlockList_t *connectionRequests;

    EMBX_EventState_t         *initEvent;
    EMBX_EventState_t         *closeEvent;

    EMBX_THREAD                initThread;

} EMBXLB_Transport_t;



extern EMBXLB_Transport_t        _embxlb_transport_template;
extern EMBXLB_Transport_t        _embxlb_blocktransport_template;
extern EMBXLB_TransportHandle_t  _embxlb_transport_handle_template;
extern EMBXLB_LocalPortHandle_t  _embxlb_localport_handle_template;
extern EMBXLB_RemotePortHandle_t _embxlb_remoteport_handle_template;

#endif /* _EMBXLBP_H */
