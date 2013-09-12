/* ----------------------------------------------------------------------------
File Name: d0297J.h

Description:

    stv0297J demod driver.

Copyright (C) 1999-2001 STMicroelectronics

   date: 15-May-2002
version: 3.5.0
 author: from STV0297J and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_D0297J_H
#define __STTUNER_CAB_D0297J_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"

/* internal types */
#include "dbtypes.h"    /* types of cable driver database */


/* STV0297J common definitions */
#define DEF0297J_SCAN 	                    0
#define DEF0297J_CHANNEL	                1

#define DEF0297J_CARRIEROFFSET_RATIO        4

#define	DEF0297J_NBREG		                241
#define DEF0297J_NBFIELD		            487


/* enumerated types -------------------------------------------------------- */


/* driver internal types --------------------------------------------------- */


/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of time */

#define STV0297J_DelayInMicroSec(micro_sec)  STOS_TaskDelayUs(micro_sec)
    
#define STV0297J_DelayInMilliSec(milli_sec)  STV0297J_DelayInMicroSec(milli_sec*1000)

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297J_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0297J_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_D0297J_H */


/* End of d0297J.h */
