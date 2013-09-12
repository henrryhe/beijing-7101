/*****************************************************************************
File Name   : stbootcache.h

Description : CACHE setup

              This module contains functions for configuring the system
              on-board instruction and data caches.

Copyright (C) 1999-2005 STMicroelectronics

Reference   :
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __STBOOTCACHE_H
#define __STBOOTCACHE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes --------------------------------------------------------------- */

#include "stddefs.h"
#include "stboot.h"

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

#if defined(ST_OS20)
#if !defined(PROCESSOR_C1)
#pragma ST_device( CacheReg_t )
#endif
#endif
typedef volatile U8 CacheReg_t;

/* Exported Macros -------------------------------------------------------- */

/* Exported Functions ----------------------------------------------------- */

#ifdef ST_OS20
void stboot_InitICache(CacheReg_t *CacheDevice_p,
                       BOOL ICacheEnable);

BOOL stboot_InitDCache(CacheReg_t *CacheDevice_p,
                       STBOOT_DCache_Area_t *DCacheMap);

void stboot_LockCaches(CacheReg_t *CacheDevice_p);
#endif

#ifdef ST_OS21
ST_ErrorCode_t stboot_st200_InitDCache(STBOOT_InitParams_t *InitParams_p);
#endif

#endif /* ifndef __STBOOTCACHE_H */

#ifdef __cplusplus
}
#endif

/* End of stbootcache.h */
