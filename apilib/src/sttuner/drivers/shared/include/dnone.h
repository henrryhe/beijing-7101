/* ----------------------------------------------------------------------------
File Name: dnone.h

Description: 

    null demod driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 19-June-2001
version: 3.1.0
 author: GJP
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SHARED_DNONE_H
#define __STTUNER_SHARED_DNONE_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_DEMOD_NONE_Install(STTUNER_demod_dbase_t *Demod);
ST_ErrorCode_t STTUNER_DRV_DEMOD_NONE_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SHARED_DNONE_H */


/* End of dnone.h */
