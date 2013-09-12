/* ----------------------------------------------------------------------------
File Name: d0370VSB.h

Description:

    

Copyright (C) 2005-2006 STMicroelectronics

History:

   date: 
version: 
 author: 
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_D0370_H
#define __STTUNER_TER_D0370_H

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
#include "370_VSB_drv.h"

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
    STTUNER_IOREG_DeviceMap_t 	DeviceMap;            /* stb370 VSB register map & data table */
     U32                         StandBy_Flag;   /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
 } D0370VSB_InstanceData_t;



/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0370VSB_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0370VSB_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_D0370_H */


/* End of d0370VSB.h */
