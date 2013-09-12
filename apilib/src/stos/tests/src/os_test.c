/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

File name   : os_test.c
Description : STOS specific tests

Note        : All functions return TRUE if error for CLILIB compatibility

Date          Modification                                    Initials
----          ------------                                    --------
06 Mar 2007   Creation                                        CD

************************************************************************/

/*#######################################################################*/
/*########################## INCLUDES FILE ##############################*/
/*#######################################################################*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef ST_OSLINUX
#include "stostest_ioctl.h"
#endif

#include "stdevice.h"
#include "sttbx.h"
#include "stboot.h"

#include "memory.h"
#include "tasks.h"
#include "regions.h"
#include "semaphores.h"
#include "stos_mutex.h"


/*#######################################################################*/
/*########################### DEFINITION ################################*/
/*#######################################################################*/

/* --- Constants (default values) -------------------------------------- */

/* --- Private Constants --- */
#if !defined ST_OSLINUX
#define STTBX_DEVICE_NAME   "TBX"
#define STBOOT_DEVICE_NAME  "STBOOT"


#if defined(mb411)
/* 7100/7109*/
#define BOARD_NAME                         "mb411"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x00600000   /* 6.0 MBytes */
#endif /* SYSTEM_SIZE */
/*  no DataPartition, we will allocate buffer to load streams in the STAVMEM_SYS_MEMORY */
/*  in vid_util.c instead of memory allocate in DataPartition_p */

#elif defined(mb519)
/* 7200 */
#define BOARD_NAME                         "mb519"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        SH4_SYSTEM_PARTITION_MEMORY_SIZE
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   NCACHE_PARTITION_MEMORY_SIZE
#endif /* DATA_MEMORY_SIZE */

#elif defined(mb391)
/* 7710 */
#define BOARD_NAME                         "mb391"
#ifndef SYSTEM_SIZE
#define SYSTEM_SIZE                        0x0600000    /* 6 MBytes */
#endif /* SYSTEM_SIZE */
#ifndef DATA_MEMORY_SIZE
#define DATA_MEMORY_SIZE                   0x0800000    /* 8 MBytes */
#endif /* DATA_MEMORY_SIZE */

#endif  /* All SYSTEM_SIZE & DATA_MEMORY_SIZE */



#if defined CACHEABLE_BASE_ADDRESS
#define DCACHE_START  CACHEABLE_BASE_ADDRESS /* assumed ok */
#define DCACHE_END    CACHEABLE_STOP_ADDRESS /* assumed ok */

#elif defined(mb314) || defined(mb361) || defined(mb382)
#if defined (UNIFIED_MEMORY)
#define DCACHE_START  0xC0380000
#define DCACHE_END    0xC07FFFFF
#else /* UNIFIED_MEMORY */

#define DCACHE_START  0x40200000
#define DCACHE_END    0x4FFFFFFF  /* STBOOT tests: 0x407FFFFF */
#endif /* UNIFIED_MEMORY */

#elif defined(mb290) /* assumption: no Unified memory configuration requested on mb290 */
#define DCACHE_START  0x40080000
#define DCACHE_END    0x4FFFFFFF  /* STBOOT tests: 0x407FFFFF */
#define DCACHE_SECOND_BANK

#else
#define DCACHE_START  0x40080000
#define DCACHE_END    0x4FFFFFFF  /* STBOOT tests: 0x7FFFFFFF */
#endif

/*if defined DCACHE_SECOND_BANK*/
#define DCACHE_START2       0x50000000
#define DCACHE_END2         0x5FFFFFFF

#ifdef DCACHE_TESTS_SDRAM
#define DCACHE_START3       SDRAM_BASE_ADDRESS
#define DCACHE_END3         (SDRAM_BASE_ADDRESS + 0x00080000 - 1)
#endif /* #ifdef DCACHE_TESTS_SDRAM */

#ifndef NCACHE_PARTITION_SIZE
    #define NCACHE_PARTITION_SIZE   0x0080000
#endif
#ifndef NCACHE_SIZE
    #if defined (ST_5301) ||  defined (ST_5525)
        #define NCACHE_SIZE             0x0800000    /* 8MB */
    #else
        #define NCACHE_SIZE             NCACHE_PARTITION_SIZE    /* As defined in clavmem.h */
    #endif
#endif /* NCACHE_SIZE not defined */


#endif  /* !LINUX */



/* --- Global variables --- */
typedef struct
{
    U8              Choice;
    char            Description[50];
    STOS_Error_t    (*Function)(void);
} TestFunction_t;

static TestFunction_t MemoryTestFunctions[] = {
                        {'1', "TestAllocate_write_read              ",  TestAllocate_write_read},
                        {'2', "TestDeallocate_after_alloc           ",  TestDeallocate_after_alloc},
                        {'3', "TestAllocateClear_clear_write_read   ",  TestAllocateClear_clear_write_read},
                        {'4', "TestDeallocate_after_allocclear      ",  TestDeallocate_after_allocclear},
                        {'5', "TestReallocate_write_read            ",  TestReallocate_write_read},
                        {'6', "TestDeallocate_after_realloc         ",  TestDeallocate_after_realloc},
                        {'7', "TestReallocate_block_NULL            ",  TestReallocate_block_NULL},
                        {'8', "TestReallocate_size_zero             ",  TestReallocate_size_zero}
                                                };
static TestFunction_t TasksTestFunctions[] = {
                        {'1', "TestTasks_tasks_end                  ",  TestTasks_tasks_end}
                                                };
static TestFunction_t RegionsTestFunctions[] = {
                        {'1', "TestMapRegister_write_read           ",  TestMapRegister_write_read},
                        {'2', "TestUnmapRegister_aftermap           ",  TestUnmapRegister_aftermap},
                        {'3', "TestUnmapRegister_nomap              ",  TestUnmapRegister_nomap},
                        {'4', "TestVirtToPhys_correct_address       ",  TestVirtToPhys_correct_address},
                        {'5', "TestInvalidateRegion_invalidate      ",  TestInvalidateRegion_invalidate},
                        {'6', "TestFlushRegion_flush                ",  TestFlushRegion_flush}
                                                };
static TestFunction_t SemaphoresTestFunctions[] = {
                        {'1', "TestSemaphores_performances          ",  TestSemaphores_performances},
                        {'2', "TestSemaphores_wait_signal_priorities",  TestSemaphores_wait_signal_priorities}
                                                };

static TestFunction_t MutexTestFunctions[] = {
                        {'1', "TestMutex_mutex_fifo                 ",  TestMutex_mutex_fifo},
                        {'2', "TestMutex_mutex_stress               ",  TestMutex_mutex_stress},
                        {'3', "TestMutex_mutex_delete               ",  TestMutex_mutex_delete},
                        {'4', "TestMutex_mutex_lock                 ",  TestMutex_mutex_lock},
                        {'5', "TestMutex_mutex_memory               ",  TestMutex_mutex_memory}
                                                };

/* --- Extern variables --- */

/* --- Private variables --- */
#if !defined ST_OSLINUX

/* Allow room for OS20 segments in internal memory */
#ifdef ST_OS20
#if defined(mb391)
/* Manage only 7710 case */
static unsigned char    InternalBlock [ST20_INTERNAL_MEMORY_SIZE-1200];
#pragma ST_section      (InternalBlock, "internal_section")
#endif
#endif /* ST_OS20 */

static unsigned char    ExternalBlock[SYSTEM_SIZE];
#pragma ST_section      (ExternalBlock, "system_section")

#if defined (mb411) || defined (mb519)
    /* no DataBlock definition */
    /* fixed NCacheMemory mapping */
    #if defined(ST_7200)
    /* Check memory mapping in mb519.h */
    static unsigned char    *NCacheMemory = (unsigned char*)(NCACHE_PARTITION_ADDRESS);
    #elif defined(ST_7109)
    /* Check memory mapping in the clavmem.h */
    static unsigned char    *NCacheMemory = (unsigned char*)0xA5500000;
    #elif defined(ST_7100)
    /* Check memory mapping in the clavmem.h */
    static unsigned char    *NCacheMemory = (unsigned char*)0xA5400000;
    #endif /* defined (ST_7200) */
#elif defined (mb391)

    static unsigned char    NCacheMemory [NCACHE_SIZE];
    #pragma ST_section      (NCacheMemory , "ncache_section")

    /* DataBlock is declared here to avoid linker warning: all the data is allocated in ncache_section to
     * be ble to use more memory */
    static unsigned char    DataBlock[1];
    #pragma ST_section      (DataBlock, "data_section")
#endif /* not mb411 */

static unsigned char    InternalBlockNoinit[1];
#pragma ST_section      ( InternalBlockNoinit, "internal_section_noinit")

static unsigned char    SystemBlockNoinit[1];
#pragma ST_section      ( SystemBlockNoinit, "system_section_noinit")


#ifdef ST_OS20
static partition_t      TheInternalPartition;
static partition_t      TheExternalPartition;
static partition_t      TheNCachePartition;
static partition_t      TheDataPartition;
#endif /* ST_OS20 */


ST_Partition_t          *InternalPartition_p;

/* Global variables ------------------------------------------------------- */
#ifdef ST_OS20
ST_Partition_t          *InternalPartition_p;
#endif
ST_Partition_t          *SystemPartition_p;
ST_Partition_t          *DriverPartition_p;
ST_Partition_t          *NCachePartition_p;

/* following 2 variable are needed by OS20.lib (needed when using PTI and some others not identified */
ST_Partition_t          *ncache_partition;

ST_Partition_t          *internal_partition;

ST_Partition_t          *system_partition;

#if !defined(mb411) && !defined(mb519)
ST_Partition_t          *DataPartition_p;
#endif



STBOOT_DCache_Area_t DCacheMap[]  =
{
    { (U32 *)DCACHE_START, (U32 *)DCACHE_END },
#ifdef DCACHE_SECOND_BANK
    { (U32 *)DCACHE_START2, (U32 *)DCACHE_END2 },
#endif
#ifdef DCACHE_TESTS_SDRAM
    { (U32 *)DCACHE_START3, (U32 *)DCACHE_END3 },
#endif
    { NULL, NULL }
};
STBOOT_DCache_Area_t *DCacheMap_p = &DCacheMap[0];
size_t DCacheMapSize              = sizeof(DCacheMap);


#endif  /* !LINUX */

/* --- Extern functions prototypes --- */

/* --- Private Function prototypes --- */
static void Start(void *ptr);

/*#########################################################################
 *                                 MAIN
 *#######################################################################*/


/*******************************************************************************
Name        : TBX_Init
Description : Initialise the STTBX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
#if !defined ST_OSLINUX
static BOOL TBX_Init(void)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STTBX_InitParams_t          InitParams;
    STTBX_BuffersConfigParams_t BuffParams;
    BOOL                        RetOk = TRUE;

    InitParams.SupportedDevices = STTBX_DEVICE_DCU;
    InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    InitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
    BuffParams.Enable = FALSE;

	/* Following pointer must not be NULL but are not used with LINUX */
	InitParams.CPUPartition_p = (void *) SystemPartition_p;

    ErrorCode = STTBX_Init(STTBX_DEVICE_NAME, &InitParams);
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("STTBX initialized,\trevision=%s\n",STTBX_GetRevision()));
        BuffParams.KeepOldestWhenFull = TRUE;
        STTBX_SetBuffersConfig (&BuffParams);
    }
    else
    {
        printf("STTBX_Init(%s) error=0x%0x !\n", STTBX_DEVICE_NAME, ErrorCode);
        RetOk = FALSE;
    }

    return(RetOk);
}
#endif  /* !LINUX */

/*******************************************************************************
Name        : TBX_Term
Description : Initialise the STTBX driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
#if !defined ST_OSLINUX
static void TBX_Term(void)
{
    ST_ErrorCode_t     ErrCode = ST_NO_ERROR;
    STTBX_TermParams_t TermParams;

    ErrCode = STTBX_Term( STTBX_DEVICE_NAME, &TermParams);
    if (ErrCode == ST_NO_ERROR)
    {
        printf("STTBX_Term(%s): ok\n", STTBX_DEVICE_NAME);
    }
    else
    {
        printf("STTBX_Term(%s): error=0x%0x !\n", STTBX_DEVICE_NAME, ErrCode);
    }
} /* end TBX_Term() */
#endif  /* !LINUX */



/*******************************************************************************
Name        : BOOT_Init
Description : STBOOT initialisation
Parameters  : none
Assumptions :
Limitations :
Returns     : TRUE if success, FALSE otherwise
*******************************************************************************/
#if !defined ST_OSLINUX
BOOL BOOT_Init(void)
{
    STBOOT_InitParams_t InitStboot;
    BOOL                RetOk = TRUE;

    InitStboot.CacheBaseAddress          = (U32 *) CACHE_BASE_ADDRESS;
    InitStboot.ICacheEnabled             = FALSE;
    InitStboot.DCacheMap = DCacheMap;
    InitStboot.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    InitStboot.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    InitStboot.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

    InitStboot.SDRAMFrequency            = SDRAM_FREQUENCY;    /* in kHz */
    InitStboot.MemorySize                = SDRAM_SIZE;
    /* Use default int config whatever compilation flag used for STBOOT driver */
    InitStboot.IntTriggerMask            = 0;
#if !defined ST_OS21
    InitStboot.IntTriggerTable_p         = NULL;
#endif /* NOT ST_OS21 */

#ifdef ST_OS21
    InitStboot.TimeslicingEnabled		 = TRUE;
    InitStboot.DCacheEnabled             = TRUE;
#endif /* ST_OS21 */


    if (STBOOT_Init(STBOOT_DEVICE_NAME, &InitStboot) != ST_NO_ERROR)
    {
        printf("Error: STBOOT failed to initialize !\n");
        RetOk = FALSE;
    }
    else
    {
        printf("STBOOT initialized,\trevision=%s\n",STBOOT_GetRevision());
    }
    return(RetOk);
} /* End of BOOT_Init() function */
#endif  /* !LINUX */

/*******************************************************************************
Name        : BOOT_Term
Description : Terminate the BOOT driver
Parameters  : None
Assumptions :
Limitations :
Returns     : none
*******************************************************************************/
#if !defined ST_OSLINUX
void BOOT_Term(void)
{
    ST_ErrorCode_t      ErrorCode = ST_NO_ERROR;
    STBOOT_TermParams_t TermParams;

    ErrorCode = STBOOT_Term( STBOOT_DEVICE_NAME, &TermParams);

    if (ErrorCode == ST_NO_ERROR)
    {
        printf("STBOOT_Term(%s): ok\n", STBOOT_DEVICE_NAME);
    }
    else
    {
        printf("STBOOT_Term(%s): error 0x%0x\n", STBOOT_DEVICE_NAME, ErrorCode);
    }
} /* end BOOT_Term() */
#endif  /* !LINUX */



/*-------------------------------------------------------------------------
 * Function : Init
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static BOOL Init(void)
{
    BOOL    NoError = TRUE;

#ifdef ST_OS21
/* This is to avoid a compiler warning */
    InternalBlockNoinit[0] = 0;
    SystemBlockNoinit[0] = 0;

    SystemPartition_p = partition_create_heap((void*) ExternalBlock, SYSTEM_SIZE);
    NCachePartition_p = partition_create_heap((void*)NCacheMemory, NCACHE_SIZE);

#if defined (mb411) || defined (mb519)
    /*  no DataPartition_p, we will allocate buffer to load streams in the STAVMEM_SYS_MEMORY in vid_util.c instead of memory allocate in DataPartition_p */
#else /* not mb411 */
    DataPartition_p = partition_create_heap((void *) DataBlock, DATA_MEMORY_SIZE);
#endif /* mb411 */

    ncache_partition    = NCachePartition_p;
    system_partition    = SystemPartition_p;
    DriverPartition_p   = SystemPartition_p;

    if (NoError)
        NoError = BOOT_Init();
    if (NoError)
        NoError = TBX_Init();


#elif defined(ST_OS20)

/* This is to avoid a compiler warning */
    InternalBlockNoinit[0] = 0;
    SystemBlockNoinit[0] = 0;

#if defined(mb391)
    DataBlock[0]=0;
#endif

    InternalPartition_p = &TheInternalPartition;
    partition_init_simple(InternalPartition_p, (void *) InternalBlock, sizeof(InternalBlock));

    SystemPartition_p = &TheExternalPartition;
    partition_init_heap(SystemPartition_p, (void *) ExternalBlock, sizeof(ExternalBlock));

    NCachePartition_p = &TheNCachePartition;
    partition_init_heap(NCachePartition_p, (void *)NCacheMemory, sizeof(NCacheMemory));

    DataPartition_p = &TheDataPartition;

#if defined(mb391)
    partition_init_heap(DataPartition_p, (void *) NCacheMemory, sizeof(NCacheMemory));
#else
    partition_init_heap(DataPartition_p, (void *) DataBlock, sizeof(DataBlock));
#endif

    internal_partition  = InternalPartition_p;
    ncache_partition    = NCachePartition_p;
    system_partition    = SystemPartition_p;
    DriverPartition_p   = SystemPartition_p;


    if (NoError)
        NoError = BOOT_Init();
    if (NoError)
        NoError = TBX_Init();



#elif defined (ST_OSLINUX)
    NoError = (STOSTEST_Init() == STOS_SUCCESS);


#else
    NoError = FALSE;

#endif

    return NoError;
}

/*-------------------------------------------------------------------------
 * Function : Term
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void Term(void)
{
#ifdef ST_OSLINUX
    STOSTEST_Term();
#else
    TBX_Term();
    BOOT_Term();
#endif
}

/*-------------------------------------------------------------------------
 * Function : Generic_Menu
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void Generic_Menu(char * TestName, TestFunction_t * TestFunctions_p, U32 NbFunctions)
{
    STOS_Error_t    Error = STOS_SUCCESS;
    STOS_Error_t    ErrorAllTests = STOS_SUCCESS;
    BOOL            RelevantChoice;
    BOOL            LoopAllTests = FALSE;
    U32             IndexAllTests = 0;
    U32             CurrentTestIndex = 0;
    U8              choice = 0;
    U32             i;

    do
    {
        if (! LoopAllTests)
        {
            /* Not looping: print menu and get user choice */
            if (choice != '\n')
            {
                printf("-------------------------------------------------\n");
                printf("-  0: ALL TESTS for %s                 -\n", TestName);

                /* Prints test list */
                for (i=0 ; i<NbFunctions ; i++)
                {
                    printf("-  %c: %s     -\n", TestFunctions_p[i].Choice, TestFunctions_p[i].Description);
                }

                printf("-                                               -\n");
                printf("-  q: quit                                      -\n");
                printf("-------------------------------------------------\n");
                printf("Please select test number\n\n");
            }
            choice = getchar();
        }
        else
        {
            /* Looping: goes to next listed test */
            if (choice == '0')
            {
                /* First time we enter the loop */
                /* set index to first test      */
                IndexAllTests = 0;
                ErrorAllTests = STOS_SUCCESS;
            }

            if (IndexAllTests < NbFunctions)
            {
                choice = TestFunctions_p[IndexAllTests++].Choice;
            }
            else
            {
                LoopAllTests = FALSE;
                choice = 0;
                printf("\n\n--------------- ALL TESTS result: %s ---------------\n\n", (ErrorAllTests == STOS_SUCCESS) ? "OK" : "Errors !!!");
            }
        }

        RelevantChoice = FALSE;
        for (i=0 ; i<NbFunctions ; i++)
        {
            /* Checking choice against available tests */
            if (choice == TestFunctions_p[i].Choice)
            {
                RelevantChoice = TRUE;
                CurrentTestIndex = i;
                break;
            }
        }

        if (! RelevantChoice)
        {
            switch(choice)
            {
                case '0':
                    printf("---------------- Starting ALL TESTS ----------------\n\n");
                    RelevantChoice = FALSE;
                    LoopAllTests = TRUE;
                    break;

                case 'q':
                    printf("Exiting memory test menu ...\n");
                    RelevantChoice = FALSE;
                    break;

                case '\n':
                default:
                    RelevantChoice = FALSE;
                    break;
            }
        }

        if (RelevantChoice)
        {
            /* User choice has been found, run associated test */
            Error = TestFunctions_p[CurrentTestIndex].Function();
            ErrorAllTests = (Error == STOS_SUCCESS) ? ErrorAllTests : STOS_FAILURE;

            printf("\n ==> Test result: \t\t%s\n", (Error == STOS_SUCCESS) ? "OK" : "Errors !!!");

#ifdef ST_OSLINUX
            Error = STOS_FAILURE;
            for (i=0 ; i<sizeof(Generic_List)/sizeof(STOSTEST_Generic_List_t) ; i++)
            {
                if (Generic_List[i].Generic_Function == TestFunctions_p[CurrentTestIndex].Function)
                {
                    Error = KERNEL_Generic_Test(Generic_List[i].Ioctl_Code);
                    break;
                }
            }
            ErrorAllTests = (Error == STOS_SUCCESS) ? ErrorAllTests : STOS_FAILURE;

            printf(" ==> KERNEL part test result: \t%s\n\n", (Error == STOS_SUCCESS) ? "OK" : "Errors !!!");
#endif
            printf("\n");
        }
    } while(choice != 'q');

    printf("\n");
}

/*-------------------------------------------------------------------------
 * Function : Start
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/
static void Start(void *ptr)
{
    U8  choice = 0;

    UNUSED_PARAMETER(ptr);

    if (!Init())
    {
        return;
    }

    do
    {
        if (choice != '\n')
        {
            printf("---------------------------------\n");
            printf("-  1: Memory tests              -\n");
            printf("-  2: Tasks tests               -\n");
            printf("-  3: Regions tests             -\n");
            printf("-  4: Semaphores tests          -\n");
            printf("-  5: Mutexes tests             -\n");
            printf("-  6: Message queues            -\n");
            printf("-  7: Time tests                -\n");
            printf("-                               -\n");
            printf("-  q: quit                      -\n");
            printf("---------------------------------\n");
            printf("Please select test number\n\n");
        }
        choice = getchar();

        switch(choice)
        {
            case '1':  Generic_Menu("MEMORY     ", MemoryTestFunctions, sizeof(MemoryTestFunctions)/sizeof(TestFunction_t)); break;
            case '2':  Generic_Menu("TASKS      ", TasksTestFunctions, sizeof(TasksTestFunctions)/sizeof(TestFunction_t)); break;
            case '3':  Generic_Menu("REGIONS    ", RegionsTestFunctions, sizeof(RegionsTestFunctions)/sizeof(TestFunction_t)); break;
            case '4':  Generic_Menu("SEMAPHORES ", SemaphoresTestFunctions, sizeof(SemaphoresTestFunctions)/sizeof(TestFunction_t)); break;
            case '5':  Generic_Menu("MUTEXES    ", MutexTestFunctions, sizeof(MutexTestFunctions)/sizeof(TestFunction_t)); break;
            case '6':
            case '7':
                printf("Tests not yet available ...\n");
                break;

            case 'q':
                printf("Exiting test menu ...\n");
                break;

            case '\n':
                break;

            default:
                printf("Wrong test number !!!\n");
                break;

        }
    } while(choice != 'q');

    Term();
} /* end Start */


/*-------------------------------------------------------------------------
 * Function : main
 * Input    :
 * Output   :
 * Return   :
 * ----------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    UNUSED_PARAMETER(argc);
    UNUSED_PARAMETER(argv);

#ifdef ST_OS21
#ifdef ST_OSWINCE
    SetFopenBasePath("/stbgr-prj-stos/tests/src/objs/ST40");
#endif

    printf("\nBOOT ...\n");
    setbuf(stdout, NULL);
#endif
    Start(NULL);

    printf ("\n --- End of main --- \n");
    fflush (stdout);

    exit (0);
}

/* end of os_test.c */
