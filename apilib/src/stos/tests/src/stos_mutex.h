/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : mutex.h
Description : STOS Mutex specific tests

Note        :

Date          Modification                                    Initials
----          ------------                                    --------
May 2007      Creation                                        WA

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STOS_MUTEX_H__
#define __STOS_MUTEX_H__

/* Includes ----------------------------------------------------------------- */

#if !defined (ST_OSLINUX) || !defined (MODULE)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#endif

#include "stddefs.h"
#include "stos.h"

/* C++ support */
#ifdef __cplusplus
extern "C" {
#endif

/* Exported Constants ------------------------------------------------------- */

/* Exported Types ----------------------------------------------------------- */

/* Exported Variables ------------------------------------------------------- */

/* Exported Macros ---------------------------------------------------------- */

/* Exported Functions ------------------------------------------------------- */

STOS_Error_t TestMutex_mutex_fifo(void);
STOS_Error_t TestMutex_mutex_stress(void);
STOS_Error_t TestMutex_mutex_delete(void);
STOS_Error_t TestMutex_mutex_lock(void);
STOS_Error_t TestMutex_mutex_memory(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __STOS_MUTEX_H__ */


/* end of mutex.h */



