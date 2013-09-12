
#ifndef __MPEG2DEBLOCK_VIDEO_TRANSFORMER_TYPES_H
#define __MPEG2DEBLOCK_VIDEO_TRANSFORMER_TYPES_H

#include "stddefs.h"

#ifdef MULTICOM
#include "mme.h"
#endif

#define MPEG2DEBLOCK_MME_TRANSFORMER_NAME    "MPEG2DEBLOCK_DEBLOCK"

/*
MPEG2DEBLOCK_MME_VERSION:
Identifies the MME API version of the MPEG2 Deblocking firmware.
If wants to build the firmware for old MME API version, change this string correspondingly.
*/
#ifndef MPEG2DEBLOCK_MME_VERSION
	#define MPEG2DEBLOCK_MME_VERSION	 11		/* Latest MME API version */
/*    #define MPEG2DEBLOCK_MME_VERSION   10     /* old MME API version */
#endif

#if (MPEG2DEBLOCK_MME_VERSION==11)
	#define MPEG2DEBLOCK_MME_API_VERSION "1.1"
#elif (MPEG2DEBLOCK_MME_VERSION==10)
    #define MPEG2DEBLOCK_MME_API_VERSION "1.0"
#else
    #define MPEG2DEBLOCK_MME_API_VERSION "Unknown"
#endif





#define MPEG2DEBLOCK_Q_MATRIX_SIZE		64

#define MPEG2DEBLOCK_DECODER_ID        0xCAFE
#define MPEG2DEBLOCK_DECODER_BASE      (MPEG2DEBLOCK_DECODER_ID << 16)

/*
** Definitions of the "mpeg_decoding_flags", according to the bit position, inside the 32 bits word
*/
#define MPEG_DECODING_FLAGS_TOP_FIELD_FIRST            0x0001      /* position at bit 1 : used to
                                                                      determine the type of picture */
#define MPEG_DECODING_FLAGS_FRAME_PRED_FRAME_DCT       0x0002      /* position at bit 2 : used for
                                                                      parsing progression purpose only */
#define MPEG_DECODING_FLAGS_CONCEALMENT_MOTION_VECTORS 0x0004      /* position at bit 3 : used for
                                                                      parsing progression purpose only */
#define MPEG_DECODING_FLAGS_Q_SCALE_TYPE               0x0008      /* position at bit 4 : used for the
                                                                      Inverse Quantisation process */
#define MPEG_DECODING_FLAGS_INTRA_VLC_FORMAT           0x0010      /* position at bit 5 : used for the
                                                                      selection of VLC tables when
                                                                      decode the DCT coefficients */
#define MPEG_DECODING_FLAGS_ALTERNATE_SCAN             0x0020      /* position at bit 6 : used for the
                                                                      Inverse Scan process */

#define MPEG_DECODING_FLAGS_PROGRESSIVE_FRAME		   0x0080      /* position at bit 8 used for post processing*/


/*
** MPEG2DEBLOCK_ChromaFormat_t :
**		Identifies the Type of Luma and Chroma Pixels of the Decoded Picture
*/
typedef enum
{
	MPEG2DEBLOCK_CHROMA_RESERVED = 0x00000000,   /* reserved chroma type */
	MPEG2DEBLOCK_CHROMA_4_2_0   = 0x00000001,    /* chroma type 4:2:0       */
	MPEG2DEBLOCK_CHROMA_4_2_2   = 0x00000002,    /* chroma type 4:2:2       */
	MPEG2DEBLOCK_CHROMA_4_4_4   = 0x00000003     /* chroma type 4:4:4       */
}   MPEG2Deblock_ChromaFormat_t;

/*
** MPEG2DEBLOCK_SequenceParam_t :
**		These parameters comes from the "video_sequence" parsed by the Host.
*/
typedef struct {
	U32                  StructSize;    /* Size of the structure in bytes */
	BOOL				 mpeg2;			/* TRUE for a MPEG2 bitstream */
	U32                  horizontal_size;
	U32                  vertical_size;
	U32                  progressive_sequence;
    MPEG2Deblock_ChromaFormat_t chroma_format;
	U8                   intra_quantiser_matrix[MPEG2DEBLOCK_Q_MATRIX_SIZE];
	U8                   non_intra_quantiser_matrix[MPEG2DEBLOCK_Q_MATRIX_SIZE];
	U8                   chroma_intra_quantiser_matrix[MPEG2DEBLOCK_Q_MATRIX_SIZE];
	U8                   chroma_non_intra_quantiser_matrix[MPEG2DEBLOCK_Q_MATRIX_SIZE];
}   MPEG2Deblock_SequenceParam_t;

/*
** MPEG2DEBLOCK_PictureCodingType_t :
**		Identifies the Picture Prediction : used of no, one or two reference pictures
*/
typedef enum
{
	MPEG2DEBLOCK_FORBIDDEN_PICTURE     = 0x00000000,  /* forbidden coding type                 */
	MPEG2DEBLOCK_INTRA_PICTURE         = 0x00000001,  /* intra (I) picture coding type         */
	MPEG2DEBLOCK_PREDICTIVE_PICTURE    = 0x00000002,  /* predictive (P) picture coding type    */
	MPEG2DEBLOCK_BIDIRECTIONAL_PICTURE = 0x00000003,  /* bidirectional (B) picture coding type */
	MPEG2DEBLOCK_DC_INTRA_PICTURE      = 0x00000004,  /* dc intra (D) picture coding type      */
	MPEG2DEBLOCK_RESERVED_1_PICTURE    = 0x00000005,  /* reserved coding type                  */
	MPEG2DEBLOCK_RESERVED_2_PICTURE    = 0x00000006,  /* reserved coding type                  */
	MPEG2DEBLOCK_RESERVED_3_PICTURE    = 0x00000007   /* reserved coding type                  */
}   MPEG2Deblock_PictureCodingType_t;

/*
** MPEG2DEBLOCK_IntraDcPrecision_t :
**		Identifies the Intra DC Precision
*/
typedef enum
{
	MPEG2DEBLOCK_INTRA_DC_PRECISION_8_BITS  = 0x00000000,   /* 8 bits Intra DC Precision  */
	MPEG2DEBLOCK_INTRA_DC_PRECISION_9_BITS  = 0x00000001,   /* 9 bits Intra DC Precision  */
	MPEG2DEBLOCK_INTRA_DC_PRECISION_10_BITS = 0x00000002,   /* 10 bits Intra DC Precision */
	MPEG2DEBLOCK_INTRA_DC_PRECISION_11_BITS = 0x00000003    /* 11 bits Intra DC Precision */
}   MPEG2Deblock_IntraDCPrecision_t;

/*
** MPEG2DEBLOCK_PictureStructure_t :
**		Identifies the Picture Type
*/
typedef enum
{
	MPEG2DEBLOCK_RESERVED_TYPE     = 0x00000000,   /* identifies a reserved structure type   */
	MPEG2DEBLOCK_TOP_FIELD_TYPE    = 0x00000001,   /* identifies a top field picture type    */
	MPEG2DEBLOCK_BOTTOM_FIELD_TYPE = 0x00000002,   /* identifies a bottom field picture type */
	MPEG2DEBLOCK_FRAME_TYPE        = 0x00000003    /* identifies a frame picture type        */
}   MPEG2Deblock_PictureStructure_t;
/*
** MPEG2DEBLOCK_MBDescr_t :
**		Defines the address type for the MPEG2 macro block description
*/
typedef U32 * MPEG2Deblock_MBDescr_t;

/*
** MPEG2DEBLOCK_PictureParam_t :
**      FilterOffsetQP is a deblocking using control in the range -51 to +51.
**      MBDescr_p is the address of the MB description buffer written by the firmware.
**		The other parameters come from the "picture_header" and "picture_coding_extension"
**      parsed by the video driver.
*/
typedef struct {
	U32                        StructSize;                 /* Size of the structure in bytes */
	MPEG2Deblock_PictureCodingType_t  picture_coding_type;        /* Parsing purpose */
	U32                        forward_horizontal_f_code;  /* Parsing purpose */
	U32                        forward_vertical_f_code;    /* Parsing purpose */
	U32                        backward_horizontal_f_code; /* Parsing purpose */
	U32                        backward_vertical_f_code;   /* Parsing purpose */
	MPEG2Deblock_IntraDCPrecision_t   intra_dc_precision;         /* used for the Inverse Quantisation process */
	MPEG2Deblock_PictureStructure_t   picture_structure;          /* Frame/field picture type */
	U32                        mpeg_decoding_flags;        /* Group of the six boolean values*/
	MPEG2Deblock_MBDescr_t            MBDescr_p;	               /* Address of the MB description buffer */
    int                        FilterOffsetQP;             /* Deblocking user control */
}   MPEG2Deblock_PictureParam_t;

/*
** MPEG2DEBLOCK_CompressedData_t :
**		Defines the start and stop addresses of a compressed picture.
*/
typedef U32 * MPEG2Deblock_CompressedData_t;

/*
** MPEG2DEBLOCK_LumaAddress_t :
**		Defines the address of a reconstructed luma frame buffer.
*/
typedef U32 * MPEG2Deblock_LumaAddress_t;

/*
** MPEG2DEBLOCK_ChromaAddress_t :
**		Defines the address of a reconstructed chroma frame buffer.
*/
typedef U32 * MPEG2Deblock_ChromaAddress_t;

/*
** MPEG2DEBLOCK_PictureBufferAddress_t :
**		Defines the addresses of the decoded and filtered pictures.
*/
typedef struct {
	U32                    StructSize;        /* Size of the structure in bytes */
    MPEG2Deblock_LumaAddress_t    DecodedLuma_p;     /* address of the decoded luma frame buffer */
	MPEG2Deblock_ChromaAddress_t  DecodedChroma_p;   /* address of the decoded chroma frame buffer */
	MPEG2Deblock_LumaAddress_t    MainLuma_p;		  /* address of the main luma frame buffer */
	MPEG2Deblock_ChromaAddress_t  MainChroma_p;	  /* address of the main chroma frame buffer */
	MPEG2Deblock_LumaAddress_t    DecimatedLuma_p;	  /* address of the decimated luma frame buffer */
	MPEG2Deblock_ChromaAddress_t  DecimatedChroma_p; /* address of the decimated chroma frame buffer */
}   MPEG2Deblock_PictureBufferAddress_t;

/*
** MPEG2DEBLOCK_HorizontalDeciFactor _t :
**		Identifies the horizontal decimation factor
*/
typedef enum
{
	MPEG2DEBLOCK_HDEC_1          = 0x00000000,  /* no H resize       */
	MPEG2DEBLOCK_HDEC_2          = 0x00000001,  /* H/2  resize       */
	MPEG2DEBLOCK_HDEC_4          = 0x00000002,  /* H/4  resize       */
	MPEG2DEBLOCK_HDEC_RESERVED   = 0x00000003,  /* reserved H resize */
	MPEG2DEBLOCK_HDEC_ADVANCED_2 = 0x00000101,  /* Advanced H/2 resize using improved 8-tap filters */
	MPEG2DEBLOCK_HDEC_ADVANCED_4 = 0x00000102   /* Advanced H/4 resize using improved 8-tap filters */

}   MPEG2Deblock_HorizontalDeciFactor_t;

/*
** MPEG2DEBLOCK_VerticalDeciFactor_t :
**		Identifies the vertical decimation factor
*/
typedef enum
{
	MPEG2DEBLOCK_VDEC_1        = 0x00000000,  /* no V resize              */
	MPEG2DEBLOCK_VDEC_2_PROG   = 0x00000004,  /* V/2 , progressive resize */
	MPEG2DEBLOCK_VDEC_2_INT    = 0x00000008,  /* V/2 , interlaced resize  */
	MPEG2DEBLOCK_VDEC_RESERVED = 0x00000012   /* Reserved V resize        */
}   MPEG2Deblock_VerticalDeciFactor_t;

/*
** MPEG2DEBLOCK_MainAuxEnable_t :
**		Used for enabling Main/Aux outputs
*/
typedef enum
{
	MPEG2DEBLOCK_AUXOUT_EN         = 0x00000010,  /* enable decimated reconstruction             */
	MPEG2DEBLOCK_MAINOUT_EN        = 0x00000020,  /* enable main reconstruction                  */
	MPEG2DEBLOCK_AUX_MAIN_OUT_EN   = 0x00000030   /* enable both main & decimated reconstruction */
}   MPEG2Deblock_MainAuxEnable_t;

/*
** MPEG2DEBLOCK_DeblockingEnable_t :
**		Identifies the deblocking mode.
*/
typedef enum
{
	MPEG2DEBLOCK_DEBLOCK_DISABLE = 0,      /* Disable deblocking on the full screen */
	MPEG2DEBLOCK_DEBLOCK_DEMO_RIGHT= 1,    /* Enable deblocking demo on the right side */
	MPEG2DEBLOCK_DEBLOCK_DEMO_LEFT = 2,    /* Enable deblocking demo on the left side */
	MPEG2DEBLOCK_DEBLOCK_FULLSCREEN = 3    /* Enable deblocking on the full screen */
} 	MPEG2Deblock_DeblockingEnable_t;

/*
** MPEG2DEBLOCK_TransformParam_t :
**		Specific parameters for the MME_TRANSFORM command send by MME_SendCommand()
*/
typedef struct {
	U32                          StructSize;                         /* Size of the structure in bytes */
	MPEG2Deblock_CompressedData_t       PictureStartAddrCompressedBuffer_p; /* Start for the compressed picture */
	MPEG2Deblock_CompressedData_t       PictureStopAddrCompressedBuffer_p;  /* Stop for the compressed picture */
	MPEG2Deblock_PictureBufferAddress_t PictureBufferAddress;        /* Address where the pictures are stored */
	MPEG2Deblock_PictureParam_t         PictureParam;                /* Picture parameters */
	MPEG2Deblock_HorizontalDeciFactor_t HorizontalDecimationFactor;  /* Horizontal decimation factor */
	MPEG2Deblock_VerticalDeciFactor_t   VerticalDecimationFactor;    /* Vertical decimation factor */
	MPEG2Deblock_MainAuxEnable_t        MainAuxEnable;               /* Enable Main and/or Aux outputs */
	MPEG2Deblock_DeblockingEnable_t     DeblockingEnable;            /* Enable demo R or L or fullscreen */
}   MPEG2Deblock_TransformParam_t;

/*
** MPEG2DEBLOCK_InitTransformerParam_t:
**		Parameters required for a firmware transformer initialization.
**
**		Naming convention :
**		-------------------
**			MPEG2DEBLOCK_InitTransformerParam_t
**			       |
**			       |-------------> This structure defines the parameter required when
**		                               using the MME_InitTransformer() function.
*/
typedef struct {
	U32 InputBufferBegin;	/* Begin address of the input circular buffer */
	U32 InputBufferEnd;		/* End address of the input circular buffer */
} MPEG2Deblock_InitTransformerParam_t;

/*
** MPEG2DEBLOCK_DecodingError_t :
** 	Errors returned in MPEG2DEBLOCK_CommandStatus_t.ErrorCode.
** 	These errors are bitfields, and several bits can be raised at the same time.
*/
typedef enum
{
#ifdef MULTICOM
    /* The transform completed successfully. */

    MPEG2DEBLOCK_DECODER_NO_ERROR = (MPEG2DEBLOCK_DECODER_BASE + 0),
#endif

	/* The firmware decoded to much MBs.
	 * MME_CommandStatus_t.Error can also contain MPEG2DEBLOCK_DECODER_ERROR_RECOVERED.
	 */
	MPEG2DEBLOCK_DECODER_ERROR_MB_OVERFLOW = (MPEG2DEBLOCK_DECODER_BASE + 1),

	/* The firmware encountered error(s) that were recovered:
     * MME_CommandStatus_t.Error can also contain MPEG2DEBLOCK_DECODER_ERROR_MB_OVERFLOW.

	 */
	MPEG2DEBLOCK_DECODER_ERROR_RECOVERED = (MPEG2DEBLOCK_DECODER_BASE + 2),

	/* The firmware encountered an error that can't be recovered:
	 * MPEG2DEBLOCK_CommandStatus_t.Status has no meaning.
	 * MME_CommandStatus_t.Error is equal to MPEG2DEBLOCK_DECODER_ERROR_NOT_RECOVERED.
	 */
	MPEG2DEBLOCK_DECODER_ERROR_NOT_RECOVERED = (MPEG2DEBLOCK_DECODER_BASE + 4),

	/* The transform was not completed when the timeout specified in
     * MPEG2DEBLOCK_CommandStatus_t.TransformTimeoutInMicros ellapsed.
	 */
	MPEG2DEBLOCK_DECODER_ERROR_TIMEOUT = (MPEG2DEBLOCK_DECODER_BASE + 5)

} MPEG2Deblock_DecodingError_t;

/*
** MPEG2DEBLOCK_CommandStatus_t :
** 	Data returned in MME_CommandStatus_t.AdditionalInfo_p when
** 	CmdCode is equal to MME_TRANSFORM.
*/
typedef struct {

    MPEG2Deblock_DecodingError_t ErrorCode; /* See MPEG2DEBLOCK_DecodingError_t */
    U32 TransformTimeoutInMicros; /* Transform command timeout in microseconds */
    U32 DecodeTimeInMicros; /* Decoding duration in microseconds */
    U32 FilterTimeInMicros; /* Filtering duration in microseconds */

#if (MPEG2DEBLOCK_MME_VERSION > 10)
    U32 PictureMeanQP; /* Picture mean QP */
    U32 PictureVarianceQP; /* Picture variance QP */
#endif

} MPEG2Deblock_CommandStatus_t;

#endif /* #ifndef __MPEG2DEBLOCK_VIDEO_TRANSFORMER_TYPES_H */

