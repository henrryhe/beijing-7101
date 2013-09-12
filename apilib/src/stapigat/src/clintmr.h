/*******************************************************************************

File name   : clintmr.h

Description : INTMR configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
16 Apr 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLINTMR_H
#define __CLINTMR_H


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

#define STINTMR_DEVICE_NAME             "INTMR"

/* Exported Functions ------------------------------------------------------- */

BOOL INTMR_Init(void);
void INTMR_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLINTMR_H */

/* End of clintmr.h */
