/*---------------------------------------------------------------
File Name: swioctl.h

Description: 

    This handles ioctl function calls when the device is STTUNER_SOFT_ID.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 17-July-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SWIOCTL_H
#define __STTUNER_SWIOCTL_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "sttuner.h"

/* sttuner internal */
#include "dastatus.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */



/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_SWIOCTL_Default(STTUNER_Handle_t Handle, U32 Function, void *InParams, void *OutParams, STTUNER_Da_Status_t *Status);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SWIOCTL_H */

/* End of swioctl.h */
