/* ----------------------------------------------------------------------------
File Name: tnone.h

Description: 

    null tuner driver

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 20-June-2001
version: 3.1.0
 author: GJP
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SHARED_TNONE_H
#define __STTUNER_SHARED_TNONE_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_NONE_Install(STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_NONE_UnInstall(STTUNER_tuner_dbase_t *Tuner);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SHARED_TNONE_H */


/* End of tnone.h */
