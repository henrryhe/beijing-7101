/*---------------------------------------------------------------
File Name: terapi.h

Description:

     ter management functions to interface to STAPI STTUNER API layer.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 10-Sept-2001
version: 3.1.0
 author: GB
comment: new

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TERAPI_H
#define __STTUNER_TERAPI_H


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
ST_ErrorCode_t STTUNER_TERR_Init(STTUNER_Handle_t Handle, STTUNER_InitParams_t *InitParams);

/* term.c */
ST_ErrorCode_t STTUNER_TERR_Term(STTUNER_Handle_t Handle, const STTUNER_TermParams_t *TermParams);

/* open.c */
ST_ErrorCode_t STTUNER_TERR_Open(STTUNER_Handle_t Handle, const STTUNER_OpenParams_t *OpenParams);

/* close.c */
ST_ErrorCode_t STTUNER_TERR_Close(STTUNER_Handle_t Handle);

/* ioctl.c */
ST_ErrorCode_t STTUNER_TERR_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams);

/* scan.c */
ST_ErrorCode_t STTUNER_TERR_Scan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, U32 Timeout);
ST_ErrorCode_t STTUNER_TERR_ScanContinue(STTUNER_Handle_t Handle, U32 Timeout);
ST_ErrorCode_t STTUNER_TERR_Unlock(STTUNER_Handle_t Handle);

/* get.c */
ST_ErrorCode_t STTUNER_TERR_GetBandList     (STTUNER_Handle_t Handle, STTUNER_BandList_t      *BandList);
ST_ErrorCode_t STTUNER_TERR_GetCapability   (STTUNER_Handle_t Handle, STTUNER_Capability_t    *Capability);
ST_ErrorCode_t STTUNER_TERR_GetScanList     (STTUNER_Handle_t Handle, STTUNER_ScanList_t      *ScanList);
ST_ErrorCode_t STTUNER_TERR_GetStatus       (STTUNER_Handle_t Handle, STTUNER_Status_t        *Status);
ST_ErrorCode_t STTUNER_TERR_GetThresholdList(STTUNER_Handle_t Handle, STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t *QualityFormat);
ST_ErrorCode_t STTUNER_TERR_GetTunerInfo    (STTUNER_Handle_t Handle, STTUNER_TunerInfo_t     *TunerInfo);
#ifdef STTUNER_USE_TER
ST_ErrorCode_t STTUNER_TERR_GetTPSCellId    (STTUNER_Handle_t Handle,  U16     *TPSCellID);
#endif

/* set.c */
ST_ErrorCode_t STTUNER_TERR_SetBandList     (STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList);
ST_ErrorCode_t STTUNER_TERR_SetFrequency    (STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams, U32 Timeout);
ST_ErrorCode_t STTUNER_TERR_SetScanList     (STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList);
ST_ErrorCode_t STTUNER_TERR_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t QualityFormat);
ST_ErrorCode_t STTUNER_TERR_StandByMode (STTUNER_Handle_t Handle,STTUNER_StandByMode_t PowerMode);


/* ---------- support functions ---------- */

/* utils.c */
ST_ErrorCode_t STTUNER_TERR_Utils_InitParamCheck(STTUNER_InitParams_t *InitParams);
ST_ErrorCode_t STTUNER_TERR_Utils_CleanInstHandle(STTUNER_Handle_t Handle);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TERAPI_H */

/* End of terapi.h */
