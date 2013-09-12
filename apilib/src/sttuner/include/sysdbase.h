/*---------------------------------------------------------------
File Name: sysdbase.h

Description: 

     database functions

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 19-June-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SYSDBASE_H
#define __STTUNER_SYSDBASE_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "dbtypes.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#ifdef STTUNER_MINIDRIVER
#if defined(ST_OS21) || defined(ST_OSLINUX)
#define SEM_LOCK(n)   semaphore_wait(n)
#define SEM_UNLOCK(n) semaphore_signal(n)
#else
#define SEM_LOCK(n)   semaphore_wait(&(n))
#define SEM_UNLOCK(n) semaphore_signal(&(n))
#endif 	 
#endif

/* functions --------------------------------------------------------------- */

/* functions to share the address of these databases declared in sysdbase.c */

/* device drivers */
STTUNER_DriverDbase_t   *STTUNER_GetDrvDb(void);

/* instance data */
STTUNER_InstanceDbase_t *STTUNER_GetDrvInst(void);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SYSDBASE_H */

/* End of sysdbase.h */
