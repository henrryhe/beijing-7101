/*!
 *******************************************************************************
 * \file parser.c
 *
 * \brief MPEG4 Part2 parsing and filling of data structures
 *
 * \author
 * Sibnath Ghosh \n
 * \n
 * Copyright (C) 2005 STMicroelectronics
 *
 *******************************************************************************
 */

/* TBD - To Be Done */
/* TBC - To Be Checked */
/* NK  - Not Known */

/* Includes ----------------------------------------------------------------- */
/* System */
#if !defined ST_OSLINUX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
/* mpeg4p2 Parser specific */
#include "mpg4p2parser.h"

/* Workaround for GNBvd57772: BFF MPEG4P2 streams are badly displayed 7100/7109 (field inversions) */
/*#define WA_GNBvd57772*/
#define DIVX_CODEC 1  /* define only needed to parse DivX userData */

#define MAXIMUM_U32 0xFFFFFFFF
#define MINIMUM_S32 0x80000000
#define MAXIMUM_S32 0x7FFFFFFF
static void mpeg4p2par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle);
__inline U32 CountNumberOfBits(U32 value);

/* Macro ----------------------------------------------------------------- */
 #if defined(ST_OSLINUX)
 #define atoi(a)  simple_strtol(a,NULL,10);
 #endif
/* Functions ----------------------------------------------------------------- */
__inline U32 CountNumberOfBits(U32 value)
{
	U8 Temp;
	U32 i;

	i = 0;
	do {
		i++;
		Temp = ((value >> (32 - i)) & 0x1);
	}while(Temp != 1);

	return (32 - (i - 1));
}

/******************************************************************************/
/*! \brief Initializes the structure variables.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_InitParserVariables (const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->VideoObjectLayer.LowDelay = 0;
	PARSER_Data_p->VideoObjectLayer.Bitrate = 0;
	PARSER_Data_p->VideoObjectLayer.VBVBufferSize = 0;

}


/******************************************************************************/
/*! \brief Parses a Visual Object Sequence Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseVisualObjectSequenceHeader (const PARSER_Handle_t  ParserHandle)
{
	U8 profile_and_level_indication;
	U32 Temp;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	profile_and_level_indication = mpeg4p2par_GetUnsigned(8, ParserHandle);
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication = profile_and_level_indication;

	while ((Temp = mpeg4p2par_GetUnsigned(32, ParserHandle)) == USER_DATA_START_CODE)
		mpeg4p2par_ParseUserDataHeader(ParserHandle);

#if defined(TRACE_UART)
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSESEQ, "Profile&Level: %x \n", profile_and_level_indication);
#endif
}

/******************************************************************************/
/*! \brief Parses a Visual Object Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseVisualObjectHeader (const PARSER_Handle_t  ParserHandle)
{
	U8 is_visual_object_identifier;
	U8 visual_object_verid;
	U8 visual_object_priority;
	U8 visual_object_type;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	is_visual_object_identifier = mpeg4p2par_GetUnsigned(1, ParserHandle);
	if (is_visual_object_identifier)
	{
		visual_object_verid = mpeg4p2par_GetUnsigned(4, ParserHandle);
		visual_object_priority = mpeg4p2par_GetUnsigned(3, ParserHandle);
	}
	else
		visual_object_verid = 1;

	visual_object_type = mpeg4p2par_GetUnsigned(4, ParserHandle);

#if defined(TRACE_UART)
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVO, "VisualObjectIdentifier: %d \n", is_visual_object_identifier);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVO, "VisualObjectVerId: %x \n", visual_object_verid);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVO, "VisualObjectPriority: %d \n", visual_object_priority);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVO, "VisualObjectType: %x \n", visual_object_type);
#endif

	if (visual_object_type == VIDEO_ID || visual_object_type == STILL_TEXTURE_ID)
		mpeg4p2par_ParseVideoSignalType(ParserHandle);

}

/******************************************************************************/
/*! \brief Parses a Video Object Layer Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
U32 mpeg4p2par_ParseVideoObjectLayerHeader (const PARSER_Handle_t  ParserHandle)
{
	/* U8 video_object_type_indication; */
	U8 is_object_layer_identifier;
	U8 video_object_layer_verid;
	U8 video_object_layer_priority;
	U8 aspect_ratio_info;
	/* U8 par_width, par_height; */
	U8 vol_control_parameters;
	U8 chroma_format;
	U8 vbv_parameters;
	U16 first_half_bit_rate;
	U16 latter_half_bit_rate;
	/* U32 bit_rate; */
	U16 first_half_vbv_buffer_size;
	U16 latter_half_vbv_buffer_size;
	U8 video_object_layer_shape;
	U8 video_object_layer_shape_extension;
	U32 vop_time_increment_resolution;
	U8 fixed_vop_rate = 0;
	U32 video_object_layer_width;
	U32 video_object_layer_height;
	/* U8 marker_bit; */
	U8 interlace = 0;
	U8 sprite_enable;
	U8 not_8_bit;
	U8 bits_per_pixel;
	U8 quant_type;
	U8 complexity_estimation_disable;
	U8 newpred_enable;
	U8 scalability;
	U8 load_intra_quant_mat;
	U8 load_nonintra_quant_mat;
	U8 load_intra_quant_mat_grayscale;
	U8 load_nonintra_quant_mat_grayscale;
	U8 hierarchy_type;
	U8 enhancement_type;
	U32 flush_bits;
	U32 Temp;
	U32 i;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

	Temp = mpeg4p2par_GetUnsigned(9, ParserHandle);	/* random_accessible_vol, video_object_type_indication */
	PARSER_Data_p->VideoObjectLayer.RandomAccessibleVOL = ((Temp >> 8) & 0x1);

	is_object_layer_identifier = mpeg4p2par_GetUnsigned(1, ParserHandle);
	if (is_object_layer_identifier)
	{
		video_object_layer_verid = mpeg4p2par_GetUnsigned(4, ParserHandle);
		video_object_layer_priority = mpeg4p2par_GetUnsigned(3, ParserHandle);
	}
	else
		video_object_layer_verid = 1;

	aspect_ratio_info = mpeg4p2par_GetUnsigned(4, ParserHandle);
	if (aspect_ratio_info == EXTENDED_PAR)
	{
      PARSER_Data_p->VideoObjectLayer.PARWidth = mpeg4p2par_GetUnsigned(8, ParserHandle);
      PARSER_Data_p->VideoObjectLayer.PARHeight = mpeg4p2par_GetUnsigned(8, ParserHandle);
   }

	vol_control_parameters = mpeg4p2par_GetUnsigned(1, ParserHandle);
	if (vol_control_parameters)
	{
		chroma_format = mpeg4p2par_GetUnsigned(2, ParserHandle);
		PARSER_Data_p->VideoObjectLayer.LowDelay = mpeg4p2par_GetUnsigned(1, ParserHandle);
		vbv_parameters = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (vbv_parameters)
		{
			first_half_bit_rate = mpeg4p2par_GetUnsigned(15, ParserHandle);
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);
			latter_half_bit_rate = mpeg4p2par_GetUnsigned(15, ParserHandle);
			PARSER_Data_p->VideoObjectLayer.Bitrate = 400 * ((first_half_bit_rate << 15) | (latter_half_bit_rate));

			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);

			first_half_vbv_buffer_size = mpeg4p2par_GetUnsigned(15, ParserHandle);
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);
			latter_half_vbv_buffer_size = mpeg4p2par_GetUnsigned(3, ParserHandle);
			PARSER_Data_p->VideoObjectLayer.VBVBufferSize = ((16384 * ((first_half_vbv_buffer_size << 3) | (latter_half_vbv_buffer_size))) / 8); /* size in bytes */

			flush_bits = mpeg4p2par_GetUnsigned(28, ParserHandle);	/* vbv_occupancy */
		}
		else
		{
			if (PARSER_Data_p->VideoObjectLayer.Bitrate == 0)
				PARSER_Data_p->VideoObjectLayer.Bitrate = MPEG4P2_DEFAULT_BIT_RATE;
			if (PARSER_Data_p->VideoObjectLayer.VBVBufferSize == 0)
				PARSER_Data_p->VideoObjectLayer.VBVBufferSize = MPEG4P2_DEFAULT_VBVMAX;
		}
	}
	else
	{
		chroma_format = 1;	/* 4:2:0 */
		PARSER_Data_p->VideoObjectLayer.LowDelay = 1;	/* CAREFUL; the default value is 0 for VO types that support B-VOP otherwise it is 1 */
		if (PARSER_Data_p->VideoObjectLayer.Bitrate == 0)
			PARSER_Data_p->VideoObjectLayer.Bitrate = MPEG4P2_DEFAULT_BIT_RATE;
		if (PARSER_Data_p->VideoObjectLayer.VBVBufferSize == 0)
			PARSER_Data_p->VideoObjectLayer.VBVBufferSize = MPEG4P2_DEFAULT_VBVMAX;
	}

	video_object_layer_shape = mpeg4p2par_GetUnsigned(2, ParserHandle);
	PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape = video_object_layer_shape;

	if (video_object_layer_shape == GRAYSCALE && video_object_layer_verid != 0x01)
		video_object_layer_shape_extension = mpeg4p2par_GetUnsigned(4, ParserHandle);
	else
		video_object_layer_shape_extension = 0;

	switch (video_object_layer_shape_extension)
	{
		case 0:
		case 1:
		case 5:
		case 7:
		case 8:
			PARSER_Data_p->VideoObjectLayer.AuxCompCount = 1;
			break;

		case 2:
		case 3:
		case 6:
		case 9:
		case 11:
			PARSER_Data_p->VideoObjectLayer.AuxCompCount = 2;
			break;

		case 4:
		case 10:
		case 12:
			PARSER_Data_p->VideoObjectLayer.AuxCompCount = 3;
			break;

	}

	Temp = mpeg4p2par_GetUnsigned(19, ParserHandle);	/* marker_bit, vop_time_increment_resolution, marker_bit, fixed_vop_rate */
	vop_time_increment_resolution = (Temp >> 2) & 0xFFFF;
	PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution = vop_time_increment_resolution;
	fixed_vop_rate = Temp & 0x1;
	PARSER_Data_p->VideoObjectLayer.FixedVOPRate = fixed_vop_rate;

	if (fixed_vop_rate)
	{
		U32 NoOfBits = CountNumberOfBits(vop_time_increment_resolution);
		NoOfBits =  (NoOfBits < 1)? 1 : NoOfBits;
		PARSER_Data_p->VideoObjectLayer.FixedVOPTimeIncrement = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);	/* gives the fixed vop or frame rate */
	}
	/* STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VOPIncrReso: %d fixedVOPRate %d VOPTimeIncr %d\n", vop_time_increment_resolution, fixed_vop_rate, PARSER_Data_p->VideoObjectLayer.FixedVOPTimeIncrement));*/

	if (video_object_layer_shape != BINARY_ONLY)
	{
		if (video_object_layer_shape == RECTANGULAR)
		{
			Temp = mpeg4p2par_GetUnsigned(29, ParserHandle);	/* marker_bit, video_object_layer_width, marker_bit, video_object_layer_height, marker_bit  */
			video_object_layer_width = ((Temp >> 15) & 0x1FFF);
			video_object_layer_height = ((Temp >> 1) & 0x1FFF);
			PARSER_Data_p->VideoObjectPlane.VOPWidth = video_object_layer_width;
			PARSER_Data_p->VideoObjectPlane.VOPHeight = video_object_layer_height;
		}

		interlace = (mpeg4p2par_GetUnsigned(2, ParserHandle) >> 1);	/* interlace, obmc_disable */

        PARSER_Data_p->VideoObjectLayer.Interlace = interlace;

		if (video_object_layer_verid == 0x1)
			sprite_enable = (mpeg4p2par_GetUnsigned(1, ParserHandle) & 0x1);
		else
			sprite_enable = (mpeg4p2par_GetUnsigned(2, ParserHandle) & 0x3);
		PARSER_Data_p->VideoObjectLayer.SpriteEnable = sprite_enable;

		if (sprite_enable == STATIC_SPRITE || sprite_enable == GMC_SPRITE)
		{
			if (sprite_enable != GMC_SPRITE)
			{
				flush_bits = mpeg4p2par_GetUnsigned(28, ParserHandle);	/*	sprite_width, marker_bit, sprite_height, marker_bit */
				flush_bits = mpeg4p2par_GetUnsigned(28, ParserHandle);	/* sprite_left_coordinate, marker_bit, sprite_top_coordinate, marker_bit */
			}
			Temp = mpeg4p2par_GetUnsigned(9, ParserHandle);	/* no_of_sprite_warping_points, sprite_warping_accuracy, sprite_brightness_change */
			PARSER_Data_p->VideoObjectLayer.SpriteWarpingPoints = ((Temp >> 3) & 0x3F);
			PARSER_Data_p->VideoObjectLayer.SpriteWarpingAccuracy = ((Temp >> 1) & 0x3);
			PARSER_Data_p->VideoObjectLayer.SpriteBrightnessChange = (Temp & 0x1);

			if (sprite_enable != GMC_SPRITE)
				flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	low_latency_sprite_enable */
		}
		else
		{
			PARSER_Data_p->VideoObjectLayer.SpriteWarpingPoints = 0;	/* Stationary */
			PARSER_Data_p->VideoObjectLayer.SpriteWarpingAccuracy = 0;	/* 1/2 Pixel */
			PARSER_Data_p->VideoObjectLayer.SpriteBrightnessChange = 0;	/* No Brightness Change */
		}

		if (video_object_layer_verid != 0x1 && video_object_layer_shape != RECTANGULAR)
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	sadct_disable */

		not_8_bit = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	not_8_bit */
		if (not_8_bit)
		{
			Temp = mpeg4p2par_GetUnsigned(8, ParserHandle);	/*	quant_precision, bits_per_pixel */
			PARSER_Data_p->VideoObjectLayer.QuantPrecision = ((Temp >> 4) & 0xF);
			bits_per_pixel = (Temp & 0xF);
		}
		else
		{
			PARSER_Data_p->VideoObjectLayer.QuantPrecision = 5;	/* default value */
			bits_per_pixel = 8;
		}

		if (video_object_layer_shape == GRAYSCALE)
			flush_bits = mpeg4p2par_GetUnsigned(3, ParserHandle);	/*	no_gray_quant_update, composition_method, linear_composition */

		quant_type = mpeg4p2par_GetUnsigned(1, ParserHandle);
		PARSER_Data_p->VideoObjectLayer.QuantType = quant_type;
		if (quant_type)
		{
			load_intra_quant_mat = mpeg4p2par_GetUnsigned(1, ParserHandle);
			if (load_intra_quant_mat)
			{
				U32 value;
				i = 0;
				do {
					value = mpeg4p2par_GetUnsigned(8, ParserHandle);
					i++;
				} while (i < 64 && (value != 0));
			}

			load_nonintra_quant_mat = mpeg4p2par_GetUnsigned(1, ParserHandle);
			if (load_nonintra_quant_mat)
			{
				U32 value;
				i = 0;
				do {
					value = mpeg4p2par_GetUnsigned(8, ParserHandle);
					i++;
				} while (i < 64 && (value != 0));
			}

			if (video_object_layer_shape == GRAYSCALE)
			{
				for(i = 0; i < PARSER_Data_p->VideoObjectLayer.AuxCompCount; i++)
				{
					load_intra_quant_mat_grayscale = mpeg4p2par_GetUnsigned(1, ParserHandle);
					if (load_intra_quant_mat_grayscale)
						flush_bits = mpeg4p2par_GetUnsigned(8, ParserHandle);
					load_nonintra_quant_mat_grayscale = mpeg4p2par_GetUnsigned(1, ParserHandle);
					if (load_nonintra_quant_mat_grayscale)
						flush_bits = mpeg4p2par_GetUnsigned(8, ParserHandle);
				}
			}
		}

		if (video_object_layer_verid != 0x1)
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	quarter_sample */

		complexity_estimation_disable = mpeg4p2par_GetUnsigned(1, ParserHandle);
		PARSER_Data_p->VideoObjectLayer.ComplexityEstimationDisable = complexity_estimation_disable;
		if (!complexity_estimation_disable)
			mpeg4p2par_ParseDefineVOPComplexityEstimationHeader(ParserHandle);

		Temp = mpeg4p2par_GetUnsigned(2, ParserHandle);	/*	resync_marker_disable, data_partitioned */
		PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable = (Temp >> 1);
		PARSER_Data_p->VideoObjectLayer.DataPartitioned = (Temp & 0x1);

		PARSER_Data_p->VideoObjectLayer.DataPartitioned = (Temp & 0x1);
		if (PARSER_Data_p->VideoObjectLayer.DataPartitioned)
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	reversible_vlc */

		if(video_object_layer_verid != 0x1)
		{
			newpred_enable = mpeg4p2par_GetUnsigned(1, ParserHandle);
			PARSER_Data_p->VideoObjectLayer.NewpredEnable = newpred_enable;

			if (newpred_enable)
				flush_bits = mpeg4p2par_GetUnsigned(3, ParserHandle);	/*	requested_upstream_message_type, newpred_segment_type */

			PARSER_Data_p->VideoObjectLayer.ReducedResolutionVopEnable = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	reduced_resolution_vop_enable */
		}
		else
		{
			PARSER_Data_p->VideoObjectLayer.ReducedResolutionVopEnable = 0;	/*	reduced_resolution_vop_enable */
			PARSER_Data_p->VideoObjectLayer.NewpredEnable = 0;	/* newpred_enable default value */
		}


		scalability = mpeg4p2par_GetUnsigned(1, ParserHandle);
		PARSER_Data_p->VideoObjectLayer.Scalability = scalability;
		if (scalability)
		{
			hierarchy_type = mpeg4p2par_GetUnsigned(1, ParserHandle);
			flush_bits = mpeg4p2par_GetUnsigned(25, ParserHandle);	/*	ref_layer_id, ref_layer_sampling_direc, hor_sampling_factor_n,
																					hor_sampling_factor_m, vert_sampling_factor_n, vert_sampling_factor_m */
			enhancement_type = mpeg4p2par_GetUnsigned(1, ParserHandle);
			PARSER_Data_p->VideoObjectLayer.EnhancementType = enhancement_type;

			if (video_object_layer_shape == BINARY && hierarchy_type == 0)
			{
				flush_bits = mpeg4p2par_GetUnsigned(22, ParserHandle);	/*	use_ref_shape, use_ref_texture, shape_hor_sampling_factor_n,
																		shape_hor_sampling_factor_m, shape_vert_sampling_factor_n, shape_vert_sampling_factor_m */
			}
		}
		else
			PARSER_Data_p->VideoObjectLayer.EnhancementType = 0;
	}
	else
	{
		if (video_object_layer_verid != 0x1)
		{
			scalability = mpeg4p2par_GetUnsigned(1, ParserHandle);
			PARSER_Data_p->VideoObjectLayer.Scalability = scalability;
			if (scalability)
			{
				flush_bits = mpeg4p2par_GetUnsigned(24, ParserHandle);	/* ref_layer_id, shape_hor_sampling_factor_n, shape_hor_sampling_factor_m,
																						shape_vert_sampling_factor_n, shape_vert_sampling_factor_m */
			}
		}

		PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	resync_marker_disable */
	}

	mpeg4p2par_NextStartCode(ParserHandle);	/* align the byte stream pointer */

	switch (aspect_ratio_info)
	{
		case 0:	/* should have been Forbidden, but streams are not coded properly */
			/* ASSERT */
			/* DumpError(PARSER_Data_p->PictureSpecificData.PictureError.VOLError.AspectRatio,
						 "mpeg4p2par_ParseVideoObjectLayerHeader: Forbidden Aspect Ratio\n"); */
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
			break;

		case 1:	/* 1:1 (Square) */
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_SQUARE;
			break;

		case 4:	/* 16:11 (625-type stretched for 16:9 picture) */
		case 5:	/* 40:33 (525-type stretched for 16:9 picture) */
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
			break;

		case 7:	/* extended PAR */
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_EXTENDED_PAR;
			break;

		case 2:	/* 12:11 (625-type for 4:3 picture) */
		case 3:	/* 10:11 (525-type for 4:3 picture) */
		default:
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
			break;

	}

	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType = (interlace == 1)? STVID_SCAN_TYPE_INTERLACED : STVID_SCAN_TYPE_PROGRESSIVE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate = PARSER_Data_p->VideoObjectLayer.Bitrate;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_14496_2;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay = PARSER_Data_p->VideoObjectLayer.LowDelay;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize = PARSER_Data_p->VideoObjectLayer.VBVBufferSize;
	if (PARSER_Data_p->VideoObjectLayer.FixedVOPRate)
	{
        if ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution == 30000) ||
			(PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution == 60000) ||
			(PARSER_Data_p->VideoObjectLayer.FixedVOPTimeIncrement == 0))               /* Recovery action to avoid zero divide */
		{
			PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / 1001);
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / 1001);
		}
		else /*if (PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution == 15)*/
		{
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / PARSER_Data_p->VideoObjectLayer.FixedVOPTimeIncrement);
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / PARSER_Data_p->VideoObjectLayer.FixedVOPTimeIncrement);
		}
    }

	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  (interlace == 1)? STGXOBJ_INTERLACED_SCAN : STGXOBJ_PROGRESSIVE_SCAN;

#if defined(TRACE_UART)
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "ObjectLayerIdentifier: %d \n", is_object_layer_identifier);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VideoObjectLayerVerId: %x \n", video_object_layer_verid);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VideoObjectLayerPriority: %d \n", video_object_layer_priority);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "AspectRatioInfo: %x \n", aspect_ratio_info);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "PARWidth: %d \n", PARSER_Data_p->VideoObjectLayer.PARWidth);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "PARHeight: %d \n", PARSER_Data_p->VideoObjectLayer.PARHeight);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VolControlParameters: %d \n", vol_control_parameters);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "ChromaFormat: %x \n", chroma_format);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "LowDelay: %d \n", PARSER_Data_p->VideoObjectLayer.LowDelay);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VbvParameters: %d \n", vbv_parameters);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "Bitrate: %d \n", PARSER_Data_p->VideoObjectLayer.Bitrate);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "BufferSize: %d \n", PARSER_Data_p->VideoObjectLayer.VBVBufferSize);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VideoObjectLayerShape: %x \n", video_object_layer_shape);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VideoObjectLayerShapeExtension: %x \n", video_object_layer_shape_extension);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VOPTimeIncrResolution: %d \n", vop_time_increment_resolution);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "FixedVOPRate: %d \n", fixed_vop_rate);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VOLWidth: %d \n", video_object_layer_width);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "VOLHeight: %d \n", video_object_layer_height);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "Interlace: %d \n", interlace);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "Not8Bit: %d \n", not_8_bit);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "QuantType: %d \n", quant_type);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "ComplexityEstimation: %d \n", complexity_estimation_disable);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "ResyncMarkerDisable: %d \n", PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "NewpredEnable: %d \n", newpred_enable);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "ReducedResolutionVOPEnable: %d \n", PARSER_Data_p->VideoObjectLayer.ReducedResolutionVopEnable);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "Scalability: %d \n", scalability);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOL, "EnhancementType: %d \n", PARSER_Data_p->VideoObjectLayer.EnhancementType);
#endif

	return 0;
}

/******************************************************************************/
/*! \brief Parses a Group of Video Object Plane Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseGroupOfVOPHeader (const PARSER_Handle_t  ParserHandle)
{
	U32 time_code;
	U8 closed_gov;
	U8 broken_link;
	U32 i;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	time_code = mpeg4p2par_GetUnsigned(18, ParserHandle);
	closed_gov = mpeg4p2par_GetUnsigned(1, ParserHandle);
	broken_link = mpeg4p2par_GetUnsigned(1, ParserHandle);

	/* If closed_gov set to 1, we do not need any of our reference frames any more, so
	 * mark them as no longer valid.  Also mark the 3 reference lists as empty*/
	if(closed_gov == 1)
	{
		for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
		{
			PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] = FALSE;
			PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = FALSE;
		}

		/* The first reference picture will be placed at index zero in the FullReferenceFrameList */
		PARSER_Data_p->StreamState.NextReferencePictureIndex = 0;

		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;

	}
	/* If closed_gov=0 and broken_link=1, that means there are missing reference frame.
	 * In this case, all current reference pictures must be marked with the broken link flag */
	else if(PARSER_Data_p->PrivatePictureContextData.BrokenLinkFlag == 1)
	{
		for(i = 0; i < VIDCOM_MAX_NUMBER_OF_REFERENCES_FRAMES; i++)
		{
			/* mark all valid reference pictures with the broken link flag */
			if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[i] == TRUE)
				PARSER_Data_p->PictureGenericData.IsBrokenLinkIndexInReferenceFrame[i] = TRUE;
		}
	}


	/* Check the next bits for user_data_start_code for the user_data() */
}

/******************************************************************************/
/*! \brief Parses a Video Plane With Short Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseVideoPlaneWithShortHeader (const PARSER_Handle_t  ParserHandle)
{
	U8 source_format;
	U8 picture_coding_type;
	U8 vop_quant;
	U32 vop_width;
	U32 vop_height;
	U32 num_macroblocks_in_gob;
	U32 num_gobs_in_vop;
	U8 obmc_disable;
	U8 block_count;
	U8 reversible_vlc;
	U8 use_intra_dc_vlc;
	U8 not_8_bit;
	U8 bits_per_pixel;
	U32 RefPicIndex0;
	U32 RefPicIndex1;
	U8 pei;
	U32 Temp;
	U32 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);


	Temp = mpeg4p2par_GetUnsigned(27, ParserHandle);	/* temporal_reference, marker bit, zero_bit, split_screen_indicator,
																			document_camera_indicator, full_picture_freeze_release,
																			source_format, picture_coding_type, four_reserved_zero_bits,
																			vop_quant, zero_bit */

	source_format = ((Temp >> 11) & 0x7);
	picture_coding_type = ((Temp >> 10) & 0x1);
	vop_quant = ((Temp >> 1) & 0x1F);

	switch (source_format)
	{
	case 1:	/* sub-QCIF */
		vop_width = 128;
		vop_height = 96;
		num_macroblocks_in_gob = 8;
		num_gobs_in_vop = 6;
		break;
	case 2:	/* QCIF */
		vop_width = 176;
		vop_height = 144;
		num_macroblocks_in_gob = 11;
		num_gobs_in_vop = 9;
		break;
	case 3:	/* CIF */
		vop_width = 352;
		vop_height = 288;
		num_macroblocks_in_gob = 22;
		num_gobs_in_vop = 18;
		break;
	case 4:	/* 4CIF */
		vop_width = 704;
		vop_height = 576;
		num_macroblocks_in_gob = 88;
		num_gobs_in_vop = 18;
		break;
	case 5:	/* 16CIF */
		vop_width = 1408;
		vop_height = 1152;
		num_macroblocks_in_gob = 352;
		num_gobs_in_vop = 18;
		break;
	default:
		return ; /* no valid picture format found */
	}

	PARSER_Data_p->VideoObjectPlane.VOPWidth = vop_width;
	PARSER_Data_p->VideoObjectPlane.VOPHeight = vop_height;
	PARSER_Data_p->VideoObjectPlane.VopCodingType = (picture_coding_type == 0)? I_VOP : P_VOP;
	PARSER_Data_p->VideoObjectPlane.NumMacroblocksInGOB = num_gobs_in_vop;

	do {
		pei = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (pei == 1)
			flush_bits = mpeg4p2par_GetUnsigned(8, ParserHandle); /* psupp */
	}while (pei == 1);

	PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape = RECTANGULAR;
	obmc_disable = 1;
	PARSER_Data_p->VideoObjectLayer.QuantType = 0;
	PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable = 1;
	PARSER_Data_p->VideoObjectLayer.DataPartitioned = 0;
	block_count = 6;
	reversible_vlc = 0;
	PARSER_Data_p->VideoObjectPlane.VopRoundingType = 0;
	PARSER_Data_p->VideoObjectPlane.VopFcodeForward = 1;
	PARSER_Data_p->VideoObjectPlane.VopCoded = 1;
	PARSER_Data_p->VideoObjectLayer.Interlace = 0;
	PARSER_Data_p->VideoObjectLayer.ComplexityEstimationDisable = 1;
	use_intra_dc_vlc = 0;
	PARSER_Data_p->VideoObjectLayer.Scalability = 0;
	not_8_bit = 0;
	bits_per_pixel = 8;

	PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries = 1;
	PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = 1;
	PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients = 6;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate = PARSER_Data_p->VideoObjectLayer.Bitrate;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_14496_2;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = ((30000 * 1000) / 1001);
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = ((30000 * 1000) / 1001);

    /* Forward Reference frame/field for B fields/frames */
	RefPicIndex0 = PARSER_Data_p->StreamState.NextReferencePictureIndex;
	/* Backward Reference frame for B fields or frames and Reference frame for P fields/frames */
	RefPicIndex1 = PARSER_Data_p->StreamState.NextReferencePictureIndex ^ 1;

	/* Update the picture parameters */
	mpeg4p2par_UpdatePictureParams(ParserHandle);

	/* The video plane with short header is a valid picture */
	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;
}

/******************************************************************************/
/*! \brief Parses a Video Object Plane Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
U32 mpeg4p2par_ParseVideoObjectPlaneHeader (U8 *StartCodeValueAddress, U8 *NextStartCodeValueAddress, const PARSER_Handle_t ParserHandle)
{
	U8 vop_coding_type;
	U8 modulo_time_base;
	U16 vop_time_increment;
	U8 vop_coded;
	U16 vop_id;
	U8 vop_id_for_prediction;
	U8 vop_rounding_type;
	/* U8 vop_reduced_resolution; */
	U16 vop_width;
	U16 vop_height;
	U8 vop_constant_alpha;
	U8 top_field_first;
	/* U8 sprite_transmit_mode; */
	U8 vop_fcode_forward = 0;
	U8 vop_fcode_backward;
	/* U8 vop_shape_coding_type; */
	U32 flush_bits;
	U8 vop_id_for_prediction_indication;
	U32 vop_quant;
	U32 Temp;
	U32 NoOfBits;
	/* BOOL ReferenceVOP; */ /* TRUE if this is a reference picture, false otherwise; */
	U32 BrightnessChangeFactor;
	BOOL VOPSynchronize = FALSE;
	U32 Error = 0;
	STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;	/* mapping for converting from the Device to CPU address */
	U8 * BitBufferStartAddr_p;	/* noncached CPU address to dump the memory contents of the bit-buffer */
	U8 * BitBufferStopAddr_p;	/* noncached CPU address to dump the memory contents of the bit-buffer */


	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

	/* Default: this VOP is not valid */
	PARSER_Data_p->PictureLocalData.IsValidPicture = FALSE;

	/* Update position in bit buffer */
	/* The picture start address used through the CODEC API is the position of the beginning of the start code */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) mpeg4p2par_MoveBackInStream(StartCodeValueAddress, 3, ParserHandle);
	/* The stop address is the byte preceeding the next start code */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = (void *) mpeg4p2par_MoveBackInStream(NextStartCodeValueAddress, (3 + 1), ParserHandle);
	BitBufferStartAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress, &VirtualMapping);
	BitBufferStopAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress, &VirtualMapping);

	vop_coding_type = mpeg4p2par_GetUnsigned(2, ParserHandle);
	PARSER_Data_p->VideoObjectPlane.VopCodingType = vop_coding_type;

	do {
		modulo_time_base = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (modulo_time_base)
		{
			VOPSynchronize = TRUE;
		}
	}while(modulo_time_base != 0);

	flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* marker_bit */
	NoOfBits = CountNumberOfBits(PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution);
	NoOfBits =  (NoOfBits < 1)? 1 : NoOfBits;
	Temp = mpeg4p2par_GetUnsigned((NoOfBits + 2), ParserHandle);	/* vop_time_increment, marker_bit, vop_coded */
	vop_time_increment = (Temp >> 2);
	PARSER_Data_p->VideoObjectPlane.VopTimeIncrement = vop_time_increment;

	vop_coded = (Temp & 0x1);
	PARSER_Data_p->VideoObjectPlane.VopCoded = vop_coded;
	if (PARSER_Data_p->VideoObjectPlane.VopCoded == 0)	/* no further VOP data exixts for this VOP */
	{
		mpeg4p2par_NextStartCode(ParserHandle);	/* align the byte stream pointer */

		/* update the parameters */
		mpeg4p2par_UpdatePictureParams(ParserHandle);

		/*DumpError(PARSER_Data_p->PictureSpecificData.PictureError.VOPError.CodedVOP,
						"mpeg4p2par_ParseVideoObjectPlaneHeader: vop_coded = 0\n");*/

		/* The VOP is NOT the valid picture */
		PARSER_Data_p->PictureLocalData.IsValidPicture = FALSE;

		return 0;
	}

	/* Calculate the framerate from vop_time_increment, vop_time_increment_resolution and synchronize points						*/
	/* If it is a reference frame the time difference is calculated between two previous reference frames or between			*/
	/* previous reference frame and previous B frame depending upon if the previous frame was a reference frame or not		*/
	/* If it is a B frame the time diff is calculated between the current B frame and the previous reference frame or			*/
	/* previous B frame depending upon if the previous frame was reference frame or not													*/
	/* At the VOP synchronisation time when the VOPTimeIncrementResolution value is reached, the VOPTimeIncrementResolution	*/
	/* is added to the current VOP time to calculate the difference																			*/
	PARSER_Data_p->VideoObjectPlane.CurrVOPTime = vop_time_increment;
	/* STTBX_Report((STTBX_REPORT_LEVEL_INFO, "VOPIncr: %d\n", vop_time_increment)); */

	if (PARSER_Data_p->VideoObjectLayer.FixedVOPRate == 0)
	{
		if (vop_coding_type == I_VOP || vop_coding_type == P_VOP)
		{
			if (PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime == 0)
			{
				if (PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution == 25 || PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution == 30)
					PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = 1;
				else
					PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = 1001;

				PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime;
				PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime = PARSER_Data_p->VideoObjectPlane.CurrVOPTime;
				PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef = TRUE;
			}
			else
			{
				if (PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef)
				{
					if (PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime < PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime)
						PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = (PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution + PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime) - PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime;
					else if (PARSER_Data_p->VideoObjectPlane.CurrVOPTime == PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime)
					{
						PARSER_Data_p->VideoObjectPlane.CurrVOPTime = PARSER_Data_p->VideoObjectPlane.CurrVOPTime + PARSER_Data_p->VideoObjectPlane.PrevVOPTimeDiff;
						PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime - PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime;
					}
					else
						PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime - PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime;
					PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime;
					PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime = PARSER_Data_p->VideoObjectPlane.CurrVOPTime;
					PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef = TRUE;
				}
				else
				{
					if (PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime < PARSER_Data_p->VideoObjectPlane.PrevVOPTime)
						PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = (PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution + PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime) - PARSER_Data_p->VideoObjectPlane.PrevVOPTime;
					else
						PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime - PARSER_Data_p->VideoObjectPlane.PrevVOPTime;
					PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime;
					/*PARSER_Data_p->VideoObjectPlane.PrevVOPTime = PARSER_Data_p->VideoObjectPlane.CurrRefVOPTIme;*/
					PARSER_Data_p->VideoObjectPlane.CurrRefVOPTime = PARSER_Data_p->VideoObjectPlane.CurrVOPTime;
					PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef = TRUE;
				}
			}
		}
		else if (vop_coding_type == B_VOP)
		{
			if (PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef == TRUE)
			{
				if (PARSER_Data_p->VideoObjectPlane.CurrVOPTime < PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime)
					PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = (PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution + PARSER_Data_p->VideoObjectPlane.CurrVOPTime) - PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime;
				else
					PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = PARSER_Data_p->VideoObjectPlane.CurrVOPTime - PARSER_Data_p->VideoObjectPlane.PrevRefVOPTime;
			}
			else
			{
				if (PARSER_Data_p->VideoObjectPlane.CurrVOPTime < PARSER_Data_p->VideoObjectPlane.PrevVOPTime)
					PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = (PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution + PARSER_Data_p->VideoObjectPlane.CurrVOPTime) - PARSER_Data_p->VideoObjectPlane.PrevVOPTime;
				else
					PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = PARSER_Data_p->VideoObjectPlane.CurrVOPTime - PARSER_Data_p->VideoObjectPlane.PrevVOPTime;
			}

			PARSER_Data_p->VideoObjectPlane.PrevVOPTime = PARSER_Data_p->VideoObjectPlane.CurrVOPTime;
			PARSER_Data_p->VideoObjectPlane.PrevVOPWasRef = FALSE;
		}

        if (PARSER_Data_p->VideoObjectPlane.VOPTimeDiff == 0)
        {
		   PARSER_Data_p->VideoObjectPlane.VOPTimeDiff = PARSER_Data_p->VideoObjectPlane.PrevVOPTimeDiff;
        }
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / PARSER_Data_p->VideoObjectPlane.VOPTimeDiff);
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / PARSER_Data_p->VideoObjectPlane.VOPTimeDiff);
		PARSER_Data_p->VideoObjectPlane.PrevVOPTimeDiff = PARSER_Data_p->VideoObjectPlane.VOPTimeDiff;
	}
	PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = FALSE;

	if (PARSER_Data_p->VideoObjectLayer.NewpredEnable)
	{
		NoOfBits = ((vop_time_increment + 3) > 15 )? 15 : (vop_time_increment + 3);
		vop_id = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);
		vop_id_for_prediction_indication = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (vop_id_for_prediction_indication)
		{
			Temp = mpeg4p2par_GetUnsigned((NoOfBits+1), ParserHandle);	/* vop_id_for_prediction, marker_bit */
			vop_id_for_prediction = (Temp >> 1);
		}
	}

	if ((PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == BINARY_ONLY) && (vop_coding_type == P_VOP ||
		(vop_coding_type == S_VOP && PARSER_Data_p->VideoObjectLayer.SpriteEnable == GMC_SPRITE)))
	{
		vop_rounding_type = mpeg4p2par_GetUnsigned(1, ParserHandle);
		PARSER_Data_p->VideoObjectPlane.VopRoundingType = vop_rounding_type;
	}
	else
		PARSER_Data_p->VideoObjectPlane.VopRoundingType = vop_rounding_type = 0;

	if ((PARSER_Data_p->VideoObjectLayer.ReducedResolutionVopEnable) && (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == RECTANGULAR) &&
		(vop_coding_type == P_VOP || vop_coding_type == I_VOP))
		PARSER_Data_p->VideoObjectPlane.VopReducedResolution = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* vop_reduced_resolution */
	else
		PARSER_Data_p->VideoObjectPlane.VopReducedResolution = 0;


	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != RECTANGULAR)
	{
		if (!(PARSER_Data_p->VideoObjectLayer.SpriteEnable == STATIC_SPRITE && vop_coding_type == I_VOP))
		{
		Temp = mpeg4p2par_GetUnsigned(28, ParserHandle);
		vop_width = ((Temp >> 15) & 0x1FFF);
		vop_height = ((Temp >> 1) & 0x1FFF);
		PARSER_Data_p->VideoObjectPlane.VOPWidth = vop_width;
		PARSER_Data_p->VideoObjectPlane.VOPHeight = vop_height;

		flush_bits = mpeg4p2par_GetUnsigned(28, ParserHandle);	/* vop_horizontal_mc_spatial_ref, marker_bit,
																				vop_vertical_mc_spatial_ref, marker_bit */
		}

		if ((PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == BINARY_ONLY) &&
			PARSER_Data_p->VideoObjectLayer.Scalability && PARSER_Data_p->VideoObjectLayer.EnhancementType)
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* background_composition */

		Temp = mpeg4p2par_GetUnsigned(2, ParserHandle);	/* change_conv_ratio_disable, vop_constant_alpha */
		vop_constant_alpha = Temp >> 1;
		if (vop_constant_alpha)
			flush_bits = mpeg4p2par_GetUnsigned(8, ParserHandle);	/* vop_constant_alpha_value */
	}

	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != BINARY_ONLY)
		if (!PARSER_Data_p->VideoObjectLayer.ComplexityEstimationDisable)
			mpeg4p2par_ParseReadVOPComplexityEstimationHeader(ParserHandle);

	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != BINARY_ONLY)
	{
		flush_bits = mpeg4p2par_GetUnsigned(3, ParserHandle);	/* intra_dc_vlc_thr */
        if (PARSER_Data_p->VideoObjectLayer.Interlace)
		{
			Temp = mpeg4p2par_GetUnsigned(2, ParserHandle);	/* top_field_first, alternate_vertical_scan_flag */
			top_field_first =  (Temp >> 1);
			PARSER_Data_p->VideoObjectPlane.TopFieldFirst = top_field_first;

#ifdef WA_GNBvd57772
            /* Workaround for GNBvd57772: BFF MPEG4P2 streams are badly displayed 7100/7109 (field inversions) */
            /* Force TopFieldFirst in all cases for interlaced frames */
            if (PARSER_Data_p->VideoObjectPlane.TopFieldFirst == 0)
            {
                PARSER_Data_p->VideoObjectPlane.TopFieldFirst = 1;
            }
#endif /* WA_GNBvd57772 */

			/* probably the firmware arranges the fields with first field at the start of the buffer */
		}
		else	/* Progressive */
            PARSER_Data_p->VideoObjectPlane.TopFieldFirst = top_field_first = 0;
    }

	if ((PARSER_Data_p->VideoObjectLayer.SpriteEnable == STATIC_SPRITE || PARSER_Data_p->VideoObjectLayer.SpriteEnable == GMC_SPRITE) &&
		vop_coding_type == S_VOP)
	{
		if (PARSER_Data_p->VideoObjectLayer.SpriteWarpingPoints > 0)
		{
			Error = mpeg4p2par_ParseSpriteTrajectory(ParserHandle);
			/*if (Error != 0)*/
		}

		if (PARSER_Data_p->VideoObjectLayer.SpriteBrightnessChange)
			BrightnessChangeFactor = mpeg4p2par_ParseBrightnessChangeFactor(ParserHandle);

		if (PARSER_Data_p->VideoObjectLayer.SpriteEnable == STATIC_SPRITE)
		{
			return 1;
		}

	}

	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != BINARY_ONLY)
	{
		U32 i;

		vop_quant = mpeg4p2par_GetUnsigned(PARSER_Data_p->VideoObjectLayer.QuantPrecision, ParserHandle);

		if(PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == GRAYSCALE)
			for (i = 0; i < PARSER_Data_p->VideoObjectLayer.AuxCompCount; i++)
				flush_bits = mpeg4p2par_GetUnsigned(6, ParserHandle);

		if (vop_coding_type == I_VOP)
		{
			vop_fcode_forward = mpeg4p2par_GetUnsigned(3, ParserHandle);
			PARSER_Data_p->VideoObjectPlane.VopFcodeForward = vop_fcode_forward;
		}

		if (vop_coding_type == B_VOP)
		{
			vop_fcode_backward = mpeg4p2par_GetUnsigned(3, ParserHandle);
			PARSER_Data_p->VideoObjectPlane.VopFcodeBackward = vop_fcode_backward;
		}

		if (!PARSER_Data_p->VideoObjectLayer.Scalability)
		{
			if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != RECTANGULAR && vop_coding_type != I_VOP)
				flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* vop_shape_coding_type */
		}


	}

	if (PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable == 0)
	{
		if (vop_coding_type == I_VOP || PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == BINARY_ONLY)
			PARSER_Data_p->VideoObjectPlane.ResyncMarker = 17;	/* resync_marker */

		if (vop_coding_type == P_VOP || vop_coding_type == S_VOP)
			PARSER_Data_p->VideoObjectPlane.ResyncMarker = (15 + vop_fcode_forward) + 1;	/* resync_marker */

		if (vop_coding_type == B_VOP)
			PARSER_Data_p->VideoObjectPlane.ResyncMarker = (((15 + vop_fcode_forward)> 17)?(15 + vop_fcode_forward):17) + 1;	/* resync_marker */
	}



	/* update the parameters */
	mpeg4p2par_UpdatePictureParams(ParserHandle);

	/* The VOP is the valid picture */
	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

#if defined(TRACE_UART)
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "BBStartAddress: %x \n", PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "BBStopAddress: %x \n", PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPCodingType: %d \n", vop_coding_type);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPTimeIncrement: %d \n", vop_time_increment);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPCoded: %d \n", vop_coded);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "FrameRate: %d \n", PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPIdForPredictionIndication: %d \n", vop_id_for_prediction_indication);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPRoundingType: %x \n", vop_rounding_type);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPReducedResolution: %d \n", PARSER_Data_p->VideoObjectPlane.VopReducedResolution);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPWidth: %d \n", vop_width);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "VOPHeight: %d \n", vop_height);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "TopFieldFirst: %d \n", top_field_first);
	TraceBufferColorConditional(MPEG4P2PAR_TRACE_BLOCKID, MPEG4P2PAR_TRACE_PARSEVOP, "ResyncMarker: %x \n", PARSER_Data_p->VideoObjectPlane.ResyncMarker);
#endif

	return 0;
}

/******************************************************************************/
/*! \brief Parses a DivX 311 frame and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseDivx311Header (const PARSER_Handle_t ParserHandle)
{
	U8 value[4];
	U32 DwScale, DwRate, FrameRate, Width, Height, Bitrate;
	U32 Temp;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	Temp = mpeg4p2par_GetUnsigned(32, ParserHandle);
	value[0] = Temp >> 24;
	value[1] = Temp >> 16;
	value[2] = Temp >> 8;
	value[3] = Temp;
	DwScale = value[0] + (value[1]* 256) + (value[2]*256*256) + (value[3]*256*256*256);

	Temp = mpeg4p2par_GetUnsigned(32, ParserHandle);
	value[0] = Temp >> 24;
	value[1] = Temp >> 16;
	value[2] = Temp >> 8;
	value[3] = Temp;
	DwRate = value[0] + (value[1]* 256) + (value[2]*256*256) + (value[3]*256*256*256);
    FrameRate = ((DwRate * 1000) / DwScale);
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = FrameRate;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = FrameRate;

	Temp = mpeg4p2par_GetUnsigned(32, ParserHandle);
	value[0] = Temp >> 24;
	value[1] = Temp >> 16;
	value[2] = Temp >> 8;
	value[3] = Temp;
	Bitrate = value[0] + (value[1]* 256) + (value[2]*256*256) + (value[3]*256*256*256);
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate = Bitrate;

	Temp = mpeg4p2par_GetUnsigned(32, ParserHandle);
	value[0] = Temp >> 24;
	value[1] = Temp >> 16;
	value[2] = Temp >> 8;
	value[3] = Temp;
	Width = value[0] + (value[1]* 256) + (value[2]*256*256) + (value[3]*256*256*256);
	Temp = mpeg4p2par_GetUnsigned(32, ParserHandle);
	value[0] = Temp >> 24;
	value[1] = Temp >> 16;
	value[2] = Temp >> 8;
	value[3] = Temp;
	Height = value[0] + (value[1]* 256) + (value[2]*256*256) + (value[3]*256*256*256);

	PARSER_Data_p->VideoObjectPlane.VOPWidth = Width;
	PARSER_Data_p->VideoObjectPlane.VOPHeight = Height;
}

/******************************************************************************/
/*! \brief Parses a DivX 311 frame and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseDivx311Frame (U8 *StartCodeValueAddress, U8 *NextStartCodeValueAddress, const PARSER_Handle_t ParserHandle)
{
	U8 vop_coding_type;
	STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping;	/* mapping for converting from the Device to CPU address */
	U8 * BitBufferStartAddr_p;	/* noncached CPU address to dump the memory contents of the bit-buffer */
	U8 * BitBufferStopAddr_p;	/* noncached CPU address to dump the memory contents of the bit-buffer */
	/* U32 DwScale, DwRate, FrameRate; */

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
	STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

	/* Default: this VOP is not valid */
	PARSER_Data_p->PictureLocalData.IsValidPicture = FALSE;

/*
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ProfileAndLevelIndication = 0xF4;
	PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries								= 1;
	PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics						= 1;
	PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients							= 1;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VideoFormat					= 1;
*/

	/* Update position in bit buffer */
	/* The picture start address used through the CODEC API is the position of the beginning of the start code */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) mpeg4p2par_MoveForwardInStream(StartCodeValueAddress, 1, ParserHandle);
	/* The stop address is the byte preceeding the next start code */
	PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = (void *) mpeg4p2par_MoveBackInStream(NextStartCodeValueAddress, (3 + 1), ParserHandle);

	BitBufferStartAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress, &VirtualMapping);
	BitBufferStopAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress, &VirtualMapping);

	vop_coding_type = mpeg4p2par_GetUnsigned(2, ParserHandle);
	PARSER_Data_p->VideoObjectPlane.VopCodingType = vop_coding_type;
	PARSER_Data_p->VideoObjectPlane.VopCoded = 1;

    PARSER_Data_p->PictureGenericData.IsExtendedPresentationOrderIDValid = FALSE;

	PARSER_Data_p->PictureSpecificData.CodecVersion = 311;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType = STVID_SCAN_TYPE_PROGRESSIVE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard = STVID_MPEG_STANDARD_ISO_IEC_14496_2;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay = TRUE;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize = MPEG4P2_DEFAULT_VBVMAX;
	PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  STGXOBJ_PROGRESSIVE_SCAN;

	PARSER_Data_p->VideoObjectLayer.Interlace = 0;

	/* update the parameters */
	mpeg4p2par_UpdatePictureParams(ParserHandle);

	/* The VOP is the valid picture */
	PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

}

/******************************************************************************/
/*! \brief Parses a User Data Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseUserDataHeader (const PARSER_Handle_t  ParserHandle)
{
	char UserDataBuffer[64];
	U8 *StrPointer = NULL;
	U32 BufferPos = 0;
#ifdef DIVX_CODEC
	U32 userdata_build_number = 0;
#endif /* DIVX_CODEC */
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	memset(UserDataBuffer, 0, 64);

	while (mpeg4p2par_ReadUnsigned(24, ParserHandle) != 0x000001)
	{
		UserDataBuffer[BufferPos] = mpeg4p2par_GetUnsigned(8, ParserHandle);	/*	user_data */
		BufferPos++;
		BufferPos = (BufferPos == 64)? 0 : BufferPos;
	}

	StrPointer = (U8*)strstr(UserDataBuffer, "XviD");
	if (StrPointer)
	{
		PARSER_Data_p->PictureSpecificData.CodecVersion = 500;
	}
	else
	{
#ifdef DIVX_CODEC

    	StrPointer = (U8*)strstr(UserDataBuffer, "DivX");
    	if (StrPointer)
    	{
    		PARSER_Data_p->PictureSpecificData.CodecVersion = atoi((char *)(&StrPointer[4]));
         }
    	StrPointer = (U8*)strstr(UserDataBuffer, "Build");
    	if (StrPointer)
    	{
    		userdata_build_number = atoi((char *)(&StrPointer[5]));
    	}
    	else
    	{
        	StrPointer = (U8*)strstr(UserDataBuffer, "b");
        	if (StrPointer)
        	{
        		userdata_build_number = atoi((char *)(&StrPointer[1]));
        	}
    	}
#endif /* DIVX_CODEC */
    }
    return;
}

/******************************************************************************/
/*! \brief Parses a Define VOP Complexity Estimation Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseDefineVOPComplexityEstimationHeader (const PARSER_Handle_t  ParserHandle)
{
	U8 shape_complexity_estimation_disable;
	U8 texture_complexity_estimation_set_1_disable;
	U8 texture_complexity_estimation_set_2_disable;
	U8 motion_compensation_complexity_disable;
	U8 version2_complexity_estimation_disable;
	/* U32 flush_bits; */
	U32 Temp;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->VOPComplexityEstimation.Estimation_Method = mpeg4p2par_GetUnsigned(2, ParserHandle);
	if (PARSER_Data_p->VOPComplexityEstimation.Estimation_Method == 0 ||
		PARSER_Data_p->VOPComplexityEstimation.Estimation_Method == 1)
	{
		shape_complexity_estimation_disable = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (!shape_complexity_estimation_disable)
		{
			Temp = mpeg4p2par_GetUnsigned(6, ParserHandle);	/* opaque, transparent, intra_cae, inter_cae, no_update, upsampling */
			PARSER_Data_p->VOPComplexityEstimation.Opaque = ((Temp >> 5) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Transparent = ((Temp >> 4) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Intra_Cae = ((Temp >> 3) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Inter_Cae = ((Temp >> 2) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.No_Update = ((Temp >> 1) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Upsampling = (Temp & 0x1);
		}

		texture_complexity_estimation_set_1_disable = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (!texture_complexity_estimation_set_1_disable)
		{
			Temp = mpeg4p2par_GetUnsigned(4, ParserHandle);	/* intra_blocks, inter_blocks, inter4v_blocks, not_coded_blocks */
			PARSER_Data_p->VOPComplexityEstimation.Intra_Blocks = ((Temp >> 3) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Inter_Blocks = ((Temp >> 2) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Inter4v_Blocks = ((Temp >> 1) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Not_Coded_Blocks = (Temp & 0x1);
		}

		texture_complexity_estimation_set_2_disable = (mpeg4p2par_GetUnsigned(2, ParserHandle) & 0x1);
		if (!texture_complexity_estimation_set_2_disable)
		{
			Temp = mpeg4p2par_GetUnsigned(4, ParserHandle);	/* dct_coefs, dct_lines, vlc_symbols, vlc_bits */
			PARSER_Data_p->VOPComplexityEstimation.Dct_Coefs = ((Temp >> 3) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Dct_Lines = ((Temp >> 2) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Vlc_Symbols = ((Temp >> 1) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Vlc_bits = (Temp & 0x1);
		}

		motion_compensation_complexity_disable = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (!motion_compensation_complexity_disable)
		{
			Temp = mpeg4p2par_GetUnsigned(6, ParserHandle);	/* apm, npm, interpolate_mc_q, forw_back_mc_q, halfpel2, halfpel4 */
			PARSER_Data_p->VOPComplexityEstimation.Apm = ((Temp >> 5) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Npm = ((Temp >> 4) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Interpolate_Mc_Q = ((Temp >> 3) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Forw_Back_Mc_Q = ((Temp >> 2) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Halfpel2 = ((Temp >> 1) & 0x1);
			PARSER_Data_p->VOPComplexityEstimation.Halfpel4 = (Temp & 0x1);
		}

		if (PARSER_Data_p->VOPComplexityEstimation.Estimation_Method == 1)
		{
			version2_complexity_estimation_disable = (mpeg4p2par_GetUnsigned(2, ParserHandle) & 0x1);
			if (!version2_complexity_estimation_disable)
			{
				Temp = mpeg4p2par_GetUnsigned(2, ParserHandle);	/* sadct, quarterpel */
				PARSER_Data_p->VOPComplexityEstimation.Sadct = ((Temp >> 1) & 0x1);
				PARSER_Data_p->VOPComplexityEstimation.Quarterpel = (Temp & 0x1);
			}
		}
	}
}

/******************************************************************************/
/*! \brief Parses a Read VOP Complexity Estimation Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseReadVOPComplexityEstimationHeader (const PARSER_Handle_t  ParserHandle)
{
	U32 NoOfBits;
	U32 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (PARSER_Data_p->VOPComplexityEstimation.Estimation_Method == 0)
	{
		if (PARSER_Data_p->VideoObjectPlane.VopCodingType == I_VOP)
		{
			NoOfBits =  (((PARSER_Data_p->VOPComplexityEstimation.Opaque) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Transparent) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Intra_Cae) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Inter_Cae) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.No_Update) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Upsampling) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Intra_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Not_Coded_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Dct_Coefs) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Dct_Lines) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Vlc_Symbols) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Vlc_bits) * 4) +
									((PARSER_Data_p->VOPComplexityEstimation.Sadct) * 8));

			while (NoOfBits > 32)
			{
				flush_bits = mpeg4p2par_GetUnsigned(32, ParserHandle);
				NoOfBits -= 32;
			}
			flush_bits = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);
		}

		if (PARSER_Data_p->VideoObjectPlane.VopCodingType == P_VOP)
		{
			NoOfBits =  (((PARSER_Data_p->VOPComplexityEstimation.Opaque) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Transparent) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Intra_Cae) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Inter_Cae) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.No_Update) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Upsampling) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Intra_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Not_Coded_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Dct_Coefs) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Dct_Lines) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Vlc_Symbols) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Vlc_bits) * 4) +
									((PARSER_Data_p->VOPComplexityEstimation.Inter_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Inter4v_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Apm) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Npm) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Forw_Back_Mc_Q) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Halfpel2) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Halfpel4) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Sadct) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Quarterpel) * 8));

			while (NoOfBits > 32)
			{
				flush_bits = mpeg4p2par_GetUnsigned(32, ParserHandle);
				NoOfBits -= 32;
			}
			flush_bits = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);
		}

		if (PARSER_Data_p->VideoObjectPlane.VopCodingType == B_VOP)
		{
			NoOfBits =  (((PARSER_Data_p->VOPComplexityEstimation.Opaque) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Transparent) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Intra_Cae) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Inter_Cae) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.No_Update) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Upsampling) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Intra_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Not_Coded_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Dct_Coefs) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Dct_Lines) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Vlc_Symbols) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Vlc_bits) * 4) +
									((PARSER_Data_p->VOPComplexityEstimation.Inter_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Inter4v_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Apm) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Npm) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Forw_Back_Mc_Q) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Halfpel2) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Halfpel4) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Interpolate_Mc_Q) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Sadct) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Quarterpel) * 8));

			while (NoOfBits > 32)
			{
				flush_bits = mpeg4p2par_GetUnsigned(32, ParserHandle);
				NoOfBits -= 32;
			}
			flush_bits = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);
		}

		if (PARSER_Data_p->VideoObjectPlane.VopCodingType == S_VOP && PARSER_Data_p->VideoObjectLayer.SpriteEnable == STATIC_SPRITE)
		{
			NoOfBits =  (((PARSER_Data_p->VOPComplexityEstimation.Intra_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Not_Coded_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Dct_Coefs) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Dct_Lines) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Vlc_Symbols) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Vlc_bits) * 4) +
									((PARSER_Data_p->VOPComplexityEstimation.Inter_Blocks) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Inter4v_Blocks) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Apm) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Npm) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Forw_Back_Mc_Q) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Halfpel2) * 8) +
									((PARSER_Data_p->VOPComplexityEstimation.Halfpel4) * 8) + ((PARSER_Data_p->VOPComplexityEstimation.Interpolate_Mc_Q) * 8));

			while (NoOfBits > 32)
			{
				flush_bits = mpeg4p2par_GetUnsigned(32, ParserHandle);
				NoOfBits -= 32;
			}
			flush_bits = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);
		}
	}
}

/******************************************************************************/
/*! \brief Parses a Video Signal Type and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseVideoSignalType (const PARSER_Handle_t  ParserHandle)
{
	U8 video_signal_type;
	U8 video_format;
	U8 video_range;
	U8 colour_primaries;
	U8 transfer_characteristics;
	U8 matrix_coefficients;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	video_signal_type = mpeg4p2par_GetUnsigned(1, ParserHandle);
	if (video_signal_type)
	{
		U8 colour_description;
		video_format = mpeg4p2par_GetUnsigned(3, ParserHandle);
		video_range = mpeg4p2par_GetUnsigned(1, ParserHandle);
		colour_description = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (colour_description)
		{
			colour_primaries = mpeg4p2par_GetUnsigned(8, ParserHandle);
			transfer_characteristics = mpeg4p2par_GetUnsigned(8, ParserHandle);
			matrix_coefficients = mpeg4p2par_GetUnsigned(8, ParserHandle);
		}
		else
		{
			colour_primaries = 1;	/*COLOUR_PRIMARIES_ITU_R_BT_709 */
			transfer_characteristics = 1;	/*TRANSFER_ITU_R_BT_709 */
			matrix_coefficients = 1;	/*MATRIX_COEFFICIENTS_ITU_R_BT_709 */
		}
	}
	else
	{
		video_format = UNSPECIFIED_VIDEO_FORMAT;
		colour_primaries = 1;	/*COLOUR_PRIMARIES_ITU_R_BT_709 */
		transfer_characteristics = 1;	/*TRANSFER_ITU_R_BT_709 */
		matrix_coefficients = 1;	/*MATRIX_COEFFICIENTS_ITU_R_BT_709 */
	}

	PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries = colour_primaries;
	PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = transfer_characteristics;
	PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients = matrix_coefficients;
	PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VideoFormat = video_format;
}

/******************************************************************************/
/*! \brief Parses a Video Packet Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseVideoPacketHeader (const PARSER_Handle_t  ParserHandle)
{
	U8 header_extension_code = 0;
	U8 vop_width, vop_height;
	U32 WidthInMacroblocks, HeightInMacroblocks;
	U32 MacroblockLength;
	U32 MacroblockSize;
	U16 macroblock_number;
	U8 modulo_time_base;
	U16 vop_time_increment;
	U8 vop_coding_type;
	U8 vop_shape_coding_type;
	U8 vop_fcode_forward;
	U8 vop_fcode_backward;
	U16 vop_id;
	U8 vop_id_for_prediction_indication;
	U32 vop_id_for_prediction;
	U32 NoOfBits;
	U32 Temp;
	U32 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	mpeg4p2par_NextResyncMarker(ParserHandle);
	if (PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable == 0 && PARSER_Data_p->VideoObjectPlane.ResyncMarker != 0)
		flush_bits = mpeg4p2par_GetUnsigned(PARSER_Data_p->VideoObjectPlane.ResyncMarker, ParserHandle);

	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != RECTANGULAR)
	{
		header_extension_code = mpeg4p2par_GetUnsigned(1, ParserHandle);

		if (header_extension_code && !(PARSER_Data_p->VideoObjectLayer.SpriteEnable == STATIC_SPRITE && PARSER_Data_p->VideoObjectPlane.VopCodingType == I_VOP))
		{
			Temp = mpeg4p2par_GetUnsigned(28, ParserHandle);	/* vop_width, marker_bit, vop_height, marker_bit */
			vop_width = ((Temp >> 15) & 0x1FFF);
			vop_height = ((Temp >> 1) & 0x1FFF);
			PARSER_Data_p->VideoObjectPlane.VOPWidth = vop_width;
			PARSER_Data_p->VideoObjectPlane.VOPHeight = vop_height;

			flush_bits = mpeg4p2par_GetUnsigned(28, ParserHandle);	/* vop_horizontal_mc_spatial_ref, marker_bit,
																					vop_vertical_mc_spatial_ref, marker_bit */
		}

	}

	WidthInMacroblocks = (PARSER_Data_p->VideoObjectPlane.VOPWidth + 15)/ 16;
	HeightInMacroblocks = (PARSER_Data_p->VideoObjectPlane.VOPHeight + 15)/ 16;
	MacroblockSize = WidthInMacroblocks * HeightInMacroblocks;
	MacroblockLength = CountNumberOfBits(MacroblockSize);
	macroblock_number = mpeg4p2par_GetUnsigned(MacroblockLength, ParserHandle);

	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != BINARY_ONLY)
		flush_bits = mpeg4p2par_GetUnsigned(5, ParserHandle);	/*quant_scale */

	if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == RECTANGULAR)
		header_extension_code = mpeg4p2par_GetUnsigned(1, ParserHandle);

	if (header_extension_code)
	{
		do{
			modulo_time_base = mpeg4p2par_GetUnsigned(1, ParserHandle);
		} while (modulo_time_base != 0);

		flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* marker_bit */
		NoOfBits = CountNumberOfBits(PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution);
		NoOfBits =  (NoOfBits < 1)? 1 : NoOfBits;
		Temp = mpeg4p2par_GetUnsigned((NoOfBits + 3), ParserHandle);	/* vop_time_increment, marker_bit, vop_coding_type */
		vop_time_increment = (Temp >> 3);
		/*
		if (PARSER_Data_p->VideoObjectLayer.FixedVOPRate == 0)
		{
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / vop_time_increment);
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = ((PARSER_Data_p->VideoObjectLayer.VOPTimeIncrementResolution * 1000) / vop_time_increment);
        }
		*/
		vop_coding_type = (Temp & 0x3);
		PARSER_Data_p->VideoObjectPlane.VopCodingType = vop_coding_type;
		if (PARSER_Data_p->VideoObjectPlane.VopCodingType == I_VOP)
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
			PARSER_Data_p->PictureSpecificData.PictureType = MPEG4P2_I_VOP;
		}
		else if (PARSER_Data_p->VideoObjectPlane.VopCodingType == P_VOP)
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
			PARSER_Data_p->PictureSpecificData.PictureType = MPEG4P2_P_VOP;
		}
		else if (PARSER_Data_p->VideoObjectPlane.VopCodingType == B_VOP)
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
			PARSER_Data_p->PictureSpecificData.PictureType = MPEG4P2_B_VOP;
		}

		if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != RECTANGULAR)
		{
			flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* change_conv_ratio_disable */
			if (vop_coding_type != I_VOP)
				vop_shape_coding_type = mpeg4p2par_GetUnsigned(1, ParserHandle);
		}

		if (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape != BINARY_ONLY)
		{
			flush_bits = mpeg4p2par_GetUnsigned(3, ParserHandle);	/* intra_dc_vlc_thr */

			if ((PARSER_Data_p->VideoObjectLayer.ReducedResolutionVopEnable) && (PARSER_Data_p->VideoObjectLayer.VideoObjectLayerShape == RECTANGULAR) &&
				((PARSER_Data_p->VideoObjectPlane.VopCodingType == P_VOP) || (PARSER_Data_p->VideoObjectPlane.VopCodingType == I_VOP)))
				flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* vop_reduced_resolution */

			if (PARSER_Data_p->VideoObjectPlane.VopCodingType != I_VOP)
			{
				vop_fcode_forward = mpeg4p2par_GetUnsigned(3, ParserHandle);
				PARSER_Data_p->VideoObjectPlane.VopFcodeForward = vop_fcode_forward;
			}

			if (PARSER_Data_p->VideoObjectPlane.VopCodingType == B_VOP)
			{
				vop_fcode_backward = mpeg4p2par_GetUnsigned(3, ParserHandle);
				PARSER_Data_p->VideoObjectPlane.VopFcodeBackward = vop_fcode_backward;
			}
		}
	}

	if (PARSER_Data_p->VideoObjectLayer.NewpredEnable)
	{
		NoOfBits = ((PARSER_Data_p->VideoObjectPlane.VopTimeIncrement + 3) > 15 )? 15 : (PARSER_Data_p->VideoObjectPlane.VopTimeIncrement + 3);
		vop_id = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);

		vop_id_for_prediction_indication = mpeg4p2par_GetUnsigned(1, ParserHandle);
		if (vop_id_for_prediction_indication)
			vop_id_for_prediction = mpeg4p2par_GetUnsigned(NoOfBits, ParserHandle);

		flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/*	marker_bit */
	}
}

/******************************************************************************/
/*! \brief Parses a Group Of Blocks Layer and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseGOBLayer (const PARSER_Handle_t  ParserHandle)
{
	U8 gob_header_empty;
	U32 flush_bits;
	U32 Temp;
	U32 i;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	gob_header_empty = 1;
	if (PARSER_Data_p->VideoObjectPlane.GOBNumber != 0)
	{
		if (mpeg4p2par_ReadUnsigned(17, ParserHandle) == RESYNC_MARKER)
		{
			gob_header_empty = 0;
			flush_bits = mpeg4p2par_ReadUnsigned(17, ParserHandle);	/* gob_resync_marker */
			Temp = mpeg4p2par_ReadUnsigned(12, ParserHandle);	/* gob_number, gob_frame_id, quant_scale */
			PARSER_Data_p->VideoObjectPlane.GOBNumber = ((Temp >> 7) & 0x1F);	/* gob_number */
		}
	}

	for(i = 0; i < PARSER_Data_p->VideoObjectPlane.NumMacroblocksInGOB; i++)
		mpeg4p2par_ParseMacroblock(ParserHandle);

	if ((mpeg4p2par_ReadUnsigned(17, ParserHandle) != RESYNC_MARKER) &&
		(mpeg4p2par_NextBitsByteAligned(ParserHandle, 17) == RESYNC_MARKER))
		mpeg4p2par_ByteAlign(ParserHandle);	/* align the byte stream */

	PARSER_Data_p->VideoObjectPlane.GOBNumber++;
}

/******************************************************************************/
/*! \brief Parses a Macroblock and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ParseMacroblock (const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

}

/******************************************************************************/
/*! \brief Searches for the next resync marker.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
U32 mpeg4p2par_NextBitsResyncMarker(const PARSER_Handle_t  ParserHandle)
{
	/* U8 flush_bits; */
	U32 Value;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (PARSER_Data_p->VideoObjectLayer.ResyncMarkerDisable == 0)
	{
		Value = mpeg4p2par_NextBitsByteAligned(ParserHandle, PARSER_Data_p->VideoObjectPlane.ResyncMarker);
		if (Value == 0)
			return 2;
		else if (Value == 1)
			return 1;
	}
	return 0;
}


/******************************************************************************/
/*! \brief Searches for the next resync marker.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_NextResyncMarker(const PARSER_Handle_t  ParserHandle)
{
	U8 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* removes zero bit */
	mpeg4p2par_ByteAlign(ParserHandle);
}

/******************************************************************************/
/*! \brief Removes any zero bit and a string of 0 to 7 1 bits used for stuffing.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_NextStartCode(const PARSER_Handle_t  ParserHandle)
{
	U8 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* removes zero bit */
	mpeg4p2par_ByteAlign(ParserHandle);
}

/******************************************************************************/
/*! \brief Aligns the stream pointer to the end of a byte.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_ByteAlign(const PARSER_Handle_t  ParserHandle)
{
	U32 SkipBits = 0;
	U32 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	while (! mpeg4p2par_ByteAligned(ParserHandle, SkipBits))
		SkipBits += 1;

	flush_bits = mpeg4p2par_GetUnsigned(SkipBits, ParserHandle);	/* remove SkipBits bits */
}

/******************************************************************************/
/*! \brief Checking if the stream pointer is byte aligned.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return U32
 */
/******************************************************************************/
U32 mpeg4p2par_ByteAligned(const PARSER_Handle_t  ParserHandle, U32 nBits)
{
	U32 BitOffset;

	/* U32 flush_bits; */

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	BitOffset = PARSER_Data_p->ByteStream.BitOffset;	/* bit offset in a byte. Max offset is 7 and when aligned its -1 */
    /* equivalent to BitOffset - nBits == -1 , without the warning of comparison between signed and unsigned */
    if ((BitOffset + 1) == nBits)
		return 1;
	else
		return 0;
}

/******************************************************************************/
/*! \brief Returns next nBits starting from the byte aligned position.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return U32
 */
/******************************************************************************/
U32 mpeg4p2par_NextBitsByteAligned(const PARSER_Handle_t  ParserHandle, U32 nBits)
{
	U32 SkipBits = 0;
	U32 Value;

    /* to mask the n least significant bits of an integer */
    static const U32 mask[33] =
    {
       0x00000000, 0x00000001, 0x00000003, 0x00000007,
       0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
       0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
       0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
       0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
       0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
       0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
       0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
       0xffffffff
    };

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (mpeg4p2par_ByteAligned(ParserHandle, SkipBits))
	{
		if (mpeg4p2par_ReadUnsigned(8, ParserHandle) == 0x7F)	/* stuffing bits */
			SkipBits += 8;
	}
	else
	{
		while (! mpeg4p2par_ByteAligned(ParserHandle, SkipBits))	/* count SkipBits to align */
			SkipBits += 1;
	}

	Value = mpeg4p2par_ReadUnsigned((nBits + SkipBits), ParserHandle);
	return (Value & mask[nBits]);
}

/***************************************************************************************************************/
/*
Table B-33 -- Code table for the first trajectory point
dmv value							SSS	VLC													dmv_code
-16383  -8192, 8192  16383	14	111111111110	00000000000000...01111111111111, 10000000000000...11111111111111
-8191  -4096, 4096  8191		13	11111111110		  0000000000000...0111111111111, 1000000000000...1111111111111
-4095  -2048, 2048  4095		12	1111111110			 000000000000...011111111111, 100000000000...111111111111
-2047...-1024, 1024...2047		11	111111110				00000000000...01111111111, 10000000000...11111111111
-1023...-512, 512...1023		10	11111110					  0000000000...0111111111, 1000000000...1111111111
-511...-256, 256...511			9	1111110						 000000000...011111111, 100000000...111111111
-255...-128, 128...255			8	111110							00000000...01111111, 10000000...11111111
-127...-64, 64...127				7	11110								  0000000...0111111, 1000000...1111111
-63...-32, 32...63				6	1110									 000000...011111, 100000...111111
-31...-16, 16...31				5	110										00000...01111, 10000...1111
-15...-8, 8...15					4	101										  0000...0111, 1000...1111
-7...-4, 4...7						3	100											 000...011, 100...111
-3...-2, 2...3						2	011												00...01, 10...11
-1, 1									1	010														0, 1
0										0	00															-
*/
/****************************************************************************************************************/

static __inline U32 ReadDMVLength(const PARSER_Handle_t  ParserHandle)
{
	U32 Value;
	switch(mpeg4p2par_GetUnsigned(2, ParserHandle))
	{
		default:
		case 0:
			return 0;
		case 1:
			return (1 + mpeg4p2par_GetUnsigned(1, ParserHandle));
		case 2:
			return (3 + mpeg4p2par_GetUnsigned(1, ParserHandle));
		case 3:
			Value = 5;
			while(mpeg4p2par_GetUnsigned(1, ParserHandle) && (Value <= 14))
				Value++;
			if(Value == 15)
				return -1;
			return Value;
	}
}

static __inline U32 ReadDMVCode(const PARSER_Handle_t  ParserHandle, U32 len)
{
	U32 base;

	if(!len)
		return 0;

	if(mpeg4p2par_GetUnsigned(1, ParserHandle) == 1)
		base = 1 << (len - 1);
	else
		base =- (1 << len) + 1;

	if(len > 1)
		base += mpeg4p2par_GetUnsigned((len - 1), ParserHandle);

	return base;
}

/******************************************************************************/
/*! \brief Parses a Sprite Trajectory and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
U32 mpeg4p2par_ParseSpriteTrajectory (const PARSER_Handle_t  ParserHandle)
{
	U32 i;
	S32 dmv_length;
	U32 warping_mv_code[4][2];
	U32 flush_bits;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if(/*(PARSER_Data_p->VideoObjectLayer.SpriteWarpingPoints < 0) ||*/ (PARSER_Data_p->VideoObjectLayer.SpriteWarpingPoints > 3))
		return 1;

	for (i = 0; i < PARSER_Data_p->VideoObjectLayer.SpriteWarpingPoints; i++)
	{
		dmv_length = ReadDMVLength(ParserHandle);
		if(dmv_length < 0)
			return 1;
		warping_mv_code[i][0]=ReadDMVCode(ParserHandle, dmv_length);	/* warping_mv_code(du[i]) */
		/*if(!bad_divx)
			mpeg4p2par_GetUnsigned(1, ParserHandle);*/
		dmv_length = ReadDMVLength(ParserHandle);
		if(dmv_length < 0)
			return 1;
		warping_mv_code[i][1] = ReadDMVCode(ParserHandle, dmv_length);	/* warping_mv_code(dv[i]) */
		flush_bits = mpeg4p2par_GetUnsigned(1, ParserHandle);	/* marker_bit */

	}

	return 0;
}

/******************************************************************************/
/*! \brief Parses a Brightness Change Factor and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
 /*
 *	change_factor value	change_factor_length value	change_factor_length VLC	change_factor
 *	-16...-1, 1...16					0								0							00000...01111, 10000...11111
 *	-48...-17, 17...48				1								10							000000...011111, 100000...111111
 *	112...-49, 49...112				2								110						0000000...0111111, 1000000...1111111
 *	113624								3								1110						000000000...111 111 111
 *	625...1648							4								1111						00000000001111111111
 */
/******************************************************************************/
U32 mpeg4p2par_ParseBrightnessChangeFactor (const PARSER_Handle_t  ParserHandle)
{
	U32 change_factor = 0;
	U32 length = 0;
	/* U32 i; */

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	while(mpeg4p2par_GetUnsigned(1, ParserHandle) && (length < 4))
		length++;

	switch(length)
	{
	case 0:
		if(mpeg4p2par_GetUnsigned(1, ParserHandle) == 1)
			change_factor = 1 + mpeg4p2par_GetUnsigned(4, ParserHandle);
		else
			change_factor = -16 + mpeg4p2par_GetUnsigned(4, ParserHandle);
		break;
	case 1:
		if(mpeg4p2par_GetUnsigned(1, ParserHandle) == 1)
			change_factor = 17 + mpeg4p2par_GetUnsigned(5, ParserHandle);
		else
			change_factor = -48 + mpeg4p2par_GetUnsigned(5, ParserHandle);
		break;
	case 2:
		if(mpeg4p2par_GetUnsigned(1, ParserHandle) == 1)
			change_factor = 49 + mpeg4p2par_GetUnsigned(6, ParserHandle);
		else
			change_factor = -112 + mpeg4p2par_GetUnsigned(6, ParserHandle);
		break;
	case 3:
		change_factor = 113 + mpeg4p2par_GetUnsigned(9, ParserHandle);
		break;
	case 4:
		change_factor = 625 + mpeg4p2par_GetUnsigned(10, ParserHandle);
		break;
	}

	return change_factor;
}

/******************************************************************************/
/*! \brief Parses a Motion Shape Texture and fills data structures.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_MotionShapeTexture(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	if (PARSER_Data_p->VideoObjectLayer.DataPartitioned)
		mpeg4p2par_DataPartitionedMotionShapeTexture(ParserHandle);
	else
		mpeg4p2par_CombinedMotionShapeTexture(ParserHandle);
}

/******************************************************************************/
/*! \brief Parses a DataPartitionedMotionShapeTexture and fills data structures.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_DataPartitionedMotionShapeTexture(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

}

/******************************************************************************/
/*! \brief Parses a CombinedMotionShapeTexture and fills data structures.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_CombinedMotionShapeTexture(const PARSER_Handle_t  ParserHandle)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	do {
		mpeg4p2par_ParseMacroblock(ParserHandle);
	} while ((mpeg4p2par_NextBitsResyncMarker(ParserHandle) != 1) && (mpeg4p2par_NextBitsByteAligned(ParserHandle, 23) != 0));
}

/******************************************************************************/
/*! \brief Parses a Update Picture Params and fills data structures.
 *
 *
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void mpeg4p2par_UpdatePictureParams(const PARSER_Handle_t  ParserHandle)
{
	U32 RefPicIndex0;
	U32 RefPicIndex1;
	U32 i;

	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorType = MPEG4P2_DEFAULT_COLOR_TYPE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BitmapType = MPEG4P2_DEFAULT_BITMAP_TYPE;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.PreMultipliedColor = MPEG4P2_DEFAULT_PREMULTIPLIED_COLOR;

	switch (PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients)
	{
		case MATRIX_COEFFICIENTS_UNSPECIFIED :
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_CONVERSION_MODE_UNKNOWN;
			break;

		case MATRIX_COEFFICIENTS_FCC : /* Very close to ITU_R_BT601 matrixes */
		PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT601;
			break;

		case MATRIX_COEFFICIENTS_ITU_R_BT_470_2_BG : /* Same matrixes as ITU_R_BT601 */
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT470_2_BG;
			break;

		case MATRIX_COEFFICIENTS_SMPTE_170M : /* Same matrixes as ITU_R_BT601 */
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_170M;
			break;

		case MATRIX_COEFFICIENTS_SMPTE_240M : /* Slightly differ from ITU_R_BT709 */
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_SMPTE_240M;
			break;

		case MATRIX_COEFFICIENTS_ITU_R_BT_709 :
		default : /* 709 seems to be the most suitable for the default case */
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorSpaceConversion = STGXOBJ_ITU_R_BT709;
			break;
	}

	switch (PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect)
	{
		case STVID_DISPLAY_ASPECT_RATIO_16TO9 :
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_16TO9;
			break;

		case STVID_DISPLAY_ASPECT_RATIO_SQUARE :
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_SQUARE;
			break;

		case STVID_DISPLAY_ASPECT_RATIO_4TO3 :
		default :
			PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.AspectRatio = STGXOBJ_ASPECT_RATIO_4TO3;
			break;
	}

	/* if the aspect ratio is extended PAR, take the par width and par height */
	if (PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect == STVID_DISPLAY_ASPECT_RATIO_EXTENDED_PAR)
	{
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = PARSER_Data_p->VideoObjectLayer.PARWidth;
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = PARSER_Data_p->VideoObjectLayer.PARHeight;
	}
	else
	{
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = PARSER_Data_p->VideoObjectPlane.VOPWidth;
		PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = PARSER_Data_p->VideoObjectPlane.VOPHeight;
	}

	/* Setting Interlace/Progressive and TopFieldFirst fields */
    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
	if (PARSER_Data_p->VideoObjectLayer.Interlace)
	{
		if(PARSER_Data_p->VideoObjectPlane.TopFieldFirst == 1)
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
			PARSER_Data_p->PictureSpecificData.TffFlag = TRUE;
		}
		else
		{
			PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = FALSE;
			PARSER_Data_p->PictureSpecificData.TffFlag = FALSE;
		}

		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  STGXOBJ_INTERLACED_SCAN;
		PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
	}
	else	/* Progressive */
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
		PARSER_Data_p->PictureSpecificData.TffFlag = FALSE;
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  STGXOBJ_PROGRESSIVE_SCAN;
		PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
	}


	/* Forward Reference frame/field (for B fields or frames) */
	RefPicIndex0 = PARSER_Data_p->StreamState.NextReferencePictureIndex;
	/* Backward Reference frame (for B fields or frames) and Reference frame (for P pictures) */
	RefPicIndex1 = PARSER_Data_p->StreamState.NextReferencePictureIndex ^ 1;

	if (PARSER_Data_p->VideoObjectPlane.VopCodingType == I_VOP)
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
		PARSER_Data_p->PictureSpecificData.PictureType = MPEG4P2_I_VOP;
		PARSER_Data_p->PictureGenericData.IsReference = (PARSER_Data_p->VideoObjectPlane.VopCoded == 1)? TRUE: FALSE;
		PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = (PARSER_Data_p->VideoObjectPlane.VopCoded == 1)? TRUE: FALSE;

		mpeg4p2par_HandlePictureTypeI(ParserHandle, RefPicIndex0);
	}
	else if (PARSER_Data_p->VideoObjectPlane.VopCodingType == P_VOP)
	{
		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
		PARSER_Data_p->PictureSpecificData.PictureType = MPEG4P2_P_VOP;
		PARSER_Data_p->PictureGenericData.IsReference = (PARSER_Data_p->VideoObjectPlane.VopCoded == 1)? TRUE: FALSE;
		PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

		mpeg4p2par_HandlePictureTypeP(ParserHandle, RefPicIndex1, RefPicIndex0);
	}
	else if (PARSER_Data_p->VideoObjectPlane.VopCodingType == B_VOP)
	{

		PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
		PARSER_Data_p->PictureSpecificData.PictureType = MPEG4P2_B_VOP;
		PARSER_Data_p->PictureGenericData.IsReference = FALSE;
		PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

		mpeg4p2par_HandlePictureTypeB(ParserHandle, RefPicIndex1, RefPicIndex0);
	}

	if (PARSER_Data_p->VideoObjectLayer.RandomAccessibleVOL == 1)	/* All VOP in VOL are individually decodable */
    {
        PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = TRUE;
    }

    /* reset discontinuity flag if we encountered a random access point */
    if (PARSER_Data_p->PictureLocalData.IsRandomAccessPoint)
    {
        PARSER_Data_p->DiscontinuityDetected = FALSE;
    }

	for(i = 0; i < VIDCOM_MAX_NUMBER_OF_PAN_AND_SCAN; i++) {
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreHorizontalOffset = 0;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].FrameCentreVerticalOffset = 0;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayHorizontalSize = 16 * PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].DisplayVerticalSize = 16 * PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height;
		PARSER_Data_p->PictureGenericData.PanAndScanIn16thPixel[i].HasDisplaySizeRecommendation = FALSE;
	}

	PARSER_Data_p->PictureGenericData.NumberOfPanAndScan = 0;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Width = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Height = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Pitch = PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width; /* TODO: check right value */
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Offset = MPEG4P2_DEFAULT_BITMAPPARAMS_OFFSET;
	/* TODO? Data1_p, Size1, Data2_p, Size2 seem unknown at parser level. Is that true ??? */
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.SubByteFormat = MPEG4P2_DEFAULT_SUBBYTEFORMAT;
	PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BigNotLittle = MPEG4P2_DEFAULT_BIGNOTLITTLE;

    mpeg4p2par_SearchForAssociatedPTS(ParserHandle);

    if (PARSER_Data_p->PTSAvailableForNextPicture)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = TRUE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS = PARSER_Data_p->PTSForNextPicture;
        PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated = FALSE;
    }
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE; /* Set to TRUE as soon as a PTS is detected */
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS = 0;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33 = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated = TRUE; /* default zero PTS value must be considered as interpolated for the producer */
    }



}

/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type I.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        PrevPrevRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void mpeg4p2par_HandlePictureTypeI(const PARSER_Handle_t ParserHandle, U32 PrevPrevRefPicIndex)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

   UNUSED_PARAMETER(PrevPrevRefPicIndex);

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->StreamState.RefDistPreviousRefPic = 0;
	PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
}


/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type P.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        PrevPrevRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void mpeg4p2par_HandlePictureTypeP(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 PrevPrevRefPicIndex)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

   UNUSED_PARAMETER(PrevPrevRefPicIndex);

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
	PARSER_Data_p->StreamState.RefDistPreviousRefPic = 0;

	/* First check to make sure that the reference frame is available. */
	if(PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE)
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	}
	else
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 1;
		PARSER_Data_p->PictureGenericData.ReferenceListP0[0] = PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];
	}
}/* end void vc1par_HandleAPPictureTypeP() */


/*******************************************************************************/
/*! \Does processing related to obtaining an picture of type B.  This assumes that the frame
*    header has already been parsed.
*
* Picture info is updated
*
* \param ParserHandle Parser Handle (variables managed by the parser)
*        PrevRefPicIndex - The index in the full reference picture list
*                          to the most recent previous reference picture
*        PrevPrevRefPicIndex - The index in the full reference picture list
*                          to the oldest reference picture
*
* \return void
*/
/******************************************************************************/
void mpeg4p2par_HandlePictureTypeB(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 PrevPrevRefPicIndex)
{
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListP0 = 0;
	if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevPrevRefPicIndex] == FALSE) ||
	   (PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevRefPicIndex] == FALSE))
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = FALSE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 0;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 0;
	}
	else
	{
		PARSER_Data_p->AllReferencesAvailableForFrame = TRUE;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB0 = 1;
		PARSER_Data_p->PictureGenericData.UsedSizeInReferenceListB1 = 1;
		PARSER_Data_p->PictureGenericData.ReferenceListB0[0] = PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevPrevRefPicIndex];
		PARSER_Data_p->PictureGenericData.ReferenceListB1[0] = PARSER_Data_p->PictureGenericData.FullReferenceFrameList[PrevRefPicIndex];

	}/* end if((PARSER_Data_p->PictureGenericData.IsValidIndexInReferenceFrame[PrevPrevRefPicIndex] == FALSE) ... -- else*/
}/* end void vc1par_HandlePictureTypeB() */


/******************************************************************************/
/*! \brief Move back by n byte in the stream
 *
 * \param Address position from where to rewind
 * \param RewindInByte
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void * BackAddress
 */
/******************************************************************************/
U8 * mpeg4p2par_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle)
{
	U8 * BackAddress;
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	BackAddress = Address - RewindInByte;

	/* Adjust with bit buffer boundaries */
	if (BackAddress < PARSER_Data_p->BitBuffer.ES_Start_p)
	{
		/* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
		BackAddress = PARSER_Data_p->BitBuffer.ES_Stop_p
						- PARSER_Data_p->BitBuffer.ES_Start_p
						+ BackAddress
						+ 1;
	}
	return (BackAddress);
}/* end mpeg4p2par_MoveBackInStream() */


/******************************************************************************/
/*! \brief Move forward by n byte in the stream
 *
 * \param Address position from where to start
 * \param Number of bytes to move forward
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void * BackAddress
 */
/******************************************************************************/
U8 * mpeg4p2par_MoveForwardInStream(U8 * Address, U8 Increment, const PARSER_Handle_t  ParserHandle)
{
	U8 * ForwardAddress;
	/* dereference ParserHandle to a local variable to ease debug */
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

	/* Adjust with bit buffer boundaries */
	if (Address > PARSER_Data_p->BitBuffer.ES_Stop_p - Increment)
	{
		/* Bit buffer stands from ES_Start_p to ES_Stop_p inclusive */
		ForwardAddress = PARSER_Data_p->BitBuffer.ES_Start_p +
							(Increment - (PARSER_Data_p->BitBuffer.ES_Stop_p -
						Address) - 1);
	}
	else
	{
		ForwardAddress = Address + Increment;
	}
	return (ForwardAddress);
}/* end mpg4p2par_MoveForwardInStream() */


static void mpeg4p2par_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle)
{
    void* ES_Write_p;
	MPEG4P2ParserPrivateData_t * PARSER_Data_p;
    U8 * PictureStartAddress_p;

	PARSER_Data_p = ((MPEG4P2ParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    PictureStartAddress_p = PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress;

    /* no PTS associated to current picture */
    PARSER_Data_p->PTSAvailableForNextPicture = FALSE;

    /* Current position of write pointer in BitBuffer to check PTS & picture address side */
    ES_Write_p = PARSER_Data_p->BitBuffer.ES_Write_p;

    /* Check if PTS address is before PICT address in ES */
    while(
           (PARSER_Data_p->PTS_SCList.Read_p != PARSER_Data_p->PTS_SCList.Write_p) && /* PTS List is empty */
           (
                (
                    /* 1rst case : PictAdd > PTSAdd.                              */
                    (((U32)PictureStartAddress_p) >= ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p + 1)) &&
                    /* The 2 pointers are on the same side of ES_Write and at least one byte separate them.  */
                    (
                        /* PictAdd > PTSAdd => The 2 pointers must be on the same side of ES_Write */
                        (
                            (((U32)ES_Write_p) > ((U32)PictureStartAddress_p)) &&
                            (((U32)ES_Write_p) > ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p))
                        ) ||
                        (
                            (((U32)ES_Write_p) < ((U32)PictureStartAddress_p)) &&
                            (((U32)ES_Write_p) < ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p))
                        )
                    )
                ) ||
                (
                    /* 2nd case : PictAdd < PTSAdd.                               */
                    (((U32)PictureStartAddress_p) < ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p)) &&
                    /* The 2 pointers are not on the same side of ES_Write and at least 1 byte separate them.  */
                    (((U32)PARSER_Data_p->BitBuffer.ES_Stop_p
                    - (U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p + 1
                    + (U32)PictureStartAddress_p
                    - (U32)PARSER_Data_p->BitBuffer.ES_Start_p + 1 ) > 1 ) &&
                    /* PictAdd < PTSAdd => The 2 pointers must not be on the same side of ES_Write */
                    (((U32)ES_Write_p) > ((U32)PictureStartAddress_p)) &&
                    (((U32)ES_Write_p) < ((U32)PARSER_Data_p->PTS_SCList.Read_p->Address_p))
                )
            )
        )
    {
        /* Associate PTS to Picture */
        PARSER_Data_p->PTSAvailableForNextPicture = TRUE;
        PARSER_Data_p->PTSForNextPicture.PTS = PARSER_Data_p->PTS_SCList.Read_p->PTS32;
        PARSER_Data_p->PTSForNextPicture.PTS33 = PARSER_Data_p->PTS_SCList.Read_p->PTS33;
        PARSER_Data_p->PTSForNextPicture.Interpolated = FALSE;
        PARSER_Data_p->PTSForNextPicture.IsValid = TRUE;
        PARSER_Data_p->PTS_SCList.Read_p++;
        if (((U32)PARSER_Data_p->PTS_SCList.Read_p) >= ((U32)PARSER_Data_p->PTS_SCList.SCList + sizeof(PTS_t) * PTS_SCLIST_SIZE))
        {
            PARSER_Data_p->PTS_SCList.Read_p = (PTS_t*)PARSER_Data_p->PTS_SCList.SCList;
        }
    }
} /* End of mpeg4p2par_SearchForAssociatedPTS() function */


