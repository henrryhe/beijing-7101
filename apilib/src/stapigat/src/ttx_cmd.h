/*******************************************************************************

File name   : ttx_cmd.h

Description : STTTX commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
14 Jun 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __TTX_CMD_H
#define __TTX_CMD_H


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

#define STTTX_DEVICE_NAME             "TTX"

/* Exported Functions ------------------------------------------------------- */

BOOL TTX_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __TTX_CMD_H */

/* End of ttx_cmd.h */
