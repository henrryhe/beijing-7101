/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : demux_cmd.h
Description : Used DEMUX definition

Date          Modification
----          ------------
Feb 2006      Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __DEMUX_CMD_H
#define __DEMUX_CMD_H

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

#define STDEMUX_DEVICE_NAME "DEMUX"

/* Exported Functions ------------------------------------------------------- */

BOOL DEMUX_RegisterCmd(void);
BOOL DEMUX_InitComponent(void);
void DEMUX_TermComponent(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __DEMUX_CMD_H */
