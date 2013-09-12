/* ----------------------------------------------------------------------------
File Name: d0297e.h

Description:

    stv0297e demod driver.

Copyright (C) 1999-2006 STMicroelectronics

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_D0297E_H
#define __STTUNER_CAB_D0297E_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"

/* internal types */
#include "dbtypes.h"    /* types of cable driver database */

/* STV0297E common definitions */


/* enumerated types -------------------------------------------------------- */


/* driver internal types --------------------------------------------------- */


/* macros ------------------------------------------------------------------ */


/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297E_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297E_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_D0297E_H */


/* End of d0297e.h */
