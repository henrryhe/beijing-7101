/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : link_cmd.h
Description : Used LINK definition

Date          Modification
----          ------------
June 2002    Creation
************************************************************************/
/* Define to prevent recursive inclusion */
#ifndef __LINK_CMD_H
#define __LINK_CMD_H

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

/* Exported Functions ------------------------------------------------------- */

BOOL PTI_RegisterCmd(void);
BOOL PTI_InitComponent(void);
void PTI_TermComponent(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __LINK_CMD_H */
