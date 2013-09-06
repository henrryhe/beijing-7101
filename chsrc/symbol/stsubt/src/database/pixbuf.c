/******************************************************************************\
 *
 * File Name    : pixbuf.c
 *
 * Description  : 
 *     Pixel Buffer is used to reserve memory for all regions that are (planned)
 *     to be used during an epoch. The pixel buffer memory is allocated for a 
 *     display set that follows a PCS with page_state = mode_change. 
 *     Once introduced, the content of the pixel buffer relative to a region 
 *     accumulates consecutives modifications made in each display set. 
 *     The memory allocation is retained until a new PCS with 
 *     page_state = mode_change destroys all the memory allocation, leaving 
 *     the contents undefined. 
 *
 * Author : Marino Congiu - Jan 1999
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <stddefs.h>
#include <sttbx.h>


#include "pixbuf.h"


/******************************************************************************\
 * Function: PixelBuffer_Create
 * Purpose : Create a pixel buffer of suitable size
 * Parameters:
 *      MemoryPartition: 
 *           Partition from which memory is allocated.
 *      BufferSize: 
 *           Required size of buffer in bytes.
 * Return  : A pointer to created pixel buffer data structure; 
 *           NULL in case of error
\******************************************************************************/

PixelBuffer_t * PixelBuffer_Create (ST_Partition_t *MemoryPartition,
                                    U32 BufferSize)
{
    PixelBuffer_t *pbuffer_p;

    /* Alloc memory for buffer and initialize structure  */

    if ((pbuffer_p = (PixelBuffer_t *) 
        memory_allocate(MemoryPartition, sizeof(PixelBuffer_t))) == NULL)
    {
        return (NULL);
    }

    /* register partition memory */

    pbuffer_p->MemoryPartition = MemoryPartition;

    /* Alloc memory buffer */

    if ((pbuffer_p->Buffer_p = 
        memory_allocate(MemoryPartition, BufferSize))  == NULL)   
    {
        memory_deallocate(MemoryPartition, pbuffer_p);
        return (NULL);
    }

    pbuffer_p->Free_p = pbuffer_p->Buffer_p;
    pbuffer_p->BufferSize = BufferSize;
    pbuffer_p->NumBytesStored = 0;

    pbuffer_p->DataInvalidated = FALSE;

    return (pbuffer_p);                                           
}


/******************************************************************************\
 * Function: PixelBuffer_Delete
 * Purpose : Delete the pixel buffer; 
 *           All allocated resources are freed.
\******************************************************************************/

void PixelBuffer_Delete(PixelBuffer_t *pbuffer_p)
{
    ST_Partition_t *MemoryPartition = pbuffer_p->MemoryPartition;
    memory_deallocate (MemoryPartition, pbuffer_p->Buffer_p);
    memory_deallocate (MemoryPartition, pbuffer_p);
}


/******************************************************************************\
 * Function: PixelBuffer_Reset
 * Purpose : Reset the status of the buffer.
\******************************************************************************/
 
void PixelBuffer_Reset (PixelBuffer_t *pbuffer_p)
{
    pbuffer_p->Free_p = pbuffer_p->Buffer_p;

    pbuffer_p->NumBytesStored = 0;

    pbuffer_p->DataInvalidated = FALSE;
}


/******************************************************************************\
 * Function: PixelBuffer_Allocate
 * Purpose : Allocates memory for a region into the Pixel Buffer.
 *           Creates the corresponding bitmap header.
 * Return  : A pointer to allocated region
\******************************************************************************/

U8 *PixelBuffer_Allocate (PixelBuffer_t *pbuffer_p, 
                          U16 Width,
                          U16 Height,
                          U8  BitsPerPixel,
                          U32 RegionSize)
{
  U8  *Region_p;
  STSUBT_Bitmap_t *Bitmap_p;
 
  RegionSize += BITMAP_HEADER_SIZE;

  /* check if there is enoght space in buffer */
 
  if ((pbuffer_p->NumBytesStored + RegionSize) <= pbuffer_p->BufferSize)
  {
      /* allocate the Region starting from the first free byte in buffer */
 
      Region_p = pbuffer_p->Free_p;
 
      /* fill in bitmap header */
      
      Bitmap_p = (STSUBT_Bitmap_t*)Region_p;

      Bitmap_p->UpdateFlag = FALSE;
      Bitmap_p->BitsPerPixel = BitsPerPixel;
      Bitmap_p->Width = Width;
      Bitmap_p->Height = Height;
      Bitmap_p->DataPtr = (Region_p + BITMAP_HEADER_SIZE);

      /* update control information */
 
      pbuffer_p->Free_p += RegionSize;
      pbuffer_p->NumBytesStored += RegionSize;
 
      return (Region_p);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** PixelBuffer_Allocate: no more space **\n"));
      return (NULL);
  }
}

