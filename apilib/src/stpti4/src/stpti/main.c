/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: main.c 
 Description: build stpti4.lib and link to check for unresolved references

******************************************************************************/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <semaphor.h>

#include "stddefs.h"
#include "stdevice.h"
#include "stpti.h"


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


int main(void)
{
    ST_ErrorCode_t ST_ErrorCode;
    STPTI_TimeStamp_t T;
    STPTI_SlotOrPid_t SlotOrPid;
    STPTI_IndexDefinition_t I;

ST_ErrorCode = STPTI_AlternateOutputSetDefaultAction(66, 0, 0);
ST_ErrorCode = STPTI_BufferAllocate(66, 0, 0, NULL);
ST_ErrorCode = STPTI_BufferAllocateManual(66, NULL, 0, 0, NULL); 
#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode = STPTI_BufferClearGPDMAParams(88);
#endif
ST_ErrorCode = STPTI_BufferDeallocate(88);
ST_ErrorCode = STPTI_BufferExtractData(88, 0, 0, NULL, 0, NULL, 0, NULL, 0);
ST_ErrorCode = STPTI_BufferExtractPartialPesPacketData(88, NULL, NULL, NULL, NULL);

ST_ErrorCode = STPTI_BufferExtractPesPacketData(88, NULL, NULL, NULL, NULL, NULL);

ST_ErrorCode = STPTI_BufferExtractSectionData(88, NULL, 0, NULL, NULL, NULL);
ST_ErrorCode = STPTI_BufferExtractTSHeaderData(88, NULL);
ST_ErrorCode = STPTI_BufferFlush(88);
ST_ErrorCode = STPTI_BufferGetFreeLength(88, NULL);
ST_ErrorCode = STPTI_BufferGetWritePointer(88, NULL);
ST_ErrorCode = STPTI_BufferLinkToCdFifo(88, NULL);
ST_ErrorCode = STPTI_BufferRead(88, NULL, 0, NULL, 0, NULL, 0);
ST_ErrorCode = STPTI_BufferReadNBytes(88, NULL, 0, NULL, 0, NULL, 0, 0);
ST_ErrorCode = STPTI_BufferReadPartialPesPacket(88, NULL, 0, NULL,0, NULL, NULL, NULL, NULL, 0);
ST_ErrorCode = STPTI_BufferReadPes(88, NULL, 0, NULL, 0, NULL, 0);
ST_ErrorCode = STPTI_BufferReadSection(88, NULL, 0, NULL, NULL, NULL, 0, NULL, 0, NULL, 0);

ST_ErrorCode = STPTI_BufferReadTSPacket(88, NULL, 0, NULL, 0,NULL, 0);
#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode = STPTI_BufferSetGPDMAParams(88, 0);
#endif
ST_ErrorCode = STPTI_BufferSetMultiPacket(88, 0);
ST_ErrorCode = STPTI_BufferSetOverflowControl(88, 0);
ST_ErrorCode = STPTI_BufferSetReadPointer(88,NULL);
ST_ErrorCode = STPTI_BufferTestForData(88, NULL);
ST_ErrorCode = STPTI_BufferUnLink(3);
#ifdef STPTI_CAROUSEL_SUPPORT
ST_ErrorCode = STPTI_CarouselAllocate(66, NULL);
ST_ErrorCode = STPTI_CarouselDeallocate(55);
ST_ErrorCode = STPTI_CarouselEntryAllocate(66, NULL);
ST_ErrorCode = STPTI_CarouselEntryDeallocate(44);
ST_ErrorCode = STPTI_CarouselEntryLoad(22, NULL, 0);
ST_ErrorCode = STPTI_CarouselGetCurrentEntry(55, NULL, NULL);
ST_ErrorCode = STPTI_CarouselInsertEntry(55, 0, 0, 0);
ST_ErrorCode = STPTI_CarouselInsertTimedEntry(0, NULL, 0, 0, T, T, 0, 0,0, 0, 0);
ST_ErrorCode = STPTI_CarouselLinkToBuffer(55, 88);
ST_ErrorCode = STPTI_CarouselLock(55);
ST_ErrorCode = STPTI_CarouselRemoveEntry(55, 0);
ST_ErrorCode = STPTI_CarouselSetAllowOutput(55, 0);
ST_ErrorCode = STPTI_CarouselSetBurstMode(55, 0, 0);
ST_ErrorCode = STPTI_CarouselUnlinkBuffer(55, 88);
ST_ErrorCode = STPTI_CarouselUnLock(55);
#endif
ST_ErrorCode = STPTI_Close(66);
#ifdef STPTI_DEBUG_SUPPORT
ST_ErrorCode = STPTI_DebugDMAHistorySetup(NULL, NULL, 0);
ST_ErrorCode = STPTI_DebugGetDMAHistory(NULL, NULL, NULL);
ST_ErrorCode = STPTI_DebugInterruptHistorySetup(NULL, NULL, 0);
ST_ErrorCode = STPTI_DebugGetInterruptHistory(NULL, NULL, NULL);
ST_ErrorCode = STPTI_DebugGetTCIPTR(NULL, NULL);
ST_ErrorCode = STPTI_DebugGetTCDRAM(NULL);
ST_ErrorCode = STPTI_DebugStatisticsRead(NULL, NULL);
ST_ErrorCode = STPTI_DebugStatisticsReset(NULL);
ST_ErrorCode = STPTI_DebugStatisticsStart(NULL);
ST_ErrorCode = STPTI_DebugStatisticsStop(NULL);
#endif
ST_ErrorCode = STPTI_DescramblerAllocate(66, NULL, 0);
ST_ErrorCode = STPTI_DescramblerAssociate(0, SlotOrPid);
ST_ErrorCode = STPTI_DescramblerDeallocate(0);
ST_ErrorCode = STPTI_DescramblerDisassociate(0,SlotOrPid);
ST_ErrorCode = STPTI_DescramblerSet(0, 0, 0, NULL);
ST_ErrorCode = STPTI_FilterAllocate(66, 0, NULL);
ST_ErrorCode = STPTI_FilterAssociate(77, 99);
ST_ErrorCode = STPTI_FilterDeallocate(77);
ST_ErrorCode = STPTI_FilterDisassociate(77, 99);
ST_ErrorCode = STPTI_FilterSet(77, NULL);
ST_ErrorCode = STPTI_FilterSetMatchAction(77, NULL, 0);
ST_ErrorCode = STPTI_FiltersFlush(88, NULL, 0);
ST_ErrorCode = STPTI_GetCapability(NULL, NULL);
ST_ErrorCode = STPTI_GetCurrentPTITimer(NULL, NULL);
ST_ErrorCode = STPTI_GetInputPacketCount(NULL, NULL);
ST_ErrorCode = STPTI_GetPacketArrivalTime(66, NULL,NULL);
ST_ErrorCode = STPTI_GetPacketErrorCount(NULL, NULL);
ST_ErrorCode = STPTI_GetPresentationSTC(NULL, 0, NULL);
ST_ErrorCode = STPTI_HardwarePause(NULL);
ST_ErrorCode = STPTI_HardwareFIFOLevels(NULL, NULL, NULL, NULL, NULL);
ST_ErrorCode = STPTI_HardwareReset(NULL);
ST_ErrorCode = STPTI_HardwareResume(NULL);
#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode = STPTI_IndexAllocate( 0,  NULL);
ST_ErrorCode = STPTI_IndexAssociate( 0,  SlotOrPid);
ST_ErrorCode = STPTI_IndexDeallocate( 0);
ST_ErrorCode = STPTI_IndexDisassociate( 0,  SlotOrPid);
ST_ErrorCode = STPTI_IndexSet( 0, I,  0,  0);
ST_ErrorCode = STPTI_IndexStart(NULL);
ST_ErrorCode = STPTI_IndexStop(NULL);
#endif
ST_ErrorCode = STPTI_Init(NULL, NULL);
ST_ErrorCode = STPTI_ModifyGlobalFilterState(NULL, 0, NULL, 0);
ST_ErrorCode = STPTI_ModifySyncLockAndDrop(NULL, 0, 0);
ST_ErrorCode = STPTI_Open(NULL, NULL, NULL);
ST_ErrorCode = STPTI_PidQuery(NULL, 0, NULL);
ST_ErrorCode = STPTI_SignalAbort(0 );
ST_ErrorCode = STPTI_SignalAllocate(66, NULL);
ST_ErrorCode = STPTI_SignalAssociateBuffer(0 , 88);
ST_ErrorCode = STPTI_SignalDeallocate(0 );
ST_ErrorCode = STPTI_SignalDisassociateBuffer(0 , 88);
ST_ErrorCode = STPTI_SignalWaitBuffer(0 , NULL,  0);
ST_ErrorCode = STPTI_SlotAllocate(66, NULL, NULL);
ST_ErrorCode = STPTI_SlotClearPid(99);
ST_ErrorCode = STPTI_SlotDeallocate(99);
ST_ErrorCode = STPTI_SlotDescramblingControl(99, 0);
ST_ErrorCode = STPTI_SlotLinkToBuffer(0, 0);
ST_ErrorCode = STPTI_SlotLinkToCdFifo(0, NULL);
#ifdef STPTI_SP_SUPPORT
ST_ErrorCode = STPTI_SlotLinkShadowToLegacy  (0, 0, 0 );
ST_ErrorCode = STPTI_SlotUnLinkShadowToLegacy(0, 0);
#endif
ST_ErrorCode = STPTI_SlotPacketCount(99, NULL);
ST_ErrorCode = STPTI_SlotQuery(99, NULL, NULL, NULL,NULL);
ST_ErrorCode = STPTI_SlotSetAlternateOutputAction(99, 0, 0);
ST_ErrorCode = STPTI_SlotSetCCControl(99, 0);
ST_ErrorCode = STPTI_SlotSetPid(99, 0);
ST_ErrorCode = STPTI_SlotState(99,  NULL, NULL,NULL);
ST_ErrorCode = STPTI_SlotUnLink(0);
ST_ErrorCode = STPTI_Term(NULL, NULL);
ST_ErrorCode = STPTI_UserDataBlockMove(NULL,  0, NULL, 0);
ST_ErrorCode = STPTI_UserDataCircularAppend( 0, NULL,NULL);
ST_ErrorCode = STPTI_UserDataCircularSetup(NULL,  0, NULL,  0, NULL, NULL);
ST_ErrorCode = STPTI_UserDataRemaining(NULL, NULL);
ST_ErrorCode = STPTI_UserDataSynchronize( NULL);
ST_ErrorCode = STPTI_UserDataWrite(NULL,  0,NULL);
(void *)STPTI_GetRevision();
ST_ErrorCode = STPTI_EnableErrorEvent(NULL, 0);
ST_ErrorCode = STPTI_DisableErrorEvent(NULL, 0);
ST_ErrorCode = STPTI_EnableScramblingEvents(99);
ST_ErrorCode = STPTI_DisableScramblingEvents(99);
 
    return(0);
}


/* --------------------------------------------- */

/* EOF */
