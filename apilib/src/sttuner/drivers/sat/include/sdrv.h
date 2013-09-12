/*---------------------------------------------------------------
File Name: sdrv.h

Description: 

     Interface to drivers.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 18-June-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SDRV_H
#define __STTUNER_SDRV_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "sttuner.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* install parameters */
typedef struct
{
    ST_Partition_t  *MemoryPartition;
    STTUNER_TunerType_t    TunerType;
} SDRV_InstallParams_t;

/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_SAT_DRV_Install  (SDRV_InstallParams_t *InstallParams);
ST_ErrorCode_t STTUNER_SAT_DRV_UnInstall(void);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SDRV_H */

/* End of sdrv.h */
