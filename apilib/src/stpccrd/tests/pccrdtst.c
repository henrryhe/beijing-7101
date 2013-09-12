/*****************************************************************************
File Name   : pccrdtst.c

Description : Test harness for the STPCCRD driver

History     : Support for the Tylko/STi5518 and Mocha/STi5514 is added.

              Tested on mb400/STb5105, mb390/STi5100 & mb391/STi7710.

Copyright (C) 2005 STMicroelectronics

Reference   : ST API Definition "STPCCRD Driver API" STB-API-332
              STPCCRD Driver Test Plan V1.2
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stddef.h>
#include <stdio.h>              /* C libs */
#include <string.h>

#if defined(ST_OSLINUX)
#include <unistd.h>
#endif

#if defined(ST_OSLINUX)
#define  STTBX_Print(x) printf x
#include "compat.h"
#else
#include "sttbx.h"
#endif

/* STAPI Includes */
#include "stdevice.h"
#include "stlite.h"
#include "stddefs.h"

#if !defined(ST_OSLINUX)
#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
#include "sti2c.h"
#include "stpio.h"
#endif
#endif/*OS_LINUX*/

#include "stevt.h"
#include "stboot.h"
#include "stcommon.h"
#include "stsys.h"
#include "stpccrd.h"
#include "stcommon.h"
#include "stos.h"

#ifndef STPCCRD_DISABLE_TESTTOOL
#include "testtool.h"
#endif /* STPCCRD_DISABLE_TESTTOOL */

/* Private Includes */
#include "pccrdtst.h"

/* Private variables ------------------------------------------------------ */

extern const U8 Revision[];

#ifndef STPCCRD_DISABLE_TESTTOOL
TESTTOOL_InitParams_t TST_InitParams;    /* Testtool init params */
#endif /* STPCCRD_DISABLE_TESTTOOL */

/* For storing global clock information */
static ST_ClockInfo_t ClockInfo;

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
static ST_ErrorCode_t  MultipleClientErrCode;
/* For Multiple Client Test Info */
static U8              Multiple_Task_Done;
static U32             Multi_Client_Result_Failed;

#if defined(ST_OSLINUX)
static pthread_t task_multiclients[1];
static pthread_attr_t *tattr_multithread_p = NULL;
static void *myParams_multiclients_p=NULL;
#endif

#endif

/* Device names -- used for all tests */
static ST_DeviceName_t  PCCRDDeviceName[MAX_PCCRD_DEVICE+1];

#if defined(ST_OSLINUX)

#define I2C_BACK_BUS            0
#define I2C_FRONT_BUS           1
ST_DeviceName_t I2CDeviceName[] = { "I2C_BACK", "I2C_FRONT" };

#else

#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
static ST_DeviceName_t  PioDeviceName[MAX_PIO+1];
static ST_DeviceName_t  I2CDeviceName[MAX_I2C+1];
#endif
#endif/*OS_LINUX*/

/* PC card Init params */
static STPCCRD_InitParams_t  PCCRD_InitParams[MAX_PCCRD_DEVICE];

#ifndef ST_OSLINUX
#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
/* PIO Init params */
static STPIO_InitParams_t    PioInitParams[MAX_ACTUAL_PIO];

/* I2C Init params */
static STI2C_InitParams_t    STI2C_InitParams[MAX_I2C];
#endif
#endif/*(ST_OSLINUX))*/

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
static ST_DeviceName_t    EVTDeviceName[MAX_EVT+1];
static STEVT_InitParams_t EVTInitParams; /* Event handler init params */
#endif /* STPCCRD_HOT_PLUGIN_WITH_STEVT */
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

/* Global semaphore used in callbacks */
#if defined(ST_OS20)
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
static semaphore_t           MultipleTaskSem;
#endif
#endif
/* Global semaphore used in callbacks */
static semaphore_t *EventSemaphore_p;
static semaphore_t *Lock_p;

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
static semaphore_t           *MultipleTaskSem_p;
#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED

static STPCCRD_EventType_t LastEvent;
static ST_ErrorCode_t      LastError;

/* Encapsulates events that come from the PC card
 * driver.  We must maintain a queue of events as they tend to be
 * generated too quickly for the caller to process them.
 */
typedef struct {
    STPCCRD_Handle_t    Handle;
    STPCCRD_EventType_t Event;
    ST_ErrorCode_t      ErrorCode;
} PCCRD_Notify_t;

/* Notify queue */
static PCCRD_Notify_t   MsgQueue[MQUEUE_SIZE];

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

static U8 WriteOffset = 0;
static U8 ReadOffset  = 0;

#define AWAIT_ANY_EVENT        ((U32)-1)
#define FlushMessageQueue()    WriteOffset = ReadOffset; \
                   while ( STOS_SemaphoreWaitTimeOut(  \
                               EventSemaphore_p, TIMEOUT_IMMEDIATE ) == 0 ) ;

/* Declarations for memory partitions */
#if defined(ST_OS20)
/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_7710) || defined(ST_5100) || defined(ST_5301) \
|| defined(ST_5105) || defined(ST_5528) || defined(ST_5107)
#if defined(UNIFIED_MEMORY)
#define                 NCACHE_MEMORY_SIZE          0x80000
#else
#define                 NCACHE_MEMORY_SIZE          0x200000
#endif
#elif defined(ST_5518)
#if defined(maly) || defined(tylko) || defined(mb5518)
#define NCACHE_MEMORY_SIZE      0x40000         /*256K, although 512K reserved for NCache*/
#else
#define NCACHE_MEMORY_SIZE      0x80000 - 0x10  /*512K in case of HDD application implemented */
#endif
#endif

static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
#pragma ST_section      ( external_block, "system_section")


partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;
partition_t             *ncache_partition = &the_ncache_partition;

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

#if !defined(ST_5514) && !defined(ST_5517) && !defined(ST_5516) && !defined(ST_5518)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#ifdef UNIFIED_MEMORY
static unsigned char    ncache2_block[1];
#pragma ST_section      ( ncache2_block, "ncache2_section")
#endif

#elif defined(ST_OS21)

#define SYSTEM_PARTITION_SIZE     0x100000
#define NCACHE_PARTITION_SIZE     0x10000
static unsigned char              external_block[SYSTEM_PARTITION_SIZE];
ST_Partition_t                    *system_partition;
static unsigned char              ncache_block[NCACHE_PARTITION_SIZE];
ST_Partition_t                    *NcachePartition;

#elif defined(ST_OSLINUX)/* if defined ST_OS21 */
#define SYSTEM_PARTITION_SIZE     0x1
#define NCACHE_PARTITION_SIZE     0x1
ST_Partition_t                    *DriverPartition = (ST_Partition_t*)1;
ST_Partition_t                    *system_partition = (ST_Partition_t*)1;
ST_Partition_t                    *NcachePartition = (ST_Partition_t*)1;
#endif /* end of OS21 */

/*all memory partition info are commented out for ST_OSLINUX*/

/* Private function prototypes -------------------------------------------- */

static ST_ErrorCode_t GlobalInit(void);
static ST_ErrorCode_t GlobalTerm(void);
char   *PCCRD_EventToString(STPCCRD_EventType_t Event);

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
static PCCRD_TestResult_t PCCRDInsertRemove(PCCRD_TestParams_t *Params_p);
#endif  /* STPCCRD_HOT_PLUGIN_ENABLED */

static PCCRD_TestResult_t PCCRDAPI(PCCRD_TestParams_t *Params_p);
static PCCRD_TestResult_t PCCRDStackUsage(PCCRD_TestParams_t *Params_p);
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
static PCCRD_TestResult_t PCCRDMultipleClients(PCCRD_TestParams_t *Params_p);
#endif
static PCCRD_TestResult_t PCCRDConfigurationTest(PCCRD_TestParams_t *Params_p);
static PCCRD_TestResult_t PCCRD_DVBCI_Test(PCCRD_TestParams_t *Params_p);

#ifndef STPCCRD_DISABLE_TESTTOOL
static BOOL tt_pccrdapi( parse_t *pars_p, char *result_sym_p );
static BOOL tt_pccrdstackusage( parse_t *pars_p, char *result_sym_p );
static BOOL tt_pccrdconfig( parse_t *pars_p, char *result_sym_p );
static BOOL tt_pccrdvbci( parse_t *pars_p, char *result_sym_p );
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
static BOOL tt_pccrdmultipleclient( parse_t *pars_p, char *result_sym_p );
#endif
#ifdef  STPCCRD_HOT_PLUGIN_ENABLED
static BOOL tt_pccrdinsertremove( parse_t *pars_p, char *result_sym_p );
#endif
#endif /* STPCCRD_DISABLE_TESTTOOL */

/* Initialize table of TestEntry Struct with TestFunctions and required parameters */
static PCCRD_TestEntry_t TestTable[] =
{
    {
        PCCRDAPI,
        "API Test for Module A",
        1,
        STPCCRD_MOD_A
    },
#if !defined(ST_5105) && !defined(ST_5107) && !defined(STPCCRD_USE_ONE_MODULE)
    {
        PCCRDAPI,
        "API Test for Module B",
        1,
        STPCCRD_MOD_B
    },
#endif
#if !defined(ST_OSLINUX)
    {
        PCCRDStackUsage,
        "Stack and Data Usage Test for Module A",
        1,
        STPCCRD_MOD_A
    },

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
    {
        PCCRDStackUsage,
        "Stack and Data Usage Test for Module B",
        1,
        STPCCRD_MOD_B
    },
#endif
#endif/*ST_OSLINUX*/
    {
        PCCRDConfigurationTest,
        "Configuration Test for Module A",
        1,
        STPCCRD_MOD_A
    },

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
    {
        PCCRDConfigurationTest,
        "Configuration Test for Module B",
        1,
        STPCCRD_MOD_B
    },
#endif

    {
        PCCRD_DVBCI_Test,
        "DVB CI Communication Test for Module A",
        1,
        STPCCRD_MOD_A
    },

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
    {
        PCCRD_DVBCI_Test,
        "DVB CI Communication Test for Module B",
        1,
        STPCCRD_MOD_B
    },
    {
      PCCRDMultipleClients,
      "Multiple Client Test",
      1,
      STPCCRD_MOD_B
    },

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    {
        PCCRDInsertRemove,
        "Insert/Remove Test",
        1,
        STPCCRD_MOD_B
    },
#endif
#endif  /* not defined ST_5105 & STPCCRD_USE_ONE_MODULE*/
    { NULL, "No Test", 1, STPCCRD_MOD_B }
};

/* Function definitions --------------------------------------------------- */

/*int main(int argc, char *argv[])*/
int main (void)
{
    ST_ErrorCode_t          ErrCode;

#if !defined(ST_OSLINUX)
    STTBX_InitParams_t      TBXInitParams;
#endif
#if !defined(ST_OSLINUX)
    STBOOT_InitParams_t     BootParams;
    STBOOT_TermParams_t     BootTerm;
    STBOOT_DCache_Area_t    DCacheMap[] =
    {
        { (U32 *)DCACHE_START, (U32 *)DCACHE_END }, /* Cacheable */
        { NULL, NULL }                              /* End */
    };
#endif /*(ST_OSLINUX)*/

#ifdef STPCCRD_DISABLE_TESTTOOL
    U32                 Section = 1;
    PCCRD_TestParams_t  TestParams;
    PCCRD_TestEntry_t   *Test_p;
    PCCRD_TestResult_t  Total = TEST_RESULT_ZERO, Result;
#else
    BOOL                RetCode;
#endif /* STPCCRD_DISABLE_TESTTOOL */

#if !defined(ST_OSLINUX)
#if defined(ST_OS20)


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
    system_block_noinit[0]   = 0;
#if !defined(ST_5514) && !defined(ST_5517) && !defined(ST_5516) && !defined(ST_5518)
    data_section[0]          = 0;
#endif

#ifdef UNIFIED_MEMORY
    ncache2_block[0] = 0;
#endif

#else /* defined ST_OS21 */

    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));
    NcachePartition  = partition_create_heap((void *)(ST40_NOCACHE_NOTRANSLATE(ncache_block)),
                                              sizeof(ncache_block));

#endif /* end of ST_OS21 */

    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency   = (U32)SDRAM_FREQUENCY;
    BootParams.ICacheEnabled    = ICACHE;
    BootParams.DCacheMap        = DCacheMap;
    BootParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

    /* Constant, defined in include; set automatically for
     * board configuration
     */
    BootParams.MemorySize = (STBOOT_DramMemorySize_t)SDRAM_SIZE;

    ErrCode = STBOOT_Init( "boot", &BootParams );
    if (ErrCode != ST_NO_ERROR)
    {
        printf("Unable to initialize boot %d\n", ErrCode);
        return 0;
    }

    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
#if defined(ST_7710)
    TBXInitParams.CPUPartition_p      = ncache_partition;
#else
    TBXInitParams.CPUPartition_p      = system_partition;
#endif
    ErrCode = STTBX_Init( "tbx", &TBXInitParams );

#endif /*(ST_OSLINUX)*/

    /* Obtain subsystem clock speeds */
    ST_GetClockInfo(&ClockInfo);

    PCCRD_DebugMessage("******************************************************");
    PCCRD_DebugMessage("                STPCCRD Test Harness                  ");
    PCCRD_DebugPrintf(("Driver Revision: %s\n", STPCCRD_GetRevision()));
    PCCRD_DebugPrintf(("Test Harness Revision: %s\n", Revision));
    PCCRD_DebugPrintf(("Clock Speed: %d\n", CLOCK_CPU));
    PCCRD_DebugPrintf(("Comms Block: %d\n", CLOCK_COMMS));

#if !defined(ST_OSLINUX)
#if defined(ST_7100) || defined(ST_7109)
    PCCRD_DebugPrintf(("EPLD Version: 0x%x\n",*(U32*)(0xA3000000)));
#endif
#endif

    PCCRD_DebugMessage("******************************************************");

    /* Initialize Global parameters */
    ErrCode = GlobalInit();

#if !defined(ST_OSLINUX)
    (void) task_priority_set(NULL, MIN_USER_PRIORITY);
#endif/*ST_OSLINUX*/
    if (ErrCode != ST_NO_ERROR)
    {
        return 0;
    }

#ifdef STPCCRD_DISABLE_TESTTOOL

    Test_p = TestTable;
    while(Test_p->TestFunction != NULL)
    {
        U8 Repeat = Test_p->RepeatCount;
        while (Repeat > 0)  /* Repeat the test till given count exhausts */
        {
            PCCRD_DebugMessage("******************************************************");
            PCCRD_DebugPrintf(("SECTION %d - %s\n", Section,
                              Test_p->TestInfo));
            PCCRD_DebugMessage("******************************************************");

            /* Set up parameters for test function */
            TestParams.Ref = Section;
            TestParams.Slot = Test_p->Slot;

            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            FlushMessageQueue();

            /* Update counters as according the outcome of test performed */
            Total.NumberPassed += Result.NumberPassed;
            Total.NumberFailed += Result.NumberFailed;
            Repeat--;
        }

        Test_p++;           /* Proceed to next */
        Section++;
    }

#else

#if defined(ST_7710)
    TST_InitParams.CPUPartition_p      = ncache_partition;
#else
    TST_InitParams.CPUPartition_p      = system_partition;
#endif
    TST_InitParams.NbMaxOfSymbols = 1000;

    if (STTST_Init(&TST_InitParams) == TRUE )
    {
        PCCRD_DebugMessage("STTST_Init()=Failed");
        return (ST_ERROR_BAD_PARAMETER);
    }

    RetCode = STTST_RegisterCommand( "API_TEST", tt_pccrdapi,
                                     "PC Card API Testing");
    RetCode = STTST_RegisterCommand( "STACK_USAGE", tt_pccrdstackusage,
                                     "PC Card Stack Usage Testing");
    RetCode = STTST_RegisterCommand( "CONFIG_TEST", tt_pccrdconfig,
                                     "PC Card Configuration Testing");
    RetCode = STTST_RegisterCommand( "DVBCI_TEST", tt_pccrdvbci,
                                     "PC Card DVBCI Stack Testing \n This test must be run after CONFIGTEST is successful");
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
    RetCode = STTST_RegisterCommand( "MULTIPLE_CLIENT", tt_pccrdmultipleclient,
                                     "PC Card Multiple Client Testing");
#endif
#ifdef STPCCRD_HOT_PLUGIN_ENABLED

    RetCode = STTST_RegisterCommand( "INSERT_REMOVE_TEST", tt_pccrdinsertremove,
                                     "PC Card Hot Swapping - Insert Remove Testing");
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    if ( RetCode == FALSE ) /* If everything done well */
    {
        RetCode = STTST_Start();
    }
    else
    {
        PCCRD_DebugMessage("Error in Command Registeration of Testtool!");
        return (ST_ERROR_BAD_PARAMETER);
    }

#endif /* STPCCRD_DISABLE_TESTTOOL */

    PCCRD_DebugMessage("**************************************************");
    GlobalTerm();

#ifdef STPCCRD_DISABLE_TESTTOOL
    /* Output running analysis */
    PCCRD_DebugMessage("**************************************************");
    PCCRD_DebugPrintf(("PASSED: %d\n", Total.NumberPassed));
    PCCRD_DebugPrintf(("FAILED: %d\n", Total.NumberFailed));
    PCCRD_DebugMessage("**************************************************");
#endif /* STPCCRD_DISABLE_TESTTOOL */

#if !defined(ST_OSLINUX)
    STBOOT_Term("boot", &BootTerm);
#endif/*(ST_OSLINUX)*/
    return 0;
}

#ifdef STPCCRD_HOT_PLUGIN_ENABLED

#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
#define CallbackNotifyFunction  NULL
/******************************************************************************
Name        : EvtNotifyFunction.

Description : Callback function registered with the STEVT events .
******************************************************************************/
void EvtNotifyFunction( STEVT_CallReason_t    Reason,
                        const ST_DeviceName_t RegistrantName,
                        STEVT_EventConstant_t Event,
                        const void *EventDataconst ,
                        const void *SubscriberData_p)
{
    ST_ErrorCode_t ErrorCode;


    ErrorCode = ST_NO_ERROR;
    STOS_SemaphoreWait(Lock_p);          /* Stop re-entrancy */

    LastEvent = Event;
    LastError = ErrorCode;
    MsgQueue[WriteOffset].Event     = Event;
    MsgQueue[WriteOffset].ErrorCode = ErrorCode;
    if (++WriteOffset >= MQUEUE_SIZE)
    {
        WriteOffset = 0;
    }
    STOS_SemaphoreSignal(Lock_p);
    STOS_SemaphoreSignal(EventSemaphore_p);   /* Signal 'new event' */

}

/******************************************************************************
Name        : SubscribeEvents.

Description : Subscribes events for which function want to be notified.

Return      : STPCCRD_ERROR_EVT - On Failure
              ST_NO_ERROR       - On Success.
******************************************************************************/

static ST_ErrorCode_t SubscribeEvents (const ST_DeviceName_t  EvtHandlerName)
{
    STEVT_OpenParams_t            EVTOpenParams;
    STEVT_Handle_t                Handle;
    STEVT_DeviceSubscribeParams_t SubscribeParams;

    SubscribeParams.NotifyCallback     = EvtNotifyFunction;
   /* SubscribeParams.RegisterCallback   = NULL;*/
    /*SubscribeParams.UnregisterCallback = NULL;*/
    SubscribeParams.SubscriberData_p   = NULL;

    EVTOpenParams.dummy = 0;

    /* First open the specified event handler */
    if (STEVT_Open (EvtHandlerName, &EVTOpenParams,
                   &Handle) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    /* Now Subscribe desirable events notification */
    if (STEVT_SubscribeDeviceEvent( Handle, PCCRDDeviceName[FIRST_INSTANCE],
                                    STPCCRD_EVT_MOD_A_INSERTED,&SubscribeParams) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    if (STEVT_SubscribeDeviceEvent( Handle,PCCRDDeviceName[FIRST_INSTANCE],
                                    STPCCRD_EVT_MOD_B_INSERTED, &SubscribeParams) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    if (STEVT_SubscribeDeviceEvent( Handle, PCCRDDeviceName[FIRST_INSTANCE],
                                    STPCCRD_EVT_MOD_A_REMOVED,&SubscribeParams) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    if (STEVT_SubscribeDeviceEvent( Handle, PCCRDDeviceName[FIRST_INSTANCE],
                                     STPCCRD_EVT_MOD_B_REMOVED,&SubscribeParams) != ST_NO_ERROR)
    {
        return (STPCCRD_ERROR_EVT);
    }

    return (ST_NO_ERROR);
}
#else

#define CallbackNotifyFunction  DirectNotifyFunction
/*****************************************************************************
Name        : DirectNotifyFunction.

Description : Callback function for other tests.
****************************************************************************/

void DirectNotifyFunction(STPCCRD_Handle_t Handle, STPCCRD_EventType_t Event,
                          ST_ErrorCode_t ErrorCode)
{
    STOS_SemaphoreWait(Lock_p);          /* Stop re-entrancy */

    LastEvent = Event;
    LastError = ErrorCode;
    MsgQueue[WriteOffset].Event     = Event;
    MsgQueue[WriteOffset].ErrorCode = ErrorCode;
    if (++WriteOffset >= MQUEUE_SIZE)
    {
        WriteOffset = 0;
    }
    STOS_SemaphoreSignal(Lock_p);
    STOS_SemaphoreSignal(EventSemaphore_p);   /* Signal 'new event' */
}

/*****************************************************************************
Name        : DirectNotifyCardA.

Description : Callback function specific to Card A to test for its Insertion/
              Removal.
****************************************************************************/

void DirectNotifyCardA(STPCCRD_Handle_t Handle, STPCCRD_EventType_t Event,
                          ST_ErrorCode_t ErrorCode)
{
    STOS_SemaphoreWait(Lock_p);          /* Stop re-entrancy */

    if ((Event == STPCCRD_EVT_MOD_A_INSERTED)||(Event==STPCCRD_EVT_MOD_A_REMOVED))
    {
        LastEvent = Event;
        LastError = ErrorCode;
        MsgQueue[WriteOffset].Event     = Event;
        MsgQueue[WriteOffset].ErrorCode = ErrorCode;
        if (++WriteOffset >= MQUEUE_SIZE)
        {
            WriteOffset = 0;
        }
        STOS_SemaphoreSignal(Lock_p);
        STOS_SemaphoreSignal(EventSemaphore_p);   /* Signal 'new event' */
    }
    else
    {
        STOS_SemaphoreSignal(Lock_p);
    }
}

/*****************************************************************************
Name        : DirectNotifyCardB.

Description : Callback function specific to Card B to test for its Insertion/
              Removal.
****************************************************************************/

void DirectNotifyCardB(STPCCRD_Handle_t Handle, STPCCRD_EventType_t Event,
                          ST_ErrorCode_t ErrorCode)
{

    STOS_SemaphoreWait(Lock_p);          /* Stop re-entrancy */

    if ((Event == STPCCRD_EVT_MOD_B_INSERTED)||(Event == STPCCRD_EVT_MOD_B_REMOVED))
    {
        LastEvent = Event;
        LastError = ErrorCode;
        MsgQueue[WriteOffset].Event     = Event;
        MsgQueue[WriteOffset].ErrorCode = ErrorCode;
        if (++WriteOffset >= MQUEUE_SIZE)
        {
            WriteOffset = 0;
        }

    STOS_SemaphoreSignal(Lock_p);
    STOS_SemaphoreSignal(EventSemaphore_p);   /* Signal 'new event' */
    }
    else
    {
        STOS_SemaphoreSignal(Lock_p);
    }
}
#endif
/*****************************************************************************
Name        : AwaitNotifyEvent.

Description : Provides a convenient way of awaiting on Module Insertion
              and Removal events. The caller may specify a specific event
              to wait for or 'AWAIT_ANY_EVENT'.

Parameters  : NeedEvent,     Event type or AWAIT_ANY_EVENT.
              Event_p,       Pointer to area to store event that has occurred.
              Error_p,       Pointer to area to store error associated with event.

See Also    : NotifyFunction().
*****************************************************************************/

void AwaitNotifyEvent(STPCCRD_EventType_t NeedEvent,
                      STPCCRD_EventType_t *Event_p,
                      ST_ErrorCode_t *Error_p)
{
    STPCCRD_EventType_t Evt;
    do
    {
        STOS_SemaphoreWait(EventSemaphore_p);
        Evt = MsgQueue[ReadOffset].Event;
        if (Event_p != NULL)
        {
            *Event_p = MsgQueue[ReadOffset].Event;
        }
        if (Error_p != NULL)
        {
            *Error_p = MsgQueue[ReadOffset].ErrorCode;
        }
        if (++ReadOffset >= MQUEUE_SIZE)
        {
            ReadOffset = 0;
        }

    } while (NeedEvent != Evt && NeedEvent != AWAIT_ANY_EVENT);
}

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
/*****************************************************************************
Name         : GlobalInit.

Description  : Intialise All the dependent Driver Including the STPCCRD

Parameters   : None

Return Value : ST_NO_ERROR - in case of no error,
               Errors As given by the drivers while Calling STXXX_Init().
*****************************************************************************/

static ST_ErrorCode_t GlobalInit(void)
{
    ST_ErrorCode_t       ErrCode = ST_NO_ERROR;
    U32                  Index;
    ST_DeviceName_t      DeviceName;

#if !defined(ST_OSLINUX)
#if defined(ST_5514) || defined(ST_5518)
    STPIO_BitMask_t      BitMask;
    STPIO_OpenParams_t   CIRSTOpenParams;
    STPIO_Handle_t       CIRSTHandle;
#endif
#endif/*OS_LINUX*/

#if !defined(ST_OSLINUX)
#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105)  \
&& !defined(ST_5107)
    /* Automatically generate device names */
    for (Index = 0; Index < MAX_PIO; Index++)
    {
        sprintf((char *)DeviceName, "STPIO_%d", Index);
        strcpy((char *)PioDeviceName[Index], (char *)DeviceName);
    }

    PioDeviceName[Index][0] = 0;
    PioDeviceName[PIO_DEVICE_NOT_USED][0] = 0;

    for (Index = 0; Index < MAX_I2C; Index++)
    {
        sprintf((char *)DeviceName, "STI2C_%d", Index);
        strcpy((char *)I2CDeviceName[Index], (char *)DeviceName);
    }

    I2CDeviceName[Index][0] = 0;

#endif
#endif/*!ST_OSLINUX*/

    for (Index = 0; Index < MAX_PCCRD_DEVICE; Index++)        /* SC */
    {
        sprintf((char *)DeviceName, "STPCCRD_%d", Index);
        strcpy((char *)PCCRDDeviceName[Index], (char *)DeviceName);
    }

    PCCRDDeviceName[Index][0] = 0;

#ifdef STPCCRD_HOT_PLUGIN_ENABLED

#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
    for (Index = 0; Index < MAX_EVT; Index++)       /* EVT */
    {
        sprintf((char *)DeviceName, "STEVT_%d", Index);
        strcpy((char *)EVTDeviceName[Index], (char *)DeviceName);
    }
    EVTDeviceName[Index][0] = 0;
#endif

#endif

#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
#if !defined(ST_OSLINUX)

    /* Setup PIO initialization parameters */
    PioInitParams[0].DriverPartition = system_partition;
    PioInitParams[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;

    PioInitParams[1].DriverPartition = system_partition;
    PioInitParams[1].BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
    PioInitParams[1].InterruptNumber = PIO_1_INTERRUPT;
    PioInitParams[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;

    PioInitParams[2].DriverPartition = system_partition;
    PioInitParams[2].BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams[2].InterruptNumber = PIO_2_INTERRUPT;
    PioInitParams[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;


    PioInitParams[3].DriverPartition = system_partition;
    PioInitParams[3].BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
    PioInitParams[3].InterruptNumber = PIO_3_INTERRUPT;
    PioInitParams[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;

    PioInitParams[4].DriverPartition = system_partition;
    PioInitParams[4].BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;

#if (MAX_ACTUAL_PIO >= 6)
    PioInitParams[5].DriverPartition = system_partition;
    PioInitParams[5].BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
#endif

#if (MAX_ACTUAL_PIO >= 7)
    PioInitParams[6].DriverPartition = system_partition;
    PioInitParams[6].BaseAddress     = (U32 *)PIO_6_BASE_ADDRESS;
    PioInitParams[6].InterruptNumber = PIO_6_INTERRUPT;
    PioInitParams[6].InterruptLevel  = PIO_6_INTERRUPT_LEVEL;
#endif

#if (MAX_ACTUAL_PIO >= 8)
    PioInitParams[7].DriverPartition = system_partition;
    PioInitParams[7].BaseAddress     = (U32 *)PIO_7_BASE_ADDRESS;
    PioInitParams[7].InterruptNumber = PIO_7_INTERRUPT;
    PioInitParams[7].InterruptLevel  = PIO_7_INTERRUPT_LEVEL;
#endif

    /* Back I2C bus */
    STI2C_InitParams[0].BaseAddress       = (U32 *)SSC_0_BASE_ADDRESS;
    STI2C_InitParams[0].InterruptNumber   = SSC_0_INTERRUPT;
    STI2C_InitParams[0].InterruptLevel    = SSC_0_INTERRUPT_LEVEL;
    STI2C_InitParams[0].PIOforSDA.BitMask = PIO_FOR_SSC0_SDA;
    STI2C_InitParams[0].PIOforSCL.BitMask = PIO_FOR_SSC0_SCL;
    STI2C_InitParams[0].MasterSlave       = STI2C_MASTER;
    STI2C_InitParams[0].BaudRate          = STI2C_RATE_NORMAL;
    STI2C_InitParams[0].MaxHandles        = 8;
    STI2C_InitParams[0].ClockFrequency    = ClockInfo.CommsBlock;
    STI2C_InitParams[0].DriverPartition   = system_partition;

    strcpy(STI2C_InitParams[0].PIOforSDA.PortName, PioDeviceName[BACK_PIO]);
    strcpy(STI2C_InitParams[0].PIOforSCL.PortName, PioDeviceName[BACK_PIO]);

#if !defined(TYLKO) && !defined(ST_5528)
    /* front I2C bus */
    STI2C_InitParams[1].BaseAddress       = (U32 *) SSC_1_BASE_ADDRESS;
    STI2C_InitParams[1].InterruptNumber   = SSC_1_INTERRUPT;
    STI2C_InitParams[1].InterruptLevel    = SSC_1_INTERRUPT_LEVEL;
    STI2C_InitParams[1].PIOforSDA.BitMask = PIO_FOR_SSC1_SDA;
    STI2C_InitParams[1].PIOforSCL.BitMask = PIO_FOR_SSC1_SCL;
    STI2C_InitParams[1].MasterSlave       = STI2C_MASTER;
    STI2C_InitParams[1].BaudRate          = STI2C_RATE_NORMAL;
    STI2C_InitParams[1].MaxHandles        = 8;
    STI2C_InitParams[1].ClockFrequency    = ClockInfo.CommsBlock;
    STI2C_InitParams[1].DriverPartition   = system_partition;

    strcpy(STI2C_InitParams[1].PIOforSCL.PortName, PioDeviceName[FRONT_PIO]);
    strcpy(STI2C_InitParams[1].PIOforSDA.PortName, PioDeviceName[FRONT_PIO]);
#endif /* tylko */
#endif/*OS_LINUX*/

#if defined(ST_5514) || defined(ST_5518) || defined(ST_5528)
#if defined(TYLKO) || defined(MOCHA) || defined(ST_5528)
    strcpy(PCCRD_InitParams[0].I2CDeviceName,I2CDeviceName[I2C_BACK_BUS]);
#endif
#elif defined(ST_5516) || defined(ST_5517)
    strcpy(PCCRD_InitParams[0].I2CDeviceName,I2CDeviceName[I2C_FRONT_BUS]);
#elif defined(ST_7100) || defined(ST_7109)
    strcpy(PCCRD_InitParams[0].I2CDeviceName,I2CDeviceName[I2C_BACK_BUS]);
#endif /* ST_5514 */

#endif/*#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)*/

    PCCRD_InitParams[0].DeviceCode = PCCRD_DEV_CODE;
    PCCRD_InitParams[0].MaxHandle  = MAX_PCCRD_HANDLE;
#if defined(ST_7710)
    PCCRD_InitParams[0].DriverPartition_p = ncache_partition;
#else
    PCCRD_InitParams[0].DriverPartition_p = system_partition;
#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#if defined(ST_5514)
#if defined(MOCHA)
    PCCRD_InitParams[0].InterruptNumber = EXTERNAL_2_INTERRUPT;
#endif
#elif defined(ST_5516) || defined(ST_5517) || defined(ST_7100) || defined(ST_7109)
    PCCRD_InitParams[0].InterruptNumber = EXTERNAL_0_INTERRUPT;
#elif defined(ST_7710)
    PCCRD_InitParams[0].InterruptNumber = EXTERNAL_2_INTERRUPT; /* DDTS 45810 */
#elif defined(ST_5528)
    PCCRD_InitParams[0].InterruptNumber = EXTERNAL_0_INTERRUPT;
#endif /* ST_5514 */

    PCCRD_InitParams[0].InterruptLevel  = EXTERNAL_0_INTERRUPT_LEVEL;

#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
    strcpy(PCCRD_InitParams[0].EvtHandlerName,EVTDeviceName[0]);
#else
    PCCRD_InitParams[0].EvtHandlerName[0] = 0;
#endif /* STPCCRD_HOT_PLUGIN_WITH_STEVT */


/*******************************************************************
    Initialize all devices
*******************************************************************/
    /* EVT */

#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
    /* Event handler initialization */
#if defined(ST_7710)
    EVTInitParams.MemoryPartition = ncache_partition;
#else
    EVTInitParams.MemoryPartition = system_partition;
#endif

    EVTInitParams.MemorySizeFlag  = STEVT_UNKNOWN_SIZE;
    EVTInitParams.EventMaxNum     = MAX_EVT_NUM;
    EVTInitParams.ConnectMaxNum   = MAX_EVT_CONNCT_NUM;
    EVTInitParams.SubscrMaxNum    = MAX_EVT_SUBS_NUM;

    for (Index = 0; Index < MAX_EVT; Index++)   /* Initialize each event handler */
    {
        ErrCode = STEVT_Init(EVTDeviceName[Index], &EVTInitParams);
        PCCRD_DebugError("STEVT_Init()", ErrCode);

        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage("Unable to initialize EVT device");
            return ErrCode;
        }
    }
#endif /* STPCCRD_HOT_PLUGIN_WITH_STEVT */
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

#if !defined(ST_OSLINUX)
#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
    /* PIO */
    for (Index = 0; Index < MAX_ACTUAL_PIO; Index++)
    {
        ErrCode = STPIO_Init(PioDeviceName[Index],
                           &PioInitParams[Index]);
        PCCRD_DebugError("STPIO_Init()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugPrintf(("Unable to initialize %s.\n",
                               PioDeviceName[Index]));
        }
    }

    /* I2C bus */
    for (Index = 0; Index < MAX_I2C; Index++)
    {
        ErrCode =  STI2C_Init(I2CDeviceName[Index],
                                &STI2C_InitParams[Index]);
        PCCRD_DebugError("STI2C_Init()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugPrintf(("Unable to initialize %s.\n",
                               I2CDeviceName[Index]));
        }
    }

#if defined(ST_5514)

#if defined(MOCHA)

    /* Setting Bit 1 of Port 4 as output */
    CIRSTOpenParams.ReservedBits    = PIO_BIT_1;
    BitMask                         = CIRSTOpenParams.ReservedBits;
    CIRSTOpenParams.BitConfigure[1] = STPIO_BIT_OUTPUT;
    CIRSTOpenParams.IntHandler      = NULL;

    /* Open PIO if necessary */
    if ((ErrCode = STPIO_Open("STPIO_4", &CIRSTOpenParams, &CIRSTHandle)) != ST_NO_ERROR )
    {
        return ErrCode;
    }

    if ((ErrCode = STPIO_Close(CIRSTHandle)) != ST_NO_ERROR )
    {
        return ErrCode;
    }
#endif /*MOCHA*/

#elif defined(ST_5518)

#if defined(TYLKO)
    /* Manage RESET pin of STV0700 VIA PIO0(2) */
    CIRSTOpenParams.ReservedBits    = PIO_BIT_2;
    BitMask                         = CIRSTOpenParams.ReservedBits;
    CIRSTOpenParams.BitConfigure[2] = STPIO_BIT_OUTPUT; /* STPIO_BIT_BIDIRECTIONAL; */
    CIRSTOpenParams.IntHandler      = NULL;

    /* Open PIO if necessary */
    if ( (ErrCode = STPIO_Open("STPIO_0", &CIRSTOpenParams, &CIRSTHandle)) != ST_NO_ERROR )
    {
            return ErrCode;
    }
    if ((ErrCode = STPIO_Close(CIRSTHandle)) != ST_NO_ERROR )
    {
        return ErrCode;
    }
#endif /*TYLKO*/

#endif /*ST_5514 or ST_5518*/

#endif/*!defined ST_7710*/
#endif/*!OS_LINUX*/

    /* PCCRD Device */
    for (Index = 0;Index < MAX_PCCRD_DEVICE; Index++)
    {
        ErrCode =  STPCCRD_Init(PCCRDDeviceName[Index],
                                &PCCRD_InitParams[Index]);
        PCCRD_DebugError("STPCCRD_Init()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugPrintf(("Unable to initialize %s.\n",
                               PCCRDDeviceName[Index]));
        }
    }

        /* Setup global semaphores */
    EventSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    Lock_p = STOS_SemaphoreCreateFifo(NULL,1);

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
    SubscribeEvents(PCCRD_InitParams[0].EvtHandlerName);
#endif /* STPCCRD_HOT_PLUGIN_WITH_STEVT */
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

    return ErrCode;
}
/*****************************************************************************
Name         : GlobalTerm.

Description  : Terminate All the dependent Driver Including the STPCCRD ,
               de-allocate all memory structs ,variables and semaphores .

Return Value : ST_NO_ERROR - in case of no error ,
               Errors As given by the drivers while Calling STXXX_Init().
*****************************************************************************/

static ST_ErrorCode_t GlobalTerm(void)
{
    ST_ErrorCode_t ErrCode = ST_NO_ERROR;
    ST_DeviceName_t      DeviceName;
    U32            Index;

    /* Deleting the Semaphore */
    STOS_SemaphoreDelete(NULL,Lock_p);
    STOS_SemaphoreDelete(NULL,EventSemaphore_p);

#ifdef STPCCRD_TESTTOOL
    /* TESTTOOL */
    {
        if ( STTST_Term() )
        {
            PCCRD_DebugMessage("STTST_Term()=Failed");
        }
    }
#endif /* STPCCRD_TESTTOOL */

    /* STPCCRD */
    for (Index = 0; Index < MAX_PCCRD_DEVICE; Index++)
    {
        STPCCRD_TermParams_t    TermParams;
        sprintf((char *)DeviceName, "STPCCRD_%d", Index);
        strcpy((char *)PCCRDDeviceName[Index], (char *)DeviceName);
        TermParams.ForceTerminate = TRUE;

        ErrCode = STPCCRD_Term(PCCRDDeviceName[Index],&TermParams);
        PCCRD_DebugError("STPCCRD_Term()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugPrintf(("Unable to terminate %s\n",
                               PCCRDDeviceName[Index]));
        }
    }

#ifdef STPCCRD_HOT_PLUGIN_ENABLED

#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
        /* Terminate event handlers */
        {
            U8 Index;
            STEVT_TermParams_t TermParams;
            for (Index = 0; Index < MAX_EVT; Index++)
            {
                TermParams.ForceTerminate = TRUE;
                ErrCode = STEVT_Term(EVTDeviceName[Index], &TermParams);
                PCCRD_DebugError("STEVT_Term()", ErrCode);
                if (ErrCode != ST_NO_ERROR)
                {
                    PCCRD_DebugPrintf(("Unable to terminate %s.\n",
                                       EVTDeviceName[Index]));
                }
            }
        }
#endif /* STPCCRD_HOT_PLUGIN_WITH_STEVT */
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

#if !defined(ST_OSLINUX)
#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105) \
&& !defined(ST_5107)
    /* I2C */
    for (Index = 0; Index < MAX_I2C; Index++)
    {
        STI2C_TermParams_t TermParams;
        TermParams.ForceTerminate = TRUE;
        ErrCode = STI2C_Term(I2CDeviceName[Index],&TermParams);
        PCCRD_DebugError("STI2C_Term()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugPrintf(("Unable to terminate %s.\n",I2CDeviceName[Index]));
        }
    }

    /* PIO */
    for (Index = 0; Index < MAX_ACTUAL_PIO; Index++)
    {
        STPIO_TermParams_t TermParams;
        ErrCode = STPIO_Term(PioDeviceName[Index],&TermParams);
        PCCRD_DebugError("STPIO_Term()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugPrintf(("Unable to terminate %s.\n",PioDeviceName[Index]));
        }
    }

#endif
#endif/*!OS_LINUX*/
    return ErrCode;
}

/* Test harness tests ---------------------------------------------------------*/
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
/**********************************************************************************
Name        : PCCRDInsertRemove.

Description : Test for exhaustively testing event notification for card insert and
              remove.

Parameters  : Params_p,       pointer to test params block.

Return Value: PCCRD_TestResult_t -- reflects the number of tests that pass/fail.
***********************************************************************************/

static PCCRD_TestResult_t PCCRDInsertRemove(PCCRD_TestParams_t *Params_p)
{
    PCCRD_TestResult_t Result = TEST_RESULT_ZERO;
    STPCCRD_Handle_t   HandleA,HandleB;
    ST_ErrorCode_t     ErrCode;
    U32                TestCount;

    PCCRD_DebugMessage("Ensures Insertion/Removal events are correctly notified ");
    /* Opening the Clients */
    {
        STPCCRD_OpenParams_t OpenParams;
        /* Setup the open params */
        /* For Module A */
#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#else
        OpenParams.NotifyFunction = DirectNotifyCardA;
#endif
        OpenParams.ModName = STPCCRD_MOD_A;

        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE] ,
                             &OpenParams,&HandleA);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Unable to open", GetErrorText(ErrCode));
            goto ir_end;
        }

#ifndef STV0701
        /* For Module B */
#ifdef STPCCRD_HOT_PLUGIN_WITH_STEVT
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#else
        OpenParams.NotifyFunction = DirectNotifyCardB;
#endif
        OpenParams.ModName=STPCCRD_MOD_B;
        ErrCode = STPCCRD_Open( PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,&HandleB);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Unable to open", GetErrorText(ErrCode));
            goto ir_end;
        }
#endif /* STV0701 */
    }/* Opening the Clients */

    /* Checking if the card is inserted before commencing */
    {
        STPCCRD_EventType_t Event;
        STPCCRD_EventType_t ExpectedEventA,ExpectedEventB;

        ErrCode = STPCCRD_ModCheck(HandleA);
        if (ErrCode == ST_NO_ERROR)
        {
            PCCRD_DebugMessage("Card A is Present");
            ExpectedEventA=STPCCRD_EVT_MOD_A_REMOVED;
        }
        else
        {
            if (ErrCode == STPCCRD_ERROR_NO_MODULE)
            {
                PCCRD_DebugMessage("Card A is Not Present");
                ExpectedEventA = STPCCRD_EVT_MOD_A_INSERTED;
            }
        }
        if (ErrCode == ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_DebugMessage("ST_ERROR_INVALID_HANDLE");
        }

#ifndef STV0701
        ErrCode = STPCCRD_ModCheck(HandleB);
        if (ErrCode == ST_NO_ERROR)
        {
            PCCRD_DebugMessage("Card B is Present");
            ExpectedEventB = STPCCRD_EVT_MOD_B_REMOVED;

        }
        else
        {
            if (ErrCode == STPCCRD_ERROR_NO_MODULE)
            {
                PCCRD_DebugMessage("Card B is Not Present");
                ExpectedEventB = STPCCRD_EVT_MOD_B_INSERTED;
            }
        }
#endif /* STV0701 */

        PCCRD_DebugMessage("Insert/Remove Cards to Proceed ");
        for (TestCount = 0; TestCount < INSRT_RMV_TST_RETRY; TestCount++)
        {
            AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &ErrCode);
            if ((Event == ExpectedEventA)||(Event == ExpectedEventB))
            {
                switch(Event)
                {
                    case  STPCCRD_EVT_MOD_A_INSERTED:
                        {
                            PCCRD_DebugMessage("Card A Inserted");
                            ExpectedEventA = STPCCRD_EVT_MOD_A_REMOVED;
                            break;
                        }
                    case  STPCCRD_EVT_MOD_B_INSERTED:
                        {
                            PCCRD_DebugMessage("Card B Inserted");
                            ExpectedEventB = STPCCRD_EVT_MOD_B_REMOVED;
                            break;
                        }
                    case  STPCCRD_EVT_MOD_A_REMOVED:
                        {
                            PCCRD_DebugMessage("Card A Removed");
                            ExpectedEventA = STPCCRD_EVT_MOD_A_INSERTED;
                            break;
                        }
                    case  STPCCRD_EVT_MOD_B_REMOVED:
                        {
                            PCCRD_DebugMessage("Card B Removed");
                            ExpectedEventB = STPCCRD_EVT_MOD_B_INSERTED;
                            break;
                        }
                 }
            }
            else
            {
                break;
            }
        }/* For Loop */

        if (TestCount == INSRT_RMV_TST_RETRY)
        {
            PCCRD_TestPassed(Result);
        }
        else
        {
            PCCRD_TestFailed(Result, "Incorrect event",
                             PCCRD_EventToString(Event));
        }
    }
ir_end:
    {
        ErrCode = STPCCRD_Close(HandleA);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);

        ErrCode = STPCCRD_Close(HandleB);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);
    }

    return Result;

} /* PCCRDInsertRemove() */

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
/*****************************************************************************
Name        : task_one.

Description : trying to Access the module in Mutiple Client Test.

Parameters  : Handle_p , Valid handle reference to STPCCRD struct.
*****************************************************************************/

void task_one(STPCCRD_Handle_t *Handle_p)
{
    ST_ErrorCode_t          ErrCode;
    STPCCRD_Handle_t        Handle;

    Handle = *Handle_p;
    PCCRD_DebugMessage("Task One Doing ModReset");
    ErrCode = STPCCRD_CIReset(Handle);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_DEVICE_BUSY))
    {
        Multi_Client_Result_Failed++;
    }
    PCCRD_DebugError("Task One STPCCRD_ModReset()", ErrCode);
    Multiple_Task_Done++;

    for(;;)
    {
        task_delay(1000);
        if (Multiple_Task_Done == NUM_MULTIPLE_TASK)
        {
            break;
        }
    }
    STOS_SemaphoreSignal(MultipleTaskSem_p);
    return;
}
/*****************************************************************************
Name        : task_two.

Description : trying to Access the module in Mutiple Client Test.

Parameters  : Handle_p , Valid handle reference to STPCCRD struct.
*****************************************************************************/

void task_two(STPCCRD_Handle_t *Handle_p)
{
    ST_ErrorCode_t          ErrCode;
    STPCCRD_Handle_t        Handle;

    Handle = *Handle_p;
    PCCRD_DebugMessage("Task Two Doing ModReset");
    ErrCode = STPCCRD_CIReset(Handle);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_DEVICE_BUSY))
    {
        Multi_Client_Result_Failed++;
    }
    PCCRD_DebugError("Task Two STPCCRD_ModReset()", ErrCode);
    Multiple_Task_Done++;

    for (;;)
    {
        task_delay(1000);
        if (Multiple_Task_Done == NUM_MULTIPLE_TASK)
        {
            break;
        }
    }
    STOS_SemaphoreSignal(MultipleTaskSem_p);
    return;
}
/*****************************************************************************
Name        : task_three.

Description : trying to Access the module in Mutiple Client Test.

Parameters  : Handle_p , Valid handle reference to STPCCRD struct.
*****************************************************************************/

void task_three(STPCCRD_Handle_t *Handle_p)
{
    ST_ErrorCode_t          ErrCode;
    STPCCRD_Handle_t        Handle;

    Handle = *Handle_p;
    PCCRD_DebugMessage("Task three Doing ModReset");
    ErrCode = STPCCRD_CIReset(Handle);
    if ((ErrCode != ST_NO_ERROR) && (ErrCode != ST_ERROR_DEVICE_BUSY))
    {
        Multi_Client_Result_Failed++;
    }
    PCCRD_DebugError("Task three STPCCRD_ModReset()", ErrCode);
    Multiple_Task_Done++;

    for (;;)
    {
        task_delay(1000);
        if (Multiple_Task_Done == NUM_MULTIPLE_TASK)
        {
            break;
        }
    }
    STOS_SemaphoreSignal(MultipleTaskSem_p);
    return;
}

/********************************************************************************
Name        : PCCRDMultipleClients.

Description : Test the ability to prevent simultaneous access of DVB CI Module.

Parameters  : Params_p, Pointer to test params block.

Return Value: PCCRD_TestResult_t -- reflects the number of tests that pass/fail.
********************************************************************************/
static PCCRD_TestResult_t PCCRDMultipleClients(PCCRD_TestParams_t *Params_p)
{
    ST_ErrorCode_t          ErrCode;
    STPCCRD_Handle_t        HandleArray[MAX_PCCRD_HANDLE];
    U8                      NumTask;
#if defined(ST_OS20)
    tdesc_t                 Tdesc[NUM_MULTIPLE_TASK];
    task_t                  Task[NUM_MULTIPLE_TASK];
    U8                      TaskStack[NUM_MULTIPLE_TASK][SW_TASK_STACK_SIZE];
#elif defined(ST_OSLINUX)
   int                      rc;
#endif

#if !defined(ST_OSLINUX)
    char                    Taskname[10];
    S32                     TaskRet;
    task_t                  *Task_p[NUM_MULTIPLE_TASK];
#endif

    void (*func_table[])(STPCCRD_Handle_t *) = {
                                         task_one,
                                         task_two,
                                         task_three,
                                         NULL
                                        };
    void (*func)(void *);

    PCCRD_TestResult_t Result = TEST_RESULT_ZERO;

    PCCRD_DebugMessage("Accessing Modules simultaneously with different Clients");

#if defined(ST_OS20)
    for (NumTask = 0; NumTask < NUM_MULTIPLE_TASK; NumTask++)
    {
        Task_p[NumTask] = &Task[NumTask];
    }
#endif

    /* Opening the Handles */
    {
        STPCCRD_OpenParams_t    OpenParams;

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif
        /* For Module A */
        OpenParams.ModName = STPCCRD_MOD_A;
        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,&HandleArray[0]);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed( Result, "Failed to open all clients",
                              GetErrorText(ErrCode));
            return Result;
        }

#ifndef STV0701
        /* For Module B */
        OpenParams.ModName = STPCCRD_MOD_B;
        ErrCode = STPCCRD_Open(PCCRDDeviceName[0],&OpenParams,&HandleArray[1]);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Failed to open all clients",
                             GetErrorText(ErrCode));
            ErrCode = STPCCRD_Close(HandleArray[0]);
            return Result;
        }
#endif /* STV0701 */
    }

    /*Checking the Presence of Both Modules*/
    {
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        STPCCRD_EventType_t Event;
        STPCCRD_EventType_t ExpectedEvent;
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */

        ErrCode = STPCCRD_ModCheck(HandleArray[0]);
        if (ErrCode == STPCCRD_ERROR_NO_MODULE)
        {
            PCCRD_DebugMessage("Insert Card A");
            for (;;)
            {
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
                ExpectedEvent=STPCCRD_EVT_MOD_A_INSERTED;
                AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &ErrCode);
                if (Event == ExpectedEvent)
                {
                    break;
                }
#else
                /* Keep Polling for the Module */
                task_delay(1000);
                ErrCode = STPCCRD_ModCheck(HandleArray[0]);
                if (ErrCode == ST_NO_ERROR)
                {
                    break;
                }
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/
            }
        }/* if */
#ifndef STV0701
        ErrCode = STPCCRD_ModCheck(HandleArray[1]);
        if (ErrCode == STPCCRD_ERROR_NO_MODULE)
        {
            PCCRD_DebugMessage("Insert Card B");
            for (;;)
            {
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
                ExpectedEvent = STPCCRD_EVT_MOD_B_INSERTED;
                AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &ErrCode);
                if (Event == ExpectedEvent)
                {
                    break;
                }
#else
                task_delay(1000);
                ErrCode = STPCCRD_ModCheck(HandleArray[1]);
                if (ErrCode == ST_NO_ERROR)
                {
                    break;
                }
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/
            }
        }/* if */
#endif /* STV0701*/
    }

    /* Initialising the multiple clients */
    {
        PCCRD_DebugMessage("Intializing tasks");
#if defined(ST_OS20)
        MultipleTaskSem_p = &MultipleTaskSem;
        semaphore_init_fifo(&MultipleTaskSem, 0);
#else
        MultipleTaskSem_p = semaphore_create_fifo(0);
#endif

        Multiple_Task_Done = 0;
        Multi_Client_Result_Failed = 0;

        for (NumTask = 0; NumTask < NUM_MULTIPLE_TASK;NumTask++)
        {
            func = (void (*)(void *))func_table[NumTask];
#if !defined(ST_OSLINUX)
            sprintf((char *)Taskname, "task_%d", NumTask);
#endif
           if (NumTask == (NUM_MULTIPLE_TASK-1))
           {
#if defined(ST_OS20)
              TaskRet = task_init( func, (void *)(&HandleArray[0]), TaskStack[NumTask],SW_TASK_STACK_SIZE, Task_p[NumTask],
                          &Tdesc[NumTask], MAX_USER_PRIORITY, Taskname,(task_flags_t)0);
#elif defined(ST_OS21)
              Task_p[NumTask] = task_create( func, (void *)(&HandleArray[0]),20*SW_TASK_STACK_SIZE,MAX_USER_PRIORITY, Taskname,
                                  (task_flags_t)0);
#else

              rc = pthread_create(&task_multiclients[NumTask],tattr_multithread_p,(void *)func,(void *)(&HandleArray[0]));
#endif
           }
           else
           {
#if defined(ST_OS20)
              TaskRet = task_init( func, (void *)(&HandleArray[1]),TaskStack[NumTask], SW_TASK_STACK_SIZE, Task_p[NumTask],
                           &Tdesc[NumTask], MAX_USER_PRIORITY, Taskname, (task_flags_t)0);
              if (TaskRet !=0)
              {
                    STTBX_Print(("Task  %d not Intialised ", NumTask));
              }
#elif defined(ST_OS21)
             Task_p[NumTask] = task_create( func, (void *)(&HandleArray[1]), 20*SW_TASK_STACK_SIZE,
                              MAX_USER_PRIORITY, Taskname, (task_flags_t)0);
             if (Task_p[NumTask] == NULL)
             {
                 STTBX_Print(("Task  %d not Intialised ", NumTask));
             }
#else
             rc = pthread_create(&task_multiclients[NumTask],tattr_multithread_p,(void *)func,(void *)(&HandleArray[1]));
#endif
          }
        }
        /* Wait till all the clients finish their desired task */
        STOS_SemaphoreWait(MultipleTaskSem_p);

#if !defined(ST_OSLINUX)
        for (NumTask = 0; NumTask<NUM_MULTIPLE_TASK; NumTask++)
        {
            TaskRet = task_wait(&Task_p[NumTask], 1,TIMEOUT_INFINITY);
            if (TaskRet != -1)
            {
                task_delete(Task_p[NumTask]);
            }
        }
#else
      /* Delete client tasks */
     for (NumTask = 0; NumTask < NUM_MULTIPLE_TASK; NumTask++)
     {
         pthread_cancel(task_multiclients[NumTask]);
     }
#endif
    }
    /* Check for the result */
    if (Multi_Client_Result_Failed > 0)
    {
        PCCRD_TestFailed(Result, "Unable to Multiple Client Test", GetErrorText(MultipleClientErrCode));
    }
    else
    {
        PCCRD_TestPassed(Result);
    }
    /* Closing the Clients */
    {
        STOS_SemaphoreDelete(NULL,MultipleTaskSem_p);

        ErrCode = STPCCRD_Close(HandleArray[0]);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);

        ErrCode = STPCCRD_Close(HandleArray[1]);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);
    }

    return Result;
}
#endif

/******************************************************************************
Name         : test_overhead.

Description  : Just an empty function.

Parameters   : Params_p, Pointer to test params block.
********************************************************************************/

void test_overhead(PCCRD_TestParams_t *Params_p)
{
 /* Doing nothing */
}

/******************************************************************************
Name         : test_init

Description  : Calls the STPCCRD_Init API.

Parameters   : PCCRD Test Parameters Reference.
********************************************************************************/

void test_init(PCCRD_TestParams_t *Params_p)
{
    ST_ErrorCode_t ErrCode;

    ErrCode = STPCCRD_Init(PCCRDDeviceName[FIRST_INSTANCE],PCCRD_InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("Failed to init: ", ErrCode);
    }
}
/******************************************************************************
Name         : test_typical

Description  : Task calling the APIs of driver.

Parameters   : Params_p, Pointer to test params block.
********************************************************************************/

void test_typical(PCCRD_TestParams_t *Params_p)
{
    ST_ErrorCode_t          ErrCode;
    STPCCRD_OpenParams_t    OpenParams;
    STPCCRD_Handle_t        Handle;

    /* Setup the open params */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
    OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif /*STPCCRD_HOT_PLUGIN_ENABLED*/

    OpenParams.ModName = Params_p->Slot;

    /* Open the PCCRDcard */
    ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE], &OpenParams, &Handle);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Open()", ErrCode);
        return;
    }

    /* Check that a card is inserted before commencing ?*/
    ErrCode = STPCCRD_ModCheck(Handle);
    if (ErrCode == STPCCRD_ERROR_NO_MODULE)
    {
        PCCRD_DebugMessage("No Card Detected");
    }
    else if (ErrCode == ST_ERROR_INVALID_HANDLE)
    {
        PCCRD_DebugMessage("ST_ERROR_INVALID_HANDLE");
    }
    else
    {

#if defined(ST_7100) || defined(ST_7109)
        ErrCode = STPCCRD_ModPowerOff(Handle);
        PCCRD_DebugError("STPCCRD_ModPowerOff()", ErrCode);
#endif

#ifdef STPCCRD_VCC_5V
        ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_5000);
#else
        ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_3300);
#endif
        PCCRD_DebugError("STPCCRD_ModPowerOn()", ErrCode);


        /* If Module is present, reset it and check for CIS */
        ErrCode = STPCCRD_ModReset(Handle);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugError("STPCCRD_ModReset()", ErrCode);
        }

        ErrCode = STPCCRD_CheckCIS(Handle);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugError("STPCCRD_CheckCIS()", ErrCode);
        }

    }

    /* Close PC-Card Handle*/
    ErrCode = STPCCRD_Close(Handle);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);
    }
}
/******************************************************************************
Name         : test_term

Description  : Checks for proper behavior STPCCRD_Term() API

Parameters   : Params_p,  Pointer to test params block.
******************************************************************************/

void test_term(PCCRD_TestParams_t *Params_p)
{
    ST_ErrorCode_t ErrCode;
    STPCCRD_TermParams_t TermParams;

    TermParams.ForceTerminate = TRUE;

    ErrCode = STPCCRD_Term(PCCRDDeviceName[FIRST_INSTANCE], &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("Failed to terminate", ErrCode);
    }
}
#ifdef STPCCRD_DISABLE_TESTTOOL

/******************************************************************************
Name         : PCCRDDataUsage()

Description  : Checks for memory usage by InitParams in case MaxHandles vary.

Parameters   : Params_p,       Pointer to test params block.
*******************************************************************************/

void PCCRDDataUsage(PCCRD_TestParams_t *Params_p)
{
    U8                   *Marker_p;
    U32                  MemBeg_p;
    U32                  MemEnd_p;
    U32                  StoredNumber;
    ST_ErrorCode_t       ErrCode;
    STPCCRD_TermParams_t TermParams;

    StoredNumber = PCCRD_InitParams[0].MaxHandle;
    TermParams.ForceTerminate = FALSE;

    /* Initial marker */
#if defined(ST_7710)
    Marker_p = memory_allocate((ST_Partition_t *)ncache_partition, 1);
#else
    Marker_p = memory_allocate((ST_Partition_t *)system_partition, 1);
#endif

    if (Marker_p == NULL)
    {
        PCCRD_DebugMessage("Couldn't allocate memory for marker");
        return;
    }
    MemBeg_p = (U32)Marker_p;

    /* Note - bulk of function makes (possibly unsafe) assumption that, if tmp
     * can be allocated initially, later allocations won't fail. */
#if defined(ST_7710)
    memory_deallocate((ST_Partition_t *)ncache_partition, Marker_p);
#else
    memory_deallocate((ST_Partition_t *)system_partition, Marker_p);
#endif


    /* Try with MaxHandles = 1 */
    PCCRD_InitParams[0].MaxHandle = 1;

    ErrCode = STPCCRD_Init(PCCRDDeviceName[FIRST_INSTANCE],PCCRD_InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Init()", ErrCode);
        return;
    }

#if defined(ST_7710)
    Marker_p = memory_allocate((ST_Partition_t *)ncache_partition, 1);
#else
    Marker_p = memory_allocate((ST_Partition_t *)system_partition, 1);
#endif

    MemEnd_p = (U32)Marker_p;
#if defined(ST_7710)
    memory_deallocate((ST_Partition_t *)ncache_partition, Marker_p);
#else
    memory_deallocate((ST_Partition_t *)system_partition, Marker_p);
#endif

    ErrCode = STPCCRD_Term(PCCRDDeviceName[FIRST_INSTANCE], &TermParams);
    PCCRD_DebugPrintf(("Maximum handles: 1; used memory: %d\n", MemBeg_p - MemEnd_p));
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Term()", ErrCode);
        return;
    }

    /* Try with MaxHandles = 2 */
    PCCRD_InitParams[FIRST_INSTANCE].MaxHandle = MAX_PCCRD_HANDLE;
    ErrCode = STPCCRD_Init(PCCRDDeviceName[FIRST_INSTANCE], PCCRD_InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Init()", ErrCode);
        return;
    }

#if defined(ST_7710)
    Marker_p = memory_allocate((ST_Partition_t *)ncache_partition, 1);
#else
    Marker_p = memory_allocate((ST_Partition_t *)system_partition, 1);
#endif

    MemEnd_p = (U32)Marker_p;
#if defined(ST_7710)
    memory_deallocate((ST_Partition_t *)ncache_partition, Marker_p);
#else
    memory_deallocate((ST_Partition_t *)system_partition, Marker_p);
#endif

    PCCRD_DebugPrintf(("Maximum handles: 2; used memory: %i\n", MemBeg_p - MemEnd_p));
    ErrCode = STPCCRD_Term(PCCRDDeviceName[FIRST_INSTANCE], &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Term()",ErrCode);
        return;
    }

    /* Reset MaxHandles */
    PCCRD_InitParams[FIRST_INSTANCE].MaxHandle = StoredNumber;
}

#endif /*STPCCRD_DISABLE_TESTTOOL*/

/******************************************************************************
Name         : PCCRDStackUsage

Description  : Monitors Stack and Memory usage used by STPCCRD Driver

Parameters   : Params_p,       Pointer to test params block.

Return Value : PCCRD_TestResult_t -- reflects the number of tests that pass/fail.
********************************************************************************/
#if !defined(ST_OSLINUX)
static PCCRD_TestResult_t PCCRDStackUsage(PCCRD_TestParams_t *Params_p)
{
    ST_ErrorCode_t       ErrCode;
    STPCCRD_TermParams_t TermParams;
    PCCRD_TestResult_t   Result = TEST_RESULT_ZERO;

#ifndef ST_OS21
    task_t               Task;
    tdesc_t              Tdesc;
#endif
    task_t               *Task_p;
    task_status_t        Status;
#ifdef ST_OS21
    char                 Stack[20 * SW_TASK_STACK_SIZE];
#else
    char                 Stack[4 * SW_TASK_STACK_SIZE];
#endif
    U8                   Indx;

    void (*func_table[])(PCCRD_TestParams_t *) = {
                                     test_overhead,
                                     test_init,
                                     test_typical,
                                     test_term,
                                     NULL
                                   };
    void (*func)(void *);

    /* Terminate existing instance */
    TermParams.ForceTerminate = TRUE;
    ErrCode = STPCCRD_Term(PCCRDDeviceName[FIRST_INSTANCE], &TermParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Term()", ErrCode);
        return Result;
    }
#ifndef ST_OS21
    Task_p = &Task;
#endif

    for (Indx = 0; func_table[Indx] != NULL; Indx++)
    {
        func = (void (*)(void *))func_table[Indx];
        /* Creating A Task */

#ifndef ST_OS21
        task_init(func, (void *)Params_p, Stack, 4 * SW_TASK_STACK_SIZE, Task_p,
                  &Tdesc, MAX_USER_PRIORITY, "stack_test", (task_flags_t)0);
#else



        Task_p = task_create(func, (void *)Params_p, 20 * SW_TASK_STACK_SIZE,
                 MAX_USER_PRIORITY, "stack_test", (task_flags_t)0);
        if (Task_p == NULL)
        {
            PCCRD_DebugPrintf(("Task creation FAILED()"));
        }
#endif
        task_wait(&Task_p, 1, TIMEOUT_INFINITY);                     /* Wait till task terminates */
        task_status(Task_p, &Status, task_status_flags_stack_used);  /* Get stack used */
        task_delete(Task_p);                                         /* Free all the memory structs used by the task */


        PCCRD_DebugPrintf(("Function %i, stack used: %i\n",
                          Indx, Status.task_stack_used));

    }
#ifdef STPCCRD_DISABLE_TESTTOOL
    /* Call Memory Usage Check function */
    PCCRDDataUsage(Params_p);
#endif
    PCCRD_DebugMessage("Result: **** PASS ****\n");

    /* Initialise terminated instance */
    ErrCode = STPCCRD_Init(PCCRDDeviceName[FIRST_INSTANCE],PCCRD_InitParams);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugError("STPCCRD_Init()", ErrCode);
    }
   return Result;
}
#endif/*ST_OSLINUX*/
/*****************************************************************************
Name        : PCCRDAPI()

Description : Tests the robustness and validity of the STPCCRD APIs.

Parameters  : Params_p,       pointer to test params block.

Return Value: PCCRD_TestResult_t -- reflects the number of tests that pass/fail.
*******************************************************************************/

static PCCRD_TestResult_t PCCRDAPI(PCCRD_TestParams_t *Params_p)
{
    PCCRD_TestResult_t Result = TEST_RESULT_ZERO;
    STPCCRD_Handle_t   Handle = 0;
    ST_ErrorCode_t     ErrCode;
    U32                NumHandle;
    U32                Index = 0;

    STTBX_Print(("\n"));
    PCCRD_DebugPrintf(("%d.%d: Multiple init calls\n", Params_p->Ref, Index++));
    PCCRD_DebugMessage("Purpose: Ensure the driver rejects multiple init calls");
    {
        ErrCode =  STPCCRD_Init( PCCRDDeviceName[FIRST_INSTANCE],
                                 &PCCRD_InitParams[0]);
        PCCRD_DebugError("STPCCRD_Init()", ErrCode);
        if (ErrCode != ST_ERROR_ALREADY_INITIALIZED)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_ALREADY_INITIALIZED",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }
    }
    STTBX_Print(("\n"));
    PCCRD_DebugPrintf(("%d.%d: Invalid handle\n", Params_p->Ref, Index++));
    PCCRD_DebugMessage("Purpose: Checks API calls with invalid handle");
    {
        U8 DummyBuffer;
        U16 DummyPointer;
        ErrCode = STPCCRD_Close(Handle);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_ModCheck(Handle);
        PCCRD_DebugError("STPCCRD_ModCheck()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }
        ErrCode = STPCCRD_Read(Handle,&DummyBuffer,&DummyPointer);

        PCCRD_DebugError("STPCCRD_Read()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_Write(Handle,&DummyBuffer,0,&DummyPointer);
        PCCRD_DebugError("STPCCRD_Write()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_CheckCIS(Handle);
        PCCRD_DebugError("STPCCRD_CheckCIS()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_WriteCOR(Handle);
        PCCRD_DebugError("STPCCRD_WriteCOR()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_ModReset(Handle);
        PCCRD_DebugError("STPCCRD_ModReset()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_CIReset(Handle);
        PCCRD_DebugError("STPCCRD_CIReset()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_ControlTS(Handle,STPCCRD_ENABLE_TS_PROCESSING);
        PCCRD_DebugError("STPCCRD_ControlTS()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

#if defined(ST_7100) || defined(ST_7109)
        ErrCode = STPCCRD_ModPowerOff(Handle);
        PCCRD_DebugError("STPCCRD_ModPowerOff()", ErrCode);
#endif

#ifdef STPCCRD_VCC_5V
        ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_5000);
#else
        ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_3300);
#endif

        PCCRD_DebugError("STPCCRD_ModPowerOn()", ErrCode);

        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_ModPowerOff(Handle);
        PCCRD_DebugError("STPCCRD_ModPowerOff()", ErrCode);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_TestFailed( Result, "Expected ST_ERROR_INVALID_HANDLE",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

    }

    STTBX_Print(("\n"));
    PCCRD_DebugPrintf(("%d.%d: Open tests\n", Params_p->Ref, Index++));
    PCCRD_DebugMessage("Purpose: Attempts to exceed max. clients limit");
    {
        STPCCRD_Handle_t     HandleArray[2];
        STPCCRD_OpenParams_t OpenParams;

        PCCRD_DebugMessage("Opening maximum number of clients...");
        for (NumHandle = 0; NumHandle < MAX_PCCRD_HANDLE; NumHandle++)
        {

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
            OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif
            OpenParams.ModName = (STPCCRD_ModName_t)NumHandle;
            ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,&HandleArray[NumHandle]);
            if (ErrCode != ST_NO_ERROR)
            {
                break;
            }
        }

        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Failed to open all clients",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_DebugMessage("Attempting to exceed maximum number of clients");
            ErrCode = STPCCRD_Open( PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,
                                    &Handle);
            PCCRD_DebugError("STPCCRD_Open()", ErrCode);

            if (ErrCode != ST_ERROR_NO_FREE_HANDLES)
            {
                PCCRD_TestFailed(Result,"Expected ST_ERROR_NO_FREE_HANDLES",
                                 GetErrorText(ErrCode));
            }
            else
            {
                PCCRD_TestPassed(Result);
            }
        }

        /* Close down all handles again */
        for (NumHandle = 0; NumHandle < MAX_PCCRD_HANDLE; NumHandle++)
        {
            STPCCRD_Close(HandleArray[NumHandle]);
        }
    }
    STTBX_Print(("\n"));
    PCCRD_DebugPrintf(("%d.%d: Bad Parameters \n", Params_p->Ref, Index++));
    PCCRD_DebugMessage("Purpose: Ensure Wrong Parameters are always rejected");
    {
        STPCCRD_OpenParams_t OpenParams;
        STPCCRD_Handle_t     HandleArray[MAX_PCCRD_HANDLE];
        STPCCRD_TermParams_t TermParams;
        U8                   Status,Val;
        U16                  NumRead;

        PCCRD_DebugMessage("Opening a connection with wrong device name");
        ErrCode = STPCCRD_Open(PCCRDDeviceName[1],&OpenParams,&HandleArray[0]);
        PCCRD_DebugError("STPCCRD_Open()", ErrCode);
        if (ErrCode != ST_ERROR_UNKNOWN_DEVICE)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_UNKNOWN_DEVICE",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Opening a connection with wrong Module name");

        OpenParams.ModName= (STPCCRD_ModName_t)3;
        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,&HandleArray[0]);
        PCCRD_DebugError("STPCCRD_Open()", ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Terminating the Driver with wrong device name");
        TermParams.ForceTerminate = FALSE;
        ErrCode = STPCCRD_Term(PCCRDDeviceName[1],&TermParams);
        PCCRD_DebugError("STPCCRD_Term()", ErrCode);
        if (ErrCode != ST_ERROR_UNKNOWN_DEVICE)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_UNKNOWN_DEVICE",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        /* First opening a client */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif
        OpenParams.ModName= STPCCRD_MOD_A;
        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,&HandleArray[0]);
        if (ErrCode != ST_NO_ERROR)
        {
             PCCRD_TestFailed(Result, "Expected ST_NO_ERROR",
                             GetErrorText(ErrCode));
             return Result;
        }

        PCCRD_DebugMessage("Terminating the Driver with one Open Connection");
        TermParams.ForceTerminate = FALSE;
        ErrCode = STPCCRD_Term(PCCRDDeviceName[FIRST_INSTANCE],&TermParams);
        PCCRD_DebugError("STPCCRD_Term()", ErrCode);
        if (ErrCode != ST_ERROR_OPEN_HANDLE)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_OPEN_HANDLE",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Asking for the wrong Status bit");
        ErrCode = STPCCRD_GetStatus(HandleArray[0], (STPCCRD_Status_t)4, &Status);
        PCCRD_DebugError("STPCCRD_GetStatus()",ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Setting the wrong Command bit");
        Val = 0;
        ErrCode = STPCCRD_SetStatus(HandleArray[0],(STPCCRD_Command_t)4, Val);
        PCCRD_DebugError("STPCCRD_SetStatus()",ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Specifying wrong set/reset argument");
        Val = 2;
        ErrCode = STPCCRD_SetStatus(HandleArray[0],(STPCCRD_Command_t)1, Val);
        PCCRD_DebugError("STPCCRD_SetStatus()",ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }
        PCCRD_DebugMessage("Specifying wrong TS processing Parameter");
        Val = 2;
        ErrCode = STPCCRD_ControlTS(HandleArray[0],(STPCCRD_TSBypassMode_t)Val);
        PCCRD_DebugError("STPCCRD_ControlTS()",ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER &&ErrCode != STPCCRD_ERROR_NO_MODULE )
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Sending NULL buffer while reading ");
        ErrCode = STPCCRD_Read(HandleArray[0],NULL,&NumRead);
        PCCRD_DebugError("STPCCRD_Read()",ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        PCCRD_DebugMessage("Sending NULL buffer while Writing ");
        ErrCode = STPCCRD_Write(HandleArray[0],NULL,1,&NumRead);
        PCCRD_DebugError("STPCCRD_Write()",ErrCode);
        if (ErrCode != ST_ERROR_BAD_PARAMETER)
        {
            PCCRD_TestFailed(Result, "Expected ST_ERROR_BAD_PARAMETER",
                             GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }
        ErrCode= STPCCRD_Close(HandleArray[0]);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);

        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed( Result, "Expected ST_NO_ERROR",
                              GetErrorText(ErrCode));
        }
        else
        {
            PCCRD_TestPassed(Result);
        }
    }

    STTBX_Print(("\n"));
    PCCRD_DebugPrintf(("%d.%d: Checking if Memory Can be used after Term \n", Params_p->Ref, Index++));
    PCCRD_DebugMessage("Purpose: Using handle after closing/terminating");
    {
        STPCCRD_Handle_t      Handle;
        STPCCRD_OpenParams_t  OpenParams;
        STPCCRD_TermParams_t  TermParams;

        PCCRD_DebugMessage("Opening a client");
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif
        OpenParams.ModName = Params_p->Slot;

        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE],&OpenParams,&Handle);

        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Failed to open the client",
                             GetErrorText(ErrCode));
        }

        TermParams.ForceTerminate = TRUE;
        ErrCode = STPCCRD_Term(PCCRDDeviceName[FIRST_INSTANCE],&TermParams);
        PCCRD_DebugError("STPCCRD_Term()", ErrCode);

        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Failed to terminate the client",
                             GetErrorText(ErrCode));

        }
        else
        {
            PCCRD_TestPassed(Result);
        }

        ErrCode = STPCCRD_CheckCIS(Handle);
        if (ErrCode != ST_ERROR_INVALID_HANDLE)
        {
             PCCRD_TestFailed( Result, "Expected result ST_ERROR_INVALID_HANDLE ",
                               GetErrorText(ErrCode));
        }
        else
        {
            ErrCode = STPCCRD_Init(PCCRDDeviceName[FIRST_INSTANCE],&PCCRD_InitParams[FIRST_INSTANCE]);
            PCCRD_DebugError("STPCCRD_Init()", ErrCode);
            PCCRD_TestPassed(Result);
        }
    }

    return Result;

} /* PCCRDAPI() */

/****************************************************************************
Name         : NegotiateBuffSize()

Description  : Local Function doing the Buffer size negotiation
               Called after Configuring the Module in Memory Mode.

Parameters   : STPCCRD_Handle_t Handle identifies the device,
               *BuffSize_p      Pointer to the buffer size being negotiated.

Return Value : ST_ErrorCode_t specified as
               ST_ERROR_BAD_PARAMETER      Invalid parameter(s)
               ST_ERROR_DEVICE_BUSY        Device is currently busy
               STPCCRD_ERROR_BADWRITE      Error during write
               STPCCRD_ERROR_BADREAD       Error during read
               ST_NO_ERROR                 No Error Occurred
               Value of Buffer size negotiated in *BuffSize_p ptr.
****************************************************************************/

ST_ErrorCode_t NegotiateBuffSize( STPCCRD_Handle_t Handle, U16 *BuffSize_p)
{
    ST_ErrorCode_t    ErrCode;
    U8                Status;
    U8                BufferSize[2];
    U16               RetryCnt;
    U16               NumberRead=0;
    U16               NumberToBeWritten;
    U16               NumberWritten=0;

    PCCRD_DebugMessage("Doing the Buffer Size Negotiation");

    /* Ask the module to provide maximum buf size by setting the SR bit */
    *BuffSize_p = 0;

    ErrCode = STPCCRD_SetStatus(Handle,STPCCRD_CI_SIZE_READ,SET_COMMAND_BIT);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugMessage("STPCCRD_SetStatus Failed");
        return (ErrCode);
    }

    PCCRD_DebugMessage("Waiting for the DA bit to set");
    ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_DATA_AVAILABLE,&Status);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugMessage("STPCCRD_GetStatus Failed");
        return (ErrCode);
    }

    /* Try it for sometime */
    for (RetryCnt = 0; RetryCnt < MAX_RETRY_CNT; RetryCnt++)
    {
        if (!Status) /* Checking if DA Bit is set */
        {
            /* Again wait for DA bit to get set! */
            task_delay( ST_GetClocksPerSecond()/100);
            ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_DATA_AVAILABLE,&Status);
            if (ErrCode != ST_NO_ERROR)
            {
                PCCRD_DebugMessage("STPCCRD_GetStatus Failed");
                return (ErrCode);
            }
        }
        else
        {
            break;
        }
    }/*End For*/

    if (RetryCnt == MAX_RETRY_CNT)
    {
        PCCRD_DebugMessage("STPCCRD_GetStatus Failed Timeout");
        return (ST_ERROR_TIMEOUT);
    }

    PCCRD_DebugMessage("Reading Size of Buffer");
    /* Read the Buffer size being supported by the Module */
    ErrCode = STPCCRD_Read(Handle,BufferSize,&NumberRead);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugMessage("STPCCRD_Read Failed");
        /* *BuffSize_p = (BufferSize[0]<<SHIFT_EIGHT |BufferSize[1]); */
        *BuffSize_p = (BufferSize[0]*0x100 + BufferSize[1]);
        return (ErrCode);
    }
    else
    {
        /* Here setting the SW bit to get FR bit set */
        ErrCode = STPCCRD_SetStatus(Handle,STPCCRD_CI_SIZE_WRITE,SET_COMMAND_BIT);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage("STPCCRD_SetStatus Failed");
            return (ErrCode);
        }

        ErrCode = STPCCRD_GetStatus(Handle, STPCCRD_CI_FREE, &Status);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage("STPCCRD_GetStatus Failed");
            return (ErrCode);
        }

        /* Try it for sometime */
        for (RetryCnt= 0; RetryCnt <(MAX_RETRY_CNT); RetryCnt++)
        {
            if (!Status)
            {
                task_delay( ST_GetClocksPerSecond()/100);
                ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_FREE,&Status);
                if (ErrCode != ST_NO_ERROR)
                {
                    PCCRD_DebugMessage("STPCCRD_GetStatus Failed");
                    return (ErrCode);
                }
            }
            else
            {
                break;
            }
        }/* End for */

        if (RetryCnt == MAX_RETRY_CNT)
        {
            PCCRD_DebugMessage("STPCCRD_GetStatus Failed Timeout");
            return (ST_ERROR_TIMEOUT);
        }

        /* Now module is free to accept data from host! */
        /* Write back the BufferSize size */
        NumberToBeWritten = 2;
        ErrCode = STPCCRD_Write( Handle,BufferSize,NumberToBeWritten,
                                &NumberWritten);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_Write Failed \n"));
            *BuffSize_p=(BufferSize[0]<<SHIFT_EIGHT |BufferSize[1]);
            return (ErrCode);
        }
    }

    /* Here reset the SW bit of the command register after Data Transaction is over */
    ErrCode = STPCCRD_SetStatus(Handle, STPCCRD_CI_SIZE_WRITE, RESET_COMMAND_BIT);
    if (ErrCode != ST_NO_ERROR)
    {
        PCCRD_DebugMessage("STPCCRD_SetStatus Failed");
        return (ErrCode);
    }

    /* Make out the Buffer size read */
    *BuffSize_p=(BufferSize[0]<<SHIFT_EIGHT |BufferSize[1]);

    return (ErrCode);
}

/********************************************************************************
Name        : PCCRDConfigurationTest()

Description : Test the basic functionality of driver
              Do the module configuration in Memory mode and then do the buffer
              size negotiation with the module.

Parameters  : Params_p,       Pointer to test params block.

Return Value: PCCRD_TestResult_t -- reflects the number of tests that pass/fail.
********************************************************************************/

static PCCRD_TestResult_t PCCRDConfigurationTest(PCCRD_TestParams_t *Params_p)
{
    PCCRD_TestResult_t Result = TEST_RESULT_ZERO;
    STPCCRD_Handle_t   Handle;
    ST_ErrorCode_t     ErrCode ;
    U16                BuffSize = 0;
    U16                ErrCnt =0;

    PCCRD_DebugMessage("Configuring the Module for DVB CI Communication");
    /* Opening the Connection */
    {
        STPCCRD_OpenParams_t OpenParams;

        /* Setup the open params */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif
        OpenParams.ModName = Params_p->Slot;
        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE], &OpenParams, &Handle);
        PCCRD_DebugError("STPCCRD_Open()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Unable to open", GetErrorText(ErrCode));
            goto ir_end;
        }
    }

    /* Check that a card is inserted before commencing */
    {
        ErrCode = STPCCRD_ModCheck(Handle);
        if (ErrCode == STPCCRD_ERROR_NO_MODULE)
        {
            if (Params_p->Slot == STPCCRD_MOD_A)
            {
                PCCRD_DebugMessage("Please insert a card in slot A to proceed..!");
            }

            if (Params_p->Slot == STPCCRD_MOD_B)
            {
                PCCRD_DebugMessage("Please insert a card in slot B to proceed..!");
            }

            for (;;) /*Check untill ModA/ModB inserted !*/
            {
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
                /* If Hot Plugin is enabled then wait for the relevant event */
                STPCCRD_EventType_t Event;
                if (Params_p->Slot == STPCCRD_MOD_A)
                {
                    AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &ErrCode);
                    if (Event == STPCCRD_EVT_MOD_A_INSERTED)
                    {
                        break;
                    }
                }
                if (Params_p->Slot == STPCCRD_MOD_B)
                {
                    AwaitNotifyEvent(AWAIT_ANY_EVENT, &Event, &ErrCode);
                    if (Event == STPCCRD_EVT_MOD_B_INSERTED)
                    {
                        break;
                    }
                }
#else
                /* If Hot Plugin is not enabled keep polling */
                task_delay( ST_GetClocksPerSecond()/100);
                ErrCode = STPCCRD_ModCheck(Handle);
                if (ErrCode == ST_NO_ERROR)
                {
                    break;
                }

#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
            }/* End For */
        }/* End IF */
        else if (ErrCode == ST_ERROR_INVALID_HANDLE)
        {
            PCCRD_DebugMessage("ST_ERROR_INVALID_HANDLE");
        }
    }/* Card checking done */

    /* Here starts the configuration of DVBCI Module */
    {
#if defined(ST_7100) || defined(ST_7109)
        ErrCode = STPCCRD_ModPowerOff(Handle);
        PCCRD_DebugError("STPCCRD_ModPowerOff()", ErrCode);
#endif

        /* You should Power On the Module */
#ifdef STPCCRD_VCC_5V
        ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_5000);
#else
        ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_3300);
#endif
        PCCRD_DebugError("STPCCRD_ModPowerOn()", ErrCode);


        ErrCode = STPCCRD_ModReset(Handle);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugError("STPCCRD_ModReset()", ErrCode);
            ErrCnt= ErrCnt+1;
            goto ir_end_new;
        }
        else
        {
            PCCRD_DebugMessage("STPCCRD_ModReset() performed.");
        }


        /*
        * This delay is necessary just after ModReset,
        * value of delay depends from module to module. Here we have taken
        * the worst case of CONX Module For Iredeto this value is 1000,
        * so user has check for this value
        */
        task_delay( ST_GetClocksPerSecond()/100);
        ErrCode = STPCCRD_CheckCIS(Handle);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugError("STPCCRD_CheckCIS()", ErrCode);
            ErrCnt = ErrCnt + 1;
            goto ir_end_new;
        }
        else
        {
           PCCRD_DebugError("STPCCRD_CheckCIS()", ErrCode);
        }

        ErrCode = STPCCRD_WriteCOR(Handle);
        if (ErrCode != ST_NO_ERROR)
        {
           PCCRD_DebugError("STPCCRD_WriteCOR()", ErrCode);
           ErrCnt = ErrCnt + 1;
           goto ir_end_new;
        }
        else
        {
           PCCRD_DebugError("STPCCRD_WriteCOR()", ErrCode);
        }

         task_delay( ST_GetClocksPerSecond()/100);

        /* Here also delay is required after switching from Memory mode to IO mode
         * since that is a small delay ,that is being taken care in the STPCCRD_CIReset API
         */
        ErrCode = STPCCRD_CIReset(Handle);
        if (ErrCode != ST_NO_ERROR)
        {
           PCCRD_DebugError("STPCCRD_CIReset()", ErrCode);
           ErrCnt = ErrCnt + 1;
           goto ir_end_new;
        }
        else
        {
           PCCRD_DebugError("STPCCRD_CIReset()", ErrCode);
        }

        /* Doing the Buffer size Negotiation */
        ErrCode = NegotiateBuffSize(Handle,&BuffSize);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage("Buffer Size Negotiation ::Failed ");
            ErrCnt = ErrCnt + 1;
            PCCRD_DebugPrintf(("Buffer Fail Size %d: \n", BuffSize));
            goto ir_end_new;
        }
        else
        {
            PCCRD_DebugMessage("Buffer Size Negotiation ::Passed ");
            PCCRD_DebugPrintf(("Buffer Size Is %d: bytes\n", BuffSize));
        }
        task_delay( ST_GetClocksPerSecond()/100);
    }
ir_end_new:
    if (ErrCnt > 0)
    {
        PCCRD_TestFailed(Result, "Unable to Perform Configuration Test", GetErrorText(ErrCode));
    }
    else
    {
        PCCRD_TestPassed(Result);
    }
ir_end:
    /* Closing the Connections */
    {
        ErrCode = STPCCRD_Close(Handle);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);
    }
    return Result;

} /* PCCRDConfigurationTest() */

/********************************************************************************
Name        : PCCRD_DVBCI_Test()

Description : This test starts the communication with the DVB CI Module
              by sending and receving the PDUs.
              This test is only valid when Module is configured and Buffer Size
              Confgiuration is done on the required module.

Parameters  : Params_p,       pointer to test params block.

Return Value: PCCRD_TestResult_t -- reflects the number of tests that pass/fail.
********************************************************************************/

static PCCRD_TestResult_t PCCRD_DVBCI_Test(PCCRD_TestParams_t *Params_p)
{
    PCCRD_TestResult_t  Result = TEST_RESULT_ZERO;
    STPCCRD_Handle_t    Handle;
    ST_ErrorCode_t      ErrCode;
    U16                 ErrCnt = 0;

    PCCRD_DebugMessage("Starting DVB CI Communication with the Module ");
    PCCRD_DebugMessage("Test is valid only when Configuration Test is Passed");
    /* Opening the clients */
    {
        STPCCRD_OpenParams_t OpenParams;
        /* Setup the open params */
#ifdef STPCCRD_HOT_PLUGIN_ENABLED
        OpenParams.NotifyFunction = CallbackNotifyFunction;
#endif
        OpenParams.ModName = Params_p->Slot;
        /* Open the PCCRDcard */
        ErrCode = STPCCRD_Open(PCCRDDeviceName[FIRST_INSTANCE] ,
                             &OpenParams,&Handle);
        PCCRD_DebugError("STPCCRD_Open()", ErrCode);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_TestFailed(Result, "Unable to open", GetErrorText(ErrCode));
            goto ir_end;
        }
    }

    /* Check that a card is inserted before commencing */
    ErrCode = STPCCRD_ModCheck(Handle);
    if (ErrCode == STPCCRD_ERROR_NO_MODULE)
    {
        if (Params_p->Slot == STPCCRD_MOD_A)
        {
            PCCRD_DebugMessage("DVB CI Module is Not in slot A ");
        }
        if (Params_p->Slot == STPCCRD_MOD_B)
        {
            PCCRD_DebugMessage("DVB CI Module is Not in slot B ");
        }
        ErrCnt++;
        goto ir_end_new;
    }
    else if (ErrCode == ST_ERROR_INVALID_HANDLE)
    {
        PCCRD_DebugMessage("ST_ERROR_INVALID_HANDLE");
    }

#ifdef STPCCRD_VCC_5V
    ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_5000);
#else
    ErrCode = STPCCRD_ModPowerOn(Handle,STPCCRD_POWER_3300);
#endif

    PCCRD_DebugError("STPCCRD_ModPowerOn()", ErrCode);


    /* Making some delay */
    PCCRD_DebugMessage("Now Trying to Communicate with DVBCI Module ");
    {
        U16       NumberRead = 0;
        U16       NumberWritten = 0;
        U16       RetryCnt/*,DelayCnt*/;
        U8        Status;
        U8        Indx;
        U8        Buffer[BUFFER_LEN];

        /* for ( DelayCnt =0; DelayCnt < MAX_RETRY_CNT; DelayCnt++); */
        task_delay( ST_GetClocksPerSecond()/10);
        PCCRD_DebugMessage(("Sending the Create TC PDU to the Module"));

        ErrCode = STPCCRD_Write(Handle,Create_T_C,CREATE_TC_LEN,&NumberWritten);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_Write Failed "));
            ErrCnt++;
            goto ir_end_new;
        }

        /* Making some delay */
        /*for ( DelayCnt =0; DelayCnt < MAX_RETRY_CNT; DelayCnt++);*/
        task_delay( ST_GetClocksPerSecond()/10);

        /* Check and Wait for DA bit to set */
        ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_DATA_AVAILABLE,&Status);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_GetStatus Failed"));
            ErrCnt++;
            goto ir_end_new;

        }
        for (RetryCnt = 0; RetryCnt <MAX_RETRY_CNT; RetryCnt++)
        {
            if (!Status)
            {
                /*for ( DelayCnt =0; DelayCnt < MAX_RETRY_CNT; DelayCnt++);*/
                task_delay( ST_GetClocksPerSecond()/10);
                ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_DATA_AVAILABLE,&Status);
                if (ErrCode != ST_NO_ERROR)
                {
                    PCCRD_DebugMessage(("STPCCRD_GetStatus Failed "));
                    ErrCnt++;
                    goto ir_end_new;
                }
            }
            else
            {
                break;
            }
        }

        if (RetryCnt == MAX_RETRY_CNT)
        {
            PCCRD_DebugMessage(("STPCCRD_GetStatus Failed Timeout"));
            ErrCnt++;
            goto ir_end_new;
        }

        ErrCode = STPCCRD_Read(Handle,Buffer,&NumberRead);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_Read Failed"));
            ErrCnt++;
            goto ir_end_new;
        }

        /* Checking if it is the expected response */
        if (NumberRead !=CREATE_TC_REPLY_LEN)
        {
            ErrCnt++;
            ErrCode= STPCCRD_ERROR_BADREAD;
            goto ir_end_new;
        }

        PCCRD_DebugMessage(("Checking the Create TC Response"));
        for (Indx = 0; Indx<NumberRead; Indx++)
        {
            if (Buffer[Indx]!=Create_T_C_Reply[Indx])
            {
                ErrCnt++;
                ErrCode = STPCCRD_ERROR_BADREAD;
                goto ir_end_new;
            }
        }

        PCCRD_DebugMessage(("Sending the T_Data_Last PDU To Module"));
        ErrCode = STPCCRD_Write( Handle,T_DATA_LAST,T_DATA_LAST_LEN,
                                &NumberWritten);

        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_Write Failed "));
            ErrCnt++;
            goto ir_end_new;
        }

        ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_DATA_AVAILABLE,&Status);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_GetStatus Failed"));
            ErrCnt++;
            goto ir_end_new;
        }

        for (RetryCnt = 0; RetryCnt <(MAX_RETRY_CNT); RetryCnt++)
        {
            if (!Status)
            {
            	/*for (DelayCnt=0; DelayCnt < MAX_RETRY_CNT; DelayCnt++);*/
                task_delay( ST_GetClocksPerSecond()/10);
                ErrCode = STPCCRD_GetStatus(Handle,STPCCRD_CI_DATA_AVAILABLE,&Status);
                if (ErrCode != ST_NO_ERROR)
                {
                    PCCRD_DebugMessage(("STPCCRD_GetStatus Failed "));
                    ErrCnt++;
                    goto ir_end_new;
                }

            }
            else
            {
                break;
            }
        }
        if (RetryCnt == MAX_RETRY_CNT)
        {
            PCCRD_DebugMessage(("STPCCRD_GetStatus Failed Timeout"));
            ErrCnt++;
            goto ir_end_new;
        }

        PCCRD_DebugMessage(("Checking the T_SB Response"));
        ErrCode = STPCCRD_Read(Handle,Buffer,&NumberRead);
        if (ErrCode != ST_NO_ERROR)
        {
            PCCRD_DebugMessage(("STPCCRD_Read Failed"));
            ErrCnt++;
            goto ir_end_new;
        }

        /* Checking if it is the expected response */
        if (NumberRead != T_SB_LEN)
        {
            ErrCnt++;
            ErrCode = STPCCRD_ERROR_BADREAD;
            goto ir_end_new;
        }
        for (Indx = 0; Indx<(NumberRead-1); Indx++)
        {
            if (Buffer[Indx]!=T_SB[Indx])
            {
                ErrCnt++;
                ErrCode= STPCCRD_ERROR_BADREAD;
                goto ir_end_new;
            }
        }
        if (!((Buffer[T_SB_LEN-1]==0)||(Buffer[T_SB_LEN-1]==DA_BIT_SET)))
        {
            ErrCnt++;
            ErrCode = STPCCRD_ERROR_BADREAD;
            goto ir_end_new;
        }

     }
ir_end_new:
    if (ErrCnt > 0)
    {
        PCCRD_TestFailed(Result, "Unable to Perform DVBCI Communication Test", GetErrorText(ErrCode));
    }
    else
    {
        PCCRD_TestPassed(Result);
    }
ir_end:
    /* Closing the Client */
    {
        ErrCode = STPCCRD_Close(Handle);
        PCCRD_DebugError("STPCCRD_Close()", ErrCode);
    }

    return Result;

} /* PCCRD_DVBCI_Test() */

#ifndef STPCCRD_DISABLE_TESTTOOL
/*****************************************************************************
Name        : tt_pccrdapi()

Description : Callback function registered with respect to API_TEST Command.
              Calls PCCRDAPI() test function for ModuleA/B depending upon
              user choice.

Parameters  : Command line arguments reference and variable reference for
              storage of result after call(if any).

Return Value: FALSE - On Success
              TRUE  - On Error.
*****************************************************************************/

static BOOL tt_pccrdapi( parse_t *pars_p, char *result_sym_p )
{
    PCCRD_TestEntry_t *Test_p;
    PCCRD_TestParams_t TestParams;
    char Text[4];
    PCCRD_TestResult_t Result;

    STTST_GetItem( pars_p, "", Text, 4);
    if ( strcmp( Text, "A" ) == 0 || strcmp( Text, "a" ) == 0)
    {
        Test_p = &TestTable[0];
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */
        TestParams.Slot = Test_p->Slot;
        TestParams.Ref  = 1;

        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
    else if ( strcmp( Text, "B" ) == 0 || strcmp( Text, "b" ) == 0)
    {
        Test_p = &TestTable[1];
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */
        TestParams.Slot = Test_p->Slot;
        TestParams.Ref  = 1;
        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
    else
    {
        PCCRD_DebugMessage ("Error :Wrong  Argument has been passed !");
        PCCRD_DebugMessage ("For Module A, type A or a !");
        PCCRD_DebugMessage ("For Module B, type B or b !");
        return (TRUE);
    }

    return (FALSE);

} /* end of tt_pccrdapi() */

/*****************************************************************************
Name        : tt_pccrdstackusage()

Description : Callback function registered with respect to  STACK_USAGE Command.
              Calls PCCRDStackUsage() test function for ModuleA/B depending upon
              user choice.

Parameters  : Command line arguments reference and variable reference for
              storage of result after call(if any).

Return Value: FALSE - On Success
              TRUE  - On Error.
*****************************************************************************/

static BOOL tt_pccrdstackusage( parse_t *pars_p, char *result_sym_p )
{
    PCCRD_TestEntry_t *Test_p;
    PCCRD_TestParams_t TestParams;
    char Text[4];
    PCCRD_TestResult_t Result;

    STTST_GetItem( pars_p, "", Text, 4);
    if ( strcmp( Text, "A" ) == 0 || strcmp( Text, "a" ) == 0)
    {
        Test_p = &TestTable[2];
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */
        TestParams.Slot = Test_p->Slot;
        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
    else if ( strcmp( Text, "B" ) == 0 || strcmp( Text, "b" ) == 0)
    {
        Test_p = &TestTable[3];
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */
        TestParams.Slot = Test_p->Slot;

        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
   else
   {
        PCCRD_DebugMessage ("Error :Wrong  Argument has been passed !");
        PCCRD_DebugMessage ("For Module A, type A or a !");
#ifndef STV0701
        PCCRD_DebugMessage ("For Module B, type B or b !");
#endif /* STV0701 */
        return (TRUE);
   }

   return (FALSE);

} /* end of tt_pccrdstackusage() */

/*****************************************************************************
Name        : tt_pccrdconfig()

Description : Callback function registered with respect to  CONFIG_TEST Command.
              Calls PCCRDConfigurationTest() test function for ModuleA/B depending
              upon  user choice.

Parameters  : Command line arguments reference and variable reference for
              storage of result after call(if any).

Return Value: FALSE - On Success
              TRUE  - On Error.
*****************************************************************************/

static BOOL tt_pccrdconfig( parse_t *pars_p, char *result_sym_p )
{
    PCCRD_TestEntry_t *Test_p;
    PCCRD_TestParams_t TestParams;
    char Text[4];
    PCCRD_TestResult_t Result;

    STTST_GetItem( pars_p, "", Text, 4);
    if ( strcmp( Text, "A" ) == 0 || strcmp( Text, "a" ) == 0)
    {
#if defined(ST_OSLINUX)
        Test_p = &TestTable[2];
#else
        Test_p = &TestTable[4];
#endif
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */

        TestParams.Slot = Test_p->Slot;

        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
    else if ( strcmp( Text, "B" ) == 0 || strcmp( Text, "b" ) == 0)
    {
#if defined(ST_OSLINUX)
  	    Test_p = &TestTable[3];
#else
        Test_p = &TestTable[5];
#endif
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */

        TestParams.Slot = Test_p->Slot;

        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
   else
   {
        PCCRD_DebugMessage ("Error :Wrong  Argument has been passed !");
        PCCRD_DebugMessage ("For Module A, type A or a !");
#ifndef STV0701
        PCCRD_DebugMessage ("For Module B, type B or b !");
#endif /* STV0701 */

        return (TRUE);

    }

    return (FALSE);

} /* end of tt_pccrdconfig() */

/*****************************************************************************
Name        : tt_pccrdvbci()

Description : Callback function registered with respect to  DVBCI_TEST Command.
              Calls PCCRD_DVBCI_Test() test function for ModuleA/B depending upon
              user choice.

Parameters  : Command line arguments reference and variable reference for
              storage of result after call(if any).

Return Value: FALSE - On Success
              TRUE  - On Error.
*****************************************************************************/

static BOOL tt_pccrdvbci( parse_t *pars_p, char *result_sym_p )
{
    PCCRD_TestEntry_t *Test_p;
    PCCRD_TestParams_t TestParams;
    char Text[4];
    PCCRD_TestResult_t Result;

    STTST_GetItem( pars_p, "", Text, 4);
    if ( strcmp( Text, "A" ) == 0 || strcmp( Text, "a" ) == 0)
    {
#if defined(ST_OSLINUX)
  	    Test_p = &TestTable[4];
#else
        Test_p = &TestTable[6];
#endif
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */
        TestParams.Slot = Test_p->Slot;
        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
    else if ( strcmp( Text, "B" ) == 0 || strcmp( Text, "b" ) == 0)
    {
#if defined(ST_OSLINUX)
  	    Test_p = &TestTable[5];
#else
        Test_p = &TestTable[7];
#endif
        PCCRD_DebugMessage("**************************************************");
        PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
        PCCRD_DebugMessage("**************************************************");

        /* Set up parameters for test function */

        TestParams.Slot = Test_p->Slot;
        /* Call test */
        Result = Test_p->TestFunction(&TestParams);

        FlushMessageQueue();
    }
    else
    {
        PCCRD_DebugMessage ("Error :Wrong  Argument has been passed !");
        PCCRD_DebugMessage ("For Module A, type A or a !");
#ifndef STV0701
        PCCRD_DebugMessage ("For Module B, type B or b !");
#endif /* STV0701*/
        return (TRUE);
    }

  return (FALSE);

} /* end of tt_pccrdvbci() */

#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5528) && !defined(STPCCRD_USE_ONE_MODULE)
/*****************************************************************************
Name        : tt_pccrdmultipleclient()

Description : Callback function registered with respect to  MULTIPLE_CLIENT Command.
              Calls PCCRDMultipleClients() test function for ModuleA/B depending upon
              user choice.

Parameters  : Command line arguments reference and variable reference for
              storage of result after call(if any).

Return      : FALSE - On Success
              TRUE  - On Error.
*****************************************************************************/

static BOOL tt_pccrdmultipleclient( parse_t *pars_p, char *result_sym_p )
{
    PCCRD_TestEntry_t *Test_p;
    PCCRD_TestParams_t TestParams;

    PCCRD_TestResult_t Result;

#if defined(ST_OSLINUX)
  	    Test_p = &TestTable[6];
#else
        Test_p = &TestTable[8];
#endif
    PCCRD_DebugMessage("**************************************************");
    PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
    PCCRD_DebugMessage("**************************************************");

    /* Set up parameters for test function */

    TestParams.Slot = Test_p->Slot;
    /* Call test */
    Result = Test_p->TestFunction(&TestParams);

    FlushMessageQueue();
    return (FALSE);

} /* end of tt_pccrdmultipleclient() */

#endif

#ifdef STPCCRD_HOT_PLUGIN_ENABLED
/*****************************************************************************
Name        : tt_pccrdinsertremove()

Description : Callback function registered with respect to  INSERT_REMOVE_TEST Command.
              Calls PCCRDInsertRemove() test function for ModuleA/B depending upon
              user choice.

Parameters  : Command line arguments reference and variable reference for
              storage of result after call(if any).

Return Value: FALSE - On Success
              TRUE  - On Error.
*****************************************************************************/

static BOOL tt_pccrdinsertremove( parse_t *pars_p, char *result_sym_p )
{
    PCCRD_TestEntry_t *Test_p;
    PCCRD_TestParams_t TestParams;
    PCCRD_TestResult_t Result;

    Test_p = &TestTable[9];
    PCCRD_DebugMessage("**************************************************");
    PCCRD_DebugPrintf(("%s\n",Test_p->TestInfo));
    PCCRD_DebugMessage("**************************************************");

    /* Set up parameters for test function */
    TestParams.Slot = Test_p->Slot;

    /* Call test */
    Result = Test_p->TestFunction(&TestParams);

    FlushMessageQueue();
    return (FALSE);

} /* end of tt_multipleclient() */
#endif /* STPCCRD_HOT_PLUGIN_ENABLED */
#endif /* STPCCRD_DISABLE_TESTTOOL */

/* Debug routines --------------------------------------------------------- */
/*****************************************************************************
Name        : PCCRD_EventToString()

Description : Converts the Event to String

Parameters  : STPCCRD_EventType_t Event

Return Value: Corresponding string
*****************************************************************************/

char *PCCRD_EventToString(STPCCRD_EventType_t Event)
{
    static char Events[][30] = {
        "STPCCRD_EVT_MOD_A_INSERTED",
        "STPCCRD_EVT_MOD_B_INSERTED",
        "STPCCRD_EVT_MOD_A_REMOVED",
        "STPCCRD_EVT_MOD_B_REMOVED"
    };
    return Events[(Event - STPCCRD_DRIVER_BASE)];
}

/*****************************************************************************
Name        : GetErrorText()

Description : Converts the Error Code to String

Parameters  : ST_ErrorCode_t ErrCode

Return Value: Corresponding string
*****************************************************************************/

char *GetErrorText(ST_ErrorCode_t ErrCode)
{
    switch(ErrCode)
    {
#if !defined(ST_OSLINUX)
       /* stboot.h */
        case STBOOT_ERROR_KERNEL_INIT:
            return("STBOOT_ERROR_KERNEL_INIT");
        case STBOOT_ERROR_KERNEL_START:
            return("STBOOT_ERROR_KERNEL_START");
        case STBOOT_ERROR_INTERRUPT_INIT:
            return("STBOOT_ERROR_INTERRUPT_INIT");
        case STBOOT_ERROR_INTERRUPT_ENABLE:
            return("STBOOT_ERROR_INTERRUPT_ENABLE");
        case STBOOT_ERROR_UNKNOWN_BACKEND:
            return("STBOOT_ERROR_UNKNOWN_BACKEND");
        case STBOOT_ERROR_BACKEND_MISMATCH:
            return("STBOOT_ERROR_BACKEND_MISMATCH");
        case STBOOT_ERROR_INVALID_DCACHE:
            return("STBOOT_ERROR_INVALID_DCACHE");
        case STBOOT_ERROR_SDRAM_INIT:
            return("STBOOT_ERROR_SDRAM_INIT");
#endif
        /* stddefs.h */
        case ST_ERROR_BAD_PARAMETER:
            return("ST_ERROR_BAD_PARAMETER");
        case ST_ERROR_NO_MEMORY:
            return("ST_ERROR_NO_MEMORY");
        case ST_ERROR_UNKNOWN_DEVICE:
            return("ST_ERROR_UNKNOWN_DEVICE");
        case ST_ERROR_ALREADY_INITIALIZED:
            return("ST_ERROR_ALREADY_INITIALIZED");
        case ST_ERROR_NO_FREE_HANDLES:
            return("ST_ERROR_NO_FREE_HANDLES");

        case ST_ERROR_OPEN_HANDLE:
            return("ST_ERROR_OPEN_HANDLE");
        case ST_ERROR_INVALID_HANDLE:
            return("ST_ERROR_INVALID_HANDLE");
        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            return("ST_ERROR_FEATURE_NOT_SUPPORTED");
        case ST_ERROR_INTERRUPT_INSTALL:
            return("ST_ERROR_INTERRUPT_INSTALL");
        case ST_ERROR_INTERRUPT_UNINSTALL:
            return("ST_ERROR_INTERRUPT_UNINSTALL");
        case ST_ERROR_TIMEOUT:
            return("ST_ERROR_TIMEOUT");
        case ST_ERROR_DEVICE_BUSY:
            return("ST_ERROR_DEVICE_BUSY");

        /* stevt.h */
        case STEVT_ERROR_INVALID_EVENT_ID:
            return("STEVT_ERROR_INVALID_EVENT_ID");
        case STEVT_ERROR_INVALID_SUBSCRIBER_ID:
            return("STEVT_ERROR_INVALID_SUBSCRIBER_ID");
        case STEVT_ERROR_ALREADY_SUBSCRIBED:
            return("STEVT_ERROR_ALREADY_SUBSCRIBED");
        case STEVT_ERROR_ALREADY_REGISTERED:
            return("STEVT_ERROR_ALREADY_REGISTERED");
        case STEVT_ERROR_NO_MORE_SPACE:
            return("STEVT_ERROR_NO_MORE_SPACE");
        case STEVT_ERROR_INVALID_EVENT_NAME:
            return("STEVT_ERROR_INVALID_EVENT_NAME");
        case STEVT_ERROR_ALREADY_UNREGISTERED:
            return("STEVT_ERROR_ALREADY_UNREGISTERED");
        case STEVT_ERROR_MISSING_NOTIFY_CALLBACK:
            return("STEVT_ERROR_MISSING_NOTIFY_CALLBACK");
        case STEVT_ERROR_NOT_SUBSCRIBED:
            return("STEVT_ERROR_NOT_SUBSCRIBED");

#if !defined(ST_OSLINUX)
#if !defined(ST_7710) && !defined(ST_5100) && !defined(ST_5301) && !defined(ST_5105)  \
&& !defined(ST_5107)

        /* sti2c.h */
        case STI2C_ERROR_LINE_STATE:
            return("STI2C_ERROR_LINE_STATE");
        case STI2C_ERROR_STATUS:
            return("STI2C_ERROR_STATUS");
        case STI2C_ERROR_ADDRESS_ACK:
            return("STI2C_ERROR_ADDRESS_ACK");
        case STI2C_ERROR_WRITE_ACK:
            return("STI2C_ERROR_WRITE_ACK");
        case STI2C_ERROR_NO_DATA:
            return("STI2C_ERROR_NO_DATA");
        case STI2C_ERROR_PIO:
            return("STI2C_ERROR_PIO");
        case STI2C_ERROR_BUS_IN_USE:
            return("STI2C_ERROR_BUS_IN_USE");
        case STI2C_ERROR_EVT_REGISTER:
            return("STI2C_ERROR_EVT_REGISTER");
        case STI2C_ERROR_NO_FREE_SSC:
            return("STI2C_ERROR_NO_FREE_SSC");
#endif

        /* sttbx.h */
        case STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED:
            return("STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED");

        /* stuart.h */
        case STUART_ERROR_OVERRUN:
            return("STUART_ERROR_OVERRUN");
        case STUART_ERROR_PARITY:
            return("STUART_ERROR_PARITY");
        case STUART_ERROR_FRAMING:
            return("STUART_ERROR_FRAMING");
        case STUART_ERROR_ABORT:
            return("STUART_ERROR_ABORT");
        case STUART_ERROR_RETRIES:
            return("STUART_ERROR_RETRIES");
        case STUART_ERROR_PIO:
            return("STUART_ERROR_PIO");
#endif
        /* stpccrd.h */
        case STPCCRD_ERROR_EVT:
            return("STPCCRD_ERROR_EVT");
        case STPCCRD_ERROR_BADREAD:
            return("STPCCRD_ERROR_BADREAD");
        case STPCCRD_ERROR_BADWRITE:
            return("STPCCRD_ERROR_BADWRITE");
        case STPCCRD_ERROR_I2C:
            return("STPCCRD_ERROR_I2C");
        case STPCCRD_ERROR_CIS_READ:
            return("STPCCRD_ERROR_CIS_READ");
        case STPCCRD_ERROR_NO_MODULE:
            return("STPCCRD_ERROR_NO_MODULE");
        case STPCCRD_ERROR_CI_RESET:
            return("STPCCRD_ERROR_CI_RESET");
        case STPCCRD_ERROR_MOD_RESET:
            return("STPCCRD_ERROR_MOD_RESET");

        case ST_NO_ERROR:
            return("ST_NO_ERROR");

        default:
            break;
    }
    return("UNKNOWN ERROR");
}
/* End Of File */

