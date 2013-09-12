/*****************************************************************************
File Name   : irtest.h

Description : STBLAST test harness header file.

Copyright (C) 1999 STMicroelectronics

Reference   :

ST API Definition "BLAST Driver API" DVD-API-22 Revision 0.1
*****************************************************************************/

#ifndef __BLASTTEST_H
#define __BLASTTEST_H

#if !defined(ST_OSLINUX)
#include "sttbx.h"
#endif

#include "stblast.h"


#if !defined(DISABLE_TBX)
#define STBLAST_Print(x)          STTBX_Print(x);
#else
#define STBLAST_Print(x)          printf x;
#endif

/*****************************************************************************
Prototypes for testing STBLAST driver's internal routines:
The driver must be built with "-DSTBLAST_DEBUG" to test driver internal
functions.  See "Makefile" for more information.
*****************************************************************************/

/* Pass/fail counts */
typedef struct BLAST_TestResult_s
{
    U32 NumberPassed;
    U32 NumberFailed;
} BLAST_TestResult_t;

/* Error messages */
typedef struct BLAST_ErrorMessage_s
{
    ST_ErrorCode_t Error;
    char ErrorMsg[56];
} BLAST_ErrorMessage_t;

/* Test parameters -- passed to every test routine */
typedef struct BLAST_TestParams_s
{
    U32 Ref;
    STBLAST_Handle_t *Handle;
} BLAST_TestParams_t;

/* Defines a test harness function */
struct BLAST_TestEntry_s
{
    BLAST_TestResult_t (*TestFunction)(BLAST_TestParams_t *);
    char TestInfo[50];
    U32 RepeatCount;
};

typedef struct BLAST_TestEntry_s BLAST_TestEntry_t;

/* Debug message prefix string (always appended with a ':') */
#define DEBUG_PREFIX                "STBLAST"

/* Debug output */
#define BLAST_DebugPrintf(args)      STBLAST_Print(("%s: ", DEBUG_PREFIX)); STBLAST_Print(args)
#define BLAST_DebugError(msg,err)    STBLAST_Print(("%s: %s = %s\n", DEBUG_PREFIX, msg, BLAST_ErrorString(err)))
#define BLAST_DebugMessage(msg)      STBLAST_Print(("%s: %s\n", DEBUG_PREFIX, msg))
char *BLAST_ErrorString(ST_ErrorCode_t Error);

/* Test success indicator */
#define BLAST_TestPassed(x) x.NumberPassed++; BLAST_DebugPrintf(("Result: **** PASS **** \n"))
#define BLAST_TestFailed(x,reason) x.NumberFailed++; BLAST_DebugPrintf(("Result: !!!! FAIL !!!! (%s)\n", reason))
#define TEST_RESULT_ZERO    {0,0}

#endif /* __BLASTTEST_H */

/* End of header */
