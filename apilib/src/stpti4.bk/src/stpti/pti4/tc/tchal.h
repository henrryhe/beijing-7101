/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: tchal.h
 Description: HAL for accessing the TC & its CAM (filters)

******************************************************************************/

#ifndef __TCHAL_H
 #define __TCHAL_H

/* Includes ------------------------------------------------------------ */


#include "stddefs.h"
#include "stdevice.h"

#include "stpti.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "pti4.h"


/* ------------------------------------------------------------------------- */


/* get */

TCMainInfo_t   *TcHal_GetMainInfo( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber );

TCMainInfo_t   *TcHal_GetUnmatchedMainInfo( STPTI_TCParameters_t *STPTI_TCParameters_p );

TCGlobalInfo_t *TcHal_GetGlobalInfo( STPTI_TCParameters_t *STPTI_TCParameters_p );

TCDMAConfig_t  *TcHal_GetTCDMAConfig( STPTI_TCParameters_t *STPTI_TCParameters_p, U16 DmaNumber );

TCKey_t        *TcHal_GetDescramblerKey( STPTI_TCParameters_t *STPTI_TCParameters_p, U16 KeyNumber );

TCSystemKey_t *TcHal_GetSystemKey( STPTI_TCParameters_t *STPTI_TCParameters_p, U16 KeyNumber);

TCPESFilter_t  *TcHal_GetPESFilter( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 PesFilterNumber );


/* associate */

void TcHal_MainInfoAssociateDmaWithSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, U16 DmaNumber );
void TcHal_MainInfoUnlinkDmaWithSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber );

#if !defined ( STPTI_BSL_SUPPORT )
void TcHal_MainInfoAssociateRecordDmaWithSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, U8 DmaNumber );
void TcHal_MainInfoUnlinkRecordDmaWithSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber );
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

void TcHal_MainInfoAssociateDescramblerPtrWithSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, TCKey_t *TCKey_p );
void TcHal_MainInfoAssociatePesFilterPtrWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, TCPESFilter_t *TCPESFilter_p);


/* disassociate */

void TcHal_MainInfoDisassociatePesFilterWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber);
void TcHal_MainInfoDisassociateDescramblerWithSlot(STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber);

/* pid */

void TcHal_LookupTableSetPidForSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, STPTI_Pid_t Pid );

void TcHal_LookupTableGetPidForSlot( STPTI_TCParameters_t *STPTI_TCParameters_p, U32 SlotNumber, STPTI_Pid_t *Pid );


/* filter (CAMs) */
ST_ErrorCode_t TcCam_Initialize( FullHandle_t DeviceHandle );

ST_ErrorCode_t TcCam_Terminate ( FullHandle_t DeviceHandle );

ST_ErrorCode_t TcCam_FilterAllocateSession ( FullHandle_t DeviceHandle );

ST_ErrorCode_t TcCam_FilterFreeSession     ( FullHandle_t DeviceHandle );

int            TcCam_GetMetadataSizeInBytes( void );

U32 TcCam_GetFilterOffset( FullHandle_t FullBufferHandle );

void stptiHAL_MaskConvertToList(  U8* MetaData, STPTI_Filter_t MatchedFilterList[], U32 MaxLengthofFilterList, BOOL CRCValid, U32 *NumOfFilterMatches_p, TCPrivateData_t *PrivateData_p, FullHandle_t FullBufferHandle, BOOL DSS_MPT_Mode );

ST_ErrorCode_t TcCam_FilterAllocate  ( FullHandle_t DeviceHandle, STPTI_FilterType_t FilterType, U32 *FilterIndex );

ST_ErrorCode_t TcCam_FilterDeallocate( FullHandle_t DeviceHandle, U32  FilterIndex );

void           TcCam_ClearSectionInfoAssociations( TCSectionFilterInfo_t *TCSectionFilterInfo_p );

ST_ErrorCode_t TcCam_AssociateFilterWithSectionInfo( FullHandle_t DeviceHandle, TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex );

ST_ErrorCode_t TcCam_DisassociateFilterFromSectionInfo( FullHandle_t DeviceHandle,TCSectionFilterInfo_t *TCSectionFilterInfo_p, U32 FilterIndex );

BOOL           TcCam_SectionInfoNothingIsAssociated( TCSectionFilterInfo_t *TCSectionFilterInfo_p );

ST_ErrorCode_t TcCam_FilterSetSection(FullHandle_t FilterHandle, STPTI_FilterData_t *FilterData_p);

ST_ErrorCode_t TcCam_EnableFilter( FullHandle_t FilterHandle, BOOL enable );

void           TcCam_SetFilterEnableState( TCSessionInfo_t *TCSessionInfo_p, U32 FilterIndex, U16 TC_FilterMode, BOOL EnableFilter );

ST_ErrorCode_t TcCam_SetMode(TCSectionFilterInfo_t *TCSectionFilterInfo_p, Slot_t *Slot_p, STPTI_FilterType_t FilterType);

U32 TcCam_RelativeMetaBitMaskPosition(FullHandle_t FullFilterHandle);


#if defined(STPTI_ARCHITECTURE_PTI4_LITE)
/* Mutex protected write and read for DMA */
void WriteDMAReg32( void *reg, U32 u32value, STPTI_TCParameters_t *TC_Params_p);
U32 ReadDMAReg32( void *reg, STPTI_TCParameters_t *TC_Params_p);
#endif

#endif /* __TCHAL_H */


/* EOF --------------------------------------------------------------------- */
