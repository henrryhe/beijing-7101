/*******************************************************************************

File name   : vid_err.h

Description : Video error recovery feature header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
 6 Oct 2000        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_ERROR_RECOVERY_H
#define __VIDEO_ERROR_RECOVERY_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "api.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */


ST_ErrorCode_t stvid_InitErrorRecovery(VideoDevice_t * const Device_p, const STVID_ErrorRecoveryMode_t Mode, const ST_DeviceName_t VideoName, const U8 DecoderNumber);
ST_ErrorCode_t stvid_NewVTGErrorRecovery(VideoDevice_t * const Device_p, const ST_DeviceName_t VTGName);
ST_ErrorCode_t stvid_SetSpeedErrorRecovery(VideoDevice_t * const Device_p, const S32 Speed);
ST_ErrorCode_t stvid_StartVideoErrorRecovery(VideoDevice_t * const Device_p, BOOL IsStartInRealTime);
ST_ErrorCode_t stvid_TermErrorRecovery(VideoDevice_t * const Device_p);
#ifdef ST_producer
ST_ErrorCode_t stvid_ReadyToDecodeNewPictureErrorRecovery(VideoDevice_t * const Device_p);
#endif /* ST_producer */

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_ERROR_RECOVERY_H */

/* End of vid_err.h */
