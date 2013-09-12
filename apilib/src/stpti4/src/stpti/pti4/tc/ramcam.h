/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: ramcam.h
 Description: PTI cam HAL.

******************************************************************************/

#ifndef __RAMCAM_H
 #define __RAMCAM_H

/* Includes ------------------------------------------------------------ */


#include "stddefs.h"
#include "stdevice.h"


/* ------------------------------------------------------------------------- */

/*

 768 bytes per CAM (A&B) which is 1.5K in total.

  The minimal allocation size is 4 filters of 8 bytes data & 8 bytes mask,
 which is 64 bytes. This means we can fit in 12 blocks per CAM or 24 x 4 (96)
 filter for the whole SF. The minimal stpti_init() allocation is 8 filters, 4 each
 from CAMs A&B, also those items allocated MUST be at the SAME offset (address) as
 each other in their respective CAM areas.

  Each entry consists of 4 bytes & 4 masks per set (2 words)
 Unlike the old CAM the 4 bytes refer to 4 filters per word and NOT 4 bytes of
 one filter.

  Of course for all this to work properly CAM_Config[0] (CAM_Not_Mask) *MUST* be set to zero
 or else the masks will be be treated as filter bytes!
 
*/

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
#define SF_BYTES_PER_CAM     1024   /* Fixed value from hardware specs. */
#else
#define SF_BYTES_PER_CAM       768  /* Fixed value from hardware specs. */
#endif
#define SF_FILTER_LENGTH         8  /* Minimum CAM filter length, must be multiple of 8 (8=short section filter mode) */
#define SF_CAMLINE_WIDTH         4  /* Fixed value from hardware specs. */

#define SF_BYTES_PER_MIN_ALLOC ( SF_CAMLINE_WIDTH * (SF_FILTER_LENGTH * 2)) /* N filters of M bytes & masks */
#define SF_NUM_FILTERS_PER_BLOCK ( SF_CAMLINE_WIDTH * 2 )       /* block spans the 2 CAMs each holding SF_CAMLINE_WIDTH filters */
#define SF_NUM_BLOCKS_PER_CAM  ( SF_BYTES_PER_CAM / SF_BYTES_PER_MIN_ALLOC ) 

#define NOT_MATCH_ENABLE    0x20
#define NOT_MATCH_DATA_MASK 0x1f
#define NOT_MATCH_MASK_MASK 0xc1        /* Clear mask bits 1-5 for NotMatch Mode */

#if defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) && (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 2)
#define TC_NUMBER_OF_HARDWARE_NOT_FILTERS 128
#elif defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE)
#define TC_NUMBER_OF_HARDWARE_NOT_FILTERS 96
#else
#define TC_NUMBER_OF_HARDWARE_NOT_FILTERS 32
#endif


/* ------------------------------------------------------------------------- */
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCFilterLine_t)
#endif
typedef volatile union TCFilterLine_s 
{
    struct
    {
        U8 Filter[SF_CAMLINE_WIDTH];
    } Element;

    U32 Word;

} TCFilterLine_t;

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1) 
#pragma ST_device(TCCamEntry_t)
#endif
typedef volatile struct TCCamEntry_s 
{
    TCFilterLine_t Data;
    TCFilterLine_t Mask;
} TCCamEntry_t;
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCCamIndex_t)
#endif
typedef volatile struct TCCamIndex_s 
{
    TCCamEntry_t Index[ SF_FILTER_LENGTH ];
} TCCamIndex_t;

#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
#pragma ST_device(TCSectionFilterArrays_t)
#endif
typedef volatile struct TCSectionFilterArrays_s 
{
    TCCamIndex_t CamA_Block[ SF_NUM_BLOCKS_PER_CAM ];
#if !defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) || (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
    U32          ReservedA[256/4];
#endif
    TCCamIndex_t CamB_Block[ SF_NUM_BLOCKS_PER_CAM ];
#if !defined(STPTI_ARCHITECTURE_PTI4_SECURE_LITE) || (STPTI_ARCHITECTURE_PTI4_SECURE_LITE == 1)
    U32          ReservedB[256/4];
#endif

    U32          NotFilter[ TC_NUMBER_OF_HARDWARE_NOT_FILTERS ];
} TCSectionFilterArrays_t;


#endif /* __RAMCAM_H */


/* EOF --------------------------------------------------------------------- */
 
