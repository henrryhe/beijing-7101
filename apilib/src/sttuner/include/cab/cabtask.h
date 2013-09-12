
/* ----------------------------------------------------------------------------
File Name: cabtask.h

Description:

    Data structures, for managing cable device scan task.

Copyright (C) 1999-2001 STMicroelectronics

   date: 01-October-2001
version: 3.2.0
 author: from STV0299 and MB validation drivers.
comment: Write for multi-instance/multi-FrontEnd.

Revision History:

Reference:
    ST API Definition "TUNER Driver API" DVD-API-06
---------------------------------------------------------------------------- */

/* define to prevent recursive inclusion */
#ifndef __STTUNER_CAB_CABTASK_H
#define __STTUNER_CAB_CABTASK_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


/* constants --------------------------------------------------------------- */

/*increasing task size due to big arrays(52800bytes) in use for 498 & 297E in Adjacent channel & All pass filters*/
#ifdef ST_OS21
#define CAB_SCANTASK_STACK_SIZE ( (16*1024) +52800)
#else
#define CAB_SCANTASK_STACK_SIZE ( (8*1024) + 52800)
#endif

#ifndef STTUNER_SCAN_CAB_TASK_PRIORITY    /* for GNBvd25440 --> Allow override of task priorities */
#define STTUNER_SCAN_CAB_TASK_PRIORITY    MAX_USER_PRIORITY
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
    }
    CABTASK_ScanTask_t;


    /* Scan information */
    typedef struct
    {
        STTUNER_Scan_t      *Scan;
        STTUNER_Status_t    *Status;
        U32                 ScanIndex;
        U32                 FrequencyStart;
        U32                 FrequencyEnd;
        U32                 FrequencyFineTune;
        S32                 FrequencyStep;
        S32                 ScanDirection;
        U32                 NextFrequency;
        U32                 LockCount;
        BOOL                FineSearch;
        U32                 AGCLLevel;
        U32                 AGCHLevel;
        STTUNER_J83_t           J83;
    }
    CABTASK_ScanInfo_t;


/* functions --------------------------------------------------------------- */

/* called by open.c to start the scan task */
ST_ErrorCode_t CABTASK_Open (STTUNER_Handle_t Handle);
ST_ErrorCode_t CABTASK_Close(STTUNER_Handle_t Handle);

ST_ErrorCode_t CABTASK_ScanAbort   (STTUNER_Handle_t Handle);
ST_ErrorCode_t CABTASK_ScanStart   (STTUNER_Handle_t Handle);
ST_ErrorCode_t CABTASK_ScanContinue(STTUNER_Handle_t Handle);



#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* __STTUNER_CAB_CABTASK_H */


/* End of cabtask.h */
