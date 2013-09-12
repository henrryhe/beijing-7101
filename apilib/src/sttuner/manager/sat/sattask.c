/*----------------------------------------------------------------------------
File Name: sattask.c (was tuner.c)

Description: Wrapper API for interfacing with the low-level tuner modules.

Copyright (C) 1999 STMicroelectronics

Version: 3.1.0

Revision History:

    21/01/00    Search algorithm updated to allow direct setting of LNB
                tone state in scan params.

    24/01/00    FEC rates are now setting during ScanExact() i.e., per
                scan list element rather than a global OR of all
                FEC rates for all scan list elements.

    9/07/2001   re-wrote as part of  version 3.1.0 STTUNER.

    4/08/2001   Added retries for single scan and automatic relocking

   22/10/2001   Added SetModulation() and band selection.

   22/02/2002   Updates to correct C band support.

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
#include "sttbx.h"
/* Standard includes */
#include "stlite.h"

#ifndef STTUNER_MINIDRIVER
#include "sioctl.h"
#include "util.h"       /* generic utility functions for sttuner */
#include "stevt.h"
#endif
#ifdef STTUNER_MINIDRIVER
  #ifdef STTUNER_DRV_SAT_STV0299
	#include "d0299.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#else
	  #include "lnbIO.h"
	#endif
  #endif
  #if defined(STTUNER_DRV_SAT_STV0399) || defined(STTUNER_DRV_SAT_STV0399E)
        #include "d0399.h"
	#ifdef STTUNER_DRV_SAT_LNBH21
	  #include "lnbh21.h"
	#elif defined STTUNER_DRV_SAT_LNB21
	  #include "lnb21.h"
	#elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	  #include "lnbh24.h"
	#else
	  #include "lnbIO.h"
	#endif
  #endif  
#endif

/* STAPI */
#include "stcommon.h"

#include "sttuner.h"
/* local to sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */
#include "sattask.h"   /* header for this file */
#include "stapitnr.h" 

/* Global variable to keep track of current running task */
task_t *TunerTask1, *TunerTask2; 
task_t *TimeoutTask1, *TimeoutTask2; 
/* Global variable which sets to 1 when both task are running and helps
 to differentiate when single tuner is running and when dual tuner is running */
U8 ucDualTunerFlag = 0;
U8 ucDualTimeoutFlag = 0;

/* Private types/constants ------------------------------------------------ */

/* Digital bandwidth roll-off */
#define DIG_ROLL_OFF_SCALING            100

#define SCALED_DIG_ROLL_OFF             135 /* Actual 1.35 */

/* Maximum signal quality */
#define TEST_MAX_BER 200000

/* Analog carrier bandwidth -- big step speeds up scan process */
#define ANALOG_CARRIER_BANDWIDTH_KHZ    29000
#define NUMBER_LOST_BEFORE_UNLOCK       3


/* Delay required for the scan task */
#ifndef STTUNER_SCAN_TASK_DELAY_MS

 #define STTUNER_SCAN_TASK_DELAY_MS 500  /* millisecond delay (0.5 sec) */

#endif

/* In Hz -- Usually 125KHz (Ver1 code in KHz) */
#define SATTASK_TUNER_STEP 125000

/* 6 recommended */
#define SATTASK_DEMOD_DEROTATOR_STEP 6
/* local functions  -------------------------------------------------------- */

void SATTASK_ScanTask(STTUNER_Handle_t *Handle);
void SATTASK_TimeoutTask(STTUNER_Handle_t *Handle);

ST_ErrorCode_t SATTASK_ProcessScanExact (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t SATTASK_ProcessTracking  (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t SATTASK_ProcessThresholds(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t SATTASK_ProcessNextScan  (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);

ST_ErrorCode_t SATTASK_GetTunerInfo (STTUNER_Handle_t Handle);
ST_ErrorCode_t SATTASK_LNB_PowerDown(STTUNER_Handle_t Handle);


/*----------------------------------------------------------------------------
Name: SATTASK_Open()

Description:
    After open.c initialized each tuner component and then stored each
    device's capabilities this fn starts up the scan task for an instance.

Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_Open(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int result;

    Inst = STTUNER_GetDrvInst();
 
    strcpy(Inst[Handle].Sat.ScanTask.TaskName, Inst[Handle].Name);


#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s this TaskName [%s]\n", identity, Inst[Handle].Sat.ScanTask.TaskName));
#endif

    /* Create the scan busy semaphore required to synchronize access to the scan status, etc. */

   Inst[Handle].Sat.ScanTask.GuardSemaphore =  STOS_SemaphoreCreateFifo(NULL,1);



    /* Create the timeout semaphore, this allows us to periodically wakeup the scan task to
      perform background processing e.g. tracking the locked frequency. */

    Inst[Handle].Sat.ScanTask.TimeoutSemaphore = STOS_SemaphoreCreateFifoTimeOut(NULL,0);


    /* Reset the delete flag to ensure that the task doesn't exit prematurely */
    Inst[Handle].Sat.ScanTask.DeleteTask = FALSE;

    /* Sets the scan task timer delay, this controls how often the scan task awakes to perform background processing */
    Inst[Handle].Sat.ScanTask.TimeoutDelayMs = STTUNER_SCAN_TASK_DELAY_MS;

    /* Initialize the scan task status to startup defaults */
    Inst[Handle].Sat.ScanInfo.Scan           = NULL;
    Inst[Handle].Sat.ScanInfo.ScanIndex      = 0;
    Inst[Handle].Sat.ScanInfo.NextFrequency  = 0;
    Inst[Handle].Sat.ScanInfo.LockCount      = 0;    /* For monitoring tracking */
    Inst[Handle].Sat.ScanInfo.PlrMask        = 0;      /* Current polarizations */
    Inst[Handle].Sat.ScanTask.TopLevelHandle = Handle;
    Inst[Handle].Sat.EnableReLocking = FALSE;

    /* Assume that the scan status is "unlocked" to begin with */
    Inst[Handle].TunerInfo.Status = STTUNER_STATUS_IDLE;
    Inst[Handle].PreStatus = STTUNER_STATUS_IDLE;
    Inst[Handle].PowerMode = STTUNER_NORMAL_POWER_MODE;
    Inst[Handle].Sat.EnableReLocking = FALSE;
    
  

/* Create and start the scan task */
    result = STOS_TaskCreate( (void(*)(void *))SATTASK_ScanTask,            /* task function      */
                        &Inst[Handle].Sat.ScanTask.TopLevelHandle,    /* user params        */
                        Inst[Handle].DriverPartition,				/* stack partition    */
                         SAT_SCANTASK_STACK_SIZE,                      /* stack size         */
                         (void*)&Inst[Handle].Sat.ScanTask.ScanTaskStack,     /* stack              */
                       Inst[Handle].DriverPartition,					/* task partiton       */
                        &Inst[Handle].Sat.ScanTask.Task,              /* task control block */
                        &Inst[Handle].Sat.ScanTask.TaskDescriptor,    /* task descriptor    */
                        STTUNER_SCAN_SAT_TASK_PRIORITY,               /* high priority, prevents stevt notify blocking */ /*removing hardcoded value for scantask priority for GNBvd25440 --> Allow override of task priorities*/
                        Inst[Handle].Sat.ScanTask.TaskName,           /* task name          */
                        0                                             /* flags              */
                      );
    

   if(Handle == 0)
     TunerTask1 = Inst[Handle].Sat.ScanTask.Task;
    if(Handle == 1) 
     TunerTask2 = Inst[Handle].Sat.ScanTask.Task;
   
    ++ucDualTunerFlag;
         
     
    /* Task creation failed? */
    if (result != 0)
    {
        Error = ST_ERROR_NO_MEMORY;
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s fail\n", identity));
#endif
    }




    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s task started ok\n", identity));
#endif
    }

    return(Error);

} /* STASK_Open() */


/*----------------------------------------------------------------------------
Name: SATTASK_TimeoutTaskOpen()

Description:
    This task will be created in sattask after scanning flag is rtue and there is exact scan operation.
    Thi is used to return a timeout event in case scanning taks more time than timeout, given by application

Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_TimeoutTaskOpen(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_TimeoutTaskOpen()";
#endif

    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int result;

    Inst = STTUNER_GetDrvInst();
 
    strcpy(Inst[Handle].Sat.TimeoutTask.TaskName, Inst[Handle].Name);
    
    /* Create the timeout semaphore, this allows us to periodically wakeup the scan task to
      perform background processing e.g. tracking the locked frequency. */
    Inst[Handle].Sat.TimeoutTask.TimeoutSemaphore = STOS_SemaphoreCreateFifoTimeOut(NULL,0);


    /* Reset the delete flag to ensure that the task doesn't exit prematurely */
    Inst[Handle].Sat.TimeoutTask.DeleteTask = FALSE;

    /* Sets the scan task timer delay, this controls how often the scan task awakes to perform background processing */
    Inst[Handle].Sat.TimeoutTask.TimeoutDelayMs = STTUNER_SCAN_TASK_DELAY_MS;

    Inst[Handle].Sat.TimeoutTask.TopLevelHandle = Handle;
    
    
      
      /* Create and start the timeout task */
    result = STOS_TaskCreate( (void(*)(void *))SATTASK_TimeoutTask,            /* task function      */
                        &Inst[Handle].Sat.TimeoutTask.TopLevelHandle,    /* user params        */
                        Inst[Handle].DriverPartition,				/* stack partition    */
                        SAT_SCANTASK_STACK_SIZE,                         /* stack size         */
                        (void*)&Inst[Handle].Sat.TimeoutTask.ScanTaskStack,     /* stack              */
                        Inst[Handle].DriverPartition,					/* task partiton       */
                        &Inst[Handle].Sat.TimeoutTask.Task,              /* task control block */
                        &Inst[Handle].Sat.TimeoutTask.TaskDescriptor,    /* task descriptor    */
                        STTUNER_SCAN_SAT_TASK_PRIORITY,                  /* high priority, prevents stevt notify blocking */ /*removing hardcoded value for scantask priority for GNBvd25440 --> Allow override of task priorities*/
                        Inst[Handle].Sat.TimeoutTask.TaskName,           /* task name          */
                        0                                                /* flags              */
                      );
    
         
     if(Handle == 0)
     TimeoutTask1 = Inst[Handle].Sat.TimeoutTask.Task;
    if(Handle == 1) 
     TimeoutTask2 = Inst[Handle].Sat.TimeoutTask.Task;
   
    ++ucDualTimeoutFlag;
    /* Task creation failed? */
    if (result != 0)
    {
        Error = ST_ERROR_NO_MEMORY;
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s fail\n", identity));
#endif
    }
   
   
   
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s task started ok\n", identity));
#endif
    }

    return(Error);

} /* STASK_Open() */




/*----------------------------------------------------------------------------
Name: SATTASK_Close()
Description:

Parameters:

Return Value:

See Also:
    STASK_Open()
----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_Close(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    task_t *task;

    #ifndef STTUNER_MINIDRIVER
    Error = SATTASK_ScanAbort(Handle);
    /* Also make LNB Power OFF -> done for teamlog request */
    Error |= SATTASK_LNB_PowerDown(Handle);
    #endif
   if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s fail SATTASK_ScanAbort()\n", identity));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s scan aborted ok\n", identity));
#endif
    }

    Inst = STTUNER_GetDrvInst();
	
	
	    task = Inst[Handle].Sat.ScanTask.Task;   
	   
           /* wait until scantask is out of its critical section then grab semaphore to keep it waiting */
	    STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);
	
	    /* Wakeup the scan task (fallthrough early) */
	    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.TimeoutSemaphore);
	
	 
    /* modify the delete task flag as waiting to be deleted */
    Inst[Handle].Sat.ScanTask.DeleteTask = TRUE;

    /* allow scantask to procede */

    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);

   
    
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s instance %d waiting for task [%s] to end...", identity, Handle, Inst[Handle].Sat.ScanTask.TaskName));
#endif

    /* The task should be in the process of returning, but wait for it first before deleting it. */
    if (STOS_TaskWait(&task, TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the task */
       
        if (STOS_TaskDelete(Inst[Handle].Sat.ScanTask.Task,Inst[Handle].DriverPartition,Inst[Handle].Sat.ScanTask.ScanTaskStack,Inst[Handle].DriverPartition) == 0)
      
        {
    	
            /* Delete all semaphores associated with the this scantask */
            STOS_SemaphoreDelete(NULL,Inst[Handle].Sat.ScanTask.GuardSemaphore);
   	    /* decrement the value of the ucDualTunerFlag which keep tracks of dual tuner */
    	    --ucDualTunerFlag;                   
            STOS_SemaphoreDelete(NULL,Inst[Handle].Sat.ScanTask.TimeoutSemaphore);

     
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
            STTBX_Print(("ok\n"));
#endif
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
            STTBX_Print(("fail task_delete()\n"));
#endif
            Error = ST_ERROR_DEVICE_BUSY;   /* task may not be deleted */
        }
    }


    return(Error);

} /* STASK_Close() */


/*----------------------------------------------------------------------------
Name: SATTASK_TimeoutTaskClose()
Description:

Parameters:

Return Value:

See Also:
    STASK_Open()
----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_TimeoutTaskClose(STTUNER_Handle_t Handle)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    task_t *task;

   
    Inst = STTUNER_GetDrvInst();

  
    task = Inst[Handle].Sat.TimeoutTask.Task;



    /* modify the delete task flag as waiting to be deleted */
    Inst[Handle].Sat.TimeoutTask.DeleteTask = TRUE;
   


#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s instance %d waiting for task [%s] to end...", identity, Handle, Inst[Handle].Sat.ScanTask.TaskName));
#endif
    /* The task should be in the process of returning, but wait for it first before deleting it. */
    if (STOS_TaskWait(&task,  TIMEOUT_INFINITY) == 0)
    {
  
        /* Now it should be safe to delete the task */
         if(STOS_TaskDelete(Inst[Handle].Sat.TimeoutTask.Task,Inst[Handle].DriverPartition,Inst[Handle].Sat.TimeoutTask.ScanTaskStack,Inst[Handle].DriverPartition)==0)

        {
         --ucDualTimeoutFlag;                   
        STOS_SemaphoreDelete(NULL,Inst[Handle].Sat.TimeoutTask.TimeoutSemaphore);
	}

      

     }       

    return(Error);

} /* SATTASK_TimeoutTaskClose() */

#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: SATTASK_ScanAbort()

Description:
    This routine will abort the current scan step.

    NOTE:   This call will block until it is able to safely abort the
            current scan operation -- a callback may be invoked if the
            caller has set one up.
Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_ScanAbort(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ScanAbort()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    
    SATIOCTL_SETREG_InParams_t InParams;
    STTUNER_Da_Status_t Status;
    int OutParams;
    U8 bit;

    
   
    Inst = STTUNER_GetDrvInst();
    
    
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
#endif

    /* Needs to go before the semaphore as it is the ultimate termination */
    Inst[Handle].ForceSearchTerminate = TRUE;
        
    /* Lock out the scan task whilst we modify the status code */
 
    STOS_SemaphoreWait(Inst[Handle].Sat.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("ok\n"));
#endif

    /* Forcibly unlock the tuner  */
    Inst[Handle].TunerInfo.Status    = STTUNER_STATUS_UNLOCKED;
    Inst[Handle].Sat.EnableReLocking = FALSE;
/* change IQ mode to unlock the H/W during scan abort-> GNBvd27468*/
    if(Inst[Handle].Sat.Demod.Driver->ID == STTUNER_DEMOD_STV0399)
    {
    /*IQ changed to unlock the H/W*/
    InParams.RegIndex = 0x30;
    InParams.Value    = 0x00;
    Error = (Inst[Handle].Sat.Demod.Driver->demod_ioctl)(Inst[Handle].Sat.Demod.DrvHandle,STTUNER_IOCTL_REG_IN, &InParams, &OutParams, &Status);
    bit = (OutParams) & 0x01;
    InParams.Value = (bit == 0) ? OutParams | 0x01 : OutParams & 0xFE ;
    Error = (Inst[Handle].Sat.Demod.Driver->demod_ioctl)(Inst[Handle].Sat.Demod.DrvHandle,STTUNER_IOCTL_REG_OUT, &InParams, &OutParams, &Status);
    }
    else if (Inst[Handle].Sat.Demod.Driver->ID == STTUNER_DEMOD_STV0299)
    {
    /*IQ changed to unlock the H/W*/
    InParams.RegIndex = 0x0C;
    InParams.Value    = 0x00;
    Error = (Inst[Handle].Sat.Demod.Driver->demod_ioctl)(Inst[Handle].Sat.Demod.DrvHandle,STTUNER_IOCTL_REG_IN, &InParams, &OutParams, &Status);
    bit = (OutParams) & 0x01;
    InParams.Value = (bit == 0) ? OutParams | 0x01 : OutParams & 0xFE ;
    Error = (Inst[Handle].Sat.Demod.Driver->demod_ioctl)(Inst[Handle].Sat.Demod.DrvHandle,STTUNER_IOCTL_REG_OUT, &InParams, &OutParams, &Status);
    
    }
    /* Release semaphore -- allows scan task to resume */
    
    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);


    return(Error);

} /* TUNER_AbortScan() */
#endif


/*----------------------------------------------------------------------------
Name: SATTASK_ScanStart()

Description:
    This routine will commence a new scan based on the current scan list.
    The scan list is first examined to ensure it is valid, if not an error
    is generated.  If the scan list is ok, the scan task is awoken and the
    first scan step will be started.

    NOTE:   If a scan is already in progress, the current scan will be
            aborted and re-started at the next available opportunity.

Parameters:

Return Value:
    TUNER_ERROR_BAD_PARAMETER,      the scan list is invalid.
    TUNER_NO_ERROR,                 the operation completed without error.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_ScanStart(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ScanStart()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    
#ifndef STTUNER_MINIDRIVER
    U32 SymbolMin, loop;
    Inst = STTUNER_GetDrvInst();
    /* if the band list is not setup then the scan is not valid so exit */
    if (Inst[Handle].BandList.NumElements < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s fail STTUNER_SetBandList() not done?\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   /* if the scan list is not setup then the scan is not valid so exit */ 
    if ((!Inst[Handle].Sat.ScanExact) && (!Inst[Handle].Sat.BlindScan))
    {
	    if (Inst[Handle].ScanList.NumElements < 1)
	    {
	#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
	        STTBX_Print(("%s fail STTUNER_SetScanList() not done?\n", identity));
	#endif
	        return(ST_ERROR_BAD_PARAMETER);
	    }
    }

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
#endif


    /* Wakeup the scan task and lock out the scan task whilst we modify the status code. */
    STOS_SemaphoreWait(  Inst[Handle].Sat.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("ok\n"));
#endif


    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.TimeoutSemaphore);
  

    /* Set the scan status and reset the scan element pointer */
    
    
        if(Inst[Handle].PowerMode == STTUNER_STANDBY_POWER_MODE) 
        {	
        	
         Inst[Handle].TunerInfo.Status = STTUNER_STATUS_STANDBY;
         
        }
        else
        {
     
        /* Set the scan status and reset the scan element pointer */	
         Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING;
         Inst[Handle].Sat.EnableReLocking = FALSE;   /* only relock on first single scan with a valid frequency */
         Inst[Handle].ForceSearchTerminate = FALSE;
          
        }

    /* First calculate the minimum symbol rate for the scan */
    SymbolMin = ((U32)~0);
    if((!Inst[Handle].Sat.ScanExact) && (!Inst[Handle].Sat.BlindScan))
    {
	    for (loop = 0; loop < Inst[Handle].ScanList.NumElements; loop++)
	    {
	        if (Inst[Handle].ScanList.ScanList[loop].SymbolRate < SymbolMin)
	        {
	            SymbolMin = Inst[Handle].ScanList.ScanList[loop].SymbolRate;
	        }
	    }
    }
    else if(Inst[Handle].Sat.ScanExact)
    {
    	SymbolMin = Inst[Handle].Sat.SingleScan.SymbolRate;
    }
    
    /* Store the minimum symbol width for future reference */
    if(!Inst[Handle].Sat.BlindScan)
    {
    	Inst[Handle].Sat.SymbolWidthMin = ( (SymbolMin / 1000) * SCALED_DIG_ROLL_OFF) / (DIG_ROLL_OFF_SCALING);
    
        /* Set frequency step to default */
        if (Inst[Handle].Sat.Ioctl.TunerStepSize == 0) 
        {
            Inst[Handle].Sat.ScanInfo.FrequencyStep = (Inst[Handle].Sat.SymbolWidthMin); /* removed /2 factor to FrequencyStep to avoid repeated search in second half of search range */
        }
        else
        {
            Inst[Handle].Sat.ScanInfo.FrequencyStep = Inst[Handle].Sat.Ioctl.TunerStepSize;
        }
    }
    else if(Inst[Handle].Sat.BlindScan)
    Inst[Handle].Sat.ScanInfo.FrequencyStep = Inst[Handle].Sat.Ioctl.TunerStepSize;
    

    /* Setup scan parameters is we're using the scan list */
    if ((!Inst[Handle].Sat.ScanExact) && (!Inst[Handle].Sat.BlindScan))
    {
		#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
		        STTBX_Print(("%s using scan list\n", identity));
		#endif
        Inst[Handle].Sat.ScanInfo.Scan          = Inst[Handle].ScanList.ScanList;
        Inst[Handle].Sat.ScanInfo.ScanIndex     = 0;
        Inst[Handle].Sat.ScanInfo.NextFrequency = Inst[Handle].Sat.ScanInfo.FrequencyStart;
        
        Inst[Handle].Sat.ScanInfo.PlrMask       = STTUNER_PLR_HORIZONTAL; /* H first */
        
        #ifdef STTUNER_DRV_SAT_SCR
        if((Inst[Handle].Capability.SCREnable) && (Inst[Handle].Sat.ScanInfo.Scan->LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        {
        	Inst[Handle].Sat.ScanInfo.NextFrequency   = Inst[Handle].Sat.ScanInfo.FrequencyStart;
        	Inst[Handle].Sat.ScanInfo.PlrMask         = STTUNER_PLR_ALL;    /*polarization shouldn't be masked in SCR mode*/
        }
        else
        {
        	Inst[Handle].Sat.ScanInfo.NextFrequency = Inst[Handle].Sat.ScanInfo.FrequencyStart;
	        Inst[Handle].Sat.ScanInfo.PlrMask       = STTUNER_PLR_HORIZONTAL; /* H first */
        }
        #endif
        
        /* Find out what polarizations are in use */
        Inst[Handle].Sat.PolarizationMask = 0;

        for (loop = 0; loop <  Inst[Handle].ScanList.NumElements; loop++)
        {
            Inst[Handle].Sat.PolarizationMask |= Inst[Handle].ScanList.ScanList[loop].Polarization;
        }

    }
    else if ((Inst[Handle].Sat.ScanExact) && (!Inst[Handle].Sat.BlindScan))
    {
        #ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s using single scan\n", identity));
        #endif
        /* We only have one scan parameter for an exact scan */
        Inst[Handle].Sat.ScanInfo.Scan          = &Inst[Handle].Sat.SingleScan;
        Inst[Handle].Sat.ScanInfo.PlrMask       = Inst[Handle].Sat.SingleScan.Polarization;
        Inst[Handle].Sat.PolarizationMask       = Inst[Handle].Sat.SingleScan.Polarization;
        Inst[Handle].Sat.FECRates               = Inst[Handle].Sat.SingleScan.FECRates;
        Inst[Handle].Sat.ScanInfo.NextFrequency = Inst[Handle].Sat.ScanInfo.FrequencyStart;
    }
    else if(Inst[Handle].Sat.BlindScan)
    {
    	Inst[Handle].Sat.ScanInfo.BlindScanInfo          = &Inst[Handle].Sat.BlindScanInfo;
    	Inst[Handle].Sat.SymbolWidthMin         = 0;
    	Inst[Handle].Sat.ScanInfo.NextFrequency = Inst[Handle].Sat.ScanInfo.FrequencyStart;
    	Inst[Handle].Sat.ScanInfo.PlrMask       = STTUNER_PLR_HORIZONTAL; /* H first */
        #ifdef STTUNER_DRV_SAT_SCR
        if((Inst[Handle].Capability.SCREnable) && (Inst[Handle].Sat.BlindScanInfo.LNB_SignalRouting == STTUNER_VIA_SCRENABLED_LNB))
        {
        	Inst[Handle].Sat.ScanInfo.PlrMask       = STTUNER_PLR_ALL;    /*polarization shouldn't be masked in SCR mode*/
		}
		else
		{
			Inst[Handle].Sat.ScanInfo.PlrMask       = STTUNER_PLR_HORIZONTAL; /* H first */
		}
        
        #endif
        /* Find out what polarizations are in use */
        Inst[Handle].Sat.PolarizationMask = STTUNER_PLR_ALL;
    }
    /* Ensure polarization mask is within device capabilities */
    Inst[Handle].Sat.PolarizationMask &= Inst[Handle].Capability.Polarization;
    /* Release semaphore -- allows scan task to resume */

    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);


#endif

#ifdef STTUNER_MINIDRIVER
    Inst = STTUNER_GetDrvInst();

/* if the band list is not setup then the scan is not valid so exit */
    if (Inst[Handle].BandList.NumElements < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s fail STTUNER_SetBandList() not done?\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   

    /* Wakeup the scan task and lock out the scan task whilst we modify the status code. */
    STOS_SemaphoreWait(  Inst[Handle].Sat.ScanTask.GuardSemaphore);


    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.TimeoutSemaphore);


    /* Set the scan status and reset the scan element pointer */
    Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING;
    Inst[Handle].Sat.EnableReLocking = FALSE;   /* only relock on first single scan with a valid frequency */
    Inst[Handle].ForceSearchTerminate = FALSE;

   
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s using single scan\n", identity));
#endif
       
        /* Ensure polarization mask is within device capabilities */
    	/*Inst[Handle].Sat.PolarizationMask &= Inst[Handle].Capability.Polarization;*/
    /* Release semaphore -- allows scan task to resume */

    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);
   

#endif
    return(Error);

} /* SATTASK_ScanStart() */
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: SATTASK_ScanContinue()
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
ST_ErrorCode_t SATTASK_ScanContinue(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ScanContinue()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 FrequencyStep, SymbolLocked;

    Inst = STTUNER_GetDrvInst();

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
#endif

    /* Wakeup the scan task and lock out the scan task whilst we modify the status code */

    STOS_SemaphoreWait(  Inst[Handle].Sat.ScanTask.GuardSemaphore);
     

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("ok\n"));
#endif


    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.TimeoutSemaphore);


    /* A scan may only be continued if the current scan status is
      "locked" or "unlocked". i.e., "not found" implies the end
      of the frequency range has been encountered or a scan has not
      yet started.  "scanning" implies the current scan step has
      not yet completed, therefore it doesn't make sense to
      continue yet */
    if ( (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_LOCKED)  ||
         (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_UNLOCKED) )
      {
        /* The last scan was successful, so a bigger step can be calculated */
        if (Inst[Handle].TunerInfo.Status == STTUNER_STATUS_LOCKED)
        {
            /* Get the current locked symbol rate (Khz) */
            if(Inst[Handle].Sat.BlindScan)
            {
            	SymbolLocked = Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate/1000;
            	FrequencyStep = SymbolLocked * SCALED_DIG_ROLL_OFF /
                            (2 * DIG_ROLL_OFF_SCALING) + Inst[Handle].Sat.ScanInfo.FrequencyStep/2;
            }
            else
            {
            	SymbolLocked = Inst[Handle].Sat.ScanInfo.Scan->SymbolRate / 1000;
            	FrequencyStep = SymbolLocked * SCALED_DIG_ROLL_OFF /
                            (2 * DIG_ROLL_OFF_SCALING) + (Inst[Handle].Sat.SymbolWidthMin  / 2 );
                /* Continue at the next frequency step */
                Inst[Handle].Sat.ScanInfo.ScanIndex = Inst[Handle].ScanList.NumElements;
            }
	    
            Inst[Handle].Sat.ScanInfo.FrequencyStep = FrequencyStep;
            
        }

        Inst[Handle].TunerInfo.Status   = STTUNER_STATUS_SCANNING;

        /* only relock on first single scan with a valid frequency */
        Inst[Handle].Sat.EnableReLocking = FALSE;
    }
    else
    {
        /* Unable to continue the scan -- the scan task is in the wrong
           state or the we've reached the end of the scan list */
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s fail incorrect scan state or end of scan list\n", identity));
#endif
        Error = ST_ERROR_BAD_PARAMETER;
    }

    /* Release semaphore -- allows scan task to resume */

    STOS_SemaphoreSignal(Inst[Handle].Sat.ScanTask.GuardSemaphore);

    return(Error);

} /* SATTASK_ScanContinue() */
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
/*#define DEBUG_SCANTASK */
void SATTASK_ScanTask(STTUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ScanTask()";
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
    U32 a, retries = 1;
    U8 TimeoutTaskDeletedFlag = 0;
   
    Inst = STTUNER_GetDrvInst();
    index = *Handle;
    
#ifndef STTUNER_MINIDRIVER
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s Task for instance %d up.\n", identity, index));
#endif

Inst[index].CurrentPreStatus = STTUNER_STATUS_IDLE;
Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_IDLE;
Inst[index].CurrentPowerMode = STTUNER_NORMAL_POWER_MODE;
    while(1)
    {
#ifdef ST_OS21
        /* Calculate wake-up time for next scan process */
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Sat.ScanTask.TimeoutDelayMs)/ 1000));

       
#else
        /* Calculate wake-up time for next scan process */
        #ifdef PROCESSOR_C1 /*for GNBvd48688-->Time calculation will be overflow*/
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond()/1000) * Inst[index].Sat.ScanTask.TimeoutDelayMs));
        #else
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Sat.ScanTask.TimeoutDelayMs)/ 1000));
        #endif

#endif/*ST_OS21*/

     
         /* Delay until either we're awoken or the wake-up time comes */
        STOS_SemaphoreWaitTimeOut(Inst[index].Sat.ScanTask.TimeoutSemaphore, &clk);
        /* We are about to enter a critical section of code -- we must not
         * allow external interruptions whilst checking the scan status, etc*/
        STOS_SemaphoreWait(Inst[index].Sat.ScanTask.GuardSemaphore);


        /* If the driver is being terminated then this flag will be set
         * to TRUE -- we should cleanly exit the task. */
        if (Inst[index].Sat.ScanTask.DeleteTask == TRUE)
        {
    	
            STOS_SemaphoreSignal(Inst[index].Sat.ScanTask.GuardSemaphore);
    
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
                /*Inst[index].CurrentTunerInfo.PowerMode=STTUNER_NORMAL_POWER_MODE;  */         
                Inst[index].CurrentPowerMode=STTUNER_NORMAL_POWER_MODE; 
                if (Inst[index].Sat.ScanExact)
                {
                    if((Inst[index].Sat.Timeout != 0) && (Inst[index].Sat.EnableReLocking != TRUE))
                    {TimeoutTaskDeletedFlag = 0;
                    Error = SATTASK_TimeoutTaskOpen(index);}
                    
                    for (a = 1; a <= retries; a++)
                    {
                    	Error = SATTASK_ProcessScanExact(index, &Event);
                        if (Event == STTUNER_EV_LOCKED) break;
                    }

                    /* for(a) breaks to here */
                    if (Inst[index].Sat.EnableReLocking == FALSE) /* if first pass */
                    {
                        if (Event != STTUNER_EV_LOCKED)  /* failed to lock */
                        {
                            /* Since we are scanning for a single frequency, unless we get
                            the locked event we should treat this as scan failed */
                            if (Event != STTUNER_EV_LNB_FAILURE) /* If Event is STTUNER_EV_LNB_FAILURE then dont
                            				 change the event status */
                            {
                            Event = STTUNER_EV_SCAN_FAILED;
                            }
                        }
                        else /* locked ok */
                        {
                           Inst[index].Sat.EnableReLocking = TRUE; /* also, at this point: Event == STTUNER_EV_LOCKED */
                        }
                    }

                }   /* if (ScanExact) */
                else
                {
                   
                    Inst[index].Sat.EnableReLocking = FALSE;     /* not valid when scanning a list */
                    Error = SATTASK_ProcessNextScan(index, &Event);
                   
                }
               
                break;


            /* In the "locked" case we have a frequency that we must attempt to maintain, or track */
            case STTUNER_STATUS_LOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_LOCKED\n"));
#endif


                Error = SATTASK_ProcessTracking(index, &Event);
                /* critical flag to lock copy of currenttunerinfo in tunerinfo in file get.c before this ProcessTracking, so always return latest tunerinfo*/
				
				/* Only process thresholds if the lock wasn't lost */
                if (Event == STTUNER_EV_NO_OPERATION)
                {
                    Error = SATTASK_ProcessThresholds(index, &Event);
                   
                }
                else if (Inst[index].Sat.EnableReLocking == TRUE) /* if lost lock then try again */
                {

                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_SCANNING;

                }
                
                break;


             case STTUNER_STATUS_STANDBY:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_STANDBY\n"));
#endif
                 
                 Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_STANDBY;
                 Inst[index].CurrentPowerMode=STTUNER_STANDBY_POWER_MODE;
                 /*Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_STANDBY;
                 
                 Inst[index].CurrentTunerInfo.PowerMode=STTUNER_STANDBY_POWER_MODE;*/
                
                 Event = STTUNER_EV_NO_OPERATION;
                
                 
               
                break;



           case STTUNER_STATUS_IDLE:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_IDLE\n"));
#endif           
                Inst[index].CurrentPowerMode=STTUNER_NORMAL_POWER_MODE;
                /*Inst[index].CurrentTunerInfo.PowerMode=STTUNER_NORMAL_POWER_MODE;*/
                /* Fall through */


            /* "not found" or "unlocked" cases require no further action.
                The scan task will have to wait for user intervention */
            case STTUNER_STATUS_UNLOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_UNLOCKED\n"));
#endif
                /* Check the last status to ensure that the state hasn't 
                been changed externally e.g., scan aborted */
                if (Inst[index].Sat.EnableReLocking == TRUE)
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
            if ((Inst[index].TunerInfo.Status == STTUNER_STATUS_SCANNING) && (Inst[index].Sat.EnableReLocking == FALSE)) 
            {
                Event = STTUNER_EV_SCAN_FAILED;
            }
            else if (Inst[index].TunerInfo.Status == STTUNER_STATUS_LOCKED)
            {   
                Event = STTUNER_EV_UNLOCKED;
            }
            
        }

        /* Check the outcome of the last operation */
        switch (Event)
        {
            case STTUNER_EV_LOCKED:                     /* We are now locked */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_LOCKED;
                Inst[index].Sat.ScanInfo.LockCount  = NUMBER_LOST_BEFORE_UNLOCK;
                /*Inst[index].CurrentTunerInfo.PreStatus = STTUNER_STATUS_LOCKED;*/
                Inst[index].CurrentPreStatus = STTUNER_STATUS_LOCKED;
                /* Reset signal indicators */
                Inst[index].Sat.LastThreshold = ((U32)-1);     /* Undefined threshold */
                Inst[index].Sat.LastSignal    = ((U32)-1);     /* Undefined signal */
                break;

            case STTUNER_EV_UNLOCKED:                   /* We have lost lock */
                if (Inst[index].Sat.EnableReLocking != TRUE)
                {
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_UNLOCKED;
                    Inst[index].CurrentPreStatus = STTUNER_STATUS_UNLOCKED;
                     /*Inst[index].CurrentTunerInfo.PreStatus = STTUNER_STATUS_UNLOCKED;*/
                }
                break;

            case STTUNER_EV_SCAN_FAILED:                /* The last scan failed */
                /* The end of the scan list has been reached, we drop to the
                 "not found" status.  A callback may be invoked soon */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                Inst[index].CurrentPreStatus = STTUNER_STATUS_NOT_FOUND;
                 /*Inst[index].CurrentTunerInfo.PreStatus = STTUNER_STATUS_NOT_FOUND;*/
                break;

            case STTUNER_EV_TIMEOUT:
                /* If a timeout occurs, we do not allow the scan to continue.
                 this is effectively the same as reaching the end of the
                 scan list -- we reset our status to "not found" */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                Inst[index].CurrentPreStatus = STTUNER_STATUS_NOT_FOUND;
                /*Inst[index].CurrentTunerInfo.PreStatus = STTUNER_STATUS_NOT_FOUND; */
                break;

            case STTUNER_EV_SIGNAL_CHANGE:
                /* No action required.  The event will be processed in the
                 * call back below...
                 */
                break;
            case STTUNER_EV_LNB_FAILURE:                /* lnb failed due to overheating or over current */
                /*  */
                if ((Inst[index].Sat.ScanExact) && (Inst[index].Sat.EnableReLocking != TRUE))
                {
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
	        }
	        else if((!Inst[index].Sat.ScanExact)&& (!Inst[index].Sat.BlindScan))
	        {
	             if ( (Inst[index].TunerInfo.Status       == STTUNER_STATUS_SCANNING         ) &&
                     (Inst[index].Sat.ScanInfo.ScanIndex <  Inst[index].ScanList.NumElements) &&
                     (Inst[index].Sat.EnableReLocking    != TRUE                            ) )
                     {
	                    /* Proceed to the next element.  In order to avoid a spurious callback being invoked,
	                    we reset the event to a "no op" */
	                    Inst[index].Sat.ScanInfo.Scan++;
	                    Inst[index].Sat.ScanInfo.ScanIndex++;
	
	                    /* Avoid task delay */
               
	                    STOS_SemaphoreSignal(Inst[index].Sat.ScanTask.TimeoutSemaphore);
                    
	              }
	
	              Inst[index].CurrentTunerInfo.Status = Inst[index].TunerInfo.Status;
	              Inst[index].CurrentPreStatus = Inst[index].PreStatus;
	              Inst[index].CurrentPowerMode = Inst[index].PowerMode;
	        }
	         else if(Inst[index].Sat.BlindScan)
	        {
		        if ( (Inst[index].TunerInfo.Status       == STTUNER_STATUS_SCANNING)  &&
		           (Inst[index].Sat.EnableReLocking    != TRUE) )
		        {
		                   
		                    /* Avoid task delay */
					           
		                    STOS_SemaphoreSignal(Inst[index].Sat.ScanTask.TimeoutSemaphore);
					                 
	                }
	                Inst[index].CurrentTunerInfo.Status = Inst[index].TunerInfo.Status;
	                Inst[index].CurrentPreStatus = Inst[index].PreStatus;
	                Inst[index].CurrentPowerMode = Inst[index].PowerMode;
	        }
	        
        
                break;

            default:
                /* We must now proceed to the next scan step, if there is one,
                 if there isn't a next scan step then a callback might be invoked */
                if((!Inst[index].Sat.ScanExact) && (!Inst[index].Sat.BlindScan))
                {
                	if ( (Inst[index].TunerInfo.Status       == STTUNER_STATUS_SCANNING         ) &&
	                     (Inst[index].Sat.ScanInfo.ScanIndex <  Inst[index].ScanList.NumElements) &&
	                     (Inst[index].Sat.EnableReLocking    != TRUE                            ) )
	                {
	                    /* Proceed to the next element.  In order to avoid a spurious callback being invoked,
	                    we reset the event to a "no op" */
	                    Inst[index].Sat.ScanInfo.Scan++;
	                    Inst[index].Sat.ScanInfo.ScanIndex++;
	
	                    /* Avoid task delay */
	       
	                    STOS_SemaphoreSignal(Inst[index].Sat.ScanTask.TimeoutSemaphore);
	              
	                }
	        }
	        else if(Inst[index].Sat.BlindScan)
	        {
		        if ( (Inst[index].TunerInfo.Status       == STTUNER_STATUS_SCANNING)  &&
		           (Inst[index].Sat.EnableReLocking    != TRUE) )
		        {
		                   
		                    /* Avoid task delay */
					              
		                    STOS_SemaphoreSignal(Inst[index].Sat.ScanTask.TimeoutSemaphore);
					       
	                }
	        }

                Inst[index].CurrentTunerInfo.Status = Inst[index].TunerInfo.Status;
                Inst[index].CurrentPreStatus = Inst[index].PreStatus;
	Inst[index].CurrentPowerMode = Inst[index].PowerMode;
               
                break;

        }   /* switch(Event) */

       
        /* Commit new tuner information -- the API level can now see it */
         Inst[index].TunerInfo = Inst[index].CurrentTunerInfo;
         Inst[index].PreStatus = Inst[index].CurrentPreStatus  ;
         Inst[index].PowerMode = Inst[index].CurrentPowerMode ;        

        /* Critical section end */
      
        /* Check whether timeout task has return its event */
        if ((Inst[index].Sat.ScanExact) &&  (Inst[index].Sat.Timeout != 0))
        {
        
	        if(Inst[index].Sat.TimeoutTask.DeleteTask == TRUE)
	        {
	        	Event = STTUNER_EV_NO_OPERATION;
	        	if(TimeoutTaskDeletedFlag == 0)
	        	{
	        		Error = SATTASK_TimeoutTaskClose(index);
	        		Inst[index].Sat.TimeoutTask.DeleteTask = FALSE;
	        		TimeoutTaskDeletedFlag = 1;
	        	}
	        	
	        }
	        
	        if(Event != STTUNER_EV_NO_OPERATION)
	        {
	        	if(TimeoutTaskDeletedFlag == 0)
	        	{
        		
	        		STOS_SemaphoreSignal(Inst[index].Sat.TimeoutTask.TimeoutSemaphore);

	        		
	        		Error = SATTASK_TimeoutTaskClose(index);
	        		Inst[index].Sat.TimeoutTask.DeleteTask = FALSE;
	        		TimeoutTaskDeletedFlag = 1;
	        	}
	        }
	        
        }
     
        STOS_SemaphoreSignal(Inst[index].Sat.ScanTask.GuardSemaphore);


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

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
            if (Error != ST_NO_ERROR) STTBX_Print(("%s STEVT error = %u\n", identity, Error));
#endif
        }

    }   /* while(1)*/    

#endif

#ifdef STTUNER_MINIDRIVER

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s Task for instance %d up.\n", identity, index));
#endif
    while(1)
    {
#ifdef ST_OS21
        /* Calculate wake-up time for next scan process */
        clk = time_plus(time_now(), ((ST_GetClocksPerSecond() * Inst[index].Sat.ScanTask.TimeoutDelayMs)/ 1000));

        /* Delay until either we're awoken or the wake-up time comes */
        semaphore_wait_timeout(Inst[index].Sat.ScanTask.TimeoutSemaphore, &clk);
        /* We are about to enter a critical section of code -- we must not
         * allow external interruptions whilst checking the scan status, etc*/
        semaphore_wait(Inst[index].Sat.ScanTask.GuardSemaphore);
#else
        /* Calculate wake-up time for next scan process */
        #ifdef PROCESSOR_C1 /*for GNBvd48688-->Time calculation will be overflow*/
        clk = time_plus(time_now(), ((ST_GetClocksPerSecond() / 1000) * Inst[index].Sat.ScanTask.TimeoutDelayMs));
        #else
        clk = time_plus(time_now(), ((ST_GetClocksPerSecond() * Inst[index].Sat.ScanTask.TimeoutDelayMs)/ 1000));
        #endif
#ifdef ST_OSLINUX
        semaphore_wait_timeout(Inst[index].Sat.ScanTask.TimeoutSemaphore, &clk);
        semaphore_wait(Inst[index].Sat.ScanTask.GuardSemaphore);
#else
        

        /* Delay until either we're awoken or the wake-up time comes */
        semaphore_wait_timeout(&Inst[index].Sat.ScanTask.TimeoutSemaphore, &clk);
        /* We are about to enter a critical section of code -- we must not
         * allow external interruptions whilst checking the scan status, etc*/
        semaphore_wait(&Inst[index].Sat.ScanTask.GuardSemaphore);
#endif /* ifdef ST_OSLINUX */
#endif
        /* If the driver is being terminated then this flag will be set
         * to TRUE -- we should cleanly exit the task. */
        if (Inst[index].Sat.ScanTask.DeleteTask == TRUE)
        {
#if defined(ST_OS21) || defined(ST_OSLINUX)     	      	
            semaphore_signal(Inst[index].Sat.ScanTask.GuardSemaphore);
#else
            semaphore_signal(&Inst[index].Sat.ScanTask.GuardSemaphore);
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
                
                if (Inst[index].Sat.ScanExact)
                {
                    if((Inst[index].Sat.Timeout != 0) && (Inst[index].Sat.EnableReLocking != TRUE))
                    {TimeoutTaskDeletedFlag = 0;
                    Error = SATTASK_TimeoutTaskOpen(index);}
                    
                    for (a = 1; a <= retries; a++)
                    {
                    	Error = SATTASK_ProcessScanExact(index, &Event);
                        if (Event == STTUNER_EV_LOCKED) break;
                    }

                    /* for(a) breaks to here */
                    if (Inst[index].Sat.EnableReLocking == FALSE) /* if first pass */
                    {
                        if (Event != STTUNER_EV_LOCKED)  /* failed to lock */
                        {
                            /* Since we are scanning for a single frequency, unless we get
                            the locked event we should treat this as scan failed */
                            if (Event != STTUNER_EV_LNB_FAILURE) /* If Event is STTUNER_EV_LNB_FAILURE then dont
                            				 change the event status */
                            {
                            Event = STTUNER_EV_SCAN_FAILED;
                            }
                        }
                        else /* locked ok */
                        {
                           Inst[index].Sat.EnableReLocking = TRUE; /* also, at this point: Event == STTUNER_EV_LOCKED */
                        }
                    }

                }   /* if (ScanExact) */
                               
                break;


            /* In the "locked" case we have a frequency that we must attempt to maintain, or track */
            case STTUNER_STATUS_LOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_LOCKED\n"));
#endif


                Error = SATTASK_ProcessTracking(index, &Event);

                /* Only process thresholds if the lock wasn't lost */
                if (Event == STTUNER_EV_NO_OPERATION)
                {

                    /*Error = SATTASK_ProcessThresholds(index, &Event);*/

                }
                else if (Inst[index].Sat.EnableReLocking == TRUE) /* if lost lock then try again */
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
                if (Inst[index].Sat.EnableReLocking == TRUE)
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
                Inst[index].Sat.ScanInfo.LockCount  = NUMBER_LOST_BEFORE_UNLOCK;

                /* Reset signal indicators */
                Inst[index].Sat.LastThreshold = ((U32)-1);     /* Undefined threshold */
                Inst[index].Sat.LastSignal    = ((U32)-1);     /* Undefined signal */
                break;

            case STTUNER_EV_UNLOCKED:                   /* We have lost lock */
                if (Inst[index].Sat.EnableReLocking != TRUE)
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
            case STTUNER_EV_LNB_FAILURE:                /* lnb failed due to overheating or over current */
                /*  */
                if ((Inst[index].Sat.ScanExact) && (Inst[index].Sat.EnableReLocking != TRUE))
                {
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
	        }
	       
        
                break;

            default:
                /* We must now proceed to the next scan step, if there is one,
                 if there isn't a next scan step then a callback might be invoked */
                
                Inst[index].CurrentTunerInfo.Status = Inst[index].TunerInfo.Status;
                break;

        }   /* switch(Event) */


        /* Commit new tuner information -- the API level can now see it */
        Inst[index].TunerInfo = Inst[index].CurrentTunerInfo;

        /* Critical section end */
        
        /* Check whether timeout task has return its event */
        if ((Inst[index].Sat.ScanExact) &&  (Inst[index].Sat.Timeout != 0))
        {
        
	        if(Inst[index].Sat.TimeoutTask.DeleteTask == TRUE)
	        {
	        	Event = STTUNER_EV_NO_OPERATION;
	        	if(TimeoutTaskDeletedFlag == 0)
	        	{
	        		Error = SATTASK_TimeoutTaskClose(index);
	        		Inst[index].Sat.TimeoutTask.DeleteTask = FALSE;
	        		TimeoutTaskDeletedFlag = 1;
	        	}
	        	
	        }
	        
	        if(Event != STTUNER_EV_NO_OPERATION)
	        {
	        	if(TimeoutTaskDeletedFlag == 0)
	        	{
#if defined(ST_OS21) || defined(ST_OSLINUX)       		
	        		semaphore_signal(Inst[index].Sat.TimeoutTask.TimeoutSemaphore);
#else
				semaphore_signal(&Inst[index].Sat.TimeoutTask.TimeoutSemaphore);
#endif
	        		
	        		Error = SATTASK_TimeoutTaskClose(index);
	        		Inst[index].Sat.TimeoutTask.DeleteTask = FALSE;
	        		TimeoutTaskDeletedFlag = 1;
	        	}
	        }
	        
        }
#if defined(ST_OS21) || defined(ST_OSLINUX)         
        semaphore_signal(Inst[index].Sat.ScanTask.GuardSemaphore);
#else
	semaphore_signal(&Inst[index].Sat.ScanTask.GuardSemaphore);
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

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
            if (Error != ST_NO_ERROR) STTBX_Print(("%s STEVT error = %u\n", identity, Error));
#endif
        }

    }   /* while(1)*/    

#endif




#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s task ending\n", identity));
#endif
    return;

} /* SATTASK_ScanTask() */


void SATTASK_TimeoutTask(STTUNER_Handle_t *Handle)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_EventType_t Event = STTUNER_EV_NO_OPERATION;
    STTUNER_EvtParams_t EventInfo;                                                                                                                                                                 
    STTUNER_InstanceDbase_t *Inst;
    clock_t Timeout;
    STTUNER_Handle_t index;
    int i = 0;
    
    Inst = STTUNER_GetDrvInst();
    index = *Handle;
    
#ifdef ST_OS21   
     Timeout = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Sat.Timeout)/ 1000));
     
#else
     #ifdef PROCESSOR_C1 /*for GNBvd48688-->Time calculation will be overflow*/
     Timeout = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() / 1000) * Inst[index].Sat.Timeout));
     #else
     Timeout = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Sat.Timeout)/ 1000));
     #endif
     
i = STOS_SemaphoreWaitTimeOut(Inst[index].Sat.TimeoutTask.TimeoutSemaphore, &Timeout);
#endif  /* ifdef ST_OS21 */     
	        if(i == -1)
	        {
		        Event = STTUNER_EV_TIMEOUT;
		        Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
		        Inst[index].TunerInfo = Inst[index].CurrentTunerInfo;
		        
					
			    /* Use the event handler to notify */
		            EventInfo.Handle = INDEX2HANDLE(index);
		            EventInfo.Error  =  Error;
			        if(Inst[index].NotifyFunction != NULL)/*GNBvd36969*/
				{
					(Inst[index].NotifyFunction)(INDEX2HANDLE(index), Event, Error);
				}
				else
				{
		         		 Error = STEVT_Notify(Inst[index].EVTHandle, Inst[index].EvtId[EventToId(Event)], &EventInfo);
		         	}
		          		  Inst[index].Sat.TimeoutTask.DeleteTask = TRUE;
		
				}


	
	return;
}



/*----------------------------------------------------------------------------
Name: SATTASK_ProcessScanExact()
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
/* #define SATTASK_SYMOK */
ST_ErrorCode_t SATTASK_ProcessScanExact(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ProcessScanExact()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    BOOL ScanSuccess = FALSE;
    U32  ScanFrequency;
    U32  NewFrequency;
    U32  index;
    LNB_Config_t      LnbConfig;
    STTUNER_Band_t   *Band;
    
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21 )|| defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)   
    BOOL  IsOverTemp, IsCurrentOvrLoad;
#endif    
    
#ifndef STTUNER_MINIDRIVER
    STTUNER_FECRate_t    Fec;
    STTUNER_Modulation_t Mod;
    Inst = STTUNER_GetDrvInst();

    *Event = STTUNER_EV_NO_OPERATION;   /* Assume no lock is found */
    /* Setup the current element -- this represents a working copy we can
     modify of the current element in the scan list -- on completion it
     will be made visible to the ST TUNER API.*/
    if(!Inst[Handle].Sat.BlindScan)
    {
    	Inst[Handle].CurrentTunerInfo.ScanInfo = *Inst[Handle].Sat.ScanInfo.Scan;
    }
    else
    {
    	Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization = Inst[Handle].Sat.BlindScanInfo.Polarization;
    	Inst[Handle].CurrentTunerInfo.ScanInfo.LNBToneState = Inst[Handle].Sat.BlindScanInfo.LNBToneState;
    	Inst[Handle].CurrentTunerInfo.ScanInfo.Band = Inst[Handle].Sat.BlindScanInfo.Band;
    	Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate = 0;
    	#ifdef STTUNER_DRV_SAT_SCR
    	Inst[Handle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting = Inst[Handle].Sat.BlindScanInfo.LNB_SignalRouting;
    	#endif
    }
    /*Adding following line for GNBvd28574(Useage of STTUNER_GetTunerInfo) ->for proper reporting of frequency being scanned at the time*/
    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.ScanInfo.NextFrequency;

#ifdef SATTASK_SYMOK
    /* Check that the symbol rate is supported */
    if ( ((Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate >= Inst[Handle].Capability.SymbolMin) &&
         (Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate <= Inst[Handle].Capability.SymbolMax)) || (Inst[Handle].Sat.BlindScan))
       {
#endif
        /* Mask with the capabilties of the device */
        if(!Inst[Handle].Sat.BlindScan)
        {
        	Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation &= Inst[Handle].Capability.Modulation;   
        }
       
       /* Mask with current polarization mask */
       #ifdef STTUNER_DRV_SAT_SCR
        if ((!Inst[Handle].Capability.SCREnable) || (Inst[Handle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting != STTUNER_VIA_SCRENABLED_LNB))
        {  
        	Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization &= Inst[Handle].Sat.ScanInfo.PlrMask;
        }
        
       #else
       /*Mask with current polarization mask */
       Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization &= Inst[Handle].Sat.ScanInfo.PlrMask;
       #endif
       
       
        /* Check the LNB polarization is the current */
        if (Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization != 0)
        {
             /* Proceed only if it is a valid band */
            if (Inst[Handle].CurrentTunerInfo.ScanInfo.Band < Inst[Handle].BandList.NumElements)
            {

                /* Set requested band */
                index = Inst[Handle].CurrentTunerInfo.ScanInfo.Band;
                Band  = &Inst[Handle].BandList.BandList[index];

                /* Ensure current frequency falls inside selected band */
              if ((Inst[Handle].Sat.ScanInfo.NextFrequency >= Band->BandStart) &&
                     (Inst[Handle].Sat.ScanInfo.NextFrequency <= Band->BandEnd))
                {
                	/* Calc the downconverted frequency used by the auto scan routine 

                       Ku band (Europe):  Tuner PLL = Transponder freq  -  LO freq
                       C  band            Tuner PLL =          LO freq  -  Transponder freq  
                    */

                    Inst[Handle].Sat.ScanInfo.FrequencyLO = Band->LO;

                    if (Band->DownLink == STTUNER_DOWNLINK_Ku)
                    {
						#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s using Ku band\n", identity));
						#endif
                        ScanFrequency = Inst[Handle].Sat.ScanInfo.NextFrequency - Inst[Handle].Sat.ScanInfo.FrequencyLO;
                    }
                    else if (Band->DownLink == STTUNER_DOWNLINK_C)
                    {
						#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s using C band\n", identity));
						#endif
                        ScanFrequency = Inst[Handle].Sat.ScanInfo.FrequencyLO   - Inst[Handle].Sat.ScanInfo.NextFrequency;
                    }
                    else
                    {
						#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s fail, no such band\n", identity));
						#endif
                        return(ST_ERROR_BAD_PARAMETER);
                    }                

                    Inst[Handle].Sat.ScanInfo.DownLink = Band->DownLink;    /* save band info for tracking */

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                    STTBX_Print(("DEMOD_ScanFrequency() with F=%u IF=%u.\n", Inst[Handle].Sat.ScanInfo.NextFrequency, ScanFrequency));
#endif
		#ifdef STTUNER_DRV_SAT_SCR
		if((Inst[Handle].Capability.SCREnable == FALSE) || (Inst[Handle].CurrentTunerInfo.ScanInfo.LNB_SignalRouting != STTUNER_VIA_SCRENABLED_LNB))
    	{
    		    LnbConfig.TopLevelHandle = Handle;
                    LnbConfig.Polarization = Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization;   /* Setup the Satellite Dish LNB Polarization */
                    if(Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization == STTUNER_LNB_OFF)
                    {
                    	LnbConfig.Status       = LNB_STATUS_OFF;                                         /* Set the Satellite Dish LNB power to OFF */
                    }
                    else 
                    {
                    	LnbConfig.Status       = LNB_STATUS_ON;  
                    }
                    
                    LnbConfig.ToneState    = Inst[Handle].CurrentTunerInfo.ScanInfo.LNBToneState;   /* Setup the require tone state */
                    
                    /* Invoke new configuration */
                    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
                    if (Error != ST_NO_ERROR)
                    {
						#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
						                        STTBX_Print(("%s fail set lnb\n", identity));
						#endif
                        return(Error);
                    }
		 }
		 else
		 {
		 	LnbConfig.Polarization = Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization;   /* Setup the Satellite Dish LNB Polarization */
            LnbConfig.ToneState    = Inst[Handle].CurrentTunerInfo.ScanInfo.LNBToneState;  
         }
		 #else
		    		LnbConfig.TopLevelHandle = Handle;
                    LnbConfig.Polarization = Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization;   /* Setup the Satellite Dish LNB Polarization */
                    if(Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization == STTUNER_LNB_OFF)
                    {
                    	LnbConfig.Status       = LNB_STATUS_OFF;                                         /* Set the Satellite Dish LNB power to OFF */
                    }
                    else 
                    {
                    	LnbConfig.Status       = LNB_STATUS_ON;  
                    }
                    
                    LnbConfig.ToneState    = Inst[Handle].CurrentTunerInfo.ScanInfo.LNBToneState;   /* Setup the require tone state */
                    
                    /* Invoke new configuration */
                    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
                    if (Error != ST_NO_ERROR)
                    {
						#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
						                        STTBX_Print(("%s fail set lnb\n", identity));
						#endif
						return(Error);
                    }
		#endif
         
	
                    /* Set required FEC rates - according to device capability */
                        if(!Inst[Handle].Sat.BlindScan)
	                    {
	                           Fec = Inst[Handle].CurrentTunerInfo.ScanInfo.FECRates & Inst[Handle].Capability.FECRates;
	                           Error = (Inst[Handle].Sat.Demod.Driver->demod_SetFECRates)(Inst[Handle].Sat.Demod.DrvHandle, Fec/*Rate*/);
	                           Mod = Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation;
	                           Error|= (Inst[Handle].Sat.Demod.Driver->demod_SetModulation)(Inst[Handle].Sat.Demod.DrvHandle, Mod);	                            
					    }
					    else if(Inst[Handle].Sat.BlindScan)
		                {
		                	Error = (Inst[Handle].Sat.Demod.Driver->demod_SetFECRates)(Inst[Handle].Sat.Demod.DrvHandle, STTUNER_FEC_ALL);
		                }
		                if (Error != ST_NO_ERROR)
                        {
							#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
			                STTBX_Print(("%s fail set FEC rates\n", identity));
							#endif
                            return(Error);  /* if this fails then FEC selected not supported by the driver */
                        }		    

		    
                    /* Perform auto search algorithm */
                            Error = (Inst[Handle].Sat.Demod.Driver->demod_ScanFrequency)( 
                                                Inst[Handle].Sat.Demod.DrvHandle,  
                                                ScanFrequency,
                                                Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,
                                                (Inst[Handle].Sat.SymbolWidthMin * 1000) / 2,
                                                SATTASK_TUNER_STEP,  SATTASK_DEMOD_DEROTATOR_STEP,
                                                &ScanSuccess,  &NewFrequency,0,0,0,0,
                                                (U32) Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode,
                                                0,0,0);
                            if (Error != ST_NO_ERROR)
                            {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                                STTBX_Print(("%s fail set demod\n", identity));
#endif
                                return(Error);
                             }
                             
                /**********************************************************************************/    
                    
                    if ( ScanSuccess )
                    {
                        /* Polarization */
                        Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization = LnbConfig.Polarization;

                        /*          Compute actual frequency

                           Ku band (Europe):  Transponder freq = Tuner PLL + LO freq
                           C  band            Transponder freq = LO freq   - Tuner PLL
                        */
                        if (Inst[Handle].Sat.ScanInfo.DownLink == STTUNER_DOWNLINK_Ku)
                            Inst[Handle].Sat.ScanInfo.NextFrequency = NewFrequency + Inst[Handle].Sat.ScanInfo.FrequencyLO;
                        else
                            Inst[Handle].Sat.ScanInfo.NextFrequency = Inst[Handle].Sat.ScanInfo.FrequencyLO - NewFrequency;

                        Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.ScanInfo.NextFrequency;

                        SATTASK_GetTunerInfo(Handle);

                        /* Update status and lock count ready for tracking */
                        *Event = STTUNER_EV_LOCKED;
                    }
                    else /*if ( ScanSuccess )*/
                    {
                    	/*Check LNB OTF and OLF bits */
if((Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNB21) || (Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH21) || (Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH24) || (Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH23))
{
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
 			            Error = (Inst[Handle].Sat.Lnb.Driver->lnb_overloadcheck)(Inst[Handle].Sat.Lnb.DrvHandle,&IsOverTemp,&IsCurrentOvrLoad);
 			            if (Error == ST_NO_ERROR)
 			            {
 				            if(IsOverTemp ||IsCurrentOvrLoad )
 				            {
 				                *Event = STTUNER_EV_LNB_FAILURE;
 				            }
 			            }
#endif 			            
}
                    } /* else */
               }
            }
            else
            {
                /* Invalid band specified */
                Error = ST_ERROR_BAD_PARAMETER;
            }
            
            }
#ifdef SATTASK_SYMOK
    }
#endif

#endif

#ifdef STTUNER_MINIDRIVER

     Inst = STTUNER_GetDrvInst();
    *Event = STTUNER_EV_NO_OPERATION;   /* Assume no lock is found */
 /* Setup the current element -- this represents a working copy we can
     modify of the current element in the scan list -- on completion it
     will be made visible to the ST TUNER API.*/
    Inst[Handle].CurrentTunerInfo.ScanInfo = Inst[Handle].Sat.SingleScan;
    
    /*Adding following line for GNBvd28574(Useage of STTUNER_GetTunerInfo) ->for proper reporting of frequency being scanned at the time*/
    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.SingleScan.Frequency;

#ifdef SATTASK_SYMOK
    /* Check that the symbol rate is supported */
    if ( (Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate >= Inst[Handle].Sat.Capability.SymbolMin) &&
         (Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate <= Inst[Handle].Sat.Capability.SymbolMax) )
    {
#endif
                /* Check the LNB polarization is the current */
        if (Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization != 0)
        {
            /* Proceed only if it is a valid band */
            if (Inst[Handle].CurrentTunerInfo.ScanInfo.Band < Inst[Handle].BandList.NumElements)
            {

                /* Set requested band */
                index = Inst[Handle].CurrentTunerInfo.ScanInfo.Band;
                Band  = &Inst[Handle].BandList.BandList[index];

                /* Ensure current frequency falls inside selected band */
              if ( (Inst[Handle].Sat.SingleScan.Frequency >= Band->BandStart) &&
                     (Inst[Handle].Sat.SingleScan.Frequency <= Band->BandEnd) )
                {
                	/* Calc the downconverted frequency used by the auto scan routine 

                       Ku band (Europe):  Tuner PLL = Transponder freq  -  LO freq
                       C  band            Tuner PLL =          LO freq  -  Transponder freq  
                    */

                    Inst[Handle].Sat.SingleScan.FrequencyLO = Band->LO;

                    if (Band->DownLink == STTUNER_DOWNLINK_Ku)
                    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s using Ku band\n", identity));
#endif
                        ScanFrequency = Inst[Handle].Sat.SingleScan.Frequency - Inst[Handle].Sat.SingleScan.FrequencyLO;
                    }
                    else if (Band->DownLink == STTUNER_DOWNLINK_C)
                    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s using C band\n", identity));
#endif
                        ScanFrequency = Inst[Handle].Sat.SingleScan.FrequencyLO   - Inst[Handle].Sat.SingleScan.Frequency;
                    }
                    else
                    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s fail, no such band\n", identity));
#endif
                        return(ST_ERROR_BAD_PARAMETER);
                    }                

                    Inst[Handle].Sat.SingleScan.DownLink = Band->DownLink;    /* save band info for tracking */

#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                    STTBX_Print(("DEMOD_ScanFrequency() with F=%u IF=%u.\n", Inst[Handle].Sat.ScanInfo.NextFrequency, ScanFrequency));
#endif
		
                    LnbConfig.TopLevelHandle = Handle;
                    LnbConfig.Polarization = Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization;   /* Setup the Satellite Dish LNB Polarization */
                    if(Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization == STTUNER_LNB_OFF)
                    {
                    	LnbConfig.Status       = LNB_STATUS_OFF;                                         /* Set the Satellite Dish LNB power to OFF */
                    }
                    else 
                    {
                    	LnbConfig.Status       = LNB_STATUS_ON;  
                    }
                    
                    LnbConfig.ToneState    = Inst[Handle].CurrentTunerInfo.ScanInfo.LNBToneState;   /* Setup the require tone state */
                    
                    /* Invoke new configuration */
                    #ifdef STTUNER_DRV_SAT_STV0299
	                    #ifdef STTUNER_DRV_SAT_LNB21
	                    Error = lnb_lnb21_SetConfig(I&LnbConfig);
	                    #elif defined STTUNER_DRV_SAT_LNBH21
	                    Error = lnb_lnbh21_SetConfig(&LnbConfig);
	                    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	                    Error = lnb_lnbh24_SetConfig(&LnbConfig);
	                    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	                    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	                    #endif
	             #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
	             	    #ifdef STTUNER_DRV_SAT_LNB21
	                    Error = lnb_lnb21_SetConfig(&LnbConfig);
	                    #elif defined STTUNER_DRV_SAT_LNBH21
	                    Error = lnb_lnbh21_SetConfig(&LnbConfig);
	                    #elif defined (STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
	                    Error = lnb_lnbh24_SetConfig(&LnbConfig);
	                    #elif !defined(STTUNER_LNB_POWER_THRU_BACKENDGPIO)
	                    Error = lnb_lnb_demodIO_SetConfig(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
	                    #endif
	             #endif
                    
                    if (Error != ST_NO_ERROR)
                    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                        STTBX_Print(("%s fail set lnb\n", identity));
#endif
    return(Error);
                    }                   
                    /* Perform auto search algorithm */
                    #ifdef STTUNER_DRV_SAT_STV0299
                            Error = demod_d0299_ScanFrequency( 
                                                Inst[Handle].Sat.Demod.DrvHandle,  
                                                ScanFrequency,
                                                Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,
                                                (Inst[Handle].Sat.SymbolWidthMin * 1000) / 2,
                                                SATTASK_TUNER_STEP,  SATTASK_DEMOD_DEROTATOR_STEP,
                                                &ScanSuccess,  &NewFrequency,0,0,0,0,
                                                (U32) Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode,
                                                0,0,0);
                    #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
                            Error = demod_d0399_ScanFrequency( 
                                                Inst[Handle].Sat.Demod.DrvHandle,  
                                                ScanFrequency,
                                                Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,
                                                (Inst[Handle].Sat.SymbolWidthMin * 1000) / 2,
                                                SATTASK_TUNER_STEP,  SATTASK_DEMOD_DEROTATOR_STEP,
                                                &ScanSuccess,  &NewFrequency,0,0,0,0,
                                                (U32) Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode,
                                                0,0,0);
                    #endif
                            if (Error != ST_NO_ERROR)
                            {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                                STTBX_Print(("%s fail set demod\n", identity));
#endif
                                return(Error);
                             }
                             
                /**********************************************************************************/    
                    
                    if ( ScanSuccess )
                    {
                        /* Polarization */
                        Inst[Handle].CurrentTunerInfo.ScanInfo.Polarization = LnbConfig.Polarization;

                        /*          Compute actual frequency

                           Ku band (Europe):  Transponder freq = Tuner PLL + LO freq
                           C  band            Transponder freq = LO freq   - Tuner PLL
                        */
                        if (Inst[Handle].Sat.SingleScan.DownLink == STTUNER_DOWNLINK_Ku)
                            Inst[Handle].Sat.SingleScan.Frequency = NewFrequency + Inst[Handle].Sat.SingleScan.FrequencyLO;
                        else
                            Inst[Handle].Sat.SingleScan.Frequency = Inst[Handle].Sat.SingleScan.FrequencyLO - NewFrequency;

                        Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.SingleScan.Frequency;

                        /* Extract remaining tuner info from h/w */
                        Error = SATTASK_GetTunerInfo(Handle);
                        if (Error != ST_NO_ERROR)
                        {
                            return(Error);
                        }

                        /* Update status and lock count ready for tracking */
                        *Event = STTUNER_EV_LOCKED;
                    }
                    else /*if ( ScanSuccess )*/
                    {
                    	/*Check LNB OTF and OLF bits */
                        #if defined(STTUNER_DRV_SAT_LNBH21) 
 			Error = lnb_lnbh21_overloadcheck(&IsOverTemp,&IsCurrentOvrLoad);
 			if (Error == ST_NO_ERROR)
 			{
 				if(IsOverTemp ||IsCurrentOvrLoad )
 				{
 					*Event = STTUNER_EV_LNB_FAILURE; /* This is to be used in later major release of STTUNER*/
 				}
 			}
 			#elif defined(STTUNER_DRV_SAT_LNB21)      
 			Error = lnb_lnb21_overloadcheck(&IsOverTemp,&IsCurrentOvrLoad);      	
 			if (Error == ST_NO_ERROR)
 			{
 				if(IsOverTemp ||IsCurrentOvrLoad )
 				{
 					*Event = STTUNER_EV_LNB_FAILURE; /* This is to be used in later major release of STTUNER*/
 				}
 			}
 			#elif defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)      
 			Error = lnb_lnbh24_overloadcheck(&IsOverTemp,&IsCurrentOvrLoad);      	
 			if (Error == ST_NO_ERROR)
 			{
 				if(IsOverTemp ||IsCurrentOvrLoad )
 				{
 					*Event = STTUNER_EV_LNB_FAILURE; /* This is to be used in later major release of STTUNER*/
 				}
 			}
 			#endif
 			
                    } /* else */
               }
            }
            else
            {
                /* Invalid band specified */
                Error = ST_ERROR_BAD_PARAMETER;
            }
        }
        
	#ifdef SATTASK_SYMOK
	    }
	#endif
#endif

    return(Error);
}



/*----------------------------------------------------------------------------
Name: SATTASK_ProcessTracking()
Description:
    This routine calls the DEMOD device in order to perform derotator
    centring or tracking.  This process ensures we maintain a tuner lock
    for the currently tuned frequency.
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
ST_ErrorCode_t SATTASK_ProcessTracking(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ProcessTracking()";
#endif
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STTUNER_EventType_t ThisEvent = STTUNER_EV_NO_OPERATION;
    STTUNER_InstanceDbase_t *Inst;
    BOOL IsLocked, IsSignal;
#if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)    
    BOOL  IsOverTemp, IsCurrentOvrLoad;
#endif    
    U32 IF;

    Inst = STTUNER_GetDrvInst();
#ifndef STTUNER_MINIDRIVER

    Error = (Inst[Handle].Sat.Demod.Driver->demod_Tracking)(Inst[Handle].Sat.Demod.DrvHandle, FALSE, &IF, &IsSignal);

    if (Error == ST_NO_ERROR)
    {
        /* The tracking function returns the intermediate frequency (IF).

           Ku band (Europe):  Transponder freq = Tuner PLL + LO freq
           C  band            Transponder freq = LO freq   - Tuner PLL
        */
       
        if (Inst[Handle].Sat.ScanInfo.DownLink == STTUNER_DOWNLINK_Ku)
        {
            Inst[Handle].CurrentTunerInfo.Frequency = IF + Inst[Handle].Sat.ScanInfo.FrequencyLO;
        }
        else if (Inst[Handle].Sat.ScanInfo.DownLink == STTUNER_DOWNLINK_C)
        {
            Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.ScanInfo.FrequencyLO - IF;
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
        }
        
        if (Error == ST_NO_ERROR)
        {
            Error = (Inst[Handle].Sat.Demod.Driver->demod_IsLocked)(Inst[Handle].Sat.Demod.DrvHandle, &IsLocked);
        }    


        if (Error == ST_NO_ERROR)
        {
            if (IsLocked && IsSignal)
            {
                Inst[Handle].Sat.ScanInfo.LockCount = NUMBER_LOST_BEFORE_UNLOCK;
            }
            else
            {
				if((Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNB21) || (Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH21) || (Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH24) || (Inst[Handle].Sat.Lnb.Driver->ID == STTUNER_LNB_LNBH23))
				{
				    #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
			            	/*Check LNB OTF and OLF bits */
		 		    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_overloadcheck)(Inst[Handle].Sat.Lnb.DrvHandle,&IsOverTemp,&IsCurrentOvrLoad);            	
		 		    if (Error == ST_NO_ERROR)
		 		    {
		 			    if(IsOverTemp ||IsCurrentOvrLoad )
		 			    {
		 				    ThisEvent = STTUNER_EV_LNB_FAILURE;  /* To be used in later major release of STTUNER  */ 				
		 			    }
		 		    }
				    #endif 		    
				}
 		 		
                --(Inst[Handle].Sat.ScanInfo.LockCount);
                
            }
			
			if (Inst[Handle].Sat.ScanInfo.LockCount == 0)
            {
                ThisEvent = STTUNER_EV_UNLOCKED;
            }
            else
            {
	        	
            }
        }
    }

    Inst[Handle].TunerInfoError = Error;
    if (Error != ST_NO_ERROR)
    {
        ThisEvent = STTUNER_EV_UNLOCKED;
    }

    *Event = ThisEvent;
#endif
#ifdef STTUNER_MINIDRIVER
    /*Error = demod_d0299_Tracking(Inst[Handle].Sat.Demod.DrvHandle, FALSE, &IF, &IsSignal);*/
    
    if (Error == ST_NO_ERROR)
    {
        /* The tracking function returns the intermediate frequency (IF).

           Ku band (Europe):  Transponder freq = Tuner PLL + LO freq
           C  band            Transponder freq = LO freq   - Tuner PLL
        */
        if (Inst[Handle].Sat.SingleScan.DownLink == STTUNER_DOWNLINK_Ku)
        {
            Inst[Handle].CurrentTunerInfo.Frequency = IF + Inst[Handle].Sat.SingleScan.FrequencyLO;
        }
        else if (Inst[Handle].Sat.SingleScan.DownLink == STTUNER_DOWNLINK_C)
        {
            Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.SingleScan.FrequencyLO - IF;
        }
        else
        {
            Error = ST_ERROR_BAD_PARAMETER;
        }
        Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Sat.SingleScan.Frequency;
        if (Error == ST_NO_ERROR)
        {
            #ifdef STTUNER_DRV_SAT_STV0299
            Error = demod_d0299_IsLocked(Inst[Handle].Sat.Demod.DrvHandle, &IsLocked);
            #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)
            Error = demod_d0399_IsLocked(Inst[Handle].Sat.Demod.DrvHandle, &IsLocked);
            #endif
        }    


        if (Error == ST_NO_ERROR)
        {
            
            if (IsLocked && IsSignal)
            {
                Inst[Handle].Sat.ScanInfo.LockCount = NUMBER_LOST_BEFORE_UNLOCK;
            }
            else
            {
                #if defined(STTUNER_DRV_SAT_LNB21) || defined(STTUNER_DRV_SAT_LNBH21) || defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)
            	/*Check LNB OTF and OLF bits */
                        #if defined(STTUNER_DRV_SAT_LNBH21) 
 			Error = lnb_lnbh21_overloadcheck(&IsOverTemp,&IsCurrentOvrLoad);
 			#elif defined(STTUNER_DRV_SAT_LNB21)      
 			Error = lnb_lnb21_overloadcheck(&IsOverTemp,&IsCurrentOvrLoad); 
 			#elif defined(STTUNER_DRV_SAT_LNBH24) || defined (STTUNER_DRV_SAT_LNBH23)     
 			Error = lnb_lnbh24_overloadcheck(&IsOverTemp,&IsCurrentOvrLoad);     	
 			#endif
 			if (Error == ST_NO_ERROR)
 		        {
	 			if(IsOverTemp ||IsCurrentOvrLoad )
	 			{
	 				ThisEvent = STTUNER_EV_LNB_FAILURE;  /* To be used in later major release of STTUNER  */ 				
	 			}
 		        }
                #endif
 		 		
                Inst[Handle].Sat.ScanInfo.LockCount--;
            }
	    
	    if (IsLocked && IsSignal)
            {
                Error = SATTASK_GetTunerInfo(Handle);
            }
            
        }
    }

    Inst[Handle].TunerInfoError = Error;
    if (Error != ST_NO_ERROR)
    {
        ThisEvent = STTUNER_EV_UNLOCKED;
    }

    *Event = ThisEvent;
#endif
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* SATTASK_ProcessTracking() */



#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: SATTASK_ProcessThresholds()
Description:
Parameters:
    Tuner_p,    pointer to the tuner control block.
    Event_p,    pointer to area to store event:

    STTUNER_EV_NO_OPERATION,    no event has occurred i.e. lock is ok.
    STTUNER_EV_SIGNAL_CHANGE,   the signal quality has changed threshold.

Return Value:
    TUNER_NO_ERROR,     the operation completed without error.
    I2C_xxxx,           there was a problem accessing a device.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_ProcessThresholds(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ProcessThresholds()";
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

    LastSignal = Inst[Handle].Sat.LastSignal;

    if ( (CurrentSignal == LastSignal) ||  (Inst[Handle].ThresholdList.NumElements == 0) )
    {
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
        STTBX_Print(("%s info: no change in signal\n", identity));
#endif
        return(Error); /* No need to proceed -- no change in signal */
    }

    /* Re-set last signal indicator */
    Inst[Handle].Sat.LastSignal = CurrentSignal;

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
        for (index = 0; index < HitCount && ThresholdHits[index] != Inst[Handle].Sat.LastThreshold; index++); /* Do nothing */

        /* Check for all thresholds not equal to last threshold */
        if (index == HitCount)
        {
            /* Threshold has changed, we should ensure an event is generated */
            *Event = STTUNER_EV_SIGNAL_CHANGE;
            Inst[Handle].Sat.LastThreshold = ThresholdHits[0];
        }

    }
    else
    {
        /* The hit count is zero -- no thresholds hold the current
         * signal value.  An event must be generated unless this was
         * the case last time we checked.
         */
        if (Inst[Handle].Sat.LastThreshold != ((U32)-1))
        {
            *Event = STTUNER_EV_SIGNAL_CHANGE;
            Inst[Handle].Sat.LastThreshold = ((U32)-1);
        }
    }

    return(Error);
}

#endif
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: SATTASK_ProcessNextScan()
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
ST_ErrorCode_t SATTASK_ProcessNextScan(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_ProcessNextScan()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 FrequencyEnd, FrequencyStart;
    int FrequencyGap ;

    Inst = STTUNER_GetDrvInst();


    /* Check to see if we are at the end of the scan list */
    if ((Inst[Handle].Sat.ScanInfo.ScanIndex >= Inst[Handle].ScanList.NumElements) || (Inst[Handle].Sat.BlindScan))
    {
        /* Calculate correct start and end frequencies */
        if (Inst[Handle].Sat.ScanInfo.ScanDirection < 0)
        {
            FrequencyEnd   = Inst[Handle].Sat.ScanInfo.FrequencyStart;
            FrequencyStart = Inst[Handle].Sat.ScanInfo.FrequencyEnd;
        }
        else
        {
            FrequencyEnd   = Inst[Handle].Sat.ScanInfo.FrequencyEnd;
            FrequencyStart = Inst[Handle].Sat.ScanInfo.FrequencyStart;
        }

              
        /* Apply next frequency step */
        Inst[Handle].Sat.ScanInfo.NextFrequency +=
            (Inst[Handle].Sat.ScanInfo.ScanDirection * (S32)Inst[Handle].Sat.ScanInfo.FrequencyStep);

        /* Set frequency step to default - in case we just had a large step */
        if (Inst[Handle].Sat.Ioctl.TunerStepSize == 0) 
        {
            Inst[Handle].Sat.ScanInfo.FrequencyStep = (Inst[Handle].Sat.SymbolWidthMin); /* Removed /2 factor to freq step to avoid repeated search in second half of the search range */
        }
        else
        {
            Inst[Handle].Sat.ScanInfo.FrequencyStep = Inst[Handle].Sat.Ioctl.TunerStepSize;
        }

        
	      
	        if(Inst[Handle].Sat.ScanInfo.ScanDirection > 0)
	        {
	        	FrequencyGap = FrequencyEnd - Inst[Handle].Sat.ScanInfo.NextFrequency;
	        	if(FrequencyGap < 0)
	        	{
	        		if((FrequencyGap*(-1))<Inst[Handle].Sat.ScanInfo.FrequencyStep/2)
	        		{
	        			Inst[Handle].Sat.ScanInfo.NextFrequency +=
		            		(Inst[Handle].Sat.ScanInfo.ScanDirection * (S32)FrequencyGap);
		            	}
		            	
	            	}
	        }
	        
	        else
	        {
	        	FrequencyGap = Inst[Handle].Sat.ScanInfo.NextFrequency - FrequencyStart;
	        
	        	if(FrequencyGap < 0)
	        	{
	        		if((FrequencyGap*(-1))<Inst[Handle].Sat.ScanInfo.FrequencyStep/2)
	        		{
	        			Inst[Handle].Sat.ScanInfo.NextFrequency +=
		            		(Inst[Handle].Sat.ScanInfo.ScanDirection * (S32)FrequencyGap);
		            	}
	            	}
	        }
	        
       
        /* If we've reached the limit of our frequency range, then we should try
         * and use the next polarization setting -- if there is one.
         */
        if ( (Inst[Handle].Sat.ScanInfo.NextFrequency > FrequencyEnd)   ||
             (Inst[Handle].Sat.ScanInfo.NextFrequency < FrequencyStart) )
        {
            /* Shift the polarization mask along to the next valid polarization */
            do
            {
                Inst[Handle].Sat.ScanInfo.PlrMask <<= 1;
                
            }
            while ((Inst[Handle].Sat.ScanInfo.PlrMask != 0 &&
               (Inst[Handle].Sat.ScanInfo.PlrMask & Inst[Handle].Sat.PolarizationMask) == 0) &&
               (Inst[Handle].Sat.ScanInfo.PlrMask < 8));
            
            /* Check there are still polarizations to check */
            if ((((Inst[Handle].Sat.ScanInfo.PlrMask != 0) && (Inst[Handle].Sat.ScanInfo.PlrMask != 8) ) || (Inst[Handle].Sat.PolarizationMask < 8) )&& (Inst[Handle].Sat.ScanInfo.PlrMask < 8))
            {
                Inst[Handle].Sat.ScanInfo.NextFrequency = Inst[Handle].Sat.ScanInfo.FrequencyStart;
            }
            else
            {
                /* No more combinations to try -- scan has failed */
                *Event = STTUNER_EV_SCAN_FAILED;
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
                 STTBX_Print(("%s STTUNER_EV_SCAN_FAILED\n", identity));
#endif
                return(Error);
            }

        }

        /* Set scan list to beginning */
        if(!Inst[Handle].Sat.BlindScan)
        {
	        Inst[Handle].Sat.ScanInfo.Scan = Inst[Handle].ScanList.ScanList;
	        Inst[Handle].Sat.ScanInfo.ScanIndex = 0;
	}
    }

    /* Process the next frequency scan step */
    Error = SATTASK_ProcessScanExact(Handle, Event);

    return(Error);
}
#endif


/*----------------------------------------------------------------------------
Name: SATTASK_GetTunerInfo()
Description:
    Calls the DEMOD device in order to get the current demodulation settings
    for updating the current tuner info.  This information will eventually
    be forwarded back up to the STTUNER API tuner info.
Parameters:

Return Value:
See Also:
----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_GetTunerInfo(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
    const char *identity = "STTUNER sattask.c SATTASK_GetTunerInfo()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    /* Get the current demodulation parameters */


#ifndef STTUNER_MINIDRIVER   
 
    Error  = (Inst[Handle].Sat.Demod.Driver->demod_GetAGC)(Inst[Handle].Sat.Demod.DrvHandle, 
                                                          &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);

    Error |= (Inst[Handle].Sat.Demod.Driver->demod_GetSignalQuality)(Inst[Handle].Sat.Demod.DrvHandle, 
                                                                    &Inst[Handle].CurrentTunerInfo.SignalQuality,
                                                                    &Inst[Handle].CurrentTunerInfo.BitErrorRate);

    Error |= (Inst[Handle].Sat.Demod.Driver->demod_GetModulation)(Inst[Handle].Sat.Demod.DrvHandle, 
                                                                 &Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation);
                                                                 
    
  
    Error |= (Inst[Handle].Sat.Demod.Driver->demod_GetIQMode)(Inst[Handle].Sat.Demod.DrvHandle, 
                                                          &Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode); /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/
    
    Error |= (Inst[Handle].Sat.Demod.Driver->demod_GetFECRates)(Inst[Handle].Sat.Demod.DrvHandle, 
                                                              &Inst[Handle].CurrentTunerInfo.ScanInfo.FECRates);


                   

    #ifdef STTUNER_DRV_SAT_STX0288
    if(Inst[Handle].Sat.Demod.Driver->ID == STTUNER_DEMOD_STX0288)
    {                                                     
    Error |= (Inst[Handle].Sat.Demod.Driver->demod_GetSymbolrate)(Inst[Handle].Sat.Demod.DrvHandle, 
  							&Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate); 
    }
    #endif
    #ifdef STTUNER_DRV_SAT_STB0899
    if(Inst[Handle].Sat.Demod.Driver->ID == STTUNER_DEMOD_STB0899)
    {
		#ifdef STTUNER_DRV_SAT_STB0899    	
    	Error |= (Inst[Handle].Sat.Demod.Driver->demod_GetModeCode)(Inst[Handle].Sat.Demod.DrvHandle, 
                                                              &Inst[Handle].CurrentTunerInfo.ScanInfo.ModeCode);
		#endif                                                              
    }
    #endif
                                            
#endif
#ifdef STTUNER_MINIDRIVER

    #ifdef STTUNER_DRV_SAT_STV0299
    Error = demod_d0299_GetSignalQuality(Inst[Handle].Sat.Demod.DrvHandle, 
                                                                    &Inst[Handle].CurrentTunerInfo.SignalQuality,
                                                                    &Inst[Handle].CurrentTunerInfo.BitErrorRate);
    #elif defined STTUNER_DRV_SAT_STV0399  || defined(STTUNER_DRV_SAT_STV0399E)                                                             
    Error = demod_d0399_GetSignalQuality(Inst[Handle].Sat.Demod.DrvHandle, 
                                                                    &Inst[Handle].CurrentTunerInfo.SignalQuality,
                                                                    &Inst[Handle].CurrentTunerInfo.BitErrorRate);
    #endif
    /*Error |= demod_d0299_GetIQMode(Inst[Handle].Sat.Demod.DrvHandle, 
                                                          &Inst[Handle].CurrentTunerInfo.ScanInfo.IQMode);*/ /*added for GNBvd26107->I2C failure due to direct access to demod device at API level*/


#endif
   


    if (Error != ST_NO_ERROR)
    {

	#ifdef STTUNER_DEBUG_MODULE_SAT_SATTASK
	        STTBX_Print(("%s fail get demod params, Error = %u\n", identity, Error));
	#endif
        return(Error);
    }

#ifndef ST_OSLINUX
    /* The following code will be ENABLED if you wish to introduce 'random noise' into the 
    generated signal quality values. This is useful for testing for signal threshold events */

    #if !defined(STTUNER_DRV_SAT_STX0288) && !defined(STTUNER_DRV_SAT_STB0899)
    if((Inst[Handle].Sat.Demod.Driver->ID != STTUNER_DEMOD_STX0288) && (Inst[Handle].Sat.Demod.Driver->ID != STTUNER_DEMOD_STB0899))
    {
    if(Inst[Handle].Sat.Ioctl.SignalNoise == TRUE)
    {
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
    }
    }
    #endif
  #endif
    return(Error);
} /* SATTASK_GetTunerInfo() */


#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
Name: SATTASK_LNB_PowerDown()

Description:
    This routine calls the LNB device in order to remove power to the LNB.

Parameters:

Return Value:

See Also:
----------------------------------------------------------------------------*/
ST_ErrorCode_t SATTASK_LNB_PowerDown(STTUNER_Handle_t Handle)
{
    ST_ErrorCode_t Error;
    LNB_Config_t LnbConfig;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    Error = (Inst[Handle].Sat.Lnb.Driver->lnb_GetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);

    /* Attempt to turn the LNB power off, as the scan has finished */
    if (Error == ST_NO_ERROR)
    {
        LnbConfig.Status = LNB_STATUS_OFF;
        Error = (Inst[Handle].Sat.Lnb.Driver->lnb_SetConfig)(Inst[Handle].Sat.Lnb.DrvHandle, &LnbConfig);
    }

    return(Error);

} /* SATTASK_LNB_PowerDown() */
#endif

/* End of sattask.c */
