/*******************************************************************************

File name   : vout_cmd.h

Description : STVOUT commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
24 Apr 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VOUT_CMD_H
#define __VOUT_CMD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "denc_cmd.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STVOUT_DEVICE_NAME             "VOUT"

/* Exported Functions ------------------------------------------------------- */
BOOL VOUT_TextError(ST_ErrorCode_t Error, char *Text);
BOOL VOUT_DotTextError(ST_ErrorCode_t ErrorCode, char *Text);
BOOL VOUT_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VOUT_CMD_H */

/* End of vout_cmd.h */
