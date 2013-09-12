/*****************************************************************************

File name   : trace.h

Description : visible functions available for development purpose

COPYRIGHT (C) ST-Microelectronics 2002.

Date               Modification                 Name      Origin
----               ------------                 ----      ------
5-sept-00          Ported to 5510               PLe       develop
*****************************************************************************/

/* Define to prevent recursive inclusion */

#ifndef __TRACE_H
#define __TRACE_H

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

#define TraceBuffer(x) trace x

/* Exported Functions ------------------------------------------------------- */
void            trace (const char *format,...);
BOOL            TraceInit (void);
BOOL            TraceDebug (void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* #ifndef __TRACE_H */

/* End of trace.h */


