/****************************************************************************

File Name   : stclkrvt.c

Description : Clock Recovery API Standard Test Routines

Copyright (C) 2007, ST Microelectronics

References  :

$ClearCase (VOB: stclkrv)

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 5.0

****************************************************************************/



/* Includes --------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>        /* Needed for print if STTBX fails to init */
#include <string.h>

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
#include "stboot.h"
#include "stlite.h"
#include "stsys.h"
#include "sttbx.h"
#endif

#include "stos.h"
#include "stdevice.h"
#include "stclkrv.h"
#include "stevt.h"
#include "stpio.h"

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




/* Private Constants ------------------------------------------------------ */

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            0x100000

#define STCLKRV_HANDLE            1 /* white-box knowledge of STCLKRV */

#define CONV90K_TO_27M            300           /* 90kHz to 27MHz conversion */
#define DELAY_PERIOD              ST_GetClocksPerSecond()

#if defined(DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4) ||  defined (DVD_TRANSPORT_DEMUX)
#define CLKRV_SLOT_1  1
#define CLKRV_SLOT_2  2
#endif

/* Test clock recovery init params defines */
#define TEST_PCR_DRIFT_THRES      200
#define TEST_MIN_SAMPLE_THRES     10
#define TEST_MAX_WINDOW_SIZE      50

#define CLK_REC_TRACKING_INTERRUPT_LEVEL 7

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) || defined(ST_5188)
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
#elif defined (ST_5525)
#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_TRACKING_INTERRUPT_1  MPEG_CLK_REC_1_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_5525
#elif defined (ST_7200)
#define CLK_REC_TRACKING_INTERRUPT    MPEG_CLK_REC_INTERRUPT;
#define CLK_REC_TRACKING_INTERRUPT_1  MPEG_CLK_REC_1_INTERRUPT;
#define CLK_REC_DEVICE_TYPE           STCLKRV_DEVICE_TYPE_7200
#define AUD_PCM_CLK_FS_BASE_ADDRESS   AUDIO_IF_BASE_ADDRESS
#endif

#define MAX_SLAVE_CLOCK 2

#define MASTER_CLK_NAME  "SD/STC"
#define SLAVE1_CLK_NAME "PCM"

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301)|| defined (ST_5107) || defined(ST_5188)
#define SLAVE2_CLK_NAME "SPDIF"
#elif defined (ST_7710) || defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
#define SLAVE2_CLK_NAME "HD"
#endif

/* The following table stores pairs of PCR and STC counters in 27MHz units,
   preceded by the boolean flag indicating whether or not there is an
   explicit PCR jump.  It generates the the three events in the clock recovery
   The recovery is faster than for an implicit PCR jump. */

const U32 PcrEvtTable[][3] = { { FALSE, 1080080,  1080000 },
                              { FALSE,  2160160,  2160000 },
                              { FALSE,  3240240,  3240000 },
                              {  TRUE,  4320320,  4320000 }, /* explicit */
                              { FALSE,  5400400,  5400000 },
                              { FALSE,  6480480,  6480000 },
                              { FALSE,  7560560,  7560000 },
                              { FALSE,  8640640,  8640000 },
                              { FALSE,  9720720,  9720000 },
                              { FALSE, 10800800, (10800000 + 1000) }, /* Glitch STC delayed */
                              { FALSE, 11880880, 11880002 },
                              { FALSE, 12960960, 12960004 },
                              { FALSE, 14041040, 14040006 },
                              { FALSE, 15121120, 15120008 },
                              { FALSE, 16201200, 16200012 },
                              { FALSE, 17281280, 17280016 },
                              { FALSE, 18361360, (18360020 + 1000) }, /* Glitch STC delayed */
                              { FALSE, 19441440, (19440026 + 1000) }, /* Glitch STC delayed */
                              { FALSE, 20521520, 20520032 },
                              { FALSE, 21601600, 21600038 },
                              { FALSE, 22681680, 22680044 },
                              { FALSE, 21601600, 21600038 },
                              { FALSE, 22681680, 22680044 },
                              { FALSE, 23761760, 23760053 },
                              { FALSE, 24841840, 24840061 },
                              { FALSE, 25921920, 25920069 },
                              { FALSE,  1080080, 27000079 }, /* Simulate channel change */
                              { FALSE,  2160160, 28080089 },
                              { FALSE,  3240240, 29160100 },
                              { FALSE,  4320320, 30240110 },
                              { FALSE,  5400400, 31320122 },
                              { FALSE,  6480480, 32400134 },
                              { FALSE,  7560560, 33480147 }
                             };


static const U8  HarnessRev[] = "5.4.0";

/* globals identifying Slaves on various boards */

/* 5100     7710
   PCM      PCM
   SPDIF    HD */


#if defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) || defined (ST_5188)
#define MAX_SLAVES 2
static STCLKRV_ClockSource_t TestSlaveClock[MAX_SLAVES] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_SPDIF_0};
U32 TestSlaveFrequency[MAX_SLAVES] = { 8192000, 11300000};
#elif defined (ST_7710)
#define MAX_SLAVES 2
static STCLKRV_ClockSource_t TestSlaveClock[MAX_SLAVES] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_HD_0};
U32 TestSlaveFrequency[MAX_SLAVES] = { 11300000, 148500000};
#elif defined (ST_5525)
#define MAX_SLAVES 5
static STCLKRV_ClockSource_t TestSlaveClock[MAX_SLAVES] = { STCLKRV_CLOCK_PCM_0,
                                                   STCLKRV_CLOCK_PCM_1,
                                                   STCLKRV_CLOCK_PCM_2,
                                                   STCLKRV_CLOCK_PCM_3,
                                                   STCLKRV_CLOCK_SPDIF_0 };
U32 TestSlaveFrequency[MAX_SLAVES] =             { 8192000,
                                                   12288000,
                                                   22579200,
                                                   74250000,
                                                   148500000 };
#elif defined (ST_7100) || defined (ST_7109)
#define MAX_SLAVES 4
static STCLKRV_ClockSource_t TestSlaveClock[MAX_SLAVES] = {STCLKRV_CLOCK_PCM_0, STCLKRV_CLOCK_HD_0, STCLKRV_CLOCK_PCM_1, STCLKRV_CLOCK_SPDIF_0 };
U32 TestSlaveFrequency[MAX_SLAVES] = { 8192000, 98304000, 22579200, 148500000};
#elif defined (ST_7200)
#define MAX_SLAVES 7
static STCLKRV_ClockSource_t TestSlaveClock[MAX_SLAVES] = { STCLKRV_CLOCK_PCM_0,
                                                     STCLKRV_CLOCK_PCM_1,
                                                     STCLKRV_CLOCK_PCM_2,
                                                     STCLKRV_CLOCK_PCM_3,
                                                     STCLKRV_CLOCK_SPDIF_HDMI_0,
                                                     STCLKRV_CLOCK_SPDIF_0,
                                                     STCLKRV_CLOCK_HD_0};
U32 TestSlaveFrequency[MAX_SLAVES] =               { 8192000,
                                                     12288000,
                                                     22579200,
                                                     74250000,
                                                     130000000,
                                                     148500000,
                                                     189000000 };
#ifdef STCLKRV_NO_PTI
#if defined (ST_7200) || defined (ST_5525)
static STCLKRV_ClockSource_t TestSlaveClockAux[MAX_SLAVES] = { STCLKRV_CLOCK_PCM_0,
                                                     STCLKRV_CLOCK_PCM_1,
                                                     STCLKRV_CLOCK_PCM_2,
                                                     STCLKRV_CLOCK_PCM_3,
                                                     STCLKRV_CLOCK_SPDIF_HDMI_0,
                                                     STCLKRV_CLOCK_SPDIF_0,
                                                     STCLKRV_CLOCK_HD_1};
#endif
#endif
#endif

/* Private Types ---------------------------------------------------------- */


/*#ifndef STCLKRV_NO_PTI
 typedef void (*UpdatedPCR)(STEVT_CallReason_t EventReason,
                           STEVT_EventConstant_t EventOccured,
#ifdef ST_5188
                           STDEMUX_EventData_t *PcrData_p);
#else
                           STPTI_EventData_t *PcrData_p);
#endif
#endif*/

/* Declarations for memory partitions */
#define                 SYSTEM_MEMORY_SIZE         5000000

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

#if defined(ST_7710) || defined(ST_5100) || defined (ST_5105) || defined (ST_5188) || defined (ST_5107)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif

#elif defined(ST_OS21)

static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#endif /* ST_OS21 */

/* Memory partitions */
#ifdef ST_OS21

#define TEST_PARTITION_1        system_partition
#define TEST_PARTITION_2        system_partition

#elif defined(ST_OS20)

#define TEST_PARTITION_1         &the_system_partition
#define TEST_PARTITION_2         &the_internal_partition

#else /* Linux */

#define TEST_PARTITION_1         NULL
#define TEST_PARTITION_2         NULL

#endif
/* Private Variables ------------------------------------------------------ */

static ST_ErrorCode_t          retVal;
#ifdef ST_OS20
#pragma ST_section             ( retVal,   "ncache_section")
#endif

static ST_DeviceName_t         EVTDevNam;


static STEVT_InitParams_t      EVTInitPars;
static STEVT_TermParams_t      EVTTermPars;

#ifndef STCLKRV_NO_PTI
static STEVT_OpenParams_t      EVTOpenPars;
static STEVT_Handle_t          EVTHandle;
#endif /* STCLKRV_NO_PTI */

#if defined(DVD_TRANSPORT_STPTI) || defined (DVD_TRANSPORT_STPTI4) || defined (DVD_TRANSPORT_DEMUX)
STEVT_Handle_t          EVTHandlePcr;
ST_DeviceName_t         EVTDevNamPcr;
#endif

/* Track events received from STCLKRV */
static BOOL NewEventRxd;
static STEVT_EventConstant_t EventReceived;

static ST_DeviceName_t         DeviceNam;
static STCLKRV_InitParams_t    CLKRVInitPars;
static STCLKRV_OpenParams_t    CLKRVOpenPars;
static STCLKRV_TermParams_t    CLKRVTermPars;
static STCLKRV_Handle_t        Handle;

#ifndef STCLKRV_NO_PTI

static STCLKRV_Handle_t        Handle2;
static STCLKRV_Handle_t        Handle3;

/*UpdatedPCR                     ActionPCR;*/  /* pointer to PTI event function */

#ifdef ST_5188
/*ST_ErrorCode_t STDEMUX_GetPacketArrivalTime(STDEMUX_Handle_t Hndl,
                                          STDEMUX_TimeStamp_t * ArrivalTime_p,
                                          U16 *ArrivalTimeExtension_p);*/
ST_ErrorCode_t STDEMUX_GetCurrentPTITimer(ST_DeviceName_t DeviceName,
                                          STDEMUX_TimeStamp_t * TimeStamp);
#else
ST_ErrorCode_t STPTI_GetPacketArrivalTime(STPTI_Handle_t Hndl,
                                          STPTI_TimeStamp_t * ArrivalTime_p,
                                          U16 *ArrivalTimeExtension_p);
ST_ErrorCode_t STPTI_GetCurrentPTITimer(ST_DeviceName_t DeviceName,
                                          STPTI_TimeStamp_t * TimeStamp);
#endif

#endif /* STCLKRV_NO_PTI */


/* Pass/fail counts */
typedef struct
{
    U32 NumberPassed;
    U32 NumberFailed;
} CLKRV_TestResult_t;

/* Test variables */
static CLKRV_TestResult_t Result;
static BOOL Passed = TRUE;

/* Function prototypes ---------------------------------------------------- */



/* Private Function prototypes -------------------------------------------- */
#ifndef STCLKRV_NO_PTI
static void NormalUse( void );
static void ErrantUse( void );
#else
static void NormalUseNoPTI( void );
#endif


static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr );

#ifndef STCLKRV_NO_PTI
static ST_ErrorCode_t CheckEventGenerated( U32 TestSet, U32 TestIteration );

static void STCLKRVT_EventCallback( STEVT_CallReason_t Reason,
                                    const ST_DeviceName_t RegistrantName,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData,
                                    const void *SubscriberData_p);

static STEVT_DeviceSubscribeParams_t EVTSubscrParams =
                                   { STCLKRVT_EventCallback};
#endif /* STCLKRV_NO_PTI */

void PrintPassFail(BOOL IsPassed, char *PassMsg, char *FailMsg);

/* Private Varaibles --------------------------------------------------------- */

extern ST_ErrorCode_t STCLKRV_GetClocksFrequency( STCLKRV_Handle_t Handle,
                                                U32 *pSTCFrequency,
                                                U32 *pFS1Frequency,
                                                U32 *pFS2Frequency,
                                                U32 *pFS3Frequency,
                                                U32 *pFS4Frequency,
                                                U32 *pFS5Frequency,
                                                U32 *pFS6Frequency );

/* Functions -------------------------------------------------------------- */

#if 0
#include "clkrvreg.h"
#include "stsys.h"


#define CLKRV_DELAY(x) { int temp; for(temp=0;temp<x;temp++); }

semaphore_t xsem;


__inline void clkrv_writeReg1(STSYS_DU32 *Base, U32 Reg, U32 Value)
{
    U32 *Regaddr;

    Regaddr = (U32 *)((U32)Base + Reg);
    *Regaddr = Value;
}


__inline void clkrv_readReg1(STSYS_DU32 *Base, U32 Reg, U32 *Value)
{
    U32 *Regaddr;

    Regaddr = (U32 *)((U32)Base + Reg);
    *Value = *Regaddr;
}

/***************************** End of Utility Functions ********************/

    U32 SlaveCounter1=0;
    U32 SlaveCounter2 =0;

void ClkrvTrackingInterruptHandlerEmu( void *Data )
{
    STSYS_DU32 *BaseA = (U32 *)CKG_BASE_ADDRESS;


    /* Read Latched Counter Values */
    clkrv_readReg1(BaseA, CAPTURE_COUNTER_PCM, &SlaveCounter1);
    clkrv_readReg1(BaseA, CAPTURE_COUNTER_SPDIF, &SlaveCounter2);

    STOS_SemaphoreSignal(&xsem);


    /* Clear Interrupt so that it can come again */
    clkrv_writeReg1(BaseA, CLK_REC_CMD, 0x02 ); /* ? ? ? ? ? ? ?  reserved bits */
#if defined(ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) || defined(ST_5188)
    CLKRV_DELAY(1); /* HW Bug DDTS -- */
#endif
    clkrv_writeReg1(BaseA, CLK_REC_CMD, 0x00 );

    clkrv_writeReg1(BaseA, CLK_REC_CMD, 0x01);
#if defined(ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107) || defined(ST_5188)
    CLKRV_DELAY(2);
#endif
    clkrv_writeReg1(BaseA, CLK_REC_CMD, 0x00);



}

void emu()
{
int IntReturn;
    ST_ErrorCode_t ReturnCode              = ST_NO_ERROR;
    ST_ErrorCode_t RetCode                 = ST_NO_ERROR;
    STSYS_DU32 *BaseA = (U32 *)CKG_BASE_ADDRESS;


    clkrv_writeReg1(BaseA, 0x300, 0xf0);
    clkrv_writeReg1(BaseA, 0x300, 0x0f);

    {
        U32 Val;
        clkrv_readReg1( BaseA, 0x10, &Val);
        printf ("fs = %u\n",Val);
    }



    /* Install DCO Interrupt */

#if 0

    {
        U32 Val,Val1,Val2,Val3;



        clkrv_writeReg(BaseA, 0x200, 0x00);
        clkrv_writeReg(BaseA, 0x204, 0x00);
        clkrv_writeReg(BaseA, 0x208, 0x01);

        STOS_TaskDelay(1);
        clkrv_readReg( BaseA, 0x214, &Val1);
        STOS_TaskDelay(1);
        clkrv_readReg( BaseA, 0x214, &Val2);
        STOS_TaskDelay(1);
        clkrv_readReg( BaseA, 0x214, &Val3);
        STOS_TaskDelay(1);

        printf ("cnt =%u, %u, %u \n",Val1, Val2, Val3);

        while(1);
        clkrv_readReg( BaseA, 0x210, &Val);
        printf ("cnt =%u\n",Val);


        clkrv_readReg( BaseA, 0x214, &Val);
        printf ("cnt = %u\n",Val);

        /*clkrv_writeReg(BaseA, 0x218, 0x01);*/

        clkrv_readReg( BaseA, 0x210, &Val);
        printf ("cnt =%u\n",Val);

        clkrv_readReg( BaseA, 0x214, &Val);
        printf ("cnt =%u\n",Val);

        while(1);


    }
#endif







    if (ReturnCode == ST_NO_ERROR)
    {

        IntReturn = STOS_InterruptInstall( DCO_CORRECTION_INTERRUPT,
                                           7,
                                           ClkrvTrackingInterruptHandlerEmu,
                                           "DCO",
                                           NULL);
        if (IntReturn == 0)
        {
            /* interrupt_enable call will be redundant after change will be done in STBOOT*/
            IntReturn = STOS_InterruptEnable(DCO_CORRECTION_INTERRUPT,7);

            if(IntReturn != 0)
            {
                ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
                printf ("Error 1\n");
            }
        }
        else
        {
            ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
            printf ("Error 2");
        }
    }



    clkrv_writeReg1( BaseA, REF_MAX_CNT, 27000000);

    /* ISR may deschedule this and may lead to Glitchy clock */
    /* ReSet DCO_CMD  to 0x01 */
    clkrv_writeReg1(BaseA, CLK_REC_CMD, 0x01);
    /* Delay is required for 7710 ?? */


    clkrv_writeReg1(BaseA, CLK_REC_CMD, 0x00);

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
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
#ifndef STCLKRV_NO_TBX
    STTBX_InitParams_t      sttbx_InitPars;
#endif
#endif/* linux changes */

#if defined (ST_7100)  || defined (ST_7109) || defined (ST_7200)
    STPIO_InitParams_t      PIOInitParams;
    STPIO_OpenParams_t      PIOOpenParams;
    STPIO_Handle_t          PIOHandle;
    STPIO_TermParams_t      PIOTermParams;
#endif

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
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
#else
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF },
        { NULL, NULL }                  /* End */
    };
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition  = partition_create_heap((U8*)external_block, sizeof(external_block));

#else
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
    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;
#endif

#if defined(ST_7710) || defined(ST_5100) || defined (ST_5105) || defined (ST_5188) || defined (ST_5107) || defined(ST_5188)
    data_section[0]  = 0;
#endif
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
     stboot_InitPars.CacheBaseAddress  = (U32*) CACHE_BASE_ADDRESS;
#if defined (ST_5525)
     stboot_InitPars.DCacheMap         = NULL;
#else
     stboot_InitPars.DCacheMap         = DCacheMap;
#endif

     stboot_InitPars.MemorySize        = SDRAM_SIZE;
     retVal = STBOOT_Init( "CLKRVTest", &stboot_InitPars );
    if( retVal != ST_NO_ERROR )
    {
         printf("ERROR: STBOOT_Init() returned %d\n", retVal);
    }
    else
    {
#ifndef STCLKRV_NO_TBX
        /* Initialize the toolbox */
        sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

        sttbx_InitPars.CPUPartition_p = TEST_PARTITION_1;

        retVal = STTBX_Init( "tbx", &sttbx_InitPars );
#endif
        if( retVal != ST_NO_ERROR )
        {
            printf( "ERROR: STTBX_Init() returned %d\n", retVal );
            return -1;
        }
    }
#endif
    {
#if defined (ST_7710)
        /* monitor clocks */
        volatile U32 *addptr = (U32 *)0x380000C8;
        U32 regval = 0x5AA50000;
        U32 regval2 = 0;
        regval2 = *addptr;

        *addptr = (regval2 | regval | 0x100);
#elif defined (ST_5100) || defined (ST_5105) || defined (ST_5301) || defined (ST_5107)

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
        *addptr = 0x29;
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

#ifdef STCLKRV_NO_PTI

    NormalUseNoPTI();
#else
    NormalUse();
    ErrantUse();
#endif

#if defined (ST_7100)  || defined (ST_7109) || defined (ST_7200)
    PIOTermParams.ForceTerminate = FALSE;
    retVal = STPIO_Term("PIO5", &PIOTermParams);
#endif

#ifndef ST_OSLINUX
    retVal = STBOOT_Term( "CLKRVTest", &stboot_TermPars );
#endif

    return retVal;
}

/* Tests below for STPTI compatible, multi instance version
   of the clock recovery driver. Single instance is tested
   using STPTI interface method with simulated data. */

#ifdef STCLKRV_NO_PTI
#if !defined(ST_OSLINUX)
extern U32 ClkrvCounters[];
#endif

void ProgramAuxClock(STCLKRV_ClockSource_t Clock)
{

#if defined (ST_5525)
    switch(Clock)
    {
        case STCLKRV_CLOCK_PCM_0:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x30);
            break;
        case STCLKRV_CLOCK_PCM_1:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x31);
            break;
        case STCLKRV_CLOCK_PCM_2:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x32);
            break;
        case STCLKRV_CLOCK_PCM_3:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x33);
            break;
        case STCLKRV_CLOCK_SD_0:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x34);
            break;
        case STCLKRV_CLOCK_SD_1:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x35);
            break;
        case STCLKRV_CLOCK_SPDIF_0:
            STSYS_WriteRegDev32LE((void*)(0x18400188), 0x36);
            break;
        default:
            break;
    }
#endif
}



void NormalUseNoPTI( void )
{
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    int i;

    STCLKRV_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STCLKRV_Print(( "============================================================\n" ));
    STCLKRV_Print(( "Commencing NormalUse Test Function For STCLKRV ....\n" ));
    STCLKRV_Print(( "============================================================\n\n" ));

    /* Clock recovery instance devicename */
    strcpy( DeviceNam, "STCLKRV" );

    /* Setup Event registration for notification of PCR_RECEIVED
       event and callback to ActionPCR */
    strcpy( EVTDevNam, "STEVT2" );
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum  = 9;
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STCLKRV_Print(( "Calling STEVT_Init()....\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    /* revision string available before STCLKRV Initialized */
    STCLKRV_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STCLKRV_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STCLKRV_Print(( "----------------------------------------------------------\n" ));
#endif

    /* Initialize our native Clock Recovery Driver */

    CLKRVInitPars.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitPars.PCRDriftThres     = TEST_PCR_DRIFT_THRES;
    CLKRVInitPars.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    CLKRVInitPars.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    CLKRVInitPars.Partition_p       = TEST_PARTITION_1;
    CLKRVInitPars.DeviceType        = CLK_REC_DEVICE_TYPE;
    CLKRVInitPars.FSBaseAddress_p   = (U32 *)CKG_B_BASE_ADDRESS;
#if defined (ST_7100) || defined (ST_7109) || defined (ST_7200)
    CLKRVInitPars.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#elif defined (ST_7710)
    CLKRVInitPars.ADSCBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif

#if defined (ST_5525) || defined (ST_7200)
    CLKRVInitPars.DecodeType        = STCLKRV_DECODE_PRIMARY;
#endif
#ifdef ST_7200
    CLKRVInitPars.CRUBaseAddress_p  = (U32*)(CLKRV_BASE_ADDRESS);
#endif
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitPars.InterruptLevel    = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitPars.PCREvtHandlerName, EVTDevNamPcr );
    strcpy( CLKRVInitPars.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitPars.PTIDeviceName, "PTI" );

#if defined (ST_5525)
    STSYS_WriteRegDev32LE((void*)(0x18400188), 0x34);
#endif

#if defined (ST_5188)
    STSYS_WriteRegDev32LE((void*)(0x20F00188), 0x33);
#endif

#if defined(ST_7100) || defined (ST_7109)
    {
        volatile U32 *addptr = (U32 *)0xB90000B4;
        *addptr = 0x8;
    }
#endif

#ifndef ST_OSLINUX
#if defined(ST_7200)
    {
        volatile U32 *addptr = (U32 *)0xfd701054;
        *addptr = 0xd5;
    }
#endif
#endif

#if defined (ST_5525) || defined (ST_7200)
    CLKRVInitPars.DecodeType        = 3;
    STCLKRV_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    CLKRVInitPars.DecodeType        = STCLKRV_DECODE_PRIMARY;
#endif

    STCLKRV_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Monitor Pixel clock\n" ));
    STOS_TaskDelay(ST_GetClocksPerSecond());

#if defined(ST_OSLINUX)
    /* revision string available before STCLKRV Open ( Linux) */
    STCLKRV_Print(( "Calling STCLKRV_GetRevision() preOpen....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STCLKRV_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STCLKRV_Print(( "----------------------------------------------------------\n" ));
#endif

    /* Open the Clock Recovery Driver */

    STCLKRV_Print(( "Calling STCLKRV_Open()....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set/Monitor Nominal clock */

    for (i = 0; i < MAX_SLAVES; i++ ) {

        ProgramAuxClock(TestSlaveClock[i]);
        STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
        STCLKRV_Print(( "Clock[%u].Freq = %u\n",TestSlaveClock[i],TestSlaveFrequency[MAX_SLAVES - i - 1]  ));
        ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[i],TestSlaveFrequency[MAX_SLAVES - i - 1] );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STOS_TaskDelay(ST_GetClocksPerSecond()*2);
    }

    /* delay for interrupt to occur */
    STOS_TaskDelay(ST_GetClocksPerSecond()*2);
#if !defined(ST_OSLINUX)
    for (i = 0; i < 10; i++ ) {
        STCLKRV_Print(( "%u, %u, %u, %u, %u, %u\n",ClkrvCounters[0],
                                                ClkrvCounters[1],
                                                ClkrvCounters[2],
                                                ClkrvCounters[3],
                                                ClkrvCounters[4],
                                                ClkrvCounters[5]);
        STOS_TaskDelay((ST_GetClocksPerSecond()*3)/2));

    }
#endif

    for (i = 0; i < MAX_SLAVES; i++ ) {

        ProgramAuxClock(TestSlaveClock[i]);
        STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
        STCLKRV_Print(( "Clock[%u].Freq = %u\n",TestSlaveClock[i],TestSlaveFrequency[i]  ));
        ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[i],TestSlaveFrequency[i] );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STOS_TaskDelay(ST_GetClocksPerSecond()*2);
    }

#if !defined(ST_OSLINUX)
    for (i = 0; i < 10; i++ ) {
        STCLKRV_Print(( "%u, %u, %u, %u, %u, %u\n",ClkrvCounters[0],
                                                ClkrvCounters[1],
                                                ClkrvCounters[2],
                                                ClkrvCounters[3],
                                                ClkrvCounters[4],
                                                ClkrvCounters[5]);
        STOS_TaskDelay((ST_GetClocksPerSecond()*3)/2));

    }
#endif

#if defined (ST_5525) || defined (ST_7200)

    STCLKRV_Print(( "Calling STCLKRV_Close() again (simulating 2nd thread)....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */

    CLKRVTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STCLKRV_Term() unforced....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* DUAL tests */
    /* Clock recovery instance devicename */
    strcpy( DeviceNam, "STCLKRV2" );

    /* Initialize our native Clock Recovery Driver */
    CLKRVInitPars.DecodeType        = STCLKRV_DECODE_SECONDARY;
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT_1;
#ifdef ST_7200
    CLKRVInitPars.CRUBaseAddress_p  = (U32*)(CLKRV1_BASE_ADDRESS);
#endif
    /* pixel clock 2 */
#if defined (ST_5525)
    STSYS_WriteRegDev32LE((void*)(0x18400188), 0x35);
#endif
    STCLKRV_Print(( "Monitor Pixel clock\n" ));

    STOS_TaskDelay(ST_GetClocksPerSecond()*2);

    STCLKRV_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */

    STCLKRV_Print(( "Calling STCLKRV_Open()....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* At the moment No protection between */

    /* Set/Monitor Nominal clock */

    for (i = 0; i < MAX_SLAVES; i++ ) {

        ProgramAuxClock(TestSlaveClock[i]);
        STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
#ifdef ST_5525
        STCLKRV_Print(( "Clock[%u].Freq = %u\n",TestSlaveClock[i],TestSlaveFrequency[MAX_SLAVES - i - 1]  ));
        ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[i],TestSlaveFrequency[MAX_SLAVES - i - 1] );
#else
        STCLKRV_Print(( "Clock[%u].Freq = %u\n",TestSlaveClockAux[i],TestSlaveFrequency[MAX_SLAVES - i - 1] ));
        ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClockAux[i],TestSlaveFrequency[MAX_SLAVES - i - 1] );
#endif
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STOS_TaskDelay(ST_GetClocksPerSecond()*2);
    }

#if !defined(ST_OSLINUX)
    for (i = 0; i < 10; i++ ) {
        STCLKRV_Print(( "%u, %u, %u, %u, %u, %u\n",ClkrvCounters[0],
                                                ClkrvCounters[1],
                                                ClkrvCounters[2],
                                                ClkrvCounters[3],
                                                ClkrvCounters[4],
                                                ClkrvCounters[5]);
        STOS_TaskDelay((ST_GetClocksPerSecond()*3)/2));

    }
#endif

    for (i = 0; i < MAX_SLAVES; i++ ) {

        ProgramAuxClock(TestSlaveClock[i]);
        STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
#ifdef ST_5525
        STCLKRV_Print(( "Clock[%u].Freq = %u\n",TestSlaveClock[i],TestSlaveFrequency[i]  ));
        ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[i],TestSlaveFrequency[i] );
#else
        STCLKRV_Print(( "Clock[%u].Freq = %u\n",TestSlaveClockAux[i],TestSlaveFrequency[i] ));
        ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClockAux[i],TestSlaveFrequency[i] );
#endif
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        STOS_TaskDelay(ST_GetClocksPerSecond()*2);
    }

#if !defined(ST_OSLINUX)
    for (i = 0; i < 10; i++ ) {
        STCLKRV_Print(( "%u, %u, %u, %u, %u, %u\n",ClkrvCounters[0],
                                                ClkrvCounters[1],
                                                ClkrvCounters[2],
                                                ClkrvCounters[3],
                                                ClkrvCounters[4],
                                                ClkrvCounters[5]
                                                );
        STOS_TaskDelay((ST_GetClocksPerSecond()*3)/2));

    }
#endif


#endif



#if defined (ST_7710) || defined (ST_7100)  || defined (ST_7109)

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    STCLKRV_Print(( "Clock[%u].Freq = %u\n",STCLKRV_CLOCK_HD_0,    108000000 ));
    ReturnError = STCLKRV_SetNominalFreq(Handle,STCLKRV_CLOCK_HD_0,108000000 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetApplicationMode()....\n" ));
    ReturnError = STCLKRV_SetApplicationMode(Handle, STCLKRV_APPLICATION_MODE_SD_ONLY );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* should wait here for atleast 2s second else
       last snapped values will be printed...*/

#if !defined(ST_OSLINUX)
    for (i = 0; i < 10; i++ ) {
        STCLKRV_Print(( "%u, %u, %u, %u, %u, %u\n",ClkrvCounters[0],
                                                ClkrvCounters[1],
                                                ClkrvCounters[2],
                                                ClkrvCounters[3],
                                                ClkrvCounters[4],
                                                ClkrvCounters[5]);
        STOS_TaskDelay((ST_GetClocksPerSecond()*3)/2));

    }
#endif

#endif

    STCLKRV_Print(( "Calling STCLKRV_Close() again (simulating 2nd thread)....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */

    CLKRVTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STCLKRV_Term() unforced....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close and terminate the subordinate EVT Driver */

    EVTTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STEVT_Term()....\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "\nOverall test Result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

}
#else
/****************************************************************************
Name         : NormalUse()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising
               automated control within the Clock Recovery Driver.
               Arrays of PCR and STC counters are simplified to
               27MHz values only to ease table creation/maintenance.
               The BaseValues are generated "on-the-fly" for
               passing to the ActionPCR() call.

NOTE:          This version of the NormalUse test is for the Multi instance,
               STPTI compatible version of the clock recovery driver.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_REVISION_NOT_SUPPORTED Version mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STCLKRV_ERROR_PCR_UNAVAILABLE   Valid PCR can't be returned

See Also     : STCLKRV_ErrorCode_t
 ****************************************************************************/

void NormalUse( void )
{
    U32 i = 0;
    U32 SystemTimeClock;
    U32 STCFreq, Slave1Freq, Slave2Freq;            /* returned FS Frequency setting */

    U32 Slave3Freq, Slave4Freq, Slave5Freq, Slave6Freq;


    STCLKRV_ExtendedSTC_t ExtendedSTC;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    STEVT_EventID_t CLKRVEventID;
#ifdef ST_5188
    STDEMUX_EventData_t    PcrData;
#else
    STPTI_EventData_t      PcrData;
#endif
    STCLKRV_SourceParams_t PCRSrc;
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;

    STCLKRV_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    STCLKRV_Print(( "============================================================\n" ));
    STCLKRV_Print(( "Commencing NormalUse Test Function For STCLKRV ....\n" ));
    STCLKRV_Print(( "============================================================\n\n" ));

    /* Clock recovery instance devicename */
    strcpy( DeviceNam, "STCLKRV" );

    /* Initialize the sub-ordinate EVT Driver */

    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum  = 9; /* Modified from 3 for new EVT handler */
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STCLKRV_Print(( "Calling STEVT_Init()....\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */

    STCLKRV_Print(( "Calling STEVT_Open()....\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Subscribe to PCR Events now so that we can get
       callbacks when events are registered */
    STCLKRV_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT)....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Setup Event registration for notification of PCR_RECEIVED
       event and callback to ActionPCR */
    strcpy( EVTDevNamPcr, "STEVT2" );
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum  = 9;
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STCLKRV_Print(( "Calling STEVT_Init()....\n" ));
    ReturnError = STEVT_Init( EVTDevNamPcr, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */
    STCLKRV_Print(( "Calling STEVT_Open()....\n" ));
    ReturnError = STEVT_Open( EVTDevNamPcr, &EVTOpenPars, &EVTHandlePcr );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(("Calling STEVT_Register(PCR_RECIEVED_EVT)....\n"));
#ifdef ST_5188
    ReturnError = STEVT_Register(EVTHandlePcr, (U32)STDEMUX_EVENT_PCR_RECEIVED_EVT, &CLKRVEventID);
#else
    ReturnError = STEVT_Register(EVTHandlePcr, (U32)STPTI_EVENT_PCR_RECEIVED_EVT, &CLKRVEventID);
#endif
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    /* revision string available before STCLKRV Initialized */
    STCLKRV_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STCLKRV_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STCLKRV_Print(( "----------------------------------------------------------\n" ));
#endif

    /* Initialize our native Clock Recovery Driver */

    CLKRVInitPars.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitPars.PCRDriftThres     = TEST_PCR_DRIFT_THRES;
    CLKRVInitPars.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    CLKRVInitPars.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    CLKRVInitPars.Partition_p       = TEST_PARTITION_1;
    CLKRVInitPars.DeviceType        = CLK_REC_DEVICE_TYPE;
    CLKRVInitPars.FSBaseAddress_p   = (U32 *)CKG_BASE_ADDRESS;
#if defined(ST_7100) || defined (ST_7109) || defined (ST_7200)
    CLKRVInitPars.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitPars.ADSCBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
#ifdef ST_7200
    CLKRVInitPars.CRUBaseAddress_p  = (U32*)(CLKRV_BASE_ADDRESS);
    CLKRVInitPars.DecodeType        = STCLKRV_DECODE_PRIMARY;
#endif
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitPars.InterruptLevel    = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitPars.PCREvtHandlerName, EVTDevNamPcr );
    strcpy( CLKRVInitPars.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitPars.PTIDeviceName, "PTI" );

    STCLKRV_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_OSLINUX) /* linux changes */
    /* revision string available before STCLKRV Initialized */
    STCLKRV_Print(( "Calling STCLKRV_GetRevision() preInit....\n" ));
    RevisionStr = STCLKRV_GetRevision();
    STCLKRV_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STCLKRV_Print(( "----------------------------------------------------------\n" ));
#endif

    /* Open the Clock Recovery Driver */

    STCLKRV_Print(( "Calling STCLKRV_Open()....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[0],
                                                 22579200);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_SetNominalFreq()....\n" ));
    ReturnError = STCLKRV_SetNominalFreq(Handle, TestSlaveClock[1],
                                                 108000000);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Check all PCR events have now been registered */

    STCLKRV_Print(("\n----------------------------------------------------------\n"));
    STCLKRV_Print(("TEST: Multiple Open should return SAME handle.\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    /* Open again (simulating another thread) */

    STCLKRV_Print(( "Calling STCLKRV_Open() again (simulating 2nd thread)....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle2 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    if (Handle2 == Handle)
    {
        Passed = TRUE;
    }
    else
    {
        Passed = FALSE;
    }

    PrintPassFail(Passed, "Same handle returned", "Incorrect handle returned");


    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */
    STCLKRV_Print(( "Calling STCLKRV_SetPCRSource() to allocate slot ID....\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    /* Report the Initial Frequency Value */
    STCLKRV_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));

    ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &Slave1Freq, &Slave2Freq,
                                              &Slave3Freq, &Slave4Freq, &Slave5Freq, &Slave6Freq );


    STCLKRV_Print(( "%s = %d, %s = %d, %s = %d\n", MASTER_CLK_NAME, STCFreq,
                                                 SLAVE1_CLK_NAME, Slave1Freq,
                                                 SLAVE2_CLK_NAME, Slave2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Loop on PcrEvtTable entry pairs, exercising
       the Clock Recovery Event functionality */

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("TEST: EVENTS GENERATION TEST.......\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    Passed = TRUE;

    for( i = 0; i < ( sizeof( PcrEvtTable ) / 12 ); i++ )
    {
        NewEventRxd = FALSE;   /* Clear Event received flag */

         /* Load up callback data structure and triger event */
#ifdef ST_5188
        PcrData.member.PCREventData.DiscontinuityFlag = PcrEvtTable[i][0];
        PcrData.member.PCREventData.PCRBase.LSW =  PcrEvtTable[i][1] / CONV90K_TO_27M;
        PcrData.member.PCREventData.PCRExtension = PcrEvtTable[i][1] -
                               (PcrData.member.PCREventData.PCRBase.LSW * CONV90K_TO_27M );

        PcrData.member.PCREventData.PCRArrivalTime.LSW = PcrEvtTable[i][2] / CONV90K_TO_27M;
        PcrData.member.PCREventData.PCRArrivalTimeExtension = PcrEvtTable[i][2] -
                               (PcrData.member.PCREventData.PCRArrivalTime.LSW * CONV90K_TO_27M );

        PcrData.member.PCREventData.PCRArrivalTime.Bit32 = 0;
        PcrData.member.PCREventData.PCRBase.Bit32 = 0;
        PcrData.ObjectHandle = CLKRV_SLOT_1;
#else
        PcrData.u.PCREventData.DiscontinuityFlag = PcrEvtTable[i][0];
        PcrData.u.PCREventData.PCRBase.LSW =  PcrEvtTable[i][1] / CONV90K_TO_27M;
        PcrData.u.PCREventData.PCRExtension = PcrEvtTable[i][1] -
                               (PcrData.u.PCREventData.PCRBase.LSW * CONV90K_TO_27M );

        PcrData.u.PCREventData.PCRArrivalTime.LSW = PcrEvtTable[i][2] / CONV90K_TO_27M;
        PcrData.u.PCREventData.PCRArrivalTimeExtension = PcrEvtTable[i][2] -
                               (PcrData.u.PCREventData.PCRArrivalTime.LSW * CONV90K_TO_27M );

        PcrData.u.PCREventData.PCRArrivalTime.Bit32 = 0;
        PcrData.u.PCREventData.PCRBase.Bit32 = 0;
        PcrData.SlotHandle = CLKRV_SLOT_1;
#endif

        STCLKRV_Print(( "Calling notify function\n"));
        (void)STEVT_Notify(EVTHandlePcr, CLKRVEventID, &PcrData);

        /* Time for processing */
        STOS_TaskDelay(ST_GetClocksPerSecond()/2000);

        STCLKRV_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
        ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &Slave1Freq, &Slave2Freq,
                                                 &Slave3Freq, &Slave4Freq, &Slave5Freq, &Slave6Freq );
        STCLKRV_Print(( "%s = %d, %s = %d, %s = %d\n",MASTER_CLK_NAME, STCFreq, SLAVE1_CLK_NAME,
                                                    Slave1Freq, SLAVE2_CLK_NAME, Slave2Freq));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        STOS_TaskDelay( DELAY_PERIOD );

        /* Check events are generated at the right time */

        STCLKRV_Print(( "Checking events received....\n" ));
        ReturnError = CheckEventGenerated( 0, i );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
     }

    PrintPassFail(Passed, "EVENTS Generated correctly", "ERROR in EVENTS");


    NewEventRxd = FALSE;
    STCLKRV_Print(( "Calling STCLKRV_InvDecodeClk()....\n" ));
    ReturnError =  STCLKRV_InvDecodeClk (Handle);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Check PCR discontinuity event was received */

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("TEST: Checking for STCLKRV_DISCONTINUITY event...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

#ifdef ST_OSLINUX
    sleep(1);
#endif

    Passed = TRUE;

    if( NewEventRxd == FALSE )
    {
        Passed = FALSE;
        STCLKRV_Print(( "ERROR: Missing event\n" ));
        ReturnError = STCLKRV_ERROR_EVT_REGISTER;
    }
    else if( EventReceived != (U32)STCLKRV_PCR_DISCONTINUITY_EVT )
    {
        Passed = FALSE;
        STCLKRV_Print(( "ERROR: Wrong event received\n" ));
        ReturnError = STCLKRV_ERROR_EVT_REGISTER;
    }
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PrintPassFail(Passed, "DISCONTINUITY event generated correctly",
                                                    "DISCONTINUITY event NOT generated");

    /* Force a Termination, Closing the open channel */

    CLKRVTermPars.ForceTerminate = TRUE;
    STCLKRV_Print(( "Calling STCLKRV_Term(), forced Close....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("TEST: Re-Init....\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));


    /* Re-Init the Clock Recovery */

    STCLKRV_Print(( "Calling STCLKRV_Init() after Term....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Re-Open the Clock Recovery */


    STCLKRV_Print(( "Calling STCLKRV_Open() after ReInit....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open again (simulating another thread) */

    STCLKRV_Print(( "Calling STCLKRV_Open() again (simulating 2nd thread)....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle2 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    if (Handle2 == Handle)
    {
        Passed = TRUE;
    }
    else
    {
        Passed = FALSE;
    }

    PrintPassFail(Passed, "Same handle returned", "Incorrect handle returned");


    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */
    STCLKRV_Print(( "Calling STCLKRV_SetPCRSource() to allocate slot ID....\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    /* Report the Initial Frequency Value */
    STCLKRV_Print(( "Calling STCLKRV_GetClocksFrequency()....\n" ));
    ReturnError = STCLKRV_GetClocksFrequency( Handle, &STCFreq, &Slave1Freq, &Slave2Freq,
                                              &Slave3Freq, &Slave4Freq, &Slave5Freq, &Slave6Freq );
    STCLKRV_Print(( "%s = %d, %s = %d, %s = %d\n",MASTER_CLK_NAME, STCFreq, SLAVE1_CLK_NAME,
                                                Slave1Freq, SLAVE2_CLK_NAME, Slave2Freq));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Try calling the 'decode clock' routines. Note that the 'get' will fail
       as the time value returned to it by the harness will be considered
       stale. */

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("TEST: Get STC/ExtendedSTC will FAIL as (Simulation - NO PTI )\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));


    STCLKRV_Print(( "Calling STCLKRV_GetSTC()....\n" ));
    ReturnError =  STCLKRV_GetSTC (Handle, &SystemTimeClock);
    ErrorReport( &StoredError, ReturnError, STCLKRV_ERROR_PCR_UNAVAILABLE );

    STCLKRV_Print(( "Calling STCLKRV_GetExtendedSTC()....\n" ));
    ReturnError =  STCLKRV_GetExtendedSTC (Handle, &ExtendedSTC);
    ErrorReport( &StoredError, ReturnError, STCLKRV_ERROR_PCR_UNAVAILABLE );

    PrintPassFail(Passed, "PCR UNAVAILABLE returned", "Error in GetSTC");


    /* Close Clock Recovery again */

    STCLKRV_Print(( "Calling STCLKRV_Close() again (simulating 2nd thread)....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */

    CLKRVTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STCLKRV_Term() unforced....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_OPEN_HANDLE );

    STCLKRV_Print(( "Calling STCLKRV_Close() again (simulating 2nd thread)....\n" ));
    ReturnError = STCLKRV_Close( Handle2 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    CLKRVTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STCLKRV_Term() unforced....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close and terminate the subordinate EVT Driver */

    STCLKRV_Print(( "Calling STEVT_Close()....\n" ));
    ReturnError = STEVT_Close( EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STEVT_Close()....\n" ));
    ReturnError = STEVT_Close( EVTHandlePcr );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    EVTTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STEVT_Term()....\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    EVTTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STEVT_Term() for EVTDevNamPcr....\n" ));
    ReturnError = STEVT_Term( EVTDevNamPcr, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Output running analysis */
    STCLKRV_Print(("**********************************************************\n"));
    STCLKRV_Print(("PASSED: %3d\n", Result.NumberPassed));
    STCLKRV_Print(("FAILED: %3d\n", Result.NumberFailed));
    STCLKRV_Print(("**********************************************************\n"));

    STCLKRV_Print(( "\nOverall test Result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

}


/****************************************************************************
Name         : STPTI_GetPacketArrivalTime

Description  : Stub routine to resolve PTI library reference
               Returns fail to STCLKRV_GetSTC/GetExtSTC functions

Parameters   :

Return Value : Error

See Also     : PTI
****************************************************************************/
#ifdef ST_5188
/*ST_ErrorCode_t STDEMUX_GetPacketArrivalTime(STDEMUX_Handle_t Hndl, STDEMUX_TimeStamp_t * ArrivalTime_p,
                                          U16 *ArrivalTimeExtension_p)
{
    STCLKRV_Print(("GPAT dummy function\n"));
    return (ST_NO_ERROR + 1);
}

ST_ErrorCode_t STDEMUX_GetCurrentPTITimer(ST_DeviceName_t DeviceName,
                                          STDEMUX_TimeStamp_t * TimeStamp)
{
    STCLKRV_Print(("GPAT dummy function\n"));
    return (ST_NO_ERROR + 1);
}*/
#else
ST_ErrorCode_t STPTI_GetPacketArrivalTime(STPTI_Handle_t Hndl, STPTI_TimeStamp_t * ArrivalTime_p,
                                          U16 *ArrivalTimeExtension_p)
{
    STCLKRV_Print(("GPAT dummy function\n"));
    return (ST_NO_ERROR + 1); /* return error */
}

ST_ErrorCode_t STPTI_GetCurrentPTITimer(ST_DeviceName_t DeviceName,
                                          STPTI_TimeStamp_t * TimeStamp)
{
    STCLKRV_Print(("GPAT dummy function\n"));
    return (ST_NO_ERROR + 1); /* return error */
}
#endif


/****************************************************************************
Name         : ErrantUse()

Description  : Performs API interface calls, generally mis-sequenced,
               or with illegal parameters.  This is intended to
               exercise the error checking built into the routines.
               Note that errors should be raised during this test,
               but these MUST be the expected ones.  If this is the
               case, an overall pass will be reported.

NOTE:          This version of the ErrantUse test is for both single instance
               PTI and the Multi instance, STPTI compatible version of the
               clock recovery driver.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors occurred
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_REVISION_NOT_SUPPORTED Version mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STCLKRV_ERROR_PCR_UNAVAILABLE   Valid PCR can't be returned
               STCLKRV_ERROR_CALLBACK_OPEN     RemoveEvent() not called


See Also     : STCL_ErrorCode_t
****************************************************************************/
void ErrantUse()
{

    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    STCLKRV_ClockSource_t SlaveClock;
    U32 SlaveFrequency = 27000000;

    STEVT_EventID_t CLKRVEventID;
    STCLKRV_SourceParams_t PCRSrc;

    STCLKRV_ExtendedSTC_t STCBase;
    U32 STC;
    STCLKRV_ExtendedSTC_t ExtendedSTC;

    Result.NumberPassed = Result.NumberFailed = 0;

    STCLKRV_Print(( "\n" ));
    STCLKRV_Print(( "==========================================================\n" ));


    STCLKRV_Print(( "Commencing ErrantUse Test Function for STPTI version..\n" ));

    STCLKRV_Print(( "==========================================================\n\n" ));

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("TEST: Testing STCLKRV_Init()...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));


    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_Init() tests: Bad Parameters\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));


    /* the following will be OK following NormalUse
       call, but are re-initialised anyway, in case
       ErrantUse is called first or in isolation...  */

    strcpy( EVTDevNam, "STEVT" );
#if defined(DVD_TRANSPORT_STPTI) || defined(DVD_TRANSPORT_STPTI4) || defined(DVD_TRANSPORT_DEMUX)
    strcpy( EVTDevNamPcr, "STEVT2" );
#endif
    strcpy( (char*)DeviceNam, "STCLKRV" );

    /* call Term without prior Init call... */
    CLKRVTermPars.ForceTerminate = TRUE;
    STCLKRV_Print(( "Calling STCLKRV_Term() forced w/o Init.... \n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    /* set up Init parameters correctly initially */
    strcpy( DeviceNam, "STCLKRV" );
    /* Initialize our native Clock Recovery Driver */
    CLKRVInitPars.PCRMaxGlitch      = STCLKRV_PCR_MAX_GLITCH;
    CLKRVInitPars.PCRDriftThres     = TEST_PCR_DRIFT_THRES;
    CLKRVInitPars.MinSampleThres    = STCLKRV_MIN_SAMPLE_THRESHOLD;
    CLKRVInitPars.MaxWindowSize     = STCLKRV_MAX_WINDOW_SIZE;
    CLKRVInitPars.Partition_p       = TEST_PARTITION_1;
    CLKRVInitPars.DeviceType        = CLK_REC_DEVICE_TYPE;
    CLKRVInitPars.FSBaseAddress_p   = (U32 *)CKG_BASE_ADDRESS;
#if defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
    CLKRVInitPars.AUDCFGBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#else
    CLKRVInitPars.ADSCBaseAddress_p = (U32 *)AUD_PCM_CLK_FS_BASE_ADDRESS;
#endif
#ifdef ST_7200
    CLKRVInitPars.CRUBaseAddress_p  = (U32*)(CLKRV_BASE_ADDRESS);
    CLKRVInitPars.DecodeType        = STCLKRV_DECODE_PRIMARY;
#endif
    CLKRVInitPars.InterruptNumber   = CLK_REC_TRACKING_INTERRUPT;
    CLKRVInitPars.InterruptLevel    = CLK_REC_TRACKING_INTERRUPT_LEVEL;

    strcpy( CLKRVInitPars.PCREvtHandlerName, EVTDevNamPcr );
    strcpy( CLKRVInitPars.EVTDeviceName, EVTDevNam );
    strcpy( CLKRVInitPars.PTIDeviceName, "PTI" );


    STCBase.BaseValue = 0;
    STCBase.BaseBit32 = 0;
    STCBase.Extension = 0;


    Passed = TRUE;
    /* deliberately spike Init calls with a
       single invalid parameter...          */
    STCLKRV_Print(( "Null Device Name.... \n" ));
    ReturnError = STCLKRV_Init( NULL, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STCLKRV_Print(( "Null String as Name....\n" ));
    ReturnError = STCLKRV_Init( "", &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STCLKRV_Print(( "Long Device Name.... \n" ));
    ReturnError = STCLKRV_Init( "TooLongDeviceName", &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STCLKRV_Print(( "Null Init Params....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, NULL );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );


    CLKRVInitPars.EVTDeviceName[0] = '\0';
    STCLKRV_Print(( "NULL EVT Device Name....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    strcpy( CLKRVInitPars.EVTDeviceName, "TooLongDeviceNam" );
    STCLKRV_Print(( "Long EVT Device Name....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    strcpy( CLKRVInitPars.EVTDeviceName, EVTDevNam );

#if defined(ST_OS20) || defined(ST_OS21) /* linux changes */
    CLKRVInitPars.Partition_p =  NULL;
    STCLKRV_Print(( "Null memory partition....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    CLKRVInitPars.Partition_p = TEST_PARTITION_2;
#endif

    CLKRVInitPars.MaxWindowSize = 0;
    STCLKRV_Print(( "MaxWindowSize at 0....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    CLKRVInitPars.MaxWindowSize = STCLKRV_MAX_WINDOW_SIZE;

    CLKRVInitPars.MinSampleThres = STCLKRV_MAX_WINDOW_SIZE+1;
    STCLKRV_Print(( "MinSampleThres too high....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    CLKRVInitPars.MinSampleThres = STCLKRV_MIN_SAMPLE_THRESHOLD;

    CLKRVInitPars.MaxWindowSize = 10000000;
    STCLKRV_Print(( "MaxWindowSize too high....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
#ifdef ST_OSLINUX
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
#else
    ErrorReport( &StoredError, ReturnError, ST_ERROR_NO_MEMORY );
#endif
    CLKRVInitPars.MaxWindowSize = STCLKRV_MAX_WINDOW_SIZE;

    PrintPassFail(Passed, "STCLKRV_Init acted as expected", "bad return code(s)");


    /* Init bad param section completed print the results*/
    /* STCLKRV_Init has not been called with success
       yet, so the following Open call OUGHT to fail */


    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_Open() tests: Bad Parameters...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));


    /* call Open with parameter errors...  */
    STCLKRV_Print(( "NULL Name....\n" ));
    ReturnError = STCLKRV_Open( NULL, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STCLKRV_Print(( "Empty String DeviceName....\n" ));
    ReturnError = STCLKRV_Open( "", &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    /* long name checking*/
    STCLKRV_Print(( "Long Device Name....\n" ));
    ReturnError = STCLKRV_Open( "TooLongDeviceName", &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    STCLKRV_Print(( "NULL Handle....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, NULL );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STCLKRV_Print(( "Different Name....\n" ));
    ReturnError = STCLKRV_Open( "Different", &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    STCLKRV_Print(( "W/O valid Init....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    PrintPassFail(Passed, "STCLKRV_Open acted as expected", "bad return code(s)");


    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_Init() tests: W/O EVT PTI Init...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    STCLKRV_Print(( "Calling STCLKRV_Init(), W/O STEVT_Init.... \n" ));
    CLKRVInitPars.Partition_p = TEST_PARTITION_1;
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, STCLKRV_ERROR_HANDLER_INSTALL );

    /* Initialize the sub-ordinate EVT Driver */
    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 3;
    EVTInitPars.SubscrMaxNum  = 9;
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STCLKRV_Print(( "Calling STEVT_Init()\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_Init(), W/O STEVT_Init.... \n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, STCLKRV_ERROR_HANDLER_INSTALL );

    /* Call STEVT_Open() so that we can subscribe to events */
    STCLKRV_Print(( "Calling STEVT_Open()....\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Subscribe to PCR Events now so that we can get
       callbacks when events are registered */
    STCLKRV_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_VALID_EVT)..\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent(EVTHandle,
                                             DeviceNam,
                                             (U32)STCLKRV_PCR_VALID_EVT,
                                             &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT).....\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_DISCONTINUITY_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STEVT_SubscribeDeviceEvent(STCLKRV_PCR_GLITCH_EVT).\n" ));
    ReturnError = STEVT_SubscribeDeviceEvent( EVTHandle,
                                              DeviceNam,
                                              (U32)STCLKRV_PCR_GLITCH_EVT,
                                              &EVTSubscrParams );

    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_Init(), W/O PCR Event installed\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, STCLKRV_ERROR_HANDLER_INSTALL );


    /* Setup Event registration for notification of PCR_RECEIVED
       event and callback to ActionPCR */
    EVTInitPars.EventMaxNum   = 3;
    EVTInitPars.ConnectMaxNum = 2;
    EVTInitPars.SubscrMaxNum  = 9;
    EVTInitPars.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STCLKRV_Print(( "Calling STEVT_Init()....\n" ));
    ReturnError = STEVT_Init( EVTDevNamPcr, &EVTInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */

    STCLKRV_Print(( "Calling STEVT_Open()....\n" ));
    ReturnError = STEVT_Open( EVTDevNamPcr, &EVTOpenPars, &EVTHandlePcr );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(("Calling STEVT_Register(PCR_RECIEVED_EVT)....\n"));
#ifdef ST_5188
    ReturnError = STEVT_Register(EVTHandlePcr, (U32)STDEMUX_EVENT_PCR_RECEIVED_EVT, &CLKRVEventID);
#else
    ReturnError = STEVT_Register(EVTHandlePcr, (U32)STPTI_EVENT_PCR_RECEIVED_EVT, &CLKRVEventID);
#endif
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    strcpy( CLKRVInitPars.PCREvtHandlerName, "\0" );
    STCLKRV_Print(( "W/O PCR callback event handler name....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, STCLKRV_ERROR_HANDLER_INSTALL );

    strcpy( CLKRVInitPars.PCREvtHandlerName, EVTDevNamPcr );


    /* Successful call to STCLKRV_Init() */
    STCLKRV_Print(( "Clean STCLKRV_Init....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Again Successful call to STCLKRV_Init() */
    STCLKRV_Print(( "Again Clean STCLKRV_Init....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &CLKRVInitPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_ALREADY_INITIALIZED );

    /* clean Open */
    STCLKRV_Print(( "Calling STCLKRV_Open(), no errors....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PrintPassFail(Passed, "STCLKRV_Init as expected W/O dependent drivers", "Unknown Device(s)");

    /*STCLKRV_SetNominalFreq*/
    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_SetNominalFreq() tests:...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    SlaveClock = STCLKRV_CLOCK_PCM_0;
    SlaveFrequency = 27000000;

    STCLKRV_Print(( "w/o handle....\n"));
    ReturnError = STCLKRV_SetNominalFreq((STCLKRV_Handle_t)NULL, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* BAD handle */
    STCLKRV_Print(( "bad handle....\n"));
    ReturnError = STCLKRV_SetNominalFreq((STCLKRV_Handle_t)0x12345678, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    SlaveClock = STCLKRV_CLOCK_SD_0;
    /* BAD Slave Clock as SD/STC */
    STCLKRV_Print(( "bad Slave Clock  as SD/STC ....\n"));
    ReturnError = STCLKRV_SetNominalFreq(Handle, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, STCLKRV_ERROR_INVALID_SLAVE);

    SlaveClock = (STCLKRV_ClockSource_t)((U32)STCLKRV_CLOCK_PCM_0 + 7);
    /* BAD Slave Clock */
    STCLKRV_Print(( "bad Slave Clock ....\n"));
    ReturnError = STCLKRV_SetNominalFreq(Handle, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, STCLKRV_ERROR_INVALID_SLAVE);

    SlaveClock = STCLKRV_CLOCK_PCM_0;

    SlaveFrequency = 0x123456;
    /* BAD Slave Clock frequency*/
    STCLKRV_Print(( "bad Slave Clock frequency....\n"));
    ReturnError = STCLKRV_SetNominalFreq(Handle, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, STCLKRV_ERROR_INVALID_FREQUENCY);

    SlaveFrequency = 22579200;

    /* first Good Slave Clock & Frequency */
    STCLKRV_Print(( "Correct Slave Clock & frequency....\n"));
    ReturnError = STCLKRV_SetNominalFreq(Handle, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    SlaveClock = STCLKRV_CLOCK_PCM_0;
    SlaveFrequency = 74250000;

    /* Second Good Slave Clock & Frequency */
    STCLKRV_Print(( "Correct Slave Clock & frequency....\n"));
    ReturnError = STCLKRV_SetNominalFreq(Handle, SlaveClock, SlaveFrequency);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    /*STCLKRV_SetPCRSource*/
    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_SetPCRSource() tests:...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;

    /* NULL handle */
    STCLKRV_Print(( "w/o handle....\n"));
    ReturnError = STCLKRV_SetPCRSource((STCLKRV_Handle_t)NULL, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* BAD handle */
    STCLKRV_Print(( "bad handle....\n"));
    ReturnError = STCLKRV_SetPCRSource((STCLKRV_Handle_t)0x12345678, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* NULL structure */
    STCLKRV_Print(( "NULL param....\n"));
#ifdef ST_5188
    PCRSrc.Source_u.STPTI_s.Slot = (STDEMUX_Slot_t) NULL;;
#else
    PCRSrc.Source_u.STPTI_s.Slot = (STPTI_Slot_t) NULL;;
#endif
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_BAD_PARAMETER);

    /* Good slot allocate */
    STCLKRV_Print(( "good params....\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    PrintPassFail(Passed, "STCLKRV_SetPCRSource as expected", "Unknown Device(s)");

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_SetSTCSource() tests:...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    /* STCLKRV_SetSTCSource */

    /* NULL Handle  */
    STCLKRV_Print(("NULL handle....\n"));
    ReturnError = STCLKRV_SetSTCSource((STCLKRV_Handle_t)NULL, STCLKRV_STC_SOURCE_PCR);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

   /* Bad Handle  */
    STCLKRV_Print(("Bad handle....\n"));
    ReturnError = STCLKRV_SetSTCSource(0x12345678, STCLKRV_STC_SOURCE_PCR);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

   /* Invalid source value   */
    STCLKRV_Print(("Invalid source....\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, (STCLKRV_STCSource_t)0x12345678);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_BAD_PARAMETER);

    /* Good params */
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_SetSTCSource(Handle, STCLKRV_STC_SOURCE_PCR);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    PrintPassFail(Passed, "STCLKRV_SetSTCSource as expected", "Unknown Device(s)");

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_SetSTCBaseline() tests:...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    /* STCLkRV_SetSTCBaseline */
    /* NB: no error checking in STC baseline parameter */

    /* No handle */
    STCLKRV_Print(("NULL Handle....\n"));
    ReturnError = STCLKRV_SetSTCBaseline((STCLKRV_Handle_t)NULL, &STCBase);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Wrong handle */
    STCLKRV_Print(("Bad Handle....\n"));
    ReturnError = STCLKRV_SetSTCBaseline(0x12345678, &STCBase);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Good handle */
#ifndef ST_OSLINUX
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_SetSTCBaseline(Handle, &STCBase);
#if defined (ST_7710) || defined (ST_5105) || defined (ST_7100) || defined (ST_7109) \
 || defined (ST_5301) || defined (ST_5107) || defined (ST_5188) || defined (ST_7200)
    ErrorReport(&StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE);
#else
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);
#endif
#endif

    PrintPassFail(Passed, "STCLKRV_SetSTCBaseline as expected", "Unknown Device(s)");


    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_GetSTC \\ ExtendedSTC() tests:...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    /* STCLKRV_GetSTC */

    STC = 0x12345678;
    /* NULL Handle  */
    STCLKRV_Print(("NULL handle....\n"));
    ReturnError = STCLKRV_GetSTC((STCLKRV_Handle_t)NULL, &STC);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

   /* Bad Handle  */
    STCLKRV_Print(("Bad handle....\n"));
    ReturnError = STCLKRV_GetSTC(0x12345678, &STC);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

   /* Invalid source value   */
    STCLKRV_Print(("Invalid source....\n"));
    ReturnError = STCLKRV_GetSTC(Handle, (U32 *)NULL);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_BAD_PARAMETER);

    /* Good params */
    STC = 0x12345678;
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_GetSTC(Handle, &STC);
    ErrorReport(&StoredError, ReturnError, STCLKRV_ERROR_PCR_UNAVAILABLE);



    /* STCLKRV_GetExtendedSTC */

    STCLKRV_Print(("NULL handle....\n"));
    ReturnError = STCLKRV_GetExtendedSTC((STCLKRV_Handle_t)NULL, &ExtendedSTC);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

   /* Bad Handle  */
    STCLKRV_Print(("Bad handle....\n"));
    ReturnError = STCLKRV_GetExtendedSTC(0x12345678, &ExtendedSTC);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

   /* Invalid source value   */
    STCLKRV_Print(("Invalid source....\n"));
    ReturnError = STCLKRV_GetExtendedSTC(Handle, (STCLKRV_ExtendedSTC_t *)NULL);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_BAD_PARAMETER);

    /* Good params */
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_GetExtendedSTC(Handle, &ExtendedSTC);
    ErrorReport(&StoredError, ReturnError, STCLKRV_ERROR_PCR_UNAVAILABLE);



    PrintPassFail(Passed, "STCLKRV_GetSTC \\ ExtendedSTC as expected", "Unknown Device(s)");

    STCLKRV_Print(("----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_InvDecodeClk() tests:...\n"));
    STCLKRV_Print(("----------------------------------------------------------\n\n"));

    /*STCLKRV_InvDecodeClk*/

    /* No handle */
    STCLKRV_Print(("NULL Handle....\n"));
    ReturnError = STCLKRV_InvDecodeClk((STCLKRV_Handle_t)NULL);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Wrong handle */
    STCLKRV_Print(("Bad Handle....\n"));
    ReturnError = STCLKRV_InvDecodeClk(0x12345678);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Good handle */
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_InvDecodeClk(Handle);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    PrintPassFail(Passed, "STCLKRV_InvDecodeClk as expected", "Unknown Device(s)");

    STCLKRV_Print(("-----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_Disable \\ Enable() tests:...\n"));
    STCLKRV_Print(("-----------------------------------------------------------\n\n"));

    /* STCLKRV_Disable */

    /* No handle */
    STCLKRV_Print(("NULL Handle....\n"));
    ReturnError = STCLKRV_Disable((STCLKRV_Handle_t)NULL);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Wrong handle */
    STCLKRV_Print(("Bad Handle....\n"));
    ReturnError = STCLKRV_Disable(0x12345678);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Good handle */
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_Disable(Handle);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    /* STCLKRV_Enable */

    /* No handle */
    STCLKRV_Print(("NULL Handle....\n"));
    ReturnError = STCLKRV_Enable((STCLKRV_Handle_t)NULL);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Wrong handle */
    STCLKRV_Print(("Bad Handle....\n"));
    ReturnError = STCLKRV_Enable(0x12345678);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Good handle */
    STCLKRV_Print(("No errors....\n"));
    ReturnError = STCLKRV_Enable(Handle);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    PrintPassFail(Passed, "Enable \\ Disable invlidate as expected", "Unknown Device(s)");

    STCLKRV_Print(("-----------------------------------------------------------\n"));
    STCLKRV_Print(("STCLKRV_Close \\ Term() tests:...\n"));
    STCLKRV_Print(("-----------------------------------------------------------\n\n"));

    /* STCLKRV_Close */
    /* No handle */
    STCLKRV_Print(("NULL Handle....\n"));
    ReturnError = STCLKRV_Close((STCLKRV_Handle_t)NULL);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* Wrong handle */
    STCLKRV_Print(("Bad Handle....\n"));
    ReturnError = STCLKRV_Close(0x12345678);
    ErrorReport(&StoredError, ReturnError, ST_ERROR_INVALID_HANDLE);

    /* STCLKRV_Term */

    STCLKRV_Print(( "Calling STCLKRV_Term() with NULL Device Name....\n" ));
    ReturnError = STCLKRV_Term( NULL , &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    STCLKRV_Print(( "Calling STCLKRV_Term() with NULL params .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, NULL );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    /* STCLKRV_Open */

    /* try a second Open - simulating 2nd thread */
    STCLKRV_Print(( "Calling STCLKRV_Open(), already Open (1)....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle2 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* try a 3rd Open - simulating 3rd thread */
    STCLKRV_Print(( "Calling STCLKRV_Open(), already Open (2)....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle3 );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Now try closing 2 of the previous 3 opens, so still 1 open */
    STCLKRV_Print(( "Calling STCLKRV_Close() (leaving 2 open)....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_Close() (leaving 1 open)....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* call Term with Open Handle */
    CLKRVTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STCLKRV_Term() with open handle....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_OPEN_HANDLE );

    /* Now try to close twice more - 2nd close will return error */
    STCLKRV_Print(( "Calling STCLKRV_Close() (leaving none open)....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STCLKRV_Print(( "Calling STCLKRV_Close(), with none open....\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );

    /* Open again (so we can try forced terminate) */
    STCLKRV_Print(( "Calling STCLKRV_Open(), no errors....\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &CLKRVOpenPars, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* finally a forced Term call to clean up */
    CLKRVTermPars.ForceTerminate = TRUE;
    STCLKRV_Print(( "Calling STCLKRV_Term(), forced....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &CLKRVTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Close and terminate the subordinate EVT Driver */
    STCLKRV_Print(( "Calling STEVT_Close()....\n" ));
    ReturnError = STEVT_Close( EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    EVTTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STEVT_Term()....\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    /* Close and terminate the subordinate PTI EVT Driver */
    STCLKRV_Print(( "Calling STEVT_Close()....\n" ));
    ReturnError = STEVT_Close( EVTHandlePcr );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    EVTTermPars.ForceTerminate = FALSE;
    STCLKRV_Print(( "Calling STEVT_Term()....\n" ));
    ReturnError = STEVT_Term( EVTDevNamPcr, &EVTTermPars );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PrintPassFail(Passed, "close\\term as expected", "Unknown Device(s)");

    STCLKRV_Print(( "\nOverall test Result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Output running analysis */
    STCLKRV_Print(("**********************************************************\n"));
    STCLKRV_Print(("PASSED: %3d\n", Result.NumberPassed));
    STCLKRV_Print(("FAILED: %3d\n", Result.NumberFailed));
    STCLKRV_Print(("**********************************************************\n"));
}
#endif

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

    ST_ErrorCode_t  Temp = ErrorGiven;
    U32             i;

    for( i = 0; i < 2; i++ )
    {

        switch( Temp )
        {
            case ST_NO_ERROR:
                STCLKRV_Print(( "ST_NO_ERROR - Successful Return\n" ));
                break;

            case ST_ERROR_ALREADY_INITIALIZED:
                STCLKRV_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
                break;

            case ST_ERROR_UNKNOWN_DEVICE:
                STCLKRV_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
                break;

            case ST_ERROR_INVALID_HANDLE:
                STCLKRV_Print(( "ST_ERROR_INVALID_HANDLE - Handle number not recognized\n" ));
                break;

            case ST_ERROR_OPEN_HANDLE:
                STCLKRV_Print(( "ST_ERROR_OPEN_HANDLE - unforced Term with handle open\n" ));
                break;

            case ST_ERROR_BAD_PARAMETER:
                STCLKRV_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
                break;

            case ST_ERROR_FEATURE_NOT_SUPPORTED:
                STCLKRV_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
                break;

            case ST_ERROR_NO_MEMORY:
                STCLKRV_Print(( "ST_ERROR_NO_MEMORY - Not enough space for dynamic memory allocation\n" ));
                break;

            case ST_ERROR_NO_FREE_HANDLES:
                STCLKRV_Print(( "ST_ERROR_NO_FREE_HANDLES - Channel already Open\n" ));
                break;

            case STCLKRV_ERROR_HANDLER_INSTALL:
                STCLKRV_Print(( "STCLKRV_ERROR_HANDLER_INSTALL - Action_PCR not (de)installed\n" ));
                break;

            case STCLKRV_ERROR_PCR_UNAVAILABLE:
                STCLKRV_Print(( "STCLKRV_ERROR_PCR_UNAVAILABLE - PCR can't be returned\n" ));
                break;

            case STCLKRV_ERROR_EVT_REGISTER:
                STCLKRV_Print(( "STCLKRV_ERROR_EVT_REGISTER - Error occured in sub component EVT\n" ));
                break;

            case STCLKRV_ERROR_INVALID_FREQUENCY:
                STCLKRV_Print(( "STCLKRV_ERROR_INVALID_FREQUENCY - Bad Slave Frequency\n" ));
                break;

            case STCLKRV_ERROR_INVALID_SLAVE:
                STCLKRV_Print(( "STCLKRV_ERROR_INVALID_SLAVE - Invalid Slave\n" ));
                break;


            default:
                STCLKRV_Print(( "*** Unrecognised return code ***\n" ));
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
                STCLKRV_Print(( ">>>>> ERROR!!\nMismatch, expected code was:\n" ));
            }
        }
    }
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

    STCLKRV_Print(( "----------------------------------------------------------\n" ));

}

/****************************************************************************
Name         : CheckEventGenerated()

Description  : This routine has system knowledge and
               checks that correct events are generated
               as the two sets of test data are run.

Parameters   : TestSet             0 = CountTable
               TestIteration       offset into TestSet.

Return Value : ST_NO_ERROR         Expected event occurred
               STCLKRV_ERROR_EVT   Event missing or wrong event rxd
 ****************************************************************************/
#ifndef STCLKRV_NO_PTI
static ST_ErrorCode_t CheckEventGenerated( U32 TestSet, U32 TestIteration )
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

    if(TestSet == 0)
    {
        /* Using CountTable */

        switch( TestIteration )
        {
          case 1:
          case 4:
          case 11:
          case 19:
            if( NewEventRxd == FALSE )
            {
                STCLKRV_Print(( "ERROR: STCLKRV_PCR_VALID_EVT event missing\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
            else if( EventReceived != (U32)STCLKRV_PCR_VALID_EVT )
            {
                STCLKRV_Print(( "ERROR: Wrong event received when expecting STCLKRV_PCR_VALID_EVT\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
            break;

          case 3:
            if( NewEventRxd == FALSE )
            {
                STCLKRV_Print(( "ERROR: STCLKRV_PCR_DISCONTINUITY_EVT event missing\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
            else if( EventReceived != (U32)STCLKRV_PCR_DISCONTINUITY_EVT )
            {
                STCLKRV_Print(( "ERROR: Wrong event received when expecting STCLKRV_PCR_DISCONTINUITY_EVT\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
            break;

          case 10:
          case 18:
            if( NewEventRxd == FALSE )
            {
                STCLKRV_Print(( "ERROR: STCLKRV_PCR_GLITCH_EVT event missing\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
            else if( EventReceived != (U32)STCLKRV_PCR_GLITCH_EVT )
            {
                STCLKRV_Print(( "ERROR: Wrong event received when expecting STCLKRV_PCR_GLITCH_EVT\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
            break;

          default:
            if( NewEventRxd == TRUE )
            {
                STCLKRV_Print(( "ERROR: Unexpected event received\n" ));
                ErrorCode = STCLKRV_ERROR_EVT_REGISTER;
            }
        }
    }
    else
    {
    }

    return( ErrorCode );
}
#endif /* STCLKRV_NO_PTI */

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
    /* Only one instance, so should be true alays... */
    if (strcmp(RegistrantName,DeviceNam) == 0)
    {
        switch ( Event )
        {
        case STCLKRV_PCR_VALID_EVT:
            STCLKRV_Print( ("STCLKRV_PCR_VALID_EVT event received\n") );
            break;

        case STCLKRV_PCR_DISCONTINUITY_EVT:
            STCLKRV_Print( ("STCLKRV_PCR_DISCONTINUITY_EVT event received\n") );
            break;

        case STCLKRV_PCR_GLITCH_EVT:
            STCLKRV_Print( ("STCLKRV_PCR_GLITCH_EVT event received\n") );
            break;

        default:
            STCLKRV_Print( ("ERROR: Unknown STCLKRV event received\n") );
            break;
        }

        NewEventRxd = TRUE;
        EventReceived = Event;
    }
    else
    {
        STCLKRV_Print( ("ERROR: Unknown device event received\n") );
    }
}

/****************************************************************************
Name         : PrintPassFail()

Description  : Print "PASSED: <text>" or "FAILED: <text>" depending on the
               value of 'passed'. Also increments the appropriate counter
               in 'Result' accordingly.

Parameters:
               Passed      - whether the test(s) passed or failed
               PassMsg     - message to print when passed
               FailMsg     - message to print when failed

Return Value : NONE

See Also     :
 ****************************************************************************/

void PrintPassFail(BOOL IsPassed, char *PassMsg, char *FailMsg)
{
    STCLKRV_Print(("\n==========================================================\n"));
    if (IsPassed == TRUE)
    {
        STCLKRV_Print(("PASSED: %s\n", PassMsg));
        Result.NumberPassed++;
    }
    else
    {
        STCLKRV_Print(("FAILED: %s\n", FailMsg));
        Result.NumberFailed++;
    }

    /* Always display number passed */
    STCLKRV_Print(( "PASSED: (%d)\n",Result.NumberPassed));

    /* Only display failed if failures occur */
    if (Result.NumberFailed !=0)
    {
        STCLKRV_Print(( "FAILED: (%d)\n",Result.NumberFailed));
    }
    STCLKRV_Print(("==========================================================\n\n"));

}

/* End of stclkrvt.c module */


