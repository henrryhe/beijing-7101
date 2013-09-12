/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: pti_sp.c
 Description: sony passage CA support.

******************************************************************************/


#ifndef STPTI_SP_SUPPORT
 #error Incorrect build option!
#endif


/* Includes ---------------------------------------------------------------- */


#ifdef STPTI_INTERNAL_DEBUG_SUPPORT
 #define STTBX_PRINT
#endif


#include <assert.h>
#include <string.h>

#include "stcommon.h"
#include "sttbx.h"
#include "stpti.h"

#include "pti_hndl.h"
#include "pti_evt.h"
#include "pti_list.h"
#include "pti_loc.h"
#include "pti_mem.h"
#include "pti_hal.h"
#include "validate.h"

#include "memget.h"


/* Functions --------------------------------------------------------------- */


/*-----------------------------------------------------------------------------
Function Name : STPTI_SlotLinkShadowToLegacy
  Description :
   Parameters :
-----------------------------------------------------------------------------*/
ST_ErrorCode_t STPTI_SlotLinkShadowToLegacy(STPTI_Slot_t ShadowSlotHandle, STPTI_Slot_t LegacySlotHandle, STPTI_PassageMode_t PassageMode )
{
    return( STPTI_ERROR_FUNCTION_NOT_SUPPORTED );
}


/*-----------------------------------------------------------------------------
Function Name : STPTI_SlotUnLinkShadowToLegacy
  Description :
   Parameters :
-----------------------------------------------------------------------------*/
ST_ErrorCode_t STPTI_SlotUnLinkShadowToLegacy(STPTI_Slot_t ShadowSlotHandle, STPTI_Slot_t LegacySlotHandle )
{
    return( STPTI_ERROR_FUNCTION_NOT_SUPPORTED );
}


/* ------------------------------------------------------------------------- */


/* --- EOF --- */
