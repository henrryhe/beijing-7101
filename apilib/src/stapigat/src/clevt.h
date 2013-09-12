/*******************************************************************************

File name   : clevt.h

Description : EVT configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                           HSdLM
07 May 2003        Remove dependency upon stevt.h                    HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLEVT_H
#define __CLEVT_H


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

#define STEVT_DEVICE_NAME             "EVT"

/* Exported Functions ------------------------------------------------------- */

BOOL EVT_Init(void);
void EVT_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLEVT_H */

/* End of clevt.h */
