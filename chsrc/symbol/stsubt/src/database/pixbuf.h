/******************************************************************************\
 *
 * File Name   : pixbuf.h
 *
 * Description : Definition of the Pixel Buffer
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/

#ifndef __PIXBUF_H
#define __PIXBUF_H

#include <stddefs.h>

#define PIXEL_BUFFER_ERROR_NOT_ENOUGHT_SPACE	1

/* ---------------------------------------------------------- */
/* --- Define a region bitmap type into the Pixel Buffer  --- */
/* ---------------------------------------------------------- */
 
typedef struct STSUBT_Bitmap_s {
  BOOL UpdateFlag;	/* bitmap content changed */ 
  U16  Width;
  U16  Height;
  U8   BitsPerPixel;
  U8  *DataPtr; 
} STSUBT_Bitmap_t;

#define BITMAP_HEADER_SIZE       sizeof(STSUBT_Bitmap_t) 


/* --------------------------------------------- */
/* --- Defines a Pixel Buffer data structure --- */
/* --------------------------------------------- */

typedef struct PixelBuffer_s {
  ST_Partition_t *MemoryPartition;
  U32 	BufferSize;
  U32 	NumBytesStored;
  U8 	*Buffer_p;
  U8 	*Free_p;
  BOOL 	DataInvalidated;
} PixelBuffer_t ;


/* ---------------------------------------- */
/* --- Pixel Buffer: Exported Functions --- */
/* ---------------------------------------- */

/* Create the Pixel Buffer */

PixelBuffer_t * PixelBuffer_Create (ST_Partition_t *MemoryPartition,
                                    U32 BufferSize);

/* Delete the Pixel Buffer */

void PixelBuffer_Delete (PixelBuffer_t *cbuffer_p);

/* Allocate memory for a region into the Pixel Buffer */

U8 *PixelBuffer_Allocate (PixelBuffer_t *cbuffer_p, 
                          U16 Width,
                          U16 Height,
                          U8  BitsPerPixel,
                          U32 RegionSize);

/* Reset the Pixel Buffer */

void PixelBuffer_Reset (PixelBuffer_t *cbuffer_p);

#endif  /* #ifndef __PIXBUF_H */

