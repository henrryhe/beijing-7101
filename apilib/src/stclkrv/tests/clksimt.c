/****************************************************************************

File Name   : clksimt.c

Description : Clock Recovery API Standard Test Routines

Copyright (C) 2006, ST Microelectronics

References  :

$ClearCase (VOB: stclkrv)

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 5.0

****************************************************************************/


/* Includes ----------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>        /* Needed for print if STTBX fails to init */
#include <string.h>

#include "stboot.h"
#include "stos.h"
#include "stdevice.h"
#include "stevt.h"
#include "stlite.h"
#include "sttbx.h"
#include "testtool.h"
#include "stclkrv.h"
#include "../clkrvsim.h"

/* Private Types ------------------------------------------------------ */

typedef enum INIT_Errors_s
{
    NO_ERROR,
    STTST_START_FAILED,
    STCLKRVSIM_INIT_FAILED,
    STEVT_INIT_FAILED,
    STTST_REGISTER_FAILED,
    STTST_INIT_FAILED,
    STTBX_INIT_FAILED,
    STBOOT_FAILED

}INIT_Errors_t;


/* Private Constants -------------------------------------------------- */

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

#define MASTER_CLK_NAME  "SD/STC"
#define SLAVE1_CLK_NAME "PCM"

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) || defined (ST_5188)
#define SLAVE2_CLK_NAME "SPDIF"
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
#define SLAVE2_CLK_NAME "HD"
#endif


/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE     (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE       800000

/* Memory partitions */
#ifdef ST_OS21
#define TEST_PARTITION_1            system_partition
#define TEST_PARTITION_2            system_partition
#else
#define TEST_PARTITION_1            system_partition
#define TEST_PARTITION_2            internal_partition
#endif
/* Lock tests specific */

#define MIN_SLAVE_INDEX 0
#define MAX_SLAVE_INDEX 35
#define DEFAULT_SLAVE1  1
#define DEFAULT_SLAVE2  2

#define SLAVE_FREQ_CHANGE_TIME      20     /* Seconds */


/* Lock time tests */
#define DEFAULT_FREQ_ERROR          1000   /* In Hz */
#define MAX_FREQ_ERROR              6000
#define MIN_FREQ_ERROR             -6000


#define DEFAULT_LOCK_CYCLES         1      /* no. of times with multiple of start error */
#define MAX_LOCK_CYCLES             50
#define MIN_LOCK_CYCLES             1


#define DEFAULT_LOCK_TIME          60      /* secs */
#define MAX_LOCK_TIME              2000    /* 10 minutes */
#define MIN_LOCK_TIME              2       /* Minimum lock time */


#define DEFAULT_CYCLES              1
#define MAX_CYCLES                  6
#define MIN_CYCLES                  1

#define DEFAULT_CYCLE_TIME          100    /* secs */
#define MAX_CYCLE_TIME              5000
#define MIN_CYCLE_TIME              15

/* Jitter commom for all tests */
#define DEFAULT_JITTER              0      /* Ticks */
#define MAX_JITTER                  10000
#define MIN_JITTER                  0

/* Real time tests */
#define DEFAULT_REAL_EVENTS         10     /* 10 events, = 10 seconds for real time test */
#define MIN_REAL_EVENTS             5      /* Random events for real time test */
#define MAX_REAL_EVENTS             600    /* One event per second = 10 minute */

/* Sync test */
#define DEFAULT_SYNCH_STEP          1000
#define MIN_SYNCH_STEP              0
#define MAX_SYNCH_STEP              2000

#define DEFAULT_SYNC_CYCLES         2      /* no. of times with multiple of start error */
#define MAX_SYNC_CYCLES             10
#define MIN_SYNC_CYCLES             1

#define DEFAULT_SYNC_TIME           15     /* secs */
#define MAX_SYNC_TIME               120    /* 2 minutes */
#define MIN_SYNC_TIME               2      /* Minimum lock time */


/* Replay from file test */
#define MIN_READ_TIME               1      /* seconds */
#define MAX_READ_TIME               600

#define MAX_FILENAME_LENGTH         50

/* Set Params */
#define MIN_PCR_INTERVAL_TIME       10     /* Standard specifies 100 ms */
#define MAX_PCR_INTERVAL_TIME       200
#define DEFAULT_PCR_INTERVAL_TIME   40     /* Easy for calculations */

#define MIN_WINDOW_SIZE             3
#define MAX_WINDOW_SIZE             200
#define DEFAULT_MAX_WINDOW_SIZE     30     /* Defaults for satellite */
#define DEFAULT_MIN_WINDOW_SIZE     3


#define MIN_DRIFT_THRESHOLD         200
#define MAX_DRIFT_THRESHOLD         10000
#define DEFAULT_DRIFT_THRESHOLD     200
#define DEFAULT_SEED                1

#define MIN_SLAVE_CHANGE            1
#define MAX_SLAVE_CHANGE            50
#define DEFAULT_SLAVE_CHANGE        20




#define FILE_READ_TEST_TIME         100    /* 10 minutes */

#define STCLKRV_5100_INTERRUPT      PIO_2_INTERRUPT
#define STCLKRV_5100_INTLEVEL       1


/* Various Device Names */
#define EVT_DEVNAME                 "STEVT"
#define EVT_DEVNAME_PCR             "STEVT2"
#define CLKRV_DEVNAME               "STCLKRV"
#define DEFAULT_FILE                "test.txt"

/* Private Variables -------------------------------------------------- */

/* Declarations for memory partitions */
#ifndef ST_OS21
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      (internal_block, "internal_section")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#pragma ST_section      ( system_block,   "system_section")

/* Following code ensures that system_section_noinit and internal_section_noinit
   (which are placed in mb314 config file) are actually created.
   Removes linker warnings.
*/
static U8               system_noinit_block [1];
static partition_t      the_system_noinit_partition;
partition_t             *system_noinit_partition = &the_system_noinit_partition;
#pragma ST_section      ( system_noinit_block, "system_section_noinit")

static U8               internal_noinit_block [1];
static partition_t      the_internal_noinit_partition;
partition_t             *noinit_partition = &the_internal_noinit_partition;
#pragma ST_section      (internal_noinit_block, "internal_section_noinit")

#if defined(ST_5100) || defined (ST_7710) || defined (ST_5105) || defined (ST_5107) || defined(ST_5188)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#else/* ifndef ST_OS21 */

static unsigned char    system_block[SYSTEM_PARTITION_SIZE];
partition_t             *system_partition;

#endif /* ST_OS21 */

static ST_ErrorCode_t   RetVal;
#ifndef ST_OS21
#pragma ST_section      (RetVal,   "ncache_section")        /* why in non cache section */
#endif
/* Reg Array Base Address*/
CLKRVBaseAddress*       SimBaseAddr_p;                      /* To Be Passed to Driver */


/* MPEG limitation on PCR time is 100 ms */
S32 PCRIntervalTime = DEFAULT_PCR_INTERVAL_TIME;

/* clock recovery init params */
S32 Min_Window_Size = DEFAULT_MIN_WINDOW_SIZE;
S32 Max_Window_Size = DEFAULT_MAX_WINDOW_SIZE;
S32 Drift_Threshold = STCLKRV_PCR_DRIFT_THRES;
#ifdef SLAVE_FREQ_CHANGE_TEST
S32 SlaveFreqChangeTime = 1;
#endif
/* CLKRV Related */
static STCLKRV_InitParams_t InitParams;
static STCLKRV_OpenParams_t OpenParams;
static STCLKRV_TermParams_t TermParams;
static ST_DeviceName_t      DeviceNam;

/* EVT Related */
ST_DeviceName_t      EVTDevNam;
STEVT_Handle_t       EVTHandle;

#ifdef SLAVE_FREQ_CHANGE_TEST
static void ChangeSlave1Freq(void * NtUSd);
static void ChangeSlave2Freq(void * NtUSd);

static task_t *ChangeSlave1FreqTask_p;
static tdesc_t ChangeSlave1FreqTaskDescriptor;
static void* ChangeSlave1FreqTaskStack_p;

static task_t *ChangeSlave2FreqTask_p;
static tdesc_t ChangeSlave2FreqTaskDescriptor;
static void *ChangeSlave2FreqTaskStack_p;

U32 T1Term = 1;
U32 T2Term = 1;

STCLKRV_Handle_t         GlbClkrvHndl;
#endif

/* List of slave frequencies */

U32 SlaveFreqArray[] ={ 2048000,
                        2822400,
                        3072000,

                        4096000,
                        4233600,
                        4608000,
                        5644800,
                        6144000, /* 8 */

                        8192000,
                        8200000,
                        8467200,
                        9216000,
                        11289600,
                        11300000,
                        12288000,
                        12300000, /* 16 */

                        16384000,
                        16934400,
                        18432000,
                        22579200,
                        24576000,
                        24600000, /* 22 */

                        32768000,
                        33868800,
                        36864000,
                        45158400,
                        49152000, /* 28 */

                        67737600,
                        73728000,
                        74250000,

                        107892107,
                        108000000,

                        148351648,
                        148500000}; /* 36 -1 == 35 */





/* globals identifying Slaves on various boards */

/* 5100     7710
   PCM      PCM
   SPDIF    HD */


#if defined (ST_5100)  || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)
static STCLKRV_ClockSource_t TestSlaveClock[2] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_SPDIF_0};
U32 TestSlaveFrequency[2] = { 11300000, 45158400};
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
static STCLKRV_ClockSource_t TestSlaveClock[2] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_HD_0};
U32 TestSlaveFrequency[2] = { 11300000, 148500000};
#elif defined (ST_5188)
static STCLKRV_ClockSource_t TestSlaveClock[2] = {STCLKRV_CLOCK_PCM_0};
U32 TestSlaveFrequency[2] = { 12288000};
#endif


static const U8  HarnessRev[] = "5.3.1";

/* Private Macros ----------------------------------------------------- */

#define DELAY_PERIOD   ST_GetClocksPerSecond()

/* Private Function prototypes ---------------------------------------- */
/* EVT Related */
static ST_ErrorCode_t EVT_Init(void);
static void EVT_Term(void);

/* Tests Fired from Test Tool */
static BOOL HelpFilterTest( parse_t *pars_p, char *result_sym_p );
static BOOL SetParams( parse_t *pars_p, char *result_sym_p );

static BOOL GetLockTimeTestParams( parse_t *pars_p, char *result_sym_p );
static BOOL GetSynchTestParams( parse_t *pars_p, char *result_sym_p );
static BOOL GetRealTimeTestParams( parse_t *pars_p, char *result_sym_p );
static BOOL GetReplayFromFileTestParams( parse_t *pars_p, char *result_sym_p );


static ST_ErrorCode_t RunLockTimeTest(S32 FreqError,S32 LockTimeCycles,S32 LockTime,S32 Jitter,
                                    U32 FreqSlave1, U32 FreqSlave2);
static ST_ErrorCode_t RunSynchTest(S32 SynchStep, S32 SynchCycles, S32 SynchTime, S32 Jitter,
                                   S32 FreqSlave1, S32 FreqSlave2);
static ST_ErrorCode_t RunRealTimeTest(S32 FreqError, S32 Number_Events,
                                       S32 Jitter, S32 Seed,
                                       S32 FreqSlave1, S32 FreqSlave2);
static ST_ErrorCode_t RunReplayFromFileTest(char* FileName_p, S32 TestTime);

/* Utility functions */
static BOOL IsOutsideRange(S32 ValuePassed,S32 LowerBound,S32 UpperBound);

static void STCLKRVT_EventCallback( STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData,
                                    const void *SubscriberData_p);

static STEVT_DeviceSubscribeParams_t EVTSubscrParams = {
                                                         STCLKRVT_EventCallback,
                                                         NULL };

static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr );

static void Exit_Clean(INIT_Errors_t Error);

/* Functions ---------------------------------------------------------- */

/****************************************************************************
Name         : main()

Description  : Initializes Dependencies and Calls the specific test functions

Parameters   : void

Return Value : int

See Also     : None
 ****************************************************************************/

int main( void )
{
    STTBX_InitParams_t      sttbx_InitPars;
    STBOOT_InitParams_t     stboot_InitPars;
    STTST_InitParams_t      sttst_InitPar;
    STCLKRVSIM_InitParams_t sim_InitPars;
    ST_ErrorCode_t          StoredError = ST_NO_ERROR;
    BOOL RetErr             = FALSE;

    STBOOT_DCache_Area_t    Cache[] =
    {
#ifndef DISABLE_DCACHE
        #if defined (mb314) || defined(mb361) || defined(mb382)
            {(U32 *) CACHEABLE_BASE_ADDRESS, (U32 *) CACHEABLE_STOP_ADDRESS},
        #else
         /* these probably aren't correct, but were used already and appear to work */
            {(U32 *) 0x40080000, (U32 *) 0x4FFFFFFF},
        #endif
#endif
        {NULL, NULL}
    };

/* Avoid compiler warnings */
#if defined(ST_7710) || defined(ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5188)
    data_section[0]  = 0;
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

    RetVal = STBOOT_Init( "CLKRVSIM", &stboot_InitPars );
    if( RetVal != ST_NO_ERROR )
    {
        printf("ERROR: STBOOT_Init() returned %d\n", RetVal );
        Exit_Clean(STBOOT_FAILED);
        return RetVal;
    }



    /* Initialize All Partitions */
#ifdef ST_OS21
    /* Create memory partitions */
    system_partition   =  partition_create_heap((U8*)system_block, sizeof(system_block));
#else
    partition_init_heap (&the_internal_partition,
                            internal_block,sizeof(internal_block));
    partition_init_heap (&the_system_partition,
                            system_block,sizeof(system_block));
    partition_init_heap (&the_system_noinit_partition,
                             system_noinit_block, sizeof(system_noinit_block));
    partition_init_heap (&the_internal_noinit_partition,
                             internal_noinit_block, sizeof(internal_noinit_block));
#endif

    /* Initialize the ToolBox */
    sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

    sttbx_InitPars.CPUPartition_p      = TEST_PARTITION_1;

    RetVal = STTBX_Init("TBX", &sttbx_InitPars );
    if( RetVal != ST_NO_ERROR )
    {
        printf( "ERROR: STTBX_Init() returned %d\n", RetVal );
        Exit_Clean(STTBX_INIT_FAILED);
        return RetVal;
    }

    STTBX_Print(( "\nHarness Software Revision is %s\n", HarnessRev ));
    STTBX_Print(( "\n============================================================\n" ));
    STTBX_Print(( "CLOCK RECOVERY FILTER Tests\n" ));
    STTBX_Print(( " - LOCK TIME Test\n" ));
    STTBX_Print(( " - SYNC Test\n" ));
    STTBX_Print(( " - REAL TIME Test\n" ));
    STTBX_Print(( " - REPLAY FILE(PCR,STC) Test\n" ));
    STTBX_Print(( "============================================================\n\n" ));

    /* Test Tool Init */
    sttst_InitPar.CPUPartition_p = TEST_PARTITION_1 ;
    sttst_InitPar.NbMaxOfSymbols = 1000;
    memset(sttst_InitPar.InputFileName,'\0',sizeof(char)*(max_tok_len));

    STTBX_Print(( "Calling STTST_Init()......\n" ));
    RetErr = STTST_Init(&sttst_InitPar);
    ErrorReport( &StoredError, RetErr, ST_NO_ERROR );

    RetErr |= STTST_RegisterCommand("TESTHELP",HelpFilterTest,"Help");
    RetErr |= STTST_RegisterCommand("SETPARAMS",SetParams,"Set Parameters");
    RetErr |= STTST_RegisterCommand("LOCK",GetLockTimeTestParams,"LOCK Time Test");
    RetErr |= STTST_RegisterCommand("SYNC",GetSynchTestParams,"Synchronization Test");
    RetErr |= STTST_RegisterCommand("REALTIME",GetRealTimeTestParams,"Random SeqTest");
    RetErr |= STTST_RegisterCommand("REPLAYFILE",GetReplayFromFileTestParams,"File Read Test");


    if(RetErr == TRUE)
    {
        STTBX_Print(("ERROR: STTST_RegisterCommand()\n"));
        Exit_Clean(STTST_REGISTER_FAILED);
        return RetErr;
    }

    /* Now Initialize EVT     */
    RetVal = EVT_Init();
    if(RetVal != ST_NO_ERROR )
    {
        STTBX_Print(( "ERROR: EVTInit()\n"));
        Exit_Clean(STEVT_INIT_FAILED);
        return RetVal;
    }

    /* Init Simulator */
    strcpy(sim_InitPars.EVTDevNamPcr, EVT_DEVNAME_PCR );

    sim_InitPars.SysPartition_p      = TEST_PARTITION_1; /* or TEST_PARTITION_2 */
    sim_InitPars.InternalPartition_p = TEST_PARTITION_2; /* or TEST_PARTITION_1 */
    sim_InitPars.InterruptNumber     = STCLKRV_5100_INTERRUPT ;
    STTBX_Print(( "\nCalling STCLKRVSIM_Init()....\n" ));
    RetVal = STCLKRVSIM_Init(&sim_InitPars, &SimBaseAddr_p);
    ErrorReport( &StoredError, RetVal, ST_NO_ERROR );
    if((RetVal != ST_NO_ERROR) ||(SimBaseAddr_p == NULL))
    {
        STTBX_Print(( "STCLKRVSIM_Init Failed\n" ));
        Exit_Clean(STCLKRVSIM_INIT_FAILED);
        return RetVal;
    }

    STTBX_Print(( "\nFor HELP!! Run TESTHELP on prompt\n\n" ));

    /* launch the command interpretor now */
    STTBX_Print(( "Calling STTST_Start()....\n" ));
    RetErr = STTST_Start();
    if(RetErr == TRUE)
    {
        STTBX_Print(( "Test Tool Start Failed\n" ));
        Exit_Clean(STTST_START_FAILED);
        return RetErr;
    }

    /* Terminate All drivers before exit*/
    Exit_Clean(NO_ERROR);

    return RetVal;
}
/****************************************************************************
Name         : HelpFilterTest()

Description  : Used to display help for test cases

Parameters   : Pointers to parse_t structure type and char type

Return Value : TRUE/FALSE

See Also     : None
 ****************************************************************************/
static BOOL HelpFilterTest( parse_t *pars_p, char *result_sym_p )
{
    STTBX_Print(( "TEST MACROS HELP:\n" ));
    STTBX_Print(( "TESTHELP\n" ));
    STTBX_Print(( "SETPARAM pcrtime minwindow maxwindow pcrdrift\n" ));
    STTBX_Print(( "LOCK error lockcycles locktime jitter SlaveIndx1 SlaveIndx2 -[FreqChangeTime]\n" ));
    STTBX_Print(( "SYNC syncstep synCycles synctime jitter SlaveIndx1 SlaveIndx2\n" ));
    STTBX_Print(( "REAL error numberofEvents jitter seed SlaveIndx1 SlaveIndx2\n" ));
    STTBX_Print(( "REPLAY maximumRunTime\n" ));
    return TRUE;
}


/****************************************************************************
Name         : Exit_Clean()

Description  : Used to exit cleanly at different points of Init

Parameters   : INIT_Errors_t

Return Value : void

See Also     : None
 ****************************************************************************/

static void Exit_Clean(INIT_Errors_t Error)
{
    STTBX_TermParams_t      sttbx_TermParams;
    STBOOT_TermParams_t     stboot_TermPars;

    switch(Error)
    {
        case NO_ERROR:
        case STTST_START_FAILED:
             STCLKRVSIM_Term();
             /* Fall through */

        case STCLKRVSIM_INIT_FAILED:
             EVT_Term();
             /* Fall through */

        case STTST_REGISTER_FAILED:
        case STEVT_INIT_FAILED:
             STTST_Term();
             /* Fall through */

        case STTST_INIT_FAILED:
             STTBX_Term("TBX",&sttbx_TermParams);
             /* Fall through */

        case STTBX_INIT_FAILED:
             STBOOT_Term("CLKRVSIMTest", &stboot_TermPars );
             /* Fall through */

        case STBOOT_FAILED:
             /* Do Nothing */
             break;

        default:
             STTBX_Print(("Exit_Clean Called with Error type default\n"));
    }
}


/****************************************************************************
Name         : EVT_Init

Description  : Initializes EVT and subscribes to events

Parameters   : void

Return Value : ST_ErrorCode_t

See Also     :
 ****************************************************************************/

static ST_ErrorCode_t EVT_Init(void)
{
    STEVT_InitParams_t   EVTInitPars_s;
    STEVT_OpenParams_t   EVTOpenPars_s;
    ST_ErrorCode_t       EvtError = ST_NO_ERROR;
    ST_ErrorCode_t       StoredError = ST_NO_ERROR;

    /* Initialize the sub-ordinate EVT Driver */

    EVTInitPars_s.EventMaxNum       = 3;
    EVTInitPars_s.ConnectMaxNum     = 2;
    EVTInitPars_s.SubscrMaxNum      = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars_s.MemoryPartition   = TEST_PARTITION_1;
    EVTInitPars_s.MemorySizeFlag    = STEVT_UNKNOWN_SIZE;

    STTBX_Print(( "Calling STEVT_Init()....\n" ));

    strcpy( EVTDevNam,EVT_DEVNAME);
    STTBX_Print(( "Calling STEVT_Init()....\n" ));
    EvtError = STEVT_Init(EVTDevNam, &EVTInitPars_s );
    ErrorReport( &StoredError, EvtError, ST_NO_ERROR );

    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(( "ERROR:STEVT_Init() ....\n" ));
        return EvtError;
    }
    /* Call STEVT_Open() so that we can subscribe to events */

    STTBX_Print(( "Calling STEVT_Open()....\n" ));
    EvtError = STEVT_Open(EVTDevNam, &EVTOpenPars_s,&EVTHandle);
    ErrorReport( &StoredError, EvtError, ST_NO_ERROR );

    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(( "ERROR:STEVT_Open() ....\n" ));
        return EvtError;
    }
    /* Subscribe to PCR Events now so that we can get
       callbacks when events are registered */
    strcpy( DeviceNam, CLKRV_DEVNAME );

    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    EvtError |= STEVT_SubscribeDeviceEvent(EVTHandle,
                                          DeviceNam,
                                          (U32)STCLKRV_PCR_VALID_EVT,
                                          &EVTSubscrParams
                                          );
    ErrorReport( &StoredError, EvtError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    EvtError |= STEVT_SubscribeDeviceEvent(EVTHandle,
                                          DeviceNam,
                                          (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                          &EVTSubscrParams
                                          );
    ErrorReport( &StoredError, EvtError, ST_NO_ERROR );
    STTBX_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    EvtError |= STEVT_SubscribeDeviceEvent(EVTHandle,
                                          DeviceNam,
                                          (U32)STCLKRV_PCR_GLITCH_EVT,
                                          &EVTSubscrParams
                                          );
    ErrorReport( &StoredError, EvtError, ST_NO_ERROR );
    if(EvtError != ST_NO_ERROR)
    {
        STTBX_Print(( "ERROR: STEVT setup....\n" ));
        return EvtError;
    }

    return EvtError;
 }

/****************************************************************************
Name         : Term_EVT

Description  : Unsubscribes to Events and Terminates EVT

Parameters   : void

Return Value : void

See Also     :
****************************************************************************/

static void EVT_Term(void)
{

    STEVT_TermParams_t  EVTTermPars_s;
    ST_ErrorCode_t       EvtError = ST_NO_ERROR;

    EvtError = STEVT_UnsubscribeDeviceEvent(EVTHandle,
                                            DeviceNam,
                                            (U32)STCLKRV_PCR_VALID_EVT);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Call to STEVT_Unsubsrcibe() failed"));
    }

    EvtError = STEVT_UnsubscribeDeviceEvent(EVTHandle,
                                            DeviceNam,
                                            (U32)STCLKRV_PCR_DISCONTINUITY_EVT);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Call to STEVT_Unsubsrcibe() failed"));
    }

    EvtError = STEVT_UnsubscribeDeviceEvent(EVTHandle,
                                            DeviceNam,
                                            (U32)STCLKRV_PCR_GLITCH_EVT);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Call to STEVT_Unsubsrcibe() failed"));
    }

    EvtError = STEVT_Close(EVTHandle);
    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Call to STEVT_Close() failed"));
    }

    EVTTermPars_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STEVT_Term()....\n" ));
    EvtError = STEVT_Term(EVTDevNam, &EVTTermPars_s);

    if (EvtError != ST_NO_ERROR)
    {
        STTBX_Print(("Call to STEVT_Term() failed"));
    }

}


/****************************************************************************
Name         : SetParams

Description  : Takes Parameters from Test Tool which act as environment variables
               for Simulator.
Parameters   : Pointers to parse_t structure type and char type

Return Value : TRUE or FALSE

See Also     :
 ****************************************************************************/

static BOOL SetParams( parse_t *pars_p, char *result_sym_p )
{
    BOOL                RetErr = FALSE;

    /* 1st argument : get PCR Interval Time */
    RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_PCR_INTERVAL_TIME, &PCRIntervalTime);
    if (RetErr == FALSE)
    {
        if(IsOutsideRange(PCRIntervalTime,(S32) MIN_PCR_INTERVAL_TIME, MAX_PCR_INTERVAL_TIME) == TRUE)
        {
            RetErr = TRUE;
            STTST_TagCurrentLine(pars_p,"Expected INPUT: PCR IntervalTime not in range");
        }
    }

    /* 2nd argument : get Window Size */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_MIN_WINDOW_SIZE,&Min_Window_Size);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(Min_Window_Size,MIN_WINDOW_SIZE,MAX_WINDOW_SIZE) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Min Sample thres not in range");
            }
        }
    }

    /* 2nd argument : get Window Size */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_MAX_WINDOW_SIZE,&Max_Window_Size);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(Max_Window_Size,MIN_WINDOW_SIZE,MAX_WINDOW_SIZE) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: WINDOW SIZE not in range");
            }
        }
    }

    /* 3rd argument : Drift Threshold */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_DRIFT_THRESHOLD,&Drift_Threshold);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(Drift_Threshold,MIN_DRIFT_THRESHOLD, MAX_DRIFT_THRESHOLD) == TRUE)
            {
                 RetErr = TRUE;
                 STTST_TagCurrentLine( pars_p, "Expected: Drift Threshold not in range");
            }
        }
    }

    if ( RetErr == FALSE ) /* i.e if parameters are ok */
    {
        STTBX_Print(( "Clock recovery parameters are\n"));
        STTBX_Print(( "MinSampleThres    = %d\n",Min_Window_Size));
        STTBX_Print(( "MaxWindowSize     = %d\n",Max_Window_Size));
        STTBX_Print(( "PCRDriftThres     = %d\n",Drift_Threshold));
        STTBX_Print(( "PCR Interval time = %d\n",PCRIntervalTime));
    }
    else
    {
         STTBX_Print(("Invalid Parameters:\n"));
    }
    return (RetErr);
}

/****************************************************************************
Name         : GetLockTimeTestParams()

Description  : Takes Parameters from Test Tool which act as input values
               for Lock Test.
Parameters   : Pointers to parse_t structure type and char type

Return Value : TRUE or FALSE

See Also     :
 ****************************************************************************/
static BOOL GetLockTimeTestParams( parse_t *pars_p, char *result_sym_p )
{
    S32                 Jitter    = 0;
    S32                 FreqError = 0;
    S32                 LockTime  = 0;
    S32                 LockTimeCycles = 0;
    BOOL                RetErr = FALSE;
    S32                 Slave1Index = 0 ,Slave2Index = 0;


    /* 1st argument : get Frequency Error */
    RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_FREQ_ERROR, &FreqError);
    if (RetErr == FALSE)
    {
        if(IsOutsideRange(FreqError,MIN_FREQ_ERROR,MAX_FREQ_ERROR) == TRUE)
        {
            RetErr = TRUE;
            STTST_TagCurrentLine(pars_p,"Expected INPUT: Frequency Error in Range");
        }
    }

    /* 2nd argument : get Lock Time Cycles */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_LOCK_CYCLES, &LockTimeCycles);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(LockTimeCycles, MIN_LOCK_CYCLES, MAX_LOCK_CYCLES) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: LOCK_TIME_CYCLES in range");
            }
        }
    }

    /* 3rd argument : get Lock Time */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_LOCK_TIME, &LockTime);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(LockTime,MIN_LOCK_TIME,MAX_LOCK_TIME)== TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: LOCK TIME in Range");
            }
        }
    }

    /* 4th argument : get Max Jitter in ticks */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_JITTER, &Jitter);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Jitter, MIN_JITTER, MAX_JITTER) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Jitter in range");
            }
        }
    }

    /* 5th argument : get slave1 Freq  index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE1, &Slave1Index);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Slave1Index, MIN_SLAVE_INDEX, MAX_SLAVE_INDEX) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }

    /* 6th argument : get  slave1 Freq Index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE2, &Slave2Index);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Slave2Index, MIN_SLAVE_INDEX, MAX_SLAVE_INDEX) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }

#ifdef SLAVE_FREQ_CHANGE_TEST
    /* 6th argument : get  slave1 Freq Index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE_CHANGE, &SlaveFreqChangeTime);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(SlaveFreqChangeTime, MIN_SLAVE_CHANGE, MAX_SLAVE_CHANGE) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }
#endif


    if ( RetErr == FALSE ) /* i.e if parameters are ok */
    {
        STTBX_Print(( "\nCalling LOCK TIME TEST with values:\n"
                        "Frequency Error = %d Hz\nLock Time Cycles = %d\n"
                        "LockTime = %d sec.\nJitter = %d\nFS1 Freq = %u\nFS2 Freq = %u",
                        FreqError,LockTimeCycles,LockTime,Jitter,
                        SlaveFreqArray[Slave1Index], SlaveFreqArray[Slave2Index]));

        /* Invoke Lock Time Test now */
        RunLockTimeTest(FreqError, LockTimeCycles, LockTime, Jitter,
                        SlaveFreqArray[Slave1Index], SlaveFreqArray[Slave2Index]);
    }
    else
    {
        STTBX_Print(("Invalid test parameters\n"));
    }

    return (RetErr);
}


/****************************************************************************
Name         : RunLockTimeTest()

Description  : The input params from test Tool are passed and the Lock Test Run

Parameters   : FreqError      - Induced Error in Encoder Frequency
               LockTimeCycles - Number of Times to Run the Test
               LockTime       - Time for which test should run
               Jitter         - Max Jitter value
               Slave freq     - Fs1
               Slave freq     - Fs2

Return Value : ST_ErrorCode_t

See Also     : STCLKRV_ErrorCode_t
 ****************************************************************************/

static ST_ErrorCode_t RunLockTimeTest(S32 FreqError,S32 LockTimeCycles,S32 LockTime,S32 Jitter,
                                      U32 FreqSlave1, U32 FreqSlave2)
{
    S32                      i = 0;
    STCLKRV_SourceParams_t   PCRSrc;
    STCLKRV_Handle_t         Handle;
    ST_Revision_t            RevisionStr;
    FILE                     *file_p = NULL;
    STCLKRVSIM_StartParams_t SimStart_params;
    U8                       StreamStatus = 0;
    S32                      TotalFreqError = 0;
    U32                      EncoderFrequency = 0;
    ST_ErrorCode_t           StoredError = ST_NO_ERROR;
    ST_ErrorCode_t           ReturnError = ST_NO_ERROR;
    char                     filename[MAX_FILENAME_LENGTH];

    STTBX_Print(( "\n============================================================\n" ));
    STTBX_Print(( "Commencing Lock Time Test Function....\n" ));
    STTBX_Print(( "============================================================\n\n" ));

    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    for(i=0; i < LockTimeCycles; i++)
    {
        memset(filename, '\0', MAX_FILENAME_LENGTH);

#ifdef SLAVE_FREQ_CHANGE_TEST
        TotalFreqError = FreqError;
#else
        TotalFreqError = FreqError*(i+1);
#endif

        sprintf(filename,"LOCK_%d.csv",TotalFreqError);

        file_p = fopen(filename,"w+");
        if(file_p !=NULL)
        {
            /* Clock recovery instance devicename */
            strcpy(DeviceNam, CLKRV_DEVNAME);

            InitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
            InitParams.FSBaseAddress_p     = SimBaseAddr_p;
#if defined (ST_7200)
            InitParams.DecodeType          = STCLKRV_DECODE_PRIMARY;
            InitParams.CRUBaseAddress_p    = (U32*)(CLKRV_BASE_ADDRESS);
#endif
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
            InitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
            InitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
            InitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
            InitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;
            InitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
            InitParams.PCRDriftThres       = Drift_Threshold;
            InitParams.MinSampleThres      = Min_Window_Size;
            InitParams.MaxWindowSize       = Max_Window_Size;
            InitParams.Partition_p         = TEST_PARTITION_1;

            strcpy(InitParams.PCREvtHandlerName, EVT_DEVNAME_PCR );
            strcpy(InitParams.EVTDeviceName, EVT_DEVNAME );
            strcpy(InitParams.PTIDeviceName, "STPTI" );

            STTBX_Print(( "Calling STCLKRV_Init()....\n" ));
            ReturnError = STCLKRV_Init( DeviceNam, &InitParams);
            ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

             /* Open the Clock Recovery Driver */
            STTBX_Print(( "Calling STCLKRV_Open()....\n" ));
            ReturnError = STCLKRV_Open(DeviceNam, &OpenParams, &Handle );
            ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

#ifdef SLAVE_FREQ_CHANGE_TEST
            if (ReturnError == ST_NO_ERROR)
                GlbClkrvHndl = Handle;
#endif

            /* Set SlotID value to instance. Required for callback identification
               of relevant instance to action the callback */
            STTBX_Print(( "Calling STCLKRV_SetPCRSource() to allocate slot ID....\n"));
            PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
            ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
            ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

            /* Set Nominal Freq for Slave Clocks */
            STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS1/PCM %u....\n",FreqSlave1));
            ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0], FreqSlave1);
            ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#ifndef ST_5188
            STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS2/SPDIF %u....\n",FreqSlave2));
            ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1], FreqSlave2);
            ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

            /* start with defined error*/
            EncoderFrequency = DEFAULT_SD_FREQUENCY + TotalFreqError;

            STTBX_Print(( "\nEncoder Frequency = %u Hz\n", EncoderFrequency ));
            STTBX_Print(( "Jitter = %u Ticks\n", Jitter));

            STTBX_Print(( "Calling STCLKRVSIM_SetTestParameters()....\n" ));
            ReturnError = STCLKRVSIM_SetTestParameters(EncoderFrequency, Jitter, StreamStatus);
            ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

            fprintf(file_p,"Test Parameters\n");
            fprintf(file_p,"PCRIntervalTime,%d\nMax_Window_Size,%d\nDrift_Threshold,%d\n",
                                                    PCRIntervalTime,Max_Window_Size,Drift_Threshold);
            fprintf(file_p,"Encoder Frequency,%d\nTotalFreqError,%d\nJitter,%d\n"
                "LockTimeCycles,%d\nLock Time,%d",EncoderFrequency,TotalFreqError,Jitter,
                                                                    LockTimeCycles,LockTime);

            fprintf(file_p,"\nCode,Count,STC-Freq.,PCR,STC,Tick error,Freq Error,Avg. Error(Hz),SUM,"
            "SD-AEPS,SD-OSE,Time,FS1(PCM)Hz,FS2(SPDIF/HD)\n");

            SimStart_params.File_p               = (file_p);
            SimStart_params.PCR_Interval         = PCRIntervalTime;
            SimStart_params.PCRDriftThreshold    = Drift_Threshold;
            SimStart_params.FileRead             = 0;

            /*  Start generating PCR,STC values in Simulator */
            STTBX_Print(( "Calling STCLKRVSIM_Start()....\n" ));
            ReturnError = STCLKRVSIM_Start(&SimStart_params);
            ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
            if(ReturnError!= ST_NO_ERROR)
            {
                STTBX_Print(("Failed Simulator LockTestStart\n"));
            }
            else
            {
#ifdef SLAVE_FREQ_CHANGE_TEST

                T1Term = 1;
                T2Term = 1;
                /* Kick off Two task for changing slave frequencies randomly */
                STOS_TaskCreate((void(*)(void *))ChangeSlave1Freq,
                                    NULL,
                                    TEST_PARTITION_1,
                                    4*256,
                                    &ChangeSlave1FreqTaskStack_p,
                                    TEST_PARTITION_1,
                                    &ChangeSlave1FreqTask_p,
                                    &ChangeSlave1FreqTaskDescriptor,
                                    MAX_USER_PRIORITY,
                                    "ChangeSlave1FreqTask",
                                    (task_flags_t)0 );

                STOS_TaskCreate((void(*)(void *))ChangeSlave2Freq,
                                    NULL,
                                    TEST_PARTITION_1,
                                    4*256,
                                    &ChangeSlave2FreqTaskStack_p,
                                    TEST_PARTITION_1,
                                    &ChangeSlave2FreqTask_p,
                                    &ChangeSlave2FreqTaskDescriptor,
                                    MAX_USER_PRIORITY,
                                    "ChangeSlave2FreqTask",
                                    (task_flags_t)0 );

#endif

                STOS_TaskDelay(DELAY_PERIOD * LockTime);     /* Delay for lock time now */
                STCLKRVSIM_Stop();                       /* Stop generating PCR,STC values */
            }

#ifdef SLAVE_FREQ_CHANGE_TEST
            T1Term = 0;
            T2Term = 0;

            /* Wait for ExtSTC task to complete before proceeding */
            STOS_TaskWait ( &ChangeSlave1FreqTask_p, TIMEOUT_INFINITY );
            /* ExtSTC has already finished */
            STOS_TaskDelete (ChangeSlave1FreqTask_p,TEST_PARTITION_1,
                             ChangeSlave1FreqTaskStack_p,TEST_PARTITION_1);

            /* Wait for ExtSTC task to complete before proceeding */
            STOS_TaskWait ( &ChangeSlave2FreqTask_p, TIMEOUT_INFINITY );
                /* ExtSTC has already finished */
            STOS_TaskDelete (ChangeSlave2FreqTask_p,TEST_PARTITION_1,
                             ChangeSlave2FreqTaskStack_p,TEST_PARTITION_1);
#endif

            STTBX_Print(( "Calling STCLKRV_Close()....\n" ));
            ReturnError = STCLKRV_Close(Handle);
            ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

            STTBX_Print(( "Calling STCLKRV_Term()....\n" ));
            ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
            ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
            fclose(file_p);
        }
        else
        {
            STTBX_Print(("***ERROR:Could Not Open File %s****\n",filename));
        }
    }

    STTBX_Print(( "\n----------------------------------------------------------\n" ));
    STTBX_Print(("Completed LockTime Test.\n"));
    STTBX_Print(( "----------------------------------------------------------\n\n" ));
    return ST_NO_ERROR;
}


/****************************************************************************
Name         : GetSynchTestParams()

Description  : Takes Parameters from Test Tool which act as input values
               for Synch Test.
Parameters   : Pointers to parse_t structure type and char type

Return Value : TRUE or FALSE

See Also     :
 ****************************************************************************/

static BOOL GetSynchTestParams( parse_t *pars_p, char *result_sym_p )
{
    S32                 SynchStep   = 0;
    S32                 SynchTime   = 0;
    S32                 SynchCycles = 0;
    S32                 Jitter = 0;
    S32                 Slave1Index=0, Slave2Index=0;
    BOOL                RetErr = FALSE;

    /* 1st argument : Sync Step */
    RetErr = STTST_GetInteger(pars_p,(S32)DEFAULT_SYNCH_STEP, &SynchStep);
    if (RetErr == FALSE)
    {
        if(IsOutsideRange(SynchStep, MIN_SYNCH_STEP, MAX_SYNCH_STEP) == TRUE)
        {
            RetErr = TRUE;
            STTST_TagCurrentLine(pars_p,"Expected INPUT: Synch. Step");
        }
    }

    /* 2nd argument : Sync Cycles */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p,(S32)DEFAULT_SYNC_CYCLES, &SynchCycles);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(SynchCycles, MIN_SYNC_CYCLES, MAX_SYNC_CYCLES) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Synch Cycles in Range");
            }
        }
    }

    /* 3rd argument : Sync Time */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_SYNC_TIME, &SynchTime);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(SynchTime,MIN_SYNC_TIME,MAX_SYNC_TIME) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Synch_Time in range");
            }
        }
    }
    /* 4th argument : get Jitter */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p,(S32)DEFAULT_JITTER, &Jitter);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(Jitter, MIN_JITTER, MAX_JITTER) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Jitter in range");
            }
        }
    }


    /* 5th argument : get slave1 Freq  index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE1, &Slave1Index);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Slave1Index, MIN_SLAVE_INDEX, MAX_SLAVE_INDEX) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }

    /* 6th argument : get  slave1 Freq Index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE2, &Slave2Index);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Slave2Index, MIN_SLAVE_INDEX, MAX_SLAVE_INDEX) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }

    if ( RetErr == FALSE ) /* i.e if parameters are ok */
    {
        STTBX_Print(( "Calling filter SYNC TEST with values:\n"
                      "Synch Step   = %d\nSynch Time   = %d\n"
                      "Synch Cycles = %d\nJITTER       = %d\n"
                      "FS1 = %d\nFS2 = %d",
                      SynchStep, SynchTime, SynchCycles, Jitter,
                      SlaveFreqArray[Slave1Index], SlaveFreqArray[Slave2Index]));

        /* Invoke Synch Test now */
        RunSynchTest(SynchStep, SynchCycles, SynchTime, Jitter,
                        SlaveFreqArray[Slave1Index], SlaveFreqArray[Slave2Index]);
    }
    else
    {
        STTBX_Print(("Invalid params: Can Not Run Synch Test\n"));
    }

    return ( RetErr );
}


/****************************************************************************
Name         : RunSynchTest()

Description  : The input params from Test Tool are passed and the Synch Test Run

Parameters   : SynchStep      - Induced Error in Encoder Frequency
               SynchCycles    - Number of Times to Run the Test
               SynchTime      - Time for which test should run
               Jitter         - Max Jitter value
               Slave freq     - Fs1
               Slave freq     - Fs2

Return Value : ST_ErrorCode_t

See Also     : STCLKRV_ErrorCode_t
 ****************************************************************************/

static ST_ErrorCode_t RunSynchTest(S32 SynchStep, S32 SynchCycles, S32 SynchTime, S32 Jitter,
                                   S32 FreqSlave1, S32 FreqSlave2)
{
    S32                      i,Toggle = 0;
    ST_ErrorCode_t           StoredError = ST_NO_ERROR;
    ST_ErrorCode_t           ReturnError;
    ST_Revision_t            RevisionStr;
    STCLKRV_SourceParams_t   PCRSrc;
    STCLKRVSIM_StartParams_t SimStart_params;
    U32                      EncoderFrequency = 0;
    U32                      InitialEncoderFreq = 0;
    STCLKRV_Handle_t         Handle;
    FILE                     *file_p = NULL;
    char                     filename[MAX_FILENAME_LENGTH];

    STTBX_Print(( "\n============================================================\n" ));
    STTBX_Print(( "Commencing SYNC TIME TEST Function....\n" ));
    STTBX_Print(( "============================================================\n\n" ));


    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STTBX_Print(( "----------------------------------------------------------\n" ));

    /* Clock recovery instance devicename */
    strcpy(DeviceNam, CLKRV_DEVNAME);

    InitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    InitParams.FSBaseAddress_p     = SimBaseAddr_p;
#if defined (ST_7200)
    InitParams.DecodeType          = STCLKRV_DECODE_PRIMARY;
    InitParams.CRUBaseAddress_p    = (U32*)(CLKRV_BASE_ADDRESS);
#endif
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    InitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    InitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    InitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    InitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;
    InitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    InitParams.PCRDriftThres       = Drift_Threshold;
    InitParams.MinSampleThres      = Min_Window_Size;
    InitParams.MaxWindowSize       = Max_Window_Size;
    InitParams.Partition_p         = TEST_PARTITION_1;

    strcpy(InitParams.PCREvtHandlerName, EVT_DEVNAME_PCR);
    strcpy(InitParams.EVTDeviceName, EVT_DEVNAME );
    strcpy(InitParams.PTIDeviceName, "STPTI" );
    STTBX_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &InitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open()....\n" ));
    ReturnError = STCLKRV_Open(DeviceNam, &OpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource() to allocate slot ID....\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    /* Set Nominal Freq for Slave Clocks */
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS1/PCM %u....\n",FreqSlave1));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0], FreqSlave1);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#ifndef ST_5188
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS2/SPDIF %u....\n",FreqSlave2));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1], FreqSlave2);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    memset(filename,'\0',MAX_FILENAME_LENGTH);
    sprintf(filename,"SYNCH_%d.csv",SynchStep);
    file_p = fopen(filename,"w+");
    if(file_p!=NULL)
    {
        InitialEncoderFreq = DEFAULT_SD_FREQUENCY + SynchStep;

        STTBX_Print(( "Calling STCLKRVSIM_SetTestParameters()....\n" ));
        STCLKRVSIM_SetTestParameters(InitialEncoderFreq, Jitter, 0);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        fprintf(file_p,"\nTest Parameters\n");
        fprintf(file_p,"InitialEncoderFreq,%d\nPCRIntervalTime,%d\nMax_Window_Size,%d\n"
         "Drift_Threshold,%d\n",InitialEncoderFreq,PCRIntervalTime,Max_Window_Size,Drift_Threshold);

        fprintf(file_p,"Initial EncoderFreq,%d\nSynchStep,%d\nJitter,%d\n"
                "SynchCycles,%d\nSynchTime,%d",EncoderFrequency,SynchStep,Jitter,
                                                                SynchCycles,SynchTime);

        fprintf(file_p,"\nCode,Count,STC-Freq.,PCR,STC,Tick error,Freq Error,Avg. Error(Hz),SUM,"
            "SD-AEPS,SD-OSE,Time,FS1(PCM)Hz,FS2(SPDIF/HD)\n");

        SimStart_params.File_p               = (file_p);
        SimStart_params.PCR_Interval         = PCRIntervalTime;
        SimStart_params.PCRDriftThreshold    = Drift_Threshold;
        SimStart_params.FileRead             = 0;

        EncoderFrequency = InitialEncoderFreq;

        STTBX_Print(( "\nEncoder Frequency = %u Hz\n", EncoderFrequency ));
        STTBX_Print(( "Jitter = %u Ticks\n", Jitter));

        /*  Start generating PCR,STC values in Simulator */
        STTBX_Print(( "Calling STCLKRVSIM_Start()....\n" ));
        ReturnError = STCLKRVSIM_Start(&SimStart_params);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        if(ReturnError!= ST_NO_ERROR)
        {
            STTBX_Print(("Failed Simulator Synch TestStart\n"));
        }
        else
        {
            STOS_TaskDelay(DELAY_PERIOD * (SynchTime));

            for(i=0; i < SynchCycles; i++)
            {
                if(!Toggle)
                {
                    EncoderFrequency = EncoderFrequency - (2 * SynchStep);
                    Toggle = 1;
                }
                else
                {
                    EncoderFrequency = EncoderFrequency + (2 * SynchStep);
                    Toggle = 0;
                }

                STTBX_Print(( "\nEncoder Frequency = %u Hz\n", EncoderFrequency ));
                STTBX_Print(( "Jitter = %u Ticks\n", Jitter));

                STTBX_Print(( "Calling STCLKRVSIM_SetTestParameters()....\n" ));
                ReturnError = STCLKRVSIM_SetTestParameters(EncoderFrequency,Jitter, MASK_BIT_BASE_CHANGE);
                ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

                STOS_TaskDelay(DELAY_PERIOD * SynchTime); /* Delay for lock time now */
            }

            STTBX_Print(( "Calling STCLKRVSIM_Stop()....\n" ));
            STCLKRVSIM_Stop();                      /* Stop generating PCR,STC values */
            ErrorReport(&StoredError, ST_NO_ERROR, ST_NO_ERROR);

        }

        fclose(file_p);
        STTBX_Print(( "Calling STCLKRV_Close()....\n" ));
        ReturnError = STCLKRV_Close(Handle);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        STTBX_Print(( "Calling STCLKRV_Term()....\n" ));
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
    }
    else
    {
        STTBX_Print(("***ERROR:Could Not Open File%s****",filename));

        STTBX_Print(( "Calling STCLKRV_Close()....\n" ));
        ReturnError = STCLKRV_Close(Handle);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        STTBX_Print(( "Calling STCLKRV_Term()....\n" ));
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
        return ReturnError;
    }

    STTBX_Print(( "\n----------------------------------------------------------\n" ));
    STTBX_Print(("Completed Sync Time Test.\n"));
    STTBX_Print(( "----------------------------------------------------------\n\n" ));
    return ST_NO_ERROR;
}



/****************************************************************************
Name         : GetRealTimeTestParams()

Description  : Takes Parameters from Test Tool which act as input values
               for Random Sequence Test.
Parameters   : Pointers to parse_t structure type and char type

Return Value : TRUE or FALSE

See Also     :
 ****************************************************************************/

static BOOL GetRealTimeTestParams( parse_t *pars_p, char *result_sym_p )
{
    S32                 FreqError = 0;
    S32                 NO_Events = 0;
    S32                 Jitter    = 0;
    S32                 Seed      = 0;
    S32                 Slave1Index =0 ,Slave2Index = 0;
    BOOL                RetErr    = FALSE;

    /* 1st argument : FreqError */
    RetErr = STTST_GetInteger(pars_p,(S32)DEFAULT_FREQ_ERROR, &FreqError);
    if (RetErr == FALSE)
    {
        if (IsOutsideRange(FreqError,MIN_FREQ_ERROR,MAX_FREQ_ERROR) == TRUE)
        {
            RetErr = TRUE;
            STTST_TagCurrentLine(pars_p,"Expected INPUT: Frequency error not in rangen\n");
        }
    }

    /* 2nd argument : Events */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger(pars_p, (S32)DEFAULT_REAL_EVENTS, &NO_Events);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(NO_Events, MIN_REAL_EVENTS, MAX_REAL_EVENTS) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Number of Events in Range");
            }
        }
    }

    /* 3th argument : get Jitter */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p,(S32)DEFAULT_JITTER, &Jitter);
        if (RetErr == FALSE)
        {
            if (IsOutsideRange(Jitter,MIN_JITTER,MAX_JITTER) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Jitter in range");
            }
        }
    }

    /* 5th argument : get Seed */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p,(S32)DEFAULT_SEED, &Seed);
    }

        /* 5th argument : get slave1 Freq  index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE1, &Slave1Index);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Slave1Index, MIN_SLAVE_INDEX, MAX_SLAVE_INDEX) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }

    /* 6th argument : get  slave1 Freq Index */
    if (RetErr == FALSE)
    {
        RetErr = STTST_GetInteger( pars_p, (S32)DEFAULT_SLAVE2, &Slave2Index);
        if (RetErr == FALSE)
        {
            if(IsOutsideRange(Slave2Index, MIN_SLAVE_INDEX, MAX_SLAVE_INDEX) == TRUE)
            {
                RetErr = TRUE;
                STTST_TagCurrentLine( pars_p, "Expected: Value for Index in range");
            }
        }
    }


    if ( RetErr == FALSE ) /* i.e if parameters are ok */
    {
        STTBX_Print(( "Invoking Random Seq Test with values:\n"
                      "Freq Error    = %d\nNo. Of Events = %d\n"
                      "JITTER        = %d\nSeed          = %d\n"
                      "FS1 = %d\nFS2 = %d",
                      FreqError, NO_Events, Jitter, Seed,
                      SlaveFreqArray[Slave1Index], SlaveFreqArray[Slave2Index]));

        /* Invoke Random Sequence Test now */
        RunRealTimeTest(FreqError, NO_Events, Jitter, Seed,
                            SlaveFreqArray[Slave1Index], SlaveFreqArray[Slave2Index]);
    }
    else
    {
        STTBX_Print(("Invalid params: Can Not Run Random Sequence Test\n"));
    }

    return ( RetErr );
}




/****************************************************************************
Name         : RunRealTimeTest()

Description  : The input params from Test Tool are passed and the Random
               Sequence Test Run

Parameters   : FreqError      - Induced Error in Encoder Frequency
               Cycles         - Number of Times to Run the Test
               CycleTime      - Time for which test should run
               Jitter         - Max Jitter value
               Seed           - the Seed value to be used for Random function
               Slave freq     - Fs1
               Slave freq     - Fs2

Return Value : ST_ErrorCode_t

See Also     : STCLKRV_ErrorCode_t
 ****************************************************************************/

static ST_ErrorCode_t RunRealTimeTest(S32 FreqError, S32 Number_Events,
                                       S32 Jitter, S32 Seed,
                                       S32 FreqSlave1, S32 FreqSlave2)
{
    U32                      Toggle    = 0;
    ST_ErrorCode_t           StoredError = ST_NO_ERROR;
    ST_ErrorCode_t           ReturnError = ST_NO_ERROR;
    ST_Revision_t            RevisionStr;
    STCLKRV_SourceParams_t   PCRSrc;
    STCLKRVSIM_StartParams_t SimStart_params;
    U32                      EncoderFrequency = 0;
    STCLKRV_Handle_t         Handle;
    FILE                     *file_p = NULL;
    char                     filename[MAX_FILENAME_LENGTH];
    U32                      Status = 0;
    U32                      Random = 0;
    U32                      InitialEncoderFreq = 0;
    S32                      i;

    STTBX_Print(( "\n============================================================\n" ));
    STTBX_Print(( "Commencing Real time Test Function.....\n" ));
    STTBX_Print(( "============================================================\n\n" ));

    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));

    /* Clock recovery instance devicename */
    strcpy(DeviceNam, CLKRV_DEVNAME);

    InitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    InitParams.FSBaseAddress_p     = SimBaseAddr_p;
#if defined (ST_7200)
    InitParams.DecodeType          = STCLKRV_DECODE_PRIMARY;
    InitParams.CRUBaseAddress_p    = (U32*)(CLKRV_BASE_ADDRESS);
#endif
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    InitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    InitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    InitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    InitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;
    InitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    InitParams.PCRDriftThres       = Drift_Threshold;
    InitParams.MinSampleThres      = Min_Window_Size;
    InitParams.MaxWindowSize       = Max_Window_Size;
    InitParams.Partition_p         = TEST_PARTITION_1;

    strcpy(InitParams.PCREvtHandlerName, EVT_DEVNAME_PCR );
    strcpy(InitParams.EVTDeviceName, EVT_DEVNAME );
    strcpy(InitParams.PTIDeviceName, "STPTI" );
    STTBX_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &InitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    /* Open the Clock Recovery Driver */
    STTBX_Print(( "Calling STCLKRV_Open()....\n" ));
    ReturnError = STCLKRV_Open(DeviceNam, &OpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource() to allocate slot ID....\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    /* Set Nominal Freq for Slave Clocks */
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS1/PCM %u....\n",FreqSlave1));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0], FreqSlave1);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#ifndef ST_5188
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS2/SPDIF %u....\n",FreqSlave2));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1], FreqSlave2);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    memset(filename,'\0',MAX_FILENAME_LENGTH);
    sprintf(filename,"Realtime_%d.csv",Seed);
    file_p = fopen(filename,"w+");
    if(file_p != NULL)
    {
        EncoderFrequency = InitialEncoderFreq = (DEFAULT_SD_FREQUENCY + FreqError);


        STTBX_Print(( "\nEncoder Frequency = %u Hz\n", EncoderFrequency ));
        STTBX_Print(( "Jitter = %u Ticks\n", Jitter));

        STTBX_Print(( "Calling STCLKRVSIM_SetTestParameters()....\n"));
        ReturnError = STCLKRVSIM_SetTestParameters(EncoderFrequency,Jitter, 0);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        fprintf(file_p,"\nTest Parameters\n");
        fprintf(file_p,"PCRIntervalTime,%d\nMax_Window_Size,%d\nDrift_Threshold,%d\n",
                                                PCRIntervalTime,Max_Window_Size,Drift_Threshold);

        fprintf(file_p,"\nCode,Count,STC-Freq.,PCR,STC,Tick error,Freq Error,Avg. Error(Hz),SUM,"
            "SD-AEPS,SD-OSE,Time,FS1(PCM)Hz,FS2(SPDIF/HD)\n");

        SimStart_params.File_p               = (file_p);
        SimStart_params.PCR_Interval         = PCRIntervalTime;
        SimStart_params.PCRDriftThreshold    = Drift_Threshold;
        SimStart_params.FileRead             = 0;

        /*  Start generating PCR,STC values in Simulator */
        STTBX_Print(( "Calling STCLKRVSIM_Start()....\n"));
        ReturnError = STCLKRVSIM_Start(&SimStart_params);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
        if(ReturnError!= ST_NO_ERROR)
        {
            STTBX_Print(("Failed Simulator Random TestStart\n"));
        }
        else
        {
            STOS_TaskDelay(DELAY_PERIOD * 10);
            srand(Seed);

            for(i=0;i<(Number_Events);i++)
            {
                Random = rand()%100;
                Status = 0;

                if ((Random > 0) && (Random < 20))
                {
                    /* Generate Glitch */
                    STTBX_Print(("Glitch!!!\n"));
                    Status = MASK_BIT_GENERATE_GLITCH;
                }
                else if ((Random >25) && (Random < 55))
                {
                    /* Generate Packet Loss */
                    STTBX_Print(("PACKET DROPPED!!!\n"));
                    Status = MASK_BIT_PACKET_DROP;
                }
                else if ((Random >= 55) && (Random < 57))
                {
                    /* Generate Channel Change */
                    if (!Toggle)
                    {
                        EncoderFrequency =  InitialEncoderFreq + FreqError;
                        STTBX_Print(("\nEncoder Frequency = %u,\n",EncoderFrequency));
                        Toggle = 1;
                    }
                    else
                    {

                        EncoderFrequency =  InitialEncoderFreq - FreqError;
                        STTBX_Print(("\nEncoder Frequency = %u,\n",EncoderFrequency));
                        Toggle = 0;
                    }
                    Status = MASK_BIT_BASE_CHANGE;
                }
                else
                {
                  /* Do nothing */
                  Status = 0;
                  i--;
                }

                ReturnError  = STCLKRVSIM_SetTestParameters(EncoderFrequency,Jitter, Status);

                STOS_TaskDelay(DELAY_PERIOD); /* Delay for lock time now */
            }

            STCLKRVSIM_Stop();    /* Stop generating PCR,STC values */
        }

        fclose(file_p);
        STTBX_Print(( "Calling STCLKRV_Close()....\n"));
        ReturnError = STCLKRV_Close(Handle);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        STTBX_Print(( "Calling STCLKRV_Term()....\n"));
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    }
    else
    {
        STTBX_Print(("***ERROR:Could Not Open File%s****\n",filename));

        STTBX_Print(( "Calling STCLKRV_Close()....\n"));
        ReturnError = STCLKRV_Close(Handle);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        STTBX_Print(( "Calling STCLKRV_Term()....\n"));
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
        return ReturnError;
    }

    STTBX_Print(( "\n----------------------------------------------------------\n" ));
    STTBX_Print(("Completed Real Time Test.\n"));
    STTBX_Print(( "----------------------------------------------------------\n\n" ));
    return ST_NO_ERROR;
}

/****************************************************************************
Name         : GetReplayFromFileTestParams()

Description  : Takes Parameters from Test Tool which act as input values
               for FileRead Test.
Parameters   : Pointers to parse_t structure type and char type

Return Value : TRUE or FALSE

See Also     :
 ****************************************************************************/

static BOOL GetReplayFromFileTestParams( parse_t *pars_p, char *result_sym_p )
{
    char FileName[MAX_FILENAME_LENGTH];
    S32  FileReadTime = 0;
    BOOL RetErr = FALSE;


    memcpy(FileName, DEFAULT_FILE, (strlen(DEFAULT_FILE)+1));

    /* 1st argument : FreqError */
    RetErr = STTST_GetInteger(pars_p,(S32)MAX_READ_TIME, &FileReadTime);
    if (RetErr == FALSE)
    {
        if (IsOutsideRange(FileReadTime,MIN_READ_TIME,MAX_READ_TIME) == TRUE)
        {
            RetErr = TRUE;
            STTST_TagCurrentLine(pars_p,"Expected INPUT: Read time in range\n");
        }
    }

    STTBX_Print(( "Calling REPLAY FROM FILE TEST with values:\n"
                  "File Name = %s\nRead Time = %d\n",FileName,FileReadTime));

    RunReplayFromFileTest(FileName, FileReadTime);

    return ( RetErr );
}

/****************************************************************************
Name         : RunReplayFromFileTest()

Description  : The input params from Test Tool are passed and the File
               Read Test Run

Parameters   : FileName_p      - Pointer to file name
               TestTime        - Time for which to run

Return Value : ST_ErrorCode_t

See Also     : STCLKRV_ErrorCode_t
****************************************************************************/

static ST_ErrorCode_t RunReplayFromFileTest(char* FileName_p, S32 TestTime)
{
    ST_ErrorCode_t           StoredError = ST_NO_ERROR;
    ST_ErrorCode_t           ReturnError;
    ST_Revision_t            RevisionStr;
    STCLKRV_SourceParams_t   PCRSrc;
    STCLKRVSIM_StartParams_t SimStart_params;
    STCLKRV_Handle_t         Handle;
    FILE*                    file_p = NULL;
    char                     filename[MAX_FILENAME_LENGTH];

    STTBX_Print(( "\n============================================================\n" ));
    STTBX_Print(( "Commencing REPLAY (PCR,STC)FILE Test Function....\n" ));
    STTBX_Print(( "============================================================\n\n" ));

    STTBX_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STTBX_Print(( "Software Revision reported as %s\n", RevisionStr ));

    /* Clock recovery instance devicename */
    strcpy(DeviceNam, CLKRV_DEVNAME);

    InitParams.DeviceType          = CLK_REC_DEVICE_TYPE;
    InitParams.FSBaseAddress_p     = SimBaseAddr_p;
#if defined (ST_7200)
    InitParams.DecodeType          = STCLKRV_DECODE_PRIMARY;
    InitParams.CRUBaseAddress_p    = (U32*)(CLKRV_BASE_ADDRESS);
#endif
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    InitParams.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    InitParams.ADSCBaseAddress_p   = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
    InitParams.InterruptNumber     = CLK_REC_TRACKING_INTERRUPT;
    InitParams.InterruptLevel      = CLK_REC_TRACKING_INTERRUPT_LEVEL;
    InitParams.PCRMaxGlitch        = STCLKRV_PCR_MAX_GLITCH;
    InitParams.PCRDriftThres       = Drift_Threshold;
    InitParams.MinSampleThres      = Min_Window_Size;
    InitParams.MaxWindowSize       = Max_Window_Size;
    InitParams.Partition_p         = TEST_PARTITION_1;

    strcpy(InitParams.PCREvtHandlerName, EVT_DEVNAME_PCR );
    strcpy(InitParams.EVTDeviceName, EVT_DEVNAME );
    strcpy(InitParams.PTIDeviceName, "STPTI" );
    STTBX_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &InitParams);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    /* Open the Clock Recovery Driver */

    STTBX_Print(( "Calling STCLKRV_Open()....\n" ));
    ReturnError = STCLKRV_Open(DeviceNam, &OpenParams, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR);

    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource() to allocate slot ID....\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    /* Set Nominal Freq for Slave Clocks */
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS1/PCM %u....\n", SlaveFreqArray[DEFAULT_SLAVE1]));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0], SlaveFreqArray[DEFAULT_SLAVE1]);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

#ifndef ST_5188
    STTBX_Print(( "Calling STCLKRV_SetNominalFreq() For FS2/SPDIF %u....\n",SlaveFreqArray[DEFAULT_SLAVE2]));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1], SlaveFreqArray[DEFAULT_SLAVE2]);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif

    memset(filename,'\0',MAX_FILENAME_LENGTH);
    sprintf(filename,"replay_test.csv");
    file_p = fopen(filename,"w+");

    if(file_p !=NULL)
    {
        fprintf(file_p,"\nTest Parameters\nFileName = %s\n",FileName_p);
        fprintf(file_p,"PCRIntervalTime = %d\nMax_Window_Size = %d\nDrift_Threshold = %d\n",
                                                PCRIntervalTime,Max_Window_Size,Drift_Threshold);
        fprintf(file_p,"\nCode,Count,STC-Freq.,PCR,STC,Tick error,Freq Error,Avg. Error(Hz),SUM,"
            "SD-AEPS,SD-OSE,Time,FS1(PCM)Hz,FS2(SPDIF/HD)\n");

        SimStart_params.File_p               = (file_p);
        SimStart_params.PCR_Interval         = PCRIntervalTime;
        SimStart_params.PCRDriftThreshold    = Drift_Threshold;
        SimStart_params.FileRead             = 1;
        SimStart_params.TestFile             = FileName_p;

        STTBX_Print(( "Calling STCLKRVSIM_Start....\n"));
        ReturnError = STCLKRVSIM_Start(&SimStart_params);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
        if(ReturnError!= ST_NO_ERROR)
        {
            STTBX_Print(("Failed Simulator Random TestStart\n"));
        }
        else
        {
            STTBX_Print((".....Please Wait for %d seconds....\n",TestTime));
            STOS_TaskDelay(DELAY_PERIOD * TestTime);
            STCLKRVSIM_Stop();     /* Stop generating PCR,STC values */
        }
        fclose(file_p);
        STTBX_Print(( "Calling STCLKRV_Close()....\n"));
        ReturnError = STCLKRV_Close(Handle);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

        STTBX_Print(( "Calling STCLKRV_Term()....\n"));
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
    }
    else
    {
        STTBX_Print(("***ERROR:Could Not Open File %s****\n",filename));
        STTBX_Print(( "Calling STCLKRV_Close()....\n"));
        ReturnError = STCLKRV_Close(Handle);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
        STTBX_Print(( "Calling STCLKRV_Term()....\n"));
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams);
        ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
        return ReturnError;
    }

    STTBX_Print(( "\n----------------------------------------------------------\n" ));
    STTBX_Print(("Completed File Read Test.\n"));
    STTBX_Print(( "----------------------------------------------------------\n\n" ));
    return ST_NO_ERROR;
}

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
static void STCLKRVT_EventCallback( STEVT_CallReason_t      Reason,
                                    const ST_DeviceName_t   RegistrantName,
                                    STEVT_EventConstant_t   Event,
                                    const void              *EventData,
                                    const void              *SubscriberData_p
                                  )
{
    /* Only one instance, so should be true alays... */
    if (strcmp(RegistrantName,DeviceNam) == 0)
    {
        switch ( Event )
        {
            case STCLKRV_PCR_VALID_EVT:
                STTBX_Print( ("STCLKRV_PCR_VALID_EVT event received\n") );
                break;

            case STCLKRV_PCR_DISCONTINUITY_EVT:
                STTBX_Print( ("STCLKRV_PCR_DISCONTINUITY_EVT event received\n") );
                break;

            case STCLKRV_PCR_GLITCH_EVT:
                STTBX_Print( ("STCLKRV_PCR_GLITCH_EVT event received\n") );
                break;

            default:
                STTBX_Print( ("ERROR: Unknown STCLKRV event received\n") );
                break;
        }
    }
}

/****************************************************************************
Name         : IsOutsideRange()

Description  : Checks if U32 formal parameter 1 is outside the range of
               bounds (U32 formal parameters 2 and 3).

Parameters   : Value for checking, followed by the two bounds, preferably
               the lower bound followed by the upper bound.

Return Value : TRUE  if parameter is outside given range
               FALSE if parameter is within  given range
 ****************************************************************************/

static BOOL IsOutsideRange( S32 ValuePassed,
                     S32 LowerBound,
                     S32 UpperBound
                   )
{
    U32 Temp;

    /* ensure bounds are the correct way around */
    if( UpperBound < LowerBound ) /* bounds require swapping */
    {
        Temp       = LowerBound;
        LowerBound = UpperBound;
        UpperBound = Temp;
    }
    if( ( ValuePassed < LowerBound ) ||
        ( ValuePassed > UpperBound ) )
    {
        return( TRUE );
    }
    else
    {
        return( FALSE );
    }
}


/****************************************************************************
Name         : ErrorReport()

Description  : Outputs the appropriate error message string.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     :
 ****************************************************************************/

static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr
                )
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
            STTBX_Print(( "STCLKRV_ERROR_INVALID_FREQUENCY - Error occured in CLKRV Set Nominal\n" ));
            break;

        case STCLKRV_ERROR_INVALID_SLAVE:
            STTBX_Print(( "STCLKRV_ERROR_INVALID_SLAVE - Error occured in CLKRV Set Nominal\n" ));
            break;

        default:
            STTBX_Print(( "*** Unrecognised return code ***\n" ));
    }

    /* if ErrorGiven does not match ExpectedErr, update
       ErrorStore only if no previous error was noted    */
    if( ErrorGiven != ExpectedErr )
    {
        if( *ErrorStore == ST_NO_ERROR )
        {
            *ErrorStore = ErrorGiven;
            STTBX_Print(("*** EXITING TEST  ***\n"));
        }
    }

    STTBX_Print(( "----------------------------------------------------------\n" ));
}


#ifdef SLAVE_FREQ_CHANGE_TEST

/****************************************************************************
Name         : ChangeSlave1Freq()

Description  : Changes Slaves clock frequencies after some time

Parameters   : None

Return Value : none

See Also     :
 ****************************************************************************/


static void ChangeSlave1Freq(void * NtUSd)
{
    U32 Irndm;
    ST_ErrorCode_t ErrCode;

    STOS_TaskDelay(DELAY_PERIOD * SlaveFreqChangeTime);

    while(1)
    {
        Irndm = rand()%MAX_SLAVE_INDEX;
        if (T1Term == 0)
            break;
        /* Set Nominal Freq for Slave Clocks */
        STTBX_Print(( "Setting FS1 = %u\n",SlaveFreqArray[Irndm]));
        ErrCode = STCLKRV_SetNominalFreq(GlbClkrvHndl, TestSlaveClock[0], SlaveFreqArray[Irndm]);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(( "Error in STCLKRV_SetNominalFreq(FS1), %u\n",ErrCode));
        }

        STOS_TaskDelay(DELAY_PERIOD * SlaveFreqChangeTime);
    }
}

/****************************************************************************
Name         : ChangeSlave1Freq()

Description  : Changes Slaves clock frequencies after some time

Parameters   : None

Return Value : none

See Also     :
 ****************************************************************************/

static void ChangeSlave2Freq(void * NtUSd)
{
    U32 Irndm;
    ST_ErrorCode_t ErrCode;

    STOS_TaskDelay(DELAY_PERIOD * SlaveFreqChangeTime);

    while(1)
    {
        Irndm = rand()%MAX_SLAVE_INDEX;
        if (T2Term == 0)
            break;
        STTBX_Print(( "Setting FS2 = %u\n",SlaveFreqArray[Irndm]));
        ErrCode = STCLKRV_SetNominalFreq(GlbClkrvHndl, TestSlaveClock[1], SlaveFreqArray[Irndm]);
        if (ErrCode != ST_NO_ERROR)
        {
            STTBX_Print(( "Error in STCLKRV_SetNominalFreq(FS2), %u\n",ErrCode));
        }
        STOS_TaskDelay(DELAY_PERIOD * SlaveFreqChangeTime);
    }
}
#endif

/* End of clksimt.c */
