/*!
 ************************************************************************
 * \file mpg2toh264.h
 *
 * \brief Transcoder MPEG2 to H264 data structures and functions prototypes
 *
 * \author
 * Olivier Deygas \n
 * CMG/STB \n
 * Copyright (C) 2004 STMicroelectronics
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion */

#ifndef __MPG2TOH264_H
#define __MPG2TOH264_H

/* Includes ----------------------------------------------------------------- */
#include "transcodec.h"

#include "stfdma.h"
#include "inject.h"

#ifdef ST_OSLINUX
#include "compat.h"
#else

#ifndef STTBX_REPORT
    #define STTBX_REPORT
#endif
#ifndef STTBX_PRINT
    #define STTBX_PRINT
#endif
#include "sttbx.h"
#include "stsys.h"
#endif

#include "vid_ctcm.h"

/* Constants ------------------------------------------------------- */


/* Exported types ----------------------------------------------------------- */
extern const SOURCE_FunctionsTable_t   SOURCE_MPG2Functions;
extern const TARGET_FunctionsTable_t TARGET_H264Functions;
extern const XCODE_FunctionsTable_t XCODE_MPG2TOH264Functions;

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */
/* Source decoder */
ST_ErrorCode_t mpg2toh264Source_Init(const SOURCE_Handle_t SourceHandle, const SOURCE_InitParams_t * const InitParams_p);
ST_ErrorCode_t mpg2toh264Source_Start(const SOURCE_Handle_t SourceHandle, const SOURCE_StartParams_t * const StartParams_p);
ST_ErrorCode_t mpg2toh264Source_UpdateAndGetRefList(const SOURCE_Handle_t SourceHandle, SOURCE_UpdateAndGetRefListParams_t * const SOURCE_UpdateAndGetRefListParams_p);
BOOL mpg2toh264Source_IsPictureToBeSkipped(const SOURCE_Handle_t SourceHandle,
                                           const U32 CurrentPictureDOFID,
                                           const STVID_MPEGFrame_t MPEGFrame,
                                           const U32 SourceFrameRate,
                                           const U32 TargetFrameRate);
ST_ErrorCode_t mpg2toh264Source_Term(const SOURCE_Handle_t SourceHandle);

/* Target Decoder */
ST_ErrorCode_t mpg2toh264Target_Init(const TARGET_Handle_t TargetHandle, const TARGET_InitParams_t * const InitParams_p);
ST_ErrorCode_t mpg2toh264Target_Start(const TARGET_Handle_t TargetHandle, const TARGET_StartParams_t * const StartParams_p);
ST_ErrorCode_t mpg2toh264Target_Term(const TARGET_Handle_t TargetHandle);
U32 mpg2toh264Target_GetPictureMaxSizeInByte(const U32 PictureWidth, const U32 PictureHeight, const U32 TargetFrameRate, const U32 TargetBitRate);

/* Xcode Transformer */
ST_ErrorCode_t mpg2toh264Xcode_Init(const XCODE_Handle_t XcodeHandle, const XCODE_InitParams_t * const InitParams_p);
ST_ErrorCode_t mpg2toh264Xcode_TranscodePicture(const XCODE_Handle_t XcodeHandle, const XCODE_TranscodePictureParams_t * const TranscodePictureParams_p);
ST_ErrorCode_t mpg2toh264Xcode_Term(const XCODE_Handle_t XcodeHandle);

#endif /* #ifdef __MPG2TOH264_H */

