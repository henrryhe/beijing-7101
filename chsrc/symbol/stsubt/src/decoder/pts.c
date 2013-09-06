/******************************************************************************\
 *
 * File Name   : pts.c
 *
 * Description : Process a single pts segment
 *
 *               See SDD document for more details.
 *
 * Author : Marino Congiu - Jan 1998
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
 
#include <stddefs.h>
#include <subtdev.h>
 
#include <compbuf.h>	/* for pts_t */

#include "decoder.h"


/******************************************************************************\
 * Function: pts_processor
 *
 * Purpose : Process a single pts segment
 *
 * Description: 
 *           PTS segment contains the value of candidate PTSs that have to be
 *           associated with the corresponding PCS. When a PTS segment arrives,
 *           the Subtitle Processor registers it as the last one.
 *
 * Return  : ST_NO_ERROR on success,
\******************************************************************************/
 
ST_ErrorCode_t pts_processor (STSUBT_InternalHandle_t *DecoderHandle, 
                              Segment_t *Segment)
{
  pts_t pts;
 
  STTBX_Report((STTBX_REPORT_LEVEL_INFO,"\n** pts_processor: processing PTS **\n"));

  skip_bytes(Segment, 6);

  pts.high = ((U32)get_byte(Segment) << 24)
           + ((U32)get_byte(Segment) << 16)
           + ((U32)get_byte(Segment) <<  8)
           + get_byte(Segment);

  pts.low  = ((U32)get_byte(Segment) << 24)
           + ((U32)get_byte(Segment) << 16)
           + ((U32)get_byte(Segment) <<  8)
           + get_byte(Segment);

  /* register new pts */

  DecoderHandle->LastPTS = pts;

  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pts_processor: pts.high = %.8x **\n", pts.high));
  STTBX_Report((STTBX_REPORT_LEVEL_USER1,"** pts_processor: pts.low  = %.8x **\n", pts.low ));

  return (ST_NO_ERROR);
}

