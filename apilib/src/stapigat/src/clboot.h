/*******************************************************************************

File name   : clboot.h

Description : BOOT configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
31 Jan 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLBOOT_H
#define __CLBOOT_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stboot.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

extern STBOOT_DCache_Area_t *DCacheMap_p;
extern size_t               DCacheMapSize;

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

BOOL BOOT_Init(void);
void BOOT_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLBOOT_H */

/* End of clboot.h */
