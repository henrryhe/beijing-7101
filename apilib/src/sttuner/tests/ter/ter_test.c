/*****************************************************************************
File Name   : ter_test.c

Description : Test harness for the STTUNER driver for STi55xx.

Copyright (C) 2001 STMicroelectronics

Revision History    :

    13/11/2003 - Revision number is modified according to latest release
    06/11/2001 - file copied from sat_test.c to ter_test.c
    27/12/2004 - Satellite related parameters eliminated - CC
    
Reference:

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

#include "../tuner_test.h"         /* common test includes */


#define TUN1
#define TUN2
#if NUM_TUNER == 2
  #define TEST_DUAL 1
#endif 

/* Private types/constants ------------------------------------------------ */
#ifdef TEST_NOIDA_DTT_TRANSMISSION /* due to Noida requirements*/
#if !defined(TUNER_370VSB) && !defined(TUNER_372)
#define TEST_FREQUENCY 514
#else
#define TEST_FREQUENCY 551
#endif
#else
#define TEST_FREQUENCY 419
#endif 

#ifdef TEST_SET_30MHZ_REG_SET   /*for register setting for 30MHZ crystal*/
#define PLLNDIV_30MHZ           0x0d
#define TRLNORMRATELSB_30MHZ    0x00 
#define TRLNORMRATELO_30MHZ     0x2A
#define TRLNORMRATEHI_30MHZ     0x59
#define INCDEROT1_30MHZ  	0x4f
#define INCDEROT2_30MHZ  	0xa5
#endif

/* hardware in system */
/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
#if defined(ST_5105)|| defined(ST_5188) || defined(ST_5107)
#define NUM_PIO     4
#define NUM_I2C     2
#elif defined(ST_7200)
#define NUM_I2C     3
#define NUM_PIO     8
#else
#define NUM_PIO     5
#define NUM_I2C     2
#endif 
#endif/* USE_STI2C_WRAPPER */


#ifdef ST_OSLINUX
#define debugmessage(args) 	printf(args)
#endif
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
    PIO_DEVICE_5,
    #if defined(ST_7200)
   PIO_DEVICE_6,
   PIO_DEVICE_7,
   #endif
    PIO_DEVICE_NOT_USED
};

#define PIO_BIT_NOT_USED    0.
/*#endif*/
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

#define PIO_5_BASE_ADDRESS	ST5100_PIO5_BASE_ADDRESS
#define PIO_5_INTERRUPT		ST5100_PIO5_INTERRUPT
#endif
#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517)|| defined(ST_5528) || defined(ST_5100) || defined(ST_7710)|| defined(ST_5105)|| defined(ST_7100)|| defined(ST_7109)||defined(ST_5301) || defined(ST_8010)|| defined(ST_5107)|| defined(ST_5188)
 #define BACK_I2C_BASE  SSC_0_BASE_ADDRESS
 #define FRONT_I2C_BASE SSC_1_BASE_ADDRESS
#endif

#if defined (ST_7200)
 #define BACK_I2C_BASE  SSC_1_BASE_ADDRESS
 #define FRONT_I2C_BASE SSC_2_BASE_ADDRESS
 #define RESET_I2C_BASE SSC_4_BASE_ADDRESS
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

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) |defined (ST_5100)|| defined(ST_5105) || defined(ST_5301) || defined(ST_8010)|| defined(ST_5107)
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


/*#endif  */ /*ST_OSLINUX*/
#endif /*USE_STI2C_WRAPPER*/
/* I2C used by TUNER */
#if defined(ST_5508) || defined(ST_5518)
/* mb5518 board */
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

#elif defined(espresso) || defined(ST_5100) || defined(ST_7710)|| defined(ST_7100) || defined(ST_7109) || defined(ST_5301) || defined(ST_8010) || defined(ST_7200)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS
 
/* 5514/mediaref 399 tuners are on a common bus */
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)|| defined(ST_5528)|| defined(ST_5105)|| defined(ST_5107)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_FRONTBUS

#elif defined(ST_5188)
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS/*I2C_DEVICE_BACKBUS*/
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS/*I2C_DEVICE_BACKBUS*/

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_FRONTBUS


/* 5512 etc. */
#else
 #define TUNER0_DEMOD_I2C   I2C_DEVICE_FRONTBUS
 #define TUNER0_TUNER_I2C   I2C_DEVICE_FRONTBUS

 #define TUNER1_DEMOD_I2C   I2C_DEVICE_BACKBUS
 #define TUNER1_TUNER_I2C   I2C_DEVICE_BACKBUS

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

#if !defined(TUNER_370VSB) && !defined(TUNER_372)
#ifdef TUNER_361
#define DUT_DEMOD STTUNER_DEMOD_STV0361
#elif defined (TUNER_362)
#define DUT_DEMOD STTUNER_DEMOD_STV0362
#else
#define DUT_DEMOD STTUNER_DEMOD_STV0360
#endif
#define DUT_TUNER STTUNER_TUNER_DTT7572
#endif 

#ifdef TUNER_370VSB
#define DUT_DEMOD STTUNER_DEMOD_STB0370VSB
#define DUT_TUNER STTUNER_TUNER_DTT7600
#elif defined(TUNER_372)
#define DUT_DEMOD STTUNER_DEMOD_STV0372
#define DUT_TUNER STTUNER_TUNER_DTT7600
#endif 


#if !defined(TUNER_370VSB) && !defined(TUNER_372)
#ifdef TUNER_361
static const char ChipName[] = "STV0361";
#elif defined (TUNER_362)
static const char ChipName[] = "STV0362";
#else 
static const char ChipName[] = "STV0360";
#endif
#endif
#ifdef TUNER_370VSB
static const char ChipName[] = "STV0370VSB";
#elif defined(TUNER_372)
static const char ChipName[] = "STV0372";
#endif

/* frequency limits for scanning */
#ifdef TEST_NOIDA_DTT_TRANSMISSION
#define FLIMIT_LO 54000 
#define FLIMIT_HI 858000 
#else
#define FLIMIT_LO 50000
#define FLIMIT_HI 800000
#endif

#ifdef TEST_NOIDA_DTT_TRANSMISSION
#define BAND_NUMELEMENTS 2
#define SCAN_NUMELEMENTS 3
#else
#define BAND_NUMELEMENTS 2
#define SCAN_NUMELEMENTS 3
#endif
#ifndef ST_OSLINUX
/* Private variables ------------------------------------------------------ */
#if defined(ST_5105) || defined(ST_5107)|| defined(ST_5188)
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

#endif   /*ST_OSLINUX*/
/* Test harness revision number */
static U8 Revision[] = "6.2.1";

/*For displaying the Platform & Backend in use*/
static char Platform[20];
static char Backend[20];


#if defined (ST_OS21) || defined (ST_OSLINUX)
#define STTUNER_TaskDelay(x) STOS_TaskDelay((signed int)x)
#else
#define STTUNER_TaskDelay(x) STOS_TaskDelay((unsigned int)x)
#endif

/* Error message look up table */
#define ST_ERROR_UNKNOWN ((U32)-1)
#define HIER_MAX 5
static TUNER_ErrorMessage TUNER_ErrorLUT[] =
{
    /* I2C */
    { STI2C_ERROR_LINE_STATE,  "STI2C_ERROR_LINE_STATE"  },
    { STI2C_ERROR_STATUS,      "STI2C_ERROR_STATUS"      },
    { STI2C_ERROR_ADDRESS_ACK, "STI2C_ERROR_ADDRESS_ACK" },
    { STI2C_ERROR_WRITE_ACK,   "STI2C_ERROR_WRITE_ACK"   },
    { STI2C_ERROR_NO_DATA,     "STI2C_ERROR_NO_DATA"     },

    /* STTUNER */
    { STTUNER_ERROR_UNSPECIFIED,  "STTUNER_ERROR_UNSPECIFIED"  },
    { STTUNER_ERROR_HWFAIL,       "STTUNER_ERROR_HWFAIL"       },
    { STTUNER_ERROR_EVT_REGISTER, "STTUNER_ERROR_EVT_REGISTER" },
    { STTUNER_ERROR_ID,           "STTUNER_ERROR_ID"           },
    { STTUNER_ERROR_POINTER,      "STTUNER_ERROR_POINTER"      },
    { STTUNER_ERROR_INITSTATE,    "STTUNER_ERROR_INITSTATE"    },

    /* STAPI */
    { ST_NO_ERROR,                    "ST_NO_ERROR"                    },
    { ST_ERROR_NO_MEMORY,             "ST_ERROR_NO_MEMORY"             },
    { ST_ERROR_DEVICE_BUSY,           "ST_ERROR_DEVICE_BUSY"           },
    { ST_ERROR_INTERRUPT_INSTALL,     "ST_ERROR_INTERRUPT_INSTALL"     },
    { ST_ERROR_ALREADY_INITIALIZED,   "ST_ERROR_DEVICE_INITIALIZED"    },
    { ST_ERROR_UNKNOWN_DEVICE,        "ST_ERROR_UNKNOWN_DEVICE"        },
    { ST_ERROR_BAD_PARAMETER,         "ST_ERROR_BAD_PARAMETER"         },
    { ST_ERROR_OPEN_HANDLE,           "ST_ERROR_OPEN_HANDLE"           },
    { ST_ERROR_NO_FREE_HANDLES,       "ST_ERROR_NO_FREE_HANDLES"       },
    { ST_ERROR_INVALID_HANDLE,        "ST_ERROR_INVALID_HANDLE"        },
    { ST_ERROR_INTERRUPT_UNINSTALL,   "ST_ERROR_INTERRUPT_UNINSTALL"   },
    { ST_ERROR_TIMEOUT,               "ST_ERROR_TIMEOUT"               },
    { ST_ERROR_FEATURE_NOT_SUPPORTED, "ST_ERROR_FEATURE_NOT_SUPPORTED" },
    { ST_ERROR_UNKNOWN,               "ST_ERROR_UNKNOWN"               } /* Terminator */
};

/* Tuner device name -- used for all tests */
static ST_DeviceName_t TunerDeviceName[NUM_TUNER+1];

/*#ifndef ST_OSLINUX*/
#ifndef USE_STI2C_WRAPPER
/* List of PIO device names */
static ST_DeviceName_t PioDeviceName[NUM_PIO+1];
/*#endif */ /*ST_OSLINUX*/
#endif/*USE_STI2C_WRAPPER*/
/* List of I2C device names */
static ST_DeviceName_t I2CDeviceName[NUM_I2C+1];

#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
/* PIO Init parameters */
static STPIO_InitParams_t PioInitParams[NUM_PIO];
#endif  /*ST_OSLINUX*/
#endif/*USE_STI2C_WRAPPER*/

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

#else /*ST_OS21*/
#define                 SYSTEM_MEMORY_SIZE          0x200000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;
partition_t             *driver_partition;

#endif/*ST_OS21*/
#endif /*ST_OSLINUX*/

#ifdef ST_OSLINUX
#define system_partition NULL;
#endif

static STTUNER_Band_t BandList[10];
static STTUNER_Scan_t ScanList[10];


static STTUNER_BandList_t Bands;
static STTUNER_ScanList_t Scans;




static const ST_DeviceName_t EVT_RegistrantName1 = "stevt_1";
#ifndef STTUNER_MINIDRIVER
static const ST_DeviceName_t EVT_RegistrantName2 = "stevt_2";
#endif

/* Private function prototypes -------------------------------------------- */

char *TUNER_FecToString(STTUNER_FECRate_t Fec);
char *TUNER_DemodToString(STTUNER_Modulation_t Mod);
char *TUNER_EventToString(STTUNER_EventType_t Evt);
char *TUNER_AGCToPowerOutput(U8 Agc);
char *TUNER_ModulationToString(STTUNER_Modulation_t Modul);


char *TUNER_ModeToString(STTUNER_Mode_t Mode);


char *TUNER_SpectrumToString(STTUNER_Spectrum_t Spectrum);

char *TUNER_HierarchyToString(STTUNER_Hierarchy_t Hierarchy);


char *TUNER_GuardToString(STTUNER_Guard_t Guard);

char *TUNER_StatusToString(STTUNER_Status_t Status);
long GetPowOf10(int number);
long GetPowOf10_BER(int number);
int PowOf10(int number);
static int bit2num( int bit );
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_boot(void);
#ifndef DISABLE_TOOLBOX
static ST_ErrorCode_t Init_sttbx(void);
#endif
#endif  /* ST_OSLINUX */
#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_pio(void);
#endif
#endif /* #ifndef USE_STI2C_WRAPPER */
static ST_ErrorCode_t Init_i2c(void);
static ST_ErrorCode_t Init_tunerparams(void);
static ST_ErrorCode_t Init_scanparams(void);
static ST_ErrorCode_t Init_evt(void);
static ST_ErrorCode_t Open_evt(void);
static ST_ErrorCode_t GlobalTerm(void);


/* Section: Simple */

static TUNER_TestResult_t TunerLock(TUNER_TestParams_t *Params_p);
#ifndef STTUNER_MINIDRIVER
static TUNER_TestResult_t TunerMeasure(TUNER_TestParams_t *Params_p);
#ifndef ST_OSLINUX
static TUNER_TestResult_t TunerAPI(TUNER_TestParams_t *Params_p);
#endif
static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p);

/****New test case added here *****************/
static TUNER_TestResult_t TunerReLock(TUNER_TestParams_t *Params_p) ;

static TUNER_TestResult_t TunerScan(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerTimedScan(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerScanExact(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerTimedScanExact(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerStandByMode(TUNER_TestParams_t *Params_p);

static TUNER_TestResult_t TunerTracking(TUNER_TestParams_t *Params_p);
#if !defined(TUNER_370VSB) && !defined(TUNER_372)
#ifdef TEST_HIERARCHY 
static TUNER_TestResult_t TunerHierarchy(TUNER_TestParams_t *Params_p);
#endif
#endif
#if !defined(TUNER_370VSB) && !defined(TUNER_372)
#ifdef TPS_CELLID_TEST
static TUNER_TestResult_t TunerTPSCellIdExtraction(TUNER_TestParams_t *Params_p);
#endif
#endif
static TUNER_TestResult_t TunerTerm(TUNER_TestParams_t *Params_p);
#if !defined(TUNER_370VSB ) && !defined(TUNER_372)
static TUNER_TestResult_t TunerScanVHFUHF(TUNER_TestParams_t *Params_p);
#endif
#ifdef TEST_CHANNEL_ZAP
static TUNER_TestResult_t TunerScanVHFUHFChannelZap(TUNER_TestParams_t *Params_p);
#endif
#ifdef TEST_DUAL
static TUNER_TestResult_t TunerDualScanStabilityTest(TUNER_TestParams_t *Params_p);/* New Function for Dual Scan Stablity test */
static TUNER_TestResult_t TunerDualScanSimultaneous(TUNER_TestParams_t *Params_p); /* New Function for Dual Scan */
#endif
#endif
#ifdef ST_7200
static ST_ErrorCode_t Reset_NIM(void);
#endif
/***#define for dual tuner test*******/


/*******Settings for VHF and UHF search in Mhz**************/
#define VHFBAND1_START    58
#define VHFBAND1_END      88
#define VHFBAND1_STEPSIZE 7

#define VHFBAND2_START    88
#define VHFBAND2_END      108
#define VHFBAND2_STEPSIZE 7

#define VHFBAND3_START    174
#define VHFBAND3_END      230
#define VHFBAND3_STEPSIZE 7

#define UHFBAND_START    470
#define UHFBAND_END      862
#define UHFBAND_STEPSIZE 8

/*************************************************/
/*************************************************/
#undef  CHANNEL_BANDWIDTH_6
#undef CHANNEL_BANDWIDTH_7
#define   CHANNEL_BANDWIDTH_8
/*************************************************/
/*i2c frequency for different backend boards*/
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
#define ST_I2C_CLOCKFREQUENCY  90000000
#elif defined(ST_7100)
#define ST_I2C_CLOCKFREQUENCY   58000000
#elif defined(ST_7109) || defined (ST_7200)
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

/* Test table */
static TUNER_TestEntry_t TestTable[] =
{
    #ifndef STTUNER_MINIDRIVER
    {TunerIoctl,           "Ioctl API 1.0.1 test ",                       1 },
    #ifndef ST_OSLINUX
   {TunerAPI,             "Test the stablility of the STTUNER API",      1 },
    #endif
   {TunerTerm,            "Termination during a scan",                   1 },
    #endif
    {TunerLock,            "Lock on a frequency",                         1 },
    #ifndef STTUNER_MINIDRIVER
    {TunerReLock,          "Relock on a particular channel",              1 },
    {TunerScan,            "Scan test to acquire channels in band",       1 },
    {TunerTimedScan,       "Speed test for a scan operation",             1 },
    {TunerScanExact,       "Scan exact through all acquired channels",    1 },
    {TunerTimedScanExact,  "Speed test for locking to acquired channels", 1 },   
    
    #ifdef TEST_DUAL
    { TunerDualScanStabilityTest,"Test Stability scan for two tuners  ",  1 },  
    { TunerDualScanSimultaneous,"Test Simultaneous scan for two tuners",  1 },
    #endif
    { TunerMeasure,        "Measure meantime to lock/unlock",             1 },
     #if !defined(TUNER_370VSB ) && !defined(TUNER_372)
    {TunerScanVHFUHF,       "To Scan Different Bands",                    1 },
     #endif
     #ifdef TEST_CHANNEL_ZAP
     {TunerScanVHFUHFChannelZap,  " For channel zap" ,			  1},	
     #endif
    {TunerTracking,        "Test tracking of a locked frequency",         1 },
  {TunerStandByMode,     "StandBy test",                                1},
#if !defined(TUNER_370VSB) && !defined(TUNER_372)    
    #ifndef ST_OSLINUX  
    #ifdef TEST_HIERARCHY 
    {TunerHierarchy,	   "Test for hierarchy modulation    "	,	0},
#endif  
#endif  
    #endif
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    #ifdef TPS_CELLID_TEST
    {TunerTPSCellIdExtraction, " Test TPS Cell Id Extraction ",           1 },
    #endif
    #endif
    #endif
    { 0,                   "",                                            0 }
};


/* Function definitions --------------------------------------------------- */

int main(int argc, char *argv[])
{
	#ifndef ST_OSLINUX
    ST_ErrorCode_t error = ST_NO_ERROR;
    #endif
    TUNER_TestResult_t GrandTotal, Result;
    TUNER_TestResult_t Total[NUM_TUNER];
    TUNER_TestEntry_t *Test_p;
    U32 Section;
    S32 tuners;
    TUNER_TestParams_t TestParams;
    #ifndef ST_OSLINUX  
    STBOOT_TermParams_t BootTerm;
    char str[100]; 
    #endif
    
    char temp1[50];/*max length of string describing LLA version=49+1(for '\0');total 5 demods,therefore 5*50=250*/
	char * LLARevision1=temp1;
	memset(LLARevision1,'\0',sizeof(temp1));
  
   
    debugmessage("\nTESTS ENVIRONMENT\n");
    debugmessage("Channel bandwidth  = 8MHz\n");
    debugmessage("FFT mode           = 8k\n");
    debugmessage("Guard              = 1/4\n");
    debugmessage("Constellation      = 64QAM\n");
    debugmessage("Code rate          = 1/2\n");
    debugmessage("Spectrum inversion = none\n");
    debugmessage("Frequency offset   = none\n\n");
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
#elif defined(ST_5100)
    strcpy(Backend,"STi5100");
#elif defined(ST_7710)
   strcpy(Backend, "STi7710");
#elif defined(ST_5528)
    strcpy(Backend,"STi5528");
#elif defined(ST_5105) || defined(ST_5107) || defined(ST_5188)
   strcpy(Backend,"STi5105");
   #elif defined(ST_7100)
   strcpy(Backend, "STi7100");
   #elif defined(ST_7109)
   strcpy(Backend, "STi7109");
   #elif defined(ST_7200)
   strcpy(Backend, "STi7200");
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
#elif defined(mb390)
    strcpy(Platform,"mb390");
#elif defined(mb391)
    strcpy(Platform,"mb391");
#elif defined(mb376)
    strcpy(Platform,"mb376");
#elif defined(espresso)
    strcpy(Platform,"espresso");
#elif defined(mb411)
    strcpy(Platform,"mb411");
#elif defined(mb519)
   strcpy(Platform, "mb519");
#elif defined(mb421)
    strcpy(Platform,"mb421");
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
    
     #ifdef ST_7200
   Reset_NIM();
   #endif

    debugmessage("Init_tunerparams() ");
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
    #ifndef ST_OSLINUX
    TUNER_DebugPrintf(("           STEVT:  %s\n",  STEVT_GetRevision()   ));
    #endif
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
    #ifndef STTUNER_MINIDRIVER
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    #ifdef TUNER_361
    strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0361)); 
    TUNER_DebugPrintf(("         361 LLA: %s\n",   LLARevision1 ));
    #elif defined (TUNER_362)
     strcpy(LLARevision1, (char *)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0362)); 
    TUNER_DebugPrintf(("         362 LLA: %s\n",   LLARevision1 ));
    #else
     strcpy(LLARevision1, (char*)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0360));
    TUNER_DebugPrintf(("         360 LLA: %s\n",  LLARevision1 ));
    #endif
    #endif
    #ifdef TUNER_370VSB
     strcpy(LLARevision1,(char*) STTUNER_GetLLARevision(STTUNER_DEMOD_STB0370VSB));
    TUNER_DebugPrintf(("         370VSB LLA: %s\n",   LLARevision1 ));
    #elif defined(TUNER_372)
    strcpy(LLARevision1, (char*)STTUNER_GetLLARevision(STTUNER_DEMOD_STV0372));
    TUNER_DebugPrintf(("         372 LLA: %s\n",   LLARevision1 ));
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
            TUNER_DebugPrintf(("                FAILED: %d\n", Total[tuners].NumberFailed));
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
    time = STOS_time_plus(STOS_time_now(), 20 * ST_GetClocksPerSecond());

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

time = STOS_time_plus(STOS_time_now(), 20 * ST_GetClocksPerSecond());

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
	
	#ifdef ST_OS21
	  BootParams.TimeslicingEnabled = TRUE;
	#endif 

#if defined(ST_5516) && defined(mb361)
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
    
#elif defined(ST_5516) && defined(mb382)
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
    
#elif defined(ST_5517) && defined(mb382)
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
        
#elif defined(ST_5514) && defined(mb314)
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
    
#elif defined(ST_5100) && defined(mb390)
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
    
#elif defined(ST_5518) && defined(mb5518)
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
#else 
    BootParams.ICacheEnabled = TRUE;
    BootParams.DCacheMap     = DCacheMap;
#endif

    BootParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BootParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    BootParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;

#if defined(mb275_64) || defined(mb5518) || defined(mb5518um)
    BootParams.MemorySize = STBOOT_DRAM_MEMORY_SIZE_64_MBIT;
#elif defined(ST_5528) || defined(ST_7710) || defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5100) || defined(ST_5105) || defined(ST_7100)|| defined(ST_7109)|| defined(ST_5301) || defined(ST_8010) || defined(ST_5107) || defined(ST_5188)
   BootParams.MemorySize = SDRAM_SIZE;
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
#ifndef USE_STI2C_WRAPPER
#ifndef ST_OSLINUX
static ST_ErrorCode_t Init_pio(void)
{
    ST_DeviceName_t DeviceName;
    ST_ErrorCode_t error = ST_NO_ERROR;
    U32 i;

    /* Automatically generate device names */
    for (i = 0; i < NUM_PIO; i++)       /* [0..NUM_PIO-1] */
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

#if !defined(ST_5512) && !defined(ST_5105) && !defined(ST_5107)&& !defined(ST_5188)
    PioInitParams[5].DriverPartition = (ST_Partition_t *)system_partition;
    PioInitParams[5].BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
    PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
    PioInitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
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
#endif /*OSLinux*/
#endif  /*USE_STI2C_WRAPPER*/
#ifndef USE_STI2C_WRAPPER
/* ------------------------------------------------------------------------- */


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
	TUNER_DebugMessage (("Can't open I2C device STI2C[2] for NIM reset\n"));
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
	TUNER_DebugMessage (("NIM Reset failed Error in I2C R/W\n"));
        }
     }

	Error=STI2C_Close (Handle);

        return (Error);
}
#endif 



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
    I2CInitParams[0].MaxHandles        = 8; /* 2 per stv0360 */
    I2CInitParams[0].DriverPartition   = system_partition;
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
    I2CInitParams[1].MaxHandles        = 8; /* 2 per stv0360 */
    I2CInitParams[1].DriverPartition   = system_partition;
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
#if defined(ST_5105) || defined(ST_5107)|| defined(ST_5188)
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
#else 

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
        TunerInitParams[i].Device = STTUNER_DEVICE_TERR;

        TunerInitParams[i].Max_BandList      = 10;
        TunerInitParams[i].Max_ThresholdList = 10;
        TunerInitParams[i].Max_ScanList      = 10;

        strcpy(TunerInitParams[i].EVT_DeviceName,"stevt");
	
	
        TunerInitParams[i].DriverPartition = system_partition;
#if !defined(TUNER_370VSB) && !defined(TUNER_372)
        /* STV0360 ter drivers */
        #ifdef TUNER_361
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0361;
        #endif
        #ifdef TUNER_362
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0362;
        #endif
        #ifdef TUNER_360
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0360;
        #endif
        #ifdef TUNER_361
        TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;
        #else
        TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;
        #endif
       
#endif
#ifdef   TUNER_370VSB 
	TunerInitParams[i].DemodType         = STTUNER_DEMOD_STB0370VSB;
	TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;	
#elif defined(TUNER_372)
        TunerInitParams[i].DemodType         = STTUNER_DEMOD_STV0372;
	TunerInitParams[i].TunerType         = TEST_TUNER_TYPE;		
 #endif
        
        TunerInitParams[i].DemodIO.Route     = STTUNER_IO_DIRECT;       /* I/O direct to hardware */
        TunerInitParams[i].DemodIO.Driver    = STTUNER_IODRV_I2C;

        if (i==0) TunerInitParams[i].DemodIO.Address =DEMOD_I2C_0;               /* stv0360 address */
        else      TunerInitParams[i].DemodIO.Address =DEMOD_I2C_1;               /* stv0360 address */

        TunerInitParams[i].TunerIO.Route     = STTUNER_IO_REPEATER;     /* use demod repeater functionality to pass tuner data */
        TunerInitParams[i].TunerIO.Driver    = STTUNER_IODRV_I2C;
        TunerInitParams[i].TunerIO.Address   = TUNER_I2C;               /* address for Tuner (e.g. VG1011 = 0xC0) */

       #ifdef ST_5188
        TunerInitParams[i].ExternalClock     = 30000000;
        #else
        TunerInitParams[i].ExternalClock     = 4000000;
         #endif
       
        TunerInitParams[i].TSOutputMode      = STTUNER_TS_MODE_PARALLEL;
        TunerInitParams[i].SerialClockSource = STTUNER_SCLK_DEFAULT;
        TunerInitParams[i].SerialDataMode    = STTUNER_SDAT_DEFAULT;
        #if !defined(TUNER_370VSB) && !defined(TUNER_372)
        TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_DVB;
        #else
        TunerInitParams[i].FECMode           = STTUNER_FEC_MODE_ATSC;
        #endif
        #if !defined(TUNER_370VSB) && !defined(TUNER_372)
        TunerInitParams[i].ClockPolarity     = STTUNER_DATA_CLOCK_POLARITY_RISING;
        #else
        TunerInitParams[i].ClockPolarity     =STTUNER_DATA_CLOCK_INVERTED;/*For 370/372 VSB it is different*/
        #endif
    }

    TunerDeviceName[i][0] = 0;

    strcpy(TunerInitParams[0].DemodIO.DriverName, I2CDeviceName[TUNER0_DEMOD_I2C]);       /* front I2C bus          */
    strcpy(TunerInitParams[0].TunerIO.DriverName, I2CDeviceName[TUNER0_TUNER_I2C]);       /* front I2C bus          */
    strcpy(TunerInitParams[1].DemodIO.DriverName, I2CDeviceName[TUNER0_DEMOD_I2C]);       /* front I2C bus          */
    strcpy(TunerInitParams[1].TunerIO.DriverName, I2CDeviceName[TUNER0_TUNER_I2C]);       /* front I2C bus          */

    strcpy(TunerInitParams[0].EVT_RegistrantName,EVT_RegistrantName1);
   #ifndef STTUNER_MINIDRIVER
    strcpy(TunerInitParams[1].EVT_RegistrantName,EVT_RegistrantName2);
    #endif 
    return(error);
}

/* ------------------------------------------------------------------------- */

static ST_ErrorCode_t Init_scanparams(void)
{
	
    ST_ErrorCode_t error = ST_NO_ERROR;
    /* Set tuner band list */
    Bands.NumElements = BAND_NUMELEMENTS ;
    #ifdef TEST_NOIDA_DTT_TRANSMISSION
    /*** BandList[0] is set for Noida terrestrial transmission ****/
    BandList[0].BandStart = 54000;
    BandList[0].BandEnd   = 858000;
    #else
    BandList[0].BandStart = 50000;
    BandList[0].BandEnd   = 800000;
    #endif
    #ifdef TEST_NOIDA_DTT_TRANSMISSION
    /*** BandList[0] is set for Noida terrestrial transmission ****/
    BandList[0].BandStart = 54000;
    BandList[0].BandEnd   = 858000;
    #else
    BandList[1].BandStart = 50000;
    BandList[1].BandEnd   = 800000;
    #endif

    /* Setup scan list */
    Scans.NumElements = SCAN_NUMELEMENTS;
    #ifdef TEST_NOIDA_DTT_TRANSMISSION
    /*** ScanList[0] is set for Noida terrestrial transmission ****/
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_ALL;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].Band         = 0;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[0].Mode         = STTUNER_MODE_8K;
    ScanList[0].Guard        = STTUNER_GUARD_1_4;
    ScanList[0].Force        = STTUNER_FORCE_NONE;
    ScanList[0].Hierarchy    = STTUNER_HIER_PRIO_ANY;
    #endif
    ScanList[0].Spectrum     = STTUNER_INVERSION_AUTO;
    ScanList[0].FreqOff      = STTUNER_OFFSET_NONE;
    ScanList[0].ChannelBW    = STTUNER_CHAN_BW_8M;/*STTUNER_CHAN_BW_8M;*//*STTUNER_CHAN_BW_7M;*/ /*STTUNER_CHAN_BW_6M;*/
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[0].EchoPos      = 0;
    #endif
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanList[0].SymbolRate   =  10762000 ;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    #else
    ScanList[0].FECRates     = STTUNER_FEC_ALL;
    ScanList[0].Modulation   = STTUNER_MOD_ALL;
    ScanList[0].AGC          = STTUNER_AGC_ENABLE;
    ScanList[0].Band         = 0;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[0].Mode         = STTUNER_MODE_2K;
    ScanList[0].Guard        = STTUNER_GUARD_1_4;
    ScanList[0].Force        = STTUNER_FORCE_NONE;
    ScanList[0].Hierarchy    = STTUNER_HIER_PRIO_ANY;
    #endif
    ScanList[0].Spectrum     = STTUNER_INVERSION_AUTO;
    ScanList[0].FreqOff      = STTUNER_OFFSET;
    ScanList[0].ChannelBW    = STTUNER_CHAN_BW_8M;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[0].EchoPos      = 0;
    #endif
    
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanList[0].SymbolRate   =  10762000 ;
    ScanList[0].IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    
    #endif

    #ifdef TEST_NOIDA_DTT_TRANSMISSION
    /*** ScanList[1] is set for Noida terrestrial transmission ****/
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_ALL;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].Band         = 0;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[1].Mode         = STTUNER_MODE_8K;
    ScanList[1].Guard        = STTUNER_GUARD_1_4;
    ScanList[1].Force        = STTUNER_FORCE_NONE;
    ScanList[1].Hierarchy    = STTUNER_HIER_NONE;
    #endif 
    ScanList[1].Spectrum     = STTUNER_INVERSION_AUTO;
    ScanList[1].FreqOff      = STTUNER_OFFSET_NONE;
    ScanList[1].ChannelBW    = STTUNER_CHAN_BW_8M/*STTUNER_CHAN_BW_8M*/;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[1].EchoPos      = 0;
    #endif
    
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanList[1].SymbolRate   =  10762000 ;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    
    #else
    ScanList[1].FECRates     = STTUNER_FEC_ALL;
    ScanList[1].Modulation   = STTUNER_MOD_ALL;
    ScanList[1].AGC          = STTUNER_AGC_ENABLE;
    ScanList[1].Band         = 0;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[1].Mode         = STTUNER_MODE_2K;
    ScanList[1].Guard        = STTUNER_GUARD_1_4;
    ScanList[1].Force        = STTUNER_FORCE_NONE;
    ScanList[1].Hierarchy    = STTUNER_HIER_NONE;
    #endif 
    ScanList[1].Spectrum     = STTUNER_INVERSION_AUTO;
    ScanList[1].FreqOff      = STTUNER_OFFSET;
    ScanList[1].ChannelBW    = STTUNER_CHAN_BW_8M;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[1].EchoPos      = 0;
    #endif 
    
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanList[1].SymbolRate   =  10762000 ;
    ScanList[1].IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    
    #endif
    #ifdef TEST_NOIDA_DTT_TRANSMISSION
    /*** ScanList[2] is set for Noida terrestrial transmission ****/
    ScanList[2].FECRates     = STTUNER_FEC_ALL;
    ScanList[2].Modulation   = STTUNER_MOD_ALL;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].Band         = 0;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[2].Mode         = STTUNER_MODE_8K;
    ScanList[2].Guard        = STTUNER_GUARD_1_4;
    ScanList[2].Force        = STTUNER_FORCE_NONE;
    ScanList[2].Hierarchy    = STTUNER_HIER_NONE;
    #endif 
    ScanList[2].Spectrum     = STTUNER_INVERSION_AUTO;
    ScanList[2].FreqOff      = STTUNER_OFFSET_NONE;
    ScanList[2].ChannelBW    = STTUNER_CHAN_BW_8M /*STTUNER_CHAN_BW_8M*/;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[2].EchoPos      = 0;
    #endif 
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanList[2].SymbolRate   =  10762000 ;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    #else
    ScanList[2].FECRates     = STTUNER_FEC_ALL;
    ScanList[2].Modulation   = STTUNER_MOD_ALL;
    ScanList[2].AGC          = STTUNER_AGC_ENABLE;
    ScanList[2].Band         = 0;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[2].Mode         = STTUNER_MODE_2K;
    ScanList[2].Guard        = STTUNER_GUARD_1_4;
    ScanList[2].Force        = STTUNER_FORCE_NONE;
    ScanList[2].Hierarchy    = STTUNER_HIER_NONE;
    #endif 
    ScanList[2].Spectrum     = STTUNER_INVERSION_AUTO;
    ScanList[2].FreqOff      = STTUNER_OFFSET;
    ScanList[2].ChannelBW    = STTUNER_CHAN_BW_8M;
    #if !defined(TUNER_370VSB)&& !defined(TUNER_372)
    ScanList[2].EchoPos      = 0;
    #endif 
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanList[2].SymbolRate   =  10762000 ;
    ScanList[2].IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif    
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

	EventSemaphore1=STOS_SemaphoreCreateFifoTimeOut(NULL,0);
	EventSemaphore2=STOS_SemaphoreCreateFifoTimeOut(NULL,0);    


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

#ifndef STTUNER_MINIDRIVER

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
#if !defined(TUNER_370VSB)&& !defined(TUNER_372)       
#ifdef TUNER_361
#define STV0361_REG_ID_INDEX    0
#elif defined (TUNER_362)
#define STV0362_REG_ID_INDEX    0
#else
#define STV0360_REG_ID_INDEX    0
#endif
 
#define START_I2C_ADDR      0x38
#define END_I2C_ADDR        0x3E
#endif

#ifdef TUNER_370VSB
#define STB0370VSB_REG_ID_INDEX  0
#define START_I2C_ADDR      0x18
#define END_I2C_ADDR        0x1E

#elif defined(TUNER_372)
#define STV0372_REG_ID_INDEX  0
#define START_I2C_ADDR      0x18
#define END_I2C_ADDR        0x1E
#endif








 
static TUNER_TestResult_t TunerIoctl(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t     Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t         Error, errc, errt;
    STTUNER_Handle_t       Handle;
    STTUNER_OpenParams_t   OpenParams;
    STTUNER_TermParams_t   TermParams;
    U32                    inparam, outparam; 
    #ifndef ST_OSLINUX
    U32                    original;
    #endif
    U32                    Test = 0;
    U8                     ID;
    TEST_ProbeParams_t     ProbeParams;
    U8                     address;

    /* ---------- PROBE FOR CHIP TYPE - TUNER UNINITALIZED ---------- */

    TUNER_DebugPrintf(("Test #%d.1\n", ++Test));
    TUNER_DebugMessage("Purpose: Probe to find chip types on I2C bus (STTUNER_IOCTL_PROBE)");
    TUNER_DebugMessage("         With tuner uninitalized.");

    /* look for a chip ID (register 0x00) at I2C address 0x38, 0x3A, 0x3C and 0x3E */
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

    for(address = START_I2C_ADDR; address <= END_I2C_ADDR; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(STTUNER_MAX_HANDLES, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(STTUNER_MAX_HANDLES)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n", ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0xF1)
            {
                case 0x10:
                    if (ProbeParams.Value == 0x10)
                    {
#if defined(TUNER_370VSB)
                    TUNER_DebugMessage("Chip is STV0370 cut 2.0");
#elif defined(TUNER_372)
		    TUNER_DebugMessage("Chip is STV0372 cut 1.0");
#endif
                    }
                    else 
                    {
                    TUNER_DebugMessage("Chip is STV0360 cut 1.0");
                    }
                    break;

                case 0x21:
                    TUNER_DebugMessage("Chip is STV0360 cut 2.1");
                    break;
                /**********Added for STV0361******************/
                case 0x30:
                    TUNER_DebugMessage("Chip is STV0361 cut 1.0");
                    break;
                 /**********Added for STV0362******************/
                case 0x40:
                    TUNER_DebugMessage("Chip is STV0362 cut 1.0");
                    break;
                case 0x41:
                    TUNER_DebugMessage("Chip is STV0362 cut 1.1");
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

    for(address = START_I2C_ADDR; address <= END_I2C_ADDR; address += 2)
    {
        ProbeParams.Address = address; /* address of chip */
        ProbeParams.Value   = 0;       /* clear value for return */
        Error = STTUNER_Ioctl(Handle, STTUNER_SOFT_ID, STTUNER_IOCTL_PROBE, &ProbeParams, NULL);
        TUNER_DebugError("STTUNER_Ioctl(Handle)", Error);
        if (Error == ST_NO_ERROR)
        {
            TUNER_DebugPrintf(("Address:0x%02x   ID:0x%02x\n",ProbeParams.Address, ProbeParams.Value));

            switch(ProbeParams.Value & 0xF1)
            {
                case 0x10:
                    if (ProbeParams.Value == 0x10)
                    {
#if defined(TUNER_370VSB)
                    TUNER_DebugMessage("Chip is STV0370 cut 2.0");
#elif defined(TUNER_372)
		    TUNER_DebugMessage("Chip is STV0372 cut 1.0");
#endif
                    }
                    else
                    {
                    TUNER_DebugMessage("Chip is STV0360 cut 1.0");
                    }
                    break;

                case 0x21:
                    TUNER_DebugMessage("Chip is STV0360 cut 2.1");
                    break;
                /**********Added for STV0361******************/
                case 0x30:
                    TUNER_DebugMessage("Chip is STV0361 cut 1.0");
                    break;
                /**********Added for STV0361******************/
                case 0x40:
                    TUNER_DebugMessage("Chip is STV0362 cut 1.0");
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

            switch(ProbeParams.Value & 0xF1)
           
            {
                case 0x10:
                    if (ProbeParams.Value == 0x10)
                    {
#if defined(TUNER_370VSB)
                    TUNER_DebugMessage("Chip is STV0370 cut 2.0");
#elif defined(TUNER_372)
		    TUNER_DebugMessage("Chip is STV0372 cut 1.0");
#endif
                    }
                    else
                    {
                    TUNER_DebugMessage("Chip is STV0360");
                    }
                    break;

                case 0x21:
                    TUNER_DebugMessage("Chip is STV0360 cut 2.1");
                    break;
                /**********Added for STV0361******************/
                case 0x30:
                    TUNER_DebugMessage("Chip is STV0361 cut 1.0");
                    break;
                /**********Added for STV0362******************/
                case 0x40:
                    TUNER_DebugMessage("Chip is STV0362 cut 1.0");
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

    /* ---------- GET STV0360 ID REGISTER VALUE ---------- */

    TUNER_DebugPrintf(("Test #%d\n", ++Test));
    TUNER_DebugPrintf(("Purpose: Read %s ID register (STTUNER_IOCTL_REG_IN)\n", ChipName));
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    #ifdef TUNER_361
    inparam = STV0361_REG_ID_INDEX; /* index to ID register see: 361_map.h */
    ID = 0x30;
    #elif defined (TUNER_362)
    inparam = STV0362_REG_ID_INDEX; /* index to ID register see: 362_map.h */
    ID = 0x40;
    #else
    inparam = STV0360_REG_ID_INDEX; /* index to ID register see: 360_map.h */
    ID = 0x21;
    #endif
    #endif
    #ifdef TUNER_370VSB
    inparam = STB0370VSB_REG_ID_INDEX; /* index to ID register see: 370VSB_map.h */
     ID = 0x10;
     #elif defined(TUNER_372)
     inparam = STV0372_REG_ID_INDEX; /* index to ID register see: 370VSB_map.h */
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

  /* ----------  USE RAW IO TO GET CHIP ID NUMBER  ---------- */

  /* IMPORTANT NOTE: This test must ALWAYS come after the 'modify SFRM register' test */


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

#endif
/*----------------------------------------------------------------------------
TunerLock()
Description:
    This routine locks to a frequency.

    We then call the abort routine to ensure we are notified of the
    unlocking of the tuner.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t TunerLock(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t      Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t          error;
    STTUNER_Handle_t        TUNERHandle;
    STTUNER_EventType_t     Event, ExpectedEvent;
    STTUNER_TunerInfo_t     TunerInfo;
    STTUNER_OpenParams_t    OpenParams;
    STTUNER_TermParams_t    TermParams;
    STTUNER_Scan_t          ScanParams;
    BOOL                    connect =FALSE;
    U32                     loop;
    U32                     exp,div,quo,rem,exp1;
    S32                     offset_type=0;
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
   
#ifdef TEST_CONFIGURE_IF_IQMODE 
    STTUNER_IF_IQ_Mode                   outparam;
    STTUNER_IF_IQ_Mode 			inparam=STTUNER_IQ_MODE_NORMAL;
#endif
   
#ifdef TEST_SET_30MHZ_REG_SET
U32                    outparam1;
STTUNER_demod_IOCTL_30MHZ_REG_t inparam1;
#endif
    /* Setup Tuner ------------------------- */

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return (Result);
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
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
    }


#ifdef TEST_CONFIGURE_IF_IQMODE    /*for configure IQ mode*/
outparam=0;
error = STTUNER_Ioctl(TUNERHandle, STTUNER_DEMOD_STV0362, STTUNER_IOCTL_SET_IF_IQMODE, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
    }
    outparam=0;
    error = STTUNER_Ioctl(TUNERHandle, STTUNER_DEMOD_STV0362, STTUNER_IOCTL_GET_IF_IQMODE, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", error);

    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
    }
	#endif
   
    
    
    /* Setup scan -------------------------- */

    STTUNER_SetBandList(TUNERHandle, &Bands);
    #ifndef STTUNER_MINIDRIVER
    STTUNER_SetScanList(TUNERHandle, &Scans);
    #endif
    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = STTUNER_FEC_1_2;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = STTUNER_MODE_8K;
    ScanParams.Guard      = STTUNER_GUARD_1_4;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = STTUNER_HIER_NONE;
    #endif 
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = STTUNER_OFFSET_NONE;
    ScanParams.FECRates   = STTUNER_FEC_1_2;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_6M;
    #endif
    #ifdef CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 
    #if defined(TUNER_370VSB) || defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif 
    /* Lock to a frequency ----------------- */

#ifdef TEST_SET_30MHZ_REG_SET    /*for register setting for 30MHZ crystal*/
inparam1.RPLLDIV=PLLNDIV_30MHZ ;    
inparam1.TRLNORMRATELSB=TRLNORMRATELSB_30MHZ ;
inparam1.TRLNORMRATELO= TRLNORMRATELO_30MHZ;
inparam1.TRLNORMRATEHI= TRLNORMRATEHI_30MHZ;
inparam1.INCDEROT1=INCDEROT1_30MHZ ;
inparam1.INCDEROT2=INCDEROT2_30MHZ ;

outparam1=0;
error = STTUNER_Ioctl(TUNERHandle, DUT_DEMOD, STTUNER_IOCTL_SET_30MHZ_REG, &inparam1, &outparam1);
    TUNER_DebugError("STTUNER_Ioctl()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
    }
#endif


    error = STTUNER_SetFrequency(TUNERHandle, TEST_FREQUENCY*1000, &ScanParams, 0);  /*gbgb*/
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
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
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));

    /* Check whether or not we're locked */
    
    if (Event != STTUNER_EV_LOCKED)
    {
        TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
        goto locking_end;
    }

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
    	rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
   #endif 
   #if defined(TUNER_370VSB) || defined(TUNER_372)
 exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
  #endif                         
    /* detect feed status ------------------ */

    for(loop = 1; loop <= RELOCK_TEST_NUMTIMES; loop++)
    {
        TUNER_DebugPrintf(("Pass %d of %d\n", loop, RELOCK_TEST_NUMTIMES));

        if (connect == FALSE)
        {
            TUNER_DebugMessage(("Please DISCONNECT terrestrial feed\n"));
            ExpectedEvent = STTUNER_EV_UNLOCKED;
            connect = TRUE;
        }
        else
        {
            TUNER_DebugMessage(("Please RECONNECT terrestrial feed\n" ));
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
            goto locking_end;
        }

        /* report on tuner status */
        error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

        exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
        exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
        div = PowOf10(exp1);
        quo = TunerInfo.BitErrorRate / div;
        rem = TunerInfo.BitErrorRate % div;
        div /= 100;                 /* keep only two numbers after decimal point */
         if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
        rem /= div;
	}
        if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
   #if !defined(TUNER_370VSB) && !defined(TUNER_372) 
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    }   /* for(loop) */
    #endif
    
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
     exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
                }/* for(loop) */

    
    #endif 

#ifdef STTUNER_MINIDRIVER
TUNER_TestPassed(Result);
#endif
    /* check freqency will unlock ---------- */
#ifndef STTUNER_MINIDRIVER
    error = STTUNER_Unlock(TUNERHandle);
    TUNER_DebugError("STTUNER_Unlock()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
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
#endif
    /* ------------------------------------- */


locking_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);


    return(Result);
}
#ifndef STTUNER_MINIDRIVER
/*----------------------------------------------------------------------------
TunerMeasure()
Description:
    This routine loops on locking to a good and a bad frequency and measures
    teh time need to lock or to find it cannot lock. It then computes the
    mean time and the variance.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
----------------------------------------------------------------------------*/
static TUNER_TestResult_t TunerMeasure(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t      Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t          error;
    STTUNER_Handle_t        TUNERHandle;
    STTUNER_EventType_t     Event;
    STTUNER_OpenParams_t    OpenParams;
    STTUNER_TermParams_t    TermParams;
    STTUNER_Scan_t          ScanParams;
#ifdef ST_OS21
    osclock_t                 time1,time2,time_diff;
    osclock_t                 locktime[105],unlocktime[105];
#else    
    clock_t                 time1,time2,time_diff;
    clock_t                 locktime[105],unlocktime[105];
#endif    
    U32                     Freq,Nbmillisec,i,ilock,iunlock;
    
    U32                     SX,QX,ns;

    /* Setup Tuner ------------------------- */

    error = STTUNER_Init(TunerDeviceName[Params_p->Tuner], &TunerInitParams[Params_p->Tuner]);
    TUNER_DebugError("STTUNER_Init()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Unable to initialize TUNER device");
        return (Result);
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
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto locking_end;
    }

    /* Setup scan -------------------------- */

    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = STTUNER_FEC_1_2;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = STTUNER_MODE_8K;
    ScanParams.Guard      = STTUNER_GUARD_1_4;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = STTUNER_HIER_NONE;
    #endif
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = STTUNER_OFFSET_NONE;
    ScanParams.FECRates   = STTUNER_FEC_1_2;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif 

    ilock   = 0;
    iunlock = 0;
    TUNER_DebugPrintf(("Try 100 times to lock on %d and 200 MHz\n",TEST_FREQUENCY));
    for (i=0; i<200; i++)
    {
        if ((i & 1) == 0)
        {
            Freq = TEST_FREQUENCY*1000;
            STTBX_Print(("."));            
        }
        else
        {
            Freq = 200000;       
        }
        
        time1 = STOS_time_now();       
        error = STTUNER_SetFrequency(TUNERHandle, Freq, &ScanParams, 0);
        if (Params_p->Tuner == 0)
        {
            AwaitNotifyFunction1(&Event);
        }
        else
        {
            AwaitNotifyFunction2(&Event);
        }
        time2      = STOS_time_now();
        time_diff  = STOS_time_minus(time2,time1);
        Nbmillisec = (time_diff * 1000 ) / ST_GetClocksPerSecond();

        if (Event == STTUNER_EV_LOCKED)   locktime[  ilock++] = Nbmillisec;
        else                            unlocktime[iunlock++] = Nbmillisec;
    }
    STTBX_Print(("\n"));

    SX = 0;
    ns = 0; 
                           /* nb of success */
    for (i=0; i<ilock; i++)
    {
        Nbmillisec = locktime[i];
        SX = (SX*ns + Nbmillisec*100) / (ns+1);
        ns++;
    }
    SX += 50;                           /* round value before to divide by 100 */
    SX /= 100;
    QX = 0;
    ns = 0;                             /* nb of success */
    for (i=0; i<ilock; i++)
    {
        Nbmillisec = abs(locktime[i] - SX);
        QX = (QX*ns + Nbmillisec*Nbmillisec) / (ns+1);
        ns++;
    }
    TUNER_DebugPrintf(("  Locked %d times, mean time = %dms, variance = %d\n", ilock, SX, QX));

    SX = 0;
    ns = 0;                             /* nb of success */
    for (i=0; i<iunlock; i++)
    {
        Nbmillisec = unlocktime[i];
        SX = (SX*ns + Nbmillisec*100) / (ns+1);
        ns++;
    }
    SX += 50;                           /* round value before to divide by 100 */
    SX /= 100;
    QX = 0;
    ns = 0;                             /* nb of success */
    for (i=0; i<iunlock; i++)
    {
        Nbmillisec = abs(unlocktime[i] - SX);
        QX = (QX*ns + Nbmillisec*Nbmillisec) / (ns+1);
        ns++;
    }
    TUNER_DebugPrintf(("Unlocked %d times, mean time = %dms, variance = %d\n", iunlock, SX, QX));

    TUNER_TestPassed(Result);

locking_end:

    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
}



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
    /* Try various API calls with an invalid handle */
    {
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
#ifdef ST_OSLINUX
	error = STTUNER_GetBandList(STTUNER_MAX_HANDLES, &BandList);
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
        error = STTUNER_GetScanList(STTUNER_MAX_HANDLES, &ScanList);
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
       
        BOOL failed = FALSE;
#ifndef ST_OSLINUX	        
        partition_status_t       status;
        partition_status_flags_t flags=0;
         S32 retval, before, after;
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
#endif
#endif

/* ------------------------------------------------------------------------- */
/* /\/\/\/\/\/\/\/\/\/\/\/\/\UTILITY Functions/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\ */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------ */
/* Debug routines --------------------------------------------------------- */
/* ------------------------------------------------------------------------ */


char *TUNER_ErrorString(ST_ErrorCode_t Error)
{
    TUNER_ErrorMessage *mp;

    mp = TUNER_ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return mp->ErrorMsg;
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
    static char Modulation[][25] =
    {
        "QPSK",
        "8PSK",
        "QAM",
        "16QAM",
        "32QAM",
        "64QAM",
        "128QAM",
        "256QAM"
    };

    for (i = 0; Mod > 0; i++, Mod>>=1)
        ;
    return Modulation[i-1];
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

        default:
                break;
    }
    i++;
    if (i >= NUM_TUNER) i = 0;
    sprintf(c[i], "NOT_STTUNER_EVENT(%d)", Evt);
    return(c[i]);
}

/* ------------------------------------------------------------------------ */

char *TUNER_AGCToPowerOutput(U8 Agc)
{
    static char buf[10];

    sprintf(buf, "%5d.", -(127 - (Agc / 2)));
    return strcat(buf, (Agc % 2) ? "0" : "5" );
}

/*-------------------------------------------------------------------------
 * Function : bit2num
 *            Converts bit to number for array selection
 * Input    : bit selected
 * Output   :
 * Return   : position of bit
 * ----------------------------------------------------------------------*/
static int bit2num( int bit )
{
    int i;
    for ( i = 0 ; i <= 16; i ++)
    {
        if (( 1 << i ) & bit )
            break;
    }

   /* return ++i;*/
      if(i==0)
      return (i+1);
      else
      return i;

} /* end bit2num */

/*****************************************************
**FUNCTION  ::  PowOf10
**ACTION    ::  Compute  10^n (where n is an integer)
**PARAMS IN	::	number -> n
**PARAMS OUT::	NONE
**RETURN    ::  10^n
*****************************************************/
int PowOf10(int number)
{
	int i;
    int result=1;

	for(i=0;i<number;i++)
        result*=10;

	return result;
}

/*****************************************************
**FUNCTION  ::  GetPowOf10
**ACTION    ::  Compute  x according to y with y=10^x
**PARAMS IN	::	number -> y
**PARAMS OUT::	NONE
**RETURN	::	x
*****************************************************/
long GetPowOf10(int number)
{
	int i=0;

    while(PowOf10(i)<number) i++;

	return i;
}

/*****************************************************
**FUNCTION  ::  GetPowOf10_BER
**ACTION    ::  Compute  x according to y with y=10^x taking
                into consideration in the low level driver
                BER value has been scaled up by 10^10 .
**PARAMS IN	::	number -> y
**PARAMS OUT::	NONE
**RETURN	::	x
*****************************************************/
long GetPowOf10_BER(int number)
{
	int i=0;

    while(PowOf10(i)<number) i++;
    if((i!=0) && (i>0))
    {
       #if !defined(TUNER_370VSB) && !defined(TUNER_372)
       i=(10-i)+1;
       #endif
       #if defined(TUNER_370VSB) ||defined(TUNER_372)
       i = (8 - i)+1;
       #endif
    }
    

	return i;
}

/*-------------------------------------------------------------------------
 * Function : TUNER_ModulationToString
 *            Convert Modulation value to text
 * Input    : Modulation value
 * Output   :
 * Return   : pointer to Modulation text
 * ----------------------------------------------------------------------*/
char *TUNER_ModulationToString(STTUNER_Modulation_t Modul)
{
    static char Modulation[][8] =
    {
        "ALL   ",
        "QPSK  ",
        "8PSK  ",
        "QAM   ",
        "16QAM ",
        "32QAM ",
        "64QAM ",
        "128QAM",
        "256QAM",
        "BPSK  ",
        "8VSB"
        
    };

    if ( Modul == STTUNER_MOD_ALL )
        return Modulation[0];
    else
        return Modulation[bit2num(Modul)];
      

} /* end TUNER_ModulationToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_ModeToString
 *            Convert Mode value to text
 * Input    : Mode value
 * Output   :
 * Return   : pointer to Mode text
 * ----------------------------------------------------------------------*/
char *TUNER_ModeToString(STTUNER_Mode_t Mode)
{
    if(Mode == STTUNER_MODE_2K)
    return "MODE_2K";
    else 
    return "MODE_8K";

} /* end TUNER_ModeToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_HierAlphaToString
 *            Convert Hierarchical Alpha value to text
 * Input    : Alpha value
 * Output   :
 * Return   : pointer to Alpha text
 * ----------------------------------------------------------------------*/
char *TUNER_HierAlphaToString(STTUNER_Hierarchy_Alpha_t Alpha)
{
    if (Alpha==STTUNER_HIER_ALPHA_1)
    {
       return "ALPHA_1";
    }
    else if (Alpha==STTUNER_HIER_ALPHA_2)
    {
       return "ALPHA_2";	
    }
    else if (Alpha==STTUNER_HIER_ALPHA_4)
    {
       return "ALPHA_4";	
    }
    else
    {
       return "ALPHA_NONE";
    }
} /* end TUNER_ModeToString */


/*-------------------------------------------------------------------------
 * Function : TUNER_SpectrumToString
 *            Convert Spectrum value to text
 * Input    : Spectrum value
 * Output   :
 * Return   : pointer to Spectru text
 * ----------------------------------------------------------------------*/
char *TUNER_SpectrumToString(STTUNER_Spectrum_t Spectrum)
{
    if(Spectrum == STTUNER_INVERSION)
    return "YES";
    else 
    return "NO";

} /* end TUNER_SpectrumToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_HierarchyToString
 *            Convert Hierarchy value to text
 * Input    : Hierarchy value
 * Output   :
 * Return   : pointer to Hierarchy text
 * ----------------------------------------------------------------------*/
char *TUNER_HierarchyToString(STTUNER_Hierarchy_t Hierarchy)
{
    if(Hierarchy == STTUNER_HIER_LOW_PRIO)
    return "HIERARCHY LP";
    else if((Hierarchy == STTUNER_HIER_HIGH_PRIO) || (Hierarchy == STTUNER_HIER_PRIO_ANY))/* ||(Hierarchy==STTUNER_HIER_PRIO_ANY))*/
    return "HIERARCHY HP";
   else 
    return "HIERARCHY NONE";

} /* end TUNER_SpectrumToString */

/*-------------------------------------------------------------------------
 * Function : TUNER_GuardToString
 *            Convert Spectrum value to text
 * Input    : Guard value
 * Output   :
 * Return   : pointer to Guard text
 * ----------------------------------------------------------------------*/
char *TUNER_GuardToString(STTUNER_Guard_t Guard)
{
    if(Guard == STTUNER_GUARD_1_32)
    return "GUARD_1_32";
    else if(Guard == STTUNER_GUARD_1_16) 
    return "GUARD_1_16";
    else if (Guard == STTUNER_GUARD_1_8)
    return "GUARD_1_8";
    else if (Guard ==STTUNER_GUARD_1_4)
    return "GUARD_1_4";
    else
    return "NO GUARD";  

} /* end TUNER_GuardToString */

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

#ifndef STTUNER_MINIDRIVER
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
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    STTUNER_Scan_t       ScanParams;
    U32 FStart, FEnd,FStepSize;
    U32                     exp,div,quo,rem,exp1;
    S32                     offset_type=0;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif

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
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize =((TEST_FREQUENCY*1000) - FStart)/2 ;

    /* Scan -------------------------------- */

    error = STTUNER_Scan(TUNERHandle, FStart,FEnd,FStepSize, 0);
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

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
    rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    
 #endif
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
     exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
     quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
                
     #endif
    
    /* Lock to a frequency ----------------- */

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = TunerInfo.ScanInfo.Mode;
    ScanParams.Guard      = TunerInfo.ScanInfo.Guard;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = TunerInfo.ScanInfo.Hierarchy;
    #endif
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 
    
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    
    
    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
   
    /*** Add the offset to get the exact central carrier frequency*****/
    TUNER_DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n",TunerInfo.Frequency,TunerInfo.ScanInfo.ResidualOffset,TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset));
    TunerInfo.Frequency=TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset;
    
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
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

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
   if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
    rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
                       
                        
   #endif 
   #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                        
                
                      
   
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
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd, FTmp , FStepSize;
    U32 ChannelCount = 0;
    U8 i;
    U32 exp,div,quo,rem,exp1;
    S32                     offset_type=0;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
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


    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);
    
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    

    for (i = 0; i < 2; i++)
    {
    	if(FStart < FEnd)
    	{
    	   FStepSize =((TEST_FREQUENCY*1000) - FStart)/2 ;
        }
    	else
    	{
    	   FStepSize = (FStart - (TEST_FREQUENCY*1000))/2 ;
        }
    	
        /* initiate scan */
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd,FStepSize, 0);
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
               /* Get tuner information */
               
                error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

                exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
                exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
                div = PowOf10(exp1);
                quo = TunerInfo.BitErrorRate / div;
                rem = TunerInfo.BitErrorRate % div;
                div /= 100;                 /* keep only two numbers after decimal point */
                 if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         	{
                rem /= div;
		}
                if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
   #endif                        
                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
     exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
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
            TUNER_DebugPrintf(("Scanned for next element in ScanList \n"));
            STTUNER_TaskDelay(10);
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


#if !defined(TUNER_370VSB ) && !defined(TUNER_372)
/*****************************************************************************
TunerScanVHFUHF()
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

static TUNER_TestResult_t TunerScanVHFUHF(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd,FStepSize=0;
    U32 ChannelCount ;
    U8 i,j;
    U32 exp,div,quo,rem,exp1;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
#ifdef ST_OS21
   U32 Time/* osclock_t Time*/;
#else    
    clock_t Time;
#endif    
    
    S32   offset_type=0;

    ChannelCount = 0;
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


    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);
    
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    
    TUNER_DebugMessage("Scanning to be done for European Bands");
    TUNER_DebugMessage("--------------------------------------");
    TUNER_DebugMessage("VHF BAND1 - VHF BAND2 - VHF BAND3 - UHF BAND 4 & 5");
    for (i = 0; i < 4; i++)
    {
    	  
    	  if(i==0)
    	  {
    	  	  TUNER_DebugMessage("SCANNING VHF BAND 1 -------------------");
    	     FStart=VHFBAND1_START*1000+((VHFBAND1_STEPSIZE*1000)/2); 
    	     FEnd=VHFBAND1_END*1000-((VHFBAND1_STEPSIZE*1000)/2);
    	     FStepSize=VHFBAND1_STEPSIZE*1000;
    	     if(VHFBAND1_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(VHFBAND1_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  } /*end of i==0 */
    	  else if(i==1)
    	  {
    	     TUNER_DebugMessage("SCANNING VHF BAND 2 -------------------");
    	     FStart=VHFBAND2_START*1000+((VHFBAND2_STEPSIZE*1000)/2); 
    	     FEnd=VHFBAND2_END*1000-((VHFBAND2_STEPSIZE*1000)/2);
    	     FStepSize=VHFBAND2_STEPSIZE*1000;
    	     if(VHFBAND2_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(VHFBAND2_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  }
    	  else if(i==2)
    	  {
    	     TUNER_DebugMessage("SCANNING VHF BAND 3 -------------------");
    	     FStart=VHFBAND3_START*1000+((VHFBAND3_STEPSIZE*1000)/2); 
    	     FEnd=VHFBAND3_END*1000-((VHFBAND3_STEPSIZE*1000)/2);
    	     FStepSize=VHFBAND3_STEPSIZE*1000;
    	     if(VHFBAND3_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(VHFBAND3_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  }
    	  else if(i==3)
    	  {
    	     TUNER_DebugMessage("SCANNING UHF BAND 4 & 5 -------------------");
    	     FStart=UHFBAND_START*1000+((UHFBAND_STEPSIZE*1000)/2); 
    	     FEnd=UHFBAND_END*1000-((UHFBAND_STEPSIZE*1000)/2);
    	     FStepSize=UHFBAND_STEPSIZE*1000;
    	     if(UHFBAND_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(UHFBAND_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  }
    	
    	  
        /* initiate scan */
        Time = STOS_time_now();
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd,FStepSize, 0);
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
               /* Get tuner information */
               
                error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

                exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
                exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
                div = PowOf10(exp1);
                quo = TunerInfo.BitErrorRate / div;
                rem = TunerInfo.BitErrorRate % div;
                div /= 100;                 /* keep only two numbers after decimal point */
                 if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         	{
                rem /= div;
		}
                if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                        
   #if defined(TUNER_370VSB) || defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                                
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
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
            STTUNER_TaskDelay(10);
            STTUNER_ScanContinue(TUNERHandle, 0);
            

}  /* for() */
       Time = (1000 * STOS_time_minus(STOS_time_now(), Time)) / ST_GetClocksPerSecond();
       
    /* Delay for a while and ensure we do not receive */
       TUNER_DebugPrintf(("Time taken = %u ms to scan band for %u channels.\n", (int)Time, ChannelCount));

        if (ChannelCount > 0)
        {
            TUNER_DebugPrintf(("ChannelCount = %u\n", ChannelCount));
            TUNER_TestPassed(Result);
        }
        else
        {
            TUNER_TestFailed(Result, "no channels found");
        }

        ChannelCount = 0;
    } /* for */


simple_end:
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);
        /********* RESET THE SCAN PARAMETERS****************/
        Init_scanparams();

    return Result;
} /* TunerScanVHFUHF() */
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
    U32 Time/*osclock_t Time*/;
#else    
    clock_t Time;
#endif    
    U32 ChannelCount ;
    U32 FStart, FEnd , FStepSize;

    ChannelCount = 0 ;
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
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;
    /* Now attempt a scan */
    TUNER_DebugMessage("Commencing scan");

    Time = STOS_time_now();
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd,FStepSize, 0);

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
#ifdef TUNER_362            
            ChannelTable[ChannelCount].ScanInfo.Spectrum = STTUNER_INVERSION_AUTO;
 #endif 

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
    S32    offset_type=0;
    U32 exp,div,quo,rem,exp1;
   #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
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
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
            if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
            rem /= div;
	}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                        
                        
                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
     exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =  %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                
             TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
            ChannelCount++;
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            
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
    U32 ChannelCount ;
#ifdef ST_OS21
    /*osclock_t*/ U32 StartClocks, TotalClocks;
#else    
    clock_t StartClocks, TotalClocks;
#endif    
    U32 i;
    ChannelCount = 0 ;
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
    U32 FStart, FEnd,FStepSize;
    U32 rem,div,exp,quo,exp1,i;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
    STTUNER_Scan_t       ScanParams;
    S32   offset_type=0; 
   
   #ifdef TEST_CONFIGURE_IF_IQMODE 
    STTUNER_IF_IQ_Mode                   outparam;
    STTUNER_IF_IQ_Mode 			inparam=STTUNER_IQ_MODE_NORMAL;
#endif
   
#ifdef TEST_SET_30MHZ_REG_SET
U32                    outparam1;
STTUNER_demod_IOCTL_30MHZ_REG_t inparam1;
#endif
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
#ifdef TEST_CONFIGURE_IF_IQMODE    /*for configure IQ mode*/
outparam=0;
error = STTUNER_Ioctl(TUNERHandle, STTUNER_DEMOD_STV0362, STTUNER_IOCTL_SET_IF_IQMODE, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }
    outparam=0;
    error = STTUNER_Ioctl(TUNERHandle, STTUNER_DEMOD_STV0362, STTUNER_IOCTL_GET_IF_IQMODE, &inparam, &outparam);
    TUNER_DebugError("STTUNER_Ioctl()", error);

    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }
	#endif
	
	#ifdef TEST_SET_30MHZ_REG_SET    /*for register setting for 30MHZ crystal*/
inparam1.RPLLDIV=PLLNDIV_30MHZ ;    
inparam1.TRLNORMRATELSB=TRLNORMRATELSB_30MHZ ;
inparam1.TRLNORMRATELO= TRLNORMRATELO_30MHZ;
inparam1.TRLNORMRATEHI= TRLNORMRATEHI_30MHZ;
inparam1.INCDEROT1=INCDEROT1_30MHZ ;
inparam1.INCDEROT2=INCDEROT2_30MHZ ;

outparam1=0;
error = STTUNER_Ioctl(TUNERHandle, DUT_DEMOD, STTUNER_IOCTL_SET_30MHZ_REG, &inparam1, &outparam1);
    TUNER_DebugError("STTUNER_Ioctl()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto tracking_end;
    }
#endif
    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Scan to the first frequency lock and let tracking run for a reasonable period.");

    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;
    /* Now attempt a scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
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
          if(error != ST_NO_ERROR)
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
             TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            /* goto tracking_end;*/
          }
         
         
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
         if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    
   /* TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s,Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
        */                
                        
     TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy)));
    
    
    TUNER_DebugPrintf(("Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    
                    
                        
                        
    #endif                        
                        
                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
     quo_snr = (TunerInfo.SignalQuality *36)/100;
     rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
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

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = TunerInfo.ScanInfo.Mode;
    ScanParams.Guard      = TunerInfo.ScanInfo.Guard;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = STTUNER_HIER_LOW_PRIO/*TunerInfo.ScanInfo.Hierarchy*/;
    #endif
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = STTUNER_OFFSET_NONE;/*TunerInfo.ScanInfo.FreqOff;*/
    ScanParams.FECRates   =  TunerInfo.ScanInfo.FECRates;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 

    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_INVERSION_AUTO;
    #endif

    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
   
    /*** Add the offset to get the exact central carrier frequency*****/
    TUNER_DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n",TunerInfo.Frequency,TunerInfo.ScanInfo.ResidualOffset,TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset ));
    TunerInfo.Frequency=TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset;
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
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

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    if(error != ST_NO_ERROR)
    {
       TUNER_DebugError("STTUNER_GetTunerInfo()", error);
       TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto tracking_end;
    }

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
     {
    rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
#if !defined(TUNER_370VSB) && !defined(TUNER_372)
		/*TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
		TunerInfo.Frequency,
		TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
		TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
		TUNER_StatusToString(TunerInfo.Status),
		TunerInfo.SignalQuality,
		TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
		TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
		TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
		TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
		offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
*/


 TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy)));
    
    
    TUNER_DebugPrintf(("Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    


#endif                        

                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
     quo_snr = (TunerInfo.SignalQuality *36)/100;
     rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
    if (Event == STTUNER_EV_LOCKED)
    {
        /* Begin polling driver for tracking tests... */
        TUNER_DebugMessage("Polling the tuner status -- ensure lock is kept...");

         for (i = 0;i <TUNER_TRACKING;i++)
         {
            STTUNER_TaskDelay(ST_GetClocksPerSecond());
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
            if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                goto tracking_end;
            }

         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
    
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
   /* TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
      */
      
  TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy)));
    
    
    TUNER_DebugPrintf(("Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
        
                       
#endif                        
                        
  #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
     quo_snr = (TunerInfo.SignalQuality *36)/100;
     rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                           
                       
#ifdef STTUNER_RF_LEVEL_MONITOR 
      	    /* RFLevel monitoring */
            TUNER_DebugPrintf(("\nRF level %ddBm \n",TunerInfo.RFLevel )); 
#endif 
           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                error = STTUNER_SetScanList(TUNERHandle, &Scans);
                 /* goto tracking_end;*/
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
} /* end of TunerTracking */






 /* end of TunerTracking */





 
 
 #if !defined(TUNER_370VSB) && !defined(TUNER_372)
 #ifndef ST_OSLINUX
 #ifdef TEST_HIERARCHY 
 /*****************************************************************************
TunerHierarchy()
Description:
    This routine checks whether tuner is in which hierarchy mode.
Parameters:
    Params_p,       pointer to test params block.
Return Value:
    TUNER_TestResult_t -- reflects the number of tests that pass/fail.
See Also:
    Nothing.
*****************************************************************************/

static TUNER_TestResult_t TunerHierarchy(TUNER_TestParams_t *Params_p)
{	
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd,FStepSize, i,k;
    U32 rem,div,exp,quo,exp1;
   
    STTUNER_Scan_t       ScanParams;
    S32   offset_type=0; 
   
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
        goto hierarchy_end;
    }

    /* Setup scan and band lists */
    STTUNER_SetBandList(TUNERHandle, &Bands);
    STTUNER_SetScanList(TUNERHandle, &Scans);

    TUNER_DebugPrintf(("%d.0: Track frequency\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Scan to the first frequency lock and let tracking run for a reasonable period.");

    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;
    /* Now attempt a scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
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
          if(error != ST_NO_ERROR)
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
             TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            /* goto tracking_end;*/
          }
         
         
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s,Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    #endif                        
                        
                        
     
                
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
            TUNER_TestFailed(Result, "Expected = 'STTUNER_EV_LOCKED'");
            goto hierarchy_end;
        }
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto hierarchy_end;
    }
    

    /* Lock to a frequency ----------------- */

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = TunerInfo.ScanInfo.Mode;
    ScanParams.Guard      = TunerInfo.ScanInfo.Guard;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = STTUNER_HIER_HIGH_PRIO;
    #endif
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = STTUNER_OFFSET_NONE;/*TunerInfo.ScanInfo.FreqOff;*/
    ScanParams.FECRates   =  TunerInfo.ScanInfo.FECRates;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 

    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_INVERSION_AUTO;
    #endif

    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
   
    /*** Add the offset to get the exact central carrier frequency*****/
    TUNER_DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n",TunerInfo.Frequency,TunerInfo.ScanInfo.ResidualOffset,TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset ));
    TunerInfo.Frequency=TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset;
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto hierarchy_end;
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

    /* remove----this parrtCheck whether or not we're locked */
    
    
    
    
    
    
    
    
    
    
    if (Event == STTUNER_EV_LOCKED)
    {
        /* Begin polling driver for tracking tests... */
        TUNER_DebugMessage("Polling the tuner status -- ensure lock is kept...");
	
	/************Test for HIGH priority******************/
	
	for(i=0;i<HIER_MAX;i++)
	{
            STTUNER_TaskDelay(ST_GetClocksPerSecond());
		           
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
           
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
             if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                goto hierarchy_end;
            }
            
                    
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}	
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
    
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
#endif                        
                        
             
                       
/*#ifdef STTUNER_RF_LEVEL_MONITOR 
      	  
            TUNER_DebugPrintf(("\nRF level %ddBm \n",TunerInfo.RFLevel )); 
#endif */
           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                error = STTUNER_SetScanList(TUNERHandle, &Scans);
                 
            }
            
            
            
            if (TunerInfo.ScanInfo.Hierarchy!=STTUNER_HIER_HIGH_PRIO) 
	{
	TUNER_TestFailed(Result,"Expected = 'STTUNER_HIGH PRIORITY'");
               goto hierarchy_end;
                	}
            
      
        }
        
      /***************Test for Low Priority******************/  
        
        ScanParams.Hierarchy  = STTUNER_HIER_LOW_PRIO;
        
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto hierarchy_end;
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


    
		
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
        
        
        
        
        for(i=0;i<HIER_MAX;i++)
	{
            STTUNER_TaskDelay(ST_GetClocksPerSecond());
		
           
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
           
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
             if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                goto hierarchy_end;
            }
            
            
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}	
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
    
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
#endif                        
                        
                   
                       
/*#ifdef STTUNER_RF_LEVEL_MONITOR 
      	    
            TUNER_DebugPrintf(("\nRF level %ddBm \n",TunerInfo.RFLevel )); 
#endif */
           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                error = STTUNER_SetScanList(TUNERHandle, &Scans);
                
            }
            
            
            
            if (TunerInfo.ScanInfo.Hierarchy!=STTUNER_HIER_LOW_PRIO) 
	{
	TUNER_TestFailed(Result,"Expected = 'STTUNER_LOW PRIORITY'");
               goto hierarchy_end;
                	}
            
      
        }
        
  /************Test for Any Priority******************************/      
        
        ScanParams.Hierarchy  =STTUNER_HIER_PRIO_ANY;

error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto hierarchy_end;
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


		
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
        
        
        
        
        for(i=0;i<HIER_MAX;i++)
	{
            STTUNER_TaskDelay(ST_GetClocksPerSecond());
		/*try1=TunerInfo.ScanInfo.Hierarchy;
		STTBX_Print("value of hier before get tuner  = %s",try1);*/
           
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
           
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
             if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                goto hierarchy_end;
            }
            
              
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}	
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
    
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
#endif                        
                        
                  
                       
/*#ifdef STTUNER_RF_LEVEL_MONITOR 
      	    
            TUNER_DebugPrintf(("\nRF level %ddBm \n",TunerInfo.RFLevel )); 
#endif */
           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                error = STTUNER_SetScanList(TUNERHandle, &Scans);
                
            }
            
            
            
            if (TunerInfo.ScanInfo.Hierarchy!=STTUNER_HIER_HIGH_PRIO) 
	{
	TUNER_TestFailed(Result,"Expected = 'STTUNER_ANY PRIORITY ");
               goto hierarchy_end;
                	}
            
     
        }
 
        /**********Test for None Priority************************/
        
      
       
        ScanParams.Hierarchy  =STTUNER_HIER_NONE;
        
        error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
    TUNER_DebugError("STTUNER_SetFrequency()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto hierarchy_end;
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


     
		
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
        
        
        
        
        for(i=0;i<HIER_MAX;i++)
	{
            STTUNER_TaskDelay(ST_GetClocksPerSecond());
		
           for(k=0;k<1;k++)
           {
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
           
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
             if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                goto hierarchy_end;
            }
            
              
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}	
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
    
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
#endif                        
                        
                
                       
/*#ifdef STTUNER_RF_LEVEL_MONITOR 
      	  
            TUNER_DebugPrintf(("\nRF level %ddBm \n",TunerInfo.RFLevel )); 
#endif */
           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                error = STTUNER_SetScanList(TUNERHandle, &Scans);
                 /* goto tracking_end;*/
            }
            
            
            
            if (TunerInfo.ScanInfo.Hierarchy!=STTUNER_HIER_HIGH_PRIO) 
	{
	TUNER_TestFailed(Result,"Expected = 'STTUNER_NONE PRIORITY'");
               goto hierarchy_end;
                	}
            
     } 
        }
  
      
        
        
   /************************************/      
        }
    else
    {
        TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
    }




    error = STTUNER_Unlock(TUNERHandle);
    TUNER_DebugError("STTUNER_Unlock()", error);

    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto hierarchy_end;
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

hierarchy_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
} /* end of TunerHierarchy */
#endif
#endif 
 #endif
 

 
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
    U32 FStart, FEnd,FStepSize;
   
    U32 rem,div,exp,quo,exp1;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
    STTUNER_Scan_t       ScanParams;
    S32   offset_type=0; 
   
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
    	TUNER_DebugPrintf(("\nFeature Not supported\n"));
    	goto standby_end;
    }
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        
    }
    else
    {
       TUNER_DebugPrintf(("\n\t\t***** StandByMode is switched ON ***** \n"));
    }
     /* Get tuner information */
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
   
       /* Get tuner information */
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
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize =((TEST_FREQUENCY*1000) - FStart)/2 ;

    /* Scan -------------------------------- */

    error = STTUNER_Scan(TUNERHandle, FStart,FEnd,FStepSize, 0);
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
    
    
    
     error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    
     if (error != ST_NO_ERROR)
    {
        TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
        goto standby_end;
    }
   
 TUNER_DebugPrintf(("TunerStatus: %s\n",TUNER_StatusToString(TunerInfo.Status)));
    
    

    TUNER_DebugMessage("Scanning...");
    
    
  /*  TUNER_DebugPrintf(("TunerStatus: %s\n", TUNER_StatusToString(TunerInfo.Status)));*/
    
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

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
    	rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    
 #endif
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
     exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
     quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
                
     #endif
    
    /* Lock to a frequency ----------------- */

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = TunerInfo.ScanInfo.Mode;
    ScanParams.Guard      = TunerInfo.ScanInfo.Guard;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = TunerInfo.ScanInfo.Hierarchy;
    #endif
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 
    
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    
    
    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
   
    /*** Add the offset to get the exact central carrier frequency*****/
    TUNER_DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n",TunerInfo.Frequency,TunerInfo.ScanInfo.ResidualOffset,TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset));
    TunerInfo.Frequency=TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset;
    
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
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

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
    {
    rem /= div;
   }
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
                       
                        
   #endif 
   #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                        
                    
   
      
       TunerInfo.ScanInfo.ResidualOffset = 0;
   
	/*** Trying to lock in StandBy Mode ***/
	
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
    
    /* Lock to a frequency ----------------- */

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    ScanParams.Mode       = TunerInfo.ScanInfo.Mode;
    ScanParams.Guard      = TunerInfo.ScanInfo.Guard;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = TunerInfo.ScanInfo.Hierarchy;
    #endif
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = /*TunerInfo.ScanInfo.FreqOff*/STTUNER_OFFSET_NONE;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 
    
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    ScanParams.SymbolRate   =  10762000 ;
    ScanParams.IQMode       = STTUNER_IQ_MODE_AUTO;
    #endif
    
   
    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
   
    /*** Add the offset to get the exact central carrier frequency*****/
    TUNER_DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n",TunerInfo.Frequency,TunerInfo.ScanInfo.ResidualOffset,TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset));
    TunerInfo.Frequency=TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset;
    
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
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
     TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
    
     if (error != ST_NO_ERROR)
    {
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
    	TUNER_DebugPrintf(("\n***** Standby Mode not working properly ***** \n "));
        TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_TIMEOUT'");
        goto standby_end;
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
    	 /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
    	rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
   
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
                     
                        
   #endif 
   #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
    
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
          
   #endif    
}

standby_end:
    error = STTUNER_Close(TUNERHandle);
    TUNER_DebugError("STTUNER_Close()", error);

    TermParams.ForceTerminate = FALSE;
    error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
    TUNER_DebugError("STTUNER_Term()", error);

    return(Result);
}
/* end of TunerStandbyMode */

 #if !defined(TUNER_370VSB) && !defined(TUNER_372)
 #ifdef TPS_CELLID_TEST
/*****************************************************************************
TunerTPSCellIdExtraction()
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

static TUNER_TestResult_t TunerTPSCellIdExtraction(TUNER_TestParams_t *Params_p)
{	
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd,FStepSize, i;
    U32 rem,div,exp,quo,exp1;
    STTUNER_Scan_t       ScanParams;
    S32   offset_type=0; 
    U16 TPSCellId=0;
    clock_t                 time1,time2,time_diff;
    U32 Millisecond =0;
    
   
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
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;
    /* Now attempt a scan */
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
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
          if(error != ST_NO_ERROR)
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
             TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            /* goto tracking_end;*/
          }
         
         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }

         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
          
          time1 = time_now();
         /*******TPSCELLID check*************/
        error= STTUNER_GetTPSCellId(TUNERHandle,&TPSCellId);
        time2 = STOS_time_now();
        time_diff  = STOS_time_minus(time2,time1);
        Millisecond = (time_diff * 1000 ) / ST_GetClocksPerSecond();
        if(error == ST_NO_ERROR)
        {
           TUNER_DebugPrintf(("\n *******The time taken by TPSCellID extraction is <%d msec> with Cell id <%x> after successful SCANNING***\n",Millisecond,TPSCellId));	
        }
        else
        {
           TUNER_DebugPrintf(("\n TIMEOUT ERROR\n")); 
        }
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

    ScanParams.Band       = 0;
    ScanParams.AGC        = 1;
    ScanParams.FECRates   = TunerInfo.ScanInfo.FECRates/*STTUNER_FEC_1_2*/;
    ScanParams.Mode       = TunerInfo.ScanInfo.Mode;
    ScanParams.Guard      = TunerInfo.ScanInfo.Guard;
    ScanParams.Force      = STTUNER_FORCE_NONE;
    ScanParams.Hierarchy  = TunerInfo.ScanInfo.Hierarchy;
    ScanParams.Spectrum   = STTUNER_INVERSION_AUTO;
    ScanParams.FreqOff    = STTUNER_OFFSET_NONE;/*TunerInfo.ScanInfo.FreqOff;*/
    ScanParams.FECRates   =  TunerInfo.ScanInfo.FECRates;
    #ifdef CHANNEL_BANDWIDTH_6
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_6M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_7
    ScanParams.ChannelBW  =STTUNER_CHAN_BW_7M;
    #endif
    #ifdef  CHANNEL_BANDWIDTH_8
    ScanParams.ChannelBW  = STTUNER_CHAN_BW_8M;
    #endif 

    TUNER_DebugMessage("Tuning...");

    /* We locked due to a scan so the relocking functionality is not enabled, to
    do this we need to do a STTUNER_SetFrequency() to the current frequency. */
   
    /*** Add the offset to get the exact central carrier frequency*****/
    TUNER_DebugPrintf(("Frequency set Frequency %d + Offset %d = %d\n",TunerInfo.Frequency,TunerInfo.ScanInfo.ResidualOffset,TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset ));
    TunerInfo.Frequency=TunerInfo.Frequency+TunerInfo.ScanInfo.ResidualOffset;
    error = STTUNER_SetFrequency(TUNERHandle,TunerInfo.Frequency, &ScanParams, 0);
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

    /* Get tuner information */
    error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
    if(error != ST_NO_ERROR)
    {
       TUNER_DebugError("STTUNER_GetTunerInfo()", error);
       TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
       goto tracking_end;
    }

    exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
    exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
    div = PowOf10(exp1);
    quo = TunerInfo.BitErrorRate / div;
    rem = TunerInfo.BitErrorRate % div;
    div /= 100;                 /* keep only two numbers after decimal point */
     if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
    	rem /= div;
	}
    if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
    if (Event == STTUNER_EV_LOCKED)
    {
        /* Begin polling driver for tracking tests... */
        TUNER_DebugMessage("Polling the tuner status -- ensure lock is kept...");
        
        for (i = 0;i < TUNER_TPS_COUNTER;i++)
        {
        STTUNER_TaskDelay(ST_GetClocksPerSecond());
        
        time1 = STOS_time_now();
         /*******TPSCELLID check*************/
        error= STTUNER_GetTPSCellId(TUNERHandle,&TPSCellId);
        time2 = STOS_time_now();
        time_diff  = STOS_time_minus(time2,time1);
        Millisecond = (time_diff * 1000 ) / ST_GetClocksPerSecond();
        if(error == ST_NO_ERROR)
        {
           
           TUNER_DebugPrintf(("\n *******The time taken by TPSCellID extraction is <%d msec> with Cell id <%x> after successful TUNNING***\n",Millisecond,TPSCellId));	
          
        }
        else
        {
           TUNER_DebugPrintf(("\n TIMEOUT ERROR\n")); 
        }
       
            error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
            TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
            if (error != ST_NO_ERROR)
            {
                TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                /*goto tracking_end;*/
            }

         exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
	}
         if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
    
     
    
   TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
   
                       
#ifdef STTUNER_RF_LEVEL_MONITOR 
      	    /* RFLevel monitoring */
            TUNER_DebugPrintf(("\nRF level %ddBm \n",TunerInfo.RFLevel )); 
#endif 
           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
               /* goto tracking_end;*/
            }
            
        }/* end of for loop*/
        
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
} /* end of TunerTPSCellIdExtraction */
#endif
#endif
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
    U32 FStart=0, FEnd=0 , FStepSize=0;
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

    TUNER_DebugPrintf(("%d.1: Check Band and Scan lists have to be set before a scan.\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Ensure no scan if band and scan lists not set.");


    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
   
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error == ST_ERROR_BAD_PARAMETER)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_ERROR_BAD_PARAMETER'");
    }
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;
    /* attempt a scan */
    TUNER_DebugMessage("Commencing scan...");
 

    TUNER_DebugPrintf(("%d.2: Terminate during scan\n", Params_p->Ref));
    TUNER_DebugMessage("Purpose: Ensure driver terminates cleanly when scanning");

  /*  error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
    TUNER_DebugError("STTUNER_Scan()", error);
    if (error == ST_ERROR_BAD_PARAMETER)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result,"Expected = 'ST_ERROR_BAD_PARAMETER'");
    }*/
    /* Setup scan and band lists */

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);
 
    /* attempt another scan */
    TUNER_DebugMessage("Commencing scan...");
    error = STTUNER_Scan(TUNERHandle, FStart, FEnd, FStepSize, 0);
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
 /*   if (Params_p->Tuner == 0)
    {
        AwaitNotifyFunction1(&Event);
    }
    else
    {
        AwaitNotifyFunction2(&Event);
    }
    TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));*/

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

    STTUNER_TaskDelay( ST_GetClocksPerSecond() * 1);

    return(Result);

} /* TunerTerm() */

#ifdef TEST_DUAL
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
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif

    U32 FStart, FEnd ,FStepSize,exp ,div ,quo ,rem,exp1 ;
    S32     offset_type=0;
    Init_scanparams();
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;

    TermParams1.ForceTerminate = FALSE;
    TermParams2.ForceTerminate = FALSE;

/* Initialize Tuner1 */
 #ifdef TUN1
    error = STTUNER_Init(TunerDeviceName[0], &TunerInitParams[0]);
    TUNER_DebugError("STTUNER_Init(0)", error);
 #endif

/* Initialize Tuner2 */
 #ifdef TUN2
    error = STTUNER_Init(TunerDeviceName[1], &TunerInitParams[1]);
    TUNER_DebugError("STTUNER_Init(1)", error);
 #endif



/* Open Tuner1 */
 #ifdef TUN1
    OpenParams.NotifyFunction = CallbackNotifyFunction1;
    error = STTUNER_Open(TunerDeviceName[0], &OpenParams, &TUNERHandle1);
    TUNER_DebugError("STTUNER_Open(0)", error);
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
    error = STTUNER_SetScanList(TUNERHandle2, &Scans);
    TUNER_DebugError("STTUNER_SetScanList(1)", error);
 #endif


/* Start Scanning for Tuner2 */
#ifdef TUN2
    error = STTUNER_Scan(TUNERHandle2, FStart, FEnd,FStepSize, 0);
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
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         	{
            	rem /= div;
		}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
#endif                        
                        
                        
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =  %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif      
                
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event2)));
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
  /*  error = STTUNER_Scan(TUNERHandle2, FStart, FEnd, 0, 0);
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
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         	{
            	rem /= div;
		}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
#endif                        
                        
   #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event2)));
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
    error = STTUNER_Scan(TUNERHandle1, FStart, FEnd,FStepSize, 0);
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
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         	{
            	rem /= div;
		}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
#endif                        
                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event1)));
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

}/* End of TunerDualScanStabilityTest */


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
    STTUNER_TunerInfo_t  TunerInfo;
    STTUNER_TermParams_t TermParams1, TermParams2;
    U32 FStart, FEnd, i ,FStepSize,exp ,div ,quo ,rem,exp1;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
    S32   offset_type=0;
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    FStepSize = ((TEST_FREQUENCY*1000) - FStart)/2 ;
   
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
    error = STTUNER_SetScanList(TUNERHandle2, &Scans);
    TUNER_DebugError("STTUNER_SetScanList(1)", error);
 #endif


#ifdef TUN1
    error = STTUNER_Scan(TUNERHandle1, FStart, FEnd,FStepSize, 0);
    TUNER_DebugError("STTUNER_Scan(0)", error);
#endif




#ifdef TUN1
      AwaitNotifyFunction1(&Event1);
      TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));

        if (Event1 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle1, &TunerInfo);
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         	{
            	rem /= div;
		}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
#endif                        
   
   #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));;
   #endif   
        }
        else
        {
            TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
/*            TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));
            TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");
*/
        }
#endif

#ifdef TUN2
    error = STTUNER_Scan(TUNERHandle2, FStart, FEnd,FStepSize, 0);
    TUNER_DebugError("STTUNER_Scan(1)", error);
#endif
#ifdef TUN2
      AwaitNotifyFunction2(&Event2);
      TUNER_DebugPrintf(("(2)Event = %s.\n", TUNER_EventToString(Event2)));

        if (Event2 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
            rem /= div;
	}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
    #endif                        
                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                       
        }
        else
        {
            TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/

        }

#endif

/* Check for 30 times */
for(i = 0; i <30; i++)
{

 #ifdef TUN2
        TUNER_DebugPrintf(("(2)Event = %s.\n", TUNER_EventToString(Event2)));
        if (Event2 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle2, &TunerInfo);
            if(error != ST_NO_ERROR)
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
             TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            /* goto tracking_end;*/
          }
          else
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
          }
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
            rem /= div;
	}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
#endif                        
                        
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
     exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
        }
        else
        {
	    TUNER_DebugPrintf(("Result: !!!! FAIL !!!! Expected = 'STTUNER_EV_LOCKED'\n"));
           /* TUNER_TestFailed(Result,"Expected = 'STTUNER_EV_LOCKED'");*/
        }

#endif
#ifdef TUN1

        TUNER_DebugPrintf(("(1)Event = %s.\n", TUNER_EventToString(Event1)));
        if (Event1 == STTUNER_EV_LOCKED)
        {
            /* Get status information */
            error = STTUNER_GetTunerInfo(TUNERHandle1, &TunerInfo);
            if(error != ST_NO_ERROR)
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
             TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
            /* goto tracking_end;*/
          }
          else
          {
             TUNER_DebugError("STTUNER_GetTunerInfo()", error);
          }
            
            exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
            exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
            div = PowOf10(exp1);
            quo = TunerInfo.BitErrorRate / div;
            rem = TunerInfo.BitErrorRate % div;
            div /= 100;                 /* keep only two numbers after decimal point */
             if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
            rem /= div;
	}
            if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
	#endif                        
                        
     #if defined(TUNER_370VSB) ||defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
     quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%(%u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif  

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
#endif

#ifdef TUN2
    error = STTUNER_Unlock(TUNERHandle2);
    TUNER_DebugError("STTUNER_Unlock(2)", error);
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
#endif


#ifdef TEST_CHANNEL_ZAP
/*****************************************************************************
TunerScanVHFUHFChannelZap()
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

static TUNER_TestResult_t TunerScanVHFUHFChannelZap(TUNER_TestParams_t *Params_p)
{
    TUNER_TestResult_t Result = TEST_RESULT_ZERO;
    ST_ErrorCode_t error;
    STTUNER_Handle_t TUNERHandle;
    STTUNER_OpenParams_t OpenParams;
    STTUNER_EventType_t Event;
    STTUNER_TunerInfo_t TunerInfo;
    STTUNER_TermParams_t TermParams;
    U32 FStart, FEnd,FStepSize;
    U8 i,j,k;
    U32 exp,div,quo,rem,exp1;
    #if defined(TUNER_370VSB) ||defined(TUNER_372)
    U32 quo_snr,rem_snr;
    #endif
#ifdef ST_OS21
   U32 Time/* osclock_t Time*/;
#else    
    clock_t Time;
#endif    
    
    S32   offset_type=0;
   /*************/
   /* Stored table of channels */
   TUNER_Channel_t ChannelTable[TUNER_TABLE_SIZE];
   U32 ChannelCount,ChannelTableCount;
               /* Increment channel count */
  
   /**************/
    ChannelCount = 0;
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


    TUNER_DebugMessage("Purpose: Check that a scan across a range works correctly.");

    error = STTUNER_SetBandList(TUNERHandle, &Bands);
    TUNER_DebugError("STTUNER_SetBandList()", error);

    error = STTUNER_SetScanList(TUNERHandle, &Scans);
    TUNER_DebugError("STTUNER_SetScanList()", error);
    
    FStart = FLIMIT_LO;
    FEnd   = FLIMIT_HI;
    
    TUNER_DebugMessage("Scanning to be done for European Bands");
    TUNER_DebugMessage("--------------------------------------");
    TUNER_DebugMessage("VHF BAND1 - VHF BAND2 - VHF BAND3 - UHF BAND 4 & 5");
    for (i = 0; i < 4; i++)
    {
    	  
    	  if(i==0)
    	  {
    	  	  TUNER_DebugMessage("SCANNING VHF BAND 1 -------------------");
    	     FStart=VHFBAND1_START*1000+((VHFBAND1_STEPSIZE*1000)/2); 
    	     FEnd=VHFBAND1_END*1000-((VHFBAND1_STEPSIZE*1000)/2);
    	     FStepSize=VHFBAND1_STEPSIZE*1000;
    	     if(VHFBAND1_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(VHFBAND1_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  } /*end of i==0 */
    	  else if(i==1)
    	  {
    	     TUNER_DebugMessage("SCANNING VHF BAND 2 -------------------");
    	     FStart=VHFBAND2_START*1000+((VHFBAND2_STEPSIZE*1000)/2); 
    	     FEnd=VHFBAND2_END*1000-((VHFBAND2_STEPSIZE*1000)/2);
    	     FStepSize=VHFBAND2_STEPSIZE*1000;
    	     if(VHFBAND2_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(VHFBAND2_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  }
    	  else if(i==2)
    	  {
    	     TUNER_DebugMessage("SCANNING VHF BAND 3 -------------------");
    	     FStart=VHFBAND3_START*1000+((VHFBAND3_STEPSIZE*1000)/2); 
    	     FEnd=VHFBAND3_END*1000-((VHFBAND3_STEPSIZE*1000)/2);
    	     FStepSize=VHFBAND3_STEPSIZE*1000;
    	     if(VHFBAND3_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(VHFBAND3_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  }
    	  else if(i==3)
    	  {
    	     TUNER_DebugMessage("SCANNING UHF BAND 4 & 5 -------------------");
    	     FStart=UHFBAND_START*1000+((UHFBAND_STEPSIZE*1000)/2); 
    	     FEnd=UHFBAND_END*1000-((UHFBAND_STEPSIZE*1000)/2);
    	     FStepSize=UHFBAND_STEPSIZE*1000;
    	     if(UHFBAND_STEPSIZE==6)
    	     {
    	     	  for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_6M;
    	        }
    	     }
    	     else if(UHFBAND_STEPSIZE==7)
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_7M;
    	        }	
    	     }
    	     else
    	     {
    	        for(j=0;j<SCAN_NUMELEMENTS;j++)
    	     	  {
    	           Scans.ScanList[j].ChannelBW=STTUNER_CHAN_BW_8M;
    	        }
    	     }
    	     error = STTUNER_SetScanList(TUNERHandle, &Scans);
           TUNER_DebugError("STTUNER_SetScanList()", error);
    	  }
    	
    	  
        /* initiate scan */
        Time = time_now();
        error = STTUNER_Scan(TUNERHandle, FStart, FEnd,FStepSize, 0);
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
               /* Get tuner information */
               
                error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
     /*****/
	  /* Store channel information in channel table */
            ChannelTable[ChannelCount].F = TunerInfo.Frequency;
            memcpy(&ChannelTable[ChannelCount].ScanInfo, &TunerInfo.ScanInfo, sizeof(STTUNER_Scan_t) );
#ifdef TUNER_362            
            ChannelTable[ChannelCount].ScanInfo.Spectrum = STTUNER_INVERSION_AUTO;
 #endif 
 ChannelCount++;
 /******/
                exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
                exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
                div = PowOf10(exp1);
                quo = TunerInfo.BitErrorRate / div;
                rem = TunerInfo.BitErrorRate % div;
                div /= 100;                 /* keep only two numbers after decimal point */
                 if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
                rem /= div;
	}
                if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
    {
       offset_type=1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
    {
       offset_type=-1;
    }
    else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    {
       offset_type=0;
    }
     
    
     #if !defined(TUNER_370VSB) && !defined(TUNER_372)
    TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),
                        TunerInfo.SignalQuality,
                        TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                        TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                        TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                        offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                        
   #if defined(TUNER_370VSB) || defined(TUNER_372)
      exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
         exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
         div = PowOf10(exp1);
         quo = TunerInfo.BitErrorRate / div;
         rem = TunerInfo.BitErrorRate % div;
         div /= 100;                 /* keep only two numbers after decimal point */
          if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         {
         rem /= div;
        }
      quo_snr = (TunerInfo.SignalQuality *36)/100;
      rem_snr = (TunerInfo.SignalQuality *36)%100;
      
      TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        TunerInfo.Frequency,
                        TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                        quo_snr,rem_snr,
                        TunerInfo.ScanInfo.IQMode,
                        TunerInfo.BitErrorRate,quo,rem,exp));
   #endif   
                                
                TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));
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
            STTUNER_TaskDelay(10);
            STTUNER_ScanContinue(TUNERHandle, 0);
            

}  /* for() */
       Time = (1000 * time_minus(time_now(), Time)) / ST_GetClocksPerSecond();
       
    /* Delay for a while and ensure we do not receive */
       TUNER_DebugPrintf(("Time taken = %u ms to scan band for %u channels.\n", Time, ChannelCount));

        if (ChannelCount > 0)
        {
            TUNER_DebugPrintf(("ChannelCount = %u\n", ChannelCount));
            TUNER_TestPassed(Result);
        }
        else
        {
            TUNER_TestFailed(Result, "no channels found");
        }

      /*  ChannelCount = 0;*/
    } /* for */
ChannelTableCount=ChannelCount;
ChannelCount=0;
/****Channel zapping **/

if (ChannelTableCount < 1)
    {
        TUNER_TestFailed(Result,"Cannot perform test with no channels found");
        return(Result);
    }
    
    
    for (k=0;k<50;k++){
      for (j = 0; j < ChannelTableCount; j++)
    {
    	
        STTUNER_SetFrequency(TUNERHandle, ChannelTable[j].F, &ChannelTable[j].ScanInfo, 0);
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
        	 
          TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
          	
            ChannelCount++;
            /****/
            for (i = 0;i </*TUNER_TRACKING*/20;i++)
         			{
            		 STTUNER_TaskDelay(ST_GetClocksPerSecond());

                 error = STTUNER_GetTunerInfo(TUNERHandle, &TunerInfo);
                 TUNER_DebugError("STTUNER_GetTunerInfo()", error);
            
                 if (error != ST_NO_ERROR)
                   {
                      TUNER_TestFailed(Result,"Expected = 'ST_NO_ERROR'");
                      goto simple_end;
           				 }

        				 exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
        				 exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
        				 div = PowOf10(exp1);
        				 quo = TunerInfo.BitErrorRate / div;
       					 rem = TunerInfo.BitErrorRate % div;
        				 div /= 100;                 /* keep only two numbers after decimal point */
        				  if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
         				{
         				 rem /= div;
					}
        				 if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_POSITIVE)
   								 {
  							     offset_type=1;
 								   }
  						  else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NEGATIVE)
   								 {
   								    offset_type=-1;
    								}
   						 else if(TunerInfo.ScanInfo.FreqOff == STTUNER_OFFSET_NONE)
    							{
     								  offset_type=0;
   								 }
    
     
#if !defined(TUNER_370VSB) && !defined(TUNER_372)
   				 TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality = %u%%,Mode = %s, Hierachy = %s, Alpha Factor = %s, SpectrumInversion = %s , Frequency Offset Type = %d , Frequency Offset = %d Khz, Guard = %s ,BER (%d) = %d.%dx10**-%d\n",
                       			 TunerInfo.Frequency,
                        		 TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                   			     TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                       			 TUNER_StatusToString(TunerInfo.Status),
                       			 TunerInfo.SignalQuality,
                      		  TUNER_ModeToString(TunerInfo.ScanInfo.Mode),
                      		  TUNER_HierarchyToString(TunerInfo.ScanInfo.Hierarchy),
                      		  TUNER_HierAlphaToString(TunerInfo.Hierarchy_Alpha),
                      		  TUNER_SpectrumToString(TunerInfo.ScanInfo.Spectrum),
                      		  offset_type,TunerInfo.ScanInfo.ResidualOffset,TUNER_GuardToString(TunerInfo.ScanInfo.Guard),TunerInfo.BitErrorRate,quo,rem,exp));
                        
#endif                        
                        
  #if defined(TUNER_370VSB) ||defined(TUNER_372)
      						exp = (U32)GetPowOf10_BER(TunerInfo.BitErrorRate);
        					exp1 = (U32)GetPowOf10(TunerInfo.BitErrorRate) - 1;
        				  div = PowOf10(exp1);
         					quo = TunerInfo.BitErrorRate / div;
         					rem = TunerInfo.BitErrorRate % div;
        					div /= 100;                 /* keep only two numbers after decimal point */
        					 if(div != 0)     /*use to avoid crashing of something/0 division in toolset ST40R4.0.1 */
      						   {
          						  rem /= div;
          					    } 
     							quo_snr = (TunerInfo.SignalQuality *36)/100;
     							rem_snr = (TunerInfo.SignalQuality *36)%100;
      
     						 TUNER_DebugPrintf(("F = %u KHz, FEC = %s, Modulation = %s, Status \'%s\', Quality =%u%%( %u.%u Db), SpectrumInversion = %u , BER (%d) = %d.%dx10**-%d\n",
                        			TunerInfo.Frequency,
                        			TUNER_FecToString(TunerInfo.ScanInfo.FECRates),
                        			TUNER_ModulationToString(TunerInfo.ScanInfo.Modulation),
                        			TUNER_StatusToString(TunerInfo.Status),TunerInfo.SignalQuality,
                       				quo_snr,rem_snr,
                        			TunerInfo.ScanInfo.IQMode,
                      				TunerInfo.BitErrorRate,quo,rem,exp));
   #endif                           
                       

           
          TUNER_DebugPrintf(("Event = %s.\n", TUNER_EventToString(Event)));

            if (TunerInfo.Status != STTUNER_STATUS_LOCKED)
            {
                TUNER_TestFailed(Result,"Expected = 'STTUNER_STATUS_LOCKED'");
                error = STTUNER_SetScanList(TUNERHandle, &Scans);
                 /* goto tracking_end;*/
            }
        }
            /****/
             
        }
        else
        {
            TUNER_DebugPrintf(("Event = %s\n", TUNER_EventToString(Event) ));
        }
        
     
    }
  }/*for k*/
     
    if (ChannelCount == ChannelTableCount)
    {
        TUNER_TestPassed(Result);
    }
    else
    {
        TUNER_TestFailed(Result, "Incorrect number of channels");
    }
/**********/

simple_end:
        error = STTUNER_Close(TUNERHandle);
        TUNER_DebugError("STTUNER_Close()", error);

        TermParams.ForceTerminate = FALSE;
        error = STTUNER_Term(TunerDeviceName[Params_p->Tuner], &TermParams);
        TUNER_DebugError("STTUNER_Term()", error);
        /********* RESET THE SCAN PARAMETERS****************/
        Init_scanparams();

    return Result;
} /* TunerScanVHFUHF() */

#endif

#endif
/* ------------------------------------------------------------------------ */

/* EOF */
