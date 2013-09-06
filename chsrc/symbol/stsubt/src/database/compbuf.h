/******************************************************************************\
 * 
 * File Name   : compbuf.h
 * 
 * Description : Definition of Composition Data Buffer 
 * 
 * (c) Copyright STMicroelectronics - Jannuary 1999
 * 
\******************************************************************************/

#ifndef __COMPBUF_H
#define __COMPBUF_H

#include <stddefs.h>

/* --------------------------------------------- */
/* --- Define Composition Buffer error codes --- */
/* --------------------------------------------- */

#define STSUBT_COMPBUFFER_NO_SPACE  		1
#define STSUBT_COMPBUFFER_BAD_NUM_ENTRIES  	2


/* ------------------------------------------- */
/* --- Define the decoding data structures --- */
/* ------------------------------------------- */

typedef struct CLUT_TableEntry_s {
  S16  entry_version_number; 	/* CLUT version of last update */
  U32  entry_color_value;           
} CLUT_Entry_t;

/* --- Define the CLUT Definition data structure --- */

typedef struct CLUT_s {
  U8       CLUT_id;
  U8       CLUT_version_number;
  BOOL     two_bit_entry_CLUT_flag;
  BOOL     four_bit_entry_CLUT_flag;
  BOOL     eight_bit_entry_CLUT_flag;
  CLUT_Entry_t *two_bit_entry_CLUT_p;
  CLUT_Entry_t *four_bit_entry_CLUT_p;
  CLUT_Entry_t *eight_bit_entry_CLUT_p;
  void     *next_CLUT_p;
} CLUT_t;

/* --- Descriptor of an object rendered in a region --- */
 
typedef struct Object_s {
  U16      object_id;
  U16 	   horizontal_position;
  U16 	   vertical_position;
  void     *next_object_p;
} Object_t;

/* --- Define the Region Composition data structure --- */

typedef struct RCS_s {
  U8 	   region_id;
  U16  	   region_width;
  U16  	   region_height;
  U8  	   region_version_number;
  U8  	   region_depth;
  Object_t *object_list_p;
  CLUT_t   *CLUT_p;
  U8       *region_pixel_p;
  void     *next_region_p;
} RCS_t;

/* --- Define the list of regions visible in a page composition  --- */

typedef struct VisibleRegion_s {
  U8 	   region_id;
  RCS_t    *region_p;
  U16 	   horizontal_position;
  U16 	   vertical_position;
  void     *next_VisibleRegion_p;
} VisibleRegion_t;



/* --- Define the list of regions which refer to a given object --- */
 
typedef struct ReferingRegion_s {
  U8     region_depth;
  U8     NumPixelperByte;
  U16    region_width;
  U16    region_height;
  U8    *region_pixel_p;
  U16    ObjVertPos;
  U16    ObjHorizPos;
  U16    ObjWidth;
  U8    *ObjLookahead_p;
  U8     ObjOffset;
  void  *next_ReferingRegion_p;
} ReferingRegion_t;



/* ----------------------------------------------------- */
/* --- Defines the Composition Buffer data structure --- */
/* ----------------------------------------------------- */

typedef struct CompBuffer_s {
  void 	   *EpochBuffer_p;	/* alloc RCS and CLUT */
  U32 	   EpochBufferSize;
  U8   	   *Epoch_Free_p;
  U8   	   *DisplaySet_Free_p;
  U32 	   Epoch_AllocatedBytes;
  U32 	   DisplaySet_AllocatedBytes;
  RCS_t    *EpochRegionList_p;
  CLUT_t   *EpochCLUTList_p;
  void 	   *EndEpochBuffer_p;
  VisibleRegion_t *VisRegionBuffer_p;
  VisibleRegion_t *VisRegion_Free_p;
  U32      MaxNumVisRegion;
  U32      NumVisRegionAllocated;
  ST_Partition_t *MemoryPartition;
} CompBuffer_t ;



/* ----------------------------------------------------- */
/* --- Define the Page Composition data structure --- */
/* ----------------------------------------------------- */

#define STSUBT_MODE_CHANGE  	1
#define STSUBT_NORMAL_CASE 	2

typedef struct PCS_s {
  U16 	   page_id;
  U8  	   acquisition_mode;
  U8  	   page_version_number;
  clock_t  page_time_out;
  clock_t  presentation_time;  
  VisibleRegion_t *VisibleRegion_p;
  RCS_t   *EpochRegionList;
  U32      EpochNumber;
  CompBuffer_t *CompositionBuffer_p; 
} PCS_t;




/* ---------------------------------------------- */
/* --- Composition Buffer: Exported Functions --- */
/* ---------------------------------------------- */

/* Create the Composition Buffer */

CompBuffer_t * CompBuffer_Create (ST_Partition_t *MemoryPartition,
                                  U32 EpochBufferSize, U32 MaxNumVisRegion);

/* Delete the Composition Buffer */

void CompBuffer_Delete (CompBuffer_t *cbuffer_p);

/* Reset the Composition Buffer */

void CompBuffer_EpochReset (CompBuffer_t *cbuffer_p);
void CompBuffer_DisplaySetReset (CompBuffer_t *cbuffer_p);

/* Support routines for PCS processing */

VisibleRegion_t *CompBuffer_NewVisibleRegion (
                            CompBuffer_t *cbuffer_p,
                            U8  region_id,
                            U16 horizontal_position,
                            U16 vertical_position,
                            U32 AcquisitionMode);

/* Support routines for RCS processing */

RCS_t *CompBuffer_NewRegion (CompBuffer_t *cbuffer_p, RCS_t RCS);
RCS_t *CompBuffer_GetRegion (CompBuffer_t *cbuffer_p, U8 region_id);
Object_t *CompBuffer_NewObject (CompBuffer_t *cbuffer_p, Object_t Object);

/* Support routines for CLUT processing */

CLUT_t *CompBuffer_NewCLUT (CompBuffer_t *cbuffer_p, U8 CLUT_id);
CLUT_t *CompBuffer_GetCLUT (CompBuffer_t *cbuffer_p, U8 CLUT_id);
ST_ErrorCode_t CompBuffer_AllocateCLUT (CompBuffer_t *cbuffer_p, 
                                        CLUT_t *CLUT, U32 nEntry);

/* Support routines for Object processing */

void CompBuffer_GetInfo (CompBuffer_t *cbuffer_p,
                         U32 *DisplaySet_AllocatedBytes,
                         U8 **DisplaySet_Free_p);

void CompBuffer_Restore (CompBuffer_t *cbuffer_p,
                         U32 DisplaySet_AllocatedBytes,
                         U8 *DisplaySet_Free_p);

ReferingRegion_t *CompBuffer_NewReferingRegion (
                             CompBuffer_t *cbuffer_p,
                             U8     region_depth,
                             U8     NumPixelperByte,
                             U16    region_width,
                             U16    region_height,
                             U8    *region_pixel_p,
                             U16    ObjVertPos,
                             U16    ObjHorizPos,
                             U8    *ObjLookahead,
                             U8     ObjOffset);

ST_ErrorCode_t CompBuffer_BoundVisibleRegions(PCS_t * PCS);

#endif  /* #ifndef __COMPBUF_H */

