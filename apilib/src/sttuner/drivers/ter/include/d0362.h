/* ----------------------------------------------------------------------------
File Name: d0362.h

Description:

    stv0362 demod driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 8-March-2006
 author: CC from work by NOIDA
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_D0362_H
#define __STTUNER_TER_D0362_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"

/* internal types */
#include "dbtypes.h"    /* types of terrestrial driver database */

/* LLA */
#include "d0362_drv.h"

/* defines ----------------------------------------------------------------- */


/* ---------- per instance of driver ---------- */
/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t             *DeviceName;        /* unique name for opening under */
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, etc. using this */
    IOARCH_Handle_t             IOHandle;           /* instance access to I/O driver     */

    ST_Partition_t              *MemoryPartition;   /* which partition this data block belongs to */
    void                        *InstanceChainPrev; /* previous data block in chain or NULL if not */
    void                        *InstanceChainNext; /* next data block in chain or NULL if last */
    U32                         ExternalClock;      /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DataClockPolarity_t ClockPolarity;
    U8    			*DemodRegVal;
    U32				StandBy_Flag;
    STTUNER_Spectrum_t          ResultSpectrum; /*Used to retain the result to value of spectrum to let 
    							tunerinfo know the value*/
    U32                         UnlockCounter;  /*Used to check unlock counter value before
    						 performing core on/off in tuner tracking */						
    FE_362_SearchResult_t       Result;  
    STTUNER_IF_IQ_Mode           IFIQMode; /*IQmode (whether normal IF or long path IF or long path IQ)*/  						 					
   } D0362_InstanceData_t;

                                  

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0362_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0362_UnInstall(STTUNER_demod_dbase_t *Demod);
D0362_InstanceData_t *d0362_GetInstFromHandle(DEMOD_Handle_t Handle);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_D0362_H */


/* End of d0362.h */
