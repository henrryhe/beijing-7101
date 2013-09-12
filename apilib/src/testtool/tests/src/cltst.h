/*******************************************************************************

File name   : cltst.h

Description : Testtool configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
09 Apr 2002        Created                                          HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLTST_H
#define __CLTST_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "testtool.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STTST_DEVICE_NAME  "TESTTOOL"

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

BOOL TST_Init(const char * DefaultScript);
void TST_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLTST_H */

/* End of cltst.h */
