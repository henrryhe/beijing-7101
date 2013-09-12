/* ----------------------------------------------------------------------------
File Name: put.c

Description:

     This module handles the ter management functions for the STAPI level
    interface for STTUNER for setting parameters.


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


#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_TER_STV0360
	#include "d0360.h"
	
	#endif
  #endif	
#ifndef STTUNER_MINIDRIVER
   #include "stevt.h"
        /* generic utility functions for sttuner */
#endif


#include "tertask.h"    /* scan task */
#include "terapi.h"     /* terrestrial manager API (functions in this file exported via this header) */



#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif
 
/* ----------------------------------------------------------------------------
Name: STTUNER_SetBandList()
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
ST_ErrorCode_t STTUNER_TERR_SetBandList(STTUNER_Handle_t Handle, const STTUNER_BandList_t *BandList)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    const char *identity = "STTUNER put.c STTUNER_TERR_SetBandList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if(BandList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
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


            STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);


            Inst[Handle].BandList.NumElements = BandList->NumElements;

            nbytes = BandList->NumElements * sizeof(STTUNER_Band_t);

            memcpy(Inst[Handle].BandList.BandList, BandList->BandList, nbytes);
            

            STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);


        }
        else
        {
            /* Scan task is busy */
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
            STTBX_Print(("%s fail Scan task is busy\n", identity));
#endif
            Error = ST_ERROR_DEVICE_BUSY;
        }
    }
    else
    {
        /* The handle is invalid */
        Error = ST_ERROR_INVALID_HANDLE;
    }

    return(Error);

} /* STTUNER_TERR_SetBandList() */



/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_SetFrequency()
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
ST_ErrorCode_t STTUNER_TERR_SetFrequency(STTUNER_Handle_t Handle, U32 Frequency, STTUNER_Scan_t *ScanParams, U32 Timeout)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    const char *identity = "STTUNER put.c STTUNER_TERR_SetFrequency()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ScanParams == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
        STTBX_Print(("%s fail ScanParams is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    /* Set internal control block parameters */
    /* ISSUE: scan +/- 3MHz from this frequency for tuner tolerence? (ioctl on/off/setvalue it) */
    Inst[Handle].Terr.ScanInfo.FrequencyStart = Frequency;
    Inst[Handle].Terr.ScanInfo.FrequencyEnd   = Frequency;
    Inst[Handle].Terr.Timeout    = Timeout;
    Inst[Handle].Terr.SingleScan = *ScanParams;
    Inst[Handle].Terr.ScanExact  = TRUE;

    /* Ensure the tuner is not already scanning */
    if (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_SCANNING)
    {
    	
      #ifndef STTUNER_MINIDRIVER
       TERTASK_ScanAbort(Handle);      /* We must first abort the current scanning operation */
      #endif
    }

    Error = TERTASK_ScanStart(Handle);  /* Commence a scan with the required parameters */


#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* STTUNER_TERR_SetFrequency() */


/*#define DEBUG_STANDBYMODE*/
/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_StandByMode()
Description:
    To put terrestrial device in Standby Mode or out of it .
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


ST_ErrorCode_t STTUNER_TERR_StandByMode(STTUNER_Handle_t Handle, STTUNER_StandByMode_t PowerMode)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    const char *identity = "STTUNER put.c STTUNER_TERR_StandByMode()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int task_state = 0;
   
    Inst = STTUNER_GetDrvInst();
   
    STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);
   
    if (PowerMode == STTUNER_STANDBY_POWER_MODE)
    {
       if (Inst[Handle].Task_Run == 0 )
       {
       #ifdef DEBUG_STANDBYMODE
       STTBX_Print(("\n Device going in standby mode\n"));
       #endif
        Error  = (Inst[Handle].Terr.Demod.Driver->demod_StandByMode)(Inst[Handle].Terr.Demod.DrvHandle,STTUNER_STANDBY_POWER_MODE);
           if(Error != ST_NO_ERROR)
        {
        	return(Error);
        }                    
       Inst[Handle].PreStatus = Inst[Handle].TunerInfo.Status;
       Inst[Handle].PowerMode = STTUNER_STANDBY_POWER_MODE;
       Inst[Handle].TunerInfo.Status = STTUNER_STATUS_STANDBY ; 
       STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);
      
        
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
       STTBX_Print(("\n Device has resumed in normal mode\n"));
       #endif
       Error  = (Inst[Handle].Terr.Demod.Driver->demod_StandByMode)(Inst[Handle].Terr.Demod.DrvHandle,STTUNER_NORMAL_POWER_MODE);
          if(Error != ST_NO_ERROR)
        {
        	return(Error);
        }     
       
        Inst[Handle].PowerMode = STTUNER_NORMAL_POWER_MODE;
       
         if(Inst[Handle].PreStatus == STTUNER_STATUS_IDLE)
         {
           Inst[Handle].TunerInfo.Status = STTUNER_STATUS_IDLE;
         }
         else
         {
            Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING ; 
         }  
        
        STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);
         
         
      
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
  
     STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);
    
     
     #ifdef DEBUG_STANDBYMODE
          STTBX_Print(("\n Try to Sync scan task with main task after Scan task resume \n"));
     #endif
    
     STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);
     
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    
    return(Error);

} /* STTUNER_TERR_StandByMode() */


/* ----------------------------------------------------------------------------
ame: STTUNER_TERR_SetScanList()
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
ST_ErrorCode_t STTUNER_TERR_SetScanList(STTUNER_Handle_t Handle, const STTUNER_ScanList_t *ScanList)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    const char *identity = "STTUNER put.c STTUNER_TERR_SetScanList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ScanList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
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
            STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);


            Inst[Handle].ScanList.NumElements = ScanList->NumElements;

            nbytes = ScanList->NumElements * sizeof(STTUNER_Scan_t);

            memcpy(Inst[Handle].ScanList.ScanList, ScanList->ScanList, nbytes);
           

            STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);


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


#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* STTUNER_TERR_SetScanList() */




/* ----------------------------------------------------------------------------
Name: STTUNER_TERR_SetThresholdList()
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
ST_ErrorCode_t STTUNER_TERR_SetThresholdList(STTUNER_Handle_t Handle, const STTUNER_ThresholdList_t *ThresholdList, STTUNER_QualityFormat_t QualityFormat)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
    const char *identity = "STTUNER put.c STTUNER_TERR_SetThresholdList()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 nbytes;

    Inst = STTUNER_GetDrvInst();

    /* Check the parameters */
    if (ThresholdList == NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
        STTBX_Print(("%s fail ThresholdList is null\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if ( (QualityFormat != STTUNER_QUALITY_BER) &&
         (QualityFormat != STTUNER_QUALITY_CN)  )
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_PUT
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
      
            STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);
      

            Inst[Handle].ThresholdList.NumElements = ThresholdList->NumElements;

            nbytes = ThresholdList->NumElements * sizeof(STTUNER_SignalThreshold_t);

            memcpy(Inst[Handle].ThresholdList.ThresholdList, ThresholdList->ThresholdList, nbytes);

            Inst[Handle].QualityFormat = QualityFormat;

        STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);

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

} /* STTUNER_TERR_SetThresholdList() */





/* End of put.c */
