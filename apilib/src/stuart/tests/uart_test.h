/*****************************************************************************
File Name   : uart_test.h

Description : STUART test harness header file.

Copyright (C) 1999 STMicroelectronics

Reference   :

ST API Definition "UART Driver API" DVD-API-22 Revision 0.1
*****************************************************************************/

#ifndef __UARTTEST_H
#define __UARTTEST_H

#ifdef STTBX_PRINT
#include "sttbx.h"
#endif

/*****************************************************************************
Prototypes for testing STUART driver's internal routines:
The driver must be built with "-DSTUART_DEBUG" to test driver internal
functions.  See "Makefile" for more information.
*****************************************************************************/

/* Pass/fail counts */
typedef struct
{
    U32 NumberPassed;
    U32 NumberFailed;
} UART_TestResult_t;

/* Error messages */
typedef struct
{
    ST_ErrorCode_t Error;
    char ErrorMsg[32];
} UART_ErrorMessage;

/* Test parameters -- passed to every test routine */
typedef struct
{
    U32 Ref;
    U8 ASCNumber;
#if defined(TEST_MULTITHREADSAFE)
    BOOL IsBlocking;
#endif
} UART_TestParams_t;

/* Defines a test harness function */
struct UART_TestEntry_s
{
    UART_TestResult_t (*TestFunction)(UART_TestParams_t *);
    char TestInfo[50];
    U32 RepeatCount;
    U8 ASCNumber;
#if defined(TEST_MULTITHREADSAFE)
    BOOL IsBlocking;
#endif
};

typedef struct UART_TestEntry_s UART_TestEntry_t;

/* Debug message prefix string (always appended with a ':') */
#define DEBUG_PREFIX                "STUART"

/* Debug output */
#ifdef STTBX_PRINT
#define UART_DebugPrintf(args)      STTBX_Print(("%s: ", DEBUG_PREFIX)); STTBX_Print(args)
#define UART_DebugError(msg,err)    STTBX_Print(("%s: %s = %s\n", DEBUG_PREFIX, msg, UART_ErrorString(err)))
#define UART_DebugMessage(msg)      STTBX_Print(("%s: %s\n", DEBUG_PREFIX, msg))
#else
#define UART_DebugPrintf(args)      printf(DEBUG_PREFIX ": "); printf args
#define UART_DebugError(msg,err)    printf(DEBUG_PREFIX ": %s:%s\n", msg, UART_ErrorString(err))
#define UART_DebugMessage(msg)      printf(DEBUG_PREFIX ": %s\n", msg)
#endif

char *UART_ErrorString(ST_ErrorCode_t Error);

/* Test success indicator */
#define UART_TestPassed(x) x.NumberPassed++; UART_DebugPrintf(("Result: **** PASS **** \n"))
#define UART_TestFailed(x,reason) x.NumberFailed++; UART_DebugPrintf(("Result: !!!! FAIL !!!! (%s)\n", reason))
#define TEST_RESULT_ZERO    {0,0}

#endif /* __UARTTEST_H */

/* End of header */
