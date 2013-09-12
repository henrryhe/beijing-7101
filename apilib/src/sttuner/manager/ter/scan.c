/* ----------------------------------------------------------------------------
File Name: scan.c

Description:

     This module handles the ter management functions for the STAPI level
    interface for STTUNER for scanning.


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



/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Scan()

Description:
    This routine commences a scan starting from the "FreqFrom" and
    iterates in steps of "FreqStep" until "FreqTo".

    It is non-blocking i.e., returns to the caller immediately.  A callback
    routine may be invoked with an appropriate error code, if either:

    a) The tuner has just lost lock.
    b) The scan completed without a lock i.e., "not found".
    c) The scan has got a lock.
    d) The scan has timed out.

    If no callback is setup, the caller may interrogate the progress of the
    scan by calling STTUNER_GetStatus() at any time.

    NOTE:   The caller can advance to the next scan frequency by using the
            STTUNER_ScanContinue() call.
Parameters:
    Handle,     the handle of the tuner device.
    FreqFrom,   Start frequency.
    FreqTo,     End frequency.
    FreqStep,   Frequency step - not used at present.
    Timeout,    timeout period (in ms) to allow for lock.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_BAD_PARAMETER,     one of the frequency parameters passed in
                                is invalid.
See Also:
    STTUNER_ScanContinue()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_Scan(STTUNER_Handle_t Handle, U32 FreqFrom, U32 FreqTo, U32 FreqStep, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_SCAN
    const char *identity = "STTUNER scan.c STTUNER_TERR_Scan()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    /* Check the parameters */
    if(FreqFrom == 0)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_SCAN
        STTBX_Print(("%s fail FreqFrom is zero\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    Inst = STTUNER_GetDrvInst();

    /* Determine scan step direction for the scan loop */
    if (FreqFrom <= FreqTo)
    {
        /* Ascending direction (multiply step by 1) */
        Inst[Handle].Terr.ScanInfo.ScanDirection = 1;
    }
    else
    {
        /* Descending direction (multiply step by -1) */
        Inst[Handle].Terr.ScanInfo.ScanDirection = -1;
    }

    Inst[Handle].Terr.ScanInfo.FrequencyStart = FreqFrom;
    Inst[Handle].Terr.ScanInfo.FrequencyEnd   = FreqTo;
    Inst[Handle].Terr.ScanInfo.FrequencyStep  = FreqStep;
        
    Inst[Handle].Terr.Timeout   = Timeout;
    Inst[Handle].Terr.ScanExact = FALSE ; 
    
    /* Give ChannelBW to ScanInfo for the "GetTunerInfo" function */
    Inst[Handle].TunerInfo.ScanInfo.ChannelBW =Inst[Handle].ScanList.ScanList->ChannelBW;

    /* Ensure the tuner is not already scanning */
    if (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_SCANNING)
    {
        TERTASK_ScanAbort(Handle);      /* We must first abort the current scanning operation */
    }

    Error = TERTASK_ScanStart(Handle);  /* Commence a scan with the required parameters */

    return(Error);

} /* STTUNER_TERR_Scan() */



/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_ScanContinue()
Description:
    This routine is used to continue a previously invoked scan operatation.
    It is non-blocking i.e., returns to the caller immediately -- see
    STTUNER_Scan() for more information.

    NOTE:   STTUNER_ScanContinue() will return an error if the previous
            scan step has not completed.

Parameters:
    Handle,   the handle of the tuner device.
    Timeout,  timeout period (in ms) to allow for lock.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_DEVICE_BUSY,       the previous scan step is still in progress.
    ST_ERROR_BAD_PARAMETER,     the end of the scan list has been reached, or
                                a scan has not yet been initiated.
See Also:
    STTUNER_Scan()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_ScanContinue(STTUNER_Handle_t Handle, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_SCAN
    const char *identity = "STTUNER scan.c STTUNER_TERR_ScanContinue()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    /* Attempt to continue for the next scan */
     Inst[Handle].Terr.Timeout   = Timeout;
     Inst[Handle].Terr.ScanExact = FALSE;

    Error = TERTASK_ScanContinue(Handle);


#ifdef STTUNER_DEBUG_MODULE_TERR_SCAN
        STTBX_Print(("%sdone ok\n", identity));
#endif
    return(Error);

} /* STTUNER_TERR_ScanContinue() */



/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_Unlock()

Description:
    Aborts the current scan i.e., the scan status will subsequently become
    'unlocked'.
Parameters:
    Handle,     the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation completed without error.
    ST_ERROR_INVALID_HANDLE,    the handle is invalid.
    ST_ERROR_BAD_PARAMETER      Handle NULL
See Also:
    Nothing.
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_TERR_Unlock(STTUNER_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    Error = TERTASK_ScanAbort(Handle);

    return(Error);
} /* STTUNER_TERR_Unlock() */



/* End of scan.c */
