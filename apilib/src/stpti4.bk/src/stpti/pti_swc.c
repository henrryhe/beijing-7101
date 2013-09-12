/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_swc.c
 Description: SWCDFIFO functions


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
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #endif !defined(ST_OSLINUX) */

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"
#include "validate.h"

#include "memget.h"


/******************************************************************************
Function Name : STPTI_BufferGetWritePointer
  Description : Get the Write pointer in  a buffer
   Parameters : the relevant buffer handle and a reference to update with the
                write pointer
******************************************************************************/
ST_ErrorCode_t STPTI_BufferGetWritePointer(STPTI_Buffer_t BufferHandle, void **Write_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;
    
    
    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;
    Error = stptiValidate_BufferGetWritePointer(FullBufferHandle);
    if(Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferGetWritePointer(FullBufferHandle, Write_p);
    
    }    
    
    STOS_SemaphoreSignal(stpti_MemoryLock);

    return Error;
}

/******************************************************************************
Function Name : STPTI_BufferSetOverflowControl
  Description : Enable/Disable overflow checking on a specific buffer
   Parameters : The buffer in question, and the control for its overflow
******************************************************************************/
ST_ErrorCode_t STPTI_BufferSetOverflowControl(STPTI_Buffer_t BufferHandle, BOOL IgnoreOverflow)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;

    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;
    Error = stptiValidate_BufferSetOverflowControl(FullBufferHandle);
    if(Error == ST_NO_ERROR)
    {
        Error = stptiHAL_BufferSetOverflowControl(FullBufferHandle, IgnoreOverflow);
        
        if(Error == ST_NO_ERROR)
        {
           stptiMemGet_Buffer(FullBufferHandle)->IgnoreOverflow = IgnoreOverflow;
        }
    }    
    
    STOS_SemaphoreSignal(stpti_MemoryLock);

    return Error;

}

/******************************************************************************
Function Name : STPTI_BufferSetReadPointer
  Description : Update the read pointer on a buffer
   Parameters : The relevant buffer handle and the new read pointer
******************************************************************************/
ST_ErrorCode_t STPTI_BufferSetReadPointer(STPTI_Buffer_t BufferHandle, void* Read_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;
    
    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;
    Error = stptiValidate_BufferSetReadPointer(FullBufferHandle);
    if(Error == ST_NO_ERROR)
    {
        /*Only need to do this if were accepting overflows*/
        if(stptiMemGet_Buffer(FullBufferHandle)->IgnoreOverflow == FALSE)
            Error = stptiHAL_BufferSetReadPointer(FullBufferHandle, Read_p);
    }    
    STOS_SemaphoreSignal(stpti_MemoryLock);

    return Error;

}

/******************************************************************************
Function Name : BufferLevelChangeEvent
  Description : Enable the buffer level change event.

  When swcd fifo read ponter is updated then event is sent indicating this..

   Parameters : 
******************************************************************************/
ST_ErrorCode_t STPTI_BufferLevelChangeEvent(STPTI_Handle_t BufferHandle, BOOL Enable)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    FullHandle_t FullBufferHandle;
    
    STOS_SemaphoreWait(stpti_MemoryLock);
    FullBufferHandle.word = BufferHandle;
 
    Error = stptiValidate_BufferLevelChangeEvent(FullBufferHandle, Enable);
    if(Error == ST_NO_ERROR)
    {
        /* When true generate a STPTI_EVENT_BUFFER_LEVEL_CHANGE_EVT.*/
        stptiMemGet_Buffer(FullBufferHandle)->EventLevelChange = Enable;
    }

    STOS_SemaphoreSignal(stpti_MemoryLock);

    return Error;

}

