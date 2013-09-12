
/* ----------------------------------------------------------------------------
File Name: sattask.h

Description: 

    Data structures, for managing satellite device scan task.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 6-June-2001
version: 3.1.0
 author: GJP from work by LW (was tuner.h)
comment: 
    
Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_SAT_SATTASK_H
#define __STTUNER_SAT_SATTASK_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* constants --------------------------------------------------------------- */
#ifdef ST_OS21
#define SAT_SCANTASK_STACK_SIZE 16*1024
#else
#define SAT_SCANTASK_STACK_SIZE 4096
#endif

#ifndef STTUNER_SCAN_SAT_TASK_PRIORITY    /* for GNBvd25440 --> Allow override of task priorities */
#define STTUNER_SCAN_SAT_TASK_PRIORITY    MAX_USER_PRIORITY
#endif


/* includes ---------------------------------------------------------------- */

#include "sttuner.h"
#include "stevt.h"
#include "stapitnr.h"


/* ------------------------------------------------------------------------- */

    /* Instance Scan Task Information */
    typedef struct
    {
        STTUNER_Handle_t TopLevelHandle;


	task_t        *Task;
        tdesc_t      TaskDescriptor;
  
        char         TaskName[16];
        U8           *ScanTaskStack;

        semaphore_t  *GuardSemaphore;
        semaphore_t  *TimeoutSemaphore;
    
        U32          TimeoutDelayMs;
        BOOL         DeleteTask;
    } SATTASK_ScanTask_t;


    /* Scan information */
    typedef struct
    {
        STTUNER_Scan_t      *Scan;
        STTUNER_Status_t    *Status;
        U32                  ScanIndex;
        U32                  FrequencyStart;
        U32                  FrequencyEnd;
        S32                  FrequencyStep;
        S32                  ScanDirection;
        U32                  FrequencyLO;
        U32                  NextFrequency;
        U32                  LockCount;
        U32                  PlrMask;
        STTUNER_DownLink_t   DownLink;
        STTUNER_BlindScan_t     *BlindScanInfo;
    } SATTASK_ScanInfo_t;


/* functions --------------------------------------------------------------- */

/* called by open.c to start the scan task */
ST_ErrorCode_t SATTASK_Open (STTUNER_Handle_t Handle);
ST_ErrorCode_t SATTASK_Close(STTUNER_Handle_t Handle);

/* called by sattask for timeout implementation in setfrequency */
ST_ErrorCode_t SATTASK_TimeoutTaskOpen (STTUNER_Handle_t Handle);
ST_ErrorCode_t SATTASK_TimeoutTaskClose(STTUNER_Handle_t Handle);


ST_ErrorCode_t SATTASK_ScanAbort   (STTUNER_Handle_t Handle);
ST_ErrorCode_t SATTASK_ScanStart   (STTUNER_Handle_t Handle);
ST_ErrorCode_t SATTASK_ScanContinue(STTUNER_Handle_t Handle);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_SAT_SATTASK_H */


/* End of sattask.h */
