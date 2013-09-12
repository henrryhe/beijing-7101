/****************************************************************************

File Name   : clkdualpti.c

Description : Clock Recovery API Dual clock recovery Test Routines using STPTI

Copyright (C) 2007, ST Microelectronics

References  :

$ClearCase (VOB: stclkrv)

NOTE: This test file is ONLY compatible with multi instance STPTI compatible
clock recvovery software
****************************************************************************/


/* Includes --------------------------------------------------------------- */

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

#if !defined (ST_OSLINUX)
#define CLKRV_MAX_SAMPLES  5000
extern U32 ClkrvDbg[13][CLKRV_MAX_SAMPLES];
extern U32 SampleCount;
#endif

/* Private Constants ------------------------------------------------------ */

/* TWO PCR slots for two clock recovery */

#ifndef PCR_PID1              /* This can be overriden on the command line */
#define PCR_PID1 8190           /* mux1_188.trp  */
#endif

#ifndef PCR_PID2              /* This can be overriden on the command line */
#define PCR_PID2 8190           /* mux1_188.trp  */
#endif

/* delay */
#define DELAY_PERIOD            ST_GetClocksPerSecond()
#define TICKS_PER_SECOND        ST_GetClocksPerSecond()

/* Event message queue */
#define TEST_EVT_CONNECTIONS    20

#define START_OF_TS_PACKET      0x47
#define TEST_SYNC_LOCK             0
#define TEST_SYNC_DROP             0

/* Test clock recovery init params defines */
#define TEST_PCR_DRIFT_THRES      400
#define TEST_MIN_SAMPLE_THRES     10
#define TEST_MAX_WINDOW_SIZE      50

#define CLK_REC_TRACKING_INTERRUPT_LEVEL 7

#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_TRACKING_INTERRUPT_1  MPEG_CLK_REC_1_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7200
#define AUD_PCM_CLK_FS_BASE_ADDRESS   AUDIO_IF_BASE_ADDRESS


/* STC value store count */
#define MAX_STC_COUNT           50

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE     (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE       0x0200000
#define NCACHE_PARTITION_SIZE       0x100000


/* Private Types ---------------------------------------------------------- */

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


static U32              TestsFailed = 0;  /* Overall failure count*/
static ST_ErrorCode_t   retVal;

static BOOL ErrorReportSeparator = TRUE;


/* Variables related to STC tasks */
#if !defined (ST_OSLINUX)
static STCLKRV_ExtendedSTC_t ExtendedSTC[MAX_STC_COUNT];
static U32 STC[MAX_STC_COUNT];

#ifdef ST_OS20
static tdesc_t GetSTCTestTaskDescriptor;
static void *GetSTCTestTaskStack_p;

static tdesc_t GetExtSTCTestTaskDescriptor;
static void *GetExtSTCTestTaskStack_p;
#endif

task_t *GetSTCTestTask_p;
task_t *GetExtSTCTestTask_p;
#endif

#define CLKRV_MAX_SAMPLES  5000
extern U32 ClkrvDbg[13][CLKRV_MAX_SAMPLES];
extern U32 SampleCount;

extern U32 ClkrvDbg2[13][CLKRV_MAX_SAMPLES];
extern U32 SampleCount2;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

static void TestDualClockrecoveryWithPCR(void);

/* Task funcion for calling Get STC\extended STC */
#if !defined (ST_OSLINUX)
static void GetSTCTestTaskFunc(void *vData_p);
static void GetExtSTCTestTaskFunc(void *vData_p);
#endif

static STCLKRV_ClockSource_t TestSlaveClockA[4] = { STCLKRV_CLOCK_PCM_0,
                                                    STCLKRV_CLOCK_PCM_2,
                                                    STCLKRV_CLOCK_SPDIF_HDMI_0,
                                                    STCLKRV_CLOCK_HD_0};
U32 TestSlaveFrequencyA[4] = { 3072000, 12300000, 50400000, 148500000};

static STCLKRV_ClockSource_t TestSlaveClockB[4] = { STCLKRV_CLOCK_PCM_1,
                                                    STCLKRV_CLOCK_PCM_3,
                                                    STCLKRV_CLOCK_SPDIF_0,
                                                    STCLKRV_CLOCK_HD_1};
U32 TestSlaveFrequencyB[4] = { 11300000, 45158400, 67737600, 157500000};

/* clock recovery event handler */
static void STCLKRVT_EventCallback( STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData,
                                    const void *SubscriberData_p);

static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr );

/* Exported functions */
extern ST_ErrorCode_t STCLKRV_GetClocksFrequency( STCLKRV_Handle_t Handle,
                                                U32 *pSTCFrequency,
                                                U32 *pFS1Frequency,
                                                U32 *pFS2Frequency,
                                                U32 *pFS3Frequency,
                                                U32 *pFS4Frequency,
                                                U32 *pFS5Frequency,
                                                U32 *pFS6Frequency );

/* Functions -------------------------------------------------------------- */

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
#endif /* LINUX CHANGES */

#if defined (ST_7200)
    STPIO_InitParams_t      PIOInitParams;
    STPIO_OpenParams_t      PIOOpenParams;
    STPIO_Handle_t          PIOHandle;
    STPIO_TermParams_t      PIOTermParams;
#endif

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#ifdef ST_OS21
    /* Create memory partitions */
    system_partition   =  partition_create_heap((U8*)system_block, sizeof(system_block));
    ncache_partition   =  partition_create_heap((U8*)ST40_NOCACHE_NOTRANSLATE(ncache_block),
                                                                         sizeof(ncache_block));
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

#if defined (ST_7200)

    /* To view clocks on the PIO5 pin 2 */
    PIOInitParams.BaseAddress      = (U32*)ST7200_PIO5_BASE_ADDRESS;
    PIOInitParams.InterruptNumber  = ST7200_PIO5_INTERRUPT;
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
        PIOOpenParams.BitConfigure[5]  =  STPIO_BIT_ALTERNATE_OUTPUT;
        PIOOpenParams.ReservedBits     =  0x20;

        retVal = STPIO_Open ( "PIO5",&PIOOpenParams, &PIOHandle );
        if (retVal!= ST_NO_ERROR )
        {
            STCLKRV_Print (("Error: STPIO_Open = 0x%08X\n", retVal ));
        }
    }
#endif /* #if defined(ST_7200) */

    TestDualClockrecoveryWithPCR();

#if defined (ST_7200)
    PIOTermParams.ForceTerminate = FALSE;
    retVal = STPIO_Term("PIO5", &PIOTermParams);
#endif

#ifndef ST_OSLINUX
    retVal = STBOOT_Term( "CLKRVTest", &stboot_TermPars );
#endif

    return retVal;
}

#ifdef DVD_TRANSPORT_STPTI

/***********************************************************************************
* Name:     TestDualClockrecoveryWithPCR
*
* Description:  Tests dual clock recovery with STPTI use requiring packet injector
*
* Notes:        Two clock recovery instances on two pcr slots
*
***********************************************************************************/
static void TestDualClockrecoveryWithPCR()
{

    ST_ErrorCode_t        StoredError = ST_NO_ERROR;
    ST_ErrorCode_t        ReturnError;
    ST_Revision_t         RevisionStr;

    /* STEVT */
    ST_DeviceName_t       EVTDevNam;
    STEVT_TermParams_t    EVTTermPars;
    STEVT_InitParams_t    EVTInitPars;
    STEVT_OpenParams_t    EVTOpenPars;
    STEVT_Handle_t        EVTHandle;

    static const STEVT_DeviceSubscribeParams_t EVTSubscrParams ={
                                         STCLKRVT_EventCallback,
                                         NULL };
    /* STPTI4 */
    /* Two PTI instance required */

    ST_DeviceName_t       STPTIDevNamA;
    ST_DeviceName_t       STPTIDevNamB;

    STPTI_InitParams_t    STPTIInitPars;
    STPTI_OpenParams_t    STPTIOpenPars;
    STPTI_TermParams_t    STPTITermPars;

    STPTI_Handle_t        PTISessionHandleA;
    STPTI_Handle_t        PTISessionHandleB;

#ifdef ST_7200
    STPTI_FrontendParams_t  PTIFrontendParams;
    STPTI_Frontend_t        PTIFrontendHandleA;
    STPTI_Frontend_t        PTIFrontendHandleB;
#endif

    STPTI_SlotData_t      SlotDataA;
    STPTI_Slot_t          PcrSlotA;

    STPTI_SlotData_t      SlotDataB;
    STPTI_Slot_t          PcrSlotB;

    ST_DeviceName_t       PTIEVTDevNamA;
    ST_DeviceName_t       PTIEVTDevNamB;
    STEVT_InitParams_t    PTIEVTInitPars;

    /* Two PCR slots needed for two different PCR PID*/
    U16 PacketCount       = 0;
    STCLKRV_SourceParams_t PCRSrcA;
    STCLKRV_SourceParams_t PCRSrcB;

    /* Two clock recovery instances */

    ST_DeviceName_t         CLKRVDevNamA;
    ST_DeviceName_t         CLKRVDevNamB;

    STCLKRV_Handle_t        CLKRVHandleA;
    STCLKRV_Handle_t        CLKRVHandleB;

    STCLKRV_InitParams_t    CLKRVInitPars;
    STCLKRV_OpenParams_t    CLKRVOpenPars;
    STCLKRV_TermParams_t    CLKRVTermPars;

    U32 STCFreqA, FS1FreqA, FS2FreqA, FS3FreqA, FS4FreqA, FS5FreqA, FS6FreqA;
    U32 STCFreqB, FS1FreqB, FS2FreqB, FS3FreqB, FS4FreqB, FS5FreqB, FS6FreqB;
    U32 i = 0;
    U16 PacketCountA = 0;
    U16 PacketCountB = 0;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STTBX_Print(( "\n==========================================================\n" ));
    STTBX_Print(( "Commencing TestDualDecodeBothPCR Test for STPTI....\n" ));
    STTBX_Print(( "Two instances of Clock recovery are syncronized by two\n" ));
    STTBX_Print(( "Different PCR PID.\n" ));
    STTBX_Print(( "==========================================================\n\n" ));

    /* Name two clock recovery driver */
    strcpy( CLKRVDevNamA, "STCLKRVA" );
    strcpy( CLKRVDevNamB, "STCLKRVB" );

    /* Initialize the sub-ordinate EVT Driver */

    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum   = TEST_EVT_CONNECTIONS;
    EVTInitPars.ConnectMaxNum = TEST_EVT_CONNECTIONS;
    EVTInitPars.SubscrMaxNum  = TEST_EVT_CONNECTIONS;
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag  = STEVT_UNKNOWN_SIZE;

    STTBX_Print(( "Calling STEVT_Init() for clkrv events...\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Subscribe to PCR Events now for clock recovery A */
    STTBX_Print(( "Subscribing Events for Clock Recovery [A]....\n" ));
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             CLKRVDevNamA,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              CLKRVDevNamA,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              CLKRVDevNamA,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Subscribe to PCR Events now for clock recovery B */
    STTBX_Print(( "Subscribing Events for Clock Recovery [B]....\n" ));
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             CLKRVDevNamB,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              CLKRVDevNamB,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              CLKRVDevNamB,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Initialize the EVT Driver for PTI event handling*/
    strcpy( PTIEVTDevNamA, "STEVTptiA" );
    PTIEVTInitPars.EventMaxNum =   50;
    PTIEVTInitPars.ConnectMaxNum = 50;
    PTIEVTInitPars.SubscrMaxNum =  10; /* Modified from 3 for new EVT handler */
    PTIEVTInitPars.MemoryPartition = TEST_PARTITION_1;
    PTIEVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for PTI [A]\n" ));
    ReturnError = STEVT_Init( PTIEVTDevNamA, &PTIEVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Initialize the EVT Driver for PTI event handling*/
    strcpy( PTIEVTDevNamB, "STEVTptiB" );
    PTIEVTInitPars.EventMaxNum =   50;
    PTIEVTInitPars.ConnectMaxNum = 50;
    PTIEVTInitPars.SubscrMaxNum =  10; /* Modified from 3 for new EVT handler */
    PTIEVTInitPars.MemoryPartition = TEST_PARTITION_1;
    PTIEVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init() for PTI [B]\n" ));
    ReturnError = STEVT_Init( PTIEVTDevNamB, &PTIEVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialise STPTI4 */
    strcpy( STPTIDevNamA, "STPTIA");
    memset(&STPTIInitPars, 0, sizeof(STPTI_InitParams_t));
    STPTIInitPars.Device                     = STPTI_DEVICE_PTI_4;
#ifdef STPTI_DTV_SUPPORT
    STPTIInitPars.TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV;
#else
    STPTIInitPars.TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
#endif
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    STPTIInitPars.TCLoader_p                 = STPTI_DVBTCLoaderSL2;
#endif
    STPTIInitPars.InterruptLevel             = PTI_INTERRUPT_LEVEL;
    STPTIInitPars.InterruptNumber            = PTIA_INTERRUPT;
    STPTIInitPars.SyncLock                   = 0;
    STPTIInitPars.SyncDrop                   = 0;
    STPTIInitPars.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_8x8;
    STPTIInitPars.TCDeviceAddress_p          = (U32*) PTIA_BASE_ADDRESS;
    STPTIInitPars.DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitPars.NCachePartition_p          = TEST_PARTITION_2;
    STPTIInitPars.Partition_p                = TEST_PARTITION_1;
    STPTIInitPars.EventProcessPriority       = MIN_USER_PRIORITY;
    STPTIInitPars.InterruptProcessPriority   = MAX_USER_PRIORITY;
    STPTIInitPars.AlternateOutputLatency     = 42;
    STPTIInitPars.StreamID                   = STPTI_STREAM_ID_TSIN0;
    STPTIInitPars.NumberOfSlots              = 10;
#ifdef STPTI_INDEX_SUPPORT
    STPTIInitPars.IndexAssociation           = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitPars.IndexProcessPriority       = MIN_USER_PRIORITY;
#endif
#ifdef STPTI_CAROUSEL_SUPPORT
    STPTIInitPars.CarouselProcessPriority    = MIN_USER_PRIORITY;
    STPTIInitPars.CarouselEntryType          = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif
    STPTIInitPars.PacketArrivalTimeSource = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
    memcpy (&(STPTIInitPars.EventHandlerName), PTIEVTDevNamA, sizeof (ST_DeviceName_t));

#ifdef ST_OSLINUX
    ReturnError = STPTI_TEST_ForceLoader(1);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(( "Calling STPTI_Init() A................\n" ));
    ReturnError = STPTI_Init(STPTIDevNamA, &STPTIInitPars);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize second instance of STPTI4 */
    strcpy( STPTIDevNamB, "STPTIB");

    STPTIInitPars.TCDeviceAddress_p = (U32*) PTIB_BASE_ADDRESS;;
    STPTIInitPars.InterruptNumber   = PTIB_INTERRUPT;
    STPTIInitPars.StreamID          = STPTI_STREAM_ID_TSIN1;
    memcpy(&STPTIInitPars.EventHandlerName, PTIEVTDevNamB, sizeof(ST_DeviceName_t));

#ifdef ST_OSLINUX
    ReturnError = STPTI_TEST_ForceLoader(1);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(( "Calling STPTI_Init() B................\n" ));
    ReturnError = STPTI_Init(STPTIDevNamB, &STPTIInitPars);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* open session */
        STPTIOpenPars.DriverPartition_p    = TEST_PARTITION_1;
        STPTIOpenPars.NonCachedPartition_p =  TEST_PARTITION_2;

        STTBX_Print(( "Calling STPTI_Open() ................\n" ));
        ReturnError = STPTI_Open(STPTIDevNamA, &STPTIOpenPars, &PTISessionHandleA);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

#ifdef ST_7200

    STTBX_Print(( "Calling STPTI_FrontendAllocate() ................\n" ));

    ReturnError = STPTI_FrontendAllocate(PTISessionHandleA, &PTIFrontendHandleA, STPTI_FRONTEND_TYPE_TSIN);
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
    ReturnError = STPTI_FrontendSetParams(PTIFrontendHandleA, &PTIFrontendParams);

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STPTI_FrontendLinkToPTI() ..........\n" ));
    ReturnError = STPTI_FrontendLinkToPTI(PTIFrontendHandleA, PTISessionHandleA);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotDataA.SlotType = STPTI_SLOT_TYPE_PCR;
        SlotDataA.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotDataA.SlotFlags.CollectForAlternateOutputOnly = FALSE;
        SlotDataA.SlotFlags.StoreLastTSHeader = FALSE;
        SlotDataA.SlotFlags.InsertSequenceError = FALSE;
        SlotDataA.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
        SlotDataA.SlotFlags.OutPesWithoutMetadata = FALSE;
        SlotDataA.SlotFlags.ForcePesLengthToZero = FALSE;
        SlotDataA.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
        SlotDataA.SlotFlags.SoftwareCDFifo = FALSE;

        STTBX_Print(( "Calling STPTI_SlotAllocate() ..........\n" ));
        ReturnError = STPTI_SlotAllocate(PTISessionHandleA, &PcrSlotA, &SlotDataA);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
        /* Set PCR pid */
        STTBX_Print(( "Calling STPTI_SlotSetPid() on PCR(%d)..........\n", PCR_PID1));
        ReturnError = STPTI_SlotSetPid(PcrSlotA,PCR_PID1);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    /* Get some packets */
    STOS_TaskDelay(TICKS_PER_SECOND);

    STPTI_SlotPacketCount(PcrSlotA, &PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(( "\n----------------------------------------------------------\n" ));
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n ", PacketCount));
        STTBX_Print(( "----------------------------------------------------------\n\n" ));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        STTBX_Print(("**** TEST CAN NOT BE DONE  **************\n"));
       /* return;*/
    }

    if (ReturnError == ST_NO_ERROR)
    {
        /* open session */
        STPTIOpenPars.DriverPartition_p = TEST_PARTITION_1;
        STPTIOpenPars.NonCachedPartition_p =  TEST_PARTITION_2;

        STTBX_Print(( "Calling STPTI_Open() ................\n" ));
        ReturnError = STPTI_Open(STPTIDevNamB, &STPTIOpenPars, &PTISessionHandleB);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

#ifdef ST_7200

    STTBX_Print(( "Calling STPTI_FrontendAllocate() ................\n" ));

    ReturnError = STPTI_FrontendAllocate(PTISessionHandleB, &PTIFrontendHandleB, STPTI_FRONTEND_TYPE_TSIN);
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
    PTIFrontendParams.u.TSIN.ClkRvSrc = STPTI_FRONTEND_CLK_RCV1;
    PTIFrontendParams.u.TSIN.MemoryPktNum = 48;

    STTBX_Print(( "Calling STPTI_FrontendSetParams() ..........\n" ));
    ReturnError = STPTI_FrontendSetParams(PTIFrontendHandleB, &PTIFrontendParams);

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STPTI_FrontendLinkToPTI() ..........\n" ));
    ReturnError = STPTI_FrontendLinkToPTI(PTIFrontendHandleB, PTISessionHandleB);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    if (ReturnError == ST_NO_ERROR)
    {
        /* Allocate slot type */
        SlotDataB.SlotType = STPTI_SLOT_TYPE_PCR;
        SlotDataB.SlotFlags.SignalOnEveryTransportPacket = FALSE;
        SlotDataB.SlotFlags.CollectForAlternateOutputOnly = FALSE;
        SlotDataB.SlotFlags.StoreLastTSHeader = FALSE;
        SlotDataB.SlotFlags.InsertSequenceError = FALSE;
        SlotDataB.SlotFlags.AlternateOutputInjectCarouselPacket = FALSE;
        SlotDataB.SlotFlags.OutPesWithoutMetadata = FALSE;
        SlotDataB.SlotFlags.ForcePesLengthToZero = FALSE;
        SlotDataB.SlotFlags.AppendSyncBytePrefixToRawData = FALSE;
        SlotDataB.SlotFlags.SoftwareCDFifo = FALSE;

        STTBX_Print(( "Calling STPTI_SlotAllocate() ..........\n" ));
        ReturnError = STPTI_SlotAllocate(PTISessionHandleB, &PcrSlotB, &SlotDataB);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if (ReturnError == ST_NO_ERROR)
    {
         /* Set PCR pid */
        STTBX_Print(( "Calling STPTI_SlotSetPid() on PCR(%d)..........\n", PCR_PID2));
        ReturnError = STPTI_SlotSetPid(PcrSlotB,PCR_PID2);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    /* Get some packets */
    STOS_TaskDelay(TICKS_PER_SECOND);

    PacketCount =0;
    STPTI_SlotPacketCount(PcrSlotB, &PacketCount);
    if (PacketCount != 0)
    {
        STTBX_Print(( "\n----------------------------------------------------------\n" ));
        STTBX_Print(("Packet Count on PCR Slot OK:  %i\n ", PacketCount));
        STTBX_Print(( "----------------------------------------------------------\n\n" ));
    }
    else
    {
        STTBX_Print(("**** NO PACKETS COUNTED ON GIVEN PCR SLOT\n"));
        STTBX_Print(("**** TEST CAN NOT BE DONE  **************\n"));
        /*return;*/
    }

    /* revision string available before STCLKRV Initialized */
    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Initialize our native Clock Recovery Driver */
    CLKRVInitPars.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitPars.PCRDriftThres     = TEST_PCR_DRIFT_THRES;
    CLKRVInitPars.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    CLKRVInitPars.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    CLKRVInitPars.Partition_p       = TEST_PARTITION_1;
    CLKRVInitPars.DeviceType        = CLK_REC_DEVICE_TYPE;
    CLKRVInitPars.FSBaseAddress_p   = (U32 *)CKG_B_BASE_ADDRESS;
    CLKRVInitPars.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
    CLKRVInitPars.DecodeType        = STCLKRV_DECODE_PRIMARY;
#ifdef ST_7200
    CLKRVInitPars.CRUBaseAddress_p  = (U32*)(CLKRV_BASE_ADDRESS);
#endif
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitPars.InterruptLevel    = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitPars.PCREvtHandlerName, PTIEVTDevNamA );
    strcpy( CLKRVInitPars.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitPars.PTIDeviceName, STPTIDevNamA );

    STTBX_Print(( "Calling STCLKRV_Init() [A] ..............\n" ));
    ReturnError = STCLKRV_Init( CLKRVDevNamA, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    CLKRVInitPars.DecodeType = STCLKRV_DECODE_SECONDARY;
#ifdef ST_7200
    CLKRVInitPars.CRUBaseAddress_p  = (U32*)(CLKRV1_BASE_ADDRESS);
#endif
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT_1;
    strcpy(CLKRVInitPars.PCREvtHandlerName, PTIEVTDevNamB);
    strcpy( CLKRVInitPars.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitPars.PTIDeviceName, STPTIDevNamB );

    STTBX_Print(( "Calling STCLKRV_Init() [B]..............\n" ));
    ReturnError = STCLKRV_Init( CLKRVDevNamB, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() [A]..............\n" ));
    ReturnError = STCLKRV_Open( CLKRVDevNamA, &CLKRVOpenPars, &CLKRVHandleA );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open() [B]..............\n" ));
    ReturnError = STCLKRV_Open( CLKRVDevNamB, &CLKRVOpenPars, &CLKRVHandleB );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slaves frequencies*/
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleA, TestSlaveClockA[0],
                                                       TestSlaveFrequencyA[0]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleA, TestSlaveClockA[1],
                                                       TestSlaveFrequencyA[1]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleA, TestSlaveClockA[2],
                                                       TestSlaveFrequencyA[2]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleA, TestSlaveClockA[3],
                                                       TestSlaveFrequencyA[3]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleB, TestSlaveClockB[0],
                                                       TestSlaveFrequencyB[0]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleB, TestSlaveClockB[1],
                                                       TestSlaveFrequencyB[1]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleB, TestSlaveClockB[2],
                                                       TestSlaveFrequencyB[2]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandleB, TestSlaveClockB[3],
                                                       TestSlaveFrequencyB[3]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Disabling Clock Recovery [A]...\n"));
    ReturnError = STCLKRV_Disable(CLKRVHandleA);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Disabling Clock Recovery [B]...\n"));
    ReturnError = STCLKRV_Disable(CLKRVHandleB);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    /* Set Slot ID for STPTI communication */

    STTBX_Print(( "Calling STCLKRV_SetPCRSource() [A]...\n"));
    PCRSrcA.Source_u.STPTI_s.Slot = PcrSlotA;
    ReturnError = STCLKRV_SetPCRSource(CLKRVHandleA, &PCRSrcA);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    /* Set Slot ID for STPTI communication */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource() [B]...\n"));
    PCRSrcB.Source_u.STPTI_s.Slot = PcrSlotB;
    ReturnError = STCLKRV_SetPCRSource(CLKRVHandleB, &PCRSrcB);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print(("Clock recovery Behaviour test\n"));
    STTBX_Print(("Frequency Expected value will vary on different boards\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Report the Initial Frequency Value */
    STTBX_Print(( "CRU[0] Block Frequencies before enable\n"));
    ReturnError = STCLKRV_GetClocksFrequency( CLKRVHandleA, &STCFreqA, &FS1FreqA, &FS2FreqA,
                                              &FS3FreqA, &FS4FreqA, &FS5FreqA, &FS6FreqA );
    STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreqA, FS1FreqA, FS2FreqA));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "CRU[1] Block Frequencies before enable\n"));
    ReturnError = STCLKRV_GetClocksFrequency( CLKRVHandleB, &STCFreqB, &FS1FreqB, &FS2FreqB,
                                              &FS3FreqB, &FS4FreqB, &FS5FreqB, &FS6FreqB );
    STTBX_Print(( "FS0 = %u, FS1 = %u, FS2 = %u\n", STCFreqB, FS1FreqB, FS2FreqB));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Enable Clock Recovery [A]...\n\n"));
    ReturnError = STCLKRV_Enable(CLKRVHandleA);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Enable Clock Recovery [B]...\n\n"));
    ReturnError = STCLKRV_Enable(CLKRVHandleB);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("----------------------------------------------------------\n" ));
    STTBX_Print(("Observe Both  CRU[0] and CRU[1] after CLKRV enable....\n"));
    STTBX_Print(("(Frequency sampled every 10ms)\n"));

    STTBX_Print((" CRU[0]\tCRU[1]\n"));

    for (i = 0; i < 1000; i++)
    {
        STCLKRV_GetClocksFrequency( CLKRVHandleA, &STCFreqA, &FS1FreqA, &FS2FreqA,
                                    &FS3FreqA, &FS4FreqA, &FS5FreqA, &FS6FreqA );
        STCLKRV_GetClocksFrequency( CLKRVHandleB, &STCFreqB, &FS1FreqB, &FS2FreqB,
                                    &FS3FreqB, &FS4FreqB, &FS5FreqB, &FS6FreqB );
        STPTI_SlotPacketCount(PcrSlotA,&PacketCountA);
        STPTI_SlotPacketCount(PcrSlotB,&PacketCountB);
        STTBX_Print(("%9u, %9u, %9u, %9u, %9u",STCFreqA, FS1FreqA, FS2FreqA, FS3FreqA, FS4FreqA));
        STTBX_Print(("%9u, %9u, %9u, %9u, %9u\n",STCFreqB, FS1FreqB, FS2FreqB, FS3FreqB, FS4FreqB));

        STOS_TaskDelay(TICKS_PER_SECOND/25);
    }


#if 0
    for (i = 0; i < SampleCount; i++)
    {
        STTBX_Print(("CE1: %12d, %12d\n", ClkrvDbg[8][i],ClkrvDbg[4][i]));

}

 for (i = 0; i < SampleCount2; i++)
    {
        STTBX_Print(("CE2: %12d, %12d\n", ClkrvDbg2[8][i],ClkrvDbg2[4][i]));

}
#endif

    STTBX_Print(("\n-----------------------------------------------------------\n" ));
    STTBX_Print(("FS clocks behaviour test completed.\n" ));
    STTBX_Print(("-----------------------------------------------------------\n" ));


    STTBX_Print(("\n=============================================================\n" ));
    STTBX_Print(("TEST: CLKRV[1]- STCLKRV_GetSTC called from one thread..\n"));
    STTBX_Print(("      CLKRV[2]- STCLKRV_GetExtendedSTC called from second thread..\n"));
    STTBX_Print(("=============================================================\n\n" ));
    STTBX_Print(("....Please Wait....\n" ));


    /* Kick off get stc test task */
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#ifdef ST_OS20
    STOS_TaskCreate((void(*)(void *))GetSTCTestTaskFunc,
                                &CLKRVHandleA,
                                TEST_PARTITION_1,
                                TASK_STACK_SIZE,
                                &GetSTCTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetSTCTestTask_p,
                                &GetSTCTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetSTCTestTask",
                                (task_flags_t)0);

#else
    STOS_TaskCreate((void(*)(void *))GetSTCTestTaskFunc,
                                &CLKRVHandleA,
                                TEST_PARTITION_1,
                                ((16*1024)+(4*256)),
                                NULL,
                                TEST_PARTITION_1,
                                &GetSTCTestTask_p,
                                NULL,
                                MAX_USER_PRIORITY,
                                "GetSTCTestTask",
                                (task_flags_t)task_flags_no_min_stack_size);
#endif

    STTBX_Print(( "\nCLKRV[2] Get Extended STC Test Task now running ...\n"));

    for (i = 0; i < MAX_STC_COUNT; i++)
    {

        ReturnError = STCLKRV_GetExtendedSTC( CLKRVHandleB, &ExtendedSTC[i] );

        if (ReturnError != ST_NO_ERROR)
        {
            ExtendedSTC[i].BaseBit32 = 0;
            ExtendedSTC[i].BaseValue = 0;
            ExtendedSTC[i].Extension = 0;
        }
        STOS_TaskDelay(TICKS_PER_SECOND / 10);

    }

    /* Wait for STC task to complete before proceeding */
    STOS_TaskWait(&GetSTCTestTask_p, TIMEOUT_INFINITY);

    /* Cleaning up tasks */
#ifdef ST_OS20
    STOS_TaskDelete(GetSTCTestTask_p,TEST_PARTITION_1,
                        GetSTCTestTaskStack_p,TEST_PARTITION_1);
#else
    STOS_TaskDelete(GetSTCTestTask_p,TEST_PARTITION_1,
                                                  NULL,TEST_PARTITION_1);
#endif

    STTBX_Print(("\n=============================================================\n" ));
    STTBX_Print(("\tCLKRV[1]\t\tCLKRV[2]\n" ));
    STTBX_Print(("\tSTC\t\t\tExtended STC\n" ));
    STTBX_Print(("=============================================================\n" ));

    /* Out put the STCs */
    for (i = 0; i < MAX_STC_COUNT; i++)
    {
        STTBX_Print(("%12u, %12u, %12u, %12u\n", STC[i],
                                      ExtendedSTC[i].BaseBit32,
                                      ExtendedSTC[i].BaseValue,
                                      ExtendedSTC[i].Extension));
    }


    STTBX_Print(("\n=============================================================\n" ));
    STTBX_Print(("TEST: CLKRV[1]- STCLKRV_GetextendedSTC called from one thread..\n"));
    STTBX_Print(("      CLKRV[2]- STCLKRV_GetSTC called from second thread..\n"));
    STTBX_Print(("=============================================================\n\n" ));
    STTBX_Print(("....Please Wait....\n" ));

#ifdef ST_OS20
    /* Kick off get extended stc test task */
    STOS_TaskCreate((void(*)(void *))GetExtSTCTestTaskFunc,
                                &CLKRVHandleA,
                                TEST_PARTITION_1,
                                TASK_STACK_SIZE,
                                &GetExtSTCTestTaskStack_p,
                                TEST_PARTITION_1,
                                &GetExtSTCTestTask_p,
                                &GetSTCTestTaskDescriptor,
                                MAX_USER_PRIORITY,
                                "GetExtSTCTestTask",
                                (task_flags_t)0);
#else
    STOS_TaskCreate((void(*)(void *))GetExtSTCTestTaskFunc,
                                &CLKRVHandleA,
                                TEST_PARTITION_1,
                                ((16*1024)+(4*256)),
                                NULL,
                                TEST_PARTITION_1,
                                &GetExtSTCTestTask_p,
                                NULL,
                                MAX_USER_PRIORITY,
                                "GetExtSTCTestTask",
                                (task_flags_t)task_flags_no_min_stack_size);
#endif

    STTBX_Print(( "\nCLKRV[2] Get STC Test Task now running ...\n"));

    for (i=0; i < MAX_STC_COUNT; i++)
    {
        ReturnError = STCLKRV_GetSTC( CLKRVHandleB, &STC[i] );

        if (ReturnError != ST_NO_ERROR)
        {
            STC[i] = 0;
        }

        STOS_TaskDelay(TICKS_PER_SECOND / 100);
    }

    /* Wait for both task to finish */
    /* Wait for ExtSTC task to complete before proceeding */
    STOS_TaskWait(&GetExtSTCTestTask_p, TIMEOUT_INFINITY);

    /* Cleaning up tasks */
    /* ExtSTC has already finished */
#ifdef ST_OS20
    STOS_TaskDelete(GetExtSTCTestTask_p,TEST_PARTITION_1,
                        GetExtSTCTestTaskStack_p,TEST_PARTITION_1);
#else
    STOS_TaskDelete(GetExtSTCTestTask_p,TEST_PARTITION_1,
                                                  NULL,TEST_PARTITION_1);
#endif

    STTBX_Print(("\n=============================================================\n" ));
    STTBX_Print(("\tClKRV[2]\t\tCLKRV[1]\n" ));
    STTBX_Print(("\tSTC\t\t\tExtended STC\n" ));
    STTBX_Print(("=============================================================\n" ));

    /* Out put the STCs */
    for (i = 0; i < MAX_STC_COUNT; i++)
    {
        STTBX_Print(("%12u, %12u, %12u, %12u\n", STC[i],
                                      ExtendedSTC[i].BaseBit32,
                                      ExtendedSTC[i].BaseValue,
                                      ExtendedSTC[i].Extension));
    }


    STTBX_Print(("\n-----------------------------------------------------------\n" ));
    STTBX_Print(("GET STC \\ Extended STC test completed.\n" ));
    STTBX_Print(("-----------------------------------------------------------\n" ));
#endif /* #if defined(ST_OS20) || defined(ST_OS21) */

    /* Invalidate clock and receive PCR events... */
    STTBX_Print(( "Calling STCLKRV_InvDecodeClk() ...\n" ));
    ReturnError = STCLKRV_InvDecodeClk(CLKRVHandleA);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Disable display seperator */
    ErrorReportSeparator = TRUE;

    /* Close Clock Recovery explicitly */
    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( CLKRVHandleA );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( CLKRVHandleB );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    CLKRVTermPars.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( CLKRVDevNamA, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    CLKRVTermPars.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( CLKRVDevNamB, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() .for PTIA........\n" ));
    ReturnError = STEVT_Term( PTIEVTDevNamA, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() .for PTIB........\n" ));
    ReturnError = STEVT_Term( PTIEVTDevNamB, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STPTITermPars.ForceTermination = TRUE;
    STTBX_Print(("Calling STPTI_Term() forced.....\n"));
    ReturnError = STPTI_Term(STPTIDevNamA,&STPTITermPars);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Term STPTI */
    STPTITermPars.ForceTermination = TRUE;
    STTBX_Print(("Calling STPTI_Term() forced.....\n"));
    ReturnError = STPTI_Term(STPTIDevNamB,&STPTITermPars);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

}

#endif /* STPTI */

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
    switch( ErrorGiven )
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

        case STCLKRV_ERROR_INVALID_FREQUENCY:
            STTBX_Print(( "STCLKRV_ERROR_INVALID_FREQUENCY - Bad Slave Frequency\n" ));
            break;

        case STCLKRV_ERROR_INVALID_SLAVE:
            STTBX_Print(( "STCLKRV_ERROR_INVALID_SLAVE - Invalid Slave\n" ));
            break;

        default:
            STTBX_Print(( "*** Unrecognised return code : 0x%x (%d) ***\n",ErrorGiven, ErrorGiven ));
    }

    /* if ErrorGiven does not match ExpectedErr, update
       ErrorStore only if no previous error was noted    */
    if (ErrorGiven != ExpectedErr)
    {
        STTBX_Print(("*** TEST FAILED ***\n"));
        if( *ErrorStore == ST_NO_ERROR )
        {
            *ErrorStore = ErrorGiven;
            TestsFailed++;
        }
    }

    if (ErrorReportSeparator)
    {
        STTBX_Print(( "----------------------------------------------------------\n" ));
    }
}

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
/****************************************************************************
Name         : void GetSTCTestTaskFunc

Description  : Sets up a task to call GetSTC and store values in an array


Parameters   : *vData_p : Handle ptr

Return Value : None
****************************************************************************/
static void GetSTCTestTaskFunc(void *vData_p)
{
    U32 STCount;
    U32 LocalSTC;
    U32 ReturnError;
    STCLKRV_Handle_t *Handle_p;

    /* Initialize locals */
    ReturnError = ST_NO_ERROR;
    STCount     = 0;
    LocalSTC    = 0;

    if (vData_p != NULL)
    {
        Handle_p = (STCLKRV_Handle_t *) vData_p;
        STTBX_Print(( "\nCLKRV[1] Get STC Test Task now running ...\n"));
    }
    else
    {
        STTBX_Print(( "GetSTCTestTaskFunc FAILED!! ...\n"));
        return;
    }

    /* Setup interrupt for GetSTC call from Interrupt test */

    while ((STCount < MAX_STC_COUNT))
    {
        /* call STC */
        ReturnError = STCLKRV_GetSTC( (*Handle_p), &LocalSTC );
        if (ReturnError == ST_NO_ERROR)
        {
            STC[STCount] = LocalSTC;
        }
        else
        {
            STC[STCount] = 0;
        }
        STCount++;

        STOS_TaskDelay(TICKS_PER_SECOND / 100);
    }

}

/****************************************************************************
Name         : void GetExtSTCInterruptTestTaskFunc

Description  : Sets up a task to call GetExtended STC dumps into an array

Parameters   : *vData_p : Handle ptr

Return Value : None
****************************************************************************/
static void GetExtSTCTestTaskFunc(void *vData_p)
{
    U32 ReturnError;
    U32 STCount;
    STCLKRV_Handle_t *Handle_p;

    /* Initialize local variables */
    STCount = 0;

    if (vData_p != NULL)
    {
        Handle_p = (STCLKRV_Handle_t *) vData_p;
        STTBX_Print(( "\nCLKRV[1] Get Extended STC Test Task now running ...\n"));
    }
    else
    {
        STTBX_Print(( "GetSTCTestTaskFunc FAILED!! ...\n"));
        return;
    }

    /* Setup interrupt for GetSTC call from Interrupt test */

    while ((STCount < MAX_STC_COUNT))
    {
        /* call extended STC */
        ReturnError = STCLKRV_GetExtendedSTC( (*Handle_p), &ExtendedSTC[STCount] );

        if (ReturnError != ST_NO_ERROR)
        {
            ExtendedSTC[STCount].BaseBit32 = 0;
            ExtendedSTC[STCount].BaseValue = 0;
            ExtendedSTC[STCount].Extension = 0;
        }
        STCount++;

        STOS_TaskDelay(TICKS_PER_SECOND / 100);
    }
}
#endif

/****************************************************************************
Name         : STCLKRVT_EventCallback()

Description  : Callback function for STCLKRV events

Parameters   : STEVT_CallReason_t Reason
               ST_DeviceName_t RegistrantName : Device event instance ident
               STEVT_EventConstant_t Event : Event ident
               void *EventData
               void *SubscriberData_p

Return Value : None
 ****************************************************************************/
void STCLKRVT_EventCallback(STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
   /* Do Nothing */
}





/* End of clkrvdualpti.c module */

