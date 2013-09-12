/* -------------------------------------------------------------------------

File Name   : sat_test.c

Description : Test harness for the STTUNER driver for STi55xx.

Copyright (C) 2000 STMicroelectronics

Revision History    :

   19/04/2000   Added support for STi5508.
   08/08/2001   Modifications to support STTUNER 3.1.0 (new multi-instance version).
   17/08/2001   STTUNER_Ioctl() tests added.
   20/08/2001   STV0399 support added.
   10/09/2001   change name from tuner_test.c to sat_test.c
   24/10/2001   add more error code strings.
   27/11/2001   Support 399 & Mediaref plus added memory leak tests.
   21/12/2001   TunerAPI function change (STTUNER_Open() can now return ST_ERROR_OPEN_HANDLE
                for an already opened handle (e.g. when trying to open the same device again)
                TunerAPI now checks handle signature.
   1/May/2002   Tests added for IQ mode specification and terminate search improvements.
  10/June/2002  Added TunerFecMode() test for DTV with dual stem support, note that this may
                invalidate old EVAL0299 boards with modified (to work under DTV) tuner data cables,
                just use a standard unmodified cable instead, also if using STPTI set
                'STPTI_InitParams.DiscardSyncByte = TRUE' for DTV to get data through.
  19/June/2002  Change so 299eval and 299STEM can be tested using test harness.
  13/Nov/2003   Test harness also supports STi5517,STi5518,STi5528(mb376 & espresso boards).
                LLA revision is reported. Support for lnb21 added. Some tests modified.
  23/Mar/2004   Support for running on 5100.
  06/04/2004(SD) TunerInitParams for SatCR added. Now two scanlists are defined for SatCR to get 
                 two SCRBPF bands with different transponder signal.
  15/03/2005    Support added for running on 5301. Option to bypass STTBX for output by setting DISABLE_TOOLBOX=1

Reference:
ST API Definition "TUNER Driver API" DVD-API-06

Note : 15/02/2006 Sushil Dutt
	Linux integration.

---------------------------------------------------------------------------- */

/* Includes --------------------------------------------------------------- */

#include <stdio.h>              /* C libs */
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

#include "stpio.h"
#include "stcommon.h"
#include "stevt.h"

#ifdef ST_OS21
    #include "os21debug.h"
#endif
#ifdef ST_OS20
    #include "debug.h"
#endif

#include "../tuner_test.h"         /* common test includes */
#include "../diseqc_app.h"
/* Private types/constants ------------------------------------------------ */

/* hardware in system */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER

#if defined(ST_5518)
#define NUM_PIO     6
#elif defined(ST_5105)|| defined(ST_5188) || defined(ST_5107)
#define NUM_PIO     4
#else
#define NUM_PIO     5/*6*/
#endif
#endif /* USE_STI2C_WRAPPER */



#if defined(ST_5105)
	#define NUM_I2C     1
#else
	#define NUM_I2C     2
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
#define ST_I2C_CLOCKFREQUENCY   70000000
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
#elif defined(ST_5105) || defined(ST_5107)
#define ST_I2C_CLOCKFREQUENCY   85000000
#elif defined(ST_5188)
#define ST_I2C_CLOCKFREQUENCY   100000000
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
} TUNER_ProbeParams_t;


#ifdef ST_OSLINUX
#define debugmessage(args) 	printf(args)
#endif

/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER

/* Enum of PIO devices */
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    #if defined(ST_5518) ||  defined(ST_7100)
    PIO_DEVICE_5,
    #endif
    PIO_DEVICE_NOT_USED
};

#define PIO_BIT_NOT_USED    0

#endif /*USE_STI2C_WRAPPER*/
/* Enum of I2C devices */
enum
{
    I2C_DEVICE_0,
    I2C_DEVICE_1,
    I2C_DEVICE_NOT_USED
};

#ifndef ST_OSLINUX
#ifdef ST_5100
#define PIO_4_BASE_ADDRESS	ST5100_PIO4_BASE_ADDRESS
#define PIO_4_INTERRUPT		ST5100_PIO4_INTERRUPT
#endif
#endif /*ST_OSLINUX*/

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105)|| defined(ST_5107)|| defined(ST_5188) || defined(ST_7100) || defined(ST_7109)|| defined(ST_5301) || defined(ST_8010)
 #define BACK_I2C_BASE  SSC_0_BASE_ADDRESS
 #define FRONT_I2C_BASE SSC_1_BASE_ADDRESS
#endif

/* I2C device map */
#if defined(ST_5508) || defined(ST_5518)
 #define I2C_DEVICE_BACKBUS  I2C_DEVICE_0
#if defined(ST_5518)
 #define I2C_DEVICE_FRONTBUS I2C_DEVICE_1
#else
 #define I2C_DEVICE_FRONTBUS I2C_DEVICE_NOT_USED
#endif
#else
 #define I2C_DEVICE_BACKBUS  I2C_DEVICE_0
 #define I2C_DEVICE_FRONTBUS I2C_DEVICE_1
#endif

/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
/* PIO pins used for I2C */

#if defined(ST_5508) || defined(ST_5518)
 #define I2C_0_CLK_DEV       PIO_DEVICE_1
 #define I2C_0_CLK_BIT       PIO_BIT_1
 #define I2C_0_DATA_DEV      PIO_DEVICE_1
 #define I2C_0_DATA_BIT      PIO_BIT_0
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL

#if defined(ST_5518)
 #define I2C_1_CLK_DEV       PIO_DEVICE_5
 #define I2C_1_CLK_BIT       PIO_BIT_1
 #define I2C_1_DATA_DEV      PIO_DEVICE_5
 #define I2C_1_DATA_BIT      PIO_BIT_0
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_1_INTERRUPT_LEVEL
#else
 #define I2C_1_CLK_DEV       PIO_DEVICE_NOT_USED
 #define I2C_1_CLK_BIT       PIO_BIT_NOT_USED
 #define I2C_1_DATA_DEV      PIO_DEVICE_NOT_USED
 #define I2C_1_DATA_BIT      PIO_BIT_NOT_USED
 #define I2C_1_BASE_ADDRESS  0
 #define I2C_1_INT_NUMBER    0
 #define I2C_1_INT_LEVEL     0
#endif
 
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_5107)|| defined(ST_5301) || defined(ST_8010)
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
 
#elif defined(ST_5188)

 #define I2C_0_CLK_DEV       PIO_DEVICE_0
 #define I2C_0_DATA_DEV      PIO_DEVICE_0
 #define I2C_0_CLK_BIT       PIO_BIT_5
 #define I2C_0_DATA_BIT      PIO_BIT_6
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL


 #define I2C_1_CLK_DEV       PIO_DEVICE_0
 #define I2C_1_DATA_DEV      PIO_DEVICE_0
 #define I2C_1_CLK_BIT       PIO_BIT_2
 #define I2C_1_DATA_BIT		PIO_BIT_3
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_1_INTERRUPT_LEVEL
 /*#define I2C_1_BASE_ADDRESS  (U32 *)PIO_0_BASE_ADDRESS
 #define I2C_1_INT_NUMBER    PIO_0_INTERRUPT
 #define I2C_1_INT_LEVEL     PIO_0_INTERRUPT_LEVEL*/

#elif defined(ST_7710) || defined(ST_7100)|| defined(ST_7109)
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
/*#endif*/ /*ST_OSLINUX*/
#endif /*USE_STI2C_WRAPPER*/

#if defined(ST_5518)
/* mb5518 board */
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

#elif defined(ST_5508)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

#elif defined(espresso) || defined(mb390)|| defined(ST_7710) || defined(ST_7100)|| defined(ST_7109) || defined(ST_5301) || defined(ST_8010)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS
 
#elif defined(mb395)|| defined(ST_5188)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_FRONTBUS
 
#elif defined(maly3s) || defined(mb400)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

/* presume 5514 (mediaref) tuners are on a common bus */
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)
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


/* Special setup for For Mediaref 5514 with 299 */
/* physical: 399s on front I2C bus (motherboard)
             299  on  back I2C bus (connected via Packet Injector STEM board i/p TS B (CN15&16)) */
#ifdef MREF_STEM_299
 #undef TUNER0_DEMOD_I2C
 #undef TUNER0_TUNER_I2C
 #undef TUNER1_DEMOD_I2C
 #undef TUNER1_TUNER_I2C

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS
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


#ifdef TUNER_399
 #define DUT_DEMOD STTUNER_DEMOD_STV0399
 #define DUT_TUNER STTUNER_TUNER_NONE
 #define DUT_LNB   STTUNER_LNB_DEMODIO
 static const char ChipName[] = "STV0399";
#elif defined TUNER_299
 #define DUT_DEMOD STTUNER_DEMOD_STV0299
 #define DUT_TUNER STTUNER_TUNER_VG1011
 #define DUT_LNB   STTUNER_LNB_DEMODIO
 static const char ChipName[] = "STV0299";
#elif defined TUNER_288
 #define DUT_DEMOD STTUNER_DEMOD_STX0288
 #define DUT_TUNER STTUNER_TUNER_VG1011
 #define DUT_LNB   STTUNER_LNB_DEMODIO
 static const char ChipName[] = "STX0288";
 #elif defined TUNER_289
 #define DUT_DEMOD STTUNER_DEMOD_STB0289
 #define DUT_TUNER STTUNER_TUNER_STB6000
 #define DUT_LNB   STTUNER_LNB_DEMODIO
 static const char ChipName[] = "STB0289";
#elif defined TUNER_899
 #define DUT_DEMOD STTUNER_DEMOD_STB0899
 #define DUT_TUNER STTUNER_TUNER_STB6100
 #define DUT_LNB   STTUNER_LNB_LNBH21
 static const char ChipName[] = "STB0899";

#else

 #define DUT_DEMOD STTUNER_DEMOD_STV0299
 #define DUT_TUNER STTUNER_TUNER_VG1011
 #define DUT_LNB   STTUNER_LNB_DEMODIO
 static const char ChipName[] = "STV0299";

#endif



#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif

/* frequency limits for scanning */

#ifdef  C_BAND_TESTING

	#ifdef SAT_TEST_SCR
		#define FLIMIT_HI    3705000
	#else
		#define FLIMIT_HI    3684000
	#endif
	
	#define FLIMIT_LO    3680000
#else /* Ku Band is default */
	#define FLIMIT_HI   12400000
	#define FLIMIT_LO   12390000
	
#endif


/* Change tuner scan step from default (calculated) value,
   bigger value == faster scanning, smaller value == more channels picked up */

#ifdef C_BAND_TESTING
#define FIXED_TUNER_SCAN_STEP  10000  /* <-- best for STV0399. 7000 5000 <3000> 2500 1000 0(default) */
#else /* Ku band is default */
#define FIXED_TUNER_SCAN_STEP   3000  /* <-- best for STV0399. 7000 5000 <3000> 2500 1000 0(default) */
#endif

/*#define TEST_DUAL 1*/
#if NUM_TUNER == 2
  #define TEST_DUAL 1
#endif 

#ifndef ST_OSLINUX
/* Private variables ------------------------------------------------------ */

#if defined(ST_5105)|| defined(ST_5188) || defined(ST_5107)
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
#endif /*ST_OSLINUX*/
/* Test harness revision number */
static U8 Revision[] = "5.6.0";

/*For displaying the Platform & Backend in use*/
static char Platform[20];
static char Backend[20];

/* Tuner device name -- used for all tests */
static ST_DeviceName_t TunerDeviceName[NUM_TUNER+1];
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
/* List of PIO device names */
static ST_DeviceName_t PioDeviceName[NUM_PIO+1];
/*#endif*/ /*ST_OSLINUX*/
#endif  /*USE_STI2C_WRAPPER*/

/* List of I2C device names */
static ST_DeviceName_t I2CDeviceName[NUM_I2C+1];

#ifndef ST_OSLINUX
#ifndef USE_STI2C_WRAPPER
/* PIO Init parameters */
static STPIO_InitParams_t PioInitParams[NUM_PIO];
#endif /*ST_OSLINUX*/
#endif  /*USE_STI2C_WRAPPER*/

/* I2C Init params */
static STI2C_InitParams_t I2CInitParams[NUM_I2C];

/* TUNER Init parameters */
static STTUNER_InitParams_t TunerInitParams[NUM_TUNER];

/* Global semaphore used in callbacks */


static semaphore_t *EventSemaphore1, *EventSemaphore2;

static STTUNER_EventType_t LastEvent1, LastEvent2;


/* Stored table of channels */
#ifndef STTUNER_MINIDRIVER
static TUNER_Channel_t ChannelTable[TUNER_TABLE_SIZE];

#endif

static U32 ChannelTableCount;

#if defined(TUNER_288) || defined(TUNER_289)
static U32 BlindScan_ChannelTableCount;
static TUNER_Channel_t BlindScan_ChannelTable[TUNER_TABLE_SIZE];
#endif

/* Declarations for memory partitions */
#ifndef ST_OSLINUX
#ifndef ST_OS21
/* Allow room for OS20 segments in internal memory */
static unsigned char    internal_block [ST20_INTERNAL_MEMORY_SIZE-1200];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif


#define                 NCACHE_MEMORY_SIZE          0x80000
static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( ncache_memory , "ncache_section" )
#endif

/* 2Mb */
#define                 SYSTEM_MEMORY_SIZE       0x200000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
partition_t             *driver_partition = &the_system_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( external_block, "system_section")
#endif


#else /* ST_OS21*/
/* 2Mb */
#define                 SYSTEM_MEMORY_SIZE         0x300000
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

#ifdef SAT_TEST_SCR
#ifdef TEST_DUAL
static STTUNER_Scan_t ScanList1[10];
#endif
#endif
#ifndef ST_OSLINUX
#ifndef STTUNER_MINIDRIVER
#ifdef TUNER_SIGNAL_CHANGE
static STTUNER_ThresholdList_t Thresholds;
static STTUNER_SignalThreshold_t ThresholdList[10];
#endif
#endif
#endif


static STTUNER_BandList_t Bands;
static STTUNER_ScanList_t Scans;

#ifdef SAT_TEST_SCR
#ifdef TEST_DUAL
static STTUNER_ScanList_t Scans1;
#endif
#endif

static const ST_DeviceName_t EVT_RegistrantName1 = "stevt_1";
#ifndef STTUNER_MINIDRIVER
static const ST_DeviceName_t EVT_RegistrantName2 = "stevt_2";
#endif

/* Private function prototypes -------------------------------------------- */

char *TUNER_FecToString(STTUNER_FECRate_t Fec);
char *TUNER_DemodToString(STTUNER_Modulation_t Mod);
char *TUNER_PolarityToString(STTUNER_Polarization_t Plr);
char *TUNER_EventToString(STTUNER_EventType_t Evt);
char *TUNER_AGCToPowerOutput(S16 Agc);
char *TUNER_ToneToString(STTUNER_LNBToneState_t ToneState);
char *TUNER_StatusToString(STTUNER_Status_t Status);

static char *STAPI_ErrorMessages(ST_ErrorCode_t ErrCode);

#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_boot(void);
#ifndef DISABLE_TOOLBOX
static ST_ErrorCode_t Init_sttbx(void);
#endif
#endif /* ST_OSLINUX */
#ifndef ST_OSLINUX
#ifndef USE_STI2C_WRAPPER
static ST_ErrorCode_t Init_pio(void);
#endif /* #ifndef USE_STI2C_WRAPPER */
#endif

static ST_ErrorCode_t Init_tunerparams(void);
static ST_ErrorCode_t Init_scanparams(void);
static ST_ErrorCode_t Init_evt(void);
static ST_ErrorCode_t Open_evt(void);
static ST_ErrorCode_t GlobalTerm(void);
static ST_ErrorCode_t Init_i2c(void);
#ifdef STTUNER_MINIDRIVER
static TUNER_TestResult_t TunerSimple(TUNER_TestParams_t *Params_p);
#endif
#ifndef STTUNER_MINIDRIVER
/* Section: Simple */

#if defined(TUNER_288) && !defined(STTUNER_DISEQC2_SW_DECODE_VIA_PIO)
#ifndef SAT_TEST_SCR
#define MAX_NUM_RECEIVE_BYTES	7
static TUNER_TestResult_t TunerDiSEqC2(TUNER_TestParams_t *Params_p);
#endif
#endif
static TUNER_TestResult_t TunerReLock(TUNER_TestParams_t *Params_p);
#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerFecMode(TUNER_TestParams_t *Params_p);
#ifdef TUNER_SIGNAL_CHANGE
static TUNER_TestResult_t TunerSignalChange(TUNER_TestParams_t *Params_p);
#endif
static TUNER_TestResult_t TunerAPI(TUNER_TestParams_t *Params_p);
#if defined(TUNER_899)&&defined(TEST_DVBS2_MODE)
static TUNER_TestResult_t TunerTestDVBS2Modes(TUNER_TestParams_t *Params_p);
#endif
#endif
static TUNER_TestResult_t TunerScan(TUNER_TestParams_t *Params_p);
#ifdef TUNER_288
static TUNER_TestResult_t TunerBlindScan(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTimedBlindScan(TUNER_TestParams_t *Params_p);
#ifdef DEBUG_288
static TUNER_TestResult_t TunerBlindScanExact(TUNER_TestParams_t *Params_p);
#endif
#endif
static TUNER_TestResult_t TunerTimedScan(TUNER_TestParams_t *Params_p);
 #if defined(TUNER_399)  || defined(TUNER_288)
static TUNER_TestResult_t TunerStandByMode(TUNER_TestParams_t *Params_p);
#endif
static TUNER_TestResult_t TunerScanExact(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTimedScanExact(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerTracking(TUNER_TestParams_t *Params_p);

#if defined(TUNER_299) || defined(TUNER_399) || defined(DUAL_STEM_299)
static TUNER_TestResult_t TunerIQTest(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTerminateSearchTest(TUNER_TestParams_t *Params_p);
#endif

#if defined(TUNER_299) || defined(TUNER_399) || defined(DUAL_STEM_299) || defined(TUNER_288)
static TUNER_TestResult_t TunerScanKuBand(TUNER_TestParams_t *Params_p);
#endif


static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTerm(TUNER_TestParams_t *Params_p);

#endif


#ifdef TEST_DUAL
static TUNER_TestResult_t TunerDualScanSimultaneous(TUNER_TestParams_t *Params_p); /* New Function for Dual Scan */
static TUNER_TestResult_t TunerDualScanStabilityTest(TUNER_TestParams_t *Params_p);/* New Function for Dual Scan Stablity test */
#endif

#ifdef TEST_DISHPOSITIONING
static TUNER_TestResult_t TunerDishPositioning(TUNER_TestParams_t *Params_p);
#endif

#ifdef STTUNER_DISEQC2_SW_DECODE_VIA_PIO
#define MAX_NUM_RECEIVE_BYTES	7
static TUNER_TestResult_t TunerDiSEqC2(TUNER_TestParams_t *Params_p);
#ifdef TEST_DUAL
static TUNER_TestResult_t TunerDiSEqC2Dual(TUNER_TestParams_t *Params_p);
#endif 
#endif
/* Added for Tuner, undefine which is not required */
#define TUN1
#define TUN2

/******* Declarations for DiSEqC Tests****************/
#undef DISEQC_TEST_ON /* for normal unitary test undef */
/*#define DISEQC_TEST_ON 1*/
ST_ErrorCode_t TunerDiseqc(STTUNER_Handle_t TUNERHandle);
/* declaration of the ISR Functions */
void CaptureReplyPulse1(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
void CaptureReplyPulse2(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
/*********************/

/* Test table */
static TUNER_TestEntry_t TestTable[] =
{
    #ifdef STTUNER_MINIDRIVER
    { TunerSimple,              "Simple API 1.0.1 test ",                       1 },
    #endif
    
    #ifndef STTUNER_MINIDRIVER
    
   { TunerIoctl,              "Ioctl API 1.0.1 test ",                       1 },
   #ifndef ST_OSLINUX
  { TunerFecMode,            "DTV/DVB mode check test",                     1 },
    { TunerAPI,                "Test the stablility of the STTUNER API",      1 },
    #endif
    { TunerTerm,               "Termination during a scan",                   1 }, 
    { TunerScan,               "Scan test to acquire channels in band",       1 },
      { TunerReLock,             "Relock channel test ",                        1	},
   #if defined(TUNER_299) || defined(TUNER_399) || defined(DUAL_STEM_299) || defined(TUNER_288)
    { TunerScanKuBand,         "Scan test to acquire channels in Ku band",    1 },
   #endif									
    #if defined(TUNER_399)  || defined(TUNER_288)
    { TunerStandByMode,          "To check the standby Mode",             1 },
    #endif
    { TunerTimedScan,          "Speed test for a scan operation",             1 },    
    { TunerScanExact,          "Scan exact through all acquired channels",    1 },
    { TunerTimedScanExact,     "Speed test for locking to acquired channels", 1 },
    #ifdef TUNER_288
    { TunerBlindScan,          "Scan test to acquire blind channels in band",  1 },
   { TunerTimedBlindScan,     "Scan test to acquire blind channels in band",  1 },
    #ifdef DEBUG_288
    { TunerBlindScanExact,     "Scan all the channels found in blindscan",     1 },
    #endif
    #endif
  #ifdef TUNER_SIGNAL_CHANGE
    { TunerSignalChange,       "Signal change events",                        1 },
   #endif
    { TunerTracking,           "Test tracking of a locked frequency",         1 },
   
    #if defined(TUNER_288) && !defined(STTUNER_DISEQC2_SW_DECODE_VIA_PIO)
    #ifndef SAT_TEST_SCR
    { TunerDiSEqC2  ,           "DiSEqC test for demod  ",         1 },
    #endif
    #endif
    #if defined(TUNER_299) || defined(TUNER_399) || defined(DUAL_STEM_299)
    { TunerIQTest  ,           "Test the IQ settings of the demod  ",         1 },
    #endif
    #if defined(TUNER_299) || defined(TUNER_399)  || defined(DUAL_STEM_299)
    { TunerTerminateSearchTest,"Test the forced termination of the search  ", 1 },
    #endif
   
    #ifdef TEST_DUAL
    { TunerDualScanStabilityTest,"Test Stability scan for two tuners  ",       1},
    { TunerDualScanSimultaneous,"Test Simultaneous scan for two tuners",       1},
    #endif
    #ifdef STTUNER_DISEQC2_SW_DECODE_VIA_PIO
    {TunerDiSEqC2, " Test for DiSEqC",						1},
    #ifdef TEST_DUAL
    {TunerDiSEqC2Dual, " Test for DiSEqC with dual tuner",			1},
    #endif 
    #endif 

    #ifdef TEST_DISHPOSITIONING
    { TunerDishPositioning, "Test for Dish Positioning",                         1 },
    #endif
   
    { 0,                   "",                                                  0 }
    #endif
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
    #endif
    #ifndef ST_OSLINUX
    #ifndef DISABLE_TOOLBOX
    char str[100];
    ST_ErrorCode_t error = ST_NO_ERROR;
    #endif
    #endif

#ifdef ST_5188    
/*STSYS_WriteRegDev32LE(0x20402010, 0x00200020);*/
/*STSYS_WriteRegDev32LE(0x20402010, 0);*/
STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, 0xC0022);
#endif

#if defined(ST_5510)
    strcpy(Backend,"STi5510");
#elif defined(ST_5508)
    strcpy(Backend,"STi5508");
#elif defined(ST_5512)
    strcpy(Backend,"STi5512");
#elif defined(ST_5514)
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
   strcpy(Backend, "STi7710");
#elif defined(ST_7100)
   strcpy(Backend, "STi7100");
#elif defined(ST_7109)
   strcpy(Backend, "STi7109");   
#elif defined(ST_5105)|| defined(ST_5188)|| defined(ST_5107)
   strcpy(Backend,"STi5105");
#elif defined(ST_5301)
   strcpy(Backend,"STi5301");
#elif defined(ST_8010)
   strcpy(Backend,"STm8010");
#endif

#if defined(mb231)
    strcpy(Platform,"mb231");
#elif defined(mb275)
    strcpy(Platform,"mb275");
#elif defined(mb282)
    strcpy(Platform,"mb282");
#elif defined(mb314)
    strcpy(Platform,"mb314");
#elif defined(mb361)
    strcpy(Platform,"mb361");
#elif defined(mb382)
    strcpy(Platform,"mb382");
#elif defined(mb5518)
    strcpy(Platform,"mb5518");
#elif defined(mb376)
    strcpy(Platform,"mb376");
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
#elif defined(mb395)
    strcpy(Platform,"mb395");
#elif defined(maly3s)
    strcpy(Platform,"maly3s");
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

    debugmessage("Init_i2c()");
    if (Init_i2c() != ST_NO_ERROR)
    {
        debugmessage("fail\n");
        return(0);
    }
    debugmessage("ok\n");
    
    debugmessage("Init_tunerparams() \n");
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


    /* ---------------------------------------- */
#ifndef ST_OSLINUX
    task_priority_set(NULL, 0);
#endif
    
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
    TUNER_DebugPrintf(("              Tuner type: %s\n", ChipName));
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
    #endif /* ST_OSLINUX */
    TUNER_DebugPrintf(("         STTUNER:  %s\n", (char *) STTUNER_GetRevision() ));
#ifndef STTUNER_MINIDRIVER
    #ifdef TUNER_399   
    TUNER_DebugPrintf(("         399 LLA: %s\n",   STTUNER_GetLLARevision(STTUNER_DEMOD_STV0399) ));
    #endif
    #if defined(TUNER_299) || defined(DUAL_STEM_299)
    TUNER_DebugPrintf(("         299 LLA: %s\n",   STTUNER_GetLLARevision(STTUNER_DEMOD_STV0299) ));
    #endif
    #if defined(TUNER_899)
    TUNER_DebugPrintf(("         899 LLA: %s\n",  (char *) STTUNER_GetLLARevision(STTUNER_DEMOD_STB0899) ));
    #endif 
    #if defined(TUNER_288)
    TUNER_DebugPrintf(("         288 LLA: %s\n",  (char *) STTUNER_GetLLARevision(STTUNER_DEMOD_STX0288) ));
    #endif
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
    for(tuners = NUM_TUNER-1;tuners >= 0; tuners--)
#endif
    {

        Init_scanparams();
        Test_p  = TestTable;
        Section = 1;
        ChannelTableCount  = 0;
        #ifdef TUNER_288
        BlindScan_ChannelTableCount  = 0;
        #endif

        Total[tuners].NumberPassed = 0;
        Total[tuners].NumberFailed = 0;

        TUNER_DebugMessage("==================================================");
        TUNER_DebugPrintf(("  Test tuner: '%s'\n", TunerDeviceName[tuners] ));
        TUNER_DebugMessage("==================================================");


      if(NUM_TUNER == 1 || NUM_TUNER == 2){
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
            TUNER_DebugPrintf(("                FAILED: %d\n", Total[tuners].NumberFailed));
            TUNER_DebugMessage("--------------------------------------------------");
        }
       } /* endof if statement*/
       GrandTotal.NumberPassed += Total[tuners].NumberPassed;
        GrandTotal.NumberFailed += Total[tuners].NumberFailed;

    } /* for(tuners) */

    /* finally do the dual scan if two tuners are used */

   
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
    time = STOS_time_plus(STOS_time_now(), 60 * ST_GetClocksPerSecond()); /*Timeout maybe increased if scanning a large frequency band*/

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
    time = STOS_time_plus(STOS_time_now(), 60 * ST_GetClocksPerSecond()); /*Timeout maybe increased if scanning a large frequency band*/

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
#elif defined(ST_8010)
        {(U32 *) 0x80000000, (U32 *) 0x807FFFFF},   /* LMI 8Mb */
#elif defined(ST_5301)
        {(U32 *) 0xC0000000, (U32 *) 0xC07FFFFF},   /* LMI 8Mb */
#else
        {(U32 *) 0x40000000, (U32 *) 0x4fffffff},   /* EMI 16Mb */
#endif
        {NULL, NULL}
    };
    
   /* Create memory partitions */
#ifdef ST_OS21   
    system_partition =  partition_create_heap( (U8 *)external_block, sizeof(external_block) );
#else
    partition_init_simple(&the_internal_partition, (U8 *)internal_block, sizeof(internal_block) );
    partition_init_heap(  &the_system_partition,   (U8 *)external_block, sizeof(external_block) );
    partition_init_heap(  &the_ncache_partition,         ncache_memory,   sizeof(ncache_memory) );
#endif    

    BootParams.SDRAMFrequency   = SDRAM_FREQUENCY;
    BootParams.CacheBaseAddress = (U32 *)CACHE_BASE_ADDRESS;

    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;

    BootParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

#if defined(ST_5512) || defined(mb5518)
    BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_64_MBIT;
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105) || defined(ST_5107)|| defined(ST_5188)|| defined(ST_7100)|| defined(ST_7109) || defined(ST_5301) || defined(ST_8010)  /* do not init SMI memory */
    BootParams.MemorySize = SDRAM_SIZE;
#else /* Default 32Mbit on 5510 processor */
    BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_32_MBIT;
#endif

    return( STBOOT_Init( "boot", &BootParams ) );
}
#endif /* ST_OSLINUX */
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
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
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

#if !defined(ST_5188) && !defined(ST_5105) && !defined(ST_5107)
    PioInitParams[4].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[4].BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
    PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
    PioInitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
#endif

#if defined(ST_5518)|| defined (ST_7100)
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
#endif  /*USE_STI2C_WRAPPER*/
#endif  /*ST_OSLINUX*/
/* ------------------------------------------------------------------------- */
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
    #ifdef ST_5188
    I2CInitParams[0].BaudRate          = 50000;
    #else
    I2CInitParams[0].BaudRate          = I2C_BAUDRATE;
    #endif
    I2CInitParams[0].ClockFrequency    = ST_I2C_CLOCKFREQUENCY;
    I2CInitParams[0].MaxHandles        = 8; /* 2 per stv0299, 1 per stv0399*/
    I2CInitParams[0].DriverPartition   = system_partition;
    I2CInitParams[0].FifoEnabled   = FALSE;
    
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
    #ifdef ST_5188
    I2CInitParams[1].BaudRate          = 50000;
    #else
    I2CInitParams[1].BaudRate          = I2C_BAUDRATE;
    #endif
   
    I2CInitParams[1].ClockFrequency    = ST_I2C_CLOCKFREQUENCY;
    
    I2CInitParams[1].MaxHandles        = 8; /* 2 per stv0299, 1 per stv0399*/
    I2CInitParams[1].DriverPartition   = system_partition;
    I2CInitParams[1].FifoEnabled   = FALSE;
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
#if defined(ST_5105)|| defined(ST_5188) || defined(ST_5107)
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

#else /* USE_STI2C_WRAPPER */

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
        debugmessage("Unable to initialize STI2C \n");
    }
    else
    {
        debugmessage("ok \n");
    }
    return error;
}

#endif /* USE_STI2C_WRAPPER */

static ST_ErrorCode_t Init_tunerparams(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    ST_DeviceName_t DeviceName;
    U32 i;
    TUNER_ProbeParams_t           ProbeParams;
    #ifndef STTUNER_MINIDRIVER
    #if defined(LNB_21) || defined(DUAL_STEM_299) || defined(LNBH_21)
    U8 LnbAddress;
    #endif
    #endif
    
    ProbeParams.Driver     = STTUNER_IODRV_I2C;
    ProbeParams.SubAddress = 0;
    ProbeParams.XferSize   = 1;
    ProbeParams.TimeOut    = 50;
    
    for (i = 0; i < NUM_TUNER; i++)     /* TUNER */
   {
        sprintf((char *)DeviceName, "STTUNER[%d]", i);
        strcpy((char *)TunerDeviceName[i], (char *)DeviceName);
#if defined(ST_5100) || defined(ST_7710)|| defined(ST_5105) || defined(ST_5107)|| defined(ST_5188)|| defined(ST_7100) || defined(ST_7109)|| defined(ST_5301) || defined(ST_8010)
        strcpy(ProbeParams.DriverName, (char *)I2CDeviceName[0] );/*0 for backbus*/
#else
		strcpy(ProbeParams.DriverName, (char *)I2CDeviceName[1] );
#endif
        /* Set init params */
        TunerInitParams[i].Device = STTUNER_DEVICE_SATELLITE;

        TunerInitParams[i].Max_BandList      = 10;
        TunerInitParams[i].Max_ThresholdList = 10;
        TunerInitParams[i].Max_ScanList      = 10;

       
        strcpy(TunerInitParams[i].EVT_DeviceName,"stevt");
	

        TunerInitParams[i].DriverPartition = system_partition;
        
		#ifdef SAT_TEST_SCR
			TunerInitParams[0].Scr_App_Type             = STTUNER_SCR_APPLICATION;
			TunerInitParams[0].SCRBPF                   = STTUNER_SCRBPF1; 
			TunerInitParams[0].SCR_Mode                 = STTUNER_SCR_MANUAL;
			TunerInitParams[0].NbScr                    = 4;
                        TunerInitParams[0].NbLnb                    = 2;
                        TunerInitParams[0].SCRBPFFrequencies[0]     = 1399;
                        TunerInitParams[0].SCRBPFFrequencies[1]     = 1516;
                        TunerInitParams[0].SCRBPFFrequencies[2]     = 1632;
                        TunerInitParams[0].SCRBPFFrequencies[3]     = 1748;
                        TunerInitParams[0].SCRLNB_LO_Frequencies[0] = 9750;
                        TunerInitParams[0].SCRLNB_LO_Frequencies[1] = 10600;
                           
                    			
	        #ifdef TEST_DUAL
		        TunerInitParams[1].Scr_App_Type             = STTUNER_SCR_APPLICATION;
			TunerInitParams[1].SCRBPF                   = STTUNER_SCRBPF1; 
			TunerInitParams[1].SCR_Mode                 = STTUNER_SCR_MANUAL;
			TunerInitParams[1].NbScr                    = 4;
                        TunerInitParams[1].NbLnb                    = 2;
                        TunerInitParams[1].SCRBPFFrequencies[0]     = 1399;
                        TunerInitParams[1].SCRBPFFrequencies[1]     = 1516;
                        TunerInitParams[1].SCRBPFFrequencies[2]     = 1632;
                        TunerInitParams[1].SCRBPFFrequencies[3]     = 1748;
                        TunerInitParams[1].SCRLNB_LO_Frequencies[0] = 9750;
                        TunerInitParams[1].SCRLNB_LO_Frequencies[1] = 10600;
                           
        	#endif
        	TunerInitParams[i].ScrIO.Route       = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
		TunerInitParams[i].ScrIO.Driver      = STTUNER_IODRV_I2C;
		TunerInitParams[i].ScrIO.Address     = 0x00;
		#endif

	/* configure diseqc (5100) related initparams */
#if defined(TEST_DISEQC_5100)
                                TunerInitParams[0].DiseqcType = STTUNER_DISEQC_5100;
		TunerInitParams[0].Init_Diseqc.BaseAddress = (U32 *) ST5100_DISEQC_BASE_ADDRESS;
		
		TunerInitParams[0].Init_Diseqc.InterruptNumber = ST5100_DISEQC_INTERRUPT;
		TunerInitParams[0].Init_Diseqc.InterruptLevel = 1/*DISEQC_INTERRUPT_LEVEL*/;
#endif
#if defined(TEST_DISEQC_5301)
                                TunerInitParams[0].DiseqcType = STTUNER_DISEQC_5301;/*OS21*//*st 200*/
		TunerInitParams[0].Init_Diseqc.BaseAddress =(U32 *)0xB8068000;/*(U32 *)ST5301_DISEQC_BASE_ADDRESS;*/
		TunerInitParams[0].Init_Diseqc.InterruptNumber = OS21_INTERRUPT_DISEQ;/*ST5301_DISEQC_INTERRUPT;*/
		TunerInitParams[0].Init_Diseqc.InterruptLevel = 1;/*DISEQC_INTERRUPT_LEVEL;*/
#endif
#if defined(TEST_DISEQC_5100) || defined(TEST_DISEQC_5301)
		TunerInitParams[0].Init_Diseqc.PIOPort =PIO_DEVICE_5 /*PIO_DEVICE_2;*/
		TunerInitParams[0].Init_Diseqc.PIOPinRx = PIO_BIT_4;/*PIO_BIT_6;*/
		TunerInitParams[0].Init_Diseqc.PIOPinTx = PIO_BIT_5;/*PIO_BIT_7;*/
		
		
		TunerInitParams[0].DiseqcIO.Route = STTUNER_IO_DIRECT ;
		TunerInitParams[0].DiseqcIO.Driver = STTUNER_IODRV_NULL;
		TunerInitParams[0].DiseqcIO.Address = 0x00 ;/* Dummy */
		
		
#endif 
     
#if defined(TUNER_399)
        debugmessage(" configuring for EVAL0399\n");
        
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0399;
        #if defined(LNB_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNB21;
		#elif defined(LNBH_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNBH21;
        #else
        TunerInitParams[i].LnbType           = DUT_LNB;
        #endif
        TunerInitParams[i].TunerType         = STTUNER_TUNER_NONE;
        /* MediaRef */
        TunerInitParams[0].DemodIO.Address   = 0xD0;  /* if JP5 then change address = 0xD2 */
        TunerInitParams[1].DemodIO.Address   = 0xD2;  /* if JP6 then change address = 0xD6 */
     
        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_DIRECT;       /* I/O direct to hardware */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;

        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_PASSTHRU;     /* ignored */
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;       /* ignored */
        TunerInitParams[i].TunerIO.Address   = 0;                       /* ignored */

        #if defined(LNB_21) || defined(LNBH_21)
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_DIRECT;     /* use demod I/O handle */
        #else
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
        #endif
        
        TunerInitParams[i].LnbIO.Driver      = STTUNER_IODRV_I2C;
        TunerInitParams[i].LnbIO.Address     = 0x00;                    /* ignored */

        TunerInitParams[i].ExternalClock     = 144000000;
        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
        #ifdef USA_STACK_LNB
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DIRECTV;
        #else
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;
        #endif
#elif defined(TUNER_299)
        debugmessage(" configuring for EVAL0299\n");
        /* STV0299 sat drivers */
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0299;   /* EVAL0299 board v2.1 */        
        #if defined(LNB_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNB21;
	#elif defined(LNBH_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNBH21;
        #else
        TunerInitParams[i].LnbType           = STTUNER_LNB_DEMODIO;
        #endif
        if((TEST_TUNER_TYPE==STTUNER_TUNER_MAX2116) || (TEST_TUNER_TYPE==STTUNER_TUNER_MAX2118))
        {
	        TunerInitParams[i].TunerType = STTUNER_TUNER_MAX2116;
        }
	else    
        TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;

        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_DIRECT;      /* I/O direct to hardware */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].DemodIO.Address   = DEMOD_I2C;               /* stv0299 address */

        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].TunerIO.Address   = TUNER_I2C;               /* address for Tuner (e.g. VG1011 = 0xC0) */

        #if defined(LNB_21) || defined(LNBH_21)
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_DIRECT;     /* use demod I/O handle */
        #else
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
        #endif
        TunerInitParams[i].LnbIO.Driver      = STTUNER_IODRV_I2C;
        TunerInitParams[i].LnbIO.Address     = 0x00;                    /* ignored */

        TunerInitParams[i].ExternalClock     = 4000000;
        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
               
        #ifdef USA_STACK_LNB
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DIRECTV;
        #else
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;
        #endif
        
 #elif defined(TUNER_288)
        debugmessage(" configuring for EVAL0288\n");
        /* STV0288 sat drivers */
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STX0288;   /* EVAL0299 board v2.1 */
        
        #if defined(LNB_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNB21;
	#elif defined(LNBH_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNBH21;
        #else
        TunerInitParams[0].LnbType           = STTUNER_LNB_BACKENDGPIO;
        /*TunerInitParams[0].LnbIOPort.PioDeviceName = PioDeviceName[0];*/
        strcpy(TunerInitParams[0].LnbIOPort.PIODeviceName,PioDeviceName[PIO_DEVICE_3]);
        TunerInitParams[0].LnbIOPort.VSEL_PIOPort = PIO_DEVICE_3;
        TunerInitParams[0].LnbIOPort.VSEL_PIOPin  = PIO_BIT_5;
        TunerInitParams[0].LnbIOPort.VSEL_PIOBit  = 5;
        TunerInitParams[0].LnbIOPort.VEN_PIOPort  = PIO_DEVICE_3;
        TunerInitParams[0].LnbIOPort.VEN_PIOPin   = PIO_BIT_6;
        TunerInitParams[0].LnbIOPort.VEN_PIOBit   = 6;
        TunerInitParams[0].LnbIOPort.TEN_PIOPort  = PIO_DEVICE_3;
        TunerInitParams[0].LnbIOPort.TEN_PIOPin   = PIO_BIT_7;
        TunerInitParams[0].LnbIOPort.TEN_PIOBit   = 7;
        #endif
        if((TEST_TUNER_TYPE==STTUNER_TUNER_MAX2116) || (TEST_TUNER_TYPE==STTUNER_TUNER_MAX2118))
        {
	        TunerInitParams[i].TunerType =STTUNER_TUNER_MAX2116;
        }
	else    
        TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;

        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_DIRECT;      /* I/O direct to hardware */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].DemodIO.Address   = DEMOD_I2C;               /* stv0299 address */
		#if defined(ST_5188)
        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;    
        #else
        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;  /* use demod repeater functionality to pass tuner data */
        #endif
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].TunerIO.Address   = TUNER_I2C;               /* address for Tuner (e.g. VG1011 = 0xC0) */

        #if defined(LNB_21) || defined(LNBH_21)
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_DIRECT;     /* use demod I/O handle */
        #else
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
        #endif
        TunerInitParams[i].LnbIO.Driver      = STTUNER_IODRV_I2C;
        TunerInitParams[i].LnbIO.Address     = 0x00;                    /* ignored */
	#ifdef ST_5188
        TunerInitParams[i].ExternalClock     = 30000000;
        #else
        TunerInitParams[i].ExternalClock     = 4000000;
        #endif
        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
               
        #ifdef USA_STACK_LNB
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DIRECTV;
        #else
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;
        #endif
        
   #elif defined(TUNER_899)
        debugmessage(" configuring for EVAL0899\n");
        /* STV0299 sat drivers */
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STB0899;   /* EVAL0299 board v2.1 */
        #if defined(LNB_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNB21;
	#elif defined(LNBH_21)
        TunerInitParams[i].LnbType           = STTUNER_LNB_LNBH21;
        #else
        TunerInitParams[i].LnbType           = STTUNER_LNB_NONE;
        #endif
        
        TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;

        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_DIRECT;      /* I/O direct to hardware */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].DemodIO.Address   = DEMOD_I2C;              /* stv0299 address */

        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].TunerIO.Address   = TUNER_I2C;               /* address for Tuner (e.g. VG1011 = 0xC0) */

        #if defined(LNB_21) || defined(LNBH_21)
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_DIRECT;     /* use demod I/O handle */
        #else
        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
        #endif
        TunerInitParams[i].LnbIO.Driver      = STTUNER_IODRV_I2C;
        TunerInitParams[i].LnbIO.Address     = 0x00;                    /* ignored */

        TunerInitParams[i].ExternalClock     = 4000000;
        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
        TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;/* Does not suppport for DirecTV*/
                      
 #else
        /* STEM sat drivers */
        debugmessage(" configuring for STEM0299\n");
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0299;   /* Duel STEM board */
        TunerInitParams[i].LnbType           = STTUNER_LNB_STEM;
        TunerInitParams[i].TunerType         = STTUNER_TUNER_HZ1184;

        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_REPEATER ; /*STTUNER_IO_DIRECT;*/ /*I/O direct to hardware */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].DemodIO.Address   = DEMOD_I2C;               /* stv0299 address */

        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].TunerIO.Address   = TUNER_I2C;               /* address for Tuner (e.g. VG1011 = 0xC0) */


        TunerInitParams[i].LnbIO.Route       = STTUNER_IO_DIRECT;  /* use demod I/O handle */
        TunerInitParams[i].LnbIO.Driver      = STTUNER_IODRV_I2C;


        #ifdef DUAL_STEM_299
        {
            for (LnbAddress = 0x42; LnbAddress <= 0x4E; LnbAddress +=2)
            {
                ProbeParams.Address = LnbAddress; /* address of DUAL STEM LNB i/o port. Can be 0x40 to 0x4E */
                error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
                if (error == ST_NO_ERROR)   /* found an address */
                {
                    TUNER_DebugPrintf(("Reply from address 0x%02x indicating the address of the DUAL STEM card.\n", LnbAddress));
                    TunerInitParams[i].LnbIO.Address=LnbAddress;
                    break;  /* out of for(LnbAddress) */
                }

            }   /* for(LnbAddress) */
        }
        #else
            TunerInitParams[i].LnbIO.Address     = 0x40;
        #endif /* defined(ST_5516) */

        TunerInitParams[i].ExternalClock     = 4000000;
        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
        #ifdef USA_STACK_LNB
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DIRECTV;
        #else
            TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;
        #endif
#endif 


}

#ifdef STTUNER_DISEQC2_SW_DECODE_VIA_PIO
  /* Initialize for DiSEqC2.0 */
  TunerInitParams[0].DiSEqC.PIOPort =  PIO_DEVICE_4;
  TunerInitParams[0].DiSEqC.PIOPin  =  PIO_BIT_5;
  TunerInitParams[1].DiSEqC.PIOPort =  PIO_DEVICE_4;
  TunerInitParams[1].DiSEqC.PIOPin  =  PIO_BIT_6;
#endif 

    TunerDeviceName[i][0] = 0;

    strcpy(TunerInitParams[0].DemodIO.DriverName, I2CDeviceName[TUNER0_DEMOD_I2C]);       /* back I2C bus          */
    strcpy(TunerInitParams[0].TunerIO.DriverName, I2CDeviceName[TUNER0_TUNER_I2C]);       /* back I2C bus          */
    strcpy(TunerInitParams[0].LnbIO.DriverName,   I2CDeviceName[TUNER0_DEMOD_I2C]);       /* back I2C bus          */

    strcpy(TunerInitParams[1].DemodIO.DriverName, I2CDeviceName[TUNER1_DEMOD_I2C]);       /* back I2C bus          */
    strcpy(TunerInitParams[1].TunerIO.DriverName, I2CDeviceName[TUNER1_TUNER_I2C]);       /* back I2C bus          */
    strcpy(TunerInitParams[1].LnbIO.DriverName,   I2CDeviceName[TUNER1_DEMOD_I2C]);       /* back I2C bus          */

    strcpy(TunerInitParams[0].EVT_RegistrantName,(char *)EVT_RegistrantName1);
#ifdef TEST_DUAL /*added this so that debugger doesnt crash on breakpoints (in case of NUM_TUNER = 1), on the following line*/
    strcpy(TunerInitParams[1].EVT_RegistrantName,(char *)EVT_RegistrantName2);
#endif
 #ifndef STTUNER_MINIDRIVER   
    #if defined(LNB_21) || defined(LNBH_21)
    #if !defined(ST_5105) && !defined(ST_5107)
    for (i = 0; i < NUM_TUNER; i++)     /* TUNER */
    {   
          strcpy(ProbeParams.DriverName, TunerInitParams[i].LnbIO.DriverName);
          for (LnbAddress = 0x10; LnbAddress <= 0x16; LnbAddress +=2)
          {
                ProbeParams.Address = LnbAddress;
                error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
                if (error == ST_NO_ERROR)   /* found an address */
                {
                    TUNER_DebugPrintf(("Reply from address 0x%02x\n", LnbAddress));
                    TunerInitParams[i].LnbIO.Address=LnbAddress;
                    if(i==0)/* For first tuner,LNBAddress will be 0x10; for second tuner lnbaddress will be 0x16*/
                    break;  /* out of for(LnbAddress) */
                }

	  }   /* for(LnbAddress) */
    }
    LnbAddress=TunerInitParams[0].LnbIO.Address=0x16;
    #else
    /*TunerInitParams[0].LnbIO.Address=0x16;*/ /*changed to remove warning*/
    LnbAddress=TunerInitParams[0].LnbIO.Address=0x16 ;
    #endif
    #endif
    
    #endif
    #ifdef STTUNER_MINIDRIVER
    #if defined(ST_5105) || defined(ST_5107)
    TunerInitParams[0].LnbIO.Address=0x16;
    #else
    TunerInitParams[0].LnbIO.Address=0x10;
    TunerInitParams[1].LnbIO.Address=0x16;
    #endif
    #endif
    
    return(error);    
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Init_scanparams(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Set tuner band list */
    Bands.NumElements = 2;

#ifndef USA_STACK_LNB
 #ifdef C_BAND_TESTING
        BandList[0].BandStart = 2950000; /*L Band (LO - Transponder freq): 2200 MHz*/
        BandList[0].BandEnd   = 4250000; /*L Band (LO - Transponder freq): 900  MHz*/
        BandList[0].LO        = 5150000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_C;

        BandList[1].BandStart = 10700000;
        BandList[1].BandEnd   = 11700000;
        BandList[1].LO        = 9760000;
        BandList[1].DownLink  = STTUNER_DOWNLINK_Ku;
 #else /* Ku band testing */
        BandList[0].BandStart = 10600000;
        BandList[0].BandEnd   = 11700000;
        BandList[0].LO        =  9750000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_Ku;

        BandList[1].BandStart = 11700000;
        BandList[1].BandEnd   = 12750000;
        BandList[1].LO        = 10600000;
        BandList[1].DownLink  = STTUNER_DOWNLINK_Ku;

 #endif

#else /* #ifndef USA_STACK_LNB  */

 #ifdef C_BAND_TESTING
        BandList[0].BandStart =  3200000;
        BandList[0].BandEnd   =  5150000;
        BandList[0].LO        =  5150000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_C;

        BandList[1].BandStart = 10700000;
        BandList[1].BandEnd   = 11700000;
        BandList[1].LO        = 9760000;
        BandList[1].DownLink  = STTUNER_DOWNLINK_Ku;
 #else /* Ku band is default */
        BandList[0].BandStart = 12200000;
        BandList[0].BandEnd   = 12700000;
        BandList[0].LO        = 11250000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_Ku;

        BandList[1].BandStart = 12200000;
        BandList[1].BandEnd   = 12700000;
        BandList[1].LO        = 10675000;
        BandList[1].DownLink  = STTUNER_DOWNLINK_Ku;
 #endif

#endif /* #ifndef USA_STACK_LNB  */


    /* Setup scan list */

#ifdef USA_STACK_LNB

    Scans.NumElements = 4;
  #ifdef C_BAND_TESTING
    ScanList[0].SymbolRate   = 27000000;
    ScanList[0].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[0].FECRates     = STTUNER_FEC_ALL /*STTUNER_FEC_ALL */;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[0].Band         = 0;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[1].SymbolRate   = 27000000;
    ScanList[1].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[1].FECRates     = STTUNER_FEC_ALL; /*STTUNER_FEC_ALL;*/
    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[1].Band         = 0;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[2].SymbolRate   = 27000000;
    ScanList[2].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[2].FECRates     = STTUNER_FEC_ALL; /*STTUNER_FEC_ALL;*/
    ScanList[2].Modulation   = STTUNER_MOD_QPSK;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[2].Band         = 1;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[3].SymbolRate   = 27000000;
    ScanList[3].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[3].FECRates     = STTUNER_FEC_ALL; /*STTUNER_FEC_ALL;*/
    ScanList[3].Modulation   = STTUNER_MOD_QPSK;
    ScanList[3].AGC          = STTUNER_AGC_ENABLE;
    ScanList[3].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[3].Band         = 1;
    ScanList[3].IQMode       = STTUNER_IQ_MODE_AUTO;
    
 #else /* Ku band is default */

    ScanList[0].SymbolRate   = 27500000;
    ScanList[0].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[0].Band         = 0;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[1].SymbolRate   = 22000000;
    ScanList[1].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[1].Band         = 0;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[2].SymbolRate   = 27500000;
    ScanList[2].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[2].FECRates     = STTUNER_FEC_ALL;
    ScanList[2].Modulation   = STTUNER_MOD_QPSK;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[2].Band         = 1;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[3].SymbolRate   = 22000000;
    ScanList[3].Polarization = STTUNER_PLR_VERTICAL;/* This is so it doesn't do two scans - just the one */
    ScanList[3].FECRates     = STTUNER_FEC_ALL;
    ScanList[3].Modulation   = STTUNER_MOD_QPSK;
    ScanList[3].AGC          = STTUNER_AGC_ENABLE;
    ScanList[3].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[3].Band         = 1;
    ScanList[3].IQMode       = STTUNER_IQ_MODE_AUTO;
    
  #endif  /* ifdef C_BAND_TESTING */

#elif defined(TUNER_LOW_BAND)

    Scans.NumElements = 3;

  #ifdef C_BAND_TESTING
    ScanList[0].SymbolRate   = 27000000;
    ScanList[0].Polarization = STTUNER_PLR_VERTICAL ; /*STTUNER_PLR_ALL;*/
    ScanList[0].FECRates     = STTUNER_FEC_3_4; /*STTUNER_FEC_ALL;*/
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[0].Band         = 0;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[1].SymbolRate   = 27000000;
    ScanList[1].Polarization = STTUNER_PLR_VERTICAL ; /*STTUNER_PLR_ALL;*/
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[1].Band         = 0;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[2].SymbolRate   = 27000000;
    ScanList[2].Polarization = STTUNER_PLR_VERTICAL ; /*STTUNER_PLR_ALL;*/
    ScanList[2].FECRates     = STTUNER_FEC_3_4; /*STTUNER_FEC_ALL;*/
    ScanList[2].Modulation   = STTUNER_MOD_QPSK;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[2].Band         = 0;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    #else /* Ku band is default */

    ScanList[0].SymbolRate   = 27500000;
    ScanList[0].Polarization = STTUNER_PLR_ALL;
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[0].Band         = 0;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[1].SymbolRate   = 27250000;
    ScanList[1].Polarization = STTUNER_PLR_ALL;
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[1].Band         = 0;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    
    ScanList[2].SymbolRate   = 27000000;
    ScanList[2].Polarization = STTUNER_PLR_ALL;
    ScanList[2].FECRates     = STTUNER_FEC_ALL;
    ScanList[2].Modulation   = STTUNER_MOD_QPSK;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanList[2].Band         = 0;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    
  #endif /* #ifdef C_BAND_TESTING */

#else


  Scans.NumElements = 2;
  #ifdef C_BAND_TESTING
  
	    ScanList[0].SymbolRate   = 26670000;
	    ScanList[0].Polarization = STTUNER_PLR_HORIZONTAL;
	    ScanList[0].FECRates     = STTUNER_FEC_ALL;
	    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[0].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[0].Band         = 0;
	    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
	    ScanList[0].FECType      = STTUNER_FEC_MODE_DVBS1;
	    #ifdef SAT_TEST_SCR
	    ScanList[0].ScrParams.LNBIndex = STTUNER_LNB3;
	    ScanList[0].ScrParams.SCRBPF = STTUNER_SCRBPF3 ;
	    ScanList[0].LNB_SignalRouting = STTUNER_VIA_SCRENABLED_LNB;
	    #endif
	        
	    ScanList[1].SymbolRate   = 27500000;
	    ScanList[1].Polarization = STTUNER_PLR_VERTICAL;
	    ScanList[1].FECRates     = STTUNER_FEC_ALL;
	    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[1].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[1].Band         = 0;
	    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
	    ScanList[1].FECType      = STTUNER_FEC_MODE_DVBS2;
	    #ifdef SAT_TEST_SCR
	    ScanList[1].ScrParams.LNBIndex = STTUNER_LNB1;
	    ScanList[1].ScrParams.SCRBPF = STTUNER_SCRBPF1 ;
	    ScanList[1].LNB_SignalRouting = STTUNER_VIA_SCRENABLED_LNB;
	    #endif
     
       /* For  SCR application two scanlist are defined just to get two different transponder 
       from each band pass filters */
    #ifdef SAT_TEST_SCR
	    #ifdef TEST_DUAL
	    	Scans1.NumElements = 2;
	    	
		    ScanList1[0].SymbolRate   = 26670000;
		    ScanList1[0].Polarization = STTUNER_PLR_HORIZONTAL;
		    ScanList1[0].FECRates     = STTUNER_FEC_ALL;
		    ScanList1[0].Modulation   = STTUNER_MOD_QPSK;
		    ScanList1[0].AGC          = STTUNER_AGC_ENABLE;
		    ScanList1[0].LNBToneState = STTUNER_LNB_TONE_OFF;
		    ScanList1[0].Band         = 1;
		    ScanList1[0].IQMode       = STTUNER_IQ_MODE_AUTO;
		    #ifdef SAT_TEST_SCR
		    ScanList1[0].ScrParams.LNBIndex = STTUNER_LNB1;
		    ScanList1[0].ScrParams.SCRBPF = STTUNER_SCRBPF0 ;
		    #endif
		    
		    ScanList1[1].SymbolRate   = 27500000;
		    ScanList1[1].Polarization = STTUNER_PLR_VERTICAL;
		    ScanList1[1].FECRates     = STTUNER_FEC_ALL;
		    ScanList1[1].Modulation   = STTUNER_MOD_QPSK;
		    ScanList1[1].AGC          = STTUNER_AGC_ENABLE;
		    ScanList1[1].LNBToneState = STTUNER_LNB_TONE_OFF;
		    ScanList1[1].Band         = 1;
		    ScanList1[1].IQMode       = STTUNER_IQ_MODE_AUTO;
		    #ifdef SAT_TEST_SCR
		    ScanList1[1].ScrParams.LNBIndex = STTUNER_LNB0;
		    ScanList1[1].ScrParams.SCRBPF = STTUNER_SCRBPF3 ;
		    #endif
		    Scans1.ScanList = ScanList1;
	    #endif
    #endif
            
    #else /* default is Ku band */

    ScanList[0].SymbolRate   = 27500000;
    ScanList[0].Polarization = STTUNER_PLR_ALL;
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[0].Band         = 1;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanList[0].ScrParams.LNBIndex = STTUNER_LNB3;
    ScanList[0].ScrParams.SCRBPF = STTUNER_SCRBPF1 ;
    #endif
    
    ScanList[1].SymbolRate   = 27500000;
    ScanList[1].Polarization = STTUNER_PLR_ALL;
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[1].Band         = 1;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanList[1].ScrParams.LNBIndex = STTUNER_LNB3;
    ScanList[1].ScrParams.SCRBPF = STTUNER_SCRBPF1 ;
    #endif
    
    ScanList[2].SymbolRate   = 36000000;
    ScanList[2].Polarization = STTUNER_PLR_HORIZONTAL;
    ScanList[2].FECRates     = STTUNER_FEC_ALL;
    ScanList[2].Modulation   = STTUNER_MOD_QPSK;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[2].Band         = 1;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanList[2].ScrParams.LNBIndex = STTUNER_LNB1;
    ScanList[2].ScrParams.SCRBPF = STTUNER_SCRBPF0 ;
    #endif
    
    ScanList[3].SymbolRate   = 27500000;
    ScanList[3].Polarization = STTUNER_PLR_HORIZONTAL;
    ScanList[3].FECRates     = STTUNER_FEC_ALL;
    ScanList[3].Modulation   = STTUNER_MOD_QPSK;
    ScanList[3].AGC          = STTUNER_AGC_ENABLE;
    ScanList[3].LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanList[3].Band         = 1;
    ScanList[3].IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanList[3].ScrParams.LNBIndex = STTUNER_LNB0;
    ScanList[3].ScrParams.SCRBPF = STTUNER_SCRBPF0 ;
    #endif
    
    /* For  SCR application two scanlist are defined just to get two different transponder 
       from each band pass filters */
    #ifdef SAT_TEST_SCR
	    #ifdef TEST_DUAL
	    	Scans1.NumElements = 2;
	    	
		    ScanList1[0].SymbolRate   = 27500000;
		    ScanList1[0].Polarization = STTUNER_PLR_HORIZONTAL;
		    ScanList1[0].FECRates     = STTUNER_FEC_ALL;
		    ScanList1[0].Modulation   = STTUNER_MOD_QPSK;
		    ScanList1[0].AGC          = STTUNER_AGC_ENABLE;
		    ScanList1[0].LNBToneState = STTUNER_LNB_TONE_OFF;
		    ScanList1[0].Band         = 1;
		    ScanList1[0].IQMode       = STTUNER_IQ_MODE_AUTO;
		    #ifdef SAT_TEST_SCR
		    ScanList1[0].ScrParams.LNBIndex = STTUNER_LNB2;
		    ScanList1[0].ScrParams.SCRBPF = STTUNER_SCRBPF0 ;
		    #endif
		    
		    ScanList1[1].SymbolRate   = 26000000;
		    ScanList1[1].Polarization = STTUNER_PLR_HORIZONTAL;
		    ScanList1[1].FECRates     = STTUNER_FEC_ALL;
		    ScanList1[1].Modulation   = STTUNER_MOD_QPSK;
		    ScanList1[1].AGC          = STTUNER_AGC_ENABLE;
		    ScanList1[1].LNBToneState = STTUNER_LNB_TONE_OFF;
		    ScanList1[1].Band         = 1;
		    ScanList1[1].IQMode       = STTUNER_IQ_MODE_AUTO;
		    #ifdef SAT_TEST_SCR
		    ScanList1[1].ScrParams.LNBIndex = STTUNER_LNB2;
		    ScanList1[1].ScrParams.SCRBPF = STTUNER_SCRBPF0 ;
		    #endif
		    Scans1.ScanList = ScanList1;
	    #endif
	#endif
  #endif /* C BAND Single Frequency */

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
    
    error = STEVT_SubscribeDeviceEvent(EVTHandle1, (char *)EVT_RegistrantName1, STTUNER_EV_LNB_FAILURE, &SubscribeParams);
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
   
    error = STEVT_SubscribeDeviceEvent(EVTHandle2, (char *)EVT_RegistrantName2, STTUNER_EV_LNB_FAILURE, &SubscribeParams);
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

#ifndef STTUNER_MINIDRIVER

/* ------------------------------------------------------------------------- */
/* ------------------------ Test harness tests ----------------------------- */
/* ------------------------------------------------------------------------- */
#ifndef  ST_OSLINUX
#if defined TUNER_399 || defined(TUNER_899) || defined(TUNER_288) 
 static TUNER_TestResult_t TunerFecMode(TUNER_TestParams_t *Params_p)
 {
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;

    TUNER_DebugMessage("Note: STV0399/STB0899 not supported for this test.");
    return(Result);
 }
#else
/*----------------------------------------------------------------------------
TunerFecMode()

Description:
    This routine checks a 299 register for the correct operating mode for DTV or DVB.

Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t TunerFecMode(TUNER_TestParams_t *Params_p)
{
    ST_ErrorCode_t      ST_ErrorCode;
    TUNER_TestResult_t  Result = TEST_RESULT_ZERO;

    STTUNER_InitParams_t    STTUNER_InitParams;
    STTUNER_Handle_t        STTUNER_Handle;
    STTUNER_OpenParams_t    STTUNER_OpenParams;
    STTUNER_TermParams_t    STTUNER_TermParams;

    U32                     inparam, outparam;

/*
      Chip: STV0299
  Register: RS (0x33)
       Bit: RS2 - Block Synchro (bit 2 of 7..0)

1: The first byte of each packet is forced to Hex 47 in mode A.
0: The first byte is the one that is received. In DVB, it should be the synchro byte,
   complemented every 8th packet.

           7654 3210   2
           ---- ----   -
DVB: 0xFC  1111 1100   1
DTV: 0xF8  1111 1000   0

*/
    /* --- Setup Tuner for DVB --- */
    TUNER_DebugMessage("DVB test");

    /* make local copy of tuner prams to mess around with */
    memcpy (&STTUNER_InitParams, &TunerInitParams[Params_p->Tuner], sizeof( STTUNER_InitParams_t ) );
    STTUNER_InitParams.FECMode = STTUNER_FEC_MODE_DVB;

    ST_ErrorCode = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &STTUNER_InitParams);
    TUNER_DebugError("STTUNER_Init()", ST_ErrorCode);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return (Result);
    }

    #ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
        STTUNER_OpenParams.NotifyFunction = CallbackNotifyFunction1;
    else
        STTUNER_OpenParams.NotifyFunction = CallbackNotifyFunction2;
    #else
        STTUNER_OpenParams.NotifyFunction = NULL;
    #endif /* ST_OSLINUX*/

    ST_ErrorCode = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &STTUNER_OpenParams, &STTUNER_Handle);
    TUNER_DebugError("STTUNER_Open()", ST_ErrorCode);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tunermode_end;
    }

    inparam  = 0x28;  /* index to FECM register, see: reg0299.h */
    outparam = 0;

    ST_ErrorCode = STTUNER_Ioctl(STTUNER_Handle, STTUNER_DEMOD_STV0299, STTUNER_IOCTL_REG_IN, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", ST_ErrorCode);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tunermode_end;
    }
    TUNER_DebugPrintf(("FECM register = 0x%x\n", outparam ));
    
	    if (((outparam & 0xf0) != 0x00) && ((outparam & 0xf0)!= 0x10))
	    {
	        TUNER_TestFailed(Result,"expected a value of 0x00 or 0x10");
	        goto tunermode_end;
	    }
    
    ST_ErrorCode = STTUNER_Close(STTUNER_Handle);
    TUNER_DebugError("STTUNER_Close()", ST_ErrorCode);

    STTUNER_TermParams.ForceTerminate = FALSE;
    ST_ErrorCode = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &STTUNER_TermParams);
    TUNER_DebugError("STTUNER_Term()", ST_ErrorCode);


    /* --- Setup Tuner for DTV --- */
    TUNER_DebugMessage("DTV test");

    STTUNER_InitParams.FECMode = STTUNER_FEC_MODE_DIRECTV;

    ST_ErrorCode = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &STTUNER_InitParams);
    TUNER_DebugError("STTUNER_Init()", ST_ErrorCode);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return (Result);
    }

    #ifndef ST_OSLINUX
    if (Params_p->Tuner == 0)
        STTUNER_OpenParams.NotifyFunction = CallbackNotifyFunction1;
    else
        STTUNER_OpenParams.NotifyFunction = CallbackNotifyFunction2;
    #else
        STTUNER_OpenParams.NotifyFunction = NULL;
    #endif

    ST_ErrorCode = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &STTUNER_OpenParams, &STTUNER_Handle);
    TUNER_DebugError("STTUNER_Open()", ST_ErrorCode);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tunermode_end;
    }

    inparam  = 0x20;
    outparam = 0;

    ST_ErrorCode = STTUNER_Ioctl(STTUNER_Handle, STTUNER_DEMOD_STV0299, STTUNER_IOCTL_REG_IN, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", ST_ErrorCode);
    if (ST_ErrorCode != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tunermode_end;
    }

    TUNER_DebugPrintf(("FECM register = 0x%x\n", outparam ));
    if (((outparam & 0xf0) != 0x40))
	    {
	        TUNER_TestFailed(Result,"expected a value of 0x40");
	        goto tunermode_end;
	    }

    TUNER_TestPassed(Result);

    /* ------------------------------------- */

tunermode_end:

    ST_ErrorCode = STTUNER_Close(STTUNER_Handle);
    TUNER_DebugError("STTUNER_Close()", ST_ErrorCode);

    STTUNER_TermParams.ForceTerminate = FALSE;
    ST_ErrorCode = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &STTUNER_TermParams);
    TUNER_DebugError("STTUNER_Term()", ST_ErrorCode);

    return(Result);
}
#endif
#endif


/*----------------------------------------------------------------------------
Function:
    TunerIoctl()

Description:
    This routine tests features of the ioctl API 1.0.1.

Parameters:
    Params_p,       pointer to test params block.

Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.

See Also:
    Nothing.
----------------------------------------------------------------------------*/

/* NOTE: The two defines below are not the register addresses but INDEXES
        into an array holding information on the registers,
        see sttuner/drivers/ioreg.c for more information. */
#define STV0299_REG_ID_INDEX    0
#define STV0299_REG_SFRM_INDEX  0x20

#define STV0399_REG_ID_INDEX    0
#define STV0399_REG_SFRM_INDEX  0x29

#define STB0899_REG_ID_INDEX    0xf000
#define STB0899_REG_SFRM_INDEX 0xf447

#define STX0288_REG_ID_INDEX    0
#define STX0288_REG_SFRM_INDEX  0x1c


static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t     Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t         Error;
    #ifndef ST_OSLINUX 
    #if defined(LNBH_21)
    TEST_LNB_IOCTL_Config_t InParam,OutParam;
    #endif
    BOOL                   bin_inparam, bin_outparam;
    U8                     ID;
    SETREG_InParams_t      WriteReg;
    U32                    loop,original;
    U32                    inparam, outparam;
    STTUNER_OpenParams_t   OpenParams;
    STTUNER_TermParams_t   TermParams;
    STTUNER_Handle_t       Handle; 
    ST_ErrorCode_t         errc, errt;
    #endif
   
    U32                    Test = 0;
    
#if !defined(DUAL_STEM_299) && !defined(LNB_21) && !defined(LNBH_21)
    TEST_IOARCH_Params_t   IOParams;
    U8                     Data[20];
#endif
   
    
    TEST_ProbeParams_t     ProbeParams;

    U8                     address;

    /* ---------- PROBE FOR CHIP TYPE - TUNER UNINITALIZED ---------- */

    TUNER_DebugPrintf(("Test #%d.1\n", ++Test));
    TUNER_DebugMessage("Purpose: Probe to find chip types on I2C bus (STTUNER_IOCTL_PROBE)");
    TUNER_DebugMessage("         With tuner uninitalized.");

    /* look for a chip ID (register 0x00) at I2C address 0xD0, 0xD2, 0xD4 and 0xD6 */
    /* ASSUME: chip ID register is at offset zero! */
    ProbeParams.Driver     = STTUNER_IODRV_I2C;
    ProbeParams.SubAddress = 0;     /* register address */
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

    for(address = 0xD0; address <= 0xD6; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(STTUNER_MAX_HANDLES)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n", ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0xF0)
            {
                case 0xA0:
                    TUNER_DebugMessage("Chip is STV0299");
                    break;

                case 0xB0:
                    TUNER_DebugMessage("Chip is STV0399");
                    break;
                
                case 0x10:
                    TUNER_DebugMessage("Chip is STx0288");
                    break;
                    
                case 0x30:
                    TUNER_DebugMessage("Chip is STB0899");
                    break;

                default:
                    TUNER_DebugMessage("Chip present but is unknown");
                    break;
            }
        }
        else
        {
            TUNER_DebugPrintf(("Chip not present at address 0x%02x\n", address));
        }

    }   /* for(address) */

#ifndef ST_OSLINUX 
    /* ---------- INITALIZE & OPEN TUNER ---------- */

    TUNER_DebugPrintf(("Test #%d.2\n", Test));
    TUNER_DebugMessage("Purpose: Initalize & open tuner instance");

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

    for(address = 0xD0; address <= 0xD6; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(Handle)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n",ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0xF0)
            {
                case 0xA0:
                    TUNER_DebugMessage("Chip is STV0299");
                    break;

                case 0xB0:
                    TUNER_DebugMessage("Chip is STV0399");
                    break;
                    
                case 0x10:
                    TUNER_DebugMessage("Chip is STx0288");
                    break;
                    
                case 0x30:
                    TUNER_DebugMessage("Chip is STB0899");
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

    for(address = 0xD0; address <= 0xD6; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(STTUNER_MAX_HANDLES)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n",ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0xF0)
            {
                case 0xA0:
                    TUNER_DebugMessage("Chip is STV0299");
                    break;

                case 0xB0:
                    TUNER_DebugMessage("Chip is STV0399");
                    break;
                    
                case 0x10:
                    TUNER_DebugMessage("Chip is STx0288");
                    break;
                    
                case 0x30:
                    TUNER_DebugMessage("Chip is STB0899");
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


    /* ---------- ENABLE/DISABLE 'SIGNAL NOISE' TEST MODE ---------- */

    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugMessage("Purpose: Enable/Disable 'Signal Noise' test mode (STTUNER_IOCTL_TEST_SN)");
    bin_inparam = TRUE;
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_TEST_SN, &bin_inparam, &bin_outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    if (bin_outparam != TRUE)
    {
        TUNER_TestFailed(Result,"Expected TRUE");
        goto ioctl_end;
    }

    bin_inparam = FALSE;
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_TEST_SN, &bin_inparam, &bin_outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    if (bin_outparam != FALSE)
    {
        TUNER_TestFailed(Result,"Expected FALSE");
        goto ioctl_end;
    }


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

#endif /*ST_OSLINUX*/
    /* ---------- GET STV0299/STV0399 ID REGISTER VALUE ---------- */

    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugPrintf(("Purpose: Read %s ID register (STTUNER_IOCTL_REG_IN)\n", ChipName));
#ifdef TUNER_399
    inparam = STV0399_REG_ID_INDEX; /* index to ID register see: reg0399.h */
    ID = 0xB0;
#elif defined (TUNER_299) || defined(DUAL_STEM_299)
    inparam = STV0299_REG_ID_INDEX; /* index to ID register see: reg0299.h */
    ID = 0xA0;
#elif defined TUNER_899
    inparam = STB0899_REG_ID_INDEX;
    ID = 0x30;
 
    
#elif defined TUNER_288
    inparam = STX0288_REG_ID_INDEX;
    ID = 0x10;
   
#endif

    Error = STTUNER_Ioctl(Handle, DUT_DEMOD, STTUNER_IOCTL_REG_IN, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    TUNER_DebugPrintf(("Chip ID register: 0x%x\n", outparam ));
    if ( (outparam & ID) !=  ID)
    {
        TUNER_TestFailed(Result,"Incorrect chip ID");
        /* continue */
    }

    /* ---------- MODIFY SFRM REGISTER VALUE ---------- */

    /*  SFRM chosen because it can be read from and written to. */
    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugPrintf(("Purpose: Modify %s SFRM register (STTUNER_IOCTL_REG_OUT)\n", ChipName));

    for(loop = 1; loop <= 4; loop++)
    {
        TUNER_DebugPrintf(("Test #%d.%d\n", Test, loop ));

#ifdef TUNER_399
        inparam = STV0399_REG_SFRM_INDEX;
#elif defined TUNER_299 || defined(DUAL_STEM_299)
        inparam = STV0299_REG_SFRM_INDEX;
#elif defined TUNER_899        
        inparam = STB0899_REG_SFRM_INDEX;
#elif defined TUNER_288       
        inparam = STX0288_REG_SFRM_INDEX;
#endif
        WriteReg.RegIndex = inparam;
        outparam = 0xff;    /* clean variable, in general SFRM seems to start out as zero */
        Error = STTUNER_Ioctl(Handle, DUT_DEMOD, STTUNER_IOCTL_REG_IN, &inparam, &outparam);
        outparam &= 0xff; 
        TUNER_DebugError("STTUNER_Ioctl()", Error);
        if (Error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            goto ioctl_end;
        }
        TUNER_DebugPrintf(("%s SFRM register (0x%02x) <read>   %d\n", ChipName, inparam, outparam ));


        WriteReg.Value = (outparam + (0x10 * loop) + (loop)) & 0xff; /* change upper & lower nibbles. */
        TUNER_DebugPrintf(("%s SFRM register <modify> %d\n", ChipName, WriteReg.Value ));


        Error = STTUNER_Ioctl(Handle, DUT_DEMOD, STTUNER_IOCTL_REG_OUT, &WriteReg, NULL);
        TUNER_DebugError("STTUNER_Ioctl()", Error);
        if (Error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            goto ioctl_end;
        }
        TUNER_DebugPrintf(("%s SFRM register <write>  %d\n", ChipName, WriteReg.Value ));


        outparam = 0xff;    /* least likley to be a valid value read back */
        Error = STTUNER_Ioctl(Handle, DUT_DEMOD, STTUNER_IOCTL_REG_IN, &inparam, &outparam);
        outparam &= 0xff;
        TUNER_DebugError("STTUNER_Ioctl()", Error);
        if (Error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            goto ioctl_end;
        }
        TUNER_DebugPrintf(("%s SFRM register <read>   %d\n", ChipName, outparam ));
        if (outparam != WriteReg.Value)
        {
            TUNER_TestFailed(Result,"Expected SFRM register updated");
            /* continue */
        }

    }   /* for(loop) */


  /* ----------  USE RAW IO TO GET CHIP ID NUMBER  ---------- */

  /* IMPORTANT NOTE: This test must ALWAYS come after the 'modify SFRM register' test */

    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugPrintf(("Purpose: Read %s registers using raw I/O via LNB device-driver (STTUNER_IOCTL_RAWIO)\n", ChipName));

#if defined(DUAL_STEM_299) || defined(LNB_21) || defined(LNBH_21)
        TUNER_DebugPrintf(("This test is not applicable with the STEM board or with LNB21\n"));
#else

    IOParams.Operation    = TEST_IOARCH_SA_READ;
#ifdef TUNER_399
    IOParams.SubAddr      = STV0399_REG_ID_INDEX;      /* ID register address 399 */
#elif defined TUNER_299|| defined(DUAL_STEM_299)
    IOParams.SubAddr      = STV0299_REG_ID_INDEX;      /* ID register address 299 */
#elif defined TUNER_899
    IOParams.SubAddr      = STB0899_REG_ID_INDEX;      /* ID register address 899 */
#elif defined TUNER_288
    IOParams.SubAddr      = STX0288_REG_ID_INDEX;      /* ID register address 288 */
#endif
    IOParams.Data         = Data;   /* point to data buffer for I/O       */
    IOParams.TransferSize = 1;      /* read back one byte (the register contents) */
    IOParams.Timeout      = 20;     /* I2C timeout (20 milli-Seconds)    */
    Data[0]               = 0x00;   /* clean out location of return data */
   
    Error = STTUNER_Ioctl(Handle, DUT_LNB, STTUNER_IOCTL_RAWIO, &IOParams, NULL);   
     
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    TUNER_DebugPrintf(("%s Chip ID register: 0x%x\n", ChipName, Data[0] ));
#ifdef TUNER_399
    if ( (Data[0] & 0xB0) !=  0xB0)
#elif defined TUNER_299
    if ( (Data[0] & 0xA0) !=  0xA0)
#elif defined TUNER_899
    if ( (Data[0] & 0x30) !=  0x30)
#elif defined TUNER_288
    if ( (Data[0] & 0x10) !=  0x10)
#endif
    {
        TUNER_TestFailed(Result,"Incorrect chip ID");
        /* continue */
    }


#ifdef TUNER_399
    IOParams.SubAddr = STV0399_REG_SFRM_INDEX;    /* SFRM register address 399 */
#elif defined TUNER_299 || defined(DUAL_STEM_299)
    IOParams.SubAddr = STV0299_REG_SFRM_INDEX;    /* SFRM register address 299 */
#elif defined TUNER_899
    IOParams.SubAddr = STB0899_REG_SFRM_INDEX;    /* SFRM register address 899 */
#elif defined TUNER_288
    IOParams.SubAddr = STX0288_REG_SFRM_INDEX;    /* SFRM register address 288 */
#endif
    Data[0]          = 0x00;    /* clean out location of return data */    
   
    Error = STTUNER_Ioctl(Handle, DUT_LNB, STTUNER_IOCTL_RAWIO, &IOParams, NULL);
    
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto ioctl_end;
    }
    TUNER_DebugPrintf(("%s SFRM register: %d\n", ChipName, Data[0] ));
    if ( Data[0] != WriteReg.Value )
    {
        TUNER_TestFailed(Result,"Expected SFRM to be the same.");
        /* continue */
    }

#endif /* DUAL_STEM_299 or LNB_21*/


  /* ---------- Change tuner scan step to a fixed value & back to the default calculated value.---------- */
#ifndef  ST_OSLINUX
    TUNER_DebugPrintf(("Test #%d\n", ++Test));

    TUNER_DebugPrintf(("Purpose: Get tuner scan step size (STTUNER_IOCTL_TUNERSTEP)\n"));
    outparam = (U32)-1; /* biggest number */
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_TUNERSTEP, NULL, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"failed to set tuner step");
        goto ioctl_end;
    }
    else
    {
        if (outparam == 0)
        {
            TUNER_DebugPrintf(("Tuner step size: %d (calculated)\n", outparam));
        }
        else
        {
            TUNER_DebugPrintf(("Tuner step size: %d (fixed)\n", outparam ));
        }
    }

    TUNER_DebugPrintf(("Purpose: Set tuner scan step to a fixed value (STTUNER_IOCTL_TUNERSTEP)\n"));
    inparam = FIXED_TUNER_SCAN_STEP;
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_TUNERSTEP, &inparam, NULL);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"failed to set tuner step");
        goto ioctl_end;
    }

    TUNER_DebugPrintf(("Purpose: Set tuner scan step to calculated value (STTUNER_IOCTL_TUNERSTEP)\n"));
    inparam = 0;
    Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_TUNERSTEP, &inparam, NULL);
    TUNER_DebugError("STTUNER_Ioctl()", Error);
    if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"failed to set tuner step");
        goto ioctl_end;
    }
    
  
#if defined(LNBH_21)
/* Get /Set LNB parameters throuh ioctl */
  InParam.TTX_Mode = LNB_IOCTL_TTXMODE_RX; /* RX*/
  InParam.PowerControl = LNB_IOCTL_POWERBLCOKS_ENABLED; /* Enable power block*/
  InParam.ShortCircuitProtectionMode = LNB_IOCTL_PROTECTION_STATIC; /* static current limiting selection */
  InParam.LLC_Mode = LNB_IOCTL_LLCMODE_NOCHANGE;
  Error = STTUNER_Ioctl(Handle, STTUNER_LNB_LNBH21, STTUNER_IOCTL_SETLNB, &InParam, NULL);
  if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"failed to set the lnb params");
        goto ioctl_end;
    }
  Error = STTUNER_Ioctl(Handle, STTUNER_LNB_LNBH21, STTUNER_IOCTL_GETLNB, NULL, &OutParam);
  if (Error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"failed to get the lnb params");
        goto ioctl_end;
    }
#endif /*defined(LNBH_21)*/
    /* ----------  END OF IOCTL TESTS ---------- */
  #endif /*ST_OSLINUX*/
ioctl_end:
    errc = STTUNER_Close(Handle);
    TUNER_DebugError("STTUNER_Close()", errc);

    TermParams.ForceTerminate = FALSE;
    errt = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", errt);

    /* OR errors together to find out if any happened */
    Error = Error | errc | errt;
#endif 
    if (Error == ST_NO_ERROR)
    {
        TUNER_TestPassed(Result);
    }

    return(Result);
}


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
    STTUNER_EventType_t Event, ExpectedEvent;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_Scan_t       ScanParams;
    U32 FStart, FEnd, loop;
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
    FStart = FLIMIT_LO;     /*changed from FStart = FLIMIT_HI;    */     
    FEnd   = FLIMIT_HI;     /*changed from FEnd   = FLIMIT_LO;    */

    /* Scan -------------------------------- */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
    
    TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                       TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                      ));

    /* Lock to a frequency ----------------- */
    
    ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
    ScanParams.FECRates     = TunerInfo.ScanInfo.FECRates;
    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.IQMode       = TunerInfo.ScanInfo.IQMode;

    ScanParams.ModeCode     = TunerInfo.ScanInfo.ModeCode;




    ScanParams.FECType     = TunerInfo.ScanInfo.FECType;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
    ScanParams.LNB_SignalRouting       = TunerInfo.ScanInfo.LNB_SignalRouting;
    #endif
    
    TUNER_DebugMessage("Tuning...");
    
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
    
    TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% Modulation = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                       TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                      ));
                           
                                    
    /* detect feed status ------------------ */

    for(loop = 1; loop <= RELOCK_TEST_NUMTIMES; loop++)
    {
        TUNER_DebugPrintf(("Pass %d of %d\n", loop, RELOCK_TEST_NUMTIMES));

        if (connect == FALSE)
        {
        	
            TUNER_DebugMessage(("Please DISCONNECT satellite feed\n"));
            ExpectedEvent = STTUNER_EV_UNLOCKED;
            connect = TRUE;
        }
        else
        {
        	
            TUNER_DebugMessage(("Please RECONNECT satellite feed\n" ));
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
        
        TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                           TunerInfo.Frequency,
                           TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                           TunerInfo.SignalQuality,
                           TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                           TunerInfo.BitErrorRate,
                           TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                           TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                           TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
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




/*-----------------------------------------------------------------------------------------
TunerTestDVBS2Modes()
Description:
    This routine tests all DVBS2 Modes.

    We then call the abort routine to ensure we are notified of the
    unlocking of the tuner.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
------------------------------------------------------------------------------------------*/
#if defined(TUNER_899)&&defined(TEST_DVBS2_MODE)
#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerTestDVBS2Modes(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t      error;
    STTUNER_Handle_t    TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_Scan_t       ScanParams;
    U8 index = 0;
    int FecMod[29][2] = {
    			 /*{STTUNER_MOD_QPSK,   STTUNER_FEC_1_4},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_1_3},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_2_5},*/
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_1_2},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_3_5},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_2_3},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_3_4},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_4_5},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_5_6},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_8_9},
    			 {STTUNER_MOD_QPSK,   STTUNER_FEC_9_10},
    			 {STTUNER_MOD_8PSK,   STTUNER_FEC_3_5},
    			 {STTUNER_MOD_8PSK,   STTUNER_FEC_2_3},
    			 {STTUNER_MOD_8PSK,   STTUNER_FEC_3_4},
    			 {STTUNER_MOD_8PSK,   STTUNER_FEC_5_6},
    			 {STTUNER_MOD_8PSK,   STTUNER_FEC_8_9},
    			 {STTUNER_MOD_8PSK,   STTUNER_FEC_9_10}
    			 /*{STTUNER_MOD_16APSK, STTUNER_FEC_2_3},
    			 {STTUNER_MOD_16APSK, STTUNER_FEC_3_4},
    			 {STTUNER_MOD_16APSK, STTUNER_FEC_4_5},
    			 {STTUNER_MOD_16APSK, STTUNER_FEC_5_6},
    			 {STTUNER_MOD_16APSK, STTUNER_FEC_8_9},
    			 {STTUNER_MOD_16APSK, STTUNER_FEC_9_10},
    			 {STTUNER_MOD_32APSK, STTUNER_FEC_3_4},
    			 {STTUNER_MOD_32APSK, STTUNER_FEC_4_5},
    			 {STTUNER_MOD_32APSK, STTUNER_FEC_5_6},
    			 {STTUNER_MOD_32APSK, STTUNER_FEC_8_9},
    			 {STTUNER_MOD_32APSK, STTUNER_FEC_9_10}*/
    			};
    char  ModCodLUT[][40] = {
             
		/*"FE_QPSK_14",*//* Not Supported in FPGA Modulator*/
		/*"FE_QPSK_13",
		"FE_QPSK_25",*/
		"FE_QPSK_12",    
		"FE_QPSK_35",    
		"FE_QPSK_23",    
		"FE_QPSK_34",    
		"FE_QPSK_45",    
		"FE_QPSK_56",    
		"FE_QPSK_89",    
		"FE_QPSK_910",   
		"FE_8PSK_35",    
		"FE_8PSK_23",    
		"FE_8PSK_34",    
		"FE_8PSK_56",    
		"FE_8PSK_89",    
		"FE_8PSK_910",   
		/*"FE_16APSK_23",   
		"FE_16APSK_34",  
		"FE_16APSK_45",  
		"FE_16APSK_56",  
		"FE_16APSK_89",  
		"FE_16APSK_910", 
		"FE_32APSK_34",  
		"FE_32APSK_45",  
		"FE_32APSK_56",  
		"FE_32APSK_89",  
		"FE_32APSK_910", */
		"\0"
	};
    

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
        goto dvbs2test_end;
    } 


    /* Setup scan -------------------------- */

    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Testing All DVBS2 FecModes\n", Params_p->Ref));
    
    TunerInfo.Frequency = 3700000;
    ScanParams.Polarization = STTUNER_LNB_OFF;
    ScanParams.FECRates     = STTUNER_FEC_NONE;
    ScanParams.Modulation   = STTUNER_MOD_NONE;
    ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanParams.Band         = 0;
    ScanParams.AGC          = STTUNER_AGC_ENABLE;
    ScanParams.SymbolRate   = 20000000;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF   = TunerInfo.ScanInfo.ScrParams.SCRBPF;
    #endif

    while(ModCodLUT[index] != "\0")
    {
    	TUNER_DebugPrintf(("SET FREQUENCY = %d  :: SYMBOLRATE = %d",TunerInfo.Frequency, ScanParams.SymbolRate));
    	TUNER_DebugPrintf(("SET FECMODE  %s   ::YOU HAVE 7 SECONDS TO SET MODULATOR", ModCodLUT[index]));
    	
    	STTUNER_TaskDelay(7*ST_GetClocksPerSecond());
    	
	    TUNER_DebugMessage("Tuning...");

	    error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
	    TUNER_DebugError("STTUNER_SetFrequency()", error);
	    if (error != ST_NO_ERROR)
	    {
	        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
	        goto dvbs2test_end;
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
	        TUNER_DebugPrintf(("FAILED AT FECMODE   %s ", ModCodLUT[index]));
	        /*goto dvbs2test_end;*/
	    }
	     /* Get tuner information */
	    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
	    if((TunerInfo.ScanInfo.FECRates != FecMod[index][1]) || (TunerInfo.ScanInfo.Modulation != FecMod[index][0]))
	    {
	    	TUNER_DebugPrintf(("EXPECTED FEC = %s  :: MODULATION = %s  BUT FOUND  FEC = %s  :: MODULATION = %s",
	    	                    TUNER_FecToString(FecMod[index][1]),
	    	                    TUNER_DemodToString(FecMod[index][0]),
	    	                    TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
	    	                    TUNER_DemodToString(TunerInfo.ScanInfo.Modulation)
	    	                  ));
	    	 TUNER_TestFailed(Result,"Expected NOT FOUND");
	    	 /*goto dvbs2test_end;*/
	     }
	   
	    
	    TUNER_DebugPrintf(("F = %u LNB = %s kHz Q = %u%% BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
	                       TunerInfo.Frequency,
	                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
	                       TunerInfo.SignalQuality,
	                       TunerInfo.BitErrorRate,
	                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
	                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
	                       TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
	                      ));
	    TUNER_DebugPrintf(("   Modulation=%s ToneState=%s \n",
                                   TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TUNER_ToneToString(TunerInfo.ScanInfo.LNBToneState)
                                  ));
	                      
	   index++;
	
   }

dvbs2test_end:

    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
}
#endif  /* Only for 899 DVBS2 modes*/
#endif
/*****************************************************************************
TunerDiseqc()
Description:
    Test to generate Tone corresponding to different DiSEqC Commands
    Tones should checked by Oscilloscope /LA
Parameters:

Return Value:

See Also:
    Nothing.
*****************************************************************************/
/***************/

ST_ErrorCode_t TunerDiseqc(STTUNER_Handle_t TUNERHandle)
{
	ST_ErrorCode_t Error;

	/*TUNER_TestResult_t Result = TEST_RESULT_ZERO;*/

	/*STTUNER_OpenParams_t OpenParams;*/

	unsigned char command_array [5] ={ FB_COMMAND_NO_REPLY, AB_LNB, CB_CLR_CONTEND, 55, 55};

	unsigned char TestSeq [7] = {
							 STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH,
							 STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON ,
	   						 STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW,
						/*3*/	 STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED ,
						/*4*/	 STTUNER_DiSEqC_TONE_BURST_SEND_1,
						/*5*/	 STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED,
							 STTUNER_DiSEqC_COMMAND
							};
							
unsigned char  Testno=0;
STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
void *pDiSEqCReceivePacket=NULL;
unsigned int temp;


TUNER_DebugMessage("Before lnb off  \n");

Error = STTUNER_SetLNBConfig(TUNERHandle, STTUNER_LNB_TONE_OFF,STTUNER_PLR_HORIZONTAL);
  
	if(Error != ST_NO_ERROR)
	{
        TUNER_DebugMessage("Failed LNB TONE OFF function" );
	}
	else {
		TUNER_DebugMessage("**********!LNB_TONE_OFF!!!!!!!!!!" );
	}

	STTUNER_TaskDelay(10000);

	TUNER_DebugMessage("LNB OFF\n");

	/*Error = STTUNER_SetLNBToneState(TUNERHandle, STTUNER_LNB_TONE_22KHZ);
	if(Error != ST_NO_ERROR){
        TUNER_DebugMessage( "********** Failed  LNB TONE ON !!!!!!!!!!!");

							}

	else {
		TUNER_DebugMessage("TONE ON!!!! LNB TONE ON!!!!" );
	}
	TUNER_DebugMessage("LNB ON\n");*/

    STTUNER_TaskDelay(10);
	
	/**********************************************/

	DiSEqCSendPacket.uc_TotalNoOfBytes =5;
DiSEqCSendPacket.DiSEqCCommandType= STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON/*STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH  STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH*/;

/*TUNER_DebugPrintf(("STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON"));*/
DiSEqCSendPacket.pFrmAddCmdData = &command_array[0] ;	/*		Pointer to data to be sent; Data has to be sequentially placed*/

DiSEqCSendPacket.uc_msecBeforeNextCommand =0;	/*Time gap required in milliseconds (ms) */

Error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,
									      pDiSEqCReceivePacket
  								     );
TUNER_DebugMessage("DISEqc continuous\n");


/*DiSEqC_Delay_ms(0x1f);*/

DiSEqCSendPacket.DiSEqCCommandType= STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH  ;


Error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,
									      pDiSEqCReceivePacket
 								     );
TUNER_DebugMessage("diseqc high\n");


/*for (Testno =0 ;Testno <7;Testno++)*/
for(;;)

	{DiSEqCSendPacket.DiSEqCCommandType=  TestSeq[5] ;


	TUNER_DebugPrintf((" --- DiSEqC Test no #%d ---\n",Testno ));
	TUNER_DebugPrintf(("proceed to see different tests on different diseqc command \n"));
	
	Error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,

									  pDiSEqCReceivePacket);

	   TUNER_DebugError("STTUNER_DiSEqCSendReceive()", Error);

	DiSEqC_Delay_ms(0x50);

	}

	DiSEqCSendPacket.uc_msecBeforeNextCommand =20;	/*Time gap required in milliseconds (ms) */

/*for checking time delay before next command*/
for (temp =0 ;temp <0x500;temp++)
	{
	                DiSEqCSendPacket.DiSEqCCommandType=  STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW ;
		DiSEqCSendPacket.uc_msecBeforeNextCommand =2;
		TUNER_DebugPrintf((" STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW\n"));

		Error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,
												  pDiSEqCReceivePacket );

		DiSEqCSendPacket.uc_msecBeforeNextCommand =20;
		DiSEqCSendPacket.DiSEqCCommandType= STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH /*STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON  */ ;

		TUNER_DebugPrintf((" STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH\n"));

		Error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,
											  pDiSEqCReceivePacket);

		}


return(Error);
}

#if defined(TEST_DISEQC_5100) || defined(TEST_DISEQC_5301)
/*****************************************************************************
TunerDiseqcBackend()
Description:
    
Parameters:

Return Value:

See Also:
    Nothing.
*****************************************************************************/
/***************/

ST_ErrorCode_t TunerDiseqcBackend(STTUNER_Handle_t TUNERHandle)
{
  ST_ErrorCode_t Error;
  STPIO_Handle_t          HandlePIO;
  int i;
   STPIO_OpenParams_t      OpenParamsPIO;
   static ST_DeviceName_t      DeviceName[] =
   {
    "STPIO[0]",
    "STPIO[1]",
    "STPIO[2]",
    "STPIO[3]",
    "STPIO[4]",
    "STPIO[5]",     /* 14, 16, 17, 28 only */
    "STPIO[6]",     /* 28 only */
    "STPIO[7]"      /* 28 only */
 };

	

	unsigned char command_array [21] ={ /*FB_COMMAND_REPLY*/0x55,0x00, 0x00, 0x01, 0x02,0,1,9,0,0,0,0,0,0,0,0,0,0,0,0,0};

	/*unsigned char TestSeq [7] ={
							 STTUNER_DiSEqC_TONE_BURST_OFF_F22_HIGH,
							 STTUNER_DiSEqC_CONTINUOUS_TONE_BURST_ON ,
	   						 STTUNER_DiSEqC_TONE_BURST_OFF_F22_LOW,
							 STTUNER_DiSEqC_TONE_BURST_SEND_0_MODULATED ,
							 STTUNER_DiSEqC_TONE_BURST_SEND_1,
							 STTUNER_DiSEqC_TONE_BURST_SEND_0_UNMODULATED,
							 STTUNER_DiSEqC_COMMAND
							};*//**/



STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
STTUNER_DiSEqCResponsePacket_t pDiSEqCReceivePacket;



    OpenParamsPIO.ReservedBits    = TunerInitParams[0].Init_Diseqc.PIOPinTx | TunerInitParams[0].Init_Diseqc.PIOPinRx; 
    OpenParamsPIO.BitConfigure[7] = STPIO_BIT_ALTERNATE_OUTPUT;
    OpenParamsPIO.BitConfigure[6] = STPIO_BIT_INPUT;
    OpenParamsPIO.IntHandler      = NULL;

    Error = STPIO_Open(DeviceName[TunerInitParams[0].Init_Diseqc.PIOPort], &OpenParamsPIO, &HandlePIO);
    TUNER_DebugError("STTUNER_Open()", Error);
    if (Error != ST_NO_ERROR)
    {
       goto simple_end; 
    }

/***/
#if 0
for (;;){
Error = STPIO_Write(HandlePIO,1);
TUNER_DebugError("STTUNER_Open()", Error);
    if (Error != ST_NO_ERROR)
    {
       goto simple_end; 
    }
    STTUNER_TaskDelay(10000);
    Error = STPIO_Write(HandlePIO,0);
    TUNER_DebugError("STTUNER_Open()", Error);
    if (Error != ST_NO_ERROR)
    {
       goto simple_end; 
    }
    STTUNER_TaskDelay(10000);
}
#endif

/***/

   TUNER_DebugMessage("Before lnb off  \n");

    Error = STTUNER_SetLNBConfig(TUNERHandle, STTUNER_LNB_TONE_OFF, STTUNER_PLR_HORIZONTAL);
  
	if(Error != ST_NO_ERROR){
        TUNER_DebugMessage("Failed LNB TONE OFF function" );

							}

	else {
		TUNER_DebugMessage("**********!LNB_TONE_OFF!!!!!!!!!!" );
	}

	STTUNER_TaskDelay(10000);

	TUNER_DebugMessage("LNB OFF\n");
    STTUNER_TaskDelay(10);
	
	/**********************************************/

DiSEqCSendPacket.uc_TotalNoOfBytes =3;
DiSEqCSendPacket.DiSEqCCommandType= STTUNER_DiSEqC_COMMAND;
DiSEqCSendPacket.pFrmAddCmdData = &command_array[0] ;	/*		Pointer to data to be sent; Data has to be sequentially placed*/

DiSEqCSendPacket.uc_msecBeforeNextCommand =0;	/*Time gap required in milliseconds (ms) */

/*for (i=0;i<100;i++)*/
for(;;)
{

Error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,
									      &pDiSEqCReceivePacket
  								     );
  /* Check the reply byte */
   TUNER_DebugPrintf(("Number of reply bytes received: %d\n",pDiSEqCReceivePacket.uc_TotalBytesReceived));
   for(i=0;i<pDiSEqCReceivePacket.uc_TotalBytesReceived;i++)
      {
       	TUNER_DebugPrintf(("0x%x\t\n",pDiSEqCReceivePacket.ReceivedFrmAddData[i]));
      }
  TUNER_DebugPrintf(("\n"));
  STTUNER_TaskDelay(15625);
}  								     
TUNER_DebugMessage("DISEqc continuous\n");


simple_end:
	Error = STPIO_Close(HandlePIO);
	TUNER_DebugError("STPIO_Close()", Error);        
       
return(Error);
}

#endif

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
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FTmp;
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


    /* We compile for low/high band as the feed must come from a different satellite dish. */
#ifdef TUNER_LOW_BAND
    TUNER_DebugPrintf(("%d.0: Scan LOW BAND\n", Params_p->Ref));

    #ifdef C_BAND_TESTING
    FEnd   = 5150000;
    FStart = 3500000;
    #else /* Ku band is default */
    FEnd   = 10600000;
    FStart = 11700000;
    #endif /* ifdef C_BAND_TESTING */

#else
    TUNER_DebugPrintf(("%d.0: Scan HIGH BAND\n", Params_p->Ref));
    FEnd   = FLIMIT_HI;
    FStart = FLIMIT_LO;
#endif


#ifdef DISEQC_TEST_ON

/*************/

    /*** Function TunerDiseqc() is used to test the different commands for DiSEqC module . ***
	 ***To make it on make the #undef DISEQC_TEST_ON to #define DISEQC_TEST_ON in the same file***
	 ***sat_test.c .  For normal Tuner Scan Test TunerDiseqc  should not be called
	******************************/


	error= TunerDiseqc( TUNERHandle);


/***********/

#endif
#if defined(TEST_DISEQC_5100) ||  defined(TEST_DISEQC_5301)
TunerDiseqcBackend (TUNERHandle) ;
#endif

    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);

    for (i = 0; i < 2; i++)
    {
        /* initiate scan */
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
                
                TUNER_DebugPrintf(("%02d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm S = %u IQ = %d\n",
                                   ChannelCount,TunerInfo.Frequency,TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                                   TunerInfo.SignalQuality,TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TunerInfo.BitErrorRate,TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                                   TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                                   TunerInfo.ScanInfo.SymbolRate, TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                                 ));

                TUNER_DebugPrintf(("   Modulation=%s ToneState=%s Band=%d\n",
                                   TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TUNER_ToneToString(TunerInfo.ScanInfo.LNBToneState),
                                   TunerInfo.ScanInfo.Band
                                  ));

                ChannelCount++;
            }
            else if (Event == STTUNER_EV_SCAN_FAILED)
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
                break;
            }
            else
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            }

            if(Event != STTUNER_EV_LNB_FAILURE)
            {
            STTUNER_ScanContinue(TUNERHandle, 0);
            }

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



#ifdef TUNER_288
static TUNER_TestResult_t TunerBlindScan(TUNER_TestParams_t *Params_p)
{
	
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FTmp;
       
    U32 ChannelCount = 0;
    STTUNER_BlindScan_t BlindScanParams;
    U8 i;
    
    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        goto simple_end;
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
        goto simple_end;
    }
    
  
    /* We compile for low/high band as the feed must come from a different satellite dish. */
#ifdef TUNER_LOW_BAND
    TUNER_DebugPrintf(("%d.0: Scan LOW BAND\n", Params_p->Ref));

    #ifdef C_BAND_TESTING
    FEnd   = 5150000;
    FStart = 3500000;
    #else /* Ku band is default */
    FEnd   = 10600000;
    FStart = 11700000;
    #endif /* ifdef C_BAND_TESTING */

#else
    TUNER_DebugPrintf(("%d.0: Scan HIGH BAND\n", Params_p->Ref));
    FEnd   = FLIMIT_HI;
    FStart = FLIMIT_LO;
#endif

#ifdef USA_STACK_LNB
   FEnd   = 3760000;
   FStart = 3740000;
#else	
    FEnd   = 4200000;
    FStart = 3600000;
#endif
#ifdef DISEQC_TEST_ON

/*************/

    /*** Function TunerDiseqc() is used to test the different commands for DiSEqC module . ***
	 ***To make it on make the #undef DISEQC_TEST_ON to #define DISEQC_TEST_ON in the same file***
	 ***sat_test.c .  For normal Tuner Scan Test TunerDiseqc  should not be called
	******************************/
	error= TunerDiseqc( TUNERHandle);
	
/***********/

#endif

#ifdef TEST_DISEQC_5100
TunerDiseqcBackend (TUNERHandle) ;
#endif


    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

	BlindScanParams.Polarization = STTUNER_PLR_HORIZONTAL | STTUNER_PLR_VERTICAL;
	BlindScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
	BlindScanParams.Band = 0;
	#ifdef SAT_TEST_SCR
		BlindScanParams.ScrParams.LNBIndex = 1;
		BlindScanParams.ScrParams.SCRBPF = 1;
		BlindScanParams.LNB_SignalRouting = STTUNER_VIA_SCRENABLED_LNB;
	#endif
      for (i = 0; i <2; i++)
    {
    	/* initiate scan */
        error = STTUNER_BlindScan(TUNERHandle, FStart, FEnd, 0, &BlindScanParams,0);
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
                /* Store channel information in channel table */
            /*BlindScan_ChannelTable[ChannelCount].F = TunerInfo.Frequency;
            memcpy(&BlindScan_ChannelTable[ChannelCount].ScanInfo, &TunerInfo.ScanInfo, sizeof(STTUNER_Scan_t) );*/

            /* Increment channel count */
            ChannelCount++;
            TUNER_DebugPrintf(("%02d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm S = %u IQ = %d\n",
                                   ChannelCount, TunerInfo.Frequency,
                                   TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                                   TunerInfo.SignalQuality,TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                                   TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                                   TunerInfo.ScanInfo.SymbolRate, TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                                 ));

                TUNER_DebugPrintf(("   Modulation=%s ToneState=%s Band=%d\n",
                                   TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TUNER_ToneToString(TunerInfo.ScanInfo.LNBToneState),
                                   TunerInfo.ScanInfo.Band
                                  ));

               
            }
            else if (Event == STTUNER_EV_SCAN_FAILED)
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
                break;
            }
            else
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            }

            if(Event != STTUNER_EV_LNB_FAILURE)
            {
            STTUNER_ScanContinue(TUNERHandle, 0);
            }

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

   /*BlindScan_ChannelTableCount = ChannelCount;*/
simple_end:
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);

    return Result;
} /* TunerSimple() */
#endif
#ifdef TUNER_288
static TUNER_TestResult_t TunerTimedBlindScan(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd;
    U32 ChannelCount = 0;
    STTUNER_BlindScan_t BlindScanParams;
    
    #ifdef ST_OS21    
    osclock_t Time;
    #else
    clock_t Time;
    #endif    
    
    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        goto simple_end;
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
        goto simple_end;
    }

    /* We compile for low/high band as the feed must come from a different satellite dish. */
#ifdef TUNER_LOW_BAND
    TUNER_DebugPrintf(("%d.0: Scan LOW BAND\n", Params_p->Ref));

    #ifdef C_BAND_TESTING
    FEnd   = 5150000;
    FStart = 3500000;
    #else /* Ku band is default */
    FEnd   = 10600000;
    FStart = 11700000;
    #endif /* ifdef C_BAND_TESTING */

#else
    TUNER_DebugPrintf(("%d.0: Scan HIGH BAND\n", Params_p->Ref));
    FEnd   = FLIMIT_HI;
    FStart = FLIMIT_LO;
#endif
   #ifdef USA_STACK_LNB
   FEnd   = 3760000;
   FStart = 3740000;
#else
    FEnd   = 4200000;
    FStart = 3600000;
#endif

#ifdef DISEQC_TEST_ON

/*************/

    /*** Function TunerDiseqc() is used to test the different commands for DiSEqC module . ***
	 ***To make it on make the #undef DISEQC_TEST_ON to #define DISEQC_TEST_ON in the same file***
	 ***sat_test.c .  For normal Tuner Scan Test TunerDiseqc  should not be called
	******************************/


	error= TunerDiseqc( TUNERHandle);


/***********/

#endif
#ifdef TEST_DISEQC_5100
TunerDiseqcBackend (TUNERHandle) ;
#endif

    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

BlindScanParams.Polarization = STTUNER_PLR_HORIZONTAL | STTUNER_PLR_VERTICAL;
BlindScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
BlindScanParams.Band = 0;
#ifdef SAT_TEST_SCR
		BlindScanParams.ScrParams.LNBIndex = 0;
		BlindScanParams.ScrParams.SCRBPF = 0;
		BlindScanParams.LNB_SignalRouting = STTUNER_VIA_SCRENABLED_LNB;
	#endif

TUNER_DebugMessage("Scanning...");
        /* initiate scan */
        Time = STOS_time_now();
        error = STTUNER_BlindScan(TUNERHandle, FStart, FEnd, 0, &BlindScanParams,0);
        TUNER_DebugError("STTUNER_BlindScan()", error);
        if (error != ST_NO_ERROR)
        {
            TUNER_TestFailed(Result, "Unable to commence tuner scan");
            goto simple_end;
        }

        
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
                /* Store channel information in channel table */
            BlindScan_ChannelTable[ChannelCount].F = TunerInfo.Frequency;
            memcpy(&BlindScan_ChannelTable[ChannelCount].ScanInfo, &TunerInfo.ScanInfo, sizeof(STTUNER_Scan_t) );

            /* Increment channel count */
            ChannelCount++;
                           
            }
            else if (Event == STTUNER_EV_SCAN_FAILED)
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
                break;
            }
            else
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            }

            if(Event != STTUNER_EV_LNB_FAILURE)
            {
            STTUNER_ScanContinue(TUNERHandle, 0);
            }

        }   /* for() */
        
        Time = (1000 * STOS_time_minus(STOS_time_now(), Time)) / ST_GetClocksPerSecond();
        
    BlindScan_ChannelTableCount = ChannelCount;

        if (ChannelCount > 0)
        {
            TUNER_DebugPrintf(("ChannelCount = %u   Time = %d\n", ChannelCount, (int)Time));
            TUNER_TestPassed(Result);
        }
        else
        {
            TUNER_TestFailed(Result, "no channels found");
        }


   simple_end:
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);

    return Result;
} /* TunerSimple() */
#endif

#if defined(TUNER_299) || defined(TUNER_399) || defined(DUAL_STEM_299) || defined(TUNER_288)
static TUNER_TestResult_t TunerScanKuBand(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FTmp;
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
   
    /*NSS6 Horizontal & LowBand Freq*/
    FEnd   = 11215000;
    FStart = 11209000;


    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);

    for (i = 0; i < 2; i++)
    {
        /* initiate scan */
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
                
                TUNER_DebugPrintf(("%02d F = %u (%s) kHz Q = %u%% BER = %5d FEC = %s AGC = %sdBm S = %u IQ = %d\n",
                                   ChannelCount,
                                   TunerInfo.Frequency,
                                   TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                                   TunerInfo.SignalQuality,
                                   TunerInfo.BitErrorRate,
                                   TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                                   TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                                   TunerInfo.ScanInfo.SymbolRate, TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                                 ));

                TUNER_DebugPrintf(("   Modulation=%s ToneState=%s Band=%d\n",
                                   TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TUNER_ToneToString(TunerInfo.ScanInfo.LNBToneState),
                                   TunerInfo.ScanInfo.Band
                                  ));

                ChannelCount++;
            }
            else if (Event == STTUNER_EV_SCAN_FAILED)
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
                break;
            }
            else
            {
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            }

            if(Event != STTUNER_EV_LNB_FAILURE)
            {
            STTUNER_ScanContinue(TUNERHandle, 0);
            }

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
    }
#endif
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
    STTUNER_EventType_t Event;
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
            
            TUNER_DebugPrintf(("%02d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm S = %u IQ = %d\n",
                               ChannelCount, TunerInfo.Frequency,TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                                TunerInfo.SignalQuality,TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                TunerInfo.BitErrorRate,TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
            ChannelCount++;
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_DebugPrintf(("Tried F = %u (%s) kHz FEC = %s S = %u\n",
                               ChannelTable[i].F,
                               TUNER_PolarityToString(ChannelTable[i].ScanInfo.Polarization),
                               TUNER_FecToString(ChannelTable[i].ScanInfo.FECRates),
                               ChannelTable[i].ScanInfo.SymbolRate
                              ));
        }
   

     }  /* for(i) */


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

#ifdef TUNER_288
#ifdef DEBUG_288
/*****************************************************************************
TunerBlindScanExact()
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

static TUNER_TestResult_t TunerBlindScanExact(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 ChannelCount = 0;
    U32 i;
    #ifdef ST_OS21    
    osclock_t StartClocks, TotalClocks;
#else
    clock_t StartClocks, TotalClocks;
#endif    

    if (BlindScan_ChannelTableCount < 1)
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
    for (i = 0; i < BlindScan_ChannelTableCount; i++)
    {
    	
	StartClocks = STOS_time_now();
        STTUNER_SetFrequency(TUNERHandle, BlindScan_ChannelTable[i].F, &BlindScan_ChannelTable[i].ScanInfo, 0);
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
        	TotalClocks = (1000 * STOS_time_minus(STOS_time_now(), StartClocks)) /  ST_GetClocksPerSecond();
   
    	    TUNER_DebugPrintf(("Time taken = %u ms.\n", TotalClocks));
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            
            TUNER_DebugPrintf(("%02d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm S = %u IQ = %d\n",
                               ChannelCount, TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.SymbolRate,
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
            ChannelCount++;
            
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_DebugPrintf(("Tried F = %u (%s) kHz FEC = %s S = %u\n",
                               BlindScan_ChannelTable[i].F,
                               TUNER_PolarityToString(BlindScan_ChannelTable[i].ScanInfo.Polarization),
                               TUNER_FecToString(BlindScan_ChannelTable[i].ScanInfo.FECRates),
                               BlindScan_ChannelTable[i].ScanInfo.SymbolRate
                              ));
        }
   

     }  /* for(i) */


    TUNER_DebugPrintf(("Channels = %u / %u\n", ChannelCount, BlindScan_ChannelTableCount));
    if (ChannelCount == BlindScan_ChannelTableCount)
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

tse_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);
    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);
    return(Result);
} /* TunerScanExact() */
#endif
#endif

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
    STTUNER_EventType_t Event;
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
    TUNER_DebugPrintf(("Time taken = %u ms.\n",(int) TotalClocks));


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

#ifdef TUNER_SIGNAL_CHANGE
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
#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerSignalChange(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_Status_t Status;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 i, f;
    BOOL SignalNoiseTestEnable = TRUE;

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
        goto tsc_end;
    }


    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    /* Setup threshold checking based on CN ratio */
    Thresholds.ThresholdList = ThresholdList;
    Thresholds.NumElements = 3;
    ThresholdList[0].SignalLow  =  0; /* Bad */
    ThresholdList[0].SignalHigh = 60;

    ThresholdList[1].SignalLow  = 60; /* Normal */
    ThresholdList[1].SignalHigh = 85;

    ThresholdList[2].SignalLow  =  85; /* Good */
    ThresholdList[2].SignalHigh = 100;

    error = STTUNER_SetThresholdList(TUNERHandle, &Thresholds, STTUNER_QUALITY_CN);
    TUNER_DebugError("STTUNER_SetThresholdList()", error);

    /* choose a good signal (test may stall with a low quality signal - too low a delta) */
    f = 0;
    while(1)
    {

        /* attempt a set frequency */
        TUNER_DebugPrintf((" Trying F = %u (%s) kHz FEC = %s S = %u\n",
                               ChannelTable[f].F,
                               TUNER_PolarityToString(ChannelTable[f].ScanInfo.Polarization),
                               TUNER_FecToString(ChannelTable[f].ScanInfo.FECRates),
                               ChannelTable[f].ScanInfo.SymbolRate ));

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
        error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

        if (TunerInfo.SignalQuality > 80)
        {
             break;
        }
        else
        {
            TUNER_DebugPrintf((" Rejecting, Q = %u%% \n", TunerInfo.SignalQuality));
        }

        f++;
        if (f >= ChannelTableCount)
        {
            TUNER_TestFailed(Result,"Cannot find a good enough signal (Q > 80%)");
            /*goto tsc_end;*/
            f--;
            break;
        }
   }

    TUNER_DebugPrintf((" Selecting F = %u (%s) kHz FEC = %s S = %u Q = %u%%\n",
                           ChannelTable[f].F,
                           TUNER_PolarityToString(ChannelTable[f].ScanInfo.Polarization),
                           TUNER_FecToString(ChannelTable[f].ScanInfo.FECRates),
                           ChannelTable[f].ScanInfo.SymbolRate,
                           TunerInfo.SignalQuality
                     ));

    /* enable random BER generation (by sttuner software) for this test */
    error = STTUNER_Ioctl(TUNERHandle, STTUNER_SOFT_ID, STTUNER_IOCTL_TEST_SN, &SignalNoiseTestEnable, NULL);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tsc_end;
    }


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
            
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %6d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        }
    }   /* for(i) */

    /* Setup signal checking based on bit error rate */
    Thresholds.ThresholdList = ThresholdList;
    Thresholds.NumElements = 3;
    ThresholdList[0].SignalLow  = 100000; /* Bad */
    ThresholdList[0].SignalHigh = 200000;

    ThresholdList[1].SignalLow  =  50000; /* Normal */
    ThresholdList[1].SignalHigh = 100000;

    ThresholdList[2].SignalLow  =      0; /* Good */
    ThresholdList[2].SignalHigh =  50000;

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
            
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %6d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
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
#endif
#endif

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
#ifndef ST_OSLINUX
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
            TUNER_TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER' ");
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
        error = STTUNER_SetBandList(STTUNER_MAX_HANDLES, &Bands);
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
 error = STTUNER_SetScanList(STTUNER_MAX_HANDLES, &Scans);
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
#ifdef ST_OSLINUX
error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, &Bands);
#else
        error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, NULL);
#endif
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

#ifdef ST_OSLINUX
        error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, &Scans);
#else
        error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, NULL);
#endif

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
        #ifndef ST_OSLINUX
        S32 retval, before, after;
        #endif
        BOOL failed = FALSE;
#ifndef ST_OSLINUX
        partition_status_t       status;
        partition_status_flags_t flags;
#endif

#ifndef ST_OSLINUX
        /* setup parameters */
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
#endif


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
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_Scan_t       ScanParams;
    U32 FStart, FEnd, i;

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

    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;

    /* Now attempt a scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
            
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
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

    ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
    ScanParams.FECRates     = TunerInfo.ScanInfo.FECRates;
    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.IQMode       = TunerInfo.ScanInfo.IQMode;
    ScanParams.FECType      = TunerInfo.ScanInfo.FECType;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
    ScanParams.LNB_SignalRouting       = TunerInfo.ScanInfo.LNB_SignalRouting;
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
                /*goto tracking_end;*/
            }
            TUNER_DebugPrintf(("\n"));
            TUNER_DebugPrintf(("TunerStatus: %s\n", TUNER_StatusToString(TunerInfo.Status)));
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
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
#if defined(TUNER_299) || defined(TUNER_399) || defined(DUAL_STEM_299)
/*****************************************************************************
IQStateTest()
Description:
    This routine scans to the first frequency.  We then try and change the IQ
    state to see if the tuner unlocks.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerIQTest(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd;
    /*U32 i;*/
    /*STTUNER_Status_t Status;*/
    STTUNER_IQMode_t RightIQMode,WrongIQMode;
    U8 Loop;
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
        goto iq_end;
    }


    /* Test done twice so that Scan and SetFrequency can be tested. */
    for(Loop=0;Loop<2;Loop++)
    {

        /* Setup scan and band lists */
        STTUNER_SetBandList(TUNERHandle, &Bands);
        STTUNER_SetScanList(TUNERHandle, &Scans);


        TUNER_DebugPrintf(("%d.%d: Manipulate IQ sense of demod\n", Params_p->Ref,Loop));
        TUNER_DebugMessage("Purpose: Scan to the first frequency lock and change the IQ sense and try and re-lock, first with scan and then with SetFrequency");

        FStart = FLIMIT_LO;
        FEnd   = FLIMIT_HI;

        /* Now attempt a scan */
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
                
                TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));

                if (Loop==1) /* Second pass through */
                {
                    ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
                    ScanParams.FECRates     = TunerInfo.ScanInfo.FECRates;
                    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
                    ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
                    ScanParams.Band         = TunerInfo.ScanInfo.Band;
                    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
                    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
                    ScanParams.FECType      = TunerInfo.ScanInfo.FECType;
                    #ifdef SAT_TEST_SCR
		    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
		    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
		    ScanParams.LNB_SignalRouting       = TunerInfo.ScanInfo.LNB_SignalRouting;
		    #endif

                }

                if (TunerInfo.ScanInfo.IQMode==STTUNER_IQ_MODE_NORMAL)
                {
                    TUNER_DebugPrintf(("STTUNER_GetInfo reports IQ mode as NORMAL\n"));
                    RightIQMode=STTUNER_IQ_MODE_NORMAL;
                    WrongIQMode=STTUNER_IQ_MODE_INVERTED;
                }
                else
                {
                    TUNER_DebugPrintf(("Get Info reports IQ mode as INVERTED\n"));
                    RightIQMode=STTUNER_IQ_MODE_INVERTED;
                    WrongIQMode=STTUNER_IQ_MODE_NORMAL;
                }

            }
            else
            {
                TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
                TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
                goto iq_end;
            }
        }
        else
        {
            TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            goto iq_end;
        }

        /* Unlock to clear the scanning process */
        error = STTUNER_Unlock(TUNERHandle);

        if (Loop==0) /* First pass */
        {

            /* Change the IQ sense to one that shouldn't work */
            Scans.ScanList[0].IQMode=WrongIQMode;
            Scans.ScanList[1].IQMode=WrongIQMode;
            Scans.ScanList[2].IQMode=WrongIQMode;

            /* Setup scan and band lists */
            STTUNER_SetBandList(TUNERHandle, &Bands);
            STTUNER_SetScanList(TUNERHandle, &Scans);

            FStart = FLIMIT_LO;
            FEnd   = FLIMIT_HI;

            /* Now attempt a scan */
            error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
            TUNER_DebugError("STTUNER_Scan()", error);

        }
        else
        {
            ScanParams.IQMode=WrongIQMode;
            error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
            TUNER_DebugError("STTUNER_SetFrequency()", error);

        }

        if (error == ST_NO_ERROR)
        {
            if(Loop==0)
            {
                TUNER_DebugMessage("Scanning...");
            }
            else
            {
                TUNER_DebugMessage("Set Frequency ...");
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

            if(Loop==0)
            {
                /* Check whether or not we're locked */
                if ((Event != STTUNER_EV_TIMEOUT)&&(Event != STTUNER_EV_SCAN_FAILED))
                {
                    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
                    TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_TIMEOUT/STTUNER_EV_SCAN_FAILED'");
                    goto iq_end;
                }
                else
                {
                    TUNER_DebugPrintf(("Timeout or SCAN FAILED - Expected result as IQ has been inverted from working IQ.\n"));
                }
            }
            else
            {
                /* Check whether or not we're locked */
                if (Event != STTUNER_EV_SCAN_FAILED)
                {
                    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
                    TUNER_TestFailed(Result, "STTUNER_EV_SCAN_FAILED Expected = 'STTUNER_EV_SCAN_FAILED'");
                    goto iq_end;
                }
                else
                {
                    TUNER_DebugPrintf(("STTUNER_EV_SCAN_FAILED - Expected result as IQ has been inverted from working IQ.\n"));
                }
            }
        }
        else
        {
            TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            goto iq_end;
        }

        /* Unlock to clear the scanning process */
        error = STTUNER_Unlock(TUNERHandle);
        TUNER_DebugPrintf(("Tuner unlocked.\n"));

        if(Loop==0)
        {
            /* Need to wait for algorithm under the bonnet to end */
            AwaitNotifyFunction1(&Event);
            TUNER_DebugPrintf(("Event = %s. Scan has really finished now.\n", TUNER_EventToString(Event) ));
        }

        if(Loop==0)
        {
            /* Change the IQ sense to one that shouldn't work */
            Scans.ScanList[0].IQMode=RightIQMode;
            Scans.ScanList[1].IQMode=RightIQMode;
            Scans.ScanList[2].IQMode=RightIQMode;

            /* Setup scan and band lists */
            STTUNER_SetBandList(TUNERHandle, &Bands);
            STTUNER_SetScanList(TUNERHandle, &Scans);

            FStart = FLIMIT_LO;
            FEnd   = FLIMIT_HI;

            /* Now attempt a scan */
            error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
            TUNER_DebugError("STTUNER_Scan()", error);

        }
        else
        {

            ScanParams.IQMode=RightIQMode;

            error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
            TUNER_DebugError("STTUNER_SetFrequency()", error);

        }

        if (error == ST_NO_ERROR)
        {
            if(Loop==0)
            {
                TUNER_DebugMessage("Scanning...");
            }
            else
            {
                TUNER_DebugMessage("Set Frequency ...");
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

            /* Check whether or not we're locked */
            if (Event == STTUNER_EV_LOCKED)
            {

                error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
                
                TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));

                if (TunerInfo.ScanInfo.IQMode==STTUNER_IQ_MODE_NORMAL)
                {
                    TUNER_DebugPrintf(("STTUNER_GetInfo reports IQ mode as NORMAL\n"));
                }
                else
                    TUNER_DebugPrintf(("Get Info reports IQ mode as INVERTED\n"));

                TUNER_DebugPrintf(("Now locks with correct IQ sense \n"));

            }
            else
            {
                TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
                TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
                goto iq_end;
            }
        }
        else
        {
            TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            goto iq_end;
        }

    } /* Test loop "Loop" */

    TUNER_TestPassed(Result);

iq_end:


    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);

}
#endif

#if defined(TUNER_299) || defined(TUNER_399)  || defined(DUAL_STEM_299)
/*****************************************************************************
TunerTerminateSearchTestTest()
Description:

Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerTerminateSearchTest(TUNER_TestParams_t *Params_p)
{

    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd;
    /*FTmp;*/
    U32 ChannelCount = 0;
    /*U8 i;*/
    STTUNER_IQMode_t RightIQMode,WrongIQMode;



    TUNER_DebugPrintf(("%d: Terminate a search", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Terminate a search whilst its searching");

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        goto simple_end;
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
        goto simple_end;
    }


    /* We compile for low/high band as the feed must come from a different satellite dish. */
#ifdef TUNER_LOW_BAND
    TUNER_DebugPrintf(("%d.0: Scan LOW BAND\n", Params_p->Ref));
    FEnd   = 10600000;
    FStart = 11700000;
#else
    TUNER_DebugPrintf(("%d.0: Scan HIGH BAND\n", Params_p->Ref));
    FEnd   = FLIMIT_HI;
    FStart = FLIMIT_LO;
#endif


    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);

    /* initiate scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
            
            TUNER_DebugPrintf(("%02d F = %u (%s) kHz Q = %u%% BER = %5d FEC = %s AGC = %sdBm S = %u IQ = %d\n",
                                   ChannelCount,
                                   TunerInfo.Frequency,
                                   TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                                   TunerInfo.SignalQuality,
                                   TunerInfo.BitErrorRate,
                                   TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                                   TunerInfo.ScanInfo.SymbolRate, TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));

            TUNER_DebugPrintf(("   Modulation=%s ToneState=%s Band=%d\n",
                                   TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                                   TUNER_ToneToString(TunerInfo.ScanInfo.LNBToneState),
                                   TunerInfo.ScanInfo.Band
                                  ));

            /* Switch the IQ so that it won't lock */
            if (TunerInfo.ScanInfo.IQMode==STTUNER_IQ_MODE_NORMAL)
            {
                TUNER_DebugPrintf(("STTUNER_GetInfo reports IQ mode as NORMAL\n"));
                RightIQMode=STTUNER_IQ_MODE_NORMAL;
                WrongIQMode=STTUNER_IQ_MODE_INVERTED;
            }
            else
            {
                TUNER_DebugPrintf(("Get Info reports IQ mode as INVERTED\n"));
                RightIQMode=STTUNER_IQ_MODE_INVERTED;
                WrongIQMode=STTUNER_IQ_MODE_NORMAL;
            }

            /* Change the IQ sense to one that shouldn't work */
            Scans.ScanList[0].IQMode=WrongIQMode;
            Scans.ScanList[1].IQMode=WrongIQMode;
            Scans.ScanList[2].IQMode=WrongIQMode;

            /* Setup scan and band lists */
            STTUNER_SetBandList(TUNERHandle, &Bands);
            STTUNER_SetScanList(TUNERHandle, &Scans);

            error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);

            TUNER_DebugError("STTUNER_Scan()", error);
            if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result, "Unable to commence tuner scan");
                goto simple_end;
            }

            TUNER_DebugMessage("Scanning...");

            /* Wait for 5s then termionate search */
            STTUNER_TaskDelay( ST_GetClocksPerSecond() * 5);

            TUNER_DebugPrintf(("Tuner unlocked...."));
            error = STTUNER_Unlock(TUNERHandle);
            TUNER_DebugPrintf(("Ok.\n"));

            /* Wait for notify function to return */
            if (Params_p->Tuner == 0)
            {
                AwaitNotifyFunction1(&Event);
            }
            else
            {
                AwaitNotifyFunction2(&Event);
            }

            if (Event == STTUNER_EV_SCAN_FAILED)
            {
                TUNER_DebugPrintf(("Correct event = %s.\n", TUNER_EventToString(Event)));
                break;
            }
            else
            {
                 TUNER_DebugPrintf(("Incorrect event = %s.\n", TUNER_EventToString(Event)));
            }

        }
        else if (Event == STTUNER_EV_SCAN_FAILED)
        {
            TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            break;
        }
        else
        {
            TUNER_DebugPrintf(("Bottom : Event = %s.\n", TUNER_EventToString(Event)));
        }


     }   /* for() */
    

    TUNER_TestPassed(Result);

simple_end:

        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);

    return Result;

}
#endif

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
    STTUNER_EventType_t  Event;
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
    U32 FStart, FEnd;

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
    TUNER_DebugMessage("Purpose: Time to scan across the high band");

    ChannelCount = 0;
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;

    /* Now attempt a scan */
    TUNER_DebugMessage("Commencing scan");

    Time = STOS_time_now();
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);

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

            STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

            /* Store channel information in channel table */
            ChannelTable[ChannelCount].F = TunerInfo.Frequency;
            memcpy(&ChannelTable[ChannelCount].ScanInfo, &TunerInfo.ScanInfo, sizeof(STTUNER_Scan_t) );

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
    STTUNER_EventType_t Event;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd;


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

 
    TUNER_DebugPrintf(("%d.1: Check Band and Scan lists have to be set before a scan.\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Ensure no scan if band and scan lists not set.");

    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;

    /* attempt a scan */
    TUNER_DebugMessage("Commencing scan...");
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error == ST_ERROR_BAD_PARAMETER)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_ERROR_BAD_PARAMETER'");
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
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan()", error);

    if (error == ST_NO_ERROR)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
    }

 /* Wait for notify function to return */
    if (/*Params_p->Tuner == 0*/1)
    {   
        AwaitNotifyFunction1(&Event);
    }
    else
    {      
        AwaitNotifyFunction2(&Event);
    }
 
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
    
    TUNER_DebugMessage("Forcing termination...");
    TermParams.ForceTerminate = TRUE;

    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);
   
    STTUNER_TaskDelay( ST_GetClocksPerSecond() * 1);

    return(Result);

} /* TunerTerm() */

#ifdef TEST_DUAL
/*----------------------------------------------------------------------------
TunerDualScanSimultaneous()

Description:
    Setup both tuners ie initialize, open and scan at the same time

Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.

Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.

See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t TunerDualScanSimultaneous(TUNER_TestParams_t *Params_p )
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t       error;
    STTUNER_Handle_t     TUNERHandle1, TUNERHandle2;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t  Event1,Event2;
    STTUNER_Scan_t       ScanParams1,ScanParams2;
    STTUNER_TunerInfo_t  TunerInfo1,TunerInfo;
    STTUNER_TermParams_t TermParams1, TermParams2;
    U32 FStart, FEnd, i;

    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
   /* FStart = 3200000;
    FEnd   = 5150000;*/

    TermParams1.ForceTerminate = FALSE;
    TermParams2.ForceTerminate = FALSE;

#ifdef TUN1
    error = STTUNER_Init(TunerDeviceName[0], &TunerInitParams[0]);
    TUNER_DebugError("STTUNER_Init(0)", error);
 #endif

 #ifdef TUN2
    error = STTUNER_Init(TunerDeviceName[1], &TunerInitParams[1]);
    TUNER_DebugError("STTUNER_Init(1)", error);
 #endif


   OpenParams.NotifyFunction = CallbackNotifyFunction1;

#ifdef TUN1
    error = STTUNER_Open(TunerDeviceName[0], &OpenParams, &TUNERHandle1);
    TUNER_DebugError("STTUNER_Open(0)", error);
 #endif


    OpenParams.NotifyFunction = CallbackNotifyFunction2;
 #ifdef TUN2
    error = STTUNER_Open(TunerDeviceName[1], &OpenParams, &TUNERHandle2);
    TUNER_DebugError("STTUNER_Open(1)", error);
 #endif


 #ifdef TUN1
    error = STTUNER_SetBandList(TUNERHandle1, &Bands);
    TUNER_DebugError("STTUNER_SetBandList(0)", error);
 #endif

 #ifdef TUN2
    error = STTUNER_SetBandList(TUNERHandle2, &Bands);
    TUNER_DebugError("STTUNER_SetBandList(1)", error);
 #endif



 #ifdef TUN1
    error = STTUNER_SetScanList(TUNERHandle1, &Scans);
    TUNER_DebugError("STTUNER_SetScanList(0)", error);
 #endif

 #ifdef TUN2
 #ifdef SAT_TEST_SCR
    error = STTUNER_SetScanList(TUNERHandle2, &Scans1);
 #else
    error = STTUNER_SetScanList(TUNERHandle2, &Scans);
 #endif
    TUNER_DebugError("STTUNER_SetScanList(1)", error);
 #endif


#ifdef TUN1
    error = STTUNER_Scan(TUNERHandle1, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan(0)", error);
#endif




#ifdef TUN1
      AwaitNotifyFunction1(&Event1);
      TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));

        if (Event1 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle1, &TunerInfo1);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               1, TunerInfo1.Frequency,
                               TUNER_PolarityToString(TunerInfo1.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo1.SignalQuality, TunerInfo1.BitErrorRate,
                               TUNER_FecToString(TunerInfo1.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo1.ScanInfo.AGC),
                               TunerInfo1.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
/*            TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));
            TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");
*/
        }
    ScanParams1.Polarization = TunerInfo1.ScanInfo.Polarization;
    ScanParams1.FECRates     = TunerInfo1.ScanInfo.FECRates;
    ScanParams1.Modulation   = TunerInfo1.ScanInfo.Modulation;
    ScanParams1.LNBToneState = TunerInfo1.ScanInfo.LNBToneState;
    ScanParams1.Band         = TunerInfo1.ScanInfo.Band;
    ScanParams1.AGC          = TunerInfo1.ScanInfo.AGC;
    ScanParams1.SymbolRate   = TunerInfo1.ScanInfo.SymbolRate;
    ScanParams1.IQMode       = TunerInfo1.ScanInfo.IQMode;
    ScanParams1.ModeCode     = TunerInfo1.ScanInfo.ModeCode;
    
        TUNER_DebugMessage("Tuning...");
    
    error = STTUNER_SetFrequency(TUNERHandle1, TunerInfo1.Frequency, &ScanParams1, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
    }

    /* Wait for notify function to return */
    if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event1);
    }
    else
    {
        AwaitNotifyFunction2(&Event1);
    }
    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event1)));

    /* Check whether or not we're locked */
    if (Event1 != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
    }

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle1, &TunerInfo1);
    
    TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                       TunerInfo1.Frequency,
                       TUNER_PolarityToString(TunerInfo1.ScanInfo.Polarization),
                       TunerInfo1.SignalQuality,
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo1.BitErrorRate,
                       TUNER_FecToString(TunerInfo1.ScanInfo.FECRates),
                       TUNER_AGCToPowerOutput(TunerInfo1.ScanInfo.AGC),
                       TunerInfo1.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                      ));
                      
#endif

#ifdef TUN2
    error = STTUNER_Scan(TUNERHandle2, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan(1)", error);
#endif
#ifdef TUN2
      AwaitNotifyFunction2(&Event2);
      TUNER_DebugPrintf(("(2)Event = %s.\n", TUNER_EventToString(Event2)));

        if (Event2 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               2, TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.SignalQuality, TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/

        }

#endif

	ScanParams2.Polarization = TunerInfo.ScanInfo.Polarization;
    ScanParams2.FECRates     = TunerInfo.ScanInfo.FECRates;
    ScanParams2.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams2.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
    ScanParams2.Band         = TunerInfo.ScanInfo.Band;
    ScanParams2.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams2.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams2.IQMode       = TunerInfo.ScanInfo.IQMode;
    ScanParams2.ModeCode     = TunerInfo.ScanInfo.ModeCode;

    
    TUNER_DebugMessage("Tuning...");
    
    error = STTUNER_SetFrequency(TUNERHandle2, TunerInfo.Frequency, &ScanParams2, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
    }

    /* Wait for notify function to return */
        AwaitNotifyFunction2(&Event2);

    TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event2)));

    /* Check whether or not we're locked */
    if (Event2 != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
    }

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
    
    TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                       TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                      ));
/*****************************************************************************/

/* Check for 30 times */
for(i = 0; i <30; i++)
{
STTUNER_TaskDelay(ST_GetClocksPerSecond()); /*1 second delay between readings*/
TUNER_DebugPrintf(("\n\n"));

 #ifdef TUN2
        TUNER_DebugPrintf(("(2)TunerStatus  = %s\n", TUNER_StatusToString(TunerInfo.Status)));
        if (Event2 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               2, TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.SignalQuality, TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
	    TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/
        }

#endif
#ifdef TUN1

        TUNER_DebugPrintf(("(1)TunerStatus  = %s\n", TUNER_StatusToString(TunerInfo1.Status)));
        if (Event1 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle1, &TunerInfo1);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s  BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               1, TunerInfo1.Frequency,
                               TUNER_PolarityToString(TunerInfo1.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo1.SignalQuality, TunerInfo1.BitErrorRate,
                               TUNER_FecToString(TunerInfo1.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo1.ScanInfo.AGC),
                               TunerInfo1.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
          TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/
        }

#endif

} /* End of for loop*/


#ifdef TUN1
    error = STTUNER_Unlock(TUNERHandle1);
    TUNER_DebugError("STTUNER_Unlock(1)", error);

      AwaitNotifyFunction1(&Event1);
      TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));

        if (Event1 == STTUNER_EV_LOCKED)
        {
            TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_UNLOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/
        }
        else
        {
            TUNER_DebugPrintf(("Result: !!!! PASS !!!! Expected = 'STTUNER_EV_UNLOCKED'\n"));
/*            TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));
            TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");
*/
        }
#endif

#ifdef TUN2
    error = STTUNER_Unlock(TUNERHandle2);
    TUNER_DebugError("STTUNER_Unlock(2)", error);

        AwaitNotifyFunction2(&Event2);
        TUNER_DebugPrintf(("(2)Event = %s.\n", TUNER_EventToString(Event2)));
        if (Event2 == STTUNER_EV_LOCKED)
        {
            TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_UNLOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/
        }
        else
        {
	    TUNER_DebugPrintf(("Result: !!!! PASS !!!! Expected = 'STTUNER_EV_UNLOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/
        }

#endif

#ifdef TUN1
    error = STTUNER_Close(TUNERHandle1);
    TUNER_DebugError("STTUNER_Close(1)", error);
#endif


#ifdef TUN2
    error = STTUNER_Close(TUNERHandle2);
    TUNER_DebugError("STTUNER_Close(2)", error);
#endif

#ifdef TUN1
     error = STTUNER_Term(TunerDeviceName[0], &TermParams1);
     TUNER_DebugError("STTUNER_Term(1)", error);
#endif

#ifdef TUN2
     error = STTUNER_Term(TunerDeviceName[1], &TermParams2);
     TUNER_DebugError("STTUNER_Term(2)", error);
#endif

if (error == ST_NO_ERROR)
{
    TUNER_TestPassed(Result);
}
else
{
    TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
}

return (Result);
}/* End of TunerDualScanSimultaneous function */



/*----------------------------------------------------------------------------
TunerDualScanStabilityTest()

Description:
    Tests the robustness of the software

Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.

Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.

See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t TunerDualScanStabilityTest(TUNER_TestParams_t *Params_p )
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t       error;
    STTUNER_Handle_t     TUNERHandle1, TUNERHandle2;
    STTUNER_OpenParams_t OpenParams;
    
    STTUNER_EventType_t Event1,Event2;
    STTUNER_TunerInfo_t  TunerInfo;
    STTUNER_TermParams_t TermParams1, TermParams2;

    U32 FStart, FEnd ;
    
    Init_scanparams();
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
   /* FStart = 3200000;
    FEnd   = 11700000;*/

    TermParams1.ForceTerminate = FALSE;
    TermParams2.ForceTerminate = FALSE;

/* Initialize Tuner1 */
 #ifdef TUN1
    error = STTUNER_Init(TunerDeviceName[0], &TunerInitParams[0]);
    TUNER_DebugError("STTUNER_Init(0)", error);
 #endif


/* Open Tuner1 */
 #ifdef TUN1
    OpenParams.NotifyFunction = CallbackNotifyFunction1;
    error = STTUNER_Open(TunerDeviceName[0], &OpenParams, &TUNERHandle1);
    TUNER_DebugError("STTUNER_Open(0)", error);
 #endif

 /* Initialize Tuner2 */
 #ifdef TUN2
    error = STTUNER_Init(TunerDeviceName[1], &TunerInitParams[1]);
    TUNER_DebugError("STTUNER_Init(1)", error);
 #endif



/* Open Tuner2 */
 #ifdef TUN2
    OpenParams.NotifyFunction = CallbackNotifyFunction2;
    error = STTUNER_Open(TunerDeviceName[1], &OpenParams, &TUNERHandle2);
    TUNER_DebugError("STTUNER_Open(1)", error);
 #endif



/* Setting Bandlist for Tuner2 */
 #ifdef TUN2
    error = STTUNER_SetBandList(TUNERHandle2, &Bands);
    TUNER_DebugError("STTUNER_SetBandList(1)", error);
 #endif


/* Setting Scanlist for Tuner2 */
 #ifdef TUN2
 #ifdef SAT_TEST_SCR
    error = STTUNER_SetScanList(TUNERHandle2, &Scans1);
    #else
    error = STTUNER_SetScanList(TUNERHandle2, &Scans);
 #endif
    TUNER_DebugError("STTUNER_SetScanList(1)", error);
 #endif


/* Start Scanning for Tuner2 */
#ifdef TUN2
    error = STTUNER_Scan(TUNERHandle2, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan(1)", error);
#endif



/* Checking locked frequency for Tuner2 */
#ifdef TUN2
     AwaitNotifyFunction2(&Event2);
      TUNER_DebugPrintf(("(2)Event = %s.\n", TUNER_EventToString(Event2)));

        if (Event2 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               2, TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.SignalQuality, TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");

        }

#endif

/* Tuner1 Close */
#ifdef TUN1
    error = STTUNER_Close(TUNERHandle1);
    TUNER_DebugError("STTUNER_Close(0)", error);
#endif

/* Tuner1 Term */
#ifdef TUN1
     error = STTUNER_Term(TunerDeviceName[0], &TermParams1);
     TUNER_DebugError("STTUNER_Term(0)", error);
#endif

/**********Once again initializing 1st Tuner****************/
/* Initialize Tuner1 */
 #ifdef TUN1
    error = STTUNER_Init(TunerDeviceName[0], &TunerInitParams[0]);
    TUNER_DebugError("STTUNER_Init(0)", error);
 #endif
 
 /* Open Tuner1 */
 #ifdef TUN1
    OpenParams.NotifyFunction = CallbackNotifyFunction1;
    error = STTUNER_Open(TunerDeviceName[0], &OpenParams, &TUNERHandle1);
    TUNER_DebugError("STTUNER_Open(0)", error);
 #endif
 
 /* Start Scanning for Tuner2 */
#ifdef TUN2
    /*error = STTUNER_Scan(TUNERHandle2, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan(1)", error);*/
#endif

STTUNER_TaskDelay(100);


/* Checking locked frequency for Tuner2 */
#ifdef TUN2
     
    
      TUNER_DebugPrintf(("(2)Event = %s.\n", TUNER_EventToString(Event2)));

        if (Event2 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               2, TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.SignalQuality, TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");

        }
       

#endif


#ifdef TUN2
 /*  error=STTUNER_Unlock(TUNERHandle2);
   TUNER_DebugError("STTUNER_Unlock(1)", error);*/
#endif

/* Tuner2 Close */
#ifdef TUN2
    error = STTUNER_Close(TUNERHandle2);
    TUNER_DebugError("STTUNER_Close(1)", error);
#endif


/* Tuner2 Term */
#ifdef TUN2
     error = STTUNER_Term(TunerDeviceName[1], &TermParams2);
     TUNER_DebugError("STTUNER_Term(1)", error);
#endif

/**********Initializing 2nd Tuner*******************/
#ifdef TUN2
    error = STTUNER_Init(TunerDeviceName[1], &TunerInitParams[1]);
    TUNER_DebugError("STTUNER_Init(1)", error);
 #endif

/* Open Tuner2 */
 #ifdef TUN2
    OpenParams.NotifyFunction = CallbackNotifyFunction2;
    error = STTUNER_Open(TunerDeviceName[1], &OpenParams, &TUNERHandle2);
    TUNER_DebugError("STTUNER_Open(1)", error);
 #endif
 
 /*********Scanning for 1st Tuner*************/
 /* Setting Bandlist for Tuner1 */
 #ifdef TUN1
    error = STTUNER_SetBandList(TUNERHandle1, &Bands);
    TUNER_DebugError("STTUNER_SetBandList(0)", error);
 #endif


/* Setting Scanlist for Tuner1 */
 #ifdef TUN1
    error = STTUNER_SetScanList(TUNERHandle1, &Scans);
    TUNER_DebugError("STTUNER_SetScanList(0)", error);
 #endif


/* Start Scanning for Tuner1 */
#ifdef TUN1
    error = STTUNER_Scan(TUNERHandle1, FStart, FEnd, 0, 0);
    TUNER_DebugError("STTUNER_Scan(0)", error);
#endif



/* Checking locked frequency for Tuner1 */
#ifdef TUN1
      AwaitNotifyFunction1(&Event1);
      TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));

        if (Event1 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle1, &TunerInfo);
            
            TUNER_DebugPrintf(("%2d F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               1, TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.SignalQuality, TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");

        }

#endif

#ifdef TUN1
  /* error=STTUNER_Unlock(TUNERHandle1);
   TUNER_DebugError("STTUNER_Unlock(0)", error);*/
#endif
/* Tuner1 Close */
#ifdef TUN1
    error = STTUNER_Close(TUNERHandle1);
    TUNER_DebugError("STTUNER_Close(0)", error);
#endif

/* Tuner1 Term */
#ifdef TUN1
     error = STTUNER_Term(TunerDeviceName[0], &TermParams1);
     TUNER_DebugError("STTUNER_Term(0)", error);
#endif

/* Tuner2 Close */
#ifdef TUN2
    error = STTUNER_Close(TUNERHandle2);
    TUNER_DebugError("STTUNER_Close(1)", error);
#endif


/* Tuner2 Term */
#ifdef TUN2
     error = STTUNER_Term(TunerDeviceName[1], &TermParams2);
     TUNER_DebugError("STTUNER_Term(1)", error);
#endif

if (error == ST_NO_ERROR)
{
    TUNER_TestPassed(Result);
}
else
{
    TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
}
return (Result);

}/* End of RigorousTunerDualScanStabilityTest */
#endif

#ifdef STTUNER_DISEQC2_SW_DECODE_VIA_PIO

/*****************************************************************************
TunerDiSEqC2()
Description:
    
Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/
static TUNER_TestResult_t TunerDiSEqC2(TUNER_TestParams_t *Params_p)
{
	TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParamsTuner;
    STTUNER_TermParams_t TermParams; 
    STPIO_Handle_t          HandlePIO;
    STPIO_OpenParams_t      OpenParamsPIO;
    U8 i,Testno;
    U8 DummyArray[MAX_NUM_RECEIVE_BYTES];
    U8 command_array [1] ={FB_COMMAND_REPLY};
    STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t  pDiSEqCReceivePacket;
    static ST_DeviceName_t      DeviceName[] =
{
    "STPIO[0]",
    "STPIO[1]",
    "STPIO[2]",
    "STPIO[3]",
    "STPIO[4]",
    "STPIO[5]",     /* 14, 16, 17, 28 only */
    "STPIO[6]",     /* 28 only */
    "STPIO[7]"      /* 28 only */
};
    
    pDiSEqCReceivePacket.uc_ExpectedNoOfBytes = 0;
    pDiSEqCReceivePacket.ReceivedFrmAddData = &DummyArray[0]; /* Allocate memory for the received array*/
    memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
    pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
    
    OpenParamsPIO.ReservedBits    = TunerInitParams[Params_p->Tuner].DiSEqC.PIOPin ; 
    OpenParamsPIO.BitConfigure[5] = STPIO_BIT_INPUT;
    OpenParamsPIO.IntHandler      = CaptureReplyPulse1;

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {        
        goto simple_end;
    }

 
    /* Open params setup */
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParamsTuner, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
         goto simple_end;
    }
    error = STPIO_Open(DeviceName[TunerInitParams[0].DiSEqC.PIOPort], &OpenParamsPIO, &HandlePIO);
    TUNER_DebugError("STPIO_Open()", error);
    
    if (error != ST_NO_ERROR)
    {
        goto simple_end;
    }
    
    /* Associate PIOHandle with Instance Database */
    error = STTUNER_PIOHandleInInstDbase(TUNERHandle,HandlePIO);
    TUNER_DebugError("STTUNER_PIOHandleInInstDbase()", error);
    
    TUNER_DebugPrintf(("proceed to see DiSEqC2 reply receive \n"));
    for (Testno =0 ;Testno <50;Testno++)

    {
	DiSEqCSendPacket.DiSEqCCommandType=  STTUNER_DiSEqC_COMMAND;
	DiSEqCSendPacket.uc_TotalNoOfBytes =1;
	DiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
	DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
		

	TUNER_DebugPrintf((" \n--- DiSEqC Test no #%d ---\n",Testno ));
	/* Send DiSEqC command that waits for reply */
	error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,&pDiSEqCReceivePacket);	
	        
	if( error != ST_NO_ERROR)
	{
   	     TUNER_DebugPrintf(("Error in STTUNER_DiSEqCSendReceive() function\n"));
	}
	
	/* Check the reply byte */
	TUNER_DebugPrintf(("Number of reply bytes received: %d\n",pDiSEqCReceivePacket.uc_TotalBytesReceived));
        for(i=0;i<pDiSEqCReceivePacket.uc_TotalBytesReceived;i++)
        {
       	TUNER_DebugPrintf(("0x%x\t",pDiSEqCReceivePacket.ReceivedFrmAddData[i]));
	}
	TUNER_DebugPrintf(("\n"));
	
	memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
	pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
	DiSEqC_Delay_ms(0);/* Wait for 1 sec for next reply to come */    
    }


simple_end:
	error = STPIO_Close(HandlePIO);
	TUNER_DebugError("STPIO_Close()", error);
        
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} 


#ifdef TEST_DUAL
/*****************************************************************************
TunerDiSEqC2Dual()
Description:
    
Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/
static TUNER_TestResult_t TunerDiSEqC2Dual(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t     TUNERHandle1, TUNERHandle2;
    STTUNER_OpenParams_t OpenParamsTuner;
    STTUNER_TermParams_t TermParams1,TermParams2; 
    STPIO_Handle_t          HandlePIO1,HandlePIO2;
    STPIO_OpenParams_t      OpenParamsPIO1,OpenParamsPIO2;
    U8 i,Testno;
    U8 DummyArray[MAX_NUM_RECEIVE_BYTES];
    U8 command_array [1] ={FB_COMMAND_REPLY};
    STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t  pDiSEqCReceivePacket;
    static ST_DeviceName_t      DeviceName[] =
{
    "STPIO[0]",
    "STPIO[1]",
    "STPIO[2]",
    "STPIO[3]",
    "STPIO[4]",
    "STPIO[5]",     /* 14, 16, 17, 28 only */
    "STPIO[6]",     /* 28 only */
    "STPIO[7]"      /* 28 only */
};
    
    pDiSEqCReceivePacket.uc_ExpectedNoOfBytes = 0;
    pDiSEqCReceivePacket.ReceivedFrmAddData = &DummyArray[0]; /* Allocate memory for the received array*/
    memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
    pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
    
    OpenParamsPIO1.ReservedBits    =  TunerInitParams[0].DiSEqC.PIOPin; 
    OpenParamsPIO1.BitConfigure[5] = STPIO_BIT_INPUT;
    OpenParamsPIO1.IntHandler      = CaptureReplyPulse1;
    
    OpenParamsPIO2.ReservedBits    = TunerInitParams[1].DiSEqC.PIOPin; 
    OpenParamsPIO2.BitConfigure[6] = STPIO_BIT_INPUT;
    OpenParamsPIO2.IntHandler      = CaptureReplyPulse2;
    
#ifdef TUN1
    error = STTUNER_Init(TunerDeviceName[0], &TunerInitParams[0]);
    TUNER_DebugError("STTUNER_Init(1)", error);
 #endif


#ifdef TUN2
    error = STTUNER_Init(TunerDeviceName[1], &TunerInitParams[1]);
    TUNER_DebugError("STTUNER_Init(2)", error);
 #endif
 
 
#ifdef TUN1
    error = STTUNER_Open(TunerDeviceName[0], &OpenParamsTuner, &TUNERHandle1);
    TUNER_DebugError("STTUNER_Open(1)", error);
 #endif
 
 #ifdef TUN2
    error = STTUNER_Open(TunerDeviceName[1], &OpenParamsTuner, &TUNERHandle2);
    TUNER_DebugError("STTUNER_Open(2)", error);
 #endif
   
 #ifdef TUN1
    error = STPIO_Open(DeviceName[TunerInitParams[0].DiSEqC.PIOPort], &OpenParamsPIO1, &HandlePIO1);
    TUNER_DebugError("STPIO_Open(1)", error); 
#endif

#ifdef TUN2
    error = STPIO_Open(DeviceName[TunerInitParams[1].DiSEqC.PIOPort], &OpenParamsPIO2, &HandlePIO2);
    TUNER_DebugError("STPIO_Open(2)", error); 
#endif


#ifdef TUN1   
    /* Associate PIOHandle with Instance Database */
    error = STTUNER_PIOHandleInInstDbase(TUNERHandle1,HandlePIO1);
    TUNER_DebugError("STTUNER_PIOHandleInInstDbase(1)", error);
#endif

#ifdef TUN2   
    /* Associate PIOHandle with Instance Database */
    error = STTUNER_PIOHandleInInstDbase(TUNERHandle2,HandlePIO2);
    TUNER_DebugError("STTUNER_PIOHandleInInstDbase(2)", error);
#endif
    
    TUNER_DebugPrintf(("proceed to see DiSEqC2 reply receive \n"));
    for (Testno =0 ;Testno <50;Testno++)

    {
	DiSEqCSendPacket.DiSEqCCommandType=  STTUNER_DiSEqC_COMMAND;
	DiSEqCSendPacket.uc_TotalNoOfBytes =1;
	DiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
	DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
		

	TUNER_DebugPrintf((" \n--- DiSEqC Test no #%d ---\n",Testno ));
#ifdef TUN1
	/* Send DiSEqC command that waits for reply */
	error= STTUNER_DiSEqCSendReceive ( TUNERHandle1, &DiSEqCSendPacket,&pDiSEqCReceivePacket);	
	        
	if( error != ST_NO_ERROR)
	{
   	     TUNER_DebugPrintf(("Error in STTUNER_DiSEqCSendReceive(1) function\n"));
	}
#endif
	
	/* Check the reply byte */
	TUNER_DebugPrintf(("Tuner(1)....Number of reply bytes received: %d\n",pDiSEqCReceivePacket.uc_TotalBytesReceived));
       for(i=0;i<pDiSEqCReceivePacket.uc_TotalBytesReceived;i++)
       {
       	TUNER_DebugPrintf(("0x%x\t",pDiSEqCReceivePacket.ReceivedFrmAddData[i]));
	}
	TUNER_DebugPrintf(("\n"));
	
	memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
	pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
	DiSEqC_Delay_ms(5);/* Wait for 1 sec for next reply to come */  
	
#ifdef TUN2
	/* Send DiSEqC command that waits for reply */
	error= STTUNER_DiSEqCSendReceive ( TUNERHandle2, &DiSEqCSendPacket,&pDiSEqCReceivePacket);	
	        
	if( error != ST_NO_ERROR)
	{
   	     TUNER_DebugPrintf(("Error in STTUNER_DiSEqCSendReceive(2) function\n"));
	}
#endif
	
	/* Check the reply byte */
	TUNER_DebugPrintf(("Tuner(2)....Number of reply bytes received: %d\n",pDiSEqCReceivePacket.uc_TotalBytesReceived));
       for(i=0;i<pDiSEqCReceivePacket.uc_TotalBytesReceived;i++)
       {
       	TUNER_DebugPrintf(("0x%x\t",pDiSEqCReceivePacket.ReceivedFrmAddData[i]));
	}
	TUNER_DebugPrintf(("\n"));
	
	memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
	pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
	DiSEqC_Delay_ms(5);/* Wait for 1 sec for next reply to come */  
    }


#ifdef TUN1
    error = STPIO_Close(HandlePIO1);
    TUNER_DebugError("STPIO_Close(1)", error);
#endif

#ifdef TUN2
    error = STPIO_Close(HandlePIO2);
    TUNER_DebugError("STPIO_Close(2)", error);
#endif

#ifdef TUN1
    error = STTUNER_Close(TUNERHandle1);
    TUNER_DebugError("STTUNER_Close(1)", error);
#endif


#ifdef TUN2
    error = STTUNER_Close(TUNERHandle2);
    TUNER_DebugError("STTUNER_Close(2)", error);
#endif

#ifdef TUN1
     error = STTUNER_Term(TunerDeviceName[0], &TermParams1);
     TUNER_DebugError("STTUNER_Term(1)", error);
#endif

#ifdef TUN2
     error = STTUNER_Term(TunerDeviceName[1], &TermParams2);
     TUNER_DebugError("STTUNER_Term(2)", error);
#endif

return(Result);
}

#endif
#endif 
#if defined(TUNER_288) && !defined(STTUNER_DISEQC2_SW_DECODE_VIA_PIO)
#ifndef SAT_TEST_SCR
/*****************************************************************************
TunerDiSEqC2()
Description:
    
Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/
static TUNER_TestResult_t TunerDiSEqC2(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParamsTuner;
    STTUNER_TermParams_t TermParams; 
    
    U8 i,Testno;
    U8 DummyArray[MAX_NUM_RECEIVE_BYTES];
    U8 command_array [] ={0xE2, 0x00, 0x00};
    STTUNER_DiSEqCSendPacket_t DiSEqCSendPacket;
    STTUNER_DiSEqCResponsePacket_t  pDiSEqCReceivePacket;
   
    
    pDiSEqCReceivePacket.uc_ExpectedNoOfBytes = 0;
    pDiSEqCReceivePacket.ReceivedFrmAddData = &DummyArray[0]; /* Allocate memory for the received array*/
    memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
    pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
    
   
    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {        
        goto simple_end;
    }

  
    /* Open params setup */
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParamsTuner, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
         goto simple_end;
    }
       
    TUNER_DebugPrintf(("proceed to see DiSEqC2 reply receive \n"));
    for (Testno =0 ;Testno <50;Testno++)

    {
	DiSEqCSendPacket.DiSEqCCommandType=  STTUNER_DiSEqC_COMMAND;
	DiSEqCSendPacket.uc_TotalNoOfBytes =3;
	DiSEqCSendPacket.pFrmAddCmdData = &command_array[0];
	DiSEqCSendPacket.uc_msecBeforeNextCommand =0;
		

	TUNER_DebugPrintf((" \n--- DiSEqC Test no #%d ---\n",Testno ));
	TUNER_DebugPrintf((" \n \t \t--- Sending Bytes  ---\n"));
	for(i=0;i<DiSEqCSendPacket.uc_TotalNoOfBytes;i++)
        {
       	TUNER_DebugPrintf(("0x%x\t\n",DiSEqCSendPacket.pFrmAddCmdData[i]));
	}
	
	/* Send DiSEqC command that waits for reply */
	error= STTUNER_DiSEqCSendReceive ( TUNERHandle, &DiSEqCSendPacket,&pDiSEqCReceivePacket);	
	        
	if( error != ST_NO_ERROR)
	{
   	     TUNER_DebugPrintf(("Error in STTUNER_DiSEqCSendReceive() function\n"));
	}
	
	/* Check the reply byte */
	TUNER_DebugPrintf(("Number of reply bytes received: %d\n",pDiSEqCReceivePacket.uc_TotalBytesReceived));
        for(i=0;i<pDiSEqCReceivePacket.uc_TotalBytesReceived;i++)
        {
       	TUNER_DebugPrintf(("0x%x\t\n",pDiSEqCReceivePacket.ReceivedFrmAddData[i]));
	}
	TUNER_DebugPrintf(("\n"));
	
	memset(pDiSEqCReceivePacket.ReceivedFrmAddData,0x00,MAX_NUM_RECEIVE_BYTES);
	pDiSEqCReceivePacket.uc_TotalBytesReceived = 0;
	DiSEqC_Delay_ms(0);/* Wait for 1 sec for next reply to come */    
    }


simple_end:
	
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} 

#endif
#endif
#ifdef TEST_DISHPOSITIONING
/*****************************************************************************
TunerDishPositioning()
Description:
    This test is for test dish positioning algorithm.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerDishPositioning(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    STTUNER_Handle_t TUNERHandle;
    ST_ErrorCode_t error;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_Scan_t       ScanParams;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
       
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

    /* Setup scan and band lists */
    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

   
    /* Lock to a frequency ----------------- */

    TunerInfo.Frequency               = 3840000;
    ScanParams.Polarization = STTUNER_PLR_HORIZONTAL;
    ScanParams.FECRates     = STTUNER_FEC_7_8;;
    ScanParams.Modulation   = STTUNER_MOD_QPSK;;
    ScanParams.LNBToneState = STTUNER_LNB_TONE_22KHZ;
    ScanParams.Band         = 1;
    ScanParams.AGC          = STTUNER_AGC_ENABLE;
    ScanParams.SymbolRate   = 26850000;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;


    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
    
    /* Make DishPositiong Flag TRUE */
    error = STTUNER_DishPositioning(TUNERHandle, TunerInfo.Frequency, &ScanParams, TRUE , 0);
    TUNER_DebugError("STTUNER_DishPositioning()", error);
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

	/* Make DishPositiong Flag FALSE */
    error = STTUNER_DishPositioning(TUNERHandle, TunerInfo.Frequency, &ScanParams, FALSE , 0);
    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    
    TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                       TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                      ));
                      
     
    
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

} /* TunerDishPositioning() */
#endif

#endif

#ifdef STTUNER_MINIDRIVER
static TUNER_TestResult_t TunerSimple(TUNER_TestParams_t *Params_p)
{
 TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t      error;
    STTUNER_Handle_t    TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_Scan_t       ScanParams;
    U32 i;
    

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
      /* Lock to a frequency ----------------- */

    ScanParams.Frequency  =  3700000;
    ScanParams.Polarization = STTUNER_PLR_VERTICAL;
    ScanParams.FECRates     = STTUNER_FEC_3_4;
    ScanParams.Modulation   = STTUNER_MOD_QPSK;
    ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanParams.Band         = 0;
    ScanParams.AGC          = 1;
    ScanParams.SymbolRate   = 27500000;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
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
    
#ifndef ST_OSLINUX
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
#endif /*ST_OSLINUX*/

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% BER = %5d FEC = %s\n",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates)
                      
                      ));
                      
    
    ScanParams.Frequency    = 3760000;
    ScanParams.Polarization = STTUNER_PLR_HORIZONTAL;
    ScanParams.FECRates     = STTUNER_FEC_7_8;
    ScanParams.Modulation   = STTUNER_MOD_QPSK;
    ScanParams.LNBToneState = STTUNER_LNB_TONE_OFF;
    ScanParams.Band         = 0;
    ScanParams.AGC          = 1;
    ScanParams.SymbolRate   = 26000000;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
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
    /*Event be ignored : huangfei 2005-08-11*/
#ifndef ST_OSLINUX
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
#endif /*ST_OSLINUX*/
    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
      TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% BER = %5d FEC = %s\n ",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates)
                      
                      ));
    for (i = 0; i < TUNER_TRACKING; i++)
    {
    	STTUNER_TaskDelay(ST_GetClocksPerSecond());
        error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
        TUNER_DebugPrintf(("\n"));
        TUNER_DebugPrintf(("TunerStatus: %s\n", TUNER_StatusToString(TunerInfo.Status)));
        TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% BER = %5d FEC = %s\n ",
                       TunerInfo.Frequency,
                       TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_FecToString(TunerInfo.ScanInfo.FECRates)
                      ));        
    }
    relocking_end:

    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
    
}
#endif
/* TunerSimple() */

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


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
        "4_5",
        "5_6",
        "6_7",
        "7_8",
        "8_9",
        "1_4",
        "1_3",
        "2_5",
        "3_5",
        "9_10"
        
    };

    for (i = 0; Fec > 0; i++, Fec>>=1);
    return FecRate[i-1];
}


/* ------------------------------------------------------------------------ */

char *TUNER_StatusToString(STTUNER_Status_t Status)
{
	switch(Status)
	{
		case STTUNER_STATUS_UNLOCKED:
		    return("UNLOCKED");
		
		case STTUNER_STATUS_SCANNING:
            return("SCANNING");
        
        case STTUNER_STATUS_LOCKED:
            return("LOCKED");
        
        case STTUNER_STATUS_NOT_FOUND:
            return("NOT_FOUND");
            
         case STTUNER_STATUS_STANDBY:
            return("STANDBY");    
            
             case STTUNER_STATUS_IDLE:
            return("IDLE");    
        
        default:
            break;
    }
    return("!!UNKNOWN!!");
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
        "BPSK\0" ,     /* 8 */
        "16APSK\0",
        "32APSK\0",
        "NONE\0",
        "ALL\0",
        "!!INVALID MODULATION!!\0"
    };

    if (Mod == STTUNER_MOD_NONE) return Modulation[12];
    if (Mod == STTUNER_MOD_ALL)  return Modulation[13];
    for (i = 0; Mod > 0; i++, Mod>>=1);
    return Modulation[i-1];
}

/* ------------------------------------------------------------------------ */

char *TUNER_PolarityToString(STTUNER_Polarization_t Plr)
{
    static char *Polarity[]= { "HORIZONTAL", "VERTICAL",
                               "ALL(H+V)",   "OFF", "!!INVALID POLARITY!!" };


    if ((Plr > STTUNER_PLR_ALL) || (Plr == 5) || (Plr == 6)) return Polarity[4];
    if ((Plr == STTUNER_PLR_ALL) || (Plr == 3) ) return Polarity[2];
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
            
            case STTUNER_EV_LNB_FAILURE:
            return("STTUNER_EV_LNB_FAILURE");

        default:
                break;
    }
    i++;
    if (i >= NUM_TUNER) i = 0;
    sprintf(c[i], "NOT_STTUNER_EVENT(%d)", Evt);
    return(c[i]);
}

/* ------------------------------------------------------------------------ */

char *TUNER_AGCToPowerOutput(S16 Agc)
{
    static char buf[10];

    sprintf(buf, "%5d.", Agc/10);
    return strcat(buf, (Agc % 2) ? "0" : "5" );
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

#ifndef DISABLE_TOOLBOX
#ifndef STTBX_NO_UART
        /* stuart.h */
        #ifndef ST_OSLINUX
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
            #endif
#endif

        /* sttbx.h */
        /*case STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED:
            return("STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED");*/
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
 #if defined(TUNER_399)  || defined(TUNER_288)
static TUNER_TestResult_t TunerStandByMode(TUNER_TestParams_t *Params_p)
{	
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
   
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd;

    STTUNER_Scan_t       ScanParams;
 
    
   
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
    error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *)&OpenParams, &TUNERHandle);
    TUNER_DebugError("STTUNER_Open()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto standby_end;
    }
       
    
     /************ Test the StandByMode just after open **********************/
    error=STTUNER_StandByMode(TUNERHandle,STTUNER_STANDBY_POWER_MODE);
    if(error == ST_ERROR_FEATURE_NOT_SUPPORTED)
    {
    	TUNER_DebugPrintf(("\nStandByMode Feature not supported\n"));
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
    
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Scan to the first frequency lock and let tracking run for a reasonable period.");

    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    
   

    /* Now attempt a scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, 0, 0);
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
             TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
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
    
    
    
      /* Lock to a frequency ----------------- */

    ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
    ScanParams.FECRates     = TunerInfo.ScanInfo.FECRates;
    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.IQMode       = TunerInfo.ScanInfo.IQMode;
    #ifdef STTUNER_ECHO
    ScanParams.DEMODMode    = TunerInfo.ScanInfo.DEMODMode;
    #endif
    ScanParams.FECType      = TunerInfo.ScanInfo.FECType;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
    ScanParams.LNB_SignalRouting       = TunerInfo.ScanInfo.LNB_SignalRouting;
    #endif



    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
    error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
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
        goto standby_end;
    }

    if (Event == STTUNER_EV_LOCKED)
    {
    	/* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
            goto standby_end;
        }
    
    
    
    
    /************ Trying to lock in STANDBY mode **********************/
    error=STTUNER_StandByMode(TUNERHandle,STTUNER_STANDBY_POWER_MODE);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto standby_end;
    }
    else
    {
       TUNER_DebugPrintf(("\n\t\t***** StandByMode is switched ON ***** \n"));
    }
    
   
      /************Again calling Tunning function**********************************/
    
    ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
    ScanParams.FECRates     = TunerInfo.ScanInfo.FECRates;
    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate/*27500000;*/;
    ScanParams.IQMode       = TunerInfo.ScanInfo.IQMode;
    ScanParams.ModeCode     = TunerInfo.ScanInfo.ModeCode;
    #ifdef STTUNER_ECHO
    ScanParams.DEMODMode     = TunerInfo.ScanInfo.DEMODMode;
    #endif
    ScanParams.FECType     = TunerInfo.ScanInfo.FECType;
    #ifdef SAT_TEST_SCR
    ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
    ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
    ScanParams.LNB_SignalRouting       = TunerInfo.ScanInfo.LNB_SignalRouting;
    #endif
    
        TUNER_DebugMessage("Tuning...");
    
     error = STTUNER_SetFrequency(TUNERHandle, /*TunerInfo.Frequency*/3680000/*3700790*/, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
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
   
    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    if(error != ST_NO_ERROR)
    {
       TUNER_DebugError("STTUNER_GetTunerInfo()", error);
       TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto standby_end;
    }
         
            	                      
    if (Event != STTUNER_EV_LOCKED)
    {
       TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
       TUNER_DebugPrintf((" \n Tuner not LOCKED as device is in Standby Mode \n"));   
        
    }
    else
    {
    	TUNER_DebugPrintf(("\n Standby Mode not working properly ********** \n "));
        TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_TIMEOUT'");
        goto standby_end;
    }
    

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
    
     error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    
     if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
   
 TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));

    /* Check whether or not we're locked */
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto standby_end;
    }

    if (Event == STTUNER_EV_LOCKED)
    {
    	/* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            
            TUNER_DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s AGC = %sdBm IQ = %d\n",
                               TunerInfo.Frequency,
                               TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization),
                               TunerInfo.SignalQuality,
                               TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                               TunerInfo.BitErrorRate,
                               TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                               TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC),
                               TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/
                              ));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
            goto standby_end;
        }
        

standby_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} /* end of TunerStandbyMode */
#endif

/* EOF */
