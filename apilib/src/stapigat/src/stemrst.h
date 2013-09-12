/*******************************************************************************

File name   : stemrst.h

Description : STEM reset configuration header file

COPYRIGHT (C) STMicroelectronics 2003.

Date               Modification                                     Name
----               ------------                                     ----
02 Apr 2003        Created                                          HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __STEM_RESET_H
#define __STEM_RESET_H


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

BOOL StemReset_Init(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __STEM_RESET_H */

/* End of stemrst.h */
