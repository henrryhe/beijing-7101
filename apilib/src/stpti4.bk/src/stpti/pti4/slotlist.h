/******************************************************************************
COPYRIGHT (C) STMicroelectronics 2005.  All rights reserved.

   File Name: slotlist.h
 Description: HAL for accessing the TC pid lookup table

******************************************************************************/

#ifndef __SLOTLIST_H
 #define __SLOTLIST_H


/* Includes ------------------------------------------------------------ */


#include "stddefs.h"
#include "stdevice.h"

#include "stpti.h"
#include "pti_loc.h"
#include "pti_hal.h"
#include "pti4.h"


/* ------------------------------------------------------------------------- */


ST_ErrorCode_t stptiHelper_SlotList_Init( FullHandle_t DeviceHandle );

ST_ErrorCode_t stptiHelper_SlotList_Term( FullHandle_t DeviceHandle );


ST_ErrorCode_t stptiHelper_SlotList_Alloc( FullHandle_t DeviceHandle );

ST_ErrorCode_t stptiHelper_SlotList_Free( FullHandle_t DeviceHandle );


/* ------------------------------------------------------------------------- */


#endif /* __SLOTLIST_H */


/* EOF --------------------------------------------------------------------- */
