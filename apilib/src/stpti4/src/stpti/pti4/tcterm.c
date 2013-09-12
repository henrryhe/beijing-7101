/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: tcterm.c
 Description: TC termination functions, used by basic.c: stptiHAL_term()

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #endif !defined(ST_OSLINUX) */

#if defined(ST_OSLINUX)
#include <linux/dma-mapping.h> /* dma_alloc_coherent etc. */
#endif

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "stpti.h"

#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"

/* ------------------------------------------------------------------------- */

#if !defined ( STPTI_BSL_SUPPORT )
void stptiHelper_TCTerm_FreePrivateData( FullHandle_t DeviceHandle )
{
    Device_t *Device_p = stptiMemGet_Device( DeviceHandle );
    TCPrivateData_t *PrivateData_p     = &Device_p->TCPrivateData;
    ST_Partition_t  *Partition_p       = Device_p->DriverPartition_p;

    ST_Partition_t  *InterruptBufferPartition_p = Device_p->NonCachedPartition_p;

    /* See if we are using cached partitions */
    if( InterruptBufferPartition_p == NULL )
    {
        InterruptBufferPartition_p = Device_p->DriverPartition_p;
    }
    
    /* Prevent compiler warning with STOS_MemoryAllocate() for platforms which ignore the first parameter */
    Partition_p = Partition_p;

#if defined(ST_OSLINUX) && defined(STPTI_SUPPRESS_CACHING)
    if (PrivateData_p->InterruptBufferStart_p   != NULL){
        dma_free_coherent(NULL,
                          PrivateData_p->InterruptDMABufferSize,
                          bus_to_virt( (U32)PrivateData_p->InterruptBufferRealStart_p ),
                          PrivateData_p->InterruptDMABufferInfo );    
    }
#else
    if (PrivateData_p->InterruptBufferStart_p   != NULL) STOS_MemoryDeallocate(InterruptBufferPartition_p, PrivateData_p->InterruptBufferRealStart_p);
#endif /* #endif defined(ST_OSLINUX)  */

#ifndef STPTI_NO_INDEX_SUPPORT                
    if (PrivateData_p->IndexHandles_p           != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->IndexHandles_p);
#endif                
    if (PrivateData_p->BufferHandles_p          != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->BufferHandles_p);
    if (PrivateData_p->SlotHandles_p            != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->SlotHandles_p);
    if (PrivateData_p->PCRSlotHandles_p         != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->PCRSlotHandles_p);
    if (PrivateData_p->SectionFilterHandles_p   != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->SectionFilterHandles_p);
    if (PrivateData_p->SectionFilterStatus_p    != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->SectionFilterStatus_p);
    if (PrivateData_p->PesFilterHandles_p       != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->PesFilterHandles_p);
    if (PrivateData_p->TransportFilterHandles_p != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->TransportFilterHandles_p);
    if (PrivateData_p->PesStreamIdFilterHandles_p != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->PesStreamIdFilterHandles_p);
    if (PrivateData_p->DescramblerHandles_p     != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->DescramblerHandles_p);
    if (PrivateData_p->SeenStatus_p             != NULL) STOS_MemoryDeallocate(Partition_p, PrivateData_p->SeenStatus_p);
    if (PrivateData_p->TCUserDma_p              != NULL) STOS_MemoryDeallocate(Partition_p, (void *)PrivateData_p->TCUserDma_p);
    /* 'implicit cast of pointer to non-equal pointer' st20cc error if TCUserDma_p is not cast to void* */
#ifndef STPTI_NO_INDEX_SUPPORT                
    PrivateData_p->IndexHandles_p           = NULL;
#endif
    PrivateData_p->BufferHandles_p          = NULL;
    PrivateData_p->SlotHandles_p            = NULL;
    PrivateData_p->PCRSlotHandles_p         = NULL;
    PrivateData_p->SectionFilterHandles_p   = NULL;
    PrivateData_p->SectionFilterStatus_p    = NULL;
    PrivateData_p->PesFilterHandles_p       = NULL;
    PrivateData_p->TransportFilterHandles_p = NULL;
    PrivateData_p->DescramblerHandles_p     = NULL;
    PrivateData_p->SeenStatus_p             = NULL;
    PrivateData_p->TCUserDma_p              = NULL;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */



/* EOF  -------------------------------------------------------------------- */

