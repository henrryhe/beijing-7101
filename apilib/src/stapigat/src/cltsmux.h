/*******************************************************************************

File name   : cltsmux.h

Description : GPDMA configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
15 Jul 2002        Created                                           FQ
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLTSMUX_H
#define __CLTSMUX_H


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

#define STTSMUX_DEVICE_NAME             "TSMUX"

/* Exported Functions ------------------------------------------------------- */

BOOL TSMUX_Init(void);
void TSMUX_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLTSMUX_H */

/* End of cttsmux.h */
