/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : vid_cmd.h
Description : Used STAUD definition

Date          Modification
----          ------------
June 2002    Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __AUD_CMD_H
#define __AUD_CMD_H

/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Demux statistics structure */
/* -------------------------- */
/* External declarations */
/* --------------------- */
/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */


#define STAUD_DEVICE_NAME1              "AUD1"

/* Exported Functions ------------------------------------------------------- */

BOOL AUD_RegisterCmd(void);
BOOL AUD_InitComponent(void);
BOOL MBX_Init(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __AUD_CMD_H */
