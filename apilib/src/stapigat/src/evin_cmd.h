/*******************************************************************************

File name   : evin_cmd.h

Description : EXTVIN commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
17 Jul 2002        Created                                           GG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __EVIN_CMD_H
#define __EVIN_CMD_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "evin_cmd.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

BOOL EXTVIN_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __EVIN_CMD_H */

/* End of evin_cmd.h */
