/*******************************************************************************
File Name   : vcodh264.h

Description : Video driver CODEC H264 header file

Copyright (C) 2004 STMicroelectronics

Date               Modification                                     Name
----               ------------                                     ----
26 Jan 2004        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDCOD_H264_H
#define __STVIDCOD_H264_H

/* Includes ----------------------------------------------------------------- */
#include "stddefs.h"
#include "stos.h"

#ifndef ST_OSLINUX
#include "stlite.h"
#endif

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#include "vidcodec.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Exported Constants ------------------------------------------------------- */

extern const DECODER_FunctionsTable_t   DECODER_H264Functions;
extern const PARSER_FunctionsTable_t    PARSER_H264Functions;


/* Exported Types ----------------------------------------------------------- */

/* Maximum number of SPS to manage in the SPS array */
#define MAX_SPS 32

/* Maximum number of PPS to manage in the PPS array */
#define MAX_PPS 256

typedef struct H264_SPSError_s
{
	BOOL MaxBytesPerPicDenomRangeError;
	BOOL MaxBitsPerMBDenomError;
	BOOL Log2MaxMvLengthHorizontalRangeError;
	BOOL Log2MaxMvLengthVerticalRangeError;
	BOOL NumReorderFrameRangeError;
	BOOL MaxDecBufferingRangeError;
    BOOL IsNotSupportedProfileError;
    BOOL AuxCodedPicturePresent;
    BOOL ReservedZero4BitsError;
	BOOL LevelIdcRangeError;
    BOOL Log2MaxFrameNumMinus4RangeError;
	BOOL Log2MaxPicOrderCntLsbMinus4RangeError;
	BOOL NumRefFramesRangeError;
	BOOL Direct8x8InferenceFlagNullWithFrameMbsOnlyFlagError;
    BOOL Direct8x8InferenceFlagNullWithLevel3OrMoreError;
    BOOL TruncatedNAL; /* set to TRUE when the parsing exceeds NAL boundary */
	BOOL NalCpbCntMinus1RangeError;
	BOOL NalBitRateRangeError;
	BOOL NalCpbSizeRangeError;
	BOOL NonIncreasingNalBitRateError;
	BOOL NonIncreasingNalCpbSizeError;
	BOOL VclCpbCntMinus1RangeError;
	BOOL VclBitRateRangeError;
	BOOL VclCpbSizeRangeError;
	BOOL NonIncreasingVclBitRateError;
	BOOL NonIncreasingVclCpbSizeError;
	BOOL InitialCpbRemovalDelayDifferError;
	BOOL CpbRemovalDelayLengthDifferError;
	BOOL DpbOutputDelayLengthDifferError;
	BOOL TimeOffsetLengthDifferError;
	BOOL LowDelayHrdFlagNotNullError;
	BOOL WidthRangeError;
	BOOL HeightRangeError;
	BOOL AspectRatioRangeError;
} H264_SPSError_t;

typedef struct H264_SpecificScalingMatrix_s
{
    BOOL    ScalingListPresentFlag[8];
    U32     FirstSixScalingList[6][16];
    U32     NextTwoScalingList[2][64];
} H264_SpecificScalingMatrix_t;

typedef struct H264_GlobalDecodingContextSpecificData_s
{
    /* From SPS */
    U32		seq_parameter_set_id;
    U32     chroma_format_idc;
    BOOL    residual_colour_transform_flag;
    U32     bit_depth_luma_minus8;
    U32     bit_depth_chroma_minus8;
    BOOL    qpprime_y_zero_transform_bypass_flag;
    BOOL    seq_scaling_matrix_present_flag;
    H264_SpecificScalingMatrix_t  ScalingMatrix;
    U32		log2_max_frame_num_minus4;
    U32		pic_order_cnt_type;
    U32		log2_max_pic_order_cnt_lsb_minus4;
    U32		delta_pic_order_always_zero_flag;
    U32 	frame_mbs_only_flag;
    U32		mb_adaptive_frame_field_flag;
    U32		direct_8x8_inference_flag;
    /* From VUI */
    BOOL    overscan_appropriate_flag; /* display */
    BOOL	video_full_range_flag; /* display */
    U32		chroma_sample_loc_type_top_field; /* display */
    U32 	chroma_sample_loc_type_bottom_field; /* display */
    BOOL	motion_vectors_over_pic_boundaries_flag; /* control */
    U32		max_bytes_per_pic_denom; /* control */
    U32		max_bits_per_mb_denom; /* control */
    U32 	log2_max_mv_length_horizontal; /* control */
    U32		log2_max_mv_length_vertical; /* control */
    U32 	num_reorder_frames; /* control */
    U32		nal_cpb_cnt_minus1; /* control */
    U32		vcl_cpb_cnt_minus1; /* control */
    U32		nal_BitRate[32]; /* array of cpb_cnt_minus1 elements */ /* control */
    U32		nal_CpbSize[32]; /* array of cpb_cnt_minus1 elements */ /* control */
    U32		nal_cbr_flag[32]; /* array of cpb_cnt_minus1 elements */ /* control */
    U32		vcl_BitRate[32]; /* array of cpb_cnt_minus1 elements */ /* control */
    U32		vcl_CpbSize[32]; /* array of cpb_cnt_minus1 elements */ /* control */
    U32		vcl_cbr_flag[32]; /* array of cpb_cnt_minus1 elements */ /* control */
    H264_SPSError_t SPSError;
    BOOL SPS_HasChanged;
} H264_GlobalDecodingContextSpecificData_t;

typedef struct H264_PPSError_s
{
	BOOL NumSliceGroupsMinus1NotNullError;
	BOOL NumRefIdxActiveMinus1RangeError;
	BOOL WeightedBiPredIdcRangeError;
	BOOL PicInitQpMinus26RangeError;
	BOOL PicInitQsMinus26RangeError;
	BOOL ChromaQpIndexOffsetRangeError;
    BOOL TruncatedNAL; /* set to TRUE when the parsing exceeds NAL boundary */
} H264_PPSError_t;

typedef struct H264_PictureDecodingContextSpecificData_s
{
    U32		pic_parameter_set_id;
    U32 	entropy_coding_mode_flag;
    U32     pic_order_present_flag;
    U32 	num_ref_idx_l0_active_minus1;
    U32		num_ref_idx_l1_active_minus1;
    U32		weighted_pred_flag;
    U32		weighted_bipred_idc;
    S32     pic_init_qp_minus26;
    S32		chroma_qp_index_offset;
    U32 	deblocking_filter_control_present_flag;
    U32		constrained_intra_pred_flag;
    BOOL    transform_8x8_mode_flag;
    BOOL    pic_scaling_matrix_present_flag;
    H264_SpecificScalingMatrix_t  ScalingMatrix;
    BOOL    ScalingMatrixUpdatedFromSPS;
    S32     second_chroma_qp_index_offset;
    H264_PPSError_t PPSError;
    BOOL PPS_HasChanged;
} H264_PictureDecodingContextSpecificData_t;

typedef struct H264_SpecificReferenceList_s
{
	BOOL IsLongTerm;
    BOOL IsNonExisting;
	S32  PictureNumber;
    S32  NonExistingPicOrderCnt;
} H264_SpecificReferenceList_t;

typedef struct H264_ClockTS_s
{
    BOOL    discontinuity_flag;
    U32     clockTimeStamp;
} H264_ClockTS_t;

typedef struct H264_MarkingError_s
{
	BOOL MarkInUsedFrameBufferError;
	BOOL DPBRefFullError;
	BOOL NoShortTermToSlideError;
	BOOL NoShortTermToMarkAsUnusedError;
	BOOL NoLongTermToMarkAsUnusedError;
	BOOL MarkNonExistingAsLongError;
	BOOL NoShortPictureToMarkAsLongError;
	BOOL Marking2ndFieldWithDifferentLongTermFrameIdxError;
	BOOL NoLongTermAssignmentAllowedError;
	BOOL LongTermFrameIdxOutOfRangeError;
} H264_MarkingError_t;

typedef struct H264_SliceError_s
{
	BOOL NalRefIdcNullForIDRError;
	BOOL NotTheFirstSliceError;
	BOOL NoPPSAvailableError;
	BOOL NoSPSAvailableError;
	BOOL UnintentionalLossOfPictureError;
	BOOL DeltaPicOrderCntBottomRangeError;
	BOOL PicOrderCntType2SemanticsError;
	BOOL LongTermReferenceFlagNotNullError;
	BOOL MaxLongTermFrameIdxPlus1RangeError;
	BOOL DuplicateMMCO4Error;
	BOOL MMCO1OrMMCO3WithMMCO5Error;
	BOOL DuplicateMMCO5Error;
	BOOL IdrPicIdViolationError;
	BOOL NumRefIdxActiveMinus1RangeError;
	BOOL NumRefIdxActiveOverrrideFlagNullError;
    BOOL SameParityOnComplementaryFieldsError;
} H264_SliceError_t;

typedef struct H264_POCError_s
{
	BOOL IDRWithPOCNotNullError;
	BOOL PicOrderCntTypeRangeError;
	BOOL ListDOverFlowError; /* fixed length buffer ListD is full (too many pictures between 2 DPB refresh) */
	BOOL FieldOrderCntNotConsecutiveInListOError;
	BOOL DuplicateTopFieldOrderCntInListOError;
	BOOL DuplicateBottomFieldOrderCntInListOError;
	BOOL NonPairedFieldsShareSameFieldOrderCntInListOError;
	BOOL DifferenceBetweenPicOrderCntExceedsAllowedRangeError;
} H264_POCError_t;

typedef struct H264_SEIError_s
{
    BOOL ReservedPictStructSEIMessageError;
    BOOL TruncatedNAL; /* set to TRUE when the parsing exceeds NAL boundary */
} H264_SEIError_t;

typedef struct H264_PictureError_s
{
	H264_SliceError_t SliceError;
	H264_POCError_t POCError;
	H264_MarkingError_t MarkingError;
	H264_SEIError_t SEIError;
    BOOL TruncatedNAL; /* set to TRUE when the parsing exceeds NAL boundary */
} H264_PictureError_t;

typedef struct H264_PictureSpecificData_s
{
    BOOL            IsIDR;
    S32				PicOrderCntTop; /* see MME API */
    S32				PicOrderCntBot; /* see MME API */
    S32             PreDecodingPicOrderCntTop; /* POC Top to use while decoding a MMCO=5 picture */
    S32             PreDecodingPicOrderCntBot; /* POC Bot to use while decoding a MMCO=5 picture */
    S32             PreDecodingPicOrderCnt;    /* POC to use while decoding a MMCO=5 picture */
    H264_SpecificReferenceList_t  FullReferenceTopFieldList[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES]; /* Type of same index in FullReferenceFrameList */
    H264_SpecificReferenceList_t  FullReferenceBottomFieldList[VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES];
    BOOL            IsReferenceTopFieldP0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
    BOOL            IsReferenceTopFieldB0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
    BOOL            IsReferenceTopFieldB1[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
    U32             NumClockTS; /* size of next array */
    H264_ClockTS_t  ClockTS[3]; /* array of up to 3 elements */
	H264_PictureError_t PictureError;
} H264_PictureSpecificData_t;

typedef struct H264_DECODERError_s
{
    /* TBD !!! */
    BOOL SyntaxError;

} H264_DECODERError_t;

typedef struct H264_DECODERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} H264_DECODERStatistics_t;


typedef struct H264_PARSERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} H264_PARSERStatistics_t;

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
#ifdef VALID_TOOLS
ST_ErrorCode_t H264PARSER_RegisterTrace(void);
ST_ErrorCode_t H264DECODER_RegisterTrace(void);
#endif /* VALID_TOOLS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDCOD_H264_H */

/* End of vcodh264.h */
