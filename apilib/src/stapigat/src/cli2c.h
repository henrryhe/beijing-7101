/*******************************************************************************

File name   : cli2c.h

Description : I2C configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLI2C_H
#define __CLI2C_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sti2c.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STI2C_FRONT_DEVICE_NAME       "I2CFRONT"
#define STI2C_BACK_DEVICE_NAME        "I2CBACK"

/* Exported Functions ------------------------------------------------------- */

BOOL I2C_Init(void);
void I2C_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLI2C_H */

/* End of cli2c.h */
