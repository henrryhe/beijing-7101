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
#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif /* #if !defined(ST_OSLINUX) */
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
           Status: NEW FUNCTIONALITY
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


    Write_p    =  TRANSLATE_LOG_ADD(DMAConfig_p->DMAQWrite_p, Buffer_p->Start_p);
    Top_p      =  TRANSLATE_LOG_ADD((DMAConfig_p->DMATop_p & ~0xf) + 16, Buffer_p->Start_p);
    Base_p     =  TRANSLATE_LOG_ADD(DMAConfig_p->DMABase_p, Buffer_p->Start_p);
    BufferSize = Top_p - Base_p;
    
    STSYS_WriteRegDev32LE((void*)&DMAConfig_p->DMARead_p, (U32) TRANSLATE_PHYS_ADD(Read_p) );
    
    
    STSYS_WriteTCReg16LE((void*)&DMAConfig_p->Threshold, (STSYS_ReadRegDev16LE((void*)&DMAConfig_p->Threshold))+((Buffer_p->MultiPacketSize) & 0xffff));

    
    /* Signal or Re-enable interrupts as appropriate */

    /* Is there enough data in the buffer to go again ? */
    Write_p = TRANSLATE_LOG_ADD(DMAConfig_p->DMAQWrite_p, Buffer_p->Start_p);

    BytesInBuffer = (Write_p < (U32)Read_p)?(BufferSize + Write_p - (U32)Read_p):(Write_p - (U32)Read_p);
    if (BytesInBuffer >= (((Buffer_p->MultiPacketSize) & 0xffff) * 188))
    {
        stptiHelper_SignalBuffer(FullBufferHandle, 0XFFFFFFFF);
    }
    else
    {
        STSYS_ClearTCMask16LE((void*)&DMAConfig_p->SignalModeFlags, TC_DMA_CONFIG_SIGNAL_MODE_FLAGS_SIGNAL_DISABLE);
    }

    if( Buffer_p->EventLevelChange == TRUE ){
        EventData.u.BufferLevelData.BufferHandle = FullBufferHandle.word;
        stptiHAL_BufferGetFreeLength(FullBufferHandle, &EventData.u.BufferLevelData.FreeLength);
        STEVT_Notify(stptiMemGet_Device(FullBufferHandle)->EventHandle,
                     stptiMemGet_Device(FullBufferHandle)->EventID[STPTI_EVENT_BUFFER_LEVEL_CHANGE_EVT - STPTI_EVENT_BASE],
                     &EventData);
       
    }
    
    return Error;
}


/* ------------------------- 
           Status: NEW FUNCTIONALITY
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
    STPTI_TCParameters_t *TC_Params_p   = (STPTI_TCParameters_t *) &PrivateData_p->TC_Params;
    
    
    DMAIdent = stptiMemGet_Buffer(FullBufferHandle)->TC_DMAIdent;
    DMAConfig_p = TcHal_GetTCDMAConfig( TC_Params_p, DMAIdent);

    *Write_p = (void*) NONCACHED_TRANSLATION( STSYS_ReadRegDev32LE((void*)&DMAConfig_p->DMAQWrite_p) ); 
    
    return Error;

}

/* ------------------------- 
           Status: NEW FUNCTIONALITY
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
