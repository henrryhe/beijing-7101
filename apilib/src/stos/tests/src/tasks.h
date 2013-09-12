/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : tasks.h
Description : STOS tasks/threads tests header file

Note        : 

Date          Modification                                    Initials
----          ------------                                    --------
Mar 2007      Creation                                        CD

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STOS_TASKS_H__
#define __STOS_TASKS_H__

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

STOS_Error_t TestTasks_tasks_end(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __STOS_TASKS_H__ */


/* end of tasks.h */
