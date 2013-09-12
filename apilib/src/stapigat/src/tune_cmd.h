/************************************************************************
COPYRIGHT (C) STMicroelectronics 2002

File name   : tune_cmd.h

Description : Used TUNER definition

Date          Modification                                    Initials
----          ------------                                    --------
28 Aug 2002   Creation                                        BS

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __TUNER_CMD_H
#define __TUNER_CMD_H

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

#define STTUNER_DEVICE_NAME "TUNER"

/* Exported Functions ------------------------------------------------------- */

BOOL TUNER_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __TUNER_CMD_H */
