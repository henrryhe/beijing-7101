/*******************************************************************************

File name   : parser_avs.c

Description : AVS parsing and filling of data structures

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    First STAPI version                              PLE
*******************************************************************************/

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
/* avs Parser specific */
#include "avsparser.h"

#define MAXIMUM_U32 0xFFFFFFFF
#define MINIMUM_S32 0x80000000
#define MAXIMUM_S32 0x7FFFFFFF

U8 * avspar_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle);
U8 * avspar_MoveForwardInStream(U8 * Address, U8 Increment, const PARSER_Handle_t  ParserHandle);
static void avspar_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle);
/* Conflict with AVS PARSER BY LQ */
#if 0
__inline U32 CountNumberOfBits(U32 value);
#endif

/* Macro ----------------------------------------------------------------- */
 #if defined(ST_OSLINUX)
 #define atoi(a)  simple_strtol(a,NULL,10);
 #endif
/* Functions ----------------------------------------------------------------- */
/* Conflict with AVS PARSER BY LQ */
#if 0
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
#endif
/******************************************************************************/
/*! \brief Initializes the structure variables.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void avspar_InitParserVariables (const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->VideoSequenceHeader.LowDelay = 0;
    PARSER_Data_p->VideoSequenceHeader.BitRate = 0;
    PARSER_Data_p->VideoSequenceHeader.VBVBufferSize = 0;

}


/******************************************************************************/
/*! \brief Parses a Video Sequence Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void avspar_ParseVideoSequenceHeader (const PARSER_Handle_t  ParserHandle)
{
    /* U8 video_object_type_indication; */
    U16 first_half_bit_rate;
    U16 latter_half_bit_rate;
    /* U32 bit_rate; */
    U16 first_half_vbv_buffer_size;
    /* U8 marker_bit; */
    U8 /* progressive_sequence, frame_rate_code, */ flush_bits;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

    /*PARSER_Data_p->VideoObjectLayer.RandomAccessibleVOL =TRUE;*/


    PARSER_Data_p->VideoSequenceHeader.ProfileID =  avspar_GetUnsigned(8, ParserHandle);
    PARSER_Data_p->VideoSequenceHeader.LevelID = avspar_GetUnsigned(8, ParserHandle);

    /*progressive sequence flag*/
    PARSER_Data_p->VideoSequenceHeader.ProgressiveSequence = avspar_GetUnsigned(1, ParserHandle);
    /* Picture width and Height*/
    PARSER_Data_p->VideoPictureHeader.PicWidth = avspar_GetUnsigned(14, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.PicHeight = avspar_GetUnsigned(14, ParserHandle);
    /* flush chroma_format 2-bit and sample_precision 3-bit, since we support 420 and 8-bit only*/
    PARSER_Data_p->VideoSequenceHeader.ChromaFormat = avspar_GetUnsigned(2, ParserHandle);
    PARSER_Data_p->VideoSequenceHeader.SmaplePrecision = avspar_GetUnsigned(3, ParserHandle);

    /* aspect ratio info */
    PARSER_Data_p->VideoSequenceHeader.AspectRatioInfo = avspar_GetUnsigned(4, ParserHandle);
    /*frame_rate_code*/
    PARSER_Data_p->VideoSequenceHeader.FrameRateCode = avspar_GetUnsigned (4, ParserHandle);

    /*Bitrate*/
    first_half_bit_rate = avspar_GetUnsigned(18, ParserHandle);
    flush_bits = avspar_GetUnsigned(1, ParserHandle);
    latter_half_bit_rate = avspar_GetUnsigned(12, ParserHandle);
    PARSER_Data_p->VideoSequenceHeader.BitRate = 400 * ((first_half_bit_rate << 18) | (latter_half_bit_rate));


    /* AVS low_delay flag*/
    PARSER_Data_p->VideoSequenceHeader.LowDelay = avspar_GetUnsigned(1, ParserHandle);

    flush_bits = avspar_GetUnsigned(1, ParserHandle);

    /* vbv buffer size*/
    first_half_vbv_buffer_size= avspar_GetUnsigned(18, ParserHandle);
    PARSER_Data_p->VideoSequenceHeader.VBVBufferSize= 8192*first_half_vbv_buffer_size;
    /*"reserved bits"*/
    flush_bits = avspar_GetUnsigned(3, ParserHandle);

    avspar_NextStartCode(ParserHandle); /* align the byte stream pointer */

    if (PARSER_Data_p->VideoSequenceHeader.BitRate == 0)
        PARSER_Data_p->VideoSequenceHeader.BitRate = AVS_DEFAULT_BIT_RATE;
    if (PARSER_Data_p->VideoSequenceHeader.VBVBufferSize == 0)
        PARSER_Data_p->VideoSequenceHeader.VBVBufferSize = AVS_DEFAULT_VBVMAX;

    switch (PARSER_Data_p->VideoSequenceHeader.AspectRatioInfo)
    {
        case 0: /* should have been Forbidden, but streams are not coded properly */
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            break;

        case 4: /* 1:1 (Square) */
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_221TO1;
            break;

        case 3: /* 16:11 (625-type stretched for 16:9 picture) */
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_16TO9;
            break;

        case 2: /* 12:11 (625-type for 4:3 picture) */
        default:
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Aspect = STVID_DISPLAY_ASPECT_RATIO_4TO3;
            break;

    }

    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.ScanType = (PARSER_Data_p->VideoSequenceHeader.ProgressiveSequence == 0)? STVID_SCAN_TYPE_INTERLACED : STVID_SCAN_TYPE_PROGRESSIVE;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.BitRate = PARSER_Data_p->VideoSequenceHeader.BitRate;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.MPEGStandard = STVID_AVS_STANDARD_GB_T_20090_2;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.IsLowDelay = PARSER_Data_p->VideoSequenceHeader.LowDelay;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VBVBufferSize = PARSER_Data_p->VideoSequenceHeader.VBVBufferSize;

    switch (/* frame_rate_code*/ PARSER_Data_p->VideoSequenceHeader.FrameRateCode) /* PLE_TO_DO : check that this correction is OK */
    {
        case 0: /* should have been Forbidden, but streams are not coded properly */
        case 3: /* frame rate is 25 */
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 25000;
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 25000;
                break;
        case 4: /* 29.97 */
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 29970;
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 29970;
            break;
        case 5:  /*30*/
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 30000;
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 30000;
            break;
        case 1:  /*23.97*/
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 23976;
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 23976;
            break;
        case 2: /* 24 */
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 24000;
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 24000;
            break;
        default:
            PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.FrameRate = 25000;
            PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.FrameRate = 25000;
            break;

    }

    PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  (PARSER_Data_p->VideoSequenceHeader.ProgressiveSequence == 0)? STGXOBJ_INTERLACED_SCAN : STGXOBJ_PROGRESSIVE_SCAN;

    /* return 0; */
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
void avspar_ParseVideoEditCode (const PARSER_Handle_t  ParserHandle)
{
    U32 i;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = TRUE;

    /* If closed_gov set to 1, we do not need any of our reference frames any more, so
     * mark them as no longer valid.  Also mark the 3 reference lists as empty*/
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


    /* Check the next bits for user_data_start_code for the user_data() */
}
/******************************************************************************/
/*! \brief Parses a Intra Video picture Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void avspar_ParseIntraPictureHeader(U8 *StartCodeValueAddress, U8 *NextStartCodeValueAddress,const PARSER_Handle_t  ParserHandle)
{
    U8 * BitBufferStartAddr_p;  /* noncached CPU address to dump the memory contents of the bit-buffer */
    U8 * BitBufferStopAddr_p;   /* noncached CPU address to dump the memory contents of the bit-buffer */
    /* U32 NextBits; */
    U32 time_code_flag,flush_bits;
    /* dereference ParserHandle to a local variable to ease debug */
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping; /* mapping for converting from the Device to CPU address */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    /* Default: this VOP is not valid */
    PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;
    PARSER_Data_p->VideoPictureHeader.SkipModeFlag = 0;

    /* Update position in bit buffer */
    /* The picture start address used through the CODEC API is the position of the beginning of the start code */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) avspar_MoveBackInStream(StartCodeValueAddress, 3, ParserHandle);
    /* The stop address is the byte preceeding the next start code */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = (void *) avspar_MoveBackInStream(NextStartCodeValueAddress, (3 + 1), ParserHandle);
    BitBufferStartAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress, &VirtualMapping);
    BitBufferStopAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress, &VirtualMapping);

    PARSER_Data_p->VideoPictureHeader.PictureCodingType = I_IMG;

    PARSER_Data_p->VideoPictureHeader.VbvDelay = avspar_GetUnsigned(16, ParserHandle);

    time_code_flag = avspar_GetUnsigned(1, ParserHandle);

    if (time_code_flag)
        PARSER_Data_p->VideoPictureHeader.TimeCode = avspar_GetUnsigned(24, ParserHandle);

    /*  if (Codec_Version_no > 520)  */  /*Codec_Version_no is 520 for rm52g, 521 for rm52h so on */
    flush_bits = avspar_GetUnsigned(1, ParserHandle);        /*marker_bit is not present in rm52g */

    PARSER_Data_p->VideoPictureHeader.PictureDistance = avspar_GetUnsigned(8, ParserHandle);

    if (PARSER_Data_p->VideoSequenceHeader.LowDelay)
        PARSER_Data_p->VideoPictureHeader.VbvCheckTimes = ue_v (ParserHandle);

    PARSER_Data_p->VideoPictureHeader.ProgressiveFrame = avspar_GetUnsigned(1, ParserHandle);

    if (!PARSER_Data_p->VideoPictureHeader.ProgressiveFrame)
        PARSER_Data_p->VideoPictureHeader.PictureStructure = avspar_GetUnsigned(1, ParserHandle);
    else
        PARSER_Data_p->VideoPictureHeader.PictureStructure = 1;

    PARSER_Data_p->VideoPictureHeader.TopFieldFirst =avspar_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.RepeatFirstField=avspar_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.FixedPictureQP=avspar_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.PictureQP=avspar_GetUnsigned(6, ParserHandle);

    if (PARSER_Data_p->VideoPictureHeader.ProgressiveFrame == 0)
        if (PARSER_Data_p->VideoPictureHeader.PictureStructure == 0)
            PARSER_Data_p->VideoPictureHeader.SkipModeFlag = avspar_GetUnsigned(1, ParserHandle);

    flush_bits = avspar_GetUnsigned(4, ParserHandle);
    /*"reserved bits"*/

    PARSER_Data_p->VideoPictureHeader.LoopFilterDisable = avspar_GetUnsigned(1, ParserHandle);
    if (!PARSER_Data_p->VideoPictureHeader.LoopFilterDisable)
        {
            PARSER_Data_p->VideoPictureHeader.LoopFilterParameterFlag = avspar_GetUnsigned(1, ParserHandle);
            if (PARSER_Data_p->VideoPictureHeader.LoopFilterParameterFlag )
                {
            PARSER_Data_p->VideoPictureHeader.AlphaCOffset = se_v (ParserHandle);
            PARSER_Data_p->VideoPictureHeader.BetaOffset = se_v (ParserHandle);
        }
    }
    /* update the parameters */
    avspar_UpdatePictureParams(ParserHandle);

    /* The VOP is the valid picture */
    PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

    /*Added by LQ*/
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *)avspar_MoveBackInStream(PARSER_Data_p->StartCode.SliceStartAddress_p, 4, ParserHandle);

}


/******************************************************************************/
/*! \brief Parses a Inter Video picture Header and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void avspar_ParseInterPictureHeader(U8 *StartCodeValueAddress, U8 *NextStartCodeValueAddress,const PARSER_Handle_t  ParserHandle)
{
    U8 * BitBufferStartAddr_p;  /* noncached CPU address to dump the memory contents of the bit-buffer */
    U8 * BitBufferStopAddr_p;   /* noncached CPU address to dump the memory contents of the bit-buffer */
    /* U32 NextBits; */
    U32 flush_bits,picture_coding_type;
    /* dereference ParserHandle to a local variable to ease debug */
    STAVMEM_SharedMemoryVirtualMapping_t    VirtualMapping; /* mapping for converting from the Device to CPU address */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
    STAVMEM_GetSharedMemoryVirtualMapping(&VirtualMapping );

    /* Default: this VOP is not valid */
    PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;
    /*PARSER_Data_p->VideoPictureHeader.PictureReferenceFlag = 1;*/
    PARSER_Data_p->VideoPictureHeader.PictureReferenceFlag = 0;

    /* Update position in bit buffer */
    /* The picture start address used through the CODEC API is the position of the beginning of the start code */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) avspar_MoveBackInStream(StartCodeValueAddress, 3, ParserHandle);
    /* The stop address is the byte preceeding the next start code */
    PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress  = (void *) avspar_MoveBackInStream(NextStartCodeValueAddress, (3 + 1), ParserHandle);
    BitBufferStartAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress, &VirtualMapping);
    BitBufferStopAddr_p = (U8*) STAVMEM_DeviceToCPU(PARSER_Data_p->PictureGenericData.BitBufferPictureStopAddress, &VirtualMapping);

    PARSER_Data_p->VideoPictureHeader.VbvDelay =avspar_GetUnsigned(16, ParserHandle);

    picture_coding_type = avspar_GetUnsigned(2, ParserHandle);
    if (picture_coding_type == 1)
        PARSER_Data_p->VideoPictureHeader.PictureCodingType = P_IMG;
    else
        PARSER_Data_p->VideoPictureHeader.PictureCodingType = B_IMG;

    PARSER_Data_p->VideoPictureHeader.PictureDistance = avspar_GetUnsigned(8, ParserHandle);

    if (PARSER_Data_p->VideoSequenceHeader.LowDelay)
        PARSER_Data_p->VideoPictureHeader.VbvCheckTimes = ue_v (ParserHandle);

    PARSER_Data_p->VideoPictureHeader.ProgressiveFrame = avspar_GetUnsigned(1, ParserHandle);
    if (!PARSER_Data_p->VideoPictureHeader.ProgressiveFrame)
    {
        PARSER_Data_p->VideoPictureHeader.PictureStructure = avspar_GetUnsigned(1, ParserHandle);
        if (!PARSER_Data_p->VideoPictureHeader.PictureStructure)         /*advanced_pred_mode_disable*/
        PARSER_Data_p->VideoPictureHeader.AdvancedPredModeDisable= avspar_GetUnsigned(1, ParserHandle);
    }
    else
        PARSER_Data_p->VideoPictureHeader.PictureStructure=1;

    PARSER_Data_p->VideoPictureHeader.TopFieldFirst =avspar_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.RepeatFirstField=avspar_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.FixedPictureQP=avspar_GetUnsigned(1, ParserHandle);
    PARSER_Data_p->VideoPictureHeader.PictureQP=avspar_GetUnsigned(6, ParserHandle);

    if (!(picture_coding_type == 2 && PARSER_Data_p->VideoPictureHeader.PictureStructure == 1))
    {
        PARSER_Data_p->VideoPictureHeader.PictureReferenceFlag = avspar_GetUnsigned(1, ParserHandle);
    }
    PARSER_Data_p->VideoPictureHeader.NoForwardReferenceFlag =avspar_GetUnsigned(1, ParserHandle);
    flush_bits = avspar_GetUnsigned(3, ParserHandle);

    PARSER_Data_p->VideoPictureHeader.SkipModeFlag = avspar_GetUnsigned(1, ParserHandle);

    PARSER_Data_p->VideoPictureHeader.LoopFilterDisable = avspar_GetUnsigned(1, ParserHandle);
    if (!PARSER_Data_p->VideoPictureHeader.LoopFilterDisable)
    {
        PARSER_Data_p->VideoPictureHeader.LoopFilterParameterFlag = avspar_GetUnsigned(1, ParserHandle);
        if (PARSER_Data_p->VideoPictureHeader.LoopFilterParameterFlag )
        {
            PARSER_Data_p->VideoPictureHeader.AlphaCOffset = se_v (ParserHandle);
            PARSER_Data_p->VideoPictureHeader.BetaOffset = se_v (ParserHandle);
        }
    }
    /* update the parameters */
    avspar_UpdatePictureParams(ParserHandle);

    /* The VOP is the valid picture */
    PARSER_Data_p->PictureLocalData.IsValidPicture = TRUE;

    /*Added by LQ*/
    PARSER_Data_p->PictureGenericData.BitBufferPictureStartAddress = (void *) avspar_MoveBackInStream(PARSER_Data_p->StartCode.SliceStartAddress_p, 4, ParserHandle);

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
void avspar_ParseUserDataHeader (const PARSER_Handle_t  ParserHandle)
{
    char UserDataBuffer[64];
    /* U32 NextBits; */
    U8 *StrPointer = NULL;
    U32 BufferPos = 0;
#ifdef AVS_CODEC
    U32 userdata_build_number = 0;
#endif /* AVS_CODEC */
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    memset(UserDataBuffer, 0, 64);

    while (avspar_ReadUnsigned(24, ParserHandle) != 0x000001)
    {
        UserDataBuffer[BufferPos] = avspar_GetUnsigned(8, ParserHandle);    /*  user_data */
        BufferPos++;
        BufferPos = (BufferPos == 64)? 0 : BufferPos;
    }

    if (StrPointer)
    {
        PARSER_Data_p->PictureSpecificData.ProgressiveSequence= 1;/*500;*/
    }
    return;
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
/* PLE_TO_DO. This function is unused */
#if 0
void avspar_ParseVideoSequenceExtensionHeader (const PARSER_Handle_t  ParserHandle)
{
    U8 video_signal_type;
    U8 video_format;
    U8 video_range;
    U8 colour_primaries;
    U8 transfer_characteristics;
    U8 matrix_coefficients;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    video_signal_type = avspar_GetUnsigned(1, ParserHandle);
    if (video_signal_type)
    {
        U8 colour_description;
        video_format = avspar_GetUnsigned(3, ParserHandle);
        video_range = avspar_GetUnsigned(1, ParserHandle);
        colour_description = avspar_GetUnsigned(1, ParserHandle);
        if (colour_description)
        {
            colour_primaries = avspar_GetUnsigned(8, ParserHandle);
            transfer_characteristics = avspar_GetUnsigned(8, ParserHandle);
            matrix_coefficients = avspar_GetUnsigned(8, ParserHandle);
        }
        else
        {
            colour_primaries = 1;   /*COLOUR_PRIMARIES_ITU_R_BT_709 */
            transfer_characteristics = 1;   /*TRANSFER_ITU_R_BT_709 */
            matrix_coefficients = 1;    /*MATRIX_COEFFICIENTS_ITU_R_BT_709 */
        }
    }
    else
    {
        video_format = UNSPECIFIED_VIDEO_FORMAT;
        colour_primaries = 1;   /*COLOUR_PRIMARIES_ITU_R_BT_709 */
        transfer_characteristics = 1;   /*TRANSFER_ITU_R_BT_709 */
        matrix_coefficients = 1;    /*MATRIX_COEFFICIENTS_ITU_R_BT_709 */
    }

    PARSER_Data_p->GlobalDecodingContextGenericData.ColourPrimaries = colour_primaries;
    PARSER_Data_p->GlobalDecodingContextGenericData.TransferCharacteristics = transfer_characteristics;
    PARSER_Data_p->GlobalDecodingContextGenericData.MatrixCoefficients = matrix_coefficients;
    PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.VideoFormat = video_format;
}
#endif
/******************************************************************************/
/*! \brief Parses a Macroblock and fills data structures.
 *
 * GlobalDecodingContext is updated with stream info
 *
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void
 */
/******************************************************************************/
void avspar_ParseMacroblock (const PARSER_Handle_t  ParserHandle)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void avspar_NextStartCode(const PARSER_Handle_t  ParserHandle)
{
    U8 flush_bits;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    flush_bits = avspar_GetUnsigned(1, ParserHandle);   /* removes zero bit */
    avspar_ByteAlign(ParserHandle);
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
void avspar_ByteAlign(const PARSER_Handle_t  ParserHandle)
{
    U32 SkipBits = 0;
    U32 flush_bits;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    while (! avspar_ByteAligned(ParserHandle, SkipBits))
        SkipBits += 1;

    flush_bits = avspar_GetUnsigned(SkipBits, ParserHandle);    /* remove SkipBits bits */
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
U32 avspar_ByteAligned(const PARSER_Handle_t  ParserHandle, U32 nBits)
{
    U32 BitOffset;
    /* U32 flush_bits; */

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    BitOffset = PARSER_Data_p->ByteStream.BitOffset;    /* bit offset in a byte. Max offset is 7 and when aligned its -1 */
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
U32 avspar_NextBitsByteAligned(const PARSER_Handle_t  ParserHandle, U32 nBits)
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
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    if (avspar_ByteAligned(ParserHandle, SkipBits))
    {
        if (avspar_ReadUnsigned(8, ParserHandle) == 0x7F)   /* stuffing bits */
            SkipBits += 8;
    }
    else
    {
        while (! avspar_ByteAligned(ParserHandle, SkipBits))    /* count SkipBits to align */
            SkipBits += 1;
    }

    Value = avspar_ReadUnsigned((nBits + SkipBits), ParserHandle);
    return (Value & mask[nBits]);
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
void avspar_UpdatePictureParams(const PARSER_Handle_t  ParserHandle)
{
    U32 RefPicIndex0;
    U32 RefPicIndex1;
    U32 i;

    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.ColorType = AVS_DEFAULT_COLOR_TYPE;
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BitmapType = AVS_DEFAULT_BITMAP_TYPE;
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.PreMultipliedColor = AVS_DEFAULT_PREMULTIPLIED_COLOR;

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
        PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = PARSER_Data_p->VideoPictureHeader.PicWidth ;
        PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = PARSER_Data_p->VideoPictureHeader.PicHeight;
    }
    else
    {
        PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Width = PARSER_Data_p->VideoPictureHeader.PicWidth ;
        PARSER_Data_p->GlobalDecodingContextGenericData.SequenceInfo.Height = PARSER_Data_p->VideoPictureHeader.PicHeight;
    }

    /* Setting Interlace/Progressive and TopFieldFirst fields */
    if (!PARSER_Data_p->VideoPictureHeader.PictureStructure)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_TOP_FIELD;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
        PARSER_Data_p->PictureSpecificData.TffFlag = TRUE;

        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  STGXOBJ_INTERLACED_SCAN;
        PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
    }
    else    /* Progressive */
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PictureStructure = STVID_PICTURE_STRUCTURE_FRAME;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.TopFieldFirst = TRUE;
        PARSER_Data_p->PictureSpecificData.TffFlag = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.ScanType =  STGXOBJ_PROGRESSIVE_SCAN;
        PARSER_Data_p->PictureGenericData.RepeatFirstField = FALSE;
    }


    /* Forward Reference frame/field (for B fields or frames) */
    RefPicIndex0 = (PARSER_Data_p->StreamState.NextReferencePictureIndex+1)%3;
    /* Backward Reference frame (for B fields or frames) and Reference frame (for P pictures) */
    RefPicIndex1 = (PARSER_Data_p->StreamState.NextReferencePictureIndex+2)%3;

    if (PARSER_Data_p->VideoPictureHeader.PictureCodingType == I_IMG)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_I;
        PARSER_Data_p->PictureSpecificData.PictureType = AVS_I_VOP;
        PARSER_Data_p->PictureGenericData.IsReference = TRUE;
        PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = TRUE;

        avspar_HandlePictureTypeI(ParserHandle, RefPicIndex0);
    }
    else if (PARSER_Data_p->VideoPictureHeader.PictureCodingType == P_IMG)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_P;
        PARSER_Data_p->PictureSpecificData.PictureType = AVS_P_VOP;
        PARSER_Data_p->PictureGenericData.IsReference =  TRUE;
        PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

        avspar_HandlePictureTypeP(ParserHandle, RefPicIndex1, RefPicIndex0);
    }
    else if (PARSER_Data_p->VideoPictureHeader.PictureCodingType == B_IMG)
    {

        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.MPEGFrame = STVID_MPEG_FRAME_B;
        PARSER_Data_p->PictureSpecificData.PictureType = AVS_B_VOP;
        PARSER_Data_p->PictureGenericData.IsReference = FALSE;
        PARSER_Data_p->PictureLocalData.IsRandomAccessPoint = FALSE;

        avspar_HandlePictureTypeB(ParserHandle, RefPicIndex1, RefPicIndex0);
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
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.Offset = AVS_DEFAULT_BITMAPPARAMS_OFFSET;
    /* TODO? Data1_p, Size1, Data2_p, Size2 seem unknown at parser level. Is that true ??? */
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.SubByteFormat = AVS_DEFAULT_SUBBYTEFORMAT;
    PARSER_Data_p->PictureGenericData.PictureInfos.BitmapParams.BigNotLittle = AVS_DEFAULT_BIGNOTLITTLE;

    avspar_SearchForAssociatedPTS(ParserHandle);

    if (PARSER_Data_p->PTSAvailableForNextPicture)
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = TRUE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS = PARSER_Data_p->PTSForNextPicture;
        PARSER_Data_p->PTSAvailableForNextPicture = FALSE;
    }
    else
    {
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.IsValid = FALSE; /* Set to TRUE as soon as a PTS is detected */
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS = 0;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.PTS33 = FALSE;
        PARSER_Data_p->PictureGenericData.PictureInfos.VideoParams.PTS.Interpolated = TRUE; /* default zero PTS value must be considered as interpolated for the producer */
    }

    /* Added by LQ For supporting AVS FW R2 */

    PARSER_Data_p->PictureSpecificData.AlphaCOffset = PARSER_Data_p->VideoPictureHeader.AlphaCOffset;
    PARSER_Data_p->PictureSpecificData.BetaOffset= PARSER_Data_p->VideoPictureHeader.BetaOffset;
    PARSER_Data_p->PictureSpecificData.FixedPictureQP= PARSER_Data_p->VideoPictureHeader.FixedPictureQP;
    PARSER_Data_p->PictureSpecificData.LoopFilterDisable = PARSER_Data_p->VideoPictureHeader.LoopFilterDisable;
    PARSER_Data_p->PictureSpecificData.LoopFilterParameterFlag= PARSER_Data_p->VideoPictureHeader.LoopFilterParameterFlag;
    PARSER_Data_p->PictureSpecificData.PictureCodingType= PARSER_Data_p->VideoPictureHeader.PictureCodingType;
    PARSER_Data_p->PictureSpecificData.PictureDistance= PARSER_Data_p->VideoPictureHeader.PictureDistance;
    PARSER_Data_p->PictureSpecificData.PictureQP= PARSER_Data_p->VideoPictureHeader.PictureQP;
    PARSER_Data_p->PictureSpecificData.PictureReferenceFlag= PARSER_Data_p->VideoPictureHeader.PictureReferenceFlag;
    PARSER_Data_p->PictureSpecificData.PictureStructure= PARSER_Data_p->VideoPictureHeader.PictureStructure;
    PARSER_Data_p->PictureSpecificData.SkipModeFlag= PARSER_Data_p->VideoPictureHeader.SkipModeFlag;
    PARSER_Data_p->PictureSpecificData.ProgressiveSequence= PARSER_Data_p->VideoSequenceHeader.ProgressiveSequence;
/*End*/
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
void avspar_HandlePictureTypeI(const PARSER_Handle_t ParserHandle, U32 PrevPrevRefPicIndex)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

   UNUSED_PARAMETER(PrevPrevRefPicIndex);

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void avspar_HandlePictureTypeP(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 PrevPrevRefPicIndex)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

   UNUSED_PARAMETER(PrevPrevRefPicIndex);

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
void avspar_HandlePictureTypeB(const PARSER_Handle_t ParserHandle, U32 PrevRefPicIndex, U32 PrevPrevRefPicIndex)
{
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
U8 * avspar_MoveBackInStream(U8 * Address, U8 RewindInByte, const PARSER_Handle_t  ParserHandle)
{
    U8 * BackAddress;
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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
}/* end avspar_MoveBackInStream() */


/******************************************************************************/
/*! \brief Move forward by n byte in the stream
 *
 * \param Address position from where to start
 * \param Number of bytes to move forward
 * \param ParserHandle Parser Handle (variables managed by the parser)
 * \return void * BackAddress
 */
/******************************************************************************/
U8 * avspar_MoveForwardInStream(U8 * Address, U8 Increment, const PARSER_Handle_t  ParserHandle)
{
    U8 * ForwardAddress;
    /* dereference ParserHandle to a local variable to ease debug */
    AVSParserPrivateData_t * PARSER_Data_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);

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

/******************************************************************************/
/*! \brief
 *
 * \param ParserHandle Parser Handle
 * \return void
 */
/******************************************************************************/
static void avspar_SearchForAssociatedPTS(const PARSER_Handle_t ParserHandle)
{
    void* ES_Write_p;
    AVSParserPrivateData_t * PARSER_Data_p;
    U8 * PictureStartAddress_p;

    PARSER_Data_p = ((AVSParserPrivateData_t *)((PARSER_Properties_t *) ParserHandle)->PrivateData_p);
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
} /* End of avspar_SearchForAssociatedPTS() function */

