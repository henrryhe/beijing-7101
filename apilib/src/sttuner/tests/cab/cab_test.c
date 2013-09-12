/*****************************************************************************
File Name   : cab_test.c

Description : Test harness for the STTUNER driver for STi55xx.

Copyright (C) 2000 STMicroelectronics

Revision History    :

    19/Apr/2000   Added support for STi5508.
    08/Aug/2001   Modifications to support STTUNER 3.1.0 (new multi-instance version)
    25/Apr/2002   Modifications to support STTUNER 4.1.0 (Update from sat_test)
    06/May/2004   Support for running on 5100.
    07/Oct/2004   Support for OS21 and for running on 7710.
    18/Nov/2004   Support for MT2060 tuner.
    07/Jan/2005   Support for running on 5105 and Espresso.
    26/Apr/2005   Support for running on 5301 and 7100 platforms

Reference   :

ST API Definition "TUNER Driver API" DVD-API-06
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>              /* C libs */
#include <stdlib.h>
#include <string.h>

#ifndef ST_OSLINUX
#include "stlite.h"             /* Standards includes */
#include "stboot.h"
#endif /*ST_OSLINUX*/
#include "stdevice.h"

#ifdef ST_OSLINUX
#include "stos.h"
#include <pthread.h>
#endif /*ST_OSLINUX*/


#include "sttuner.h"            /* STAPI includes */
#include "sti2c.h"
#include "stboot.h"

#ifndef ST_OSLINUX
#ifndef DISABLE_TOOLBOX
#include "sttbx.h"
#ifndef STTBX_NO_UART
#include "stuart.h"
#endif
#endif
#include "stsys.h"
#endif /*ST_OSLINUX*/

#include "stcommon.h"
#include "stevt.h"

#ifdef ST_OS21
#include "os21debug.h"
#endif
#ifdef ST_OS20
#include "debug.h" /*it may needed to be changed for os21*/
#endif 
#include "../tuner_test.h"                 /* Local includes */

/* Private types/constants ------------------------------------------------ */

/* hardware in system */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
#if defined(ST_5518)
#define NUM_PIO     6
#elif defined(ST_5105)|| defined(ST_5107)
#define NUM_PIO     4
#else
#define NUM_PIO     5
#endif
#endif/*#ifndef USE_STI2C_WRAPPER*/

#ifdef QAMi5107
#define NUM_I2C     1
#else
#define NUM_I2C     2
#endif

#ifdef ST_OSLINUX
#define debugmessage(args) 	printf(args)
#endif

#if defined(ST_5514)
#define ST_I2C_CLOCKFREQUENCY   58000000
#elif defined(ST_5516)
#define ST_I2C_CLOCKFREQUENCY   58000000
#elif defined(ST_5517)
#define ST_I2C_CLOCKFREQUENCY   56000000
#elif defined(ST_5518)
#define ST_I2C_CLOCKFREQUENCY   56000000
#elif defined(ST_5528)
	#if defined(ST_OS21)
	#define ST_I2C_CLOCKFREQUENCY   43000000
	#elif defined(ST_OS20)
	#define ST_I2C_CLOCKFREQUENCY   116000000
	#endif
#elif defined(espresso)
	#if defined(ST_OS21)
	#define ST_I2C_CLOCKFREQUENCY   43000000
	#elif defined(ST_OS20)
	#define ST_I2C_CLOCKFREQUENCY   106000000
	#endif
#elif defined(ST_7710)
#define ST_I2C_CLOCKFREQUENCY   90000000
#elif defined(ST_7100)
#define ST_I2C_CLOCKFREQUENCY   58000000
#elif defined(ST_7109)
#define ST_I2C_CLOCKFREQUENCY   58000000
#elif defined(ST_5100)
#define ST_I2C_CLOCKFREQUENCY   88000000
#elif defined(ST_5301)
#define ST_I2C_CLOCKFREQUENCY   90000000
#elif defined(ST_8010)
#define ST_I2C_CLOCKFREQUENCY   117000000
#elif defined(ST_5105)|| defined(ST_5107)
#define ST_I2C_CLOCKFREQUENCY   85000000
#endif

/* parameters for tuner ioctl call 'STTUNER_IOCTL_PROBE' */
typedef struct TUNER_ProbeParams_s
{
    STTUNER_IODriver_t Driver;
    ST_DeviceName_t    DriverName;
    U32                Address;
    U32                SubAddress;
    U32                Value;
    U32                XferSize;
    U32                TimeOut;
}
TUNER_ProbeParams_t;

/* Enum of PIO devices */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    #if defined(ST_5518)
    PIO_DEVICE_5,
    #endif
    PIO_DEVICE_NOT_USED
};
#define PIO_BIT_NOT_USED    0
#endif/*#ifndef USE_STI2C_WRAPPER*/



/* Enum of I2C devices */
enum
{
    I2C_DEVICE_0,
    I2C_DEVICE_1,
    I2C_DEVICE_NOT_USED
};

#ifdef TUNER_370QAM
#define START_I2C_ADDR      0x18
#define END_I2C_ADDR        0x1E
#else
#define START_I2C_ADDR      0x38
#define END_I2C_ADDR        0x3E

#endif

#ifndef ST_OSLINUX
#ifdef ST_5100
#define PIO_4_BASE_ADDRESS	ST5100_PIO4_BASE_ADDRESS
#define PIO_4_INTERRUPT		ST5100_PIO4_INTERRUPT


#define PIO_5_BASE_ADDRESS	ST5100_PIO5_BASE_ADDRESS
#define PIO_5_INTERRUPT		ST5100_PIO5_INTERRUPT
#endif
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105)|| defined(ST_5107) || defined(ST_7100) || defined(ST_7109) || defined(ST_5301)
 #define BACK_I2C_BASE  SSC_0_BASE_ADDRESS
 #define FRONT_I2C_BASE SSC_1_BASE_ADDRESS
#endif


/* I2C device map */
#if defined(ST_5508) || defined(ST_5518)
 #define I2C_DEVICE_BACKBUS  I2C_DEVICE_0
 #define I2C_DEVICE_FRONTBUS I2C_DEVICE_1
#else
 #define I2C_DEVICE_BACKBUS  I2C_DEVICE_0
 #define I2C_DEVICE_FRONTBUS I2C_DEVICE_1
#endif

/* PIO pins used for I2C */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER

#if defined(ST_5508) || defined(ST_5518)
 #define I2C_0_CLK_DEV       PIO_DEVICE_1
 #define I2C_0_CLK_BIT       PIO_BIT_1
 #define I2C_0_DATA_DEV      PIO_DEVICE_1
 #define I2C_0_DATA_BIT      PIO_BIT_0
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL

 #define I2C_1_CLK_DEV       PIO_DEVICE_5
 #define I2C_1_CLK_BIT       PIO_BIT_1
 #define I2C_1_DATA_DEV      PIO_DEVICE_5
 #define I2C_1_DATA_BIT      PIO_BIT_0
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_1_INTERRUPT_LEVEL

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105)|| defined(ST_5107) || defined(ST_5301)
 #define I2C_0_CLK_DEV       PIO_DEVICE_3
 #define I2C_0_DATA_DEV      PIO_DEVICE_3
 #define I2C_0_CLK_BIT       PIO_BIT_1
 #define I2C_0_DATA_BIT      PIO_BIT_0
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL

 #define I2C_1_CLK_DEV       PIO_DEVICE_3
 #define I2C_1_DATA_DEV      PIO_DEVICE_3
 #define I2C_1_CLK_BIT       PIO_BIT_3
 #define I2C_1_DATA_BIT      PIO_BIT_2
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_1_INTERRUPT_LEVEL

#elif defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
 #define I2C_0_CLK_DEV       PIO_DEVICE_2
 #define I2C_0_DATA_DEV      PIO_DEVICE_2
 #define I2C_0_CLK_BIT       PIO_BIT_0
 #define I2C_0_DATA_BIT      PIO_BIT_1
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL

 #define I2C_1_CLK_DEV       PIO_DEVICE_3
 #define I2C_1_DATA_DEV      PIO_DEVICE_3
 #define I2C_1_CLK_BIT       PIO_BIT_0
 #define I2C_1_DATA_BIT      PIO_BIT_1
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_1_INTERRUPT_LEVEL

#else
 #define I2C_0_CLK_DEV       PIO_DEVICE_1
 #define I2C_0_DATA_DEV      PIO_DEVICE_1
 #define I2C_0_CLK_BIT       PIO_BIT_2
 #define I2C_0_DATA_BIT      PIO_BIT_0
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL

 #define I2C_1_CLK_DEV       PIO_DEVICE_3
 #define I2C_1_DATA_DEV      PIO_DEVICE_3
 #define I2C_1_CLK_BIT       PIO_BIT_2
 #define I2C_1_DATA_BIT      PIO_BIT_0
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_1_INTERRUPT_LEVEL
#endif
/*#endif */ /*ST_OSLINUX*/
#endif /*USE_STI2C_WRAPPER*/
/* I2C used by TUNER */
#if defined(ST_5508) || defined(ST_5518)
/* mb5518 board */
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

#elif defined(espresso) || defined(ST_5100)|| defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

#elif defined(QAMi5107)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS
 
/* presume 5514 (mediaref) tuners are on a common bus */
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5105)|| defined(ST_5107)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_FRONTBUS

/* 5512 etc. */
#else
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

#endif

#ifdef QAMi5107
#define PIO1_PC0  (0x20821000 + 0x24)
#define PIO1_PC1  (0x20821000 + 0x34)
#define PIO1_PC2  (0x20821000 + 0x44)
#endif

/* Defines a single channel setting */
typedef struct
{
    U32 F;
    STTUNER_Scan_t ScanInfo;
} TUNER_Channel_t;

#ifndef TUNER_TABLE_SIZE
 #define TUNER_TABLE_SIZE 120
#endif

#ifdef TUNER_370QAM
#ifndef FCE_START
 #define FCE_START  213000000 
#endif
#ifndef FCE_END
 #define FCE_END    216000000 
#endif

#ifndef FCE_STEP
 #define FCE_STEP     1000000 
#endif
#endif


#ifndef TUNER_370QAM
#ifndef FCE_START
 #define FCE_START  320000000
#endif
#ifndef FCE_END
 #define FCE_END    600000000
#endif
#ifndef FCE_STEP
 #define FCE_STEP     7000000
#endif
#endif



#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif

static char Platform[20];
static char Backend[20];
static char DemodType[20];
static char TunerType[20];

/* Private variables ------------------------------------------------------ */

static STTUNER_DemodType_t TestDemodType;
static STTUNER_TunerType_t TestTunerType;

#ifndef ST_OSLINUX
#if defined(ST_5105)|| defined(ST_5107)
U32 ReadValue=0xFFFF;
#define INTERCONNECT_BASE                      (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#endif
#endif/*ST_OSLINUX*/
/* Test harness revision number */
static U8 Revision[] = "5.0.2";

/* Tuner device name -- used for all tests */
static ST_DeviceName_t TunerDeviceName[NUM_TUNER+1];

/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
/* List of PIO device names */
static ST_DeviceName_t PioDeviceName[NUM_PIO+1];
/*#endif */ /*ST_OSLINUX*/
#endif /*#ifndef USE_STI2C_WRAPPER*/
/* List of I2C device names */
static ST_DeviceName_t I2CDeviceName[NUM_I2C+1];

#ifndef ST_OSLINUX
#ifndef USE_STI2C_WRAPPER
/* PIO Init parameters */
static STPIO_InitParams_t PioInitParams[NUM_PIO];
#endif   /*ST_OSLINUX*/
#endif  /* #ifndef USE_STI2C_WRAPPER */
/* I2C Init params */
static STI2C_InitParams_t I2CInitParams[NUM_I2C];

/* TUNER Init parameters */
static STTUNER_InitParams_t TunerInitParams[NUM_TUNER];

/* Global semaphore used in callbacks */

static semaphore_t *EventSemaphore1, *EventSemaphore2;

static STTUNER_EventType_t LastEvent1, LastEvent2;

/* Stored table of channels */
static TUNER_Channel_t ChannelTable[TUNER_TABLE_SIZE];
static U32 ChannelTableCount;

#ifndef ST_OSLINUX
/* Declarations for memory partitions */
#ifndef ST_OS21
/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif

#define                 NCACHE_MEMORY_SIZE          0x80000 - 0x10 /* keep room for os20_lock_fix section */
static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( ncache_memory , "ncache_section" )
#endif

/* 2Mb */
#define                 SYSTEM_MEMORY_SIZE          0x200000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( external_block, "system_section")
#endif


#else /* ST_OS21*/
/* 2Mb */
#define                 SYSTEM_MEMORY_SIZE          0x200000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;
partition_t             *driver_partition;
#endif /*ST_OS21 */
#endif /*ST_OSLINUX*/

#ifdef ST_OSLINUX
#define system_partition NULL;
#endif

static STTUNER_Band_t BandList[10];
static STTUNER_Scan_t ScanList[10];
static STTUNER_SignalThreshold_t ThresholdList[10];

static STTUNER_BandList_t Bands;
static STTUNER_ScanList_t Scans;
static STTUNER_ThresholdList_t Thresholds;


static const ST_DeviceName_t EVT_RegistrantName1 = "stevt_1";
static const ST_DeviceName_t EVT_RegistrantName2 = "stevt_2";


/* Private function prototypes -------------------------------------------- */

char *TUNER_FecToString(STTUNER_FECRate_t Fec);
char *TUNER_DemodToString(STTUNER_Modulation_t Mod);
char *TUNER_PolarityToString(STTUNER_Polarization_t Plr);
char *TUNER_EventToString(STTUNER_EventType_t Evt);
char *TUNER_AGCToPowerOutput(S32 Agc);
char *TUNER_ToneToString(STTUNER_LNBToneState_t ToneState);

static char *STAPI_ErrorMessages(ST_ErrorCode_t ErrCode);
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_boot(void);
#ifndef DISABLE_TOOLBOX
static ST_ErrorCode_t Init_sttbx(void);
#endif
#endif  /* ST_OSLINUX */
#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_pio(void);
#endif  /* ST_OSLINUX */
#endif /**/
static ST_ErrorCode_t Init_i2c(void);
static ST_ErrorCode_t Init_tunerparams(void);
static ST_ErrorCode_t Init_scanparams(void);
static ST_ErrorCode_t Init_evt(void);
static ST_ErrorCode_t Open_evt(void);
static ST_ErrorCode_t GlobalTerm(void);


/* Section: Simple */
static TUNER_TestResult_t TunerReLock(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerStandByMode(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerScan(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTimedScan(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerScanExact(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTimedScanExact(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerSignalChange(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTracking(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p);
#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerAPI(TUNER_TestParams_t *Params_p);
#endif
static TUNER_TestResult_t TunerTerm(TUNER_TestParams_t *Params_p);


/* Test table */
static TUNER_TestEntry_t TestTable[] =
{
    { TunerIoctl,          "Ioctl API 3.2.0 test ",                       1 },
    { TunerReLock,         "Relock channel test ",                        1 },
    { TunerScan,           "Scan test to acquire channels in band",       1 },
    { TunerTimedScan,      "Speed test for a scan operation",             1 },
    { TunerScanExact,      "Scan exact through all acquired channels",    1 },
    { TunerTimedScanExact, "Speed test for locking to acquired channels", 1 },
    { TunerSignalChange,   "Signal change events",                        1 },
    { TunerTracking,       "Test tracking of a locked frequency",         1 },
    #ifndef ST_OSLINUX
    { TunerAPI,            "Test the stablility of the STTUNER API",      1 },
    #endif
    { TunerTerm,           "Termination during a scan",                   1 },
    {TunerStandByMode,     "StandBy test",                                1},
   
    { 0,                   "",                                            0 }
};


/* Function definitions --------------------------------------------------- */

int main(int argc, char *argv[])
{
    TUNER_TestResult_t GrandTotal, Result;
    TUNER_TestResult_t Total[NUM_TUNER];
    TUNER_TestEntry_t *Test_p;
    U32 Section;
    S32 tuners;
    TUNER_TestParams_t TestParams;
    #ifndef ST_OSLINUX  
    STBOOT_TermParams_t BootTerm;
    ST_ErrorCode_t error = ST_NO_ERROR;
    char str[100];
    #endif
    #ifndef DISABLE_TOOLBOX
    char temp1[50];/*max length of string describing LLA version=49+1(for '\0');total 5 demods,therefore 5*50=250*/
    char * LLARevision1=temp1;
    memset(LLARevision1,'\0',sizeof(temp1));
    #endif
#if defined(ST_5514)
    strcpy(Backend,"STi5514");
#elif defined(ST_5516)
    strcpy(Backend,"STi5516");
#elif defined(ST_5517)
    strcpy(Backend,"STi5517");
#elif defined(ST_5518)
    strcpy(Backend,"STi5518");
#elif defined(ST_5528)
    strcpy(Backend,"STi5528");
#elif defined(ST_5100)
    strcpy(Backend,"STi5100");
#elif defined(ST_7710)
    strcpy(Backend,"STi7710");
#elif defined(ST_7100)
   strcpy(Backend, "STi7100");
#elif defined(ST_7109)
   strcpy(Backend, "STi7109");   
#elif defined(ST_5105)
    strcpy(Backend,"STi5105");
#elif defined(ST_5107)
    strcpy(Backend,"STi5107");
#elif defined(ST_5301)
    strcpy(Backend,"STi5301");
#else
    strcpy(Backend, "undef");
#endif

#if defined(mb314)
    strcpy(Platform,"mb314");
#elif defined(mb361)
    strcpy(Platform,"mb361");
#elif defined(mb376)
    strcpy(Platform,"mb376");
#elif defined(mb382)
    strcpy(Platform,"mb382");
#elif defined(mb5518)
    strcpy(Platform,"mb5518");
#elif defined(espresso)
    strcpy(Platform,"espresso");
#elif defined(mb390)
    strcpy(Platform,"mb390");
#elif defined(mb391)
    strcpy(Platform,"mb391");
#elif defined(mb400)
    strcpy(Platform,"mb400");
#elif defined(mb411)
    strcpy(Platform,"mb411");
#elif defined(mb421)
    strcpy(Platform,"mb421");
#elif defined(mb436)
    strcpy(Platform,"mb436");
#else
    strcpy(Platform,"undef");
#endif


    TestDemodType = TEST_DEMOD_TYPE;
    switch (TestDemodType)
    {
       
        case STTUNER_DEMOD_STV0297J:
            strcpy(DemodType,"STV0297J");
            break;

        case STTUNER_DEMOD_STV0297E:
            strcpy(DemodType,"STV0297E");
            break;

        case STTUNER_DEMOD_STV0297:
            strcpy(DemodType,"STV0297");
            break;
            
        case STTUNER_DEMOD_STB0370QAM:
            strcpy(DemodType,"STB0370QAM");
            break;

        default:
            strcpy(DemodType,"NO TUNER DEFINED");
            break;
   
    }
 
 
    TestTunerType = TEST_TUNER_TYPE;
    switch (TestTunerType)
    {
        case STTUNER_TUNER_MT2030:     strcpy(TunerType, "MT2030");     break;
        case STTUNER_TUNER_MT2040:     strcpy(TunerType, "MT2040");     break;
        case STTUNER_TUNER_MT2050:     strcpy(TunerType, "MT2050");     break;
        case STTUNER_TUNER_MT2060:     strcpy(TunerType, "MT2060");     break;
        case STTUNER_TUNER_TDEE4X012A: strcpy(TunerType, "TDEE4X012A"); break;
        case STTUNER_TUNER_DCT7040:    strcpy(TunerType, "DCT7040");    break;
        case STTUNER_TUNER_DCT7050:    strcpy(TunerType, "DCT7050");    break;
        case STTUNER_TUNER_TD1336:     strcpy(TunerType, "TD1336");     break;
        case STTUNER_TUNER_DCT7045:    strcpy(TunerType, "DCT7045");    break;
        case STTUNER_TUNER_TDQE3:      strcpy(TunerType, "TDQE3");      break;
        case STTUNER_TUNER_DCF8783:    strcpy(TunerType, "DCF8783");    break;
        case STTUNER_TUNER_DCT7045EVAL: strcpy(TunerType, "DCT7045EVAL"); break;
        case STTUNER_TUNER_DCT70700:   strcpy(TunerType, "DCT70700");   break;
        case STTUNER_TUNER_TDCHG:      strcpy(TunerType, "TDCHG");      break;
        case STTUNER_TUNER_TDCJG:      strcpy(TunerType, "TDCJG");      break;
        case STTUNER_TUNER_TDCGG:      strcpy(TunerType, "TDCGG");      break;
        default:                       strcpy(TunerType, "NO TUNER");   break;
    }

#ifndef ST_OSLINUX
#ifdef QAMi5107
    /*For QAMi5107 FE Reset */
    STSYS_WriteRegDev32LE((void*)(PIO1_PC0), (0x00|STSYS_ReadRegDev32LE((void*)(PIO1_PC0))));
    STSYS_WriteRegDev32LE((void*)(PIO1_PC1), (0x02|STSYS_ReadRegDev32LE((void*)(PIO1_PC1))));
    STSYS_WriteRegDev32LE((void*)(PIO1_PC2), (0x00|STSYS_ReadRegDev32LE((void*)(PIO1_PC2))));
#endif
#endif


#ifndef ST_OSLINUX  
    debugmessage("Init_boot() ");
    if (Init_boot() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");

   
 #ifndef DISABLE_TOOLBOX
    debugmessage("Init_sttbx() ");
    error = Init_sttbx();
    sprintf(str, "Init_sttbx = %d\n", error);
    debugmessage(str);
    if (error != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");
 #endif
  #endif /* ST_OSLINUX */ 
    debugmessage("Init_evt() ");
    if (Init_evt() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");

   #ifndef ST_OSLINUX
    debugmessage("Init_pio() ");
    if (Init_pio() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");
#endif /* ST_OSLINUX */

    debugmessage("Init_i2c() ");
    if (Init_i2c() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");

    debugmessage("Init_tunerparams() ");
    if (Init_tunerparams() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");
    
    debugmessage("Init_scanparams() ");
    if (Init_tunerparams() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");

    debugmessage("Open_evt() ");
    if (Open_evt() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");
    
    #ifndef ST_OSLINUX
    task_priority_set(NULL, 0);
#endif

    /* ---------------------------------------- */

    TUNER_DebugMessage("**************************************************");
    TUNER_DebugMessage("                STTUNER Test Harness              ");
    TUNER_DebugPrintf(("   Test Harness Revision: %s\n", Revision));
    TUNER_DebugPrintf(("                Platform: %s\n", Platform));
    TUNER_DebugPrintf(("                 Backend: %s\n", Backend));
    TUNER_DebugPrintf(("               I+D Cache: Y\n"));
#if defined(UNIFIED_MEMORY)
    TUNER_DebugPrintf(("          Unified memory: Y\n"));
#else
    TUNER_DebugPrintf(("          Unified memory: N\n"));
#endif
    TUNER_DebugPrintf(("Number of tuners to test: %d\n", NUM_TUNER));
    TUNER_DebugPrintf(("              Demod type: %s\n", DemodType));
    TUNER_DebugPrintf(("              Tuner type: %s\n", TunerType));
     
    TUNER_DebugPrintf(("       Crystal Frequency: %dHz\n", XTAL_CLOCK));
   
#ifndef TUNER_FIRST
    TUNER_DebugPrintf(("              Test Order: 1 to %d\n", NUM_TUNER));
#else
    TUNER_DebugPrintf(("              Test Order: %d to 1\n", NUM_TUNER));
#endif
    TUNER_DebugMessage("**************************************************");
    TUNER_DebugMessage("\n");
    TUNER_DebugMessage("--------------------------------------------------");
    TUNER_DebugPrintf(("                Driver revisions                  \n"));
    TUNER_DebugMessage("--------------------------------------------------");
    #ifndef ST_OSLINUX  
    TUNER_DebugPrintf(("          STBOOT:  %s\n",  STBOOT_GetRevision()  ));
    TUNER_DebugPrintf(("        STCOMMON:  %s\n",  STCOMMON_REVISION     ));
   
    TUNER_DebugPrintf(("           STEVT:  %s\n",  STEVT_GetRevision()   ));
    
    TUNER_DebugPrintf(("           STI2C:  %s\n",  STI2C_GetRevision()   ));
    TUNER_DebugPrintf(("           STPIO:  %s\n",  STPIO_GetRevision()   ));
    TUNER_DebugPrintf(("           STSYS:  %s\n",  STSYS_GetRevision()   ));
   
     #ifndef DISABLE_TOOLBOX
    TUNER_DebugPrintf(("           STTBX:  %s\n",  STTBX_GetRevision()   ));
    #ifndef STTBX_NO_UART
    TUNER_DebugPrintf(("          STUART:  %s\n",  STUART_GetRevision()  ));
    #endif
    #endif
    #endif  /* ST_OSLINUX */
    TUNER_DebugPrintf(("         STTUNER:  %s\n",  STTUNER_GetRevision() ));
    TUNER_DebugMessage("--------------------------------------------------\n");
    
#ifndef ST_OSLINUX    
#if defined (ST_5518)
    if (strcmp(STI2C_GetRevision(),"STI2C-REL_1.5.0A1") > 0)
    {
        TUNER_DebugPrintf(("5518 does not support STI2C version greater than 1.5.0A1\n"));
        return(0);
    }
#else

    if (strcmp(STI2C_GetRevision(),"STI2C-REL_2.0.0") < 0)
    {
        TUNER_DebugPrintf(("STI2C version less than 2.0.0 is not recommended for %s chip,\n", Backend));
        return(0);
    }
#endif    
#endif/*ST_OSLINUX*/

    #ifdef TUNER_297
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0297)); 
    TUNER_DebugPrintf(("         297 LLA: %s\n",   LLARevision1 ));
    #endif
    #ifdef TUNER_297J
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0297J)); 
    TUNER_DebugPrintf(("         297J LLA: %s\n",   LLARevision1 ));
    #endif
    #ifdef TUNER_297E
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0297E)); 
    TUNER_DebugPrintf(("         297E LLA: %s\n",   LLARevision1 ));
    #endif
    #ifdef TUNER_370QAM
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STB0370QAM)); 
    TUNER_DebugPrintf(("         370QAM LLA: %s\n",   LLARevision1 ));
    #endif

    TUNER_DebugMessage("--------------------------------------------------");
    TUNER_DebugMessage("                  Test built on");
    TUNER_DebugPrintf(("        Date: %s   Time: %s \n", __DATE__, __TIME__  ));
    TUNER_DebugMessage("--------------------------------------------------\n");

    GrandTotal.NumberPassed = 0;
    GrandTotal.NumberFailed = 0;

#ifndef TUNER_FIRST
    for(tuners = 0; tuners < NUM_TUNER; tuners++)
#else
    for(tuners = NUM_TUNER-1; tuners >= 0; tuners--)
#endif
    {
        Init_scanparams();
        Test_p  = TestTable;
        Section = 1;
        ChannelTableCount  = 0;
        Total[tuners].NumberPassed = 0;
        Total[tuners].NumberFailed = 0;

        TUNER_DebugMessage("==================================================");
        TUNER_DebugPrintf(("  Test tuner: '%s'\n", TunerDeviceName[tuners] ));
        TUNER_DebugMessage("==================================================");


        while (Test_p->TestFunction != NULL)
        {
            TUNER_DebugMessage("**************************************************");
            TUNER_DebugPrintf(("SECTION %d - %s\n", Section, Test_p->TestInfo));
            TUNER_DebugMessage("**************************************************");

            /* Set up parameters for test function */
            TestParams.Ref   = Section;
            TestParams.Tuner = tuners;

            /* Call test */
            Result = Test_p->TestFunction(&TestParams);

            /* Update counters */
            Total[tuners].NumberPassed += Result.NumberPassed;
            Total[tuners].NumberFailed += Result.NumberFailed;

            Test_p++;
            Section++;

            TUNER_DebugMessage("--------------------------------------------------");
            TUNER_DebugPrintf((" Running total: PASSED: %d\n", Total[tuners].NumberPassed));
        TUNER_DebugPrintf(("FAILED: %d\n", Total[tuners].NumberFailed));
            TUNER_DebugMessage("--------------------------------------------------");
        }

        GrandTotal.NumberPassed += Total[tuners].NumberPassed;
        GrandTotal.NumberFailed += Total[tuners].NumberFailed;

    } /* for(tuners) */


    TUNER_DebugMessage("==================================================");
    TUNER_DebugPrintf((" Grand Total: PASSED: %d\n", GrandTotal.NumberPassed));
    TUNER_DebugPrintf(("              FAILED: %d\n", GrandTotal.NumberFailed));
    TUNER_DebugMessage("==================================================");
    GlobalTerm();
 #ifndef ST_OSLINUX  
    STBOOT_Term( "boot", &BootTerm );
    #endif 

    return(0);
}

/* ------------------------------------------------------------------------- */
/*                     EVENT callback & wait functions                       */
/* ------------------------------------------------------------------------- */


/* If using normal callback function */
#ifndef STEVT_ENABLED

static void CallbackNotifyFunction1(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType, ST_ErrorCode_t Error)
{
    LastEvent1 = EventType;

    STOS_SemaphoreSignal(EventSemaphore1);

}

static void CallbackNotifyFunction2(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType, ST_ErrorCode_t Error)
{
    LastEvent2 = EventType;

    STOS_SemaphoreSignal(EventSemaphore2);

}

#else

#define CallbackNotifyFunction1      NULL
#define CallbackNotifyFunction2      NULL

/* If using Event Handler API */
static void EVTNotifyFunction1(STEVT_CallReason_t     Reason,
                              const ST_DeviceName_t  RegistrantName,
                              STEVT_EventConstant_t  Event,
                              const void            *EventData,
                              const void            *SubscriberData_p )
{
    LastEvent1 = Event;

    STOS_SemaphoreSignal(EventSemaphore1);

}

static void EVTNotifyFunction2(STEVT_CallReason_t     Reason,
                              const ST_DeviceName_t  RegistrantName,
                              STEVT_EventConstant_t  Event,
                              const void            *EventData,
                              const void            *SubscriberData_p )
{
    LastEvent2 = Event;

    STOS_SemaphoreSignal(EventSemaphore2);

}
#endif


static void AwaitNotifyFunction1(STTUNER_EventType_t *EventType_p)
{
#ifdef ST_OS21
    osclock_t time;
#else
    clock_t time;
#endif
    int result;
    time = STOS_time_plus(STOS_time_now(), 30 * ST_GetClocksPerSecond());

    result = STOS_SemaphoreWaitTimeOut(EventSemaphore1, &time);

    if (result == -1) *EventType_p = STTUNER_EV_TIMEOUT;
     else  *EventType_p = LastEvent1;
}


static void AwaitNotifyFunction2(STTUNER_EventType_t *EventType_p)
{
#ifdef ST_OS21
    osclock_t time;
#else
    clock_t time;
#endif
    int result;
    time = STOS_time_plus(STOS_time_now(),30 * ST_GetClocksPerSecond());

    result = STOS_SemaphoreWaitTimeOut(EventSemaphore2, &time);


    if (result == -1) *EventType_p = STTUNER_EV_TIMEOUT;
     else  *EventType_p = LastEvent2;
}



/* ------------------------------------------------------------------------- */
/*                       Initalization functions                             */
/* ------------------------------------------------------------------------- */
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_boot(void)
{
    STBOOT_InitParams_t BootParams;

    /* Cache Map */
    STBOOT_DCache_Area_t DCacheMap[] = {
#if defined(UNIFIED_MEMORY)
        {(U32 *) 0xC0380000, (U32 *) 0xC07FFFFF},   /* SMI Mb */
#else
        {(U32 *) 0x40000000, (U32 *) 0x4fffffff},   /* EMI 16Mb */
#endif
        {NULL, NULL}
    };

   /* Create memory partitions */
#ifdef ST_OS21
/* partition_create_simple_p((partition_t*)&the_internal_partition, (U8 *)internal_block, sizeof(internal_block) );*/
system_partition =  partition_create_heap( (U8 *)external_block, sizeof(external_block) );
/*Partition =  partition_create_heap_p(  (partition_t*)&the_ncache_partition,         ncache_memory,   sizeof(ncache_memory) );*/
#else
    partition_init_simple(&the_internal_partition, (U8 *)internal_block, sizeof(internal_block) );
    partition_init_heap(  &the_system_partition,   (U8 *)external_block, sizeof(external_block) );
    partition_init_heap(  &the_ncache_partition,         ncache_memory,   sizeof(ncache_memory) );
#endif

    BootParams.SDRAMFrequency   = SDRAM_FREQUENCY;
    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;
	#ifdef ST_OS21
	  BootParams.TimeslicingEnabled = TRUE;
	#endif 
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;

    BootParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

#if defined(ST_5512) || defined(mb5518) || defined(mb5518um)
    BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_64_MBIT;
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100)|| defined(ST_7710) || defined(ST_5105)|| defined(ST_5107) || defined(ST_7100) || defined(ST_7109) || defined(ST_5301)  /* do not init SMI memory */
    BootParams.MemorySize = SDRAM_SIZE;
#else /* Default 32Mbit on 5510 processor */
    BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_32_MBIT;
#endif

    return( STBOOT_Init( "boot", &BootParams ) );
}
#endif /*ST_OSLINUX*/
/* ------------------------------------------------------------------------- */
#ifndef ST_OSLINUX
#ifndef DISABLE_TOOLBOX
static ST_ErrorCode_t Init_sttbx(void)
{
    STTBX_InitParams_t TBXInitParams;

    memset(&TBXInitParams, '\0', sizeof(STTBX_InitParams_t) );
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p      = system_partition;

    return( STTBX_Init( "tbx", &TBXInitParams ) );
}
#endif
#endif /*ST_OSLINUX*/
/* ------------------------------------------------------------------------- */
#ifndef ST_OSLINUX
#ifndef USE_STI2C_WRAPPER
static ST_ErrorCode_t Init_pio(void)
{
    ST_DeviceName_t DeviceName;
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;

    /* Automatically generate device names */
    for (i = 0; i < NUM_PIO; i++)       /* PIO */
    {
        sprintf((char *)DeviceName, "STPIO[%d]", i);
        strcpy((char *)PioDeviceName[i], (char *)DeviceName);
    }
    PioDeviceName[i][0] = 0;

    PioInitParams[0].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;

    PioInitParams[1].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[1].BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
    PioInitParams[1].InterruptNumber = PIO_1_INTERRUPT;
    PioInitParams[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;

    PioInitParams[2].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[2].BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
    PioInitParams[2].InterruptNumber = PIO_2_INTERRUPT;
    PioInitParams[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;

    PioInitParams[3].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[3].BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
    PioInitParams[3].InterruptNumber = PIO_3_INTERRUPT;
    PioInitParams[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;

#if !defined(ST_5105) && !defined(ST_5107)
    PioInitParams[4].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[4].BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
#endif

#if defined(ST_5518)
    PioInitParams[5].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[5].BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
#endif

    /* Initialize drivers */
    for (i = 0; i < NUM_PIO; i++)
    {
        error = STPIO_Init(PioDeviceName[i], &PioInitParams[i]);
        debugmessage(" STPIO_Init()");
        if (error != ST_NO_ERROR)
        {
            debugmessage("Unable to initialize STPIO");
            return(error);
        }
    }
    debugmessage(" .\n");

    return(error);
}
#endif /*#ifndef USE_STI2C_WRAPPER*/
#endif /*ST_OSLINUX*/

/* ------------------------------------------------------------------------- */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
static ST_ErrorCode_t Init_i2c(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    ST_DeviceName_t DeviceName;
    U32 i;

    for (i = 0; i < NUM_I2C; i++)       /* I2C */
    {
        sprintf((char *)DeviceName, "STI2C[%d]", i);
        strcpy((char *)I2CDeviceName[i], (char *)DeviceName);
    }
    I2CDeviceName[i][0] = 0;

    I2CInitParams[0].BaseAddress       = I2C_0_BASE_ADDRESS;
    I2CInitParams[0].PIOforSDA.BitMask = I2C_0_DATA_BIT;
    I2CInitParams[0].PIOforSCL.BitMask = I2C_0_CLK_BIT;
    I2CInitParams[0].InterruptNumber   = I2C_0_INT_NUMBER;
    I2CInitParams[0].InterruptLevel    = I2C_0_INT_LEVEL;
    I2CInitParams[0].MasterSlave       = STI2C_MASTER;
    I2CInitParams[0].BaudRate          = I2C_BAUDRATE;
    I2CInitParams[0].ClockFrequency    = ST_I2C_CLOCKFREQUENCY;
    I2CInitParams[0].MaxHandles        = 8;
    I2CInitParams[0].DriverPartition   = system_partition;

    #ifdef ST_5100
        I2CInitParams[0].GlitchWidth = 0x05;
    #endif
    strcpy(I2CInitParams[0].PIOforSCL.PortName, PioDeviceName[I2C_0_CLK_DEV]);
    strcpy(I2CInitParams[0].PIOforSDA.PortName, PioDeviceName[I2C_0_DATA_DEV]);

    I2CInitParams[1].BaseAddress       = I2C_1_BASE_ADDRESS;
    I2CInitParams[1].PIOforSDA.BitMask = I2C_1_DATA_BIT;
    I2CInitParams[1].PIOforSCL.BitMask = I2C_1_CLK_BIT;
    I2CInitParams[1].InterruptNumber   = I2C_1_INT_NUMBER;
    I2CInitParams[1].InterruptLevel    = I2C_1_INT_LEVEL;
    I2CInitParams[1].MasterSlave       = STI2C_MASTER;
    I2CInitParams[1].BaudRate          = I2C_BAUDRATE;

    I2CInitParams[1].ClockFrequency    = ST_I2C_CLOCKFREQUENCY;

    I2CInitParams[1].MaxHandles        = 8;
    I2CInitParams[1].DriverPartition   = system_partition;

    #ifdef ST_5100
        I2CInitParams[1].GlitchWidth = 0x05;
    #endif
    strcpy(I2CInitParams[1].PIOforSDA.PortName, PioDeviceName[I2C_1_DATA_DEV]);
    strcpy(I2CInitParams[1].PIOforSCL.PortName, PioDeviceName[I2C_1_CLK_DEV]);

    for (i = 0; i < NUM_I2C; i++)
    {
        debugmessage(" STI2C_Init()...");
        if (I2CInitParams[i].BaseAddress != 0) /* Is in use? */
        {
            error = STI2C_Init(I2CDeviceName[i], &I2CInitParams[i]);
            if (error != ST_NO_ERROR)
            {
                debugmessage("Unable to initialize STI2C");
                return(error);
            }
            else
            {
                debugmessage("ok ");
            }
        }
        else
        {
            debugmessage("fail ");
        }
    }
    debugmessage(" done\n");
#if defined(ST_5105) || defined(ST_5107)
 /* Configure Config Control Reg G for selecting ALT0 functions for PIO3<3:2> */
   /* 0000_0000_0000_0000_0000_0000_0000_0000 (INT_CONFIG_CONTROL_G)            */
   /* |||| |||| |||| |||| |||| |||| |||| ||||_____ pio2_altfop_mux0sel<7:0>     */
   /* |||| |||| |||| |||| |||| ||||_______________ pio2_altfop_mux1sel<7:0>     */
   /* |||| |||| |||| ||||_________________________ pio3_altfop_mux0sel<7:0>     */
   /* |||| ||||___________________________________ pio3_altfop_mux1sel<7:0>     */
   /* ========================================================================= */
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
   STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, ReadValue | 0x00000000 );
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
   /* Configure Config Control Reg H for selecting ALT1 functions for PIO4<3:2> */
   /*                0000_0000_0000_0000_1100 (INT_CONFIG_CONTROL_H)            */
   /*                |||| |||| |||| |||| ||||_____ pio4_altfop_mux0sel<3:0>     */
   /*                |||| |||| |||| ||||__________ Reserved<3:0>                */
   /*                |||| |||| ||||_______________ pio4_altfop_mux1sel<3:0>     */
   /*                |||| ||||____________________ Reserved<3:0>                */
   /* ========================================================================= */
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
   STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, ReadValue |0x0c000000 );
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
#endif
    return(error);
}
#else /* */

static ST_ErrorCode_t Init_i2c(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    ST_DeviceName_t DeviceName;
    U32 i;

    for (i = 0; i < NUM_I2C; i++)       /* I2C */
    {
        sprintf((char *)DeviceName, "STI2C[%d]", i);
        strcpy((char *)I2CDeviceName[i], (char *)DeviceName);
    }
    I2CDeviceName[i][0] = 0;

    error = STI2C_Init(I2CDeviceName[0], &I2CInitParams[0]);
    if (error != ST_NO_ERROR)
    {
        printf("Unable to initialize STI2C \n");
    }
    else
    {
        printf("ok \n");
    }
    return error;
}

#endif /* #ifndef USE_STI2C_WRAPPER */
/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Init_tunerparams(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    ST_DeviceName_t DeviceName;
    U32 i;
   

    for (i = 0; i < NUM_TUNER; i++)     /* TUNER */
    {
    	
        sprintf((char *)DeviceName, "STTUNER[%d]", i);
        strcpy((char *)TunerDeviceName[i], (char *)DeviceName);

        /* Set init params */
        
        TunerInitParams[i].Device            = STTUNER_DEVICE_CABLE;
	
        TunerInitParams[i].Max_BandList      = 10;
        TunerInitParams[i].Max_ThresholdList = 10;
        TunerInitParams[i].Max_ScanList      = 10;

        strcpy( TunerInitParams[i].EVT_DeviceName, "stevt" );


        TunerInitParams[i].DriverPartition   = system_partition;
	
        
        TunerInitParams[i].DemodType         = TEST_DEMOD_TYPE;
        
	
        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_DIRECT;        /* send to o/i function            */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].DemodIO.Address   = DEMOD_I2C;               /* stv0297/stv0297J/stv0297E/370QAM address        */
        TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;      
               

        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;      /* send to demod repeater function */
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].TunerIO.Address   = TUNER_I2C;                /* address for Tuner (VG1011)      */
        TunerInitParams[i].ExternalClock     = XTAL_CLOCK;
        TunerInitParams[i].SyncStripping     = STTUNER_SYNC_STRIP_DEFAULT;
        TunerInitParams[i].ClockPolarity     = STTUNER_DATA_CLOCK_POLARITY_DEFAULT;
        TunerInitParams[i].BlockSyncMode     = STTUNER_SYNC_FORCED;

        switch( TunerInitParams[i].TunerType )
        {
        	
            case STTUNER_TUNER_TDBE1:
            case STTUNER_TUNER_TDBE2:
            case STTUNER_TUNER_TDDE1:
            case STTUNER_TUNER_SP5730:
            case STTUNER_TUNER_DCT7040:
            case STTUNER_TUNER_DCT7710:
            case STTUNER_TUNER_DCF8710:
            case STTUNER_TUNER_CD1516LI:
            case STTUNER_TUNER_DF1CS1223:
            case STTUNER_TUNER_SHARPXX:
            case STTUNER_TUNER_MT2030:
            case STTUNER_TUNER_MT2040:
            case STTUNER_TUNER_MT2050:
            case STTUNER_TUNER_MT2060:
            case STTUNER_TUNER_TD1336:
            case STTUNER_TUNER_DCT7045:
            case STTUNER_TUNER_TDQE3:
            case STTUNER_TUNER_DCF8783:
            case STTUNER_TUNER_DCT7045EVAL:
            case STTUNER_TUNER_DCT70700:
            case STTUNER_TUNER_TDCHG:
            case STTUNER_TUNER_TDCJG:
            case STTUNER_TUNER_TDCGG:

                TunerInitParams[i].TunerIO.Route        = STTUNER_IO_REPEATER;  /*STTUNER_IO_PASSTHRU STTUNER_IO_DIRECT STTUNER_IO_REPEATER;*/      /* send to demod repeater function */
                TunerInitParams[i].TunerIO.Driver       = STTUNER_IODRV_I2C;
                break;
            case STTUNER_TUNER_TDBE1X016A:
                TunerInitParams[i].TunerIO.Route        = STTUNER_IO_DIRECT;  /*STTUNER_IO_PASSTHRU STTUNER_IO_DIRECT STTUNER_IO_REPEATER*/;      /* send to demod repeater function */
                TunerInitParams[i].TunerIO.Driver       = STTUNER_IODRV_I2C;
                break;
            case STTUNER_TUNER_TDBE1X601:
                TunerInitParams[i].TunerIO.Route        = STTUNER_IO_DIRECT;  /*STTUNER_IO_PASSTHRU STTUNER_IO_DIRECT STTUNER_IO_REPEATER*/;      /* send to demod repeater function */
                TunerInitParams[i].TunerIO.Driver       = STTUNER_IODRV_I2C;
                break;
            case STTUNER_TUNER_TDEE4X012A:
                TunerInitParams[i].TunerIO.Driver       = STTUNER_IODRV_I2C;
                break;
            default:
                TunerInitParams[i].TunerIO.Address      = 0xC2;                 /* address for Tuner       */
                TunerInitParams[i].TunerIO.Route        = STTUNER_IO_REPEATER;  /*STTUNER_IO_PASSTHRU STTUNER_IO_DIRECT STTUNER_IO_REPEATER;*/      /* send to demod repeater function */
                TunerInitParams[i].TunerIO.Driver       = STTUNER_IODRV_I2C;
                TunerInitParams[i].ExternalClock        = 28920000;             /* On Eval297 */
                break;
                
        }

/*        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_PASSTHRU;
        TunerInitParams[i].LnbIO.Driver      = STTUNER_IODRV_I2C;
        TunerInitParams[i].LnbIO.Address     = 0x00; */                               /* ignored                         */

        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
      /*  TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;*/

    }

    TunerDeviceName[i][0] = 0;

    strcpy(TunerInitParams[0].DemodIO.DriverName, I2CDeviceName[TUNER0_DEMOD_I2C]);       /* front I2C bus                   */
    strcpy(TunerInitParams[0].TunerIO.DriverName, I2CDeviceName[TUNER0_TUNER_I2C]);       /* front I2C bus                   */
   /* strcpy(TunerInitParams[0].LnbIO.DriverName,   I2CDeviceName[TUNER0_DEMOD_I2C]); */      /* front I2C bus, ignored          */

    strcpy(TunerInitParams[1].DemodIO.DriverName, I2CDeviceName[TUNER1_DEMOD_I2C]);       /* front I2C bus                   */
    strcpy(TunerInitParams[1].TunerIO.DriverName, I2CDeviceName[TUNER1_TUNER_I2C]);       /* front I2C bus                   */
   /* strcpy(TunerInitParams[1].LnbIO.DriverName,   I2CDeviceName[TUNER1_DEMOD_I2C]);*/       /* front I2C bus, ignored          */

    strcpy( TunerInitParams[0].EVT_RegistrantName, EVT_RegistrantName1 );
    strcpy( TunerInitParams[1].EVT_RegistrantName, EVT_RegistrantName2 );
    return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Init_scanparams(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Set tuner band list */
    Bands.NumElements = 2;

    BandList[0].BandStart = 40000000;              /* Hz */
    BandList[0].BandEnd   = 900000000;              /* Hz */
/*    BandList[0].LO        = 0;*/

    /* Setup scan list */
    Scans.NumElements = 2;

    /* gbgb ScanList[0].SymbolRate   = 6875000; gbgb*/            /* S/s */
    #ifdef TUNER_370QAM
    ScanList[0].SymbolRate   = 5057000;             /* S/s */
    #else
    ScanList[0].SymbolRate   = 6875000;             /* S/s */
    #endif
    
    #ifdef TUNER_297J
      ScanList[0].SymbolRate   = 5057000;
     
    #endif   
     
   /* ScanList[0].Polarization = 0;
    ScanList[0].FECRates     = 0;*/
    ScanList[0].Modulation   =STTUNER_MOD_64QAM;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
   /* ScanList[0].LNBToneState = 0;*/
    ScanList[0].Band         = 0;
    ScanList[0].Spectrum     = STTUNER_INVERSION_AUTO;
    
    #ifdef TUNER_370QAM
    ScanList[0].J83          = STTUNER_J83_B;
    #else
    ScanList[0].J83          = STTUNER_J83_A;
    #endif
    
     #ifdef TUNER_297J
     
        ScanList[0].J83   = STTUNER_J83_B;
     
    #endif
    	
    #ifdef  TUNER_370QAM
    ScanList[1].SymbolRate   = 5360500;             /* S/s */
    #else
    ScanList[1].SymbolRate   = 4583000;             /* S/s */
    #endif
    
    #ifdef TUNER_297J
      
        ScanList[1].SymbolRate   = 5360500;
     
    #endif 
/*    ScanList[1].Polarization = 0;
    ScanList[1].FECRates     = 0;*/
    ScanList[1].Modulation   = STTUNER_MOD_256QAM;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
   /* ScanList[1].LNBToneState = 0;*/
    ScanList[1].Band         = 0;
    ScanList[1].Spectrum     = STTUNER_INVERSION_AUTO;
    
    #ifdef TUNER_370QAM
    ScanList[1].J83          = STTUNER_J83_B;
    #else
    ScanList[1].J83          = STTUNER_J83_A;
    #endif
    
     #ifdef TUNER_297J
      
        ScanList[1].J83   = STTUNER_J83_B;
    
    #endif  
    Bands.BandList = BandList;
    Scans.ScanList = ScanList;

    return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Init_evt(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

#ifdef STEVT_ENABLED
    STEVT_InitParams_t EVTInitParams;

    /* Event handler initialization */
    EVTInitParams.MemoryPartition = system_partition;
    EVTInitParams.EventMaxNum   = 12;
    EVTInitParams.ConnectMaxNum = 10;
    EVTInitParams.SubscrMaxNum  = 10;

    error = STEVT_Init("stevt", &EVTInitParams);
    TUNER_DebugError("STEVT_Init()", error);

    if (error != ST_NO_ERROR)
    {
        TUNER_DebugMessage("Unable to initialize EVT device");
    }
#endif

    /* Create global semaphore */

    EventSemaphore1 = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    EventSemaphore2 = STOS_SemaphoreCreateFifoTimeOut(NULL,0);


    return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Open_evt(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

#ifdef STEVT_ENABLED
    STEVT_DeviceSubscribeParams_t SubscribeParams;
    STEVT_OpenParams_t EVTOpenParams;
    STEVT_Handle_t EVTHandle1, EVTHandle2;

    SubscribeParams.NotifyCallback     = EVTNotifyFunction1;
    SubscribeParams.SubscriberData_p   = NULL;

    error = STEVT_Open("stevt", &EVTOpenParams, &EVTHandle1);
    TUNER_DebugError("STEVT_Open()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle1, (char *)EVT_RegistrantName1, STTUNER_EV_LOCKED, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle1, (char *)EVT_RegistrantName1, STTUNER_EV_UNLOCKED, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle1, (char *)EVT_RegistrantName1, STTUNER_EV_TIMEOUT, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle1, (char *)EVT_RegistrantName1, STTUNER_EV_SCAN_FAILED, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle1, (char *)EVT_RegistrantName1, STTUNER_EV_SIGNAL_CHANGE, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    SubscribeParams.NotifyCallback     = EVTNotifyFunction2;
    SubscribeParams.SubscriberData_p   = NULL;

    error = STEVT_Open("stevt", &EVTOpenParams, &EVTHandle2);
   TUNER_DebugError("STEVT_Open()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle2, (char *)EVT_RegistrantName2, STTUNER_EV_LOCKED, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle2, (char *)EVT_RegistrantName2, STTUNER_EV_UNLOCKED, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle2, (char *)EVT_RegistrantName2, STTUNER_EV_TIMEOUT, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle2, (char *)EVT_RegistrantName2, STTUNER_EV_SCAN_FAILED, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

    error = STEVT_SubscribeDeviceEvent(EVTHandle2, (char *)EVT_RegistrantName2, STTUNER_EV_SIGNAL_CHANGE, &SubscribeParams);
    TUNER_DebugError("STEVT_Subscribe()", error);

#endif

    return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t GlobalTerm(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;
    STI2C_TermParams_t I2C_TermParams;
    #ifndef ST_OSLINUX
    STPIO_TermParams_t PIO_TermParams;
    #endif

    /* Delete global semaphore */

    STOS_SemaphoreDelete(NULL,EventSemaphore1);
    STOS_SemaphoreDelete(NULL,EventSemaphore2);


    for (i = 0; i < NUM_I2C; i++)
    {
        if (I2CInitParams[i].BaseAddress != 0) /* Is in use? */
        {
            error = STI2C_Term(I2CDeviceName[i], &I2C_TermParams);
            TUNER_DebugError("STI2C_Term()", error);
            if (error != ST_NO_ERROR)
            {
                TUNER_DebugPrintf(("Unable to terminate %s.\n", I2CDeviceName[i]));
            }
        }
    }
#ifndef ST_OSLINUX
    for (i = 0; i < NUM_PIO; i++)
    {

        error = STPIO_Term(PioDeviceName[i], &PIO_TermParams);
        TUNER_DebugError("STPIO_Term()", error);
        if (error != ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Unable to terminate %s.\n", PioDeviceName[i]));
        }
    }
#endif /*ST_OSLINUX*/
    return(error);
}



/* ------------------------------------------------------------------------- */
/* ------------------------ Test harness tests ----------------------------- */
/* ------------------------------------------------------------------------- */


/*----------------------------------------------------------------------------
Function:
    TunerIoctl()

Description:
    This routine tests features of the ioctl API 3.2.0

Parameters:
    Params_p,       pointer to test params block.

Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.

See Also:
    Nothing.
----------------------------------------------------------------------------*/

static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t     Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t         Error, errc, errt;
    STTUNER_Handle_t       Handle;
    STTUNER_OpenParams_t   OpenParams;
    STTUNER_TermParams_t   TermParams;
    #ifndef ST_OSLINUX
    U32                    inparam,outparam,original;
    #endif
    U32                    Test = 0;
    TEST_ProbeParams_t     ProbeParams;
    U8                     address;

    /* ---------- PROBE FOR CHIP TYPE - TUNER UNINITALIZED ---------- */

    TUNER_DebugPrintf(("Test #%d.1\n", ++Test));
    TUNER_DebugMessage("Purpose: Probe to find chip types on I2C bus (STTUNER_IOCTL_PROBE)");
    TUNER_DebugMessage("         With tuner uninitalized.");

    /* look for a chip ID (register 0x00) at I2C address 0xD0, 0xD2, 0xD4 and 0xD6 */
    /* ASSUME: chip ID register is at offset zero! */
    ProbeParams.Driver     = STTUNER_IODRV_I2C;
    #ifdef TUNER_370QAM
    ProbeParams.SubAddress = 0x00;     /* register address */
    #else
    ProbeParams.SubAddress = 0x80;     /* register address */
    #endif
    
    ProbeParams.XferSize   = 1;     /* get one byte     */
    ProbeParams.TimeOut    = 50;    /* I2C timeout, be generous */

    /* get I2C bus to use from tuner under test */
    strcpy(ProbeParams.DriverName, TunerInitParams[Params_p->Tuner].DemodIO.DriverName);

    if (strcmp(TunerInitParams[Params_p->Tuner].DemodIO.DriverName,"STI2C[0]") == 0)
    {
         TUNER_DebugMessage(("--- Probe BACK I2C bus ---"));
    }
    else if (strcmp(TunerInitParams[Params_p->Tuner].DemodIO.DriverName,"STI2C[1]") == 0)
    {
         TUNER_DebugMessage(("--- Probe FRONT I2C bus ---"));
    }
    else
    {
         TUNER_DebugMessage(("--- Probe ????? I2C bus ---"));
    }

    for(address = START_I2C_ADDR; address <= END_I2C_ADDR; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(STTUNER_MAX_HANDLES)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n", ProbeParams.Address, ProbeParams.Value));

            switch (TestDemodType)
            {
                
                case STTUNER_DEMOD_STV0297:
                    switch(ProbeParams.Value & 0x70)
                    {
                        case 0x20:
                            TUNER_DebugMessage("Chip is STV0297");
                            break;
                        default:
                            TUNER_DebugMessage("Chip present but is unknown");
                            break;
                    }
                    break;
                case STTUNER_DEMOD_STV0297J:
                    switch(ProbeParams.Value & 0x70)
                    {
                        case 0x01:
                            TUNER_DebugMessage("Chip is STV0297J");
                            break;
                        default:
                            TUNER_DebugMessage("Chip is STV0297J");
                            break;
                    }
                    break;
                case STTUNER_DEMOD_STV0297E:
                    switch(ProbeParams.Value & 0x70)
                    {
                        case 0x10:
                            TUNER_DebugMessage("Chip is STV0297E");
                            break;
                        default:
                            TUNER_DebugMessage("Chip present but is unknown");
                            break;
                    }
                    break;
                case STTUNER_DEMOD_STB0370QAM:
                    switch(ProbeParams.Value & 0x70)
                    {
                        case 0x10:
                            TUNER_DebugMessage("Chip is STB0370QAM");
                            break;
                default:
                            TUNER_DebugMessage("Chip present but is unknown");
                            break;
                    }
                    break;
                default:
                    break;
                    
            }
        }
        else
        {
            TUNER_DebugPrintf(("Chip not present at address 0x%02x\n", address));
        }

    }   /* for(address) */


    /* ---------- INITALIZE & OPEN TUNER ---------- */

    TUNER_DebugPrintf(("Test #%d.2\n", Test));
    TUNER_DebugMessage("Purpose: Initalize & open tuner instance");
     
    /*Checking INIT PARSMS*/
   
    
  
    Error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }
#ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
#else
        OpenParams.NotifyFunction = NULL;
#endif

  
    Error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &Handle);
    TUNER_DebugError("STTUNER_Open()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }

    /* ---------- PROBE FOR CHIP TYPE - TUNER INITALIZED ---------- */

    TUNER_DebugPrintf(("Test #%d.3\n", Test));
    TUNER_DebugMessage("Purpose: Probe to find chip types on I2C bus (STTUNER_IOCTL_PROBE)");
    TUNER_DebugMessage("         With tuner initalized (Handle)");

    for(address = START_I2C_ADDR; address <= END_I2C_ADDR; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(Handle)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n",ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0x70)
            {
                case 0x20:
                    TUNER_DebugMessage("Chip is STV0297");
                    break;
                case 0x10:
                            TUNER_DebugMessage("Chip is STB0370QAM");
                            break;
                default:
                    TUNER_DebugMessage("Chip unknown");
                    break;
            }
        }
        else
        {
            TUNER_DebugPrintf(("Chip not present at address 0x%02x\n", address));
        }

    }   /* for(address) */

    TUNER_DebugPrintf(("Test #%d.4\n", Test));
    TUNER_DebugMessage("Purpose: Probe to find chip types on I2C bus (STTUNER_IOCTL_PROBE)");
    TUNER_DebugMessage("         With tuner initalized (STTUNER_MAX_HANDLES)");

    for(address = START_I2C_ADDR; address <= END_I2C_ADDR; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(STTUNER_MAX_HANDLES)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n",ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0x70)
            {
                case 0x20:
                    TUNER_DebugMessage("Chip is STV0297");
                    break;
                case 0x10:
                     TUNER_DebugMessage("Chip is STB0370QAM");
                     break;
                default:
                    TUNER_DebugMessage("Chip unknown");
                    break;
            }
        }
        else
        {
            TUNER_DebugPrintf(("Chip not present at address 0x%02x\n", address));
        }

    }   /* for(address) */

    /* ---------- CALL DEMO FUNCTION ---------- */

    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugMessage("Purpose: Call demo ioctl function (STTUNER_IOCTL_TEST_01)");
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_TEST_01, NULL, NULL);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    TUNER_DebugError("STTUNER_Ioctl()", Error);


    /* ---------- CHANGE SCAN TASK PRIORITY ---------- */
     #ifndef ST_OSLINUX /**** This done because right now no support to retrieve task information in Linux ******/

    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugMessage("Purpose: Change scan task priority (STTUNER_IOCTL_SCANPRI)");

    TUNER_DebugPrintf(("Test #%d.1: Get scan task priority.\n", Test));
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_SCANPRI, NULL, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    original = outparam; /* save original priority */


    TUNER_DebugPrintf(("Test #%d.2: Set scan task priority from %d to 0.\n", Test, original));
    inparam = 0;
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_SCANPRI, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    TUNER_DebugPrintf(("Scan task priority is now: %d\n", outparam ));
    if (outparam != 0)
    {
        TUNER_TestFailed(Result,"Expected priority set to 0.");
        goto ioctl_end;
    }


    TUNER_DebugPrintf(("Test #%d.3: Set scan task priority back to original (%d)\n", Test, original ));
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_SCANPRI, &original, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    TUNER_DebugPrintf(("Scan task priority is now: %d\n", outparam ));
    if (outparam != original)
    {
        TUNER_TestFailed(Result,"Expected original priority.");
        goto ioctl_end;
    }
   #endif /*** end of #ifndef ST_OSLINUX*****/

    /* ----------  END OF IOCTL TESTS ---------- */

ioctl_end:
    errc = STTUNER_Close(Handle);
    TUNER_DebugError("STTUNER_Close()", errc);

    TermParams.ForceTerminate = FALSE;
    errt = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", errt);

    /* OR errors together to find out if any happened */
    Error = Error | errc | errt;

    if (Error == ST_NO_ERROR)
    {
        TUNER_TestPassed(Result);
    }

    return(Result);
}

/*-------------------------------------------------------------------------
 * Function : TUNER_StatusToString
 *            Convert Status value to text
 * Input    : Event value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
char *TUNER_StatusToString(STTUNER_Status_t Status)
{
    static char StatusDescription[][10] =
    {
        "UNLOCKED",
        "SCANNING",
        "LOCKED",
        "NOT_FOUND",
        "STANDBY",
        "IDLE"
    };

    return StatusDescription[Status];

} /* end TUNER_StatusToString */

/*----------------------------------------------------------------------------
TunerReLock()
Description:
    This routine scans to the first frequency.

    We then call the abort routine to ensure we are notified of the
    unlocking of the tuner.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t TunerReLock(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t      error;
    STTUNER_Handle_t    TUNERHandle;
    STTUNER_EventType_t Event=STTUNER_EV_UNLOCKED, ExpectedEvent;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_Scan_t       ScanParams;
    U32 FStart, FEnd, FStep, loop;
    BOOL connect = FALSE;

    /* Setup Tuner ------------------------- */

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return (Result);
    }
#ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
    #else
        OpenParams.NotifyFunction = NULL;
#endif
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto relocking_end;
    }

    /* Setup scan -------------------------- */

    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Scan to the first frequency, and monitor the events for the feed being (dis)connected");
    
    FStart = FCE_START;
    FEnd   = FCE_END;
    FStep  = FCE_STEP;

    /* Scan -------------------------------- */



    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto relocking_end;
    }

    TUNER_DebugMessage("Scanning...");

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));

    /* Check whether or not we're locked */
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto relocking_end;
    }

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
                      
                    

    /* Lock to a frequency ----------------- */

/*    ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
    ScanParams.FECRates     = TunerInfo.ScanInfo.FECRates;*/
    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
   /* ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;*/
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.Spectrum     = TunerInfo.ScanInfo.Spectrum;

    #ifdef TUNER_370QAM
    ScanParams.J83          = STTUNER_J83_B;
    #else
    ScanParams.J83          = STTUNER_J83_A;
    #endif
    
     #ifdef TUNER_297J
     
        ScanParams.J83   = STTUNER_J83_B;
      
    #endif 
    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
    error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto relocking_end;
    }

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

    /* Check whether or not we're locked */
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto relocking_end;
    }

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));

    /* detect feed status ------------------ */

    for(loop = 1; loop <= RELOCK_TEST_NUMTIMES; loop++)
    {
        TUNER_DebugPrintf(("Pass %d of %d\n", loop, RELOCK_TEST_NUMTIMES));

        if (connect == FALSE)
        {
            TUNER_DebugMessage(("Please DISCONNECT Cable feed\n"));
            ExpectedEvent = STTUNER_EV_UNLOCKED;
            connect = TRUE;
        }
        else
        {
            TUNER_DebugMessage(("Please RECONNECT Cable feed\n" ));
            ExpectedEvent = STTUNER_EV_LOCKED;
            connect = FALSE;
        }

    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
        TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

        if (Event != ExpectedEvent)
        {
            TUNER_TestFailed(Result, "Expected:");
            TUNER_DebugPrintf((" '%s' but got '%s'\n", TUNER_EventToString(ExpectedEvent), TUNER_EventToString(Event) ));
            goto relocking_end;
        }

        /* report on tuner status */
        error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

        TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                           TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                           TunerInfo.Frequency,
                           TunerInfo.ScanInfo.SymbolRate,
                           TunerInfo.SignalQuality,
                           TunerInfo.BitErrorRate,
                           TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                          ));
    }   /* for(loop) */


    /* check freqency will unlock ---------- */

    error = STTUNER_Unlock(TUNERHandle);
    TUNER_DebugError("STTUNER_Unlock()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto relocking_end;
    }

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }

    if (Event != STTUNER_EV_UNLOCKED)
    {
        TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_UNLOCKED'");
    }
    else
    {
        TUNER_TestPassed(Result);
    }

    /* ------------------------------------- */

relocking_end:

    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
}



/*****************************************************************************
TunerScan()
Description:
    Performs a typical run-through of the API calls. e.g., initializes,
    opens, attempts to scans across known frequencies and tidies up afterwards.

    NOTE: The scan is performed in both directions.

Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerScan(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event=STTUNER_EV_UNLOCKED;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FStep, FTmp;
    U32 ChannelCount = 0;
    U8 i;


    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        goto simple_end;
    }
#ifndef ST_OSLINUX 
    /* Setup open params for tuner */
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
#else
        OpenParams.NotifyFunction = NULL;
#endif
    /* Open params setup */
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to open tuner device");
        goto simple_end;
    }

    /* We compile for low/high band as the feed must come from a different cable network. */
    TUNER_DebugPrintf(("%d.0: Scan \n", Params_p->Ref));
    
    FStart = FCE_START;
    FEnd   = FCE_END;
    FStep  = FCE_STEP;
    

    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);

    for (i = 0; i < 2; i++)
    {

        /* initiate scan */
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);
        TUNER_DebugError("STTUNER_Scan()", error);
        if (error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result, "Unable to commence tuner scan");
            goto simple_end;
        }

        TUNER_DebugMessage("Scanning...");
        for (;;)
        {

            /* Wait for notify function to return */
            if (Params_p->Tuner == 0)
            {
                AwaitNotifyFunction1(&Event);
            }
            else
            {
                AwaitNotifyFunction2(&Event);
            }

            if (Event == STTUNER_EV_LOCKED)
            {
                /* Get status information */
                error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
                if (error != ST_NO_ERROR)
                {
                    TUNER_TestFailed(Result, "STTUNER_GetTunerInfo() failed");
                    goto simple_end;
                }

                TUNER_DebugPrintf(("%d %s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                                   ChannelCount,
                                   TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TunerInfo.Frequency,
                                   TunerInfo.ScanInfo.SymbolRate,
                                   TunerInfo.SignalQuality,
                                   TunerInfo.BitErrorRate,
                                   TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                                  ));
                ChannelCount++;
            }
            else if (Event == STTUNER_EV_SCAN_FAILED)
            {
                TUNER_DebugPrintf(("End of Scan\n"));
                break;
            }
            else
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            }

            STTUNER_ScanContinue(TUNERHandle, 0);

        }   /* for() */

        if (ChannelCount > 0)
        {
            TUNER_DebugPrintf(("ChannelCount = %u\n", ChannelCount));
            TUNER_TestPassed(Result);
        }
        else
        {
            TUNER_TestFailed(Result, "no channels found");
        }

        /* Swap direction of scan */
        FTmp   = FStart;
        FStart = FEnd;
        FEnd   = FTmp;
        ChannelCount = 0;

    } /* for */


simple_end:
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);

    return Result;
} /* TunerSimple() */



/*****************************************************************************
TunerScanExact()
Description:
    Test to scan throught the channel table and attempt to lock to each
    frequency.
Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerScanExact(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event=STTUNER_EV_UNLOCKED;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 ChannelCount = 0;
    U32 i;


    if (ChannelTableCount < 1)
    {
        TUNER_TestFailed(Result,"Cannot perform test with no channels found");
        return(Result);
    }
    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }

    /* Setup open params for tuner */
#ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
#else
    OpenParams.NotifyFunction = NULL;
#endif
    /* Open params setup */
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to open tuner device");
        goto tse_end;
    }

    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("Purpose: Scan through stored channel information (%d channels)\n", ChannelTableCount));

    /* Now attempt a set frequency */
    for (i = 0; i < ChannelTableCount; i++)
    {

        STTUNER_SetFrequency(TUNERHandle, ChannelTable[i].F, &ChannelTable[i].ScanInfo, 0);
        if (Params_p->Tuner == 0)
        {
            AwaitNotifyFunction1(&Event);
        }
        else
        {
            AwaitNotifyFunction2(&Event);
        }

        if (Event == STTUNER_EV_LOCKED)
        {
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

            TUNER_DebugPrintf(("%d %s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                               ChannelCount,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.Frequency,
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.SignalQuality,
                               TunerInfo.BitErrorRate,
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                              ));
            ChannelCount++;
        }
        else
        {
            TUNER_DebugPrintf(("%d %s Tried F = %u Hz SR = %u S/s\n",
                               i,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               ChannelTable[i].F,
                               ChannelTable[i].ScanInfo.SymbolRate
                              ));
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        }

    }


    TUNER_DebugPrintf(("Channels = %u / %u\n", ChannelCount, ChannelTableCount));
    if (ChannelCount == ChannelTableCount)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result, "Incorrect number of channels");
    }

    /* If we're currently locked then we should unlock and then catch the
     event to ensure we leave things in a tidy state. */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));

    if (Event == STTUNER_EV_LOCKED)
    {
        /* Ensures we unlock the tuner */
        STTUNER_Unlock(TUNERHandle);
        if (Params_p->Tuner == 0)
        {
            AwaitNotifyFunction1(&Event);
        }
        else
        {
            AwaitNotifyFunction2(&Event);
        }
    }

tse_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);
    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} /* TunerScanExact() */


/*****************************************************************************
TunerTimedScanExact()
Description:
    Test to scan throught the channel table and attempt to lock to each
    frequency, recording the time to complete the task.
Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerTimedScanExact(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t     TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_EventType_t Event=STTUNER_EV_UNLOCKED;
    U32 ChannelCount = 0;
#ifdef ST_OS21
    osclock_t StartClocks, TotalClocks;
#else
    clock_t StartClocks, TotalClocks;
#endif
    U32 i;

    if (ChannelTableCount < 1)
    {
        TUNER_TestFailed(Result,"Cannot perform test with no channels found");
        return(Result);
    }

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }
#ifndef ST_OSLINUX
    /* Setup open params for tuner */
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
#else
    OpenParams.NotifyFunction = NULL;
#endif
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to open tuner device");
        goto ttse_end;
    }

    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugMessage("Purpose: Scan through stored channel information.");
    TUNER_DebugMessage("Starting timer...");

    StartClocks = STOS_time_now();

    /* Now attempt a set frequency */
    for (i = 0; i < ChannelTableCount; i++)
    {
        STTUNER_SetFrequency(TUNERHandle, ChannelTable[i].F, &ChannelTable[i].ScanInfo, 0);
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
        if (Event == STTUNER_EV_LOCKED)
        {
            ChannelCount++;
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        }
    }

    TotalClocks = (1000 * STOS_time_minus(STOS_time_now(), StartClocks)) /  ST_GetClocksPerSecond();
    TUNER_DebugMessage("Stopped timer...");
    TUNER_DebugPrintf(("Channels = %u / %u\n", ChannelCount, ChannelTableCount));
    TUNER_DebugPrintf(("Time taken = %u ms.\n", (int)TotalClocks));

    if (ChannelCount == ChannelTableCount)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result, "Incorrect number of channels");
    }

    /* If we're currently locked then we should unlock and then catch the
     event to ensure we leave things in a tidy state. */
    if (Event == STTUNER_EV_LOCKED)
    {
        /* Ensures we unlock the tuner */
        STTUNER_Unlock(TUNERHandle);
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    }


ttse_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} /* TunerTimedScanExact() */


/*****************************************************************************
TunerSignalChange()
Description:
Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerSignalChange(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event=STTUNER_EV_UNLOCKED;
    STTUNER_Status_t Status;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 i, f;

    if (ChannelTableCount < 1)
    {
        TUNER_TestFailed(Result,"Cannot perform test with no channels found");
        return(Result);
    }

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }

    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to open tuner device");
        goto tsc_end;
    }

    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    /* Setup threshold checking based on CN ratio */
    Thresholds.ThresholdList = ThresholdList;
    Thresholds.NumElements = 3;
    ThresholdList[0].SignalLow  =  0; /* Bad */
    ThresholdList[0].SignalHigh = 80;

    ThresholdList[1].SignalLow  = 80; /* Normal */
    ThresholdList[1].SignalHigh = 88;

    ThresholdList[2].SignalLow  =  88; /* Good */
    ThresholdList[2].SignalHigh = 100;

    error = STTUNER_SetThresholdList(TUNERHandle, &Thresholds, STTUNER_QUALITY_CN);
    TUNER_DebugError("STTUNER_SetThresholdList()", error);

     /* attempt a set frequency */
     #if !defined(TUNER_370QAM)
    TUNER_DebugPrintf(("Selecting F = %u Hz SR = %u S/s %s\n",
                           ChannelTable[0].F,
                           ChannelTable[0].ScanInfo.SymbolRate,
                           TUNER_DemodToString(ChannelTable[0].ScanInfo.Modulation)
                           ));
     #elif defined(TUNER_370QAM)
     TUNER_DebugPrintf(("Selecting F = %u Hz SR = %u S/s %s\n",
                           ChannelTable[0].F,
                           ChannelTable[0].ScanInfo.SymbolRate,
                           TUNER_DemodToString(ChannelTable[0].ScanInfo.Modulation)
                           ));
     #endif                           
    f = 0;
    STTUNER_SetFrequency(TUNERHandle, ChannelTable[f].F, &ChannelTable[f].ScanInfo, 0);
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

    /* Process events */
    for (i = 0; i < 5; i++)
    {
        /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }

        if (Event == STTUNER_EV_SIGNAL_CHANGE)
        {

            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

            TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.Frequency,
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.SignalQuality,
                               TunerInfo.BitErrorRate,
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        }
    }

    /* Setup signal checking based on bit error rate */
    Thresholds.ThresholdList = ThresholdList;
    Thresholds.NumElements = 3;
    ThresholdList[0].SignalLow  =    200; /* Bad */
    ThresholdList[0].SignalHigh = 200000;

    ThresholdList[1].SignalLow  =    100; /* Normal */
    ThresholdList[1].SignalHigh =    200;

    ThresholdList[2].SignalLow  =      0; /* Good */
    ThresholdList[2].SignalHigh =    100;

    /* Setup required threshold list */
    error = STTUNER_SetThresholdList(TUNERHandle, &Thresholds, STTUNER_QUALITY_BER);
    TUNER_DebugError("STTUNER_SetThresholdList()", error);

    /* Process events */
    /* Now attempt a set frequency */
    STTUNER_SetFrequency(TUNERHandle, ChannelTable[f].F, &ChannelTable[f].ScanInfo, 0);
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));

    for (i = 0; i < 5; i++)
    {
        /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }

        if (Event == STTUNER_EV_SIGNAL_CHANGE)
        {

            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

            TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.Frequency,
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.SignalQuality,
                               TunerInfo.BitErrorRate,
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        }
    }

    /* Unlock the tuner */
    STTUNER_GetStatus(TUNERHandle, &Status);
    if (Status == STTUNER_STATUS_LOCKED)
    {
        STTUNER_Unlock(TUNERHandle);
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    }

    /* Disable threshold list */
    Thresholds.NumElements = 0;

    /* Setup required threshold list */
    error = STTUNER_SetThresholdList(TUNERHandle, &Thresholds, STTUNER_QUALITY_CN);
    TUNER_DebugError("STTUNER_SetThresholdList()", error);


tsc_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} /* TunerSignalChange() */


#ifndef ST_OSLINUX
/*****************************************************************************
TunerAPI()
Description:
    This routine aims to test the robustness and validity of the API.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerAPI(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.0: Open without init\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to open the driver without initializing it first.");
    /* ------------------------------------------------------------------ */
    {
        STTUNER_OpenParams_t OpenParams;

        /* Setup open params for tuner */
        if (Params_p->Tuner == 0)
        {
            OpenParams.NotifyFunction = CallbackNotifyFunction1;
        }
        else
        {
            OpenParams.NotifyFunction = CallbackNotifyFunction2;
        }

        /* Open params setup */
        error = STTUNER_Open(TunerDeviceName[Params_p->Tuner],
                             (const STTUNER_OpenParams_t *)&OpenParams,
                             &TUNERHandle);
        TUNER_DebugError("STTUNER_Open()", error);
        if (error != ST_ERROR_UNKNOWN_DEVICE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.1: Term without init\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to term the driver without initializing it first.");
    /* ------------------------------------------------------------------ */
    {
        STTUNER_TermParams_t TermParams;
        TermParams.ForceTerminate = FALSE;

        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);
        if (error != ST_ERROR_BAD_PARAMETER)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.2: Multiple inits\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to init the driver twice with the same name.");
    /* ------------------------------------------------------------------ */
    {
        error = STTUNER_Init(TunerDeviceName[Params_p->Tuner],
                             &TunerInitParams[Params_p->Tuner]);
        TUNER_DebugError("STTUNER_Init()", error);

        if (error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result, "Expected 'ST_NO_ERROR'");
        }

        if (error == ST_NO_ERROR)
        {
            error = STTUNER_Init(TunerDeviceName[Params_p->Tuner],
                                 &TunerInitParams[Params_p->Tuner]);
            TUNER_DebugError("STTUNER_Init()", error);

            if (error != ST_ERROR_BAD_PARAMETER)
            {
                TUNER_TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER'");
            }
            else
            {
                TUNER_TestPassed(Result);
            }
        }
        else
        {
            TUNER_TestFailed(Result, "Expected 'ST_NO_ERROR'");
        }
    }


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.3: Open with invalid device\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to open the driver with an invalid device name.");
    /* ------------------------------------------------------------------ */
    {
        STTUNER_OpenParams_t OpenParams;

        /* Setup open params for tuner */
        if (Params_p->Tuner == 0)
        {
            OpenParams.NotifyFunction = CallbackNotifyFunction1;
        }
        else
        {
            OpenParams.NotifyFunction = CallbackNotifyFunction2;
        }

        /* Open params setup */
        error = STTUNER_Open("invalid",
                             (const STTUNER_OpenParams_t *)&OpenParams,
                             &TUNERHandle);
        TUNER_DebugError("STTUNER_Open()", error);
        if (error != ST_ERROR_UNKNOWN_DEVICE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.4: Multiple opens\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to open the device twice with the same name");
    /* ------------------------------------------------------------------ */
    {
        STTUNER_OpenParams_t OpenParams;

        /* Setup open params for tuner */
        if (Params_p->Tuner == 0)
        {
            OpenParams.NotifyFunction = CallbackNotifyFunction1;
        }
        else
        {
            OpenParams.NotifyFunction = CallbackNotifyFunction2;
        }

        /* Open params setup */
        error = STTUNER_Open(TunerDeviceName[Params_p->Tuner],
                             (const STTUNER_OpenParams_t *)&OpenParams,
                             &TUNERHandle);
        TUNER_DebugError("STTUNER_Open()", error);

        TUNER_DebugPrintf(("Tuner handle = 0x%08x\n", TUNERHandle));
        /* ASSUME: 0x00068000 == (STTUNER_DRIVER_BASE | 0x8000) */
        if ( (TUNERHandle & 0xFFFFFF00) != 0x00068000)
        {
            TUNER_TestFailed(Result, "Expected handle to be of the form 0x000680nn");
        }

        if (error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result, "Expected 'ST_NO_ERROR'");
        }
        else
        {
            /* Open params setup */
            error = STTUNER_Open(TunerDeviceName[Params_p->Tuner],
                                 (const STTUNER_OpenParams_t *)&OpenParams,
                                 &TUNERHandle);
            TUNER_DebugError("STTUNER_Open()", error);

            if (error != ST_ERROR_OPEN_HANDLE)
            {
                TUNER_TestFailed(Result, "Expected 'ST_ERROR_OPEN_HANDLE'");
            }
            else
            {
                TUNER_TestPassed(Result);
            }
        }
    }


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.5: Invalid handle tests\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to call API with a known invalid handle.");
    /* ------------------------------------------------------------------ */
    {   /* Try various API calls with an invalid handle */
        error = STTUNER_Close(STTUNER_MAX_HANDLES);
        TUNER_DebugError("STTUNER_Close()", error);

        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, 0, 0, NULL, NULL);
        TUNER_DebugError("STTUNER_Ioctl()", error);

        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_Scan(STTUNER_MAX_HANDLES, 0, 0, 0, 0);
        TUNER_DebugError("STTUNER_Scan()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_ScanContinue(STTUNER_MAX_HANDLES, 0);
        TUNER_DebugError("STTUNER_ScanContinue()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        #ifdef ST_OSLINUX
        error = STTUNER_SetBandList(STTUNER_MAX_HANDLES, &BandList);
        #else
        error = STTUNER_SetBandList(STTUNER_MAX_HANDLES, NULL);
        #endif
        TUNER_DebugError("STTUNER_SetBandList()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        #ifdef ST_OSLINUX
	error = STTUNER_SetScanList(STTUNER_MAX_HANDLES, &ScanList);
#else    	
        error = STTUNER_SetScanList(STTUNER_MAX_HANDLES, NULL);
#endif    
        TUNER_DebugError("STTUNER_SetScanList()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_GetStatus(STTUNER_MAX_HANDLES, NULL);
        TUNER_DebugError("STTUNER_GetStatus()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_GetTunerInfo(STTUNER_MAX_HANDLES, NULL);
        TUNER_DebugError("STTUNER_GetTunerInfo()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, NULL);
        TUNER_DebugError("STTUNER_GetBandList()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, NULL);
        TUNER_DebugError("STTUNER_GetScanList()", error);
        if (error != ST_ERROR_INVALID_HANDLE)
        {
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }


    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.6: Get capability test\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt to call this API with a known invalid device name.");
    /* ------------------------------------------------------------------ */
    {
        error = STTUNER_GetCapability("invalid", NULL);
        TUNER_DebugError("STTUNER_GetCapability()", error);
        /* ST_ERROR_UNKNOWN_DEVICE if pointer checking turned off else STTUNER_ERROR_POINTER*/
        if ((error != STTUNER_ERROR_POINTER) && (error != ST_ERROR_UNKNOWN_DEVICE))
        {
            TUNER_TestFailed(Result, "Expected 'STTUNER_ERROR_POINTER'");
        }
        else
        {
            TUNER_TestPassed(Result);
        }
    }

    {
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);
    }

    {
        STTUNER_TermParams_t TermParams;
        TermParams.ForceTerminate = FALSE;

        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);
    }

    /* ------------------------------------------------------------------ */
    TUNER_DebugPrintf(("%d.7: init->open->set bands->close->term tuner 10 times\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Attempt a normal operation multiple times");
    /* ------------------------------------------------------------------ */
    {
#ifndef ST_OSLINUX    	
#ifdef ST_OS21
        #include <os21/partition.h>
#else
        #include <partitio.h>
#endif       
#endif /*ST_OSLINUX*/    
        #define ABS(X) ((X)<0 ? (-X) : (X))
        #define NUMLOOPS 10
        STTUNER_TermParams_t TermParams;
        STTUNER_OpenParams_t OpenParams;
        int loop;
        BOOL failed = FALSE;
       #ifndef ST_OSLINUX	
        S32                   retval,before,after;         
        partition_status_t       status;
        partition_status_flags_t flags=0;
#endif  

        /* setup parameters */
        #ifndef ST_OSLINUX	        
        if (Params_p->Tuner == 0) OpenParams.NotifyFunction = CallbackNotifyFunction1;
         else OpenParams.NotifyFunction = CallbackNotifyFunction2;
        TermParams.ForceTerminate = FALSE;
#else
        OpenParams.NotifyFunction = NULL;
#endif        

#ifndef ST_OSLINUX	
        retval = partition_status(system_partition, &status, flags);
        before = status.partition_status_free;
        TUNER_DebugPrintf(("dynamic memory free before test: %d bytes\n", before));
#endif           
        for(loop = 1; loop <= NUMLOOPS; loop++)
        {
              TUNER_DebugPrintf((" --- cycle #%d ---\n", loop));

            /* Init */
            error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
              TUNER_DebugError("STTUNER_Init()", error);
            if (error != ST_NO_ERROR) failed = TRUE;

            /* Open */
            error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *)&OpenParams, &TUNERHandle);
            TUNER_DebugError("STTUNER_Open()", error);
            if (error != ST_NO_ERROR) failed = TRUE;

            /* Setup scan and band lists */
            STTUNER_SetBandList(TUNERHandle, &Bands);
            TUNER_DebugError("STTUNER_SetBandList()", error);
            if (error != ST_NO_ERROR) failed = TRUE;

            STTUNER_SetScanList(TUNERHandle, &Scans);
            TUNER_DebugError("STTUNER_SetScanList()", error);
            if (error != ST_NO_ERROR) failed = TRUE;

            error = STTUNER_Close(TUNERHandle);
            TUNER_DebugError("STTUNER_Close()", error);
            if (error != ST_NO_ERROR) failed = TRUE;

            error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
            TUNER_DebugError("STTUNER_Term()", error);
            if (error != ST_NO_ERROR) failed = TRUE;

        }   /* for(loop) */

      #ifndef ST_OSLINUX
        retval = partition_status(system_partition, &status, flags);
        after = status.partition_status_free;
        TUNER_DebugPrintf(("dynamic memory free after test: %d bytes\n", after));
#endif   
        if (failed == FALSE)
        {
            TUNER_TestPassed(Result);
        }
        else
        {
            TUNER_TestFailed(Result, "Expected 'ST_NO_ERROR' from all functions");
        }
#ifndef ST_OSLINUX
        if (before == after)
        {
            TUNER_DebugPrintf(( "No memory leaks detected\n"));
            TUNER_TestPassed(Result);
        }
        else
        {
            TUNER_TestFailed(Result, "Memory leak detected");
            TUNER_DebugPrintf(( "Leak of %d bytes, or %d bytes per test\n", ABS(before - after),  ABS(before - after) / NUMLOOPS ));
        }
#endif
    }

    /* ------------------------------------------------------------------ */


    return Result;
} /* TunerAPI() */
#endif/*ST_OSLINUX*/
/*****************************************************************************
TunerTracking()
Description:
    This routine scans to the first frequency.  We test 'tracking' of the
    frequency by waiting for a reasonable amount of time.
    We then call the abort routine to ensure we are notified of the
    unlocking of the tuner.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerTracking(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event=STTUNER_EV_UNLOCKED;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FStep, i;
    STTUNER_Scan_t       ScanParams;

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }
#ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
    #else
        OpenParams.NotifyFunction = NULL;
#endif  
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *)&OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }

    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Scan to the first frequency lock and let tracking run for a reasonable period.");
    
    FStart = FCE_START;
    FEnd   = FCE_END;
    FStep  = FCE_STEP;
     
    /* Now attempt a scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);
    TUNER_DebugError("STTUNER_Scan()", error);

    if (error == ST_NO_ERROR)
    {
        TUNER_DebugMessage("Scanning...");

        /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }

        /* Check whether or not we're locked */
        if (Event == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

            TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.Frequency,
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.SignalQuality,
                               TunerInfo.BitErrorRate,
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
            goto tracking_end;
        }
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }

	/* Lock to a frequency ----------------- */

    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.Spectrum     = TunerInfo.ScanInfo.Spectrum;

    #ifdef TUNER_370QAM
    ScanParams.J83          = STTUNER_J83_B;
    #else
    ScanParams.J83          = STTUNER_J83_A;
    #endif
    
     #ifdef TUNER_297J
     
        ScanParams.J83   = STTUNER_J83_B;
      
    #endif 
    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
    error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

    /* Check whether or not we're locked */
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto tracking_end;
    }


    if (Event == STTUNER_EV_LOCKED)
    {
        /* Begin polling driver for tracking tests... */
        TUNER_DebugMessage("Polling the tuner status -- ensure lock is kept...");

       for (i = 0; i < TUNER_TRACKING; i++)
        {
            STTUNER_TaskDelay(ST_GetClocksPerSecond());

            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);

            if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                goto tracking_end;
            }

            TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.Frequency,
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.SignalQuality,
                               TunerInfo.BitErrorRate,
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                              ));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                /*goto tracking_end;*/
            }
        }
    }
    else
    {
        TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
    }

    /* Abort the current scan */
    error = STTUNER_Unlock(TUNERHandle);
    TUNER_DebugError("STTUNER_Unlock()", error);

    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }

    if (Event != STTUNER_EV_UNLOCKED)
    {
        TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_UNLOCKED'");
    }
    else
    {
        TUNER_TestPassed(Result);
    }

tracking_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
}

/*****************************************************************************
TunerTimedScan()
Description:
    This routine performs a full scan test and records the time taken for
    it to complete.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerTimedScan(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    STTUNER_Handle_t     TUNERHandle;
    STTUNER_EventType_t  Event=STTUNER_EV_UNLOCKED;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_TunerInfo_t  TunerInfo;
    ST_ErrorCode_t error;
#ifdef ST_OS21
    osclock_t Time;
#else
    clock_t Time;
#endif
    U32 ChannelCount = 0;
    U32 FStart, FEnd, FStep;

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }
#ifndef ST_OSLINUX

    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
    #else
        OpenParams.NotifyFunction = NULL;
#endif    
    /* Open params setup */
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto timing_end;
    }

    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Timing test\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Time to scan");

    ChannelCount = 0;
    
    FStart = FCE_START;
    FEnd   = FCE_END;
    FStep  = FCE_STEP;
    

    /* Now attempt a scan */
    TUNER_DebugMessage("Commencing scan");

    Time = STOS_time_now();
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);

    for (;;)
    {
        if (Params_p->Tuner == 0)
        {
            AwaitNotifyFunction1(&Event);
        }
        else
        {
            AwaitNotifyFunction2(&Event);
        }

        if (Event == STTUNER_EV_LOCKED)
        {

            STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

            /* Store channel information in channel table */
            ChannelTable[ChannelCount].F = TunerInfo.Frequency;
            ChannelTable[ChannelCount].ScanInfo = TunerInfo.ScanInfo;

            /* Increment channel count */
            ChannelCount++;
        }
        else if (Event == STTUNER_EV_SCAN_FAILED)
        {
            break;
        }
        else
        {
            TUNER_DebugPrintf(("Unexpected event (%s)\n", TUNER_EventToString(Event)));
        }
        /* Next scan step */
        STTUNER_ScanContinue(TUNERHandle, 0);
    }

    Time = (1000 * STOS_time_minus(STOS_time_now(), Time)) / ST_GetClocksPerSecond();
    ChannelTableCount = ChannelCount;

    /* Delay for a while and ensure we do not receive */
    TUNER_DebugPrintf(("Time taken = %u ms to scan band for %u channels.\n", (int)Time, ChannelCount));

    if (ChannelTableCount > 0)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"No channels found");
    }

timing_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} /* TunerTiming() */





/*****************************************************************************
TunerStandByMode()
Description:
   This routine first checks the Standby mode . Then call scan function and tune to one of the locked 
    frequency in normal mode.Then the device goes in standby mode.Again tune function is called to see the
    effect . After this make StandMode off and checks whether the locked is maintained or not. 
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerStandByMode(TUNER_TestParams_t *Params_p)
{	
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart,FEnd,FStep;
   
    STTUNER_Scan_t       ScanParams;
   
    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }

    #ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
    #else
        OpenParams.NotifyFunction = NULL;
#endif   
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *)&OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto standby_end;
    }
  
 
    /************ Test the StandByMode just after open **********************/
    error=STTUNER_StandByMode(TUNERHandle,STTUNER_STANDBY_POWER_MODE);
    
    if (error == ST_ERROR_FEATURE_NOT_SUPPORTED)
    {
      TUNER_DebugMessage("Result: ST_ERROR_FEATURE_NOT_SUPPORTED");
      goto standby_end;
    }
    
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto standby_end;
    }
    else
    {
       TUNER_DebugPrintf(("\n\t\t***** StandByMode is switched ON ***** \n"));
    }
    
   
   error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    
     if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
  TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));
   	
    
   
    
    error=STTUNER_StandByMode(TUNERHandle,STTUNER_NORMAL_POWER_MODE);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
      goto standby_end;
    }
    else
    {
       TUNER_DebugPrintf(("\n\t\t***** StandByMode is switched OFF *****\n"));
    }
    
   error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    
     if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
  TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));
  
   
    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Scan to the first frequency, and monitor the events for the feed being (dis)connected");
    
    FStart = FCE_START;
    FEnd   = FCE_END;
    FStep  = FCE_STEP;
    	
    /* Scan -------------------------------- */

    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }

    TUNER_DebugMessage("Scanning...");

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));

    /* Check whether or not we're locked */
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto standby_end;
    }

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
   
    TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));
    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
                      
                      

    /* Lock to a frequency ----------------- */


    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.Spectrum     = TunerInfo.ScanInfo.Spectrum;
   
    #ifdef TUNER_370QAM
    ScanParams.J83          = STTUNER_J83_B;
    #else
    ScanParams.J83          = STTUNER_J83_A;
    #endif
    
     #ifdef TUNER_297J
     
        ScanParams.J83   = STTUNER_J83_B;
      
    #endif 
    TUNER_DebugMessage("Tuning...");

    
    error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error == ST_NO_ERROR)
    {
        TUNER_DebugMessage("Scanning...");

        /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

    /* Check whether or not we're locked */
    if (Event == STTUNER_EV_LOCKED)
    {
        
     /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
      }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
            goto standby_end;
        }
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
    
                             /*Trying to lock in STANDBY mode*/
    
        
    error=STTUNER_StandByMode(TUNERHandle,STTUNER_STANDBY_POWER_MODE);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
    else
    {
         TUNER_DebugPrintf(("\n\t\t***** StandByMode is switched ON *****\n"));
    }
  
   
    
    /************Again calling Tunning function**********************************/
    
   
    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.Spectrum     = TunerInfo.ScanInfo.Spectrum;
   
    #ifdef TUNER_370QAM
    ScanParams.J83          = STTUNER_J83_B;
    #else
    ScanParams.J83          = STTUNER_J83_A;
    #endif
     #ifdef TUNER_297J
     
        ScanParams.J83   = STTUNER_J83_B;
      
    #endif 
    TUNER_DebugMessage("Tuning...");

    
    error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error == ST_NO_ERROR)
    {
        TUNER_DebugMessage("Scanning...");

        /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

    /* Check whether or not we're locked */
    if (Event == STTUNER_EV_LOCKED)
    {
        
     /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
      }
       
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
    if (Event != STTUNER_EV_LOCKED)
    {
       
       TUNER_DebugPrintf((" \n Timeout occured as scan task is suspended during Standby Mode \n"));   
        
    }
    else
    {
    	TUNER_DebugPrintf(("\n ***** Standby Mode not working properly ***** \n "));
        TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_TIMEOUT'");
       /* goto standby_end;*/
    }
    /************************************************************************/
   
    
    error=STTUNER_StandByMode(TUNERHandle,STTUNER_NORMAL_POWER_MODE);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
    else
    {
       TUNER_DebugPrintf(("\n\t\t***** StandByMode is switched OFF *****\n"));
    }
    
     error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    
     if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
   
 TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));
    
      
    
     /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

    /* Check whether or not we're locked */
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto standby_end;
    }
    else
    {    
    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    TUNER_DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));

    }
    
standby_end:    

    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);

 
} /* end of TunerStandbyMode */
	



/*****************************************************************************
TunerTerm()
Description:
    Attempts to terminate the tuner during a scan.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerTerm(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    STTUNER_Handle_t TUNERHandle;
    ST_ErrorCode_t error;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FStep;
    STTUNER_EventType_t Event;
    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return Result;
    }
#ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction1;
    }
    else
    {
        OpenParams.NotifyFunction = CallbackNotifyFunction2;
    }
    #else
        OpenParams.NotifyFunction = NULL;
#endif   
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *)&OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result, "Expected = 'ST_NO_ERROR'");
        return Result;
    }

    TUNER_DebugPrintf(("%d.0: Terminate during scan\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Ensure driver terminates cleanly when scanning");

    FStart = FCE_START;
    FEnd   = FCE_END;
    FStep  = FCE_STEP;

    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    /* Now attempt a scan */
    TUNER_DebugMessage("Commencing scan...");
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error == ST_NO_ERROR)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
    }

    TUNER_DebugPrintf(("%d.2: Terminate during scan\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Ensure driver terminates cleanly when scanning");

    /* Setup scan and band lists */
    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);

    /* attempt another scan */
    TUNER_DebugMessage("Commencing scan...");
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStep, 0);
    TUNER_DebugError("STTUNER_Scan()", error);

    if (error == ST_NO_ERROR)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
    }

    TUNER_DebugMessage("Forcing termination...");
    TermParams.ForceTerminate = TRUE;

    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
    TUNER_DebugError("STTUNER_Term()", error);

    if (error == ST_NO_ERROR)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
    }


    return(Result);

} /* TunerTerm() */



/* ------------------------------------------------------------------------ */
/* Debug routines --------------------------------------------------------- */
/* ------------------------------------------------------------------------ */


char *TUNER_ErrorString(ST_ErrorCode_t Error)
{
    return STAPI_ErrorMessages(Error);
}

/* ------------------------------------------------------------------------ */

char *TUNER_FecToString(STTUNER_FECRate_t Fec)
{
    int i;
    static char FecRate[][25] =
    {
        "1_2",
        "2_3",
        "3_4",
        "5_6",
        "5_6",
        "6_7",
        "7_8",
        "8_9"
    };

    for (i = 0; Fec > 0; i++, Fec>>=1);
    return FecRate[i-1];
}

/* ------------------------------------------------------------------------ */

char *TUNER_DemodToString(STTUNER_Modulation_t Mod)
{
    int i;
    static char *Modulation[] =
    {
        "QPSK\0",     /* 0 */
        "8PSK\0",
        "QAM\0",
        "4QAM\0",
        "16QAM\0",
        "32QAM\0",
        "64QAM\0",
        "128QAM\0",
        "256QAM\0",
        "BPSK\0"      /* 9 */
        "NONE\0",
        "ALL\0",
        "!!INVALID MODULATION!!\0"
    };

    if (Mod == STTUNER_MOD_NONE) return Modulation[10];
    if (Mod == STTUNER_MOD_ALL)  return Modulation[11];
    if (Mod > STTUNER_MOD_BPSK)  return Modulation[12];

    for (i = 0; Mod > 0; i++, Mod>>=1);
    return Modulation[i-1];
}

/* ------------------------------------------------------------------------ */

char *TUNER_PolarityToString(STTUNER_Polarization_t Plr)
{
    static char *Polarity[]= { "HORIZONTAL", "VERTICAL",
                               "ALL(H+V)",   "!!INVALID POLARITY!!" };

    if (Plr > STTUNER_PLR_ALL) return Polarity[3];
    return Polarity[Plr-1];
}

/* ------------------------------------------------------------------------ */

char *TUNER_ToneToString(STTUNER_LNBToneState_t ToneState)
{
    switch(ToneState)
    {
        case STTUNER_LNB_TONE_DEFAULT:
            return("Default");

        case STTUNER_LNB_TONE_OFF:
            return("OFF");

        case STTUNER_LNB_TONE_22KHZ:
            return("22KHZ");

        default:
            break;
    }
    return("!!UNKNOWN!!");
}

/* ------------------------------------------------------------------------ */

char *TUNER_EventToString(STTUNER_EventType_t Evt)
{
    static int i = 0;
    static char c[NUM_TUNER][80];

    switch(Evt)
    {
        case STTUNER_EV_NO_OPERATION:
            return("STTUNER_EV_NO_OPERATION");

        case STTUNER_EV_LOCKED:
            return("STTUNER_EV_LOCKED");

        case STTUNER_EV_UNLOCKED:
            return("STTUNER_EV_UNLOCKED");

        case STTUNER_EV_SCAN_FAILED:
            return("STTUNER_EV_SCAN_FAILED");

        case STTUNER_EV_TIMEOUT:
            return("STTUNER_EV_TIMEOUT");

        case STTUNER_EV_SIGNAL_CHANGE:
            return("STTUNER_EV_SIGNAL_CHANGE");

        case STTUNER_EV_SCAN_NEXT:
            return("STTUNER_EV_SCAN_NEXT");

        default:
                break;
    }
    i++;
    if (i >= NUM_TUNER) i = 0;
    sprintf(c[i], "NOT_STTUNER_EVENT(%d)", Evt);
    return(c[i]);
}

/* ------------------------------------------------------------------------ */

char *TUNER_AGCToPowerOutput(S32 Agc)
{
    static char buf[10];

    #ifdef TUNER_297E
    sprintf(buf, "%d", Agc);
	return buf;
	#else
    sprintf(buf, "%5d.", -(127 - (Agc / 2)));
    return strcat(buf, (Agc % 2) ? "0" : "5" );
    #endif
}

/* ------------------------------------------------------------------------ */

static char *STAPI_ErrorMessages(ST_ErrorCode_t ErrCode)
{
    switch(ErrCode)
    {
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

#ifndef ST_OSLINUX
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


        /* sttbx.h */
        case STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED:
            return("STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED");
#endif

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


        /* sttuner.h */
        case STTUNER_ERROR_UNSPECIFIED:
            return("STTUNER_ERROR_UNSPECIFIED");

        case STTUNER_ERROR_HWFAIL:
            return("STTUNER_ERROR_HWFAIL");

        case STTUNER_ERROR_EVT_REGISTER:
            return("STTUNER_ERROR_EVT_REGISTER");

        case STTUNER_ERROR_ID:
            return("STTUNER_ERROR_ID");

        case STTUNER_ERROR_POINTER:
            return("STTUNER_ERROR_POINTER");

        case STTUNER_ERROR_INITSTATE:
            return("STTUNER_ERROR_INITSTATE");

        case STTUNER_ERROR_END_OF_SCAN:
            return("STTUNER_ERROR_END_OF_SCAN");


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


        case ST_NO_ERROR:
            return("ST_NO_ERROR");

        default:
            break;
    }

    return("UNKNOWN ERROR");
}    /* STAPI_ErrorMessages */

/* ------------------------------------------------------------------------ */
/* EOF */
