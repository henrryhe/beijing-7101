/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005, 2006, 2007.  All rights reserved.

   File Name: pti_basic.c
 Description: core demux functions

Notes:
       STPTI_BufferReadNBytes was moved fom here to pti_extended.c following DDTS18257
       STPTI_SlotLinkToCdFifo was moved from here to pti_extended.c following DDTS18257

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#define VIRTUAL_PTI_RAMCAM_SUPPORT

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

#include "stcommon.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stdevice.h"
#include "stpti.h"
#include "osx.h"
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
#include "pti_linux.h"
#include "stpti4_core_proc.h"
#include <linux/bigphysarea.h>

#define KMALLOC_LIMIT 131072
#endif /* #endif ST_OSLINUX */

#if defined (STPTI_FRONTEND_HYBRID)
#include "frontend.h"
#endif

/* Private Constants ------------------------------------------------------- */

volatile U32 stpti_InterruptCount[64] = { 0 };
volatile U32 stpti_InterruptTotalCount = 0;

/* Private Variables ------------------------------------------------------- */

#if defined( STPTI_GPDMA_SUPPORT )
    STGPDMA_DmaTransfer_t GPDMATransferParams;
#endif
#if defined( STPTI_FDMA_SUPPORT )
    STFDMA_Node_t *FDMANode_p, *OriginalFDMANode_p[3];
    STFDMA_TransferGenericParams_t FDMAGenericTransferParams;
    STFDMA_TransferId_t     FDMATransferId;

#if !( defined( STPTI_FDMA_BLOCK ) || defined( STPTI_FDMA_VERSION1_SUPPORT ) )
/* Choose a default FDMA_BLOCK (first block) unless we have suppressed support for version 2.0.0 of fdma driver */
#define STPTI_FDMA_BLOCK STFDMA_1
#endif
       
#endif

/* ------------------------------------------------------------------------- */

Driver_t STPTI_Driver = {
                            {NULL, 0, NULL, NULL, NULL, 0},                 /* MemCtl initialisations */
#if defined(ST_OSLINUX)
                            NULL, NULL, EVENT_Q_STACK, 0,
                            NULL, NULL, INT_Q_STACK, 0,
#else
                            NULL, NULL, EVENT_Q_STACK, MIN_USER_PRIORITY,   /* Event Q initialisations */
                            NULL, NULL, INT_Q_STACK,MIN_USER_PRIORITY,
#endif /* #endif..else #if defined(ST_OSLINUX) */

     /* Interrupt Q initialisations */

#ifndef STPTI_NO_INDEX_SUPPORT

#if !defined(ST_OSLINUX)
                            NULL, NULL, INDEX_Q_STACK, MIN_USER_PRIORITY,   /* Index Q initialisations */
#endif /* #endif !defined(ST_OSLINUX) */

#endif /* #endifndef STPTI_NO_INDEX_SUPPORT */

                        };

/* Private Function Prototypes --------------------------------------------- */

static ST_ErrorCode_t stpti_Slot_ClearPid( FullHandle_t FullSlotHandle, BOOL WindbackDMA );
#if !defined ( STPTI_BSL_SUPPORT )
static ST_ErrorCode_t stpti_Slot_ClearPCRSlot( FullHandle_t FullSlotHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

#ifdef STTBX_PRINT
/* If STTBX_Print is defined, dumps state of slots */
static void DumpState( FullHandle_t NewSlotHandle, const char *label, int indentChange );
#endif

/* Functions --------------------------------------------------------------- */

ST_ErrorCode_t stpti_Slot_SetPid( FullHandle_t FullSlotHandle,
                                         STPTI_Pid_t Pid,
                                         STPTI_Pid_t MapPid );

#ifndef STPTI_NO_INDEX_SUPPORT
extern  ST_ErrorCode_t stpti_IndexQueryPid( FullHandle_t DeviceHandle,
                                            STPTI_Pid_t Pid,
                                            STPTI_Index_t *Index_p );
#endif


extern ST_ErrorCode_t stpti_DescramblerQueryPid( FullHandle_t DeviceHandle,
                                                 STPTI_Pid_t Pid,
                                                 STPTI_Descrambler_t * Descrambler_p );

extern ST_ErrorCode_t stpti_Descrambler_AssociateSlot( FullHandle_t DescramblerHandle,
                                                       FullHandle_t SlotHandle );

#ifndef STPTI_NullHandle
STPTI_Handle_t STPTI_NullHandle( void )
{
    FullHandle_t FullHandle;

    FullHandle.member.Object     =  0;
    FullHandle.member.ObjectType = ~0;
    FullHandle.member.Session    =  0;
    FullHandle.member.Device     =  0;

    return FullHandle.word;
}
#endif


/******************************************************************************
Function Name : STPTI_BufferAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferAllocate( STPTI_Handle_t Handle,
                                     U32 RequiredSize,
                                     U32 NumberOfPacketsInMultiPacket,
                                     STPTI_Buffer_t * BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullBufferHandle;
    size_t ActualSize;
    Buffer_t TmpBuffer;
    U8 *Buffer_p = NULL;
#if defined(ST_OSLINUX)
    void *MappedBuffer_p = NULL;
#endif /* #endif defined(ST_OSLINUX) */
    
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSessionHandle.word = Handle;
    
#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_BufferAllocate( FullSessionHandle,
                                          RequiredSize,
                                          NumberOfPacketsInMultiPacket,
                                          BufferHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if ( Error == ST_NO_ERROR )
    {
        /* Used for both TC1 & TC3 pw1*/
        ActualSize = stpti_BufferSizeAdjust( RequiredSize );

        /*
         *  Create the buffer in the partition allocated for it when the
         *  session was opened
         */
#if defined(ST_OSLINUX)
        {
            U32 Size = (ActualSize + STPTI_BUFFER_ALIGN_MULTIPLE - 1) ;
#if defined( USING_BIG_PHYSAREA )
            U32 pages = Size / PAGE_SIZE;

            if (Size % PAGE_SIZE)
                pages++;

            /* This is a virtual address remember to convert it to a bus address before use. */
            Buffer_p = bigphysarea_alloc_pages(pages, 0, GFP_KERNEL | __GFP_DMA);
#else
            if( Size > KMALLOC_LIMIT )
            {
                printk(KERN_WARNING"pticore: attempted to kmalloc an area in excess of %d bytes - required: %d   aligned size: %d\n",KMALLOC_LIMIT,Size,RequiredSize);
            }

            Buffer_p = kmalloc( Size, GFP_KERNEL | __GFP_DMA );
#endif /* #if defined .. #else ( USING_BIG_PHYSAREA ) */
            if( Buffer_p == NULL )
            {
                printk(KERN_ALERT"pticore: Failed to allocate memory for buffer. Size %d %d\n",Size,RequiredSize);
                Error = ST_ERROR_NO_MEMORY;
            }
            else
            {
                /* This is a bus address remember to convert it to a convert before freeing. */

                Buffer_p = (U8 *)virt_to_bus( Buffer_p );

                MappedBuffer_p = ioremap_nocache( (U32)Buffer_p, Size );
                /* CPU accesses will use this address as the base */
            }
        }
#else
        Buffer_p = memory_allocate( stptiMemGet_Session( FullSessionHandle )->NonCachedPartition_p,
                                   ActualSize + STPTI_BUFFER_ALIGN_MULTIPLE - 1 );
#endif /*  #if..#else defined(ST_OSLINUX) */

        if ( Buffer_p != NULL )
        {
            Error = stpti_CreateObjectHandle( FullSessionHandle,
                                              OBJECT_TYPE_BUFFER,
                                              sizeof( Buffer_t ),
                                              &FullBufferHandle );
            if ( Error == ST_NO_ERROR )
            {
                /* load default buffer configuration */
                TmpBuffer.OwnerSession            = Handle;
                TmpBuffer.Handle                  = FullBufferHandle.word;
#if defined( STPTI_GPDMA_SUPPORT )
                TmpBuffer.STGPDMAHandle           = NULL;
#endif

#if defined(ST_OSLINUX)
                TmpBuffer.MappedStart_p           = MappedBuffer_p; /* ioremapped buffer address. Used for CPU reads. */
                TmpBuffer.UserSpaceAddressDiff    = (U32)-1;
#endif
                TmpBuffer.Start_p                 = (U8 *) stpti_BufferBaseAdjust( Buffer_p );
                TmpBuffer.RealStart_p             = Buffer_p;
                TmpBuffer.ActualSize              = ActualSize;
                TmpBuffer.RequestedSize           = RequiredSize;
                TmpBuffer.Type                    = STPTI_SLOT_TYPE_NULL;
                /* 0 may be safer if no param checking */
                TmpBuffer.TC_DMAIdent             = UNININITIALISED_VALUE;
                /*
                 * AS 26/03/03 Initializing DirectDma default Value as 0,
                 * because only DMA1,2,3 are linked, GNBvd19253
                 */
                TmpBuffer.DirectDma               = 0;
                TmpBuffer.CDFifoAddr              = ( U32 ) NULL;
                TmpBuffer.Holdoff                 = 0;
                TmpBuffer.Burst                   = 0;
                TmpBuffer.Queue_p                 = NULL;
                TmpBuffer.BufferPacketCount       = 0;
                TmpBuffer.MultiPacketSize         = (NumberOfPacketsInMultiPacket & 0xffff);
                TmpBuffer.SlotListHandle.word     = STPTI_NullHandle();
                TmpBuffer.SignalListHandle.word   = STPTI_NullHandle();
#ifdef STPTI_CAROUSEL_SUPPORT
                TmpBuffer.CarouselListHandle.word = STPTI_NullHandle();
#endif
                TmpBuffer.ManualAllocation  = FALSE;
                TmpBuffer.IgnoreOverflow    = FALSE;
                TmpBuffer.IsRecordBuffer    = FALSE;
                TmpBuffer.EventLevelChange  = FALSE;

                memcpy( ( U8 * ) stptiMemGet_Buffer( FullBufferHandle ),
                        ( U8 * ) &TmpBuffer,
                        sizeof( Buffer_t ));

#if !defined ( STPTI_BSL_SUPPORT )
                Error = stpti_BufferHandleCheck( FullBufferHandle );
#endif

                *BufferHandle = FullBufferHandle.word;

#if defined( ST_OSLINUX )
                /* Add to the proc file system */
                AddPTIBufferToProcFS(FullBufferHandle);
#endif
            }
        }
        else
        {
            Error = ST_ERROR_NO_MEMORY;
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name : STPTI_FilterAllocate
  Description : Allocate a TC Filter structure, ( may be SECTION, TSHeader, or PES )
                    and record the allocation for future use
                    ( SF CAM entries are implicitly reserved ).
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FilterAllocate( STPTI_Handle_t Handle,
                                     STPTI_FilterType_t FilterType,
                                     STPTI_Filter_t * FilterObject )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullFilterHandle;
    U32 TC_FilterIdent;
    Filter_t TmpFilter;

    
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSessionHandle.word = Handle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FilterAllocate( FullSessionHandle, FilterType, FilterObject );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error == ST_NO_ERROR )
    {
        switch ( FilterType )
        {
#if !defined ( STPTI_BSL_SUPPORT )
        case STPTI_FILTER_TYPE_TSHEADER_FILTER:
            Error = stptiHAL_FilterAllocateTransport( FullSessionHandle,
                                                      &TC_FilterIdent );
            break;
        case STPTI_FILTER_TYPE_PES_FILTER:
            Error = stptiHAL_FilterAllocatePES( FullSessionHandle,
                                                &TC_FilterIdent );
            break;
        case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
            Error = stptiHAL_FilterAllocatePESStreamId( FullSessionHandle,
                                                        &TC_FilterIdent );
            break;
        case STPTI_FILTER_TYPE_EMM_FILTER:
            Error = stptiHAL_FilterAllocateSection( FullSessionHandle,
                                                    STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
                                                    &TC_FilterIdent );                /* EMM filters are section filters */
            break;
        case STPTI_FILTER_TYPE_ECM_FILTER:
            Error = stptiHAL_FilterAllocateSection( FullSessionHandle,
                                                    STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE,
                                                    &TC_FilterIdent );                /* ECM filters are section filters */
            break;
#ifdef STPTI_DC2_SUPPORT
        case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
        case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
        case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
            Error = stptiHAL_DC2FilterAllocateMulticast16( FullSessionHandle,
                                                           &TC_FilterIdent );
            break;
#endif
#endif /* STPTI_BSL_SUPPORT */
        default:
            Error = stptiHAL_FilterAllocateSection( FullSessionHandle,
                                                    FilterType,
                                                    &TC_FilterIdent );
            break;
        }

        if ( Error == ST_NO_ERROR )
        {
            Error = stpti_CreateObjectHandle( FullSessionHandle,
                                              OBJECT_TYPE_FILTER,
                                              sizeof( Filter_t ),
                                              &FullFilterHandle );
            if ( Error == ST_NO_ERROR )
            {
                TmpFilter.OwnerSession               = Handle;                  /* Load Parameters */
                TmpFilter.Handle                     = FullFilterHandle.word;
                TmpFilter.Type                       = FilterType;
                TmpFilter.TC_FilterIdent             = TC_FilterIdent;
                TmpFilter.Enabled                    = FALSE;
                TmpFilter.DiscardOnCrcError          = TRUE;
                TmpFilter.SlotListHandle.word        = STPTI_NullHandle();
                TmpFilter.MatchFilterListHandle.word = STPTI_NullHandle();

                memcpy((U8 *) stptiMemGet_Filter( FullFilterHandle ),
                        (U8 *) &TmpFilter,
                       sizeof( Filter_t ));

#if !defined ( STPTI_BSL_SUPPORT )
                Error = stpti_FilterHandleCheck( FullFilterHandle );
#endif
                *FilterObject = FullFilterHandle.word;

                switch ( FilterType )
                {
#if !defined ( STPTI_BSL_SUPPORT )
                case STPTI_FILTER_TYPE_TSHEADER_FILTER:
                    Error = stptiHAL_FilterInitialiseTransport( FullSessionHandle,
                                                                TC_FilterIdent,
                                                                FullFilterHandle.word );
                    break;
                case STPTI_FILTER_TYPE_PES_FILTER:
                    Error = stptiHAL_FilterInitialisePES( FullSessionHandle,
                                                          TC_FilterIdent,
                                                          FullFilterHandle.word );
                    break;
                case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
                    Error = stptiHAL_FilterInitialisePESStreamId( FullSessionHandle,
                                                                  TC_FilterIdent,
                                                                  FullFilterHandle.word );
                    break;
                case STPTI_FILTER_TYPE_EMM_FILTER:
                    Error = stptiHAL_FilterInitialiseSection( FullSessionHandle,
                                                              TC_FilterIdent,
                                                              FullFilterHandle.word,
                                                              STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE );  /* EMM filters are section filters */
                    break;
                case STPTI_FILTER_TYPE_ECM_FILTER:
                    Error = stptiHAL_FilterInitialiseSection( FullSessionHandle,
                                                              TC_FilterIdent,
                                                              FullFilterHandle.word,
                                                              STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE );  /* ECM filters are section filters */
                    break;
#ifdef STPTI_DC2_SUPPORT
                case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
                case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
                case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
                    Error = stptiHAL_DC2FilterInitialiseMulticast16( FullSessionHandle,
                                                                     TC_FilterIdent,
                                                                     FullFilterHandle.word );
                    break;
#endif
#endif  /* STPTI_BSL_SUPPORT */
                default:
                    Error = stptiHAL_FilterInitialiseSection( FullSessionHandle,
                                                              TC_FilterIdent,
                                                              FullFilterHandle.word,
                                                              FilterType );
                    break;
                }
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}


/******************************************************************************
Function Name : STPTI_SlotAllocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotAllocate( STPTI_Handle_t Handle,
                                   STPTI_Slot_t * SlotHandle,
                                   STPTI_SlotData_t * SlotData )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullSlotHandle;
    U32 TC_SlotIdent;
    Slot_t TmpSlot;

    
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSessionHandle.word = Handle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_SlotAllocate( FullSessionHandle, SlotHandle, SlotData );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_SlotAllocate( FullSessionHandle, &TC_SlotIdent );
        if ( Error == ST_NO_ERROR )
        {
            Error = stpti_CreateObjectHandle( FullSessionHandle,
                                              OBJECT_TYPE_SLOT,
                                              sizeof( Slot_t ),
                                              &FullSlotHandle );
            if ( Error == ST_NO_ERROR )
            {
                /* Load Parameters */
                TmpSlot.OwnerSession               = Handle;
                TmpSlot.Handle                     = FullSlotHandle.word;
                TmpSlot.TC_SlotIdent               = TC_SlotIdent;
                TmpSlot.PCRReturnHandle            = STPTI_NullHandle();
                TmpSlot.Type                       = SlotData->SlotType;
                TmpSlot.Flags                      = SlotData->SlotFlags;
                TmpSlot.Pid                        = TC_INVALID_PID;
#ifdef STPTI_CAROUSEL_SUPPORT
                if ( SlotData->SlotFlags.AlternateOutputInjectCarouselPacket )
                    TmpSlot.UseDefaultAltOut       = FALSE;
                else
#endif
                TmpSlot.UseDefaultAltOut           = TRUE;
                TmpSlot.AlternateOutputType        = stptiMemGet_Device( FullSessionHandle )->AlternateOutputType;
                TmpSlot.AlternateOutputTag         = stptiMemGet_Device( FullSessionHandle )->AlternateOutputTag;
                TmpSlot.CDFifoAddr                 = ( U32 ) NULL;
                TmpSlot.TC_DMAIdent                = 0;
                TmpSlot.AssociatedFilterMode       = 0;
                TmpSlot.DescramblerListHandle.word = STPTI_NullHandle();
                TmpSlot.FilterListHandle.word      = STPTI_NullHandle();
                TmpSlot.BufferListHandle.word      = STPTI_NullHandle();
#ifndef STPTI_NO_INDEX_SUPPORT
                TmpSlot.IndexListHandle.word = STPTI_NullHandle();
#endif

                if(SlotData->SlotType==STPTI_SLOT_TYPE_ECM)
                {
                    TmpSlot.Type = STPTI_SLOT_TYPE_SECTION;
                }


                if(SlotData->SlotType==STPTI_SLOT_TYPE_EMM)
                {
                    TmpSlot.Type = STPTI_SLOT_TYPE_SECTION;
                }

                
                memcpy( (U8 *) stptiMemGet_Slot( FullSlotHandle ),
                        (U8 *) &TmpSlot,
                        sizeof( Slot_t ));

#if !defined ( STPTI_BSL_SUPPORT )
                Error = stpti_SlotHandleCheck( FullSlotHandle );
#endif

                if ( Error == ST_NO_ERROR )
                {
                    *SlotHandle = FullSlotHandle.word;

                    Error = stptiHAL_SlotInitialise( FullSessionHandle,
                                                     SlotData,
                                                     TC_SlotIdent,
                                                     FullSlotHandle.word );
                }
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferFlush
  Description : Flush the contents of a circular buffer.
   Parameters : a handle to the bufferto be flushed
******************************************************************************/
ST_ErrorCode_t STPTI_BufferFlush( STPTI_Buffer_t BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_BufferFlush( FullBufferHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_BufferFlush( FullBufferHandle );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}



/******************************************************************************
Function Name : STPTI_BufferLevelSignalEnable
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferLevelSignalEnable( STPTI_Buffer_t BufferHandle, U32 BufferLevelThreshold )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;
    Error = stptiValidate_BufferLevelSignalEnable( FullBufferHandle, BufferLevelThreshold );
    if(Error != ST_NO_ERROR)
        return(Error);
    stptiHelper_BufferLevelSignalEnable(FullBufferHandle, BufferLevelThreshold);
    STOS_SemaphoreSignal( stpti_MemoryLock );
#else
    Error = STPTI_ERROR_FUNCTION_NOT_SUPPORTED;
#endif
        
    return(Error);
}


/******************************************************************************
Function Name : STPTI_BufferLevelSignalDisable
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferLevelSignalDisable( STPTI_Buffer_t BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#if defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;
    Error = stptiValidate_BufferLevelSignalDisable( FullBufferHandle );
    if(Error != ST_NO_ERROR)
        return(Error);
    stptiHelper_BufferLevelSignalDisable(FullBufferHandle);
    STOS_SemaphoreSignal( stpti_MemoryLock );
#else
    Error = STPTI_ERROR_FUNCTION_NOT_SUPPORTED;
#endif
        
    return(Error);
}




/******************************************************************************
Function Name : STPTI_BufferReadSection
  Description :
   Parameters :
******************************************************************************/
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
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    
    
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    if ( NULL != DataSize_p )
        *DataSize_p = 0;
    
#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_BufferReadSection( FullBufferHandle,
                                             MatchedFilterList,
                                             MaxLengthofFilterList,
                                             NumOfFilterMatches_p,
                                             CRCValid_p,
                                             Destination0_p,
                                             DestinationSize0,
                                             Destination1_p,
                                             DestinationSize1,
                                             DataSize_p,
                                             DmaOrMemcpy );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if ( Error == ST_NO_ERROR )
    {

#ifndef STPTI_LOADER_SUPPORT
        if ( ( stptiMemGet_Device( FullBufferHandle )->TCCodes == STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB )
           )
        {
            Error = stptiHAL_DVBBufferReadSection( FullBufferHandle,
                                                   MatchedFilterList,
                                                   MaxLengthofFilterList,
                                                   NumOfFilterMatches_p,
                                                   CRCValid_p,
                                                   Destination0_p,
                                                   DestinationSize0,
                                                   Destination1_p,
                                                   DestinationSize1,
                                                   DataSize_p,
                                                   DmaOrMemcpy );
        }
#endif

    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : STPTI_Close
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_Close( STPTI_Handle_t Handle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;

    /* Escape if there is no MemoryLock Semaphore - means we have not initialised any PTI sessions */
    if( stpti_MemoryLock == NULL ) return( STPTI_ERROR_NOT_INITIALISED );

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSessionHandle.word = Handle;

    Error = stptiValidate_Close( FullSessionHandle );

    /* DeallocateSession and 'force' deallocation of child objects */
    if ( Error == ST_NO_ERROR )
    {

#ifdef ST_OSLINUX
        Session_t *Session_p = stptiMemGet_Session( FullSessionHandle );

        /* Ensure no registered events. */
        stpti_RemoveSessionFromEvtArray(FullSessionHandle);
        stpti_LinuxDeallocateEvtStruct(Session_p->evt_p);
#endif
        Error = stpti_DeallocateSession( FullSessionHandle, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferDeallocate( STPTI_Buffer_t BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferDeallocate( FullBufferHandle );
    if ( Error == ST_NO_ERROR )
    {
        Error = stpti_DeallocateBuffer( FullBufferHandle, TRUE );


    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name :STPTI_FilterDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FilterDeallocate( STPTI_Filter_t FilterHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFilterHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullFilterHandle.word = FilterHandle;

    Error = stptiValidate_FilterDeallocate( FullFilterHandle );
    if ( Error == ST_NO_ERROR )
    {
        Error = stpti_DeallocateFilter( FullFilterHandle, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name :STPTI_SlotDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotDeallocate( STPTI_Slot_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotDeallocate( FullSlotHandle );
    if ( Error == ST_NO_ERROR )
    {
        Error = stpti_DeallocateSlot( FullSlotHandle, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : STPTI_FilterAssociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FilterAssociate( STPTI_Filter_t FilterHandle,
                                      STPTI_Slot_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFilterHandle, FilterListHandle;
    FullHandle_t FullSlotHandle, SlotListHandle;
    FullHandle_t ListFilterHandle;
    U16 Indx;
    U16 NumberOfFilters;

    
    /* LOCK */
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullFilterHandle.word = FilterHandle;
    FullSlotHandle.word   = SlotHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FilterAssociate( FullFilterHandle, FullSlotHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error != ST_NO_ERROR )
    {
        STOS_SemaphoreSignal( stpti_MemoryLock );     /* UNLOCK */
        return( Error );
    }

    /* --- BEGIN testing for GNBvd19871 --- */
    FilterListHandle = ( stptiMemGet_Slot( FullSlotHandle ))->FilterListHandle;

    if ( FilterListHandle.word != STPTI_NullHandle())
    {
        NumberOfFilters = stptiMemGet_List( FilterListHandle )->MaxHandles;

        /*
         * Check that the filters for the FilterListHandle associated with the
         * slot have all the same type as the new filter to associate with the
         * slot.
         */
        for ( Indx = 0; Indx < NumberOfFilters; Indx++ )
        {
            ListFilterHandle.word = ( &stptiMemGet_List( FilterListHandle )->Handle )[Indx];

            if ( ListFilterHandle.word != STPTI_NullHandle())
            {
                if (( stptiMemGet_Filter( FullFilterHandle ))->Type !=
                            ( stptiMemGet_Filter( ListFilterHandle ))->Type )
                {
                    STTBX_Print(( "!!Association of different Filter types on the same slot!!\n" ));
                    /* UNLOCK */
                    STOS_SemaphoreSignal( stpti_MemoryLock );
                    return( STPTI_ERROR_INVALID_FILTER_TYPE );

                   /*
                    * If many filters have already been associated they must
                    * all have the same type.  We only go through the list
                    * because some must have been disasociated and then be
                    * empty
                    */
                }
            }
        }
    }
    /* --- END testing for GNBvd19871 --- */

    switch (( stptiMemGet_Filter( FullFilterHandle ))->Type )
    {

#if !defined ( STPTI_BSL_SUPPORT )
    case STPTI_FILTER_TYPE_TSHEADER_FILTER:
        Error = stptiHAL_FilterAssociateTransport( FullSlotHandle, FullFilterHandle );
        break;

    case STPTI_FILTER_TYPE_PES_FILTER:
        Error = stptiHAL_FilterAssociatePES( FullSlotHandle, FullFilterHandle );
        break;
    case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
        Error = stptiHAL_FilterAssociatePESStreamId( FullSlotHandle, FullFilterHandle );
        break;

    case STPTI_FILTER_TYPE_ECM_FILTER:
        Error = stptiHAL_FilterAssociateSection( FullSlotHandle, FullFilterHandle );
        break;

    case STPTI_FILTER_TYPE_EMM_FILTER:
        Error = stptiHAL_FilterAssociateSection( FullSlotHandle, FullFilterHandle );
        break;
        
#ifdef STPTI_DC2_SUPPORT
    case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
    case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
    case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
        Error = stptiHAL_DC2FilterAssociateMulticast16( FullSlotHandle, FullFilterHandle );
        break;
#endif
#endif  /* #if !defined ( STPTI_BSL_SUPPORT ) */

    default:
        Error = stptiHAL_FilterAssociateSection( FullSlotHandle, FullFilterHandle );
        break;
    }

    if ( Error == ST_NO_ERROR )
    {
       FilterListHandle = ( stptiMemGet_Slot( FullSlotHandle ))->FilterListHandle;

        if ( FilterListHandle.word == STPTI_NullHandle())
        {
            Error = stpti_CreateListObject( FullSlotHandle, &FilterListHandle );
            if ( Error == ST_NO_ERROR )
            {
                ( stptiMemGet_Slot( FullSlotHandle ))->FilterListHandle = FilterListHandle;
            }
        }

        if ( Error == ST_NO_ERROR )
        {
            SlotListHandle = ( stptiMemGet_Filter( FullFilterHandle ))->SlotListHandle;

            if ( SlotListHandle.word == STPTI_NullHandle())
            {
                Error = stpti_CreateListObject( FullFilterHandle, &SlotListHandle );
                if ( Error == ST_NO_ERROR )
                {
                    ( stptiMemGet_Filter( FullFilterHandle ))->SlotListHandle = SlotListHandle;
                }
            }

            if ( Error == ST_NO_ERROR )
            {
                Error = stpti_AssociateObjects( FilterListHandle,
                                                FullSlotHandle,
                                                SlotListHandle,
                                                FullFilterHandle );
            }
        }
    }

    /* UNLOCK */
    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}


/******************************************************************************
Function Name :STPTI_FilterDisassociate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FilterDisassociate( STPTI_Filter_t FilterHandle,
                                         STPTI_Slot_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFilterHandle;
    FullHandle_t FullSlotHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullFilterHandle.word = FilterHandle;
    FullSlotHandle.word   = SlotHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FilterDeallocate( FullFilterHandle );
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiValidate_SlotDeallocate( FullSlotHandle );
        if ( Error == ST_NO_ERROR )
        {
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
            Error = stpti_SlotDisassociateFilter( FullSlotHandle,
                                                  FullFilterHandle,
                                                  FALSE );
#if !defined ( STPTI_BSL_SUPPORT )
        }
    }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}

/******************************************************************************
Function Name :STPTI_FilterSet
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_FilterSet( STPTI_Filter_t FilterHandle,
                                STPTI_FilterData_t *FilterData_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullFilterHandle;

    
    STOS_SemaphoreWait( stpti_MemoryLock );

    FullFilterHandle.word = FilterHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_FilterSetParams( FullFilterHandle,  FilterData_p );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error != ST_NO_ERROR )
    {
        STOS_SemaphoreSignal( stpti_MemoryLock );
        return( Error );
    }

    ( stptiMemGet_Filter( FullFilterHandle ))->Enabled =
                                            FilterData_p->InitialStateEnabled;

    switch ( FilterData_p->FilterType )
    {
    case STPTI_FILTER_TYPE_SECTION_FILTER:
    case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
    case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:
    case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
        ( stptiMemGet_Filter( FullFilterHandle ))->DiscardOnCrcError =
                            FilterData_p->u.SectionFilter.DiscardOnCrcError;
        Error = stptiHAL_FilterSetSection( FullFilterHandle, FilterData_p );
        break;
#if !defined ( STPTI_BSL_SUPPORT )
    case STPTI_FILTER_TYPE_PES_FILTER:
        Error = stptiHAL_FilterSetPES( FullFilterHandle, FilterData_p );
        break;
    case STPTI_FILTER_TYPE_TSHEADER_FILTER:
        Error = stptiHAL_FilterSetTransport( FullFilterHandle, FilterData_p );
        break;
    case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
        Error = stptiHAL_FilterSetPESStreamId( FullFilterHandle, FilterData_p );
        break;
    case STPTI_FILTER_TYPE_ECM_FILTER:
        /* ECM Filters are section filters */
        ( stptiMemGet_Filter( FullFilterHandle ))->DiscardOnCrcError =
                            FilterData_p->u.SectionFilter.DiscardOnCrcError;
        Error = stptiHAL_FilterSetSection( FullFilterHandle, FilterData_p );
        break;
    case STPTI_FILTER_TYPE_EMM_FILTER:
        /* EMM Filters are section filters */
        ( stptiMemGet_Filter( FullFilterHandle ))->DiscardOnCrcError =
                            FilterData_p->u.SectionFilter.DiscardOnCrcError;
        Error = stptiHAL_FilterSetSection( FullFilterHandle, FilterData_p );
        break;
#ifdef STPTI_DC2_SUPPORT
    case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
        Error = stptiHAL_DC2FilterSetMulticast16( FullFilterHandle, FilterData_p );
        break;
    case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
        Error = stptiHAL_DC2FilterSetMulticast32( FullFilterHandle, FilterData_p );
        break;
    case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
        Error = stptiHAL_DC2FilterSetMulticast48( FullFilterHandle, FilterData_p );
        break;
#endif
#endif  /* #if !defined ( STPTI_BSL_SUPPORT ) */
    default:
        Error = ST_ERROR_BAD_PARAMETER;
        break;
    }
    STOS_SemaphoreSignal( stpti_MemoryLock );

    return Error;
}


/******************************************************************************
Function Name: STPTI_Init
  Description:
   Parameters: See STPTI_InitParams_t
        Notes: Adding multi-session support on the same cell as of 20-Aug-2K3 GJP
******************************************************************************/
ST_ErrorCode_t STPTI_Init( ST_DeviceName_t DeviceName,
                          const STPTI_InitParams_t *InitParams )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Device_t       TmpDevice;
    FullHandle_t   OldDeviceHandle;
    U32            device;

    U16            InitialSessions = 1;
    BOOL           FirstDevice = FALSE;
    static BOOL    FirstTime = TRUE;
    FullHandle_t   ExistingSessionDeviceHandle;
    
    
#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_Init( DeviceName, InitParams );
    if ( Error != ST_NO_ERROR )
        return( Error );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    STOS_TaskLock();
    
    /* if this is the first time then initialise the memory protection semaphore */
    if ( FirstTime )
    {
        stpti_MemoryLock = STOS_SemaphoreCreatePriority( NULL, SEM_MUTUAL_EXCLUSION );
        FirstTime = FALSE;
        STTBX_Print(( "STPTI: Very first initalization\n" ));
    }

    STOS_TaskUnlock();
        
    STOS_SemaphoreWait( stpti_MemoryLock );

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock)== 0); /* Must be entered with Memory Locked */


    /*
     * if this is the very first stpti initalization then setup the basic
     * infrastructure
     */
    /* for STPTI_Driver struct see pti_loc.h */
    /* ST_OSLINUX: Even though partitions not used Partition_p is used as a flag to indicate that
        the device has been initialized.. */
    if ( STPTI_Driver.MemCtl.Partition_p == NULL )
    {
        STTBX_Print(( "STPTI: First Device\n" ));
        FirstDevice = TRUE;

#if !defined ( STPTI_BSL_SUPPORT )
        /* Initialise memory for One Device & Handle to it */
        Error = stpti_MemInit( InitParams->Partition_p,
                               sizeof( Device_t ) + sizeof( CellHeader_t ),
                               sizeof( Handle_t ),
                               &STPTI_Driver.MemCtl );
#else
        /* Initialise memory for 2 Device & Handle to it as reallocate won't work */
        Error = stpti_MemInit( InitParams->Partition_p,
                               (sizeof( Device_t ) + sizeof( CellHeader_t ))*2,
                               sizeof( Handle_t )*2,
                               &STPTI_Driver.MemCtl );
#endif                               

        /* Initialise InterruptQueue and associated process */
#ifdef STPTI_INTERRUPT_TASK_PRIORITY
	STPTI_Driver.InterruptTaskPriority = STPTI_INTERRUPT_TASK_PRIORITY;
#else
	STPTI_Driver.InterruptTaskPriority = InitParams->InterruptProcessPriority;
#endif
        /* Partition passed for task_init GNBvd21068*/
        Error = stpti_InterruptQueueInit( InitParams->Partition_p );

        /* Initialise EventQueue and associated process */
#ifdef STPTI_EVENT_TASK_PRIORITY
    	STPTI_Driver.EventTaskPriority = STPTI_EVENT_TASK_PRIORITY;
#else
        STPTI_Driver.EventTaskPriority = InitParams->EventProcessPriority;
#endif
        /* Partition passed for task_init GNBvd21068*/
         Error = stpti_EventQueueInit( InitParams->Partition_p );

#ifndef STPTI_NO_INDEX_SUPPORT
        /* Initialise IndexQueue and associated process */
#ifdef STPTI_INDEX_TASK_PRIORITY
        STPTI_Driver.IndexTaskPriority = STPTI_INDEX_TASK_PRIORITY;
#else
        STPTI_Driver.IndexTaskPriority = InitParams->IndexProcessPriority;
#endif
#endif
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

    ExistingSessionDeviceHandle.word = STPTI_NullHandle();

    if ( Error == ST_NO_ERROR )   /* Initialise memory for NSessions & Handles to them */
    {
#if defined (STPTI_FRONTEND_HYBRID)
        U32              DuplicateStreams = 0;
        STPTI_StreamID_t ExistingStreamID = 0;
#endif
        STOS_SemaphoreWait( stpti_MemoryLock );

        /* Scan all initialised drivers and check if TCDeviceAddress  or with the same name */

        STTBX_Print(( "STPTI: scanning %d devices for a match\n", stpti_GetNumberOfExistingDevices()));

        for ( device = 0; device < stpti_GetNumberOfExistingDevices(); ++device )
        {
            Device_t *Device_p = stpti_GetDevice( device );

            if ( Device_p != NULL )
            {
#if !defined ( STPTI_BSL_SUPPORT )
                if ( stpti_DeviceNameExists( DeviceName ))   /* all stpti device names must be unique */
                {
                    STTBX_Print(( "STPTI: names equal '%s' '%s'\n", Device_p->DeviceName, DeviceName ));
                    Error = ST_ERROR_ALREADY_INITIALIZED;
                    break;
                }
#endif
                STTBX_Print(("Device found on TCDevice 0x%08x requested TCDevice 0x%08x\n",(U32)Device_p->TCDeviceAddress_p, (U32)InitParams->TCDeviceAddress_p ));
                if ( Device_p->TCDeviceAddress_p == InitParams->TCDeviceAddress_p )
                {
                    STTBX_Print(( "STPTI: PTI cell address match 0x%08x...", (U32)InitParams->TCDeviceAddress_p ));

                    /* if tsmerger bypass mode ( no tag bytes ) then we can only have one session as the
                     tag bytes tell us which transport i/p is routed to which session */
                    if (  Device_p->StreamID == STPTI_STREAM_ID_NOTAGS )
                    {
                        STTBX_Print(( "STPTI_STREAM_ID_NOTAGS!\n" ));
                        Error = ST_ERROR_ALREADY_INITIALIZED;
                        break;
                    }
#if defined (STPTI_FRONTEND_HYBRID)
                    STTBX_Print(("STPTI_Init: Found Device StreamID 0x%08x requested 0x%08x\n", Device_p->StreamID, InitParams->StreamID));
                    if ( (Device_p->StreamID & 0xFF7F) == InitParams->StreamID )
                    {
                        ExistingStreamID = Device_p->StreamID;
                        STTBX_Print(("\nSTPTI_Init: Existing stream match 0x%04x\n",Device_p->StreamID));
                        DuplicateStreams++;
                        STTBX_Print(("STPTI_Init: duplicate stream found\n"));
                    }

                    /* Allow only 2 stream IDs the same for TSIN types */
                    if ( InitParams->StreamID & (STPTI_STREAM_IDTYPE_TSIN << 8) )
                    {
                        if (( (Device_p->StreamID & 0xFF7F) == InitParams->StreamID ) &&
                            ( DuplicateStreams > 1 )  )
                        {
                            STTBX_Print(( "exceeds two: a driver->StreamID = InitParams->StreamID = %d\n", InitParams->StreamID ));
                            Error = ST_ERROR_ALREADY_INITIALIZED;
                            break;
                        }
                    }
                    else
                    {
#endif
                        /* transport routing must be unique for SWTS type */
                        if (( Device_p->StreamID == InitParams->StreamID ) &&
                            ( InitParams->StreamID != STPTI_STREAM_ID_NONE ) )
                        {
                            STTBX_Print(( "not unique: a driver->StreamID = InitParams->StreamID = %d\n", InitParams->StreamID ));
                            Error = ST_ERROR_ALREADY_INITIALIZED;
                            break;
                        }
#if defined (STPTI_FRONTEND_HYBRID)
                    }
#endif
                    if ( stptiHAL_PeekNextFreeSession( Device_p->Handle ) == SESSION_NOT_ALLOCATED )
                    {
                        STTBX_Print(( "Session limit for individual cell! " ));
                        Error = ST_ERROR_ALREADY_INITIALIZED;
                        break;
                    }
                    else    /* a previous session exists */
                    {
                        if ( ExistingSessionDeviceHandle.word == STPTI_NullHandle()) /* choose first device we come across */
                        {
                            ExistingSessionDeviceHandle = Device_p->Handle;
                            STTBX_Print(( "STPTI_init: ExistingSessionDeviceHandle:%x\n", ExistingSessionDeviceHandle.word ));
                        }
                    }
                    STTBX_Print(( "STPTI_init: scan loop device:%d ok\n", device ));
                }
#ifdef STTBX_PRINT
                else
                {
                    STTBX_Print(( "STPTI: PTI cell address NO match ( current: 0x%08x ) ( InitParams: 0x%08x )\n",
                                       Device_p->TCDeviceAddress_p, InitParams->TCDeviceAddress_p ));
                }
#endif
            }

        }   /* for( device ) */

        if ( Error == ST_NO_ERROR )
        {
            Error = stpti_MemInit( InitParams->Partition_p, ( sizeof( Session_t ) + sizeof( CellHeader_t )) * InitialSessions,
                              sizeof( Handle_t ) * InitialSessions, (MemCtl_t *) &TmpDevice.MemCtl );

            if ( Error == ST_NO_ERROR )
            {
                /* Create a Device entry in parent Driver */
                Error = stpti_CreateDeviceHandle( &TmpDevice.Handle );
                if ( Error != ST_NO_ERROR )
                {
#if !defined ( STPTI_BSL_SUPPORT )
                    stpti_MemTerm( &TmpDevice.MemCtl );   /* free memory block before Error return */
#endif
                }
                else
                {
                    /* Load Device Parameters ( that aren't already loaded ) */
                    memcpy((void *) &(TmpDevice.DeviceName), DeviceName, sizeof( ST_DeviceName_t ));

                    /* EventHandlerName will be filled in after successful Initialisation */
                    memset((void *) &(TmpDevice.EventHandlerName), 0, sizeof( ST_DeviceName_t ));

                    TmpDevice.TCDeviceAddress_p          = InitParams->TCDeviceAddress_p;
                    TmpDevice.Architecture               = InitParams->Device;
                    TmpDevice.TCCodes                    = InitParams->TCCodes;
                    TmpDevice.DescramblerAssociationType = InitParams->DescramblerAssociation;

                    TmpDevice.DriverPartition_p          = InitParams->Partition_p;
                    TmpDevice.NonCachedPartition_p       = InitParams->NCachePartition_p;
                    TmpDevice.InterruptLevel             = InitParams->InterruptLevel;
                    TmpDevice.InterruptNumber            = InitParams->InterruptNumber;
                    TmpDevice.SyncLock                   = InitParams->SyncLock;
                    TmpDevice.SyncDrop                   = InitParams->SyncDrop;
                    TmpDevice.SectionFilterOperatingMode = InitParams->SectionFilterOperatingMode;

                    TmpDevice.AlternateOutputSet         = STPTI_NullHandle();
                    TmpDevice.AlternateOutputType        = STPTI_ALTERNATE_OUTPUT_TYPE_NO_OUTPUT;
                    TmpDevice.AlternateOutputTag         = 0;

                    TmpDevice.EventHandle                = 0;
                    TmpDevice.EventHandleValid           = FALSE;

#if defined (STPTI_FRONTEND_HYBRID)
                    TmpDevice.FrontendListHandle.word    = STPTI_NullHandle();
#endif
#ifdef STPTI_CAROUSEL_SUPPORT
                    TmpDevice.EntryType                  = InitParams->CarouselEntryType;
                    TmpDevice.CarouselHandle             = STPTI_NullHandle();
                    TmpDevice.CarouselSem_p              = NULL;
#ifdef STPTI_CAROUSEL_TASK_PRIORITY
                    TmpDevice.CarouselProcessPriority    = STPTI_CAROUSEL_TASK_PRIORITY;
#else
                    TmpDevice.CarouselProcessPriority    = InitParams->CarouselProcessPriority;
#endif
#endif
                    TmpDevice.WildCardUse                = STPTI_NullHandle();
                    TmpDevice.NegativePIDUse             = STPTI_NullHandle();
#ifndef STPTI_NO_INDEX_SUPPORT
                    TmpDevice.IndexAssociationType       = InitParams->IndexAssociation;
                    TmpDevice.IndexEnable                = FALSE;
#endif
                    TmpDevice.DiscardSyncByte            = FALSE;
                    TmpDevice.StreamID                   = InitParams->StreamID;
#if defined (STPTI_FRONTEND_HYBRID)
                    /* Alter the replication bit if necessary */
                    if ( DuplicateStreams )
                    {
                        STTBX_Print(("STPTI_Init replicating stream ID for existing ID 0x%04x\n",ExistingStreamID))
                        if ( !(ExistingStreamID & 0x0080) )
                        {
                             TmpDevice.StreamID |= 0x0080;
                        }
                        STTBX_Print(("STPTI_Init replicating new stream ID to 0x%04x\n", TmpDevice.StreamID));
                    }
#endif
                    TmpDevice.PacketArrivalTimeSource    = InitParams->PacketArrivalTimeSource;
                    TmpDevice.NumberOfSlots              = InitParams->NumberOfSlots;   /* the HAL deals with validating this */
                    TmpDevice.Session                    = SESSION_NOT_ALLOCATED;       /* stptiHAL_Init() will modify this value */
                    TmpDevice.AlternateOutputLatency     = InitParams->AlternateOutputLatency;
                    TmpDevice.DiscardOnCrcError          = InitParams->DiscardOnCrcError;

                    switch ( TmpDevice.SectionFilterOperatingMode )
                    {
                        case  STPTI_FILTER_OPERATING_MODE_8x8:
                        case  STPTI_FILTER_OPERATING_MODE_8x16:
                                        TmpDevice.NumberOfSectionFilters = 8;
                                        break;

                        case STPTI_FILTER_OPERATING_MODE_16x8:
                        case STPTI_FILTER_OPERATING_MODE_16x16:
                                        TmpDevice.NumberOfSectionFilters = 16;
                                        break;

                        case STPTI_FILTER_OPERATING_MODE_32x8:
                        case STPTI_FILTER_OPERATING_MODE_32x16:
                                        TmpDevice.NumberOfSectionFilters = 32;
                                        break;

                        case STPTI_FILTER_OPERATING_MODE_48x8:
                                        TmpDevice.NumberOfSectionFilters = 48;
                                        break;

                        case STPTI_FILTER_OPERATING_MODE_64x8:
                                        TmpDevice.NumberOfSectionFilters = 64;
                                        break;

                        case STPTI_FILTER_OPERATING_MODE_ANYx8:
                                        /* get number of filters from new init parameter.
                                        This provides finer granularity of filter usage across sessions */
                                        TmpDevice.NumberOfSectionFilters = InitParams->NumberOfSectionFilters;
                                        break;

                        case STPTI_FILTER_OPERATING_MODE_NONE:
                                        TmpDevice.NumberOfSectionFilters = 0;
                                        break;

                        default:        return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
                    }

                    /* No interrupts installed for this device. */
                    TmpDevice.IRQHolder = FALSE;
                    
                    /* Copy Data to allocated memory */
                    memcpy((void *) stptiMemGet_Device( TmpDevice.Handle ),
                            (void *) &TmpDevice,
                            sizeof( Device_t ));
#if defined( ST_OSLINUX )
                    stpti_LinuxEventQueueInit( TmpDevice.Handle );
#endif /* #if defined(ST_OSLINUX) */
                    STTBX_Print(( "STPTI_init: TmpDevice.Handle:%x\n", TmpDevice.Handle.word ));

                    /* Initialise TC code and data */
                    Error = stptiHAL_Init( TmpDevice.Handle,
                                           ExistingSessionDeviceHandle,
                                           InitParams->TCLoader_p);

                    /* we should have a valid session */
                    assert( stptiMemGet_Device( TmpDevice.Handle )->Session != SESSION_NOT_ALLOCATED );


                    
                    if ( Error != ST_NO_ERROR )   /* Initialise Event Handling */
                    {
#if !defined ( STPTI_BSL_SUPPORT )
                        stpti_DeallocateDevice(TmpDevice.Handle, TRUE);
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
                    }
                    else
                    {
#if defined( ST_OSLINUX )
                        AddPTIDeviceToProcFS( TmpDevice.Handle );
#endif /* #if defined( ST_OSLINUX ) */
                        
                        /* Check if the named event handler has been used with other STPTI devices */
                        for ( device = 0; device < stpti_GetNumberOfExistingDevices(); ++device )
                        {
                            Device_t *Device_p = stpti_GetDevice( device );

                            if ( Device_p != NULL )
                            {
                                OldDeviceHandle = Device_p->Handle;

                                if (strncmp((void *)&(InitParams->EventHandlerName),
                                            (void *)&(stptiMemGet_Device(OldDeviceHandle)->EventHandlerName),
                                            sizeof( ST_DeviceName_t )) == 0 )
                                {
                                    break;
                                }
                            }
                        }

#if defined( STPTI_GPDMA_SUPPORT )
                        /* Setup common STGPDMA params */
                     /* GPDMATransferParams.TimingModel              = STGPDMA_TIMING_FREE_RUNNING; */
                        GPDMATransferParams.Source.TransferType      = STGPDMA_TRANSFER_1D_INCREMENTING;
                        GPDMATransferParams.Source.UnitSize          = 32;
                        GPDMATransferParams.Destination.TransferType = STGPDMA_TRANSFER_1D_INCREMENTING;
                        GPDMATransferParams.Destination.UnitSize     = 32;
                        GPDMATransferParams.Next                     = NULL;
#endif
#if defined( STPTI_FDMA_SUPPORT )
                        /*IF FIRST TIME*/

                        OriginalFDMANode_p[stptiMemGet_Device(TmpDevice.Handle)->Session] = memory_allocate( TmpDevice.NonCachedPartition_p,
                                                              sizeof( STFDMA_Node_t ) + 31 );
                        FDMANode_p = (STFDMA_Node_t *) ((((U32) OriginalFDMANode_p[stptiMemGet_Device(TmpDevice.Handle)->Session] ) +31 ) & ~31 );
                        FDMANode_p->Next_p = NULL;
                        FDMANode_p->SourceStride = 0;
                        FDMANode_p->DestinationStride = 0;
                        FDMANode_p->NodeControl.PaceSignal = STFDMA_REQUEST_SIGNAL_NONE;
                        FDMANode_p->NodeControl.SourceDirection = STFDMA_DIRECTION_INCREMENTING;
                        FDMANode_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
                        FDMANode_p->NodeControl.NodeCompleteNotify = FALSE;
                        FDMANode_p->NodeControl.NodeCompletePause = FALSE;
                        FDMANode_p->NodeControl.Reserved = 0;

                        FDMAGenericTransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
                        FDMAGenericTransferParams.Pool = STFDMA_DEFAULT_POOL;
                        FDMAGenericTransferParams.CallbackFunction = NULL;
                        FDMAGenericTransferParams.BlockingTimeout = 0;
                        FDMAGenericTransferParams.ApplicationData_p = NULL;
                        FDMAGenericTransferParams.NodeAddress_p = (STFDMA_GenericNode_t *) ((U32)TRANSLATE_PHYS_ADD(FDMANode_p));

#if defined( STPTI_FDMA_BLOCK )
                        /* If using 2.0.0 or later of STFDMA define STPTI_FDMA_BLOCK to be STFDMA_1 or STFDMA_2 depending on which */
                        /* block you want STPTI to use, if you don't at the top of this file STPTI_FDMA_BLOCK will default to      */
                        /* STFDMA_1 (see top of this file).   If using an older version of STFDMA don't define STPTI_FDMA_BLOCK    */
                        /* and instead define STPTI_FDMA_VERSION1_SUPPORT */
                        FDMAGenericTransferParams.FDMABlock = STPTI_FDMA_BLOCK;
#endif

#endif

                        /* Store event handler name in device structure */
                        memcpy( (void *) &(stptiMemGet_Device( TmpDevice.Handle )->EventHandlerName),
                                (void *) &(InitParams->EventHandlerName),
                                sizeof( ST_DeviceName_t ));
                        {
                            /* Register Events */
                            Error = stpti_EventOpen( TmpDevice.Handle );
                            if ( Error == ST_NO_ERROR )
                            {
                                Error = stpti_EventRegister( TmpDevice.Handle );
                            }
                        }
                    }
                }
            }
        }

        STOS_SemaphoreSignal( stpti_MemoryLock );
    }
    
    return Error;
}

/******************************************************************************
Function Name :STPTI_SlotLinkToBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotLinkToBuffer( STPTI_Slot_t Slot, STPTI_Buffer_t Buffer )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;
    FullHandle_t FullBufferHandle;
    FullHandle_t BufferListHandle;
    FullHandle_t SlotListHandle;
    BOOL DMA_Exists;

    
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word   = Slot;
    FullBufferHandle.word = Buffer;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_SlotLinkToBuffer( FullSlotHandle, FullBufferHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error == ST_NO_ERROR )
    {
        /* If buffer already has a slot linked to it, then re-use the DMA previously allocated */
        SlotListHandle = ( stptiMemGet_Buffer( FullBufferHandle ))->SlotListHandle;

        if ( SlotListHandle.word != STPTI_NullHandle() )
        {
            /* This is only allowed on RAW mode slots... check and return Error if violated */
            if ((( stptiMemGet_Slot( FullSlotHandle ))->Type != STPTI_SLOT_TYPE_RAW )
             &(( stptiMemGet_Slot( FullSlotHandle ))->Type != STPTI_SLOT_TYPE_ECM ) &
                 (( stptiMemGet_Slot( FullSlotHandle ))->Type != STPTI_SLOT_TYPE_EMM )
                 )
            {
                Error = STPTI_ERROR_SLOT_NOT_RAW_MODE;
            }
            else
            {
                DMA_Exists = TRUE;
            }
        }
        else
        {
            /* Enable buffer Interrupt and set stream type */
            ( stptiMemGet_Buffer( FullBufferHandle ))->Type =
                                  (stptiMemGet_Slot( FullSlotHandle ))->Type;
            DMA_Exists = FALSE;
        }

        if ( Error == ST_NO_ERROR )
        {
            /* set-up a new DMA structure or reuse existing one */
            Error = stptiHAL_SlotLinkToBuffer( FullSlotHandle,
                                               FullBufferHandle,
                                               DMA_Exists );

            if ( Error == ST_NO_ERROR )
            {
                BufferListHandle = ( stptiMemGet_Slot( FullSlotHandle ))->BufferListHandle;

                if ( BufferListHandle.word == STPTI_NullHandle())
                {
                    Error = stpti_CreateListObject( FullSlotHandle, &BufferListHandle );
                    if ( Error == ST_NO_ERROR )
                    {
                        ( stptiMemGet_Slot( FullSlotHandle ))->BufferListHandle = BufferListHandle;
                    }
                }

                if ( Error == ST_NO_ERROR )
                {
                    SlotListHandle = ( stptiMemGet_Buffer( FullBufferHandle ))->SlotListHandle;

                    if ( SlotListHandle.word == STPTI_NullHandle())
                    {
                        Error = stpti_CreateListObject( FullBufferHandle,
                                                        &SlotListHandle );
                        if ( Error == ST_NO_ERROR )
                        {
                            ( stptiMemGet_Buffer( FullBufferHandle ))->SlotListHandle = SlotListHandle;
                        }
                    }
                }

                if ( Error == ST_NO_ERROR )
                {
                    Error = stpti_AssociateObjects( BufferListHandle,
                                                    FullSlotHandle,
                                                    SlotListHandle,
                                                    FullBufferHandle );
                }
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

#if !defined ( STPTI_BSL_SUPPORT )
/************************************************************************************************************
Name:         STPTI_SlotLinkToRecordBuffer
Description:  Links a slot of any slot type to a buffer for watch and record.
              The buffer must be previously allocated with a call to STPTI_BufferAllocate().
              Only one `Watch & Record' buffer can be used per session.
              
             
Parameters:              
              SlotHandle   - The slot to be linked to the record buffer.  
              BufferHandle - The record buffers handlde. Only one per virtual pti session can exist. 
              DescrambleTS - A boolean indicating if the recording is to be as transmitted or descrambled.
Errors:
              STPTI_ERROR_INVALID_SLOT_HANDLE
              STPTI_ERROR_BUFFER_NOT_LINKED
              STPTI_ERROR_SLOT_ALREADY_LINKED
              STPTI_ERROR_NO_FREE_DMAS
              STPTI_ERROR_INVALID_BUFFER_HANDLE - the buffer handle is invalid
              STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION - all slots associated with a given record buffer must be from the same session.
              
************************************************************************************************************/
ST_ErrorCode_t STPTI_SlotLinkToRecordBuffer(STPTI_Slot_t Slot, STPTI_Buffer_t Buffer, BOOL DescrambleTS)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;
    FullHandle_t FullBufferHandle;
    FullHandle_t BufferListHandle;
    FullHandle_t SlotListHandle;
    BOOL DMA_Exists;
    Device_t   *DeviceStruct_p;
    Slot_t      *SlotStruct_p;
    Buffer_t    *BufferStruct_p;
    

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word   = Slot;
    FullBufferHandle.word = Buffer;

    DeviceStruct_p = stptiMemGet_Device(FullSlotHandle);
    SlotStruct_p    = stptiMemGet_Slot(FullSlotHandle);
    BufferStruct_p  = stptiMemGet_Buffer(FullBufferHandle);

    Error = stptiValidate_SlotLinkToRecordBuffer( FullSlotHandle, FullBufferHandle );
    if ( Error == ST_NO_ERROR )
    {
        SlotListHandle = BufferStruct_p->SlotListHandle;

        if ( SlotListHandle.word != STPTI_NullHandle() )
        {
            if ( FALSE == BufferStruct_p->IsRecordBuffer )
            {
                Error = STPTI_ERROR_SLOT_ALREADY_LINKED;
            }
            else
            {
                DMA_Exists = TRUE;
            }
        }
        else
        {
            /* Set Slot type */
            BufferStruct_p->Type = SlotStruct_p->Type;
            DMA_Exists = FALSE;
        }

        if ( Error == ST_NO_ERROR )
        {
            /* set-up a new DMA structure or reuse existing one */
            Error = stptiHAL_SlotLinkToRecordBuffer( FullSlotHandle,
                                                     FullBufferHandle,
                                                     DMA_Exists,
                                                     DescrambleTS );

            if ( Error == ST_NO_ERROR )
            {
                /* Make sure you set the buffer as record for future associations */
                BufferStruct_p->IsRecordBuffer = TRUE;

                BufferListHandle = SlotStruct_p->BufferListHandle;

                if ( BufferListHandle.word == STPTI_NullHandle())
                {
                    Error = stpti_CreateListObject( FullSlotHandle, &BufferListHandle );
                    if ( Error == ST_NO_ERROR )
                    {
                        SlotStruct_p->BufferListHandle = BufferListHandle;
                    }
                }

                if ( Error == ST_NO_ERROR )
                {
                    SlotListHandle = BufferStruct_p->SlotListHandle;

                    if ( SlotListHandle.word == STPTI_NullHandle())
                    {
                        Error = stpti_CreateListObject( FullBufferHandle,
                                                        &SlotListHandle );
                        if ( Error == ST_NO_ERROR )
                        {
                            BufferStruct_p->SlotListHandle = SlotListHandle;
                        }
                    }
                }

                if ( Error == ST_NO_ERROR )
                {
                    Error = stpti_AssociateObjects( BufferListHandle,
                                                    FullSlotHandle,
                                                    SlotListHandle,
                                                    FullBufferHandle );
                }
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}



/******************************************************************************
Function Name : STPTI_SlotState
  Description :
   Parameters :
******************************************************************************/
/*
 * Harden stptiHAL_SlotState() as it is recursive ( 48 slots )
 * valid MAX_RECURSE_DEPTH range is 1 to 255
 */
#define MAX_RECURSE_DEPTH 49
ST_ErrorCode_t STPTI_SlotState( STPTI_Slot_t SlotHandle,
                                U32 *SlotCount_p,
                                STPTI_ScrambleState_t *ScrambleState_p,
                                STPTI_Pid_t *Pid_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotState( FullSlotHandle,
                                     SlotCount_p,
                                     ScrambleState_p,
                                     Pid_p );
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_SlotState( FullSlotHandle,
                                    SlotCount_p,
                                    ScrambleState_p,
                                    Pid_p,
                                    MAX_RECURSE_DEPTH );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
/******************************************************************************
Function Name : STPTI_Open
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_Open( ST_DeviceName_t DeviceName,
                           const STPTI_OpenParams_t *OpenParams,
                           STPTI_Handle_t * Handle )
{
    FullHandle_t DeviceHandle;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Session_t TmpSession;
    U16 InitialObjects = 100;
    U16 indx;
    U8 *src, *dest;

    
    /* Escape if there is no MemoryLock Semaphore - means we have not initialised any PTI sessions */
    if( stpti_MemoryLock == NULL ) return( STPTI_ERROR_NOT_INITIALISED );

    STOS_SemaphoreWait( stpti_MemoryLock );

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_Open( DeviceName, OpenParams, Handle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error == ST_NO_ERROR )
    {
        Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, TRUE );
        if ( Error == ST_NO_ERROR )
        {
            /* Initialise memory block for N Slots and handles to them */
            size_t SizeDataSpace   = ( sizeof( Slot_t ) + sizeof( CellHeader_t )) * InitialObjects;
            size_t SizeHandleSpace = sizeof( Handle_t ) * InitialObjects;

            stpti_MemInit( OpenParams->DriverPartition_p,
                           SizeDataSpace,
                           SizeHandleSpace,
                           &TmpSession.MemCtl );

            /* Create a session entry in parent Device */
            Error = stpti_CreateSessionHandle( DeviceHandle, &TmpSession.Handle );

            if ( Error == ST_NO_ERROR )
            {
                /* Load Session Parameters ( that aren't already loaded ) */
                TmpSession.DriverPartition_p    = OpenParams->DriverPartition_p;
                TmpSession.NonCachedPartition_p = OpenParams->NonCachedPartition_p;

                for ( indx = 0; indx <= OBJECT_TYPE_LIST; ++indx )
                {
                    TmpSession.AllocatedList[indx].word = STPTI_NullHandle();
                }
#if defined( ST_OSLINUX )
                TmpSession.evt_p = stpti_LinuxAllocateEvtStruct(SESSION_EVT_QSIZE);
#endif /* #endif defined( ST_OSLINUX ) */
                src  = ( U8 * ) &TmpSession;
                dest = ( U8 * ) stptiMemGet_Session( TmpSession.Handle );

                memcpy( dest, src, sizeof( Session_t ));    /* Copy Data to allocated memory */

                *Handle = TmpSession.Handle.word;
            }
        }
    }
    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_SearchSpecifiedSlotsForPid
  Description : Iterates from specified Slot number to last Slot on Sessions
                from StartSessionNo to MaxSession ( recovered from Handle ).
                to see if the Pid in question is already being collected.

                If Pid is collected already then
                    Slot_p is pointed to the slot on which it is being collected
                else
                    Slot_p is set to STPTI_NullHandle

  Parameters : NewSlotHandle - Handle to the slot for which we are looking for
               an already existing duplicate.  Also supplies handle to Session
               to be searched.
               Pid - Pid for which the NewSlot is to be set to collect.
               Slot_p - return value; holds handle of matching existing slot,
               or NullHandle if no such duplicate exists.
               StartSlotNo - Slot number to start at.

  Returns :    ST_ErrorCode as appropriate.
               If ErrorCode != NO_ERROR then Slot_p == NullHandle
 ******************************************************************************/
static ST_ErrorCode_t stpti_SearchSpecifiedSlotsForPid( FullHandle_t NewSlotHandle,
                                                        STPTI_Pid_t Pid,
                                                        STPTI_Slot_t * Slot_p,
                                                        U16 StartSessionNo,
                                                        U16 StartSlotNo )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    /* Ensure that early termination returns the right result; no match */
    *Slot_p = STPTI_NullHandle();

    
    /* Check we were given a valid handle */
    if ( NewSlotHandle.word != STPTI_NullHandle())
    {
        U16 MaxSessions = stptiMemGet_Device( NewSlotHandle )->MemCtl.MaxHandles;
        U16 SessionIndex = StartSessionNo;
        FullHandle_t SessionHandle;

        while (( SessionIndex < MaxSessions ) && ( *Slot_p == STPTI_NullHandle()))
        {
            SessionHandle = stptiMemGet_Device( NewSlotHandle )->MemCtl.Handle_p[SessionIndex].Hndl_u;

            /* Is Session handle valid? */
            if ( SessionHandle.word != STPTI_NullHandle())
            {
                /*
                 *  Recover the handle of the list of slots currently allocated to
                 *  the Session
                 */
                FullHandle_t SlotListHandle =
                    stptiMemGet_Session( SessionHandle )->AllocatedList[OBJECT_TYPE_SLOT];

                /* Check whether the list of slots' handle is valid */
                if ( SlotListHandle.word != STPTI_NullHandle())
                {
                    FullHandle_t SlotHandle;
                    U16 MaxSlots = stptiMemGet_List( SlotListHandle )->MaxHandles;
                    /*
                     * If this is the first Session we've searched, start where the caller
                     * specified. If this isn't the first Session, StartSlotNo will have
                     * been set to 0 ( see a few lines below )
                     */
                    U16 SlotIndex = StartSlotNo;
                    /* If we start another Session, we need start at the first Slot */
                    StartSlotNo = 0;

                    /*
                     * Iterate over all allocated slots until we find a matching pid,
                     * or run out of Slots.
                     */
                    while (( SlotIndex < MaxSlots ) && ( *Slot_p == STPTI_NullHandle()))
                    {
                        SlotHandle.word =
                            ( &stptiMemGet_List( SlotListHandle )->Handle )[SlotIndex];

                        /*
                         * If the Slot handle is valid AND
                         *  is on the same Pid AND
                         *  is not the new slot we passed in THEN
                         *  It's the match we're looking for
                         */
                        if (( SlotHandle.word != STPTI_NullHandle()) &&
                            (( stptiMemGet_Slot( SlotHandle ))->Pid == Pid ) &&
                            ( SlotHandle.word != NewSlotHandle.word ))
                        {
                            /* Set returned value and force loop termination */
                            *Slot_p = SlotHandle.word;
                        }
                        ++SlotIndex;
                    } /* SlotIndex is valid and we haven't found a match */
                } /* Slot list handle was valid */
            } /* Slot is allocated to a Session */
            ++SessionIndex;
        } /* While valid session, and not found match */
    }
    else
    {
        /* The new slot handle passed in was invalid */
        Error = ST_ERROR_BAD_PARAMETER;
    }

    return( Error );
}


/******************************************************************************
Function Name : stpti_SlotQueryPid
  Description : Iterates over all slots on Session to which the NewSlotHandle
                is allocated, to see if the Pid in question is already being
                collected.

                If Pid is collected already then
                    Slot_p is pointed to the slot on which it is being collected
                else
                    Slot_p is set to STPTI_NullHandle

  Parameters : NewSlotHandle - Handle to the slot for which we are looking for
               an already existing duplicate.  Also supplies handle to Session
               to be searched.
               Pid - Pid for which the NewSlot is to be set to collect.
               Slot_p - return value; holds handle of matching existing slot,
               or NullHandle if no such duplicate exists.

  Returns :    ST_ErrorCode as appropriate.
               If ErrorCode != NO_ERROR then Slot_p == NullHandle
******************************************************************************/
ST_ErrorCode_t stpti_SlotQueryPid( FullHandle_t NewSlotHandle,
                                  STPTI_Pid_t Pid,
                                  STPTI_Slot_t * Slot_p )
{
    
    /* 0, 0 => Search ALL slots */
    return stpti_SearchSpecifiedSlotsForPid( NewSlotHandle, Pid, Slot_p, 0, 0 );
}


/******************************************************************************
Function Name : stpti_NextSlotQueryPid
  Description : Goes through all slots on all sessions to see if the Pid in
                question is already being collected.
                If Pid is collected already then
                    Slot_p is pointed to the slot on which it is being collected
                else
                    Slot_p is set to STPTI_NullHandle

   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_NextSlotQueryPid( FullHandle_t NewSlotHandle,
                                       STPTI_Pid_t Pid,
                                       STPTI_Slot_t * Slot_p )
{
    /* Set default values, which specify full search of all Slots on all Sessions */
    U16 Slot = 0;
    U16 Session = 0;

    if ( *Slot_p != STPTI_NullHandle() )
    {
        /*
         * Caller has specified that search is not to cover all Slots for all
         * Sessions, but is to start at the Slot after the specified current
         * position
         */
        FullHandle_t FullSlotHandle;
        FullHandle_t SessionHandle;

        FullSlotHandle.word = *Slot_p;

        /* Start with the current slot's session */
        Session = FullSlotHandle.member.Session;

        /* Unfortunately we can't use
         *   Slot = FullSlotHandle.member.Object + 1;
         * because member.Object is not an index into the array we are using
         */

        /* We need to iterate over the Session's Slot list looking for slot_p */
        SessionHandle = stptiMemGet_Device(NewSlotHandle)->MemCtl.Handle_p[Session].Hndl_u;

        if (SessionHandle.word != STPTI_NullHandle())
        {
            FullHandle_t SlotListHandle;

            SlotListHandle = stptiMemGet_Session(SessionHandle)->AllocatedList[OBJECT_TYPE_SLOT];

            if (SlotListHandle.word != STPTI_NullHandle())
            {
                U16 MaxSlots;

                MaxSlots = stptiMemGet_List(SlotListHandle)->MaxHandles;

                while ((Slot < MaxSlots) &&
                      (FullSlotHandle.word != (&stptiMemGet_List(SlotListHandle)->Handle)[Slot]))
                {
                    ++Slot;
                }

                if (Slot < MaxSlots)
                {
                    /* Don't check the slot given to us, start at the next one
                     *
                     * don't worry about overflowing session's slots...boundaries are checked
                     * by stpti_SearchSpecifiedSlotsForPid
                     */
                    ++Slot;
                }
                else
                    /* We were promised a match & we didn't get one - we have an error */
                    return ST_ERROR_BAD_PARAMETER;
            }
            else
                return ST_ERROR_BAD_PARAMETER;
        }
        else
            return ST_ERROR_BAD_PARAMETER;
    }
    /* else - check all slots on all sessions */

    return stpti_SearchSpecifiedSlotsForPid( NewSlotHandle,
                                             Pid,
                                             Slot_p,
                                             Session,
                                             Slot );
}

/******************************************************************************
Function Name :STPTI_SignalAllocate
  Description :
   Parameters :
******************************************************************************/

ST_ErrorCode_t STPTI_SignalAllocate( STPTI_Handle_t Handle,
                                     STPTI_Signal_t *SignalHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSessionHandle;
    FullHandle_t FullSignalHandle;
    Signal_t TmpSignal;
    message_queue_t *Queue_p;


    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSessionHandle.word = Handle;

    Error = stptiValidate_SignalAllocate( FullSessionHandle, SignalHandle );
    if ( Error == ST_NO_ERROR )
    {
        partition_t *Partition_p = stptiMemGet_Session( FullSessionHandle )->DriverPartition_p;
        Partition_p = Partition_p;    /* avoid unused variable compiler warning when compiling under OS21 */

        Queue_p = STOS_MessageQueueCreate( 2 * sizeof( STPTI_Buffer_t ), SIGNAL_QUEUE_SIZE );

        if ( Queue_p == NULL )
        {
            Error = ST_ERROR_NO_MEMORY;
        }
        else if ( ST_NO_ERROR != ( Error = stpti_CreateObjectHandle( FullSessionHandle,
                                                                     OBJECT_TYPE_SIGNAL,
                                                                     sizeof( Signal_t ),
                                                                     &FullSignalHandle )))
        {
            message_delete_queue( Queue_p );  /* Free up the queue before quiting */
        }
        else
        {
            /* Load Parameters */
            TmpSignal.OwnerSession          = Handle;
            TmpSignal.Handle                = FullSignalHandle.word;
            TmpSignal.QueueWaiting          = 0;
            TmpSignal.Queue_p               = Queue_p;
#if defined( ST_OSLINUX )
            TmpSignal.BufferWithData = STPTI_NullHandle();
#endif /* #endif defined( ST_OSLINUX ) */
            TmpSignal.BufferListHandle.word = STPTI_NullHandle();

            memcpy( ( U8* ) stptiMemGet_Signal( FullSignalHandle ),
                    ( U8 * ) &TmpSignal,
                    sizeof( Signal_t ));

            *SignalHandle = FullSignalHandle.word;

#if defined( ST_OSLINUX )
            /* Add to the proc file system */
            AddPTISignalToProcFS(FullSignalHandle);
#endif /* #endif defined( ST_OSLINUX ) */
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}


/******************************************************************************
Function Name :STPTI_SignalAssociateBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SignalAssociateBuffer( STPTI_Signal_t SignalHandle,
                                            STPTI_Buffer_t BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSignalHandle, SignalListHandle;
    FullHandle_t FullBufferHandle, BufferListHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSignalHandle.word = SignalHandle;
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_SignalAssociateBuffer( FullSignalHandle,
                                                 FullBufferHandle );

    if ( Error == ST_NO_ERROR )
    {
        if (( stptiMemGet_Buffer( FullBufferHandle ))->SignalListHandle.word != STPTI_NullHandle())
        {
            Error = STPTI_ERROR_ONLY_ONE_SIGNAL_PER_BUFFER;
        }
        else
        {
            SignalListHandle = ( stptiMemGet_Buffer( FullBufferHandle ))->SignalListHandle;

            if ( SignalListHandle.word == STPTI_NullHandle())
            {
                Error = stpti_CreateListObject( FullBufferHandle, &SignalListHandle );
                if ( Error == ST_NO_ERROR )
                {
                    ( stptiMemGet_Buffer( FullBufferHandle ))->SignalListHandle = SignalListHandle;
                }
            }

            if ( Error == ST_NO_ERROR )
            {
                BufferListHandle = ( stptiMemGet_Signal( FullSignalHandle ))->BufferListHandle;

                if ( BufferListHandle.word == STPTI_NullHandle())
                {
                    Error = stpti_CreateListObject( FullSignalHandle, &BufferListHandle );
                    if ( Error == ST_NO_ERROR )
                    {
                        ( stptiMemGet_Signal( FullSignalHandle ))->BufferListHandle = BufferListHandle;
                    }
                }
            }

            if ( Error == ST_NO_ERROR )
            {
                Error = stpti_AssociateObjects( SignalListHandle,
                                                FullBufferHandle,
                                                BufferListHandle,
                                                FullSignalHandle );
            }
        }

        ( stptiMemGet_Buffer( FullBufferHandle ))->Queue_p =
                           ( stptiMemGet_Signal( FullSignalHandle ))->Queue_p;

        if (( stptiMemGet_Buffer( FullBufferHandle ))->SlotListHandle.word != STPTI_NullHandle())
        {
            stptiHAL_SignalSetup( FullBufferHandle ); /* Set up signals */
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name :STPTI_SignalDeallocate
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SignalDeallocate( STPTI_Signal_t SignalHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSignalHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSignalHandle.word = SignalHandle;

    Error = stptiValidate_SignalDeallocate( FullSignalHandle );

    if ( Error == ST_NO_ERROR )
    {
        Error = stpti_DeallocateSignal( FullSignalHandle, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}


/******************************************************************************
Function Name :STPTI_SignalDisassociateBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SignalDisassociateBuffer( STPTI_Signal_t SignalHandle,
                                               STPTI_Buffer_t BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSignalHandle;
    FullHandle_t FullBufferHandle;

    FullSignalHandle.word = SignalHandle;
    FullBufferHandle.word = BufferHandle;
    STOS_SemaphoreWait( stpti_MemoryLock );

    Error = stptiValidate_SignalDisassociateBuffer( FullSignalHandle,
                                                    FullBufferHandle );

    if ( Error == ST_NO_ERROR )
    {
        Error = stpti_SignalDisassociateBuffer( FullSignalHandle,
                                                FullBufferHandle,
                                                FALSE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name :STPTI_SignalWaitBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SignalWaitBuffer( STPTI_Signal_t SignalHandle,
                                       STPTI_Buffer_t * BufferWithData,
                                       U32 TimeoutMS )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t     FullSignalHandle;
    message_queue_t *Queue_p;
    STPTI_Buffer_t  *buffer_p;
    CLOCK  ClocksPerSecond = TICKS_PER_SECOND;
    CLOCK *WaitTime;
    CLOCK  time;
    CLOCK  now;

    /* Calculate the wait time */
    if ( TimeoutMS == STPTI_TIMEOUT_IMMEDIATE )
    {
        WaitTime = ( CLOCK * ) TIMEOUT_IMMEDIATE;
    }
    else if ( TimeoutMS == STPTI_TIMEOUT_INFINITY )
    {
        WaitTime = ( CLOCK * ) TIMEOUT_INFINITY;
    }
    else if (( TimeoutMS / 1000 ) > ( INT_MAX / ClocksPerSecond ))
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        now = time_now();

        if ( TimeoutMS < ( INT_MAX / ClocksPerSecond ))
        {
            time = time_plus( now, ( ClocksPerSecond * TimeoutMS ) / 1000 );
        }
        else
        {
            time = time_plus( now, ClocksPerSecond * ( TimeoutMS / 1000 ));
        }
        WaitTime = &time;
    }

    /* Wait on the buffer */

    if ( Error == ST_NO_ERROR )
    {
        if ( BufferWithData == NULL )
        {
            Error = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            FullSignalHandle.word = SignalHandle;

            STOS_SemaphoreWait( stpti_MemoryLock );   /* LOCK */

            Error = stpti_SignalHandleCheck( FullSignalHandle );
            if ( Error == ST_NO_ERROR )
            {

                Queue_p = ( stptiMemGet_Signal( FullSignalHandle ))->Queue_p;
                assert( Queue_p != NULL );
                ( stptiMemGet_Signal( FullSignalHandle ))->QueueWaiting++;

                STOS_SemaphoreSignal( stpti_MemoryLock );   /* UNLOCK */

                buffer_p = message_receive_timeout( Queue_p, WaitTime );
                if ( buffer_p == NULL )
                {
                    *BufferWithData = STPTI_NullHandle();
                    Error = ST_ERROR_TIMEOUT;
                }
                else
                {
                    *BufferWithData = buffer_p[0];
                    if ( *BufferWithData == STPTI_NullHandle())
                    {
                        Error = STPTI_ERROR_SIGNAL_ABORTED;
                    }

                    message_release( Queue_p, buffer_p );
                }

                STOS_SemaphoreWait( stpti_MemoryLock );   /* LOCK */

                ( stptiMemGet_Signal( FullSignalHandle ))->QueueWaiting--;

                STOS_SemaphoreSignal( stpti_MemoryLock ); /* UNLOCK */
            }
            else
            {
                STOS_SemaphoreSignal( stpti_MemoryLock ); /* UNLOCK */
            }
        }
    }

    return( Error );
}


/******************************************************************************
Function Name :STPTI_SignalAbort
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SignalAbort( STPTI_Signal_t SignalHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSignalHandle;
    message_queue_t *Queue_p;
    STPTI_Buffer_t *buffer_p;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSignalHandle.word = SignalHandle;

    Error = stptiValidate_SignalAbort( FullSignalHandle );
    if ( Error == ST_NO_ERROR )
    {
        Queue_p = ( stptiMemGet_Signal( FullSignalHandle ))->Queue_p;
        if ( Queue_p != NULL )
        {
             buffer_p = (STPTI_Buffer_t *)message_claim( Queue_p );
            *buffer_p = STPTI_NullHandle();
            message_send( Queue_p, buffer_p );
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name : stpti_Descrambler_AssociateSlot
  Description :
   Parameters :
         Note : Moved here from pti_extended.c following DDTS 18257
******************************************************************************/
ST_ErrorCode_t stpti_Descrambler_AssociateSlot( FullHandle_t DescramblerHandle,
                                                FullHandle_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stptiValidate_Descrambler_AssociateSlot( DescramblerHandle,
                                                     SlotHandle );

    if ( Error == ST_NO_ERROR )
    {
        FullHandle_t DescramblerListHandle;
        FullHandle_t OldDescramblerListHandle;

        /*
         * A slot can only associate with one descrambler ( but a descrambler
         * can associate with many slots!! ) .... check that this specific slot
         * does not already have an associated descrambler
         */
        DescramblerListHandle = ( stptiMemGet_Slot( SlotHandle ))->DescramblerListHandle;

        if ( DescramblerListHandle.word != STPTI_NullHandle())
        {
            /*
             * Remember that we need to associate the new descrambler
             * and then disassociate the current descrambler when the new
             * association is made
             */
            OldDescramblerListHandle = DescramblerListHandle;
        }
        else
        {
            OldDescramblerListHandle.word = STPTI_NullHandle();
        }
                
        {
            FullHandle_t SlotListHandle;

            /* If we need to dissociate the old descrambler */
            if ( OldDescramblerListHandle.word != STPTI_NullHandle() )
            {
                FullHandle_t OldDescramblerHandle;

                /* Get descrambler handle from list */
                Error = stpti_GetFirstHandleFromList( OldDescramblerListHandle,
                                                      &OldDescramblerHandle );

                if ( ST_NO_ERROR == Error )
                {
                    if ( stptiMemGet_Device( DescramblerHandle )->DescramblerAssociationType ==
                             STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
                    {
                         Error = stpti_SlotDisassociateDescrambler( SlotHandle,
                                                                    OldDescramblerHandle,
                                                                    FALSE,
                                                                    TRUE );
                    }
                    else
                    {
                        Error = stpti_PidDisassociateDescrambler( OldDescramblerHandle,
                                                                  TRUE );
                    }
                }
            }

            DescramblerListHandle = ( stptiMemGet_Slot( SlotHandle ))->DescramblerListHandle;

            if ( stptiMemGet_Device( DescramblerHandle )->DescramblerAssociationType ==
                                     STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
            {
            
                Error = stptiHAL_DescramblerAssociate( DescramblerHandle, SlotHandle );
                if (ST_NO_ERROR == Error)
                {
                    /* We KNOW this is the one-and-only association, so don't bother to check that!!! */
                    Error = stpti_CreateListObject( SlotHandle, &DescramblerListHandle );

                    if ( Error == ST_NO_ERROR )
                    {
                        ( stptiMemGet_Slot( SlotHandle ))->DescramblerListHandle = DescramblerListHandle;

                        SlotListHandle = ( stptiMemGet_Descrambler( DescramblerHandle ))->SlotListHandle;

                        if ( SlotListHandle.word == STPTI_NullHandle())
                        {
                            Error = stpti_CreateListObject( DescramblerHandle, &SlotListHandle );
                            if ( Error == ST_NO_ERROR )
                            {
                                ( stptiMemGet_Descrambler( DescramblerHandle ))->SlotListHandle = SlotListHandle;
                            }
                        }

                        if ( Error == ST_NO_ERROR )
                        {
                            Error = stpti_AssociateObjects( DescramblerListHandle,
                                                            SlotHandle,
                                                            SlotListHandle,
                                                            DescramblerHandle );
                        }
                    }
                }                
            }
            else
            {
                STPTI_Slot_t Slot;
                STPTI_Pid_t  Pid;

                Pid = ( stptiMemGet_Descrambler( DescramblerHandle ))->AssociatedPid;

                /* Scan for a Slot using this pid and associate the descrambler with it */
                Error = stpti_SlotQueryPid( DescramblerHandle, Pid, &Slot );

                while (( Error == ST_NO_ERROR ) && ( Slot != STPTI_NullHandle()))
                {
                    SlotHandle.word = Slot;

                    Error = stptiHAL_DescramblerAssociate( DescramblerHandle, SlotHandle );                    

                    if ( Error == ST_NO_ERROR )
                    {

                        if (( stptiMemGet_Slot( SlotHandle ))->DescramblerListHandle.word !=
                                                                         STPTI_NullHandle())
                        {
                            /* Already associated - this looks wrong, but ignore for now */
                        }
                        else
                        {
                            Error = stpti_CreateListObject( SlotHandle,
                                                            &DescramblerListHandle );

                            if ( Error == ST_NO_ERROR )
                            {
                                ( stptiMemGet_Slot( SlotHandle ))->DescramblerListHandle =
                                                                     DescramblerListHandle;

                                SlotListHandle = (stptiMemGet_Descrambler( DescramblerHandle ))->SlotListHandle;

                                if ( SlotListHandle.word == STPTI_NullHandle())
                                {
                                    Error = stpti_CreateListObject( DescramblerHandle,
                                                                    &SlotListHandle );
                                    if ( Error == ST_NO_ERROR )
                                    {
                                        ( stptiMemGet_Descrambler( DescramblerHandle ))->SlotListHandle =
                                                                           SlotListHandle;
                                    }
                                }

                                if ( Error == ST_NO_ERROR )
                                {
                                    Error = stpti_AssociateObjects( DescramblerListHandle,
                                                                    SlotHandle,
                                                                    SlotListHandle,
                                                                    DescramblerHandle );
                                }
                            }
                        }

                        if ( Error == ST_NO_ERROR )
                        {
                            Error = stpti_NextSlotQueryPid( DescramblerHandle, Pid, &Slot );
                        }
                    }
                }
            }
        }
    }

    return ( Error );
}

/******************************************************************************
Function Name : STPTI_SlotQuery
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotQuery( STPTI_Slot_t SlotHandle,
                                BOOL *PacketsSeen,
                                BOOL *TransportScrambledPacketsSeen,
                                BOOL *PESScrambledPacketsSeen,
                                STPTI_Pid_t *Pid )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotQuery( FullSlotHandle,
                                     PacketsSeen,
                                     TransportScrambledPacketsSeen,
                                     PESScrambledPacketsSeen, Pid );
    if ( Error == ST_NO_ERROR )
    {
        *Pid = ( stptiMemGet_Slot( FullSlotHandle ))->Pid;
        Error = stptiHAL_SlotQuery( FullSlotHandle,
                                    PacketsSeen,
                                    TransportScrambledPacketsSeen,
                                    PESScrambledPacketsSeen );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name : stpti_TransferPCRHandle
  Description : Transfer PCRReturnHandle as required 
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_TransferPCRHandle(FullHandle_t FullSlotHandle,
                                              STPTI_Slot_t ExistingSlot,
                                              BOOL *PidNeedsToBeSet)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* Slot Exists with same pid */
    FullHandle_t FullExistingSlotHandle;

    FullExistingSlotHandle.word = ExistingSlot;

    
    /* check if new slot is of type PCR */
    if (( stptiMemGet_Slot( FullSlotHandle ))->Type == STPTI_SLOT_TYPE_PCR )
    {
        /*
         * set PCRReturnHandle in existing slot to slot handle of new
         * slot in SLOT STRUCTURE
         */
        ( stptiMemGet_Slot( FullExistingSlotHandle ))->PCRReturnHandle =
                                                     FullSlotHandle.word;

        Error = stptiHAL_SlotEnableIndexing( FullExistingSlotHandle );
    }
    /* check if existing slot is of type PCR */
    else if (( stptiMemGet_Slot( FullExistingSlotHandle ))->Type ==
                                                 STPTI_SLOT_TYPE_PCR )
    {

        Error = stpti_Slot_ClearPCRSlot( FullExistingSlotHandle );
        if ( Error == ST_NO_ERROR )
        {
            /*
             * Set PCRReturnHandle of new slot to slot handle of the
             * existing slot in SLOT STRUCTURE
             */
            (stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle =
                                         FullExistingSlotHandle.word;
            Error = stptiHAL_SlotSetPCRReturn( FullSlotHandle );
            *PidNeedsToBeSet = TRUE;
        }
    }
    else
    {
        /*
         * if the pid is already collected & neither the current
         * or previous slot is a pcr slot then return an error.
         */
        Error = STPTI_ERROR_PID_ALREADY_COLLECTED;
    }

    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name :stpti_Slot_SetPid
  Description :
   Parameters :

    NOTES on handling and reporting of PCRs

   An element of the slot structure is the PCRreturn handle, this is
   the slot handle that is to be returned via the event handling mechanism
   if a PCR is found on the slot.

   If the PCR is not to be reported then it is set to a NULL handle. If
   the slot is not a PCR type but still needs to report any found on it then
   the the PCRreturn Handle will be set to the handle of the PCR slot that
   would have carried the PCR pid.

   This handles the situation where the PCR pid is the same as ( say ) a VIDEO
   pid. If the PCR is carried on an otherwise unused pid then the PCRreturn
   handle will be ther same as the slot handle.

   We therefore have a mechanism whereby PCR's may be collected from a
   subset of the pids carrying PCRs.

   The slothandle of the ONE slot that provides the PCRs for clock recovery
   must be passed to the clock recovery driver so that it can filter out
   the PCRs that it requires from all those reporting.

   This is the 'set-pid' algorithm.... see also 'clear pid' algorithm

   for ( all the slots on this device )
   {
        if ( existing pid == new pid )
        {
            if ( new pid is of PCR type )
            {
                add slot handle to existing slot's 'PCR return'
                get out
            }
            else if ( existing pid is of PCR type )
            {
                clear existing pid
                set new pid in new slot
                add existing slot as 'PCR return' in new structure
                get out
            }
            else
            {
                ERROR!!
                get out
            }
        }
    }
    if ( no matched pids found )
    {
        set new pid in new slot
    }

******************************************************************************/
ST_ErrorCode_t stpti_Slot_SetPid( FullHandle_t FullSlotHandle,
                                         STPTI_Pid_t Pid,
                                         STPTI_Pid_t MapPid )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#if !defined ( STPTI_BSL_SUPPORT )
    STPTI_Pid_t OldPid;
    STPTI_Slot_t ExistingSlot;
#if defined (STPTI_FRONTEND_HYBRID)
    FullHandle_t FullFrontendHandle;
    FullHandle_t FullSessionHandle;
    FullHandle_t CloneFullFrontendHandle;
    BOOL         FrontendRebuildPIDs = FALSE;

    FullFrontendHandle.word = STPTI_NullHandle();
    FullSessionHandle.word = STPTI_NullHandle();

    if ( STPTI_NullHandle() != (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle.word )
    {
        FullFrontendHandle.word = ( &stptiMemGet_List( (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle )->Handle )[0];
        /* Get the session handle */
        if ( STPTI_NullHandle() != (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle.word )
        {
            FullSessionHandle.word = ( &stptiMemGet_List( (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle )->Handle )[0];
        }
    }
#endif
    OldPid = ( stptiMemGet_Slot( FullSlotHandle ))->Pid;

#ifdef STTBX_PRINT
    STTBX_Print(( "\nstpti_Slot_SetPid H %x -> P %x", FullSlotHandle, Pid ));
    DumpState( FullSlotHandle, "SetPid - Pre", 1 );
#endif

    if (( stptiMemGet_Slot( FullSlotHandle ))->Type != STPTI_SLOT_TYPE_RAW )
    {
        /* PID mapping is only permitted for RAW slots */
        MapPid = STPTI_InvalidPid();
    }

    /* If the new Pid is different from the existing Pid on the Slot */
    if ( OldPid != Pid )
    {
        BOOL PidNeedsToBeSet = FALSE;
        /*
         * If the existing Pid is a valid one, then we need it cleared down,
         * use the existing mechanism
         */
        if ( OldPid != TC_INVALID_PID )
        {
            STTBX_Print(( "\nstpti_Slot_SetPid Calls ClearPid" ));
            Error = stpti_Slot_ClearPid( FullSlotHandle, TRUE );
#if defined (STPTI_FRONTEND_HYBRID)
            /* Check if linked to a frontend object of TSIN type */
            if ( (STPTI_NullHandle() != FullSessionHandle.word) && ((stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN) )
            {
                U32 CloneWildCardCount = 0;

                /* If OldPId was wildPID dec counter */
                if ( OldPid == STPTI_WildCardPid() )
                {
                    (stptiMemGet_Frontend( FullFrontendHandle ))->WildCardPidCount--;

                    CloneFullFrontendHandle = (stptiMemGet_Frontend( FullFrontendHandle ))->CloneFrontendHandle;
                    if (STPTI_NullHandle() == CloneFullFrontendHandle.word )
                    {
                        CloneWildCardCount = (stptiMemGet_Frontend( FullFrontendHandle ))->WildCardPidCount;
                    }

                    if ( (!(stptiMemGet_Frontend( FullFrontendHandle ))->WildCardPidCount) && (!CloneWildCardCount) )
                    {
                        FrontendRebuildPIDs = TRUE;
                    }
                }
            }
#endif
        }

        /* now we're ready to set the new Pid */

        /* check if the pid is being collected already */
        stpti_SlotQueryPid( FullSlotHandle, Pid, &ExistingSlot );

        if ( ExistingSlot != STPTI_NullHandle() )
        {
            Error = stpti_TransferPCRHandle( FullSlotHandle,
                                             ExistingSlot,
                                             &PidNeedsToBeSet);
        }
        else
        {
            /* There are no Slots currently looking at specified PID */
            if (( stptiMemGet_Slot( FullSlotHandle ))->Type == STPTI_SLOT_TYPE_PCR )
            {
                /* We are trying to set a pid on a STPTI_SLOT_TYPE_PCR */
                (stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle = FullSlotHandle.word;
            }
            else
            {
                (stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle = STPTI_NullHandle();
            }

            Error = stptiHAL_SlotSetPCRReturn( FullSlotHandle );
            PidNeedsToBeSet = TRUE;
        }

        if ( Error == ST_NO_ERROR )
        {
            if ( Pid == STPTI_NegativePid())
            {
                stptiMemGet_Device( FullSlotHandle )->NegativePIDUse =
                                                           FullSlotHandle.word;
            }

            if ( Pid == STPTI_WildCardPid())
            {
                stptiMemGet_Device( FullSlotHandle )->WildCardUse =
                                                           FullSlotHandle.word;
            }

            ( stptiMemGet_Slot( FullSlotHandle ))->Pid = Pid;

            if ( stptiMemGet_Device( FullSlotHandle )->DescramblerAssociationType ==
                STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_PIDS )
            {
                STPTI_Descrambler_t Descrambler;

                /*
                 * if there is a descrambler associated to this pid then make
                 * the association
                 */
                Error = stpti_DescramblerQueryPid( FullSlotHandle,
                                                   Pid,
                                                   &Descrambler );
                if ( Descrambler != STPTI_NullHandle())
                {
                    FullHandle_t DescramblerHandle;

                    DescramblerHandle.word = Descrambler;
                    Error = stpti_Descrambler_AssociateSlot( DescramblerHandle,
                                                             FullSlotHandle );
                }
            }
#ifndef STPTI_NO_INDEX_SUPPORT
            if ( stptiMemGet_Device( FullSlotHandle )->IndexAssociationType ==
                                 STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_PIDS )
            {
                STPTI_Index_t Index;

                /* if there is a index associated to this pid then make the association */
                Error = stpti_IndexQueryPid( FullSlotHandle, Pid, &Index );
                if ( Index != STPTI_NullHandle())
                {
                    FullHandle_t IndexHandle;

                    IndexHandle.word = Index;
                    Error = stpti_IndexAssociateSlot( IndexHandle, FullSlotHandle );
                }
            }
#endif
        }

        /* now all associations are done, so set the PID */
        if (( Error == ST_NO_ERROR ) && ( PidNeedsToBeSet == TRUE ))
        {
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
            Error = stptiHAL_SlotSetPid( FullSlotHandle, Pid, MapPid );
#if defined (STPTI_FRONTEND_HYBRID)
            if ( (STPTI_NullHandle() != FullSessionHandle.word) && ((stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN) )
            {
                /* If the OldPid was an STPTI_WildCardPid then re-sync as you need to rebuild the PID table */
                if ( FrontendRebuildPIDs == TRUE )
                {
                    stptiHAL_SyncFrontendPids(  FullFrontendHandle, FullSessionHandle );
                }
                else
                {
                    /* Update the Frontend PID filter */
                    Error = stptiHAL_FrontendSetPid ( FullSlotHandle, Pid );
                }
            }
#endif
#if !defined ( STPTI_BSL_SUPPORT )
        }
    }
    else /* ( OldPid == Pid ) */
    {
        /*
         * We aren't changing the Slot's Pid, we are just changing where the
         *  PCRs are sent IF the Slot is of type PCR
         */
        /* check if the pid is being collected on another slot */
        stpti_SlotQueryPid( FullSlotHandle, OldPid, &ExistingSlot );

        if ((( stptiMemGet_Slot( FullSlotHandle ))->Type == STPTI_SLOT_TYPE_PCR ) &&
            ( ExistingSlot == STPTI_NullHandle()))
        {
            ( stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle = FullSlotHandle.word;

            Error = stptiHAL_SlotSetPCRReturn( FullSlotHandle );
            Error = stptiHAL_SlotSetPid( FullSlotHandle, OldPid, MapPid );
        }
        else
        {
#ifdef STTBX_PRINT
            U16 type = ( stptiMemGet_Slot( FullSlotHandle ))->Type;
            U16 pcrtype = STPTI_SLOT_TYPE_PCR;

            STTBX_Print(( "ERROR - stpti_Slot_SetPid - PID already Collected - Type %d =?= %d  Exist: %x\n",
                          type, pcrtype, ExistingSlot ));
#endif
            Error = STPTI_ERROR_PID_ALREADY_COLLECTED;
        }
    }

#ifdef STTBX_PRINT
    DumpState( FullSlotHandle, "SetPid - Post", -1 );
#endif

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    return( Error );
}

/******************************************************************************
Function Name : stpti_Slot_ClearPid
  Description :
   Parameters :

    This is the 'clear-pid' algorithm.... see also 'set pid' algorithm

    // If the slot is not a PCR slot we must set the pid on any slot whose
    // handle is stored in the PCRReturnHandle
    if (( SlotHandle != PCRReturnHandle ) && ( PCRReturnHandle != NULL ))
    {
        setpid on 'PCRReturnHandle Slot
    }

    // if the slot is a PCR slot we must clear any other slot's PCRReturnHandle
    // that is the same as the PCRSlotHandle.
    else if (( SlotHandle == PCRReturnHandle ) && ( PCRReturnHandle != NULL ))
    {
        for ( all the slots on this device )
        {
            // if another slot is collecting the PCRs ( ie a video slot )
            if ( AllocatedSlots->PCRReturnHandle == SlotHandle )
            {
                set AllocatedSlots->PCRReturnHandle = NULL
            }
        }
    }
    clearPid   // on this slot only
    set PCRReturnHandle = NULL

    Set WindbackDMA to TRUE to make sure we don't get any half sections in
    any attached buffer; this will discard any partial sections.

******************************************************************************/
static ST_ErrorCode_t stpti_Slot_ClearPid( FullHandle_t FullSlotHandle, BOOL WindbackDMA )
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
#if defined (STPTI_FRONTEND_HYBRID)
    FullHandle_t FullFrontendHandle;
    FullHandle_t FullSessionHandle;
    BOOL         PidStillInUse = FALSE;
    
#endif
#if !defined ( STPTI_BSL_SUPPORT )
    STPTI_Pid_t OldPid = ( stptiMemGet_Slot( FullSlotHandle ))->Pid;

    
#ifdef STTBX_PRINT
    STTBX_Print(( "\nstpti_Slot_ClearPid H %x", FullSlotHandle ));
    DumpState( FullSlotHandle, "ClearPid - Pre", 1 );
#endif

#if defined (STPTI_FRONTEND_HYBRID)
    FullFrontendHandle.word = STPTI_NullHandle();
    FullSessionHandle.word = STPTI_NullHandle();

    if ( STPTI_NullHandle() != (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle.word )
    {
        FullFrontendHandle.word = ( &stptiMemGet_List( (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle )->Handle )[0];
        /* Get the session handle */
        if ( STPTI_NullHandle() != (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle.word )
        {
            FullSessionHandle.word = ( &stptiMemGet_List( (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle )->Handle )[0];
        }
    }
#endif
    if ( OldPid == STPTI_WildCardPid())
    {
        stptiMemGet_Device( FullSlotHandle )->WildCardUse = STPTI_NullHandle();
    }

    if ( OldPid == STPTI_NegativePid())
    {
        stptiMemGet_Device( FullSlotHandle )->NegativePIDUse = STPTI_NullHandle();
    }

    if ( OldPid != TC_INVALID_PID )
    {
        STPTI_Slot_t PCRReturnHandle;

        /* Get a safe copy of the PCR return handle, before we clear Slot */
        PCRReturnHandle = ( stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle;

        /*
         *  if the slot is a PCR slot we must clear any other slot's
         *  PCRReturnHandle that is the same as the PCRSlotHandle
         */
        if (( stptiMemGet_Slot( FullSlotHandle ))->Type == STPTI_SLOT_TYPE_PCR )
        {
            STPTI_Slot_t ExistingSlot;

            /* check if the pid is being collected already */
            stpti_SlotQueryPid( FullSlotHandle, OldPid, &ExistingSlot );

            if ( ExistingSlot != STPTI_NullHandle() )
            {
                FullHandle_t FullExistingSlotHandle;

                FullExistingSlotHandle.word = ExistingSlot;

                ( stptiMemGet_Slot( FullExistingSlotHandle ))->PCRReturnHandle =
                                                             STPTI_NullHandle();
#if defined (STPTI_FRONTEND_HYBRID)
                PidStillInUse = TRUE;
#endif
            }
        }

#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
        if ( ST_NO_ERROR != (Error = stptiHAL_SlotClearPid( FullSlotHandle, WindbackDMA )))
        {
            STTBX_Print(( "stptiHAL_SlotClearPid Error\n"));
        }
#if defined (STPTI_FRONTEND_HYBRID)
        else
        {
            /* Don't clear the pid in the stfe filter in the case of this is a PCR slot and the Pid is already in use */
            if ( FALSE == PidStillInUse )
            {
                if ( (STPTI_NullHandle() != (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle.word ) && ((stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN) )
                {
                    FullFrontendHandle.word = ( &stptiMemGet_List( (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle )->Handle )[0];
                    
                    if ( STPTI_NullHandle() != (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle.word )
                    {
                        /* Get the session handle */
                        FullSessionHandle.word = ( &stptiMemGet_List( (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle )->Handle )[0];
                        if ( (STPTI_NullHandle() != FullSessionHandle.word) && ((stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN) )
                        {
                            STTBX_Print(("stpti_Slot_ClearPid - Calling stptiHAL_FrontendClearPid\n"));
                            Error = stptiHAL_FrontendClearPid( FullSlotHandle, OldPid );
                        }
                    }
                }
            }
        }
#endif
        (stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle = STPTI_NullHandle();
        (stptiMemGet_Slot( FullSlotHandle ))->Pid = TC_INVALID_PID;
#if !defined ( STPTI_BSL_SUPPORT )
        if ( Error == ST_NO_ERROR )
        {
            if ( stptiMemGet_Device( FullSlotHandle )->DescramblerAssociationType ==
                                     STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_PIDS )
            {
                /* if there is a descrambler associated-clear the association */
                FullHandle_t DescramblerHandle;
                FullHandle_t DescramblerListHandle;

                DescramblerListHandle =
                    ( stptiMemGet_Slot( FullSlotHandle ))->DescramblerListHandle;

                if ( DescramblerListHandle.word != STPTI_NullHandle())
                {
                    DescramblerHandle.word =
                         ( &stptiMemGet_List( DescramblerListHandle )->Handle )[0];
                    if ( DescramblerHandle.word != STPTI_NullHandle())
                    {
                        Error = stpti_SlotDisassociateDescrambler( FullSlotHandle,
                                                                   DescramblerHandle,
                                                                   TRUE,
                                                                   FALSE );
                    }
                }
            }
#ifndef STPTI_NO_INDEX_SUPPORT
            if ( stptiMemGet_Device( FullSlotHandle )->IndexAssociationType ==
                                   STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_PIDS )
            {
                FullHandle_t IndexListHandle;

                /* if there is a Index associated then clear the association */
                IndexListHandle =
                           ( stptiMemGet_Slot( FullSlotHandle ))->IndexListHandle;

                if ( IndexListHandle.word != STPTI_NullHandle())
                {
                    FullHandle_t IndexHandle;

                    IndexHandle.word = ( &stptiMemGet_List( IndexListHandle )->Handle )[0];
                    if ( IndexHandle.word != STPTI_NullHandle())
                    {
                        Error = stpti_SlotDisassociateIndex( FullSlotHandle,
                                                             IndexHandle,
                                                             TRUE );
                    }
                }
            }
#endif
        }

        /*
         * if we had a slot that was not a PCR slot and it had a pcr return
         * handle then we must reset this pid as we are not clearing this pid
         */
        if ((( stptiMemGet_Slot( FullSlotHandle ))->Type != STPTI_SLOT_TYPE_PCR ) &&
            ( PCRReturnHandle != STPTI_NullHandle()))
        {
            FullHandle_t FullPCRReturnHandle;

            FullPCRReturnHandle.word = PCRReturnHandle;
            STTBX_Print(( "Clear Pid calls SetPid(FH %x  PCRH %x) Pid %x\n",
                          (U32)FullSlotHandle.word, (U32)FullPCRReturnHandle.word, (U32)OldPid ));
#if defined (STPTI_FRONTEND_HYBRID)
            if ( ST_NO_ERROR == (Error = stpti_Slot_SetPid( FullPCRReturnHandle, OldPid, OldPid ) ))
            {
                if ( (STPTI_NullHandle() != (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle.word ) && ((stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN) )
                {
                    FullFrontendHandle.word = ( &stptiMemGet_List( (stptiMemGet_Device(FullSlotHandle))->FrontendListHandle )->Handle )[0];
                    
                    if ( STPTI_NullHandle() != (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle.word )
                    {
                        /* Get the session handle */
                        FullSessionHandle.word = ( &stptiMemGet_List( (stptiMemGet_Frontend( FullFrontendHandle ))->SessionListHandle )->Handle )[0];
                        if ( (STPTI_NullHandle() != FullSessionHandle.word) && ((stptiMemGet_Frontend( FullFrontendHandle ))->Type == STPTI_FRONTEND_TYPE_TSIN) )
                        {
                            STTBX_Print(("stpti_Slot_ClearPid - Calling stptiHAL_FrontendSetPid\n"));
                            Error = stptiHAL_FrontendSetPid( FullSlotHandle, OldPid );
                        }
                    }
                }
            }
#else
            Error = stpti_Slot_SetPid( FullPCRReturnHandle, OldPid, OldPid );
#endif

        }
    }

#ifdef STTBX_PRINT
    DumpState( FullSlotHandle, "ClearPid - Post", -1 );
#endif
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    return( Error );
}

#if !defined ( STPTI_BSL_SUPPORT )

/******************************************************************************
Function Name : stpti_Slot_ClearPCRSlot
  Description : Like clear Pid but does not clear the Pid from the slot
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_Slot_ClearPCRSlot( FullHandle_t FullSlotHandle )
{
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STPTI_Pid_t     OldPid;


    
    OldPid = ( stptiMemGet_Slot( FullSlotHandle ))->Pid;
    if ( OldPid == STPTI_WildCardPid())
    {
        stptiMemGet_Device( FullSlotHandle )->WildCardUse = STPTI_NullHandle();
    }

    if ( OldPid != TC_INVALID_PID )
    {
        if (( stptiMemGet_Slot( FullSlotHandle ))->Type == STPTI_SLOT_TYPE_PCR )
        {
            Error = stptiHAL_SlotClearPid( FullSlotHandle, TRUE );
            ( stptiMemGet_Slot( FullSlotHandle ))->PCRReturnHandle = STPTI_NullHandle();
        }
    }

    return( Error );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name :STPTI_SlotClearPid
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotClearPid( STPTI_Slot_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    
    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_SlotClearPid( FullSlotHandle );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error == ST_NO_ERROR )
    {
        STTBX_Print(("\n**ClearPid request**  Handle %x\n", (U32)FullSlotHandle.word ));
        Error = stpti_Slot_ClearPid( FullSlotHandle, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name : STPTI_SlotSetPid
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotSetPid( STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    FullSlotHandle.word = SlotHandle;
    
    
    STOS_SemaphoreWait( stpti_MemoryLock );

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_SlotSetPid( FullSlotHandle, Pid );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if ( Error == ST_NO_ERROR )
    {
        STTBX_Print(("\n**SetPid request**  Handle %x -> Pid %x\n", FullSlotHandle.word, Pid ));
        Error = stpti_Slot_SetPid( FullSlotHandle, Pid, Pid );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    
    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : STPTI_SlotUpdatePid
  Description : This function allows the user to change the PID on a slot.
                Does the same as ClearPid followed by SetPid except it doesn't
                windback the DMA, meaning that you could get partial sections,
                in any attached buffer.  This feature was request by customer
                in GNBvd40091.
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_SlotUpdatePid( STPTI_Slot_t SlotHandle, STPTI_Pid_t Pid )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotClearPid( FullSlotHandle );
    if ( Error == ST_NO_ERROR )
    {
        STTBX_Print(("\n**ClearPid request**  Handle %x\n", FullSlotHandle.word ));
        Error = stpti_Slot_ClearPid( FullSlotHandle, FALSE );
    }

    Error = stptiValidate_SlotSetPid( FullSlotHandle, Pid );
    if ( Error == ST_NO_ERROR )
    {
        STTBX_Print(("\n**SetPid request**  Handle %x -> Pid %x\n", FullSlotHandle.word, Pid ));
        Error = stpti_Slot_SetPid( FullSlotHandle, Pid, Pid );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}



/******************************************************************************
Function Name : STPTI_SlotSetPidAndRemap
  Description : Based on STPTI_SlotSetPid, this call will cause the TP PID
                    value to be changed at the output. Only for DVB and RAW slots.

                This call does the following:

                1) Check that the InputPid  to be used by stpti_Slot_SetPid()  is valid.
                2) Check that the OutputPid to be used by stpti_SlotRemapPid() is valid.
                3) If InputPID and OutputPID's are vaild, then call
                    - stpti_Slot_SetPid( FullSlotHandle, InputPid )

   Parameters : InputPID  - Incoming PID in TP Header
                OutputPID - PID to replace incoming PID at the outputed TP header
******************************************************************************/
ST_ErrorCode_t STPTI_SlotSetPidAndRemap( STPTI_Slot_t SlotHandle, STPTI_Pid_t InputPid, STPTI_Pid_t OutputPid )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    FullSlotHandle.word = SlotHandle;
    STOS_SemaphoreWait( stpti_MemoryLock );

    Error = stptiValidate_SlotSetPid( FullSlotHandle, InputPid );
    if ( Error == ST_NO_ERROR )
    {
        STTBX_Print(("\n**SetPid request**  Handle %x -> Pid %x[%x]\n", (U32)FullSlotHandle.word, InputPid, OutputPid ));

        Error = stpti_Slot_SetPid( FullSlotHandle, InputPid, OutputPid );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}

/******************************************************************************
Function Name : STPTI_Term
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_Term( ST_DeviceName_t DeviceName,
                           const STPTI_TermParams_t * TermParams )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;
    
    
    /* Escape if there is no MemoryLock Semaphore - means we have not initialised any PTI sessions */
    if( stpti_MemoryLock == NULL ) return( STPTI_ERROR_NOT_INITIALISED );

    STOS_SemaphoreWait( stpti_MemoryLock );

    Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, TRUE );
    
    if ( Error == ST_NO_ERROR )
    {
#ifdef ST_OSLINUX    
        stpti_LinuxEventQueueTerm( DeviceHandle );
#endif
#if !defined ( STPTI_BSL_SUPPORT )
        Error = stpti_DeallocateDevice( DeviceHandle, TermParams->ForceTermination );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    
    return( Error );
}

#ifdef ST_OSLINUX
/******************************************************************************
Function Name : STPTI_TermAll
  Description : Terminate all devices. New addition for Linux.
                Good for tidying up after errors.
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_TermAll()
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    /* If MemoryLock not created then hasn't been initialised. */
    if( stpti_MemoryLock )
    {
        U32 Device;
        int rerun;

        STOS_SemaphoreWait( stpti_MemoryLock );

        do
        {
            rerun = 0;
            for (Device = 0; Device < stpti_GetNumberOfExistingDevices(); ++Device)
            {
                /*FullHandle_t DevHandle;                                    */
                /*DevHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;   */
                Device_t *Device_p = stpti_GetDevice( Device );

                if (NULL != Device_p)
                {
                    stpti_GetDeviceHandleFromDeviceName( (char*)Device_p->DeviceName, &DeviceHandle, TRUE );

                    stpti_LinuxEventQueueTerm( DeviceHandle );

                    stpti_DeallocateDevice( DeviceHandle, TRUE );

                    rerun = 1;
                }
            }
        }
        while (rerun);

        STOS_SemaphoreSignal( stpti_MemoryLock );
    }
    else
    {
        Error = STPTI_ERROR_NOT_INITIALISED;
    }

    return( Error );
}
#endif

/******************************************************************************
Function Name : STPTI_SlotUnLink
  Description : Disassociate a main buffer from a slot
   Parameters :

******************************************************************************/
ST_ErrorCode_t STPTI_SlotUnLink( STPTI_Slot_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;
    FullHandle_t BufferListHandle;
    U32 CDFifoAddr;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotUnLink( FullSlotHandle );
    if ( Error == ST_NO_ERROR )
    {
        BufferListHandle = ( stptiMemGet_Slot( FullSlotHandle ))->BufferListHandle;
        CDFifoAddr = ( stptiMemGet_Slot( FullSlotHandle ))->CDFifoAddr;

        if ( BufferListHandle.word == STPTI_NullHandle() && CDFifoAddr == 0 )
        {
            Error = STPTI_ERROR_SLOT_NOT_ASSOCIATED;
        }
        else
        {
            if ( BufferListHandle.word != STPTI_NullHandle())
            {
                if ( stptiMemGet_List( BufferListHandle )->UsedHandles == 0 )
                {
                    Error = STPTI_ERROR_INVALID_SLOT_HANDLE;
                }
                else
                {
                    U16 Indx;
                    U16 MaxIndx = stptiMemGet_List(BufferListHandle)->MaxHandles;

                    for (Indx = 0; Indx < MaxIndx; ++Indx)
                    {
                        FullHandle_t BufferHandle;

                        BufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[Indx];
                        if (BufferHandle.word != STPTI_NullHandle())
                        {
                            if (FALSE == stptiMemGet_Buffer(BufferHandle)->IsRecordBuffer)
                            {
                                Error = stpti_SlotDisassociateBuffer(FullSlotHandle, BufferHandle, FALSE);
                                break;
                            }
                        }
                    }
                    if (Indx == MaxIndx)
                    {
                        Error = STPTI_ERROR_INVALID_SLOT_HANDLE;
                    }
                }
            }

            if ( CDFifoAddr != 0 )
            {
                Error = stptiHAL_SlotUnLinkDirect( FullSlotHandle );

                if ( Error == ST_NO_ERROR )
                {
                    ( stptiMemGet_Slot( FullSlotHandle ))->CDFifoAddr = ( U32 ) NULL;
                }
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name :STPTI_SlotUnLinkRecordBuffer
  Description : Attempt to unlink/dissasociate the slot from the session
                `Watch & Record' Buffer
   Parameters : SlotHandle
 
               For watch and record the Slot can also be associated to a record
               buffer on a per session basis so maximum buffer associations = 2
              
******************************************************************************/
ST_ErrorCode_t STPTI_SlotUnLinkRecordBuffer( STPTI_Slot_t SlotHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;
    FullHandle_t FullSlotHandle;
    FullHandle_t BufferListHandle;
    U32 CDFifoAddr;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullSlotHandle.word = SlotHandle;

    Error = stptiValidate_SlotUnLink( FullSlotHandle );
    if ( Error == ST_NO_ERROR )
    {
        BufferListHandle = ( stptiMemGet_Slot( FullSlotHandle ))->BufferListHandle;
        CDFifoAddr = ( stptiMemGet_Slot( FullSlotHandle ))->CDFifoAddr;

        if ( BufferListHandle.word == STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_SLOT_NOT_ASSOCIATED;
        }
        else
        {
            U16 Index;
            U16 MaxIndex = stptiMemGet_List(BufferListHandle)->MaxHandles;

            for ( Index=0; Index < MaxIndex; Index++ )
            {
                FullBufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[Index];
                if (FullBufferHandle.word != STPTI_NullHandle()) 
                {
                    if(TRUE == stptiMemGet_Buffer(FullBufferHandle)->IsRecordBuffer)
                    {
                        Error = stpti_SlotDisassociateBuffer( FullSlotHandle, FullBufferHandle, FALSE );
                        break;
                    }
                }
            }
            if (Index == MaxIndex)
            {
                Error = STPTI_ERROR_INVALID_SLOT_HANDLE;
            }
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name :STPTI_EnableErrorEvent
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_EnableErrorEvent( ST_DeviceName_t DeviceName,
                                       STPTI_Event_t EventName )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    
    STOS_SemaphoreWait( stpti_MemoryLock );

    Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, TRUE );
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_ErrorEventModifyState( DeviceHandle, EventName, TRUE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


/******************************************************************************
Function Name :STPTI_DisableErrorEvent
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_DisableErrorEvent( ST_DeviceName_t DeviceName,
                                        STPTI_Event_t EventName )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t DeviceHandle;

    
    STOS_SemaphoreWait( stpti_MemoryLock );

    Error = stpti_GetDeviceHandleFromDeviceName( DeviceName,
                                                 &DeviceHandle,
                                                 TRUE );

    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_ErrorEventModifyState( DeviceHandle,
                                                EventName,
                                                FALSE );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );
    return( Error );
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : STPTI_BufferUnLink
  Description :
   Parameters :
         Note : Moved here from pti_extended.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t STPTI_BufferUnLink( STPTI_Buffer_t BufferHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    Error = stptiValidate_BufferUnLink( FullBufferHandle );
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_BufferUnLink( FullBufferHandle );
        if ( Error == ST_NO_ERROR )
        {
            ( stptiMemGet_Buffer( FullBufferHandle ))->CDFifoAddr = ( U32 ) NULL;
        }
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}


/******************************************************************************
Function Name : STPTI_BufferReadTSPacket
  Description :
   Parameters :
         Note : Moved here from pti_extended.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t STPTI_BufferReadTSPacket( STPTI_Buffer_t BufferHandle,
                                         U8 *Destination0_p,
                                         U32 DestinationSize0,
                                         U8 *Destination1_p,
                                         U32 DestinationSize1,
                                         U32 *DataSize_p,
                                         STPTI_Copy_t DmaOrMemcpy )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait( stpti_MemoryLock );
    FullBufferHandle.word = BufferHandle;

    if ( NULL != DataSize_p )
        *DataSize_p = 0;

    Error = stptiValidate_BufferReadTSPacket( FullBufferHandle,
                                              Destination0_p,
                                              DestinationSize0,
                                              Destination1_p,
                                              DestinationSize1,
                                              DataSize_p,
                                              DmaOrMemcpy );
    if ( Error == ST_NO_ERROR )
    {
        Error = stptiHAL_BufferReadTSPacket( FullBufferHandle,
                                             Destination0_p,
                                             DestinationSize0,
                                             Destination1_p,
                                             DestinationSize1,
                                             DataSize_p,
                                             DmaOrMemcpy );
    }

    STOS_SemaphoreSignal( stpti_MemoryLock );

    return( Error );
}


/******************************************************************************
Function Name : stpti_DescramblerQueryPid
  Description :
   Parameters :
         Note : Moved here from pti_extended.c following DDTS18257
******************************************************************************/
ST_ErrorCode_t stpti_DescramblerQueryPid( FullHandle_t DeviceHandle,
                                          STPTI_Pid_t Pid,
                                          STPTI_Descrambler_t *Descrambler_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SessionHandle;
    FullHandle_t DescramblerListHandle;
    FullHandle_t DescramblerHandle;
    U16 MaxSessions;
    U16 session = 0;
    U16 MaxDescrambler;
    U16 descrambler = 0;

    *Descrambler_p = STPTI_NullHandle();

    MaxSessions = stptiMemGet_Device( DeviceHandle )->MemCtl.MaxHandles;

    /* For all sessions */
    while (( session < MaxSessions ) &&
           ( *Descrambler_p == STPTI_NullHandle()))
    {
        SessionHandle = stptiMemGet_Device( DeviceHandle )->MemCtl.Handle_p[session].Hndl_u;

        if ( SessionHandle.word != STPTI_NullHandle())
        {
            DescramblerListHandle =
               stptiMemGet_Session(SessionHandle)->AllocatedList[OBJECT_TYPE_DESCRAMBLER];

            if ( DescramblerListHandle.word != STPTI_NullHandle())
            {
                /* for all descramblers */
                descrambler = 0;
                MaxDescrambler = stptiMemGet_List( DescramblerListHandle )->MaxHandles;

                while (( descrambler < MaxDescrambler ) &&
                       ( *Descrambler_p == STPTI_NullHandle()))
                {
                    DescramblerHandle.word =
                       (&stptiMemGet_List(DescramblerListHandle)->Handle)[descrambler];
                    if (( DescramblerHandle.word != STPTI_NullHandle() ) &&
                        (( stptiMemGet_Descrambler( DescramblerHandle ))->AssociatedPid == Pid ))
                    {
                        *Descrambler_p = DescramblerHandle.word;
                    }
                    ++descrambler;
                }
            }
        }
        ++session;
    }

    return( Error );
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : STPTI_SetDiscardParams
  Description :
   Parameters : DeviceName, and the number of bytes to be discarded.
******************************************************************************/
ST_ErrorCode_t STPTI_SetDiscardParams(ST_DeviceName_t Device, U8 NumberOfDiscardBytes)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullDeviceHandle;


    
    STOS_SemaphoreWait(stpti_MemoryLock);

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_SetDiscardParams( Device, NumberOfDiscardBytes );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if( Error == ST_NO_ERROR )
    {
        Error = stpti_GetDeviceHandleFromDeviceName(Device, &FullDeviceHandle, TRUE);
        if( Error == ST_NO_ERROR )
        {
            Error = stptiHAL_SetDiscardParams( FullDeviceHandle, NumberOfDiscardBytes );

        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return Error;
}

/******************************************************************************
Function Name : STPTI_BufferTestForData
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t STPTI_BufferTestForData(STPTI_Buffer_t BufferHandle, U32 *BytesInBuffer)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;
    U32 Count;

    
    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;

#if !defined ( STPTI_BSL_SUPPORT )
    Error = stptiValidate_BufferTestForData( FullBufferHandle, BytesInBuffer);
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if (Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferTestForData(FullBufferHandle, &Count);
        if (Error == ST_NO_ERROR)
        {
             *BytesInBuffer = Count;
        }
        else
        {
            *BytesInBuffer = 0;
        }
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);
    return( Error );
}


/* --- NOTE following helper functions are common to PTI1, 3 and 4 --- */

/******************************************************************************
Function Name :stpti_BufferSizeAdjust
  Description :
   Parameters :
******************************************************************************/
size_t stpti_BufferSizeAdjust( size_t Size )
{
    
    return (( Size + STPTI_BUFFER_SIZE_MULTIPLE - 1 ) &
                                       ( ~( STPTI_BUFFER_SIZE_MULTIPLE - 1 )));
}


/******************************************************************************
Function Name :stpti_BufferBaseAdjust
  Description :
   Parameters :
******************************************************************************/

void *stpti_BufferBaseAdjust( void *Base_p )
{
    return ( void * ) ((( U32 ) Base_p + STPTI_BUFFER_ALIGN_MULTIPLE - 1 ) &
                                     ( ~( STPTI_BUFFER_ALIGN_MULTIPLE - 1 )));
}

#if !defined ( STPTI_BSL_SUPPORT )
#ifdef STTBX_PRINT
/* NB Indent Change requires singlethreaded calls to work correctly */
static void DumpState( FullHandle_t NewSlotHandle, const char *label, int indentChange )
{
    U16 MaxSessions = stptiMemGet_Device( NewSlotHandle )->MemCtl.MaxHandles;
    U16 SessionIndex = 0;
    FullHandle_t SessionHandle;
    static int indent = 0;
    int loop = 0;

    if (indentChange > 0)
        indent += indentChange;

    STTBX_Print(( "\n" ));
    for (loop = 0; loop < indent; ++loop) {STTBX_Print(("    "));}
    STTBX_Print(( "vvvvvvvvvv \"%s\" %x\n", label, NewSlotHandle ));

    while ( SessionIndex < MaxSessions )
    {
        SessionHandle =
        stptiMemGet_Device( NewSlotHandle )->MemCtl.Handle_p[SessionIndex].Hndl_u;

        /* Is Session handle valid? */
        if ( SessionHandle.word != STPTI_NullHandle())
        {
            FullHandle_t SlotListHandle =
               stptiMemGet_Session( SessionHandle )->AllocatedList[OBJECT_TYPE_SLOT];

            /* Check whether the list of slots' handle is valid */
            if ( SlotListHandle.word != STPTI_NullHandle())
            {
                FullHandle_t SlotHandle;
                U16 MaxSlots = stptiMemGet_List( SlotListHandle )->MaxHandles;
                U16 SlotIndex = 0;
                while ( SlotIndex < MaxSlots )
                {
                    SlotHandle.word =
                            ( &stptiMemGet_List( SlotListHandle )->Handle )[SlotIndex];

                    if ((SlotHandle.word != STPTI_NullHandle()) &&
                        (( stptiMemGet_Slot( SlotHandle ))->Pid != 0xE000))
                    {
                        for (loop = 0; loop < indent; ++loop) {STTBX_Print(("    "));}
                        if (( stptiMemGet_Slot( SlotHandle ))->PCRReturnHandle != STPTI_NullHandle() )
                        {
                            STTBX_Print(("S%x H%x P%04x Ty%2d  (PCRH %x)\n",
                                  SessionIndex,
                                  SlotHandle.word,
                                  ( stptiMemGet_Slot( SlotHandle ))->Pid,
                                  ( stptiMemGet_Slot( SlotHandle ))->Type,
                                    ( stptiMemGet_Slot( SlotHandle ))->PCRReturnHandle ));
                        }
                        else
                        {
                            STTBX_Print(("S%x H%x P%04x Ty%2d\n",
                                  SessionIndex,
                                  SlotHandle.word,
                                  ( stptiMemGet_Slot( SlotHandle ))->Pid,
                                  ( stptiMemGet_Slot( SlotHandle ))->Type));
                        }
                    }
                    ++SlotIndex;
                } /* SlotIndex is valid and we haven't found a match */
            } /* Slot list handle was valid */
        } /* Slot is allocated to a Session */
        ++SessionIndex;
    } /* While valid session, and not found match */
    for (loop = 0; loop < indent; ++loop) {STTBX_Print(("    "));}
    STTBX_Print(( "^^^^^^^^^^ %s\n", label ));

    if (indentChange < 0)
        indent += indentChange;
}
#endif
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/* --- EOF --- */
