/*---------------------------------------------------------------
File Name: drivers.h

Description: 

     Manager interface to install drivers.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_DRIVERS_H
#define __STTUNER_DRIVERS_H


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
} DRIVERS_InstallParams_t;


/* functions --------------------------------------------------------------- */

/* drivers.c */

ST_ErrorCode_t STTUNER_DRV_Install(DRIVERS_InstallParams_t *InstallParams);
ST_ErrorCode_t STTUNER_DRV_UnInstall(void);
ST_Revision_t STTUNER_DRV_GetLLARevision(int demodtype);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_DRIVERS_H */

/* End of drivers.h */
