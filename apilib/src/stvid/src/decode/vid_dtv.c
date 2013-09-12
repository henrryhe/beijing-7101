/*******************************************************************************

File name   : vid_dtv.c

Description : Directv standard source file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
14 Jun 2000        Created                                           HG
23 Jan 2001        Updated according to Chicago lab's code (MPEG1)   HG
14 May 2001        Now using masks for data process (shifts bugged)  HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <string.h>
#endif

#include "stddefs.h"

#include "vid_dtv.h"

/* Define to add debug info and traces */
/*#define TRACE_DECODE*/

#ifdef TRACE_DECODE
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif
#define DECODE_DEBUG
#endif

/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */

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


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */


/* Private Function prototypes ---------------------------------------------- */


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : ProcessUserDataDirectv
Description : Process information contained in user data for DirecTV
Parameters  : picture user data, structures to update.
              PictureCodingExtension_p should be NULL for MPEG1, not NULL for MPEG2
              PictureDisplayExtension_p should always be given, even in MPEG1. If NULL: not updated.
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void ProcessPictureUserDataDirectv(const UserData_t * const PictureUserData_p, STVID_PTS_t * const PTS_p,
                            PictureCodingExtension_t * const PictureCodingExtension_p,
                            PictureDisplayExtension_t * const PictureDisplayExtension_p)
{
    U8 user_data_length;
    U16 user_data_type;
    /* Data have already been retrieved in PictureUserData_p->Data_p, DataIndex points them */
    U32 DataIndex = 0;
    U8 Tmp8;
    U32 Tmp32;
    S16 TmpS16;
    U8 NumberPreviousPanAndScan = 0;

    if (PictureUserData_p == NULL)
    {
        return;
    }

    /* Process all UserData. As bytes are read at least 3 by 3, check there are 3 bytes remaining. */
    while ((DataIndex + DIRECTV_MIN_NUMBER_BYTES_PER_USER_DATA_TYPE) < PictureUserData_p->UsedSize)
    {
        user_data_length = (PictureUserData_p->Data_p)[DataIndex++];
        user_data_type   = (U16) ((PictureUserData_p->Data_p)[DataIndex++]);
        if (user_data_type == DIRECTV_ESCAPE_TO_EXT_USER_DATA_TYPES)
        {
            user_data_type = (user_data_type << 8) | ((U16) ((PictureUserData_p->Data_p)[DataIndex++]));
        }

        switch(user_data_type)
        {
            case DIRECTV_PRESENTATION_TIME_STAMP :
                Tmp32  = ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0x03)) << 30;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0x7F)) << 23;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0xFF)) << 15;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0x7F)) <<  8;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0xFF));
                /* Put it in 90 kHz range */
                Tmp32 /= 300;
                /* Update PTS if required and if it is completely obtained ! */
                if ((PTS_p != NULL) && /* PTS required */
                    (DataIndex <= PictureUserData_p->UsedSize)) /* DataIndex didn't go outside the available data size. */
                {
                    PTS_p->Interpolated = FALSE;
                    PTS_p->PTS33 = FALSE; /* In Directv PTS is a 32 bit value */
                    PTS_p->PTS = Tmp32;
                }
                break;

            case DIRECTV_DECODE_TIME_STAMP :
                Tmp32  = ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0x03)) << 30;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0x7F)) << 23;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0xFF)) << 15;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0x7F)) <<  8;
                Tmp32 |= ((U32) (((PictureUserData_p->Data_p)[DataIndex++]) & 0xFF));
                /* Put it in 90 kHz range */
                Tmp32 /= 300;
                /* Take it if it is completely obtained ! (DataIndex <= PictureUserData_p->UsedSize) */
                break;

            case DIRECTV_PAN_AND_SCAN :
                /* DIRECTV MPEG-2 Video Bitstream Specification for the IRD V2.1 03/02/00           */
                /* For those IRDs certified before July. 1, 1999, use the P&S in DIRECTV user data. */
                /* For those certified after July. 1, 1999, use MPEG-2.                             */

                /* So takes pan and scan from user data only in MPEG1 */
                if ((PictureCodingExtension_p == NULL) && /* MPEG1 */
                    (PictureDisplayExtension_p != NULL) && /* User wants this info */
                    (NumberPreviousPanAndScan < MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS) && /* Max allowed not reached */
                    ((DataIndex + 2) <= PictureUserData_p->UsedSize) /* Enough data in UserData to get valid information */
                   )
                {
                    /* MPEG1: take pan and scan for horizontal offset */
                    TmpS16  = (S16) (((U16) ((PictureUserData_p->Data_p)[DataIndex++])) << 8);
                    TmpS16 |=       (((U16) ((PictureUserData_p->Data_p)[DataIndex++])) & 0xF0);
                    /* Pan and scan 12bits signed value is in MSB's of TmpS16.
                      Now should divide by 16 to recover value, but we divide by 4 only, to convert value from 4th of pixel to 16th of pixel (standard unit) */
                    TmpS16 /= 4;
                    PictureDisplayExtension_p->FrameCentreOffsets[NumberPreviousPanAndScan].frame_centre_horizontal_offset = TmpS16;
                    if (NumberPreviousPanAndScan == 0)
                    {
                        /* First time: copy values for all horizontal offsets */
                        for (Tmp8 = 1; Tmp8 < MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS; Tmp8++)
                        {
                            PictureDisplayExtension_p->FrameCentreOffsets[Tmp8].frame_centre_horizontal_offset = TmpS16;
                        }
                    }
                    NumberPreviousPanAndScan ++;
                    /* Update number_of_frame_centre_offsets */
                    PictureDisplayExtension_p->number_of_frame_centre_offsets = NumberPreviousPanAndScan;
                }
                else
                {
                    /* Those pan and scan shall be ignored in MPEG2 */
                    DataIndex += 2;
                }
                break;

            case DIRECTV_FIELD_DISPLAY_FLAGS :
                /* Take pan and scan only in MPEG2 */
                if (PictureCodingExtension_p != NULL) /* MPEG2 */
                {
                    Tmp8 = (PictureUserData_p->Data_p)[DataIndex++];

#ifdef TRACE_UART
                    TraceBuffer(("-Having DirecTV FieldDisp=%d", Tmp8));
#endif

                    PictureCodingExtension_p->top_field_first = ((Tmp8 & 0x80) == 0);
                    if (((Tmp8 & 0x70) >> 4) == 0x02)
                    {
                        PictureCodingExtension_p->repeat_first_field = TRUE;
                    }
                    else
                    {
                        PictureCodingExtension_p->repeat_first_field = FALSE;
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
