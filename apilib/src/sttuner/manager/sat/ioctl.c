/* ----------------------------------------------------------------------------
File Name: ioctl.c

Description: 

     This module handles the sat management functions for the STAPI level
    interface for STTUNER for ioctl.


Copyright (C) 1999-2001 STMicroelectronics

History:
 
   date: 20-June-2001
version: 3.1.0
 author: GJP from work by LW
comment: rewrite for multi-instance.
    
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

#include "satapi.h"     /* satellite manager API (functions in this file exported via this header) */
#include "swioctl.h"    /* software target for ioctl calls */

/* driver API*/
#include "drivers.h"
#include "ioarch.h"


/* local functions */

static ST_ErrorCode_t STTUNER_SAT_Ioctl_ScanPri    (STTUNER_Handle_t Handle, void *InParams, void *OutParams);
static ST_ErrorCode_t STTUNER_SAT_Ioctl_SignalNoise(STTUNER_Handle_t Handle, void *InParams, void *OutParams);
static ST_ErrorCode_t STTUNER_SAT_Ioctl_TunerStep  (STTUNER_Handle_t Handle, void *InParams, void *OutParams);

/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Ioctl()

Description:
    
Parameters:
    
Return Value:
    
See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_SAT_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
    const char *identity = "STTUNER ioctl.c STTUNER_SAT_Ioctl()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_Da_Status_t Status;

    DEMOD_Handle_t  DHandle;
    TUNER_Handle_t  THandle;
    LNB_Handle_t    LHandle;


    /* although the decision to check for STTUNER_SOFT_ID could be done at the stapi.c level
      so that general functions can be called it is left to each individual service to
      filter (if it is programmed to do so) what can be called or not or to select a service
      specific software IOCTL function instead (via FunctionIndex) else call 
      STTUNER_SWIOCTL_Default() */

    if (DeviceID == STTUNER_SOFT_ID)
    {
        switch(FunctionIndex){

            case STTUNER_IOCTL_SCANPRI: /* get/set scan task priority */
                Error = STTUNER_SAT_Ioctl_ScanPri(Handle, InParams, OutParams);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
                STTBX_Print(("%s STTUNER_IOCTL_SCANPRI ioctl done\n", identity));
#endif
                return(Error); 

            case STTUNER_IOCTL_TEST_SN: /* enable/disable test mode: geenrarte random BER rates */
                Error = STTUNER_SAT_Ioctl_SignalNoise(Handle, InParams, OutParams);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
                STTBX_Print(("%s STTUNER_IOCTL_TEST_SN ioctl done\n", identity));
#endif
                return(Error); 

            case STTUNER_IOCTL_TUNERSTEP: /* tuner step size for ScanContinue in scantask */
                Error = STTUNER_SAT_Ioctl_TunerStep(Handle, InParams, OutParams);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
                STTBX_Print(("%s STTUNER_IOCTL_TUNERSTEP ioctl done\n", identity));
#endif
                return(Error);

            case STTUNER_IOCTL_SEARCH_TEST: /* simulate the lock in tuner search loop */
                Inst = STTUNER_GetDrvInst();
                DHandle = Inst[Handle].Sat.Demod.DrvHandle;
                Error = (Inst[Handle].Sat.Demod.Driver->demod_ioctl)(DHandle, FunctionIndex, InParams, OutParams, &Status);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
                STTBX_Print(("%s STTUNER_IOCTL_SEARCH_TEST ioctl done\n", identity));
#endif
                return(Error); 

             default:   /* no function indexes match so call default handler */
                Error = STTUNER_SWIOCTL_Default(Handle, FunctionIndex, InParams, OutParams, &Status);  /* default higher level function */
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
                STTBX_Print(("%s default ioctl done\n", identity));
#endif
                return(Error); 
        }



    }

    /* else look to see if any device driver IDs in this instance match */

    Inst = STTUNER_GetDrvInst();

    /* ---------- look in this Instance for the device ID ---------- */

    DHandle = Inst[Handle].Sat.Demod.DrvHandle;
    if (Inst[Handle].Sat.Demod.Driver->ID == DeviceID)  
    {
        Error = (Inst[Handle].Sat.Demod.Driver->demod_ioctl)(DHandle, FunctionIndex, InParams, OutParams, &Status);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s demod ioctl done\n", identity));
#endif
        return(Error);
    }


    THandle = Inst[Handle].Sat.Tuner.DrvHandle;
    if (Inst[Handle].Sat.Tuner.Driver->ID == DeviceID)
    {
        Error = (Inst[Handle].Sat.Tuner.Driver->tuner_ioctl)(THandle, FunctionIndex, InParams, OutParams, &Status);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s tuner ioctl done\n", identity));
#endif
        return(Error);
    }


    LHandle = Inst[Handle].Sat.Lnb.DrvHandle;
    if (Inst[Handle].Sat.Lnb.Driver->ID == DeviceID)
    {
        Error = (Inst[Handle].Sat.Lnb.Driver->lnb_ioctl)(LHandle, FunctionIndex, InParams, OutParams, &Status);
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s lnb ioctl done\n", identity));
#endif
        return(Error);
    }


#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s fail no such device in this instance %d\n", identity, DeviceID));
#endif


   return(STTUNER_ERROR_ID);

} /* STTUNER_SAT_Ioctl() */



/* ========================================================================= */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\IOCTL Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ========================================================================= */



/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Ioctl_ScanPri()

Description: get/set scan task priority.
    
Parameters: 
    Handle    - STTUNER instance (opened)
    InParams  - if not NULL then sets scan task priority.
    OutParams - if not NULL then returns current scan task priority.

Return Value:
    
See Also:
---------------------------------------------------------------------------- */
static ST_ErrorCode_t STTUNER_SAT_Ioctl_ScanPri(STTUNER_Handle_t Handle, void *InParams, void *OutParams)
{               
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
    const char *identity = "STTUNER ioctl.c STTUNER_SAT_Ioctl_ScanPri()";
#endif
    STTUNER_InstanceDbase_t *Inst;
    task_t *task;

    Inst = STTUNER_GetDrvInst();
 
    task = Inst[Handle].Sat.ScanTask.Task;





    if(InParams != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s Setting scan task priority to %d\n", identity, *(int *)InParams ));
#endif
        /* Lock out the scan task from doing anything worthy before modifying its priority */

        STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);


#if !defined(ST_OSLINUX) 
        task_priority_set(task, *(int *)InParams);
#endif        

        /* Release semaphore -- allows scan task to resume */
 
        STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
 

    }

    if(OutParams != NULL)
    {
#if !defined(ST_OSLINUX)
    	
        *(int *)OutParams = task_priority(task);
#else
        *(int *)OutParams = 0;/* which number is here? */
#endif
       
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s returning current scan task priority of %d\n", identity, *(int *)OutParams ));
#endif
    }    

   return(ST_NO_ERROR);

}



/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Ioctl_TunerStep()

Description: get/set scan task tuner step size.
    
Parameters: 
    Handle    - STTUNER instance (opened)
    InParams  - if not NULL then set tuner step size ( 0 == use default step size).
    OutParams - if not NULL then return tuner step size.

Return Value:
    
See Also:
---------------------------------------------------------------------------- */
static ST_ErrorCode_t STTUNER_SAT_Ioctl_TunerStep(STTUNER_Handle_t Handle, void *InParams, void *OutParams)
{               
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
    const char *identity = "STTUNER ioctl.c STTUNER_SAT_Ioctl_ScanPri()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    if(InParams != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s Setting default tuner step size to %d\n", identity, *(int *)InParams ));
#endif
        Inst[Handle].Sat.Ioctl.TunerStepSize = *(U32 *)InParams;
    }

    if(OutParams != NULL)
    {
        *(U32 *)OutParams = Inst[Handle].Sat.Ioctl.TunerStepSize;
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s returning current tuner step size of %d\n", identity, *(int *)OutParams ));
#endif
    }    

   return(ST_NO_ERROR);

}



/* ----------------------------------------------------------------------------
Name: STTUNER_SAT_Ioctl_SignalNoise()

Description: get/set test mode enable for generating random BER for unitary test.
    
Parameters: 
    Handle    - STTUNER instance (opened)
    InParams  - if not NULL then sets 'SignalNoise' test mode.
    OutParams - if not NULL then returns current 'SignalNoise' test mode.

Return Value:
    
See Also:
---------------------------------------------------------------------------- */
static ST_ErrorCode_t STTUNER_SAT_Ioctl_SignalNoise(STTUNER_Handle_t Handle, void *InParams, void *OutParams)
{               
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
    const char *identity = "STTUNER ioctl.c STTUNER_SAT_Ioctl_SignalNoise()";
#endif
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    if(InParams != NULL)
    {
        if ((*(BOOL *)InParams != TRUE) && (*(BOOL *)InParams != FALSE))
        {
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
            STTBX_Print(("%s fail setting 'SignalNoise' not TRUE or FALSE %d\n", identity, *(BOOL *)InParams ));
#endif
            return(ST_ERROR_BAD_PARAMETER);
        }
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s Setting scan task priority to %d\n", identity, *(BOOL *)InParams ));
#endif
        Inst[Handle].Sat.Ioctl.SignalNoise = *(BOOL *)InParams;
    }

    if(OutParams != NULL)
    {
        *(BOOL *)OutParams = Inst[Handle].Sat.Ioctl.SignalNoise;
#ifdef STTUNER_DEBUG_MODULE_SAT_IOCTL
        STTBX_Print(("%s returning current 'SignalNoise' test mode of %d\n", identity, *(BOOL *)OutParams ));
#endif
    }    

   return(ST_NO_ERROR);

}



/* End of ioctl.c */
