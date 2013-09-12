/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pcp.c
 Description: 
 
******************************************************************************/

#ifndef STPTI_PCP_SUPPORT
 #error Incorrect build options!
#else

#include <string.h>

#include "stddefs.h"
#include "stdevice.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"



#include "memget.h"

/* ------------------------------------------------------------------------- */

/*
 * Function Name : stptiHAL_BufferSetPCPParams
 * Description   : Sets the auxiliary parameters used by helper.c stptiHelper_CircToCircCopy
 */
ST_ErrorCode_t stptiHAL_BufferSetPCPParams(FullHandle_t FullBufferHandle, STPTI_PCPParams_t *PCPParams)
{
    Buffer_t *Buffer_p = stptiMemGet_Buffer( FullBufferHandle );

    Buffer_p->STPCPHandle = PCPParams->Handle;
    Buffer_p->STPCPEngineModeEncrypt = PCPParams->EngineModeEncrypt;

    return( ST_NO_ERROR );
}


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHAL_BufferClearPCPParams(FullHandle_t FullBufferHandle)
{
    STPTI_PCPParams_t PCPParams;

    PCPParams.Handle      = NULL;
    PCPParams.EngineModeEncrypt = 0;

    return( stptiHAL_BufferSetPCPParams( FullBufferHandle, &PCPParams) );
}


/* ------------------------------------------------------------------------- */

#endif
/* EOF */
