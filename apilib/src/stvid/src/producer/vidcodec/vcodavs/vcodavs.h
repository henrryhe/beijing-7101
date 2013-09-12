/*******************************************************************************
File Name   : vcodavs.h

Description : Video driver CODEC AVS header file

COPYRIGHT (C) STMicroelectronics 2008

Date               Modification                                     Name
----               ------------                                     ----
29 January 2008    First STAPI version                              PLE
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STVIDCOD_AVS_H
#define __STVIDCOD_AVS_H


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

extern const DECODER_FunctionsTable_t   DECODER_AVSFunctions;
extern const PARSER_FunctionsTable_t    PARSER_AVSFunctions;

/* Definition of Picture Distance */
#define PICTURE_DISTANCE_MODULO       256
/* Exported Types ----------------------------------------------------------- */

typedef enum AVS_Profile_s
{
    AVS_BASELINE_PROFILE = 0,
    AVS_ENHANCED_PROFILE = 1,
    AVS_RESERVED_PROFILE = 2
}AVS_Profile_t;


typedef enum AVS_PictureType_s
{
    AVS_I_VOP = 0,
    AVS_P_VOP = 1,
    AVS_B_VOP = 2
}AVS_PictureType_t;


typedef struct AVS_GlobalDecodingContextSpecificData_s
{
    U8                      QuantizedFrameRateForPostProcessing;
    U8                      QuantizedBitRateForPostProcessing;
    BOOL                    PostProcessingFlag;
    BOOL                    PulldownFlag;
    BOOL                    InterlaceFlag;
    BOOL                    TfcntrFlag;
    BOOL                    FinterpFlag;
    BOOL                    ProgressiveSegmentedFrame;
    U8                      AspectNumerator;
    U8                      AspectDenominator;
    U8                      NumLeakyBuckets;
    U8                      BitRateExponent;
    U8                      BufferSizeExponent;
    BOOL                    AebrFlag; /* Anti-Emulation byte removal -- not in the sequence header.
                                         Should this be here or in picture specific data ?*/
    /* The following is only for simple and main profile. */
    U32                     ABSHrdBuffer;
    U32                     ABSHrdRate;
    BOOL                    MultiresFlag;
    BOOL                    SyncmarkerFlag;
    BOOL                    RangeredFlag;
    U8                      Maxbframes;
    BOOL                    FirstPictureEver;
    U32                     ExtendedPictureDistance;
    U32                     PictureDistanceOffset;
    BOOL                    OnePictureDistanceOverflowed;
    U32                     PreviousGreatestExtendedPictureDistance;
    U32                     PictureCounter;
} AVS_GlobalDecodingContextSpecificData_t;


typedef struct AVS_PictureDecodingContextSpecificData_s
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

} AVS_PictureDecodingContextSpecificData_t;


typedef struct AVS_SequenceError_s
{
   BOOL ProfileAndLevel;
} AVS_SequenceError_t;


typedef struct AVS_PicError_s
{
   BOOL CodedVOP;
} AVS_PicError_t;


typedef struct AVS_VEDError_s
{
   BOOL ClosedGOV;
   BOOL BrokenLink;
} AVS_VEDError_t;


typedef struct AVS_PictureError_s
{
   AVS_SequenceError_t SeqError;
   AVS_PicError_t PicError;
     AVS_VEDError_t VEDError;
} AVS_PictureError_t;


typedef struct AVS_PictureSpecificData_s
{
    U8 RndCtrl;
    U32 BackwardRefDist;
    AVS_PictureType_t PictureType;
    BOOL ForwardRangeredfrmFlag;
    BOOL HalfWidthFlag;
    BOOL HalfHeightFlag;
    BOOL IsReferenceTopFieldP0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
    BOOL IsReferenceTopFieldB0[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
    BOOL IsReferenceTopFieldB1[VIDCOM_MAX_NUMBER_OF_REFERENCES_FIELDS];
    U32  FramePictureLayerSize;
    BOOL TffFlag;
    U32  RefDist;
  AVS_PictureError_t PictureError;
    U32 ProgressiveSequence;
/*For AVS FW R2 Get those information from PictureHeader_t*/
  U8 Progressive_frame; /*Progressive_frame*/
    U8 PictureCodingType; /*frame_type*/
    U8 PictureDistance; /*tr*/
    U8 PictureStructure; /*picture_struct*/
    U8 FixedPictureQP; /*Fixed_picture_qp*/
    U8 PictureQP; /*Picture_qp*/
    U8 PictureReferenceFlag; /*picture_ref_flag*/
    U8 SkipModeFlag; /*Skip_mode_flag*/
    U8 LoopFilterDisable; /*loop_filter_disable*/
    U8 LoopFilterParameterFlag;
    U8 AlphaCOffset; /*alpha_offset*/
    U8 BetaOffset; /*beta_offset*/
    U32 TopFieldPos; /*topfield_pos*/
    U32 BotFieldPos; /*botfield_pos*/
} AVS_PictureSpecificData_t;


typedef struct AVS_DECODERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} AVS_DECODERStatistics_t;


typedef struct AVS_PARSERStatistics_s
{
    /* TBD !!! */
    BOOL ProcessedPicturesNumber;
} AVS_PARSERStatistics_t;

/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */
#ifdef VALID_TOOLS
ST_ErrorCode_t AVSPARSER_RegisterTrace(void);
ST_ErrorCode_t AVSDECODER_RegisterTrace(void);
#endif /* VALID_TOOLS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifndef __STVIDCOD_AVS_H */

/* End of vcodavs.h */
