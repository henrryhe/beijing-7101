/******************************************************************************\
 *
 * File Name   : clut.c
 *
 * Description : Process a single clut segment
 *
 *               See SDD document for more details.
 *
 * Author : Marino Congiu - Jan 1998
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/

#include <stdlib.h>
 
#include <stddefs.h>
#include <stlite.h>
#include <stevt.h>

#include <subtdev.h>
 
#include "decoder.h"

/******************************************************************************\
 * Function: clut_processor
 *
 * Purpose : Process a single clut segment
 *
 * Description: 
 *
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t clut_processor (STSUBT_InternalHandle_t *DecoderHandle, 
                               Segment_t *Segment)
{
  CLUT_t *CLUT_p;
  ST_ErrorCode_t res;
  U8    CLUT_id;
  U8    previous_version_number;
  U8    current_version_number;
  U16   processed_length;
  U16   segment_length;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n** clut_processor: processing CLUT **\n"));

  if (DecoderHandle->AcquisitionMode == STSUBT_Starting)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** clut_processor: decoder starting, ignoring CLUT **\n"));
      return (ST_NO_ERROR);
  }

  /* --- extract segment_length, CLUT_id and version from CLUT segment --- */
 
  skip_bytes(Segment, 4);
 
  segment_length = ((U16)get_byte(Segment) << 8) + get_byte(Segment);

  CLUT_id = get_byte(Segment);

  current_version_number = get_byte(Segment) >> 4;

  /* --- check segment length --- */

  if ((segment_length + 6) != Segment->Size)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: bad segment **\n"));
      if (STEVT_Notify( DecoderHandle->stevt_handle,
          DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_SEGMENT
			 & 0x0000FFFF], DecoderHandle))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: Error notifying event**\n"));
      }

      return (STSUBT_ERROR_BAD_SEGMENT);
  }

  /* --- Retrieve the corresponding CLUT structure from the CLUT list --- */

  CLUT_p = CompBuffer_GetCLUT (DecoderHandle->CompositionBuffer, CLUT_id);

  if (CLUT_p == NULL)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: undefined CLUT referenced **\n"));
      if (STEVT_Notify(
                DecoderHandle->stevt_handle,
                DecoderHandle->stevt_event_table[STSUBT_EVENT_UNKNOWN_CLUT & 0x0000FFFF],
                DecoderHandle))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: Error notifying event**\n"));
      }
      return (ST_NO_ERROR);
  }

  /* --- get previous version number --- */

  previous_version_number = CLUT_p->CLUT_version_number;

  /* --- check CLUT segment version (when in Normal_Case only) --- */

  /* When any of the contents of the segment change, 
   * the version number is incremented (modulo 16)
   */
  
  /*
  if (DecoderHandle->AcquisitionMode == STSUBT_Normal_Case)
  { 
      if (newer_version(previous_version_number, current_version_number) != 1)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_FATAL,"** clut_processor: old version **\n"));
          return (ST_NO_ERROR);
      }
  }
  */

  /* --- Update CLUT segment version --- */

  CLUT_p->CLUT_version_number = current_version_number;

  /* --- Process segment --- */

  /* set segment_length to be number of bytes into the segment loop */

  segment_length -= 2;
  processed_length = 0;

  while (processed_length < segment_length)
  {
      U8  byte_flag;
      U8  two_bit_entry_CLUT_flag;
      U8  four_bit_entry_CLUT_flag;
      U8  eight_bit_entry_CLUT_flag;
      U8  full_range_flag;
      U8  CLUT_entry_id;
      U32 CLUT_entry_color;
      U8 *CLUT_entry_color_p = (U8*)&CLUT_entry_color;
      CLUT_Entry_t CLUT_TableEntry;

      STOP_PROCESSING_IF_REQUIRED(DecoderHandle);

      CLUT_entry_id = get_byte(Segment);

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"\n** clut_processor: getting entry (%d)**\n", CLUT_entry_id));

      byte_flag = get_byte(Segment);
      two_bit_entry_CLUT_flag = ((byte_flag & 0x80) >> 7);
      four_bit_entry_CLUT_flag = ((byte_flag & 0x40) >> 6);
      eight_bit_entry_CLUT_flag = ((byte_flag & 0x20) >> 5);


      full_range_flag = byte_flag & 0x1;

      /* --- check 2-bit entry CLUT --- */

      if (full_range_flag == 1 )
      {
          /* get clut entry */

          CLUT_entry_color = ((U32)get_byte(Segment) << 24)
                           + ((U32)get_byte(Segment) << 16)
                           + ((U32)get_byte(Segment) <<  8)
                           + get_byte(Segment);

          processed_length += 6;
      }
      else /* full_range_flag = 0 */
      {

          /* only most significant bits were trasmitted (6,4,4,2) */

          U32 Y, Cr, Cb, T;

          /* get clut entry */

          CLUT_entry_color = 0;
          CLUT_entry_color_p[1] = get_byte(Segment);
          CLUT_entry_color_p[0] = get_byte(Segment);

          Y = CLUT_entry_color_p[1] & 0xfc;

          Cr = ((CLUT_entry_color_p[1] & 0x03) << 6)
                | ((CLUT_entry_color_p[0] & 0xc0) >> 2);

          Cb = ((CLUT_entry_color_p[0] & 0x3c) << 2);

          T = ((CLUT_entry_color_p[0] & 0x03) << 6);

          CLUT_entry_color = (Y << 24) + (Cr << 16) + (Cb << 8) + T;

          processed_length += 4;
      }

      CLUT_TableEntry.entry_color_value = CLUT_entry_color;
      CLUT_TableEntry.entry_version_number = current_version_number;

      if ((two_bit_entry_CLUT_flag == 1) && (CLUT_entry_id < 4))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** clut_processor: processing 2-bit CLUT **\n"));

          /* check if CLUT structure refers to a default CLUT */
          if (CLUT_p->two_bit_entry_CLUT_flag == TRUE)
          {
              /* it was a default clut, so a new clut is loaded */

              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** clut_processor: loading 2-bit CLUT **\n"));

              /*  Allocate space for 4 entry and copy default CLUT */

              res = CompBuffer_AllocateCLUT(
                        DecoderHandle->CompositionBuffer, CLUT_p, 4);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: cannot load 2-bit CLUT **\n"));
                  if (res == STSUBT_COMPBUFFER_NO_SPACE)
                  if (STEVT_Notify(
                            DecoderHandle->stevt_handle,
                            DecoderHandle->stevt_event_table
                                            [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                            DecoderHandle))
                  {
                      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"**clut_processor: Error notifying event*\n"));
                  }
                  return (1);
              }
              CLUT_p->two_bit_entry_CLUT_flag = FALSE;
          }


          /* update clut */


          CLUT_p->two_bit_entry_CLUT_p[CLUT_entry_id] = CLUT_TableEntry;
      }

      /* --- check 4-bit entry CLUT --- */

      if ((four_bit_entry_CLUT_flag == 1) && (CLUT_entry_id < 16))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** clut_processor: processing 4-bit CLUT **\n"));

          /* check if CLUT structure refers to a default CLUT */
          if (CLUT_p->four_bit_entry_CLUT_flag == TRUE)
          {
              /* it was a default clut, so a new clut is loaded */

              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** clut_processor: loading 4-bit CLUT **\n"));

              res = CompBuffer_AllocateCLUT(
                        DecoderHandle->CompositionBuffer, CLUT_p, 16);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: cannot load 4-bit CLUT **\n"));
                  if (res == STSUBT_COMPBUFFER_NO_SPACE)
                  if (STEVT_Notify(
                        DecoderHandle->stevt_handle,
                        DecoderHandle->stevt_event_table
                                            [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                        DecoderHandle))
                  {
                      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: Error notifying event\n"));
                  }
                  return (1);
              }
              CLUT_p->four_bit_entry_CLUT_flag = FALSE;
          }
 
          /* update clut */

          CLUT_p->four_bit_entry_CLUT_p[CLUT_entry_id] = CLUT_TableEntry;
      }

      /* --- check 8-bit entry CLUT --- */
 
      if (eight_bit_entry_CLUT_flag == 1)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** clut_processor: processing 8-bit CLUT **\n"));

          /* check if CLUT structure refers to a default CLUT */
          if (CLUT_p->eight_bit_entry_CLUT_flag == TRUE)
          {
              /* it was a default clut, so a new clut is loaded */

              STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** clut_processor: loading 8-bit CLUT **\n"));

              
              res = CompBuffer_AllocateCLUT(
                        DecoderHandle->CompositionBuffer, CLUT_p, 256);
              if (res != ST_NO_ERROR)
              {
                  STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: cannot load 8-bit CLUT **\n"));
                  if (res == STSUBT_COMPBUFFER_NO_SPACE)
                  if (STEVT_Notify(
                        DecoderHandle->stevt_handle,
                        DecoderHandle->stevt_event_table
                                            [STSUBT_EVENT_COMP_BUFFER_NO_SPACE & 0x0000FFFF],
                        DecoderHandle))
                  {
                      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** clut_processor: Error notifying event\n"));
                  }
                  return (1);
              }
              CLUT_p->eight_bit_entry_CLUT_flag = FALSE;
          }

          /* update clut */


          CLUT_p->eight_bit_entry_CLUT_p[CLUT_entry_id] = CLUT_TableEntry;
      }
  }

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** clut_processor: CLUT processed **\n"));

  return (ST_NO_ERROR);
}

