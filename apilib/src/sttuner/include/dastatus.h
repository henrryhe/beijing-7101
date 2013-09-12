
/* ----------------------------------------------------------------------------
File Name: dastatus.h

Description: 

    Status reporting for drivers and instances of STTUNER.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DASTATUS_H
#define __STTUNER_DASTATUS_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* -------------------------------------------------------------------------
    STTUNER_Da_Status_t
    
    Handle Status for top level instance and drivers.
   ------------------------------------------------------------------------- */
    
    typedef enum
    {
        STTUNER_DASTATUS_UNKNOWN = 0,   /* state indeterminate/unstable    */
        STTUNER_DASTATUS_INIT,          /* initalized handle               */
        STTUNER_DASTATUS_OPEN,          /* opened handle                   */
        STTUNER_DASTATUS_CLOSE,         /* closed handle                   */
        STTUNER_DASTATUS_FREE,          /* termed handle                   */
        STTUNER_DASTATUS_FAIL           /* non-functional, serious failure */
    }
    STTUNER_Da_Status_t;
 

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DASTATUS_H */


/* End of dastatus.h */
