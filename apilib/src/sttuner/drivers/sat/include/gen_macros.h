/* ----------------------------------------------------------------------------
File Name: gen_macros.h

Description: 

    glue LLA to STAPI

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 22-Oct-2001
version: 3.1.1
 author: GJP
comment: Initial version.
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_GEN_MACROS_H
#define __STTUNER_SAT_GEN_MACROS_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* Includes --------------------------------------------------------------- */

#include "util.h"       /* MIN MAX macros */
#include "stddef.h"
#include "stddefs.h"     /* NULL */


/* macros ------------------------------------------------------------------ */

#define SystemWaitFor(x) UTIL_Delay((x*1000))





#ifdef __cplusplus
}
#endif                          /* __cplusplus */



#endif  /* __STTUNER_SAT_GEN_MACROS_H */


/* End of gen_macros.h */
