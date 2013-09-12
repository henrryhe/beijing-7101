/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : semaphores.h
Description : STOS semaphores tests header file

Note        : 

Date          Modification                                    Initials
----          ------------                                    --------
May 2007      Creation                                        CD

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STOS_SEMAPHORES_H__
#define __STOS_SEMAPHORES_H__

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

STOS_Error_t TestSemaphores_performances(void);
STOS_Error_t TestSemaphores_wait_signal_priorities(void);

/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __STOS_SEMAPHORES_H__ */


/* end of semaphores.h */
