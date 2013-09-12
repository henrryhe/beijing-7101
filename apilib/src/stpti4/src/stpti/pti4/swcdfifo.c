/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: swcdfifo.c
 Description: Provides interface to PTI side of SWCDFIFO

******************************************************************************/

/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
#define STTBX_PRINT
#endif

#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #if !defined(ST_OSLINUX) */

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "stpti.h"

#include "pti_evt.h"
#include "pti_hndl.h"
#include "pti_loc.h"
#include "pti_hal.h"

#include "memget.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"


/* Functions --------------------------------------------------------------- */

/* ------------------------- 
    Referenced by: STPTI_BufferSetReadPointer 
              HAL: 4
      description: FDMA helper function
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferSetReadPointer(FullHandle_t FullBufferHandle, void *Read_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    
    U32 BufferSize;
    U32 BytesInBuffer;
    U32 DMAIdent;
    TCDMAConfig_t *DMAConfig_p;
    U32 Write_p;
    U32 Top_p;  
    U32 Base_p;
    STPTI_EventData_t EventData;
    Buffer_t *Buffer_p;
    
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;

    Buffer_p = stptiMemGet_Buffer(FullBufferHandle);    
    
    DMAIdent = Buffer_p->TC_DMAIdent;
    DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent);

    Top_p      =  (DMAConfig_p->DMATop_p & ~0xf) + 16;
    Base_p     =  DMAConfig_p->DMABase_p;
    BufferSize = Top_p - Base_p;
    
#if defined(STPTI_LEGACY_LINUX_SWCDFIFO)
    /* Historically under linux we work with physical addresses */
    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, (U32) Read_p );
    STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, (STSYS_ReadRegDev16LE((void*)&DMAConfig_p->Threshold))+(Buffer_p->MultiPacketSize&0xffff));
    Write_p = DMAConfig_p->DMAQWrite_p;
#else
    /* Under all other OSes (other than linux) we work only with virtual addresses */
    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, ( (U32)Read_p - (U32)Buffer_p->MappedStart_p ) + (U32)Base_p);
    STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, (STSYS_ReadRegDev16LE((void*)&DMAConfig_p->Threshold))+(Buffer_p->MultiPacketSize&0xffff));
    Write_p = DMAConfig_p->DMAQWrite_p - (U32)Base_p + (U32)Buffer_p->MappedStart_p;
#endif
    
    /* Signal or Re-enable interrupts as appropriate */

    /* Is there enough data in the buffer to go again ? */
    /* Check a signal is still associated */
    if (Buffer_p->SignalListHandle.word != STPTI_NullHandle())
    {
        BytesInBuffer = (Write_p < (U32)Read_p)?(BufferSize + Write_p - (U32)Read_p):(Write_p - (U32)Read_p);
        if (BytesInBuffer >= ((Buffer_p->MultiPacketSize&0xffff) * 188))
        {
            stptiHelper_SignalBuffer(FullBufferHandle, 0XFFFFFFFF);
        }
        else
        {
            STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
        }
    }
    else
    {
      /* if no signal linked to this buffer, do not wait for signal !! */
      STSYS_SetTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
    }
    
    if( Buffer_p->EventLevelChange == TRUE ){
        EventData.u.BufferLevelData.BufferHandle = FullBufferHandle.word;
        stptiHAL_BufferGetFreeLength(FullBufferHandle, &EventData.u.BufferLevelData.FreeLength);
#if defined( ST_OSLINUX )
        /* We use STEVT_NotifyWithSize() here for linux to avoid a compile warning */
        STEVT_NotifyWithSize(stptiMemGet_Device(FullBufferHandle)->EventHandle,
                             stptiMemGet_Device(FullBufferHandle)->EventID[STPTI_EVENT_BUFFER_LEVEL_CHANGE_EVT - STPTI_EVENT_BASE],
                             &EventData,
                             sizeof(EventData));
#else
        STEVT_Notify(stptiMemGet_Device(FullBufferHandle)->EventHandle,
                     stptiMemGet_Device(FullBufferHandle)->EventID[STPTI_EVENT_BUFFER_LEVEL_CHANGE_EVT - STPTI_EVENT_BASE],
                     &EventData);
#endif       
    }
    
    return Error;
}


/* ------------------------- 
    Referenced by: STPTI_BufferGetWritePointer  
              HAL: 4
      description: FDMA helper function
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferGetWritePointer(FullHandle_t FullBufferHandle, void **Write_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 DMAIdent;
    TCDMAConfig_t *DMAConfig_p;
    
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
    FullHandle_t          SlotListHandle = stptiMemGet_Buffer(FullBufferHandle)->SlotListHandle;
    FullHandle_t          FullSlotHandle;
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
#if !defined(STPTI_LEGACY_LINUX_SWCDFIFO)
    Buffer_t             *Buffer_p      = stptiMemGet_Buffer(FullBufferHandle);    
#endif
    
    DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent);

    FullSlotHandle.word = ( &stptiMemGet_List( SlotListHandle )->Handle )[0];
    if(FullSlotHandle.word != STPTI_NullHandle())
    {        
#if !defined( STPTI_SWCDFIFO_QUANTISED_PES )
        if((stptiMemGet_Buffer(FullBufferHandle)->IsRecordBuffer == TRUE) || 
                (stptiMemGet_Slot(FullSlotHandle)->Flags.SoftwareCDFifo == FALSE))
#endif      /* !defined( STPTI_SWCDFIFO_QUANTISED_PES ) */
        {
            /* Return Quantised write pointers for record buffers and all non-CDFIFO modes */
#if defined(STPTI_LEGACY_LINUX_SWCDFIFO)
            /* Historically under linux we work with physical addresses here */
            *Write_p = (void*)(STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAQWrite_p)); 
#else
            /* Under all other OSes (other than linux) we work only with virtual addresses */
            *Write_p = (void*)(STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAQWrite_p) - (U32)Buffer_p->Start_p + (U32)Buffer_p->MappedStart_p); 
#endif    
        }
#if !defined( STPTI_SWCDFIFO_QUANTISED_PES )
        else
        {
            /* Return Raw Write pointers for CDFIFO mode (Pointer Windback on error is disabled in TC) */
#if defined(STPTI_LEGACY_LINUX_SWCDFIFO)
            /* Historically under linux we work with physical addresses here */
            *Write_p = (void*)(STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAWrite_p)); 
#else
            /* Under all other OSes (other than linux) we work only with virtual addresses */
            *Write_p = (void*)(STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAWrite_p) - (U32)Buffer_p->Start_p + (U32)Buffer_p->MappedStart_p); 
#endif    
        }
#endif      /* !defined( STPTI_SWCDFIFO_QUANTISED_PES ) */
    }
    else
    {
        Error = STPTI_ERROR_SLOT_NOT_ASSOCIATED;
    }

    return Error;

}

/* ------------------------- 
    Referenced by: STPTI_BufferSetOverflowControl
              HAL: 4
      description: This function is used to assess the free length remaining in a PTI buffer (the write pointer 
                   is used - not the Qwrite pointer) This function is useful for flow control in pull mode.
   previous alias: ...
   ------------------------- */
ST_ErrorCode_t stptiHAL_BufferSetOverflowControl(FullHandle_t FullBufferHandle, BOOL OverflowControl)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U32 DMAIdent;
    
    TCDMAConfig_t *DMAConfig_p;
    
    TCPrivateData_t      *PrivateData_p = stptiMemGet_PrivateData(FullBufferHandle);
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    
    
    DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent);

    if(OverflowControl) /*If true, we're ignoring overflows*/
    {
        STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p,STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMATop_p) + 256); /*Set read pointer to outside buffer, hence overflow should never occur*/
    }        
    else
    {
        STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p,STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAWrite_p)); /*Set buffer to empty*/ 
    }  
          
    return Error;

}

