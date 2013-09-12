/*---------------------------------------------------------------
File Name: sharedrv.h

Description: 

     Interface to shared drivers.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 18-June-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SHAREDRV_H
#define __STTUNER_SHAREDRV_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* install parameters */
typedef struct
{
    ST_Partition_t  *MemoryPartition;
} SHARE_InstallParams_t;

/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_SHARE_DRV_Install(SHARE_InstallParams_t *InstallParams);
ST_ErrorCode_t STTUNER_SHARE_DRV_UnInstall(void);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SHAREDRV_H */

/* End ofsharedrvsdrv.h */
