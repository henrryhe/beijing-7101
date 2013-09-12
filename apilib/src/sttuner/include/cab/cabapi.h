/*---------------------------------------------------------------
File Name: cabapi.h

Description: 

     cable management functions to interface to STAPI STTUNER API layer.

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
#ifndef __STTUNER_CABAPI_H
#define __STTUNER_CABAPI_H


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
ST_ErrorCode_t STTUNER_CABLE_Init(STTUNER_Handle_t Handle, STTUNER_InitParams_t *InitParams);

/* term.c */
ST_ErrorCode_t STTUNER_CABLE_Term(STTUNER_Handle_t Handle, const STTUNER_TermParams_t *TermParams);

/* open.c */
ST_ErrorCode_t STTUNER_CABLE_Open(STTUNER_Handle_t Handle, const STTUNER_OpenParams_t *OpenParams);

/* close.c */
ST_ErrorCode_t STTUNER_CABLE_Close(STTUNER_Handle_t Handle);

/* ioctl.c */
ST_ErrorCode_t STTUNER_CABLE_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams);

/* scan.c */
ST_ErrorCode_t STTUNER_CABLE_Scan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, U32 Timeout);
ST_ErrorCode_t STTUNER_CABLE_ScanContinue(STTUNER_Handle_t Handle, U32 Timeout);
ST_ErrorCode_t STTUNER_CABLE_Unlock(STTUNER_Handle_t Handle);

/* get.c */
ST_ErrorCode_t STTUNER_CABLE_GetBandList     (STTUNER_Handle_t Handle, STTUNER_BandList_t      *BandList);
ST_ErrorCode_t STTUNER_CABLE_GetCapability   (STTUNER_Handle_t Handle, STTUNER_Capability_t    *Capability);
ST_ErrorCode_t STTUNER_CABLE_GetScanList     (STTUNER_Handle_t Handle, STTUNER_ScanList_t      *ScanList); 
ST_ErrorCode_t STTUNER_CABLE_GetStatus       (STTUNER_Handle_t Handle, STTUNER_Status_t        *Status);
ST_ErrorCode_t STTUNER_CABLE_GetThresholdList(STTUNER_Handle_t Handle, STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t *QualityFormat);
ST_ErrorCode_t STTUNER_CABLE_GetTunerInfo    (STTUNER_Handle_t Handle, STTUNER_TunerInfo_t     *TunerInfo);

/* set.c */
ST_ErrorCode_t STTUNER_CABLE_SetBandList     (STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList);
ST_ErrorCode_t STTUNER_CABLE_SetFrequency    (STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams, U32 Timeout);
ST_ErrorCode_t STTUNER_CABLE_SetScanList     (STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList);
ST_ErrorCode_t STTUNER_CABLE_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t QualityFormat);

/*put.c*/
ST_ErrorCode_t STTUNER_CABLE_StandByMode (STTUNER_Handle_t Handle,STTUNER_StandByMode_t PowerMode);


/* ---------- support functions ---------- */

/* utils.c */
ST_ErrorCode_t STTUNER_CABLE_Utils_InitParamCheck(STTUNER_InitParams_t *InitParams);
ST_ErrorCode_t STTUNER_CABLE_Utils_CleanInstHandle(STTUNER_Handle_t Handle);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CABAPI_H */

/* End of cabapi.h */
