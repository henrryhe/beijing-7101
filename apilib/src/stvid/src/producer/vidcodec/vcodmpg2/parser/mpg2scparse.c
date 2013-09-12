/*****************************************************************************
                           Explorer System Software

                   Copyright 2004 Scientific-Atlanta, Inc.

                           Subscriber Engineering
                           5030 Sugarloaf Parkway
                               P.O.Box 465447
                          Lawrenceville, GA 30042

                        Proprietary and Confidential
              Unauthorized distribution or copying is prohibited
                            All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Scientific Atlanta, Inc.

*****************************************************************************

 File Name:    mpg2scparse.cpp

 See Also:

 Project:      Zeus ASIC

 Compiler:     ST40

 Author:       John Bean

 Description:

 Documents:

 Notes:

*****************************************************************************
 History:
 Rev Level    Date    Name    ECN#    Description
 -----------------------------------------------------------------------------
 1.00       11/02/04  JRB     ---     Initial Creation from SCGen
*****************************************************************************/

/* ====== includes ======*/

#if !defined ST_OSLINUX
/*#include <malloc.h>*/
#include <stdio.h>
#endif

#include "mpg2parser.h"
#include "mpg2getbits.h"

/* ====== file info ======*/


/* ====== defines ======*/


/* ====== enums ======*/

/* ====== globals ======*/

/* ====== statics ======*/


#define MME_SCANORDER 1
#ifdef MME_SCANORDER

/* zig-zag and alternate scan patterns for loading matrices*/

const U8 gES_scan[2][64] =
{
   {  /*Zig-Zag scan pattern */
      0,1,8,16,9,2,3,10,17,24,32,25,18,11,4,5,
      12,19,26,33,40,48,41,34,27,20,13,6,7,14,21,28,
      35,42,49,56,57,50,43,36,29,22,15,23,30,37,44,51,
      58,59,52,45,38,31,39,46,53,60,61,54,47,55,62,63
   },
   {  /*Alternate scan pattern*/
      0,8,16,24,1,9,2,10,17,25,32,40,48,56,57,49,
      41,33,26,18,3,11,4,12,19,27,34,42,50,58,35,43,
      51,59,20,28,5,13,6,14,21,29,36,44,52,60,37,45,
      53,61,22,30,7,15,23,31,38,46,54,62,39,47,55,63
   }
};

/* default intra quantization matrix */
const U8 gES_DefaultIntraQM[64] =
{
   8, 16, 19, 22, 26, 27, 29, 34,
   16, 16, 22, 24, 27, 29, 34, 37,
   19, 22, 26, 27, 29, 34, 34, 38,
   22, 22, 26, 27, 29, 34, 37, 40,
   22, 26, 27, 29, 32, 35, 40, 48,
   26, 27, 29, 32, 35, 40, 48, 58,
   26, 27, 29, 34, 38, 46, 56, 69,
   27, 29, 35, 38, 46, 56, 69, 83
};

#else
const char gES_DefaultIntraQM[] = {
                8, 16, 16, 19, 16, 19, 22, 22,
               22, 22, 22, 22, 26, 24, 26, 27,
               27, 27, 26, 26, 26, 26, 27, 27,
               27, 29, 29, 29, 34, 34, 34, 29,
               29, 29, 27, 27, 29, 29, 32, 32,
               34, 34, 37, 38, 37, 35, 35, 34,
               35, 38, 38, 40, 40, 40, 48, 48,
               46, 46, 56, 56, 58, 69, 69, 83
};
#endif


/*====== prototypes ======*/

void es_processStartCode ( U32 sc, tESState *pESState);
void es_ProcessMPEG1SeqHdr(tESState *pESState);
void es_ProcessPictureHeader (tESState *pESState);
void es_ProcessUserDataStart (tESState *pESState);
void es_ProcessSequenceHeader (tESState *pESState);
void es_ProcessExtensionStart (tESState *pESState);
void es_ProcessQuantMatrix (tESState *pESState, BOOL Sequence_H);
void es_ProcessGOPStart (tESState *pESState);
void es_ProcessDiscontinuityStart (tESState *pESState);
U32  es_SkipToNextState (U32 current_state, U32 start_code);



#ifdef DEBUG_STARTCODE
/*******************************************************************************
 * Name        : ???
 * Description : ???
 * Parameters  : ???
 * Assumptions : ???
 * Limitations : ???
 * Returns     : ???
 * *******************************************************************************/
void FlushSCBuffer()
{
  FILE* DumpFile_p = NULL;
  U32 i;
    DumpFile_p = fopen(TRACES_FILE_NAME, "w");
    if (DumpFile_p == NULL)
    {
        printf("*** fopen error on file '%s' ***\n", TRACES_FILE_NAME);
        return;
    }

    for (i = 0; i <=indSC; i++)
    {
      fprintf(DumpFile_p, "%s", &tracesSC[i][0]);
    }

    for (i = 0; i <=indAdr; i++)
    {
      fprintf(DumpFile_p, "%d\n", SCListAddress[i]);
    }
    fclose(DumpFile_p);

}
#endif /* DEBUG_STARTCODE */

/*****************************************************************************
Function:    mpg2par_InitStartCode

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Initializes State for sC Parsing

Behavior:

Exceptions:  None

*****************************************************************************/
ST_ErrorCode_t mpg2par_InitStartCode(const PARSER_Handle_t ParserHandle)
{
   PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
   MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;

   PARSER_Data_p->SCParserState = ES_state_inactive;

   return (ST_NO_ERROR);
}


/*****************************************************************************
Function:    mpg2par_ProcessStartCode

Parameters:  StartCode
             StartCodeValueAddress
             NextPictureStartCodeValueAddress
             ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Processes a start code

Behavior:

Exceptions:  None

*****************************************************************************/
ST_ErrorCode_t mpg2par_ProcessStartCode(U8 StartCode, U8 * StartAddress_p, U8 * StopAddress_p, const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t       * PARSER_Properties_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t   * PARSER_Data_p = (MPG2ParserPrivateData_t *) PARSER_Properties_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParsingJobResults_p = &PARSER_Data_p->ParserJobResults;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParsingJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    tESState                    ESState;
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;
    void*                                    ES_TOP=NULL;
    void*                                    ES_BASE=NULL;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;

    /* Process the start code. Fill in the PJR structure as possible*/

    ESState.parse_state  = PARSER_Data_p->SCParserState;
    ESState.gbHandle     = PARSER_Data_p->gbHandle;
    ESState.ParserJobResults_p          = &PARSER_Data_p->ParserJobResults;
    ESState.UserData     = &PARSER_Data_p->UserData;
    ESState.UserDataSize = PARSER_Data_p->UserDataSize;
    ESState.error_mask   = 0;
    ESState.error_count  = 0; /* TODO OD: error management */
    ESState.parse_error  = 0;

/*
#ifdef DEBUG_STARTCODE
   if (indSC >= MAX_TRACES_SC)
   {
      indSC = 0;
   }
   sprintf (&tracesSC[indSC][0], "State=0x%02x sc=0x%02x adr=0x%08x\n", ESState.parse_state, StartCode, StartAddress_p);
   indSC++;
#endif
*/

   /* Initialize GetBits for this buffer. The buffer starts with the byte following the startcode*/
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );
    ES_TOP = STAVMEM_DeviceToCPU((U32)PARSER_Data_p->BitBuffer.ES_Stop_p, &VirtualMapping);
    ES_BASE =  STAVMEM_DeviceToCPU((U32)PARSER_Data_p->BitBuffer.ES_Start_p, &VirtualMapping);

    if (StopAddress_p < StartAddress_p)
    {
        if((StartCode>1)||(StartCode>1))
        {
            ESState.error_mask   = 0;
        }
        gb_InitBuffer (ESState.gbHandle, StartAddress_p + 1,
                    (((U32)StopAddress_p - (U32)ES_BASE) +  ((U32)ES_TOP - (U32)StartAddress_p +1)  - 1),ES_TOP, ES_BASE);
    }
    else
    {
        gb_InitBuffer (ESState.gbHandle, StartAddress_p + 1,
                    ((U32)StopAddress_p - (U32)StartAddress_p - 1), ES_TOP, ES_BASE);
    }


    es_processStartCode (StartCode, &ESState);

    if (GlobalDecodingContextSpecificData_p->DiscontinuityStartCodeDetected)
    {
        PictureSpecificData_p->PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
        PictureSpecificData_p->LastReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
        /* In backward, we don't need to check discontinuity. Discontinuities are from one buffer to the other. */
        PARSER_Data_p->DiscontinuityDetected = TRUE;
#ifdef ST_speed
#ifdef STVID_TRICKMODE_BACKWARD
        if (!PARSER_Data_p->Backward)
#endif
        {
            STEVT_Notify(PARSER_Properties_p->EventsHandle, PARSER_Data_p->RegisteredEventsID[PARSER_FIND_DISCONTINUITY_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST NULL);
        }
#endif /* ST_speed */
        GlobalDecodingContextSpecificData_p->DiscontinuityStartCodeDetected = FALSE;
    }

/*    Maintain state across calls to startcode parser*/
    PARSER_Data_p->SCParserState = ESState.parse_state;

   /*Handle Parsing Errors*/

#ifdef DEBUG_STARTCODE
    if (indSC >= MAX_TRACES_SC)
    {
        FlushSCBuffer();
        exit(0);
    }
#endif

    if (ESState.error_mask != 0)
    {
/*       TODO: Handle Parser Errors*/
/*      printf ("ES: Error: error_mask = 0x%02x  StartCode = 0x%08x\n", ESState.error_mask, StartCode);*/
#ifdef DEBUG_STARTCODE
    if (indSC >= MAX_TRACES_SC)
    {
        indSC = 0;
    }
    sprintf (&tracesSC[indSC][0], "ES: Error: error_mask = 0x%02x  StartCode = 0x%08x\n", ESState.error_mask, StartCode);
    indSC++;
    FlushSCBuffer();
    exit(0);
#endif

        if ((ESState.error_mask & ES_error_unexpected_startcode) ||
            (ESState.error_mask & ES_error_aspect_ratio) ||
            (ESState.error_mask & ES_error_frame_rate) ||
            (ESState.error_mask & ES_error_missing_2nd_field))
        {
            /* Error_Recovery_Case_n01 : The frame rate of the sequence is not supported.                           */
            /* Error_Recovery_Case_n02 : The dimensions for the sequence don't correspond to the MPEG Profile.      */
            /* Error_Recovery_Case_n04 : Missing expected 2nd field.                                                */
            PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
        }

        if ((ESState.error_mask & ES_error_invalid_frame_type) ||
            (ESState.error_mask & ES_error_marker_bit) ||
            (ESState.error_mask & ES_error_constrained_parm_flag) ||
            (ESState.error_mask & ES_error_unexpected_chroma_mtx))
        {
    /*         Picture level error, so indicate the error in the PGD structure*/
            PARSER_Data_p->ParserJobResults.ParsedPictureInformation_p->PictureGenericData.ParsingError = PARSER_ERROR_STREAM_SYNTAX;
        }
        PARSER_Data_p->ParserJobResults.ParsingJobStatus.HasErrors = TRUE;
        PARSER_Data_p->ParserJobResults.ParsingJobStatus.ErrorCode = PARSER_ERROR_STREAM_SYNTAX;
    }

   return (ST_NO_ERROR);
}



/*****************************************************************************
Function:    es_SkipToNextState

Parameters:  current_state - the current state
             start_code - the unexpected startcode

Returns:     The next state

Scope:       Local to the es parsing code

Purpose:     Chooses a next state when an unexpected startcode is
             encountered in the stream.

Behavior:    The general behavior is that the state associated
             with the startcode is transitioned to, independent
             of the current state. I am putting this all in a
             single function so that the behavior can be easily
             tweaked as the requirements firm.


Exceptions:  None

*****************************************************************************/
U32 es_SkipToNextState (U32 current_state, U32 start_code)
{
   U32 next_state;

   switch (start_code)
   {
      case(PICTURE_START_CODE):
         next_state = ES_state_picture_header;
         break;

      case(USER_DATA_START_CODE):
      case(EXTENSION_START_CODE):
         next_state = (current_state < ES_state_ext0_user0) ? ES_state_ext0_user0 : ES_state_ext2_user2;
         break;

      case(SEQUENCE_HEADER_CODE):
         next_state = ES_state_sequence_header;
         break;

      case(SEQUENCE_END_CODE):
         next_state = ES_state_sequence_end;
         break;

      case(GROUP_START_CODE):
         next_state = ES_state_gop_header;
         break;

      case(RESERVED_START_CODE_0):
      case(RESERVED_START_CODE_1):
      case(RESERVED_START_CODE_6):
      case(SEQUENCE_ERROR_CODE):
      default:
         if ((start_code >= FIRST_SLICE_START_CODE) &&
             (start_code <= GREATEST_SLICE_START_CODE))
         {
            next_state =  ES_state_picture_data;
         }
         else
         {
            next_state = ES_state_no_transition;
         }
   }

   return (next_state);
}


/*****************************************************************************
Function:    es_processStartCode

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:
             The state machine is derived from the one included in the
             mpeg-2 spec (13818-2, fig 6-15). The state is maintained for
             each stream across the PES packet boundary.

             Note that right now, the code is written to assume:
             1) Startcodes will not be split across PES packets
             2) Syntactic elements will not be split across PES packets

Exceptions:  None

*****************************************************************************/
void es_processStartCode ( U32 sc, tESState *pESState)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    U32 sc_ext_id;
    U32  nextState = ES_state_inactive;

    if (sc == EXTENSION_START_CODE)
    {
/*      For Sequence Extension Startcodes, get the Extension ID*/
        sc_ext_id = gb_Lookahead(pESState->gbHandle, 0) >> 4;
    }
    else
    {
        sc_ext_id = 0;
    }



   /* Handle the Stream Syntax*/

   /* Based on the current state, and the startcode that has been received,*/
   /* determine the next state. Also, detect syntax errors.*/


   switch (pESState->parse_state)
   {

   case   ES_state_inactive:

      /* Start State machine at first sequence header*/
      if (sc == SEQUENCE_HEADER_CODE)
      {
         nextState = ES_state_sequence_header;
      }

      break;

   case   ES_state_sequence_header:

      /*Looking for extension or group startcodes*/
      if ((sc == EXTENSION_START_CODE) &&
          (sc_ext_id == ES_EXT_SEQUENCE))
      {
         /*MPEG2 stream*/
         nextState = ES_state_sequence_extension;
      }
      else if (sc == USER_DATA_START_CODE)
      {
         /* MPEG1 stream - complete Seq Hdr processing before continuing*/
         nextState = ES_state_ext0_user0;
         es_ProcessMPEG1SeqHdr(pESState);
      }
      else if (sc == GROUP_START_CODE)
      {
        /* MPEG1 stream - complete Seq Hdr processing before continuing*/
         nextState = ES_state_gop_header;
         es_ProcessMPEG1SeqHdr(pESState);
      }
      else
      {
         /*Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }
      break;

   case   ES_state_sequence_extension:

      /* Looking for GOP header, extension or user data*/
      if (sc == GROUP_START_CODE)
      {
         nextState = ES_state_gop_header;
      }
      else if ((sc == USER_DATA_START_CODE) ||
               ((sc == EXTENSION_START_CODE) &&
                 ((sc_ext_id == ES_EXT_SEQ_DISPLAY) ||
                  (sc_ext_id == ES_EXT_SEQ_SCALABLE))))
      {
         nextState = ES_state_ext0_user0;
      }
      else if (sc == PICTURE_START_CODE)
      {
      	nextState = ES_state_picture_header;
      }
      else
      {
        /* Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }

      break;

   case   ES_state_ext0_user0:

      /*Looking for GOP header, extension, user data or picture hdr*/
      if (sc == GROUP_START_CODE)
      {
         nextState = ES_state_gop_header;
      }
      else if ((sc == USER_DATA_START_CODE) ||
               ((sc == EXTENSION_START_CODE) &&
                ((sc_ext_id == ES_EXT_SEQ_DISPLAY) ||
                 (sc_ext_id == ES_EXT_SEQ_SCALABLE))))
      {
         nextState = ES_state_ext0_user0;
      }
      else if (sc == PICTURE_START_CODE)
      {
         nextState = ES_state_picture_header;
      }
      else
      {
         /*Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }

      break;

   case   ES_state_gop_header:

        /*Looking for user data or picture hdr*/
        if (sc == USER_DATA_START_CODE)
        {
            nextState = ES_state_user1;
        }
        else if (sc == PICTURE_START_CODE)
        {
            nextState = ES_state_picture_header;
        }
        else
        {
            /*Unexpected sc. Indicate and transition directly to the new state.*/
            pESState->error_mask |= ES_error_unexpected_startcode;
            nextState = es_SkipToNextState (pESState->parse_state, sc);
        }

        break;

   case   ES_state_user1:

        /*Looking for user data or picture hdr*/
        if (sc == USER_DATA_START_CODE)
        {
            nextState = ES_state_no_transition;
        }
        else if (sc == PICTURE_START_CODE)
        {
            nextState = ES_state_picture_header;
        }
        else
        {
            /*Unexpected sc. Indicate and transition directly to the new state.*/
            pESState->error_mask |= ES_error_unexpected_startcode;
            nextState = es_SkipToNextState (pESState->parse_state, sc);
        }
        break;

   case   ES_state_picture_header:

      /*Looking for Picture Coding Ext or Ext2*/
      if ((sc == EXTENSION_START_CODE) &&
          (sc_ext_id == ES_EXT_PICTURE_CODING))
      {
         /*MPEG2 stream*/
         nextState = ES_state_picture_coding_ext;
      }
      else if (sc == PICTURE_START_CODE)
      {
          nextState = ES_state_picture_header;
      }
      else if ((sc == USER_DATA_START_CODE) ||
               ((sc == EXTENSION_START_CODE) &&
                ((sc_ext_id == ES_EXT_QUANT_MATRIX) ||
                 (sc_ext_id == ES_EXT_COPYRIGHT) ||
                 (sc_ext_id == ES_EXT_PICTURE_DISPLAY) ||
                 (sc_ext_id == ES_EXT_PICTURE_SPATIAL_SCALABLE) ||
                 (sc_ext_id == ES_EXT_PICTURE_TEMPORAL_SCALABLE))))
      {
         nextState = ES_state_ext2_user2;
      }
      else if ((sc >= FIRST_SLICE_START_CODE) &&
               (sc <= GREATEST_SLICE_START_CODE) &&
               (GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1))
      {
         /*MPEG1 stream -- Picture Data*/
         nextState = ES_state_picture_data;
      }
      else
      {
         /*Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }

      break;

   case   ES_state_picture_coding_ext:

      if ((sc == USER_DATA_START_CODE) ||
          ((sc == EXTENSION_START_CODE) &&
           ((sc_ext_id == ES_EXT_QUANT_MATRIX) ||
            (sc_ext_id == ES_EXT_COPYRIGHT) ||
            (sc_ext_id == ES_EXT_PICTURE_DISPLAY) ||
            (sc_ext_id == ES_EXT_PICTURE_SPATIAL_SCALABLE) ||
            (sc_ext_id == ES_EXT_PICTURE_TEMPORAL_SCALABLE))))
      {
         nextState = ES_state_ext2_user2;
      }
      else if ((sc & 0xffffff00) == PICTURE_START_CODE)
      {
          /*Picture Data*/
         nextState = ES_state_picture_data;
      }
      else
      {
          /*Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }
      break;

   case   ES_state_ext2_user2:

      if ((sc == USER_DATA_START_CODE) ||
          ((sc == EXTENSION_START_CODE) &&
           ((sc_ext_id == ES_EXT_QUANT_MATRIX) ||
            (sc_ext_id == ES_EXT_COPYRIGHT) ||
            (sc_ext_id == ES_EXT_PICTURE_DISPLAY) ||
            (sc_ext_id == ES_EXT_PICTURE_SPATIAL_SCALABLE) ||
            (sc_ext_id == ES_EXT_PICTURE_TEMPORAL_SCALABLE))))
      {
         nextState = ES_state_no_transition;
      }
      else if ((sc >= FIRST_SLICE_START_CODE) &&
               (sc <= GREATEST_SLICE_START_CODE))
      {
          /*Picture Data*/
         nextState = ES_state_picture_data;
      }
      else
      {
          /*Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }

      break;

   case   ES_state_picture_data:

      if ((sc >= FIRST_SLICE_START_CODE) &&
          (sc <= GREATEST_SLICE_START_CODE))
      {
          /*Picture Data*/
         nextState = ES_state_no_transition;
      }
      else if (sc == GROUP_START_CODE)
      {
         nextState = ES_state_gop_header;
      }
      else if (sc == PICTURE_START_CODE)
      {
         nextState = ES_state_picture_header;
      }
      else if (sc == SEQUENCE_END_CODE)
      {
         nextState = ES_state_sequence_end;
      }
      else if (sc == SEQUENCE_HEADER_CODE)
      {
         nextState = ES_state_sequence_header;
      }
      else
      {
         /* Unexpected sc. Indicate and transition directly to the new state.*/
         pESState->error_mask |= ES_error_unexpected_startcode;
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }

      break;

   case   ES_state_sequence_end:

      if (sc == SEQUENCE_HEADER_CODE)
      {
         nextState = ES_state_sequence_header;
      }
      else
      {
          /*Unexpected sc. Indicate and transition directly to the new state.*/
         if (sc != SEQUENCE_END_CODE)
         {
            pESState->error_mask |= ES_error_unexpected_startcode;
         }
         nextState = es_SkipToNextState (pESState->parse_state, sc);
      }

      break;
   }

    /*Store next state for use in startcode processing*/
   pESState->next_state = nextState;

   if (nextState != ES_state_inactive)
   {
       /*Perform any processing necessary for this startcode*/
      switch (sc)
      {

      case PICTURE_START_CODE:
         es_ProcessPictureHeader (pESState);
         break;

      case USER_DATA_START_CODE:
         es_ProcessUserDataStart (pESState);
         break;

      case SEQUENCE_HEADER_CODE:
         es_ProcessSequenceHeader (pESState);
         break;

      case SEQUENCE_ERROR_CODE:
         break;

      case EXTENSION_START_CODE:
         es_ProcessExtensionStart (pESState);
         break;

      case SEQUENCE_END_CODE:
         break;

      case GROUP_START_CODE:
         es_ProcessGOPStart (pESState);
         break;

      case DISCONTINUITY_START_CODE:
         es_ProcessDiscontinuityStart (pESState);
         break;

      default:
         if ((sc & 0xffffff00) == PICTURE_START_CODE)
         {
             /*Picture Data*/
         }
      }

       /*Transition to next state, if required*/
      if (nextState != ES_state_no_transition)
      {
         pESState->parse_state = nextState;
      }
   }
}

/*****************************************************************************
Function:    es_ProcessMPEG1SeqHdr

Parameters:  None

Returns:     None

Scope:

Purpose:     Fills in structure members that would be filled in
             when the MPEG2 seq extension arrives.

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessMPEG1SeqHdr (tESState *pESState)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    STVID_SequenceInfo_t *SequenceInfo_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo;
    MPEG2_PictureSpecificData_t *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STVID_VideoParams_t *VideoParams_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    STGXOBJ_Bitmap_t    *Bitmap_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.BitmapParams;
    VIDCOM_GlobalDecodingContextGenericData_t *GlobalDecodingContextGenericData_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;


   /*Maintain the stream type change flag*/
   /*This indicates that the picture is the first one with a new*/
   /*stream type*/
    if ((GlobalDecodingContextSpecificData_p->FirstPictureEver) || (GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1 == FALSE))
    {
        GlobalDecodingContextSpecificData_p->StreamTypeChange = TRUE;
    }
    else
    {
        GlobalDecodingContextSpecificData_p->StreamTypeChange = FALSE;
    }

/*    We know that this is MPEG1, since the seq ext didn't arrive*/
    SequenceInfo_p->MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_11172;
    GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1 = TRUE;

/*   Since we know we're looking at MPEG1, we can handle the error condition properly*/
    switch (SequenceInfo_p->Aspect)
    {
        case (ASPECT_RATIO_INFO_SQUARE):
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
            Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
            break;
        case (ASPECT_RATIO_INFO_4TO3):
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
            break;
        case (ASPECT_RATIO_INFO_16TO9):
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
            Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
            break;
        case (ASPECT_RATIO_INFO_221TO1):
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
            Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_221TO1;
            break;
        default :
            /* In MPEG1, many values can occur here */
            /* In MPEG1, only 0 and 0xf are errors */
            if ((SequenceInfo_p->Aspect == 0) || (SequenceInfo_p->Aspect == 0xf))
            {
                pESState->error_mask |= ES_error_aspect_ratio;
            }
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
            break;
    }

    /*For MPEG1, all sequences are Progressive*/
    SequenceInfo_p->ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
    VideoParams_p->ScanType = STGXOBJ_PROGRESSIVE_SCAN;

    /*MPEG1 always uses 601 color space*/
    GlobalDecodingContextGenericData_p->ColourPrimaries = 0;
    Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT601;

    /*for MPEG1, always 4:2:0  */
    GlobalDecodingContextSpecificData_p->chroma_format = CHROMA_FORMAT_4_2_0;


    /*For MPEG1, no sequences are low delay*/
    SequenceInfo_p->IsLowDelay = FALSE;

    /*For MPEG1, no meaning for these fields*/
    SequenceInfo_p->ProfileAndLevelIndication = 0;
    SequenceInfo_p->FrameRateExtensionN = 1;
    SequenceInfo_p->FrameRateExtensionD = 1;
    SequenceInfo_p->VideoFormat = VIDEO_FORMAT_UNSPECIFIED;

   /* Initialize these fields from Pic Coding Extension for MPEG1  */

    PictureSpecificData_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]  = 0;
    PictureSpecificData_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL]    = 0;
    PictureSpecificData_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL] = 0;
    PictureSpecificData_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]   = 0;
    PictureSpecificData_p->intra_dc_precision                         = 0;

    VideoParams_p->PictureStructure   = STVID_PICTURE_STRUCTURE_FRAME;

/*   VideoParams_p->TopFieldFirst      = TRUE;*/
    VideoParams_p->TopFieldFirst      = FALSE;         /*To Match ref_output*/

/*   PSD->frame_pred_frame_dct        = 0;*/
    PictureSpecificData_p->frame_pred_frame_dct        = 1;   /*To Match ref_output */
    PictureSpecificData_p->concealment_motion_vectors  = 0;
    PictureSpecificData_p->q_scale_type                = 0;
    PictureSpecificData_p->intra_vlc_format            = 0;
    PictureSpecificData_p->alternate_scan              = 0;
    PictureSpecificData_p->repeat_first_field          = 0;
    PictureSpecificData_p->chroma_420_type             = 0;
    PictureSpecificData_p->progressive_frame           = 0;
    PictureSpecificData_p->composite_display_flag      = 0;
    PictureSpecificData_p->v_axis                      = 0;
    PictureSpecificData_p->field_sequence              = 0;
    PictureSpecificData_p->sub_carrier                 = 0;
    PictureSpecificData_p->burst_amplitude             = 0;
    PictureSpecificData_p->sub_carrier_phase           = 0;


/*
#ifdef DEBUG_STARTCODE
   {
      printf ("    ES: Performing MPEG1 Post Sequence Header Init\n");
      printf ("        SequenceInfo_p->Aspect = %d\n", SequenceInfo_p->Aspect);
      printf ("        SequenceInfo_p->ScanType = %d\n", SequenceInfo_p->ScanType);
      printf ("        SequenceInfo_p->IsLowDelay = %d\n", SequenceInfo_p->IsLowDelay);
      printf ("        SequenceInfo_p->ProfileAndLevelIndication = %d\n", SequenceInfo_p->ProfileAndLevelIndication);
      printf ("        SequenceInfo_p->FrameRateExtensionN = %d\n", SequenceInfo_p->FrameRateExtensionN);
      printf ("        SequenceInfo_p->FrameRateExtensionD = %d\n", SequenceInfo_p->FrameRateExtensionD);
      printf ("        SequenceInfo_p->VideoFormat = %d\n", SequenceInfo_p->VideoFormat);
   }
#endif
*/
    return;
}

/*****************************************************************************
Function:    es_ProcessPictureHeader

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessPictureHeader (tESState *pESState)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    MPEG2_PictureDecodingContextSpecificData_t *PictureDecodingContextSpecificData_p = (MPEG2_PictureDecodingContextSpecificData_t *)ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p;
    MPEG2_PictureSpecificData_t *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STVID_VideoParams_t *VideoParams_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    gb_Handle_t   gbH = pESState->gbHandle;
    U32 value,i;

/*    Store current values for use when the picture coding extension arrives*/
    PictureSpecificData_p->PreviousMPEGFrame = VideoParams_p->MPEGFrame;

/*   Temporal Reference*/
    PictureSpecificData_p->temporal_reference = gb_GetBits (gbH, 10, NULL);

/*    Frame Type*/
    value = gb_GetBits (gbH, 3, NULL);
    if (value == 1)
    {
        VideoParams_p->MPEGFrame = STVID_MPEG_FRAME_I;
    }
    else if (value == 2)
    {
        VideoParams_p->MPEGFrame = STVID_MPEG_FRAME_P;
    }
    else if (value == 3)
    {
        VideoParams_p->MPEGFrame = STVID_MPEG_FRAME_B;
    }
    else
    {
        /*Error - Bogus frame type*/
        pESState->error_mask |= ES_error_invalid_frame_type;
    }

    PictureSpecificData_p->vbv_delay = gb_GetBits (gbH, 16, NULL);


    if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_P)
    {
        PictureSpecificData_p->full_pel_forward_vector  = gb_GetBits (gbH, 1, NULL);
        PictureSpecificData_p->forward_f_code           = gb_GetBits (gbH, 3, NULL);
        PictureSpecificData_p->full_pel_backward_vector = 0;
        PictureSpecificData_p->backward_f_code          = 0;
    }
    else if (VideoParams_p->MPEGFrame == STVID_MPEG_FRAME_B)
    {
        PictureSpecificData_p->full_pel_forward_vector  = gb_GetBits (gbH, 1, NULL);
        PictureSpecificData_p->forward_f_code           = gb_GetBits (gbH, 3, NULL);
        PictureSpecificData_p->full_pel_backward_vector = gb_GetBits (gbH, 1, NULL);
        PictureSpecificData_p->backward_f_code          = gb_GetBits (gbH, 3, NULL);
    }
    else
    {
        PictureSpecificData_p->full_pel_forward_vector  = 0;
        PictureSpecificData_p->forward_f_code           = 0;
        PictureSpecificData_p->full_pel_backward_vector = 0;
        PictureSpecificData_p->backward_f_code          = 0;
    }
    if (VideoParams_p->MPEGFrame != STVID_MPEG_FRAME_B)
    {
        if (!(PictureDecodingContextSpecificData_p->GotFirstReferencePicture))
        {
            /* This is the first I after the group of pictures header: just pass */
            PictureDecodingContextSpecificData_p->GotFirstReferencePicture = TRUE;
        }
        else
        {
            if (PictureDecodingContextSpecificData_p->IsBrokenLinkActive)
            {
                PictureDecodingContextSpecificData_p->IsBrokenLinkActive = FALSE;
                for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
                {
                    ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = FALSE;
                }
            }
        }
    }
    else
    {
        if (PictureDecodingContextSpecificData_p->IsBrokenLinkActive)
        {
            for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
            {
                if(ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsValidIndexInReferenceFrame[i] == TRUE)
                {
                    ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = TRUE;
                }
            }
        }
    }
}

/*****************************************************************************
Function:    es_ProcessUserDataStart

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessUserDataStart (tESState *pESState)
{
    gb_Handle_t   gbH = pESState->gbHandle;
    U32 nextState = pESState->next_state;
    U8  *UDBuffer;
    U32 UDBufferLen;
    U32 BitsLeft;

#ifdef DEBUG_STARTCODE
    if (indSC >= MAX_TRACES_SC)
    {
        indSC = 0;
    }
    sprintf (&tracesSC[indSC][0], "    ES: Processing User Data Startcode\n");
    indSC++;
#endif

/*
    Copy the data from the bitbuffer bytewise to keep
    byte ordering correct on all platforms. The spec says
    that the data will be 8-bit integers, so we shouldn't
    have to worry about leftover bits.
*/
    UDBuffer = (U8*) pESState->UserData->Buff_p;
    UDBufferLen = 0;

    do
    {
        *UDBuffer = gb_GetBits (gbH, 8, &BitsLeft);
        UDBuffer++;
        UDBufferLen++;
    }
    while ( (pESState->UserDataSize > UDBufferLen) && (BitsLeft > 0));

    pESState->UserData->BufferOverflow = (BitsLeft > 0) ? TRUE : FALSE;
    pESState->UserData->Length = UDBufferLen;



/*   The method used to extract the User Data varies based
   on the location the user data was found.  This function
   will extract the data as appropriate for its type, then
   indicate that it is available. The message with the data
   will be sent from the parser task once the parsing of this
   startcode is complete.
*/

    switch (nextState)
    {
        case ES_state_ext0_user0:
        {
            /* This is a sequence user data */
            pESState->UserData->PositionInStream = STVID_USER_DATA_AFTER_SEQUENCE;
            break;
        }

        case ES_state_user1:
        {
            /* This is a group user data */
            pESState->UserData->PositionInStream = STVID_USER_DATA_AFTER_GOP;
            break;
        }

        case ES_state_ext2_user2:
        {
            /* This is a picture user data */
            pESState->UserData->PositionInStream = STVID_USER_DATA_AFTER_PICTURE;
            break;
        }
    }
/*
#ifdef DEBUG_STARTCODE
   {
      int i, cnt;
      UDBuffer = (U8*) pESState->UserData->Buff_p;
      printf ("User Data Startcode. Location = %s\n", (nextState == ES_state_ext0_user0) ? "STVID_USER_DATA_AFTER_SEQUENCE" : (nextState == ES_state_user1) ? "STVID_USER_DATA_AFTER_GOP" : "STVID_USER_DATA_AFTER_PICTURE");

      for (i=0; i < (UDBufferLen/16) + 1 ; i++)
      {
         printf ("%08x:  ", i * 16);

         for (cnt = (i * 16); cnt < (i * 16) + 16; cnt++)
         {
            printf("  %02x", UDBuffer[cnt]);
         }

         printf ("    ");
         for (cnt = (i * 16); cnt < (i * 16) + 16; cnt++)
         {
            if ((UDBuffer[cnt] >= 0x20) && (UDBuffer[cnt] != 0x7f))
               printf("%1c", UDBuffer[cnt]);
            else
               printf(".");
         }
         printf ("\n");
      }
   }
#endif
*/

}

/*****************************************************************************
 Function:    es_ProcessSequenceHeader

 Parameters:  None

 Returns:     None

 Scope:

 Purpose:

 Behavior:
              The state machine is derived from the one included in the
              mpeg-2 spec (13818-2, fig 6-15). The state is maintained for
              each stream across the PES packet boundary.

              Note that right now, the code is written to assume:
              1) Startcodes will not be split across PES packets
              2) Syntactic elements will not be split across PES packets

 Exceptions:  None

*****************************************************************************/

void es_ProcessSequenceHeader(tESState *pESState)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    STVID_SequenceInfo_t *SequenceInfo_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    VIDCOM_GlobalDecodingContextGenericData_t *GlobalDecodingContextGenericData_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData;
    STGXOBJ_Bitmap_t    *Bitmap_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.BitmapParams;
    STVID_VideoParams_t *VideoParams_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    MPEG2_PictureSpecificData_t *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    gb_Handle_t   gbH = pESState->gbHandle;
    U32  value;
    BOOL QuantiserFromSeq;
    GlobalDecodingContextSpecificData_p->NewSeqHdr = TRUE;

  /* Note that many of these are partial values. Final values*/
  /* will be calculated once seq_ext is received for MPEG2 stream*/
  /* or next startcode for MPEG1.*/

  /* Get the Width and Height (only partial for MPEG2)*/
    SequenceInfo_p->Width  = gb_GetBits(gbH, 12, NULL);
    Bitmap_p->Width = Bitmap_p->Pitch = SequenceInfo_p->Width;

    SequenceInfo_p->Height = gb_GetBits(gbH, 12, NULL);
    Bitmap_p->Height = SequenceInfo_p->Height;

   /*Get the Aspect Ratio*/
    SequenceInfo_p->Aspect = /*(STVID_DisplayAspectRatio_t)*/ gb_GetBits(gbH, 4, NULL);

    /* Compute frame rate */
    value = gb_GetBits(gbH, 4, NULL);
    switch (value)
    {
        case FRAME_RATE_CODE_24000_1001 :
            SequenceInfo_p->FrameRate = 23976;
            break;

        case FRAME_RATE_CODE_24 :
            SequenceInfo_p->FrameRate = 24000;
            break;

        case FRAME_RATE_CODE_25 :
            SequenceInfo_p->FrameRate = 25000;
            break;

        case FRAME_RATE_CODE_30000_1001 :
            SequenceInfo_p->FrameRate = 29970;
            break;

        case FRAME_RATE_CODE_30 :
            SequenceInfo_p->FrameRate = 30000;
            break;

        case FRAME_RATE_CODE_50 :
            SequenceInfo_p->FrameRate = 50000;
            break;

        case FRAME_RATE_CODE_60000_1001 :
            SequenceInfo_p->FrameRate = 59940;
            break;

        case FRAME_RATE_CODE_60 :
            SequenceInfo_p->FrameRate = 60000;
            break;

        default :
            /*Error - Unsupported Frame Rate*/
            SequenceInfo_p->FrameRate = 50000;
            pESState->error_mask |= ES_error_frame_rate;
            break;
    }

    VideoParams_p->FrameRate = SequenceInfo_p->FrameRate;
    GlobalDecodingContextSpecificData_p->HasGroupOfPictures = FALSE;


    /*Store lower 18 bits of bitrate.*/
    SequenceInfo_p->BitRate = gb_GetBits(gbH, 18, NULL);

    /*Marker Bit check*/
    if (gb_GetBits(gbH, 1, NULL) != 1)
    {
        pESState->error_mask |= ES_error_marker_bit;
    }

    /*Store lower 10 bits of VBV buffer size. Calculate VBVBufferSize*/
    /*Calculate VBVBufferSize using MPEG-1 method for now. If this is*/
    /*an MPEG-2 stream, it will be recalculated when the seq extension*/
    /*arrives.*/
    GlobalDecodingContextSpecificData_p->vbv_buffer_size_value = gb_GetBits (gbH, 10, NULL);
    SequenceInfo_p->VBVBufferSize = (GlobalDecodingContextSpecificData_p->vbv_buffer_size_value) * 2048;

    /*Flag Check (flag used in ISO 11172-2 only)*/
    value = gb_GetBits(gbH, 1, NULL);
    if (value != 0)
    {
        /*Error - constrained Parameters Flag should be 0*/
        pESState->error_mask |= ES_error_constrained_parm_flag;
    }

    QuantiserFromSeq= TRUE;
    es_ProcessQuantMatrix (pESState, QuantiserFromSeq);

    GlobalDecodingContextSpecificData_p->NewQuantMxt = TRUE;



    /*Initialize Default Sequence Based Data for Optional Startcodes*/




    /*Sequence Extension Data*/
    /*For MPEG-1 streams, this data will be overwritten when the seq ext*/
    /*is not received.*/
    /*For MPEG-2 streams, this data will be loaded when its seq ext*/
    /*is recieved.*/



    /*Sequence Display Extension Data*/


    /*Flag indicating Existance of Sequence Display Extension*/
    GlobalDecodingContextSpecificData_p->HasSequenceDisplay = 0;

    /*Video Format*/
    SequenceInfo_p->VideoFormat = 5;

    /*Colour Primaries*/
    /*TODO: The older code had defaults for this based on the BroadcastProfile flag*/
    /*We don't need this. Does ST?*/
    GlobalDecodingContextGenericData_p->ColourPrimaries = COLOUR_PRIMARIES_ITU_R_BT_709;
    Bitmap_p->ColorSpaceConversion = STGXOBJ_ITU_R_BT709;

    /*    Transfer Characteristics*/
    GlobalDecodingContextGenericData_p->TransferCharacteristics = TRANSFER_ITU_R_BT_709;

    /*    Matrix Coefficients*/
    GlobalDecodingContextGenericData_p->MatrixCoefficients = MATRIX_COEFFICIENTS_ITU_R_BT_709;

    /*   Display Horizontal Size*/
    GlobalDecodingContextSpecificData_p->display_horizontal_size = 0;

    /*    Display Vertical Size*/
    GlobalDecodingContextSpecificData_p->display_vertical_size = 0;


    /*Sequence Scalable Extension Data (Not supported)*/



    /*Sequence Quant Matrix Extension (Init'ed by Seq Hdr)*/



    /*Sequence Copyright Extension (Not supported)*/



    /*Sequence Picture Display Extension*/

    PictureSpecificData_p->number_of_frame_centre_offsets = 0;


    /* Sequence Picture Spatial Scalable Extension (Not supported)*/



    /* Picture Temporal Scalable Extension (Not supported)*/

    /* For Gop less streams */
    PictureSpecificData_p->PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
    if (GlobalDecodingContextSpecificData_p->FirstPictureEver == FALSE)
    {
        /*The default values for these are set up the first time the PJR is*/
        /*built. The arrival of the GOP header implies reset of temporal */
        /*reference to 0: update offset with picture counter to continue */
        /*counting up*/
        GlobalDecodingContextSpecificData_p->TemporalReferenceOffset += GlobalDecodingContextSpecificData_p->PictureCounter;
        GlobalDecodingContextSpecificData_p->PictureCounter = 0;
        if ((GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference > 0) &&
            (GlobalDecodingContextSpecificData_p->TemporalReferenceOffset <= GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference))
        {
            /* In case the last calculated ExtendedTemporalReference is higher than
                the offset (ex: cut stream after Pn and Bn-2/Bn-1 missing), push the offset up */
            GlobalDecodingContextSpecificData_p->TemporalReferenceOffset = GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference + 1;
        }
    }
        GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed = FALSE;
        GlobalDecodingContextSpecificData_p->GetSecondReferenceAfterOverflowing = FALSE;
        GlobalDecodingContextSpecificData_p->tempRefReachedZero = FALSE;


/*
#ifdef DEBUG_STARTCODE
   {
      printf ("    ES: Processing Sequence Header\n");
      printf ("        Width = %d\n", SequenceInfo_p->Width);
      printf ("        Height = %d\n", SequenceInfo_p->Height);
      printf ("        Aspect Ratio = %d\n", SequenceInfo_p->Aspect);
      printf ("        Frame Rate = %d\n", SequenceInfo_p->FrameRate);
      printf ("        Bit Rate = %d\n", SequenceInfo_p->BitRate);
      printf ("        VBV Buffer size (10 bits) = %d\n", SequenceInfo_p->VBVBufferSize);
      printf ("        load intra quantizer matrix = %d\n", GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix);
      printf ("        load non-intra quantizer matrix = %d\n", GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix);
   }
#endif
*/
}

/*****************************************************************************
Function:    es_ProcessExtensionStart

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessExtensionStart (tESState *pESState)
{
    U32 nextState = pESState->next_state;
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    STVID_SequenceInfo_t *SequenceInfo_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo;
    VIDCOM_GlobalDecodingContextGenericData_t *GlobalDecodingContextGenericData_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    MPEG2_PictureSpecificData_t *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STGXOBJ_Bitmap_t    *Bitmap_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.BitmapParams;
    STVID_VideoParams_t *VideoParams_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    gb_Handle_t   gbH = pESState->gbHandle;
    U32  index, value;
    BOOL QuantiserFromSeq;
    U16    ProfileAndLevelHorizontalSize;
    U16    ProfileAndLevelVerticalSize;

    switch (nextState)
    {
        case ES_state_sequence_extension:
        {
            value = gb_GetBits (gbH, 4, NULL);

            /*The Sequence Extension indicates that it is MPEG2*/
            SequenceInfo_p->MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_13818;

            /*The StreamTypeChange flag indicates that the picture is*/
            /*the first one with a new stream type*/
            if ((GlobalDecodingContextSpecificData_p->FirstPictureEver) || (GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1 == TRUE))
            {
                GlobalDecodingContextSpecificData_p->StreamTypeChange = TRUE;
            }
            else
            {
                GlobalDecodingContextSpecificData_p->StreamTypeChange = FALSE;
            }

            GlobalDecodingContextSpecificData_p->IsBitStreamMPEG1 = FALSE;



          /*Since we know we're looking at MPEG2, we can handle the error condition properly*/
            switch (SequenceInfo_p->Aspect)
            {
                case (ASPECT_RATIO_INFO_SQUARE):
                    SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
                    Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
                    break;
                case (ASPECT_RATIO_INFO_4TO3):
                    SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
                    Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
                    break;
                case (ASPECT_RATIO_INFO_16TO9):
                    SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
                    Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
                    break;
                case (ASPECT_RATIO_INFO_221TO1):
                    SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
                    Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_221TO1;
                    Bitmap_p->AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
                    break;
                default :
                    SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
                    pESState->error_mask |= ES_error_aspect_ratio;
                    break;
            }

            SequenceInfo_p->ProfileAndLevelIndication = gb_GetBits (gbH, 8, NULL);

            value = gb_GetBits (gbH, 1, NULL);
            SequenceInfo_p->ScanType =  (value == 1) ? STVID_SCAN_TYPE_PROGRESSIVE : STVID_SCAN_TYPE_INTERLACED;
            VideoParams_p->ScanType  = (value == 1) ? STGXOBJ_PROGRESSIVE_SCAN : STGXOBJ_INTERLACED_SCAN;

            /*Chroma Format*/
            GlobalDecodingContextSpecificData_p->chroma_format = gb_GetBits (gbH, 2, NULL);

            /*Complete Height and Width calculations*/
            value = gb_GetBits (gbH, 2, NULL);
            SequenceInfo_p->Width |= value << 12;
            Bitmap_p->Width = Bitmap_p->Pitch = SequenceInfo_p->Width;


            value = gb_GetBits (gbH, 2, NULL);
            SequenceInfo_p->Height |= value << 12;
            Bitmap_p->Height = SequenceInfo_p->Height;


            /* Complete Bit Rate Calculation*/
            value = gb_GetBits (gbH, 12, NULL);
            SequenceInfo_p->BitRate |= value << 18;

            /*Marker Bit check*/
            if (gb_GetBits(gbH, 1, NULL) != 1)
            {
                pESState->error_mask |= ES_error_marker_bit;
                break;
            }

/*          Complete vbv buffer size Calculation now that all 18 bits are present*/
            value = gb_GetBits (gbH, 8, NULL);
            SequenceInfo_p->VBVBufferSize = ((U32)GlobalDecodingContextSpecificData_p->vbv_buffer_size_value + (value << 10)) * 2048;

/*          Low Delay Sequence Flag*/
            SequenceInfo_p->IsLowDelay =  gb_GetBits (gbH, 1, NULL);

/*          Frame Rate Extension*/
            SequenceInfo_p->FrameRateExtensionN =  gb_GetBits (gbH, 2, NULL);
            SequenceInfo_p->FrameRateExtensionD =  gb_GetBits (gbH, 5, NULL);

            /* Check sequence parameters according to MPEG profile */

            switch (SequenceInfo_p->ProfileAndLevelIndication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
            {
                case (PROFILE_MAIN | LEVEL_MAIN) : /* MPML */
                    ProfileAndLevelHorizontalSize = 720;
                    ProfileAndLevelVerticalSize = 576;
                    break;

                case (PROFILE_MAIN | LEVEL_HIGH) : /* MPHL */
                    ProfileAndLevelHorizontalSize = 1920;
                    ProfileAndLevelVerticalSize = 1152;
                    break;

                case (PROFILE_MAIN | LEVEL_HIGH_1440) : /* MP H1440 */
                    ProfileAndLevelHorizontalSize = 1440;
                    ProfileAndLevelVerticalSize = 1152;
                    break;

                default :
                    ProfileAndLevelHorizontalSize = 720;
                    ProfileAndLevelVerticalSize = 576;
                    break;
            }

            /* Check if the Width and Height values correspond to the MPEG Profile. */
            if ((SequenceInfo_p->Width > ProfileAndLevelHorizontalSize) ||
                (SequenceInfo_p->Height > ProfileAndLevelVerticalSize ))
            {
                /* Error_Recovery_Case_n02 : The dimensions for the sequence don't correspond to the MPEG Profile.    */
                /* The stream won't be displayed as is because such as dimensions are not managed. Skip the sequence. */
                pESState->error_mask |= ES_error_unexpected_profile;
            }
/*
#ifdef DEBUG_STARTCODE
         {
            printf ("    ES: Processing Sequence Extension\n");
            printf ("        Aspect Ratio = %d\n", SequenceInfo_p->Aspect);
            printf ("        ProfileAndLevelIndication = %d\n", SequenceInfo_p->ProfileAndLevelIndication);
            printf ("        ScanType = %d\n", SequenceInfo_p->ScanType);
            printf ("        chroma_format = %d\n", GlobalDecodingContextSpecificData_p->chroma_format);
            printf ("        BitRate = %d\n", SequenceInfo_p->BitRate);
            printf ("        VBVBufferSize = %d\n", SequenceInfo_p->VBVBufferSize);
            printf ("        IsLowDelay = %d\n", SequenceInfo_p->IsLowDelay);
            printf ("        FrameRateExtensionN = %d\n", SequenceInfo_p->FrameRateExtensionN);
            printf ("        FrameRateExtensionD = %d\n", SequenceInfo_p->FrameRateExtensionD);
         }
#endif
*/
            break;
        }

        case ES_state_ext0_user0:
        {
            value = gb_GetBits (gbH, 4, NULL);

            if (value == ES_EXT_SEQ_DISPLAY)
            {
                GlobalDecodingContextSpecificData_p->HasSequenceDisplay = 1;

/*             Video Format*/
                SequenceInfo_p->VideoFormat = gb_GetBits (gbH, 3, NULL);

/*            Color Description*/
                value = gb_GetBits (gbH, 1, NULL);

                if (value == 1)
                {
/*                Colour Primaries. The STGXOBJ enum maps directly to this bitmap*/
                    GlobalDecodingContextGenericData_p->ColourPrimaries = gb_GetBits (gbH, 8, NULL);
                    Bitmap_p->ColorSpaceConversion = GlobalDecodingContextGenericData_p->ColourPrimaries;


/*                Transfer Characteristics*/
                    GlobalDecodingContextGenericData_p->TransferCharacteristics = gb_GetBits (gbH, 8, NULL);

/*                Matrix Coefficients*/
                    GlobalDecodingContextGenericData_p->MatrixCoefficients = gb_GetBits (gbH, 8, NULL);
                    GlobalDecodingContextSpecificData_p->matrix_coefficients = GlobalDecodingContextGenericData_p->MatrixCoefficients;
                }

/*             Display Horizontal Size*/
                GlobalDecodingContextSpecificData_p->display_horizontal_size = gb_GetBits (gbH, 14, NULL);

/*             Marker Bit check*/
                if (gb_GetBits(gbH, 1, NULL) != 1)
                {
                    pESState->error_mask |= ES_error_marker_bit;
                    break;
                }

             /*Display Vertical Size*/
                GlobalDecodingContextSpecificData_p->display_vertical_size = gb_GetBits (gbH, 14, NULL);
            }
            else  /*(value == ES_EXT_SEQ_SCALABLE)*/
            {
             /*Process Sequence Scalable Extension*/


             /*For now, do nothing.*/
             /*This Extension is not supported in SA or STAPI drivers*/

            }
/*
#ifdef DEBUG_STARTCODE
         {
            printf ("    ES: Processing Sequence Display Extension\n");
            printf ("        HasSequenceDisplay = %d\n", GlobalDecodingContextSpecificData_p->HasSequenceDisplay);
            printf ("        VideoFormat = %d\n", SequenceInfo_p->VideoFormat);
            printf ("        ColourPrimaries = %d\n", GlobalDecodingContextGenericData_p->ColourPrimaries);
            printf ("        TransferCharacteristics = %d\n", GlobalDecodingContextGenericData_p->TransferCharacteristics);
            printf ("        MatrixCoefficients = %d\n", GlobalDecodingContextGenericData_p->MatrixCoefficients);
            printf ("        display_horizontal_size = %d\n", GlobalDecodingContextSpecificData_p->display_horizontal_size);
            printf ("        display_vertical_size = %d\n", GlobalDecodingContextSpecificData_p->display_vertical_size);
         }
#endif
*/
            break;
        }

        case ES_state_ext2_user2:
        {
            value = gb_GetBits (gbH, 4, NULL);
            if (value == ES_EXT_QUANT_MATRIX)
            {
                GlobalDecodingContextSpecificData_p->NewQuantMxt = true;
                QuantiserFromSeq=FALSE;
                es_ProcessQuantMatrix (pESState, QuantiserFromSeq);

            }
            else if (value == ES_EXT_COPYRIGHT)
            {

             /*Process Copyright Extension*/



             /*For now, do nothing.*/
             /*This Extension is not supported in SA or STAPI drivers*/


            }
            else if (value == ES_EXT_PICTURE_DISPLAY)
            {
                if (SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE)
                {
                    if (PictureSpecificData_p->repeat_first_field)
                    {
                        if (VideoParams_p->TopFieldFirst)
                        {
                            PictureSpecificData_p->number_of_frame_centre_offsets = 3;
                        }
                        else
                        {
                            PictureSpecificData_p->number_of_frame_centre_offsets = 2;
                        }
                    }
                    else
                    {
                        PictureSpecificData_p->number_of_frame_centre_offsets = 1;
                    }
                }
                else
                {
                    if ((VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_TOP_FIELD) ||
                        (VideoParams_p->PictureStructure == STVID_PICTURE_STRUCTURE_BOTTOM_FIELD))
                    {
                        PictureSpecificData_p->number_of_frame_centre_offsets = 1;
                    }
                    else
                    {
                        if (PictureSpecificData_p->repeat_first_field)
                        {
                            PictureSpecificData_p->number_of_frame_centre_offsets = 3;
                        }
                        else
                        {
                            PictureSpecificData_p->number_of_frame_centre_offsets = 2;
                        }
                    }
                }

                for (index = 0; index < (U32) PictureSpecificData_p->number_of_frame_centre_offsets; index++)
                {
                    PictureSpecificData_p->FrameCentreOffsets[index].frame_centre_horizontal_offset = gb_GetBits(gbH, 16, NULL);

    /*                Marker Bit check*/
                    if (gb_GetBits(gbH, 1, NULL) != 1)
                    {
                        pESState->error_mask |= ES_error_marker_bit;
                        break;
                    }

                    PictureSpecificData_p->FrameCentreOffsets[index].frame_centre_vertical_offset = gb_GetBits(gbH, 16, NULL);

    /*                Marker Bit check*/
                    if (gb_GetBits(gbH, 1, NULL) != 1)
                    {
                        pESState->error_mask |= ES_error_marker_bit;
                        break;
                    }
                }

            }
            else if (value == ES_EXT_PICTURE_SPATIAL_SCALABLE)
            {

                /*Process Picture Spatial Scalable Extension*/



                /*For now, do nothing.*/
                /*This Extension is not supported in SA or STAPI drivers*/


            }
            else  /*(value == ES_EXT_PICTURE_TEMPORAL_SCALABLE)*/
            {

                /*Process Picture Temporal Scalable Extension*/



                /*For now, do nothing.*/
                /*This Extension is not supported in SA or STAPI drivers*/

            }

            break;
        }

        case ES_state_picture_coding_ext:
        {
            U8  picture_structure;

            value = gb_GetBits (gbH, 4, NULL);

            PictureSpecificData_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]  = gb_GetBits (gbH, 4, NULL);
            PictureSpecificData_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL]    = gb_GetBits (gbH, 4, NULL);
            PictureSpecificData_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL] = gb_GetBits (gbH, 4, NULL);
            PictureSpecificData_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]   = gb_GetBits (gbH, 4, NULL);
            PictureSpecificData_p->intra_dc_precision                         = gb_GetBits (gbH, 2, NULL);

            picture_structure                               = gb_GetBits (gbH, 2, NULL);
            if (picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE)
            {
                VideoParams_p->PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
            }
            if (picture_structure == PICTURE_STRUCTURE_TOP_FIELD)
            {
                VideoParams_p->PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
            }
            else if (picture_structure == PICTURE_STRUCTURE_BOTTOM_FIELD)
            {
                VideoParams_p->PictureStructure = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
            }
            else
            {
                /*Not sure what to do with a "reserved" so pick a frame*/
                VideoParams_p->PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
            }


            VideoParams_p->TopFieldFirst                               = gb_GetBits (gbH, 1, NULL);

            PictureSpecificData_p->frame_pred_frame_dct                       = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->concealment_motion_vectors                 = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->q_scale_type                               = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->intra_vlc_format                           = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->alternate_scan                             = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->repeat_first_field                         = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->chroma_420_type                            = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->progressive_frame                          = gb_GetBits (gbH, 1, NULL);
            PictureSpecificData_p->composite_display_flag                     = gb_GetBits (gbH, 1, NULL);
            if (PictureSpecificData_p->composite_display_flag)
            {
                PictureSpecificData_p->v_axis            = gb_GetBits (gbH, 1, NULL);
                PictureSpecificData_p->field_sequence    = gb_GetBits (gbH, 3, NULL);
                PictureSpecificData_p->sub_carrier       = gb_GetBits (gbH, 1, NULL);
                PictureSpecificData_p->burst_amplitude   = gb_GetBits (gbH, 7, NULL);
                PictureSpecificData_p->sub_carrier_phase = gb_GetBits (gbH, 8, NULL);
            }
            else
            {
                PictureSpecificData_p->v_axis            = 0;
                PictureSpecificData_p->field_sequence    = 0;
                PictureSpecificData_p->sub_carrier       = 0;
                PictureSpecificData_p->burst_amplitude   = 0;
                PictureSpecificData_p->sub_carrier_phase = 0;
            }


       /* Manage Flags for Field-based streams*/



/*
#ifdef DEBUG_STARTCODE
         {
            printf ("    ES: Processing Picture Decoding Extension\n");

            printf ("        f_code = %d, %d, %d, %d\n", PictureSpecificData_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL],
                     PictureSpecificData_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL], PictureSpecificData_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL],
                     PictureSpecificData_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]);
            printf ("        top_field_first = %d\n", VideoParams_p->TopFieldFirst);
            printf ("        frame_pred_frame_dct = %d\n", PictureSpecificData_p->frame_pred_frame_dct);
            printf ("        concealment_motion_vectors = %d\n", PictureSpecificData_p->concealment_motion_vectors);
            printf ("        q_scale_type = %d\n", PictureSpecificData_p->q_scale_type);
            printf ("        intra_vlc_format = %d\n", PictureSpecificData_p->intra_vlc_format);
            printf ("        alternate_scan = %d\n", PictureSpecificData_p->alternate_scan);
            printf ("        repeat_first_field = %d\n", PictureSpecificData_p->repeat_first_field);
            printf ("        chroma_420_type = %d\n", PictureSpecificData_p->chroma_420_type);
            printf ("        progressive_frame = %d\n", PictureSpecificData_p->progressive_frame);
            printf ("        composite_display_flag = %d\n", PictureSpecificData_p->composite_display_flag);
            printf ("        v_axis = %d\n", PictureSpecificData_p->v_axis);
            printf ("        field_sequence = %d\n", PictureSpecificData_p->field_sequence);
            printf ("        sub_carrier = %d\n", PictureSpecificData_p->sub_carrier);
            printf ("        burst_amplitude = %d\n", PictureSpecificData_p->burst_amplitude);
            printf ("        sub_carrier_phase = %d\n", PictureSpecificData_p->sub_carrier_phase);
            printf ("        TopFieldFirst = %d\n", VideoParams_p->TopFieldFirst);
         }
#endif
*/
            break;
        }
    }
}
/*****************************************************************************
Function:    es_ProcessExtensionQuantMatrix

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessQuantMatrix (tESState *pESState, BOOL Sequence_H)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *)ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    MPEG2_PictureSpecificData_t *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    gb_Handle_t   gbH = pESState->gbHandle;
    U32 index;
    BOOL  prev_load_intra_quantizer_matrix = GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix;
    BOOL  prev_load_non_intra_quantizer_matrix = GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix;


   /*Loads the Quantization Matrices from the Sequence Header or*/
   /*Quant matrix. If the flag indicates that the matrix need*/
   /*not be loaded, the default matrix is loaded instead.*/


   /*Luma Intra Quantizer Matrix*/
    GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix = (gb_GetBits(gbH, 1, NULL) != 0);
    for (index = 0; index < MPEG2_Q_MATRIX_SIZE; index ++)
    {
        if (GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix)
        {
            if (Sequence_H== TRUE)
            {
#ifdef MME_SCANORDER
                GlobalDecodingContextSpecificData_p->intra_quantizer_matrix[gES_scan[0][index]] = gb_GetBits(gbH, 8, NULL);
#else
                GlobalDecodingContextSpecificData_p->intra_quantizer_matrix[index] = gb_GetBits(gbH, 8, NULL);
#endif
            }
            else
            {
#ifdef MME_SCANORDER
                PictureSpecificData_p->LumaIntraQuantMatrix[gES_scan[0][index]] = gb_GetBits(gbH, 8, NULL);
#else
                PictureSpecificData_p->LumaIntraQuantMatrix[index] = gb_GetBits(gbH, 8, NULL);
#endif
            }
        }
        else
        {
            if (Sequence_H== TRUE)
            {
                GlobalDecodingContextSpecificData_p->intra_quantizer_matrix[index] = gES_DefaultIntraQM [index];
            }
            else
            {
                PictureSpecificData_p->LumaIntraQuantMatrix[index] = gES_DefaultIntraQM [index];
            }
        }
    }


   /*The Intra Quant Mtx has changed if a new one was loaded, or if the default matrix*/
   /*was loaded over a non-default matrix. Note that the flag must be initialized to */
   /*TRUE for this logic to work.*/

    if ((GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix == TRUE) ||
        ((GlobalDecodingContextSpecificData_p->load_intra_quantizer_matrix == FALSE) &&
        (prev_load_intra_quantizer_matrix == TRUE)))
    {
        PictureSpecificData_p->IntraQuantMatrixHasChanged = TRUE;
    }
    else
    {
        PictureSpecificData_p->IntraQuantMatrixHasChanged = FALSE;
    }
    if (Sequence_H== TRUE)
    {
        PictureSpecificData_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(GlobalDecodingContextSpecificData_p->intra_quantizer_matrix[0]);
    }
    else
    {
        PictureSpecificData_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(PictureSpecificData_p->LumaIntraQuantMatrix[0]);
    }

   /*Luma Non-Intra Quantizer Matrix*/
    GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix = (gb_GetBits(gbH, 1, NULL) != 0);
    for (index = 0; index < MPEG2_Q_MATRIX_SIZE; index ++)
    {
        if (GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix)
        {
            if (Sequence_H== TRUE)
            {
#ifdef MME_SCANORDER
                GlobalDecodingContextSpecificData_p->non_intra_quantizer_matrix[gES_scan[0][index]] = gb_GetBits(gbH, 8, NULL);
#else
                GlobalDecodingContextSpecificData_p->non_intra_quantizer_matrix[index] = gb_GetBits(gbH, 8, NULL);
#endif
            }
            else
            {
#ifdef MME_SCANORDER
                PictureSpecificData_p->LumaNonIntraQuantMatrix[gES_scan[0][index]] = gb_GetBits(gbH, 8, NULL);
#else
                PictureSpecificData_p->LumaNonIntraQuantMatrix[index] = gb_GetBits(gbH, 8, NULL);
#endif
            }
        }
        else
        {
            if (Sequence_H== TRUE)
            {
                GlobalDecodingContextSpecificData_p->non_intra_quantizer_matrix[index] = 16;
            }
            else
            {
                PictureSpecificData_p->LumaNonIntraQuantMatrix[index] = 16;
            }
        }
   }


   /*The Non Intra Quant Mtx has changed if a new one was loaded, or if the */
   /*default matrix was loaded over a non-default matrix. Note that the flag */
   /*must be initialized to TRUE for this logic to work.*/

    if ((GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix == TRUE) ||
        ((GlobalDecodingContextSpecificData_p->load_non_intra_quantizer_matrix == FALSE) &&
        (prev_load_non_intra_quantizer_matrix == TRUE)))
    {
        PictureSpecificData_p->NonIntraQuantMatrixHasChanged = TRUE;
    }
    else
    {
        PictureSpecificData_p->NonIntraQuantMatrixHasChanged = FALSE;
    }
    if (Sequence_H== TRUE)
    {
        PictureSpecificData_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *)&(GlobalDecodingContextSpecificData_p->non_intra_quantizer_matrix[0]);
    }
    else
    {
        PictureSpecificData_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *)&(PictureSpecificData_p->LumaNonIntraQuantMatrix[0]);
    }

   /*Load Chroma Intra Quantizer Matrix if flag set, and we're processing Extension*/
    if (pESState->next_state == ES_state_ext2_user2)
    {
        GlobalDecodingContextSpecificData_p->load_chroma_intra_quantizer_matrix = gb_GetBits(gbH, 1, NULL);

        if ((GlobalDecodingContextSpecificData_p->load_chroma_intra_quantizer_matrix == 1) && (GlobalDecodingContextSpecificData_p->chroma_format == CHROMA_FORMAT_4_2_0))
        {
            /*Error -- matrix not needed for this format*/
            pESState->error_count++;
            pESState->parse_error = ES_error_unexpected_chroma_mtx;
            GlobalDecodingContextSpecificData_p->load_chroma_intra_quantizer_matrix = FALSE;
        }
    }
    else
    {
        GlobalDecodingContextSpecificData_p->load_chroma_intra_quantizer_matrix = FALSE;
    }

    for (index = 0; index < MPEG2_Q_MATRIX_SIZE; index ++)
    {
        if (GlobalDecodingContextSpecificData_p->load_chroma_intra_quantizer_matrix)
        {
            if (Sequence_H== TRUE)
            {
                GlobalDecodingContextSpecificData_p->chroma_intra_quantizer_matrix[index] = gb_GetBits(gbH, 8, NULL);
            }
            else
            {
                PictureSpecificData_p->ChromaIntraQuantMatrix[index] = gb_GetBits(gbH, 8, NULL);
            }
        }
        else
        {
            if (Sequence_H== TRUE)
            {
                GlobalDecodingContextSpecificData_p->chroma_intra_quantizer_matrix[index] = gES_DefaultIntraQM [index];
            }
            else
            {
                PictureSpecificData_p->ChromaIntraQuantMatrix[index] = gES_DefaultIntraQM [index];
            }
        }
    }

    if (Sequence_H== TRUE)
    {
        PictureSpecificData_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *)&(GlobalDecodingContextSpecificData_p->chroma_intra_quantizer_matrix[0]);
    }
    else
    {
        PictureSpecificData_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *)&(PictureSpecificData_p->ChromaIntraQuantMatrix[0]);
    }

    /*Load Chroma Non-Intra Quantizer Matrix if flag set, and we're processing Extension*/
    if (pESState->next_state == ES_state_ext2_user2)
    {
        GlobalDecodingContextSpecificData_p->load_chroma_non_intra_quantizer_matrix = gb_GetBits(gbH, 1, NULL);

        if ((GlobalDecodingContextSpecificData_p->load_chroma_non_intra_quantizer_matrix == TRUE) && (GlobalDecodingContextSpecificData_p->chroma_format == CHROMA_FORMAT_4_2_0))
        {
            /*Error -- matrix not needed for this format*/
            pESState->error_count++;
            pESState->parse_error = ES_error_unexpected_chroma_mtx;
            GlobalDecodingContextSpecificData_p->load_chroma_non_intra_quantizer_matrix = FALSE;
        }
    }
    else
    {
        GlobalDecodingContextSpecificData_p->load_chroma_non_intra_quantizer_matrix = FALSE;
    }

    for (index = 0; index < MPEG2_Q_MATRIX_SIZE; index ++)
    {
        if (GlobalDecodingContextSpecificData_p->load_chroma_non_intra_quantizer_matrix)
        {
            if (Sequence_H== TRUE)
            GlobalDecodingContextSpecificData_p->chroma_non_intra_quantizer_matrix[index] = gb_GetBits(gbH, 8, NULL);
            else
            PictureSpecificData_p->ChromaNonIntraQuantMatrix[index] = gb_GetBits(gbH, 8, NULL);
        }
        else
        {
            if (Sequence_H== TRUE)
            GlobalDecodingContextSpecificData_p->chroma_non_intra_quantizer_matrix[index] = 16;
            else
            PictureSpecificData_p->ChromaNonIntraQuantMatrix[index] = 16;
        }
    }

    if (Sequence_H== TRUE)
    {
        PictureSpecificData_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *)&(GlobalDecodingContextSpecificData_p->chroma_non_intra_quantizer_matrix[0]);
    }
    else
    {
        PictureSpecificData_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *)&(PictureSpecificData_p->ChromaNonIntraQuantMatrix[0]);
    }
}

/*****************************************************************************
Function:    es_ProcessGOPStart

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessGOPStart (tESState *pESState)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    MPEG2_PictureDecodingContextSpecificData_t *PictureDecodingContextSpecificData_p = (MPEG2_PictureDecodingContextSpecificData_t *)ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->PictureDecodingContextSpecificData_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;
    MPEG2_PictureSpecificData_t *PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STVID_TimeCode_t *timecode = &ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams.TimeCode;
    gb_Handle_t   gbH = pESState->gbHandle;
    U32 value;
    U32 i;
    GlobalDecodingContextSpecificData_p->HasGroupOfPictures = TRUE;
    PictureSpecificData_p->PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */

    /*Drop Frame Flag */
    /*TODO: Where is Drop Frame Flag used?*/
    value = gb_GetBits (gbH, 1, NULL);

    timecode->Hours   = gb_GetBits (gbH, 5, NULL);
    timecode->Minutes = gb_GetBits (gbH, 6, NULL);

/*    Marker Bit check*/
    if (gb_GetBits(gbH, 1, NULL) != 1)
    {
        pESState->error_mask |= ES_error_marker_bit;
    }

    timecode->Seconds  = gb_GetBits (gbH, 6, NULL);
    timecode->Frames   = gb_GetBits (gbH, 6, NULL);
    timecode->Interpolated = FALSE;


    PictureDecodingContextSpecificData_p->closed_gop  = gb_GetBits (gbH, 1, NULL);
    PictureDecodingContextSpecificData_p->broken_link = gb_GetBits (gbH, 1, NULL);

    if (GlobalDecodingContextSpecificData_p->FirstPictureEver == FALSE)
    {
        /*The default values for these are set up the first time the PJR is*/
        /*built. The arrival of the GOP header implies reset of temporal */
        /*reference to 0: update offset with picture counter to continue */
        /*counting up*/
        GlobalDecodingContextSpecificData_p->TemporalReferenceOffset += GlobalDecodingContextSpecificData_p->PictureCounter;
        GlobalDecodingContextSpecificData_p->PictureCounter = 0;
        if ((GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference > 0) &&
            (GlobalDecodingContextSpecificData_p->TemporalReferenceOffset <= GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference))
        {
            /* In case the last calculated ExtendedTemporalReference is higher than
                the offset (ex: cut stream after Pn and Bn-2/Bn-1 missing), push the offset up */
            GlobalDecodingContextSpecificData_p->TemporalReferenceOffset = GlobalDecodingContextSpecificData_p->PreviousGreatestExtendedTemporalReference + 1;
        }
    }
        GlobalDecodingContextSpecificData_p->OneTemporalReferenceOverflowed = FALSE;
        GlobalDecodingContextSpecificData_p->GetSecondReferenceAfterOverflowing = FALSE;
        GlobalDecodingContextSpecificData_p->tempRefReachedZero = FALSE;

    /* If closed_gop set to 1, we do not need any of our reference frames any more, so
    * mark them as no longer valid.  Also mark the 3 reference lists as empty*/
    if(PictureDecodingContextSpecificData_p->closed_gop == 1)
    {
        for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
        {
            ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsValidIndexInReferenceFrame[i] = FALSE;
            ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = FALSE;
        }

        ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
        ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
        ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
        PictureDecodingContextSpecificData_p->GotFirstReferencePicture = TRUE;

    }
    /* If closed_gop=0 and broken_link=1, that means there are missing reference frame.
    * In this case, all current reference pictures must be marked with the broken link flag */
    else if(PictureDecodingContextSpecificData_p->broken_link == 1)
    {
        PictureDecodingContextSpecificData_p->IsBrokenLinkActive = TRUE;
        PictureDecodingContextSpecificData_p->GotFirstReferencePicture = FALSE;
    }
    else
    {
        PictureDecodingContextSpecificData_p->IsBrokenLinkActive = FALSE;
        PictureDecodingContextSpecificData_p->GotFirstReferencePicture = TRUE;
    }
    for (i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS; i++)
    {
        ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = FALSE;
    }
} /* es_ProcessGOPStart */

/*****************************************************************************
Function:    es_ProcessDiscontinuityStart

Parameters:  None

Returns:     None

Scope:

Purpose:

Behavior:

Exceptions:  None

*****************************************************************************/
void es_ProcessDiscontinuityStart (tESState *pESState)
{
    PARSER_ParsingJobResults_t *ParserJobResults_p = pESState->ParserJobResults_p;
    MPEG2_GlobalDecodingContextSpecificData_t *GlobalDecodingContextSpecificData_p = (MPEG2_GlobalDecodingContextSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextSpecificData_p;

    GlobalDecodingContextSpecificData_p->DiscontinuityStartCodeDetected = TRUE;
    GlobalDecodingContextSpecificData_p->TemporalReferenceOffset += TEMPORAL_REFERENCE_MODULO;
} /* es_ProcessDiscontinuityStart */

