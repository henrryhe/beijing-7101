#ifndef STPTI_MAKE_DOCS
/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005,2006.  All rights reserved.

       File: pti_hal.h
Description: Common HAL interface to STPTI

  The API functions are specified to reflect the top level API names
 where possible.

Rev     Date            Auth    Reason
1       16 June 2003    GJP     Initial version to be comitted to SCM system
2       17 June 2003    GJP     Added PP debug API sets, cleaned up helper functions
3       27 Aug  2003    GJP     see: diff with 6.0.0.A1 rel for refactoring changes.
4       08 Jan  2004    GJP     Added stptiHAL_BackendInterruptHandler()
5       08 Aug  2006    TN      Added stptiHAL_DescramblerSetType()

******************************************************************************/

#ifndef __PTI_HAL_H
#define __PTI_HAL_H

#if !defined(ST_OSLINUX)
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#endif /* #endif !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stpti.h"
#include "pti_loc.h"

/*  Approximate packet time in us Previous value 40us may be too low in
   some apps, upped to 75 equivalent to 20Mbps */

#ifndef TC_ONE_PACKET_TIME
#define TC_ONE_PACKET_TIME 75
#endif


/* Constants common for HAL and TC code */

#define TC_INVALID_PID			((unsigned int)0xe000)
#define TC_INVALID_LINK			((unsigned int)0xf000)
#define NO_DMA_PENDING			((unsigned int)0x0000)

#define TC_ERROR_TRANSFERRING_BUFFERED_DATA      0xffffffffU


/* Constants determined by the dvb standard */

#define DISCONTINUITY_INDICATOR		        0x0080
#define PAYLOAD_UNIT_START			0x0040
#define PAYLOAD_PRESENT				0x0010
#define CC_FIELD				0x000f
/* Bits Defining Section Parameters */
#define SSI_MASK                                0x80


/* Exported Macros ----------------------------------------------------- */


#define GetTCData(ptr, x) {                                                 \
                            U32 tmp;                                        \
                            STPTI_DevicePtr_t p;                            \
                            if(((U32)(ptr) % 4) == 0)                       \
                            {                                               \
                                p = (STPTI_DevicePtr_t)(ptr);               \
                                tmp = STSYS_ReadRegDev32LE( (void*)p );          \
                            }                                               \
                            else                                            \
                            {                                               \
                                p = (STPTI_DevicePtr_t)((U32)(ptr) & ~0x3); \
                                tmp = STSYS_ReadRegDev32LE( (void*)p );          \
                                tmp >>= 16;                                 \
                            }                                               \
                            tmp &= 0xffff;                                  \
                            (x) = tmp;                                      \
                        }

#define PutTCData(ptr, x) {                                                 \
                            U32 tmp;                                        \
                            STPTI_DevicePtr_t p;                            \
                            if(((U32)(ptr) % 4) == 0)                       \
                            {                                               \
                                p = (STPTI_DevicePtr_t)(ptr);               \
                                tmp = STSYS_ReadRegDev32LE( (void*)p );          \
                                tmp &= 0xffff0000;                          \
                                tmp |= ((x)&0xffff);                        \
                            }                                               \
                            else                                            \
                            {                                               \
                                p = (STPTI_DevicePtr_t)((U32)(ptr) & ~0x3); \
                                tmp = STSYS_ReadRegDev32LE( (void*)p );          \
                                tmp &= 0x0000ffff;                          \
                                tmp |= ((x)<<16);                           \
                            }                                               \
                            STSYS_WriteRegDev32LE( (void*)p, tmp );              \
                         }




/* ------------------------- Exported Function Prototypes ------------------------- */


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: pti_int.c
              HAL: 4
      description: 'bottom-half' of PTI isr routine
   previous alias: n/a
   ------------------------- */
BOOL stptiHAL_BackendInterruptHandler(BackendInterruptData_t *BackendInterruptData_p, BufferInterrupt_t *BufferInterrupt_p );


#ifdef STPTI_SP_SUPPORT
/* ------------------------- 
           Status: NOT YET IMPLEMENTED, NEW FUNCTIONALITY
    Referenced by: STPTI_SlotLinkShadowToLegacy
              HAL: 4
      description: Sony Passage conditional access support
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotLinkShadowToLegacy(FullHandle_t ShadowSlotHandle, FullHandle_t LegacySlotHandle, STPTI_PassageMode_t PassageMode);


/* ------------------------- 
           Status: NOT YET IMPLEMENTED, NEW FUNCTIONALITY
    Referenced by: STPTI_SlotUnLinkShadowToLegacy
              HAL: 4
      description: Sony Passage conditional access support
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotUnLinkShadowToLegacy(FullHandle_t ShadowSlotHandle, FullHandle_t LegacySlotHandle);
#endif






#ifdef STPTI_DC2_SUPPORT

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...InitialiseDC2Multicast16Filter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterInitialiseMulticast16(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...TerminateDC2MulticastFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterDeallocateMulticast(FullHandle_t DeviceHandle, U32 FilterIdent);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...GetFreeDC2Multicast16Filter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterAllocateMulticast16(FullHandle_t DeviceHandle, U32 *FilterIdent);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAssociate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...AssociateDC2Multicast16Filter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterAssociateMulticast16(FullHandle_t SlotHandle, FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDisassociate STPTI_FilterDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisassociateDC2MulticastFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterDisassociateMulticast(FullHandle_t SlotHandle, FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpDC2Multicast16Filter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterSetMulticast16(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpDC2Multicast32Filter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterSetMulticast32(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpDC2Multicast48Filter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterSetMulticast48(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...EnableDC2MulticastFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterEnableMulticast(FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisableDC2MulticastFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DC2FilterDisableMulticast(FullHandle_t FilterHandle);


#endif  /* STPTI_DC2_SUPPORT */



#ifdef STPTI_CAROUSEL_SUPPORT

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_CarouselSetAllowOutput
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetCarouselOutput
   ------------------------- */
ST_ErrorCode_t stptiHAL_CarouselSetAllowOutput(FullHandle_t CarouselHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_CarouselLinkToBuffer
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpCarouselDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_CarouselLinkToBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle, BOOL DMAExists);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: stpti_TimedCarouselTask
              HAL: 4
      description: New access function
   previous alias: ...n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_CarouselGetSubstituteDataStart(FullHandle_t CarouselHandle, STPTI_DevicePtr_t *Control_p);


#endif  /* STPTI_CAROUSEL_SUPPORT */



#ifndef STPTI_NO_INDEX_SUPPORT

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexAllocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetFreeIndex
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexAllocate(FullHandle_t DeviceHandle, U32 *IndexIdent);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexAllocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...InitialiseIndex
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexInitialise(FullHandle_t IndexHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexDeallocate STPTI_Close STPTI_Term
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...TerminateIndex
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexDeallocate(FullHandle_t IndexHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexAssociate STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...AssociateIndexWithSlot
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexAssociate(FullHandle_t IndexHandle, FullHandle_t SlotHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexDisassociate STPTI_IndexDeallocate STPTI_Close STPTI_Term STPTI_SlotDeallocate STPTI_SlotClearPid  STPTI_SlotSetPid
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...DisassociateIndexFromSlot
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexDisassociate(FullHandle_t IndexHandle, FullHandle_t SlotHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: stptiHAL_IndexChain
              HAL: 1 3 4
      description: As per STPTI API
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexChain(FullHandle_t *IndexHandles, U32 NumberOfHandles);

        
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexSet
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...IndexSet
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexSet(FullHandle_t IndexHandle);


/* ------------------------- 
    Referenced by: STPTI_IndexReset
   previous alias: None
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexReset(FullHandle_t IndexHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexStart
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...IndexStart
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexStart(FullHandle_t Handle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_IndexStop
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...IndexStop
   ------------------------- */
ST_ErrorCode_t stptiHAL_IndexStop(FullHandle_t Handle);


#endif  /* STPTI_NO_INDEX_SUPPORT */


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...DisableSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterDisableSection(FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisableTransportFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterDisableTransport(FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisablePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterDisablePES(FullHandle_t FilterHandle);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...EnableSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterEnableSection(FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...EnableTransportFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterEnableTransport(FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifyGlobalFilterState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...EnablePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterEnablePES(FullHandle_t FilterHandle);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferExtractPartialPesPacketData
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ExtractPartialPESData
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBBufferExtractPartialPesPacketData(FullHandle_t FullBufferHandle, BOOL *PayloadUnitStart, BOOL *CCDiscontinuity, U8 *ContinuityCount, U16 *DataLength);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferExtractSectionData
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ExtractSectionData
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBBufferExtractSectionData(FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U16 MaxLengthofFilterList, U16 *NumOfFilterMatches, BOOL *CRCValid, U32 *SectionHeader);
                                          

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FiltersFlush
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...FlushFilters
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFiltersFlush(FullHandle_t FullBufferHandle, STPTI_Filter_t FilterHandles[], U32 NumberOfFilters);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_GetPacketErrorCount
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetPacketErrorCount
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBGetPacketErrorCount(FullHandle_t DeviceHandle, U32 *Count_p);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferReadPes
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ReadPES
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBBufferReadPes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferReadPartialPesPacket
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ReadPartialPESData
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBBufferReadPartialPesPacket(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, BOOL *PayloadUnitStart_p, BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferReadSection
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ReadSection
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBBufferReadSection(FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U32 MaxLengthofFilterList, U32 *NumOfFilterMatches_p, BOOL *CRCValid_p, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSetMatchAction
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SectionFilterSetMatchAction
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBFilterSetMatchAction(FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotSetAlternateOutputAction
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetSlotAlternateOutputAction
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBSlotSetAlternateOutputAction(FullHandle_t SlotHandle, STPTI_AlternateOutputType_t Method);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotDescramblingControl
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SlotDescramblingControl
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBSlotDescramblingControl(FullHandle_t SlotHandle, BOOL status);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferExtractPesPacketData
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ExtractPESData
   ------------------------- */
ST_ErrorCode_t stptiHAL_DVBBufferExtractPesPacketData(FullHandle_t FullBufferHandle, U8 *PesFlags_p, U8 *TrickModeFlags_p, U32 *PacketLength, STPTI_TimeStamp_t * PTSValue, STPTI_TimeStamp_t * DTSValue);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotQuery
              HAL: 1 4
      description: As per STPTI API
   previous alias: stptiHAL_DVBSlotQuery
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotQuery(FullHandle_t SlotHandle, BOOL *PacketsSeen, BOOL *TransportScrambledPacketsSeen, BOOL *PESScrambledPacketsSeen);




/* -------------------------  common functions   ------------------------- */



/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: 
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...
   ------------------------- */
/* **** DEPRECIATED *** ST_ErrorCode_t stptiHAL_AssociateDescrambler(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle); */


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_DescramblerAssociate STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...AssociateDescramblerWithSlot
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerAssociate(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle);
/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: STPTI_FilterAssociate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...AssociatePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAssociatePES(FullHandle_t SlotHandle, FullHandle_t FilterHandle);

/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: stptiHAL_FilterAssociatePESStreamId
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...AssociatePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAssociatePESStreamId(FullHandle_t SlotHandle, FullHandle_t FilterHandle);

/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: STPTI_FilterAssociate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...AssociateSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAssociateSection(FullHandle_t SlotHandle, FullHandle_t FilterHandle);


/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: STPTI_FilterAssociate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...AssociateTSFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAssociateTransport(FullHandle_t SlotHandle, FullHandle_t FilterHandle);

/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: STPTI_BufferFlush
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...BufferFlush
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferFlush(FullHandle_t FullBufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferFlush / STPTI_BufferDeallocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: 
   ------------------------- */
ST_ErrorCode_t stptiHAL_FlushSignalsForBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_DescramblerDisassociate STPTI_DescramblerDeallocate STPTI_Close STPTI_Term STPTI_DescramblerAssociate STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...DisassociateDescrambler
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerDisassociate(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle);

/* ------------------------- 
           Status: NEW
    Referenced by:
              HAL: 4
      description: As per STPTI API
   previous alias: none
   ------------------------- */

ST_ErrorCode_t stptiHAL_SetSystemKey(FullHandle_t DeviceHandle, U8 *Data, U16 KeyNumber);



/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDisassociate STPTI_FilterDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisassociatePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDisassociatePES(FullHandle_t SlotHandle, FullHandle_t FilterHandle);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDisassociate STPTI_FilterDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisassociatePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDisassociatePESStreamId(FullHandle_t SlotHandle, FullHandle_t FilterHandle);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDisassociate STPTI_FilterDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...DisassociateSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDisassociateSection(FullHandle_t SlotHandle, FullHandle_t FilterHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDisassociate STPTI_FilterDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...DisassociateTSFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDisassociateTransport(FullHandle_t SlotHandle, FullHandle_t FilterHandle);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferExtractData
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ExtractData
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferExtractData(FullHandle_t FullBufferHandle, U32 Offset, U32 NumBytesToExtract, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferExtractTSHeaderData
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ExtractTSHeaderData
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferExtractTSHeaderData(FullHandle_t FullBufferHandle, U32 *TSHeader_p);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferTestForData
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetBytesInBuffer
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferTestForData(FullHandle_t FullBufferHandle, U32 *Count);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_DescramblerAllocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetFreeDescrambler
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerAllocate(FullHandle_t DeviceHandle, U32 *DescIdent);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...GetFreePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAllocatePES(FullHandle_t DeviceHandle, U32 *FilterIdent);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...GetFreePESStreamIdFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAllocatePESStreamId(FullHandle_t DeviceHandle, U32 *FilterIdent);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 1 (alias GetFreeSF) 3 4
      description: As per STPTI API
   previous alias: ...GetFreeSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAllocateSection(FullHandle_t DeviceHandle, STPTI_FilterType_t FilterType, U32 *SFIdent);



/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SetStreamID()
              HAL:  4
      description: As per STPTI4 API

   ------------------------- */
ST_ErrorCode_t stptiHAL_SetStreamID(FullHandle_t DeviceHandle, STPTI_StreamID_t StreamID);

/* ------------------------- 
           Status: ADDED 
    Referenced by: STPTI_SetDiscardParams()
              HAL:  4
      description: As per STPTI4 API

   ------------------------- */
ST_ErrorCode_t stptiHAL_SetDiscardParams(FullHandle_t DeviceHandle, U8 NumberOfDiscardBytes);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotAllocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetFreeSlot
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotAllocate(FullHandle_t DeviceHandle, U32 *SlotIdent);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...GetFreeTSFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterAllocateTransport(FullHandle_t DeviceHandle, U32 *FilterIdent);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_GetPacketArrivalTime
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetPacketArrivalTime
   ------------------------- */
ST_ErrorCode_t stptiHAL_GetPacketArrivalTime(FullHandle_t DeviceHandle, STPTI_TimeStamp_t *ArrivalTime, U16 *ArrivalTimeExtension);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_GetPacketArrivalTime stpti_TC3InterruptHandler
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...ConvertPCRToTimestamp
   ------------------------- */
void stptiHAL_PcrToTimestamp(STPTI_TimeStamp_t *TimeStamp, U8 *RawPCR);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: stptiHAL_STCToTimestamp 
              HAL: 3 4
      description: As per STPTI API
   previous alias:
   ------------------------- */
void stptiHAL_STCToTimestamp(Device_t *Device_p,
                                 U16 STC0,
                                 U16 STC1,
                                 U16 STC2, 
                                 STPTI_TimeStamp_t *TimeStamp,
                                 U16 *ArrivalTimeExtension_p );

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_GetCurrentPTITimer
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...GetCurrentPTITimer
   ------------------------- */
ST_ErrorCode_t stptiHAL_GetCurrentPTITimer(FullHandle_t DeviceHandle, STPTI_TimeStamp_t *TimeStamp);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: N/A
              HAL: 3 4
      description: Irq from TC
   previous alias: ...InterruptHandler
   ------------------------- */
STOS_INTERRUPT_DECLARE(stptiHAL_InterruptHandler, DeviceHandle);

                                         
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_HardwarePause STPTI_HardwareReset
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...HardwarePause
   ------------------------- */
ST_ErrorCode_t stptiHAL_HardwarePause(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_HardwareReset
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...HardwareReset
   ------------------------- */
ST_ErrorCode_t stptiHAL_HardwareReset(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_HardwareResume
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...HardwareResume
   ------------------------- */
ST_ErrorCode_t stptiHAL_HardwareResume(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_Init
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...Init
   ------------------------- */
ST_ErrorCode_t stptiHAL_Init(FullHandle_t DeviceHandle, FullHandle_t ExistingSessionDeviceHandle, ST_ErrorCode_t (*TCLoader) (STPTI_DevicePtr_t, void *));


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_Init
              HAL: 4
      description: As per STPTI API
   previous alias: n/a
   ------------------------- */
U16 stptiHAL_GetNextFreeSession(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_Init
              HAL: 4
      description: As per STPTI API
   previous alias: n/a
   ------------------------- */
U16 stptiHAL_PeekNextFreeSession(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_Init
              HAL: 4
      description: As per STPTI API
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_ReleaseSession(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_Term
              HAL: 4
      description: As per STPTI API
   previous alias: n/a
   ------------------------- */
int stptiHAL_NumberOfSessions(FullHandle_t DeviceHandle);


/* previous alias: n/a
   returns (void *) TCSessionInfo_t *
   Don't wish to have to define TCSessionInfo_t * here
   Returns NULL if StreamID not currently in use
 */
void *stptiHAL_GetSessionPtrForStreamId(FullHandle_t PTIHandle, STPTI_StreamID_t StreamID);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_DescramblerAllocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...InitialiseDescrambler
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerInitialise(FullHandle_t DescramblerHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...InitialisePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterInitialisePES(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...InitialisePESStreamIdFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterInitialisePESStreamId(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 1 (alias InitialiseSF) 3 4
      description: As per STPTI API
   previous alias: ...InitialiseSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterInitialiseSection(FullHandle_t DeviceHandle, U32 SFIdent, STPTI_Filter_t FilterHandle, STPTI_FilterType_t FilterType);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotAllocate
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...InitialiseSlot
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotInitialise(FullHandle_t DeviceHandle, STPTI_SlotData_t * SlotData, U32 SlotIdent, STPTI_Slot_t SlotHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterAllocate
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...InitialiseTSFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterInitialiseTransport(FullHandle_t DeviceHandle, U32 FilterIdent, STPTI_Filter_t FilterHandle);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_ModifySyncLockAndDrop
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...ModifySyncLockAndDrop
   ------------------------- */
ST_ErrorCode_t stptiHAL_ModifySyncLockAndDrop(FullHandle_t DeviceHandle, U8 SyncLock, U8 SyncDrop);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferRead
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ReadBuffer
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferRead(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferReadNBytes
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...ReadNBytes
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferReadNBytes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy);
                                 

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferReadTSPacket
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...ReadTSPacket
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferReadTSPacket(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferUnLink
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...RemoveBufferDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferUnLink(FullHandle_t BufferHandle);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotUnLink STPTI_BufferDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...RemoveGeneralDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotUnLinkGeneral(FullHandle_t SlotHandle, FullHandle_t BufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotUnLinkRecordBuffer
              HAL: 4
      description: As per STPTI API
   previous alias: 
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotUnLinkRecord(FullHandle_t SlotHandle, FullHandle_t BufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotUnLink
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...RemoveSlotDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotUnLinkDirect(FullHandle_t SlotHandle);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by:STPTI_DisableErrorEvent STPTI_EnableErrorEvent
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetInterruptMask
   ------------------------- */
ST_ErrorCode_t stptiHAL_ErrorEventModifyState(FullHandle_t DeviceHandle, STPTI_Event_t EventName, BOOL Enable);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_AlternateOutputSetDefaultAction
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetDefaultAlternateOutputAction
   ------------------------- */
ST_ErrorCode_t stptiHAL_AlternateOutputSetDefaultAction(FullHandle_t SessionHandle, STPTI_AlternateOutputType_t Method);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_DescramblerSet
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetDescramblerKey
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerSet(FullHandle_t DescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data);


/* ------------------------- 
           Status: 
    Referenced by: STPTI_DescramblerSetType
              HAL: 4
      description: As per STPTI API
   previous alias: N/A
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerSetType(FullHandle_t DescramblerHandle, STPTI_DescramblerType_t DescramblerType);


/* ------------------------- 
           Status: 
    Referenced by: stptiHAL_DescramblerSetSVP
              HAL: 4
      description: As per STPTI API
   previous alias: N/A
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerSetSVP(FullHandle_t DescramblerHandle, BOOL Clear_SCB, STPTI_NSAMode_t mode);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferLinkToCdFifo
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetUpBufferDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferLinkToCdFifo(FullHandle_t BufferHandle, STPTI_DMAParams_t * CdFifoParams);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotLinkToBuffer
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetUpGeneralDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotLinkToBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle, BOOL DMAExists);


/* ------------------------- 
           Status: 
    Referenced by: STPTI_SlotLinkToRecordBuffer
              HAL: 4
      description: As per STPTI API
   previous alias: N/A
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotLinkToRecordBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle, BOOL DMAExists, BOOL DescrambleTS);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpPESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterSetPES(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpPESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterSetPESStreamId(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetUpSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterSetSection(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData);

/* ------------------------- 
           Status: 
    Referenced by: stptiHAL_ObjectEnableDisableFeatures
              HAL: 4
      description: As per STPTI API
   previous alias: N/A
   ------------------------- */

ST_ErrorCode_t stptiHAL_ObjectEnableDisableFeatures(FullHandle_t FullObjectHandle, STPTI_FeatureInfo_t FeatureInfo, BOOL Enable);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterSet
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SetUpTSFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterSetTransport(FullHandle_t FilterHandle, STPTI_FilterData_t * FilterData_p);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotLinkToCdFifo
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetUpSlotDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotLinkToCdFifo(FullHandle_t SlotHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_UserDataWrite STPTI_UserDataBlockMove STPTI_UserDataCircularSetup STPTI_BufferLinkToCdFifo
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SetUpUserDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_UserDataWrite(FullHandle_t DeviceHandle, U8 *DataPtr, size_t DataLength, U8 *BufferPtr_p, U8 *BufferEnd_p, U8 **NextPtr_p, STPTI_DMAParams_t * CdFifoParams, BOOL DMASignal, U32 DMASetup, U32 *DirectDmaReturn);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_UserDataCircularAppend
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...AppendUserDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_UserDataCircularAppend(FullHandle_t DeviceHandle, size_t DataLength, U8 **NextPtr_p, STPTI_DMAParams_t * DMAParams);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotClearPid STPTI_SlotSetPid
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SlotClearPid
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotClearPid(FullHandle_t SlotHandle, BOOL WindbackDMA, U32 AlternativeTCIdentIfPCR);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotSetCCControl
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SlotSetCCControl
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotSetCCControl(FullHandle_t FullSlotHandle, BOOL SetOnOff);

/* ------------------------- 
           Status: ADDED
    Referenced by: STPTI_SlotSetCorruptionParams
              HAL: 4
      description: As per STPTI API
   previous alias: none
   ------------------------- */
void stptiHAL_SlotSetCorruptionParams( FullHandle_t FullSlotHandle, U8 Offset, U8 Value );


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SlotSetPid
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotSetPid(FullHandle_t SlotHandle, U16 Pid, U16 MapPid);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotState
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...SlotState
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotState(FullHandle_t FullSlotHandle, U32 *SlotCount_p, STPTI_ScrambleState_t * ScrambleState_p, STPTI_Pid_t * Pid_p, U32 RecurseDepth);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_UserDataSynchronize
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...SyncUserDirectDMA
   ------------------------- */
ST_ErrorCode_t stptiHAL_UserDataSynchronize(FullHandle_t DeviceHandle, STPTI_DMAParams_t * DMAParams);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_Term
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...Term
   ------------------------- */
ST_ErrorCode_t stptiHAL_Term(FullHandle_t DeviceHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_DescramblerDeallocate STPTI_Close STPTI_Term
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...TerminateDescrambler
   ------------------------- */
ST_ErrorCode_t stptiHAL_DescramblerDeallocate(FullHandle_t DescramblerHandle);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...TerminatePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDeallocatePES(FullHandle_t DeviceHandle, U32 FilterIdent);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...TerminatePESFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDeallocatePESStreamId(FullHandle_t DeviceHandle, U32 FilterIdent);
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDeallocate STPTI_Close STPTI_Term
              HAL: 1 (alias stpti_TC1TerminateSF) 3 4
      description: As per STPTI API
   previous alias: ...TerminateSectionFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDeallocateSection(FullHandle_t DeviceHandle, U32 FilterIdent);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_FilterDeallocate STPTI_Close STPTI_Term
              HAL: 3 4
      description: As per STPTI API
   previous alias: ...TerminateTSFilter
   ------------------------- */
ST_ErrorCode_t stptiHAL_FilterDeallocateTransport(FullHandle_t DeviceHandle, U32 FilterIdent);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...TerminateSlot
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotDeallocate(FullHandle_t DeviceHandle, U32 SlotIdent);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_GetInputPacketCount
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetInputPacketCount
   ------------------------- */
ST_ErrorCode_t stptiHAL_GetInputPacketCount(FullHandle_t DeviceHandle, U16 *Count);


ST_ErrorCode_t stptiHAL_GetPtiPacketCount(FullHandle_t DeviceHandle, U32 *Count_p);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotPacketCount
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...GetSlotPacketCount
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotPacketCount(FullHandle_t SlotHandle, U16 *Count);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_UserDataRemaining
              HAL: 1 3 4
      description: As per STPTI API
   previous alias: ...UserDataRemaining
   ------------------------- */
ST_ErrorCode_t stptiHAL_UserDataRemaining(FullHandle_t DeviceHandle, STPTI_DMAParams_t *DMAParams, U32 *UserDataRemaining);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 3 4
      description: Helper function
   previous alias: ...SlotSetPCRReturn
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotSetPCRReturn(FullHandle_t SlotHandle);


/* ------------------------- 
           Status: NOT YET CONVERTED 
    Referenced by: stpti_InterruptTask stpti_TC1ReadSection
              HAL: 1
      description: Helper function.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_SkipInvalidSections(FullHandle_t BufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_SignalAssociateBuffer
              HAL: 3 4
      description: Helper function.
   previous alias: ...SignalRenable
   ------------------------- */
void stptiHAL_SignalRenable(FullHandle_t FullBufferHandle);


/* ------------------------- Newly abstracted functions ------------------------- */

#ifdef STPTI_GPDMA_SUPPORT
/* ------------------------- 
           Status: CONVERTED 
    Referenced by: stptiHAL_BufferSetGPDMAParams
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferSetGPDMAParams(FullHandle_t FullBufferHandle, STPTI_GPDMAParams_t *GPDMAParams);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_BufferClearGPDMAParams
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferClearGPDMAParams(FullHandle_t FullBufferHandle);
#endif

/* ------------------------- 
    Referenced by: STPTI_SignalAssociateBuffer
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
void stptiHAL_SignalSetup(FullHandle_t FullBufferHandle);
/* ------------------------- 
    Referenced by: stpti_SignalDisassociateBuffer
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
void stptiHAL_SignalUnsetup(FullHandle_t FullBufferHandle);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_EnableScramblingEvents
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_EnableScramblingEvents(FullHandle_t FullSlotHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_DisableScramblingEvents
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_DisableScramblingEvents(FullHandle_t FullSlotHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_GetPresentationSTC
              HAL: 4
      description: Helper function.
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_GetPresentationSTC(FullHandle_t DeviceHandle, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: STPTI_HardwareFIFOLevels
              HAL: 4
      description: As per STPTI API
   previous alias: ...n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_HardwareFIFOLevels(FullHandle_t DeviceHandle, BOOL *Overflow, U16 *InputLevel, U16 *AltLevel, U16 *HeaderLevel);

/* ------------------------- 
           Status: CONVERTED 
    Referenced by: stpti_Slot_SetPid
              HAL: 4
      description: As per STPTI API
   previous alias: ...n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_SlotEnableIndexing(FullHandle_t FullSlotHandle);



#if defined( STPTI_DEBUG_SUPPORT ) || defined( ST_OSLINUX )

int stptiHAL_read_proctcdata( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_proc_cam( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_procregs( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_procglobal( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_procfilters( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_procmaininfo( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_proc_sess( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_proc_globaltcdmacfg( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_proc_lut( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_proc_tcparams( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_proc_objects( char *buf, char **start, off_t offset, int count, int *eof, void *data );
int stptiHAL_read_procmisc( char *buf, char **start, off_t offset, int count, int *eof, void *data );

#endif /* STPTI_DEBUG_SUPPORT */


/* ------------------------- 
           Status: NOT YET IMPLEMENTED, NEW FUNCTIONALITY
    Referenced by: STPTI_BufferSetOverflowControl  
              HAL: 4
      description:  FDMA helper function
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferSetOverflowControl(FullHandle_t FullBufferHandle, BOOL OverflowControl);


/* ------------------------- 
           Status: NOT YET IMPLEMENTED, NEW FUNCTIONALITY
    Referenced by: STPTI_BufferFlush  
              HAL: 4
      description: FDMA helper function 
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferFlushManual(STPTI_Buffer_t BufferHandle);


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_BufferSetReadPointer 
              HAL: 4
      description: FDMA helper function
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferSetReadPointer(FullHandle_t FullBufferHandle, void *Read_p);


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_BufferGetWritePointer  
              HAL: 4
      description: FDMA helper function
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferGetWritePointer(FullHandle_t FullBufferHandle, void **Write_p);


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_BufferGetFreeLength
              HAL: 4
      description: This function is used to assess the free length remaining in a PTI buffer (the write pointer 
                   is used - not the Qwrite pointer) This function is useful for flow control in pull mode.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferGetFreeLength(FullHandle_t FullBufferHandle, U32 *FreeLength_p);

/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_BufferPacketCount
              HAL: 4
      description: This function is used to get the DMA packet counter
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferPacketCount(FullHandle_t FullBufferHandle, U32 * PacketCount_p);

/* ------------------------- Validation functions --------------------- */


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: stptiValidate_FilterSetParams
              HAL: 4
      description: checks if STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_ONE_SHOT is supported.
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stpti_HAL_Validate_FilterOneShotSupported( FullHandle_t FullFilterHandle );


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: stptiValidate_FilterSetParams
              HAL: 4
      description: checks if filter setup is correct for operating in not match mode
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stpti_HAL_Validate_FilterNotMatchModeSetup( FullHandle_t FullFilterHandle );



/* ------------------------- Helper functions ------------------------- */




/* --- TC Loader related access functions --- */

typedef enum stptiHAL_LoaderConfig_e     /* NEW FUNCTIONALITY */
{
    ALTERNATEOUTPUTSUPPORT,
    AUTOMATICSECTIONFILTERING,
    CAROUSELOUTPUTSUPPORT,
    CODESIZE,
    CODESTART,
    DMACONFIGSTART,
    DATASTART,
    DESCRAMBLERKEYSSTART,
    ECMSTART,
    EMMSTART,
    GLOBALDATASTART,
    INDEXDATASTART,
    INDEXSUPPORT,
    INPUTPACKETCOUNTSUPPORT,
    INTERRUPTDMACONFIGSTART,
    INTERRUPTDATASTART,
    LOOKUPTABLESTART,
    MAININFOSTART,
    MANUALSECTIONFILTERING,
    MATCHACTIONSUPPORT,
    NUMBERCAROUSELS,
    NUMBERDMAS,
    NUMBERDESCRAMBLERKEYS,
    NUMBERECMFILTERS,
    NUMBEREMMFILTERS,
    NUMBERINDEXS,
    NUMBERPESFILTERS,
    NUMBERSECTIONFILTERS,
    NUMBERSLOTS,
    NUMBERTRANSPORTFILTERS,
    PESFILTERSTART,
    PIDWILDCARDINGSUPPORT,
    RAWSTREAMDESCRAMBLING,
    SFACTIONENABLES_HI,
    SFACTIONENABLES_LO,
    SFACTIONMASKS_HI,
    SFACTIONMASKS_LO,
    SFACTIONTABLESTART,
    SFSTATUSSTART,
    SIGNALEVERYTRANSPORTPACKET,
    STATUSBLOCKSTART,
    SUBSTITUTEDATASTART,
    TRANSPORTFILTERSTART,
    UNMATCHEDMAININFOSTART
} stptiHAL_LoaderConfig_t;



/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: stpti core  
              HAL: 4
      description: Get specific loader configuration via a neutral interface
   previous alias: n/a
   ------------------------- */
U32 stptiHAL_LoaderGetConfig(FullHandle_t DeviceHandle, stptiHAL_LoaderConfig_t LoaderConfig);


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_Init
              HAL: 4
      description: Get all loader configuration details
   previous alias: n/a
   ------------------------- */
ST_ErrorCode_t stptiHAL_LoaderGetCapability(FullHandle_t FullHandle, STPTI_Capability_t *DeviceCapability);


/* ------------------------- 
           Status: NEW FUNCTIONALITY
    Referenced by: STPTI_Init
              HAL:  4
      description: Get the HAL (pti cell) this was built for
   previous alias: n/a
   ------------------------- */
STPTI_Device_t stptiHAL_Device( void );



/* Local PTI HAL functions - The criteria is that they are not called from stpti
  core functions, only its specific HAL. Described here so as to present a unified
  name to functions that may appear in various HAL implementations. Note they have
  the prefix 'stptiHelper_' rather than 'stptiHAL_' */

/* ------------------------- 
           Status: CONVERTED 
    Referenced by:  
              HAL: 1
      description: Helper function.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHelper_SkipPartialPESData(FullHandle_t FullBufferHandle);




/* ------------------------- 
           Status: CONVERTED 
    Referenced by:  
              HAL: 1
      description: Helper function.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHelper_SkipPartialPESData(FullHandle_t FullBufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by:  
              HAL: 1
      description: Helper function.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHelper_SkipTSPacket(FullHandle_t FullBufferHandle);


/* ------------------------- 
           Status: CONVERTED 
    Referenced by: n/a
              HAL: 3
      description: Enable/Disable what interrupts are raised
   previous alias: ...SlotSetGlobalInterruptMask
   ------------------------- */
ST_ErrorCode_t stptiHelper_SlotSetGlobalInterruptMask(FullHandle_t SlotHandle, U32 Flags, BOOL Enable);


/* ------------------------- 
           Status: NOT YET CONVERTED  
    Referenced by: STPTI_BufferExtractData STPTI_BufferReadSection STPTI_BufferRead STPTI_BufferReadNBytes STPTI_BufferReadPartialPesPacket STPTI_BufferReadPes STPTI_BufferReadTSPacket  
              HAL: 1 (alais CircToCircCopy) 3 (alias stpti_CircToCircCopy) 4
      description: Helper function
   previous alias: ...
   ------------------------- */
U32 stptiHelper_CircToCircCopy(U32 SrcSize0, U8 *Src0_p, U32 SrcSize1, U8 *Src1_p, U32 *DestSize0, U8 **Dest0_p, U32 *DestSize1, U8 **Dest1_p, STPTI_Copy_t DmaOrMemcpy, FullHandle_t FullBufferHandle );


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_SignalAssociateBuffer STPTI_BufferFlush  STPTI_BufferReadSection STPTI_BufferRead STPTI_BufferReadNBytes STPTI_BufferReadPartialPesPacket STPTI_BufferReadPes STPTI_BufferReadTSPacket  
              HAL: 1 3 4
      description: Helper function
   previous alias: ...
   ------------------------- */
void stptiHelper_SignalBuffer(FullHandle_t BufferHandle, U32 Flags);


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_BufferFlush STPTI_BufferUnLink STPTI_SlotUnLink STPTI_BufferDeallocate STPTI_SlotDeallocate STPTI_Close STPTI_Term STPTI_HardwarePause STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 3 4
      description: Helper function
   previous alias: ...
   ------------------------- */
void stptiHelper_WaitForDMAtoComplete(FullHandle_t DeviceHandle, U32 SlotIdent);


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_FilterSetMatchAction STPTI_BufferFlush STPTI_HardwarePause STPTI_HardwareReset STPTI_UserDataSynchronize STPTI_SlotDeallocate STPTI_Close STPTI_Term
              HAL: 1 (alias TC1_WaitPacketTime) 3 (alias stpti_TC3WaitPacketTime) 4
      description: Helper function
   previous alias: ...
   ------------------------- */
void stptiHelper_WaitPacketTime(void);


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_BufferReadSection STPTI_BufferReadPes STPTI_BufferReadTSPacket  
              HAL: 3 4
      description: Helper function
   previous alias: ...
   ------------------------- */
void stptiHelper_SignalEnable(FullHandle_t FullBufferHandle, U16 MultiSize);

/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_BufferReadSection STPTI_BufferReadPes STPTI_BufferReadTSPacket  
              HAL: 3 4
      description: Helper function
   previous alias: ...
   ------------------------- */

void stptiHelper_BufferLevelSignalEnable(FullHandle_t FullBufferHandle, U32 BufferLevelThreshold);
/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_BufferLevelSignalEnable()  
              HAL: 4
      description: Helper function
   previous alias: ...
   ------------------------- */

void stptiHelper_BufferLevelSignalDisable(FullHandle_t FullBufferHandle);
/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_BufferLevelSignalDisable()
              HAL: 4
      description: Helper function
   previous alias: ...
   ------------------------- */

void stptiHelper_SignalEnableIfDataWaiting(FullHandle_t FullBufferHandle, U16 MultiSize);


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_BufferFlush STPTI_HardwareResume STPTI_HardwarePause STPTI_HardwareReset STPTI_SlotClearPid STPTI_SlotSetPid
              HAL: 1 3 4
      description: Helper function
   previous alias: ...
   ------------------------- */
void stptiHelper_SetLookupTableEntry(FullHandle_t DeviceHandle, U16 slot, U16 Pid);


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_UserDataSynchronize
              HAL: 1 (alias TC1_WaitPacketTimeDeschedule) 3 (alias stpti_TC3WaitPacketTimeDeschedule) 4
      description: Helper function
   previous alias: ...
   ------------------------- */
void stptiHelper_WaitPacketTimeDeschedule(void);


/* ------------------------- 
           Status: NOT YET CONVERTED
    Referenced by: STPTI_HardwarePause STPTI_HardwareReset STPTI_SlotSetPid STPTI_SlotClearPid
              HAL: 1 (alias TC1_WindBackDMA) 3 (alias stpti_TC3WindBackDMA) 4
      description: Helper function.
   previous alias: ...
   ------------------------- */
void stptiHelper_WindBackDMA(FullHandle_t DeviceHandle, U32 SlotIdent);
                          
/* ------------------------- 
           Status: CONVERTED, DUPLICATION with core level function
    Referenced by: STPTI_Init
              HAL: 3 4
      description: Helper function.
   previous alias: ...
   ------------------------- */
size_t stptiHelper_BufferSizeAdjust(size_t Size);


/* ------------------------- 
           Status: CONVERTED, DUPLICATION with core level function
    Referenced by: STPTI_Init
              HAL: 3 4
      description: Helper function.
   previous alias: ...
   ------------------------- */
void *stptiHelper_BufferBaseAdjust(void *Base_p);


/* ------------------------- 
           Status: CONVERTED
    Referenced by: STPTI_Init
              HAL: 3 4
      description: Helper function.
   previous alias: ...ClearLookupTable
   ------------------------- */
void stptiHelper_ClearLookupTable(FullHandle_t DeviceHandle);


/* -------------------------  ------------------------- */

ST_ErrorCode_t stptiHAL_PrintDebug(FullHandle_t DeviceHandle);

ST_ErrorCode_t stptiHAL_DumpInputTS(FullHandle_t FullBufferHandle, U16 bytes_to_capture_per_packet);

#endif  /* __PTI_HAL_H */




/* Shell (csh) line noise to HTML-ize the functions so that they can be
  imported into the HAL API document with only minor manual intervention,
  javadoc it ain't. To generate the html, at the shell prompt type: make docs.
   The code below gets preprocessed to stdout, piped to a file and then 
  csh (or bash) then runs the resulting script.
*/


#else   /* STPTI_MAKE_DOCS */

rm -f pti_hal.html; cat pti_hal.h | sed "s/\n/ /g;s/Status:/<FONT COLOR="blue">Status:<\/FONT>/g;" | \\

sed "s/NEW FUNCTIONALITY/<FONT COLOR="green">NEW FUNCTIONALITY<\/FONT>/g;s/NOT YET IMPLEMENTED/<FONT COLOR="red">NOT YET IMPLEMENTED<\/FONT>/g;" | \\

sed "s/NOT YET CONVERTED/<FONT COLOR="orange">NOT YET CONVERTED<\/FONT>/g;" | \\

sed "s/#ifdef/<H2>/g;s/SUPPORT/SUPPORT<\/H2>/g;" | \\

sed "s/#endif/<P><HR>/g;" | \\

sed "s/DUPLICATION /<I><FONT COLOR="red">DUPLICATION <\/FONT><\/I>/g;" | \\

sed "s/\/\* -------------------------/<HR><P><B><\/TT>/g;s/------------------------- \*\//<\/B><TT><P>/g;s/HAL:/<BR><FONT COLOR="blue">HAL:<\/FONT>/g;" | \\

sed "s/Referenced by:/<\/FONT><BR><FONT COLOR="blue">Referenced by:<\/FONT>/g;s/previous alias:/<BR><FONT COLOR="blue">Previous alias:<\/FONT>/g;" | \\

sed "s/#/<BR>#/g;s/\/\*/<BR><BR>\/\*/g;s/\*\//\*\/<BR>/g;s/description:/<BR><FONT COLOR="blue">Description:<\/FONT>/g;" > pti_hal.html

#endif  /* STPTI_MAKE_DOCS */


/* EOF */
