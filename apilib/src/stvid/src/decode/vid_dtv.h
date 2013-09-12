/*******************************************************************************

File name   : vid_dtv.h

Description : Directv standard header file

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
14 Jun 2000        Created                                           HG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIDEO_DIRECTV_STD_H
#define __VIDEO_DIRECTV_STD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "stvid.h"

#include "vid_mpeg.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */

void ProcessPictureUserDataDirectv(const UserData_t * const UserData_p,
                            STVID_PTS_t * const PTS_p,
                            PictureCodingExtension_t * const PictureCodingExtension_p,
                            PictureDisplayExtension_t * const PictureDisplayExtension_p
                            );

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIDEO_DIRECTV_STD_H */

/* End of vid_dtv.h */
