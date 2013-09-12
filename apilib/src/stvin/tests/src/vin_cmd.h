/*******************************************************************************

File name   : vin_cmd.h

Description : STVIN commands configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
17 Jul 2002        Created                                           GG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __VIN_CMD_H
#define __VIN_CMD_H


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

#define STVIN_DEVICE_NAME             "VIN"

/* Exported Functions ------------------------------------------------------- */

BOOL VIN_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __VIN_CMD_H */

/* End of vin_cmd.h */
