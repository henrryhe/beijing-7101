/*------------------------------------------------File Name: d0899.h

date: 17-Jan-2005
version: 
 author: SD 
comment: Add support for 0899.
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_D0899_H
#define __STTUNER_SAT_D0899_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

/* Standard definitions */
#include "stddefs.h"
/* internal types */
#include "dbtypes.h"    /* types of satellite driver database */
#include "sttuner.h"
#include "ioarch.h"
#include "stcommon.h"

/* LLA */
#include "drv0899.h"

typedef struct
{
    ST_DeviceName_t            *DeviceName;         /* unique name for opening under */
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, lnb etc. using this */
    STTUNER_IOREG_DeviceMap_t 	DeviceMap;            /* stb0899 register map & data table */
    IOARCH_Handle_t             IOHandle;           /* instance access to I/O driver     */
    /*FE_STB0899_Handle_t         FE_STB0899_Handle;  */    /* instance of specific 0899 register map*/
    FE_STB0899_Modulation_t     FE_STB0899_Modulation;
    /* define sttuner modulation & ModeCode for DVBS2*/
    ST_Partition_t             *MemoryPartition;    /* which partition this data block belongs to  */
    void                       *InstanceChainPrev;  /* previous data block in chain or NULL if not */
    void                       *InstanceChainNext;  /* next data block in chain or NULL if last    */
    U32                         ExternalClock;      /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_BlockSyncMode_t     BlockSyncMode;     /* add block sync bit control for bug GNBvd27452*/
    STTUNER_DataFIFOMode_t      DataFIFOMode;      /* add block sync bit control for bug GNBvd27452*/
    STTUNER_OutputFIFOConfig_t  OutputFIFOConfig;  /* add block sync bit control for bug GNBvd27452*/
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_DataClockPolarity_t      ClockPolarity;
    STTUNER_DataClockAtParityBytes_t DataClockAtParityBytes;
    STTUNER_TSDataOutputControl_t    TSDataOutputControl;
    STTUNER_FECType_t           FECType;
    STTUNER_FECType_t           FECMode;
    STTUNER_DiSEqCConfig_t      DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of current state**/
    BOOL                        DISECQ_ST_ENABLE;  /* Demod is capable to send DiSEqC-ST command or not Direct-True; LOOPTHROUGH-FALSE*/
    U32                         DiSEqC_RxFreq;
     BOOL                        SET_TRACKING_PARAMS; 
     U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
} STB0899_InstanceData_t;   /* struct exported as 399 lnb driver also uses this */

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0899_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STB0899_UnInstall(STTUNER_demod_dbase_t *Demod);

#ifdef __cplusplus
extern "C"
{
#endif 

#endif

