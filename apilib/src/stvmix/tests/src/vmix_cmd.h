/*******************************************************************************

File name   : vmix_cmd.h

Description : STVMIX commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
10 Jun 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VMIX_CMD_H
#define __VMIX_CMD_H


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

#define STVMIX_DEVICE_NAME             "VMIX"

/* Exported Functions ------------------------------------------------------- */

BOOL VMIX_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VMIX_CMD_H */

/* End of vmix_cmd.h */
