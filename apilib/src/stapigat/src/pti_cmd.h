/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : pti_cmd.h
Description : Used PTI definition

Date          Modification
----          ------------
April 2001    Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __PTI_CMD_H
#define __PTI_CMD_H

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

#define STPTI_DEVICE_NAME "PTI"

/* Exported Functions ------------------------------------------------------- */

BOOL PTI_RegisterCmd(void);
BOOL PTI_InitComponent(void);
void PTI_TermComponent(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __PTI_CMD_H */
