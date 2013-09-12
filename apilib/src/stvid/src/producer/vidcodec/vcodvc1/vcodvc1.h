/*******************************************************************************
File Name   : vcodvc1.h

Description : Video driver CODEC VC1 header file

Copyright (C) 2004 STMicroelectronics

Date               Modification                                     Name
----               ------------                                     ----
21 Jul 2005        Created                                           PC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDCOD_VC1_H
#define __STVIDCOD_VC1_H


/* Includes ----------------------------------------------------------------- */
#include "stlite.h"
#include "stddefs.h"

#include "stevt.h"
#include "stavmem.h"

#include "stgxobj.h"
#include "stlayer.h"

#include "stvid.h"

#include "vidcodec.h"
#ifdef NATIVE_CORE
    #define VC9_MME_VERSION 14
#else
    #define VC9_MME_VERSION 14
#endif    
#include "VC9_VideoTransformerTypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Exported Constants ------------------------------------------------------- */

extern const DECODER_FunctionsTable_t   DECODER_VC1Functions;
extern const PARSER_FunctionsTable_t    PARSER_VC1Functions;

#define MAX_NUM_LEAKY_BUCKETS 31 /*from vc1 spec, 6.1.15.1 HRD_PARAM*/


/* Exported Types ----------------------------------------------------------- */

typedef enum VC1_Profile_s
{
	VC1_SIMPLE_PROFILE = 0,
	VC1_MAIN_PROFILE = 1,
	VC1_RESERVED_PROFILE = 2,
	VC1_ADVANCED_PROFILE = 3
}VC1_Profile_t; 


typedef struct VC1_HRD_Buffer_Param_s
{
	U16 HrdRate;
	U16 HrdBuffer;

}VC1_HRD_Buffer_Param_t;

typedef struct VC1_SeqHdrError_s
{
	BOOL ProfileOutOfRange;
	BOOL LevelOutOfRange;
	BOOL ColorDiffFormatOutOfRange;
	BOOL MaxCodedWidthOutOfRange;
    BOOL MaxCodedHeightOutOfRange;
	BOOL ReservedBitOutOfRange;
	BOOL SampleAspectRatioOutOfRange;
	BOOL FrameRateNROutOfRange;
	BOOL FrameRateDROutOfRange;
	BOOL ColorPrimariesOutOfRange;
	BOOL TransferCharOutOfRange;
	BOOL MatrixCoefOutOfRange;
	BOOL NoC5inRCVFormat;
	BOOL No00000004inRCVFormat;
	BOOL FrameRateOutOfRange;
	BOOL RES1OutOfRange;
	BOOL RES2OutOfRange;
	BOOL RES3OutOfRange;
	BOOL RES4OutOfRange;
	BOOL RES5OutOfRange;
	BOOL RES6OutOfRange;
	BOOL RES7OutOfRange;
	BOOL LoopFilterFlagSPOutOfRange;
	BOOL SyncMarkerFlagSPOutOfRange;
	BOOL RangeredFlagSPOutOfRange;
	BOOL No0000000CinRCVFormat;
	
}VC1_SeqHdrError_t;

typedef struct VC1_GlobalDecodingContextSpecificData_s
{
	U8						QuantizedFrameRateForPostProcessing;
	U8						QuantizedBitRateForPostProcessing;
	BOOL					PostProcessingFlag;
	BOOL					PulldownFlag;
	BOOL					InterlaceFlag;
	BOOL					TfcntrFlag;
	BOOL					FinterpFlag;
	BOOL					ProgressiveSegmentedFrame;
	U8						AspectNumerator;
	U8						AspectDenominator;
	U8						NumLeakyBuckets;
	U8						BitRateExponent;
	U8						BufferSizeExponent;	
	VC1_HRD_Buffer_Param_t	HrdBufferParams[MAX_NUM_LEAKY_BUCKETS];
	BOOL					AebrFlag; /* Anti-Emulation byte removal -- not in the sequence header.
										 Should this be here or in picture specific data ?*/
	/* The following is only for simple and main profile. */
	U32						ABSHrdBuffer;
	U32						ABSHrdRate;
	BOOL					MultiresFlag;
	BOOL					SyncmarkerFlag;
	BOOL					RangeredFlag;
	U8						Maxbframes;
	VC1_SeqHdrError_t       SeqHdrError;
} VC1_GlobalDecodingContextSpecificData_t;

typedef struct VC1_EntryPointHdrError_s
{
	BOOL BrokenLinkOutOfRange;
	BOOL CodedWidthOutOfRange;
	BOOL CodedHeightOutOfRange;

}VC1_EntryPointHdrError_t;

typedef struct VC1_PictureDecodingContextSpecificData_s
{
	BOOL PanScanFlag;	
	BOOL RefDistFlag;
	BOOL LoopFilterFlag;	
	BOOL Fastuvmc;
	BOOL ExtendedMVFlag;
	char Dquant;
	BOOL VstransformFlag;
	BOOL OverlapFlag;
	char Quantizer;
	char HRDFullness[MAX_NUM_LEAKY_BUCKETS];
	U16  CodedWidth;
	U16  CodedHeight;
	BOOL ExtendedDMVFlag;
	BOOL Range_MAPYFlag;
	char Range_MAPY;
	BOOL Range_MAPUVFlag;
	char Range_MAPUV;
	VC1_EntryPointHdrError_t	EntryPointHdrError;

} VC1_PictureDecodingContextSpecificData_t;


typedef struct VC1_PictureError_s
{
	/* If this flag is set then there were previous pictures that
	   were not passed to the producer because they had missing
	   refrence pictures. */
	BOOL MissingReferenceInformation;

	 /* Before this picture that is being returned, a second field occurred without
	    a prior first field. This also is used when two second fields in a row are
	    received. */
	BOOL SecondFieldWithoutFirstFieldError;

	/* Before this picture that is being returned, a first field occurred without
	    a corresponding second field. */
	BOOL NoSecondFieldAfterFirstFieldError;

	BOOL RefDistOutOfRange;
	BOOL BFractionOutOfRange;
	BOOL RESOutOfRange;
    
}VC1_PictureError_t;

typedef struct VC1_PictureSpecificData_s
{
	VC9_IntensityComp_t IntComp;
	U8 RndCtrl;
	U32  NumeratorBFraction;
	U32  DenominatorBFraction;
	U32 BackwardRefDist; 
	VC9_PictureType_t PictureType;
	BOOL ForwardRangeredfrmFlag;
	BOOL HalfWidthFlag;
	BOOL HalfHeightFlag;
	U8   NumSlices;
	VC9_SliceParam_t SliceArray[MAX_SLICE];
	BOOL IsReferenceTopFieldP0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
	BOOL IsReferenceTopFieldB0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
	BOOL IsReferenceTopFieldB1[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
	U32  FramePictureLayerSize;
	BOOL TffFlag;
	BOOL SecondFieldFlag;
	U32  RefDist;
	VC9_PictureSyntax_t PictureSyntax;
	VC9_PictureSyntax_t ForwardReferencePictureSyntax;  /* Picture Syntax for the forward reference picture  */
	VC9_PictureSyntax_t BackwardReferencePictureSyntax; /* Picture Syntax for the backward reference picture */
	VC1_PictureError_t  PictureError;
} VC1_PictureSpecificData_t;

typedef struct VC1_DECODERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} VC1_DECODERStatistics_t;


typedef struct VC1_PARSERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} VC1_PARSERStatistics_t;

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
#ifdef VALID_TOOLS
ST_ErrorCode_t VC1PARSER_RegisterTrace(void);
ST_ErrorCode_t VC1DECODER_RegisterTrace(void);
#endif /* VALID_TOOLS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDCOD_VC1_H */

/* End of vcodvc1.h */
