/*---------------------------------------------------------------
File Name: util.h

Description: 

     Misc utility functions for all levels of STTUNER.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_UTIL_H
#define __STTUNER_UTIL_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "stcommon.h"   /* for ST_GetClocksPerSecond() */

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */



/* constants --------------------------------------------------------------- */

#define SEM_LOCK(n)   STOS_SemaphoreWait(n)
#define SEM_UNLOCK(n) STOS_SemaphoreSignal(n)

    
/* MACRO definitions ------------------------------------------------------- */

#define ABS(X)          ((X)<0 ?   (-X) : (X))
#define MAX(X,Y)        ((X)>=(Y) ? (X) : (Y))
#define MIN(X,Y)        ((X)<=(Y) ? (X) : (Y)) 
#define MAKEWORD(X,Y)   ((X<<8)+(Y))
#define LSB(X)          ((X & 0xFF))
#define MSB(Y)          ((Y>>8)& 0xFF)
#define INRANGE(X,Y,Z)  (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)

#define INTERPOLATE(A, B, C, D, E)  (A+(((E-C)*( B-A))/(D-C)))



#define UTIL_Delay(micro_sec) STOS_TaskDelayUs(micro_sec)
#define UTIL_SystemWaitFor(x) UTIL_Delay((x*1000))


/* types ------------------------------------------------------------------- */

    /* support type: TUNER status */
    typedef struct
    {
        U32 Base;
        U32 Size;
    }
    STTUNER_Util_MemLimit_t;


/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_Util_CheckPtrNull(void *Pointer);

long STTUNER_Util_PowOf2(int number);
long STTUNER_Util_LongSqrt(long Value);
long STTUNER_Util_BinaryFloatDiv(long n1, long n2, int precision);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_UTIL_H */

/* End of util.h */
