/* ----------------------------------------------------------------------------
File Name: get.c

Description:

     This module handles the ter management functions for the STAPI level
    interface for STTUNER for getting parameters.


Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 12-Sept-2001
version: 3.1.0
 author: GB from work by GJP
comment: rewrite for terrestrial.

Reference:

    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */
#ifndef ST_OSLINUX
   
/* C libs */
#include <string.h>
#endif
/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "tertask.h"    /* scan task */
#include "terapi.h"     /* terrestrial manager API (functions in this file exported via this header) */


#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_GetBandList()
Description:
    Obtains the current frequency bands and associated local oscillator
    frequencies.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error
    ST_ERROR_INVALID_HANDLE,    the handle was invalid
    ST_ERROR_BAD_PARAMETER      Handle or BandList_p NULL
See Also:
    STTUNER_SetBandList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_GetBandList(STTUNER_Handle_t Handle, STTUNER_BandList_t *BandList)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
    const char *identity = "STTUNER get.c STTUNER_TERR_GetBandList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (BandList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
        STTBX_Print(("%s fail bandlist is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Copy the band/lo frequency list to the caller supplied area */
    BandList->NumElements = Inst[Handle].BandList.NumElements;

    nbytes = Inst[Handle].BandList.NumElements * sizeof(STTUNER_Band_t);

    memcpy(BandList->BandList, Inst[Handle].BandList.BandList, nbytes);

    return(Error);

} /* STTUNER_TERR_GetBandList() */
#endif

#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_GetScanList()
Description:
    Obtains the ordered scan list that dictates the criteria during a
    tuner scan at each frequency step.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error
    ST_ERROR_INVALID_HANDLE,    the handle was invalid
    ST_ERROR_BAD_PARAMETER      Handle or ScanList_p NULL
See Also:
    STTUNER_SetScanList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_GetScanList(STTUNER_Handle_t Handle, STTUNER_ScanList_t *ScanList)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
    const char *identity = "STTUNER get.c STTUNER_TERR_GetScanList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ScanList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
        STTBX_Print(("%s fail scanlist is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Copy the scan list for the caller */
    ScanList->NumElements = Inst[Handle].ScanList.NumElements;

    nbytes = Inst[Handle].ScanList.NumElements * sizeof(STTUNER_Scan_t);

    memcpy(ScanList->ScanList, Inst[Handle].ScanList.ScanList, nbytes);

    return(Error);

} /* STTUNER_TERR_GetScanList() */

#endif
#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_GetThresholdList()
Description:
    Obtains the ordered threshold list that contain the high/low signal
    quality thresholds for notifying signal quality change events.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_BAD_PARAMETER      Handle or ThresholdList_p or QualityFormat_p NULL
See Also:
    STTUNER_SetScanList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_GetThresholdList(STTUNER_Handle_t Handle, STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t *QualityFormat)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
    const char *identity = "STTUNER get.c STTUNER_TERR_GetThresholdList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();


    /* Check the parameters */
    if( (ThresholdList == NULL) || (QualityFormat == NULL) )
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
        STTBX_Print(("%s fail ThresholdList or QualityFormat is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

        /* Copy the information for the caller */
    ThresholdList->NumElements = Inst[Handle].ThresholdList.NumElements;

    nbytes = Inst[Handle].ThresholdList.NumElements * sizeof(STTUNER_SignalThreshold_t);

    memcpy(ThresholdList->ThresholdList, Inst[Handle].ThresholdList.ThresholdList,  nbytes);

    *QualityFormat = Inst[Handle].QualityFormat;

    return(Error);

} /* STTUNER_TERR_GetThresholdList() */
#endif


/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_GetStatus()
Description:
    Obtains the current status of driver i.e., whether the tuner is locked,
    unlocked, scanning or failed.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error
    ST_ERROR_INVALID_HANDLE,    the handle was invalid
    ST_ERROR_BAD_PARAMETER      Handle or Status_p NULL
See Also:
    Nothing.
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_GetStatus(STTUNER_Handle_t Handle, STTUNER_Status_t *Status)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
    const char *identity = "STTUNER get.c STTUNER_TERR_GetStatus()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

     /* Check the parameters */
    if (Status == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
        STTBX_Print(("%s fail Status is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set the status for the caller */
    *Status = Inst[Handle].TunerInfo.Status;

    return(Error);

} /* STTUNER_TERR_GetStatus() */



/* ----------------------------------------------------------------------------
Name: STTUNER_GetTunerInfo()
Description:
    Obtains the current scanning status, even during a scan.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error
    ST_ERROR_INVALID_HANDLE,    the handle was invalid
    ST_ERROR_BAD_PARAMETER      Handle or TunerInfo_p NULL
See Also:
    Nothing.
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_GetTunerInfo(STTUNER_Handle_t Handle, STTUNER_TunerInfo_t *TunerInfo)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
    const char *identity = "STTUNER get.c STTUNER_TERR_GetTunerInfo()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    
    Inst = STTUNER_GetDrvInst();
    
    /* Check the parameters */
    if (TunerInfo == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
        STTBX_Print(("%s fail TunerInfo is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

        /* Copy the current tuner information to the user */
         *TunerInfo = Inst[Handle].TunerInfo;
             
         Error=Inst[Handle].TunerInfoError;
    
    
    

    return(Error);

} /* STTUNER_TERR_GetTunerInfo() */

#ifndef STTUNER_MINIDRIVER 
#ifdef STTUNER_USE_TER
/* ----------------------------------------------------------------------------
Name: STTUNER_TER_GetTPSCellId()
Description:
    
Parameters:
    
Return Value:
    ST_NO_ERROR,                the operation was carried out without error
    ST_ERROR_INVALID_HANDLE,    the handle was invalid
    ST_ERROR_BAD_PARAMETER      Handle or TunerInfo_p NULL
See Also:
    Nothing.
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_GetTPSCellId    (STTUNER_Handle_t Handle,  U16     *TPSCellID)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
    const char *identity = "STTUNER get.c STTUNER_TERR_GetTPSCellId ()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

     /* Check the parameters */
    if (TPSCellID == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_GET
        STTBX_Print(("%s fail Status is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   /********Semaphore given for task sync between main task and scan task***********/ 
    
 
     /* Get the TPS Cell ID */
 
    STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);
    
    /* Get the TPS Cell ID */
   Error  = (Inst[Handle].Terr.Demod.Driver->demod_GetTPSCellId)(Inst[Handle].Terr.Demod.DrvHandle, 
                                                          TPSCellID);
 
 
  STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);
  

    return(Error);


}
#endif
#endif
/* End of get.c */
