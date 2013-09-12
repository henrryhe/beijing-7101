/* ----------------------------------------------------------------------------
File Name: d0498.h

Description:

    stv0498 demod driver.

Copyright (C) 1999-2007 STMicroelectronics

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_D0498_H
#define __STTUNER_CAB_D0498_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"

/* internal types */
#include "dbtypes.h"    /* types of cable driver database */

/* STV0498 common definitions */


/* enumerated types -------------------------------------------------------- */


/* driver internal types --------------------------------------------------- */


/* macros ------------------------------------------------------------------ */


/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0498_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0498_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_D0498_H */


/* End of d0498.h */
