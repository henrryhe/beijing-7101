/*******************************************************************
               MPEG Video Codec Parser Task Module

            Copyright 2004 Scientific-Atlanta, Inc.

                  Subscriber Engineering
                  5030 Sugarloaf Parkway
                     P. O. Box 465447
                  Lawrenceville, GA 30042

                 Proprietary and Confidential
        Unauthorized distribution or copying is prohibited
                    All rights reserved

 No part of this computer software may be reprinted, reproduced or utilized
 in any form or by any electronic, mechanical, or other means, now known or
 hereafter invented, including photocopying and recording, or using any
 information storage and retrieval system, without permission in writing
 from Scientific Atlanta, Inc.

*******************************************************************

 File Name:    mpg2dtvud.c

 See Also:     mpg2dtvud.h

 Project:      Zeus

 Compiler:     ST40

 Author:       John Bean

 Description:  DirecTV User Data Parsing Code

 Documents:

 Notes:

******************************************************************
 History:
 Rev Level    Date    Name    ECN#    Description
------------------------------------------------------------------
 1.00       01/25/05  JRB             First Release-see vid_dtv.c
******************************************************************/



/*====== includes ======*/
#if !defined(ST_OSLINUX)
#include <string.h>
#endif

#include "stddefs.h"

#include "mpg2parser.h"

/* Define to add debug info and traces */
/*#define TRACE_DECODE*/

#ifdef TRACE_DECODE
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif
#define DECODE_DEBUG
#endif


/*====== file info ======*/


/*====== defines ======*/

/* Min length in bytes of meaningful DirecTV UserData information: one byte user_data_length, one byte user_data_type, and some data ! */
#define DIRECTV_MIN_NUMBER_BYTES_PER_USER_DATA_TYPE 3

/* Directv user data types definition */
#define DIRECTV_PRESENTATION_TIME_STAMP         0x02
#define DIRECTV_DECODE_TIME_STAMP               0x04
#define DIRECTV_CHROMA_FLAGS                    0x05
#define DIRECTV_PAN_AND_SCAN                    0x06
#define DIRECTV_FIELD_DISPLAY_FLAGS             0x07
#define DIRECTV_NO_BURST                        0x08
#define DIRECTV_CLOSED_CAPTION                  0x09
#define DIRECTV_EXTENDED_DATA_SERVICES          0x0a
#define DIRECTV_ESCAPE_TO_EXT_USER_DATA_TYPES   0xff


/* ====== enums ======*/


/* ====== typedefs ======*/


/* ====== globals ======*/



/* ====== statics ======*/




/* ====== prototypes ======*/




/*******************************************************************
Function:    mpg2par_DirecTVUserData

Parameters:  ParserHandle

Returns:     None

Scope:       Private to parser

Purpose:     Parses DirecTV encoded User Data

Behavior:

Exceptions:  None

*******************************************************************/

void mpg2par_DirecTVUserData(const PARSER_Handle_t ParserHandle)
{
    PARSER_Properties_t         * PARSER_Data_p = (PARSER_Properties_t *) ParserHandle;
    MPG2ParserPrivateData_t     * PrivateData_p = (MPG2ParserPrivateData_t *) PARSER_Data_p->PrivateData_p;
    PARSER_ParsingJobResults_t  * ParserJobResults_p = &PrivateData_p->ParserJobResults;
    STVID_SequenceInfo_t        * SequenceInfo_p = &ParserJobResults_p->ParsedPictureInformation_p->PictureDecodingContext_p->GlobalDecodingContext_p->GlobalDecodingContextGenericData.SequenceInfo;
    STVID_VideoParams_t         * VideoParams_p = (STVID_VideoParams_t *)&ParserJobResults_p->ParsedPictureInformation_p->PictureGenericData.PictureInfos.VideoParams;
    MPEG2_PictureSpecificData_t * PictureSpecificData_p = (MPEG2_PictureSpecificData_t *) ParserJobResults_p->ParsedPictureInformation_p->PictureSpecificData_p;
    STVID_UserData_t                 * PictureUserData_p = &PrivateData_p->UserData;
    U8                          * UserData_p = (U8 *)PictureUserData_p->Buff_p;
    U32 DataIndex = 0;
    U8 user_data_length;
    U16 user_data_type;
    U8 Tmp8;
    U32 Tmp32;
    S16 TmpS16;
    U8 NumberPreviousPanAndScan = 0;


/*   If this an HD MPEG2 stream, treat the UserData as standard UserData*/

    if  ((SequenceInfo_p->MPEGStandard != STVID_MPEG_STANDARD_ISO_IEC_11172) &&
            ( ((SequenceInfo_p->ProfileAndLevelIndication & 0x70) < PROFILE_MAIN) ||
            ((SequenceInfo_p->ProfileAndLevelIndication & 0x0F) < LEVEL_MAIN)))
    {
        return;
    }

    /* Process all UserData. As bytes are read at least 3 by 3, check there are 3 bytes remaining. */
    while ((DataIndex + DIRECTV_MIN_NUMBER_BYTES_PER_USER_DATA_TYPE) < PictureUserData_p->Length)
    {
        user_data_length = UserData_p[DataIndex++];
        user_data_type   = (U16) (UserData_p[DataIndex++]);
        if (user_data_type == DIRECTV_ESCAPE_TO_EXT_USER_DATA_TYPES)
        {
            user_data_type = (user_data_type << 8) | ((U16) (UserData_p[DataIndex++]));
        }

        switch(user_data_type)
        {
            case DIRECTV_PRESENTATION_TIME_STAMP :
                Tmp32  = ((U32) ((UserData_p[DataIndex++]) & 0x03)) << 30;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0x7F)) << 23;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0xFF)) << 15;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0x7F)) <<  8;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0xFF));
                /* Put it in 90 kHz range */
                Tmp32 /= 300;
                /* Update PTS if it is completely obtained ! */
                if (DataIndex <= PictureUserData_p->Length)                 {
                    VideoParams_p->PTS.Interpolated = FALSE;
                    VideoParams_p->PTS.PTS33 = FALSE; /* In Directv PTS is a 32 bit value */
                    VideoParams_p->PTS.PTS = Tmp32;
                    VideoParams_p->PTS.IsValid = TRUE;
                }
                break;

            case DIRECTV_DECODE_TIME_STAMP :
                Tmp32  = ((U32) ((UserData_p[DataIndex++]) & 0x03)) << 30;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0x7F)) << 23;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0xFF)) << 15;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0x7F)) <<  8;
                Tmp32 |= ((U32) ((UserData_p[DataIndex++]) & 0xFF));
                /* Put it in 90 kHz range */
                Tmp32 /= 300;
                /* Take it if it is completely obtained ! (DataIndex <= PictureUserData_p->UsedSize) */
                break;

            case DIRECTV_PAN_AND_SCAN :
                /* DIRECTV MPEG-2 Video Bitstream Specification for the IRD V2.1 03/02/00           */
                /* For those IRDs certified before July. 1, 1999, use the P&S in DIRECTV user data. */
                /* For those certified after July. 1, 1999, use MPEG-2.                             */

                /* So takes pan and scan from user data only in MPEG1 */
                if ((SequenceInfo_p->MPEGStandard == STVID_MPEG_STANDARD_ISO_IEC_11172) && /* MPEG1 */
                    (NumberPreviousPanAndScan < MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS) && /* Max allowed not reached */
                    ((DataIndex + 2) <= PictureUserData_p->Length) /* Enough data in UserData to get valid information */
                   )
                {
                    /* MPEG1: take pan and scan for horizontal offset */
                    TmpS16  = (S16) (((U16) (UserData_p[DataIndex++])) << 8);
                    TmpS16 |=       (((U16) (UserData_p[DataIndex++])) & 0xF0);

                    /* Pan and scan 12bits signed value is in MSB's of TmpS16.
                      Now should divide by 16 to recover value, but we divide by 4 only, to convert value from 4th of pixel to 16th of pixel (standard unit) */
                    TmpS16 /= 4;
                    PictureSpecificData_p->FrameCentreOffsets[NumberPreviousPanAndScan].frame_centre_horizontal_offset = TmpS16;
                    if (NumberPreviousPanAndScan == 0)
                    {
                        /* First time: copy values for all horizontal offsets */
                        for (Tmp8 = 1; Tmp8 < MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS; Tmp8++)
                        {
                            PictureSpecificData_p->FrameCentreOffsets[Tmp8].frame_centre_horizontal_offset = TmpS16;
                        }
                    }
                    NumberPreviousPanAndScan ++;
                    /* Update number_of_frame_centre_offsets */
                    PictureSpecificData_p->number_of_frame_centre_offsets = NumberPreviousPanAndScan;
                }
                else
                {
                    /* Those pan and scan shall be ignored in MPEG2 */
                    DataIndex += 2;
                }
                break;

            case DIRECTV_FIELD_DISPLAY_FLAGS :
                /* Take pan and scan only in MPEG2 */
                if ((SequenceInfo_p->MPEGStandard != STVID_MPEG_STANDARD_ISO_IEC_11172)) /* MPEG2 */
                {
                    Tmp8 = UserData_p[DataIndex++];

#ifdef TRACE_UART
                    TraceBuffer(("-Having DirecTV FieldDisp=%d", Tmp8));
#endif

                    VideoParams_p->TopFieldFirst = ((Tmp8 & 0x80) == 0);
                    if (((Tmp8 & 0x70) >> 4) == 0x02)
                    {
                        PictureSpecificData_p->repeat_first_field = TRUE;
                    }
                    else
                    {
                        PictureSpecificData_p->repeat_first_field = FALSE;
                    }
                }
                else
                {
                    /* Those field display shall be ignored in MPEG1 */
                    DataIndex ++;
                }
                break;

            case DIRECTV_CLOSED_CAPTION :
                /* !!! Not exploited now */
                DataIndex += 2;
                /* Take it if it is completely obtained ! (DataIndex <= PictureUserData_p->UsedSize) */
                break;

            case DIRECTV_EXTENDED_DATA_SERVICES :
                /* !!! Not exploited now */
                DataIndex += 2;
                /* Take it if it is completely obtained ! (DataIndex <= PictureUserData_p->UsedSize) */
                break;

            default:
                /* Unknown data, junk: skip (user_data_length - 1) bytes of data to find the next data */
                if (user_data_length > 1)
                {
                    DataIndex += user_data_length - 1;
                }
                break;

        } /* end switch(user_data_type) */
    } /* end while data */
} /* End of ProcessUserDataDirectv() function */

/* End of vid_dtv.c */
