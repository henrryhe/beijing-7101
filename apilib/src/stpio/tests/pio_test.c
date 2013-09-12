/*****************************************************************************
File Name   : pio_test.c

Description : Test harness for the PIO driver for STi55xx.

Copyright (C) 2005 STMicroelectronics

Revision History    :

    28feb00     Imported test code from original pio test harness.
                Converted all output to use toolbox via macro wrappers.

    31mar00     Modified tests to perform pio pin loopback from output
                to input pin -- this allows complete automation of the
                the tests and avoids the necessity of having to watch
                a heartbeat LED.

    [...]
    08Apr03     Added defines for 5528
    17Sep03     Added defines for 5100
    30Jan04     Added defines for 7710
    30Sep04     Added defines for 5105
    13Dec04     Added defines for 5700
    10Jan05     Added defines for 7100
    14Jan05     Added defines for 5301
    11Apr05     Added defines for 8010
    27Oct05     Added defines for 7109
    26Nov05     Added defines for 5188
    05Jan06     Added defines for 5525
    24Apr06     Added defines for 5107

Reference           :

*****************************************************************************/

#include <stdio.h>                      /* C libs */
#include <stdlib.h>
#include <string.h>

#if defined(ST_OSLINUX)
#include <unistd.h>
#endif

#include "stlite.h"                     /* System */
#include "stdevice.h"

#include "stpio.h"                      /* STAPI */
#include "stcommon.h"

#include "stos.h"

#if !defined(ST_OSLINUX)
#include "stboot.h"
#ifdef STTBX_PRINT
#include "sttbx.h"
#endif
#endif /*ST_OSLINUX*/

#include "pio_test.h"                   /* Local */
#ifdef CODETEST
#include "codetest.h"
#endif

#ifdef ST_OS20
#include "debug.h"
#endif

/*****************************************************************************
Types and definitions
*****************************************************************************/

/* Register offsets for direct set/clear tests (should be same as stpio.c) */
#define PIO_OUT_SET               1
#define PIO_OUT_CLR               2

/* Enum of ASC devices */
enum PIO_Device_e {
    PIO_DEVICE_0 = 0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5
};

#define ST_ERROR_UNKNOWN ((U32)-1)

/* Define number of iterations for each test */
#ifndef APITestNumIterations
#define APITestNumIterations            1
#endif

#ifndef SetClrTestNumIterations
#define SetClrTestNumIterations         1
#endif

#ifndef SpeedTestNumIterations
#define SpeedTestNumIterations          1
#endif

#ifndef MultipleUseTestNumIterations

/* Test not available on MB317 (GX1-DEMO board) */
#ifndef mb317
#define MultipleUseTestNumIterations    1
#else
#define MultipleUseTestNumIterations    0
#endif
#endif

#ifndef CompareTestNumIterations
#define CompareTestNumIterations        1
#endif

/*****************************************************************************
Global data
*****************************************************************************/

/* Test harness revision number */
static U8 Revision[] = "1.4.0A4";

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5100) || defined(ST_7710) \
|| defined(ST_5700) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
#define NUM_PIO_DEVICES     6
#elif defined(ST_5528) || defined(ST_7200)
#define NUM_PIO_DEVICES     8
#elif defined(ST_8010)
#define NUM_PIO_DEVICES     7
#elif defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
#define NUM_PIO_DEVICES     4
#elif defined(ST_5525)
#define NUM_PIO_DEVICES     9
#else
#define NUM_PIO_DEVICES     5
#endif

/* Device name list */
static ST_DeviceName_t      DeviceName[] =
{
    "pio0",
    "pio1",
    "pio2",
    "pio3",
    "pio4",
    "pio5",     /* 14, 16, 17 only */
    "pio6",     /* 28,5525 only */
    "pio7",     /* 28 ,5525 only */
    "pio8"      /*5525 only*/
};

/* Init parameters */

static STPIO_InitParams_t   InitParams[NUM_PIO_DEVICES];

/* Semaphores */
static semaphore_t *CompareSemaphore_p;

/* Declarations for memory partitions */
#ifdef ST_OS20

/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif

#ifdef ARCHITECTURE_ST20
#define                 NCACHE_MEMORY_SIZE          0x80000
static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )
#endif

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( external_block, "system_section")
#endif


#ifdef ARCHITECTURE_ST20
/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")
#endif

#if defined(ST_7710) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_7100) \
|| defined(ST_5301) || defined(ST_7109)  || defined(ST_5188) || defined(ST_5525) || defined(ST_5107) \
|| defined(ST_7200)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#elif defined(ST_OS21)

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#else /*ST_OSLINUX*/
ST_Partition_t         *DriverPartition = (ST_Partition_t*)1;
ST_Partition_t         *system_partition = (ST_Partition_t*)1;
#endif

/*****************************************************************************
Function prototypes
*****************************************************************************/

/* Function prototypes */

static void GlobalInitialize(void);
const char *ConfigToString(STPIO_BitConfig_t Config);
void DisplayPioStatus(STPIO_Status_t *Status_p);

#if APITestNumIterations
static TestResult_t APITest(TestParams_t *Params_p);
#endif

#if SetClrTestNumIterations
static TestResult_t SetClrTest(TestParams_t *Params_p);
#endif

#if SpeedTestNumIterations
static TestResult_t SpeedTest(TestParams_t *Params_p);
#endif

#if MultipleUseTestNumIterations
static TestResult_t MultipleUseTest(TestParams_t *Params_p);
#endif

#ifdef ARCHITECTURE_ST20
static TestResult_t StackUsage(TestParams_t *Params_p);
#endif

#if CompareTestNumIterations
static TestResult_t CompareTest(TestParams_t *Params_p);
static void CompareNotifyFunction(STPIO_Handle_t Handle,
                                  STPIO_BitMask_t ActiveBits);
static void CompareClearNotifyFunction(STPIO_Handle_t Handle,
                                  STPIO_BitMask_t ActiveBits);
#endif

/*****************************************************************************
Test parameters
*****************************************************************************/

typedef struct
{
    U8      Device;
    U32     NumIterations;
    U8      BitMask;
} SpeedParams_t;

typedef struct
{
    U8      Device;
    U32     NumIterations;
    U8      OutputBitMask;
    U8      InputBitMask;
} SetClrParams_t;

typedef struct
{
    U8      InputDevice;
    U8      OutputDevice;
    U32     NumIterations;
    U8      OutputBitMask;
    U8      InputBitMask;
} MultipleParams_t;

typedef struct
{
    U8      Device;
    U8      BitMask;
} CompareParams_t;

/* Set/clr loop-through test number of iterations */
#ifndef SETCLR_NUM_ITERATIONS
#if defined(ST_OSLINUX)
#define SETCLR_NUM_ITERATIONS  100
#else
#define SETCLR_NUM_ITERATIONS  100000
#endif
#endif /*SETCLR_NUM_ITERATIONS*/

#ifndef SETCLR_OUT_BITMASK

#if (defined(ST_5508) || defined(ST_5518))
#define SETCLR_OUT_BITMASK     PIO_BIT_0

#elif defined(ST_TP3) || defined(ST_5700)
#define SETCLR_OUT_BITMASK     PIO_BIT_5

#elif defined(ST_GX1) || defined(ST_NGX1)
#define SETCLR_OUT_BITMASK     PIO_BIT_2

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_7710) || defined(ST_5105) ||\
defined(ST_7100) || defined(ST_7109) || defined(ST_5525)
#define SETCLR_OUT_BITMASK     PIO_BIT_7

#elif defined(ST_7200)
#define SETCLR_OUT_BITMASK     PIO_BIT_6

#elif defined(ST_8010) || defined(ST_5188) || defined(ST_5107)
#define SETCLR_OUT_BITMASK     PIO_BIT_3

#else /* ST_5514 || ST_5516 || ST_5517 || ST_5528 || ST_5100 || ST_5301 */
#define SETCLR_OUT_BITMASK     PIO_BIT_4

#endif /*(defined(ST_5508) | defined(ST_5518))*/
#endif /*SETCLR_OUT_BITMASK*/

#ifndef SETCLR_IN_BITMASK

#if (defined(ST_5508) || defined(ST_5518))
#define SETCLR_IN_BITMASK      PIO_BIT_3

#elif defined(ST_TP3)
#define SETCLR_IN_BITMASK      PIO_BIT_6

#elif defined(ST_GX1) || defined(ST_NGX1) || defined(ST_5188)
#define SETCLR_IN_BITMASK      PIO_BIT_0

#elif defined(ST_5510) || defined(ST_5512)
#define SETCLR_IN_BITMASK      PIO_BIT_6

#elif defined(ST_5100) || defined(ST_5301)
#define SETCLR_IN_BITMASK      PIO_BIT_5

#elif defined(ST_8010) || defined(ST_5107)
#define SETCLR_IN_BITMASK      PIO_BIT_2

#elif defined(ST_7710)
#define SETCLR_IN_BITMASK      PIO_BIT_5

#elif defined(ST_5525) || defined (ST_7200)
#define SETCLR_IN_BITMASK      PIO_BIT_4

#else
/* ST_5514 || ST_5516 || ST_5517 || ST_5528 || ST_5105 || ST_5700 || ST_7f10 || ST_7109 */
#define SETCLR_IN_BITMASK      PIO_BIT_3

#endif
#endif /*SETCLR_IN_BITMASK*/

#ifndef SETCLR_PIO

#if defined(ST_5508) || defined(ST_5518) || defined(ST_TP3) || defined(ST_5188)
#define SETCLR_PIO             PIO_DEVICE_3

#elif defined(ST_5700)
#define SETCLR_PIO             PIO_DEVICE_4

#elif defined(ST_GX1) || defined(ST_NGX1)
#define SETCLR_PIO             PIO_DEVICE_1

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_7710)
#define SETCLR_PIO             PIO_DEVICE_3

#elif defined(ST_5107)
#define SETCLR_PIO             PIO_DEVICE_2

#elif defined(ST_7200)
#define SETCLR_PIO             PIO_DEVICE_5

#else
/* ST_5514 || ST_5516 || ST_5517 || ST_5528 || ST_5100 || ST_5105 || ST_7100 || ST_5301 ||\
ST_8010 || ST_5525*/
#define SETCLR_PIO             PIO_DEVICE_0


#endif
#endif /*SETCLR_PIO*/

static SetClrParams_t SetClrParams =
{
    (U32)SETCLR_PIO,                /* Device to use */
    SETCLR_NUM_ITERATIONS,     /* Number of iterations */
    SETCLR_OUT_BITMASK,        /* PIO bit for output */
    SETCLR_IN_BITMASK          /* PIO bit for input */
};

/* Speed test number of set/clr calls in benchmark */
#ifndef SPEED_TEST_NUM_CALLS
#define SPEED_TEST_NUM_CALLS   2000000 /* Make 2M calls */
#endif /*SPEED_TEST_NUM_CALLS*/

#if SpeedTestNumIterations
static SpeedParams_t SpeedParams =
{
    (U32)SETCLR_PIO,
    SPEED_TEST_NUM_CALLS,
    SETCLR_OUT_BITMASK
};
#endif

/* Compare function test */
#ifndef COMPARE_TEST_PIO

#if (defined(ST_5508) || defined(ST_5518))
#define COMPARE_TEST_PIO             PIO_DEVICE_3

#elif defined(ST_TP3)
#define COMPARE_TEST_PIO             PIO_DEVICE_3

#elif defined(ST_GX1) || defined(ST_NGX1) || defined(ST_5107)
#define COMPARE_TEST_PIO             PIO_DEVICE_1

#elif defined(ST_5514)
#define COMPARE_TEST_PIO             PIO_DEVICE_5

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5525)
#define COMPARE_TEST_PIO             PIO_DEVICE_3

#elif defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5301) \
|| defined(ST_7100) || defined(ST_8010) || defined(ST_7109) || defined(ST_5188)    \
|| defined(ST_7200)
#define COMPARE_TEST_PIO             PIO_DEVICE_2

#else
/* ST_5516 || ST_5517 || ST_5528  */
#define COMPARE_TEST_PIO             PIO_DEVICE_4

#endif
#endif /*COMPARE_TEST_PIO*/

#ifndef COMPARE_TEST_BITMASK

#if (defined(ST_5508) || defined(ST_5518))
#define COMPARE_TEST_BITMASK    PIO_BIT_6

#elif defined(ST_TP3)
#define COMPARE_TEST_BITMASK    PIO_BIT_7

#elif defined(ST_GX1) || defined(ST_NGX1) || defined(ST_7710) || defined(ST_5700)  || \
defined(ST_8010)  || defined(ST_5107)

#define COMPARE_TEST_BITMASK    PIO_BIT_2

#elif defined(ST_5514) || defined(ST_5100) || defined(ST_5301)
#define COMPARE_TEST_BITMASK    PIO_BIT_4

#elif defined(ST_5105) || defined(ST_5188)
#define COMPARE_TEST_BITMASK    PIO_BIT_1

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_5525)
#define COMPARE_TEST_BITMASK    PIO_BIT_7

#else
/* ST_5516 || ST_5517 || ST_5528 || ST_7100 || ST_7109 */
#define COMPARE_TEST_BITMASK    PIO_BIT_3

#endif
#endif /*COMPARE_TEST_BITMASK*/


static CompareParams_t CompareParams =
{
    (U32)COMPARE_TEST_PIO,
    COMPARE_TEST_BITMASK
};

/* Multiple pio test params */

/* STi5512 LSA11:4-5 LSA11:7-13
 * STi5510 LSA7:4-5 LSA7:5-13
 * STi5508 LSA1:11-8 LSA1:8-12
 * STi5518 LSA1:11-8 LSA1:8-12
 * ST_TP3  LSA2: 5-6  LSA2: 11-12
 * GX1     CN38: 15-17 CN34: 7-8
 * ST_5514 LSA6: 15-16 LSA6: 11-12
 */
#ifndef MULTIPLE_IN_PIO

#if defined(ST_5508) || defined(ST_5518) || defined(ST_5700) || defined(ST_8010) || defined(ST_5188)\
|| defined(ST_5525)  || defined(ST_5107)
#define MULTIPLE_IN_PIO        PIO_DEVICE_2

#elif defined(ST_TP3)
#define MULTIPLE_IN_PIO        PIO_DEVICE_1

#elif defined(ST_GX1) || defined(ST_NGX1)
#define MULTIPLE_IN_PIO        PIO_DEVICE_1

#elif defined(ST_5510) || defined(ST_5512)
#define MULTIPLE_IN_PIO        PIO_DEVICE_1

#elif defined(ST_7710)
#define MULTIPLE_IN_PIO        PIO_DEVICE_5

#elif defined(ST_5105) || defined(ST_7100) || defined(ST_7109)
#define MULTIPLE_IN_PIO        PIO_DEVICE_0

#elif defined(ST_7200)
#define MULTIPLE_IN_PIO        PIO_DEVICE_4

#else
/* ST_5514 || ST_5516 || ST_5517 || ST_5528 || ST_5100 || ST_5301 */
#define MULTIPLE_IN_PIO        PIO_DEVICE_0

#endif
#endif/*MULTIPLE_IN_PIO*/

#ifndef MULTIPLE_OUT_PIO

#if (defined(ST_5508) || defined(ST_5518))
#define MULTIPLE_OUT_PIO        PIO_DEVICE_3

#elif defined(ST_TP3)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_3

#elif defined(ST_GX1) || defined(ST_NGX1)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_1

#elif defined(ST_5510) || defined(ST_5512)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_3

#elif defined(ST_5100) || defined(ST_5301) || defined(ST_7100) || defined(ST_7109) || defined(ST_5188)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_1

#elif defined(ST_7200)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_5

#elif defined(ST_5105)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_2

#elif defined(ST_7710) || defined(ST_5700) || defined(ST_8010) || defined(ST_5525)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_4

#elif defined(ST_5528)
/* PIo2 not suitable on this chip (different LSA to PIO0) */
#define MULTIPLE_OUT_PIO        PIO_DEVICE_1

#elif defined(ST_5107)
#define MULTIPLE_OUT_PIO        PIO_DEVICE_0

#else
/* ST_5514 || ST_5516 || ST_5517 || ST_5105 */
#define MULTIPLE_OUT_PIO        PIO_DEVICE_2


#endif
#endif /*MULTIPLE_OUT_PIO*/

#ifndef MULTIPLE_ITERATIONS
#if defined(ST_OSLINUX)
#define MULTIPLE_ITERATIONS    10
#else
#define MULTIPLE_ITERATIONS    100000
#endif
#endif

#ifndef MULTIPLE_OUT_BITMASK

#if (defined(ST_5508) || defined(ST_5518))
#define MULTIPLE_OUT_BITMASK        PIO_BIT_2

#elif defined(ST_TP3)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_0

#elif defined(ST_GX1) || defined(ST_NGX1)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_2

#elif defined(ST_5510) || defined(ST_5512)  || defined(ST_5700) || defined(ST_8010)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_4

#elif defined(ST_5100) || defined(ST_5301)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_2

#elif defined(ST_5105)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_1

#elif defined(ST_7710)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_5

#elif defined(ST_5188)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_6

#elif defined(ST_5525)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_3

#elif defined(ST_5107)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_7

#elif defined(ST_7200)
#define MULTIPLE_OUT_BITMASK        PIO_BIT_5

#else
/* ST_5514 || ST_5516 || ST_5517 || ST_5528 || ST_7100 || ST_7109 */
#define MULTIPLE_OUT_BITMASK        PIO_BIT_0


#endif
#endif /*MULTIPLE_OUT_BITMASK*/

#ifndef MULTIPLE_IN_BITMASK

#if (defined(ST_5508) || defined(ST_5518))
#define MULTIPLE_IN_BITMASK        PIO_BIT_7

#elif defined(ST_TP3)
#define MULTIPLE_IN_BITMASK        PIO_BIT_7

#elif defined(ST_GX1) || defined(ST_NGX1) || defined(ST_5700) || defined(ST_8010) || defined(ST_5525)
#define MULTIPLE_IN_BITMASK        PIO_BIT_0

#elif defined(ST_5510) || defined(ST_5512)
#define MULTIPLE_IN_BITMASK        PIO_BIT_6

#elif defined(ST_5100) || defined(ST_5301)
#define MULTIPLE_IN_BITMASK        PIO_BIT_3

#elif defined(ST_7710) || defined(ST_5105) || defined(ST_7100) || defined(ST_7109)
#define MULTIPLE_IN_BITMASK        PIO_BIT_4

#elif defined(ST_7200)
#define MULTIPLE_IN_BITMASK        PIO_BIT_7

#elif defined(ST_5188)
#define MULTIPLE_IN_BITMASK        PIO_BIT_6

#elif defined(ST_5107)
#define MULTIPLE_IN_BITMASK        PIO_BIT_5

#else
/* ST_5514 || ST_5516 || ST_5517 || ST_5528  */
#define MULTIPLE_IN_BITMASK        PIO_BIT_7

#endif
#endif /*MULTIPLE_IN_BITMASK*/

#if MultipleUseTestNumIterations
static MultipleParams_t MultipleParams =
{
    (U32)MULTIPLE_IN_PIO,
    (U32)MULTIPLE_OUT_PIO,
    MULTIPLE_ITERATIONS,
    MULTIPLE_OUT_BITMASK,
    MULTIPLE_IN_BITMASK
};
#endif

#if defined(ARCHITECTURE_ST40)

#undef PIO_0_INTERRUPT_LEVEL
#undef PIO_1_INTERRUPT_LEVEL
#undef PIO_2_INTERRUPT_LEVEL
#undef PIO_3_INTERRUPT_LEVEL
#undef PIO_4_INTERRUPT_LEVEL

#define PIO_0_INTERRUPT_LEVEL    0
#define PIO_1_INTERRUPT_LEVEL    0
#define PIO_2_INTERRUPT_LEVEL    0
#define PIO_3_INTERRUPT_LEVEL    0
#define PIO_4_INTERRUPT_LEVEL    0

#endif

#if !defined(PIO_8_INTERRUPT_LEVEL)   /* to be added in include for 5525*/
#define PIO_8_INTERRUPT_LEVEL  1
#endif

/*****************************************************************************
Table of test functions to execute
*****************************************************************************/


static TestEntry_t TestTable[] =
{
#if !defined(ST_OSLINUX	)
#ifdef ARCHITECTURE_ST20
    {
        StackUsage,
        "Report on stack usage for API calls",
        1,
        NULL
    },
#endif
#endif/*ST_OSLINUX*/
#if APITestNumIterations
    {
        APITest,                        /* Function */
        "Test the API with invalid parameters", /* Textual description */
        APITestNumIterations,           /* Number of iterations */
        NULL
    },
#endif

#if SetClrTestNumIterations
#if !defined(mb426)
    {
        SetClrTest,                     /* Function */
        "Set & clear pio bits",         /* Textual description */
        SetClrTestNumIterations,        /* Number of iterations */
        &SetClrParams
    },
#endif
#endif

#if !defined(ST_OSLINUX)
#if SpeedTestNumIterations
    {
        SpeedTest,                      /* Function */
        "Benchmark set and clear functions", /* Textual description */
        SpeedTestNumIterations,         /* Number of iterations */
        &SpeedParams
    },
#endif
#endif /*ST_OSLINUX*/

#if MultipleUseTestNumIterations
#if !defined(mb426)
    {
        MultipleUseTest,                  /* Function */
        "Test with multiple pio devices at once", /* Textual description */
        MultipleUseTestNumIterations,   /* Number of iterations */
        &MultipleParams
    },
#endif
#endif

#if !defined(ST_OSLINUX)
#if CompareTestNumIterations
    {
        CompareTest,                    /* Function */
        "Test notification of a compare function", /* Textual description */
        CompareTestNumIterations,       /* Number of iterations */
        &CompareParams                  /* Set compare params */
    },
#endif
#endif

    { 0, "", 0 }                        /* End of table marker */

};

/*****************************************************************************
Main
*****************************************************************************/

void os20_main(void *ptr)
{
    TestResult_t Total = TEST_RESULT_ZERO, Result = TEST_RESULT_ZERO;
    TestEntry_t *Test_p = NULL;
    U32 Section;
    TestParams_t TestParams;

#if !defined(ST_OSLINUX)
#ifdef STTBX_PRINT
    STTBX_InitParams_t TBXInitParams;
#endif
    STBOOT_InitParams_t BootParams;
    STBOOT_TermParams_t BootTerm;
    U32 error;
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
#elif defined(ST_5514) || defined(ST_5516) /*|| defined(ST_5517)*/
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40020000, (U32 *)0x407FFFFF },
        { NULL, NULL }                  /* End */
    };
#elif defined(ST_5107)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x30001000, (U32 *)0x30001FFF},
        { NULL, NULL }                  /* End */
    };
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF },
        { NULL, NULL }                  /* End */
    };
#endif

#ifdef CODETEST
    /* Codetest initialization */
    codetest_init();
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));

#else /* ifndef ST_OS21 */


    /* Create memory partitions */
    partition_init_heap(&the_internal_partition,
                        (U8*)internal_block,
                        sizeof(internal_block)
                       );
    partition_init_heap(&the_system_partition,
                        (U8*)external_block,
                        sizeof(external_block));

#ifdef ARCHITECTURE_ST20
    partition_init_heap(&the_ncache_partition,
                        ncache_memory,
                        sizeof(ncache_memory));

    /* Avoids a compiler warning */
    internal_block_noinit[0]= 0;
    system_block_noinit[0]  = 0;

#if defined(ST_7710) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) ||\
defined(ST_5301)|| defined(ST_5188) || defined(ST_5525) || defined(ST_5107)
    data_section[0]         = 0;
#endif

#endif

#endif /* ST_OS21 */

    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
#if defined(DISABLE_ICACHE)
    BootParams.ICacheEnabled = FALSE;
#else
    BootParams.ICacheEnabled = TRUE;
#endif
    BootParams.DCacheMap = DCacheMap;
    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.MemorySize = (STBOOT_DramMemorySize_t)SDRAM_SIZE;

    error = STBOOT_Init( "boot", &BootParams );
    if (error != ST_NO_ERROR)
    {
        printf("Unable to initialize boot driver - %d\n", error);
        return;
    }

    /* Initialize the toolbox */
#ifdef STTBX_PRINT
    TBXInitParams.SupportedDevices = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p = system_partition;
    (void)STTBX_Init( "tbx", &TBXInitParams );
#endif
#endif /*ST_OSLINUX*/
    /* Initialize global data */
    GlobalInitialize();

    DebugMessage("**************************************************");
    DebugMessage("                 STPIO Test Harness               ");
    DebugPrintf(("Driver Revision: %s\n", STPIO_GetRevision()));
    DebugPrintf(("Test Harness Revision: %s\n", Revision));

    Section = 1;
    Test_p = TestTable;
    while (Test_p->TestFunction != NULL)
    {
        U32 Count;
        Count = Test_p->RepeatCount;

        while (Count > 0)
        {
            DebugMessage("**************************************************");
            DebugPrintf(("SECTION %d - %s\n", Section,
                              Test_p->TestInfo));
            DebugMessage("**************************************************");

            /* Set up parameters for test function */
            TestParams.Ref = Section;
            TestParams.Params_p = Test_p->TestParams_p;

            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            /* Update counters */
            Total.NumberPassed += Result.NumberPassed;
            Total.NumberFailed += Result.NumberFailed;
            Count--;
        }

        Test_p++;
        Section++;
    }

    /* Output running analysis */
    DebugMessage("**************************************************");
    DebugPrintf(("PASSED: %d\n", Total.NumberPassed));
    DebugPrintf(("FAILED: %d\n", Total.NumberFailed));
    DebugMessage("**************************************************");

#if !defined(ST_OSLINUX)
    (void)STBOOT_Term( "boot", &BootTerm );
#endif

}

int main(int argc, char *argv[])
{
#if defined(ARCHITECTURE_ST40) && !defined(ST_OS21) && !defined(ST_OSLINUX)
    setbuf(stdout, NULL);
    OS20_main(argc, argv, os20_main);
#else
    os20_main(NULL);
#endif
}

/*****************************************************************************
Initialize global variables
*****************************************************************************/

/* Initialize global data structures, semaphores, etc */
static void GlobalInitialize(void)
{
    /* PIO 0 */
    InitParams[0].BaseAddress     = (U32*)PIO_0_BASE_ADDRESS;
    InitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;
    InitParams[0].InterruptNumber = PIO_0_INTERRUPT;
    InitParams[0].DriverPartition = system_partition;

    /* PIO 1 */
    InitParams[1].BaseAddress     = (U32*)PIO_1_BASE_ADDRESS;
    InitParams[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;
    InitParams[1].InterruptNumber = PIO_1_INTERRUPT;
    InitParams[1].DriverPartition = system_partition;

    /* PIO 2 */
    InitParams[2].BaseAddress     = (U32*)PIO_2_BASE_ADDRESS;
    InitParams[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;
    InitParams[2].InterruptNumber = PIO_2_INTERRUPT;
    InitParams[2].DriverPartition = system_partition;

    /* PIO 3 */
    InitParams[3].BaseAddress     = (U32*)PIO_3_BASE_ADDRESS;
    InitParams[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;
    InitParams[3].InterruptNumber = PIO_3_INTERRUPT;
    InitParams[3].DriverPartition = system_partition;

#if (NUM_PIO_DEVICES > 4)
    /* PIO 4 */
    InitParams[4].BaseAddress     = (U32*)PIO_4_BASE_ADDRESS;
    InitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
    InitParams[4].InterruptNumber = PIO_4_INTERRUPT;
    InitParams[4].DriverPartition = system_partition;
#endif

#if (NUM_PIO_DEVICES > 5)
    /* PIO 5 */
    InitParams[5].BaseAddress     = (U32*)PIO_5_BASE_ADDRESS;
    InitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
    InitParams[5].InterruptNumber = PIO_5_INTERRUPT;
    InitParams[5].DriverPartition = system_partition;
#endif

#if (NUM_PIO_DEVICES > 6)
    /* PIO 6 */
    InitParams[6].BaseAddress     = (U32*)PIO_6_BASE_ADDRESS;
    InitParams[6].InterruptLevel  = PIO_6_INTERRUPT_LEVEL;
    InitParams[6].InterruptNumber = PIO_6_INTERRUPT;
    InitParams[6].DriverPartition = system_partition;
#endif

    /* This is getting progressively uglier. */
#if (NUM_PIO_DEVICES > 7)
    /* PIO 7 */
    InitParams[7].BaseAddress     = (U32*)PIO_7_BASE_ADDRESS;
    InitParams[7].InterruptLevel  = PIO_7_INTERRUPT_LEVEL;
    InitParams[7].InterruptNumber = PIO_7_INTERRUPT;
    InitParams[7].DriverPartition = system_partition;
#endif

    /* This is getting progressively uglier. */
#if (NUM_PIO_DEVICES > 8)
    /* PIO 8 */
    InitParams[8].BaseAddress     = (U32*)PIO_8_BASE_ADDRESS;
    InitParams[8].InterruptLevel  = PIO_8_INTERRUPT_LEVEL;
    InitParams[8].InterruptNumber = PIO_8_INTERRUPT;
    InitParams[8].DriverPartition = system_partition;
#endif

}

/*****************************************************************************
API robustness
*****************************************************************************/

#if APITestNumIterations
static TestResult_t APITest(TestParams_t *TestParams_p)
{
    ST_ErrorCode_t          error = ST_NO_ERROR;
    STPIO_Handle_t          HandlePIO0;
    STPIO_Handle_t          InvalidHandle = 0;
    STPIO_OpenParams_t      OpenParams;
    STPIO_TermParams_t      TermParams;
    STPIO_BitMask_t         bitMask = 0;
    STPIO_Compare_t         compareStatus;
    STPIO_CompareClear_t    compareClearStatus;
    STPIO_BitConfig_t       bitConfig[8];
    STPIO_Status_t          Status;
    U8                      writeBuffer = 0;
    U8                      readBuffer;
    TestResult_t            Result = TEST_RESULT_ZERO;


    DebugMessage(("Invalid handle checking...."));

    /***************************************************
    Read with no Init
    ****************************************************/
    error = STPIO_Read (InvalidHandle, &readBuffer);
    DebugError("STPIO_Read()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    Write Tests - PIO0
    *****************************************/
    error = STPIO_Write(InvalidHandle, writeBuffer);
    DebugError("STPIO_Write()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    Set Tests - PIO0
    *****************************************/
    error = STPIO_Set(InvalidHandle, bitMask);
    DebugError("STPIO_Set()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    Clear Tests - PIO0
    *****************************************/
    error = STPIO_Clear(InvalidHandle, bitMask);
    DebugError("STPIO_Clear()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    GetCompare Tests - PIO0
    *****************************************/
#if !defined(ST_OSLINUX)
    error = STPIO_GetCompare(InvalidHandle, &compareStatus);
    DebugError("STPIO_GetCompare()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }
#endif/*ST_OSLINUX*/

    /*****************************************
    SetCompare Tests - PIO0
    *****************************************/
    error = STPIO_SetCompare(InvalidHandle, &compareStatus);
    DebugError("STPIO_SetCompare()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    GetCompareClear Tests - PIO0
    *****************************************/
#if !defined(ST_OSLINUX)
    error = STPIO_GetCompareClear(InvalidHandle, &compareClearStatus);
    DebugError("STPIO_GetCompareClear()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }
#endif /*ST_OSLINUX*/
    /*****************************************
    SetCompareClear Tests - PIO0
    *****************************************/
    error = STPIO_SetCompareClear(InvalidHandle, compareClearStatus);
    DebugError("STPIO_SetCompareClear()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    SetConfig Tests - PIO0
    *****************************************/
    error = STPIO_SetConfig(InvalidHandle, bitConfig);
    DebugError("STPIO_SetConfig()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    GetStatus Tests - PIO0
    *****************************************/
#if !defined(ST_OSLINUX)
    error = STPIO_GetStatus(InvalidHandle, &Status);
    DebugError("STPIO_GetStatus()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }
#endif/*ST_OSLINUX*/
    /*****************************************
    Close Tests - PIO0
    *****************************************/
    error = STPIO_Close(InvalidHandle);
    DebugError("STPIO_Close()", error);
    if (error == ST_ERROR_INVALID_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_INVALID_HANDLE");
    }

    /*****************************************
    Init Tests - PIO0
    *****************************************/
    DebugMessage(("Initialize a device twice...."));

    /* Test for a repeated init error */
    error = STPIO_Init(DeviceName[PIO_DEVICE_0], &InitParams[PIO_DEVICE_0]);
    DebugError("STPIO_Init()", error);

    if (error == ST_NO_ERROR)
    {
        error = STPIO_Init(DeviceName[PIO_DEVICE_0], &InitParams[PIO_DEVICE_0]);
        DebugError("STPIO_Init()", error);
        if (error == ST_ERROR_ALREADY_INITIALIZED)
        {
            TestPassed(Result);
        }
        else
        {
            TestFailed(Result, "Expected ST_ERROR_ALREADY_INITIALIZED");
        }
    }
    else
    {
        TestFailed(Result, "Unable to initialize first instance");
    }

    /*****************************************
    Open Tests - PIO0
    *****************************************/
    /* try to open a device which does not exist */

    DebugMessage(("Open with invalid device name...."));

    OpenParams.ReservedBits    = PIO_BIT_0 | PIO_BIT_1 | PIO_BIT_2;

    /* reserve three bits and give it a NULL IRQ handler */
    OpenParams.BitConfigure[0] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[1] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[2] = STPIO_BIT_OUTPUT;
    OpenParams.IntHandler      = NULL;

    /* Make the call and then print the error value */
    error = STPIO_Open("invalid", &OpenParams, &HandlePIO0);
    DebugError("STPIO_Open()", error);

    if (error == ST_ERROR_UNKNOWN_DEVICE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_UNKNOWN_DEVICE");
    }

    /* now try to open a device already in use */
    DebugMessage(("Attempt open whilst busy...."));

    /* Make the call and then print the error value */
    error = STPIO_Open(DeviceName[PIO_DEVICE_0], &OpenParams, &HandlePIO0);
    DebugError("STPIO_Open()", error);

    if (error == ST_NO_ERROR)
    {
        /* Make the call and then print the error value */
        error = STPIO_Open(DeviceName[PIO_DEVICE_0], &OpenParams, &HandlePIO0);
        DebugError("STPIO_Open()", error);
        if (error == ST_ERROR_DEVICE_BUSY)
        {
            TestPassed(Result);
        }
        else
        {
            TestFailed(Result, "Expected ST_ERROR_DEVICE_BUSY");
        }
    }
    else
    {
        TestFailed(Result, "Unable to open first handle");
    }

    /*****************************************
    Term Tests - PIO0
    *****************************************/

    DebugMessage(("Terminate with invalid device name...."));

    TermParams.ForceTerminate = FALSE;
    error = STPIO_Term("invalid", &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_ERROR_UNKNOWN_DEVICE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_UNKNOWN_DEVICE");
    }

    DebugMessage(("Terminate with open handles (not forced)...."));

    error = STPIO_Term(DeviceName[PIO_DEVICE_0], &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_ERROR_OPEN_HANDLE)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_ERROR_OPEN_HANDLE");
    }

    DebugMessage(("Terminate with open handles (forced)...."));

    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(DeviceName[PIO_DEVICE_0], &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    return Result;
}
#endif /* APITestNumIterations */

/*****************************************************************************
Set/Clr feedback test
*****************************************************************************/

#if SetClrTestNumIterations
static TestResult_t SetClrTest(TestParams_t *TestParams_p)
{
    ST_ErrorCode_t     error = ST_NO_ERROR;
    STPIO_Handle_t     InputHandle;
    STPIO_Handle_t     OutputHandle;
    STPIO_OpenParams_t OpenParams;
    STPIO_CompareClear_t    CompareClearStatus;
    U32                i;
    U32 Count;
    SetClrParams_t *Params_p = TestParams_p->Params_p;
    TestResult_t Result = TEST_RESULT_ZERO;
    U8 Device;
    U8 InputBitMask, OutputBitMask;
    STPIO_TermParams_t TermParams;

    /* Set test params */
    Count = Params_p->NumIterations;
    OutputBitMask = Params_p->OutputBitMask;
    InputBitMask = Params_p->InputBitMask;
    Device = Params_p->Device;


    /*****************************************
    Init Tests
    *****************************************/

    /* Set up the Init params to use the required device */
    /* Make the call and then print the error value */
    error = STPIO_Init(DeviceName[Device],
                       &InitParams[Device]);
    DebugError("STPIO_Init()", error);

    if ((error == ST_NO_ERROR) || (error == ST_ERROR_ALREADY_INITIALIZED))
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to initialize device");
    }

    /*****************************************
    Open Tests
    *****************************************/

    /* setup pio bit required */
    OpenParams.ReservedBits     = OutputBitMask;
    OpenParams.BitConfigure[0]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[1]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[2]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[3]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[4]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[5]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[6]  = STPIO_BIT_OUTPUT;
#if defined(ST_5107)
    if(DeviceName[Device]!="pio3")
       OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
    else
       OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
       OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
#endif

    OpenParams.IntHandler       = NULL;
    error = STPIO_Open(DeviceName[Device], &OpenParams, &OutputHandle);
    DebugError("STPIO_Open()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to open output the device");
    }

    OpenParams.ReservedBits     = InputBitMask;
    OpenParams.BitConfigure[0]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[1]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[2]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[3]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[4]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[5]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[6]  = STPIO_BIT_INPUT;
#if defined(ST_5107)
    /*PIO3- 7-Bit bank*/
    if(DeviceName[Device]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_INPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_INPUT;
#endif

    OpenParams.IntHandler       = NULL;
    error = STPIO_Open(DeviceName[Device], &OpenParams, &InputHandle);
    DebugError("STPIO_Open()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to open the input device");
    }

    /*****************************************
    Set / Clear Tests
    *****************************************/

    DebugPrintf(("Performing set, read, clear, read cycle...\n"));
    for (i=0; i < Count; i++)
    {
        U8 InputPin;

        error = STPIO_Clear(OutputHandle, OutputBitMask);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Clear()", error);
            break;
        }

        /* Read the input bit */
        error = STPIO_Read(InputHandle, &InputPin);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Read()", error);
            break;
        }

        /* Ensure input pin state is zero */
        if ((InputPin & InputBitMask) != 0)
        {
            DebugPrintf(("Expected input pin to be clear\n"));
            break;
        }

        error = STPIO_Set(OutputHandle, OutputBitMask);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Set()", error);
            break;
        }
        /* Read the input bit */
        error = STPIO_Read(InputHandle,&InputPin);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Read()", error);
            break;
        }

        /* Ensure input pin state is high */
        if ((InputPin & InputBitMask) == 0)
        {
            DebugPrintf(("Expected input pin to be high\n"));
            break;
        }
    }

    if (i == Count)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Failed to set or clear pin during test");
    }
  /* Check that setcompareclear actually changes things; should start off
     * as auto.
     */
#if !defined(ST_OSLINUX)
    DebugPrintf(("Checking SetCompareClear and SetCompareClear\n"));
    error = STPIO_GetCompareClear(InputHandle, &CompareClearStatus);
    if (error != ST_NO_ERROR)
    {
          DebugError("STPIO_GetCompareClear()", error);

    }
    else
    {
        if (CompareClearStatus != STPIO_COMPARE_CLEAR_AUTO)
        {
            DebugPrintf(("Wrong initial default! Wasn't auto clear\n"));
        }

        error = STPIO_SetCompareClear(InputHandle, STPIO_COMPARE_CLEAR_MANUAL);
        if (error == ST_NO_ERROR)
        {
            error = STPIO_GetCompareClear(InputHandle, &CompareClearStatus);
            if (error != ST_NO_ERROR)
            {
                DebugError("STPIO_GetCompareClear()", error);
            }
            else
            {
                if (CompareClearStatus != STPIO_COMPARE_CLEAR_MANUAL)
                {
                    TestFailed(Result,
                               "Didn't set CompareClear value correctly!");
                }
                else
                {
                    TestPassed(Result);
                }
            }

            error = STPIO_SetCompareClear(InputHandle, STPIO_COMPARE_CLEAR_AUTO);
        }
        else
        {
            DebugError("STPIO_SetCompareClear()", error);
        }
    }
#endif /*ST_OSLINUX*/
    /*****************************************
    Close Tests
    *****************************************/

    /* Don't close and use a use Force Terminate mode */
    TermParams.ForceTerminate = TRUE;

    error = STPIO_Term(DeviceName[Device], &TermParams);
    DebugError("STPIO_Term()", error);

    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    return Result;
}
#endif

/*****************************************************************************
Speed test for benchmarking driver performance
*****************************************************************************/

#if SpeedTestNumIterations
static TestResult_t SpeedTest(TestParams_t *TestParams_p)
{
    ST_ErrorCode_t     error = ST_NO_ERROR;
    STPIO_Handle_t     HandleST;
    STPIO_OpenParams_t OpenParams;
    U32       i;
    SpeedParams_t *Params_p = TestParams_p->Params_p;
    TestResult_t Result = TEST_RESULT_ZERO;
    U8 SetClrBitMask;
    U8 Device;
    U8 writeBufferHigh;
    U8 writeBufferLow;
#if defined(ARCHITECTURE_ST200)
    U32 StartTime;
    U32 EndTime;
    U32 WriteResult;
    U32 QuickResult;
    U32 SetClearResult;
#else
    osclock_t StartTime;
    osclock_t EndTime;
    osclock_t WriteResult;
    osclock_t QuickResult;
    osclock_t SetClearResult;
#endif
    U32 counter;
    STPIO_TermParams_t TermParams;
#if defined(ARCHITECTURE_ST20) && !defined(PROCESSOR_C1)
    #pragma ST_device(PioReg)
#endif
    volatile U32 *PioReg;

    /* Set test params */
    counter = Params_p->NumIterations;
    SetClrBitMask = Params_p->BitMask;
    Device = Params_p->Device;

    /*****************************************
    Init Tests
    *****************************************/
    /* set up the Init params to use the required device */

    /* Make the call and then print the error value */
    error = STPIO_Init(DeviceName[Device],
                       &InitParams[Device]);
    DebugError("STPIO_Init()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to initialize pio device");
    }

    /*****************************************
    Open Tests
    *****************************************/

    DebugMessage("Opening pio port pin for benchmarking....");

    /* setup required bit */
    OpenParams.ReservedBits    = SetClrBitMask;
#if defined(ST_5107)
     /*PIO3- 7-Bit bank*/
    if(DeviceName[Device]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
#endif

    OpenParams.IntHandler      = NULL;
    /* Make the call and then print the error value */
    error = STPIO_Open(DeviceName[Device], &OpenParams, &HandleST);
    DebugError("STPIO_Open()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to open pio device");
    }

    /*****************************************
    Set / Clear Tests
    *****************************************/

    DebugMessage("Benchmarking set/clear....");

    StartTime = time_now();
    for (i=0; i<counter; i++)
    {
        error = STPIO_Clear(HandleST, SetClrBitMask);
        error = STPIO_Set(HandleST, SetClrBitMask);
    }
    EndTime = time_now();
    SetClearResult = time_minus(EndTime, StartTime);

    DebugMessage("...done");

    /*****************************************
    Write Tests
    *****************************************/

    DebugMessage("Benchmarking write....");

    writeBufferHigh = SetClrBitMask;
    writeBufferLow  = 0;

    StartTime = time_now();
    for (i=0; i<counter; i++)
    {
        error = STPIO_Write(HandleST, writeBufferHigh);
        error = STPIO_Write(HandleST, writeBufferLow);
    }
    EndTime = time_now();
    WriteResult = time_minus(EndTime, StartTime);

    DebugMessage("...done");

    /*****************************************
    Quickest Possible Tests
    *****************************************/

    DebugMessage("Benchmarking direct device access (not using stpio)....");

    /* Set the base address to the device pointer */
    PioReg = (volatile U32 *)InitParams[Device].BaseAddress;

    StartTime = time_now();
    for (i=0; i<counter; i++)
    {
        PioReg[PIO_OUT_SET] = SetClrBitMask;
        PioReg[PIO_OUT_CLR] = SetClrBitMask;
    }
    EndTime = time_now();
    QuickResult = time_minus(EndTime, StartTime);
    DebugMessage("...done");

    DebugPrintf(("Set/Clear tests time = %d ticks.\n", SetClearResult));
    DebugPrintf(("Write tests time = %d ticks.\n", WriteResult));
    DebugPrintf(("Direct tests time = %d ticks.\n", QuickResult));
    DebugPrintf(("Speedup = x %d if using direct access.\n",
                (SetClearResult / QuickResult)));

    /*****************************************
    Term Tests
    *****************************************/

    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(DeviceName[Device], &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    return Result;
}
#endif

/*****************************************************************************
Multiple usage test
*****************************************************************************/
#if MultipleUseTestNumIterations
static TestResult_t MultipleUseTest(TestParams_t *TestParams_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    STPIO_Handle_t InputHandle;
    STPIO_Handle_t OutputHandle;
    STPIO_OpenParams_t OpenParams;
    STPIO_TermParams_t TermParams;
    MultipleParams_t *Params_p = TestParams_p->Params_p;
    U8 InputDevice, OutputDevice, InputBitMask, OutputBitMask;
    TestResult_t Result = TEST_RESULT_ZERO;
    U32 Count, i;

    /* Extract test parameters */
    InputDevice = Params_p->InputDevice;
    OutputDevice = Params_p->OutputDevice;
    InputBitMask = Params_p->InputBitMask;
    OutputBitMask = Params_p->OutputBitMask;
    Count = Params_p->NumIterations;

    /*****************************************
    Init Tests
    *****************************************/

    DebugMessage("Initializing input and output pio devices....");

    /* Make the call and then print the error value */
    error = STPIO_Init(DeviceName[InputDevice], &InitParams[InputDevice]);
    DebugError("STPIO_Init()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to initialize input pio device");
    }

    error = STPIO_Init(DeviceName[OutputDevice], &InitParams[OutputDevice]);
    DebugError("STPIO_Init()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to initialize output pio device");
    }

    /*****************************************
    Open Tests
    *****************************************/

    DebugMessage("Opening input and output pio devices....");

    /* Configure input open params */
    OpenParams.ReservedBits = InputBitMask;
    OpenParams.BitConfigure[0] = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[1] = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[2] = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[3] = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[4] = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[5] = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[6] = STPIO_BIT_INPUT;
#if defined(ST_5107)
    if(DeviceName[InputDevice]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_INPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_INPUT;
#endif
    OpenParams.IntHandler = NULL;

    /* Make the call and then print the error value */
    error = STPIO_Open(DeviceName[InputDevice], &OpenParams, &InputHandle);
    DebugError("STPIO_Open()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to open input pio for use");
    }

    /* Configure output open params */
    OpenParams.ReservedBits    = OutputBitMask;
    OpenParams.BitConfigure[0] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[1] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[2] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[3] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[4] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[5] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[6] = STPIO_BIT_OUTPUT;
#if defined(ST_5107)
    if(DeviceName[OutputDevice]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
#endif
    OpenParams.IntHandler      = NULL;
    error = STPIO_Open(DeviceName[OutputDevice], &OpenParams, &OutputHandle);
    DebugError("STPIO_Open()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to open output pio for use");
    }

    /*****************************************
    Set / clear on separate pio ports
    *****************************************/

    DebugPrintf(("Performing set, read, clear, read cycle...\n"));
    for (i = 0; i < Count; i++)
    {
        U8 InputPin;

        error = STPIO_Clear(OutputHandle, OutputBitMask);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Clear()", error);
            break;
        }
        /* Read the input bit */
        error = STPIO_Read(InputHandle, &InputPin);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Read()", error);
            break;
        }
        /* Ensure input pin state is zero */
        if ((InputPin & InputBitMask) != 0)
        {
            DebugPrintf(("Expected input pin to be clear\n"));
            break;
        }
        error = STPIO_Set(OutputHandle, OutputBitMask);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Set()", error);
            break;
        }

        /* Read the input bit */
        error = STPIO_Read(InputHandle, &InputPin);
        if (error != ST_NO_ERROR)
        {
            DebugError("STPIO_Read()", error);
            break;
        }

        /* Ensure input pin state is high */
        if ((InputPin & InputBitMask) == 0)
        {
            DebugPrintf(("Expected input pin to be high\n"));
            break;
        }
    }

    if (i == Count)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Failed to set or clear pin during test");
    }

    /*****************************************
    Close Tests
    *****************************************/

    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(DeviceName[InputDevice], &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(DeviceName[OutputDevice], &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    return Result;
}
#endif

#if CompareTestNumIterations
static TestResult_t CompareTest(TestParams_t *TestParams_p)
{
    ST_ErrorCode_t     error = ST_NO_ERROR;
    STPIO_Handle_t     HandleIRQ;
    STPIO_OpenParams_t OpenParams;
    STPIO_Compare_t    setCmpReg,getCmpReg;
    STPIO_Status_t     status;
    TestResult_t Result = TEST_RESULT_ZERO;
    U8 BitMask;
    U8 Device;
    STPIO_TermParams_t TermParams;
    osclock_t clk;
    CompareParams_t *Params_p = TestParams_p->Params_p;

    /* Set test params */
    BitMask = Params_p->BitMask;
    Device = Params_p->Device;
#if !defined(ST_OSLINUX)
    /* Create timeout semaphore */
    CompareSemaphore_p =STOS_SemaphoreCreateFifoTimeOut(NULL,0);
#endif
    /*****************************************
    Init Tests - smartcard
    *****************************************/

    /* Make the call and then print the error value */
    error = STPIO_Init(DeviceName[Device], &InitParams[Device]);
    DebugError("STPIO_Init()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to initialize pio device");
    }

    /*****************************************
    Open Tests - smartcard
    *****************************************/

    /* set up for "detect" on smartcard */
    OpenParams.ReservedBits    = BitMask;
    OpenParams.BitConfigure[0] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[1] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[2] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[3] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[4] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[5] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[6] = STPIO_BIT_OUTPUT;
#if defined(ST_5107)
    if(DeviceName[Device]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
#endif

     OpenParams.IntHandler      = CompareNotifyFunction;
    /* Make the call and then print the error value */
    error = STPIO_Open(DeviceName[Device], &OpenParams, &HandleIRQ);
    DebugError("STPIO_Open()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Unable to open pio device");
    }
#if !defined(ST_OSLINUX)
    /* Obtains status and display */
    error = STPIO_GetStatus(HandleIRQ, &status);
    DebugError("STPIO_GetStatus()", error);
    DisplayPioStatus(&status);
#endif
    /*****************************************
    Compare Tests
    *****************************************/

    /* Clear compare bit to ensure pattern is zero */
    DebugMessage("Ensure compare bit is cleared....");
    error = STPIO_Clear(HandleIRQ, BitMask);

    /* Set compare tests */
    DebugMessage("Configuring compare pattern for compare bit....");

    setCmpReg.CompareEnable = BitMask;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    DebugError("STPIO_SetCompare()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

#if !defined(ST_OSLINUX)
    /* Wait for a settling period to ensure no callbacks occur */
    DebugMessage("Wait for a settling period (in case of spurious callbacks)....");
    clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != -1)
    {
        TestFailed(Result, "Unexpected notify callback");
    }
#endif/*ST_OSLINUX*/

    DebugMessage("Set the compare bit to invoke the callback....");
    error = STPIO_Set(HandleIRQ, BitMask);
    DebugError("STPIO_Set()", error);

#if !defined(ST_OSLINUX)
    clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != 0)
    {
    	TestFailed(Result, "Timed out waiting for callback");
    }
    else
    {
        DebugMessage("Received callback!");
        TestPassed(Result);
    }
#endif/*ST_OSLINUX*/

    error = STPIO_Close(HandleIRQ);
    DebugError("STPIO_Close()", error);

    /*****************************************
    CompareClear Tests
    *****************************************/
#if !defined(ST_OSLINUX)

    OpenParams.IntHandler = CompareClearNotifyFunction;
    error = STPIO_Open(DeviceName[Device], &OpenParams, &HandleIRQ);
    DebugError("STPIO_Open()", error);

    /* Check that auto mode still works as it should */
    DebugMessage("Checking that auto mode gives two callbacks");

    /* Set compare tests */
    DebugMessage("Clearing compare enable bits....");

    setCmpReg.CompareEnable  = 0;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    DebugError("STPIO_SetCompare()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    /* Clear compare bit to ensure pattern is zero */
    DebugMessage("Ensure compare bit is cleared....");
    error = STPIO_Clear(HandleIRQ, BitMask);

    /* Do not set compareclear value - should default to auto. If not, that's
     * also a bug...
     */

    /* Set compare */
    DebugMessage("Configuring compare pattern for compare bit....");

    setCmpReg.CompareEnable = BitMask;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    DebugError("STPIO_SetCompare()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    /* Wait for a settling period to ensure no callbacks occur */
    DebugMessage("Wait for a settling period (in case of spurious callbacks)....");
    clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != -1)
    {
        TestFailed(Result, "Unexpected notify callback");
    }

    DebugMessage("Set the compare bit to invoke the callback....");
    error = STPIO_Set(HandleIRQ, BitMask);
    DebugError("STPIO_Set()", error);

    clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != 0)
    {
        TestFailed(Result, "Timed out waiting for callback");
    }
    else
    {
        if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != 0)
        {
            TestFailed(Result, "Only received one callback");
        }
        else
        {
            DebugMessage("Received two callbacks");
            TestPassed(Result);
        }
    }
    /* Check that manual mode works as it should */

    DebugMessage("Checking that manual mode only gives one callback");

    /* Set compare tests */
    DebugMessage("Clearing compare enable bits....");

    setCmpReg.CompareEnable = 0;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    DebugError("STPIO_SetCompare()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    /* Clear compare bit to ensure pattern is zero */
    DebugMessage("Ensure compare bit is cleared....");
    error = STPIO_Clear(HandleIRQ, BitMask);

    DebugMessage("Setting CompareClear value");
    error = STPIO_SetCompareClear(HandleIRQ, STPIO_COMPARE_CLEAR_MANUAL);
    DebugError("STPIO_SetCompareClear()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    /* Set compare tests */
    DebugMessage("Configuring compare pattern for compare bit....");

    setCmpReg.CompareEnable = BitMask;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    DebugError("STPIO_SetCompare()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    /* Wait for a settling period to ensure no callbacks occur */
    DebugMessage("Wait for a settling period (in case of spurious callbacks)....");

    clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != -1)
    {
        TestFailed(Result, "Unexpected notify callback");
    }

    DebugMessage("Set the compare bit to invoke the callback....");
    error = STPIO_Set(HandleIRQ, BitMask);
    DebugError("STPIO_Set()", error);

    clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != 0)
    {
        TestFailed(Result, "Timed out waiting for callback");
    }
    else
    {
        if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, TIMEOUT_IMMEDIATE) != 0)
        {
            DebugMessage("Received *single* callback");
            TestPassed(Result);
        }
        else
        {
            TestFailed(Result, "Received *two* callbacks!");
        }
    }
    /* Tidy up */
    STOS_SemaphoreDelete(NULL,CompareSemaphore_p);
#endif /*ST_OSLINUX*/
    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(DeviceName[Device], &TermParams);
    DebugError("STPIO_Term()", error);
    if (error == ST_NO_ERROR)
    {
        TestPassed(Result);
    }
    else
    {
        TestFailed(Result, "Expected ST_NO_ERROR");
    }

    return Result;
}

static void CompareNotifyFunction(STPIO_Handle_t Handle,
                                  STPIO_BitMask_t ActiveBits)
{
    STOS_SemaphoreSignal(CompareSemaphore_p);
}

static void CompareClearNotifyFunction(STPIO_Handle_t Handle,
                                       STPIO_BitMask_t ActiveBits)
{
    /* Clear the pins, so we shouldn't keep getting interrupts */
    STPIO_Clear(Handle, ActiveBits);
    STOS_SemaphoreSignal(CompareSemaphore_p);

}

#endif/*CompareTestNumIterations*/

#ifdef ARCHITECTURE_ST20

/* Stack usage code - worker functions */

static void stack_overhead(void *dummy) { }

static void stack_init(void *dummy)
{
    ST_ErrorCode_t      error;

    error = STPIO_Init(DeviceName[0], &InitParams[0]);
    if (error != ST_NO_ERROR)
    {
        DebugPrintf(("Error trying to initialise - %s\n",
                    ErrorToString(error)));
    }
}

static void stack_typical(void *dummy)
{
    ST_ErrorCode_t      error;
    STPIO_OpenParams_t  OpenParams;
    STPIO_Handle_t      InputHandle, OutputHandle;
    U8                  InputPin;

    /*****************************************
    Open Tests
    *****************************************/

    /* setup pio bit required */
    OpenParams.ReservedBits     = 32;
    OpenParams.BitConfigure[0]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[1]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[2]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[3]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[4]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[5]  = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[6]  = STPIO_BIT_OUTPUT;
#if defined(ST_5107)
    if(DeviceName[0]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_OUTPUT;
#endif
    OpenParams.IntHandler       = NULL;

    error = STPIO_Open(DeviceName[0], &OpenParams, &OutputHandle);
    if (error != ST_NO_ERROR)
    {
        DebugError("STPIO_Open()", error);
    }

    OpenParams.ReservedBits     = 64;
    OpenParams.BitConfigure[0]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[1]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[2]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[3]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[4]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[5]  = STPIO_BIT_INPUT;
    OpenParams.BitConfigure[6]  = STPIO_BIT_INPUT;
#if defined(ST_5107)
    if(DeviceName[0]!="pio3")
        OpenParams.BitConfigure[7]  = STPIO_BIT_INPUT;
    else
        OpenParams.BitConfigure[7]  = STPIO_BIT_NOT_SPECIFIED;
#else
        OpenParams.BitConfigure[7]  = STPIO_BIT_INPUT;
#endif
    OpenParams.IntHandler       = NULL;

    error = STPIO_Open(DeviceName[0], &OpenParams, &InputHandle);
    if (error != ST_NO_ERROR)
    {
        DebugError("STPIO_Open()", error);
    }

    error = STPIO_Clear(OutputHandle, 32);
    if (error != ST_NO_ERROR)
    {
        DebugError("STPIO_Clear()", error);
    }

    /* Read the input bit */
    error = STPIO_Read(InputHandle, &InputPin);
    if (error != ST_NO_ERROR)
    {
        DebugError("STPIO_Read()", error);
    }

    error = STPIO_Set(OutputHandle, 32);
    if (error != ST_NO_ERROR)
    {
        DebugError("STPIO_Set()", error);
    }

    error = STPIO_Close(InputHandle);
    if (error != ST_NO_ERROR)
    {
        DebugPrintf(("Error closing input handle - %s\n",
                    ErrorToString(error)));
    }
    error = STPIO_Close(OutputHandle);
    if (error != ST_NO_ERROR)
    {
        DebugPrintf(("Error closing output handle - %s\n",
                    ErrorToString(error)));
    }
}

static void stack_term(void *dummy)
{
    ST_ErrorCode_t      error;
    STPIO_TermParams_t  TermParams;

    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(DeviceName[0], &TermParams);
    if (error != ST_NO_ERROR)
    {
        DebugPrintf(("Error trying to terminate - %s\n",
                    ErrorToString(error)));
    }
}

void test_datasize(void)
{
    STPIO_TermParams_t  TermParams;
    ST_ErrorCode_t  error;
    U8              *marker;
    U32             Address1, Address2;

    marker = STOS_MemoryAllocate(system_partition, 1);
    Address1 = (U32)marker;
    STOS_MemoryDeallocate(system_partition, marker);

    error = STPIO_Init(DeviceName[0], &InitParams[0]);
    if (error != ST_NO_ERROR)
    {
        DebugPrintf(("Error trying to initialise - %s\n",
                    ErrorToString(error)));
    }
    else
    {
        marker = STOS_MemoryAllocate(system_partition, 1);
        Address2 = (U32)marker;
        STOS_MemoryDeallocate(system_partition, marker);

        TermParams.ForceTerminate = FALSE;
        error = STPIO_Term(DeviceName[0], &TermParams);

        DebugPrintf(("Memory used by one instance: %i\n", Address1 - Address2));
    }
}

#if !defined(ST_OSLINUX)
/* Stack usage test - main test function */
static TestResult_t StackUsage(TestParams_t *TestParams_p)
{
    TestResult_t Result = TEST_RESULT_ZERO;
#ifndef ST_OS21
    tdesc_t tdesc;
    task_t task;
#endif

    task_t *task_p;
    task_status_t   status;
    char stack[4 * 1024];
    void (*func_table[])(void *) = { stack_overhead,
                                     stack_init,
                                     stack_typical,
                                     stack_term,
                                     NULL
                                   };
    U8 i;
    void (*func)(void *);

#ifndef ST_OS21
    task_p = &task;
#endif

    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];
#ifdef ST_OS21
        task_p = task_create(func, NULL, 16*1024, MAX_USER_PRIORITY,
                             "stack_test", 0);

        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        task_status(task_p, &status, task_status_flags_stack_used);
#else
        task_init(func, NULL, stack, 4 * 1024, task_p, &tdesc,
                  MAX_USER_PRIORITY, "stack_test", 0);

        task_wait(&task_p, 1, TIMEOUT_INFINITY);

        task_status(task_p, &status, 0);
#endif
        DebugPrintf(("Function %i used %i bytes stack (max)\n",
                    i, status.task_stack_used));
        task_delete(task_p);
    }

    test_datasize();

    return Result;
}

#endif
#endif/*ST_OSLINUX*/
/*****************************************************************************
Debug output functions
*****************************************************************************/

/* Error message look up table */
static ErrorMessage_t ErrorLUT[] =
{
    /***************************** STAPI *******************************/
    { ST_NO_ERROR, "ST_NO_ERROR" },
    { ST_ERROR_NO_MEMORY, "ST_ERROR_NO_MEMORY" },
    { ST_ERROR_INTERRUPT_INSTALL, "ST_ERROR_INTERRUPT_INSTALL" },
    { ST_ERROR_ALREADY_INITIALIZED, "ST_ERROR_DEVICE_INITIALIZED" },
    { ST_ERROR_UNKNOWN_DEVICE, "ST_ERROR_UNKNOWN_DEVICE" },
    { ST_ERROR_BAD_PARAMETER, "ST_ERROR_BAD_PARAMETER" },
    { ST_ERROR_OPEN_HANDLE, "ST_ERROR_OPEN_HANDLE" },
    { ST_ERROR_NO_FREE_HANDLES, "ST_ERROR_NO_FREE_HANDLES" },
    { ST_ERROR_INVALID_HANDLE, "ST_ERROR_INVALID_HANDLE" },
    { ST_ERROR_INTERRUPT_UNINSTALL, "ST_ERROR_INTERRUPT_UNINSTALL" },
    { ST_ERROR_TIMEOUT, "ST_ERROR_TIMEOUT" },
    { ST_ERROR_FEATURE_NOT_SUPPORTED, "ST_ERROR_FEATURE_NOT_SUPPORTED" },
    { ST_ERROR_DEVICE_BUSY, "ST_ERROR_DEVICE_BUSY" },
#if !defined(ST_OSLINUX)
    /***************************** STBOOT ******************************/
    { STBOOT_ERROR_KERNEL_INIT, "STBOOT_ERROR_KERNEL_INIT" },
    { STBOOT_ERROR_KERNEL_START, "STBOOT_ERROR_KERNEL_START" },
    { STBOOT_ERROR_INTERRUPT_INIT, "STBOOT_ERROR_INTERRUPT_INIT" },
    { STBOOT_ERROR_INTERRUPT_ENABLE, "STBOOT_ERROR_INTERRUPT_ENABLE" },
    { STBOOT_ERROR_UNKNOWN_BACKEND, "STBOOT_ERROR_UNKNOWN_BACKEND" },
    { STBOOT_ERROR_BACKEND_MISMATCH, "STBOOT_ERROR_BACKEND_MISMATCH" },
    { STBOOT_ERROR_INVALID_DCACHE, "STBOOT_ERROR_INVALID_DCACHE" },
    { STBOOT_ERROR_SDRAM_INIT, "STBOOT_ERROR_SDRAM_INIT" },
#endif/*LINUX*/
    /******************************* END *******************************/
    { ST_ERROR_UNKNOWN, "ST_ERROR_UNKNOWN" } /* Terminator */
};

char *ErrorToString(ST_ErrorCode_t Error)
{
    ErrorMessage_t *mp;

    mp = ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return (char *)mp->ErrorMsg;
}

const char *ConfigToString(STPIO_BitConfig_t Config)
{
    static const char *ConfigStr[] =
    {
        "STPIO_BIT_NOT_SPECIFIED",
        "STPIO_BIT_BIDIRECTIONAL",
        "STPIO_BIT_OUTPUT",
        "STPIO_BIT_INPUT",
        "STPIO_BIT_ALTERNATE_OUTPUT",
        "STPIO_BIT_ALTERNATE_BIDIRECTIONAL"
    };

    return ConfigStr[(U32)Config - (U32)STPIO_BIT_NOT_SPECIFIED];
}

void DisplayPioStatus(STPIO_Status_t *Status_p)
{
    U8 i;

    DebugPrintf(("Used bits are 0x%02x...\n", Status_p->BitMask));
    for (i = 0; i < 8; i++)
    {
        DebugPrintf(("Bit %d config = %s\n",
                    i,
                    ConfigToString(Status_p->BitConfigure[i])));
    }
}

/* End of module */
