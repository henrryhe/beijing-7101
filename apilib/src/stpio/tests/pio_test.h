/*****************************************************************************
File Name   : pio_test.h

Description : STPIO test harness header file.

Copyright (C) 2000 STMicroelectronics

Reference   :

*****************************************************************************/

#ifndef __PIOTEST_H
#define __PIOTEST_H

#if !defined(ST_OSLINUX)
#ifdef STTBX_PRINT
#include "sttbx.h"
#endif
#endif

/* Pass/fail counts */
typedef struct
{
    U32 NumberPassed;
    U32 NumberFailed;
} TestResult_t;

/* Error messages */
typedef struct
{
    ST_ErrorCode_t Error;
    const char ErrorMsg[32];
} ErrorMessage_t;

/* Test parameters -- passed to every test routine */
typedef struct
{
    U32     Ref;
    void    *Params_p;
} TestParams_t;

/* Defines a test harness function */
struct TestEntry_s
{
    TestResult_t    (*TestFunction)(TestParams_t *);
    char            TestInfo[50];
    U32             RepeatCount;
    void            *TestParams_p;
};

typedef struct TestEntry_s TestEntry_t;

/* Debug message prefix string (always appended with a ':') */
#define DEBUG_PREFIX                "STPIO"

/* Debug output */
#ifdef STTBX_PRINT
#define DebugPrintf(args)      STTBX_Print(("%s: ", DEBUG_PREFIX)); STTBX_Print(args)
#define DebugError(msg,err)    STTBX_Print(("%s: %s = %s\n", DEBUG_PREFIX, msg, ErrorToString(err)))
#define DebugMessage(msg)      STTBX_Print(("%s: %s\n", DEBUG_PREFIX, msg))
#else
#define DebugPrintf(args)      printf("%s: ", DEBUG_PREFIX); printf args
#define DebugError(msg,err)    printf("%s: %s = %s\n", DEBUG_PREFIX, msg, ErrorToString(err))
#define DebugMessage(msg)      printf("%s: %s\n", DEBUG_PREFIX, msg)
#endif

char *ErrorToString(ST_ErrorCode_t Error);

/* Test success indicator */
#define TestPassed(x) x.NumberPassed++; DebugPrintf(("Result: **** PASS **** \n"))
#define TestFailed(x,reason) x.NumberFailed++; DebugPrintf(("Result: !!!! FAIL !!!! (%s)\n", reason))
#define TEST_RESULT_ZERO    {0,0}

#endif /* __PIOTEST_H */

/* End of header */
