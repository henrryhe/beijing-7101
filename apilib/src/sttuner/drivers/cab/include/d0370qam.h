/* ----------------------------------------------------------------------------
File Name: d0370qam.h

Description:

    

Copyright (C) 2005-2006 STMicroelectronics

   date: 
version: 
 author: 
comment: 

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_D0370QAM_H
#define __STTUNER_CAB_D0370QAM_H

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
#define DEF0370QAM_SCAN 	                    0
#define DEF0370QAM_CHANNEL	                1

#define DEF0370QAM_CARRIEROFFSET_RATIO        4

/* Number of registers  */
	#define		STB0370_QAM_NBREGS 163
	#define 	STB0370_QAM_NBFIELDS 343

typedef enum {NO = 0, YES = 1} FLAG_370QAM ;

/* enumerated types -------------------------------------------------------- */


/* driver internal types --------------------------------------------------- */


/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of time */

#define STB0370QAM_DelayInMicroSec(micro_sec)  STOS_TaskDelayUs(micro_sec)
 
#define STB0370QAM_DelayInMilliSec(milli_sec)  STB0370QAM_DelayInMicroSec(milli_sec*1000)

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0370QAM_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0370QAM_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_D0370QAM_H */


/* End of d0370qam.h */
