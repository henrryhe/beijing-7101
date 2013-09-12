/****************************************************************************

File Name   : stclkrvmi.c

Description : Multi instance clock cecovery  using STAPI Standard Test Routines

Copyright (C) 2007, ST Microelectronics

References  :

$ClearCase (VOB: stclkrv)

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 4.3

NOTE : Based on the tests carried out by the stclkrvt.c and uses the same data,
but tests multiple instances of the clock recovery using STAPI.
This test file is ONLY compatible with multi instance STPTI compatible
clock recvovery software, and will test several (hardware dependant, but not
build controlled) of clock recovery. Number defined by NUM_REQUIRED_INSTANCE
****************************************************************************/


/* Includes --------------------------------------------------------------- */
#if defined (ST_5301)
#include <machine/bsp.h>
#endif
#include <stdlib.h>
#include <stdio.h>        /* Needed for print if STTBX fails to init */
#include <string.h>

#include "stboot.h"
#include "stclkrv.h"
#include "stos.h"
#include "stdevice.h"
#include "stevt.h"
#include "stlite.h"
#include "sttbx.h"
#if !defined(ST_5105) && !defined(ST_5107) && !defined(ST_5188) && !defined(ST_7200)
#include "stmerge.h"
#endif
#include "stsys.h"

U32 BASEADD_ARRAY1[1000];
U32 BASEADD_ARRAY2[1000];
U32 BASEADD_ARRAY3[1000];


/* Private Constants ------------------------------------------------------ */

#define NUM_REQUIRED_INSTANCES  2         /* Number of instances to test */
#define MAX_NUM_INSTANCES       3
#define MAX_EVT         MAX_NUM_INSTANCES
#define MAX_CLKRV       MAX_NUM_INSTANCES
/*....generic */
/* PTI */
                                    /* Similarity is the key */
#define TEST_PTI_BASE_ADDRESS       (U32*)PTI_BASE_ADDRESS
#define TEST_PTI_INTERRUPT          PTI_INTERRUPT


#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
/* TSMERGER */
#define TEST_TSMERGE_BASE_ADDRESS   TSMERGE_BASE_ADDRESS
#endif


#if defined (ST_5100)
#define STMERGE_PTI                 STMERGE_PTI_0
#define TEST_TSIN                   STMERGE_TSIN_0
#elif defined (ST_7710)
#define STMERGE_PTI                 STMERGE_PTI_0
#define TEST_TSIN                   STMERGE_TSIN_0
#endif

#ifndef PCR_PID              /* This can be overriden on the command line */
#define PCR_PID 60           /* canal10.trp  */
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

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            0x0200000
#define NCACHE_PARTITION_SIZE            0x100000

/* Memory partitions */
#define TEST_PARTITION_1          system_partition
#define TEST_PARTITION_2          ncache_partition

#define CONV90K_TO_27M            300           /* 90kHz to 27MHz conversion */
#define DELAY_PERIOD              ST_GetClocksPerSecond()


static const U8  HarnessRev[] = "5.4.0";

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

/* Exproted functions */
extern ST_ErrorCode_t STCLKRV_GetClocksFrequency( STCLKRV_Handle_t Handle,
                                                U32 *pSTCFrequency,
                                                U32 *pFS1Frequency,
                                                U32 *pFS2Frequency,
                                                U32 *pFS3Frequency,
                                                U32 *pFS4Frequency,
                                                U32 *pFS5Frequency,
                                                U32 *pFS6Frequency );
/* Private Types ---------------------------------------------------------- */

typedef void (*UpdatedPCR)(STEVT_CallReason_t EventReason,
                           STEVT_EventConstant_t EventOccured,
#ifdef ST_5188
                           STDEMUX_EventData_t *PcrData_p);
#else
                           STPTI_EventData_t *PcrData_p);
#endif
/* Private Variables ------------------------------------------------------ */

/* Declarations for memory partitions */
/* Declarations for memory partitions */
#ifndef ST_OS21
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
#if defined(ST_5100) || defined (ST_7710) || defined (ST_5105) || defined (ST_5107) || defined (ST_5188)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif


/* Partition pointers for PTI usage */
ST_Partition_t          *ncache_partition = &the_ncache_partition;
ST_Partition_t          *system_partition = &the_system_partition;
#else /* ifndef ST_OS21 */


static unsigned char    system_block[SYSTEM_PARTITION_SIZE];
partition_t             *system_partition;
static unsigned char    ncache_block[NCACHE_PARTITION_SIZE];
partition_t             *ncache_partition;

#endif /* ST_OS21 */

#ifndef ST_5188
#ifdef ST_7200
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_TSIN0;
#else
static STPTI_StreamID_t STPTI_StreamID = STPTI_STREAM_ID_NOTAGS;
#endif
#endif

#ifdef DVD_TRANSPORT_STPTI              /* stpti or stpti4 */
ST_DeviceName_t       STPTI_DeviceName = "STPTI";
#elif defined DVD_TRANSPORT_DEMUX
ST_DeviceName_t       STDEMUX_DeviceName = "STDEMUX";
STDEMUX_InitParams_t  STDEMUXInitParams;
#endif
/* Test variables */
static BOOL Passed = TRUE;
static BOOL NewEventRxd[MAX_CLKRV];
static STEVT_EventConstant_t EventReceived[MAX_CLKRV];


/*.....Globals for generic MI software */
static ST_DeviceName_t EVTDeviceName;
static ST_DeviceName_t EVTPCRDeviceName[MAX_EVT];
static ST_DeviceName_t CLKRVDeviceName[MAX_CLKRV];

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_5188) && !defined (ST_7200)
static ST_DeviceName_t         STMERGE_Name;
static STMERGE_InitParams_t    STMERGE_InitParams;
#endif
static STEVT_InitParams_t EVTInitPars;
static STCLKRV_InitParams_t CLKRVInitParams[MAX_CLKRV];
#ifndef ST_5188
static STPTI_InitParams_t  STPTIInitPars;
#endif
static STEVT_Handle_t EVTHandle;
static STEVT_Handle_t EVTPCRHandle[MAX_EVT];
static STCLKRV_Handle_t CLKRVHandle[MAX_CLKRV];
#ifdef ST_5188
static STDEMUX_Handle_t DEMUXSessionHandle;
#endif
STEVT_EventID_t    CLKRVEventID[MAX_CLKRV];




/* ...end of generic specific */

static STEVT_OpenParams_t      EVTOpenPars;
static STCLKRV_OpenParams_t    CLKRVOpenParams;
static STCLKRV_TermParams_t    CLKRVTermParams;
static STEVT_TermParams_t      EVTTermPars;
#ifndef ST_5188
static STPTI_TermParams_t      STPTITermParams;
#endif
static ST_ErrorCode_t          retVal;
/*#pragma ST_section             ( retVal,   "ncache_section")*/

static void STCLKRVT_EventCallback(STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData,
                                    const void *SubscriberData_p);


static U32      TestsFailed = 0;    /* Overall failure count */
static STEVT_DeviceSubscribeParams_t EVTSubscrParams = {
                                     STCLKRVT_EventCallback,  /* Shared callback */
                                     NULL };           /* Params */


/* Private Function prototypes -------------------------------------------- */


static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr
                         );
static ST_ErrorCode_t InitialiseDevices(void);

void TestSTCBaseLineMultiInstance(void);

static ST_ErrorCode_t TerminateDevices(void);

static void TitleDisplay(void);

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
#endif
/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : main()

Description  : Initialises the hardware and devices, spawns the tasks of
               instances to test, runs the tests on those instances and
               terminates.

Parameters   : void

Return Value : int

See Also     : None
 ****************************************************************************/

int main( void )
{
    STTBX_InitParams_t      sttbx_InitPars;
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    STBOOT_DCache_Area_t    Cache[] =
    {
#ifndef DISABLE_DCACHE
  #if defined (mb314) || defined(mb361)  || defined(mb382)
        { (U32 *) CACHEABLE_BASE_ADDRESS, (U32 *) CACHEABLE_STOP_ADDRESS },
  #else
        /* these probably aren't correct,
          but were used already and appear to work */
        { (U32 *) 0x40080000, (U32 *) 0x4FFFFFFF },
  #endif
#endif
        { NULL, NULL }
    };

    /* Initialize stboot */
#ifdef DISABLE_ICACHE
     stboot_InitPars.ICacheEnabled             = FALSE;
#else
     stboot_InitPars.ICacheEnabled             = TRUE;
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

#if defined(ST_5301)
    DCacheOff();
#endif

/* Avoid compiler warnings */
#if defined(ST_7710) || defined(ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5188)
        data_section[0]  = 0;
#endif
        /* Initialize the toolbox */
        sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

        sttbx_InitPars.CPUPartition_p = TEST_PARTITION_1;  /* SYSTEM PARTITION */

        retVal = STTBX_Init( "tbx", &sttbx_InitPars );
        if( retVal != ST_NO_ERROR )
        {
         printf( "ERROR: STTBX_Init() returned %d\n", retVal );
        }
        else  /* If all is well, continue with tests */
        {
            /* Flow Control through tests */
            TitleDisplay();
            InitialiseDevices();
            TestSTCBaseLineMultiInstance();
            TerminateDevices();

            retVal = STBOOT_Term( "CLKRVTest", &stboot_TermPars );
        }
     }

    if (TestsFailed == 0)
    {
        STTBX_Print(( "No Return Parameter Errors\n"));
    }
    else
    {
        STTBX_Print(( "**** ERRORS: Return parameter errors. %i counted\n", TestsFailed));
    }

    return retVal;
}


/**********************************************************************************
Name            :       InitialiseDevices

Description     :       Sets up all devices prior to clock recovery Open call

Parameters      :       None

Return          :       ST_ErrorCode_t
*********************************************************************************/
ST_ErrorCode_t InitialiseDevices()
{
    ST_ErrorCode_t            StoredError = ST_NO_ERROR;
    ST_ErrorCode_t            ReturnError;
    ST_DeviceName_t           DeviceName;
#ifdef ST_5188
    STDEMUX_OpenParams_t      STDEMUXOpenParams;
    STDEMUX_SlotData_t        SlotData;
    STDEMUX_Slot_t            PcrSlot;
    STDEMUX_StreamParams_t    STDEMUXStreamParams;
    U32                       PacketCount = 0;
#else
    STPTI_OpenParams_t        STPTIOpenParams;
    STPTI_Handle_t            PTISessionHandle;
    STPTI_SlotData_t          SlotData;
    STPTI_Slot_t              PcrSlot;
    U16                       PacketCount = 0;
#endif
#ifdef ST_7200
    STPTI_FrontendParams_t    PTIFrontendParams;
    STPTI_Frontend_t          PTIFrontendHandle;
#endif
    U16                       i=0;

    /* Device naming */
    strcpy((char *)EVTDeviceName, "STEVT");

    for (i = 0; i < MAX_EVT; i++)
    {
        sprintf((char *)DeviceName, "STEVT_Pcr_%d", i);
        strcpy((char *)EVTPCRDeviceName[i], (char *)DeviceName);
    }
    /* STEVT Init Params */

    EVTInitPars.EventMaxNum =  20;
    EVTInitPars.ConnectMaxNum = 20;
    EVTInitPars.SubscrMaxNum = 20; /* Modified from 3 for new EVT handler ??AK */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;

    /* Init devices */

    /* Two EVT handlers, one for PCR one for clkrv */
    STTBX_Print(( "STEVT_Init() for EVTDevice.....\n" ));
    ReturnError = STEVT_Init( EVTDeviceName, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STTBX_Print(( "STEVT_Open() for EVTHandler.....\n" ));
    ReturnError = STEVT_Open( EVTDeviceName, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Subscribe to PCR Events now so that we can get
       callbacks when events are registered */
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             CLKRVDeviceName[1],
                                             STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              CLKRVDeviceName[1],
                                              STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              CLKRVDeviceName[1],
                                              STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );


    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );



    /* Simulate STPTI operation of PCR event registration */

    STTBX_Print(( "STEVT_Init() for EVTPCRDevice.....\n" ));
    ReturnError = STEVT_Init( EVTPCRDeviceName[0], &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "STEVT_Open() for EVTPCRHandler.....\n" ));
    ReturnError = STEVT_Open( EVTPCRDeviceName[0], &EVTOpenPars, &EVTPCRHandle[0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(("Calling STEVT_RegisterDeviceEvent(PCR_RECIEVED_EVT)....\n"));
#ifdef ST_5188
    ReturnError = STEVT_RegisterDeviceEvent(EVTPCRHandle[0],
                                            CLKRVDeviceName[1],
                                            STDEMUX_EVENT_PCR_RECEIVED_EVT,
                                            &CLKRVEventID[0]);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#else
    ReturnError = STEVT_RegisterDeviceEvent(EVTPCRHandle[0],
                                            CLKRVDeviceName[1],
                                            STPTI_EVENT_PCR_RECEIVED_EVT,
                                            &CLKRVEventID[0]);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

#ifndef ST_5188
#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_7200)
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

    ReturnError = STMERGE_ConnectGeneric(STMERGE_TSIN_0,STMERGE_PTI_0,STMERGE_BYPASS_GENERIC);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#endif

    /* Inititalize init parameters according to chip type */
    STPTIInitPars.Device                     = STPTI_DEVICE_PTI_4;
#ifdef STPTI_DTV_SUPPORT
    STPTIInitPars.TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DTV;
    STPTIInitPars.TCLoader_p                 = STPTI_DTVTCLoader;
    STPTIInitPars.SyncLock                   = 0;
    STPTIInitPars.SyncDrop                   = 0;
    STPTIInitPars.DiscardSyncByte            = 0;
#else
    STPTIInitPars.TCCodes                    = STPTI_SUPPORTED_TCCODES_SUPPORTS_DVB;
#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#if defined(ST_5105) || defined (ST_5107)
    STPTIInitPars.TCLoader_p                 = STPTI_DVBTCLoaderUL;
#elif defined(ST_7109)
    STPTIInitPars.TCLoader_p                 = STPTI_DVBTCLoaderSL;
#elif defined(ST_7100) || defined(ST_5100)
    STPTIInitPars.TCLoader_p                 = STPTI_DVBTCLoaderL;
#elif defined(ST_7200)
    STPTIInitPars.TCLoader_p                 = STPTI_DVBTCLoaderSL2;
#else
    STPTIInitPars.TCLoader_p                 = STPTI_DVBTCLoader;
#endif
#endif
    STPTIInitPars.SyncLock                   = 0;
    STPTIInitPars.SyncDrop                   = 0;
#endif
    STPTIInitPars.TCDeviceAddress_p          = TEST_PTI_BASE_ADDRESS;
    STPTIInitPars.SectionFilterOperatingMode = STPTI_FILTER_OPERATING_MODE_8x8; /* ??? */
    STPTIInitPars.InterruptLevel             = PTI_INTERRUPT_LEVEL;
    STPTIInitPars.InterruptNumber            = TEST_PTI_INTERRUPT;

    STPTIInitPars.NCachePartition_p          = ncache_partition;
    STPTIInitPars.DescramblerAssociation     = STPTI_DESCRAMBLER_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitPars.Partition_p                = TEST_PARTITION_1;
    STPTIInitPars.EventProcessPriority       = MIN_USER_PRIORITY;
    STPTIInitPars.InterruptProcessPriority   = MAX_USER_PRIORITY;
#ifdef STPTI_INDEX_SUPPORT
    STPTIInitPars.IndexAssociation           = STPTI_INDEX_ASSOCIATION_ASSOCIATE_WITH_SLOTS;
    STPTIInitPars.IndexProcessPriority       = MIN_USER_PRIORITY;
#endif

#ifdef STPTI_CAROUSEL_SUPPORT
    STPTIInitPars.CarouselProcessPriority    = MIN_USER_PRIORITY;
    STPTIInitPars.CarouselEntryType          = STPTI_CAROUSEL_ENTRY_TYPE_SIMPLE;
#endif

    STPTIInitPars.StreamID                   = STPTI_StreamID;
#if defined (ST_7200)
    STPTIInitPars.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_TSMERGER;
#else
    STPTIInitPars.PacketArrivalTimeSource    = STPTI_ARRIVAL_TIME_SOURCE_PTI;
#endif
    STPTIInitPars.NumberOfSlots              = 10;
    STPTIInitPars.AlternateOutputLatency     = 42;

    memcpy (&(STPTIInitPars.EventHandlerName), EVTPCRDeviceName[0], sizeof (ST_DeviceName_t));

    STTBX_Print(( "Calling STPTI_Init() ................\n" ));
    ReturnError = STPTI_Init(STPTI_DeviceName, &STPTIInitPars);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    if (ReturnError == ST_NO_ERROR)
    {
        /* open session */
        STPTIOpenParams.DriverPartition_p = TEST_PARTITION_1;
        STPTIOpenParams.NonCachedPartition_p =  ncache_partition;
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
    /* Get some packets */
    STOS_TaskDelay(ST_GetClocksPerSecond());
    STPTI_GetInputPacketCount(STPTI_DeviceName,&PacketCount);
    STTBX_Print(("Input Packet Count :  %i\n", PacketCount));
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
    }
#else
    FEI_Setup();
    FdmaSetup();

    /* Set up the parameters */
    STDEMUXInitParams.Device = STDEMUX_DEVICE_CHANNEL_FEC1;
    STDEMUXInitParams.Parser = STDEMUX_PARSER_TYPE_TS;
    STDEMUXInitParams.Partition_p = TEST_PARTITION_1;
    STDEMUXInitParams.NCachePartition_p = TEST_PARTITION_2;

    strcpy(STDEMUXInitParams.EventHandlerName, EVTPCRDeviceName[0]);

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
    STOS_TaskDelay(ST_GetClocksPerSecond());

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
        STTBX_Print(("**** STDEMUX TEST!! can not be performed.  \n"));
        STTBX_Print(("**********************************************************\n"));
    }
#endif
    return ReturnError;

}

/**********************************************************************************
Name            :       TestSTCBaseLineMultiInstance

Description     :     Task to test the instance using the data given.

Parameters      :       None

Return          :       None
*********************************************************************************/
void TestSTCBaseLineMultiInstance(void)
{
    ST_ErrorCode_t              ReturnError;
    ST_ErrorCode_t              StoredError = ST_NO_ERROR;
    ST_DeviceName_t             DeviceName;
    U8                          i;

    U32 STCFreq, FS1Freq, FS2Freq, STC;            /* returned FS Frequency setting */
    U32 FS3Freq, FS4Freq, FS5Freq, FS6Freq;
    STCLKRV_ExtendedSTC_t ExtendedSTC;
    STCLKRV_ExtendedSTC_t Baseline;

    for (i = 0; i < 3; i++)
    {
        sprintf((char *)DeviceName, "STCLKRV_%d", i);
        strcpy((char *)CLKRVDeviceName[i], (char *)DeviceName);
    }
    STTBX_Print(( "\n===============================================================\n" ));
    STTBX_Print(( "Commencing TestSTCBaseLineMultiInstance Test Function....\n" ));
    STTBX_Print(( "===============================================================\n\n" ));

    CLKRVInitParams[0].DeviceType        = STCLKRV_DEVICE_TYPE_BASELINE_ONLY;
    CLKRVInitParams[0].Partition_p       = TEST_PARTITION_1;
#ifdef ST_5188
    strcpy( CLKRVInitParams[0].PTIDeviceName, STDEMUX_DeviceName );
#else
    strcpy( CLKRVInitParams[0].PTIDeviceName, STPTI_DeviceName );
#endif
    STTBX_Print(( "Calling STCLKRV_Init in baseline mode.\n" ));
    ReturnError = STCLKRV_Init( CLKRVDeviceName[0], &CLKRVInitParams[0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    CLKRVInitParams[1].DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams[1].PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitParams[1].PCRDriftThres       = TEST_PCR_DRIFT_THRES;
    CLKRVInitParams[1].MinSampleThres      = STCLKRV_MIN_SAMPLE_THRESHOLD;
    CLKRVInitParams[1].MaxWindowSize       = STCLKRV_MAX_WINDOW_SIZE;
    CLKRVInitParams[1].Partition_p         = TEST_PARTITION_1;
    CLKRVInitParams[1].DeviceType          = CLK_REC_DEVICE_TYPE;
    CLKRVInitParams[1].FSBaseAddress_p     = (U32 *)CKG_BASE_ADDRESS;
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    CLKRVInitParams[1].AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitParams[1].ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    CLKRVInitParams[1].InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitParams[1].InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;
#if defined (ST_7200)
    CLKRVInitParams[1].DecodeType          = STCLKRV_DECODE_PRIMARY;
    CLKRVInitParams[1].CRUBaseAddress_p    = (U32*)(CLKRV_BASE_ADDRESS);
#endif

    strcpy( CLKRVInitParams[1].PCREvtHandlerName, EVTPCRDeviceName[0] );
    strcpy( CLKRVInitParams[1].EVTDeviceName, EVTDeviceName );
#ifdef ST_5188
    strcpy( CLKRVInitParams[1].PTIDeviceName, STDEMUX_DeviceName );
#else
    strcpy( CLKRVInitParams[1].PTIDeviceName, STPTI_DeviceName );
#endif
    /* STCLKRV Init Params */
    STTBX_Print(( "Calling STCLKRV_Init for hardware initialization.\n" ));
    ReturnError = STCLKRV_Init( CLKRVDeviceName[1], &CLKRVInitParams[1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    CLKRVInitParams[2].DeviceType        = STCLKRV_DEVICE_TYPE_BASELINE_ONLY;
    CLKRVInitParams[2].Partition_p       = TEST_PARTITION_1;
#ifdef ST_5188
    strcpy( CLKRVInitParams[2].PTIDeviceName, STDEMUX_DeviceName );
#else
    strcpy( CLKRVInitParams[2].PTIDeviceName, STPTI_DeviceName );
#endif

    STTBX_Print(( "Calling STCLKRV_Init in baseline mode.\n" ));
    ReturnError = STCLKRV_Init( CLKRVDeviceName[2], &CLKRVInitParams[2] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);


    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Inst 0: STCLKRV_Open().\n" ));
    ReturnError = STCLKRV_Open( CLKRVDeviceName[0], &CLKRVOpenParams, &CLKRVHandle[0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Inst 1: STCLKRV_Open().\n" ));
    ReturnError = STCLKRV_Open( CLKRVDeviceName[1], &CLKRVOpenParams, &CLKRVHandle[1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Inst 2: STCLKRV_Open().\n" ));
    ReturnError = STCLKRV_Open( CLKRVDeviceName[2], &CLKRVOpenParams, &CLKRVHandle[2] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set Slaves frequencies*/

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandle[0], TestSlaveClock[0],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );

#ifndef ST_5188
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandle[0], TestSlaveClock[1],
                                                 148500000);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );
#endif

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandle[1], TestSlaveClock[0],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#ifndef ST_5188
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandle[1], TestSlaveClock[1],
                                                 148500000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandle[2], TestSlaveClock[0],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );

#ifndef ST_5188
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(CLKRVHandle[2], TestSlaveClock[1],
                                                 148500000);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );
#endif

    STTBX_Print(("\n===========================================================\n"));
    STTBX_Print((" STC Baseline ON, Clock Recovery OFF\n"));
    STTBX_Print(("===========================================================\n\n"));

    /* Disable clock recovery */
    STTBX_Print(( "Calling STCLKRV_Disable()\n" ));
    ReturnError = STCLKRV_Disable(CLKRVHandle[0]);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED);

    STTBX_Print(( "Calling STCLKRV_Disable()\n" ));
    ReturnError = STCLKRV_Disable(CLKRVHandle[1]);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    STTBX_Print(( "Calling STCLKRV_Disable()\n" ));
    ReturnError = STCLKRV_Disable(CLKRVHandle[2]);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED);

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
    ReturnError = STCLKRV_SetSTCBaseline(CLKRVHandle[0], &Baseline);
    ErrorReport( &StoredError,ReturnError, ST_NO_ERROR);

    /* Enable baslining */
    STTBX_Print(( "\nCalling STCLKRV_SetSTCSource()\n" ));
    STTBX_Print(("...set source to STC baseline\n"));
    ReturnError = STCLKRV_SetSTCSource(CLKRVHandle[0], STCLKRV_STC_SOURCE_BASELINE);
    ErrorReport(&StoredError,ReturnError, ST_NO_ERROR);

    STTBX_Print(( "Calling STCLKRV_SetSTCBaseline again to enable()\n" ));
    ReturnError = STCLKRV_SetSTCBaseline(CLKRVHandle[0], &Baseline);
    ErrorReport(&StoredError,ReturnError, ST_NO_ERROR);

    /* Watch STC and clock values */
    for (i = 0; i != 15; i++)
    {
        STTBX_Print(("\n....Value check %d......\n",i));
        STTBX_Print(( "\nCalling STCLKRV_GetExtendedSTC()\n" ));
        ReturnError = STCLKRV_GetExtendedSTC( CLKRVHandle[0], &ExtendedSTC );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STCBaseBit32: %12u\n"
                      "STCBaseValue: %12u\n"
                      "STCExtension: %12u\n",
                      ExtendedSTC.BaseBit32,
                      ExtendedSTC.BaseValue,
                      ExtendedSTC.Extension ));

        STTBX_Print(("\nCalling STCLKRV_GetSTC()\n"));
        ReturnError = STCLKRV_GetSTC(CLKRVHandle[0], &STC);
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STTBX_Print(( "STC: %12u\n",STC));

        STOS_TaskDelay( DELAY_PERIOD/10 );
    }

    STTBX_Print(("\n-----------------------------------------------------------\n"));
    STTBX_Print((" Value Checks complete.\n"));
    STTBX_Print(("-----------------------------------------------------------\n"));

    /* Report the Initial Frequency Value */
    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n"));
    ReturnError = STCLKRV_GetClocksFrequency( CLKRVHandle[0], &STCFreq, &FS1Freq, &FS2Freq, &FS3Freq,
                                                                        &FS4Freq, &FS5Freq, &FS6Freq );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );

    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n"));
    ReturnError = STCLKRV_GetClocksFrequency( CLKRVHandle[1], &STCFreq, &FS1Freq, &FS2Freq, &FS3Freq,
                                                                        &FS4Freq, &FS5Freq, &FS6Freq );
    STTBX_Print(( "FS Block Frequencies\n"));
    STTBX_Print(( "Inst %i: FS0 = %d, FS1 = %d, FS2 = %d\n", i, STCFreq, FS1Freq, FS2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_GetClocksFrequency()....\n"));
    ReturnError = STCLKRV_GetClocksFrequency( CLKRVHandle[2], &STCFreq, &FS1Freq, &FS2Freq, &FS3Freq,
                                                                        &FS4Freq, &FS5Freq, &FS6Freq );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );

    /* Close the Clock Recovery Driver */
    STTBX_Print(( "Inst 0: STCLKRV_Close().\n" ));
    ReturnError = STCLKRV_Close( CLKRVHandle[0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Inst 1: STCLKRV_Close().\n" ));
    ReturnError = STCLKRV_Close( CLKRVHandle[1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Inst 2: STCLKRV_Close().\n" ));
    ReturnError = STCLKRV_Close( CLKRVHandle[2] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */
    CLKRVTermParams.ForceTerminate = FALSE;

    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( CLKRVDeviceName[0], &CLKRVTermParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( CLKRVDeviceName[1], &CLKRVTermParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( CLKRVDeviceName[2], &CLKRVTermParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

}
/**********************************************************************************
Name            :       TerminateDevices

Description     :       Sets up all devices prior to clock recovery Open call

Parameters      :       None

Return          :       ST_ErrorCode_t
*********************************************************************************/
ST_ErrorCode_t TerminateDevices()
{
    ST_ErrorCode_t              ReturnError;
    ST_ErrorCode_t              StoredError = ST_NO_ERROR;

#if !defined (ST_5105) && !defined (ST_5107) && !defined (ST_5188) && !defined (ST_7200)
    /*Term STMERGE */
    STTBX_Print(( "Calling STMERGE_Term ................\n" ));
    ReturnError = STMERGE_Term(STMERGE_Name, NULL);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    /* Terminate the subordinate EVT Driver */
    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDeviceName, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#ifndef ST_5188
    STPTITermParams.ForceTermination = TRUE;
    STTBX_Print(("Calling STPTI_Term() forced.....\n"));
    ReturnError = STPTI_Term(STPTI_DeviceName,&STPTITermParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    EVTTermPars.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() for PCR........\n" ));
    ReturnError = STEVT_Term( EVTPCRDeviceName[0], &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    return ReturnError;
}
/****************************************************************************
Name         : ErrorReport()

Description  : Outputs the appropriate error message string.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : STLLI_ErrorCode_t
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
Name         : FindDeviceNameIndex()

Description  : Test utility: Searches the DeviceName table for the index
               value of the clock recovery device. Returns > MAX if not found

Parameters   : DeivceName to search for

Return Value : U32 index value into array, or > MAX_CLKRV (must be error
               checked on return!!!)
 ****************************************************************************/
U32 FindDeviceNameIndex(const ST_DeviceName_t RegistrantName)
{
    U32 i = 0;


    while (i <= MAX_CLKRV)
    {
        if (strcmp(CLKRVDeviceName[i], RegistrantName) == 0)
        {
            return i;
        }
        else
        {
            i++;
        }
    }
     return MAX_CLKRV + 1;
}

/****************************************************************************
Name         : STCLKRVT_EventCallback()

Description  : Callback function for STCLKRV events

Parameters   : STEVT_CallReason_t Reason      Reason for callback
               const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant Event      Event ID
               const void *EventData          Event associated data
               const void *SubscriberData_p

               Return Value : None
 ****************************************************************************/
void STCLKRVT_EventCallback(STEVT_CallReason_t Reason,
                            const ST_DeviceName_t RegistrantName,
                            STEVT_EventConstant_t Event,
                            const void *EventData,
                            const void *SubscriberData_p)
{
    U32 i;

    /* Slot value offset by 1 due to NULL */
    i =  FindDeviceNameIndex(RegistrantName);

    if (i <= MAX_CLKRV)
    {

        switch ( Event )
        {
        case STCLKRV_PCR_VALID_EVT:
            STTBX_Print( ("Inst %i: STCLKRV_PCR_VALID_EVT event received\n",i) );
            break;

        case STCLKRV_PCR_DISCONTINUITY_EVT:
            STTBX_Print( ("Inst %i: STCLKRV_PCR_DISCONTINUITY_EVT event received\n",i) );
            break;

        case STCLKRV_PCR_GLITCH_EVT:
            STTBX_Print( ("Inst %i: STCLKRV_PCR_GLITCH_EVT event received\n",i) );
            break;

        default:
            STTBX_Print( ("Inst %i: ERROR: Unknown STCLKRV event received\n",i) );
            break;
        }

        NewEventRxd[i] = TRUE;
        EventReceived[i] = Event;
    }
    else
    {
        STTBX_Print(("*** Inst %i out of range for EventCallback\n",i));
    }
}


/***************************************************************************
Name            :       TitleDisplay

Description     :       Prints title to serial output

Params:         :       None
**************************************************************************/
void TitleDisplay(void)
{
    ST_Revision_t   RevisionStr;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));
    STTBX_Print(( "----------------------------------------------------------\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "\n============================================================\n" ));
    STTBX_Print(( " Commencing NormalUse Test Function For Multiple Instance \n"));
    STTBX_Print(( "============================================================\n\n" ));


}

/* end of stclkrvmi.c */
