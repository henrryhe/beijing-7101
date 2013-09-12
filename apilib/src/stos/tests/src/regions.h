/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : regions.h
Description : STOS regions tests header file

Note        : 

Date          Modification                                    Initials
----          ------------                                    --------
Mar 2007      Creation                                        CD

************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STOS_REGIONS_H__
#define __STOS_REGIONS_H__

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

STOS_Error_t TestMapRegister_write_read(void);
STOS_Error_t TestUnmapRegister_aftermap(void);
STOS_Error_t TestUnmapRegister_nomap(void);
STOS_Error_t TestVirtToPhys_correct_address(void);
STOS_Error_t TestInvalidateRegion_invalidate(void);
STOS_Error_t TestFlushRegion_flush(void);


/* C++ support */
#ifdef __cplusplus
}
#endif

#endif /* __STOS_REGIONS_H__ */


/* end of regions.h */
