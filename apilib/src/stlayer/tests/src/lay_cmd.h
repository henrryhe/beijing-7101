/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : lay_cmd.h
Description : Used layers definition

Date          Modification
----          ------------
April 2001    Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __LAY_CMD_H
#define __LAY_CMD_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stgxobj.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STLAYER_GFX_DEVICE_NAME "LAYGFX"
#define STLAYER_VID_DEVICE_NAME "LAYVID"

/* Exported Functions ------------------------------------------------------- */

BOOL LAYER_RegisterCmd(void);
BOOL LAYER_TextError(ST_ErrorCode_t Error, char *Text);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __LAY_CMD_H */
