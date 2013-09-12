/***********************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : i2ctest.c

Master mode Test Harness for the I2C driver.

************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#if defined(ST_OS20)
#include <ostime.h>
#include <kernel.h>
#include "debug.h"
#endif

#if defined(ST_OSLINUX)
#include <unistd.h>
#define  STTBX_Print(x) printf x
#include "compat.h"
#else
#include "sttbx.h"
#include "stpio.h"
#endif

#include "stos.h"
#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"
#include "stboot.h"
#include "stcommon.h"
#include "sti2c.h"
/*added for 5100 sysytem register configuration*/
#include "stsys.h"
/*#include "cache.h"*/
#ifdef TUNER_TEST_EXTENDED
#include "tunerdat.h"
#include <assert.h>
#endif /* END OF TUNER_TEST_EXTENED*/


/* Sizes of partitions */
#if defined(ST_OSLINUX)
#define SYSTEM_PARTITION_SIZE     (ST_Partition_t *)0x1
#define TINY_PARTITION_SIZE       (ST_Partition_t *)0x1
#define TEST_PARTITION_1          (ST_Partition_t *)0x1
#define TINY_PARTITION_1          (ST_Partition_t *)0x1
#define STACK_PARTITION_1         (ST_Partition_t *)0x1
ST_Partition_t                    *DriverPartition = (ST_Partition_t*)1;
ST_Partition_t                    *system_partition = (ST_Partition_t*)1;
ST_Partition_t                    *NcachePartition = (ST_Partition_t*)1;

#else
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            5000000
#define TINY_PARTITION_SIZE              32

#ifdef ST_OS21
#define TEST_PARTITION_1       system_partition_p
#else
#define TEST_PARTITION_1       &the_system_partition
#endif

#ifdef ST_OS21
#define TINY_PARTITION_1       the_tiny_partition_p
#else
#define TINY_PARTITION_1       &the_tiny_partition
#endif

#ifdef ST_OS21
#define STACK_PARTITION_1        system_partition_p
#else
#define STACK_PARTITION_1        TEST_PARTITION_1
#endif

#endif /*ST_OSLINUX*/


#if !defined(ST_OSLINUX)
#ifdef ST_OS20
/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section_noinit")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
#pragma ST_section      ( system_block,   "system_section_noinit")

#if defined(ST_5528)|| defined(ST_5100) || defined(ST_7710) ||defined(ST_5105) || \
defined(ST_7100) || defined(ST_5700) || defined(ST_7109) || defined(ST_5188) || \
defined(ST_5107) || defined(ST_7200)

static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

#ifdef UNIFIED_MEMORY

#if defined (ST_5516) || defined(ST_5517)
static unsigned char    ncache2_block[1];
#pragma ST_section      ( ncache2_block, "ncache2_section")
#endif

#endif

static U8               tiny_block [TINY_PARTITION_SIZE];
static partition_t      the_tiny_partition;
#pragma ST_section      ( tiny_block,     "ncache_section")

/* This is to avoid a linker warning */
static unsigned char    internal_block_init[1];
#pragma ST_section      ( internal_block_init, "internal_section")

static unsigned char    system_block_init[1];
#pragma ST_section      ( system_block_init, "system_section")
/* Temporary, for compatibility with old drivers (EVT) */
partition_t             *system_partition= &the_system_partition;

#else /* ST_OS21 */
#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_PARTITION_SIZE];
partition_t             *system_partition_p;
static U8               tiny_block [TINY_PARTITION_SIZE];
partition_t             *the_tiny_partition_p;

#endif /* ST_OS21 */
#endif/*!(ST_OSLINUX)*/

/* Private types/constants ----------------------------------------------- */
#if defined(ST_5100)

#define INTERCONNECT_BASE                       (U32)0x20D00000

#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#endif

#if defined(ST_7710)

#define INTERCONNECT_BASE              (U32)0x20068000

#define INTERCONNECT_COMMS_CFG_1       0x04
#define INTERCONNECT_COMMS_CFG_2       0x08
#define INTERCONNECT_COMMS_CFG_3       0x0C
#define INTERCONNECT_COMMS_PWR_CTRL    0x10

#define CONFIG_CONTROL_1   (INTERCONNECT_BASE + INTERCONNECT_COMMS_CFG_1)
#define CONFIG_CONTROL_2   (INTERCONNECT_BASE + INTERCONNECT_COMMS_CFG_2)
#define CONFIG_CONTROL_3   (INTERCONNECT_BASE + INTERCONNECT_COMMS_CFG_3)
#define CONFIG_CONTROL_PWR (INTERCONNECT_BASE + INTERCONNECT_COMMS_PWR_CTRL)

#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)

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

/* Test configuration information ------------------------------------------*/
/*extended tuner test specific defines*/
#ifdef TUNER_TEST_EXTENDED

#ifndef TUNER_TEST_CYCLES
#define TUNER_TEST_CYCLES   1
#endif

#define TUNER_DEVICE_ID_REG 0x00
#define MAX_REG_TUNER       255
#define MAX_NAME_LENGTH     20

#define MAX_TUNER_REG       255

#define TUNER_DEVICE_ID_REG 0x00
#define MCR_REG_299         0x02
#define SYNTCTRL_REG_399    0x46
#define TOPCTRL_REG_360     0x02

#define NOT_SUPPORTED       4

#define TUNER_MODE_STANDBY 0
#define TUNER_MODE_NORMAL  1

#endif /* END OF TUNER_TEST_EXTENDED*/

#define  LOCK_TEST
/*#undef  LOCK_TEST             */         /* set this to run concurrent tests */
#undef  LEAK_TEST                      /* set to run memory leak test - SLOW */
#undef  DISPLAY_INIT_CHAIN             /* set to print init linked list */
#undef  DISPLAY_OPEN_CHAIN             /* set to print open linked list */
#undef  TEST_LINE_STATE

/* NB speed test can cause EEPROM interface problems, as it uses transfers
   larger than the EEPROM page size. The EEPROM does not support this!
   (especially apparent when running with I-cache enabled). */
#define SPEED_TEST

#define MEM_TEST_CYCLES        100000     /* Used with LEAK_TEST  */
#define SPEED_BUF_LEN          (8*1024)    /* Used with SPEED_TEST */
#define TASK_WRITE_CYCLES      100        /* Used with LOCK_TEST  */

/* Set to print out durations of read & write tests */
#undef  DURATION

/* Set these to make WriteMsgA/B to writes instead of reads */
#undef  WRITE_A_TEST
#undef  WRITE_B_TEST

#if defined(mb5518)
#undef MB5518
#define MB5518
#endif

/* NOTE: to use trace, compiler option -DSTI2C_TRACE must be applied to both
   this file & the driver. */

/* Enum of PIO devices */
enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_6,
    PIO_DEVICE_7,
    PIO_DEVICE_NOT_USED
};
/* Enum of SSC devices */
enum
{
    SSC_DEVICE_0,
    SSC_DEVICE_1,
    SSC_DEVICE_2,
    SSC_DEVICE_3,
    SSC_DEVICE_4,
    SSC_DEVICE_5,
    SSC_DEVICE_6,
    SSC_DEVICE_7,
    SSC_DEVICE_NOT_USED
};

ST_DeviceName_t      PIODevName[] =
{
    "PIO_0",
    "PIO_1",
    "PIO_2",
    "PIO_3",
    "PIO_4",
    "PIO_5",
    "PIO_6",
    "PIO_7",
    ""
};
ST_DeviceName_t      I2CDevName[] =
{
    "SSC_0",
    "SSC_1",
    "SSC_2",
    "SSC_3",
    "SSC_4",
    "SSC_5",
    "SSC_6",
    "SSC_7",
    ""
};

/* Interrupt Levels*/
#define SSC_2_INTERRUPT_LEVEL	8
#define SSC_3_INTERRUPT_LEVEL	9
#define SSC_4_INTERRUPT_LEVEL	10

/* Board/device specific constants */
#if defined(ST_7710) || defined(ST_5525)
#define TEST_NUM_SSC 4
#elif defined(ST_5100)||defined(ST_7100)||defined(ST_5301)||defined(ST_8010)||defined(ST_7109)
#define TEST_NUM_SSC 3
#elif defined(ST_7200)
#define TEST_NUM_SSC 5
#else
#define TEST_NUM_SSC 2 /*5105,5188,5107*/
#endif

#if defined(ST_5105) || defined(ST_5107) ||defined(ST_5188) 
#define NUMBER_PIO 4
#elif defined(ST_5100) ||defined(ST_5301) ||defined(ST_5512)
#define NUMBER_PIO 5
#elif defined(ST_7200)
#define NUMBER_PIO 8
#elif defined(ST_8010)
#define NUMBER_PIO 7
#else
#define NUMBER_PIO 6
#endif

#if defined(ST_5508) || defined(ST_5518) || defined(MB5518)
#define PIO_FOR_SSC0_SDA            PIO_BIT_0
#define PIO_FOR_SSC0_SCL            PIO_BIT_1
#define PIO_FOR_SSC1_SDA            PIO_BIT_0
#define PIO_FOR_SSC1_SCL            PIO_BIT_1

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_TP3)
#define PIO_FOR_SSC0_SDA            PIO_BIT_0
#define PIO_FOR_SSC0_SCL            PIO_BIT_2
#define PIO_FOR_SSC1_SDA            PIO_BIT_0
#define PIO_FOR_SSC1_SCL            PIO_BIT_2

#elif defined(ST_5100) || defined(ST_5301)
#define PIO_FOR_SSC0_SDA            PIO_BIT_0
#define PIO_FOR_SSC0_SCL            PIO_BIT_1
#define PIO_FOR_SSC1_SDA            PIO_BIT_2
#define PIO_FOR_SSC1_SCL            PIO_BIT_3
#define PIO_FOR_SSC2_SDA            PIO_BIT_3
#define PIO_FOR_SSC2_SCL            PIO_BIT_2

#elif defined(ST_5700)
#define PIO_FOR_SSC0_SDA            PIO_BIT_0
#define PIO_FOR_SSC0_SCL            PIO_BIT_1
#define PIO_FOR_SSC1_SDA            PIO_BIT_5
#define PIO_FOR_SSC1_SCL            PIO_BIT_7

#elif defined(ST_5188)
#define PIO_FOR_SSC0_SDA            PIO_BIT_6
#define PIO_FOR_SSC0_SCL            PIO_BIT_5
#define PIO_FOR_SSC1_SDA            PIO_BIT_3
#define PIO_FOR_SSC1_SCL            PIO_BIT_2

#elif defined(ST_7100) || defined(ST_7109)
#define PIO_FOR_SSC0_SDA            PIO_BIT_1
#define PIO_FOR_SSC0_SCL            PIO_BIT_0
#define PIO_FOR_SSC1_SDA            PIO_BIT_1
#define PIO_FOR_SSC1_SCL            PIO_BIT_0
#define PIO_FOR_SSC2_SDA            PIO_BIT_1
#define PIO_FOR_SSC2_SCL            PIO_BIT_0

#elif defined(ST_7710)
#define PIO_FOR_SSC0_SDA            PIO_BIT_1
#define PIO_FOR_SSC0_SCL            PIO_BIT_0
#define PIO_FOR_SSC1_SDA            PIO_BIT_1
#define PIO_FOR_SSC1_SCL            PIO_BIT_0
#define PIO_FOR_SSC2_SDA            PIO_BIT_1
#define PIO_FOR_SSC2_SCL            PIO_BIT_0
#define PIO_FOR_SSC3_SDA            PIO_BIT_6
#define PIO_FOR_SSC3_SCL            PIO_BIT_5

#elif defined(ST_7200)
#define PIO_FOR_SSC0_SDA            PIO_BIT_1
#define PIO_FOR_SSC0_SCL            PIO_BIT_0
#define PIO_FOR_SSC1_SDA            PIO_BIT_1
#define PIO_FOR_SSC1_SCL            PIO_BIT_0
#define PIO_FOR_SSC2_SDA            PIO_BIT_1
#define PIO_FOR_SSC2_SCL            PIO_BIT_0
#define PIO_FOR_SSC3_SDA            PIO_BIT_1
#define PIO_FOR_SSC3_SCL            PIO_BIT_0
#define PIO_FOR_SSC4_SDA            PIO_BIT_7
#define PIO_FOR_SSC4_SCL            PIO_BIT_6

#else
/* 5514, 5516, 5517, 5528... */
#define PIO_FOR_SSC0_SDA            PIO_BIT_0
#define PIO_FOR_SSC0_SCL            PIO_BIT_1
#define PIO_FOR_SSC1_SDA            PIO_BIT_2
#define PIO_FOR_SSC1_SCL            PIO_BIT_3

#endif

#if defined(ST_5508) || defined(ST_5518) || defined(MB5518)
#define SSC0_SDA_DEV                PIO_DEVICE_3
#define SSC0_SCL_DEV                PIO_DEVICE_3
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3

#elif defined(ST_5510) || defined(ST_5512) || defined(ST_TP3)
#define SSC0_SDA_DEV                PIO_DEVICE_3
#define SSC0_SCL_DEV                PIO_DEVICE_3
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3

#elif defined(ST_5100) || defined(ST_5301)
#define SSC0_SDA_DEV                PIO_DEVICE_3
#define SSC0_SCL_DEV                PIO_DEVICE_3
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3
#define SSC2_SDA_DEV                PIO_DEVICE_4
#define SSC2_SCL_DEV                PIO_DEVICE_4

#elif defined(ST_5700)
#define SSC0_SDA_DEV                PIO_DEVICE_1
#define SSC0_SCL_DEV                PIO_DEVICE_1
#define SSC1_SDA_DEV                PIO_DEVICE_0
#define SSC1_SCL_DEV                PIO_DEVICE_0

#elif defined(ST_5188)
#define SSC0_SDA_DEV                PIO_DEVICE_0 /* Dummy */
#define SSC0_SCL_DEV                PIO_DEVICE_0 /* Dummy */
#define SSC1_SDA_DEV                PIO_DEVICE_0
#define SSC1_SCL_DEV                PIO_DEVICE_0

#elif defined(ST_7100) || defined(ST_7109)
#define SSC0_SDA_DEV                PIO_DEVICE_2
#define SSC0_SCL_DEV                PIO_DEVICE_2
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3
#define SSC2_SDA_DEV                PIO_DEVICE_4
#define SSC2_SCL_DEV                PIO_DEVICE_4

#elif defined(ST_7710)
#define SSC0_SDA_DEV                PIO_DEVICE_2
#define SSC0_SCL_DEV                PIO_DEVICE_2
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3
#define SSC2_SDA_DEV                PIO_DEVICE_4
#define SSC2_SCL_DEV                PIO_DEVICE_4
#define SSC3_SDA_DEV                PIO_DEVICE_5
#define SSC3_SCL_DEV                PIO_DEVICE_5

#elif defined(ST_7200)
#define SSC0_SDA_DEV                PIO_DEVICE_2
#define SSC0_SCL_DEV                PIO_DEVICE_2
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3
#define SSC2_SDA_DEV                PIO_DEVICE_4
#define SSC2_SCL_DEV                PIO_DEVICE_4
#define SSC3_SDA_DEV                PIO_DEVICE_5
#define SSC3_SCL_DEV                PIO_DEVICE_5
#define SSC4_SDA_DEV                PIO_DEVICE_7
#define SSC4_SCL_DEV                PIO_DEVICE_7

#else
/* 5514, 5516, 5517, 5528... */
#define SSC0_SDA_DEV                PIO_DEVICE_3
#define SSC0_SCL_DEV                PIO_DEVICE_3
#define SSC1_SDA_DEV                PIO_DEVICE_3
#define SSC1_SCL_DEV                PIO_DEVICE_3

#endif

/* Other Private types/constants ------------------------------------------ */
#define MAX_DEVICES         10      /* Should be enough */

/* Page size of EEPROM device. Default is for STi5510-DEMO board
   (M24C32 device) */
#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE       32
#endif
#ifdef ST_OS21
#define STACK_SIZE             8*1024 + 10000
#else
#define STACK_SIZE             10000
#endif

#if defined(ST_5105) || defined(ST_5700) || defined(ST_8010) || defined(ST_5188) || defined(ST_5107)
#define EEPROM_DEVICE_ID       0xae     /* On 5105 E1,E2 and E3 are connected to Vcc */
#else
#define EEPROM_DEVICE_ID       0xa0
#endif

#define TUNER_DEVICE_ID        0xD0     /* C0 might work for non-mb5518 */
#define TUNER_DEVICE_ID1       0xD2
#define TUNER_DEVICE_ID2       0xD4
#define TUNER_DEVICE_ID3       0xD6
#define TUNER_DEVICE_ID4       0x38
#define TUNER_DEVICE_ID5       0x3A
#define TUNER_DEVICE_ID6       0x3C
#define TUNER_DEVICE_ID7       0x3E


#define SCART_DEVICE_ID        0x96

#define TUNER_OFFSET_0         0x00     /* R   - Device ID register     */
#define TUNER_OFFSET_1         0x01     /* R/W - AVAILABLE on 199 & 299 */
#define TUNER_OFFSET_2         0x02     /* R/W - AVAILABLE on 199 & 299 */

#define TRACE_PAGE_SIZE        50

/* LUT terminator */
#define ST_ERROR_UNKNOWN            ((U32)-1)


void myprintf (char *format, ...)
{
    /* Local equivalent of printf, which can be defined to use STTBX or
       debug functions. */
    char s[256];
    va_list v;

    va_start (v, format);
    vsprintf (s, format, v);
#ifndef STI2C_NO_TBX
    STTBX_Print ((s));
#else
    printf(s);
#endif

    va_end (v);
}

/* Error messages */
typedef struct  I2CDeviceType_s
{
    ST_ErrorCode_t Error;
    char ErrorMsg[56];
} I2C_ErrorMessage;

typedef enum I2CDeviceType_e
{
    TUNER,
    EEPROM,
    SCART,
    NO_DEVICE
} I2CDeviceType_t;


#ifdef TUNER_TEST_EXTENDED
typedef enum TunerChip_e
{
    TUNER_299,
    TUNER_399,
    TUNER_360
}TunerChip_t;

typedef struct TunerRegister_s
{
    U8 RegData[MAX_TUNER_REG][2];
    U8 NumberOfRegisters;
    U8 BaseReg;
}TunerRegister_t;


#endif /* END OF TUNER_TEST_EXTENDED*/

typedef struct BusDefinition_s
{
    U32                 SSCBaseAddress;
#ifdef ST_OS20
    U32                 Interrupt;
#endif
    U32                 InterruptLevel;
    U32                 PioBase;
#ifdef ST_OS20
    U32                 PioInterrupt;
#endif
    U32                 PioInterruptLevel;
    U32                 BaudRate;
    U32                 BitforSDA;
    U32                 BitforSCL;
} BusDefinition_t;

typedef struct DeviceDefinition_s
{
    ST_DeviceName_t     Name;
    U32                 Address;
    I2CDeviceType_t     DeviceType;
} DeviceDefinition_t;

/* Private variables ------------------------------------------------------ */

#ifdef TUNER_TEST_EXTENDED
extern U8 RegData299[][2];
extern U8 RegData399[][2];
extern U8 RegData360[][2];
#endif /* END OF TUNER_TEST_EXTENDED*/


#if defined(STI2C_DEBUG)
extern U8 TraceBuffer[32 * 1024];
extern U32 TraceBufferPos = 0;
#endif

#if defined(STI2C_TRACE)
#define DBG_REC_SIZE 10
#define DBG_SIZE 10000
extern U32 I2cIntCount;
extern volatile U32 I2cIndex;
extern volatile U32 I2cLastIndex;
extern U32 I2cDebug [DBG_SIZE * DBG_REC_SIZE];
#endif


/* Used in stack tests, to avoid inflating figures too much */
U8               stackLocalBuff [1024];

/* Variables used in multiple task test sections ie handles and task  */
/* descriptors                                                               */

static U32     ErrorCount    = 0;        /* Overall count of errors */
static U32     BusyCount     = 0;        /* Count of 'device busy' */
static U32     SuccessCount  = 0;        /* Count of successful msgs */
static BOOL    Quiet         = FALSE;    /* set to suppress confirmation
                                            messages on expected results */
static clock_t ClocksPerSec;
static U32 ClockSpeed;
static STI2C_Handle_t HandleEEPROM, HandleEEPROM2, HandleTuner;

#ifdef TUNER_TEST_EXTENDED

static STI2C_Handle_t HandleTunerPrimary,HandleTunerSecondary;

TunerChip_t TunerChip;

U8 StandByReg[] = {MCR_REG_299,SYNTCTRL_REG_399,TOPCTRL_REG_360};

char TunerName[4][MAX_NAME_LENGTH] = {"299","399","360","Not Supported"};

static U32 TotalTunerMismatch = 0;
static U32 OtherFailures = 0;

static U8 ReadBuffPrim[MAX_TUNER_REG];
static U8 ReadBuffSec[MAX_TUNER_REG];

static TunerRegister_t PrimTuner,SecTuner;

static void        *taskStackTunerPrim_p;
static void        *taskStackTunerSec_p;
static task_t      *taskTunerPrim_p;
static task_t      *taskTunerSec_p;
static tdesc_t     tdescTunerPrim;
static tdesc_t     tdescTunerSec;

static semaphore_t *CompareAllSem_p;
static semaphore_t *PrintSem_p;

#endif /* END OF TUNER_TEST_EXTENDED*/

static void        *taskStackOne_p;
static void        *taskStackTwo_p;
static void        *taskStackThree_p;
static void        *taskStackFour_p;

static tdesc_t     tdescOne;
static tdesc_t     tdescTwo;
static tdesc_t     tdescThree;
static tdesc_t     tdescFour;

static task_t      *taskOne_p;
static task_t      *taskTwo_p;
static task_t      *taskThree_p;
static task_t      *taskFour_p;
static semaphore_t *TaskaSem_p;
static semaphore_t *TaskbSem_p;
static semaphore_t *TaskcSem_p;
static semaphore_t *TaskdSem_p;
semaphore_t        *LockTestSem_p;

/* Variables for the lock tests */

U32                      LockTestResult = 0;

static U32               ReadTimeout=1000, WriteTimeout=1000;
static clock_t           Start1, Start2, Finish1, Finish2;


#ifdef STI2C_TRACE
extern U32 I2cPrintTrace (S32 Start, S32 Number);
#endif

#if !defined(ST_OSLINUX)
#ifndef ST_7200
#ifdef ST_OS20
#ifdef BAUD_RATE_FAST

    static BusDefinition_t  Buses[TEST_NUM_SSC] =
                        { { SSC_0_BASE_ADDRESS, SSC_0_INTERRUPT, SSC_0_INTERRUPT_LEVEL,
                            STI2C_RATE_FASTMODE, PIO_FOR_SSC0_SDA, PIO_FOR_SSC0_SCL }
#ifdef TEST_FRONT_BUS
                        , { SSC_1_BASE_ADDRESS, SSC_1_INTERRUPT, SSC_1_INTERRUPT_LEVEL,
                             STI2C_RATE_FASTMODE, PIO_FOR_SSC1_SDA, PIO_FOR_SSC1_SCL }
#endif
                        };

#else
    static BusDefinition_t  Buses[TEST_NUM_SSC] =
                       { {    SSC_0_BASE_ADDRESS, SSC_0_INTERRUPT, SSC_0_INTERRUPT_LEVEL,
                              STI2C_RATE_NORMAL, PIO_FOR_SSC0_SDA, PIO_FOR_SSC0_SCL }
#ifdef TEST_FRONT_BUS
                          , { SSC_1_BASE_ADDRESS,SSC_1_INTERRUPT, SSC_1_INTERRUPT_LEVEL,
                              STI2C_RATE_NORMAL, PIO_FOR_SSC1_SDA, PIO_FOR_SSC1_SCL }
#endif
                        };

#endif /*END OF BAUD_RATE_FAST*/

#else

#ifdef BAUD_RATE_FAST

    static BusDefinition_t  Buses[TEST_NUM_SSC] =
                        { { SSC_0_BASE_ADDRESS, SSC_0_INTERRUPT_LEVEL,
                            STI2C_RATE_FASTMODE, PIO_FOR_SSC0_SDA, PIO_FOR_SSC0_SCL }
#ifdef TEST_FRONT_BUS
                        , { SSC_1_BASE_ADDRESS, SSC_1_INTERRUPT_LEVEL,
                            STI2C_RATE_FASTMODE, PIO_FOR_SSC1_SDA, PIO_FOR_SSC1_SCL }
#endif
                        };

#else
    static BusDefinition_t  Buses[TEST_NUM_SSC] =
                       { {    SSC_0_BASE_ADDRESS, SSC_0_INTERRUPT_LEVEL,
                              STI2C_RATE_NORMAL, PIO_FOR_SSC0_SDA, PIO_FOR_SSC0_SCL }
#ifdef TEST_FRONT_BUS
                          , { SSC_1_BASE_ADDRESS, SSC_1_INTERRUPT_LEVEL,
                              STI2C_RATE_NORMAL, PIO_FOR_SSC1_SDA, PIO_FOR_SSC1_SCL }
#endif
                        };

#endif /*END OF BAUD_RATE_FAST*/

#endif /*END OF ST_OS21 and linux*/
#endif
#endif
static DeviceDefinition_t Devices[] = { { "EEPROM", EEPROM_DEVICE_ID, EEPROM },
                                        { "TUNER", TUNER_DEVICE_ID,  TUNER },
                                        { "TUNER", TUNER_DEVICE_ID1, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID2, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID3, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID4, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID5, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID5, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID6, TUNER },
                                        { "TUNER", TUNER_DEVICE_ID7, TUNER },
                                        { "641x", SCART_DEVICE_ID, SCART },
                                        { "EndOfList", 0x00, NO_DEVICE }
                                      };

static U32 NumberPresent[TEST_NUM_SSC];
static U32 DevicesPresent[TEST_NUM_SSC][MAX_DEVICES];

static BOOL DoTunerTests = TRUE;
static BOOL DoEEPROMTests = TRUE;

/* Private function prototypes -------------------------------------------- */

static BOOL CompareData (const U8 *b1, const U8 *b2, U32 size);
static ST_ErrorCode_t WriteTuner  (U32 address, U8 data);
static ST_ErrorCode_t ReadTuner  (U32 address, U8 *data);
static ST_ErrorCode_t WriteTuner2 (STI2C_Handle_t LocalHandleTuner,
                                   U32 address, U8 data);
static ST_ErrorCode_t ReadTuner2 (STI2C_Handle_t HandleTuner,
                                  U32 address, U8 *data);
static void BusProbe(STI2C_InitParams_t *InitParams_p);
static S8   FindEEPROM(void);
static S8 FindTuner(void);
char *I2C_ErrorString(ST_ErrorCode_t Error);


static char * GetTunerChipName(U8 ChipId);
static S8 FindTunerAddress(U8 *Address);

#ifdef TUNER_TEST_EXTENDED

static S8 FindTunerAddress(U8 *Address);
static ST_ErrorCode_t StartTunerTestSingle(void );
static ST_ErrorCode_t StartTunerTestMultiple(void );
static void TestTunerSecondary(void * voidptr);
static void TestTunerPrimary(void * voidptr);
static void WriteAllTunerReg(STI2C_Handle_t Handle,U8 BaseReg,
                                    U8 NumberOfReg2Wr,U8 regDataPtr[][2]);
static void ReadAllTunerReg(STI2C_Handle_t Handle, U8 BaseReg,
                                U8 NumberOfReg2Wr,U8 readBuffer[]);
static U32 CompareAllReg(U8 NumberOfReg,U8 readBuffer_p[],U8 regData_p[][2]);
static ST_ErrorCode_t SetTunerMode( STI2C_Handle_t Handle, U32 Mode);
static void DoTheTunerTest(void);
static ST_ErrorCode_t ReadTunerNoPrint  ( STI2C_Handle_t Handle,
                                            U32 address, U8 *pData);
static ST_ErrorCode_t WriteTunerNoPrint ( STI2C_Handle_t Handle,
                                            U32 address, U8 data);

#endif /* END OF TUNER_TEST_EXTENDED */

#ifndef TUNER_TEST_EXTENDED
static void DoTheTunerTest(void){  } /* to handle warning when not defined extended tests */
#endif /* END OF TUNER_TEST_EXTENDED */

#if  defined(ST_OSLINUX) && defined(ST_7200)
void DoTheTests_7200_Linux(void);
#endif
void DoTheTests (void);
static void DisplayError( ST_ErrorCode_t ErrorCode );
static ST_ErrorCode_t WriteEEPROM (U32 address, U8 *buffer,
                                       U32 size, U32 *pActLen);
static ST_ErrorCode_t ReadEEPROM (U32 address, U8 *buffer,
                                  U32 size, U32 *pActLen);
#if !defined(ST_OSLINUX) 
#if !defined(ST_7200)                             
static ST_ErrorCode_t WriteEEPROM2(STI2C_Handle_t, U32 address, U8 *buffer,
                                   U32 size, U32 *pActLen);
static ST_ErrorCode_t ReadEEPROM2(STI2C_Handle_t, U32 address, U8 *buffer,
                                   U32 size, U32 *pActLen);
void DumpI2cRegs(void *addr);
static void StackTest(void);

/*local functions*/

/* - Stack usage code ------------------------------------------------------ */

static void test_overhead (void *dummy) { }
static void test_init(void *dummy)
{
    ST_ErrorCode_t      error;
    STI2C_InitParams_t  InitParams;

    /* Set up initparams */
#ifdef ST_5188
    InitParams.BaseAddress           = (U32*)Buses[1].SSCBaseAddress;
    InitParams.InterruptNumber       = Buses[1].Interrupt;
    InitParams.InterruptLevel        = Buses[1].InterruptLevel;
    InitParams.BaudRate              = STI2C_RATE_NORMAL;
    InitParams.MasterSlave           = STI2C_MASTER;
    InitParams.ClockFrequency        = ClockSpeed;
    InitParams.GlitchWidth           = 0;
    InitParams.PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    InitParams.PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;
    InitParams.MaxHandles            = 4;
    InitParams.DriverPartition       = TEST_PARTITION_1;
    InitParams.FifoEnabled           = FALSE;
    /* Initialised by DoTheTests before we get called */
    strcpy( InitParams.PIOforSDA.PortName, PIODevName[0] );
    strcpy( InitParams.PIOforSCL.PortName, PIODevName[0] );

#else

    InitParams.BaseAddress           = (U32*)Buses[0].SSCBaseAddress;
#if defined (ST_OS21)
    InitParams.InterruptNumber       = SSC_0_INTERRUPT;
#else
    InitParams.InterruptNumber       = Buses[0].Interrupt;
#endif
    InitParams.InterruptLevel        = Buses[0].InterruptLevel;
#ifdef BAUD_RATE_FAST
    InitParams.BaudRate              = STI2C_RATE_FASTMODE;
#else
    InitParams.BaudRate              = STI2C_RATE_NORMAL;
#endif
    InitParams.MasterSlave           = STI2C_MASTER;
    InitParams.ClockFrequency        = ClockSpeed;
    InitParams.GlitchWidth           = 0;
    InitParams.PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    InitParams.PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;
    InitParams.MaxHandles            = 4;
#if defined(ST_OS21)
    InitParams.DriverPartition       = system_partition_p;
#else
    InitParams.DriverPartition       = TEST_PARTITION_1;
#endif

#ifdef TEST_FIFO
    InitParams.FifoEnabled           = TRUE;
#else
    InitParams.FifoEnabled           = FALSE;
#endif
    /* Initialised by DoTheTests before we get called */
    strcpy( InitParams.PIOforSDA.PortName, PIODevName[0] );
    strcpy( InitParams.PIOforSCL.PortName, PIODevName[0] );
#endif

    /* Make the call */
    error = STI2C_Init("STACKTEST", &InitParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while initialising:", I2C_ErrorString(error));
    }
}

static void test_term(void *dummy)
{
    ST_ErrorCode_t      error;
    STI2C_TermParams_t  TermParams;

    TermParams.ForceTerminate = TRUE;
    error = STI2C_Term("STACKTEST", &TermParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while initialising:", I2C_ErrorString(error));
    }
}

/* This test makes the assumption that an E2PROM block is present on
 * bus 0.
 */
static void test_typical(void *dummy)
{
    U8  Buffer[10], i;
    U32 ActLen;

    ST_ErrorCode_t      error;
    STI2C_OpenParams_t  OpenParams;
    STI2C_Handle_t      Handle;

    /* Open block */
    OpenParams.BusAccessTimeOut      = 20;
    OpenParams.AddressType           = STI2C_ADDRESS_7_BITS;
    OpenParams.I2cAddress            = EEPROM_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    OpenParams.BaudRate              = STI2C_RATE_NORMAL;
#endif
    error = STI2C_Open("STACKTEST", &OpenParams, &Handle);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while opening: %s\n", I2C_ErrorString(error));
    }
    if (FindEEPROM() >= 0)
    {
        for (i = 0; i < 10; i++)
        {
            Buffer[i] = i;
        }

        error = WriteEEPROM2(Handle, 0x100, Buffer, 10, &ActLen);

        if (error == ST_NO_ERROR)
        {
            memset(Buffer, 0, sizeof(Buffer));
            error = ReadEEPROM2(Handle, 0x100, Buffer, 10, &ActLen);

            if (error == ST_NO_ERROR)
            {
                for (i = 0; i < 10; i++)
                {
                    if (Buffer[i] != i)
                    {
                        myprintf("Error at position %i; got %i, expected %i\n",
                                i, Buffer[i], i);
                    }
                }
            }
        }
    }
    error = STI2C_Close(Handle);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while closing: %s\n", I2C_ErrorString(error));
    }
}


/* See how changing init params affects memory used */
static void test_data_use(void)
{
    ST_ErrorCode_t      error;
    STI2C_InitParams_t  InitParams;
    STI2C_TermParams_t  TermParams;
    U32                 Address1, Address2;
    U8                  *dummy;

    /* Get initial address */
#if defined(ST_OS21)
    dummy = memory_allocate(system_partition_p, 1);
#else
    dummy = memory_allocate(TEST_PARTITION_1, 1);
#endif

    Address1 = (U32)dummy;

#if defined(ST_OS21)
    /*dummy = memory_deallocate(system_partition_p, (void *)dummy);*/
#else
    memory_deallocate(TEST_PARTITION_1, dummy);
#endif

    /* Set up basic params */
    TermParams.ForceTerminate        = FALSE;
    InitParams.BaseAddress           = (U32*)Buses[0].SSCBaseAddress;
#if defined (ST_OS21)
    InitParams.InterruptNumber       = SSC_0_INTERRUPT;
#else
    InitParams.InterruptNumber       = Buses[0].Interrupt;
#endif
    InitParams.InterruptLevel        = Buses[0].InterruptLevel;

#ifdef BAUD_RATE_FAST
    InitParams.BaudRate              = STI2C_RATE_FASTMODE;
#else
    InitParams.BaudRate              = STI2C_RATE_NORMAL;
#endif

    InitParams.MasterSlave           = STI2C_MASTER;
    InitParams.ClockFrequency        = ClockSpeed;
    InitParams.GlitchWidth           = 0;
    InitParams.PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    InitParams.PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;
    InitParams.MaxHandles            = 1;

#if defined(ST_OS21)
    InitParams.DriverPartition       = system_partition_p;
#else
    InitParams.DriverPartition       = TEST_PARTITION_1;
#endif

#ifdef TEST_FIFO
    InitParams.FifoEnabled           = TRUE;
#else
    InitParams.FifoEnabled           = FALSE;
#endif
    /* Initialised by DoTheTests before we get called */
    strcpy( InitParams.PIOforSDA.PortName, PIODevName[0] );
    strcpy( InitParams.PIOforSCL.PortName, PIODevName[0] );

    /* Make the call */
    error = STI2C_Init("STACKTEST", &InitParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while initialising: %s\n", I2C_ErrorString(error));
    }

    /* Get address */
#if defined(ST_OS21)
    dummy = memory_allocate(system_partition_p, 1);
#else
    dummy = memory_allocate(TEST_PARTITION_1, 1);
#endif

    Address2 = (U32)dummy;

#if defined(ST_OS21)
     /*dummy = memory_deallocate(system_partition_p, (void *)dummy);*/
#else
    memory_deallocate(TEST_PARTITION_1, dummy);
#endif
    myprintf("1 handle - address 2: %08x, difference: %d\n",
        Address2, Address1 - Address2);

    /* Term, change number of handles, init */
    error = STI2C_Term("STACKTEST", &TermParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while terminating: %s\n", I2C_ErrorString(error));
    }

    InitParams.MaxHandles = 2;
    error = STI2C_Init("STACKTEST", &InitParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while initialising: %s\n", I2C_ErrorString(error));
    }

    /* Get address */
#if defined(ST_OS21)
    dummy = memory_allocate(system_partition_p, 1);
#else
    dummy = memory_allocate(TEST_PARTITION_1, 1);
    Address2 = (U32)dummy;
#endif

#if defined(ST_OS21)
     /*dummy = memory_deallocate(system_partition_p, (void *)dummy);*/
#else
    memory_deallocate(TEST_PARTITION_1, dummy);
#endif
    myprintf("2 handles - address 2: %08x, difference: %d\n",
        Address2, Address1 - Address2);

    /* Term, change number of handles, init */
    error = STI2C_Term("STACKTEST", &TermParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while terminating: %s\n", I2C_ErrorString(error));
    }

    InitParams.MaxHandles = 3;
    error = STI2C_Init("STACKTEST", &InitParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while initialising: %s\n", I2C_ErrorString(error));
    }

    /* Get address */
#if defined(ST_OS21)
    dummy = memory_allocate(system_partition_p, 1);
#else
    dummy = memory_allocate(TEST_PARTITION_1, 1);
    Address2 = (U32)dummy;
#endif

#if defined(ST_OS21)
     /*dummy = memory_deallocate(system_partition_p, (void *)dummy);*/
#else
    memory_deallocate(TEST_PARTITION_1, dummy);
#endif
    myprintf("3 handles - address 2: %08x, difference: %d\n",
        Address2, Address1 - Address2);

    /* Final term */
    error = STI2C_Term("STACKTEST", &TermParams);
    if (error != ST_NO_ERROR)
    {
        myprintf("Error while terminating: %s\n", I2C_ErrorString(error));
    }
}
static void StackTest(void)
{
#ifdef ST_OS20
#define STI2C_STACK_DEPTH 4*1024
#else
#define STI2C_STACK_DEPTH 16*1024
#endif

    tdesc_t         tdesc;
    task_t          *task_p;
    task_status_t   status;
    void            *stack_p;
    void (*func_table[])(void *) =  {   test_overhead,
                                        test_init,
                                        test_typical,
                                        test_term,
                                        NULL
                                    };
    void (*func)(void *);
    U8              i;

    myprintf("Stack usage tests\n-----------------\n");

    for (i = 0; func_table[i] != NULL; i++)
    {
        func = func_table[i];


        STOS_TaskCreate(func, NULL, STACK_PARTITION_1, STI2C_STACK_DEPTH, &stack_p, STACK_PARTITION_1,
                            &task_p, &tdesc,MAX_USER_PRIORITY,"stack_test", 0);

        STOS_TaskWait(&task_p, TIMEOUT_INFINITY);

        task_status(task_p, &status, task_status_flags_stack_used);
        myprintf("Function %i, stack used = %d\n", i,
                    status.task_stack_used);

        STOS_TaskDelete(task_p, STACK_PARTITION_1,
                    stack_p, STACK_PARTITION_1);
    }

    test_data_use();
}
#endif
#endif/*ST_OSLINUX*/
/* ------------------------------------------------------------------------- */

#ifdef STI2C_TRACE

static U8 GetNextChar (void)
{
    U8 s[80];
    long Length, i;

    while (1)
    {
        Length = debuggets (s, sizeof(s));
        for (i=0; i<Length; i++)
        {
            if (s[i] > ' ')
            {
                return (s[i]);
            }
        }
    }
    return 0;
}

static void QueryTrace (void)
{
    /* Print I2C trace buffer, if user enters 'y' */
    U8 c;

    myprintf("Print trace?\n");
    while (1)
    {
        c = GetNextChar();
        if ((I2cPrintTrace (-1, (c=='y') ? TRACE_PAGE_SIZE : 0) == 0) ||
             (c != 'y') )
        {
            break;
        }
        myprintf("More?\n");
    }
}

#else /* not STI2C_TRACE */

/* Dummy functions to satisfy references */
static void QueryTrace (void) {}

#endif /* STI2C_TRACE */

static void PrintDuration (clock_t Start, clock_t Finish)
{
#ifdef DURATION
    myprintf("Start: %u  Finish: %u  Duration = %u (%u sec)\n",
              Start, Finish, Finish-Start, (Finish-Start)/ClocksPerSec);
#endif
}

static void DisplayError( ST_ErrorCode_t ErrorCode )
{
    switch( ErrorCode )
    {

        case ST_NO_ERROR:
            myprintf("ST_NO_ERROR ");
            break;
        case ST_ERROR_BAD_PARAMETER:
            myprintf("ST_ERROR_BAD_PARAMETER ");
            break;
        case ST_ERROR_ALREADY_INITIALIZED:
            myprintf("ST_ERROR_ALREADY_INITIALIZED ");
            break;
        case ST_ERROR_UNKNOWN_DEVICE:
            myprintf("ST_ERROR_UNKNOWN_DEVICE ");
            break;
        case ST_ERROR_INVALID_HANDLE:
            myprintf("ST_ERROR_INVALID_HANDLE ");
            break;
        case ST_ERROR_OPEN_HANDLE:
            myprintf("ST_ERROR_OPEN_HANDLE ");
            break;
        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            myprintf("ST_ERROR_FEATURE_NOT_SUPPORTED ");
            break;
        case ST_ERROR_NO_MEMORY:
            myprintf("ST_ERROR_NO_MEMORY ");
            break;
        case ST_ERROR_NO_FREE_HANDLES:
            myprintf("ST_ERROR_NO_FREE_HANDLES ");
            break;
        case ST_ERROR_TIMEOUT:
            myprintf("ST_ERROR_TIMEOUT ");
            break;
        case ST_ERROR_DEVICE_BUSY:
            myprintf("ST_ERROR_DEVICE_BUSY ");
            break;
        case STI2C_ERROR_LINE_STATE:
            myprintf("STI2C_ERROR_LINE_STATE ");
            break;
        case STI2C_ERROR_STATUS:
            myprintf("STI2C_ERROR_STATUS ");
            break;
        case STI2C_ERROR_ADDRESS_ACK:
            myprintf("STI2C_ERROR_ADDRESS_ACK ");
            break;
        case STI2C_ERROR_WRITE_ACK:
            myprintf("STI2C_ERROR_WRITE_ACK ");
            break;
        case STI2C_ERROR_NO_DATA:
            myprintf("STI2C_ERROR_NO_DATA ");
            break;
        case STI2C_ERROR_PIO:
            myprintf("STI2C_ERROR_PIO ");
            break;
        case STI2C_ERROR_BUS_IN_USE:
            myprintf("STI2C_ERROR_BUS_IN_USE ");
            break;
        case STI2C_ERROR_EVT_REGISTER:
            myprintf("STI2C_ERROR_EVT_REGISTER ");
            break;
        default:
            myprintf("unknown error code %u ", ErrorCode );
            break;
      }
}

/***********************************************************************
   TEST HARNESS - A call to DoTheTests() is made from within
   main. This effectively isolates the 5510 init code within main().
   This allows easy reuse as your own custom tests can be added to
   this function without altering the setup code.
***********************************************************************/

int main (void)
{
#if !defined(ST_OSLINUX)
    ST_ErrorCode_t        Code;    
    STBOOT_InitParams_t   InitParams;
    STBOOT_TermParams_t   TermParams;
    ST_DeviceName_t       Name = "DEV0";
#endif
    STPIO_InitParams_t    PioInitParams[NUMBER_PIO];
#if defined(ST_5100) || defined(ST_7710) || defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    U32 ReadValue=0xFFFF;
#endif
    /* Begin the extremely long cachemap definition */

#if !defined(ST_OSLINUX)
#if defined(DISABLE_DCACHE)
    STBOOT_DCache_Area_t Cache[] =
    {
        { NULL, NULL }
    };
#elif defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t Cache[] =
    {
        /* assumed ok */
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS },
        { NULL, NULL }
    };
#elif defined(ST_5514) && defined(UNIFIED_MEMORY)
    /* Constants not defined, fallbacks from here on down */
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0xC0380000, (U32 *)0xc03fffff },
        { NULL, NULL}
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5512)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40080000, (U32 *)0x5FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5301)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5525)
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#else
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40080000, (U32 *)0x4FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#ifndef STI2C_NO_TBX
    STTBX_InitParams_t    TBX_InitParams;
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition_p   =  partition_create_heap((U8*)external_block, sizeof(external_block));
    the_tiny_partition_p =  partition_create_heap((U8*)tiny_block,sizeof(tiny_block));
#else /* ifndef ST_OS21 */
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));
    partition_init_heap (&the_tiny_partition,   tiny_block,
                         sizeof(tiny_block));

    /* Avoid compiler warnings */
    internal_block_init[0] = 0;
    system_block_init[0]   = 0;

#if defined(ST_5528)|| defined(ST_5100) || defined(ST_7710) || defined(ST_5105) ||\
defined(ST_7100) || defined(ST_5700) || defined(ST_7109) || defined(ST_5188) || defined(ST_5107) || defined(ST_7200)

    data_section[0] = 0;
#endif
#endif /* ST_OS21 */

#ifdef UNIFIED_MEMORY
#if defined(ST_5516) || defined(ST_5517)
    ncache2_block[0] = 0;
#endif
#endif /*UNIFIED_MEMORY*/
#endif/*!ST_OSLINUX*/

#if defined(ST_5100)||defined(ST_5105) || defined(ST_5107)

    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, ReadValue | 0x00000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);

    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, ReadValue | 0x0c000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
#endif

#if defined(ST_5188)

    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
    /*STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, 0x000000122 );*/
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, 0x0000C0022 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);

#endif

#if defined(ST_7710)

    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_1);
    /* Writing to SSC0 & SSC1 Mux Selects Registers in COMMS GLUE */
    /* ========================================================== */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_1,  0x00000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_1);
#endif

#if !defined(ST_OSLINUX)
#if defined(DISABLE_ICACHE)
    InitParams.ICacheEnabled             = FALSE;
#else
    InitParams.ICacheEnabled             = TRUE;
#endif
    InitParams.DCacheMap                 = Cache;
    InitParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    InitParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    InitParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    InitParams.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
    InitParams.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    InitParams.SDRAMFrequency            = SDRAM_FREQUENCY;

    printf ("Entering STBOOT_Init\n");
    if ((Code = STBOOT_Init (Name, &InitParams)) != ST_NO_ERROR)
    {
        printf ("ERROR: STBOOT_Init returned code %d\n", Code);
    }
    else
    {
#ifndef STI2C_NO_TBX
#if defined(ST_OS21)
        TBX_InitParams.CPUPartition_p       =  (ST_Partition_t *)system_partition_p;
#else
        TBX_InitParams.CPUPartition_p       = &the_system_partition;
#endif
        TBX_InitParams.SupportedDevices    = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
        if ((Code = STTBX_Init( "TBX0", &TBX_InitParams )) != ST_NO_ERROR )
        {
            printf ("Error: STTBX_Init = 0x%08X\n", Code );
        }
        else
#endif
#endif /*(ST_OSLINUX)*/
        {
            ST_ClockInfo_t ClockInfo;
            ClocksPerSec = ST_GetClocksPerSecond();
            ST_GetClockInfo(&ClockInfo);
            ClockSpeed = ClockInfo.CommsBlock;
           
            /* Setup PIO global parameters */
            PioInitParams[0].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
            PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
            PioInitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;
        
            PioInitParams[1].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[1].BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
            PioInitParams[1].InterruptNumber = PIO_1_INTERRUPT;
            PioInitParams[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;
        
            PioInitParams[2].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[2].BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
            PioInitParams[2].InterruptNumber = PIO_2_INTERRUPT;
            PioInitParams[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;
        
            PioInitParams[3].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[3].BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
            PioInitParams[3].InterruptNumber = PIO_3_INTERRUPT;
            PioInitParams[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;
#if (NUMBER_PIO > 4)
            PioInitParams[4].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[4].BaseAddress     = (U32 *)PIO_4_BASE_ADDRESS;
            PioInitParams[4].InterruptNumber = PIO_4_INTERRUPT;
            PioInitParams[4].InterruptLevel  = PIO_4_INTERRUPT_LEVEL;
#endif
        
#if (NUMBER_PIO > 5)
            PioInitParams[5].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[5].BaseAddress     = (U32 *)PIO_5_BASE_ADDRESS;
            PioInitParams[5].InterruptNumber = PIO_5_INTERRUPT;
            PioInitParams[5].InterruptLevel  = PIO_5_INTERRUPT_LEVEL;
#endif
        
#if (NUMBER_PIO > 6)
            PioInitParams[6].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[6].BaseAddress     = (U32 *)PIO_6_BASE_ADDRESS;
            PioInitParams[6].InterruptNumber = PIO_6_INTERRUPT;
            PioInitParams[6].InterruptLevel  = PIO_6_INTERRUPT_LEVEL;
#endif
        
#if (NUMBER_PIO > 7)
            PioInitParams[7].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[7].BaseAddress     = (U32 *)PIO_7_BASE_ADDRESS;
            PioInitParams[7].InterruptNumber = PIO_7_INTERRUPT;
            PioInitParams[7].InterruptLevel  = PIO_7_INTERRUPT_LEVEL;
#endif
        
#ifdef ST_5525
            PioInitParams[8].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
            PioInitParams[8].BaseAddress     = (U32 *)PIO_8_BASE_ADDRESS;
            PioInitParams[8].InterruptNumber = PIO_8_INTERRUPT;
            PioInitParams[8].InterruptLevel  = 5;
#endif
#if !defined(ST_OSLINUX)
	{
            U32 i =0; 
            /* Initialize all pio devices */
            myprintf("Initializing pio devices\n");
            
            for ( i = 0; i < NUMBER_PIO; i++)
            {
                Code = STPIO_Init(PIODevName[i], &PioInitParams[i]);
                if (Code != ST_NO_ERROR)
                {
                    myprintf( "STPIO_Init() returned %d for PIO-%d\n", Code,i );
                    return 0;
                }
            }
	}
#endif

#if  defined(ST_OSLINUX) && defined(ST_7200)
            DoTheTests_7200_Linux();
#else 
            DoTheTests();
#endif
        }
        
#if !defined(ST_OSLINUX)
        STBOOT_Term (Name, &TermParams);
     }
#endif
    return 0;
}

/*---------------------------------------------------------------------------*/
/* Some utility functions for the multitasking test                          */
/*---------------------------------------------------------------------------*/

static BOOL TaskCheckOkOrBusy (char *msg, S32 n, ST_ErrorCode_t code)
{
    if (code == ST_ERROR_DEVICE_BUSY)
    {
        BusyCount++;
    }
    else if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: %s (%d) got code %d\n", msg, n, code);
        ErrorCount++;
        return FALSE;
    }
    else
        SuccessCount++;
    return TRUE;
}

static BOOL TaskCheckOk (char *msg, S32 n, ST_ErrorCode_t code)
{
    if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: %s (%d) got code %d\n", msg, n, code);
        ErrorCount++;
        return FALSE;
    }
    return TRUE;
}

/*---------------------------------------------------------------------------*/
/* Other utility functions for checking various things.                      */
/*---------------------------------------------------------------------------*/

static BOOL CompareData (const U8 *b1, const U8 *b2, U32 size)
{
    if (memcmp (b1, b2, size) == 0)
    {
        if (!Quiet)
        {
            myprintf("Data read matches data written!\n");
        }
        return TRUE;
    }
    else
    {
        myprintf("ERROR: data read does NOT match data written\n");
        if (size == 1)
        {
            myprintf("       Got %i, expected %i\n", *b1, *b2);
        }
        ErrorCount++;
        return FALSE;
    }
}

static BOOL CheckCode (ST_ErrorCode_t actualCode, ST_ErrorCode_t expectedCode)
{
    if (actualCode == expectedCode)
    {
        if (!Quiet)
        {
            myprintf("Got expected code ");
        }
            DisplayError( expectedCode );
            myprintf("\n", expectedCode);
            myprintf( "----------------------------------------------------------\n" );
            return TRUE;
    }
    else
    {
        myprintf("ERROR: got code ");
        DisplayError( actualCode );
        myprintf("\nexpected code ");
        DisplayError( expectedCode );
        myprintf( "\n" );
        ErrorCount++;
        myprintf( "----------------------------------------------------------\n" );
        return FALSE;
    }
}



static BOOL CheckCodeOk (ST_ErrorCode_t actualCode)
{
    return CheckCode (actualCode, ST_NO_ERROR);
}


/*---------------------------------------------------------------------------*/
/* The actual tasks used in the multitasking tests                           */
/* Note that when writing to EEPROM, the message size must be only 2 bytes.  */
/* Otherwise the EEPROM would commence its write cycle, which is a bad idea  */
/* because:                                                                  */
/*    - can't do next write for 10 ms.                                       */
/*    - large number of writes wears out the EEPROM.                         */
/*---------------------------------------------------------------------------*/

void WriteMsgA( void )
{
    ST_ErrorCode_t  RetCode = ST_NO_ERROR;
    U32             ActLen, Timeout, Size;
    U8              WriteBuf [EEPROM_PAGE_SIZE];
    int             i;

    if (DoEEPROMTests == TRUE)
    {
#ifdef WRITE_A_TEST
        Size = 2;    /* Only send an address, to avoid doing an EEPROM 'write' */
        Timeout = WriteTimeout;
        WriteBuf[0] = 0x0f;
        WriteBuf[1] = 0xc0;
#else
        Size = sizeof(WriteBuf);
        Timeout = ReadTimeout;
        WriteBuf[0] = WriteBuf[1] = 0;
#endif
        Start1 = STOS_time_now();

        for (i=0; i<TASK_WRITE_CYCLES; i++)
        {
            if ((i != 0) && ((i % 100) == 0))
            {
                myprintf(".");
            }
            RetCode =
#ifdef WRITE_A_TEST
                      STI2C_Write
#else
                      STI2C_Read
#endif
                                 (HandleEEPROM, WriteBuf, Size, Timeout, &ActLen);
            TaskCheckOkOrBusy ("Task A 1st transfer", i, RetCode);
#if defined(STI2C_TRACE)
            if ((RetCode != ST_NO_ERROR) && (RetCode != ST_ERROR_DEVICE_BUSY))
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
            RetCode =
#ifdef WRITE_A_TEST
                      STI2C_Write
#else
                      STI2C_Read
#endif
                                 (HandleEEPROM2, WriteBuf, Size, Timeout, &ActLen);
            TaskCheckOkOrBusy ("Task A 2nd transfer", i, RetCode);
#if defined(STI2C_TRACE)
            if ((RetCode != ST_NO_ERROR) && (RetCode != ST_ERROR_DEVICE_BUSY))
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
        }

        Finish1 = STOS_time_now();
    }
    STOS_SemaphoreSignal(TaskaSem_p);
}

void WriteMsgB( void )
{
    ST_ErrorCode_t  RetCode = ST_NO_ERROR;
    U32             ActLen, Timeout, Size;
    U8              WriteBuf [EEPROM_PAGE_SIZE];
    int             i;


    if (DoEEPROMTests == TRUE)
    {
#ifdef WRITE_B_TEST
        Size = 2;    /* Only send an address, to avoid doing an EEPROM 'write' */
        Timeout = WriteTimeout;
        WriteBuf[0] = 0x0f;
        WriteBuf[1] = 0xc0;
#else
        Size = sizeof(WriteBuf);
        Timeout = ReadTimeout;
        WriteBuf[0] = WriteBuf[1] = 0;
#endif
        Start2 = STOS_time_now();

        for (i=0; i<TASK_WRITE_CYCLES; i++)
        {
            if ((i != 0) && ((i % 100) == 0))
            {
                myprintf("-");
            }
            RetCode =

#ifdef WRITE_B_TEST
                      STI2C_Write
#else
                      STI2C_Read
#endif
                                 (HandleEEPROM, WriteBuf, Size, Timeout, &ActLen);
            TaskCheckOkOrBusy ("Task B 1st transfer", i, RetCode);
#if defined(STI2C_TRACE)
            if ((RetCode != ST_NO_ERROR) && (RetCode != ST_ERROR_DEVICE_BUSY))
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif

            RetCode =
#ifdef WRITE_B_TEST
                      STI2C_Write
#else
                      STI2C_Read
#endif
                                 (HandleEEPROM2, WriteBuf, Size, Timeout, &ActLen);
            TaskCheckOkOrBusy ("Task B 2nd transfer", i, RetCode);
#if defined(STI2C_TRACE)
            if ((RetCode != ST_NO_ERROR) && (RetCode != ST_ERROR_DEVICE_BUSY))
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
        }
        Finish2 = STOS_time_now();
    }
    STOS_SemaphoreSignal(TaskbSem_p );
}


void WriteMsgC( void )
{
    ST_ErrorCode_t  RetCode = ST_NO_ERROR;
    U32             ActLen;
    U8              WriteBuf[2];
    int             i;

    if (DoEEPROMTests == TRUE)
    {
        WriteBuf[0] = 0x0f;
        WriteBuf[1] = 0xc0;
        Start1 = STOS_time_now();

        for (i=0; i<TASK_WRITE_CYCLES; i++)
        {
            RetCode = STI2C_Write( HandleEEPROM, WriteBuf, sizeof(WriteBuf),
                                   WriteTimeout, &ActLen);
            TaskCheckOk ("Task C Write", i, RetCode);
#if defined(STI2C_TRACE)
            if (RetCode != ST_NO_ERROR)
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
        }

        Finish1 = STOS_time_now();
    }
    STOS_SemaphoreSignal(TaskcSem_p );
}


void WriteMsgD( void )
{
    ST_ErrorCode_t  RetCode     = ST_NO_ERROR;
    U32             ActLen;
    U8              WriteBuf[2] = {TUNER_OFFSET_1, 0x18};
    int             i;

    if (DoTunerTests == TRUE)
    {
        Start2 = STOS_time_now();

        for (i=0; i<TASK_WRITE_CYCLES; i++)
        {
            RetCode = STI2C_Write( HandleTuner, WriteBuf, sizeof(WriteBuf),
                                   WriteTimeout, &ActLen  );
            TaskCheckOk ("Task D Write", i, RetCode);
#if defined(STI2C_TRACE)
            if (RetCode != ST_NO_ERROR)
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
        }

        Finish2 = STOS_time_now();
    }
    STOS_SemaphoreSignal(TaskdSem_p );
}

/*
** Function used in test of repeated start functions
** This function should always succeed as it is started
** first and performs transfers with repeated starts
*/
void ReadMsgE( void )
{
    ST_ErrorCode_t  RetCode = ST_NO_ERROR;
    U32             ActLen, Timeout;
    U8              WriteBuf [EEPROM_PAGE_SIZE];
    int             i;

    if (DoEEPROMTests == TRUE)
    {
        Timeout = ReadTimeout;
        WriteBuf[0] = WriteBuf[1] = 0;

        Start1 = STOS_time_now();

        for (i=0; i<TASK_WRITE_CYCLES; i++)
        {
            if ((i != 0) && ((i % 100) == 0))
            {
                myprintf(".");
            }
            RetCode = STI2C_ReadNoStop(HandleEEPROM, WriteBuf,
                                       2, Timeout, &ActLen);
            TaskCheckOk ("Task E transfer", i, RetCode);
#if defined(STI2C_TRACE)
            if (RetCode != ST_NO_ERROR)
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
        }

        Finish1 = STOS_time_now();
    }
    STOS_SemaphoreSignal(TaskaSem_p );
}

/*
** Function used in test of repeated start functions
** This function should always fail as it is started
** after ReadMsgE which uses repeated start transfers
*/
void ReadMsgF( void )
{
    ST_ErrorCode_t  RetCode = ST_NO_ERROR;
    U32             ActLen, Timeout, Size;
    U8              WriteBuf [EEPROM_PAGE_SIZE];
    int             i;


    if (DoEEPROMTests == TRUE)
    {
        Size = sizeof(WriteBuf);
        Timeout = ReadTimeout;
        WriteBuf[0] = WriteBuf[1] = 0;

        Start2 = STOS_time_now();

        for (i=0; i<TASK_WRITE_CYCLES; i++)
        {
            if ((i != 0) && ((i % 100) == 0))
            {
                myprintf("-");
            }
            RetCode = STI2C_Read(HandleEEPROM2, WriteBuf, Size, Timeout, &ActLen);
            TaskCheckOkOrBusy ("Task F transfer", i, RetCode);
#if defined(STI2C_TRACE)
            if ((RetCode != ST_NO_ERROR) && (RetCode != ST_ERROR_DEVICE_BUSY))
            {
                QueryTrace();
            }
            else
            {
                I2cLastIndex = I2cIndex;
            }
#endif
        }
        Finish2 = STOS_time_now();
    }
    STOS_SemaphoreSignal(TaskbSem_p );
}

void LockTask1( void )
{
    U32 WaitingTime;
    ST_ErrorCode_t error = ST_NO_ERROR;

    WaitingTime = ST_GetClocksPerSecond() * 2;

    /* Lock the bus; should succeed */
    myprintf("Task 1: Locking EEPROM...");
    error = STI2C_Lock(HandleEEPROM, FALSE);
    CheckCodeOk (error);
    if (error == ST_NO_ERROR)
    {
        LockTestResult++;
    }

    /* Wait for signal from task 2 */
    STOS_SemaphoreWait(LockTestSem_p);

    /* Got it, wait to give the other task a chance to make the lock... */
    STOS_TaskDelay(WaitingTime);

    /* Release lock; should succeed */
    myprintf("Task 1: Unlocking EEPROM...\n");
    error = STI2C_Unlock(HandleEEPROM);
    CheckCodeOk(error);
    if (error == ST_NO_ERROR)
    {
        LockTestResult++;
    }
    STOS_TaskDelay(WaitingTime/2);
    /* At this point, the other task should have the lock - so we should
     * fail
     */
    myprintf("Task 1: Locking EEPROM...\n");
    error = STI2C_Lock(HandleEEPROM, FALSE);
    CheckCode(error, ST_ERROR_DEVICE_BUSY);
    if (error == ST_ERROR_DEVICE_BUSY)
    {
        LockTestResult++;
    }

    /* Try a transfer; should get device busy */
    {
        U32             ActLen;
        U8              WriteBuf[2];

        WriteBuf[0] = 0x0f;
        WriteBuf[1] = 0xc0;

        error = STI2C_Write(HandleEEPROM, WriteBuf, sizeof(WriteBuf),
                            WriteTimeout, &ActLen);
        if (error == ST_ERROR_DEVICE_BUSY)
        {
            LockTestResult++;
        }
    }
}

void LockTask2( void )
{
    U32 WaitingTime;
    ST_ErrorCode_t  error     = ST_NO_ERROR;

    WaitingTime = ST_GetClocksPerSecond() * 2;

    /* Pause, to give locktest1 a chance to obtain first lock */
    STOS_TaskDelay(ST_GetClocksPerSecond());

    /* Lock the bus; should fail */
    myprintf("Task 2: Trying to lock EEPROM2...\n");
    error = STI2C_Lock(HandleEEPROM2, FALSE);
    CheckCode(error, ST_ERROR_DEVICE_BUSY);
    if (error == ST_ERROR_DEVICE_BUSY)
    {
        LockTestResult++;
    }
    /* Signal the other task we're about to try waiting for a lock */
    STOS_SemaphoreSignal(LockTestSem_p);

    /* Then go for the lock */
    myprintf("Task 2: Trying to lock EEPROM2...\n");
    error = STI2C_Lock(HandleEEPROM2, TRUE);
    CheckCodeOk(error);
    if (error == ST_NO_ERROR)
    {
        LockTestResult++;
    }
    /* Hold on for a second to let the other task have a go... */
    STOS_TaskDelay(WaitingTime);

    /* Now release */
    myprintf("Task 2: Unlocking EEPROM2...\n");
    error = STI2C_Unlock(HandleEEPROM2);
    CheckCodeOk(error);
    if (error == ST_NO_ERROR)
    {
        LockTestResult++;
    }
}

void SpawnSameBusTest(void)
{
    myprintf("Attempting to start two tasks...");

    if(STOS_TaskCreate((void(*)(void*))WriteMsgA, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackOne_p,STACK_PARTITION_1,
                       &taskOne_p, &tdescOne, MAX_USER_PRIORITY, "FirstTask",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create first task\n");
    }

    if(STOS_TaskCreate((void(*)(void*))WriteMsgB, NULL, STACK_PARTITION_1,STACK_SIZE,&taskStackTwo_p,STACK_PARTITION_1,
                       &taskTwo_p, &tdescTwo,MAX_USER_PRIORITY, "SecondTask", (task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create second task\n");
    }

    myprintf("done\n");
}

void SpawnRepeatStartTest(void)
{
    myprintf("Attempting to start two tasks...");

    if(STOS_TaskCreate((void(*)(void*))ReadMsgE, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackOne_p, STACK_PARTITION_1,
                       &taskOne_p, &tdescOne, MAX_USER_PRIORITY, "FirstTask",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create first task\n");
    }

    STOS_TaskDelay( ClocksPerSec / 10);

    if(STOS_TaskCreate((void(*)(void*))ReadMsgF, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackTwo_p, STACK_PARTITION_1,
                      &taskTwo_p, &tdescTwo, MAX_USER_PRIORITY, "SecondTask",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create second task\n");
    }

    myprintf("done\n");
}

void SpawnDiffBusTest(void)
{
    myprintf("Attempting to start two tasks...");

    if(STOS_TaskCreate((void(*)(void*))WriteMsgC, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackThree_p, STACK_PARTITION_1,
                        &taskThree_p, &tdescThree, MAX_USER_PRIORITY, "ThirldTask",(task_flags_t)0)!=ST_NO_ERROR)

    {
        myprintf("\nError: task_create. Unable to create second task\n");
    }

    if(STOS_TaskCreate((void(*)(void*))WriteMsgD, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackFour_p, STACK_PARTITION_1,
                      &taskFour_p, &tdescFour,MAX_USER_PRIORITY, "FourthTask",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create second task\n");
    }

    myprintf("done\n");
}

void DoLockFuncTest(void)
{
    task_t *tasks[2];

    LockTestResult = ST_NO_ERROR;

    LockTestSem_p =  STOS_SemaphoreCreateFifo(NULL, 0);
    myprintf("Attempting to start two tasks...");

    if(STOS_TaskCreate((void(*)(void*))LockTask1, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackOne_p, STACK_PARTITION_1,
                        &taskOne_p, &tdescOne, MAX_USER_PRIORITY, "LockTask1",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create first task\n");
    }

    if(STOS_TaskCreate((void(*)(void*))LockTask2, NULL, STACK_PARTITION_1, STACK_SIZE, &taskStackTwo_p, STACK_PARTITION_1,
                       &taskTwo_p, &tdescTwo,MAX_USER_PRIORITY, "LockTask2",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create second task\n");
    }

    myprintf("done\n");

    tasks[0] = taskOne_p;
    tasks[1] = taskTwo_p;

    STOS_TaskWait(&tasks[0], TIMEOUT_INFINITY);

    STOS_TaskDelete(taskOne_p, STACK_PARTITION_1,
                    taskStackOne_p, STACK_PARTITION_1);

    STOS_TaskWait(&tasks[1], TIMEOUT_INFINITY);

    STOS_TaskDelete(taskTwo_p, STACK_PARTITION_1,
                     taskStackTwo_p, STACK_PARTITION_1);
    STOS_SemaphoreDelete(NULL, LockTestSem_p);

    if (LockTestResult != 7)
    {
        myprintf("Lock test failed - overall result %i\n",
                    LockTestResult);
    }
    else
    {
        myprintf("Lock test passed\n");
    }
}

/*---------------------------------------------------------------------------*/
/* Functions for R/W to Tuner & EEPROM.                                      */
/* For Tuner Write  :   1 byte address + 1 byte data                         */
/* For Tuner Read   :   Write 1 byte address; Read 1 byte data               */
/* For EEPROM Write :   2 byte address + N bytes data                        */
/* For EEPROM Read  :   Write 2 byte address with no STOP; Read N bytes data */
/* There is also a variant of the normal EEPROM read which reads the bytes   */
/* individually, in order to test STI2C_ReadNoStop.                          */
/*---------------------------------------------------------------------------*/

static ST_ErrorCode_t WriteTuner (U32 address, U8 data)
{
    ST_ErrorCode_t   code;
    U8               localBuff[2];
    U32              actLen;
    clock_t          Start;

    localBuff[0] = (U8) address;
    localBuff[1] = data;

    if (DoTunerTests == FALSE)
    {
        return ST_NO_ERROR;
    }
    myprintf("\nFrontbus (Tuner) - Trying to write %02x to address 0x%04x...\n",
           data, address);
    Start = STOS_time_now();
    STOS_TaskDelay(2);
    code = STI2C_Write( HandleTuner, localBuff, 2, WriteTimeout, &actLen );
    PrintDuration (Start, STOS_time_now());
    if( code != ST_NO_ERROR )
    {
        CheckCode( code, ST_NO_ERROR );
    }
    else
    {
        myprintf("Write returned %d and wrote %d bytes\n", code, actLen );
    }
    QueryTrace();
    return code;
}

static ST_ErrorCode_t WriteTuner2 (STI2C_Handle_t LocalHandleTuner,
                                   U32 address, U8 data)
{
    ST_ErrorCode_t   code;
    U8               localBuff[2];
    U32              actLen;

    localBuff[0] = (U8) address;
    localBuff[1] = data;

    code = STI2C_Write( LocalHandleTuner, localBuff, 2, WriteTimeout, &actLen );
    return code;
}

static ST_ErrorCode_t ReadTuner  (U32 address, U8 *pData)
{
    ST_ErrorCode_t   code;
    U32              actLen;
    clock_t          Start;

    if (DoTunerTests == FALSE)
        return ST_NO_ERROR;

    myprintf("\nFrontbus (Tuner) - Trying to read byte from address 0x%04x...\n",
           address);
    Start = STOS_time_now();
    code = STI2C_Write( HandleTuner, (U8 *) &address, 1, WriteTimeout,
                        &actLen );
    PrintDuration (Start, STOS_time_now());
    if( code != ST_NO_ERROR )
    {
        CheckCode( code, ST_NO_ERROR );
    }
    else
    {
        code = STI2C_Read (HandleTuner, pData, 1, ReadTimeout, &actLen);
        if (code != ST_NO_ERROR)
        {
            CheckCode( code, ST_NO_ERROR );
        }
        else
        {
            myprintf("Read returned %d and read %d bytes\n", code, actLen );
        }
    }
    QueryTrace();
    return code;
}

static ST_ErrorCode_t ReadTuner2 (STI2C_Handle_t HandleTuner,
                                  U32 address, U8 *pData)
{
    ST_ErrorCode_t   code;
    U32              actLen;

    code = STI2C_Write( HandleTuner, (U8 *) &address, 1, WriteTimeout,
                        &actLen );
    if( code == ST_NO_ERROR )
    {
        code = STI2C_Read(HandleTuner, pData, 1, ReadTimeout, &actLen);
    }
    return code;
}

static ST_ErrorCode_t WriteEEPROM (U32 address, U8 *buffer,
                                       U32 size, U32 *pActLen)
{
    ST_ErrorCode_t   code=0;
    U8               localBuff [1024];
    clock_t          Start;

    if (DoEEPROMTests == FALSE)
    {
        return ST_NO_ERROR;
    }
    myprintf("\nBackbus (EEPROM) - Trying to write %u bytes to address 0x%04x...\n",
            size, address);
    localBuff[0] = address >> 8;
    localBuff[1] = address & 0xFF;
    memcpy (&localBuff[2], buffer, size);
    size += 2;
    Start = STOS_time_now();
    code = STI2C_Write( HandleEEPROM, localBuff, size, WriteTimeout, pActLen);
    PrintDuration (Start, STOS_time_now());

    if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: STI2C_Write returned code %d\n", code);
        ErrorCount++;
    }
    else if (*pActLen != size)
    {
        myprintf("ERROR: %d bytes written, %d expected\n", *pActLen, size);
        ErrorCount++;
    }
    else
    {
        myprintf("Write returned %d and wrote %d bytes\n", code, *pActLen );
    }
    QueryTrace();
    return code;
}


static ST_ErrorCode_t ReadEEPROM (U32 address, U8 *buffer,
                                  U32 size, U32 *pActLen)
{
    ST_ErrorCode_t  code;
    U8              localBuff[2];
    U32             actLen;
    U32             i;
    clock_t         Start;

    if (DoEEPROMTests == FALSE)
    {
        return ST_NO_ERROR;
    }

    myprintf("\nBackbus (EEPROM) - Trying to read %u bytes from address 0x%04x...\n",
            size, address);
    localBuff[0] = address >> 8;
    localBuff[1] = address & 0xFF;
    for (i=0; i<size; i++)
    {
        buffer[i] = 0;
    }

    /* Polling algorithm during internal write cycle */
    code = -1;
    for (i = 0; i < 100 && code != ST_NO_ERROR; i++)
    {
        code = STI2C_WriteNoStop( HandleEEPROM, localBuff, 2, WriteTimeout,
                                &actLen );
    }
    if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: STI2C_WriteNoStop returned code %d\n", code);
        ErrorCount++;
    }
    else if (actLen != 2)
    {
        myprintf("ERROR: actLen for write = %d\n", actLen);
        ErrorCount++;
    }
    else
    {
        /* No errors during addressing, so start read op */
        Start = STOS_time_now();
        code = STI2C_Read (HandleEEPROM, buffer, size, ReadTimeout, pActLen);
        PrintDuration (Start, STOS_time_now());
        if (code != ST_NO_ERROR)
        {
            myprintf("ERROR: STI2C_read returned code %d\n", code);
            ErrorCount++;
        }
        else if (*pActLen != size)
        {
            myprintf("ERROR: %d bytes read, %d expected\n", *pActLen, size);
            ErrorCount++;
        }
        else
        {
            myprintf("Read returned %d and read %d bytes\n", code, *pActLen );
        }
    }
    QueryTrace();
    return code;
}

#if !defined(ST_OSLINUX) && !defined(ST_7200)
static ST_ErrorCode_t WriteEEPROM2 (STI2C_Handle_t Handle, U32 address,
                                    U8 *buffer, U32 size, U32 *pActLen)
{
    ST_ErrorCode_t   code;

    if (DoEEPROMTests == FALSE)
    {
        return ST_NO_ERROR;
    }
    stackLocalBuff[0] = address >> 8;
    stackLocalBuff[1] = address & 0xFF;
    memcpy (&stackLocalBuff[2], buffer, size);
    size += 2;

    code = STI2C_Write( Handle, stackLocalBuff, size, WriteTimeout, pActLen);

    if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: STI2C_Write returned code %x\n", code);
        ErrorCount++;
    }
    else if (*pActLen != size)
    {
        myprintf("ERROR: %d bytes written, %d expected\n", *pActLen, size);
        ErrorCount++;
    }
    return code;
}


static ST_ErrorCode_t ReadEEPROM2 (STI2C_Handle_t Handle, U32 address,
                                   U8 *buffer, U32 size, U32 *pActLen)
{
    ST_ErrorCode_t  code = ST_NO_ERROR;
    U32             actLen;
    U32             i;

    if (DoEEPROMTests == FALSE)
    {
        return ST_NO_ERROR;
    }
    stackLocalBuff[0] = address >> 8;
    stackLocalBuff[1] = address & 0xFF;

    /* Polling algorithm during internal write cycle */
    code = -1;
    for (i = 0; i < 100; i++)
    {
        code = STI2C_WriteNoStop( Handle, stackLocalBuff, 2, WriteTimeout,
                                  &actLen );
        if (code == ST_NO_ERROR)
        {
            break;
        }
    }

    if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: STI2C_WriteNoStop returned code %x\n", code);
        ErrorCount++;
    }
    else if (actLen != 2)
    {
        myprintf("ERROR: actLen for write = %d\n", actLen);
        ErrorCount++;
    }
    else
    {
        /* No errors during addressing, so start read op */
        code = STI2C_Read (Handle, buffer, size, ReadTimeout, pActLen);
        if (code != ST_NO_ERROR)
        {
            myprintf("ERROR: STI2C_read returned code %x\n", code);
            ErrorCount++;
        }
        else if (*pActLen != size)
        {
            myprintf("ERROR: %d bytes read, %d expected\n", *pActLen, size);
            ErrorCount++;
        }

    }
    return code;
}
#endif

static ST_ErrorCode_t ReadIndividualEEPROM (U32 address, U8 *buffer,
                                            U32 size, U32 *pActLen)
{
    ST_ErrorCode_t  code;
    U8              localBuff[2];
    U32             actLen;
    U32             i;
    clock_t         Start;

    myprintf("\nBackbus (EEPROM) - Trying to read %u bytes (individually) from address 0x%04x...\n",
            size, address);
    localBuff[0] = address >> 8;
    localBuff[1] = address & 0xFF;
    for (i=0; i<size; i++)
    {
        buffer[i] = 0;
    }
    *pActLen = 0;

    /* Polling algorithm during internal write cycle */
    code = -1;
    for (i = 0; i < 500 && code != ST_NO_ERROR; i++)
    {
        code = STI2C_WriteNoStop ( HandleEEPROM, localBuff, 2, WriteTimeout,
                                &actLen );
    }
    if (code != ST_NO_ERROR)
    {
        myprintf("ERROR: STI2C_WriteNoStop returned code %d\n", code);
        ErrorCount++;
    }
    else if (actLen != 2)
    {
        myprintf("ERROR: actLen for write = %d\n", actLen);
        ErrorCount++;
    }
    else
    {
        Start = STOS_time_now();
        /* Read bytes one by one */
        for (i=0; i<size-1; i++)
        {
            code = STI2C_ReadNoStop (HandleEEPROM, &buffer[i], 1, ReadTimeout,
                                     &actLen);
            if (code != ST_NO_ERROR)
            {
                myprintf("ERROR: STI2C_ReadNoStop returned code %d on byte %i\n", code, i + 1);
                ErrorCount++;
                break;
            }
            else
            {
                (*pActLen)++;
            }
        }
        PrintDuration (Start, STOS_time_now());

        if (code == ST_NO_ERROR)
        {
            /* Read last byte with stop */
            code = STI2C_Read (HandleEEPROM, &buffer[size-1], 1, ReadTimeout,
                               &actLen);
            if (code != ST_NO_ERROR)
            {
                myprintf("ERROR: STI2C_Read returned code %d\n", code);
                ErrorCount++;
            }
            else
            {
                (*pActLen)++;
                myprintf("Read returned %d and read %d bytes\n",
                          code, *pActLen);
            }
        }
    }
    QueryTrace();
    return code;
}

/**************************************************************************
       THIS FUNCTION WILL CALL ALL THE TEST ROUTINES...
**************************************************************************/

void DoTheTests(void)
{
    /* Declarations ---------------------------------------------------------*/
    STI2C_InitParams_t   I2cInitParams[TEST_NUM_SSC];
    STI2C_TermParams_t   I2cTermParams[TEST_NUM_SSC];

    /* Open blocks */
    STI2C_OpenParams_t   I2cOpenParams;
    STI2C_OpenParams_t   I2cOpenParams1;
    STI2C_OpenParams_t   I2cOpenParams2;
    STI2C_OpenParams_t   I2cOpenParams4;
    STI2C_OpenParams_t   I2cOpenParams5;

    STI2C_Params_t       I2cParams;
    STI2C_Params_t       I2cParams1;
    STI2C_Handle_t       Handle2;
    STI2C_Handle_t       Handle3;
    STI2C_Handle_t       Handle5;

    /* Read and write */
    U8                   WriteBuffer[6]       = "QWERT";
    U8                   WriteByteBuffer[1]   = {0xaa};
    U8                   WritePageBuffer[EEPROM_PAGE_SIZE];
    U8                   ReadBuffer[1024];

    U8                   WriteTunerData1      = 0x18;
    U8                   WriteTunerData2      = 0x34;

    U32                  i;
    U32                  ActLen               = 0;
    U32                  ErrorDelta;
    U32                  Seed;

    U32                  busloop;
    U8                   DevAdd;

    /* Initialisation ------------------------------------------------------*/

    /* Block 0 */
    I2cInitParams[0].BaseAddress           = (U32*)SSC_0_BASE_ADDRESS;
    I2cInitParams[0].InterruptNumber       = SSC_0_INTERRUPT;
    I2cInitParams[0].InterruptLevel        = SSC_0_INTERRUPT_LEVEL;
    I2cInitParams[0].MasterSlave           = STI2C_MASTER;
    I2cInitParams[0].ClockFrequency        = ClockSpeed;
    I2cInitParams[0].PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    I2cInitParams[0].PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;
    I2cInitParams[0].MaxHandles            = 6;
    I2cInitParams[0].DriverPartition       = TEST_PARTITION_1;
#ifdef BAUD_RATE_FAST
    I2cInitParams[0].BaudRate              = STI2C_RATE_FASTMODE;
#else
    I2cInitParams[0].BaudRate              = STI2C_RATE_NORMAL;
#endif
#ifdef TEST_FIFO
    I2cInitParams[0].FifoEnabled           = TRUE;
#else
    I2cInitParams[0].FifoEnabled           = FALSE;
#endif
    strcpy( I2cInitParams[0].PIOforSDA.PortName, PIODevName[SSC0_SDA_DEV] );
    strcpy( I2cInitParams[0].PIOforSCL.PortName, PIODevName[SSC0_SCL_DEV] );

#if (TEST_NUM_SSC > 1)
    I2cInitParams[1].BaseAddress          = (U32*)SSC_1_BASE_ADDRESS;
    I2cInitParams[1].InterruptNumber      = SSC_1_INTERRUPT;
    I2cInitParams[1].InterruptLevel       = SSC_1_INTERRUPT_LEVEL;
    I2cInitParams[1].MasterSlave          = STI2C_MASTER;
    I2cInitParams[1].ClockFrequency       = ClockSpeed;
    I2cInitParams[1].PIOforSDA.BitMask    = PIO_FOR_SSC1_SDA;
    I2cInitParams[1].PIOforSCL.BitMask    = PIO_FOR_SSC1_SCL;
    I2cInitParams[1].MaxHandles           = 6;
    I2cInitParams[1].DriverPartition      = TEST_PARTITION_1;
#ifdef BAUD_RATE_FAST
    I2cInitParams[1].BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParams[1].BaudRate             = STI2C_RATE_NORMAL;
#endif
#ifdef TEST_FIFO
    I2cInitParams[1].FifoEnabled          = TRUE;
#else
    I2cInitParams[1].FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParams[1].PIOforSDA.PortName, PIODevName[SSC1_SDA_DEV] );
    strcpy( I2cInitParams[1].PIOforSCL.PortName, PIODevName[SSC1_SCL_DEV] );
#endif

#if (TEST_NUM_SSC > 2)
    I2cInitParams[2].BaseAddress          = (U32*)SSC_2_BASE_ADDRESS;
    I2cInitParams[2].InterruptNumber      = SSC_2_INTERRUPT;
    I2cInitParams[2].InterruptLevel       = SSC_2_INTERRUPT_LEVEL;
    I2cInitParams[2].MasterSlave          = STI2C_MASTER;
    I2cInitParams[2].ClockFrequency       = ClockSpeed;
    I2cInitParams[2].PIOforSDA.BitMask    = PIO_FOR_SSC2_SDA;
    I2cInitParams[2].PIOforSCL.BitMask    = PIO_FOR_SSC2_SCL;
    I2cInitParams[2].MaxHandles           = 6;
    I2cInitParams[2].DriverPartition      = TEST_PARTITION_1;
#ifdef BAUD_RATE_FAST
    I2cInitParams[2].BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParams[2].BaudRate             = STI2C_RATE_NORMAL;
#endif
#ifdef TEST_FIFO
    I2cInitParams[2].FifoEnabled          = TRUE;
#else
    I2cInitParams[2].FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParams[2].PIOforSDA.PortName, PIODevName[SSC2_SDA_DEV] );
    strcpy( I2cInitParams[2].PIOforSCL.PortName, PIODevName[SSC2_SCL_DEV] );
#endif

#if (TEST_NUM_SSC > 3)
    I2cInitParams[3].BaseAddress          = (U32*)SSC_3_BASE_ADDRESS;
    I2cInitParams[3].InterruptNumber      = SSC_3_INTERRUPT;
    I2cInitParams[3].InterruptLevel       = SSC_3_INTERRUPT_LEVEL;
    I2cInitParams[3].MasterSlave          = STI2C_MASTER;
    I2cInitParams[3].ClockFrequency       = ClockSpeed;
    I2cInitParams[3].PIOforSDA.BitMask    = PIO_FOR_SSC3_SDA;
    I2cInitParams[3].PIOforSCL.BitMask    = PIO_FOR_SSC3_SCL;
    I2cInitParams[3].MaxHandles           = 6;
    I2cInitParams[3].DriverPartition      = TEST_PARTITION_1;
#ifdef BAUD_RATE_FAST
    I2cInitParams[3].BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParams[3].BaudRate             = STI2C_RATE_NORMAL;
#endif
#ifdef TEST_FIFO
    I2cInitParams[3].FifoEnabled          = TRUE;
#else
    I2cInitParams[3].FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParams[3].PIOforSDA.PortName, PIODevName[SSC3_SDA_DEV] );
    strcpy( I2cInitParams[3].PIOforSCL.PortName, PIODevName[SSC3_SCL_DEV] );
#endif

#if (TEST_NUM_SSC > 4)
    I2cInitParams[4].BaseAddress          = (U32*)SSC_4_BASE_ADDRESS;
    I2cInitParams[4].InterruptNumber      = SSC_4_INTERRUPT;
    I2cInitParams[4].InterruptLevel       = SSC_4_INTERRUPT_LEVEL;
    I2cInitParams[4].MasterSlave          = STI2C_MASTER;
    I2cInitParams[4].ClockFrequency       = ClockSpeed;
    I2cInitParams[4].PIOforSDA.BitMask    = PIO_FOR_SSC4_SDA;
    I2cInitParams[4].PIOforSCL.BitMask    = PIO_FOR_SSC4_SCL;
    I2cInitParams[4].MaxHandles           = 6;
    I2cInitParams[4].DriverPartition      = TEST_PARTITION_1;
#ifdef BAUD_RATE_FAST
    I2cInitParams[4].BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParams[4].BaudRate             = STI2C_RATE_NORMAL;
#endif
#ifdef TEST_FIFO
    I2cInitParams[4].FifoEnabled          = TRUE;
#else
    I2cInitParams[4].FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParams[4].PIOforSDA.PortName, PIODevName[SSC4_SDA_DEV] );
    strcpy( I2cInitParams[4].PIOforSCL.PortName, PIODevName[SSC4_SCL_DEV] );
#endif


#if (TEST_NUM_SSC > 5)
    I2cInitParams[5].BaseAddress          = (U32*)SSC_5_BASE_ADDRESS;
    I2cInitParams[5].InterruptNumber      = SSC_5_INTERRUPT;
    I2cInitParams[5].InterruptLevel       = SSC_5_INTERRUPT_LEVEL;
    I2cInitParams[5].MasterSlave          = STI2C_MASTER;
    I2cInitParams[5].ClockFrequency       = ClockSpeed;
    I2cInitParams[5].PIOforSDA.BitMask    = PIO_FOR_SSC5_SDA;
    I2cInitParams[5].PIOforSCL.BitMask    = PIO_FOR_SSC5_SCL;
    I2cInitParams[5].MaxHandles           = 6;
    I2cInitParams[5].DriverPartition      = TEST_PARTITION_1;
#ifdef BAUD_RATE_FAST
    I2cInitParams[5].BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParams[5].BaudRate             = STI2C_RATE_NORMAL;
#endif
#ifdef TEST_FIFO
    I2cInitParams[5].FifoEnabled          = TRUE;
#else
    I2cInitParams[5].FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParams[5].PIOforSDA.PortName, PIODevName[SSC5_SDA_DEV] );
    strcpy( I2cInitParams[5].PIOforSCL.PortName, PIODevName[SSC5_SCL_DEV] );
#endif

    myprintf("\nInitializing all the SSCs...\n\n");    
    for (i = 0; i < TEST_NUM_SSC; i++)
    {
        CheckCodeOk (STI2C_Init( I2CDevName[i], &I2cInitParams[i]) );
    }

    /* Open block 0 */
    I2cOpenParams.BusAccessTimeOut      = 1;
    /* Should be short for concurrency test */
    I2cOpenParams.AddressType           = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.I2cAddress            = EEPROM_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams.BaudRate              = STI2C_RATE_FASTMODE;
#endif
    /* Open block 1 */
    I2cOpenParams1.BusAccessTimeOut     = 1000;
                /* Should be long for concurrency test */
    I2cOpenParams1.AddressType          = STI2C_ADDRESS_7_BITS;
    I2cOpenParams1.I2cAddress           = EEPROM_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams1.BaudRate             = STI2C_RATE_NORMAL;
#endif
    /* Open block 2 */
    I2cOpenParams2.BusAccessTimeOut     = 20;
    I2cOpenParams2.AddressType          = STI2C_ADDRESS_7_BITS;
    I2cOpenParams2.I2cAddress           = EEPROM_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams2.BaudRate             = STI2C_RATE_FASTMODE;
#endif
    /* Open block 4 */
    I2cOpenParams4.BusAccessTimeOut     = 200;
    I2cOpenParams4.AddressType          = STI2C_ADDRESS_7_BITS;
    I2cOpenParams4.I2cAddress           = TUNER_DEVICE_ID;     /*default address need to be changed..?*/
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams4.BaudRate             = STI2C_RATE_FASTMODE;
#endif
    /* Open block 5 */
    I2cOpenParams5.BusAccessTimeOut     = 20;
    I2cOpenParams5.AddressType          = STI2C_ADDRESS_7_BITS;
    I2cOpenParams5.I2cAddress           = TUNER_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams5.BaudRate             = STI2C_RATE_NORMAL;
#endif
    /* Term blocks */
    for (i = 0; i < TEST_NUM_SSC; i++)
    {
        I2cTermParams[i].ForceTerminate        = FALSE;
    }

    /* Setup the write page buffer with a string */
    Seed = STOS_time_now() & 0xff;
    for ( i=2; i < sizeof(WritePageBuffer); i++ )
    {
        WritePageBuffer[i] = (Seed + i) & 0xFF;
    }
    WritePageBuffer[0] = WritePageBuffer[1] = 0;   /* address field */


    /* Program --------------------------------------------------------------*/
#ifdef TEST_FIFO
    myprintf("\nSTAPI I2C DRIVER TEST SUITE WITH FIFO\n\n");
#if (!defined (ST_5100)) && (!defined (ST_7710)) && (!defined (ST_5105)) && \
    (!defined (ST_5301)) && (!defined (ST_7100)) && (!defined (ST_8010)) && \
    (!defined (ST_7109)) && (!defined (ST_5188)) && (!defined (ST_5525)) && \
    (!defined (ST_5107)) && (!defined (ST_7200))
    myprintf("\nFIFO Feature is not supported\n\n");
    return;
#endif
#else
    myprintf("\nSTAPI I2C DRIVER TEST SUITE\n\n");
#endif


    myprintf("TEST_FRONT_BUS is set\n");

#if defined(UNIFIED_MEMORY)
    myprintf("Testing in unified memory\n");
#else
    myprintf("Testing in non-unified memory\n");
#endif
#ifdef LEAK_TEST
    myprintf("LEAK_TEST is set\n");
#else
    myprintf("LEAK_TEST is unset\n");
#endif /* LEAK_TEST */

    /*----------------------------------------------------------------------*/
    /* Test GetRevision */

    myprintf("GetRevision() returns: ");
    myprintf( (char*)STI2C_GetRevision() );
    myprintf("\n\n");

    /* See what hardware we've got where */
    BusProbe(I2cInitParams);

    for (i = 0; i < TEST_NUM_SSC; i++)
    {
        CheckCodeOk (STI2C_Term( I2CDevName[i], &I2cTermParams[i]) );
    }
    
    if (DoTunerTests == TRUE)
    {
        DoTheTunerTest(); /*In case not ext test not defined empty call */
    }

    /* Do the stack tests */
#if !defined(ST_OSLINUX)
#ifndef ST_7200
    StackTest();
#endif
#endif
    /*----------------------------------------------------------------------*/
    myprintf("\nTesting API\n\n");

    myprintf("Testing Open before 1st Init...\n");
    /* test open before init condition */
    myprintf("Open before Init test returns...\n");
    CheckCode (STI2C_Open( I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3 ),
               ST_ERROR_UNKNOWN_DEVICE );

   /* Test all return codes for init */

    myprintf("\nTesting Init...\n\n");

   /* device initialized */
    myprintf("Device initialized test returns...\n");

    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_NO_ERROR);
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_ALREADY_INITIALIZED );
               
    STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] );

    /* bad params */
#if !defined(ST_8010)&&!defined(ST_5525)&&!defined(ST_OSLINUX)
    myprintf("Bad params test returns...\n");
    I2cInitParams[SSC_DEVICE_0].PIOforSDA.BitMask = 0;
    CheckCode (STI2C_Init (I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER );
    STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] );
#endif
    I2cInitParams[SSC_DEVICE_0].PIOforSDA.BitMask = PIO_FOR_SSC0_SDA;

#ifndef STI2C_NO_PARAM_CHECK
    myprintf("NULL name...\n");
    CheckCode(STI2C_Init(NULL, &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER);
    myprintf("Name too long...\n");
    CheckCode(STI2C_Init("ThisNameIsWayTooLong", &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER);
    myprintf("NULL initparams...\n");
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], NULL), ST_ERROR_BAD_PARAMETER);
#ifndef ST_OSLINUX
    myprintf("Bad clock frequency (0)...\n");
    I2cInitParams[SSC_DEVICE_0].ClockFrequency = 0;
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]),ST_ERROR_BAD_PARAMETER);
    I2cInitParams[SSC_DEVICE_0].ClockFrequency = ClockSpeed;

    myprintf("MaxHandles of 0...\n");
    I2cInitParams[SSC_DEVICE_0].MaxHandles = 0;
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER);
    I2cInitParams[SSC_DEVICE_0].MaxHandles = 2;

    myprintf("Bad slave address...\n");
    I2cInitParams[SSC_DEVICE_0].MasterSlave = STI2C_SLAVE;
    I2cInitParams[SSC_DEVICE_0].SlaveAddress =0;
#ifdef STI2C_MASTER_ONLY
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_FEATURE_NOT_SUPPORTED);
#else
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER);
#endif
    I2cInitParams[SSC_DEVICE_0].MasterSlave = STI2C_MASTER;
    I2cInitParams[SSC_DEVICE_0].SlaveAddress = 0;

#if !defined(ST_8010)&&!defined(ST_5525)
    myprintf("Too High Mask...\n");
    I2cInitParams[SSC_DEVICE_0].PIOforSDA.BitMask     = 0;
    I2cInitParams[SSC_DEVICE_0].PIOforSCL.BitMask     = 0;
    CheckCode(STI2C_Init(I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER);
#endif
    I2cInitParams[SSC_DEVICE_0].PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    I2cInitParams[SSC_DEVICE_0].PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;

    myprintf("Null partition test returns...\n");
    I2cInitParams[SSC_DEVICE_0].DriverPartition = NULL;
    CheckCode (STI2C_Init (I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_BAD_PARAMETER );
#endif
#endif /*STI2C_NO_PARAM_CHECK*/

    STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] );
#ifndef ST_OSLINUX
#if defined(ST_OS21)
    I2cInitParams[SSC_DEVICE_0].DriverPartition       = (ST_Partition_t *)system_partition_p;
#else
    I2cInitParams[SSC_DEVICE_0].DriverPartition       = TEST_PARTITION_1;
#endif

    /* partition too small */
    myprintf("Tiny partition test returns...\n");
    I2cInitParams[SSC_DEVICE_0].DriverPartition       = TINY_PARTITION_1 ;
    CheckCode (STI2C_Init (I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]), ST_ERROR_NO_MEMORY );
    STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] );

#if defined(ST_OS21)
    I2cInitParams[SSC_DEVICE_0].DriverPartition       = (ST_Partition_t *)system_partition_p;
#else
    I2cInitParams[SSC_DEVICE_0].DriverPartition       = TEST_PARTITION_1;
#endif

    /* line state - Only works on boards with no pull up resistors and no
       devices attached to the front bus */

#ifdef SSC_1_PRESENT
#ifdef TEST_LINE_STATE
    myprintf("Line state test returns...\n");
    CheckCode (STI2C_Init(I2CDevName[SSC_DEVICE_1], &I2cInitParams[SSC_DEVICE_1]), STI2C_ERROR_LINE_STATE );
    STI2C_Term( I2CDevName[SSC_DEVICE_1], &I2cTermParams[SSC_DEVICE_1] );
#endif
#endif
#endif
    /* no error */
    myprintf("Valid Init - Master Mode...\n");
    I2cInitParams[SSC_DEVICE_0].MaxHandles = 1;
    CheckCodeOk (STI2C_Init ( I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]) );
    I2cInitParams[SSC_DEVICE_0].MaxHandles = 6;


    /*----------------------------------------------------------------------*/
    /* Test all return codes for open */

    myprintf("\nTesting Open...\n\n");

#ifndef STI2C_NO_PARAM_CHECK
    myprintf("NULL name...\n");
    CheckCode(STI2C_Open(NULL, &I2cOpenParams, &Handle3), ST_ERROR_BAD_PARAMETER);

    myprintf("Name too long...\n");
    CheckCode(STI2C_Open("ThisNameIsWayTooLong", &I2cOpenParams, &Handle3),
                         ST_ERROR_BAD_PARAMETER);

    myprintf("NULL OpenParams...\n");
    CheckCode(STI2C_Open( I2CDevName[SSC_DEVICE_0], NULL, &Handle3), ST_ERROR_BAD_PARAMETER);

    myprintf("NULL Handle...\n");
    CheckCode(STI2C_Open( I2CDevName[SSC_DEVICE_0], &I2cOpenParams, NULL), ST_ERROR_BAD_PARAMETER);
    /* bad params */
#ifndef ST_OSLINUX
    myprintf("Bad params test returns...\n");
    I2cOpenParams.AddressType = STI2C_ADDRESS_10_BITS;
    CheckCode (STI2C_Open(  I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3 ),ST_ERROR_FEATURE_NOT_SUPPORTED );
    I2cOpenParams.AddressType = STI2C_ADDRESS_7_BITS;
#endif
#endif /*STI2C_NO_PARAM_CHECK*/

    /* unknown devicename */
    myprintf("Devicename test returns...\n");
    CheckCode (STI2C_Open( "UNKNOWN_DEVICE", &I2cOpenParams, &Handle3 ),
               ST_ERROR_UNKNOWN_DEVICE );

    /* no free handles */
    myprintf("No free handles test returns...\n");
    STI2C_Open(  I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle2);
    CheckCode (STI2C_Open(  I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3),
               ST_ERROR_NO_FREE_HANDLES );
    STI2C_Close( Handle2 );
    /*STI2C_Close( Handle3 );*/

    /* no error */
    myprintf("No error test returns...\n");
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams.BaudRate = STI2C_RATE_NORMAL;
#endif
    CheckCodeOk (STI2C_Open(  I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3) );

    /* Check failed init does not affect an existing successful init */
    myprintf("\nTesting 'Already Initialized' error behaviour...\n\n");
    myprintf("Close Handle3...\n");
    CheckCodeOk (STI2C_Close (Handle3));
    myprintf("Already Init...\n");
    CheckCode (STI2C_Init ( I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]),
               ST_ERROR_ALREADY_INITIALIZED );
    myprintf("Open after 'Already Init'...\n");
    CheckCodeOk (STI2C_Open( I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3) );


    /*----------------------------------------------------------------------*/
    /* Test all return codes for write */

    myprintf("\nTesting Write...\n\n");

    /* Invalid handle */
    myprintf("Invalid handle test returns...\n");
    CheckCode (STI2C_Write (HandleEEPROM, WritePageBuffer,
                            sizeof(WritePageBuffer), WriteTimeout, &ActLen),
                            ST_ERROR_INVALID_HANDLE );
#ifndef STI2C_NO_PARAM_CHECK

    /* Null buffer */
    myprintf("Data buffer null...\n");
    CheckCode (STI2C_Write (Handle3, NULL,
                            sizeof(WritePageBuffer), WriteTimeout, &ActLen),
                            ST_ERROR_BAD_PARAMETER );
    /* Null data length */
    myprintf("Data length written ptr null...\n");
    CheckCode (STI2C_Write (Handle3, WritePageBuffer,
                            sizeof(WritePageBuffer), WriteTimeout, NULL),
                            ST_ERROR_BAD_PARAMETER );
#endif

    /* line state */
    /* Write timeout */
    /* No error */
#if !defined(ST_5188) && !defined(ST_7200)
    if (FindEEPROM() >= 0)
    {
        myprintf("No error test returns...\n");
        CheckCodeOk (STI2C_Write (Handle3, WritePageBuffer,
                             sizeof(WritePageBuffer), WriteTimeout, &ActLen));
    }
#endif

#if defined(STI2C_TRACE)
    QueryTrace();
#endif

    /*----------------------------------------------------------------------*/
    /* Test all return codes for read */

    myprintf("\nTesting Read...\n\n");

    /* Invalid handle */
    myprintf("Invalid handle test returns...\n");
    CheckCode (STI2C_Read (HandleEEPROM, ReadBuffer, EEPROM_PAGE_SIZE,
                           ReadTimeout, &ActLen), ST_ERROR_INVALID_HANDLE );

#ifndef STI2C_NO_PARAM_CHECK

    /* Null buffer */
    myprintf("Data buffer null...\n");
    CheckCode (STI2C_Read (Handle3, NULL,
                            sizeof(WritePageBuffer), WriteTimeout, &ActLen),
                            ST_ERROR_BAD_PARAMETER );
    /* Null data length */
    myprintf("Data length read ptr null...\n");
    CheckCode (STI2C_Read (Handle3, WritePageBuffer,
                            sizeof(WritePageBuffer), WriteTimeout, NULL),
                            ST_ERROR_BAD_PARAMETER );
#endif

    /* line state */
    /* Write timeout */
    /* No error */
#if !defined(ST_5188) && !defined(ST_7200)
    if (FindEEPROM() >= 0)
    {
        myprintf("No error test returns...\n");
        CheckCodeOk (STI2C_Read (Handle3, ReadBuffer, EEPROM_PAGE_SIZE,
                             ReadTimeout, &ActLen) );
    }
#endif
    QueryTrace();

    /*----------------------------------------------------------------------*/
    /* Test all return codes for GetParams */

    myprintf("\nTesting GetParams...\n\n");

    /* Invalid handle */
    myprintf("Invalid handle test returns...\n");
    CheckCode (STI2C_GetParams (HandleEEPROM, &I2cParams), ST_ERROR_INVALID_HANDLE );

#ifndef STI2C_NO_PARAM_CHECK
    /* Null getparams */
    myprintf("Null getparams...\n");
    CheckCode (STI2C_GetParams (Handle3, NULL), ST_ERROR_BAD_PARAMETER );
#endif

    /* No error */
    myprintf("No error test returns...\n");
    CheckCodeOk (STI2C_GetParams (Handle3, &I2cParams) );
   /*----------------------------------------------------------------------*/
    /* Test all return codes for SetParams */

    myprintf("\nTesting SetParams...\n\n");

    /* Invalid handle */
    myprintf("Invalid handle test returns...\n");
    CheckCode (STI2C_SetParams (HandleEEPROM, &I2cParams), ST_ERROR_INVALID_HANDLE );

#ifndef STI2C_NO_PARAM_CHECK

    /* Null setparams */
    myprintf("Null setparams...\n");
    CheckCode (STI2C_SetParams (Handle3, NULL), ST_ERROR_BAD_PARAMETER );

    /* Bad params */
    myprintf("Bad params test returns...\n");
    I2cParams.AddressType = STI2C_ADDRESS_10_BITS;
    CheckCode (STI2C_SetParams (Handle3, &I2cParams), ST_ERROR_BAD_PARAMETER );
    I2cParams.AddressType = STI2C_ADDRESS_7_BITS;
#endif

    /* No error */
    myprintf("No error test returns...\n");
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cParams.BaudRate = STI2C_RATE_NORMAL;
#endif
    CheckCodeOk (STI2C_SetParams (Handle3, &I2cParams) );

   /*----------------------------------------------------------------------*/
    /* Test all return codes for close */

    myprintf("\nTesting Close...\n\n");

    /* Invalid handle */
    myprintf("Invalid handle test returns...\n");
    CheckCode (STI2C_Close (HandleEEPROM), ST_ERROR_INVALID_HANDLE );

    /* No error */
    myprintf("No error test returns...\n");
    CheckCodeOk (STI2C_Close (Handle3) );

    /*----------------------------------------------------------------------*/
    /* Test all return codes for term */

    myprintf("\nTesting Term...\n\n");

#ifndef STI2C_NO_PARAM_CHECK

    /* Null Name */
    myprintf("Null Name...\n");
    CheckCode (STI2C_Term (NULL, &I2cTermParams[SSC_DEVICE_0]),
               ST_ERROR_BAD_PARAMETER);

    /* too long name */
    /*myprintf("Too long name...\n");
    CheckCode (STI2C_Term ("ThisNameIsWayTooLong", &I2cTermParams),
               ST_ERROR_BAD_PARAMETER);*/

    /* null params */
    myprintf("Too long name...\n");
    CheckCode (STI2C_Term (I2CDevName[SSC_DEVICE_0], NULL),
               ST_ERROR_BAD_PARAMETER);
#endif

    /* Not initialised */
    myprintf("Not initialised test returns...\n");
    CheckCode (STI2C_Term ("NOT_INITIALIZED", &I2cTermParams[SSC_DEVICE_0]),
               ST_ERROR_UNKNOWN_DEVICE);

    /* Terminating */
    myprintf("Error terminating test returns...\n");
    I2cTermParams[SSC_DEVICE_0].ForceTerminate = FALSE;
    STI2C_Open( I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3 );
    CheckCode (STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] ), ST_ERROR_OPEN_HANDLE );

    /* No error */
    myprintf("No error test returns...\n");
    STI2C_Close (Handle3);
    CheckCodeOk (STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] ) );

    /* Force terminate */
    STI2C_Init( I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0] );
    myprintf("Force terminate test returns...\n");
    STI2C_Open( I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3 );
    I2cTermParams[SSC_DEVICE_0].ForceTerminate = TRUE;
    CheckCodeOk (STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0]) );


    /*-----------------------------------------------------------------------*/
    /* Init and term tests - This section shows that the driver can be       */
    /* initialised & terminated multiple times for two buses without failing */

    myprintf("\nInitialisation and termination\n\n");
    for (i = 0; i < TEST_NUM_SSC; i++)
    {
        /* Init block 0 */
        myprintf("Initialising bus %i...\n", i);
        CheckCodeOk (STI2C_Init( I2CDevName[i], &I2cInitParams[i]) );

#ifdef DISPLAY_INIT_CHAIN
        DumpInitList();
#endif

#ifdef DISPLAY_OPEN_CHAIN
        DumpOpenList();
#endif
    }

    /* Term block 0 */
    myprintf("Terminating bus 0...\n");
    CheckCodeOk (STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0] ) );

#ifdef DISPLAY_INIT_CHAIN
    DumpInitList();
#endif

#ifdef DISPLAY_OPEN_CHAIN
    DumpOpenList();
#endif

    myprintf("Initialising bus 0...\n");
    CheckCodeOk (STI2C_Init( I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0] ) );

#ifdef DISPLAY_INIT_CHAIN
    DumpInitList();
#endif

#ifdef DISPLAY_OPEN_CHAIN
    DumpOpenList();
#endif

    myprintf("\n");

    /*----------------------------------------------------------------------*/
    /* Open and close tests - This section shows that handles can be opened
       and closed on two buses. No problem just opening handles for devices
       that don't exist (only trying to use them), so this section left
       unmodified.                                                          */

    myprintf("\nOpening and closing\n\n");

    myprintf("Opening Handle3 (bus 0)...\n");
    CheckCodeOk (STI2C_Open(I2CDevName[SSC_DEVICE_0] , &I2cOpenParams, &Handle3 ) );

    myprintf("Opening Handle5 (bus 0)...\n");
    CheckCodeOk (STI2C_Open(I2CDevName[SSC_DEVICE_0] , &I2cOpenParams5, &Handle5 ) );

    myprintf("Opening Handle2 (bus 0)...\n");
    CheckCodeOk (STI2C_Open(I2CDevName[SSC_DEVICE_0] , &I2cOpenParams2, &Handle2 ) );

    if (FindEEPROM() >= 0)
    {
        S8 EEPROMBus = FindEEPROM();

        myprintf("Opening HandleEEPROM2 (bus %i)...\n", EEPROMBus);
        CheckCodeOk (STI2C_Open(I2CDevName[EEPROMBus], &I2cOpenParams1, &HandleEEPROM2));

        /* Open for next test section */
        myprintf("Opening HandleEEPROM (bus %i)...\n", EEPROMBus);
#ifdef STI2C_ADAPTIVE_BAUDRATE
        I2cOpenParams.BaudRate = STI2C_RATE_FASTMODE;
#endif
        CheckCodeOk (STI2C_Open(I2CDevName[EEPROMBus], &I2cOpenParams, &HandleEEPROM));
    }
    else
    {
        myprintf("No EEPROMs found, disabling tests\n");
        DoEEPROMTests = FALSE;
    }

    if (FindTuner() >= 0)
    {
        S8 TunerBus = FindTunerAddress(&DevAdd);
        I2cOpenParams4.I2cAddress = DevAdd;
#ifdef STI2C_ADAPTIVE_BAUDRATE
        I2cOpenParams4.BaudRate = STI2C_RATE_FASTMODE;
#endif
        myprintf("Opening HandleTuner (bus %i)...\n", TunerBus);
        CheckCodeOk (STI2C_Open(I2CDevName[TunerBus], &I2cOpenParams4, &HandleTuner));
    }
    else
    {
        myprintf("No tuners found, disabling tests\n");
        DoTunerTests = FALSE;
    }

#ifdef DISPLAY_OPEN_CHAIN
    DumpOpenList();
#endif

    /* Must leave HandleEEPROM, HandleEEPROM2 & HandleTuner open as used by
       other tests... */
    myprintf("Closing Handle3 (bus 0)...\n");
    CheckCodeOk (STI2C_Close( Handle3 ) );

    myprintf("Closing Handle2 (bus 0)...\n");
    CheckCodeOk (STI2C_Close( Handle2 ) );

    myprintf("Closing Handle5 (bus 0)...\n");
    CheckCodeOk (STI2C_Close(Handle5));

#ifdef DISPLAY_OPEN_CHAIN
    DumpOpenList();
#endif

    myprintf("\n");

    /*----------------------------------------------------------------------*/
    /* GetParams & SetParams test - This section show that the operating    */
    /* parameters can be returned from the driver and set to new values     */
    /* No read/writes are performed, so addresses used do not have to be    */
    /* valid                                                                */

    if(DoEEPROMTests)
    {
        myprintf("Getting and setting parameters tests\n\n");

        myprintf( "Calling GetParams ...\n" );
        CheckCodeOk( STI2C_GetParams( HandleEEPROM, &I2cParams ) );
        if( I2cParams.I2cAddress != I2cOpenParams.I2cAddress )
        {
            myprintf( "ERROR: Address returned does not match set value\n");
            ErrorCount++;
        }
        if( I2cParams.AddressType != I2cOpenParams.AddressType )
        {
            myprintf( "ERROR: Address Type returned does not match set value\n");
            ErrorCount++;
        }
        if( I2cParams.BusAccessTimeOut != I2cOpenParams.BusAccessTimeOut )
        {
            myprintf( "ERROR: Bus access timeout returned does not match set value\n");
            ErrorCount++;
        }

        I2cParams1.BusAccessTimeOut     = 100;
        I2cParams1.AddressType          = STI2C_ADDRESS_7_BITS;
        I2cParams1.I2cAddress           = TUNER_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
        I2cParams1.BaudRate             = STI2C_RATE_FASTMODE;
#endif
        myprintf( "Calling SetParams...\n" );
        CheckCodeOk( STI2C_SetParams( HandleEEPROM, &I2cParams1 ) );
        myprintf( "Checking with GetParams ...\n" );
        CheckCodeOk( STI2C_GetParams( HandleEEPROM, &I2cParams ) );
        if( I2cParams.I2cAddress != I2cParams1.I2cAddress )
        {
            myprintf( "ERROR: Address returned does not match set value\n");
            ErrorCount++;
        }
        if( I2cParams.AddressType != I2cParams1.AddressType )
        {
            myprintf( "ERROR: Address Type returned does not match set value\n");
            ErrorCount++;
        }

        if( I2cParams.BusAccessTimeOut != I2cParams1.BusAccessTimeOut )
        {
            myprintf( "ERROR: Bus access timeout returned does not match set value\n");
            ErrorCount++;
        }

        /* Return parameters to original values */
        I2cParams.I2cAddress = I2cOpenParams.I2cAddress;
        I2cParams.AddressType = I2cOpenParams.AddressType;
        I2cParams.BusAccessTimeOut = I2cOpenParams.BusAccessTimeOut;
#ifdef STI2C_ADAPTIVE_BAUDRATE
        I2cParams.BaudRate         = STI2C_RATE_NORMAL;
#endif
        STI2C_SetParams( HandleEEPROM, &I2cParams );
        STOS_TaskDelay(10);
    }

    /*----------------------------------------------------------------------*/
    /* Write tests - This section shows that data of various lengths can be */
    /* written to devices on the i2c buses                                  */


    myprintf("\nWriting and reading tests\n\n");

    if (DoEEPROMTests == TRUE)
    {

        WriteEEPROM (0x0140, WriteBuffer, sizeof(WriteBuffer), &ActLen);
        WriteEEPROM (0x1357, WriteByteBuffer, sizeof(WriteByteBuffer), &ActLen);
        WriteEEPROM (0x0760, WritePageBuffer, sizeof(WritePageBuffer), &ActLen);
    }
{ 
	U32 counter=0;
	while(counter<2)
	{
    		if (DoTunerTests == TRUE)
    		{
        		WriteTuner (TUNER_OFFSET_1, WriteTunerData1);
        		WriteTuner (TUNER_OFFSET_2, WriteTunerData2);
        		task_delay(1*ST_GetClocksPerSecond());
#ifdef STI2C_ADAPTIVE_BAUDRATE
        		STI2C_SetBaudRate(HandleTuner,STI2C_RATE_FASTMODE);
#endif
    		}
   		if (DoTunerTests == TRUE)
    		{
        		ReadTuner (TUNER_OFFSET_1, ReadBuffer);
        		CompareData (ReadBuffer, &WriteTunerData1, 1);
        		STOS_TaskDelay(10);
        		ReadTuner (TUNER_OFFSET_2, ReadBuffer);
        		CompareData (ReadBuffer, &WriteTunerData2, 1);
        		task_delay(1*ST_GetClocksPerSecond());
#ifdef STI2C_ADAPTIVE_BAUDRATE
        		STI2C_SetBaudRate(HandleTuner,STI2C_RATE_NORMAL);
#endif
    		}
		counter++;
	}
}

    /*----------------------------------------------------------------------*/
    /* Read tests - This section shows that data of various lengths can be
       read from i2c devices. Note that every read must be preceded by a dummy
       write to set the address to be read from inside the device.          */

    WriteEEPROM(0x140, WriteBuffer, sizeof(WriteBuffer), &ActLen);
    ReadEEPROM (0x140,  ReadBuffer, sizeof(WriteBuffer), &ActLen);

    if (DoEEPROMTests == TRUE)
    {
        ReadEEPROM (0x140,  ReadBuffer, sizeof(WriteBuffer), &ActLen);
        CompareData (ReadBuffer, WriteBuffer, sizeof(WriteBuffer));
        ReadEEPROM (0x140,  ReadBuffer, sizeof(WriteBuffer), &ActLen);
        CompareData (ReadBuffer, WriteBuffer, sizeof(WriteBuffer));

        ReadEEPROM (0x1357, ReadBuffer, sizeof(WriteByteBuffer), &ActLen);
        CompareData (ReadBuffer, WriteByteBuffer, sizeof(WriteByteBuffer));
        ReadEEPROM (0x1357, ReadBuffer, sizeof(WriteByteBuffer), &ActLen);
        CompareData (ReadBuffer, WriteByteBuffer, sizeof(WriteByteBuffer));

        ReadEEPROM (0x760,  ReadBuffer, sizeof(WritePageBuffer), &ActLen);
        CompareData (ReadBuffer, WritePageBuffer, sizeof(WritePageBuffer));
        ReadIndividualEEPROM (0x760, ReadBuffer, sizeof(WritePageBuffer), &ActLen);
        CompareData (ReadBuffer, WritePageBuffer, sizeof(WritePageBuffer));
        ReadEEPROM (0x760,  ReadBuffer, sizeof(WritePageBuffer), &ActLen);
        CompareData (ReadBuffer, WritePageBuffer, sizeof(WritePageBuffer));
        ReadIndividualEEPROM (0x760, ReadBuffer, sizeof(WritePageBuffer), &ActLen);
        CompareData (ReadBuffer, WritePageBuffer, sizeof(WritePageBuffer));
        ReadEEPROM (0x760,  ReadBuffer, sizeof(WritePageBuffer), &ActLen);
        CompareData (ReadBuffer, WritePageBuffer, sizeof(WritePageBuffer));
    }
    myprintf("\n");

#ifdef LOCK_TEST
    myprintf("\nStarting Multiple Lock tests...\n");
    {
        U8                      localBuff[2];
        U32                     HandleNo;
        STI2C_OpenParams_t      I2cLockOpenParams;
        STI2C_Handle_t          HandlePool[10];
        S8                      TunerBus;
        localBuff[0] = 0x01;
        localBuff[1] = 0x18;
        if (FindTuner() >= 0)
        {
            TunerBus = FindTunerAddress(&DevAdd);
            I2cLockOpenParams.BusAccessTimeOut     = 200;
            I2cLockOpenParams.AddressType          = STI2C_ADDRESS_7_BITS;
            I2cLockOpenParams.I2cAddress = DevAdd;
        #ifdef STI2C_ADAPTIVE_BAUDRATE
            I2cLockOpenParams.BaudRate = STI2C_RATE_FASTMODE;
        #else
            I2cLockOpenParams.BaudRate = STI2C_RATE_NORMAL;
        #endif
            I2cInitParams[TunerBus].MaxHandles = 4;
            for(HandleNo = 0; HandleNo < I2cInitParams[TunerBus].MaxHandles; HandleNo++)
            {
                myprintf("Opening Handle %d\n",HandleNo);
                CheckCodeOk (STI2C_Open(I2CDevName[TunerBus] , &I2cLockOpenParams, &HandlePool[HandleNo]));
            }
            myprintf("Locking all Handles except last\n");
            CheckCodeOk (STI2C_LockMultiple(HandlePool, I2cInitParams[TunerBus].MaxHandles-1, TRUE));

            for(HandleNo = 0; HandleNo < I2cInitParams[TunerBus].MaxHandles-1; HandleNo++)
            {
                myprintf("Perform Write on - Handle %d\n",HandleNo);
                CheckCodeOk (STI2C_Write(HandlePool[HandleNo],localBuff,2,1000,&ActLen));
            }
            myprintf("Perform Write on Unlocked handle - Handle %d\n",I2cInitParams[TunerBus].MaxHandles-1);
            CheckCode(STI2C_Write(HandlePool[I2cInitParams[TunerBus].MaxHandles-1],localBuff,2,1000,&ActLen),ST_ERROR_DEVICE_BUSY);
            myprintf("UnLocking all Locked Handles \n");
            CheckCodeOk (STI2C_UnlockMultiple(HandlePool, I2cInitParams[TunerBus].MaxHandles-1 ));
            myprintf("Perform Write on Unlocked handle again - Handle %d\n",I2cInitParams[TunerBus].MaxHandles-1);
            CheckCodeOk (STI2C_Write(HandlePool[I2cInitParams[TunerBus].MaxHandles-1],localBuff,2,1000,&ActLen));
            myprintf("Locking only Handle1\n");
            CheckCodeOk (STI2C_Lock(HandlePool[1],TRUE));
            myprintf("Locking all Handles \n");
            CheckCode(STI2C_LockMultiple(HandlePool, I2cInitParams[TunerBus].MaxHandles, FALSE),ST_ERROR_DEVICE_BUSY);
            myprintf("Perform Write on Handle 1\n");
            CheckCodeOk (STI2C_Write(HandlePool[1],localBuff,2,1000,&ActLen));
            myprintf("UnLocking only Handle1\n");
            CheckCodeOk (STI2C_Unlock(HandlePool[1]));
            myprintf("UnLocking all Locked Handles \n");
            CheckCode(STI2C_UnlockMultiple(HandlePool, I2cInitParams[TunerBus].MaxHandles),ST_ERROR_BAD_PARAMETER);
        }
        else
        {
            myprintf("No tuners found, disabling tests\n");
        }
    }

    /*----------------------------------------------------------------------*/
    /* Test simultaneous read and write operations by spawning two tasks
       which try to write to the same device at the same time. Half of the
       writes use a handle opened with a short timeout & the other half use a
       handle opened with a long timeout. The short timeout writes will
       generally fail with 'Device Busy', but the long timeout writes should
       succeed. So at most half the writes should fail.  */

    myprintf("\nStarting concurrent access to same bus tests...\n");

    TaskaSem_p=STOS_SemaphoreCreatePriority(NULL, 0);
    TaskbSem_p=STOS_SemaphoreCreatePriority(NULL, 0);

    BusyCount  = SuccessCount = 0;
    ErrorDelta = ErrorCount;
    SpawnSameBusTest();

    /* Semaphores to hold main task until test tasks finish */
    STOS_SemaphoreWait(TaskaSem_p);
    STOS_SemaphoreWait(TaskbSem_p);

    myprintf("\nConcurrency test stage 1 completed\n");
    if( (ErrorCount - ErrorDelta) > 0 )
    {
        myprintf("%4u errors occurred - STAGE 1 FAILED\n",
                     ErrorCount - ErrorDelta);
    }
    myprintf("%4u occurrences of 'device busy'\n", BusyCount);
    myprintf("%4u occurrences of 'no error'\n", SuccessCount);
    PrintDuration (Start1, Finish1);
    PrintDuration (Start2, Finish2);
    QueryTrace();

    STOS_TaskDelete(taskOne_p, STACK_PARTITION_1,
                    taskStackOne_p, STACK_PARTITION_1);
    STOS_TaskDelete(taskTwo_p, STACK_PARTITION_1,
                    taskStackTwo_p, STACK_PARTITION_1);

    /* Not more than half the operations should result in 'busy'. */
    if (BusyCount > 2 * TASK_WRITE_CYCLES)
    {
        myprintf("ERROR: Too many 'device busy' detected - STAGE 1 FAILED\n");
        ErrorCount++;
    }
    if( (ErrorCount - ErrorDelta) == 0 )
    {
        myprintf("No errors, stage 1 passed\n");
    }

    /*----------------------------------------------------------------------*/
    /* This test writes to device handles on different buses at the same time
       so all elements of it should succeed                                */
#if !defined (STI2C_STSPI_SAME_SSC_SUPPORT)
    myprintf("\nStarting concurrent access to different bus tests...\n");
    if ((FindEEPROM() != FindTuner()) &&
        (FindEEPROM() != -1) &&
        (FindTuner() != -1))
    {
        ErrorDelta = ErrorCount;
        TaskcSem_p=STOS_SemaphoreCreatePriority(NULL, 0);
        TaskdSem_p=STOS_SemaphoreCreatePriority(NULL, 0);
        SpawnDiffBusTest();

        /* Semaphores to hold main task until test tasks finish */
        STOS_SemaphoreWait(TaskcSem_p);
        STOS_SemaphoreWait(TaskdSem_p);
        myprintf("Concurrency test stage 2 completed\n");
        PrintDuration (Start1, Finish1);
        PrintDuration (Start2, Finish2);
        if( (ErrorCount - ErrorDelta) == 0 )
        {
            myprintf("No errors, stage 2 passed\n");
        }
        else
        {
            myprintf("ERROR: %u errors occurred during stage 2 - STAGE 2 FAILED\n",
                         ErrorCount - ErrorDelta );
        }
        QueryTrace();
        STOS_TaskDelete(taskThree_p, STACK_PARTITION_1,
                        taskStackThree_p, STACK_PARTITION_1);
        STOS_TaskDelete(taskFour_p, STACK_PARTITION_1,
                        taskStackFour_p, STACK_PARTITION_1);
    }
    else
    {
        myprintf("Only found devices on one bus, cannot run this test\n");
    }
#endif
    /*----------------------------------------------------------------------*/
    /* Start the STI2C_Lock/Unlock function tests */
    if(DoEEPROMTests)
    {
        myprintf("\nStarting test of locking functions...\n");
        DoLockFuncTest();
    }

#endif /* LOCK_TEST */

    /*----------------------------------------------------------------------*/
    /* Close and re-open handles to set time-outs to be the same */
    if(DoEEPROMTests)
    {
        myprintf("\nClosing HandleEEPROM...\n");
        CheckCodeOk (STI2C_Close( HandleEEPROM ) );
        myprintf("Closing HandleEEPROM2...\n");
        CheckCodeOk (STI2C_Close( HandleEEPROM2 ) );
        myprintf("Opening HandleEEPROM...\n");
        CheckCodeOk (STI2C_Open(I2CDevName[FindEEPROM()], &I2cOpenParams, &HandleEEPROM));
        myprintf("Opening HandleEEPROM2...\n");
        CheckCodeOk (STI2C_Open( I2CDevName[FindEEPROM()], &I2cOpenParams, &HandleEEPROM2));
    }
    /*----------------------------------------------------------------------*/
    /* Use repeated start variant of read to lock bus to a particular handle */

    /* These tests use the EEPROM handle (at present) */
    if (DoEEPROMTests == TRUE)
    {
        myprintf("\nStarting test of repeated start functions...\n");
    TaskaSem_p=STOS_SemaphoreCreatePriority(NULL, 0);
    TaskbSem_p=STOS_SemaphoreCreatePriority(NULL, 0);
        BusyCount = SuccessCount = 0;
        ErrorDelta = ErrorCount;
        SpawnRepeatStartTest();

        /* Semaphores to hold main task until test tasks finish */
        STOS_SemaphoreWait(TaskaSem_p );
        STOS_SemaphoreWait(TaskbSem_p );
        myprintf("\nRepeated start tests completed\n");
        PrintDuration (Start1, Finish1);
        if( (ErrorCount - ErrorDelta) > 0 )
        {
            myprintf( "%4u errors occurred - TEST FAILED\n",
                          ErrorCount - ErrorDelta );
        }
        myprintf("%4u occurrences of 'device busy'\n", BusyCount);
        QueryTrace();

        /* All of the 2nd task's operations should result in busy. */
        if (SuccessCount > 0)
        {
            myprintf("ERROR: RepeatedStart task was interrupted.\n");
            ErrorCount++;
        }
        if( (ErrorCount - ErrorDelta) == 0 )
        {
            myprintf("No errors, RepeatedStart test passed\n");
        }
        else
        {
            myprintf( "ERROR: %u errors occurred during RepeatedStart test\n",
                          ErrorCount - ErrorDelta );
        }
    }

    /* XXX */
    ReadEEPROM (0x140,  ReadBuffer, sizeof(WriteBuffer), &ActLen);

    /* Note a STOP condition has not yet been generated,
       this will be done when the handle is closed */

   /*----------------------------------------------------------------------*/
    /* Close and term remaining structures trying first unforced then forced
       termination                                                         */

    myprintf("\nForce termination test\n");

    for (busloop = 0; busloop < TEST_NUM_SSC; busloop++)
    {
        /* This will only work if we've actually opened handles on it... */
        if ((FindEEPROM() == busloop) || (FindTuner() == busloop))
        {
            myprintf("Bus %i (%s) - unforced termination (handles open)..\n",
                        busloop, I2CDevName[busloop]);
            I2cTermParams[busloop].ForceTerminate = FALSE;
            CheckCode (STI2C_Term(I2CDevName[busloop], &I2cTermParams[busloop] ),
                        ST_ERROR_OPEN_HANDLE);
        }

        myprintf("Bus %i (%s) - forced termination...\n",
                    busloop, I2CDevName[busloop]);
        I2cTermParams[busloop].ForceTerminate = TRUE;
        CheckCodeOk (STI2C_Term(I2CDevName[busloop], &I2cTermParams[busloop]));
    }

    myprintf("\n");

#ifdef LEAK_TEST
    /*----------------------------------------------------------------------*/
    /* Test for memory leak by repeatedly performing init, open, close, term
       cycles                                                               */
    STOS_TaskDelay(ST_GetClocksPerSecond());
    {
        partition_status_t          Statusp;
        partition_status_flags_t    flags = 0;
        U32                         FreeMemoryBefore = 0;
        U32                         FreeMemoryAfter = 0;
        ST_ErrorCode_t              RetCode;
        BOOL                        TestPassed = TRUE;
        U32                         division = (MEM_TEST_CYCLES / 100);

        myprintf("Starting memory leak test with %d cycles:\n",
                        MEM_TEST_CYCLES);
        i=0;
        do
        {

            if( partition_status(TEST_PARTITION_1,&Statusp,flags)!=0)
            {
                myprintf("Test Partition Not Valid\n");
                break;
            }
            FreeMemoryBefore = Statusp.partition_status_free;

            RetCode = STI2C_Init( I2CDevName[SSC_DEVICE_0], &I2cInitParams[SSC_DEVICE_0]);

            if(ST_NO_ERROR == RetCode)
            {
                RetCode = STI2C_Open( I2CDevName[SSC_DEVICE_0], &I2cOpenParams, &Handle3);
            }
            else
            {
                break;
            }
            if(ST_NO_ERROR == RetCode)
            {
                RetCode = STI2C_Close( Handle3 );
                if(ST_NO_ERROR == RetCode)
                {
                    RetCode = STI2C_Term( I2CDevName[SSC_DEVICE_0], &I2cTermParams[SSC_DEVICE_0]);
                    if(ST_NO_ERROR != RetCode)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
                i++;
                if ((i % division) == 0)
                {
                    myprintf(".");
                }

                if ( 0 != partition_status(TEST_PARTITION_1,&Statusp,flags))
                {
                    myprintf("Test Partition Not Valid\n");
                    break;
                }

                FreeMemoryAfter = Statusp.partition_status_free;
                /*check free mem in partition*/
                if(FreeMemoryAfter != FreeMemoryBefore)
                {
                    myprintf("%d of %d cycles completed\n", i, MEM_TEST_CYCLES );
                    TestPassed = FALSE;
                    ErrorCount++;
                    break;
                }

            }
        }
        while((RetCode == ST_NO_ERROR) && (i < MEM_TEST_CYCLES));

        if(RetCode == ST_NO_ERROR)
        {
            if(TestPassed == TRUE)
            {
                myprintf("%d of %d cycles completed.\n", i, MEM_TEST_CYCLES );
                myprintf("Leak Test Passed.\n" );
            }
            else
            {
                myprintf("ERROR! Leak Test Failed\n");
            }
        }
        else
        {
            myprintf("ERROR! Leak Test Failed with %u error code\n", RetCode );
        }
   }
#endif /* LEAK_TEST */



#ifdef SPEED_TEST
    /*----------------------------------------------------------------------*/
    /* Test speed by timing the number of low clock ticks to write and then
       read a large buffer                                                  */
    if (FindEEPROM() >= 0)
    {
        ST_ErrorCode_t code;
        clock_t     Start, Stop, Diff;
        U8          LongWriteBuffer[SPEED_BUF_LEN];
        U32         rate;
        U8          index =  FindEEPROM();
        STI2C_Handle_t Handle;

        myprintf("Starting speed test using %d byte buffer\n", SPEED_BUF_LEN);
        LongWriteBuffer[0] = LongWriteBuffer[1] = 0;   /* set address to 0 */

        myprintf("Init                :");
#ifdef ST_7200
        if (CheckCodeOk (STI2C_Init(I2CDevName[index], &I2cInitParams[SSC_DEVICE_4] )))
#elif defined(ST_5188)
        if (CheckCodeOk (STI2C_Init(I2CDevName[index], &I2cInitParams[SSC_DEVICE_1] )))
#else
        if (CheckCodeOk (STI2C_Init(I2CDevName[index], &I2cInitParams[SSC_DEVICE_0] )))
#endif
        {
            myprintf("Open                :");
            if (CheckCodeOk (STI2C_Open(I2CDevName[index], &I2cOpenParams, &Handle )))
            {
                myprintf("Write speed test    :");
                Start = STOS_time_now();
                CheckCodeOk (STI2C_Write( Handle, LongWriteBuffer,
                                          sizeof(LongWriteBuffer),
                                          0, &ActLen ));
                Stop = STOS_time_now();
                myprintf("Write completed ");
                Diff = STOS_time_minus( Stop, Start );
                /*
                 * bits per sec = 8*(no of bytes) * TicksPerSec / (ticks taken)
                 * Divide top by 10 initially & multiply again at the end to
                 * avoid overflow.
                 */

                rate = (SPEED_BUF_LEN * (ST_GetClocksPerSecond()/1000)) /
                       (Diff/8);
                rate *= 1000;
                myprintf("in %d low clock ticks - %u bits/sec\n",
                          (int)Diff, rate);

                myprintf("Read speed test     :");
                Start = STOS_time_now();
                CheckCodeOk (STI2C_Read( Handle, LongWriteBuffer,
                                         sizeof(LongWriteBuffer),
                                         0, &ActLen ));
                Stop = STOS_time_now();
                myprintf("Read completed ");
                Diff = STOS_time_minus( Stop, Start );

                rate = (SPEED_BUF_LEN * (ST_GetClocksPerSecond()/1000)) /
                       (Diff/8);
                rate *= 1000;
                myprintf("in %d low clock ticks - %u bits/sec\n\n",
                          (int)Diff, rate);
                code = STI2C_Close(Handle);
            }
            code = STI2C_Term(I2CDevName[index], &I2cTermParams[index]);
        }
    }
    else
    {
        myprintf("No EEPROM present for speed tests.\n");
    }
#endif /* SPEED_TEST */

    if (ErrorCount==0)
    {
        myprintf("Tests completed - PASSED\n");
    }
    else
    {
        myprintf("Tests completed - FAILED, %d errors encountered\n",
                  ErrorCount);
    }
    exit(0);
}

/*  Currently, return bus of first EEPROM found (searching bus, then devices).
    Returns -1 if none found. */
static S8 FindEEPROM(void)
{
    U8 busloop, deviceloop;

    for (busloop = 0; busloop < TEST_NUM_SSC; busloop++)
    {
        for (deviceloop = 0; deviceloop < NumberPresent[busloop]; deviceloop++)
        {
            if (Devices[DevicesPresent[busloop][deviceloop]].DeviceType == EEPROM)
            {
                return busloop;
            }
        }
    }

    return -1;
}

/*  Currently, return bus of first tuner found (searching bus, then devices).
    Returns -1 if not found. */
static S8 FindTuner(void)
{
    U8 busloop, deviceloop;

    for (busloop = 0; busloop < TEST_NUM_SSC; busloop++)
    {
        for (deviceloop = 0; deviceloop < NumberPresent[busloop]; deviceloop++)
        {
            if (Devices[DevicesPresent[busloop][deviceloop]].DeviceType == TUNER)
            {
                return busloop;
            }
        }
    }

    return -1;
}

/*       Name: BusProbe
 *    Purpose: Attempt to see which devices exist on which buses
 *    Returns: Nothing
 * Parameters:
 *    Changes: DevicesPresent, NumberPresent (both global)
 */

static void BusProbe(STI2C_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t          error = ST_NO_ERROR;
    STI2C_OpenParams_t      OpenParams;
    STI2C_Handle_t          Handle;
    U32                     busloop, deviceloop;
    U8                      localBuff[2];
    U32                     actLen;
    U8                      WriteTunerData1      = 0x18;
    U8                      WriteTunerData2      = 0x34;
    static ST_DeviceName_t  DeviceName[TEST_NUM_SSC][11];

    /* Set up common parameters */
    myprintf("Clock speed: %i\n", ClockSpeed);
    myprintf("Clocks per second: %i\n", ClocksPerSec);

    memset(NumberPresent, 0, sizeof(NumberPresent[0]) * TEST_NUM_SSC);

    for (busloop = 0; busloop < TEST_NUM_SSC; busloop++)
    {
            myprintf(" \n\n** Probing for all the devices on SSC - %d [ 0x%x ]\n", busloop,InitParams_p[busloop].BaseAddress);
		deviceloop = 0;
        /* 0 cannot be a valid address. */
        while (Devices[deviceloop].Address != 0)
        {
            myprintf("   Checking for device %s at address %04x... ",
                        Devices[deviceloop].Name,
                        Devices[deviceloop].Address);

            /* Open handle */
            OpenParams.BusAccessTimeOut      = 1;
            OpenParams.AddressType           = STI2C_ADDRESS_7_BITS;
            OpenParams.I2cAddress            = Devices[deviceloop].Address;
#ifdef STI2C_ADAPTIVE_BAUDRATE
            OpenParams.BaudRate              = STI2C_RATE_NORMAL;
#endif

            error = STI2C_Open(I2CDevName[busloop], &OpenParams, &Handle);
            if (error != ST_NO_ERROR)
            {
                myprintf("\n    Couldn't open handle - error %s\n",
                            I2C_ErrorString(error));
                deviceloop++;
                continue;
            }

            /* See if it responds */
            switch (Devices[deviceloop].DeviceType)
            {
                case EEPROM:
                    localBuff[0] = 0x01;
                    localBuff[1] = 0x00;
                    error = STI2C_Write(Handle, localBuff, 2,
                                        WriteTimeout,
                                        &actLen);
                    strcpy(DeviceName[busloop][deviceloop],Devices[deviceloop].Name);
                    break;
                case TUNER:
                    error = WriteTuner2(Handle, TUNER_OFFSET_1, WriteTunerData1);
                    if (error == ST_NO_ERROR)
                    {
                        error = ReadTuner2(Handle, TUNER_OFFSET_1, localBuff);
                        if (error == ST_NO_ERROR)
                        {
                            error = WriteTuner2(Handle, TUNER_OFFSET_2, WriteTunerData2);
                            if (error == ST_NO_ERROR)
                            {
                                error = ReadTuner2(Handle, TUNER_OFFSET_2, localBuff);
                                if (error == ST_NO_ERROR)
                                {
                                    error = ReadTuner2(Handle, TUNER_OFFSET_0, localBuff);
                                    if (error == ST_NO_ERROR)
                                    {
                                        strcpy(DeviceName[busloop][deviceloop], GetTunerChipName(localBuff[0]));
                                    }
                                }
                            }
                        }
                    }
                    break;
                case SCART:
                    localBuff[0] = 0x00;
                    error = STI2C_Write(Handle, localBuff, 1,
                                        WriteTimeout,
                                        &actLen);
                    if (error == ST_NO_ERROR)
                    {
                        error = STI2C_Read(Handle, localBuff, 1,
                                           WriteTimeout,
                                           &actLen);
                    }
                    strcpy(DeviceName[busloop][deviceloop],Devices[deviceloop].Name);
                    break;
                default:
                    myprintf("\n        Don't understand how to test for this device\n");
                    error = ST_ERROR_UNKNOWN_DEVICE;
                    break;
            }

            /* Did we find it? */
            if (error == ST_NO_ERROR)
            {
                U32 index;

                /* Yes, so mark it present in the array for this bus */
                myprintf("found.\n");
                index = NumberPresent[busloop];
                NumberPresent[busloop]++;
                DevicesPresent[busloop][index] = deviceloop;
            }
#ifndef ST_OSLINUX
            else if (error == STI2C_ERROR_ADDRESS_ACK)
#else
            else if (error == ST_ERROR_BAD_PARAMETER)
#endif
            {
                /* Nothing responded to that address */
                myprintf("no response\n");
            }
            else if (error == ST_ERROR_UNKNOWN_DEVICE)
            {
                /* Do nothing; should only come from default in case
                 * statement above.
                 */
            }
            else
            {
                /* Something else went wrong */
                myprintf("\n        Unexpected error occurred - %s\n",
                            I2C_ErrorString(error));
#if defined(STI2C_TRACE)
                QueryTrace();
#endif
                if (error == STI2C_ERROR_LINE_STATE)
                {
                    static int try = 0;
                    if (try == 0)
                    {
                        /* Force stop */
                        error = STI2C_Read(Handle, localBuff, 0,
                                           WriteTimeout,
                                           &actLen);
                        myprintf("Forced stop: %s\n",
                                    I2C_ErrorString(error));
                        try = 1;
                        deviceloop--;
                    }
                    else if (try == 1)
                    {
                        U32 Clocks;
                        error = STI2C_GenerateClocks(Handle, &Clocks);
                        myprintf("Generate clocks: %s\n",
                                    I2C_ErrorString(error));
                        try = 2;
                        deviceloop--;
                    }
                    else
                    {
                        myprintf("Giving up.\n");
                        break;
                    }
                }
            }

            /* Close the handle */
            error = STI2C_Close(Handle);

            deviceloop++;
        }

    }
    myprintf("\n");

    /* Dump the device list */
    for (busloop = 0; busloop < TEST_NUM_SSC; busloop++)
    {
        myprintf("On bus %i, found: ", busloop);
        for (deviceloop = 0; deviceloop < NumberPresent[busloop]; deviceloop++)
        {
            U32 index = DevicesPresent[busloop][deviceloop];
            myprintf("%s ", DeviceName[busloop][index]);
        }
        myprintf("\n");
    }
    myprintf("\n");
}

/*****************************************************************************
 Error routines
*****************************************************************************/

/* Error message look up table */
static I2C_ErrorMessage I2C_ErrorLUT[] =
{
    { STI2C_ERROR_LINE_STATE, "STI2C_ERROR_LINE_STATE" },
    { STI2C_ERROR_STATUS, "STI2C_ERROR_STATUS" },
    { STI2C_ERROR_ADDRESS_ACK, "STI2C_ERROR_ADDRESS_ACK" },
    { STI2C_ERROR_WRITE_ACK, "STI2C_ERROR_WRITE_ACK" },
    { STI2C_ERROR_NO_DATA, "STI2C_ERROR_NO_DATA" },
    { STI2C_ERROR_PIO, "STI2C_ERROR_PIO" },
    { STI2C_ERROR_BUS_IN_USE, "STI2C_ERROR_BUS_IN_USE" },
    { STI2C_ERROR_EVT_REGISTER, "STI2C_ERROR_EVT_REGISTER" },
#ifdef STI2C_DEBUG_BUS_STATE
    { STI2C_ERROR_SDA_LOW_SLAVE, "STI2C_ERROR_SDA_LOW_SLAVE" },
    { STI2C_ERROR_SCL_LOW_SLAVE, "STI2C_ERROR_SCL_LOW_SLAVE" },
    { STI2C_ERROR_SDA_LOW_MASTER, "STI2C_ERROR_SDA_LOW_MASTER" },
    { STI2C_ERROR_SCL_LOW_MASTER, "STI2C_ERROR_SCL_LOW_MASTER" },
    { STI2C_ERROR_FALSE_START, "STI2C_ERROR_FALSE_START" },
#endif

    /* STAPI */
    { ST_NO_ERROR, "ST_NO_ERROR" },
    { ST_ERROR_NO_MEMORY, "ST_ERROR_NO_MEMORY" },
    { ST_ERROR_INTERRUPT_INSTALL, "ST_ERROR_INTERRUPT_INSTALL" },
    { ST_ERROR_ALREADY_INITIALIZED, "ST_ERROR_DEVICE_INITIALIZED" },
    { ST_ERROR_UNKNOWN_DEVICE, "ST_ERROR_UNKNOWN_DEVICE" },
    { ST_ERROR_BAD_PARAMETER, "ST_ERROR_BAD_PARAMETER" },
    { ST_ERROR_OPEN_HANDLE, "ST_ERROR_OPEN_HANDLE" },
    { ST_ERROR_NO_FREE_HANDLES, "ST_ERROR_NO_FREE_HANDLES" },
    { ST_ERROR_INVALID_HANDLE, "ST_ERROR_INVALID_HANDLE" },
    { ST_ERROR_INTERRUPT_UNINSTALL, "ST_ERROR_INTERRUPT_UNINSTALL" },
    { ST_ERROR_TIMEOUT, "ST_ERROR_TIMEOUT" },
    { ST_ERROR_FEATURE_NOT_SUPPORTED, "ST_ERROR_FEATURE_NOT_SUPPORTED" },
    { ST_ERROR_DEVICE_BUSY, "ST_ERROR_DEVICE_BUSY" },
    { ST_ERROR_UNKNOWN, "ST_ERROR_UNKNOWN" } /* Terminator */
};

/* Return the error from the lookup table */
char *I2C_ErrorString(ST_ErrorCode_t Error)
{
    I2C_ErrorMessage *mp = NULL;

    mp = I2C_ErrorLUT;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return mp->ErrorMsg;
}

#if !defined(ST_OSLINUX)
void DumpI2cRegs(void *addr)
{
#if (!defined(PROCESSOR_C1)&& defined(ST_OS20))
#pragma ST_device(i2cregs)
#endif
    volatile U32 *i2cregs;

    i2cregs = (volatile U32 *)addr;

    printf("\n--------------------------------\n");
    printf("SSCnBRG    = 0x%04x\n", i2cregs[0]);
    printf("SSCnTX     = 0x%04x\n", i2cregs[1]);
    printf("SSCnRX     = 0x%04x\n", i2cregs[2]);
    printf("SSCnCON    = 0x%04x\n", i2cregs[3]);
    printf("SSCnINTEN  = 0x%04x\n", i2cregs[4]);
    printf("SSCnSTAT   = 0x%04x\n", i2cregs[5]);
    printf("SSCnI2C    = 0x%04x\n", i2cregs[6]);
    printf("SSCnSLAD   = 0x%04x\n", i2cregs[7]);
    printf("SSCnCLR    = 0x%04x\n", i2cregs[32]);
    printf("SSCnGWDTH  = 0x%04x\n", i2cregs[64]);
    printf("SSCnPRE    = 0x%04x\n", i2cregs[65]);
    printf("--------------------------------\n");
}
#endif

/* Helper functions for identifying Tuner Chip */

static char * GetTunerChipName(U8 ChipId)
{
    ChipId&=0xF0;

    switch(ChipId)
    {
        case 0xA0:
            return "Tuner(299)";
        case 0xB0:
            return "Tuner(399)";
        case 0x20:
            return "Tuner(360)";
        default:
            return "Unknown";
    }
}


/*  Currently, return I2C address of first tuner found (searching bus, then devices).
    Returns -1 if not found. */

static S8 FindTunerAddress(U8 *Address)
{
    U8 busloop, deviceloop;

    for (busloop = 0; busloop < TEST_NUM_SSC; busloop++)
    {
        for (deviceloop = 0; deviceloop < NumberPresent[busloop]; deviceloop++)
        {
            if (Devices[DevicesPresent[busloop][deviceloop]].DeviceType == TUNER)
            {
                /*assuming all the i2caddresses are 7 bit*/
                *Address = (U8) Devices[DevicesPresent[busloop][deviceloop]].Address;
                return busloop;
            }
        }
    }

    return -1;
}


/* Following tuner related Function only defined for Optional tuner test*/

#ifdef TUNER_TEST_EXTENDED

/*---------------------------------------------------------------------------*/
/* Functions for R/W to Tuner (DEMOD)                                        */
/* For Tuner Write  :   1 byte address + 1 byte data                         */
/* For Tuner Read   :   Write 1 byte address; Read 1 byte data               */
/* For Thread Protection STI2C_Lock and STI2C_Unlock are used for Reading    */
/* Although, only 299 support Write + NoStop variation for Read operation    */
/*---------------------------------------------------------------------------*/

static ST_ErrorCode_t WriteTunerNoPrint( STI2C_Handle_t Handle,
                                            U32 Address, U8 data)
{
    ST_ErrorCode_t   code;
    U8               localBuff[2];
    U32              actLen;
    clock_t          Start;

    localBuff[0] = (U8) Address;
    localBuff[1] = data;

    if (DoTunerTests == FALSE)
    {
        return ST_NO_ERROR;
    }
    Start = STOS_time_now();

    code = STI2C_Lock(Handle,TRUE);
    if( code == ST_NO_ERROR)
    {
        code = STI2C_Write( Handle, localBuff, 2, WriteTimeout, &actLen );
    }
    else
    {
        CheckCode( code, ST_NO_ERROR);
    }
    PrintDuration (Start, STOS_time_now());
    if( code != ST_NO_ERROR )
    {
        STOS_SemaphoreWait(PrintSem_p);
        CheckCode( code, ST_NO_ERROR );
        STOS_SemaphoreSignal(PrintSem_p);
        code = STI2C_Unlock(Handle);
        if (code != ST_NO_ERROR)
        {
            CheckCode( code, ST_NO_ERROR);
        }
    }
    else
    {
        code = STI2C_Unlock(Handle);
        if (code != ST_NO_ERROR)
        {
            CheckCode( code, ST_NO_ERROR);
        }
    }
    QueryTrace();
    return code;
}

static ST_ErrorCode_t ReadTunerNoPrint(STI2C_Handle_t Handle,
                                        U32 Address, U8 *pData)
{
    ST_ErrorCode_t   code;
    U32              actLen;
    clock_t          Start;

    if (DoTunerTests == FALSE)
    {
        return ST_NO_ERROR;
    }
    Start = STOS_time_now();

    code = STI2C_Lock(Handle,TRUE);
    if( code == ST_NO_ERROR)
    {
        code = STI2C_Write( Handle, (U8 *) &Address, 1, WriteTimeout,
                        &actLen );
    }
    else
    {
        CheckCode( code, ST_NO_ERROR);
    }

    PrintDuration (Start, STOS_time_now());
    if( code != ST_NO_ERROR )
    {
        STOS_SemaphoreWait(PrintSem_p);
        CheckCode( code, ST_NO_ERROR);
        STOS_SemaphoreSignal(PrintSem_p);

        code = STI2C_Unlock(Handle);
        if (code != ST_NO_ERROR)
        {
            CheckCode( code, ST_NO_ERROR);
        }
    }
    else
    {
        code = STI2C_Read (Handle, pData, 1, ReadTimeout, &actLen);
        if (code != ST_NO_ERROR)
        {
            STOS_SemaphoreWait(PrintSem_p);
            CheckCode( code, ST_NO_ERROR );
            STOS_SemaphoreSignal(PrintSem_p);
        }

        code = STI2C_Unlock(Handle);
        if (code != ST_NO_ERROR)
        {
            CheckCode( code, ST_NO_ERROR );
        }
    }
    QueryTrace();
    return code;
}

static U32 GetTunerChipType(U8 ChipId)
{
    U32 Chip = 0;

    ChipId &= 0xF0; /* Only MSNibble is relevent */

    switch(ChipId)
    {
        case 0xA0:
            Chip=TUNER_299;
            break;
        case 0xB0:
            Chip=TUNER_399;
            break;
        case 0x20:
            Chip=TUNER_360;
            break;
        default:
            Chip=NOT_SUPPORTED;
            break;
    }
    return Chip;
}

/* Sets tuner to Normal Mode, see Specific Tuner Data sheet for Information */

static ST_ErrorCode_t SetTunerMode( STI2C_Handle_t Handle, U32 Mode)
{
    U8              *pData;
    ST_ErrorCode_t  code = ST_NO_ERROR;
    switch(Mode)
    {
         case TUNER_MODE_NORMAL:
            code = ReadTunerNoPrint( Handle, StandByReg[TunerChip], pData);
            *pData &= 0x7F; /*turn MSB bit Low refer specific tuner data sheet*/
            if(code == ST_NO_ERROR)
            {
                WriteTunerNoPrint(Handle, StandByReg[TunerChip], *pData);
            }
            break;

        default:
            myprintf("Only Normal Mode Required!\n");
            break;
    }

    return code;
}


static void WriteAllTunerReg(STI2C_Handle_t Handle,U8 basereg,
                               U8 Reg2Wr,U8 regData_p[][2])
{
    U8 i=0;
    ST_ErrorCode_t   code;
    U8 Data;

    for(i=0;i<Reg2Wr;i++)
    {
        Data = regData_p[i][1];
        code = WriteTunerNoPrint(Handle, (basereg+i), Data);
        if( code != ST_NO_ERROR )
        {
            OtherFailures++;
        }
     }
}

static void ReadAllTunerReg(STI2C_Handle_t Handle,U8 BaseReg,
                                U8 NumberOfReg2Rd, U8 readBuffer[])
{
    U8 i;
    ST_ErrorCode_t   code;
    U8 Address;
    U8 Data;

    for(i=0;i<NumberOfReg2Rd;i++)
    {
        Address = BaseReg + i;
        code = ReadTunerNoPrint(Handle, (U8)Address, &Data);
        if( code == ST_NO_ERROR )
        {
            readBuffer[i]=Data;
        }
        else
        {
            OtherFailures++;
        }
    }
}

static U32 CompareAllReg(U8 NumberOfReg,U8 readBuffer_p[],U8 regData_p[][2])
{
    U32 i = 0;
    U32 MisMatches = 0;

    for(i=0;i<NumberOfReg;i++)
    {
        if(0 == regData_p[i][0])
        {
            if(readBuffer_p[i] != regData_p[i][1])
            {
                myprintf("\nMISMATCH! in %s at Loc=%x Written=%x Read=%x\n",
                                task_name(NULL), i, regData_p[i][1], readBuffer_p[i]);
                QueryTrace();
                TotalTunerMismatch++;
                MisMatches++;
            }

        }
    }
    return MisMatches;
}

/* This Functions access whole tuner in case of single thread and  */
/* and half of register space in case of multiple threads          */

static void TestTunerPrimary(void * voidptr)
{
   TunerRegister_t *TunerPrimRegs;
   U32             LoopMisMatch = 0;
   U32             Times2RdWr = 0;

   assert(NULL != voidptr);
   TunerPrimRegs = (TunerRegister_t *)voidptr;

   while( Times2RdWr++ < TUNER_TEST_CYCLES)
   {
       WriteAllTunerReg(HandleTunerPrimary, TunerPrimRegs->BaseReg,
                              TunerPrimRegs->NumberOfRegisters,
                              TunerPrimRegs->RegData );

       memset(ReadBuffPrim, 0x00, sizeof(ReadBuffPrim));
       ReadAllTunerReg(HandleTunerPrimary, TunerPrimRegs->BaseReg,
                              TunerPrimRegs->NumberOfRegisters,
                              ReadBuffPrim);
       /* Semaphore used for printing infromation about mismatches */
       STOS_SemaphoreWait(PrintSem_p);
       LoopMisMatch=CompareAllReg(TunerPrimRegs->NumberOfRegisters,
                                 ReadBuffPrim, TunerPrimRegs->RegData);
       STOS_SemaphoreSignal(PrintSem_p);
   }

}


/* This Functions access half of register space in case of multiple threads */

static void TestTunerSecondary(void * voidptr)
{

    TunerRegister_t *TunerSecRegs;
    U32             Times2RdWr = 0;
    U32             LoopMisMatch = 0;

    assert(NULL != voidptr);
    TunerSecRegs = (TunerRegister_t *)voidptr;

    while(Times2RdWr++ < TUNER_TEST_CYCLES)
    {

        WriteAllTunerReg(HandleTunerSecondary, TunerSecRegs->BaseReg,
                            TunerSecRegs->NumberOfRegisters,
                            TunerSecRegs->RegData );
        memset(ReadBuffPrim, 0x00, sizeof(ReadBuffPrim));
        ReadAllTunerReg(HandleTunerSecondary, TunerSecRegs->BaseReg,
                            TunerSecRegs->NumberOfRegisters, ReadBuffSec);

        /* used for printing infromation about mismatches */

        STOS_SemaphoreWait(CompareAllSem_p);
        LoopMisMatch = CompareAllReg(TunerSecRegs->NumberOfRegisters,
                             ReadBuffSec, TunerSecRegs->RegData);
        STOS_SemaphoreSignal(CompareAllSem_p);
    }
}

/* Access tuner register space with multiple threads */

static ST_ErrorCode_t StartTunerTestSingle(void )
{

    ST_ErrorCode_t          code = ST_NO_ERROR;
    U32                     RegistersInTuner = 0;
    U32                     Times2RdWr       = 0;
    U8                      ReadBuffPrim1[MAX_TUNER_REG];
    TunerRegister_t         PrimTuner;
    U32                     MisMatch = 0;

    TotalTunerMismatch = 0;
    OtherFailures = 0;

    switch(TunerChip)
    {
        case TUNER_299:

            /* Find the total number of registers to Rd and Write */
            /* Access whole register space in case of single thread */
            /* A sepearate copy of register data is created and passed
            to thread writing to DEMOD, May it is not the best way to
            do it, As this data is read only and so also thread safe
            could also be passed as a pointer */

            RegistersInTuner = sizeof(RegData299)/2;
            PrimTuner.NumberOfRegisters = RegistersInTuner;
            PrimTuner.BaseReg = 0;
            memcpy(PrimTuner.RegData, RegData299 + PrimTuner.BaseReg,
                      PrimTuner.NumberOfRegisters*2);
            break;

        case TUNER_399:
            RegistersInTuner = sizeof(RegData399)/2;
            PrimTuner.NumberOfRegisters = RegistersInTuner;
            PrimTuner.BaseReg = 0;
            memcpy(PrimTuner.RegData, RegData399 + PrimTuner.BaseReg,
                                        PrimTuner.NumberOfRegisters*2);
            break;

        case TUNER_360:
            /*find the total number of registers to Rd and Write*/
            RegistersInTuner = sizeof(RegData360)/2;
            PrimTuner.NumberOfRegisters = RegistersInTuner;
            PrimTuner.BaseReg = 0;
            memcpy(PrimTuner.RegData, RegData360 + PrimTuner.BaseReg,
                                        PrimTuner.NumberOfRegisters*2);
            break;

        default:
            myprintf("\nUNKNOWN TUNER TYPE!\n");
            return code;
            break;
    }

    myprintf("\nStarting Single Thread test...\n");

    while(Times2RdWr++ < TUNER_TEST_CYCLES)
    {
        WriteAllTunerReg(HandleTunerPrimary, PrimTuner.BaseReg,
                           PrimTuner.NumberOfRegisters, PrimTuner.RegData );
        memset(ReadBuffPrim, 0x00, sizeof(ReadBuffPrim1));
        ReadAllTunerReg(HandleTunerPrimary, PrimTuner.BaseReg,
                           PrimTuner.NumberOfRegisters, ReadBuffPrim1);
        TotalTunerMismatch = CompareAllReg(PrimTuner.NumberOfRegisters,
                                            ReadBuffPrim1, PrimTuner.RegData);
        myprintf("\nTotal MisMatch in Single Thread = %u", TotalTunerMismatch);
    }

    if(TotalTunerMismatch == 0)
    {
        myprintf("\nSingle Thread TEST PASSED.");
    }
    else if (TotalTunerMismatch > 0)
    {
        myprintf("\nTEST FAILED! with = %u MisMatch\n", MisMatch);
    }

    TotalTunerMismatch = 0;

    return code;
}

/* Access tuner register space with multiple threads */

static ST_ErrorCode_t StartTunerTestMultiple(void )
{

    ST_ErrorCode_t   code = ST_NO_ERROR;
    S32              RetVal;
    U32              RegistersInTuner = 0;
    task_t           *tasks[2];
    CompareAllSem_p = STOS_SemaphoreCreateFifo(NULL, 1); /* To protect output on screen in case of multiple threads */
    PrintSem_p      = STOS_SemaphoreCreateFifo(NULL, 1); /* To protect output on screen in case of multiple threads */

    TotalTunerMismatch = 0;
    OtherFailures = 0;

    switch(TunerChip)
    {
        case TUNER_299:
            RegistersInTuner = sizeof(RegData299)/2;
            /* Access Half of register space in case of multiple thread */
            PrimTuner.NumberOfRegisters = RegistersInTuner/2;
            PrimTuner.BaseReg = 0;
            memcpy(PrimTuner.RegData, RegData299 + PrimTuner.BaseReg,
                                      PrimTuner.NumberOfRegisters*2);
            SecTuner.NumberOfRegisters = RegistersInTuner -
                                     PrimTuner.NumberOfRegisters;
            SecTuner.BaseReg = PrimTuner.BaseReg +
                           PrimTuner.NumberOfRegisters ;
            memcpy(SecTuner.RegData, RegData299 + SecTuner.BaseReg,
                                     SecTuner.NumberOfRegisters*2);
           break;

        case TUNER_399:
            RegistersInTuner = sizeof(RegData399)/2;
            PrimTuner.NumberOfRegisters = RegistersInTuner/2;
            PrimTuner.BaseReg = 0;
            memcpy(PrimTuner.RegData, RegData399 + PrimTuner.BaseReg,
                     PrimTuner.NumberOfRegisters*2);
            SecTuner.NumberOfRegisters = RegistersInTuner -
                     PrimTuner.NumberOfRegisters;
            SecTuner.BaseReg = PrimTuner.BaseReg + PrimTuner.NumberOfRegisters;
            memcpy(SecTuner.RegData, RegData399 + SecTuner.BaseReg,
                                     SecTuner.NumberOfRegisters*2);
            break;

        case TUNER_360:
            /*Find the total number of registers to Read and Write*/
            RegistersInTuner = sizeof(RegData360)/2;
            PrimTuner.NumberOfRegisters = RegistersInTuner/2;
            PrimTuner.BaseReg = 0;
            memcpy(PrimTuner.RegData, RegData360 + PrimTuner.BaseReg,
                                      PrimTuner.NumberOfRegisters*2);
            SecTuner.NumberOfRegisters = RegistersInTuner - PrimTuner.NumberOfRegisters;
            SecTuner.BaseReg = PrimTuner.BaseReg + PrimTuner.NumberOfRegisters;
            memcpy(SecTuner.RegData, RegData360 + SecTuner.BaseReg,
                                     SecTuner.NumberOfRegisters*2);
            break;

        default:
            myprintf("\nUNKNOWN TUNER TYPE!\n");
            return code;
            break;
    }
    /* Create Two Threads to write in two different Partitions
        One is called Primary, and other secondary. */

    /* These threads needs to be passed Reg data for the chip */

    myprintf("\n\nStarting Multiple Thread test...\n");

    if(STOS_TaskCreate(TestTunerPrimary,(void *) &PrimTuner, STACK_PARTITION_1, STACK_SIZE, &taskStackTunerPrim_p, STACK_PARTITION_1,
                      &taskTunerPrim_p, &tdescTunerPrim,MAX_USER_PRIORITY, "PrimaryThread",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create tuner Primary task\n");
    }

    if(STOS_TaskCreate(TestTunerSecondary,(void *) &SecTuner, STACK_PARTITION_1, STACK_SIZE, &taskStackTunerSec_p, STACK_PARTITION_1,
                       &taskTunerSec_p, &tdescTunerSec, MAX_USER_PRIORITY, "SecondaryThread",(task_flags_t)0)!=ST_NO_ERROR)
    {
        myprintf("\nError: task_create. Unable to create tuner Secondary task\n");
    }
    myprintf("Done.\n\n");


    tasks[0] = taskTunerPrim_p;
    tasks[1] = taskTunerSec_p;

    STOS_TaskWait(&tasks[0], TIMEOUT_INFINITY);
    STOS_TaskDelete(taskTunerPrim_p, STACK_PARTITION_1,
                taskStackTunerPrim_p, STACK_PARTITION_1);
    STOS_TaskWait(&tasks[1], TIMEOUT_INFINITY);
    STOS_TaskDelete(taskTunerSec_p, STACK_PARTITION_1,
                taskStackTunerSec_p,STACK_PARTITION_1);

    return code;
}


static void DoTheTunerTest(void)
{
    U8                   Data;
    U8                   DevAddress;
    ST_ErrorCode_t       RetCode=ST_NO_ERROR;
    ST_DeviceName_t      DevNameTuner;

    /*Declare Two Handles and open them.*/
    STI2C_OpenParams_t   Tuner1_I2cOpenParams;
    STI2C_OpenParams_t   Tuner2_I2cOpenParams;

    STI2C_InitParams_t   I2cInitParamsTuner;
    STI2C_TermParams_t   I2cTermParamsTuner;

    /* Block 1 */

#if defined (ST_7710)
    strcpy( DevNameTuner, "I2C-0" );

    I2cInitParamsTuner.BaseAddress          = (U32*)SSC_0_BASE_ADDRESS;
    I2cInitParamsTuner.InterruptNumber      = SSC_0_INTERRUPT;
    I2cInitParamsTuner.InterruptLevel       = SSC_0_INTERRUPT_LEVEL;
#ifdef BAUD_RATE_FAST
    I2cInitParamsTuner.BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParamsTuner.BaudRate             = STI2C_RATE_NORMAL;
#endif
    I2cInitParamsTuner.MasterSlave          = STI2C_MASTER;
    I2cInitParamsTuner.ClockFrequency       = ClockSpeed;
    I2cInitParamsTuner.GlitchWidth          = 0;
    I2cInitParamsTuner.PIOforSDA.BitMask    = PIO_FOR_SSC0_SDA;
    I2cInitParamsTuner.PIOforSCL.BitMask    = PIO_FOR_SSC0_SCL;
    I2cInitParamsTuner.MaxHandles           = 4;
    I2cInitParamsTuner.DriverPartition      = TEST_PARTITION_1;
#ifdef TEST_FIFO
    I2cInitParamsTuner.FifoEnabled          = TRUE;
#else
    I2cInitParamsTuner.FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParamsTuner.PIOforSDA.PortName, PIODevName[SSC0_SDA_DEV] );
    strcpy( I2cInitParamsTuner.PIOforSCL.PortName, PIODevName[SSC0_SCL_DEV] );

#else
    strcpy( DevNameTuner, "I2C-1" );
    I2cInitParamsTuner.BaseAddress          = (U32*)SSC_1_BASE_ADDRESS;
    I2cInitParamsTuner.InterruptNumber      = SSC_1_INTERRUPT;
    I2cInitParamsTuner.InterruptLevel       = SSC_1_INTERRUPT_LEVEL;
#ifdef BAUD_RATE_FAST
    I2cInitParamsTuner.BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParamsTuner.BaudRate             = STI2C_RATE_NORMAL;
#endif
    I2cInitParamsTuner.MasterSlave          = STI2C_MASTER;
    I2cInitParamsTuner.ClockFrequency       = ClockSpeed;
    I2cInitParamsTuner.GlitchWidth          = 0;
    I2cInitParamsTuner.PIOforSDA.BitMask    = PIO_FOR_SSC1_SDA;
    I2cInitParamsTuner.PIOforSCL.BitMask    = PIO_FOR_SSC1_SCL;
    I2cInitParamsTuner.MaxHandles           = 4;
    I2cInitParamsTuner.DriverPartition      = TEST_PARTITION_1;
#ifdef TEST_FIFO
    I2cInitParamsTuner.FifoEnabled          = TRUE;
#else
    I2cInitParamsTuner.FifoEnabled          = FALSE;
#endif
    strcpy( I2cInitParamsTuner.PIOforSDA.PortName, PIODevName[SSC1_SDA_DEV] );
    strcpy( I2cInitParamsTuner.PIOforSCL.PortName, PIODevName[SSC1_SCL_DEV] );

#endif /*END of 7710*/

    /* Open block Primary */

    Tuner1_I2cOpenParams.BusAccessTimeOut     = 20;
    Tuner1_I2cOpenParams.AddressType          = STI2C_ADDRESS_7_BITS;
    Tuner1_I2cOpenParams.I2cAddress           = 0xD0;    /* default address */
#ifdef STI2C_ADAPTIVE_BAUDRATE
    Tuner1_I2cOpenParams.BaudRate             = STI2C_RATE_NORMAL;
#endif
    /* Open block Secondary */
    Tuner2_I2cOpenParams.BusAccessTimeOut     = 20;
    Tuner2_I2cOpenParams.AddressType          = STI2C_ADDRESS_7_BITS;
    Tuner2_I2cOpenParams.I2cAddress           = 0xD0;    /* default address */
#ifdef STI2C_ADAPTIVE_BAUDRATE
    Tuner2_I2cOpenParams.BaudRate             = STI2C_RATE_NORMAL;
#endif
    myprintf("\nInitializing I2c for tuner test ...\n");
    CheckCode(STI2C_Init(DevNameTuner, &I2cInitParamsTuner), ST_NO_ERROR);

    myprintf("\nExtensive Tuner Test...\n-----------------------\n");
    myprintf("\nTest will write to all registers...\n");

    if (FindTuner() >= 0)
    {
        S8 TunerBus = FindTunerAddress(&DevAddress);
        myprintf("Opening First Handle for Tuner (bus %i)...\n", TunerBus);
        Tuner1_I2cOpenParams.I2cAddress = DevAddress;
        CheckCodeOk (STI2C_Open(DevNameTuner, &Tuner1_I2cOpenParams,
                                            &HandleTunerPrimary));
        myprintf("Opening Second Handle for Tuner (bus %i)...\n", TunerBus);
        Tuner2_I2cOpenParams.I2cAddress = DevAddress;
        CheckCodeOk (STI2C_Open(DevNameTuner, &Tuner2_I2cOpenParams,
                                            &HandleTunerSecondary));

        /* Read it's Device Id register */

        RetCode = ReadTunerNoPrint(HandleTunerPrimary, TUNER_DEVICE_ID_REG, &Data );

        if(RetCode == ST_NO_ERROR)
        {
            TunerChip = (TunerChip_t)GetTunerChipType(Data); /* Used To Print Name*/
            myprintf("Tuner Chip %s detected\n", TunerName[TunerChip]);

            /* Read Tuner Mode(Standby/Normal) */
            /* These Tests can only be performed in Normal Mode */
            myprintf("Setting Tuner Mode to Normal...");
            RetCode = SetTunerMode(HandleTunerPrimary, TUNER_MODE_NORMAL);

            if(RetCode == ST_NO_ERROR)
            {
                /*Proceed Further*/
                StartTunerTestSingle();
                StartTunerTestMultiple();
                if((TotalTunerMismatch == 0) && (OtherFailures == 0))
                {
                    myprintf("\nTuner TEST PASSED.\n");
                }
                else
                {
                    myprintf("\nTuner TEST FAILED with %u bad write/read and %u other Errors\n",
                                TotalTunerMismatch,OtherFailures);
                }
            }
            else
            {
                myprintf("Tuner Test can't be done in Standby Mode...\n");
            }
        }

        myprintf("\nClosing First Handle for Tuner...\n");
        CheckCodeOk (STI2C_Close( HandleTunerPrimary ) );

        myprintf("Closing Second Handle for Tuner...\n");
        CheckCodeOk (STI2C_Close( HandleTunerSecondary ) );
    }
    else
    {
        myprintf("No tuners found, disabling tests\n");
        DoTunerTests = FALSE;
    }

    I2cTermParamsTuner.ForceTerminate       = FALSE;
    myprintf("Terminating I2c for Tuner...\n");
    CheckCodeOk (STI2C_Term( DevNameTuner, &I2cTermParamsTuner ));
}

#endif /* END OF TUNER_TEST_EXTENDED*/

#if defined(ST_OSLINUX) && defined(ST_7200)
void DoTheTests_7200_Linux(void)
{
    /* Declarations ---------------------------------------------------------*/

    /* Init block 1 setup and term */
    ST_DeviceName_t      DevName,DevName1;
    STI2C_InitParams_t   I2cInitParams, I2cInitParams1;
    STI2C_TermParams_t   I2cTermParams;

   /* Open blocks */
    STI2C_OpenParams_t   I2cOpenParams;
    U32              ActLen;
    /* Initialisation ------------------------------------------------------*/
    /* Block 1 */
    strcpy( DevName, "I2C-2" );
    I2cInitParams.BaseAddress          = (U32*)SSC_2_BASE_ADDRESS;
    I2cInitParams.InterruptNumber      = SSC_2_INTERRUPT;
    I2cInitParams.InterruptLevel       = 8;
    I2cInitParams.BaudRate             = STI2C_RATE_NORMAL;
    I2cInitParams.MasterSlave          = STI2C_MASTER;
    I2cInitParams.ClockFrequency       = ClockSpeed;
    I2cInitParams.GlitchWidth          = 0;
    I2cInitParams.PIOforSDA.BitMask    = PIO_FOR_SSC2_SDA;
    I2cInitParams.PIOforSCL.BitMask    = PIO_FOR_SSC2_SCL;
    I2cInitParams.MaxHandles           = 4;
#if defined(ST_OS21)
    I2cInitParams.DriverPartition       = (ST_Partition_t *)system_partition_p;
#else
    I2cInitParams.DriverPartition       = TEST_PARTITION_1;
#endif
#ifdef TEST_FIFO
    I2cInitParams.FifoEnabled           = TRUE;
#else
    I2cInitParams.FifoEnabled           = FALSE;
#endif

    strcpy( I2cInitParams.PIOforSDA.PortName, PIODevName[SSC_DEVICE_2] );
    strcpy( I2cInitParams.PIOforSCL.PortName, PIODevName[SSC_DEVICE_2] );

	/* Block 2 */
    strcpy( DevName1, "I2C-4" );
    I2cInitParams1.BaseAddress          = (U32*)SSC_4_BASE_ADDRESS;
    I2cInitParams1.InterruptNumber      = SSC_4_INTERRUPT;
    I2cInitParams1.InterruptLevel       = 10;
    I2cInitParams1.BaudRate             = STI2C_RATE_NORMAL;
    I2cInitParams1.MasterSlave          = STI2C_MASTER;
    I2cInitParams1.ClockFrequency       = ClockSpeed;
    I2cInitParams1.GlitchWidth          = 0;
    I2cInitParams1.PIOforSDA.BitMask    = PIO_FOR_SSC4_SDA;
    I2cInitParams1.PIOforSCL.BitMask    = PIO_FOR_SSC4_SCL;
    I2cInitParams1.MaxHandles           = 4;
#if defined(ST_OS21)
    I2cInitParams1.DriverPartition       = (ST_Partition_t *)system_partition_p;
#else
    I2cInitParams1.DriverPartition       = TEST_PARTITION_1;
#endif
#ifdef TEST_FIFO
    I2cInitParams1.FifoEnabled           = TRUE;
#else
    I2cInitParams1.FifoEnabled           = FALSE;
#endif

    strcpy( I2cInitParams1.PIOforSDA.PortName, PIODevName[SSC_DEVICE_4] );
    strcpy( I2cInitParams1.PIOforSCL.PortName, PIODevName[SSC_DEVICE_4] );
	
                            /* Open block  */
    I2cOpenParams.BusAccessTimeOut     = 10000;
    I2cOpenParams.AddressType          = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.BaudRate             = STI2C_RATE_NORMAL;

                            /* Term blocks */
    I2cTermParams.ForceTerminate        = FALSE;


    /*-----------------------------------------------------------------------*/
    /* Init and term tests - This section shows that the driver can be       */
    /* initialised & terminated multiple times for two buses without failing */

    myprintf("\n\nInitializing SSC-2\n");       
    CheckCodeOk (STI2C_Init( I2CDevName[2], &I2cInitParams) );

    myprintf("Opening Handle to Tuner at Address 0xd2...\n" );
    I2cOpenParams.I2cAddress = 0xd2;
    CheckCodeOk (STI2C_Open(I2CDevName[2], &I2cOpenParams, &HandleTuner));
    myprintf("\n");

    U8 WriteBuffer[3]= {0x06,0x57,0xa4};
    U8 ReadBuffer[3]={0x00,0x00,0x00};
    
    WriteTuner(0x01, 0x18);
    ReadTuner(0x01, ReadBuffer);
    WriteTuner(0x02, 0x23);
    ReadTuner(0x02, ReadBuffer);
    myprintf("\nClosing Tuner Handle\n" );
    CheckCodeOk(STI2C_Close(HandleTuner));

    myprintf("\n\nInitialization of SSC-4\n");
    CheckCodeOk (STI2C_Init( I2CDevName[4], &I2cInitParams1) );
    myprintf("Opening Handle to EEPROM at address 0xa0...\n" ); 
    I2cOpenParams.I2cAddress       = 0xa0; 
    CheckCodeOk(STI2C_Open(I2CDevName[4],&I2cOpenParams,&HandleEEPROM));
    WriteEEPROM(0x140, WriteBuffer, sizeof(WriteBuffer), &ActLen);
    ReadEEPROM (0x140, ReadBuffer, sizeof(WriteBuffer), &ActLen);
    myprintf("\nClosing EEPROM Handle\n" );
    CheckCodeOk(STI2C_Close(HandleEEPROM));

    /* Term block 2 */
    myprintf("\n\nTerminating bus SSC-2...\n");
    CheckCodeOk (STI2C_Term(  I2CDevName[2], &I2cTermParams) );

    /* Term block 4 */
    myprintf("Terminating bus SSC-4...\n");
    CheckCodeOk (STI2C_Term(  I2CDevName[4], &I2cTermParams) );    

    myprintf("\n");
    if (ErrorCount==0)
    {
        myprintf("Tests completed - PASSED\n");
    }
    else
    {
        myprintf("Tests completed - FAILED, %d errors encountered\n",
                  ErrorCount);
    }
    exit(0);
}
#endif
/* EOF */

