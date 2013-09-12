/* ----------------------------------------------------------------------------
File Name: d0399.h

Description: 

    stv0399 demod driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 20-August-2001
version: 3.2.0
 author: GJP
comment: Initial version.

date: 07-April-2004
version: 
 author: SD 
comment: Add support for SatCR.
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_D0399_H
#define __STTUNER_SAT_D0399_H

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
/* LLA */
#include "399_drv.h"



/* defines ----------------------------------------------------------------- */

/* Maximum signal quality */
#define STV0399_MAX_SIGNAL_QUALITY     100


/* STV0399 VSTATUS Register (READ reg 0x24) */
#define STV0399_VSTATUS_PR_1_2  (0)     /* Basic 1/2 */
#define STV0399_VSTATUS_PR_2_3  (1)     /* Punctured 2/3 */
#define STV0399_VSTATUS_PR_3_4  (2)     /* Punctured 3/4 */
#define STV0399_VSTATUS_PR_5_6  (3)     /* Punctured 5/6 */
#define STV0399_VSTATUS_PR_6_7  (4)     /* Punctured 6/7 */
#define STV0399_VSTATUS_PR_7_8  (5)     /* Punctured 7/8 */
#define STV0399_VSTATUS_PR_RE1  (6)     /* reserved */
#define STV0399_VSTATUS_PR_RE2  (7)     /* reserved */


/* STV0399 Register VENRATE (0x09) Puncture rate enable (WRITE reg 0x37) */
#define STV0399_PR_MASK_7_8  (0x20)  /* Enable Puncture Rate 7/8 */
#define STV0399_PR_MASK_6_7  (0x10)  /* Enable Puncture Rate 6/7 */
#define STV0399_PR_MASK_5_6  (0x08)  /* Enable Puncture Rate 5/6 */
#define STV0399_PR_MASK_3_4  (0x04)  /* Enable Puncture Rate 3/4 */
#define STV0399_PR_MASK_2_3  (0x02)  /* Enable Puncture Rate 2/3 */
#define STV0399_PR_MASK_1_2  (0x01)  /* Enable Puncture Rate 1/2 */


/* macros ------------------------------------------------------------------ */


/* driver internal types --------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t            *DeviceName;         /* unique name for opening under */
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, lnb etc. using this */
    IOARCH_Handle_t             IOHandle;           /* instance access to I/O driver     */

    FE_399_Modulation_t         FE_399_Modulation;
    ST_Partition_t             *MemoryPartition;    /* which partition this data block belongs to  */
    void                       *InstanceChainPrev;  /* previous data block in chain or NULL if not */
    void                       *InstanceChainNext;  /* next data block in chain or NULL if last    */
    STTUNER_IOREG_DeviceMap_t 	DeviceMap;
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
    STTUNER_FECMode_t           FECMode;
    STTUNER_DiSEqCConfig_t      DiSEqCConfig; /** Added DiSEqC configuration structure to get the trace of current state**/
    BOOL                        DISECQ_ST_ENABLE;  /* Demod is capable to send DiSEqC-ST command or not Direct-True; LOOPTHROUGH-FALSE*/
     U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot execute more than once ***/
} D0399_InstanceData_t;   /* struct exported as 399 lnb driver also uses this */

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0399_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0399_UnInstall(STTUNER_demod_dbase_t *Demod);

#endif
/***********************MINIDRIVER***********************
**********************************************************
*****************************************************************/
#ifdef STTUNER_MINIDRIVER

/* API */
ST_ErrorCode_t demod_d0399_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0399_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0399_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0399_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0399_IsAnalogCarrier (DEMOD_Handle_t Handle, BOOL *IsAnalog);        
ST_ErrorCode_t demod_d0399_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0399_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0399_SetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t  Modulation);
ST_ErrorCode_t demod_d0399_GetAGC          (DEMOD_Handle_t Handle, U32                  *Agc);
ST_ErrorCode_t demod_d0399_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0399_GetIQMode       (DEMOD_Handle_t Handle, STTUNER_IQMode_t     *IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
ST_ErrorCode_t demod_d0399_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0399_SetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t     FECRates);
ST_ErrorCode_t demod_d0399_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);

ST_ErrorCode_t demod_d0399_ScanFrequency   (DEMOD_Handle_t Handle, U32  InitialFrequency, U32   SymbolRate,    U32   MaxOffset, 
                                                                   U32  TunerStep,        U8    DerotatorStep, BOOL *ScanSuccess,
                                                                   U32 *NewFrequency,     U32   Mode,          U32   Guard,
                                                                   U32  Force,            U32   Hierarchy,     U32   Spectrum,
                                                                   U32  FreqOff,          U32   ChannelBW,     S32   EchoPos);

/* added for DiSEqC API support*/
ST_ErrorCode_t demod_d0399_DiSEqC(DEMOD_Handle_t Handle, STTUNER_DiSEqCSendPacket_t *pDiSEqCSendPacket, STTUNER_DiSEqCResponsePacket_t *pDiSEqCResponsePacket);

ST_ErrorCode_t demod_d0399_DiSEqCGetConfig ( DEMOD_Handle_t Handle ,STTUNER_DiSEqCConfig_t * DiSEqCConfig);
ST_ErrorCode_t demod_d0399_DiSEqCBurstOFF ( DEMOD_Handle_t Handle );

ST_ErrorCode_t demod_d0399_tonedetection(DEMOD_Handle_t Handle,U32 StartFreq, U32 StopFreq,U8  *NbTones,U32 *ToneList, U8 mode, int* power_detection_level);

#endif
#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_D0399_H */


/* End of d0399.h */
