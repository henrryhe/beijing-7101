/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : vid_cmd.h
Description : Used STVID definition

Date          Modification
----          ------------
June 2002    Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __VID_CMD_H
#define __VID_CMD_H

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

#define STVID_DEVICE_NAME1              "VID1"
#define STVID_DEVICE_NAME2              "VID2"
#define STVID_XCODER_NAME1              "XCOD1"

/* Exported Functions ------------------------------------------------------- */

BOOL VID_RegisterCmd(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __VID_CMD_H */
