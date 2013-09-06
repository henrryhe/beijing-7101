/******************************************************************************\
 *
 * File Name   : pcs.c
 *
 * Description : 
 *     Contains implementation of PCS Processor.  It processes a single 
 *     Page Composition Segment (PCS). The PCS Processor mantaines a 
 *     list of regions to be the displayed at the time indicated by the 
 *     last received PTS (that associated with the current display set). 
 *     After processed, the PCS is sent to Display Engine to arrange the 
 *     display. 
 *
 *     The PCS Processor invokes CompBuffer_DisplaySetReset to make a "soft"
 *     reset of the Composition Buffer at begining of a display set. 
 *
 *     The PCS Processor resets the Composition Buffer and Pixel Buffer 
 *     when an acquisition point is detected. 
 *
 *     The list of regions that will be visible in the display defined by 
 *     current PCS is created.
 *
 *     See SDD document for more details.
 *
 * Author : Marino Congiu - Jan 1998
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/
#include <stdlib.h>
#include <time.h>
 
#include <stddefs.h>
#include <stlite.h>
#include <stclkrv.h>
#include <stevt.h>

#include <subtdev.h>
#include <compbuf.h>
#include <pixbuf.h>

#include "decoder.h"



#define SCALE 100    /* Scale value for PTS to ticks conversion (get_sync_time()) */


/******************************************************************************\
 * Function: get_sync_time
 * Purpose : compute presentation time and page timeout.
 * Note    : computed presentation time and page timeout are converted 
 *           in number of clock ticks.
\******************************************************************************/
 
ST_ErrorCode_t get_sync_time (STSUBT_InternalHandle_t *DecoderHandle,
                              U8 page_time_out,
                              clock_t *presentation, 
                              clock_t *timeout)
{
  ST_ErrorCode_t res = ST_NO_ERROR;
  pts_t PTS = DecoderHandle->LastPTS;
  U32 ActualDecoderClock;

  /* --- compute presentation time --- */

  if (DecoderHandle->WorkingMode == STSUBT_NormalMode) 
  {
      res = STCLKRV_GetDecodeClk (DecoderHandle->stclkrv_handle,
                                  &ActualDecoderClock);
     
      switch (res) {
        case ST_ERROR_BAD_PARAMETER:
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor ERROR: getting decode time **\n"));
            return (res);
        }
        case ST_ERROR_INVALID_HANDLE:
        {
            STTBX_Report(
            (STTBX_REPORT_LEVEL_ERROR,"** pcs_processor ERROR: STCLKRV not open or inv handle\n"));
            return (res);
        }
        case STCLKRV_ERROR_PCR_UNAVAILABLE:
        {
	    STTBX_Report((STTBX_REPORT_LEVEL_USER1,"pcs_processor: PCR Unavailable !!"));
            /* No PCR so Freerun (DDTS 13897) */
             *presentation = time_plus (time_now(), (DecoderHandle->clocks_per_sec / 10));
            *timeout = time_plus (*presentation, (clock_t) (5 * DecoderHandle->clocks_per_sec));
            res = ST_NO_ERROR;	/* ignore error */
            break;
        }
        case ST_NO_ERROR :
        {
            STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: got PCR **\n"));

            if (PTS.low <= ActualDecoderClock)
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: ERROR: invalid PTS\n"));
                /* Bad PTS value so freerun (DDTS 13897)*/
                *presentation = time_plus (time_now(), (DecoderHandle->clocks_per_sec / 10));
                *timeout = time_plus (*presentation, (clock_t) (5 * DecoderHandle->clocks_per_sec));

            }
            else
            {
                U32 Diff90K;
                U32 Ticks;

                /* Get time to presentation in 90Khz units */
                Diff90K = PTS.low - ActualDecoderClock;
                
                /** Find time to presentation in terms of system time (clocks_per_sec / 90Khz)...
                 *
                 *  Diff90K should be relatively small value, since presentation is assumed to be
                 *  within the next few seconds. However, this cannot be guaranteed. So in the
                 *  below calculation care is taken to ensure both U32 is not exceeded and there
                 *  is sufficient room for a large Diff90K value, aswell as trying not to lose
                 *  too accuracy in the calculation.
                 *  Below can be seen choice of SCALE  values and their associated max time to
                 *  display in Diff90K before overflow. Values based on 5518 @ 81Mhz
                 *  with clocks per secs of 20748.
                 *  1000: 2300secs (38mins)
                 *  100:  230secs  (3.8mins)
                 *  10:   23secs
                 *  1:    2secs
                 *               
                 **/                
                Ticks = (( Diff90K * (DecoderHandle->clocks_per_sec / SCALE)) / (90000 / SCALE));

                /* Set presentation time and timeout */
                *presentation = time_plus (time_now(), Ticks);
                *timeout = time_plus (*presentation, page_time_out * DecoderHandle->clocks_per_sec);
		
            }

            break;
        }
        default:
        {
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: ERROR: STCLKRV inaspected error\n"));
            return (res);
        }
      }     
  }
  else /* overlay mode */
  {
      /* displays are processed immediatelly */
      *presentation = 0;
      *timeout = page_time_out;
  }
  return (res);
}


/******************************************************************************\
 * Function: AddToVisibleRegionList
 * Purpose : Add a VisibleRegion descriptor to the visible region list of
 *           a PCS. 
\******************************************************************************/
 
static __inline
void AddToVisibleRegionList (PCS_t *PCS, VisibleRegion_t *VisibleRegion)
{
    
  U16 vertical_position = VisibleRegion->vertical_position;
  VisibleRegion_t *lookup = PCS->VisibleRegion_p;
  VisibleRegion_t *next;

  if ((lookup == NULL) || (vertical_position <= lookup->vertical_position))
  {
      PCS->VisibleRegion_p = VisibleRegion;
      VisibleRegion->next_VisibleRegion_p = lookup;
      return;
  }

  while (lookup->next_VisibleRegion_p != NULL) 
  {
      next = lookup->next_VisibleRegion_p;
      if (vertical_position <= next->vertical_position)
      {
          VisibleRegion->next_VisibleRegion_p = next;
          lookup->next_VisibleRegion_p = VisibleRegion;
          break;
      }
      lookup = lookup->next_VisibleRegion_p;
  }

  lookup->next_VisibleRegion_p = VisibleRegion;
}


/******************************************************************************\
 * Function: CreateVisibleRegionList
 * Purpose : get/create the list of visible regions into this display 
\******************************************************************************/
 
ST_ErrorCode_t CreateVisibleRegionList (
                            STSUBT_InternalHandle_t *DecoderHandle,
                            Segment_t *PCS_Segment,
                            U16 segment_length,
                            STSUBT_AcquisitionMode_t AcquisitionMode)
{
  U8     region_id;
  U16    region_horizontal_position;
  U16    region_vertical_position;
  U16    processed_length;


  /* set segment_length to be the number of bytes into the segment loop */

  segment_length -= 2;
  processed_length = 0;

  while (processed_length < segment_length)
  {
      VisibleRegion_t *VisibleRegionItem;

      STOP_PROCESSING_IF_REQUIRED(DecoderHandle);

      region_id = get_byte(PCS_Segment);

      skip_bytes(PCS_Segment, 1);

      region_horizontal_position = ((U16)get_byte(PCS_Segment) << 8) 
                                 + get_byte(PCS_Segment);

      region_vertical_position   = ((U16)get_byte(PCS_Segment) << 8) 
                                 + get_byte(PCS_Segment);

      /* Create a VisibleRegion descriptor; it is linked 
       * to the corresponding RCS in the EpochregionList (normal case only).
       */
 
      VisibleRegionItem = CompBuffer_NewVisibleRegion (
                                     DecoderHandle->CompositionBuffer,
                                     region_id,
                                     region_horizontal_position,
                                     region_vertical_position,
                                     AcquisitionMode);
 
      /* corresponding RCS not found; skip region */

      if (VisibleRegionItem == NULL)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: corresponding region not found**\n"));
          if (STEVT_Notify(
                    DecoderHandle->stevt_handle,
                    DecoderHandle->stevt_event_table
                                           [STSUBT_EVENT_UNKNOWN_REGION & 0x0000FFFF],
                    DecoderHandle))
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: Error notifying event**\n"));
          }
          continue;
      }
 
      /* Add region to the PCS visible region list */

      AddToVisibleRegionList (&DecoderHandle->PCS, VisibleRegionItem);

      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: added visible region %d **\n", region_id));

      processed_length += 6;
  }
  return (ST_NO_ERROR);
}


/******************************************************************************\
 * Function: Notify_PCS
 * Purpose : Notify processed pcs to Display Engine (DVB Standard) or
 *           Display Timer (DTG Standard).
 *           PCS is not notified in overlay mode.
 *           That is obviously useless when driver is not displaying.
\******************************************************************************/
 
static __inline
ST_ErrorCode_t Notify_PCS (STSUBT_InternalHandle_t *DecoderHandle)
{
  ST_ErrorCode_t res = ST_NO_ERROR;


  if ((DecoderHandle->SubtitleDisplayEnabled == TRUE)
  &&  (DecoderHandle->WorkingMode == STSUBT_NormalMode) )
  {
      if (DecoderHandle->SubtitleStandard == STSUBT_DVBStandard)
      {
          /* PCS is sent to PCS Buffer to be processed by the Display Engine */
          res = fcBuffer_Insert (DecoderHandle->PCSBuffer, &DecoderHandle->PCS);
      }
      else /* DTG standard */
      {
          /* PCS is sent to Display Buffer to be processed by Display Timer */
          STSUBT_DisplayItem_t DisplayItem;
          PCS_t PCS = DecoderHandle->PCS;

          DisplayItem.PCS = PCS;
          DisplayItem.display_descriptor = NULL;
          DisplayItem.presentation_time = PCS.presentation_time;
          DisplayItem.display_timeout = PCS.page_time_out;
          DisplayItem.EpochNumber = DecoderHandle->EpochNumber;
          DisplayItem.PcsNumber = DecoderHandle->PcsNumber;

	  res = fcBuffer_Insert (DecoderHandle->DisplayBuffer, &DisplayItem);
      }
  }
  return (res);
}


/******************************************************************************\
 * Function: DetermineMode
 * Purpose : Determine the decoder acquisition mode. New mode dependes on the
 *           current acquisition mode and new page_state
 * Return  : New acquisition mode.
\******************************************************************************/

static __inline 
STSUBT_AcquisitionMode_t DetermineMode (STSUBT_InternalHandle_t *DecoderHandle,
                                        STSUBT_AcquisitionMode_t current_mode,
                                        U8 page_state)
{
  STSUBT_AcquisitionMode_t new_mode;

  switch (current_mode) {
    case STSUBT_Starting:
      switch (page_state) {
        case 0:                         /* normal_case */
             new_mode = STSUBT_Starting;
             break;
        case 1:  			/* acquisition_point */
        case 2:  			/* mode_change */
             new_mode = STSUBT_Acquisition;
             STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** pcs_processor: decoder acquiring service **\n"));
             break;
      }
      break;
    case STSUBT_Acquisition:
    case STSUBT_Normal_Case:
      switch (page_state) {
        case 0:  			/* normal_case */
             new_mode = STSUBT_Normal_Case;
             break;
        case 1:  		       /* acquisition_point */
            new_mode = STSUBT_Acquisition;
             break;
        case 2:  			/* mode_change */
             new_mode = STSUBT_Acquisition;
             break;
      }
      break;
    default:
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** pcs_processor: invalid page state **\n"));
      new_mode = STSUBT_Starting;
      break;
  }

  return (new_mode);
}


/******************************************************************************\
 * Function: reset_decoder_status
 * Purpose : reset the status of decoder
 * Description: 
 *           reset Composition Buffer and Pixel Buffer
 *           reset the epoch counter.
\******************************************************************************/
 
static __inline
void reset_decoder_status (STSUBT_InternalHandle_t *DecoderHandle)
{
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** pcs_processor: doing an EPOCH reset **\n"));
  
  if(DecoderHandle->EpochNumber%2)
  {
     CompBuffer_EpochReset(DecoderHandle->CompositionBuffer1);
     PixelBuffer_Reset(DecoderHandle->PixelBuffer1);  
     DecoderHandle->CompositionBuffer=DecoderHandle->CompositionBuffer1;
     DecoderHandle->PixelBuffer=DecoderHandle->PixelBuffer1;
  }
  else
  {
     CompBuffer_EpochReset(DecoderHandle->CompositionBuffer2);
     PixelBuffer_Reset(DecoderHandle->PixelBuffer2);  
     DecoderHandle->CompositionBuffer=DecoderHandle->CompositionBuffer2;
     DecoderHandle->PixelBuffer=DecoderHandle->PixelBuffer2;
  }
  DecoderHandle->EpochNumber++;
  DecoderHandle->ReferredObjectInCache = NULL;
  DecoderHandle->PcsNumber =0;
}

/******************************************************************************\
 * Function: display_set_reset
 * Purpose : do a display set reset
\******************************************************************************/

static __inline
void display_set_reset (STSUBT_InternalHandle_t *DecoderHandle)
{
  CompBuffer_DisplaySetReset(DecoderHandle->CompositionBuffer);

  DecoderHandle->ReferredObjectInCache = NULL;
}


/******************************************************************************\
 * Function: pcs_processor
 *
 * Purpose : Process a single pcs segment
 *
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t pcs_processor (STSUBT_InternalHandle_t *DecoderHandle,
                              Segment_t *PCS_Segment)
{
  ST_ErrorCode_t res;
  clock_t presentation_time;
  clock_t display_timeout;
  U8      page_time_out;
  U8	  page_state;
  U16     page_id;
  U16     segment_length;
  U8      page_version_number;
  U8      temp_byte;


  STSUBT_AcquisitionMode_t PreviousAcquisitionMode;
  STSUBT_AcquisitionMode_t NewAcquisitionMode;
  STSUBT_AcquisitionMode_t NotifyingMode;



  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n** pcs_processor: processing PCS **\n"));

  /* --- extract page_time_out and page_state from PCS segment --- */

  skip_bytes(PCS_Segment, 2);

  page_id = ((U16)get_byte(PCS_Segment) << 8) + get_byte(PCS_Segment);

  segment_length = ((U16)get_byte(PCS_Segment) << 8) + get_byte(PCS_Segment);

  page_time_out = get_byte(PCS_Segment);
  if (page_time_out == 0)
  {
      page_time_out = 5; 
  }

  temp_byte = get_byte(PCS_Segment);

  page_version_number = (temp_byte & 0xf0) >> 4;

  page_state = (temp_byte & 0x0c) >> 2;


  /* --- check segment length --- */

  if ((segment_length + 6) != PCS_Segment->Size) 
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: bad segment **\n"));
      if (STEVT_Notify(
          DecoderHandle->stevt_handle,
          DecoderHandle->stevt_event_table[STSUBT_EVENT_BAD_SEGMENT & 0x0000FFFF],
          DecoderHandle))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: Error notifying event**\n"));
      } 

      return (STSUBT_ERROR_BAD_SEGMENT);
  }

      
  /* --- determine previous acquisition mode --- */

  PreviousAcquisitionMode = DecoderHandle->AcquisitionMode;

  /* --- update acquisition mode --- */

  NewAcquisitionMode = DetermineMode (DecoderHandle, 
                                      PreviousAcquisitionMode,
                                      page_state);

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: page_id = %d **\n", page_id));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: page_time_out = %d **\n", page_time_out));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: page_version = %d **\n", page_version_number));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: segment_length = %d **\n",segment_length));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: page_state = %d **\n", page_state));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: old mode = %d **\n",PreviousAcquisitionMode));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: new mode = %d **\n", NewAcquisitionMode));

  /* resulting acquisition mode can be either Starting, Acquisition or 
   * Normal_Case. In case of starting, do nothing.
   */


  if (NewAcquisitionMode == STSUBT_Starting) 
  {
      DecoderHandle->EpochNumber = 0;
      return (ST_NO_ERROR);
  }

  DecoderHandle->ObjectNumber=0;

  /* --- register new acquisition mode --- */

  DecoderHandle->AcquisitionMode = NewAcquisitionMode;


  /* --------------------------------------------- */
  /* --- Process pending planning PCS (if any) --- */
  /* --------------------------------------------- */

  /* If previous mode was acquisition, a planning PCS of
   * mode change, which contains as visible region
   * list all regions planned for current epoch, is sent.
   *
   * Note: when in DTG standard, if an EDS segment was found
   * then planning was already done by EDS processor. In that case
   * the acquisition point was updated to normal case.
   */

  NotifyingMode = NewAcquisitionMode;

  if (DecoderHandle->WorkingMode == STSUBT_NormalMode) 
  {
      if (PreviousAcquisitionMode == STSUBT_Acquisition)
      {  
          STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** pcs_processor: Building planning PCS **\n"));
    
          /* --- Set epoch region list in the PCS --- */
       
         SetEpochRegionList(DecoderHandle);
    
          if (DecoderHandle->SubtitleStandard == STSUBT_DTGStandard)
          {
              /* --- notify planning pcs to Display Timer --- */
    
              res = Notify_PCS (DecoderHandle);
              if (res != ST_NO_ERROR)
              {
                  STOP_PROCESSING_IF_REQUIRED(DecoderHandle);
                  STTBX_Report(
                  (STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: Error: Notify_PCS (Mode_Change) **\n"));
                  return (STSUBT_ERROR_PROCESSING_SEGMENT);
              }
              STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n** pcs_processor: PCS (Mode_Change) notified **\n"));
          }
          else              
          {              
              NotifyingMode = STSUBT_Acquisition;
          }
      }
  }

  /* ---------------------------------- */
  /* --- Process PCS of Mode Change --- */
  /* ---------------------------------- */

  if (NewAcquisitionMode == STSUBT_Acquisition)
  {
      /* A complete description of the memory use is being
       * broadcast; decoder prepares to acquire the service.
       *
       * Note: PCS of mode change is sent at next EDS or PCS.
       */

      STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** pcs_processor: Building PCS of STSUBT_Acquisition **\n"));

      /* --- reset the decoder status --- */


      reset_decoder_status (DecoderHandle);
      
      /* --- set (basic) PCS data structure --- */
      
      DecoderHandle->PCS.page_id = page_id;
      DecoderHandle->PCS.presentation_time = 0;
      DecoderHandle->PCS.page_time_out = page_time_out;
      DecoderHandle->PCS.acquisition_mode = STSUBT_Acquisition;
      DecoderHandle->PCS.page_version_number = page_version_number;
      DecoderHandle->PCS.VisibleRegion_p = NULL;
      DecoderHandle->PCS.EpochRegionList = NULL;
      DecoderHandle->PCS.EpochNumber = DecoderHandle->EpochNumber;
      DecoderHandle->PCS.CompositionBuffer_p = DecoderHandle->CompositionBuffer;  

      if ((DecoderHandle->SubtitleStandard == STSUBT_DTGStandard)
      ||  (DecoderHandle->WorkingMode == STSUBT_OverlayMode)
      || (DecoderHandle->SubtitleStandard == STSUBT_DVBStandard))
      {
              
          /* --- Compute presentation time and page timeout --- */
       
          if (get_sync_time (DecoderHandle, page_time_out, &presentation_time, &display_timeout)
              != ST_NO_ERROR)
          {
              STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: Error: getting sync times **\n"));
              return (STSUBT_ERROR_PROCESSING_SEGMENT);
          }
              
          DecoderHandle->PCS.presentation_time = presentation_time;
          DecoderHandle->PCS.page_time_out = display_timeout;
                    
          /* --- get/create the list of visible regions into this display --- */              
          CreateVisibleRegionList (DecoderHandle, PCS_Segment,
                                   segment_length, NewAcquisitionMode);
      }

      return (ST_NO_ERROR);
  }



  /* ---------------------------------- */
  /* --- Process PCS of Normal case --- */
  /* ---------------------------------- */

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** pcs_processor: Process PCS of Normal case **\n"));

  /* --- do a display set reset --- */

  display_set_reset(DecoderHandle);

  /* --- check segment data version --- */
  if (newer_version (DecoderHandle->PCS.page_version_number,
                     page_version_number) != 1)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: Obsolete Version %d %d**\n",DecoderHandle->PCS.page_version_number,page_version_number));
      return (ST_NO_ERROR);
  }

  /* --- Compute presentation time and page timeout --- */
 
  if (get_sync_time (DecoderHandle,
                     page_time_out,
                     &presentation_time,
                     &display_timeout) != ST_NO_ERROR)
  {
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: Error: getting sync time **\n"));
      return (STSUBT_ERROR_PROCESSING_SEGMENT);
  }

  /* --- set (basic) PCS data structure --- */

  DecoderHandle->PcsNumber++;

  DecoderHandle->PCS.page_id = page_id;
  DecoderHandle->PCS.presentation_time = presentation_time;
  DecoderHandle->PCS.page_time_out = display_timeout;
  DecoderHandle->PCS.acquisition_mode = NotifyingMode;
  DecoderHandle->PCS.page_version_number = page_version_number;
  DecoderHandle->PCS.VisibleRegion_p = NULL;
  DecoderHandle->PCS.EpochNumber = DecoderHandle->EpochNumber;

  /* --- get/create the list of visible regions into this display   --- */
  CreateVisibleRegionList (DecoderHandle, PCS_Segment, 
                           segment_length, STSUBT_Normal_Case);

  STOP_PROCESSING_IF_REQUIRED(DecoderHandle);

  /* ---------------------------- */
  /* --- notify processed PCS --- */
  /* ---------------------------- */
  res = Notify_PCS (DecoderHandle);
  if (res != ST_NO_ERROR) 
  {
      STOP_PROCESSING_IF_REQUIRED(DecoderHandle);
      STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** pcs_processor: Error : Notify_PCS (Normal Case)**\n"));
      return (STSUBT_ERROR_PROCESSING_SEGMENT);
  }
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pcs_processor: PCS of Normal Case notified **\n"));

  return (ST_NO_ERROR);
}



