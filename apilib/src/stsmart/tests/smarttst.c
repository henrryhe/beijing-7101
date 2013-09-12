/*****************************************************************************
File Name   : smarttst.c

Description : Test harness for the STSMART driver for STi55XX.

Copyright (C) 2006 STMicroelectronics

Revision History :

    06/04/00    Re-structured for porting to different platforms.
                Added STi5508 support.
    10/11/00    Added tests for T=1
    29/03/05    Ported to STi7100(ST40) and STi5301(ST200).

Reference   :

ST API Definition "SMARTCARD Driver API" DVD-API-11
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>              /* C libs */
#include <string.h>
#include <stdlib.h>

#if defined(ST_OSLINUX)
#include <unistd.h>
/*#include <linux/pthread.h>*/
#endif
#if defined(ST_OSLINUX)
#define STTBX_Print(x) printf x
#include "compat.h"
/*#include "linux_map.h"*/
#endif
#include "sttbx.h"
#include "stsys.h"

#include "stdevice.h"
#include "stddefs.h"

#include "stpio.h"

#include "stevt.h"
#include "stsmart.h"            /* STAPI includes */

#if defined(ST_OSLINUX)
#include "../stuart.h"
#endif

#include "stboot.h"
#include "stcommon.h"

#include "dslib.h"              /* Local includes */
#include "smarttst.h"


#if defined(ST_5105) || defined(ST_7100) || defined(ST_8010) || defined(ST_5100) || defined(ST_7109) \
|| defined(ST_5107)  || defined(ST_7200)
#include "stsys.h"
#endif
#include "stos.h"

#if defined(ST_OSLINUX)
#define SMART_NUMBER_CLIENTS 4
#endif

/* Private types/constants ------------------------------------------------ */


void DeviceConfig(void);

/* Private variables ------------------------------------------------------ */

/* Defined in smarttst.h now; this means where no test harness changes
 * are required (e.g. new derivative chip), any changes occur only in
 * that file.
 */
extern const U8 Revision[];

#if defined(ST_OSLINUX)
static pthread_t task_multiclients[SMART_NUMBER_CLIENTS];
/*static void *myParams_multiclients_p=NULL;*/
#endif
/*
#if defined(ST_OSLINUX)
static pthread_t task_stackusage;
static pthread_attr_t *tattr_stackusage_p=NULL;
static void *myParams_stackusage_p=NULL;
#endif
*/
#if defined(ST_OSLINUX) /*STEVT,STPOLL related*/
/*static      STSMART_EventType_t LastEvent1, LastEvent2;*/
pthread_t   STSMART_EventTask;
#endif

/* For storing global clock information */
static ST_ClockInfo_t ClockInfo;

/* Error message look up table */

#define ST_ERROR_UNKNOWN ((U32)-1)

static SMART_ErrorMessage SMART_ErrorLUT[] =
{
    /* STUART */
    { STUART_ERROR_OVERRUN, "STUART_ERROR_OVERRUN" },
    { STUART_ERROR_PARITY, "STUART_ERROR_PARITY" },
    { STUART_ERROR_FRAMING, "STUART_ERROR_FRAMING" },
    { STUART_ERROR_ABORT, "STUART_ERROR_ABORT" },
    { STUART_ERROR_RETRIES, "STUART_ERROR_RETRIES" },
    { STUART_ERROR_PIO, "STUART_ERROR_PIO" },

    /* STSMART */
    { STSMART_ERROR_WRONG_TS_CHAR, "STSMART_ERROR_WRONG_TS_CHAR" },
    { STSMART_ERROR_TOO_MANY_RETRIES, "STSMART_ERROR_TOO_MANY_RETRIES" },
    { STSMART_ERROR_OVERRUN, "STSMART_ERROR_OVERRUN" },
    { STSMART_ERROR_PARITY, "STSMART_ERROR_PARITY" },
    { STSMART_ERROR_FRAME, "STSMART_ERROR_FRAME" },
    { STSMART_ERROR_PTS_NOT_SUPPORTED, "STSMART_ERROR_PTS_NOT_SUPPORTED" },
    { STSMART_ERROR_NO_ANSWER, "STSMART_ERROR_NO_ANSWER" },
    { STSMART_ERROR_INVALID_STATUS_BYTE, "STSMART_ERROR_INVALID_STATUS_BYTE" },
    { STSMART_ERROR_INCORRECT_REFERENCE, "STSMART_ERROR_INCORRECT_REFERENCE" },
    { STSMART_ERROR_INCORRECT_LENGTH, "STSMART_ERROR_INCORRECT_LENGTH" },
    { STSMART_ERROR_UNKNOWN, "STSMART_ERROR_UNKNOWN" },
    { STSMART_ERROR_NOT_INSERTED, "STSMART_ERROR_NOT_INSERTED" },
    { STSMART_ERROR_NOT_RESET, "STSMART_ERROR_NOT_RESET" },
    { STSMART_ERROR_RESET_FAILED, "STSMART_ERROR_RESET_FAILED" },
    { STSMART_ERROR_INVALID_PROTOCOL, "STSMART_ERROR_INVALID_PROTOCOL" },
    { STSMART_ERROR_INVALID_CLASS, "STSMART_ERROR_INVALID_CLASS" },
    { STSMART_ERROR_INVALID_CODE, "STSMART_ERROR_INVALID_CODE" },
    { STSMART_ERROR_IN_PTS_CONFIRM, "STSMART_ERROR_IN_PTS_CONFIRM" },
    { STSMART_ERROR_ABORTED, "STSMART_ERROR_ABORTED" },
    { STSMART_ERROR_EVT_REGISTER, "STSMART_ERROR_EVT_REGISTER" },
    { STSMART_ERROR_EVT_UNREGISTER, "STSMART_ERROR_EVT_UNREGISTER" },
    { STSMART_ERROR_CARD_ABORTED, "STSMART_ERROR_CARD_ABORTED" },
    { STSMART_ERROR_BLOCK_RETRIES, "STSMART_ERROR_BLOCK_RETRIES" },
    { STSMART_ERROR_BLOCK_TIMEOUT, "STSMART_ERROR_BLOCK_TIMEOUT" },
    { STSMART_ERROR_CARD_VPP, "STSMART_ERROR_CARD_VPP "},

    /* STAPI */
    { ST_NO_ERROR, "ST_NO_ERROR" },
    { ST_ERROR_NO_MEMORY, "ST_ERROR_NO_MEMORY" },
    { ST_ERROR_INTERRUPT_INSTALL, "ST_ERROR_INTERRUPT_INSTALL" },
    { ST_ERROR_ALREADY_INITIALIZED, "ST_ERROR_ALREADY_INITIALIZED" },
    { ST_ERROR_UNKNOWN_DEVICE, "ST_ERROR_UNKNOWN_DEVICE" },
    { ST_ERROR_BAD_PARAMETER, "ST_ERROR_BAD_PARAMETER" },
    { ST_ERROR_OPEN_HANDLE, "ST_ERROR_OPEN_HANDLE" },
    { ST_ERROR_NO_FREE_HANDLES, "ST_ERROR_NO_FREE_HANDLES" },
    { ST_ERROR_INVALID_HANDLE, "ST_ERROR_INVALID_HANDLE" },
    { ST_ERROR_INTERRUPT_UNINSTALL, "ST_ERROR_INTERRUPT_UNINSTALL" },
    { ST_ERROR_TIMEOUT, "ST_ERROR_TIMEOUT" },
    { ST_ERROR_FEATURE_NOT_SUPPORTED, "ST_ERROR_FEATURE_NOT_SUPPORTED" },
    { ST_ERROR_DEVICE_BUSY, "ST_ERROR_DEVICE_BUSY" },
    { ST_ERROR_UNKNOWN, "ST_ERROR_UNKNOWN" } /* Terminator */
};

/* Device names -- used for all tests */
static ST_DeviceName_t SmartDeviceName[MAX_SC+1];
static ST_DeviceName_t UartDeviceName[MAX_ASC+1];
static ST_DeviceName_t PioDeviceName[MAX_PIO+1];
#ifdef STSMART_USE_STEVT
static ST_DeviceName_t EVTDeviceName[MAX_EVT+1];
#endif

/* Event handler init params */
static STEVT_InitParams_t EVTInitParams;

/* Smartcard Init params */
static STSMART_InitParams_t SmartInitParams[MAX_SC];

#ifndef ST_OSLINUX
/* UART Init parameters */
static STUART_InitParams_t UartInitParams[MAX_ASC];
#endif

/* PIO Init params */
static STPIO_InitParams_t PioInitParams[MAX_ACTUAL_PIO];


/* Global semaphore used in callbacks */
static semaphore_t *EventSemaphore_p;
static semaphore_t *Lock_p;

static STSMART_EventType_t LastEvent;
static ST_ErrorCode_t LastError;

#define AWAIT_ANY_EVENT         ((U32)-1)

/* Encapsulates events that come from the smartcard
 * driver.  We must maintain a queue of events as they tend to be
 * generated too quickly for the caller to process them.
 */
typedef struct {
    STSMART_Handle_t Handle;
    STSMART_EventType_t Event;
    ST_ErrorCode_t ErrorCode;
} SMART_Notify_t;

/* Size of notify event queue */
#define MQUEUE_SIZE 50

/* Notify queue */
static SMART_Notify_t MsgQueue[MQUEUE_SIZE];
static U8 WriteOffset = 0;
static U8 ReadOffset = 0;

/* Current position through command-response sequences */
static U8 ResponseNumber = 0;
static U32 T1Mismatch = FALSE;
T1Test_t T1Test;
/* Whether the driver should use the 'fake' routines or not */
U8 UseSim = 0;

#define FlushMessageQueue()    WriteOffset = ReadOffset; \
                   while ( STOS_SemaphoreWaitTimeOut(  \
                               EventSemaphore_p, TIMEOUT_IMMEDIATE ) == 0 ) ;


/* Declarations for memory partitions */
#if !defined(ST_OSLINUX)
#if !defined(ST_OS21)

/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

#define                 NCACHE_MEMORY_SIZE          0x80000
static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;
#pragma ST_section      ( external_block, "system_section")

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

#if defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) \
|| defined(ST_7100) || defined(ST_5301) || defined(ST_8010) || defined(ST_7109) \
|| defined(ST_5107) || defined(ST_7200)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif


#else /* define ST_OS21 */

#define                 SYSTEM_MEMORY_SIZE          0x400000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#endif
#else
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;
#endif /*all memory partition info are commented out for ST_OSLINUX*/

/* Private function prototypes -------------------------------------------- */

static ST_ErrorCode_t GlobalInit(void);
static ST_ErrorCode_t GlobalTerm(void);
char *SMART_EventToString(STSMART_EventType_t Event);
U32 ShowProgress(U32 Now, U32 Total, U32 Last);

#if defined(TEST_INSERT_REMOVE)         /* insrtrm.lku */

static SMART_TestResult_t SmartInsertRemove(SMART_TestParams_t *Params_p);

#elif defined(TEST_DS_CARD)             /* dscard.lku */

static SMART_TestResult_t SmartDS(SMART_TestParams_t *Params_p);

#elif defined(TEST_T1_CARD_SIM)

static SMART_TestResult_t SmartT1Sim(SMART_TestParams_t *Params_p);

static U8 HexToU8(const char *string);
BOOL T1TestLoader(FILE *TestFile, T1Test_t *Test_p);
static void GenerateLRC(U8 *Buffer_p, U32 Count, U8 *LRC_p);

#elif defined(TEST_T1_CARD_SCOPE)

static SMART_TestResult_t SmartT1Scope(SMART_TestParams_t *Params_p);

#else                                   /* core.lku */

static SMART_TestResult_t SmartSimple(SMART_TestParams_t *Params_p);
static SMART_TestResult_t SmartEvents(SMART_TestParams_t *Params_p);

#if defined(SMART_MULTIPLE_RESET)
static SMART_TestResult_t SmartMultipleReset(SMART_TestParams_t *Params_p);
#endif

#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5100) && !defined(ST_5301) \
&& !defined(ST_5107)
static SMART_TestResult_t SmartMultipleClients(SMART_TestParams_t *Params_p);
#endif
static SMART_TestResult_t SmartAPI(SMART_TestParams_t *Params_p);
static SMART_TestResult_t SmartRawAPI(SMART_TestParams_t *Params_p);
#if !defined(ST_OSLINUX)
static SMART_TestResult_t SmartStackUsage(SMART_TestParams_t *Params_p);
#endif
#if defined(UART_F_PRESENT)
static SMART_TestResult_t SmartTestGuardTime(SMART_TestParams_t *Params_p);
#endif

/* Global reset count variable */
#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5301) && !defined(ST_5100) \
&& !defined(ST_5107)
static U32 ResetCount;
#endif

/* Used for stack tests */
static U8 StackTestAnswer[33];
static U32 StackTestAnswerLength;

#endif

/* Test table */
static SMART_TestEntry_t TestTable[] =
{
#if defined(TEST_INSERT_REMOVE)
    {
        SmartInsertRemove,
        "Test insert/remove events",
        1,
        SC_DEFAULT_SLOT
    },
#if !defined(mb275) && !defined(mb400)           /* Has only one slot */
    {
        SmartInsertRemove,
        "Test insert/remove events",
        1,
        SC_SLOT_1
    },
#endif
#elif defined(TEST_DS_CARD)
    {
        SmartDS,
        "Tests tailored for use with DS Smartcards",
        1,
        SC_DEFAULT_SLOT
    },
#elif defined(TEST_T1_CARD_SIM)

    {
        SmartT1Sim,
        "Tests tailored for T=1 Smartcards (loader)",
        1,
        SC_DEFAULT_SLOT
    },
#elif defined(TEST_T1_CARD_SCOPE)
    {
        SmartT1Scope,
        "Tests tailored for T=1 Smartcards (scope)",
        1,
        SC_DEFAULT_SLOT
    },
#else
#if !defined(ST_OSLINUX)
    {
        SmartStackUsage,
        "Testing stack and data space usage",
        1,
        SC_DEFAULT_SLOT
    },
#endif
    {
        SmartSimple,
        "Typical session with the smartcard API",
        1,
        SC_DEFAULT_SLOT
    },
    {
        SmartAPI,
        "Tests robustness and validity of API",
        1,
        SC_DEFAULT_SLOT
    },
#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5301) && !defined(ST_5100) \
&& !defined(ST_5107)
    {
        SmartMultipleClients,
        "Tests for multiple clients",
        1,
        0
    },
#endif
    {
        SmartEvents,
        "Testing of smartcard event notification",
        1,
        SC_DEFAULT_SLOT
    },
    {
        SmartRawAPI,
        "Testing of raw API functions",
        1,
        SC_DEFAULT_SLOT
    },
#if defined(SMART_MULTIPLE_RESET)
    {
        SmartMultipleReset,
        "Tests for multiple reset on single SC",
        1,
        SC_DEFAULT_SLOT
    },
#endif

#if defined(UART_F_PRESENT)
    {
        SmartTestGuardTime,
        "Check chip can support 256-guardtime",
        1,
        SC_DEFAULT_SLOT
    },
#endif
#endif
    { 0, "", 0, 0 }
};

/* Function definitions --------------------------------------------------- */

int main(int argc, char *argv[])
{
    ST_ErrorCode_t error;
    SMART_TestResult_t Total = TEST_RESULT_ZERO, Result;
    SMART_TestEntry_t *Test_p;
    U32 Section = 1;
    SMART_TestParams_t TestParams;
#if !defined(ST_OSLINUX)
    STTBX_InitParams_t TBXInitParams;
#endif
#if !defined(ST_OSLINUX)
    STBOOT_InitParams_t BootParams;
    STBOOT_TermParams_t BootTerm;

#if defined(DISABLE_DCACHE)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { NULL, NULL }                  /* End */
    };
#elif defined(CACHEABLE_BASE_ADDRESS)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS },
        { NULL, NULL }                  /* End */
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40020000, (U32 *)0x407FFFFF },
        { NULL, NULL }                  /* End */
    };

#elif defined(ST_5528)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        {(U32 *) 0x407FFFFF, (U32 *) 0x7FFFFFFF},    /* Cacheable */
        { NULL, NULL}                                 /* End */
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF },
        { NULL, NULL }                  /* End */
    };
#endif

#if defined(ST_OS21)
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));
#elif defined(ST_OS20)
    /* Create memory partitions */
    partition_init_simple(&the_internal_partition,
                          (U8*)internal_block,
                          sizeof(internal_block)
                         );
    partition_init_heap(&the_system_partition,
                        (U8*)external_block,
                        sizeof(external_block));
    partition_init_heap(&the_ncache_partition,
                        ncache_memory,
                        sizeof(ncache_memory));

    /* Avoids compiler warnings about not being used */
    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;
#if defined(ST_5528) || defined(ST_5100) ||defined(ST_7710) || defined(ST_5105) \
|| defined(ST_7100) || defined(ST_5301) || defined(ST_8010) || defined(ST_7109) \
|| defined(ST_5107) || defined(ST_7200)
    data_section[0] = 0;
#endif

#endif

    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
    BootParams.ICacheEnabled = ICACHE;
    BootParams.DCacheMap = DCacheMap;
    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

#if defined(ST_OS21)
    BootParams.TimeslicingEnabled = TRUE;
#endif

    /* Constant, defined in include; set automatically for
     * board configuration
     */
    BootParams.MemorySize = SDRAM_SIZE;

    /* Initialize the boot driver */
    error = STBOOT_Init( "boot", &BootParams );
    if (error != ST_NO_ERROR)
    {
        printf("Unable to initialize boot %d\n", error);
        return 0;
    }
#endif /* ST_OSLINUX */

#if !defined(DISABLE_TBX)
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p      = system_partition;
    error = STTBX_Init( "tbx", &TBXInitParams );
#endif

    /* Obtain subsystem clock speeds */
    ST_GetClockInfo(&ClockInfo);

    /* Perform any device-specific top-level configuration */
    DeviceConfig();

    SMART_DebugMessage("**************************************************");
    SMART_DebugMessage("                STSMART Test Harness              ");
    SMART_DebugPrintf(("Driver Revision: %s\n", STSMART_GetRevision()));
    SMART_DebugPrintf(("Test Harness Revision: %s\n", Revision));
    SMART_DebugPrintf(("Clock Speed: %d\n", CLOCK_CPU));
    SMART_DebugPrintf(("Comms Block: %d\n", CLOCK_COMMS));
    SMART_DebugMessage("**************************************************");

    error = GlobalInit();
    if (error != ST_NO_ERROR)
    {
        return 0;
    }

    Test_p = TestTable;
    while (Test_p->TestFunction != NULL)
    {
        U8 Repeat = Test_p->RepeatCount;
        while (Repeat > 0)
        {
            SMART_DebugMessage("**************************************************");
            SMART_DebugPrintf(("SECTION %d - %s\n", Section,
                              Test_p->TestInfo));
            SMART_DebugMessage("**************************************************");

            /* Set up parameters for test function */
            TestParams.Ref = Section;
            TestParams.Slot = Test_p->Slot;

            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            FlushMessageQueue();

            /* Update counters */
            Total.NumberPassed += Result.NumberPassed;
            Total.NumberFailed += Result.NumberFailed;
            Repeat--;
        }

        Test_p++;
        Section++;
    }

    SMART_DebugMessage("**************************************************");
    GlobalTerm();

    /* Output running analysis */
    SMART_DebugMessage("**************************************************");
    SMART_DebugPrintf(("PASSED: %d\n", Total.NumberPassed));
    SMART_DebugPrintf(("FAILED: %d\n", Total.NumberFailed));
    SMART_DebugMessage("**************************************************");

#if !defined(ST_OSLINUX)
    STBOOT_Term("boot", &BootTerm);
#endif

    return 0;
}

/* Test harness tests ----------------------------------------------------- */

#ifdef STSMART_USE_STEVT

static STEVT_Handle_t EVTHandle[MAX_EVT];

#define CallbackNotifyFunction  NULL

void EvtNotifyFunction(STEVT_CallReason_t Reason,
                       STEVT_EventConstant_t Event,
                       const void *Params_p)
{
    STSMART_EvtParams_t *EvtParams_p = (STSMART_EvtParams_t *)Params_p;
    ST_ErrorCode_t ErrorCode;

    /* Obtain event / error from notify params */
    ErrorCode = EvtParams_p->Error;

    STOS_SemaphoreWait(Lock_p);          /* Stop re-entrancy */
    LastEvent = Event;
    LastError = ErrorCode;
    MsgQueue[WriteOffset].Event = Event;
    MsgQueue[WriteOffset].ErrorCode = ErrorCode;
    if (++WriteOffset >= MQUEUE_SIZE)
    {
        WriteOffset = 0;
    }
    STOS_SemaphoreSignal(Lock_p);
    STOS_SemaphoreSignal(EventSemaphore_p);   /* Signal 'new event' */
}

#else

#define CallbackNotifyFunction  DirectNotifyFunction

void DirectNotifyFunction(STSMART_Handle_t Handle, STSMART_EventType_t Event,
                          ST_ErrorCode_t ErrorCode)
{
    STOS_SemaphoreWait(Lock_p);          /* Stop re-entrancy */
    LastEvent = Event;
    LastError = ErrorCode;
    MsgQueue[WriteOffset].Event = Event;
    MsgQueue[WriteOffset].ErrorCode = ErrorCode;
    if (++WriteOffset >= MQUEUE_SIZE)
    {
        WriteOffset = 0;
    }
    STOS_SemaphoreSignal(Lock_p);
    STOS_SemaphoreSignal(EventSemaphore_p);   /* Signal 'new event' */
}

#endif

/*****************************************************************************
AwaitNotifyEvent()
Description:
    Provides a convenient way of awaiting on IEEE 1284 events.
    The caller may specify a specific event to wait for or
    'AWAIT_ANY_EVENT'.
Parameters:
    NeedEvent,      I1284 event type or AWAIT_ANY_EVENT.
    Event_p,        pointer to area to store event that has occurred.
    Error_p,        pointer to area to store error associated with event.
Return Value:
    None.
See Also:
    NotifyFunction()
*****************************************************************************/

void AwaitNotifyEvent(STSMART_EventType_t NeedEvent,
                      STSMART_EventType_t *Event_p,
                      ST_ErrorCode_t *Error_p)
{
    STSMART_EventType_t Ev;
    do
    {
#if defined(SMARTTST_DEBUG)
        printf("Entering AwaitNotifyEvent(),b4 STOS_SemaphoreWait(EventSemaphore_p),NeedEvent=0x%x\n",NeedEvent);
#endif
        STOS_SemaphoreWait(EventSemaphore_p);
        Ev = MsgQueue[ReadOffset].Event;
#if defined(SMARTTST_DEBUG)
        printf("In AwaitNotifyEvent(),Ev=0x%x,Event_p=0x%x,Error_p=0x%x\n", Ev, Event_p,Error_p);
#endif
        if (Event_p != NULL)
        {
#if defined(SMARTTST_DEBUG)
            printf("In AwaitNotifyEvent(),Ev=0x%x, *Event_p=0x%x\n", Ev,MsgQueue[ReadOffset].Event);
#endif
            *Event_p = MsgQueue[ReadOffset].Event;
        }
        if (Error_p != NULL)
        {
#if defined(SMARTTST_DEBUG)
             printf("In AwaitNotifyEvent(),Ev=0x%x, *Error_p=0x%x\n", Ev,MsgQueue[ReadOffset].ErrorCode);
#endif
            *Error_p = MsgQueue[ReadOffset].ErrorCode;
        }
        if (++ReadOffset >= MQUEUE_SIZE)
        {
            ReadOffset = 0;
        }
    } while (NeedEvent != Ev &&
             NeedEvent != AWAIT_ANY_EVENT);
#if defined(SMARTTST_DEBUG)
    printf("In AwaitNotifyEvent(),reach the end of it!\n");
#endif
}



static ST_ErrorCode_t GlobalInit(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;
    ST_DeviceName_t DeviceName;

    /* Automatically generate device names */

    for (i = 0; i < MAX_PIO; i++)
    {
        sprintf((char *)DeviceName, "STPIO_%d", i);
        strcpy((char *)PioDeviceName[i], (char *)DeviceName);
    }
    PioDeviceName[i][0] = 0;
    PioDeviceName[PIO_DEVICE_NOT_USED][0] = 0;

    for (i = 0; i < MAX_ASC; i++)       /* UART */
    {
        sprintf((char *)DeviceName, "STUART_%d", i);
        strcpy((char *)UartDeviceName[i], (char *)DeviceName);
    }
    UartDeviceName[i][0] = 0;

    for (i = 0; i < MAX_SC; i++)        /* SC */
    {
        sprintf((char *)DeviceName, "STSMART_%d", i);
        strcpy((char *)SmartDeviceName[i], (char *)DeviceName);
    }
    SmartDeviceName[i][0] = 0;

#ifdef STSMART_USE_STEVT
    for (i = 0; i < MAX_EVT; i++)       /* EVT */
    {
        sprintf((char *)DeviceName, "STEVT_%d", i);
        strcpy((char *)EVTDeviceName[i], (char *)DeviceName);
    }
    EVTDeviceName[i][0] = 0;
#endif

#ifndef ST_OSLINUX
    /* Set UART initialization params */
    UartInitParams[0].UARTType = ASC_DEVICE_TYPE;
    UartInitParams[0].DriverPartition = (ST_Partition_t *)system_partition;
    UartInitParams[0].BaseAddress = (U32 *)ASC_0_BASE_ADDRESS;
    UartInitParams[0].InterruptNumber = ASC_0_INTERRUPT;
    UartInitParams[0].InterruptLevel = ASC_0_INTERRUPT_LEVEL;
    UartInitParams[0].ClockFrequency = CLOCK_COMMS;
    UartInitParams[0].SwFIFOEnable = TRUE;
    UartInitParams[0].FIFOLength = 256;
    strcpy(UartInitParams[0].RXD.PortName, PioDeviceName[ASC_0_RXD_DEV]);
    UartInitParams[0].RXD.BitMask = ASC_0_RXD_BIT;
    strcpy(UartInitParams[0].TXD.PortName, PioDeviceName[ASC_0_TXD_DEV]);
    UartInitParams[0].TXD.BitMask = ASC_0_TXD_BIT;
    strcpy(UartInitParams[0].RTS.PortName, PioDeviceName[ASC_0_RTS_DEV]);
    UartInitParams[0].RTS.BitMask = ASC_0_RTS_BIT;
    strcpy(UartInitParams[0].CTS.PortName, PioDeviceName[ASC_0_CTS_DEV]);
    UartInitParams[0].CTS.BitMask = ASC_0_CTS_BIT;
    UartInitParams[0].DefaultParams = NULL;

#if (MAX_ASC > 1)
    UartInitParams[1].UARTType = ASC_DEVICE_TYPE;
    UartInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
    UartInitParams[1].BaseAddress = (U32 *)ASC_1_BASE_ADDRESS;
    UartInitParams[1].InterruptNumber = ASC_1_INTERRUPT;
    UartInitParams[1].InterruptLevel = ASC_1_INTERRUPT_LEVEL;
    UartInitParams[1].ClockFrequency = CLOCK_COMMS;
    UartInitParams[1].SwFIFOEnable = TRUE;
    UartInitParams[1].FIFOLength = 256;
    strcpy(UartInitParams[1].RXD.PortName, PioDeviceName[ASC_1_RXD_DEV]);
    UartInitParams[1].RXD.BitMask = ASC_1_RXD_BIT;
    strcpy(UartInitParams[1].TXD.PortName, PioDeviceName[ASC_1_TXD_DEV]);
    UartInitParams[1].TXD.BitMask = ASC_1_TXD_BIT;
    strcpy(UartInitParams[1].RTS.PortName, PioDeviceName[ASC_1_RTS_DEV]);
    UartInitParams[1].RTS.BitMask = ASC_1_RTS_BIT;
    strcpy(UartInitParams[1].CTS.PortName, PioDeviceName[ASC_1_CTS_DEV]);
    UartInitParams[1].CTS.BitMask = ASC_1_CTS_BIT;
    UartInitParams[1].DefaultParams = NULL;
#endif

#if (MAX_ASC > 2)
    UartInitParams[2].UARTType = ASC_DEVICE_TYPE;
    UartInitParams[2].DriverPartition = (ST_Partition_t *)system_partition;
    UartInitParams[2].BaseAddress = (U32 *)ASC_2_BASE_ADDRESS;
    UartInitParams[2].InterruptNumber = ASC_2_INTERRUPT;
    UartInitParams[2].InterruptLevel = ASC_2_INTERRUPT_LEVEL;
    UartInitParams[2].ClockFrequency = CLOCK_COMMS;
    UartInitParams[2].SwFIFOEnable = TRUE;
    UartInitParams[2].FIFOLength = 256;
    strcpy(UartInitParams[2].RXD.PortName, PioDeviceName[ASC_2_RXD_DEV]);
    UartInitParams[2].RXD.BitMask = ASC_2_RXD_BIT;
    strcpy(UartInitParams[2].TXD.PortName, PioDeviceName[ASC_2_TXD_DEV]);
    UartInitParams[2].TXD.BitMask = ASC_2_TXD_BIT;
    strcpy(UartInitParams[2].RTS.PortName, PioDeviceName[ASC_2_RTS_DEV]);
    UartInitParams[2].RTS.BitMask = ASC_2_RTS_BIT;
    strcpy(UartInitParams[2].CTS.PortName, PioDeviceName[ASC_2_CTS_DEV]);
    UartInitParams[2].CTS.BitMask = ASC_2_CTS_BIT;
    UartInitParams[2].DefaultParams = NULL;
#endif

#if (MAX_ASC > 3)
    UartInitParams[3].UARTType = ASC_DEVICE_TYPE;
    UartInitParams[3].DriverPartition = (ST_Partition_t *)system_partition;
    UartInitParams[3].BaseAddress = (U32 *)ASC_3_BASE_ADDRESS;
    UartInitParams[3].InterruptNumber = ASC_3_INTERRUPT;
    UartInitParams[3].InterruptLevel = ASC_3_INTERRUPT_LEVEL;
    UartInitParams[3].ClockFrequency = CLOCK_COMMS;
    UartInitParams[3].SwFIFOEnable = TRUE;
    UartInitParams[3].FIFOLength = 256;
    strcpy(UartInitParams[3].RXD.PortName, PioDeviceName[ASC_3_RXD_DEV]);
    UartInitParams[3].RXD.BitMask = ASC_3_RXD_BIT;
    strcpy(UartInitParams[3].TXD.PortName, PioDeviceName[ASC_3_TXD_DEV]);
    UartInitParams[3].TXD.BitMask = ASC_3_TXD_BIT;
    strcpy(UartInitParams[3].RTS.PortName, PioDeviceName[ASC_3_RTS_DEV]);
    UartInitParams[3].RTS.BitMask = ASC_3_RTS_BIT;
    strcpy(UartInitParams[3].CTS.PortName, PioDeviceName[ASC_3_CTS_DEV]);
    UartInitParams[3].CTS.BitMask = ASC_3_CTS_BIT;
    UartInitParams[3].DefaultParams = NULL;
#endif
#endif/* ST_OSLINUX*/

    /* Setup PIO initialization parameters */
    PioInitParams[0].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;

    PioInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[1].BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
    PioInitParams[1].InterruptNumber = PIO_1_INTERRUPT;
    PioInitParams[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;

    PioInitParams[2].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[2].BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams[2].InterruptNumber = PIO_2_INTERRUPT;
    PioInitParams[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;

#if (MAX_ACTUAL_PIO >= 4)
    PioInitParams[3].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[3].BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
    PioInitParams[3].InterruptNumber = PIO_3_INTERRUPT;
    PioInitParams[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;
#endif

#if (MAX_ACTUAL_PIO >= 5)
    PioInitParams[4].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[4].BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
#endif

#if (MAX_ACTUAL_PIO >= 6)
    PioInitParams[5].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[5].BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
#endif

#if (MAX_ACTUAL_PIO >= 7)
    PioInitParams[6].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[6].BaseAddress     = (U32 *)PIO_6_BASE_ADDRESS;
    PioInitParams[6].InterruptNumber = PIO_6_INTERRUPT;
    PioInitParams[6].InterruptLevel  = PIO_6_INTERRUPT_LEVEL;
#endif

#if (MAX_ACTUAL_PIO >= 8)
    PioInitParams[7].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[7].BaseAddress     = (U32 *)PIO_7_BASE_ADDRESS;
    PioInitParams[7].InterruptNumber = PIO_7_INTERRUPT;
    PioInitParams[7].InterruptLevel  = PIO_7_INTERRUPT_LEVEL;
#endif


    /* Setup SC initialization parameters */
    strcpy(SmartInitParams[0].UARTDeviceName,
           UartDeviceName[SC_0_ASC_DEV]);

#if defined (STSMART_WARM_RESET)
    SmartInitParams[0].DeviceType = STSMART_ISO_NDS;
#else
    SmartInitParams[0].DeviceType = STSMART_ISO;
#endif
    SmartInitParams[0].DriverPartition = (ST_Partition_t *)system_partition;
#if defined(ST_OSLINUX)
    SMART_DebugPrintf(("In User Space SmartInitParams[0].DriverPartition=0x%x\n", (unsigned int)SmartInitParams[0].DriverPartition));
#endif
    SmartInitParams[0].BaseAddress = (U32 *)SMARTCARD0_BASE_ADDRESS;

    strcpy(SmartInitParams[0].ClkGenExtClk.PortName,PioDeviceName[SC_0_EXTCLK_DEV]);
    strcpy(SmartInitParams[0].Clk.PortName,PioDeviceName[SC_0_CLK_DEV]);
    strcpy(SmartInitParams[0].Reset.PortName,PioDeviceName[SC_0_RST_DEV]);
    strcpy(SmartInitParams[0].CmdVcc.PortName,PioDeviceName[SC_0_VCC_DEV]);
    strcpy(SmartInitParams[0].CmdVpp.PortName,PioDeviceName[SC_0_VPP_DEV]);
    strcpy(SmartInitParams[0].Detect.PortName,PioDeviceName[SC_0_DETECT_DEV]);
    SmartInitParams[0].ClkGenExtClk.BitMask = SC_0_EXTCLK_BIT;
    SmartInitParams[0].Clk.BitMask = SC_0_CLK_BIT;
    SmartInitParams[0].Reset.BitMask = SC_0_RST_BIT;
    SmartInitParams[0].CmdVcc.BitMask = SC_0_VCC_BIT;
    SmartInitParams[0].CmdVpp.BitMask = SC_0_VPP_BIT;
    SmartInitParams[0].Detect.BitMask = SC_0_DETECT_BIT;
    SmartInitParams[0].ClockSource = SC_0_CLK_SRC;
    SmartInitParams[0].ClockFrequency = SC_0_CLK_SPD;
    SmartInitParams[0].MaxHandles = 32;
#ifdef STSMART_USE_STEVT
    strcpy(SmartInitParams[0].EVTDeviceName,EVTDeviceName[EVT_DEVICE_0]);
#else
    SmartInitParams[0].EVTDeviceName[0] = 0;
#endif

#if(MAX_SC >= 2)
    strcpy(SmartInitParams[1].UARTDeviceName,
           UartDeviceName[SC_1_ASC_DEV]);
    SmartInitParams[1].DeviceType = STSMART_ISO;
    SmartInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
    SmartInitParams[1].BaseAddress = (U32 *)SMARTCARD1_BASE_ADDRESS;

/* #ifdef ST_OSLINUX */
#if 0
    SmartInitParams[1].ClkGenExtClk.PortNum = 1;
    SmartInitParams[1].Clk.PortNum = 1;
    SmartInitParams[1].Reset.PortNum = 1;
    SmartInitParams[1].CmdVcc.PortNum = 1;
    SmartInitParams[1].CmdVpp.PortNum = 1;
    SmartInitParams[1].Detect.PortNum = 1;
    SmartInitParams[1].ClkGenExtClk.PinNum = 2;
    SmartInitParams[1].Clk.PinNum = 3;
    SmartInitParams[1].Reset.PinNum = 4;
    SmartInitParams[1].CmdVcc.PinNum = 5;
    SmartInitParams[1].CmdVpp.PinNum = 6;
    SmartInitParams[1].Detect.PinNum = 7;
#else
    strcpy(SmartInitParams[1].ClkGenExtClk.PortName,PioDeviceName[SC_1_EXTCLK_DEV]);
    strcpy(SmartInitParams[1].Clk.PortName,PioDeviceName[SC_1_CLK_DEV]);
    strcpy(SmartInitParams[1].Reset.PortName,PioDeviceName[SC_1_RST_DEV]);
    strcpy(SmartInitParams[1].CmdVcc.PortName,PioDeviceName[SC_1_VCC_DEV]);
    strcpy(SmartInitParams[1].CmdVpp.PortName,PioDeviceName[SC_1_VPP_DEV]);
    strcpy(SmartInitParams[1].Detect.PortName,PioDeviceName[SC_1_DETECT_DEV]);
    SmartInitParams[1].ClkGenExtClk.BitMask = SC_1_EXTCLK_BIT;
    SmartInitParams[1].Clk.BitMask = SC_1_CLK_BIT;
    SmartInitParams[1].Reset.BitMask = SC_1_RST_BIT;
    SmartInitParams[1].CmdVcc.BitMask = SC_1_VCC_BIT;
    SmartInitParams[1].CmdVpp.BitMask = SC_1_VPP_BIT;
    SmartInitParams[1].Detect.BitMask = SC_1_DETECT_BIT;
#endif
    SmartInitParams[1].ClockSource = SC_1_CLK_SRC;
    SmartInitParams[1].ClockFrequency = SC_1_CLK_SPD;
    SmartInitParams[1].MaxHandles = 32;
#ifdef STSMART_USE_STEVT
    strcpy(SmartInitParams[1].EVTDeviceName,
           EVTDeviceName[EVT_DEVICE_1]);
#else
    SmartInitParams[1].EVTDeviceName[0] = 0;
#endif
#endif

    /*******************************************************************
    Initialize all devices
    *******************************************************************/

    /* EVT */
#ifdef STSMART_USE_STEVT
    /* Event handler initialization */
    EVTInitParams.MemoryPartition = system_partition;
    EVTInitParams.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    EVTInitParams.EventMaxNum = STSMART_NUMBER_EVENTS * 2;
    EVTInitParams.ConnectMaxNum = 3;
    EVTInitParams.SubscrMaxNum = STSMART_NUMBER_EVENTS * 2;

    for (i = 0; i < MAX_EVT; i++)   /* Initialize each event handler */
    {
        error = STEVT_Init(EVTDeviceName[i], &EVTInitParams);
        SMART_DebugError("STEVT_Init()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_DebugMessage("Unable to initialize EVT device");
            return error;
        }
    }
#endif


    /* PIO */
    for (i = 0; i < MAX_ACTUAL_PIO; i++)
    {
        error = STPIO_Init(PioDeviceName[i],
                           &PioInitParams[i]);
        SMART_DebugError("STPIO_Init()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Unable to initialize %s.\n",
                               PioDeviceName[i]));
        }
    }

#if !defined(ST_OSLINUX)

    /* Uart */
    for (i = 0; i < MAX_ASC; i++)
    {
        error = STUART_Init(UartDeviceName[i],
                             &UartInitParams[i]);
        SMART_DebugError("STUART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Unable to initialize %s.\n",
                               UartDeviceName[i]));
        }
    }
#endif

    /* SC */
    for (i = 0; i < MAX_SC; i++)
    {
        error = STSMART_Init(SmartDeviceName[i],
                             &SmartInitParams[i]);
        SMART_DebugError("STSMART_Init()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Unable to initialize %s.\n",
                               SmartDeviceName[i]));
        }
    }

    /* Subscribe to events */
#ifdef STSMART_USE_STEVT
    {
        U8 i;

        STEVT_SubscribeParams_t SubscribeParams =
        {
            EvtNotifyFunction,

        };
        STEVT_OpenParams_t EVTOpenParams;

        for (i = 0; i < MAX_EVT; i++)   /* Initialize each event handler */
        {
            error = STEVT_Open(EVTDeviceName[i], &EVTOpenParams, &EVTHandle[i]);
            SMART_DebugError("STEVT_Open()", error);

            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_CARD_INSERTED,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_CARD_REMOVED,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_CARD_RESET,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_CARD_DEACTIVATED,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_TRANSFER_COMPLETED,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_READ_COMPLETED,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
            error = STEVT_Subscribe(EVTHandle[i],
                                    STSMART_EV_WRITE_COMPLETED,
                                    &SubscribeParams);
            SMART_DebugError("STEVT_Subscribe()", error);
        }
    }
#endif

    /* Setup global semaphores */
    EventSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    Lock_p = STOS_SemaphoreCreateFifo(NULL,1);

    return error;
}

static ST_ErrorCode_t GlobalTerm(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;

    STOS_SemaphoreDelete(NULL,Lock_p);
    STOS_SemaphoreDelete(NULL,EventSemaphore_p);

    /* SC */
    for (i = 0; i < MAX_SC; i++)
    {
        STSMART_TermParams_t TermParams;

        error = STSMART_Term(SmartDeviceName[i],
                             &TermParams);
        SMART_DebugError("STSMART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Unable to terminate %s.\n",
                               SmartDeviceName[i]));
        }
    }

#if !defined(ST_OSLINUX)
    /* UART */

    for (i = 0; i < MAX_ASC; i++)
    {
        STUART_TermParams_t TermParams;

        error = STUART_Term(UartDeviceName[i],
                            &TermParams);
        SMART_DebugError("STUART_Term()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Unable to terminate %s.\n",
                               UartDeviceName[i]));
        }
    }

#endif

    /* PIO */

    for (i = 0; i < MAX_ACTUAL_PIO; i++)
    {
        STPIO_TermParams_t TermParams;

        error = STPIO_Term(PioDeviceName[i],
                           &TermParams);
        SMART_DebugError("STPIO_Term()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Unable to terminate %s.\n",
                               PioDeviceName[i]));
        }
    }


#ifdef STSMART_USE_STEVT
    /* Terminate event handlers */
    {
        U8 i;
        STEVT_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;

        for (i = 0; i < MAX_EVT; i++)
        {
            STEVT_TermParams_t TermParams;
            TermParams.ForceTerminate = TRUE;
            error = STEVT_Term(EVTDeviceName[i], &TermParams);
            SMART_DebugError("STEVT_Term()", error);
            if (error != ST_NO_ERROR)
            {
                SMART_DebugPrintf(("Unable to terminate %s.\n",
                                   EVTDeviceName[i]));
            }
        }
    }
#endif

    return error;
}


/* DeviceConfig()
 *
 * This function performs any necessary top-level configuration
 * for the STSMART test harness to function correctly.
 */
void DeviceConfig(void)
{
#if defined(ST_5105) || defined(ST_5107)

    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F, 0 );
    STSYS_WriteRegDev32LE(CONFIG_MONITOR_G, 0 );
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, 0x02000000);

#elif defined(ST_7200)
#if !defined(ST_OSLINUX)
    STSYS_WriteRegDev32LE(SYSTEM_CONFIG_7,0x1E0  );
#endif

#elif defined(ST_7100) || defined(ST_7109)
#if !defined(ST_OSLINUX)
    STSYS_WriteRegDev32LE(SYSTEM_CONFIG_7,0x1B0  );
#endif

#elif defined(ST_5100)

    STSYS_WriteRegDev32LE(0x20D00014,0x10000 ); /* PIO0-0 into a bi-directional mode */


#elif defined(ST_8010)

    /* For enabling TX on ASC0 */
    STSYS_WriteRegDev32LE(0x50003014,0x20000000 ); /* SYSCONF_CON1B -> bit 61 */

    /* For enabling TX on ASC0 & CLK */
    STSYS_WriteRegDev32LE(0x5000301C,0x00003000 ); /* SYSCONF_CON2B -> bit 44,45 */

#endif

}
#if defined(TEST_INSERT_REMOVE)         /* insrtrm.lku */

/*****************************************************************************
SmartInsertRemove()
Description:
    Test for exhaustively testing event notification for card insert and
    remove.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartInsertRemove(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    STSMART_Status_t Status;
    ST_ErrorCode_t error;
    U8 Index = 0;
    U32 c;

    /* STSMART_Open() */
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = CallbackNotifyFunction;
        OpenParams.BlockingIO = FALSE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto ir_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);

        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugPrintf(("Please insert a card in slot %d"
                               " to proceed.\n", Params_p->Slot));
            for (;;)
            {
                STSMART_EventType_t Event;
                AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
                if (Event == STSMART_EV_CARD_INSERTED)
                {
                    break;
                }
            }
        }
    }

    SMART_DebugPrintf(("%d.%d: Insert/remove card (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure notification is received of "
                       "insertion/removal of a smartcard");
    {
        STSMART_EventType_t ExpectedEvent = STSMART_EV_CARD_REMOVED;
        STSMART_EventType_t Event;
        char MsgInsert[] = "Insert smartcard";
        char MsgRemove[] = "Remove smartcard";
        char *Msg_p = MsgRemove;

        for (c = 0; c < 10; c++)
        {
            SMART_DebugMessage(Msg_p);
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
            if (Event == ExpectedEvent)
            {
                ExpectedEvent = (ExpectedEvent == STSMART_EV_CARD_REMOVED)
                                ? STSMART_EV_CARD_INSERTED :
                                STSMART_EV_CARD_REMOVED;
                Msg_p = (ExpectedEvent == STSMART_EV_CARD_REMOVED) ? MsgRemove :
                        MsgInsert;
            }
            else
            {
                break;
            }
        }

        if (c == 10)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Incorrect event",
                             SMART_EventToString(Event));
        }
    }

ir_end:
    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartInsertRemove() */

#elif defined(TEST_DS_CARD)            /* dscard.lku */

/*****************************************************************************
SmartDS()
Description:
    This routine runs through the functions supplied by dslib for the
    DS smartcard.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartDS(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    STSMART_Status_t Status;
    ST_ErrorCode_t error;

    /* STSMART_Open() */
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto ds_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed...");
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond());
            } while (STSMART_GetStatus(Handle, &Status) == STSMART_ERROR_NOT_INSERTED);
        }
    }

    /* STSMART_Reset() */
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif
        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
            {
                STSMART_Print(("[0x%02x]", Answer[i]));
            }
            STSMART_Print(("\n"));
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
            goto ds_end;
        }
    }

    /* Check the card inserted is a DS card -- otherwise skip the test */
    {
        U8 HistoryBytes[32];
        U32 HistoryLength;
        U8 DsHistoryBytes[] = { 0x24, 0xc2, 0x01, 0x90, 0x00 };

        /* First ensure a DS card is inserted */
        SMART_DebugMessage("Checking smartcard history bytes...");
        STSMART_GetHistory(Handle, HistoryBytes, &HistoryLength);

        if (HistoryLength != 5 ||
            memcmp(HistoryBytes, DsHistoryBytes, 5) != 0)
        {
            SMART_DebugMessage("...not a DS smartcard!!!");
        }
        else
        {
            SMART_DebugMessage("...Ok");
        }
    }

    /* Testing with the DS library */
    {
        U8 Id[2];
        U8 DsStatus[2];
        U8 Buf[255];
        U8 VerifyBuf[255];
        U32 i, j, k = -1;

        Id[0] = 0x5f;
        Id[1] = 0x00;

        SMART_DebugMessage("Starting intensive T=0 I/O test...");
        SMART_DebugPrintf((" "));

        for (j = 0; j < 100; j++)
        {
            error = DS_Select(Handle, Id, DsStatus);

            if (DsStatus[0] != 0x90)
            {
                sprintf((char *)Buf, "SW1/SW2=0x%02x/0x%02x",
                        DsStatus[0], DsStatus[1]);
                SMART_TestFailed(Result, "Select failed ", Buf);
                if (error != ST_NO_ERROR)
                {
                    SMART_DebugPrintf(("STSMART_Transfer() returned %s\n",
                                      SMART_ErrorString(error)));
                }
                break;
            }

            error = DS_ReadBinary(Handle, 0, 100, Buf, DsStatus);

            if (DsStatus[0] != 0x90)
            {
                sprintf((char *)Buf, "SW1/SW2=0x%02x/0x%02x",
                        DsStatus[0], DsStatus[1]);
                SMART_TestFailed(Result, "Read failed ", Buf);
                break;
            }

            /* Invert contents */
            for (i = 0; i < 100; i++)
            {
                Buf[i] = ~Buf[i];
            }

            error = DS_UpdateBinary(Handle, 0, 100, Buf, DsStatus);

            if (DsStatus[0] != 0x90)
            {
                sprintf((char *)Buf, "SW1/SW2=0x%02x/0x%02x",
                        DsStatus[0], DsStatus[1]);
                SMART_TestFailed(Result, "Update failed ", Buf);
                break;
            }

            /* Verify contents were updated */
            error = DS_ReadBinary(Handle, 0, 100, VerifyBuf, DsStatus);

            if (DsStatus[0] != 0x90)
            {
                sprintf((char *)Buf, "SW1/SW2=0x%02x/0x%02x",
                        DsStatus[0], DsStatus[1]);
                SMART_TestFailed(Result, "Read failed ", Buf);
                break;
            }

            /* Verify contents */
            for (i = 0; i < 100 && VerifyBuf[i] == Buf[i]; i++)
                /* Do nothing */;

            if (i != 100)                /* All bytes check? */
            {
                sprintf((char *)Buf, "Byte %d", i);
                SMART_TestFailed(Result, "Byte verification failed", (char*)Buf);
            }

            /* Output progress */
            k = ShowProgress((j+1), 100, k);
        }

        if (j == 100)
        {
            SMART_TestPassed(Result);
        }
    }

ds_end:
    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartDS() */

#elif defined(TEST_T1_CARD_SIM)

/*****************************************************************************
SmartT1Sim()
Description:
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartT1Sim(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    STSMART_Status_t Status;
    ST_ErrorCode_t error;
    FILE *TestFile;
    BOOL okay = FALSE;
    STSMART_Params_t Params;

    /* STSMART_Open() */
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;
        OpenParams.SAD = 1;
        OpenParams.DAD = 2;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto t1_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed...");
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond());
            } while (STSMART_GetStatus(Handle, &Status) == STSMART_ERROR_NOT_INSERTED);
        }
    }

    /* STSMART_Reset() */
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif
        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
            {
                STSMART_Print(("[0x%02x]", Answer[i]));
            }
            STSMART_Print(("\n"));
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }
    }

    /* Check the card inserted is a T=1 card -- otherwise skip the test */
    SMART_DebugPrintf(("Obtaining card params (slot %d)\n", Params_p->Slot));

    error = STSMART_GetParams(Handle, &Params);
    SMART_DebugError("STSMART_GetParams()", error);

    if (error == ST_NO_ERROR)
    {
        SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                           Params.SupportedProtocolTypes));
        SMART_DebugPrintf(("ProtocolChangeable = %s\n",
                           Params.ProtocolTypeChangeable ? "YES" : "NO" ));
        SMART_DebugPrintf(("BaudRate = %d\n",
                           Params.BaudRate));
        SMART_DebugPrintf(("MaxBaudRate = %d\n",
                           Params.MaxBaudRate));
        SMART_DebugPrintf(("Clock = %d\n",
                           Params.ClockFrequency));
        SMART_DebugPrintf(("MaxClock = %d\n",
                           Params.MaxClockFrequency));
        SMART_DebugPrintf(("ETU = %d\n",
                           Params.WorkETU));
        SMART_DebugPrintf(("WWT = %d\n",
                           Params.WorkWaitingTime));
        SMART_DebugPrintf(("Guard = %d\n",
                           Params.GuardDelay));
        SMART_DebugPrintf(("Retries = %d\n",
                           Params.TxRetries));
        SMART_DebugPrintf(("Vpp = %s\n",
                           Params.VppActive ? "ACTIVE" : "INACTIVE" ));
        SMART_DebugPrintf(("BitConvention = %s\n",
                           Params.DirectBitConvention ? "DIRECT" : "INVERSE" ));

        if ((Params.SupportedProtocolTypes & 2) != 0)
        {
            SMART_DebugPrintf(("BlockRetries: %i\n", Params.BlockRetries));
            SMART_DebugPrintf(("BlockWaitingTime: %i\n",
                                Params.BlockWaitingTime));
            SMART_DebugPrintf(("BlockGuardTime: %i\n",
                                Params.BlockGuardTime));
            SMART_DebugPrintf(("CharacterWaitingTime: %i\n",
                                Params.CharacterWaitingTime));
        }

        SMART_DebugMessage("Checking smartcard capabilities...");
        if ((Params.SupportedProtocolTypes & 2) == 0)
        {
            SMART_DebugMessage("Not capable of T=1 protocol -- exiting");
            goto t1_end;
        }
        else
        {
            SMART_DebugMessage("...Ok");
        }

        SMART_TestPassed(Result);
    }
    else
    {
        SMART_TestFailed(Result, "Unable to get params", SMART_ErrorString(error));
        goto t1_end;
    }

    /* Check that we can set the GuardDelay to 11etu */
    {
        U32 OldDelay = Params.GuardDelay;

        SMART_DebugMessage("Checking we can set GuardDelay of 11...");

        error = STSMART_SetGuardDelay(Handle, 11);
        if (error == ST_NO_ERROR)
        {
            error = STSMART_GetParams(Handle, &Params);
            if (error == ST_NO_ERROR)
            {
                if (Params.GuardDelay == 11)
                {
                    /* Now reset to original value */
                    error = STSMART_SetGuardDelay(Handle, OldDelay);
                    if (error == ST_NO_ERROR)
                    {
                        SMART_TestPassed(Result);
                    }
                    else
                    {
                        SMART_TestFailed(Result,
                                        "Couldn't restore original delay",
                                        SMART_ErrorString(error));
                    }
                }
                else
                {
                    SMART_TestFailed(Result, "GuardDelay not equal to 11", "");
                }
            }
            else
            {
                SMART_TestFailed(Result, "Couldn't get smartcard parameters",
                                SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_TestFailed(Result, "Couldn't set guard delay",
                            SMART_ErrorString(error));
        }
    }

    /* Make sure calls are going through the simulated calls, not the
     * real ones
     */
    UseSim = 1;

    /* Open the file */
    TestFile = fopen("t1test.inf", "ra");
    if (TestFile == NULL)
    {
        SMART_DebugMessage("Unable to open t1test.inf\n");
        goto t1_end;
    }

    /* Load the test */
    okay = T1TestLoader(TestFile, &T1Test);
    while (okay)
    {
        /* Issue the command */
        U32 Written = 0, Read = 0;
        U8 Response[254];

        ResponseNumber = 0;
        T1Mismatch = FALSE;

        SMART_DebugPrintf(("Running test: %s\n", T1Test.TestName));
        error = STSMART_Transfer(Handle, T1Test.Command, T1Test.CommandLength,
                                 &Written, Response, 102, &Read, &Status);

        SMART_DebugPrintf(("Error: %s\n", SMART_ErrorString(error)));
        /* What did we get? */
        if ((error != ST_NO_ERROR) || (TRUE == T1Mismatch))
        {
            if (TRUE == T1Test.SuccessExpected)
            {
                if (error != ST_NO_ERROR)
                {
                    SMART_TestFailed(Result, "Expected ST_NO_ERROR",
                                     SMART_ErrorString(error));
                }
                else
                {
                    SMART_TestFailed(Result, "Expected good transfer",
                                     "Got mismatch");
                }
            }
            else
            {
                SMART_TestPassed(Result);
            }
        }
        else
        {
            /* error == st_no_error */
            if (TRUE == T1Test.SuccessExpected)
            {
                SMART_TestPassed(Result);
            }
            else
            {
                if (ST_NO_ERROR == error)
                {
                    SMART_TestFailed(Result, "Expected an error!", SMART_ErrorString(error));
                }
                else
                {
                    SMART_TestFailed(Result, "Expected bad transfer", "no mismatch");
                }
            }
        }

        okay = T1TestLoader(TestFile, &T1Test);
    }

    if (TestFile != NULL)
    {
        fclose(TestFile);
    }

    /* Now we fill the T1Test structure ourselves (rather than from
     * file), in order to test the SetInfoFieldSize call. Note that if
     * the t1test.inf file changes, then the sequence numbers in this
     * part will also have to be adapted.
     */
    {
        U8 IFSD_Request[] = { 0x21, 0xc1, 0x01, 0x05, 0xe4 };
        U8 IFSC_Response[] = { 0x21, 0xe1, 0x01, 0x05, 0xc4 };

        strcpy((char *)T1Test.TestName, "Scenario 4 - Test IFSD set");
        T1Test.SuccessExpected = TRUE;
        /* Expect interface to request IFSD of the value we give below */
        memcpy(T1Test.Expected[0], IFSD_Request, sizeof(IFSD_Request));
        memcpy(T1Test.Responses[0], IFSC_Response, sizeof(IFSC_Response));
        T1Test.ResponsesPresent = 1;
        memset(T1Test.GenerateLRC, TRUE, MAXCARDRESPONSE);
        /* Reset to the start */
        ResponseNumber = 0;
        T1Mismatch = FALSE;

        SMART_DebugPrintf(("Running test: %s\n", T1Test.TestName));
        error = STSMART_SetInfoFieldSize(Handle, 5);
        SMART_DebugPrintf(("Error: %s\n", SMART_ErrorString(error)));
        if ((error == ST_NO_ERROR) && (FALSE == T1Mismatch))
        {
            SMART_TestPassed(Result);
        }
        else
        {
            if (error != ST_NO_ERROR)
            {
                SMART_TestFailed(Result, "Expected ST_NO_ERROR",
                                 SMART_ErrorString(error));
            }
            else
            {
                SMART_TestFailed(Result, "Expected good transfer",
                                 "Got mismatch");
            }
        }
    }

t1_end:
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    /* Make sure calls aren't going through the simulated calls, but the
     * real ones
     */
    UseSim = 0;

    return Result;
} /* SmartT1() */

/* Loader - when called with an open file, it will read and parse (building up a
 * t1test_t structure) the next test, if tests remain in file.
 * Returns TRUE if test loaded, FALSE if EOF or error.
 */
BOOL T1TestLoader(FILE *TestFile, T1Test_t *Test_p)
{
    char *string_p;
    char string[MAXSTRING];
    BOOL error = FALSE;
    BOOL TheEnd = FALSE;
    BOOL CardResponse = FALSE;
    BOOL CalculateLRC = TRUE;
    U8 Response = 0;
    U16 Byte = 0;

    /* Clear out the test structure */
    memset(Test_p, 0, sizeof(T1Test_t));

    /* And off we go */
    string_p = fgets(string, MAXSTRING, TestFile);
    if (string_p == NULL)
    {
        volatile int something = ferror(TestFile);
        if (something)
            perror("__FILE__ __LINE__");
    }

    Test_p->SuccessExpected = TRUE;
    while ((!TheEnd) && (error == FALSE) && (string_p != NULL))
    {
        Byte = 0;

        while ((strlen(string) > 0) &&
                ((string[strlen(string) - 1] == 0x0a) ||
                 (string[strlen(string) - 1] == 0x0d))
              )
        {
            string[strlen(string) - 1] = 0;
        }

        /* Comment? */
        if (strlen(string) == 0)
        {
            /* end of test */
            if (!CardResponse)
            {
                /* if we haven't read the name yet, it'll be null ie zerolength
                 * therefore error message is adequate either way */
                SMART_DebugPrintf(("Test \"%s\" did not end with a card response!\n",
                    Test_p->TestName));
            }
            TheEnd = TRUE;
        }
        else
        {
            if (string[0] != '#')
            {
                CardResponse = FALSE;

                /* Command? */
                if (string[0] == '[')
                {
                    if (strlen((char *)Test_p->TestName) != 0)
                    {
                        SMART_DebugPrintf(("Error in file when loading test %s\n", string));
                        error = TRUE;
                    }
                    else
                    {
                        strncpy((char *)Test_p->TestName, (char *)&string[1],
                            strlen(string) - 2);
                    }
                }
                else if (!strncmp(string, "Command:", 8))
                {
                    char *token;

                    /* Abstract to another function later */
                    token = strtok((char *)&string[9], ",");
                    while ((token != NULL) && (Byte < MAXBLOCKLENGTH))
                    {
                        if (!strcmp(token, "FAIL"))
                        {
                            Test_p->SuccessExpected = FALSE;
                        }
                        else
                        {
                            Test_p->Command[Byte] = HexToU8(token);
                            Byte++;
                        }

                        token = strtok(NULL, ",");
                    }
                    Test_p->CommandLength = Byte;
                }
                else if (!strncmp(string, "I:", 2))
                {
                    char *token;

                    /* Abstract to another function later */
                    token = strtok((char *)&string[2], ",");
                    while ((token != NULL) && (Byte < MAXBLOCKLENGTH))
                    {
                        if (!strcmp(token, "NOLRC"))
                        {
                            CalculateLRC = FALSE;
                        }
                        else
                        {
                            Test_p->Expected[Response][Byte] = HexToU8(token);
                            Byte++;
                        }

                        token = strtok(NULL, ",");
                    }
                    if (CalculateLRC)
                    {
                        GenerateLRC(Test_p->Expected[Response], Byte,
                                    &Test_p->Expected[Response][Byte]);
                        Byte++;
                    }

                }
                else if (!strncmp(string, "C:", 2))
                {
                    char *token;

                    /* Abstract to another function later */
                    CardResponse = TRUE;
                    token = strtok((char *)&string[2], ",");
                    while ((token != NULL) && (Byte < MAXBLOCKLENGTH))
                    {
                        if (!strcmp(token, "NOLRC"))
                        {
                            CalculateLRC = FALSE;
                        }
                        else
                        {
                            Test_p->Responses[Response][Byte] = HexToU8(token);
                            Byte++;
                        }

                        token = strtok(NULL, ",");
                    }
                    if (CalculateLRC)
                    {
                        GenerateLRC(Test_p->Responses[Response], Byte,
                                    &Test_p->Responses[Response][Byte]);
                        Byte++;
                    }

                    Response++;
                }
                else
                {
                    if (strlen(string) > 0)
                    {
                        SMART_DebugPrintf(("Unrecognised command \"%s\" loaded\n",
                                            string));
                    }
                }
            }
        }

        /* load the next string for processing... */
        if (!TheEnd)
        {
            string_p = fgets(string, MAXSTRING, TestFile);
            CalculateLRC = TRUE;
        }
    }

    Test_p->ResponsesPresent = Response;
    if ((Response > 0) && (feof(TestFile)))
        TheEnd = TRUE;

    return TheEnd;
}

static U8 HexToU8(const char *string)
{
    unsigned int retval;
    U8 tmp1;

    tmp1 = sscanf(string, "%02x", &retval);

    /* Make sure the return value's defined */
    if (tmp1 != 1)
        retval = 0;

    return retval;
}

static void GenerateLRC(U8 *Buffer_p, U32 Count, U8 *LRC_p)
{
    U32 i;

    *LRC_p = 0;
    for (i = 0; i < Count; i++)
    {
        *LRC_p = *LRC_p ^ Buffer_p[i];
    }
}

#elif defined(TEST_T1_CARD_SCOPE)

/*****************************************************************************
SmartT1Scope()
Description:
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartT1Scope(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    STSMART_Status_t Status;
    ST_ErrorCode_t error;

    /* STSMART_Open() */
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;
        OpenParams.SAD = 1;
        OpenParams.DAD = 2;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto t1_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed...");
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond());
            } while (STSMART_GetStatus(Handle, &Status) == STSMART_ERROR_NOT_INSERTED);
        }
    }

    /* STSMART_Reset() */
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif
        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
                STSMART_Print(("[0x%02x]", Answer[i]));
            STSMART_Print(("\n"));
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }
    }

    /* Check the card inserted is a T=1 card -- otherwise skip the test */
    {
        STSMART_Params_t Params;

        SMART_DebugPrintf(("Obtaining card params (slot %d)\n", Params_p->Slot));

        error = STSMART_GetParams(Handle, &Params);
        SMART_DebugError("STSMART_GetParams()", error);

        if (error == ST_NO_ERROR)
        {
            SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                               Params.SupportedProtocolTypes));
            SMART_DebugPrintf(("ProtocolChangeable = %s\n",
                               Params.ProtocolTypeChangeable ? "YES" : "NO" ));
            SMART_DebugPrintf(("BaudRate = %d\n",
                               Params.BaudRate));
            SMART_DebugPrintf(("MaxBaudRate = %d\n",
                               Params.MaxBaudRate));
            SMART_DebugPrintf(("Clock = %d\n",
                               Params.ClockFrequency));
            SMART_DebugPrintf(("MaxClock = %d\n",
                               Params.MaxClockFrequency));
            SMART_DebugPrintf(("ETU = %d\n",
                               Params.WorkETU));
            SMART_DebugPrintf(("WWT = %d\n",
                               Params.WorkWaitingTime));
            SMART_DebugPrintf(("Guard = %d\n",
                               Params.GuardDelay));
            SMART_DebugPrintf(("Retries = %d\n",
                               Params.TxRetries));
            SMART_DebugPrintf(("Vpp = %s\n",
                               Params.VppActive ? "ACTIVE" : "INACTIVE" ));
            SMART_DebugPrintf(("BitConvention = %s\n",
                               Params.DirectBitConvention ? "DIRECT" : "INVERSE" ));

            if ((Params.SupportedProtocolTypes & 2) != 0)
            {
                SMART_DebugPrintf(("BlockRetries: %i\n", Params.BlockRetries));
                SMART_DebugPrintf(("BlockWaitingTime: %i\n",
                                    Params.BlockWaitingTime));
                SMART_DebugPrintf(("BlockGuardTime: %i\n",
                                    Params.BlockGuardTime));
                SMART_DebugPrintf(("CharacterWaitingTime: %i\n",
                                    Params.CharacterWaitingTime));
            }

            SMART_DebugMessage("Checking smartcard capabilities...");
            if ((Params.SupportedProtocolTypes & 2) == 0)
            {
                SMART_DebugMessage("Not capable of T=1 protocol -- exiting");
                goto t1_end;
            }
            else
            {
                SMART_DebugMessage("...Ok");
            }

            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to get params", SMART_ErrorString(error));
            goto t1_end;
        }
    }

    /* Make sure calls are going through the IO routines, not the simulated ones */
    UseSim = 0;

    {
        /* Class 0, command 0x0E, offset 0x100, erase 0x200 bytes  */
        U8 Command[6] = { 0x00, 0x0e, 0x01, 0x00, 0x02, 0x00 };
        /* Ensure commands don't return anything longer than this */
        U8 Response[MAXBLOCKLENGTH];
        U32 Written, Read;
        ST_ErrorCode_t error;

        SMART_DebugMessage("Testing erase command");
        error = STSMART_Transfer(Handle, Command, 6, &Written, Response,
                                 MAXBLOCKLENGTH, &Read, &Status);
        if (error == ST_NO_ERROR)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR", SMART_ErrorString(error));
        }
    }

    {
        /* Class 0, command 0xD0, offset 0x100, 0xC bytes long, data */
        U8 Command[17] = { 0x00, 0xD0, 0x01, 0x00, 0x0C, 0x01, 0x02, 0x03,
                           0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                           0x0c };
        /* Ensure commands don't return anything longer than this */
        U8 Response[MAXBLOCKLENGTH];
        U32 Written, Read;
        ST_ErrorCode_t error;

        SMART_DebugMessage("Testing write command");
        error = STSMART_Transfer(Handle, Command, 17, &Written, Response,
                                 MAXBLOCKLENGTH, &Read, &Status);
        if (error == ST_NO_ERROR)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR", SMART_ErrorString(error));
        }
    }

    {
        /* Class 0, command 0xb0, offset 0x100, 0x0C bytes long */
        U8 Command[5] = { 0x00, 0xB0, 0x01, 0x00, 0x0C };
        /* Ensure commands don't return anything longer than this */
        U8 Response[MAXBLOCKLENGTH] = {0};
        U32 Written, Read;
        ST_ErrorCode_t error;

        SMART_DebugMessage("Testing read command (comparing to just written)");
        error = STSMART_Transfer(Handle, Command, 5, &Written, Response,
                                 MAXBLOCKLENGTH, &Read, &Status);
        if (error == ST_NO_ERROR)
        {
            if (Read == (0x0C + 2))             /* requested + SW bytes */
            {
                BOOL Okay = TRUE;
                U8 i;

                SMART_DebugMessage("Transfer okay, checking bytes against written");
                for (i = 0; i < 0x0C; i++)      /* Check the bytes we read */
                    if (Response[i] != (i + 1))
                        Okay = FALSE;

                if (Okay)
                {
                    SMART_TestPassed(Result);
                }
                else
                {
                    SMART_TestFailed(Result, "Failed!",
                                "Bytes read back were different to written!");
                }
            }
            else
            {
                SMART_TestFailed(Result, "Failed", "Wrong number of bytes returned!");
            }
        }
        else
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR", SMART_ErrorString(error));
        }
    }

    {
        /* Class 0, command 0xb0, offset 0x100, 0x64 bytes long */
        U8 Command[5] = { 0x00, 0xB0, 0x01, 0x00, 0x64 };
        /* Ensure commands don't return anything longer than this */
        U8 Response[MAXBLOCKLENGTH];
        U32 Written, Read;
        ST_ErrorCode_t error;

        SMART_DebugMessage("Testing read command (long)");
        error = STSMART_Transfer(Handle, Command, 5, &Written, Response,
                                 MAXBLOCKLENGTH, &Read, &Status);
        if (error == ST_NO_ERROR)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR", SMART_ErrorString(error));
        }
    }

t1_end:
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartT1() */

#else

/*****************************************************************************
SmartSimple()
Description:
    This routine performs a typical session with the ST smartcard API.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartSimple(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    ST_ErrorCode_t error;
    STSMART_Status_t Status;
    U8 Index = 0;

    /* STSMART_GetCapability() */
    SMART_DebugPrintf(("%d.%d: Get driver capabilities (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure driver reports capabilities correctly");
    {
        STSMART_Capability_t Caps;

        /* Open the smartcard */
        error = STSMART_GetCapability(SmartDeviceName[Params_p->Slot],
                                      &Caps);
        SMART_DebugError("STSMART_GetCapability()", error);

        if (error == ST_NO_ERROR)
        {
            /* Display device capabilities */
            SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                               Caps.SupportedISOProtocols ));
            SMART_DebugPrintf(("DeviceType = %d\n",
                               Caps.DeviceType ));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR", SMART_ErrorString(error));
            goto simple_end;
        }
    }

    /* STSMART_Open() */
    SMART_DebugPrintf(("%d.%d: Open (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure an smartcard device can be opened");
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto simple_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed...");
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond());
            } while (STSMART_GetStatus(Handle, &Status) == STSMART_ERROR_NOT_INSERTED);
        }
        }
    /* STSMART_Reset() */
    SMART_DebugPrintf(("%d.%d: Answer-to-reset (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure a blocking answer-to-reset can be performed");
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif
        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
                STSMART_Print(("[0x%02x]", Answer[i]));
            STSMART_Print(("\n"));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }
    }

    /* STSMART_GetHistory() */
    SMART_DebugPrintf(("%d.%d: Obtain history bytes (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugPrintf(("Purpose: To ensure history bytes are returned correctly,Handle=0x%x\n",Handle));
    {
        U8 History[32];
        U32 HistoryLength;
        U8 i;

        error = STSMART_GetHistory(Handle, History, &HistoryLength);
        SMART_DebugError("STSMART_GetHistory()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output history data */
            SMART_DebugPrintf(("%d history bytes follow:\n", HistoryLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < HistoryLength; i++)
                STSMART_Print(("[0x%02x]", History[i]));
            STSMART_Print(("\n"));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to get history", SMART_ErrorString(error));
        }
    }

    /* STSMART_GetStatus() */
    SMART_DebugPrintf(("%d.%d: Obtain card status (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure last errorcode and status bytes are correct");
    {
        error = STSMART_GetStatus(Handle, &Status);
        SMART_DebugError("STSMART_GetStatus()", error);

        if (error == ST_NO_ERROR)
        {
            SMART_DebugPrintf(("Protocol = %d Status = %s.\n",
                               Status.Protocol,
                               SMART_ErrorString(Status.ErrorCode)));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to get status", SMART_ErrorString(error));
        }
    }

#if defined (STSMART_WARM_RESET) /* WARM RESET */
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;
        /* NDS Test for Warm Reset for P5 Cards - CATEST67 */

        error = STSMART_Reset(Handle,
                              Answer,
                              &AnswerLength,
                              SC_WARM_RESET);

        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data - Expected ATR Return */
            /* "3F","7F","15","25","02","54","B0","02","FF","FF","4A",
                    "50","80","80","00","00","00","48","55","05" */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
                STSMART_Print(("[0x%02x]", Answer[i]));
            STSMART_Print(("\n"));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }

    }
#endif


    /* STSMART_GetParams() */
    SMART_DebugPrintf(("%d.%d: Obtain card params (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure current operating params are accessible");
    {
        STSMART_Params_t Params;

        error = STSMART_GetParams(Handle, &Params);
        SMART_DebugError("STSMART_GetParams()", error);

        if (error == ST_NO_ERROR)
        {
            SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                               Params.SupportedProtocolTypes));
            SMART_DebugPrintf(("ProtocolChangeable = %s\n",
                               Params.ProtocolTypeChangeable ? "YES" : "NO" ));
            SMART_DebugPrintf(("BaudRate = %d\n",
                               Params.BaudRate));
            SMART_DebugPrintf(("MaxBaudRate = %d\n",
                               Params.MaxBaudRate));
            SMART_DebugPrintf(("Clock = %d\n",
                               Params.ClockFrequency));
            SMART_DebugPrintf(("MaxClock = %d\n",
                               Params.MaxClockFrequency));
            SMART_DebugPrintf(("ETU = %d\n",
                               Params.WorkETU));
            SMART_DebugPrintf(("WWT = %d\n",
                               Params.WorkWaitingTime));
            SMART_DebugPrintf(("Guard = %d\n",
                               Params.GuardDelay));
            SMART_DebugPrintf(("Retries = %d\n",
                               Params.TxRetries));
            SMART_DebugPrintf(("Vpp = %s\n",
                               Params.VppActive ? "ACTIVE" : "INACTIVE" ));
            SMART_DebugPrintf(("BitConvention = %s\n",
                               Params.DirectBitConvention ? "DIRECT" : "INVERSE" ));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to get params", SMART_ErrorString(error));
        }
    }

simple_end:
    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartSimple() */

/*****************************************************************************
SmartEvents()
Description:
    This routine performs testing of asynchronous callback events.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartEvents(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    ST_ErrorCode_t error;
    STSMART_Status_t Status;
    U8 Index = 0;


    /* STSMART_Open() */
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = CallbackNotifyFunction;
        OpenParams.BlockingIO = FALSE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto events_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);

        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed.");
            for (;;)
            {
                STSMART_EventType_t Event;
                AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
                if (Event == STSMART_EV_CARD_INSERTED)
                {
                    break;
                }
            }
        }
    }

    /* STSMART_Reset() */
    SMART_DebugPrintf(("%d.%d: Answer to reset (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure an ISO7816 smartcard resets");
    {
        STSMART_EventType_t Event;
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif

        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
            SMART_DebugPrintf(("Event = %s Error = %s\n", SMART_EventToString(Event),
                               SMART_ErrorString(error)));
        }

#if defined(ST_OSLINUX)
        if(Event == STSMART_EV_CARD_RESET)
        {
            error = STSMART_GetReset(Handle, Answer, &AnswerLength);
        }
#endif
        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
            {
                STSMART_Print(("[0x%02x]", Answer[i]));
            }
            STSMART_Print(("\n"));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }

    }
    /* STSMART_Transfer() */
    SMART_DebugPrintf(("%d.%d: Non-blocking command & response (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To check transfer functionality");
    {
        STSMART_EventType_t Event;

        U8 Cmd[] = { 0x00, 0xc0, 0x00, 0x00, 0x17 };
        U8 Buf[255];
        U32 NumberWritten, NumberRead, i;

        error = STSMART_Transfer(Handle, Cmd, 5, &NumberWritten,
                                 Buf, 0, &NumberRead, &Status);
        SMART_DebugError("STSMART_Transfer()", error);

        if (error == ST_NO_ERROR)
        {
            U32 BytesPresent;

            STOS_TaskDelay(ST_GetClocksPerSecond());

            error = STSMART_DataAvailable(Handle, &BytesPresent);
            SMART_DebugMessage("Checking STSMART_DataAvailable()...");
            SMART_DebugError("STSMART_DataAvailable()", error);
            if (error != ST_NO_ERROR)
            {
                SMART_TestFailed(Result, "Expected ST_NO_ERROR",
                                 SMART_ErrorString(error));
            }
            else
            {
                /* A zero might not actually be a fail - but we'd normally
                 * expect to have started receiving data by now.
                 */
                if (BytesPresent == 0)
                {
                    SMART_DebugMessage(("Either the transfer is finised or it is not started"));
                }
                else
                {
                    SMART_TestPassed(Result);
                }
            }

            SMART_DebugMessage("Awaiting notification...");
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
            SMART_DebugPrintf(("Event = %s Error = %s\n",
                               SMART_EventToString(Event),
                               SMART_ErrorString(error)));
#if defined(ST_OSLINUX)
            if(Event==STSMART_EV_TRANSFER_COMPLETED){
                  error = STSMART_GetTransfer(Handle, Cmd, 5, &NumberWritten,
                                            Buf, 0, &NumberRead, &Status);
            }
#endif

            SMART_DebugPrintf(("%d data bytes follow:\n", NumberRead));
            SMART_DebugPrintf((" "));
            for (i = 0; i < NumberRead; i++)
            {
                STSMART_Print(("[0x%02x]", Buf[i]));
            }
            STSMART_Print(("\n"));
        }

        SMART_DebugPrintf(("Status = %s PB1 = 0x%02x PB2 = 0x%02x.\n",
                           SMART_ErrorString(Status.ErrorCode),
                           Status.StatusBlock.T0.PB[0], Status.StatusBlock.T0.PB[1]));

        if (NumberRead == 23 && error == ST_NO_ERROR)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Incorrect number of bytes read",
                             SMART_ErrorString(error));
        }
    }

    SMART_DebugPrintf(("%d.%d: DataAvailable with no operation (slot %d)\n",
                      Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To check function returns 0");
    {
        U32 BytesPresent;

        error = STSMART_DataAvailable(Handle, &BytesPresent);
        SMART_DebugError("STSMART_DataAvailable()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR",
                             SMART_ErrorString(error));
        }
        else
        {
            if (BytesPresent != 0)
            {
                SMART_TestFailed(Result, "Expected 0 bytes back",
                                 "didn't get BytesPresent of 0");
            }
            else
            {
                SMART_TestPassed(Result);
            }
        }
    }


    /* Disable interrupt level of I/O communications -- this will prevent
     * the I/O from completing before we abort...
     */
#if defined(ST_OS21)
    STOS_InterruptDisable(ASC_0_INTERRUPT,NULL);
#elif defined(ST_OSLINUX)
   /*disable_irq(123);*/ /*20050913 123 is irq number for ASC_0_INTERRUPT_LINUX of ST_7100*/
#else
    STOS_InterruptDisable(NULL,1);
#endif

    /* STSMART_Abort() */
    SMART_DebugPrintf(("%d.%d: Abort non-blocking command & response (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To check abort functionality");
    {
        STSMART_EventType_t Event;

        U8 Cmd[] = { 0x00, 0xc0, 0x00, 0x00, 0x17 };
        U8 Buf[255];
        U32 NumberWritten, NumberRead;

        error = STSMART_Transfer(Handle, Cmd, 5, &NumberWritten,
                                 Buf, 0, &NumberRead, &Status);

        if (error == ST_NO_ERROR)
        {
            error = STSMART_Abort(Handle);

            SMART_DebugError("STSMART_Transfer()", ST_NO_ERROR);
            SMART_DebugMessage("Aborting operation...");
            SMART_DebugError("STSMART_Abort()", error);

            /* Wait for completion event */
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
            SMART_DebugPrintf(("Event = %s Error = %s\n",
                               SMART_EventToString(Event),
                               SMART_ErrorString(error)));

            /* Check operation was aborted */
            if (error == STSMART_ERROR_ABORTED)
            {
                SMART_TestPassed(Result);
            }
            else
            {
                SMART_TestFailed(Result, "Expected STSMART_ERROR_ABORTED",
                                 SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_DebugError("STSMART_Transfer()", error);
            SMART_TestFailed(Result, "Unable to start transfer",
                             SMART_ErrorString(error));
        }
    }

    /* STSMART_Abort() */
    SMART_DebugPrintf(("%d.%d: Abort raw write (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To check abort functionality");
    {
        STSMART_EventType_t Event;
        U8 Cmd[] = { 0x00, 0xc0, 0x00, 0x00, 0x17 };
        U32 NumberWritten;

        error = STSMART_RawWrite(Handle, Cmd, 5, &NumberWritten, 0);

        if (error == ST_NO_ERROR)
        {
            error = STSMART_Abort(Handle);

            SMART_DebugError("STSMART_RawWrite()", ST_NO_ERROR);
            SMART_DebugMessage("Aborting operation...");
            SMART_DebugError("STSMART_Abort()", error);

            /* Wait for completion event */
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
            SMART_DebugPrintf(("Event = %s Error = %s\n",
                               SMART_EventToString(Event),
                               SMART_ErrorString(error)));

            /* Check operation was aborted */
            if (error == STSMART_ERROR_ABORTED)
            {
                SMART_TestPassed(Result);
            }
            else
            {
                SMART_TestFailed(Result, "Expected STSMART_ERROR_ABORTED",
                                 SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_DebugError("STSMART_RawWrite()", error);
            SMART_TestFailed(Result, "Unable to start transfer",
                             SMART_ErrorString(error));
        }
    }

    /* STSMART_Abort() */
    SMART_DebugPrintf(("%d.%d: Abort raw read (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To check abort functionality");
    {
        STSMART_EventType_t Event;
        U8 Buf[255];
        U32 NumberRead;

        error = STSMART_RawRead(Handle, Buf, 23, &NumberRead, 0);
        if (error == ST_NO_ERROR)
        {
            error = STSMART_Abort(Handle);

            SMART_DebugError("STSMART_RawRead()", ST_NO_ERROR);
            SMART_DebugMessage("Aborting operation...");
            SMART_DebugError("STSMART_Abort()", error);

            /* Wait for completion event */
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
            SMART_DebugPrintf(("Event = %s Error = %s\n",
                               SMART_EventToString(Event),
                               SMART_ErrorString(error)));

            /* Check operation was aborted */
            if (error == STSMART_ERROR_ABORTED)
            {
                SMART_TestPassed(Result);
            }
            else
            {
                SMART_TestFailed(Result, "Expected STSMART_ERROR_ABORTED",
                                 SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_DebugError("STSMART_RawRead()", error);
            SMART_TestFailed(Result, "Unable to start transfer",
                             SMART_ErrorString(error));
        }
    }

    /* Enable interrupt level once more */
#if defined(ST_OS21)
    STOS_InterruptEnable(ASC_0_INTERRUPT,NULL);
#elif defined(ST_OSLINUX)
#else
    STOS_InterruptEnable(NULL,1);
#endif

events_end:
    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartEvents() */
#if defined(SMART_MULTIPLE_RESET)
/*****************************************************************************
SmartMultipleReset()
Description:
    This routine performs testing of asynchronous callback events.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartMultipleReset(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    ST_ErrorCode_t error;
    STSMART_Status_t Status;
    U8 Index = 0,count = 0;


    /* STSMART_Open() */
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = CallbackNotifyFunction;
        OpenParams.BlockingIO = FALSE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto events_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);

        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed.");
            for (;;)
            {
                STSMART_EventType_t Event;
                AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
                if (Event == STSMART_EV_CARD_INSERTED)
                {
                    break;
                }
            }
        }
    }

    SMART_DebugPrintf(("%d.%d: Multiple reset test\n", Params_p->Ref, Index++));
    for ( count = 0; count < 9; count++ )
    {

        /* STSMART_Reset() */
        SMART_DebugPrintf(("Answer to reset count %d)\n",count));
        {
            STSMART_EventType_t Event;
            U8 Answer[33];
            U32 AnswerLength;
            U8 i;
            error = STSMART_Reset(Handle,
                                 Answer,
#if defined (STSMART_WARM_RESET)
                                 &AnswerLength,
                                 SC_COLD_RESET);
#else
                                 &AnswerLength );
#endif

            SMART_DebugError("STSMART_Reset()", error);

            if (error == ST_NO_ERROR)
            {
                AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
                SMART_DebugPrintf(("Event = %s Error = %s\n", SMART_EventToString(Event),
                               SMART_ErrorString(error)));
            }

            if ( error == ST_NO_ERROR )
            {
                /* Output ATR data */
                SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
                SMART_DebugPrintf((" "));
                for (i = 0; i < AnswerLength; i++)
                {
                    STSMART_Print(("[0x%02x]", Answer[i]));
                }
                STSMART_Print(("\n"));
            }
            else
            {
                break;
            }

            if ( Event == STSMART_EV_CARD_RESET )
            {
                error = STSMART_Deactivate(Handle);
                SMART_DebugError("STSMART_Deactivate()", error);


                if (error == ST_NO_ERROR)
                {
                    AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
                    SMART_DebugPrintf(("Event = %s Error = %s\n", SMART_EventToString(Event),
                               SMART_ErrorString(error)));
                }
            }

        }


    } /* for loop */


    if ( count == 9 )
    {
        SMART_TestPassed(Result);

    }
    else
    {
        SMART_TestFailed(Result, "Muliple reset", SMART_ErrorString(error));
    }


    /* Enable interrupt level once more */
#ifdef ST_OS21
    STOS_InterruptEnable(ASC_0_INTERRUPT, NULL);
#else
    STOS_InterruptEnable(NULL,1);
#endif

events_end:
    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;

} /* SmartMultipleReset */
#endif

#if !defined(ST_5105) && !defined(ST_8010) && !defined(ST_5301) && !defined(ST_5100) \
&& !defined(ST_5107)
/*****************************************************************************
SmartClientTask()
Description:
    This routine is run in the context as a separate task and aims to reflect
    the lifecycle of a client connected to the smartcard interface.
Parameters:
    Args_p,        pointer to smartcard slot to open
Return Value:
    Nothing.
See Also:
    Nothing.
*****************************************************************************/

static void SmartClientTask(void *Args_p)
{
    STSMART_Handle_t Handle;
    ST_ErrorCode_t error;
    STSMART_OpenParams_t OpenParams;

    /* Setup the open params */
    OpenParams.NotifyFunction = NULL;
    OpenParams.BlockingIO = TRUE;

    /* Open the smartcard */
    error = STSMART_Open(SmartDeviceName[(U32)Args_p],
                         &OpenParams,
                         &Handle);

    if (error == ST_NO_ERROR)
    {
        U8 Answer[64];
        U32 AnswerLength;

        STSMART_Print(("==> Client 0x%x opened slot %d...\n", Handle, (int)Args_p));
        STSMART_Lock(Handle, TRUE);
        STSMART_Print(("==> Client 0x%x has locked slot %d...\n", Handle, (int)Args_p));
        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif

        STSMART_Print(("==> Client 0x%x has reset slot %d...%s\n", Handle, (int)Args_p,
                     (error == ST_NO_ERROR) ? "ok":"failed" ));
        if (error == ST_NO_ERROR)
        {
            ResetCount++;
        }
        else
        {
            STSMART_Print(("Client tried to reset slot %d... failed: %s\n",
                    (U32)Args_p, SMART_ErrorString(error)));
        }

        STSMART_Print(("==> Client 0x%x has unlocked slot %d...\n", Handle, (int)Args_p));
        STSMART_Unlock(Handle);
        error = STSMART_Close(Handle);
        STSMART_Print(("==> Client 0x%x closed slot %d...%s\n", Handle,
                     (int)Args_p, (error == ST_NO_ERROR) ? "ok":"failed" ));
    }
    else
    {
        STSMART_Print(("==> Unable to open\n"));
    }
} /* SmartClientTask() */


/*****************************************************************************
SmartMultipleClients()
Description:
    This routine aims to test the driver with multiple clients connected.
    Both smartcards should be inserted for this test, as the clients are run
    on both cards i.e.,

    client 0 - slot 0
    client 1 - slot 1
    client 2 - slot 0
    client 3 - slot 1
    ...

Parameters:
    Params_p,       pointer to test params block.
Return Value:
    SMART_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static SMART_TestResult_t SmartMultipleClients(SMART_TestParams_t *Params_p)
{
    ST_ErrorCode_t error;
    SMART_TestResult_t Result = TEST_RESULT_ZERO;

#if defined (ST_OSLINUX)
    int rc;
#endif
#if !defined(ST_OSLINUX)
    task_t *Task_ptr[SMART_NUMBER_CLIENTS];
#endif
#if defined(ST_OS20 )
    task_t Task[SMART_NUMBER_CLIENTS];
    tdesc_t TaskDescriptor[SMART_NUMBER_CLIENTS];
    tdesc_t TaskStack[SMART_NUMBER_CLIENTS][4096];
#endif

    STSMART_Handle_t Hdl[2];
    U32 Index = 0;
    int i;

    SMART_DebugPrintf(("%d.%d: Multiple clients/cards\n", Params_p->Ref, Index++));
    SMART_DebugMessage("Purpose: Test exclusive smartcard access with multiple "
                       "clients resetting both smartcards");

    /* Ensure both smartcards are inserted before starting */

    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;

        /* Open the smartcard */
        STSMART_Open(SmartDeviceName[0],
                     &OpenParams,
                     &Hdl[0]);

        STSMART_Open(SmartDeviceName[1],
                     &OpenParams,
                     &Hdl[1]);
    }

    /* Check that a card is inserted before commencing */
    for (i = 0; i < 2; i++)
    {
        STSMART_Status_t Status;

        error = STSMART_GetStatus(Hdl[i], &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugPrintf(("Please insert a card into slot %d\n", i));
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond());
            } while (STSMART_GetStatus(Hdl[i], &Status) == STSMART_ERROR_NOT_INSERTED);
        }

        /* Close down the handle */
        STSMART_Close(Hdl[i]);
    }

    /* Initialize client tasks */
    SMART_DebugMessage("Waiting for client tasks to complete...");

    ResetCount = 0;

    for (i = 0; i < SMART_NUMBER_CLIENTS; i++)
    {
#if defined(ST_OS21)

        Task_ptr[i] = task_create((void(*)(void *))SmartClientTask,
                      (void *)((i&1)?SC_SLOT_0:SC_SLOT_1),
                      20 * 1024,
                      MAX_USER_PRIORITY,
                      "SmartClientTask",
                      0);

#elif defined(ST_OS20)
        task_init((void(*)(void *))SmartClientTask,
                  (void *)((i&1)?SC_SLOT_0:SC_SLOT_1),
                  TaskStack[i],
                  4096,
                  &Task[i],
                  &TaskDescriptor[i],
                  MAX_USER_PRIORITY,
                  "SmartClientTask",
                  0);
        Task_ptr[i] = &Task[i];
#elif defined(ST_OSLINUX)

        rc=pthread_create(&task_multiclients[i],NULL,(void*)SmartClientTask,(void *)((i&1)?SC_SLOT_0:SC_SLOT_1));
#else
        SMART_DebugMessage(("SmartClientTask Task Can not be created "));
#endif

    }

#if !defined(ST_OSLINUX)
    /* Wait for client tasks to complete */
    for (i = 0; i < SMART_NUMBER_CLIENTS; i++)
    {
        task_t *Task_p;

        Task_p =  Task_ptr[i];

        task_wait(&Task_p, 1, TIMEOUT_INFINITY);
    }

    /* Delete client tasks */
    for (i = 0; i < SMART_NUMBER_CLIENTS; i++)
    {
        task_delete(Task_ptr[i]);
    }
#elif defined(ST_OSLINUX)
    STOS_TaskDelay(10*ST_GetClocksPerSecond());
    /* Delete client tasks */
    for (i = 0; i < SMART_NUMBER_CLIENTS; i++)
    {
        pthread_cancel(task_multiclients[i]);
    }
#endif
    if (ResetCount == SMART_NUMBER_CLIENTS)
    {
        SMART_TestPassed(Result);
    }
    else
    {
        char Buf[4];

        sprintf(Buf, "= %d", ResetCount);
        SMART_TestFailed(Result, "Incorrect number of successful resets",
                         Buf);
    }

    return Result;
} /* SmartMultipleClients() */

#endif

/*****************************************************************************
SmartAPI()
Description:
    Tests the robustness and validity of the API.
Parameters:
Return Value:
See Also:

*****************************************************************************/

static SMART_TestResult_t SmartAPI(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = { 0, 0 };
    ST_ErrorCode_t error;
    U32 Index = 0, Dummy;
    STSMART_Handle_t Handle=0;

    SMART_DebugPrintf(("%d.%d: Multiple init calls\n", Params_p->Ref, Index++));
    SMART_DebugMessage("Purpose: Ensure the driver rejects multiple init calls");
    {
        error = STSMART_Init(SmartDeviceName[Params_p->Slot],
                             &SmartInitParams[Params_p->Slot]);
        SMART_DebugError("STSMART_Init()", error);
        if (error != ST_ERROR_ALREADY_INITIALIZED)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_ALREADY_INITIALIZED",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }
    }

    SMART_DebugPrintf(("%d.%d: Invalid handle\n", Params_p->Ref, Index++));
    SMART_DebugPrintf(("Purpose: Checks API calls with invalid handle, Handle=0x%x\n",Handle));
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_DataAvailable(Handle, 0);
        SMART_DebugError("STSMART_DataAvailable()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Deactivate(Handle);
        SMART_DebugError("STSMART_Deactivate()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_GetHistory(Handle, 0, 0);
        SMART_DebugError("STSMART_GetHistory()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_GetPtsBytes(Handle, 0);
        SMART_DebugError("STSMART_GetPtsBytes()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_GetStatus(Handle, 0);
        SMART_DebugError("STSMART_GetStatus()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Lock(Handle, TRUE);
        SMART_DebugError("STSMART_Lock()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Reset(Handle,
                              0,
#if defined (STSMART_WARM_RESET)
                              0,
                              SC_COLD_RESET);
#else
                              0 );
#endif
        SMART_DebugError("STSMART_Reset()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetBlockWaitIndex(Handle, 0);
        SMART_DebugError("STSMART_SetBlockWaitIndex()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetCharWaitIndex(Handle, 0);
        SMART_DebugError("STSMART_SetCharWaitIndex()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        /* There's nothing really special about 3500000 */
        error = STSMART_SetClockFrequency(Handle, 3500000, &Dummy);
        SMART_DebugError("STSMART_SetClockFrequency()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetErrorCodeType(Handle, STSMART_CODETYPE_LRC);
        SMART_DebugError("STSMART_SetErrorCodeType()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetInfoFieldSize(Handle, 0x10);
        SMART_DebugError("STSMART_SetInfoFieldSize()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetProtocol(Handle, 0);
        SMART_DebugError("STSMART_SetProtocol()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Unlock(Handle);
        SMART_DebugError("STSMART_Unlock()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Transfer(Handle, 0, 0, &Dummy, 0, 0, &Dummy, 0);
        SMART_DebugError("STSMART_Transfer()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_RawRead(Handle, 0, 0, &Dummy, 0);
        SMART_DebugError("STSMART_RawRead()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_RawWrite(Handle, 0, 0, &Dummy, 0);
        SMART_DebugError("STSMART_RawWrite()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetBaudRate(Handle, 0);
        SMART_DebugError("STSMART_SetBaudRate()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetGuardDelay(Handle, 0);
        SMART_DebugError("STSMART_SetGuardDelay()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetTxRetries(Handle, 0);
        SMART_DebugError("STSMART_SetTxRetries()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetVpp(Handle, 0);
        SMART_DebugError("STSMART_SetVpp()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetWorkWaitingTime(Handle, 0);
        SMART_DebugError("STSMART_SetWorkWaitingTime()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Abort(Handle);
        SMART_DebugError("STSMART_Abort()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Flush(Handle);
        SMART_DebugError("STSMART_Flush()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }
    }

    SMART_DebugPrintf(("%d.%d: Open tests\n", Params_p->Ref, Index++));
    SMART_DebugMessage("Purpose: Attempts to exceed max. clients limit");
    {
        STSMART_Handle_t HandleArray[32];
        U32 i;
        STSMART_OpenParams_t OpenParams = { NULL };

        SMART_DebugMessage("Opening maximum number of clients...");
        for (i = 0; i < 32; i++)
        {
            error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                                 &OpenParams,
                                 &HandleArray[i]);
            if (error != ST_NO_ERROR)
                break;
        }

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Failed to open all clients",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_DebugMessage("Attempting to exceed maximum number of clients...");
            error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                                 &OpenParams,
                                 &Handle);
            SMART_DebugError("STSMART_Open()", error);

            if (error != ST_ERROR_NO_FREE_HANDLES)
            {
                SMART_TestFailed(Result, "Expected ST_ERROR_NO_FREE_HANDLES",
                                 SMART_ErrorString(error));
            }
            else
            {
                SMART_TestPassed(Result);
            }
        }

        /* Close down all handles again */
        for (i = 0; i < 32; i++)
        {
            STSMART_Close(HandleArray[i]);
        }
    }

    SMART_DebugPrintf(("%d.%d: Lock acquisition\n", Params_p->Ref, Index++));
    SMART_DebugMessage("Purpose: Ensure clients can't obtain smartcard lock twice");
    {
        STSMART_OpenParams_t OpenParams = { NULL };
        STSMART_Handle_t HandleArray[2];
        U32 i;

        SMART_DebugMessage("Clients connecting...");
        for (i = 0; i < 2; i++)
        {
            STSMART_Open(SmartDeviceName[Params_p->Slot], &OpenParams,
                         &HandleArray[i]);
        }

        SMART_DebugMessage("Client 0 acquiring lock...");
        error = STSMART_Lock(HandleArray[0], FALSE);
        SMART_DebugError("STSMART_Lock()", error);

        SMART_DebugMessage("Client 1 acquiring lock (should fail)...");
        error = STSMART_Lock(HandleArray[1], FALSE);
        SMART_DebugError("STSMART_Lock()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        SMART_DebugMessage("Client 0 releasing lock...");
        STSMART_Unlock(HandleArray[0]);
        SMART_DebugError("STSMART_Unlock()", error);

        SMART_DebugMessage("Client 1 acquiring lock (should pass)...");
        error = STSMART_Lock(HandleArray[1], FALSE);
        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        SMART_DebugMessage("Client 0 now attempting API calls (should fail)...");

        error = STSMART_Deactivate(HandleArray[0]);
        SMART_DebugError("STSMART_Deactivate()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_GetHistory(HandleArray[0], 0, 0);
        SMART_DebugError("STSMART_GetHistory()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_GetStatus(HandleArray[0], 0);
        SMART_DebugError("STSMART_GetStatus()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Reset(HandleArray[0],
                              0,
#if defined (STSMART_WARM_RESET)
                              0,
                              SC_COLD_RESET);
#else
                              0 );
#endif
        SMART_DebugError("STSMART_Reset()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_SetProtocol(HandleArray[0], 0);
        SMART_DebugError("STSMART_SetProtocol()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Unlock(HandleArray[0]);
        SMART_DebugError("STSMART_Unlock()", error);
        if (error != ST_ERROR_BAD_PARAMETER)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        error = STSMART_Transfer(HandleArray[0], 0, 0, 0, 0, 0, 0, 0);
        SMART_DebugError("STSMART_Transfer()", error);
        if (error != ST_ERROR_DEVICE_BUSY)
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        /* End of api lockout tests */
        SMART_DebugMessage("Closing client 1 connection (with lock held)...");
        error = STSMART_Close(HandleArray[1]);
        SMART_DebugError("STSMART_Close()", error);

        SMART_DebugMessage("Client 0 re-acquiring lock (should pass)...");
        error = STSMART_Lock(HandleArray[0], FALSE);
        SMART_DebugError("STSMART_Lock()", error);
        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Expected ST_NO_ERROR",
                             SMART_ErrorString(error));
        }
        else
        {
            SMART_TestPassed(Result);
        }

        SMART_DebugMessage("Closing client 0 connection...");
        STSMART_Close(HandleArray[0]);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartAPI() */

static SMART_TestResult_t SmartRawAPI(SMART_TestParams_t *Params_p)
{
    STSMART_Params_t Params, OriginalParams;
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    ST_ErrorCode_t error;
    STSMART_Status_t Status;
    U8 Index = 0;

    /* STSMART_Open() */
    SMART_DebugPrintf(("%d.%d: Open (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure an smartcard device can be opened");
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed...");
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond()*2);
            } while (STSMART_GetStatus(Handle, &Status) == STSMART_ERROR_NOT_INSERTED);
        }
    }

    /* STSMART_Reset() */
    SMART_DebugPrintf(("%d.%d: Answer-to-reset (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure a blocking answer-to-reset can be performed");
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif

        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
            {
                STSMART_Print(("[0x%02x]", Answer[i]));
            }
            STSMART_Print(("\n"));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }

    }

    /* STSMART_GetParams() */
    SMART_DebugPrintf(("%d.%d: Obtain card params (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure current operating params are accessible");
    {
        error = STSMART_GetParams(Handle, &Params);
        SMART_DebugError("STSMART_GetParams()", error);

        if (error == ST_NO_ERROR)
        {
            SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                               Params.SupportedProtocolTypes));
            SMART_DebugPrintf(("ProtocolChangeable = %s\n",
                               Params.ProtocolTypeChangeable ? "YES" : "NO" ));
            SMART_DebugPrintf(("BaudRate = %d\n",
                               Params.BaudRate));
            SMART_DebugPrintf(("MaxBaudRate = %d\n",
                               Params.MaxBaudRate));
            SMART_DebugPrintf(("Clock = %d\n",
                               Params.ClockFrequency));
            SMART_DebugPrintf(("MaxClock = %d\n",
                               Params.MaxClockFrequency));
            SMART_DebugPrintf(("ETU = %d\n",
                               Params.WorkETU));
            SMART_DebugPrintf(("WWT = %d\n",
                               Params.WorkWaitingTime));
            SMART_DebugPrintf(("Guard = %d\n",
                               Params.GuardDelay));
            SMART_DebugPrintf(("Retries = %d\n",
                               Params.TxRetries));
            SMART_DebugPrintf(("Vpp = %s\n",
                               Params.VppActive ? "ACTIVE" : "INACTIVE" ));
            SMART_DebugPrintf(("BitConvention = %s\n",
                               Params.DirectBitConvention ? "DIRECT" : "INVERSE" ));

            /* Store params away */
            OriginalParams = Params;

            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to get params", SMART_ErrorString(error));
        }
    }

    /* STSMART_SetBaudRate() */
    SMART_DebugPrintf(("%d.%d: Change baudrate (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure maximum baudrate can be specified");
    {
        U32 BitRateError, OrigBaudRate;

        /* Since we overwrite this later. */
        OrigBaudRate = Params.BaudRate;

        /* Set max baudrate */
        error = STSMART_SetBaudRate(Handle, Params.MaxBaudRate);
        SMART_DebugError("STSMART_SetBaudRate()", error);

        /* Get new params */
        STSMART_GetParams(Handle, &Params);

        /* Calculate error against theoretical maximum - note that
         * error is introduced by the smartcard CLKGEN granularity.
         */
        BitRateError = ((abs(Params.MaxBaudRate - Params.BaudRate) * 1000) /
                        Params.MaxBaudRate);

        /* Display params */
        SMART_DebugPrintf(("BaudRate = %d\n", Params.BaudRate));
        SMART_DebugPrintf(("Error = %d.%d%%\n", BitRateError/10, BitRateError%10));

        /* Check baudrate has been applied within a 12% margin */
        if ((error == ST_NO_ERROR) && (BitRateError/10 < 12))
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Not maximum baudrate",
                             SMART_ErrorString(error));
        }

        SMART_DebugMessage("Restoring original baudrate");
        error = STSMART_SetBaudRate(Handle, OrigBaudRate);
        SMART_DebugError("STSMART_SetBaudRate()", error);
    }

    /* STSMART_SetClockFrequency() */
    SMART_DebugPrintf(("%d.%d: Changing clock frequency (slot %d)\n",
                       Params_p->Ref,
                       Index++,
                       Params_p->Slot));
    SMART_DebugMessage("Purpose: Tests clock changing functionality");
    {
        U32 ActualFreq, OrigFreq;
        BOOL Passed = TRUE;

        /* Since we overwrite it shortly */
        OrigFreq = Params.ClockFrequency;

        SMART_DebugPrintf(("Changing clock frequency to max (%i)\n",
                          Params.MaxClockFrequency));
        error = STSMART_SetClockFrequency(Handle,
                                          Params.MaxClockFrequency,
                                          &ActualFreq);
        SMART_DebugError("STSMART_SetClockFrequency()", error);
        if (error != ST_NO_ERROR)
        {
            Passed = FALSE;
        }
        else
        {
            SMART_DebugPrintf(("Actual frequency: %i\n", ActualFreq));
        }

        error = STSMART_GetParams(Handle, &Params);
        SMART_DebugError("STSMART_GetParams()", error);
        if (error != ST_NO_ERROR)
        {
            Passed = FALSE;
        }
        else
        {
            if (ActualFreq != Params.ClockFrequency)
            {
                Passed = FALSE;
                SMART_DebugPrintf(("Reported frequency differs from SetClockFrequency return\n"));
                SMART_DebugPrintf(("Params.ClockFrequency: %i\n",
                                  Params.ClockFrequency));
            }
        }

        if (SmartInitParams[0].DeviceType != STSMART_ISO_NDS)
        {
            SMART_DebugMessage("Trying to change beyond max - should fail");
            error = STSMART_SetClockFrequency(Handle,
                                              Params.MaxClockFrequency + 1,
                                              &ActualFreq);
            SMART_DebugError("STSMART_SetClockFrequency()", error);
            if (error != ST_ERROR_BAD_PARAMETER)
            {
                Passed = FALSE;
            }
        }

        SMART_DebugPrintf(("Restoring original clock frequency (%i)\n",
                          OrigFreq));
        error = STSMART_SetClockFrequency(Handle, OrigFreq, &ActualFreq);
        SMART_DebugError("STSMART_SetClockFrequency()", error);
        if (error != ST_NO_ERROR)
        {
            Passed = FALSE;
        }
        else
        {
            if (ActualFreq != OrigFreq)
            {
                Passed = FALSE;
                STSMART_Print(("Actual frequency != original frequency! (%i %i)\n",
                            ActualFreq, OrigFreq));
            }
        }

        error = STSMART_GetParams(Handle, &Params);
        SMART_DebugError("STSMART_GetParams()", error);
        if (error != ST_NO_ERROR)
        {
            Passed = FALSE;
        }
        else
        {
            if (OrigFreq != Params.ClockFrequency)
            {
                Passed = FALSE;
                SMART_DebugPrintf(("Reported frequency differs from original frequency\n"));
                SMART_DebugPrintf(("Params.ClockFrequency: %i\n",
                                  Params.ClockFrequency));
            }
        }
        if (FALSE == Passed)
        {
            SMART_TestFailed(Result, "SetClockFrequency tests failed",
                             SMART_ErrorString(error));
        }
        else
        {
            error = STSMART_GetParams(Handle, &Params);
            SMART_TestPassed(Result);
        }
    }

    /* STSMART_RawWrite() */
    SMART_DebugPrintf(("%d.%d: Raw write/read (slot %d)\n",
                       Params_p->Ref,
                       Index++,
                       Params_p->Slot));
    SMART_DebugMessage("Purpose: Tests raw write/read functionality");
    {
        U8 Cmd[] = { 0x00, 0xc0, 0x00, 0x00, 0x17 };
        U8 Buf[255];
        U32 NumberWritten, NumberRead, i;

        error = STSMART_RawWrite(Handle, Cmd, 5, &NumberWritten, ((U32)-1));
        SMART_DebugError("STSMART_RawWrite()", error);
        if (error == ST_NO_ERROR)
        {

            STOS_TaskDelay(ST_GetClocksPerSecond()*2);
            error = STSMART_RawRead(Handle, Buf, 24, &NumberRead, ((U32)-1));
            SMART_DebugError("STSMART_RawRead()", error);

            /* First byte should be procedure byte */
            if (Cmd[1] == Buf[0])
            {
                SMART_DebugPrintf(("Procedure byte: [0x%02x]\n", Buf[0]));
                if (NumberRead == 24)
                {
                    SMART_DebugPrintf(("%d data bytes follow:\n", NumberRead-1));
                    SMART_DebugPrintf((" "));
                    for (i = 1; i < NumberRead; i++)
                    {
                        STSMART_Print(("[0x%02x]", Buf[i]));
                    }

                    STSMART_Print(("\n"));
                    SMART_TestPassed(Result);
                }
                else
                {
                    SMART_TestFailed(Result, "Incorrect number of bytes read",
                                     SMART_ErrorString(error));
                }
            }
            else
            {
                SMART_TestFailed(Result, "Incorrect procedure byte",
                                 SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_TestFailed(Result, "RawWrite failed",
                                     SMART_ErrorString(error));
        }
    }

    /* STSMART_Flush() */
    SMART_DebugPrintf(("%d.%d: Flush input buffers (slot %d)\n",
                       Params_p->Ref,
                       Index++,
                       Params_p->Slot));
    SMART_DebugMessage("Purpose: Ensures flush really removes any pending data");
    {
        U8 Cmd[] = { 0x00, 0xc0, 0x00, 0x00, 0x17 };
        U8 Buf[255];
        U32 NumberWritten, NumberRead;

        error = STSMART_RawWrite(Handle, Cmd, 5, &NumberWritten, ((U32)-1));
        SMART_DebugError("STSMART_RawWrite()", error);

        if (error == ST_NO_ERROR)
        {
            /* Short delay to allow all data to be received */
            SMART_DebugMessage("Short delay to allow smartcard to respond...");
            STOS_TaskDelay(ST_GetClocksPerSecond()*2);

            /* Flush rx data */
            SMART_DebugMessage("Flushing...");
            error = STSMART_Flush(Handle);
            SMART_DebugError("STSMART_Flush()", error);

            /* First byte should be procedure byte */
            if (error == ST_NO_ERROR)
            {
                SMART_DebugPrintf(("Now trying to read data after flush...\n"));

                error = STSMART_RawRead(Handle, Buf, 24, &NumberRead, ((U32)-1));
                SMART_DebugError("STSMART_RawRead()", error);

                if (error == ST_ERROR_TIMEOUT)
                {
                    SMART_TestPassed(Result);
                }
                else
                {
                    SMART_TestFailed(Result, "Expected timeout",
                                     SMART_ErrorString(error));
                }
            }
            else
            {
                SMART_TestFailed(Result, "Flush failed",
                                 SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_TestFailed(Result, "Write failed", SMART_ErrorString(error));
        }
    }

    /* STSMART_SetVpp() */
    SMART_DebugPrintf(("%d.%d: Change Vpp state (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure Vpp can be modified");
    {
        /* Set Vpp active */
        error = STSMART_SetVpp(Handle, TRUE);
        SMART_DebugError("STSMART_SetVpp()", error);

        /* Get new params */
        STSMART_GetParams(Handle, &Params);
        SMART_DebugPrintf(("Vpp = %s\n",
                           Params.VppActive ? "ACTIVE" : "INACTIVE" ));

        /* Check Vpp has been applied */
        if (Params.VppActive)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Vpp not active",
                             SMART_ErrorString(error));
        }

        /* Set Vpp active */
        error = STSMART_SetVpp(Handle, FALSE);
        SMART_DebugError("STSMART_SetVpp()", error);

        /* Get new params */
        STSMART_GetParams(Handle, &Params);
        SMART_DebugPrintf(("Vpp = %s\n",
                           Params.VppActive ? "ACTIVE" : "INACTIVE" ));

        /* Check Vpp has been applied */
        if (!Params.VppActive)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Vpp still active",
                             SMART_ErrorString(error));
        }
    }

    /* STSMART_SetTxRetries() */
    SMART_DebugPrintf(("%d.%d: Sets tx retries (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure number of retries can be modified");
    {
        /* Disable retries */
        error = STSMART_SetTxRetries(Handle, 0);
        SMART_DebugError("STSMART_SetTxRetries()", error);

        /* Get new params */
        STSMART_GetParams(Handle, &Params);
        SMART_DebugPrintf(("TxRetries = %d\n",
                           Params.TxRetries));

        /* Check retries count has been applied */
        if (Params.TxRetries == 0)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Retries not updated",
                             SMART_ErrorString(error));
        }

    }

    /* STSMART_SetWorkWaitingTime() */
    SMART_DebugPrintf(("%d.%d: Sets work waiting time (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure WWT can be modified");
    {
        /* Try minimum WWT */
        error = STSMART_SetWorkWaitingTime(Handle, 0);
        SMART_DebugError("STSMART_SetWorkWaitingTime()", error);

        /* Get new params */
        STSMART_GetParams(Handle, &Params);
        SMART_DebugPrintf(("WWT = %d\n",
                           Params.WorkWaitingTime));

        /* Ensure minimum WWT is applied (should be 1ms) */
        if (Params.WorkWaitingTime > 0)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Invalid WWT",
                             SMART_ErrorString(error));
        }
    }

    /* STSMART_SetGuardDelay() */
    SMART_DebugPrintf(("%d.%d: Sets tx guard delay time (slot %d)\n",
                      Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure guard delay can be modified");
    {
        /* Try invalid guard delay 257 */
        error = STSMART_SetGuardDelay(Handle, 257);
        SMART_DebugError("STSMART_SetGuardDelay()", error);

        if (error == ST_ERROR_BAD_PARAMETER)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             SMART_ErrorString(error));
        }

        /* Get new params */
        STSMART_GetParams(Handle, &Params);
        SMART_DebugPrintf(("GuardDelay = %d\n",
                           Params.GuardDelay));

        /* Ensure minimum guard delay has not changed */
        if (Params.GuardDelay != 1)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Invalid guard delay",
                             SMART_ErrorString(error));
        }

        /* Try valid guard delay */
        error = STSMART_SetGuardDelay(Handle, 3);
        SMART_DebugError("STSMART_SetGuardDelay()", error);

        /* Get new params */
        STSMART_GetParams(Handle, &Params);
        SMART_DebugPrintf(("GuardDelay = %d\n",
                           Params.GuardDelay));

        /* Ensure minimum guard delay has not changed */
        if (Params.GuardDelay == 3)
        {
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Guard delay not updated",
                             SMART_ErrorString(error));
        }
    }

    /* STSMART_Reset() */
    SMART_DebugPrintf(("%d.%d: Reset params to defaults (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure a reset defaults params");
    {
        U8 Answer[33];
        U32 AnswerLength;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif

        SMART_DebugError("STSMART_Reset()", error);
        if (error == ST_NO_ERROR)
        {
            error = STSMART_GetParams(Handle, &Params);
            SMART_DebugError("STSMART_GetParams()", error);

            SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                               Params.SupportedProtocolTypes));
            SMART_DebugPrintf(("ProtocolChangeable = %s\n",
                               Params.ProtocolTypeChangeable ? "YES" : "NO" ));
            SMART_DebugPrintf(("BaudRate = %d\n",
                               Params.BaudRate));
            SMART_DebugPrintf(("MaxBaudRate = %d\n",
                               Params.MaxBaudRate));
            SMART_DebugPrintf(("Clock = %d\n",
                               Params.ClockFrequency));
            SMART_DebugPrintf(("MaxClock = %d\n",
                               Params.MaxClockFrequency));
            SMART_DebugPrintf(("ETU = %d\n",
                               Params.WorkETU));
            SMART_DebugPrintf(("WWT = %d\n",
                               Params.WorkWaitingTime));
            SMART_DebugPrintf(("Guard = %d\n",
                               Params.GuardDelay));
            SMART_DebugPrintf(("Retries = %d\n",
                               Params.TxRetries));
            SMART_DebugPrintf(("Vpp = %s\n",
                               Params.VppActive ? "ACTIVE" : "INACTIVE" ));
            SMART_DebugPrintf(("BitConvention = %s\n",
                               Params.DirectBitConvention ? "DIRECT" : "INVERSE" ));

            /* Check params are back to defaults */
            if ( Params.SupportedProtocolTypes == OriginalParams.SupportedProtocolTypes &&
                 Params.ProtocolTypeChangeable == OriginalParams.ProtocolTypeChangeable &&
                 Params.BaudRate == OriginalParams.BaudRate &&
                 Params.MaxBaudRate == OriginalParams.MaxBaudRate &&
                 Params.ClockFrequency == OriginalParams.ClockFrequency &&
                 Params.MaxClockFrequency == OriginalParams.MaxClockFrequency &&
                 Params.WorkETU == OriginalParams.WorkETU &&
                 Params.WorkWaitingTime == OriginalParams.WorkWaitingTime &&
                 Params.GuardDelay == OriginalParams.GuardDelay &&
                 Params.TxRetries == OriginalParams.TxRetries &&
                 Params.VppActive == OriginalParams.VppActive &&
                 Params.DirectBitConvention == OriginalParams.DirectBitConvention)
            {
                SMART_TestPassed(Result);
            }
            else
            {
                SMART_TestFailed(Result, "Param check failed",
                                 SMART_ErrorString(error));
            }
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }
    }

    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartRawAPI() */


void test_overhead(SMART_TestParams_t *Params_p) { }

void test_init(SMART_TestParams_t *Params_p)
{
    ST_ErrorCode_t error;

    error = STSMART_Init(SmartDeviceName[Params_p->Slot],
                         &SmartInitParams[Params_p->Slot]);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("Failed to init: ", error);
    }
}

void test_typical(SMART_TestParams_t *Params_p)
{
    ST_ErrorCode_t          error;
    STSMART_OpenParams_t    OpenParams;
    STSMART_Handle_t        Handle;
    STSMART_Status_t        Status;
    STSMART_Params_t        Params;

    /* Setup the open params */
    OpenParams.NotifyFunction = NULL;
    OpenParams.BlockingIO = TRUE;

    /* Open the smartcard */
    error = STSMART_Open(SmartDeviceName[Params_p->Slot], &OpenParams, &Handle);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Open()", error);
        return;
    }

    /* Check that a card is inserted before commencing */
    error = STSMART_GetStatus(Handle, &Status);
    if (error == STSMART_ERROR_NOT_INSERTED)
    {
        SMART_DebugMessage("No smartcard inserted");
    }

    /* STSMART_Reset() */
            error = STSMART_Reset(Handle,
                                  StackTestAnswer,
#if defined (STSMART_WARM_RESET)
                                  &StackTestAnswerLength,
                                  SC_COLD_RESET);
#else
                                  &StackTestAnswerLength );
#endif
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Reset()", error);
    }

    /* STSMART_GetHistory() */
    error = STSMART_GetHistory(Handle, StackTestAnswer,
                               &StackTestAnswerLength);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_GetHistory()", error);
    }

    /* STSMART_GetStatus() */
    error = STSMART_GetStatus(Handle, &Status);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_GetStatus()", error);
    }

    /* STSMART_GetParams() */
    error = STSMART_GetParams(Handle, &Params);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_GetParams()", error);
    }

    /* STSMART_Close() */
    error = STSMART_Close(Handle);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Close()", error);
    }
}

void test_term(SMART_TestParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STSMART_TermParams_t TermParams;

    TermParams.ForceTerminate = TRUE;
    error = STSMART_Term(SmartDeviceName[Params_p->Slot], &TermParams);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("Failed to terminate", error);
    }
}

/* Note - bulk of function makes (possibly unsafe) assumption that, if tmp
 * can be allocated initially, later allocations won't fail.
 */
void SmartDataUsage(SMART_TestParams_t *Params_p)
{
    U8 *tmp;
    U32 add1, add2;
    U32 StoredNumber;
    ST_ErrorCode_t error;
    STSMART_TermParams_t TermParams;

    StoredNumber = SmartInitParams[Params_p->Slot].MaxHandles;
    TermParams.ForceTerminate = FALSE;

    /* Initial marker */
    tmp = STOS_MemoryAllocate(system_partition, 1);
    if (tmp == NULL)
    {
        SMART_DebugMessage(("Couldn't allocate memory for marker\n"));
        return;
    }
    add1 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);

    /* Try with MaxHandles = 1 */
    SmartInitParams[Params_p->Slot].MaxHandles = 1;
    error = STSMART_Init(SmartDeviceName[Params_p->Slot], &SmartInitParams[Params_p->Slot]);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);

    SMART_DebugPrintf(("Maximum handles: 1; used memory: %i\n", add1 - add2));
    error = STSMART_Term(SmartDeviceName[Params_p->Slot], &TermParams);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Term()", error);
        return;
    }

    /* Try with MaxHandles = 2 */
    SmartInitParams[Params_p->Slot].MaxHandles = 2;
    error = STSMART_Init(SmartDeviceName[Params_p->Slot], &SmartInitParams[Params_p->Slot]);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);

    SMART_DebugPrintf(("Maximum handles: 2; used memory: %i\n", add1 - add2));
    error = STSMART_Term(SmartDeviceName[Params_p->Slot], &TermParams);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Term()", error);
        return;
    }

    /* Try with MaxHandles = 3 */
    SmartInitParams[Params_p->Slot].MaxHandles = 3;
    error = STSMART_Init(SmartDeviceName[Params_p->Slot], &SmartInitParams[Params_p->Slot]);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Init()", error);
        return;
    }
    tmp = STOS_MemoryAllocate(system_partition, 1);
    add2 = (U32)tmp;
    STOS_MemoryDeallocate(system_partition, tmp);

    SMART_DebugPrintf(("Maximum handles: 3; used memory: %i\n", add1 - add2));
    error = STSMART_Term(SmartDeviceName[Params_p->Slot], &TermParams);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Term()", error);
        return;
    }

    /* Reset MaxHandles */
    SmartInitParams[Params_p->Slot].MaxHandles = StoredNumber;
}

#if !defined(ST_OSLINUX)
static SMART_TestResult_t SmartStackUsage(SMART_TestParams_t *Params_p)
{
    ST_ErrorCode_t error;
    STSMART_TermParams_t TermParams;
    SMART_TestResult_t result = TEST_RESULT_ZERO;

#if defined(ST_OS20)
    task_t task;
    tdesc_t tdesc;
#endif
#if defined(ST_OSLINUX)
int rc;
#else
    task_t *task_p;
#endif
#if !defined(ST_OSLINUX)
    task_status_t status;
#endif
#if defined (ST_OS20)
    char stack[4 * 1024]; 
#endif
    U32 i;
    void (*func_table[])(SMART_TestParams_t *) = {
                                     test_overhead,
                                     test_init,
                                     test_typical,
                                     test_term,
                                     NULL
                                   };
    void (*func)(void *);

    /* Terminate existing instance */
    TermParams.ForceTerminate = TRUE;
    error = STSMART_Term(SmartDeviceName[Params_p->Slot], &TermParams);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Term()", error);
        return result;
    }
#if !defined(ST_OS21) && !defined(ST_OSLINUX)
    task_p = &task;
#endif

    for (i = 0; func_table[i] != NULL; i++)
    {
        func = (void (*)(void *))func_table[i];
#if defined(ST_OS21)
        task_p = task_create(func,(void *)Params_p,20*1024,
                             MAX_USER_PRIORITY,"stack_test",0);
        task_wait(&task_p, 1, TIMEOUT_INFINITY);
        task_status(task_p, &status, task_status_flags_stack_used);
#elif defined(ST_OS20)
        task_init(func, (void *)Params_p, stack, 4 * 1024, task_p,
                  &tdesc, MAX_USER_PRIORITY, "stack_test", 0);
        task_wait(&task_p, 1, TIMEOUT_INFINITY);
        task_status(task_p, &status, task_status_flags_stack_used);
#elif defined(ST_OSLINUX)
        rc=pthread_create(&task_stackusage,tattr_stackusage_p,(void*)func,myParams_stackusage_p);
#else
        printf("Can not create the task \n");
#endif

#if !defined(ST_OSLINUX)
        task_delete(task_p);
#endif
#if !defined(ST_OSLINUX)
        SMART_DebugPrintf(("Function %i, stack used: %i\n",
                          i, status.task_stack_used));
#endif
    }

    SmartDataUsage(Params_p);

    /* Initialise terminated instance */
    error = STSMART_Init(SmartDeviceName[Params_p->Slot],
                         &SmartInitParams[Params_p->Slot]);
    if (error != ST_NO_ERROR)
    {
        SMART_DebugError("STSMART_Init()", error);
    }

    return result;
}
#endif

#if defined(UART_F_PRESENT)
/*****************************************************************************
SmartTestGuardTime()
Description:
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/
static SMART_TestResult_t SmartTestGuardTime(SMART_TestParams_t *Params_p)
{
    SMART_TestResult_t Result = TEST_RESULT_ZERO;
    STSMART_Handle_t Handle;
    ST_ErrorCode_t error;
    STSMART_Status_t Status;
    U8 Index = 0;
    U16 GuardTime = 0;

    SMART_DebugMessage("SmartTestGuardTime");

    /* STSMART_Open() */
    SMART_DebugPrintf(("%d.%d: Open (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    {
        STSMART_OpenParams_t OpenParams;

        /* Setup the open params */
        OpenParams.NotifyFunction = NULL;
        OpenParams.BlockingIO = TRUE;

        /* Open the smartcard */
        error = STSMART_Open(SmartDeviceName[Params_p->Slot],
                             &OpenParams,
                             &Handle);
        SMART_DebugError("STSMART_Open()", error);

        if (error != ST_NO_ERROR)
        {
            SMART_TestFailed(Result, "Unable to open", SMART_ErrorString(error));
            goto simple_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        error = STSMART_GetStatus(Handle, &Status);
        if (error == STSMART_ERROR_NOT_INSERTED)
        {
            SMART_DebugMessage("Please insert a smartcard to proceed...");
            do
            {
               STOS_TaskDelay(ST_GetClocksPerSecond());
            } while (STSMART_GetStatus(Handle, &Status) == STSMART_ERROR_NOT_INSERTED);
        }
    }

    /* STSMART_Reset() */
    SMART_DebugPrintf(("%d.%d: Answer-to-reset (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure a blocking answer-to-reset can be performed");
    {
        U8 Answer[33];
        U32 AnswerLength;
        U8 i;

        error = STSMART_Reset(Handle,
                              Answer,
#if defined (STSMART_WARM_RESET)
                              &AnswerLength,
                              SC_COLD_RESET);
#else
                              &AnswerLength );
#endif

        SMART_DebugError("STSMART_Reset()", error);

        if (error == ST_NO_ERROR)
        {
            /* Output ATR data */
            SMART_DebugPrintf(("%d ATR bytes follow:\n", AnswerLength));
            SMART_DebugPrintf((" "));
            for (i = 0; i < AnswerLength; i++)
                STSMART_Print(("[0x%02x]", Answer[i]));
            STSMART_Print(("\n"));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to reset", SMART_ErrorString(error));
        }
    }

    /* STSMART_GetParams() */
    SMART_DebugPrintf(("%d.%d: Obtain card params (slot %d)\n", Params_p->Ref, Index++, Params_p->Slot));
    SMART_DebugMessage("Purpose: To ensure current operating params are accessible");
    {
        STSMART_Params_t Params;

        error = STSMART_GetParams(Handle, &Params);
        SMART_DebugError("STSMART_GetParams()", error);

        if (error == ST_NO_ERROR)
        {
            SMART_DebugPrintf(("SupportedProtocols = 0x%02x\n",
                               Params.SupportedProtocolTypes));
            SMART_DebugPrintf(("ProtocolChangeable = %s\n",
                               Params.ProtocolTypeChangeable ? "YES" : "NO" ));
            SMART_DebugPrintf(("BaudRate = %d\n",
                               Params.BaudRate));
            SMART_DebugPrintf(("MaxBaudRate = %d\n",
                               Params.MaxBaudRate));
            SMART_DebugPrintf(("Clock = %d\n",
                               Params.ClockFrequency));
            SMART_DebugPrintf(("MaxClock = %d\n",
                               Params.MaxClockFrequency));
            SMART_DebugPrintf(("ETU = %d\n",
                               Params.WorkETU));
            SMART_DebugPrintf(("WWT = %d\n",
                               Params.WorkWaitingTime));
            SMART_DebugPrintf(("Guard = %d\n",
                               Params.GuardDelay));
            SMART_DebugPrintf(("Retries = %d\n",
                               Params.TxRetries));
            SMART_DebugPrintf(("Vpp = %s\n",
                               Params.VppActive ? "ACTIVE" : "INACTIVE" ));
            SMART_DebugPrintf(("BitConvention = %s\n",
                               Params.DirectBitConvention ? "DIRECT" : "INVERSE" ));
            SMART_TestPassed(Result);
        }
        else
        {
            SMART_TestFailed(Result, "Unable to get params", SMART_ErrorString(error));
        }
    }

    /* STSMART_Transfer() */
    for (GuardTime = 2 + 253; GuardTime < 2 + 255; GuardTime++)
    {
        STSMART_EventType_t Event;
        U8 Cmd[] = { 0x00, 0xc0, 0x00, 0x00, 0x17 };
        U8 Buf[255];
        U32 NumberWritten, NumberRead;

        /* Set this iterations transmission-time value */
        SMART_DebugPrintf(("Setting guard-delay of %i\n", GuardTime));
        error = STSMART_SetGuardDelay(Handle, GuardTime);
        if (error != ST_NO_ERROR)
        {
            SMART_DebugError("STSMART_SetGuardDelay()", error);
        }

        /* Now do the transfer */
        error = STSMART_Transfer(Handle, Cmd, 5, &NumberWritten,
                                 Buf, 0, &NumberRead, &Status);
        SMART_DebugError("STSMART_Transfer()", error);

        if (error == ST_NO_ERROR)
        {
            SMART_DebugMessage("Awaiting notification...");
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &error);
        }
        else
        {
            if (error == STSMART_ERROR_INVALID_STATUS_BYTE)
            {
                SMART_DebugMessage("This error is expected (card guardtime != driver guardtime)");
            }
        }

        SMART_DebugMessage("Waiting at end of transfer...");
        STOS_TaskDelay(ST_GetClocksPerSecond() / 2);
    }

    SMART_DebugMessage("All transfers complete; now check scope for GuardTime");

simple_end:
    /* STSMART_Close() */
    {
        error = STSMART_Close(Handle);
        SMART_DebugError("STSMART_Close()", error);
    }

    return Result;
} /* SmartTestGuardTime() */
#endif /* defined(UART_F_PRESENT) */


#endif

/* Debug routines --------------------------------------------------------- */

static char *SMART_ErrorString(ST_ErrorCode_t Error)
{
    SMART_ErrorMessage *mp;

    mp = SMART_ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return mp->ErrorMsg;
}

char *SMART_EventToString(STSMART_EventType_t Event)
{
    static char Events[][30] = {
        "STSMART_EV_CARD_INSERTED",
        "STSMART_EV_CARD_REMOVED",
        "STSMART_EV_CARD_RESET",
        "STSMART_EV_CARD_DEACTIVATED",
        "STSMART_EV_TRANSFER_COMPLETED",
        "STSMART_EV_WRITE_COMPLETED",
        "STSMART_EV_READ_COMPLETED",
        "STSMART_EV_NO_OPERATION"
    };

    return Events[(Event - STSMART_DRIVER_BASE)];
}

ST_ErrorCode_t GetBlock(U8 *Buffer, U32 *Length)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;

    if (ResponseNumber < T1Test.ResponsesPresent)
    {
        /* how much? */
        *Length = 3 + T1Test.Responses[ResponseNumber][2] + 1;

        /* Fill buffer */
        memcpy(Buffer, &T1Test.Responses[ResponseNumber][0], *Length);

        /* Print a copy of what we're transmitting */
        SMART_DebugPrintf(("Sending from ICC:\n"));
        SMART_DebugPrintf((" "));
        for (i = 0; i < *Length; i++)
        {
            printf("%02x ", Buffer[i]);
            if ((i % 16 == 0) && (i != 0))
            {
                printf("\n");
                SMART_DebugPrintf((" "));
            }
        }
        if (i % 16 != 0)
            printf("\n");

        ResponseNumber++;
    }
    else
    {
        SMART_DebugMessage("GetBlock(): warning - no more responses to send!");
        T1Mismatch = TRUE;
        error = STSMART_ERROR_BLOCK_TIMEOUT;
    }

    /* return. */
    return error;
}

ST_ErrorCode_t PutBlock(U8 *Buffer, U32 Length)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;
    S32 Difference = -1;

    SMART_DebugPrintf(("PutBlock(): received from IFD: \n"));
    SMART_DebugPrintf((" "));
    for (i = 0; i < Length; i++)
    {
        printf("%02x ", Buffer[i]);
        if ((i % 16 == 0) && (i != 0))
        {
            printf("\n");
            SMART_DebugPrintf((" "));
        }
        if ((Buffer[i] != T1Test.Expected[ResponseNumber][i]) &&
            (Difference < 0))
        {
            Difference = i;
        }
    }
    if (i % 16 != 0)
        printf("\n");

    if (Difference != -1)
    {
        SMART_DebugPrintf(("Discrepancy first noted at position %i!\n",
                Difference));
        T1Mismatch = TRUE;
        if (ResponseNumber < T1Test.ResponsesPresent)
        {
            SMART_DebugPrintf(("Expecting: \n"));
            SMART_DebugPrintf((" "));
            Length = 3 + T1Test.Expected[ResponseNumber][2] + 1;
            for (i = 0; i < Length; i++)
            {
                printf("%02x ", T1Test.Expected[ResponseNumber][i]);
                if ((i % 16 == 0) && (i != 0))
                {
                    printf("\n");
                    SMART_DebugPrintf((" "));
                }
            }
            if (i % 16 != 0)
                printf("\n");
        }
        else
        {
            SMART_DebugMessage("No more blocks defined from IFD!");
        }
    }

    return error;
}

U32 ShowProgress(U32 Now, U32 Total, U32 Last)
{
    U32 Div;

    Div = (Now * 100) / Total;
    if ((Div/10) != (Last/10))
        STSMART_Print(("%d%%  ", Div));
    if (Div == 100)
        STSMART_Print(("\n"));
    return Div;
}


/* End of module */


