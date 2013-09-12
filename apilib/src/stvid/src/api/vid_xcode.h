/*******************************************************************************

File name   : vid_xcode.h

Description : Video transocde feature header file

COPYRIGHT (C) STMicroelectronics 2007.

Date               Modification                                     Name
----               ------------                                     ----
14 Mar 2007        Created                                           LM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_XCODE_H
#define __VIDEO_XCODE_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "api.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

typedef STVID_EncoderInitParams_t             STVID_TranscoderInitParams_t;
typedef STVID_EncoderTermParams_t             STVID_TranscoderTermParams_t;
typedef STVID_EncoderOpenParams_t             STVID_TranscoderOpenParams_t;
typedef STVID_EncoderCloseParams_t            STVID_TranscoderCloseParams_t;
typedef STVID_EncoderStartParams_t            STVID_TranscoderStartParams_t;
typedef STVID_EncoderStopParams_t             STVID_TranscoderStopParams_t;
typedef STVID_EncoderLinkParams_t             STVID_TranscoderLinkParams_t;
typedef STVID_EncoderUnlinkParams_t           STVID_TranscoderUnlinkParams_t;
typedef STVID_EncoderSetParams_t              STVID_TranscoderSetParams_t;
typedef STVID_EncoderSetMode_t                STVID_TranscoderSetMode_t;
typedef STVID_EncoderHandle_t                 STVID_TranscoderHandle_t;
typedef STVID_EncoderStatistics_t             STVID_TranscoderStatistics_t;
typedef STVID_EncoderErrorRecoveryMode_t      STVID_TranscoderErrorRecoveryMode_t;
typedef STVID_EncoderError_t                  STVID_TranscoderError_t;
typedef STVID_PictureEncodedParam_t           STVID_PictureTranscodedParam_t;

/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */

#define STVID_TranscoderInit                  STVID_EncoderInit
#define STVID_TranscoderTerm                  STVID_EncoderTerm
#define STVID_TranscoderOpen                  STVID_EncoderOpen
#define STVID_TranscoderClose                 STVID_EncoderClose
#define STVID_TranscoderStart                 STVID_EncoderStart
#define STVID_TranscoderStop                  STVID_EncoderStop
#define STVID_TranscoderLink                  STVID_EncoderLink
#define STVID_TranscoderUnlink                STVID_EncoderUnlink
#define STVID_TranscoderSetParams             STVID_EncoderSetParams
#define STVID_TranscoderSetMode               STVID_EncoderSetMode
#define STVID_TranscoderSetDisplayMode        STVID_EncoderSetDisplayMode
#define STVID_TranscoderGetStatistics         STVID_EncoderGetStatistics
#define STVID_TranscoderResetAllStatistics    STVID_EncoderResetAllStatistics
#define STVID_TranscoderSetErrorRecoveryMode  STVID_EncoderSetErrorRecoveryMode
#define STVID_TranscoderGetErrorRecoveryMode  STVID_EncoderGetErrorRecoveryMode
#define STVID_TranscoderInformReadAddress     STVID_EncoderInformReadAddress

/* Exported Functions ------------------------------------------------------- */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_XCODE_H */

/* End of vid_xcode.h */



