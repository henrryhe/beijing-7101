/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005-2007.  All rights reserved.

   File Name: pti_hndl.c
 Description: manage handles

******************************************************************************/

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif

/* Includes ---------------------------------------------------------------- */

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>

#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"

#include "memget.h"

#ifdef ST_OSLINUX
#include "pti_linux.h"
#include "stpti4_core_proc.h"
#include <linux/syscalls.h>
#include <linux/bigphysarea.h>
#endif

#if defined (STPTI_FRONTEND_HYBRID)
#include "frontend.h"
#endif

/* Private Function Prototypes --------------------------------------------- */
#ifdef ST_OSLINUX
void WalkBlockAndFree( MemCtl_t *MemCtl_p);
#endif

#if !defined ( STPTI_BSL_SUPPORT )
static ST_ErrorCode_t stpti_BufferClearSignals(FullHandle_t BufferHandle);
static ST_ErrorCode_t stpti_BufferClearSlots(FullHandle_t BufferHandle);
static ST_ErrorCode_t stpti_DescramblerClearSlots(FullHandle_t DescramblerHandle);
static ST_ErrorCode_t stpti_FilterClearSlots(FullHandle_t FilterHandle);
static ST_ErrorCode_t stpti_SignalClearBuffers(FullHandle_t SignalHandle);
static ST_ErrorCode_t stpti_SlotClearBuffers(FullHandle_t SlotHandle);
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

#ifndef STPTI_NO_INDEX_SUPPORT
static ST_ErrorCode_t stpti_SlotClearIndex(FullHandle_t SlotHandle);
#endif

#if !defined ( STPTI_BSL_SUPPORT )
static ST_ErrorCode_t stpti_SlotClearDescramblers(FullHandle_t SlotHandle);
static ST_ErrorCode_t stpti_SlotClearFilters(FullHandle_t SlotHandle);
#endif

#ifdef STPTI_CAROUSEL_SUPPORT
static ST_ErrorCode_t stpti_BufferClearCarousels(FullHandle_t BufferHandle);
#endif

#if defined( STPTI_FDMA_SUPPORT )
extern STFDMA_Node_t *OriginalFDMANode_p[3];
#endif
/* Functions --------------------------------------------------------------- */


/******************************************************************************
Function Name : stpti_DeviceNameExists
  Description :
   Parameters :
******************************************************************************/
int stpti_GetNumberOfExistingDevices(void)
{
    return( STPTI_Driver.MemCtl.MaxHandles );   /* Note: 0..n-1 */
}


/******************************************************************************
Function Name : stpti_GetDevice
  Description :
   Parameters :
******************************************************************************/
Device_t *stpti_GetDevice(int DeviceNumber)
{
    Cell_t *Cell_p;

    if ( DeviceNumber >= stpti_GetNumberOfExistingDevices() )
        return( NULL );

    Cell_p = STPTI_Driver.MemCtl.Handle_p[ DeviceNumber ].Data_p;
    if ( Cell_p == NULL )
        return( NULL );

    return( (Device_t *) &Cell_p->data );
}


/******************************************************************************
Function Name : stpti_GetDeviceHandleFromDeviceName
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_GetDeviceHandleFromDeviceName(ST_DeviceName_t DeviceName, FullHandle_t *DeviceHandle, BOOL MemLockCheck)
{
    U16 indx;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    /* It is advisable to use MemLockCheck == TRUE at all times unless the call is to run at
    interrupt level */
    if(MemLockCheck)
    {
        assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */
    }

    /* Get Handle to List of Devices */

    for (indx = 0; indx < stpti_GetNumberOfExistingDevices(); ++indx)
    {
        Device_t *Device_p = stpti_GetDevice( indx );

        if (Device_p != NULL)
        {
            if ( !strncmp( (void *)Device_p->DeviceName, DeviceName, sizeof(ST_DeviceName_t)) )
            {
                STTBX_Print(("\nstpti_GetDeviceHandleFromDeviceName: Found '%s'\n", DeviceName));
                /* Names are equal */
                break;
            }
        }
    }

    if (indx == stpti_GetNumberOfExistingDevices() )
    {
        Error = ST_ERROR_UNKNOWN_DEVICE;
    }
    else
    {
        DeviceHandle->word = STPTI_Driver.MemCtl.Handle_p[indx].Hndl_u.word;

        Error = ST_NO_ERROR;
        STTBX_Print(("\nstpti_GetDeviceHandleFromDeviceName: DeviceHandle->word '%x'\n", DeviceHandle->word ));
    }

    return Error;
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_DeviceNameExists
  Description :
   Parameters :
******************************************************************************/
BOOL stpti_DeviceNameExists(ST_DeviceName_t DeviceName)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t   DeviceHandle;

    Error = stpti_GetDeviceHandleFromDeviceName( DeviceName, &DeviceHandle, FALSE );
    if ( Error == ST_ERROR_UNKNOWN_DEVICE)
        return( FALSE );

    return( TRUE );
}

/******************************************************************************
Function Name : stpti_ObjectHandleCheck
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_ObjectHandleCheck(FullHandle_t Handle, ObjectType_t ObjectType)
{
    /* Check Type */
    if (Handle.member.ObjectType != ObjectType)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if (ST_NO_ERROR != stpti_HandleCheck(Handle))
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    if (stptiMemGet_Session(Handle)->MemCtl.Handle_p[Handle.member.Object].Hndl_u.word != Handle.word)
    {
        return ST_ERROR_INVALID_HANDLE;
    }

    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stpti_HandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_HandleCheck(FullHandle_t Handle)
{
    /* Check Device exists */
    {
        U16 Device = Handle.member.Device;

        if (Device >= stpti_GetNumberOfExistingDevices() )
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    /* Check Session exists */
    {
        U16 Session = Handle.member.Session;

        if (Session >= stptiMemGet_Device(Handle)->MemCtl.MaxHandles)
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (stptiMemGet_Device(Handle)->MemCtl.Handle_p[Session].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    /* Check Object exists */
    {
        U16 Object = Handle.member.Object;

        if (Object >= stptiMemGet_Session(Handle)->MemCtl.MaxHandles)
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (stptiMemGet_Session(Handle)->MemCtl.Handle_p[Object].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stpti_DeviceHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeviceHandleCheck(FullHandle_t Handle)
{
    /* Check Device exists */
    {
        U16 Device = Handle.member.Device;

        if (Device >= stpti_GetNumberOfExistingDevices() )
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stpti_BufferHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_BufferHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_BUFFER);
    if (Error == ST_ERROR_INVALID_HANDLE)
    {
        Error = STPTI_ERROR_INVALID_BUFFER_HANDLE;
    }

    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Buffer(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INVALID_BUFFER_HANDLE;
    }
    return Error;
}


/******************************************************************************
Function Name : stpti_DescramblerHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DescramblerHandleCheck(FullHandle_t Handle)
{
#ifdef STPTI_LOADER_SUPPORT
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_DESCRAMBLER);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE;

    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Descrambler(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INVALID_DESCRAMBLER_HANDLE;
    }
    return Error;
#endif
}

#if defined (STPTI_FRONTEND_HYBRID)
/******************************************************************************
Function Name : stpti_FrontendHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_FrontendHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_FRONTEND);
    if (Error == ST_ERROR_INVALID_HANDLE)
    {
        Error = ST_ERROR_INVALID_HANDLE;
    }

    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Frontend(Handle))->Handle != Handle.word)
            Error = ST_ERROR_INVALID_HANDLE;
    }
    return Error;
}
#endif

/******************************************************************************
Function Name : stpti_ObjectDeviceCheck
  Description : check that the handle has been allocated on a valid device
   Parameters : FullHandle_t Handle - A handle to any object that has been allocated.
                BOOL MemLockCheck - If TRUE check for mem lock being set. Only done
           		if call is not to be made from an ISR.

       Note:  It is advisable to use MemLockCheck == TRUE at all times unless
             the call is to run at interrupt level
******************************************************************************/
ST_ErrorCode_t stpti_ObjectDeviceCheck(FullHandle_t Handle, BOOL MemLockCheck)
{
    /* check that the handle has been allocated on a valid device */
    if(MemLockCheck)
    {
      assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0);
    }

    /* Check Device exists */
    {
        U16 Device = Handle.member.Device;

        if (Device >= stpti_GetNumberOfExistingDevices())
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    return ST_NO_ERROR;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


#ifdef STPTI_CAROUSEL_SUPPORT
/******************************************************************************
Function Name : stpti_CarouselHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CarouselHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_CAROUSEL);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INVALID_CAROUSEL_HANDLE;
    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Carousel(Handle))->Handle != Handle.word)
        {
            Error = STPTI_ERROR_INVALID_CAROUSEL_HANDLE;
        }
    }
    return Error;

}


/******************************************************************************
Function Name : stpti_CarouselEntryHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CarouselEntryHandleCheck(FullHandle_t Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_CAROUSEL_ENTRY);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INVALID_CAROUSEL_ENTRY_HANDLE;
    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Carousel(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INVALID_CAROUSEL_ENTRY_HANDLE;
    }
    return Error;
}
#endif /* #ifdef STPTI_CAROUSEL_SUPPORT */


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_FilterHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_FilterHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_FILTER);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INVALID_FILTER_HANDLE;
    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Filter(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INVALID_FILTER_HANDLE;
    }
    return Error;
}

/******************************************************************************
Function Name : stpti_SignalHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SignalHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_SIGNAL);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INVALID_SIGNAL_HANDLE;
    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Signal(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INVALID_SIGNAL_HANDLE;
    }
    return Error;
}


/******************************************************************************
Function Name : stpti_SlotHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SlotHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_SLOT);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INVALID_SLOT_HANDLE;

    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Slot(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INVALID_SLOT_HANDLE;
    }
    return Error;
}


#ifndef STPTI_NO_INDEX_SUPPORT
/******************************************************************************
Function Name : stpti_IndexHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_IndexHandleCheck(FullHandle_t Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Error = stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_INDEX);
    if (Error == ST_ERROR_INVALID_HANDLE)
        Error = STPTI_ERROR_INDEX_INVALID_HANDLE;
    if (Error == ST_NO_ERROR)
    {
        if ((stptiMemGet_Index(Handle))->Handle != Handle.word)
            Error = STPTI_ERROR_INDEX_INVALID_HANDLE;
    }
    return Error;
}
#endif


/******************************************************************************
Function Name : stpti_ListHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_ListHandleCheck(FullHandle_t Handle)
{
    return stpti_ObjectHandleCheck(Handle, OBJECT_TYPE_LIST);
}


/******************************************************************************
Function Name : stpti_SessionHandleCheck
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SessionHandleCheck(FullHandle_t Handle)
{

    /* check that handle is a session handle and that it exists in the handle array */
    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    /* Check Type && Instance should be same as session */

    if (Handle.member.ObjectType != OBJECT_TYPE_SESSION || Handle.member.Session != Handle.member.Object)
        return ST_ERROR_INVALID_HANDLE;

    /* Check Device exists */
    {
        U16 Device = Handle.member.Device;

        if (Device >= stpti_GetNumberOfExistingDevices())
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    /* Check Session exists */
    {
        U16 Session = Handle.member.Session;

        if (Session >= stptiMemGet_Device(Handle)->MemCtl.MaxHandles)
        {
            return ST_ERROR_INVALID_HANDLE;
        }

        if (stptiMemGet_Device(Handle)->MemCtl.Handle_p[Session].Hndl_u.word == STPTI_NullHandle())
        {
            return ST_ERROR_INVALID_HANDLE;
        }
    }

    return ST_NO_ERROR;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stpti_CreateDeviceHandle
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CreateDeviceHandle(FullHandle_t * ReturnedHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t Handle;
    U16 Device;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Error = stpti_CreateObject(&STPTI_Driver.MemCtl, &Device, sizeof(Device_t));
    if (Error == ST_NO_ERROR)
    {
        Handle.member.Device = Device;
        Handle.member.Session = 0;
        Handle.member.ObjectType = OBJECT_TYPE_DEVICE;
        Handle.member.Object = Device;

        STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u.word = Handle.word;
        STPTI_Driver.MemCtl.Handle_p[Device].Data_p->Header.Handle.word = Handle.word;

        *ReturnedHandle = Handle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_CreateSessionHandle
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CreateSessionHandle(FullHandle_t DeviceHandle, FullHandle_t * ReturnedHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t Handle;
    U16 Session;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Error = stpti_CreateObject(&stptiMemGet_Device(DeviceHandle)->MemCtl, &Session, sizeof(Session_t));

    if (Error == ST_NO_ERROR)
    {
        Handle.member.Device = DeviceHandle.member.Device;
        Handle.member.Session = Session;
        Handle.member.ObjectType = OBJECT_TYPE_SESSION;
        Handle.member.Object = Session;

        stptiMemGet_Device(DeviceHandle)->MemCtl.Handle_p[Session].Hndl_u.word = Handle.word;
        stptiMemGet_Device(DeviceHandle)->MemCtl.Handle_p[Session].Data_p->Header.Handle.word = Handle.word;

        *ReturnedHandle = Handle;
    }
    return Error;
}

#ifdef ST_OSLINUX
void WalkBlockAndFree( MemCtl_t *MemCtl_p)
{
    /* Walk Block_p */
    {
    Cell_t *CellPtr;
    CellPtr = MemCtl_p->Block_p;

    printk(KERN_ALERT"Block_p:\n");
    while(CellPtr){
        printk(KERN_ALERT"%08x %08x %08x %04d.%01d.%01d.%01d %08x %08x\n",
                    (U32)CellPtr,(U32)CellPtr->Header.next_p,(U32)CellPtr->Header.prev_p,
                    (U32)CellPtr->Header.Handle.member.Object,
                    (U32)CellPtr->Header.Handle.member.ObjectType,
                    (U32)CellPtr->Header.Handle.member.Session,
                    (U32)CellPtr->Header.Handle.member.Device,
                    (U32)CellPtr->Header.Size,
                    (U32)&CellPtr->data);
        CellPtr = CellPtr->Header.next_p;
    }
    }

    /* Walk FreeList_p */
    {
    Cell_t *CellPtr;
    CellPtr = MemCtl_p->FreeList_p;

    printk(KERN_ALERT"FreeList_p:\n");
    while(CellPtr){
        printk(KERN_ALERT"%08x %08x %08x %04d.%01d.%01d.%01d %08x %08x\n",
                    (U32)CellPtr,(U32)CellPtr->Header.next_p,(U32)CellPtr->Header.prev_p,
                    (U32)CellPtr->Header.Handle.member.Object,
                    (U32)CellPtr->Header.Handle.member.ObjectType,
                    (U32)CellPtr->Header.Handle.member.Session,
                    (U32)CellPtr->Header.Handle.member.Device,
                    (U32)CellPtr->Header.Size,
                    (U32)&CellPtr->data);
        CellPtr = CellPtr->Header.next_p;
    }
    }
}
#endif

/******************************************************************************
Function Name : stpti_CreateObjectHandle
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CreateObjectHandle(FullHandle_t SessionHandle, ObjectType_t ObjectType, size_t Size, FullHandle_t * ReturnedHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t Handle;
    FullHandle_t ListHandle;
    U16 Object;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Error = stpti_CreateObject(&stptiMemGet_Session(SessionHandle)->MemCtl, &Object, Size);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }

    Handle.member.Device = SessionHandle.member.Device;
    Handle.member.Session = SessionHandle.member.Session;
    Handle.member.ObjectType = ObjectType;
    Handle.member.Object = Object;

    stptiMemGet_Session(SessionHandle)->MemCtl.Handle_p[Object].Hndl_u.word = Handle.word;
    stptiMemGet_Session(SessionHandle)->MemCtl.Handle_p[Object].Data_p->Header.Handle.word = Handle.word;

    /* Add Object to List */

    ListHandle = stptiMemGet_Session(SessionHandle)->AllocatedList[ObjectType];
    if (ListHandle.word == STPTI_NullHandle())
    {
        /* Create a list object */
        Error = stpti_CreateListObject(SessionHandle, &ListHandle);
        if (Error != ST_NO_ERROR)
        {
            return Error;
        }
        stptiMemGet_Session(SessionHandle)->AllocatedList[ObjectType] = ListHandle;
    }

    Error = stpti_AddObjectToList(ListHandle, Handle);
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }

    ReturnedHandle->word = Handle.word;

    return ST_NO_ERROR;
}


/******************************************************************************
Function Name :stpti_CreateObjectListHandle
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CreateObjectListHandle(FullHandle_t SessionHandle, FullHandle_t * ReturnedHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t Handle;
    U16 Instance;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Error = stpti_CreateObject(&stptiMemGet_Session(SessionHandle)->MemCtl, &Instance, sizeof(List_t));
    if (Error == ST_NO_ERROR)
    {
        Handle.word = SessionHandle.word;
        Handle.member.ObjectType = OBJECT_TYPE_LIST;
        Handle.member.Object = Instance;

        stptiMemGet_Session(SessionHandle)->MemCtl.Handle_p[Instance].Hndl_u.word = Handle.word;
        stptiMemGet_Session(SessionHandle)->MemCtl.Handle_p[Instance].Data_p->Header.Handle.word = Handle.word;

        *ReturnedHandle = Handle;
    }
    return Error;
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_DeallocateDevice
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateDevice(FullHandle_t DeviceHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SessionHandle;
    U16 MaxHandles;
    U16 Session;                /* all sessions across pti cells */
    BOOL LastDevice  = FALSE;   /* pti cells */


    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    MaxHandles = stptiMemGet_Device(DeviceHandle)->MemCtl.MaxHandles;

    STTBX_Print(("stpti_DeallocateDevice::MaxSessions=%d\n", MaxHandles ));

    for (Session = 0; Session < MaxHandles; ++Session)
    {
        SessionHandle = stptiMemGet_Device(DeviceHandle)->MemCtl.Handle_p[Session].Hndl_u;

        if (SessionHandle.word != STPTI_NullHandle())
        {
            STTBX_Print(("stpti_DeallocateDevice::SessionHandle=0x%x\n", SessionHandle.word ));

            if ( Force == FALSE )
                return( ST_ERROR_DEVICE_BUSY );

            Error = stpti_DeallocateSession(SessionHandle, Force);
            STTBX_Print(("stpti_DeallocateDevice::stpti_DeallocateSession=%d\n", Error ));

            if (Error != ST_NO_ERROR)
                return( Error );
        }
    }

#if defined( ST_OSLINUX )
    STTBX_Print(("Remove from proc FS\n"));
    RemovePTIDeviceFromProcFS( DeviceHandle );    
#endif
        
    /* --- Check if this is the last device --- */

    {
        U16 MaxDevices;
        U32 Device;
        U32 UsedDevices = 0;
        FullHandle_t DevHandle;

        MaxDevices = stpti_GetNumberOfExistingDevices();

        for (Device = 0; Device < MaxDevices; ++Device)
        {
            DevHandle = STPTI_Driver.MemCtl.Handle_p[Device].Hndl_u;

            if (DevHandle.word != STPTI_NullHandle()) UsedDevices++;
       }

        if (UsedDevices == 1) LastDevice = TRUE;
        STTBX_Print(("stpti_DeallocateDevice::UsedDevices=%d\n", UsedDevices));
    }

    Error = stptiHAL_Term(DeviceHandle);

    /* --- remove session data --- */

    Error = stpti_EventTerm(DeviceHandle);

    Error = stpti_MemTerm(&(stptiMemGet_Device(DeviceHandle)->MemCtl));
    if (Error != ST_NO_ERROR)
        return( Error );

    stpti_MemDealloc(&STPTI_Driver.MemCtl, DeviceHandle.member.Device);

    /* --- If this was the last device then deallocate stpti infrastructure memory --- */

    if ( LastDevice )
    {
        STTBX_Print(("stpti_DeallocateDevice::deallocation for LAST <Device> in stpti\n"));

        Error = stpti_EventQueueTerm(); /* Kill event notification process */
        if (Error != ST_NO_ERROR)
            return( Error );

        Error = stpti_InterruptQueueTerm(); /* Kill Interrupt process */
        if (Error != ST_NO_ERROR)
            return( Error );

        Error = stpti_MemTerm(&STPTI_Driver.MemCtl);    /* Deallocate Driver memory */
    }

    return( ST_NO_ERROR );
}

/******************************************************************************
Function Name : stpti_DeallocateSession
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateSession(FullHandle_t SessionHandle, BOOL Force)
{
    ObjectType_t ObjType;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    if (!Force)
    {
        /* Check for 'owned' objects that are still allocated */
        for (ObjType = 0; ObjType <= OBJECT_TYPE_LIST; ++ObjType)
        {
            FullHandle_t *ListHandle_p = &stptiMemGet_Session(SessionHandle)->AllocatedList[ObjType];

            if (ListHandle_p->word != STPTI_NullHandle())
            {
                return STPTI_ERROR_INTERNAL_OBJECT_ALLOCATED;
            }
        }
    }
    else
    {
        /* Deallocate All Objects */
        for (ObjType = 0; ObjType <= OBJECT_TYPE_LIST; ++ObjType)
        {
            FullHandle_t ListHandle = stptiMemGet_Session(SessionHandle)->AllocatedList[ObjType];

            while (ListHandle.word != STPTI_NullHandle())
            {
                Error = stpti_DeallocateNextObjectInList(ListHandle);
                if (Error != ST_NO_ERROR)
                {
                    return Error;
                }
                ListHandle = stptiMemGet_Session(SessionHandle)->AllocatedList[ObjType];
            }
        }
    }

    /* Free Session's memory */

    Error = stpti_MemTerm(&(stptiMemGet_Session(SessionHandle)->MemCtl));
    if (Error != ST_NO_ERROR)
    {
        return Error;
    }

    stpti_MemDealloc(&(stptiMemGet_Device(SessionHandle)->MemCtl), SessionHandle.member.Session);

#if defined( STPTI_FDMA_SUPPORT )
    memory_deallocate(stptiMemGet_Device(SessionHandle)->NonCachedPartition_p, OriginalFDMANode_p[stptiMemGet_Device(SessionHandle)->Session]);
#endif

    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stpti_DeallocateObject
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateObject(FullHandle_t ObjectHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    switch (ObjectHandle.member.ObjectType)
    {
    case OBJECT_TYPE_BUFFER:
        Error = stpti_DeallocateBuffer(ObjectHandle, Force);
        break;

#ifdef STPTI_CAROUSEL_SUPPORT
    case OBJECT_TYPE_CAROUSEL:
        Error = stpti_DeallocateCarousel(ObjectHandle, Force);
        break;

    case OBJECT_TYPE_CAROUSEL_ENTRY:
        Error = stpti_DeallocateCarouselEntry(ObjectHandle, Force);
        break;
#endif

    case OBJECT_TYPE_DESCRAMBLER:
        Error = stpti_DeallocateDescrambler(ObjectHandle, Force);
        break;

    case OBJECT_TYPE_FILTER:
        Error = stpti_DeallocateFilter(ObjectHandle, Force);
        break;

    case OBJECT_TYPE_SIGNAL:
        Error = stpti_DeallocateSignal(ObjectHandle, Force);
        break;

    case OBJECT_TYPE_SLOT:
        Error = stpti_DeallocateSlot(ObjectHandle, Force);
        break;
#if defined (STPTI_FRONTEND_HYBRID)
    case OBJECT_TYPE_FRONTEND:
        Error = stpti_DeallocateFrontend(ObjectHandle, Force);
        break; 
#endif
#ifndef STPTI_NO_INDEX_SUPPORT
    case OBJECT_TYPE_INDEX:
        Error = stpti_DeallocateIndex(ObjectHandle, Force);
        break;
#endif
    case OBJECT_TYPE_SESSION:
    case OBJECT_TYPE_DEVICE:
    case OBJECT_TYPE_DRIVER:
    case OBJECT_TYPE_LIST:

        /* Fall Through */

    default:
        Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        break;
    }
    return Error;
}


/******************************************************************************
Function Name : stpti_DissassociateObjects
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DissassociateObjects(FullHandle_t SrcList, FullHandle_t SrcHandle, FullHandle_t DestList, FullHandle_t DestHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Error = stpti_RemoveObjectFromList(SrcList, SrcHandle);
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_RemoveObjectFromList(DestList, DestHandle);
    }
    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */


/******************************************************************************
Function Name : stpti_AssociateObjects
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_AssociateObjects(FullHandle_t SrcList, FullHandle_t SrcHandle, FullHandle_t DestList, FullHandle_t DestHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    /* Add to source list and destination lists */

    Error = stpti_AddObjectToList(SrcList, DestHandle);
    if (Error == ST_NO_ERROR)
    {
        Error = stpti_AddObjectToList(DestList, SrcHandle);
#if !defined ( STPTI_BSL_SUPPORT )
        if (Error != ST_NO_ERROR)
        {
            (void) stpti_RemoveObjectFromList(SrcList, DestHandle);
        }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    }
    return Error;
}


#if !defined ( STPTI_BSL_SUPPORT )
/******************************************************************************
Function Name : stpti_DeallocateDescrambler
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateDescrambler(FullHandle_t Handle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    if ((stptiMemGet_Descrambler(Handle))->SlotListHandle.word != STPTI_NullHandle())
    {
        if (Force)
        {
            Error = stpti_DescramblerClearSlots(Handle);
            if (Error != ST_NO_ERROR)
                return Error;
        }
        else
            return STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
    }

    Error = stptiHAL_DescramblerDeallocate(Handle);

    if (Error != ST_NO_ERROR)
        return Error;

    /* Remove from session lists */
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;
        FullHandle_t ListHandle;

        ListHandle = stptiMemGet_Session(Handle)->AllocatedList[Handle.member.ObjectType];
        Error = stpti_RemoveObjectFromList(ListHandle, Handle);
        if (Error != ST_NO_ERROR)
            return Error;
        stpti_ResizeObjectList(&stptiMemGet_Session(Handle)->AllocatedList[Handle.member.ObjectType]);
    }

    /* Blow Descrambler away ! */

    stpti_MemDealloc(&(stptiMemGet_Session(Handle)->MemCtl), Handle.member.Object);

    return ST_NO_ERROR;

}

/******************************************************************************
Function Name : stpti_DeallocateFilter
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateFilter(FullHandle_t FilterHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t MatchFilterListHandle;
    U32 TC_FilterIdent;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    if ((stptiMemGet_Filter(FilterHandle))->SlotListHandle.word != STPTI_NullHandle())
    {
        if (Force)
        {
            ST_ErrorCode_t Error = ST_NO_ERROR;

            Error = stpti_FilterClearSlots(FilterHandle);
            if (Error != ST_NO_ERROR)
                return Error;
        }
        else
            return STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
    }

    /* Remove 'Filter Match Action' List */

    MatchFilterListHandle = (stptiMemGet_Filter(FilterHandle))->MatchFilterListHandle;
    if (MatchFilterListHandle.word != STPTI_NullHandle())
    {
        stpti_MemDealloc(&(stptiMemGet_Session(MatchFilterListHandle)->MemCtl), MatchFilterListHandle.member.Object);
        (stptiMemGet_Filter(FilterHandle))->MatchFilterListHandle.word = STPTI_NullHandle();
    }

    TC_FilterIdent = (stptiMemGet_Filter(FilterHandle))->TC_FilterIdent;

    switch ((stptiMemGet_Filter(FilterHandle))->Type)
    {
        case STPTI_FILTER_TYPE_TSHEADER_FILTER:
            Error = stptiHAL_FilterDeallocateTransport(FilterHandle, TC_FilterIdent);
            break;

        case STPTI_FILTER_TYPE_PES_FILTER:
            Error = stptiHAL_FilterDeallocatePES(FilterHandle, TC_FilterIdent);
            break;

        case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
	        Error = stptiHAL_FilterDeallocatePESStreamId(FilterHandle, TC_FilterIdent);
    	    break;

        case STPTI_FILTER_TYPE_ECM_FILTER:
            Error = stptiHAL_FilterDeallocateSection(FilterHandle, TC_FilterIdent);           /* ECM filters are section filters */
            break;

        case STPTI_FILTER_TYPE_EMM_FILTER:
            Error = stptiHAL_FilterDeallocateSection(FilterHandle, TC_FilterIdent);           /* EMM filters are section filters */
            break;
                        
#ifdef STPTI_DC2_SUPPORT
        case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
            Error = stptiHAL_DC2FilterDeallocateMulticast(FilterHandle, TC_FilterIdent);
            break;
#endif
        default:
            Error = stptiHAL_FilterDeallocateSection(FilterHandle, TC_FilterIdent);
            break;
    }

    if (Error != ST_NO_ERROR)
        return Error;

    /* Remove from session lists */
    {
        ST_ErrorCode_t Error = ST_NO_ERROR;
        FullHandle_t ListHandle;

        ListHandle = stptiMemGet_Session(FilterHandle)->AllocatedList[FilterHandle.member.ObjectType];
        Error = stpti_RemoveObjectFromList(ListHandle, FilterHandle);
        if (Error != ST_NO_ERROR)
            return Error;
        stpti_ResizeObjectList(&stptiMemGet_Session(FilterHandle)->AllocatedList[FilterHandle.member.ObjectType]);
    }

    /* Blow Filter away ! */
    stpti_MemDealloc(&(stptiMemGet_Session(FilterHandle)->MemCtl), FilterHandle.member.Object);
    return ST_NO_ERROR;
}


/******************************************************************************
Function Name : stpti_DeallocateSignal
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateSignal(FullHandle_t SignalHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

#if defined( ST_OSLINUX )
    /* Remove from the proc file system */
    /* This is removed here because it can be called when term is forced.
       In which case STPTI_SignalDeallocate() is not called. */
    RemovePTISignalFromProcFS(SignalHandle);
#endif

    if ((stptiMemGet_Signal(SignalHandle))->BufferListHandle.word != STPTI_NullHandle())
    {
        if (Force)
        {
            Error = stpti_SignalClearBuffers(SignalHandle); /* Clear Buffer associations */
        }
        else
            Error = STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
    }

    if (Error == ST_NO_ERROR)
    {
        FullHandle_t ListHandle;

        message_queue_t *SignalQueue_p;

        /* Destroy queue */

        SignalQueue_p = (stptiMemGet_Signal(SignalHandle))->Queue_p;
        assert(SignalQueue_p != NULL);

        (stptiMemGet_Signal(SignalHandle))->Queue_p = NULL; /* this stops data being added to
                                                                    the queue */

        /* Wait for every one to stop listening on the queue */

        if (SignalQueue_p != NULL)
        {
            while ((stptiMemGet_Signal (SignalHandle))->QueueWaiting)
            {
                STPTI_Buffer_t *buffer_p = (STPTI_Buffer_t *)message_claim (SignalQueue_p);

                *buffer_p = STPTI_NullHandle ();
                message_send (SignalQueue_p, buffer_p);

                STOS_SemaphoreSignal (stpti_MemoryLock);

#if defined(ST_OSLINUX)
                schedule();
#else
                task_delay (10);        /* De-shedule */
#endif /* #endif defined(ST_OSLINUX) */
                STOS_SemaphoreWait (stpti_MemoryLock);
            }
        }

        /* run round in circles emptying queue */

        while (TRUE)
        {
            STPTI_Buffer_t *buffer_p;

            buffer_p = message_receive_timeout(SignalQueue_p, TIMEOUT_IMMEDIATE);
            if (buffer_p != NULL)
                message_release(SignalQueue_p, buffer_p);
            else
                break;
        }

        /* now it's safe to delete the queue */
        message_delete_queue(SignalQueue_p);

        /* Remove from session lists */

        ListHandle = stptiMemGet_Session(SignalHandle)->AllocatedList[SignalHandle.member.ObjectType];

        Error = stpti_RemoveObjectFromList(ListHandle, SignalHandle);

        if (Error == ST_NO_ERROR)
        {
            stpti_ResizeObjectList(&stptiMemGet_Session(SignalHandle)->AllocatedList[SignalHandle.member.ObjectType]);

            /* Blow Signal away ! */
            stpti_MemDealloc(&(stptiMemGet_Session(SignalHandle)->MemCtl), SignalHandle.member.Object);
        }
    }
    return Error;
}


/******************************************************************************
Function Name : stpti_DeallocateSlot
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateSlot(FullHandle_t Handle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 TC_SlotIdent;

    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    if (((stptiMemGet_Slot(Handle))->DescramblerListHandle.word != STPTI_NullHandle())
        || ((stptiMemGet_Slot(Handle))->FilterListHandle.word != STPTI_NullHandle())
        || ((stptiMemGet_Slot(Handle))->BufferListHandle.word != STPTI_NullHandle())
#ifndef STPTI_NO_INDEX_SUPPORT
        || ((stptiMemGet_Slot(Handle))->IndexListHandle.word != STPTI_NullHandle())
#endif
        )
    {
        if (Force)
        {
            Error = stpti_SlotClearDescramblers(Handle); /* Clear Descrambler Associations */
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_SlotClearFilters(Handle); /* Clear Filter Associations */
                if (Error == ST_NO_ERROR)
                {
                    Error = stpti_SlotClearBuffers(Handle); /* Clear Buffer Associations */
#ifndef STPTI_NO_INDEX_SUPPORT
                    if (Error == ST_NO_ERROR)
                    {
                        Error = stpti_SlotClearIndex(Handle); /* Clear Index Associations */
                    }
#endif
                }
            }
        }
        else
        {
            Error = STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
        }
    }

    if (Error == ST_NO_ERROR)
    {
        TC_SlotIdent = (stptiMemGet_Slot(Handle))->TC_SlotIdent;

        Error = stptiHAL_SlotDeallocate(Handle, TC_SlotIdent);

        if (Error == ST_NO_ERROR)
        {
            FullHandle_t ListHandle;

            /* Remove from session lists */
            ListHandle = stptiMemGet_Session(Handle)->AllocatedList[Handle.member.ObjectType];
            Error = stpti_RemoveObjectFromList(ListHandle, Handle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&stptiMemGet_Session(Handle)->AllocatedList[Handle.member.ObjectType]);

                /* Blow Slot away ! */
                stpti_MemDealloc(&(stptiMemGet_Session(Handle)->MemCtl), Handle.member.Object);
            }
        }
    }

    return Error;
}

#if defined (STPTI_FRONTEND_HYBRID)
/******************************************************************************
Function Name : stpti_DeallocateFrontend
  Description : Free resources for a frontend object
   Parameters : FrontendHandle
                Force
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateFrontend(FullHandle_t FrontendHandle, BOOL Force)
{
    Session_t * Session_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

    Session_p = stptiMemGet_Session( FrontendHandle );

    if ((stptiMemGet_Frontend(FrontendHandle))->SessionListHandle.word != STPTI_NullHandle() )
    {
        if (Force)
        {
            FullHandle_t SessionHandle;

            SessionHandle.word = (&stptiMemGet_List((stptiMemGet_Frontend(FrontendHandle))->SessionListHandle )->Handle)[0];

            Error = stpti_FrontendDisassociatePTI( FrontendHandle, SessionHandle, TRUE);

            if (Error != ST_NO_ERROR)
                return Error;
        }
        else
            return STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
    }

    if ( (stptiMemGet_Frontend(FrontendHandle))->Type == STPTI_FRONTEND_TYPE_TSIN )
    {

        if ( NULL != (stptiMemGet_Frontend(FrontendHandle))->RAMRealStart_p)
        {
            STOS_MemoryDeallocate( Session_p->NonCachedPartition_p, (stptiMemGet_Frontend(FrontendHandle))->RAMRealStart_p );
            (stptiMemGet_Frontend(FrontendHandle))->RAMStart_p     = NULL;
            (stptiMemGet_Frontend(FrontendHandle))->RAMRealStart_p = NULL;
            (stptiMemGet_Frontend(FrontendHandle))->AllocatedNumOfPkts = 0;
        }

        if ( NULL != (stptiMemGet_Frontend(FrontendHandle))->PIDTableRealStart_p)
        {
            STOS_MemoryDeallocate( Session_p->NonCachedPartition_p, (stptiMemGet_Frontend(FrontendHandle))->PIDTableRealStart_p  );
            (stptiMemGet_Frontend(FrontendHandle))->PIDTableStart_p     = NULL;
            (stptiMemGet_Frontend(FrontendHandle))->PIDTableRealStart_p = NULL;
        }
    }

    /* Remove from session lists */
    {
        FullHandle_t ListHandle;

        ListHandle = stptiMemGet_Session(FrontendHandle)->AllocatedList[FrontendHandle.member.ObjectType];
        Error = stpti_RemoveObjectFromList(ListHandle, FrontendHandle);

        if (Error != ST_NO_ERROR)
        {
            return Error;
        }

        stpti_ResizeObjectList(&stptiMemGet_Session(FrontendHandle)->AllocatedList[FrontendHandle.member.ObjectType]);
    }

    /* Deallocate FrontendHandle control block */
    stpti_MemDealloc(&(stptiMemGet_Session(FrontendHandle)->MemCtl), FrontendHandle.member.Object);

    return ST_NO_ERROR;
}
#endif /* #if defined (STPTI_FRONTEND_HYBRID) */

/******************************************************************************
Function Name : stpti_DeallocateBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_DeallocateBuffer(FullHandle_t BufferHandle, BOOL Force)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    assert(SEMAPHORE_P_COUNT(stpti_MemoryLock) == 0); /* Must be entered with Memory Locked */

#if defined( ST_OSLINUX )
    /* Remove from the proc file system */
    RemovePTIBufferFromProcFS(BufferHandle);
#endif
    if ((stptiMemGet_Buffer(BufferHandle))->SlotListHandle.word != STPTI_NullHandle()
        || (stptiMemGet_Buffer(BufferHandle))->SignalListHandle.word != STPTI_NullHandle())
    {
        if (Force)
        {
            /*
               Note: stpti_BufferClearSlots MUST be called AFTER stpti_BufferClearSignals,
                     as it clears a DMAIdent value that is used by stpti_BufferClearSignals
            */
            Error = stpti_BufferClearSignals(BufferHandle); /* Clear Signal Associations */

            if (Error == ST_NO_ERROR) Error = stpti_BufferClearSlots(BufferHandle); /* Clear Slot Associations */

            if (Error != ST_NO_ERROR)
                return Error;
        }
        else
            return STPTI_ERROR_INTERNAL_OBJECT_ASSOCIATED;
    }

    /* Free Actual buffer in noncached memory */

#if defined(ST_OSLINUX)
    {
        U32 Addr;
        Addr = (U32)(stptiMemGet_Buffer(BufferHandle))->RealStart_p;

        if( Addr ){
            iounmap( (stptiMemGet_Buffer(BufferHandle))->MappedStart_p );

            if(!stptiMemGet_Buffer(BufferHandle)->ManualAllocation)
#ifdef USING_BIG_PHYSAREA
                bigphysarea_free_pages( bus_to_virt(Addr) );
#else
                kfree( bus_to_virt(Addr) );
#endif /* #endifdef USING_BIG_PHYSAREA */
        }

    }
#else
    if(!stptiMemGet_Buffer(BufferHandle)->ManualAllocation)
        memory_deallocate( stptiMemGet_Session(BufferHandle)->NonCachedPartition_p,
                           (stptiMemGet_Buffer(BufferHandle))->RealStart_p);
#endif /* #endif defined(ST_OSLINUX) */

#ifdef STPTI_CAROUSEL_SUPPORT
    /* Disassociate from the carousel */

    Error = stpti_BufferClearCarousels(BufferHandle);
    if (Error != ST_NO_ERROR)
        return Error;
#endif

    /* Remove from session lists */
    {
        FullHandle_t ListHandle;

        ListHandle = stptiMemGet_Session(BufferHandle)->AllocatedList[BufferHandle.member.ObjectType];
        Error = stpti_RemoveObjectFromList(ListHandle, BufferHandle);

        if (Error != ST_NO_ERROR)
        {
            return Error;
        }

        stpti_ResizeObjectList(&stptiMemGet_Session(BufferHandle)->AllocatedList[BufferHandle.member.ObjectType]);
    }

    /* Deallocate Buffer control block */
    stpti_MemDealloc(&(stptiMemGet_Session(BufferHandle)->MemCtl), BufferHandle.member.Object);

    return ST_NO_ERROR;
}

/******************************************************************************
Function Name : stpti_BufferClearSlots
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_BufferClearSlots(FullHandle_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SlotListHandle;

    SlotListHandle = (stptiMemGet_Buffer(BufferHandle))->SlotListHandle;
    while (SlotListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(SlotListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t SlotHandle;

            SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[Indx];
            if (SlotHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateBuffer(SlotHandle, BufferHandle, TRUE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        SlotListHandle = (stptiMemGet_Buffer(BufferHandle))->SlotListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_DescramblerClearSlots
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_DescramblerClearSlots(FullHandle_t DescramblerHandle)
{
#ifdef STPTI_LOADER_SUPPORT
    return ST_ERROR_FEATURE_NOT_SUPPORTED;
#else
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SlotListHandle;

    SlotListHandle = (stptiMemGet_Descrambler(DescramblerHandle))->SlotListHandle;
    while (SlotListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(SlotListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t SlotHandle;

            SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[Indx];
            if (SlotHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateDescrambler(SlotHandle, DescramblerHandle, TRUE, FALSE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        SlotListHandle = (stptiMemGet_Descrambler(DescramblerHandle))->SlotListHandle;
    }
    return Error;
#endif
}

/******************************************************************************
Function Name : stpti_FilterClearSlots
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_FilterClearSlots(FullHandle_t FilterHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SlotListHandle;

    SlotListHandle = (stptiMemGet_Filter(FilterHandle))->SlotListHandle;
    while (SlotListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(SlotListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t SlotHandle;

            SlotHandle.word = (&stptiMemGet_List(SlotListHandle)->Handle)[Indx];
            if (SlotHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateFilter(SlotHandle, FilterHandle, TRUE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        SlotListHandle = (stptiMemGet_Filter(FilterHandle))->SlotListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_SignalClearBuffers
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_SignalClearBuffers(FullHandle_t SignalHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t BufferListHandle;

    BufferListHandle = (stptiMemGet_Signal(SignalHandle))->BufferListHandle;
    while (BufferListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(BufferListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t BufferHandle;

            BufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[Indx];
            if (BufferHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SignalDisassociateBuffer(SignalHandle, BufferHandle, TRUE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        BufferListHandle = (stptiMemGet_Signal(SignalHandle))->BufferListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_SlotClearDescramblers
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_SlotClearDescramblers(FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
#ifndef STPTI_LOADER_SUPPORT
    FullHandle_t DescramblerListHandle;

    DescramblerListHandle = (stptiMemGet_Slot(SlotHandle))->DescramblerListHandle;
    while (DescramblerListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(DescramblerListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t DescramblerHandle;

            DescramblerHandle.word = (&stptiMemGet_List(DescramblerListHandle)->Handle)[Indx];
            if (DescramblerHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateDescrambler(SlotHandle, DescramblerHandle, TRUE, FALSE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        DescramblerListHandle = (stptiMemGet_Slot(SlotHandle))->DescramblerListHandle;
    }
#endif
    return Error;
}


/******************************************************************************
Function Name : stpti_SlotClearFilters
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_SlotClearFilters(FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FilterListHandle;

    FilterListHandle = (stptiMemGet_Slot(SlotHandle))->FilterListHandle;
    while (FilterListHandle.word != STPTI_NullHandle())
    {
        /* get next filter handle */
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(FilterListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t FilterHandle;

            FilterHandle.word = (&stptiMemGet_List(FilterListHandle)->Handle)[Indx];
            if (FilterHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateFilter(SlotHandle, FilterHandle, TRUE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        FilterListHandle = (stptiMemGet_Slot(SlotHandle))->FilterListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_SlotClearBuffers
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_SlotClearBuffers(FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t BufferListHandle;

    BufferListHandle = (stptiMemGet_Slot(SlotHandle))->BufferListHandle;
    while (BufferListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(BufferListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t BufferHandle;

            BufferHandle.word = (&stptiMemGet_List(BufferListHandle)->Handle)[Indx];
            if (BufferHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateBuffer(SlotHandle, BufferHandle, TRUE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        BufferListHandle = (stptiMemGet_Slot(SlotHandle))->BufferListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_SlotClearIndex
  Description :
   Parameters :
******************************************************************************/
#ifndef STPTI_NO_INDEX_SUPPORT
static ST_ErrorCode_t stpti_SlotClearIndex(FullHandle_t SlotHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t IndexListHandle;

    IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;
    while (IndexListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(IndexListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t IndexHandle;

            IndexHandle.word = (&stptiMemGet_List(IndexListHandle)->Handle)[Indx];
            if (IndexHandle.word != STPTI_NullHandle())
            {
                Error = stpti_SlotDisassociateIndex(SlotHandle, IndexHandle, TRUE);;
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        IndexListHandle = (stptiMemGet_Slot(SlotHandle))->IndexListHandle;
    }

    return Error;
}
#endif


/******************************************************************************
Function Name : stpti_BufferClearSignals
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_BufferClearSignals(FullHandle_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t SignalListHandle;

    SignalListHandle = (stptiMemGet_Buffer(BufferHandle))->SignalListHandle;
    if (SignalListHandle.word != STPTI_NullHandle())
    {
        while (SignalListHandle.word != STPTI_NullHandle())
        {
            U16 Indx;
            U16 MaxIndx = stptiMemGet_List(SignalListHandle)->MaxHandles;

            for (Indx = 0; Indx < MaxIndx; ++Indx)
            {
                FullHandle_t SignalHandle;

                SignalHandle.word = (&stptiMemGet_List(SignalListHandle)->Handle)[Indx];
                if (SignalHandle.word != STPTI_NullHandle())
                {
                    Error = stpti_SignalDisassociateBuffer(SignalHandle, BufferHandle, TRUE);
                    if(Error == ST_NO_ERROR)
                    {
                        Error=stptiHAL_FlushSignalsForBuffer(SignalHandle, BufferHandle);
                    }
                    break;
                }
            }
            if (Indx == MaxIndx)
            {
                Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
            }
            if (Error != ST_NO_ERROR)
            {
                break;
            }

            SignalListHandle = (stptiMemGet_Buffer(BufferHandle))->SignalListHandle;
        }
    }
    else
        Error = ST_NO_ERROR;

    return Error;
}

/******************************************************************************
Function Name : stpti_SignalDisassociateBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SignalDisassociateBuffer(FullHandle_t SignalHandle, FullHandle_t BufferHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t BufferListHandle = (stptiMemGet_Signal(SignalHandle))->BufferListHandle;
    FullHandle_t SignalListHandle = (stptiMemGet_Buffer(BufferHandle))->SignalListHandle;

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (BufferListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_SIGNAL_HANDLE;
        else if (SignalListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_BUFFER_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(SignalListHandle, SignalHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(BufferListHandle, BufferHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Disable Signals on the Buffer */
        stptiHAL_SignalUnsetup(BufferHandle);

        /* Object specific disassociations here */
        (stptiMemGet_Buffer(BufferHandle))->Queue_p = NULL;

        Error=stptiHAL_FlushSignalsForBuffer(SignalHandle, BufferHandle);

        /* Disassociate and resize */
        Error = stpti_RemoveObjectFromList(BufferListHandle, BufferHandle);
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(SignalListHandle, SignalHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_Buffer(BufferHandle))->SignalListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Signal(SignalHandle))->BufferListHandle);
            }
        }
    }

    return Error;
}

#if defined (STPTI_FRONTEND_HYBRID)
/******************************************************************************
Function Name : stpti_FrontendDisassociatePTI
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_FrontendDisassociatePTI(FullHandle_t FullFrontendHandle, FullHandle_t FullSessionHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t     SessionListHandle = (stptiMemGet_Frontend(FullFrontendHandle))->SessionListHandle;
    FullHandle_t     FrontendListHandle = (stptiMemGet_Device(FullSessionHandle))->FrontendListHandle;
    STPTI_StreamID_t StreamID           = (stptiMemGet_Device(FullSessionHandle))->StreamID;

    STTBX_Print(("Calling stpti_FrontendDisassociatePTI.....\n"));

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (FrontendListHandle.word == STPTI_NullHandle())
            Error = ST_ERROR_INVALID_HANDLE;
        else if (SessionListHandle.word == STPTI_NullHandle())
            Error = ST_ERROR_INVALID_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(SessionListHandle, FullSessionHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(FrontendListHandle, FullFrontendHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */
        stptiHAL_FrontendUnlink( FullFrontendHandle, StreamID );

        /* Disassociate and resize */
        Error = stpti_RemoveObjectFromList(FrontendListHandle, FullFrontendHandle);

        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(SessionListHandle, FullSessionHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_Device(FullSessionHandle))->FrontendListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Frontend(FullFrontendHandle))->SessionListHandle);
            }
        }
    }

    return Error;
}
#endif /* #if defined (STPTI_FRONTEND_HYBRID) */

/******************************************************************************
Function Name : stpti_SlotDisassociateBuffer
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SlotDisassociateBuffer(FullHandle_t SlotHandle, FullHandle_t BufferHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t BufferListHandle = (stptiMemGet_Slot(SlotHandle))->BufferListHandle;
    FullHandle_t SlotListHandle = (stptiMemGet_Buffer(BufferHandle))->SlotListHandle;

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (BufferListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_SLOT_HANDLE;
        else if (SlotListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_BUFFER_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(SlotListHandle, SlotHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(BufferListHandle, BufferHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */

        /* ----- UNLINK DMA ------- */
        if(stptiMemGet_Buffer(BufferHandle)->IsRecordBuffer == FALSE)
            Error = stptiHAL_SlotUnLinkGeneral(SlotHandle, BufferHandle);
        else
            Error = stptiHAL_SlotUnLinkRecord(SlotHandle, BufferHandle);

        /* Disassociate and resize */

        Error = stpti_RemoveObjectFromList(BufferListHandle, BufferHandle);

        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(SlotListHandle, SlotHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_Buffer(BufferHandle))->SlotListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Slot(SlotHandle))->BufferListHandle);
            }
        }
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_SlotDisassociateDescrambler
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SlotDisassociateDescrambler(FullHandle_t SlotHandle, FullHandle_t DescramblerHandle, BOOL PreChecked, BOOL FastDisasscoiate)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t DescramblerListHandle = (stptiMemGet_Slot(SlotHandle))->DescramblerListHandle;
    FullHandle_t SlotListHandle = (stptiMemGet_Descrambler(DescramblerHandle))->SlotListHandle;

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (DescramblerListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_DESCRAMBLER_NOT_ASSOCIATED;
        else if (SlotListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_DESCRAMBLER_NOT_ASSOCIATED;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(SlotListHandle, SlotHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(DescramblerListHandle, DescramblerHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */

        if (!FastDisasscoiate)
            Error = stptiHAL_DescramblerDisassociate(DescramblerHandle, SlotHandle);

        /* Disassociate and resize */

        Error = stpti_RemoveObjectFromList(DescramblerListHandle, DescramblerHandle);
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(SlotListHandle, SlotHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_Descrambler(DescramblerHandle))->SlotListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Slot(SlotHandle))->DescramblerListHandle);
            }
        }
    }

    return Error;
}

/******************************************************************************
Function Name : stpti_PidDisassociateDescrambler
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_PidDisassociateDescrambler(FullHandle_t DescramblerHandle, BOOL FastDisasscoiate)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STPTI_Slot_t Slot;
    STPTI_Pid_t Pid = (stptiMemGet_Descrambler(DescramblerHandle))->AssociatedPid;

    /* Scan for a Slot using this pid and dissassociate the descrambler with it */

    Error = stpti_SlotQueryPid(DescramblerHandle, Pid, &Slot);

    while ((Error == ST_NO_ERROR) && (Slot != STPTI_NullHandle()))
    {
        FullHandle_t SlotHandle;

        SlotHandle.word = Slot;
        Error = stpti_SlotDisassociateDescrambler(SlotHandle, DescramblerHandle, TRUE, FastDisasscoiate);

        if (Error == ST_NO_ERROR)
        {
            Error = stpti_NextSlotQueryPid(DescramblerHandle, Pid, &Slot);
        }
    }

    if (Error == ST_NO_ERROR)
    {
        (stptiMemGet_Descrambler(DescramblerHandle))->AssociatedPid = TC_INVALID_PID;
    }

    return Error;
}
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

/******************************************************************************
Function Name : stpti_SlotDisassociateFilter
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_SlotDisassociateFilter(FullHandle_t SlotHandle, FullHandle_t FilterHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if !defined ( STPTI_BSL_SUPPORT )
    FullHandle_t FilterListHandle = (stptiMemGet_Slot(SlotHandle))->FilterListHandle;
    FullHandle_t SlotListHandle = (stptiMemGet_Filter(FilterHandle))->SlotListHandle;
#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (FilterListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_SLOT_HANDLE;
        else if (SlotListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_FILTER_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(SlotListHandle, SlotHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(FilterListHandle, FilterHandle.word, &Instance);
            }
        }
    }
#endif
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */

            switch ((stptiMemGet_Filter(FilterHandle))->Type)
            {
#if !defined ( STPTI_BSL_SUPPORT )
            case STPTI_FILTER_TYPE_TSHEADER_FILTER:
                Error = stptiHAL_FilterDisassociateTransport(SlotHandle, FilterHandle);
                break;

            case STPTI_FILTER_TYPE_PES_FILTER:
                Error = stptiHAL_FilterDisassociatePES(SlotHandle, FilterHandle);
                break;

            case STPTI_FILTER_TYPE_PES_STREAMID_FILTER:
	            Error = stptiHAL_FilterDisassociatePESStreamId( SlotHandle, FilterHandle );
	            break;

            case STPTI_FILTER_TYPE_ECM_FILTER:
                Error = stptiHAL_FilterDisassociateSection(SlotHandle, FilterHandle);           /* ECM filters are section filters */
                break;

            case STPTI_FILTER_TYPE_EMM_FILTER:
                Error = stptiHAL_FilterDisassociateSection(SlotHandle, FilterHandle);           /* EMM filters are section filters */
                break;
#ifdef STPTI_DC2_SUPPORT
            case STPTI_FILTER_TYPE_DC2_MULTICAST16_FILTER:
            case STPTI_FILTER_TYPE_DC2_MULTICAST32_FILTER:
            case STPTI_FILTER_TYPE_DC2_MULTICAST48_FILTER:
                Error = stptiHAL_DC2FilterDisassociateMulticast(SlotHandle, FilterHandle);
                break;
#endif
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */

            default:
                Error = stptiHAL_FilterDisassociateSection(SlotHandle, FilterHandle);
                break;
            }

        /* Disassociate and resize */
#if !defined ( STPTI_BSL_SUPPORT )
        Error = stpti_RemoveObjectFromList(FilterListHandle, FilterHandle);
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(SlotListHandle, SlotHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_Filter(FilterHandle))->SlotListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Slot(SlotHandle))->FilterListHandle);
            }
        }
#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */
    }

    return Error;
}

#if !defined ( STPTI_BSL_SUPPORT )

#ifdef STPTI_CAROUSEL_SUPPORT


/******************************************************************************
Function Name : stpti_BufferClearCarousels
  Description :
   Parameters :
******************************************************************************/
static ST_ErrorCode_t stpti_BufferClearCarousels(FullHandle_t BufferHandle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t CarouselListHandle;

    CarouselListHandle = (stptiMemGet_Buffer(BufferHandle))->CarouselListHandle;
    while (CarouselListHandle.word != STPTI_NullHandle())
    {
        U16 Indx;
        U16 MaxIndx = stptiMemGet_List(CarouselListHandle)->MaxHandles;

        for (Indx = 0; Indx < MaxIndx; ++Indx)
        {
            FullHandle_t CarouselHandle;

            CarouselHandle.word = (&stptiMemGet_List(CarouselListHandle)->Handle)[Indx];
            if (CarouselHandle.word != STPTI_NullHandle())
            {
                Error = stpti_BufferDisassociateCarousel(BufferHandle, CarouselHandle, TRUE);
                break;
            }
        }
        if (Indx == MaxIndx)
        {
            Error = STPTI_ERROR_INTERNAL_ALL_MESSED_UP;
        }
        if (Error != ST_NO_ERROR)
        {
            break;
        }
        CarouselListHandle = (stptiMemGet_Buffer(BufferHandle))->CarouselListHandle;
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_BufferDisassociateCarousel
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_BufferDisassociateCarousel(FullHandle_t BufferHandle, FullHandle_t CarouselHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t BufferListHandle   = (stptiMemGet_Carousel(CarouselHandle))->BufferListHandle;
    FullHandle_t CarouselListHandle = (stptiMemGet_Buffer(BufferHandle))->CarouselListHandle;
    /* for GNBvd26004*/
    FullHandle_t SlotListHandle     = (stptiMemGet_Buffer(BufferHandle))->SlotListHandle;
    TCPrivateData_t *PrivateData_p  = stptiMemGet_PrivateData(CarouselHandle);

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (BufferListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_BUFFER_NOT_LINKED;
        else if (CarouselListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_BUFFER_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(CarouselListHandle, CarouselHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(BufferListHandle, BufferHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */

        /* Disassociate and resize */

        Error = stpti_RemoveObjectFromList(BufferListHandle, BufferHandle);

        if (Error == ST_NO_ERROR)
        {
            /* GNBvd26004, STPTI_CarouselUnlinkBuffer() will return DMA_UNAVAILABLE after "MaxDMA"  */
            /* ----- UNLINK DMA ------- */

            if (BufferListHandle.word != STPTI_NullHandle())
            {
                U32 UsedHandles = stptiMemGet_List(CarouselListHandle)->UsedHandles;

                if ((UsedHandles == 1) && (SlotListHandle.word == STPTI_NullHandle()))
                {
                    U32 TC_DMAIdent;

                    /* No Slot is associated with this buffer, dissassociate DMA from the buffer */

                    TC_DMAIdent = stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent;
                    /*if TC_DMAIdent=UNININITIALISED_VALUE. Then DMA has already
                     been freed. GNBvd26004*/
                    if(TC_DMAIdent != UNININITIALISED_VALUE)
                    {
                        PrivateData_p->BufferHandles_p[TC_DMAIdent] = STPTI_NullHandle();
                        stptiMemGet_Buffer(BufferHandle)->TC_DMAIdent = UNININITIALISED_VALUE;
                    }

                }
            }

            Error = stpti_RemoveObjectFromList(CarouselListHandle, CarouselHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_Buffer(BufferHandle))->CarouselListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Carousel(CarouselHandle))->BufferListHandle);
            }
        }
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_CarouselDisassociateSimpleEntry
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CarouselDisassociateSimpleEntry(FullHandle_t CarouselHandle, FullHandle_t EntryHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t EntryListHandle = (stptiMemGet_Carousel(CarouselHandle))->CarouselEntryListHandle;
    FullHandle_t CarouselListHandle = (stptiMemGet_CarouselSimpleEntry(EntryHandle))->CarouselListHandle;

#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
    STTBX_Print(("stpti_CarouselDisassociateEntry(CarouselHandle:0x%08x, EntryHandle:0x%08x, PreChecked:%s\n",
                 CarouselHandle.word, EntryHandle.word, (PreChecked) ? "TRUE" : "FALSE"));
#endif

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (EntryListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_CAROUSEL_HANDLE;
        else if (CarouselListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_CAROUSEL_ENTRY_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(CarouselListHandle, CarouselHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(EntryListHandle, EntryHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */

        /* if Entry is linked into Carousel then it must be unlinked */

        if ((stptiMemGet_CarouselSimpleEntry(EntryHandle))->Linked)
        {
            stpti_CarouselUnlinkSimpleEntry(CarouselHandle, EntryHandle);
        }

        /* Disassociate and resize */

        Error = stpti_RemoveObjectFromList(EntryListHandle, EntryHandle);
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(CarouselListHandle, CarouselHandle);
            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_CarouselSimpleEntry(EntryHandle))->CarouselListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Carousel(CarouselHandle))->CarouselEntryListHandle);
            }
        }
    }

    return Error;
}


/******************************************************************************
Function Name : stpti_CarouselDisassociateTimedEntry
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stpti_CarouselDisassociateTimedEntry(FullHandle_t CarouselHandle, FullHandle_t EntryHandle, BOOL PreChecked)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    FullHandle_t EntryListHandle = (stptiMemGet_Carousel(CarouselHandle))->CarouselEntryListHandle;
    FullHandle_t CarouselListHandle = (stptiMemGet_CarouselTimedEntry(EntryHandle))->CarouselListHandle;

#ifndef STPTI_NO_USAGE_CHECK
    if (!PreChecked)
    {
        if (EntryListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_CAROUSEL_HANDLE;
        else if (CarouselListHandle.word == STPTI_NullHandle())
            Error = STPTI_ERROR_INVALID_CAROUSEL_ENTRY_HANDLE;
        else
        {
            U32 Instance;

            /* Check Cross associations */

            Error = stpti_GetInstanceOfObjectInList(CarouselListHandle, CarouselHandle.word, &Instance);
            if (Error == ST_NO_ERROR)
            {
                Error = stpti_GetInstanceOfObjectInList(EntryListHandle, EntryHandle.word, &Instance);
            }
        }
    }
#endif

    if (Error == ST_NO_ERROR)
    {
        /* Object specific disassociations here */

        /* Entry is linked into Carousel then it must be unlinked */

        if ((stptiMemGet_CarouselTimedEntry(EntryHandle))->Linked)
        {
            stpti_CarouselUnlinkTimedEntry(CarouselHandle, EntryHandle);
        }

        /* Disassociate and resize */

        Error = stpti_RemoveObjectFromList(EntryListHandle, EntryHandle);
        if (Error == ST_NO_ERROR)
        {
            Error = stpti_RemoveObjectFromList(CarouselListHandle, CarouselHandle);

            if (Error == ST_NO_ERROR)
            {
                stpti_ResizeObjectList(&(stptiMemGet_CarouselTimedEntry(EntryHandle))->CarouselListHandle);
                stpti_ResizeObjectList(&(stptiMemGet_Carousel(CarouselHandle))->CarouselEntryListHandle);
            }
        }
    }

    return Error;
}


#endif  /* STPTI_CAROUSEL_SUPPORT */


#endif /* #if !defined ( STPTI_BSL_SUPPORT ) */ 


/* --- EOF --- */
