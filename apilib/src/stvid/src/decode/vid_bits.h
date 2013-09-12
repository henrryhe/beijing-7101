/*******************************************************************************

File name   : vid_bits.h

Description : Header data management header file

COPYRIGHT (C) STMicroelectronics 2004

Date               Modification                                     Name
----               ------------                                     ----
22 Feb 2000        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VID_MPEG_BITS_H
#define __VID_MPEG_BITS_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

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

#ifndef ST_OSWINCE
__inline void viddec_HeaderDataResetBitsCounter(HeaderData_t * const HeaderData_p);
__inline U32 viddec_HeaderDataGetBitsCounter(HeaderData_t * const HeaderData_p);
__inline U32 viddec_HeaderDataGetMissingBitsCounter(HeaderData_t * const HeaderData_p);
#else
void viddec_HeaderDataResetBitsCounter(HeaderData_t * const HeaderData_p);
U32 viddec_HeaderDataGetBitsCounter(HeaderData_t * const HeaderData_p);
U32 viddec_HeaderDataGetMissingBitsCounter(HeaderData_t * const HeaderData_p);
#endif
U32 viddec_HeaderDataGetBits(HeaderData_t * const HeaderData_p, U32 NbRequestedBits);
U8 viddec_HeaderDataGetStartCode(HeaderData_t * const HeaderData_p);
BOOL viddec_SearchAndGetNextStartCode(HeaderData_t * const HeaderData_p,
                                      const U8 * const CPUSearchAddressStart_p,
                                      const U32 MaxByteSearchMinusFour,
                                      U8 * const StartCode_p,
                                      const U8 ** StartCodeAddress_p);
#ifdef ST_SecondInstanceSharingTheSameDecoder
BOOL viddec_FindOneSequenceAndPicture(HeaderData_t * const HeaderData_p,
                                      const BOOL SearchThroughHeaderDataNotCPU,
                                      const U8 * const CPUSearchAddressStart_p,
                                      MPEG2BitStream_t * const Stream_p,
                                      STVID_PictureInfos_t * const PictureInfos_p,
                                      StreamInfoForDecode_t * const StreamInfoForDecode_p,
                                      PictureStreamInfo_t * const PictureStreamInfo_p,
                                      VIDDEC_SyntaxError_t * const SyntaxError_p,
                                      U32 * const TemporalReferenceOffset_p,
                                      void ** PictureStartAddress_p);
#endif /* ST_SecondInstanceSharingTheSameDecoder */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VID_MPEG_BITS_H */

/* End of vid_bits.h */
