/* ----------------------------------------------------------------------------
File Name: drv0297J.h (was drv0297.h)

Description: 

    stv0297J demod driver internal low-level functions.

Copyright (C) 1999-2001 STMicroelectronics

   date: 15-May-2002
version: 3.5.0
 author: from STV0297 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.
    
Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_DRV0297J_H
#define __STTUNER_CAB_DRV0297J_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

#include "sttuner.h"
#include "d0297j.h"    /* top level include for this driver */

/* types ------------------------------------------------------------------- */

typedef enum
{
    E297J_NOCARRIER=0,
    E297J_CARRIEROK,

    E297J_NOAGC,
    E297J_AGCOK,
    E297J_AGCMAX,

    E297J_NOEQUALIZER,
    E297J_EQUALIZEROK,

    E297J_NODATA,
    E297J_DATAOK,

    E297J_LOCKOK
} 
D0297J_SignalType_t;


/*	Scan direction enum	*/
typedef enum 
{
    E297J_SCANUP   =  1,
    E297J_SCANDOWN = -1
}
D0297J_Scandir_t; 


/* driver internal types --------------------------------------------------- */

/*	Search result structure	*/    
typedef struct
{
    D0297J_SignalType_t     SignalType;
    long                    Frequency;
    long                    SymbolRate; 
    STTUNER_Modulation_t    QAMSize;
}
D0297J_Searchresult_t;

/* AGC2 gain and frequency structure */
typedef struct
{
    int  NbPoints;
    int  Gain[3];
    long Frequency[3];
}
D0297J_Agc2TimingOK_t;	


/*	Search parameters structure	*/         
typedef struct
{
    D0297J_SignalType_t     State;

    long Frequency;
    long SymbolRate;
    long RollOff;
    long TunerStep;
    long TunerOffset;
    long TunerBW;
    long TunerIF;

    short int Direction;
}
D0297J_SearchParams_t;


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

    D0297J_SearchParams_t   Params;
    D0297J_Searchresult_t   Result;

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
D0297J_StateBlock_t;

/* functions --------------------------------------------------------------- */

ST_Revision_t Drv0297J_GetLLARevision(void);
void Drv0297J_WaitTuner      (STTUNER_tuner_instance_t *TunerInstance_p, int TimeOut);
long Drv0297J_CarrierWidth   (long SymbolRate, long RollOff);

D0297J_SignalType_t Drv0297J_CheckAgc     (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                                           D0297J_SearchParams_t *Params_p);
D0297J_SignalType_t Drv0297J_CheckData    (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                                           D0297J_SearchParams_t *Params_p);

void Drv0297J_InitParams     (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle);
void Drv0297J_InitSearch     (STTUNER_tuner_instance_t  *TunerInstance_p, D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_Modulation_t Modulation, int Frequency, int SymbolRate, STTUNER_Spectrum_t Spectrum,
                             BOOL ScanExact, STTUNER_J83_t J83);


void Driv0297JDemodSetting   (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             long Offset);
void Driv0297JCNEstimator    (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             int *Mean_p, int *CN_dB100_p, int *CN_Ratio_p);

void Driv0297JBertCount      (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             int * _pBER_cnt, int *_pBER);

void Driv0297JGetAGC         (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p,
                             int *Agc1, int *Agc2);

BOOL Driv0297JDataSearch     (STTUNER_IOREG_DeviceMap_t *DeviceMap_p, IOARCH_Handle_t IOHandle,
                             D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p);
BOOL Driv0297JCarrierSearch  (STTUNER_IOREG_DeviceMap_t *pDeviceMap, IOARCH_Handle_t IOHandle,
                             D0297J_StateBlock_t *StateBlock_p,
                             STTUNER_tuner_instance_t *TunerInstance_p);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_DRV0297J_H */


/* End of drv0297J.h */
