/*******************************************************************************

File name   : clpwm.h

Description : PWM configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLPWM_H
#define __CLPWM_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stpwm.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STPWM_DEVICE_NAME             "PWM"

/* Exported Functions ------------------------------------------------------- */

BOOL PWM_Init(void);
void PWM_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLPWM_H */

/* End of clpwm.h */
