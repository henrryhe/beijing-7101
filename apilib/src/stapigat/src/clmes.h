/*******************************************************************************

File name   : clmes.h

Description : MES configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2007

Date               Modification                                     Name
----               ------------                                     ----
10 Jul 2007        Created                                          OG + DG
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLMES_H
#define __CLMES_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "stmes.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

BOOL MES_Init(void);
void MES_Term(void);
BOOL MES_RegisterCmd(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLMES_H */

/* End of clmes.h */
