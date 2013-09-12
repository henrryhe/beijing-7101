/* ----------------------------------------------------------------------------
File Name: tertask.h

Description:

    Data structures, for managing terrestrial device scan task.

Copyright (C) 1999-2001 STMicroelectronics

History:

   date: 11-Sept-2001
version: 3.1.0
 author: GB from work by GJP (was tuner.h)
comment:

Reference:

---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_TER_TERTASK_H
#define __STTUNER_TER_TERTASK_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* constants --------------------------------------------------------------- */
#ifdef ST_OS21
#define SCAN_TASK_STACK_SIZE (16 * 1024)
#else
#define SCAN_TASK_STACK_SIZE (4 * 1024)
#endif

#ifndef STTUNER_SCAN_TER_TASK_PRIORITY    /* for GNBvd25440 --> Allow override of task priorities */
#define STTUNER_SCAN_TER_TASK_PRIORITY    MAX_USER_PRIORITY
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

        task_t       *Task;

        tdesc_t      TaskDescriptor;

        char         TaskName[16];
        U8           *ScanTaskStack;

        semaphore_t  *GuardSemaphore;
        semaphore_t  *TimeoutSemaphore;

        U32          TimeoutDelayMs;
        BOOL         DeleteTask;
    } TERTASK_ScanTask_t;


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
    } TERTASK_ScanInfo_t;


/* functions --------------------------------------------------------------- */

/* called by open.c to start the scan task */
ST_ErrorCode_t TERTASK_Open (STTUNER_Handle_t Handle);
ST_ErrorCode_t TERTASK_Close(STTUNER_Handle_t Handle);

ST_ErrorCode_t TERTASK_ScanAbort   (STTUNER_Handle_t Handle);
ST_ErrorCode_t TERTASK_ScanStart   (STTUNER_Handle_t Handle);
ST_ErrorCode_t TERTASK_ScanContinue(STTUNER_Handle_t Handle);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_TER_TERTASK_H */


/* End of tertask.h */
