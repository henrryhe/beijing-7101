/******************************************************************************\
 *
 * File Name   : eds.c
 *
 * Description : 
 *     Contains implementation of EDS Processor.  It processes a single 
 *     End of Display-Set Segment (EDS). 
 *
 *     This segment type has been introduced by the Digital Television Group.
 *     It  represents the last segemnt in a display set. 
 *
 *     When a EDS segment is detected the decoder switch to STSUBT_DTG_Standard
 *     mode (only in auto detection mode, otherwise an error is returned).
 *
 * Author : Marino Congiu - Apr 1999
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
 
#include <stddefs.h>
#include <stevt.h>
 
#include "subtdev.h"
#include "decoder.h"

/******************************************************************************\
 * Function: Notify_PCS
 * Purpose : Notify processed pcs to Display Engine.
 *           That is obviously useless when driver is not displaying.
\******************************************************************************/

static __inline
ST_ErrorCode_t Notify_PCS (STSUBT_InternalHandle_t *DecoderHandle)
{
  ST_ErrorCode_t res = ST_NO_ERROR;
  STSUBT_DisplayItem_t DisplayItem;

  if (DecoderHandle->SubtitleDisplayEnabled == TRUE)
  {
       PCS_t PCS = DecoderHandle->PCS;

       DisplayItem.PCS = PCS;
       DisplayItem.display_descriptor = NULL;
       DisplayItem.presentation_time = PCS.presentation_time;
       DisplayItem.display_timeout = PCS.page_time_out;
       DisplayItem.EpochNumber = DecoderHandle->EpochNumber;
       DisplayItem.PcsNumber = DecoderHandle->PcsNumber;
       res = fcBuffer_Insert (DecoderHandle->DisplayBuffer,
                              (void*)&DisplayItem);
  }
  return (res);
}


/******************************************************************************\
 * Function: eds_processor
 *
 * Purpose : Process a single eds segment
 *
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t eds_processor (STSUBT_InternalHandle_t *DecoderHandle, 
                              Segment_t *Segment)
{
  ST_ErrorCode_t res;

  STSUBT_AcquisitionMode_t AcquisitionMode;

  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n** eds_processor: processing EDS **\n"));

  /* --- determine acquisition mode --- */
 
  AcquisitionMode = DecoderHandle->AcquisitionMode;

  /* --- check/set standard --- */

  /* if we are in auto standard detection mode then we switch to DTG.
   * Otherwise, if DVB standard is set, an error is returned (not a DVB segemnt)
   */

  if (DecoderHandle->SubtitleStandard == STSUBT_DVBStandard)
  {
      if (STEVT_Notify(
                DecoderHandle->stevt_handle,
                DecoderHandle->stevt_event_table[STSUBT_EVENT_DTG_STANDARD & 0x0000FFFF],
                DecoderHandle))
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** eds_processor: Error notifying event**\n"));
      }

      if (DecoderHandle->AutoDetection == FALSE)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** eds_processor: DVB standard set and EDS detected\n"));
          return (STSUBT_ERROR_PROCESSING_SEGMENT);
      }
      else 
          DecoderHandle->SubtitleStandard = STSUBT_DTGStandard;
  }

  /* in case AcquisitionMode is starting, noop */

  if (AcquisitionMode == STSUBT_Starting) 
      return (ST_NO_ERROR);

  /* In case of Mode Change, pack a "fake" pcs structure whose VisibleRegion_p 
   * field contains the list of all regions will be used in the current epoch.
   * That will be used by Display Service to plan the current epoch.
   *
   * Epoch planning may happen in 2 moments:
   * either when a EDS segment is found or when next PCS of normal case
   * arrives. In the first case, after planning the decoder acquisition mode
   * is set to Normal case so that at next PCS the pcs processor will
   * not try to plan the epoch again.
   */

  if (AcquisitionMode == STSUBT_Acquisition )
  {
      /* --- Build a planning PCS --- */
 
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** eds_processor: Building planning PCS **\n"));

      SetEpochRegionList(DecoderHandle);
 
      /* --- notify pcs to Display Engine --- */
     
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** eds_processor: notifying PCS **\n"));

      res = Notify_PCS (DecoderHandle); 

      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** eds_processor: Error: Notify_PCS **\n"));
          return (STSUBT_ERROR_PROCESSING_SEGMENT);
      }
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** eds_processor: PCS notified **\n"));

      /* set decoder acquisition mode  to normal case */

      DecoderHandle->AcquisitionMode = STSUBT_Normal_Case;
  }


  if (AcquisitionMode == STSUBT_Change_Case )
  {
 
      /* --- notify pcs to Display Engine --- */
     
      STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** eds_processor: notifying PCS **\n"));

      res = Notify_PCS (DecoderHandle); 

      if (res != ST_NO_ERROR)
      {
          STTBX_Report((STTBX_REPORT_LEVEL_ERROR,"** eds_processor: Error: Notify_PCS **\n"));
          return (STSUBT_ERROR_PROCESSING_SEGMENT);
      }
      STTBX_Report((STTBX_REPORT_LEVEL_INFO,"** eds_processor: PCS notified **\n"));

      /* set decoder acquisition mode  to normal case */

      DecoderHandle->AcquisitionMode = STSUBT_Normal_Case;
  }

  return (ST_NO_ERROR);
}

