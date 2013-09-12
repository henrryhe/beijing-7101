/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_ioctl
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description :
 *
 *****************************************************************************/
#ifndef STPTI4_IOCTL_H
#define STPTI4_IOCTL_H

#include <linux/ioctl.h>   /* Defines macros for ioctl numbers */

#include "stpti.h"

/*** IOCtl defines ***/

#if !defined(STPTI_IOCTL_MAGIC_NUMBER)
#define STPTI_IOCTL_MAGIC_NUMBER    STPTI_DRIVER_ID      /* Unique module id - See 'ioctl-number.txt' */
#endif

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t          ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STPTI_InitParams_t      InitParams;
    int                     ForceLoader;
} STPTI_Ioctl_Init_t;

#define STPTI_IOC_INIT                    _IOWR(STPTI_IOCTL_MAGIC_NUMBER, 0, STPTI_Ioctl_Init_t*)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STPTI_TermParams_t      TermParams;
} STPTI_Ioctl_Term_t;

#define STPTI_IOC_TERM               _IOWR(STPTI_IOCTL_MAGIC_NUMBER, 1, STPTI_Ioctl_Term_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    ST_DeviceName_t         DeviceName;
    STPTI_OpenParams_t      OpenParams;
    STPTI_Handle_t          Handle;
} STPTI_Ioctl_Open_t;

#define STPTI_IOC_OPEN               _IOWR(STPTI_IOCTL_MAGIC_NUMBER, 2, STPTI_Ioctl_Open_t *)

typedef struct
{
    /* Error code retrieved by STAPI function */
    ST_ErrorCode_t  ErrorCode;

    /* Parameters to the function */
    STPTI_Handle_t         Handle;
} STPTI_Ioctl_Close_t;

#define STPTI_IOC_CLOSE              _IOWR(STPTI_IOCTL_MAGIC_NUMBER, 3, STPTI_Ioctl_Close_t *)

typedef struct{
    STPTI_Handle_t SessionHandle;     /* In */
    STPTI_Slot_t SlotHandle;          /* Out . If no error*/
    STPTI_SlotData_t SlotData;        /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotAllocate_t;
#define STPTI_IOC_SLOTALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 8, STPTI_Ioctl_SlotAllocate_t* )

typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotDeallocate_t;
#define STPTI_IOC_SLOTDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 9, STPTI_Ioctl_SlotDeallocate_t* )

/*
ST_ErrorCode_t STPTI_SlotSetPid( STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid )
*/
typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    STPTI_Pid_t Pid;                  /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotSetPid_t;
#define STPTI_IOC_SLOTSETPID _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 10, STPTI_Ioctl_SlotSetPid_t* )

typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotClearPid_t;
#define STPTI_IOC_SLOTCLEARPID _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 11, STPTI_Ioctl_SlotClearPid_t* )

typedef struct{
    STPTI_Handle_t SessionHandle;       /* In */
    U32 RequiredSize;                   /* In */
    U32 NumberOfPacketsInMultiPacket;   /* In */
    STPTI_Buffer_t BufferHandle;        /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferAllocate_t;
#define STPTI_IOC_BUFFERALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 12, STPTI_Ioctl_BufferAllocate_t* )

typedef struct{
    STPTI_Buffer_t BufferHandle;        /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferDeallocate_t;
#define STPTI_IOC_BUFFERDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 13, STPTI_Ioctl_BufferDeallocate_t* )

typedef struct{
    STPTI_Slot_t Slot;                  /* In */
    STPTI_Buffer_t BufferHandle;        /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SlotLinkToBuffer_t;
#define STPTI_IOC_SLOTLINKTOBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 14, STPTI_Ioctl_SlotLinkToBuffer_t* )

typedef struct{
    STPTI_Slot_t Slot;                  /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SlotUnLink_t;
#define STPTI_IOC_SLOTUNLINK _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 15, STPTI_Ioctl_SlotUnLink_t* )

typedef struct{
    STPTI_Buffer_t BufferHandle;        /* In */
    STPTI_DMAParams_t CdFifoParams;     /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferLinkToCdFifo_t;
#define STPTI_IOC_BUFFERLINKTOCDFIFO _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 16, STPTI_Ioctl_BufferLinkToCdFifo_t* )

typedef struct{
    STPTI_Buffer_t BufferHandle;        /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferUnLink_t;

#define STPTI_IOC_BUFFERUNLINK _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 17, STPTI_Ioctl_BufferUnLink_t* )

/*
ST_ErrorCode_t STPTI_EnableErrorEvent( ST_DeviceName_t DeviceName,
                                       STPTI_Event_t EventName )
*/
typedef struct{
    ST_DeviceName_t     DeviceName; /* In */
    STPTI_Event_t       EventName;  /* In */
    ST_ErrorCode_t      Error;      /* Out */
} STPTI_Ioctl_ErrorEvent_t;

#define STPTI_IOC_ENABLEERROREVENT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 18,STPTI_Ioctl_ErrorEvent_t* )
/*
ST_ErrorCode_t STPTI_DisableErrorEvent( ST_DeviceName_t DeviceName,
                                        STPTI_Event_t EventName )
*/
#define STPTI_IOC_DISABLEERROREVENT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 19,STPTI_Ioctl_ErrorEvent_t* )

/*
ST_ErrorCode_t STPTI_GetSWCDFifoCfg(void** GetWritePtrFn, void** SetReadPtrFn)
*/

typedef struct{
    void **GetWritePtrFn ;               /* Out */
    void **SetReadPtrFn;                 /* Out */
    ST_ErrorCode_t      Error;          /* Out */
} STPTI_Ioctl_GetSWCDFifoCfg_t;

#define STPTI_IOC_GETSWCDFIFOCFG _IOR( STPTI_IOCTL_MAGIC_NUMBER, 20, STPTI_Ioctl_GetSWCDFifoCfg_t *)

typedef struct{
    STPTI_Handle_t SessionHandle;       /* In */
    U8 *Base_p;                         /* In */
    U32 RequiredSize;                   /* In */
    U32 NumberOfPacketsInMultiPacket;   /* In */
    STPTI_Buffer_t BufferHandle;        /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferAllocateManual_t;
#define STPTI_IOC_BUFFERALLOCATEMANUAL _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 21, STPTI_Ioctl_BufferAllocateManual_t* )

/*
ST_ErrorCode_t STPTI_BufferSetOverflowControl(STPTI_Buffer_t BufferHandle, BOOL IgnoreOverflow)
*/
typedef struct{
    STPTI_Handle_t BufferHandle;       /* In */
    BOOL IgnoreOverflow;               /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferSetOverflowControl_t;

#define STPTI_IOC_BUFFERSETOVERFLOWCONTROL _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 22, STPTI_Ioctl_BufferSetOverflowControl_t* )

/*
ST_ErrorCode_t STPTI_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    U16 Count;                         /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_GetInputPacketCount_t;

#define STPTI_IOC_GETINPUTPACKETCOUNT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 23, STPTI_Ioctl_GetInputPacketCount_t* )

/*
ST_ErrorCode_t STPTI_SlotPacketCount(STPTI_Slot_t SlotHandle, U16 *Count)
*/
typedef struct{
    STPTI_Slot_t SlotHandle;            /* In */
    U16 Count;                         /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SlotPacketCount_t;

#define STPTI_IOC_SLOTPACKETCOUNT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 24, STPTI_Ioctl_SlotPacketCount_t* )

/*
ST_Revision_t STPTI_GetRevision(void)
*/
/* Must be called with an array that is >=31 bytes */
#define STPTI_IOC_GETREVISION _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 25, char* )

/*
ST_ErrorCode_t STPTI_SubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID );
*/
typedef struct{
    STPTI_Handle_t SessionHandle;      /* In */
    STPTI_Event_t Event_ID;            /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SubscribeEvent_t;
#define STPTI_IOC_SUBSCRIBEEVENT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 26, STPTI_Ioctl_SubscribeEvent_t* )

/*
ST_ErrorCode_t STPTI_UnsubscribeEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID );
*/
typedef struct{
    STPTI_Handle_t SessionHandle;      /* In */
    STPTI_Event_t Event_ID;            /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_UnsubscribeEvent_t;
#define STPTI_IOC_UNSUBSCRIBEEVENT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 27, STPTI_Ioctl_UnsubscribeEvent_t* )

/*
ST_ErrorCode_t STPTI_SendEvent(STPTI_Handle_t Session, STPTI_Event_t Event_ID, U32 EventDataSize, void *EventData );
*/
typedef struct{
    STPTI_Handle_t SessionHandle;      /* In */
    STPTI_Event_t Event_ID;            /* In */
    STPTI_EventData_t EventData;       /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SendEvent_t;
#define STPTI_IOC_SENDEVENT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 28, STPTI_Ioctl_SendEvent_t* )

/*
ST_ErrorCode_t STPTI_WaitFotEvent(STPTI_Handle_t Session, STPTI_Event_t *Event_ID, U32 EventDataSize, void *EventData );
*/
typedef struct{
    STPTI_Handle_t SessionHandle;      /* In */
    STPTI_Event_t Event_ID;            /* In */
    STPTI_EventData_t EventData;       /* Out */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_WaitForEvent_t;
#define STPTI_IOC_WAITFOREVENT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 29, STPTI_Ioctl_WaitForEvent_t* )

/*
void STPTI_TEST_ResetAllPti();
*/
#define STPTI_IOC_TEST_RESETALLPTI _IOW( STPTI_IOCTL_MAGIC_NUMBER, 30, void* )

/*
ST_ErrorCode_t STPTI_SignalAllocate(STPTI_Handle_t Handle, STPTI_Signal_t * SignalHandle_p);
*/
typedef struct{
    STPTI_Handle_t SessionHandle;      /* In */
    STPTI_Signal_t SignalHandle;       /* Out */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SignalAllocate_t;
#define STPTI_IOC_SIGNALALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 31, STPTI_Ioctl_SignalAllocate_t* )

/*
ST_ErrorCode_t STPTI_SignalAssociateBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t BufferHandle);
*/
typedef struct{
    STPTI_Signal_t SignalHandle;      /* In */
    STPTI_Buffer_t BufferHandle;       /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SignalAssociateBuffer_t;
#define STPTI_IOC_SIGNALASSOCIATEBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 32, STPTI_Ioctl_SignalAssociateBuffer_t* )

/*
ST_ErrorCode_t STPTI_SignalDeallocate(STPTI_Signal_t SignalHandle);
*/
typedef struct{
    STPTI_Signal_t SignalHandle;       /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SignalDeallocate_t;
#define STPTI_IOC_SIGNALDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 33, STPTI_Ioctl_SignalDeallocate_t* )

/*
ST_ErrorCode_t STPTI_SignalDisassociateBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t BufferHandle);
*/
typedef struct{
    STPTI_Signal_t SignalHandle;      /* In */
    STPTI_Buffer_t BufferHandle;       /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SignalDisassociateBuffer_t;
#define STPTI_IOC_SIGNALDISASSOCIATEBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 34, STPTI_Ioctl_SignalDisassociateBuffer_t* )

/*
ST_ErrorCode_t STPTI_SignalWaitBuffer(STPTI_Signal_t SignalHandle, STPTI_Buffer_t * BufferHandle_p, U32 TimeoutMS);
*/
typedef struct{
    STPTI_Signal_t SignalHandle;      /* In */
    STPTI_Buffer_t BufferHandle;       /* Out */
    U32 TimeoutMS;                      /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SignalWaitBuffer_t;
#define STPTI_IOC_SIGNALWAITBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 35, STPTI_Ioctl_SignalWaitBuffer_t* )

/*
ST_ErrorCode_t STPTI_SignalAbort( STPTI_Signal_t SignalHandle )
*/
typedef struct{
    STPTI_Signal_t SignalHandle;      /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_SignalAbort_t;
#define STPTI_IOC_SIGNALABORT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 36, STPTI_Ioctl_SignalAbort_t* )

/*
ST_ErrorCode_t STPTI_SlotQuery( STPTI_Slot_t SlotHandle,
                                BOOL *PacketsSeen,
                                BOOL *TransportScrambledPacketsSeen,
                                BOOL *PESScrambledPacketsSeen,
                                STPTI_Pid_t *Pid )
*/
typedef struct{
    STPTI_Slot_t SlotHandle;    /* In */
    BOOL PacketsSeen;           /* Out */
    BOOL TransportScrambledPacketsSeen;     /* Out */
    BOOL PESScrambledPacketsSeen;           /* Out */
    STPTI_Pid_t Pid;            /* Out */
    ST_ErrorCode_t Error;       /* Out */
} STPTI_Ioctl_SlotQuery_t;
#define STPTI_IOC_SLOTQUERY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 37, STPTI_Ioctl_SlotQuery_t* )
/*
ST_ErrorCode_t STPTI_BufferFlush( STPTI_Buffer_t BufferHandle )
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;    /* In */
    ST_ErrorCode_t Error;           /* Out */
} STPTI_Ioctl_BufferFlush_t;
#define STPTI_IOC_BUFFERFLUSH _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 38, STPTI_Ioctl_BufferFlush_t* )

/*
ST_ErrorCode_t STPTI_BufferGetFreeLength(STPTI_Buffer_t BufferHandle, U32 *FreeLength_p)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;    /* In */
    U32 FreeLength;                 /* Out */
    ST_ErrorCode_t Error;
} STPTI_Ioctl_BufferGetFreeLength_t;
#define STPTI_IOC_BUFFERGETFREELENGTH _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 39, STPTI_Ioctl_BufferGetFreeLength_t* )
/*
ST_ErrorCode_t STPTI_BufferReadPes(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                   U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle; /* In */
    U8 *Destination0_p;          /* In */
    U32 DestinationSize0;        /* In */
    U8 *Destination1_p;          /* In */
    U32 DestinationSize1;        /* In */
    U32 DataSize;               /* Out */
    STPTI_Copy_t DmaOrMemcpy;    /* In */
    ST_ErrorCode_t Error;        /* Out */
} STPTI_Ioctl_BufferReadPes_t;
#define STPTI_IOC_BUFFERREADPES _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 40, STPTI_Ioctl_BufferReadPes_t* )

/*
ST_ErrorCode_t STPTI_BufferTestForData(STPTI_Buffer_t BufferHandle, U32 *BytesInBuffer)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;
    U32 BytesInBuffer;
    ST_ErrorCode_t Error;
} STPTI_Ioctl_BufferTestForData_t;
#define STPTI_IOC_BUFFERTESTFORDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 41, STPTI_Ioctl_BufferTestForData_t* )

/*
ST_ErrorCode_t STPTI_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t *DeviceCapability)
*/
typedef struct{
    ST_DeviceName_t     DeviceName;             /* In */
    STPTI_Capability_t  DeviceCapability;       /* Out */
    ST_ErrorCode_t      Error;                  /* Out */
} STPTI_Ioctl_GetCapability_t;

#define STPTI_IOC_GETCAPABILITY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 42,STPTI_Ioctl_GetCapability_t* )

/*
ST_ErrorCode_t STPTI_FilterAllocate( STPTI_Handle_t Handle,
                                     STPTI_FilterType_t FilterType,
                                     STPTI_Filter_t * FilterObject )
*/
typedef struct{
    STPTI_Handle_t SessionHandle;       /* In */
    STPTI_FilterType_t FilterType;      /* In */
    STPTI_Filter_t FilterObject;      /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_FilterAllocate_t;
#define STPTI_IOC_FILTERALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 43, STPTI_Ioctl_FilterAllocate_t* )

/*
ST_ErrorCode_t STPTI_FilterAssociate( STPTI_Filter_t FilterHandle,
                                      STPTI_Slot_t SlotHandle )
*/
typedef struct{
    STPTI_Filter_t FilterHandle;        /* In */
    STPTI_Slot_t SlotHandle;            /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_FilterAssociate_t;
#define STPTI_IOC_FILTERASSOCIATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 44, STPTI_Ioctl_FilterAssociate_t* )

/*
STPTI_FilterDeallocate( STPTI_Filter_t FilterHandle )
*/
typedef struct{
    STPTI_Filter_t FilterHandle;       /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_FilterDeallocate_t;
#define STPTI_IOC_FILTERDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 45, STPTI_Ioctl_FilterDeallocate_t* )

/*
ST_ErrorCode_t STPTI_FilterDisassociate( STPTI_Filter_t FilterHandle,
                                         STPTI_Slot_t SlotHandle )
*/
typedef struct{
    STPTI_Filter_t FilterHandle;        /* In */
    STPTI_Slot_t SlotHandle;            /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_FilterDisassociate_t;
#define STPTI_IOC_FILTERDISASSOCIATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 46, STPTI_Ioctl_FilterDisassociate_t* )

/*
ST_ErrorCode_t STPTI_FilterSet( STPTI_Filter_t FilterHandle,
                                STPTI_FilterData_t *FilterData_p )
*/
typedef struct{
    STPTI_Filter_t FilterHandle;       /* In */
    STPTI_FilterData_t FilterData;  /* In/out ? */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_FilterSet_t;
#define STPTI_IOC_FILTERSET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 47, STPTI_Ioctl_FilterSet_t* )

/******************************************************************************
Function Name : STPTI_BufferReadNBytes
  Description : A function to move n bytes from the contents of an internal
                buffer to a user-supplied buffer.
   Parameters : BufferHandle        Handle to a buffer.
                Destination0_p      Base of the linear buffer that the data will be copied into.
                DestinationSize0    Size of the linear buffer.
                Destination1_p      Base of the second buffer that will be copied into.
                                    Used if destination is a circular buffer.
                                    Set to NULL for basic linear copy.
                DestinationSize1    Size of second buffer
                DataSize_p          Amount of data actually moved
                DmaOrMemcpy         Request transfer to be performed by DMA or ST20
                BytesToCopy         Number of bytes to read from the internal circular buffer.
         Note : Moved here from pti_basic.c following DDTS 18257
******************************************************************************/
/*
ST_ErrorCode_t STPTI_BufferReadNBytes(STPTI_Buffer_t BufferHandle,
                                      U8 *Destination0_p, U32 DestinationSize0,
                                      U8 *Destination1_p, U32 DestinationSize1,
                                      U32 *DataSize_p,
                                      STPTI_Copy_t DmaOrMemcpy,
                                      U32 BytesToCopy)

Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY
*/

typedef struct{
    STPTI_Buffer_t BufferHandle; /* In */
    U8 *Destination0_p;          /* In */
    U32 DestinationSize0;        /* In */
    U8 *Destination1_p;          /* In */
    U32 DestinationSize1;        /* In */
    U32 DataSize;               /* Out */
    STPTI_Copy_t DmaOrMemcpy;    /* In */
    U32 BytesToCopy;             /* In */
    ST_ErrorCode_t Error;        /* Out */
} STPTI_Ioctl_BufferReadNBytes_t;
#define STPTI_IOC_BUFFERREADNBYTES _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 48, STPTI_Ioctl_BufferReadNBytes_t* )

/******************************************************************************
Function Name : STPTI_SlotSetCCControl
  Description :
   Parameters : SetOnOff - when TRUE   CC checking will be disabled.
                         - when FALSE  CC checking will be enabled.
******************************************************************************/
/*
ST_ErrorCode_t STPTI_SlotSetCCControl(STPTI_Slot_t SlotHandle, BOOL SetOnOff)
*/
typedef struct{
    STPTI_Slot_t SlotHandle;    /* In */
    BOOL SetOnOff;              /* In */
    ST_ErrorCode_t Error;       /* Out */
} STPTI_Ioctl_SlotSetCCControl_t;
#define STPTI_IOC_SLOTSETCCCONTROL _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 49, STPTI_Ioctl_SlotSetCCControl_t* )


/******************************************************************************
Function Name : STPTI_BufferReadSection
  Description :
   Parameters :
******************************************************************************/
/*
ST_ErrorCode_t STPTI_BufferReadSection( STPTI_Buffer_t BufferHandle,
                                        STPTI_Filter_t MatchedFilterList[],
                                        U32 MaxLengthofFilterList,
                                        U32 *NumOfFilterMatches_p,
                                        BOOL *CRCValid_p,
                                        U8 *Destination0_p,
                                        U32 DestinationSize0,
                                        U8 *Destination1_p,
                                        U32 DestinationSize1,
                                        U32 *DataSize_p,
                                        STPTI_Copy_t DmaOrMemcpy )
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;            /* In */
    U32 MaxLengthofFilterList;              /* In */
    U32 NumOfFilterMatches;              /* Out */
    BOOL CRCValid;                       /* Out */
    U8 *Destination0_p;          /* In */
    U32 DestinationSize0;        /* In */
    U8 *Destination1_p;          /* In */
    U32 DestinationSize1;        /* In */
    U32 DataSize;               /* Out */
    STPTI_Copy_t DmaOrMemcpy;    /* In */
    ST_ErrorCode_t Error;        /* Out */
    STPTI_Filter_t *MatchedFilterList;      /* In */
} STPTI_Ioctl_BufferReadSection_t;
#define STPTI_IOC_BUFFERREADSECTION _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 50, STPTI_Ioctl_BufferReadSection_t* )

/*
ST_ErrorCode_t STPTI_WaitEventList( STPTI_Handle_t Handle,
                                  U32 NbEvents,
                                  U32 *Events_p,
                                  U32 *Event_That_Has_Occurred,
                                  U32 EventDataSize,
                                  void *EventData_p );
*/
typedef struct{
    STPTI_Handle_t SessionHandle;
    U32 NbEvents;
    U32 Event_That_Has_Occurred;
    U32 EventDataSize; /* Size of Event data pointed to by EventData_p */
    ST_ErrorCode_t Error;        /* Out */
    U8  FuncData[]; /* FuncData will point to first event in event List.
                       FuncData + (4*NbEvents) points to first bytes of returned event data area */
} STPTI_Ioctl_WaitEventList_t;

#define STPTI_IOC_WAITEVENTLIST _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 51, STPTI_Ioctl_WaitEventList_t* )

typedef struct {
 STPTI_Handle_t          PtiHandle;
 STPTI_Descrambler_t     DescramblerObject;
 STPTI_DescramblerType_t DescramblerType;
 ST_ErrorCode_t          Error;
}STPTI_DescAllocate_t;
#define STPTI_IOC_DESCALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 52, STPTI_DescAllocate_t* )


typedef struct {
 STPTI_Descrambler_t DescramblerHandle;
 STPTI_SlotOrPid_t   SlotOrPid;
 ST_ErrorCode_t      Error;
}STPTI_DescAssociate_t;
#define STPTI_IOC_DESCASSOCIATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 53, STPTI_DescAssociate_t* )


typedef struct {
 STPTI_Descrambler_t DescramblerHandle;
 ST_ErrorCode_t      Error;
}STPTI_DescDeAllocate_t;
#define STPTI_IOC_DESCDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 54, STPTI_DescDeAllocate_t* )

typedef struct {
 STPTI_Descrambler_t DescramblerHandle;
 STPTI_SlotOrPid_t   SlotOrPid;
 ST_ErrorCode_t      Error;
}STPTI_DescDisAssociate_t;
#define STPTI_IOC_DESCDISASSOCIATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 55, STPTI_DescDisAssociate_t* )

typedef struct {
 STPTI_Descrambler_t DescramblerHandle;
 STPTI_KeyParity_t   Parity;
 STPTI_KeyUsage_t    Usage;
 U8                  *UserKeyPtr;
 U8                  Data[32];
 ST_ErrorCode_t Error;
}STPTI_DescSetKeys_t;
#define STPTI_IOC_DESCSETKEYS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 56, STPTI_DescSetKeys_t* )

typedef struct {
 STPTI_Handle_t     Handle;
 STPTI_TimeStamp_t  ArrivalTime;
 U16                ArrivalTimeExtension;
 ST_ErrorCode_t Error;
}STPTI_GetArrivalTime_t;
#define STPTI_IOC_GETARRIVALTIME _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 57, STPTI_GetArrivalTime_t* )

typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    BOOL EnableDescramblingControl;   /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotDescramblingControl_t;
#define STPTI_IOC_SLOTDESCRAMBLINGCONTROL _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 58, STPTI_Ioctl_SlotDescramblingControl_t* )

/*
ST_ErrorCode_t STPTI_SlotSetAlternateOutputAction(STPTI_Slot_t SlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
*/
typedef struct{
    STPTI_Slot_t SlotHandle;            /* In */
    STPTI_AlternateOutputType_t Method; /* In */
    STPTI_AlternateOutputTag_t Tag;
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SlotSetAlternateOutputAction_t;
#define STPTI_IOC_SLOTSETALTERNATEOUTPUTACTION _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 59, STPTI_Ioctl_SlotSetAlternateOutputAction_t* )

/*
ST_ErrorCode_t STPTI_SlotState( STPTI_Slot_t SlotHandle,
                                U32 *SlotCount_p,
                                STPTI_ScrambleState_t *ScrambleState_p,
                                STPTI_Pid_t *Pid_p )
*/
typedef struct{
    STPTI_Slot_t SlotHandle;                    /* In */
    U32 SlotCount;                              /* Out */
    STPTI_ScrambleState_t ScrambleState;        /* Out */
    STPTI_Pid_t Pid;                            /* Out */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_SlotState_t;
#define STPTI_IOC_SLOTSTATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 60, STPTI_Ioctl_SlotState_t* )

/*
ST_ErrorCode_t STPTI_AlternateOutputSetDefaultAction(STPTI_Slot_t SlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
*/
typedef struct{
    STPTI_Slot_t SlotHandle;            /* In */
    STPTI_AlternateOutputType_t Method; /* In */
    STPTI_AlternateOutputTag_t Tag;
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_AlternateOutputSetDefaultAction_t;
#define STPTI_IOC_ALTERNATEOUTPUTSETDEFAULTACTION _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 61, STPTI_Ioctl_AlternateOutputSetDefaultAction_t* )

/*
ST_ErrorCode_t STPTI_HardwarePause(ST_DeviceName_t DeviceName)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_HardwareData_t;

#define STPTI_IOC_HARWAREPAUSE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 62, STPTI_Ioctl_HardwareData_t* )

/*
ST_ErrorCode_t STPTI_(ST_DeviceName_t DeviceName)
*/

#define STPTI_IOC_HARWARERESET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 63, STPTI_Ioctl_HardwareData_t* )

/*
ST_ErrorCode_t STPTI_(ST_DeviceName_t DeviceName)
*/

#define STPTI_IOC_HARWARERESUME _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 64, STPTI_Ioctl_HardwareData_t* )

/*
ST_ErrorCode_t STPTI_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    STPTI_Timer_t Timer;                /* In */
    STPTI_TimeStamp_t TimeStamp;        /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_GetPresentationSTC_t;

#define STPTI_IOC_GETPRESENTATIONSTC _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 65, STPTI_Ioctl_GetPresentationSTC_t* )

/*
ST_ErrorCode_t STPTI_BufferSetMultiPacket(STPTI_Buffer_t BufferHandle, U32 NumberOfPacketsInMultiPacket)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;        /* In */
    U32 NumberOfPacketsInMultiPacket;   /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_BufferSetMultiPacket_t;
#define STPTI_IOC_BUFFERSETMULTIPACKET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 66, STPTI_Ioctl_BufferSetMultiPacket_t* )

/******************************************************************************
Function Name : STPTI_BufferReadNBytes
  Description : A function to move n bytes from the contents of an internal
                buffer to a user-supplied buffer.
   Parameters : BufferHandle        Handle to a buffer.
                Destination0_p      Base of the linear buffer that the data will be copied into.
                DestinationSize0    Size of the linear buffer.
                Destination1_p      Base of the second buffer that will be copied into.
                                    Used if destination is a circular buffer.
                                    Set to NULL for basic linear copy.
                DestinationSize1    Size of second buffer
                DataSize_p          Amount of data actually moved
                DmaOrMemcpy         Request transfer to be performed by DMA or ST20
                BytesToCopy         Number of bytes to read from the internal circular buffer.
         Note : Moved here from pti_basic.c following DDTS 18257
******************************************************************************/
/*
ST_ErrorCode_t STPTI_BufferRead(STPTI_Buffer_t BufferHandle,
                                      U8 *Destination0_p, U32 DestinationSize0,
                                      U8 *Destination1_p, U32 DestinationSize1,
                                      U32 *DataSize_p,
                                      STPTI_Copy_t DmaOrMemcpy)

Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY
*/

typedef struct{
    STPTI_Buffer_t BufferHandle; /* In */
    U8 *Destination0_p;          /* In */
    U32 DestinationSize0;        /* In */
    U8 *Destination1_p;          /* In */
    U32 DestinationSize1;        /* In */
    U32 DataSize;               /* Out */
    STPTI_Copy_t DmaOrMemcpy;    /* In */
    ST_ErrorCode_t Error;        /* Out */
} STPTI_Ioctl_BufferRead_t;
#define STPTI_IOC_BUFFERREAD _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 67, STPTI_Ioctl_BufferRead_t* )

/*
ST_ErrorCode_t STPTI_BufferExtractData(STPTI_Buffer_t BufferHandle,
                                       U32 Offset,
                                       U32 NumBytesToExtract,
                                       U8 * Destination0_p,
                                       U32 DestinationSize0,
                                       U8 * Destination1_p,
                                       U32 DestinationSize1,
                                       U32 * DataSize_p,
                                       STPTI_Copy_t DmaOrMemcpy)
Only Memcpy supported - STPTI_COPY_TRANSFER_BY_MEMCPY
*/

typedef struct{
    STPTI_Buffer_t BufferHandle; /* In */
    U32 Offset;                  /* In */
    U32 NumBytesToExtract;       /* In */
    U8 *Destination0_p;          /* In */
    U32 DestinationSize0;        /* In */
    U8 *Destination1_p;          /* In */
    U32 DestinationSize1;        /* In */
    U32 DataSize;               /* Out */
    STPTI_Copy_t DmaOrMemcpy;    /* In */
    ST_ErrorCode_t Error;        /* Out */
} STPTI_Ioctl_BufferExtractData_t;
#define STPTI_IOC_BUFFEREXTRACTDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 68, STPTI_Ioctl_BufferExtractData_t* )

/*
ST_ErrorCode_t STPTI_BufferExtractPartialPesPacketData(STPTI_Buffer_t BufferHandle,
                                                       BOOL *PayloadUnitStart_p,
                                                       BOOL *CCDiscontinuity_p,
                                                       U8 *ContinuityCount_p,
                                                       U16 *DataLength_p)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle; /* In */
    BOOL PayloadUnitStart;      /* In */
    BOOL CCDiscontinuity;       /* In */
    U8 ContinuityCount;         /* In */
    U16 DataLength;             /* In */
    ST_ErrorCode_t Error;        /* Out */
} STPTI_Ioctl_BufferExtractPartialPesPacketData_t;
#define STPTI_IOC_BUFFEREXTRACTPARTIALPESPACKETDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 69, STPTI_Ioctl_BufferExtractPartialPesPacketData_t* )

/*
ST_ErrorCode_t STPTI_BufferExtractPesPacketData(STPTI_Buffer_t BufferHandle,
                                                U8 *PesFlags_p,
                                                U8 *TrickModeFlags_p,
                                                U32 *PESPacketLength_p,
                                                STPTI_TimeStamp_t * PTSValue_p,
                                                STPTI_TimeStamp_t * DTSValue_p)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;                /* In */
    U8 PesFlags;                                /* In */
    U8 TrickModeFlags;                          /* In */
    U32 PESPacketLength;                        /* In */
    STPTI_TimeStamp_t PTSValue;                 /* In */
    STPTI_TimeStamp_t DTSValue;                 /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_BufferExtractPesPacketData_t;
#define STPTI_IOC_BUFFEREXTRACTPESPACKETDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 70, STPTI_Ioctl_BufferExtractPesPacketData_t* )

/******************************************************************************

Function Name : STPTI_BufferExtractSectionData
  Description :
   Parameters :-

        STPTI_Buffer_t      BufferHandle          : Handle of Buffer containing section

        STPTI_Filter_t *    MatchedFilterList     : A pointer to previously allocated space
                                                    where a list of filters that this section
                                                    matched may be placed

        U16                 MaxLengthofFilterList : Maximum number of filters that may be palced
                                                    in list ( i.e ) the size of the allocated
                                                    memory in units of sizeof(STPTI_Filter_t)

        U16 *               NumOfFilterMatches    : A pointer to a location there the number of
                                                    filters that the section matched is to be placed.
                                                    If this is less than 'MaxLengthofFilterList'
                                                    then it will be the number of filters in the list,
                                                    otherwise the list will contain only the first
                                                    'MaxLengthofFilterList' of the total matched

        BOOL *              CRCValid              : A pointer to a location where a BOOL is written
                                                    that will indicate if the section's CRC was valid.
                                                    Only works with Manual mode TC code, With Automatic
                                                    Mode TC code then TRUE will always be returned.

        U32 *               SectionHeader         : A pointer to a location where the first 3 bytes of
                                                    the section header will be placed ( MS byte set to zero )

******************************************************************************/
/*
ST_ErrorCode_t STPTI_BufferExtractSectionData(STPTI_Buffer_t BufferHandle,
                                              STPTI_Filter_t MatchedFilterList[],
                                              U16 MaxLengthofFilterList,
                                              U16 *NumOfFilterMatches_p,
                                              BOOL *CRCValid_p,
                                              U32 *SectionHeader_p)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;                /* In */
    U16 MaxLengthofFilterList;                  /* In */
    U16 NumOfFilterMatches;                     /* Out */
    BOOL CRCValid;                              /* Out */
    U32 SectionHeader;                          /* Out */
    ST_ErrorCode_t Error;                       /* Out */
    STPTI_Filter_t *MatchedFilterList;          /* Out */
} STPTI_Ioctl_BufferExtractSectionData_t;
#define STPTI_IOC_BUFFEREXTRACTSECTIONDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 71, STPTI_Ioctl_BufferExtractSectionData_t* )

/*
ST_ErrorCode_t STPTI_BufferExtractTSHeaderData(STPTI_Buffer_t BufferHandle, U32 *TSHeader)
*/

typedef struct{
    STPTI_Buffer_t BufferHandle;                /* In */
    U32 TSHeader;                               /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_BufferExtractTSHeaderData_t;
#define STPTI_IOC_BUFFEREXTRACTTSHEADERDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 72, STPTI_Ioctl_BufferExtractTSHeaderData_t* )

/*
ST_ErrorCode_t STPTI_BufferReadPartialPesPacket(STPTI_Buffer_t BufferHandle,
                                                U8 *Destination0_p,
                                                U32 DestinationSize0,
                                                U8 *Destination1_p,
                                                U32 DestinationSize1,
                                                BOOL *PayloadUnitStart_p,
                                                BOOL *CCDiscontinuity_p,
                                                U8 *ContinuityCount_p,
                                                U32 *DataSize_p,
                                                STPTI_Copy_t DmaOrMemcpy)
 */
typedef struct{
    STPTI_Buffer_t BufferHandle; /* In */
    U8 *Destination0_p;          /* In */
    U32 DestinationSize0;        /* In */
    U8 *Destination1_p;          /* In */
    U32 DestinationSize1;        /* In */
    BOOL PayloadUnitStart;       /* Out */
    BOOL CCDiscontinuity;        /* Out */
    U8 ContinuityCount;          /* Out */
    U32 DataSize;               /* Out */
    STPTI_Copy_t DmaOrMemcpy;    /* In */
    ST_ErrorCode_t Error;        /* Out */
} STPTI_Ioctl_BufferReadPartialPesPacket_t;
#define STPTI_IOC_BUFFERREADPARTIALPESPACKET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 73, STPTI_Ioctl_BufferReadPartialPesPacket_t* )

/*
ST_ErrorCode_t STPTI_BufferReadTSPacket( STPTI_Buffer_t BufferHandle,
                                         U8 *Destination0_p,
                                         U32 DestinationSize0,
                                         U8 *Destination1_p,
                                         U32 DestinationSize1,
                                         U32 *DataSize_p,
                                         STPTI_Copy_t DmaOrMemcpy )
 */
/* Share STPTI_Ioctl_BufferRead_t structure */

#define STPTI_IOC_BUFFERREADTSPACKET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 74, STPTI_Ioctl_BufferRead_t* )

/*
ST_ErrorCode_t STPTI_FilterSetMatchAction(STPTI_Filter_t FilterHandle, STPTI_Filter_t FiltersToEnable[],U16 NumOfFiltersToEnable)
*/

typedef struct{
    STPTI_Filter_t FilterHandle;                /* In */
    U16 NumOfFiltersToEnable;                   /* In */
    ST_ErrorCode_t Error;                       /* Out */
    STPTI_Filter_t FiltersToEnable[];           /* In */
} STPTI_Ioctl_FilterSetMatchAction_t;
#define STPTI_IOC_FILTERSETMATCHACTION _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 75, STPTI_Ioctl_FilterSetMatchAction_t* )

/*
ST_ErrorCode_t STPTI_FiltersFlush(STPTI_Buffer_t BufferHandle, STPTI_Filter_t Filters[], U16 NumOfFiltersToFlush)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;                /* In */
    U16 NumOfFiltersToFlush;                   /* In */
    ST_ErrorCode_t Error;                       /* Out */
    STPTI_Filter_t Filters[];                   /* In */
} STPTI_Ioctl_FiltersFlush_t;
#define STPTI_IOC_FILTERSFLUSH _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 76, STPTI_Ioctl_FiltersFlush_t* )

/*
ST_ErrorCode_t STPTI_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[],
                                             U16 NumOfFiltersToDisable,
                                             STPTI_Filter_t FiltersToEnable[],
                                             U16 NumOfFiltersToEnable)
*/
typedef struct{
    U16 NumOfFiltersToDisable;                /* In */
    U16 NumOfFiltersToEnable;                   /* In */
    ST_ErrorCode_t Error;                       /* Out */
    STPTI_Filter_t Filters[];                   /* In */
} STPTI_Ioctl_ModifyGlobalFilterState_t;
#define STPTI_IOC_MODIFYGLOBALFILTERSTATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 77, STPTI_Ioctl_ModifyGlobalFilterState_t* )

/*
ST_ErrorCode_t STPTI_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count_p)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    U32 Count;                         /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_GetPacketErrorCount_t;

#define STPTI_IOC_GETPACKETERRORCOUNT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 78, STPTI_Ioctl_GetPacketErrorCount_t* )

/*
ST_ErrorCode_t STPTI_PidQuery(ST_DeviceName_t DeviceName, STPTI_Pid_t Pid, STPTI_Slot_t *Slot_p)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    STPTI_Pid_t Pid;                    /* In */
    STPTI_Slot_t Slot;                  /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_PidQuery_t;
#define STPTI_IOC_PIDQUERY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 79, STPTI_Ioctl_PidQuery_t* )
/*
ST_ErrorCode_t STPTI_GetCurrentPTITimer(ST_DeviceName_t DeviceName, STPTI_TimeStamp_t * TimeStamp)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    STPTI_TimeStamp_t TimeStamp;      /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_GetCurrentPTITimer_t;
#define STPTI_IOC_GETCURRENTPTITIMER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 80, STPTI_Ioctl_GetCurrentPTITimer_t* )

/*
ST_ErrorCode_t STPTI_GetGlobalPacketCount(ST_DeviceName_t DeviceName, U32 *Count)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    U32 Count;                         /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_GetGlobalPacketCount_t;
#define STPTI_IOC_GETGLOBALPACKETCOUNT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 81, STPTI_Ioctl_GetGlobalPacketCount_t* )

/*
ST_ErrorCode_t STPTI_GetBuffersFromSlot(STPTI_Slot_t SlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p)
 */
typedef struct{
    STPTI_Slot_t SlotHandle;            /* In */
    STPTI_Buffer_t BufferHandle;        /* Out */
    STPTI_Buffer_t RecordBufferHandle;  /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_GetBuffersFromSlot_t;
#define STPTI_IOC_GETBUFFERSFROMSLOT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 82, STPTI_Ioctl_GetBuffersFromSlot_t* )



/*
ST_ErrorCode_t STPTI_SetDiscardParams(ST_DeviceName_t DeviceName, U8 NumberOfDiscardBytes)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    U8 NumberOfDiscardBytes;            /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SetDiscardParams_t;
#define STPTI_IOC_SETDISCARDPARAMS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 83, STPTI_Ioctl_SetDiscardParams_t* )

/*
ST_ErrorCode_t STPTI_SetStreamID(ST_DeviceName_t Device, STPTI_StreamID_t StreamID)
*/
typedef struct{
    ST_DeviceName_t Device;             /* In */
    STPTI_StreamID_t StreamID;          /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SetStreamID_t;
#define STPTI_IOC_SETSTREAMID _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 84, STPTI_Ioctl_SetStreamID_t* )

/*
ST_ErrorCode_t STPTI_HardwareFIFOLevels(ST_DeviceName_t DeviceName, BOOL *Overflow, U16 *InputLevel, U16 *AltLevel, U16 *HeaderLevel)
*/
typedef struct{
    ST_DeviceName_t DeviceName;         /* In */
    BOOL Overflow;                      /* Out */
    U16 InputLevel;                     /* Out */
    U16 AltLevel;                       /* Out */
    U16 HeaderLevel;                    /* Out */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_HardwareFIFOLevels_t;
#define STPTI_IOC_HARDWAREFIFOLEVELS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 85, STPTI_Ioctl_HardwareFIFOLevels_t* )

/*
ST_ErrorCode_t STPTI_EnableScramblingEvents(STPTI_Slot_t SlotHandle)
*/
typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_EnableScramblingEvents_t;
#define STPTI_IOC_ENABLESCRAMBLINGEVENTS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 86, STPTI_Ioctl_EnableScramblingEvents_t* )





/* Added Indexing Under Linux */
#ifndef STPTI_NO_INDEX_SUPPORT
/*
ST_ErrorCode_t STPTI_IndexAllocate(STPTI_Handle_t SessionHandle, STPTI_Index_t *IndexHandle_p)
*/
typedef struct{
    STPTI_Handle_t SessionHandle;       /* In */
    STPTI_Index_t IndexHandle;          /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_IndexAllocate_t;
#define STPTI_IOC_INDEXALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 87, STPTI_Ioctl_IndexAllocate_t* )

/*
ST_ErrorCode_t STPTI_IndexAssociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid)
*/
typedef struct{
    STPTI_SlotOrPid_t SlotOrPid;
    STPTI_Index_t IndexHandle;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexAssociate_t;
#define STPTI_IOC_INDEXASSOCIATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 88, STPTI_Ioctl_IndexAssociate_t* )

/*
ST_ErrorCode_t STPTI_IndexDeallocate(STPTI_Index_t IndexHandle)
*/
typedef struct{
    STPTI_Index_t IndexHandle;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexDeallocate_t;
#define STPTI_IOC_INDEXDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 89, STPTI_Ioctl_IndexDeallocate_t* )

/*
ST_ErrorCode_t STPTI_IndexDisassociate(STPTI_Index_t IndexHandle, STPTI_SlotOrPid_t SlotOrPid)
*/
typedef struct{
    STPTI_SlotOrPid_t SlotOrPid;
    STPTI_Index_t IndexHandle;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexDisassociate_t;
#define STPTI_IOC_INDEXDISASSOCIATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 90, STPTI_Ioctl_IndexDisassociate_t* )

/*
ST_ErrorCode_t STPTI_IndexReset(STPTI_Index_t IndexHandle)
*/
typedef struct{
    STPTI_Index_t IndexHandle;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexReset_t;
#define STPTI_IOC_INDEXRESET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 91, STPTI_Ioctl_IndexReset_t* )

/*
ST_ErrorCode_t STPTI_IndexSet(STPTI_Index_t IndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode)

*/
typedef struct{
    STPTI_Index_t IndexHandle;
    STPTI_IndexDefinition_t IndexMask;
    U32 MPEGStartCode;
    U32 MPEGStartCodeMode;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexSet_t;
#define STPTI_IOC_INDEXSET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 92, STPTI_Ioctl_IndexSet_t* )

/*
ST_ErrorCode_t STPTI_IndexStart(ST_DeviceName_t DeviceName)
*/
typedef struct{
    ST_DeviceName_t DeviceName;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexStart_t;
#define STPTI_IOC_INDEXSTART _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 93, STPTI_Ioctl_IndexStart_t* )

/*
ST_ErrorCode_t STPTI_IndexStop(ST_DeviceName_t DeviceName)
*/
typedef struct{
    ST_DeviceName_t DeviceName;
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_IndexStop_t;
#define STPTI_IOC_INDEXSTOP _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 94, STPTI_Ioctl_IndexStop_t* )

#endif /* STPTI_NO_INDEX_SUPPORT */


/*
ST_ErrorCode_t STPTI_SlotLinkToRecordBuffer(STPTI_Slot_t Slot, STPTI_Buffer_t Buffer, BOOL DescrambleTS)
*/
typedef struct{
    STPTI_Slot_t Slot;                /* In */
    STPTI_Buffer_t Buffer;            /* In */
    BOOL DescrambleTS;                /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotLinkToRecordBuffer_t;
#define STPTI_IOC_SLOTLINKTORECORDBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 95, STPTI_Ioctl_SlotLinkToRecordBuffer_t* )


/*
ST_ErrorCode_t STPTI_SlotUnLinkRecordBuffer( STPTI_Slot_t SlotHandle )
*/
typedef struct{
    STPTI_Slot_t SlotHandle;                /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotUnLinkRecordBuffer_t;
#define STPTI_IOC_SLOTUNLINKRECORDBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 96, STPTI_Ioctl_SlotUnLinkRecordBuffer_t* )


/*
ST_ErrorCode_t STPTI_SlotSetCorruptionParams(STPTI_Slot_t SlotHandle, U8 Offset, U8 Value )
*/
typedef struct{
    STPTI_Slot_t SlotHandle;            /* In */
    U8 Offset;                          /* In */
    U8 Value;                           /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_SlotSetCorruptionParams_t;
#define STPTI_IOC_SLOTSETCORRUPTIONPARAMS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 97, STPTI_Ioctl_SlotSetCorruptionParams_t* )



/*
ST_ErrorCode_t STPTI_SlotUpdatePid( STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid )
*/
typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    STPTI_Pid_t Pid;                  /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SlotUpdatePid_t;
#define STPTI_IOC_SLOTUPDATEPID _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 102, STPTI_Ioctl_SlotUpdatePid_t* )

/*
ST_ErrorCode_t STPTI_BufferSetReadPointer(STPTI_Buffer_t BufferHandle, void* Read_p)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;      /* In */
    void* Read_p;                     /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_BufferSetReadPointer_t;
#define STPTI_IOC_BUFFERSETREADPOINTER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 103, STPTI_Ioctl_BufferSetReadPointer_t* )

/*
ST_ErrorCode_t STPTI_BufferGetWritePointer(STPTI_Buffer_t BufferHandle, void **Write_p)
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;      /* In */
    void* Write_p;                    /* Out */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_BufferGetWritePointer_t;
#define STPTI_IOC_BUFFERGETWRITEPOINTER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 104, STPTI_Ioctl_BufferGetWritePointer_t* )

/*
ST_ErrorCode_t STPTI_DisableScramblingEvents(STPTI_Slot_t SlotHandle)
*/
typedef struct{
    STPTI_Slot_t SlotHandle;          /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_DisableScramblingEvents_t;
#define STPTI_IOC_DISABLESCRAMBLINGEVENTS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 105, STPTI_Ioctl_DisableScramblingEvents_t* )

/*
ST_ErrorCode_t STPTI_ModifySyncLockAndDrop(ST_DeviceName_t DeviceName, U8 SyncLock, U8 SyncDrop)
*/
typedef struct{
    ST_DeviceName_t DeviceName;       /* In */
    U8 SyncLock;                      /* In */
    U8 SyncDrop;                      /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_ModifySyncLockAndDrop_t;
#define STPTI_IOC_MODIFYSYNCLOCKANDDROP _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 106, STPTI_Ioctl_ModifySyncLockAndDrop_t* )

/*
ST_ErrorCode_t STPTI_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data)
*/
typedef struct{
    ST_DeviceName_t DeviceName;       /* In */
    U8 *Data;                         /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_SetSystemKey_t;
#define STPTI_IOC_SETSYSTEMKEY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 107, STPTI_Ioctl_SetSystemKey_t* )

/*
ST_ErrorCode_t STPTI_DmaFree(U32 Size, void *Buffer_p)
*/
typedef struct{
    void* Buffer_p;                   /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_DmaFree_t;
#define STPTI_IOC_DMAFREE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 108, STPTI_Ioctl_DmaFree_t* )

#define STPTI_IOC_BUFFERALLOCATEMANUALUSER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 109, STPTI_Ioctl_BufferAllocateManual_t* )

/*
ST_ErrorCode_t STPTI_DumpInputTS(FullHandle_t BufferHandle, U16 bytes_to_capture_per_packet);
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;      /* In */
    U16 bytes_to_capture_per_packet;  /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_DumpInputTS_t;
#define STPTI_IOC_DUMPINPUTTS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 110, STPTI_Ioctl_DumpInputTS_t* )

#if defined (STPTI_FRONTEND_HYBRID)

/*
ST_ErrorCode_t STPTI_FrontendAllocate   ( STPTI_Handle_t PTIHandle, STPTI_Frontend_t * FrontendHandle_p, STPTI_FrontendType_t FrontendType);
*/
typedef struct{
    STPTI_Handle_t PTIHandle;                   /* In */
    STPTI_Frontend_t   FrontendHandle;          /* Out */
    STPTI_FrontendType_t FrontendType;          /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendAllocate_t;
#define STPTI_IOC_FRONTENDALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 111, STPTI_Ioctl_FrontendAllocate_t* )


/*
ST_ErrorCode_t STPTI_FrontendDeallocate ( STPTI_Frontend_t FrontendHandle );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;            /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendDeallocate_t;
#define STPTI_IOC_FRONTENDDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 112, STPTI_Ioctl_FrontendDeallocate_t* )


/*
ST_ErrorCode_t STPTI_FrontendGetParams  ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendType_t * FrontendType_p, STPTI_FrontendParams_t * FrontendParams_p );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;            /* In */
    STPTI_FrontendType_t FrontendType;          /* Out */
    STPTI_FrontendParams_t FrontendParams;      /* Out */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendGetParams_t;
#define STPTI_IOC_FRONTENDGETPARAMS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 113, STPTI_Ioctl_FrontendGetParams_t* )


/*
ST_ErrorCode_t STPTI_FrontendGetStatus  ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;            /* In */
    STPTI_FrontendStatus_t FrontendStatus;      /* Out */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendGetStatus_t;
#define STPTI_IOC_FRONTENDGETSTATUS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 114, STPTI_Ioctl_FrontendGetStatus_t* )


/*
ST_ErrorCode_t STPTI_FrontendInjectData ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p, U32 NumberOfSWTSNodes );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;               /* In */
    STPTI_FrontendSWTSNode_t* FrontendSWTSNode_p;  /* In */
    U32 NumberOfSWTSNodes;                         /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_FrontendInjectData_t;
#define STPTI_IOC_FRONTENDINJECTDATA _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 115, STPTI_Ioctl_FrontendInjectData_t* )


/*
ST_ErrorCode_t STPTI_FrontendLinkToPTI  ( STPTI_Frontend_t FrontendHandle, STPTI_Handle_t PTIHandle );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;           /* In */
    STPTI_Handle_t PTIHandle;                  /* In */
    ST_ErrorCode_t Error;                      /* Out */
} STPTI_Ioctl_FrontendLinkToPTI_t;
#define STPTI_IOC_FRONTENDLINKTOPTI _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 116, STPTI_Ioctl_FrontendLinkToPTI_t* )


/*
ST_ErrorCode_t STPTI_FrontendReset      ( STPTI_Frontend_t FrontendHandle );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;            /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendReset_t;
#define STPTI_IOC_FRONTENDRESET _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 117, STPTI_Ioctl_FrontendReset_t* )


/*
ST_ErrorCode_t STPTI_FrontendSetParams  ( STPTI_Frontend_t FrontendHandle, STPTI_FrontendParams_t * FrontendParams_p );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;            /* In */
    STPTI_FrontendParams_t FrontendParams;      /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendSetParams_t;
#define STPTI_IOC_FRONTENDSETPARAMS _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 118, STPTI_Ioctl_FrontendSetParams_t* )


/*
ST_ErrorCode_t STPTI_FrontendUnLink     ( STPTI_Frontend_t FrontendHandle );
*/
typedef struct{
    STPTI_Frontend_t FrontendHandle;            /* In */
    ST_ErrorCode_t Error;                       /* Out */
} STPTI_Ioctl_FrontendUnLink_t;
#define STPTI_IOC_FRONTENDUNLINK _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 119, STPTI_Ioctl_FrontendUnLink_t* )

#endif // #if defined (STPTI_FRONTEND_HYBRID) ... #endif


/*
ST_ErrorCode_t STPTI_BufferLevelSignalEnable(STPTI_Buffer_t BufferHandle, U32 BufferLevelThreshold);
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;       /* In */
    U32 BufferLevelThreshold;          /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_BufferLevelSignalEnable_t;
#define STPTI_IOC_BUFFERLEVELSIGNALENABLE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 120, STPTI_Ioctl_BufferLevelSignalEnable_t* )

/*
ST_ErrorCode_t STPTI_BufferLevelSignalDisable(STPTI_Buffer_t BufferHandle);
*/
typedef struct{
    STPTI_Buffer_t BufferHandle;       /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_BufferLevelSignalDisable_t;
#define STPTI_IOC_BUFFERLEVELSIGNALDISABLE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 121, STPTI_Ioctl_BufferLevelSignalDisable_t* )

/*
ST_ErrorCode_t STPTI_PrintDebug(ST_DeviceName_t DeviceName);
*/
typedef struct{
    ST_DeviceName_t DeviceName;       /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_PrintDebug_t;
#define STPTI_IOC_PRINTDEBUG _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 122, STPTI_Ioctl_PrintDebug_t* )

#ifndef STPTI_NO_INDEX_SUPPORT
/*
ST_ErrorCode_t STPTI_IndexChain(STPTI_Index_t *IndexHandles, U32 NumberOfHandles);
*/
#define STPTI_INDEXCHAIN_MAX_HANDLES  16
typedef struct{
    STPTI_Index_t IndexHandles[STPTI_INDEXCHAIN_MAX_HANDLES];    /* In */
    U32 NumberOfHandles;               /* In */
    ST_ErrorCode_t Error;              /* Out */
} STPTI_Ioctl_IndexChain_t;
#define STPTI_IOC_INDEXCHAIN _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 123, STPTI_Ioctl_IndexChain_t* )
#endif

#ifdef STPTI_CAROUSEL_SUPPORT
typedef struct{
    STPTI_Handle_t SessionHandle;     /* In */
    STPTI_Carousel_t CarouselHandle;  /* Out . If no error*/
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_CarouselAllocate_t;
#define STPTI_IOC_CAROUSELALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 124, STPTI_Ioctl_CarouselAllocate_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;  /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_CarouselDeallocate_t;
#define STPTI_IOC_CAROUSELDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 125, STPTI_Ioctl_CarouselDeallocate_t* )

typedef struct{
    STPTI_Handle_t SessionHandle;         /* In */
    STPTI_Carousel_t CarouselEntryHandle; /* Out . If no error*/
    ST_ErrorCode_t Error;                 /* Out */
} STPTI_Ioctl_CarouselEntryAllocate_t;
#define STPTI_IOC_CAROUSELENTRYALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 126, STPTI_Ioctl_CarouselEntryAllocate_t* )

typedef struct{
    STPTI_CarouselEntry_t CarouselEntryHandle; /* In */
    ST_ErrorCode_t Error;                      /* Out */
} STPTI_Ioctl_CarouselEntryDeallocate_t;
#define STPTI_IOC_CAROUSELENTRYDEALLOCATE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 127, STPTI_Ioctl_CarouselEntryDeallocate_t* )

typedef struct{
    STPTI_CarouselEntry_t CarouselEntryHandle; /* In */
    STPTI_TSPacket_t Packet;                   /* In */
    U8 FromByte;                               /* In */
    ST_ErrorCode_t Error;                      /* Out */
} STPTI_Ioctl_CarouselEntryLoad_t;
#define STPTI_IOC_CAROUSELENTRYLOAD _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 128, STPTI_Ioctl_CarouselEntryLoad_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;      /* In */
    STPTI_CarouselEntry_t Entry;          /* Out if no error */
    U32 PrivateData;                   /* In */
    ST_ErrorCode_t Error;                 /* Out */
} STPTI_Ioctl_CarouselGetCurrentEntry_t;
#define STPTI_IOC_CAROUSELGETCURRENTENTRY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 129, STPTI_Ioctl_CarouselGetCurrentEntry_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    STPTI_AlternateOutputTag_t AlternateOutputTag; /* In */
    STPTI_InjectionType_t InjectionType;           /* In */
    STPTI_CarouselEntry_t Entry;                   /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselInsertEntry_t;
#define STPTI_IOC_CAROUSELINSERTENTRY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 130, STPTI_Ioctl_CarouselInsertEntry_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    U8 *TSPacket_p;                                /* In */
    U8 CCValue;                                    /* In */
    STPTI_InjectionType_t InjectionType;           /* In */
    STPTI_TimeStamp_t MinOutputTime;               /* In */
    STPTI_TimeStamp_t MaxOutputTime;               /* In */
    BOOL EventOnOutput;                            /* In */
    BOOL EventOnTimeout;                           /* In */
    U32 PrivateData;                               /* In */
    STPTI_CarouselEntry_t Entry;                   /* In */
    U8 FromByte;                                   /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselInsertTimedEntry_t;
#define STPTI_IOC_CAROUSELINSERTTIMEDENTRY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 131, STPTI_Ioctl_CarouselInsertTimedEntry_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    STPTI_Buffer_t BufferHandle;                   /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselLinkToBuffer_t;
#define STPTI_IOC_CAROUSELLINKTOBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 132, STPTI_Ioctl_CarouselLinkToBuffer_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselLock_t;
#define STPTI_IOC_CAROUSELLOCK _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 133, STPTI_Ioctl_CarouselLock_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    STPTI_CarouselEntry_t Entry;                   /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselRemoveEntry_t;
#define STPTI_IOC_CAROUSELREMOVEENTRY _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 134, STPTI_Ioctl_CarouselRemoveEntry_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    STPTI_AllowOutput_t OutputAllowed;             /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselSetAllowOutput_t;
#define STPTI_IOC_CAROUSELSETALLOWOUTPUT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 135, STPTI_Ioctl_CarouselSetAllowOutput_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    U32 PacketTimeMs;                              /* In */
    U32 CycleTimeMs;                               /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselSetBurstMode_t;
#define STPTI_IOC_CAROUSELSETBURSTMODE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 136, STPTI_Ioctl_CarouselSetBurstMode_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    STPTI_Buffer_t BufferHandle;                   /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselUnlinkBuffer_t;
#define STPTI_IOC_CAROUSELUNLINKBUFFER _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 137, STPTI_Ioctl_CarouselUnlinkBuffer_t* )

typedef struct{
    STPTI_Carousel_t CarouselHandle;               /* In */
    ST_ErrorCode_t Error;                          /* Out */
} STPTI_Ioctl_CarouselUnLock_t;
#define STPTI_IOC_CAROUSELUNLOCK _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 138, STPTI_Ioctl_CarouselUnLock_t* )

#endif /* #ifdef STPTI_CAROUSEL_SUPPORT */

/*
ST_ErrorCode_t STPTI_Debug(ST_DeviceName_t DeviceName, char *dbg_class, char *string, int string_size);
*/
typedef struct{
    ST_DeviceName_t DeviceName;     /* In */
    char dbg_class[32];             /* In */
    char *string;                   /* In/Out */
    int string_size;                /* In */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_Debug_t;
#define STPTI_IOC_DEBUG _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 139, STPTI_Ioctl_Debug_t* )

typedef struct{
    STPTI_Buffer_t BufferHandle;      /* In */
    U32 Count;                        /* Out */
    ST_ErrorCode_t Error;             /* Out */
} STPTI_Ioctl_BufferPacketCount_t;
#define STPTI_IOC_BUFFERPACKETCOUNT _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 140, STPTI_Ioctl_BufferPacketCount_t* )

typedef struct{
	STPTI_Handle_t ObjectHandle;		/* In */
    STPTI_FeatureInfo_t FeatureInfo;    /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_ObjectEnableFeature_t;
#define STPTI_IOC_OBJECTENABLEFEATURE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 141, STPTI_Ioctl_ObjectEnableFeature_t* )

typedef struct{
	STPTI_Handle_t ObjectHandle;        /* In */
    STPTI_Feature_t Feature;            /* In */
    ST_ErrorCode_t Error;               /* Out */
} STPTI_Ioctl_ObjectDisableFeature_t;
#define STPTI_IOC_OBJECTDISABLEFEATURE _IOWR( STPTI_IOCTL_MAGIC_NUMBER, 142, STPTI_Ioctl_ObjectDisableFeature_t* )

#endif  /* #ifndef STPTI4_IOCTL_H */

