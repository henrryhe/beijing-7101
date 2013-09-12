/*---------------------------------------------------------------
File Name: satioctl.h

Description: 

     typedefs for satellite ioctl functions, copy these or point to this header
     so applications can use these.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 18-June-2001
version: 3.1.0
 author: GJP
comment: new
    
   date: 17-August-2001
version: 3.1.1
 author: GJP
comment: update SubAddr to U16

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SATIOCTL_H
#define __STTUNER_SATIOCTL_H


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
} SATIOCTL_SETREG_InParams_t;


typedef struct
{
    STTUNER_IOARCH_Operation_t Operation; 
    U16  SubAddr;
    U8  *Data;
    U32  TransferSize;
    U32  Timeout;
} SATIOCTL_IOARCH_Params_t;


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SATIOCTL_H */

/* End of satioctl.h */
