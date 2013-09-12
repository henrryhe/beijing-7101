 /****************************************************************************

File Name   : stclkpti.c

Description : Clock Recovery API Standard Test Routines using STPTI

Copyright (C) 2007, ST Microelectronics

References  :

$ClearCase (VOB: stclkrv)

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 5.0

****************************************************************************/


/* Includes --------------------------------------------------------------- */
#if defined (ST_5301)
#include <machine/bsp.h>
#endif
#include <stdlib.h>
#include <stdio.h>        /* Needed for print if STTBX fails to init */
#include <string.h>

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include "stboot.h"
#include "stlite.h"
#include "sttbx.h"
#include "stsys.h"
#endif

#include "stos.h"
#include "stdevice.h"
#include "stclkrv.h"
#include "stevt.h"
#include "stpio.h"

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_5188) && !defined (ST_7200)
#include "stmerge.h"
#endif

#if !defined (ST_OSLINUX)
#define CLKRV_MAX_SAMPLES  5000
extern U32 ClkrvDbg[13][CLKRV_MAX_SAMPLES];
extern U32 SampleCount;
#endif

/* Private Constants ------------------------------------------------------ */

#ifndef PCR_PID              /* This can be overriden on the command line */
#define PCR_PID 8190         /* mux1_188.trp  */
#endif

/* Event message queue */
#define EVENT_QUEUE_SIZE            5

#define DELAY_PERIOD                ST_GetClocksPerSecond()
#define TICKS_PER_SECOND            ST_GetClocksPerSecond()

#define START_OF_TS_PACKET          0x47
#define TEST_SYNC_LOCK              1
#define TEST_SYNC_DROP              3


/* Local error code */
#define LOCAL_ERROR_FREERUN_INC     0xFFFFFF01
#define LOCAL_ERROR_INTERRUPT_STC   0xFFFFFF02
/* Freerun test duration in minutes */
#define FREERUN_TEST_MINS           1



/* PTI */
                                    /* Similarity is the key */
#define TEST_PTI_BASE_ADDRESS       (U32*)PTI_BASE_ADDRESS
#define TEST_PTI_INTERRUPT          PTI_INTERRUPT


#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_5188)
/* TSMERGER */
#define TEST_TSMERGE_BASE_ADDRESS   TSMERGE_BASE_ADDRESS
#endif


#if defined (ST_5100) || defined (ST_5301)
#define STMERGE_PTI                 STMERGE_PTI_0
#define TEST_TSIN                   STMERGE_TSIN_0
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
#define STMERGE_PTI                 STMERGE_PTI_0
#define TEST_TSIN                   STMERGE_TSIN_0
#endif

/* Test clock recovery init params defines */
#define TEST_PCR_DRIFT_THRES      400
#define TEST_MIN_SAMPLE_THRES     10
#define TEST_MAX_WINDOW_SIZE      50

#define CLK_REC_TRACKING_INTERRUPT_LEVEL 7

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) || defined (ST_5188)
#define CLK_REC_TRACKING_INTERRUPT    DCO_CORRECTION_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_5100
#define AUD_PCM_CLK_FS_BASE_ADDRESS   NULL
#elif defined (ST_7710)
#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7710
#define AUD_PCM_CLK_FS_BASE_ADDRESS   AUDIO_IF_BASE_ADDRESS
#elif defined (ST_7100)
#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7100
#define AUD_PCM_CLK_FS_BASE_ADDRESS   AUDIO_IF_BASE_ADDRESS
#elif defined (ST_7109)
#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7100
#define AUD_PCM_CLK_FS_BASE_ADDRESS   AUDIO_IF_BASE_ADDRESS
#elif defined (ST_7200)
#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7200
#define AUD_PCM_CLK_FS_BASE_ADDRESS   AUDIO_IF_BASE_ADDRESS
#endif

#define MAX_SLAVE_CLOCK 2

#define MASTER_CLK_AME  "SD/STC"
#define SLAVE1_CLK_NAME "PCM"

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined(ST_5107)
#define SLAVE2_CLK_NAME "SPDIF"
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109)
#define SLAVE2_CLK_NAME "HD"
#endif

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE     (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE       0x0200000
#define NCACHE_PARTITION_SIZE       0x100000

/* Private Types ---------------------------------------------------------- */

/* Pass/fail counts */
typedef struct
{
    U32 NumberPassed;
    U32 NumberFailed;
} CLKRV_TestResult_t;


/* Private Variables ------------------------------------------------------ */

static const U8  HarnessRev[] = "5.4.0";

/* Declarations for memory partitions */
#ifdef ST_OS20
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
#pragma ST_section      ( system_block,   "system_section")

static U8               ncache_block [NCACHE_PARTITION_SIZE];
static partition_t      the_ncache_partition;
#pragma ST_section      ( ncache_block,   "ncache_section")

static U8               system_noinit_block [1];
static partition_t      the_system_noinit_partition;
partition_t             *system_noinit_partition = &the_system_noinit_partition;
#pragma ST_section      ( system_noinit_block, "system_section_noinit")

static U8               internal_noinit_block [1];
static partition_t      the_internal_noinit_partition;
partition_t             *noinit_partition = &the_internal_noinit_partition;
#pragma ST_section      ( internal_noinit_block, "internal_section_noinit")

/* To avoid warning */
#if defined(ST_5100) || defined (ST_7710) || defined (ST_5105) || defined(ST_5107) || defined (ST_5188)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif


/* Partition pointers for PTI usage */
ST_Partition_t          *ncache_partition = &the_ncache_partition;
ST_Partition_t          *system_partition = &the_system_partition;
#elif defined(ST_OS21) /* ifdef ST_OS20 */


static unsigned char    system_block[SYSTEM_PARTITION_SIZE];
partition_t             *system_partition;
static unsigned char    ncache_block[NCACHE_PARTITION_SIZE];
partition_t             *ncache_partition;

#endif /* ST_OS21 */

/* Memory partitions */
#if defined(ST_OS20) || defined(ST_OS21) /* Linux changes */
#define TEST_PARTITION_1        system_partition
#define TEST_PARTITION_2        ncache_partition

#else /* Linux */

#define TEST_PARTITION_1         NULL
#define TEST_PARTITION_2         NULL

#endif
/* PTI */
/*static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_NOTAGS;*/
#if !defined(ST_5188)
#ifdef ST_7200
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_TSIN0;
#else
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_NOTAGS;
#endif
#endif

/* Test variables */
/* Pass/fail counts */
static CLKRV_TestResult_t Result;
static BOOL Passed = (U32)FALSE;

/* Following are declared globals for stpti initialization */
#if defined(DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)
ST_DeviceName_t       STPTI_DeviceName;
STPTI_InitParams_t    STPTIInitParams;
ST_DeviceName_t       PTIEVTDevNam;
#endif
#if defined(ST_5188)
ST_DeviceName_t       STDEMUX_DeviceName;
STDEMUX_InitParams_t  STDEMUXInitParams;
ST_DeviceName_t       DEMUXEVTDevNam;
#endif

static ST_ErrorCode_t          retVal;
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
static semaphore_t             *event_counter_p;
#endif
static ST_DeviceName_t         EVTDevNam;
static ST_DeviceName_t         DeviceNam;

static STCLKRV_TermParams_t    TermParams_s;

static STEVT_TermParams_t      EVTTermPars;
static STEVT_InitParams_t      EVTInitPars;
static STEVT_OpenParams_t      EVTOpenPars;
static STEVT_Handle_t          EVTHandle;

static STEVT_EventConstant_t EventsRxd[EVENT_QUEUE_SIZE];
static STEVT_EventConstant_t *pReadEventRxd = EventsRxd;
static STEVT_EventConstant_t *pWriteEventRxd = EventsRxd;
static U32 EventsDisplayed = 0;

#ifndef RESOURCE_USAGE_TESTS

static STCLKRV_OpenParams_t    CLKRVOpenParams;
static STCLKRV_Handle_t        Handle;
static STCLKRV_InitParams_t    CLKRVInitParams;

/* Track events registered by STCLKRV */
#if !defined(ST_OSLINUX)
static U32 InterruptSTC = 0;
static STCLKRV_ExtendedSTC_t InterruptExtSTC;
static BOOL GetSTCInterruptTestOK;
static BOOL GetExtSTCInterruptTestOK;

static task_t *DisplayTask_p;
static tdesc_t DisplayTaskDescriptor;
static void *DisplayTaskStack_p;
#endif

#ifdef ST_OS20
#define DISPLAY_EVENTS_STACK_SIZE 4*256
static task_t *GetSTCInterruptTestTask_p;
static tdesc_t GetSTCInterruptTestTaskDescriptor;
static void *GetSTCInterruptTestTaskStack_p;

static task_t *GetExtSTCInterruptTestTask_p;
static tdesc_t GetExtSTCInterruptTestTaskDescriptor;
static void *GetExtSTCInterruptTestTaskStack_p;
#endif

#if defined(ST_OS21)
#define DISPLAY_EVENTS_STACK_SIZE 17*1024
static task_t *DisplayTask_p;
static task_t *GetSTCInterruptTestOSXTask_p;
#endif

#endif

/* globals identifying Slaves on various boards */

/* 5100     7710
   PCM      PCM
   SPDIF    HD */

#ifndef RESOURCE_USAGE_TESTS
#if defined (ST_5100) || defined (ST_5105) || defined(ST_5301) || defined(ST_5107)
static STCLKRV_ClockSource_t TestSlaveClock[2] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_SPDIF_0};
U32 TestSlaveFrequency[2] = { 11300000, 45158400};
#elif defined (ST_7710)
static STCLKRV_ClockSource_t TestSlaveClock[2] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_HD_0};
U32 TestSlaveFrequency[2] = { 11300000, 148500000};
#elif defined (ST_7100) || defined (ST_7109)
static STCLKRV_ClockSource_t TestSlaveClock[4] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_HD_0, STCLKRV_CLOCK_PCM_1, STCLKRV_CLOCK_SPDIF_0 };
U32 TestSlaveFrequency[4] = { 11300000, 148500000, 3072000, 67737600};
#elif defined (ST_5188)
#define FEI_SOFT_RST                  (FEI_BASE_ADDRESS + 0x00000000 )
#define FEI_ENABLE                    (FEI_BASE_ADDRESS + 0x00000008 )
#define FEI_CONFIG                    (FEI_BASE_ADDRESS + 0x00000010 )
#define FEI_STATUS                    (FEI_BASE_ADDRESS + 0x00000018 )
#define FEI_STATUS_CLR                (FEI_BASE_ADDRESS + 0x00000020 )
#define FEI_OUT_DATA                  (FEI_BASE_ADDRESS + 0x00000028 )
#define FEI_OUT_ERROR                 (FEI_BASE_ADDRESS + 0x00000030 )
#define FEI_SECTOR_SIZE               (FEI_BASE_ADDRESS + 0x00000038 )
static STCLKRV_ClockSource_t TestSlaveClock[2] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_SPDIF_0};
U32 TestSlaveFrequency[2] = { 11300000, 45158400};
#elif defined (ST_7200)
static STCLKRV_ClockSource_t TestSlaveClock[7] = { STCLKRV_CLOCK_PCM_0,
                                                     STCLKRV_CLOCK_PCM_1,
                                                     STCLKRV_CLOCK_PCM_2,
                                                     STCLKRV_CLOCK_PCM_3,
                                                     STCLKRV_CLOCK_SPDIF_HDMI_0,
                                                     STCLKRV_CLOCK_SPDIF_0,
                                                     STCLKRV_CLOCK_HD_0};
U32 TestSlaveFrequency[7] = { 3072000, 11300000, 12300000, 45158400, 50400000, 67737600, 148500000};
#endif
#endif

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */
static void STCLKRVT_EventCallback( STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData,
                                    const void *SubscriberData_p);

static void display_events( void *pDummy );

static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr );

void PrintPassFail(BOOL Passed, char *PassMsg, char *FailMsg);

#ifndef RESOURCE_USAGE_TESTS

#ifndef ST_5188
void TestPcrWithSTPTI(void);
static void TestSTCBaselineWithSTPTI(void);
#else
void TestPcrWithSTDEMUX(void);
static void TestSTCBaselineWithSTDEMUX(void);
#endif

void STCBaselineTest1(STCLKRV_Handle_t Handle,
                      STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError);

void STCBaselineTest2(STCLKRV_Handle_t Handle,
                      STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError);

void STCBaselineTest3(STCLKRV_Handle_t Handle,
                      STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError);

void STCBaselineTest4(STCLKRV_Handle_t Handle,
                      STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError);

void STCBaselineTest5(STCLKRV_Handle_t Handle,
                      STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError);

void STCBaselineTest6(STCLKRV_Handle_t Handle,
                      ST_ErrorCode_t *StoredError);

#ifdef ST_OS20
static void GetSTCInterruptTest(void *NotUsed);
static void GetSTCInterruptTestTaskFunc(void *NotUsed);

static void GetExtSTCInterruptTest(void *NotUsed);
static void GetExtSTCInterruptTestTaskFunc(void *NotUsed);

#elif defined(ST_OS21)

static void GetSTCInterruptTestOSXTaskFunc(void *NotUsed);
int GetSTCInterruptTestOSX(void *NotUsed);

#endif

/* Prototypes for resource usage test code */
#else
static BOOL StackTest_Main(void);
#endif

/* Exproted functions */
extern ST_ErrorCode_t STCLKRV_GetClocksFrequency( STCLKRV_Handle_t Handle,
                                                U32 *pSTCFrequency,
                                                U32 *pFS1Frequency,
                                                U32 *pFS2Frequency,
                                                U32 *pFS3Frequency,
                                                U32 *pFS4Frequency,
                                                U32 *pFS5Frequency,
                                                U32 *pFS6Frequency );

void clkrv_ChangeFSClock( STCLKRV_ClockSource_t Clock,
                          S32 Control );


/* Private Variables ------------------------------------------------------ */

static const STEVT_DeviceSubscribeParams_t EVTSubscrParams ={
                                     STCLKRVT_EventCallback, NULL};



/* Functions -------------------------------------------------------------- */
#if defined(ST_5301)
/****************************************************************************/
/**
 * @brief Turns off all caching of LMI
 *
 * Turns off caching of LMI from 0xc000_0000 to 0xC200_0000
 *
 ****************************************************************************/
void DCacheOff(void)
{
          bsp_mmu_memory_unmap( (void *)(0xC0000000), 0x02000000);

          bsp_mmu_memory_map((void *)(0xc0000000),
                           0x02000000,
                           PROT_USER_EXECUTE|
                           PROT_USER_WRITE|
                           PROT_USER_READ|
                           PROT_SUPERVISOR_EXECUTE|
                           PROT_SUPERVISOR_WRITE|
                           PROT_SUPERVISOR_READ,
                           MAP_SPARE_RESOURCES,
                           (void *)(0xc0000000));

}
#endif

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : void

Return Value : int

See Also     : None
 ****************************************************************************/
int main( void )
{
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STTBX_InitParams_t      sttbx_InitPars;
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    STBOOT_DCache_Area_t    Cache[] =
    {
#ifndef DISABLE_DCACHE

#if defined (mb314) || defined(mb361) || defined(mb382)

        { (U32 *) CACHEABLE_BASE_ADDRESS, (U32 *) CACHEABLE_STOP_ADDRESS },

#else

        /* these probably aren't correct,
          but were used already and appear to work */
        { (U32 *) 0x40080000, (U32 *) 0x4FFFFFFF },

#endif

#endif
        { NULL, NULL }
    };

/* PoKe required for configuring STEM 499 for injection of
   TS from Packet Injector */
#if defined (ST_5100)
#if defined TEST_STCLKRV_5100_WITH_STEM
    *(volatile U32*)(0x41800000) = 0x00080008;
#endif
#endif

#endif /* LINUX CHANGES */

#if defined (ST_7100)  || defined (ST_7109) || defined (ST_7200)
    STPIO_InitParams_t      PIOInitParams;
    STPIO_OpenParams_t      PIOOpenParams;
    STPIO_Handle_t          PIOHandle;
    STPIO_TermParams_t      PIOTermParams;
#endif

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#ifdef ST_OS21
    /* Create memory partitions */
    system_partition   =  partition_create_heap((U8*)system_block, sizeof(system_block));
#ifdef ARCHITECTURE_ST200
    ncache_partition   =  partition_create_heap((U8*)(ncache_block),sizeof(ncache_block));
#else
    ncache_partition   =  partition_create_heap((U8*)ST40_NOCACHE_NOTRANSLATE(ncache_block),
                                                                         sizeof(ncache_block));
#endif
#else
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));
    partition_init_heap (&the_ncache_partition,   ncache_block,
                         sizeof(ncache_block));
    partition_init_heap (&the_system_noinit_partition,   system_noinit_block,
                         sizeof(system_noinit_block));
    partition_init_heap (&the_internal_noinit_partition, internal_noinit_block,
                         sizeof(internal_noinit_block));
#endif
/* Avoid compiler warnings */
#if defined(ST_7710) || defined(ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5188)
    data_section[0]  = 0;
#endif

#if defined(ST_5301)
    DCacheOff();
#endif

    /* Initialize stboot */
#ifdef DISABLE_ICACHE
    stboot_InitPars.ICacheEnabled             = FALSE;
#else
    stboot_InitPars.ICacheEnabled             = TRUE;
#endif
#ifdef ST_OS21
    stboot_InitPars.TimeslicingEnabled        = TRUE;
#endif
    stboot_InitPars.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitPars.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
    stboot_InitPars.DCacheMap                 = Cache;
    stboot_InitPars.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    retVal = STBOOT_Init( "CLKRVTest", &stboot_InitPars );
    if( retVal != ST_NO_ERROR )
    {
        STTBX_Print(( "ERROR: STBOOT_Init() returned %d\n", retVal ));
    }
    else
    {
        /* Initialize the toolbox */
        sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

        sttbx_InitPars.CPUPartition_p = TEST_PARTITION_1;

        retVal = STTBX_Init( "tbx", &sttbx_InitPars );
        if( retVal != ST_NO_ERROR )
        {
            printf( "ERROR: STTBX_Init() returned %d\n", retVal );
            return -1;
        }
    }
#endif /* Linux Changes */

#ifndef RESOURCE_USAGE_TESTS
    {
#if defined (ST_7710)
        /* monitor clocks */
        volatile U32 *addptr = (U32 *)0x380000C8;
        U32 regval = 0x5AA50000;
        U32 regval2 = 0;
        regval2 = *addptr;

        *addptr = (regval2 | regval | 0x100);
#else
#if defined (ST_5105) || defined (ST_5107)

        volatile U32 *addptr = (U32 *)0x20F00300;




        /* Unlock system services registers */
        *addptr = 0xF0;
        *addptr = 0x0F;

#if defined (ST_5105) || defined (ST_5107)
        addptr = (U32 *)0x20402000;
        *addptr = 0x800000;
#endif

        /* FOR debugging any Clock at AUX clk */
        /* 0x29 for FS0, 0x2A for PCM(FS0)*/
        addptr = (U32 *)0x20F00188;
        *addptr = 0x2D;
#endif

#endif
    }

#if defined (ST_7100)  || defined (ST_7109) || defined (ST_7200)

    /* To view clocks on the PIO5 pin 2 */
#if defined (ST_7100)
    PIOInitParams.BaseAddress      = (U32*)ST7100_PIO5_BASE_ADDRESS;
    PIOInitParams.InterruptNumber  = ST7100_PIO5_INTERRUPT;
#elif defined (ST_7109)
    PIOInitParams.BaseAddress      = (U32*)ST7109_PIO5_BASE_ADDRESS;
    PIOInitParams.InterruptNumber  = ST7109_PIO5_INTERRUPT;
#elif defined (ST_7200)
    PIOInitParams.BaseAddress      = (U32*)ST7200_PIO5_BASE_ADDRESS;
    PIOInitParams.InterruptNumber  = ST7200_PIO5_INTERRUPT;
#endif


    PIOInitParams.InterruptLevel   = 2;
    PIOInitParams.DriverPartition  = TEST_PARTITION_1;

    retVal = STPIO_Init( "PIO5", &PIOInitParams );
    if (retVal!= ST_NO_ERROR )
    {
        STCLKRV_Print (("Error: STPIO_Init = 0x%08X\n", retVal ));
    }
    else
    {
        PIOOpenParams.IntHandler       =  NULL;
#ifdef ST_7200
        PIOOpenParams.BitConfigure[5]  =  STPIO_BIT_ALTERNATE_OUTPUT;
        PIOOpenParams.ReservedBits     =  0x20;
#else
        PIOOpenParams.BitConfigure[2]  =  STPIO_BIT_ALTERNATE_OUTPUT;
        PIOOpenParams.ReservedBits     =  0x04;
#endif

        retVal = STPIO_Open ( "PIO5",&PIOOpenParams, &PIOHandle );
        if (retVal!= ST_NO_ERROR )
        {
            STCLKRV_Print (("Error: STPIO_Open = 0x%08X\n", retVal ));
        }
    }
#endif /* #if defined (ST_7100)  || defined (ST_7109) || defined (ST_7200) */

#ifndef ST_5188
    TestPcrWithSTPTI();
    TestSTCBaselineWithSTPTI();
#else
    TestPcrWithSTDEMUX();
    TestSTCBaselineWithSTDEMUX();
#endif

#if defined (ST_7100)  || defined (ST_7109) || defined (ST_7200)
    PIOTermParams.ForceTerminate = FALSE;
    retVal = STPIO_Term("PIO5", &PIOTermParams);
#endif

#else  /* execute resource usage tests */
    StackTest_Main();
#endif

#ifndef ST_OSLINUX
    retVal = STBOOT_Term( "CLKRVTest", &stboot_TermPars );
#endif

    return retVal;
}


#if defined(DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)

/***********************************************************************************
* Name:     stpti4_InitSetup()
*
* Description:  Initialize stpti4 init params according to chip type
***********************************************************************************/
static void stpti4_InitSetup(STPTI_InitParams_t * STPTIInitPars_p)
{
    /* Initialise STPTI4 */
    strcpy( STPTI_DeviceName, "STPTI");
    STPTIInitPars_p->Device                     = STPTI_DEVICE_PTI_4;
#ifdef STPTI_DTV_SUPPORT
    STPTIInitPars_p->TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV;
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STPTIInitPars_p->TCLoader_p                 = STPTI_DTVTCLoader;
#endif
    STPTIInitPars_p->SyncLock                   = 0;
    STPTIInitPars_p->SyncDrop                   = 0;
    STPTIInitPars_p->DiscardSyncByte            = 0;
#else
    STPTIInitPars_p->TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#if defined(ST_5105) || defined (ST_5107)
    STPTIInitPars_p->TCLoader_p                 = STPTI_DVBTCLoaderUL;
#elif defined(ST_7109)
    STPTIInitPars_p->TCLoader_p                 = STPTI_DVBTCLoaderSL;
#elif defined(ST_7100) || defined(ST_5100)
    STPTIInitPars_p->TCLoader_p                 = STPTI_DVBTCLoaderL;
#elif defined(ST_7200)
    STPTIInitPars_p->TCLoader_p                 = STPTI_DVBTCLoaderSL2;
#else
    STPTIInitPars_p->TCLoader_p                 = STPTI_DVBTCLoader;
#endif
#endif
    STPTIInitPars_p->SyncLock                   = 0;
    STPTIInitPars_p->SyncDrop                   = 0;
#endif
    STPTIInitPars_p->TCDeviceAddress_p          = TEST_PTI_BASE_ADDRESS;
    STPTIInitPars_p->SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_8x8; /* ??? */
    STPTIInitPars_p->InterruptLevel             = PTI_INTERRUPT_LEVEL;
    STPTIInitPars_p->InterruptNumber            = TEST_PTI_INTERRUPT;

    STPTIInitPars_p->NCachePartition_p          = TEST_PARTITION_2;
    STPTIInitPars_p->DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitPars_p->Partition_p                = TEST_PARTITION_1;
    STPTIInitPars_p->EventProcessPriority       = MAX_USER_PRIORITY-1;
    STPTIInitPars_p->InterruptProcessPriority   = MAX_USER_PRIORITY;
#ifdef STPTI_INDEX_SUPPORT
    STPTIInitPars_p->IndexAssociation           = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitPars_p->IndexProcessPriority       = MIN_USER_PRIORITY;
#endif

#ifdef STPTI_CAROUSEL_SUPPORT
    STPTIInitPars_p->CarouselProcessPriority    = MIN_USER_PRIORITY;
    STPTIInitPars_p->CarouselEntryType          = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif

    STPTIInitPars_p->StreamID                   = STPTI_StreamID;
#if defined (ST_7200)
    STPTIInitPars_p->PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
#else
    STPTIInitPars_p->PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_PTI;
#endif
    STPTIInitPars_p->NumberOfSlots              = 10;
    STPTIInitPars_p->AlternateOutputLatency     = 42;

    memcpy (&(STPTIInitPars_p->EventHandlerName), PTIEVTDevNam, sizeof (ST_DeviceName_t));
}


/***********************************************************************************
* Name:     CLKRV_STPTI_InitSetup
*
* Description:  Initialize stpti init params according to chip type
***********************************************************************************/

static void CLKRV_STPTI_InitSetup(STPTI_InitParams_t * STPTIInitPars_p)
{
    memset(STPTIInitPars_p, 0, sizeof(STPTI_InitParams_t));
    stpti4_InitSetup(STPTIInitPars_p);
}

#endif /* STPTI INIT */


/* Internal switch for building main test harness or resource usage code */
#ifndef RESOURCE_USAGE_TESTS

#ifdef ST_5188
static void  FEI_Setup(void)
{
    STSYS_WriteRegDev32LE (0x20402000, 0x00200000);  /*for 5188 */

    STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x00);

    STSYS_WriteRegDev32LE (FEI_CONFIG, 0x5E); /*Bit 6 set */

    STSYS_WriteRegDev32LE (FEI_SECTOR_SIZE, 0x00);

    STSYS_WriteRegDev32LE (FEI_SOFT_RST, 0x01);

}
/*-------------------------------------------------------------------------
 * Function : STAPP_FdmaSetup
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
static ST_ErrorCode_t FdmaSetup(void)
{
    ST_ErrorCode_t       ST_ErrorCode;
    STFDMA_InitParams_t  STFDMA_InitParams;

    STFDMA_InitParams.DeviceType          = STFDMA_DEVICE_FDMA_2;
    STFDMA_InitParams.DriverPartition_p   = TEST_PARTITION_1;
    STFDMA_InitParams.NCachePartition_p   = TEST_PARTITION_2;
    STFDMA_InitParams.BaseAddress_p       = (void *)FDMA_BASE_ADDRESS;
    STFDMA_InitParams.InterruptNumber     = FDMA_INTERRUPT;
    STFDMA_InitParams.InterruptLevel      = 14;
    STFDMA_InitParams.NumberCallbackTasks = 1;
    STFDMA_InitParams.FDMABlock           = STFDMA_1;
    STFDMA_InitParams.ClockTicksPerSecond = ST_GetClocksPerSecond();

    ST_ErrorCode = STFDMA_Init("FDMA0", &STFDMA_InitParams);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        return( ST_ErrorCode );
    }
    STTBX_Print(("%s\n", STFDMA_GetRevision() ));

    return( ST_ErrorCode );
}
#if 0
#define LOG_FILE "../../clkrv_enc_log.csv"
FILE *clk_p=NULL;
#endif
void TestPcrWithSTDEMUX()
{
    STCLKRV_ExtendedSTC_t     ExtendedSTC;
    ST_ErrorCode_t            StoredError = ST_NO_ERROR;
    ST_ErrorCode_t            ReturnError;
    ST_Revision_t             RevisionStr;
    U32 STCFreq, FS1Freq,     FS2Freq, FS3Freq, FS4Freq, FS5Freq;            /* returned FS Frequency setting */
    STDEMUX_OpenParams_t      STDEMUXOpenParams;
    STDEMUX_TermParams_t      STDEMUXTermParams;
    STDEMUX_SlotData_t        SlotData;
    STDEMUX_Slot_t            PcrSlot;
    STDEMUX_StreamParams_t    STDEMUXStreamParams;
    STDEMUX_Handle_t          DEMUXSessionHandle;
    STEVT_InitParams_t        DEMUXEVTInitPars;
    U32                       PacketCount = 0;
    STCLKRV_SourceParams_t    PCRSrc;
    U32 STC;
    U32 StartTime;

    U32 i = 0;

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    Result.NumberPassed = Result.NumberFailed = 0;
    Passed = TRUE;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing TestPcrWithSTDEMUX Test Function for STPTI....\n" ));
    STTBX_Print(( "TEST - FS Clocks behaviour and GetSTC Functionality test\n" ));
    STTBX_Print(( "==========================================================\n\n" ));

    /* Name clock recovery driver */
    strcpy( DeviceNam, "STCLKRV" );

    /* Initialize the sub-ordinate EVT Driver */
    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum  = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for clkrv events...\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    event_counter_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Subscribe to PCR Events now */
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    FEI_Setup();
    FdmaSetup();

   /* Initialize the EVT Driver for PTI event handling*/
    strcpy( DEMUXEVTDevNam, "STEVTdemux" );
    DEMUXEVTInitPars.EventMaxNum   = 50;
    DEMUXEVTInitPars.ConnectMaxNum = 50;
    DEMUXEVTInitPars.SubscrMaxNum  = 20; /* Modified from 3 for new EVT handler */
    DEMUXEVTInitPars.MemoryPartition = TEST_PARTITION_1;
    DEMUXEVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for DEMUX\n" ));
    ReturnError = STEVT_Init( DEMUXEVTDevNam, &DEMUXEVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set up the parameters */
    STDEMUXInitParams.Device = STDEMUX_DEVICE_CHANNEL_FEC1;
    STDEMUXInitParams.Parser = STDEMUX_PARSER_TYPE_TS;
    STDEMUXInitParams.Partition_p = TEST_PARTITION_1;
    STDEMUXInitParams.NCachePartition_p = TEST_PARTITION_2;

    strcpy(STDEMUXInitParams.EventHandlerName, DEMUXEVTDevNam);

    STDEMUXInitParams.EventProcessPriority = MAX_USER_PRIORITY;
    STDEMUXInitParams.DemuxProcessPriority = 13;
    STDEMUXInitParams.DemuxMessageQueueSize = 256;
    STDEMUXInitParams.InputPacketSize = 192;
    STDEMUXInitParams.OutputModeBlocking = FALSE;
    STDEMUXInitParams.SyncLock = 1;
    STDEMUXInitParams.SyncDrop = 0;
    STDEMUXInitParams.InputMetaData = STDEMUX_INPUT_METADATA_PRE;

    STDEMUXInitParams.NumOfPcktsInPesBuffer = 150;
    STDEMUXInitParams.NumOfPcktsInRawBuffer = 0;
    STDEMUXInitParams.NumOfPcktsInSecBuffer = 0;

    STTBX_Print(( "Calling STDEMUX_Init() ................\n" ));
    strcpy( STDEMUX_DeviceName, "STDEMUX");
    ReturnError = STDEMUX_Init(STDEMUX_DeviceName, &STDEMUXInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* Open session with STDEMUX */
        STTBX_Print(("Calling STDEMUX_Open()...\n"));
        STDEMUXOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STDEMUXOpenParams.NonCachedPartition_p = TEST_PARTITION_2;
        ReturnError = STDEMUX_Open(STDEMUX_DeviceName, &STDEMUXOpenParams,
                             &DEMUXSessionHandle);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    }


    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotData.SlotType = STDEMUX_SLOT_TYPE_PCR;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.DiscardOnCrcError = FALSE;
        SlotData.SlotFlags.SoftwareCDFifo = FALSE;

        STTBX_Print(( "Calling STDEMUX_SlotAllocate() ..........\n" ));
        ReturnError = STDEMUX_SlotAllocate(DEMUXSessionHandle, &PcrSlot, &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
        /* Set PCR pid */
        STDEMUXStreamParams.member.Pid = PCR_PID;
        STTBX_Print(( "Calling STDEMUX_SlotSetParams(%d) ..........\n", STDEMUXStreamParams.member.Pid ));
        ReturnError = STDEMUX_SlotSetParams(PcrSlot, &STDEMUXStreamParams);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    /* Get some packets */
    STOS_TaskDelay(TICKS_PER_SECOND);

    STDEMUX_GetInputPacketCount(STDEMUX_DeviceName,&PacketCount);
    STTBX_Print(("Input Packet Count :  %i\n", PacketCount));

    STDEMUX_SlotPacketCount(PcrSlot,&PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n", PacketCount));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        /* Output running analysis */
        STTBX_Print(("\n**********************************************************\n"));
        STTBX_Print(("**** STDEMUX TEST!! can not be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
        Result.NumberFailed++;;
        Passed = FALSE;
        goto abortTestPcrWithSTDEMUX;
    }

    Passed =TRUE;

    /* revision string available before STCLKRV Initialized */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Initialize our native Clock Recovery Driver */
    CLKRVInitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres       = TEST_PCR_DRIFT_THRES;
    CLKRVInitParams.MinSampleThres      = TEST_MIN_SAMPLE_THRES;
    CLKRVInitParams.MaxWindowSize       = TEST_MAX_WINDOW_SIZE;
    CLKRVInitParams.Partition_p         = TEST_PARTITION_1;
    CLKRVInitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams.FSBaseAddress_p     = (U32 *)CKG_BASE_ADDRESS;
    CLKRVInitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
    CLKRVInitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitParams.PCREvtHandlerName, DEMUXEVTDevNam );
    strcpy( CLKRVInitParams.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitParams.PTIDeviceName, STDEMUX_DeviceName );

#if defined (ST_5188)
    STSYS_WriteRegDev32LE((void*)(0x20F00188), 0x33);
#endif
    task_delay(ST_GetClocksPerSecond());

    STTBX_Print(( "Calling STCLKRV_Init() ..............\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slaves frequencies*/

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 12288000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1],
                                                 12288000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    STTBX_Print(("Disabling Clock Recovery...\n"));
    ReturnError = STCLKRV_Disable(Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slot ID for STPTI communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STPTI_s.Slot = PcrSlot;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST [0]: Clock recovery Behaviour test\n"));
    STTBX_Print(("Frequency Expected value will vary on different boards\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Report the Initial Frequency Value */
    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
    ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                              &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
    STTBX_Print(( "FS Block Frequencies before enable\n"));
    STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreq, FS1Freq, FS2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /*STTBX_Print(("Enable Clock Recovery...\n\n"));*/
    ReturnError = STCLKRV_Enable(Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Observe FS Block Frequencies after enable\n"));
    STTBX_Print(("(Freq. sampled every 10ms)\n"));

#ifdef CLKRV_DETAIL_TEST
    for (i = 0; i < 5000; i++)
#else
    for (i = 0; i < 50; i++)
#endif
    {
        STDEMUX_SlotPacketCount(PcrSlot,&PacketCount);
        STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                    &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        STTBX_Print(("%12u, %12u, %12u, %12u, %12u, %12u, %12u, %12u, %12u\n",STCFreq, FS1Freq, FS2Freq,
                                                             FS3Freq, FS4Freq, FS5Freq, FS6Freq, PacketCount, i ));
        STOS_TaskDelay(TICKS_PER_SECOND);

    }

#if 0 /* Just to regress more */
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 22579200);
    if (ReturnError!=ST_NO_ERROR)
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    for (i = 0; i < 1; i++)
    {
        STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                    &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        STTBX_Print(("%12u, %12u, %12u, %12u, %12u, %12u, %12u\n",STCFreq, FS1Freq, FS2Freq,
                                                             FS3Freq, FS4Freq, FS5Freq, FS6Freq ));

        STOS_TaskDelay(TICKS_PER_SECOND / 25);
    }
#endif


#if 0
    /* Dump the debug data */
    ReturnError = STCLKRV_Disable(Handle);
    clk_p = fopen(LOG_FILE, "wb");

    for (i = 0; i < SampleCount; i++)
    {

        /*STTBX_Print((" %12u, %12u\n",ClkrvDbg[2][i],ClkrvDbg[3][i]));*/
        fprintf(clk_p, "%u, %u, %u, %u, %d, %d, %d, %d, %d, %d, %d\n",
                                             ClkrvDbg[0][i],ClkrvDbg[2][i],
                                             ClkrvDbg[3][i],ClkrvDbg[1][i],
                                             ClkrvDbg[8][i],ClkrvDbg[4][i],
                                             ClkrvDbg[5][i],ClkrvDbg[6][i],
                                             ClkrvDbg[7][i],ClkrvDbg[9][i],
                                             ClkrvDbg[10][i]);
    }
#endif


    STTBX_Print(("\n-----------------------------------------------------------\n"));
    STTBX_Print(("FS Clocks Behaviour test complete.\n"));
    STTBX_Print(("-----------------------------------------------------------\n\n"));

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST: STCLKRV_GetSTC and STCLKRV_GetExtendedSTC\n"));
    STTBX_Print(("Called from Interrupt context\n"));
    STTBX_Print(("===========================================================\n\n"));

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STTBX_Print(( "Commencing calls from interrupt tests...\n\n" ));

    /* Kick off event monitoring task */
    STOS_TaskCreate((void(*)(void *))display_events,
                                NULL,
                                TEST_PARTITION_1,
                                DISPLAY_EVENTS_STACK_SIZE,
                                &DisplayTaskStack_p,
                                TEST_PARTITION_1,
                                &DisplayTask_p,
                                &DisplayTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "display_task",
                                (task_flags_t)0);
#ifdef ST_OS21
    /* Kick off interrupt context test task */
    STOS_TaskCreate((void(*)(void *))GetSTCInterruptTestOSXTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                ((16*1024)+(4*256)),
                                NULL,
                                TEST_PARTITION_1,
                                &GetSTCInterruptTestOSXTask_p,
                                NULL,
                                MAX_USER_PRIORITY,
                                "GetSTCInterruptTestTask",
                                (task_flags_t)task_flags_no_min_stack_size);

    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetSTCInterruptTestOSXTask_p, TIMEOUT_INFINITY);

#else
    /* Kick off interrupt context test task */
    STOS_TaskCreate((void(*)(void *))GetSTCInterruptTestTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                4*256,
                                &GetSTCInterruptTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetSTCInterruptTestTask_p,
                                &GetSTCInterruptTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetSTCInterruptTestTask",
                                (task_flags_t)0);

    STOS_TaskCreate((void(*)(void *))GetExtSTCInterruptTestTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                4*256,
                                &GetExtSTCInterruptTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetExtSTCInterruptTestTask_p,
                                &GetExtSTCInterruptTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetExtSTCInterrupTestTask",
                                (task_flags_t)0);

    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetExtSTCInterruptTestTask_p, TIMEOUT_INFINITY);

#endif

    STTBX_Print(("\nAssessing calls from interrupt test results....\n"));
    if ((GetSTCInterruptTestOK != TRUE) || (GetExtSTCInterruptTestOK != TRUE))
    {
        Passed = FALSE;
        ErrorReport( &StoredError, LOCAL_ERROR_INTERRUPT_STC, ST_NO_ERROR );
    }
    else
    {
        ErrorReport( &StoredError, ST_NO_ERROR, ST_NO_ERROR );
    }
#endif /* Linux */
    PrintPassFail(Passed, "STC values OK", "Invalid STC values");

    STTBX_Print(( "Commencing calls from task tests.....\n\n" ));

    /* Invalidate clock and receive PCR events... */
    STTBX_Print(( "Calling STCLKRV_InvDecodeClk() ...\n" ));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* wait before calling Get_XXX else you will get NO_PCR_ERROR */
    STOS_TaskDelay(TICKS_PER_SECOND / 10);

    /* Run for period of time */
    /*while( EventsDisplayed < 5 )*/
    StartTime = STOS_time_now();

    /* Enter timed loop...*/
    while ((STOS_time_minus(STOS_time_now(),StartTime)) < (TICKS_PER_SECOND * 2))
    {
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC() ...\n" ));

        ReturnError = STCLKRV_GetExtendedSTC( Handle,
                                              &ExtendedSTC );

        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        if( ReturnError == ST_NO_ERROR )
        {
            STTBX_Print(( "STCBaseBit32 %12u\n"
                          "STCBaseValue %12u\n"
                          "STCExtension %12u\n",
                          ExtendedSTC.BaseBit32,
                          ExtendedSTC.BaseValue,
                          ExtendedSTC.Extension ));
        }
        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        if (ReturnError == ST_NO_ERROR)
        {
            STTBX_Print(( "STC: %12u\n",STC));
        }

        STTBX_Print(( "----------------------------------------------------------\n" ));

        STOS_TaskDelay(DELAY_PERIOD/10);
    }

    /* Cleaning up tasks */
#ifdef ST_OS20
    /* ExtSTC has already finished */
    STOS_TaskDelete(GetExtSTCInterruptTestTask_p,TEST_PARTITION_1,
                     GetExtSTCInterruptTestTaskStack_p,TEST_PARTITION_1);

    /* Wait for STC task, then delete */
    STTBX_Print(("Waiting for STC interrupt test task to end\n"));
    STOS_TaskWait(&GetSTCInterruptTestTask_p,TIMEOUT_INFINITY);

    STTBX_Print(("Deleting STC interrupt test task\n"));
    STOS_TaskDelete(GetSTCInterruptTestTask_p,TEST_PARTITION_1,
                        GetSTCInterruptTestTaskStack_p,TEST_PARTITION_1);
#elif defined (ST_OS21)
    STTBX_Print(("Deleting STC interrupt test task\n"));
    STOS_TaskDelete(GetSTCInterruptTestOSXTask_p,TEST_PARTITION_1,
                                                  NULL,TEST_PARTITION_1);
#endif

abortTestPcrWithSTDEMUX:

    /* Close Clock Recovery explicitly */
    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STTBX_Print(("Calling STDMUX_Close()...........\n" ));
    ReturnError = STDEMUX_Close( DEMUXSessionHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STDEMUXTermParams.ForceTermination = TRUE;
    STTBX_Print(("Calling STDEMUX_Term() forced.....\n"));
    ReturnError = STDEMUX_Term(STDEMUX_DeviceName,&STDEMUXTermParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() for PTI........\n" ));
    ReturnError = STEVT_Term( DEMUXEVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("\n-----------------------------------------------------------\n"));
    STTBX_Print(( "Recovery behaviour and GetSTC Functionality test completed.\n" ));
    STTBX_Print(("-----------------------------------------------------------\n\n"));

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Output running analysis */
    STTBX_Print(("\n**********************************************************\n"));
    STTBX_Print(("PASSED: %3d\n", Result.NumberPassed));
    STTBX_Print(("FAILED: %3d\n", Result.NumberFailed));
    STTBX_Print(("**********************************************************\n"));

}

/***********************************************************************************
* Name:     TestSTCBaselineWithSTPTI()
*
* Description:  Tests clock recovery with STPTI use requiring packet injecto
*               Exercises STC basleline functionality.
*
* Notes:        Same test philosophy as TestPcrWithPti, but uses STPTI
*               instead of Pti calls.
*
***********************************************************************************/
static void TestSTCBaselineWithSTDEMUX()
{
    ST_ErrorCode_t            StoredError = ST_NO_ERROR;
    ST_ErrorCode_t            ReturnError;
    ST_Revision_t             RevisionStr;
    STDEMUX_OpenParams_t      STDEMUXOpenParams;
    STDEMUX_TermParams_t      STDEMUXTermParams;
    STDEMUX_SlotData_t        SlotData;
    STDEMUX_Slot_t            PcrSlot;
    STDEMUX_StreamParams_t    STDEMUXStreamParams;
    STDEMUX_Handle_t          DEMUXSessionHandle;
    STEVT_InitParams_t        DEMUXEVTInitPars;
    U32                       PacketCount = 0;
    STCLKRV_SourceParams_t    PCRSrc;

    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    /* Name clock recovery driver */
    strcpy( DeviceNam, "STCLKRV" );

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "\n===============================================================\n" ));
    STTBX_Print(( "Commencing TestSTCBaselineWithSTDEMUX Test Function for STDEMUX....\n" ));
    STTBX_Print(( "TEST - SYNC and BASELINE mode functionality test\n" ));
    STTBX_Print(( "===============================================================\n\n" ));

    /* Initialize the sub-ordinate EVT Driver */
    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for clkrv events...\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    event_counter_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Subscribe to PCR Events now */
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    FEI_Setup();
    FdmaSetup();

   /* Initialize the EVT Driver for DEMUX event handling*/
    strcpy( DEMUXEVTDevNam, "STEVTdemux" );
    DEMUXEVTInitPars.EventMaxNum   = 50;
    DEMUXEVTInitPars.ConnectMaxNum = 50;
    DEMUXEVTInitPars.SubscrMaxNum  = 20; /* Modified from 3 for new EVT handler */
    DEMUXEVTInitPars.MemoryPartition = TEST_PARTITION_1;
    DEMUXEVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for DEMUX\n" ));
    ReturnError = STEVT_Init( DEMUXEVTDevNam, &DEMUXEVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set up the parameters */
    STDEMUXInitParams.Device = STDEMUX_DEVICE_CHANNEL_FEC1;
    STDEMUXInitParams.Parser = STDEMUX_PARSER_TYPE_TS;
    STDEMUXInitParams.Partition_p = TEST_PARTITION_1;
    STDEMUXInitParams.NCachePartition_p = TEST_PARTITION_2;

    strcpy(STDEMUXInitParams.EventHandlerName, DEMUXEVTDevNam);

    STDEMUXInitParams.EventProcessPriority = MAX_USER_PRIORITY;
    STDEMUXInitParams.DemuxProcessPriority = 13;
    STDEMUXInitParams.DemuxMessageQueueSize = 256;
    STDEMUXInitParams.InputPacketSize = 192;
    STDEMUXInitParams.OutputModeBlocking = FALSE;
    STDEMUXInitParams.SyncLock = 1;
    STDEMUXInitParams.SyncDrop = 0;
    STDEMUXInitParams.InputMetaData = STDEMUX_INPUT_METADATA_PRE;

    STDEMUXInitParams.NumOfPcktsInPesBuffer = 300;
    STDEMUXInitParams.NumOfPcktsInRawBuffer = 0;
    STDEMUXInitParams.NumOfPcktsInSecBuffer = 0;

    STTBX_Print(( "Calling STDEMUX_Init() ................\n" ));
    strcpy( STDEMUX_DeviceName, "STDEMUX");
    ReturnError = STDEMUX_Init(STDEMUX_DeviceName, &STDEMUXInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* Open session with STDEMUX */
        STTBX_Print(("Calling STDEMUX_Open()...\n"));
        STDEMUXOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STDEMUXOpenParams.NonCachedPartition_p = TEST_PARTITION_2;
        ReturnError = STDEMUX_Open(STDEMUX_DeviceName, &STDEMUXOpenParams,
                             &DEMUXSessionHandle);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    }


    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotData.SlotType = STDEMUX_SLOT_TYPE_PCR;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.DiscardOnCrcError = FALSE;
        SlotData.SlotFlags.SoftwareCDFifo = FALSE;

        STTBX_Print(( "Calling STDEMUX_SlotAllocate() ..........\n" ));
        ReturnError = STDEMUX_SlotAllocate(DEMUXSessionHandle, &PcrSlot, &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
        /* Set PCR pid */
        STDEMUXStreamParams.member.Pid = PCR_PID;
        STTBX_Print(( "Calling STDEMUX_SlotSetParams(%d) ..........\n", STDEMUXStreamParams.member.Pid ));
        ReturnError = STDEMUX_SlotSetParams(PcrSlot, &STDEMUXStreamParams);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    /* Get some packets */
    STOS_TaskDelay(TICKS_PER_SECOND);

    STDEMUX_GetInputPacketCount(STDEMUX_DeviceName,&PacketCount);
    STTBX_Print(("Input Packet Count :  %i\n", PacketCount));

    STDEMUX_SlotPacketCount(PcrSlot,&PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n", PacketCount));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        STTBX_Print(("\n**********************************************************\n"));
        STTBX_Print(("**** STPTI TEST!! can not be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
        return;
    }

    /* revision string available before STCLKRV Initialized */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Initialize our native Clock Recovery Driver */
    CLKRVInitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres       = TEST_PCR_DRIFT_THRES;
    CLKRVInitParams.MinSampleThres      = TEST_MIN_SAMPLE_THRES;
    CLKRVInitParams.MaxWindowSize       = TEST_MAX_WINDOW_SIZE;
    CLKRVInitParams.Partition_p         = TEST_PARTITION_1;
    CLKRVInitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams.FSBaseAddress_p     = (U32 *)CKG_BASE_ADDRESS;
    CLKRVInitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
    CLKRVInitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitParams.PCREvtHandlerName, DEMUXEVTDevNam );
    strcpy( CLKRVInitParams.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitParams.PTIDeviceName, STDEMUX_DeviceName );

    STTBX_Print(( "Calling STCLKRV_Init() ..............\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slaves frequencies*/
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slot ID for STPTI communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STPTI_s.Slot = PcrSlot;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STOS_TaskDelay(TICKS_PER_SECOND);

    STDEMUX_SlotPacketCount(PcrSlot,&PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n ", PacketCount));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        STTBX_Print(("\n**********************************************************\n"));
        STTBX_Print(("**** STDEMUX TEST!! cannot be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
        Result.NumberFailed++;;
        Passed = FALSE;
        goto abortTestSTCBaselineWithSTDEMUX;
    }

    /* Report the Initial Frequency Value */
    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
    ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                              &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
    STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreq, FS1Freq, FS2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "----------------------------------------------------------\n" ));

    STOS_TaskCreate((void(*)(void *))display_events,
                            NULL,
                            TEST_PARTITION_1,
                            DISPLAY_EVENTS_STACK_SIZE,
                            &DisplayTaskStack_p,
                            TEST_PARTITION_1,
                            &DisplayTask_p,
                            &DisplayTaskDescriptor,
                            MAX_USER_PRIORITY,
                            "display_task",
                            (task_flags_t)0);

    /* Allow settling time before invalidating PCR clock */
    STOS_TaskDelay( DELAY_PERIOD );

    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest1(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest2(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest3(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest4(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest5(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n************************************************************\n" ));

abortTestSTCBaselineWithSTDEMUX:

    /* Close Clock Recovery explicitly */
    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(("Calling STDEMUX_Close()...........\n" ));
    ReturnError = STDEMUX_Close( DEMUXSessionHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STDEMUXTermParams.ForceTermination = TRUE;
    STTBX_Print(("Calling STDEMUX_Term() forced.....\n"));
    ReturnError = STDEMUX_Term(STDEMUX_DeviceName,&STDEMUXTermParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() for PTI........\n" ));
    ReturnError = STEVT_Term( DEMUXEVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Output running analysis */
    STTBX_Print(("\n**********************************************************\n"));
    STTBX_Print(("PASSED: %3d\n", Result.NumberPassed));
    STTBX_Print(("FAILED: %3d\n", Result.NumberFailed));
    STTBX_Print(("**********************************************************\n"));

}
#endif

#if defined(DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)
/***********************************************************************************
* Name:     TestPcrWithSTPTI
*
* Description:  Tests clock recovery with STPTI use requiring packet injector
*
* Notes:        Ssame test philosophy as TestPcrWithPti, but uses STPTI
*               instead of Pti calls.
*
***********************************************************************************/
void TestPcrWithSTPTI()
{
    STCLKRV_ExtendedSTC_t ExtendedSTC;
    ST_ErrorCode_t        StoredError = ST_NO_ERROR;
    ST_ErrorCode_t        ReturnError;
    ST_Revision_t         RevisionStr;
    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */
    STPTI_OpenParams_t    STPTIOpenParams;
    STPTI_TermParams_t    STPTITermParams;
    STPTI_Handle_t        PTISessionHandle;
    STPTI_SlotData_t      SlotData;
    STPTI_Slot_t          PcrSlot;
    STEVT_InitParams_t    PTIEVTInitPars;
    U16 PacketCount       = 0;
    STCLKRV_SourceParams_t PCRSrc;
    U32 STC;
    U32 StartTime;
#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
    ST_DeviceName_t         STMERGE_Name;
    STMERGE_InitParams_t    STMERGE_InitParams;
#endif
#ifdef ST_7200
    STPTI_FrontendParams_t  PTIFrontendParams;
    STPTI_Frontend_t        PTIFrontendHandle;
#endif

    /*#define LOG_FILE "../../clkrv_enc_log.csv"
    FILE *clk_p=NULL;*/

    U32 i = 0;

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    Result.NumberPassed = Result.NumberFailed = 0;
    Passed = TRUE;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing TestPcrWithSTPTI Test Function for STPTI....\n" ));
    STTBX_Print(( "TEST - FS Clocks behaviour and GetSTC Functionality test\n" ));
    STTBX_Print(( "==========================================================\n\n" ));

    /* Name clock recovery driver */
    strcpy( DeviceNam, "STCLKRV" );

    /* Initialize the sub-ordinate EVT Driver */
    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum  = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for clkrv events...\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    event_counter_p = STOS_SemaphoreCreateFifo(NULL,0);
#endif

    /* Subscribe to PCR Events now */
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize the EVT Driver for PTI event handling*/
    strcpy( PTIEVTDevNam, "STEVTpti" );
    PTIEVTInitPars.EventMaxNum   = 50;
    PTIEVTInitPars.ConnectMaxNum = 50;
    PTIEVTInitPars.SubscrMaxNum  = 20; /* Modified from 3 for new EVT handler */
    PTIEVTInitPars.MemoryPartition = TEST_PARTITION_1;
    PTIEVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for PTI\n" ));
    ReturnError = STEVT_Init( PTIEVTDevNam, &PTIEVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
#if 0
    STMERGE_InitParams.DeviceType = STMERGE_DEVICE_1;
    STMERGE_InitParams.DriverPartition_p = TEST_PARTITION_1;
    STMERGE_InitParams.BaseAddress_p = (U32 *)TEST_TSMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    /*STMERGE_InitParams.Mode = STMERGE_NORMAL_OPERATION;*/

    STMERGE_InitParams.Mode = STMERGE_NORMAL_OPERATION_TO_PTI0;
    strcpy(STMERGE_Name,"TSMERGE");

    STTBX_Print(("Calling STMERGE_Init()... \n"));
    ReturnError = STMERGE_Init(STMERGE_Name, &STMERGE_InitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    SetParams.SyncLock  = 0;
    SetParams.SyncDrop  = 0;
    SetParams.SOPSymbol = 0x47;

    SetParams.u.TSIN.SerialNotParallel  = FALSE; /* FALSE in Parallel Mode */
    SetParams.u.TSIN.InvertByteClk      = FALSE;
    SetParams.u.TSIN.ByteAlignSOPSymbol = FALSE; /* FALSE for Parallel Mode */

    SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    SetParams.u.TSIN.SyncNotAsync       = TRUE;
    SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;

    /* Set params and Get Params function checking */
    STTBX_Print(("SetParams..\n"));

    /* Set Params for TSIN1 */
    ReturnError = STMERGE_SetParams(STMERGE_TSIN_0,&SetParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Connect TSIN1 with PTI0 */
    ReturnError = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#else
    /* Set TSmerge(7710) in Bypass mode  simply/ But you can not
       deliver STC from TSMERGE then */
    /* Better write a seperate test case for that ??? or some build flag */
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)TEST_TSMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    STMERGE_InitParams.Mode              = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
    STMERGE_InitParams.DriverPartition_p = TEST_PARTITION_1;

    /*Init merge*/
    strcpy(STMERGE_Name,"TSMERGE");
    STTBX_Print(("Calling STMERGE Init....\n"));
    ReturnError = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    ReturnError = STMERGE_ConnectGeneric(STMERGE_TSIN_0,STMERGE_PTI_0,STMERGE_BYPASS_GENERIC);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#endif
#endif

    /* Initialize STPTI init params according to board type */
    CLKRV_STPTI_InitSetup(&STPTIInitParams);

#ifdef ST_OSLINUX
    ReturnError = STPTI_TEST_ForceLoader(1);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(( "Calling STPTI_Init() ................\n" ));
    ReturnError = STPTI_Init(STPTI_DeviceName, &STPTIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* open session */
        STPTIOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STPTIOpenParams.NonCachedPartition_p =  TEST_PARTITION_2;
        STTBX_Print(( "Calling STPTI_Open() ................\n" ));
        ReturnError = STPTI_Open(STPTI_DeviceName, &STPTIOpenParams, &PTISessionHandle);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

#ifdef ST_7200

    STTBX_Print(( "Calling STPTI_FrontendAllocate() ................\n" ));

    ReturnError = STPTI_FrontendAllocate(PTISessionHandle, &PTIFrontendHandle, STPTI_FRONTEND_TYPE_TSIN);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PTIFrontendParams.PktLength = STPTI_FRONTEND_PACKET_LENGTH_DVB;
    PTIFrontendParams.u.TSIN.SyncLDEnable = FALSE;
    PTIFrontendParams.u.TSIN.SerialNotParallel = TRUE;
    PTIFrontendParams.u.TSIN.AsyncNotSync = FALSE;
    PTIFrontendParams.u.TSIN.AlignByteSOP = TRUE;
    PTIFrontendParams.u.TSIN.InvertTSClk = FALSE;
    PTIFrontendParams.u.TSIN.IgnoreErrorInByte = FALSE;
    PTIFrontendParams.u.TSIN.IgnoreErrorInPkt = FALSE;
    PTIFrontendParams.u.TSIN.IgnoreErrorAtSOP = FALSE;
    PTIFrontendParams.u.TSIN.InputBlockEnable = TRUE;
    PTIFrontendParams.u.TSIN.ClkRvSrc = STPTI_FRONTEND_CLK_RCV0;
    PTIFrontendParams.u.TSIN.MemoryPktNum = 48;

    STTBX_Print(( "Calling STPTI_FrontendSetParams() ..........\n" ));
    ReturnError = STPTI_FrontendSetParams(PTIFrontendHandle, &PTIFrontendParams);

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STPTI_FrontendLinkToPTI() ..........\n" ));
    ReturnError = STPTI_FrontendLinkToPTI(PTIFrontendHandle, PTISessionHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif
    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
        SlotData.SlotFlags.StoreLastTSHeader = FALSE;
        SlotData.SlotFlags.InsertSequenceError = FALSE;
        SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
        SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
        SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
        SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
        SlotData.SlotFlags.SoftwareCDFifo = FALSE;

        STTBX_Print(( "Calling STPTI_SlotAllocate() ..........\n" ));
        ReturnError = STPTI_SlotAllocate(PTISessionHandle, &PcrSlot, &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
        /* Set PCR pid */
        STTBX_Print(( "Calling STPTI_SlotSetPid(%d) ..........\n", PCR_PID ));
        ReturnError = STPTI_SlotSetPid(PcrSlot,PCR_PID);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    /* Get some packets */
    STOS_TaskDelay(TICKS_PER_SECOND);

    STPTI_GetInputPacketCount(STPTI_DeviceName,&PacketCount);
    STTBX_Print(("Input Packet Count :  %i\n", PacketCount));

    STPTI_SlotPacketCount(PcrSlot,&PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n", PacketCount));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        /* Output running analysis */
        STTBX_Print(("\n**********************************************************\n"));
        STTBX_Print(("**** STPTI TEST!! can not be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
        Result.NumberFailed++;;
        Passed = FALSE;
        goto abortTestPcrWithSTPTI;
    }

    Passed =TRUE;

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    /* revision string available before STCLKRV Initialized */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));
#endif

    /* Initialize our native Clock Recovery Driver */

    CLKRVInitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres       = TEST_PCR_DRIFT_THRES;
    CLKRVInitParams.MinSampleThres      = TEST_MIN_SAMPLE_THRES;
    CLKRVInitParams.MaxWindowSize       = TEST_MAX_WINDOW_SIZE;
    CLKRVInitParams.Partition_p         = TEST_PARTITION_1;
    CLKRVInitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams.FSBaseAddress_p     = (U32 *)CKG_BASE_ADDRESS;
#if defined(ST_7100) || defined (ST_7109) || defined (ST_7200)
    CLKRVInitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    CLKRVInitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;
#if defined (ST_5525) || defined (ST_7200)
    CLKRVInitParams.DecodeType          = STCLKRV_DECODE_PRIMARY;
#endif
#ifdef ST_7200
    CLKRVInitParams.CRUBaseAddress_p    = (U32*)(CLKRV_BASE_ADDRESS);
#endif

    strcpy( CLKRVInitParams.PCREvtHandlerName, PTIEVTDevNam );
    strcpy( CLKRVInitParams.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitParams.PTIDeviceName, STPTI_DeviceName );

    STTBX_Print(( "Calling STCLKRV_Init() ..............\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_7100) || defined (ST_7109)
    /*{
        volatile U32 *addptr = (U32 *)0xB90000B4;
        *addptr = 0x8;
    }
    task_delay(30*ST_GetClocksPerSecond());*/
#endif

#if defined(ST_OSLINUX)
    /* revision string available before STCLKRV Opened */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preOpen\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));
#endif
    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slaves frequencies*/

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 12288000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
#ifdef CLKRV_FSCLKRV_TEST_SD_ONLY
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1],
                                                 108000000);
#else

#if defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1],
                                                 148500000);
#else
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1],
                                                 108000000);
#endif

#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[2],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[3],
                                                 24576000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#if defined (ST_7200)
    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[4],
                                                 45158400);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[5],
                                                 74250000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

#endif /*  CLKRV_FSCLKRV_TEST_SD_ONLY */

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Disabling Clock Recovery...\n"));
    ReturnError = STCLKRV_Disable(Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slot ID for STPTI communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STPTI_s.Slot = PcrSlot;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST [0]: Clock recovery Behaviour test\n"));
    STTBX_Print(("Frequency Expected value will vary on different boards\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Report the Initial Frequency Value */
    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
    ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                              &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
    STTBX_Print(( "FS Block Frequencies before enable\n"));
    STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreq, FS1Freq, FS2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#ifdef CLKRV_FSCLKRV_TEST_SD_ONLY
    /* Set Application mode to SD_only */
    STTBX_Print(( "Calling STCLKRV_SetApplicationMode()....\n" ));
    ReturnError = STCLKRV_SetApplicationMode( Handle, STCLKRV_APPLICATION_MODE_SD_ONLY );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /*STTBX_Print(("Enable Clock Recovery...\n\n"));*/
    ReturnError = STCLKRV_Enable(Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Observe FS Block Frequencies after enable\n"));
    STTBX_Print(("(Freq. sampled every 10ms)\n"));

    PacketCount =0;

#ifdef CLKRV_DETAIL_TEST
    for (i = 0; i < 1000; i++)
#else
    for (i = 0; i < 1000; i++)
#endif
    {

        STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                    &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        STPTI_SlotPacketCount(PcrSlot,&PacketCount);
        STTBX_Print(("%12u, %12u, %12u, %12u, %12u, %12u, %12u, %12u\n",STCFreq, FS1Freq, FS2Freq,
                                                             FS3Freq, FS4Freq, FS5Freq, FS6Freq, PacketCount ));

        STOS_TaskDelay(TICKS_PER_SECOND/25);
    }

#if 0 /* Just to regress more */
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 22579200);
    if (ReturnError!=ST_NO_ERROR)
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    for (i = 0; i < 500; i++)
    {

        STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                    &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        STTBX_Print(("%12u, %12u, %12u, %12u, %12u, %12u, %12u\n",STCFreq, FS1Freq, FS2Freq,
                                                             FS3Freq, FS4Freq, FS5Freq, FS6Freq ));
        STOS_TaskDelay(TICKS_PER_SECOND / 25);
    }
#endif

#if 0
    ReturnError = STCLKRV_Disable(Handle);
    clk_p = fopen(LOG_FILE, "wb");
    for (i = 0; i < SampleCount; i++)
    {
        /*STTBX_Print((" %12u, %12u\n",ClkrvDbg[2][i],ClkrvDbg[3][i]));*/
        fprintf(clk_p, "%u, %u, %u, %u, %d, %d, %d, %d, %d, %d, %d\n",
                                             ClkrvDbg[0][i],ClkrvDbg[2][i],
                                             ClkrvDbg[3][i],ClkrvDbg[1][i],
                                             ClkrvDbg[8][i],ClkrvDbg[4][i],
                                             ClkrvDbg[5][i],ClkrvDbg[6][i],
                                             ClkrvDbg[7][i],ClkrvDbg[9][i],
                                             ClkrvDbg[10][i]);
    }
#endif

    STTBX_Print(("\n-----------------------------------------------------------\n"));
    STTBX_Print(("FS Clocks Behaviour test complete.\n"));
    STTBX_Print(("-----------------------------------------------------------\n\n"));

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST: STCLKRV_GetSTC and STCLKRV_GetExtendedSTC\n"));
    STTBX_Print(("===========================================================\n\n"));

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STTBX_Print(( "Commencing calls from interrupt tests...\n\n" ));

        /* Kick off event monitoring task */
    STOS_TaskCreate((void(*)(void *))display_events,
                                NULL,
                                TEST_PARTITION_1,
                                DISPLAY_EVENTS_STACK_SIZE,
                                &DisplayTaskStack_p,
                                TEST_PARTITION_1,
                                &DisplayTask_p,
                                &DisplayTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "display_task",
                                (task_flags_t)0);
#ifdef ST_OS21
    /* Kick off interrupt context test task */
    STOS_TaskCreate((void(*)(void *))GetSTCInterruptTestOSXTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                ((16*1024)+(4*256)),
                                NULL,
                                TEST_PARTITION_1,
                                &GetSTCInterruptTestOSXTask_p,
                                NULL,
                                MAX_USER_PRIORITY,
                                "GetSTCInterruptTestTask",
                                (task_flags_t)task_flags_no_min_stack_size);

    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetSTCInterruptTestOSXTask_p, TIMEOUT_INFINITY);

#else
    /* Kick off interrupt context test task */
    STOS_TaskCreate((void(*)(void *))GetSTCInterruptTestTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                4*256,
                                &GetSTCInterruptTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetSTCInterruptTestTask_p,
                                &GetSTCInterruptTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetSTCInterruptTestTask",
                                (task_flags_t)0);

    STOS_TaskCreate((void(*)(void *))GetExtSTCInterruptTestTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                4*256,
                                &GetExtSTCInterruptTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetExtSTCInterruptTestTask_p,
                                &GetExtSTCInterruptTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetExtSTCInterrupTestTask",
                                (task_flags_t)0);

    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetExtSTCInterruptTestTask_p, TIMEOUT_INFINITY);

#endif

    STTBX_Print(("\nAssessing calls from interrupt test results....\n"));
    if ((GetSTCInterruptTestOK != TRUE) || (GetExtSTCInterruptTestOK != TRUE))
    {
        Passed = FALSE;
        ErrorReport( &StoredError, LOCAL_ERROR_INTERRUPT_STC, ST_NO_ERROR );
    }
    else
    {
        ErrorReport( &StoredError, ST_NO_ERROR, ST_NO_ERROR );
    }

#endif /* Linux */

    PrintPassFail(Passed, "STC values OK", "Invalid STC values");

    STTBX_Print(( "Commencing calls from task tests.....\n\n" ));

    /* Invalidate clock and receive PCR events... */
    STTBX_Print(( "Calling STCLKRV_InvDecodeClk() ...\n" ));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* wait before calling Get_XXX else you will get NO_PCR_ERROR */
    STOS_TaskDelay(TICKS_PER_SECOND * 2);

    /* Run for period of time */
    /*while( EventsDisplayed < 5 )*/
    StartTime = STOS_time_now();

    /* Enter timed loop...*/
    while ((STOS_time_minus(STOS_time_now(),StartTime)) < (TICKS_PER_SECOND * 2))
    {
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC() ...\n" ));

        ReturnError = STCLKRV_GetExtendedSTC( Handle,
                                              &ExtendedSTC );

        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        if( ReturnError == ST_NO_ERROR )
        {
            STTBX_Print(( "STCBaseBit32 %12u\n"
                          "STCBaseValue %12u\n"
                          "STCExtension %12u\n",
                          ExtendedSTC.BaseBit32,
                          ExtendedSTC.BaseValue,
                          ExtendedSTC.Extension ));
        }
        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        if (ReturnError == ST_NO_ERROR)
        {
            STTBX_Print(( "STC: %12u\n",STC));
        }

        STTBX_Print(( "----------------------------------------------------------\n" ));

        STOS_TaskDelay(DELAY_PERIOD/10);
    }

    /* Cleaning up tasks */
#ifdef ST_OS20
    /* ExtSTC has already finished */
    STOS_TaskDelete(GetExtSTCInterruptTestTask_p,TEST_PARTITION_1,
                     GetExtSTCInterruptTestTaskStack_p,TEST_PARTITION_1);

    /* Wait for STC task, then delete */
    STTBX_Print(("Waiting for STC interrupt test task to end\n"));
    STOS_TaskWait(&GetSTCInterruptTestTask_p,TIMEOUT_INFINITY);
    STTBX_Print(("Deleting STC interrupt test task\n"));
    STOS_TaskDelete(GetSTCInterruptTestTask_p,TEST_PARTITION_1,
                        GetSTCInterruptTestTaskStack_p,TEST_PARTITION_1);
#elif defined (ST_OS21)
    STTBX_Print(("Deleting STC interrupt test task\n"));
    STOS_TaskDelete(GetSTCInterruptTestOSXTask_p,TEST_PARTITION_1,
                                                  NULL,TEST_PARTITION_1);
#endif

abortTestPcrWithSTPTI:

    /* Close Clock Recovery explicitly */
    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STTBX_Print(("Calling STPTI_Close()...........\n" ));
    ReturnError = STPTI_Close( PTISessionHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STPTITermParams.ForceTermination = TRUE;
    STTBX_Print(("Calling STPTI_Term() forced.....\n"));
    ReturnError = STPTI_Term(STPTI_DeviceName,&STPTITermParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() for PTI........\n" ));
    ReturnError = STEVT_Term( PTIEVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
    /* Terminate STMERGE  */
    STTBX_Print(("Calling STMERGE_Term() ...............\n"));
    ReturnError = STMERGE_Term(STMERGE_Name, NULL);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    STTBX_Print(("\n-----------------------------------------------------------\n"));
    STTBX_Print(( "Recovery behaviour and GetSTC Functionality test completed.\n" ));
    STTBX_Print(("-----------------------------------------------------------\n\n"));

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Output running analysis */
    STTBX_Print(("\n**********************************************************\n"));
    STTBX_Print(("PASSED: %3d\n", Result.NumberPassed));
    STTBX_Print(("FAILED: %3d\n", Result.NumberFailed));
    STTBX_Print(("**********************************************************\n"));

}

/***********************************************************************************
* Name:     TestSTCBaselineWithSTPTI()
*
* Description:  Tests clock recovery with STPTI use requiring packet injecto
*               Exercises STC basleline functionality.
*
* Notes:        Same test philosophy as TestPcrWithPti, but uses STPTI
*               instead of Pti calls.
*
***********************************************************************************/
static void TestSTCBaselineWithSTPTI()
{

    ST_ErrorCode_t        StoredError = ST_NO_ERROR;
    ST_ErrorCode_t        ReturnError;
    ST_Revision_t         RevisionStr;
    STPTI_OpenParams_t    STPTIOpenParams;
    STPTI_TermParams_t    STPTITermParams;
    STPTI_Handle_t        PTISessionHandle;
    STPTI_SlotData_t      SlotData;
    STPTI_Slot_t          PcrSlot;
    STEVT_InitParams_t    PTIEVTInitPars_s;
    U16 PacketCount       = 0;
    STCLKRV_SourceParams_t PCRSrc;
#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
    ST_DeviceName_t         STMERGE_Name;
    STMERGE_InitParams_t    STMERGE_InitParams;
#endif
#ifdef ST_7200
    STPTI_FrontendParams_t  PTIFrontendParams;
    STPTI_Frontend_t        PTIFrontendHandle;
#endif

    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    /* Name clock recovery driver */
    strcpy( DeviceNam, "STCLKRV" );

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "\n===============================================================\n" ));
    STTBX_Print(( "Commencing TestSTCBaselineWithSTPTI Test Function for STPTI....\n" ));
    STTBX_Print(( "TEST - SYNC and BASELINE mode functionality test\n" ));
    STTBX_Print(( "===============================================================\n\n" ));

    /* Initialize the sub-ordinate EVT Driver */
    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for clkrv events...\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    event_counter_p = STOS_SemaphoreCreateFifo(NULL,0);
#endif

    /* Subscribe to PCR Events now */
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

   /* Initialize the EVT Driver for PTI event handling*/
    strcpy( PTIEVTDevNam, "STEVTpti" );
    PTIEVTInitPars_s.EventMaxNum   = 50;
    PTIEVTInitPars_s.ConnectMaxNum = 50;
    PTIEVTInitPars_s.SubscrMaxNum  = 20; /* Modified from 3 for new EVT handler */
    PTIEVTInitPars_s.MemoryPartition = TEST_PARTITION_1;
    PTIEVTInitPars_s.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for PTI\n" ));
    ReturnError = STEVT_Init( PTIEVTDevNam, &PTIEVTInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
#if 0
    STMERGE_InitParams.DeviceType = STMERGE_DEVICE_1;
    STMERGE_InitParams.DriverPartition_p = TEST_PARTITION_1;
    STMERGE_InitParams.BaseAddress_p = (U32 *)TEST_TSMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    /*STMERGE_InitParams.Mode = STMERGE_NORMAL_OPERATION;*/

    STMERGE_InitParams.Mode = STMERGE_NORMAL_OPERATION_TO_PTI0;

    strcpy(STMERGE_Name,"TSMERGE");

    STTBX_Print(("Calling STMERGE_Init()... \n"));
    ReturnError = STMERGE_Init(STMERGE_Name, &STMERGE_InitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    SetParams.Priority  = STMERGE_PRIORITY_HIGHEST;
    SetParams.SyncLock  = 0;
    SetParams.SyncDrop  = 0;
    SetParams.SOPSymbol = 0x47;

    SetParams.u.TSIN.SerialNotParallel  = FALSE; /* FALSE in Parallel Mode */
    SetParams.u.TSIN.InvertByteClk      = FALSE;
    SetParams.u.TSIN.ByteAlignSOPSymbol = FALSE; /* FALSE for Parallel Mode */

    SetParams.PacketLength              = STMERGE_PACKET_LENGTH_DVB;
    SetParams.u.TSIN.SyncNotAsync       = TRUE;
    SetParams.u.TSIN.ReplaceSOPSymbol   = FALSE;

    /* Set params and Get Params function checking */
    STTBX_Print(("SetParams..\n"));

    /* Set Params for TSIN1 */
    ReturnError = STMERGE_SetParams(STMERGE_TSIN_0,&SetParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Connect TSIN1 with PTI0 */
    STTBX_Print(("Calling STMERGE_Connect... \n"));
    ReturnError = STMERGE_Connect(STMERGE_TSIN_0,STMERGE_PTI_0);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#else
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)TEST_TSMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    STMERGE_InitParams.Mode              = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
    STMERGE_InitParams.DriverPartition_p = TEST_PARTITION_1;

    /*Init merge*/
    strcpy(STMERGE_Name,"TSMERGE");
    STTBX_Print(("Calling STMERGE Init....\n"));
    ReturnError = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    ReturnError = STMERGE_ConnectGeneric(STMERGE_TSIN_0,STMERGE_PTI_0,STMERGE_BYPASS_GENERIC);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#endif
#endif

    /* Inititalize init parameters according to chip type */
    CLKRV_STPTI_InitSetup(&STPTIInitParams);

#ifdef ST_OSLINUX
    ReturnError = STPTI_TEST_ForceLoader(1);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(( "Calling STPTI_Init() ................\n" ));
    ReturnError = STPTI_Init(STPTI_DeviceName, &STPTIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* open session */
        STPTIOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STPTIOpenParams.NonCachedPartition_p =  TEST_PARTITION_2;
        STTBX_Print(( "Calling STPTI_Open() ................\n" ));
        ReturnError = STPTI_Open(STPTI_DeviceName, &STPTIOpenParams, &PTISessionHandle);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

#ifdef ST_7200
    STTBX_Print(( "Calling STPTI_FrontendAllocate() ................\n" ));

    ReturnError = STPTI_FrontendAllocate(PTISessionHandle, &PTIFrontendHandle, STPTI_FRONTEND_TYPE_TSIN);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PTIFrontendParams.PktLength = STPTI_FRONTEND_PACKET_LENGTH_DVB;
    PTIFrontendParams.u.TSIN.SyncLDEnable = FALSE;
    PTIFrontendParams.u.TSIN.SerialNotParallel = TRUE;
    PTIFrontendParams.u.TSIN.AsyncNotSync = FALSE;
    PTIFrontendParams.u.TSIN.AlignByteSOP = FALSE;
    PTIFrontendParams.u.TSIN.InvertTSClk = FALSE;
    PTIFrontendParams.u.TSIN.IgnoreErrorInByte = FALSE;
    PTIFrontendParams.u.TSIN.IgnoreErrorInPkt = FALSE;
    PTIFrontendParams.u.TSIN.IgnoreErrorAtSOP = FALSE;
    PTIFrontendParams.u.TSIN.InputBlockEnable = TRUE;
    PTIFrontendParams.u.TSIN.ClkRvSrc = STPTI_FRONTEND_CLK_RCV0;
    PTIFrontendParams.u.TSIN.MemoryPktNum = 48;

    STTBX_Print(( "Calling STPTI_FrontendSetParams() ..........\n" ));
    ReturnError = STPTI_FrontendSetParams(PTIFrontendHandle, &PTIFrontendParams);

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STPTI_FrontendLinkToPTI() ..........\n" ));
    ReturnError = STPTI_FrontendLinkToPTI(PTIFrontendHandle, PTISessionHandle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
        SlotData.SlotFlags.StoreLastTSHeader = FALSE;
        SlotData.SlotFlags.InsertSequenceError = FALSE;
        SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
        SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
        SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
        SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;

        STTBX_Print(( "Calling STPTI_SlotAllocate() ..........\n" ));
        ReturnError = STPTI_SlotAllocate(PTISessionHandle, &PcrSlot, &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
         /* Set PCR pid */
        STTBX_Print(( "Calling STPTI_SlotSetPid() ..........\n" ));
        ReturnError = STPTI_SlotSetPid(PcrSlot,PCR_PID);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    STOS_TaskDelay(TICKS_PER_SECOND);


    STPTI_SlotPacketCount(PcrSlot,&PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n ", PacketCount));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        STTBX_Print(("\n**********************************************************\n"));
        STTBX_Print(("**** STPTI TEST!! can not be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
        return;
    }

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */

    /* revision string available before STCLKRV Initialized */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));
#endif

    /* Initialize our native Clock Recovery Driver */
    CLKRVInitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres       = TEST_PCR_DRIFT_THRES;
    CLKRVInitParams.MinSampleThres      = TEST_MIN_SAMPLE_THRES;
    CLKRVInitParams.MaxWindowSize       = TEST_MAX_WINDOW_SIZE;
    CLKRVInitParams.Partition_p         = TEST_PARTITION_1;
    CLKRVInitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams.FSBaseAddress_p     = (U32 *)CKG_BASE_ADDRESS;
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    CLKRVInitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    CLKRVInitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitParams.PCREvtHandlerName, PTIEVTDevNam );
    strcpy( CLKRVInitParams.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitParams.PTIDeviceName, STPTI_DeviceName );

    STTBX_Print(( "Calling STCLKRV_Init() ..............\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_OSLINUX)
    /* revision string available before STCLKRV Opened */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preOpen\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));
#endif
    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slaves frequencies*/

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1],
                                                 148500000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#ifdef CLKRV_FSCLKRV_TEST_SD_ONLY
    /* Set Application mode to SD_only */
    STTBX_Print(( "Calling STCLKRV_SetApplicationMode()....\n" ));
    ReturnError = STCLKRV_SetApplicationMode( Handle, STCLKRV_APPLICATION_MODE_NORMAL );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    /* Set Slot ID for STPTI communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STPTI_s.Slot = PcrSlot;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STOS_TaskDelay(TICKS_PER_SECOND);

    STPTI_SlotPacketCount(PcrSlot,&PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n ", PacketCount));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        STTBX_Print(("\n**********************************************************\n"));
        STTBX_Print(("**** STPTI TEST!! cannot be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
        Result.NumberFailed++;;
        Passed = FALSE;
        goto abortTestSTCBaselineWithSTPTI;
    }


    /* Report the Initial Frequency Value */
    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
    ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                              &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
    STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreq, FS1Freq, FS2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "----------------------------------------------------------\n" ));

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STOS_TaskCreate((void(*)(void *))display_events,
                            NULL,
                            TEST_PARTITION_1,
                            DISPLAY_EVENTS_STACK_SIZE,
                            &DisplayTaskStack_p,
                            TEST_PARTITION_1,
                            &DisplayTask_p,
                            &DisplayTaskDescriptor,
                            MAX_USER_PRIORITY,
                            "display_task",
                            (task_flags_t)0);
#endif

    /* Allow settling time before invalidating PCR clock */
    STOS_TaskDelay( DELAY_PERIOD );

    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest1(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest2(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest3(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest4(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

    STCBaselineTest5(Handle, &PCRSrc, &StoredError);
    STTBX_Print(( "\n************************************************************\n" ));

    STCBaselineTest6(Handle, &StoredError);
    STTBX_Print(( "\n***********************************************************\n" ));

abortTestSTCBaselineWithSTPTI:

    /* Close Clock Recovery explicitly */
    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    STTBX_Print(("Calling STPTI_Close()...........\n" ));
    ReturnError = STPTI_Close( PTISessionHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STPTITermParams.ForceTermination = TRUE;
    STTBX_Print(("Calling STPTI_Term() forced.....\n"));
    ReturnError = STPTI_Term(STPTI_DeviceName,&STPTITermParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() for PTI........\n" ));
    ReturnError = STEVT_Term( PTIEVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
    /*Term STMERGE */
    STTBX_Print(( "Calling STMERGE_Term ................\n" ));
    ReturnError = STMERGE_Term(STMERGE_Name, NULL);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    /* Output running analysis */
    STTBX_Print(("\n**********************************************************\n"));
    STTBX_Print(("PASSED: %3d\n", Result.NumberPassed));
    STTBX_Print(("FAILED: %3d\n", Result.NumberFailed));
    STTBX_Print(("**********************************************************\n"));

}
#endif

/***********************************************************************************
* Name:     STCBaselineTest1()
* Description:  Test Initialise condition.
* Expect:
*                Events should be generated on InvDecodeClock call,
*                STC should increment and clocks should change.
* Parameters:    Clock recovery instance handle,
*                Pointer to StoredError to keep track of errors
*                Pointer to PCRSrc value for SetPCRSource call test
***********************************************************************************/
void STCBaselineTest1(STCLKRV_Handle_t Handle, STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError)
{

    STCLKRV_ExtendedSTC_t ExtendedSTC;
    ST_ErrorCode_t ReturnError;
    U32 STC;
    U8 i;
    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */
    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 1: STC Baseline OFF, Clock Recovery ON (Init condition)\n"));
    STTBX_Print(("===========================================================\n\n"));


    /* Invalidate clock and receive PCR events... */
    STTBX_Print(( "Calling STCLKRV_InvDecodeClk()\n" ));
    STTBX_Print(("....check discontinuity  event occurs now\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

    /* wait before calling Get_XXX else you will get NO_PCR_ERROR */
    STOS_TaskDelay(TICKS_PER_SECOND / 5);

    /* Watch STC and clock values */
    for (i = 0; i != 5; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));

        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( Handle, &ExtendedSTC );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STC: %12u\n",STC));

        /* Report the Initial Frequency Value */
        STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreq, FS1Freq, FS2Freq ));
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

        STOS_TaskDelay( DELAY_PERIOD/10);
    }


    /* Invalidate clock and generate events by SetPCRSource call... */
    STTBX_Print(( "\nCalling STCLKRV_SetPCRSource()...\n"));
    STTBX_Print(("....check disconintuity event occurs now\n"));
    ReturnError = STCLKRV_SetPCRSource(Handle, PCRSrc);
    ErrorReport(StoredError, ReturnError, ST_NO_ERROR);


}



/***********************************************************************************
* Name:     STCBaselineTest2()
* Description:   No clock recovery, STC baseline active.
* Expect:
*                Events should NOT be generated on InvDecodeClock/SetPCRSource call,
*                STC should increment from given baseline
*                clocks should NOT change.
* Parameters:    Clock recovery instance handle,
*                Pointer to StoredError to keep track of errors
*                Pointer to PCRSrc value for SetPCRSource call test
***********************************************************************************/
void STCBaselineTest2(STCLKRV_Handle_t Handle,  STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError)
{

    U32 i;
    U32 STC, PrevSTC;
    STCLKRV_ExtendedSTC_t ExtendedSTC;
    STCLKRV_ExtendedSTC_t Baseline;
    ST_ErrorCode_t ReturnError;
    U32 PreSTCFreq, STCFreq, FS1Freq, FS2Freq;
    U32 FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */
    clock_t TestDuration;
    clock_t StartTime;
    U32 STCAcceptableDriftMax;
    U32 STCAcceptableDriftMin;
    U32 FrequencyDriftMax;

    BOOL FreerunIncCheckOK = TRUE;

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 2: STC Baseline ON, Clock Recovery OFF\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Disable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Disable()\n" ));
    ReturnError = STCLKRV_Disable(Handle);
    ErrorReport(StoredError, ReturnError, ST_NO_ERROR);

    /* Set Baseline value */
    Baseline.BaseBit32 = 0;
    Baseline.BaseValue = 22222222;
    Baseline.Extension = 0;
    STTBX_Print(("\nSTCBaseline set to :\n"));
    STTBX_Print(( "BaseBit32: %12u\n"
                  "BaseValue: %12u\n"
                  "Extension: %12u\n",
                   Baseline.BaseBit32,
                   Baseline.BaseValue,
                   Baseline.Extension ));
    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Enable baslining */
    STTBX_Print(( "\nCalling STCLKRV_SetSTCSource()\n" ));
    STTBX_Print(("...set source to STC baseline\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, STCLKRV_STC_SOURCE_BASELINE);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Invalidate clock check no events... */
    STTBX_Print(( "\nCalling STCLKRV_InvDecodeClk()\n" ));
    STTBX_Print(("...check no discontinuity event occurs now\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline again to enable()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Watch STC and clock values */
    for (i = 0; i != 5; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( Handle, &ExtendedSTC );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STC: %12u\n",STC));

        /* Report the Initial Frequency Value */
        STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        STTBX_Print(( "FS0 = %d, FS1 = %d, FS2 = %d\n", STCFreq, FS1Freq, FS2Freq));
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

        STOS_TaskDelay( DELAY_PERIOD/10 );
    }


    /* Invalidate clock and generate no events by SetPCRSource call... */
    STTBX_Print(( "\nCalling STCLKRV_SetPCRSource()...\n"));
    STTBX_Print(("....check no discontinuity event occurs now\n"));
    ReturnError = STCLKRV_SetPCRSource(Handle, PCRSrc);
    ErrorReport(StoredError, ReturnError, ST_NO_ERROR);


    STTBX_Print(("\n-----------------------------------------------------------\n"));
    STTBX_Print(("TEST 2 - Value Checks complete.\n"));
    STTBX_Print(("-----------------------------------------------------------\n"));

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 2 - Stage 2:  Freerun Auto Check\n"));
    STTBX_Print(("Checks for Freerunning STC increment consistancy and \nClock stability\n"));
    STTBX_Print(("Test should last %i  minutes at most (less if fail).\n",FREERUN_TEST_MINS));
    STTBX_Print(("===========================================================\n"));
    STTBX_Print(("\n....Please wait....\n\n"));

    /* Set initial condition */
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    TestDuration = ((TICKS_PER_SECOND * 60) * FREERUN_TEST_MINS);
    STCAcceptableDriftMax = 18000;  /* ~ 200ms : Based on an approx 100ms sample loop */
    STCAcceptableDriftMin = 7000;  /* < 100ms :  Based on an approx 100ms sample loop  */
    FrequencyDriftMax = 100;
    StartTime = STOS_time_now();
#else
    TestDuration = 60 * FREERUN_TEST_MINS;
    STCAcceptableDriftMax = 18000;  /* ~ 200ms : Based on an approx 100ms sample loop */
    STCAcceptableDriftMin = 7000;  /* < 100ms :  Based on an approx 100ms sample loop  */
    FrequencyDriftMax = 100;
    StartTime = time(NULL);
#endif
    FreerunIncCheckOK = TRUE;
    ReturnError = STCLKRV_GetSTC(Handle, &PrevSTC);
    if (ReturnError != ST_NO_ERROR)
    {
        STTBX_Print(("***ERROR: Failed obtained reference STC from STCLKRV_GetSTC.\n"));
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        FreerunIncCheckOK = FALSE;
    }

    ReturnError = STCLKRV_GetClocksFrequency( Handle, &PreSTCFreq, &FS1Freq, &FS2Freq,
                                              &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );

    if (ReturnError != ST_NO_ERROR)
    {
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(("***ERROR: while calling STCLKRV_GetClocksFrequency..something wrong\n"));
        FreerunIncCheckOK = FALSE;
    }

    Passed = TRUE;

    /* Enter timed loop...*/
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    while (((STOS_time_minus(STOS_time_now(),StartTime)) < TestDuration) && (FreerunIncCheckOK))
#else
    i=0;
    while (((time(NULL)- StartTime) < TestDuration) && (FreerunIncCheckOK))
#endif
    {
        /* approx 100ms delay between samples */
        STOS_TaskDelay( DELAY_PERIOD/10 );

        /* Check STC increment step not too big */
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
            STTBX_Print(("***ERROR: while calling STCLKRV_GetSTC..something wrong\n"));
            FreerunIncCheckOK = FALSE;
        }
        if (((STC - PrevSTC) < STCAcceptableDriftMin) || ((STC - PrevSTC) > STCAcceptableDriftMax))
        {
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
            STTBX_Print(("***ERROR: STCIncrement beyond tolerated drift...something wrong\n"));
            STTBX_Print(("STCDiff = %i\n",(STC - PrevSTC)));
            FreerunIncCheckOK = FALSE;
#else
            i++;
#endif
        }
        PrevSTC = STC;

        /* Check Clock freq. not changing */
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
            STTBX_Print(("***ERROR: while calling STCLKRV_GetClocksFrequency..something wrong\n"));
            FreerunIncCheckOK = FALSE;
        }
        if ((PreSTCFreq > STCFreq && (PreSTCFreq - STCFreq) > FrequencyDriftMax) ||
            (PreSTCFreq < STCFreq && (STCFreq - PreSTCFreq) > FrequencyDriftMax))
        {
            STTBX_Print(("***ERROR: Frequency value changed...something wrong\n"));
            STTBX_Print(("FreqDiff = %i\n",(PreSTCFreq - STCFreq)));
            FreerunIncCheckOK = FALSE;
        }
        PreSTCFreq = STCFreq;

    }/* end while */
#ifdef ST_OS_LINUX
    if (i>10) /* acceptable glitches under Linux */
    {
        STTBX_Print(("***ERROR: STCIncrement beyond tolerated drift too many times...something wrong\n"));
        STTBX_Print(("Number of errors = %i\n",i));
        FreerunIncCheckOK = FALSE;
    }
#endif

    /* Summarise... */
    if (FreerunIncCheckOK == FALSE)
    {
        Passed = FALSE;
        /* Failed, report using standard method but local error code */
        ErrorReport(StoredError, LOCAL_ERROR_FREERUN_INC, ST_NO_ERROR );

    }
    else
    {
        STTBX_Print(("Ok. Increment within accepted threshold.\n"));
        STTBX_Print(("Test2 Stage 2 passed.\n"));
    }

    PrintPassFail(Passed,"Free Run Test OK", "Free Run Test FAILED!!" );
}

/***********************************************************************************
* Name:     STCBaselineTest3()
* Description:   Tests STC active while clkrv in progress.
* Expect:
*                Events should be generated on InvDecodeClock/SetPCRSource call,
*                STC should increment from given baseline
*                clock should change.
* Parameters:    Clock recovery instance handle,
*                Pointer to StoredError to keep track of errors
*                Pointer to PCRSrc value for SetPCRSource call test
***********************************************************************************/
void STCBaselineTest3(STCLKRV_Handle_t Handle, STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError)
{

    STCLKRV_ExtendedSTC_t Baseline;
    ST_ErrorCode_t ReturnError;

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("\nTEST 3: STC Baseline ON, Clock Recovery ON \n\n"));
    STTBX_Print(("\nTest to check that STC values in BASELINE mode"));
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STTBX_Print(("\ncalled in INTERRUPT context\n"));
#else
    STTBX_Print(("\n - doesn't do much under Linux\n"));
#endif
    STTBX_Print(("===========================================================\n\n"));

#ifdef ST_OSLINUX
    /* Enable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Enable().....\n" ));
    ReturnError = STCLKRV_Enable(Handle);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);
#endif

    /* Set Baseline value */
    Baseline.BaseBit32 = 0;
    Baseline.BaseValue = 3333333;
    Baseline.Extension = 0;
    STTBX_Print(("\nSTCBaseline set to :\n"));
    STTBX_Print(( "BaseBit32: %12u\n"
                  "BaseValue: %12u\n"
                  "Extension: %12u\n",
                   Baseline.BaseBit32,
                   Baseline.BaseValue,
                   Baseline.Extension ));

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);


    /* Enable baslining */
    STTBX_Print(( "\nCalling STCLKRV_SetSTCSource()\n" ));
    STTBX_Print(("...set source to STC baseline\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, STCLKRV_STC_SOURCE_BASELINE);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);


    /* Invalidate clock, check events... */
    STTBX_Print(( "\nCalling STCLKRV_InvDecodeClk()\n" ));
    STTBX_Print(("...check discontinuity event occurs now\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline to enable()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /*
    * Init two task to print STC
    * One Display task to display events
    */

    /* Kick off interrupt context test task */
#ifdef ST_OS21
    /* Kick off interrupt context test task */
    STOS_TaskCreate((void(*)(void *))GetSTCInterruptTestOSXTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                ((16*1024)+(4*256)),
                                NULL,
                                TEST_PARTITION_1,
                                &GetSTCInterruptTestOSXTask_p,
                                NULL,
                                MAX_USER_PRIORITY,
                                "GetSTCInterruptTestTask",
                                (task_flags_t)task_flags_no_min_stack_size);

    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetSTCInterruptTestOSXTask_p, TIMEOUT_INFINITY);

#elif defined (ST_OS20)
    /* Kick off interrupt context test task */
    STOS_TaskCreate((void(*)(void *))GetSTCInterruptTestTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                4*256,
                                &GetSTCInterruptTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetSTCInterruptTestTask_p,
                                &GetSTCInterruptTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetSTCInterruptTestTask",
                                (task_flags_t)0);

    STOS_TaskCreate((void(*)(void *))GetExtSTCInterruptTestTaskFunc,
                                NULL,
                                TEST_PARTITION_1,
                                4*256,
                                &GetExtSTCInterruptTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetExtSTCInterruptTestTask_p,
                                &GetExtSTCInterruptTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetExtSTCInterrupTestTask",
                                (task_flags_t)0);

    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetExtSTCInterruptTestTask_p, TIMEOUT_INFINITY);
    /* Wait for STC task to complete before proceeding */
    STOS_TaskWait(&GetSTCInterruptTestTask_p, TIMEOUT_INFINITY);

#endif

    Passed = TRUE;
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STTBX_Print(("\nAssessing calls from interrupt test results....\n"));
    if ((GetSTCInterruptTestOK != TRUE) || (GetExtSTCInterruptTestOK != TRUE))
    {
        Passed = FALSE;
        ErrorReport( StoredError, LOCAL_ERROR_INTERRUPT_STC, ST_NO_ERROR );
    }
    else
    {
        ErrorReport( StoredError, ST_NO_ERROR, ST_NO_ERROR );
    }
#endif

    PrintPassFail(Passed, "STC values OK", "Invalid STC values");

    /* Delete the both the task... */

    /* Cleaning up tasks */
#ifdef ST_OS20
    /* ExtSTC has already finished */
    STOS_TaskDelete(GetExtSTCInterruptTestTask_p,TEST_PARTITION_1,
                     GetExtSTCInterruptTestTaskStack_p,TEST_PARTITION_1);

    /* Wait for STC task, then delete */
    STTBX_Print(("Deleting STC interrupt test task\n"));
    STOS_TaskDelete(GetSTCInterruptTestTask_p,TEST_PARTITION_1,
                        GetSTCInterruptTestTaskStack_p,TEST_PARTITION_1);
#elif defined (ST_OS21)
    STTBX_Print(("Deleting STC interrupt test task\n"));
    STOS_TaskDelete(GetSTCInterruptTestOSXTask_p,TEST_PARTITION_1,
                                                  NULL,TEST_PARTITION_1);
#endif

    /* Invalidate clock and generate events by SetPCRSource call... */
    STTBX_Print(( "\nCalling STCLKRV_SetPCRSource()...\n"));
    STTBX_Print(("....check discontinuity event occurs now\n"));
    ReturnError = STCLKRV_SetPCRSource(Handle, PCRSrc);
    ErrorReport(StoredError, ReturnError, ST_NO_ERROR);

}

/***********************************************************************************
* Name:     STCBaselineTest4()
* Description:   Tests STC active while clkrv in progress.
* Expect:
*                Events should be generated on InvDecodeClock/SetPCRSource call,
*                STC should increment from given baseline
*                clock should change.
* Parameters:    Clock recovery instance handle,
*                Pointer to StoredError to keep track of errors
*                Pointer to PCRSrc value for SetPCRSource call test
***********************************************************************************/
void STCBaselineTest4(STCLKRV_Handle_t Handle, STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError)
{

    STCLKRV_ExtendedSTC_t ExtendedSTC;
    STCLKRV_ExtendedSTC_t Baseline;
    ST_ErrorCode_t ReturnError;
    U32 STC;
    U8 i;
    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 4: STC Baseline ON, Clock Recovery ON\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Enable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Enable().....\n" ));
    ReturnError = STCLKRV_Enable(Handle);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Set Baseline value */
    Baseline.BaseBit32 = 0;
    Baseline.BaseValue = 44444444;
    Baseline.Extension = 0;
    STTBX_Print(("\nSTCBaseline set to :\n"));
    STTBX_Print(( "BaseBit32: %12u\n"
                  "BaseValue: %12u\n"
                  "Extension: %12u\n",
                   Baseline.BaseBit32,
                   Baseline.BaseValue,
                   Baseline.Extension ));

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);


    /* Enable baslining */
    STTBX_Print(( "\nCalling STCLKRV_SetSTCSource()\n" ));
    STTBX_Print(("...set source to STC baseline\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, STCLKRV_STC_SOURCE_BASELINE);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);


    /* Invalidate clock, check events... */
    STTBX_Print(( "\nCalling STCLKRV_InvDecodeClk()\n" ));
    STTBX_Print(("...check discontinuity event occurs now\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline to enable()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Watch STC and clock values */
    for (i = 0; i != 5; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( Handle, &ExtendedSTC );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STC: %12u\n",STC));

        /* Report the Initial Frequency Value */
        STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "FS0 = %d, FS1 = %d, FS2 = %d\n", STCFreq, FS1Freq, FS2Freq));


        STOS_TaskDelay( DELAY_PERIOD/10);

    }

    /* Invalidate clock and generate events by SetPCRSource call... */
    STTBX_Print(( "\nCalling STCLKRV_SetPCRSource()...\n"));
    STTBX_Print(("....check discontinuity event occurs now\n"));
    ReturnError = STCLKRV_SetPCRSource(Handle, PCRSrc);
    ErrorReport(StoredError, ReturnError, ST_NO_ERROR);


}


/***********************************************************************************
* Name:     STCBaselineTest5()
* Description:   Tests the disabling of STCbaselining and clkrv with a call
*                InvDecodeClk causing invalid PCR condition.
* Expect:
*                Events should NOT be generated on InvDecodeClock/SetPCRSource call,
*                STC should be invalid because of Invadilate call.
*                clock should NOT change.
* Parameters:    Clock recovery instance handle,
*                Pointer to StoredError to keep track of errors
*                Pointer to PCRSrc value for SetPCRSource call test
***********************************************************************************/
void STCBaselineTest5(STCLKRV_Handle_t Handle, STCLKRV_SourceParams_t *PCRSrc,
                      ST_ErrorCode_t *StoredError)
{

    STCLKRV_ExtendedSTC_t ExtendedSTC;
    STCLKRV_ExtendedSTC_t Baseline;
    ST_ErrorCode_t ReturnError;
    U32 STC;
    U8 i;
    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */

    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 5: STC Baseline OFF, Clock Recovery OFF \n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Disable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Disable()\n" ));
    ReturnError = STCLKRV_Disable(Handle);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Set Baseline value */
    Baseline.BaseBit32 = 0;
    Baseline.BaseValue = 55555555;
    Baseline.Extension = 0;
    STTBX_Print(("\nSTCBaseline set to :\n"));
    STTBX_Print(( "BaseBit32: %12u\n"
                  "BaseValue: %12u\n"
                  "Extension: %12u\n",
                   Baseline.BaseBit32,
                   Baseline.BaseValue,
                   Baseline.Extension ));
    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);


    /* Disable baslining */
    STTBX_Print(( "\nCalling STCLKRV_SetSTCSource()\n" ));
    STTBX_Print(("...set STCSource to PCR\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, STCLKRV_STC_SOURCE_PCR);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);


    /* Invalidate clock check no events... */
    STTBX_Print(( "\nCalling STCLKRV_InvDecodeClk()\n" ));
    STTBX_Print(( "...check no discontinuity event occurs now\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

    /* Watch STC and clock values */
    for (i = 0; i != 5; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( Handle, &ExtendedSTC );
        ErrorReport( StoredError, ReturnError, STCLKRV_ERROR_PCR_UNAVAILABLE );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( StoredError, ReturnError, STCLKRV_ERROR_PCR_UNAVAILABLE );
        STTBX_Print(( "STC: %12u\n",STC));

        /* Report the Initial Frequency Value */
        STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "FS0 = %d, FS1 = %d, FS2 = %d\n", STCFreq, FS1Freq, FS2Freq));

        STOS_TaskDelay( DELAY_PERIOD/10);
    }

    /* Invalidate clock and generate no events by SetPCRSource call... */
    STTBX_Print(( "\nCalling STCLKRV_SetPCRSource()...\n"));
    STTBX_Print(("....check no discontinuity event occurs now\n"));
    ReturnError = STCLKRV_SetPCRSource(Handle, PCRSrc);
    ErrorReport(StoredError, ReturnError, ST_NO_ERROR);


}


/***********************************************************************************
* Name:     STCBaselineTest6()
* Description:   Test the deliberate setup of default condition, then kill
*                clock recovery, but keep PCR reference VALID.
*
* Expect:        First, STC reference to PCR and incrementing, clocks changing,
*                and events generated upon invalidation.
*                Second, STC reference to (old) PCR and incrementing, clocks NOT changing,
*                and no events generated.
* Parameters:    Clock recovery instance handle,
*                Pointer to StoredError to keep track of errors
*
***********************************************************************************/
void STCBaselineTest6(STCLKRV_Handle_t Handle,  ST_ErrorCode_t *StoredError)
{

    STCLKRV_ExtendedSTC_t ExtendedSTC;
    STCLKRV_ExtendedSTC_t Baseline;
    ST_ErrorCode_t ReturnError;
    U32 STC;
    U8 i;
    U32 STCFreq, FS1Freq, FS2Freq, FS3Freq, FS4Freq, FS5Freq, FS6Freq;            /* returned FS Frequency setting */
    FS1Freq = FS2Freq = FS3Freq = FS4Freq = FS5Freq = FS6Freq = 0;

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 6: First Stage....\n"));
    STTBX_Print (("STC Baseline OFF, Clock Recovery ON (forced initial condition)\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Enable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Enable()\n" ));
    ReturnError = STCLKRV_Enable(Handle);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Set Baseline value */
    Baseline.BaseBit32 = 0;
    Baseline.BaseValue = 66666666;
    Baseline.Extension = 0;
    STTBX_Print(("\nSTCBaseline set to :\n"));
    STTBX_Print(( "BaseBit32 %12u\n"
                  "BaseValue %12u\n"
                  "Extension %12u\n",
                   Baseline.BaseBit32,
                   Baseline.BaseValue,
                   Baseline.Extension ));

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &Baseline);
    ErrorReport(StoredError,ReturnError,ST_NO_ERROR);


    /* Disable baselining */
    STTBX_Print(( "\nCalling STCLKRV_SetSTCSource()\n" ));
    STTBX_Print(("...set STCBaseline to PCR\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, STCLKRV_STC_SOURCE_PCR);
    ErrorReport(StoredError,ReturnError,ST_NO_ERROR);


    /* Invalidate clock check no events... */
    STTBX_Print(( "\nCalling STCLKRV_InvDecodeClk()\n" ));
    STTBX_Print(("...check events occur\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport( StoredError, ReturnError, ST_NO_ERROR );

    /* wait before calling Get_XXX else you will get NO_PCR_ERROR */
    STOS_TaskDelay(TICKS_PER_SECOND / 10);

    /* Watch STC and clock values */
    for (i = 0; i != 5; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( Handle, &ExtendedSTC );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STC: %12u\n",STC));

        /* Report the Initial Frequency Value */
        STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "FS0 = %d, FS1 = %d, FS2 = %d\n", STCFreq, FS1Freq, FS2Freq));

        STOS_TaskDelay( DELAY_PERIOD/10 );
    }

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("TEST 6: Second Stage....\n"));
    STTBX_Print(("...STC Baseline OFF, Clock Recovery OFF without Invalidating PCR\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Disable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Disable()\n" ));
    ReturnError = STCLKRV_Disable(Handle);
    ErrorReport(StoredError,ReturnError, ST_NO_ERROR);

    /* Watch STC and clock values */
    for (i = 0; i != 5; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( Handle, &ExtendedSTC );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(Handle, &STC);
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STC: %12u\n",STC));

        /* Report the Initial Frequency Value */
        STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &FS1Freq, &FS2Freq,
                                                 &FS3Freq, &FS4Freq, &FS5Freq, &FS6Freq );
        ErrorReport( StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "FS0 = %d, FS1 = %d, FS2 = %d\n", STCFreq, FS1Freq, FS2Freq));

        STOS_TaskDelay(DELAY_PERIOD/10);
    }
}
/***************************************************************************
                    Interrrupt Rasing code for OS21
***************************************************************************/
#ifdef ST_OS21

static interrupt_t * interrupt_wdt;
static volatile int interrupted = 0;
int enable;

#ifdef ARCHITECTURE_ST200
#define OS21_TEST_INTERRUPT OS21_INTERRUPT_TEST
#else
#define OS21_TEST_INTERRUPT OS21_INTERRUPT_WDT_ITI
#endif


extern interrupt_name_t  OS21_TEST_INTERRUPT;

#define WTCNT_W (unsigned short*)0xffc00008
#define WTCNT_R (unsigned char* )0xffc00008
#define WTCSR_W (unsigned short*)0xffc0000c
#define WTCSR_R (unsigned char* )0xffc0000c

#define WTCNT_WRITE(val) *WTCNT_W = ((0x5a00) | (val))
#define WTCSR_WRITE(val) *WTCSR_W = ((0xa500) | (val))

osclock_t t1, t2;

semaphore_t * intmr_sync_p;

static void GetSTCInterruptTestOSXTaskFunc(void *NotUsed)
{
    S32 IntReturn;
    U32 STCInterruptCount;
    BOOL InterruptSTCSecondErr;
    U32 toggler = 0;

    STTBX_Print(( "GetSTCInterruptTestTaskFunc now running ...\n"));
    /* Setup interrupt for GetSTC call from Interrupt test */

    GetSTCInterruptTestOK = TRUE;
    GetExtSTCInterruptTestOK = TRUE;

#ifdef ARCHITECTURE_ST40
    intmr_sync_p = STOS_SemaphoreCreateFifo(NULL,0);
#endif

    interrupt_wdt = interrupt_handle (OS21_TEST_INTERRUPT);
    interrupted = interrupt_install (OS21_TEST_INTERRUPT, 0,GetSTCInterruptTestOSX, NULL);
    enable = interrupt_enable_number (OS21_TEST_INTERRUPT);

    IntReturn = 0;
    STCInterruptCount = 0;
    InterruptSTCSecondErr = FALSE;

    while ((STCInterruptCount < 40) && (IntReturn == 0))
    {
        /*
        * Start the watchdog in interval timer mode, to fire
        * in a short while
        */
#ifdef ARCHITECTURE_ST40
        WTCSR_WRITE(0x00);
        WTCNT_WRITE(0x07);
        WTCSR_WRITE(0x80);
        STOS_SemaphoreWait(intmr_sync_p);
#else
        IntReturn = interrupt_raise(interrupt_wdt);
#endif

        if ( toggler == 0)
        {
            toggler = 1;
            STTBX_Print( ("Interrupt STC    : %12u\n",InterruptSTC));
            if (InterruptSTC == 0)
            {
                /* if errored previously...not discontinuity/glitch so error */
                if (InterruptSTCSecondErr)
                    GetSTCInterruptTestOK = FALSE;
                else
                    InterruptSTCSecondErr = TRUE;
            }
            else
            {
                InterruptSTCSecondErr = FALSE;
            }

        }
        else
        {
            toggler = 0;
            STTBX_Print( ("Interrupt ExtSTC : %12u\n",InterruptExtSTC.BaseValue));
            if (InterruptExtSTC.BaseValue == 0)
            {
                /* if errored previously...not discontinuity glitch so error */
                if (InterruptSTCSecondErr)
                    GetExtSTCInterruptTestOK = FALSE;
                else
                    InterruptSTCSecondErr = TRUE;
            }
            else
            {
                InterruptSTCSecondErr = FALSE;
            }
        }
        STCInterruptCount++;
        STOS_TaskDelay(ST_GetClocksPerSecond() / 100);

    }
#ifdef ARCHITECTURE_ST200
    if ((interrupt_disable(interrupt_wdt)) != 0)
       STTBX_Print(("Interrupt disable failed\n"));

    if ((interrupt_uninstall(OS21_TEST_INTERRUPT, 0)) != 0)
        STTBX_Print(("Interrupt uninstall failed\n"));
#endif
}

int GetSTCInterruptTestOSX(void *NotUsed)
{

    /* should be same name as running task */

    STCLKRV_OpenParams_t  OpenParams_s;
    static STCLKRV_Handle_t HandleL;
    ST_ErrorCode_t IntTestRetError = LOCAL_ERROR_INTERRUPT_STC;

    t2 = STOS_time_now();

#ifdef ARCHITECTURE_ST40
    WTCSR_WRITE(0x00);
    WTCNT_WRITE(0x00);
#endif

    InterruptSTC = 0;

    STCLKRV_Open(DeviceNam, &OpenParams_s, &HandleL);

    IntTestRetError = STCLKRV_GetSTC(HandleL, &InterruptSTC);

    if (IntTestRetError == ST_NO_ERROR ){}
    else if (IntTestRetError == STCLKRV_ERROR_PCR_UNAVAILABLE ){}
    else if (IntTestRetError != ST_NO_ERROR)
    {
        InterruptSTC = 0;
    }

    IntTestRetError = STCLKRV_GetExtendedSTC(HandleL, &InterruptExtSTC);
    if (IntTestRetError == ST_NO_ERROR ){}
    else if (IntTestRetError == STCLKRV_ERROR_PCR_UNAVAILABLE ){}
    else if (IntTestRetError != ST_NO_ERROR)
    {
        InterruptExtSTC.BaseValue = 0;
    }

    STCLKRV_Close(HandleL);


#ifdef ARCHITECTURE_ST40
    WTCSR_WRITE(0x00);
    WTCNT_WRITE(0x00);

    STOS_SemaphoreSignal(intmr_sync_p);
#else
    interrupt_unraise(interrupt_wdt);
#endif

    return (OS21_SUCCESS);

}
#endif /* OS21 */

/****************************************************************************
Name         : void GetSTCInterruptTestTaskFunc

Description  : Sets up a task to call raise an interrupt for calling GetSTC
               and displays the contents of a global into which the ISR dumps
               the call contents

Parameters   : *NotUsed : Dummy ptr

Return Value : None
****************************************************************************/
#ifdef ST_OS20
static void GetSTCInterruptTestTaskFunc(void *NotUsed)
{
    S32 IntReturn;
    U32 STCInterruptCount;
    BOOL InterruptSTCSecondErr;

    STTBX_Print(( "GetSTCInterruptTestTaskFunc now running ...\n"));
    /* Setup interrupt for GetSTC call from Interrupt test */

    GetSTCInterruptTestOK = TRUE;


    interrupt_lock();

    /* Video/Audio interrupt safest for test harness usage */
    IntReturn = interrupt_install (VIDEO_INTERRUPT,
                                   VIDEO_INTERRUPT_LEVEL,
                                   GetSTCInterruptTest,
                                   NULL );
    if(IntReturn == 0 )
    {
        IntReturn = interrupt_enable(VIDEO_INTERRUPT_LEVEL);
        interrupt_unlock();
        if( IntReturn != 0 )
        {
            STTBX_Print(( "***GetSTCInterruptTestTaskFunc : ST_ERROR_INTERRUPT_INSTALL ...\n" ));
            return;
        }
    }
    else
    {
        interrupt_unlock();
        STTBX_Print(( "***GetSTCInterruptTestTaskFunc : ST_ERROR_INTERRUPT_INSTALL ...\n" ));
        return;
    }


    IntReturn = 0;
    STCInterruptCount = 0;
    InterruptSTCSecondErr = FALSE;

    while ((STCInterruptCount < 20) && (IntReturn == 0))
    {
        IntReturn = interrupt_raise_number(VIDEO_INTERRUPT);
        STTBX_Print( ("Interrupt STC    : %12u\n",InterruptSTC));
        if (InterruptSTC == 0)
        {
            /* if errored previously...not discontinuity/glitch so error */
            if (InterruptSTCSecondErr)
                GetSTCInterruptTestOK = FALSE;
            else
                InterruptSTCSecondErr = TRUE;
        }
        else
        {
            InterruptSTCSecondErr = FALSE;
        }

        STCInterruptCount++;
        STOS_TaskDelay(TICKS_PER_SECOND);

    }

    if ((interrupt_disable(VIDEO_INTERRUPT_LEVEL)) != 0)
       STTBX_Print(("GetSTCInterrupt interrupt disable failed\n"));

    if ((interrupt_uninstall(VIDEO_INTERRUPT, VIDEO_INTERRUPT_LEVEL)) != 0)
        STTBX_Print(("GetSTCInterrupt uninstall failed\n"));
}


/****************************************************************************
Name         : void GetExtSTCInterruptTestTaskFunc

Description  : Sets up a task to call raise an interrupt for calling
               GetExtendedSTC and displays the contents of a global into which
               the ISR dumps the call contents

Parameters   : *NotUsed : Dummy ptr

Return Value : None
****************************************************************************/
static void GetExtSTCInterruptTestTaskFunc(void *NotUsed)
{
    S32 IntReturn;
    U32 ExtSTCInterruptCount;
    BOOL InterruptSTCSecondErr;

    STTBX_Print(( "GetExtSTCInterruptTestTaskFunc now running ...\n"));
    /* Setup interrupt for GetSTC call from Interrupt test */

    GetExtSTCInterruptTestOK = TRUE;

    interrupt_lock();

    /* Video/Audio interrupt safest for test harness usage */
    IntReturn = interrupt_install (AUDIO_INTERRUPT,
                                   AUDIO_INTERRUPT_LEVEL,
                                   GetExtSTCInterruptTest,
                                   NULL );
    if(IntReturn == 0 )
    {
        IntReturn = interrupt_enable(AUDIO_INTERRUPT_LEVEL);
        interrupt_unlock();
        if( IntReturn != 0 )
        {
            STTBX_Print(( "***GetExtSTCInterruptTestTask : ST_ERROR_INTERRUPT_INSTALL ...\n" ));
            return;
        }
    }
    else
    {
        interrupt_unlock();
        STTBX_Print(( "***GetExtSTCInterruptTestTask : ST_ERROR_INTERRUPT_INSTALL ...\n" ));
        return;
    }


    IntReturn = 0;
    ExtSTCInterruptCount = 0;
    InterruptSTCSecondErr = FALSE;

    while ((ExtSTCInterruptCount < 20) && (IntReturn == 0))
    {
        IntReturn = interrupt_raise_number(AUDIO_INTERRUPT);
        STTBX_Print( ("Interrupt ExtSTC : %12u\n",InterruptExtSTC.BaseValue));
        if (InterruptExtSTC.BaseValue == 0)
        {
            if (InterruptSTCSecondErr)
                GetExtSTCInterruptTestOK = FALSE;
            else
                InterruptSTCSecondErr = TRUE;
        }
        else
        {
            InterruptSTCSecondErr = FALSE;
        }

        ExtSTCInterruptCount++;
        STOS_TaskDelay(TICKS_PER_SECOND);

    }

    if ((interrupt_disable(AUDIO_INTERRUPT_LEVEL)) != 0)
       STTBX_Print(("GetExtSTCInterrupt interrupt disable failed\n"));

    if ((interrupt_uninstall(AUDIO_INTERRUPT, AUDIO_INTERRUPT_LEVEL)) != 0)
        STTBX_Print(("GetExtSTCInterrupt uninstall failed\n"));

}


/****************************************************************************
Name         : static void GetSTCInterruptTest

Description  : ISR for call GetSTC

Parameters   : *NotUsed : Dummy ptr

Return Value : None from function, but global values of STC, ExtSTC and
               Test error flag updated for assessment later.
****************************************************************************/
static void GetSTCInterruptTest(void *NotUsed)
{
    /* should be same name as running task */

    STCLKRV_OpenParams_t  OpenParams_s;
    static STCLKRV_Handle_t ClkrvHandle;
    ST_ErrorCode_t IntTestRetError = LOCAL_ERROR_INTERRUPT_STC;

    InterruptSTC = 0;

    STCLKRV_Open(DeviceNam, &OpenParams_s, &ClkrvHandle);

    IntTestRetError = STCLKRV_GetSTC(ClkrvHandle, &InterruptSTC);
    if (IntTestRetError != ST_NO_ERROR)
    {
        InterruptSTC = 0;
    }

    STCLKRV_Close(ClkrvHandle);

    interrupt_clear_number(VIDEO_INTERRUPT);

}

/****************************************************************************
Name         : static void GetExtSTCInterruptTest

Description  : ISR to call GetExtSTC

Parameters   : *NotUsed : Dummy ptr

Return Value : None from function, but global values of STC, ExtSTC and
               Test error falg updated for assessment later.
****************************************************************************/
static void GetExtSTCInterruptTest(void *NotUsed)
{
    /* should be same name as running task */

    STCLKRV_OpenParams_t  OpenParams_s;
    static STCLKRV_Handle_t ClkrvHandle;
    ST_ErrorCode_t IntTestRetError = LOCAL_ERROR_INTERRUPT_STC;

    InterruptExtSTC.BaseValue = 0;
    InterruptExtSTC.BaseBit32 = 0;

    STCLKRV_Open(DeviceNam, &OpenParams_s, &ClkrvHandle);

    IntTestRetError = STCLKRV_GetExtendedSTC(ClkrvHandle, &InterruptExtSTC);
    if (IntTestRetError != ST_NO_ERROR)
    {
        InterruptExtSTC.BaseValue = 0;
    }

    STCLKRV_Close(ClkrvHandle);

    interrupt_clear_number(AUDIO_INTERRUPT);

}
#endif
#endif /* ifndef RESOURCE_USAGE_TESTS */

/****************************************************************************
Name         : void  display_events

Description  : Task for displaying message when an event from the clkrv
               driver is recieved.

Parameters   : *pDummy : Dummy ptr, not used

Return Value : None, but global count incremented with each event.

****************************************************************************/
void  display_events( void *pDummy )
{
    STEVT_EventConstant_t NewEvent;

    STTBX_Print(( "display_events() now running ...\n"));
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STTBX_Print(( "display_events() now running ...\n"));
    while( 1 )
    {
        STOS_SemaphoreWait(event_counter_p);
#endif
        /* Get new event from queue */

        NewEvent = *pReadEventRxd++;

        /* Check for pointer wraparound */

        if( pReadEventRxd >= &EventsRxd[EVENT_QUEUE_SIZE] )
          pReadEventRxd = EventsRxd;

        switch ( NewEvent )
        {
          case STCLKRV_PCR_VALID_EVT:
            STTBX_Print( ("**STCLKRV_PCR_VALID_EVT** event received\n") );
            break;

          case STCLKRV_PCR_DISCONTINUITY_EVT:
            STTBX_Print( ("**STCLKRV_PCR_DISCONTINUITY_EVT** event received\n") );
            break;

          case STCLKRV_PCR_GLITCH_EVT:
            STTBX_Print( ("**STCLKRV_PCR_GLITCH_EVT event** received\n") );
            break;

          default:
            STTBX_Print( ("**ERROR**: Unknown STCLKRV event received\n") );
            break;
        }
        EventsDisplayed++;
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    }
#endif
}

/****************************************************************************
Name         : STCLKRVT_EventCallback()

Description  : Callback function for STCLKRV events

Parameters   : STEVT_CallReason_t Reason      Reason for callback
               ST_DeviceName_t RegistrantName Instance ID
               STEVT_EventConstant Event      Event ID
               const void *EventData          Event associated data
               *SubscriberData_p              Associated data

Return Value : None
 ****************************************************************************/
void STCLKRVT_EventCallback( STEVT_CallReason_t Reason,
                             const ST_DeviceName_t RegistrantName,
                             STEVT_EventConstant_t Event,
                             const void *EventData,
                             const void *SubscriberData_p)

{
    /* Only one instance, so should be true alays... */
    if (strcmp(RegistrantName,DeviceNam) == 0)
    {

        /* Add new event to queue */

        *pWriteEventRxd++ = Event;

        /* Check for pointer wraparound */

        if( pWriteEventRxd >= &EventsRxd[EVENT_QUEUE_SIZE] )
            pWriteEventRxd = EventsRxd;

        /* Signal new event in queue */
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
        STOS_SemaphoreSignal(event_counter_p);
#else
        display_events(NULL);
#endif
    }
}


/****************************************************************************
Name         : ErrorReport()

Description  : Outputs the appropriate error message string.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : ST_ErrorCode_t
 ****************************************************************************/

void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr )
{
    U32             i;
    ST_ErrorCode_t  Temp = ErrorGiven;

    for( i = 0; i < 2; i++ )
    {

        switch( Temp )
        {
            case ST_NO_ERROR:
                STTBX_Print(( "ST_NO_ERROR - Successful Return\n" ));
                break;

            case ST_ERROR_ALREADY_INITIALIZED:
                STTBX_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
                break;

            case ST_ERROR_UNKNOWN_DEVICE:
                STTBX_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
                break;

            case ST_ERROR_INVALID_HANDLE:
                STTBX_Print(( "ST_ERROR_INVALID_HANDLE - Handle number not recognized\n" ));
                break;

            case ST_ERROR_OPEN_HANDLE:
                STTBX_Print(( "ST_ERROR_OPEN_HANDLE - unforced Term with handle open\n" ));
                break;

            case ST_ERROR_BAD_PARAMETER:
                STTBX_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
                break;

            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                STTBX_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
                break;

            case ST_ERROR_NO_MEMORY:
                STTBX_Print(( "ST_ERROR_NO_MEMORY - Not enough space for dynamic memory allocation\n" ));
                break;

            case ST_ERROR_NO_FREE_HANDLES:
                STTBX_Print(( "ST_ERROR_NO_FREE_HANDLES - Channel already Open\n" ));
                break;

            case ST_ERROR_INTERRUPT_INSTALL:
                STTBX_Print(( "ST_ERROR_INTERRUPT_INSTALL - Interrupt install failed\n" ));
                break;

            case ST_ERROR_INTERRUPT_UNINSTALL:
                STTBX_Print(( "ST_ERROR_INTERRUPT_UNINSTALL - Interrupt uninstall failed\n" ));
                break;

            case  ST_ERROR_TIMEOUT:
                STTBX_Print(( "ST_ERROR_TIMEOUT - Timeout occured\n" ));
                break;

            case ST_ERROR_DEVICE_BUSY :
                STTBX_Print(( "ST_ERROR_DEVICE_BUSY - Device is currently busy \n" ));
                break;

            case STCLKRV_ERROR_HANDLER_INSTALL:
                STTBX_Print(( "STCLKRV_ERROR_HANDLER_INSTALL - Action_PCR not (de)installed\n" ));
                break;

            case STCLKRV_ERROR_PCR_UNAVAILABLE:
                STTBX_Print(( "STCLKRV_ERROR_PCR_UNAVAILABLE - PCR can't be returned\n" ));
                break;

            case STCLKRV_ERROR_EVT_REGISTER:
                STTBX_Print(( "STCLKRV_ERROR_EVT_REGISTER - Error occured in sub component EVT\n" ));
                break;
            case LOCAL_ERROR_FREERUN_INC:
                STTBX_Print(( "FREERUN INCREMENT TEST FAILED: Unexpected difference between consecutive STC or clocks values encountered\n" ));
                break;
            case LOCAL_ERROR_INTERRUPT_STC:
                STTBX_Print(( "STC CALLS FROM INTERRUPT TEST FAILED: Conescutive invalid STC or ExtSTC values returned\n" ));
                break;

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_5188) && !defined (ST_7200)
            case STMERGE_ERROR_NOT_INITIALIZED:
                STTBX_Print(( "STMERGE_ERROR_NOT_INITIALIZED - Driver in not initialised\n"));
                break;

            case STMERGE_ERROR_BYPASS_MODE:
                STTBX_Print(( "STMERGE_ERROR_BYPASS_MODE - Driver in bypass mode,Requested feature not supported\n"));
                break;

            case STMERGE_ERROR_DEVICE_NOT_SUPPORTED:
                STTBX_Print(( "STMERGE_ERROR_DEVICE_NOT_SUPPORTED - The Specified DeviceType is not supported\n"));
                break;

            case STMERGE_ERROR_INVALID_BYTEALIGN:
                STTBX_Print(( "STMERGE_ERROR_INVALID_BYTEALIGN - Byte aliignment of SOP requested in parallel mode\n"));
                break;

            case STMERGE_ERROR_INVALID_PRIORITY:
                STTBX_Print(( "STMERGE_ERROR_INVALID_PRIORITY - Priority is not valid \n"));
                break;

            case STMERGE_ERROR_ILLEGAL_CONNECTION:
                STTBX_Print(( "STMERGE_ERROR_ILLEGAL_CONNECTION - Invalid Connection\n"));
                break;

            case STMERGE_ERROR_INVALID_COUNTER_RATE:
                STTBX_Print(( "STMERGE_ERROR_INVALID_COUNTER_RATE - Counter Increment rate is out of range\n"));
                break;

            case STMERGE_ERROR_INVALID_COUNTER_VALUE:
                STTBX_Print(( "STMERGE_ERROR_INVALID_COUNTER_VALUE - Counter Value is out of range\n"));
                break;

            case STMERGE_ERROR_INVALID_DESTINATION_ID:
                STTBX_Print(( "STMERGE_ERROR_INVALID_DESTINATION_ID - Destination Object Id is not a valid destination\n"));
                break;

            case STMERGE_ERROR_INVALID_ID:
                STTBX_Print(( "STMERGE_ERROR_INVALID_ID - Object Id is not recognized\n"));
                break;

            case STMERGE_ERROR_INVALID_LENGTH:
                STTBX_Print(( "STMERGE_ERROR_INVALID_LENGTH - Packet length is out of range\n"));
                break;

            case STMERGE_ERROR_INVALID_MODE:
                STTBX_Print(( "STMERGE_ERROR_INVALID_MODE - Requested mode is not supported\n"));
                break;

            case STMERGE_ERROR_INVALID_PACE:
                STTBX_Print(( "STMERGE_ERROR_INVALID_PACE - Pace value out of range\n"));
                break;

            case STMERGE_ERROR_INVALID_PARALLEL:
                STTBX_Print(( "STMERGE_ERROR_INVALID_PARALLEL - TSIN instance can only operate in SERIAL mode\n"));
                break;

            case STMERGE_ERROR_INVALID_SOURCE_ID:
                STTBX_Print(( "STMERGE_ERROR_INVALID_SOURCE_ID - Source Object Id is not a valid Source\n"));
                break;

            case STMERGE_ERROR_INVALID_SYNCDROP:
                STTBX_Print(( "STMERGE_ERROR_INVALID_SYNCDROP - Sync drop is out of range\n"));
                break;

            case STMERGE_ERROR_INVALID_SYNCLOCK:
                STTBX_Print(( "STMERGE_ERROR_INVALID_SYNCLOCK - Sync lock is out of range\n"));
                break;

            case STMERGE_ERROR_INVALID_TAGGING:
                STTBX_Print(( "STMERGE_ERROR_INVALID_TAGGING - Tagging value is invalid\n"));
                break;

            case STMERGE_ERROR_MERGE_MEMORY:
                STTBX_Print(( "STMERGE_ERROR_MERGE_MEMORY - Merge Memory Map is invalid\n"));
                break;
#endif

            case STCLKRV_ERROR_INVALID_FREQUENCY:
                STTBX_Print(( "STCLKRV_ERROR_INVALID_FREQUENCY - Bad Slave Frequency\n" ));
                break;

            case STCLKRV_ERROR_INVALID_SLAVE:
                STTBX_Print(( "STCLKRV_ERROR_INVALID_SLAVE - Invalid Slave\n" ));
                break;

            default:
                STTBX_Print(( "*** Unrecognised return code : 0x%x (%d) ***\n",ErrorGiven, ErrorGiven ));
        }

        if( ErrorGiven == ExpectedErr )
        {
            break;
        }
        else
        {
            if( i == 0 )                            /* first pass? */
            {
                Temp = ExpectedErr;
                STTBX_Print(( ">>>>> ERROR!!\nMismatch, expected code was:\n" ));
            }
        }
    } /* end Loop */

/* if ErrorGiven does not match ExpectedErr, update
       ErrorStore only if no previous error was noted */

    if( ErrorGiven != ExpectedErr )
    {
        /* Don't touch passed otherwise - needs to be set to 'true' before
         * each batch of tests
         */
        Passed = FALSE;

        if( *ErrorStore == ST_NO_ERROR )
        {
            *ErrorStore = ErrorGiven;
        }
    }

    STTBX_Print(( "----------------------------------------------------------\n" ));
}

/****************************************************************************
Name         : PrintPassFail()

Description  : Print "PASSED: <text>" or "FAILED: <text>" depending on the
               value of 'passed'. Also increments the appropriate counter
               in 'result' accordingly.

Parameters:
               Passed      - whether the test(s) passed or failed
               PassMsg     - message to print when passed
               FailMsg     - message to print when failed

Return Value : NONE

See Also     :
 ****************************************************************************/

void PrintPassFail(BOOL LocalPassed, char *PassMsg, char *FailMsg)
{
    STTBX_Print(("\n==========================================================\n"));
    if (LocalPassed == TRUE)
    {

        STTBX_Print(("PASSED: %s\n", PassMsg));
        Result.NumberPassed++;
    }
    else
    {
        STTBX_Print(("FAILED: %s\n", FailMsg));
        Result.NumberFailed++;
    }

    /* Always display number passed */
    STTBX_Print(( "PASSED: (%d)\n", Result.NumberPassed));

    /* Only display failed if failures occur */
    if( Result.NumberFailed != 0 )
    {
        STTBX_Print(( "FAILED: (%d)\n", Result.NumberFailed));
    }
    STTBX_Print(("==========================================================\n\n"));

}


#ifdef RESOURCE_USAGE_TESTS

/****************************************************************************
Name         : StackTest_Overhead

Description  : Calling as part of stack test shows the OS/20  overhead that
               should be subtracted from the other tests.

****************************************************************************/
static void StackTest_Overhead(void *dummy)
{

}

/****************************************************************************
Name         : StackTest_Init

Description  : Performs an init of the driver to show the stack used by the
               on initialisation.

****************************************************************************/
static void StackTest_Init(void *dummy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    ST_ErrorCode_t StoredError;
    static STCLKRV_InitParams_t    CLKRVInitParams;

    /* Initialize our native Clock Recovery Driver */
    CLKRVInitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams.PCRDriftThres       = TEST_PCR_DRIFT_THRES;
    CLKRVInitParams.MinSampleThres      = TEST_MIN_SAMPLE_THRES;
    CLKRVInitParams.MaxWindowSize       = TEST_MAX_WINDOW_SIZE;
    CLKRVInitParams.Partition_p         = TEST_PARTITION_1;
    CLKRVInitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams.FSBaseAddress_p     = (U32 *)CKG_BASE_ADDRESS;
#if defined(ST_7100) || defined(ST_7109)
    CLKRVInitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    CLKRVInitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitParams.PCREvtHandlerName, PTIEVTDevNam );
    strcpy( CLKRVInitParams.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitParams.PTIDeviceName, STPTI_DeviceName );

    Error = STCLKRV_Init( DeviceNam, &CLKRVInitParams );

    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error initialising driver\n"));
        ErrorReport( &StoredError, Error, ST_NO_ERROR );
    }

}

/****************************************************************************
Name         : StackTest_Typical

Description  : Performs some "typical" use of the driver to determine an
               amount of stack consumed during standard operations.

****************************************************************************/
static void StackTest_Typical(void *dummy)
{


    ST_ErrorCode_t Error = ST_NO_ERROR;
    STCLKRV_ExtendedSTC_t ExtendedSTC;
    U32 STC;
    U8 i;

    STCLKRV_Handle_t CLKHandle;
    STCLKRV_OpenParams_t    OpenParams_s;
    STCLKRV_ExtendedSTC_t Baseline;

    /* Open the Clock Recovery Driver */
    Error = STCLKRV_Open( DeviceNam, &OpenParams_s, &CLKHandle );
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error Opening  driver\n"));
    }

    /* PCR processing...*/

    /* STC Baseline and freerun */
    STTBX_Print(( "Disabling and Baselining\n" ));
    Error = STCLKRV_Disable(CLKHandle);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error using driver\n"));
    }

    /* Set Baseline value */
    Baseline.BaseBit32 = 0;
    Baseline.BaseValue = 22222222;
    Baseline.Extension = 0;
    Error = STCLKRV_SetSTCBaseline(CLKHandle, &Baseline);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error using driver\n"));
    }

    /* Enable baslining */
    Error = STCLKRV_SetSTCSource(CLKHandle, STCLKRV_STC_SOURCE_BASELINE);
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error SetSTCSource  0x%x\n",Error));
    }

    for (i = 0; i != 5; i++)
    {
        Error = STCLKRV_GetExtendedSTC( CLKHandle, &ExtendedSTC );
        if (Error != ST_NO_ERROR )
        {
            STTBX_Print(("Error GetExtSTC 0x%x\n",Error));
        }


        Error = STCLKRV_GetSTC(CLKHandle, &STC);
        if (Error != ST_NO_ERROR)
        {
            STTBX_Print(("Error GetSTC\n"));
        }

        STOS_TaskDelay(DELAY_PERIOD/10);
    }



    Error = STCLKRV_Close(CLKHandle);

    /* check error condition */
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error closing driver 0x%x\n",Error));
    }


}

/****************************************************************************
Name         : StackTest_Term

Description  : Performs a standard terminate.

****************************************************************************/
static void StackTest_Term(void *dummy)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    static STCLKRV_TermParams_t    CLKTermParams_s;

    TermParams_s.ForceTerminate = FALSE;
    Error = STCLKRV_Term( DeviceNam, &CLKTermParams_s );
    if (Error != ST_NO_ERROR)
    {
        STTBX_Print(("RSRC TEST: Error terminating driver 0x%x\n",Error));
    }
}
#if 0 /* development use only */
/****************************************************************************
Name         : DisplayPartitionStatus

Description  : Displays the memory partition status
****************************************************************************/
static void DisplayPartitionStatus(partition_status_t *Partition_p)
{

    STTBX_Print((" State: "));
    switch (Partition_p->partition_status_state)
    {
    case partition_status_state_valid :
        STTBX_Print(("   VALID\n"));
        break;
    case partition_status_state_invalid :
        STTBX_Print(("  INVALID\n"));
        break;
    default :
        STTBX_Print(("  UNKNOWN\n"));
        break;
    }

    STTBX_Print((" Type: "));
    switch (Partition_p->partition_status_type)
    {
    case partition_status_type_simple :
        STTBX_Print(("    Simple\n"));
        break;
    case partition_status_type_fixed :
        STTBX_Print(("    Fixed\n"));
        break;
    case partition_status_type_heap :
        STTBX_Print(("    Heap\n"));
        break;
    default :
        STTBX_Print(("    UNKNOWN\n"));
        break;
    }

    STTBX_Print((" Size:     %d\n",Partition_p->partition_status_size));
    STTBX_Print((" Free:     %d\n",Partition_p->partition_status_free));
    STTBX_Print((" Largest:  %d\n",Partition_p->partition_status_free_largest));
    STTBX_Print((" Used:     %d\n",Partition_p->partition_status_used));

}
#endif

/****************************************************************************
Name         : StackTest_Main()

Description  : Stack consumption test entry point. Sets up a task and calls
               the necessary functions showing stack consumed after each call.
Note: This test and associated functions uses globals so as not report stack
consumption of non driver related variables. Global used begin RescTest
****************************************************************************/
static BOOL StackTest_Main(void)
{
    U32 i =0;
#ifndef ST_OS21
    task_t task;
    tdesc_t tdesc;
    void *stack_p;
#endif
    task_t *task_p;
    task_status_t status;
    partition_status_t    MemBeforeInit;
    partition_status_t    MemAfterInit;
    partition_status_t    MemAfterTerm;
    void (*func_table[])(void *) =
    {
        StackTest_Overhead,
        StackTest_Init,
        StackTest_Typical,
        StackTest_Term,
        NULL
    };
    void (*func)(void *);
    ST_ErrorCode_t        StoredError = ST_NO_ERROR;
    ST_ErrorCode_t        ReturnError;
#if !defined (ST_5105) && !defined (ST_5107)
    ST_DeviceName_t         STMERGE_Name;
    STMERGE_InitParams_t    STMERGE_InitParams;
#endif


#if defined(DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4)
    STPTI_OpenParams_t    STPTIOpenParams;
    STPTI_TermParams_t    STPTITermParams;
    ST_DeviceName_t       STPTI_DeviceName;
    STPTI_Handle_t        PTISessionHandle;
    STPTI_SlotData_t      SlotData;
    STPTI_Slot_t          PcrSlot;
    STEVT_InitParams_t    PTIEVTInitPars;
#else
    BOOL                  RetErr;
#endif

    Result.NumberPassed = Result.NumberFailed = 0;
    Passed = TRUE;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(("----------------------------------------------------------\n"));
    STTBX_Print(("TEST: RESOURCE USAGE and MEMEORY LEAK TEST.............\n"));
    STTBX_Print(("----------------------------------------------------------\n\n"));


    /* Name clock recovery driver */
    strcpy( DeviceNam, "STCLKRV" );

    /* Initialize the sub-ordinate EVT Driver */
    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for clkrv events...\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


#if !defined (ST_5105) && !defined (ST_5107)
    /*Init params Setting*/
    STMERGE_InitParams.DeviceType        = STMERGE_DEVICE_1;
    STMERGE_InitParams.BaseAddress_p     = (U32 *)TEST_TSMERGE_BASE_ADDRESS;
    STMERGE_InitParams.MergeMemoryMap_p  = STMERGE_DEFAULT_MERGE_MEMORY_MAP;
    STMERGE_InitParams.Mode              = STMERGE_BYPASS_TSIN_TO_PTI_ONLY;
    STMERGE_InitParams.DriverPartition_p = TEST_PARTITION_1;

    /*Init merge*/
    strcpy(STMERGE_Name,"TSMERGE");
    STTBX_Print(("Calling STMERGE Init....\n"));
    ReturnError = STMERGE_Init(STMERGE_Name,&STMERGE_InitParams);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    /* Initialize the EVT Driver for PTI event handling*/
    strcpy( PTIEVTDevNam, "STEVTpti" );
    PTIEVTInitPars.EventMaxNum = 50;
    PTIEVTInitPars.ConnectMaxNum = 50;
    PTIEVTInitPars.SubscrMaxNum = 20; /* Modified from 3 for new EVT handler */
    PTIEVTInitPars.MemoryPartition = TEST_PARTITION_1;
    PTIEVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for PTI\n" ));
    ReturnError = STEVT_Init( PTIEVTDevNam, &PTIEVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize init parameters for stpti */
    /* Initialise STPTI */
    strcpy( STPTI_DeviceName, "STPTI");
    CLKRV_STPTI_InitSetup(&STPTIInitParams);

    STTBX_Print(( "Calling STPTI_Init() ................\n" ));
    ReturnError = STPTI_Init(STPTI_DeviceName, &STPTIInitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* open session */
        STPTIOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STPTIOpenParams.NonCachedPartition_p =  TEST_PARTITION_2;
        STTBX_Print(( "Calling STPTI_Open() ................\n" ));
        ReturnError = STPTI_Open(STPTI_DeviceName, &STPTIOpenParams, &PTISessionHandle);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }


    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotData.SlotType = STPTI_SLOT_TYPE_PCR;
        SlotData.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotData.SlotFlags.CollectForAlternateOutputOnly = FALSE;
        SlotData.SlotFlags.StoreLastTSHeader = FALSE;
        SlotData.SlotFlags.InsertSequenceError = FALSE;
        SlotData.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
        SlotData.SlotFlags.OutPesWithoutMetadata = FALSE;
        SlotData.SlotFlags.ForcePesLengthToZero = FALSE;
        SlotData.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;

        STTBX_Print(( "Calling STPTI_SlotAllocate() ..........\n" ));
        ReturnError = STPTI_SlotAllocate(PTISessionHandle, &PcrSlot, &SlotData);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
         /* Set PCR pid */
        STTBX_Print(( "Calling STPTI_SlotSetPid() ..........\n" ));
        ReturnError = STPTI_SlotSetPid(PcrSlot,PCR_PID);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
    event_counter_p = STOS_SemaphoreCreateFifo(NULL,0);

    /* Subscribe to PCR Events now */
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    STTBX_Print(("----------------------------------------------------------\n"));
    STTBX_Print(("TEST: MEMEORY LEAK TEST.............\n"));
    STTBX_Print(("----------------------------------------------------------\n\n"));



    /* Get partition details */
    if (partition_status(TEST_PARTITION_2, &MemBeforeInit, 0) == -1)
    {
        STTBX_Print(("*** ERROR: Could not get partition status\n"));
    }

/*    DisplayPartitionStatus(&MemBeforeInit);*/

    for (i=0; i!=4; i++)
    {
        func = func_table[i];
        /* Start the task */
        STOS_TaskCreate((void(*)(void *))func,
                        NULL,
                        TEST_PARTITION_1,
                        STACKTEST_STACK_SIZE,
                        &Stack_p,
                        TEST_PARTITION_1,
                        &task_p,
                        &tdesc,
                        MAX_USER_PRIORITY,
                        "stack_test",
                        (task_flags_t)0);

        /* Wait for task to complete */
        STOS_TaskWait(&task_p,TIMEOUT_INFINITY);

        /* Dump stack usage */
        task_status(task_p, &status, task_status_flags_stack_used);
        STTBX_Print(("Stack and memory usage figures are:\n"));
        switch(i)
        {
        case 0 :
            STTBX_Print(("Overhead: %d\n",status.task_stack_used));
            break;
        case 1:
            STTBX_Print(("Init: %d\n",status.task_stack_used));
            /* Get partition details...must use same partition as Init did! */
            if (partition_status(TEST_PARTITION_2, &MemAfterInit, 0) == -1)
            {
                STTBX_Print(("*** ERROR: Could not get partition status\n"));
            }
            /* compare details */
            STTBX_Print(("Memory usage: %d bytes\n\n",MemAfterInit.partition_status_used-
                         MemBeforeInit.partition_status_used));
            break;
        case 2:
            STTBX_Print(("Typical: %d\n",status.task_stack_used));
            break;
        case 3:
            STTBX_Print(("Term: %d\n",status.task_stack_used));
            /* Get partition details */
            if (partition_status(TEST_PARTITION_2, &MemAfterTerm, 0) == -1)
            {
                Passed = FALSE;
                STTBX_Print(("*** ERROR: Could not get partition status\n"));
            }
            /* Compare */
            if ((MemBeforeInit.partition_status_used-MemAfterTerm.partition_status_used) != 0)
            {
                Passed = FALSE;
                STTBX_Print(("****ERROR: Memory leak!!\n"));
            }

            PrintPassFail(Passed, "Leak test Passed", "MEMORY LEAK!!!");

            break;
        default:
            /* no action */
            break;
        }

        /* Tidy up */
        task_delete(task_p);
    }

#if !defined (ST_5105) && !defined (ST_5107)
    /*Term STMERGE */
    ReturnError = STMERGE_Term(STMERGE_Name, NULL);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() .for PTI........\n" ));
    ReturnError = STEVT_Term( PTIEVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STPTITermParams.ForceTermination = TRUE;
    STTBX_Print(("Calling STPTI_Term() forced.....\n"));
    ReturnError = STPTI_Term(STPTI_DeviceName,&STPTITermParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Output running analysis */
    STTBX_Print(("**********************************************************\n"));
    STTBX_Print(("PASSED: %3d\n", Result.NumberPassed));
    STTBX_Print(("FAILED: %3d\n", Result.NumberFailed));
    STTBX_Print(("**********************************************************\n"));

    return TRUE;
}



#endif  /* Resource usage testing code */

/* End of stclkpti.c module */

