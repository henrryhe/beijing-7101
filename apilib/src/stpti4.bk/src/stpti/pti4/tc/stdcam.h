/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: stdcam.h
 Description: PTI cam HAL.

******************************************************************************/

#ifndef __STDCAM_H
 #define __STDCAM_H

/* Includes ------------------------------------------------------------ */


#include "stddefs.h"
#include "stdevice.h"


/* ------------------------------------------------------------------------- */

#define TC_NUMBER_OF_HARDWARE_SECTION_FILTERS 32

#define NOT_MATCH_ENABLE    0x20
#define NOT_MATCH_DATA_MASK 0x1f


/* ------------------------------------------------------------------------- */
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSectionFilter_t)
#endif
typedef volatile struct
{
    U32 SFFilterDataLS;
    U32 SFFilterMaskLS;
    U32 SFFilterDataMS;
    U32 SFFilterMaskMS;
} TCSectionFilter_t;

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSectionFilterArrays_t)
#endif
typedef volatile struct
{
    TCSectionFilter_t FilterA  [TC_NUMBER_OF_HARDWARE_SECTION_FILTERS];
    TCSectionFilter_t FilterB  [TC_NUMBER_OF_HARDWARE_SECTION_FILTERS];
    U32               NotFilter[TC_NUMBER_OF_HARDWARE_SECTION_FILTERS];
} TCSectionFilterArrays_t;


/* ------------------------------------------------------------------------- */


#endif /* __STDCAM_H */


/* EOF --------------------------------------------------------------------- */
 
