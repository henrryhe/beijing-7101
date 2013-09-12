/*****************************************************************************
File Name   : boot_test.c

Description : Test harness for the STBOOT driver for STxxxxx.

Copyright (C) 2000-2006 STMicroelectronics

Revision History :

    18/05/00  MemorySize boot init params now uses SDRAM_SIZE constant
              exported by include.
    17Aug2004 Added semaphore test for checking whether timer working
              correctly (particularly needed to check for ST20C1 cores).
              Added OS21 support.

Reference   :

ST API Definition "STAPI Boot Initialization" DVD-API-191
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>                /* C libs */

#ifdef ST_OS20
#include "kernel.h"               /* OS20 includes */
#include "task.h"
#include "ostime.h"
#endif

#ifdef ST_OS21
#include <os21.h>
#ifdef ARCHITECTURE_ST40
#include <os21/st40.h>
#endif /*#ifdef ARCHITECTURE_ST40*/
#endif /*#ifdef ST_OS21*/

#include "stlite.h"               /* System includes */
#include "stdevice.h"

#include "stboot.h"               /* STAPI includes */
#include "boot_test.h"            /* Local includes */
#include "stsys.h"
#include "stcommon.h"

/* Private types/constants ------------------------------------------------ */

#define min(x,y)            ((x)<(y)?x:y)

#define ST_ERROR_UNKNOWN    ((U32)-1)

#define CACHE_REGS_SIZE     0xA01

/* 4629 register addresses*/
#ifdef ARCHITECTURE_ST20
#define STI4629_PLL_CFG        (STI4629_BASE_ADDRESS + 0x220)
#define STI4629_PLL_MDIV       (STI4629_BASE_ADDRESS + 0x221)
#define STI4629_PLL_NDIV       (STI4629_BASE_ADDRESS + 0x222)
#define STI4629_PLL_PDIV       (STI4629_BASE_ADDRESS + 0x223)
#define STI4629_PLL_SETUP_LOW  (STI4629_BASE_ADDRESS + 0x224)
#define STI4629_PLL_SETUP_HIGH (STI4629_BASE_ADDRESS + 0x225)
#define STI4629_PLL_NRST       (STI4629_BASE_ADDRESS + 0x226)
#endif

#ifdef ARCHITECTURE_ST40
#define STI4629_PLL_CFG        (STI4629_ST40_BASE_ADDRESS + 0x220) /*0xA2800220*/
#define STI4629_PLL_MDIV       (STI4629_ST40_BASE_ADDRESS + 0x221) /*0xA2800221*/
#define STI4629_PLL_NDIV       (STI4629_ST40_BASE_ADDRESS + 0x222) /*0xA2800222*/
#define STI4629_PLL_PDIV       (STI4629_ST40_BASE_ADDRESS + 0x223) /*0xA2800223*/
#define STI4629_PLL_SETUP_LOW  (STI4629_ST40_BASE_ADDRESS + 0x224) /*0xA2800224*/
#define STI4629_PLL_SETUP_HIGH (STI4629_ST40_BASE_ADDRESS + 0x225) /*0xA2800225*/
#define STI4629_PLL_NRST       (STI4629_ST40_BASE_ADDRESS + 0x226) /*0xA2800226*/
#endif

/* Private variables ------------------------------------------------------ */

/* Test harness revision number */
static const ST_Revision_t Revision = "2.15.2";

/* Error message look up table */
static BOOT_ErrorMessage BOOT_ErrorLUT[] =
{
    /* STBOOT */
    { STBOOT_ERROR_KERNEL_INIT, "STBOOT_ERROR_KERNEL_INIT" },
    { STBOOT_ERROR_KERNEL_START, "STBOOT_ERROR_KERNEL_START" },
    { STBOOT_ERROR_INTERRUPT_INIT, "STBOOT_ERROR_INTERRUPT_INIT" },
    { STBOOT_ERROR_INTERRUPT_ENABLE, "STBOOT_ERROR_INTERRUPT_ENABLE" },
    { STBOOT_ERROR_UNKNOWN_BACKEND, "STBOOT_ERROR_UNKNOWN_BACKEND" },
    { STBOOT_ERROR_BACKEND_MISMATCH, "STBOOT_ERROR_BACKEND_MISMATCH" },
    { STBOOT_ERROR_INVALID_DCACHE, "STBOOT_ERROR_INVALID_DCACHE" },
    { STBOOT_ERROR_SDRAM_INIT, "STBOOT_ERROR_SDRAM_INIT" },

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

/* Private function prototypes -------------------------------------------- */

BOOT_TestResult_t BOOTInvalidConfig(BOOT_TestParams_t *Params_p);
BOOT_TestResult_t BOOTValidConfig(BOOT_TestParams_t *Params_p);
BOOT_TestResult_t BOOTInfo(BOOT_TestParams_t *Params_p);
#if defined (ARCHITECTURE_ST20) || defined (ST_OS21)
BOOT_TestResult_t BOOTMemoryTest(BOOT_TestParams_t *Params_p);
#endif
#if defined (mb376) || defined(espresso)
BOOT_TestResult_t BOOT4629Test(BOOT_TestParams_t* Params_p);
#endif

/* Test table */
static BOOT_TestEntry_t TestTable[] =
{
#ifdef ST_OS20
    { BOOTInfo, "Test get backend info function", 1 },
#endif
#if TEST_INVALID /*1*/
#ifdef ST_OS20
    { BOOTInvalidConfig, "Test DCACHE setup with invalid settings", 1 },
#endif
#else
#ifdef ST_OS21
    { BOOTValidConfig, "Test STBOOT_Init() & Timer functioning", 1 },
#endif
#ifdef ST_OS20
    { BOOTValidConfig, "Test DCACHE setup(valid settings) & Timer", 1 },
#endif
#if defined (ST_OS20)
    { BOOTMemoryTest, "Write/Read test on various memory sections", 1},
#endif
#if defined(ST_OS21)
    { BOOTMemoryTest, "Write/Read test on memory", 1},
#endif
#if defined (mb376) || defined(espresso)
    { BOOT4629Test, "Read test on 4629 registers", 1},
#endif
#endif /*1*/
    { 0, "", 0 }
};

#if defined (ST_OS21)
/*unsigned char  InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];*/
/*int            TheInternalPartition;
partition_t      *internal_partition = (partition_t *) &TheInternalPartition;*/
/*partition_t    *internal_partition=NULL;*/

#define          SYSTEM_MEMORY_SIZE          0x100000
unsigned char    ExternalBlock[SYSTEM_MEMORY_SIZE];
partition_t      *system_partition;


#elif defined (ARCHITECTURE_ST20)
/* Data partitions for memory tests */
unsigned char    InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];
partition_t      TheInternalPartition;

#pragma ST_section      ( InternalBlock, "internal_section_noinit")

#ifndef CACHED_SIZE
#define          CACHED_SIZE                 0x00400000    /* 4 MBytes */
#endif
unsigned char    CachedBlock[CACHED_SIZE];
partition_t      TheCachedPartition;
#pragma ST_section      ( CachedBlock, "system_section_noinit")

#ifndef NCACHE_SIZE
#if defined(mb361) && defined(UNIFIED_MEMORY)
#define          NCACHE_SIZE                 0x00800000    /* 8 MBytes */
#else
#define          NCACHE_SIZE                 0x00080000    /* 512 KBytes */
#endif
#endif

unsigned char    NcacheBlock[NCACHE_SIZE];
partition_t      TheNcachePartition;
#if defined(mb361) && defined(UNIFIED_MEMORY)
#pragma ST_section      ( NcacheBlock, "ncache2_section")
#else
#pragma ST_section      ( NcacheBlock, "ncache_section")
#endif

#define          SMI_SIZE                    0x00400000    /* 4 MBytes */
partition_t      TheSMIPartition;
#if defined(mb314) /* force mb314/382 SMI if db573 is present */
#define          SMI_BASE_ADDRESS               MB314_SDRAM_BASE_ADDRESS
#elif defined(mb382)
#define          SMI_BASE_ADDRESS               MB382_SDRAM_BASE_ADDRESS
#elif defined SDRAM_WINDOW_BASE_ADDRESS
#define          SMI_BASE_ADDRESS               SDRAM_WINDOW_BASE_ADDRESS
#else
#define          SMI_BASE_ADDRESS               SDRAM_BASE_ADDRESS
#endif

partition_t      TheEMIPartition; /* only used on mb376 */

#else

#error Undefined ARCHITECTURE

#endif

#ifdef ARCHITECTURE_ST40
/* No real h/w on ST40 */
#define USE_SW_REGS     1
#endif

#ifdef USE_SW_REGS
static U8 CacheRegsMemory[CACHE_REGS_SIZE];
#else
static volatile U8 *CacheRegsMemory = (U8 *)CACHE_BASE_ADDRESS;
#endif

U32 testtime1,testtime2;
/* Function definitions --------------------------------------------------- */

int main(int argc, char *argv[])
{
    BOOT_TestResult_t Total = TEST_RESULT_ZERO, Result;
    BOOT_TestEntry_t *Test_p;
    U32 Section = 1;
    BOOT_TestParams_t TestParams;
    
    BOOT_DebugMessage("**************************************************");
    BOOT_DebugMessage("                STBOOT Test Harness");
    BOOT_DebugPrintf(("STBOOT Driver Revision: %s\n", STBOOT_GetRevision()));
    BOOT_DebugPrintf(("Test Harness Revision: %s\n", Revision));
#if defined(ST_OS21)
/*Print Nothing*/
#elif defined(UNIFIED_MEMORY) || defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5700) || defined(ST_5188) || defined(ST_5107)
    BOOT_DebugPrintf(("UNIFIED_MEMORY: "));
    printf(("TRUE\n"));
#else
    BOOT_DebugPrintf(("UNIFIED_MEMORY: "));
    printf(("FALSE\n"));
#endif
    BOOT_DebugPrintf(("DISABLE_DCACHE: "));
#if defined DISABLE_DCACHE
    printf(("TRUE\n"));
#else
    printf(("FALSE\n"));
#endif
    BOOT_DebugPrintf(("DISABLE_ICACHE: "));
#if defined DISABLE_ICACHE
    printf(("TRUE\n"));
#else
    printf(("FALSE\n"));
#endif
    BOOT_DebugMessage("**************************************************");
    BOOT_DebugMessage("              Dependencies Revisions              ");
    BOOT_DebugPrintf(("           BOARD:  %s\n",  BOARD_GetRevision() ));
    BOOT_DebugPrintf(("            CHIP:  %s\n",  CHIP_GetRevision() ));
    BOOT_DebugPrintf(("         INCLUDE:  %s\n",  INCLUDE_GetRevision() ));
    BOOT_DebugPrintf(("        STCOMMON:  %s\n",  STCOMMON_REVISION      ));
    BOOT_DebugPrintf(("           STSYS:  %s\n",  STSYS_GetRevision()    ));
    BOOT_DebugMessage("**************************************************");
    BOOT_DebugMessage("                  Test built on                   ");
    BOOT_DebugPrintf(("        Date: %s   Time: %s \n", __DATE__, __TIME__ ));

    Test_p = TestTable;
    while (Test_p->TestFunction != NULL)
    {
        while (Test_p->RepeatCount > 0)
        {
            BOOT_DebugMessage("**************************************************");
            BOOT_DebugPrintf(("SECTION %d - %s\n", Section,
                              Test_p->TestInfo));
            BOOT_DebugMessage("**************************************************");

            /* Set up parameters for test function */
            TestParams.Ref = Section;

            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            /* Update counters */
            Total.NumberPassed += Result.NumberPassed;
            Total.NumberFailed += Result.NumberFailed;
            Test_p->RepeatCount--;
        }

        Test_p++;
        Section++;
    }

    /* Output running analysis */
    BOOT_DebugMessage("**************************************************");
    BOOT_DebugPrintf(("PASSED: %d\n", Total.NumberPassed));
    BOOT_DebugPrintf(("FAILED: %d\n", Total.NumberFailed));
    testtime2=time_now();
    BOOT_DebugPrintf(("Time taken by tests after STBOOT_Init(in ticks):%d\n",(testtime2-testtime1)));
    BOOT_DebugMessage("**************************************************");

    return 0;
}

/* Test harness tests ----------------------------------------------------- */

/*****************************************************************************
BOOTInfo()
Description:
    Test backend info function.
Parameters:
    Params_p,   Not used.
Return Value:
See Also:
*****************************************************************************/
#ifdef ST_OS20
BOOT_TestResult_t BOOTInfo(BOOT_TestParams_t *Params_p)
{
    BOOT_TestResult_t Res = { 0, 0 };
    ST_ErrorCode_t Error;
    STBOOT_Backend_t Backend;
    STBOOT_Device_t Device = 0x00;

#if defined(ST_5508)
    Device = STBOOT_DEVICE_5508;
#elif defined(ST_5510)
    Device = STBOOT_DEVICE_5510;
#elif defined(ST_5512)
    Device = STBOOT_DEVICE_5512;
#elif defined(ST_5514)
    Device = STBOOT_DEVICE_5514;
#elif defined(ST_5518)
    Device = STBOOT_DEVICE_5518;
#elif defined(ST_5580)
    Device = STBOOT_DEVICE_5580;
#elif defined(ST_TP3)
    Device = STBOOT_DEVICE_TP3;
#elif defined(ST_5516)
    Device = STBOOT_DEVICE_5516;
#elif defined(ST_5517)
    Device = STBOOT_DEVICE_5517;
#elif defined(ST_5528)
    Device = STBOOT_DEVICE_5528;
#elif defined(ST_5100)
    Device = STBOOT_DEVICE_5100;
#elif defined(ST_5101)
    Device = STBOOT_DEVICE_5101;
#elif defined(ST_7710)
    Device = STBOOT_DEVICE_7710;
#elif defined(ST_5105)
    Device = STBOOT_DEVICE_5105;
#elif defined(ST_5700)
    Device = STBOOT_DEVICE_5700;
#elif defined(ST_5188)
    Device = STBOOT_DEVICE_5188;
#elif defined(ST_5107)
    Device = STBOOT_DEVICE_5107;
#else
#error No OS20 device specified (check DVD_FRONTEND)
#endif

    /* Note the "GetBackendInfo" actually describes the chip OS20 is running on,
      which may not be DVD_BACKEND ! The naming is historical */

    Error = STBOOT_GetBackendInfo(&Backend);
    if (Error == ST_NO_ERROR)
    {
        if (Backend.DeviceType != Device)
        {
            char Msg[100];

            sprintf(Msg, "Incorrect device 0x%x - expected 0x%x",
                    Backend.DeviceType,
                    Device);
            BOOT_TestFailed(Res,
                            Msg,
                            BOOT_ErrorString(Error));

            sprintf( Msg, "device_name() returned: %s",
                     device_name(device_id()) );
            BOOT_DebugMessage( Msg );
        }
        else
        {
            BOOT_TestPassed(Res);
        }
    }
    else
    {
        BOOT_TestFailed(Res, "Expected ST_NO_ERROR", BOOT_ErrorString(Error));
    }

    return Res;
}
#endif /*#ifdef ST_OS20*/

/*****************************************************************************
BOOTInvalidConfig()
Description:
    Calls stboot init. in order to test dcache initialization with
    an invalid dcache map.
Parameters:
    Params_p,   Not used.
Return Value:
See Also:
*****************************************************************************/
#ifdef ST_OS20
BOOT_TestResult_t BOOTInvalidConfig(BOOT_TestParams_t *Params_p)
{
    ST_ErrorCode_t Error;
    STBOOT_InitParams_t BootParams;
    STBOOT_TermParams_t BootTerm;
    BOOT_TestResult_t Res = { 0, 0 };
    STBOOT_DCache_Area_t DCacheMap[][5] =
    {
        {
            { (U32 *)0xC0040000, (U32 *)0xC008FFFF }, /* invalid */
            { (U32 *)0xC0200000, (U32 *)0xC026FFFF }, /* ok */
            { (U32 *)0xC0270000, (U32 *)0xC027FFFF }, /* ok */
            { NULL, NULL }
        },
        {
            { (U32 *)0xC0041000, (U32 *)0xC007FFFF }, /* invalid */
            { (U32 *)0xC0200000, (U32 *)0xC026FFFF }, /* ok */
            { (U32 *)0xC0270000, (U32 *)0xC027FFFF }, /* ok */
            { NULL, NULL }
        },
        {
            { (U32 *)0xC0040000, (U32 *)0xC007FFFF }, /* ok */
            { (U32 *)0xC0200000, (U32 *)0xC026FFFF }, /* ok */
            { (U32 *)0xC0270000, (U32 *)0xC027FFFF }, /* ok */
            { (U32 *)0x40000001, (U32 *)0x4FFFFFFF }, /* invalid */
            { NULL, NULL }
        }
    };
    U32 i;

    for ( i = 0; i < 3; i++ )
    {
        BootParams.CacheBaseAddress = (U32 *)CacheRegsMemory;
        BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
	BootParams.MemorySize = SDRAM_SIZE;
#if defined DISABLE_ICACHE
        BootParams.ICacheEnabled = FALSE;
#else
        BootParams.ICacheEnabled = TRUE;
#endif
        /* Ignore DISABLE_DCACHE compile option as
           this is a test of bad DCache settings */
        BootParams.DCacheMap = DCacheMap[i];
        BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
        BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
        BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

        Error = STBOOT_Init( "boot", &BootParams );
        BOOT_DebugError("STBOOT_Init()", Error);

        if (Error == STBOOT_ERROR_INVALID_DCACHE)
        {
            BOOT_TestPassed(Res);
        }
        else
        {
            BOOT_TestFailed(Res, "Expected STBOOT_ERROR_INVALID_DCACHE",
                            BOOT_ErrorString(Error));
        }

        Error = STBOOT_Term( "boot", &BootTerm );
        BOOT_DebugError("STBOOT_Term()", Error);
    }

/* The cache structure is different in 5514/16/17/28/5100/5101/7710/5105/5700/5188/5107 */

#if !defined(ST_5514) && !defined(ST_5516) && !defined(ST_5517) && !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_5188) && !defined(ST_5107)
    /* Dump out value of cache register map */
    BOOT_DebugMessage("Cache registers dump...");
    for (i = 0; i < CACHE_REGS_SIZE; i += 0x100)
        printf("--- 0x%03x = 0x%02x\n", i, CacheRegsMemory[i]);
#endif

    return Res;
} /* BOOTInvalidConfig() */
#endif /*#ifdef ST_OS20*/

/*****************************************************************************
BOOTValidConfig()
Description:
    Calls stboot init. in order to test dcache initialization with
    a valid dcache map.
Parameters:
    Params_p,   Not used.
Return Value:
See Also:
*****************************************************************************/

BOOT_TestResult_t BOOTValidConfig(BOOT_TestParams_t *Params_p)
{
    ST_ErrorCode_t Error;
    STBOOT_InitParams_t BootParams;
    STBOOT_TermParams_t BootTerm;
    U32 time_beforecall, time_aftercall;
    BOOT_TestResult_t Res = { 0, 0 };
    
    #ifdef ST_OS21
    semaphore_t *test_sem1;
    osclock_t test_sem_timeout; /*no. of ticks for delay*/
    #endif
    #ifdef ST_OS20
    semaphore_t test_sem1;
    clock_t test_sem_timeout; /*no. of ticks for delay*/
    #endif
    S8 test_sem_result=0;

#if !defined (DISABLE_DCACHE)
#if defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS }, /* assumed ok */
        { NULL, NULL }
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5301)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5525)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#ifdef ARCHITECTURE_ST200
#ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP
#if defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t HostMemoryMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS }, /* assumed ok */
        { NULL, NULL }
    };
#elif defined(ST_5301)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5525)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#endif /*#ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP*/
#endif /*#ifdef ARCHITECTURE_ST200*/
#endif /*#if !defined (DISABLE_DCACHE)*/

    
    BootParams.CacheBaseAddress = (U32 *)CacheRegsMemory;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
    BootParams.MemorySize = SDRAM_SIZE;
#if defined DISABLE_DCACHE
    BootParams.DCacheMap = NULL;
    #ifdef ST_OS21
    BootParams.DCacheEnabled = FALSE;
    #endif
#else
    BootParams.DCacheMap = DCacheMap;
    #ifdef ST_OS21
    BootParams.DCacheEnabled = TRUE;
    #ifdef ARCHITECTURE_ST200
    #ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP
    BootParams.HostMemoryMap = HostMemoryMap;
    #endif
    #endif
    #endif
#endif
#if defined DISABLE_ICACHE
    BootParams.ICacheEnabled = FALSE;
#else
    BootParams.ICacheEnabled = TRUE;
#endif
    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
#ifdef ST_OS21
    BootParams.TimeslicingEnabled = TRUE;
#endif
    
    Error = STBOOT_Init( "boot", &BootParams );
    BOOT_DebugError("STBOOT_Init()", Error);
    if (Error != ST_NO_ERROR)
    {
        BOOT_TestFailed(Res, "STBOOT_Init() failed", "");
    }
    
    testtime1=time_now();
    
    /*Adding a semaphore to check timer functionality*/
    #ifdef ST_OS21
    test_sem1 = semaphore_create_fifo(0);
    #else
    semaphore_init_fifo_timeout(&test_sem1, 0);
    #endif
    
    test_sem_timeout = (10*ST_GetClocksPerSecond());
    BOOT_DebugPrintf(("Value returned by ST_GetClocksPerSecond():%d\n",ST_GetClocksPerSecond()));
    BOOT_DebugPrintf(("Checking functioning of Timer:\n"));
    BOOT_DebugPrintf(("Calling semaphore_wait_timeout() with Timeout of 10 seconds\n"));
    
    #ifdef ST_OS21
        test_sem_result = semaphore_wait_timeout(test_sem1, &test_sem_timeout);
    #else
        test_sem_result = semaphore_wait_timeout(&test_sem1, &test_sem_timeout);
    #endif
    
    #ifdef ST_OS21
    if (test_sem_result == OS21_FAILURE)
    {
        BOOT_DebugPrintf(("Timeout Occurred.\n"));
    }
    else
    {
    	BOOT_TestFailed(Res, "Timeout didnt occur", "");
    }
    #else
    if (test_sem_result == -1)
    {
        BOOT_DebugPrintf(("Timeout Occurred.\n"));
    }
    else
    {
    	BOOT_TestFailed(Res, "Timeout didnt occur", "");
    }
    #endif /*#ifdef ST_OS21*/
    
    #ifdef ST_OS21
    test_sem_result = semaphore_delete(test_sem1);
    if (test_sem_result == OS21_FAILURE)
    {
        BOOT_TestFailed(Res, "Error!!! Semaphore could not be deleted.", "");
    }
    else
    {
        BOOT_DebugPrintf(("Semaphore Deleted.\n"));
    }
    #endif
    #ifdef ST_OS20
    semaphore_delete(&test_sem1);
    BOOT_DebugPrintf(("Semaphore Deleted.\n"));
    #endif
    BOOT_DebugPrintf(("Calling task_delay() of 10 seconds\n"));
    time_beforecall=time_now();
    task_delay(10*ST_GetClocksPerSecond());
    time_aftercall=time_now();
    BOOT_DebugPrintf(("Task_delay() completed, Time Taken:%d seconds\n",((time_aftercall-time_beforecall)/ST_GetClocksPerSecond() )));
    

/* The cache structure is different in 5514/16/17/5100/5101/7710/5105/5700/7100/5301/8010/7109/5188/5525/5107/7200 */
#if !defined(ST_5514) &&  !defined(ST_5516) &&  !defined(ST_5517) && !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_7100) && !defined(ST_5301) && !defined(ST_8010) && !defined(ST_7109) && !defined(ST_5188) && !defined(ST_5525) && !defined(ST_5107) && !defined(ST_7200)
    /* Dump out value of cache register map */
    BOOT_DebugMessage("Cache registers dump...");
    {
        U32 i;
        for (i = 0; i < CACHE_REGS_SIZE; i += 0x100)
            printf("--- 0x%03x = 0x%02x\n", i, CacheRegsMemory[i]);
    }
#endif

#if defined(ST_5508) || defined(ST_5510) || defined(ST_5512) || \
    defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5518)
    {
        /* Confirm new policy PBO = off */
        U32 CFG_CCF_val = (U32) *(U8*)0x1;
        
        BOOT_DebugPrintf(("CFG_CCF = 0x%02X: PBO = %s\n", CFG_CCF_val,
                          CFG_CCF_val & (1 << 5) ? "1" : "0"));
    }
#endif
    
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    /* confirm new MPEGCONTROL = 0x77000 not 0x67000 */
    BOOT_DebugPrintf(("MPEGCONTROL = 0x%08X\n", *(U32*)0x5000));
#endif
    
    if (Error == ST_NO_ERROR)
    {
        BOOT_TestPassed(Res);
    }
    else
    {
        BOOT_TestFailed(Res, "Expected ST_NO_ERROR",
                        BOOT_ErrorString(Error));
    }

    Error = STBOOT_Term( "boot", &BootTerm );
    BOOT_DebugError("STBOOT_Term()", Error);

    return Res;
} /* BOOTValidConfig() */

#if defined (ARCHITECTURE_ST20)
typedef struct {
    partition_t *Partition_p;
    U32 TestSize;
    U8  Name[32];
} MemTest_t;
#endif

#ifdef ST_OS20
MemTest_t MemTest[] = {
    { &TheInternalPartition, (3*sizeof(InternalBlock)/4), "internal memory" },
    { &TheCachedPartition,   (3*sizeof(CachedBlock)/4), "cacheable memory" },
    { &TheNcachePartition,   (3*sizeof(NcacheBlock)/4), "non-cacheable memory" },
    
#if !defined (UNIFIED_MEMORY) && !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_5188) && !defined(ST_5107)
    { &TheSMIPartition,      (3*SMI_SIZE/4), "SMI memory" },
#endif
#if defined(mb376)
    /* we always run 5528 in the 'unified memory' configuration; here we test
      the other (EMI) memory block present on mb376 (not Espresso). The
      base and size defines come from mb376.h */
    /*{ &TheEMIPartition,      (EMI_MEMORY_SIZE/3), "EMI memory" },*/
    { &TheEMIPartition,      (SDRAM_SIZE/3), "LMI memory" },
#endif

    /* Last entry, signifies end of memory tests */
    { NULL, 0, "" }
};
#endif /*#ifdef ST_OS20*/

/*****************************************************************************
BOOTMemoryTest()
Description:
    Tests various areas of memory using random write/read tests
    NOTE: Assumes successful STBOOT_Init() has already been performed
Parameters:
    Params_p,   Not used.
Return Value:
See Also:
*****************************************************************************/
    
BOOT_TestResult_t BOOTMemoryTest(BOOT_TestParams_t *Params_p)
{
    int i;
    #ifndef ST_OS21
    int Test;
    #endif
    U32 *Data_p;
    U32 Seed;
    BOOT_TestResult_t Res = { 0, 0 };
    char Msg[80];

    /*
    ** Setup memory partitions required for tests
    **
    ** NB internal_partition and system_partition should really be initialised before
    ** kernel_init. As it stands, any allocation from these partitions risks
    ** clobbering kernel data structures. We only get away with it because we don't
    ** create multiple threads.
    */
#ifdef ST_OS21
    system_partition = partition_create_heap((U8*)ExternalBlock, sizeof(ExternalBlock));
#endif

#ifdef ST_OS20
    if(partition_init_simple(&TheInternalPartition, (U8*) InternalBlock, sizeof(InternalBlock)))
    {
        sprintf(Msg, "partition_init_simple(Internal, %u) FAILED",
                sizeof(InternalBlock) );
        BOOT_TestFailed(Res, Msg, "ST_ERROR_NO_MEMORY");
        return Res;
    }
    if(partition_init_heap  (&TheNcachePartition, NcacheBlock, sizeof(NcacheBlock)))
    {
        sprintf(Msg, "partition_init_heap(NCACHE, %u) FAILED",
                sizeof(NcacheBlock) );
        BOOT_TestFailed(Res, Msg, "ST_ERROR_NO_MEMORY");
        return Res;
    }
    if(partition_init_heap  (&TheCachedPartition, CachedBlock, sizeof(CachedBlock)))
    {
        sprintf(Msg, "partition_init_heap(CACHED, %u) FAILED",
                sizeof(CachedBlock) );
        BOOT_TestFailed(Res, Msg, "ST_ERROR_NO_MEMORY");
        return Res;
    }
#if !defined (UNIFIED_MEMORY) && !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_5188) && !defined(ST_5107)
    if(partition_init_heap  (&TheSMIPartition, (void *)SMI_BASE_ADDRESS, SMI_SIZE))
    {
        sprintf(Msg, "partition_init_heap(SMI, %u) FAILED", SMI_SIZE);
        BOOT_TestFailed(Res, Msg, "ST_ERROR_NO_MEMORY");
        return Res;
    }
#endif
#if defined(mb376)
    /*if(partition_init_heap  (&TheEMIPartition, (void *)EMI_MEMORY_BASE_ADDRESS, EMI_MEMORY_SIZE))*/
    if(partition_init_heap  (&TheEMIPartition, (void *)SDRAM_BASE_ADDRESS, SDRAM_SIZE))
    {
        sprintf(Msg, "partition_init_heap(EMI, %u) FAILED", SMI_SIZE);
        BOOT_TestFailed(Res, Msg, "ST_ERROR_NO_MEMORY");
        return Res;
    }
#endif
#endif /*#ifdef ST_OS20*/

    /*
    ** Perform memory tests
    */
#ifdef ST_OS21
        Seed = (U32)time_now();
        Data_p = memory_allocate(system_partition, 3*sizeof(ExternalBlock)/4);
        if(Data_p == NULL)
        {
            /* unable to allocated required test area */
            sprintf(Msg, "Unable to allocate %u bytes buffer", 3*sizeof(ExternalBlock)/4);
            BOOT_TestFailed(Res, Msg, "ST_ERROR_NO_MEMORY");
        }
        
        /* Display stage test message */
        sprintf(Msg, "Writing and reading %u bytes",3*sizeof(ExternalBlock)/4);
        BOOT_DebugMessage(Msg);

        /* Write to test area */
        for(i=0; i<(3*sizeof(ExternalBlock)/4)/4; i++)
        {
            Data_p[i] = Seed + i;
        }
        
        /* Read back from test area and compare */
        for(i=0; i<(3*sizeof(ExternalBlock)/4)/4; i++)
        {
            if(Data_p[i] != Seed + i)
            {
                /* abort compare operation on first failure */
                sprintf(Msg, "read %u", Data_p[i]);
                BOOT_TestFailed(Res, "Data mismatch", Msg);
                break;
            }
        }

        /* Display Pass message if appropriate */
        if(i == (3*sizeof(ExternalBlock)/4)/4)
        {
            BOOT_TestPassed(Res);
        }

        /* Clean up allocated memory */
        memory_deallocate(system_partition, Data_p);

#else
    for(Test=0; MemTest[Test].Partition_p != NULL; Test++)
    {
        /* Different data every iteration */
        Seed = (U32)time_now();

        /* Allocate memory for test */
        Data_p = memory_allocate(MemTest[Test].Partition_p,
                                 MemTest[Test].TestSize);
        if(Data_p == NULL)
        {
            /* unable to allocated required test area,
               go on to next test */
            sprintf(Msg, "Unable to allocate %u bytes buffer from %s",
                    MemTest[Test].TestSize, MemTest[Test].Name);
            BOOT_TestFailed(Res, Msg,
                            "ST_ERROR_NO_MEMORY");
            continue;
        }

        /* Display stage test message */
        sprintf(Msg, "Writing and reading %u bytes to %s...",
                MemTest[Test].TestSize, MemTest[Test].Name);
        BOOT_DebugMessage(Msg);

        /* Write to test area */
        for(i=0; i<MemTest[Test].TestSize/4; i++)
        {
            Data_p[i] = Seed + i;
        }
        
        /* Read back from test area and compare */
        for(i=0; i<MemTest[Test].TestSize/4; i++)
        {
            if(Data_p[i] != Seed + i)
            {
                /* abort compare operation on first failure */
                sprintf(Msg, "read %u", Data_p[i]);
                BOOT_TestFailed(Res, "Data mismatch", Msg);
                break;
            }
        }

        /* Display Pass message if appropriate */
        if(i == MemTest[Test].TestSize/4)
        {
            BOOT_TestPassed(Res);
        }

        /* Clean up allocated memory */
        memory_deallocate(MemTest[Test].Partition_p, Data_p);
    }
#endif /*#ifdef ST_OS21*/
    return Res;
}


#if defined (mb376) || defined(espresso)
/*****************************************************************************
BOOT4629Test()
Description:
    Reads the 4629 PLL regsisters
    NOTE: Assumes successful STBOOT_Init() has already been performed
Parameters:
    Params_p,   Not used.
Return Value:
See Also:
*****************************************************************************/
    
BOOT_TestResult_t BOOT4629Test(BOOT_TestParams_t* Params_p)
{
    BOOT_TestResult_t Res     = { 0, 0 };
    U8                TempReg = 0xFF;
    
    TempReg = STSYS_ReadRegDev8((void*)STI4629_PLL_CFG);
    BOOT_DebugPrintf(("4629 PLL CFG is 0x%02X\n", (U32)TempReg));
    TempReg = STSYS_ReadRegDev8((void*)STI4629_PLL_MDIV);
    BOOT_DebugPrintf(("4629 PLL MDIV is 0x%02X\n", TempReg));
    TempReg = STSYS_ReadRegDev8((void*)STI4629_PLL_NDIV);
    BOOT_DebugPrintf(("4629 PLL NDIV is 0x%02X\n", TempReg));
    TempReg = STSYS_ReadRegDev8((void*)STI4629_PLL_PDIV);
    BOOT_DebugPrintf(("4629 PLL PDIV is 0x%02X\n", TempReg));
    TempReg = STSYS_ReadRegDev8((void*)STI4629_PLL_SETUP_LOW);
    BOOT_DebugPrintf(("4629 PLL SETUP LOW is 0x%02X\n", TempReg));
    TempReg = STSYS_ReadRegDev8((void*)STI4629_PLL_SETUP_HIGH);
    BOOT_DebugPrintf(("4629 PLL SETUP HIGH 0x%02X\n", TempReg));
    
    return Res;
}
#endif

/* Debug routines --------------------------------------------------------- */

char *BOOT_ErrorString(ST_ErrorCode_t Error)
{
    BOOT_ErrorMessage *mp;

    mp = BOOT_ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return mp->ErrorMsg;
}

/* End of module */
