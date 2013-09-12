/*******************************************************************************

File name   : cltbx.h

Description : TBX configuration initialisation header file

COPYRIGHT (C) STMicroelectronics 2002.

Date               Modification                                     Name
----               ------------                                     ----
05 Fev 2002        Created                                           HSdLM
09 Dec 2002        TraceBuffer can already be defined in trace.h     HSdLM
*******************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __CLTBX_H
#define __CLTBX_H


/* Includes ----------------------------------------------------------------- */

#include "stddefs.h"
#include "sttbx.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

#define STTBX_DEVICE_NAME  "TBX"

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */
/* TraceBuffer can already be defined in trace.h */
#ifndef TraceBuffer
#define TraceBuffer(x) STTBX_Print(x)
#endif /* TraceBuffer */
/* Exported Functions ------------------------------------------------------- */

BOOL TBX_Init(void);
void TBX_Term(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __CLTBX_H */

/* End of cltbx.h */
