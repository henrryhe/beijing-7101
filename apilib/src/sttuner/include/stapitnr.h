/*---------------------------------------------------------------
File Name: stapitnr.h

Description: 

     STAPI management functions in sttuner.c that are to be exported 
     from sttuner (mostly for internal use) but not destined for sttuner.h

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 7-June-2001
version: 3.1.0
 author: GJP
comment: new
    
   date: 27-Nov-2001
version: 3.1.0
 author: GJP
comment: renamed from stapi.h to stapitnr.h

   date: 21-Dec-2001
version: 3.2.0
 author: GJP
comment: moved handle mask constants & macros from manager/sttuner.c to here
         so all of sttuner can access them.

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_STAPITNR_H
#define __STTUNER_STAPITNR_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "sttuner.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* Exported Macros -------------------------------------------------------- */

/* Converts an STTUNER event to an offset in the EvtId[] array stored in the tuner control block. */
#define EventToId(x) (x-STTUNER_EV_LOCKED)

/* convert between handle and zero based index into an array (cannot have a zero value for a valid STAPI handle) */
#define HANDLE_AND_MASK 0x000000FF
#define HANDLE_INV_MASK 0xFFFFFF00
#define HANDLE_OR_MASK  (STTUNER_DRIVER_BASE | 0x8000)  /* STTUNER handle signature */

#define HANDLE2INDEX(x) ((x)&(HANDLE_AND_MASK))
#define INDEX2HANDLE(x) ((x)|(HANDLE_OR_MASK ))


/* functions --------------------------------------------------------------- */

/* sttuner.c */
BOOL STTUNER_IsInitalized(void); /* system initalization state */

/* validate.c */
ST_ErrorCode_t VALIDATE_InitParams(STTUNER_InitParams_t *InitParams_p);
ST_ErrorCode_t VALIDATE_IsUniqueName(ST_DeviceName_t DeviceName);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_STAPITNR_H */

/* End of stapitnr.h */
