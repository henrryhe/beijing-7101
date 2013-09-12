/* ----------------------------------------------------------------------------
File Name: tcdrv.h  (was tdrv.h from sat)

Description:

    EXTERNAL (ZIF) tuner drivers:

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
#ifndef __STTUNER_CAB_TCDRV_H
#define __STTUNER_CAB_TCDRV_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

/* constants --------------------------------------------------------------- */

#define TCDRV_IOBUFFER_MAX      21      /* 21 registers for MT2030/40 */
#define TCDRV_IOBUFFER_SIZE      4      /* 4 registers */

#define TCDRV_BANDWIDTH_MAX     10

/* enumerated types -------------------------------------------------------- */

    /* PLL type */
    typedef enum
    {
        TUNER_PLL_TDBE1,
        TUNER_PLL_TDBE2,
        TUNER_PLL_TDDE1,
        TUNER_PLL_SP5730,
        TUNER_PLL_MT2030,
        TUNER_PLL_MT2040,
        TUNER_PLL_MT2050,
        TUNER_PLL_MT2060,
        TUNER_PLL_DCT7040,
        TUNER_PLL_DCT7050,
        TUNER_PLL_DCT7710,
        TUNER_PLL_DCF8710,
        TUNER_PLL_DCF8720,
        TUNER_PLL_MACOETA50DR,
        TUNER_PLL_CD1516LI,
        TUNER_PLL_DF1CS1223,
        TUNER_PLL_SHARPXX,
        TUNER_PLL_TDBE1X016A,
        TUNER_PLL_TDBE1X601,
        TUNER_PLL_TDEE4X012A,
        TUNER_PLL_DCT7045,
        TUNER_PLL_TDQE3,
        TUNER_PLL_DCF8783,
        TUNER_PLL_DCT7045EVAL,
        TUNER_PLL_DCT70700,
        TUNER_PLL_TDCHG,
        TUNER_PLL_TDCJG,
        TUNER_PLL_TDCGG,
        TUNER_PLL_MT2011
    }
    TCDRV_PLLType_t;

/*
**  Structure of data needed for Spur Avoidance
*/
#define MAX_ZONES 20

typedef struct
{
    U32         min;
    U32         max;
}
MT_ExclZone_t;

typedef struct
{
    U32     nAS_Algorithm;
    U32     f_ref;
    U32     f_in;
    U32     f_LO1;
    U32     f_if1_Center;
    U32     f_if1_Request;
    U32     f_if1_bw;
    U32     f_LO2;
    U32     f_out;
    U32     f_out_bw;
    U32     f_LO1_Step;
    U32     f_LO2_Step;
    U32     f_LO1_FracN_Avoid;
    U32     f_LO2_FracN_Avoid;
    U32     f_zif_bw;
    U32     f_min_LO_Separation;
    U32     maxH1;
    U32     maxH2;
    U32     bSpurPresent;
    U32     bSpurAvoided;
    U32     nSpursFound;
    U32     nZones;
    MT_ExclZone_t MT_ExclZones[MAX_ZONES];
}
MT_AvoidSpursData_t;

typedef struct
{
    ST_DeviceName_t     *DeviceName;            /* unique name for opening under */
    STTUNER_Handle_t    TopLevelHandle;         /* access tuner, lnb etc. using this */
    IOARCH_Handle_t     IOHandle;               /* passed in Open() to an instance of this driver */
    STTUNER_TunerType_t TunerType;              /* tuner ID number */
    TCDRV_PLLType_t     PLLType;                /* PLL on this type of tuner */

    U32         version;
    U32         tuner_id;
    MT_AvoidSpursData_t AS_Data;
    U32         f_IF1_actual;
    U32         LNA_Bands[10];

    U32                 FreqFactor;
    TUNER_Status_t      Status;
    U8                  IOBuffer[TCDRV_IOBUFFER_MAX];    /* buffer for ioarch I/O */
    U32                 BandWidth[TCDRV_BANDWIDTH_MAX];
    long                TunerLowBand;
    long                TunerMidBand;
    ST_Partition_t      *MemoryPartition;       /* which partition this data block belongs to */
    void                *InstanceChainPrev;     /* previous data block in chain or NULL if not */
    void                *InstanceChainNext;     /* next data block in chain or NULL if last */
}
TCDRV_InstanceData_t;

/*MT2011*/

typedef enum  eTuner_Type
{
   kRx_TypeMT2111 = 0xFF,  /* Microtune 2111 */
   kRx_TypeMT2011 = 0x50,  /* Microtune 2011 */
   kRx_TypeMT2060 = 0x4F,  /* Microtune 2060 */
   kRx_TypeMT2121 = 0x4E,  /* Microtune 2121 */
   kRx_TypeMT2050 = 0x41   /* Microtune 2050 */
} eTuner_Type;


typedef struct tRxtuner_Status
{  
   BOOL initialized; 
   eTuner_Type type;      
   U32 address; 
   U32 IF2; 
   U32 gFrequency;
   U32 gLO1;
   U32 gLO2; 
   U8 PartCode;
   U8 RevCode;
   U8 *pTunerName; 
} tRxtuner_Status;

/********************************************************/

/* functions --------------------------------------------------------------- */

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE2_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDDE1_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_SP5730_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2030_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2040_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2050_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_Install      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7040_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7050_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7710_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8710_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8720_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MACOETA50DR_Install (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_CD1516LI_Install    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DF1CS1223_Install   (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_SHARPXX_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X016A_Install  (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X601_Install   (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDEE4X012A_Install  (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDQE3_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8783_Install     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045EVAL_Install (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT70700_Install    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCHG_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCJG_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCGG_Install       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2011_Install      (STTUNER_tuner_dbase_t *Tuner);

/* populate database & init driver (not actual hardware) */
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE2_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDDE1_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_SP5730_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2030_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2040_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2050_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2060_UnInstall      (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7040_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7050_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7710_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8710_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8720_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MACOETA50DR_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_CD1516LI_UnInstall    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DF1CS1223_UnInstall   (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_SHARPXX_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X016A_UnInstall  (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDBE1X601_UnInstall   (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDEE4X012A_UnInstall  (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDQE3_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCF8783_UnInstall     (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT7045EVAL_UnInstall (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_DCT70700_UnInstall    (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCHG_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCJG_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_TDCGG_UnInstall       (STTUNER_tuner_dbase_t *Tuner);
ST_ErrorCode_t STTUNER_DRV_TUNER_MT2011_UnInstall      (STTUNER_tuner_dbase_t *Tuner);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_TCDRV_H */


/* End of tcdrv.h */
