/*******************************************************************************

File name   : vbi_cmd.h

Description : STVBI commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
14 Jun 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VBI_CMD_H
#define __VBI_CMD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STVBI_DEVICE_NAME             "VBI"

/* Exported Functions ------------------------------------------------------- */

BOOL VBI_RegisterCmd(void);
BOOL VBI_TextError(ST_ErrorCode_t Error, char *Text);
/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VBI_CMD_H */

/* End of vbi_cmd.h */
