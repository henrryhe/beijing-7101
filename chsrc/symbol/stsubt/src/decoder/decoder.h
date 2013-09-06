/******************************************************************************\
 *
 * File Name   : decoder.h
 *
 * Description : Exported types and function for segment processing
 *
 * Author : Marino Congiu - Jan 1998
 *
 * Copyright 1999 STMicroelectronics. All Rights Reserved.
 *
\******************************************************************************/
 
#ifndef __DECODER_H
#define __DECODER_H
 
#include <stddefs.h>

#include <subtdev.h>

#define STSUBT_ERROR_PROCESSING_SEGMENT		1

/* --------------------------------------- */
/* --- Control functions for Processor --- */
/* --------------------------------------- */

ST_ErrorCode_t Processor_Create (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Processor_Delete (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Processor_Start  (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Processor_Stop   (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Processor_Pause  (STSUBT_InternalHandle_t *);
ST_ErrorCode_t Processor_Resume (STSUBT_InternalHandle_t *);
 
/* ------------------------ */
/* --- Parseing Support --- */
/* ------------------------ */

typedef struct {
  U8  *Data;
  U16  Size;
} Segment_t;

/******************************************************************************\
 * Macro: skip_bytes
 * Purpose : skip nbyte of data from segment in Coded Data Buffer
\******************************************************************************/

#define skip_bytes(Segment,nbytes) (Segment->Data += nbytes)


/******************************************************************************\
 * Macro: get_byte
 * Purpose : get a byte of segment data in Coded Data Buffer
\******************************************************************************/

#define get_byte(Segment) (*Segment->Data++)


/* -------------------------------------- */
/* --- Segment Processors definitions --- */
/* -------------------------------------- */

extern ST_ErrorCode_t pts_processor(STSUBT_InternalHandle_t *, Segment_t *);
extern ST_ErrorCode_t pcs_processor(STSUBT_InternalHandle_t *, Segment_t *);
extern ST_ErrorCode_t rcs_processor(STSUBT_InternalHandle_t *, Segment_t *);
extern ST_ErrorCode_t clut_processor(STSUBT_InternalHandle_t *, Segment_t *);
extern ST_ErrorCode_t object_processor(STSUBT_InternalHandle_t *, Segment_t *);
extern ST_ErrorCode_t eds_processor(STSUBT_InternalHandle_t *, Segment_t *);

/* ------------------------------- */
/* --- support decoding macros --- */
/* ------------------------------- */

/******************************************************************************\
 * Macro   : STOP_PROCESSING_IF_REQUIRED
 * Purpose : Cause a segment processor stop processing and to return
\******************************************************************************/
 
#define STOP_PROCESSING_IF_REQUIRED(handle)              \
    if (handle->ProcessorStatus == STSUBT_TaskStopped)   \
    {                                                    \
        return (ST_NO_ERROR);                            \
    }


/******************************************************************************\
 * Macro   : newer_version
 * Purpose : macro to compare two segment versions
 *           it is valued TRUE if v2 is newer that v1
\******************************************************************************/
 
#ifdef BEFORE_V_MBFIX_3_0_7
#define newer_version(v1,v2) ( ((v1 <  8) && (v1 < v2) && (v2 <= (v1 + 8))) ||\
                               ((v1 >= 8) && ((v1 < v2) || ((v2 + 8) <= v1))) )
#else

#define newer_version(v1,v2) ((v1) != (v2) ? TRUE : FALSE )
                            
#endif
 

/******************************************************************************\
 * Function: SetEpochRegionList
 * Purpose : Set ep[och region list in PCS
\******************************************************************************/

#define SetEpochRegionList(DecoderHandle) 			\
  (DecoderHandle->PCS).EpochRegionList =			\
      (DecoderHandle->CompositionBuffer)->EpochRegionList_p;

#endif  /* #ifndef __DECODER_H */

