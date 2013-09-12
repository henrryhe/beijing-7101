/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: main.c
 Description: build HAL to check for unresolved references

******************************************************************************/
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "stddefs.h"
#include "stboot.h"
#include "stdevice.h"

#include "sttbx.h"
#include "stpti.h"

#include "pti_evt.h"
#include "pti_hndl.h"

#include "pti_loc.h"
#include "pti_hal.h"

#include "pti4.h"


/* --------------------------------------------- */

#define SYSTEM_HEADROOM              0x00200000

#ifndef ST_5528
#if defined(ST_5514)
 #define NCACHE_HEADROOM              0x00100000
 #define NCACHE_MEMORY_SIZE           0x00200000
 #define SYSTEM_MEMORY_SIZE          (0x01000000 - NCACHE_MEMORY_SIZE)
#else
 #define NCACHE_HEADROOM              0x00010000
 #define NCACHE_MEMORY_SIZE           0x00080000
 #define SYSTEM_MEMORY_SIZE          (0x01000000 - NCACHE_MEMORY_SIZE)
#endif

#else   /* 5528 */

#define NCACHE_HEADROOM              0x00100000
#define NCACHE_MEMORY_SIZE           0x00200000
#define SYSTEM_MEMORY_SIZE          (0x00800000 - NCACHE_MEMORY_SIZE)

#endif

/* --------------------------------------------- */

unsigned char InternalBlock[ST20_INTERNAL_MEMORY_SIZE - 1200];
#pragma ST_section      (InternalBlock, "internal_section_noinit")

unsigned char NcacheMemory[NCACHE_MEMORY_SIZE - NCACHE_HEADROOM];
#pragma ST_section      (NcacheMemory , "ncache_section")

#define SUBSECTION_SIZE (10*1024)
unsigned char ExternalBlock[SYSTEM_MEMORY_SIZE - SUBSECTION_SIZE - SYSTEM_HEADROOM];
#pragma ST_section      (ExternalBlock, "system_section_noinit")

unsigned char OtherBlock[SUBSECTION_SIZE];
#pragma ST_section      (OtherBlock, "system_section_noinit")

ST_Partition_t TheInternalPartition;
ST_Partition_t TheSystemPartition;
ST_Partition_t TheOtherPartition;
ST_Partition_t TheNcachePartition;

/* the 3 declarations below MUST be kept so os20 doesn't fall over!! */
ST_Partition_t *internal_partition = &TheInternalPartition;
ST_Partition_t *system_partition = &TheSystemPartition;
ST_Partition_t *ncache_partition = &TheNcachePartition;

/* --------------------------------------------- */

/* things that drop out because we are not including stpti core files */

Driver_t STPTI_Driver;

/* pti_evt.h */
ST_ErrorCode_t stpti_DirectQueueEvent(FullHandle_t Handle, STEVT_EventConstant_t Event, STPTI_EventData_t * EventData)
{
    return( ST_NO_ERROR );
}

/* pti_indx.h */
ST_ErrorCode_t stpti_DirectQueueIndex(FullHandle_t Handle, STEVT_EventConstant_t Event, BOOL Dumped, BOOL Signalled, STPTI_EventData_t * EventData)
{
    return( ST_NO_ERROR );
}
        

/* stpti_ba.c */
volatile U32 stpti_InterruptTotalCount = 0;

/* stpti_ba.c */
ST_ErrorCode_t stpti_PidQueryExistingSlot(FullHandle_t DeviceHandle, STPTI_Pid_t Pid, STPTI_Slot_t * Slot_p)
{
    return( ST_NO_ERROR );
}

/* --------------------------------------------- */

int main(void)
{
    FullHandle_t ShadowSlotHandle, SlotHandle, FullBufferHandle, DeviceHandle;
    FullHandle_t LegacySlotHandle, CarouselHandle, FilterHandle, FullHandle;
    ST_ErrorCode_t ST_ErrorCode;
    BOOL status;
    void *DeviceHandle_p;
    U32 Flags;
    BOOL Enable;
    STPTI_Filter_t *MatchedFilterList;
    U32 MaxLengthofFilterList;
    U32 *NumOfFilterMatches_p;
    STPTI_MPTFlags_t *MPTFlags_p;
    BOOL *CRCValid_p;
    U32 *FilterIdent;
    U32 *BufferCount;
    STPTI_Pid_t *LastPid;
    U32 *IndexIdent;
    U16 *Count_p;

    printf(" you shouldn't really run this program...\n");

    /* call a function from every file built for hal.lib to make sure its linked in (check main.map after build for verification) */
    
    ST_ErrorCode = stptiHelper_SlotSetGlobalInterruptMask(SlotHandle, Flags, Enable);     /* helper.c */
                   stptiHAL_InterruptHandler(DeviceHandle_p);                             /* irq.c */
                   stptiHAL_DebugDumpStatus(DeviceHandle);                                /* debug.c */
    ST_ErrorCode = stptiHAL_SPSlotLinkShadowToLegacy(ShadowSlotHandle, LegacySlotHandle); /* sp.c  */
    ST_ErrorCode = stptiHAL_DVBSlotDescramblingControl(SlotHandle, status);               /* dvb.c */
    ST_ErrorCode = stptiHAL_DC2FilterAllocateMulticast16(DeviceHandle, FilterIdent);      /* dc2.c */
    ST_ErrorCode = stptiHAL_CarouselSetAllowOutput(CarouselHandle);                       /* carousel.c */
    ST_ErrorCode = stptiHAL_IndexAllocate(DeviceHandle, IndexIdent);                      /* index.c */
    ST_ErrorCode = stptiHAL_SlotPacketCount(SlotHandle, Count_p);                         /* pti_tc4/extended.c */
    ST_ErrorCode = stptiHAL_FilterAssociateSection(SlotHandle, FilterHandle);             /* pti_ba4/basic.c */
    ST_ErrorCode = stpti_HAL_Validate_FilterOneShotSupported( SlotHandle );               /* validate.c */
    Flags        = stptiHAL_LoaderGetConfig( FullHandle, 0 );                             /* config.c */
#ifdef STPTI_GPDMA_SUPPORT
    ST_ErrorCode = stptiHAL_BufferClearGPDMAParams( FullBufferHandle );                   /* gpdma.c */
#endif    
    printf("...oh well!\n");

    return(0);
}


/* --------------------------------------------- */

/* EOF */
