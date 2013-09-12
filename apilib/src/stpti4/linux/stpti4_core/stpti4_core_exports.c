/*****************************************************************************
 * COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.
 *
 *  Module      : stpti4_core_exports.c
 *  Date        : 17-04-2005
 *  Author      : STIEGLITZP
 *  Description : Implementation for exporting STAPI functions
 *
 *****************************************************************************/


/* Requires   MODULE         defined on the command line */
/* Requires __KERNEL__       defined on the command line */
/* Requires __SMP__          defined on the command line for SMP systems   */
/* Requires EXPORT_SYMTAB    defined on the command line to export symbols */

#include <linux/init.h>    /* Initiliasation support */
#include <linux/module.h>  /* Module support */
#include <linux/kernel.h>  /* Kernel support */
#include <linux/version.h> /* Kernel version */

#include "stpti.h"
#include "memget.h"

#include "pti_linux.h"

/*** PROTOTYPES **************************************************************/

/*** LOCAL TYPES *********************************************************/


/*** LOCAL CONSTANTS ********************************************************/

/*** LOCAL VARIABLES *********************************************************/

/*** METHODS ****************************************************************/

#define EXPORT_SYMTAB

/* Exported STPTI functions */
EXPORT_SYMBOL(STPTI_AlternateOutputSetDefaultAction);
EXPORT_SYMBOL(STPTI_BufferAllocate);
EXPORT_SYMBOL(STPTI_BufferAllocateManual);
EXPORT_SYMBOL(STPTI_BufferDeallocate);
EXPORT_SYMBOL(STPTI_BufferExtractData);
EXPORT_SYMBOL(STPTI_BufferExtractPartialPesPacketData);
EXPORT_SYMBOL(STPTI_BufferExtractPesPacketData);
EXPORT_SYMBOL(STPTI_BufferExtractSectionData);
EXPORT_SYMBOL(STPTI_BufferExtractTSHeaderData);
EXPORT_SYMBOL(STPTI_BufferFlush);
EXPORT_SYMBOL(STPTI_BufferGetFreeLength);
EXPORT_SYMBOL(STPTI_BufferGetWritePointer);
EXPORT_SYMBOL(STPTI_BufferPacketCount);
EXPORT_SYMBOL(STPTI_BufferLevelChangeEvent);
EXPORT_SYMBOL(STPTI_BufferLinkToCdFifo);
EXPORT_SYMBOL(STPTI_BufferRead);
EXPORT_SYMBOL(STPTI_BufferReadNBytes);
EXPORT_SYMBOL(STPTI_BufferReadPartialPesPacket);
EXPORT_SYMBOL(STPTI_BufferReadPes);
EXPORT_SYMBOL(STPTI_BufferReadSection);
EXPORT_SYMBOL(STPTI_BufferReadTSPacket);
EXPORT_SYMBOL(STPTI_BufferSetMultiPacket);
EXPORT_SYMBOL(STPTI_BufferSetOverflowControl);
EXPORT_SYMBOL(STPTI_BufferSetReadPointer);
EXPORT_SYMBOL(STPTI_BufferTestForData);
EXPORT_SYMBOL(STPTI_BufferUnLink);
EXPORT_SYMBOL(STPTI_Close);
EXPORT_SYMBOL(STPTI_Debug);
EXPORT_SYMBOL(STPTI_DescramblerAllocate);
EXPORT_SYMBOL(STPTI_DescramblerAssociate);
EXPORT_SYMBOL(STPTI_DescramblerDeallocate);
EXPORT_SYMBOL(STPTI_DescramblerDisassociate);
EXPORT_SYMBOL(STPTI_DescramblerSet);
EXPORT_SYMBOL(STPTI_DescramblerSetType);
EXPORT_SYMBOL(stptiMemGet_Descrambler);
EXPORT_SYMBOL(stpti_DescramblerHandleCheck);
EXPORT_SYMBOL(STPTI_DisableErrorEvent);
EXPORT_SYMBOL(STPTI_DisableScramblingEvents);
EXPORT_SYMBOL(STPTI_DumpInputTS);
EXPORT_SYMBOL(STPTI_EnableErrorEvent);
EXPORT_SYMBOL(STPTI_EnableScramblingEvents);
EXPORT_SYMBOL(STPTI_FilterAllocate);
EXPORT_SYMBOL(STPTI_FilterAssociate);
EXPORT_SYMBOL(STPTI_FilterDeallocate);
EXPORT_SYMBOL(STPTI_FilterDisassociate);
EXPORT_SYMBOL(STPTI_FilterSet);
EXPORT_SYMBOL(STPTI_FilterSetMatchAction);
EXPORT_SYMBOL(STPTI_FiltersFlush);
EXPORT_SYMBOL(STPTI_GetBuffersFromSlot);
EXPORT_SYMBOL(STPTI_GetCapability);
EXPORT_SYMBOL(STPTI_GetCurrentPTITimer);
EXPORT_SYMBOL(STPTI_GetGlobalPacketCount);
EXPORT_SYMBOL(STPTI_GetInputPacketCount);
EXPORT_SYMBOL(STPTI_GetPacketArrivalTime);
EXPORT_SYMBOL(STPTI_GetPacketErrorCount);
EXPORT_SYMBOL(STPTI_GetPresentationSTC);
EXPORT_SYMBOL(STPTI_GetRevision);
EXPORT_SYMBOL(STPTI_HardwareFIFOLevels);
EXPORT_SYMBOL(STPTI_HardwarePause);
EXPORT_SYMBOL(STPTI_HardwareReset);
EXPORT_SYMBOL(STPTI_HardwareResume);
EXPORT_SYMBOL(STPTI_Init);
EXPORT_SYMBOL(STPTI_ModifyGlobalFilterState);
EXPORT_SYMBOL(STPTI_ModifySyncLockAndDrop);
EXPORT_SYMBOL(STPTI_Open);
EXPORT_SYMBOL(STPTI_PidQuery);
EXPORT_SYMBOL(STPTI_PrintDebug);
EXPORT_SYMBOL(STPTI_SetDiscardParams);
EXPORT_SYMBOL(STPTI_SetStreamID);
EXPORT_SYMBOL(STPTI_SetSystemKey);
EXPORT_SYMBOL(STPTI_SignalAbort);
EXPORT_SYMBOL(STPTI_SignalAllocate);
EXPORT_SYMBOL(STPTI_SignalAssociateBuffer);
EXPORT_SYMBOL(STPTI_SignalDeallocate);
EXPORT_SYMBOL(STPTI_SignalDisassociateBuffer);
EXPORT_SYMBOL(STPTI_SignalWaitBuffer);
EXPORT_SYMBOL(STPTI_SlotAllocate);
EXPORT_SYMBOL(STPTI_SlotClearPid);
EXPORT_SYMBOL(STPTI_SlotDeallocate);
EXPORT_SYMBOL(STPTI_SlotDescramblingControl);
EXPORT_SYMBOL(STPTI_SlotLinkToBuffer);
EXPORT_SYMBOL(STPTI_SlotLinkToRecordBuffer);
EXPORT_SYMBOL(STPTI_SlotPacketCount);
EXPORT_SYMBOL(STPTI_SlotQuery);
EXPORT_SYMBOL(STPTI_SlotSetAlternateOutputAction);
EXPORT_SYMBOL(STPTI_SlotSetCCControl);
EXPORT_SYMBOL(STPTI_SlotSetPid);
EXPORT_SYMBOL(STPTI_SlotState);
EXPORT_SYMBOL(STPTI_SlotUnLink);
EXPORT_SYMBOL(STPTI_SlotUnLinkRecordBuffer);
EXPORT_SYMBOL(STPTI_SlotUpdatePid);
EXPORT_SYMBOL(STPTI_Term);
EXPORT_SYMBOL(STPTI_TermAll);
EXPORT_SYMBOL(STPTI_WaitEventList);

#ifndef STPTI_NO_INDEX_SUPPORT
EXPORT_SYMBOL(STPTI_IndexAllocate);
EXPORT_SYMBOL(STPTI_IndexAssociate);
EXPORT_SYMBOL(STPTI_IndexDeallocate);
EXPORT_SYMBOL(STPTI_IndexDisassociate);
EXPORT_SYMBOL(STPTI_IndexReset);
EXPORT_SYMBOL(STPTI_IndexSet);
EXPORT_SYMBOL(STPTI_IndexChain);
EXPORT_SYMBOL(STPTI_IndexStart);
EXPORT_SYMBOL(STPTI_IndexStop);
#endif /* #ifndef STPTI_NO_INDEX_SUPPORT */

EXPORT_SYMBOL(STPTI_SlotSetCorruptionParams);


/* User space mapping helpers */
EXPORT_SYMBOL(stpti_GetUserSpaceMap);
EXPORT_SYMBOL(stpti_SetUserSpaceMap);
EXPORT_SYMBOL(stpti_BufferCacheInvalidate);

/* The following is for test only */
EXPORT_SYMBOL(STPTI_OSLINUX_SendSessionEvent);
EXPORT_SYMBOL(STPTI_OSLINUX_IsEvent);
EXPORT_SYMBOL(STPTI_OSLINUX_GetNextEvent);
EXPORT_SYMBOL(STPTI_OSLINUX_SubscribeEvent);
EXPORT_SYMBOL(STPTI_OSLINUX_UnsubscribeEvent);
EXPORT_SYMBOL(STPTI_OSLINUX_GetWaitQueueHead);
EXPORT_SYMBOL(STPTI_OSLINUX_WaitForEvent);
EXPORT_SYMBOL(STPTI_OSLINUX_QueueSessionEvent);

#if defined( STPTI_FRONTEND_HYBRID )
EXPORT_SYMBOL(STPTI_FrontendAllocate);
EXPORT_SYMBOL(STPTI_FrontendDeallocate);
EXPORT_SYMBOL(STPTI_FrontendGetParams);
EXPORT_SYMBOL(STPTI_FrontendGetStatus);
EXPORT_SYMBOL(STPTI_FrontendInjectData);
EXPORT_SYMBOL(STPTI_FrontendLinkToPTI);
EXPORT_SYMBOL(STPTI_FrontendReset);
EXPORT_SYMBOL(STPTI_FrontendSetParams);
EXPORT_SYMBOL(STPTI_FrontendUnLink);
#endif /* #if defined( STPTI_FRONTEND_HYBRID ) ... #endif */

#ifdef STPTI_CAROUSEL_SUPPORT
EXPORT_SYMBOL(STPTI_CarouselAllocate);
EXPORT_SYMBOL(STPTI_CarouselDeallocate);
EXPORT_SYMBOL(STPTI_CarouselEntryAllocate);
EXPORT_SYMBOL(STPTI_CarouselEntryDeallocate);
EXPORT_SYMBOL(STPTI_CarouselEntryLoad);
EXPORT_SYMBOL(STPTI_CarouselGetCurrentEntry);
EXPORT_SYMBOL(STPTI_CarouselInsertEntry);
EXPORT_SYMBOL(STPTI_CarouselInsertTimedEntry);
EXPORT_SYMBOL(STPTI_CarouselLinkToBuffer);
EXPORT_SYMBOL(STPTI_CarouselLock);
EXPORT_SYMBOL(STPTI_CarouselRemoveEntry);
EXPORT_SYMBOL(STPTI_CarouselSetAllowOutput);
EXPORT_SYMBOL(STPTI_CarouselSetBurstMode);
EXPORT_SYMBOL(STPTI_CarouselUnlinkBuffer);
EXPORT_SYMBOL(STPTI_CarouselUnLock);
#endif /* #ifdef STPTI_CAROUSEL_SUPPORT */


EXPORT_SYMBOL(STPTI_BufferLevelSignalDisable);
EXPORT_SYMBOL(STPTI_BufferLevelSignalEnable);

EXPORT_SYMBOL(STPTI_ObjectEnableFeature);
EXPORT_SYMBOL(STPTI_ObjectDisableFeature);

