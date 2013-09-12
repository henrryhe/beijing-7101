/*---------------------------------------------------------------
File Name: cioctl.h (was siocth.h)

Description: 

     typedefs for cable ioctl functions, copy these or point to this header
     so applications can use these.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

   date: 22-Feb-2002
version: 3.2.0
 author: GJP
comment: update SubAddr to U16

Revision History:
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CABIOCTL_H
#define __STTUNER_CABIOCTL_H


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
}
CABIOCTL_SETREG_InParams_t;


typedef struct
{
    STTUNER_IOARCH_Operation_t Operation; 
    U16 SubAddr;
    U8 *Data;
    U32 TransferSize;
    U32 Timeout;
}
CABIOCTL_IOARCH_Params_t;


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CABIOCTL_H */

/* End of cioctl.h */
