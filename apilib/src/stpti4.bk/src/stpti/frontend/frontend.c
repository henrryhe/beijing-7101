/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2007. All rights reserved.

   File Name: frontend.c
 Description: Provides low level interface to STFE

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX)  */

#include "stos.h"
#include "stcommon.h"
#include "stpti.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "memget.h"

#if defined (STPTI_FRONTEND_HYBRID)

#include "tsgdma.h"
#include "stfe.h"

static U8  STFE_First = 1;

/* Private Function Prototypes --------------------------------------------- */
/* Functions --------------------------------------------------------------- */

/******************************************************************************
Function Name : stpti_FrontendHALGetStatus
  Description : Get the IB & DMA status for the hardware of the stream ID
   Parameters : FullFrontendHandle
                FrontendStatus_p
                StreamID
******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendGetStatus( FullHandle_t FullFrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p, STPTI_StreamID_t StreamID )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 Status,Lock;

    Status = StfeHal_ReadStatusReg( StreamID );

    if ( STFE_STATUS_FIFO_OVERFLOW & Status )
        FrontendStatus_p->FifoOverflow = TRUE;
    if ( STFE_STATUS_BUFFER_OVERFLOW & Status )
        FrontendStatus_p->BufferOverflow = TRUE;
    if ( STFE_STATUS_OUT_OF_ORDER_RP & Status )
        FrontendStatus_p->OutOfOrderRP = TRUE;
    if ( STFE_STATUS_PKT_OVERFLOW & Status )
        FrontendStatus_p->PktOverflow = TRUE;

    FrontendStatus_p->ErrorPackets = ( (Status & STFE_STATUS_ERROR_PKTS) >> STFE_STATUS_ERROR_PKTS_POSITION);
    FrontendStatus_p->ShortPackets = ( (Status & STFE_STATUS_ERROR_SHORT_PKTS) >> STFE_STATUS_ERROR_SHORT_PKTS_POSITION);

    FrontendStatus_p->StreamID = StreamID;

    /* To find out if we have a Lock then look at the pointers of the DMA
       Unfortunately if we have locked stream but no matched PIDs the lock will return FALSE */

    Lock = StfeHal_ReadWPReg(StreamID);
    STOS_TaskDelayUs(1000);
    if (Lock == StfeHal_ReadWPReg(StreamID))
    {
        FrontendStatus_p->Lock = FALSE;
    }
    else
    {
        FrontendStatus_p->Lock = TRUE;
    }

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_FrontendCloneInput
  Description : Setup TSGDMA to route the same input to two sessions
   Parameters : FullFrontendHandle
                FullPrimaryFrontendHandle
******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendCloneInput( FullHandle_t FullFrontendHandle, FullHandle_t FullPrimaryFrontendHandle )
{
    ST_ErrorCode_t Error    = ST_NO_ERROR;
    Device_t *  Device_p   = stptiMemGet_Device(FullPrimaryFrontendHandle);

    STTBX_Print(( "stptiHAL_FrontendCloneInput pri device 0x%08x\n", (U32)Device_p->TCDeviceAddress_p));

    /* SW 20/07/2007 - The Partion is not used under linux and now used to pass the TCDeviceAddress_p to the TSGDMA driver before
    it's remapped to a virtual address. This is only intended as a temporary mod and we should look for a more perminant solution ASAP */
    TsgdmaHAL_WritePTILiveDestination(Device_p->StreamID,
#if defined(ST_OSLINUX)
    (STPTI_DevicePtr_t) Device_p->DriverPartition_p
#else
    Device_p->TCDeviceAddress_p
#endif
    );

    Device_p = stptiMemGet_Device(FullFrontendHandle);

    STTBX_Print(( "stptiHAL_FrontendCloneInput sec device 0x%08x\n", (U32)Device_p->TCDeviceAddress_p));

    TsgdmaHAL_WritePTILiveDestination(Device_p->StreamID,
#if defined(ST_OSLINUX)
    (STPTI_DevicePtr_t) Device_p->DriverPartition_p
#else
    Device_p->TCDeviceAddress_p
#endif
    );

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_TSINLinkToPTI
  Description : Setup the Parameters to the STFE IB
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_TSINLinkToPTI( FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle )
{
    ST_ErrorCode_t Error         = ST_NO_ERROR;
    Frontend_t *   Frontend_p    = stptiMemGet_Frontend( FullFrontendHandle );
    Device_t *     Device_p      = stptiMemGet_Device(FullSessionHandle);
    U32            PIDBitOffset  = 0;
    U8             PIDBitSize    = DVB_PID_BIT_SIZE;
    U8             Lock          = 0;
    U8             Drop          = 0;
    U8             Token         = 0;
    U32            PktSize       = STFE_DVB_DMA_PKT_SIZE;

    STTBX_Print(( "Calling stptiHAL_TSINLinkToPTI Frontend handle 0x%08x\n", FullFrontendHandle.word));

    /* Disable the IB for safety */
    StfeHal_Enable (  Device_p->StreamID, FALSE );

    /* Wait a short while */
    STOS_TaskDelayUs(100);

    /* Enable tagging and tag position based on params */
    StfeHal_TAGBytesConfigure ( Device_p->StreamID, Device_p->StreamID);

    /* Configure the pointers for the PID lookup table & clear PIDs */
    memset( Frontend_p->PIDTableStart_p, 0x00, STFE_PID_RAM_SIZE );

    if (STFE_First)
    {
        /* Clear Internal RAM */
        memset( (void*)STFE_INTERNAL_RAM_BASE_ADDRESS, 0x00, STFE_INTERNAL_RAM_SIZE);
    }

    StfeHal_WritePIDBase ( Device_p->StreamID, (U32) STOS_VirtToPhys(Frontend_p->PIDTableStart_p) );

    /* Calculate internal parameters based on transport type etc */
    switch ( Frontend_p->UserParams.PktLength )
    {
        case (STPTI_FRONTEND_PACKET_LENGTH_DVB):
        default:
            PIDBitOffset = STFE_DVB_PID_BIT_OFFSET;
            PIDBitSize   = DVB_PID_BIT_SIZE;
            PktSize      = STFE_DVB_DMA_PKT_SIZE;
            Lock         = STFE_DVB_DEFAULT_LOCK;
            Drop         = STFE_DVB_DEFAULT_DROP;
            Token        = SYNC_BYTE;

          break;
    }

    /* Enable PID filtering on the input block - PID offset is dependant on transport */
    StfeHal_PIDFilterConfigure ( Device_p->StreamID, PIDBitSize, PIDBitOffset, Frontend_p->PIDFilterEnabled );

    StfeHal_WriteTagBytesHeader ( Device_p->StreamID, (U16)Device_p->StreamID );

    StfeHal_TAGCounterSelect( Device_p->StreamID, (U8) (Frontend_p->UserParams.u.TSIN.ClkRvSrc));

    /* Configure the input parameters based on transport type */
    StfeHal_WriteSLD ( Device_p->StreamID, Lock, Drop, Token );

    /* Configure the input format register */
    {
        U32 IFormat = 0;

        if ( TRUE == Frontend_p->UserParams.u.TSIN.SerialNotParallel )
        {
            IFormat  = STFE_INPUT_FORMAT_SERIAL_NOT_PARALLEL;
            /* We may need to provide option to the API at a later date */
            IFormat |= STFE_INPUT_FORMAT_BYTE_ENDIANNESS;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.AsyncNotSync )
        {
            IFormat |= STFE_INPUT_FORMAT_ASYNC_NOT_SYNC;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.AlignByteSOP )
        {
            IFormat |= STFE_INPUT_FORMAT_ALIGN_BYTE_SOP;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.InvertTSClk )
        {
            IFormat |= STFE_INPUT_FORMAT_INVERT_TS_CLK;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.IgnoreErrorInByte )
        {
            IFormat |= STFE_INPUT_FORMAT_IGNORE_ERROR_BYTE;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.IgnoreErrorInPkt )
        {
            IFormat |= STFE_INPUT_FORMAT_IGNORE_ERROR_PKT;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.IgnoreErrorAtSOP )
        {
            IFormat |= STFE_INPUT_FORMAT_IGNORE_ERROR_SOP;
        }

        StfeHal_WriteInputFormat( Device_p->StreamID, IFormat);
    }

    StfeHal_WritePktLength ( Device_p->StreamID, Frontend_p->UserParams.PktLength );

    /* Setup the DMA block */
    StfeHal_WriteDMAModeCircular ( Device_p->StreamID );

    if (STFE_First)
    {
        StfeHal_WriteDMAPointerBase ( Device_p->StreamID, 0x00000000 );
        STFE_First = 0;
    }

    {
        U32 BufferSize = ( Frontend_p->AllocatedNumOfPkts * PktSize );

        STTBX_Print(( "StfeHal_WriteInternalRAMRecord BufferSize %u NumPkts %u PktSize %u\n", BufferSize, Frontend_p->AllocatedNumOfPkts, PktSize));

        StfeHal_WriteInternalRAMRecord ( Device_p->StreamID,
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p),
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + (BufferSize/2) - 1),
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + BufferSize/2),
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + BufferSize-1),
                                         PktSize );

        STTBX_Print(( "RAM BASE  0x%08x\n",(U32) STOS_VirtToPhys(Frontend_p->RAMStart_p) ));
        STTBX_Print(( "RAM TOP   0x%08x\n",(U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + (BufferSize/2) - 1) ));
        STTBX_Print(( "RAM NBASE 0x%08x\n",(U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + BufferSize/2) ));
        STTBX_Print(( "RAM NTOP  0x%08x\n",(U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + BufferSize-1) ));

        StfeHal_WriteDMABusSize( Device_p->StreamID, Frontend_p->UserParams.u.TSIN.MemoryPktNum );
    }

    /* Setup remaining internal RAM pointers */
    StfeHal_ConfigureIBInternalRAM( Device_p->StreamID );

    /* Init TSGDMA */
    TsgdmaHAL_SlimCoreInit();

    /* Setup TSGDMA pointers etc*/
    TsgdmaHAL_WriteLiveDMA(Device_p->StreamID,
                           (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p),
                           (Frontend_p->UserParams.u.TSIN.MemoryPktNum * PktSize),
                           Frontend_p->UserParams.PktLength);

    /* Configure TSGDMA routing */

    /* SW 20/07/2007 - The Partion is not used under linux and now used to pass the TCDeviceAddress_p to the TSGDMA driver before
    it's remapped to a virtual address. This is only intended as a temporary mod and we should look for a more perminant solution ASAP */
    TsgdmaHAL_WritePTILiveDestination(Device_p->StreamID,
#if defined(ST_OSLINUX)
    (STPTI_DevicePtr_t) Device_p->DriverPartition_p
#else
    Device_p->TCDeviceAddress_p
#endif
    );

    /* Start TSGDMA channel */
    TsgdmaHAL_StartLiveChannel(Device_p->StreamID);

    /* Enable the IB if set */
    StfeHal_Enable (  Device_p->StreamID, Frontend_p->UserParams.u.TSIN.InputBlockEnable );

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_SWTSLinkToPTI
  Description : Start the slimcore
   Parameters :
******************************************************************************/
void stptiHAL_SWTSLinkToPTI(void)
{
    STTBX_Print(( "Calling stptiHAL_SWTSLinkToPTI\n"));

    /* ORDER SENSITIVE!

       The slim core is initialised before its interrupt is installed, but protects
       against unhandled interrupts by clearing the mailbox and mask responsible for
       generating the interrupt before loading the slim core firmware.

    */

    /* Init TSGDMA */
    TsgdmaHAL_SlimCoreInit();

    TSGDMA_InterruptInstall();
}

/******************************************************************************
Function Name : stptiHAL_FrontendReset
  Description : Reset the stfe IB
   Parameters : FullFrontendHandle
                StreamID
******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendReset( FullHandle_t FullFrontendHandle, STPTI_StreamID_t StreamID )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    BOOL WasEnabled = FALSE;

    STTBX_Print(( "stptiHAL_FrontendReset Frontend handle 0x%08x StreamID 0x%04x\n", FullFrontendHandle.word, StreamID));

    if (StfeHal_ReadSystemReg(StreamID))
    {
        /* Disable the IB */
        StfeHal_Enable (  StreamID, FALSE );

        WasEnabled = TRUE;
    }
    STOS_TaskDelayUs(1000);

    StfeHal_Reset ( StreamID );

    /* Restart the tsgdma */
    TsgdmaHAL_StartLiveChannel(StreamID);

    if ( WasEnabled == TRUE )
    {
        /* Enable the IB if set */
        StfeHal_Enable ( StreamID, TRUE );
    }

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_FrontendSWTSSetPace
  Description : Configure the TSGDMA SWTS Rate
   Parameters : FullSessionHandle
                PaceBps
******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendSWTSSetPace( FullHandle_t FullSessionHandle, U32 PaceBps )
{
    Device_t * Device_p = stptiMemGet_Device(FullSessionHandle);

    STTBX_Print(("stptiHAL_FrontendSWTSSetPace Session handle 0x%08x PaceBps %u\n", FullSessionHandle.word, PaceBps));

    TsgdmaHAL_WriteSWTSPace( Device_p->StreamID, PaceBps );

    return ST_NO_ERROR;
}

/******************************************************************************
Function Name : stptiHAL_FrontendSetParams
  Description : Configure the IB -
   Parameters : FullFrontendHandle
                FullSessionHandle
******************************************************************************/
void stptiHAL_FrontendSetParams( FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle )
{
    Frontend_t *   Frontend_p    = stptiMemGet_Frontend( FullFrontendHandle );
    Device_t *     Device_p      = stptiMemGet_Device(FullSessionHandle);
    U32            PIDBitOffset;
    U8             PIDBitSize    = DVB_PID_BIT_SIZE;
    U8             Lock          = 0;
    U8             Drop          = 0;
    U8             Token         = 0;
    U32            PktSize       = STFE_DVB_DMA_PKT_SIZE; /* Default size for DVB */

    STTBX_Print(( "Calling stptiHAL_FrontendSetParams Frontend handle 0x%08x\n", FullFrontendHandle.word));

    /* If the channel is running then halt the STFE */
    if (StfeHal_ReadSystemReg(Device_p->StreamID))
    {
        U32 Count;

        /* Disable the IB */
        StfeHal_Enable (  Device_p->StreamID, FALSE );

        /* Wait for the TSGDMA to finish - This can be changed to use ints later */
        for(Count=0; Count<100; Count++)
        {
            if ( TsgdmaHAL_ReadDREQStatus() & (1 << (Device_p->StreamID & 0x3f)))
            {
                STOS_TaskDelayUs(10);
            }
            else
            {
                break;
            }
        }
    }

    /* Calculate internal parameters based on transport type etc */
    switch ( Frontend_p->UserParams.PktLength )
    {
        case (STPTI_FRONTEND_PACKET_LENGTH_DVB):
        default:
            PIDBitOffset = STFE_DVB_PID_BIT_OFFSET;
            Lock = STFE_DVB_DEFAULT_LOCK;
            Drop = STFE_DVB_DEFAULT_DROP;
            Token = SYNC_BYTE;
            PktSize = STFE_DVB_DMA_PKT_SIZE;
          break;
    }

    /* Enable tagging and tag header */
    StfeHal_TAGBytesConfigure ( Device_p->StreamID, Device_p->StreamID);

    StfeHal_WritePIDBase ( Device_p->StreamID, (U32) STOS_VirtToPhys(Frontend_p->PIDTableStart_p) );

    /* Enable PID filtering on the input block - PID offset is dependant on transport */
    StfeHal_PIDFilterConfigure ( Device_p->StreamID, PIDBitSize, PIDBitOffset, Frontend_p->PIDFilterEnabled );

    StfeHal_WriteTagBytesHeader ( Device_p->StreamID, (U16)Device_p->StreamID );

    StfeHal_TAGCounterSelect( Device_p->StreamID, (U8) (Frontend_p->UserParams.u.TSIN.ClkRvSrc));

    /* Configure the input parameters based on transport type */
    StfeHal_WriteSLD ( Device_p->StreamID, Lock, Drop, Token );

    {
        U32 IFormat = 0;

        if ( TRUE == Frontend_p->UserParams.u.TSIN.SerialNotParallel )
        {
            IFormat  = STFE_INPUT_FORMAT_SERIAL_NOT_PARALLEL;
            /* We may need to provide option to the API at a later date */
            IFormat |= STFE_INPUT_FORMAT_BYTE_ENDIANNESS;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.AsyncNotSync )
        {
            IFormat |= STFE_INPUT_FORMAT_ASYNC_NOT_SYNC;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.AlignByteSOP )
        {
            IFormat |= STFE_INPUT_FORMAT_ALIGN_BYTE_SOP;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.InvertTSClk )
        {
            IFormat |= STFE_INPUT_FORMAT_INVERT_TS_CLK;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.IgnoreErrorInByte )
        {
            IFormat |= STFE_INPUT_FORMAT_IGNORE_ERROR_BYTE;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.IgnoreErrorInPkt )
        {
            IFormat |= STFE_INPUT_FORMAT_IGNORE_ERROR_PKT;
        }
        if ( TRUE == Frontend_p->UserParams.u.TSIN.IgnoreErrorAtSOP )
        {
            IFormat |= STFE_INPUT_FORMAT_IGNORE_ERROR_SOP;
        }

        StfeHal_WriteInputFormat( Device_p->StreamID, IFormat);
    }

    StfeHal_WritePktLength ( Device_p->StreamID, Frontend_p->UserParams.PktLength );

    /* Setup the DMA block operating mode */
    StfeHal_WriteDMAModeCircular ( Device_p->StreamID );

    if (STFE_First)
    {
        StfeHal_WriteDMAPointerBase ( Device_p->StreamID, 0x00000000 );
        STFE_First = 0;
    }

    {
        U32 BufferSize = ( Frontend_p->AllocatedNumOfPkts * PktSize );

        STTBX_Print(( "stptiHAL_FrontendSetParams BufferSize %u NumPkts %u PktSize %u\n", BufferSize, Frontend_p->AllocatedNumOfPkts, PktSize));

        StfeHal_WriteInternalRAMRecord ( Device_p->StreamID,
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p),
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + (BufferSize/2) - 1),
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + BufferSize/2),
                                         (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p + BufferSize-1),
                                         PktSize );

        StfeHal_WriteDMABusSize( Device_p->StreamID, Frontend_p->UserParams.u.TSIN.MemoryPktNum );
    }

    /* Setup remaining internal RAM pointers */
    StfeHal_ConfigureIBInternalRAM( Device_p->StreamID );

    /* Init TSGDMA */
    TsgdmaHAL_SlimCoreInit();

    /* Setup TSGDMA pointers etc*/
    TsgdmaHAL_WriteLiveDMA(Device_p->StreamID,
                           (U32) STOS_VirtToPhys(Frontend_p->RAMStart_p),
                           (Frontend_p->UserParams.u.TSIN.MemoryPktNum * PktSize),
                           Frontend_p->UserParams.PktLength);

    /* Configure TSGDMA routing */
    /* SW 20/07/2007 - The Partion is not used under linux and now used to pass the TCDeviceAddress_p to the TSGDMA driver before
    it's remapped to a virtual address. This is only intended as a temporary mod and we should look for a more perminant solution ASAP */
    TsgdmaHAL_WritePTILiveDestination(Device_p->StreamID,
#if defined(ST_OSLINUX)
    (STPTI_DevicePtr_t) Device_p->DriverPartition_p
#else
    Device_p->TCDeviceAddress_p
#endif
    );


    /* Configure the TSGDMA channel */
    TsgdmaHAL_StartLiveChannel(Device_p->StreamID);

    /* Enable the IB if set */
    StfeHal_Enable (  Device_p->StreamID, Frontend_p->UserParams.u.TSIN.InputBlockEnable );
}

/******************************************************************************
Function Name : stptiHAL_FrontendUnlink
  Description : Disable the Frontend

   Parameters : FullFrontendHandle

******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendUnlink( FullHandle_t FullFrontendHandle, STPTI_StreamID_t StreamID )
{
    ST_ErrorCode_t Error    = ST_NO_ERROR;
    Device_t *     Device_p = stptiMemGet_Device(FullFrontendHandle);

    switch ((stptiMemGet_Frontend(FullFrontendHandle))->Type)
    {
    case ( STPTI_FRONTEND_TYPE_TSIN ):
            TsgdmaHAL_ClearPTILiveDestination(Device_p->StreamID, Device_p->TCDeviceAddress_p);
        StfeHal_Enable ( StreamID, FALSE);
        TsgdmaHAL_ClearPTILiveDestination(Device_p->StreamID, Device_p->TCDeviceAddress_p);
        break;

    case ( STPTI_FRONTEND_TYPE_SWTS ):
        Error = TSGDMA_InterruptUnInstall();
        break;
    default:
     break;
    }

    return Error;
}

/******************************************************************************
Function Name : stptiHAL_FrontendClearPid
  Description : Clear the Frontend PID search engine with requested Pid number.
                The PID is only cleared if requested by only one PTI session.

   Parameters : FullSlotHandle
                Pid
******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendClearPid( FullHandle_t FullSlotHandle, STPTI_Pid_t Pid )
{
    ST_ErrorCode_t Error               = ST_NO_ERROR;
    Frontend_t *   Frontend_p;
    Device_t *     Device_p            = stptiMemGet_Device(FullSlotHandle);
    FullHandle_t   FullFrontendHandle;

    STTBX_Print(( "stptiHAL_FrontendClearPid Pid 0x%04x\n", Pid));
    /* Only Clear the Pid if the Device is associated with a Frontend object */
    if (STPTI_NullHandle() != Device_p->FrontendListHandle.word)
    {
        /* Get the Frontend structure */
        FullFrontendHandle.word = ( &stptiMemGet_List( Device_p->FrontendListHandle )->Handle )[0];
        Frontend_p              = stptiMemGet_Frontend( FullFrontendHandle );

        /* Check if the PID filtering is enabled */
        if ( (Pid != STPTI_WildCardPid()) && (Frontend_p->PIDFilterEnabled = TRUE))
        {
            BOOL ClonePidFound = FALSE;

            STTBX_Print(( "stptiHAL_FrontendClearPid - Clearing PID 0x%04x on Stream 0x%04x\n", Pid, (U32)Device_p->StreamID));

            /* Only clear the PID if you can't find it on both the prinmary & clone session */
            if ( STPTI_NullHandle() != Frontend_p->CloneFrontendHandle.word )
            {
                FullHandle_t SlotListHandle = stptiMemGet_Session( Frontend_p->CloneFrontendHandle )->AllocatedList[OBJECT_TYPE_SLOT];
                FullHandle_t SlotHandle;
                U16          MaxSlots       = stptiMemGet_List( SlotListHandle )->MaxHandles;
                U16          SlotIndex      = 0;

                /*
                 * Iterate over all allocated slots until we find a matching pid,
                 * or run out of Slots.
                 */
                while ( SlotIndex < MaxSlots )
                {
                    SlotHandle.word = ( &stptiMemGet_List( SlotListHandle )->Handle )[SlotIndex];

                    if ( SlotHandle.word != STPTI_NullHandle() )
                    {
                        if ( Pid == (stptiMemGet_Slot( SlotHandle ))->Pid)
                        {
                            ClonePidFound = TRUE;
                            break;
                        }
                    }
                    ++SlotIndex;
                }
            }
            if ( FALSE == ClonePidFound )
            {
                StfeHal_ClearPid( (U32)Frontend_p->PIDTableStart_p, Pid);
            }
        }
    }
    return Error;
}

/******************************************************************************
Function Name : stptiHAL_FrontendSetPid
  Description : Set the Frontend PID search engine with requested Pid number

                If there is a wildcard PID set then PID filtering on the STFE
                will be disabled and the PTI(s) will receive full data throughput.

   Parameters : FullSlotHandle
                Pid
******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendSetPid( FullHandle_t FullSlotHandle, STPTI_Pid_t Pid )
{
    ST_ErrorCode_t Error               = ST_NO_ERROR;
    Frontend_t *   Frontend_p;
    Device_t *     Device_p            = stptiMemGet_Device(FullSlotHandle);
    BOOL           EnableFilter        = TRUE; /* Assume we want PID filtering unless we find a wildcard PID */
    FullHandle_t   FullFrontendHandle;

    STTBX_Print(( "stptiHAL_FrontendSetPid Pid 0x%04x\n", Pid));

    /* Only Set the Pid if the Device is associated with a Frontend object */
    if (STPTI_NullHandle() != Device_p->FrontendListHandle.word)
    {
        /* Get the Frontend structure */
        FullFrontendHandle.word = ( &stptiMemGet_List( Device_p->FrontendListHandle )->Handle )[0];
        Frontend_p              = stptiMemGet_Frontend( FullFrontendHandle );

        if ( Pid == STPTI_WildCardPid())
        {
            Frontend_p->WildCardPidCount++;
        }

        /* Make sure this is the primary handle in the event of cloning an input */
        if ( (STPTI_NullHandle() != Frontend_p->CloneFrontendHandle.word) &&
             (TRUE == (stptiMemGet_Frontend( Frontend_p->CloneFrontendHandle))->IsAssociatedWithHW) )
        {
            Frontend_p = stptiMemGet_Frontend( Frontend_p->CloneFrontendHandle);
        }

        /* Check if the PID filtering is enabled */
        if ( Pid == STPTI_WildCardPid())
        {
            /* Disable the PID filter */
            EnableFilter = FALSE;
        }
        else if ( Pid != STPTI_InvalidPid() && Pid != STPTI_NegativePid() )
        {
            STTBX_Print(( "stptiHAL_FrontendSetPid - Setting PID 0x%04x on Stream 0x%04x\n", Pid, (U32)Device_p->StreamID));
            StfeHal_SetPid( (U32)Frontend_p->PIDTableStart_p, Pid);
        }

        /* Check if its necessary to disable the PID filtering */
        if ( FALSE == EnableFilter )
        {
            Frontend_p->PIDFilterEnabled = FALSE;
            /* Set all pid filtering on  - effectively disabling the filter but eliminating the need to halt the frontend */
            memset( Frontend_p->PIDTableStart_p, 0xFF, STFE_PID_RAM_SIZE );
        }
    }
    return Error;
}

/******************************************************************************
Function Name : stptiHAL_SyncFrontendPids
  Description : Sync the Frontend PID search engine with the PTI PID lookup table

                If there is a cloned session set PIDs for both in the single table.
                If there is a wildcard PID set then PID filtering on the STFE
                will be disabled and the PTI(s) will receive full data throughput.

   Parameters : FullFrontendHandle
                FullSessionHandle
******************************************************************************/
void stptiHAL_SyncFrontendPids( FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle )
{
    Frontend_t *   Frontend_p     = stptiMemGet_Frontend( FullFrontendHandle );
#if defined (STTBX_PRINT)
    Device_t *     Device_p       = stptiMemGet_Device(FullSessionHandle);
#endif
    FullHandle_t   SlotListHandle = stptiMemGet_Session( FullSessionHandle )->AllocatedList[OBJECT_TYPE_SLOT];
    BOOL EnableFilter = TRUE; /* Assume we want PID filtering unless we find a wildcard PID */

    STTBX_Print(( "Calling stptiHAL_SyncFrontendPids\n"));

    if ( SlotListHandle.word != STPTI_NullHandle())
    {
        FullHandle_t SlotHandle;
        U16 MaxSlots = stptiMemGet_List( SlotListHandle )->MaxHandles;
        U16 SlotIndex = 0;

        /* Find the primary Frontend handle */
        if (STPTI_NullHandle() != Frontend_p->CloneFrontendHandle.word)
        {
            if (TRUE == (stptiMemGet_Frontend( Frontend_p->CloneFrontendHandle))->IsAssociatedWithHW)
            {
                Frontend_p = stptiMemGet_Frontend( Frontend_p->CloneFrontendHandle);
            }
        }

        if ( FALSE == Frontend_p->PIDFilterEnabled )
        {
            /* Zero the PID table before rebuilding */
            memset( Frontend_p->PIDTableStart_p, 0x00, STFE_PID_RAM_SIZE );
        }

        /*
         * Iterate over all allocated slots until we find a matching pid,
         * or run out of Slots.
         */
        while ( SlotIndex < MaxSlots )
        {
            SlotHandle.word = ( &stptiMemGet_List( SlotListHandle )->Handle )[SlotIndex];

            if ( SlotHandle.word != STPTI_NullHandle() )
            {
                U16 Pid;

                Pid = (stptiMemGet_Slot( SlotHandle ))->Pid;

                if ( Pid == STPTI_WildCardPid())
                {
                    /* As soon as we hit a wildcard PID disable the PID filtering */
                    Frontend_p->WildCardPidCount++;
                    EnableFilter = FALSE;
                    break;
                }
                else if ( Pid != STPTI_InvalidPid() && Pid != STPTI_NegativePid() )
                {
                    STTBX_Print(( "stptiHAL_SyncFrontendPids - Setting PID 0x%03x on Stream 0x%04x\n", Pid, (U32)Device_p->StreamID));
                    StfeHal_SetPid( (U32)Frontend_p->PIDTableStart_p, Pid);
                }
            }
            ++SlotIndex;
        }

        /* Restore original pointer*/
        Frontend_p = stptiMemGet_Frontend( FullFrontendHandle );

        /* Check for a clone session now */
        if ( Frontend_p->CloneFrontendHandle.word != STPTI_NullHandle())
        {
            Frontend_t * CloneFrontend_p = stptiMemGet_Frontend( Frontend_p->CloneFrontendHandle );
            /* Make sure the Clone is linked to a device structure and then check the slot list */
            if ( CloneFrontend_p->SessionListHandle.word != STPTI_NullHandle() )
            {
                FullHandle_t CloneFullSessionHandle;
                CloneFullSessionHandle.word = ( &stptiMemGet_List( CloneFrontend_p->SessionListHandle )->Handle )[0];

                SlotListHandle = stptiMemGet_Session( CloneFullSessionHandle )->AllocatedList[OBJECT_TYPE_SLOT];

                MaxSlots = stptiMemGet_List( SlotListHandle )->MaxHandles;
                SlotIndex = 0;
                /*
                 * Iterate over all allocated slots until we find a matching pid,
                 * or run out of Slots.
                 */
                while ( SlotIndex < MaxSlots )
                {
                    SlotHandle.word = ( &stptiMemGet_List( SlotListHandle )->Handle )[SlotIndex];

                    if ( SlotHandle.word != STPTI_NullHandle() )
                    {
                        U16 Pid;

                        Pid = (stptiMemGet_Slot( SlotHandle ))->Pid;

                        if ( Pid == STPTI_WildCardPid())
                        {
                            /* As soon as we hit a wildcard PID disable the PID filtering */
                            CloneFrontend_p->WildCardPidCount++;
                            EnableFilter = FALSE;
                            break;
                        }
                        else if ( Pid != STPTI_InvalidPid() && Pid != STPTI_NegativePid() )
                        {
                            STTBX_Print(( "stptiHAL_SyncFrontendPids - Setting PID 0x%03x on Stream 0x%04x\n", Pid, (U32)Device_p->StreamID));
                            StfeHal_SetPid( (U32)Frontend_p->PIDTableStart_p, Pid);
                        }
                    }
                    ++SlotIndex;
                }

            }
        }


    }

    /* Check if its necessary to disable the PID filtering */
    if ( FALSE == EnableFilter )
    {
        Frontend_p->PIDFilterEnabled = FALSE;
        /* Set all pid filtering on  - effectively disabling the filter but eliminating the need to halt the frontend */
        memset( Frontend_p->PIDTableStart_p, 0xFF, STFE_PID_RAM_SIZE );
    }
}


/******************************************************************************
Function Name : stptiHAL_FrontendInjectData
  Description : Inject SWTS data

   Parameters : FullFrontendHandle

******************************************************************************/
ST_ErrorCode_t stptiHAL_FrontendInjectData( FullHandle_t FullFrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p, U32 NumberOfSWTSNodes, STPTI_StreamID_t StreamID )
{
    ST_ErrorCode_t      Error = ST_NO_ERROR;
    U32                 *SWTS_NodeMemory_p;
    TSGDMA_SWTSNode_t   *TSGSWTSNode_p;
    U32                 i;
    FullHandle_t        FullSessionListHandle;
    ST_Partition_t      *NonCachedPartition_p;

#if defined( STPTI_INTERNAL_DEBUG_SUPPORT )
    STTBX_Print(( "%s( FullFrontendHandle = 0x%08x, FrontendSWTSNode_p = 0x%08x, NumberOfSWTSNodes = %d, StreamID = 0x%04x )\n", __FUNCTION__, (U32)FullFrontendHandle.word, (U32)FrontendSWTSNode_p, NumberOfSWTSNodes, (U32)StreamID   ));
    #endif

    /* Lock access to this SWTS Channel */
    STOS_SemaphoreWait(TSGDMA_SWTSLock[StreamID&0x3F]);
    STTBX_Print(( "%s: TSGDMA_SWTSLock taken\n", __FUNCTION__ ));

    /* Lock the PTI */
    STOS_SemaphoreWait( stpti_MemoryLock );
    STTBX_Print(( "%s: stpti_MemoryLock taken\n", __FUNCTION__ ));

    /* check if PTI device, session running & object exist */
    Error = stpti_HandleCheck( FullFrontendHandle );

    if(Error == ST_NO_ERROR)
    {
        NonCachedPartition_p = stptiMemGet_Session( FullFrontendHandle )->NonCachedPartition_p;

        /* allocate some memory for the TSGDMA linked list */
        SWTS_NodeMemory_p = (U32 *) STOS_MemoryAllocate(NonCachedPartition_p, (sizeof(TSGDMA_SWTSNode_t) * NumberOfSWTSNodes +32) );

        if(SWTS_NodeMemory_p == NULL)
        {
            STTBX_Print(( "%s: Unable to allocate memory for SWTS Node\n", __FUNCTION__ ));
            Error = ST_ERROR_NO_MEMORY;
        }

        if(Error == ST_NO_ERROR)
        {
            U32 PacketLength;
            Frontend_t * Frontend_p = stptiMemGet_Frontend( FullFrontendHandle );

            /* align to 32 byte boundary */
            TSGSWTSNode_p = (TSGDMA_SWTSNode_t*) stpti_MemoryAlign( (void*) SWTS_NodeMemory_p, 32);

            FullSessionListHandle = Frontend_p->SessionListHandle;
            if ( FullSessionListHandle.word == STPTI_NullHandle() )
            {
                Error = STPTI_ERROR_FRONTEND_NOT_LINKED;
            }
            else
            {
                U32               ChannelDestination;
                Device_t *        Device_p    = stptiMemGet_Device(FullFrontendHandle);
                STPTI_DevicePtr_t BaseAddress_p = NULL;

                /* determine what packets we are injecting */

                {
                    PacketLength = DVB_TS_PACKET_LENGTH;
                }

#if defined(ST_OSLINUX)
                BaseAddress_p =(STPTI_DevicePtr_t) Device_p->DriverPartition_p;
#else
                BaseAddress_p =(STPTI_DevicePtr_t) Device_p->TCDeviceAddress_p;
#endif
                /* determine which PTI to inject the data to */
                if (BaseAddress_p != NULL)
                {
                    switch ( (U32) BaseAddress_p )
                    {
#if defined(ST_OSLINUX)
                        case 1:
#endif
                        case PTIB_BASE_ADDRESS:
                        {
                            ChannelDestination = TSGDMA_DEST_PTI1;
                            break;
                        }
#if defined(ST_OSLINUX)
                        case 0:
#endif
                        case PTIA_BASE_ADDRESS:
                        default :
                        {
                            ChannelDestination = TSGDMA_DEST_PTI0;
                            break;
                        }
                    }

                    /* populate the link list now it is algined */
                    for (i=0; i<NumberOfSWTSNodes; i++)
                    {
#if defined( STPTI_INTERNAL_DEBUG_SUPPORT )
                        {
                            STTBX_Print(( "%s: FrontendSWTSNode_p[i].Data_p = %08x\n", __FUNCTION__, (U32)FrontendSWTSNode_p[i].Data_p ));
                            STTBX_Print(( "%s: FrontendSWTSNode_p[i].Size   = %d\n", __FUNCTION__, FrontendSWTSNode_p[i].SizeOfData ));
                            STTBX_Print(( "%s: STOS_VirtToPhys( FrontendSWTSNode_p[i].Data_p ) = %08x\n", __FUNCTION__, (U32) STOS_VirtToPhys(FrontendSWTSNode_p[i].Data_p ) ));
                        }
                        #endif

                        TSGSWTSNode_p[i].CtrlWord     = 0;
                        TSGSWTSNode_p[i].TSData_p     = (U32*) STOS_VirtToPhys(FrontendSWTSNode_p[i].Data_p );
                        TSGSWTSNode_p[i].ActualSize   = FrontendSWTSNode_p[i].SizeOfData;
                        TSGSWTSNode_p[i].PacketLength = PacketLength;
                        TSGSWTSNode_p[i].Channel_Dest = ChannelDestination;
                        TSGSWTSNode_p[i].TagHeader    = StreamID;
                        TSGSWTSNode_p[i].Stuffing     = 0;

                        /* Set the pointer to the next node */
                        if (i < NumberOfSWTSNodes-1)
                        {
                            TSGSWTSNode_p[i].NextNode_p   = (TSGDMA_SWTSNode_t *) STOS_VirtToPhys((&TSGSWTSNode_p[i+1]));
                        }
                        else
                        {
                            TSGSWTSNode_p[i].NextNode_p   = NULL;
                        }

                        STOS_FlushRegion((void*) &TSGSWTSNode_p[i], sizeof(STPTI_FrontendSWTSNode_t));              /* flush the cache for the node */
                        STOS_FlushRegion((void*) FrontendSWTSNode_p[i].Data_p, FrontendSWTSNode_p[i].SizeOfData);   /* Flush ram for injected data */

                    }

                    /* Release lock */
                    STOS_SemaphoreSignal( stpti_MemoryLock );

                    /* call blocking TsgdmaHAL_StartSWTSChannel */
                    TsgdmaHAL_StartSWTSChannel(StreamID, TSGSWTSNode_p);

                }
            }

            /* free memory after injection has compelted */
            STOS_MemoryDeallocate(NonCachedPartition_p, SWTS_NodeMemory_p);
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    STTBX_Print(( "%s: stpti_MemoryLock released\n", __FUNCTION__ ));

    STOS_SemaphoreSignal(TSGDMA_SWTSLock[StreamID&0x3F]);
    STTBX_Print(( "%s: TSGDMA_SWTSLock released\n", __FUNCTION__ ));

    return Error;
}

#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */

/* EOF  -------------------------------------------------------------------- */
