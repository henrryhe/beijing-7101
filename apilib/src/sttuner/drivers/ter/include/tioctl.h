/*---------------------------------------------------------------
File Name: tioctl.h

Description:

     typedefs for terrestrial ioctl functions, copy these or point to this header
     so applications can use these.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from the work by GJP
comment: new

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TERIOCTL_H
#define __STTUNER_TERIOCTL_H


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
} TERIOCTL_SETREG_InParams_t;


typedef struct
{
    STTUNER_IOARCH_Operation_t Operation;
    U8  SubAddr;
    U8 *Data;
    U32 TransferSize;
    U32 Timeout;
} TERIOCTL_IOARCH_Params_t;


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TERIOCTL_H */

/* End of tioctl.h */
