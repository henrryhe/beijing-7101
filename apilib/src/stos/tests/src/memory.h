/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : memory.h
Description : STOS memory tests header file

Note        : 

Date          Modification                                    Initials
----          ------------                                    --------
Mar 2007      Creation                                        CD

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STOS_MEMORY_H__
#define __STOS_MEMORY_H__

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

STOS_Error_t TestAllocate_write_read(void);
STOS_Error_t TestDeallocate_after_alloc(void);
STOS_Error_t TestAllocateClear_clear_write_read(void);
STOS_Error_t TestDeallocate_after_allocclear(void);
STOS_Error_t TestReallocate_write_read(void);
STOS_Error_t TestDeallocate_after_realloc(void);
STOS_Error_t TestReallocate_block_NULL(void);
STOS_Error_t TestReallocate_size_zero(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __STOS_MEMORY_H__ */


/* end of memory.h */
