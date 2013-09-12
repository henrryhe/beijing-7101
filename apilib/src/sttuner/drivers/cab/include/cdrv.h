/*---------------------------------------------------------------
File Name: cdrv.h

Description: 

     Interface to drivers.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CDRV_H
#define __STTUNER_CDRV_H


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
}
CDRV_InstallParams_t;

/* functions --------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_CABLE_DRV_Install  (CDRV_InstallParams_t *InstallParams);
ST_ErrorCode_t STTUNER_CABLE_DRV_UnInstall(void);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CDRV_H */

/* End of cdrv.h */
