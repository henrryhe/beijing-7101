/* ----------------------------------------------------------------------------
File Name: ioctl.c

Description:

     This module handles the cable management functions for the STAPI level
     interface for STTUNER for ioctl.

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

#include "cabapi.h"     /* cable manager API (functions in this file exported via this header) */
#include "swioctl.h"    /* software target for ioctl calls */

/* driver API*/
#include "drivers.h"
#include "ioarch.h"

/* local functions */

static ST_ErrorCode_t STTUNER_CABLE_Ioctl_ScanPri    (STTUNER_Handle_t Handle, void *InParams, void *OutParams);

/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Ioctl()

Description:

Parameters:

Return Value:

See Also:
---------------------------------------------------------------------------- */
ST_ErrorCode_t STTUNER_CABLE_Ioctl(STTUNER_Handle_t Handle, U32 DeviceID, U32 FunctionIndex, void *InParams, void *OutParams)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
    const char *identity = "STTUNER ioctl.c STTUNER_CABLE_Ioctl()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_Da_Status_t Status;

    DEMOD_Handle_t  DHandle;
    TUNER_Handle_t  THandle;

    Inst = STTUNER_GetDrvInst();


    /* although the decision to check for STTUNER_SOFT_ID could be done at the stapi.c level
      so that general functions can be called it is left to each individual service to
      filter (if it is programmed to do so) what can be called or not or to select a service
      specific software IOCTL function instead (via FunctionIndex) else call
      STTUNER_SWIOCTL_Default() */

    if (DeviceID == STTUNER_SOFT_ID)
    {
        switch(FunctionIndex)
        {

            case STTUNER_IOCTL_SCANPRI: /* get/set scan task priority */
                Error = STTUNER_CABLE_Ioctl_ScanPri(Handle, InParams, OutParams);
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
                STTBX_Print(("%s STTUNER_IOCTL_SCANPRI ioctl done\n", identity));
#endif
                return(Error);

             default:   /* no function indexes match so call default handler */
                Error = STTUNER_SWIOCTL_Default(Handle, FunctionIndex, InParams, OutParams, &Status);  /* default higher level function */
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
                STTBX_Print(("%s default ioctl done\n", identity));
#endif
                return(Error);
        }
    }
    /* else look to see if any device driver IDs match */


    /* ---------- look in this Instance for the device ID ---------- */

    DHandle = Inst[Handle].Cable.Demod.DrvHandle;
    if (Inst[Handle].Cable.Demod.Driver->ID == DeviceID)
    {
        Error = (Inst[Handle].Cable.Demod.Driver->demod_ioctl)(DHandle, FunctionIndex, InParams, OutParams, &Status);
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
        STTBX_Print(("%s demod ioctl done\n", identity));
#endif
        return(Error);
    }


    THandle = Inst[Handle].Cable.Tuner.DrvHandle;
    if (Inst[Handle].Cable.Tuner.Driver->ID == DeviceID)
    {
        Error = (Inst[Handle].Cable.Tuner.Driver->tuner_ioctl)(THandle, FunctionIndex, InParams, OutParams, &Status);
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
        STTBX_Print(("%s tuner ioctl done\n", identity));
#endif
        return(Error);
    }

#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
        STTBX_Print(("%s fail no such device in this instance %d\n", identity, DeviceID));
#endif


   return(STTUNER_ERROR_ID);

} /* STTUNER_CABLE_Ioctl() */



/* ========================================================================= */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\IOCTL Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ========================================================================= */



/* ----------------------------------------------------------------------------
Name: STTUNER_CABLE_Ioctl_ScanPri()

Description: get/set scan task priority.

Parameters:
    Handle    - STTUNER instance (opened)
    InParams  - if not NULL then sets scan task priority.
    OutParams - if not NULL then returns current scan task priority.

Return Value:

See Also:
---------------------------------------------------------------------------- */
static ST_ErrorCode_t STTUNER_CABLE_Ioctl_ScanPri(STTUNER_Handle_t Handle, void *InParams, void *OutParams)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
    const char *identity = "STTUNER ioctl.c STTUNER_CABLE_Ioctl_ScanPri()";
#endif
    STTUNER_InstanceDbase_t *Inst;
    task_t *task;

    Inst = STTUNER_GetDrvInst();

    task = Inst[Handle].Cable.ScanTask.Task;


    if(InParams != NULL)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
        STTBX_Print(("%s Setting scan task priority to %d\n", identity, *(int *)InParams ));
#endif
        /* Lock out the scan task from doing anything worthy before modifying its priority */

        STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);

#if !defined(ST_OSLINUX) 
        task_priority_set(task, *(int *)InParams);
#endif
        /* Release semaphore -- allows scan task to resume */

        STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);


    }

    if(OutParams != NULL)
    {
    	#if !defined(ST_OSLINUX)    
        *(int *)OutParams = task_priority(task);
        #endif
#ifdef STTUNER_DEBUG_MODULE_CAB_IOCTL
        STTBX_Print(("%s returning current scan task priority of %d\n", identity, *(int *)OutParams ));
#endif
    }

   return(ST_NO_ERROR);

}




/* End of ioctl.c */

