/*---------------------------------------------------------------
File Name: shioctl.h

Description:

     typedefs for shared  ioctl functions, copy these or point to this header
     so applications can use these.

Copyright (C) 2005-2006 STMicroelectronics

History:

   date: 
version: 
 author:
comment: 

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SHIOCTL_H
#define __STTUNER_SHIOCTL_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "ioarch.h"     /* base driver I/O      */

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */



/* types --------------------------------------------------------------- */

typedef struct
{
    int           RegIndex;
    unsigned char Value;
} SHIOCTL_SETREG_InParams_t;


typedef struct
{
    STTUNER_IOARCH_Operation_t Operation;
    U8  SubAddr;
    U8 *Data;
    U32 TransferSize;
    U32 Timeout;
} SHIOCTL_IOARCH_Params_t;


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SHIOCTL_H */

/* End of shioctl.h */
