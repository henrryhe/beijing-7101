/*!
 *******************************************************************************
 * \file parse_sps.c
 *
 * \brief H264 SPS parsing and filling of data structures
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 *******************************************************************************
 */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

/* H264 Parser specific */
#include "h264parser.h"

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

const U32 ZZ_SCAN[16]  =
{  0,  1,  4,  8,  5,  2,  3,  6,  9, 12, 13, 10,  7, 11, 14, 15
};

const U32 ZZ_SCAN8[64] =
{  0,  1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

/* Constant to check TimeScale to prevent U32 overflow in Frame rate calculation */
#define MAX_TIME_SCALE_TO_PREVENT_OVERFLOW 4294967 /* =int(max U32 /1000) */

/* Functions ---------------------------------------------------------------- */
static void h264par_InitializeDefaultHRDParameters(U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle);

/******************************************************************************/
/*! \brief Set scaling Matrix to Flat
 *
 * Set scaling Matrix to Flat (default for MAIN profile and HIGH profile
 *         without scaling Matrix)
 *
 * \param ScalingMatrix to be set
 * \return void
 */
/******************************************************************************/
void h264par_SetFlatScalingMatrix(H264_SpecificScalingMatrix_t *ScalingMatrix)
{
    U32 ScalingListCounter;
    U32 CoefCounter;

    for(ScalingListCounter = 0 ; ScalingListCounter < 8 ; ScalingListCounter++)
    {
        if(ScalingListCounter < 6)
        {
            for(CoefCounter = 0 ; CoefCounter < 16 ; CoefCounter++)
            {
                ScalingMatrix->FirstSixScalingList[ScalingListCounter][CoefCounter] = 16;
            }
        }
        else
        {
            for(CoefCounter = 0 ; CoefCounter < 64 ; CoefCounter++)
            {
                ScalingMatrix->NextTwoScalingList[ScalingListCounter-6][CoefCounter] = 16;
            }
        }
    }
}

/******************************************************************************/
/*! \brief Set scaling List to default values
 *
 * \param ScalingMatrix where resides the scaling list to be set
 * \param ScalingListNumber to be set
 * \return void
 */
/******************************************************************************/
void h264par_SetScalingListDefault(H264_SpecificScalingMatrix_t *ScalingMatrix, U32 ScalingListNumber)
{
    static U32 DefaultScalingList4x4Intra[16] = { 6, 13, 13, 20,
                                                 20, 20, 28, 28,
                                                 28, 28, 32, 32,
                                                 32, 37, 37, 42};
    static U32 DefaultScalingList4x4Inter[16] = {10, 14, 14, 20,
                                                 20, 20, 24, 24,
                                                 24, 24, 27, 27,
                                                 27, 30, 30, 34};
    static U32 DefaultScalingList8x8Intra[64] = { 6, 10, 10, 13, 11, 13, 16, 16,
                                                 16, 16, 18, 18, 18, 18, 18, 23,
                                                 23, 23, 23, 23, 23, 25, 25, 25,
                                                 25, 25, 25, 25, 27, 27, 27, 27,
                                                 27, 27, 27, 27, 29, 29 ,29, 29,
                                                 29, 29, 29, 31, 31, 31, 31, 31,
                                                 31, 33, 33, 33, 33, 33, 36, 36,
                                                 36, 36, 38, 38, 38, 40, 40, 42};
    static U32 DefaultScalingList8x8Inter[64] = { 9, 13, 13, 15, 13, 15 ,17 ,17,
                                                 17, 17, 19, 19, 19, 19, 19, 21,
                                                 21, 21, 21, 21, 21, 22, 22, 22,
                                                 22, 22, 22, 22, 24, 24, 24, 24,
                                                 24, 24, 24, 24, 25, 25, 25, 25,
                                                 25, 25, 25, 27, 27, 27, 27, 27,
                                                 27, 28, 28, 28, 28, 28, 30, 30,
                                                 30, 30, 32, 32, 32, 33, 33, 35};
    U32 j;
    U32 scanj;


    switch(ScalingListNumber)
    {
        case 0 :
        case 1 :
        case 2 :
            for(j=0;j<16;j++)
            {
                scanj = ZZ_SCAN[j];
                ScalingMatrix->FirstSixScalingList[ScalingListNumber][scanj] = DefaultScalingList4x4Intra[j];
            }
            break;
        case 3 :
        case 4 :
        case 5 :
            for(j=0;j<16;j++)
            {
                scanj = ZZ_SCAN[j];
                ScalingMatrix->FirstSixScalingList[ScalingListNumber][scanj] = DefaultScalingList4x4Inter[j];
            }
            break;
        case 6 :
            for(j=0;j<64;j++)
            {
                scanj = ZZ_SCAN8[j];
                ScalingMatrix->NextTwoScalingList[ScalingListNumber-6][scanj] = DefaultScalingList8x8Intra[j];
            }
            break;
        case 7 :
            for(j=0;j<64;j++)
            {
                scanj = ZZ_SCAN8[j];
                ScalingMatrix->NextTwoScalingList[ScalingListNumber-6][scanj] = DefaultScalingList8x8Inter[j];
            }
            break;
    }
}

/******************************************************************************/
/*! \brief Set scaling List with respect to Fall Back Rule A
 *
 * \param ScalingMatrix where is the scaling list to update
 * \param ScalingListNumber : Number of the scaling list to update
 * \return void
 */
/******************************************************************************/
void h264par_SetScalingListFallBackA(H264_SpecificScalingMatrix_t *ScalingMatrix,U32 ScalingListNumber)
{
    switch(ScalingListNumber)
    {
        case 0 :
        case 3 :
        case 6 :
        case 7 :
            h264par_SetScalingListDefault(ScalingMatrix, ScalingListNumber);
            break;
        case 1 :
        case 2 :
        case 4 :
        case 5 :
            STOS_memcpy(&(ScalingMatrix->FirstSixScalingList[ScalingListNumber]), ScalingMatrix->FirstSixScalingList[ScalingListNumber-1], sizeof(ScalingMatrix->FirstSixScalingList[ScalingListNumber]));
            break;
    }
}

/******************************************************************************/
/*! \brief Set scaling List with respect to Fall Back Rule B
 *
 * \param PPSScalingMatrix where is the PPS scaling list to update
 * \param SPSScalingMatrix where is the SPS scaling list to use for update
 * \param ScalingListNumber : Number of the scaling list to update
 * \return void
 */
/******************************************************************************/
void h264par_SetScalingListFallBackB(H264_SpecificScalingMatrix_t *PPSScalingMatrix, const H264_SpecificScalingMatrix_t *SPSScalingMatrix,U32 ScalingListNumber)
{
    switch(ScalingListNumber)
    {
        case 0 :
        case 3 :
            STOS_memcpy(&(PPSScalingMatrix->FirstSixScalingList[ScalingListNumber]), SPSScalingMatrix->FirstSixScalingList[ScalingListNumber], sizeof(PPSScalingMatrix->FirstSixScalingList[ScalingListNumber]));
            break;
        case 6 :
        case 7 :
            STOS_memcpy(&(PPSScalingMatrix->NextTwoScalingList[ScalingListNumber-6]), SPSScalingMatrix->NextTwoScalingList[ScalingListNumber-6], sizeof(PPSScalingMatrix->NextTwoScalingList[ScalingListNumber-6]));
            break;
        case 1 :
        case 2 :
        case 4 :
        case 5 :
            STOS_memcpy(&(PPSScalingMatrix->FirstSixScalingList[ScalingListNumber]), PPSScalingMatrix->FirstSixScalingList[ScalingListNumber-1], sizeof(PPSScalingMatrix->FirstSixScalingList[ScalingListNumber]));
            break;
    }
}

/******************************************************************************/
/*! \brief Get scaling List from stream or set it to default
 *
 * \param ParserHandle handle of parser to access local data
 * \param ScalingMatrix where is the scaling list to update
 * \param ScalingListNumber : Number of the scaling list to update
 * \return void
 */
/******************************************************************************/
void h264par_GetScalingList(const PARSER_Handle_t  ParserHandle, H264_SpecificScalingMatrix_t *ScalingMatrix,U32 ScalingListNumber)
{
    U32 lastScale;
    U32 nextScale;
    U32 sizeOfScalingList;
    S32 DeltaScale;
    BOOL useDefaultScalingMatrixFlag;
    U32 CoefCounter;
    U32 *ScalingList;
    U32 scanj;

    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    useDefaultScalingMatrixFlag = FALSE;
    lastScale = 8;
    nextScale = 8;
    if(ScalingListNumber < 6)
    {
        sizeOfScalingList = 16;
        ScalingList = (U32 *)ScalingMatrix->FirstSixScalingList[ScalingListNumber];
    }
    else
    {
        sizeOfScalingList = 64;
        ScalingList = (U32 *)ScalingMatrix->NextTwoScalingList[ScalingListNumber-6];
    }

    for(CoefCounter = 0 ; (CoefCounter < sizeOfScalingList)  && !useDefaultScalingMatrixFlag; CoefCounter++ )
    {
        scanj = (sizeOfScalingList==16) ? ZZ_SCAN[CoefCounter]:ZZ_SCAN8[CoefCounter];

        if(nextScale != 0)
        {
            DeltaScale = h264par_GetSignedExpGolomb(ParserHandle);
            nextScale = (lastScale + DeltaScale + 256) % 256;
            if((CoefCounter == 0) && (nextScale == 0))
            {
                useDefaultScalingMatrixFlag = TRUE;
            }
        }
        if(nextScale == 0)
        {
            ScalingList[scanj] = lastScale;
        }
        else
        {
            ScalingList[scanj] = nextScale;
        }
        lastScale = ScalingList[scanj];
    }
    if(useDefaultScalingMatrixFlag)
    {
        h264par_SetScalingListDefault(ScalingMatrix, ScalingListNumber);
    }
}


/******************************************************************************/
/*! \brief Initialize GDC Generic and Specific data HRD parameters.
 *
 * Conditional parameters are parameters that may not be present in the stream.
 * The standard imposes a default value for these parameters.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param SeqParameterSetId the GDC to initialize
 * \return void
 */
/******************************************************************************/
static void h264par_InitializeDefaultHRDParameters(U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* Default value for nal hrd parameters */
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].NalHrdBpPresentFlag = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_cpb_cnt_minus1 = 0;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[0] = 1200 * PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxBR;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[0] = 1200 * PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxCPB;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_cbr_flag[0] = 0;
    /* SequenceInfo.BitRate is the maximum of nal_bitrate. Defined in the API in unit of 400 bit/s */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.BitRate =
         PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[0] / 400;
    /* SequenceInfo.VBVBufferSize is the maximum of nal_Cpbsize. SequenceInfo.VBVBufferSize in bytes, nal_Cpbsize in bits */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.VBVBufferSize =
         PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[0] / 8;

    /* Default value for vcl hrd parameters */
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].VclHrdBpPresentFlag = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_cpb_cnt_minus1 = 0;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[0] = 1000 * PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxBR;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[0] = 1000 * PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxCPB;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_cbr_flag[0] = 0;
}

/******************************************************************************/
/*! \brief Initialize GDC Generic and Specific data conditional parameters.
 *
 * Conditional parameters are parameters that may not be present in the stream.
 * The standard imposes a default value for these parameters.
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \param SeqParameterSetId the GDC to initialize
 * \return void
 */
/******************************************************************************/
void h264par_InitializeGlobalDecodingContext(U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle)
{
    U32 ScalingListCounter;

    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* default values for SPS parameters */

    /* Specifies that the stream follows H264 syntax */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_14496;

    /* Set Low Delay flag to FALSE by default*/
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.IsLowDelay = FALSE;

    /* Set chroma_format_idc to default value 1 (4:2:0) for not-high profile */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_format_idc = 1;

    /* Set residual_colour_transform_flag to default value for chroma_format_idc != 3 */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].residual_colour_transform_flag = FALSE;

    /* Set bit_depth_luma_minus8 to default value 0 (8 bits) for not-high profile */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].bit_depth_luma_minus8 = 0;

    /* Set bit_depth_chroma_minus8 to default value 0 (8 bits) for not-high profile */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].bit_depth_chroma_minus8 = 0;

    /* Set qpprime_y_zero_transform_bypass_flag to default value FALSE for not-high profile */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].qpprime_y_zero_transform_bypass_flag = FALSE;

    /* Set seq_scaling_matrix_present_flag to default value FALSE for not-high profile */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].seq_scaling_matrix_present_flag = FALSE;

    /* Set scaling list to Flat (default for MAIN profile and HIGH profile without scaling list) */
    for(ScalingListCounter = 0 ; ScalingListCounter < 8 ; ScalingListCounter++)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].ScalingMatrix.ScalingListPresentFlag[ScalingListCounter] = FALSE;
    }
    h264par_SetFlatScalingMatrix(&(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].ScalingMatrix));

    /* When mb_adaptive_frame_field_flag is not present,
     * it shall be inferred to be equal to 0
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].mb_adaptive_frame_field_flag = 0;

    /* When frame_cropping_flag is equal to 0, the following values shall be inferred:
     * frame_crop_left_offset = 0, frame_crop_right_offset = 0,
     * frame_crop_top_offset = 0, and frame_crop_bottom_offset = 0
     */
     PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.LeftOffset = 0;
     PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.RightOffset = 0;
     PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.TopOffset = 0;
     PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.BottomOffset = 0;

    /* default values for conditional VUI parameters */

    /* Default value for Aspect ratio is 4/3 . Same as MPEG 2 */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;

    /* When the video_format syntax element is not present,
     * video_format value shall be inferred to be equal to 5 (unspecified)
     */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.VideoFormat = UNSPECIFIED_VIDEO_FORMAT;

    /* When the video_full_range_flag syntax element is not present,
     * video_full_range_flag value shall be inferred to be equal to 0
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].video_full_range_flag = 0;

    /*      When the colour_primaries syntax element is not present,
     * the value of colour_primaries shall be inferred to be equal to 2
     * (the chromaticity is unspecified or is determined by the application).
     */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ColourPrimaries = UNSPECIFIED_COLOUR_PRIMARIES;

    /* When the transfer_characteristics syntax element is not present,
     * the value of transfer_characteristics shall be inferred to be equal to 2
     * (the transfer characteristics are unspecified or are determined by the application)
     */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].TransferCharacteristics = UNSPECIFIED_TRANSFER_CHARACTERISTICS;

    /* When the matrix_coefficients syntax element is not present,
     * the value of matrix_coefficients shall be inferred to be equal to 2
     */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].MatrixCoefficients = MATRIX_COEFFICIENTS_UNSPECIFIED;

    /* When the chroma_sample_loc_type_top_field and chroma_sample_loc_type_bottom_field
     * are not present, the values of chroma_sample_loc_type_top_field and
     * chroma_sample_loc_type_bottom_field shall be inferred to be equal to 0
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_sample_loc_type_top_field = 0;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_sample_loc_type_bottom_field = 0;

    /* default value for SequenceInfo.FrameRate: unspecified */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRate = H264_UNSPECIFIED_FRAME_RATE;

    /* Copy the StreamID retrieved from the Setup stage */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.StreamID = PARSER_Data_p->ParserGlobalVariable.StreamID;

    /* Default values for HRD parameters common between NAL and VCL */
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].num_ref_frames_in_pic_cnt_cycle = 0;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].pic_struct_present_flag = FALSE;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].CpbDpbDelaysPresentFlag = 0;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelayLength = 24;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].CpbRemovalDelayLength = 24;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].DpbOutputDelayLength = 24;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].TimeOffsetLength = 24;

    /* Default value for IsInitialCpbRemovalDelayValid */
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsInitialCpbRemovalDelayValid = FALSE;

    /* When the motion_vectors_over_pic_boundaries_flag syntax element is not present,
     * motion_vectors_over_pic_boundaries_flag value shall be inferred to be equal to 1
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].motion_vectors_over_pic_boundaries_flag = 1;

    /* When the max_bytes_per_pic_denom syntax element is not present,
     * the value of max_bytes_per_pic_denom shall be inferred to be equal to 2
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].max_bytes_per_pic_denom = 2;

    /* When the max_bits_per_mb_denom is not present, the value of max_bits_per_mb_denom
     * shall be inferred to be equal to 1
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].max_bits_per_mb_denom = 1;

    /* When log2_max_mv_length_horizontal is not present, the values of
     * log2_max_mv_length_horizontal and log2_max_mv_length_vertical shall be inferred
     * to be equal to 16
     */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_mv_length_horizontal = 16;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_mv_length_vertical = 16;

    /* In H264, Progressive/Interlaced is decided picture by picture, thus it is set to "Interlace"
     * See vid_com.h
     */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.ScanType = STVID_SCAN_TYPE_INTERLACED;

    /* Initialize errors in SPS */
    /* Errors will stay present until the SPS is erased by another SPS with the
     * same seq_parameter_set_id
     */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_NONE;

    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.MaxBytesPerPicDenomRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.MaxBitsPerMBDenomError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxMvLengthHorizontalRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxMvLengthVerticalRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NumReorderFrameRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.MaxDecBufferingRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.IsNotSupportedProfileError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.AuxCodedPicturePresent = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.ReservedZero4BitsError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.LevelIdcRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxFrameNumMinus4RangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxPicOrderCntLsbMinus4RangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NumRefFramesRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Direct8x8InferenceFlagNullWithFrameMbsOnlyFlagError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Direct8x8InferenceFlagNullWithLevel3OrMoreError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.TruncatedNAL = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalCpbCntMinus1RangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalBitRateRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalCpbSizeRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingNalBitRateError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingNalCpbSizeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclCpbCntMinus1RangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclBitRateRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclCpbSizeRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingVclBitRateError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingVclCpbSizeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.InitialCpbRemovalDelayDifferError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.CpbRemovalDelayLengthDifferError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.DpbOutputDelayLengthDifferError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.TimeOffsetLengthDifferError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.LowDelayHrdFlagNotNullError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.WidthRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.HeightRangeError = FALSE;
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.AspectRatioRangeError = FALSE;

    /* Unused entries for H.264: fill them with a default unused value */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRateExtensionN = H264_DEFAULT_NON_RELEVANT;
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRateExtensionD = H264_DEFAULT_NON_RELEVANT;

}

/******************************************************************************/
/*! \brief Computes the Sample Aspect Ratios (width, height) from AspectRatioIdc
 *
 * This is table E-1
 *
 * \param AspectRatioIdc stream parameter
 * \param SarWidth the sample aspect ratio width
 * \param SarHeight the sample aspect ratio height
 * \return void
 */
/******************************************************************************/
void h264par_GetSampleAspectRatio(U8 AspectRatioIdc, U16 * SarWidth, U16 * SarHeight)
{
    *SarWidth = 4;     /* Set a default value (4:3)*/
    *SarHeight = 3;
    switch (AspectRatioIdc)
    {
        case 0:
            /* Unspecified */
            break;
        case 1: /* square pixel */
            * SarWidth = 1;
            * SarHeight = 1;
            break;
       case 2: /* 12:11 */
            * SarWidth = 12;
            * SarHeight = 11;
            break;
       case 3: /* 10:11 */
            * SarWidth = 10;
            * SarHeight = 11;
            break;
       case 4: /* 16:11 */
            * SarWidth = 16;
            * SarHeight = 11;
            break;
       case 5: /* 40:33 */
            * SarWidth = 40;
            * SarHeight = 33;
            break;
       case 6: /* 24:11 */
            * SarWidth = 24;
            * SarHeight = 11;
            break;
       case 7: /* 20:11 */
            * SarWidth = 20;
            * SarHeight = 11;
            break;
       case 8: /* 32:11 */
            * SarWidth = 32;
            * SarHeight = 11;
            break;
       case 9: /* 80:33 */
            * SarWidth = 80;
            * SarHeight = 33;
            break;
       case 10: /* 18:11 */
            * SarWidth = 18;
            * SarHeight = 11;
            break;
       case 11: /* 15:11 */
            * SarWidth = 15;
            * SarHeight = 11;
            break;
       case 12: /* 64:33 */
            * SarWidth = 64;
            * SarHeight = 33;
            break;
       case 13: /* 160:99 */
            * SarWidth = 160;
            * SarHeight = 99;
            break;
       case 14: /* 4:3 */       /* 14,15,16 added, as described in doc "Liaison statement to ATSC S6" */
            * SarWidth = 4;
            * SarHeight = 3;
            break;
       case 15: /* 3:2 */
            * SarWidth = 3;
            * SarHeight = 2;
            break;
       case 16: /* 2:1 */
            * SarWidth = 2;
            * SarHeight = 1;
            break;
       case 255: /* Extended SAR */
            * SarWidth = 4; /* dont care, as these values will be extracted from the stream */
            * SarHeight = 3;
            break;
       default:
            /* when AspectRatioIdc = 14...254 */
            /* Reserved */
            /* TODO: fill default value for Sample Aspect Ratio */
            break;
    }
}

/******************************************************************************/
/*! \brief Parses VUI and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param SetParameterSetId index where to store VUI data
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseVUI (U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle)
{
    U8  AspectRatioInfoPresentFlag;
    U8  AspectRatioIdc;
    U16 SarWidth;
    U16 SarHeight;
    U8  OverscanInfoPresentFlag;
    U8  VideoSignalTypePresentFlag;
    U8  VideoFormat;
    U8  VideoFullRangeFlag = 0;    /* default value must be checked ... */
    U8  ColourDescriptionPresentFlag;
    U8  ColourPrimaries;
    U8  TransferCharacteristics;
    U8  MatrixCoefficients;
    U8  ChromaLocInfoPresentFlag;
    U8  TimingInfoPresentFlag;
    U32 NumUnitsInTicks;
    U32 TimeScale;
    U32 IntPart = 0;
    U32 Remaining = 0;
    U8  FixedFrameRateFlag;
    U8  NalHrdParametersPresentFlag;
    U8  VclHrdParametersPresentFlag;
    U8  LowDelayHrdFlag;
    U8  PicStructPresentFlag;
    U8  BitstreamRestrictionFlag;
    U8  NalCpbCntMinus1 = - 1;  /* default value must be checked ... */
    U8  VclCpbCntMinus1 = -1;   /* default value must be checked ... */
    U8  BitRateScale;
    U8  CpbSizeScale;
    U8  SchedSelIdx;
    U32 BitRateValueMinus1;
    U32 CpbSizeValueMinus1;
    U8  VclInitialCpbRemovalDelayLength;
    U8  VclCpbRemovalDelayLength;
    U8  VclDpbOutputDelayLength;
    U8  VclTimeOffsetLength;
    U32 DisplayAspectRatio;
    U32 NalMaxBitRate;
    U32 VclMaxBitRate;
    U32 NalMaxCpbSize;
    U32 VclMaxCpbSize;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* Default for aspect ratio = square pixel */
    SarWidth = 1;
    SarHeight = 1;
    AspectRatioIdc = 0;

    /* No default is specified in the standard, however the less restrictive is chosen to
     * allow the semantics checks for low_delay_hrd_flag
     */
    FixedFrameRateFlag = 0;

    /* (Nal|Vcl)MaxBitRate is the maximum of (nal|vcl)_BitRate found */
    /* If no bit rate is defined in this VUI, take the default value (see initialization) */
    NalMaxBitRate = 0;
    VclMaxBitRate = 0;

    /* (Nal|Vcl)MaxCpbSize is the maximum of (nal|vcl)_CpbSize found */
    /* If no CPB size is defined in this VUI, take the default value (see initialization) */
    NalMaxCpbSize = 0;
    VclMaxCpbSize = 0;

    AspectRatioInfoPresentFlag = h264par_GetUnsigned(1, ParserHandle);

    if (AspectRatioInfoPresentFlag == 1)
    {
        AspectRatioIdc = h264par_GetUnsigned(8, ParserHandle);
        h264par_GetSampleAspectRatio(AspectRatioIdc, &SarWidth, &SarHeight);
        if (AspectRatioIdc == ExtendedSAR)
        {
            SarWidth = h264par_GetUnsigned(16, ParserHandle);
            SarHeight = h264par_GetUnsigned(16, ParserHandle);
        }

    }

    /* Caution! AspectRatioIdc stands for sample aspect ratio, we have to convert it into a DISPLAY aspect ratio for SequenceInfo */
    if ((AspectRatioIdc == 0)|| (SarWidth == 0)||(SarHeight == 0))
    {
        /* Unspecified: the 4/3 is chosen as default value. Same for MPEG 2 */
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
    }
    else
    {
        /* DisplayAspectRatio is set to 9 times the aspect ratio, so that a further integer comparison
        * gives the correspondance with STVID types
        */
        DisplayAspectRatio = (9* SarWidth  * PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Width) /
                             (SarHeight * PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Height);

        /* 4:3 is provided when DisplayAspectRatio is around 12 (+-20% = [9;14[) */
        if ((DisplayAspectRatio >= 9) && (DisplayAspectRatio < 14))
        {
           PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
        }
        /* 16:9 is provided when DisplayAspectRatio is around 16 (+-20% = [14;18[) */
        else if ((DisplayAspectRatio >= 14) && (DisplayAspectRatio < 18))
        {
           PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
        }
        /* 2.21:1 is provided when DisplayAspectRatio is around 20 (+-20% = [18;22[) */
        else if ((DisplayAspectRatio >= 18) && (DisplayAspectRatio < 22))
        {
           PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
        }
        else
        {
            /* DisplayAspectRatio is not around (+-20%) of any known value.
             * An error is raised and the default aspect ratio is selected
             */
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.AspectRatioRangeError,
                "parse_sps.c: (ParseVUI) error: Aspect ratio out of range\n");
        }
    }

    OverscanInfoPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (OverscanInfoPresentFlag == 1)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].overscan_appropriate_flag = h264par_GetUnsigned(1, ParserHandle);
    }


    VideoSignalTypePresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (VideoSignalTypePresentFlag == 1)
    {
        VideoFormat = h264par_GetUnsigned(3, ParserHandle);
        /* Same meaning between MPEG2 and H.264! */
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.VideoFormat = VideoFormat;

        VideoFullRangeFlag = h264par_GetUnsigned(1, ParserHandle);

        ColourDescriptionPresentFlag = h264par_GetUnsigned(1, ParserHandle);
        if (ColourDescriptionPresentFlag == 1)
        {
            ColourPrimaries = h264par_GetUnsigned(8, ParserHandle);
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ColourPrimaries = ColourPrimaries;

            TransferCharacteristics = h264par_GetUnsigned(8, ParserHandle);
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].TransferCharacteristics = TransferCharacteristics;

            MatrixCoefficients = h264par_GetUnsigned(8, ParserHandle);
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].MatrixCoefficients = MatrixCoefficients;
        }
    }

    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].video_full_range_flag = VideoFullRangeFlag;

    ChromaLocInfoPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (ChromaLocInfoPresentFlag == 1)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_sample_loc_type_top_field = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_sample_loc_type_bottom_field = h264par_GetUnsignedExpGolomb(ParserHandle);
    }

    TimingInfoPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (TimingInfoPresentFlag == 1)
    {
        NumUnitsInTicks = h264par_GetUnsigned(32, ParserHandle);
        TimeScale = h264par_GetUnsigned(32, ParserHandle);
        FixedFrameRateFlag = h264par_GetUnsigned(1, ParserHandle);

		/* TODO: frameRate cannot be computed accurately here. The driver needs parsing the SEI message associated with
		 * the SPS (see fixed_frame_rate_flag semantics and table E-6)
		 */
		if (NumUnitsInTicks != 0)
        {
            if (TimeScale >= MAX_TIME_SCALE_TO_PREVENT_OVERFLOW)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRate = 1000 * (TimeScale / NumUnitsInTicks);
                IntPart = 1000 * (TimeScale / NumUnitsInTicks);
                Remaining = (1000 * (TimeScale % NumUnitsInTicks)) / NumUnitsInTicks;
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRate = IntPart + Remaining;
            }
            else
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRate = (1000 * TimeScale) / NumUnitsInTicks;
            }
        }
        else
        {
            /* as we cannot compute the FrameRate, mark it as unspecified */
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.FrameRate = H264_UNSPECIFIED_FRAME_RATE;

        }
    }

    NalHrdParametersPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (NalHrdParametersPresentFlag == 1)
    {
        NalCpbCntMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
        if (NalCpbCntMinus1 > 31)
        {
            NalCpbCntMinus1 = 1; /* Error recovery set to 1 */
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalCpbCntMinus1RangeError,
            "parse_sps.c: (ParseVUI) error: (nal)cpb_cnt_minus1 out of range\n");
            NalCpbCntMinus1 = 31;
        }
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_cpb_cnt_minus1 = NalCpbCntMinus1;
        BitRateScale = h264par_GetUnsigned(4, ParserHandle);
        CpbSizeScale = h264par_GetUnsigned(4, ParserHandle);
        for (SchedSelIdx = 0; SchedSelIdx <= NalCpbCntMinus1; SchedSelIdx ++)
        {
            if ((BitRateValueMinus1=h264par_GetUnsignedExpGolomb(ParserHandle)) == UNSIGNED_EXPGOLOMB_FAILED)
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[SchedSelIdx] = 1;   /* Error case: set to 1 (To be Checked) */

                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalBitRateRangeError,
                "parse_sps.c: (ParseVUI) error: (nal)BitRate out of range\n");
            }
            else
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[SchedSelIdx] = (BitRateValueMinus1 + 1) * (1 << (6 + BitRateScale));
            }

            if ((CpbSizeValueMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle)) == UNSIGNED_EXPGOLOMB_FAILED)
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[SchedSelIdx] = 1;   /* Error case: set to 1 (To be Checked) */

                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalCpbSizeRangeError,
                "parse_sps.c: (ParseVUI) error: (nal)CpbSize out of range\n");
            }
            else
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[SchedSelIdx] = (CpbSizeValueMinus1 + 1) * (1 << (4 + CpbSizeScale));
            }

            PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_cbr_flag[SchedSelIdx] = h264par_GetUnsigned(1, ParserHandle);

            if (SchedSelIdx > 0)
            {
                if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[SchedSelIdx] <=
                    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[SchedSelIdx - 1]
                   )
                {
                    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                    DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingNalBitRateError,
                    "parse_sps.c: (ParseVUI) error: non increasing (nal)BitRate\n");
                }
                if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[SchedSelIdx] <=
                    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[SchedSelIdx - 1]
                   )
                {
                    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                    DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingNalCpbSizeError,
                    "parse_sps.c: (ParseVUI) error: non increasing (nal)CpbSize\n");
                }

            }
            /* Set NalMaxBitRate as the maximum of nal_bitrate found */
            if (NalMaxBitRate < PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[SchedSelIdx])
            {
                NalMaxBitRate = PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_BitRate[SchedSelIdx];
            }
            /* Set NalMaxCpbSize as the maximum of nal_CpbSize found */
            if (NalMaxCpbSize < PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[SchedSelIdx])
            {
                NalMaxCpbSize = PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].nal_CpbSize[SchedSelIdx];
            }
        } /* end for */
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].NalHrdBpPresentFlag = TRUE;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelayLength = h264par_GetUnsigned(5, ParserHandle) + 1; /* norminative read value is initial_cpb_removal_delay_length_minus1 */;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].CpbRemovalDelayLength = h264par_GetUnsigned(5, ParserHandle) + 1; /* norminative read value is cpb_removal_delay_length_minus1 */
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].DpbOutputDelayLength = h264par_GetUnsigned(5, ParserHandle) + 1; /* norminative read value is dpb_output_delay_length_minus1 */
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].TimeOffsetLength = h264par_GetUnsigned(5, ParserHandle);
    }

    VclHrdParametersPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (VclHrdParametersPresentFlag == 1)
    {
        VclCpbCntMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
        if (VclCpbCntMinus1 > 31)
        {
            VclCpbCntMinus1 = 1; /* Error recovery set to 1 */
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclCpbCntMinus1RangeError,
            "parse_sps.c: (ParseVUI) error: (vcl)cpb_cnt_minus1 out of range\n");
            VclCpbCntMinus1 = 31;
        }
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_cpb_cnt_minus1 = VclCpbCntMinus1;
        BitRateScale = h264par_GetUnsigned(4, ParserHandle);
        CpbSizeScale = h264par_GetUnsigned(4, ParserHandle);
        for (SchedSelIdx = 0; SchedSelIdx <= VclCpbCntMinus1; SchedSelIdx ++)
        {
            if ((BitRateValueMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle)) == UNSIGNED_EXPGOLOMB_FAILED)
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[SchedSelIdx] = 1;   /* Error case: set to 1 (To be Checked) */

                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclBitRateRangeError,
                "parse_sps.c: (ParseVUI) error: (vcl)BitRate out of range\n");
            }
            else
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[SchedSelIdx] = (BitRateValueMinus1 + 1) * (1 << (6 + BitRateScale));
            }

            if ((CpbSizeValueMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle)) == UNSIGNED_EXPGOLOMB_FAILED)
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[SchedSelIdx] = 1;   /* Error case: set to 1 (To be Checked) */

                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclCpbSizeRangeError,
                "parse_sps.c: (ParseVUI) error: (vcl)CpbSize out of range\n");
            }
            else
            {
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[SchedSelIdx] = (CpbSizeValueMinus1 + 1) * (1 << (4 + CpbSizeScale));
            }

            PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_cbr_flag[SchedSelIdx] = h264par_GetUnsigned(1, ParserHandle);

            if (SchedSelIdx > 0)
            {
                if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[SchedSelIdx] <=
                    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[SchedSelIdx - 1]
                   )
                {
                    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                    DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingVclBitRateError,
                    "parse_sps.c: (ParseVUI) error: non increasing (vcl)BitRate\n");
                }
                if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[SchedSelIdx] <=
                    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[SchedSelIdx - 1]
                   )
                {
                    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                    DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NonIncreasingVclCpbSizeError,
                    "parse_sps.c: (ParseVUI) error: non increasing (vcl)CpbSize\n");
                }

            }
            /* Set VclMaxBitRate as the maximum of nal_bitrate found */
            if (VclMaxBitRate < PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[SchedSelIdx])
            {
                VclMaxBitRate = PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_BitRate[SchedSelIdx];
            }
            /* Set VclMaxCpbSize as the maximum of vcl_CpbSize found */
            if (VclMaxCpbSize < PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[SchedSelIdx])
            {
                VclMaxCpbSize = PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].vcl_CpbSize[SchedSelIdx];
            }
        } /* end for */
        VclInitialCpbRemovalDelayLength = h264par_GetUnsigned(5, ParserHandle) + 1; /* norminative read value is initial_cpb_removal_delay_length_minus1 */
        VclCpbRemovalDelayLength = h264par_GetUnsigned(5, ParserHandle) + 1; /* norminative read value is cpb_removal_delay_length_minus1 */
        VclDpbOutputDelayLength = h264par_GetUnsigned(5, ParserHandle) + 1; /* norminative read value is dpb_output_delay_length_minus1 */
        VclTimeOffsetLength = h264par_GetUnsigned(5, ParserHandle);
        if (NalHrdParametersPresentFlag == 1)
        {
            if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelayLength != VclInitialCpbRemovalDelayLength)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.InitialCpbRemovalDelayDifferError,
                "parse_sps.c: (ParseVUI) error: initial_cpb_removal_delay_length value differs between nal and vcl\n");
            }
            if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].CpbRemovalDelayLength != VclCpbRemovalDelayLength)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.CpbRemovalDelayLengthDifferError,
                "parse_sps.c: (ParseVUI) error: cpb_removal_delay_length value differs between nal and vcl\n");
            }
            if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].DpbOutputDelayLength != VclDpbOutputDelayLength)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.DpbOutputDelayLengthDifferError,
                "parse_sps.c: (ParseVUI) error: dpb_output_delay_length value differs between nal and vcl\n");
            }
            if (PARSER_Data_p->SPSLocalData[SeqParameterSetId].TimeOffsetLength != VclTimeOffsetLength)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.TimeOffsetLengthDifferError,
                "parse_sps.c: (ParseVUI) error: time_offset_length value differs between nal and vcl\n");
            }
        }
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].VclHrdBpPresentFlag = TRUE;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].InitialCpbRemovalDelayLength = VclInitialCpbRemovalDelayLength;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].CpbRemovalDelayLength = VclCpbRemovalDelayLength;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].DpbOutputDelayLength = VclDpbOutputDelayLength;
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].TimeOffsetLength = VclTimeOffsetLength;
    }

    if ((NalHrdParametersPresentFlag == 1) || (VclHrdParametersPresentFlag == 1))
    {
        /* Assign the Sequence BitRate as the maximum of all nal|vcl_bitrate
         * Unit is done on Nal ( = 1.2 * Vcl)
         */
        if (NalMaxBitRate > (6 * VclMaxBitRate / 5))
        {
            /* BitRate is defined in 400 bit/s unit */
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.BitRate = NalMaxBitRate / 400;
        }
        else
        {
            /* BitRate is defined in 400 bit/s unit */
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.BitRate = (6 * VclMaxBitRate / 5) / 400;
        }

        /* Assign the Sequence VBVBufferSize as the maximum of all nal|vcl_cpbsize
         * Unit is done on Nal ( = 1.2 * Vcl)
         * Nal CPB is in bits, export VBVBufferSize in bytes
         */
        if (NalMaxCpbSize > (6 * VclMaxCpbSize / 5))
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.VBVBufferSize = NalMaxCpbSize / 8;
        }
        else
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.VBVBufferSize = (6 * VclMaxCpbSize / 5) / 8;
        }

        PARSER_Data_p->SPSLocalData[SeqParameterSetId].CpbDpbDelaysPresentFlag = 1;

        LowDelayHrdFlag = h264par_GetUnsigned(1, ParserHandle);
        /* semantics checks when LowDelayHrdFlag == 1 */
        if (LowDelayHrdFlag == 1)
        {
            if (FixedFrameRateFlag == 1)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.LowDelayHrdFlagNotNullError,
                "parse_sps.c: (ParseVUI) error: low_delay_hrd_flag not equal to 0 while fixed_frame_rate_flag is true\n");
            }
            if (NalCpbCntMinus1 != 0)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NalCpbCntMinus1RangeError,
                "parse_sps.c: (ParseVUI) error: (nal)cpb_cnt_minus1 out of range\n");
            }
            if (VclCpbCntMinus1 != 0)
            {
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
                DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.VclCpbCntMinus1RangeError,
                "parse_sps.c: (ParseVUI) error: (vcl)cpb_cnt_minus1 out of range\n");
            }
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.IsLowDelay = TRUE;
        }

    }

    PicStructPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (PicStructPresentFlag == 1)
    {
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].pic_struct_present_flag = TRUE;
    }
    else
    {
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].pic_struct_present_flag = FALSE;
    }

    BitstreamRestrictionFlag = h264par_GetUnsigned(1, ParserHandle);
    if (BitstreamRestrictionFlag == 1)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].motion_vectors_over_pic_boundaries_flag = h264par_GetUnsigned(1, ParserHandle);

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].max_bytes_per_pic_denom = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* Check MaxBytesPerPicDenom range */
        if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].max_bytes_per_pic_denom > 16)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.MaxBytesPerPicDenomRangeError,
            "parse_sps.c: (ParseVUI) parse error: max_bytes_per_pic_denom out of range\n");
        }

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].max_bits_per_mb_denom = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* Check MaxBitsPerMbDenom range */
        if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].max_bits_per_mb_denom > 16)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.MaxBitsPerMBDenomError,
            "parse_sps.c: (ParseVUI) parse error: max_bits_per_mb_denom out of range\n");
        }

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_mv_length_horizontal = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* Check Log2MaxMvLengthHorizontal range */
        if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_mv_length_horizontal > 16)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxMvLengthHorizontalRangeError,
            "parse_sps.c: (ParseVUI) parse error: log2_max_mv_length_horizontal out of range\n");
        }

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_mv_length_vertical = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* Check Log2MaxMvLengthVertical range */
        if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_mv_length_vertical > 16)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxMvLengthVerticalRangeError,
            "parse_sps.c: (ParseVUI) parse error: log2_max_mv_length_vertical out of range\n");
        }

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].num_reorder_frames = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].MaxDecFrameBuffering = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* Check num_reorder_frames range */
        if (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].num_reorder_frames > PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].MaxDecFrameBuffering)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NumReorderFrameRangeError,
            "parse_sps.c: (ParseVUI) parse error: num_reorder_frames out of range\n");
        }
        /* Check max_dec_frame_numbering */
        if (PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].MaxDecFrameBuffering > PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxDpbSize)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.MaxDecBufferingRangeError,
            "parse_sps.c: (ParseVUI) parse error: max_dec_frame_buffering out of range\n");
        }
    }
}


/******************************************************************************/
/*! \brief Computes MaxDpbSize, MaxBR and MaxCPB from the Level
 *
 * See equation in A.3.1 and Table A-1
 *
 * \param LevelIdc Bistream Level (10 to 40)
 * \param PicWidthInMbsMinus1 SPS stream element
 * \param PicHeightInMapUnitsMinus1 SPS stream element
 * \param FrameMbsOnlyFlag SPS stream element
 * \param SeqParameterSetId stream element
 * \param ParserHandle Parser Handle (variables managed by the parser)
 */
/******************************************************************************/
void h264par_ComputeMaxFromLevel(U8 LevelIdc, U8 PicWidthInMbsMinus1, U8 PicHeightInMapUnitsMinus1, U8 FrameMbsOnlyFlag, U8 SeqParameterSetId, const PARSER_Handle_t  ParserHandle)
{
    U8 FrameHeightInMbs;
    U8 PicWidthInMbs;
    U8 PicHeightInMapUnits;

    U32 TempMaxDpbSize;
    U32 MaxDPB = 0;
    U32 MaxBR = 0;
    U32 MaxCPB = 0;
    U32 MaxWidth = 0;
    U32 MaxHeight = 0;
    U32 MaxPixel = 0;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    /* equation (7-3) */
    PicWidthInMbs = PicWidthInMbsMinus1 + 1;

    /* equation (7-6) */
    PicHeightInMapUnits = PicHeightInMapUnitsMinus1 + 1;

    /* equation (7-8) */
    FrameHeightInMbs = (2 - FrameMbsOnlyFlag) * PicHeightInMapUnits;

    /* Table A-1 Level limits for MaxDPB */
    switch (LevelIdc)
    {
        case 10: MaxDPB = 152064; /* in byte */
                 MaxBR = 64;
                 MaxCPB = 175;
                 MaxPixel = 99 * 256;
                 MaxWidth = 448;
                 MaxHeight = 448;
                 break;

        case 11: MaxDPB = 345600; /* in byte */
                 MaxBR = 192;
                 MaxCPB = 500;
                 MaxPixel = 396 * 256;
                 MaxWidth = 896;
                 MaxHeight = 896;
                 break;

        case 12: MaxDPB = 912384; /* in byte */
                 MaxBR = 384;
                 MaxCPB = 1000;
                 MaxPixel = 396 * 256;
                 MaxWidth = 896;
                 MaxHeight = 896;
                 break;

        case 13: MaxDPB = 912384; /* in byte */
                 MaxBR = 768;
                 MaxCPB = 2000;
                 MaxPixel = 396 * 256;
                 MaxWidth = 896;
                 MaxHeight = 896;
                 break;

        case 20: MaxDPB = 912384; /* in byte */
                 MaxBR = 2000;
                 MaxCPB = 2000;
                 MaxPixel = 396 * 256;
                 MaxWidth = 896;
                 MaxHeight = 896;
                 break;

        case 21: MaxDPB = 1824768; /* in byte */
                 MaxBR = 4000;
                 MaxCPB = 4000;
                 MaxPixel = 792 * 256;
                 MaxWidth = 1264;
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;

        case 22: MaxDPB = 3110400; /* in byte */
                 MaxBR = 4000;
                 MaxCPB = 4000;
                 MaxPixel = 1620 * 256;
                 MaxWidth = 1808;
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;

        case 30: MaxDPB = 3110400; /* in byte */
                 MaxBR = 10000;
                 MaxCPB = 10000;
                 MaxPixel = 1620 * 256;
                 MaxWidth = 1808;
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;

        case 31: MaxDPB = 6912000; /* in byte */
                 MaxBR = 14000;
                 MaxCPB = 14000;
                 MaxPixel = 3600 * 256;
                 MaxWidth = 1920; /* Clipped to max available for delta */
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;

        case 32: MaxDPB = 7864320; /* in byte */
                 MaxBR = 20000;
                 MaxCPB = 20000;
                 MaxPixel = 5120 * 256;
                 MaxWidth = 1920; /* Clipped to max available for delta */
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;

        case 40: MaxDPB = 12582912; /* in byte */
                 MaxBR = 20000;
                 MaxCPB = 25000;
                 MaxPixel = 8192 * 256;
                 MaxWidth = 1920; /* Clipped to max available for delta */
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;

        case 41: MaxDPB = 12582912; /* in byte */
                 MaxBR = 50000;
                 MaxCPB = 62500;
                 MaxPixel = 8192 * 256;
                 MaxWidth = 1920; /* Clipped to max available for delta */
                 MaxHeight = 1088; /* Clipped to max available for delta */
                 break;
    }

    /* equation A.3.1 for MaxDpbSize */
    if (PicWidthInMbs == 0)
        PicWidthInMbs = 1;
    if (FrameHeightInMbs == 0)
        FrameHeightInMbs = 1;

    TempMaxDpbSize = (U32)(MaxDPB / (PicWidthInMbs * FrameHeightInMbs * NB_BYTES_PER_MB));

    if (TempMaxDpbSize > 16)
    {
        TempMaxDpbSize = 16;
    }

    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxDpbSize =  TempMaxDpbSize;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxBR = MaxBR;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxCPB = MaxCPB;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxWidth = MaxWidth;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxHeight = MaxHeight;
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxPixel = MaxPixel;
}

/******************************************************************************/
/*! \brief Parses a SPS and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSPS (const PARSER_Handle_t  ParserHandle)
{
    /* the parser knows where to store the parameters once it reads the seq_parameter_set_id
     * As this parameter comes only in 6th position, the function stores the first 5
     * parameters in local variables before storing them
     */
    U8 ProfileIdc;
    U8 ConstraintSet0Flag;
    U8 ConstraintSet1Flag;
    U8 ConstraintSet2Flag;
    U8 ConstraintSet3Flag;
    U8 ReservedZero4Bits;
    U8 LevelIdc;

    U8 SeqParameterSetId; /* the index where to store the parsed SPS parameters */

    U8 PicOrderCntType;
    U8 Log2MaxFrameNumMinus4;
    U8 Log2MaxPixOrderCntLsbMinus4;
    U32 NumRefFramesInPicOrderCntCycle;
    U32 i; /* a loop index as defined in the standard syntax */

    U8 NumRefFrames;

    U8 PicWidthInMbsMinus1;
    U8 PicHeightInMapUnitsMinus1;
    U8 FrameMbsOnlyFlag;
    U8 Direct8x8InferenceFlag;
    U8 FrameCroppingFlag;
    U16 PictureDisplayHeightAfterCropping;
    U32 CropUnitX, CropUnitY;

    U8  VuiParametersPresentFlag;

    U32 ProfileAndLevelIndication;

    BOOL SeqScalingListPresentFlag;
    U32 ScalingListNumber;

    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


    ProfileIdc = h264par_GetUnsigned(8, ParserHandle);
    PARSER_Data_p->ParserGlobalVariable.ProfileIdc = ProfileIdc;

    ConstraintSet0Flag = h264par_GetUnsigned(1, ParserHandle);

    ConstraintSet1Flag = h264par_GetUnsigned(1, ParserHandle);

    ConstraintSet2Flag = h264par_GetUnsigned(1, ParserHandle);

    ConstraintSet3Flag = h264par_GetUnsigned(1, ParserHandle);

    ReservedZero4Bits = h264par_GetUnsigned(4, ParserHandle);

    LevelIdc = h264par_GetUnsigned(8, ParserHandle);


    SeqParameterSetId = h264par_GetUnsignedExpGolomb(ParserHandle);
    if (SeqParameterSetId > (MAX_SPS-1))
    {
        return;
    }

    h264par_InitializeGlobalDecodingContext(SeqParameterSetId, ParserHandle);

    /* Check profile_idc and constraint_set1_flag */
    if ((ProfileIdc != PROFILE_MAIN) && (ProfileIdc != PROFILE_HIGH) && (ConstraintSet1Flag != 1))
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.IsNotSupportedProfileError,
        "parse_sps.c: (ParseSPS) parse error: stream does not comply with main or high profile constraints\n");
    }

    /* Check reserved_zero_5bits */
    if (ReservedZero4Bits != 0)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.ReservedZero4BitsError,
        "parse_sps.c: (ParseSPS) parse error: reserved_zero_5bits is not 0\n");
    }

    /* Check level versus capability of the decoder */
    if (LevelIdc > 41)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.LevelIdcRangeError,
        "parse_sps.c: (ParseSPS) parse error: level_idc is higher than the decoder capability\n");
    }

    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].seq_parameter_set_id = SeqParameterSetId;

    /* Fill Profile and Level indication in GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication */
    ProfileAndLevelIndication = ((ConstraintSet3Flag << 19) | /* Bit [19] : constraint_set3_flag */
                                 (ConstraintSet2Flag << 18) | /* Bit [18] : constraint_set2_flag */
                                 (ConstraintSet1Flag << 17) | /* Bit [17] : constraint_set1_flag */
                                 (ConstraintSet0Flag << 16) | /* Bit [16] : constraint_set0_flag */
                                 (ProfileIdc << 8) |          /* Bits[15-8] : profile_idc        */
                                 LevelIdc                     /* Bits[7-0]  : level_idc          */
                                );

    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.ProfileAndLevelIndication = ProfileAndLevelIndication;

    if (ProfileIdc == PROFILE_HIGH)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_format_idc = h264par_GetUnsignedExpGolomb(ParserHandle);
        if(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_format_idc == 3)
        {
            PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].residual_colour_transform_flag = h264par_GetUnsigned(1, ParserHandle);
        }
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].bit_depth_luma_minus8 = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].bit_depth_chroma_minus8 = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].qpprime_y_zero_transform_bypass_flag = h264par_GetUnsigned(1, ParserHandle);
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].seq_scaling_matrix_present_flag = h264par_GetUnsigned(1, ParserHandle);
        if(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].seq_scaling_matrix_present_flag)
        {
            for(ScalingListNumber = 0 ; ScalingListNumber < 8 ; ScalingListNumber++)
            {
                SeqScalingListPresentFlag = h264par_GetUnsigned(1, ParserHandle);
                PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].ScalingMatrix.ScalingListPresentFlag[ScalingListNumber] = SeqScalingListPresentFlag;
                if(SeqScalingListPresentFlag)
                {
                    h264par_GetScalingList(ParserHandle, &(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].ScalingMatrix), ScalingListNumber);
                }
                else
                {
                    h264par_SetScalingListFallBackA(&(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].ScalingMatrix),ScalingListNumber);
                }
            }
        }
    }

    Log2MaxFrameNumMinus4 = h264par_GetUnsignedExpGolomb(ParserHandle);
    /* log2_max_frame_num_minus4 shall be in the range 0 to 12 inclusive */
    if (Log2MaxFrameNumMinus4 > 12)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxFrameNumMinus4RangeError,
        "parse_sps.c: (ParseSPS) parse error: log2_max_frame_num_minus4 exceeds range\n");
        Log2MaxFrameNumMinus4 = 12;
    }

    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_frame_num_minus4 = Log2MaxFrameNumMinus4;

    /* MaxFrameNum computation (for further POC processing)
     * MaxFrameNum = 2^(log2_max_frame_num_minus4 + 4)
     */
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxFrameNum = 1 << (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_frame_num_minus4 + 4);

    PicOrderCntType = h264par_GetUnsignedExpGolomb(ParserHandle);
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].pic_order_cnt_type = PicOrderCntType;
    /* Semantics checks on pic_order_cnt_type is implemented in the slice header parsing */

    if (PicOrderCntType == 0)
    {
        Log2MaxPixOrderCntLsbMinus4 = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* log2_max_pic_order_cnt_lsb_minus4 shall be in the range 0 to 12 inclusive */
        if (Log2MaxPixOrderCntLsbMinus4 > 12)
        {
            PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
            DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Log2MaxPicOrderCntLsbMinus4RangeError,
            "parse_sps.c: (ParseSPS) parse error: log2_max_pic_order_cnt_lsb_minus4 exceeds range\n");
            Log2MaxPixOrderCntLsbMinus4 = 12;
        }

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].log2_max_pic_order_cnt_lsb_minus4 = Log2MaxPixOrderCntLsbMinus4;

        /* MaxPicOrderCntLsb computation (for further POC processing) */
        /* MaxPicOrderCntLsb = 2^(log2_max_pic_order_cnt_lsb_minus4 + 4) */
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxPicOrderCntLsb = 1 << (Log2MaxPixOrderCntLsbMinus4 + 4);

        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].delta_pic_order_always_zero_flag = 0;
    }
    else if (PicOrderCntType == 1)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].delta_pic_order_always_zero_flag = h264par_GetUnsigned(1, ParserHandle);

        PARSER_Data_p->SPSLocalData[SeqParameterSetId].offset_for_non_ref_pic = h264par_GetSignedExpGolomb(ParserHandle);

        PARSER_Data_p->SPSLocalData[SeqParameterSetId].offset_for_top_to_bottom_field = h264par_GetSignedExpGolomb(ParserHandle);

        NumRefFramesInPicOrderCntCycle = h264par_GetUnsignedExpGolomb(ParserHandle);
        if (NumRefFramesInPicOrderCntCycle > 255)    /* max value as defined in 7.4.2.1 */
        {
            NumRefFramesInPicOrderCntCycle = 0;        /* Error case: set to 0 */
        }
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].num_ref_frames_in_pic_cnt_cycle = (U8)NumRefFramesInPicOrderCntCycle;

        for (i = 0; i < NumRefFramesInPicOrderCntCycle; i++)
        {
            PARSER_Data_p->SPSLocalData[SeqParameterSetId].offset_for_ref_frame[i] = h264par_GetSignedExpGolomb(ParserHandle);
        }

    }
    else
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].delta_pic_order_always_zero_flag = 0;
    }

    NumRefFrames = h264par_GetUnsignedExpGolomb(ParserHandle);

    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].NumberOfReferenceFrames = NumRefFrames;

    PARSER_Data_p->SPSLocalData[SeqParameterSetId].gaps_in_frame_num_allowed_flag = h264par_GetUnsigned(1, ParserHandle);

    PicWidthInMbsMinus1 = (U8)h264par_GetUnsignedExpGolomb(ParserHandle);
    if ((U8)(PicWidthInMbsMinus1 + 1) == 0)
    {
        PicWidthInMbsMinus1 = 0;        /* Error case: set to 0 (To be Checked) */
    }

    /* SequenceInfo.Width contains the width in pixel */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Width = 16 * (PicWidthInMbsMinus1 + 1);

    PicHeightInMapUnitsMinus1 = (U8)h264par_GetUnsignedExpGolomb(ParserHandle);
    if ((U8)(PicHeightInMapUnitsMinus1 + 1) == 0)
    {
        PicHeightInMapUnitsMinus1 = 0;        /* Error case: set to 0 (To be Checked) */
    }

    FrameMbsOnlyFlag = h264par_GetUnsigned (1, ParserHandle);
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].frame_mbs_only_flag = FrameMbsOnlyFlag;

    /* FrameHeightInMbs = (2 - frame_mbs_only_flag) * (PicHeightInMapUnitsMinus1 + 1) (equation (7-8)) */
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Height = 16 * (2 - FrameMbsOnlyFlag) * (PicHeightInMapUnitsMinus1 + 1);


    h264par_ComputeMaxFromLevel(LevelIdc, PicWidthInMbsMinus1, PicHeightInMapUnitsMinus1, FrameMbsOnlyFlag, SeqParameterSetId, ParserHandle);

    /* Initialize parameters that depend upon MaxDpbSize */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].num_reorder_frames = PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxDpbSize;
    PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].MaxDecFrameBuffering = PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxDpbSize;

    /* Initialize HRD parameters that depend upon MaxCPBSize & MaxBR*/
    h264par_InitializeDefaultHRDParameters(SeqParameterSetId, ParserHandle);

    /* Check range of MB */
    if (PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Width *
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Height >
        PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxPixel)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.WidthRangeError,
        "parse_sps.c: (ParseSPS) error: Number of macroblocks exceeds level_idc limit\n");
    }

    /* Check range of Width */
    if (PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Width > PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxWidth)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.WidthRangeError,
        "parse_sps.c: (ParseSPS) error: Width exceeds level_idc limit\n");
    }
    /* Check range of Height */
    if (PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Height > PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxHeight)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_BAD_DISPLAY;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.HeightRangeError,
        "parse_sps.c: (ParseSPS) error: Height exceeds level_idc limit\n");
    }

    /* Check range of NumRefFrames */
    if (PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].NumberOfReferenceFrames > PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxDpbSize) /* See corrigendum K012 */
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.NumRefFramesRangeError,
        "parse_sps.c: (ParseSPS) parse error: num_ref_frame exceeds range\n");
        /* Clip value */
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].NumberOfReferenceFrames = PARSER_Data_p->SPSLocalData[SeqParameterSetId].MaxDpbSize;
    }

    if (FrameMbsOnlyFlag == 0)
    {
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].mb_adaptive_frame_field_flag = h264par_GetUnsigned(1, ParserHandle);
    }

    Direct8x8InferenceFlag = h264par_GetUnsigned(1, ParserHandle);

    if ((FrameMbsOnlyFlag == 0) && (Direct8x8InferenceFlag == 0))
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Direct8x8InferenceFlagNullWithFrameMbsOnlyFlagError,
        "parse_sps.c: (ParseSPS) parse error: direct_8x8_inference_flag shall be equal to 1 when frame_mbs_only_flag is 0\n");
    }

    if ((Direct8x8InferenceFlag == 0) && (LevelIdc >= 30))
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_NONE;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.Direct8x8InferenceFlagNullWithLevel3OrMoreError,
        "parse_sps.c: (ParseSPS) parse error: direct_8x8_inference_flag shall be equal to 1 when LevelIdc >= 3\n");
        /* Direct8x8InferenceFlag is forced to 1 as this impacts the preprocessing */
        /* We assume here the encoder forgot to set this bit */
        Direct8x8InferenceFlag = 1;
    }
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].direct_8x8_inference_flag = Direct8x8InferenceFlag;

    FrameCroppingFlag = h264par_GetUnsigned(1, ParserHandle);

    if (FrameCroppingFlag == 1)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.LeftOffset = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.RightOffset = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.TopOffset = h264par_GetUnsignedExpGolomb(ParserHandle);
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.BottomOffset = h264par_GetUnsignedExpGolomb(ParserHandle);
        /* compute CropUnitX & CropUnitY according to equations (7-18) & (7-19) */
        switch (PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].chroma_format_idc)
        {
            case 1 : /* 4:2:0 */
                /* SubWidthC == 2 SubHeightC == 2 according to Table 6 1 */
                CropUnitX = 2;
                CropUnitY = 2 * ( 2 - FrameMbsOnlyFlag );
                break;

            case 2 : /* 4:2:2 */
                /* SubWidthC == 2 SubHeightC == 1 according to Table 6 1 */
                CropUnitX = 2;
                CropUnitY = ( 2 - FrameMbsOnlyFlag );
                break;

            case 3 : /* 4:4:4 */
            case 0 : /* Monochrome */
            default :
                /* SubWidthC == 1 SubHeightC == 1 or SubWidthC & SubHeightC not applicable (monochrome) according to Table 6 1 */
                CropUnitX = 1;
                CropUnitY = ( 2 - FrameMbsOnlyFlag );
                break;
        }
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.LeftOffset *= CropUnitX;
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.RightOffset *= CropUnitX;
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.TopOffset *= CropUnitY;
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.BottomOffset *= CropUnitY;
    }

    /* Test that display height is no bigger than 1080 lines. If it is, crop it from bottom */
    PictureDisplayHeightAfterCropping = PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Height
                                        - PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.TopOffset
                                        - PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.BottomOffset;

    /*- DVB states that height 720 streams are progressive only */
    if (PictureDisplayHeightAfterCropping == 720)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
    }

    if (PictureDisplayHeightAfterCropping > 1080)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].FrameCropInPixel.BottomOffset =
                PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].SequenceInfo.Height - 1080;
    }

    VuiParametersPresentFlag = h264par_GetUnsigned(1, ParserHandle);
    if (VuiParametersPresentFlag == 1)
    {
        h264par_ParseVUI(SeqParameterSetId, ParserHandle);
    }

    /* This SPS is now available for use */
    PARSER_Data_p->SPSLocalData[SeqParameterSetId].IsAvailable = TRUE;

    /* Check wether end of NAL has been reached while parsing */
    if (PARSER_Data_p->NALByteStream.EndOfStreamFlag == TRUE)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.TruncatedNAL = TRUE;
    }

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpSPS(ParserHandle, SeqParameterSetId);
#endif

    /* Set the HasChanged flag so that the new SPS is sent to the MME when necessary */
    /* Removed the memcmp, the overhead of memcmps is probably higher than sending */
    PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPS_HasChanged = TRUE;
#if defined(TRACE_UART)
    TraceBuffer(("new SPS %d received\r\n", (U32)SeqParameterSetId));
#endif
}

/******************************************************************************/
/*! \brief Parses a SPS extension and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParseSPSExtension (const PARSER_Handle_t  ParserHandle)
{
    /* the parser knows where to store the parameters once it reads the seq_parameter_set_id
     * As this parameter comes only in 6th position, the function stores the first 5
     * parameters in local variables before storing them
     */
    U8  SeqParameterSetId; /* the index where to store the parsed SPS parameters */
    U8  AuxFormatIdc;      /* Indicates if auxiliary coded pictures are present and what to do with */
    U8  BitDepthAuxMinus8;
    U8  AlphaIncrFlag;
    U16 AlphaOpaqueValue;
    U16 AlphaTransparentValue;
    U8  AdditionalExtensionFlag;

    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    /* For profiles other than High profile, this NAL unit is reserved, thus shall not be parsed */
    if (PARSER_Data_p->ParserGlobalVariable.ProfileIdc != PROFILE_HIGH)
    {
      return;
    }

    SeqParameterSetId = h264par_GetUnsignedExpGolomb(ParserHandle);
    if (SeqParameterSetId > (MAX_SPS-1))
    {
        return;
    }

    AuxFormatIdc = h264par_GetUnsignedExpGolomb(ParserHandle);

    if (AuxFormatIdc != 0)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_INFORMATION;
        DumpError(PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.AuxCodedPicturePresent,
        "parse_sps.c: (ParseSPSExtension) parse warning: stream contains aux_format_idc != 0\n");

        BitDepthAuxMinus8 = h264par_GetUnsignedExpGolomb(ParserHandle);
        AlphaIncrFlag = h264par_GetUnsigned(1, ParserHandle);
        AlphaOpaqueValue = h264par_GetUnsigned(BitDepthAuxMinus8+9, ParserHandle);
        AlphaTransparentValue = h264par_GetUnsigned(BitDepthAuxMinus8+9, ParserHandle);
    }

    AdditionalExtensionFlag = h264par_GetUnsigned(1, ParserHandle);

    /* Check wether end of NAL has been reached while parsing */
    if (PARSER_Data_p->NALByteStream.EndOfStreamFlag == TRUE)
    {
        PARSER_Data_p->GlobalDecodingContextGenericData[SeqParameterSetId].ParsingError = VIDCOM_ERROR_LEVEL_CRASH;
        PARSER_Data_p->GlobalDecodingContextSpecificData[SeqParameterSetId].SPSError.TruncatedNAL = TRUE;
    }
}
