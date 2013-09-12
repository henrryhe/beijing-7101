
#ifndef MP2H264INTERFACE_H
#define MP2H264INTERFACE_H

#include "stddefs.h"

#define MP2H264XCODE_MME_TRANSFORMER_NAME "MP2H264XCODER"

#define MC_INTRA 0
#define MC_FIELD 1
#define MC_FRAME 2
#define MC_16X8  2 
#define MC_DMV   3

#define XCODE_MPEG2_PRED_DIRECTION_FORWARD 0
#define XCODE_MPEG2_PRED_DIRECTION_BACKWARD 1

#define XCODE_MPEG2_FRAME_BLOCK 0
#define XCODE_MPEG2_UPPER_BLOCK 0
#define XCODE_MPEG2_LOWER_BLOCK 1


typedef U32 *Xcode_LumaAddress_t;
typedef U32 *Xcode_ChromaAddress_t;
typedef void *Xcode_GenericParam_t;

typedef enum 
{
	XCODE_ADDITIONAL_FLAG_NONE = 0,
	XCODE_ADDITIONAL_FLAG_DEBUG /* = 0x80000000 */
} Xcode_AdditionalFlags_t;

typedef enum 
{
	XCODE_CHROMA_FORMAT_420
} Xcode_ChromaFormat_t;

typedef enum
{
	XCODE_H264_ENTROPY_ENCODING_MODE_CAVLC,
	XCODE_H264_ENTROPY_ENCODING_MODE_CABAC
} Xcode_H264_EntropyEncodingMode_t;

typedef enum
{
	XCODE_MPEG2_MBTYPE_INTRA = 0,
	XCODE_MPEG2_MBTYPE_MOTION_FW = 1,
	XCODE_MPEG2_MBTYPE_MOTION_BW = 2,
	XCODE_MPEG2_MBTYPE_MOTION_BI = 3
} Xcode_MPEG2_MBType_t;


typedef enum
{
	XCODE_MPEG2_MB_MOTION_TYPE_NONE = 0,
	XCODE_MPEG2_MB_MOTION_TYPE_MC_FRAME = 2,
	XCODE_MPEG2_MB_MOTION_TYPE_MC_FIELD = 1,
	XCODE_MPEG2_MB_MOTION_TYPE_MC_16x8 = 2,
	XCODE_MPEG2_MB_MOTION_TYPE_MC_DMV = 3
} Xcode_MPEG2_MBMotionType_t;

typedef enum 
{
	XCODE_DECIMATION_FACTOR_NONE = 0x00000000, /* no resize */
	XCODE_DECIMATION_FACTOR_HDEC_2 = 0x00000001, /* H/2 resize */
	XCODE_DECIMATION_FACTOR_HDEC_4 = 0x00000002, /* H/4 resize */
	XCODE_DECIMATION_FACTOR_VDEC_2 = 0x00000004  /* V/2 , progressive resize */
}XCodeDecimationFactor_t;

typedef enum 
{
	XCODE_DISPLAY_ASPECT_RATIO_16TO9, /* 16/9 pixel aspect ratio */
	XCODE_DISPLAY_ASPECT_RATIO_4TO3,  /* 4/3 pixel aspect ratio */
	XCODE_DISPLAY_ASPECT_RATIO_221TO1, /* 2.21/1 pixel aspect ratio */
	XCODE_DISPLAY_ASPECT_RATIO_SQUARE  /* Square pixels. */
} Xcode_DisplayAspectRatio_t;

typedef enum 
{
	XCODE_ENCODING_MODE_FRAME,
	XCODE_ENCODING_MODE_FIELD 
} Xcode_EncodingMode_t;

typedef enum 
{
	XCODE_I_PICTURE,
	XCODE_P_PICTURE,
	XCODE_B_PICTURE
}Xcode_PictureCodingType_t;

typedef enum
{
	XCODE_H264_BASELINE_PROFILE = 66,
	XCODE_H264_MAIN_PROFILE = 77
} Xcode_H264_ProfileIdc_t;

typedef enum 
{
	XCODE_PICTURE_FLAGS_TOP_FIELD_FIRST = 1,
	XCODE_PICTURE_FLAGS_REPEAT_FIRST_FIELD = 2
}Xcode_PictureFlags_t;

typedef enum 
{
	XCODE_PICTURE_STRUCTURE_TOP_FIELD,
	XCODE_PICTURE_STRUCTURE_BOTTOM_FIELD,
	XCODE_PICTURE_STRUCTURE_FRAME
}Xcode_PictureStructure_t;


typedef enum 
{
	XCODE_RATE_CONTROL_NONE,
	XCODE_RATE_CONTROL_VBR,
	XCODE_RATE_CONTROL_CBR
}Xcode_RateControl_t;

typedef enum
{
	XCODE_SCAN_TYPE_PROGRESSIVE,
	XCODE_SCAN_TYPE_INTERLACED
}Xcode_ScanType_t;

typedef struct 
{
	U32 *CompressedData_p;
	U32  CompressedDataBufferSize;
}Xcode_OutputBuffer_t;


typedef struct 
{
	unsigned int  MBInfo; /* MPEG-2 decoded macroblock parameters packed in a single word */
	int MV[2][2];         /* MV[TOP(FRAME)/BOTTOM][foward/backward] */
}XCode_MPEG2H264_MBDescr_t;  


typedef struct 
{
	Xcode_LumaAddress_t Luma_p;		/*	Address of the frame buffer containing the 
										luma data of a picture */
	Xcode_ChromaAddress_t Chroma_p; /*	Address of the frame buffer containing 
										the chroma data of a picture */
	Xcode_GenericParam_t Param_p;	/*	Pointer to an allocated parameter array 
										that contains information required to perform
										the transcoding operation. (typically a set of 
										information extracted during the decoding process
										for every MBs of the decoded picture such as MB type,
										motion vectors.*/
} Xcode_InputBuffer_t;


typedef struct 
{
	U32 Dummy;
} Xcode_MPEG2H264_InitTransformerParam_t;


typedef struct 
{
    unsigned int    caps_len;
} caps_len_t;

/* this structure is as big as returned via caps_len */
typedef struct 
{
    unsigned char   *caps;
} caps_t;


typedef struct 
{
	Xcode_H264_ProfileIdc_t profile_idc;
	Xcode_H264_EntropyEncodingMode_t EntropyCodingMode;
#if defined(CIRCULAR_BUFFER)
	unsigned char *OutputBufferStart_p;
	unsigned int OutputBuffer_Size ;
#endif
}Xcode_MPEG2H264_GlobalParams_t;


typedef struct 
{
	U32 FrameRate; /* Sequence frame rate (from incoming stream) multiplied by 100 */
	U32 PictureWidth; /* Original picture width (from incoming stream) */
	U32 PictureHeight; /* Original picture height (from incoming stream) */
	U32 DecimationFactors; /* Set of flags as defined in Xcode_DecimationFactor _t */
	Xcode_ChromaFormat_t ChromaFormat; /* Chroma format from incoming stream */
	U32 RCOutputBitrateValue; /* Sequence target bit rate in bits/s */
	Xcode_RateControl_t RateControlMode;
	BOOL HalfPictureFlag; /* Enables the possibility to throw away one of the two
							   incoming fields  */
	Xcode_DisplayAspectRatio_t AspectRatio;
	BOOL ProgressiveSequence;
	U32 VideoFormat;
	U32 ColourPrimaries;
	U32 TransferCharacteristics;
	U32 MatrixCoefficients;

} Xcode_GlobalParams_t;


typedef struct 
{
	Xcode_InputBuffer_t RefPictureBuffer[2]; /* Previous frames buffers for reference pictures.
												RefPictureBuffer[1] and RefPictureBuffer [0] are
												previous reference decoded frames in decode order
												(RefPictureBuffer [1] being most recently decoded
												H.264 frame) */
#ifdef ENABLE_PICTURE_RECON
	Xcode_InputBuffer_t ReconBuffer;
#endif
}Xcode_MPEG2H264_TransformParams_t;

typedef struct 
{
	BOOL EndOfBitstreamFlag;					/*	TRUE=no more pictures to code FALSE=still coding
													set frame by frame from incoming stream */
	S32 TemporalReference;						/*	Current picture temporal reference */
	Xcode_PictureCodingType_t PictureCodingType;/*	Type of the original encoded picture. */
	Xcode_PictureStructure_t PictureStructure;	/*	Structure of the original encoded picture */
	Xcode_ScanType_t PictureScanType;			/*	Scan type of the original encoded picture */
	U32 PictureFlags;							/*	Set of flags as defined in Xcode_PictureFlags
													that can be ORed together */
	Xcode_EncodingMode_t EncodingMode;			/*	To specifiy if the picture shall be field 
													encoded or frame encoded */
	Xcode_InputBuffer_t InputBuffer;			/*	Structure containing addresses where
													the pixels of the picture to be transcoded
													and associated additional info are located */
	Xcode_OutputBuffer_t OutputBuffer;			/*	Structure containing the information related
													To the result of the transcoding operation */
	Xcode_AdditionalFlags_t AdditionalFlags;
} Xcode_TransformParams_t;

typedef struct {
  	U32  		UsedOutputBufferSizeInBits;
}XCode_TransformStatusAdditionalInfo_t;

#ifdef LATEST_MME_API_CHANGES

#define  I_16x16_DC_ALLOWED_SHIFT					0
#define  I_16X16_HORIZONTAL_ALLOWED_SHIFT	        1   
#define  I_16X16_VERTICAL_SHIFT						2	     
#define  I_16X16_PLANE_SHIFT						3	
#define  P_16X16_ALLOWED_SHIFT						4
#define  P_16X8_ALLOWED_SHIFT						5
#define  SKIP_ALLOWED_SHIFT							6     
#define  B16X16_L0_ALLOWED_SHIFT					7     
#define  B16X16_L1_ALLOWED_SHIFT					8
#define  B16X8_L0_ALLOWED_SHIFT					    9     
#define  B16X8_L1_ALLOWED_SHIFT                     10
#define  B_BI_ALLOWED_SHIFT							11
#define  DIRECT_ALLOWED_SHIFT						12     

#if 0
typedef enum
{
  I_16x16_DC_ALLOWED					= 1 << I_16x16_DC_ALLOWED_SHIFT ,
  I_16X16_HORIZONTAL_ALLOWED  = 1 << I_16X16_HORIZONTAL_ALLOWED_SHIFT ,
  I_16X16_VERTICAL_ALLOWED		= 1 << I_16X16_VERTICAL_SHIFT,
  I_16X16_PLANE_ALLOWED			= 1 << I_16X16_PLANE_SHIFT,
  P_16X16_ALLOWED						= 1 << P_16X16_ALLOWED_SHIFT,
  P_16X8_ALLOWED						= 1 << P_16X8_ALLOWED_SHIFT,
  SKIP_ALLOWED							= 1 <<  SKIP_ALLOWED_SHIFT,
  B16X16_L0_ALLOWED					= 1 << B16X16_L0_ALLOWED_SHIFT,
  B16X16_L1_ALLOWED					= 1 << B16X16_L1_ALLOWED_SHIFT,
  B16X8_L0_ALLOWED					= 1 <<B16X8_L0_ALLOWED_SHIFT,
  B16X8_L1_ALLOWED					= 1 << B16X8_L1_ALLOWED_SHIFT,
  B_BI_ALLOWED							= 1 << B_BI_ALLOWED_SHIFT,
  DIRECT_ALLOWED						= 1 <<  DIRECT_ALLOWED_SHIFT,
  ALL_MB_TYPES_ALLOWED			= 0xFFFFFFFF
} Xcode_MPEG2H264_AllowedMbTypes_t;
#endif

#define ALL_MB_TYPES_ALLOWED                    0xFFFFFFFF     

/*
** Xcode_MPEG2H264_Debug_t :
** To be used for debug mode only.
*/
typedef struct
{
  U32 AllowedMbTypes;
}Xcode_MPEG2H264_Debug_t;
#endif


#endif /* MP2H264INTERFACE_H */

