/*****************************************************************************
File Name   : boot_test.h

Description : STBOOT test harness header file.

Copyright (C) 1999 SGS Thomson Microelectronics

Reference   :

ST API Definition "STAPI Boot Initialization" DVD-API-xxx
*****************************************************************************/

#ifndef __BOOT_TEST_H
#define __BOOT_TEST_H

#include "stddefs.h"

/*****************************************************************************
Prototypes for testing STBOOT driver's internal routines:
The driver must be built with "-DSTBOOT_DEBUG" to test driver internal
functions.  See "makefile" for more information.
*****************************************************************************/

/* Pass/fail counts */
typedef struct
{
    U32     NumberPassed;
    U32     NumberFailed;
} BOOT_TestResult_t;

/* Error messages */
typedef struct
{
    ST_ErrorCode_t  Error;
    char            ErrorMsg[32];
} BOOT_ErrorMessage;

/* Test parameters -- passed to every test routine */
typedef struct
{
    U32     Ref;
    U8      Slot;
} BOOT_TestParams_t;

/* Defines a test harness function */
struct BOOT_TestEntry_s
{
    BOOT_TestResult_t  (*TestFunction)(BOOT_TestParams_t *);
    char                TestInfo[50];
    U32                 RepeatCount;
};

typedef struct BOOT_TestEntry_s BOOT_TestEntry_t;

/* Debug message prefix string (always appended with a ':') */
#define BOOT_DEBUG_PREFIX           "STBOOT"

/* Debug output */
#define BOOT_DebugPrintf(args)      printf("%s: ", BOOT_DEBUG_PREFIX); printf args
#define BOOT_DebugError(msg,err)    printf("%s: %s = %s\n", BOOT_DEBUG_PREFIX,msg, BOOT_ErrorString(err))
#define BOOT_DebugMessage(msg)      printf("%s: %s\n", BOOT_DEBUG_PREFIX, msg)
char *BOOT_ErrorString(ST_ErrorCode_t Error);

/* Test success indicator */
#define BOOT_TestPassed(x) x.NumberPassed++; BOOT_DebugPrintf(("Result: PASS\n"))
#define BOOT_TestFailed(x,msg,reason) x.NumberFailed++; BOOT_DebugPrintf(("Result: FAIL: %s (%s)\n", msg, reason))

/* Zero out test result structure */
#define TEST_RESULT_ZERO    {0, 0}

#endif /* __BOOT_TEST_H */

/* End of header */
