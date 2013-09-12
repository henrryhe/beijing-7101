/* ----------------------------------------------------------------------------
File Name: drv0297.h (was drv0299.h)

Description: 

    stv0297 demod driver internal low-level functions.

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
#ifndef __STTUNER_CAB_DRV0297_H
#define __STTUNER_CAB_DRV0297_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

#include "sttuner.h"
#include "d0297.h"    /* top level include for this driver */

/* types ------------------------------------------------------------------- */

typedef enum
{
    E297_NOCARRIER=0,
    E297_CARRIEROK,

    E297_NOAGC,
    E297_AGCOK,
    E297_AGCMAX,

    E297_NOEQUALIZER,
    E297_EQUALIZEROK,

    E297_NODATA,
    E297_DATAOK,

    E297_LOCKOK
} 
D0297_SignalType_t;


/*	Scan direction enum	*/
typedef enum 
{
    E297_SCANUP   =  1,
    E297_SCANDOWN = -1
}
D0297_Scandir_t; 


/* driver internal types --------------------------------------------------- */

/*	Search result structure	*/    
typedef struct
{
    D0297_SignalType_t      SignalType;
    long                    Frequency;
    long                    SymbolRate; 
    STTUNER_Modulation_t    QAMSize;
}
D0297_Searchresult_t;

/* AGC2 gain and frequency structure */
typedef struct
{
    int  NbPoints;
    int  Gain[3];
    long Frequency[3];
}
D0297_Agc2TimingOK_t;	


/*	Search parameters structure	*/         
typedef struct
{
    D0297_SignalType_t      State;

    long Frequency;
    long SymbolRate;
    long RollOff;
    long TunerStep;
    long TunerOffset;
    long TunerBW;
    long TunerIF;

    short int Direction;
}
D0297_SearchParams_t;


typedef struct
{
    int                     CarrierOffset;
    int                     TimingOffset;
    int                     lastAGC2Coef;
    int                     ScanMode;
    /**/
    STTUNER_Modulation_t    QAMSize;
    STTUNER_Spectrum_t      SpectrumInversion;
    STTUNER_J83_t           J83;
    int                     SweepRate;
    long                    InitDemodOffset;
    /**/

    D0297_SearchParams_t Params;
    D0297_Searchresult_t Result;

    /**/
    int                     Ber[3];             /* 0 : Ber Cnt
                                                   1 : Ber Saturation en %
                                                   2 : Ber
                                                   */
    int                     CNdB;               /* In dB */
    int                     SignalQuality;      /* SignalQuality from 0 to 100 % */

    U16                     BlkCounter;
    U16                     CorrBlk;
    U16                     UncorrBlk;

} 
D0297_StateBlock_t;

/* functions --------------------------------------------------------------- */

ST_Revision_t Drv0297_GetLLARevision(void);
void Drv0297_WaitTuner      (STTUNER_tuner_instance_t *TunerInstance_p, int TimeOut);
long Drv0297_CarrierWidth   (long SymbolRate, long RollOff);

D0297_SignalType_t Drv0297_CheckAgc     (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                                         D0297_SearchParams_t *Params_p);
D0297_SignalType_t Drv0297_CheckData    (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                                         D0297_SearchParams_t *Params_p);

void Drv0297_InitParams     (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle);
void Drv0297_InitSearch     (STTUNER_tuner_instance_t  *TunerInstance_p, D0297_StateBlock_t *StateBlock_p,
                             STTUNER_Modulation_t Modulation, int Frequency, int SymbolRate, STTUNER_Spectrum_t Spectrum,
                             BOOL ScanExact);


void Driv0297DemodSetting   (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             long Offset);
void Driv0297CNEstimator    (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             int *Mean_p, int *CN_dB100_p, int *CN_Ratio_p);

void Driv0297BertCount      (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             int * _pBER_cnt, int *_pBER);

void Driv0297GetAGC         (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             int *Agc1, int *Agc2);

BOOL Driv0297DataSearch     (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p);
BOOL Driv0297CarrierSearch  (STTUNER_IOREG_DeviceMap_t *pDeviceMap, IOARCH_Handle_t IOHandle,
                             D0297_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_DRV0297_H */


/* End of drv0297.h */
