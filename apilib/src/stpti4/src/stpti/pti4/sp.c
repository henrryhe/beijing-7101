/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: sp.c
 Description: Sony Passage CA support

******************************************************************************/


/* Includes ---------------------------------------------------------------- */


#include <string.h>

#include "stddefs.h"
#include "stdevice.h"
#include "sttbx.h"
#include "stpti.h"

#include "pti_loc.h"
#include "pti_hndl.h"
#include "pti_hal.h"

#include "cam.h"
#include "tchal.h"

#include "pti4.h"

#ifndef STPTI_SP_SUPPORT
 #error Incorrect build options!
#endif


/* ------------------------------------------------------------------------- */


/******************************************************************************
Function Name : stptiHAL_SlotLinkShadowToLegacy
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotLinkShadowToLegacy(FullHandle_t ShadowSlotHandle, FullHandle_t LegacySlotHandle, STPTI_PassageMode_t PassageMode)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}


/******************************************************************************
Function Name : stptiHAL_SlotUnLinkShadowToLegacy
  Description :
   Parameters :
******************************************************************************/
ST_ErrorCode_t stptiHAL_SlotUnLinkShadowToLegacy(FullHandle_t ShadowSlotHandle, FullHandle_t LegacySlotHandle)
{
    return( ST_ERROR_FEATURE_NOT_SUPPORTED );
}

/* --- EOF --- */
