/*******************************************************************************

File name   : vid_sc.c

Description : Video start code controller source file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
 4 Dec 2000        Created                                           HG
 2 May 2001        Changed behavior of SEQUENCE_INFO_EVT: now
                   notified each time sequence info is changing      HG
22 Jul 2003        Added error recovery mode HIGH                    HG
*******************************************************************************/

/* Private preliminary definitions (internal use only) ---------------------- */
/* Define to add debug info and traces */
/*#define TRACE_DECODE*/
 
#ifdef TRACE_DECODE
#define TRACE_UART
#ifdef TRACE_UART
#include "trace.h"
#endif
#define DECODE_DEBUG
#endif

/* Define STVID_FORCE_INTERLACED_SCAN to ignore the MPEG2 parameter for picture per picture scan type.
   Instead, all pictures of the sequence are considered to be of the same scan type.
   STVID_FORCE_INTERLACED_SCAN is used to avoid changing display filters on a picture per picture basis,
   thus often creating some visible artifacts.
   This WA can be considered a good fix, as:
    - when scan type changes for each picture often, this is probably un-realistic and in fact not true,
      so there should be no problem in handling all pics the same way.
    - applying interlaced filters to a progressive image will not cause that many visible problems, at least
      this is less damaging than changing filters too often.
*/
/* Enable again STVID_FORCE_INTERLACED_SCAN flag because mixed streams generate display artefacts like black flashes with LMU */
#define STVID_FORCE_INTERLACED_SCAN

/* Includes ----------------------------------------------------------------- */
#if !defined ST_OSLINUX
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stos.h"

#ifdef STVID_STVAPI_ARCHITECTURE
#include "dtvdefs.h"
#include "stgvobj.h"
#include "stavmem.h"
#include "dv_rbm.h"
#endif /* def STVID_STVAPI_ARCHITECTURE */

#include "vid_sc.h"
#include "sttbx.h"
#include "vid_ctcm.h"

#ifdef STVID_DIRECTV
#include "vid_dtv.h"
#endif


/* Private Types ------------------------------------------------------------ */


/* Private Constants -------------------------------------------------------- */
#define MAX_BITRATE_IN_STREAM_IN_BIT_PER_SEC    (20 * 1024 * 1024)
#define ERROR_MARGIN_IN_PERCENT                 120

#define VBV_TO_BYTES_FACTOR                     (2 * 1024)

/* Max vbv_delay to wait according to specifiactions.   */
/* (i.e. 597*2048 = 1194 KBytes)                        */
#define MAX_VBV_SIZE_TO_REACH_BEFORE_START_IN_BYTES  (597 * VBV_TO_BYTES_FACTOR)

#define VbvSizeToReachWhenTooBig(BitBufferSize) (((BitBufferSize) * 8) / 10)


/* Private Variables (static)------------------------------------------------ */

/* Global Variables --------------------------------------------------------- */


/* Private Macros ----------------------------------------------------------- */

/* Error recovery strategy macros : */
#define IsSpuriousSliceStartCodeCausingPictureSyntaxError(ErrorRecoveryMode) (((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_FULL) || ((ErrorRecoveryMode) == STVID_ERROR_RECOVERY_HIGH))

/* Private Function prototypes ---------------------------------------------- */

ST_ErrorCode_t ComputeSequenceInformation(const MPEG2BitStream_t * const CurrentStream_p,
                                                 STVID_SequenceInfo_t * const SequenceInfo_p,
                                                 VIDDEC_SyntaxError_t * const SyntaxError_p);

static ST_ErrorCode_t FillPictureParamsStructures(const VIDDEC_Handle_t DecodeHandle,
                                        const MPEG2BitStream_t * const Stream_p,
                                        STVID_PictureInfos_t * const PictureInfos_p,
                                        StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                        PictureStreamInfo_t * const PictureStreamInfo_p);

#ifdef ST_SecondInstanceSharingTheSameDecoder
ST_ErrorCode_t FillPictureParamsStructuresForStill(  StillDecodeParams_t StillDecodeParams ,
                                    STVID_SequenceInfo_t * const SequenceInfo_p,
                                        const MPEG2BitStream_t * const Stream_p,
                                        STVID_PictureInfos_t * const PictureInfos_p,
                                        StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                        PictureStreamInfo_t * const PictureStreamInfo_p);
#endif /* ST_SecondInstanceSharingTheSameDecoder */

/* Exported functions ------------------------------------------------------- */



/*******************************************************************************
Name        : viddec_ProcessStartCode
Description : Process MPEG start code data.
              Find Syntax errors here. Actions are taken in the upper layer.
Parameters  : decoder handle, start code data parsed, stream structure updated
              SyntaxErrors are
              HasCriticalSequenceSyntaxErrors  : critical error in sequence.
              HasRecoveredSequenceSyntaxErrors : error in sequence but recovered.
              HasProfileErrors         : profile not compatible with the sequence.
              HasPictureSyntaxErrors   : error in picture.
              HasStartCodeParsingError :
              PreviousPictureHasAMissingField : previous picture not completed.
Assumptions :
Limitations :
Returns     : ST_NO_ERROR if success
              ST_ERROR_BAD_PARAMETER if bad parameter
              ST_ERROR_
*******************************************************************************/
ST_ErrorCode_t viddec_ProcessStartCode(const VIDDEC_Handle_t DecodeHandle,
                                       const U8 StartCode,
                                       MPEG2BitStream_t * const Stream_p,
                                       STVID_PictureInfos_t * const PictureInfos_p,
                                       StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                       PictureStreamInfo_t * const PictureStreamInfo_p,
                                       VIDDEC_SyntaxError_t * const SyntaxError_p,
                                       U32 * const TemporalReferenceOffset_p)
{
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;
    U16    ProfileAndLevelHorizontalSize;
    U16    ProfileAndLevelVerticalSize;
    U32    FrameRateForCalculation;
    STVID_PTS_t NullPTS;
    ST_ErrorCode_t  ErrorCode;
    U32 MaxBitrate, PictureAverageSize, MaxPictureSize, DifferenceBetweenTwoTemporalReferences;
    U32 ProfileAndLevelIndication;

    if (Stream_p == NULL)
    {
        /* Error case */
        return(ST_ERROR_BAD_PARAMETER);
    }

#ifdef STVID_MEASURES
    MeasuresRB[MEASURE_BEGIN][MeasureIndexRB] = time_now();
#endif

    if ((VIDDEC_Data_p->ComputeSequenceInfoOnNextPictureOrGroup) &&
        ((StartCode == PICTURE_START_CODE) || (StartCode == GROUP_START_CODE)))
    {
        VIDDEC_Data_p->ComputeSequenceInfoOnNextPictureOrGroup = FALSE;
        /* Caution: function below doesn't fill field SequenceInfo.StreamID, it is filled at start ! */
        ErrorCode = ComputeSequenceInformation(Stream_p, &(VIDDEC_Data_p->StartCodeProcessing.SequenceInfo), SyntaxError_p);

        if((VIDDEC_Data_p->PreviousSequenceInfo.ProfileAndLevelIndication != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.ProfileAndLevelIndication) || (VIDDEC_Data_p->PreviousSequenceInfo.FrameRate != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate))
        {
            switch (Stream_p->SequenceExtension.profile_and_level_indication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
            {
                case (PROFILE_MAIN | LEVEL_HIGH) : /* MPHL */
                case (PROFILE_MAIN | LEVEL_HIGH_1440) : /* MP HL1440 */
                    MaxPictureSize = MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_HIGH;
                    MaxBitrate = VIDCOM_MAX_BITRATE_MPEG_HD;
                break;

                case (PROFILE_MAIN | LEVEL_MAIN) : /* MPML */
                default :
                    MaxPictureSize = MAX_VIDEO_ES_SIZE_PROFILE_MAIN_LEVEL_MAIN;
                    MaxBitrate = VIDCOM_MAX_BITRATE_MPEG_SD;
                    break;
            }
            PictureAverageSize = ((MaxBitrate * 400) / 8) / ((VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate + 999) / 1000);
            /* Update the BitBuffer threshold for sending the underflow event,
             * according to the new Sequence Infos */
            VIDDEC_Data_p->BitBufferUnderflowMargin = PictureAverageSize * VIDCOM_DECODER_SPEED_MPEG;
            /* Update the BitBuffer required duration to inject according to the new
             * frame rate */
            VIDDEC_Data_p->BitBufferRequiredDuration = /* 2 VSYNC */
                ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION * 1000) / VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate);
            /* Update the max bitbuffer level for finding another startcode with
               the max picture size for the current profile and level */
            VIDDEC_Data_p->HWStartCodeDetector.MaxBitbufferLevelForFindingStartCode = MaxPictureSize;
        }

        /* Store basic current information. */
        VIDDEC_Data_p->DecoderOperatingMode.IsLowDelay = VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.IsLowDelay;

        /* Check sequence parameters according to MPEG profile */
        if (!(Stream_p->MPEG1BitStream))
        {
            /* MPEG-2: check parameters of the Profile and Level. */
            switch (Stream_p->SequenceExtension.profile_and_level_indication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
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
                    STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Profile and Level of stream -%d- not supported: "
                            "MP@ML is taken instead.", Stream_p->SequenceExtension.profile_and_level_indication ));
                    break;
            }

             /* check if MPEG profile correspond to the chip performances */
            if (((VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5518_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5517_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5516_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5514_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5512_MPEG)||
                 (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5510_MPEG))
                  && ((ProfileAndLevelHorizontalSize!=720)||(ProfileAndLevelVerticalSize!=576)))
            {
                SyntaxError_p->PictureSyntaxError.HasProfileErrors = TRUE;
            }
            else
            {

                /* Check if the Width and Height values correspond to the MPEG Profile. */
                if ((VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.Width > ProfileAndLevelHorizontalSize) ||
                    (VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.Height > ProfileAndLevelVerticalSize ))
                {
                    /* Error_Recovery_Case_n02 : The dimensions for the sequence don't correspond to the MPEG Profile.    */
                    /* The stream won't be displayed as is because such as dimensions are not managed. Skip the sequence. */
                    SyntaxError_p->PictureSyntaxError.HasProfileErrors = TRUE;
                }
                else
                {
                    /* Normal case no error : the stream can be displayed. */
                    SyntaxError_p->PictureSyntaxError.HasProfileErrors = FALSE;
                }
            }

            /* Check chroma format. The ST decoders are supporting 4:2:0 format only. */
            if (Stream_p->SequenceExtension.chroma_format != CHROMA_FORMAT_4_2_0)
            {
                SyntaxError_p->PictureSyntaxError.HasCriticalSequenceSyntaxErrors = TRUE;
            }
        } /* end MPEG2 */

        if ((VIDDEC_Data_p->SendSequenceInfoEventOnceAfterStart) || /* Requested by VIDDEC_Start() */
            (VIDDEC_Data_p->PreviousSequenceInfo.Height    != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.Height) || /* Height changing or... */
            (VIDDEC_Data_p->PreviousSequenceInfo.Width     != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.Width) || /* Width changing or... */
            (VIDDEC_Data_p->PreviousSequenceInfo.Aspect    != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.Aspect) || /* Aspect changing or... */
            (VIDDEC_Data_p->PreviousSequenceInfo.ScanType  != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.ScanType) || /* ScanType changing or... */
            (VIDDEC_Data_p->PreviousSequenceInfo.FrameRate != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate) || /* FrameRate changing */
            (VIDDEC_Data_p->PreviousSequenceInfo.MPEGStandard != VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.MPEGStandard) /* MPEGStandard changing */
            )
        {
            /* Send EVENT_ID_SEQUENCE_INFO EVT, it can only be send after having next start code after the sequence */

            STEVT_Notify(VIDDEC_Data_p->EventsHandle, VIDDEC_Data_p->RegisteredEventsID[VIDDEC_SEQUENCE_INFO_EVT_ID], STEVT_EVENT_DATA_TYPE_CAST &(VIDDEC_Data_p->StartCodeProcessing.SequenceInfo));
            /* Store this sequence information */
            VIDDEC_Data_p->PreviousSequenceInfo = VIDDEC_Data_p->StartCodeProcessing.SequenceInfo;
            /* Event has been sent once after start: now only when parameters are changing */
            VIDDEC_Data_p->SendSequenceInfoEventOnceAfterStart = FALSE;
        }

#ifdef STVID_DEBUG_GET_STATISTICS
        if (VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.VBVBufferSize > (VIDDEC_Data_p->BitBuffer.TotalSize / VBV_TO_BYTES_FACTOR))
        {
            VIDDEC_Data_p->Statistics.DecodePbVbvSizeGreaterThanBitBuffer ++;
        }
#endif /* STVID_DEBUG_GET_STATISTICS */


        /* At this stage the VBV Buffer Size is valid. If first picture after SoftReset, decide way of start. */
        /* GNBvd16209 : VbvBufferSize is to be considered only if vbv_delay is 0xFFFF (variant bit rate streams) */
        /* but first picture vbv_delay is still not computed. vbv_buffer_size has to be computed ! */
        if (VIDDEC_Data_p->VbvStartCondition.FirstSequenceSinceLastSoftReset)
        {
            /* First picture: decide whether to start decode or not */
            if (VIDDEC_Data_p->RealTime) /* &&
                (!(VIDDEC_Data_p->VbvStartCondition.VbvSizeReachedAccordingToInterruptBBT))) */
            {
                /* Need to start on vbv_size level and level not reached: compute level */
                /* If VBVBufferSize larger than bit buffer, arbitrary set the level
                to reach before starting the decode of first picture at 2/3. */
                if ((VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.VBVBufferSize * VBV_TO_BYTES_FACTOR) <= VIDDEC_Data_p->BitBuffer.TotalSize)
                {
                    /* By default, try to wait VBVBufferSize before starting the decode of first picture after Start(). */
                    VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes = VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.VBVBufferSize * VBV_TO_BYTES_FACTOR;
                }
                else
                {
                    /* we will run at a Bit Buffer Level egal to 80% (arbitrary!) */
                    VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes = VbvSizeToReachWhenTooBig((U32) VIDDEC_Data_p->BitBuffer.TotalSize);
                }
                /* Test if the VBVSizeToReachBeforeStart is available according   */
                /* to maximum values allowed by ATSC, ... specifications.         */
                if (VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes > MAX_VBV_SIZE_TO_REACH_BEFORE_START_IN_BYTES)
                {
                    VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes = MAX_VBV_SIZE_TO_REACH_BEFORE_START_IN_BYTES;
                }

                /* Set bit buffer threshold for BBT interrupt at level desired */
                HALDEC_SetDecodeBitBufferThreshold(VIDDEC_Data_p->HALDecodeHandle, VIDDEC_Data_p->VbvStartCondition.VbvSizeToReachBeforeStartInBytes);

                /* Start With vbv buffer size until vbv has not been parsed */
                VIDDEC_Data_p->VbvStartCondition.StartWithVbvBufferSize = TRUE;

                /* Disable decode and wait for BBT interrupt... */
                /* VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = FALSE; */
            }
            /* else */
            /* { */
                /* Start immediately: either don't start on vbv_size, or the level is already reached. */
                /* VIDDEC_Data_p->VbvStartCondition.IsDecodeEnable = TRUE; */
            /* } */

            /* first sequence just parsed */
            VIDDEC_Data_p->VbvStartCondition.FirstSequenceSinceLastSoftReset = FALSE;
            /* vbv_delay of first picture following this sequence will have to be parsed */
            /* set FirstPictureAfterFirstSequence to 1 in order to. */
            VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence = TRUE;

        } /* end first sequence after soft reset */
    } /* end compute sequence information and SC picture or group */
    /* ******************************************************************************** */
    /* Warning : Previous versions of video driver were managing a threshold calculated */
    /* according to VSync/5 decode task reaction time and maximum bit rate of injection.*/
    /* Further test have shown that this management introduced error when playing the   */
    /* stream VBV.ts. So, this threshold is no more managed, and assumptions are made   */
    /* that no overflow can occur whith a normal use of the video driver (sufficient    */
    /* size of bit buffer).                                                             */
    /* ******************************************************************************** */

    /* Do actions for previous start code: for those actions who can only be done after arrival of next start code */

    switch (VIDDEC_Data_p->StartCodeProcessing.PreviousStartCode)
    {
        case SEQUENCE_HEADER_CODE : /* for the PREVIOUS Start Code */
            /*  */
            SyntaxError_p->PictureSyntaxError.FirstPictureOfSequence = TRUE;

            /* Calculations of the sequence that require the sequence_extension: now OK after next start code */
            /* By default, we have no error in the sequence. */
            SyntaxError_p->PictureSyntaxError.HasCriticalSequenceSyntaxErrors = FALSE;
            SyntaxError_p->PictureSyntaxError.HasRecoveredSequenceSyntaxErrors = FALSE;
            SyntaxError_p->PictureSyntaxError.HasProfileErrors = FALSE;

            VIDDEC_Data_p->ComputeSequenceInfoOnNextPictureOrGroup = TRUE;

            break; /* end of SEQUENCE_HEADER_CODE */

        case PICTURE_START_CODE : /* for the PREVIOUS Start Code */
            if ((!(Stream_p->MPEG1BitStream)) && /* MPEG2 */
                (Stream_p->BitStreamProgress != AFTER_PICTURE_CODING_EXTENSION) &&/* Not picture coding extension found */
                (StartCode != PICTURE_START_CODE) /* and not particular case: picture SC again */)
            {
                /* Error_Recovery_Case_n03 : No picture coding extension in MPEG2 after picture.         */
                /* In MPEG2 a Picture Coding Extension Start Code always follows a Picture Start Code.   */
                SyntaxError_p->PictureSyntaxError.HasPictureSyntaxErrors = TRUE;
                break; /* end of PICTURE_START_CODE */
            }

                if ((Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_B) &&
                         (VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame == STVID_MPEG_FRAME_B))
                {
                    /* Compute temporal reference of this picture if it has no sense */
                    DifferenceBetweenTwoTemporalReferences = abs((S32) (VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference -
                                Stream_p->Picture.temporal_reference));

                    /* ubnormal DifferenceBetweenTooTemporalReferences means wrong temporal reference
                    * expect at starting of a GOP when the prvious is IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE
                    * compute the temporal reference based on the previous value */
                    if ((DifferenceBetweenTwoTemporalReferences > 1)&&
                        (DifferenceBetweenTwoTemporalReferences < (TEMPORAL_REFERENCE_MODULO -1)) &&
                            (VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference != IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE))
                    {
                        if ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) ||
                        ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_TOP_FIELD)&&
                                (Stream_p->PictureCodingExtension.top_field_first))||
                        ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_BOTTOM_FIELD)&&
                                !(Stream_p->PictureCodingExtension.top_field_first)))
                        {
                            Stream_p->Picture.temporal_reference = VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference +1;
                        }
                        else if (((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_TOP_FIELD)&&
                                !(Stream_p->PictureCodingExtension.top_field_first))||
                            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_BOTTOM_FIELD)&&
                                (Stream_p->PictureCodingExtension.top_field_first)))
                        {
                            Stream_p->Picture.temporal_reference = VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference;
                        }
                    }
                }
            /* Calculations of the picture that require the picture_coding_extension: now OK after next start code */

            /* Check if first field of a 2 fields picture */
            /* Manage the ExpectingSecondField variable.*/
            if (VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField)  /* Last picture was a first field */
            {
                /* Error_Recovery_Case_n04 (2nd case/2) : Missing expected 2nd field.                            */
                /* We expected a 2nd field. But we get something else than a picture. Or if it is a picture, it  */
                /* is a frame or a field with an other temporal reference. So it is not the associated 2nd field.*/
                /* So the previous picture has error but here is a new picture.                                  */
                if (Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) /* Picture is a frame picture */
                {
                    /* Picture is a frame and previous was a first field: missing second field ! */
                    SyntaxError_p->PictureSyntaxError.PreviousPictureHasAMissingField = TRUE;
                    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = FALSE;
                    /*VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch += 0;*/ /* We don't increment display counter, so the previous field picture will not be displayed */
                }
                else if (Stream_p->Picture.temporal_reference != VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference)
                {
                    /* Picture is a field picture but temporal_reference is not the same as the one of last field: missing second field ! */
                    SyntaxError_p->PictureSyntaxError.PreviousPictureHasAMissingField = TRUE;
                    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = TRUE;
                    /*VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch += 0;*/ /* We don't increment display counter, so the previous field picture will not be displayed */
                }
                else
                {
                    /*  No error : we get the 2nd field. */
                    SyntaxError_p->PictureSyntaxError.PreviousPictureHasAMissingField = FALSE;
                    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = FALSE;
#ifdef ST_ordqueue
                    if (VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame == STVID_MPEG_FRAME_P)
                    {
                        /* We are sure we have the 2 field of this P. We avoid to display a 2nd field of a P where the 1st  */
                        /* field is missing. Indeed in this case the 1st field is a part of the reference of the 2nd field. */
                        VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch += 2;
                    }
                    else
                    {
                        /* We have a P and previus field was a I: the I has been passed to display, just pass the P now. */
                        VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch += 1;
                    }
#endif  /* end of def ST_ordqueue */

                }
            } /* end of if (...ExpectingSecondField) */
            else /* if !ExpectingSecondField */
            {
                if ((Stream_p->MPEG1BitStream) ||
                    (Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) || /* Picture is a frame picture */
                    (Stream_p->Picture.temporal_reference == VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference))
                {
                    /* No error : it is the frame. */
                    /* Pass the frame to display now */
                    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = FALSE;
                    SyntaxError_p->PictureSyntaxError.PreviousPictureHasAMissingField = FALSE;
#ifdef ST_ordqueue
                    VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch += 1;
#endif  /* end of def ST_ordqueue */
                }
                else
                {
#ifdef ST_ordqueue
                    /* No error : it is the first field. */
                    if (Stream_p->Picture.picture_coding_type != PICTURE_CODING_TYPE_P)
                    {
                        /* Field is not a P: pass it to display now */
                        VIDDEC_Data_p->NextAction.NumberOfDisplayToLaunch += 1;
                    }
#endif  /* end of def ST_ordqueue */
                    /* else: field is a P: don't pass it to display now, wait next P to do it */
                    VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = TRUE;
                }
            }

            /* Compute temporal reference of this picture if frame or first of 2 fields */
            if ((Stream_p->MPEG1BitStream) ||
                (Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) ||
                (VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField))
            {
                if ((VIDDEC_Data_p->StartCodeProcessing.GetSecondReferenceAfterOverflowing) && 
                    (Stream_p->Picture.picture_coding_type != PICTURE_CODING_TYPE_B))
                {
                    /* Second reference picture just after overflowing temporal_reference (1023->0),
                       no picture can be earlier ! So now add 1024 to TemporalReferenceOffset */
                    VIDDEC_Data_p->StartCodeProcessing.GetSecondReferenceAfterOverflowing = FALSE;
                    VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset   += TEMPORAL_REFERENCE_MODULO;
                }
                if (VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed)                 
                {
                    /* Last reference picture had an overflowing temporal_reference (1023->0). */
                    VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed = FALSE;
                    if ((Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_B))
                    {
                        /* B picture just after overflowing temporal_reference (1023->0),
                           It's for sure earlier ! So now subtract 1024 to TemporalReferenceOffset,
                           Addition of 1024 will be done when we get the next reference picture */
                        VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset   -= TEMPORAL_REFERENCE_MODULO;
                        VIDDEC_Data_p->StartCodeProcessing.GetSecondReferenceAfterOverflowing = TRUE;
                    }
                }

                /* Frame or first field picture: calculate extended temporal reference */
                VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference =
                        VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset + Stream_p->Picture.temporal_reference;

                if (VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver)
                {
                    /* First picture ever: initialise memory of greatest ExtendedTemporalReference of reference pictures */
                    VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference =
                        VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference;
                }
                /* Check if temporal_reference has overflowed from TEMPORAL_REFERENCE_MODULO to 0 */
                if (Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_B)
                {
                    /* B picture */
                    if (((S32) (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference -
                            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference)) >= (TEMPORAL_REFERENCE_MODULO - 1))
                    {
                        /* First B having an overflowing temporal_reference (1023->0) after a reference having the same:
                           no picture can be earlier ! So now add 1024 to TemporalReferenceOffset
                           and take this as the new base for all next temporal reference compute. */
                        VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference += TEMPORAL_REFERENCE_MODULO;
                        VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset   += TEMPORAL_REFERENCE_MODULO;
                        VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed = FALSE;
                        VIDDEC_Data_p->StartCodeProcessing.GetSecondReferenceAfterOverflowing = FALSE;
                    }
                } /* Computing ExtendedTemporalReference of a B picture. */
                else
                {
                    /* I or P picture: reference picture */
                        if (((S32) (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference -
                                VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference)) > 0)
                        {
                            /* First reference picture having an overflowing temporal_reference (1023->0),
                           so add 1024 to the extended temporal reference and to the TemporalReferenceOffset. */
                        VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference += TEMPORAL_REFERENCE_MODULO;
                        VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset   += TEMPORAL_REFERENCE_MODULO;
                                VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed = TRUE;
                    }
                    if (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference <
                        VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference)
                    {
                        /* ExtendedTemporalReference of this reference picture is going back in the past. This should
                           not happen. This could happen because temporal_reference was spoiled. This error case would
                           lock the current architecture, as the new reference could be rejected by the display because
                           too late, but still kept RESERVED as a reference. If the display was locked, it may never
                           reach condition for unlock and the decoder may never find a buffer to decode next B, in 3 buffers mode.
                           This case was experienced with spoiled streams and led to the following parsing :
                           P754-B752-B753-P757-B750-P754-B752-B753-P757
                           ______________errors^^^^-^^^^                          ) */
                        VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset += (
                            VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference -
                            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference - 1);
                        /* The case above would now lead to the following parsing :
                           P754-B752-B753-P757-B750-P758-B7xx-B7xx-P757
                           ______________errors^^^^-^^^^                          ) */
                        VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference =
                            VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference + 1;
                    }
                    /* Always store greatest ExtendedTemporalReference of reference pictures. This is used for the error case above. */
                    if (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference >
                        VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference)
                    {
                        VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference =
                            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference;
                    }
                } /* Computing ExtendedTemporalReference of a reference picture. */
            } /* End compute temporal reference */

            /* Compute PTS if required */
            if (VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate != 0)
            {
                FrameRateForCalculation = VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate;
            }
            else
            {
                /* To avoid division by 0 */
                FrameRateForCalculation = 50000;
            }

            if ((VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5100_MPEG) ||
                  (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5525_MPEG) ||
                  (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5105_MPEG) ||
                  (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_5301_MPEG) ||
                  (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_7710_MPEG) ||
                  (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_7100_MPEG) ||
                  (VIDDEC_Data_p->DeviceType == STVID_DEVICE_TYPE_7109_MPEG))
            {
                 /* Get or compute PTS */
                 if (HALDEC_GetPTS(VIDDEC_Data_p->HALDecodeHandle, &(VIDDEC_Data_p->StartCodeProcessing.PTS.PTS), &(VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33)))
                 {
#ifdef TRACE_UART
     /*                TraceBuffer(("-HwPTS(%c%d)=%d,",(Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_B?'B':
                                             ((Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_P?'P':'I'))),
                                             Stream_p->Picture.temporal_reference,
                                             VIDDEC_Data_p->StartCodeProcessing.PTS.PTS
                     ));*/
#endif
                     VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated = FALSE;
                 }
                 else
                 {
                     VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated = TRUE;
                 }
            }

            if (VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated)
            {
                if (VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver)
                {
                    /* Initialise first PTS to 0, or to a value in case of open GOP (first ExtendedTemporalReference not 0) */
                    VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration =
                            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference *
                            ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation; /* Supposing 2 fields */
                    VIDDEC_Data_p->StartCodeProcessing.PTS.PTS =
                            VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration;
                    VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33 = FALSE;
                }
                else
                {
                    U32 OldPTS = VIDDEC_Data_p->StartCodeProcessing.PTS.PTS;
                    U32 TimeDisplayPreviousPicture = ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) *
                            VIDDEC_Data_p->StartCodeProcessing.PreviousPictureNbFieldsDisplay / FrameRateForCalculation / 2;

                    if ((VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS ==
                            (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference - 1)) ||
                            /* B after B (or reference after reference immediately following, no B between) */
                        (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS ==
                        VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference)) /* Second of 2 fields picture */
                    {
                        /* No reordering */
                        VIDDEC_Data_p->StartCodeProcessing.PTS.PTS += TimeDisplayPreviousPicture;
                        if (VIDDEC_Data_p->StartCodeProcessing.PTS.PTS < OldPTS)
                        {
                            /* 33th bit to toggle */
                            VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33 = (!VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33);
                        }
                        if ((VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration != 0) &&
                                (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS ==
                                VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference))
                        {
                            VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration += TimeDisplayPreviousPicture;
                        }
                    }
                    else if (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS >
                            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference) /* B after a reference */
                    {
                        /* To be re-ordered backwards */
                        VIDDEC_Data_p->StartCodeProcessing.PTS.PTS -=
                                VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration;
                        VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration = 0;
                        if (VIDDEC_Data_p->StartCodeProcessing.PTS.PTS > OldPTS)
                        {
                            /* 33th bit to toggle */
                            VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33 = (!VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33);
                        }
                    }
                    else /* Reference */
                    {
                        /* To be re-ordered forwards */
                        /* VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration is what we suppose to be the */
                        /* duration od the display of the B between the references (not yet arrived so we can't guess !).  */
                        /* We supppose they will not be repeated, and have the same picture structure as the current       */
                        /* picture. This error will be substracted later on...                                             */
                        if (VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame == STVID_MPEG_FRAME_B)
                                /* Previous picture is not a reference */
                        {
                            VIDDEC_Data_p->StartCodeProcessing.PTS.PTS +=
                                    VIDDEC_Data_p->StartCodeProcessing.PreviousReferenceTotalNbFieldsDisplay *
                                    ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation / 2;
                            VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration =
                                    (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference -
                                    VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS - 2) *
                                    ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                                    /* Supposing 2 fields */
                        }
                        else
                        {
                            VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration =
                                    (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference -
                                    VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS - 1) *
                                    ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                                    /* Supposing 2 fields */
                        }
/*                                  if (Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE)*/
                                    /* Current picture is a frame picture */
/*                                    {*/
                            /* Supposing 2 fields */
/*                                        VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration = */
                                    /*VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration * 2;*/
/*                                    }*/
                        VIDDEC_Data_p->StartCodeProcessing.PTS.PTS += TimeDisplayPreviousPicture +
                                VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration;
                        if (VIDDEC_Data_p->StartCodeProcessing.PTS.PTS < OldPTS)
                        {
                            /* 33th bit to toggle */
                            VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33 = (!VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33);
                        }
                    }
                }
            } /* End of compute PTS interpolated */
            else
            {
                if (VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver)
                {
                    /* Initialise first PTS to 0, or to a value in case of open GOP (first ExtendedTemporalReference not 0) */
                    VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration =
                            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference *
                            ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation; /* Supposing 2 fields */
                }
                else
                {
                    if (VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame == STVID_MPEG_FRAME_B)
                            /* Previous picture is not a reference */
                    {
                        VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration =
                                (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference -
                                VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS - 2) *
                                ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                                /* Supposing 2 fields */
                    }
                    else
                    {
                        VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration =
                                (VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference -
                                VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS - 1) *
                                ((STVID_MPEG_CLOCKS_ONE_SECOND_DURATION) * 1000) / FrameRateForCalculation;
                                /* Supposing 2 fields */
                    }
/*                    VIDDEC_Data_p->StartCodeProcessing.SupposedPicturesDisplayDuration = 0;*/
                }
            }

            VIDDEC_Data_p->StartCodeProcessing.PTS.IsValid = TRUE;

            /* Compute time code */
            /* !!! Bad: should take ExtendedTemporalReference into account ! */
            if (VIDDEC_Data_p->StartCodeProcessing.TimeCode.Interpolated)
            {
                if (VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver)
                {
                    VIDDEC_Data_p->StartCodeProcessing.TimeCode.Frames = 0;
                    VIDDEC_Data_p->StartCodeProcessing.TimeCode.Seconds = 0;
                    VIDDEC_Data_p->StartCodeProcessing.TimeCode.Minutes = 0;
                    VIDDEC_Data_p->StartCodeProcessing.TimeCode.Hours = 0;
                }
                else
                {
                    if (! VIDDEC_Data_p->ForTask.PictureStreamInfo.ExpectingSecondField)
                    {
                        vidcom_IncrementTimeCode( &(VIDDEC_Data_p->StartCodeProcessing.TimeCode),
                                                  VIDDEC_Data_p->StartCodeProcessing.SequenceInfo.FrameRate,
                                                  Stream_p->HasGroupOfPictures );
                    }
                }
            } /* End compute time code */

            /* Increment picture counter to set new temporal reference offset when group arrives */
            if ((Stream_p->MPEG1BitStream) ||
                (Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) ||
                (VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField))
            {
                VIDDEC_Data_p->StartCodeProcessing.PictureCounter ++;
            }

            /* Save picture information for next picture */
            if (Stream_p->Picture.picture_coding_type != PICTURE_CODING_TYPE_B) /* reference */
            {
                VIDDEC_Data_p->StartCodeProcessing.LastButOneReferenceTemporalReference = VIDDEC_Data_p->StartCodeProcessing.LastReferenceTemporalReference;
                VIDDEC_Data_p->StartCodeProcessing.LastReferenceTemporalReference = Stream_p->Picture.temporal_reference;
            }
            VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference = Stream_p->Picture.temporal_reference;
            VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference = VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference;
            break; /* end of PICTURE_START_CODE */

        case USER_DATA_START_CODE : /* for the PREVIOUS Start Code */
            /* Processing start code after user data: can launch start code search again... */
/*            VIDDEC_Data_p->StartCodeParsing.HasUserDataProcessTriggeredStartCode = FALSE;*/
            break; /* end of USER_DATA_START_CODE */

        case FIRST_SLICE_START_CODE : /* for the PREVIOUS Start Code */
            /*  */
            SyntaxError_p->PictureSyntaxError.FirstPictureOfSequence = FALSE;

            /* Initialze the flag for the picture to come (could not be done in CURRENT=PICTURE because the user taking  */
            /* Syntax Error Data only when PREVIOUS=SLICE would see always FALSE). */
            SyntaxError_p->PictureSyntaxError.HasPictureSyntaxErrors = FALSE;

            /* Error_Recovery_Case_n04 (1st case/2) : Missing expected 2nd field.                            */
            /* We expected a 2nd field. But we get something else than a picture. Or if it is a picture, it  */
            /* is a frame or a field with an other temporal reference. So it is not the associated 2nd field.*/
            /* So the previous picture has error but here is a new picture.                                  */
            if (VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField)  /* Last picture was a first field */
            {
                if (StartCode != PICTURE_START_CODE)
                {
                    SyntaxError_p->PictureSyntaxError.PreviousPictureHasAMissingField = TRUE;
                }
            }
            break;

        default : /* Something else than Sequence, Picture or UserData */
            /* No action for this start code */
            break;
    }

    /* Do actions for current start code */
    switch (StartCode)
    {
        case SEQUENCE_HEADER_CODE : /* for the CURRENT Start Code */
            SyntaxError_p->HasStartCodeParsingError = FALSE;
/*            VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField = FALSE; *//* Reset fields expectation flag */
            VIDDEC_Data_p->BrokenLink.Active = FALSE;
            break; /* end of SEQUENCE_HEADER_CODE */

        case GROUP_START_CODE : /* for the CURRENT Start Code */
            /* Group implies reset of temporal reference to 0: update offset with picture counter to continue counting up */
            VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset += VIDDEC_Data_p->StartCodeProcessing.PictureCounter;
            VIDDEC_Data_p->StartCodeProcessing.PictureCounter = 0;
            if ((VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference > 0) &&
                (VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset <= VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference))
            {
                /* In case the last calculated ExtendedTemporalReference is higher than
                    the offset (ex: cut stream after Pn and Bn-2/Bn-1 missing), push the offset up */
                VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset = VIDDEC_Data_p->StartCodeProcessing.PreviousGreatestReferenceExtendedTemporalReference + 1;
            }
            VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
            VIDDEC_Data_p->StartCodeProcessing.GetSecondReferenceAfterOverflowing = FALSE;
            VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed = FALSE;

            /* Synthesis of the MPEG2 Standard about the closed_gop and broken_link flags  :

            ---------------------------------------------------------------------------------------------------------
            |  closed_gop                            |             0            |   0   |   1    |        1         |
            |  broken_link                           |             0            |   1   |   0    |        1         |
            ---------------------------------------------------------------------------------------------------------
            |  Are B pictures immediately following  |  Yes with the latest     |  No.  |  Yes.  |  Inconsistent    |
            |  the I picture (if any) decodable ?    |  ref of a previous seq.  |       |        |  consider : No.  |
            ---------------------------------------------------------------------------------------------------------
            */

            if (Stream_p->GroupOfPictures.closed_gop)
            {
                /* Closed group of pictures: no reference among previous pictures. */
            }
            else
            {
                /* Open group of pictures: some coming B pictures require a reference among previous pictures */
                if ((Stream_p->GroupOfPictures.broken_link) ||
                    (VIDDEC_Data_p->ReferencePicture.MissingPrevious))  /* Stream doesn't have the required reference */
                {
                    /* Required reference missing: skip B pictures following the first I */
                    VIDDEC_Data_p->BrokenLink.Active = TRUE;
                    VIDDEC_Data_p->BrokenLink.GotFirstReferencePicture = FALSE;
                }
            }
            break; /* end of GROUP_START_CODE */

        case PICTURE_START_CODE : /* for the CURRENT Start Code */
            SyntaxError_p->HasStartCodeParsingError = FALSE; /* !!! ? */

            /* For a Frame or a 1st Field : to not enter here 2 times for a field picture. */
            if (!(VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField))
            {
                /* Process broken_link skip of current group of pictures */
                if ((VIDDEC_Data_p->BrokenLink.Active) && /* broken_link active */
                    (Stream_p->Picture.picture_coding_type != PICTURE_CODING_TYPE_B)) /* Reference picture */
                {
                    /* broken_link and reference picture: end broken_link (if not first reference) */
                    if (!(VIDDEC_Data_p->BrokenLink.GotFirstReferencePicture))
                    {
                        /* This is the first I after the group of pictures header: just pass */
                        VIDDEC_Data_p->BrokenLink.GotFirstReferencePicture = TRUE;
                    }
                    else
                    {
                        /* 2nd reference : end broken_link of current group of pictures */
                        VIDDEC_Data_p->BrokenLink.Active = FALSE;
                    }
                }
            }

            /* Compute time code */
            if ((Stream_p->HasGroupOfPictures) &&
                (Stream_p->Picture.temporal_reference == 0) &&
                (VIDDEC_Data_p->StartCodeProcessing.PictureCounter < (TEMPORAL_REFERENCE_MODULO - 1)) /* Treats case of temporal_reference overflow */
                )
            {
                /* Temporal reference 0 just after a group of picture header: real value from stream */
                VIDDEC_Data_p->StartCodeProcessing.TimeCode.Interpolated = FALSE;
                VIDDEC_Data_p->StartCodeProcessing.TimeCode.Hours   = Stream_p->GroupOfPictures.time_code.hours;
                VIDDEC_Data_p->StartCodeProcessing.TimeCode.Minutes = Stream_p->GroupOfPictures.time_code.minutes;
                VIDDEC_Data_p->StartCodeProcessing.TimeCode.Seconds = Stream_p->GroupOfPictures.time_code.seconds;
                VIDDEC_Data_p->StartCodeProcessing.TimeCode.Frames  = Stream_p->GroupOfPictures.time_code.pictures;
            }
            else
            {
                VIDDEC_Data_p->StartCodeProcessing.TimeCode.Interpolated = TRUE;
            }

            if ((VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5100_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5525_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5105_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_5301_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7710_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7100_MPEG) &&
                  (VIDDEC_Data_p->DeviceType != STVID_DEVICE_TYPE_7109_MPEG))
            {

                /* Get or compute PTS */
                if (HALDEC_GetPTS(VIDDEC_Data_p->HALDecodeHandle, &(VIDDEC_Data_p->StartCodeProcessing.PTS.PTS), &(VIDDEC_Data_p->StartCodeProcessing.PTS.PTS33)))
                {
#ifdef TRACE_UART
/*                TraceBuffer(("-HwPTS(%c%d)=%d,",(Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_B?'B':
                                        ((Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_P?'P':'I'))),
                                        Stream_p->Picture.temporal_reference,
                                        VIDDEC_Data_p->StartCodeProcessing.PTS.PTS
                ));*/
#endif
                    VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated = FALSE;
                }
                else
                {
                    VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated = TRUE;
                }
            }

            break; /* end of PICTURE_START_CODE */

        case USER_DATA_START_CODE : /* for the CURRENT Start Code */
            /* Identify which user data it is: sequence, group or picture */
            if (   (Stream_p->BitStreamProgress < AFTER_GROUP_OF_PICTURES_HEADER)  /* Just after sequence */
                || (   (! Stream_p->HasGroupOfPictures)                            /* before picture with no group */
                    && (Stream_p->BitStreamProgress < AFTER_PICTURE_HEADER))
                || (Stream_p->BitStreamProgress >= AFTER_PICTURE_DATA)             /* After picture data (not supposed to happen, but better consider this a sequence user data than a picture user data ! */
               )
            {
                /* This is a sequence user data */
                VIDDEC_Data_p->UserData.PositionInStream = STVID_USER_DATA_AFTER_SEQUENCE;
                VIDDEC_Data_p->UserData.Length = Stream_p->UserData.UsedSize;
                VIDDEC_Data_p->UserData.BufferOverflow = Stream_p->UserData.Overflow;
                VIDDEC_Data_p->UserData.Buff_p = Stream_p->UserData.Data_p;

                /* Update picture user data fields */
                NullPTS.PTS = 0;
                NullPTS.PTS33 = FALSE;
                NullPTS.Interpolated = FALSE;
                NullPTS.IsValid = FALSE;
                VIDDEC_Data_p->UserData.PTS = NullPTS;
                /* Update hidden field pTemporalReference (until it disappears) */
                VIDDEC_Data_p->UserData.pTemporalReference = 0;
            }
            else if (Stream_p->BitStreamProgress < AFTER_PICTURE_HEADER) /* before picture in other cases */
            {
                /* This is a group user data */
                VIDDEC_Data_p->UserData.PositionInStream = STVID_USER_DATA_AFTER_GOP;
                VIDDEC_Data_p->UserData.Length = Stream_p->UserData.UsedSize;
                VIDDEC_Data_p->UserData.BufferOverflow = Stream_p->UserData.Overflow;
                VIDDEC_Data_p->UserData.Buff_p = Stream_p->UserData.Data_p;

                /* Update picture user data fields */
                NullPTS.PTS = 0;
                NullPTS.PTS33 = FALSE;
                NullPTS.Interpolated = FALSE;
                NullPTS.IsValid = FALSE;
                VIDDEC_Data_p->UserData.PTS = NullPTS;
                /* Update hidden field pTemporalReference (until it disappears) */
                VIDDEC_Data_p->UserData.pTemporalReference = 0;
            }
            else
            {
                /* This is a picture user data */
                VIDDEC_Data_p->UserData.PositionInStream = STVID_USER_DATA_AFTER_PICTURE;
                VIDDEC_Data_p->UserData.Length = Stream_p->UserData.UsedSize;
                VIDDEC_Data_p->UserData.BufferOverflow = Stream_p->UserData.Overflow;
                VIDDEC_Data_p->UserData.Buff_p = Stream_p->UserData.Data_p;

                /* Exploit usre data depending on profile */
                switch (VIDDEC_Data_p->BroadcastProfile)
                {
#ifdef STVID_DIRECTV
                    case STVID_BROADCAST_DIRECTV :
                        if (VIDDEC_Data_p->UserData.PositionInStream == STVID_USER_DATA_AFTER_PICTURE)
                        {
                            if ((Stream_p->MPEG1BitStream) ||
                                /* Caution >= for profile_and_level_indication means <= because values are in inverted order ! */
                                (((Stream_p->SequenceExtension.profile_and_level_indication & 0x70) >= PROFILE_MAIN) &&  /* Simple or main profile */
                                ((Stream_p->SequenceExtension.profile_and_level_indication & 0x0F) >= LEVEL_MAIN))    /* low or main level */
                                )
                            {
                                /* MPEG1 or MPEG2 SD: process user data */
                                if (Stream_p->MPEG1BitStream)
                                {
                                    /* MPEG1 */
                                    ProcessPictureUserDataDirectv(&(Stream_p->UserData), &(VIDDEC_Data_p->StartCodeProcessing.PTS),
                                            NULL, &(Stream_p->PictureDisplayExtension));
                                }
                                else
                                {
                                    /* MPEG2 */
                                    ProcessPictureUserDataDirectv(&(Stream_p->UserData), &(VIDDEC_Data_p->StartCodeProcessing.PTS),
                                            &(Stream_p->PictureCodingExtension), &(Stream_p->PictureDisplayExtension));
                                }
                            }
                        }
                        break;
#endif /* STVID_DIRECTV */

                    case STVID_BROADCAST_ATSC : /* No special process for ATSC */
                    default :
                        break;
                }


                /* Update picture user data fields */
                VIDDEC_Data_p->UserData.PTS = VIDDEC_Data_p->StartCodeProcessing.PTS;
                /* Update hidden field pTemporalReference (until it disappears) */
                VIDDEC_Data_p->UserData.pTemporalReference = Stream_p->Picture.temporal_reference;
            }
            VIDDEC_Data_p->UserData.BroadcastProfile = VIDDEC_Data_p->BroadcastProfile;
            if (VIDDEC_Data_p->UserDataEventEnabled)
            {
                /* Special function for USER_DATA_EVT event since different management between OS2x and Linux */
                vidcom_NotifyUserData(VIDDEC_Data_p->EventsHandle,
                                      VIDDEC_Data_p->RegisteredEventsID[VIDDEC_USER_DATA_EVT_ID],
                                      &(VIDDEC_Data_p->UserData));
            }

#ifdef STVID_DEBUG_GET_STATISTICS
            VIDDEC_Data_p->Statistics.DecodeUserDataFound ++;
#endif /* STVID_DEBUG_GET_STATISTICS */
            break;

        case FIRST_SLICE_START_CODE : /* for the CURRENT Start Code */
            /* First slice (of a picture) start code */
#ifdef TRACE_UART
/*            TraceBuffer(("-Sli"));*/
#endif

            /* Test if first slice start code of very first picture after first sequence, then test if      */
            /* TRUE PTS. In that case, enable the start on PTS (if vbv_delay of first picture is invalid).  */
            if (VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence)
            {
#ifdef ST_avsync
                /* First picture after sequence, check if start is to be done on PTS reach condition */
                if ((!(VIDDEC_Data_p->VbvStartCondition.StartWithVbvDelay)) &&
                    (!(VIDDEC_Data_p->StartCodeProcessing.PTS.Interpolated)))
                {
                    /* True PTS and start on vbv_delay not activated: allow the start on PTS comparison time */
                    VIDDEC_Data_p->VbvStartCondition.StartWithPTSTimeComparison = TRUE;
                    /* Save its PTS as first decode action condition */
                    VIDDEC_Data_p->VbvStartCondition.FirstPicturePTSAfterFirstSequence =
                            VIDDEC_Data_p->StartCodeProcessing.PTS;
                } /* Start not on vbv_delay, and true PTS */
#endif /* ST_avsync */
                /* Whatever the case, the first picture after sequence is managed. Reset the corresponding flag.    */
                VIDDEC_Data_p->VbvStartCondition.FirstPictureAfterFirstSequence = FALSE;
            }

            /* Error_Recovery_Case_n05 : Inconsistency in start code arrival.    */
            /* Slice not expected at this point of the bitstream. Exploit slice  */
            /* only if info from stream is valid .                               */
            if (Stream_p->BitStreamProgress != AFTER_PICTURE_DATA)
            {
                SyntaxError_p->HasStartCodeParsingError = TRUE;
            }

            if ((SyntaxError_p->HasStartCodeParsingError) && (IsSpuriousSliceStartCodeCausingPictureSyntaxError(((VIDDEC_Data_t *) DecodeHandle)->ErrorRecoveryMode)))
            {
                /* Invalid slice start code because picture was not present */
                SyntaxError_p->PictureSyntaxError.HasPictureSyntaxErrors = TRUE;
            }

            /* Only now after slice we are sure that all picture data are valid in Stream_p:
            copy picture data into relevant structures */
            *TemporalReferenceOffset_p = VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset;
#if 0
            if (VIDDEC_Data_p->StartCodeProcessing.OneTemporalReferenceOverflowed)
            {
                *TemporalReferenceOffset_p += TEMPORAL_REFERENCE_MODULO;
            }
#endif
            ErrorCode = FillPictureParamsStructures(DecodeHandle, Stream_p,
                            PictureInfos_p, StreamInfoForDecode_p, PictureStreamInfo_p);

            /* Send event about next picture to be decoded infos */
            STEVT_Notify(VIDDEC_Data_p->EventsHandle,
                VIDDEC_Data_p->RegisteredEventsID[VIDDEC_NEW_PICTURE_PARSED_EVT_ID],
                STEVT_EVENT_DATA_TYPE_CAST PictureInfos_p);


            /* FillPictureParamsStructures() passed quant matrix changes info to StreamInfoForDecode, so now consider
            that the info "quant matrix have changed" has been exploited. This has to be done ! */
            Stream_p->IntraQuantMatrixHasChanged = FALSE;
            Stream_p->NonIntraQuantMatrixHasChanged = FALSE;
            if (ErrorCode != ST_NO_ERROR)
            {
                /* Error_Recovery_Case_n06 : Picture coding type or picture structure invalid. */
                STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Error detected in picture headers data."));
                SyntaxError_p->PictureSyntaxError.HasPictureSyntaxErrors = TRUE;
            }

            if (!SyntaxError_p->PictureSyntaxError.HasPictureSyntaxErrors) /* set just below or before */
            {
                /* Valid slice start code because picture was present */
                if (PictureInfos_p->VideoParams.MPEGFrame != STVID_MPEG_FRAME_B)
                {
                    if (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS != VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference)
                    {
                        VIDDEC_Data_p->StartCodeProcessing.PreviousReferenceTotalNbFieldsDisplay = 0;
                    }
                    VIDDEC_Data_p->StartCodeProcessing.PreviousReferenceTotalNbFieldsDisplay += VIDDEC_Data_p->NbFieldsDisplayNextDecode;
                }
/*                            if (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS != VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference)*/
/*                            {*/
/*                                VIDDEC_Data_p->StartCodeProcessing.PreviousPictureNbFieldsDisplay = 0;*/
/*                            }*/
            }
                VIDDEC_Data_p->StartCodeProcessing.PreviousPictureNbFieldsDisplay /*+*/= VIDDEC_Data_p->NbFieldsDisplayNextDecode;
                VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReferenceForPTS = VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference;
                VIDDEC_Data_p->StartCodeProcessing.PreviousPictureMPEGFrame = PictureInfos_p->VideoParams.MPEGFrame;
                /* Copy PTS now that it is interpolated (or real) */
                PictureInfos_p->VideoParams.PTS    = VIDDEC_Data_p->StartCodeProcessing.PTS;
/*                            STTBX_Report((STTBX_REPORT_LEVEL_INFO, "Pic %c, TRef= %d, FieldsDisp=%d : VIDDEC_Data_p->StartCodeProcessing.PTS.PTS=%d", (Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_B?'B':((Stream_p->Picture.picture_coding_type == PICTURE_CODING_TYPE_P?'P':'I'))), VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference, VIDDEC_Data_p->NbFieldsDisplayNextDecode, VIDDEC_Data_p->StartCodeProcessing.PTS.PTS));*/

            VIDDEC_Data_p->StartCodeProcessing.FirstPictureEver = FALSE;

            /* Do all processes when receiving next start code */
            break; /* end of FIRST_SLICE_START_CODE */

        case DISCONTINUITY_START_CODE : /* for the CURRENT Start Code */
            /* Discontinuity: reset ExtendedTemporalReference */
            VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset += VIDDEC_Data_p->StartCodeProcessing.PictureCounter;
            VIDDEC_Data_p->StartCodeProcessing.PictureCounter = 0;
            if (VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset < (VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference + 1))
            {
                /* In case the last calculated ExtendedTemporalReference is higher than
                    the offset (ex: cut stream after Pn and Bn-2/Bn-1 missing), push the offset up */
                VIDDEC_Data_p->StartCodeProcessing.TemporalReferenceOffset = VIDDEC_Data_p->StartCodeProcessing.PreviousExtendedTemporalReference + 1;
            }
            VIDDEC_Data_p->StartCodeProcessing.LastReferenceTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
            VIDDEC_Data_p->StartCodeProcessing.PreviousTemporalReference = IMPOSSIBLE_VALUE_FOR_TEMPORAL_REFERENCE; /* Impossible value */
            break; /* end of DISCONTINUITY_START_CODE */

        default :
            /* No action for this start code */
            break;
    } /* end switch(StartCode) */

    /* To remember next time what was the previous start code */
    VIDDEC_Data_p->StartCodeProcessing.PreviousStartCode = StartCode;

#ifdef STVID_MEASURES
    MeasuresRB[MEASURE_END][MeasureIndexRB] = time_now();
    MeasuresRB[MEASURE_ID][MeasureIndexRB]  = MEASURE_ID_PROCESS_DECODE;
    if (MeasureIndexRB < NB_OF_MEASURES - 1)
    {
        MeasureIndexRB ++;
    }
#endif



    return(ST_NO_ERROR);
} /* End of viddec_ProcessStartCode() function */



/* Private functions -------------------------------------------------------- */

/*******************************************************************************
Name        : ComputeSequenceInformation
Description : Compute information from stream and store it in sequence structure
Parameters  : stream info, sequence info to fill
Assumptions : Pointers are not NULL
Limitations : Can be called only after the sequence in MPEG1, after the sequence_extension in MPEG2.
              Caution: field StreamID is not filled by this function !
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t ComputeSequenceInformation(const MPEG2BitStream_t * const Stream_p, STVID_SequenceInfo_t * const SequenceInfo_p, VIDDEC_SyntaxError_t * const SyntaxError_p)
{
    U32 Errors = 0;
    const Sequence_t * const CurrentSequence_p = &(Stream_p->Sequence);
    const SequenceExtension_t * const CurrentSequenceExtension_p = &(Stream_p->SequenceExtension);

    /* Compute sizes */
    SequenceInfo_p->Width  = CurrentSequence_p->horizontal_size_value;
    SequenceInfo_p->Height = CurrentSequence_p->vertical_size_value;
    if (!(Stream_p->MPEG1BitStream))
    {
        /* MPEG2 stream: add extension information */
        SequenceInfo_p->Width  += (CurrentSequenceExtension_p->horizontal_size_extension << 12);
        SequenceInfo_p->Height += (CurrentSequenceExtension_p->vertical_size_extension << 12);
    }

    /* Compute aspect */
    SyntaxError_p->PictureSyntaxError.HasRecoveredSequenceSyntaxErrors = FALSE;
    switch (CurrentSequence_p->aspect_ratio_information)
    {
        case ASPECT_RATIO_INFO_SQUARE :
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
            break;

        case ASPECT_RATIO_INFO_4TO3 :
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            break;

        case ASPECT_RATIO_INFO_16TO9 :
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
            break;

        case ASPECT_RATIO_INFO_221TO1 :
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
            break;

        default :
            /* Specially in MPEG1, many values can occur here */
            if ((!(Stream_p->MPEG1BitStream)) ||/* In MPEG2, all those other values corresponds to errors !? */
                (CurrentSequence_p->aspect_ratio_information == 0) || (CurrentSequence_p->aspect_ratio_information == 0xf)) /* In MPEG1, only 0 and 0xf are errors */
            {
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Aspect ratio of stream -%d- not supported: 4/3 is taken instead.", CurrentSequence_p->aspect_ratio_information));
                /* We are tolerant with this error because it leads to no display artefact but should only eventually lead to a bad resizing. */
                SyntaxError_p->PictureSyntaxError.HasRecoveredSequenceSyntaxErrors = TRUE;
            }
            SequenceInfo_p->Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            break;
    }

    /* Compute scan type */
    if ((Stream_p->MPEG1BitStream) || /* MPEG1 is always progressive */
        (CurrentSequenceExtension_p->progressive_sequence))  /* MPEG2 is progressive if this flag is set */
    {
        SequenceInfo_p->ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
    }
    else
    {
        /* MPEG2 and progressive flag not set */
        SequenceInfo_p->ScanType = STVID_SCAN_TYPE_INTERLACED;
    }

    /* Compute frame rate */
    switch (CurrentSequence_p->frame_rate_code)
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
            SequenceInfo_p->FrameRate = 50000;
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Frame rate of stream -%d- not supported: 50 Hz is taken instead.", CurrentSequence_p->frame_rate_code));
            Errors ++;

            /* Error_Recovery_Case_n01 : The frame rate of the sequence is not supported. */
            SyntaxError_p->PictureSyntaxError.HasCriticalSequenceSyntaxErrors = TRUE;
            break;
    }

    /* Compute bit rate */
    SequenceInfo_p->BitRate = CurrentSequence_p->bit_rate_value;
    if (!(Stream_p->MPEG1BitStream))
    {
        /* MPEG2 stream: add extension information */
        SequenceInfo_p->BitRate += (CurrentSequenceExtension_p->bit_rate_extension << 18);
    }


    /* MPEG standard, low delay */
    SequenceInfo_p->IsLowDelay = FALSE;
    if (Stream_p->MPEG1BitStream)
    {
        SequenceInfo_p->MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_11172;
    }
    else
    {
        SequenceInfo_p->MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_13818;
        if (CurrentSequenceExtension_p->low_delay)
        {
            SequenceInfo_p->IsLowDelay = TRUE;
        }
    }

    /* Compute VBVBufferSize */
    SequenceInfo_p->VBVBufferSize = CurrentSequence_p->vbv_buffer_size_value;
    if (!(Stream_p->MPEG1BitStream))
    {
        /* MPEG2 stream: add extension information */
        SequenceInfo_p->VBVBufferSize += (CurrentSequenceExtension_p->vbv_buffer_size_extension << 10);
    }

    /* Valid only in MPEG2 */
    if (Stream_p->MPEG1BitStream)
    {
        /* MPEG-1: no meaning for thos fields */
        SequenceInfo_p->ProfileAndLevelIndication = 0; /* no info */
        SequenceInfo_p->FrameRateExtensionN = 1;
        SequenceInfo_p->FrameRateExtensionD = 1;
        SequenceInfo_p->VideoFormat = VIDEO_FORMAT_UNSPECIFIED;
    }
    else
    {
        /* MPEG-2 */
        SequenceInfo_p->ProfileAndLevelIndication = CurrentSequenceExtension_p->profile_and_level_indication;
        SequenceInfo_p->FrameRateExtensionN = CurrentSequenceExtension_p->frame_rate_extension_n;
        SequenceInfo_p->FrameRateExtensionD = CurrentSequenceExtension_p->frame_rate_extension_d;
        if (Stream_p->HasSequenceDisplay)
        {
            SequenceInfo_p->VideoFormat = Stream_p->SequenceDisplayExtension.video_format;
        }
        else
        {
            SequenceInfo_p->VideoFormat = VIDEO_FORMAT_UNSPECIFIED;
        }
    }

    if (Errors != 0)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
        return(ST_NO_ERROR);
    }
} /* End of ComputeSequenceInformation() function */


/*******************************************************************************
Name        : FillPictureParamsStructures
Description : Copies info from stream and from computed sequence info into picture parameters structures
              Caution: uses data from pictures extensions (quant and PicDisplayExt), so those data must be valid in Stream_p !
Parameters  : stream info, sequence info, various params, picture structures to fill
Assumptions : Pointers are not NULL
Limitations : Caution: all params are set except:
              PictureInfos_p->BitmapParams fields Data1_p, Size1, Data2_p, Size2 are not set !
              STVID_PictureInfos_t.VideoParams field PTS is not set !
              STVID_PictureInfos_t.VideoParams field CompressionLevel and DecimationFactors are not set !
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
static ST_ErrorCode_t FillPictureParamsStructures(const VIDDEC_Handle_t DecodeHandle,
                                        const MPEG2BitStream_t * const Stream_p,
                                        STVID_PictureInfos_t * const PictureInfos_p,
                                        StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                        PictureStreamInfo_t * const PictureStreamInfo_p)
{

    U32 TopCounter = 0, BottomCounter = 0, RepeatFirstCounter = 0, RepeatProgressiveCounter = 0;
    U32 Errors = 0;
    U32 k;
    BOOL    HasNecessaryInformation;
    VIDDEC_Data_t * VIDDEC_Data_p = (VIDDEC_Data_t *) DecodeHandle;

    STVID_SequenceInfo_t * const SequenceInfo_p = &(VIDDEC_Data_p->StartCodeProcessing.SequenceInfo);

    /******************** Fiil in PictureInfos_p *****************************/

    PictureInfos_p->BitmapParams.Width  = SequenceInfo_p->Width;
    PictureInfos_p->BitmapParams.Height = SequenceInfo_p->Height;
    PictureInfos_p->BitmapParams.Pitch  = (SequenceInfo_p->Width + 15) & (~15);
    PictureInfos_p->BitmapParams.Offset = 0; /* No offset for MB type ! */

    /* Fill bitmap params */
    /* !!! ??? */
    PictureInfos_p->BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420; /* !!! */
    /* Default case.                         */
    PictureInfos_p->BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;

    PictureInfos_p->BitmapParams.PreMultipliedColor = FALSE;
    HasNecessaryInformation = FALSE;
    if (Stream_p->HasSequenceDisplay)
    {
        HasNecessaryInformation = TRUE;
        /* If has display extensions, has sequence and display one */
        switch (Stream_p->SequenceDisplayExtension.matrix_coefficients)
        {
            case MATRIX_COEFFICIENTS_FCC : /* Very close to ITU_R_BT601 matrixes */
/*                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_FCC;*/
                /* TO BE DELETED !!! */ /* Nearest until type STGXOBJ_FCC exist !!! */ PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
                break;

            case MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG : /* Same matrixes as ITU_R_BT601 */
                /* Not worth it, should take 601 ! */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
                break;

            case MATRIX_COEFFICIENTS_SMPTE_170M : /* Same matrixes as ITU_R_BT601 */
                /* Not worth it, should take 601 ! */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                break;

            case MATRIX_COEFFICIENTS_SMPTE_240M : /* Slightly differ from ITU_R_BT709 */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_240M;
                break;

            case MATRIX_COEFFICIENTS_ITU_R_BT_709 :
            case MATRIX_COEFFICIENTS_UNSPECIFIED :
            default : /* 709 seems to be the most suitable for the default case */
                HasNecessaryInformation = FALSE;
                break;
        }
    } /* end display extension */
    else if (Stream_p->MPEG1BitStream)
    {
        /* Case MPEG-1: ColorSpaceConversion is always CCIR 601 (by the norm) */
        HasNecessaryInformation = TRUE;
        PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    }
    if (!(HasNecessaryInformation))
    {
        /* When no sequence display or no relevant matrix coef information, depends on the broadcaster norm */
        switch (VIDDEC_Data_p->BroadcastProfile)
        {
#ifdef STVID_DIRECTV
            case STVID_BROADCAST_DIRECTV : /* DirecTV super-sets MPEG-2 norm */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                break;
#endif /* STVID_DIRECTV */
            case STVID_BROADCAST_DVB : /* DVB super-sets MPEG-2 norm */
                switch (Stream_p->SequenceExtension.profile_and_level_indication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
                {
                    case (PROFILE_MAIN | LEVEL_MAIN) : /* MP@ML , SD */
                        switch (Stream_p->Sequence.frame_rate_code)
                        {
                            case FRAME_RATE_CODE_25 : /* 25 Hz */
                            case FRAME_RATE_CODE_50 :
                                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
                                break;

                            default : /* 30 Hz */
                                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                                break;
                        }
                        break;

                    default : /* HD */
                        PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
                        break;
                }
                break;

            case STVID_BROADCAST_ATSC : /* ATSC follows MPEG-2 norm */
            case STVID_BROADCAST_ARIB : /* ARIB follows MPEG-2 norm */
            default :   /* Default in MPEG-2 norm */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
                break;
        } /* end switch (BroadcastProfile) */
    } /* end set default ColorSpaceConversion depending on profile */

    switch (SequenceInfo_p->Aspect)
    {
        case STVID_DISPLAY_ASPECT_RATIO_16TO9 :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
            break;

        case STVID_DISPLAY_ASPECT_RATIO_SQUARE :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
            break;

        case STVID_DISPLAY_ASPECT_RATIO_221TO1 :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_221TO1;
            break;

/*        case STVID_DISPLAY_ASPECT_RATIO_4TO3 :*/
        default :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
            break;
    }

    /* Fill video params */
    PictureInfos_p->VideoParams.FrameRate   = SequenceInfo_p->FrameRate;

    /* Compute scan type */
    switch (SequenceInfo_p->ScanType)
    {
        case STVID_SCAN_TYPE_PROGRESSIVE :
            PictureInfos_p->VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
            break;

/*      case STVID_SCAN_TYPE_INTERLACED :*/
        default :
            /* Take care of picture coding extension information. */
            if (!Stream_p->MPEG1BitStream)
            {
                /* MPEG2 case... Take care of the progressive_frame field, from  */
                /* The picture coding extension. ScanType of the current picture */
                /* depends on it !!!                                             */

#if defined(STVID_FORCE_INTERLACED_SCAN)

                PictureInfos_p->VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
#else
                if (Stream_p->PictureCodingExtension.progressive_frame)
                {
                    PictureInfos_p->VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
                }
                else
                {
                    PictureInfos_p->VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
                }
#endif  /* #if defined(STVID_FORCE_INTERLACED_SCAN) */

            }
            else
            {
                /* This should not be used. */
                PictureInfos_p->VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
            }
            break;
    }

    /* test HD-PIP cases.                                           */
    if (VIDDEC_Data_p->HDPIPParams.Enable)
    {
        /* Remarks : HDPIP feature is only available on STi7020 based plateforms    */
        /* and is restricted to interlaced management, i.e. whatever the input      */
        /* scantype, the stream will be decoded as interlaced, and reconstructed    */
        /* frame buffer presented as interlaced too !!!                             */

        /* Test if HD-PIP is to be activated.                       */
        if ((SequenceInfo_p->Width >= VIDDEC_Data_p->HDPIPParams.WidthThreshold) ||
            (SequenceInfo_p->Height >= VIDDEC_Data_p->HDPIPParams.HeightThreshold) )
        {
            /* Yes. Change the bitmap type.                         */
            PictureInfos_p->BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB_HDPIP;

            /* Modify physical size of the buffer in picture infos. */
            PictureInfos_p->BitmapParams.Width  /= 4;
            PictureInfos_p->BitmapParams.Height /= 1;
            PictureInfos_p->BitmapParams.Pitch  /= 4;

            /* Moreover, in any case, consider input as Interlaced. */
            PictureInfos_p->VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
        }
    }

    /* Compute MPEG coding type */
    switch (Stream_p->Picture.picture_coding_type)
    {
        case PICTURE_CODING_TYPE_I :
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
            break;

        case PICTURE_CODING_TYPE_P :
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
            break;

        case PICTURE_CODING_TYPE_B :
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
            break;

        default :
            /* By default say an unknown picture is a B picture,             */
            /* so that it is not re-used as reference as it may be spoiled   */
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
            /* In MPEG2, all those other values corresponds to errors
               In MPEG1, D picture may occur ? (anyway we don't support it) */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MPEG picture type -%d- not supported: B is taken instead.", Stream_p->Picture.picture_coding_type));
            Errors ++;
            break;
    }

    /* Compute TopFieldFirst and field counters */
    if (Stream_p->MPEG1BitStream)
    {
        /* Picture coding extension not valid in MPEG1 */
        TopCounter    = 1;
        BottomCounter = 1;
        PictureInfos_p->VideoParams.TopFieldFirst = TRUE;
        /* Be sure RepeatProgressiveCounter is 0: nothing to do */
    }
    else
    {
/*
        Synthesis of the MPEG2 Standard giving the duration of the field or frame in unit of field (X means forbidden) :

        picture_structure       Reserved ( = 0)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) X   X   X   X   X   X   X   X   X   X   X   X   X   X   X   X
        X becomes 2

        picture_structure       Top Field ( = 1)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) 1   X   X   X   X   X   X   X   X   X   X   X   X   X   X   X
        X becomes 1

        picture_structure       Bottom Field ( = 2)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) 1   X   X   X   X   X   X   X   X   X   X   X   X   X   X   X
        X becomes 1

        picture_structure       Frame ( = 3)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) 2   2   X   3   2   2   X   3   2   2   X   4   X   X   X   6
        X becomes 2
*/
        switch (Stream_p->PictureCodingExtension.picture_structure)
        {
            case PICTURE_STRUCTURE_TOP_FIELD :
                /* Field picture: top_field_first is 0, decode outputs one field picture */
                PictureInfos_p->VideoParams.TopFieldFirst = TRUE;
                TopCounter = 1;
                BottomCounter = 0;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = 0;
                break;

            case PICTURE_STRUCTURE_BOTTOM_FIELD :
                /* Field picture: top_field_first is 0, decode outputs one field picture */
                PictureInfos_p->VideoParams.TopFieldFirst = FALSE;
                TopCounter = 0;
                BottomCounter = 1;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = 0;
                break;

            case PICTURE_STRUCTURE_FRAME_PICTURE :
            default :
                /* Frame picture: top_field_first indicates which field is output first */
                PictureInfos_p->VideoParams.TopFieldFirst = Stream_p->PictureCodingExtension.top_field_first;
                /* Two fields: first field, second field */
                TopCounter    = 1;
                BottomCounter = 1;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = 0;
                if (SequenceInfo_p->ScanType != STVID_SCAN_TYPE_PROGRESSIVE)
                {
                    if ((Stream_p->PictureCodingExtension.progressive_frame) &&
                        (Stream_p->PictureCodingExtension.repeat_first_field))
                    {
                        /* Three fields: first field, second field, first field */
                        RepeatFirstCounter = 1;
                    }
                }
                else
                {
                    if ((Stream_p->PictureCodingExtension.repeat_first_field) &&
                        (Stream_p->PictureCodingExtension.progressive_frame) )
                    {
                        if (Stream_p->PictureCodingExtension.top_field_first)
                        {
                            /* Three identical progressive frames */
                            RepeatProgressiveCounter = 2;
                        }
                        else
                        {
                            /* Two identical progressive frames */
                            RepeatProgressiveCounter = 1;
                        }
                    }
                }
                break;
        } /* end switch(picture_structure) */

#ifdef TRACE_UART
        /* Error Recovery Case : */
        if ((Stream_p->PictureCodingExtension.picture_structure == 0) ||
            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_TOP_FIELD) &&
                        ((SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) ||
                         (Stream_p->PictureCodingExtension.top_field_first) ||
                         (Stream_p->PictureCodingExtension.repeat_first_field) ||
                         (Stream_p->PictureCodingExtension.progressive_frame))) ||
            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_BOTTOM_FIELD) &&
                        ((SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) ||
                         (Stream_p->PictureCodingExtension.top_field_first) ||
                         (Stream_p->PictureCodingExtension.repeat_first_field) ||
                         (Stream_p->PictureCodingExtension.progressive_frame))) ||
            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) &&
                        ((Stream_p->PictureCodingExtension.repeat_first_field) && (!(Stream_p->PictureCodingExtension.progressive_frame))) ||
                        ((SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) && (!(Stream_p->PictureCodingExtension.repeat_first_field)) && (Stream_p->PictureCodingExtension.top_field_first))))
        {
            TraceBuffer(("***Parsing Error***"));
        }
#endif /* TRACE_UART */

    } /* else if MPEG1 */

    /* Compute picture structure */
    if (Stream_p->MPEG1BitStream)
    {
        /* Picture coding extension not valid in MPEG1 */
        PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
    }
    else
    {
        switch (Stream_p->PictureCodingExtension.picture_structure)
        {
            case PICTURE_STRUCTURE_TOP_FIELD :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
                break;

            case PICTURE_STRUCTURE_BOTTOM_FIELD :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
                break;

            case PICTURE_STRUCTURE_FRAME_PICTURE :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
                break;

            default :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Picture structure -%d- not supported: Frame picture is taken instead.", Stream_p->PictureCodingExtension.picture_structure));
                Errors ++;
                break;
        }
    }

    PictureInfos_p->VideoParams.TimeCode            =
            VIDDEC_Data_p->StartCodeProcessing.TimeCode;

    /******************** Fiil in StreamInfoForDecode_p ***********************/

    if (Stream_p->IntraQuantMatrixHasChanged)
    {
        StreamInfoForDecode_p->IntraQuantMatrixHasChanged = TRUE;
        if (Stream_p->LumaIntraQuantMatrix_p == Stream_p->DefaultIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->LumaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->intra_quantizer_matrix)[k] = (*(Stream_p->LumaIntraQuantMatrix_p))[k];
            }
        }
        if (Stream_p->ChromaIntraQuantMatrix_p == Stream_p->DefaultIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->ChromaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->chroma_intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->chroma_intra_quantizer_matrix)[k] = (*(Stream_p->ChromaIntraQuantMatrix_p))[k];
            }
        }
    }
    if (Stream_p->NonIntraQuantMatrixHasChanged)
    {
        StreamInfoForDecode_p->NonIntraQuantMatrixHasChanged = TRUE;
        if (Stream_p->LumaNonIntraQuantMatrix_p == Stream_p->DefaultNonIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->LumaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->non_intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->non_intra_quantizer_matrix)[k] = (*(Stream_p->LumaNonIntraQuantMatrix_p))[k];
            }
        }
        if (Stream_p->ChromaNonIntraQuantMatrix_p == Stream_p->DefaultNonIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->ChromaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->chroma_non_intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->chroma_non_intra_quantizer_matrix)[k] = (*(Stream_p->ChromaNonIntraQuantMatrix_p))[k];
            }
        }
    }
    StreamInfoForDecode_p->MPEG1BitStream = Stream_p->MPEG1BitStream;
    StreamInfoForDecode_p->HorizontalSize = SequenceInfo_p->Width;
    StreamInfoForDecode_p->VerticalSize   = SequenceInfo_p->Height;
    StreamInfoForDecode_p->Picture        = Stream_p->Picture;
    /* Set picture_coding_extension */
    if (!(Stream_p->MPEG1BitStream))
    {
        /* picture_coding_extension only valid in MPEG2 */
        StreamInfoForDecode_p->PictureCodingExtension = Stream_p->PictureCodingExtension;
#ifdef ST_deblock
        StreamInfoForDecode_p->progressive_sequence = Stream_p->SequenceExtension.progressive_sequence;
        StreamInfoForDecode_p->chroma_format        = Stream_p->SequenceExtension.chroma_format;
#endif /* ST_deblock */
    }
    StreamInfoForDecode_p->ExpectingSecondField =
            VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField;


    /******************** Fiil in PictureStreamInfo_p **********************/

    PictureStreamInfo_p->HasSequenceDisplay = Stream_p->HasSequenceDisplay;

    if (Stream_p->HasSequenceDisplay)
    {
        /* If has display extensions, has sequence and display one */
        PictureStreamInfo_p->SequenceDisplayExtension = Stream_p->SequenceDisplayExtension;

        /* Check for available display_vertical_size  ...           */
        if (PictureStreamInfo_p->SequenceDisplayExtension.display_vertical_size == 0)
        {
            /* If null value, be sure the display area will be the  */
            /* entire source picture size.                          */
            PictureStreamInfo_p->SequenceDisplayExtension.display_vertical_size =
                    SequenceInfo_p->Height;
        }
        /* Check for available display_horizontal_size...           */
        if (PictureStreamInfo_p->SequenceDisplayExtension.display_horizontal_size == 0)
        {
            /* If null value, be sure the display area will be the  */
            /* entire source picture size.                          */
            PictureStreamInfo_p->SequenceDisplayExtension.display_horizontal_size =
                    SequenceInfo_p->Width;
        }
    }
    /* Picture display considered always there, reset to 0 values if not available in stream */
    PictureStreamInfo_p->PictureDisplayExtension  = Stream_p->PictureDisplayExtension;

    PictureStreamInfo_p->ExtendedTemporalReference =
            VIDDEC_Data_p->StartCodeProcessing.ExtendedTemporalReference;
    /* Field counters calculated above with TopFieldFirst */
    PictureStreamInfo_p->TopFieldCounter    = TopCounter;
    PictureStreamInfo_p->BottomFieldCounter = BottomCounter;
    PictureStreamInfo_p->RepeatFieldCounter = RepeatFirstCounter;
    PictureStreamInfo_p->RepeatProgressiveCounter = RepeatProgressiveCounter;
    PictureStreamInfo_p->ExpectingSecondField =
            VIDDEC_Data_p->StartCodeProcessing.ExpectingSecondField;
    PictureStreamInfo_p->IsBitStreamMPEG1   = Stream_p->MPEG1BitStream;

    VIDDEC_Data_p->NbFieldsDisplayNextDecode
            = (TopCounter + BottomCounter + RepeatFirstCounter) * (RepeatProgressiveCounter + 1);

    if (Errors != 0)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
        return(ST_NO_ERROR);
    }
} /* End of FillPictureParamsStructures() function */

#ifdef ST_SecondInstanceSharingTheSameDecoder
/*******************************************************************************
Name        : FillPictureParamsStructuresForStill
Description : Copies info from stream and from computed sequence info into picture parameters structures
              Caution: uses data from pictures extensions (quant and PicDisplayExt), so those data must be valid in Stream_p !
Parameters  : stream info, sequence info, various params, picture structures to fill
Assumptions : Pointers are not NULL
Limitations : Caution: all params are set except:
              PictureInfos_p->BitmapParams fields Data1_p, Size1, Data2_p, Size2 are not set !
              STVID_PictureInfos_t.VideoParams field PTS is not set !
              STVID_PictureInfos_t.VideoParams field CompressionLevel and DecimationFactors are not set !
Returns     : ST_NO_ERROR if success, ST_ERROR_BAD_PARAMETER if problem
*******************************************************************************/
ST_ErrorCode_t FillPictureParamsStructuresForStill(  StillDecodeParams_t StillDecodeParams ,
                                    STVID_SequenceInfo_t * const SequenceInfo_p,
                                        const MPEG2BitStream_t * const Stream_p,
                                        STVID_PictureInfos_t * const PictureInfos_p,
                                        StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                        PictureStreamInfo_t * const PictureStreamInfo_p)
{

    U32 TopCounter = 0, BottomCounter = 0, RepeatFirstCounter = 0, RepeatProgressiveCounter = 0;
    U32 Errors = 0;
    U32 k;
    BOOL    HasNecessaryInformation;



    /******************** Fill in PictureInfos_p *****************************/

    PictureInfos_p->BitmapParams.Width  = SequenceInfo_p->Width;
    PictureInfos_p->BitmapParams.Height = SequenceInfo_p->Height;
    PictureInfos_p->BitmapParams.Pitch  = (SequenceInfo_p->Width + 15) & (~15); /* Pitch is macroblock multiple */
    PictureInfos_p->BitmapParams.Offset = 0; /* No offset for MB type ! */

    /* Fill bitmap params */
    /* !!! ??? */
    PictureInfos_p->BitmapParams.ColorType = STGXOBJ_COLOR_TYPE_UNSIGNED_YCBCR888_420; /* !!! */
    /* Default case.                         */
    PictureInfos_p->BitmapParams.BitmapType = STGXOBJ_BITMAP_TYPE_MB;

    PictureInfos_p->BitmapParams.PreMultipliedColor = FALSE;
    HasNecessaryInformation = FALSE;
    if (Stream_p->HasSequenceDisplay)
    {
        HasNecessaryInformation = TRUE;
        /* If has display extensions, has sequence and display one */
        switch (Stream_p->SequenceDisplayExtension.matrix_coefficients)
        {
            case MATRIX_COEFFICIENTS_UNSPECIFIED :
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_CONVERSION_MODE_UNKNOWN;
                break;

            case MATRIX_COEFFICIENTS_FCC : /* Very close to ITU_R_BT601 matrixes */
/*                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_FCC;*/
                /* TO BE DELETED !!! */ /* Nearest until type STGXOBJ_FCC exist !!! */ PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
                break;

            case MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG : /* Same matrixes as ITU_R_BT601 */
                /* Not worth it, should take 601 ! */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
                break;

            case MATRIX_COEFFICIENTS_SMPTE_170M : /* Same matrixes as ITU_R_BT601 */
                /* Not worth it, should take 601 ! */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                break;

            case MATRIX_COEFFICIENTS_SMPTE_240M : /* Slightly differ from ITU_R_BT709 */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_240M;
                break;

            case MATRIX_COEFFICIENTS_ITU_R_BT_709 :
            default : /* 709 seems to be the most suitable for the default case */
                HasNecessaryInformation = FALSE;
                break;
        }
    } /* end display extension */
    else if (Stream_p->MPEG1BitStream)
    {
        /* Case MPEG-1: ColorSpaceConversion is always CCIR 601 (by the norm) */
        HasNecessaryInformation = TRUE;
        PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
    }
    if (!(HasNecessaryInformation))
    {
        /* When no sequence display or no relevant matrix coef information, depends on the broadcaster norm */
        switch (StillDecodeParams.BroadcastProfile)
        {
#ifdef STVID_DIRECTV
            case STVID_BROADCAST_DIRECTV : /* DirecTV super-sets MPEG-2 norm */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                break;
#endif /* STVID_DIRECTV */
            case STVID_BROADCAST_DVB : /* DVB super-sets MPEG-2 norm */
                switch (Stream_p->SequenceExtension.profile_and_level_indication & PROFILE_AND_LEVEL_IDENTIFICATION_MASK)
                {
                    case (PROFILE_MAIN | LEVEL_MAIN) : /* MP@ML , SD */
                        switch (Stream_p->Sequence.frame_rate_code)
                        {
                            case FRAME_RATE_CODE_25 : /* 25 Hz */
                            case FRAME_RATE_CODE_50 :
                                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
                                break;

                            default : /* 30 Hz */
                                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
                                break;
                        }
                        break;

                    default : /* HD */
                        PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
                        break;
                }
                break;

            case STVID_BROADCAST_ATSC : /* ATSC follows MPEG-2 norm */
            case STVID_BROADCAST_ARIB : /* ARIB follows MPEG-2 norm */
            default :   /* Default in MPEG-2 norm */
                PictureInfos_p->BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
                break;
        } /* end switch (BroadcastProfile) */
    } /* end set default ColorSpaceConversion depending on profile */

    switch (SequenceInfo_p->Aspect)
    {
        case STVID_DISPLAY_ASPECT_RATIO_16TO9 :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
            break;

        case STVID_DISPLAY_ASPECT_RATIO_SQUARE :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
            break;

        case STVID_DISPLAY_ASPECT_RATIO_221TO1 :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_221TO1;
            break;

/*        case STVID_DISPLAY_ASPECT_RATIO_4TO3 :*/
        default :
            PictureInfos_p->BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
            break;
    }

    /* Fill video params */
    PictureInfos_p->VideoParams.FrameRate   = SequenceInfo_p->FrameRate;

    /* Compute scan type */
    switch (SequenceInfo_p->ScanType)
    {
        case STVID_SCAN_TYPE_PROGRESSIVE :
            PictureInfos_p->VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
            break;

/*      case STVID_SCAN_TYPE_INTERLACED :*/
        default :
            /* Take care of picture coding extension information. */
            if (!Stream_p->MPEG1BitStream)
            {
                /* MPEG2 case... Take care of the progressive_frame field, from  */
                /* The picture coding extension. ScanType of the current picture */
                /* depends on it !!!                                             */
                if (Stream_p->PictureCodingExtension.progressive_frame)
                {
                    PictureInfos_p->VideoParams.ScanType = STGXOBJ_PROGRESSIVE_SCAN;
                }
                else
                {
                    PictureInfos_p->VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
                }
            }
            else
            {
                /* This should not be used. */
                PictureInfos_p->VideoParams.ScanType = STGXOBJ_INTERLACED_SCAN;
            }
            break;
    }


    /* Compute MPEG coding type */
    switch (Stream_p->Picture.picture_coding_type)
    {
        case PICTURE_CODING_TYPE_I :
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
            break;

        case PICTURE_CODING_TYPE_P :
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
            break;

        case PICTURE_CODING_TYPE_B :
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
            break;

        default :
            /* By default say an unknown picture is a B picture,             */
            /* so that it is not re-used as reference as it may be spoiled   */
            PictureInfos_p->VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
            /* In MPEG2, all those other values corresponds to errors
               In MPEG1, D picture may occur ? (anyway we don't support it) */
            STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "MPEG picture type -%d- not supported: B is taken instead.", Stream_p->Picture.picture_coding_type));
            Errors ++;
            break;
    }

    /* Compute TopFieldFirst and field counters */
    if (Stream_p->MPEG1BitStream)
    {
        /* Picture coding extension not valid in MPEG1 */
        TopCounter    = 1;
        BottomCounter = 1;
        PictureInfos_p->VideoParams.TopFieldFirst = TRUE;
        /* Be sure RepeatProgressiveCounter is 0: nothing to do */
    }
    else
    {
/*
        Synthesis of the MPEG2 Standard giving the duration of the field or frame in unit of field (X means forbidden) :

        picture_structure       Reserved ( = 0)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) X   X   X   X   X   X   X   X   X   X   X   X   X   X   X   X
        X becomes 2

        picture_structure       Top Field ( = 1)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) 1   X   X   X   X   X   X   X   X   X   X   X   X   X   X   X
        X becomes 1

        picture_structure       Bottom Field ( = 2)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) 1   X   X   X   X   X   X   X   X   X   X   X   X   X   X   X
        X becomes 1

        picture_structure       Frame ( = 3)
        progressive_sequence    0   0   0   0   0   0   0   0   1   1   1   1   1   1   1   1
        top_field_first         0   0   0   0   1   1   1   1   0   0   0   0   1   1   1   1
        repeat_first_field      0   0   1   1   0   0   1   1   0   0   1   1   0   0   1   1
        progressive_frame       0   1   0   1   0   1   0   1   0   1   0   1   0   1   0   1
        Duration (nb of fields) 2   2   X   3   2   2   X   3   2   2   X   4   X   X   X   6
        X becomes 2
*/
        switch (Stream_p->PictureCodingExtension.picture_structure)
        {
            case PICTURE_STRUCTURE_TOP_FIELD :
                /* Field picture: top_field_first is 0, decode outputs one field picture */
                PictureInfos_p->VideoParams.TopFieldFirst = TRUE;
                TopCounter = 1;
                BottomCounter = 0;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = 0;
                break;

            case PICTURE_STRUCTURE_BOTTOM_FIELD :
                /* Field picture: top_field_first is 0, decode outputs one field picture */
                PictureInfos_p->VideoParams.TopFieldFirst = FALSE;
                TopCounter = 0;
                BottomCounter = 1;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = 0;
                break;

            case PICTURE_STRUCTURE_FRAME_PICTURE :
            default :
                /* Frame picture: top_field_first indicates which field is output first */
                PictureInfos_p->VideoParams.TopFieldFirst = Stream_p->PictureCodingExtension.top_field_first;
                /* Two fields: first field, second field */
                TopCounter    = 1;
                BottomCounter = 1;
                RepeatFirstCounter = 0;
                RepeatProgressiveCounter = 0;
                if (SequenceInfo_p->ScanType != STVID_SCAN_TYPE_PROGRESSIVE)
                {
                    if ((Stream_p->PictureCodingExtension.progressive_frame) &&
                        (Stream_p->PictureCodingExtension.repeat_first_field))
                    {
                        /* Three fields: first field, second field, first field */
                        RepeatFirstCounter = 1;
                    }
                }
                else
                {
                    if ((Stream_p->PictureCodingExtension.repeat_first_field) &&
                        (Stream_p->PictureCodingExtension.progressive_frame) )
                    {
                        if (Stream_p->PictureCodingExtension.top_field_first)
                        {
                            /* Three identical progressive frames */
                            RepeatProgressiveCounter = 2;
                        }
                        else
                        {
                            /* Two identical progressive frames */
                            RepeatProgressiveCounter = 1;
                        }
                    }
                }
                break;
        } /* end switch(picture_structure) */

#ifdef TRACE_UART
        /* Error Recovery Case : */
        if ((Stream_p->PictureCodingExtension.picture_structure == 0) ||
            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_TOP_FIELD) &&
                        ((SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) ||
                         (Stream_p->PictureCodingExtension.top_field_first) ||
                         (Stream_p->PictureCodingExtension.repeat_first_field) ||
                         (Stream_p->PictureCodingExtension.progressive_frame))) ||
            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_BOTTOM_FIELD) &&
                        ((SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) ||
                         (Stream_p->PictureCodingExtension.top_field_first) ||
                         (Stream_p->PictureCodingExtension.repeat_first_field) ||
                         (Stream_p->PictureCodingExtension.progressive_frame))) ||
            ((Stream_p->PictureCodingExtension.picture_structure == PICTURE_STRUCTURE_FRAME_PICTURE) &&
                        ((Stream_p->PictureCodingExtension.repeat_first_field) && (!(Stream_p->PictureCodingExtension.progressive_frame))) ||
                        ((SequenceInfo_p->ScanType == STVID_SCAN_TYPE_PROGRESSIVE) && (!(Stream_p->PictureCodingExtension.repeat_first_field)) && (Stream_p->PictureCodingExtension.top_field_first))))
        {
            TraceBuffer(("***Parsing Error***"));
        }
#endif /* TRACE_UART */

    } /* else if MPEG1 */

    /* Compute picture structure */
    if (Stream_p->MPEG1BitStream)
    {
        /* Picture coding extension not valid in MPEG1 */
        PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
    }
    else
    {
        switch (Stream_p->PictureCodingExtension.picture_structure)
        {
            case PICTURE_STRUCTURE_TOP_FIELD :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
                break;

            case PICTURE_STRUCTURE_BOTTOM_FIELD :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_BOTTOM_FIELD;
                break;

            case PICTURE_STRUCTURE_FRAME_PICTURE :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
                break;

            default :
                PictureInfos_p->VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
                STTBX_Report((STTBX_REPORT_LEVEL_ERROR, "Picture structure -%d- not supported: Frame picture is taken instead.", Stream_p->PictureCodingExtension.picture_structure));
                Errors ++;
                break;
        }
    }

    PictureInfos_p->VideoParams.TimeCode            =
            StillDecodeParams.StillStartCodeProcessing.TimeCode;

    /******************** Fiil in StreamInfoForDecode_p ***********************/

    if (Stream_p->IntraQuantMatrixHasChanged)
    {
        StreamInfoForDecode_p->IntraQuantMatrixHasChanged = TRUE;
        if (Stream_p->LumaIntraQuantMatrix_p == Stream_p->DefaultIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->LumaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->LumaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->intra_quantizer_matrix)[k] = (*(Stream_p->LumaIntraQuantMatrix_p))[k];
            }
        }
        if (Stream_p->ChromaIntraQuantMatrix_p == Stream_p->DefaultIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->ChromaIntraQuantMatrix_p = Stream_p->DefaultIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->ChromaIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->chroma_intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->chroma_intra_quantizer_matrix)[k] = (*(Stream_p->ChromaIntraQuantMatrix_p))[k];
            }
        }
    }
    if (Stream_p->NonIntraQuantMatrixHasChanged)
    {
        StreamInfoForDecode_p->NonIntraQuantMatrixHasChanged = TRUE;
        if (Stream_p->LumaNonIntraQuantMatrix_p == Stream_p->DefaultNonIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->LumaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->LumaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->non_intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->non_intra_quantizer_matrix)[k] = (*(Stream_p->LumaNonIntraQuantMatrix_p))[k];
            }
        }
        if (Stream_p->ChromaNonIntraQuantMatrix_p == Stream_p->DefaultNonIntraQuantMatrix_p)
        {
            StreamInfoForDecode_p->ChromaNonIntraQuantMatrix_p = Stream_p->DefaultNonIntraQuantMatrix_p;
        }
        else
        {
            StreamInfoForDecode_p->ChromaNonIntraQuantMatrix_p = (const QuantiserMatrix_t *) &(StreamInfoForDecode_p->chroma_non_intra_quantizer_matrix);
            for (k = 0; k < QUANTISER_MATRIX_SIZE; k ++)
            {
                (StreamInfoForDecode_p->chroma_non_intra_quantizer_matrix)[k] = (*(Stream_p->ChromaNonIntraQuantMatrix_p))[k];
            }
        }
    }
    StreamInfoForDecode_p->MPEG1BitStream = Stream_p->MPEG1BitStream;
    StreamInfoForDecode_p->HorizontalSize = SequenceInfo_p->Width;
    StreamInfoForDecode_p->VerticalSize   = SequenceInfo_p->Height;
    StreamInfoForDecode_p->Picture        = Stream_p->Picture;

    /* Set picture_coding_extension */
    if (!(Stream_p->MPEG1BitStream))
    {
        /* picture_coding_extension only valid in MPEG2 */
        StreamInfoForDecode_p->PictureCodingExtension = Stream_p->PictureCodingExtension;
#ifdef ST_deblock
        StreamInfoForDecode_p->progressive_sequence = Stream_p->SequenceExtension.progressive_sequence;
        StreamInfoForDecode_p->chroma_format        = Stream_p->SequenceExtension.chroma_format;
#endif /* ST_deblock */

    }
    StreamInfoForDecode_p->ExpectingSecondField =
            StillDecodeParams.StillStartCodeProcessing.ExpectingSecondField;


    /******************** Fiil in PictureStreamInfo_p **********************/

    PictureStreamInfo_p->HasSequenceDisplay = Stream_p->HasSequenceDisplay;

    if (Stream_p->HasSequenceDisplay)
    {
        /* If has display extensions, has sequence and display one */
        PictureStreamInfo_p->SequenceDisplayExtension = Stream_p->SequenceDisplayExtension;

        /* Check for available display_vertical_size  ...           */
        if (PictureStreamInfo_p->SequenceDisplayExtension.display_vertical_size == 0)
        {
            /* If null value, be sure the display area will be the  */
            /* entire source picture size.                          */
            PictureStreamInfo_p->SequenceDisplayExtension.display_vertical_size =
                    SequenceInfo_p->Height;
        }
        /* Check for available display_horizontal_size...           */
        if (PictureStreamInfo_p->SequenceDisplayExtension.display_horizontal_size == 0)
        {
            /* If null value, be sure the display area will be the  */
            /* entire source picture size.                          */
            PictureStreamInfo_p->SequenceDisplayExtension.display_horizontal_size =
                    SequenceInfo_p->Width;
        }
    }
    /* Picture display considered always there, reset to 0 values if not available in stream */
    PictureStreamInfo_p->PictureDisplayExtension  = Stream_p->PictureDisplayExtension;

    PictureStreamInfo_p->ExtendedTemporalReference =
            StillDecodeParams.StillStartCodeProcessing.ExtendedTemporalReference;
    /* Field counters calculated above with TopFieldFirst */
    PictureStreamInfo_p->TopFieldCounter    = TopCounter;
    PictureStreamInfo_p->BottomFieldCounter = BottomCounter;
    PictureStreamInfo_p->RepeatFieldCounter = RepeatFirstCounter;
    PictureStreamInfo_p->RepeatProgressiveCounter = RepeatProgressiveCounter;
    PictureStreamInfo_p->ExpectingSecondField =
            StillDecodeParams.StillStartCodeProcessing.ExpectingSecondField;
    PictureStreamInfo_p->IsBitStreamMPEG1   = Stream_p->MPEG1BitStream;

    StillDecodeParams.NbFieldsDisplayNextDecode
            = (TopCounter + BottomCounter + RepeatFirstCounter) * (RepeatProgressiveCounter + 1);

    if (Errors != 0)
    {
        return(ST_ERROR_BAD_PARAMETER);
    }
    else
    {
        return(ST_NO_ERROR);
    }
} /* End of FillPictureParamsStructuresForStill() function */
#endif /* ST_SecondInstanceSharingTheSameDecoder */



/* End of vid_sc.c */
