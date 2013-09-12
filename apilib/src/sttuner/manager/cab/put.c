/* ----------------------------------------------------------------------------
File Name: put.c

Description:

     This module handles the cable management functions for the STAPI level
     interface for STTUNER for setting parameters.


Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */


/* Includes ---------------------------------------------------------------- */

#ifdef ST_OSLINUX
   #include "stos.h"
#else
/* C libs */
#include <string.h>

/* Standard includes */
#include "stlite.h"

/* STAPI */
#include "sttbx.h"
#endif
#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "cabtask.h"    /* scan task */
#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */



/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_SetBandList()
Description:
    Sets the current frequency bands and associated local oscillator
    frequencies.
Parameters:
    Handle, the handle of the tuner device.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_DEVICE_BUSY,       the band/lo frequencies can not be set
                                during a scan.
    ST_ERROR_BAD_PARAMETER      Handle or BandList_p NULL
See Also:
    STTUNER_GetBandList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_SetBandList(STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    const char *identity = "STTUNER put.c STTUNER_CABLE_SetBandList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if(BandList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail  BandList is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }


    /* Check device is idle before setting new list */
    if (Inst[Handle].TunerInfo.Status != STTUNER_STATUS_SCANNING)
    {
        /* Ensure the tuner is not currently scanning */
        if (Inst->Max_BandList >= BandList->NumElements)
        {

            /* Copy the supplied bank/LO frequencies into the tuner control block
            we must ensure we lock out the scan task whilst doing this */

            STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);


            Inst[Handle].BandList.NumElements = BandList->NumElements;

            nbytes = BandList->NumElements * sizeof(STTUNER_Band_t);

            memcpy(Inst[Handle].BandList.BandList, BandList->BandList, nbytes);


            STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);

        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;/* Number of elements exceeds maximum allowed */
        }
    }
    else
    {
         /* Scan task is busy */
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail Scan task is busy\n", identity));
#endif
        Error = ST_ERROR_DEVICE_BUSY;
    }

    return(Error);

} /* STTUNER_CABLE_SetBandList() */



/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_SetFrequency()
Description:
    Scan for an exact frequency.
Parameters:
    Handle,        handle to sttuner device
    Frequency,     required frequency to scan to
    ScanParams_p,  scan settings to use during scan
    Timeout,       timeout (in ms) to allow for lock
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_BAD_PARAMETER      one or more params was invalid.
See Also:
    STTUNER_Scan()
    STTUNER_ScanContinue()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_SetFrequency(STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    const char *identity = "STTUNER put.c STTUNER_CABLE_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ScanParams == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail ScanParams is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check the scan parameters are valid. Note that it does not
     make sense to allow xxx_ALL because we are scanning for an
     exact frequency.  We therefore reject the scan unless all
     parameters are exact.
    */
    if (    ScanParams->Modulation != STTUNER_MOD_QAM
        &&  ScanParams->Modulation != STTUNER_MOD_256QAM
        &&  ScanParams->Modulation != STTUNER_MOD_128QAM
        &&  ScanParams->Modulation != STTUNER_MOD_64QAM
        &&  ScanParams->Modulation != STTUNER_MOD_32QAM
        &&  ScanParams->Modulation != STTUNER_MOD_16QAM
        )
    {

#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail Modulation\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set internal control block parameters */
    /* ISSUE: scan +/- 3MHz from this frequency for tuner tolerence? (ioctl on/off/setvalue it) */
    Inst[Handle].Cable.ScanInfo.FrequencyStart  = Frequency;
    Inst[Handle].Cable.ScanInfo.FrequencyEnd    = Frequency;
    Inst[Handle].Cable.ScanInfo.FineSearch      = FALSE;
    Inst[Handle].Cable.Modulation               = ScanParams->Modulation;
    Inst[Handle].Cable.Timeout                  = Timeout;
    Inst[Handle].Cable.SingleScan               = *ScanParams;
    Inst[Handle].Cable.ScanExact                = TRUE;
    Inst[Handle].Cable.ScanInfo.J83             = ScanParams->J83;

    /* Ensure the tuner is not already scanning */
    if (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_SCANNING)
    {
        CABTASK_ScanAbort(Handle);      /* We must first abort the current scanning operation */
    }

    Error = CABTASK_ScanStart(Handle);  /* Commence a scan with the required parameters */

#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* STTUNER_CABLE_SetFrequency() */



#ifndef STTUNER_MINIDRIVER
/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_StandByMode()
Description:
    To put cable device in Standby Mode or out of it .
Parameters:
    Handle,        handle to sttuner device
    StandByMode,     Indicate in which mode the user want the 
                     device .
    
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    STTUNER_ERROR_UNSPECIFIED   when task_suspend and task_resume does not
                                occur succesfully
See Also:
    
---------------------------------------------------------------------------- */

ST_ErrorCode_t STTUNER_CABLE_StandByMode(STTUNER_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    const char *identity = "STTUNER put.c STTUNER_CABLE_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int task_state = 0;
   
    Inst = STTUNER_GetDrvInst();
   
    STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);
    
    if (PowerMode == STTUNER_STANDBY_POWER_MODE)
    {
       if (Inst[Handle].Task_Run == 0 )
       {
       #ifdef DEBUG_STANDBYMODE
       STTBX_Print(("\n Scan task has been suspended \n"));
       #endif
        Error  = (Inst[Handle].Cable.Demod.Driver->demod_StandByMode)(Inst[Handle].Cable.Demod.DrvHandle,STTUNER_STANDBY_POWER_MODE);
          
                  
       Inst[Handle].PreStatus = Inst[Handle].TunerInfo.Status;
       Inst[Handle].PowerMode = STTUNER_STANDBY_POWER_MODE;
         
       Inst[Handle].TunerInfo.Status = STTUNER_STATUS_STANDBY ; 
       
       STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);
      
       if(task_state == 0)
       {
          Inst[Handle].Task_Run = 1;
       }
       else
       {
          Error = STTUNER_ERROR_UNSPECIFIED;
          return(Error);
       }
          
       
       }
       else
       {
       	 #ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Task already suspended \n"));
         #endif
       }
       
    }
    else if (PowerMode == STTUNER_NORMAL_POWER_MODE)
    {
      
       if (Inst[Handle].Task_Run == 1)
       {
       #ifdef DEBUG_STANDBYMODE
       STTBX_Print(("\n Scan task has been resumed \n"));
       #endif
       Error  = (Inst[Handle].Cable.Demod.Driver->demod_StandByMode)(Inst[Handle].Cable.Demod.DrvHandle,STTUNER_NORMAL_POWER_MODE);
         
      Inst[Handle].PowerMode = STTUNER_NORMAL_POWER_MODE;
        
         
         if(Inst[Handle].PreStatus == STTUNER_STATUS_IDLE)
         {
           Inst[Handle].TunerInfo.Status = STTUNER_STATUS_IDLE;
         }
         else
         {
            Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING ; 
         }  
          STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);
             
       #ifdef DEBUG_STANDBYMODE
        STTBX_Print(("\n The value of task_state for task_resume is <%d> \n",task_state));
       #endif
       if(task_state == 0)
       {
          Inst[Handle].Task_Run = 0;
       }
       else
       {
          Error = STTUNER_ERROR_UNSPECIFIED;
          return(Error);
       }
       
       }
       else
       {
       	#ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Task already resumed \n"));
        #endif
       }
    }
    
     if (Error != ST_NO_ERROR)
     {
        return(Error);
     }
  /* STOS_TaskDelayUs(500000);*/
    
     STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);
     
    
     #ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Try to Sync scan task with main task after Scan task resume \n"));
     #endif
    
     STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);
     
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    
    return(Error);

} /* STTUNER_CABLE_StandByMode() */
#endif


/* ----------------------------------------------------------------------------
ame: STTUNER_CABLE_SetScanList()
Description:
    Sets the scanning criteria for a tuner scan.
Parameters:
    Handle,    the handle of the tuner device.
    ScanList,  pointer to a scan list reflecting the scanning criteria.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_DEVICE_BUSY,       the tuner is scanning, we do not allow
                                the parameters to be set during a scan.
    ST_ERROR_BAD_PARAMETER      Handle or ScanList_p NULL
See Also:
    STTUNER_GetScanList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_SetScanList(STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    const char *identity = "STTUNER put.c STTUNER_CABLE_SetScanList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ScanList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail ScanList is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check device is idle before setting new list */
    if (Inst[Handle].TunerInfo.Status != STTUNER_STATUS_SCANNING)
    {
        /* Ensure the tuner is not currently scanning */
        if (Inst->Max_ScanList >= ScanList->NumElements)
        {
            /* Copy the supplied scan list into the tuner control block */

            STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);


            Inst[Handle].ScanList.NumElements = ScanList->NumElements;

            nbytes = ScanList->NumElements * sizeof(STTUNER_Scan_t);

            memcpy(Inst[Handle].ScanList.ScanList, ScanList->ScanList, nbytes);


            STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);

        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER; /* Number of elements exceeds maximum allowed */
        }
    }
    else
    {
        Error = ST_ERROR_DEVICE_BUSY;   /* Scan task is busy */
    }


#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* STTUNER_CABLE_SetScanList() */




/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_SetThresholdList()
Description:
    Sets the current threshold list for checking the signal quality value.
    The caller-specificed callback may be invoked if the signal threshold
    changes position in the threshold list.
Parameters:
    Handle,         the handle of the tuner device.
    ThresholdList,  pointer to a threshold list for signal checking.
    QualityFormat,  units of measurement used in threshold list values.
Return Value:
    ST_NO_ERROR,                the operation was carried out without error.
    ST_ERROR_INVALID_HANDLE,    the handle was invalid.
    ST_ERROR_BAD_PARAMETER      Handle or ThresholdList_p NULL
See Also:
    STTUNER_GetThresholdList()
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t QualityFormat)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
    const char *identity = "STTUNER put.c STTUNER_CABLE_SetThresholdList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ThresholdList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail ThresholdList is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ( (QualityFormat != STTUNER_QUALITY_BER) &&
         (QualityFormat != STTUNER_QUALITY_CN)  )
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_PUT
        STTBX_Print(("%s fail QualityFormat not valid\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Check device is idle before setting new list */
    if (Inst[Handle].TunerInfo.Status != STTUNER_STATUS_SCANNING)
    {
        if (Inst->Max_ThresholdList >= ThresholdList->NumElements)
        {
            /* Copy the supplied threshold list into the tuner control block, also flag
             the tuner to begin signal checking at this point. */

            STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);


            Inst[Handle].ThresholdList.NumElements = ThresholdList->NumElements;

            nbytes = ThresholdList->NumElements * sizeof(STTUNER_SignalThreshold_t);

            memcpy(Inst[Handle].ThresholdList.ThresholdList, ThresholdList->ThresholdList, nbytes);

            Inst[Handle].QualityFormat = QualityFormat;


            STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);

        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER; /* Number of elements exceeds maximum allowed */
        }
    }
    else
    {
        Error = ST_ERROR_DEVICE_BUSY;   /* Scan task is busy */
    }


    return(Error);

} /* STTUNER_CABLE_SetThresholdList() */





/* End of put.c */
