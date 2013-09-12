/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: helper.c
 Description: HAL support functions.

******************************************************************************/

/* Includes ---------------------------------------------------------------- */

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <string.h>
#endif /* #endif !defined(ST_OSLINUX)  */

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"
#include "osx.h"

/* ------------------------------------------------------------------------- */

#define  STPTI_HELPER_LOOP_COUNT_MAX 16

#ifdef STPTI_GPDMA_SUPPORT
static U32 GPDMATransfer(U8 *Dest, U8 *Src, U32 CopySize, FullHandle_t FullBufferHandle);
#endif
#if defined(STPTI_FDMA_SUPPORT)
static U32 FDMATransfer(U8 *Dest, U8 *Src, U32 CopySize, FullHandle_t FullBufferHandle);
extern STFDMA_GenericNode_t *FDMANode_p;
extern STFDMA_TransferGenericParams_t FDMAGenericTransferParams;
extern STFDMA_TransferId_t     FDMATransferId;
#endif
#if defined(STPTI_PCP_SUPPORT)
static U32 PCPTransfer(U8 *Dest, U8 *Src, U32 CopySize, FullHandle_t FullBufferHandle);
#endif

#if !defined ( STPTI_BSL_SUPPORT )
static U32 DMATransfer(U8 *Dest, U8 *Src, U32 CopySize, TCDevice_t *TC_Device_p, U32 DirectDma, STPTI_TCParameters_t *TC_Params_p);
#endif

/* ------------------------------------------------------------------------- */


/******************************************************************************
Function Name : stptiHelper_BufferBaseAdjust
  Description : stpti_TCBufferBaseAdjust
   Parameters :

 We must be CACHE LINE ALIGNED as well as fit a DMA boundary.  This do with 
 cache coherency in which we must make sure that a single cache line does not
 have HOST read/writable items as well as HOST read only items.
 
 All ST40 and ST200 micros has cache line lengths of 32bytes.
 
 We also must make sure the buffers are DMA boundry aligned which is conveniently
 16bytes.
 
 See GNBvd68875
 
******************************************************************************/
void *stptiHelper_BufferBaseAdjust(void *Base_p)
{
    return( stpti_MemoryAlign(Base_p, STPTI_BUFFER_ALIGN_MULTIPLE) );
}


/******************************************************************************
Function Name : stptiHelper_BufferSizeAdjust
  Description : stpti_TCBufferSizeAdjust
   Parameters :

 We must be CACHE LINE ALIGNED as well as fit a DMA boundary.  This do with 
 cache coherency in which we must make sure that a single cache line does not
 have HOST read/writable items as well as HOST read only items.
 
 All ST40 and ST200 micros has cache line lengths of 32bytes.
 
 We also must make sure the buffers are DMA boundry aligned which is conveniently
 16bytes.
 
 See GNBvd68875
  
******************************************************************************/
size_t stptiHelper_BufferSizeAdjust(size_t Size)
{
    return( (size_t)stpti_MemoryAlign((void *)Size, STPTI_BUFFER_SIZE_MULTIPLE) );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHelper_SlotSetGlobalInterruptMask
  Description : stpti_TCSlotSetGlobalInterruptMask.

 A function to set the GlobalInterrupt Mask to Enable/Disable what interrupts are raised.

   Parameters : SlotHandle:     The SlotHandle to set the interrupt bits on.
                Flags:          The Bits to be enabled/disabled.
                Enable:         A flag to specify whether to enable or disable the bits.
                                TRUE = Enable; FALSE = Disable
******************************************************************************/
ST_ErrorCode_t stptiHelper_SlotSetGlobalInterruptMask(FullHandle_t SlotHandle, U32 Flags, BOOL Enable)
{
    Device_t                *Device_p = stptiMemGet_Device(SlotHandle);
    TCPrivateData_t    *PrivateData_p = stptiMemGet_PrivateData(SlotHandle);
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t  *TCSessionInfo_p = &((TCSessionInfo_t *)TC_Params_p->TC_SessionDataStart)[ Device_p->Session ];

    if ( Enable )
    {
        STSYS_SetTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, (Flags));
    }
    else
    {
        STSYS_ClearTCMask16LE((void*)&TCSessionInfo_p->SessionInterruptMask0, (Flags));
    }

    return( ST_NO_ERROR );
}
#endif /*#if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : stptiHelper_SetLookupTableEntry
  Description : stpti_TCSetLookupTableEntry
   Parameters :
******************************************************************************/
void stptiHelper_SetLookupTableEntry(FullHandle_t DeviceHandle, U16 slot, U16 Pid)
{
    TCPrivateData_t     *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p  = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    TcHal_LookupTableSetPidForSlot( TC_Params_p, slot, Pid);
}


/******************************************************************************
Function Name : stptiHelper_SignalEnable
  Description : stpti_SignalEnable
   Parameters : BufferHandle, MultiSize

 This function will signal again if there is enough data in the buffer to
 qualify (i.e. a multiple of MultiSize).  Otherwise it re-enables the TC
 signalling mechanism, to wait for the the new threshold to be reached.
******************************************************************************/
void stptiHelper_SignalEnable(FullHandle_t FullBufferHandle, U16 MultiSize)
{
    U16              DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    U16   PacketCount;
    U16   SignallingThreshold;
    BOOL  SignalTheBuffer = FALSE;
    U16   PacketsAboveThreshold;

    STOS_TaskLock();            /* Avoid signal enabling race condition with interrupt task */

    PacketCount = (U16) DMAConfig_p->BufferPacketCount;
    SignallingThreshold = (DMAConfig_p->Threshold + MultiSize) & 0xffff;
    PacketsAboveThreshold = (PacketCount - SignallingThreshold) & 0xffff;

    STSYS_WriteTCReg16LE((U32)&DMAConfig_p->Threshold, SignallingThreshold );

    /* Test the MSBit of PacketsAboveThreshold to see if it is positive */
    if( (PacketsAboveThreshold&0x8000) == 0 )
    {
        /* Most Significant bit is clear, so this is a positive number (PacketsAboveThreshold>=0) */
        SignalTheBuffer = TRUE;
    }
    else
    {
        /* Most Significant bit is set, so this is a negative number (PacketsAboveThreshold<0) */
        /* No more data in buffer....  re-enable TC Signalling */
        STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
    }

    STOS_TaskUnlock();

    /* Avoid signalling when in a task lock */
    if( SignalTheBuffer )
    {
        stptiHelper_SignalBuffer(FullBufferHandle, 0XCCCCCCCC);
    }

}


/******************************************************************************
Function Name : stptiHelper_BufferLevelSignalEnable
  Description :
   Parameters : BufferHandle, BufferLevelThreshold

 This function is used with BufferLevelThreshold, where the application is responsible for
 enabling the signal only when needed (not enough bytes available in buffer for copying)
 It re-enables the TC signalling mechanism, to wait for the threshold to be reached.
 It also sets the required threshold, which in turn overrides the normal multi-packet
 signal mechanism. Instead a signal-on-threshold level will be used on this buffer
******************************************************************************/
void stptiHelper_BufferLevelSignalEnable(FullHandle_t FullBufferHandle, U32 BufferLevelThreshold)
{
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    U16              DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    /* Save the Buffer Level Threshold 16 bit value in 256 byte increments - i.e discard the lowest 8 bits - for the TC */
    STSYS_WriteTCReg16LE((U32)&DMAConfig_p->BufferLevelThreshold, (BufferLevelThreshold & 0x00ffff00) >>8);
    stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize &= 0xffff;
    /* Also save the Buffer Level Threshold 16 bit value in the top half of the MultiPacketSize field for the host */
    stptiMemGet_Buffer(FullBufferHandle)->MultiPacketSize |= ((BufferLevelThreshold & 0x00ffff00) <<8);
    STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
#endif
}


/******************************************************************************
Function Name : stptiHelper_BufferLevelSignalDisable
  Description :
   Parameters : BufferHandle

 This function is used with BufferLevelThreshold, where the application is responsible for
 disabling the signal before starting the filter, only enabling it when when needed
 (not enough bytes available in buffer for copying)
******************************************************************************/
void stptiHelper_BufferLevelSignalDisable(FullHandle_t FullBufferHandle)
{
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    U16              DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    STSYS_SetTCMask16LE((void*)&(DMAConfig_p->SignalModeFlags), TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
#endif
}




/******************************************************************************
Function Name : stptiHelper_SignalEnableIfDataWaiting
  Description : stpti_SignalEnable
   Parameters : BufferHandle, MultiSize

 This function will signal again if there is ANY data in the buffer.  This
 function is used when data has been removed from buffer in non quantised lumps
 (e.g. a partical packet, partical section...).  If there is no data left in
 buffer it re-enables the TC signalling mechanism, to wait for the the new
 threshold to be reached.
******************************************************************************/
void stptiHelper_SignalEnableIfDataWaiting(FullHandle_t FullBufferHandle, U16 MultiSize)
{
    U16              DMAIdent      = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);

    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCDMAConfig_t        *DMAConfig_p   = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

    BOOL  SignalTheBuffer = FALSE;
    U16   SignallingThreshold;

    U32 Read_p  = DMAConfig_p->DMARead_p;
    U32 Write_p = DMAConfig_p->DMAQWrite_p;

    STOS_TaskLock();            /* Avoid signal enabling race condition with interrupt task */

    SignallingThreshold = (stptiMemGet_Buffer(FullBufferHandle)->BufferPacketCount + MultiSize) & 0xffff;

    /* Add multipacket size to current Buffer count and store back in Threshold value */
    STSYS_WriteTCReg16LE((U32)&DMAConfig_p->Threshold, SignallingThreshold );

    if (Write_p != Read_p)  /* there more data in the buffer! */
    {
        SignalTheBuffer = TRUE;
    }
    else /* No more data in buffer....   re-enable TC Signalling */
    {
        STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
    }

    STOS_TaskUnlock();

    /* Avoid signalling when in a task lock */
    if( SignalTheBuffer )
    {
        stptiHelper_SignalBuffer(FullBufferHandle, 0XCCCCCCCC);
    }

}


/******************************************************************************
Function Name : stptiHelper_WaitPacketTime
  Description : stpti_TCWaitPacketTime
   Parameters :

   use task delay for loader as we do not want to be dependant upon STTBX

******************************************************************************/
void stptiHelper_WaitPacketTime(void)
{

#if defined(ST_OSLINUX)
        udelay(TC_ONE_PACKET_TIME); /* 75us. De we want to de-shedule coz this will not */
#else
#if defined(STPTI_LOADER_SUPPORT) || defined(ST_UCOS_SYSTEM)
    task_delay((TC_ONE_PACKET_TIME*TICKS_PER_SECOND)/1000000);
#else
    STTBX_WaitMicroseconds(TC_ONE_PACKET_TIME); /* This API doesn't deschedule */
#endif /* #endif STPTI_LOADER_SUPPORT */
#endif /* #endif..else defined(ST_OSLINUX) */

}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stptiHelper_WaitPacketTimeDeschedule
  Description : stpti_TCWaitPacketTimeDeschedule

  Made alternative function which can be deschdule from higher prioprity tasks
   Parameters :
    delay for 320 micro seconds, enough for one packet.
******************************************************************************/
void stptiHelper_WaitPacketTimeDeschedule(void)
{
#if defined(ST_OSLINUX)
        udelay( 320 );
#else
    task_delay((5*TICKS_PER_SECOND)/15625);
#endif /* #if defined(ST_OSLINUX)  */

}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
/******************************************************************************
Function Name : stptiHelper_WindBackDMA
  Description : stpti_TCWindBackDMA
   Parameters :
******************************************************************************/
void stptiHelper_WindBackDMA(FullHandle_t DeviceHandle, U32 SlotIdent)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);

    FullHandle_t SlotHandle;
    FullHandle_t BufferListHandle;
    FullHandle_t BufferHandle;
    U32 buffer;

    stptiHelper_WaitForDMAtoComplete(DeviceHandle, SlotIdent);

    SlotHandle.word = PrivateData_p->SlotHandles_p[SlotIdent];
    if ( SlotHandle.word == STPTI_NullHandle() )
        return;

    BufferListHandle = stptiMemGet_Slot(SlotHandle)->BufferListHandle;
    if ( BufferListHandle.word == STPTI_NullHandle() )
        return;

    for (buffer = 0; buffer < stptiMemGet_List(BufferListHandle)->MaxHandles; ++buffer)
    {
        BufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[buffer];

        if ( BufferHandle.word != STPTI_NullHandle() )
        {
            U16 DMAIdent = stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent;
            STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
            TCDMAConfig_t *DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent );

            DMAConfig_p->DMAWrite_p = DMAConfig_p->DMAQWrite_p;
        }
    }
}


/******************************************************************************
Function Name : stptiHelper_SignalBuffer
  Description : stpti_SignalBuffer
   Parameters :
******************************************************************************/
void stptiHelper_SignalBuffer(FullHandle_t BufferHandle, U32 Flags)
{
    STOS_MessageQueue_t *Queue_p;
    STPTI_Buffer_t      *buffer_p;

    Queue_p = stptiMemGet_Buffer(BufferHandle)->Queue_p;

    if (Queue_p != NULL)
    {
        buffer_p = (STPTI_Buffer_t *)STOS_MessageQueueClaim(Queue_p);

        if (buffer_p != NULL)
        {
            buffer_p[0] = BufferHandle.word;
            buffer_p[1] = Flags;
            STOS_MessageQueueSend(Queue_p, buffer_p);
        }
    }
}

/******************************************************************************
Function Name : stptiHelper_WaitForDMAtoComplete
  Description : TC_WaitForDMAtoComplete
   Parameters :
******************************************************************************/
void stptiHelper_WaitForDMAtoComplete(FullHandle_t DeviceHandle, U32 SlotIdent)
{
    TCPrivateData_t    *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCMainInfo_t         *MainInfo_p  = TcHal_GetMainInfo( TC_Params_p, SlotIdent );
    U32  LoopCount;

    for ( LoopCount = 0; LoopCount < STPTI_HELPER_LOOP_COUNT_MAX; ++LoopCount )
    {
        /* if 0 then dma not in progress */
        if ( !(STSYS_ReadRegDev16LE((void*)&MainInfo_p->SlotState) & TC_MAIN_INFO_SLOT_STATE_DMA_IN_PROGRESS) )
            return;

        stptiHelper_WaitPacketTime();
    }
}



/*
 * Returns : Number of Bytes transferred, OR TC_ERROR_TRANSFERRING_BUFFERED_DATA
 * if error detected
 */
/* When linux is used DmaOrMemcpy is used to indicate whether to use a memcpy for internal copies
   or copy_to_user when th destination is pointer to be a user address.
   For Linux the source address is a ioremapped nocache address.
*/

static U32 stptiHelper_CopyChunk(U32 *SrcSize,                  /* Number useful bytes in PTIBuffer Part0 */
                                 U8 **Src_p,                    /* Start of PTIBuffer Part0  */
                                 U32 *DestSize,                 /* Space in UserBuffer0 */
                                 U8 **Dest_p,                   /* Start of UserBuffer0 */
                                 STPTI_Copy_t DmaOrMemcpy,      /* Copy Method */
                                 U32 DirectDma,
                                 FullHandle_t FullBufferHandle, /* Used if STGPDMA or STPCP transfer required*/
                                 TCDevice_t *TC_Device_p)
{
#if !defined ( STPTI_BSL_SUPPORT )
    TCPrivateData_t      *PrivateData_p =      stptiMemGet_PrivateData(FullBufferHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#endif
    U32 CopySize = MINIMUM(*SrcSize, *DestSize);

    switch (DmaOrMemcpy)
    {
#if !defined ( STPTI_BSL_SUPPORT )

#if defined(STPTI_PCP_SUPPORT)
    case STPTI_COPY_TRANSFER_BY_PCP:
        CopySize = PCPTransfer(*Dest_p, *Src_p, CopySize, FullBufferHandle);
        break;
#endif
#if defined(STPTI_GPDMA_SUPPORT)
    case STPTI_COPY_TRANSFER_BY_GPDMA:
        if (CopySize > 64)
        {
            CopySize = GPDMATransfer(*Dest_p, *Src_p, CopySize, FullBufferHandle);
        }
        else
        {
            memcpy(*Dest_p, *Src_p, CopySize);
        }
        break;
#endif
#if defined(STPTI_FDMA_SUPPORT)
    case STPTI_COPY_TRANSFER_BY_FDMA:
        CopySize = FDMATransfer(*Dest_p, *Src_p, CopySize, FullBufferHandle);
        break;
#endif
    case STPTI_COPY_TRANSFER_BY_DMA:
        CopySize = DMATransfer(*Dest_p, *Src_p, CopySize, TC_Device_p, DirectDma, TC_Params_p);
        break;
#if defined(ST_OSLINUX)
    case STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL:/* Just use a standard memcpy */
        memcpy(*Dest_p,*Src_p, CopySize);
        break;
#endif
    case STPTI_COPY_TRANSFER_BY_MEMCPY:
#endif /*#if !defined ( STPTI_BSL_SUPPORT ) */
    default:
#if defined(ST_OSLINUX)
        if(copy_to_user(*Dest_p, *Src_p, CopySize )) {
            /* Invalid user space address */
            printk(KERN_ALERT"stpti4_core: stptiHelper_CopyChunk() - Invalid user space address\n");
            CopySize = 0;
        }
#else
        memcpy(*Dest_p,*Src_p, CopySize);
#endif /* #endif defined(ST_OSLINUX)  */
        break;
    }

    if (CopySize != TC_ERROR_TRANSFERRING_BUFFERED_DATA)
    {
        *Dest_p += CopySize;
        *DestSize -= CopySize;
        *Src_p += CopySize;
        *SrcSize -= CopySize;
    }

    return CopySize;
}


/******************************************************************************
  Function Name : stptiHelper_CircToCircCopy
  Description : stpti_CircToCircCopy
  Parameters :
  Returns : Number of Bytes transferred, OR TC_ERROR_TRANSFERRING_BUFFERED_DATA
            if error detected
******************************************************************************/
U32 stptiHelper_CircToCircCopy(U32 SrcSize0,                 /* Number useful bytes in PTIBuffer Part0 */
                               U8 *Src0_p,                   /* Start of PTIBuffer Part0  */
                               U32 SrcSize1,                 /* Number useful bytes in PTIBuffer Part1 */
                               U8 *Src1_p,                   /* Start of PTIBuffer Part1  */
                               U32 *DestSize0,               /* Space in UserBuffer0 */
                               U8 **Dest0_p,                 /* Start of UserBuffer0 */
                               U32 *DestSize1,               /* Space in UserBuffer1 */
                               U8 **Dest1_p,                 /* Start of UserBuffer1 */
                               STPTI_Copy_t DmaOrMemcpy,     /* Copy Method */
                               FullHandle_t FullBufferHandle /* Used if STGPDMA or STPCP transfer required*/ )
{
    TCDevice_t *TC_Device_p = NULL;
    U32 DirectDma    = 4;
    U32 BytesCopied = 0;
    U32 CopySize = TC_ERROR_TRANSFERRING_BUFFERED_DATA;
    BOOL TCUserDmaAcquired = false;

    if (FullBufferHandle.word == STPTI_NullHandle())
    {
        /* If we don't have a buffer handle we can only use Memcpy */
        DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;
    }
    else if (DmaOrMemcpy == STPTI_COPY_TRANSFER_BY_DMA)
    {
        /* Get a free PTI DMA */
        TC_Device_p = (TCDevice_t *)stptiMemGet_Device(FullBufferHandle)->TCDeviceAddress_p;

        for (DirectDma = 1; DirectDma < 4; ++DirectDma)
        {
            if (stptiMemGet_Device(FullBufferHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] == FALSE)
            {
                /* Aquire the DMA */
                stptiMemGet_Device(FullBufferHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] = TRUE;
                TCUserDmaAcquired = true;
                break;
            }
        }

        if (DirectDma >= 4)
        {
            DmaOrMemcpy = STPTI_COPY_TRANSFER_BY_MEMCPY;
        }
    }

    while (((*DestSize0 > 0) || (*DestSize1 > 0)) &&
           ((SrcSize0 > 0) || (SrcSize1 > 0)) &&
           (BytesCopied != TC_ERROR_TRANSFERRING_BUFFERED_DATA))
    {
        if (SrcSize0 > 0)
        {
            if (*DestSize0 > 0)
                CopySize = stptiHelper_CopyChunk(&SrcSize0, &Src0_p, DestSize0, Dest0_p, DmaOrMemcpy, DirectDma, FullBufferHandle, TC_Device_p);
            else
                CopySize = stptiHelper_CopyChunk(&SrcSize0, &Src0_p, DestSize1, Dest1_p, DmaOrMemcpy, DirectDma, FullBufferHandle, TC_Device_p);
        }
        else if (SrcSize1 > 0)
        {
            if (*DestSize0 > 0)
                CopySize = stptiHelper_CopyChunk(&SrcSize1, &Src1_p, DestSize0, Dest0_p, DmaOrMemcpy, DirectDma, FullBufferHandle, TC_Device_p);
            else
                CopySize = stptiHelper_CopyChunk(&SrcSize1, &Src1_p, DestSize1, Dest1_p, DmaOrMemcpy, DirectDma, FullBufferHandle, TC_Device_p);
        }
        if (CopySize != TC_ERROR_TRANSFERRING_BUFFERED_DATA)
            BytesCopied += CopySize;
        else
            BytesCopied = TC_ERROR_TRANSFERRING_BUFFERED_DATA;
    }

    if (TCUserDmaAcquired)
    {
        /* Free the PTI DMA */
        stptiMemGet_Device(FullBufferHandle)->TCPrivateData.TCUserDma_p->DirectDmaUsed[DirectDma] = FALSE;
    }

    return BytesCopied;
}


#ifdef STPTI_PCP_SUPPORT
/******************************************************************************
Function Name :
  Description : Do a single data transfer by STPCP.
                This assumes the DMA has been aquired for use.
   Parameters :
  Returns : Number of Bytes transferred, OR TC_ERROR_TRANSFERRING_BUFFERED_DATA
            if error detected
******************************************************************************/
static U32 PCPTransfer(U8 *Dest_p, U8 *Src_p, U32 CopySize, FullHandle_t FullBufferHandle)
{
    STPCP_Transfer_t Transfer;
    STPCP_TransferId_t Tid;
    ST_ErrorCode_t Error;
    STPCP_TransferStatus_t TransferStatus;

    /* Prevents an erroneous copy count being used this time round...
     * BUT next time round we get an error anyway - so comment out, so
     * we get error quicker
     CopySize &= ~(STPTI_BUFFER_SIZE_MULTIPLE - 1);
     */

    Transfer.Mode = stptiMemGet_Buffer(FullBufferHandle)->STPCPEngineModeEncrypt;

    Transfer.Source_p = Src_p;
    Transfer.Destination_p = Dest_p;
    Transfer.Count = CopySize;

    STTBX_Print(("STPCP Handle %08X for %08X [%08X:%08X:%d]\n",
                  stptiMemGet_Buffer(FullBufferHandle)->STPCPHandle,
                  FullBufferHandle,
                  Transfer.Source_p,
                  Transfer.Destination_p,
                  CopySize));

    Error = STPCP_Transfer(stptiMemGet_Buffer(FullBufferHandle)->STPCPHandle,
                           STPCP_MODE_BLOCK,
                           &Transfer,
                           0 /* No TimeOut */,
                           &Tid,
                           &TransferStatus);

    if (Error!=ST_NO_ERROR)
    {
        STTBX_Print(("STPCP_Transfer() ERROR!! %08X\n", Error));
    }

    return ((Error != ST_NO_ERROR) ? TC_ERROR_TRANSFERRING_BUFFERED_DATA
                                   : CopySize);
}
#endif


#ifdef STPTI_GPDMA_SUPPORT
/******************************************************************************
Function Name : GPDMATransfer
  Description : Do a single data transfer by STGPDMA.
                This assumes the DMA has been aquired for use.
   Parameters :
  Returns : Number of Bytes transferred, OR TC_ERROR_TRANSFERRING_BUFFERED_DATA
            if error detected
******************************************************************************/
static U32 GPDMATransfer(U8 *Dest, U8 *Src, U32 CopySize, FullHandle_t FullBufferHandle)
{
    STGPDMA_DmaTransferId_t TrID;
    ST_ErrorCode_t Error;
    STGPDMA_DmaTransferStatus_t DMAStatus;
    STGPDMA_DmaTransfer_t LocalTransferParams;

    /* Get the memory alignment for the transfer */
    LocalTransferParams.Source.UnitSize      = 1;
    LocalTransferParams.Destination.UnitSize = 1;

    LocalTransferParams.Destination.TransferType = STGPDMA_TRANSFER_1D_INCREMENTING;
    LocalTransferParams.Destination.Address      = (U32)Dest;
    LocalTransferParams.Destination.Length      = 0;
    LocalTransferParams.Destination.Stride      = 0;

    LocalTransferParams.Source.TransferType = STGPDMA_TRANSFER_1D_INCREMENTING;
    LocalTransferParams.Source.Address      = (U32)Src;
    LocalTransferParams.Source.Length      = 0;
    LocalTransferParams.Source.Stride      = 0;

    /* Set up the transfer */
    LocalTransferParams.Count       = CopySize;
    LocalTransferParams.TimingModel = stptiMemGet_Buffer(FullBufferHandle)->STGPDMATimingModel;
    LocalTransferParams.Next = NULL;

    /* Do the transfer */
    STTBX_Print(("STGPDMA Handle %08X for %08X [%08X:%08X:%d]\n",
                  stptiMemGet_Buffer(FullBufferHandle)->STGPDMAHandle, FullBufferHandle,
                  Src, Dest, CopySize));

    Error = STGPDMA_DmaStartChannel( stptiMemGet_Buffer(FullBufferHandle)->STGPDMAHandle,
                            STGPDMA_MODE_BLOCK,
                            &LocalTransferParams,
                            1,
                            stptiMemGet_Buffer(FullBufferHandle)->STGPDMAEventLine,
                            stptiMemGet_Buffer(FullBufferHandle)->STGPDMATimeOut,
                            &TrID,
                            &DMAStatus);

    /* Check for error */

    if (Error!=ST_NO_ERROR)
    {
        STTBX_Print(("STGPDMA_DmaStartChannel() ERROR!! %08X\n", Error));
    }

    return ((Error != ST_NO_ERROR) ? TC_ERROR_TRANSFERRING_BUFFERED_DATA
                                   : CopySize);
}
#endif


#if defined(STPTI_FDMA_SUPPORT)
/******************************************************************************
Function Name : FDMATransfer
  Description : Do a single data transfer by STFDMA.
                This assumes the DMA has been aquired for use.
   Parameters :
  Returns : Number of Bytes transferred, OR TC_ERROR_TRANSFERRING_BUFFERED_DATA
            if error detected
******************************************************************************/
static U32 FDMATransfer(U8 *Dest, U8 *Src, U32 CopySize, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error;

    FDMANode_p->Node.NumberBytes = CopySize;
    FDMANode_p->Node.SourceAddress_p = (void*) ((U32)STOS_VirtToPhys(Src));
    FDMANode_p->Node.DestinationAddress_p = (void*)((U32)STOS_VirtToPhys(Dest));
    FDMANode_p->Node.Length = CopySize;

    Error = STFDMA_StartGenericTransfer(&FDMAGenericTransferParams, &FDMATransferId);

    if (Error!=ST_NO_ERROR)
    {
        STTBX_Print(("STFDMA_StartGenericTransfer() ERROR!! %08X\n", Error));
    }

    return ((Error != ST_NO_ERROR) ? TC_ERROR_TRANSFERRING_BUFFERED_DATA
                                   : CopySize);
}
#endif


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : DMATransfer
  Description : Do a single data transfer by PTI DMA.
                This assumes the DMA has been aquired for use.
   Parameters :
  Returns : Number of Bytes transferred, OR TC_ERROR_TRANSFERRING_BUFFERED_DATA
            if error detected
******************************************************************************/
static U32 DMATransfer(U8 *Dest, U8 *Src, U32 CopySize, TCDevice_t *TC_Device_p, U32 DirectDma, STPTI_TCParameters_t *TC_Params_p)
{
    volatile U32 *Read = NULL;
    U32 Write;
    /* Used to ensure termination of this routine */
    U32 DelayCount = 0;
    U32 Source_PAddr, Destination_PAddr;

    Source_PAddr      = (U32)STOS_VirtToPhys(Src);              /* Source Start */
    Write             = (U32)STOS_VirtToPhys((Src+CopySize));   /* Source End (extra brackets are required for STOS Macro) */
    Destination_PAddr = (U32)STOS_VirtToPhys(Dest);             /* Destination */

    /* PTI DMA only allows BlockMoves if the Source and Destination have the same */
    /* offset from a word boundry.  This is a bug which really renders the PTI DMA */
    /* useless for general use. */
    if( Source_PAddr%4 != Destination_PAddr%4 ) return (TC_ERROR_TRANSFERRING_BUFFERED_DATA);

    switch (DirectDma)
    {
        case 1:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~TC_GLOBAL_DMA_ENABLE_DMA1) &
                                                                 TC_GLOBAL_DMA_ENABLE_MASK,
                                                                 TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1Base, Source_PAddr & ~0X0F, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1Top, 0XFFFFFFFF, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1Write, Write, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1Read, Source_PAddr, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1Setup, 0X02, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1Holdoff, 0, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA1CDAddr, Destination_PAddr, TC_Params_p);
                TC_Device_p->DMAempty_EN &= ~0X01;
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                TC_Device_p->DMAEnable = (~TC_GLOBAL_DMA_ENABLE_DMA1) & TC_GLOBAL_DMA_ENABLE_MASK;
                TC_Device_p->DMA1Base = Source_PAddr & ~0X0F;
                TC_Device_p->DMA1Top = 0XFFFFFFFF;
                TC_Device_p->DMA1Write = Write;
                TC_Device_p->DMA1Read = Source_PAddr;
                TC_Device_p->DMA1Setup = 0X02;
                TC_Device_p->DMA1Holdoff = 0;
                TC_Device_p->DMA1CDAddr = Destination_PAddr;
                TC_Device_p->DMAempty_EN &= ~0X01;
                TC_Device_p->DMAEnable = TC_GLOBAL_DMA_ENABLE_MASK;
            } STPTI_CRITICAL_SECTION_END;
#endif

            Read = &(TC_Device_p->DMA1Read);
            break;

        case 2:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~TC_GLOBAL_DMA_ENABLE_DMA2) &
                                                                 TC_GLOBAL_DMA_ENABLE_MASK,
                                                                 TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2Base, Source_PAddr & ~0X0F, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2Top, 0XFFFFFFFF, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2Write, Write, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2Read, Source_PAddr, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2Setup, 0X02, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2Holdoff, 0, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA2CDAddr, Destination_PAddr, TC_Params_p);
                TC_Device_p->DMAempty_EN &= ~0X02;
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                TC_Device_p->DMAEnable = (~TC_GLOBAL_DMA_ENABLE_DMA2) & TC_GLOBAL_DMA_ENABLE_MASK;
                TC_Device_p->DMA2Base = Source_PAddr & ~0X0F;
                TC_Device_p->DMA2Top = 0XFFFFFFFF;
                TC_Device_p->DMA2Write = Write;
                TC_Device_p->DMA2Read = Source_PAddr;
                TC_Device_p->DMA2Setup = 0X02;
                TC_Device_p->DMA2Holdoff = 0;
                TC_Device_p->DMA2CDAddr = Destination_PAddr;
                TC_Device_p->DMAempty_EN &= ~0X02;
                TC_Device_p->DMAEnable = TC_GLOBAL_DMA_ENABLE_MASK;
            } STPTI_CRITICAL_SECTION_END;
#endif

            Read = &(TC_Device_p->DMA2Read);
            break;

        case 3:
#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
            STPTI_CRITICAL_SECTION_BEGIN {
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, (~TC_GLOBAL_DMA_ENABLE_DMA3) &
                                                                 TC_GLOBAL_DMA_ENABLE_MASK,
                                                                 TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3Base, Source_PAddr & ~0X0F, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3Top, 0XFFFFFFFF, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3Write, Write, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3Read, Source_PAddr, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3Setup, 0X02, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3Holdoff, 0, TC_Params_p);
                WriteDMAReg32((void *)&TC_Device_p->DMA3CDAddr, Destination_PAddr, TC_Params_p);
                TC_Device_p->DMAempty_EN &= ~0X04;
                WriteDMAReg32((void *)&TC_Device_p->DMAEnable, TC_GLOBAL_DMA_ENABLE_MASK, TC_Params_p);
            } STPTI_CRITICAL_SECTION_END;
#else
            STPTI_CRITICAL_SECTION_BEGIN {
                TC_Device_p->DMAEnable = (~TC_GLOBAL_DMA_ENABLE_DMA3) & TC_GLOBAL_DMA_ENABLE_MASK;
                TC_Device_p->DMA3Base = Source_PAddr & ~0X0F;
                TC_Device_p->DMA3Top = 0XFFFFFFFF;
                TC_Device_p->DMA3Write = Write;
                TC_Device_p->DMA3Read = Source_PAddr;
                TC_Device_p->DMA3Setup = 0X02;
                TC_Device_p->DMA3Holdoff = 0;
                TC_Device_p->DMA3CDAddr = Destination_PAddr;
                TC_Device_p->DMAempty_EN &= ~0X04;
                TC_Device_p->DMAEnable = TC_GLOBAL_DMA_ENABLE_MASK;
            } STPTI_CRITICAL_SECTION_END;
#endif

            Read = &(TC_Device_p->DMA3Read);
            break;

        case 0:
        default:
            /* Cause an immediate error return */
            Read       = NULL;
            DelayCount = 0;
            break;
    }

    /* If we haven't had an error by now...wait for dma completion */
    if (Read != NULL)
    {
        /* Wait for the transfer to complete (hope for faster than 1MByte/s).  DelayCount is units of 0.5ms */
        DelayCount = CopySize/500;
        if ( DelayCount < 10 ) DelayCount = 10;       /* minimum timeout is 5ms */

#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
        while ( (ReadDMAReg32((void *)Read, TC_Params_p) != Write) && (DelayCount > 0 ) )
#else
        while ( (*Read != Write) && (DelayCount > 0 ) )
#endif
        {

#if defined( ST_OSLINUX )
            udelay( 500 );
#else
            task_delay((TICKS_PER_SECOND)/2000); /* wait 0.5ms & yield to OS so that other tasks can run */
#endif
            DelayCount--;
        }
    }

    return (( DelayCount > 0 ) ? CopySize : TC_ERROR_TRANSFERRING_BUFFERED_DATA);
}


/* ------------------------------------------------------------------------- */


/******************************************************************************
Function Name : stptiHelper_ClearLookupTable
  Description : TC_ClearLookupTable
   Parameters : see: extended.c - stptiHAL_HardwarePause()
******************************************************************************/
void stptiHelper_ClearLookupTable(FullHandle_t DeviceHandle)
{
    TCPrivateData_t *PrivateData_p = stptiMemGet_PrivateData(DeviceHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U32 slot;

    /* for EVERY session on the PTI cell */
    for (slot = 0; slot < TC_Params_p->TC_NumberSlots; ++slot)
    {
        stptiHelper_SetLookupTableEntry(DeviceHandle, slot, TC_INVALID_PID);
    }
}
#endif /*#if !defined ( STPTI_BSL_SUPPORT ) */

/* EOF  -------------------------------------------------------------------- */

