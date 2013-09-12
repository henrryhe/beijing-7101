/*******************************************************************************

File name   : hdmi_cmd.h

Description : STHDMI commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2005.

Date               Modification                                     Name
----               ------------                                     ----
28 Feb 2005        Created                                          AC
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __HDMI_CMD_H
#define __HDMI_CMD_H


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

#define STHDMI_DEVICE_NAME             "HDMI"

/* Exported Functions ------------------------------------------------------- */
BOOL HDMI_TextError(ST_ErrorCode_t Error, char *Text);
BOOL HDMI_DoTextError(ST_ErrorCode_t ErrorCode, char *Text);
BOOL HDMI_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __HDMI_CMD_H */

/* End of HDMI_cmd.h */

