/***********************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : i2cspitest.c

Test Harness for communication of I2C & SPI on same SSC.
Set the flag STI2C_STSPI_SAME_SSC_SUPPORT to run this test.
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

#include "stos.h"
#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"
#include "stpio.h"
#include "sttbx.h"
#include "stboot.h"
#include "stcommon.h"
#include "sti2c.h"
#include "stspi.h"
/*added for 5100 sysytem register configuration*/
#include "stsys.h"
/*#include "cache.h"*/
/* Private types/constants ----------------------------------------------- */
#if defined(ST_5105) || defined(ST_5188)

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

/* FLASH Specific commands */

#define FLASH_READ_STATUS       0x05
#define FLASH_WRITE_STATUS      0x01
#define FLASH_PAGE_PROGRAM      0x02
#define FLASH_DATA_READ         0x03
#define FLASH_FAST_READ         0x0B
#define FLASH_SECTOR_ERASE      0xD8
#define FLASH_BULK_ERASE        0xC7
#define FLASH_WRITE_ENABLE      0x06
#define FLASH_WRITE_DISABLE     0x04
/*****************************************************************************
Types and definitions
*****************************************************************************/

#define CHIP_SELECT_LOW    0x00000000
#define CHIP_SELECT_HIGH   0x00010001

#ifdef ST_OS21
#define STACKSIZE 16*1024
#else
#define STACKSIZE 4*1024
#endif
/* Test configuration information */
#ifdef ST_5105
#define STSPI_DEV_M25P10
#elif defined(ST_5188)
#define STSPI_DEV_M25P80
#endif

#if defined(ST_5105)

#define NUMBER_PIO             4

#define SSC_BASE_ADDRESS       SSC_1_BASE_ADDRESS
#define SSC_INTERRUPT          SSC_1_INTERRUPT
#define SSC_INTERRUPT_LEVEL    SSC_1_INTERRUPT_LEVEL

#define SSC_MTSR_BIT           PIO_BIT_6
#define SSC_MTSR_DEV           PIO_DEVICE_0

#define SSC_MRST_BIT           PIO_BIT_1
#define SSC_MRST_DEV           PIO_DEVICE_0

#define SSC_SCL_BIT            PIO_BIT_2
#define SSC_SCL_DEV            PIO_DEVICE_0

#define SSC_CS_BIT             PIO_BIT_5
#define SSC_CS_DEV             PIO_DEVICE_3

#define INTERCONNECT_BASE                       (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#elif defined(ST_5188)

#define NUMBER_PIO             4

#define SSC_BASE_ADDRESS       SSC_0_BASE_ADDRESS
#define SSC_INTERRUPT          SSC_0_INTERRUPT
#define SSC_INTERRUPT_LEVEL    SSC_0_INTERRUPT_LEVEL

#define SSC_MTSR_BIT           PIO_BIT_6
#define SSC_MTSR_DEV           PIO_DEVICE_0

#define SSC_MRST_BIT           PIO_BIT_7
#define SSC_MRST_DEV           PIO_DEVICE_0

#define SSC_SCL_BIT            PIO_BIT_5
#define SSC_SCL_DEV            PIO_DEVICE_0

#define SSC_CS_BIT             PIO_BIT_4
#define SSC_CS_DEV             PIO_DEVICE_0

#define INTERCONNECT_BASE                       (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)
#endif

#if defined(ST_5105)

#define     SSC_0_PIO_BASE              PIO_3_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_3_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL
#define     SSC_1_PIO_BASE              PIO_3_BASE_ADDRESS
#define     SSC_1_PIO_INTERRUPT         PIO_3_INTERRUPT
#define     SSC_1_PIO_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL

#define PIO_FOR_SSC0_SDA            PIO_BIT_0
#define PIO_FOR_SSC0_SCL            PIO_BIT_1
#define PIO_FOR_SSC1_SDA            PIO_BIT_2
#define PIO_FOR_SSC1_SCL            PIO_BIT_3

#elif defined(ST_5188)

#define     SSC_0_PIO_BASE              PIO_0_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_0_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_0_INTERRUPT_LEVEL
#define     SSC_1_PIO_BASE              PIO_0_BASE_ADDRESS
#define     SSC_1_PIO_INTERRUPT         PIO_0_INTERRUPT
#define     SSC_1_PIO_INTERRUPT_LEVEL   PIO_0_INTERRUPT_LEVEL
#define PIO_FOR_SSC0_SDA            PIO_BIT_1
#define PIO_FOR_SSC0_SCL            PIO_BIT_0
#define PIO_FOR_SSC1_SDA            PIO_BIT_3
#define PIO_FOR_SSC1_SCL            PIO_BIT_2
#endif

#if defined(STSPI_DEV_M25P10)
#define SECTOR0_BASE_ADDRESS    0x00000
#define SECTOR1_BASE_ADDRESS    0x08000
#define SECTOR2_BASE_ADDRESS    0x10000
#define SECTOR3_BASE_ADDRESS    0x18000
#define MEMORY_ADDRESS_START    0x00000
#define MEMORY_ADDRESS_END      0x1FFFF
#define PAGE_SIZE               256            /* bytes */
#define PAGE_MASK               0x000000FF
#define SECTOR_SIZE             0x8000
#define MEMORY_SIZE             4 * 128 * 256  /* 4 sectors of 128 pages of 256 bytes each */
#define WRONG_HANDLE            0x01234567     /* invalid handle value */
#define ERASED_STATE            0xFF
#define REACHED_TIMEOUT         100000
#define NUMBER_OF_SECTORS       4

#elif defined(STSPI_DEV_M25P80)

#define SECTOR0_BASE_ADDRESS    0x00000
#define SECTOR1_BASE_ADDRESS    0x10000
#define SECTOR2_BASE_ADDRESS    0x20000
#define SECTOR3_BASE_ADDRESS    0x30000
#define SECTOR4_BASE_ADDRESS    0x40000
#define SECTOR5_BASE_ADDRESS    0x50000
#define SECTOR6_BASE_ADDRESS    0x60000
#define SECTOR7_BASE_ADDRESS    0x70000
#define SECTOR8_BASE_ADDRESS    0x80000
#define SECTOR9_BASE_ADDRESS    0x90000
#define SECTOR10_BASE_ADDRESS   0xA0000
#define SECTOR11_BASE_ADDRESS   0xB0000
#define SECTOR12_BASE_ADDRESS   0xC0000
#define SECTOR13_BASE_ADDRESS   0xD0000
#define SECTOR14_BASE_ADDRESS   0xE0000
#define SECTOR15_BASE_ADDRESS   0xF0000
#define MEMORY_ADDRESS_START    0x00000
#define MEMORY_ADDRESS_END      0xFFFFF
#define PAGE_SIZE               256            /* bytes */
#define PAGE_MASK               0x000000FF
#define SECTOR_SIZE             0x10000
#define MEMORY_SIZE             16 * 256 * 256 /* 16 sectors of 256 pages of 256 bytes each */
#define WRONG_HANDLE            0x01234567     /* invalid handle value */
#define ERASED_STATE            0xFF
#define REACHED_TIMEOUT         100000
#define NUMBER_OF_SECTORS       16
#endif

#if defined(STSPI_DEV_M25P10)
U32 SectorBase[] = {
                      SECTOR0_BASE_ADDRESS,
                      SECTOR1_BASE_ADDRESS,
                      SECTOR2_BASE_ADDRESS,
                      SECTOR3_BASE_ADDRESS
                   };
#elif defined(STSPI_DEV_M25P80)
U32 SectorBase[] = {
                      SECTOR0_BASE_ADDRESS,
                      SECTOR1_BASE_ADDRESS,
                      SECTOR2_BASE_ADDRESS,
                      SECTOR3_BASE_ADDRESS,
                      SECTOR4_BASE_ADDRESS,
                      SECTOR5_BASE_ADDRESS,
                      SECTOR6_BASE_ADDRESS,
                      SECTOR7_BASE_ADDRESS,
                      SECTOR8_BASE_ADDRESS,
                      SECTOR9_BASE_ADDRESS,
                      SECTOR10_BASE_ADDRESS,
                      SECTOR11_BASE_ADDRESS,
                      SECTOR12_BASE_ADDRESS,
                      SECTOR13_BASE_ADDRESS,
                      SECTOR14_BASE_ADDRESS,
                      SECTOR15_BASE_ADDRESS
                   };
#endif

#if defined(ST_5188)
STPIO_Handle_t          PIOHandle;
#define PIO_BIT_MASK 0x10
#endif
#if defined(ST_5105)
U32 *SlaveSelect = (U32*)0x45180000;
#endif

/* Board/device specific constants */
#if defined(ST_5508)
#undef  TEST_FRONT_BUS                    /* Cannot SSC1 if it's not there */
#else
#define SSC_1_PRESENT
#define TEST_FRONT_BUS
#endif 

/* Other Private types/constants ------------------------------------------ */

#if defined(TEST_FRONT_BUS)
#define NUMBER_BUSES        2
#else
#define NUMBER_BUSES        1
#endif
#define MAX_DEVICES         10      /* Should be enough */

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            5000000
#define NCACHE_PARTITION_SIZE            0x10000
#define TINY_PARTITION_SIZE              32

#define TEST_PARTITION_1       system_partition
#define TEST_PARTITION_2       &the_internal_partition

#ifdef ST_OS21
#define TINY_PARTITION_1       the_tiny_partition_p
#else
#define TINY_PARTITION_1       &the_tiny_partition
#endif

enum
{
    PIO_DEVICE_0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_6,/* 5528 only */
    PIO_DEVICE_7,/* 5528 only */
    PIO_DEVICE_NOT_USED
};

#ifdef ST_OS21
#define STACK_SIZE             8*1024 + 10000
#else
#define STACK_SIZE             10000
#endif

#define TUNER_DEVICE_ID        0xD0     /* C0 might work for non-mb5518 */
#define TUNER_DEVICE_ID1       0xD2

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
    NO_DEVICE
} I2CDeviceType_t;


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

static BOOL    Quiet = FALSE;
static clock_t ClocksPerSec;
static U32 ClockSpeed;
static STI2C_Handle_t HandleTuner;
static U32 ReadTimeout=1000, WriteTimeout=1000;

#ifdef ST_OS20
/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section_noinit")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
#pragma ST_section      ( system_block,   "system_section_noinit")

#if defined(ST_5105) || defined(ST_5188)
static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
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
partition_t             *system_partition = &the_system_partition;

#else /* ST_OS20 */
#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

static U8               tiny_block [TINY_PARTITION_SIZE];
partition_t             *the_tiny_partition_p;

#endif /* ST_OS20 */

#ifdef STI2C_TRACE
extern U32 I2cPrintTrace (S32 Start, S32 Number);
#endif

/* Since the number in the arrays change, the declarations are ugly... */
static ST_DeviceName_t  PIODevName[5] = { "pio0"
#ifdef TEST_FRONT_BUS
#if !defined(ST_7710)&&!defined(ST_5700)&&!defined(ST_7100)&&!defined(ST_7109)
    /* Is this correct? PW */
                                                    , "pio0"
#else
                                                    , "pio1"
                                                    , "pio2"
                                                    , "pio3"
                                                    , "pio4"
#endif
#endif
                                                   };
/* To be filled later. */   
static ST_DeviceName_t  I2CDevName[NUMBER_BUSES];


static BOOL DoTunerTests = TRUE;
/* Private function prototypes -------------------------------------------- */

static BOOL CompareData (const U8 *b1, const U8 *b2, U32 size);
static ST_ErrorCode_t WriteTuner  (U32 address, U8 data);
static ST_ErrorCode_t ReadTuner  (U32 address, U8 *data);
ST_ErrorCode_t MemoryErase( STSPI_Handle_t Handle,U32 Address,U32 Timeout);
ST_ErrorCode_t MemoryWrite(STSPI_Handle_t Handle, U32 Address, U16 *Buffer,
                           U32 NumToWrite,U32 Timeout, U32 *NumWritten);
ST_ErrorCode_t MemoryRead(STSPI_Handle_t Handle, U32 Address, U16 *Buffer,
                          U32 NumToRead, U32 Timeout, U32 *NumRead);
ST_ErrorCode_t ReadStatus(STSPI_Handle_t Handle, U32 Address);
ST_ErrorCode_t WriteStatus(STSPI_Handle_t Handle, U32 Address);
void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );
char *I2C_ErrorString(ST_ErrorCode_t Error);
static void DisplayError( ST_ErrorCode_t ErrorCode );
void DoTheTests (void);
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
/* ------------------------------------------------------------------------- */
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

int main (void)
{
    ST_ErrorCode_t        Code;
    STBOOT_InitParams_t   InitParams;
    STBOOT_TermParams_t   TermParams;
    STPIO_InitParams_t    PioInitParams[NUMBER_PIO];
    ST_DeviceName_t       Name = "DEV0";
    ST_ClockInfo_t ClockInfo;
#if defined(ST_5188)
    STPIO_OpenParams_t      PIOOpenParams;
#endif       
    
#if defined(ST_5105) || defined(ST_5188)

    U32 ReadValue=0xFFFF;  
#endif      
    /* Begin the extremely long cachemap definition */
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
#else
    STBOOT_DCache_Area_t Cache[] =
    {
        { (U32 *)0x40080000, (U32 *)0x4FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif
#if !defined(STI2C_NO_TBX) || !defined(STSPI_NO_TBX)
    STTBX_InitParams_t    TBX_InitParams;
#endif
#ifdef ST_OS21
    /* Create memory partitions */
    system_partition     =  partition_create_heap((U8*)external_block, sizeof(external_block));
    the_tiny_partition_p =  partition_create_heap((U8*)tiny_block,sizeof(tiny_block));
#else    
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));
    partition_init_heap (&the_tiny_partition,   tiny_block,
                         sizeof(tiny_block));

    /* Avoid compiler warnings */
    internal_block_init[0] = 0;
    system_block_init[0]   = 0;
#if defined(ST_5105) || defined(ST_5188)

    data_section[0] = 0;
#endif
#endif

#if defined(ST_5105)

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
#if !defined(STI2C_NO_TBX) || !defined(STSPI_NO_TBX)
#if defined(ST_OS21)
        TBX_InitParams.CPUPartition_p       =  (ST_Partition_t *)system_partition;
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
#endif

 /* Setup PIO for SPI */
    PioInitParams[0].DriverPartition = (ST_Partition_t *)TEST_PARTITION_1;
    PioInitParams[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    PioInitParams[0].InterruptNumber = PIO_0_INTERRUPT;
    PioInitParams[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;

    myprintf(("Initializing PIO for SPI..."));
    Code = STPIO_Init(PIODevName[0], &PioInitParams[0]);
    if (Code != ST_NO_ERROR)
    {
        myprintf( "STPIO_Init() returned %d\n", Code );
        return 0;
    }
    else
    {
        myprintf("\nNo Error in PioInit...\n");
    }
#if defined(ST_5188)
    PIOOpenParams.IntHandler            =  NULL;
    PIOOpenParams.BitConfigure[4]       =  STPIO_BIT_OUTPUT_HIGH;
    PIOOpenParams.ReservedBits          =  0x10;
    Code =  STPIO_Open ( PIODevName[0],&PIOOpenParams, &PIOHandle );
    if (Code != ST_NO_ERROR)
    {
        myprintf( "STPIO_Init() returned %d\n", Code );
        return 0;
    }
#endif
    
#if defined(ST_5105)
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F, STSYS_ReadRegDev32LE(CONFIG_CONTROL_F) | 0x00000046 );
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, STSYS_ReadRegDev32LE(CONFIG_CONTROL_H) | 0x00000000 );
#elif defined(ST_5188)
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) | 0x000C0022 );
#endif
    
            ClocksPerSec = ST_GetClocksPerSecond();
            ST_GetClockInfo(&ClockInfo);
            ClockSpeed = ClockInfo.CommsBlock;
            DoTheTests();
       
        STBOOT_Term (Name, &TermParams);
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
/* Some utility functions for checking various things.                      */
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
        myprintf( "----------------------------------------------------------\n" );
        return FALSE;
    }
}
static BOOL CheckCodeOk (ST_ErrorCode_t actualCode)
{
    return CheckCode (actualCode, ST_NO_ERROR);
}
/*---------------------------------------------------------------------------*/
/* Functions for R/W to Tuner & EEPROM.                                      */
/* For Tuner Write  :   1 byte address + 1 byte data                         */
/* For Tuner Read   :   Write 1 byte address; Read 1 byte data               */

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
    task_delay(2);
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

ST_ErrorCode_t ReadStatus(STSPI_Handle_t Handle, U32 Address)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 WriteBuf[1];
    U16 ReadBuf[1] = { 0xFF };
    U32 ActLen;
    U32 Timeout = 0;

#if defined(ST_5188)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]    = (U8)FLASH_READ_STATUS;

    Error = STSPI_Write( Handle, WriteBuf, 1, 100, &ActLen );
    if (Error != ST_NO_ERROR)
    {
        myprintf("STSPI_Write Failed :\n");
    }
    else if (ActLen != 1)
    {
        myprintf("ERROR: actLen for write = %d\n", ActLen);
    }
    else /* Read the status now */
    {
        /* Check for WIP bit */
        while (ReadBuf[0] & 0x01)
        {
            /* No errors during Read Status Reg comm, so start reading the status */
            Error = STSPI_Read ( Handle, ReadBuf, 1, 100, &ActLen);
            if (Error != ST_NO_ERROR)
            {
                myprintf("STSPI_Read Failed :\n");
                break;
            }
            else if (ActLen != 1)
            {
                myprintf("ERROR: %d bytes read, 1 expected\n", ActLen);
                break;
            }

            if ( Timeout++ == REACHED_TIMEOUT )
            {
                Error = ST_ERROR_TIMEOUT;
                break;
            }

        }
#if defined(ST_5188)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
        *SlaveSelect = CHIP_SELECT_HIGH;
#endif
        /* Check for block protection bits BP1 & BP0 */
        if ((ReadBuf[0] & 0x08) && (ReadBuf[0] & 0x04))
        {
            (void)WriteStatus( Handle, Address );
        }
        else
        {
            if ((ReadBuf[0] & 0x08) || (ReadBuf[0] & 0x04))
            {
                if ((Address > MEMORY_ADDRESS_START) &&
                    (Address < MEMORY_ADDRESS_END))
                {
                    (void)WriteStatus(Handle, Address);
                }
            }
        }

    }

    return(Error);
} /* Read Status Reg */

ST_ErrorCode_t WriteStatus(STSPI_Handle_t Handle, U32 Address)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 WriteBuf[2];
    U32 ActLen;

    /* Drive CS signal high to complete the execution of prev instruction*/
#if defined(ST_5188)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]  = (U8)FLASH_WRITE_ENABLE;

    Error = STSPI_Write( Handle, WriteBuf, 1 , 100, &ActLen );
    if (Error != ST_NO_ERROR)
    {
        myprintf("STSPI_Write Failed :\n");
    }
#if defined(ST_5188)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]    = (U8)FLASH_WRITE_STATUS;
    WriteBuf[1]    = (U8)0x00; /* BR1 =0 BP0 =0 to unlock */

    Error = STSPI_Write( Handle, WriteBuf, 2 , 100, &ActLen );
    if (Error != ST_NO_ERROR)
    {
        myprintf("STSPI_Write Failed :\n");
    }
    else if (ActLen != 2)
    {
        myprintf("ERROR: actLen for write = %d\n", ActLen);
    }
    else /* Read the status now to reconfirm unlocking */
    {
        Error = ReadStatus( Handle,Address );
    }

    return(Error);

} /* Write Status Reg */

ST_ErrorCode_t MemoryErase( STSPI_Handle_t Handle,U32 Address,U32 Timeout)
{

    U32 NumWritten;
    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 Buffer[4];

    /* Read status register to check for locking status */
    Error = ReadStatus( Handle, Address );
    if (Error != ST_NO_ERROR)
    {
        myprintf("Error in Reading Status Reg\n");
    }
    else
    {
        /* Drive CS signal high to complete the execution of prev instruction*/
#if defined(ST_5188)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
        STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
        *SlaveSelect = CHIP_SELECT_HIGH;
        *SlaveSelect = CHIP_SELECT_LOW;
#endif

        Buffer[0] = (U8)FLASH_WRITE_ENABLE;

        /* Enter Write Enable cmd */
        Error = STSPI_Write( Handle, Buffer, 1, Timeout, &NumWritten );
        if (Error != ST_NO_ERROR)
        {
            myprintf(" STSPI_Write Failed :\n");
            return(Error);
        }
#if defined(ST_5188)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
        STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
        *SlaveSelect = CHIP_SELECT_HIGH;
        *SlaveSelect = CHIP_SELECT_LOW;
#endif
        Buffer[0] = (U8)FLASH_SECTOR_ERASE;
        Buffer[1] = (U8)(Address >> 16);
        Buffer[2] = (U8)(Address >> 8);
        Buffer[3] = (U8)(Address);

        Error = STSPI_Write( Handle, Buffer, 4, Timeout, &NumWritten );
        if (Error != ST_NO_ERROR)
        {
            myprintf(" STSPI_Write Failed :\n");
        }
        else if (NumWritten != 4)
        {
            myprintf("Had to write 4 bytes but actually wrote %d\n",NumWritten);
        }
#if defined(ST_5188)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
        /* Drive chip select HIGH to complete the execution of prev instruction */
        *SlaveSelect = CHIP_SELECT_HIGH;
#endif
    }

    return(Error);

} /* Memory Erase */

ST_ErrorCode_t MemoryRead(STSPI_Handle_t Handle, U32 Address, U16 *ReadBuf,
                                 U32 NumToRead, U32 Timeout, U32 *NumRead)
{

    U32 NumWritten;
    U16 WriteBuf[4];
    ST_ErrorCode_t Error = ST_NO_ERROR;

#if defined(ST_5188)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
    /* Drive CS signal low */
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0] = (U8)FLASH_DATA_READ;
    WriteBuf[1] = (U8)(Address >> 16);
    WriteBuf[2] = (U8)(Address >> 8);
    WriteBuf[3] = (U8)(Address);

    Error = STSPI_Write( Handle, WriteBuf, 4, Timeout, &NumWritten );
    if (Error != ST_NO_ERROR)
    {
        myprintf(" STSPI_Write Failed :\n");

    }
    else if (NumWritten != 4)
    {
        myprintf("Had to write 4 bytes but actually wrote %d\n",NumWritten);
    }

    if (Error == ST_NO_ERROR)
    {
        Error = STSPI_Read( Handle, ReadBuf, NumToRead, Timeout, NumRead );
        if (Error != ST_NO_ERROR)
        {
            myprintf("STSPI_Read Failed with %d\n",Error);

        }
        else if (*NumRead != NumToRead)
        {
            myprintf("Had to read %d bytes but actually read %d\n",NumToRead,*NumRead);
        }

    }
#if defined(ST_5188)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
    *SlaveSelect = CHIP_SELECT_HIGH;
#endif
    return(Error);

}

ST_ErrorCode_t MemoryWrite( STSPI_Handle_t Handle, U32 Address, U16 *WriteBuf,
                             U32 NumToWrite, U32 Timeout, U32 *NumWritten)
{

    ST_ErrorCode_t Error;
    U16 Buffer[4];
    U32 BytesToWrite = 0,BytesWritten = 0,Offset =0;

    while ( NumToWrite > 0)
    {

        Error = ReadStatus( Handle ,Address);
        if (Error != ST_NO_ERROR)
        {
            myprintf("Error in Reading Status Reg\n");
            break;
        }
        else
        {
            /* CS should be LOW while giving cmd */
#if defined(ST_5188)
            STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
            Buffer[0] = (U8)FLASH_WRITE_ENABLE;

            Error = STSPI_Write( Handle, Buffer, 1, Timeout, NumWritten );
            if (Error != ST_NO_ERROR)
            {
                myprintf(" STSPI_Write Failed :\n");
                return(Error);
            }

#if defined(ST_5188)
            /* Drive CS signal high to complete the execution of prev instruction */
            STPIO_Set(PIOHandle,PIO_BIT_MASK);
            /* Drive CS low to start the Write */
            STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_5105)
            /* Drive CS signal high to complete the execution of prev instruction */
            *SlaveSelect = CHIP_SELECT_HIGH;
            /* Drive CS low to start the Write */
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
            Buffer[0] = (U8)FLASH_PAGE_PROGRAM;
            Buffer[1] = (U8)(Address >> 16);
            Buffer[2] = (U8)(Address >> 8);
            Buffer[3] = (U8)(Address);

            Error = STSPI_Write( Handle, Buffer, 4, Timeout, NumWritten );
            if (Error != ST_NO_ERROR)
            {
                myprintf(" STSPI_Write Failed :\n");
                return(Error);
            }
            else if (*NumWritten != 4)
            {
                myprintf("Had to write 4 bytes but actually wrote %d\n",*NumWritten);
                return(Error);
            }

            Offset = Address & PAGE_MASK;

            if ( NumToWrite > PAGE_SIZE)
            {
                BytesToWrite = PAGE_SIZE;
                if ((Offset + BytesToWrite) > PAGE_SIZE)
                {
                    BytesToWrite -= Offset;
                }
            }
            else
            {

                if ((Offset + NumToWrite) > PAGE_SIZE)
                {
                    BytesToWrite = NumToWrite - Offset;
                }
                else
                {
                    BytesToWrite = NumToWrite;
                }

            }

            Error = STSPI_Write( Handle, WriteBuf, BytesToWrite, Timeout, NumWritten );
            if (Error != ST_NO_ERROR)
            {
                myprintf(" STSPI_Write Failed :\n");
                return(Error);
            }
            else if (*NumWritten != BytesToWrite)
            {
                myprintf("Had to write %d bytes but actually wrote %d\n",NumToWrite,*NumWritten);
                return(Error);
            }
            /* Drive CS HIGH to complete PP instruction */
#if defined(ST_5188)
            STPIO_Set(PIOHandle,PIO_BIT_MASK);
            task_delay(100);
#elif defined(ST_5105)
            *SlaveSelect = CHIP_SELECT_HIGH;
#endif
            NumToWrite   -= *NumWritten;
            Address      += *NumWritten;
            BytesWritten += *NumWritten;
        }
    }

    *NumWritten = BytesWritten;
    return(Error);

}

/**************************************************************************
        THIS FUNCTION WILL CHECK DYNAMIC SWITCHING OF I2C & SPI...
**************************************************************************/

void DoTheTests(void)
{
    /* Declarations ---------------------------------------------------------*/
    U32                 NumberRead;
    U32                 NumberWrit;
    ST_ErrorCode_t      ReturnError;
    U32                 i;
    int  ErrorCount     = 0;
    STSPI_InitParams_t  SPIInitParams;
    STSPI_OpenParams_t  SPIOpenParams;
    STSPI_Handle_t      SPIHandle;
    STSPI_TermParams_t  SPITermParams;

    static U16          ReadArray[SECTOR_SIZE]={0};
    static U16          WritArray[SECTOR_SIZE];
    /* Init block 0 setup and term */
    
    STI2C_InitParams_t   I2cInitParams;
    STI2C_TermParams_t   I2cTermParams;

    /* Init block 0 PIO */
#if !defined(ST_8010)&&!defined(ST_5525)
    STPIO_InitParams_t   PIOInitParams;
#endif

#ifdef SSC_1_PRESENT
    /* Init block 1 setup and term */
    
    STI2C_InitParams_t   I2cInitParams1;
    STI2C_TermParams_t   I2cTermParams1;

    /* Init block 1 PIO */
#if !defined(ST_8010)&&!defined(ST_5525)
    STPIO_InitParams_t   PIOInitParams1;
#endif
#endif
                                  
 /* Open blocks */
    STI2C_OpenParams_t   I2cOpenParams;

    /* Read and write */
    U8                   ReadBuffer[1024];
    U8                   WriteTunerData1      = 0x18;

    /* Initialisation ------------------------------------------------------*/

    /* Block 0 */
    strcpy( I2CDevName[0], "I2C-0" );
    I2cInitParams.BaseAddress           = (U32*)SSC_0_BASE_ADDRESS;
    I2cInitParams.InterruptNumber       = SSC_0_INTERRUPT;
    I2cInitParams.InterruptLevel        = SSC_0_INTERRUPT_LEVEL;

#ifdef BAUD_RATE_FAST
    I2cInitParams.BaudRate              = STI2C_RATE_FASTMODE;
#else
    I2cInitParams.BaudRate              = STI2C_RATE_NORMAL;
#endif

    I2cInitParams.MasterSlave           = STI2C_MASTER;
    I2cInitParams.ClockFrequency        = ClockSpeed;
    I2cInitParams.GlitchWidth           = 0;
    I2cInitParams.PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    I2cInitParams.PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;
    I2cInitParams.MaxHandles            = 6;
#if defined(ST_OS21)
    I2cInitParams.DriverPartition       = (ST_Partition_t *)system_partition;
#else
    I2cInitParams.DriverPartition       = TEST_PARTITION_1;
#endif
    I2cInitParams.FifoEnabled           = FALSE;
#if defined(ST_5105)
    strcpy( I2cInitParams.PIOforSDA.PortName, PIODevName[3] );
    strcpy( I2cInitParams.PIOforSCL.PortName, PIODevName[3] );
#elif defined(ST_5188)
    strcpy( I2cInitParams.PIOforSDA.PortName, PIODevName[0] );
    strcpy( I2cInitParams.PIOforSCL.PortName, PIODevName[0] );
#endif    

#if !defined(ST_8010)&&!defined(ST_5525)
    PIOInitParams.BaseAddress           = (U32*)SSC_0_PIO_BASE;
    PIOInitParams.InterruptNumber       = SSC_0_PIO_INTERRUPT;
    PIOInitParams.InterruptLevel        = SSC_0_PIO_INTERRUPT_LEVEL;
#if defined(ST_OS21)
    PIOInitParams.DriverPartition       = (ST_Partition_t *)system_partition;
#else        
    PIOInitParams.DriverPartition       = TEST_PARTITION_1;
#endif    
#endif

#ifdef SSC_1_PRESENT
    /* Block 1 */
    strcpy(I2CDevName[1], "I2C-1" );
    I2cInitParams1.BaseAddress          = (U32*)SSC_1_BASE_ADDRESS;
    I2cInitParams1.InterruptNumber      = SSC_1_INTERRUPT;
    I2cInitParams1.InterruptLevel       = SSC_1_INTERRUPT_LEVEL;
#ifdef BAUD_RATE_FAST
    I2cInitParams1.BaudRate             = STI2C_RATE_FASTMODE;
#else
    I2cInitParams1.BaudRate             = STI2C_RATE_NORMAL;
#endif
    I2cInitParams1.MasterSlave          = STI2C_MASTER;
    I2cInitParams1.ClockFrequency       = ClockSpeed;
    I2cInitParams1.GlitchWidth          = 0;
    I2cInitParams1.PIOforSDA.BitMask    = PIO_FOR_SSC1_SDA;
    I2cInitParams1.PIOforSCL.BitMask    = PIO_FOR_SSC1_SCL;
    I2cInitParams1.MaxHandles           = 6;
#if defined(ST_OS21)
    I2cInitParams1.DriverPartition       = (ST_Partition_t *)system_partition;
#else        
    I2cInitParams1.DriverPartition       = TEST_PARTITION_1;
#endif    
    I2cInitParams1.FifoEnabled           = FALSE;
#if defined(ST_5105)
    strcpy( I2cInitParams1.PIOforSDA.PortName, PIODevName[3] );
    strcpy( I2cInitParams1.PIOforSCL.PortName, PIODevName[3] );
#elif defined(ST_5188)
    strcpy( I2cInitParams1.PIOforSDA.PortName, PIODevName[0] );
    strcpy( I2cInitParams1.PIOforSCL.PortName, PIODevName[0] );
#endif 
#if !defined(ST_8010)&&!defined(ST_5525)
    PIOInitParams1.BaseAddress          = (U32*)SSC_1_PIO_BASE;
    PIOInitParams1.InterruptNumber      = SSC_1_PIO_INTERRUPT;
    PIOInitParams1.InterruptLevel       = SSC_1_PIO_INTERRUPT_LEVEL;
#if defined(ST_OS21)
    PIOInitParams1.DriverPartition       = (ST_Partition_t *)system_partition;
#else
    PIOInitParams1.DriverPartition       = TEST_PARTITION_1;
#endif
#endif
#endif   
 
    /* Open block */
    I2cOpenParams.BusAccessTimeOut     = 200;
    I2cOpenParams.AddressType          = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.I2cAddress           = TUNER_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams.BaudRate             = STI2C_RATE_NORMAL;
#endif
    /* Term blocks */
    I2cTermParams.ForceTerminate        = FALSE;
#ifdef SSC_1_PRESENT
    I2cTermParams1.ForceTerminate       = FALSE;
#endif
    /* Program --------------------------------------------------------------*/
    myprintf("\nSTAPI I2C DRIVER TEST SUITE\n\n");

    myprintf("TEST_FRONT_BUS is set\n");
#if !defined(ST_8010)&&!defined(ST_5525)
#if defined(ST_5105)	/*PIO_Init already done for PIO[0] for 5188 in main*/
    /* Set up the PIO */
    myprintf("Initialising PIO for I2C...\n");
    CheckCodeOk (STPIO_Init( PIODevName[3], &PIOInitParams1 ) );
#endif    
#endif

/********  SPI Initialize & Open **********/
    SPIInitParams.BaseAddress           = (U32*)SSC_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC_INTERRUPT_LEVEL;
    SPIInitParams.ClockFrequency        = ClockSpeed; /* 10 MHz */
    SPIInitParams.BusAccessTimeout      = 100000;
    SPIInitParams.MaxHandles            = 2;
    SPIInitParams.DriverPartition       = TEST_PARTITION_1;
    SPIInitParams.MasterSlave           = STSPI_MASTER;
    SPIInitParams.PIOforMTSR.BitMask    = SSC_MTSR_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC_MRST_BIT;
    SPIInitParams.PIOforSCL.BitMask     = SSC_SCL_BIT;

    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC_MRST_DEV] );
    strcpy( SPIInitParams.PIOforSCL.PortName, PIODevName[SSC_SCL_DEV] );
    SPIInitParams.DefaultParams         = NULL;

       
/************* I2C initialize & open ******************/
#if defined (ST_5188)
    myprintf("\nCalling STI2C_Init()...........\n");
    CheckCodeOk (STI2C_Init( I2CDevName[0], &I2cInitParams ) );
    
    myprintf("Calling STI2C_Open()...........\n");
    CheckCodeOk (STI2C_Open(I2CDevName[0], &I2cOpenParams, &HandleTuner));
#elif defined(ST_5105)
    myprintf("\nCalling STI2C_Init()...........\n");
    CheckCodeOk (STI2C_Init( I2CDevName[1], &I2cInitParams1 ));
    
    myprintf("Calling STI2C_Open()...........\n");
    CheckCodeOk (STI2C_Open(I2CDevName[1], &I2cOpenParams, &HandleTuner));
#endif
/************* SPI initialize & open ******************/
    myprintf( "Calling STSPI_Init() ..........\n" );
    ReturnError= STSPI_Init( "SSC_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    /* Open Sector 0 */
    myprintf( "Calling STSPI_Open() ..........\n" );
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[SSC_CS_DEV] );
    SPIOpenParams.PIOforCS.BitMask     = SSC_CS_BIT;
    ReturnError= STSPI_Open( "SSC_SPI",&SPIOpenParams,&SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR ); 

    
/**************************************************************************************/
    /* Program Sector 0 */
    for (i = 0; i < SECTOR_SIZE; i++)
    {
        WritArray[i] = (U8)i;
    }

    WriteTuner (TUNER_OFFSET_1, WriteTunerData1);
    ReadTuner (TUNER_OFFSET_1, ReadBuffer);
    CompareData (ReadBuffer, &WriteTunerData1, 1);

    myprintf( "\n(Flash)- Calling MemoryErase() to Erase Sector0 ..........\n" );
    ReturnError = MemoryErase( SPIHandle,SECTOR0_BASE_ADDRESS,10000);
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    myprintf( "\n(Flash)- Calling STSPI_Write() to write into (Flash) Sector 0..\n" );
    ReturnError= MemoryWrite( SPIHandle,SECTOR0_BASE_ADDRESS,WritArray,SECTOR_SIZE,10000,&NumberWrit );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    myprintf( "Number Actually Written are %d bytes\n",NumberWrit );
    myprintf( "\n(Flash)- Calling STSPI_Read() to verify write on (Flash) Sector 0...\n" );
    ReturnError= MemoryRead( SPIHandle,SECTOR0_BASE_ADDRESS,ReadArray,SECTOR_SIZE,10000,&NumberRead );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    myprintf( "Number Actually Read are %d bytes\n",NumberRead );

    WriteTuner (TUNER_OFFSET_1, WriteTunerData1);
    ReadTuner (TUNER_OFFSET_1, ReadBuffer);
    CompareData (ReadBuffer, &WriteTunerData1, 1);
    
    myprintf("\nCalling STI2C_Close() .........\n");
    CheckCodeOk (STI2C_Close( HandleTuner ) );
    myprintf("Calling STSPI_Close() .........\n");
    CheckCodeOk (STSPI_Close( SPIHandle ) );
    
    /* Term block 0 */
    myprintf("Calling STI2C_Term() .........\n");
#if defined (ST_5188)
    CheckCodeOk (STI2C_Term( I2CDevName[0], &I2cTermParams ) );
#elif defined(ST_5105)
    CheckCodeOk (STI2C_Term( I2CDevName[1], &I2cTermParams1 ) );
#endif    
    SPITermParams.ForceTerminate = FALSE;
    myprintf( "Calling STSPI_Term() .........\n" );
    CheckCodeOk(STSPI_Term( "SSC_SPI", &SPITermParams ));
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
    { STI2C_ERROR_NO_FREE_SSC, "STI2C_ERROR_NO_FREE_SSC" },
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
/****************************************************************************
Name         : ErrorReport()

Description  : Expands the returned error code to a message string,
               followed by the expected code/message if different.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/

void ErrorReport( int *ErrorCount,ST_ErrorCode_t ErrorGiven,ST_ErrorCode_t ExpectedErr )
{
    ST_ErrorCode_t  Temp = ErrorGiven;

    switch (Temp)
    {
        case ST_NO_ERROR:
            myprintf(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            myprintf(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            myprintf(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            myprintf(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            myprintf(( "ST_ERROR_INVALID_HANDLE - Rogue Handle value or Closed\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            myprintf(( "ST_ERROR_OPEN_HANDLE - Term called with Open Handle\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            myprintf(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            myprintf(( "ST_ERROR_NO_FREE_HANDLES - DeviceName already Open\n" ));
            break;

        case ST_ERROR_TIMEOUT:
            myprintf(( "ST_ERROR_TIMEOUT - Timeout limit reached\n" ));
            break;

        case STSPI_ERROR_NO_FREE_SSC:
            myprintf(( "STSPI_ERROR_NO_FREE_SSC - No Free SSC\n" ));
            break;

        case STSPI_ERROR_DATAWIDTH:
            myprintf(( "STSPI_ERROR_DATAWIDTH - Incorrect DataWidth\n" ));
            break;

        case STSPI_ERROR_PIO:
            myprintf(( "STSPI_ERROR_PIO - Error in PIO access\n" ));
            break;

        case STSPI_ERROR_EVT_REGISTER:
            myprintf(( "STSPI_ERROR_EVT_REGISTER - Error in EVT registration\n" ));
            break;

        default:
            myprintf( "*** Unrecognised return code %x ***\n",Temp);
    }

        if (ErrorGiven != ExpectedErr)
        {
            (*ErrorCount)++;
        }

    myprintf(( "----------------------------------------------------------\n" ));
}
