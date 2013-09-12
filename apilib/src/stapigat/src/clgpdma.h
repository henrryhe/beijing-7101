/*******************************************************************************

File name   : clgpdma.h

Description : GPDMA configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                           HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLGPDMA_H
#define __CLGPDMA_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stgpdma.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

#define STGPDMA_DEVICE_NAME             "GPDMA"

/* Exported Functions ------------------------------------------------------- */

BOOL GPDMA_UseInit(void);
void GPDMA_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLGPDMA_H */

/* End of ctgpdma.h */
