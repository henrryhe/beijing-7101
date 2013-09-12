/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005-2007.  All rights reserved.

   File Name: validate.c
 Description: validate core function parameters
      Author: GJP (2003)

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

#include "stcommon.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stdevice.h"
#include "stpti.h"
#include "sttbx.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"

#include "validate.h"

#include "memget.h"

/* ------------------------------------------------------------------------- */

/*
    Until STFDMA supports copies into linux userspace buffers, we have to return
    an error if ST_OSLINUX is defined, and a function attempts to copy data using
    FDMA.
*/

#define STFDMA_SUPPORTS_LINUX_USERSPACE_TRANSFERS            FALSE

#if defined( ST_OSLINUX )

    #define CHECK_COPY_METHOD( _method )                                                                \
                                                                                                        \
    if( _method == STPTI_COPY_TRANSFER_BY_FDMA && STFDMA_SUPPORTS_LINUX_USERSPACE_TRANSFERS == FALSE )  \
        return STPTI_ERROR_FUNCTION_NOT_SUPPORTED

#else

    #define CHECK_COPY_METHOD( _method )

#endif


/* ------------------------------------------------------------------------- */


#ifdef STPTI_NO_PARAMETER_CHECK /* Stub out functions and return a 'pass' (ST_NO_ERROR) for the validation */

 #define STPTI_VALIDATE_RETURN_NO_ERROR     {return(ST_NO_ERROR);}


/* pti_int.c */

ST_ErrorCode_t stptiValidate_IntBufferHandle(FullHandle_t BufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR


/* stpti.c */

ST_ErrorCode_t stptiValidate_DescramblerAllocate(FullHandle_t FullSessionHandle, STPTI_Descrambler_t *DescramblerObject, STPTI_DescramblerType_t DescramblerType)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferSetMultiPacket(FullHandle_t FullBufferHandle, U32 NumberOfPacketsInMultiPacket)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferLevelSignalEnable( FullHandle_t FullBufferHandle, U32 BufferLevelThreshold )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferLevelSignalDisable( FullHandle_t FullBufferHandle )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferExtractData( FullHandle_t FullBufferHandle, U32 Offset, U32 NumBytesToExtract,  U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferExtractTSHeaderData(FullHandle_t FullBufferHandle, U32 *TSHeader)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferGetFreeLength(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferGetWritePointer(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferPacketCount( FullHandle_t FullBufferHandle, U32 * Count_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferRead(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferSetOverflowControl(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferSetReadPointer(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferTestForData(FullHandle_t FullBufferHandle, U32 *BytesInBuffer)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_DescramblerDeallocate(FullHandle_t FullDescramblerHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_Descrambler_AssociatePid( FullHandle_t DescramblerHandle )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_DescramblerAssociate(FullHandle_t FullDescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_DescramblerDisassociate(FullHandle_t FullDescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_DescramblerSet(FullHandle_t FullDescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_DescramblerSetType(FullHandle_t FullDescramblerHandle, STPTI_DescramblerType_t DescramblerType)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_DescramblerSetSVP(FullHandle_t FullDescramblerHandle, STPTI_NSAMode_t mode)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_EnableDisableScramblingEvents(FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FilterSetMatchAction(FullHandle_t FullFilterHandle, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FilterHandleCheck( FullHandle_t FullFilterHandle )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t *DeviceCapability)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetPacketArrivalTime(FullHandle_t DeviceHandle, STPTI_TimeStamp_t *ArrivalTime, U16 *ArrivalTimeExtension)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetCurrentPTITimer(ST_DeviceName_t DeviceName, STPTI_TimeStamp_t *TimeStamp)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferLinkToCdFifo(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SetStreamID(ST_DeviceName_t DeviceName, STPTI_StreamID_t StreamID)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SetDiscardParams(ST_DeviceName_t DeviceName, U8 NumberOfDiscardBytes)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotSetCorruptionParams( FullHandle_t FullSlotHandle, U8 Offset)
STPTI_VALIDATE_RETURN_NO_ERROR


ST_ErrorCode_t stptiValidate_SlotSetCCControl(FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_PidQuery(ST_DeviceName_t DeviceName, STPTI_Pid_t Pid, STPTI_Slot_t *Slot_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotPacketCount(FullHandle_t FullSlotHandle, U16 *Count)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_UserDataBlockMove(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_UserDataCircularAppend(U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_UserDataCircularSetup(U8 *Buffer_p, U32 BufferSize, U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_UserDataSynchronize(STPTI_DMAParams_t *DMAParams_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_UserDataWrite(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_UserDataRemaining(STPTI_DMAParams_t *DMAParams_p, U32 *UserDataRemaining)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotLinkToCdFifo(FullHandle_t FullSlotHandle, STPTI_DMAParams_t *CdFifoParams)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferReadNBytes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferExtractSectionData( FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U16 MaxLengthofFilterList, U16 *NumOfFilterMatches_p, BOOL *CRCValid_p, U32 *SectionHeader_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferExtractPartialPesPacketData( FullHandle_t FullBufferHandle, BOOL *PayloadUnitStart_p, BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U16 *DataLength_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferExtractPesPacketData( FullHandle_t FullBufferHandle, U8 *PesFlags_p, U8 *TrickModeFlags_p, U32 *PESPacketLength_p, STPTI_TimeStamp_t *PTSValue_p, STPTI_TimeStamp_t *DTSValue_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferReadPartialPesPacket( FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, BOOL *PayloadUnitStart_p, BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferReadPes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FiltersFlush(FullHandle_t FullBufferHandle, STPTI_Filter_t Filters[], U16 NumOfFiltersToFlush)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[], U16 NumOfFiltersToDisable, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotDescramblingControl(FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotSetAlternateOutputAction(FullHandle_t FullSlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotQuery(FullHandle_t FullSlotHandle, BOOL *PacketsSeen, BOOL *TransportScrambledPacketsSeen, BOOL *PESScrambledPacketsSeen, STPTI_Pid_t *Pid)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_AlternateOutputSetDefaultAction(FullHandle_t SessionHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
STPTI_VALIDATE_RETURN_NO_ERROR

#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferClearGPDMAParams(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferSetGPDMAParams(FullHandle_t FullBufferHandle, STPTI_GPDMAParams_t GPDMAParams)
STPTI_VALIDATE_RETURN_NO_ERROR
#endif

#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferClearPCPParams(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferSetPCPParams(FullHandle_t FullBufferHandle, STPTI_PCPParams_t PCPParams)
STPTI_VALIDATE_RETURN_NO_ERROR
#endif

/* stpti_fe.c */
#if defined (STPTI_FRONTEND_HYBRID)
ST_ErrorCode_t stptiValidate_FrontendAllocate( FullHandle_t FullSessionHandle, STPTI_Frontend_t *FrontendHandle_p, STPTI_FrontendType_t FrontendType)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FrontendLinkToPTI( FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FrontendSetParams( FullHandle_t FullFrontendHandle, STPTI_FrontendParams_t * FrontendParams_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FrontendGetParams( FullHandle_t FullFrontendHandle, STPTI_FrontendType_t * FrontendType_p, STPTI_FrontendParams_t * FrontendParams_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FrontendGetStatus( FullHandle_t FullFrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FrontendInjectData( FullHandle_t FullFrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FrontendHandle(FullHandle_t FullFrontendHandle)
STPTI_VALIDATE_RETURN_NO_ERROR
#endif /* #if defined (STPTI_FRONTEND_HYBRID) */

/* stpti_ba.c */

ST_ErrorCode_t stptiValidate_Init(ST_DeviceName_t DeviceName, const STPTI_InitParams_t *InitParams)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferAllocate(FullHandle_t FullSessionHandle, U32 RequiredSize, U32 NumberOfPacketsInMultiPacket, STPTI_Buffer_t *BufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FilterAllocate(FullHandle_t FullSessionHandle, STPTI_FilterType_t FilterType, STPTI_Filter_t *FilterObject)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotAllocate(FullHandle_t FullSessionHandle, STPTI_Slot_t *SlotHandle, STPTI_SlotData_t *SlotData)
STPTI_VALIDATE_RETURN_NO_ERROR


ST_ErrorCode_t stptiValidate_BufferFlush(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferReadSection(FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U32 MaxLengthofFilterList, U32 *NumOfFilterMatches_p, BOOL *CRCValid_p, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_Close(FullHandle_t FullSessionHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferDeallocate(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FilterDeallocate(FullHandle_t FullFilterHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotDeallocate(FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FilterAssociate(FullHandle_t FullFilterHandle, FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_FilterSetParams( FullHandle_t FullFilterHandle, STPTI_FilterData_t *FilterData_p )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotLinkToBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotLinkToRecordBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GetBuffersFromSlot(FullHandle_t FullSlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotState(FullHandle_t FullSlotHandle, U32 *SlotCount_p, STPTI_ScrambleState_t *ScrambleState_p, STPTI_Pid_t *Pid_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_Open(ST_DeviceName_t DeviceName, const STPTI_OpenParams_t *OpenParams, STPTI_Handle_t *Handle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SignalAllocate(FullHandle_t FullSessionHandle, STPTI_Signal_t *SignalHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SignalAssociateBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SignalDeallocate(FullHandle_t FullSignalHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SignalDisassociateBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SignalAbort(FullHandle_t FullSignalHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_Descrambler_AssociateSlot(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotClearPid(FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotSetPid(FullHandle_t FullSlotHandle, STPTI_Pid_t Pid)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotUnLink(FullHandle_t FullSlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferUnLink(FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferReadTSPacket(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
STPTI_VALIDATE_RETURN_NO_ERROR


/* pti_indx.c */

#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode_t stptiValidate_IndexAllocate( FullHandle_t FullSessionHandle, STPTI_Index_t *IndexHandle_p)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexAssociate( FullHandle_t FullIndexHandle, STPTI_SlotOrPid_t SlotOrPid )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexDeallocate( FullHandle_t FullIndexHandle )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexStartStop( FullHandle_t DeviceHandle    )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexSet( FullHandle_t FullIndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexAssociateSlot( FullHandle_t IndexHandle, FullHandle_t SlotHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexAssociatePid( FullHandle_t IndexHandle     )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexChain        ( FullHandle_t *IndexHandles, U32 NumberOfHandles )
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_SlotDisassociateIndex(FullHandle_t SlotHandle, FullHandle_t IndexHandle, BOOL PreChecked)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_IndexDisassociate( FullHandle_t FullIndexHandle )
STPTI_VALIDATE_RETURN_NO_ERROR

#endif


/* pti_car.c */

#ifdef STPTI_CAROUSEL_SUPPORT
ST_ErrorCode_t stptiValidate_CarouselAllocate(FullHandle_t FullSessionHandle, STPTI_Carousel_t *Carousel)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselLinkToBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselEntryAllocate(FullHandle_t FullSessionHandle, STPTI_CarouselEntry_t *CarouselEntryHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselInsertEntry(FullHandle_t FullCarouselHandle, STPTI_AlternateOutputTag_t AlternateOutputTag, STPTI_InjectionType_t InjectionType, FullHandle_t FullEntryHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselInsertTimedEntry(FullHandle_t FullCarouselHandle, U8 *TSPacket_p, U8 CCValue, STPTI_InjectionType_t InjectionType, STPTI_TimeStamp_t MinOutputTime, STPTI_TimeStamp_t MaxOutputTime, BOOL EventOnOutput, BOOL EventOnTimeout, U32 PrivateData, FullHandle_t FullEntryHandle, U8 FromByte)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GeneralCarouselHandleCheck(FullHandle_t FullCarouselHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GeneralCarouselAndEntryHandleCheck(FullHandle_t FullCarouselHandle, FullHandle_t FullEntryHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselSetAllowOutput(FullHandle_t FullCarouselHandle, STPTI_AllowOutput_t OutputAllowed)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselUnlinkBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_GeneralCarouselEntryHandleCheck(FullHandle_t FullCarouselEntryHandle)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_CarouselEntryLoad(FullHandle_t FullEntryHandle, STPTI_TSPacket_t Packet, U8 FromByte)
STPTI_VALIDATE_RETURN_NO_ERROR

ST_ErrorCode_t stptiValidate_BufferLevelChangeEvent(FullHandle_t FullBufferHandle, BOOL Enable)
STPTI_VALIDATE_RETURN_NO_ERROR

#endif

ST_ErrorCode_t stptiValidate_ObjectEnableFeatures(FullHandle_t FullObjectHandle, STPTI_FeatureInfo_t FeatureInfo)
STPTI_VALIDATE_RETURN_NO_ERROR

/* ------------------------------------------------------------------------- */


#else   /* STPTI_NO_PARAMETER_CHECK. Perform real tests of the parameters    */


extern Driver_t STPTI_Driver;


/* ------------------------------------------------------------------------- */

#ifndef STPTI_NO_INDEX_SUPPORT


ST_ErrorCode_t stptiValidate_IndexAssociate( FullHandle_t FullIndexHandle, STPTI_SlotOrPid_t SlotOrPid )
{
    ST_ErrorCode_t Error;
    FullHandle_t FullSlotHandle;


    Error = stpti_IndexHandleCheck( FullIndexHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( stptiMemGet_Device(FullIndexHandle)->IndexAssociationType == STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
    {
        FullSlotHandle.word = SlotOrPid.Slot;

        Error = stpti_SlotHandleCheck( FullSlotHandle );
    }
    else
    {
        if (SlotOrPid.Pid > DVB_MAXIMUM_PID_VALUE)
        {
            Error = STPTI_ERROR_INVALID_PID;
        }
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexAllocate( FullHandle_t FullSessionHandle, STPTI_Index_t *IndexHandle_p)
{
    if (IndexHandle_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    return( stpti_SessionHandleCheck( FullSessionHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexDeallocate( FullHandle_t FullIndexHandle )
{
    return( stpti_IndexHandleCheck( FullIndexHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexStartStop( FullHandle_t DeviceHandle )
{
    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexSet( FullHandle_t FullIndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode )
{
#if !defined (STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
    /* We don't support MPEG Start Code Detection on any other platform than PTI4SL */
    if ( IndexMask.s.MPEGStartCode == 1 )
    {
        return( ST_ERROR_FEATURE_NOT_SUPPORTED );
    }
#endif

    return( stpti_IndexHandleCheck( FullIndexHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexAssociateSlot(FullHandle_t IndexHandle, FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t IndexListHandle;

    /* Index & Slot MUST be on same device and SHOULD be in same session */

    if (IndexHandle.member.Device != SlotHandle.member.Device)
        return( STPTI_ERROR_NOT_ON_SAME_DEVICE );

#ifdef STPTI_CHECK_SAME_SESSION
    if ( IndexHandle.member.Session != SlotHandle.member.Session )
        return( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
#endif

    /* A slot can only associate with one Index ( but a Index can associate with many slots!!)
       ...check that this specific slot does not already have an associated Index */

    IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;

    if ( IndexListHandle.word != STPTI_NullHandle() )
        Error = STPTI_ERROR_INDEX_SLOT_ALREADY_ASSOCIATED;

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexAssociatePid( FullHandle_t IndexHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (stptiMemGet_Index(IndexHandle))->AssociatedPid != STPTI_InvalidPid() )
        Error = STPTI_ERROR_INDEX_PID_ALREADY_ASSOCIATED;

    return( Error );
}



/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_IndexChain( FullHandle_t *IndexHandles, U32 NumberOfHandles )
{
    int i;
    ST_ErrorCode_t Error;

    if( NumberOfHandles == 0 )
        return( ST_ERROR_BAD_PARAMETER );

    if( IndexHandles == NULL )
        return( ST_ERROR_BAD_PARAMETER );

    for(i=0;i<NumberOfHandles;i++)
    {
        Error=stpti_IndexHandleCheck( IndexHandles[i] );
        if(Error != ST_NO_ERROR)
            return( Error );

        if( IndexHandles[i].member.Device != IndexHandles[i].member.Device )
            return( STPTI_ERROR_NOT_ON_SAME_DEVICE );

        if( IndexHandles[i].member.Session != IndexHandles[i].member.Session )
            return( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
    }

    return( ST_NO_ERROR );

}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotDisassociateIndex(FullHandle_t SlotHandle, FullHandle_t IndexHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;
    FullHandle_t SlotListHandle = (stptiMemGet_Index(IndexHandle))->SlotListHandle;
    U32 Instance;

    if ( PreChecked )
        return( Error );

    if ( IndexListHandle.word == STPTI_NullHandle() )
        return( STPTI_ERROR_INVALID_SLOT_HANDLE );


    if ( SlotListHandle.word == STPTI_NullHandle() )
        return( STPTI_ERROR_INVALID_SLOT_HANDLE );

    /* Check Cross associations */

    Error = stpti_GetInstanceOfObjectInList(SlotListHandle, SlotHandle.word, &Instance);

    if (Error == ST_NO_ERROR)
        Error = stpti_GetInstanceOfObjectInList(IndexListHandle, IndexHandle.word, &Instance);

    return( Error );
}


/* ------------------------------------------------------------------------- */
ST_ErrorCode_t stptiValidate_IndexDisassociate( FullHandle_t FullIndexHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_IndexHandleCheck( FullIndexHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if (stptiMemGet_Device(FullIndexHandle)->IndexAssociationType != STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS)
    {
        if ( (stptiMemGet_Index(FullIndexHandle))->AssociatedPid == STPTI_InvalidPid() )
        {
            Error = STPTI_ERROR_INDEX_NOT_ASSOCIATED;
        }
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */



/* ------------------------------------------------------------------------- */




#endif  /* STPTI_NO_INDEX_SUPPORT */


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferExtractPartialPesPacketData( FullHandle_t FullBufferHandle, BOOL *PayloadUnitStart_p,
                                                       BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U16 *DataLength_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STPTI_NO_USAGE_CHECK
    FullHandle_t SlotListHandle;
    FullHandle_t FullSlotHandle;
    U32 MaxSlotHandles;
    U32 slot;
    BOOL SignalOnEveryTransportPacket;
#endif


    if ( (PayloadUnitStart_p == NULL) ||
         (CCDiscontinuity_p  == NULL) ||
         (CCDiscontinuity_p  == NULL) ||
         (ContinuityCount_p  == NULL) ||
         (DataLength_p       == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ((stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE)
        return( STPTI_ERROR_BUFFER_NOT_LINKED );



#ifndef STPTI_NO_USAGE_CHECK

    SlotListHandle = (stptiMemGet_Buffer(FullBufferHandle))->SlotListHandle;

    if (SlotListHandle.word == STPTI_NullHandle())
        return( STPTI_ERROR_BUFFER_NOT_LINKED );

    MaxSlotHandles = stptiMemGet_List(SlotListHandle)->MaxHandles;

    /* BufferExtractPartialPesPacketData can only be called if ALL the slots feeding this buffer
     are set to 'signal-on-every-transport-packet' ... so find the slots feeding this buffer and
       abort if any are not signaling on every packet. */

    for (slot = 0; slot < MaxSlotHandles; ++slot)
    {

        FullSlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[slot];

        if (FullSlotHandle.word != STPTI_NullHandle())
        {
            SignalOnEveryTransportPacket = (stptiMemGet_Slot(FullSlotHandle))->Flags.SignalOnEveryTransportPacket;

            if (SignalOnEveryTransportPacket == FALSE)
            {
                Error = STPTI_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET;
                break;
            }
        }
    }

#endif  /* STPTI_NO_USAGE_CHECK */

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferExtractPesPacketData( FullHandle_t FullBufferHandle, U8 *PesFlags_p, U8 *TrickModeFlags_p,
                                                U32 *PESPacketLength_p, STPTI_TimeStamp_t *PTSValue_p, STPTI_TimeStamp_t *DTSValue_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ((PesFlags_p        == NULL) ||
        (TrickModeFlags_p  == NULL) ||
        (PESPacketLength_p == NULL) ||
        (PTSValue_p        == NULL) ||
        (DTSValue_p        == NULL))
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );


    return ( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_DescramblerAllocate(FullHandle_t FullSessionHandle, STPTI_Descrambler_t *DescramblerObject, STPTI_DescramblerType_t DescramblerType)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( DescramblerObject == NULL )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DVB_EAVS_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_OFB_DESCRAMBLER) &&
#if defined( STPTI_POWERKEY_SUPPORT )
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CTS_DESCRAMBLER) &&
#endif
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER) &&
#if defined( STPTI_POWERKEY_SUPPORT )
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER) &&
#endif
#if defined(ST_7100) || defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER) &&
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_LR_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_LR_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_ECB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER) &&
#endif
#endif
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_FASTI_DESCRAMBLER))
        return( STPTI_ERROR_DESCRAMBLER_TYPE_NOT_SUPPORTED );

    Error = stpti_SessionHandleCheck( FullSessionHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    switch (stptiMemGet_Device(FullSessionHandle)->TCCodes)
    {

        case    STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB:
                if ((DescramblerType != STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CBC_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_ECB_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CBC_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_OFB_DESCRAMBLER) &&
#if defined( STPTI_POWERKEY_SUPPORT )
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CTS_DESCRAMBLER) &&
#endif
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER) &&
#if defined( STPTI_POWERKEY_SUPPORT )
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER) &&
#endif
#if defined(ST_7100) || defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_DVB_EAVS_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER) &&
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_LR_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_LR_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_ECB_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER) &&
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER) &&
#endif
#endif
                    (DescramblerType != STPTI_DESCRAMBLER_TYPE_FASTI_DESCRAMBLER))
                    return( STPTI_ERROR_DESCRAMBLER_TYPE_NOT_SUPPORTED );
                break;

        default:
            return( ST_ERROR_FEATURE_NOT_SUPPORTED );
    }
    return( Error );
}


/* ------------------------------------------------------------------------- */

#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferClearGPDMAParams(FullHandle_t FullBufferHandle)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}
#endif


#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferClearPCPParams(FullHandle_t FullBufferHandle)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}
#endif

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferSetMultiPacket(FullHandle_t FullBufferHandle, U32 NumberOfPacketsInMultiPacket)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (NumberOfPacketsInMultiPacket < 1 || 0x8000 < NumberOfPacketsInMultiPacket)
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( STPTI_ERROR_INVALID_BUFFER_HANDLE );

    return( Error );
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferLevelSignalEnable( FullHandle_t FullBufferHandle, U32 BufferLevelThreshold )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );
                                    /* TC maths on Buffer Level uses middle 16 bits of address only */
                                    /* So enforce minimum of 512 bytes, maximum of 8MByte threshold */
    if((BufferLevelThreshold < 0x200) || (BufferLevelThreshold >= 0x800000))
        Error = ST_ERROR_BAD_PARAMETER;
    return(Error);
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferLevelSignalDisable( FullHandle_t FullBufferHandle )
{
    return( stpti_BufferHandleCheck( FullBufferHandle ));
}

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferExtractData( FullHandle_t FullBufferHandle, U32 Offset, U32 NumBytesToExtract,
                                                U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p,
                                                U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ((Destination0_p == NULL) ||
        (DataSize_p     == NULL))
        return( ST_ERROR_BAD_PARAMETER );

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ((DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA)   &&
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP)   &&
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY))
        return( ST_ERROR_BAD_PARAMETER );

    if ((Destination0_p < (Destination1_p+DestinationSize1)) && (Destination1_p < (Destination0_p+DestinationSize0)))
        return( ST_ERROR_BAD_PARAMETER );   /* Buffer overlap */

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t  stptiValidate_BufferExtractSectionData(FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[],
                                              U16 MaxLengthofFilterList, U16 *NumOfFilterMatches_p, BOOL *CRCValid_p,
                                              U32 *SectionHeader_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* PMC 17/12/01 Don't fail if MaxLengthofFilterList is zero as it's because we don't care ! */
    if ( ((MaxLengthofFilterList > 0) && (MatchedFilterList == NULL)) ||
          (NumOfFilterMatches_p == NULL) ||
          (CRCValid_p == NULL)           ||
          (SectionHeader_p == NULL) )
    return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );


    return( Error );
}

/* ------------------------------------------------------------------------- */



ST_ErrorCode_t stptiValidate_BufferExtractTSHeaderData(FullHandle_t FullBufferHandle, U32 *TSHeader)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (TSHeader == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferGetFreeLength(FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if(Error ==ST_NO_ERROR)
    {
        if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
            Error = STPTI_ERROR_BUFFER_NOT_LINKED;
    }
    return Error;
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferGetWritePointer(FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if(Error ==ST_NO_ERROR)
    {
        if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
            Error = STPTI_ERROR_BUFFER_NOT_LINKED;
    }
    return Error;
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferPacketCount( FullHandle_t FullBufferHandle, U32 * Count_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (Count_p == NULL)
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        Error = stpti_BufferHandleCheck( FullBufferHandle );
        if ( Error == ST_NO_ERROR )
        {
            if ((stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE)
            {
                Error = STPTI_ERROR_BUFFER_NOT_LINKED;
            }
        }
    }

    return Error;
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferRead(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0,
                                U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (DestinationSize0 != 0 && Destination0_p == NULL) ||
         (DestinationSize1 != 0 && Destination1_p == NULL) ||
         (DataSize_p == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ( (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA)   &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY) )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (Destination0_p < (Destination1_p+DestinationSize1)) &&
         (Destination1_p < (Destination0_p+DestinationSize0)) )
        return( ST_ERROR_BAD_PARAMETER );   /* Buffer overlap */

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );

    return( Error );
}


/* ------------------------------------------------------------------------- */
ST_ErrorCode_t stptiValidate_BufferSetReadPointer(FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if(Error ==ST_NO_ERROR)
    {
        if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
            Error = STPTI_ERROR_BUFFER_NOT_LINKED;
    }
    return Error;
}
/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferSetOverflowControl(FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if(Error ==ST_NO_ERROR)
    {
        if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
            Error = STPTI_ERROR_BUFFER_NOT_LINKED;
        else
        {
            if(!stptiMemGet_Buffer(FullBufferHandle)->ManualAllocation)
                Error = STPTI_ERROR_NOT_A_MANUAL_BUFFER;
        }
    }
    return Error;
}
/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferReadPartialPesPacket( FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0,
         U8 *Destination1_p, U32 DestinationSize1, BOOL *PayloadUnitStart_p, BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p,
         U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    if ( (DestinationSize0 != 0 && Destination0_p == NULL) ||
         (DestinationSize1 != 0 && Destination1_p == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (Destination1_p < (Destination0_p + DestinationSize0)) &&
         (Destination0_p < (Destination1_p + DestinationSize1)) )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (PayloadUnitStart_p == NULL) ||
         (CCDiscontinuity_p  == NULL) ||
         (ContinuityCount_p  == NULL) ||
         (DataSize_p         == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ( (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY) )
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck(FullBufferHandle);
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );


#ifndef STPTI_NO_USAGE_CHECK
    {
        FullHandle_t SlotListHandle = (stptiMemGet_Buffer(FullBufferHandle))->SlotListHandle;
        FullHandle_t FullSlotHandle;
        BOOL SignalOnEveryTransportPacket;
        U32 MaxSlotHandles = stptiMemGet_List(SlotListHandle)->MaxHandles;
        U32 slot;

        if ( SlotListHandle.word == STPTI_NullHandle() )
            return( STPTI_ERROR_BUFFER_NOT_LINKED );

        /* This function can only be called if the slots feeding the buffer are set to
           'signal-on-every-transport-packet' ... so find the slots feeding this buffer and
           abort if any are not signaling on every packet. */

        for (slot = 0; slot < MaxSlotHandles; ++slot)
        {
            FullSlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[slot];

            if ( FullSlotHandle.word != STPTI_NullHandle() )
            {
                SignalOnEveryTransportPacket = (stptiMemGet_Slot(FullSlotHandle))->Flags.SignalOnEveryTransportPacket;

                if (SignalOnEveryTransportPacket == FALSE)
                    return( STPTI_ERROR_SLOT_NOT_SIGNAL_EVERY_PACKET );
            }
        }
    }
#endif  /* STPTI_NO_USAGE_CHECK */

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferReadPes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    if ( ((DestinationSize0 != 0) && (Destination0_p == NULL)) ||
         ((DestinationSize1 != 0) && (Destination1_p == NULL)) )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (Destination0_p < (Destination1_p + DestinationSize1)) &&
         (Destination1_p < (Destination0_p + DestinationSize0)) )
        return( ST_ERROR_BAD_PARAMETER );

    if (DataSize_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ( (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA)   &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY) )
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );


    return( Error );
}


/* ------------------------------------------------------------------------- */


#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferSetGPDMAParams(FullHandle_t FullBufferHandle, STPTI_GPDMAParams_t GPDMAParams)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}
#endif


#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferSetPCPParams(FullHandle_t FullBufferHandle, STPTI_PCPParams_t PCPParams)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}
#endif


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferTestForData(FullHandle_t FullBufferHandle, U32 *BytesInBuffer)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    if (BytesInBuffer == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data)
{
    if (Data == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( ST_ERROR_UNKNOWN_DEVICE );

    if ( stpti_DeviceNameExists(DeviceName) )
            return( ST_NO_ERROR );

    return( ST_ERROR_UNKNOWN_DEVICE );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_DescramblerDeallocate(FullHandle_t FullDescramblerHandle)
{
    return( stpti_DescramblerHandleCheck( FullDescramblerHandle ) );
}


/* ------------------------------------------------------------------------- */



ST_ErrorCode_t stptiValidate_Descrambler_AssociatePid( FullHandle_t DescramblerHandle )
{
    if ( (stptiMemGet_Descrambler(DescramblerHandle))->AssociatedPid != TC_INVALID_PID )
        return( STPTI_ERROR_DESCRAMBLER_ALREADY_ASSOCIATED );

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_DescramblerAssociate(FullHandle_t FullDescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;

    Error = stpti_DescramblerHandleCheck(FullDescramblerHandle);
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

    if ( stptiMemGet_Device(FullDescramblerHandle)->DescramblerAssociationType == STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
    {
        FullSlotHandle.word = SlotOrPid.Slot;
        Error = stpti_SlotHandleCheck( FullSlotHandle );
    }
    else
    {
        if (SlotOrPid.Pid > DVB_MAXIMUM_PID_VALUE)
        {
            Error = STPTI_ERROR_INVALID_PID;
        }
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_DescramblerDisassociate(FullHandle_t FullDescramblerHandle, STPTI_SlotOrPid_t SlotOrPid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullSlotHandle;


    Error = stpti_DescramblerHandleCheck(FullDescramblerHandle);
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( stptiMemGet_Device(FullDescramblerHandle)->DescramblerAssociationType == STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS )
    {
        FullSlotHandle.word = SlotOrPid.Slot;
            Error = stpti_SlotHandleCheck( FullSlotHandle );
    }
    else
    {
        if ( (stptiMemGet_Descrambler(FullDescramblerHandle))->AssociatedPid == TC_INVALID_PID )
            Error = STPTI_ERROR_DESCRAMBLER_NOT_ASSOCIATED;
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_DescramblerSet(FullHandle_t FullDescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (Parity != STPTI_KEY_PARITY_EVEN_PARITY) &&
         (Parity != STPTI_KEY_PARITY_ODD_PARITY) )
        return( STPTI_ERROR_INVALID_PARITY );

    if ( (Usage != STPTI_KEY_USAGE_INVALID)             &&
         (Usage != STPTI_KEY_USAGE_VALID_FOR_PES)       &&
         (Usage != STPTI_KEY_USAGE_VALID_FOR_TRANSPORT) &&
         (Usage != STPTI_KEY_USAGE_VALID_FOR_ALL))
        return( STPTI_ERROR_INVALID_KEY_USAGE );

    if ( (Data == NULL) && (Usage != STPTI_KEY_USAGE_INVALID) ) /* FIX GNBvd15090 - Don't need 'Data' to be valid if we are invalidating a key */
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_DescramblerHandleCheck( FullDescramblerHandle );

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_DescramblerSetType(FullHandle_t FullDescramblerHandle, STPTI_DescramblerType_t DescramblerType)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DVB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_OFB_DESCRAMBLER) &&
#if defined( STPTI_POWERKEY_SUPPORT )
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DES_CTS_DESCRAMBLER) &&
#endif
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_OFB_DESCRAMBLER) &&
#if defined( STPTI_POWERKEY_SUPPORT )
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_MULTI2_CTS_DESCRAMBLER) &&
#endif
#if defined(ST_7100) || defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_DVB_EAVS_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_OFB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CTS_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_NSA_MDD_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_NSA_MDI_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_IPTV_CSA_DESCRAMBLER) &&
#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_ECB_LR_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_AES_CBC_LR_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_ECB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CBC_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_ECB_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CBC_DVS042_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_OFB_DESCRAMBLER) &&
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_3DES_CTS_DESCRAMBLER) &&
#endif
#endif
         (DescramblerType != STPTI_DESCRAMBLER_TYPE_FASTI_DESCRAMBLER))
        return( STPTI_ERROR_DESCRAMBLER_TYPE_NOT_SUPPORTED );

    Error = stpti_DescramblerHandleCheck( FullDescramblerHandle );

    return( Error );
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_DescramblerSetSVP(FullHandle_t FullDescramblerHandle, STPTI_NSAMode_t mode)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    switch(mode)
    {
        case STPTI_NSA_MODE_MDD:
        case STPTI_NSA_MODE_MDI:
            break;

        default:
            return( ST_ERROR_BAD_PARAMETER );
    }

    Error = stpti_DescramblerHandleCheck( FullDescramblerHandle );

    return( Error );
}

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_EnableDisableScramblingEvents(FullHandle_t FullSlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_SlotHandleCheck(FullSlotHandle);
    if (Error != ST_NO_ERROR)
        return( Error );


    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_FilterSetMatchAction(FullHandle_t FullFilterHandle, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable)
{
    return( STPTI_ERROR_FUNCTION_NOT_SUPPORTED );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_FilterHandleCheck( FullHandle_t FullFilterHandle )
{
    return( stpti_FilterHandleCheck( FullFilterHandle ) );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FiltersFlush(FullHandle_t FullBufferHandle, STPTI_Filter_t Filters[], U16 NumOfFiltersToFlush)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 filter;
    FullHandle_t FullFilterHandle;


    if (Filters == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );


    for (filter = 0; filter < NumOfFiltersToFlush; ++filter)
    {
        FullFilterHandle.word = Filters[filter];
        Error = stpti_FilterHandleCheck( FullFilterHandle );
        if (Error != ST_NO_ERROR)
            return( Error );
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t *DeviceCapability)
{
    if (DeviceCapability == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( ST_ERROR_UNKNOWN_DEVICE );

    if ( stpti_DeviceNameExists(DeviceName) )
            return( ST_NO_ERROR );

    return( ST_ERROR_UNKNOWN_DEVICE );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count)
{
    if (Count == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( ST_ERROR_UNKNOWN_DEVICE );

    if ( stpti_DeviceNameExists(DeviceName) )
            return( ST_NO_ERROR );

    return( ST_ERROR_UNKNOWN_DEVICE );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GetPacketArrivalTime(FullHandle_t DeviceHandle, STPTI_TimeStamp_t *ArrivalTime, U16 *ArrivalTimeExtension)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (ArrivalTime == NULL || ArrivalTimeExtension == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_ObjectDeviceCheck( DeviceHandle, FALSE );

    return Error;
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count_p)
{

    if (Count_p == NULL)
        return ( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return ( ST_ERROR_UNKNOWN_DEVICE );

    if ( stpti_DeviceNameExists(DeviceName) )
    {
        return( ST_NO_ERROR );
    }

    return ( ST_ERROR_UNKNOWN_DEVICE );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (TimeStamp == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GetCurrentPTITimer(ST_DeviceName_t DeviceName, STPTI_TimeStamp_t *TimeStamp)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (TimeStamp == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferLinkToCdFifo(FullHandle_t FullBufferHandle)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_SetStreamID(ST_DeviceName_t DeviceName, STPTI_StreamID_t StreamID)
{

#if defined( ST_5528 ) || defined( ST_7109 ) || defined( ST_7200 ) || defined(ST_7111) || defined(ST_7141)
    if (StreamID < STPTI_STREAM_ID_TSIN0 || StreamID > STPTI_STREAM_ID_NONE)
#else
    if ((StreamID != STPTI_STREAM_ID_TSIN0) &&
        (StreamID != STPTI_STREAM_ID_TSIN1) &&
        (StreamID != STPTI_STREAM_ID_TSIN2) &&
        (StreamID != STPTI_STREAM_ID_SWTS0) &&
        (StreamID != STPTI_STREAM_ID_NOTAGS) &&
        (StreamID != STPTI_STREAM_ID_ALTOUT) &&
          (StreamID != STPTI_STREAM_ID_NONE) )
#endif
        return( ST_ERROR_BAD_PARAMETER );

    if ( stpti_DeviceNameExists(DeviceName) )
            return( ST_NO_ERROR );

    return( ST_ERROR_UNKNOWN_DEVICE );


}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_SetDiscardParams(ST_DeviceName_t DeviceName, U8 NumberOfDiscardBytes)
{

    if ( stpti_DeviceNameExists(DeviceName) )
            return( ST_NO_ERROR );

    return( ST_ERROR_UNKNOWN_DEVICE );


}
/* ------------------------------------------------------------------------- */
ST_ErrorCode_t stptiValidate_SlotSetCorruptionParams( FullHandle_t FullSlotHandle, U8 Offset )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U8 PacketSize=0;
    U8 PayloadStart=0;

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }


    {
        PacketSize = DVB_TS_PACKET_LENGTH;
        PayloadStart = DVB_SECTION_START;
    }

    if (Offset >= PacketSize)
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    return( Error );
}

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotSetCCControl(FullHandle_t FullSlotHandle)
{
    return( stpti_SlotHandleCheck( FullSlotHandle ) );
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[], U16 NumOfFiltersToDisable, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t TmpFilterHandle;
    U32 filter;

    if (FiltersToDisable == NULL || FiltersToEnable == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    for (filter = 0; filter < NumOfFiltersToDisable; ++filter)
    {
        TmpFilterHandle.word = FiltersToDisable[filter];

        Error = stpti_FilterHandleCheck(TmpFilterHandle);
        if (Error != ST_NO_ERROR)
            return( Error );

        switch ( (stptiMemGet_Filter(TmpFilterHandle))->Type )
        {
            case STPTI_FILTER_TYPE_TSHEADER_FILTER:
            case STPTI_FILTER_TYPE_PES_FILTER:
            case STPTI_FILTER_TYPE_SECTION_FILTER:
            case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
                return( ST_ERROR_FEATURE_NOT_SUPPORTED );

            case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:
            case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
#ifdef STPTI_DC2_SUPPORT
            case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
            case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
            case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
#endif
                break;

            default:
                return( ST_ERROR_BAD_PARAMETER );
        }
    }


    for (filter = 0; filter < NumOfFiltersToEnable; ++filter)
    {

        TmpFilterHandle.word = FiltersToEnable[filter];

        Error = stpti_FilterHandleCheck(TmpFilterHandle);
        if (Error != ST_NO_ERROR)
            return( Error );

        switch ( (stptiMemGet_Filter(TmpFilterHandle))->Type )
        {
            case STPTI_FILTER_TYPE_TSHEADER_FILTER:
            case STPTI_FILTER_TYPE_PES_FILTER:
            case STPTI_FILTER_TYPE_SECTION_FILTER:
            case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
                return( ST_ERROR_FEATURE_NOT_SUPPORTED );

            case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:
            case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
#ifdef STPTI_DC2_SUPPORT
            case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
            case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
            case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
#endif
                break;

            default:
                return( ST_ERROR_BAD_PARAMETER );
        }

    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_PidQuery(ST_DeviceName_t DeviceName, STPTI_Pid_t Pid, STPTI_Slot_t *Slot_p)
{
    if (Slot_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if (Pid > DVB_MAXIMUM_PID_VALUE)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if (STPTI_Driver.MemCtl.Partition_p == NULL)
    {
        return( ST_ERROR_UNKNOWN_DEVICE );
    }
    if ( stpti_DeviceNameExists(DeviceName) )
    {
        return( ST_NO_ERROR );
    }
    return( ST_ERROR_UNKNOWN_DEVICE );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_AlternateOutputSetDefaultAction(FullHandle_t SessionHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (Method != STPTI_ALTERNATE_OUTPUT_TYPE_NO_OUTPUT)    &&
         (Method != STPTI_ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS) &&
         (Method != STPTI_ALTERNATE_OUTPUT_TYPE_DESCRAMBLED) )
        return( STPTI_ERROR_INVALID_ALTERNATE_OUTPUT_TYPE );

    Error = stpti_SessionHandleCheck(SessionHandle);
    if (Error != ST_NO_ERROR)
        return( Error );

    if (Method == STPTI_ALTERNATE_OUTPUT_TYPE_DESCRAMBLED)
        return( STPTI_ERROR_ALT_OUT_TYPE_NOT_SUPPORTED );

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotDescramblingControl(FullHandle_t FullSlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)
        return( Error );


    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotPacketCount(FullHandle_t FullSlotHandle, U16 *Count)
{
    if (Count == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    return( stpti_SlotHandleCheck( FullSlotHandle ) );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_SlotQuery(FullHandle_t FullSlotHandle, BOOL *PacketsSeen, BOOL *TransportScrambledPacketsSeen, BOOL *PESScrambledPacketsSeen, STPTI_Pid_t *Pid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (PacketsSeen == NULL) ||
         (TransportScrambledPacketsSeen == NULL) ||
         (PESScrambledPacketsSeen == NULL) ||
         (Pid == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    return( Error );
}
/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotSetAlternateOutputAction(FullHandle_t FullSlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (Method != STPTI_ALTERNATE_OUTPUT_TYPE_NO_OUTPUT)   &&
         (Method != STPTI_ALTERNATE_OUTPUT_TYPE_OUTPUT_AS_IS) &&
         (Method != STPTI_ALTERNATE_OUTPUT_TYPE_DESCRAMBLED) )
        return( STPTI_ERROR_INVALID_ALTERNATE_OUTPUT_TYPE );

    Error = stpti_SlotHandleCheck(FullSlotHandle);
    if (Error != ST_NO_ERROR)
        return( Error );


    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_UserDataBlockMove(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p)
{
#if defined ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else

    if (DMAParams_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (DMAParams_p->BurstSize > STPTI_DMA_BURST_MODE_ONE_BYTE)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( STPTI_ERROR_NOT_INITIALISED );

    return( ST_NO_ERROR );

#endif
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_UserDataCircularAppend(U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p)
{
#if defined ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else

    if (DMAParams_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( STPTI_ERROR_NOT_INITIALISED );

    return( ST_NO_ERROR );

#endif
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_UserDataCircularSetup(U8 *Buffer_p, U32 BufferSize, U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p)
{
#if defined ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else

    if (DMAParams_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (DMAParams_p->BurstSize > STPTI_DMA_BURST_MODE_ONE_BYTE)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( STPTI_ERROR_NOT_INITIALISED );

    return( ST_NO_ERROR );

#endif
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_UserDataSynchronize(STPTI_DMAParams_t *DMAParams_p)
{
#if defined ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else

    if (DMAParams_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( STPTI_ERROR_NOT_INITIALISED );

    return( ST_NO_ERROR );

#endif
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_UserDataWrite(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p)
{
#if defined ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else

    if (DataPtr_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (DMAParams_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (DMAParams_p->BurstSize > STPTI_DMA_BURST_MODE_ONE_BYTE)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( STPTI_ERROR_NOT_INITIALISED );

    return( ST_NO_ERROR );

#endif
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_UserDataRemaining(STPTI_DMAParams_t *DMAParams_p, U32 *UserDataRemaining)
{
#if defined ( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )

    return ST_ERROR_FEATURE_NOT_SUPPORTED;

#else

    if (UserDataRemaining == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (DMAParams_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
        return( STPTI_ERROR_NOT_INITIALISED );

    return( ST_NO_ERROR );

#endif
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotLinkToCdFifo(FullHandle_t FullSlotHandle, STPTI_DMAParams_t *CdFifoParams)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if( (stptiMemGet_Slot(FullSlotHandle))->CDFifoAddr != (U32) NULL )
        Error = STPTI_ERROR_SLOT_ALREADY_LINKED;

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferReadNBytes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (DestinationSize0 != 0 && Destination0_p == NULL) ||
         (DestinationSize1 != 0 && Destination1_p == NULL) ||
         (DataSize_p == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ( (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY) )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (Destination0_p < (Destination1_p+DestinationSize1)) &&    /* Buffer overlap */
         (Destination1_p < (Destination0_p+DestinationSize0)) )
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck(FullBufferHandle);
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        return( STPTI_ERROR_BUFFER_NOT_LINKED );

    return( Error );
}


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_Init(ST_DeviceName_t DeviceName, const STPTI_InitParams_t *InitParams)
{

    if (InitParams->Device != stptiHAL_Device() )   /* built for the right HAL? */
    {
        STTBX_Print(("stptiValidate_Init FAIL 2\n"));
        return( STPTI_ERROR_INVALID_DEVICE );
    }

    if (DeviceName == NULL)
    {
        STTBX_Print(("stptiValidate_Init FAIL 3\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if ( strlen(DeviceName) >= sizeof(ST_DeviceName_t) ) /* Must be >= as the terminating Null must also fit into the array */
    {
        STTBX_Print(("stptiValidate_Init FAIL 4 (%d Vs. %d)\n", strlen(DeviceName), sizeof(ST_DeviceName_t)));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if (InitParams == NULL)
    {
        STTBX_Print(("stptiValidate_Init FAIL 5\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

#if !defined(ST_OSLINUX)

#ifndef STPTI_NO_INDEX_SUPPORT

    if ((MIN_USER_PRIORITY > InitParams->IndexProcessPriority) ||
        (MAX_USER_PRIORITY < InitParams->IndexProcessPriority))
    {
        STTBX_Print(("stptiValidate_Init FAIL 6\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif /* #endifndef STPTI_NO_INDEX_SUPPORT */

    if ((MIN_USER_PRIORITY > InitParams->EventProcessPriority) ||
        (MAX_USER_PRIORITY < InitParams->EventProcessPriority))
    {
        STTBX_Print(("stptiValidate_Init FAIL 7\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if ((MIN_USER_PRIORITY > InitParams->InterruptProcessPriority) ||
        (MAX_USER_PRIORITY < InitParams->InterruptProcessPriority))
    {
        STTBX_Print(("stptiValidate_Init FAIL 8\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

#endif /* #endif !defined(ST_OSLINUX) */

    if ( (InitParams->TCCodes != STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB)
        )
    {
        STTBX_Print(("stptiValidate_Init FAIL 10\n"));
        return( STPTI_ERROR_INVALID_SUPPORTED_TC_CODE );
    }


    if ( (InitParams->DescramblerAssociation != STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_PIDS)
      && (InitParams->DescramblerAssociation != STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS))
    {
        STTBX_Print(("stptiValidate_Init FAIL 11\n"));
        return( STPTI_ERROR_INVALID_DESCRAMBLER_ASSOCIATION );
    }


#ifndef STPTI_NO_INDEX_SUPPORT
    if ( (InitParams->IndexAssociation != STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_PIDS)
      && (InitParams->IndexAssociation != STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS))
    {
        STTBX_Print(("stptiValidate_Init FAIL 12\n"));
        return( STPTI_ERROR_INDEX_INVALID_ASSOCIATION );
    }
#endif

#ifndef ST_OSLINUX
    if ( (InitParams->TCDeviceAddress_p == NULL) ||
         (InitParams->TCLoader_p        == NULL) ||
#ifdef STPTI_SUPPRESS_CACHING
         (InitParams->NCachePartition_p == NULL) ||
#endif
         (InitParams->Partition_p       == NULL) )
    {
        STTBX_Print(("stptiValidate_Init FAIL 13 TCDeviceAddress_p 0x%08x TCLoader_p %d Partition_p %d\n",
                (int)InitParams->TCDeviceAddress_p, (int)InitParams->TCLoader_p, (int)InitParams->Partition_p ));
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif /* #endifndef ST_OSLINUX */

#if !defined(ST_OSLINUX)
    if ( (MIN_USER_PRIORITY > InitParams->EventProcessPriority) ||
         (InitParams->EventProcessPriority > MAX_USER_PRIORITY) )
    {
        STTBX_Print(("stptiValidate_Init FAIL 14\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if ( (MIN_USER_PRIORITY > InitParams->InterruptProcessPriority) ||
         (InitParams->InterruptProcessPriority > MAX_USER_PRIORITY) )
    {
        STTBX_Print(("stptiValidate_Init FAIL 15\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif /* #endif !defined(ST_OSLINUX) */

    /* DTV does not care about what filter mode 'init' says it is to use but DVB *DOES* */

    if ( ( InitParams->TCCodes == STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB )
       )
    {
        if ( (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_8x8)    &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_16x8)   &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_32x8)   &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_ANYx8)  &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_64x8)   &&
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE ) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_64x16)  &&
#endif
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_8x16)   &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_16x16)  &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_32x16)  &&
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_ANYx16) &&
#if defined( STPTI_ARCHITECTURE_PTI4_LITE ) || defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_48x8)  &&
/*             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_48x16)  &&    *** Awaiting H/W Confirmation of fix *** */
#endif
             (InitParams->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_NONE) )
        {
            STTBX_Print(("stptiValidate_Init FAIL 17\n"));
            return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
    }

#if defined( STPTI_ARCHITECTURE_PTI4_LITE ) || defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
    if (InitParams->SectionFilterOperatingMode == STPTI_FILTER_OPERATING_MODE_ANYx8)
    {
        /* ramcam demands that filters are allocated in a multiple of 8 */
        if ( (InitParams->NumberOfSectionFilters % 8) != 0 )
        {
            STTBX_Print(("stptiValidate_Init FAIL 18\n"));
            return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
        /* We have a limit of 64 filters per session */
        if ( InitParams->NumberOfSectionFilters > 64 )
        {
            STTBX_Print(("stptiValidate_Init FAIL 19\n"));
            return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
    }

    if (InitParams->SectionFilterOperatingMode == STPTI_FILTER_OPERATING_MODE_ANYx16)
    {
        /* ramcam demands that filters are allocated in a multiples of 8 */
        if ( (InitParams->NumberOfSectionFilters % 8) != 0 )
        {
            STTBX_Print(("stptiValidate_Init FAIL 18\n"));
            return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
        /* We have a limit of 32 filters per session */
        /*if ( InitParams->NumberOfSectionFilters > 48 )     *** Awaiting H/W Confirmation of fix *** */
        if ( InitParams->NumberOfSectionFilters > 32 )
        {
            STTBX_Print(("stptiValidate_Init FAIL 19\n"));
            return( STPTI_ERROR_INVALID_FILTER_OPERATING_MODE );
        }
    }
#endif

    /* InitParams->NumberOfSlots checked when loader function run (dynamic checking) */

#if defined( ST_5528 ) || defined( ST_7109 )  || defined( ST_7200 ) || defined (ST_7111)
    if ( (InitParams->StreamID < STPTI_STREAM_ID_TSIN0) ||
         (InitParams->StreamID > STPTI_STREAM_ID_NONE) )
#else
    if ((InitParams->StreamID != STPTI_STREAM_ID_TSIN0) &&
        (InitParams->StreamID != STPTI_STREAM_ID_TSIN1) &&
        (InitParams->StreamID != STPTI_STREAM_ID_TSIN2) &&
    (InitParams->StreamID != STPTI_STREAM_ID_SWTS0) &&
        (InitParams->StreamID != STPTI_STREAM_ID_NOTAGS) &&
        (InitParams->StreamID != STPTI_STREAM_ID_ALTOUT) &&
          (InitParams->StreamID != STPTI_STREAM_ID_NONE) )
#endif
    {
        STTBX_Print(("stptiValidate_Init FAIL 20\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

    if ( (InitParams->StreamID == STPTI_STREAM_ID_NOTAGS)
#if defined (STPTI_FRONTEND_HYBRID)
    ||  /* Cannot use STPTI_STREAM_ID_NOTAGS or STPTI_ARRIVAL_TIME_SOURCE_PTI on 7200 */
         (InitParams->PacketArrivalTimeSource != STPTI_ARRIVAL_TIME_SOURCE_TSMERGER))
#else
    &&
         (InitParams->PacketArrivalTimeSource == STPTI_ARRIVAL_TIME_SOURCE_TSMERGER))
#endif
    {
        /* You cannot use STPTI_ARRIVAL_TIME_SOURCE_TSMERGER in Bypass mode */
        STTBX_Print(("stptiValidate_Init FAIL 21\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }

#if defined (STPTI_FRONTEND_HYBRID)
    /* The PTI's Sync Lock and Drop Mechanism is not supported on the Hybrid - use the Frontend Objects to set them instead */
    if ( InitParams->SyncLock != 0 )
    {
        STTBX_Print(("stptiValidate_Init FAIL 22\n"));
        return( ST_ERROR_BAD_PARAMETER );
    }
#endif

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferAllocate(FullHandle_t FullSessionHandle, U32 RequiredSize, U32 NumberOfPacketsInMultiPacket, STPTI_Buffer_t *BufferHandle)
{
    if (BufferHandle == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    if (NumberOfPacketsInMultiPacket < 1 || 0x8000 < NumberOfPacketsInMultiPacket)
        return( ST_ERROR_BAD_PARAMETER );

    if (RequiredSize & 0X80000000) /* Test for negative / very big buffer size */
        return( ST_ERROR_BAD_PARAMETER );

    return( stpti_SessionHandleCheck( FullSessionHandle ) );
}

/* ------------------------------------------------------------------------- */

#if defined (STPTI_FRONTEND_HYBRID)
ST_ErrorCode_t stptiValidate_FrontendAllocate( FullHandle_t FullSessionHandle, STPTI_Frontend_t *FrontendHandle_p, STPTI_FrontendType_t FrontendType)
{
    if (FrontendHandle_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    else if ( (FrontendType != STPTI_FRONTEND_TYPE_TSIN) && (FrontendType != STPTI_FRONTEND_TYPE_SWTS) )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    return( stpti_SessionHandleCheck( FullSessionHandle ) );
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FrontendLinkToPTI( FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_FrontendHandleCheck( FullFrontendHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
    {
        return ( Error );
    }

    Error = stpti_SessionHandleCheck( FullSessionHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
    {
        return ( Error );
    }

#ifndef STPTI_NO_USAGE_CHECK
    /* Frontend & PTI MUST be on same device and SHOULD be in same session */
    if ( FullFrontendHandle.member.Device != FullSessionHandle.member.Device )
    {
        return ( STPTI_ERROR_NOT_ON_SAME_DEVICE );
    }
#ifdef STPTI_CHECK_SAME_SESSION
    if ( FullFrontendHandle.member.Session != FullSessionHandle.member.Session )
    {
        return ( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
    }
#endif
#endif

    return( Error );
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FrontendSetParams( FullHandle_t FullFrontendHandle, STPTI_FrontendParams_t * FrontendParams_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_FrontendHandleCheck( FullFrontendHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
    {
        return ( Error );
    }

    if (FrontendParams_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }


    /* Check all the rest of the params here TBC */

    switch ( FrontendParams_p->PktLength )
    {
        case STPTI_FRONTEND_PACKET_LENGTH_DVB:
            break;
        default:
            return ST_ERROR_BAD_PARAMETER;
            break;
    }

    /* We need to check the rest of the params depending on the type of frontend object */
    switch ( (stptiMemGet_Frontend( FullFrontendHandle ))->Type )
    {
        case STPTI_FRONTEND_TYPE_TSIN:
        {
            if ( (FrontendParams_p->u.TSIN.MemoryPktNum == 0) || (FrontendParams_p->u.TSIN.ClkRvSrc > STPTI_FRONTEND_CLK_RCV_END ) )
            {
                Error = ST_ERROR_BAD_PARAMETER;
            }
            break;
        }
        case STPTI_FRONTEND_TYPE_SWTS:
            {
                U32 Num = (FrontendParams_p->u.SWTS.PaceBitRate);

                if ( (Num != 0) && ((Num < STPTI_SWTS_MIN_RATE) || (Num > STPTI_SWTS_MAX_RATE)) )
                {
                    Error = ST_ERROR_BAD_PARAMETER;
                }
            }
            break;
        default:
            Error = ST_ERROR_BAD_PARAMETER;
            break;
    }

    return (Error);
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FrontendGetParams( FullHandle_t FullFrontendHandle, STPTI_FrontendType_t * FrontendType_p, STPTI_FrontendParams_t * FrontendParams_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if (FrontendType_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    else if (FrontendParams_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    Error = stpti_FrontendHandleCheck( FullFrontendHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
    {
        return ( Error );
    }

    return (Error);
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FrontendGetStatus ( FullHandle_t FullFrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p )
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if (FrontendStatus_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    Error = stpti_FrontendHandleCheck( FullFrontendHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
    {
        return ( Error );
    }

    return (Error);
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FrontendInjectData( FullHandle_t FullFrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p )
{
    ST_ErrorCode_t Error;

    Error = stpti_FrontendHandleCheck( FullFrontendHandle );

    if ( (ST_NO_ERROR == Error) && (FrontendSWTSNode_p == NULL)  )
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    return Error;
}

/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_FrontendHandle(FullHandle_t FullFrontendHandle)
{
    return( stpti_FrontendHandleCheck( FullFrontendHandle ) );
}

#endif /* #if defined (STPTI_FRONTEND_HYBRID) */

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_FilterAllocate(FullHandle_t FullSessionHandle, STPTI_FilterType_t FilterType, STPTI_Filter_t *FilterObject)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (FilterObject == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_SessionHandleCheck( FullSessionHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    switch (FilterType)
    {

        /* ------------------------------------------------ */


        /* ------------------------------------------------ */

    case STPTI_FILTER_TYPE_SECTION_FILTER:

        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
        break;

        /* ------------------------------------------------ */

    case STPTI_FILTER_TYPE_EMM_FILTER:
    case STPTI_FILTER_TYPE_ECM_FILTER:

        switch (stptiMemGet_Device(FullSessionHandle)->TCCodes)
        {
            case STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB:
                break;

            /* case STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV: Do the default */
            default:
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }
        break;

        /* ------------------------------------------------ */

#ifdef STPTI_DC2_SUPPORT
    case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
    case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
    case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
        switch (stptiMemGet_Device(FullSessionHandle)->TCCodes)
        {
            case STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB:
                break;

            /* case STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV: Do the default */
            default:
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }
        break;
#endif

        /* ------------------------------------------------ */

    case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:
#if !defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
#endif
        break;

        /* ------------------------------------------------ */

    case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:

        switch (stptiMemGet_Device(FullSessionHandle)->TCCodes)
        {
            case STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB:
                break;

            /* case STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV: Do the default */
            default:
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }

#if !defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
        switch (stptiMemGet_Device(FullSessionHandle)->SectionFilterOperatingMode)
        {
            case STPTI_FILTER_OPERATING_MODE_8x8:
            case STPTI_FILTER_OPERATING_MODE_16x8:
            case STPTI_FILTER_OPERATING_MODE_32x8:
            case STPTI_FILTER_OPERATING_MODE_ANYx8:
#if defined( STPTI_ARCHITECTURE_PTI4 )
            case STPTI_FILTER_OPERATING_MODE_64x8:
#endif
#if defined( STPTI_ARCHITECTURE_PTI4_LITE ) || defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
            case STPTI_FILTER_OPERATING_MODE_48x8:
#endif
                break;

            default:
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }
#endif
        break;

        /* ------------------------------------------------ */

    case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:

        switch (stptiMemGet_Device(FullSessionHandle)->TCCodes)
        {
            case STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB:
                break;

            /* case STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV: Do the default */
            default:
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }

#if !defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
        switch (stptiMemGet_Device(FullSessionHandle)->SectionFilterOperatingMode)
        {
            case STPTI_FILTER_OPERATING_MODE_8x16:
            case STPTI_FILTER_OPERATING_MODE_16x16:
            case STPTI_FILTER_OPERATING_MODE_32x16:
#if defined( STPTI_ARCHITECTURE_PTI4_LITE ) || defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
            case STPTI_FILTER_OPERATING_MODE_48x16:
#endif
                break;

            default:
                Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
                break;
        }
#endif
        break;

        /* ------------------------------------------------ */

    case STPTI_FILTER_TYPE_PES_FILTER:
    case STPTI_FILTER_TYPE_TSHEADER_FILTER:

        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
        break;

    case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
    break;

        /* ------------------------------------------------ */

    default:
        Error = STPTI_ERROR_INVALID_FILTER_TYPE;
        break;
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotAllocate(FullHandle_t FullSessionHandle, STPTI_Slot_t *SlotHandle, STPTI_SlotData_t *SlotData)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    if ( (SlotHandle == NULL) || (SlotData == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    if (   (SlotData->SlotType != STPTI_SLOT_TYPE_NULL)        && (SlotData->SlotType != STPTI_SLOT_TYPE_SECTION)
        && (SlotData->SlotType != STPTI_SLOT_TYPE_PES)         && (SlotData->SlotType != STPTI_SLOT_TYPE_RAW)
#ifdef STPTI_DC2_SUPPORT
        && (SlotData->SlotType != STPTI_SLOT_TYPE_DC2_PRIVATE) && (SlotData->SlotType != STPTI_SLOT_TYPE_DC2_MIXED)
#endif
        && (SlotData->SlotType != STPTI_SLOT_TYPE_EMM)         && (SlotData->SlotType != STPTI_SLOT_TYPE_ECM)
        && (SlotData->SlotType != STPTI_SLOT_TYPE_PCR) )
        return( STPTI_ERROR_INVALID_SLOT_TYPE );

    if (SlotData->SlotFlags.SignalOnEveryTransportPacket && (SlotData->SlotType != STPTI_SLOT_TYPE_PES))
        return( STPTI_ERROR_SIGNAL_EVERY_PACKET_ONLY_ON_PES_SLOT );

    if (SlotData->SlotFlags.StoreLastTSHeader && (SlotData->SlotType != STPTI_SLOT_TYPE_RAW))
        return( STPTI_ERROR_STORE_LAST_HEADER_ONLY_ON_RAW_SLOT );


#ifdef STPTI_CAROUSEL_SUPPORT
    if (SlotData->SlotFlags.AlternateOutputInjectCarouselPacket &&
       (SlotData->SlotType != STPTI_SLOT_TYPE_NULL && SlotData->SlotType != STPTI_SLOT_TYPE_RAW))
        return( STPTI_ERROR_CAROUSEL_OUTPUT_ONLY_ON_NULL_SLOT );
#endif

    if (SlotData->SlotFlags.CollectForAlternateOutputOnly && SlotData->SlotType != STPTI_SLOT_TYPE_NULL)
        return( STPTI_ERROR_COLLECT_FOR_ALT_OUT_ONLY_ON_NULL_SLOT );

    if (SlotData->SlotFlags.InsertSequenceError && (! (SlotData->SlotType == STPTI_SLOT_TYPE_PES) ))
        return( ST_ERROR_FEATURE_NOT_SUPPORTED );

    Error = stpti_SessionHandleCheck( FullSessionHandle );
    if (Error != ST_NO_ERROR)
        return( Error );


    /* find out if the loader supports SignalEveryTransportPacket */
    if ( SlotData->SlotFlags.SignalOnEveryTransportPacket &&
        (!stptiHAL_LoaderGetConfig(FullSessionHandle, SIGNALEVERYTRANSPORTPACKET)) )
        return( STPTI_ERROR_SLOT_FLAG_NOT_SUPPORTED );


    if ( ( (stptiMemGet_Device(FullSessionHandle)->TCCodes == STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB)
         )
        && (SlotData->SlotType != STPTI_SLOT_TYPE_NULL)
        && (SlotData->SlotType != STPTI_SLOT_TYPE_SECTION)
        && (SlotData->SlotType != STPTI_SLOT_TYPE_PES) && (SlotData->SlotType != STPTI_SLOT_TYPE_RAW)
        && (SlotData->SlotType != STPTI_SLOT_TYPE_EMM) && (SlotData->SlotType != STPTI_SLOT_TYPE_ECM)
#ifdef STPTI_DC2_SUPPORT
        && (SlotData->SlotType != STPTI_SLOT_TYPE_DC2_PRIVATE) && (SlotData->SlotType != STPTI_SLOT_TYPE_DC2_MIXED)
#endif
        && (SlotData->SlotType != STPTI_SLOT_TYPE_PCR))
        return( ST_ERROR_FEATURE_NOT_SUPPORTED );

    return( Error );
}


/* ------------------------------------------------------------------------- */




/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferFlush(FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        Error = STPTI_ERROR_BUFFER_NOT_LINKED;

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferReadSection(FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U32 MaxLengthofFilterList, U32 *NumOfFilterMatches_p, BOOL *CRCValid_p, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if( (DataSize_p == NULL) ||
        (NumOfFilterMatches_p == NULL) ||
        (CRCValid_p == NULL) ||
        (MatchedFilterList == NULL && MaxLengthofFilterList > 0))
        return( ST_ERROR_BAD_PARAMETER );

    if ( (DestinationSize0 != 0 && Destination0_p == NULL) ||
         (DestinationSize1 != 0 && Destination1_p == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    if ( (Destination1_p < (Destination0_p + DestinationSize0)) &&
         (Destination0_p < (Destination1_p + DestinationSize1)) )
        return( ST_ERROR_BAD_PARAMETER );

    if (DataSize_p == NULL)
        return( ST_ERROR_BAD_PARAMETER );

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ( (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA)   &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY) )
        return( ST_ERROR_BAD_PARAMETER );

    Error = stpti_BufferHandleCheck(FullBufferHandle);
    if (Error != ST_NO_ERROR)
        return( Error );

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
        Error = STPTI_ERROR_BUFFER_NOT_LINKED;

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_Close(FullHandle_t FullSessionHandle)
{
    return( stpti_SessionHandleCheck( FullSessionHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferDeallocate(FullHandle_t FullBufferHandle)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_FilterDeallocate(FullHandle_t FullFilterHandle)
{
    return( stpti_FilterHandleCheck( FullFilterHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotDeallocate(FullHandle_t FullSlotHandle)
{
    return( stpti_SlotHandleCheck( FullSlotHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_FilterAssociate(FullHandle_t FullFilterHandle, FullHandle_t FullSlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    Error = stpti_FilterHandleCheck( FullFilterHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)
        return( Error );

#ifndef STPTI_NO_USAGE_CHECK
    /* Filter & Slot MUST be on same device and SHOULD be in same session */
    if (FullFilterHandle.member.Device != FullSlotHandle.member.Device)
       return( STPTI_ERROR_NOT_ON_SAME_DEVICE );

#ifdef STPTI_CHECK_SAME_SESSION
    if (FullFilterHandle.member.Session != FullSlotHandle.member.Session)
       return( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
#endif
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_FilterSetParams( FullHandle_t FullFilterHandle, STPTI_FilterData_t *FilterData_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifdef STPTI_DC2_SUPPORT
    if (FilterData_p == NULL)
        return ( ST_ERROR_BAD_PARAMETER );
#endif

    if ( FilterData_p == NULL )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* This test must follow the previous test to preclude the possibility
       of trying to dereference FilterData_p when it is NULL... */
    if (( FilterData_p->FilterBytes_p == NULL ) ||
        ( FilterData_p->FilterMasks_p == NULL ) )
    {
        if( (FilterData_p->FilterType != STPTI_FILTER_TYPE_PES_STREAMID_FILTER)
       )
        {
            return( ST_ERROR_BAD_PARAMETER );
        }
    }

    Error = stpti_FilterHandleCheck(FullFilterHandle);
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
        return ( Error );

    switch (FilterData_p->FilterType)
    {
#if !defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
        case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:    /* FilterType */
#endif
        case STPTI_FILTER_TYPE_PES_FILTER:
        case STPTI_FILTER_TYPE_TSHEADER_FILTER:
        case STPTI_FILTER_TYPE_SECTION_FILTER:
            return( ST_ERROR_FEATURE_NOT_SUPPORTED );
         /* break; */

    case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
            switch (FilterData_p->FilterRepeatMode)
            {
                case STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED:
                    break;
                case STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_ONE_SHOT:
                            /* fall through to default STPTI_ERROR_INVALID_FILTER_REPEAT_MODE */
                default:
                    return( STPTI_ERROR_INVALID_FILTER_REPEAT_MODE );
            }
            break;

        case STPTI_FILTER_TYPE_SECTION_FILTER_SHORT_MODE:
        case STPTI_FILTER_TYPE_SECTION_FILTER_NEG_MATCH_MODE:
#if defined( STPTI_ARCHITECTURE_PTI4_SECURE_LITE )
        case STPTI_FILTER_TYPE_SECTION_FILTER_LONG_MODE:    /* FilterType */
#endif
            if (FilterData_p->u.SectionFilter.NotMatchMode)
            {
                if ( stpti_HAL_Validate_FilterNotMatchModeSetup(FullFilterHandle) != ST_NO_ERROR)
                    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
            }

            /* No Break Here Intentionally! */

        case STPTI_FILTER_TYPE_EMM_FILTER:

        /* EMM Filters are Short Section filters, but DO NOT use the FilterData_p->u.SectionFilter
          union member, so cannot be checked for NotMatchMode */

            switch (FilterData_p->FilterRepeatMode)
            {
                case STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED:
                    break;

                case STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_ONE_SHOT:
                    if ( stpti_HAL_Validate_FilterOneShotSupported( FullFilterHandle ) != ST_NO_ERROR )
                        return( ST_ERROR_FEATURE_NOT_SUPPORTED );
                    break;

                default:
                    return( STPTI_ERROR_INVALID_FILTER_REPEAT_MODE );
            }

            break;




        case STPTI_FILTER_TYPE_ECM_FILTER:
#ifdef STPTI_DC2_SUPPORT
        case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
        case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
        case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
#endif
        switch (FilterData_p->FilterRepeatMode)
        {
            case STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_REPEATED:
                break;

            case STPTI_FILTER_REPEAT_MODE_STPTI_FILTER_ONE_SHOT:
                if ( stpti_HAL_Validate_FilterOneShotSupported( FullFilterHandle ) != ST_NO_ERROR )
                    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
                break;

            default:
                return( STPTI_ERROR_INVALID_FILTER_REPEAT_MODE );
        }
        break;

        default:                                            /* FilterType */
            return( STPTI_ERROR_INVALID_FILTER_TYPE );

    }   /* switch(FilterType) */

#ifndef STPTI_NO_USAGE_CHECK
    if ((stptiMemGet_Filter(FullFilterHandle))->Type != FilterData_p->FilterType)
        return( STPTI_ERROR_INVALID_FILTER_TYPE );
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotLinkToBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
        return ( Error );

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
        return ( Error );

    /* Check it's not a watch & record buffer */
    if ( TRUE == ( stptiMemGet_Buffer( FullBufferHandle ))->IsRecordBuffer)
    {
        Error = STPTI_ERROR_BUFFER_NOT_LINKED;
    }

#ifndef STPTI_NO_USAGE_CHECK
    /* Buffer & Slot MUST be on same device and SHOULD be in same session */
    if ( FullBufferHandle.member.Device != FullSlotHandle.member.Device )
        return ( STPTI_ERROR_NOT_ON_SAME_DEVICE );
#ifdef STPTI_CHECK_SAME_SESSION
    if ( FullBufferHandle.member.Session != FullSlotHandle.member.Session )
        return ( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
#endif
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotLinkToRecordBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
        return ( Error );

    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)   /* We have a bad handle */
        return ( Error );

#ifndef STPTI_NO_USAGE_CHECK
    /* Buffer & Slot MUST be on same device and SHOULD be in same session */
    if ( FullBufferHandle.member.Device != FullSlotHandle.member.Device )
        return ( STPTI_ERROR_NOT_ON_SAME_DEVICE );
#ifdef STPTI_CHECK_SAME_SESSION
    if ( FullBufferHandle.member.Session != FullSlotHandle.member.Session )
        return ( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
#endif
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_GetBuffersFromSlot(FullHandle_t FullSlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if((BufferHandle_p       == NULL) ||
       (RecordBufferHandle_p == NULL))
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }

    if(Error == ST_NO_ERROR)
    {
        Error = stpti_SlotHandleCheck( FullSlotHandle );
    }

    return(Error);
}

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotState(FullHandle_t FullSlotHandle, U32 *SlotCount_p, STPTI_ScrambleState_t *ScrambleState_p, STPTI_Pid_t *Pid_p)
{
    if ( (SlotCount_p     == NULL) ||
         (ScrambleState_p == NULL) ||
         (Pid_p           == NULL) )
        return( ST_ERROR_BAD_PARAMETER );

    return( stpti_SlotHandleCheck( FullSlotHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_Open(ST_DeviceName_t DeviceName, const STPTI_OpenParams_t *OpenParams, STPTI_Handle_t *Handle)
{
    if (OpenParams == NULL)
       return( ST_ERROR_BAD_PARAMETER );

#if !defined(ST_OSLINUX)
    if (OpenParams->DriverPartition_p == NULL)
       return( ST_ERROR_BAD_PARAMETER );

#if defined( STPTI_SUPPRESS_CACHING )
    if (OpenParams->NonCachedPartition_p == NULL)
       return( ST_ERROR_BAD_PARAMETER );
#endif

#endif

    if (Handle == NULL)
       return( ST_ERROR_BAD_PARAMETER );

    if (STPTI_Driver.MemCtl.Partition_p == NULL)
       return( ST_ERROR_BAD_PARAMETER );

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SignalAllocate(FullHandle_t FullSessionHandle, STPTI_Signal_t *SignalHandle)
{
    if (SignalHandle == NULL)
       return( ST_ERROR_BAD_PARAMETER );

    return( stpti_SessionHandleCheck( FullSessionHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SignalAssociateBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return ( Error );

    Error = stpti_SignalHandleCheck( FullSignalHandle );
    if (Error != ST_NO_ERROR)
        return ( Error );

#ifndef STPTI_NO_USAGE_CHECK
    /* Buffer & Signal MUST be on same device and SHOULD be in same session */
    if ( FullBufferHandle.member.Device != FullSignalHandle.member.Device )
        return( STPTI_ERROR_NOT_ON_SAME_DEVICE );
#ifdef STPTI_CHECK_SAME_SESSION
    if ( FullBufferHandle.member.Session != FullSignalHandle.member.Session )
        return( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
#endif
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SignalDeallocate(FullHandle_t FullSignalHandle)
{
    return( stpti_SignalHandleCheck( FullSignalHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SignalDisassociateBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
        return ( Error );

    Error = stpti_SignalHandleCheck( FullSignalHandle );
    if (Error != ST_NO_ERROR)
        return ( Error );

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SignalAbort(FullHandle_t FullSignalHandle)
{
    return( stpti_SignalHandleCheck( FullSignalHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_Descrambler_AssociateSlot(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#ifndef STPTI_NO_USAGE_CHECK
    /* Descrambler & Slot MUST be on same device and SHOULD be in same session */
    if (DescramblerHandle.member.Device != SlotHandle.member.Device)
        return( STPTI_ERROR_NOT_ON_SAME_DEVICE );
#ifdef STPTI_CHECK_SAME_SESSION
    if (DescramblerHandle.member.Session != SlotHandle.member.Session)
        return( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
#endif
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotClearPid(FullHandle_t FullSlotHandle)
{
    return( stpti_SlotHandleCheck( FullSlotHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotSetPid(FullHandle_t FullSlotHandle, STPTI_Pid_t Pid)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (Pid > DVB_MAXIMUM_PID_VALUE) &&
         (Pid != STPTI_WildCardPid()) &&
         (Pid != STPTI_NegativePid()) )
    {
        return( STPTI_ERROR_INVALID_PID );
    }
    Error = stpti_SlotHandleCheck( FullSlotHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

#ifndef STPTI_NO_USAGE_CHECK

    if ( Pid == STPTI_WildCardPid() )
    {
        if ( stptiMemGet_Device(FullSlotHandle)->WildCardUse != STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_WILDCARD_PID_ALREADY_SET;
        }
        else if ( (stptiMemGet_Slot(FullSlotHandle))->Type != STPTI_SLOT_TYPE_RAW )
        {
            Error = STPTI_ERROR_SLOT_NOT_RAW_MODE;
        }
    }

    if ( Pid == STPTI_NegativePid() )
    {
        if ( stptiMemGet_Device(FullSlotHandle)->NegativePIDUse != STPTI_NullHandle() )
        {
            Error = STPTI_ERROR_WILDCARD_PID_ALREADY_SET;
        }
        else if ( (stptiMemGet_Slot(FullSlotHandle))->Type != STPTI_SLOT_TYPE_RAW )
        {
            Error = STPTI_ERROR_SLOT_NOT_RAW_MODE;
        }
    }

#endif
    return( Error );
}

/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_SlotUnLink(FullHandle_t FullSlotHandle)
{
    return( stpti_SlotHandleCheck( FullSlotHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferUnLink(FullHandle_t FullBufferHandle)
{
    return( stpti_BufferHandleCheck( FullBufferHandle ) );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_BufferReadTSPacket(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (DestinationSize0 != 0 && Destination0_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if (DestinationSize1 != 0 && Destination1_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if ( (Destination0_p + DestinationSize0 > Destination1_p) &&
         (Destination1_p + DestinationSize1 > Destination0_p) )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    if (DataSize_p == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    CHECK_COPY_METHOD( DmaOrMemcpy );

    if ( (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_DMA)   &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_GPDMA) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_PCP) &&
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_FDMA)  &&
#if defined(ST_OSLINUX)
        (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY_LINUX_KERNEL)  &&
#endif /* #endif defined(ST_OSLINUX) */
         (DmaOrMemcpy != STPTI_COPY_TRANSFER_BY_MEMCPY) )
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

    if ( (stptiMemGet_Buffer(FullBufferHandle))->TC_DMAIdent == UNININITIALISED_VALUE )
    {
        return( STPTI_ERROR_BUFFER_NOT_LINKED );
    }

    return( Error );
}


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */


#ifdef STPTI_CAROUSEL_SUPPORT


ST_ErrorCode_t stptiValidate_CarouselAllocate(FullHandle_t FullSessionHandle, STPTI_Carousel_t *Carousel)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (Carousel == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }
    Error = stpti_SessionHandleCheck( FullSessionHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }


    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_CarouselLinkToBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_BufferHandleCheck( FullBufferHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

    Error = stpti_CarouselHandleCheck(FullCarouselHandle);
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

#ifndef STPTI_NO_USAGE_CHECK
    /* Buffer & Carousel MUST be on same device and SHOULD be in same session */
    if ( FullBufferHandle.member.Device != FullCarouselHandle.member.Device )
    {
        return( STPTI_ERROR_NOT_ON_SAME_DEVICE );
    }
#ifdef STPTI_CHECK_SAME_SESSION
    if ( FullBufferHandle.member.Session != FullCarouselHandle.member.Session )
    {
        return( STPTI_ERROR_NOT_ALLOCATED_IN_SAME_SESSION );
    }
#endif
#endif
    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_CarouselEntryAllocate(FullHandle_t FullSessionHandle, STPTI_CarouselEntry_t *CarouselEntryHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullCarouselEntryHandle;

    if (CarouselEntryHandle == NULL)
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    Error = stpti_SessionHandleCheck( FullSessionHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

    FullCarouselEntryHandle.word = *CarouselEntryHandle;
    if( ST_NO_ERROR == stpti_CarouselEntryHandleCheck( FullCarouselEntryHandle ))
    {
        return( STPTI_ERROR_CAROUSEL_ENTRY_ALREADY_ALLOCATED );
    }


    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_CarouselInsertEntry(FullHandle_t FullCarouselHandle, STPTI_AlternateOutputTag_t AlternateOutputTag, STPTI_InjectionType_t InjectionType, FullHandle_t FullEntryHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;


    if ( (InjectionType != STPTI_INJECTION_TYPE_ONE_SHOT_INJECT)       &&
         (InjectionType != STPTI_INJECTION_TYPE_REPEATED_INJECT_AS_IS) &&
         (InjectionType != STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP) )
    {
        return( STPTI_ERROR_INVALID_INJECTION_TYPE );
    }

    Error = stpti_CarouselHandleCheck( FullCarouselHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

    Error = stpti_CarouselEntryHandleCheck( FullEntryHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }


#ifndef STPTI_NO_USAGE_CHECK
    if (stptiMemGet_Device(FullCarouselHandle)->EntryType != STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE)
    {
        return( ST_ERROR_BAD_PARAMETER );   /* STPTI_ERROR_ENTRY_TYPE_NOT_SUPPORTED; */
    }
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_CarouselInsertTimedEntry(FullHandle_t FullCarouselHandle, U8 *TSPacket_p, U8 CCValue, STPTI_InjectionType_t InjectionType, STPTI_TimeStamp_t MinOutputTime, STPTI_TimeStamp_t MaxOutputTime, BOOL EventOnOutput, BOOL EventOnTimeout, U32 PrivateData, FullHandle_t FullEntryHandle, U8 FromByte)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if ( (InjectionType != STPTI_INJECTION_TYPE_ONE_SHOT_INJECT)       &&
         (InjectionType != STPTI_INJECTION_TYPE_REPEATED_INJECT_AS_IS) &&
         (InjectionType != STPTI_INJECTION_TYPE_REPEATED_INJECT_WITH_CC_FIXUP) )
    {
        return( STPTI_ERROR_INVALID_INJECTION_TYPE );
    }

    Error = stpti_CarouselHandleCheck( FullCarouselHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }

    Error = stpti_CarouselEntryHandleCheck( FullEntryHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }


#ifndef STPTI_NO_USAGE_CHECK
    if (stptiMemGet_Device(FullCarouselHandle)->EntryType != STPTI_CAROUSEL_ENTRY_TYPE_TIMED)
    {
        return( ST_ERROR_BAD_PARAMETER );   /* STPTI_ERROR_ENTRY_TYPE_NOT_SUPPORTED; */
    }

    if ( (MaxOutputTime.LSW >= MinOutputTime.LSW)?
            (MaxOutputTime.Bit32 <  MinOutputTime.Bit32)
          : (MaxOutputTime.Bit32 <= MinOutputTime.Bit32) )
    {
        return( ST_ERROR_BAD_PARAMETER );   /* Min time >= Max time; */
    }
#endif

    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GeneralCarouselHandleCheck(FullHandle_t FullCarouselHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_CarouselHandleCheck( FullCarouselHandle );
    if (Error != ST_NO_ERROR)
    {
        return( Error );
    }


    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GeneralCarouselAndEntryHandleCheck(FullHandle_t FullCarouselHandle, FullHandle_t FullEntryHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_CarouselEntryHandleCheck( FullEntryHandle );
    }
    return( Error );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_CarouselSetAllowOutput(FullHandle_t FullCarouselHandle, STPTI_AllowOutput_t OutputAllowed)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    if ( (OutputAllowed != STPTI_ALLOW_OUTPUT_UNMATCHED_PIDS) &&
         (OutputAllowed != STPTI_ALLOW_OUTPUT_SELECTED_SLOTS) )
    {
        Error = STPTI_ERROR_INVALID_ALLOW_OUTPUT_TYPE;
    }
    else
    {
        Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    }
    return Error;
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_CarouselUnlinkBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stptiValidate_GeneralCarouselHandleCheck( FullCarouselHandle );
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_BufferHandleCheck( FullBufferHandle );
    }
    return Error;
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiValidate_GeneralCarouselEntryHandleCheck(FullHandle_t FullCarouselEntryHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_CarouselEntryHandleCheck( FullCarouselEntryHandle );
    if (Error == ST_NO_ERROR)
    {

    }
    return Error;
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_CarouselEntryLoad(FullHandle_t FullEntryHandle, STPTI_TSPacket_t Packet, U8 FromByte)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stptiValidate_GeneralCarouselEntryHandleCheck( FullEntryHandle );
    if (Error == ST_NO_ERROR)
    {

#ifndef STPTI_NO_USAGE_CHECK
        if ( stptiMemGet_Device(FullEntryHandle)->EntryType != STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE )
        {
            Error = ST_ERROR_BAD_PARAMETER; /* STPTI_ERROR_ENTRY_TYPE_NOT_SUPPORTED; */
        }
        else
#endif

        if( Packet == NULL )
        {
            /* This parameter is a pointer to the base of an array used for a memcpy,
               so it should never be NULL... */
            Error = ST_ERROR_BAD_PARAMETER;
        }

        else if( FromByte >= sizeof( STPTI_TSPacket_t ))
        {
            /* FromByte is an offset from the base of the Packet, so should be less than the size of the packet */
            Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    return( Error );
}


#endif  /* STPTI_CAROUSEL_SUPPORT */


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_IntBufferHandle(FullHandle_t BufferHandle)
{
    return( stpti_BufferHandleCheck( BufferHandle ) );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_BufferLevelChangeEvent(FullHandle_t FullBufferHandle, BOOL Enable)
{
    return stpti_BufferHandleCheck( FullBufferHandle );
}


/* ------------------------------------------------------------------------- */

ST_ErrorCode_t stptiValidate_ObjectEnableFeatures(FullHandle_t FullObjectHandle, STPTI_FeatureInfo_t FeatureInfo)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (FeatureInfo.Feature <= STPTI_FEATURE_MINIMUM_ENTRY || FeatureInfo.Feature >= STPTI_FEATURE_MAXIMUM_ENTRY)
    {
        Error = ST_ERROR_BAD_PARAMETER;
    }
    else
    {
        Error = stpti_HandleCheck(FullObjectHandle);
    }

#if !defined (STPTI_FRONTEND_TSMERGER)
    if (FeatureInfo.Feature == STPTI_SLOT_FEATURE_OUTPUT_WITH_TAG_BYTES)
    {
        Error = ST_ERROR_FEATURE_NOT_SUPPORTED;
    }
#endif

    return(Error);
}

#endif /* STPTI_NO_PARAMETER_CHECK */

/* EOF */

