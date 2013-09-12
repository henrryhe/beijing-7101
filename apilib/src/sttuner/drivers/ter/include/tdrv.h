/*---------------------------------------------------------------
File Name: tdrv.h

Description:

     Interface to drivers.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 13-Sept-2001
version: 3.1.0
 author: GB from the work by GJP
comment: rewrite for terrestrial

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TDRV_H
#define __STTUNER_TDRV_H


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
    STTUNER_TunerType_t    TunerType;
} TDRV_InstallParams_t;

/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_TERR_DRV_Install  (TDRV_InstallParams_t *InstallParams);
ST_ErrorCode_t STTUNER_TERR_DRV_UnInstall(void);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TDRV_H */

/* End of tdrv.h */
