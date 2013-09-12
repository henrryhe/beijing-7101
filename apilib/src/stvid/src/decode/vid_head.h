/*******************************************************************************

File name   : vid_head.h

Description : MPEG headers parsing header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
06 Jan 2000        Created                                          HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_HEADER_PARSING_H
#define __VIDEO_HEADER_PARSING_H


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

typedef enum
{
  PARSER_MODE_STREAM_ANALYSER,  /* Parses only start codes found after sequence start code              */
  PARSER_MODE_STREAM_PARSER     /* Parses all start codes (even those before sequence start code)       */
} ParseMode_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

void viddec_ResetStream(MPEG2BitStream_t * const Stream_p);
void viddec_FoundStartCode(const VIDDEC_Handle_t DecodeHandle, const U8 StartCode);
ST_ErrorCode_t viddec_ParseStartCode(const VIDDEC_Handle_t DecodeHandle, const U8 StartCode, MPEG2BitStream_t * const Stream_p, const ParseMode_t ParseMode);
#ifdef ST_SecondInstanceSharingTheSameDecoder
ST_ErrorCode_t viddec_ParseStartCodeForStill(HeaderData_t *  HeaderData_p, const U8 StartCode, MPEG2BitStream_t * const Stream_p, const ParseMode_t ParseMode);
#endif /* ST_SecondInstanceSharingTheSameDecoder */


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_HEADER_PARSING_H */

/* End of vid_head.h */
