/* ----------------------------------------------------------------------------
File Name: d0297.h

Description:

    stv0297 demod driver.

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
#ifndef __STTUNER_CAB_D0297_H
#define __STTUNER_CAB_D0297_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"

/* internal types */
#include "dbtypes.h"    /* types of cable driver database */


/* STV0297 common definitions */
#define DEF0297_SCAN 	                0
#define DEF0297_CHANNEL	                1

#define DEF0297_CARRIEROFFSET_RATIO     4

#define	DEF0297_NBREG		 86
#define DEF0297_NBFIELD		188


/* enumerated types -------------------------------------------------------- */


/* driver internal types --------------------------------------------------- */


/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of time */
#define STV0297_DelayInMicroSec(micro_sec)  STOS_TaskDelayUs(micro_sec)
#define STV0297_DelayInMilliSec(milli_sec)  STV0297_DelayInMicroSec(milli_sec*1000)

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_D0297_H */


/* End of d0297.h */
