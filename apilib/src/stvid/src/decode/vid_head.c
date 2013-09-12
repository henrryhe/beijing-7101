/*******************************************************************************

File name   : vid_head.c

Description : MPEG headers parsing source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
22 Feb 2000        Created                                          HG
 6 Feb 2002        Out of GetUserData() if underflow (not blocked)  HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */


/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#include <limits.h>
#endif

#include "stddefs.h"
#include "stos.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"
#endif /* def STVID_STVAPI_ARCHITECTURE */


#ifdef STVID_MEASURES
#include "measures.h"
#endif

#include "vid_mpeg.h"
#include "vid_head.h"
#include "vid_dec.h"
#include "vid_bits.h"
#include "halv_dec.h"
#include "sttbx.h"


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */


/* Private Variables (static)------------------------------------------------ */


/* Global Variables --------------------------------------------------------- */

#ifdef STVID_MEASURES
U32 MeasuresSC[NB_RAWS][NB_OF_MEASURES];
U32 MeasureIndexSC = 0;
extern clock_t BeginInterrupt;
#endif


/* Private Macros ----------------------------------------------------------- */

/* To have control of number of bits retrieved ... */
#ifdef STVID_CONTROL_HEADER_BITS
#define HeaderDataResetBitsCounter(HeaderData_p) viddec_HeaderDataResetBitsCounter(HeaderData_p)
#define HeaderDataGetBitsCounter(HeaderData_p) viddec_HeaderDataGetBitsCounter(HeaderData_p)
#else
/* ... or not ! */
#define HeaderDataResetBitsCounter(HeaderData_p)
#define HeaderDataGetBitsCounter(HeaderData_p) 0
#endif

/* Private Function prototypes ---------------------------------------------- */

/* For all: DecodeHandle would be enough !!! */
static U32 GetSequence(HeaderData_t * const HeaderData_p, Sequence_t * const Sequence_p, BOOL * const HasBadMarkerBit_p);
static U32 GetUserData(HeaderData_t * const HeaderData_p, UserData_t * const UserData_p, U8 * const FoundStartCode_p);
static U32 GetSequenceExtension(HeaderData_t * const HeaderData_p, SequenceExtension_t * const SequenceExtension_p, BOOL * const HasBadMarkerBit_p);
static U32 GetSequenceDisplayExtension(HeaderData_t * const HeaderData_p, SequenceDisplayExtension_t * const SequenceDisplayExtension_p, BOOL * const HasBadMarkerBit_p);
static U32 GetSequenceScalableExtension(HeaderData_t * const HeaderData_p, SequenceScalableExtension_t * const SequenceScalableExtension_p, BOOL * const HasBadMarkerBit_p);
static U32 GetGroupOfPictures(HeaderData_t * const HeaderData_p, GroupOfPictures_t * const GroupOfPictures_p, BOOL * const HasBadMarkerBit_p);
static U32 GetPicture(HeaderData_t * const HeaderData_p, Picture_t * const Picture_p);
static U32 GetPictureCodingExtension(HeaderData_t * const HeaderData_p, PictureCodingExtension_t * const PictureCodingExtension_p);
static U32 GetQuantMatrixExtension(HeaderData_t * const HeaderData_p, QuantMatrixExtension_t * const QuantMatrixExtension_p);
static U32 GetCopyrightExtension(HeaderData_t * const HeaderData_p, CopyrightExtension_t * const CopyrightExtension_p, BOOL * const HasBadMarkerBit_p);
static U32 GetPictureDisplayExtension(HeaderData_t * const HeaderData_p,
    PictureDisplayExtension_t *      const PictureDisplayExtension_p,
    const PictureCodingExtension_t * const PictureCodingExtension_p,
    const SequenceExtension_t *      const SequenceExtension_p,
    BOOL * const HasBadMarkerBit_p
);
static U32 GetPictureTemporalScalableExtension(HeaderData_t * const HeaderData_p, PictureTemporalScalableExtension_t * const PictureTemporalScalableExtension_p, BOOL * const HasBadMarkerBit_p);
static U32 GetPictureSpatialScalableExtension(HeaderData_t * const HeaderData_p, PictureSpatialScalableExtension_t * const PictureSpatialScalableExtension_p, BOOL * const HasBadMarkerBit_p);


/* Functions ---------------------------------------------------------------- */


/*******************************************************************************
Name        : viddec_ResetStream
Description : Reset stream structure variable as if there were no stream (expecting a sequence SC)
Parameters  : pointer on stream
Assumptions :
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_ResetStream(MPEG2BitStream_t * const Stream_p)
{
    if (Stream_p == NULL)
    {
        /* Error case */
        return;
    }

    Stream_p->BitStreamProgress             = NO_BITSTREAM;
    Stream_p->HasSequence                   = FALSE;
    Stream_p->HasSequenceDisplay            = FALSE;
    Stream_p->IsScalable                    = FALSE;
    Stream_p->HasGroupOfPictures            = FALSE;
    Stream_p->HasQuantMatrixExtension       = FALSE;
    Stream_p->HasCopyrightExtension         = FALSE;
    Stream_p->HasPictureSpatialScalableExtension = FALSE;
    Stream_p->HasPictureTemporalScalableExtension = FALSE;

    Stream_p->LumaIntraQuantMatrix_p       = Stream_p->DefaultIntraQuantMatrix_p;
    Stream_p->LumaNonIntraQuantMatrix_p    = Stream_p->DefaultNonIntraQuantMatrix_p;
    Stream_p->ChromaIntraQuantMatrix_p     = Stream_p->DefaultIntraQuantMatrix_p;
    Stream_p->ChromaNonIntraQuantMatrix_p  = Stream_p->DefaultNonIntraQuantMatrix_p;
    Stream_p->IntraQuantMatrixHasChanged   = TRUE;
    Stream_p->NonIntraQuantMatrixHasChanged = TRUE;

/*    Stream_p->SequenceUserData.UsedSize         = 0;*/
/*    Stream_p->GroupOfPicturesUserData.UsedSize  = 0;*/
/*    Stream_p->PictureUserData.UsedSize          = 0;*/
    Stream_p->UserData.UsedSize          = 0;

} /* End of viddec_ResetStream() function */


/*******************************************************************************
Name        : viddec_FoundStartCode
Description : React when a new start code was found
Parameters  : decoder pointer
Assumptions : Decoder pointer is not NULL
Limitations :
Returns     : Nothing
*******************************************************************************/
void viddec_FoundStartCode(const VIDDEC_Handle_t DecodeHandle, const U8 StartCode)
{
    BOOL FirstPictureStartCodeFound = FALSE;

#ifdef STVID_MEASURES
    MeasuresP[MEASURE_BEGIN][MeasureIndexP] = time_now();
#endif

    /* Push start code into start code commands queue */
    PushStartCodeCommand(DecodeHandle, StartCode);
    semaphore_signal(((VIDDEC_Data_t *) DecodeHandle)->DecodeOrder_p);

    /* Inform the HAL of the found startcode. Done here because of the manualy parsing  */
    /* of the User Data where the HAL doesn't see the start code.                       */
    HALDEC_SetFoundStartCode(((VIDDEC_Data_t *) DecodeHandle)->HALDecodeHandle, StartCode, &FirstPictureStartCodeFound);

    /* FirstPictureStartCodeFound should be TRUE for decoder cells whose VLD pointer does not go to the */
    /* first picture start code, after the Soft Reset. Thus, only for the first Decode, the NEXT_FOUND_PICTURE */
    /* state has to be forced */
    if (FirstPictureStartCodeFound)
    {
        viddec_HWDecoderFoundNextPicture(DecodeHandle);
    }

#ifdef STVID_MEASURES
    MeasuresP[MEASURE_END][MeasureIndexP] = time_now();
    MeasuresP[MEASURE_ID][MeasureIndexP]  = MEASURE_ID_POST_START_CODE;
    if (MeasureIndexP < NB_OF_MEASURES - 1)
    {
        MeasureIndexP++;
    }
#endif
} /* End of viddec_FoundStartCode() function */

#ifdef ST_SecondInstanceSharingTheSameDecoder
/*******************************************************************************
Name        : viddec_ParseStartCodeForStill
Description : parse MPEG header data
Parameters  : decoder handle, start code just obtained, stream structure to fill and update with header data, parsing mode
Assumptions : Start code was just obtained from viddec_HeaderDataGetBits(8), next header data is ready
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_NOT_AVAILABLE if an error occured (start code data may be missing)
*******************************************************************************/
ST_ErrorCode_t viddec_ParseStartCodeForStill(HeaderData_t * const HeaderData_p, const U8 StartCode, MPEG2BitStream_t * const Stream_p, const ParseMode_t ParseMode)
{
    U32 k;
    StartCodeParsing_t StartCodeParsing ;

    if (Stream_p == NULL)
    {
        /* Error case */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Stream analyser needs first to find sequence start code to start parsing data    */
    /* whereas Stream parser parses data without any other constraint                   */
    if (   (ParseMode == PARSER_MODE_STREAM_ANALYSER)
        && (Stream_p->BitStreamProgress == NO_BITSTREAM)
        && (StartCode != USER_DATA_START_CODE)
        && (StartCode != SEQUENCE_HEADER_CODE))
    {
        /* Expecting first sequence but don't have it: skip start code */
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* Inform program that start code parsing is on going */
    HeaderData_p->IsHeaderDataSpoiled = FALSE;
    StartCodeParsing.IsOK = TRUE;
    StartCodeParsing.HasBadMarkerBit = FALSE;
    StartCodeParsing.IsOnGoing = TRUE;

#ifdef STVID_MEASURES
    MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = time_now();
#endif

    switch (StartCode)
    {
        case SEQUENCE_HEADER_CODE:
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
            /* Start measure of sequence header decoding */
/*            MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = BeginInterrupt;*/
            MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE;
#endif
            GetSequence(HeaderData_p, &(Stream_p->Sequence), &(StartCodeParsing.HasBadMarkerBit));

            /* Point to the right matrix after a new sequence: default if none was loaded, loaded one otherwise */
            if (Stream_p->Sequence.load_intra_quantizer_matrix)
            {
                /* Matrix were loaded */
                Stream_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.intra_quantizer_matrix);
                Stream_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.intra_quantizer_matrix);
                Stream_p->IntraQuantMatrixHasChanged = TRUE;
            }
            else
            {
                /* No matrix loaded: default ones */
                if ((Stream_p->LumaIntraQuantMatrix_p != Stream_p->DefaultIntraQuantMatrix_p) ||
                    (Stream_p->ChromaIntraQuantMatrix_p != Stream_p->DefaultIntraQuantMatrix_p))
                {
                    Stream_p->LumaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
                    Stream_p->ChromaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
                    Stream_p->IntraQuantMatrixHasChanged = TRUE;
                }
            }
            if (Stream_p->Sequence.load_non_intra_quantizer_matrix)
            {
                /* Matrix were loaded */
                Stream_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.non_intra_quantizer_matrix);
                Stream_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.non_intra_quantizer_matrix);
                Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
            }
            else
            {
                /* No matrix loaded: default ones */
                if ((Stream_p->LumaNonIntraQuantMatrix_p != Stream_p->DefaultNonIntraQuantMatrix_p) ||
                    (Stream_p->ChromaNonIntraQuantMatrix_p != Stream_p->DefaultNonIntraQuantMatrix_p))
                {
                    Stream_p->LumaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
                    Stream_p->ChromaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
                    Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
                }
            }

            /* Reset stream variables */
            Stream_p->HasGroupOfPictures = FALSE;

            /* picture_display_extension is considered always present by MPEG2:
            reset the values at each sequence_header. */
            for (k = 0; k < MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS; k++)
            {
                Stream_p->PictureDisplayExtension.FrameCentreOffsets[k].frame_centre_horizontal_offset = 0;
                Stream_p->PictureDisplayExtension.FrameCentreOffsets[k].frame_centre_vertical_offset   = 0;
            }
            Stream_p->PictureDisplayExtension.number_of_frame_centre_offsets = 1;

            /* Change bit stream progress follower */
            if (Stream_p->BitStreamProgress == NO_BITSTREAM)
            {
                Stream_p->BitStreamProgress = AFTER_FIRST_SEQUENCE_HEADER;
                /* After first sequence, we don't know if stream if MPEG1 or MPEG2.
                This will be determined when the next start code will arrive: if next SC
                is sequence_extension, this will be MPEG2, otherwise, this will be MPEG1.
                Until next start code arrives, let's consider the stream is MPEG1. */
                Stream_p->MPEG1BitStream = TRUE;
                /* First sequence is now available */
                Stream_p->HasSequence = TRUE;
                /* Sequence display from last sequence is not valid any more */
                Stream_p->HasSequenceDisplay = FALSE;
                /* Sequence scalable extension from last sequence is not valid any more */
                Stream_p->IsScalable = FALSE;
            }
            else
            {
                if (!(Stream_p->MPEG1BitStream))
                {
                    /* MPEG-2 */
                    Stream_p->BitStreamProgress = AFTER_REPEAT_SEQUENCE_HEADER;
                }
                else
                {
                    /* MPEG-1 */
                    Stream_p->BitStreamProgress = AFTER_MPEG1_REPEAT_SEQUENCE_HEADER;
                }
            }
            break;

        case EXTENSION_START_CODE:
            {
                U8 extension_start_code_identifier;

                /* Get start code identifier */
                extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);

                /* If we have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER, we don't know yet
                   if stream if MPEG-1 or MPEG-2. This will be determined now. (Although the stream was assumed to be MPEG1).
                   If we have BitStreamProgress < AFTER_SEQUENCE_EXTENSION, the sequence may change from MPEG-2 to MPEG-1 */
                if ((Stream_p->BitStreamProgress < AFTER_SEQUENCE_EXTENSION) && (extension_start_code_identifier == SEQUENCE_EXTENSION_ID))
                {
                    /* sequence_extension is following the first (or any) sequence_header: Stream is MPEG2 */
                    Stream_p->MPEG1BitStream = FALSE;
                }

                /* If MPEG1, we should have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_MPEG1_REPEAT_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER
                                         or BitStreamProgress == AFTER_PICTURE_HEADER */
                /* If MPEG2, we should have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_SEQUENCE_EXTENSION
                                         or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER
                                         or BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                if (Stream_p->MPEG1BitStream)
                {
                    /* MPEG1: extension data are reserved, discard all subsequent data until the next start code */
                    break;
                }

                /* Extensions below exist only in MPEG2, they don't exist in MPEG1 */
                switch (extension_start_code_identifier)
                {
                    case SEQUENCE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER
                                       or BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER */
                        GetSequenceExtension(HeaderData_p, &(Stream_p->SequenceExtension), &(StartCodeParsing.HasBadMarkerBit));
                        Stream_p->BitStreamProgress = AFTER_SEQUENCE_EXTENSION;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE_EXT;
#endif
                        break;

                    case SEQUENCE_DISPLAY_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION */
                        GetSequenceDisplayExtension(HeaderData_p, &(Stream_p->SequenceDisplayExtension), &(StartCodeParsing.HasBadMarkerBit));
                        Stream_p->HasSequenceDisplay = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE_EXT;
#endif
                        break;

                    case SEQUENCE_SCALABLE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION */
                        GetSequenceScalableExtension(HeaderData_p, &(Stream_p->SequenceScalableExtension), &(StartCodeParsing.HasBadMarkerBit));
                        Stream_p->IsScalable = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE_EXT;
#endif
                        break;

                    case PICTURE_CODING_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_HEADER */
                        GetPictureCodingExtension(HeaderData_p, &(Stream_p->PictureCodingExtension));
                        Stream_p->BitStreamProgress = AFTER_PICTURE_CODING_EXTENSION;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case QUANT_MATRIX_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetQuantMatrixExtension(HeaderData_p, &(Stream_p->QuantMatrixExtension));

                        /* Point to the right matrix after a new sequence: default if none was loaded, loaded one otherwise */
                        if (Stream_p->QuantMatrixExtension.load_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.intra_quantizer_matrix);
                            Stream_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.intra_quantizer_matrix);
                            Stream_p->IntraQuantMatrixHasChanged = TRUE;
                        }
                        if (Stream_p->QuantMatrixExtension.load_non_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.non_intra_quantizer_matrix);
                            Stream_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.non_intra_quantizer_matrix);
                            Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
                        }
                        if (Stream_p->QuantMatrixExtension.load_chroma_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.chroma_intra_quantizer_matrix);
                            Stream_p->IntraQuantMatrixHasChanged = TRUE;
                        }
                        if (Stream_p->QuantMatrixExtension.load_chroma_non_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.chroma_non_intra_quantizer_matrix);
                            Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
                        }
                        Stream_p->HasQuantMatrixExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case COPYRIGHT_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetCopyrightExtension(HeaderData_p, &(Stream_p->CopyrightExtension), &(StartCodeParsing.HasBadMarkerBit));
                        Stream_p->HasCopyrightExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case PICTURE_DISPLAY_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetPictureDisplayExtension(HeaderData_p, &(Stream_p->PictureDisplayExtension),
                                                   &(Stream_p->PictureCodingExtension), &(Stream_p->SequenceExtension), &(StartCodeParsing.HasBadMarkerBit));
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetPictureSpatialScalableExtension(HeaderData_p, &(Stream_p->PictureSpatialScalableExtension), &(StartCodeParsing.HasBadMarkerBit));
                        Stream_p->HasPictureSpatialScalableExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetPictureTemporalScalableExtension(HeaderData_p, &(Stream_p->PictureTemporalScalableExtension), &(StartCodeParsing.HasBadMarkerBit));
                        Stream_p->HasPictureTemporalScalableExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    default :
                        /* Reserved values: discard all subsequent data until the next start code */
                        break;
                } /* end of switch */
            }
            break;

        case GROUP_START_CODE:
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
            /* Start measure of group header decoding */
/*            MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = BeginInterrupt;*/
            MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_GROUP;
#endif
            /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION
                           or BitStreamProgress == AFTER_PICTURE_DATA
                           or BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER  (only if MPEG1)
                           or BitStreamProgress == AFTER_MPEG1_REPEAT_SEQUENCE_HEADER  (only if MPEG1) */
            if (Stream_p->BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER)
            {
                /* GOP following directly the sequence in MPEG-2: impossible, this must be an MPEG-1 stream */
                Stream_p->MPEG1BitStream = TRUE;
                Stream_p->HasSequenceDisplay = FALSE;
                /* Sequence scalable extension from last sequence is not valid any more */
                Stream_p->IsScalable = FALSE;
            }

            GetGroupOfPictures(HeaderData_p, &(Stream_p->GroupOfPictures), &(StartCodeParsing.HasBadMarkerBit));
            Stream_p->HasGroupOfPictures = TRUE;
            Stream_p->BitStreamProgress = AFTER_GROUP_OF_PICTURES_HEADER;
            break;

        case PICTURE_START_CODE:
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
            /* Start measure of picture header decoding */
/*            MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = BeginInterrupt;*/
            MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE;
#endif
            /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION (only if MPEG-2)
                           or BitStreamProgress == AFTER_PICTURE_DATA
                           or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER */
            if (Stream_p->BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER)
            {
                /* Picture following directly the sequence in MPEG-2: impossible, this must be an MPEG-1 stream (with missing GOP) */
                Stream_p->MPEG1BitStream = TRUE;
                Stream_p->HasSequenceDisplay = FALSE;
                /* Sequence scalable extension from last sequence is not valid any more */
                Stream_p->IsScalable = FALSE;
            }

            GetPicture(HeaderData_p, &(Stream_p->Picture));

            Stream_p->HasQuantMatrixExtension = FALSE;
            Stream_p->HasCopyrightExtension = FALSE;
            Stream_p->HasPictureSpatialScalableExtension = FALSE;
            Stream_p->HasPictureTemporalScalableExtension = FALSE;

            Stream_p->BitStreamProgress = AFTER_PICTURE_HEADER;
            break;

        case USER_DATA_START_CODE:
             /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION
                           or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER
                           or BitStreamProgress == AFTER_PICTURE_HEADER (only if MPEG1)
                           or BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION (only if MPEG2)
                           or BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER (only if MPEG1)
                           or BitStreamProgress == AFTER_MPEG1_REPEAT_SEQUENCE_HEADER (only if MPEG1) */

            /* Inform interrupt handler that the start code was found manually */
            StartCodeParsing.IsGettingUserData = TRUE;
            StartCodeParsing.HasUserDataProcessTriggeredStartCode = TRUE;

            /* Don't get user data for still, not required. */
/*            GetUserData(HeaderData_p, &(Stream_p->UserData), &(((VIDDEC_Data_t *) DecodeHandle)->LastStartCode));*/

            StartCodeParsing.IsGettingUserData = FALSE;

            /* BitStreamProgress remains the same */
            break;

        case SEQUENCE_END_CODE:
            /* We should have BitStreamProgress == AFTER_PICTURE_DATA */
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
#endif
            viddec_ResetStream(Stream_p);
            break;

        case SEQUENCE_ERROR_CODE:
            /* The sequence_error_code shall be discarded. It has been allocated for use
            by a media interface to indicate where uncorrectable errors have been detected. */
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
#endif
            break;

        default:
            /* start code slice, below picture or non video start code */
            if ((StartCode <= GREATEST_SLICE_START_CODE) && (StartCode >= FIRST_SLICE_START_CODE))
            {
                /* Slice start codes: consider invalid if not coming after picture: they correspond to nothing ! */
                if (Stream_p->BitStreamProgress >= AFTER_PICTURE_HEADER)
                {
                    Stream_p->BitStreamProgress = AFTER_PICTURE_DATA;
                }
            }
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
#endif
            break;
    } /* end of switch StartCode */

    /* Check if there were problem getting header data (then consider the parsing is not OK) */
    if (HeaderData_p->IsHeaderDataSpoiled)
    {
        StartCodeParsing.IsOK = FALSE;
    }

#ifdef STVID_MEASURES
    /* Always measure end of start code processing */
    MeasuresSC[MEASURE_END][MeasureIndexSC] = time_now();
    if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&
        (MeasureIndexSC < NB_OF_MEASURES - 1))
    {
        MeasureIndexSC++;
    }
#endif

    /* Inform program that start code parsing is finished */
    StartCodeParsing.IsOnGoing = FALSE;
    if ((!(StartCodeParsing.IsOK)) || /* error */
        (StartCodeParsing.HasBadMarkerBit)) /* bad marker bit */

    {
        /* Error while parsing: return error */
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    return(ST_NO_ERROR);
} /* End of viddec_ParseStartCodeForStill() function */
#endif /* ST_SecondInstanceSharingTheSameDecoder */

/*******************************************************************************
Name        : viddec_ParseStartCode
Description : parse MPEG header data
Parameters  : decoder handle, start code just obtained, stream structure to fill and update with header data, parsing mode
Assumptions : Start code was just obtained from viddec_HeaderDataGetBits(8), next header data is ready
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
              STVID_ERROR_NOT_AVAILABLE if an error occured (start code data may be missing)
*******************************************************************************/
ST_ErrorCode_t viddec_ParseStartCode(const VIDDEC_Handle_t DecodeHandle, const U8 StartCode, MPEG2BitStream_t * const Stream_p, const ParseMode_t ParseMode)
{
    U32 k;
    VIDDEC_Data_t  * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    /* Choose here to which set of structures to apply the start code parsing */
    BOOL *          HasBadMarkerBit_p   = &(((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.HasBadMarkerBit);
    HeaderData_t *  HeaderData_p        = &(((VIDDEC_Data_t *) DecodeHandle)->HeaderData);

    if (Stream_p == NULL)
    {
        /* Error case */
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Stream analyser needs first to find sequence start code to start parsing data    */
    /* whereas Stream parser parses data without any other constraint                   */
    if (   (ParseMode == PARSER_MODE_STREAM_ANALYSER)
        && (Stream_p->BitStreamProgress == NO_BITSTREAM)
        && (StartCode != USER_DATA_START_CODE)
        && (StartCode != SEQUENCE_HEADER_CODE))
    {
        /* Expecting first sequence but don't have it: skip start code */
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    /* Inform program that start code parsing is on going */
    HeaderData_p->IsHeaderDataSpoiled = FALSE;
    ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOK = TRUE;
    ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.HasBadMarkerBit = FALSE;
    ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOnGoing = TRUE;

#ifdef STVID_MEASURES
    MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = time_now();
#endif

#ifdef ST_smoothbackward
    /* Initialize flag */
    VIDDEC_Data_p->RequiredSearch.StartCodeWasFoundManually = FALSE;
#endif /* ST_smoothbackward */
    
    switch (StartCode)
    {
        case SEQUENCE_HEADER_CODE:
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
            /* Start measure of sequence header decoding */
/*            MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = BeginInterrupt;*/
            MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE;
#endif
            GetSequence(HeaderData_p, &(Stream_p->Sequence), HasBadMarkerBit_p);

            /* Point to the right matrix after a new sequence: default if none was loaded, loaded one otherwise */
            if (Stream_p->Sequence.load_intra_quantizer_matrix)
            {
                /* Matrix were loaded */
                Stream_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.intra_quantizer_matrix);
                Stream_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.intra_quantizer_matrix);
                Stream_p->IntraQuantMatrixHasChanged = TRUE;
            }
            else
            {
                /* No matrix loaded: default ones */
                if ((Stream_p->LumaIntraQuantMatrix_p != Stream_p->DefaultIntraQuantMatrix_p) ||
                    (Stream_p->ChromaIntraQuantMatrix_p != Stream_p->DefaultIntraQuantMatrix_p))
                {
                    Stream_p->LumaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
                    Stream_p->ChromaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
                    Stream_p->IntraQuantMatrixHasChanged = TRUE;
                }
            }
            if (Stream_p->Sequence.load_non_intra_quantizer_matrix)
            {
                /* Matrix were loaded */
                Stream_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.non_intra_quantizer_matrix);
                Stream_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->Sequence.non_intra_quantizer_matrix);
                Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
            }
            else
            {
                /* No matrix loaded: default ones */
                if ((Stream_p->LumaNonIntraQuantMatrix_p != Stream_p->DefaultNonIntraQuantMatrix_p) ||
                    (Stream_p->ChromaNonIntraQuantMatrix_p != Stream_p->DefaultNonIntraQuantMatrix_p))
                {
                    Stream_p->LumaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
                    Stream_p->ChromaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
                    Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
                }
            }

            /* Reset stream variables */
            Stream_p->HasGroupOfPictures = FALSE;

            /* picture_display_extension is considered always present by MPEG2:
            reset the values at each sequence_header. */
            for (k = 0; k < MAX_NUMBER_OF_FRAME_CENTRE_OFFSETS; k++)
            {
                Stream_p->PictureDisplayExtension.FrameCentreOffsets[k].frame_centre_horizontal_offset = 0;
                Stream_p->PictureDisplayExtension.FrameCentreOffsets[k].frame_centre_vertical_offset   = 0;
            }
            Stream_p->PictureDisplayExtension.number_of_frame_centre_offsets = 1;

            /* Change bit stream progress follower */
            if (Stream_p->BitStreamProgress == NO_BITSTREAM)
            {
                Stream_p->BitStreamProgress = AFTER_FIRST_SEQUENCE_HEADER;
                /* After first sequence, we don't know if stream if MPEG1 or MPEG2.
                This will be determined when the next start code will arrive: if next SC
                is sequence_extension, this will be MPEG2, otherwise, this will be MPEG1.
                Until next start code arrives, let's consider the stream is MPEG1. */
                Stream_p->MPEG1BitStream = TRUE;
                /* First sequence is now available */
                Stream_p->HasSequence = TRUE;
                /* Sequence display from last sequence is not valid any more */
                Stream_p->HasSequenceDisplay = FALSE;
                /* Sequence scalable extension from last sequence is not valid any more */
                Stream_p->IsScalable = FALSE;
            }
            else
            {
                if (!(Stream_p->MPEG1BitStream))
                {
                    /* MPEG-2 */
                    Stream_p->BitStreamProgress = AFTER_REPEAT_SEQUENCE_HEADER;
                }
                else
                {
                    /* MPEG-1 */
                    Stream_p->BitStreamProgress = AFTER_MPEG1_REPEAT_SEQUENCE_HEADER;
                }
            }
            break;

        case EXTENSION_START_CODE:
            {
                U8 extension_start_code_identifier;

                /* Get start code identifier */
                extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);

                /* If we have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER, we don't know yet
                   if stream if MPEG-1 or MPEG-2. This will be determined now. (Although the stream was assumed to be MPEG1).
                   If we have BitStreamProgress < AFTER_SEQUENCE_EXTENSION, the sequence may change from MPEG-2 to MPEG-1 */
                if ((Stream_p->BitStreamProgress < AFTER_SEQUENCE_EXTENSION) && (extension_start_code_identifier == SEQUENCE_EXTENSION_ID))
                {
                    /* sequence_extension is following the first (or any) sequence_header: Stream is MPEG2 */
                    Stream_p->MPEG1BitStream = FALSE;
                }

                /* If MPEG1, we should have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_MPEG1_REPEAT_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER
                                         or BitStreamProgress == AFTER_PICTURE_HEADER */
                /* If MPEG2, we should have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER
                                         or BitStreamProgress == AFTER_SEQUENCE_EXTENSION
                                         or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER
                                         or BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                if (Stream_p->MPEG1BitStream)
                {
                    /* MPEG1: extension data are reserved, discard all subsequent data until the next start code */
                    break;
                }

                /* Extensions below exist only in MPEG2, they don't exist in MPEG1 */
                switch (extension_start_code_identifier)
                {
                    case SEQUENCE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER
                                       or BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER */
                        GetSequenceExtension(HeaderData_p, &(Stream_p->SequenceExtension), HasBadMarkerBit_p);
                        Stream_p->BitStreamProgress = AFTER_SEQUENCE_EXTENSION;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE_EXT;
#endif
                        break;

                    case SEQUENCE_DISPLAY_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION */
                        GetSequenceDisplayExtension(HeaderData_p, &(Stream_p->SequenceDisplayExtension), HasBadMarkerBit_p);
                        Stream_p->HasSequenceDisplay = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE_EXT;
#endif
                        break;

                    case SEQUENCE_SCALABLE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION */
                        GetSequenceScalableExtension(HeaderData_p, &(Stream_p->SequenceScalableExtension), HasBadMarkerBit_p);
                        Stream_p->IsScalable = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_SEQUENCE_EXT;
#endif
                        break;

                    case PICTURE_CODING_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_HEADER */
                        GetPictureCodingExtension(HeaderData_p, &(Stream_p->PictureCodingExtension));
                        Stream_p->BitStreamProgress = AFTER_PICTURE_CODING_EXTENSION;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case QUANT_MATRIX_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetQuantMatrixExtension(HeaderData_p, &(Stream_p->QuantMatrixExtension));

                        /* Point to the right matrix after a new sequence: default if none was loaded, loaded one otherwise */
                        if (Stream_p->QuantMatrixExtension.load_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.intra_quantizer_matrix);
                            Stream_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.intra_quantizer_matrix);
                            Stream_p->IntraQuantMatrixHasChanged = TRUE;
                        }
                        if (Stream_p->QuantMatrixExtension.load_non_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.non_intra_quantizer_matrix);
                            Stream_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.non_intra_quantizer_matrix);
                            Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
                        }
                        if (Stream_p->QuantMatrixExtension.load_chroma_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.chroma_intra_quantizer_matrix);
                            Stream_p->IntraQuantMatrixHasChanged = TRUE;
                        }
                        if (Stream_p->QuantMatrixExtension.load_chroma_non_intra_quantizer_matrix)
                        {
                            /* Matrix were loaded */
                            Stream_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(Stream_p->QuantMatrixExtension.chroma_non_intra_quantizer_matrix);
                            Stream_p->NonIntraQuantMatrixHasChanged = TRUE;
                        }
                        Stream_p->HasQuantMatrixExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case COPYRIGHT_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetCopyrightExtension(HeaderData_p, &(Stream_p->CopyrightExtension), HasBadMarkerBit_p);
                        Stream_p->HasCopyrightExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case PICTURE_DISPLAY_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetPictureDisplayExtension(HeaderData_p, &(Stream_p->PictureDisplayExtension),
                                                   &(Stream_p->PictureCodingExtension), &(Stream_p->SequenceExtension), HasBadMarkerBit_p);
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetPictureSpatialScalableExtension(HeaderData_p, &(Stream_p->PictureSpatialScalableExtension), HasBadMarkerBit_p);
                        Stream_p->HasPictureSpatialScalableExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID :
                        /* We should have BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION */
                        GetPictureTemporalScalableExtension(HeaderData_p, &(Stream_p->PictureTemporalScalableExtension), HasBadMarkerBit_p);
                        Stream_p->HasPictureTemporalScalableExtension = TRUE;
#ifdef STVID_MEASURES
                        MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE_EXT;
#endif
                        break;

                    default :
                        /* Reserved values: discard all subsequent data until the next start code */
                        break;
                } /* end of switch */
            }
            break;

        case GROUP_START_CODE:
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
            /* Start measure of group header decoding */
/*            MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = BeginInterrupt;*/
            MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_GROUP;
#endif
            /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION
                           or BitStreamProgress == AFTER_PICTURE_DATA
                           or BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER  (only if MPEG1)
                           or BitStreamProgress == AFTER_MPEG1_REPEAT_SEQUENCE_HEADER  (only if MPEG1) */
            if (Stream_p->BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER)
            {
                /* GOP following directly the sequence in MPEG-2: impossible, this must be an MPEG-1 stream */
                Stream_p->MPEG1BitStream = TRUE;
                Stream_p->HasSequenceDisplay = FALSE;
                /* Sequence scalable extension from last sequence is not valid any more */
                Stream_p->IsScalable = FALSE;
            }

            GetGroupOfPictures(HeaderData_p, &(Stream_p->GroupOfPictures), HasBadMarkerBit_p);
            Stream_p->HasGroupOfPictures = TRUE;
            Stream_p->BitStreamProgress = AFTER_GROUP_OF_PICTURES_HEADER;
            break;

        case PICTURE_START_CODE:
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
            /* Start measure of picture header decoding */
/*            MeasuresSC[MEASURE_BEGIN][MeasureIndexSC] = BeginInterrupt;*/
            MeasuresSC[MEASURE_ID][MeasureIndexSC] = MEASURE_ID_PICTURE;
#endif
            /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION (only if MPEG-2)
                           or BitStreamProgress == AFTER_PICTURE_DATA
                           or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER */
            if (Stream_p->BitStreamProgress == AFTER_REPEAT_SEQUENCE_HEADER)
            {
                /* Picture following directly the sequence in MPEG-2: impossible, this must be an MPEG-1 stream (with missing GOP) */
                Stream_p->MPEG1BitStream = TRUE;
                Stream_p->HasSequenceDisplay = FALSE;
                /* Sequence scalable extension from last sequence is not valid any more */
                Stream_p->IsScalable = FALSE;
            }

            GetPicture(HeaderData_p, &(Stream_p->Picture));

            /* vbv_delay value to be used for video start is the one of first picture after the first sequence, and only this one. */
            if (VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence)
            {
                /* TraceBuffer(("\r\nBitRate= %d \r\n", (U32)((VIDDEC_Data_p->DecodingStream_p)->Sequence.bit_rate_value))); */
                /* TraceBuffer(("vbv_delay= %d \r\n", Picture_p->vbv_delay)); */
                /* TraceBuffer(("VBVBufferSize= %d \r\n", ((VIDDEC_Data_p->DecodingStream_p)->Sequence.vbv_buffer_size_value))); */
                /* TraceBuffer(("B= %d \r\n" , */
                        /* (U32)((VIDDEC_Data_p->DecodingStream_p)->Sequence.bit_rate_value * Picture_p->vbv_delay  / (900 * 1024 * 4)))); */

                /* vbv_delay of first picture after first sequence is parsed. set flag to FALSE */
                /* GNBvd17077 : Flag will be reset after the management of its PTS (if any).    */
                /*  VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence = FALSE;    */

                /* video start must be done with vbv_delay only if different than VARIABLE_BITRATE_STREAMS_VBV_DELAY */
                if (Stream_p->Picture.vbv_delay != VARIABLE_BITRATE_STREAMS_VBV_DELAY)
                {
                    /* the value of vbv_delay is the number of periods of a 90 kHz clock derived from the 27 MHz system clock */
                    VIDDEC_Data_p->VbvStartCondition.StartWithVbvDelay = TRUE;
                    VIDDEC_Data_p->VbvStartCondition.StartWithVbvBufferSize = FALSE;

#ifdef ST_OSLINUX
                    /* WARNING: This fix is a TEMPORARY fix, indeed this linux specific case has to be */
                    /* deleted once a generic way of managing ms to ticks is available */

                    VIDDEC_Data_p->VbvStartCondition.VbvDelayTimerExpiredDate =
                            time_plus(time_now(), (Stream_p->Picture.vbv_delay * ST_GetClocksPerSecond())/STVID_MPEG_CLOCKS_ONE_SECOND_DURATION);
#else
                    VIDDEC_Data_p->VbvStartCondition.VbvDelayTimerExpiredDate = 
                            time_plus(time_now(), Stream_p->Picture.vbv_delay * (ST_GetClocksPerSecond()/STVID_MPEG_CLOCKS_ONE_SECOND_DURATION));
#endif

                    /* Reset threshold to init level */
                    viddec_SetBitBufferThresholdToIdleValue(DecodeHandle);
                    /* Now don't need any more the BBF interrupt: disable it */
                    viddec_UnMaskBitBufferFullInterrupt(DecodeHandle);
                }
            }

            Stream_p->HasQuantMatrixExtension = FALSE;
            Stream_p->HasCopyrightExtension = FALSE;
            Stream_p->HasPictureSpatialScalableExtension = FALSE;
            Stream_p->HasPictureTemporalScalableExtension = FALSE;

            Stream_p->BitStreamProgress = AFTER_PICTURE_HEADER;
            break;

        case USER_DATA_START_CODE:
             /* We should have BitStreamProgress == AFTER_SEQUENCE_EXTENSION
                           or BitStreamProgress == AFTER_GROUP_OF_PICTURES_HEADER
                           or BitStreamProgress == AFTER_PICTURE_HEADER (only if MPEG1)
                           or BitStreamProgress == AFTER_PICTURE_CODING_EXTENSION (only if MPEG2)
                           or BitStreamProgress == AFTER_FIRST_SEQUENCE_HEADER (only if MPEG1)
                           or BitStreamProgress == AFTER_MPEG1_REPEAT_SEQUENCE_HEADER (only if MPEG1) */

            /* Inform interrupt handler that the start code was found manually */
            ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsGettingUserData = TRUE;
            ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.HasUserDataProcessTriggeredStartCode = TRUE;

            GetUserData(HeaderData_p, &(Stream_p->UserData), &(((VIDDEC_Data_t *) DecodeHandle)->LastStartCode));

            ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsGettingUserData = FALSE;
#ifdef ST_smoothbackward
            /* As a start code was obtained manually, treat it as any other start code */
            ((VIDDEC_Data_t *) DecodeHandle)->RequiredSearch.StartCodeWasFoundManually = TRUE;
#endif /* ST_smoothbackward */
            viddec_FoundStartCode(DecodeHandle, ((VIDDEC_Data_t *) DecodeHandle)->LastStartCode);

            /* BitStreamProgress remains the same */
            break;

        case SEQUENCE_END_CODE:
            /* We should have BitStreamProgress == AFTER_PICTURE_DATA */
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
#endif
            viddec_ResetStream(Stream_p);
            break;

        case SEQUENCE_ERROR_CODE:
            /* The sequence_error_code shall be discarded. It has been allocated for use
            by a media interface to indicate where uncorrectable errors have been detected. */
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
#endif
            break;

        default:
            /* start code slice, below picture or non video start code */
            if ((StartCode <= GREATEST_SLICE_START_CODE) && (StartCode >= FIRST_SLICE_START_CODE))
            {
                /* Slice start codes: consider invalid if not coming after picture: they correspond to nothing ! */
                if (Stream_p->BitStreamProgress >= AFTER_PICTURE_HEADER)
                {
                    Stream_p->BitStreamProgress = AFTER_PICTURE_DATA;
                }
            }
#ifdef STVID_MEASURES
            /* Stop last measure and start new one */
/*            if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&*/
/*                (MeasureIndexSC < NB_OF_MEASURES - 1))*/
/*            {*/
/*                MeasureIndexSC++;*/
/*            }*/
#endif
            break;
    } /* end of switch StartCode */

    /* Check if there were problem getting header data (then consider the parsing is not OK) */
    if (HeaderData_p->IsHeaderDataSpoiled)
    {
        ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOK = FALSE;
    }

#ifdef STVID_MEASURES
    /* Always measure end of start code processing */
    MeasuresSC[MEASURE_END][MeasureIndexSC] = time_now();
    if ((MeasuresSC[MEASURE_ID][MeasureIndexSC] != MEASURE_ID_NONE) &&
        (MeasureIndexSC < NB_OF_MEASURES - 1))
    {
        MeasureIndexSC++;
    }
#endif

    /* Inform program that start code parsing is finished */
    ((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOnGoing = FALSE;
    if ((!(((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.IsOK)) || /* error */
        (((VIDDEC_Data_t *) DecodeHandle)->StartCodeParsing.HasBadMarkerBit) /* bad marker bit */
       )
    {
        /* Error while parsing: return error */
        return(STVID_ERROR_NOT_AVAILABLE);
    }

    return(ST_NO_ERROR);
} /* End of viddec_ParseStartCode() function */


/*******************************************************************************
Name        : GetSequence
Description : Get sequence data from MPEG headers data
Parameters  :
Assumptions : Sequence start code was just obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetSequence(HeaderData_t * const HeaderData_p, Sequence_t * const Sequence_p, BOOL * const HasBadMarkerBit_p)
{
    U8 k;

    HeaderDataResetBitsCounter(HeaderData_p);

    Sequence_p->horizontal_size_value       = viddec_HeaderDataGetBits(HeaderData_p, 12);
    Sequence_p->vertical_size_value         = viddec_HeaderDataGetBits(HeaderData_p, 12);
    Sequence_p->aspect_ratio_information    = viddec_HeaderDataGetBits(HeaderData_p, 4);
    Sequence_p->frame_rate_code             = viddec_HeaderDataGetBits(HeaderData_p, 4);
    Sequence_p->bit_rate_value              = viddec_HeaderDataGetBits(HeaderData_p, 18);
    Sequence_p->marker_bit                  = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(Sequence_p->marker_bit))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    Sequence_p->vbv_buffer_size_value       = viddec_HeaderDataGetBits(HeaderData_p, 10);
    Sequence_p->constrained_parameters_flag = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    Sequence_p->load_intra_quantizer_matrix = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (Sequence_p->load_intra_quantizer_matrix)
    {
        for (k = 0; k < QUANTISER_MATRIX_SIZE; k++)
        {
            Sequence_p->intra_quantizer_matrix[k] = viddec_HeaderDataGetBits(HeaderData_p, 8);
        }
    }
    Sequence_p->load_non_intra_quantizer_matrix = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (Sequence_p->load_non_intra_quantizer_matrix)
    {
        for (k = 0; k < QUANTISER_MATRIX_SIZE; k++)
        {
            Sequence_p->non_intra_quantizer_matrix[k] = viddec_HeaderDataGetBits(HeaderData_p, 8);
        }
    }

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetSequence() function */


/*******************************************************************************
Name        : GetUserData
Description : Get user data from MPEG headers data
              Caution: By reading data up to next start code, this function will raise SCH interrupt.
              The consequences of this must be handled by the caller.
Parameters  :
Assumptions : User data start code was just obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : *FoundStartCode_p : start code that we are forced to retrieve to get user data
*******************************************************************************/
static U32 GetUserData(HeaderData_t * const HeaderData_p, UserData_t * const UserData_p, U8 * const FoundStartCode_p)
{
    U8 Tmp8;
    U8 ZeroBytes = 0;
    BOOL StartCodeFound = FALSE;
    U32 BytesSkipped = 0;

    HeaderDataResetBitsCounter(HeaderData_p);

    UserData_p->UsedSize = 0;
    UserData_p->Overflow = FALSE;
    do
    {
        Tmp8 = viddec_HeaderDataGetBits(HeaderData_p, 8);
        if (HeaderData_p->IsHeaderDataSpoiled)
        {
            /* Error in getting start code (underflow): now GetBits() always returns 0,
            so there will never be a 000001, so go out otherwise we are blocked in the do_while(). */
            break; /* End do_while() */
        }
        if (UserData_p->UsedSize < UserData_p->TotalSize)
        {
            /* Store user data if there's enough place in user data buffer */
            (UserData_p->Data_p)[UserData_p->UsedSize] = Tmp8;
            (UserData_p->UsedSize)++;
        }
        else
        {
            BytesSkipped++;
        }

        switch (Tmp8)
        {
            case 0x00 :
                if (ZeroBytes < UCHAR_MAX)
                {
                    ZeroBytes++;
                }
                break;

            case 0x01 :
                if (ZeroBytes >= 2)
                {
                    StartCodeFound = TRUE;
                }
                else
                {
                    ZeroBytes = 0;
                }
                break;

            default :
                ZeroBytes = 0;
        } /* end of switch */
    } while (!(StartCodeFound));

    /* Write start code */
    if (!(HeaderData_p->IsHeaderDataSpoiled))
    {
        /* No error: get next 8 bits, they are the start code found. */
        *FoundStartCode_p = viddec_HeaderDataGetBits(HeaderData_p, 8);

        /* Do not count the start code bytes */
        if (BytesSkipped < 3)
        {
            /* Just skipped one or two first bytes of a start code: they are not data ! */
            UserData_p->UsedSize = UserData_p->UsedSize - 3 + BytesSkipped;
        }
        else if (BytesSkipped > 3)
        {
            /* Skipped more than 3 bytes: lost some data bytes ! */
            UserData_p->Overflow = TRUE;
        }
    }
    else
    {
        /* There has been an error in getting data (either in do_while(), or just
        before while getting 8 bits for start code. If error: return dummy start
        code instead. */

        /* Error in getting data (underflow): return dummy start code instead of 0 ! */
        *FoundStartCode_p = GREATEST_SYSTEM_START_CODE;

        /* We do not know the amount of data fed in user data buffer. Prefer
        consider there's no data inside. */
        UserData_p->UsedSize = 0;
    }

    return(HeaderDataGetBitsCounter(HeaderData_p) - 3 * 8);
} /* end of GetUserData() function */


/*******************************************************************************
Name        : GetSequenceExtension
Description : Get sequence extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and sequence_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetSequenceExtension(HeaderData_t * const HeaderData_p, SequenceExtension_t * const SequenceExtension_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

/*    SequenceExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    SequenceExtension_p->profile_and_level_indication    = viddec_HeaderDataGetBits(HeaderData_p, 8);
    SequenceExtension_p->progressive_sequence            = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    SequenceExtension_p->chroma_format                   = viddec_HeaderDataGetBits(HeaderData_p, 2);
    SequenceExtension_p->horizontal_size_extension       = viddec_HeaderDataGetBits(HeaderData_p, 2);
    SequenceExtension_p->vertical_size_extension         = viddec_HeaderDataGetBits(HeaderData_p, 2);
    SequenceExtension_p->bit_rate_extension              = viddec_HeaderDataGetBits(HeaderData_p, 12);
    SequenceExtension_p->marker_bit                      = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(SequenceExtension_p->marker_bit))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    SequenceExtension_p->vbv_buffer_size_extension       = viddec_HeaderDataGetBits(HeaderData_p, 8);
    SequenceExtension_p->low_delay                       = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    SequenceExtension_p->frame_rate_extension_n          = viddec_HeaderDataGetBits(HeaderData_p, 2);
    SequenceExtension_p->frame_rate_extension_d          = viddec_HeaderDataGetBits(HeaderData_p, 5);

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetSequenceExtension() function */


/*******************************************************************************
Name        : GetSequenceDisplayExtension
Description : Get sequence display extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and sequence_display_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetSequenceDisplayExtension(HeaderData_t * const HeaderData_p, SequenceDisplayExtension_t * const SequenceDisplayExtension_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

/*    SequenceDisplayExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    SequenceDisplayExtension_p->video_format                    = viddec_HeaderDataGetBits(HeaderData_p, 3);
    SequenceDisplayExtension_p->colour_description              = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (SequenceDisplayExtension_p->colour_description)
    {
        SequenceDisplayExtension_p->colour_primaries            = viddec_HeaderDataGetBits(HeaderData_p, 8);
        SequenceDisplayExtension_p->transfer_characteristics    = viddec_HeaderDataGetBits(HeaderData_p, 8);
        SequenceDisplayExtension_p->matrix_coefficients         = viddec_HeaderDataGetBits(HeaderData_p, 8);
    }
    SequenceDisplayExtension_p->display_horizontal_size         = viddec_HeaderDataGetBits(HeaderData_p, 14);
    SequenceDisplayExtension_p->marker_bit                      = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(SequenceDisplayExtension_p->marker_bit))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits */
        *HasBadMarkerBit_p = TRUE;
    }
    SequenceDisplayExtension_p->display_vertical_size           = viddec_HeaderDataGetBits(HeaderData_p, 14);

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetSequenceDisplayExtension() function */


/*******************************************************************************
Name        : GetSequenceScalableExtension
Description : Get sequence scalable extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and sequence_scalable_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetSequenceScalableExtension(HeaderData_t * const HeaderData_p, SequenceScalableExtension_t * const SequenceScalableExtension_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

/*    SequenceScalableExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    SequenceScalableExtension_p->scalable_mode                   = viddec_HeaderDataGetBits(HeaderData_p, 2);
    SequenceScalableExtension_p->layer_id                        = viddec_HeaderDataGetBits(HeaderData_p, 4);
    if (SequenceScalableExtension_p->scalable_mode == SCALABLE_MODE_SPATIAL_SCALABILITY)
    {
        SequenceScalableExtension_p->lower_layer_prediction_horizontal_size = viddec_HeaderDataGetBits(HeaderData_p, 14);
        SequenceScalableExtension_p->marker_bit                             = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        if (!(SequenceScalableExtension_p->marker_bit))
        {
            /* Error_Recovery_Case_n07 : Check Marker bits.    */
            *HasBadMarkerBit_p = TRUE;
        }
        SequenceScalableExtension_p->lower_layer_prediction_vertical_size   = viddec_HeaderDataGetBits(HeaderData_p, 14);
        SequenceScalableExtension_p->horizontal_subsampling_factor_m        = viddec_HeaderDataGetBits(HeaderData_p, 5);
        SequenceScalableExtension_p->horizontal_subsampling_factor_n        = viddec_HeaderDataGetBits(HeaderData_p, 5);
        SequenceScalableExtension_p->vertical_subsampling_factor_m          = viddec_HeaderDataGetBits(HeaderData_p, 5);
        SequenceScalableExtension_p->vertical_subsampling_factor_n          = viddec_HeaderDataGetBits(HeaderData_p, 5);
    }
    else if (SequenceScalableExtension_p->scalable_mode == SCALABLE_MODE_TEMPORAL_SCALABILITY)
    {
        SequenceScalableExtension_p->picture_mux_enable = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        if (SequenceScalableExtension_p->picture_mux_enable)
        {
            SequenceScalableExtension_p->mux_to_progressive_sequence = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        }
        SequenceScalableExtension_p->picture_mux_order  = viddec_HeaderDataGetBits(HeaderData_p, 3);
        SequenceScalableExtension_p->picture_mux_factor = viddec_HeaderDataGetBits(HeaderData_p, 3);
    }

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetSequenceScalableExtension() function */


/*******************************************************************************
Name        : GetGroupOfPictures
Description : Get group of pictures data from MPEG headers data
Parameters  :
Assumptions : Groupf of pictures start code was just obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetGroupOfPictures(HeaderData_t * const HeaderData_p, GroupOfPictures_t * const GroupOfPictures_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

    GroupOfPictures_p->time_code.drop_frame_flag = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    GroupOfPictures_p->time_code.hours           = viddec_HeaderDataGetBits(HeaderData_p, 5);
    GroupOfPictures_p->time_code.minutes         = viddec_HeaderDataGetBits(HeaderData_p, 6);
    GroupOfPictures_p->time_code.marker_bit      = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(GroupOfPictures_p->time_code.marker_bit))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    GroupOfPictures_p->time_code.seconds         = viddec_HeaderDataGetBits(HeaderData_p, 6);
    GroupOfPictures_p->time_code.pictures        = viddec_HeaderDataGetBits(HeaderData_p, 6);
    GroupOfPictures_p->closed_gop                = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    GroupOfPictures_p->broken_link               = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetGroupOfPictures() function */


/*******************************************************************************
Name        : GetPicture
Description : Get picture data from MPEG headers data
Parameters  :
Assumptions : Pictre start code was just obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetPicture(HeaderData_t * const HeaderData_p, Picture_t * const Picture_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

    Picture_p->temporal_reference  = viddec_HeaderDataGetBits(HeaderData_p, 10);
    Picture_p->picture_coding_type = viddec_HeaderDataGetBits(HeaderData_p, 3);
    Picture_p->vbv_delay           = viddec_HeaderDataGetBits(HeaderData_p, 16);

    if (Picture_p->picture_coding_type == PICTURE_CODING_TYPE_P)
    {
        Picture_p->full_pel_forward_vector = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        Picture_p->forward_f_code          = viddec_HeaderDataGetBits(HeaderData_p, 3);
    }
    else if (Picture_p->picture_coding_type == PICTURE_CODING_TYPE_B)
    {
        Picture_p->full_pel_forward_vector  = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        Picture_p->forward_f_code           = viddec_HeaderDataGetBits(HeaderData_p, 3);
        Picture_p->full_pel_backward_vector = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        Picture_p->backward_f_code          = viddec_HeaderDataGetBits(HeaderData_p, 3);
    }

/* defined nowhere: this is for evolution of MPEG2 */
#ifdef ProcessPictureExtraInformation
    Picture_p->Count = 0;
    do
    {
        Picture_p->extra_bit_picture[Picture_p->Count] = ((viddec_HeaderDataGetBits(HeaderData_p, 1)) != 0);
        if (Picture_p->extra_bit_picture[Picture_p->Count])
        {
            Picture_p->extra_information_picture[Picture_p->Count] = viddec_HeaderDataGetBits(HeaderData_p, 8);
            Picture_p->Count++;
        }
    } while (Picture_p->extra_bit_picture[Picture_p->Count]);
#else
    /* Valid attitude in MPEG1/2: discard extra information data */
    /* ? just quit and leave data, or read data and loose it ? */
/*    while ((viddec_HeaderDataGetBits(HeaderData_p, 1)) != 0)*/
/*    {*/
        /* Loose data */
/*        viddec_HeaderDataGetBits(HeaderData_p, 8);*/
/*    }*/
#endif

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetPicture() function */


/*******************************************************************************
Name        : GetPictureCodingExtension
Description : Get picture coding extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and picture_coding_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetPictureCodingExtension(HeaderData_t * const HeaderData_p, PictureCodingExtension_t * const PictureCodingExtension_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

/*    PictureCodingExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    PictureCodingExtension_p->f_code[F_CODE_FORWARD][F_CODE_HORIZONTAL]  = viddec_HeaderDataGetBits(HeaderData_p, 4);
    PictureCodingExtension_p->f_code[F_CODE_FORWARD][F_CODE_VERTICAL]    = viddec_HeaderDataGetBits(HeaderData_p, 4);
    PictureCodingExtension_p->f_code[F_CODE_BACKWARD][F_CODE_HORIZONTAL] = viddec_HeaderDataGetBits(HeaderData_p, 4);
    PictureCodingExtension_p->f_code[F_CODE_BACKWARD][F_CODE_VERTICAL]   = viddec_HeaderDataGetBits(HeaderData_p, 4);
    PictureCodingExtension_p->intra_dc_precision         = viddec_HeaderDataGetBits(HeaderData_p, 2);
    PictureCodingExtension_p->picture_structure          = viddec_HeaderDataGetBits(HeaderData_p, 2);
    PictureCodingExtension_p->top_field_first            = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->frame_pred_frame_dct       = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->concealment_motion_vectors = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->q_scale_type               = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->intra_vlc_format           = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->alternate_scan             = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->repeat_first_field         = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->chroma_420_type            = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->progressive_frame          = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureCodingExtension_p->composite_display_flag     = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (PictureCodingExtension_p->composite_display_flag)
    {
        PictureCodingExtension_p->v_axis            = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        PictureCodingExtension_p->field_sequence    = viddec_HeaderDataGetBits(HeaderData_p, 3);
        PictureCodingExtension_p->sub_carrier       = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        PictureCodingExtension_p->burst_amplitude   = viddec_HeaderDataGetBits(HeaderData_p, 7);
        PictureCodingExtension_p->sub_carrier_phase = viddec_HeaderDataGetBits(HeaderData_p, 8);
    }

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetPictureCodingExtension() function */


/*******************************************************************************
Name        : GetQuantMatrixExtension
Description : Get quantization matrix extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and quant_matrix_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetQuantMatrixExtension(HeaderData_t * const HeaderData_p, QuantMatrixExtension_t * const QuantMatrixExtension_p)
{
    U8 k;

    HeaderDataResetBitsCounter(HeaderData_p);

/*    QuantMatrixExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    QuantMatrixExtension_p->load_intra_quantizer_matrix     = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (QuantMatrixExtension_p->load_intra_quantizer_matrix)
    {
        for (k = 0; k < QUANTISER_MATRIX_SIZE; k++)
        {
            QuantMatrixExtension_p->intra_quantizer_matrix[k] = viddec_HeaderDataGetBits(HeaderData_p, 8);
        }
    }

    QuantMatrixExtension_p->load_non_intra_quantizer_matrix = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (QuantMatrixExtension_p->load_non_intra_quantizer_matrix)
    {
        for (k = 0; k < QUANTISER_MATRIX_SIZE; k++)
        {
            QuantMatrixExtension_p->non_intra_quantizer_matrix[k] = viddec_HeaderDataGetBits(HeaderData_p, 8);
        }
    }

    QuantMatrixExtension_p->load_chroma_intra_quantizer_matrix = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (QuantMatrixExtension_p->load_chroma_intra_quantizer_matrix)
    {
        for (k = 0; k < QUANTISER_MATRIX_SIZE; k++)
        {
            QuantMatrixExtension_p->chroma_intra_quantizer_matrix[k] = viddec_HeaderDataGetBits(HeaderData_p, 8);
        }
    }

    QuantMatrixExtension_p->load_chroma_non_intra_quantizer_matrix = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (QuantMatrixExtension_p->load_chroma_non_intra_quantizer_matrix)
    {
        for (k = 0; k < QUANTISER_MATRIX_SIZE; k++)
        {
            QuantMatrixExtension_p->chroma_non_intra_quantizer_matrix[k] = viddec_HeaderDataGetBits(HeaderData_p, 8);
        }
    }

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetQuantMatrixExtension() function */


/*******************************************************************************
Name        : GetCopyrightExtension
Description : Get copyright extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and copyright_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetCopyrightExtension(HeaderData_t * const HeaderData_p, CopyrightExtension_t * const CopyrightExtension_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

    CopyrightExtension_p->copyright_flag        = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    CopyrightExtension_p->copyright_identifier  = viddec_HeaderDataGetBits(HeaderData_p, 8);
    CopyrightExtension_p->original_or_copy      = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    CopyrightExtension_p->reserved              = viddec_HeaderDataGetBits(HeaderData_p, 7);
    CopyrightExtension_p->marker_bit_1          = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(CopyrightExtension_p->marker_bit_1))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    CopyrightExtension_p->copyright_number_1    = viddec_HeaderDataGetBits(HeaderData_p, 20);
    CopyrightExtension_p->marker_bit_2          = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(CopyrightExtension_p->marker_bit_2))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    CopyrightExtension_p->copyright_number_2    = viddec_HeaderDataGetBits(HeaderData_p, 22);
    CopyrightExtension_p->marker_bit_3          = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(CopyrightExtension_p->marker_bit_3))
    {
        *HasBadMarkerBit_p = TRUE;
    }
    CopyrightExtension_p->copyright_number_3    = viddec_HeaderDataGetBits(HeaderData_p, 22);

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetCopyrightExtension() function */


/*******************************************************************************
Name        : GetPictureDisplayExtension
Description : Get picture display extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and picture_display_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetPictureDisplayExtension(HeaderData_t * const HeaderData_p,
    PictureDisplayExtension_t *        const PictureDisplayExtension_p,
    const PictureCodingExtension_t *   const PictureCodingExtension_p,
    const SequenceExtension_t *        const SequenceExtension_p,
    BOOL * const HasBadMarkerBit_p
)
{
    U8 k;

    HeaderDataResetBitsCounter(HeaderData_p);

/*    PictureDisplayExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/

    if (SequenceExtension_p->progressive_sequence)
    {
        if (PictureCodingExtension_p->repeat_first_field)
        {
            if (PictureCodingExtension_p->top_field_first)
            {
                PictureDisplayExtension_p->number_of_frame_centre_offsets = 3;
            }
            else
            {
                PictureDisplayExtension_p->number_of_frame_centre_offsets = 2;
            }
        }
        else
        {
            PictureDisplayExtension_p->number_of_frame_centre_offsets = 1;
        }
    }
    else
    {
        if ((PictureCodingExtension_p->picture_structure == PICTURE_STRUCTURE_TOP_FIELD) ||
            (PictureCodingExtension_p->picture_structure == PICTURE_STRUCTURE_BOTTOM_FIELD))
        {
            PictureDisplayExtension_p->number_of_frame_centre_offsets = 1;
        }
        else
        {
            if (PictureCodingExtension_p->repeat_first_field)
            {
                PictureDisplayExtension_p->number_of_frame_centre_offsets = 3;
            }
            else
            {
                PictureDisplayExtension_p->number_of_frame_centre_offsets = 2;
            }
        }
    }

    for (k = 0; k < PictureDisplayExtension_p->number_of_frame_centre_offsets; k++)
    {
        PictureDisplayExtension_p->FrameCentreOffsets[k].frame_centre_horizontal_offset =
            (S16)viddec_HeaderDataGetBits(HeaderData_p, 16);
        PictureDisplayExtension_p->marker_bit_1[k] = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        if (!(PictureDisplayExtension_p->marker_bit_1[k]))
        {
            /* Error_Recovery_Case_n07 : Check Marker bits.    */
            *HasBadMarkerBit_p = TRUE;
        }
        PictureDisplayExtension_p->FrameCentreOffsets[k].frame_centre_vertical_offset =
            (S16)viddec_HeaderDataGetBits(HeaderData_p, 16);
        PictureDisplayExtension_p->marker_bit_2[k] = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
        if (!(PictureDisplayExtension_p->marker_bit_2[k]))
        {
            *HasBadMarkerBit_p = TRUE;
        }
    }

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetPictureDisplayExtension() function */


/*******************************************************************************
Name        : GetPictureTemporalScalableExtension
Description : Get picture temporal scalable extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and picture_temporal_scalable_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetPictureTemporalScalableExtension(HeaderData_t * const HeaderData_p, PictureTemporalScalableExtension_t * const PictureTemporalScalableExtension_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

/*    PictureTemporalScalableExtension_p->extension_start_code_identifier = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    PictureTemporalScalableExtension_p->reference_select_code           = viddec_HeaderDataGetBits(HeaderData_p, 2);
    PictureTemporalScalableExtension_p->forward_temporal_reference      = viddec_HeaderDataGetBits(HeaderData_p, 10);
    PictureTemporalScalableExtension_p->marker_bit                      = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(PictureTemporalScalableExtension_p->marker_bit))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    PictureTemporalScalableExtension_p->backward_temporal_reference     = viddec_HeaderDataGetBits(HeaderData_p, 10);

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetPictureTemporalScalableExtension() function */


/*******************************************************************************
Name        : GetPictureSpatialScalableExtension
Description : Get picture spatial scalable extension data from MPEG headers data
Parameters  :
Assumptions : Extension start code and picture_spatial_scalable_extension identifier were just
              obtained from viddec_HeaderDataGetBits(), next header data is ready
Limitations :
Returns     : Total number of bits retrieved
*******************************************************************************/
static U32 GetPictureSpatialScalableExtension(HeaderData_t * const HeaderData_p, PictureSpatialScalableExtension_t * const PictureSpatialScalableExtension_p, BOOL * const HasBadMarkerBit_p)
{
    HeaderDataResetBitsCounter(HeaderData_p);

/*    PictureSpatialScalableExtension_p->extension_start_code_identifier          = viddec_HeaderDataGetBits(HeaderData_p, 4);*/
    PictureSpatialScalableExtension_p->lower_layer_temporal_reference           = viddec_HeaderDataGetBits(HeaderData_p, 10);
    PictureSpatialScalableExtension_p->marker_bit_1                             = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(PictureSpatialScalableExtension_p->marker_bit_1))
    {
        /* Error_Recovery_Case_n07 : Check Marker bits.    */
        *HasBadMarkerBit_p = TRUE;
    }
    PictureSpatialScalableExtension_p->lower_layer_horizontal_offset            = viddec_HeaderDataGetBits(HeaderData_p, 15);
    PictureSpatialScalableExtension_p->marker_bit_2                             = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    if (!(PictureSpatialScalableExtension_p->marker_bit_2))
    {
        *HasBadMarkerBit_p = TRUE;
    }
    PictureSpatialScalableExtension_p->lower_layer_vertical_offset              = viddec_HeaderDataGetBits(HeaderData_p, 15);
    PictureSpatialScalableExtension_p->spatial_temporal_weight_code_table_index = viddec_HeaderDataGetBits(HeaderData_p, 2);
    PictureSpatialScalableExtension_p->lower_layer_progressive_frame            = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);
    PictureSpatialScalableExtension_p->lower_layer_deinterlaced_field_select    = (viddec_HeaderDataGetBits(HeaderData_p, 1) != 0);

    return(HeaderDataGetBitsCounter(HeaderData_p));
} /* end of GetPictureSpatialScalableExtension() function */


/* End of vid_head.c */
