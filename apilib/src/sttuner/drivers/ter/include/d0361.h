/* ----------------------------------------------------------------------------
File Name: d0361.h

Description:

    stv0361 demod driver.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_D0361_H
#define __STTUNER_TER_D0361_H

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
#include "361_drv.h"

/* defines ----------------------------------------------------------------- */

/* STV0361 common definitions */
#define DEF0361_SCAN    0
#define DEF0361_CHANNEL 1

/**/
#define DEF0361_4MHZ 1

/**/
#define DEF0361_NBPARAM      50
#define DEF0361_NBREG        65
#define DEF0361_NBFIELD     113
/**/
#define DEF0361_SET         0
#define DEF0361_GET         1
#define DEF0361_NOCHANGE    0
#define DEF0361_END         0
#define DEF0361_ON          1
#define DEF0361_OFF         0

#define DEF0361_UNSIGNED    0
#define DEF0361_SIGNED      1

/* info */
#define DEF0361_USERCHANGE     1
#define DEF0361_TUNERCHANGE    2
#define DEF0361_REGCHANGE      3
#define DEF0361_PANELCHANGE    4
#define DEF0361_STANDBY     1000
#define DEF0361_NOSTANDBY   1999

/**/
#define DEF0361_I2CDRIVER              3
#define DEF0361_APPLICATIONMAIN      100
#define DEF0361_DRIVER               200
#define DEF0361_SYSTEM              1100
#define DEF0361_USERPARAMETERS      1200
#define DEF0361_STVFILE             1300
#define DEF0361_TRACE               1400
#define DEF0361_BATCH               1500
#define DEF0361_REGMAP              1600
#define DEF0361_PANELUTIL           1700
#define DEF0361_DRIVERMAIN         10100
#define DEF0361_I2CSTATUSPANEL     10300
#define DEF0361_REGREADMAPPANEL    10400
#define DEF0361_REGWRITEMAPPANEL   10500
#define DEF0361_REPORTPANEL        10800
#define DEF0361_READMEPANEL        10900
#define DEF0361_LOOKUPPANEL        11000

/**/
#define DEF0361_TUNERDRIVERPANEL   20200
#define DEF0361_CARRIERPANEL       20300
#define DEF0361_CLOOPPANEL         20310
#define DEF0361_TLOOPPANEL         20320
#define DEF0361_SCANPANEL          20400
#define DEF0361_CLOCKPANEL         20500
#define DEF0361_IOPANEL            20600
#define DEF0361_ERRORPANEL         20700
#define DEF0361_FECPANEL           20800
#define DEF0361_BENCHMARKPANEL     20900

/* MACRO definitions */
#define MAC0361_ABS(X)   ((X)<0 ?   (-X) : (X))
#define MAC0361_MAX(X,Y) ((X)>=(Y) ? (X) : (Y))
#define MAC0361_MIN(X,Y) ((X)<=(Y) ? (X) : (Y))
#define MAC0361_MAKEWORD(X,Y) ((X<<8)+(Y))
#define MAC0361_LSB(X) ((X & 0xFF))
#define MAC0361_MSB(Y) ((Y>>8)& 0xFF)
#define MAC0361_INRANGE(X,Y,Z) (((X<=Y) && (Y<=Z))||((Z<=Y) && (Y<=X)) ? 1 : 0)


/* Maximum signal quality */
#define STV0361_MAX_SIGNAL_QUALITY     100

/* REGISTER BIT ASSIGNMENTS */

/* STV0361 Register VSTATUS */
#define STV0361_VSTATUS_PR_1_2  (4)     /* Basic 1/2 */
#define STV0361_VSTATUS_PR_2_3  (0)     /* Punctured 2/3 */
#define STV0361_VSTATUS_PR_3_4  (1)     /* Punctured 3/4 */
#define STV0361_VSTATUS_PR_5_6  (2)     /* Punctured 5/6 */
#define STV0361_VSTATUS_PR_7_8  (3)     /* Punctured 7/8 (Mode A) or 6/7 (Mode B) */

/* STV0361 Register VENRATE (0x09) Puncture rate enable */
#define STV0361_VENRATE_E0_MSK  (0x01)  /* Enable Basic Puncture Rate 1/2 */
#define STV0361_VENRATE_E1_MSK  (0x02)  /* Enable Puncture Rate 2/3 */
#define STV0361_VENRATE_E2_MSK  (0x04)  /* Enable Puncture Rate 3/4 */
#define STV0361_VENRATE_E3_MSK  (0x08)  /* Enable Puncture Rate 5/6 */
#define STV0361_VENRATE_E4_MSK  (0x10)  /* Enable Puncture Rate 6/7 Mode B
                                           or 7/8 Mode A */


/* macros ------------------------------------------------------------------ */

/* convert FE_361_Handle_t to STCHIP_Handle_t */
#define FE2CHIP(x) ((STCHIP_Handle_t)(*(U32*)(x)))

/* enumerated types -------------------------------------------------------- */


typedef struct
{
    U32                     Frequency;
    FE_361_SearchResult_t   Result;
}
D0361_ScanResult_t;



/* ---------- per instance of driver ---------- */
typedef struct
{
    ST_DeviceName_t             *DeviceName;        /* unique name for opening under */
    STTUNER_Handle_t            TopLevelHandle;     /* access tuner, etc. using this */
    IOARCH_Handle_t             IOHandle;           /* instance access to I/O driver     */

    FE_361_Handle_t             FE_361_Handle;

    /* filled in init(), used in open() function. */
    FE_361_InitParams_t         FE_361_InitParams;
   

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
     U32                         StandBy_Flag;  /*** To take care same Standby Mode doesnot
                                                   execute more than once ***/
     FE_361_SearchResult_t       Result;
                                                   
 }
D0361_InstanceData_t;

/* macros ------------------------------------------------------------------ */

/* Delay calling task for a period of microseconds */
#define STV0361_Delay(micro_sec) STOS_TaskDelayUs(micro_sec) 
                                  

/* functions --------------------------------------------------------------- */

/* install into demod database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0361_Install(STTUNER_demod_dbase_t *Demod);

/* remove from database */
ST_ErrorCode_t STTUNER_DRV_DEMOD_STV0361_UnInstall(STTUNER_demod_dbase_t *Demod);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_D0361_H */


/* End of d0361.h */
