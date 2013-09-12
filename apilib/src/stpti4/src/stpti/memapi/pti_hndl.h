/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_hndl.h
 Description: manage handles

******************************************************************************/

#ifndef __PTI_HNDL_H
 #define __PTI_HNDL_H


/* Includes ------------------------------------------------------------ */


#include "pti_loc.h"


/* Exported Constants -------------------------------------------------- */

#define MAX_OBJECT_HANDLES          ((U16)(2 << 11))
#define MAX_DEVICE_HANDLES          ((U16)(2 << 8))
#define MAX_SESSION_HANDLES         ((U16)(2 << 8))


/* Exported Variables -------------------------------------------------- */


extern Driver_t STPTI_Driver;


/* Exported Function Prototypes ---------------------------------------- */


int            stpti_GetNumberOfExistingDevices   ( void );
Device_t      *stpti_GetDevice                    (int DeviceNumber);
ST_ErrorCode_t stpti_GetDeviceHandleFromDeviceName(ST_DeviceName_t DeviceName, FullHandle_t *DeviceHandle, BOOL MemLockCheck);
BOOL           stpti_DeviceNameExists             (ST_DeviceName_t DeviceName);


ST_ErrorCode_t stpti_CreateDeviceHandle    (FullHandle_t *ReturnedHandle);
ST_ErrorCode_t stpti_CreateObjectHandle    (FullHandle_t  SessionHandle, ObjectType_t ObjectType, size_t Size, FullHandle_t *ReturnedHandle);
ST_ErrorCode_t stpti_CreateObjectListHandle(FullHandle_t  SessionHandle, FullHandle_t *ReturnedHandle);
ST_ErrorCode_t stpti_CreateSessionHandle   (FullHandle_t  DeviceHandle,  FullHandle_t *ReturnedHandle);

ST_ErrorCode_t stpti_ReallocateDevice      (FullHandle_t DeviceHandle,  size_t Size);
ST_ErrorCode_t stpti_ReallocateObject      (FullHandle_t ObjectHandle,  size_t Size);
ST_ErrorCode_t stpti_ReallocateSession     (FullHandle_t SessionHandle, size_t Size);

ST_ErrorCode_t stpti_DeallocateBuffer      (FullHandle_t Handle,        BOOL Force);
ST_ErrorCode_t stpti_DeallocateDescrambler (FullHandle_t Handle,        BOOL Force);
ST_ErrorCode_t stpti_DeallocateDevice      (FullHandle_t DeviceHandle,  BOOL Force);
ST_ErrorCode_t stpti_DeallocateFilter      (FullHandle_t Handle,        BOOL Force);
#if defined (STPTI_FRONTEND_HYBRID)
ST_ErrorCode_t stpti_DeallocateFrontend    (FullHandle_t FrontendHandle, BOOL Force);
#endif
ST_ErrorCode_t stpti_DeallocateObject      (FullHandle_t ObjectHandle,  BOOL Force);
ST_ErrorCode_t stpti_DeallocateSession     (FullHandle_t SessionHandle, BOOL Force);
ST_ErrorCode_t stpti_DeallocateSignal      (FullHandle_t SignalHandle,  BOOL Force);
ST_ErrorCode_t stpti_DeallocateSlot        (FullHandle_t Handle,        BOOL Force);

ST_ErrorCode_t stpti_ObjectDeviceCheck     (FullHandle_t Handle, BOOL MemLockCheck);

ST_ErrorCode_t stpti_BufferHandleCheck     (FullHandle_t Handle);
ST_ErrorCode_t stpti_DescramblerHandleCheck(FullHandle_t Handle);
ST_ErrorCode_t stpti_FilterHandleCheck     (FullHandle_t Handle);
ST_ErrorCode_t stpti_ListHandleCheck       (FullHandle_t Handle);
ST_ErrorCode_t stpti_SessionHandleCheck    (FullHandle_t Handle);
#if defined (STPTI_FRONTEND_HYBRID)
ST_ErrorCode_t stpti_FrontendHandleCheck    (FullHandle_t Handle);
ST_ErrorCode_t stpti_FrontendDisassociatePTI(FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle, BOOL PreChecked);
#endif
ST_ErrorCode_t stpti_SlotHandleCheck       (FullHandle_t Handle);
ST_ErrorCode_t stpti_SignalHandleCheck     (FullHandle_t Handle);

ST_ErrorCode_t stpti_DeviceHandleCheck     (FullHandle_t Handle);
ST_ErrorCode_t stpti_HandleCheck           (FullHandle_t Handle);

ST_ErrorCode_t stpti_AssociateObjects    (FullHandle_t SrcList, FullHandle_t SrcHandle, FullHandle_t DestList, FullHandle_t DestHandle);
ST_ErrorCode_t stpti_DissassociateObjects(FullHandle_t SrcList, FullHandle_t SrcHandle, FullHandle_t DestList, FullHandle_t DestHandle);

ST_ErrorCode_t stpti_SignalDisassociateBuffer(FullHandle_t SignalHandle, FullHandle_t BufferHandle, BOOL PreChecked);
ST_ErrorCode_t stpti_SignalDisassociateSlot  (FullHandle_t SignalHandle, FullHandle_t   SlotHandle, BOOL PreChecked);

ST_ErrorCode_t stpti_SlotDisassociateBuffer     (FullHandle_t SlotHandle, FullHandle_t BufferHandle,      BOOL PreChecked);
ST_ErrorCode_t stpti_SlotDisassociateFilter     (FullHandle_t SlotHandle, FullHandle_t FilterHandle,      BOOL PreChecked);
ST_ErrorCode_t stpti_SlotDisassociateDescrambler(FullHandle_t SlotHandle, FullHandle_t DescramblerHandle, BOOL PreChecked, BOOL FastDisasscoiate);
ST_ErrorCode_t stpti_PidDisassociateDescrambler (FullHandle_t DescramblerHandle, BOOL FastDisassociate);


#ifdef STPTI_CAROUSEL_SUPPORT
ST_ErrorCode_t stpti_BufferDisassociateCarousel     (FullHandle_t BufferHandle,   FullHandle_t CarouselHandle, BOOL PreChecked);
ST_ErrorCode_t stpti_CarouselDisassociateSimpleEntry(FullHandle_t CarouselHandle, FullHandle_t EntryHandle,    BOOL PreChecked);
ST_ErrorCode_t stpti_CarouselDisassociateTimedEntry (FullHandle_t CarouselHandle, FullHandle_t EntryHandle,    BOOL PreChecked);
ST_ErrorCode_t stpti_CarouselEntryHandleCheck       (FullHandle_t         Handle);
ST_ErrorCode_t stpti_CarouselHandleCheck            (FullHandle_t         Handle);
ST_ErrorCode_t stpti_DeallocateCarousel             (FullHandle_t         Handle, BOOL Force);
ST_ErrorCode_t stpti_DeallocateCarouselEntry        (FullHandle_t         Handle, BOOL Force);
void           stpti_CarouselUnlinkSimpleEntry      (FullHandle_t CarouselHandle, FullHandle_t EntryHandle);
void           stpti_CarouselUnlinkTimedEntry       (FullHandle_t CarouselHandle, FullHandle_t EntryHandle);
#endif


#ifndef STPTI_NO_INDEX_SUPPORT
ST_ErrorCode_t stpti_IndexHandleCheck(FullHandle_t Handle);
#endif


#endif  /* __PTI_HNDL_H */
