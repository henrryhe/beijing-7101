/*******************************************************************************

File name   : clfdma.h

Description : FDMA configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
20 Nov 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLFDMA_H
#define __CLFDMA_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stfdma.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STFDMA_DEVICE_NAME             "FDMA"

/* Exported Functions ------------------------------------------------------- */

BOOL FDMA_Init(void);
void FDMA_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLFDMA_H */

/* End of clfdma.h */
