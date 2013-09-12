/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005-2007.  All rights reserved.

   File Name: validate.h
 Description: Check parameters to functions

******************************************************************************/

#if !defined(ST_OSLINUX)
#include "stcommon.h"
#endif /* #endif !defined(ST_OSLINUX) */
#include "stdevice.h"
#include "stpti.h"

#include "memapi/pti_hndl.h"
#include "memapi/pti_list.h"
#include "memapi/pti_mem.h"

#include "pti_loc.h"
#include "pti_hal.h"


ST_ErrorCode_t stptiValidate_IntBufferHandle(FullHandle_t BufferHandle);


/* ------------------------------------------------------------------------- */
/* pti_extended.c */

ST_ErrorCode_t stptiValidate_DescramblerAllocate(FullHandle_t FullSessionHandle, STPTI_Descrambler_t *DescramblerObject, STPTI_DescramblerType_t DescramblerType);
ST_ErrorCode_t stptiValidate_BufferSetMultiPacket(FullHandle_t FullBufferHandle, U32 NumberOfPacketsInMultiPacket);
ST_ErrorCode_t stptiValidate_BufferLevelSignalEnable( FullHandle_t FullBufferHandle, U32 BufferLevelThreshold );
ST_ErrorCode_t stptiValidate_BufferLevelSignalDisable( FullHandle_t FullBufferHandle );
ST_ErrorCode_t stptiValidate_BufferExtractData( FullHandle_t FullBufferHandle, U32 Offset, U32 NumBytesToExtract,  U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);
ST_ErrorCode_t stptiValidate_BufferExtractTSHeaderData(FullHandle_t FullBufferHandle, U32 *TSHeader);
ST_ErrorCode_t stptiValidate_BufferGetFreeLength(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferGetWritePointer(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferRead(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);
ST_ErrorCode_t stptiValidate_BufferSetOverflowControl(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferSetReadPointer(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferTestForData(FullHandle_t FullBufferHandle, U32 *BytesInBuffer);
ST_ErrorCode_t stptiValidate_SetSystemKey(ST_DeviceName_t DeviceName, U8 *Data);
ST_ErrorCode_t stptiValidate_DescramblerDeallocate(FullHandle_t FullDescramblerHandle);
ST_ErrorCode_t stptiValidate_Descrambler_AssociatePid( FullHandle_t DescramblerHandle );
ST_ErrorCode_t stptiValidate_DescramblerAssociate(FullHandle_t FullDescramblerHandle, STPTI_SlotOrPid_t SlotOrPid);
ST_ErrorCode_t stptiValidate_DescramblerDisassociate(FullHandle_t FullDescramblerHandle, STPTI_SlotOrPid_t SlotOrPid);
ST_ErrorCode_t stptiValidate_DescramblerSet(FullHandle_t FullDescramblerHandle, STPTI_KeyParity_t Parity, STPTI_KeyUsage_t Usage, U8 *Data);
ST_ErrorCode_t stptiValidate_DescramblerSetType(FullHandle_t FullDescramblerHandle, STPTI_DescramblerType_t DescramblerType);
ST_ErrorCode_t stptiValidate_DescramblerSetSVP(FullHandle_t FullDescramblerHandle, STPTI_NSAMode_t mode);
ST_ErrorCode_t stptiValidate_EnableDisableScramblingEvents(FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_FilterSetMatchAction(FullHandle_t FullFilterHandle, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable);
ST_ErrorCode_t stptiValidate_FilterHandleCheck( FullHandle_t FullFilterHandle );
ST_ErrorCode_t stptiValidate_GetCapability(ST_DeviceName_t DeviceName, STPTI_Capability_t *DeviceCapability);
ST_ErrorCode_t stptiValidate_GetInputPacketCount(ST_DeviceName_t DeviceName, U16 *Count);
ST_ErrorCode_t stptiValidate_GetPacketArrivalTime(FullHandle_t DeviceHandle, STPTI_TimeStamp_t *ArrivalTime, U16 *ArrivalTimeExtension);
ST_ErrorCode_t stptiValidate_GetPresentationSTC(ST_DeviceName_t DeviceName, STPTI_Timer_t Timer, STPTI_TimeStamp_t *TimeStamp);
ST_ErrorCode_t stptiValidate_GetCurrentPTITimer(ST_DeviceName_t DeviceName, STPTI_TimeStamp_t *TimeStamp);
ST_ErrorCode_t stptiValidate_BufferLinkToCdFifo(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_SlotSetCCControl(FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_PidQuery(ST_DeviceName_t DeviceName, STPTI_Pid_t Pid, STPTI_Slot_t *Slot_p);
ST_ErrorCode_t stptiValidate_SlotPacketCount(FullHandle_t FullSlotHandle, U16 *Count);
ST_ErrorCode_t stptiValidate_UserDataBlockMove(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p);
ST_ErrorCode_t stptiValidate_UserDataCircularAppend(U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p);
ST_ErrorCode_t stptiValidate_UserDataCircularSetup(U8 *Buffer_p, U32 BufferSize, U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p, U8 **NextData_p);
ST_ErrorCode_t stptiValidate_UserDataSynchronize(STPTI_DMAParams_t *DMAParams_p);
ST_ErrorCode_t stptiValidate_UserDataWrite(U8 *DataPtr_p, U32 DataLength, STPTI_DMAParams_t *DMAParams_p);
ST_ErrorCode_t stptiValidate_UserDataRemaining(STPTI_DMAParams_t *DMAParams_p, U32 *UserDataRemaining);
ST_ErrorCode_t stptiValidate_SlotLinkToCdFifo(FullHandle_t FullSlotHandle, STPTI_DMAParams_t *CdFifoParams);
ST_ErrorCode_t stptiValidate_BufferReadNBytes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy, U32 BytesToCopy);
ST_ErrorCode_t stptiValidate_SetStreamID(ST_DeviceName_t DeviceName, STPTI_StreamID_t StreamID);
ST_ErrorCode_t stptiValidate_SetDiscardParams(ST_DeviceName_t DeviceName, U8 NumberOfDiscardBytes);

ST_ErrorCode_t stptiValidate_BufferExtractSectionData( FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U16 MaxLengthofFilterList, U16 *NumOfFilterMatches_p, BOOL *CRCValid_p, U32 *SectionHeader_p);
ST_ErrorCode_t stptiValidate_BufferExtractPartialPesPacketData( FullHandle_t FullBufferHandle, BOOL *PayloadUnitStart_p, BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U16 *DataLength_p );
ST_ErrorCode_t stptiValidate_BufferExtractPesPacketData( FullHandle_t FullBufferHandle, U8 *PesFlags_p, U8 *TrickModeFlags_p, U32 *PESPacketLength_p, STPTI_TimeStamp_t *PTSValue_p, STPTI_TimeStamp_t *DTSValue_p );
ST_ErrorCode_t stptiValidate_BufferReadPartialPesPacket( FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, BOOL *PayloadUnitStart_p, BOOL *CCDiscontinuity_p, U8 *ContinuityCount_p, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy );
ST_ErrorCode_t stptiValidate_BufferReadPes(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);
ST_ErrorCode_t stptiValidate_FiltersFlush(FullHandle_t FullBufferHandle, STPTI_Filter_t Filters[], U16 NumOfFiltersToFlush);
ST_ErrorCode_t stptiValidate_GetPacketErrorCount(ST_DeviceName_t DeviceName, U32 *Count_p);
ST_ErrorCode_t stptiValidate_ModifyGlobalFilterState(STPTI_Filter_t FiltersToDisable[], U16 NumOfFiltersToDisable, STPTI_Filter_t FiltersToEnable[], U16 NumOfFiltersToEnable);
ST_ErrorCode_t stptiValidate_SlotDescramblingControl(FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_SlotSetAlternateOutputAction(FullHandle_t FullSlotHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag);

ST_ErrorCode_t stptiValidate_SlotQuery(FullHandle_t FullSlotHandle, BOOL *PacketsSeen, BOOL *TransportScrambledPacketsSeen, BOOL *PESScrambledPacketsSeen, STPTI_Pid_t *Pid);
ST_ErrorCode_t stptiValidate_AlternateOutputSetDefaultAction(FullHandle_t SessionHandle, STPTI_AlternateOutputType_t Method, STPTI_AlternateOutputTag_t Tag);

#if defined(STPTI_GPDMA_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferClearGPDMAParams(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferSetGPDMAParams(FullHandle_t FullBufferHandle, STPTI_GPDMAParams_t GPDMAParams);
#endif

#if defined(STPTI_PCP_SUPPORT)
ST_ErrorCode_t stptiValidate_BufferClearPCPParams(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferSetPCPParams(FullHandle_t FullBufferHandle, STPTI_PCPParams_t PCPParams);
#endif

/* ------------------------------------------------------------------------- */
#if defined (STPTI_FRONTEND_HYBRID)
/* pti_fe.c */

ST_ErrorCode_t stptiValidate_FrontendAllocate   ( FullHandle_t FullSessionHandle, STPTI_Frontend_t *FrontendHandle_p, STPTI_FrontendType_t FrontendType);
ST_ErrorCode_t stptiValidate_FrontendLinkToPTI  ( FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle );
ST_ErrorCode_t stptiValidate_FrontendSetParams  ( FullHandle_t FullFrontendHandle, STPTI_FrontendParams_t * FrontendParams_p );
ST_ErrorCode_t stptiValidate_FrontendGetParams  ( FullHandle_t FullFrontendHandle, STPTI_FrontendType_t * FrontendType_p, STPTI_FrontendParams_t * FrontendParams_p );
ST_ErrorCode_t stptiValidate_FrontendGetStatus  ( FullHandle_t FullFrontendHandle, STPTI_FrontendStatus_t * FrontendStatus_p );
ST_ErrorCode_t stptiValidate_FrontendInjectData ( FullHandle_t FullFrontendHandle, STPTI_FrontendSWTSNode_t *FrontendSWTSNode_p );
ST_ErrorCode_t stptiValidate_FrontendHandle     ( FullHandle_t FullFrontendHandle );
#endif

/* ------------------------------------------------------------------------- */
/* pti_basic.c */

ST_ErrorCode_t stptiValidate_Init(ST_DeviceName_t DeviceName, const STPTI_InitParams_t *InitParams);
ST_ErrorCode_t stptiValidate_BufferAllocate(FullHandle_t FullSessionHandle, U32 RequiredSize, U32 NumberOfPacketsInMultiPacket, STPTI_Buffer_t *BufferHandle);
ST_ErrorCode_t stptiValidate_FilterAllocate(FullHandle_t FullSessionHandle, STPTI_FilterType_t FilterType, STPTI_Filter_t *FilterObject);
ST_ErrorCode_t stptiValidate_SlotAllocate(FullHandle_t FullSessionHandle, STPTI_Slot_t *SlotHandle, STPTI_SlotData_t *SlotData);
ST_ErrorCode_t stptiValidate_BufferFlush(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferReadSection(FullHandle_t FullBufferHandle, STPTI_Filter_t MatchedFilterList[], U32 MaxLengthofFilterList, U32 *NumOfFilterMatches_p, BOOL *CRCValid_p, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);
ST_ErrorCode_t stptiValidate_Close(FullHandle_t FullSessionHandle);
ST_ErrorCode_t stptiValidate_BufferDeallocate(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_FilterDeallocate(FullHandle_t FullFilterHandle);
ST_ErrorCode_t stptiValidate_SlotDeallocate(FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_FilterAssociate(FullHandle_t FullFilterHandle, FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_FilterSetParams( FullHandle_t FullFilterHandle, STPTI_FilterData_t *FilterData_p );
ST_ErrorCode_t stptiValidate_SlotLinkToBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_SlotLinkToRecordBuffer(FullHandle_t FullSlotHandle, FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_GetBuffersFromSlot(FullHandle_t FullSlotHandle, STPTI_Buffer_t *BufferHandle_p, STPTI_Buffer_t *RecordBufferHandle_p);
ST_ErrorCode_t stptiValidate_SlotState(FullHandle_t FullSlotHandle, U32 *SlotCount_p, STPTI_ScrambleState_t *ScrambleState_p, STPTI_Pid_t *Pid_p);
ST_ErrorCode_t stptiValidate_Open(ST_DeviceName_t DeviceName, const STPTI_OpenParams_t *OpenParams, STPTI_Handle_t *Handle);
ST_ErrorCode_t stptiValidate_SignalAllocate(FullHandle_t FullSessionHandle, STPTI_Signal_t *SignalHandle);
ST_ErrorCode_t stptiValidate_SignalAssociateBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_SignalDeallocate(FullHandle_t FullSignalHandle);
ST_ErrorCode_t stptiValidate_SignalDisassociateBuffer(FullHandle_t FullSignalHandle, FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_SignalAbort(FullHandle_t FullSignalHandle);
ST_ErrorCode_t stptiValidate_Descrambler_AssociateSlot(FullHandle_t DescramblerHandle, FullHandle_t SlotHandle);
ST_ErrorCode_t stptiValidate_SlotClearPid(FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_SlotSetPid(FullHandle_t FullSlotHandle, STPTI_Pid_t Pid);
ST_ErrorCode_t stptiValidate_SlotRemapPid(FullHandle_t FullSlotHandle, STPTI_Pid_t Pid);
ST_ErrorCode_t stptiValidate_SlotUnLink(FullHandle_t FullSlotHandle);
ST_ErrorCode_t stptiValidate_BufferUnLink(FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_BufferReadTSPacket(FullHandle_t FullBufferHandle, U8 *Destination0_p, U32 DestinationSize0, U8 *Destination1_p, U32 DestinationSize1, U32 *DataSize_p, STPTI_Copy_t DmaOrMemcpy);


/* ------------------------------------------------------------------------- */
/* pti_indx.c */

#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode_t stptiValidate_IndexAllocate     ( FullHandle_t FullSessionHandle, STPTI_Index_t *IndexHandle_p);
ST_ErrorCode_t stptiValidate_IndexAssociate    ( FullHandle_t FullIndexHandle, STPTI_SlotOrPid_t SlotOrPid );
ST_ErrorCode_t stptiValidate_IndexDeallocate   ( FullHandle_t FullIndexHandle );
ST_ErrorCode_t stptiValidate_IndexStartStop    ( FullHandle_t DeviceHandle    );
ST_ErrorCode_t stptiValidate_IndexSet( FullHandle_t FullIndexHandle, STPTI_IndexDefinition_t IndexMask, U32 MPEGStartCode, U32 MPEGStartCodeMode );
ST_ErrorCode_t stptiValidate_IndexAssociateSlot( FullHandle_t IndexHandle, FullHandle_t SlotHandle);
ST_ErrorCode_t stptiValidate_IndexAssociatePid ( FullHandle_t IndexHandle     );
ST_ErrorCode_t stptiValidate_SlotDisassociateIndex(FullHandle_t SlotHandle, FullHandle_t IndexHandle, BOOL PreChecked);
ST_ErrorCode_t stptiValidate_IndexDisassociate ( FullHandle_t FullIndexHandle );
#endif
ST_ErrorCode_t stptiValidate_SlotSetCorruptionParams( FullHandle_t FullSlotHandle, U8 Offset );
/* ------------------------------------------------------------------------- */
/* pti_car.c */

#ifdef STPTI_CAROUSEL_SUPPORT
ST_ErrorCode_t stptiValidate_CarouselAllocate(FullHandle_t FullSessionHandle, STPTI_Carousel_t *Carousel);
ST_ErrorCode_t stptiValidate_CarouselLinkToBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_CarouselEntryAllocate(FullHandle_t FullSessionHandle, STPTI_CarouselEntry_t *CarouselEntryHandle);
ST_ErrorCode_t stptiValidate_CarouselInsertEntry(FullHandle_t FullCarouselHandle, STPTI_AlternateOutputTag_t AlternateOutputTag, STPTI_InjectionType_t InjectionType, FullHandle_t FullEntryHandle);
ST_ErrorCode_t stptiValidate_CarouselInsertTimedEntry(FullHandle_t FullCarouselHandle, U8 *TSPacket_p, U8 CCValue, STPTI_InjectionType_t InjectionType, STPTI_TimeStamp_t MinOutputTime, STPTI_TimeStamp_t MaxOutputTime, BOOL EventOnOutput, BOOL EventOnTimeout, U32 PrivateData, FullHandle_t FullEntryHandle, U8 FromByte);
ST_ErrorCode_t stptiValidate_GeneralCarouselHandleCheck(FullHandle_t FullCarouselHandle);
ST_ErrorCode_t stptiValidate_GeneralCarouselAndEntryHandleCheck(FullHandle_t FullCarouselHandle, FullHandle_t FullEntryHandle);
ST_ErrorCode_t stptiValidate_CarouselSetAllowOutput(FullHandle_t FullCarouselHandle, STPTI_AllowOutput_t OutputAllowed);
ST_ErrorCode_t stptiValidate_CarouselUnlinkBuffer(FullHandle_t FullCarouselHandle, FullHandle_t FullBufferHandle);
ST_ErrorCode_t stptiValidate_GeneralCarouselEntryHandleCheck(FullHandle_t FullCarouselEntryHandle);
ST_ErrorCode_t stptiValidate_CarouselEntryLoad(FullHandle_t FullEntryHandle, STPTI_TSPacket_t Packet, U8 FromByte);
#endif

ST_ErrorCode_t stptiValidate_BufferLevelChangeEvent(FullHandle_t FullBufferHandle, BOOL Enable);

/* ------------------------------------------------------------------------- */

/* EOF */




