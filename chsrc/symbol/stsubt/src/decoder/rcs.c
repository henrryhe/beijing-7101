/******************************************************************************\
 *
 * File Name   : rcs.c
 *
 * Description : Contain implementation of RCS Processor. It process a single 
 *               Region Composition Segment (RCS).
 *               (see description of single functions and SDD document for
 *                more detail.)
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Author      : Alessandro Innocenti - Jan 1999
 *
\******************************************************************************/
 
#include <stdlib.h>
#include <string.h>
 
#include <stddefs.h>
#include <stlite.h>
#include <stevt.h>

#include <subtdev.h>
#include "compbuf.h"
#include "pixbuf.h"
#include "decoder.h"


/******************************************************************************\
 * Function: AddToReferredObjectInCache
 * Purpose : Add a object descriptor to the list of objects resident in cache
 *           referred in a display set.
\******************************************************************************/
static void
AddToReferredObjectInCache (STSUBT_InternalHandle_t *DecoderHandle,
                            Object_t *Object)
{
  Object_t *lookup = DecoderHandle->ReferredObjectInCache;
  Object_t *next;

  Object->next_object_p = NULL;

  if ((lookup == NULL) || (Object->object_id < lookup->object_id))
  {
      Object->next_object_p = DecoderHandle->ReferredObjectInCache;
      DecoderHandle->ReferredObjectInCache = Object;
      return;
  }

  if (Object->object_id == lookup->object_id)
  {
     return;
  }
 
  while (lookup->next_object_p != NULL)
  {
      next = lookup->next_object_p;
      if (Object->object_id <= next->object_id)
      {
          if (Object->object_id == next->object_id)
          {
              return;
          }
          Object->next_object_p = next;
          break;
      }
      lookup = lookup->next_object_p;
  }

  lookup->next_object_p = Object;
}


/******************************************************************************\
 * Function: rcs_FillRegion
 *
 * Description: Read the correct region_n-bit_pixel_code
 *              and fill the region writing in the pixel_buffer memory.
 *
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
static 
ST_ErrorCode_t rcs_FillRegion (Segment_t *Segment,
                               RCS_t *CurrentRCS_Desc)
{
   U8   FillColor = 0;
   U8   cont = 0; 
   U8   ByteMask = 0;
   U8  *TmpPixBuf = NULL;
   U32  RegionSize = 0;

   STSUBT_Bitmap_t  *RegBitmapHead;

   switch(CurrentRCS_Desc->region_depth)
   {
      case 0x02 : /*2_bit_pixel :*/
         skip_bytes (Segment,1);
         FillColor = (get_byte(Segment) & 0x0c) >> 2;
         break;

      case 0x04 : /*4_bit_pixel :*/
         skip_bytes (Segment,1);
         FillColor = get_byte(Segment) >> 4;
         break;

      case 0x08 : /*8_bit_pixel :*/
         FillColor = get_byte(Segment);
         skip_bytes (Segment,1);
         break;

      default :
         break;
   }

   STTBX_Report((STTBX_REPORT_LEVEL_USER1,"* rcs_processor: Filling Region %d with color %d *\n",
              CurrentRCS_Desc->region_id,FillColor));

   TmpPixBuf = (CurrentRCS_Desc->region_pixel_p) + BITMAP_HEADER_SIZE;

   RegionSize=((CurrentRCS_Desc->region_depth * CurrentRCS_Desc->region_width *
               CurrentRCS_Desc->region_height) / 8) +
             (((CurrentRCS_Desc->region_depth * CurrentRCS_Desc->region_width *
                CurrentRCS_Desc->region_height ) % 8) != 0);


   for (cont = 0; cont < ((8 / CurrentRCS_Desc->region_depth )); cont++)
   {
      ByteMask = (ByteMask << (CurrentRCS_Desc->region_depth)) + FillColor;
   }
 
   STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: RegionSize %d Filling Mask %x **\n",
              RegionSize, ByteMask));

    memset (TmpPixBuf, ByteMask, RegionSize);

    RegBitmapHead = (STSUBT_Bitmap_t *) (CurrentRCS_Desc->region_pixel_p);

    RegBitmapHead->UpdateFlag = TRUE;

    return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: rcs_InsertObject
 *
 * Description: Insert object referenced by a RCS in Region object_list.
 *
 * Return  : ST_NO_ERROR on success,
 * 
 *
\******************************************************************************/

static __inline 
ST_ErrorCode_t rcs_InsertObject (STSUBT_InternalHandle_t *DecoderHandle, 
                                 Segment_t *Segment,
                                 U16 *processed_length, RCS_t *CurrentRCS_Desc)
{
   U8        TmpByte;
   U8        object_provided_flag = 0;
   U8        object_type = 0;
   Object_t  NewObject;
   Object_t *TmpObject_p;

   NewObject.object_id = ((U16)get_byte(Segment) << 8) + get_byte(Segment);

   TmpByte = get_byte(Segment);

   object_type = TmpByte >> 6 ;

   object_provided_flag = (TmpByte & 0x30) >> 4 ;

   NewObject.horizontal_position = ((U16)(TmpByte & 0x0f) << 8) +
                                    get_byte(Segment);

   TmpByte = get_byte(Segment);

   NewObject.vertical_position = ((U16)(TmpByte & 0x0f) << 8) +
                                  get_byte(Segment);

   *processed_length += 6;

    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: Obj_id=%d type=%d prov=%d hor=%d ver=%d*\n",
               NewObject.object_id, object_type,
               object_provided_flag, NewObject.horizontal_position,
               NewObject.vertical_position));

   if (object_type != 0x00) 
   {
       *processed_length += 2;

       STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: object type not supported **\n"));
       if (STEVT_Notify(
                 DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table
                                    [STSUBT_EVENT_NOT_SUPPORTED_OBJECT_TYPE & 0x0000FFFF],
                 DecoderHandle))
       {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
       }

       return(ST_NO_ERROR);
   }

   if ((DecoderHandle->WorkingMode == STSUBT_NormalMode) &&
       (object_provided_flag == 0x01))
   {
       STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: object type not supported **\n"));
       if (STEVT_Notify(
                 DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table
                                    [STSUBT_EVENT_NOT_SUPPORTED_OBJECT_TYPE & 0x0000FFFF],
                 DecoderHandle))
       {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
       }

       return(ST_NO_ERROR);
   }


   if ((DecoderHandle->WorkingMode == STSUBT_OverlayMode) &&
       (object_provided_flag == 0x01))
   {
       if ((TmpObject_p = CompBuffer_NewObject(DecoderHandle->CompositionBuffer,
                                               NewObject)) == NULL)
       {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: obj not stored in cache obj_list*\n"));
           if (STEVT_Notify(
                     DecoderHandle->stevt_handle,
                     DecoderHandle->stevt_event_table
                                        [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                     DecoderHandle))
           {
               STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
           }
           return(STSUBT_COMPBUFFER_NO_SPACE);
       }
       else
       {
       AddToReferredObjectInCache (DecoderHandle,TmpObject_p);
       }
   }


   if ((TmpObject_p = CompBuffer_NewObject(DecoderHandle->CompositionBuffer,
                                           NewObject)) == NULL)
   {
       STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: object not stored in obj_list **\n"));
       if (STEVT_Notify(
                 DecoderHandle->stevt_handle,
                 DecoderHandle->stevt_event_table
                                    [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                 DecoderHandle))
       {
           STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
       }
       return(STSUBT_COMPBUFFER_NO_SPACE);
   }
   else
   {
      TmpObject_p->next_object_p = CurrentRCS_Desc->object_list_p;

      CurrentRCS_Desc->object_list_p = TmpObject_p;
   }

    DecoderHandle->ObjectNumber++;

    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: Obj %d inserted in list of region %d *\n",
               NewObject.object_id, CurrentRCS_Desc->region_id));

   return (ST_NO_ERROR);
}




/******************************************************************************\
 * Function: rcs_processor
 *
 * Description : Process a single Region Composition Segment (RCS).
 *
 *               First it check the state of the decoder.
 *
 *               If the state is Acquisition the processor do the planning for
 *               the Epoch: 
 *               -Determine if the region shall be made visible;
 *               -Alloc the memory for the region in the Pixel Buffer;
 *               -Create region data structure;
 *               -Insert the region in the Composition Buffer regions list;
 *               -Create the referenced CLUT if it doesn't exist;
 *               Then:
 *               -If region_fill_flag is set it fill the region writing in the
 *                pixel Buffer memory;
 *               -Insert referenced object (if any) in object_list;
 *
 *               If the state is normal_case the processor check if the region
 *               is been already planed.
 *               Then:
 *               -Check version and visibility;
 *               -If region_fill_flag is set it fill the region writing in the
 *                pixel Buffer memory;
 *               -Insert referenced object (if any) in object_list;
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
 * Return  : ST_NO_ERROR on success,
 *
 *
\******************************************************************************/
 
ST_ErrorCode_t rcs_processor (STSUBT_InternalHandle_t *DecoderHandle, 
                              Segment_t *Segment)
{
    U8     region_id;
    U8     TmpByte;
    U8     region_version_number;
    U8     region_fill_flag;
    U8     region_level_of_compatibility;
    U8     region_depth;
    U8     Max_Display_depth;
    U8     CLUT_id;

    U16    segment_length;
    U16    processed_length = 0;

    U32    RegionSize;

    RCS_t           NewRCS;
    RCS_t          *CurrentRCS;
    CompBuffer_t   *CurrentCompBuf;
    PixelBuffer_t  *CurrentPixelBuf;

    STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** rcs_processor: Start processing RCS **\n"));

    CurrentCompBuf = DecoderHandle->CompositionBuffer; 
    CurrentPixelBuf = DecoderHandle->PixelBuffer; 
    Max_Display_depth = DecoderHandle->STSUBT_Display_Depth; 


    /*** Verify the state of the decoder ***/ 

    if (DecoderHandle->AcquisitionMode == STSUBT_Normal_Case)

    /*** Decoder is in Normal Case ***/
    {
        /*** Skip first 4 bytes of segment with redundant information  ***/ 

        skip_bytes (Segment,4);

        segment_length = ((U16)get_byte(Segment) << 8) + get_byte(Segment) + 6 ;

        region_id = get_byte(Segment);

        TmpByte = get_byte(Segment);
        region_version_number = TmpByte >> 4;
        region_fill_flag = ((TmpByte & 0x08) >> 3);

        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: Length=%d Reg_id=%d Vers=%d Fill=%d*\n",
              segment_length,region_id,region_version_number,region_fill_flag));

        /* --- check segment length --- */

        if (segment_length != Segment->Size)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: bad segment **\n"));
            if (STEVT_Notify(
                DecoderHandle->stevt_handle,
                DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_SEGMENT & 0x0000FFFF],
                DecoderHandle))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
            }
            return (STSUBT_ERROR_BAD_SEGMENT);
        }

        if ((CurrentRCS = CompBuffer_GetRegion (CurrentCompBuf,
                                                region_id)) == NULL)
        {
            /* Skip the segment because region doesn't exist in current epoch */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: region doesn't exist in epoch *\n"));
            if (STEVT_Notify(
                      DecoderHandle->stevt_handle,
                      DecoderHandle->stevt_event_table
                                         [STSUBT_EVENT_UNKNOWN_REGION & 0x0000FFFF],
                      DecoderHandle))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
            }
            return ST_NO_ERROR;
        }

        CurrentRCS->object_list_p = NULL;

        /*** Check visibility  ***/

        if (CurrentRCS->region_depth == 0)
        {
            /*** Skip the segment because it is not visible ***/
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: skip seg. because not visible *\n"));
            if (STEVT_Notify(
                      DecoderHandle->stevt_handle,
                      DecoderHandle->stevt_event_table
                                         [STSUBT_EVENT_INCOMPATIBLE_DEPTH & 0x0000FFFF],
                      DecoderHandle))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
            }
            return ST_NO_ERROR;
        }


        /*** Check segment data version ***/
 
        if (newer_version(CurrentRCS->region_version_number,
                          region_version_number) != 1)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: segment is of old version *\n"));
            return (ST_NO_ERROR);
        }

        CurrentRCS->region_version_number = region_version_number;

        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** rcs_processor: start processing existent REGION **\n"));

        /*** Skip 6 BYTES with redundant information  ***/ 

        skip_bytes (Segment,6);

        /*** Fill the region if necessary ***/ 

        if (region_fill_flag == 1)
        {
            rcs_FillRegion (Segment, CurrentRCS); 
        }
        else
        {
            skip_bytes (Segment,2);
        }


        /*** Insert Object in object list ***/ 

        processed_length += 16;


        while(processed_length <  segment_length)
        {

            if (rcs_InsertObject (DecoderHandle, Segment, 
                                  &processed_length,
                                  CurrentRCS) != ST_NO_ERROR)
            {
                return (1);
            }
        }

        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** rcs_processor: RCS processed **\n"));

        return ST_NO_ERROR;
    }

    else if (DecoderHandle->AcquisitionMode == STSUBT_Acquisition)

    /*** Decoder is in acquisition mode ***/

    {
        /*** Skip first 4 bytes of segment with redundant information  ***/ 

        skip_bytes (Segment,4);

        segment_length = ((U16)get_byte(Segment) << 8) + get_byte(Segment) + 6 ;

        /* check segment length */

        if (segment_length != Segment->Size)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: bad segment **\n"));
            if (STEVT_Notify(
                DecoderHandle->stevt_handle,
                DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_SEGMENT & 0x0000FFFF],
                DecoderHandle))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
            }
            return (STSUBT_ERROR_BAD_SEGMENT);
        }

        NewRCS.region_id = get_byte(Segment);
        TmpByte = get_byte(Segment);
        NewRCS.region_version_number = TmpByte >> 4;
        region_fill_flag = ((TmpByte & 0x08) >> 3);

        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: Length=%d Region_id=%d Ver=%d Fill=%d*\n",
                   segment_length, NewRCS.region_id,
                   NewRCS.region_version_number, region_fill_flag));

        NewRCS.region_width = ((U16)get_byte(Segment) << 8) + get_byte(Segment);

        NewRCS.region_height = ((U16)get_byte(Segment)<< 8) + get_byte(Segment);

        TmpByte = get_byte(Segment);
        region_level_of_compatibility = ((TmpByte & 0xe0) >> 5);
        region_level_of_compatibility = 1 << region_level_of_compatibility;
        region_depth = ((TmpByte & 0x1c) >> 2);
        region_depth = 1 << region_depth;

        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: weigth=%d height=%d comp=%d depth=%d *\n",
                   NewRCS.region_width, NewRCS.region_height, 
                   region_level_of_compatibility, region_depth));

        /*** Set the region_depth to the maximum possible        ***/
        /*** If is not possible show the region with the current ***/
        /*** Max_Display_depth set region_depth to 0             ***/

        if (region_depth <= Max_Display_depth)
        {
            NewRCS.region_depth = region_depth;
        }
        else
        {
            if (region_level_of_compatibility <= Max_Display_depth)
            {
                NewRCS.region_depth = Max_Display_depth;
            }
            else
            {
                /*** The region is not visible ***/

                NewRCS.region_depth = 0;
                return ST_NO_ERROR;
            }
        }

        CLUT_id = get_byte(Segment);

        NewRCS.CLUT_p = CompBuffer_GetCLUT(CurrentCompBuf, CLUT_id);
        if (NewRCS.CLUT_p == NULL)
        {
            NewRCS.CLUT_p = CompBuffer_NewCLUT (CurrentCompBuf, CLUT_id);
            if (NewRCS.CLUT_p == NULL)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Composition Buffer Full**\n"));
                if (STEVT_Notify(
                          DecoderHandle->stevt_handle,
                          DecoderHandle->stevt_event_table
                                            [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                          DecoderHandle))
               {
                   STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
               }
               return 1;
            }
        }

        RegionSize = ((NewRCS.region_depth * NewRCS.region_width *
                       NewRCS.region_height ) / 8) +
                     (((NewRCS.region_depth * NewRCS.region_width *
                       NewRCS.region_height ) % 8) != 0);

        if ((NewRCS.region_pixel_p = PixelBuffer_Allocate (CurrentPixelBuf,
                                                           NewRCS.region_width,
                                                           NewRCS.region_height,
                                                           NewRCS.region_depth,
                                                           RegionSize)) == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Pixel Buffer FULL **\n"));
            if (STEVT_Notify(
                      DecoderHandle->stevt_handle,
                      DecoderHandle->stevt_event_table
                                        [STSUBT_EVENT_PIXEL_BUFFER_NO_SPACE & 0x0000FFFF],
                      DecoderHandle))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
            }
            return (PIXEL_BUFFER_ERROR_NOT_ENOUGHT_SPACE);
        }

        NewRCS.object_list_p = NULL;

        if ((CurrentRCS = CompBuffer_NewRegion (CurrentCompBuf,NewRCS)) == NULL)
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Composition Buffer FULL **\n"));
            if (STEVT_Notify(
                      DecoderHandle->stevt_handle,
                      DecoderHandle->stevt_event_table
                                        [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                      DecoderHandle))
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** rcs_processor: Error notifying event**\n"));
            }
            return STSUBT_COMPBUFFER_NO_SPACE;
        }

        STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** rcs_processor: real_depth=%d CLUT_id=%d *\n",
                   NewRCS.region_depth, CLUT_id));

        /*** Fill the region if necessary ***/ 

        if (region_fill_flag == 1)
        {
            rcs_FillRegion (Segment, CurrentRCS);
        }
        else
        {
           skip_bytes (Segment,2);
        }

        /*** Insert Object in object list ***/ 

        processed_length += 16;
        

        while(processed_length <  segment_length)
        {
            rcs_InsertObject (DecoderHandle, Segment,
                              &processed_length, CurrentRCS);

        }

        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** rcs_processor: RCS processed **\n"));

        return ST_NO_ERROR;
    }

    else

    /*** Decoder is starting skip the segment ***/

    {
        STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** rcs_processor: skip segment decoder is in starting**\n"));
        return ST_NO_ERROR;
    }
    return ST_NO_ERROR;
}


