/*---------------------------------------------------------------
File Name: satapi.h

Description: 

     sat management functions to interface to STAPI STTUNER API layer.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 7-June-2001
version: 3.1.0
 author: GJP
comment: new
    
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SATAPI_H
#define __STTUNER_SATAPI_H


/* includes --------------------------------------------------------------- */

#include "stddefs.h"    /* Standard definitions */
#include "sttuner.h"

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */



/* functions --------------------------------------------------------------- */


/* ---------- top level core functions ---------- */

/* init.c */
ST_ErrorCode_t STTUNER_SAT_Init(STTUNER_Handle_t Handle, STTUNER_InitParams_t *InitParams);

/* term.c */
ST_ErrorCode_t STTUNER_SAT_Term(STTUNER_Handle_t Handle, const STTUNER_TermParams_t *TermParams);

/* open.c */
ST_ErrorCode_t STTUNER_SAT_Open(STTUNER_Handle_t Handle, const STTUNER_OpenParams_t *OpenParams);

/* close.c */
ST_ErrorCode_t STTUNER_SAT_Close(STTUNER_Handle_t Handle);

/* ioctl.c */
ST_ErrorCode_t STTUNER_SAT_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams);

/* scan.c */
ST_ErrorCode_t STTUNER_SAT_Scan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, U32 Timeout);
ST_ErrorCode_t STTUNER_SAT_BlindScan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, STTUNER_BlindScan_t *BlindScanParams,U32 Timeout);
ST_ErrorCode_t STTUNER_SAT_ScanContinue(STTUNER_Handle_t Handle, U32 Timeout);
ST_ErrorCode_t STTUNER_SAT_Unlock(STTUNER_Handle_t Handle);

/* get.c */
ST_ErrorCode_t STTUNER_SAT_GetBandList     (STTUNER_Handle_t Handle, STTUNER_BandList_t      *BandList);
ST_ErrorCode_t STTUNER_SAT_GetCapability   (STTUNER_Handle_t Handle, STTUNER_Capability_t    *Capability);
ST_ErrorCode_t STTUNER_SAT_GetScanList     (STTUNER_Handle_t Handle, STTUNER_ScanList_t      *ScanList); 
ST_ErrorCode_t STTUNER_SAT_GetStatus       (STTUNER_Handle_t Handle, STTUNER_Status_t        *Status);
ST_ErrorCode_t STTUNER_SAT_GetThresholdList(STTUNER_Handle_t Handle, STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t *QualityFormat);
ST_ErrorCode_t STTUNER_SAT_GetTunerInfo    (STTUNER_Handle_t Handle, STTUNER_TunerInfo_t     *TunerInfo);

/*put.c*/

ST_ErrorCode_t STTUNER_SAT_StandByMode (STTUNER_Handle_t Handle,STTUNER_StandByMode_t PowerMode);


/* set.c */
ST_ErrorCode_t STTUNER_SAT_SetBandList     (STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList);
ST_ErrorCode_t STTUNER_SAT_SetFrequency    (STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams, U32 Timeout);
ST_ErrorCode_t STTUNER_SAT_SetLNBConfig (STTUNER_Handle_t Handle, STTUNER_LNBToneState_t LNBToneState, STTUNER_Polarization_t LNBVoltage);
ST_ErrorCode_t STTUNER_SAT_SetScanList     (STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList);
ST_ErrorCode_t STTUNER_SAT_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t QualityFormat);
#ifdef STTUNER_DRV_SAT_SCR
ST_ErrorCode_t STTUNER_SAT_SetScrTone(STTUNER_Handle_t Handle);
ST_ErrorCode_t STTUNER_SAT_SCRPARAM_UPDATE(STTUNER_Handle_t Handle,STTUNER_SCRBPF_t SCR,U16* SCRBPFFrequencies);
#endif


/* ---------- support functions ---------- */

/* utils.c */
ST_ErrorCode_t STTUNER_SAT_Utils_InitParamCheck(STTUNER_InitParams_t *InitParams);
ST_ErrorCode_t STTUNER_SAT_Utils_CleanInstHandle(STTUNER_Handle_t Handle);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SATAPI_H */

/* End of satapi.h */
