/* ----------------------------------------------------------------------------
File Name: drv0299.h

Description: 

    stv0299 demod driver internal low-level functions.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 19-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_DRV0299_H
#define __STTUNER_SAT_DRV0299_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* includes ---------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */

/* internal types */
#include "dbtypes.h"    /* data types for databases */

#include "sttuner.h"
#include "d0299.h"    /* top level include for this driver */
#include "ioreg.h"
#ifdef STTUNER_MINIDRIVER
#include "iodirect.h"	 
#endif

/* functions --------------------------------------------------------------- */

ST_Revision_t Drv0299_GetLLARevision(void);
void Drv0299_WaitTuner(STTUNER_tuner_instance_t *TunerInstance, int TimeOut);

#ifndef STTUNER_MINIDRIVER

long Drv0299_CarrierWidth(long SymbolRate, long RollOff);
int  Drv0299_CarrierNotCentered(STTUNER_IOREG_DeviceMap_t *DeviceMap, D0299_SearchParams_t *Params, int AllowedOffset);
void Drv0299_IQInvertion       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
void Drv0299_IQSet             (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 IQMode);
long Drv0299_CenterTimingLoop  (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, long SymbolRate, long MasterClock);
void FE_299_OffsetSymbolRate   (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle,S32 Offset);
ST_ErrorCode_t Drv0299_SetAgc1Ref(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle );
D0299_SignalType_t Drv0299_CheckRange      (D0299_SearchParams_t *Params, D0299_Searchresult_t *Result);
D0299_SignalType_t Drv0299_CheckAgc1       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, STTUNER_TunerType_t TunerType);
D0299_SignalType_t Drv0299_CheckAgc2       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, int NbSample,    long *Agc2Value);
D0299_SignalType_t Drv0299_CheckTiming     (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_CheckCarrier    (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_CheckData       (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_SearchTiming   (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result,STTUNER_Handle_t TopLevelHandle);
D0299_SignalType_t Drv0299_SearchCarrier  (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle);
D0299_SignalType_t Drv0299_SearchData     (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle);
D0299_SignalType_t Drv0299_AutoSearchAlgo (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_StateBlock_t   *StateBlock, STTUNER_Handle_t TopLevelHandle);
void Drv0299_GetNoiseEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 *Noise, U32 *Ber);
U32 Drv0299_SpectrumAnalysis(STTUNER_tuner_instance_t *TunerInstance, STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, U32 StartFreq,U32 StopFreq,U32 StepSize,U32 *ToneList,U8 mode,int* power_detection_level);
U32 Drv0299_ToneDetection(STTUNER_IOREG_DeviceMap_t *DeviceMap,STTUNER_Handle_t TopLevelHandle,U32 StartFreq,U32 StopFreq,U32 *ToneList,U8 mode,int* power_detection_level);
#else
D0299_SignalType_t Drv0299_Searchlock(D0299_SearchParams_t *Params);
void Drv0299_GetNoiseEstimator(U32 *Noise, U32 *Ber);
long Drv0299_CarrierWidth(long SymbolRate);
void Drv0299_IQInvertion       (void);
void Drv0299_IQSet             (U32 IQMode);
D0299_SignalType_t Drv0299_CheckRange      (D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_CheckTiming     (D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_AutoSearchAlgo(U32 Frequency, U32 SymbolRate, D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_CheckCarrier    (D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_CheckData       (D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_SearchTiming   (D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_SearchCarrier  (D0299_SearchParams_t *Params);
D0299_SignalType_t Drv0299_SearchData     (D0299_SearchParams_t *Params);

#endif

long Drv0299_CalcTimingTimeConstant(long SymbolRate);
long Drv0299_CalcDerotTimeConstant (long SymbolRate);
long Drv0299_CalcDataTimeConstant  (int Er, int Sn, int To, int Hy, long SymbolRate, U32 R0299_FECMVal);

void Drv0299_InitParams(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_StateBlock_t *StateBlock);
void Drv0299_InitSearch(STTUNER_tuner_instance_t  *TunerInstance, D0299_StateBlock_t *StateBlock, int Frequency, int SymbolRate, int SearchRange, int DerotStep);

D0299_SignalType_t Drv0299_SearchFalseLock(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, STTUNER_Handle_t TopLevelHandle);
D0299_SignalType_t Drv0299_TunerCentering (STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_SearchParams_t *Params, D0299_Searchresult_t *Result, int AllowedOffset,STTUNER_Handle_t TopLevelHandle );

int                Drv0299_CarrierTracking(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle, D0299_StateBlock_t   *StateBlock, STTUNER_Handle_t TopLevelHandle);

D0299_SignalType_t Drv0299_FirstSubRange(D0299_SearchParams_t *Params);
void               Drv0299_NextSubRange(D0299_SearchParams_t *Params);

S16  Drv0299_GetPowerEstimator(STTUNER_IOREG_DeviceMap_t *DeviceMap, IOARCH_Handle_t IOHandle);
    
void   Drv0299_TurnOnInfiniteLoop(STTUNER_Handle_t Handle, BOOL *OnOrOff);
    

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_DRV0299_H */


/* End of drv0299.h */
