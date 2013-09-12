/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_extd.c
 Description: extended stpti capability (driver functions).

******************************************************************************/


/* Includes ---------------------------------------------------------------- */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif


#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stcommon.h"
#include "stdevice.h"
#include "stpti.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"
#include "validate.h"

#include "memget.h"

#ifdef ST_OSLINUX
#include "stpti4_core_proc.h"
#endif /* #ifdef ST_OSLINUX */

/* Public Variables -------------------------------------------------------- */


extern Driver_t STPTI_Driver;


/* Private Types ----------------------------------------------------------- */

/* Private Constants ------------------------------------------------------- */


#define STPTI_WORD_AT_A_TIME 0X00
#define STPTI_BYTE_AT_A_TIME 0X01

/* Private Variables ------------------------------------------------------- */

/* Private Macros ---------------------------------------------------------- */

/* Private Function Prototypes --------------------------------------------- */


/* Functions --------------------------------------------------------------- */


#ifndef STPTI_NullHandle
extern STPTI_Handle_t STPTI_NullHandle(void);
#endif

extern ST_ErrorCode_t stpti_Descrambler_AssociateSlot(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle);


/* ------------------------------------------------------------------------- */

/* moved here from pti_dvb.c */
/******************************************************************************
Function Name : STPTI_AlternateOutputSetDefaultAction
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_AlternateOutputSetDefaultAction(STPTI_Handle_t Handle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SessionHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    SessionHandle.word = Handle;

    Error = stptiValidate_AlternateOutputSetDefaultAction( SessionHandle, Method, Tag );
    if ( Error == ST_NO_ERROR )
    {
        if (Method == STPTI_ALTERNATE_OUTPUT_TYPE_NO_OUTPUT)
        {
            stptiMemGet_Device(SessionHandle)->AlternateOutputSet = STPTI_NullHandle();
        }
        else
        {
            stptiMemGet_Device(SessionHandle)->AlternateOutputSet = SessionHandle.word;
        }

        stptiMemGet_Device(SessionHandle)->AlternateOutputType = Method;
        stptiMemGet_Device(SessionHandle)->AlternateOutputTag  = Tag;

        Error = stptiHAL_AlternateOutputSetDefaultAction( SessionHandle, Method );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}
/******************************************************************************
Function Name : STPTI_DescramblerAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerAllocate(STPTI_Handle_t Handle, STPTI_Descrambler_t *DescramblerObject, STPTI_DescramblerType_t DescramblerType)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullDescramblerHandle;
    Descrambler_t TmpDescrambler;
    U32  TC_DescIdent;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSessionHandle.word = Handle;

    Error = stptiValidate_DescramblerAllocate( FullSessionHandle, DescramblerObject, DescramblerType);
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_DescramblerAllocate( FullSessionHandle, &TC_DescIdent );
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_CreateObjectHandle( FullSessionHandle, OBJECT_TYPE_DESCRAMBLER, sizeof(Descrambler_t), &FullDescramblerHandle);
            if (Error == ST_NO_ERROR)
            {
                TmpDescrambler.OwnerSession        = Handle;                        /* Load Parameters */
                TmpDescrambler.Handle              = FullDescramblerHandle.word;
                TmpDescrambler.Type                = DescramblerType;
                TmpDescrambler.TC_DescIdent        = TC_DescIdent;
                TmpDescrambler.AssociatedPid       = TC_INVALID_PID;
                TmpDescrambler.SlotListHandle.word = STPTI_NullHandle();

                memcpy((U8 *) stptiMemGet_Descrambler(FullDescramblerHandle), (U8 *) &TmpDescrambler, sizeof(Descrambler_t));

                Error = stptiHAL_DescramblerInitialise(FullDescramblerHandle);

#ifndef STPTI_NO_USAGE_CHECK
                Error = stpti_DescramblerHandleCheck(FullDescramblerHandle);
#endif
                *DescramblerObject = FullDescramblerHandle.word;
            }
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferClearGPDMAParams
  Description :
   Parameters :
******************************************************************************/
#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t STPTI_BufferClearGPDMAParams(STPTI_Buffer_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferClearGPDMAParams( FullBufferHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferClearGPDMAParams( FullBufferHandle );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}
#endif


/******************************************************************************
Function Name : STPTI_BufferClearPCPParams
  Description :
   Parameters :
******************************************************************************/
#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t STPTI_BufferClearPCPParams(STPTI_Buffer_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferClearPCPParams( FullBufferHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferClearPCPParams( FullBufferHandle );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}
#endif


/******************************************************************************
Function Name : STPTI_BufferSetMultiPacket
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferSetMultiPacket(STPTI_Buffer_t BufferHandle, U32 NumberOfPacketsInMultiPacket)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferSetMultiPacket( FullBufferHandle, NumberOfPacketsInMultiPacket );
    if (Error == ST_NO_ERROR)
    {
        ((Buffer_t *)stptiMemGet_Buffer(FullBufferHandle))->MultiPacketSize = NumberOfPacketsInMultiPacket;
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************

Function Name : STPTI_BufferExtractData
  Description :
   Parameters :-

******************************************************************************/
ST_ErrorCode_t STPTI_BufferExtractData(STPTI_Buffer_t BufferHandle, U32 Offset, U32 NumBytesToExtract,
                                       U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p,
                                       U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferExtractData( FullBufferHandle, Offset, NumBytesToExtract, Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, DataSize_p, DmaOrMemcpy );
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_BufferExtractData( FullBufferHandle, Offset, NumBytesToExtract, Destination0_p, DestinationSize0,Destination1_p, DestinationSize1, DataSize_p, DmaOrMemcpy );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}




/******************************************************************************
Function Name : STPTI_BufferExtractTSHeaderData
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferExtractTSHeaderData(STPTI_Buffer_t BufferHandle, U32 *TSHeader)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferExtractTSHeaderData( FullBufferHandle, TSHeader );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferExtractTSHeaderData(FullBufferHandle, TSHeader);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferRead
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferRead(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    FullBufferHandle.word = BufferHandle;

    if (NULL != DataSize_p)
        *DataSize_p = 0;

    Error = stptiValidate_BufferRead( FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, DataSize_p, DmaOrMemcpy);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferRead(FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p, DestinationSize1, DataSize_p, DmaOrMemcpy);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}



/* STPTI_BufferReadTSPacket was moved from here to pti_basic.c following GNBvd18257 */


/******************************************************************************
Function Name : STPTI_BufferSetGPDMAParams
  Description :
   Parameters :
******************************************************************************/
#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t STPTI_BufferSetGPDMAParams(STPTI_Buffer_t BufferHandle, STPTI_GPDMAParams_t GPDMAParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferSetGPDMAParams( FullBufferHandle, GPDMAParams);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferSetGPDMAParams(FullBufferHandle, &GPDMAParams);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}
#endif


/******************************************************************************
Function Name : STPTI_BufferSetPCPParams
  Description :
   Parameters :
******************************************************************************/
#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t STPTI_BufferSetPCPParams(STPTI_Buffer_t BufferHandle, STPTI_PCPParams_t PCPParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferSetPCPParams( FullBufferHandle, PCPParams);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferSetPCPParams( FullBufferHandle, &PCPParams);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}
#endif

/******************************************************************************
Function Name : STPTI_DescramblerDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerDeallocate(STPTI_Descrambler_t DescramblerHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDescramblerHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullDescramblerHandle.word = DescramblerHandle;

    Error = stptiValidate_DescramblerDeallocate( FullDescramblerHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_DeallocateDescrambler( FullDescramblerHandle, TRUE );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_SetSystemKey
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    Error = stptiValidate_SetSystemKey( DeviceName, Data );
    if (Error == ST_NO_ERROR)
    {
	    STOS_SemaphoreWait(stpti_MemoryLock);

        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_SetSystemKey( DeviceHandle, Data, 0 );
        }

    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }

    return( Error );
}


/*stpti_Descrambler_AssociateSlot was moved from here to pti_basic.c following GNBvd18257*/


/******************************************************************************
Function Name : stpti_Descrambler_AssociatePid
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_Descrambler_AssociatePid(FullHandle_t DescramblerHandle, STPTI_Pid_t Pid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SlotHandle;
    STPTI_Slot_t Slot, PCRSlot;


    Error = stptiValidate_Descrambler_AssociatePid( DescramblerHandle );
    if (Error == ST_NO_ERROR)
    {
        /* FIX  GNBvd25304: move stpti_SlotQueryPid() to after assigning AssociatedPid */
        (stptiMemGet_Descrambler(DescramblerHandle))->AssociatedPid = Pid;

        /* Scan for a Slot using this pid and associate the descrambler with it */
        Error = stpti_SlotQueryPid(DescramblerHandle, Pid, &Slot);
        if ( Slot != STPTI_NullHandle() )
        {
            SlotHandle.word = Slot;
            /* If the slot is a PCR slot, retry to get another */
            if(stptiMemGet_Slot( SlotHandle )->Type == STPTI_SLOT_TYPE_PCR)
            {
                PCRSlot = Slot;
                Error = stpti_NextSlotQueryPid(DescramblerHandle, Pid, &Slot);
                if ( Slot == STPTI_NullHandle() )
                {
                    /* If NullHandle then there was only a PCR slot allocated */
                    Slot = PCRSlot;
                }
            }

            SlotHandle.word = Slot;
            Error = stpti_Descrambler_AssociateSlot(DescramblerHandle, SlotHandle);
        }
    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_DescramblerAssociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerAssociate(STPTI_Descrambler_t DescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDescramblerHandle;
    FullHandle_t FullSlotHandle;
    STPTI_Pid_t Pid;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullDescramblerHandle.word = DescramblerHandle;

    Error = stptiValidate_DescramblerAssociate( FullDescramblerHandle, SlotOrPid );
    if (Error == ST_NO_ERROR)
    {
        if ( stptiMemGet_Device(FullDescramblerHandle)->DescramblerAssociationType == STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
        {
            FullSlotHandle.word = SlotOrPid.Slot;
            Error = stpti_Descrambler_AssociateSlot(FullDescramblerHandle, FullSlotHandle);
        }
        else
        {
            Pid = SlotOrPid.Pid;
            Error = stpti_Descrambler_AssociatePid(FullDescramblerHandle, Pid);
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_DescramblerDisassociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerDisassociate(STPTI_Descrambler_t DescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDescramblerHandle;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullDescramblerHandle.word = DescramblerHandle;

    Error = stptiValidate_DescramblerDisassociate( FullDescramblerHandle, SlotOrPid);
    if (Error == ST_NO_ERROR)
    {
        if ( stptiMemGet_Device(FullDescramblerHandle)->DescramblerAssociationType == STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
        {
            FullSlotHandle.word = SlotOrPid.Slot;
            Error = stpti_SlotDisassociateDescrambler(FullSlotHandle, FullDescramblerHandle, FALSE, FALSE);
        }
        else
        {
            Error = stpti_PidDisassociateDescrambler(FullDescramblerHandle, FALSE);
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_DescramblerSet
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerSet(STPTI_Descrambler_t DescramblerHandle, STPTI_KeyParity_t Parity,
                                    STPTI_KeyUsage_t Usage, U8 *Data)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDescramblerHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullDescramblerHandle.word = DescramblerHandle;

    Error = stptiValidate_DescramblerSet( FullDescramblerHandle, Parity, Usage, Data );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DescramblerSet( FullDescramblerHandle, Parity, Usage, Data );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_DescramblerSetType
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DescramblerSetType(STPTI_Descrambler_t DescHandle, STPTI_DescramblerType_t DescramblerType)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDescramblerHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullDescramblerHandle.word = DescHandle;

    Error = stptiValidate_DescramblerSetType( FullDescramblerHandle, DescramblerType);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DescramblerSetType( FullDescramblerHandle, DescramblerType);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


ST_ErrorCode_t STPTI_DescramblerSetSVP(STPTI_Descrambler_t DescHandle, BOOL Clear_SCB, STPTI_NSAMode_t mode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDescramblerHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullDescramblerHandle.word = DescHandle;

    Error = stptiValidate_DescramblerSetSVP( FullDescramblerHandle, mode);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DescramblerSetSVP( FullDescramblerHandle, Clear_SCB, mode);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_EnableScramblingEvents()
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_EnableScramblingEvents(STPTI_Slot_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_EnableDisableScramblingEvents( FullSlotHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_EnableScramblingEvents( FullSlotHandle );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_DisableScramblingEvents()
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DisableScramblingEvents(STPTI_Slot_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    FullSlotHandle.word = SlotHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_EnableDisableScramblingEvents( FullSlotHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_DisableScramblingEvents( FullSlotHandle );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}




/******************************************************************************
Function Name : STPTI_GetCapability
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t *DeviceCapability)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    Error = stptiValidate_GetCapability( DeviceName, DeviceCapability );
    if (Error == ST_NO_ERROR)
    {
	    STOS_SemaphoreWait(stpti_MemoryLock);

        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_LoaderGetCapability( DeviceHandle, DeviceCapability );
            if (Error == ST_NO_ERROR)
            {
                DeviceCapability->TCCodes = (stptiMemGet_Device(DeviceHandle))->TCCodes;
            }
        }

    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_GetInputPacketCount
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    Error = stptiValidate_GetInputPacketCount( DeviceName, Count );
    if (Error == ST_NO_ERROR)
    {
    	STOS_SemaphoreWait(stpti_MemoryLock);

        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_GetInputPacketCount(DeviceHandle, Count);
        }

    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_GetGlobalPacketCount
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetGlobalPacketCount(ST_DeviceName_t DeviceName, U32 *Count)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    Error = stptiValidate_GetInputPacketCount( DeviceName, (U16*) Count );
    if (Error == ST_NO_ERROR)
    {
    	STOS_SemaphoreWait(stpti_MemoryLock);

        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_GetPtiPacketCount(DeviceHandle, (U32 *)Count);
        }

    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_GetPacketArrivalTime
  Description :
   Parameters :
******************************************************************************/

ST_ErrorCode_t STPTI_GetPacketArrivalTime(STPTI_Handle_t Handle, STPTI_TimeStamp_t * ArrivalTime,
                                          U16 *ArrivalTimeExtension)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    /* PMC 26.09.02 removing mem_lock sem as this call needs to be made at interrupt
       level and a sem wait could block. Should be okay as long as no one calls STPTI_Init()
       at the same time. */

 /* STOS_SemaphoreWait(stpti_MemoryLock); */
    DeviceHandle.word = Handle;

    Error = stptiValidate_GetPacketArrivalTime( DeviceHandle, ArrivalTime, ArrivalTimeExtension);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_GetPacketArrivalTime( DeviceHandle, ArrivalTime, ArrivalTimeExtension );
    }

 /* STOS_SemaphoreSignal(stpti_MemoryLock); */
    return( Error );
}




/******************************************************************************
Function Name : STPTI_GetPresentationSTC
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    Error = stptiValidate_GetPresentationSTC( DeviceName, Timer, TimeStamp );
    if (Error == ST_NO_ERROR)
    {
          /* PMC 18.06.02 removing mem_lock sem as this call needs to be made at interrupt
	     level and a sem wait could block. Should be okay as long as no one calls STPTI_Init()
	     at the same time. */

     /* STOS_SemaphoreWait(stpti_MemoryLock); */

        Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, FALSE );
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_GetPresentationSTC( DeviceHandle, Timer, TimeStamp );
        }

     /* STOS_SemaphoreSignal(stpti_MemoryLock); */

    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_GetCurrentPTITimer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_GetCurrentPTITimer(ST_DeviceName_t DeviceName, STPTI_TimeStamp_t * TimeStamp)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   DeviceHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_GetCurrentPTITimer( DeviceName, TimeStamp );
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, TRUE );
        if (Error == ST_NO_ERROR)
        {
            Error = stptiHAL_GetCurrentPTITimer( DeviceHandle, TimeStamp );
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_GetRevision
  Description :
   Parameters :
******************************************************************************/
ST_Revision_t STPTI_GetRevision(void)
{
    return( STPTI_REVISION );
}


/******************************************************************************
Function Name : STPTI_HardwarePause
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_HardwarePause(ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_HardwarePause(DeviceHandle);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_HardwareFIFOLevels
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_HardwareFIFOLevels(ST_DeviceName_t DeviceName, BOOL *Overflow, U16 *InputLevel, U16 *AltLevel, U16 *HeaderLevel)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_HardwareFIFOLevels( DeviceHandle, Overflow, InputLevel, AltLevel, HeaderLevel );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/******************************************************************************
Function Name : STPTI_HardwareReset
  Description :
   Parameters :
******************************************************************************/

ST_ErrorCode_t STPTI_HardwareReset(ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_HardwareReset(DeviceHandle);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}


/******************************************************************************
Function Name : STPTI_HardwareResume
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_HardwareResume(ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_HardwareResume(DeviceHandle);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}

/******************************************************************************
Function Name : STPTI_BufferAllocateManual
  Description :Allocate an STPTI buffer using user allocated memory
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferAllocateManual(STPTI_Handle_t Handle,U8* Base_p, U32 RequiredSize, U32 NumberOfPacketsInMultiPacket, STPTI_Buffer_t * BufferHandle_p)
{  ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullBufferHandle;
    size_t ActualSize;
    Buffer_t TmpBuffer;
    U8 *Buffer_p, *Temp_p;
#if defined(ST_OSLINUX)
    void *MappedBuffer_p = NULL;
#endif


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSessionHandle.word = Handle;

    Error = stptiValidate_BufferAllocate( FullSessionHandle, RequiredSize, NumberOfPacketsInMultiPacket, BufferHandle_p);
    if (Error == ST_NO_ERROR)
    {
        if(Base_p != NULL)
        {
            Buffer_p = Base_p;
            /*As this is a manual allocation - check that the user has correctly allocated the required buffer*/
            ActualSize = stpti_BufferSizeAdjust(RequiredSize);
            Temp_p = (U8 *) stpti_BufferBaseAdjust(Buffer_p);
            if((Buffer_p != Temp_p)||(ActualSize != RequiredSize))
            {
                Error = ST_ERROR_BAD_PARAMETER;
                Buffer_p = NULL;
            }


            if (Buffer_p != NULL)
            {

#if defined(ST_OSLINUX)
                MappedBuffer_p = ioremap_nocache( (U32)Buffer_p, RequiredSize );
                /* CPU accesses will use this address as the base */
#endif
                Error = stpti_CreateObjectHandle(FullSessionHandle, OBJECT_TYPE_BUFFER, sizeof(Buffer_t), &FullBufferHandle);
                if (Error == ST_NO_ERROR)
                {
                    /* load default buffer configuration */
                    TmpBuffer.OwnerSession            = Handle;
                    TmpBuffer.Handle                  = FullBufferHandle.word;
#if defined(STPTI_GPDMA_SUPPORT)
                    TmpBuffer.STGPDMAHandle           = NULL;
    #endif

#if defined(ST_OSLINUX)
                    TmpBuffer.MappedStart_p           = MappedBuffer_p; /* ioremapped buffer address. Used for CPU reads. */
                    TmpBuffer.UserSpaceAddressDiff    = (U32)-1;
#endif
                    TmpBuffer.Start_p                 = Buffer_p;
                    TmpBuffer.RealStart_p             = Buffer_p;
                    TmpBuffer.ActualSize              = ActualSize;
                    TmpBuffer.RequestedSize           = RequiredSize;
                    TmpBuffer.Type                    = STPTI_SLOT_TYPE_NULL;
                    TmpBuffer.TC_DMAIdent             = UNININITIALISED_VALUE;  /* 0 may be safer if no param checking */
		            /* AS 26/03/03 Initializing DirectDma default Value as 0, because only DMA1,2,3 are linked, GNBvd19253 */
                    TmpBuffer.DirectDma               = 0;
                    TmpBuffer.CDFifoAddr              = (U32) NULL;
                    TmpBuffer.Holdoff                 = 0;
                    TmpBuffer.Burst                   = 0;
                    TmpBuffer.Queue_p                 = NULL;
                    TmpBuffer.BufferPacketCount       = 0;
                    TmpBuffer.MultiPacketSize         = NumberOfPacketsInMultiPacket;
                    TmpBuffer.SlotListHandle.word     = STPTI_NullHandle();
                    TmpBuffer.SignalListHandle.word   = STPTI_NullHandle();
#ifdef STPTI_CAROUSEL_SUPPORT
                    TmpBuffer.CarouselListHandle.word = STPTI_NullHandle();
    #endif

                    TmpBuffer.ManualAllocation   = TRUE;
                    TmpBuffer.IgnoreOverflow     = FALSE;
                    TmpBuffer.IsRecordBuffer    = FALSE;
                    TmpBuffer.EventLevelChange  = FALSE;
                    
                    memcpy((U8*) stptiMemGet_Buffer(FullBufferHandle), (U8 *) &TmpBuffer, sizeof(Buffer_t));

                    Error = stpti_BufferHandleCheck(FullBufferHandle);

                    *BufferHandle_p = FullBufferHandle.word;
#if defined( ST_OSLINUX )
                    /* Add to the proc file system */
                    AddPTIBufferToProcFS(FullBufferHandle);
#endif
                }
            }
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );

}

/******************************************************************************
Function Name : STPTI_BufferGetFreeLength
  Description : Get the free length remaining (based on actual write pointer)
                in a buffer
   Parameters : Relevant buffer and reference to return the free length
******************************************************************************/
ST_ErrorCode_t STPTI_BufferGetFreeLength(STPTI_Buffer_t BufferHandle, U32 *FreeLength_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;
    Error = stptiValidate_BufferGetFreeLength(FullBufferHandle);
    if(Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferGetFreeLength(FullBufferHandle, FreeLength_p);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);

    return Error;

}


/******************************************************************************
Function Name : STPTI_BufferLinkToCdFifo
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferLinkToCdFifo(STPTI_Buffer_t BufferHandle, STPTI_DMAParams_t * CdFifoParams)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_HARDWARE_CDFIFO_SUPPORT )
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferLinkToCdFifo( FullBufferHandle );
    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Buffer(FullBufferHandle))->CDFifoAddr = CdFifoParams->Destination;
        (stptiMemGet_Buffer(FullBufferHandle))->Holdoff    = CdFifoParams->Holdoff;
        (stptiMemGet_Buffer(FullBufferHandle))->Burst      = CdFifoParams->WriteLength;

        Error = stptiHAL_BufferLinkToCdFifo(FullBufferHandle, CdFifoParams);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    #endif /* defined( STPTI_HARDWARE_CDFIFO_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_SetStreamID
  Description :
   Parameters : DeviceName, and the new stream ID to set for it.
******************************************************************************/
ST_ErrorCode_t STPTI_SetStreamID(ST_DeviceName_t Device, STPTI_StreamID_t StreamID)
{
    ST_ErrorCode_t   Error            = ST_NO_ERROR;
    FullHandle_t     FullDeviceHandle;
#if defined (STPTI_FRONTEND_HYBRID)
    U32              DeviceNo;
    U32              DuplicateStreams = 0;
    STPTI_StreamID_t ExistingStreamID = 0;
#endif

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_SetStreamID( Device, StreamID );
    if( Error == ST_NO_ERROR )
    {
        Error = stpti_GetDeviceHandleFromDeviceName(Device, &FullDeviceHandle, TRUE);
        if( Error == ST_NO_ERROR )
        {
#if defined (STPTI_FRONTEND_HYBRID)
            Device_t * FocusDevice_p = stptiMemGet_Device( FullDeviceHandle );

            /* This needs to be changed but at the moment only allow the change of the stream ID when not linked to a frontend object */
            if ( STPTI_NullHandle() != FocusDevice_p->FrontendListHandle.word )
            {
                Error = STPTI_ERROR_FRONTEND_ALREADY_LINKED;
            }
            /* If we are trying to set the same ID just exit */
            else if ( (FocusDevice_p->StreamID && 0xFF7F) == StreamID )
            {
                Error = ST_NO_ERROR;
            }
            else
            {
                Device_t * Device_p;

                /* Scan all initialised drivers and check for the same StreamID */
                STTBX_Print(( "STPTI_SetStreamID: scanning %d devices for a match\n", stpti_GetNumberOfExistingDevices()));
                for ( DeviceNo = 0; DeviceNo < stpti_GetNumberOfExistingDevices(); ++DeviceNo )
                {
                    Device_p = stpti_GetDevice( DeviceNo );

                    /* Ignore the 'focus' device or device we are interested in */
                    if ( (NULL != Device_p) && (FocusDevice_p != Device_p))
                    {
                        if ( (Device_p->StreamID && 0xFF7F) == StreamID )
                        {
                            ExistingStreamID = Device_p->StreamID;
                            DuplicateStreams++;
                        }
                        /* Allow only 2 stream IDs the same for TSIN types */
                        if ( StreamID && (STPTI_STREAM_IDTYPE_TSIN << 8) )
                        {
                            if (( (Device_p->StreamID && 0xFF7F) == StreamID ) &&
                                ( DuplicateStreams > 1 )  )
                            {
                                STTBX_Print(( "exceeds two: a driver->StreamID = InitParams->StreamID = %d\n", StreamID ));
                                Error = ST_ERROR_ALREADY_INITIALIZED;
                                break;
                            }
                        }
                        else
                        {
                            /* transport routing must be unique for SWTS type */
                            if (( Device_p->StreamID == StreamID ) &&
                                ( StreamID != STPTI_STREAM_ID_NONE ) )
                            {
                                STTBX_Print(( "not unique: a driver->StreamID = InitParams->StreamID = %d\n", StreamID ));
                                Error = ST_ERROR_ALREADY_INITIALIZED;
                                break;
                            }
                        }
                    }
                }/* for( device ) */
                
                if ( ST_NO_ERROR == Error )
                {
                    /* If we got here it's OK for us to set the stream ID */
                    Error = stptiHAL_SetStreamID( FullDeviceHandle, StreamID );
                    /* Make sure we update the device structure as well as the TC session */
                    FocusDevice_p->StreamID = StreamID;
                }
           }/* else */
#else
            Error = stptiHAL_SetStreamID( FullDeviceHandle, StreamID );
#endif /* if defined (ST_7200) */
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);


    return Error;
}

/******************************************************************************
Function Name : STPTI_SlotSetCCControl
  Description :
   Parameters : SetOnOff - when TRUE   CC checking will be disabled.
                         - when FALSE  CC checking will be enabled.
******************************************************************************/
ST_ErrorCode_t STPTI_SlotSetCCControl(STPTI_Slot_t SlotHandle, BOOL SetOnOff)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotSetCCControl( FullSlotHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_SlotSetCCControl(FullSlotHandle, SetOnOff);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}




/******************************************************************************
Function Name : STPTI_ModifySyncLockAndDrop
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_ModifySyncLockAndDrop(ST_DeviceName_t DeviceName, U8 SyncLock, U8 SyncDrop)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_ModifySyncLockAndDrop(DeviceHandle, SyncLock, SyncDrop);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/*stpti_DescramblerQueryPid was moved from here to pti_basic.c following GNBvd18257*/


/******************************************************************************
Function Name : stpti_IndexQueryPid
  Description :
   Parameters :
******************************************************************************/
#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode_t stpti_IndexQueryPid(FullHandle_t DeviceHandle, STPTI_Pid_t Pid, STPTI_Index_t * Index_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SessionHandle;
    FullHandle_t IndexHandle;
    FullHandle_t IndexListHandle;
    U16 MaxSessions;
    U16 session;
    U16 MaxIndex;
    U16 index;


    *Index_p = STPTI_NullHandle();
    MaxSessions = stptiMemGet_Device(DeviceHandle)->MemCtl.MaxHandles;

    for (session = 0; session < MaxSessions; ++session) /* For all sessions */
    {
        SessionHandle = stptiMemGet_Device(DeviceHandle)->MemCtl.Handle_p[session].Hndl_u;
        if (SessionHandle.word != STPTI_NullHandle())
        {
            IndexListHandle = stptiMemGet_Session(SessionHandle)->AllocatedList[OBJECT_TYPE_INDEX];

            if (IndexListHandle.word != STPTI_NullHandle())
            {
                MaxIndex = stptiMemGet_List(IndexListHandle)->MaxHandles;

                for (index = 0; index < MaxIndex; ++index) /* for all indexes */
                {
                    IndexHandle.word = (&stptiMemGet_List(IndexListHandle)->Handle)[index];

                    if (IndexHandle.word != STPTI_NullHandle())
                    {
                        if ((stptiMemGet_Index(IndexHandle))->AssociatedPid == Pid)
                        {
                            *Index_p = IndexHandle.word;
                            break;
                        }
                    }
                }

                if (*Index_p != STPTI_NullHandle())
                    break;
            }
        }
    }

    return( Error );
}
#endif


/******************************************************************************
Function Name : STPTI_PidQuery
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_PidQuery(ST_DeviceName_t DeviceName, STPTI_Pid_t Pid, STPTI_Slot_t *Slot_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    Error = stptiValidate_PidQuery( DeviceName, Pid, Slot_p );
    if ( Error == ST_NO_ERROR )
    {
    	STOS_SemaphoreWait(stpti_MemoryLock);

        Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_SlotQueryPid(DeviceHandle, Pid, Slot_p);
        }

    	STOS_SemaphoreSignal(stpti_MemoryLock);
    }

    return( Error );
}


/******************************************************************************
Function Name : STPTI_SlotPacketCount
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotPacketCount(STPTI_Slot_t SlotHandle, U16 *Count)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    FullSlotHandle.word = SlotHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_SlotPacketCount( FullSlotHandle, Count );
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_SlotPacketCount( FullSlotHandle, Count );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}

/******************************************************************************
Function Name : STPTI_SlotSetCorruptionParams()
  Description : Corrupts a raw packets payload output by the TC.
                The payload is replaced with a constant Value.
   Parameters : SlotHandle  - The Slot to configure for corruption
                Offset      - The Offset from the Start of the packet where corruption starts
                Value       - The value to corrupt the payload with.
******************************************************************************/
ST_ErrorCode_t STPTI_SlotSetCorruptionParams(STPTI_Slot_t SlotHandle, U8 Offset, U8 Value )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    FullSlotHandle.word = SlotHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_SlotSetCorruptionParams( FullSlotHandle, Offset );
    if (Error == ST_NO_ERROR)
    {
        stptiHAL_SlotSetCorruptionParams( FullSlotHandle, Offset, Value );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}



/*STPTI_BufferUnLink was moved from here to pti_basic.c following GNBvd18257*/


/******************************************************************************
Function Name : STPTI_UserDataBlockMove
  Description : Sets up a PTI DMA for a 1D block move. The call will return as
                soon as the DMA has started.
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_UserDataBlockMove(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_MULTI_DMA_SUPPORT )
    U16 MaxDevices;
    U16 Device;
    FullHandle_t DeviceHandle;


    Error = stptiValidate_UserDataBlockMove( DataPtr_p, DataLength, DMAParams_p);
    if (Error == ST_NO_ERROR)
    {
        MaxDevices = STPTI_Driver.MemCtl.MaxHandles;

        /* look through all the initialised devices for a suitable DMA engine */

        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DeviceHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;
            if (DeviceHandle.word != STPTI_NullHandle())
            {
                /* Block Move  - user defined burst mode */
                Error = stptiHAL_UserDataWrite( DeviceHandle, DataPtr_p, DataLength, DataPtr_p, (U8*)0, NULL, DMAParams_p, TRUE, DMAParams_p->BurstSize|0x02, DMAParams_p->DMAUsed );

                if (Error == ST_NO_ERROR)   /* Stop when we have succeded */
                    break;
            }
        }
    }
    #endif /* defined( STPTI_MULTI_DMA_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_UserDataCircularAppend
  Description : Append data to an existing User Data DMA process.

                This function allow data to be added to a circular user data
                transfer as setup using STPTI_UserDataWrite(). As with this
                call, the DataLength is the amount of data to copy and
                NextData_p will return the pointer to the next available free
                space. STPTI will handle the wrapping of these pointers within
                the circular buffer.

                This function can be used to allow continual feeding of a
                CD-FIFO by appending data to a circular transfer.

                STPTI_EVENT_DMA_COMPLETE_EVT, event will be raised at the end
                of the transfer if no further call to STPTI_UserDataCircularAppend()
                is made.
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_UserDataCircularAppend(U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_MULTI_DMA_SUPPORT )
    U16 MaxDevices;
    U16 Device;
    FullHandle_t DeviceHandle;


    Error = stptiValidate_UserDataCircularAppend( DataLength, DMAParams_p, NextData_p );
    if (Error == ST_NO_ERROR)
    {
        MaxDevices = STPTI_Driver.MemCtl.MaxHandles;

        /* look through all the initialised devices for a suitable DMA engine */
        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DeviceHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;
            if (DeviceHandle.word != STPTI_NullHandle())
            {
                Error = stptiHAL_UserDataCircularAppend( DeviceHandle, DataLength, NextData_p, DMAParams_p );

                if (Error == ST_NO_ERROR)   /* Stop when we have succeded */
                    break;
            }
        }
    }
    #endif /* defined( STPTI_MULTI_DMA_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_UserDataCircularSetup
  Description : Sets up a DMA for application control of PTI DMA in circular mode.

                The function sets up a PTI DMA with a circular buffer and starts
                the first transfer as given by the *DataPtr_p and DataLength field.
                The function is non-blocking and will return as soon as the DMA
                is started, with the pointer to the next available data given by
                *NextData_p. The *NextData_p will take into account any buffer
                wrapping.

                STPTI_EVENT_DMA_COMPLETE_EVT, event will be raised at the end of
                the transfer if no further call to STPTI_UserDataCircularAppend()
                is made.
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_UserDataCircularSetup(U8 *Buffer_p, U32 BufferSize, U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_MULTI_DMA_SUPPORT )
    U16 MaxDevices;
    U16 Device;
    FullHandle_t DeviceHandle;


    Error = stptiValidate_UserDataCircularSetup( Buffer_p, BufferSize, DataPtr_p, DataLength, DMAParams_p, NextData_p);
    if (Error == ST_NO_ERROR)
    {
        MaxDevices = STPTI_Driver.MemCtl.MaxHandles;

        /* look through all the initialised devices for a suitable DMA engine */
        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DeviceHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;
            if (DeviceHandle.word != STPTI_NullHandle())
            {
              Error = stptiHAL_UserDataWrite( DeviceHandle, DataPtr_p, DataLength, Buffer_p, Buffer_p+BufferSize,
                                                NextData_p, DMAParams_p, TRUE, DMAParams_p->BurstSize|0x02, DMAParams_p->DMAUsed );
                if (Error == ST_NO_ERROR)
                    break;       /* Stop when we have succeeded */
            }
        }
    }
    #endif /* defined( STPTI_MULTI_DMA_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_UserDataSynchronize
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_UserDataSynchronize(STPTI_DMAParams_t *DMAParams_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_MULTI_DMA_SUPPORT )
    U16 MaxDevices;
    U16 Device;
    FullHandle_t DeviceHandle;


    Error = stptiValidate_UserDataSynchronize( DMAParams_p );
    if (Error == ST_NO_ERROR)
    {
        MaxDevices = STPTI_Driver.MemCtl.MaxHandles;

        /* look through all the initialised devices for a suitable DMA engine */
        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DeviceHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;
            if (DeviceHandle.word != STPTI_NullHandle())
            {
                Error = stptiHAL_UserDataSynchronize(DeviceHandle, DMAParams_p);
                if (Error == ST_NO_ERROR)
                    break;
            }
        }

        /* If the DMA is not setup then it must have completed - and probably raised an event */
        if (Error == STPTI_ERROR_DMA_UNAVAILABLE)
            Error = ST_NO_ERROR;
    }
    #endif /* defined( STPTI_MULTI_DMA_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_UserDataWrite
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_UserDataWrite(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t * DMAParams_p)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_MULTI_DMA_SUPPORT )
    U16 MaxDevices;
    U16 Device;
    FullHandle_t DeviceHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stptiValidate_UserDataWrite( DataPtr_p, DataLength, DMAParams_p);
    if (Error == ST_NO_ERROR)
    {
        MaxDevices = STPTI_Driver.MemCtl.MaxHandles;

        /* look through all the initialised devices for a suitable DMA engine */
        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DeviceHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;
            if (DeviceHandle.word != STPTI_NullHandle())
            {
                Error = stptiHAL_UserDataWrite( DeviceHandle, DataPtr_p, DataLength, DataPtr_p, (U8*)0, NULL,
                                                DMAParams_p, TRUE, (DMAParams_p->BurstSize & 0x01), DMAParams_p->DMAUsed );
                if (Error == ST_NO_ERROR)
                    break;   /* Stop when we have succeded */
            }
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    #endif /* defined( STPTI_MULTI_DMA_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_UserDataRemaining
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_UserDataRemaining(STPTI_DMAParams_t *DMAParams_p, U32 *UserDataRemaining)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_MULTI_DMA_SUPPORT )
    U16 MaxDevices;
    U16 Device;
    FullHandle_t DeviceHandle;


    Error = stptiValidate_UserDataRemaining( DMAParams_p, UserDataRemaining);
    if (Error == ST_NO_ERROR)
    {
        MaxDevices = STPTI_Driver.MemCtl.MaxHandles;

        /* look through all the initialised devices for a suitable DMA engine */
        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DeviceHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;
            if (DeviceHandle.word != STPTI_NullHandle())
            {
                Error = stptiHAL_UserDataRemaining(DeviceHandle, DMAParams_p, UserDataRemaining);

                if (Error == ST_NO_ERROR)
                    break;
            }
        }

        /* If the DMA is not setup then it must have completed - and probably raised an event */
        if (Error == STPTI_ERROR_DMA_UNAVAILABLE)
        {
            Error = ST_NO_ERROR;
            *UserDataRemaining = 0;
        }
    }
    #endif /* defined( STPTI_MULTI_DMA_SUPPORT ) */

    return( Error );
}


/******************************************************************************
Function Name : STPTI_SlotLinkToCdFifo
  Description :
   Parameters :
         Note : Moved here from pti_basic.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t STPTI_SlotLinkToCdFifo(STPTI_Slot_t SlotHandle, STPTI_DMAParams_t *CdFifoParams)
{
    ST_ErrorCode_t Error = ST_ERROR_FEATURE_NOT_SUPPORTED;

#if defined( STPTI_HARDWARE_CDFIFO_SUPPORT )
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotLinkToCdFifo( FullSlotHandle, CdFifoParams);
    if (Error == ST_NO_ERROR)
    {
	    (stptiMemGet_Slot(FullSlotHandle))->CDFifoAddr = CdFifoParams->Destination;
	    (stptiMemGet_Slot(FullSlotHandle))->Holdoff    = CdFifoParams->Holdoff;
	    (stptiMemGet_Slot(FullSlotHandle))->Burst      = CdFifoParams->WriteLength;

        Error = stptiHAL_SlotLinkToCdFifo(FullSlotHandle);
	}

    STOS_SemaphoreSignal(stpti_MemoryLock);
    #endif /* defined( STPTI_HARDWARE_CDFIFO_SUPPORT ) */

    return( Error );
}


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
ST_ErrorCode_t STPTI_BufferReadNBytes(STPTI_Buffer_t BufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;


    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

    if (NULL != DataSize_p) *DataSize_p = 0;

    Error = stptiValidate_BufferReadNBytes( FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p,
                                            DestinationSize1, DataSize_p, DmaOrMemcpy, BytesToCopy );

    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferReadNBytes( FullBufferHandle, Destination0_p, DestinationSize0, Destination1_p,
                                           DestinationSize1, DataSize_p, DmaOrMemcpy, BytesToCopy );
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}

/******************************************************************************
Function Name : STPTI_GetBuffersFromSlot
  Description : A function return the buffer handles associated with a slot
   Parameters : SlotHandle          Handle to the slot to search.
                *BufferHandle_p     pointer to returned buffer value
                *RecordBufferHandle_p pointer to returned record buffer handle
******************************************************************************/
ST_ErrorCode_t STPTI_GetBuffersFromSlot(STPTI_Slot_t SlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p)
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    Slot_t          *Slot_p ;
    FullHandle_t    FullSlotHandle;
    FullHandle_t    FullBufferHandle;
    U32             Index;
    List_t          *List_p;
    
    FullSlotHandle.word=SlotHandle;
    Slot_p = stptiMemGet_Slot( FullSlotHandle );
    
    Error = stptiValidate_GetBuffersFromSlot(FullSlotHandle, BufferHandle_p, RecordBufferHandle_p);

    if (Error == ST_NO_ERROR)
    {
        *BufferHandle_p = STPTI_NullHandle();
        *RecordBufferHandle_p = STPTI_NullHandle();
        if( Slot_p && (Slot_p->BufferListHandle.word != STPTI_NullHandle()) )
        {
            List_p = stptiMemGet_List(Slot_p->BufferListHandle);
            for(Index = 0 ; Index < STPTI_MAX_BUFFERS_PER_SLOT ; Index++)
            {
                FullBufferHandle.word = (&List_p->Handle)[Index];
                if(List_p->UsedHandles <= Index)
                {
                    break;
                }
                if (FullBufferHandle.word != STPTI_NullHandle())
                {
                    if (FALSE == stptiMemGet_Buffer(FullBufferHandle)->IsRecordBuffer)
                    {
                        *BufferHandle_p = FullBufferHandle.word;
                    }
                    else
                    {
                        *RecordBufferHandle_p = FullBufferHandle.word;
                    }
                }
            }
        }
    }

    return (Error);
}

#if !defined ( STPTI_BSL_SUPPORT )
ST_ErrorCode_t STPTI_PrintDebug(ST_DeviceName_t DeviceName)
{
    FullHandle_t DeviceHandle;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    STOS_SemaphoreWait(stpti_MemoryLock);

    Error = stpti_GetDeviceHandleFromDeviceName(DeviceName, &DeviceHandle, TRUE);
    if (Error == ST_NO_ERROR)
    {
        stptiHAL_PrintDebug(DeviceHandle);
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return ST_NO_ERROR;
}

ST_ErrorCode_t STPTI_DumpInputTS(STPTI_Buffer_t BufferHandle, U16 bytes_to_capture_per_packet)
{
    FullHandle_t FullBufferHandle;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullBufferHandle.word = BufferHandle;
    
    Error = stptiHAL_DumpInputTS(FullBufferHandle, bytes_to_capture_per_packet);
    
    return( Error );
    
}
#endif

/* --- EOF  --- */
