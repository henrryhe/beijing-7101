/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: gpdma.c
 Description: 

******************************************************************************/

#include <string.h>

#include "stddefs.h"
#include "stdevice.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#ifndef STPTI_GPDMA_SUPPORT
 #error Incorrect build options!
#endif


#include "memget.h"


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHAL_BufferSetGPDMAParams(FullHandle_t FullBufferHandle, STPTI_GPDMAParams_t *GPDMAParams)
{
    Buffer_t *Buffer_p = stptiMemGet_Buffer( FullBufferHandle );

    Buffer_p->STGPDMAHandle      = GPDMAParams->Handle;
    Buffer_p->STGPDMATimingModel = GPDMAParams->TimingModel;
    Buffer_p->STGPDMAEventLine   = GPDMAParams->EventLine;
    Buffer_p->STGPDMATimeOut     = GPDMAParams->TimeOut;

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHAL_BufferClearGPDMAParams(FullHandle_t FullBufferHandle)
{
    STPTI_GPDMAParams_t GPDMAParams;

    GPDMAParams.Handle      = NULL;
    GPDMAParams.TimingModel = 0;
    GPDMAParams.EventLine   = 0;
    GPDMAParams.TimeOut     = 0;

    return( stptiHAL_BufferSetGPDMAParams( FullBufferHandle, &GPDMAParams) );
}


/* ------------------------------------------------------------------------- */


/* EOF */
