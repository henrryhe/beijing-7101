/*******************************************************************************

File name   : vid_sc.h

Description : Video strat code controller header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
 4 Dec 2000        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_START_CODE_CONTROLLER_H
#define __VIDEO_START_CODE_CONTROLLER_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "vid_mpeg.h"

#include "vid_dec.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

ST_ErrorCode_t viddec_ProcessStartCode(const VIDDEC_Handle_t DecodeHandle,
                                       const U8 StartCode,
                                       MPEG2BitStream_t * const Stream_p,
                                       STVID_PictureInfos_t * const PictureInfos_p,
                                       StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                       PictureStreamInfo_t * const PictureStreamInfo_p,
                                       VIDDEC_SyntaxError_t * const SyntaxError_p,
                                       U32 * const TemporalReferenceOffset_p);
#ifdef ST_SecondInstanceSharingTheSameDecoder
ST_ErrorCode_t ComputeSequenceInformation(const MPEG2BitStream_t * const CurrentStream_p,
                                                 STVID_SequenceInfo_t * const SequenceInfo_p,
                                                 VIDDEC_SyntaxError_t * const SyntaxError_p);

ST_ErrorCode_t FillPictureParamsStructuresForStill(  StillDecodeParams_t StillDecodeParams ,
                                    STVID_SequenceInfo_t * const SequenceInfo_p,
                                        const MPEG2BitStream_t * const Stream_p,
                                        STVID_PictureInfos_t * const PictureInfos_p,
                                        StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                        PictureStreamInfo_t * const PictureStreamInfo_p);
#endif /* ST_SecondInstanceSharingTheSameDecoder */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_START_CODE_CONTROLLER_H */

/* End of vid_sc.h */
