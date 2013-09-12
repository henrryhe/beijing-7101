/*******************************************************************************

File name   : genbuff.h

Description : generic HAL video buffer manager

COPYRIGHT (C) STMicroelectronics 2000.

Date               Modification                                     Name
----               ------------                                     ----
18 Jun 2003        Created                                           PLe
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HAL_VIDEO_BUFFERS_GENERIC_H
#define __HAL_VIDEO_BUFFERS_GENERIC_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

#include "halv_buf.h"


/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */


/* Exported Types ----------------------------------------------------------- */


/* Exported Variables ------------------------------------------------------- */

extern const HALBUFF_FunctionsTable_t HALBUFF_GenbuffFunctions;


/* Exported Macros ---------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------- */



/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HAL_VIDEO_BUFFERS_OMEGA2_H */

/* End of genbuff.h */
