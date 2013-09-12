/*!
 *******************************************************************************
 * \file parse_pps.c
 *
 * \brief H264 PPS parsing and filling of data structures
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
#endif

/* H264 Parser specific */
#include "h264parser.h"

#if defined TRACE_UART
#include "trace.h"
#endif /* TRACE_UART */

/* Functions ---------------------------------------------------------------- */

/******************************************************************************/
/*! \brief Initialize a PPS data structure.
 *
 * \param PicParameterSetId the PPS to initialize
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_InitializePPS (U8 PicParameterSetId, const PARSER_Handle_t  ParserHandle)
{
    U32 ScalingListCounter;
    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    /* Initialize errors in PPS */
    /* Errors will stay present until the PPS is erased by another PPS with the
     * same pic_parameter_set_id
     */
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].transform_8x8_mode_flag = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_scaling_matrix_present_flag = FALSE;
    /* Only set to avoid unset values but not mandatory because */
    for(ScalingListCounter = 0 ; ScalingListCounter < 8 ; ScalingListCounter++)
    {
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrix.ScalingListPresentFlag[ScalingListCounter] = FALSE;
    }
    h264par_SetFlatScalingMatrix(&(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrix));
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrixUpdatedFromSPS = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.NumSliceGroupsMinus1NotNullError = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.NumRefIdxActiveMinus1RangeError = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.WeightedBiPredIdcRangeError = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.PicInitQpMinus26RangeError = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.PicInitQsMinus26RangeError = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.ChromaQpIndexOffsetRangeError = FALSE;
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.TruncatedNAL= FALSE;
}

/******************************************************************************/
/*! \brief Parses a PPS and fills data structures.
 *
 * PictureDecodingContextSpecificData is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void h264par_ParsePPS (const PARSER_Handle_t  ParserHandle)
{
    U16      PicParameterSetId; /* the same as the stream element pic_parameter_set_id */
    U8      NumSliceGroupsMinus1; /* the stream element: num_slice_groups_minus1 */
    S8      PicInitQsMinus26; /* the stream element: pic_init_qs_minus26 */
    BOOL    PicScalingListPresentFlag;
    U32     NumberOfScalingLists;
    U32     ScalingListNumber;

    /* dereference ParserHandle to a local variable to ease debug */
    H264ParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((H264ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PicParameterSetId = h264par_GetUnsignedExpGolomb(ParserHandle);
    if (PicParameterSetId > (MAX_PPS-1))
    {
        return;
    }

    h264par_InitializePPS(PicParameterSetId, ParserHandle);

    /* PicParameterSetId is the index to store the PPS */
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_parameter_set_id = PicParameterSetId;

    PARSER_Data_p->PPSLocalData[PicParameterSetId].seq_parameter_set_id = h264par_GetUnsignedExpGolomb(ParserHandle);

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].entropy_coding_mode_flag = h264par_GetUnsigned(1, ParserHandle);

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_order_present_flag = h264par_GetUnsigned(1, ParserHandle);

    NumSliceGroupsMinus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
    if (NumSliceGroupsMinus1 != 0)
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.NumSliceGroupsMinus1NotNullError,
        "parse_pps: (ParsePPS) error, num_slice_groups_minus1 is not equal to 0\n");
    }

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].num_ref_idx_l0_active_minus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
    if (PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].num_ref_idx_l0_active_minus1 > 31)
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.NumRefIdxActiveMinus1RangeError,
        "parse_pps: (ParsePPS) error, num_ref_idx_l0_active_minus1 exceeds range\n");
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].num_ref_idx_l0_active_minus1 = 1;
    }
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].num_ref_idx_l1_active_minus1 = h264par_GetUnsignedExpGolomb(ParserHandle);
    if (PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].num_ref_idx_l1_active_minus1 > 31)
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.NumRefIdxActiveMinus1RangeError,
        "parse_pps: (ParsePPS) error, num_ref_idx_l1_active_minus1 exceeds range\n");
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].num_ref_idx_l1_active_minus1 = 1;
    }

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].weighted_pred_flag = h264par_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].weighted_bipred_idc = h264par_GetUnsigned(2, ParserHandle);
    if (PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].weighted_bipred_idc > 2)
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.WeightedBiPredIdcRangeError,
        "parse_pps: (ParsePPS) error, weighted_bipred_idc exceeds range\n");
    }

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_init_qp_minus26 = h264par_GetSignedExpGolomb(ParserHandle);
    if ((PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_init_qp_minus26 < -26) ||
        (PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_init_qp_minus26 > 25)
       )
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.PicInitQpMinus26RangeError,
        "parse_pps: (ParsePPS) error, pic_init_qp_minus26 exceeds range\n");
    }
    PicInitQsMinus26 = h264par_GetSignedExpGolomb(ParserHandle); /* not used any further */
    if ((PicInitQsMinus26 < -26) ||     (PicInitQsMinus26 > 25))
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.PicInitQsMinus26RangeError,
        "parse_pps: (ParsePPS) error, pic_init_qs_minus26 exceeds range\n");
    }

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].chroma_qp_index_offset = h264par_GetSignedExpGolomb(ParserHandle);
    if ((PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].chroma_qp_index_offset < -12) ||
        (PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].chroma_qp_index_offset > 12)
       )
    {
        DumpError(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.ChromaQpIndexOffsetRangeError,
        "parse_pps: (ParsePPS) error, chroma_qp_index_offset exceeds range\n");
    }

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].deblocking_filter_control_present_flag = h264par_GetUnsigned(1, ParserHandle);

    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].constrained_intra_pred_flag = h264par_GetUnsigned(1, ParserHandle);

    /* redundant_pic_cnt_present_flag is discarded from the bistream as nobody uses it */
    h264par_GetUnsigned(1, ParserHandle);

    /* Get high profile data if any */
    if(h264par_IsMoreRbspData(ParserHandle))
    {
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].transform_8x8_mode_flag = h264par_GetUnsigned(1, ParserHandle);
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_scaling_matrix_present_flag = h264par_GetUnsigned(1, ParserHandle);
        if(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].pic_scaling_matrix_present_flag)
        {
            if(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].transform_8x8_mode_flag)
            {
                NumberOfScalingLists = 8;
            }
            else
            {
                NumberOfScalingLists = 6;
                PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrix.ScalingListPresentFlag[6] = FALSE;
                PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrix.ScalingListPresentFlag[7] = FALSE;
            }
            for(ScalingListNumber = 0 ; ScalingListNumber < NumberOfScalingLists ; ScalingListNumber++)
            {
                PicScalingListPresentFlag = h264par_GetUnsigned(1, ParserHandle);
                PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrix.ScalingListPresentFlag[ScalingListNumber] = PicScalingListPresentFlag;
                if(PicScalingListPresentFlag)
                {
                    h264par_GetScalingList(ParserHandle, &(PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].ScalingMatrix), ScalingListNumber);
                }
            }
        }
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].second_chroma_qp_index_offset = h264par_GetSignedExpGolomb(ParserHandle);
    }
    else
    { /* default value for second_chroma_qp_index_offset if not present is chroma_qp_index_offset */
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].second_chroma_qp_index_offset = PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].chroma_qp_index_offset;
    }

    /* This PPS is now available for use */
    PARSER_Data_p->PPSLocalData[PicParameterSetId].IsAvailable = TRUE;

    /* Check wether end of NAL has been reached while parsing */
    if (PARSER_Data_p->NALByteStream.EndOfStreamFlag == TRUE)
    {
        PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPSError.TruncatedNAL = TRUE;
    }

#if defined(STVID_PARSER_DUMP) && defined(VALID_TOOLS)
    H264ParserDumpPPS(ParserHandle, PicParameterSetId);
#endif

    /* Set the HasChanged flag so that the new SPS is sent to the MME when necessary */
    /* Removed the memcmp, the overhead of memcmps is probably higher than sending */
    PARSER_Data_p->PictureDecodingContextSpecificData[PicParameterSetId].PPS_HasChanged = TRUE;
#if defined(TRACE_UART)
    TraceBuffer(("PPS %d has changed -- %d\r\n", (U32)PicParameterSetId, PARSER_Data_p->PictureGenericData.DecodingOrderFrameID));
#endif

}
