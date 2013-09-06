/******************************************************************************\
 *
 * File Name   : osdcb.h
 *  
 * Description : Exported types and function to control Display Devices
 *  
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Marino Congiu - Feb 1999
 *  
\******************************************************************************/

#ifndef __OSDCB_H
#define __OSDCB_H

#include <semaphor.h>
#include <stddefs.h>
#include <compbuf.h> 	/* for PCS_t */
#include <stosd.h>


/* -------------------------------------------------------------------------- */

#define STSUBT_OSD_NUM_DESCRIPTORS	5  /* available descriptors in       */
                                           /* service handle                 */
#define STSUBT_OSD_REGION_PER_EPOCH	30 /* estimated max number of regions*/
                                           /* storable per epoch             */
#define STSUBT_OSD_CLUT_PER_EPOCH	10 /* estimated max number of cluts  */
                                           /* storable per epoch             */
#define STSUBT_OSD_REGION_PER_DISPLAY	20 /* estimated max number of regions*/
                                           /* per display                    */

/* ------------------------------------------------------------------ */
/* --- Define structures which provide a description of a display --- */
/* ------------------------------------------------------------------ */

/* Regions planned for an epoch are linked to form a list */
 
typedef struct STSUBT_OSD_RegionDescriptor_s {
  U8                    region_id;
  S16                   region_version;
  U16                   horizontal_position;
  U16                   old_horizontal_position;
  U16                   vertical_position;
  U16                   old_vertical_position;
  STOSD_RegionHandle_t *RegionHandle;
  void                 *NextRegion_p;
} STSUBT_OSD_RegionDescriptor_t;


/* Descriptors of palettes planned for an epoch are linked to form a list */
 
typedef struct STSUBT_OSD_PaletteDescriptor_s {
  U32                   Palette_id;
  S32                   Palette_version;
  void                 *NextPalette_p;
} STSUBT_OSD_PaletteDescriptor_t;


/* --- Displays (picture on the screen) are described by a list of regions--- */

typedef struct STSUBT_OSD_DisplayRegion_s {
  STSUBT_OSD_RegionDescriptor_t *RegionDescriptor;
  void                          *NextDisplayRegion_p;
} STSUBT_OSD_DisplayRegion_t;


typedef struct STSUBT_OSD_DisplayDescriptor_s {
  STSUBT_OSD_DisplayRegion_t *RegionList;
  STSUBT_OSD_DisplayRegion_t *TailRegionList;
} STSUBT_OSD_DisplayDescriptor_t;

/* --- Define the Display Service Handle --- */

/* The service handle is mainly a reserved buffer from which all required 
 * memory is allocated. The handle also mantains the list of all regions
 * planned in an epoch and the descriptor of the display currenctely active. 
 */

/* The buffer is composed of 5 sections:
 * 1. reserved to allocate STSUBT_OSD_NUM_DESCRIPTORS display descriptors
 * 2. reserved to store RegionDescriptors of region planned in a epoch
 * 3. reserved to allocate memory visible region lists
 * 4. reserved to allocate OSD region handles
 * 5. reserved to allocate Palette Descriptors
 */

typedef struct STSUBT_OSD_ServiceHandle_s {
  ST_Partition_t *MemoryPartition;

  void  *Buffer_p;
  U8    *base_section_1;
  STSUBT_OSD_DisplayDescriptor_t *free_section_1_p;
  U32    MaxNumDescriptors;
  U32    nDescriptorsAllocated;

  U8    *base_section_2;
  STSUBT_OSD_RegionDescriptor_t *free_section_2_p;
  U32    MaxNumRegions;
  U32    nRegionsAllocated;

  U8    *base_section_3;
  STSUBT_OSD_DisplayRegion_t *free_section_3_p;
  U32    MaxNumDisplayRegions;
  U32    nDisplayRegionsAllocated;

  U8    *base_section_4;
  STOSD_RegionHandle_t *free_section_4_p;
  U32    MaxNumRegionHandlers;
  U32    nRegionsHandlersAllocated;

  U8    *base_section_5;
  STSUBT_OSD_PaletteDescriptor_t *free_section_5_p;
  U32    MaxNumPaletteDescriptors;
  U32    nPaletteDescriptorsAllocated;

  semaphore_t *DisplaySemaphore;
  STSUBT_OSD_DisplayDescriptor_t *ActiveDisplayDescriptor;
  STSUBT_OSD_RegionDescriptor_t  *EpochRegionList;
  STSUBT_OSD_PaletteDescriptor_t *EpochPaletteList;
  BOOL   Starting;
} STSUBT_OSD_ServiceHandle_t;
 

/* --------------------------------------------- */
/* --- OSD Display Device callback functions --- */
/* --------------------------------------------- */

#define STSUBT_OSD_DisplayDepth  	8

void *STSUBT_OSD_InitializeService (STSUBT_DisplayServiceParams_t *);
ST_ErrorCode_t STSUBT_OSD_TerminateService (void *ServiceHandle);

ST_ErrorCode_t STSUBT_OSD_PrepareDisplay (PCS_t *PageComposition_p, 
                                          void  *ServiceHandle,
                                          void **display_descriptor);

ST_ErrorCode_t STSUBT_OSD_ShowDisplay (void *ServiceHandle,
                                       void *display_descriptor);
ST_ErrorCode_t STSUBT_OSD_HideDisplay (void *ServiceHandle,
                                       void *display_descriptor);

#endif  /* #ifndef __OSDCB_H */

