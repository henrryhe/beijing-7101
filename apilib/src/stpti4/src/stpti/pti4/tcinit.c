/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005, 2006.  All rights reserved.

   File Name: tcinit.c
 Description: TC initalization functions, used by basic.c:InitializeDevice()

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #endif defined(ST_OSLINUX) */

#if defined(ST_OSLINUX)
#include <linux/dma-mapping.h> /* dma_alloc_coherent etc. */
#endif

#include "stddefs.h"
#include "stdevice.h"
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #if !defined(ST_OSLINUX) */
#include "stpti.h"

#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"


/* ------------------------------------------------------------------------- */

void stptiHelper_TCInit_Stop( TCDevice_t *TC_Device )
{
    /* Ensure TC isn't running before messing with anything ! */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFFIFOEnable, 0x00 );/* Stop the IIF */
    STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 0x00 );       /* Stop the TC  */

    /* --- Perform a software reset --- */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAPTI3Prog, 0x01 );  /* PTI3 Mode   */
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAFlush, 0x01 );     /* Flush DMA 0 */

    /* For PTI4SL, if we do not enable the DMA here, the flush never occurs... */
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAEnable, 0x01 );    /* Enable the DMA */

    while (STSYS_ReadRegDev32LE( (void*)&TC_Device->DMAFlush)&0X01)

#if defined(ST_OSLINUX)
    udelay(100*64); /* 6400us */
#else
    task_delay((100*TICKS_PER_SECOND)/15625);
#endif /* #endif defined(ST_OSLINUX) */
      

    STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 0x08 );       /* Full Reset the TC  */

#if defined(ST_OSLINUX)
    udelay(200*64); /* 12800us */               /* Wait */
#else
    task_delay((200*TICKS_PER_SECOND)/15625);
#endif /* #if defined(ST_OSLINUX) */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 0x00 );       /* Finish Full Reset the TC  */

#if defined(ST_OSLINUX)
    udelay(10*64); /* 640us */
#else
    task_delay((10*TICKS_PER_SECOND)/15625);
#endif /* #endif defined(ST_OSLINUX) */

}
/* ------------------------------------------------------------------------- */

void stptiHelper_TCInit_Start( TCDevice_t *TC_Device )
{
    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFFIFOEnable, 1 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 2 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 1 );
}


/* ------------------------------------------------------------------------- */
void stptiHelper_TCInit_Hardware( FullHandle_t DeviceHandle )
{
    Device_t *Device_p = stptiMemGet_Device( DeviceHandle );

    TCDevice_t           *TC_Device     = (TCDevice_t *)  Device_p->TCDeviceAddress_p;
    U16 IIFSyncPeriod = DVB_TS_PACKET_LENGTH;    /* default is 188 for DVB */
    U16 TagBytes = 0;           /* default is TSMERGER in bypass mode (or no TSMERGER) */

    /* --- Initialize TC registers --- */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->TCMode, 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntAck0, 0xffff );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntAck1, 0xffff );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntAck2, 0xffff );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntAck3, 0xffff );

    /* disable all interrupts */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntEnable0, 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntEnable1, 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntEnable2, 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->PTIIntEnable3, 0 );

    /* Initialise various registers */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->STCTimer0, 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->STCTimer1, 0 );

    /* Initialise DMA Registers */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAEnable  , 0 );             /* Disable DMAs */
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAPTI3Prog, 1 );             /* PTI3 Mode */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Base   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Top    , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Write  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Read   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Setup  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Holdoff, (1 | (1 << 16)) );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA0Status , 0 );

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1Base   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1Top    , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1Write  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1Read   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1Setup  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1Holdoff, (1 | (1 << 16)) );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA1CDAddr , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMASecStart, 0 );

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2Base   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2Top    , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2Write  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2Read   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2Setup  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2Holdoff, (1 | (1 << 16)) );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA2CDAddr , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAFlush   , 0 );

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3Base   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3Top    , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3Write  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3Read   , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3Setup  , 0 );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3Holdoff, (1 | (1 << 16)) );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMA3CDAddr , 0 );

    STSYS_WriteRegDev32LE( (void*)&TC_Device->DMAEnable, 0xf );             /* Enable DMAs */

    /* Initialise IIF Registers */

    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFFIFOEnable, 0 );

    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFAltLatency, Device_p->AlternateOutputLatency );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFSyncLock  , Device_p->SyncLock );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFSyncDrop  , Device_p->SyncDrop );


    if ( Device_p->StreamID != STPTI_STREAM_ID_NOTAGS )
    {
        STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFSyncConfig, IIF_SYNC_CONFIG_USE_SOP );
#if defined(STPTI_FRONTEND_HYBRID)
        TagBytes = 8;
#else
        TagBytes = 6;
#endif    
    }

    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFSyncPeriod, IIFSyncPeriod + TagBytes );
    STSYS_WriteRegDev32LE( (void*)&TC_Device->IIFCAMode, 1 );
}

/* ------------------------------------------------------------------------- */


void stptiHelper_TCInit_PidSearchEngine( TCDevice_t *TC_Device, STPTI_TCParameters_t *TC_Params_p )
{
    int i;  /* Initialize PID search engine look-up table */


    for (i = 0; i < TC_Params_p->TC_NumberSlots; i++)
    {
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(Addr_p)
#endif
        volatile U16 *Addr_p = (volatile U16 *) TC_Params_p->TC_LookupTableStart;

        PutTCData(&Addr_p[i], TC_INVALID_PID);
    }
}


/* ------------------------------------------------------------------------- */


void stptiHelper_TCInit_MainInfo( TCDevice_t *TC_Device, STPTI_TCParameters_t *TC_Params_p )
{
    int i;  /* Initialize main info structures */


    for (i = 0; i < TC_Params_p->TC_NumberSlots; i++)
    {
        TCMainInfo_t *MainInfo = &((TCMainInfo_t *) TC_Params_p->TC_MainInfoStart)[i];
        STSYS_WriteTCReg16LE((void*)&MainInfo->SlotState,          0);
        STSYS_WriteTCReg16LE((void*)&MainInfo->SlotMode,           0);
        STSYS_WriteTCReg16LE((void*)&MainInfo->DescramblerKeys_p,  TC_INVALID_LINK);
        STSYS_WriteTCReg16LE((void*)&MainInfo->DMACtrl_indices,    0xFFFF);
        STSYS_WriteTCReg16LE((void*)&MainInfo->StartCodeIndexing_p,0);
        STSYS_WriteTCReg16LE((void*)&MainInfo->SectionPesFilter_p, TC_INVALID_LINK);
        STSYS_WriteTCReg16LE((void*)&MainInfo->RemainingPESLength, 0);
        STSYS_WriteTCReg16LE((void*)&MainInfo->PacketCount,        0);
        STSYS_WriteTCReg16LE((void*)&MainInfo->SlotState,          0);
        STSYS_WriteTCReg16LE((void*)&MainInfo->SlotMode,           0);

    }

}


/* ------------------------------------------------------------------------- */


void stptiHelper_TCInit_GlobalInfo( STPTI_TCParameters_t *TC_Params_p )
{
    TCGlobalInfo_t *TCGlobalInfo = (TCGlobalInfo_t *)TC_Params_p->TC_GlobalDataStart;

    /* Set the scratch area so TC can dump DMA0 data (in cdfifo-ish mode). Be paranoid
     and set the buffer to a DVB packet plus alignment/guard bytes */
    static U8 DmaFifoFlushingTarget[DVB_TS_PACKET_LENGTH + DMAScratchAreaSize];

    STTBX_Print(("Globalscratch %x\n",(int)&TCGlobalInfo->GlobalScratch));
/*    TCGlobalInfo->GlobalInterruptMask0 |= STATUS_FLAGS_PACKET_SIGNAL;*/   /* Now done in stptiHelper_TCInit_SessionInfo() */
    STSYS_WriteRegDev32LE( (void*)&TCGlobalInfo->GlobalScratch, (((U32)(DmaFifoFlushingTarget))+15)&(~0x0f) );
  /* Set the scratch area (see: GNBvd18810) */
    STTBX_Print(("Globalscratch 0x%08x 0x%08x\n",TCGlobalInfo->GlobalScratch,(((U32)(DmaFifoFlushingTarget))+15)&(~0x0f)));

#if defined(STPTI_SECTION_FILTER_TIMEOUT)
    STSYS_WriteTCReg16LE( (void*)&TCGlobalInfo->GlobalSFTimeout, STPTI_SECTION_FILTER_TIMEOUT );
#else
    STSYS_WriteTCReg16LE( (void*)&TCGlobalInfo->GlobalSFTimeout, 1429 );            /* default is 1429 iterations * 7 tc cycles = 10003 tc cycles */
#endif    

}


/* ------------------------------------------------------------------------- */


void stptiHelper_TCInit_SessionInfo( Device_t *TC_Device, STPTI_TCParameters_t *TC_Params_p )
{
    int session;  /* Initialize session info structures */

    for (session = 0; session < TC_Params_p->TC_NumberOfSessions; session++)
    {
        TCSessionInfo_t *SessionInfo_p = &((TCSessionInfo_t *) TC_Params_p->TC_SessionDataStart)[session];

        STSYS_WriteTCReg16LE((void*)&SessionInfo_p->SessionTSmergerTag,SESSION_NOT_ALLOCATED);

        /* Set SlotInterrupt in the Interrupt Mask */
        STSYS_SetTCMask16LE((void*)&SessionInfo_p->SessionInterruptMask0,STATUS_FLAGS_PACKET_SIGNAL);
        STSYS_SetTCMask16LE((void*)&SessionInfo_p->SessionInterruptMask0,STATUS_FLAGS_PACKET_SIGNAL_RECORD_BUFFER);
    }

}


/* ------------------------------------------------------------------------- */


void stptiHelper_TCInit_SessionInfoModeFlags( FullHandle_t DeviceHandle )
{
    Device_t             *Device_p      = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p = &Device_p->TCPrivateData;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCSessionInfo_t      *SessionInfo_p = &((TCSessionInfo_t *) TC_Params_p->TC_SessionDataStart)[Device_p->Session];
    
    /* Set DiscardSynchByte bit in the global data structure if STPTI has been initialised with this option */
    /* See DDTS GNBvd08680 PJW */

    if ( (Device_p->TCCodes == STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB) 
       )
    {
        /* DVB has a 1 byte header (the sync byte) */

        STSYS_SetTCMask16LE ((void*)&SessionInfo_p->SessionModeFlags,(TC_SESSION_MODE_FLAGS_DISCARD_SYNC_BYTE));
        STSYS_WriteTCReg16LE((void*)&SessionInfo_p->SessionDiscardParams, 1 );
        Device_p->DiscardBytes = 1;
    }
    else if (Device_p->DiscardSyncByte == TRUE)
    {
        STSYS_SetTCMask16LE ((void*)&SessionInfo_p->SessionModeFlags,(TC_SESSION_MODE_FLAGS_DISCARD_SYNC_BYTE));
        STSYS_WriteTCReg16LE((void*)&SessionInfo_p->SessionDiscardParams, 1 );
        Device_p->DiscardBytes = 1;
    }
    else
    {
        STSYS_ClearTCMask16LE((void*)&SessionInfo_p->SessionModeFlags,(TC_SESSION_MODE_FLAGS_DISCARD_SYNC_BYTE));
        STSYS_WriteTCReg16LE ((void*)&SessionInfo_p->SessionDiscardParams, 0 );
        Device_p->DiscardBytes = 0;
    }

#if defined(STPTI_CHANGE_SYNC_SUPPORT)
    {
        TCGlobalInfo_t       *TC_Global_p   = TcHal_GetGlobalInfo( TC_Params_p );

        /* Enable the TC to output a changes Sync Byte  */
        STSYS_WriteTCReg16LE((void*)&TC_Global_p->GlobalSwts_Params, STPTI_STREAM_ID_SWTS0);
        STSYS_SetTCMask16LE ((void*)&TC_Global_p->GlobalSwts_Params, TC_GLOBAL_SWTS_PARAMS_CHANGE_SYNC_BYTE_ENABLE_SET );
    }
#endif
    
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHelper_TCInit_InterruptDMA( FullHandle_t DeviceHandle ) /* Initialise 'Interrupt' DMA structure */
{
    Device_t *Device_p = stptiMemGet_Device( DeviceHandle );

    ST_Partition_t         *Partition_p            =  Device_p->NonCachedPartition_p;
    
    TCPrivateData_t        *PrivateData_p          = &Device_p->TCPrivateData;
    STPTI_TCParameters_t   *TC_Params_p            =   (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    TCInterruptDMAConfig_t *TCInterruptDMAConfig_p = (TCInterruptDMAConfig_t *)  TC_Params_p->TC_InterruptDMAConfigStart;
    U8 *base_p;
    U8 *top_p;
    size_t DMABufferSize = 0;

    /* See if we are using cached partitions */
    if( Partition_p == NULL )
    {
        Partition_p =  Device_p->DriverPartition_p;
    }
    
    /* Adjust the buffer size to make sure it is valid */
    DMABufferSize = stptiHelper_BufferSizeAdjust(sizeof(TCStatus_t) * NO_OF_STATUS_BLOCKS);

    PrivateData_p->InterruptDMABufferSize = DMABufferSize + DMAScratchAreaSize;

#if defined(ST_OSLINUX) && defined(STPTI_SUPPRESS_CACHING)
    /* malloc structure from non-cached ram */
    base_p = (U8 *)dma_alloc_coherent(NULL,
                                      PrivateData_p->InterruptDMABufferSize,
                                      (dma_addr_t *)&(PrivateData_p->InterruptDMABufferInfo),
                                      GFP_KERNEL);
#else

    /* Allocate the required memory which returns a virtual address */
    base_p = (U8 *) STOS_MemoryAllocate( Partition_p, DMABufferSize + DMAScratchAreaSize );

#endif /* #endif defined(ST_OSLINUX) */
    
    if (base_p == NULL)
    {
        STTBX_Print(("NO MEMORY\n"));
        PrivateData_p->InterruptBufferStart_p = NULL;
        return( ST_ERROR_NO_MEMORY );
    }

    stpti_FlushRegion((void *)base_p, DMABufferSize + DMAScratchAreaSize);

    /* Store the virtual address of start of the memory returned by the allocate */
    PrivateData_p->InterruptBufferRealStart_p = base_p;

    base_p=stptiHelper_BufferBaseAdjust(base_p);

    PrivateData_p->InterruptBufferStart_p = base_p;

    /* Convert to a physical address for TC. */
    base_p = (U8 *)STOS_VirtToPhys( base_p );

    /* Sort out base and top alignment */

    top_p = base_p + DMABufferSize;

    /* set top_p, btm, read_p & write_p ( qwrite_p etc not required ) */
    STSYS_WriteRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMABase_p, (U32) base_p );
    STSYS_WriteRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMATop_p, ((U32) top_p-1) & ~0X0f );
    STSYS_WriteRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMARead_p, (U32) base_p );
    STSYS_WriteRegDev32LE( (void*)&TCInterruptDMAConfig_p->DMAWrite_p, (U32) base_p );
    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHelper_TCInit_AllocPrivateData( FullHandle_t DeviceHandle )
{
    Device_t *Device_p = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t      *PrivateData_p = &Device_p->TCPrivateData;
    ST_Partition_t       *Partition_p   =  Device_p->DriverPartition_p;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    U16 loop;
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    U32 allocated = 0;
#endif

    /* Prevent compiler warning with STOS_MemoryAllocate() for platforms which ignore the first parameter */
    Partition_p = Partition_p;

#ifndef STPTI_NO_INDEX_SUPPORT
    PrivateData_p->IndexHandles_p             = NULL;
#endif
    PrivateData_p->BufferHandles_p            = NULL;
    PrivateData_p->SlotHandles_p              = NULL;
    PrivateData_p->PCRSlotHandles_p           = NULL;
    PrivateData_p->SectionFilterHandles_p     = NULL;
    PrivateData_p->SectionFilterStatus_p      = NULL;
    PrivateData_p->PesFilterHandles_p         = NULL;
    PrivateData_p->TransportFilterHandles_p   = NULL;
    PrivateData_p->PesStreamIdFilterHandles_p = NULL;
    PrivateData_p->DescramblerHandles_p       = NULL;
    PrivateData_p->SeenStatus_p               = NULL;


    /* Build Private data structures ( these can be in cached memory ) */

#ifndef STPTI_NO_INDEX_SUPPORT

    if (TC_Params_p->TC_NumberSlots > 0)
    {
        PrivateData_p->IndexHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Index_t) * TC_Params_p->TC_NumberIndexs);
        if (PrivateData_p->IndexHandles_p == NULL)
            return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Index_t) * TC_Params_p->TC_NumberIndexs;
        STTBX_Print(("\nstptiHelper_TCInit_AllocPrivateData() IndexHandles_p %u bytes\n", sizeof(STPTI_Index_t) * TC_Params_p->TC_NumberIndexs ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberIndexs; loop++) PrivateData_p->IndexHandles_p[loop] = STPTI_NullHandle();
        PrivateData_p->SCDTableAllocation=0;
    }
#endif

    if (TC_Params_p->TC_NumberDMAs > 0)
    {
        PrivateData_p->BufferHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Buffer_t) * TC_Params_p->TC_NumberDMAs);
        if (PrivateData_p->BufferHandles_p == NULL)
            return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Buffer_t) * TC_Params_p->TC_NumberDMAs;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() BufferHandles_p %u bytes\n", sizeof(STPTI_Buffer_t) * TC_Params_p->TC_NumberDMAs ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberDMAs; loop++) PrivateData_p->BufferHandles_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberSlots > 0)
    {
        PrivateData_p->SlotHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSlots);
        if (PrivateData_p->SlotHandles_p == NULL)
            return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSlots;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() SlotHandles_p %u bytes\n", sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSlots ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberSlots; loop++) PrivateData_p->SlotHandles_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberSlots > 0)
    {
        PrivateData_p->PCRSlotHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSlots);
        if (PrivateData_p->PCRSlotHandles_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSlots;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() PCRSlotHandles_p %u bytes\n", sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSlots ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberSlots; loop++) PrivateData_p->PCRSlotHandles_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberSectionFilters > 0)
    {
        PrivateData_p->SectionFilterHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberSectionFilters);
        if (PrivateData_p->SectionFilterHandles_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberSectionFilters;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() TC_NumberSectionFilters %u bytes\n", sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberSectionFilters ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberSectionFilters; loop++) PrivateData_p->SectionFilterHandles_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberSectionFilters > 0)
    {
        PrivateData_p->SectionFilterStatus_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSectionFilters);
        if (PrivateData_p->SectionFilterStatus_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSectionFilters;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() SectionFilterStatus_p %u bytes\n", sizeof(STPTI_Slot_t) * TC_Params_p->TC_NumberSectionFilters ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberSectionFilters; loop++) PrivateData_p->SectionFilterStatus_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberPesFilters > 0)
    {
        PrivateData_p->PesFilterHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberPesFilters);
        if (PrivateData_p->PesFilterHandles_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberPesFilters;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() PesFilterHandles_p %u bytes\n", sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberPesFilters ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberPesFilters; loop++) PrivateData_p->PesFilterHandles_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberTransportFilters > 0)
    {
        PrivateData_p->TransportFilterHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberTransportFilters);
        if (PrivateData_p->TransportFilterHandles_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberTransportFilters;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() TransportFilterHandles_p %u bytes\n", sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberTransportFilters ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberTransportFilters; loop++) PrivateData_p->TransportFilterHandles_p[loop] = STPTI_NullHandle();
    }

    if (TC_Params_p->TC_NumberSlots > 0)
    {
        PrivateData_p->PesStreamIdFilterHandles_p = STOS_MemoryAllocate(Partition_p, sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberSlots);
        if (PrivateData_p->PesStreamIdFilterHandles_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberSlots;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() PesStreamIdFilterHandles_p %u bytes\n", sizeof(STPTI_Filter_t) * TC_Params_p->TC_NumberSlots ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberSlots; loop++) PrivateData_p->PesStreamIdFilterHandles_p[loop] = STPTI_NullHandle();
    }


    
#if !defined ( STPTI_BSL_SUPPORT )
#ifndef STPTI_LOADER_SUPPORT
    if (TC_Params_p->TC_NumberDescramblerKeys > 0)
    {
        PrivateData_p->DescramblerHandles_p = STOS_MemoryAllocate(Partition_p, TC_Params_p->TC_SizeOfDescramblerKeys * TC_Params_p->TC_NumberDescramblerKeys);
        if (PrivateData_p->DescramblerHandles_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += TC_Params_p->TC_SizeOfDescramblerKeys * TC_Params_p->TC_NumberDescramblerKeys;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() DescramblerHandles_p %u bytes\n", TC_Params_p->TC_SizeOfDescramblerKeys * TC_Params_p->TC_NumberDescramblerKeys));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberDescramblerKeys; loop++) PrivateData_p->DescramblerHandles_p[loop] = STPTI_NullHandle();
    }
#endif
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

    if (TC_Params_p->TC_NumberSlots > 0)
    {
        PrivateData_p->SeenStatus_p = STOS_MemoryAllocate(Partition_p, sizeof(U8) * TC_Params_p->TC_NumberSlots);
        if (PrivateData_p->SeenStatus_p == NULL)
             return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        allocated += sizeof(U8) * TC_Params_p->TC_NumberSlots;
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() SeenStatus_p %u bytes\n",  sizeof(U8) * TC_Params_p->TC_NumberSlots ));
#endif
        for (loop = 0; loop < TC_Params_p->TC_NumberSlots; loop++) PrivateData_p->SeenStatus_p[loop] = 0;
    }

    PrivateData_p->TCUserDma_p = STOS_MemoryAllocate(Partition_p, sizeof( TCUserDma_t ) );
    if (PrivateData_p->TCUserDma_p == NULL)
         return( ST_ERROR_NO_MEMORY );
#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    allocated +=  sizeof( TCUserDma_t );
    STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() TCPrivateDma_p %u bytes\n", sizeof( TCUserDma_t ) ));
#endif
    for (loop = 0; loop < 4; loop++) PrivateData_p->TCUserDma_p->DirectDmaUsed[loop] = FALSE;


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
        STTBX_Print(("stptiHelper_TCInit_AllocPrivateData() Allocated %u bytes\n", allocated ));
#endif

    return( ST_NO_ERROR );
}


/* EOF  -------------------------------------------------------------------- */
