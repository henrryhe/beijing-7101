/****************************************************************************

File Name   : sflashtst.c

Description : serial flash memory test

Copyright (C) 2006, ST Microelectronics

History     :

    16/01/06  Added SPI support

References  :

$ClearCase (VOB: stflash)

****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "stdevice.h"
#include "stack.h"
#include "stlite.h"
#ifndef ST_OS21
#include "ostime.h"
#endif

#include "stos.h"

#include "stddefs.h"
#include "stflash.h"
#include "stboot.h"
#include "stcommon.h"
#include "stspi.h"
#include "stsys.h"
#include "stflashtst.h"
#ifdef USE_TESTTOOL
#include "testtool.h"
#endif

void DeviceConfig(void);
char *GetDeviceInfo(int DeviceNum);

/* Private Types ---------------------------------------------------------- */
/* Private Constants ------------------------------------------------------ */
static U32 ClockSpeed;

/* Enum of PIO devices */
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

ST_DeviceName_t      PIODevName[] =
{
    "pio0",
    "pio1",
    "pio2",
    "pio3",
    "pio4",
    "pio5",
    "pio6", /*5528*/
    "pio7", /*5528*/
    ""
};

/* Declarations for memory partitions */
#ifndef ST_OS21

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            0x400000

/* Memory partitions */
#define TEST_PARTITION_1        &the_system_partition
#define TEST_PARTITION_2        &the_internal_partition

/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block, "internal_section")
#endif

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( system_block,   "system_section")
#endif

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")
#endif

static unsigned char    system_block_noinit[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( system_block_noinit, "system_section_noinit")
#endif

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5528) || defined(ST_5105) || defined(ST_5301) \
|| defined(ST_7109) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107)
static unsigned char    data_section[1];
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( data_section, "data_section")
#endif
#endif

#else /* ST_OS21 */

#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#endif /* ST_OS21 */


/* Private Variables ------------------------------------------------------ */

static ST_ErrorCode_t       retVal;

/* Suppress linker warning */
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( retVal,   "ncache_section")
#endif

#if defined(FLASH_TEST_M25P10)
#define DEVICE_TYPE STFLASH_M25P10
#elif defined(FLASH_TEST_M25P80)
#define DEVICE_TYPE STFLASH_M25P80
#elif defined(FLASH_TEST_M25P16)
#define DEVICE_TYPE STFLASH_M25P16
#else
#define DEVICE_TYPE STFLASH_M25P10 /* default */
#endif

#if defined(FLASH_TEST_M25P10)
#define SECTOR0_BASE_ADDRESS    0x00000
#define SECTOR1_BASE_ADDRESS    0x08000
#define SECTOR2_BASE_ADDRESS    0x10000
#define SECTOR3_BASE_ADDRESS    0x18000

#define MEMORY_ADDRESS_START    0x00000
#define MEMORY_ADDRESS_END      0x1FFFF
#define SECTOR_SIZE             0x8000

#define MEMORY_SIZE             4 * 128 * 256  /* 4 sectors of 128 pages of 256 bytes each */
#define WRONG_HANDLE            0x01234567     /* invalid handle value */
#define ERASED_STATE            0xFF
#define REACHED_TIMEOUT         100000

#define NUMBER_OF_SECTORS       4

STFLASH_Block_t   BlockData_s[NUMBER_OF_SECTORS] =
{
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK }
};

#elif defined(FLASH_TEST_M25P80)

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

#define SECTOR_SIZE             0x10000
#define MEMORY_SIZE             16 * 256 * 256 /* 16 sectors of 256 pages of 256 bytes each */
#define WRONG_HANDLE            0x01234567     /* invalid handle value */
#define ERASED_STATE            0xFF
#define REACHED_TIMEOUT         100000

#define NUMBER_OF_SECTORS       16

STFLASH_Block_t   BlockData_s[NUMBER_OF_SECTORS] =
{
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK }
};

#elif defined(FLASH_TEST_M25P16)

#define SECTOR0_BASE_ADDRESS    0x000000
#define SECTOR1_BASE_ADDRESS    0x010000
#define SECTOR2_BASE_ADDRESS    0x020000
#define SECTOR3_BASE_ADDRESS    0x030000
#define SECTOR4_BASE_ADDRESS    0x040000
#define SECTOR5_BASE_ADDRESS    0x050000
#define SECTOR6_BASE_ADDRESS    0x060000
#define SECTOR7_BASE_ADDRESS    0x070000
#define SECTOR8_BASE_ADDRESS    0x080000
#define SECTOR9_BASE_ADDRESS    0x090000
#define SECTOR10_BASE_ADDRESS   0x0A0000
#define SECTOR11_BASE_ADDRESS   0x0B0000
#define SECTOR12_BASE_ADDRESS   0x0C0000
#define SECTOR13_BASE_ADDRESS   0x0D0000
#define SECTOR14_BASE_ADDRESS   0x0E0000
#define SECTOR15_BASE_ADDRESS   0x0F0000
#define SECTOR16_BASE_ADDRESS   0x100000
#define SECTOR17_BASE_ADDRESS   0x110000
#define SECTOR18_BASE_ADDRESS   0x120000
#define SECTOR19_BASE_ADDRESS   0x130000
#define SECTOR20_BASE_ADDRESS   0x140000
#define SECTOR21_BASE_ADDRESS   0x150000
#define SECTOR22_BASE_ADDRESS   0x160000
#define SECTOR23_BASE_ADDRESS   0x170000
#define SECTOR24_BASE_ADDRESS   0x180000
#define SECTOR25_BASE_ADDRESS   0x190000
#define SECTOR26_BASE_ADDRESS   0x1A0000
#define SECTOR27_BASE_ADDRESS   0x1B0000
#define SECTOR28_BASE_ADDRESS   0x1C0000
#define SECTOR29_BASE_ADDRESS   0x1D0000
#define SECTOR30_BASE_ADDRESS   0x1E0000
#define SECTOR31_BASE_ADDRESS   0x1F0000

#define MEMORY_ADDRESS_START    0x000000
#define MEMORY_ADDRESS_END      0x1FFFFF

#define SECTOR_SIZE             0x10000
#define MEMORY_SIZE             25 * 256 * 256 /* Actual total memory is (32 * 256 * 256): 32 sectors of 256
                                                  pages each of 256 bytes each, we are testing on for 25 sectors */
#define WRONG_HANDLE            0x01234567     /* invalid handle value */
#define ERASED_STATE            0xFF
#define REACHED_TIMEOUT         100000

#define NUMBER_OF_SECTORS       32

STFLASH_Block_t   BlockData_s[NUMBER_OF_SECTORS] =
{
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK }
};

#else /* default serial flash is M25P10 */

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

STFLASH_Block_t   BlockData_s[NUMBER_OF_SECTORS] =
{
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK },
    { SECTOR_SIZE, STFLASH_MAIN_BLOCK }
};

#endif



STFLASH_InitParams_t    InitParams_s;
#if defined(ST_5188)
STFLASH_SPIConfig_t     STFLASH_SPIConfig_s;
STFLASH_SPIModeSelect_t STFLASH_SPIMode_s;
#endif
STFLASH_Handle_t        Handle0, Handle1;
STFLASH_OpenParams_t    OpenParams_s;
STFLASH_TermParams_t    TermParams_s;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */
static int FullERW( void );

#if defined(FLASH_FAST_READ)
static int SPIFastReadTest( void );
#endif

#if defined(ST_5188)
static int SPIConfig( void );
#endif
void ExTimeReport( clock_t TimeSnap );

void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );

void ParamsReport( STFLASH_Params_t *RetParams );
void os20_main( void *ptr);

#ifdef USE_TESTTOOL
BOOL FLASH_RegisterTest( void );
static BOOL SPI_FullERW( STTST_Parse_t *pars_p, char *Result );
#if defined(ST_5188)
static BOOL SPI_SetConfig( STTST_Parse_t *pars_p, char *Result );
static BOOL SPI_SetFastRead( STTST_Parse_t *pars_p, char *Result );
#endif
#endif

#if defined(ST_7100) || defined(ST_7109)
#define SSC_BASE_ADDRESS       SSC_1_BASE_ADDRESS
#define SSC_INTERRUPT          SSC_1_INTERRUPT
#define SSC_INTERRUPT_LEVEL    SSC_1_INTERRUPT_LEVEL

#define SSC_MTSR_BIT           PIO_BIT_1
#define SSC_MTSR_DEV           PIO_DEVICE_3

#define SSC_MRST_BIT           PIO_BIT_2
#define SSC_MRST_DEV           PIO_DEVICE_3

#define SSC_SCL_BIT            PIO_BIT_0
#define SSC_SCL_DEV            PIO_DEVICE_3

#define SSC_CS_BIT             PIO_BIT_5
#define SSC_CS_DEV             PIO_DEVICE_3

#define NUM_PIO                4
#define PIO_FOR_CS             PIO_BIT_7
#define PIO_DEV_FOR_CS         PIO_DEVICE_3

#elif defined(ST_5188)

#define NUM_PIO                4

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

#define PIO_FOR_CS             PIO_BIT_4
#define PIO_DEV_FOR_CS         PIO_DEVICE_0

#define INTERCONNECT_BASE                       (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)


#elif defined(ST_5105) || defined(ST_5107)

#define NUM_PIO                4
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

#define PIO_FOR_CS             PIO_BIT_4
#define PIO_DEV_FOR_CS         PIO_DEVICE_0

#define INTERCONNECT_BASE                       (U32)0x20402000
#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#elif defined(ST_5525)

#define NUM_PIO                9

#define SSC_BASE_ADDRESS       SSC_2_BASE_ADDRESS
#define SSC_INTERRUPT          SSC_2_INTERRUPT
#define SSC_INTERRUPT_LEVEL    5

#define SSC_MTSR_BIT           PIO_BIT_4
#define SSC_MTSR_DEV           PIO_DEVICE_2

#define SSC_MRST_BIT           PIO_BIT_4
#define SSC_MRST_DEV           PIO_DEVICE_3

#define SSC_SCL_BIT            PIO_BIT_5
#define SSC_SCL_DEV            PIO_DEVICE_2

#define SSC_CS_BIT             PIO_BIT_3
#define SSC_CS_DEV             PIO_DEVICE_3

#define PIO_FOR_CS             PIO_BIT_3
#define PIO_DEV_FOR_CS         PIO_DEVICE_3

#endif



/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : none

Return Value : int

See Also     : OS20_main
 ****************************************************************************/
int main(int argc, char *argv[])
{
#if defined(ARCHITECTURE_ST40) && !defined(ST_OS21)
    printf ("\nStarting OS20_main() ...\n");
    setbuf(stdout, NULL);
    OS20_main(argc, argv, os20_main);
#else
    os20_main(NULL);
#endif
    fflush (stdout);
    return(0);
}

void os20_main( void *ptr)
{

    int                     ErrorCount=0,Error = 0;
#ifndef DISABLE_TOOLBOX
    STTBX_InitParams_t      sttbx_InitPars;
#endif
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    STPIO_InitParams_t      stpio_InitPars[NUM_PIO];
    STSPI_InitParams_t      stspi_InitPars;
    ST_ClockInfo_t          ClockInfo;
    int i =0;

#ifdef USE_TESTTOOL
    STTST_InitParams_t      sttst_InitPars;
#endif

#ifndef DISABLE_DCACHE

#if defined CACHEABLE_BASE_ADDRESS

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS},
        { NULL, NULL }
    };

#elif defined(ST_5514) || defined(ST_5516)

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };

#elif defined(ST_5528) || defined(ST_5100)

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40000000, (U32 *)0x5FFFFFFF}, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5301)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0xC0000000, (U32 *)0xC1FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_8010)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5525)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x80000000, (U32 *)0x81FFFFFF }, /* ok */
        { NULL, NULL }
    };
#else

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x7FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif

#endif /* end of !defined DISABLE_DCACHE */

#ifdef ARCHITECTURE_ST20
    /* to avoid linker warnings */
    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5528) || defined(ST_5105) || defined(ST_5301) \
|| defined(ST_7109) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107)
    data_section[0] = 0;
#endif
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));

#else /* ifndef ST_OS21 */
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));
#endif /* ST_OS21 */

#ifndef DISABLE_TOOLBOX

    /* Initialize the toolbox */
#ifdef ST_OS21
    sttbx_InitPars.CPUPartition_p      = system_partition;
#else
    sttbx_InitPars.CPUPartition_p      = TEST_PARTITION_1;
#endif

    sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

#endif /* disable TBX */

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

#ifdef DISABLE_DCACHE
    stboot_InitPars.DCacheMap                 = NULL;
#else
    stboot_InitPars.DCacheMap                 = DCacheMap;
#endif

    stboot_InitPars.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    stboot_InitPars.SDRAMFrequency            = SDRAM_FREQUENCY;

    /* Setup PIO global parameters */
#ifdef ST_OS21
    stpio_InitPars[0].DriverPartition = system_partition;
#else
    stpio_InitPars[0].DriverPartition = TEST_PARTITION_1;
#endif
    stpio_InitPars[0].BaseAddress     = (U32 *)PIO_0_BASE_ADDRESS;
    stpio_InitPars[0].InterruptNumber = PIO_0_INTERRUPT;
    stpio_InitPars[0].InterruptLevel  = PIO_0_INTERRUPT_LEVEL;

#if (NUM_PIO > 1)

#ifdef ST_OS21
    stpio_InitPars[1].DriverPartition = system_partition;
#else
    stpio_InitPars[1].DriverPartition = TEST_PARTITION_1;
#endif
    stpio_InitPars[1].BaseAddress     = (U32 *)PIO_1_BASE_ADDRESS;
    stpio_InitPars[1].InterruptNumber = PIO_1_INTERRUPT;
    stpio_InitPars[1].InterruptLevel  = PIO_1_INTERRUPT_LEVEL;
#endif

#if (NUM_PIO > 2)

#ifdef ST_OS21
    stpio_InitPars[2].DriverPartition = system_partition;
#else
    stpio_InitPars[2].DriverPartition = TEST_PARTITION_1;
#endif
    stpio_InitPars[2].BaseAddress     = (U32 *)PIO_2_BASE_ADDRESS;
    stpio_InitPars[2].InterruptNumber = PIO_2_INTERRUPT;
    stpio_InitPars[2].InterruptLevel  = PIO_2_INTERRUPT_LEVEL;
#endif


#if (NUM_PIO > 3)

#ifdef ST_OS21
    stpio_InitPars[3].DriverPartition = system_partition;
#else
    stpio_InitPars[3].DriverPartition = TEST_PARTITION_1;
#endif
    stpio_InitPars[3].BaseAddress     = (U32 *)PIO_3_BASE_ADDRESS;
    stpio_InitPars[3].InterruptNumber = PIO_3_INTERRUPT;
    stpio_InitPars[3].InterruptLevel  = PIO_3_INTERRUPT_LEVEL;
#endif

    /* SPI Initialisations */
    stspi_InitPars.BaseAddress           = (U32*)SSC_BASE_ADDRESS;
    stspi_InitPars.InterruptNumber       = SSC_INTERRUPT;
    stspi_InitPars.InterruptLevel        = SSC_INTERRUPT_LEVEL;
    stspi_InitPars.BusAccessTimeout      = 100000;
    stspi_InitPars.MaxHandles            = 2;
#ifdef ST_OS21
   stspi_InitPars.DriverPartition = system_partition;
#else
    stspi_InitPars.DriverPartition = TEST_PARTITION_1;
#endif
    stspi_InitPars.MasterSlave           = STSPI_MASTER;
    stspi_InitPars.PIOforMTSR.BitMask    = SSC_MTSR_BIT;
    stspi_InitPars.PIOforMRST.BitMask    = SSC_MRST_BIT;
    stspi_InitPars.PIOforSCL.BitMask     = SSC_SCL_BIT;

    strcpy( stspi_InitPars.PIOforMTSR.PortName, PIODevName[SSC_MTSR_DEV] );
    strcpy( stspi_InitPars.PIOforMRST.PortName, PIODevName[SSC_MRST_DEV] );
    strcpy( stspi_InitPars.PIOforSCL.PortName, PIODevName[SSC_SCL_DEV] );
    stspi_InitPars.DefaultParams         = NULL;

#ifdef USE_TESTTOOL
    /* Initialize the testtool */
#ifdef ST_OS21
    sttst_InitPars.CPUPartition_p = system_partition;
#else
    sttst_InitPars.CPUPartition_p = TEST_PARTITION_1;
#endif
    sttst_InitPars.NbMaxOfSymbols = 1000;

#ifndef TESTTOOL_INPUT_FILE_NAME
#define TESTTOOL_INPUT_FILE_NAME ""
#endif

    strcpy( sttst_InitPars.InputFileName, TESTTOOL_INPUT_FILE_NAME );

#endif

    printf ("\nStarting BOOT ...\n");
    retVal = STBOOT_Init( "FLASHTest", &stboot_InitPars );
    if (retVal != ST_NO_ERROR)
    {
        printf( "ERROR: STBOOT_Init() returned %d\n", retVal );

    }
    else
    {
    	 /* Get the clk info */
        (void)ST_GetClockInfo(&ClockInfo);
        ClockSpeed = ClockInfo.CommsBlock;

#ifndef DISABLE_TOOLBOX
    	printf ("\nStarting TBX ...\n");
        retVal = STTBX_Init( "tbx", &sttbx_InitPars );
        if (retVal != ST_NO_ERROR)
        {
            printf( "ERROR: STTBX_Init() returned %d\n", retVal );
        }
        else
#endif
        {

            STFLASH_Print(("\nStarting PIO ...\n"));
            for (i = 0; i < NUM_PIO; i++)
            {
                retVal = STPIO_Init(PIODevName[i], &stpio_InitPars[i]);
                if (retVal != ST_NO_ERROR)
                {
                    STFLASH_Print(( "STPIO_Init() returned %d\n", retVal ));
                    return;
                }
            }

            /* STSPI Init */

            {

            	STFLASH_Print(("\nStarting SPI ...\n"));
            	stspi_InitPars.ClockFrequency  = ClockSpeed; /* 10 MHz */
                retVal = STSPI_Init("spi0", &stspi_InitPars);
                if (retVal != ST_NO_ERROR)
                {
                    STFLASH_Print(( "ERROR: STSPI_Init() returned %d\n", retVal ));

                }
                else
                {
                    DeviceConfig();

                    STFLASH_Print(("============================================================\n"));
                    STFLASH_Print(("                 STFLASH SPI Test Harness                    \n"));
                    STFLASH_Print(("            Driver Revision: %s\n", STFLASH_GetRevision()));
                    STFLASH_Print(("            Test Harness Revision: %s\n", HarnessRev));
                    STFLASH_Print(("            Serial Flash Device Type: %s\n", GetDeviceInfo(DEVICE_TYPE)));
                    STFLASH_Print(("            ST_GetClocksPerSecond: %d\n", ST_GetClocksPerSecond()));
                    STFLASH_Print(( "============================================================\n" ));


#ifndef USE_TESTTOOL

                    STFLASH_Print(( "============================================================\n" ));
                    ErrorCount = FullERW();
#if defined(ST_5188)
                    Error      = SPIConfig();
#endif
#if defined(FLASH_FAST_READ)
                    Error      =  SPIFastReadTest();
#endif                    
                    ErrorCount += Error;
                    STFLASH_Print(( "Overall Test result is: " ));
                    STFLASH_Print(( "%d errors\n", ErrorCount ));
                    STFLASH_Print(( "============================================================\n" ));

#else /* USE_TESTTOOL */
                    printf ("\nStarting TESTTOOL ...\n");
                    retVal = STTST_Init(&sttst_InitPars);
                    if (retVal != ST_NO_ERROR)
                    {
                        printf( "ERROR: STTST_Init() returned %d\n", retVal );
                    }
                    else
                    {
                        FLASH_RegisterTest();
                        /* Start Testtool prompt */
                        STTST_Start();
                        ErrorCount += Error;
                        STTST_Term();
                    }

#endif /* end of USE_TESTTOOL */
                    retVal = STBOOT_Term( "FLASHTest", &stboot_TermPars );

                }
            }
        }
    }
}

/* Configure Config Control register for SPI */
void DeviceConfig(void)
{
#if defined(ST_5188)
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) | 0x00000122 );
#elif defined(ST_5105) || defined(ST_5107)
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F, STSYS_ReadRegDev32LE(CONFIG_CONTROL_F) | 0x00000046 );
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, STSYS_ReadRegDev32LE(CONFIG_CONTROL_H) | 0x00000000 );
#endif
}
#if defined(FLASH_FAST_READ)
void SpiBootConfig(void)
{
/* switch to CPU mode for using spi boot block*/	
	U32 Status = 0;
	Status = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
	Status = (Status & 0xFFFFFFF0) | 0x00000120;
#if defined(ST_5188)
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G,Status);
#endif    
}
#endif

char *GetDeviceInfo(int DeviceNum)
{
    switch (DeviceNum)
    {
        case STFLASH_M25P10:
            return("M25P10");
        case STFLASH_M25P80:
            return("M25P80");
        case STFLASH_M25P16:
            return("M25P16");
        default:
            return("Flash not supported");
    }

}

/****************************************************************************
Name         : FullERW()

Description  : Erases each block in turn and read verifies.
               Writes to each block in turn and read verifies.
               Every location is erased once, written to once
               and read twice.
               This routine is not included in the default test
               as all locations are overwritten.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE      Rogue Flash bank reference
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               ST_ERROR_TIMEOUT             Timeout error during Erase/Write
               STFLASH_ERROR_WRITE          Error during programming sequence
               STFLASH_ERROR_ERASE          Error during erasure sequence
               STFLASH_ERROR_VPP_LOW        Vpp voltage low during either of
                                            Write or Erase sequences

See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/

int FullERW( void )
{
    BOOL            Match;
    U32             i;
    U32             NumberRead = 0;
    U32             NumberWrit = 0;

    int  ErrorCount = 0;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    clock_t         SnapTime;

    static U8       ReadArray[SECTOR_SIZE] = {0};
    static U8       WritArray[SECTOR_SIZE];
    U32             TestBlock, TestSize, TestOffset, ErrorCounter;

    STFLASH_Print(( "\n============================================================\n" ));
    STFLASH_Print(( "Commencing Full Erase/Read/Write Test Function ....\n" ));
    STFLASH_Print(( "============================================================\n" ));

    /* revision string available before Flash Initialized */
    STFLASH_Print(( "Calling STFLASH_GetRevision() pre-Init \n" ));
    RevisionStr = STFLASH_GetRevision();
    STFLASH_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STFLASH_Print(( "------------------------------------------------------------\n" ));

    /* Init Bank 0, Vpp 0 */
    InitParams_s.DeviceType      = DEVICE_TYPE;
    InitParams_s.BaseAddress     = (U32*)SECTOR0_BASE_ADDRESS;
    InitParams_s.VppAddress      = NULL;
    InitParams_s.MinAccessWidth  = 0;
    InitParams_s.MaxAccessWidth  = 0;
    InitParams_s.NumberOfBlocks  = NUMBER_OF_SECTORS;
    InitParams_s.IsSerialFlash   = TRUE;
    InitParams_s.Blocks          = BlockData_s;
    strcpy(InitParams_s.PIOforCS.PortName, PIODevName[PIO_DEV_FOR_CS]);
    InitParams_s.PIOforCS.BitMask  = PIO_FOR_CS;
    strcpy(InitParams_s.SPIDeviceName, "spi0");
#ifdef ST_OS21
    InitParams_s.DriverPartition = system_partition;
#else
    InitParams_s.DriverPartition = TEST_PARTITION_1;
#endif

    STFLASH_Print(( "Calling STFLASH_Init() Bank0 ..........\n" ));
    ReturnError = STFLASH_Init( "sflash", &InitParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Open Bank 0 */
    STFLASH_Print(( "Calling STFLASH_Open() Bank0 ..........\n" ));
    ReturnError = STFLASH_Open( "sflash",
                                &OpenParams_s,
                                &Handle0 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    for (TestBlock = 0; TestBlock < InitParams_s.NumberOfBlocks; TestBlock++) /* sectors */
    {
        TestSize   = BlockData_s[TestBlock].Length;
        TestOffset = 0;
        for ( i = 0; i < TestBlock; i++ )
        {
            TestOffset += BlockData_s[i].Length;
        }

        STFLASH_Print(( "------------------------------------------------------------\n" ));
        STFLASH_Print(( " TestBlock %2u, TestOffset 0x%08x TestSize 0x%08x\n",
                        TestBlock, TestOffset, TestSize ));
        STFLASH_Print(( "------------------------------------------------------------\n" ));

#if 0
        STFLASH_Print(( "Calling STFLASH_BlockUnlock()  ........\n" ));
        ReturnError = STFLASH_BlockUnlock( Handle0, TestOffset );
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        } 
#endif

        /* Erase Bank BankNo Block TestBlock  */
        STFLASH_Print(( "Calling STFLASH_Erase()  ........\n" ));
        SnapTime = STOS_time_now();
        ReturnError = STFLASH_Erase( Handle0,
                                     TestOffset,
                                     TestSize );

        ExTimeReport( SnapTime );
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }
        /* the following reads all of the erased block   */
        STFLASH_Print(( "Calling STFLASH_Read()  ........\n" ));
        SnapTime = STOS_time_now();
        ReturnError = STFLASH_Read( Handle0,
                                    TestOffset,
                                    ReadArray,
                                    TestSize,
                                    &NumberRead );
        ExTimeReport( SnapTime );
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }
        STFLASH_Print(( "NumberActuallyRead reported as %d bytes\n",
                           NumberRead ));

        Match = TRUE;
        if ((ReturnError == ST_NO_ERROR) &&
            (NumberRead  == TestSize))
        {
            ErrorCounter = 0;
            for (i = 0; i < NumberRead; i++)
            {
                if (ReadArray[i] != ERASED_STATE)
                {
                    Match = FALSE;
                    STFLASH_Print(( "Mismatch => Erase/ReadArray[%2d] = %02X / %02X\n",
                                       i, ERASED_STATE, (U32)ReadArray[i] ));
                    ErrorCounter++;
                 }
            }

            if (Match)
            {
                STFLASH_Print(( "Erased data verified with Read of %d bytes\n",
                                   NumberRead ));
            }
            else
            {
                STFLASH_Print(( "=====> VERIFY ERROR: %u errors out of %u bytes\n",
                                   ErrorCounter, NumberRead ));
            }
        }

        /* Program Bank BankNo, Block TestBlock */

        for (i = 0; i < TestSize; i++)
        {
            WritArray[i] = (U8)(TestSize - i);
            ReadArray[i] = (U8)i;
        }
        STFLASH_Print(( "Calling STFLASH_Write()  .......\n"));
        
        SnapTime = STOS_time_now();
        ReturnError = STFLASH_Write( Handle0,
                                     TestOffset,
                                     WritArray,
                                     TestSize,
                                     &NumberWrit );
        ExTimeReport( SnapTime );   
        
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }
        STFLASH_Print(( "NumberActuallyWritten reported as %d bytes\n",
                          NumberWrit ));

        if (NumberWrit > 0)
        {
            STFLASH_Print(( "Calling STFLASH_Read()  ........\n"));
            SnapTime = STOS_time_now();
            ReturnError = STFLASH_Read( Handle0,
                                        TestOffset,
                                        ReadArray,
                                        TestSize,
                                        &NumberRead );
            ExTimeReport( SnapTime );   
            if (ReturnError != ST_NO_ERROR)
            {
                ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
            }
            
            STFLASH_Print(( "NumberActuallyRead reported as %d bytes\n",
                              NumberRead ));
            Match = TRUE;
            if ((ReturnError == ST_NO_ERROR) &&
                (NumberRead  == TestSize))
            {
                ErrorCounter = 0;
                for (i = 0; i < NumberRead; i++)
                {
                    if (WritArray[i] != ReadArray[i])
                    {
                        Match = FALSE;
                        STFLASH_Print(( "Mismatch => Write/ReadArray[%2d] = %02X / %02X\n",
                                          i, (U32)WritArray[i], (U32)ReadArray[i] ));
                        ErrorCounter++;
                     }
                }

               if (Match)
               {
                    STFLASH_Print(( "Write data verified with Read of %d bytes\n",
                                    NumberRead ));
                }
                else
                {
                    STFLASH_Print(( "=====> VERIFY ERROR: %u errors out of %u bytes\n",
                                    ErrorCounter, NumberRead ));
                }
                
            }
        }

        STFLASH_Print(( "------------------------------------------------------------\n" ));

    }

    /* Close */

    STFLASH_Print(( "Calling STFLASH_Close()  ..........\n" ));
    ReturnError = STFLASH_Close( Handle0 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    /* Term */
    TermParams_s.ForceTerminate = TRUE;
    STFLASH_Print(( "Calling STFLASH_Term()  .........\n" ));
    ReturnError = STFLASH_Term( "sflash", &TermParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    return ErrorCount;
}

#if defined(ST_5188)
/****************************************************************************
Name         : SPIConfig()

Description  :

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE      Rogue Flash bank reference
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters

See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/

int SPIConfig( void )
{

    int  ErrorCount = 0;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;

    STFLASH_Print(( "\n============================================================\n" ));
    STFLASH_Print(( "Program SPIBoot block ....\n" ));
    STFLASH_Print(( "============================================================\n" ));

    /* revision string available before Flash Initialized */
    STFLASH_Print(( "Calling STFLASH_GetRevision() pre-Init \n" ));
    RevisionStr = STFLASH_GetRevision();
    STFLASH_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STFLASH_Print(( "------------------------------------------------------------\n" ));

    /* Init Bank 0, Vpp 0 */
    InitParams_s.DeviceType      = DEVICE_TYPE;
    InitParams_s.BaseAddress     = (U32*)SECTOR0_BASE_ADDRESS;
    InitParams_s.VppAddress      = NULL;
    InitParams_s.MinAccessWidth  = 0;
    InitParams_s.MaxAccessWidth  = 0;
    InitParams_s.NumberOfBlocks  = NUMBER_OF_SECTORS;
    InitParams_s.IsSerialFlash   = TRUE;
    InitParams_s.Blocks          = BlockData_s;
    strcpy(InitParams_s.PIOforCS.PortName, PIODevName[PIO_DEV_FOR_CS]);
    InitParams_s.PIOforCS.BitMask  = PIO_FOR_CS;
    strcpy(InitParams_s.SPIDeviceName, "spi0");
#ifdef ST_OS21
    InitParams_s.DriverPartition = system_partition;
#else
    InitParams_s.DriverPartition = TEST_PARTITION_1;
#endif

    STFLASH_Print(( "Calling STFLASH_Init()  ..........\n" ));
    ReturnError = STFLASH_Init( "sflash", &InitParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Open Bank 0 */
    STFLASH_Print(( "Calling STFLASH_Open()  ..........\n" ));
    ReturnError = STFLASH_Open( "sflash",
                                &OpenParams_s,
                                &Handle0 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Set Clock Divisor */
    STFLASH_Print(( "Calling STFLASH_SetSPIClkDiv()  ..........\n" ));
    ReturnError = STFLASH_SetSPIClkDiv( Handle0, 2 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    STFLASH_SPIConfig_s.PartNum      = 0001; /* ST */
    STFLASH_SPIConfig_s.CSHighTime   = 1000;
    STFLASH_SPIConfig_s.CSHoldTime   = 1;    /* clk cycle */
    STFLASH_SPIConfig_s.DataHoldTime = 1;    /* clk cycle */

    /* Set Config */
    STFLASH_Print(( "Calling STFLASH_SetSPIConfig()  ..........\n" ));
    ReturnError = STFLASH_SetSPIConfig( Handle0, &STFLASH_SPIConfig_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Close */
    STFLASH_Print(( "Calling STFLASH_Close()  ..........\n" ));
    ReturnError = STFLASH_Close( Handle0 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Term */
    TermParams_s.ForceTerminate = TRUE;
    STFLASH_Print(( "Calling STFLASH_Term()  .........\n" ));
    ReturnError = STFLASH_Term( "sflash", &TermParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    return ErrorCount;


}
#endif

#if defined(FLASH_FAST_READ)
/****************************************************************************
Name         : SPIFastReadTest()

Description  :

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE      Rogue Flash bank reference
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters

See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/
int SPIFastReadTest( void )
{
    BOOL            Match;
    U32             i;
    U32             NumberRead = 0;
    U32             NumberWrit = 0;

    int             ErrorCount = 0;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    clock_t         SnapTime;

    static U8       ReadArray[SECTOR_SIZE] = {0};
    static U8       WritArray[SECTOR_SIZE];
    U32             TestBlock, TestSize, TestOffset, ErrorCounter;

    STFLASH_Print(( "\n============================================================\n" ));
    STFLASH_Print(( "Commencing FAST READ Test Function ....\n" ));
    STFLASH_Print(( "============================================================\n" ));

    /* revision string available before Flash Initialized */
    STFLASH_Print(( "Calling STFLASH_GetRevision() pre-Init \n" ));
    RevisionStr = STFLASH_GetRevision();
    STFLASH_Print(( "Software Revision reported as %s\n", RevisionStr ));
    STFLASH_Print(( "------------------------------------------------------------\n" ));

    /* Init Bank 0, Vpp 0 */
    InitParams_s.DeviceType      = DEVICE_TYPE;
    InitParams_s.BaseAddress     = (U32*)SECTOR0_BASE_ADDRESS;
    InitParams_s.VppAddress      = NULL;
    InitParams_s.MinAccessWidth  = 0;
    InitParams_s.MaxAccessWidth  = 0;
    InitParams_s.NumberOfBlocks  = NUMBER_OF_SECTORS;
    InitParams_s.IsSerialFlash   = TRUE;
    InitParams_s.Blocks          = BlockData_s;
    strcpy(InitParams_s.PIOforCS.PortName, PIODevName[PIO_DEV_FOR_CS]);
    InitParams_s.PIOforCS.BitMask  = PIO_FOR_CS;
    strcpy(InitParams_s.SPIDeviceName, "spi0");
#ifdef ST_OS21
    InitParams_s.DriverPartition = system_partition;
#else
    InitParams_s.DriverPartition = TEST_PARTITION_1;
#endif

    STFLASH_Print(( "Calling STFLASH_Init() Bank0 ..........\n" ));
    ReturnError = STFLASH_Init( "sflash", &InitParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Open Bank 0 */
    STFLASH_Print(( "Calling STFLASH_Open() Bank0 ..........\n" ));
    ReturnError = STFLASH_Open( "sflash",
                                &OpenParams_s,
                                &Handle0 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    SpiBootConfig(); /* Switch to CPU mode for writing spi boot registers*/
    
    /* Spi boot block register effective only in CPU mode*/
    ReturnError = STFLASH_SetSPIClkDiv( Handle0, 2);
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    STFLASH_SPIConfig_s.PartNum      = 0001; /* ST */
    STFLASH_SPIConfig_s.CSHighTime   = 1000;
    STFLASH_SPIConfig_s.CSHoldTime   = 1;    /* clk cycle */
    STFLASH_SPIConfig_s.DataHoldTime = 1;    /* clk cycle */
    STFLASH_SPIMode_s.IsFastRead     = TRUE;
    STFLASH_SPIMode_s.IsContigMode   = TRUE;

    STFLASH_Print(( "Calling STFLASH_SetSPIConfig()  ..........\n" ));
    ReturnError = STFLASH_SetSPIConfig( Handle0, &STFLASH_SPIConfig_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    STFLASH_Print(( "Calling STFLASH_SetSPIModeSelect()  ..........\n" ));
    ReturnError = STFLASH_SetSPIModeSelect( Handle0, &STFLASH_SPIMode_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    for (TestBlock = 0; TestBlock < InitParams_s.NumberOfBlocks; TestBlock++) /* sectors */
    {
        TestSize   = BlockData_s[TestBlock].Length;
        TestOffset = 0;
        for ( i = 0; i < TestBlock; i++ )
        {
            TestOffset += BlockData_s[i].Length;
        }

        STFLASH_Print(( "------------------------------------------------------------\n" ));
        STFLASH_Print(( " TestBlock %2u, TestOffset 0x%08x TestSize 0x%08x\n",
                        TestBlock, TestOffset, TestSize ));
        STFLASH_Print(( "------------------------------------------------------------\n" ));

        /* Erase Bank BankNo Block TestBlock  */
        STFLASH_Print(( "Calling STFLASH_Erase()  ........\n" ));
        
        /*switch to comms mode as spi boot provides read only access*/
        DeviceConfig(); 
        
        SnapTime = STOS_time_now();
        ReturnError = STFLASH_Erase( Handle0,
                                     TestOffset,
                                     TestSize );

        ExTimeReport( SnapTime );
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }
      
        /* the following reads all of the erased block   */
        SpiBootConfig(); /* Switch to CPU mode for reading*/
        
        STFLASH_Print(( "FAST READ opeartion()  ..........\n" ));
        SnapTime = STOS_time_now();
        ReturnError = STFLASH_Read( Handle0,
                                    TestOffset,
                                    ReadArray,
                                    TestSize,
                                    &NumberRead );

        ExTimeReport( SnapTime );
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }
        STFLASH_Print(( "NumberActuallyRead reported as %d bytes\n",
                           NumberRead ));

        Match = TRUE;
        if ((ReturnError == ST_NO_ERROR) &&
            (NumberRead  == TestSize - 1))
        {
            ErrorCounter = 0;
            for (i = 0; i < NumberRead; i++)
            {
                if (ReadArray[i] != ERASED_STATE)
                {
                    Match = FALSE;
                    STFLASH_Print(( "Mismatch => Erase/ReadArray[%2d] = %02X / %02X\n",
                                       i, ERASED_STATE, (U32)ReadArray[i] ));
                    ErrorCounter++;
                 }
            }

            if (Match)
            {
                STFLASH_Print(( "Erased data verified with Read of %d bytes\n",
                                   NumberRead ));
            }
            else
            {
                STFLASH_Print(( "=====> VERIFY ERROR: %u errors out of %u bytes\n",
                                   ErrorCounter, NumberRead ));
            }
        }
        /* Program Bank BankNo, Block TestBlock */
        for (i = 0; i < TestSize; i++)
        {
            WritArray[i] = (U8)(TestSize - i);
            ReadArray[i] = (U8)i;
        }
        DeviceConfig(); /* Change back to Comms mode */
        STFLASH_Print(( "Calling STFLASH_Write()  .......\n"));
        
        SnapTime = STOS_time_now();
        ReturnError = STFLASH_Write( Handle0,
                                     TestOffset,
                                     WritArray,
                                     TestSize,
                                     &NumberWrit );
        ExTimeReport( SnapTime );   
        
        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }
        STFLASH_Print(( "NumberActuallyWritten reported as %d bytes\n",
                          NumberWrit ));

        if (NumberWrit > 0)
        {
        	SpiBootConfig(); /* Switch to CPU mode for reading*/
	        STFLASH_Print(( "Calling STFLASH_SetSPIModeSelect()  ..........\n" ));
		    ReturnError = STFLASH_SetSPIModeSelect( Handle0, &STFLASH_SPIMode_s );
		    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

        
            STFLASH_Print(( "Calling STFLASH_Read()  ........\n"));
            SnapTime = STOS_time_now();
            ReturnError = STFLASH_Read( Handle0,
                                        TestOffset,
                                        ReadArray,
                                        TestSize,
                                        &NumberRead );
            NumberRead = TestSize;                                  
            ExTimeReport( SnapTime );   
            if (ReturnError != ST_NO_ERROR)
            {
                ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
            }
            
            STFLASH_Print(( "NumberActuallyRead reported as %d bytes\n",
                              NumberRead ));
            Match = TRUE;
            if ((ReturnError == ST_NO_ERROR) &&
                (NumberRead  == TestSize - 1))
            {
                ErrorCounter = 0;
                for (i = 0; i < NumberRead; i++)
                {
                    if (WritArray[i] != ReadArray[i])
                    {
                        Match = FALSE;
                        STFLASH_Print(( "Mismatch => Write/ReadArray[%2d] = %02X / %02X\n",
                                          i, (U32)WritArray[i], (U32)ReadArray[i] ));
                        ErrorCounter++;
                     }
                }

               if (Match)
               {
                    STFLASH_Print(( "Write data verified with Read of %d bytes\n",
                                    NumberRead ));
                }
                else
                {
                    STFLASH_Print(( "=====> VERIFY ERROR: %u errors out of %u bytes\n",
                                    ErrorCounter, NumberRead ));
                }
                
            }
        }
        STFLASH_Print(( "------------------------------------------------------------\n" ));

    }

    /* Close */
    STFLASH_Print(( "Calling STFLASH_Close()  ..........\n" ));
    ReturnError = STFLASH_Close( Handle0 );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    /* Term */
    TermParams_s.ForceTerminate = TRUE;
    STFLASH_Print(( "Calling STFLASH_Term()  .........\n" ));
    ReturnError = STFLASH_Term( "sflash", &TermParams_s );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    
    return ErrorCount;
}
#endif
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

void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr )
{
    ST_ErrorCode_t  Temp = ErrorGiven;

    switch (Temp)
    {
        case ST_NO_ERROR:
            STFLASH_Print(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            STFLASH_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            STFLASH_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            STFLASH_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            STFLASH_Print(( "ST_ERROR_INVALID_HANDLE - Rogue Handle value or Closed\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            STFLASH_Print(( "ST_ERROR_OPEN_HANDLE - Term called with Open Handle\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            STFLASH_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            STFLASH_Print(( "ST_ERROR_NO_FREE_HANDLES - DeviceName already Open\n" ));
            break;

        case ST_ERROR_TIMEOUT:
            STFLASH_Print(( "ST_ERROR_TIMEOUT - Timeout limit reached\n" ));
            break;

        case STFLASH_ERROR_WRITE:
            STFLASH_Print(( "STFLASH_ERROR_WRITE - Error during Flash Write\n" ));
            break;

        case STFLASH_ERROR_ERASE:
            STFLASH_Print(( "STFLASH_ERROR_ERASE - Error during Flash Erase\n" ));
            break;

        case STFLASH_ERROR_VPP_LOW:
            STFLASH_Print(( "STFLASH_ERROR_VPP_LOW - Flash Vpp Voltage too low\n" ));
            break;

        default:
            STFLASH_Print(( "*** Unrecognised return code ***\n" ));
        }

        if (ErrorGiven != ExpectedErr)
        {
            (*ErrorCount)++;
        }

    STFLASH_Print(( "----------------------------------------------------------\n" ));
}



/****************************************************************************
Name         : ExTimeReport()

Description  : Outputs the Execution Time

Parameters   : clock_t SnapTime taken before driver function entry

Return Value : none

See Also     :
 ****************************************************************************/

void ExTimeReport( clock_t SnapTime )
{
    clock_t CurrTime, DiffTime;


    CurrTime = STOS_time_now();
    DiffTime = STOS_time_minus( CurrTime, SnapTime );

    STFLASH_Print(( "Execution time taken was %d slow ticks\n", (U32)DiffTime ));
}


/****************************************************************************
Name         : ParamsReport()

Description  : Outputs the GetParams_s values as text

Parameters   : STFLASH_GetParams_t pointer to returned Init data

Return Value : none

See Also     : STFLASH_Params_t
 ****************************************************************************/

void ParamsReport( STFLASH_Params_t *RetParams )
{
    U32 i;
    STFLASH_Block_t   *RetBlock_p;

    switch (RetParams->InitParams.DeviceType)
    {
        case STFLASH_M25P10:
            STFLASH_Print(( "DeviceType reported as STFLASH_M25P10\n" ));
            break;
        case STFLASH_M25P80:
            STFLASH_Print(( "DeviceType reported as STFLASH_M25P80\n" ));
            break;
        case STFLASH_M25P16:
            STFLASH_Print(( "DeviceType reported as STFLASH_M25P16\n" ));
            break;
        default:
            break;
    }

    STFLASH_Print(( "MinAccessWidth reported as %d\n",
                   RetParams->InitParams.MinAccessWidth ));

    STFLASH_Print(( "MaxAccessWidth reported as %d\n",
                   RetParams->InitParams.MaxAccessWidth ));

    STFLASH_Print(( "Number of Blocks reported as %d\n",
                   RetParams->InitParams.NumberOfBlocks ));

    RetBlock_p = RetParams->InitParams.Blocks;

    for (i = 0; i < RetParams->InitParams.NumberOfBlocks; i++)
    {
        switch (RetBlock_p[i].Type)
        {
            case STFLASH_EMPTY_BLOCK:
                STFLASH_Print(( "Empty Block \t\t" ));
                break;

            case STFLASH_BOOT_BLOCK:
                STFLASH_Print(( "Boot Block \t\t" ));
                break;

            case STFLASH_PARAMETER_BLOCK:
                STFLASH_Print(( "Parameter Block \t" ));
                break;

            case STFLASH_MAIN_BLOCK:
                STFLASH_Print(( "Main Block \t\t" ));
                break;

            default:
                STFLASH_Print(( "Unknown Block \t\t" ));
        }
        STFLASH_Print(( "0x%08X bytes long\n", RetBlock_p[i].Length ));

    }

    STFLASH_Print(( "--------------------------------------------------------------\n" ));
}
#ifdef USE_TESTTOOL
/*-------------------------------------------------------------------------
 * Function : SPI_FullERW
 *
 * Input    :
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static BOOL SPI_FullERW( STTST_Parse_t *pars_p, char *Result )
{
    int error;

    error = FullERW();

    return(error);
}
#if defined(ST_5188)
/*-------------------------------------------------------------------------
 * Function : SPI_SetConfig
 *
 * Input    :
 * Output   :
 * Return   : None
 * ----------------------------------------------------------------------*/
static BOOL SPI_SetConfig( STTST_Parse_t *pars_p, char *Result )
{
    int error;

    error = SPIConfig();

    return(error);
}
static BOOL SPI_SetFastRead( STTST_Parse_t *pars_p, char *Result )
{
	int error;

    error = SPIFastReadTest();

    return(error);
	
}
#endif
/*-------------------------------------------------------------------------
 * Function : SPI_RegisterCommands
 *            Definition of the macros
 *            (commands and constants to be used with Testtool)
 * Input    :
 * Output   :
 * Return   : TRUE if error, FALSE if success
 * ----------------------------------------------------------------------*/
BOOL FLASH_RegisterTest( void )
{
    BOOL RetErr = FALSE;

    /* API Commands */
    RetErr |= STTST_RegisterCommand("FULLERW", SPI_FullERW, "Erase & Write operations are verified");
#if defined(ST_5188)
    RetErr |= STTST_RegisterCommand("SPIConfig", SPI_SetConfig, "SPIBoot config");
    RetErr |= STTST_RegisterCommand("SPIFastRead", SPI_SetFastRead, "SPIBoot Fast Read config");
#endif
    STFLASH_Print(("FLASH_RegisterTest() : macros registrations "));
    return( RetErr );
}
#endif


