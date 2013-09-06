/******************************************************************************\
 *
 * File Name   : testcb.h
 *  
 * Description : Display Device callback to use on testing
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author : Marino Congiu - Feb 1999
 *  
\******************************************************************************/

#ifndef __TESTCB_H
#define __TESTCB_H

#include <semaphor.h>
#include <stddefs.h>
#include <compbuf.h> 	/* for PCS_t */


/* ------------------------------------------------------------------ */
/* --- Define structures which provide a description of a display --- */
/* ------------------------------------------------------------------ */

typedef struct STTEST_RegionParams_s {
  U32   RegionId;
  U32   PositionX;
  U32   PositionY;
  U32   Width;
  U32   Height;
  U32   NbBitsPerPel;
} STTEST_RegionParams_t;


/* --- Displays (picture on the screen) are described by a list of regions--- */

typedef struct STSUBT_TEST_RegionItem_s {
  U8    region_id;
  U16   region_width;
  U16   region_height;
  U16   horizontal_position;
  U16   vertical_position;
  U8    region_depth;
  void *NextRegion;
} STSUBT_TEST_RegionItem_t;


typedef struct STSUBT_TEST_DisplayDescriptor_s {
  U32      nEpoch;
  U32      nDisplay;
  clock_t  presentation;
  clock_t  timeout;
  U16      page_id;
  U8       acquisition_mode;
  STSUBT_TEST_RegionItem_t *VisibleRegionList;
  void    *NextDescriptor;
} STSUBT_TEST_DisplayDescriptor_t;


/* --- Define the displaying Service handle for testing --- */

typedef struct STSUBT_TEST_ServiceHandle_s {
  ST_Partition_t                   *MemoryPartition;
  U32    		            nEpoch;
  U32    			    nDisplay;
  semaphore_t                      *DisplaySemaphore;
  STSUBT_TEST_RegionItem_t         *EpochRegionList;
  STSUBT_TEST_DisplayDescriptor_t  *ActiveDisplayDescriptor;
  STSUBT_TEST_DisplayDescriptor_t  *DisplayList;
  STSUBT_TEST_DisplayDescriptor_t  *DisplayListTail;
  BOOL                              Starting;
} STSUBT_TEST_ServiceHandle_t;
 

/* --------------------------------------------- */
/* --- TEST Display Device callback functions --- */
/* --------------------------------------------- */

#define STSUBT_TEST_DisplayDepth  	8

void *STSUBT_TEST_InitializeService (
                           STSUBT_DisplayServiceParams_t *);

ST_ErrorCode_t STSUBT_TEST_TerminateService (
                           void *ServiceHandle);

ST_ErrorCode_t STSUBT_TEST_PrepareDisplay (
                           PCS_t *PageComposition_p, 
                           void  *ServiceHandle,
                           void **display_descriptor);

ST_ErrorCode_t STSUBT_TEST_ShowDisplay (
                           void *ServiceHandle,
                           void *display_descriptor);
ST_ErrorCode_t STSUBT_TEST_HideDisplay (
                           void *ServiceHandle,
                           void *display_descriptor);

#endif  /* #ifndef __TESTCB_H */

