/*=============================================================================
File Name   : comb_test.c

Description : Test harness for the STTUNER driver(sat+ter).

Copyright (C) 2006-2007 STMicroelectronics

Revision History    :
 Includes --------------------------------------------------------------- */

/**************************************************************************************************/




/*Note: Macros of tuners to be tested should be defined as 1*/

#define TEST_STB0899	1
#define TEST_STX0288	1   
#define TEST_STV0372	1
#define TEST_STV0362    1
#define TEST_STV297E	0



/**************g**************************************************************************************/


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

#ifndef ST_OSLINUX
#ifndef DISABLE_TOOLBOX
#include "sttbx.h"
#ifndef STTBX_NO_UART
#include "stuart.h"
#endif
#endif
#include "stsys.h"
#endif

#include "stcommon.h"
#include "stevt.h"

#include "stpio.h"
#ifdef ST_OS21
#include "os21debug.h"
#endif
#ifdef ST_OS20
#include "debug.h"
#endif

#include "../tuner_test.h"         /* common test includes */
#include "../diseqc_app.h"

#define TUNER_DEBUG_PREFIX           "STTUNER"

#ifdef DISABLE_TOOLBOX
#define DebugPrintf(args)            printf("%s(%08d): ", TUNER_DEBUG_PREFIX, (int)STOS_time_now()); printf args
#define DebugError(msg,err)    printf("%s(%08d): %s = %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(),msg, TUNER_ErrorString(err))
#define DebugMessage(msg)            printf("%s(%08d): %s", TUNER_DEBUG_PREFIX, (int)STOS_time_now(), msg)
#else
#ifdef ST_OS21

#define DebugPrintf(args)            STTBX_Print(("%s(%08d): ", TUNER_DEBUG_PREFIX, (int)STOS_time_now())); STTBX_Print(args)
#define DebugError(msg,err)    STTBX_Print(("%s(%08d): %s = %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(),msg, TUNER_ErrorString(err)))
#define DebugMessage(msg)            STTBX_Print(("%s(%08d): %s", TUNER_DEBUG_PREFIX, (int)STOS_time_now(), msg))
#else

#define DebugPrintf(args)            STTBX_Print(("%s(%08d): ", TUNER_DEBUG_PREFIX, (int)STOS_time_now())); STTBX_Print(args)
#define DebugError(msg,err)    STTBX_Print(("%s(%08d): %s = %s\n", TUNER_DEBUG_PREFIX, (int)STOS_time_now(),msg, TUNER_ErrorString(err)))
#define DebugMessage(msg)            STTBX_Print(("%s(%08d): %s", TUNER_DEBUG_PREFIX,(int)STOS_time_now(), msg))
#endif
#endif 


char *TUNER_ErrorString(ST_ErrorCode_t Error);

/* Test success indicator */
#define TestPassed(x) x.NumberPassed++; STTBX_Print(("Result: **** PASS ****\n"))
#define TestFailed(x,reason) x.NumberFailed++; DebugPrintf(("Result: !!!! FAIL !!!! (%s)\n", reason))


#define TUNER_MAX_USED 6


/* Private types/constants ------------------------------------------------ */

/* hardware in system */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
#if defined(ST_5518)
#define NUM_PIO     6
#elif defined(ST_5105)|| defined(ST_5188) || defined(ST_5107)
#define NUM_PIO     4
#elif defined(ST_7200)
#define NUM_I2C     3
#define NUM_PIO     8
#else
#define NUM_PIO     5
#define NUM_I2C     2
#endif
#endif /* USE_STI2C_WRAPPER */

#ifdef ST_OSLINUX
#define NUM_I2C     2
#endif /* ST_OSLINUX */

#define STTUNER_DEBUG_USERSPACE_TO_KERNEL 1

/* parameters for tuner ioctl call 'STTUNER_IOCTL_PROBE' */
typedef struct TUNER_ProbeParams_s
{
   STTUNER_IODriver_t   Driver;
   ST_DeviceName_t      DriverName;
   U32                  Address;
   U32                  SubAddress;
   U32                  Value;
   U32                  XferSize;
   U32                  TimeOut;
} TUNER_ProbeParams_t;

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
   #if defined(ST_5518)
   PIO_DEVICE_5,
   #endif
   #if defined(ST_7200)
   PIO_DEVICE_5,
   PIO_DEVICE_6,
   PIO_DEVICE_7,
   #endif
   PIO_DEVICE_NOT_USED
};

enum
{
   I2C_BACK_BUS     = 0,
   I2C_FRONT_BUS    = 1
};

#define PIO_BIT_NOT_USED    0
#endif /*USE_STI2C_WRAPPER*/


/* Defines a single channel setting */
typedef struct
{
   U32              F;
   STTUNER_Scan_t   ScanInfo;
} TUNER_Channel_t;

#ifndef TUNER_TABLE_SIZE
#define TUNER_TABLE_SIZE 120
#endif


/* frequency limits for scanning */

#ifdef  C_BAND_TESTING  
#define FLIMIT_HI  3684000  /*3020000 3020000 3684000*/
#define FLIMIT_LO  3680000    /*4200000 4200000 3680000*/

#else /* Ku Band is default */
#define FLIMIT_HI   11509000
#define FLIMIT_LO   11671000
#endif

/* frequency limits for scanning */
#if defined(TEST_NOIDA_DTT_TRANSMISSION) 
#define TER_FLIMIT_LO 54000 
#define TER_FLIMIT_HI 858000 
#else
#define TER_FLIMIT_LO 54000
#define TER_FLIMIT_HI 858000
#endif


#define CAB_FLIMIT_LO   320000000 
#define CAB_FLIMIT_HI   600000000 
#define FCE_STEP     7000000 


#define TEST_FREQUENCY_372 551
#define TEST_FREQUENCY 419  
#define TEST_FREQUENCY_362 514


#define CHANNEL_BANDWIDTH_6 1
#undef  CHANNEL_BANDWIDTH_6
#undef CHANNEL_BANDWIDTH_7
#define   CHANNEL_BANDWIDTH_8




#define DEMOD_SAT_ADDRESS1              0xD2 
#define DEMOD_SAT_ADDRESS2              0xD0  
#define DEMOD_SAT_ADDRESS3              0xD4
#define DEMOD_SAT_ADDRESS4              0xD6          

#define DEMOD_TER_CAB_ADDRESS1          0x38 
#define DEMOD_TER_CAB_ADDRESS2          0x3A
#define DEMOD_TER_ADDRESS3              0x3C
#define DEMOD_TER_ADDRESS4              0x3E

#define DEMOD_TER_ADDRESS5              0x18  
#define DEMOD_TER_ADDRESS6              0x1A
#define DEMOD_TER_ADDRESS7              0x1C
#define DEMOD_TER_ADDRESS8              0x1E

#define LNBH_21_I2C_ADDRESS1            0x10
#define LNBH_21_I2C_ADDRESS2            0x16

#define TUNER_DEFAULT_INSTANCE          0

/*************************************************/


/* Change tuner scan step from default (calculated) value,
   bigger value == faster scanning, smaller value == more channels picked up */

#ifdef C_BAND_TESTING
#define FIXED_TUNER_SCAN_STEP  10000  /* <-- best for STV0399. 7000 5000 <3000> 2500 1000 0(default) */
#else /* Ku band is default */
#define FIXED_TUNER_SCAN_STEP   3000  /* <-- best for STV0399. 7000 5000 <3000> 2500 1000 0(default) */
#endif

/*defined the I2C clock frequency for particular board  */ 

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
#elif defined(ST_7100)|| defined (ST_7109) || defined (ST_7200)
#define ST_I2C_CLOCKFREQUENCY   95000000	
#elif defined(ST_5100)
#define ST_I2C_CLOCKFREQUENCY   88000000
#endif

/* Enum of I2C devices */



#ifndef ST_OSLINUX
#ifdef ST_5100

#define PIO_4_BASE_ADDRESS	ST5100_PIO4_BASE_ADDRESS
#define PIO_4_INTERRUPT		ST5100_PIO4_INTERRUPT
#endif
#endif /*ST_OSLINUX*/


#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5301) || defined(ST_8010)
 #define BACK_I2C_BASE  SSC_0_BASE_ADDRESS
  #define FRONT_I2C_BASE SSC_1_BASE_ADDRESS
#endif

#if defined (ST_7100)||defined (ST_7109)
 #define BACK_I2C_BASE  SSC_0_BASE_ADDRESS
  #define FRONT_I2C_BASE SSC_2_BASE_ADDRESS
#endif
#if defined (ST_7200)
 #define BACK_I2C_BASE  SSC_1_BASE_ADDRESS
 #define FRONT_I2C_BASE SSC_2_BASE_ADDRESS
 #define RESET_I2C_BASE SSC_4_BASE_ADDRESS
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
 

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_8010)
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



 #elif defined(ST_7710) 
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

 #elif defined(ST_7109) || defined(ST_7100) 
 #define I2C_0_CLK_DEV       PIO_DEVICE_2
 #define I2C_0_DATA_DEV      PIO_DEVICE_2
 #define I2C_0_CLK_BIT       PIO_BIT_0
 #define I2C_0_DATA_BIT      PIO_BIT_1
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_0_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_0_INTERRUPT_LEVEL


 #define I2C_1_CLK_DEV       PIO_DEVICE_4
 #define I2C_1_DATA_DEV      PIO_DEVICE_4
 #define I2C_1_CLK_BIT       PIO_BIT_0
 #define I2C_1_DATA_BIT      PIO_BIT_1
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_2_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_0_INTERRUPT_LEVEL
 
 #elif defined(ST_7200)
 #define I2C_0_CLK_DEV       PIO_DEVICE_3
 #define I2C_0_DATA_DEV      PIO_DEVICE_3
 #define I2C_0_CLK_BIT       PIO_BIT_0
 #define I2C_0_DATA_BIT      PIO_BIT_1
 #define I2C_0_BASE_ADDRESS  (U32 *)BACK_I2C_BASE
 #define I2C_0_INT_NUMBER    SSC_1_INTERRUPT
 #define I2C_0_INT_LEVEL     SSC_1_INTERRUPT_LEVEL


 #define I2C_1_CLK_DEV       PIO_DEVICE_4
 #define I2C_1_DATA_DEV      PIO_DEVICE_4
 #define I2C_1_CLK_BIT       PIO_BIT_0
 #define I2C_1_DATA_BIT      PIO_BIT_1
 #define I2C_1_BASE_ADDRESS  (U32 *)FRONT_I2C_BASE
 #define I2C_1_INT_NUMBER    SSC_2_INTERRUPT
 #define I2C_1_INT_LEVEL     SSC_0_INTERRUPT_LEVEL
 
 #define I2C_2_CLK_DEV       PIO_DEVICE_7
 #define I2C_2_DATA_DEV      PIO_DEVICE_7
 #define I2C_2_CLK_BIT       PIO_BIT_6
 #define I2C_2_DATA_BIT      PIO_BIT_7
 #define I2C_2_BASE_ADDRESS  (U32 *)RESET_I2C_BASE
 #define I2C_2_INT_NUMBER    SSC_4_INTERRUPT
 #define I2C_2_INT_LEVEL     SSC_0_INTERRUPT_LEVEL
 
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

#elif defined(espresso) || defined(ST_5100)|| defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_8010)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

#elif defined(ST_7100) || defined(ST_7109)  || defined(ST_7200)

  #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS 

   #define TUNER2_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER2_TUNER_I2C   I2C_DEVICE_FRONTBUS

/* presume 5514 (mediaref) tuners are on a common bus */
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5105)
 #define TUNER0_DEMOD_I2C  I2C_DEVICE_FRONTBUS
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

















#ifndef ST_OSLINUX
/* Private variables ------------------------------------------------------ */

#if defined(ST_5105)
U32                             ReadValue               = 0xFFFF;
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

/* Stored table of channels */
static TUNER_Channel_t          ChannelTable[5][TUNER_TABLE_SIZE];
static U8                       ChannelTableCount[5];

/* Test harness revision number */
static U8                       Revision[]              = "1.0.0A0";

/*For displaying the Platform & Backend in use*/
static char                     Platform[20];
static char                     Backend[20];

/* Tuner device name -- used for all tests */
static ST_DeviceName_t          TunerDeviceName[5 + 1];/*assuming 4 max tuner devices connected on the board*/
/* TUNER Init parameters */
static STTUNER_InitParams_t     TunerInitParams[5];
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
/* List of PIO device names */
static ST_DeviceName_t          PioDeviceName[NUM_PIO + 2];
/*#endif*/ /*ST_OSLINUX*/
#endif  /*USE_STI2C_WRAPPER*/

/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
/* PIO Init parameters */
#ifndef ST_OSLINUX
static STPIO_InitParams_t       PioInitParams[NUM_PIO];
#endif
/*#endif*/ /*ST_OSLINUX*/
#endif  /*USE_STI2C_WRAPPER*/
/* List of I2C device names */
static ST_DeviceName_t          I2CDeviceName[NUM_I2C + 1];
/* I2C Init params */
static STI2C_InitParams_t       I2CInitParams[NUM_I2C];

/* Global semaphore used in callbacks */
static semaphore_t             *EventSemaphore[4];
static STTUNER_EventType_t      LastEvent[4];


#define MAX_NUM_RECEIVE_BYTES   7
#ifndef ST_OSLINUX
/* Declarations for memory partitions */
#ifndef ST_OS21
/* Allow room for OS20 segments in internal memory */
static unsigned char            internal_block[ST20_INTERNAL_MEMORY_SIZE - 1200];
static partition_t              the_internal_partition;
partition_t                    *internal_partition      = &the_internal_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif


#define                 NCACHE_MEMORY_SIZE          0x80000
static unsigned char            ncache_memory[NCACHE_MEMORY_SIZE];
static partition_t              the_ncache_partition;
partition_t                    *ncache_partition        = &the_ncache_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( ncache_memory , "ncache_section" )
#endif

/* 2Mb */
#define                 SYSTEM_MEMORY_SIZE       0x200000
static unsigned char            external_block[SYSTEM_MEMORY_SIZE];
static partition_t              the_system_partition;
partition_t                    *system_partition        = &the_system_partition;
partition_t                    *driver_partition        = &the_system_partition;

#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( external_block, "system_section")
#endif


#else /* ST_OS21*/
/* 2Mb */
#define                 SYSTEM_MEMORY_SIZE          0x200000
static unsigned char            external_block[SYSTEM_MEMORY_SIZE];
partition_t                    *system_partition;
partition_t                    *driver_partition;
#endif /*ST_OS21 */
#endif /*ST_OSLINUX*/

#ifdef ST_OSLINUX
int var;
#define system_partition &var;
#endif
static STTUNER_Band_t           BandList[10];
static STTUNER_Scan_t           ScanList[10];

static STTUNER_BandList_t       Bands;

static STTUNER_ScanList_t       Scans;

static const ST_DeviceName_t    EVT_RegistrantName[4]   =
{
   "stevt_1", "stevt_2", "stevt_3", "stevt_4"
};

/* Private function prototypes -------------------------------------------- */
char *TUNER_FecToString(STTUNER_FECRate_t Fec);
char *TUNER_DemodToString(STTUNER_Modulation_t Mod);
char *TUNER_PolarityToString(STTUNER_Polarization_t Plr);
char *TUNER_EventToString(STTUNER_EventType_t Evt);
char *TUNER_AGCToPowerOutput(U8 Agc);
char *TUNER_ToneToString(STTUNER_LNBToneState_t ToneState);

int PowOf10(int number);
long GetPowOf10(int number);
long GetPowOf10_BER(int number);
char *TUNER_TER_SpectrumToString(STTUNER_Spectrum_t Spectrum);
char *TUNER_TER_StatusToString(STTUNER_Status_t Status);
char *TUNER_TER_ModulationToString(STTUNER_Modulation_t Modul);
char *TUNER_TER_FecToString(STTUNER_FECRate_t Fec);

static char *STAPI_ErrorMessages(ST_ErrorCode_t ErrCode);
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_boot(void);
#ifndef DISABLE_TOOLBOX
static ST_ErrorCode_t Init_sttbx(void);
#endif
#endif /* ST_OSLINUX */
#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_pio(void);
#endif
#endif /* USE_STI2C_WRAPPER */
static ST_ErrorCode_t Init_i2c(void);
static ST_ErrorCode_t Init_tunerparams(void);
static ST_ErrorCode_t Init_scanparams(void);
static ST_ErrorCode_t Init_evt(void);
static ST_ErrorCode_t Open_evt(void);
static ST_ErrorCode_t GlobalTerm(void);
#ifdef ST_7200
static ST_ErrorCode_t Reset_NIM(void);
#endif
/*static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p);*/
#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerAPI(TUNER_TestParams_t *Params_p);
#endif
static TUNER_TestResult_t TunerTerm(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerReLock(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerScan(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTimedScan(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerScanExact(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTimedScanExact(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerTracking(TUNER_TestParams_t *Params_p);
/*static TUNER_TestResult_t TunerDiSEqC2(TUNER_TestParams_t *Params_p);*/

#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerMultiInstanceAPI(TUNER_TestParams_t *Params_p);
#endif
static TUNER_TestResult_t TunerMultiInstanceScanStabilityTest(TUNER_TestParams_t *Params_p); /* New Function for MultiInstance Scan Stablity test */
static TUNER_TestResult_t TunerMultiInstanceScanSimultaneous(TUNER_TestParams_t *Params_p); /* New Function for MultiInstance Scan */

static TUNER_TestResult_t TunerMultiInstanceTimedScan(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerMultiInstanceScanExact(TUNER_TestParams_t *Params_p);
static TUNER_TestResult_t TunerMultiInstanceTimedScanExact(TUNER_TestParams_t *Params_p);

static ST_DeviceName_t  I2C_DeviceName[]    =
{
   "STI2C[1]", "STI2C[0]"
};

void Sat_init();
void Ter_init();



#ifdef STTUNER_DISEQC2_SW_DECODE_VIA_PIO
#define MAX_NUM_RECEIVE_BYTES   7
/*static TUNER_TestResult_t TunerDiSEqC2(TUNER_TestParams_t *Params_p);*/
#ifdef TEST_DUAL
/*static TUNER_TestResult_t TunerDiSEqC2MultiInstance(TUNER_TestParams_t *Params_p);*/
#endif 
#endif

/******* Declarations for DiSEqC Tests****************/
#undef DISEQC_TEST_ON /* for normal unitary test undef */

ST_ErrorCode_t TunerDiseqc(STTUNER_Handle_t TUNERHandle);
/* declaration of the ISR Functions */
void CaptureReplyPulse1(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
void CaptureReplyPulse2(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);
void CaptureReplyPulse3(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);

/**************************************************************************/

/* For Probing*/


static ST_DeviceName_t  EVT_DeviceName[]    =
{
   "stevt"
};


static S32              I2C_Bus[TUNER_MAX_USED];
static U8               DemodAddress[TUNER_MAX_USED], LnbAddress[TUNER_MAX_USED];
static U32               TunerInst           = 0;
static char TunerType[TUNER_MAX_USED+2][8]; 


U8                          Notify_ID       = 0;


/**************************************************************************/

/* Test table */
static TUNER_TestEntry_t    TestTable[]     =
{
 #ifndef ST_OSLINUX
 { TunerAPI,                "Test the stablility of the STTUNER API",                             1 },
 #endif
 { TunerTerm,               "Termination during a scan",                                          1 }, 
 { TunerScan,               "Scan test to acquire channels in band",                              1 },
 { TunerReLock,             "Relock channel test ",                                               1 }, 
 { TunerTimedScan,          "Speed test for a scan operation",                                    1 },
 { TunerScanExact,          "Scan exact through all acquired channels",                           1 }, 
 { TunerTimedScanExact,     "Speed test for locking to acquired channels",                        1 },
 { TunerTracking,           "Test tracking of a locked frequency",                                1 },
 #ifndef ST_OSLINUX
 { TunerMultiInstanceAPI,                   "Test the stablility of the STTUNER API for  tuners", 1 },
 #endif
 { TunerMultiInstanceScanSimultaneous,       "Test Simultaneous scan for  tuners",                 1 },
 { TunerMultiInstanceScanStabilityTest,      "Test Stability scan for  tuners  ",                  1 }, 
 { TunerMultiInstanceTimedScan,              "Speed test for a scan operation",                    1 }, 
 { TunerMultiInstanceTimedScanExact,         "Speed Scan exact for all found channels",            1 },
 { TunerMultiInstanceScanExact,              "Scan exact for all found channels",                  1 }, 
 { 0,                   "",                                                                        0 }};



/* Function definitions --------------------------------------------------- */

int
main(int argc, char *argv[])
{
   ST_ErrorCode_t       error   = ST_NO_ERROR;
   TUNER_TestResult_t   GrandTotal, Result;
   TUNER_TestResult_t   Total[5];
   TUNER_TestEntry_t   *Test_p;
   U32                  Section;
   S32                  tuners;
   U8 TestCount=0;
   TUNER_TestParams_t   TestParams;
   #ifndef ST_OSLINUX
   STBOOT_TermParams_t  BootTerm;
   #endif


    char temp1[50];/*max length of string describing LLA version=49+1(for '\0');total 5 demods,therefore 5*50=250*/
	char * LLARevision1=temp1;
	memset(LLARevision1,'\0',sizeof(temp1));


   #if defined(ST_5510)
   strcpy(Backend, "STi5510");
   #elif defined(ST_5508)
   strcpy(Backend, "STi5508");
   #elif defined(ST_5512)
   strcpy(Backend, "STi5512");
   #elif defined(ST_5514)
   strcpy(Backend, "STi5514");
   #elif defined(ST_5516)
   strcpy(Backend, "STi5516");
   #elif defined(ST_5517)
   strcpy(Backend, "STi5517");
   #elif defined(ST_5518)
   strcpy(Backend, "STi5518");
   #elif defined(ST_5528)
   strcpy(Backend, "STi5528");
   #elif defined(ST_5100)
   strcpy(Backend, "STi5100");
   #elif defined(ST_7710)
   strcpy(Backend, "STi7710");
   #elif defined(ST_7100)
   strcpy(Backend, "STi7100");
   #elif defined(ST_7200)
   strcpy(Backend, "STi7200");
   #elif defined(ST_5105)
   strcpy(Backend, "STi5105");
   #elif defined(ST_5301)
   strcpy(Backend, "STi5301");
   #elif defined(ST_8010)
   strcpy(Backend, "STm8010");
   #endif

   #if defined(mb231)
   strcpy(Platform, "mb231");
   #elif defined(mb275)
   strcpy(Platform, "mb275");
   #elif defined(mb282)
   strcpy(Platform, "mb282");
   #elif defined(mb314)
   strcpy(Platform, "mb314");
   #elif defined(mb361)
   strcpy(Platform, "mb361");
   #elif defined(mb382)
   strcpy(Platform, "mb382");
   #elif defined(mb5518)
   strcpy(Platform, "mb5518");
   #elif defined(mb376)
   strcpy(Platform, "mb376");
   #elif defined(espresso)
   strcpy(Platform, "espresso");
   #elif defined(mb390)
   strcpy(Platform, "mb390");
   #elif defined(mb391)
   strcpy(Platform, "mb391");
   #elif defined(mb400)
   strcpy(Platform, "mb400");
   #elif defined(mb411)
   strcpy(Platform, "mb411");
   #elif defined(mb421)
   strcpy(Platform, "mb421");
   #elif defined(mb519)
   strcpy(Platform, "mb519");
   #endif
   #ifndef ST_OSLINUX
   error = Init_boot();
   if (error != ST_NO_ERROR)
   {
      DebugMessage("Init_boot failed with error \n");
      return(0);
   }
   #ifndef DISABLE_TOOLBOX

   error = Init_sttbx();
   if (error != ST_NO_ERROR)
   {
      DebugMessage("Init_sttbx failed with error\n");
      return(0);
   }
   DebugPrintf(("Init_boot....Init_sttx....Init_evt...."));
   #endif
   #endif /* ST_OSLINUX */
   error = Init_evt();

   if (error != ST_NO_ERROR)
   {
      DebugMessage("Init_evt failed with error\n");
      return(0);
   }

   #ifndef ST_OSLINUX
   
   

   STTBX_Print(("Init_pio...."));
   if (Init_pio() != ST_NO_ERROR)
   {
      DebugError("\nInit_pio failed with error\n", error);
      return(0);
   }
   
  
   
   
   #endif /* ST_OSLINUX */
   
   

   STTBX_Print(("Init_i2c...."));
   error = Init_i2c();
   if (error != ST_NO_ERROR)
   {
      DebugMessage("\nInit_i2c failed with error\n");
      DebugError("\nInit_i2c()", error);
      return(0);
   }
   STTBX_Print(("success\n\n"));
   
   
   
   #ifdef ST_7200
   Reset_NIM();
   #endif
   
   
   

   if (Init_tunerparams() != ST_NO_ERROR)
   {
      DebugMessage("Init_tunerparams failed with error\n");
      return(0);
   }

   if (Open_evt() != ST_NO_ERROR)
   {
      DebugMessage("Open_evt failed with error\n");
      return(0);
   }


   /* ---------------------------------------- */
   #ifndef ST_OSLINUX
   task_priority_set(NULL, 0);
   #endif

   DebugMessage("**************************************************\n");
   DebugMessage("                STTUNER Test Harness              \n");
   DebugPrintf(("   Test Harness Revision: %s\n", Revision));
   DebugPrintf(("                Platform: %s\n", Platform));
   DebugPrintf(("                 Backend: %s\n", Backend));
   DebugPrintf(("               I+D Cache: Y\n"));
   #if defined(UNIFIED_MEMORY)
   DebugPrintf(("          Unified memory: Y\n"));
   #else
   DebugPrintf(("          Unified memory: N\n"));
   #endif

   DebugMessage("**************************************************\n");

   DebugMessage("--------------------------------------------------\n");
   DebugPrintf(("                Driver revisions                  \n"));
   DebugMessage("--------------------------------------------------\n");
   #ifndef ST_OSLINUX
   DebugPrintf(("           STBOOT:  %s\n", STBOOT_GetRevision()));
   DebugPrintf(("           STCOMMON:  %s\n", STCOMMON_REVISION));
   DebugPrintf(("           STEVT:  %s\n", STEVT_GetRevision()));
   DebugPrintf(("           STI2C:  %s\n", STI2C_GetRevision()));
   DebugPrintf(("           STPIO:  %s\n", STPIO_GetRevision()));
   DebugPrintf(("           STSYS:  %s\n", STSYS_GetRevision()));
   #ifndef DISABLE_TOOLBOX
   DebugPrintf(("           STTBX:  %s\n", STTBX_GetRevision()));
   #ifndef STTBX_NO_UART
   DebugPrintf(("           STUART:  %s\n", STUART_GetRevision()));
   #endif
   #endif
   #endif /* ST_OSLINUX */
   DebugPrintf(("           STTUNER:  %s\n",(char *)STTUNER_GetRevision()));
   
   #ifdef TUNER_297
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0297)); 
    DebugPrintf(("         297 LLA: %s\n",   LLARevision1 ));
    #endif
    #ifdef TUNER_297J
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0297J)); 
    DebugPrintf(("         297J LLA: %s\n",   LLARevision1 ));
    #endif
    #ifdef TUNER_297E
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0297E)); 
    DebugPrintf(("         297E LLA: %s\n",   LLARevision1 ));
    #endif
    #ifdef TUNER_370QAM
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STB0370QAM)); 
    DebugPrintf(("         370QAM LLA: %s\n",   LLARevision1 ));
    #endif
   
   #ifndef STTUNER_MINIDRIVER
   
     #ifdef TUNER_361
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0361)); 
    DebugPrintf(("          361 LLA: %s\n",   LLARevision1 ));
    #elif defined (TUNER_362)
     strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0362)); 
    DebugPrintf(("          362 LLA: %s\n",   LLARevision1 ));
    #elif defined (TUNER_360)
     strcpy(LLARevision1, (char*)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0360));
    DebugPrintf(("          360 LLA: %s\n",  LLARevision1 ));
    #endif
    #endif
    
    #ifdef TUNER_370VSB
     strcpy(LLARevision1,(char*) STTUNER_GetLLARevision(STTUNER_DEMOD_STB0370VSB));
    DebugPrintf(("          370VSB LLA: %s\n",   LLARevision1 ));
    #elif defined(TUNER_372)
    strcpy(LLARevision1, (char*)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0372));
    DebugPrintf(("          372 LLA: %s\n",   LLARevision1 ));
    #endif
        
   #ifdef TUNER_899 
   strcpy(LLARevision1,(char*) STTUNER_GetLLARevision(STTUNER_DEMOD_STB0899));
    DebugPrintf(("          899 LLA: %s\n",   LLARevision1  ));
   #endif
   #if defined(TUNER_288) 
   strcpy(LLARevision1,(char*) STTUNER_GetLLARevision(STTUNER_DEMOD_STX0288));
    DebugPrintf(("          288 LLA: %s\n", LLARevision1 ));
   #endif
 

   DebugMessage("--------------------------------------------------\n");
   DebugMessage("                  Test built on\n");
   DebugPrintf(("        Date: %s   Time: %s \n", __DATE__, __TIME__));
   DebugMessage("--------------------------------------------------\n");

   GrandTotal.NumberPassed = 0;
   GrandTotal.NumberFailed = 0;

   for (tuners = 0; tuners < TunerInst; tuners++)
   {
      Init_scanparams();
      Test_p = TestTable;
      Section = 1;
      ChannelTableCount[tuners] = 0;

      Total[tuners].NumberPassed = 0;
      Total[tuners].NumberFailed = 0;

      DebugMessage("==================================================\n");
      DebugPrintf(("  Test tuner:        '%s'\n", TunerType[tuners]));
      DebugMessage("==================================================\n");




         while (Test_p->TestFunction != NULL )
         {
           

            /* Set up parameters for test function */
            TestParams.Ref = Section;
            TestParams.Tuner = tuners;
            /* Call test */
           for (TestCount=0; TestCount < Test_p->RepeatCount;  TestCount++)
            {
            DebugMessage("**************************************************\n");
            DebugPrintf(("SECTION %d - %s\n", Section, Test_p->TestInfo));
            DebugMessage("**************************************************\n");
            Result = Test_p->TestFunction(&TestParams);
            Total[tuners].NumberPassed += Result.NumberPassed;
            Total[tuners].NumberFailed += Result.NumberFailed;
            DebugMessage("--------------------------------------------------\n");
            DebugPrintf((" Running total: PASSED: %d\n", Total[tuners].NumberPassed));
            DebugPrintf(("                FAILED: %d\n", Total[tuners].NumberFailed));
            DebugMessage("--------------------------------------------------\n");
            }
            /* Update counters */
            
            Test_p++;
            Section++;

         }

      GrandTotal.NumberPassed += Total[tuners].NumberPassed;
      GrandTotal.NumberFailed += Total[tuners].NumberFailed;
   }

   DebugMessage("==================================================\n");
   DebugPrintf((" Grand Total: PASSED: %d\n", GrandTotal.NumberPassed));
   DebugPrintf(("              FAILED: %d\n", GrandTotal.NumberFailed));
   DebugMessage("==================================================\n");
   GlobalTerm();
   #ifndef ST_OSLINUX
   STBOOT_Term("boot", &BootTerm);
   #endif
   return(0);
}

/* ------------------------------------------------------------------------- */
/*                     EVENT callback & wait functions                       */
/* ------------------------------------------------------------------------- */

/* If using normal callback function */
#ifndef STEVT_ENABLED
static void
CallbackNotifyFunction(STTUNER_Handle_t Handle, STTUNER_EventType_t EventType, ST_ErrorCode_t Error)
{
   LastEvent[Notify_ID] = EventType;

   STOS_SemaphoreSignal(EventSemaphore[Notify_ID]);

}
#else
#define CallbackNotifyFunction       NULL

/* If using Event Handler API */
static void
EVTNotifyFunction(STEVT_CallReason_t     Reason, const ST_DeviceName_t  RegistrantName, STEVT_EventConstant_t  Event, const void *EventData, const void *SubscriberData_p)




{
   LastEvent[Notify_ID] = Event;

   STOS_SemaphoreSignal(EventSemaphore[Notify_ID]);
}
#endif


static void
AwaitNotifyFunction(STTUNER_EventType_t *EventType_p)
{
   #ifdef ST_OS21
   osclock_t    time;
   #else
   clock_t      time;
   #endif
   int          result; 

   time = time_plus(time_now(), 60 * ST_GetClocksPerSecond()); /*Timeout maybe increased if scanning a large frequency band*/

   result = STOS_SemaphoreWaitTimeOut(EventSemaphore[Notify_ID], &time);


   if (result == -1)
      *EventType_p = STTUNER_EV_TIMEOUT;
   else
      *EventType_p = LastEvent[Notify_ID];

}

/* ------------------------------------------------------------------------- */
/*                       Initalization functions                             */
/* ------------------------------------------------------------------------- */
#ifndef ST_OSLINUX
static ST_ErrorCode_t
Init_boot(void)
{
   STBOOT_InitParams_t      BootParams;
   /* Cache Map */

   STBOOT_DCache_Area_t     DCacheMap[] =
   {
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
   system_partition = partition_create_heap((U8 *) external_block, sizeof(external_block));
   #else
   partition_init_simple(&the_internal_partition, (U8 *) internal_block, sizeof(internal_block));
   partition_init_heap(&the_system_partition, (U8 *) external_block, sizeof(external_block));
   partition_init_heap(&the_ncache_partition, ncache_memory, sizeof(ncache_memory));
   #endif    

   BootParams.SDRAMFrequency = SDRAM_FREQUENCY;
   BootParams.CacheBaseAddress = (U32 *) CACHE_BASE_ADDRESS;

   BootParams.ICacheEnabled = TRUE;
   BootParams.DCacheMap = DCacheMap;

   BootParams.BackendType.DeviceType = STBOOT_DEVICE_UNKNOWN;
   BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
   BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

   #if defined(ST_5512) || defined(mb5518)
   BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_64_MBIT;
   #elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100)|| defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100) || defined(ST_5301) || defined(ST_8010)  /* do not init SMI memory */
   BootParams.MemorySize = SDRAM_SIZE;
   #else /* Default 32Mbit on 5510 processor */
   BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_32_MBIT;
   #endif

   return(STBOOT_Init("boot", &BootParams));
}
#endif /* ST_OSLINUX */
/* ------------------------------------------------------------------------- */
#ifndef ST_OSLINUX
#ifndef DISABLE_TOOLBOX
static ST_ErrorCode_t
Init_sttbx(void)
{
   STTBX_InitParams_t   TBXInitParams;

   memset(&TBXInitParams, '\0', sizeof(STTBX_InitParams_t));
   TBXInitParams.SupportedDevices = STTBX_DEVICE_DCU;
   TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
   TBXInitParams.DefaultInputDevice = STTBX_DEVICE_DCU;
   TBXInitParams.CPUPartition_p = system_partition;

   return(STTBX_Init("tbx", &TBXInitParams));
}
#endif
#endif /* ST_OSLINUX */
/* ------------------------------------------------------------------------- */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
static ST_ErrorCode_t
Init_pio(void)
{
   ST_DeviceName_t  DeviceName;
   ST_ErrorCode_t   error   = ST_NO_ERROR;
   U32              i;

   /* Automatically generate device names */
   for (i = 0; i < NUM_PIO; i++)       /* PIO */
   {
      sprintf((char*) DeviceName, "STPIO[%d]", i);
      strcpy((char*) PioDeviceName[i], (char*) DeviceName);
   }

   PioDeviceName[i][0] = 0;

   PioInitParams[0].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[0].BaseAddress = (U32 *) PIO_0_BASE_ADDRESS;
   PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
   PioInitParams[0].InterruptLevel = PIO_0_INTERRUPT_LEVEL;

   PioInitParams[1].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[1].BaseAddress = (U32 *) PIO_1_BASE_ADDRESS;
   PioInitParams[1].InterruptNumber = PIO_1_INTERRUPT;
   PioInitParams[1].InterruptLevel = PIO_1_INTERRUPT_LEVEL;

   PioInitParams[2].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[2].BaseAddress = (U32 *) PIO_2_BASE_ADDRESS;
   PioInitParams[2].InterruptNumber = PIO_2_INTERRUPT;
   PioInitParams[2].InterruptLevel = PIO_2_INTERRUPT_LEVEL;

   PioInitParams[3].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[3].BaseAddress = (U32 *) PIO_3_BASE_ADDRESS;
   PioInitParams[3].InterruptNumber = PIO_3_INTERRUPT;
   PioInitParams[3].InterruptLevel = PIO_3_INTERRUPT_LEVEL;

   #ifndef ST_5105
   PioInitParams[4].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[4].BaseAddress = (U32 *) PIO_4_BASE_ADDRESS;
   PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
   PioInitParams[4].InterruptLevel = PIO_4_INTERRUPT_LEVEL;
   #endif
   
   #ifdef ST_7200
   PioInitParams[5].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[5].BaseAddress = (U32 *) PIO_5_BASE_ADDRESS;
   PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
   PioInitParams[5].InterruptLevel = PIO_5_INTERRUPT_LEVEL;
   
   PioInitParams[6].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[6].BaseAddress = (U32 *) PIO_6_BASE_ADDRESS;
   PioInitParams[6].InterruptNumber = PIO_6_INTERRUPT;
   PioInitParams[6].InterruptLevel = PIO_6_INTERRUPT_LEVEL;
   
   PioInitParams[7].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[7].BaseAddress = (U32 *) PIO_7_BASE_ADDRESS;
   PioInitParams[7].InterruptNumber = PIO_7_INTERRUPT;
   PioInitParams[7].InterruptLevel = PIO_7_INTERRUPT_LEVEL;
   #endif
   
   #if defined(ST_5518)
   PioInitParams[5].DriverPartition = (ST_Partition_t *) system_partition;
   PioInitParams[5].BaseAddress = (U32 *) PIO_5_BASE_ADDRESS;
   PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
   PioInitParams[5].InterruptLevel = PIO_5_INTERRUPT_LEVEL;
   #endif

   /* Initialize drivers */
   for (i = 0; i < NUM_PIO; i++)
   {
      error = STPIO_Init(PioDeviceName[i], &PioInitParams[i]);
      /*DebugMessage(" STPIO_Init()");*/
      if (error != ST_NO_ERROR)
      {
         DebugMessage("Unable to initialize STPIO\n");
         return(error);
      }
   }
   /*DebugMessage(" .\n");*/

   return(error);
}

#endif 
#endif  /*USE_STI2C_WRAPPER*/
/*#endif */ /*ST_OSLINUX*/


#ifdef ST_7200
static ST_ErrorCode_t Reset_NIM(void)
{
STI2C_Handle_t Handle;
ST_ErrorCode_t Error;
STI2C_OpenParams_t	OpenParams;

U8 Buffer_Rd[2],Buffer_Wr[2];
U32 ActLen;

OpenParams.I2cAddress = 0x4E;                  
OpenParams.AddressType = STI2C_ADDRESS_7_BITS;
OpenParams.BusAccessTimeOut = 100000;	

  Error = STI2C_Open ("STI2C[2]", &OpenParams, &Handle);
  if (Error != ST_NO_ERROR)
     {
	DebugMessage (("Cant open I2C device STI2C[2] for NIM reset\n"));
     }
  else
     {
    /* Read from a device with a 10 ms timeout */

    Error |= STI2C_Read  (Handle, Buffer_Rd,2, 10, &ActLen);
    Buffer_Wr[0]=Buffer_Rd[0];
    Buffer_Wr[1]=Buffer_Rd[1] & 0xF0;
    Error |= STI2C_Write (Handle, Buffer_Wr, 2, 10, &ActLen);
    Buffer_Wr[1]=Buffer_Rd[1] | 0x0F;
    Error |= STI2C_Write (Handle, Buffer_Wr, 2, 10, &ActLen);
  
     if (Error != ST_NO_ERROR)
	{
	DebugMessage (("NIM Reset failed Error in I2C R/W\n"));
        }
     }

	Error=STI2C_Close (Handle);

        return (Error);
}
#endif




/* ------------------------------------------------------------------------- */
#ifndef USE_STI2C_WRAPPER
static ST_ErrorCode_t
Init_i2c(void)
{
   ST_ErrorCode_t   error   = ST_NO_ERROR;
   ST_DeviceName_t  DeviceName;
   U32              i;

   for (i = 0; i < NUM_I2C; i++)       /* I2C */
   {
      sprintf((char*) DeviceName, "STI2C[%d]", i);
      strcpy((char*) I2CDeviceName[i], (char*) DeviceName);
   }


   I2CInitParams[0].BaseAddress = I2C_0_BASE_ADDRESS;
   I2CInitParams[0].PIOforSDA.BitMask = I2C_0_DATA_BIT;
   I2CInitParams[0].PIOforSCL.BitMask = I2C_0_CLK_BIT;
   I2CInitParams[0].InterruptNumber = I2C_0_INT_NUMBER;
   I2CInitParams[0].InterruptLevel = I2C_0_INT_LEVEL;
   I2CInitParams[0].MasterSlave = STI2C_MASTER;
   I2CInitParams[0].BaudRate = /*I2C_BAUDRATE;*/100000;
   #if defined(ST_5301) || defined(ST_8010)
   I2CInitParams[0].ClockFrequency = STCOMMON_IC_CLOCK;
   #else
   I2CInitParams[0].ClockFrequency = ST_I2C_CLOCKFREQUENCY;
   #endif
   I2CInitParams[0].MaxHandles = 8; /* 2 per stv0299, 1 p
   r stv0399*/
   I2CInitParams[0].DriverPartition = system_partition;

   #ifdef ST_5100
   I2CInitParams[0].GlitchWidth = 0x05;
   #endif
   strcpy(I2CInitParams[0].PIOforSCL.PortName, PioDeviceName[I2C_0_CLK_DEV]);
   strcpy(I2CInitParams[0].PIOforSDA.PortName, PioDeviceName[I2C_0_DATA_DEV]);

   I2CInitParams[1].BaseAddress = I2C_1_BASE_ADDRESS;
   I2CInitParams[1].PIOforSDA.BitMask = I2C_1_DATA_BIT;
   I2CInitParams[1].PIOforSCL.BitMask = I2C_1_CLK_BIT;
   I2CInitParams[1].InterruptNumber = I2C_1_INT_NUMBER;
   I2CInitParams[1].InterruptLevel = I2C_1_INT_LEVEL;
   I2CInitParams[1].MasterSlave = STI2C_MASTER;
   I2CInitParams[1].BaudRate = I2C_BAUDRATE;
   #if defined(ST_5301) || defined(ST_8010)
   I2CInitParams[1].ClockFrequency = STCOMMON_IC_CLOCK;
   #else
   I2CInitParams[1].ClockFrequency = ST_I2C_CLOCKFREQUENCY;
   #endif
   I2CInitParams[1].MaxHandles = 8; /* 2 per stv0299, 1 per stv0399*/
   I2CInitParams[1].DriverPartition = system_partition;

   #ifdef ST_5100
   I2CInitParams[1].GlitchWidth = 0x05;
   #endif
   strcpy(I2CInitParams[1].PIOforSDA.PortName, PioDeviceName[I2C_1_DATA_DEV]);
   strcpy(I2CInitParams[1].PIOforSCL.PortName, PioDeviceName[I2C_1_CLK_DEV]);

   #ifdef ST_7200
   I2CInitParams[2].BaseAddress = I2C_2_BASE_ADDRESS;
   I2CInitParams[2].PIOforSDA.BitMask = I2C_2_DATA_BIT;
   I2CInitParams[2].PIOforSCL.BitMask = I2C_2_CLK_BIT;
   I2CInitParams[2].InterruptNumber = I2C_2_INT_NUMBER;
   I2CInitParams[2].InterruptLevel = I2C_2_INT_LEVEL;
   I2CInitParams[2].MasterSlave = STI2C_MASTER;
   I2CInitParams[2].BaudRate = 100000;
   I2CInitParams[2].ClockFrequency = ST_I2C_CLOCKFREQUENCY;
   I2CInitParams[2].MaxHandles = 8; 
   I2CInitParams[2].DriverPartition = system_partition;
   
   strcpy(I2CInitParams[2].PIOforSCL.PortName, PioDeviceName[I2C_2_CLK_DEV]);
   strcpy(I2CInitParams[2].PIOforSDA.PortName, PioDeviceName[I2C_2_DATA_DEV]);
   #endif


   for (i = 0; i < NUM_I2C; i++)
   {

      if (I2CInitParams[i].BaseAddress != 0) /* Is in use? */
      {
         error = STI2C_Init(I2CDeviceName[i], &I2CInitParams[i]);
         if (error != ST_NO_ERROR)
         {
            DebugMessage("Unable to initialize STI2C\n");
            return(error);
         }
         /* else
          {
              DebugMessage("ok ");
          }*/
      }
      else
      {
         DebugMessage("fail\n");
      }
   }
   /* DebugMessage(" done\n");*/
   #if defined(ST_5105)
   /* Configure Config Control Reg G for selecting ALT0 functions for PIO3<3:2> */
   /* 0000_0000_0000_0000_0000_0000_0000_0000 (INT_CONFIG_CONTROL_G)            */
   /* |||| |||| |||| |||| |||| |||| |||| ||||_____ pio2_altfop_mux0sel<7:0>     */
   /* |||| |||| |||| |||| |||| ||||_______________ pio2_altfop_mux1sel<7:0>     */
   /* |||| |||| |||| ||||_________________________ pio3_altfop_mux0sel<7:0>     */
   /* |||| ||||___________________________________ pio3_altfop_mux1sel<7:0>     */
   /* ========================================================================= */
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
   STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, ReadValue | 0x00000000);
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
   /* Configure Config Control Reg H for selecting ALT1 functions for PIO4<3:2> */
   /*                0000_0000_0000_0000_1100 (INT_CONFIG_CONTROL_H)            */
   /*                |||| |||| |||| |||| ||||_____ pio4_altfop_mux0sel<3:0>     */
   /*                |||| |||| |||| ||||__________ Reserved<3:0>                */
   /*                |||| |||| ||||_______________ pio4_altfop_mux1sel<3:0>     */
   /*                |||| ||||____________________ Reserved<3:0>                */
   /* ========================================================================= */
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
   STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, ReadValue | 0x0c000000);
   ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
   #endif
   return(error);
}
#else /* USE_STI2C_WRAPPER */

static ST_ErrorCode_t
Init_i2c(void)
{
   ST_ErrorCode_t   error   = ST_NO_ERROR;
   ST_DeviceName_t  DeviceName;
   U32              i;

   for (i = 0; i < NUM_I2C; i++)       /* I2C */
   {
      sprintf((char*) DeviceName, "STI2C[%d]", i);

      strcpy((char*) I2CDeviceName[i], (char*) DeviceName);

   }

   I2CDeviceName[i][0] = 0;

   error = STI2C_Init(I2CDeviceName[0], &I2CInitParams[0]);
   if (error != ST_NO_ERROR)
   {
      DebugMessage("Unable to initialize STI2C\n");
   }
   else
   {
      DebugMessage("ok\n");
   }

   return error;
}

#endif /* USE_STI2C_WRAPPER */


/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Init_tunerparams(void)
{

        ST_ErrorCode_t ST_ErrorCode;
        TUNER_ProbeParams_t           ProbeParams;
        U8                            BusIndex = 0;

        int                           ScanBus[] = {I2C_BACK_BUS,I2C_FRONT_BUS,  -1};

        U32                           AddressList[]={
                                                     DEMOD_SAT_ADDRESS1,                
						     DEMOD_SAT_ADDRESS2,                
						     DEMOD_SAT_ADDRESS3,              
						     DEMOD_SAT_ADDRESS4,                        
						     DEMOD_TER_CAB_ADDRESS1,         
						     DEMOD_TER_CAB_ADDRESS2,          
						     DEMOD_TER_ADDRESS3,             
						     DEMOD_TER_ADDRESS4,
						     DEMOD_TER_ADDRESS5,         
						     DEMOD_TER_ADDRESS6,          
						     DEMOD_TER_ADDRESS7,             
						     DEMOD_TER_ADDRESS8,           
                                                     0xFF};
        U8                            AddressIndex=0;


            ProbeParams.Driver     = STTUNER_IODRV_I2C;
            ProbeParams.SubAddress = 0;
            ProbeParams.XferSize   = 1;
            ProbeParams.TimeOut    = 50;
            
            
DebugMessage("*********************************************************************\n");
DebugPrintf(("                      DETECTED TUNERS\n"));

     
     
     
            while( (ScanBus[BusIndex] != -1) )
            {
                strcpy(ProbeParams.DriverName, I2C_DeviceName[ScanBus[BusIndex]]);  /* set I2C bus to scan */

                for( AddressIndex = 0; (AddressList[AddressIndex] != 0xFF) ; AddressIndex++ )
                {
                    ProbeParams.Value   = 0;                            /* clear value for return */
                    ProbeParams.Address = AddressList[AddressIndex];    /* address of next chip to try */
                    strcpy(TunerType[TunerInst],"UNKNOWN");
                		
                    ST_ErrorCode = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
                   /*DebugPrintf(("Probed Address %x   on Bus %s    %s      Chip ID %x\n",ProbeParams.Address,I2C_DeviceName[ScanBus[BusIndex]],STAPI_ErrorMessages(ST_ErrorCode),ProbeParams.Value));*/

                    if ( ST_ErrorCode != ST_NO_ERROR )
                    {
                        if (( ST_ErrorCode != ST_ERROR_TIMEOUT) &&
                            ( ST_ErrorCode != STI2C_ERROR_ADDRESS_ACK )&&( ST_ErrorCode != ST_ERROR_BAD_PARAMETER ))
                        {
                            DebugPrintf(("TUNER_Ioctl(%s) Error %s\n",
                                         I2C_DeviceName[ScanBus[BusIndex]],TUNER_ErrorString(ST_ErrorCode)));
                            
                        }
                    }
                    else /* no error */
                    {
                        switch(ProbeParams.Address)
                        {
                        case DEMOD_SAT_ADDRESS1: 
                        case DEMOD_SAT_ADDRESS2:
                        case DEMOD_SAT_ADDRESS3:
                        case DEMOD_SAT_ADDRESS4:
							
                            switch(ProbeParams.Value & 0xF0)
                            {
                            /* Value of the software version in 299 register */
                            case 0xA0:
                                DebugPrintf(("Found STV0299 tuner on bus '%s'\n", I2C_DeviceName[ ScanBus[BusIndex] ] ));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                LnbAddress[TunerInst]   = LNBH_21_I2C_ADDRESS1;
                                strcpy(TunerType[TunerInst], "STV0299");
                                break;

                            case 0xB0:  /* the release number of the circuit (in ID register) in order to ensure software compatibility.*/
                                DebugPrintf(("Found STV0399 tuner on bus '%s' DemodAddress %d\n",
                                            I2C_DeviceName[ ScanBus[BusIndex] ],ProbeParams.Address ));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                LnbAddress[TunerInst]   = LNBH_21_I2C_ADDRESS1;
                                strcpy(TunerType[TunerInst],"STV0399");
                                break;

                            case 0x10: /*For tuner 288 */
                                LnbAddress[TunerInst]   = LNBH_21_I2C_ADDRESS2;
                                DebugPrintf(("Found STB0288  tuner on bus '%s' DemodAddress %x ChipID %x  LNBAddress %x\n",
                                            I2C_DeviceName[ ScanBus[BusIndex] ],ProbeParams.Address,ProbeParams.Value,LnbAddress[TunerInst] ));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;

                                
                                if (TEST_STX0288)
                                strcpy(TunerType[TunerInst],"STX0288");
                                
                                break;
                                
                            case 0x80: /*For tuner 899 */
                            case 0x30: /*For tuner 899 */

                                 I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                LnbAddress[TunerInst]   = LNBH_21_I2C_ADDRESS2;

                                DebugPrintf(("Found STB0899  tuner on bus '%s' DemodAddress %x ChipID %x  LNBAddress %x\n",
                                            I2C_DeviceName[ ScanBus[BusIndex] ],ProbeParams.Address,ProbeParams.Value,LnbAddress[TunerInst] ));
                                if (TEST_STB0899)
                                strcpy(TunerType[TunerInst],"STB0899");
                                
                                


                                break;
                                                                                   
                           default:
                                break;
                            } /* switch(ProbeParams.Value & 0xF0) */
                            break;
	   
                        case DEMOD_TER_CAB_ADDRESS1:  
                        case DEMOD_TER_CAB_ADDRESS2:
                        case DEMOD_TER_ADDRESS3:
                        case DEMOD_TER_ADDRESS4:

                         switch(ProbeParams.Value & 0xF0)
                            {
                            /* reset value of sub-address 0x00 of 360 */
                            case 0x20:
                                DebugPrintf(("Found STV0360 tuner on bus '%s'\n", I2C_DeviceName[ScanBus[BusIndex]]));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                strcpy(TunerType[TunerInst],"STV0360");
                                break;
                            /* reset value of sub-address 0x00 of 361 */
                            case 0x30:
                                DebugPrintf(("Found STV0361 tuner on bus '%s'\n", I2C_DeviceName[ScanBus[BusIndex]]));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                strcpy(TunerType[TunerInst],"STV0361");
                                break;

                            case 0x40:
                            DebugPrintf(("Found STV0362  tuner on bus '%s' DemodAddress %x ChipID %x\n",
                                            I2C_DeviceName[ ScanBus[BusIndex] ],ProbeParams.Address,ProbeParams.Value));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                if (TEST_STV0362)
                                strcpy(TunerType[TunerInst],"STV0362");
                                
                                break;
				 case 0x00:

                                DebugPrintf(("Found STV0297 tuner on bus '%s'\n", I2C_DeviceName[ScanBus[BusIndex]]));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                strcpy(TunerType[TunerInst],"STV0297");
                                break;                            
                            case 0x10:
                                DebugPrintf(("Found STV0297E tuner on bus '%s' DemodAddress %x ChipID %x\n", I2C_DeviceName[ ScanBus[BusIndex] ], ProbeParams.Address,ProbeParams.Value));

                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                if (TEST_STV297E)
                                strcpy(TunerType[TunerInst],"STV297E");
                                
                                break;
					default:
                                break;
                            } /* switch(ProbeParams.Value) */
                            break;
 		       case DEMOD_TER_ADDRESS5:  
                       case DEMOD_TER_ADDRESS6:
                       case DEMOD_TER_ADDRESS7:
                       case DEMOD_TER_ADDRESS8:
                            switch(ProbeParams.Value & 0xF0)
                            {
                            /* reset value of sub-address 0x00 of 372 */
				case 0x10:
                                DebugPrintf(("Found STV0372 tuner on bus '%s' DemodAddress %x ChipID %x LNBAddress %x\n", I2C_DeviceName[ScanBus[BusIndex]],ProbeParams.Address,ProbeParams.Value,LnbAddress[TunerInst]));
                                I2C_Bus[TunerInst]      = ScanBus[BusIndex];
                                DemodAddress[TunerInst] = ProbeParams.Address;
                                if (TEST_STV0372)
                                strcpy(TunerType[TunerInst],"STV0372");
                                
                                 break;
                            default:
                                break;
                            } /* switch(ProbeParams.Value) */
                            break;

                        default:
                            break;
                        } /* switch(ProbeParams.Address) */
                    } /* if errorcode ... */

                    /* Tuner is found */
                    if ( strcmp(TunerType[TunerInst],"UNKNOWN") )
                    {
                        TunerInst++;
                    }
                } /* for(AddressList .... */

                /* Start looking at the start of the address list on the next bus */
                BusIndex++;

            }   /* while(ScanBus[Index]) */  
            

       
DebugMessage("*********************************************************************\n");
DebugPrintf(("                      TUNERS TO BE TESTED\n"));
   
/*DebugMessage("\n");*/

#if defined (STTUNER_USE_SAT)
Sat_init(); 
#endif

#if defined (STTUNER_USE_TER)
Ter_init();   
#endif   



DebugMessage("*********************************************************************\n\n");

 ST_ErrorCode=0;    

return(ST_ErrorCode);

} /*End of Init_tunerparams*/




#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_scanparams(void)
{

 ST_ErrorCode_t   error   = ST_NO_ERROR;
 
   /* Set tuner band list */
   Bands.NumElements = 2 ;
  #ifdef C_BAND_TESTING 
        BandList[0].BandStart  = 3000000; /*L Band (LO - Transponder freq): 2200 MHz*/
        BandList[0].BandEnd   = 5150000; /*L Band (LO - Transponder freq): 900  MHz*/
        #if defined ( TUNER_899)|| defined (TUNER_288)
        BandList[0].LO            = 5150000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_C;
        #endif        
        #else /* Ku band testing */
        BandList[0].BandStart = 10600000;
        BandList[0].BandEnd   = 11700000;
        #if defined ( TUNER_899)|| defined (TUNER_288)
        BandList[0].LO        =  9750000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_Ku;
        #endif
       #endif 
   #ifdef TEST_NOIDA_DTT_TRANSMISSION
   /*** BandList[0] is set for Noida terrestrial transmission ****/
   BandList[1].BandStart = 54000;
   BandList[1].BandEnd = 858000;
  #endif     
       



  #ifdef C_BAND_TESTING
	    ScanList[2].SymbolRate   = 26670000;/*28100000;*/
	    ScanList[2].Polarization = STTUNER_PLR_HORIZONTAL;
	    ScanList[2].FECRates     = STTUNER_FEC_ALL;
	    ScanList[2].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[2].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[2].Band         = 0;
	    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
	    ScanList[2].FECType      = STTUNER_FEC_MODE_DVBS1;
	   
	    ScanList[3].SymbolRate   = 27500000;/*28100000;*/
	    ScanList[3].Polarization = STTUNER_PLR_HORIZONTAL;	    
	    ScanList[3].FECRates     = STTUNER_FEC_ALL;
	    ScanList[3].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[3].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[3].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[3].Band         = 0;
	    ScanList[3].IQMode       = STTUNER_IQ_MODE_AUTO;	    
	    ScanList[3].FECType      = STTUNER_FEC_MODE_DVBS1;
	   
  #endif /* C BAND Single Frequency */
  
	    #if defined TUNER_372
	    
	    ScanList[0].FECRates     = STTUNER_FEC_ALL;
	    ScanList[0].Modulation   = STTUNER_MOD_ALL;
	    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[0].Band         = 1;
	    ScanList[0].Spectrum     = STTUNER_INVERSION_AUTO;
	    ScanList[0].FreqOff      = STTUNER_OFFSET_NONE;
	    ScanList[0].ChannelBW    = STTUNER_CHAN_BW_8M;
	    ScanList[0].SymbolRate   =  10762000 ;
	    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
	    
	    ScanList[1].FECRates     = STTUNER_FEC_ALL;
	    ScanList[1].Modulation   = STTUNER_MOD_ALL;
	    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[1].Band         = 1;
	    ScanList[1].Spectrum     = STTUNER_INVERSION_AUTO;
	    ScanList[1].FreqOff      = STTUNER_OFFSET_NONE;
	    ScanList[1].ChannelBW    = STTUNER_CHAN_BW_8M;
	    ScanList[1].SymbolRate   =  10762000 ;
	    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;  
  #endif /* TUNER_372*/
    
   #if defined TUNER_362
   

   ScanList[4].Modulation = STTUNER_MOD_ALL;
   ScanList[4].AGC = STTUNER_AGC_ENABLE;
   ScanList[4].Band = 1;
   ScanList[4].Mode = STTUNER_MODE_8K;
   ScanList[4].Guard = STTUNER_GUARD_1_8;
   ScanList[4].Force = STTUNER_FORCE_NONE;
   ScanList[4].Hierarchy = STTUNER_HIER_PRIO_ANY;
   ScanList[4].EchoPos = 1;
   ScanList[4].FreqOff = STTUNER_OFFSET_NONE;
   ScanList[4].ChannelBW = STTUNER_CHAN_BW_8M;
   ScanList[4].FECRates = STTUNER_FEC_ALL;
   ScanList[4].Spectrum = STTUNER_INVERSION_AUTO;
   ScanList[4].SymbolRate   = 27500000;

   ScanList[5].Modulation = STTUNER_MOD_ALL;
   ScanList[5].AGC = STTUNER_AGC_ENABLE;
   ScanList[5].Band = 1;
   ScanList[5].Mode = STTUNER_MODE_8K;
   ScanList[5].Guard = STTUNER_GUARD_1_8;
   ScanList[5].Force = STTUNER_FORCE_NONE;
   ScanList[5].Hierarchy = STTUNER_HIER_NONE;
   ScanList[5].EchoPos = 1;
   ScanList[5].FreqOff = STTUNER_OFFSET_NONE;
   ScanList[5].ChannelBW = STTUNER_CHAN_BW_8M;
   ScanList[5].FECRates = STTUNER_FEC_ALL;
   ScanList[5].Spectrum = STTUNER_INVERSION_AUTO; 
   ScanList[5].SymbolRate   = 27500000; 
 #endif
  
  Scans.NumElements = 6;

    Bands.BandList = BandList;
    Scans.ScanList = ScanList;
    /*Ter_Scans.ScanList = Ter_ScanList;*/

    return(error);
}
#else
static ST_ErrorCode_t Init_scanparams(void)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Set tuner band list */
    
#if defined ( TUNER_899)|| defined (TUNER_288)
Bands.NumElements = 2; /* 372+288 or 372+899 = 3*/
#endif
#if defined (TUNER_899) && defined (TUNER_372)
Bands.NumElements = 3;
#endif
#if defined (TUNER_288) && defined(TUNER_372) 
Bands.NumElements = 3;
#endif
 
 #ifdef C_BAND_TESTING
    
       BandList[0].BandStart  = 3000000; /*L Band (LO - Transponder freq): 2200 MHz*/
        BandList[0].BandEnd   = 5150000; /*L Band (LO - Transponder freq): 900  MHz*/
      #if defined ( TUNER_899)|| defined (TUNER_288)
       BandList[0].LO            = 5150000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_C;
        #endif
        BandList[1].BandStart  = 54000;
        BandList[1].BandEnd   = 858000;
        #ifndef ST_OSLINUX 
        BandList[2].BandStart   = 10600000;
        BandList[2].BandEnd    = 11700000;
         #if defined ( TUNER_899)|| defined (TUNER_288)
        BandList[2].LO             =  9750000;
        BandList[2].DownLink   = STTUNER_DOWNLINK_Ku;
        #endif
        #endif/*oslinux*/
 #else /* Ku band testing */
        BandList[0].BandStart = 10600000;
        BandList[0].BandEnd   = 11700000;
        #if defined ( TUNER_899)|| defined (TUNER_288)
        BandList[0].LO        =  9750000;
        BandList[0].DownLink  = STTUNER_DOWNLINK_Ku;
        #endif
        BandList[1].BandStart = 54000;
        BandList[1].BandEnd   = 858000;

#endif

  #if defined ( TUNER_899)|| defined (TUNER_288)
  Scans.NumElements =2;/*372+288 or 372+899 =4*/
#endif
 #if defined ( TUNER_899)&& defined (TUNER_372)
                  Scans.NumElements =4;
  #endif                
  #if defined ( TUNER_288)&& defined (TUNER_372) 
                    Scans.NumElements =4;
  #endif     
  #ifdef C_BAND_TESTING
  
	    ScanList[0].SymbolRate   = 26670000;/*28100000;*/
	    ScanList[0].Polarization = STTUNER_PLR_HORIZONTAL;
	    ScanList[0].FECRates     = STTUNER_FEC_ALL;
	    ScanList[0].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[0].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[0].Band         = 0;
	    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
	    ScanList[0].FECType      = STTUNER_FEC_MODE_DVBS1;
	   
	    ScanList[1].SymbolRate   = 27500000;/*28100000;*/
	    ScanList[1].Polarization = STTUNER_PLR_HORIZONTAL;
	    ScanList[1].FECRates     = STTUNER_FEC_ALL;
	    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[1].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[1].Band         = 0;
	    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
	    ScanList[1].FECType      = STTUNER_FEC_MODE_DVBS1;
	   
	   #if 0
	    ScanList[1].SymbolRate   =27500000;/*28100000;*/
	    ScanList[1].Polarization = STTUNER_PLR_VERTICAL;	    
	    ScanList[1].FECRates     = STTUNER_FEC_ALL;
	    ScanList[1].Modulation   = STTUNER_MOD_QPSK;
	    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[1].LNBToneState = STTUNER_LNB_TONE_OFF;
	    ScanList[1].Band         = 0;
	    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;	    
	    ScanList[1].FECType      = STTUNER_FEC_MODE_DVBS2;
	    #endif
	   
  #endif /* C_BAND_TESTING  */
  
	    #ifdef TUNER_372
	     ScanList[2].FECRates     = STTUNER_FEC_ALL;
	    ScanList[2].Modulation   = STTUNER_MOD_ALL;
	    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[2].Band         = 1;
	    ScanList[2].Spectrum     = STTUNER_INVERSION_AUTO;
	    ScanList[2].FreqOff      = STTUNER_OFFSET_NONE;
	    ScanList[2].ChannelBW    = STTUNER_CHAN_BW_8M;
	    ScanList[2].SymbolRate   =  10762000 ;
	    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
	    
	    ScanList[3].FECRates     = STTUNER_FEC_ALL;
	    ScanList[3].Modulation   = STTUNER_MOD_ALL;
	    ScanList[3].AGC          = STTUNER_AGC_ENABLE;
	    ScanList[3].Band         = 1;
	    ScanList[3].Spectrum     = STTUNER_INVERSION_AUTO;
	    ScanList[3].FreqOff      = STTUNER_OFFSET_NONE;
	    ScanList[3].ChannelBW    = STTUNER_CHAN_BW_8M;
	    ScanList[3].SymbolRate   =  10762000 ;
	    ScanList[3].IQMode       = STTUNER_IQ_MODE_AUTO;
	             
    	
   
 	 #endif /* TUNER_372*/
 	 
 	 #if defined TUNER_362
   

   ScanList[4].Modulation = STTUNER_MOD_ALL;
   ScanList[4].AGC = STTUNER_AGC_ENABLE;
   ScanList[4].Band = 1;
   ScanList[4].Mode = STTUNER_MODE_8K;
   ScanList[4].Guard = STTUNER_GUARD_1_8;
   ScanList[4].Force = STTUNER_FORCE_NONE;
   ScanList[4].Hierarchy = STTUNER_HIER_PRIO_ANY;
   ScanList[4].EchoPos = 1;
   ScanList[4].FreqOff = STTUNER_OFFSET_NONE;
   ScanList[4].ChannelBW = STTUNER_CHAN_BW_8M;
   ScanList[4].FECRates = STTUNER_FEC_ALL;
   ScanList[4].Spectrum = STTUNER_INVERSION_AUTO;
   ScanList[4].SymbolRate   = 27500000;

   ScanList[5].Modulation = STTUNER_MOD_ALL;
   ScanList[5].AGC = STTUNER_AGC_ENABLE;
   ScanList[5].Band = 1;
   ScanList[5].Mode = STTUNER_MODE_8K;
   ScanList[5].Guard = STTUNER_GUARD_1_8;
   ScanList[5].Force = STTUNER_FORCE_NONE;
   ScanList[5].Hierarchy = STTUNER_HIER_NONE;
   ScanList[5].EchoPos = 1;
   ScanList[5].FreqOff = STTUNER_OFFSET_NONE;
   ScanList[5].ChannelBW = STTUNER_CHAN_BW_8M;
   ScanList[5].FECRates = STTUNER_FEC_ALL;
   ScanList[5].Spectrum = STTUNER_INVERSION_AUTO; 
   ScanList[5].SymbolRate   = 27500000; 
 #endif
 
 #if defined ( TUNER_288) && defined (TUNER_372)  && defined (TUNER_899) && defined (TUNER_362) 
                    Scans.NumElements =6;
  #endif 


    Bands.BandList = BandList;
    Scans.ScanList = ScanList;
    /*Ter_Scans.ScanList = Ter_ScanList;*/

    return(error);
}
#endif






static ST_ErrorCode_t
Init_evt(void)
{
   ST_ErrorCode_t       error   = ST_NO_ERROR;

   #ifdef STEVT_ENABLED
   STEVT_InitParams_t   EVTInitParams;

   /* Event handler initialization */
   EVTInitParams.MemoryPartition = system_partition;
   EVTInitParams.EventMaxNum = 12;
   EVTInitParams.ConnectMaxNum = 10;
   EVTInitParams.SubscrMaxNum = 10;
   error = STEVT_Init("stevt", &EVTInitParams);
   /*DebugError("STEVT_Init()", error);*/

   if (error != ST_NO_ERROR)
   {
      DebugMessage("Unable to initialize EVT device\n");
   }
   #endif

   /* Create global semaphore */

   int  i;
   for (i = 0; i < TUNER_MAX_USED; i++)
   {
      EventSemaphore[i] = STOS_SemaphoreCreateFifoTimeOut(NULL, 0);
   }


   return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t
Open_evt(void)
{
   ST_ErrorCode_t                   error   = ST_NO_ERROR;
   U8                               num_handles;

   #ifdef STEVT_ENABLED
   STEVT_DeviceSubscribeParams_t    SubscribeParams;
   STEVT_OpenParams_t               EVTOpenParams;
   STEVT_Handle_t                   EVTHandle[TunerInst];
   DebugPrintf(("Subscribing for events...."));    
   for (num_handles = 0; num_handles < TunerInst; num_handles++)
   {
      SubscribeParams.NotifyCallback = EVTNotifyFunction;
      SubscribeParams.SubscriberData_p = NULL;

      error = STEVT_Open("stevt", &EVTOpenParams, &EVTHandle[num_handles]);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Open()", error);

      error = STEVT_SubscribeDeviceEvent(EVTHandle[num_handles], (char*) EVT_RegistrantName[num_handles], STTUNER_EV_LOCKED, &SubscribeParams);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Subscribe()", error);

      error = STEVT_SubscribeDeviceEvent(EVTHandle[num_handles], (char*) EVT_RegistrantName[num_handles], STTUNER_EV_UNLOCKED, &SubscribeParams);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Subscribe()", error);

      error = STEVT_SubscribeDeviceEvent(EVTHandle[num_handles], (char*) EVT_RegistrantName[num_handles], STTUNER_EV_TIMEOUT, &SubscribeParams);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Subscribe()", error);

      error = STEVT_SubscribeDeviceEvent(EVTHandle[num_handles], (char*) EVT_RegistrantName[num_handles], STTUNER_EV_SCAN_FAILED, &SubscribeParams);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Subscribe()", error);

      error = STEVT_SubscribeDeviceEvent(EVTHandle[num_handles], (char*) EVT_RegistrantName[num_handles], STTUNER_EV_SIGNAL_CHANGE, &SubscribeParams);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Subscribe()", error);

      error = STEVT_SubscribeDeviceEvent(EVTHandle[num_handles], (char*) EVT_RegistrantName[num_handles], STTUNER_EV_LNB_FAILURE, &SubscribeParams);
      if (error != ST_NO_ERROR)
         DebugError("STEVT_Subscribe()", error);
   }   

   if (error == ST_NO_ERROR)
   {
      STTBX_Print(("done. %d event handles obtained\n\n\n", num_handles));
   }   
   #endif

   return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t
GlobalTerm(void)
{
   ST_ErrorCode_t       error   = ST_NO_ERROR;
   U32                  i=0;
   STI2C_TermParams_t   I2C_TermParams;
   #ifndef ST_OSLINUX
   STPIO_TermParams_t   PIO_TermParams;
   #endif

   /* Delete global semaphore */
   int  sem_id;
   for (sem_id= 0; sem_id < TunerInst; sem_id++)
   {
     STOS_SemaphoreDelete(NULL, EventSemaphore[sem_id]);
   }

   for (i = 0; i < NUM_I2C; i++)
   {
      if (I2CInitParams[i].BaseAddress != 0) /* Is in use? */
      {
         error = STI2C_Term(I2CDeviceName[i], &I2C_TermParams);
         DebugError("STI2C_Term()", error); 
         if (error != ST_NO_ERROR)
         {
            DebugPrintf(("Unable to terminate %s.\n", I2CDeviceName[i]));
         }
      }
   }

   #ifndef ST_OSLINUX
   for (i = 0; i < NUM_PIO; i++)
   {

      error = STPIO_Term(PioDeviceName[i], &PIO_TermParams);
      DebugError("STPIO_Term()", error);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Unable to terminate %s.\n", PioDeviceName[i]));
      }

   }
   #endif /*ST_OSLINUX*/
   return(error);
}


/*****************************************************************************
TunerAPI()
Description:
    This routine aims to test the robustness and validity of the API.
    These tests intentionaly exercise the drivers API with bad parameters thereby 
    validating parameter checking throughout.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

#ifndef ST_OSLINUX
static TUNER_TestResult_t
TunerAPI(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t   Result  = TEST_RESULT_ZERO;
   ST_ErrorCode_t       error;
   STTUNER_Handle_t     TUNERHandle;
   DebugMessage("This routine aims to test the robustness and validity of the API.\n");
   DebugMessage("These tests Intentionally exercise the drivers API with bad parameters.\n"); 
   /*DebugMessage("");*/
   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.0: Open without init\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to open the driver without initializing it first.   ");
   /* ------------------------------------------------------------------ */
   {
      STTUNER_OpenParams_t      OpenParams;
      Notify_ID = Params_p->Tuner;

      /* Setup open params for tuner */
      #ifndef ST_OSLINUX
      OpenParams.NotifyFunction = CallbackNotifyFunction;
      #else
      OpenParams.NotifyFunction = NULL;
      #endif



      /* Open params setup */
      error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);

      if (error != ST_ERROR_UNKNOWN_DEVICE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
         DebugError("but occured STTUNER_Open()", error);
      }
      else
      {
         TestPassed(Result);
      }
   }


   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.1: Term without init\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to term the driver without initializing it first.   ");
   /* ------------------------------------------------------------------ */
   {
      STTUNER_TermParams_t      TermParams;
      TermParams.ForceTerminate = FALSE;

      error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);

      if (error != ST_ERROR_BAD_PARAMETER)
      {
         TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER' ");
         DebugError("but occured STTUNER_Term()", error);
      }
      else
      {
         TestPassed(Result);
      }
   }


   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.2: Multiple inits\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to init the driver twice with the same name.        ");
   /* ------------------------------------------------------------------ */
   {
      error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);



      if (error != ST_NO_ERROR)
      {
         TestFailed(Result, "Expected 'ST_NO_ERROR'");
         DebugError("Error in first init itself. STTUNER_Init()", error);
      }

      if (error == ST_NO_ERROR)
      {
         error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);



         if (error != ST_ERROR_BAD_PARAMETER)
         {
            TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER'");
            DebugError("but occured STTUNER_Init()", error);
         }
         else
         {
            TestPassed(Result);
         }
      }
   }


   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.3: Open with invalid device\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to open the driver with an invalid device name.     ");
   /* ------------------------------------------------------------------ */
   {
      STTUNER_OpenParams_t      OpenParams;

      /* Setup open params for tuner */
      #ifndef ST_OSLINUX
      OpenParams.NotifyFunction = CallbackNotifyFunction;
      #else
      OpenParams.NotifyFunction = NULL;
      #endif

      OpenParams.NotifyFunction = CallbackNotifyFunction;
      /* Open params setup */
      error = STTUNER_Open("invalid", (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);



      if (error != ST_ERROR_UNKNOWN_DEVICE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
         DebugError("but occured STTUNER_Open()", error);
      }
      else
      {
         TestPassed(Result);
      }
   }


   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.4: Multiple opens\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to open the device twice with the same name.        ");
   /* ------------------------------------------------------------------ */
   {
      STTUNER_OpenParams_t      OpenParams;

      /* Setup open params for tuner */
      #ifndef ST_OSLINUX
      OpenParams.NotifyFunction = CallbackNotifyFunction;
      #else
      OpenParams.NotifyFunction = NULL;
      #endif


      /* Open params setup */
      error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);

      /* ASSUME: 0x00068000 == (STTUNER_DRIVER_BASE | 0x8000) */                   
      if (error == ST_NO_ERROR)
      {
         if ((TUNERHandle & 0xFFFFFF00) != 0x00068000)
         {
            TestFailed(Result, "Expected handle to be of the form 0x000680nn");
            DebugPrintf(("but Tuner Handle obtained is 0x%08x\n", TUNERHandle));
            DebugMessage("First open failed\n");
         }
      }

      else
      {
         TestFailed(Result, "Expected 'ST_NO_ERROR'");
         DebugError("Error in first open itself. STTUNER_Open()", error);
      }



      /* Open params setup */
      error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);

      if (error != ST_ERROR_OPEN_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_OPEN_HANDLE'");
         DebugError("but error obtained in second open is STTUNER_Open()", error);
      }
      else
      {
         TestPassed(Result);
      }
   }




   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.5: Invalid handle tests\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to call APIs with a known invalid handle.           ");
   /* ------------------------------------------------------------------ */
   {
      /* Try various API calls with an invalid handle */
      error = STTUNER_Close(STTUNER_MAX_HANDLES);


      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_Close()", error);
      }
      else
      {
         /*DebugMessage("STTUNER_Close()");*/
         TestPassed(Result);
      }
   }

   {
      error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, 0, 0, NULL, NULL);

      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_Ioctl()", error);
      }
      else
      {
         DebugMessage("STTUNER_Ioctl()               ");
         TestPassed(Result);
      }
   }

   {
      error = STTUNER_Scan(STTUNER_MAX_HANDLES, 0, 0, 0, 0);

      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_Scan()", error);
      }
      else
      {
         DebugMessage("STTUNER_Scan()                ");
         TestPassed(Result);
      }
   }

   {
      error = STTUNER_ScanContinue(STTUNER_MAX_HANDLES, 0);
      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_ScanContinue()", error);
      }
      else
      {
         DebugMessage("STTUNER_ScanContinue()        ");
         TestPassed(Result);
      }
   }

   {
      #ifdef ST_OSLINUX
      error = STTUNER_SetBandList(STTUNER_MAX_HANDLES, &Bands);
      #else
      error = STTUNER_SetBandList(STTUNER_MAX_HANDLES, NULL);
      #endif


      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_SetBandlist()", error);
      }
      else
      {
         DebugMessage("STTUNER_SetBandlist()         ");
         TestPassed(Result);
      }
   }

   {
      #ifdef ST_OSLINUX
      error = STTUNER_SetScanList(STTUNER_MAX_HANDLES, &Scans);
      #else
      error = STTUNER_SetScanList(STTUNER_MAX_HANDLES, NULL);
      #endif

      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_SetScanlist()", error);
      }
      else
      {
         DebugMessage("STTUNER_SetScanlist()         ");
         TestPassed(Result);
      }
   }

   {
      error = STTUNER_GetStatus(STTUNER_MAX_HANDLES, NULL);

      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured STTUNER_GetStatus()", error);
      }
      else
      {
         DebugMessage("STTUNER_GetStatus()           ");
         TestPassed(Result);
      }
   }

   {
      error = STTUNER_GetTunerInfo(STTUNER_MAX_HANDLES, NULL);

      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured GetTunerInfo()", error);
      }
      else
      {
         DebugMessage("STTUNER_GetTunerInfo()        ");
         TestPassed(Result);
      }
   }

   {
      #ifdef ST_OSLINUX
      error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, &Bands);
      #else
      error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, NULL);
      #endif
      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured GetBandList()", error);
      }
      else
      {
         DebugMessage("STTUNER_GetBandList()         ");
         TestPassed(Result);
      }
   }
   {


      #ifdef ST_OSLINUX
      error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, &Scans);
      #else
      error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, NULL);
      #endif

      if (error != ST_ERROR_INVALID_HANDLE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         DebugError("but occured GetScanList()", error);
      }
      else
      {
         DebugMessage("STTUNER_GetScanList()         ");
         TestPassed(Result);
      }
   }


   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.6: Get capability test\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to call this API with a known invalid device name.  ");
   /* ------------------------------------------------------------------ */
   {
      error = STTUNER_GetCapability("invalid", NULL);


      /* ST_ERROR_UNKNOWN_DEVICE if pointer checking turned off else STTUNER_ERROR_POINTER*/
      if ((error != STTUNER_ERROR_POINTER) && (error != ST_ERROR_UNKNOWN_DEVICE))
      {
         TestFailed(Result, "Expected 'STTUNER_ERROR_POINTER'");
         DebugError("but error occured STTUNER_GetCapability()", error);
      }
      else
      {
         TestPassed(Result);
      }
   }

   {
      error = STTUNER_Close(TUNERHandle);
      if (error != ST_NO_ERROR)
      {
         DebugError("STTUNER_Close()", error);
      }
   }

   {
      STTUNER_TermParams_t      TermParams;
      TermParams.ForceTerminate = FALSE;

      error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
      if (error != ST_NO_ERROR)
      {
         DebugError("STTUNER_Term()", error);
      }
   }

   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.7:Memory leak check\n", Params_p->Ref));
   DebugMessage("Purpose: Call Init, Open, SetBandlist, SetScanList, Close and Term sequentially in 10 cycles to detect any discrepancy in memory pool allocation/deallocation\n");
   /*DebugMessage("");*/

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
      STTUNER_TermParams_t          TermParams;
      STTUNER_OpenParams_t          OpenParams;
      int                           loop;
      #ifndef ST_OSLINUX
      S32                           retval, before, after;
      #endif
      BOOL                          failed  = FALSE;
      #ifndef ST_OSLINUX
      partition_status_t            status;
      partition_status_flags_t      flags=0;
      #endif

      #ifndef ST_OSLINUX
      /* setup parameters */
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      TermParams.ForceTerminate = FALSE;
      #else
      OpenParams.NotifyFunction = NULL;
      #endif

      #ifndef ST_OSLINUX
      retval = partition_status(system_partition, &status, flags);
      before = status.partition_status_free;
      DebugPrintf(("Dynamic memory free before start:    %d bytes\n", before));

      #endif
      DebugPrintf((" "));
      for (loop = 1; loop <= NUMLOOPS; loop++)
      {
         STTBX_Print(("Cycle#%d..", loop));

         /* Init */
         error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
         if (error != ST_NO_ERROR)
         {
            DebugError("STTUNER_Init()", error);
            failed = TRUE;
         }

         /* Open */
         error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);
         if (error != ST_NO_ERROR)
         {
            DebugError("STTUNER_Open()", error);
            failed = TRUE;
         }

         /* Setup scan and band lists */
         STTUNER_SetBandList(TUNERHandle, &Bands);
         if (error != ST_NO_ERROR)
         {
            DebugError("STTUNER_SetBandList()", error);
            failed = TRUE;
         }

         STTUNER_SetScanList(TUNERHandle, &Scans);           
         if (error != ST_NO_ERROR)
         {
            DebugError("STTUNER_SetScanList()", error);
            failed = TRUE;
         }

         error = STTUNER_Close(TUNERHandle);      
         if (error != ST_NO_ERROR)
         {
            DebugError("STTUNER_Close()", error);
            failed = TRUE;
         }

         error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);       
         if (error != ST_NO_ERROR)
         {
            DebugError("STTUNER_Term()", error);
            failed = TRUE;
         }

      }   /* for(loop) */
      STTBX_Print(("\n"));


      #ifndef ST_OSLINUX
      retval = partition_status(system_partition, &status, flags);
      after = status.partition_status_free;
      DebugPrintf(("Dynamic memory free after 10 cycles: %d bytes\n", after));
      #endif
      if (failed == TRUE)
      {
         TestFailed(Result, "Expected 'ST_NO_ERROR' from all functions");
      }
      #ifndef ST_OSLINUX
      if (before == after)
      {
         DebugPrintf(("No memory leaks detected     "));
         TestPassed(Result);
      }
      else
      {
         TestFailed(Result, "Memory leak detected");
         DebugPrintf(("Leak of %d bytes, or %d bytes per test\n", ABS(before - after), ABS(before - after) / NUMLOOPS));
      }
      #endif
   }

   /* ------------------------------------------------------------------ */


   return Result;
}
#endif

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
static TUNER_TestResult_t
TunerReLock(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t       Result                  = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error;
   STTUNER_Handle_t         TUNERHandle;
   STTUNER_EventType_t      Event, ExpectedEvent;
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_TermParams_t     TermParams;
   STTUNER_Scan_t           ScanParams;
   U32                      FStart, FEnd, FStepSize = 0, loop ;
   U32                      exp, div, quo, rem, exp1;
   S32                      offset_type             = 0;
   #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
   U32                      quo_snr, rem_snr;
   #endif
   BOOL                     connect                 = FALSE;
   Notify_ID = Params_p->Tuner;


   /* Setup Tuner ------------------------- */

   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
   /*DebugError("STTUNER_Init()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to initialize TUNER device");
      return (Result);
   }

   #ifndef ST_OSLINUX

   OpenParams.NotifyFunction = CallbackNotifyFunction;
   #else
   OpenParams.NotifyFunction = NULL;
   #endif

   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
   /*DebugError("STTUNER_Open()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto relocking_end;
   }
   DebugMessage("Init success.....    Open Success.....    \n");

   /* Setup scan -------------------------- */

   STTUNER_SetBandList(TUNERHandle, &Bands);

   STTUNER_SetScanList(TUNERHandle, &Scans);

   DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
   DebugMessage("Purpose: Scan to the first frequency, and monitor the events for the feed being (dis)connected\n");

   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372)
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
   }
   
    if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
       {
       	FStart = CAB_FLIMIT_LO;
        FEnd = CAB_FLIMIT_HI;
       FStepSize  = FCE_STEP;
   }
   
   /* Scan -------------------------------- */
   error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);

   /*DebugError("STTUNER_Scan()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto relocking_end;
   }

   DebugMessage("Scanning.....\n");

   /* Wait for notify function to return */

   AwaitNotifyFunction(&Event);

   DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));

   /* Check whether or not we're locked */
   if (Event != STTUNER_EV_LOCKED)
   {
      TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
      goto relocking_end;
   }

   /* Get tuner information */
   error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      #if defined ( TUNER_899)|| defined (TUNER_288)    
      DebugPrintf(("F = %u(%s) kHz Q=%u%% MOD=%s BER=%2d FEC=%s  AGC=%sdBm IQ=%d\n", TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TUNER_DemodToString(TunerInfo.ScanInfo.Modulation), TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
      #endif
   }
  else if(TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
   {
   	#if defined (TUNER_297E)
   	  	DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
                   #endif  
                }
                 
   else 
   {
      #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
      exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
      exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
      div = PowOf10(exp1);
      quo = TunerInfo.BitErrorRate / div;
      rem = TunerInfo.BitErrorRate % div;
      div /= 100;                 /* keep only two numbers after decimal point */
      if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
         rem /= div;

      if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
      {
         offset_type = 1;
      }
      else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
      {
         offset_type = -1;
      }
      else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
      {
         offset_type = 0;
      }

      exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
      exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
      div = PowOf10(exp1);
      quo = TunerInfo.BitErrorRate / div;
      rem = TunerInfo.BitErrorRate % div;
      div /= 100;                 /* keep only two numbers after decimal point */
      if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
         rem /= div;
      quo_snr = (TunerInfo.SignalQuality * 36) / 100;
      rem_snr = (TunerInfo.SignalQuality * 36) % 100;

      DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));
      #endif
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)


   {
      /* Lock to a frequency ----------------- */
      #if defined ( TUNER_899)|| defined (TUNER_)
      ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;
      ScanParams.Modulation = TunerInfo.ScanInfo.Modulation;
      ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
      ScanParams.Band = TunerInfo.ScanInfo.Band;
      ScanParams.AGC = TunerInfo.ScanInfo.AGC;
      ScanParams.SymbolRate = TunerInfo.ScanInfo.SymbolRate;
      ScanParams.IQMode = TunerInfo.ScanInfo.IQMode;
      ScanParams.ModeCode = TunerInfo.ScanInfo.ModeCode;
      #ifdef TUNER_ECHO
      ScanParams.DEMODMode = TunerInfo.ScanInfo.DEMODMode;
      #endif
      ScanParams.FECType = TunerInfo.ScanInfo.FECType;
      /*    ScanParams.PLSIndex   = TunerInfo.ScanInfo.PLSIndex;*/
      #ifdef SAT_TEST_SCR
      ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
      ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
      ScanParams.LNB_SignalRouting = TunerInfo.ScanInfo.LNB_SignalRouting;
      #endif
      #endif
   }
   else
   {
      #if defined(TUNER_370VSB) ||defined(TUNER_372)
      /* Lock to a frequency ----------------- */
      if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372) 
      {
      ScanParams.Band = 1;
      ScanParams.AGC = 1;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;   
      ScanParams.Spectrum = STTUNER_INVERSION_AUTO;
      ScanParams.FreqOff = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;

      #ifdef CHANNEL_BANDWIDTH_6
      ScanParams.ChannelBW = STTUNER_CHAN_BW_6M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_7
      ScanParams.ChannelBW = STTUNER_CHAN_BW_7M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_8
      ScanParams.ChannelBW = STTUNER_CHAN_BW_8M;
      #endif 

      #if defined(TUNER_370VSB) ||defined(TUNER_372)
      ScanParams.SymbolRate = 10762000 ;
      ScanParams.IQMode = STTUNER_IQ_MODE_AUTO;
      #endif

      /* We locked due to a scan so the relocking functionality is not enabled, to
      do this we need to do a STTUNER_SetFrequency() to the current frequency. */

      /*** Add the offset to get the exact central carrier frequency*****/
      DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n", TunerInfo.Frequency, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset));
      TunerInfo.Frequency = TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset;
      }
      #endif
   }
  #if defined TUNER_362
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
   {
      ScanParams.Band = 1;
      ScanParams.AGC = 1;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;   
      ScanParams.Spectrum = STTUNER_INVERSION_AUTO;
      ScanParams.FreqOff = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;

      #ifdef CHANNEL_BANDWIDTH_6
      ScanParams.ChannelBW = STTUNER_CHAN_BW_6M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_7
      ScanParams.ChannelBW = STTUNER_CHAN_BW_7M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_8
      ScanParams.ChannelBW = STTUNER_CHAN_BW_8M;
      #endif 

      DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n", TunerInfo.Frequency, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset));
      TunerInfo.Frequency = TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset;
   
     }   
    #endif
     #if defined TUNER_297E
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E) 
   {

    ScanParams.Modulation   = TunerInfo.ScanInfo.Modulation;
    ScanParams.Band         = TunerInfo.ScanInfo.Band;
    ScanParams.AGC          = TunerInfo.ScanInfo.AGC;
    ScanParams.SymbolRate   = TunerInfo.ScanInfo.SymbolRate;
    ScanParams.Spectrum     = TunerInfo.ScanInfo.Spectrum;
    ScanParams.J83          = STTUNER_J83_A;
}
    
  #endif
   DebugMessage("Tuning.....\n");

   error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
   /*DebugError("STTUNER_SetFrequency()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto relocking_end;
   }

   /* Wait for notify function to return */  
   AwaitNotifyFunction(&Event);

   DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
  
   /* Check whether or not we're locked */
   if (Event != STTUNER_EV_LOCKED)
   {
      TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
      goto relocking_end;
   }

   /* Get tuner information */
   error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      #if defined ( TUNER_899)|| defined (TUNER_288)  
      DebugPrintf(("F = %u(%s) kHz Q=%u%% MOD=%s BER=%2d FEC=%s  AGC=%sdBm IQ=%d\n", TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TUNER_DemodToString(TunerInfo.ScanInfo.Modulation), TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
      #endif
   }
   
   else if(TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
    {
	#if defined(TUNER_297E)
	DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
                      #endif
                }
   else
   {
      #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
      exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
      exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
      div = PowOf10(exp1);
      quo = TunerInfo.BitErrorRate / div;
      rem = TunerInfo.BitErrorRate % div;
      div /= 100;                 /* keep only two numbers after decimal point */
      if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
         rem /= div;

      if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
      {
         offset_type = 1;
      }
      else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
      {
         offset_type = -1;
      }
      else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
      {
         offset_type = 0;
      }


      exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
      exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
      div = PowOf10(exp1);
      quo = TunerInfo.BitErrorRate / div;
      rem = TunerInfo.BitErrorRate % div;
      div /= 100;                 /* keep only two numbers after decimal point */
      if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
         rem /= div;
      quo_snr = (TunerInfo.SignalQuality * 36) / 100;
      rem_snr = (TunerInfo.SignalQuality * 36) % 100;

      DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));
      #endif
   }
   

   /* detect feed status ------------------ */

   for (loop = 1; loop <= RELOCK_TEST_NUMTIMES; loop++)
   {
      DebugPrintf(("Pass %d of %d\n", loop, RELOCK_TEST_NUMTIMES));

      if (connect == FALSE)
      {

         DebugMessage(("....Please DISCONNECT feed....\n"));
         ExpectedEvent = STTUNER_EV_UNLOCKED;
         connect = TRUE;
      }
      else
      {
         DebugMessage(("....Please RECONNECT feed....\n"));
         ExpectedEvent = STTUNER_EV_LOCKED;
         connect = FALSE;
      }

      AwaitNotifyFunction(&Event);

      DebugPrintf(("Event = %s    ", TUNER_EventToString(Event)));

      if (Event != ExpectedEvent)
      {
         TestFailed(Result, "Expected:");
         DebugPrintf((" '%s' but got '%s\n'", TUNER_EventToString(ExpectedEvent), TUNER_EventToString(Event)));
         goto relocking_end;
      }

      /* report on tuner status */
      error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

      if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
      {
         #if defined ( TUNER_899)|| defined (TUNER_288) 
         DebugPrintf(("F = %u (%s) kHz Q = %u%% MOD = %s BER = %5d FEC = %s  AGC = %sdBm IQ = %d\n", TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TUNER_DemodToString(TunerInfo.ScanInfo.Modulation), TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
         #endif
      }
      
     else if(TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
      {
      	#if defined(TUNER_297E)
	DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
                      #endif
                }
      else
      {
         #if defined(TUNER_370VSB) ||defined(TUNER_372)|| defined (TUNER_362)
         exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
         if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
            rem /= div;

         if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
         {
            offset_type = 1;
         }
         else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
         {
            offset_type = -1;
         }
         else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
         {
            offset_type = 0;
         }


         exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
         if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
            rem /= div;
         quo_snr = (TunerInfo.SignalQuality * 36) / 100;
         rem_snr = (TunerInfo.SignalQuality * 36) % 100;

         DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));

         #endif
      }
      
      	














   }   /* for(loop) */


   /* check freqency will unlock ---------- */

   error = STTUNER_Unlock(TUNERHandle);
   /*DebugError("STTUNER_Unlock()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto relocking_end;
   }

   /* Wait for notify function to return */

   AwaitNotifyFunction(&Event);


   if (Event != STTUNER_EV_UNLOCKED)
   {
      TestFailed(Result, "Expected = 'STTUNER_EV_UNLOCKED'");
   }
   else
   {
      TestPassed(Result);
   }

   /* ------------------------------------- */
   relocking_end : error = STTUNER_Close(TUNERHandle);


   /*DebugError("STTUNER_Close()", error);*/

   TermParams.ForceTerminate = FALSE;
   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   /*DebugError("STTUNER_Term()", error);*/
   DebugMessage("TUNER CLOSE....and....TUNER TERM.... Successfully\n");

   return(Result);
}





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

static TUNER_TestResult_t
TunerTerm(TUNER_TestParams_t *Params_p)
{
  

   TUNER_TestResult_t       Result                  = TEST_RESULT_ZERO;
   STTUNER_Handle_t         TUNERHandle;
   ST_ErrorCode_t           error;
   STTUNER_EventType_t      Event;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_TermParams_t     TermParams;
   U32                      FStart, FEnd, FStepSize = 0;
   Notify_ID = Params_p->Tuner;
   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);

   if (error != ST_NO_ERROR)
   {
      DebugError("STTUNER_Init()", error);
      TestFailed(Result, "Unable to initialize TUNER device");
      return Result;
   }

   /*DebugPrintf(("Init success.....\n"));*/

   #ifndef ST_OSLINUX

   OpenParams.NotifyFunction = CallbackNotifyFunction;

   #else
   OpenParams.NotifyFunction = NULL;

   #endif

   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);

   if (error != ST_NO_ERROR)
   {
      DebugError("STTUNER_Open()", error);
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      return Result;
   }

   DebugPrintf((" TUNER Init success......         TUNER Open success.....\n"));


   DebugPrintf(("%d.1: Check Band and Scan lists have to be set before a scan.\n", Params_p->Ref));
   DebugMessage("Purpose: Ensure no scan if band and scan lists not set.\n");

   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372)
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
   }
   
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
       {
       	FStart = CAB_FLIMIT_LO;
        FEnd = CAB_FLIMIT_HI;
       FStepSize  = FCE_STEP;
      }

   /* attempt a scan */
   DebugMessage("Commencing scan.......            ");
   error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
   /*DebugError("STTUNER_Scan()", error);*/
   if (error == ST_ERROR_BAD_PARAMETER)
   {
      TestPassed(Result);
   }
   else
   {
      /*TestFailed(Result,"Expected = 'ST_ERROR_BAD_PARAMETER'");*/
   }

   DebugPrintf(("%d.2: Terminate during scan\n", Params_p->Ref));
   DebugMessage("Purpose: Ensure driver terminates cleanly when scanning.\n");

   /* Setup scan and band lists */
   error = STTUNER_SetBandList(TUNERHandle, &Bands);
   /*DebugError("STTUNER_SetBandList()", error);*/

   error = STTUNER_SetScanList(TUNERHandle, &Scans);
   /*DebugError("STTUNER_SetScanList()", error);*/
   /*printf("\nFStart = %d    FEnd = %d\n",FStart, FEnd);*/
   /* attempt another scan */
   DebugMessage("Commencing scan.........          ");
   error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize,100);
   /*DebugError("STTUNER_Scan()", error);*/


   if (error == ST_NO_ERROR)
   {
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }

   /* Wait for notify function to return */

   AwaitNotifyFunction(&Event);

   DebugPrintf((" Event = %s\n", TUNER_EventToString(Event)));
   /*DebugMessage("EVENT LOCKED successfully.......\n");*/
   DebugMessage("Forcing termination.......        ");
   TermParams.ForceTerminate = TRUE;
   if (error == ST_NO_ERROR)
   {
      TestPassed(Result);
      
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }

   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   if (error != ST_NO_ERROR)
   {
      DebugError("STTUNER_Term()", error);
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      return Result;
   }
   else
   {
   DebugPrintf((" TUNER Close success......         TUNER Term success.....\n"));
   /*DebugError("STTUNER_Term()", error);*/

   DebugMessage("Test Successful.......\n");
}
   task_delay(ST_GetClocksPerSecond() * 1);

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

static TUNER_TestResult_t
TunerScan(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result                          = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error;
   STTUNER_Handle_t         TUNERHandle;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_EventType_t      Event;
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_TermParams_t     TermParams;
   U32                      FStart, FEnd, FTmp, FStepSize   = 0;
   U32                      ChannelCount                    = 0;
   U8                       Rev=0;
   U32                      exp, div, quo, rem, exp1;
   S32                      offset_type                     = 0;
   #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
   U32                      quo_snr, rem_snr;
   #endif
   Notify_ID = Params_p->Tuner;

   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
   /*DebugError("STTUNER_Init()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to initialize TUNER device");
      goto simple_end;
   }

   #ifndef ST_OSLINUX
   /* Setup open params for tuner */

   OpenParams.NotifyFunction = CallbackNotifyFunction;
   #else
   OpenParams.NotifyFunction = NULL;
   #endif

   /* Open params setup */
   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
   /*DebugError("STTUNER_Open()", error);*/
   DebugPrintf(("TUNER Init success......       TUNER Open success.....\n"));
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to open tuner device");
      goto simple_end;
   }

   /* We compile for low/high band as the feed must come from a different satellite dish. */
   

   if ((TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372) || (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362))
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
       }
        if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
       {
       	FStart = CAB_FLIMIT_LO;
        FEnd = CAB_FLIMIT_HI;
       FStepSize  = FCE_STEP;  
   }
   
   #ifdef DISEQC_TEST_ON

   /*** Function TunerDiseqc() is used to test the different commands for DiSEqC module . ***
   ***To make it on make the #undef DISEQC_TEST_ON to #define DISEQC_TEST_ON in the same file***
   ***sat_test.c .  For normal Tuner Scan Test TunerDiseqc  should not be called
   ******************************/
   error = TunerDiseqc(TUNERHandle);


   /***********/

   #endif
   #if defined(TEST_DISEQC_5100) ||  defined(TEST_DISEQC_5301)
   TunerDiseqcBackend(TUNERHandle) ;
   #endif

   DebugMessage("Purpose: Check that a scan across a range works correctly.\n");

   error = STTUNER_SetBandList(TUNERHandle, &Bands);
   /*DebugError("STTUNER_SetBandList()", error);*/

   error = STTUNER_SetScanList(TUNERHandle, &Scans);
   /*DebugError("STTUNER_SetScanList()", error);*/

   
      Rev = 0;
      if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
      {
         FStart = FLIMIT_LO;
         FEnd = FLIMIT_HI;
         FStepSize=0;
      }
      else
      {
         FStart = TER_FLIMIT_LO;
         FEnd = TER_FLIMIT_HI;
      }


 while (Rev < 2)
      {
            if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
   {   
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372) 
   {   
    

      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }

         DebugPrintf(("Start Frequency: %d The End Frequency: %d Step Size: %d \n", FStart,FEnd,FStepSize));

         error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
         if (Rev == 0)
         {
            DebugPrintf(("Scanning  Tuner[%d] forward....\n", Params_p->Tuner));
         }
         else
         {
            DebugPrintf(("Scanning  Tuner[%d] reverse....\n", Params_p->Tuner));
         }
         DebugError("STTUNER_Scan()", error);


         for (; ;)
         {
            Notify_ID = Params_p->Tuner;
            AwaitNotifyFunction(&Event);

            if (Event == STTUNER_EV_LOCKED)
            {
               /* Get status information */
               error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
#if defined(TUNER_362) || defined (TUNER_372)
               if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362 || TunerInitParams[Params_p->Tuner].DemodType==STTUNER_DEMOD_STV0372)
               {
                  exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
                  exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
                  div = PowOf10(exp1);
                  quo = TunerInfo.BitErrorRate / div;
                  rem = TunerInfo.BitErrorRate % div;
                  div /= 100;                 /* keep only two numbers after decimal point */
                  if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
                     rem /= div;

                  if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
                  {
                     offset_type = 1;
                  }
                  else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
                  {
                     offset_type = -1;
                  }
                  else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
                  {
                     offset_type = 0;
                  }                     
                  exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
                  exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
                  div = PowOf10(exp1);
                  quo = TunerInfo.BitErrorRate / div;
                  rem = TunerInfo.BitErrorRate % div;
                  div /= 100;                 /* keep only two numbers after decimal point */
                  if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
                     rem /= div;
                  quo_snr = (TunerInfo.SignalQuality * 36) / 100;
                  rem_snr = (TunerInfo.SignalQuality * 36) % 100;
                  DebugPrintf(("F=%u KHz,FEC=%s,Modulation=%s,Status \'%s\',Quality=%u%%(%u.%u Db),SpectrumInversion=%u, BER(%d)=%d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));
               }
               #endif
               #ifdef STTUNER_USE_SAT
               if ((TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)||(TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288))
               {
                  DebugPrintf(("%2d F=%u(%s)kHz Q=%u%% BER= %2d FEC=%s AGC=%sdBm IQ=%d\n", 1, TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
               }
               #endif
               ChannelCount++;
               break;
            }

            else if (Event == STTUNER_EV_SCAN_FAILED   )
            {
               DebugPrintf(("Event is  %s", TUNER_EventToString(Event)));
               break;
            }

            if (Event != STTUNER_EV_LNB_FAILURE)
            {
               STTUNER_ScanContinue(TUNERHandle, 0);
            }

            if (Event == STTUNER_EV_TIMEOUT)
            {
               break;
            }
         }

         if (ChannelCount > 0)
         {
            DebugPrintf(("ChannelCount = %u       ", ChannelCount));
            TestPassed(Result);
         }
         else
         {
            TestFailed(Result, "no channels found");
         }

         /* Swap direction of scan */
         FTmp = FStart;
         FStart = FEnd;
         FEnd = FTmp;
         Rev++;

         ChannelCount = 0;
         FStepSize = 0;
      } /*end of while*/
  
 
      if (Event == STTUNER_EV_LOCKED)
      {
         error = STTUNER_Unlock(TUNERHandle);
         Notify_ID = Params_p->Tuner;

         AwaitNotifyFunction(&Event);
         if (Event == STTUNER_EV_LOCKED)
         {
            DebugPrintf(("Unlock for Tuner[%d] !!!! FAILED !!!! \n", Params_p->Tuner));
            DebugError("STTUNER_Unlock()", error);
         }
         else
         {
            DebugPrintf(("Tuner [%d] Unlocked  Result: !!!! PASS !!!!\n", Params_p->Tuner));
         }
      }
  
simple_end:   error = STTUNER_Close(TUNERHandle);
   /*DebugError("STTUNER_Close()", error);*/

   TermParams.ForceTerminate = FALSE;
   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   /*DebugError("STTUNER_Term()", error);*/
   DebugMessage("TUNER Close successful....   TUNER Term successful....\n");

   return (Result);
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

static TUNER_TestResult_t
TunerTimedScan(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result  = TEST_RESULT_ZERO;
   STTUNER_Handle_t         TUNERHandle;
   STTUNER_EventType_t      Event;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_TermParams_t     TermParams;
   STTUNER_TunerInfo_t      TunerInfo;
   ST_ErrorCode_t           error;

   Notify_ID = Params_p->Tuner;
   #ifdef ST_OS21    
   osclock_t    Time;
   #else
   clock_t      Time;
   #endif    
   U32          ChannelCount            = 0;
   U32          FStart, FEnd,FStepSize  = 0;
   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);

   /*DebugError("STTUNER_Init()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to initialize TUNER device");
      return Result;
   }
   #ifndef ST_OSLINUX

   OpenParams.NotifyFunction = CallbackNotifyFunction;
   #else
   OpenParams.NotifyFunction = NULL;
   #endif

   /* Open params setup */
   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
   /*DebugError("STTUNER_Open()", error);*/
   DebugMessage("TUNER Init Success......    TUNER Open Success....   \n");
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto timing_end;
   }

   /* Setup scan and band lists */
   STTUNER_SetBandList(TUNERHandle, &Bands);
   STTUNER_SetScanList(TUNERHandle, &Scans);

   DebugPrintf(("%d.0: Timing test\n", Params_p->Ref));
   DebugMessage("Purpose: Time to scan across the high band\n");

   ChannelCount = 0;
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
      FStepSize= 0;
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
     
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0297E)
       {
       	FStart = CAB_FLIMIT_LO;
        FEnd = CAB_FLIMIT_HI;
       FStepSize  = FCE_STEP;  
   }
   
   /* Now attempt a scan */
   DebugMessage("Commencing scan.....\n");

   Time = time_now();
   error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);

   for (; ;)
   {
      /* Wait for notify function to return */

      AwaitNotifyFunction(&Event);

      if (Event == STTUNER_EV_LOCKED)
      {

         STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

         /* Store channel information in channel table */
         ChannelTable[Params_p->Tuner][ChannelCount].F = TunerInfo.Frequency;
         memcpy(&ChannelTable[Params_p->Tuner][ChannelCount].ScanInfo, &TunerInfo.ScanInfo, sizeof(STTUNER_Scan_t));

         /* Increment channel count */
         ChannelCount++;
      }
      else if (Event == STTUNER_EV_SCAN_FAILED)
      {
         break;
      }
      else
      {
         DebugPrintf(("Unexpected event (%s)\n", TUNER_EventToString(Event)));
      }
      /* Next scan step */
      STTUNER_ScanContinue(TUNERHandle, 0);
   }

   Time = (1000 * time_minus(time_now(), Time)) / ST_GetClocksPerSecond();

  

   /* Delay for a while and ensure we do not receive */



   DebugPrintf(("Time taken = %u ms to scan band for %u channels.         ", (unsigned int)Time, ChannelTableCount[Params_p->Tuner]));
  ChannelTableCount[Params_p->Tuner] = ChannelCount;
   if (ChannelTableCount[Params_p->Tuner] > 0)
   {
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "No channels found");
   }

   timing_end : error = STTUNER_Close(TUNERHandle);


   /*DebugError("STTUNER_Close()", error);*/

   TermParams.ForceTerminate = FALSE;
   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   /*DebugError("STTUNER_Term()", error);*/
   DebugMessage("TUNER Close Success.....      TUNER Term Success.....    \n");

   return(Result);
}

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

static TUNER_TestResult_t
TunerScanExact(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result          = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error;
   STTUNER_Handle_t         TUNERHandle;
   STTUNER_EventType_t      Event;
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_TermParams_t     TermParams;
   U32                      ChannelCount    = 0;
   U32                      i;    
   S32                      offset_type     = 0;
   U32                      exp, div, quo, rem, exp1;
   Notify_ID = Params_p->Tuner;
   #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
   U32  quo_snr, rem_snr;
   #endif

   if (ChannelTableCount[Params_p->Tuner] < 1)
   {
      TestFailed(Result, "Cannot perform test with no channels found");
      return(Result);
   }

   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
   /*DebugError("STTUNER_Init()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to initialize TUNER device");
      return Result;
   }
   #ifndef ST_OSLINUX
   /* Setup open params for tuner */

   OpenParams.NotifyFunction = CallbackNotifyFunction;
   #else
   OpenParams.NotifyFunction = NULL;
   #endif

   /* Open params setup */
   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
   /*DebugError("STTUNER_Open()", error);*/
   DebugMessage("TUNER Init Success.....         TUNER Open Success.....     \n");
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to open tuner device");
      goto tse_end;
   }

   STTUNER_SetBandList(TUNERHandle, &Bands);
   STTUNER_SetScanList(TUNERHandle, &Scans);

   DebugPrintf(("Purpose: Scan through stored channel information (%d channels)\n", ChannelTableCount[Params_p->Tuner]));

   /* Now attempt a set frequency */
   for (i = 0; i < ChannelTableCount[Params_p->Tuner]; i++)
   {
      STTUNER_SetFrequency(TUNERHandle, ChannelTable[Params_p->Tuner][i].F, &ChannelTable[Params_p->Tuner][i].ScanInfo, 0);

      AwaitNotifyFunction(&Event);



      if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
      {
         #if defined ( TUNER_899)|| defined (TUNER_288) 
         if (Event == STTUNER_EV_LOCKED)
         {
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            DebugPrintf(("%02d F=%u(%s)kHz Q=%u  MOD=%s BER=%2d FEC=%s AGC=%sdBm S=%u IQ=%d\n", ChannelCount, TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TUNER_DemodToString(TunerInfo.ScanInfo.Modulation), TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.SymbolRate, TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));

            ChannelCount++;
         }
         else
         {
            DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));
            DebugPrintf(("Tried F= %u (%s)kHz FEC= %s S= %u\n", ChannelTable[Params_p->Tuner][i].F, TUNER_PolarityToString(ChannelTable[Params_p->Tuner][i].ScanInfo.Polarization), TUNER_FecToString(ChannelTable[Params_p->Tuner][i].ScanInfo.FECRates), ChannelTable[Params_p->Tuner][i].ScanInfo.SymbolRate));
         }
         #endif
      }
      else if (Event == STTUNER_EV_LOCKED)
      {
         #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
         error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

         exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
         if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
            rem /= div;

         if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
         {
            offset_type = 1;
         }
         else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
         {
            offset_type = -1;
         }
         else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
         {
            offset_type = 0;
         }

         exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
         if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
            rem /= div;
         quo_snr = (TunerInfo.SignalQuality * 36) / 100;
         rem_snr = (TunerInfo.SignalQuality * 36) % 100;

         DebugPrintf(("F = %u KHz, FEC=%s, Modulation=%s, Status \'%s\', Quality =  %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));
         #endif   

         DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
         ChannelCount++;
      }
      
      else if (Event == STTUNER_EV_LOCKED)
         {
   	#if defined (TUNER_297E)
   	error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
   	  	DebugPrintf(("%s => F = %u Hz SR %u S/s Q = %u%% BER = %d 10E-6 AGC = %sdBm\n",
                       TUNER_DemodToString(TunerInfo.ScanInfo.Modulation),
                       TunerInfo.Frequency,
                       TunerInfo.ScanInfo.SymbolRate,
                       TunerInfo.SignalQuality,
                       TunerInfo.BitErrorRate,
                       TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC)
                      ));
                   #endif  
                }     
      else
      {
         DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));
      }

   }  /* for(i) */


   DebugPrintf(("Channels = %u / %u   ", ChannelCount, ChannelTableCount[Params_p->Tuner]));
   if (ChannelCount == ChannelTableCount[Params_p->Tuner])
   {
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Incorrect number of channels");
   }

   /* If we're currently locked then we should unlock and then catch the
    event to ensure we leave things in a tidy state. */

   if (Event == STTUNER_EV_LOCKED)
   {
      /* Ensures we unlock the tuner */
      STTUNER_Unlock(TUNERHandle);

      AwaitNotifyFunction(&Event);
   }

   tse_end : error = STTUNER_Close(TUNERHandle);


   /*DebugError("STTUNER_Close()", error);*/
   TermParams.ForceTerminate = FALSE;
   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   /*DebugError("STTUNER_Term()", error);*/
   DebugMessage("TUNER Close Success.....      TUNER Term success.....   \n");

   return(Result);
}

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

static TUNER_TestResult_t
TunerTimedScanExact(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result          = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error;
   STTUNER_Handle_t         TUNERHandle;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_TermParams_t     TermParams;
   STTUNER_EventType_t      Event;
   U32                      ChannelCount    = 0;
   #ifdef ST_OS21    
   osclock_t                StartClocks, TotalClocks;
   #else
   clock_t                  StartClocks, TotalClocks;
   #endif    
   U32                      i;
   


   if (ChannelTableCount[Params_p->Tuner] < 1)
   {
      TestFailed(Result, "Cannot perform test with no channels found");
      return(Result);
   }

   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
   /*DebugError("STTUNER_Init()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to initialize TUNER device");
      return Result;
   }
   #ifndef ST_OSLINUX
   /* Setup open params for tuner */
  Notify_ID = Params_p->Tuner;
   OpenParams.NotifyFunction = CallbackNotifyFunction;

   #else
   OpenParams.NotifyFunction = NULL;
   #endif

   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], &OpenParams, &TUNERHandle);
   /*DebugError("STTUNER_Open()", error);*/
   DebugMessage("TUNER Init Success.....     TUNER Open Success.....    \n");
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to open tuner device");
      goto ttse_end;
   }

   STTUNER_SetBandList(TUNERHandle, &Bands);
   STTUNER_SetScanList(TUNERHandle, &Scans);

   DebugMessage("Purpose: Scan through stored channel information.\n");
   DebugMessage("Starting timer...\n");

   StartClocks = time_now();

   /* Now attempt a set frequency */
   for (i = 0; i < ChannelTableCount[Params_p->Tuner]; i++)
   {
      STTUNER_SetFrequency(TUNERHandle, ChannelTable[Params_p->Tuner][i].F, &ChannelTable[Params_p->Tuner][i].ScanInfo, 0);

      Notify_ID = Params_p->Tuner;
      AwaitNotifyFunction(&Event);

      if (Event == STTUNER_EV_LOCKED)
      {
         ChannelCount++;
      }
      else
      {
         DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));
      }
   }


   TotalClocks = (1000 * time_minus(time_now(), StartClocks)) / ST_GetClocksPerSecond();

   DebugMessage("Stopped timer...\n");
   DebugPrintf(("Channels = %u / %u\n", ChannelCount, ChannelTableCount[Params_p->Tuner]));
   DebugPrintf(("Time taken = %u ms.        ",( unsigned int)TotalClocks));


   if (ChannelCount == ChannelTableCount[Params_p->Tuner])
   {
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Incorrect number of channels");
   }

   /* If we're currently locked then we should unlock and then catch the
    event to ensure we leave things in a tidy state. */
   if (Event == STTUNER_EV_LOCKED)
   {
      /* Ensures we unlock the tuner */
      STTUNER_Unlock(TUNERHandle);

      AwaitNotifyFunction(&Event);
   }



   ttse_end : error = STTUNER_Close(TUNERHandle);
   /*DebugError("STTUNER_Close()", error);*/

   TermParams.ForceTerminate = FALSE;
   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   /*DebugError("STTUNER_Term()", error);*/
   DebugMessage("TUNER Close Success......   TUNER Term Success.....    \n");
   return(Result);
}

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
static TUNER_TestResult_t
TunerTracking(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result                  = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error;
   STTUNER_Handle_t         TUNERHandle;
   STTUNER_EventType_t      Event;
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_TermParams_t     TermParams;
   STTUNER_Scan_t           ScanParams;
   U32                      FStart, FEnd,FStepSize  = 0;
   U32                      rem, div, exp, quo, exp1, i;
   #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
   U32                      quo_snr, rem_snr;
   #endif
   Notify_ID = Params_p->Tuner;

   S32  offset_type = 0; 

   error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
   /*DebugError("STTUNER_Init()", error);*/
   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Unable to initialize TUNER device");
      return Result;
   }
   #ifndef ST_OSLINUX
      OpenParams.NotifyFunction = CallbackNotifyFunction;

   #else
   OpenParams.NotifyFunction = NULL;
   #endif

   error = STTUNER_Open(TunerDeviceName[Params_p->Tuner], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle);
   /*DebugError("STTUNER_Open()", error);*/
   DebugMessage("TUNER Init Success.....     STTUNER Open Success.....    \n");

   if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto tracking_end;
   }

   /* Setup scan and band lists */
   STTUNER_SetBandList(TUNERHandle, &Bands);
   STTUNER_SetScanList(TUNERHandle, &Scans);

   DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
   DebugMessage("Purpose: Scan to the first frequency lock and let tracking run for a reasonable period.\n");

    if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0372) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
     
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }

   /* Now attempt a scan */
   error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
   /*DebugError("STTUNER_Scan()", error);*/

   if (error == ST_NO_ERROR)
   {
      DebugMessage("Scanning.....\n");

      /* Wait for notify function to return */

      AwaitNotifyFunction(&Event);
      /* Check whether or not we're locked */

      if (Event == STTUNER_EV_LOCKED)
      {
         /* Get status information */
         error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
         if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
         {
            #if defined ( TUNER_899)|| defined (TUNER_288)
            DebugPrintf(("F = %u(%s)kHz Q=%u  MOD=%s BER=%2d FEC=%s AGC=%sdBm IQ=%d\n", TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TUNER_DemodToString(TunerInfo.ScanInfo.Modulation), TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));

            #endif
         }
         else
         {
            #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
            exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            DebugPrintf(("before div\n"));
            div /= 100;                 /* keep only two numbers after decimal point */
            if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
               rem /= div;
            quo_snr = (TunerInfo.SignalQuality * 36) / 100;
            DebugPrintf(("before rem_snr assign\n"));
            rem_snr = (TunerInfo.SignalQuality * 36) % 100;
DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));
DebugPrintf(("F= %uKHz, FEC= %s, Modulation= %s, Status \'%s\', Quality=%u%%(%u.%u Db),SpectrumInversion= %u, BER(%d)= %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));
            #endif   
         }
      }
      else
      {
         DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));
         TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
         goto tracking_end;
      }
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto tracking_end;
   }

   if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
   {
      /* Lock to a frequency ----------------- */
      #if defined ( TUNER_899)|| defined (TUNER_288)
      ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;
      ScanParams.Modulation = TunerInfo.ScanInfo.Modulation;
      ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
      ScanParams.Band = TunerInfo.ScanInfo.Band;
      ScanParams.AGC = TunerInfo.ScanInfo.AGC;
      ScanParams.SymbolRate = TunerInfo.ScanInfo.SymbolRate;
      ScanParams.IQMode = TunerInfo.ScanInfo.IQMode;

      ScanParams.FECType = TunerInfo.ScanInfo.FECType;
      #ifdef SAT_TEST_SCR
      ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
      ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
      ScanParams.LNB_SignalRouting = TunerInfo.ScanInfo.LNB_SignalRouting;
      ScanParams.PLSIndex = TunerInfo.ScanInfo.PLSIndex;
      #endif

      DebugMessage("Tuning.....\n");

      /* We locked due to a scan so the relocking functionality is not enabled, to
      do this we need to do a STTUNER_SetFrequency() to the current frequency. */
      error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
      /*DebugError("STTUNER_SetFrequency()", error);*/
      #endif
   }
   else
   {
      #if defined(TUNER_370VSB) ||defined(TUNER_372)

      /* Lock to a frequency ----------------- */

      ScanParams.Band = 1;
      ScanParams.AGC = 1;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;

      #if defined(TUNER_370VSB) && defined(TUNER_372)

      ScanParams.Mode = TunerInfo.ScanInfo.Mode;
      ScanParams.Guard = TunerInfo.ScanInfo.Guard;
      ScanParams.Force = STTUNER_FORCE_NONE;
      ScanParams.Hierarchy = STTUNER_HIER_LOW_PRIO/*TunerInfo.ScanInfo.Hierarchy*/;
      #endif
      ScanParams.Spectrum = STTUNER_INVERSION_AUTO;
      ScanParams.FreqOff = STTUNER_OFFSET_NONE;/*TunerInfo.ScanInfo.FreqOff;*/
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;
      #ifdef CHANNEL_BANDWIDTH_6
      ScanParams.ChannelBW = STTUNER_CHAN_BW_6M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_7
      ScanParams.ChannelBW = STTUNER_CHAN_BW_7M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_8
      ScanParams.ChannelBW = STTUNER_CHAN_BW_8M;
      #endif 

      #if defined(TUNER_370VSB) ||defined(TUNER_372)
      ScanParams.SymbolRate = 10762000 ;
      ScanParams.IQMode = STTUNER_INVERSION_AUTO;
      #endif
      #endif
     
      if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STV0362) 
          #if defined TUNER_362
       {
      ScanParams.Band = 1;
      ScanParams.AGC = 1;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;   
      ScanParams.Spectrum = STTUNER_INVERSION_AUTO;
      ScanParams.FreqOff = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;

      #ifdef CHANNEL_BANDWIDTH_6
      ScanParams.ChannelBW = STTUNER_CHAN_BW_6M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_7
      ScanParams.ChannelBW = STTUNER_CHAN_BW_7M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_8
      ScanParams.ChannelBW = STTUNER_CHAN_BW_8M;
      #endif 
        }   
#endif
      DebugMessage("Tuning.....");

      /* We locked due to a scan so the relocking functionality is not enabled, to
      do this we need to do a STTUNER_SetFrequency() to the current frequency. */

      /*** Add the offset to get the exact central carrier frequency*****/
      DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n", TunerInfo.Frequency, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset));
      TunerInfo.Frequency = TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset;
      error = STTUNER_SetFrequency(TUNERHandle, TunerInfo.Frequency, &ScanParams, 0);
      DebugError("STTUNER_SetFrequency()", error);
   }
   
     if (error != ST_NO_ERROR)
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      goto tracking_end;
   }

   /* Wait for notify function to return */

   AwaitNotifyFunction(&Event);

   DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

   /* Check whether or not we're locked */
   if (Event != STTUNER_EV_LOCKED)
   {
      /*TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");*/
      goto tracking_end;
   }

   if (Event == STTUNER_EV_LOCKED)
   {
      /* Begin polling driver for tracking tests... */
      DebugMessage("Polling the tuner status -- ensure lock is kept...\n");

      for (i = 0; i < TUNER_TRACKING; i++)
      {
         task_delay(ST_GetClocksPerSecond());


         error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
         /*DebugError("STTUNER_GetTunerInfo()", error);*/

         if (error != ST_NO_ERROR)
         {
            TestFailed(Result, "Expected = 'ST_NO_ERROR'");
            /*goto tracking_end;*/
         }
         
         if (TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[Params_p->Tuner].DemodType == STTUNER_DEMOD_STB0899)
         {
            #if defined ( TUNER_899)|| defined (TUNER_288)  
            /*           DebugPrintf(("TunerStatus: %s\n", TUNER_StatusToString(TunerInfo.Status)));*/
            DebugPrintf(("F =%d(%s)kHz, Q=%u, MOD=%s, BER=%2d, FEC=%s,  AGC=%sdBm, IQ=%d\n", TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TUNER_DemodToString(TunerInfo.ScanInfo.Modulation), TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
            #endif
         }
         else
         {
            #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362)
            exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
            if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
               rem /= div;

            if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
            {
               offset_type = 1;
            }
            else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
            {
               offset_type = -1;
            }
            else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
            {
               offset_type = 0;
            }

            exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
            if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
               rem /= div;
            quo_snr = (TunerInfo.SignalQuality * 36) / 100;
            rem_snr = (TunerInfo.SignalQuality * 36) % 100;

            DebugPrintf(("F= %uKHz, FEC= %s, Modulation= %s, Status \'%s\', Quality =%u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));

            #endif                           
         }
         if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
         {
            TestFailed(Result, "Expected = 'STTUNER_STATUS_LOCKED'");
            /*goto tracking_end;*/
         }
      }
   }
   else
   {
      DebugPrintf(("Event = %s\n", TUNER_EventToString(Event)));
   }

   /* Abort the current scan */
   error = STTUNER_Unlock(TUNERHandle);
   /*DebugError("STTUNER_Unlock()", error);*/

   if (error != ST_NO_ERROR)
   {
      /*TestFailed(Result,"Expected = 'ST_NO_ERROR'");*/
      goto tracking_end;
   }

   /* Wait for notify function to return */

   AwaitNotifyFunction(&Event);

   if (Event != STTUNER_EV_UNLOCKED)
   {
      TestFailed(Result, "Expected = 'STTUNER_EV_UNLOCKED'");
   }
   else
   {
      DebugMessage("Tests Results in to Success             ");
      TestPassed(Result);
   }

   tracking_end : 
   	
   error = STTUNER_Close(TUNERHandle);


DebugError("STTUNER_Close()", error);

   TermParams.ForceTerminate = FALSE;
   error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
   /*DebugError("STTUNER_Term()", error);*/
   DebugMessage("TUNER closed Success.....    TUNER Term Success.....    \n");
   return(Result);
}

#ifndef ST_OSLINUX
/*****************************************************************************
TunerMultiInstanceAPI()
Description:
    This routine aims to test the robustness and validity of the APIs incase of MultiInstance Tuner.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/
static TUNER_TestResult_t
TunerMultiInstanceAPI(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t   Result  = TEST_RESULT_ZERO;
   ST_ErrorCode_t       error   = ST_NO_ERROR;
   STTUNER_Handle_t     TUNERHandle[TunerInst];
   U8                   i;

   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.0:Open without init\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to open the driver without initializing it first.\n");
   /* ------------------------------------------------------------------ */
   for (i = 0; i < TunerInst; i++)
   {
      STTUNER_OpenParams_t      OpenParams;

      /* Setup open params for tuner */
      Notify_ID = i;
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      /* Open params setup */
      error = STTUNER_Open(TunerDeviceName[i], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle[i]);

      if (error != ST_ERROR_UNKNOWN_DEVICE)
      {
         TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
         DebugPrintf(("%s", TunerType[i]));
         DebugError("STTUNER_Open()", error);
      }
      else
      {
         DebugPrintf(("%s", TunerType[i]));
         TestPassed(Result);
      }
   }


   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.1: Term without init\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to term the driver without initializing it first.\n");
   /* ------------------------------------------------------------------ */
   for (i = 0; i < TunerInst; i++)
   {
      STTUNER_TermParams_t      TermParams;
      TermParams.ForceTerminate = FALSE;

      error = STTUNER_Term(TunerDeviceName[i], &TermParams);

      if (error != ST_ERROR_BAD_PARAMETER)
      {
         DebugPrintf(("%s", TunerType[i]));
         TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER' ");
         DebugError("STTUNER_Term()", error);
      }
      else
      {
         DebugPrintf(("%s", TunerType[i]));
         TestPassed(Result);
      }
   }

   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.2: Multiple inits\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to init the driver twice with the same name.\n");
   /* ------------------------------------------------------------------ */
   for (i = 0; i < TunerInst; i++)
   {
      STTUNER_TermParams_t      TermParams;
      TermParams.ForceTerminate = FALSE;
      error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
      if (error != ST_NO_ERROR)
      {
         TestFailed(Result, "Expected 'ST_NO_ERROR'");
         DebugError("Error in first init itself. STTUNER_Init()", error);
      }
      if (error == ST_NO_ERROR)
      {
         error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
         if (error != ST_ERROR_BAD_PARAMETER)
         {
            DebugPrintf(("%s", TunerType[i]));
         DebugError("STTUNER_Init(%d)", error);
         TestFailed(Result, "Expected 'ST_ERROR_BAD_PARAMETER'");
         }
         else
         {
           DebugPrintf(("%s", TunerType[i]));
         TestPassed(Result);
         }
      }

   }

   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.3: Open with invalid device\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to open the driver with an invalid device name.\n");
   /* ------------------------------------------------------------------ */

   for (i = 0; i < TunerInst; i++)
   {
      STTUNER_OpenParams_t      OpenParams;

      /* Setup open params for tuner */  
      Notify_ID = i;  
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      /* Open params setup */
      error = STTUNER_Open("invalid", (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle[i]);
      if (error != ST_ERROR_UNKNOWN_DEVICE)
      {
         DebugPrintf(("%s", TunerType[i]));
         DebugError("STTUNER_Open()", error);
         TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
      }
      else
      {
         DebugPrintf(("%s", TunerType[i]));



         TestPassed(Result);
      }
   }
   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.4: Multiple opens\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to open the device twice with the same name\n");
   /* ------------------------------------------------------------------ */
   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;

      STTUNER_OpenParams_t      OpenParams;
      /* Setup open params for tuner */
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      /* Open params setup */
      error = STTUNER_Open(TunerDeviceName[i], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle[i]);

      /* ASSUME: 0x00068000 == (STTUNER_DRIVER_BASE | 0x8000) */
      if ((TUNERHandle[i] & 0xFFFFFF00) != 0x00068000)

      {
         TestFailed(Result, "Expected handle to be of the form 0x000680nn\n");
      }

      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("%s", TunerType[i]));
         TestFailed(Result, "Expected 'ST_NO_ERROR'");
      }
      else
      {
         /* Open params setup */
         error = STTUNER_Open(TunerDeviceName[i],
                                     (const STTUNER_OpenParams_t *)&OpenParams,
                                     &TUNERHandle[i]);

         if (error != ST_ERROR_OPEN_HANDLE)
         {
            DebugPrintf(("%s", TunerType[i]));

            /*DebugError("STTUNER_Open()", error);*/
            /*DebugPrintf(("Tuner handle = 0x%08x\n", TUNERHandle[i]))*/
            TestFailed(Result, "Expected 'ST_ERROR_UNKNOWN_DEVICE'");
         }
         else
         {
            DebugPrintf(("%s", TunerType[i]));
            TestPassed(Result);
         }
      }
   }

   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.5: Invalid handle tests\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to call API with a known invalid handle.\n");
   /* ------------------------------------------------------------------ */

   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;

      /* Try various API calls with an invalid handle */
      error = STTUNER_Close(STTUNER_MAX_HANDLES);
      /*DebugError("STTUNER_Close(0)", error);*/
      {
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            /*DebugError("STTUNER_Open()", error);*/
            DebugPrintf(("Tuner [%d] STTUNER_Open set            ", i));
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_Open set          ", i));
            TestPassed(Result);
         }
      }
      {
         error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, 0, 0, NULL, NULL);

         if (error != ST_ERROR_INVALID_HANDLE)
         {

            /*DebugError("STTUNER_Ioctl", error);*/
            DebugPrintf(("Tuner [%d] STTUNER_Ioctl set ", i));
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_Ioctl set         ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_Scan(STTUNER_MAX_HANDLES, 0, 0, 0, 0);
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            DebugPrintf(("Tuner [%d] ", i));
            /*DebugError("STTUNER_Scan", error);*/
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_Scan              ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_ScanContinue(STTUNER_MAX_HANDLES, 0);
         /*DebugError("STTUNER_ScanContinue", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_ScanContinue set  ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_SetBandList(STTUNER_MAX_HANDLES, NULL);
         /*DebugError("STTUNER_SetBandList", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_SetBandList set   ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_SetScanList(STTUNER_MAX_HANDLES, NULL);
         /*DebugError("STTUNER_SetScanList", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d]STTUNER_SetScanList Set    ", i));
            TestPassed(Result);
         }
      }


      {
         error = STTUNER_GetStatus(STTUNER_MAX_HANDLES, NULL);
         /*DebugError("STTUNER_GetStatus", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            /*TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");*/
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_Get Status set    ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_GetTunerInfo(STTUNER_MAX_HANDLES, NULL);
         /*DebugError("STTUNER_GetTunerInfo", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            /*TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");*/
         }
         else
         {
            DebugPrintf(("Tuner [%d] STTUNER_GetTunerInfo set  ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, NULL);
         /*DebugError("STTUNER_GetBandList", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf(("Tuner [%d]STTUNER_GetBandList set    ", i));
            TestPassed(Result);
         }
      }

      {
         error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, NULL);
         /*DebugError("STTUNER_GetScanList", error);*/
         if (error != ST_ERROR_INVALID_HANDLE)
         {
            TestFailed(Result, "Expected 'ST_ERROR_INVALID_HANDLE'");
         }
         else
         {
            DebugPrintf((" For Tuner [%d] all tests are ok     ", i));
            TestPassed(Result);
            DebugMessage("\n");
         }
         error = STTUNER_Close(TUNERHandle[i]);
      }



   }    
   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.6: Get capability test\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt to call this API with a known invalid device name.\n");
   /* ------------------------------------------------------------------ */

   for (i = 0; i < TunerInst; i++)
   {

      Notify_ID = i;

      error = STTUNER_GetCapability("invalid", NULL);

      /*DebugError("STTUNER_GetCapability()", error);*/
      /* ST_ERROR_UNKNOWN_DEVICE if pointer checking turned off else STTUNER_ERROR_POINTER*/
      if ((error != STTUNER_ERROR_POINTER) && (error != ST_ERROR_UNKNOWN_DEVICE))
      {
         TestFailed(Result, "Expected 'STTUNER_ERROR_POINTER'");
      }
      else
      {
         DebugPrintf(("Tuner [%d] ", i));
         TestPassed(Result);
      }

      {
         error = STTUNER_Close(TUNERHandle[i]);
         /*DebugError("STTUNER_Close(%d)", error);*/
      }

      {
         STTUNER_TermParams_t       TermParams;
         TermParams.ForceTerminate = FALSE;

         error = STTUNER_Term(TunerDeviceName[i], &TermParams);
         /*DebugError("STTUNER_Term(%d)", error);*/
         DebugMessage("Tuner Close Success.....   Tuner Term Success.....    \n");

      }
   }                        
   /* ------------------------------------------------------------------ */
   DebugPrintf(("%d.7: Tuners init->open->set bands->close->term tuner 10 times\n", Params_p->Ref));
   DebugMessage("Purpose: Attempt a normal operation multiple times\n");
   /* ------------------------------------------------------------------ */

   #ifdef ST_OS21       
   #include <os21/partition.h>
   #else
   #include <partition.h>
   #endif           
   #define ABS(X) ((X)<0 ? (-X) : (X))
   #define NUMLOOPS 10

   int                          loop;
   S32                          retval, before, after;
   BOOL                         failed  = FALSE;
   partition_status_t           status;
   partition_status_flags_t     flags=0;

   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;

      STTUNER_OpenParams_t      OpenParams;
      STTUNER_TermParams_t      TermParams;
      /* setup parameters */
      #ifndef ST_OSLINUX
      OpenParams.NotifyFunction = CallbackNotifyFunction;
      TermParams.ForceTerminate = FALSE;
      #else
      OpenParams.NotifyFunction = NULL;
      #endif
      #ifndef ST_OSLINUX
      retval = partition_status(system_partition, &status, flags);
      before = status.partition_status_free;
      DebugPrintf(("For Tuner [%d]---\n", i));
      DebugPrintf(("Dynamic memory free before test: %d bytes\n", before));
      #endif
      /*DebugPrintf((""));*/
      for (loop = 1; loop <= NUMLOOPS; loop++)
      {
         STTBX_Print((" cycle #%d ", loop));

         /* Init */
         error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);

         /*DebugError("STTUNER_Init()", error);*/
         if (error != ST_NO_ERROR)
         {
            /*DebugError("STTUNER_Init()", error);*/
            failed = TRUE;
         }

         /* Open */
         error = STTUNER_Open(TunerDeviceName[i], (const STTUNER_OpenParams_t *) &OpenParams, &TUNERHandle[i]);
         /*DebugError("STTUNER_Open", error);*/
         if (error != ST_NO_ERROR)
         {
            /*DebugError("STTUNER_open()", error);*/
            failed = TRUE;
         }
         /* Setup scan and band lists */

         STTUNER_SetBandList(TUNERHandle[i], &Bands);
         /*DebugError("STTUNER_SetBandList", error);*/
         if (error != ST_NO_ERROR)
            failed = TRUE;

         STTUNER_SetScanList(TUNERHandle[i], &Scans);
         /*DebugError("STTUNER_SetScanList", error);*/
         if (error != ST_NO_ERROR)
            failed = TRUE;

         error = STTUNER_Close(TUNERHandle[i]);
         /*DebugError("STTUNER_Close", error);*/
         if (error != ST_NO_ERROR)
            failed = TRUE;

         error = STTUNER_Term(TunerDeviceName[i], &TermParams);
         /*DebugError("STTUNER_Term", error);*/
         if (error != ST_NO_ERROR)
            failed = TRUE;
      }   /* for(loop) */
      STTBX_Print(("\n"));
      retval = partition_status(system_partition, &status, flags);
      after = status.partition_status_free;
      DebugPrintf(("Dynamic Memory free After test: %d bytes\n", after));

      if (failed == FALSE)
      {
         /*TestPassed(Result);*/
      }
      else
      {
         /*DebugPrintf(("Tuner [%d] ",i));*/
         TestFailed(Result, "Expected 'ST_NO_ERROR' from all functions");
      }

      if (before == after)
      {
         /*DebugPrintf(("Tuner [%d] ",i));*/
         DebugPrintf(("  No memory leaks detected    "));
         TestPassed(Result);
      }
      else
      {
         DebugPrintf(("Tuner [%d] ", i));
         TestFailed(Result, "Memory leak detected");
         DebugPrintf(("Leak of %d bytes, or %d bytes per test\n", ABS(before - after), ABS(before - after) / NUMLOOPS));
      }
   }

   /*#endif*/
   if (error == ST_NO_ERROR)
   {DebugPrintf(("  All Test Results in Success..    "));
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }
   /*----------------------------------*/
   return (Result); 
   /*----------------------------------*/
} /* TunerMultiInstanceAPI() */
#endif

/*#ifndef ST_OSLINUX*/
/*----------------------------------------------------------------------------
TunerMultiInstanceScanSimultaneous()

Description:
    Setup three tuners ie initialize, open and scan at the same time

Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.

Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.

See Also:
    Nothing.
----------------------------------------------------------------------------*/

static TUNER_TestResult_t
TunerMultiInstanceScanSimultaneous(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result          = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error           = ST_NO_ERROR;
   STTUNER_Handle_t         TUNERHandle[TunerInst];
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_EventType_t      Event[TunerInst];
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_TermParams_t     TermParams[TunerInst];
   U32                       Rev             = 0,i ;
   U32                      FStepSize       = 0;
   U32                      FStart, FEnd, FTmp;
   U32                      exp, div, quo, rem, exp1;
   #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined(TUNER_362)
   U32                      quo_snr, rem_snr;
   #endif


   U32                      ChannelCount    = 0;
   S32                      offset_type     = 0;
   for (i = 0; i < TunerInst; i++)
   {
      TermParams[i].ForceTerminate = FALSE;
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Init failed\n", i));
         DebugError("STTUNER_Init()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Init success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      error = STTUNER_Open(TunerDeviceName[i], &OpenParams, &TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Open failed\n", i));
         DebugError("STTUNER_Open()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Open success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetBandList(TUNERHandle[i], &Bands);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Bandlist failed\n", i));
         DebugError("STTUNER_SetBandList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Bandlist Set\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetScanList(TUNERHandle[i], &Scans);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Scanlist failed\n", i));
         DebugError("STTUNER_SetScanList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Scanlist Set\n", i));
      }
   }            
   for (i = 0; i < TunerInst; i++)
   {
      Rev = 0;
      if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)
      {
         FStart = FLIMIT_LO;
         FEnd = FLIMIT_HI;
         FStepSize=0;
      }
      else
      {
         FStart = TER_FLIMIT_LO;
         FEnd = TER_FLIMIT_HI;
      }


 while (Rev < 2)
      {
            if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362) 
   {   
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372) 
   {   
    

      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }

         DebugPrintf(("Start Frequency: %d The End Frequency: %d Step Size: %d \n", FStart,FEnd,FStepSize));

         error = STTUNER_Scan(TUNERHandle[i], FStart, FEnd, FStepSize, 0);
         if (Rev == 0)
         {
            DebugPrintf(("Scanning  Tuner[%d] forward....\n", i));
         }
         else
         {
            DebugPrintf(("Scanning  Tuner[%d] reverse....\n", i));
         }
         DebugError("STTUNER_Scan()", error);


         for (; ;)
         {
            Notify_ID = i;
            AwaitNotifyFunction(&Event[i]);

            if (Event[i] == STTUNER_EV_LOCKED)
            {
               /* Get status information */
               error = STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);
#if defined(TUNER_362) || defined (TUNER_372)
               if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362 || TunerInitParams[i].DemodType==STTUNER_DEMOD_STV0372)
               {
                  exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
                  exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
                  div = PowOf10(exp1);
                  quo = TunerInfo.BitErrorRate / div;
                  rem = TunerInfo.BitErrorRate % div;
                  div /= 100;                 /* keep only two numbers after decimal point */
                  if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
                     rem /= div;

                  if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
                  {
                     offset_type = 1;
                  }
                  else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
                  {
                     offset_type = -1;
                  }
                  else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
                  {
                     offset_type = 0;
                  }                     
                  exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
                  exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
                  div = PowOf10(exp1);
                  quo = TunerInfo.BitErrorRate / div;
                  rem = TunerInfo.BitErrorRate % div;
                  div /= 100;                 /* keep only two numbers after decimal point */
                  if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
                     rem /= div;
                  quo_snr = (TunerInfo.SignalQuality * 36) / 100;
                  rem_snr = (TunerInfo.SignalQuality * 36) % 100;
                  DebugPrintf(("F=%u KHz,FEC=%s,Modulation=%s,Status \'%s\',Quality=%u%%(%u.%u Db),SpectrumInversion=%u, BER(%d)=%d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, quo_snr, rem_snr, TunerInfo.ScanInfo.IQMode, TunerInfo.BitErrorRate, quo, rem, exp));
               }
               #endif
               #ifdef STTUNER_USE_SAT
               if ((TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)||(TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288))
               {
                  DebugPrintf(("%2d F=%u(%s)kHz Q=%u%% BER= %2d FEC=%s AGC=%sdBm IQ=%d\n", 1, TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
               }
               #endif
               ChannelCount++;
               break;
            }

            else if (Event[i] == STTUNER_EV_SCAN_FAILED   )
            {
               DebugPrintf(("Event is  %s", TUNER_EventToString(Event[i])));
               break;
            }

            if (Event[i] != STTUNER_EV_LNB_FAILURE)
            {
               STTUNER_ScanContinue(TUNERHandle[i], 0);
            }

            if (Event[i] == STTUNER_EV_TIMEOUT)
            {
               break;
            }
         }

         if (ChannelCount > 0)
         {
            DebugPrintf(("ChannelCount = %u       ", ChannelCount));
            TestPassed(Result);
         }
         else
         {
            TestFailed(Result, "no channels found");
         }

         /* Swap direction of scan */
         FTmp = FStart;
         FStart = FEnd;
         FEnd = FTmp;
         Rev++;

         ChannelCount = 0;
         FStepSize = 0;
      } /*end of while*/
   } /* for */

   for (i = 0; i < TunerInst; i++)
   {
      if (Event[i] == STTUNER_EV_LOCKED)
      {
         error = STTUNER_Unlock(TUNERHandle[i]);
         Notify_ID = i;

         AwaitNotifyFunction(&Event[i]);
         if (Event[i] == STTUNER_EV_LOCKED)
         {
            DebugPrintf(("Unlock for Tuner[%d] !!!! FAILED !!!! \n", i));
            DebugError("STTUNER_Unlock()", error);
         }
         else
         {
            DebugPrintf(("Tuner [%d] Unlocked  Result: !!!! PASS !!!!\n", i));
         }
      }
   }                    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Close(TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Close failed\n", i));
         DebugError("STTUNER_Close()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Close success\n", i));
      }
   }        
   for (i = 0; i < TunerInst; i++)

   {
      error = STTUNER_Term(TunerDeviceName[i], &TermParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Term failed\n", i));
         DebugError("STTUNER_Term()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Term success\n", i));
      }
   }
   if (error == ST_NO_ERROR)
   {DebugMessage("Tests Results in to Success        ");
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }

   return (Result);
}
/* End of TunerMultiInstanceScanSimultaneous function */
/*#endif*/

/*#ifndef ST_OSLINUX*/

/*----------------------------------------------------------------------------
TunerMultiInstanceScanStabilityTest()

Description:
    Tests the robustness of the software

Parameters:
    Params_p,   tuner test parameters -- as defined in the test header file.

Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.

See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t
TunerMultiInstanceScanStabilityTest(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result                      = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error                       = ST_NO_ERROR;
   U32                       i                           = 0;
   STTUNER_Handle_t         TUNERHandle[TunerInst];
   STTUNER_OpenParams_t     OpenParams;
   STTUNER_Scan_t           ScanParams;    
   STTUNER_EventType_t      Event[TunerInst];
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_TermParams_t     TermParams[TunerInst];
   U32                      exp, div, quo, rem, exp1;
   S32                      offset_type                 = 0;

   U32                      FStart=0, FEnd=0,  FStepSize    = 0;

   Init_scanparams();

   for (i = 0; i < TunerInst; i++)
   {
      TermParams[i].ForceTerminate = FALSE;
   }        
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Init failed\n", i));
         DebugError("STTUNER_Init()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Init success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      error = STTUNER_Open(TunerDeviceName[i], &OpenParams, &TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Open failed\n", i));
         DebugError("STTUNER_Open()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Open success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetBandList(TUNERHandle[i], &Bands);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Bandlist failed\n", i));
         DebugError("STTUNER_SetBandList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Bandlist Set\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetScanList(TUNERHandle[i], &Scans);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Scanlist failed\n", i));
         DebugError("STTUNER_SetScanList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Scanlist Set\n", i));
      }
   }            
   for (i = 0; i < TunerInst; i++)
   {
        if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
     
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }

      error = STTUNER_Scan(TUNERHandle[i], FStart, FEnd, FStepSize, 0);
      /*DebugError("STTUNER_Scan()", error);*/
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Scan failed\n", i));
         DebugError("STTUNER_Scan()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Scan Start..\n", i));
      } 

      Notify_ID = i;
      AwaitNotifyFunction(&Event[i]);
      DebugPrintf(("Event [%d] = %s.\n", i, TUNER_EventToString(Event[i])));

      if (Event[i] == STTUNER_EV_LOCKED)
      {
         /* Get status information */
         error = STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);

         if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)
         {
            #if defined ( TUNER_899)|| defined (TUNER_288)
            DebugPrintf(("%02d F =%u(%s)kHz, Q = %u, BER=%2d, FEC=%s, AGC=%sdBm, IQ=%d\n", 1, TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
            #endif
         }  
         if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362 ||TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372)
         {
            #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined(TUNER_362 )
            /* Get status information */    
            error = STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);
            exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
            if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
               rem /= div;

            if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
            {
               offset_type = 1;
            }
            else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
            {
               offset_type = -1;
            }
            else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
            {
               offset_type = 0;
            }
            DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%, SpectrumInversion = %s , 					Frequency Offset Type = %d , Frequency Offset = %d Khz, BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_TER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, TUNER_TER_SpectrumToString(TunerInfo.ScanInfo.Spectrum), offset_type, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.BitErrorRate, quo, rem, exp));

            #endif
         }

      }/* end of if EVENT_LOCKED */
      else
      {
         DebugPrintf(("Event = %s\n", TUNER_EventToString(Event[i])));
         TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
         break;
      }

      if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899||TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288)
      {
         #if defined ( TUNER_899)|| defined (TUNER_288)
         /* Lock to a frequency ----------------- */    
         ScanParams.Polarization = TunerInfo.ScanInfo.Polarization;
         ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;
         ScanParams.Modulation = TunerInfo.ScanInfo.Modulation;
         ScanParams.LNBToneState = TunerInfo.ScanInfo.LNBToneState;
         ScanParams.Band = TunerInfo.ScanInfo.Band;
         ScanParams.AGC = TunerInfo.ScanInfo.AGC;
         ScanParams.SymbolRate = TunerInfo.ScanInfo.SymbolRate;
         ScanParams.IQMode = TunerInfo.ScanInfo.IQMode;
         #ifdef TUNER_ECHO
         ScanParams.DEMODMode = TunerInfo.ScanInfo.DEMODMode;
         #endif
         ScanParams.FECType = TunerInfo.ScanInfo.FECType;
         #ifdef SAT_TEST_SCR
         ScanParams.ScrParams.LNBIndex = TunerInfo.ScanInfo.ScrParams.LNBIndex;
         ScanParams.ScrParams.SCRBPF = TunerInfo.ScanInfo.ScrParams.SCRBPF;
         ScanParams.LNB_SignalRouting = TunerInfo.ScanInfo.LNB_SignalRouting;
         ScanParams.PLSIndex = TunerInfo.ScanInfo.PLSIndex;
         #endif
         #endif
      }
      if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372) 
      {
         #if defined(TUNER_370VSB) ||defined(TUNER_372)
         /* Lock to a frequency ----------------- */
         ScanParams.Band = 1;
         ScanParams.AGC = 1;
         ScanParams.FECRates = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;

         ScanParams.Spectrum = STTUNER_INVERSION_AUTO;
         ScanParams.FreqOff = STTUNER_OFFSET_NONE;/*TunerInfo.ScanInfo.FreqOff;*/
         ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;
         #ifdef CHANNEL_BANDWIDTH_6
         ScanParams.ChannelBW = STTUNER_CHAN_BW_6M;
         #endif
         #ifdef  CHANNEL_BANDWIDTH_7
         ScanParams.ChannelBW = STTUNER_CHAN_BW_7M;

         #endif
         #ifdef  CHANNEL_BANDWIDTH_8
         ScanParams.ChannelBW = STTUNER_CHAN_BW_8M;
         #endif 
         #if defined(TUNER_370VSB) || defined(TUNER_372) || defined(TUNER_362)
         ScanParams.SymbolRate = 10762000 ;
         ScanParams.IQMode = STTUNER_IQ_MODE_AUTO;
         #endif        
         #endif
       }  
      #if defined(TUNER_362) 
    if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362) 
      {
      ScanParams.Band = 1;
      ScanParams.AGC = 1;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;   
      ScanParams.Spectrum = STTUNER_INVERSION_AUTO;
      ScanParams.FreqOff = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
      ScanParams.FECRates = TunerInfo.ScanInfo.FECRates;

      #ifdef CHANNEL_BANDWIDTH_6
      ScanParams.ChannelBW = STTUNER_CHAN_BW_6M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_7
      ScanParams.ChannelBW = STTUNER_CHAN_BW_7M;
      #endif
      #ifdef  CHANNEL_BANDWIDTH_8
      ScanParams.ChannelBW = STTUNER_CHAN_BW_8M;
      #endif 

      DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n", TunerInfo.Frequency, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset));
      TunerInfo.Frequency = TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset;
      /* We locked due to a scan so the relocking functionality is not enabled, to
           do this we need to do a STTUNER_SetFrequency() to the current frequency. */

         /*** Add the offset to get the exact central carrier frequency*****/
         DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n", TunerInfo.Frequency, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset));
         TunerInfo.Frequency = TunerInfo.Frequency + TunerInfo.ScanInfo.ResidualOffset;
      }
#endif





         DebugMessage("Tuning...\n");
         error = STTUNER_SetFrequency(TUNERHandle[i], TunerInfo.Frequency, &ScanParams, 0);
         DebugError("STTUNER_SetFrequency()", error);

      if (error != ST_NO_ERROR)
      {
         TestFailed(Result, "Expected = 'ST_NO_ERROR'");
      }
      Notify_ID = i;
      AwaitNotifyFunction(&Event[i]);

      DebugPrintf(("Event  = %s.\n", TUNER_EventToString(Event[i])));

      /* Check whether or not we're locked */
      if (Event[i] != STTUNER_EV_LOCKED)
      {
         TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
      }

      if (Event[i] == STTUNER_EV_LOCKED)

      {
         error = STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);
         DebugError("STTUNER_GetTunerInfo()", error);

         if (error != ST_NO_ERROR)
         {
            TestFailed(Result, "Expected = 'ST_NO_ERROR'");
         }
         DebugPrintf(("\n"));

         if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288)
         {
            #if defined ( TUNER_899)|| defined (TUNER_288)
            DebugPrintf(("%2d F= %u(%s)kHz, Q= %u, BER= %2d, FEC= %s, AGC= %sdBm, IQ= %d\n", 1, TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
            #endif  
         }
         if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362 ||TunerInitParams[i].DemodType==STTUNER_DEMOD_STV0372)
         {
            #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined(TUNER_362)
            /* Get tuner information */
            error = STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);
            if (error != ST_NO_ERROR)
            {
               DebugError("STTUNER_GetTunerInfo()", error);
               TestFailed(Result, "Expected = 'ST_NO_ERROR'");
            }

            exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
            if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
               rem /= div;

            if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
            {
               offset_type = 1;
            }
            else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
            {
               offset_type = -1;
            }
            else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
            {
               offset_type = 0;
            }


            DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%, SpectrumInversion =	 %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_TER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, TUNER_TER_SpectrumToString(TunerInfo.ScanInfo.Spectrum), offset_type, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.BitErrorRate, quo, rem, exp));
            #endif
         }
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Close(TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Close failed\n", i));
         DebugError("STTUNER_Close()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Close success\n", i));
      }
   }        
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Term(TunerDeviceName[i], &TermParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Term failed\n", i));
         DebugError("STTUNER_Term()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Term success\n", i));
      }
   }        


   if (error == ST_NO_ERROR)
   {
      DebugMessage("Tests Results in to Success        ");
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }
   return (Result);


}/* End of RigorousTunerMultiInstanceScanStabilityTest */

/*#endif*/

/*#ifndef ST_OSLINUX*/

/**************************************************************************
TunerMultiInstanceTimedScan()
Description:
    This routine performs a full scan test for two tuners simultaneously t and records the time taken for
    it to complete.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/
static TUNER_TestResult_t
TunerMultiInstanceTimedScan(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result                  = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error                   = ST_NO_ERROR;
   STTUNER_Handle_t         TUNERHandle[TunerInst];
   STTUNER_OpenParams_t     OpenParams;
   U32                      i;
   STTUNER_EventType_t      Event[TunerInst];
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_TermParams_t     TermParams[TunerInst];
   U32                      ChannelCount            = 0;
   U32                      FStart=0, FEnd=0, FStepSize = 0;
   FStepSize = 0;

   #ifdef ST_OS21    
   osclock_t    Time;
   #else
   clock_t      Time;
   #endif    

   Init_scanparams();

   for (i = 0; i < TunerInst; i++)
   {
      TermParams[i].ForceTerminate = FALSE;
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Init failed\n", i));
         DebugError("STTUNER_Init()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Init success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      error = STTUNER_Open(TunerDeviceName[i], &OpenParams, &TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Open failed\n", i));
         DebugError("STTUNER_Open()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Open success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetBandList(TUNERHandle[i], &Bands);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Bandlist failed\n", i));
         DebugError("STTUNER_SetBandList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Bandlist Set\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetScanList(TUNERHandle[i], &Scans);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Scanlist failed\n", i));
         DebugError("STTUNER_SetScanList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Scanlist Set\n", i));
      }
   }            

   for (i = 0; i < TunerInst; i++)
   {
        if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
      FStepSize= 0;
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
     
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }
      ChannelCount = 0;



      /* Now attempt a scan */
      DebugPrintf(("Commencing scan Tuner[%d]\n", i));
      Time = time_now();
      error = STTUNER_Scan(TUNERHandle[i], FStart, FEnd, FStepSize, 0);

      /* Checking locked frequency for Tuner */
      for (; ;)
      {
         Notify_ID = i;
         AwaitNotifyFunction(&Event[i]);
    
         if (Event[i] == STTUNER_EV_LOCKED)
         {
            /* Get status information */

            STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);
 

            /* Store channel information in channel table */
            ChannelTable[i][ChannelCount].F = TunerInfo.Frequency;
            memcpy(&ChannelTable[i][ChannelCount].ScanInfo, &TunerInfo.ScanInfo, sizeof(STTUNER_Scan_t));

            /* Increment channel count */
            ChannelCount++;

         }
         else if (Event[i] == STTUNER_EV_SCAN_FAILED)
         {
            break;
         }
         if (Event[i] != STTUNER_EV_LNB_FAILURE)
            {
               STTUNER_ScanContinue(TUNERHandle[i], 0);
            }
        /* else
         {
            DebugPrintf(("Unexpected event (%s)\n", TUNER_EventToString(Event[i])));

         }
          Next scan step */
      }/*for(;;)*/


      Time = (1000 * time_minus(time_now(), Time)) / ST_GetClocksPerSecond();

      ChannelTableCount[i] = ChannelCount;
      ChannelCount=0;

      /* Delay for a while and ensure we do not receive */
      DebugPrintf(("Time taken = %u ms to scan band for %u channels.\n", (unsigned int)Time, ChannelCount));

      if (ChannelTableCount[i] > 0)
      {
      	DebugMessage(" The channels has been Counted Successfully      ");
         TestPassed(Result);
      }
      else
      {
      	DebugMessage("No channels found       ");
         TestFailed(Result, " ");
      }
   }    
   for (i = 0; i < TunerInst; i++)

   {
      error = STTUNER_Close(TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Close failed\n", i));
         DebugError("STTUNER_Close()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Close success\n", i));
      }
   }        
   for (i = 0; i < TunerInst; i++)
   {


      error = STTUNER_Term(TunerDeviceName[i], &TermParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Term failed\n", i));
         DebugError("STTUNER_Term()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Term success\n", i));
      }
   }            
   if (error == ST_NO_ERROR)
   {  
   	DebugMessage("Tests Results in to Success        ");
        TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }

   return (Result);

} /* TunerMultiInstanceTimedScan() */
/*#endif*/

/*#ifndef ST_OSLINUX*/

/*****************************************************************************
TunerMultiInstanceScanExact()
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
static TUNER_TestResult_t
TunerMultiInstanceScanExact(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result                                          = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error                                           = ST_NO_ERROR;
   U8                       i                                               = 0,n ;
   STTUNER_OpenParams_t     OpenParams;   
   STTUNER_EventType_t      Event[TunerInst];
   STTUNER_TunerInfo_t      TunerInfo;
   STTUNER_TermParams_t     TermParams[TunerInst];
   STTUNER_Handle_t         TUNERHandle[TunerInst];
   U32                      exp, div, quo, rem, exp1;
   S32                      offset_type                                     = 0;
   U32                      ChannelCount                                    = 0;
   U32                      FStart, FEnd,  FStepSize  = 0;



   Init_scanparams();




   for (i = 0; i < TunerInst; i++)
   {
      TermParams[i].ForceTerminate = FALSE;
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Init failed\n", i));
         DebugError("STTUNER_Init()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Init success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      error = STTUNER_Open(TunerDeviceName[i], &OpenParams, &TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Open failed\n", i));
         DebugError("STTUNER_Open()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Open success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetBandList(TUNERHandle[i], &Bands);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Bandlist failed\n", i));
         DebugError("STTUNER_SetBandList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Bandlist Set\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetScanList(TUNERHandle[i], &Scans);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Scanlist failed\n", i));
         DebugError("STTUNER_SetScanList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Scanlist Set\n", i));
      }
   }            

   for (i = 0; i < TunerInst; i++)
   {
        if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
      FStepSize= 0;
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
     
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }

      DebugMessage(("Purpose: Scan through stored channel information \n"));

      /* Now attempt a set frequency */
      for (n = 0; n < ChannelTableCount[i]; n++)
      {
      	 ChannelCount=0;
         STTUNER_SetFrequency(TUNERHandle[i], ChannelTable[i][n].F, &ChannelTable[i][n].ScanInfo, 0);
         Notify_ID = i;
         AwaitNotifyFunction(&Event[i]);
         
         if (Event[i] == STTUNER_EV_LOCKED)
         {
         	ChannelCount++;
            error = STTUNER_GetTunerInfo(TUNERHandle[i], &TunerInfo);
            DebugError("STTUNER_GetTunerInfo()", error);

            if (error != ST_NO_ERROR)
            {
               TestFailed(Result, "Expected = 'ST_NO_ERROR'");
            }
            DebugPrintf(("\n"));

            if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288)
            {
               #if defined ( TUNER_899)|| defined (TUNER_288)
               DebugPrintf(("%2d F=%u(%s) kHz Q=%u%% BER=%2d FEC=%s AGC=%sdBm IQ=%d\n", 1, TunerInfo.Frequency, TUNER_PolarityToString(TunerInfo.ScanInfo.Polarization), TunerInfo.SignalQuality, TunerInfo.BitErrorRate, TUNER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_AGCToPowerOutput(TunerInfo.ScanInfo.AGC), TunerInfo.ScanInfo.IQMode /*changed for GNBvd26107->I2C failure due to direct access to demod device at API level*/));
               #endif   
            }
            if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362)
            {
               #if defined(TUNER_370VSB) ||defined(TUNER_372) || defined (TUNER_362) 
               exp = (U32) GetPowOf10_BER(TunerInfo.BitErrorRate);
               exp1 = (U32) GetPowOf10(TunerInfo.BitErrorRate) - 1;
               div = PowOf10(exp1);
               quo = TunerInfo.BitErrorRate / div;
               rem = TunerInfo.BitErrorRate % div;
               div /= 100;                 /* keep only two numbers after decimal point */
               if (div != 0)     /*use to avoid crashing in 0/0 division in toolset ST40R4.0.1 */
                  rem /= div;

               if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
               {
                  offset_type = 1;
               }
               else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
               {
                  offset_type = -1;
               }
               else if (TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
               {
                  offset_type = 0;
               }

               DebugPrintf(("F =%u KHz, FEC =%s, Modulation =%s, Status \'%s\', Quality =%u%%, SpectrumInversion =	 %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, BER (%d) = %d.%dx10**-%d\n", TunerInfo.Frequency, TUNER_TER_FecToString(TunerInfo.ScanInfo.FECRates), TUNER_TER_ModulationToString(TunerInfo.ScanInfo.Modulation), TUNER_TER_StatusToString(TunerInfo.Status), TunerInfo.SignalQuality, TUNER_TER_SpectrumToString(TunerInfo.ScanInfo.Spectrum), offset_type, TunerInfo.ScanInfo.ResidualOffset, TunerInfo.BitErrorRate, quo, rem, exp));

               #endif
            }
         }
        /* else
         {
            DebugPrintf(("Event 1= %s\n", TUNER_EventToString(Event[i])));
         }*/
         DebugPrintf(("Tuner[%d] Channels = %u / %u\n",i, ChannelCount, ChannelTableCount[i]));
         if (ChannelCount == ChannelTableCount[i])
             {
              TestPassed(Result);
             }
           else
            {
             TestFailed(Result, "Incorrect number of channels");
            }
      }  /* for(i) */
   }
   

   /* If we're currently locked then we should unlock and then catch the
   event to ensure we leave things in a tidy state. */


   for (i = 0; i < TunerInst; i++)
   {
      if (Event[i] == STTUNER_EV_LOCKED)
      {
         error = STTUNER_Unlock(TUNERHandle[i]);
         Notify_ID = i;
         AwaitNotifyFunction(&Event[i]);
         if (Event[i] == STTUNER_EV_LOCKED)
         {
            DebugPrintf(("Unlock for Tuner[%d] !!!! FAILED !!!! \n", i));
            DebugError("STTUNER_Unlock()", error);
         }
         else
         {
            DebugPrintf(("Tuner [%d] Unlocked  Result: !!!! PASS !!!!\n",i));
         }
      }
   }                    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Close(TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Close failed\n", i));
         DebugError("STTUNER_Close()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Close success\n", i));
      }
   }        
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Term(TunerDeviceName[i], &TermParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Term failed\n", i));
         DebugError("STTUNER_Term()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Term success\n", i));
      }
   }


   if (error == ST_NO_ERROR)
   {
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }

   return (Result);

} /* TunerMultiInstanceScanExact() */
/*#endif*/

/*#ifndef ST_OSLINUX*/

/*****************************************************************************
TunerMultiInstanceTimedScanExact()
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

static TUNER_TestResult_t
TunerMultiInstanceTimedScanExact(TUNER_TestParams_t *Params_p)
{
   TUNER_TestResult_t       Result                  = TEST_RESULT_ZERO;
   ST_ErrorCode_t           error                   = ST_NO_ERROR;
   STTUNER_Handle_t         TUNERHandle[TunerInst];
   STTUNER_OpenParams_t     OpenParams;

   STTUNER_EventType_t      Event[TunerInst];
   STTUNER_TermParams_t     TermParams[TunerInst];
   U8                       i, n;
   U32                      FStart,FEnd,FStepSize   = 0;
   U32                      ChannelCount            = 0;
   #ifdef ST_OS21    
   osclock_t                StartClocks, TotalClocks;
   #else
   clock_t                  StartClocks, TotalClocks;
   #endif    







   Init_scanparams();

   for (i = 0; i < TunerInst; i++)
   {
      TermParams[i].ForceTerminate = FALSE;
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Init(TunerDeviceName[i], &TunerInitParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Init failed\n", i));
         DebugError("STTUNER_Init()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Init success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      Notify_ID = i;
      OpenParams.NotifyFunction = CallbackNotifyFunction;

      error = STTUNER_Open(TunerDeviceName[i], &OpenParams, &TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Open failed\n", i));
         DebugError("STTUNER_Open()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Open success\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetBandList(TUNERHandle[i], &Bands);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Bandlist failed\n", i));
         DebugError("STTUNER_SetBandList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Bandlist Set\n", i));
      }
   }    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_SetScanList(TUNERHandle[i], &Scans);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Setting Scanlist failed\n", i));
         DebugError("STTUNER_SetScanList()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Scanlist Set\n", i));
      }
   }            
   for (i = 0; i < TunerInst; i++)
   {
        if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STX0288 || TunerInitParams[i].DemodType == STTUNER_DEMOD_STB0899)
   {
      FStart = FLIMIT_LO;
      FEnd = FLIMIT_HI;
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0362) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
      
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_362 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_362 * 1000)) / 2 ;
      }
   }
   if (TunerInitParams[i].DemodType == STTUNER_DEMOD_STV0372) 
   {
      FStart = TER_FLIMIT_LO;
      FEnd = TER_FLIMIT_HI;
     
      if (FStart < FEnd)
      {
         FStepSize = ((TEST_FREQUENCY_372 * 1000) - FStart) / 2 ;
      }
      else
      {
         FStepSize = (FStart - (TEST_FREQUENCY_372 * 1000)) / 2 ;
      }
   }
      DebugMessage("Purpose: Scan through stored channel information.\n");
      DebugMessage("Starting timer...\n");

      StartClocks = time_now();

      /* Now attempt a set frequency */
      for (n = 0; n < ChannelTableCount[i]; n++)
      {
         ChannelCount=0;
         STTUNER_SetFrequency(TUNERHandle[i], ChannelTable[i][n].F, &ChannelTable[i][n].ScanInfo, 0);
         Notify_ID = i;
         AwaitNotifyFunction(&Event[i]);
         if (Event[i] == STTUNER_EV_LOCKED)
         {
            ChannelCount++;
         }
         else
         {
            DebugPrintf(("Event  = %s\n", TUNER_EventToString(Event[i])));
         }
      }


      TotalClocks = (1000 * time_minus(time_now(), StartClocks)) / ST_GetClocksPerSecond();

      DebugMessage("Stopped timer...\n");
      DebugPrintf(("Channels = %u / %u\n", ChannelCount, ChannelTableCount[i]));
      DebugPrintf(("Time taken = %u ms.\n", (unsigned int)TotalClocks));
      if (ChannelCount == ChannelTableCount[i])
      {
         TestPassed(Result);
      }
      else
      {
         TestFailed(Result, "Incorrect number of channels\n");
      }
   }
   /* If we're currently locked then we should unlock and then catch the
    event to ensure we leave things in a tidy state. */
   for (i = 0; i < TunerInst; i++)
   {
      if (Event[i] == STTUNER_EV_LOCKED)
      {
         error = STTUNER_Unlock(TUNERHandle[i]);
         Notify_ID = i;
         AwaitNotifyFunction(&Event[i]);
         if (Event[i] == STTUNER_EV_LOCKED)
         {
            DebugPrintf(("Unlock for Tuner[%d] !!!! FAILED !!!! \n", i));
            DebugError("STTUNER_Unlock()", error);
         }
         else
         {
            DebugPrintf(("Tuner [%d] Unlocked  Result: !!!! PASS !!!!\n",i));
         }
      }
   }                    
   for (i = 0; i < TunerInst; i++)
   {
      error = STTUNER_Close(TUNERHandle[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Close failed\n", i));
         DebugError("STTUNER_Close()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Close success\n", i));
      }
   }        
   for (i = 0; i < TunerInst; i++)
   {


      error = STTUNER_Term(TunerDeviceName[i], &TermParams[i]);
      if (error != ST_NO_ERROR)
      {
         DebugPrintf(("Tuner[%d] Term failed\n", i));
         DebugError("STTUNER_Term()", error);
      }
      else
      {
         DebugPrintf(("Tuner[%d] Term success\n", i));
      }
   }

   if (error == ST_NO_ERROR)
   {
      TestPassed(Result);
   }
   else
   {
      TestFailed(Result, "Expected = 'ST_NO_ERROR'");
   }

   return (Result);
} /* TunerMultiInstanceScanExact() */
/*#endif*/

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------ */
/* Debug routines --------------------------------------------------------- */
/* ------------------------------------------------------------------------ */


char*
TUNER_ErrorString(ST_ErrorCode_t Error)
{
   return STAPI_ErrorMessages(Error);
}

/* ------------------------------------------------------------------------ */

char*
TUNER_FecToString(STTUNER_FECRate_t Fec)
{
   int          i;
   static char  FecRate[][25]   =
   {
      "1_2", "2_3", "3_4", "4_5", "5_6", "6_7", "7_8", "8_9", "1_4", "1_3", "2_5", "3_5", "9_10"
   };

   for (i = 0; Fec > 0; i++, Fec >>= 1)
      ;
   return FecRate[i - 1];
}

/* ------------------------------------------------------------------------ */

char*
TUNER_DemodToString(STTUNER_Modulation_t Mod)

{
   int              i;
   static char     *Modulation[]    =
   {
      "QPSK\0",     /* 0 */
      "8PSK\0", "QAM\0", "4QAM\0", "16QAM\0", "32QAM\0", "64QAM\0", "128QAM\0", "256QAM\0", "BPSK\0",     /* 8 */
      "16APSK\0", "32APSK\0", "NONE\0", "ALL\0", "!!INVALID MODULATION!!\0"
   };

   if (Mod == STTUNER_MOD_NONE)
      return Modulation[12];
   if (Mod == STTUNER_MOD_ALL)
      return Modulation[13];
   for (i = 0; Mod > 0; i++, Mod >>= 1)
      ;
   return Modulation[i - 1];
}

/* ------------------------------------------------------------------------ */

char*
TUNER_PolarityToString(STTUNER_Polarization_t Plr)
{
   static char     *Polarity[]  =
   {
      "HORIZONTAL", "VERTICAL", "ALL(H+V)", "OFF", "!!INVALID POLARITY!!"
   };

   if ((Plr > STTUNER_PLR_ALL) || (Plr == 3) || (Plr == 5) || (Plr == 6))
      return Polarity[4];
   if (Plr == STTUNER_PLR_ALL)
      return Polarity[2];
   return Polarity[Plr - 1];
}


/* ------------------------------------------------------------------------ */

char*
TUNER_ToneToString(STTUNER_LNBToneState_t ToneState)
{
   switch (ToneState)


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

char*
TUNER_EventToString(STTUNER_EventType_t Evt)
{
   static int   i   = 0;
   static char  c[3][80];/*earlier it was NUM_TUNER*/

   switch (Evt)
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
   if (i >= TunerInst)
      i = 0;
   sprintf(c[i], "NOT_STTUNER_EVENT(%d)", Evt);
   return(c[i]);
}

/* ------------------------------------------------------------------------ */

char*
TUNER_AGCToPowerOutput(U8 Agc)
{
   static char  buf[10];

   sprintf(buf, "%5d.", -(127 - (Agc / 2)));
   return strcat(buf, (Agc % 2) ? "0" : "5");
}

/* ------------------------------------------------------------------------ */

static char*
STAPI_ErrorMessages(ST_ErrorCode_t ErrCode)
{
   switch (ErrCode)
   {
         #ifndef ST_OSLINUX
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

         /* sttbx.h */
      case STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED:
         return("STTBX_ERROR_FUNCTION_NOT_IMPLEMENTED");
         #endif
         #endif /*  #ifndef ST_OSLINUX*/
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

char*
TUNER_TER_FecToString(STTUNER_FECRate_t Fec)
{
   int          i;
   static char  FecRate[][25]   =
   {
      "1_2", "2_3", "3_4", "5_6", "5_6", "6_7", "7_8", "8_9"
   };

   for (i = 0; Fec > 0; i++, Fec >>= 1)
      ;
   return FecRate[i - 1];


}
/*-------------------------------------------------------------------------
 * Function : bit2num
 *            Converts bit to number for array selection
 * Input    : bit selected
 * Output   :
 * Return   : position of bit
 * ----------------------------------------------------------------------*/
static int
bit2num(int bit)
{
   int  i;
   for (i = 0 ; i <= 16; i ++)
   {
      if ((1 << i) & bit)
         break;
   }

   /* return ++i;*/
   if (i == 0)
      return (i + 1);
   else
      return i;

} /* end bit2num */
/*-------------------------------------------------------------------------
 * Function : TUNER_TER_ModulationToString
 *            Convert Modulation value to text
 * Input    : Modulation value
 * Output   :
 * Return   : pointer to Modulation text
 * ----------------------------------------------------------------------*/
char*
TUNER_TER_ModulationToString(STTUNER_Modulation_t Modul)
{
   static char  Modulation[][8] =
   {
      "ALL   ", "QPSK  ", "8PSK  ", "QAM   ", "16QAM ", "32QAM ", "64QAM ", "128QAM", "256QAM", "BPSK  ", "8VSB"
   };


   if (Modul == STTUNER_MOD_ALL)

      return Modulation[0];
   else
      return Modulation[bit2num(Modul)];

} /* end TUNER_TER_ModulationToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_TER_StatusToString
 *            Convert Status value to text
 * Input    : Event value
 * Output   :
 * Return   : pointer to text
 * ----------------------------------------------------------------------*/
char*
TUNER_TER_StatusToString(STTUNER_Status_t Status)
{
   static char  StatusDescription[][10] =
   {
      "UNLOCKED", "SCANNING", "LOCKED", "NOT_FOUND"
   };

   return StatusDescription[Status];

} /* end TUNER_TER_StatusToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_SpectrumToString
 *            Convert Spectrum value to text
 * Input    : Spectrum value
 * Output   :
 * Return   : pointer to Spectru text
 * ----------------------------------------------------------------------*/
char*
TUNER_TER_SpectrumToString(STTUNER_Spectrum_t Spectrum)
{
   if (Spectrum == STTUNER_INVERSION)
      return "YES";
   else
      return "NO";

} /* end TUNER_SpectrumToString */
#ifdef TUNER_ECHO
/*-------------------------------------------------------------------------
 * Function : TUNER_HierarchyToString
 *            Convert Hierarchy value to text
 * Input    : Hierarchy value
 * Output   :
 * Return   : pointer to Hierarchy text
 * ----------------------------------------------------------------------*/
char*
TUNER_TER_HierarchyToString(STTUNER_Hierarchy_t Hierarchy)
{
   if (Hierarchy == STTUNER_HIER_LOW_PRIO)
      return "HIERARCHY a=1";
   else if (Hierarchy == STTUNER_HIER_HIGH_PRIO)
      return "HIERARCHY a=2";
   else if (Hierarchy == STTUNER_HIER_PRIO_ANY)
      return "HIERARCHY a=4";
   else
      return "STTUNER_HIER_NONE";

} /* end TUNER_SpectrumToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_GuardToString
 *            Convert Spectrum value to text
 * Input    : Guard value
 * Output   :
 * Return   : pointer to Guard text
 * ----------------------------------------------------------------------*/
char*
TUNER_TER_GuardToString(STTUNER_Guard_t Guard)
{
   if (Guard == STTUNER_GUARD_1_32)
      return "GUARD_1_32";
   else if (Guard == STTUNER_GUARD_1_16)
      return "GUARD_1_16";
   else if (Guard == STTUNER_GUARD_1_8)
      return "GUARD_1_8";
   else if (Guard == STTUNER_GUARD_1_4)
      return "GUARD_1_4";
   else
      return "NO GUARD";

} /* end TUNER_GuardToString */
#endif
/*****************************************************
**FUNCTION  ::  GetPowOf10
**ACTION    ::  Compute  x according to y with y=10^x
**PARAMS IN ::  number -> y
**PARAMS OUT::  NONE
**RETURN    ::  x
*****************************************************/
long
GetPowOf10(int number)
{
   int  i   = 0;

   while (PowOf10(i) < number)
      i++;

   return i;
}

/*****************************************************
**FUNCTION  ::  GetPowOf10_BER
**ACTION    ::  Compute  x according to y with y=10^x taking
                into consideration in the low level driver
                BER value has been scaled up by 10^10 .
**PARAMS IN ::  number -> y
**PARAMS OUT::  NONE
**RETURN    ::  x
*****************************************************/
long
GetPowOf10_BER(int number)
{
   int  i   = 0;

   while (PowOf10(i) < number)
      i++;
   if ((i != 0) && (i > 0))
   {
      #ifndef TUNER_370VSB
      i = (10 - i) + 1;
      #endif
      #ifdef TUNER_370VSB
      i = 8 + (i - 1);
      #endif
   }   


   return i;
}

/*****************************************************
**FUNCTION  ::  PowOf10
**ACTION    ::  Compute  10^n (where n is an integer)
**PARAMS IN ::  number -> n
**PARAMS OUT::  NONE
**RETURN    ::  10^n
*****************************************************/
int
PowOf10(int number)
{
   int  i;
   int  result  = 1;

   for (i = 0; i < number; i++)
      result *= 10;

   return result;
}
/**************************************************************************************************/

void Sat_init()



{
   U8               i;
   ST_DeviceName_t  DeviceName;

   #ifdef TUNER_899
   /*Checking if 899 was detected in probe or not*/
   for (i = 0; i <= TunerInst; i++)
   {
      if (strcmp(TunerType[i],"STB0899")==0)
      {
         DebugMessage("Configuring initparams for STB0899......\n");
         break;
      }
      if (i > TunerInst)
      {
         DebugMessage("Tuner STB0899 not detected during probe\n");
         return;
      }
   }      
   /* Init params for 899*/         
   sprintf((char*) DeviceName, "STTUNER[%d]", i);
   strcpy((char*) TunerDeviceName[i], (char*) DeviceName);
   strcpy(TunerInitParams[i].EVT_DeviceName, EVT_DeviceName[0]);
   strcpy(TunerInitParams[i].EVT_RegistrantName, EVT_RegistrantName[i]);
   strcpy(TunerInitParams[i].DemodIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   strcpy(TunerInitParams[i].TunerIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   strcpy(TunerInitParams[i].LnbIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);                
   TunerInitParams[i].Device = STTUNER_DEVICE_SATELLITE;
   TunerInitParams[i].Max_BandList = 10;
   TunerInitParams[i].Max_ThresholdList = 10;
   TunerInitParams[i].Max_ScanList = 10;                
   TunerInitParams[i].DriverPartition = system_partition;                     
   /* ADV1 sat drivers */
   TunerInitParams[i].DemodType = STTUNER_DEMOD_STB0899;   
   #if defined(LNB_21)
   TunerInitParams[i].LnbType = STTUNER_LNB_LNB21;
   #elif defined(LNBH_21)
   TunerInitParams[i].LnbType = STTUNER_LNB_LNBH21;
   #else
   TunerInitParams[i].LnbType = STTUNER_LNB_NONE;
   #endif
   TunerInitParams[i].TunerType = STTUNER_TUNER_STB6100;
   TunerInitParams[i].DemodIO.Route = STTUNER_IO_DIRECT;      /* I/O direct to hardware */
   TunerInitParams[i].DemodIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].DemodIO.Address = DemodAddress[i];            /* stvstc1 address */           
   TunerInitParams[i].TunerIO.Route = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
   TunerInitParams[i].TunerIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].TunerIO.Address = 0xC0;               /* address for Tuner (e.g. VG1011 = 0xC0) */            
   #if defined(LNB_21) || defined(LNBH_21)
   TunerInitParams[i].LnbIO.Route = STTUNER_IO_DIRECT;     /* use demod I/O handle */
   #else
   TunerInitParams[i].LnbIO.Route = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
   #endif
   TunerInitParams[i].LnbIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].LnbIO.Address = LnbAddress[i];                    /* ignored */           
   TunerInitParams[i].ExternalClock = 4000000;
   TunerInitParams[i].TSOutputMode = STTUNER_TS_MODE_PARALLEL;
   TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
   TunerInitParams[i].DataFIFOMode = STTUNER_DATAFIFO_ENABLED; /*controls TS output datarate and use of FIFO for dataflow*/
   TunerInitParams[i].OutputFIFOConfig.CLOCKPERIOD = 2; /*for programming data clock*/
   TunerInitParams[i].OutputFIFOConfig.OutputRateCompensationMode = STTUNER_COMPENSATE_DATACLOCK;
   TunerInitParams[i].BlockSyncMode = STTUNER_SYNC_FORCED; /*control sync byte*/
   TunerInitParams[i].ClockPolarity = STTUNER_DATA_CLOCK_POLARITY_RISING;
   TunerInitParams[i].DataClockAtParityBytes = STTUNER_DATACLOCK_CONTINOUS;
   TunerInitParams[i].TSDataOutputControl = STTUNER_TSDATA_OUTPUT_CONTROL_MANUAL;
   TunerInitParams[i].FECMode = STTUNER_FEC_MODE_DVB;
   /*STTBX_Print(("done\n"));*/
   #endif   

   #ifdef TUNER_288
   /*Checking if 288 was detected in probe or not*/
   for (i = 0; i <= TunerInst; i++)
   {
      if (strcmp(TunerType[i],"STX0288")==0)
      {
         DebugMessage("Configuring initparams for STX0288......");
         break;
      }
      if (i > TunerInst)
      {
         DebugMessage("Tuner STX0288 not detected during probe\n");
         return;
      }
   }      
   /* Init params for 288*/         
   sprintf((char*) DeviceName, "STTUNER[%d]", i);
   strcpy((char*) TunerDeviceName[i], (char*) DeviceName);
   strcpy(TunerInitParams[i].EVT_DeviceName, EVT_DeviceName[0]);
   strcpy(TunerInitParams[i].EVT_RegistrantName, EVT_RegistrantName[i]);
   strcpy(TunerInitParams[i].DemodIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   strcpy(TunerInitParams[i].TunerIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   strcpy(TunerInitParams[i].LnbIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);                
   TunerInitParams[i].Device = STTUNER_DEVICE_SATELLITE;
   TunerInitParams[i].Max_BandList = 10;
   TunerInitParams[i].Max_ThresholdList = 10;
   TunerInitParams[i].Max_ScanList = 10;                
   TunerInitParams[i].DriverPartition = system_partition;                     
   /* ADV1 sat drivers */
   TunerInitParams[i].DemodType = STTUNER_DEMOD_STX0288;   
   #if defined(LNB_21)
   TunerInitParams[i].LnbType = STTUNER_LNB_LNB21;
   #elif defined(LNBH_21)
   TunerInitParams[i].LnbType = STTUNER_LNB_LNBH21;
   #else
   TunerInitParams[i].LnbType = STTUNER_LNB_NONE;
   #endif
   TunerInitParams[i].TunerType = STTUNER_TUNER_STB6000;
   TunerInitParams[i].DemodIO.Route = STTUNER_IO_DIRECT;      /* I/O direct to hardware */
   TunerInitParams[i].DemodIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].DemodIO.Address = DemodAddress[i];            /* stvstc1 address */           
   TunerInitParams[i].TunerIO.Route = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
   TunerInitParams[i].TunerIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].TunerIO.Address = 0xC0;               /* address for Tuner (e.g. VG1011 = 0xC0) */            
   #if defined(LNB_21) || defined(LNBH_21)
   TunerInitParams[i].LnbIO.Route = STTUNER_IO_DIRECT;     /* use demod I/O handle */
   #else
   TunerInitParams[i].LnbIO.Route = STTUNER_IO_PASSTHRU;     /* use demod I/O handle */
   #endif
   TunerInitParams[i].LnbIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].LnbIO.Address = LnbAddress[i];                    /* ignored */           
   TunerInitParams[i].ExternalClock = 4000000;
   TunerInitParams[i].TSOutputMode = STTUNER_TS_MODE_PARALLEL;
   TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
   TunerInitParams[i].DataFIFOMode = STTUNER_DATAFIFO_ENABLED; /*controls TS output datarate and use of FIFO for dataflow*/
   TunerInitParams[i].OutputFIFOConfig.CLOCKPERIOD = 2; /*for programming data clock*/
   TunerInitParams[i].OutputFIFOConfig.OutputRateCompensationMode = STTUNER_COMPENSATE_DATACLOCK;
   TunerInitParams[i].BlockSyncMode = STTUNER_SYNC_FORCED; /*control sync byte*/
   TunerInitParams[i].ClockPolarity = STTUNER_DATA_CLOCK_POLARITY_RISING;
   TunerInitParams[i].DataClockAtParityBytes = STTUNER_DATACLOCK_CONTINOUS;
   TunerInitParams[i].TSDataOutputControl = STTUNER_TSDATA_OUTPUT_CONTROL_MANUAL;
   TunerInitParams[i].FECMode = STTUNER_FEC_MODE_DVB;
   STTBX_Print(("done\n"));
   #endif   



   /* Initialize for DiSEqC2.0 */
   #ifdef STTUNER_DISEQC2_SW_DECODE_VIA_PIO
   TunerInitParams[0].DiSEqC.PIOPort = PIO_DEVICE_4;
   TunerInitParams[0].DiSEqC.PIOPin = PIO_BIT_5;
   TunerInitParams[1].DiSEqC.PIOPort = PIO_DEVICE_4;
   TunerInitParams[1].DiSEqC.PIOPin = PIO_BIT_6;
   #endif 

   return;
}

void Ter_init()

{
   ST_DeviceName_t  DeviceName;
   U8               i;

   #ifdef TUNER_372
   /*Checking if 372 was detected in probe or not*/
   for (i = 0; i <= TunerInst; i++)
   {
      if (strcmp(TunerType[i],"STV0372")==0)
      {
         DebugMessage("Configuring initparams for STV0372......");
         STTBX_Print(("done\n"));
         break;
      }
      if (i > TunerInst)
      {
         DebugMessage("Tuner STX0372 not detected during probe\n");
         return;
      }
   }      
   sprintf((char*) DeviceName, "STTUNER[%d]", i);
   strcpy((char*) TunerDeviceName[i], (char*) DeviceName);
   strcpy(TunerInitParams[i].EVT_RegistrantName, EVT_RegistrantName[i]);
   strcpy(TunerInitParams[i].DemodIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   strcpy(TunerInitParams[i].TunerIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   TunerInitParams[i].Device = STTUNER_DEVICE_TERR;
   TunerInitParams[i].Max_BandList = 10;
   TunerInitParams[i].Max_ThresholdList = 10;
   TunerInitParams[i].Max_ScanList = 10;
   strcpy(TunerInitParams[i].EVT_DeviceName, EVT_DeviceName[0]);
   TunerInitParams[i].DriverPartition = system_partition;

   /* For STV0370VSB/STV0372 ter drivers */


   #ifdef   TUNER_370VSB 
   TunerInitParams[i].DemodType = STTUNER_DEMOD_STB0370VSB;
   TunerInitParams[i].TunerType = TEST_TUNER_TYPE;  
   #elif defined(TUNER_372)
   TunerInitParams[i].DemodType = STTUNER_DEMOD_STV0372;
   TunerInitParams[i].TunerType = STTUNER_TUNER_DTT7600;        
   #endif

   TunerInitParams[i].DemodIO.Route = STTUNER_IO_DIRECT;       /* I/O direct to hardware */
   TunerInitParams[i].DemodIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].DemodIO.Address = DemodAddress[i];               /* stv0360 address */
   TunerInitParams[i].TunerIO.Route = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
   TunerInitParams[i].TunerIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].TunerIO.Address = 0xC2;               /* address for Tuner (e.g. VG1011 = 0xC0) */


   #ifdef ST_5188
   TunerInitParams[i].ExternalClock = 30000000;
   #else
   TunerInitParams[i].ExternalClock = 4000000;
   #endif

   TunerInitParams[i].TSOutputMode = STTUNER_TS_MODE_PARALLEL;
   TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
   TunerInitParams[i].SerialDataMode = STTUNER_SDAT_DEFAULT;
   TunerInitParams[i].FECMode = STTUNER_FEC_MODE_ATSC;
   TunerInitParams[i].ClockPolarity = STTUNER_DATA_CLOCK_INVERTED;/*For 370/372 VSB it is different*/


   #endif

   #ifdef TUNER_362
   /*Checking if 362 was detected or not*/
   for (i = 0; i <= TunerInst; i++)
   {
      if (strcmp(TunerType[i],"STV0362")==0)
      {
         DebugMessage("Configuring initparams for STV0362......");
         STTBX_Print(("done\n"));
         break;
      }
      if (i > TunerInst)
      {
         DebugMessage("Tuner STX0362 not detected during probe\n");
         return;
      }
   }

   /* Init params for 362*/


   sprintf((char*) DeviceName, "STTUNER[%d]", i);
   strcpy((char*) TunerDeviceName[i], (char*) DeviceName);
   strcpy(TunerInitParams[i].EVT_RegistrantName, EVT_RegistrantName[i]);
   strcpy(TunerInitParams[i].DemodIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   strcpy(TunerInitParams[i].TunerIO.DriverName, I2C_DeviceName[I2C_Bus[i]]);
   TunerInitParams[i].Device = STTUNER_DEVICE_TERR;
   TunerInitParams[i].Max_BandList = 10;
   TunerInitParams[i].Max_ThresholdList = 10;
   TunerInitParams[i].Max_ScanList = 10;
   strcpy(TunerInitParams[i].EVT_DeviceName, EVT_DeviceName[0]);
   TunerInitParams[i].DriverPartition = system_partition;
   strcpy(TunerInitParams[i].EVT_DeviceName, "stevt");

   #if defined(TUNER_362) 

   TunerInitParams[i].DemodType = STTUNER_DEMOD_STV0362;
   TunerInitParams[i].TunerType = STTUNER_TUNER_DTT7300X;
   #endif

   TunerInitParams[i].DemodIO.Route = STTUNER_IO_DIRECT;       /* I/O direct to hardware */
   TunerInitParams[i].DemodIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].DemodIO.Address = DemodAddress[i];               /* stv0360 address */
   /*TunerInitParams[i].DemodIO.Address =DEMOD_I2C_1;               stv0360 address */

   TunerInitParams[i].TunerIO.Route = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
   TunerInitParams[i].TunerIO.Driver = STTUNER_IODRV_I2C;
   TunerInitParams[i].TunerIO.Address = 0xC0;              /* address for Tuner (e.g. VG1011 = 0xC0) */

   #ifdef ST_5188
   TunerInitParams[i].ExternalClock = 30000000;
   #else
   TunerInitParams[i].ExternalClock = 4000000;
   #endif

   TunerInitParams[i].TSOutputMode = STTUNER_TS_MODE_PARALLEL;
   TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
   TunerInitParams[i].SerialDataMode = STTUNER_SDAT_DEFAULT;
   #if defined(TUNER_362) 
   TunerInitParams[i].FECMode = STTUNER_FEC_MODE_DVB;
   TunerInitParams[i].FECMode = STTUNER_FEC_MODE_ATSC;
   TunerInitParams[i].ClockPolarity = STTUNER_DATA_CLOCK_POLARITY_RISING;
   TunerInitParams[i].ClockPolarity = STTUNER_DATA_CLOCK_INVERTED;/*For 370/372 VSB it is different*/
   #endif

   #endif

   return;
}


/* EOF */












