/* ----------------------------------------------------------------------------
File Name: diseqcdrv.h

Description: 

    DISEQC driver
Copyright (C) 1999-2001 STMicroelectronics

History:

   date:
version: 
 author:
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_DISEQCDRV_H
#define __STTUNER_SAT_DISEQCDRV_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* includes ---------------------------------------------------------------- */

#include "stddefs.h"            /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */



/* functions --------------------------------------------------------------- */


/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_DISEQC_5100_530X_Install(STTUNER_diseqc_dbase_t *Diseqc);
ST_ErrorCode_t STTUNER_DRV_DISEQC_5100_530X_unInstall(STTUNER_diseqc_dbase_t *Diseqc);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_DISEQCDRV_H */


/* End of diseqcdrv.h */
