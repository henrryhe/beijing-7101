/*****************************************************************************
File Name   : pccrdtst.h

Description : STPCCRD test harness header file.

History  :

Copyright (C) 2005 STMicroelectronics

Reference   : ST API Definition "STPCCRD Driver API" STB-API-332
              STPCCRD Driver Test Plan V1.1
*****************************************************************************/

#ifndef __PCCRD_TEST_H
#define __PCCRD_TEST_H

#ifdef __cplusplus
    extern "C" {
#endif
/* Includes --------------------------------------------------------------- */

/*****************************************************************************
Device enumerations for SC, ASC and PIO
*****************************************************************************/
/* Enum of PIO devices */
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_NOT_USED
};
/* Enum of I2C devices */
enum
{
    I2C_BACK_BUS,
    I2C_FRONT_BUS,
    I2C_DEVICES
};

/* Exported Constants ----------------------------------------------------- */

/* Debug message prefix string (always appended with a ':') */
#define PCCRD_DEBUG_PREFIX           "STPCCRD"
/* Zero out test result structure */
#define TEST_RESULT_ZERO    {0, 0}
/* Size of notify event queue */
#define MQUEUE_SIZE             50
#define MAXSTRING               255
/* Test harness revision number */
const U8 Revision[] = "1.0.2";

#define MAX_PCCRD_DEVICE        1          /* Number of PCCRD driver instance */
/* Number of PCCRD driver Handle */
#if defined(ST_5105) || defined(ST_5107)
#define MAX_PCCRD_HANDLE        1
#else
#define MAX_PCCRD_HANDLE        2
#endif
#define MAX_EVT                 MAX_PCCRD_DEVICE /* Number of STEVT driver instance */
#define SW_TASK_STACK_SIZE      1024        /* Task stack size */
#define NUM_MULTIPLE_TASK       3           /* Number of task for multiple client test */
#define MAX_RETRY_CNT           4000        /* Retry count for status/command bits */
#define CREATE_TC_LEN           5           /* CREATE_TC pdu length*/
#define CREATE_TC_REPLY_LEN     9           /* CREATE_TC_RELPY pdu length*/
#define T_DATA_LAST_LEN         5           /* T_DATA_LAST pdu length*/
#define T_SB_LEN                6           /* T_SB pdu length*/
#define DA_BIT_SET              0x80        /* To check if the DA bit is set while polling */
#define BUFFER_LEN              15          /* Local buffer length */
#define INSRT_RMV_TST_RETRY     10          /* Number of repeatation for Insert remove test */
#define FIRST_INSTANCE          0           /* Specifying the first instance of STPCCRD driver */

/* Device code for the I2C interface device */
#if defined(ST_5514)   || defined(ST_5518)
#define PCCRD_DEV_CODE          0x80
#elif defined(ST_5528)
#define PCCRD_DEV_CODE          0x86
#elif defined(ST_5516) || defined(ST_5517)  /* PCF8575 */
#define PCCRD_DEV_CODE          0x40
#elif defined(ST_7710) || defined(ST_5100) || defined(ST_5301)
#define EPLD_EPM3256A_ADDR      0x41900000
#define EPLD_EPM3256B_ADDR      0x41900000
#define PCCRD_DEV_CODE          EPLD_EPM3256A_ADDR
#elif defined(ST_7100) || defined(ST_7109)
#define PCCRD_DEV_CODE          0x40
#elif defined(ST_5105) || defined(ST_5107)
#define EPLD_EPM3256A_ADDR      0x45400000
#define EPLD_EPM3256B_ADDR      0x45400000
#define PCCRD_DEV_CODE          EPLD_EPM3256A_ADDR
#endif

#define MAX_EVT_SUBS_NUM        4           /* Maximum number of Events subscribed to STEVT */
#define MAX_EVT_CONNCT_NUM      3           /* Maximum number of clients connected to STEVT */
#define MAX_EVT_NUM             4           /* Maximum number of Events */
#define SET_COMMAND_BIT         1           /* To specify the command bit as high */
#define RESET_COMMAND_BIT       0           /* To specify the command bit as low */
#define SHIFT_EIGHT             8           /* To specify the eight bit shifting */
#define PCCRD_RESET_DELAY       4000        /* Delay required after module reset */

/*****************************************************************************
Definitions for PIO
*****************************************************************************/
/* Maximum total PIO devices we support */
#define MAX_PIO                  6

#if defined(TYLKO) || defined(ST_5528)
#define MAX_I2C                  1
#else
#define MAX_I2C                  2
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)
#define BACK_PIO                 3
#define FRONT_PIO                3
#elif defined(ST_7100) || defined(ST_7109)
#define BACK_PIO                 2
#define FRONT_PIO                3
#elif defined(ST_5518)
#define BACK_PIO                 1
#define FRONT_PIO                5
#endif /* ST_5514 || ST_5516 || ST_5517 */

/* Now define the number of PIOs that are actually on the current
 * platform
 */
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5518) || defined(ST_7100) \
|| defined(ST_7710) || defined(ST_7109)
#define MAX_ACTUAL_PIO          6
#elif defined(ST_5528)
#define MAX_ACTUAL_PIO          8
#else
#define MAX_ACTUAL_PIO          5
#endif /* ST_5514 || ST_5516 || ST_5517 || ST_5518*/

/* Bit definition for an unused PIO bit */
#define PIO_BIT_NOT_USED         0
#define PIO_FOR_SSC0_SDA         PIO_BIT_0
#define PIO_FOR_SSC0_SCL         PIO_BIT_1

#if !defined(ST_OSLINUX)
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
#define PIO_FOR_SSC1_SDA         PIO_BIT_2
#define PIO_FOR_SSC1_SCL         PIO_BIT_3
#elif defined(ST_5518) || defined(ST_7100) || defined(ST_5528) || defined(ST_7109)
#define PIO_FOR_SSC1_SDA         PIO_BIT_0
#define PIO_FOR_SSC1_SCL         PIO_BIT_1
#endif /* ST_5514 || ST_5516 || ST_5517 */
#endif/*ST_OSLINUX*/

#ifdef ST_OS21
/* Clock Source */
#define CLOCK_CPU                ST_GetClockSpeed()
#define CLOCK_COMMS              200000000

#elif defined(ST_OSLINUX)/*ST_7100*/
#define CLOCK_CPU                266000000
#define CLOCK_COMMS              200000000

#else
#define CLOCK_CPU                ClockInfo.C2
#define CLOCK_COMMS              ClockInfo.CommsBlock
#endif

/* Cache settings */
#if defined(DISABLE_ICACHE)
#define ICACHE  FALSE
#else
#define ICACHE  TRUE
#endif

#if defined(DISABLE_DCACHE)
#define DCACHE  FALSE
#else
#define DCACHE  TRUE
#endif

#if (DCACHE == TRUE)

#if defined(CACHEABLE_BASE_ADDRESS)
#define DCACHE_START    CACHEABLE_BASE_ADDRESS
#define DCACHE_END      CACHEABLE_STOP_ADDRESS
#else
#if defined(UNIFIED_MEMORY)
#define DCACHE_START    0x40080000
#define DCACHE_END      0x4FFFFFFF
#else
#if defined(ST_5518)
#define DCACHE_START    0x40000000
#define DCACHE_END      0x4007FFFF
#elif defined(ST_5514) || defined(ST_5516)|| defined(ST_5517) || defined(ST_7100) || defined(ST_7710) \
|| defined(ST_7109)
#define DCACHE_START    0x40200000
#define DCACHE_END      0x4FFFFFFF
#endif/* ST_5518*/
#endif
#endif
#else

#define DCACHE_START    NULL
#define DCACHE_END      NULL

#endif

/* Exported Variables ----------------------------------------------------- */
#define TC_ID                  0x01 /* Transport Connection Id */

/* Create TC PDU */
static U8 Create_T_C[5] = {TC_ID, 0x00, 0x82, 0x01, TC_ID };
/* Create TC_Reply PDU */
static U8 Create_T_C_Reply[9] = {TC_ID, 0x00, 0x83, 0x01, TC_ID,0x80,0x02,TC_ID,0x00};
/* T_DATA_LAST PDU */
static U8 T_DATA_LAST[5]= {TC_ID, 0x00,0xA0,0x01,TC_ID};
/* T_SB PDU */
static U8 T_SB[5] = {TC_ID, 0x00,0x80,0x02,TC_ID};

/* Exported Types --------------------------------------------------------- */

/* Pass/fail counts */
typedef struct
{
    U32     NumberPassed;
    U32     NumberFailed;
} PCCRD_TestResult_t;

/* Test parameters -- passed to every test routine */
typedef struct
{
    U32     Ref;
    STPCCRD_ModName_t      Slot;
} PCCRD_TestParams_t;

/* Defines a test harness function */
struct PCCRD_TestEntry_s
{
    PCCRD_TestResult_t  (*TestFunction)(PCCRD_TestParams_t *);
    char                TestInfo[50];
    U32                 RepeatCount;
    STPCCRD_ModName_t        Slot;
};

typedef struct PCCRD_TestEntry_s PCCRD_TestEntry_t;

/* Exported Functions ----------------------------------------------------- */
static char *GetErrorText(ST_ErrorCode_t ST_ErrorCode);
char *PCCRD_EventToString(STPCCRD_EventType_t Event);

/* Exported Macros -------------------------------------------------------- */

/* Debug output */

#define STPCCRD_Print(x)          STTBX_Print(x);

#define PCCRD_DebugPrintf(args)      STPCCRD_Print(("%s: ", PCCRD_DEBUG_PREFIX)); STTBX_Print(args)
#define PCCRD_DebugError(msg,err)    STPCCRD_Print(("%s: %s = %s\n", PCCRD_DEBUG_PREFIX, msg, GetErrorText(err)))
#define PCCRD_DebugMessage(msg)      STPCCRD_Print(("%s: %s\n", PCCRD_DEBUG_PREFIX, msg))

/* Test success indicator */
#define PCCRD_TestPassed(x) x.NumberPassed++; PCCRD_DebugPrintf(("Result: **** PASS ****\n"))
#define PCCRD_TestFailed(x,msg,reason) x.NumberFailed++; PCCRD_DebugPrintf(("Result: !!!! FAIL !!!! %s (%s)\n", msg, reason))

#ifdef __cplusplus
}
#endif
#endif /* __PCCRD_TEST_H */

/* End of header */
