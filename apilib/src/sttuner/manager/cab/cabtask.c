/*----------------------------------------------------------------------------
File Name: cabtask.c (was sattask.c)

Description: Wrapper API for interfacing with the low-level tuner modules.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
----------------------------------------------------------------------------*/


/* Includes --------------------------------------------------------------- */

#ifdef ST_OSLINUX
   #include "stos.h"
#else
/* C libs */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "stlite.h"
#include "sttbx.h"
#endif

/* STAPI */
#include "stcommon.h"

#include "stevt.h"
#include "sttuner.h"

/* local to sttuner */
#include "util.h"       /* generic utility functions for sttuner */
#include "dbtypes.h"    /* data types for databases */
#include "sysdbase.h"   /* functions to accesss system data */

#include "cabtask.h"    /* header for this file */

#include "stapitnr.h"

/* Private types/constants ------------------------------------------------ */

/* Digital bandwidth roll-off */
#define DIG_ROLL_OFF_SCALING            100
#define SCALED_DIG_ROLL_OFF             115 /* Actual 1.15 */

#define SCALED_FS_RATIO                 8   /* % of FS */

/* When AGC < AGC_LEVEL, Freq Step = Min Step else (SR*1.15/2) */
#define AGC_LEVEL_LO_SGL                400
#define AGC_LEVEL_HI_SGL                500

#define AGC_LEVEL_LO_NOSGL              800
#define AGC_LEVEL_HI_NOSGL              900

#define AGC_LEVEL_MIN                   AGC_LEVEL_LO_SGL
#define AGC_LEVEL_MAX                   AGC_LEVEL_HI_NOSGL

#define MIN_FREQ_STEP                   1000000   /* Freq Step Min */

#define MIN_FREQ_WIDTH                  4000000   /* Freq Width */

/* Maximum signal quality */
#define TEST_MAX_BER 200000

/* Analog carrier bandwidth -- big step speeds up scan process */
#define ANALOG_CARRIER_BANDWIDTH_KHZ    29000
#define NUMBER_LOST_BEFORE_UNLOCK       3
#define NUMBER_RETRY                    3


/* Delay required for the scan task */
#ifndef STTUNER_SCAN_TASK_DELAY_MS  /* [ */
#define STTUNER_SCAN_TASK_DELAY_MS       1800     /* millisecond delay (Take care for QAM64/256 <= 1.98s) */
#endif  /* ] STTUNER_SCAN_TASK_DELAY_MS */

#define UNDEF_TRES                      ((U32)-1)
/* local functions  -------------------------------------------------------- */


void CABTASK_ScanTask(STTUNER_Handle_t *Handle);

ST_ErrorCode_t CABTASK_ProcessScanExact (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t CABTASK_ProcessTracking  (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t CABTASK_ProcessThresholds(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);
ST_ErrorCode_t CABTASK_ProcessNextScan  (STTUNER_Handle_t Handle, STTUNER_EventType_t *Event);

ST_ErrorCode_t CABTASK_GetTunerInfo     (STTUNER_Handle_t Handle);

/*----------------------------------------------------------------------------
Name: CABTASK_Open()

Description:
    After open.c initialized each tuner component and then stored each
    device's capabilities this fn starts up the scan task for an instance.

Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t CABTASK_Open(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_Open()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    int result;
   
    Inst = STTUNER_GetDrvInst();

    strcpy(Inst[Handle].Cable.ScanTask.TaskName, Inst[Handle].Name);


#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s this TaskName [%s]\n", identity, Inst[Handle].Cable.ScanTask.TaskName));
#endif

    /* Create the scan busy semaphore required to synchronize access to the scan status, etc. */
Inst[Handle].Cable.ScanTask.GuardSemaphore =  STOS_SemaphoreCreateFifo(NULL,1);



    /* Create the timeout semaphore, this allows us to periodically wakeup the scan task to
      perform background processing e.g. tracking the locked frequency. */
      Inst[Handle].Cable.ScanTask.TimeoutSemaphore = STOS_SemaphoreCreateFifoTimeOut(NULL,0);



    /* Reset the delete flag to ensure that the task doesn't exit prematurely */
    Inst[Handle].Cable.ScanTask.DeleteTask = FALSE;

    /* Sets the scan task timer delay, this controls how often the scan task awakes to perform background processing */
    Inst[Handle].Cable.ScanTask.TimeoutDelayMs = STTUNER_SCAN_TASK_DELAY_MS;

    /* Initialize the scan task status to startup defaults */
    Inst[Handle].Cable.ScanInfo.Scan           = NULL;
    Inst[Handle].Cable.ScanInfo.ScanIndex      = 0;
    Inst[Handle].Cable.ScanInfo.NextFrequency  = 0;
    Inst[Handle].Cable.ScanInfo.LockCount      = 0;    /* For monitoring tracking */
    Inst[Handle].Cable.ScanTask.TopLevelHandle = Handle;
    Inst[Handle].Cable.EnableReLocking = FALSE;

    /* Assume that the scan status is "unlocked" to begin with */
    Inst[Handle].TunerInfo.Status = STTUNER_STATUS_IDLE;
    Inst[Handle].PreStatus = STTUNER_STATUS_IDLE;
    Inst[Handle].PowerMode = STTUNER_NORMAL_POWER_MODE;
    Inst[Handle].Cable.EnableReLocking = FALSE;
   
    result = STOS_TaskCreate( (void(*)(void *))CABTASK_ScanTask,              /* task function      */
                         &Inst[Handle].Cable.ScanTask.TopLevelHandle,    /* user params        */
                         Inst[Handle].DriverPartition,				/* stack partition    */
                         CAB_SCANTASK_STACK_SIZE,                        /* stack size         */
                         (void*)&Inst[Handle].Cable.ScanTask.ScanTaskStack,     /* stack              */
                         Inst[Handle].DriverPartition,					/* task partiton       */
                         &Inst[Handle].Cable.ScanTask.Task,              /* task control block */
                         &Inst[Handle].Cable.ScanTask.TaskDescriptor,    /* task descriptor    */
                         STTUNER_SCAN_CAB_TASK_PRIORITY,                 /* high priority, prevents stevt notify blocking */ /*for GNBvd25440 --> Allow override of task priorities*/
                         Inst[Handle].Cable.ScanTask.TaskName,           /* task name          */
                         0                                               /* flags              */
                      );

    /* Task creation failed? */
    if (result != 0)
    {
        Error = ST_ERROR_NO_MEMORY;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s fail\n", identity));
#endif
    }

    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s task started ok\n", identity));
#endif
    }

    return(Error);

} /* CABTASK_Open() */



/*----------------------------------------------------------------------------
Name: CABTASK_Close()
Description:

Parameters:

Return Value:

See Also:
    CABTASK_Open()
----------------------------------------------------------------------------*/
ST_ErrorCode_t CABTASK_Close(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_Close()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    task_t *task;

    Error = CABTASK_ScanAbort(Handle);
    if (Error != ST_NO_ERROR)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s fail CABTASK_ScanAbort()\n", identity));
#endif
        return(Error);
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s scan aborted ok\n", identity));
#endif
    }

    Inst = STTUNER_GetDrvInst();

    task = Inst[Handle].Cable.ScanTask.Task;
    /* wait until scantask is out of its critical section then grab semaphore to keep it waiting */
     STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);

    /* Wakeup the scan task (fallthrough early) */
    STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.TimeoutSemaphore);

    /* modify the delete task flag as waiting to be deleted */
    Inst[Handle].Cable.ScanTask.DeleteTask = TRUE;

    /* allow scantask to procede */
    STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s instance %d waiting for task [%s] to end...", identity, Handle, Inst[Handle].Cable.ScanTask.TaskName));
#endif
    /* The task should be in the process of returning, but wait for it first before deleting it. */
    if (STOS_TaskWait(&task,TIMEOUT_INFINITY) == 0)
    {
        /* Now it should be safe to delete the task */

        if (STOS_TaskDelete(Inst[Handle].Cable.ScanTask.Task,Inst[Handle].DriverPartition,Inst[Handle].Cable.ScanTask.ScanTaskStack,Inst[Handle].DriverPartition) == 0)

        {
        	STOS_SemaphoreDelete(NULL,Inst[Handle].Cable.ScanTask.GuardSemaphore);
                STOS_SemaphoreDelete(NULL,Inst[Handle].Cable.ScanTask.TimeoutSemaphore);


#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("ok\n"));
#endif
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("fail task_delete()\n"));
#endif
            Error = ST_ERROR_DEVICE_BUSY;   /* task may not be deleted */
        }
        
        
        
        
        
    }


    return(Error);

} /* CABTASK_Close() */



/*----------------------------------------------------------------------------
Name: CABTASK_ScanAbort()

Description:
    This routine will abort the current scan step.

    NOTE:   This call will block until it is able to safely abort the
            current scan operation -- a callback may be invoked if the
            caller has set one up.
Parameters:

Return Value:

See Also:

----------------------------------------------------------------------------*/
ST_ErrorCode_t CABTASK_ScanAbort(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ScanAbort()";
    clock_t                 time_start, time_end;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
    time_start = STOS_time_now();
#endif


    /* Needs to go before the semaphore as it is the ultimate termination */
    Inst[Handle].ForceSearchTerminate = TRUE;

    /* Lock out the scan task whilst we modify the status code */
     STOS_SemaphoreWait(Inst[Handle].Cable.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    time_end = STOS_time_now();
    STTBX_Print(("(%u ticks)(%u micro-s)",
                STOS_time_minus(time_end, time_start),
                ((STOS_time_minus(time_end, time_start))*1000000)/ST_GetClocksPerSecond()
                ));
    STTBX_Print(("ok\n"));
#endif

    /* Forcibly unlock the tuner  */
    Inst[Handle].TunerInfo.Status    = STTUNER_STATUS_UNLOCKED;
    Inst[Handle].Cable.EnableReLocking = FALSE;

    /* Release semaphore -- allows scan task to resume */
    STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);


    return(Error);

} /* CABTASK_ScanAbort() */



/*----------------------------------------------------------------------------
Name: CABTASK_ScanStart()

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
ST_ErrorCode_t CABTASK_ScanStart(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ScanStart()";
    clock_t                 time_start, time_end;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 SymbolMin, loop;


    Inst = STTUNER_GetDrvInst();

    /* if the band list is not setup then the scan is not valid so exit */
    if (Inst[Handle].BandList.NumElements < 1)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s fail STTUNER_SetBandList() not done?\n", identity));
#endif
        return(ST_ERROR_BAD_PARAMETER);
    }
   /* if the scan list is not setup then the scan is not valid so exit */
    if (!Inst[Handle].Cable.ScanExact)
    {
	    if (Inst[Handle].ScanList.NumElements < 1)
	    {
    #ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
	        STTBX_Print(("%s fail STTUNER_SetScanList() not done?\n", identity));
	#endif
	        return(ST_ERROR_BAD_PARAMETER);
	    }
    }

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
    time_start = STOS_time_now();
#endif
 STOS_SemaphoreWait(  Inst[Handle].Cable.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    time_end = STOS_time_now();
    STTBX_Print(("(%u ticks)(%u micro-s)",
                STOS_time_minus(time_end, time_start),
                ((STOS_time_minus(time_end, time_start))*1000000)/ST_GetClocksPerSecond()
                ));
    STTBX_Print(("ok\n"));
#endif
 STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.TimeoutSemaphore);

	if(Inst[Handle].PowerMode == STTUNER_STANDBY_POWER_MODE) 
    {	
        Inst[Handle].TunerInfo.Status = STTUNER_STATUS_STANDBY;
    }
    else
    {
    /* Set the scan status and reset the scan element pointer */
    Inst[Handle].TunerInfo.Status = STTUNER_STATUS_SCANNING;
    Inst[Handle].Cable.EnableReLocking = FALSE;   /* only relock on first single scan with a valid frequency */
    Inst[Handle].ForceSearchTerminate = FALSE;
	}

    /* First calculate the minimum symbol rate for the scan */
    SymbolMin = ((U32)~0);

    for (loop = 0; loop < Inst[Handle].ScanList.NumElements; loop++)
    {
        if (Inst[Handle].ScanList.ScanList[loop].SymbolRate < SymbolMin)
        {
            SymbolMin = Inst[Handle].ScanList.ScanList[loop].SymbolRate;
        }
    }

    /* Store the minimum symbol width for future reference */
    Inst[Handle].Cable.SymbolWidthMin = (SymbolMin * SCALED_DIG_ROLL_OFF) / (DIG_ROLL_OFF_SCALING);

    /* Default search algo (StepFreq = SR * 1.15 / 2) */
    Inst[Handle].Cable.ScanInfo.FineSearch = FALSE;
    Inst[Handle].Cable.ScanInfo.AGCLLevel = AGC_LEVEL_LO_NOSGL;
    Inst[Handle].Cable.ScanInfo.AGCHLevel = AGC_LEVEL_HI_NOSGL;

    /* Set frequency step to default */
    if (Inst[Handle].Cable.ScanInfo.FrequencyStep == 0)
    {
        Inst[Handle].Cable.ScanInfo.FrequencyStep = Inst[Handle].Cable.SymbolWidthMin/2;
    }

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s FrequencyStep =====> %u Hz (AGC %d to %d)\n", identity,
                Inst[Handle].Cable.ScanInfo.FrequencyStep,
                Inst[Handle].Cable.ScanInfo.AGCLLevel,
                Inst[Handle].Cable.ScanInfo.AGCHLevel));
#endif

    /* Setup scan parameters is we're using the scan list */
    if (!Inst[Handle].Cable.ScanExact)
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s using scan list\n", identity));
#endif
        Inst[Handle].Cable.ScanInfo.Scan          = Inst[Handle].ScanList.ScanList;
        Inst[Handle].Cable.ScanInfo.ScanIndex     = 0;
        Inst[Handle].Cable.ScanInfo.NextFrequency = Inst[Handle].Cable.ScanInfo.FrequencyStart;
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s using single scan\n", identity));
#endif
        /* We only have one scan parameter for an exact scan */
        Inst[Handle].Cable.ScanInfo.Scan          = &Inst[Handle].Cable.SingleScan;
        Inst[Handle].Cable.ScanInfo.NextFrequency = Inst[Handle].Cable.ScanInfo.FrequencyStart;
    }

    /* Release semaphore -- allows scan task to resume */
     STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);


    return(Error);

} /* CABTASK_ScanStart() */



/*----------------------------------------------------------------------------
Name: CABTASK_ScanContinue()
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
ST_ErrorCode_t CABTASK_ScanContinue(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ScanContinue()";
    clock_t                 time_start, time_end;
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 FrequencyStep, SymbolLocked;

    Inst = STTUNER_GetDrvInst();

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s instance %d waiting for GuardSemaphore...",identity, Handle));
    time_start = STOS_time_now();
#endif

    /* Wakeup the scan task and lock out the scan task whilst we modify the status code */
    STOS_SemaphoreWait(  Inst[Handle].Cable.ScanTask.GuardSemaphore);


#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    time_end = STOS_time_now();
    STTBX_Print(("(%u ticks)(%u micro-s)",
                STOS_time_minus(time_end, time_start),
                ((STOS_time_minus(time_end, time_start))*1000000)/ST_GetClocksPerSecond()
                ));
    STTBX_Print(("ok\n"));
#endif
STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.TimeoutSemaphore);


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
            SymbolLocked = Inst[Handle].Cable.ScanInfo.Scan->SymbolRate / 1000;

            /* Calculate next freq */
            FrequencyStep = (SymbolLocked * SCALED_DIG_ROLL_OFF) / (2 * DIG_ROLL_OFF_SCALING);
            /* round */
            FrequencyStep /= 1000;
            FrequencyStep *= 1000;
            /* Scale */
            FrequencyStep *= 1000;
            Inst[Handle].Cable.ScanInfo.NextFrequency += FrequencyStep * (Inst[Handle].Cable.ScanInfo.ScanDirection);
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("%s               SymbolLocked  =====> %u Ms\n", identity, SymbolLocked));
            STTBX_Print(("%s Continue with FrequencyStep =====> %u Hz\n", identity, Inst[Handle].Cable.ScanInfo.FrequencyStep));
            STTBX_Print(("%s               NextFrequency =====> %u Hz\n", identity, Inst[Handle].Cable.ScanInfo.NextFrequency));
#endif
        }

        /* Continue at the next frequency step */
        Inst[Handle].Cable.ScanInfo.ScanIndex = Inst[Handle].ScanList.NumElements;
        Inst[Handle].TunerInfo.Status   = STTUNER_STATUS_SCANNING;

        /* only relock on first single scan with a valid frequency */
        Inst[Handle].Cable.EnableReLocking = FALSE;
    }
    else
    {
        /* Unable to continue the scan -- the scan task is in the wrong
           state or the we've reached the end of the scan list */
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s fail incorrect scan state or end of scan list\n", identity));
#endif
        Error = STTUNER_ERROR_END_OF_SCAN;
    }

    /* Release semaphore -- allows scan task to resume */
     STOS_SemaphoreSignal(Inst[Handle].Cable.ScanTask.GuardSemaphore);

    return(Error);

} /* CABTASK_ScanContinue() */



/*----------------------------------------------------------------------------
Name: CABTASK_ScanTask()
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
/*
#define DEBUG_SCANTASK
*/
void CABTASK_ScanTask(STTUNER_Handle_t *Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ScanTask()";
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
    U32 a, retries = NUMBER_RETRY;
   
    Inst = STTUNER_GetDrvInst();
    index = *Handle;

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s Task for instance %d up.\n", identity, index));
#endif
Inst[index].CurrentPreStatus = STTUNER_STATUS_IDLE;
Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_IDLE;
Inst[index].CurrentPowerMode = STTUNER_NORMAL_POWER_MODE;
    while(1)
    {
        
#if defined(ST_OS21)||defined(ST_OSLINUX)
        /* Calculate wake-up time for next scan process */
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Cable.ScanTask.TimeoutDelayMs)/ 1000));

        /* Delay until either we're awoken or the wake-up time comes */
       
#else
        /* Calculate wake-up time for next scan process */
        #ifdef PROCESSOR_C1 /*for GNBvd48688-->Time calculation will be overflow*/
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() / 1000) * Inst[index].Cable.ScanTask.TimeoutDelayMs));
        #else
        clk = STOS_time_plus(STOS_time_now(), ((ST_GetClocksPerSecond() * Inst[index].Cable.ScanTask.TimeoutDelayMs)/ 1000));
        #endif

        /* Delay until either we're awoken or the wake-up time comes */
#endif
     STOS_SemaphoreWaitTimeOut(Inst[index].Cable.ScanTask.TimeoutSemaphore, &clk);
   /* We are about to enter a critical section of code -- we must not
      allow external interruptions whilst checking the scan status, etc*/
    STOS_SemaphoreWait(Inst[index].Cable.ScanTask.GuardSemaphore);

        /* If the driver is being terminated then this flag will be set
         * to TRUE -- we should cleanly exit the task. */
        if (Inst[index].Cable.ScanTask.DeleteTask == TRUE)
        {
        	    STOS_SemaphoreSignal(Inst[index].Cable.ScanTask.GuardSemaphore);

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
                Inst[index].CurrentPowerMode=STTUNER_NORMAL_POWER_MODE;
                if (Inst[index].Cable.ScanExact)
                {
                    for (a = 1; a <= retries; a++)
                    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s : retries %d\n", identity, retries));
#endif
                        Error = CABTASK_ProcessScanExact(index, &Event);
                        if (Event == STTUNER_EV_LOCKED) break;
                    }

                    /* for(a) breaks to here */
                    if (Inst[index].Cable.EnableReLocking == FALSE) /* if first pass */
                    {
                        if (Event != STTUNER_EV_LOCKED)  /* failed to lock */
                        {
                            /* Since we are scanning for a single frequency, unless we get
                            the locked event we should treat this as scan failed */
                            Event = STTUNER_EV_SCAN_FAILED;
                        }
                        else /* locked ok */
                        {
                            Inst[index].Cable.EnableReLocking = TRUE; /* also, at this point: Event == STTUNER_EV_LOCKED */
                        }
                    }

                }   /* if (ScanExact) */
                else
                {
                    Inst[index].Cable.EnableReLocking = FALSE;     /* not valid when scanning a list */
                    Error = CABTASK_ProcessNextScan(index, &Event);
                }
                break;


            /* In the "locked" case we have a frequency that we must attempt to maintain, or track */
            case STTUNER_STATUS_LOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_LOCKED\n"));
#endif
                Error = CABTASK_ProcessTracking(index, &Event);

                /* Only process thresholds if the lock wasn't lost */
                if (Event == STTUNER_EV_NO_OPERATION)
                {
                    Error = CABTASK_ProcessThresholds(index, &Event);
                }
                else if (Inst[index].Cable.EnableReLocking == TRUE) /* if lost lock then try again */
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
                
                 Event = STTUNER_EV_NO_OPERATION;
                
                 
               
                break;



           case STTUNER_STATUS_IDLE:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_IDLE\n"));
#endif           
                Inst[index].CurrentPowerMode=STTUNER_NORMAL_POWER_MODE;
                /* Fall through */
          


            /* "not found" or "unlocked" cases require no further action.
                The scan task will have to wait for user intervention */
            case STTUNER_STATUS_UNLOCKED:
#ifdef DEBUG_SCANTASK
                STTBX_Print(("STTUNER_STATUS_UNLOCKED\n"));
#endif
                /* Check the last status to ensure that the state hasn't
                been changed externally e.g., scan aborted */
                if (Inst[index].Cable.EnableReLocking == TRUE)
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
                Inst[index].Cable.ScanInfo.LockCount  = NUMBER_LOST_BEFORE_UNLOCK;
                 Inst[index].CurrentPreStatus = STTUNER_STATUS_LOCKED;
                /* Reset signal indicators */
                Inst[index].Cable.LastThreshold = UNDEF_TRES;     /* Undefined threshold */
                Inst[index].Cable.LastSignal    = UNDEF_TRES;     /* Undefined signal */
                break;

            case STTUNER_EV_UNLOCKED:                   /* We have lost lock */
                if (Inst[index].Cable.EnableReLocking != TRUE)
                {
                    Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_UNLOCKED;
                     Inst[index].CurrentPreStatus = STTUNER_STATUS_UNLOCKED;
                }
                break;

            case STTUNER_EV_SCAN_FAILED:                /* The last scan failed */
                /* The end of the scan list has been reached, we drop to the
                 "not found" status.  A callback may be invoked soon */
                Inst[index].CurrentTunerInfo.Status = STTUNER_STATUS_NOT_FOUND;
                Inst[index].PreStatus = STTUNER_STATUS_NOT_FOUND;
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
                     (Inst[index].Cable.ScanInfo.ScanIndex <  Inst[index].ScanList.NumElements) &&
                     (Inst[index].Cable.EnableReLocking    != TRUE                            ) )
                {
                    /* Proceed to the next element.  In order to avoid a spurious callback being invoked,
                    we reset the event to a "no op" */
                    Event = STTUNER_EV_NO_OPERATION;

                    Inst[index].Cable.ScanInfo.Scan++;
                    Inst[index].Cable.ScanInfo.ScanIndex++;

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                    STTBX_Print(("%s : >>>>>>>>>>> Scan %d ScanIndex %d AGC %d <<<<<<<<<<<<<<<\n", identity,
                                Inst[index].Cable.ScanInfo.Scan,
                                Inst[index].Cable.ScanInfo.ScanIndex,
                                Inst[index].CurrentTunerInfo.ScanInfo.AGC));
#endif
                    /*
                    --- For Fast Scan
                    */
                    if (Inst[index].CurrentTunerInfo.ScanInfo.AGC >= AGC_LEVEL_MAX)
                    {
                        Inst[index].Cable.ScanInfo.ScanIndex = Inst[index].ScanList.NumElements;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s : PASS TO NEXT SCAN >>>>>>>>>>> Scan %d ScanIndex %d AGC %d <<<<<<<<<<<<<<<\n", identity,
                                    Inst[index].Cable.ScanInfo.Scan,
                                    Inst[index].Cable.ScanInfo.ScanIndex,
                                    Inst[index].CurrentTunerInfo.ScanInfo.AGC));
#endif
                    }

                    /* Avoid task delay */
                     STOS_SemaphoreSignal(Inst[index].Cable.ScanTask.TimeoutSemaphore);

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
        STOS_SemaphoreSignal(Inst[index].Cable.ScanTask.GuardSemaphore);


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
           
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            if (Error != ST_NO_ERROR) STTBX_Print(("%s STEVT error = %u\n", identity, Error));
#endif
        }

    }   /* while(1)*/

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s task ending\n", identity));
#endif
    return;

} /* CABTASK_ScanTask() */



/*----------------------------------------------------------------------------
Name: CABTASK_ProcessScanExact()
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
ST_ErrorCode_t CABTASK_ProcessScanExact(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ProcessScanExact()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    STTUNER_Spectrum_t  Spectrum ;

    BOOL ScanSuccess = FALSE;
    BOOL AutoSpectrumAlgo = FALSE;
    U32  ScanFrequency;
    U32  NewFrequency;
    U32  index;

    STTUNER_Band_t   *Band;

    Inst = STTUNER_GetDrvInst();

    *Event = STTUNER_EV_NO_OPERATION;   /* Assume no lock is found */

    /* Setup the current element -- this represents a working copy we can
     modify of the current element in the scan list -- on completion it
     will be made visible to the ST TUNER API.*/
    Inst[Handle].CurrentTunerInfo.ScanInfo = *Inst[Handle].Cable.ScanInfo.Scan;

    /*Adding following line for GNBvd28574(Usage of STTUNER_GetTunerInfo) ->for proper reporting of frequency being scanned at the time*/
    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Cable.ScanInfo.NextFrequency;
    
    /* Check that the symbol rate is supported */
    if ( (Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate >= Inst[Handle].Capability.SymbolMin) &&
         (Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate <= Inst[Handle].Capability.SymbolMax) )
    {

        /* Store the minimum symbol width for future reference */
        Inst[Handle].Cable.SymbolWidthMin = (Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate * SCALED_DIG_ROLL_OFF) / (DIG_ROLL_OFF_SCALING);
        Inst[Handle].Cable.SymbolWidthMin /= 1000;

        /* Mask with the capabilties of the device */
        Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation &= Inst[Handle].Capability.Modulation;

        /* Proceed only if it is a valid band */
        if (Inst[Handle].CurrentTunerInfo.ScanInfo.Band < Inst[Handle].BandList.NumElements)
        {

            /* Set requested band */
            index = Inst[Handle].CurrentTunerInfo.ScanInfo.Band;
            Band  = &Inst[Handle].BandList.BandList[index];

            /* Ensure current frequency falls inside selected band */
            if ( (Inst[Handle].Cable.ScanInfo.NextFrequency >= Band->BandStart) &&
                 (Inst[Handle].Cable.ScanInfo.NextFrequency <= Band->BandEnd) )
            {

                /* Set the frequency used by the auto scan routine */
                ScanFrequency = (Inst[Handle].Cable.ScanInfo.NextFrequency)/1000;
                AutoSpectrumAlgo = FALSE;

                /* Set the Modulation/Spectrum Inversion */
                if (!Inst[Handle].Cable.ScanExact)
                {
                    /*
                    --- TUNER SCAN
                    */
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                    STTBX_Print(("%s TUNER SCAN\n", identity));
#endif
                    if (Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation == STTUNER_MOD_QAM)
                    {
                        /*
                        --- Search Most Usefull Modulation
                        */
                        if (Inst[Handle].Cable.Modulation == STTUNER_MOD_NONE)
                        {
                            Inst[Handle].Cable.Modulation = STTUNER_MOD_64QAM;
                        }
                    }
                    else
                    {
                        /*
                        --- Search Predefined Modulation
                        */
                        Inst[Handle].Cable.Modulation = Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation;
                    }
                    /*
                    --- Perform auto search
                    */
                    if (Inst[Handle].CurrentTunerInfo.ScanInfo.Spectrum==STTUNER_INVERSION_AUTO)
                    {
                        Spectrum = STTUNER_INVERSION_NONE;
                        AutoSpectrumAlgo = TRUE;
                    }
                    else
                    {
                        Spectrum = Inst[Handle].CurrentTunerInfo.ScanInfo.Spectrum;
                    }
                }
                else
                {
                    /*
                    --- TUNER SET FREQ
                    */
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                    STTBX_Print(("%s TUNER SET FREQ\n", identity));
#endif
                    /*
                    --- Perform auto search
                    */
                    if (Inst[Handle].Cable.SingleScan.Spectrum==STTUNER_INVERSION_AUTO)
                    {
                        Spectrum = STTUNER_INVERSION_NONE;
                        AutoSpectrumAlgo = TRUE;
                    }
                    else
                    {
                        Spectrum = Inst[Handle].Cable.SingleScan.Spectrum;
                    }
                }

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                switch(Inst[Handle].Cable.Modulation)
                {
                    case STTUNER_MOD_16QAM :
                        STTBX_Print(("%s QAM 16  : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                    case STTUNER_MOD_32QAM :
                        STTBX_Print(("%s QAM 32  : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                    case STTUNER_MOD_64QAM :
                        STTBX_Print(("%s QAM 64  : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                    case STTUNER_MOD_128QAM :
                        STTBX_Print(("%s QAM 128 : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                    case STTUNER_MOD_256QAM :
                        STTBX_Print(("%s QAM 256 : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                    case STTUNER_MOD_QAM :
                        STTBX_Print(("%s QAM     : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                    default :
                        STTBX_Print(("%s ???     : ScanFrequency=%u KHz %d S/s\n", identity,
                                    ScanFrequency,
                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate));
                        break;
                }
#endif

                /*
                --- 1st LOOP
                */
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                switch (Spectrum)
                {
                    case STTUNER_INVERSION:
                        STTBX_Print(("%s LOOP 1 STTUNER_INVERSION\n", identity));
                        break;
                    case STTUNER_INVERSION_NONE:
                        STTBX_Print(("%s LOOP 1 STTUNER_INVERSION_NONE\n", identity));
                        break;
                    case STTUNER_INVERSION_AUTO:
                        STTBX_Print(("%s LOOP 1 STTUNER_INVERSION_AUTO\n", identity));
                        break;
                    default:
                        STTBX_Print(("%s LOOP 1 STTUNER_INVERSION_????\n", identity));
                        break;
                }
#endif
                Inst[Handle].CurrentTunerInfo.ScanInfo.SpectrumIs = Spectrum;
                Inst[Handle].CurrentTunerInfo.Frequency = ScanFrequency*1000;
                Inst[Handle].CurrentTunerInfo.NumberChannels = 0;
                Inst[Handle].CurrentTunerInfo.SignalQuality = 0;
                Inst[Handle].CurrentTunerInfo.BitErrorRate = 0;
                Error = (Inst[Handle].Cable.Demod.Driver->demod_ScanFrequency)(
                                            Inst[Handle].Cable.Demod.DrvHandle,
                                            ScanFrequency,
                                            Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,
                                            (Inst[Handle].Cable.SymbolWidthMin * 1000) / 2,
                                            0, 0,
                                            &ScanSuccess,  &NewFrequency,
                                            0,0,0,0,
                                            Spectrum,
                                            0,0,0);

                Error  = (Inst[Handle].Cable.Demod.Driver->demod_GetAGC)(Inst[Handle].Cable.Demod.DrvHandle,
                                                                        &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);

                if (Error != ST_NO_ERROR)
                {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                    STTBX_Print(("%s fail set demod\n", identity));
#endif
                    return(Error);
                }

                if ( ScanSuccess )
                {
                    /* Compute actual frequency */
                    Inst[Handle].Cable.ScanInfo.NextFrequency = NewFrequency;
                    Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Cable.ScanInfo.NextFrequency;

                    /* Update status and lock count ready for tracking */
                    *Event = STTUNER_EV_LOCKED;

                }
                else
                {
                    /*
                    --- 2nd LOOP
                    */
                    /* After release 4.6.0, 2nd loop is not needed for 297J/E,     */
                    /* because 297J/E driver looks automatically for spectrum type */
                    if (AutoSpectrumAlgo && (Inst[Handle].Capability.DemodType != STTUNER_DEMOD_STV0297J) && (Inst[Handle].Capability.DemodType != STTUNER_DEMOD_STV0297E) )
                    {
                        Spectrum = STTUNER_INVERSION;

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        switch (Spectrum)
                        {
                            case STTUNER_INVERSION:
                                STTBX_Print(("%s LOOP 2 STTUNER_INVERSION\n", identity));
                                break;
                            case STTUNER_INVERSION_NONE:
                                STTBX_Print(("%s LOOP 2 STTUNER_INVERSION_NONE\n", identity));
                                break;
                            case STTUNER_INVERSION_AUTO:
                                STTBX_Print(("%s LOOP 2 STTUNER_INVERSION_AUTO\n", identity));
                                break;
                            default:
                                STTBX_Print(("%s LOOP 2 STTUNER_INVERSION_????\n", identity));
                                break;
                        }
#endif
                        Inst[Handle].CurrentTunerInfo.ScanInfo.SpectrumIs = Spectrum;
                        Inst[Handle].CurrentTunerInfo.Frequency = ScanFrequency*1000;
                        Inst[Handle].CurrentTunerInfo.NumberChannels = 0;
                        Inst[Handle].CurrentTunerInfo.SignalQuality = 0;
                        Inst[Handle].CurrentTunerInfo.BitErrorRate = 0;
                        Error = (Inst[Handle].Cable.Demod.Driver->demod_ScanFrequency)(
                                                    Inst[Handle].Cable.Demod.DrvHandle,
                                                    ScanFrequency,
                                                    Inst[Handle].CurrentTunerInfo.ScanInfo.SymbolRate,
                                                    (Inst[Handle].Cable.SymbolWidthMin * 1000) / 2,
                                                    0, 0,
                                                    &ScanSuccess,  &NewFrequency,
                                                    0,0,0,0,
                                                    Spectrum,
                                                    0,0,0);

                        Error  = (Inst[Handle].Cable.Demod.Driver->demod_GetAGC)(Inst[Handle].Cable.Demod.DrvHandle,
                                                                                &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);
                        if (Error != ST_NO_ERROR)
                        {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                            STTBX_Print(("%s fail set demod\n", identity));
#endif
                            return(Error);
                        }

                        if ( ScanSuccess )
                        {
                            /* Compute actual frequency */
                            Inst[Handle].Cable.ScanInfo.NextFrequency = NewFrequency;
                            Inst[Handle].CurrentTunerInfo.Frequency = Inst[Handle].Cable.ScanInfo.NextFrequency;

                            /* Extract remaining tuner info from h/w */
/*
                            Error = CABTASK_GetTunerInfo(Handle);
                            if (Error != ST_NO_ERROR)
                            {
                                return(Error);
                            }
*/

                            /* Update status and lock count ready for tracking */
                            *Event = STTUNER_EV_LOCKED;

                        }
                        else if(!Inst[Handle].Cable.ScanExact)
                        {
                            *Event = STTUNER_EV_SCAN_NEXT;
                        }
                    }
                    else if(!Inst[Handle].Cable.ScanExact)
                    {
                        *Event = STTUNER_EV_SCAN_NEXT;
                    }
                }
                /*
                ---
                */
            }
        }
        else
        {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("%s Bad Band\n", identity));
#endif
            /* Invalid band specified */
            Error = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s Bad SR\n", identity));
        /* Invalid SR specified */
#endif
        Error = ST_ERROR_BAD_PARAMETER;
    }

    return(Error);
} /* CABTASK_ProcessScanExact */



/*----------------------------------------------------------------------------
Name: CABTASK_ProcessTracking()
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
ST_ErrorCode_t CABTASK_ProcessTracking(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ProcessTracking()";
#endif
    ST_ErrorCode_t  Error = ST_NO_ERROR;
    STTUNER_EventType_t ThisEvent = STTUNER_EV_NO_OPERATION;
    STTUNER_InstanceDbase_t *Inst;
    BOOL IsLocked, IsSignal;
    U32 IF;

    Inst = STTUNER_GetDrvInst();

    /* Tracking */
    Error = (Inst[Handle].Cable.Demod.Driver->demod_Tracking)(Inst[Handle].Cable.Demod.DrvHandle, FALSE, &IF, &IsSignal);

    if (Error == ST_NO_ERROR)
    {
        /* The tracking function returns the frequency*/
        Inst[Handle].CurrentTunerInfo.Frequency = IF;

        Error = (Inst[Handle].Cable.Demod.Driver->demod_IsLocked)(Inst[Handle].Cable.Demod.DrvHandle, &IsLocked);

        if (Error == ST_NO_ERROR)
        {
            if (IsLocked && IsSignal)
            {
                Inst[Handle].Cable.ScanInfo.LockCount = NUMBER_LOST_BEFORE_UNLOCK;
            }
            else
            {
                Inst[Handle].Cable.ScanInfo.LockCount--;
            }

            if (Inst[Handle].Cable.ScanInfo.LockCount == 0)
            {
                ThisEvent = STTUNER_EV_UNLOCKED;
            }
            else
            {
                Error = CABTASK_GetTunerInfo(Handle);
            }
        }
    }


    if (Error != ST_NO_ERROR)
    {
        ThisEvent = STTUNER_EV_UNLOCKED;
    }

    *Event = ThisEvent;

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    STTBX_Print(("%s done ok\n", identity));
#endif
    return(Error);

} /* CABTASK_ProcessTracking() */




/*----------------------------------------------------------------------------
Name: CABTASK_ProcessThresholds()
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
ST_ErrorCode_t CABTASK_ProcessThresholds(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ProcessThresholds()";
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

    LastSignal = Inst[Handle].Cable.LastSignal;

    if ( (CurrentSignal == LastSignal) ||  (Inst[Handle].ThresholdList.NumElements == 0) )
    {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s info: no change in signal\n", identity));
#endif
        return(Error); /* No need to proceed -- no change in signal */
    }

    /*
    --- After 1st lock, no change
    */
    if ((Inst[Handle].Cable.LastThreshold == UNDEF_TRES) && (Inst[Handle].Cable.LastSignal == UNDEF_TRES) )
    {
        /* Re-set last signal indicator */
        Inst[Handle].Cable.LastSignal = CurrentSignal;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s info: First mesure\n", identity));
#endif
        return(Error);
    }
    else
    {
        /* Re-set last signal indicator */
        Inst[Handle].Cable.LastSignal = CurrentSignal;
    }

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
        for (index = 0; index < HitCount && ThresholdHits[index] != Inst[Handle].Cable.LastThreshold; index++); /* Do nothing */

        /* Check for all thresholds not equal to last threshold */
        if (index == HitCount)
        {
            /* Threshold has changed, we should ensure an event is generated */
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("%s info: MAJOR change in signal\n", identity));
#endif
            *Event = STTUNER_EV_SIGNAL_CHANGE;
            Inst[Handle].Cable.LastThreshold = ThresholdHits[0];
        }

    }
    else
    {
        /* The hit count is zero -- no thresholds hold the current
         * signal value.  An event must be generated unless this was
         * the case last time we checked.
         */
        if (Inst[Handle].Cable.LastThreshold != UNDEF_TRES)
        {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("%s info: MINOR change in signal\n", identity));
#endif
            *Event = STTUNER_EV_NO_OPERATION;
            Inst[Handle].Cable.LastThreshold = UNDEF_TRES;
        }
    }

    return(Error);

}   /* CABTASK_ProcessThresholds */



/*----------------------------------------------------------------------------
Name: CABTASK_ProcessNextScan()
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
ST_ErrorCode_t CABTASK_ProcessNextScan(STTUNER_Handle_t Handle, STTUNER_EventType_t *Event)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_ProcessNextScan()";
#endif
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;
    U32 FrequencyStep=0, FrequencyEnd=0, FrequencyStart=0;
    BOOL KeepFineTune=TRUE;

    Inst = STTUNER_GetDrvInst();


    /* Check to see if we are at the end of the scan list */
    if (Inst[Handle].Cable.ScanInfo.ScanIndex >= Inst[Handle].ScanList.NumElements)
    {
        /* Calculate correct start and end frequencies */
        if (Inst[Handle].Cable.ScanInfo.ScanDirection < 0)
        {
            FrequencyEnd   = Inst[Handle].Cable.ScanInfo.FrequencyStart;
            FrequencyStart = Inst[Handle].Cable.ScanInfo.FrequencyEnd;
        }
        else
        {
            FrequencyEnd   = Inst[Handle].Cable.ScanInfo.FrequencyEnd;
            FrequencyStart = Inst[Handle].Cable.ScanInfo.FrequencyStart;
        }

        /*
        --- Get AGC : Compute StepFreq
        */
        Error  = (Inst[Handle].Cable.Demod.Driver->demod_GetAGC)(Inst[Handle].Cable.Demod.DrvHandle,
                                                              &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);

        /*
        --- In case of No signals
        */
        if (Inst[Handle].CurrentTunerInfo.ScanInfo.AGC >= AGC_LEVEL_MAX)
        {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("%s AGC IS %d >>> AGC_LEVEL_MAX UPDATE AGC L/H LEVEL\n", identity, Inst[Handle].CurrentTunerInfo.ScanInfo.AGC));
#endif
            Inst[Handle].Cable.ScanInfo.AGCLLevel = AGC_LEVEL_LO_NOSGL;
            Inst[Handle].Cable.ScanInfo.AGCHLevel = AGC_LEVEL_HI_NOSGL;
        }

        if (Inst[Handle].CurrentTunerInfo.ScanInfo.AGC <= AGC_LEVEL_MIN)
        {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
            STTBX_Print(("%s AGC IS %d >>> AGC_LEVEL_MIN UPDATE AGC L/H LEVEL\n", identity, Inst[Handle].CurrentTunerInfo.ScanInfo.AGC));
#endif
            Inst[Handle].Cable.ScanInfo.AGCLLevel = AGC_LEVEL_LO_SGL;
            Inst[Handle].Cable.ScanInfo.AGCHLevel = AGC_LEVEL_HI_SGL;
        }

        /*
        --- Fine Scan algo
        */
        if (Inst[Handle].Cable.ScanInfo.FineSearch)
        {
            /*
            --- If Freq <= Inst[Handle].Cable.ScanInfo.FrequencyFineTune keep FINE STEP MODE
            */
            if (Inst[Handle].Cable.ScanInfo.ScanDirection < 0)
            {
                if (Inst[Handle].Cable.ScanInfo.NextFrequency > Inst[Handle].Cable.ScanInfo.FrequencyFineTune)
                {
                    KeepFineTune = TRUE;
                }
                else
                {
                    KeepFineTune = FALSE;
                }
            }
            else
            {
                if (Inst[Handle].Cable.ScanInfo.NextFrequency <= Inst[Handle].Cable.ScanInfo.FrequencyFineTune)
                {
                    KeepFineTune = TRUE;
                }
                else
                {
                    KeepFineTune = FALSE;
                }
            }
            if (KeepFineTune)
            {
                /* Apply next frequency step */
                if (Inst[Handle].Cable.ScanInfo.FrequencyStep < MIN_FREQ_STEP)
                {
                    FrequencyStep = Inst[Handle].Cable.ScanInfo.FrequencyStep;
                }
                else
                {
                    FrequencyStep = MIN_FREQ_STEP;
                }
            }
            else
            {
                if (Inst[Handle].CurrentTunerInfo.ScanInfo.AGC >= Inst[Handle].Cable.ScanInfo.AGCHLevel)
                {
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                    STTBX_Print(("%s AGC IS %d >>> PASS TO LARGE STEP MODE\n", identity, Inst[Handle].CurrentTunerInfo.ScanInfo.AGC));
                    STTBX_Print(("%s : FrequencyStart %d FrequencyEnd %d\n", identity, FrequencyStart, FrequencyEnd));
#endif
                    Inst[Handle].Cable.ScanInfo.FineSearch = FALSE;
                    /* Apply next frequency step */
                    FrequencyStep = Inst[Handle].Cable.ScanInfo.FrequencyStep;
                }
                else
                {
                    /* Apply next frequency step */
                    FrequencyStep = MIN_FREQ_STEP;
                }
            }
        }
        /*
        --- Large Scan algo
        */
        else
        {
            if (Inst[Handle].CurrentTunerInfo.ScanInfo.AGC <= Inst[Handle].Cable.ScanInfo.AGCLLevel)
            {
                Inst[Handle].Cable.ScanInfo.FrequencyFineTune = Inst[Handle].Cable.ScanInfo.NextFrequency;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                STTBX_Print(("%s AGC IS %d >>> PASS TO FINE STEP MODE FrequencyFineTune %d\n", identity, Inst[Handle].CurrentTunerInfo.ScanInfo.AGC, Inst[Handle].Cable.ScanInfo.FrequencyFineTune));
                STTBX_Print(("%s : FrequencyStart %d FrequencyEnd %d\n", identity, FrequencyStart, FrequencyEnd));
#endif
                Inst[Handle].Cable.ScanInfo.FineSearch = TRUE;

                /* We would like to be sure we mised nothing */
/*                Inst[Handle].Cable.ScanInfo.NextFrequency -= (Inst[Handle].Cable.ScanInfo.ScanDirection * (S32)(Inst[Handle].Cable.SymbolWidthMin/2));*/
                Inst[Handle].Cable.ScanInfo.NextFrequency -= (Inst[Handle].Cable.ScanInfo.ScanDirection * (S32)(MIN_FREQ_WIDTH));
                if (Inst[Handle].Cable.ScanInfo.ScanDirection < 0)
                {
                    if (Inst[Handle].Cable.ScanInfo.NextFrequency > FrequencyEnd)
                    {
                        Inst[Handle].Cable.ScanInfo.NextFrequency = FrequencyEnd;
                    }
                }
                else
                {
                    if (Inst[Handle].Cable.ScanInfo.NextFrequency < FrequencyStart)
                    {
                        Inst[Handle].Cable.ScanInfo.NextFrequency = FrequencyStart;
                    }
                }

                /* Apply next frequency step */
                FrequencyStep = MIN_FREQ_STEP;
            }
            else
            {
                /* Apply next frequency step */
                FrequencyStep = Inst[Handle].Cable.ScanInfo.FrequencyStep;
            }
        }

        /* Apply next frequency step */
        Inst[Handle].Cable.ScanInfo.NextFrequency += (Inst[Handle].Cable.ScanInfo.ScanDirection * (S32)FrequencyStep);

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s : NextFrequency %06d KHz AGC %04d (%04d to %04d) Step %d KHz\n", identity,
                        Inst[Handle].Cable.ScanInfo.NextFrequency/1000, Inst[Handle].CurrentTunerInfo.ScanInfo.AGC,
                        Inst[Handle].Cable.ScanInfo.AGCLLevel,
                        Inst[Handle].Cable.ScanInfo.AGCHLevel,
                        FrequencyStep/1000));
#endif

        /* If we've reached the limit of our frequency range, then we should try
         * and use the next polarization setting -- if there is one.
         */
        if ( (Inst[Handle].Cable.ScanInfo.NextFrequency > FrequencyEnd)   ||
             (Inst[Handle].Cable.ScanInfo.NextFrequency < FrequencyStart) )
        {
            /*
            --- Try all Modulations
            */
            if (Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation == STTUNER_MOD_QAM)
            {
                switch(Inst[Handle].Cable.Modulation)
                {
                    /*
                    --- Sort by most useful
                    */
                    case STTUNER_MOD_64QAM :
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s NEXT MODULATION IS QAM256\n", identity));
#endif
                        Inst[Handle].Cable.ScanInfo.NextFrequency = Inst[Handle].Cable.ScanInfo.FrequencyStart;
                        Inst[Handle].Cable.Modulation = STTUNER_MOD_256QAM;
                        break;
                    case STTUNER_MOD_256QAM :
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s NEXT MODULATION IS QAM128\n", identity));
#endif
                        Inst[Handle].Cable.ScanInfo.NextFrequency = Inst[Handle].Cable.ScanInfo.FrequencyStart;
                        Inst[Handle].Cable.Modulation = STTUNER_MOD_128QAM;
                        break;
                    case STTUNER_MOD_128QAM :
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s NEXT MODULATION IS QAM32\n", identity));
#endif
                        Inst[Handle].Cable.ScanInfo.NextFrequency = Inst[Handle].Cable.ScanInfo.FrequencyStart;
                        Inst[Handle].Cable.Modulation = STTUNER_MOD_32QAM;
                        break;
                    case STTUNER_MOD_32QAM :
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s NEXT MODULATION IS QAM16\n", identity));
#endif
                        Inst[Handle].Cable.ScanInfo.NextFrequency = Inst[Handle].Cable.ScanInfo.FrequencyStart;
                        Inst[Handle].Cable.Modulation = STTUNER_MOD_16QAM;
                        break;
                    case STTUNER_MOD_16QAM :
                    default :
                        /* No more combinations to try -- scan has failed */
                        Inst[Handle].Cable.Modulation = Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation;
                        *Event = STTUNER_EV_SCAN_FAILED;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                        STTBX_Print(("%s STTUNER_EV_SCAN_FAILED\n", identity));
#endif
                        return(Error);
                        break;
                }
            }
            else
            {
                *Event = STTUNER_EV_SCAN_FAILED;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
                STTBX_Print(("%s STTUNER_EV_SCAN_FAILED\n", identity));
#endif
                return(Error);
            }
        }

        /* Set scan list to beginning */
        Inst[Handle].Cable.ScanInfo.Scan = Inst[Handle].ScanList.ScanList;
        Inst[Handle].Cable.ScanInfo.ScanIndex = 0;
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
        STTBX_Print(("%s : >>>>>>>>>>> Scan %d ScanIndex %d AGC %d <<<<<<<<<<<<<<<\n", identity,
                    Inst[Handle].Cable.ScanInfo.Scan,
                    Inst[Handle].Cable.ScanInfo.ScanIndex,
                    Inst[Handle].CurrentTunerInfo.ScanInfo.AGC));
#endif
    }

    /* Process the next frequency scan step */
    Error = CABTASK_ProcessScanExact(Handle, Event);

    return(Error);
}



/*----------------------------------------------------------------------------
Name: CABTASK_GetTunerInfo()
Description:
    Calls the DEMOD device in order to get the current demodulation settings
    for updating the current tuner info.  This information will eventually
    be forwarded back up to the STTUNER API tuner info.
Parameters:

Return Value:
See Also:
----------------------------------------------------------------------------*/
ST_ErrorCode_t CABTASK_GetTunerInfo(STTUNER_Handle_t Handle)
{
#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
    const char *identity = "STTUNER cabtask.c CABTASK_GetTunerInfo()";
#endif
    ST_ErrorCode_t Error=ST_NO_ERROR;
    STTUNER_InstanceDbase_t *Inst;

    Inst = STTUNER_GetDrvInst();

    /* Get the current demodulation parameters */

    Error  = (Inst[Handle].Cable.Demod.Driver->demod_GetAGC)(Inst[Handle].Cable.Demod.DrvHandle,
                                                          &Inst[Handle].CurrentTunerInfo.ScanInfo.AGC);

    Error |= (Inst[Handle].Cable.Demod.Driver->demod_GetSignalQuality)(Inst[Handle].Cable.Demod.DrvHandle,
                                                                    &Inst[Handle].CurrentTunerInfo.SignalQuality,
                                                                    &Inst[Handle].CurrentTunerInfo.BitErrorRate);

    Error |= (Inst[Handle].Cable.Demod.Driver->demod_GetModulation)(Inst[Handle].Cable.Demod.DrvHandle,
                                                                 &Inst[Handle].CurrentTunerInfo.ScanInfo.Modulation);

    if (Error != ST_NO_ERROR)
    {

#ifdef STTUNER_DEBUG_MODULE_CAB_CABTASK
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

    return(Error);
} /* CABTASK_GetTunerInfo() */


/* End of cabtask.c */
