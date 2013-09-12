/* ----------------------------------------------------------------------------
File Name: d0360.h

Description:

    stv0360 demod driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_D0360_H
#define __STTUNER_TER_D0360_H

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
#include "360_echo.h" 
#include "360_drv.h"


/* defines ----------------------------------------------------------------- */

/* STV0360 common definitions */
#define DEF0360_SCAN    0
#define DEF0360_CHANNEL 1

/**/
#define DEF0360_4MHZ 1

/**/
#define DEF0360_NBPARAM      50
#define DEF0360_NBREG        65
#define DEF0360_NBFIELD     113
/**/
#define DEF0360_SET         0
#define DEF0360_GET         1
#define DEF0360_NOCHANGE    0
#define DEF0360_END         0
#define DEF0360_ON          1
#define DEF0360_OFF         0

#define DEF0360_UNSIGNED    0
#define DEF0360_SIGNED      1

/* info */
#define DEF0360_USERCHANGE     1
#define DEF0360_TUNERCHANGE    2
#define DEF0360_REGCHANGE      3
#define DEF0360_PANELCHANGE    4
#define DEF0360_STANDBY     1000
#define DEF0360_NOSTANDBY   1999

/**/
#define DEF0360_I2CDRIVER              3
#define DEF0360_APPLICATIONMAIN      100
#define DEF0360_DRIVER               200
#define DEF0360_SYSTEM              1100
#define DEF0360_USERPARAMETERS      1200
#define DEF0360_STVFILE             1300
#define DEF0360_TRACE               1400
#define DEF0360_BATCH               1500
#define DEF0360_REGMAP              1600
#define DEF0360_PANELUTIL           1700
#define DEF0360_DRIVERMAIN         10100
#define DEF0360_I2CSTATUSPANEL     10300
#define DEF0360_REGREADMAPPANEL    10400
#define DEF0360_REGWRITEMAPPANEL   10500
#define DEF0360_REPORTPANEL        10800
#define DEF0360_READMEPANEL        10900
#define DEF0360_LOOKUPPANEL        11000

/**/
#define DEF0360_TUNERDRIVERPANEL   20200
#define DEF0360_CARRIERPANEL       20300
#define DEF0360_CLOOPPANEL         20310
#define DEF0360_TLOOPPANEL         20320
#define DEF0360_SCANPANEL          20400
#define DEF0360_CLOCKPANEL         20500
#define DEF0360_IOPANEL            20600
#define DEF0360_ERRORPANEL         20700
#define DEF0360_FECPANEL           20800
#define DEF0360_BENCHMARKPANEL     20900

/* MACRO definitions */
#define MAC0360_ABS(X)   ((X)<0 ?   (-X) : (X))
#define MAC0360_MAX(X,Y) ((X)>=(Y) ? (X) : (Y))
#define MAC0360_MIN(X,Y) ((X)<=(Y) ? (X) : (Y))
#define MAC0360_MAKEWORD(X,Y) ((X<<8)+(Y))
#define MAC0360_LSB(X) ((X & 0xFF))
#define MAC0360_MSB(Y) ((Y>>8)& 0xFF)
#define MAC0360_INRANGE(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)


/* Maximum signal quality */
#define STV0360_MAX_SIGNAL_QUALITY     100

/* REGISTER BIT ASSIGNMENTS */

/* STV0360 Register VSTATUS */
#define STV0360_VSTATUS_PR_1_2  (4)     /* Basic 1/2 */
#define STV0360_VSTATUS_PR_2_3  (0)     /* Punctured 2/3 */
#define STV0360_VSTATUS_PR_3_4  (1)     /* Punctured 3/4 */
#define STV0360_VSTATUS_PR_5_6  (2)     /* Punctured 5/6 */
#define STV0360_VSTATUS_PR_7_8  (3)     /* Punctured 7/8 (Mode A) or 6/7 (Mode B) */

/* STV0360 Register VENRATE (0x09) Puncture rate enable */
#define STV0360_VENRATE_E0_MSK  (0x01)  /* Enable Basic Puncture Rate 1/2 */
#define STV0360_VENRATE_E1_MSK  (0x02)  /* Enable Puncture Rate 2/3 */
#define STV0360_VENRATE_E2_MSK  (0x04)  /* Enable Puncture Rate 3/4 */
#define STV0360_VENRATE_E3_MSK  (0x08)  /* Enable Puncture Rate 5/6 */
#define STV0360_VENRATE_E4_MSK  (0x10)  /* Enable Puncture Rate 6/7 Mode B
                                           or 7/8 Mode A */


/* macros ------------------------------------------------------------------ */



/* enumerated types -------------------------------------------------------- */


typedef struct
{
    U32                     Frequency;
    FE_360_SearchResult_t   Result;
} D0360_ScanResult_t;



/* ---------- per instance of driver ---------- */
#ifndef STTUNER_MINIDRIVER
typedef struct
{
    ST_DeviceName_t             *DeviceName;        /* unique name for opening under */
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, etc. using this */
    IOARCH_Handle_t             IOHandle;           /* instance access to I/O driver     */

    FE_360_Handle_t             FE_360_Handle;

    /* filled in init(), used in open() function. */
    FE_360_InitParams_t         FE_360_InitParams;
    
    ST_Partition_t              *MemoryPartition;   /* which partition this data block belongs to */
    void                        *InstanceChainPrev; /* previous data block in chain or NULL if not */
    void                        *InstanceChainNext; /* next data block in chain or NULL if last */
    U32                         ExternalClock;      /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_IOREG_DeviceMap_t 	DeviceMap;
    STTUNER_DataClockPolarity_t ClockPolarity;
    U32                         StandBy_Flag; /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
    FE_360_SearchResult_t       Result;                                                   
 } D0360_InstanceData_t;
#endif

#ifdef STTUNER_MINIDRIVER
typedef struct
{   
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, etc. using this */
    IOARCH_Handle_t             IOHandle;           /* instance access to I/O driver     */

    FE_360_Handle_t             FE_360_Handle;

    /* filled in init(), used in open() function. */
    FE_360_InitParams_t         FE_360_InitParams;
    
    STCHIP_Info_t               Chip;
    ST_Partition_t              *MemoryPartition;   /* which partition this data block belongs to */
    void                        *InstanceChainPrev; /* previous data block in chain or NULL if not */
    void                        *InstanceChainNext; /* next data block in chain or NULL if last */
    U32                         ExternalClock;      /* External VCO */
    STTUNER_TSOutputMode_t      TSOutputMode;
    STTUNER_SerialDataMode_t    SerialDataMode;
    STTUNER_SerialClockSource_t SerialClockSource;
    STTUNER_FECMode_t           FECMode;
    STTUNER_DataClockPolarity_t ClockPolarity;
 } D0360_InstanceData_t;
#endif
/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of microseconds */

#define STV0360_Delay(micro_sec) STOS_TaskDelayUs(micro_sec) 

/* functions --------------------------------------------------------------- */
#ifndef STTUNER_MINIDRIVER
/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0360_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0360_UnInstall(STTUNER_demod_dbase_t *Demod);
#endif
#ifdef STTUNER_MINIDRIVER
ST_ErrorCode_t demod_d0360_Init(ST_DeviceName_t *DeviceName, DEMOD_InitParams_t *InitParams);
ST_ErrorCode_t demod_d0360_Term(ST_DeviceName_t *DeviceName, DEMOD_TermParams_t *TermParams);

ST_ErrorCode_t demod_d0360_Open (ST_DeviceName_t *DeviceName, DEMOD_OpenParams_t  *OpenParams, DEMOD_Capability_t *Capability, DEMOD_Handle_t *Handle);
ST_ErrorCode_t demod_d0360_Close(DEMOD_Handle_t  Handle, DEMOD_CloseParams_t *CloseParams);

ST_ErrorCode_t demod_d0360_GetTunerInfo    (DEMOD_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo_p);
ST_ErrorCode_t demod_d0360_GetSignalQuality(DEMOD_Handle_t Handle, U32  *SignalQuality_p, U32 *Ber);
ST_ErrorCode_t demod_d0360_GetModulation   (DEMOD_Handle_t Handle, STTUNER_Modulation_t *Modulation);
ST_ErrorCode_t demod_d0360_GetAGC          (DEMOD_Handle_t Handle, S16                  *Agc);
ST_ErrorCode_t demod_d0360_GetMode         (DEMOD_Handle_t Handle, STTUNER_Mode_t       *Mode);
ST_ErrorCode_t demod_d0360_GetGuard        (DEMOD_Handle_t Handle, STTUNER_Guard_t      *Guard);
ST_ErrorCode_t demod_d0360_GetFECRates     (DEMOD_Handle_t Handle, STTUNER_FECRate_t    *FECRates);
ST_ErrorCode_t demod_d0360_IsLocked        (DEMOD_Handle_t Handle, BOOL                 *IsLocked);
ST_ErrorCode_t demod_d0360_Tracking        (DEMOD_Handle_t Handle, BOOL ForceTracking,   U32 *NewFrequency, BOOL *SignalFound);
ST_ErrorCode_t demod_d0360_ScanFrequency   (DEMOD_Handle_t  Handle,
                                            U32  InitialFrequency,
                                            U32  SymbolRate,
                                            U32  MaxOffset,
                                            U32  TunerStep,
                                            U8   DerotatorStep,
                                            BOOL *ScanSuccess,
                                            U32  *NewFrequency,
                                            U32  Mode,
                                            U32  Guard,
                                            U32  Force,
                                            U32  Hierarchy,
                                            U32  Spectrum,
                                            U32  FreqOff,
                                            U32  ChannelBW,
                                            S32  EchoPos);
#endif



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_D0360_H */


/* End of d0360.h */
