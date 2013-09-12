/************************************************************************
COPYRIGHT (C) STMicroelectronics 1999

source file name : revtest.c

Test harness for the revision reporting functions in stcommon.c

************************************************************************/

#include <stdio.h>

#include "stddefs.h"

#include "stboot.h"
#include "stcommon.h"
#include "stsys.h"
#include "stdevice.h"
#include "stlite.h"

#ifndef DISABLE_TOOLBOX
#include "sttbx.h"
#endif

extern void vClockTest( void );
extern void vWaitTest( void );

#ifdef DISABLE_TOOLBOX
#define Printf(x)       printf x
#else
#define Printf(x)       STTBX_Print(x)
#endif

/* Cache configuration */
#ifndef DISABLE_ICACHE
#define DISABLE_ICACHE   FALSE
#endif

#ifndef DISABLE_DCACHE
#define DISABLE_DCACHE   FALSE
#endif

#if (!DISABLE_ICACHE)

#if defined CACHEABLE_BASE_ADDRESS
/* Use predefined values from board vob */
#define DCACHE_START     CACHEABLE_BASE_ADDRESS
#define DCACHE_END       CACHEABLE_STOP_ADDRESS
        
#elif defined(mb314) || defined (mb361)

#define DCACHE_START     0x40200000
#define DCACHE_END       0x5FFFFFFF

#else

#define DCACHE_START     0x40080000
#define DCACHE_END       0x5FFFFFFF

#endif

#else

#define DCACHE_START     NULL
#define DCACHE_END       NULL

#endif

#ifdef ARCHITECTURE_ST20
/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;

#pragma ST_section      ( internal_block, "internal_section_noinit")

#define                 NCACHE_MEMORY_SIZE          0x80000
unsigned char           ncache_memory [NCACHE_MEMORY_SIZE];
partition_t             the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;

#pragma ST_section      ( ncache_memory , "ncache_section" )

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;

#pragma ST_section      ( external_block, "system_section_noinit")
#endif /*#ifdef ARCHITECTURE_ST20*/

#if defined(ST_OS21) || defined(ST_OSWINCE)
#define                 SYSTEM_MEMORY_SIZE          0x100000
unsigned char           ExternalBlock[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;
#endif

/* Array of test strings */
#define NUM_TESTS       9

typedef struct {
    U32 Major, Minor, Patch, Numeric;
} Rev_t;

static ST_Revision_t TestData[] =
{
    "DRIVER-REL_1.2.3",
    "DRIVER NICK-REL_12.34.56 optional string",
    "$%^&*()DRIVER NICK-REL_12.34.56 opo",
    "DRIVER*REL_12.34.56 optional string",
    "DRIVER NICK_REL_12.34.56 optional string",
    "DRIVER NICK-REL*1.2.3 optional string",
    "DRIVER NICK-REL_1.*.3 optional string",
    "DRIVER NICK-REL_1.2.& optional string",
    "DRIVER NICK-REL_2.34.56=optional string"
};

static Rev_t Results[] = {
    {       1,       2,       3, 0x10203 },
    {      12,      34,      56, 0xc2238 },
    {      12,      34,      56, 0xc2238 },
    { (U32)-1, (U32)-1, (U32)-1, 0x00000 },
    { (U32)-1, (U32)-1, (U32)-1, 0x00000 },
    { (U32)-1, (U32)-1, (U32)-1, 0x00000 },
    { (U32)-1, (U32)-1, (U32)-1, 0x00000 },
    { (U32)-1, (U32)-1, (U32)-1, 0x00000 },
    { (U32)-1, (U32)-1, (U32)-1, 0x00000 }
};

U32 TotalPassed = 0;
U32 TotalFailed = 0;

/* TEST HARNESS FUNCTION PROTOTYPES */

static void TestPassed(void) {
    Printf(("PASSED\n"));
    TotalPassed++;
}

static void TestFailed(const char *string) {
    Printf(("FAILED: %s\n", string));
    TotalFailed++;
}

void test_overhead( void *dummy ) {}

void test_typical( void *dummy )
{
    #if defined(ST_OS21) || defined(ST_OSWINCE)
    osclock_t t;
    #else
    clock_t t;
    #endif
    ST_ClockInfo_t info;
    ST_ErrorCode_t err;
    U32 m, n, p;

    /* Obtain clock information */
    err = ST_GetClockInfo(&info);

    /* Call revision string */
    m = ST_GetMajorRevision(&TestData[0]);
    n = ST_GetMinorRevision(&TestData[0]);
    p = ST_GetPatchRevision(&TestData[0]);

    /* Obtain clocks per second */
    t = ST_GetClocksPerSecond();
}

void vStackUsage( void )
{
    #ifdef ST_OS20
    task_t          task;
    tdesc_t         tdesc;
    #endif
    task_t          *task_p;
    task_status_t   status;
    #ifdef ARCHITECTURE_ST20
    U8              stack[4 * 1024];
    #endif
    void (*func_table[])(void *) =  {   test_overhead,
                                        test_typical,
                                        NULL
                                    };
    void (*func)(void *);
    U8              i;

    Printf(("**************************************************\n"));
    Printf(("TEST: Library stack usage...\n"));
    Printf(("**************************************************\n"));

    #ifdef ST_OS20
    task_p = &task;
    #endif
    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];
#ifdef ST_OS20
        task_init(func, NULL, stack, 4 * 1024, task_p, &tdesc, MAX_USER_PRIORITY, "stack_test", 0);
        task_wait(&task_p, 1, TIMEOUT_INFINITY);
        task_status(task_p, &status, task_status_flags_stack_used);
        Printf(("Function %i, stack used = %d\n", i, status.task_stack_used));
        task_delete(task_p);
#endif

#if defined(ST_OS21) || defined(ST_OSWINCE)
        task_p = task_create(func, NULL, OS21_DEF_MIN_STACK_SIZE, MAX_USER_PRIORITY, "stack_test", 0); /*OS21_DEF_MIN_STACK_SIZE is (16 * 1024)bytes for ST40, defined in the file:C:\STM\ST40R2.1.4\sh-superh-elf\include\os21\st40\osdefs.h*/
        task_wait(&task_p, 1, TIMEOUT_INFINITY);
        task_status(task_p, &status, task_status_flags_stack_used);
        Printf(("Function %i, stack used = %d bytes\n", i, status.task_stack_used));
        task_delete(task_p);
#endif
    }
}

void vRevTest( void )
{
    int  i = 0;
    ST_Revision_t *Rev_p;

    Printf(("**************************************************\n"));
    Printf(("TEST: Revision strings are processed correctly...\n"));
    Printf(("**************************************************\n"));

    /*** TRIAL TESTS ***/
    for (i = 0; i < NUM_TESTS; i++)
    {
        U32 u32NumericRevision,
            u32MajorRevision,
            u32MinorRevision,
            u32PatchRevision;

        Rev_p = &TestData[i];
        u32MajorRevision = ST_GetMajorRevision(Rev_p);
        u32MinorRevision = ST_GetMinorRevision(Rev_p);
        u32PatchRevision = ST_GetPatchRevision(Rev_p);
        u32NumericRevision = ST_ConvRevToNum(Rev_p);

        Printf(("String is %s\n", (char *)(*Rev_p)));
        Printf(("Major %d Minor %d Patch %d  ",
               u32MajorRevision, u32MinorRevision, u32PatchRevision ));
        Printf(("Numeric 0x%x\n", u32NumericRevision ));

        if ((u32MajorRevision != Results[i].Major) ||
            (u32MinorRevision != Results[i].Minor) ||
            (u32PatchRevision != Results[i].Patch) ||
            (u32NumericRevision != Results[i].Numeric))
        {
            TestFailed("Bad revision number");
        }
        else
        {
            TestPassed();
        }
    }

    return;
}


void vStringEqTest( void )
{
    BOOL returnedvalue;
    Printf(("**************************************************\n"));
    Printf(("TEST: ST_AreStringsEqual() API test...\n"));
    Printf(("**************************************************\n"));
    
    /*1*/
    Printf(("Comparing strings:test,test\n"));
	returnedvalue = ST_AreStringsEqual("test","test");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));

    if (!returnedvalue)
    {
        TestFailed("Strings didnt match.");
    }
    else
    {
        Printf(("Strings Matched.\n"));
        TestPassed();
    }
    
    /*2*/
    Printf(("Comparing strings:test1,test\n"));
    returnedvalue = ST_AreStringsEqual("test1","test");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));

    if (returnedvalue)
    {
        TestFailed("Strings Matched.");
    }
    else
    {
        Printf(("Strings didnt match.\n"));
        TestPassed();
    }
    
    /*3*/
    Printf(("Comparing strings:test,test1\n"));
    returnedvalue = ST_AreStringsEqual("test","test1");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));
    if (returnedvalue)
    {
        TestFailed("Strings Matched.");
    }
    else
    {
        Printf(("Strings didnt match.\n"));
        TestPassed();
    }
    
    /*4*/
    Printf(("Comparing strings:test1,test10\n"));
    returnedvalue = ST_AreStringsEqual("test1","test10");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));

    if (returnedvalue)
    {
        TestFailed("Strings Matched.");
    }
    else
    {
        Printf(("Strings didnt match.\n"));
        TestPassed();
    }
    
    /*5*/
    Printf(("Comparing strings:Test,TEst\n"));
    returnedvalue = ST_AreStringsEqual("Test","TEst");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));
	
    if (returnedvalue)
    {
        TestFailed("Strings Matched.");
    }
    else
    {
        Printf(("Strings didnt match.\n"));
        TestPassed();
    }
    
    /*6*/
    Printf(("Comparing strings:test@#1,test#@1\n"));
    returnedvalue = ST_AreStringsEqual("test@#1","test#@1");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));

    if (returnedvalue)
    {
        TestFailed("Strings Matched.");
    }
    else
    {
        Printf(("Strings didnt match.\n"));
        TestPassed();
    }
    
    /*7*/
    Printf(("Comparing strings:TEst#@1,TEst#@1\n"));
	returnedvalue = ST_AreStringsEqual("TEst#@1","TEst#@1");
	Printf(("Returned Value(TRUE-1, FALSE-0): %d\n",returnedvalue));

    if (!returnedvalue)
    {
        TestFailed("Strings didnt match.");
    }
    else
    {
        Printf(("Strings Matched.\n"));
        TestPassed();
    }
}


int main(int argc, char *argv[])
{
    #ifndef DISABLE_TOOLBOX
	STTBX_InitParams_t  TBXInitParams;
    #endif
    STBOOT_InitParams_t BootParams;
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)DCACHE_START, (U32 *)DCACHE_END }, /* Cacheable */
        { NULL, NULL }                  /* End */
    };
    
    /* Create memory partitions */
    #if defined(ST_OS21) || defined(ST_OSWINCE)
    system_partition = partition_create_heap((U8*)ExternalBlock, sizeof(ExternalBlock));
    #endif
    
    #ifdef ARCHITECTURE_ST20
    partition_init_simple(&the_internal_partition,
                          (U8*)internal_block,
                          sizeof(internal_block)
                         );
    partition_init_heap(&the_system_partition,
                        (U8*)external_block,
                        sizeof(external_block));
    #endif

    /* BOOT */
    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
    BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
    BootParams.ICacheEnabled = !DISABLE_ICACHE;
    BootParams.DCacheMap = DCacheMap;
    BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.MemorySize = SDRAM_SIZE;
#if defined(ST_OS21) || defined(ST_OSWINCE)
    BootParams.TimeslicingEnabled = TRUE;
#endif

    if (STBOOT_Init( "boot", &BootParams ) != ST_NO_ERROR)
    {
        Printf(("Unable to boot!\n"));
    }

#ifdef DISABLE_TOOLBOX
/*Do nothing*/
#else
    TBXInitParams.SupportedDevices = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p = system_partition;

    STTBX_Init( "tbx", &TBXInitParams );
#endif

    Printf(("**************************************************\n"));
    Printf(("              STCOMMON Test Harness               \n"));
    /*Printf(("Driver Revision: %s\n", STCOMMON_REVISION));*/
    Printf(("STCOMMON Revision: %s\n", STCOMMON_GetRevision() ));
    Printf(("BOARD Revision: %s\n", BOARD_GetRevision() ));
    Printf(("CHIP Revision: %s\n", CHIP_GetRevision() ));
    Printf(("INCLUDE Revision: %s\n", INCLUDE_GetRevision() ));
    Printf(("ICACHE = %s\n", DISABLE_ICACHE ? "NO" : "YES"));
    Printf(("DCACHE = %s\n", DISABLE_DCACHE ? "NO" : "YES"));

#if defined(ST_OS21) || defined(ST_OSWINCE)
/*Print Nothing*/
#elif defined(UNIFIED_MEMORY) || defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5700) || defined(ST_5188) || defined(ST_5107)
    Printf(("UNIFIED_MEMORY: "));
    Printf(("TRUE\n"));
#else
    Printf(("UNIFIED_MEMORY: "));
    Printf(("FALSE\n"));
#endif
    #ifdef DISABLE_TOOLBOX
    #if defined(ST_7100) || defined(ST_7109) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107)
    Printf(("Device Cut = %0x\n", ST_GetCutRevision()));
    #endif
    Printf(("CPU Clock = %d\n", ST_GetClockSpeed()));
    Printf(("**************************************************\n"));
    Printf(("              Dependencies Revisions              \n"));
    Printf(("           STBOOT:  %s\n",  STBOOT_GetRevision()   ));
    /*Printf(("            STPIO:  %s\n",  STPIO_GetRevision()    ));*/
    Printf(("            STSYS:  %s\n",  STSYS_GetRevision()    ));
    /*Printf(("            STTBX:  %s\n",  STTBX_GetRevision()   ));
    Printf(("           STUART:  %s\n",  STUART_GetRevision()  ));*/
    Printf(("**************************************************\n"));
    Printf(("                  Test built on                   \n"));
    Printf(("        Date: %s   Time: %s \n", __DATE__, __TIME__ ));
    #else
    #if defined(ST_7100) || defined(ST_7109) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107)
    Printf(("Device Cut = %0x\n", ST_GetCutRevision()));
    #endif
    Printf(("CPU Clock = %d\n", ST_GetClockSpeed()));
    Printf(("**************************************************\n"));
    Printf(("              Dependencies Revisions              \n"));
    Printf(("           STBOOT:  %s\n",  STBOOT_GetRevision()   ));
    Printf(("            STPIO:  %s\n",  STPIO_GetRevision()    ));
    Printf(("            STSYS:  %s\n",  STSYS_GetRevision()    ));
    Printf(("            STTBX:  %s\n",  STTBX_GetRevision()   ));
    Printf(("           STUART:  %s\n",  STUART_GetRevision()  ));
    Printf(("**************************************************\n"));
    Printf(("                  Test built on                   \n"));
    Printf(("        Date: %s   Time: %s \n", __DATE__, __TIME__ ));
    #endif


    /* Perform revision tests */
    vRevTest();

    /* Perform clock tests */
    vClockTest();

    /* Perform timer tests */
    /*vWaitTest();*/

    /* Stack usage tests */
    vStackUsage();
    
    /* ST_AreStringsEqual() API check test */
    vStringEqTest();

    Printf(("**************************************************\n"));
    Printf(("PASSED: %i\n", TotalPassed));
    Printf(("FAILED: %i\n", TotalFailed));
    Printf(("**************************************************\n"));
    return 0;
}

/* EOF */
