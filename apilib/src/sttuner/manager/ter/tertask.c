/*----------------------------------------------------------------------------
File Name: tertask.c (was tuner.c)

Description: Wrapper API for interfacing with the low-level tuner modules.

Copyright (C) 1999 STMicroelectronics

Version: 3.1.0

Revision History:

    24/01/00    FEC rates are now setting during ScanExact() i.e., per
                scan list element rather than a global OR of all
                FEC rates for all scan list elements.

    9/07/2001   re-wrote as part of  version 3.1.0 STTUNER.

    4/08/2001   Added retries for single scan and automatic relocking
    27/12/2004  Symbol rate calculation removed - CC 

Reference:

ST API Definition "TUNER Driver API" DVD-API-06
----------------------------------------------------------------------------*/

/* Includes --------------------------------------------------------------- */

#ifndef ST_OSLINUX
/* C libs */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#endif
/* STAPI */
#include "stlite.h"
#include "sttbx.h"
#include "stcommon.h"


#include "sttuner.h"

/* local to sttuner */
     /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "tertask.h"   /* header for this file */

/* Standard includes */

#ifndef STTUNER_MINIDRIVER
#include "util.h"       /* generic utility functions for sttuner */
#include "stevt.h"
#endif
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_TER_STV0360
	#include "360_echo.h" 
	#include "360_drv.h"
	#include "d0360.h"
  #endif
#endif


/* Private types/constants ------------------------------------------------ */

/* Digital bandwidth roll-off */
#define DIG_ROLL_OFF_SCALING            100
#define SCALED_DIG_ROLL_OFF             135 /* Actual 1.35 */
/*#define STTUNER_RFLEVEL_MONITOR*/
/* Maximum signal quality */
#define TEST_MAX_BER 200000

#define NUMBER_LOST_BEFORE_UNLOCK       3


/* Delay required for the scan task */
#ifndef STTUNER_SCAN_TASK_DELAY_MS
 #define STTUNER_SCAN_TASK_DELAY_MS 100  /* millisecond delay (0.1 sec) */
#endif


/* local functions  -------------------------------------------------------- */


void TERTASK_ScanTask(STTUNER_Handle_t *Handle);

ST_ErrorCode_t TERTASK_ProcessScanExact (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t TERTASK_ProcessTracking  (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t TERTASK_ProcessThresholds(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t TERTASK_ProcessNextScan  (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);

ST_ErrorCode_t TERTASK_GetTunerInfo (STTUNER_Handle_t Handle);


/*----------------------------------------------------------------------------
Name: TERTASK_Open()

Description:
    After open.c initialized each tuner component and then stored each
    device's capabilities this fn starts up the scan task for an instance.

Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_Open(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int result;

    Inst = STTUNER_GetDrvInst();

    strcpy(Inst[Handle].Terr.ScanTask.TaskName, Inst[Handle].Name);


#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s this TaskName [%s]\n", identity, Inst[Handle].Terr.ScanTask.TaskName));
#endif

    /* Create the scan busy semaphore required to synchronize access to the scan status, etc. */

    Inst[Handle].Terr.ScanTask.GuardSemaphore = STOS_SemaphoreCreateFifo(NULL,1);
 


    /* Create the timeout semaphore, this allows us to periodically wakeup the scan task to
      perform background processing e.g. tracking the locked frequency. */

  Inst[Handle].Terr.ScanTask.TimeoutSemaphore = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
  
  
    /* Reset the delete flag to ensure that the task doesn't exit prematurely */
    Inst[Handle].Terr.ScanTask.DeleteTask = FALSE;

    /* Sets the scan task timer delay, this controls how often the scan task awakes to perform background processing */
    Inst[Handle].Terr.ScanTask.TimeoutDelayMs = STTUNER_SCAN_TASK_DELAY_MS;

    /* Initialize the scan task status to startup defaults */
    Inst[Handle].Terr.ScanInfo.Scan           = NULL;
    Inst[Handle].Terr.ScanInfo.ScanIndex      = 0;
    Inst[Handle].Terr.ScanInfo.NextFrequency  = 0;
    Inst[Handle].Terr.ScanInfo.LockCount      = 0;    /* For monitoring tracking */
    Inst[Handle].Terr.ScanTask.TopLevelHandle = Handle;
    Inst[Handle].Terr.EnableReLocking = FALSE;

    /* Assume that the scan status is "unlocked" to begin with */
    Inst[Handle].TunerInfo.Status = STTUNER_STATUS_IDLE;
    Inst[Handle].PreStatus = STTUNER_STATUS_IDLE;
    Inst[Handle].PowerMode = STTUNER_NORMAL_POWER_MODE;
    Inst[Handle].Terr.EnableReLocking = FALSE;

    /* Create and start the scan task */

 result = STOS_TaskCreate( (void(*)(void *))TERTASK_ScanTask,            /* task function      */
                         &Inst[Handle].Terr.ScanTask.TopLevelHandle,   /* user params        */
                         Inst[Handle].DriverPartition,				/* stack partition    */
                         SCAN_TASK_STACK_SIZE,                         /* stack size         */
                         (void*)&Inst[Handle].Terr.ScanTask.ScanTaskStack,    /* stack              */
                         Inst[Handle].DriverPartition,					/* task partiton       */
                         &Inst[Handle].Terr.ScanTask.Task,              /* task control block */
                         &Inst[Handle].Terr.ScanTask.TaskDescriptor,    /* task descriptor    */
                         STTUNER_SCAN_TER_TASK_PRIORITY,               /* high priority, prevents stevt notify blocking */ /*removing hardcoded value for scantask priority for GNBvd25440 --> Allow override of task priorities*/
                         Inst[Handle].Terr.ScanTask.TaskName,          /* task name          */
                         0                                             /* flags              */
                      );

    /* Task creation failed? */
    if (result!= 0)
    {
        Error = ST_ERROR_NO_MEMORY;
        #ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s fail\n", identity));
        #endif
    }

    else
    {
        #ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s task started ok\n", identity));
        #endif
    }




    return(Error);

} /* STASK_Open() */



/*----------------------------------------------------------------------------
Name: TERTASK_Close()
Description:

Parameters:

Return Value:

See Also:
    STASK_Open()
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_Close(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    
    task_t *task;
  

    #ifndef STTUNER_MINIDRIVER
    Error = TERTASK_ScanAbort(Handle);
    #endif
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s fail TERTASK_ScanAbort()\n", identity));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s scan aborted ok\n", identity));
#endif
    }

    Inst = STTUNER_GetDrvInst();

    task = Inst[Handle].Terr.ScanTask.Task;



    /* wait until scantask is out of its critical section then grab semaphore to keep it waiting */
    STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);



    /* Wakeup the scan task (fallthrough early) */
    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.TimeoutSemaphore);


    /* modify the delete task flag as waiting to be deleted */
    Inst[Handle].Terr.ScanTask.DeleteTask = TRUE;

    /* allow scantask to procede */
    

    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s instance %d waiting for task [%s] to end...", identity, Handle, Inst[Handle].Terr.ScanTask.TaskName));
#endif
 
    /* The task should be in the process of returning, but wait for it first before deleting it. */
    if (STOS_TaskWait(&task,TIMEOUT_INFINITY) == 0)
    {

        /* Now it should be safe to delete the task */
       if (STOS_TaskDelete(Inst[Handle].Terr.ScanTask.Task,Inst[Handle].DriverPartition,Inst[Handle].Terr.ScanTask.ScanTaskStack,Inst[Handle].DriverPartition) == 0)

        {
            /* Delete all semaphores associated with the this scantask */
             
            STOS_SemaphoreDelete(NULL,Inst[Handle].Terr.ScanTask.GuardSemaphore);
            STOS_SemaphoreDelete(NULL,Inst[Handle].Terr.ScanTask.TimeoutSemaphore);
  
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
            STTBX_Print(("ok\n"));
#endif
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
            STTBX_Print(("fail task_delete()\n"));
#endif
            Error = ST_ERROR_DEVICE_BUSY;   /* task may not be deleted */
        }
    }


    return(Error);

} /* STASK_Close() */


#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: TERTASK_ScanAbort()


Description:
    This routine will abort the current scan step.

    NOTE:   This call will block until it is able to safely abort the
            current scan operation -- a callback may be invoked if the
            caller has set one up.
Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_ScanAbort(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ScanAbort()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
#endif

    /* Lock out the scan task whilst we modify the status code */

    STOS_SemaphoreWait(Inst[Handle].Terr.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("ok\n"));
#endif

    /* Forcibly unlock the tuner  */
    Inst[Handle].TunerInfo.Status    = STTUNER_STATUS_UNLOCKED;
    Inst[Handle].Terr.EnableReLocking = FALSE;

    /* Release semaphore -- allows scan task to resume */

    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);


    return(Error);

} /* TUNER_AbortScan() */
#endif


/*----------------------------------------------------------------------------
Name: TERTASK_ScanStart()

Description:
    This routine will commence a new scan based on the current scan list.
    The scan list is first examined to ensure it is valid, if not an error
    is generated.  If the scan list is ok, the scan task is awoken and the
    first scan step will be started.

    NOTE:   If a scan is already in progress, the current scan will be
            aborted and re-started at the next available opportunity.

Parameters:

Return Value:
    ST_NO_ERROR,                    the operation completed without error.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_ScanStart(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ScanStart()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    
#ifndef STTUNER_MINIDRIVER
    Inst = STTUNER_GetDrvInst();
    /********************Check given for scanlist and bandlist***********************/
    /* if the band or scan list is not setup then the scan is not valid so exit */
    if (Inst[Handle].BandList.NumElements < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s fail STTUNER_SetBandList() not done?\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }

    if (!Inst[Handle].Terr.ScanExact)
    {
       if (Inst[Handle].ScanList.NumElements < 1)
       {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
           STTBX_Print(("%s fail STTUNER_SetScanList() not done?\n", identity));
#endif
           return(ST_ERROR_BAD_PARAMETER);
       }
    }

   /**********************************************************************************/
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
#endif

    /* Wakeup the scan task and lock out the scan task whilst we modify the status code. */

    STOS_SemaphoreWait(  Inst[Handle].Terr.ScanTask.GuardSemaphore);
    

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("ok\n"));
#endif

    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.TimeoutSemaphore);
    
     if(Inst[Handle].PowerMode == STTUNER_STANDBY_POWER_MODE) 
        {
        	
         Inst[Handle].TunerInfo.Status = STTUNER_STATUS_STANDBY;     
        }
        else
        {
        	
        /* Set the scan status and reset the scan element pointer */	
          Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING;
          Inst[Handle].Terr.EnableReLocking = FALSE;   /* only relock on first single scan with a valid frequency */
        }
    
     
    /* Setup scan parameters is we're using the scan list */
    if (!Inst[Handle].Terr.ScanExact)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s using scan list\n", identity));
#endif
        Inst[Handle].Terr.ScanInfo.Scan          = Inst[Handle].ScanList.ScanList;
        Inst[Handle].Terr.ScanInfo.ScanIndex     = 0;
        Inst[Handle].Terr.ScanInfo.NextFrequency = Inst[Handle].Terr.ScanInfo.FrequencyStart;

    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s using single scan\n", identity));
#endif
        /* We only have one scan parameter for an exact scan */
        Inst[Handle].Terr.ScanInfo.Scan          = &Inst[Handle].Terr.SingleScan;

        Inst[Handle].Terr.FECRates               = Inst[Handle].Terr.SingleScan.FECRates;
        Inst[Handle].Terr.ScanInfo.NextFrequency = Inst[Handle].Terr.ScanInfo.FrequencyStart;
    }

    /* Release semaphore -- allows scan task to resume */

    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);


#endif
#ifdef STTUNER_MINIDRIVER
    Inst = STTUNER_GetDrvInst();

   
#if defined(ST_OS21) || defined(ST_OSLINUX)
    /* Wakeup the scan task and lock out the scan task whilst we modify the status code. */
    semaphore_wait(  Inst[Handle].Terr.ScanTask.GuardSemaphore);
#else
    semaphore_wait(  &Inst[Handle].Terr.ScanTask.GuardSemaphore);
#endif    
#if defined(ST_OS21) || defined(ST_OSLINUX)
    semaphore_signal(Inst[Handle].Terr.ScanTask.TimeoutSemaphore);
#else
    semaphore_signal(&Inst[Handle].Terr.ScanTask.TimeoutSemaphore);
#endif    

    /* Set the scan status and reset the scan element pointer */
    Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING;
    Inst[Handle].Terr.EnableReLocking = FALSE;   /* only relock on first single scan with a valid frequency */
    
   /* Setup scan parameters is we're using the scan list */
    if (!Inst[Handle].Terr.ScanExact)
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s using scan list\n", identity));
#endif
        Inst[Handle].Terr.ScanInfo.Scan          = Inst[Handle].ScanList.ScanList;
        Inst[Handle].Terr.ScanInfo.ScanIndex     = 0;
        Inst[Handle].Terr.ScanInfo.NextFrequency = Inst[Handle].Terr.ScanInfo.FrequencyStart;

    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s using single scan\n", identity));
#endif
        /* We only have one scan parameter for an exact scan */
        Inst[Handle].Terr.ScanInfo.Scan          = &Inst[Handle].Terr.SingleScan;

        Inst[Handle].Terr.FECRates               = Inst[Handle].Terr.SingleScan.FECRates;
        Inst[Handle].Terr.ScanInfo.NextFrequency = Inst[Handle].Terr.ScanInfo.FrequencyStart;
    }
#ifdef STTUNER_DEBUG_MODULE_TER_TERTASK
        STTBX_Print(("%s using single scan\n", identity));
#endif
 
    /* Release semaphore -- allows scan task to resume */
#if defined(ST_OS21) || defined(ST_OSLINUX) 
    semaphore_signal(Inst[Handle].Terr.ScanTask.GuardSemaphore);
#else
    semaphore_signal(&Inst[Handle].Terr.ScanTask.GuardSemaphore);
#endif    

#endif
    return(Error);

} /* TERTASK_ScanStart() */
#ifndef STTUNER_MINIDRIVER


/*----------------------------------------------------------------------------
Name: TERTASK_ScanContinue()
Description:
    This routine will proceed to the next scan step in the scan list.
    The scan task must be in the "not found" or "locked" state in order for
    this call to succeed, otherwise an error will be returned.

    NOTE:   If the end of the scan list has been encountered, then a call
            to TUNER_StartScan() must be done.
Parameters:

Return Value:
See Also:
    TUNER_StartScan()
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_ScanContinue(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ScanContinue()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    
    Inst = STTUNER_GetDrvInst();

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
#endif

    /* Wakeup the scan task and lock out the scan task whilst we modify the status code */

    STOS_SemaphoreWait(  Inst[Handle].Terr.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("ok\n"));
#endif


    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.TimeoutSemaphore);


    /* A scan may only be continued if the current scan status is
      "locked" or "unlocked". i.e., "not found" implies the end
      of the frequency range has been encountered or a scan has not
      yet started.  "scanning" implies the current scan step has
      not yet completed, therefore it doesn't make sense to
      continue yet */
    if ( (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_LOCKED)  ||
         (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_UNLOCKED) )
    {
    	/* Continue at the next frequency step */
        Inst[Handle].Terr.ScanInfo.ScanIndex = Inst[Handle].ScanList.NumElements;
        Inst[Handle].TunerInfo.Status   = STTUNER_STATUS_SCANNING;

        /* only relock on first single scan with a valid frequency */
        Inst[Handle].Terr.EnableReLocking = FALSE;
    }
    else
    {
        /* Unable to continue the scan -- the scan task is in the wrong
           state or the we've reached the end of the scan list */
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s fail incorrect scan state or end of scan list\n", identity));
#endif
        Error = ST_ERROR_BAD_PARAMETER;
    }

    /* Release semaphore -- allows scan task to resume */

    STOS_SemaphoreSignal(Inst[Handle].Terr.ScanTask.GuardSemaphore);


    return(Error);

} /* TERTASK_ScanContinue() */
#endif


/*----------------------------------------------------------------------------
Name: ScanTask()
Description:
    This routine should be run in its own thread of execution e.g., OS task.
    It is responsible for managing the process of a tuner scan based on
    the current working scan list.
    The scan task is a state machine that may be in any one of the following
    states:
    a) "scanning" -- tuner is currently scanning for a signal
    b) "locked" -- tuner is locked to a valid signal
    c) "unlocked" -- tuner is not locked
    d) "not found" -- the last scan failed
Parameters:
    Tuner_p,    a pointer to a tuner control block structure.
Return Value:
    Nothing.
See Also:
    AbortScan()
    StartScan()
    ContinueScan()
----------------------------------------------------------------------------*/
/*#define DEBUG_SCANTASK*/
void TERTASK_ScanTask(STTUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ScanTask()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_EventType_t Event;
    STTUNER_EvtParams_t EventInfo;
    STTUNER_InstanceDbase_t *Inst;
#ifdef ST_OS21
    osclock_t clk;
#else
    clock_t clk;
#endif
    STTUNER_Handle_t index;
    U32 a,retries = 1;

    Inst = STTUNER_GetDrvInst();
    index = *Handle;

#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s Task for instance %d up.\n", identity, index));
#endif
Inst[index].CurrentPreStatus = STTUNER_STATUS_IDLE;
Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_IDLE;
Inst[index].CurrentPowerMode = STTUNER_NORMAL_POWER_MODE;
    while(1)
    {
#if defined(ST_OS21) || defined(ST_OSLINUX)	
        /* Calculate wake-up time for next scan process */
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Terr.ScanTask.TimeoutDelayMs)/ 1000));

        /* Delay until either we're awoken or the wake-up time comes */

       
#else
        #ifdef PROCESSOR_C1 /*for GNBvd48688-->Time calculation will be overflow*/
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() / 1000) * Inst[index].Terr.ScanTask.TimeoutDelayMs));
        #else
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Terr.ScanTask.TimeoutDelayMs)/ 1000));
        #endif
         
#endif
 
      
       STOS_SemaphoreWaitTimeOut(Inst[index].Terr.ScanTask.TimeoutSemaphore, &clk);
        /* We are about to enter a critical section of code -- we must not
         * allow external interruptions whilst checking the scan status, etc*/
        STOS_SemaphoreWait(Inst[index].Terr.ScanTask.GuardSemaphore);
        
 
        
        
        /* If the driver is being terminated then this flag will be set
         * to TRUE -- we should cleanly exit the task. */
        if (Inst[index].Terr.ScanTask.DeleteTask == TRUE)
        {
          	
            STOS_SemaphoreSignal(Inst[index].Terr.ScanTask.GuardSemaphore);
      
            break;
        }
/**/
        /* Process the next step based on the current scan status */
        switch (Inst[index].TunerInfo.Status)
        {
            /* "scanning" - we must process a new scan step */
            case STTUNER_STATUS_SCANNING:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_SCANNING\n"));
#endif
                 
               Inst[index].CurrentPowerMode = STTUNER_NORMAL_POWER_MODE;
               
                if (Inst[index].Terr.ScanExact)
                {
                    for (a = 1; a <= retries; a++)
                    {    
                        Error = TERTASK_ProcessScanExact(index, &Event);
                        if (Event == STTUNER_EV_LOCKED) break;
                    }

                    /* for(a) breaks to here */
                    if (Inst[index].Terr.EnableReLocking == FALSE) /* if first pass */
                    {
                        if (Event != STTUNER_EV_LOCKED)  /* failed to lock */
                        {
                            /* Since we are scanning for a single frequency, unless we get
                            the locked event we should treat this as scan failed */
                            Event = STTUNER_EV_SCAN_FAILED;
                        }
                        else /* locked ok */
                        {
                          Inst[index].Terr.EnableReLocking = TRUE; /* also, at this point: Event == STTUNER_EV_LOCKED */
                        }
                    }

                }   /* if (ScanExact) */
                else
                {   
                    Inst[index].Terr.EnableReLocking = FALSE;     /* not valid when scanning a list */
                    Error = TERTASK_ProcessNextScan(index, &Event);
                }
                break;


            /* In the "locked" case we have a frequency that we must attempt to maintain, or track */
            case STTUNER_STATUS_LOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_LOCKED\n"));
#endif
              
               Error = TERTASK_ProcessTracking(index, &Event);

                /* Only process thresholds if the lock wasn't lost */
                if (Event == STTUNER_EV_NO_OPERATION)
                {
                    Error = TERTASK_ProcessThresholds(index, &Event);
                }
                else if (Inst[index].Terr.EnableReLocking == TRUE) /* if lost lock then try again */
                {
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_SCANNING;
                   
                }
                break;
                
               case STTUNER_STATUS_STANDBY:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_STANDBY\n"));
#endif
                 
                 Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_STANDBY;
                 
                 Inst[index].CurrentPowerMode = STTUNER_STANDBY_POWER_MODE;
                
                 Event = STTUNER_EV_NO_OPERATION;
                 
               
                break;
               
               
                 /* "not found" or "unlocked" cases require no further action.
                The scan task will have to wait for user intervention */
            case STTUNER_STATUS_UNLOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_UNLOCKED\n"));
#endif
                
                /* Check the last status to ensure that the state hasn't
                been changed externally e.g., scan aborted */
                if (Inst[index].Terr.EnableReLocking == TRUE)
                {
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_SCANNING; /* if lost lock then try again */
                   
                    break;
                }
                else if (Inst[index].CurrentTunerInfo.Status == STTUNER_STATUS_SCANNING)
                {
                    Event = STTUNER_EV_SCAN_FAILED;
                    break;
                }
                else if (Inst[index].CurrentTunerInfo.Status == STTUNER_STATUS_LOCKED)
                {
                    Event = STTUNER_EV_UNLOCKED; /* Aborted scan */
                    break;
                }
                /* Fall through */
                
            case STTUNER_STATUS_IDLE:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_IDLE\n"));
#endif           
                Inst[index].CurrentPowerMode = STTUNER_NORMAL_POWER_MODE;
        
              
                /* Fall through */

            case STTUNER_STATUS_NOT_FOUND:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_NOT_FOUND\n"));
#endif
                
                /* Fall through */

            default:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_EV_NO_OPERATION\n"));
#endif
                Event = STTUNER_EV_NO_OPERATION;
                break;
        }

         

        /* If an error has occurred, then we must end the scan now,we can't recover from this condition */
        if (Error != ST_NO_ERROR)
        {
            /* In the event of a bus error we should tidy up the current task appropriately */
            if (     Inst[index].TunerInfo.Status == STTUNER_STATUS_SCANNING) Event = STTUNER_EV_SCAN_FAILED;
            else if (Inst[index].TunerInfo.Status == STTUNER_STATUS_LOCKED)   Event = STTUNER_EV_UNLOCKED;
        }


        /* Check the outcome of the last operation */
        switch (Event)
        {
            case STTUNER_EV_LOCKED:                     /* We are now locked */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_LOCKED;
                Inst[index].CurrentPreStatus = STTUNER_STATUS_LOCKED;
                Inst[index].Terr.ScanInfo.LockCount  = NUMBER_LOST_BEFORE_UNLOCK;

                /* Reset signal indicators */
                Inst[index].Terr.LastThreshold = ((U32)-1);     /* Undefined threshold */
                Inst[index].Terr.LastSignal    = ((U32)-1);     /* Undefined signal */
                break;

            case STTUNER_EV_UNLOCKED:                   /* We have lost lock */
                if (Inst[index].Terr.EnableReLocking != TRUE)
                {
                	
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_UNLOCKED;
                    Inst[index].CurrentPreStatus = STTUNER_STATUS_UNLOCKED;
                }
                break;

            case STTUNER_EV_SCAN_FAILED:                /* The last scan failed */
                /* The end of the scan list has been reached, we drop to the
                 "not found" status.  A callback may be invoked soon */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                Inst[index].CurrentPreStatus = STTUNER_STATUS_NOT_FOUND;
                break;

            case STTUNER_EV_TIMEOUT:
                /* If a timeout occurs, we do not allow the scan to continue.
                 this is effectively the same as reaching the end of the
                 scan list -- we reset our status to "not found" */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                Inst[index].CurrentPreStatus = STTUNER_STATUS_NOT_FOUND;
                break;

            case STTUNER_EV_SIGNAL_CHANGE:
                /* No action required.  The event will be processed in the
                 * call back below...
                 */
                break;

            default:
                /* We must now proceed to the next scan step, if there is one,
                 if there isn't a next scan step then a callback might be invoked */
                if ( (Inst[index].TunerInfo.Status       == STTUNER_STATUS_SCANNING         ) &&
                     (Inst[index].Terr.ScanInfo.ScanIndex <  Inst[index].ScanList.NumElements) &&
                     (Inst[index].Terr.EnableReLocking    != TRUE                            ) )
                {
                    /* Proceed to the next element.  In order to avoid a spurious callback being invoked,
                    we reset the event to a "no op" */
                    Inst[index].Terr.ScanInfo.Scan++;
                    Inst[index].Terr.ScanInfo.ScanIndex++;

                    /* Avoid task delay */
           
                    STOS_SemaphoreSignal(Inst[index].Terr.ScanTask.TimeoutSemaphore);
                  
                }
                
                 
                Inst[index].CurrentTunerInfo.Status = Inst[index].TunerInfo.Status;
                Inst[index].CurrentPreStatus = Inst[index].PreStatus;
                Inst[index].CurrentPowerMode = Inst[index].PowerMode;
                break;

        }   /* switch(Event) */


        /* Commit new tuner information -- the API level can now see it */
        Inst[index].TunerInfo = Inst[index].CurrentTunerInfo;
        Inst[index].PreStatus = Inst[index].CurrentPreStatus;
        Inst[index].PowerMode = Inst[index].CurrentPowerMode;
      

        /* Critical section end */
        
        STOS_SemaphoreSignal(Inst[index].Terr.ScanTask.GuardSemaphore);
  

        /* We may have to generate an event if a notify function is setup */
        if ( (Inst[index].NotifyFunction != NULL) && (Event != STTUNER_EV_NO_OPERATION) )
        {
             /*STTBX_Print(("STEVT_Notify, index = %d, Event = %d, Error = %d\n", index, Event, Error));*/
             (Inst[index].NotifyFunction)(INDEX2HANDLE(index), Event, Error);
        }
        else if (Event != STTUNER_EV_NO_OPERATION)
        {

            /* Use the event handler to notify */
            EventInfo.Handle = INDEX2HANDLE(index);
            EventInfo.Error  =  Error;
            /*STTBX_Print(("STEVT_Notify, Handle = %d, Error = %d\n", Handle, Error));*/
            Error = STEVT_Notify(Inst[index].EVTHandle, Inst[index].EvtId[EventToId(Event)], &EventInfo);

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
            if (Error != ST_NO_ERROR) STTBX_Print(("%s STEVT error = %u\n", identity, Error));
#endif
        }

    }   /* while(1)*/
#endif
#ifdef STTUNER_MINIDRIVER
#ifdef STTUNER_DEBUG_MODULE_TER_TERTASK
        STTBX_Print(("%s Task for instance %d up.\n", identity, index));
#endif
    while(1)
    {
#if defined(ST_OS21) || defined(ST_OSLINUX)
        /* Calculate wake-up time for next scan process */
        clk = time_plus(time_now(), ((ST_GetClocksPerSecond() * Inst[index].Terr.ScanTask.TimeoutDelayMs)/ 1000));

        /* Delay until either we're awoken or the wake-up time comes */
        semaphore_wait_timeout(Inst[index].Terr.ScanTask.TimeoutSemaphore, &clk);
        /* We are about to enter a critical section of code -- we must not
         * allow external interruptions whilst checking the scan status, etc*/
        semaphore_wait(Inst[index].Terr.ScanTask.GuardSemaphore);
#else
    
        /* Calculate wake-up time for next scan process */
        #ifdef PROCESSOR_C1 /*for GNBvd48688-->Time calculation will be overflow*/
        clk = time_plus(time_now(), ((ST_GetClocksPerSecond() / 1000) * Inst[index].Terr.ScanTask.TimeoutDelayMs));
        #else
        clk = time_plus(time_now(), ((ST_GetClocksPerSecond() * Inst[index].Terr.ScanTask.TimeoutDelayMs)/ 1000));
        #endif

        /* Delay until either we're awoken or the wake-up time comes */
        semaphore_wait_timeout(&Inst[index].Terr.ScanTask.TimeoutSemaphore, &clk);
        /* We are about to enter a critical section of code -- we must not
         * allow external interruptions whilst checking the scan status, etc*/
        semaphore_wait(&Inst[index].Terr.ScanTask.GuardSemaphore);
#endif
        /* If the driver is being terminated then this flag will be set
         * to TRUE -- we should cleanly exit the task. */
        if (Inst[index].Terr.ScanTask.DeleteTask == TRUE)
        {
#ifdef ST_OS21         	
            semaphore_signal(Inst[index].Terr.ScanTask.GuardSemaphore);
#else
            semaphore_signal(&Inst[index].Terr.ScanTask.GuardSemaphore);
#endif            
            break;
        }

        /* Process the next step based on the current scan status */
        switch (Inst[index].TunerInfo.Status)
        {
            /* "scanning" - we must process a new scan step */
            case STTUNER_STATUS_SCANNING:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_SCANNING\n"));
#endif
                
                if (Inst[index].Terr.ScanExact)
                {
                                        
                    for (a = 1; a <= retries; a++)
                    {
                    	Error = TERTASK_ProcessScanExact(index, &Event);
                        if (Event == STTUNER_EV_LOCKED) break;
                    }

                    /* for(a) breaks to here */
                    if (Inst[index].Terr.EnableReLocking == FALSE) /* if first pass */
                    {
                        if (Event != STTUNER_EV_LOCKED)  /* failed to lock */
                        {
                           
                            Event = STTUNER_EV_SCAN_FAILED;
                           
                        }
                        else /* locked ok */
                        {
                           Inst[index].Terr.EnableReLocking = TRUE; /* also, at this point: Event == STTUNER_EV_LOCKED */
                        }
                    }

                }   /* if (ScanExact) */
                               
                break;


            /* In the "locked" case we have a frequency that we must attempt to maintain, or track */
            case STTUNER_STATUS_LOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_LOCKED\n"));
#endif


                Error = TERTASK_ProcessTracking(index, &Event);

                /* Only process thresholds if the lock wasn't lost */
                if (Event == STTUNER_EV_NO_OPERATION)
                {

                    /*Error = TERTASK_ProcessThresholds(index, &Event);*/

                }
                else if (Inst[index].Terr.EnableReLocking == TRUE) /* if lost lock then try again */
                {

                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_SCANNING;

                }
                
                break;


            /* "not found" or "unlocked" cases require no further action.
                The scan task will have to wait for user intervention */
            case STTUNER_STATUS_UNLOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_UNLOCKED\n"));
#endif
                /* Check the last status to ensure that the state hasn't 
                been changed externally e.g., scan aborted */
                if (Inst[index].Terr.EnableReLocking == TRUE)
                {
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_SCANNING; /* if lost lock then try again */
                    break;
                }
                else if (Inst[index].CurrentTunerInfo.Status == STTUNER_STATUS_SCANNING)
                {
                    Event = STTUNER_EV_SCAN_FAILED;
                    break;
                }
                else if (Inst[index].CurrentTunerInfo.Status == STTUNER_STATUS_LOCKED)
                {
                    Event = STTUNER_EV_UNLOCKED; /* Aborted scan */
                    break;
                }
                /* Fall through */

            case STTUNER_STATUS_NOT_FOUND:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_NOT_FOUND\n"));
#endif
                /* Fall through */

            default:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_EV_NO_OPERATION\n"));
#endif
                Event = STTUNER_EV_NO_OPERATION;
            break;
        }


        /* If an error has occurred, then we must end the scan now,we can't recover from this condition */
        if (Error != ST_NO_ERROR)
        {
            /* In the event of a bus error we should tidy up the current task appropriately */
            if (     Inst[index].TunerInfo.Status == STTUNER_STATUS_SCANNING) Event = STTUNER_EV_SCAN_FAILED;
            else if (Inst[index].TunerInfo.Status == STTUNER_STATUS_LOCKED)   Event = STTUNER_EV_UNLOCKED;
            
        }


        /* Check the outcome of the last operation */
        switch (Event)
        {
            case STTUNER_EV_LOCKED:                     /* We are now locked */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_LOCKED;
                Inst[index].Terr.ScanInfo.LockCount  = NUMBER_LOST_BEFORE_UNLOCK;

                /* Reset signal indicators */
                Inst[index].Terr.LastThreshold = ((U32)-1);     /* Undefined threshold */
                Inst[index].Terr.LastSignal    = ((U32)-1);     /* Undefined signal */
                break;

            case STTUNER_EV_UNLOCKED:                   /* We have lost lock */
                if (Inst[index].Terr.EnableReLocking != TRUE)
                {
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_UNLOCKED;
                }
                break;

            case STTUNER_EV_SCAN_FAILED:                /* The last scan failed */
                /* The end of the scan list has been reached, we drop to the
                 "not found" status.  A callback may be invoked soon */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                break;

            case STTUNER_EV_TIMEOUT:
                /* If a timeout occurs, we do not allow the scan to continue.
                 this is effectively the same as reaching the end of the
                 scan list -- we reset our status to "not found" */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                break;

            case STTUNER_EV_SIGNAL_CHANGE:
                /* No action required.  The event will be processed in the
                 * call back below...
                 */
                break;
            default:
                /* We must now proceed to the next scan step, if there is one,
                 if there isn't a next scan step then a callback might be invoked */
                #ifdef ST_OS21                    
                    semaphore_signal(Inst[index].Terr.ScanTask.TimeoutSemaphore);
#else
                    semaphore_signal(&Inst[index].Terr.ScanTask.TimeoutSemaphore);
#endif 
                Inst[index].CurrentTunerInfo.Status = Inst[index].TunerInfo.Status;
                break;

        }   /* switch(Event) */


        /* Commit new tuner information -- the API level can now see it */
        Inst[index].TunerInfo = Inst[index].CurrentTunerInfo;

        /* Critical section end */
        
        /* Check whether timeout task has return its event */
              
#ifdef ST_OS21         
        semaphore_signal(Inst[index].Terr.ScanTask.GuardSemaphore);
#else
	semaphore_signal(&Inst[index].Terr.ScanTask.GuardSemaphore);
#endif        

        /* We may have to generate an event if a notify function is setup */
        if ( (Inst[index].NotifyFunction != NULL) && (Event != STTUNER_EV_NO_OPERATION) )
        {
            (Inst[index].NotifyFunction)(INDEX2HANDLE(index), Event, Error);
        }
        else if (Event != STTUNER_EV_NO_OPERATION)
        {
            /* Use the event handler to notify */
            EventInfo.Handle = INDEX2HANDLE(index);
            EventInfo.Error  =  Error;
            Error = STEVT_Notify(Inst[index].EVTHandle, Inst[index].EvtId[EventToId(Event)], &EventInfo);

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
            if (Error != ST_NO_ERROR) STTBX_Print(("%s STEVT error = %u\n", identity, Error));
#endif
        }

    }   /* while(1)*/    
#endif

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s task ending\n", identity));
#endif

    return;

} /* TERTASK_ScanTask() */



/*----------------------------------------------------------------------------
Name: TERTASK_ProcessScanExact()
Description:
    This routine scans for a single frequency based on the current
    tuner scan parameters.
Parameters:
    Event,  pointer to location to store returned event,  may be:

    STTUNER_EV_NO_OPERATION,    we failed to achieve tuner lock.
    STTUNER_EV_LOCKED,          the tuner is locked on a frequency.

Return Value:
See Also:
    Nothing.
----------------------------------------------------------------------------*/
/* #define TERTASK_SYMOK */
ST_ErrorCode_t TERTASK_ProcessScanExact(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ProcessScanExact()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    BOOL ScanSuccess = FALSE;
    U32  ScanFrequency;
    U32  NewFrequency;
    U32  index;
    STTUNER_Band_t   *Band;
#ifdef STTUNER_USE_ATSC_VSB    
    U8 IQ_inverted =0;
#endif    
   

#ifndef STTUNER_MINIDRIVER
    Inst = STTUNER_GetDrvInst();

    *Event = STTUNER_EV_NO_OPERATION;   /* Assume no lock is found */
    
    




    /* Setup the current element -- this represents a working copy we can
     modify of the current element in the scan list -- on completion it
     will be made visible to the ST TUNER API.*/
    Inst[Handle].CurrentTunerInfo.ScanInfo = *Inst[Handle].Terr.ScanInfo.Scan;

    /*Adding following line for GNBvd28574(Useage of STTUNER_GetTunerInfo) ->for proper reporting of frequency being scanned at the time*/
    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Terr.ScanInfo.NextFrequency;
   
        /* Mask with the capabilties of the device */
        Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation &= Inst[Handle].Capability.Modulation;
        


        /* Proceed only if it is a valid band */
        if (Inst[Handle].CurrentTunerInfo.ScanInfo.Band < Inst[Handle].BandList.NumElements)
        {
            
            /* Set requested band */
            index = Inst[Handle].CurrentTunerInfo.ScanInfo.Band;
            Band  = &Inst[Handle].BandList.BandList[index];

            /* Ensure current frequency falls inside selected band */
            if ( (Inst[Handle].Terr.ScanInfo.NextFrequency >= Band->BandStart) &&
                 (Inst[Handle].Terr.ScanInfo.NextFrequency <= Band->BandEnd) )
            {
               
                /* Calc the downconverted frequency used by the auto scan routine */
                ScanFrequency = Inst[Handle].Terr.ScanInfo.NextFrequency;

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
                STTBX_Print(("DEMOD_ScanFrequency() with F=%u IF=%u.\n", Inst[Handle].Terr.ScanInfo.NextFrequency, ScanFrequency));
#endif


                /* Perform auto search algorithm */
	 if ((Inst[Handle].Terr.Demod.Driver->ID != STTUNER_DEMOD_STB0370VSB) && (Inst[Handle].Terr.Demod.Driver->ID != STTUNER_DEMOD_STV0372) )
	 {
	 	
 	
 	
 	#ifdef STTUNER_USE_TER
                Error = (Inst[Handle].Terr.Demod.Driver->demod_ScanFrequency)(
                                            Inst[Handle].Terr.Demod.DrvHandle,
                                            ScanFrequency,
                                            0,0,0,0,
                                            &ScanSuccess,  &NewFrequency,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Mode,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Guard,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Force,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Hierarchy,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Spectrum,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.FreqOff,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.ChannelBW,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.EchoPos);
         #endif
                 }
else if  ((Inst[Handle].Terr.Demod.Driver->ID == STTUNER_DEMOD_STB0370VSB )||(Inst[Handle].Terr.Demod.Driver->ID == STTUNER_DEMOD_STV0372) )

{
	#ifdef STTUNER_USE_ATSC_VSB
if (Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode == STTUNER_INVERSION_AUTO)
{

Error = (Inst[Handle].Terr.Demod.Driver->demod_ScanFrequency)(
                                            Inst[Handle].Terr.Demod.DrvHandle,
                                            ScanFrequency,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,0,0,0,
                                            &ScanSuccess,  &NewFrequency,
                                            0,
                                            0,
                                            0,
                                            0,
                                            STTUNER_IQ_MODE_NORMAL,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.FreqOff,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.ChannelBW,
                                            0);
                                            
                     if(ScanSuccess == 0)
                                 {
                                 	Error = (Inst[Handle].Terr.Demod.Driver->demod_ScanFrequency)(
                                            Inst[Handle].Terr.Demod.DrvHandle,
                                            ScanFrequency,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,0,0,0,
                                            &ScanSuccess,  &NewFrequency,
                                            0,
                                            0,
                                            0,
                                            0,
                                            STTUNER_IQ_MODE_INVERTED,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.FreqOff,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.ChannelBW,
                                            0);
                                            IQ_inverted =1;
                                            
                                 	
                                   }
                                   else
                                   {
                                   	  	Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode=STTUNER_IQ_MODE_NORMAL;
                                }
 }
 else
 {
 	Error = (Inst[Handle].Terr.Demod.Driver->demod_ScanFrequency)(
                                            Inst[Handle].Terr.Demod.DrvHandle,
                                            ScanFrequency,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,0,0,0,
                                            &ScanSuccess,  &NewFrequency,
                                            0,
                                            0,
                                            0,
                                            0,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.FreqOff,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.ChannelBW,
                                            0);
}
                                            
  #endif                  
}                                           
                
                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
                    STTBX_Print(("%s fail set demod\n", identity));
#endif
                    return(Error);
                }

                if ( ScanSuccess )
                {
                    Inst[Handle].Terr.ScanInfo.NextFrequency = NewFrequency;
                    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Terr.ScanInfo.NextFrequency;
                  
#ifdef STTUNER_USE_ATSC_VSB
	                    if ((Inst[Handle].Terr.Demod.Driver->ID == STTUNER_DEMOD_STB0370VSB) ||(Inst[Handle].Terr.Demod.Driver->ID == STTUNER_DEMOD_STV0372))
	                    {

                    if (IQ_inverted == 1)
                    {
               
                    Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode =STTUNER_IQ_MODE_INVERTED; 
                  
                    }
                }
 #endif  
                    /* Extract remaining tuner info from h/w */
                    Error = TERTASK_GetTunerInfo(Handle);
                    if (Error != ST_NO_ERROR)
                    {
                        return(Error);
                    }

                    /* Update status and lock count ready for tracking */
                    *Event = STTUNER_EV_LOCKED;
                }
            }
        }
        else
        {
            /* Invalid band specified */
            Error = ST_ERROR_BAD_PARAMETER;
        }


#ifdef TERTASK_SYMOK
    }
#endif
#endif

#ifdef STTUNER_MINIDRIVER
Inst = STTUNER_GetDrvInst();

    *Event = STTUNER_EV_NO_OPERATION;   /* Assume no lock is found */

    /* Setup the current element -- this represents a working copy we can
     modify of the current element in the scan list -- on completion it
     will be made visible to the ST TUNER API.*/
    Inst[Handle].CurrentTunerInfo.ScanInfo = *Inst[Handle].Terr.ScanInfo.Scan;

    /*Adding following line for GNBvd28574(Useage of STTUNER_GetTunerInfo) ->for proper reporting of frequency being scanned at the time*/
    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Terr.ScanInfo.NextFrequency;
    
        /* Mask with the capabilties of the device */
        Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation &= Inst[Handle].Capability.Modulation;

        /* Proceed only if it is a valid band */
        if (Inst[Handle].CurrentTunerInfo.ScanInfo.Band < Inst[Handle].BandList.NumElements)
        {

            /* Set requested band */
            index = Inst[Handle].CurrentTunerInfo.ScanInfo.Band;
            Band  = &Inst[Handle].BandList.BandList[index];

            /* Ensure current frequency falls inside selected band */
            if ( (Inst[Handle].Terr.ScanInfo.NextFrequency >= Band->BandStart) &&
                 (Inst[Handle].Terr.ScanInfo.NextFrequency <= Band->BandEnd) )
            {

                /* Calc the downconverted frequency used by the auto scan routine */
                ScanFrequency = Inst[Handle].Terr.ScanInfo.NextFrequency;

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
                STTBX_Print(("DEMOD_ScanFrequency() with F=%u IF=%u.\n", Inst[Handle].Terr.ScanInfo.NextFrequency, ScanFrequency));
#endif


                /* Perform auto search algorithm */

 
                Error = demod_d0360_ScanFrequency(
                                            Inst[Handle].Terr.Demod.DrvHandle,
                                            ScanFrequency,
                                            0,0,0,0,
                                            &ScanSuccess,  &NewFrequency,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Mode,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Guard,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Force,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Hierarchy,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.Spectrum,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.FreqOff,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.ChannelBW,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.EchoPos);
         
           
                                     
                
                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
                    STTBX_Print(("%s fail set demod\n", identity));
#endif
                    return(Error);
                }

                if ( ScanSuccess )
                {
                    Inst[Handle].Terr.ScanInfo.NextFrequency = NewFrequency;
                    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Terr.ScanInfo.NextFrequency;

                    /* Extract remaining tuner info from h/w */
                    Error = TERTASK_GetTunerInfo(Handle);
                    if (Error != ST_NO_ERROR)
                    {
                        return(Error);
                    }

                    /* Update status and lock count ready for tracking */
                    *Event = STTUNER_EV_LOCKED;
                }
            }
        }
        else
        {
            /* Invalid band specified */
            Error = ST_ERROR_BAD_PARAMETER;
        }
#endif
    return(Error);
}



/*----------------------------------------------------------------------------
Name: TERTASK_ProcessTracking()
Description:
    This routine calls the DEMOD device in order to ensure we maintain
    a tuner lock for the currently tuned frequency.
Parameters:
    Event,    pointer to area to store event:

    STTUNER_EV_NO_OPERATION,    no event has occurred i.e. lock is ok.
    STTUNER_EV_UNLOCKED,        the lost lock count has been exceeded and
                                no further attempts at tracking should be
                                attempted.
Return Value:
See Also:
    Nothing.
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_ProcessTracking(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ProcessTracking()";
#endif
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STTUNER_EventType_t ThisEvent = STTUNER_EV_NO_OPERATION;
    STTUNER_InstanceDbase_t *Inst;
    BOOL IsLocked;    
    
    BOOL IsSignal;
    U32 IF;
    

    Inst = STTUNER_GetDrvInst();

    #ifndef STTUNER_MINIDRIVER
	   if((Inst[Handle].Terr.Demod.Driver->ID != STTUNER_DEMOD_STB0370VSB) && (Inst[Handle].Terr.Demod.Driver->ID != STTUNER_DEMOD_STV0372))
	   {
	   	
   	
    Error = (Inst[Handle].Terr.Demod.Driver->demod_Tracking)(Inst[Handle].Terr.Demod.DrvHandle, FALSE, &IF, &IsSignal);
    }

    if (Error == ST_NO_ERROR)
    {
        Error = (Inst[Handle].Terr.Demod.Driver->demod_IsLocked)(Inst[Handle].Terr.Demod.DrvHandle, &IsLocked);
    }

    if (Error == ST_NO_ERROR)
    {
        if (IsLocked)
        {
            Inst[Handle].Terr.ScanInfo.LockCount = NUMBER_LOST_BEFORE_UNLOCK;
        }
        else
        {
            Inst[Handle].Terr.ScanInfo.LockCount--;
        }

         if (Inst[Handle].Terr.ScanInfo.LockCount == 0)
        {
            ThisEvent = STTUNER_EV_UNLOCKED;
        }
        else
        {
            Error = TERTASK_GetTunerInfo(Handle);
        }
    }

    if (Error != ST_NO_ERROR)
    {
        ThisEvent = STTUNER_EV_UNLOCKED;
    }

    *Event = ThisEvent;

#endif
#ifdef STTUNER_MINIDRIVER

       Error=demod_d0360_Tracking(Inst[Handle].Terr.Demod.DrvHandle, FALSE, &IF,  &IsSignal);
  

    if (Error == ST_NO_ERROR)
    {
       Error=demod_d0360_IsLocked(Inst[Handle].Terr.Demod.DrvHandle, &IsLocked);
    }

    if (Error == ST_NO_ERROR)
    {
        if (IsLocked)
        {
            Inst[Handle].Terr.ScanInfo.LockCount = NUMBER_LOST_BEFORE_UNLOCK;
        }
        else
        {
            Inst[Handle].Terr.ScanInfo.LockCount--;
        }

         if (Inst[Handle].Terr.ScanInfo.LockCount == 0)
        {
            ThisEvent = STTUNER_EV_UNLOCKED;
        }
        else
        {
            Error = TERTASK_GetTunerInfo(Handle);
        }
    }

    if (Error != ST_NO_ERROR)
    {
        ThisEvent = STTUNER_EV_UNLOCKED;
    }

    *Event = ThisEvent;

#endif
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    STTBX_Print(("%s done ok %d\n", identity, Handle));
#endif
    return(Error);

} /* TERTASK_ProcessTracking() */



#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: TERTASK_ProcessThresholds()
Description:
Parameters:
    Tuner_p,    pointer to the tuner control block.
    Event_p,    pointer to area to store event:

    STTUNER_EV_NO_OPERATION,    no event has occurred i.e. lock is ok.
    STTUNER_EV_SIGNAL_CHANGE,   the signal quality has changed threshold.

Return Value:
    ST_NO_ERROR,        the operation completed without error.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_ProcessThresholds(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ProcessThresholds()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_SignalThreshold_t *Threshold;
    U32 CurrentSignal, LastSignal;
    U32 *ThresholdHits;
    U32 index, HitCount;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();


    /* Assume signal threshold is ok i.e. no event */
    *Event = STTUNER_EV_NO_OPERATION;
    ThresholdHits = Inst[Handle].ThresholdHits;

    /* Calculate current signal and last signal values */
    CurrentSignal = ( Inst[Handle].QualityFormat == STTUNER_QUALITY_BER ) ?
            Inst[Handle].CurrentTunerInfo.BitErrorRate : Inst[Handle].CurrentTunerInfo.SignalQuality;

    LastSignal = Inst[Handle].Terr.LastSignal;

    if ( (CurrentSignal == LastSignal) ||  (Inst[Handle].ThresholdList.NumElements == 0) )
    {
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s info: no change in signal %d\n", identity, Handle));
#endif
        return(Error); /* No need to proceed -- no change in signal */
    }

    /* Re-set last signal indicator */
    Inst[Handle].Terr.LastSignal = CurrentSignal;

    /* Set threshold pointer to start of threshold list */
    Threshold = Inst[Handle].ThresholdList.ThresholdList;

    index    = 0;   /* List element counter */
    HitCount = 0;   /* Number of hits */

    /* Calculate all theshold hits */
    while ( index < Inst[Handle].ThresholdList.NumElements )
    {
        if ( (CurrentSignal >= Threshold->SignalLow) &&
             (CurrentSignal <= Threshold->SignalHigh) )
        {
            ThresholdHits[HitCount++] = index; /* Hit found */
        }

        index++;
        Threshold++;
    }


    /* The current signal may either:
     a) Not exist in any threshold (HitCount == 0)
     b) Exist in a one or more thresholds (HitCount >= 1) */

    if (HitCount > 0)
    {
        /* Unless all thresholds are different to the last threshold,
         there will not be a signal change event.
         In the case where all thresholds are different from the
         last threshold, we arbitrarily choose index = 0 as the
         new threshold and signal an event */
        for (index = 0; index < HitCount && ThresholdHits[index] != Inst[Handle].Terr.LastThreshold; index++); /* Do nothing */

        /* Check for all thresholds not equal to last threshold */
        if (index == HitCount)
        {
            /* Threshold has changed, we should ensure an event is generated */
            *Event = STTUNER_EV_SIGNAL_CHANGE;
            Inst[Handle].Terr.LastThreshold = ThresholdHits[0];
        }

    }
    else
    {
        /* The hit count is zero -- no thresholds hold the current
         * signal value.  An event must be generated unless this was
         * the case last time we checked.
         */
        if (Inst[Handle].Terr.LastThreshold != ((U32)-1))
        {
            *Event = STTUNER_EV_SIGNAL_CHANGE;
            Inst[Handle].Terr.LastThreshold = ((U32)-1);
        }
    }

    return(Error);
}

#endif
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: TERTASK_ProcessNextScan()
Description:
    This routine encapsulates the tuning strategy.
Parameters:
    Event,    pointer to location to store returned event.  May be:

                STTUNER_EV_NO_OPERATION,    we failed to achieve tuner lock.
                STTUNER_EV_LOCKED,          the tuner is locked on a frequency.
Return Value:
See Also:
    Nothing.
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_ProcessNextScan(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_ProcessNextScan()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 FrequencyEnd, FrequencyStart;

    Inst = STTUNER_GetDrvInst();


    /* Check to see if we are at the end of the scan list */
    if (Inst[Handle].Terr.ScanInfo.ScanIndex >= Inst[Handle].ScanList.NumElements)
    {
        /* Calculate correct start and end frequencies */
        if (Inst[Handle].Terr.ScanInfo.ScanDirection < 0)
        {
            FrequencyEnd   = Inst[Handle].Terr.ScanInfo.FrequencyStart;
            FrequencyStart = Inst[Handle].Terr.ScanInfo.FrequencyEnd;
        }
        else
        {
            FrequencyEnd   = Inst[Handle].Terr.ScanInfo.FrequencyEnd;
            FrequencyStart = Inst[Handle].Terr.ScanInfo.FrequencyStart;
        }


        /* Apply next frequency step */
        Inst[Handle].Terr.ScanInfo.NextFrequency +=
            (Inst[Handle].Terr.ScanInfo.ScanDirection * (S32)Inst[Handle].Terr.ScanInfo.FrequencyStep);

        if ( (Inst[Handle].Terr.ScanInfo.NextFrequency > FrequencyEnd)   ||
             (Inst[Handle].Terr.ScanInfo.NextFrequency < FrequencyStart) )
        {
            /* We have reached the limit of our frequency range */
            /* No more combinations to try -- scan has failed */
            *Event = STTUNER_EV_SCAN_FAILED;
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
             STTBX_Print(("%s STTUNER_EV_SCAN_FAILED\n", identity));
#endif
            return(Error);
        }

        /* Set scan list to beginning */
        Inst[Handle].Terr.ScanInfo.Scan = Inst[Handle].ScanList.ScanList;
        Inst[Handle].Terr.ScanInfo.ScanIndex = 0;
    }

    /* Process the next frequency scan step */
    Error = TERTASK_ProcessScanExact(Handle, Event);

    return(Error);
}
#endif


/*----------------------------------------------------------------------------
Name: TERTASK_GetTunerInfo()
Description:
    Calls the DEMOD device in order to get the current demodulation settings
    for updating the current tuner info.  This information will eventually
    be forwarded back up to the STTUNER API tuner info.
Parameters:

Return Value:
See Also:
----------------------------------------------------------------------------*/
ST_ErrorCode_t TERTASK_GetTunerInfo(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
    const char *identity = "STTUNER tertask.c TERTASK_GetTunerInfo()";
#endif
    ST_ErrorCode_t Error;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();
    
    #ifndef STTUNER_MINIDRIVER 
    /* Get the Core gain value */
    Error  = (Inst[Handle].Terr.Demod.Driver->demod_GetAGC)(Inst[Handle].Terr.Demod.DrvHandle, 
			                                  &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);
#ifdef STTUNER_RFLEVEL_MONITOR
    /* Get the RFLevel value */
    if(Inst[Handle].Terr.Demod.Driver->ID == STTUNER_DEMOD_STV0360 )
    {
    	Error  = (Inst[Handle].Terr.Demod.Driver->demod_GetRFLevel)(Inst[Handle].Terr.Demod.DrvHandle, 
                                                          &Inst[Handle].CurrentTunerInfo.RFLevel);
    }

    /* Get the current demodulation parameters */
    if(Inst[Handle].Terr.Demod.Driver->ID == STTUNER_DEMOD_STV0362 )
    {
    Error  = (Inst[Handle].Terr.Demod.Driver->demod_GetRFLevel)(Inst[Handle].Terr.Demod.DrvHandle, 
                                                          &Inst[Handle].CurrentTunerInfo.RFLevel);
      }
#endif                                                   
                                                        
    Error  = (Inst[Handle].Terr.Demod.Driver->demod_GetTunerInfo)(Inst[Handle].Terr.Demod.DrvHandle,
                                                          &Inst[Handle].CurrentTunerInfo);
                                                          
                                                         
    if (Error != ST_NO_ERROR)
    {

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s fail get demod params, Error = %u\n", identity, Error));
#endif
        return(Error);
    }


    /* The following code will be included if you wish to introduce 'random noise' into the
    generated signal quality values. This is useful for testing for signal threshold events */
#ifdef STTUNER_SIGNAL_NOISE
    {
        static BOOL SRand = TRUE;
        U32 r;

        if (SRand)
        {
            srand((unsigned int)STOS_time_now());
            SRand = FALSE;
        }

        /* Calculate percentage fluctuation from current position */
        r = rand() % 100;               /* Percent change */

        Inst[Handle].CurrentTunerInfo.SignalQuality -= (
            (((Inst[Handle].CurrentTunerInfo.SignalQuality << 7) * r) / 100) >> 7 );

        /* Estimate ber from signal quality based on a linear relationship */
        Inst[Handle].CurrentTunerInfo.BitErrorRate = ( TEST_MAX_BER - (
            (Inst[Handle].CurrentTunerInfo.SignalQuality * TEST_MAX_BER) / 100 ));
    }
#endif
#endif

#ifdef STTUNER_MINIDRIVER
Error=demod_d0360_GetAGC(Inst[Handle].Terr.Demod.DrvHandle, &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);

 /* Get the current demodulation parameters */
 Error=demod_d0360_GetTunerInfo(Inst[Handle].Terr.Demod.DrvHandle, &Inst[Handle].CurrentTunerInfo);
    
    if (Error != ST_NO_ERROR)
    {

#ifdef STTUNER_DEBUG_MODULE_TERR_TERTASK
        STTBX_Print(("%s fail get demod params, Error = %u\n", identity, Error));
#endif
        return(Error);
    }			                                  
			                                  
#endif
    return(Error);
} /* TERTASK_GetTunerInfo() */

/* End of tertask.c */
