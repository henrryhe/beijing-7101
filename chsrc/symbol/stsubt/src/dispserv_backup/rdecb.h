/******************************************************************************\
 *
 * File Name   : rdecb.h
 *  
 * Description : Exported types and function to control Display Devices
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Nunzio Raciti - Dec. 2000
 *  
\******************************************************************************/

#ifndef __RDECB_H
#define __RDECB_H

#include <semaphor.h>
#include <stsubt.h>
#include <subtdev.h>
#include <subtconf.h>
#include <stddefs.h>
#include <compbuf.h> 	/* for PCS_t */
#include <stgxobj.h>


#define RDE_BUFFER_MIN_SIZE  (24 * 1024)
#define RDE_MAX_REGIONS COMPOSITION_BUFFER_MAX_VIS_REGIONS

#define RDE_REGION_FREE         0x00
#define RDE_REGION_BIT_BUSY     0x01
#define RDE_REGION_BIT_SHOW     0x02
#define RDE_REGION_BIT_DISPLAY  0x04

#define RDE_OK          0
#define RDE_TIMEOUT     1
#define RDE_RESET       2


/* -------------------------------------------------------------------------- */

#define STSUBT_RDE_NUM_DESCRIPTORS	5  /* available descriptors in       */
                                           /* service handle                 */
#define STSUBT_RDE_REGION_PER_EPOCH	30 /* estimated max number of regions*/
                                           /* storable per epoch             */
#define STSUBT_RDE_CLUT_PER_EPOCH	10 /* estimated max number of cluts  */
                                           /* storable per epoch             */
#define STSUBT_RDE_REGION_PER_DISPLAY	20 /* estimated max number of regions*/
                                           /* per display                    */

/* ------------------------------------------------------------------ */
/* --- Define structures which provide a description of a display --- */
/* ------------------------------------------------------------------ */

/* Regions planned for an epoch are linked to form a list */
 
typedef struct STSUBT_RDE_RegionDescriptor_s {
  U8                    region_id;
  S16                   region_version;
  U16                   horizontal_position;
  U16                   old_horizontal_position;
  U16                   vertical_position;
  U16                   old_vertical_position;
  STSUBT_RDE_RegionId_t *RegionHandle;
  void                 *NextRegion_p;
} STSUBT_RDE_RegionDescriptor_t;


/* Descriptors of palettes planned for an epoch are linked to form a list */
 
typedef struct STSUBT_RDE_PaletteDescriptor_s {
  U32                   Palette_id;
  S32                   Palette_version;
  void                 *NextPalette_p;
} STSUBT_RDE_PaletteDescriptor_t;


/* --- Displays (picture on the screen) are described by a list of regions--- */

typedef struct STSUBT_RDE_DisplayRegion_s {
  STSUBT_RDE_RegionDescriptor_t *RegionDescriptor;
  void                          *NextDisplayRegion_p;
} STSUBT_RDE_DisplayRegion_t;


typedef struct STSUBT_RDE_DisplayDescriptor_s {
  STSUBT_RDE_DisplayRegion_t *RegionList;
  STSUBT_RDE_DisplayRegion_t *TailRegionList;
} STSUBT_RDE_DisplayDescriptor_t;

/* --- Define the Display Service Handle --- */

/* The service handle is mainly a reserved buffer from which all required 
 * memory is allocated. The handle also mantains the list of all regions
 * planned in an epoch and the descriptor of the display currenctely active. 
 */

/* The buffer is composed of 5 sections:
 * 1. reserved to allocate STSUBT_RDE_NUM_DESCRIPTORS display descriptors
 * 2. reserved to store RegionDescriptors of region planned in a epoch
 * 3. reserved to allocate memory visible region lists
 * 4. reserved to allocate RDE region handles
 * 5. reserved to allocate Palette Descriptors
 */

typedef struct STSUBT_RDE_ServiceHandle_s {
  ST_Partition_t *MemoryPartition;

  void  *Buffer_p;
  U8    *base_section_1;
  STSUBT_RDE_DisplayDescriptor_t *free_section_1_p;
  U32    MaxNumDescriptors;
  U32    nDescriptorsAllocated;

  U8    *base_section_2;
  STSUBT_RDE_RegionDescriptor_t *free_section_2_p;
  U32    MaxNumRegions;
  U32    nRegionsAllocated;

  U8    *base_section_3;
  STSUBT_RDE_DisplayRegion_t *free_section_3_p;
  U32    MaxNumDisplayRegions;
  U32    nDisplayRegionsAllocated;

  U8    *base_section_4;
  STSUBT_RDE_RegionId_t *free_section_4_p;
  U32    MaxNumRegionHandlers;
  U32    nRegionsHandlersAllocated;

  U8    *base_section_5;
  STSUBT_RDE_PaletteDescriptor_t *free_section_5_p;
  U32    MaxNumPaletteDescriptors;
  U32    nPaletteDescriptorsAllocated;

  semaphore_t *DisplaySemaphore;
  STSUBT_RDE_DisplayDescriptor_t *ActiveDisplayDescriptor;
  STSUBT_RDE_RegionDescriptor_t  *EpochRegionList;
  STSUBT_RDE_PaletteDescriptor_t *EpochPaletteList;
  void   *Handle_p;
  BOOL   Starting;
} STSUBT_RDE_ServiceHandle_t;
 

/* --------------------------------------------- */
/* --- RDE Display Device callback functions --- */
/* --------------------------------------------- */

#define STSUBT_RDE_DisplayDepth  	8

void *STSUBT_RDE_InitializeService (STSUBT_DisplayServiceParams_t *);
ST_ErrorCode_t STSUBT_RDE_TerminateService (void *ServiceHandle);

ST_ErrorCode_t STSUBT_RDE_PrepareDisplay (PCS_t *PageComposition_p, 
                                          void  *ServiceHandle,
                                          void **display_descriptor);

ST_ErrorCode_t STSUBT_RDE_ShowDisplay (void *ServiceHandle,
                                       void *display_descriptor);
ST_ErrorCode_t STSUBT_RDE_HideDisplay (void *ServiceHandle,
                                       void *display_descriptor);

#endif  /* #ifndef __RDECB_H */

