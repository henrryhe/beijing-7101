/*******************************************************************************
File Name   : vcodmpg4p2.h

Description : Video driver CODEC MPEG4P2 header file

Copyright (C) 2004 STMicroelectronics

Date               Modification                                     Name
----               ------------                                     ----
30 Nov 2005        Created                                           PC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDCOD_MPG4P2_H
#define __STVIDCOD_MPG4P2_H


/* Includes ----------------------------------------------------------------- */
#include "stlite.h"
#include "stddefs.h"

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

extern const DECODER_FunctionsTable_t   DECODER_MPEG4P2Functions;
extern const PARSER_FunctionsTable_t    PARSER_MPEG4P2Functions;


/* Exported Types ----------------------------------------------------------- */

typedef enum MPEG4P2_Profile_s
{
	MPEG4P2_SIMPLE_PROFILE = 0,
	MPEG4P2_MAIN_PROFILE = 1,
	MPEG4P2_RESERVED_PROFILE = 2,
	MPEG4P2_ADVANCED_PROFILE = 3
}MPEG4P2_Profile_t; 


typedef enum MPEG4P2_PictureType_s
{
	MPEG4P2_I_VOP = 0,
	MPEG4P2_P_VOP = 1,
	MPEG4P2_B_VOP = 2,
	MPEG4P2_S_VOP = 3
}MPEG4P2_PictureType_t; 


typedef struct MPEG4P2_GlobalDecodingContextSpecificData_s
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
	BOOL					AebrFlag; /* Anti-Emulation byte removal -- not in the sequence header.
										 Should this be here or in picture specific data ?*/
	/* The following is only for simple and main profile. */
	U32						ABSHrdBuffer;
	U32						ABSHrdRate;
	BOOL					MultiresFlag;
	BOOL					SyncmarkerFlag;
	BOOL					RangeredFlag;
	U8						Maxbframes;
} MPEG4P2_GlobalDecodingContextSpecificData_t;


typedef struct MPEG4P2_PictureDecodingContextSpecificData_s
{
	BOOL PanScanFlag;	
	BOOL RefDistFlag;
	BOOL LoopFilterFlag;	
	char Dquant;
	BOOL VstransformFlag;
	BOOL OverlapFlag;
	char Quantizer;
	U16  CodedWidth;
	U16  CodedHeight;

} MPEG4P2_PictureDecodingContextSpecificData_t;


typedef struct MPEG4P2_SequenceError_s
{
   BOOL ProfileAndLevel;
} MPEG4P2_SequenceError_t;


typedef struct MPEG4P2_VOLError_s
{
   BOOL AspectRatio;
} MPEG4P2_VOLError_t;


typedef struct MPEG4P2_VOPError_s
{
   BOOL CodedVOP;
} MPEG4P2_VOPError_t;


typedef struct MPEG4P2_GOVError_s
{
   BOOL ClosedGOV;
   BOOL BrokenLink;
} MPEG4P2_GOVError_t;


typedef struct MPEG4P2_PictureError_s
{
   MPEG4P2_SequenceError_t SeqError;
   MPEG4P2_VOLError_t VOLError;
   MPEG4P2_VOPError_t VOPError;
	MPEG4P2_GOVError_t GOVError;
} MPEG4P2_PictureError_t;


typedef struct MPEG4P2_PictureSpecificData_s
{
	U8 RndCtrl;
	U32 BackwardRefDist; 
	MPEG4P2_PictureType_t PictureType;
	BOOL ForwardRangeredfrmFlag;
	BOOL HalfWidthFlag;
	BOOL HalfHeightFlag;
	BOOL IsReferenceTopFieldP0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
	BOOL IsReferenceTopFieldB0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
	BOOL IsReferenceTopFieldB1[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
	U32  FramePictureLayerSize;
	BOOL TffFlag;
	U32  RefDist;
   MPEG4P2_PictureError_t PictureError;
	U32 CodecVersion;
} MPEG4P2_PictureSpecificData_t;


typedef struct MPEG4P2_DECODERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} MPEG4P2_DECODERStatistics_t;


typedef struct MPEG4P2_PARSERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} MPEG4P2_PARSERStatistics_t;

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
#ifdef VALID_TOOLS
ST_ErrorCode_t MPEG4P2PARSER_RegisterTrace(void);
ST_ErrorCode_t MPEG4P2DECODER_RegisterTrace(void);
#endif /* VALID_TOOLS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDCOD_MPG4P2_H */

/* End of vcodmpg4p2.h */
