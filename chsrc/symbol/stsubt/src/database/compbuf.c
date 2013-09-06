/******************************************************************************\
 *
 * File Name    : compbuf.c
 *
 * Description  : 
 *      The Composition Buffer includes PCS, RCSs and CLUT family definitions.  
 *
 *      - The Composition Buffer creates a linked list of the CLUTs used into 
 *        the current epoch. 
 *      - The Composition Buffer creates a linked list of the regions and CLUTs
 *        planned into an epoch. 
 *      - The PCS Processor updates or reset the Composition Buffer. 
 *      - The RCS Processor stores regions definitions . 
 *      - The CLUT Processor stores CLUT definitions
 *      - The Object Processor stores objects/regions references
 *      - The Display Engine read data from the composition buffer.
 *
 * Note :
 *      Composition Buffer is both used to hold structures whose life time is
 *      an epoch, and to hold structures, like the visible region list of a 
 *      PCS, whose life time is a display set.  
 *      "Epoch" structures are allocated on a starting epoch and need to be 
 *      holded for the whole epoch, while "display set" structures are allocated
 *      on a starting display set and removed at the and of the same.
 *      For that reason, two Composition Buffer reset operations are provided:
 *      CompBuffer_EpochReset to reset buffer at the end of an epoch, and
 *      CompBuffer_DisplaySetReset to reset buffer at the end of a display set.
 *
 *      Memory for "Epoch" structures" is allocated upward starting from 
 *      begining of buffer, while "display set" structures are allocated 
 *      downward starting from the end of the buffer.
 *
 *      Visible regions associated to a Page Composition may require
 *      to be kept across display sets, so, visible region items are
 *      allocated from a little circular buffer (VisRegionBuffer_p)
 *      which is included into Composition Buffer as well.
 *
 * Author : Marino Congiu - Jannuary 1999
 *
 * (c) Copyright STMicroelectronics - 1999
 *
\******************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <stddefs.h>
#include <subtdev.h>

#include "compbuf.h"

/* --- Define the default CLUTs --- */

/* Entry values have been precomputed for efficiency. 
 * Note: Entry color model is YCrCb. 
 */ 

/* 4-entry default CLUT */

static CLUT_Entry_t two_bit_entry_default_CLUT[4] = {
    {-1, 0x108080ff}, {-1, 0x47808000}, {-1, 0x10808000}, {-1, 0x2b808000} };

/* 16-entry default CLUT */

static CLUT_Entry_t four_bit_entry_default_CLUT[16] = {
    {-1, 0x108080ff}, {-1, 0x209c7600}, {-1, 0x30686d00}, {-1, 0x41846300},
    {-1, 0x167b9c00}, {-1, 0x27979200}, {-1, 0x37638900}, {-1, 0x47808000},
    {-1, 0x10808000}, {-1, 0x188e7b00}, {-1, 0x20747600}, {-1, 0x28827100},
    {-1, 0x137d8e00}, {-1, 0x1b8b8900}, {-1, 0x23718400}, {-1, 0x2b808000} };

/* 256-entry default CLUT */

static CLUT_Entry_t eight_bit_entry_default_CLUT[256] = {
    {-1, 0x108080ff}, {-1, 0x209c76bf}, {-1, 0x30686dbf}, {-1, 0x418463bf},
    {-1, 0x167b9cbf}, {-1, 0x279792bf}, {-1, 0x376389bf}, {-1, 0x478080bf},
    {-1, 0x1080807f}, {-1, 0x15897c7f}, {-1, 0x1a78797f}, {-1, 0x2081767f},
    {-1, 0x127e897f}, {-1, 0x1787867f}, {-1, 0x1c76837f}, {-1, 0x2280807f},
    {-1, 0x1b937900}, {-1, 0x209c7600}, {-1, 0x258b7300}, {-1, 0x2b947000},
    {-1, 0x1d918200}, {-1, 0x229a7f00}, {-1, 0x28897c00}, {-1, 0x2d937900},
    {-1, 0x1b93797f}, {-1, 0x209c767f}, {-1, 0x258b737f}, {-1, 0x2b94707f},
    {-1, 0x1d91827f}, {-1, 0x229a7f7f}, {-1, 0x28897c7f}, {-1, 0x2d93797f},
    {-1, 0x25707300}, {-1, 0x2b797000}, {-1, 0x30686d00}, {-1, 0x36716900},
    {-1, 0x276e7c00}, {-1, 0x2d777900}, {-1, 0x32667600}, {-1, 0x38707300},
    {-1, 0x2570737f}, {-1, 0x2b79707f}, {-1, 0x30686d7f}, {-1, 0x3671697f},
    {-1, 0x276e7c7f}, {-1, 0x2d77797f}, {-1, 0x3266767f}, {-1, 0x3870737f},
    {-1, 0x30836c00}, {-1, 0x368c6900}, {-1, 0x3b7b6600}, {-1, 0x41846300},
    {-1, 0x33817600}, {-1, 0x388a7300}, {-1, 0x3d797000}, {-1, 0x43836c00},
    {-1, 0x30836c7f}, {-1, 0x368c697f}, {-1, 0x3b7b667f}, {-1, 0x4184637f},
    {-1, 0x3381767f}, {-1, 0x388a737f}, {-1, 0x3d79707f}, {-1, 0x43836c7f},
    {-1, 0x147c9300}, {-1, 0x19868f00}, {-1, 0x1f758c00}, {-1, 0x247e8900},
    {-1, 0x167b9c00}, {-1, 0x1b849900}, {-1, 0x21739600}, {-1, 0x267c9300},
    {-1, 0x147c937f}, {-1, 0x19868f7f}, {-1, 0x1f758c7f}, {-1, 0x247e897f},
    {-1, 0x167b9c7f}, {-1, 0x1b84997f}, {-1, 0x2173967f}, {-1, 0x267c937f},
    {-1, 0x1f8f8c00}, {-1, 0x24998900}, {-1, 0x2a888600}, {-1, 0x2f918300},
    {-1, 0x218e9600}, {-1, 0x26979200}, {-1, 0x2c868f00}, {-1, 0x318f8c00},
    {-1, 0x1f8f8c7f}, {-1, 0x2499897f}, {-1, 0x2a88867f}, {-1, 0x2f91837f},
    {-1, 0x218e967f}, {-1, 0x2697927f}, {-1, 0x2c868f7f}, {-1, 0x318f8c7f},
    {-1, 0x2a6c8600}, {-1, 0x2f768300}, {-1, 0x34658000}, {-1, 0x3a6e7d00},
    {-1, 0x2c6b8f00}, {-1, 0x31748c00}, {-1, 0x36638900}, {-1, 0x3c6c8600},
    {-1, 0x2a6c867f}, {-1, 0x2f76837f}, {-1, 0x3465807f}, {-1, 0x3a6e7d7f},
    {-1, 0x2c6b8f7f}, {-1, 0x31748c7f}, {-1, 0x3663897f}, {-1, 0x3c6c867f},
    {-1, 0x35808000}, {-1, 0x3a897c00}, {-1, 0x40787900}, {-1, 0x45817600},
    {-1, 0x377e8900}, {-1, 0x3c878600}, {-1, 0x42768300}, {-1, 0x47808000},
    {-1, 0x3580807f}, {-1, 0x3a897c7f}, {-1, 0x4078797f}, {-1, 0x4581767f},
    {-1, 0x377e897f}, {-1, 0x3c87867f}, {-1, 0x4276837f}, {-1, 0x4780807f},
    {-1, 0x10808000}, {-1, 0x12847e00}, {-1, 0x157c7c00}, {-1, 0x18807b00},
    {-1, 0x117f8400}, {-1, 0x13838300}, {-1, 0x167b8100}, {-1, 0x19808000},
    {-1, 0x10808000}, {-1, 0x12847e00}, {-1, 0x157c7c00}, {-1, 0x18807b00},
    {-1, 0x117f8400}, {-1, 0x13838300}, {-1, 0x167b8100}, {-1, 0x19808000},
    {-1, 0x15897c00}, {-1, 0x188e7b00}, {-1, 0x1a857900}, {-1, 0x1d8a7800},
    {-1, 0x16888100}, {-1, 0x198d7f00}, {-1, 0x1b847e00}, {-1, 0x1e897c00},
    {-1, 0x15897c00}, {-1, 0x188e7b00}, {-1, 0x1a857900}, {-1, 0x1d8a7800},
    {-1, 0x16888100}, {-1, 0x198d7f00}, {-1, 0x1b847e00}, {-1, 0x1e897c00},
    {-1, 0x1a787900}, {-1, 0x1d7c7800}, {-1, 0x20747600}, {-1, 0x22787500},
    {-1, 0x1b777e00}, {-1, 0x1e7c7c00}, {-1, 0x21737b00}, {-1, 0x23787900},
    {-1, 0x1a787900}, {-1, 0x1d7c7800}, {-1, 0x20747600}, {-1, 0x22787500},
    {-1, 0x1b777e00}, {-1, 0x1e7c7c00}, {-1, 0x21737b00}, {-1, 0x23787900},
    {-1, 0x20817600}, {-1, 0x23867500}, {-1, 0x257d7300}, {-1, 0x28827100},
    {-1, 0x21807b00}, {-1, 0x24857900}, {-1, 0x267c7800}, {-1, 0x29817600},
    {-1, 0x20817600}, {-1, 0x23867500}, {-1, 0x257d7300}, {-1, 0x28827100},
    {-1, 0x21807b00}, {-1, 0x24857900}, {-1, 0x267c7800}, {-1, 0x29817600},
    {-1, 0x127e8900}, {-1, 0x14838700}, {-1, 0x177a8600}, {-1, 0x1a7f8400},
    {-1, 0x137d8e00}, {-1, 0x15828c00}, {-1, 0x18798a00}, {-1, 0x1b7e8900},
    {-1, 0x127e8900}, {-1, 0x14838700}, {-1, 0x177a8600}, {-1, 0x1a7f8400},
    {-1, 0x137d8e00}, {-1, 0x15828c00}, {-1, 0x18798a00}, {-1, 0x1b7e8900},
    {-1, 0x17878600}, {-1, 0x1a8c8400}, {-1, 0x1c838300}, {-1, 0x1f888100},
    {-1, 0x18878a00}, {-1, 0x1b8b8900}, {-1, 0x1e838700}, {-1, 0x20878600},
    {-1, 0x17878600}, {-1, 0x1a8c8400}, {-1, 0x1c838300}, {-1, 0x1f888100},
    {-1, 0x18878a00}, {-1, 0x1b8b8900}, {-1, 0x1e838700}, {-1, 0x20878600},
    {-1, 0x1c768300}, {-1, 0x1f7b8100}, {-1, 0x22728000}, {-1, 0x25777e00},
    {-1, 0x1d758700}, {-1, 0x207a8600}, {-1, 0x23718400}, {-1, 0x26768300},
    {-1, 0x1c768300}, {-1, 0x1f7b8100}, {-1, 0x22728000}, {-1, 0x25777e00},
    {-1, 0x1d758700}, {-1, 0x207a8600}, {-1, 0x23718400}, {-1, 0x26768300},
    {-1, 0x22808000}, {-1, 0x25847e00}, {-1, 0x277c7c00}, {-1, 0x2a807b00},
    {-1, 0x237f8400}, {-1, 0x26838300}, {-1, 0x287b8100}, {-1, 0x2b808000},
    {-1, 0x22808000}, {-1, 0x25847e00}, {-1, 0x277c7c00}, {-1, 0x2a807b00},
    {-1, 0x237f8400}, {-1, 0x26838300}, {-1, 0x287b8100}, {-1, 0x2b808000} };


/******************************************************************************\
 * Function: CompBuffer_Create
 * Purpose : Create a composition buffer of suitable size
 * Parameters:
 *      MemoryPartition: 
 *           Partition from which memory is allocated.
 *      BufferSize: 
 *           Required size of buffer in bytes.
 *      MaxNumVisRegion: 
 *           Max number of visible regions allocable 
 * Return  : A pointer to created composition buffer data structure; 
 *           NULL in case of error
\******************************************************************************/

CompBuffer_t * CompBuffer_Create (ST_Partition_t *MemoryPartition,
                                  U32 BufferSize, U32 MaxNumVisRegion)
{
    CompBuffer_t *cbuffer_p;
    U32 EpochBufferSize = BufferSize 
                        - (MaxNumVisRegion * sizeof(VisibleRegion_t));

    /* Alloc memory for buffer and initialize structure  */

    if ((cbuffer_p = (CompBuffer_t *)
             memory_allocate(MemoryPartition, sizeof(CompBuffer_t))) == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_Create: no space **\n"));
        return (NULL);
    }

    /* register partition memory */
    
    cbuffer_p->MemoryPartition = MemoryPartition;

    /* Alloc epoch buffer */
 
    if ((cbuffer_p->EpochBuffer_p = memory_allocate(MemoryPartition, 
                                                    BufferSize)) == NULL)
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_Create: no space **\n"));
        memory_deallocate(MemoryPartition, cbuffer_p);
        return (NULL);
    }

    /* Alloc buffer for visible region lists */

    cbuffer_p->VisRegionBuffer_p = (VisibleRegion_t*)
        memory_allocate(MemoryPartition, 
                        MaxNumVisRegion * sizeof(VisibleRegion_t));
    if (cbuffer_p->VisRegionBuffer_p == NULL)   
    {
        STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_Create: no space **\n"));
        memory_deallocate(MemoryPartition, cbuffer_p);
        memory_deallocate(MemoryPartition, cbuffer_p->EpochBuffer_p);
        return (NULL);
    }

    /* initialize the buffer */

    cbuffer_p->EpochBufferSize = EpochBufferSize;

    cbuffer_p->EndEpochBuffer_p = (void*)((U8*)cbuffer_p->EpochBuffer_p 
                                + EpochBufferSize);

    cbuffer_p->Epoch_Free_p = cbuffer_p->EpochBuffer_p;
 
    cbuffer_p->Epoch_AllocatedBytes = 0;
 
    cbuffer_p->DisplaySet_Free_p = cbuffer_p->EndEpochBuffer_p;
 
    cbuffer_p->DisplaySet_AllocatedBytes = 0;

    cbuffer_p->EpochRegionList_p = NULL;
    cbuffer_p->EpochCLUTList_p = NULL;

    cbuffer_p->VisRegion_Free_p = cbuffer_p->VisRegionBuffer_p;
    cbuffer_p->MaxNumVisRegion = MaxNumVisRegion;
    cbuffer_p->NumVisRegionAllocated = 0;

    return (cbuffer_p);                                           
}


/******************************************************************************\
 * Function: CompBuffer_Delete
 * Purpose : Delete the composition buffer; 
 *           All allocated resources are freed.
\******************************************************************************/

void CompBuffer_Delete(CompBuffer_t *cbuffer_p)
{
    ST_Partition_t *MemoryPartition = cbuffer_p->MemoryPartition;
    memory_deallocate(MemoryPartition, cbuffer_p->EpochBuffer_p);
    memory_deallocate(MemoryPartition, cbuffer_p->VisRegionBuffer_p);
    memory_deallocate(MemoryPartition, cbuffer_p);
}


/******************************************************************************\
 * Function: CompBuffer_EpochReset
 * Purpose : Reset the status of the buffer at the end of an epoch.
\******************************************************************************/
 
void CompBuffer_EpochReset (CompBuffer_t *cbuffer_p)
{
    cbuffer_p->Epoch_Free_p = cbuffer_p->EpochBuffer_p;

    cbuffer_p->Epoch_AllocatedBytes = 0;

    cbuffer_p->DisplaySet_Free_p = cbuffer_p->EndEpochBuffer_p;
 
    cbuffer_p->DisplaySet_AllocatedBytes = 0;

    cbuffer_p->VisRegion_Free_p = cbuffer_p->VisRegionBuffer_p;
    cbuffer_p->NumVisRegionAllocated = 0;

    cbuffer_p->EpochRegionList_p = NULL;

    cbuffer_p->EpochCLUTList_p = NULL;
}


/******************************************************************************\
 * Function: CompBuffer_DisplaySetReset
 * Purpose : Reset the status of the buffer at the end of display set.
\******************************************************************************/
 
void CompBuffer_DisplaySetReset (CompBuffer_t *cbuffer_p)
{
    RCS_t *lookup_p;

    cbuffer_p->DisplaySet_Free_p = cbuffer_p->EndEpochBuffer_p;
 
    cbuffer_p->DisplaySet_AllocatedBytes = 0;

    /* for each epoch region reset the object list */

    lookup_p = cbuffer_p->EpochRegionList_p;
    while (lookup_p != NULL)
    {
        lookup_p->object_list_p = NULL;
        lookup_p = lookup_p->next_region_p;
    }
}

/******************************************************************************\
 * Function: CompBuffer_NewVisibleRegion
 * Purpose : Allocate a VisibleRegion descriptor from VisRegionBuffer_p
 * Return  : A pointer to created data structure;
 *           NULL in case of error
\******************************************************************************/
 
VisibleRegion_t *CompBuffer_NewVisibleRegion (
                            CompBuffer_t *cbuffer_p,
                            U8  region_id,
                            U16 horizontal_position,
                            U16 vertical_position,
                            U32 AcquisitionMode)
{
  VisibleRegion_t *VisibleRegion_p;
  RCS_t *RCS;
 
  /* Get a VisibleRegion item (always successes) */
 
  VisibleRegion_p = cbuffer_p->VisRegion_Free_p;
 
  /* update control information */
 
  cbuffer_p->NumVisRegionAllocated =
      (cbuffer_p->NumVisRegionAllocated+1) % cbuffer_p->MaxNumVisRegion;
 
  if (cbuffer_p->NumVisRegionAllocated == 0)
       cbuffer_p->VisRegion_Free_p = cbuffer_p->VisRegionBuffer_p;
  else cbuffer_p->VisRegion_Free_p = cbuffer_p->VisRegion_Free_p + 1;

  /* fill in data structure with provided values */

  VisibleRegion_p->region_id = region_id;
  VisibleRegion_p->horizontal_position = horizontal_position;
  VisibleRegion_p->vertical_position = vertical_position;
  VisibleRegion_p->next_VisibleRegion_p = NULL;
  VisibleRegion_p->region_p = NULL;
 
  /* Link VisibleRegion to corresponding Region descriptor */

  if (AcquisitionMode == STSUBT_Normal_Case)
  {
      RCS = CompBuffer_GetRegion(cbuffer_p, region_id);
      if (RCS == NULL)
      {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_NewVisibleRegion: Region not found **\n"));
         return (NULL);
      }
      VisibleRegion_p->region_p = RCS;
  }

  return (VisibleRegion_p);
}


/******************************************************************************\
 * Function: CompBuffer_NewRegion
 * Purpose : Create a new RCS data structure including it in the EpochRegionList
 * Parameters:
 *      RCS:
 *           A filled in RCS data structurE
 * Return  : A pointer to created RCS; NULL in case of error
\******************************************************************************/

RCS_t *CompBuffer_NewRegion (CompBuffer_t *cbuffer_p, RCS_t RCS)
{
  U32 RCS_size;
  RCS_t *RCS_p;

  RCS_size = sizeof(RCS_t);

  /* check if there is enoght space in buffer */

  if (( cbuffer_p->Epoch_AllocatedBytes 
      + cbuffer_p->DisplaySet_AllocatedBytes 
      + RCS_size) <= cbuffer_p->EpochBufferSize)
  {
      /* allocate the region starting from the first free byte in buffer */

      RCS_p = (RCS_t*)cbuffer_p->Epoch_Free_p;

      /* fill in RCS data structure with provided values */

      RCS_p->region_id = RCS.region_id; 
      RCS_p->region_width = RCS.region_width; 
      RCS_p->region_height = RCS.region_height; 
      RCS_p->region_version_number = RCS.region_version_number; 
      RCS_p->region_depth = RCS.region_depth; 
      RCS_p->region_pixel_p = RCS.region_pixel_p; 
      RCS_p->CLUT_p = RCS.CLUT_p; 
      RCS_p->object_list_p = NULL; 

      /* insert RCS in EpochRegionList */

      RCS_p->next_region_p = (void *)cbuffer_p->EpochRegionList_p; 
      cbuffer_p->EpochRegionList_p = RCS_p;

      /* update control information */

      cbuffer_p->Epoch_Free_p += RCS_size;
      cbuffer_p->Epoch_AllocatedBytes += RCS_size;
 
      return (RCS_p);
  }
  else 
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_NewRegion: no space **\n"));

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Epoch_AllocatedBytes = %d\n", cbuffer_p->Epoch_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"DisplaySet_AllocatedBytes = %d\n", cbuffer_p->DisplaySet_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"EpochBufferSize = %d\n", cbuffer_p->EpochBufferSize));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"RCS_size = %d\n", RCS_size));

      return (NULL);
  }
}


/******************************************************************************\
 * Function: CompBuffer_GetRegion
 * Purpose : Find a region into the EpochRegionList
 * Parameters:
 *      region_id:
 *           region id value
 * Return  : A pointer to the corresponding RCS data structure;
 *           NULL if it was not found in the region list
\******************************************************************************/

RCS_t *CompBuffer_GetRegion (CompBuffer_t *cbuffer_p, U8 region_id)
{
   RCS_t *lookup_p;

   lookup_p = cbuffer_p->EpochRegionList_p;

   while (lookup_p != NULL)
   {
       if (lookup_p->region_id == region_id)
           break;
       lookup_p = lookup_p->next_region_p;
   }
   return (lookup_p);
}


/******************************************************************************\
 * Function: CompBuffer_NewObject
 * Purpose : Create an Object descriptor 
 * Parameters:
 *      Object:
 *           A filled in Object data structure 
 * Return  : A pointer to created Object data structure;
 *           NULL in case of error
\******************************************************************************/

Object_t *CompBuffer_NewObject (CompBuffer_t *cbuffer_p, Object_t Object)
{
  U32 Object_size;
  Object_t *object_p;
 
  Object_size = sizeof(Object_t);
 
  /* check if there is enought space in buffer */
 
  if (( cbuffer_p->Epoch_AllocatedBytes
      + cbuffer_p->DisplaySet_AllocatedBytes
      + Object_size) <= cbuffer_p->EpochBufferSize)
  {
      /* allocate the object starting from the first  
       * free byte in buffer downward from the end of buffer 
       */
 
      object_p = (Object_t*)(cbuffer_p->DisplaySet_Free_p - Object_size);
 
      /* fill in Object data structure with provided values */
 
      object_p->object_id = Object.object_id;
      object_p->horizontal_position = Object.horizontal_position;
      object_p->vertical_position = Object.vertical_position;
      object_p->next_object_p = NULL;
 
      /* update control information */
 
      cbuffer_p->DisplaySet_Free_p -= Object_size;
      cbuffer_p->DisplaySet_AllocatedBytes += Object_size;
 
      return (object_p);
  }
  else 
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_NewObject: no space **\n"));

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Epoch_AllocatedBytes = %d\n", cbuffer_p->Epoch_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"DisplaySet_AllocatedBytes = %d\n", cbuffer_p->DisplaySet_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"EpochBufferSize = %d\n", cbuffer_p->EpochBufferSize));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Object_size = %d\n", Object_size));

      return (NULL);
  }

}


/******************************************************************************\
 * Function: CompBuffer_NewCLUT
 * Purpose : Create a new CLUT data structure including it in the EpochCLUTList
 * Parameters:
 *      CLUT_id:
 *           The CLUT id value
 * Return  : A pointer to created CLUT; NULL in case of error
\******************************************************************************/
 
CLUT_t *CompBuffer_NewCLUT (CompBuffer_t *cbuffer_p, U8 CLUT_id)
{
  U32 CLUT_size;
  CLUT_t *CLUT_p;
 
  CLUT_size = sizeof(CLUT_t);
 
  /* check if there is enoght space in buffer */
 
  if (( cbuffer_p->Epoch_AllocatedBytes
      + cbuffer_p->DisplaySet_AllocatedBytes
      + CLUT_size) <= cbuffer_p->EpochBufferSize)
  {
      /* allocate the CLUT starting from the first free byte in buffer */
 
      CLUT_p = (CLUT_t*)cbuffer_p->Epoch_Free_p;
 
      /* fill in CLUT data structure with provided values */
 
      CLUT_p->CLUT_id = CLUT_id;
      CLUT_p->CLUT_version_number = 0;
      CLUT_p->two_bit_entry_CLUT_p = two_bit_entry_default_CLUT;
      CLUT_p->four_bit_entry_CLUT_p = four_bit_entry_default_CLUT;
      CLUT_p->eight_bit_entry_CLUT_p = eight_bit_entry_default_CLUT;
      CLUT_p->two_bit_entry_CLUT_flag = TRUE;
      CLUT_p->four_bit_entry_CLUT_flag = TRUE;
      CLUT_p->eight_bit_entry_CLUT_flag = TRUE;
 
      /* insert CLUT in EpochCLUTList */
 
      CLUT_p->next_CLUT_p = (void *)cbuffer_p->EpochCLUTList_p;
      cbuffer_p->EpochCLUTList_p = CLUT_p;
 
      /* update control information */
 
      cbuffer_p->Epoch_Free_p += CLUT_size;
      cbuffer_p->Epoch_AllocatedBytes += CLUT_size;
 
      return (CLUT_p);
  }
  else 
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_NewCLUT: no space **\n"));

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Epoch_AllocatedBytes = %d\n", cbuffer_p->Epoch_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"DisplaySet_AllocatedBytes = %d\n", cbuffer_p->DisplaySet_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"EpochBufferSize = %d\n", cbuffer_p->EpochBufferSize));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"CLUT_size = %d\n", CLUT_size));

      return (NULL);
  }
}
 

/******************************************************************************\
 * Function: CompBuffer_GetCLUT
 * Purpose : Find a CLUT into the EpochCLUTList
 * Parameters:
 *      CLUT_id:
 *           CLUT id value
 * Return  : A pointer to the corresponding CLUT data structure;
 *           NULL if it was not found in the CLUT list
\******************************************************************************/
 
CLUT_t *CompBuffer_GetCLUT (CompBuffer_t *cbuffer_p, U8 CLUT_id)
{
   CLUT_t *lookup_p;
 
   lookup_p = cbuffer_p->EpochCLUTList_p;
 
   while (lookup_p != NULL)
   {
       if (lookup_p->CLUT_id == CLUT_id)
           break;
       lookup_p = lookup_p->next_CLUT_p;
   }
   return (lookup_p);
}

/******************************************************************************\
 * Function: CompBuffer_AllocateCLUT
 * Purpose : Allocate a CLUT table into Composition Buffer coping
 *           default CLUT of approprate num of entries
 * Parameters:
 *      CLUT_p:
 *           Corresponding CLUT data structure
 *      nEntry:
 *           Number of entries in the CLUT, can be 4, 16, 256 
 * Return  : ST_NO_ERROR on success, 
 *           STSUBT_COMPBUFFER_NO_SPACE in case no space is available in buffer
 *           STSUBT_COMPBUFFER_BAD_NUM_ENTRIES provided bad num entries 
\******************************************************************************/
 
ST_ErrorCode_t CompBuffer_AllocateCLUT (CompBuffer_t *cbuffer_p, 
                                        CLUT_t *CLUT_p, U32 nEntry)
{
  U32  CLUT_table_size;
  CLUT_Entry_t *CLUT_table;
 
  if ((nEntry != 4) && (nEntry != 16) && (nEntry != 256))
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_AllocateCLUT: bad num entries **\n"));
      return (STSUBT_COMPBUFFER_BAD_NUM_ENTRIES);
  }
 
  CLUT_table_size = nEntry * sizeof(CLUT_Entry_t);
 
  /* check if there is enoght space in buffer */
 
  if (( cbuffer_p->Epoch_AllocatedBytes
      + cbuffer_p->DisplaySet_AllocatedBytes
      + CLUT_table_size) <= cbuffer_p->EpochBufferSize)
  {
      /* allocate the CLUT starting from the first free byte in buffer */
 
      CLUT_table = (CLUT_Entry_t*)cbuffer_p->Epoch_Free_p;
 
      /* copy corresponding default CLUT in new CLUT table */
      /* and update CLUT data structure */
 
      switch (nEntry) {
        case  4: 
            memcpy(CLUT_table, two_bit_entry_default_CLUT, CLUT_table_size);
            CLUT_p->two_bit_entry_CLUT_p = CLUT_table;
            CLUT_p->two_bit_entry_CLUT_flag = FALSE;
            break;
        case  16: 
            memcpy(CLUT_table, four_bit_entry_default_CLUT, CLUT_table_size);
            CLUT_p->four_bit_entry_CLUT_p = CLUT_table;
            CLUT_p->four_bit_entry_CLUT_flag = FALSE;
            break;
        case 256: 
            memcpy(CLUT_table, eight_bit_entry_default_CLUT, CLUT_table_size);
            CLUT_p->eight_bit_entry_CLUT_p = CLUT_table;
            CLUT_p->eight_bit_entry_CLUT_flag = FALSE;
            break;
      }
 
      /* update control information */
 
      cbuffer_p->Epoch_Free_p += CLUT_table_size;
      cbuffer_p->Epoch_AllocatedBytes += CLUT_table_size;
 
      return (ST_NO_ERROR);
  }
  else
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** CompBuffer_AllocateCLUT: no space **\n"));

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"Epoch_AllocatedBytes = %d\n", cbuffer_p->Epoch_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"DisplaySet_AllocatedBytes = %d\n", cbuffer_p->DisplaySet_AllocatedBytes));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"EpochBufferSize = %d\n", cbuffer_p->EpochBufferSize));
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"CLUT_table_size = %d\n", CLUT_table_size));

      return (STSUBT_COMPBUFFER_NO_SPACE);
  }
}


/******************************************************************************\
 * Function: CompBuffer_GetInfo
 * Purpose : get some field values of Composition Buffer;
 *           those which are changed when allocating ReferingRegions.
 *           Such a values are used to restore the Composition Buffer
 *           after the object has been decoded
\******************************************************************************/
 
void CompBuffer_GetInfo (CompBuffer_t *cbuffer_p, 
                         U32 *DisplaySet_AllocatedBytes,
                         U8 **DisplaySet_Free_p)
{
   *DisplaySet_AllocatedBytes = cbuffer_p->DisplaySet_AllocatedBytes;
   *DisplaySet_Free_p = cbuffer_p->DisplaySet_Free_p;
}

/******************************************************************************\
 * Function: CompBuffer_Restore
 * Purpose : Restore status of Composition Buffer as it was before
 *           starting decoding an object.
\******************************************************************************/
 
void CompBuffer_Restore (CompBuffer_t *cbuffer_p, 
                         U32 DisplaySet_AllocatedBytes,
                         U8 *DisplaySet_Free_p)
{
   cbuffer_p->DisplaySet_AllocatedBytes = DisplaySet_AllocatedBytes;
   cbuffer_p->DisplaySet_Free_p = DisplaySet_Free_p;
}


/******************************************************************************\
 * Function: CompBuffer_NewReferingRegion
 * Purpose : Create a new CLUT data structure including it in the EpochCLUTList
 * Parameters:
 *      CLUT_id:
 *           The CLUT id value
 * Return  : A pointer to created CLUT; NULL in case of error
\******************************************************************************/

ReferingRegion_t *CompBuffer_NewReferingRegion (CompBuffer_t *cbuffer_p,
                                                U8     region_depth,
                                                U8     NumPixelperByte,
                                                U16    region_width,
                                                U16    region_height,
                                                U8    *region_pixel_p,
                                                U16    ObjVertPos,
                                                U16    ObjHorizPos,
                                                U8    *ObjLookahead_p,
                                                U8     ObjOffset)

{
  U32 RefRegionSize;
  ReferingRegion_t *RefRegion_p;
 
  RefRegionSize = sizeof(ReferingRegion_t);
 
  /* check if there is enoght space in buffer */
 
  if (( cbuffer_p->Epoch_AllocatedBytes
      + cbuffer_p->DisplaySet_AllocatedBytes
      + RefRegionSize) <= cbuffer_p->EpochBufferSize)
  {
      /* allocate RefRegion_p starting from the first
       * free byte in buffer downward from the end of buffer
       */
 
      RefRegion_p = (ReferingRegion_t*)
                    (cbuffer_p->DisplaySet_Free_p - RefRegionSize);
 
      /* fill in data structure with provided values */
 
      RefRegion_p->region_depth = region_depth;
      RefRegion_p->NumPixelperByte = NumPixelperByte;
      RefRegion_p->region_width = region_width;
      RefRegion_p->region_height = region_height,
      RefRegion_p->region_pixel_p = region_pixel_p;
      RefRegion_p->ObjVertPos = ObjVertPos,
      RefRegion_p->ObjHorizPos = ObjHorizPos,
      RefRegion_p->ObjLookahead_p = ObjLookahead_p;
      RefRegion_p->ObjOffset = ObjOffset;
 
      RefRegion_p->next_ReferingRegion_p = NULL;
 
      /* update control information */
 
      cbuffer_p->DisplaySet_Free_p -= RefRegionSize;
      cbuffer_p->DisplaySet_AllocatedBytes += RefRegionSize;
 
      return (RefRegion_p);
  }
  else return (NULL);
}

/******************************************************************************\
 * Function: CompBuffer_BoundVisibleRegions
 * Purpose : Each visible region in the PCS is associated to corresponding
 *           region in the epoch region list.
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/

ST_ErrorCode_t CompBuffer_BoundVisibleRegions(PCS_t * PCS)
{
  RCS_t *RCS_lookup_p;
  VisibleRegion_t *VisibleRegion = PCS->VisibleRegion_p;

  while (VisibleRegion != NULL)
  {
      VisibleRegion = VisibleRegion;

      /* Link VisibleRegion to corresponding Region descriptor */

      RCS_lookup_p = PCS->EpochRegionList;
      while (RCS_lookup_p != NULL)
      {
          if (RCS_lookup_p->region_id == VisibleRegion->region_id)
              break;
          RCS_lookup_p = RCS_lookup_p->next_region_p;
      }

      if (RCS_lookup_p == NULL)
      {
         STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"## CompBuffer_BoundVisibleRegions: Region not found\n"));
         return (!ST_NO_ERROR);
      }
      VisibleRegion->region_p = RCS_lookup_p;
      VisibleRegion = VisibleRegion->next_VisibleRegion_p;
  }
  return (ST_NO_ERROR);
}

