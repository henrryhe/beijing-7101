/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: halvalid.c
 Description: validate pti4 HAL specifics

******************************************************************************/

/* Includes ---------------------------------------------------------------- */
#if !defined(ST_OSLINUX)
#include <assert.h>
#include <string.h>
#include <stdio.h>
#endif /* #enfif !defined(ST_OSLINUX) */

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


/* ------------------------------------------------------------------------- */


#ifdef STPTI_NO_PARAMETER_CHECK /* Stub out functions and return a 'pass' (ST_NO_ERROR) for the validation */


#define STPTI_VALIDATE_HAL_NO_ERROR {return(ST_NO_ERROR);}


ST_ErrorCode_t stpti_HAL_Validate_FilterOneShotSupported( FullHandle_t FullFilterHandle )
 STPTI_VALIDATE_HAL_NO_ERROR

ST_ErrorCode_t stpti_HAL_Validate_FilterNotMatchModeSetup( FullHandle_t FullFilterHandle )
 STPTI_VALIDATE_HAL_NO_ERROR


/* ------------------------------------------------------------------------- */


#else   /* STPTI_NO_PARAMETER_CHECK. Perform real tests of the parameters */

/* One shot filtering *NOT* supported for this HAL */

ST_ErrorCode_t stpti_HAL_Validate_FilterOneShotSupported( FullHandle_t FullFilterHandle )
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );  
}


/* ------------------------------------------------------------------------- */

/*  if ((stptiMemGet_Device(FullFilterHandle)->Architecture != STPTI_DEVICE_PTI_4)||
        (stptiMemGet_Device(FullFilterHandle)->SectionFilterOperatingMode != STPTI_FILTER_OPERATING_MODE_32xANY)) */

ST_ErrorCode_t stpti_HAL_Validate_FilterNotMatchModeSetup( FullHandle_t FullFilterHandle )
{
    return( ST_NO_ERROR ); 
}


/* ------------------------------------------------------------------------- */

#endif /* STPTI_NO_PARAMETER_CHECK */


/* ------------------------------------------------------------------------- */


/* EOF */

