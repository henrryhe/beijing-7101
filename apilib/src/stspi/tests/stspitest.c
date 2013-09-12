/****************************************************************************

File Name   : stspitest.c

Description : Serial Peripheral Interface API Test Routines

Copyright (C) 2005, ST Microelectronics

History     : First Release

References  :

$ClearCase (VOB: stspi)

****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#ifndef ST_OS21
#include <kernel.h>
#include "debug.h"
#endif

#include "stlite.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"
#include "stpio.h"
#include "sttbx.h"
#include "stboot.h"
#include "stcommon.h"
#include "stsys.h"
/*#include "cache.h"*/
#include "stspitest.h"
#include "stspi.h"
#ifdef ARCHITECTURE_ST200
#include <machine/mmu.h>
#endif

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
#ifndef ST_5188
#define STSPI_DEV_M25P10
#else
#define STSPI_DEV_M25P80
#endif

#if defined(ST_5100) || defined(ST_5301)

#define NUMBER_PIO             5

#ifndef PIO_4_BASE_ADDRESS
#define PIO_4_BASE_ADDRESS     ST5100_PIO4_BASE_ADDRESS
#endif

#ifndef PIO_4_INTERRUPT
#define PIO_4_INTERRUPT        ST5100_PIO4_INTERRUPT
#endif

#ifndef SSC_2_INTERRUPT
#define SSC_2_INTERRUPT        ST5100_SSC2_INTERRUPT
#endif

#ifndef SSC_2_INTERRUPT_LEVEL
#define SSC_2_INTERRUPT_LEVEL  5
#endif

#define SSC_BASE_ADDRESS       SSC_2_BASE_ADDRESS
#define SSC_INTERRUPT          SSC_2_INTERRUPT
#define SSC_INTERRUPT_LEVEL    SSC_2_INTERRUPT_LEVEL

#define SSC_MTSR_BIT           PIO_BIT_3
#define SSC_MTSR_DEV           PIO_DEVICE_4

#define SSC_MRST_BIT           PIO_BIT_1
#define SSC_MRST_DEV           PIO_DEVICE_4

#define SSC_SCL_BIT            PIO_BIT_2
#define SSC_SCL_DEV            PIO_DEVICE_4

#define SSC_CS_BIT             PIO_BIT_5
#define SSC_CS_DEV             PIO_DEVICE_3

#define CONFIG_CONTROL_G       0x20D00010
#define CONFIG_CONTROL_H       0x20D00014

#elif defined(ST_5105) || defined(ST_5107)

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

#elif defined(ST_8010)

#define NUMBER_PIO             7

#define SSC_BASE_ADDRESS       SSC_1_BASE_ADDRESS
#define SSC_INTERRUPT          SSC_1_INTERRUPT
#define SSC_INTERRUPT_LEVEL    SSC_1_INTERRUPT_LEVEL

/* On 8010 SPI EEPROM is connected to SSC1 whose output pads are not
   through PIO pins.So no need to give PIO device name and pin mask in
   SPI_InitParams.
   These #define are added simply for compilation purpose,
   driver will not be using them */

#define SSC_MTSR_BIT           0
#define SSC_MTSR_DEV           0

#define SSC_MRST_BIT           0
#define SSC_MRST_DEV           0

#define SSC_SCL_BIT            0
#define SSC_SCL_DEV            0

#define SSC_CS_BIT             0
#define SSC_CS_DEV             0

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

#elif defined(ST_5525)

#define NUMBER_PIO             9

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

#else

#define NUMBER_PIO             6

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

#endif

/* Private Variables ------------------------------------------------------ */

const U8 HarnessRev[] = "1.0.6";

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

#elif defined(STSPI_DEV_M25P16)

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

#define PAGE_SIZE               256            /* bytes */
#define PAGE_MASK               0x000000FF
#define SECTOR_SIZE             0x10000
#define MEMORY_SIZE             25 * 256 * 256 /* Actual total memory is (32 * 256 * 256): 32 sectors of 256
                                                  pages each of 256 bytes each, we are testing on for 25 sectors */
#define WRONG_HANDLE            0x01234567     /* invalid handle value */
#define ERASED_STATE            0xFF
#define REACHED_TIMEOUT         100000

#define NUMBER_OF_SECTORS       32

#endif

static U32 ClockSpeed;

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
#elif defined(STSPI_DEV_M25P16)
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
                      SECTOR15_BASE_ADDRESS,
                      SECTOR16_BASE_ADDRESS,
                      SECTOR17_BASE_ADDRESS,
                      SECTOR18_BASE_ADDRESS,
                      SECTOR19_BASE_ADDRESS,
                      SECTOR20_BASE_ADDRESS,
                      SECTOR21_BASE_ADDRESS,
                      SECTOR22_BASE_ADDRESS,
                      SECTOR23_BASE_ADDRESS,
                      SECTOR24_BASE_ADDRESS,
                      SECTOR25_BASE_ADDRESS,
                      SECTOR26_BASE_ADDRESS,
                      SECTOR27_BASE_ADDRESS,
                      SECTOR28_BASE_ADDRESS,
                      SECTOR29_BASE_ADDRESS,
                      SECTOR30_BASE_ADDRESS,
                      SECTOR31_BASE_ADDRESS
                   };
#endif

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
STPIO_Handle_t          PIOHandle;

#if defined(ST_7100) || defined(ST_7109)
#define PIO_BIT_MASK 0x80
#elif defined(ST_5188)
#define PIO_BIT_MASK 0x10
#elif defined(ST_5525)
#define PIO_BIT_MASK 0x08
#endif

#endif

#if defined(ST_5100) || defined(ST_5301)
U32 *SlaveSelect = (U32*)0x41300000;
#elif defined(ST_5105) || defined(ST_5107)
U32 *SlaveSelect = (U32*)0x45180000;
#elif defined(ST_8010)
U32 *SlaveSelect = (U32*)0x47000028;
#endif




/* Declarations for memory partitions */

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            100000
#define NCACHE_PARTITION_SIZE            0x10000

#ifndef ST_OS21
/* Declarations for memory partitions */
unsigned char                   internal_block [INTERNAL_PARTITION_SIZE];
#pragma ST_section              (internal_block, "internal_section")
ST_Partition_t                  TheInternalPartition;
ST_Partition_t                  *InternalPartition = &TheInternalPartition;

unsigned char                   system_block [SYSTEM_PARTITION_SIZE];
#pragma ST_section              (system_block,   "system_section")
ST_Partition_t                  TheSystemPartition;
ST_Partition_t                  *SystemPartition = &TheSystemPartition;


unsigned char                   ncache_block [NCACHE_PARTITION_SIZE];
#pragma ST_section              (ncache_block,   "ncache_section")
ST_Partition_t                  TheNcachePartition;
ST_Partition_t                  *NcachePartition = &TheNcachePartition;

ST_Partition_t *DriverPartition = &TheSystemPartition;

/* The 3 declarations below MUST be kept so os20 doesn't fall over!! */
ST_Partition_t *internal_partition = &TheInternalPartition;
ST_Partition_t *system_partition = &TheSystemPartition;
ST_Partition_t *ncache_partition = &TheNcachePartition;

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")

#else

static unsigned char    external_block[SYSTEM_PARTITION_SIZE];
partition_t             *system_partition;

#endif /* ST_OS21 */

/* Memory partitions */
#define TEST_PARTITION_1        system_partition
#define TEST_PARTITION_2        ncache_partition

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

/*****************************************************************************
Private Function prototypes
*****************************************************************************/
/* Local function prototypes */

ST_ErrorCode_t MemoryWrite(STSPI_Handle_t Handle, U32 Address, U16 *Buffer,
                           U32 NumToWrite,U32 Timeout, U32 *NumWritten);

ST_ErrorCode_t MemoryErase(STSPI_Handle_t Handle,U32 Address,U32 Timeout);
ST_ErrorCode_t MemoryRead(STSPI_Handle_t Handle, U32 Address, U16 *Buffer,
                          U32 NumToRead, U32 Timeout, U32 *NumRead);

ST_ErrorCode_t ReadStatus(STSPI_Handle_t Handle, U32 Address);
ST_ErrorCode_t WriteStatus(STSPI_Handle_t Handle, U32 Address);



#ifdef STSPI_MASTER_SLAVE
#if defined(ST_5100) || defined(ST_5301)
int MasterSlave(void);
#endif
#else
#ifdef ARCHITECTURE_ST20
static void StackUsage(void);
#endif
#ifdef FULL_MEMORY_ACCESS
static int FullRWTest( void );
#endif
static int NormalTest( void );
static int LeakTest(void);
static int APITest( void );
#endif

void ExTimeReport( clock_t TimeSnap );

void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );

void os20_main( void *ptr);



/* Local Functions */

ST_ErrorCode_t ReadStatus(STSPI_Handle_t Handle, U32 Address)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;
    U16 WriteBuf[1];
    U16 ReadBuf[1] = { 0xFF };
    U32 ActLen;
    U32 Timeout = 0;

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
    *SlaveSelect &= ~0x400;
    *SlaveSelect |= 0x400;
#else
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]    = (U8)FLASH_READ_STATUS;

    Error = STSPI_Write( Handle, WriteBuf, 1, 100, &ActLen );
    if (Error != ST_NO_ERROR)
    {
        STSPI_Print(("STSPI_Write Failed :\n"));
    }
    else if (ActLen != 1)
    {
        STSPI_Print(("ERROR: actLen for write = %d\n", ActLen));
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
                STSPI_Print(("STSPI_Read Failed :\n"));
                break;
            }
            else if (ActLen != 1)
            {
                STSPI_Print(("ERROR: %d bytes read, 1 expected\n", ActLen));
                break;
            }

            if ( Timeout++ == REACHED_TIMEOUT )
            {
                Error = ST_ERROR_TIMEOUT;
                break;
            }

        }
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
        *SlaveSelect &= ~0x400;
#else
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
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
    *SlaveSelect &= ~0x400;
    *SlaveSelect |= 0x400;
#else
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]  = (U8)FLASH_WRITE_ENABLE;

    Error = STSPI_Write( Handle, WriteBuf, 1 , 100, &ActLen );
    if (Error != ST_NO_ERROR)
    {
        STSPI_Print(("STSPI_Write Failed :\n"));
    }
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
    *SlaveSelect &= ~0x400;
    *SlaveSelect |= 0x400;
#else
    *SlaveSelect = CHIP_SELECT_HIGH;
    *SlaveSelect = CHIP_SELECT_LOW;
#endif

    WriteBuf[0]    = (U8)FLASH_WRITE_STATUS;
    WriteBuf[1]    = (U8)0x00; /* BR1 =0 BP0 =0 to unlock */

    Error = STSPI_Write( Handle, WriteBuf, 2 , 100, &ActLen );
    if (Error != ST_NO_ERROR)
    {
        STSPI_Print(("STSPI_Write Failed :\n"));
    }
    else if (ActLen != 2)
    {
        STSPI_Print(("ERROR: actLen for write = %d\n", ActLen));
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
        STSPI_Print(("Error in Reading Status Reg\n"));
    }
    else
    {
        /* Drive CS signal high to complete the execution of prev instruction*/
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
        STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
        *SlaveSelect &= ~0x400;
        *SlaveSelect |= 0x400;
#else
        *SlaveSelect = CHIP_SELECT_HIGH;
        *SlaveSelect = CHIP_SELECT_LOW;
#endif

        Buffer[0] = (U8)FLASH_WRITE_ENABLE;

        /* Enter Write Enable cmd */
        Error = STSPI_Write( Handle, Buffer, 1, Timeout, &NumWritten );
        if (Error != ST_NO_ERROR)
        {
            STSPI_Print((" STSPI_Write Failed :\n"));
            return(Error);
        }
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
        STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
        *SlaveSelect &= ~0x400;
        *SlaveSelect |= 0x400;
#else
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
            STSPI_Print((" STSPI_Write Failed :\n"));
        }
        else if (NumWritten != 4)
        {
            STSPI_Print(("Had to write 4 bytes but actually wrote %d\n",NumWritten));
        }
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
        STPIO_Set(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
        *SlaveSelect &= ~0x400;
#else
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

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
    STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
    *SlaveSelect &= ~0x400;
    *SlaveSelect |= 0x400;
#else
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
        STSPI_Print((" STSPI_Write Failed :\n"));

    }
    else if (NumWritten != 4)
    {
        STSPI_Print(("Had to write 4 bytes but actually wrote %d\n",NumWritten));
    }

    if (Error == ST_NO_ERROR)
    {
        Error = STSPI_Read( Handle, ReadBuf, NumToRead, Timeout, NumRead );
        if (Error != ST_NO_ERROR)
        {
            STSPI_Print(("STSPI_Read Failed with %d\n",Error));

        }
        else if (*NumRead != NumToRead)
        {
            STSPI_Print(("Had to read %d bytes but actually read %d\n",NumToRead,*NumRead));
        }

    }
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
    STPIO_Set(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
    *SlaveSelect &= ~0x400;
#else
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
            STSPI_Print(("Error in Reading Status Reg\n"));
            break;
        }
        else
        {
            /* CS should be LOW while giving cmd */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
            STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
            *SlaveSelect |= 0x400;
#else
            *SlaveSelect = CHIP_SELECT_LOW;
#endif
            Buffer[0] = (U8)FLASH_WRITE_ENABLE;

            Error = STSPI_Write( Handle, Buffer, 1, Timeout, NumWritten );
            if (Error != ST_NO_ERROR)
            {
                STSPI_Print((" STSPI_Write Failed :\n"));
                return(Error);
            }

#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
            /* Drive CS signal high to complete the execution of prev instruction */
            STPIO_Set(PIOHandle,PIO_BIT_MASK);
            /* Drive CS low to start the Write */
            STPIO_Clear(PIOHandle,PIO_BIT_MASK);
#elif defined(ST_8010)
            *SlaveSelect &= ~0x400;
            *SlaveSelect |= 0x400;
#else
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
                STSPI_Print((" STSPI_Write Failed :\n"));
                return(Error);
            }
            else if (*NumWritten != 4)
            {
                STSPI_Print(("Had to write 4 bytes but actually wrote %d\n",*NumWritten));
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
                STSPI_Print((" STSPI_Write Failed :\n"));
                return(Error);
            }
            else if (*NumWritten != BytesToWrite)
            {
                STSPI_Print(("Had to write %d bytes but actually wrote %d\n",NumToWrite,*NumWritten));
                return(Error);
            }
            /* Drive CS HIGH to complete PP instruction */
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
            STPIO_Set(PIOHandle,PIO_BIT_MASK);
            task_delay(100);
#elif defined(ST_8010)
            *SlaveSelect &= ~0x400;
#else
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

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : none

Return Value : int

See Also     : OS20_main
 ****************************************************************************/
int main(int argc, char *argv[])
{

    os20_main(NULL);
    fflush (stdout);
    return(0);
}

void os20_main( void *ptr)
{
    ST_ErrorCode_t          Code;
    int                     Error, ErrorCount = 0,i;
#ifndef STSPI_NO_TBX
    STTBX_InitParams_t      sttbx_InitPars;
#endif
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    STPIO_InitParams_t      PioInitParams[NUMBER_PIO];
#if defined(ST_7100) || defined(ST_7109) || defined(ST_5188) || defined(ST_5525)
    STPIO_OpenParams_t      PIOOpenParams;
#endif
    ST_ClockInfo_t          ClockInfo;

/* Begin the extremely long cachemap definition */
#if defined(DISABLE_DCACHE)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { NULL, NULL }
    };
#elif defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        /* assumed ok */
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS },
        { NULL, NULL }
    };
#elif defined(ST_5514) && defined(UNIFIED_MEMORY)
    /* Constants not defined, fallbacks from here on down */
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0xC0380000, (U32 *)0xc03fffff },
        { NULL, NULL}
    };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
        { NULL, NULL }
    };
#elif defined(ST_5512)
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40080000, (U32 *)0x5FFFFFFF }, /* ok */
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
        { (U32 *)0x40080000, (U32 *)0x4FFFFFFF }, /* ok */
        { NULL, NULL }
    };
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition   =  partition_create_heap((U8*)external_block, sizeof(external_block));
    /*ncache_partition   =  partition_create_heap((U8*)ST40_NOCACHE_NOTRANSLATE(ncache_block),
                                                                         sizeof(ncache_block));*/
#else
    partition_init_heap (&TheInternalPartition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&TheSystemPartition,   system_block,
                         sizeof(system_block));
    partition_init_heap (&TheNcachePartition,   ncache_block,
                         sizeof(ncache_block));
    internal_block_noinit[0] = 0;
    system_block_noinit[0]   = 0;
    data_section[0]          = 0;

#endif

#if defined(ST_5301)
    bsp_mmu_memory_map((void *)(0x41000000),
                                0x02000000,
                                PROT_USER_EXECUTE|
                                PROT_USER_WRITE|
                                PROT_USER_READ|
                                PROT_SUPERVISOR_EXECUTE|
                                PROT_SUPERVISOR_WRITE|
                                PROT_SUPERVISOR_READ,
                                MAP_SPARE_RESOURCES,
                                (void *)(0x41000000));
#endif

    /* Initialize stboot */
#if defined(DISABLE_ICACHE)
    stboot_InitPars.ICacheEnabled             = FALSE;
#else
    stboot_InitPars.ICacheEnabled             = TRUE;
#endif
    stboot_InitPars.DCacheMap                 = DCacheMap;
    stboot_InitPars.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitPars.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
    stboot_InitPars.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    stboot_InitPars.SDRAMFrequency            = SDRAM_FREQUENCY;

    printf ("Entering STBOOT_Init\n");
    Code = STBOOT_Init( "SPITest", &stboot_InitPars );
    if (Code!= ST_NO_ERROR)
    {
        printf ("ERROR: STBOOT_Init returned code %d\n", Code);
    }
    else
    {
        printf( "ERROR: ST_NO_ERROR\n");
    }
#ifndef STSPI_NO_TBX
    /* Initialize the toolbox */
    sttbx_InitPars.CPUPartition_p      = TEST_PARTITION_1;
    sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;
    printf ("\nStarting TBX ...\n");
    Code = STTBX_Init( "tbx", &sttbx_InitPars );
    if (Code != ST_NO_ERROR)
    {
        printf( "ERROR: STTBX_Init() returned %d\n", Code );
        return;
    }
#endif

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

    /* Initialize all pio devices */
    STSPI_Print(("Initializing pio devices..."));
    for (i = 0; i < NUMBER_PIO; i++)
    {
        Code = STPIO_Init(PIODevName[i], &PioInitParams[i]);
        if (Code != ST_NO_ERROR)
        {
            STSPI_Print(( "STPIO_Init() returned %d\n", Code ));
            return;
        }
    }
#ifndef STSPI_MASTER_SLAVE
#if defined(ST_7100) || defined(ST_7109)
    PIOOpenParams.IntHandler            =  NULL;
    PIOOpenParams.BitConfigure[7]       =  STPIO_BIT_OUTPUT_HIGH;
    PIOOpenParams.ReservedBits          =  0x80;
    Code =  STPIO_Open ( PIODevName[3],&PIOOpenParams, &PIOHandle );
    if (Code != ST_NO_ERROR)
    {
        STSPI_Print(( "STPIO_Init() returned %d\n", Code ));
        return;
    }
#elif defined(ST_5188)
    PIOOpenParams.IntHandler            =  NULL;
    PIOOpenParams.BitConfigure[4]       =  STPIO_BIT_OUTPUT_HIGH;
    PIOOpenParams.ReservedBits          =  0x10;
    Code =  STPIO_Open ( PIODevName[0],&PIOOpenParams, &PIOHandle );
    if (Code != ST_NO_ERROR)
    {
        STSPI_Print(( "STPIO_Init() returned %d\n", Code ));
        return;
    }
#elif defined(ST_5525)
    PIOOpenParams.IntHandler            =  NULL;
    PIOOpenParams.BitConfigure[3]       =  STPIO_BIT_OUTPUT_HIGH;
    PIOOpenParams.ReservedBits          =  0x08;
    Code =  STPIO_Open ( PIODevName[3],&PIOOpenParams, &PIOHandle );
    if (Code != ST_NO_ERROR)
    {
        STSPI_Print(( "STPIO_Init() returned %d\n", Code ));
        return;
    }
#endif
#endif
    /* Get the clk info */
    (void)ST_GetClockInfo(&ClockInfo);
    ClockSpeed = ClockInfo.CommsBlock;

    /* Program Configuartion registers for SPI */
#if defined(ST_5100) || defined(ST_5301)

#ifdef STSPI_MASTER_SLAVE
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) | 0x00000000 );
#else
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) | 0x000f0000 );
#endif
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, STSYS_ReadRegDev32LE(CONFIG_CONTROL_H) | 0x0000000f );

#elif defined(ST_5105) || defined(ST_5107)

    STSYS_WriteRegDev32LE(CONFIG_CONTROL_F, STSYS_ReadRegDev32LE(CONFIG_CONTROL_F) | 0x00000046 );
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, STSYS_ReadRegDev32LE(CONFIG_CONTROL_H) | 0x00000000 );

#elif defined(ST_5188)

    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) | 0x00000122 );

#endif



    STSPI_Print(("\n============================================================\n"));
    STSPI_Print(("                    STSPI Test Harness                    \n"));
    STSPI_Print(("            Driver Revision: %s\n", STSPI_GetRevision()));
    STSPI_Print(("            Test Harness Revision: %s\n", HarnessRev));
    STSPI_Print(( "============================================================\n" ));

#ifndef STSPI_MASTER_SLAVE
#ifdef ARCHITECTURE_ST20
    StackUsage();
#endif
    STSPI_Print(( "============================================================\n" ));


    Error = APITest();
    ErrorCount += Error;

    Error = LeakTest();
    ErrorCount += Error;

    Error = NormalTest();
    ErrorCount += Error;


#ifdef FULL_MEMORY_ACCESS

    Error = FullRWTest();
    ErrorCount += Error;

#endif /* FULL_MEMORY_ACCESS */

#else
#if defined(ST_5100) || defined(ST_7100) || defined(ST_5301) || defined(ST_8010) || defined(ST_7109) || defined(ST_5525)
    Error = MasterSlave();
#endif
    ErrorCount += Error;

#endif

    STSPI_Print(( "Overall Test result is: " ));
    STSPI_Print(( "%d errors\n", ErrorCount ));
    STSPI_Print(( "============================================================\n" ));

    Code = STBOOT_Term( "SPITest", &stboot_TermPars );

}

/* Worker routines for stackusage */
void test_overhead(void *dummy)
{
    STSPI_Print(( "test_overhead executing\n"));
}

void test_init(void *dummy)
{
    ST_ErrorCode_t error;
    STSPI_InitParams_t  InitParams;

    /* Build initparams */
    InitParams.BaseAddress        = (U32*)SSC_BASE_ADDRESS;
    InitParams.InterruptNumber    = SSC_INTERRUPT;
    InitParams.InterruptLevel     = SSC_INTERRUPT_LEVEL;
    InitParams.ClockFrequency     = ClockSpeed;
    InitParams.BusAccessTimeout   = 100000;
    InitParams.MaxHandles         = 2;
    InitParams.DriverPartition    = TEST_PARTITION_1;
    InitParams.MasterSlave        = STSPI_MASTER;
    strcpy(InitParams.PIOforMTSR.PortName, PIODevName[SSC_MTSR_DEV]);
    InitParams.PIOforMTSR.BitMask = SSC_MTSR_BIT;
    strcpy(InitParams.PIOforMRST.PortName, PIODevName[SSC_MRST_DEV]);
    InitParams.PIOforMRST.BitMask = SSC_MRST_BIT;
    strcpy(InitParams.PIOforSCL.PortName, PIODevName[SSC_SCL_DEV]);
    InitParams.PIOforSCL.BitMask  = SSC_SCL_BIT;
    InitParams.DefaultParams      = NULL;

    STSPI_Print(( "test_init executing\n"));
    /* Initialise */
    error = STSPI_Init("STACKTEST", &InitParams);
    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Error initializing, got return code %i\n", error));
    }
}

void test_term(void *dummy)
{
    ST_ErrorCode_t error;
    STSPI_TermParams_t  TermParams;

    STSPI_Print(( "test_term executing\n"));
    TermParams.ForceTerminate = FALSE;
    error = STSPI_Term("STACKTEST", &TermParams);
    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Error terminating, got return code %i\n", error));
    }
}

void test_typical(void *dummy)
{
    ST_ErrorCode_t   error = ST_NO_ERROR;
    int stored = 0;

    STSPI_OpenParams_t  OpenParams;
    STSPI_Handle_t      Handle;

    int                 i;
    U32                 NumRead;
    U32                 NumWritten;

    static U16          ReadArray[PAGE_SIZE]= {0};
    static U16          WritArray[PAGE_SIZE];

    STSPI_Print(( "test_typical executing\n"));

    /* Open Bank 0 */
    strcpy(OpenParams.PIOforCS.PortName, PIODevName[SSC_CS_DEV]);
    OpenParams.PIOforCS.BitMask = SSC_CS_BIT;

    error = STSPI_Open("STACKTEST", &OpenParams, &Handle );
    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Opening sector 0: "));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    error = MemoryErase( Handle ,SECTOR0_BASE_ADDRESS, 10000);
    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Erasing Sector 0: "));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    /* Program Sector 0,Page 0 */
    for (i = 0; i < PAGE_SIZE; i++)
    {
        WritArray[i] = (U8)i;
    }

    error = MemoryWrite( Handle, SECTOR0_BASE_ADDRESS,WritArray,
                         PAGE_SIZE , 10000, &NumWritten );
    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Writing to Sector 0,Page 0: "));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    error = MemoryRead( Handle, SECTOR0_BASE_ADDRESS,ReadArray,
                         PAGE_SIZE , 10000, &NumRead );

    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Reading Sector 0,Page 0: "));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }

    error = STSPI_Close(Handle);

    if (error != ST_NO_ERROR)
    {
        STSPI_Print(("Closing Sector 0: "));
        ErrorReport(&stored, error, ST_NO_ERROR);
    }
}

#ifndef STSPI_MASTER_SLAVE
/****************************************************************************
Name:           StackUsage
Description:    Prints out the stack required for init, term, and
                a typical sequence of operations.
Parameters:     None
Return value:   None
****************************************************************************/

static void StackUsage(void)
{
#ifndef ST_OS21
    task_t task;
    tdesc_t tdesc;
#endif
    task_t *task_p;

#ifndef ARCHITECTURE_ST40
    task_status_t status;
#endif

    char stack[STACKSIZE];
    void (*func_table[])(void *) =  {   test_init,
                                        test_overhead,
                                        test_typical,
                                        test_term,
                                        NULL
                                    };
    void (*func)(void *);
    U8 i;

    STSPI_Print(("Commencing Stack Usage Function ....\n" ));
    STSPI_Print(("============================================================\n"));

#ifndef ST_OS21
    task_p = &task;
#endif
    for (i = 0; func_table[i]!=NULL ; i++)
    {
        func = func_table[i];
#ifdef ST_OS21
        task_p = task_create(func, NULL, STACKSIZE, MAX_USER_PRIORITY,
                     "stack_test", 0);
#else
        task_init(func, NULL, stack, STACKSIZE, task_p, &tdesc,
                    (int)MAX_USER_PRIORITY, "stack_test", (task_flags_t)0);
#endif
        task_wait(&task_p, 1, TIMEOUT_INFINITY);

#ifndef ARCHITECTURE_ST40
        task_status(task_p, &status, task_status_flags_stack_used);
        STSPI_Print(("Function %i, stack used = %d\n", i,
                    status.task_stack_used));
#endif
        task_delete(task_p);
    }
    STSPI_Print(("\n"));
}

#endif
/****************************************************************************
Name         : APITest()

Description  : Performs API interface calls, generally mis-sequenced,
               with illegal parameters, or with the wrong Vpp bank
               selected.  This is intended to exercise as much of the
               error checking built into the routines as is practical.
               Note that errors should be raised during this test,
               but these MUST be the expected ones.  If this is the
               case, an overall pass will be reported.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE      Rogue Handle value passed
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               ST_ERROR_TIMEOUT             Timeout limit reached


See Also     : STSPI_ErrorCode_t
 ****************************************************************************/

int APITest( void )
{
    U32             NumberRead;
    U32             NumberWrit;

    int  ErrorCount = 0;
    ST_ErrorCode_t  ReturnError;
    static U16      ReadArray[PAGE_SIZE];
    static U16      WritArray[PAGE_SIZE];

    STSPI_InitParams_t  SPIInitParams;
    STSPI_Params_t      SPIGetParams,SPISetParams;
    STSPI_OpenParams_t  SPIOpenParams;
    STSPI_Handle_t      SPIHandle;
    STSPI_TermParams_t  SPITermParams;

    /* Build initparams */
    SPIInitParams.BaseAddress        = (U32*)SSC_BASE_ADDRESS;;
    SPIInitParams.InterruptNumber    = SSC_INTERRUPT;
    SPIInitParams.InterruptLevel     = SSC_INTERRUPT_LEVEL;
    SPIInitParams.ClockFrequency     = ClockSpeed;
    SPIInitParams.BusAccessTimeout   = 10000;
    SPIInitParams.MaxHandles         = 4;
    SPIInitParams.DriverPartition    = TEST_PARTITION_1;
    strcpy(SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC_MTSR_DEV]);
    SPIInitParams.PIOforMTSR.BitMask = SSC_MTSR_BIT;
    strcpy(SPIInitParams.PIOforMRST.PortName, PIODevName[SSC_MRST_DEV]);
    SPIInitParams.PIOforMRST.BitMask = SSC_MRST_BIT;
    strcpy(SPIInitParams.PIOforSCL.PortName, PIODevName[SSC_SCL_DEV]);
    SPIInitParams.PIOforSCL.BitMask  = SSC_SCL_BIT;

    SPIInitParams.DefaultParams      = NULL;

    STSPI_Print(( "\n" ));
    STSPI_Print(( "============================================================\n" ));
    STSPI_Print(( "Commencing API Test  ..\n" ));
    STSPI_Print(( "============================================================\n" ));

    /* Try to Open without a prior Init */
    STSPI_Print(( "Calling STSPI_Open() Sector 0 w/o Init .\n" ));
    strcpy(SPIOpenParams.PIOforCS.PortName, PIODevName[SSC_CS_DEV]);
    SPIOpenParams.PIOforCS.BitMask = SSC_CS_BIT;
    ReturnError = STSPI_Open( "SSC0_SPI",
                              &SPIOpenParams,
                              &SPIHandle );

    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    /* Exercise the Init error handling, including a successful call */
    STSPI_Print(( "Calling STSPI_Init(), NULL Name ptr. \n" ));
    ReturnError = STSPI_Init( NULL, &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    STSPI_Print(( "Calling STSPI_Init(), NULL DeviceName \n" ));
    ReturnError = STSPI_Init( "", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    STSPI_Print(( "Calling STSPI_Init(), too long Name .\n" ));
    ReturnError = STSPI_Init( "TooLongDeviceName", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    SPIInitParams.BaseAddress = NULL;    /* "forgotten" BaseAddress pointer */
    STSPI_Print(( "Calling STSPI_Init(), NULL BaseAddr. \n" ));
    ReturnError = STSPI_Init( "SSC2_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );
    SPIInitParams.BaseAddress = (U32*)SSC_BASE_ADDRESS; /* corrected value */

    SPIInitParams.DriverPartition = NULL; /* Driver partition  ptr. omitted */
    STSPI_Print(( "Calling STSPI_Init(), NULL Driver partition ptr\n" ));
    ReturnError = STSPI_Init( "SSC2_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );
    SPIInitParams.DriverPartition = TEST_PARTITION_1; /* corrected value */

    STSPI_Print(( "Calling STSPI_Init() with no errors .\n" ));
    ReturnError = STSPI_Init( "SSC2_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    STSPI_Print(( "Calling STSPI_Init() 2nd time no Term\n" ));
    ReturnError = STSPI_Init( "SSC2_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_ALREADY_INITIALIZED );

    /* Exercise API routines with wrong handle */
#ifndef ST_OS21
    STSPI_Print(( "Calling STSPI_GetParams() w/o Open ..\n" ));
    ReturnError = STSPI_GetParams( WRONG_HANDLE, &SPIGetParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_INVALID_HANDLE );

    STSPI_Print(( "Calling STSPI_SetParams() w/o Open ..\n" ));
    ReturnError = STSPI_GetParams( WRONG_HANDLE, &SPISetParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_INVALID_HANDLE );

    STSPI_Print(( "Calling STSPI_Read() w/o Open .......\n" ));
    ReturnError = STSPI_Read( WRONG_HANDLE,
                              ReadArray, /* read into this array */
                              PAGE_SIZE, /* no. to write */
                              10000,      /* timeout */
                              &NumberRead );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_INVALID_HANDLE );

    STSPI_Print(( "Calling STSPI_Write() w/o Open ......\n" ));
    ReturnError = STSPI_Write( WRONG_HANDLE,
                               WritArray,
                               PAGE_SIZE,
                               10000, /* timeout */
                               &NumberWrit );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_INVALID_HANDLE );
#endif
    /* Exercise the Open error handling, including a valid call */
    STSPI_Print(( "Calling STSPI_Open(), NULL Name .....\n" ));
    ReturnError = STSPI_Open( NULL,
                              &SPIOpenParams,
                              &SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    STSPI_Print(( "Calling STSPI_Open(), wrong Name ....\n" ));
    ReturnError = STSPI_Open( "different",
                               &SPIOpenParams,
                               &SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    STSPI_Print(( "Calling STSPI_Open(), NULL Handle ptr\n" ));
    ReturnError = STSPI_Open( "SSC2_SPI",
                              &SPIOpenParams,
                               NULL );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    STSPI_Print(( "Calling STSPI_Open(), no errors .....\n" ));
    ReturnError = STSPI_Open( "SSC2_SPI",
                              &SPIOpenParams,
                              &SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Exercise API calls with various errors */
    STSPI_Print(( "Calling STSPI_GetParams(), NULL ptr. \n" ));
    ReturnError = STSPI_GetParams(SPIHandle, NULL );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    STSPI_Print(( "Calling STSPI_Read(), zero bytes ....\n" ));
    ReturnError = STSPI_Read( SPIHandle,
                              ReadArray, /* read into this array */
                              0,         /* no. to read */
                              10000,      /* timeout */
                              &NumberRead );
    STSPI_Print(( "NumberActuallyRead reported as %d bytes\n",
                   NumberRead ));
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    STSPI_Print(( "Calling STSPI_Write(), zero bytes ...\n" ));
    ReturnError = STSPI_Write( SPIHandle,
                               WritArray,
                               0,
                               10000,
                               &NumberWrit );
    STSPI_Print(( "NumberActuallyWritten reported as %d bytes\n",
                   NumberWrit ));
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Exercise Term with Handle open, unforced */
    SPITermParams.ForceTerminate = FALSE;
    STSPI_Print(( "Calling STSPI_Term(), Handle open ...\n" ));
    ReturnError = STSPI_Term( "SSC2_SPI", &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_OPEN_HANDLE );

    /* Exercise Close error handling, including a valid call */
#ifndef ST_OS21
    STSPI_Print(( "Calling STSPI_Close(), bad Handle ...\n" ));
    ReturnError = STSPI_Close( WRONG_HANDLE );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_INVALID_HANDLE );
#endif

    STSPI_Print(( "Calling STSPI_Close() with no errors \n" ));
    ReturnError = STSPI_Close( SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Exercise remaining Term error handling, including a valid call */
    STSPI_Print(( "Calling STSPI_Term(), NULL DeviceName\n" ));
    ReturnError = STSPI_Term( NULL, &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_BAD_PARAMETER );

    STSPI_Print(( "Calling STSPI_Term(), differing Name \n" ));
    ReturnError = STSPI_Term( "NotTheSame", &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    STSPI_Print(( "Calling STSPI_Term() with no errors .\n" ));
    ReturnError = STSPI_Term( "SSC2_SPI", &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    STSPI_Print(( "Calling STSPI_Term() again, else OK .\n" ));
    ReturnError = STSPI_Term( "SSC2_SPI", &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_ERROR_UNKNOWN_DEVICE );

    STSPI_Print(( "API test result is " ));
    STSPI_Print(( "%d errors\n", ErrorCount ));

    return( ErrorCount );
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

void ErrorReport( int *ErrorCount,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr )
{
    ST_ErrorCode_t  Temp = ErrorGiven;

    switch (Temp)
    {
        case ST_NO_ERROR:
            STSPI_Print(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            STSPI_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            STSPI_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            STSPI_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            STSPI_Print(( "ST_ERROR_INVALID_HANDLE - Rogue Handle value or Closed\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            STSPI_Print(( "ST_ERROR_OPEN_HANDLE - Term called with Open Handle\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            STSPI_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            STSPI_Print(( "ST_ERROR_NO_FREE_HANDLES - DeviceName already Open\n" ));
            break;

        case ST_ERROR_TIMEOUT:
            STSPI_Print(( "ST_ERROR_TIMEOUT - Timeout limit reached\n" ));
            break;

        case STSPI_ERROR_NO_FREE_SSC:
            STSPI_Print(( "STSPI_ERROR_NO_FREE_SSC - No Free SSC\n" ));
            break;

        case STSPI_ERROR_DATAWIDTH:
            STSPI_Print(( "STSPI_ERROR_DATAWIDTH - Incorrect DataWidth\n" ));
            break;

        case STSPI_ERROR_PIO:
            STSPI_Print(( "STSPI_ERROR_PIO - Error in PIO access\n" ));
            break;

        case STSPI_ERROR_EVT_REGISTER:
            STSPI_Print(( "STSPI_ERROR_EVT_REGISTER - Error in EVT registration\n" ));
            break;

        default:
            STSPI_Print(( "*** Unrecognised return code %x ***\n",Temp));
        }

        if (ErrorGiven != ExpectedErr)
        {
            (*ErrorCount)++;
        }

    STSPI_Print(( "----------------------------------------------------------\n" ));
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


    CurrTime = time_now();
    DiffTime = time_minus( CurrTime, SnapTime );

    STSPI_Print(( "Execution time taken was %d slow ticks\n", (U32)DiffTime ));
}


/****************************************************************************
Name         : NormalTest()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising all the
               Direct Access Flash calls for all hardware supported
               combinations of memory access widths, determined by
               the input parameter values.  Both Flash banks fitted
               to the Evaluation Platform are exercised without
               interleaved Term & Init calls.  This will prove the
               additional support for multiple instances added
               between 1.0.0 and 1.1.0 Driver Releases.
               Note that no errors should be raised during this test.
               Else the first error encountered will be returned.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE      Rogue Flash bank reference
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               ST_ERROR_TIMEOUT             Timeout error during Erase/Write


See Also     : STFLASH_ErrorCode_t
 ****************************************************************************/

int NormalTest( void )
{
    BOOL                Match;
    U32                 i;
    U32                 NumberRead;
    U32                 NumberWrit;
    clock_t             SnapTime;
    int  ErrorCount     = 0;
    ST_ErrorCode_t      ReturnError;
    STSPI_InitParams_t  SPIInitParams;
    STSPI_OpenParams_t  SPIOpenParams;
    STSPI_Handle_t      SPIHandle;
    STSPI_TermParams_t  SPITermParams;

    static U16          ReadArray[SECTOR_SIZE]={0};
    static U16          WritArray[SECTOR_SIZE];

    STSPI_Print(( "\n" ));
    STSPI_Print(( "============================================================\n" ));
    STSPI_Print(( "Commencing NormalUse Test Function ....\n" ));
    STSPI_Print(( "============================================================\n" ));

    /* Initialisations */
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

    STSPI_Print(( "Calling STSPI_Init() ..........\n" ));
    ReturnError = STSPI_Init( "SSC_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Open Sector 0 */
    STSPI_Print(( "Calling STSPI_Open() ..........\n" ));
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[SSC_CS_DEV] );
    SPIOpenParams.PIOforCS.BitMask     = SSC_CS_BIT;
    ReturnError = STSPI_Open( "SSC_SPI",
                              &SPIOpenParams,
                              &SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Erase Sector 0,since E2p10A is a flash memory needs sector erase
     * before writing over it
     */
    STSPI_Print(( "Calling MemoryErase() to Erase Sector0 ..........\n" ));
    ReturnError = MemoryErase( SPIHandle,
                               SECTOR0_BASE_ADDRESS,
                               10000);
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );


    STSPI_Print(( "Calling MemoryRead() to verify Erase of Sector 0..\n" ));
    SnapTime = time_now();
    ReturnError = MemoryRead( SPIHandle,
                              SECTOR0_BASE_ADDRESS,
                              ReadArray,
                              SECTOR_SIZE,
                              10000,
                              &NumberRead );
    ExTimeReport( SnapTime );
    STSPI_Print(( "Number Actually Read are %d bytes\n",NumberRead ));
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    Match = TRUE;
    if ((ReturnError == ST_NO_ERROR) &&
        (NumberRead  == SECTOR_SIZE))
    {
        for (i = 0; i < SECTOR_SIZE; i++)
        {
            if (ReadArray[i] != ERASED_STATE)
            {
                Match = FALSE;
                STSPI_Print(( "Mismatch => Erase/ReadArray[%2d] = %02X / %02X\n",
                               i, ERASED_STATE, (U32)ReadArray[i] ));
            }
        }

        if (Match)
        {
            STSPI_Print(( "Erased data verified with Read of %d bytes\n",
                           NumberRead ));
        }

        STSPI_Print(( "----------------------------------------------------------\n" ));
    }
    /* Program Sector 0 */
    for (i = 0; i < SECTOR_SIZE; i++)
    {
        WritArray[i] = (U8)i;
    }

    SnapTime = time_now();

    STSPI_Print(( "Calling STSPI_Write() to write into Sector 0..\n" ));
    ReturnError = MemoryWrite( SPIHandle,
                               SECTOR0_BASE_ADDRESS,
                               WritArray,
                               SECTOR_SIZE,
                               10000,
                               &NumberWrit );

    ExTimeReport( SnapTime );
    STSPI_Print(( "Number Actually Written are %d bytes\n",
                   NumberWrit ));
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    if (NumberWrit > 0)
    {
        STSPI_Print(( "Calling STSPI_Read() to verify write on Sector 0...\n" ));
        SnapTime = time_now();
        ReturnError = MemoryRead( SPIHandle,
                                  SECTOR0_BASE_ADDRESS,
                                  ReadArray,
                                  SECTOR_SIZE,
                                  10000,
                                  &NumberRead );
        ExTimeReport( SnapTime );
        STSPI_Print(( "Number Actually Read are %d bytes\n",
                      NumberRead ));
        ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        Match = TRUE;
        if ((ReturnError == ST_NO_ERROR) &&
            (NumberRead  == SECTOR_SIZE))
        {
            for (i = 0; i < NumberRead; i++)
            {
                if (WritArray[i] != ReadArray[i])
                {
                    Match = FALSE;
                    STSPI_Print(( "Mismatch => Write/ReadArray[%2d] = %02X / %02X\n",
                                   i, (U32)WritArray[i], (U32)ReadArray[i] ));
                }
            }

            if (Match)
            {
                STSPI_Print(( "Write data verified with Read of %d bytes\n",
                              NumberRead ));
            }

            STSPI_Print(( "----------------------------------------------------------\n" ));
        }
    }

    /* Close Sector 0 */
    STSPI_Print(( "Calling STSPI_Close()........\n" ));
    ReturnError = STSPI_Close( SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );


    /* Term Sector 0 */
    SPITermParams.ForceTerminate = FALSE;
    STSPI_Print(( "Calling STSPI_Term() .........\n" ));
    ReturnError = STSPI_Term( "SSC_SPI", &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );


    STSPI_Print(( "----------------------------------------------------------\n" ));

    STSPI_Print(( "Normal Use test result is: " ));
    STSPI_Print(( "%d errors\n", ErrorCount ));

    return( ErrorCount );
}

#ifdef FULL_MEMORY_ACCESS

/****************************************************************************
Name         : FullRWTest()

Description  : Writes to each block in turn and read verifies.
               Every location is erased once, written to once
               and read twice.
               This routine is not included in the default test
               as all locations are overwritten.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                  No errors occurred
               ST_ERROR_ALREADY_INITIALIZED More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE      Open call w/o prior Init
               ST_ERROR_INVALID_HANDLE
               ST_ERROR_BAD_PARAMETER       One or more invalid parameters
               ST_ERROR_TIMEOUT             Timeout error during Erase/Write


See Also     : STSPI_ErrorCode_t
 ****************************************************************************/

int FullRWTest( void )
{
    BOOL                Match;
    int                 i,j;
    U32                 NumberRead;
    U32                 NumberWrit;
    clock_t             SnapTime;
    int  ErrorCount     = 0;
    ST_ErrorCode_t      ReturnError;
    STSPI_InitParams_t  SPIInitParams;
    STSPI_OpenParams_t  SPIOpenParams;
    STSPI_Handle_t      SPIHandle;
    STSPI_TermParams_t  SPITermParams;
    static U16          ReadArray[MEMORY_SIZE]={0};
    static U16          WritArray[MEMORY_SIZE];
    U32                 ErrorCounter;;


    STSPI_Print(( "\n============================================================\n" ));
    STSPI_Print(( "Commencing Full Memory Read/Write Test Function ....\n" ));
    STSPI_Print(( "============================================================\n" ));

    /* Init Sector0,page 0 */
    SPIInitParams.BaseAddress           = (U32*)SSC_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC_INTERRUPT_LEVEL;
    SPIInitParams.ClockFrequency        = ClockSpeed;
    SPIInitParams.BusAccessTimeout      = 100000;
    SPIInitParams.PIOforMTSR.BitMask    = SSC_MTSR_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC_MRST_BIT;
    SPIInitParams.PIOforSCL.BitMask     = SSC_SCL_BIT;
    SPIInitParams.MaxHandles            = 4;
    SPIInitParams.DriverPartition       = TEST_PARTITION_1;
    SPIInitParams.MasterSlave           = STSPI_MASTER;
    SPIInitParams.DefaultParams         = NULL;
    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC_MRST_DEV] );
    strcpy( SPIInitParams.PIOforSCL.PortName, PIODevName[SSC_SCL_DEV] );



    STSPI_Print(( "Calling STSPI_Init()  ..........\n" ));
    ReturnError = STSPI_Init( "SSC_SPI", &SPIInitParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    /* Open Bank 0 */
    STSPI_Print(( "Calling STSPI_Open()  ..........\n" ));
    strcpy( SPIOpenParams.PIOforCS.PortName, PIODevName[SSC_CS_DEV] );
    SPIOpenParams.PIOforCS.BitMask     = SSC_CS_BIT;
    ReturnError = STSPI_Open( "SSC_SPI",
                              &SPIOpenParams,
                              &SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );


    for ( j=0; j<NUMBER_OF_SECTORS; j++ )
    {

        /* Erase Sector 0,since E2p10A is a flash memory needs sector erase
         * before writing over it
         */
        STSPI_Print(( "Calling MemoryErase()  of Sector %d........\n",j));
        ReturnError = MemoryErase( SPIHandle,
                                   SectorBase[j],
                                   10000);
        ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

        STSPI_Print(( "Calling MemoryRead() to verify Erase of Sector %d\n",j ));
        SnapTime = time_now();
        ReturnError = MemoryRead( SPIHandle,
                                  SectorBase[j],
                                  ReadArray,
                                  SECTOR_SIZE,
                                  10000,
                                  &NumberRead );
        ExTimeReport( SnapTime );
        STSPI_Print(( "Number Actually Read are %d bytes\n",NumberRead ));
        ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        Match = TRUE;
        if ((ReturnError == ST_NO_ERROR) &&
            (NumberRead  == SECTOR_SIZE))
        {
            for (i = 0; i < SECTOR_SIZE; i++)
            {
                if(ReadArray[i] != ERASED_STATE)
                {
                    Match = FALSE;
                    STSPI_Print(( "Mismatch => Erase/ReadArray[%2d] = %02X / %02X\n",
                               i, ERASED_STATE, (U32)ReadArray[i] ));
                }
            }

            if (Match)
            {
                STSPI_Print(( "Erased data verified with Read of %d bytes\n",
                           NumberRead ));
            }

            STSPI_Print(( "----------------------------------------------------------\n" ));
        }
    }

    /* Program  Sector SectorNo, Page TestPage */
    for (i = 0; i < MEMORY_SIZE; i++)
    {
        WritArray[i] = (U8)i;
        ReadArray[i] = (U8)0;

    }

    STSPI_Print(( "Calling MemoryWrite() .......\n"));
    ReturnError = MemoryWrite( SPIHandle,
                               SECTOR0_BASE_ADDRESS,
                               WritArray,
                               MEMORY_SIZE,
                               100000,
                               &NumberWrit );

    if (ReturnError != ST_NO_ERROR)
    {
        ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
    }

    STSPI_Print(( "NumberActuallyWritten reported as %d bytes\n",
                   NumberWrit ));

    if (NumberWrit > 0)
    {
        STSPI_Print(( "Calling MemoryRead()  ........\n"));
        ReturnError = MemoryRead( SPIHandle,
                                  SECTOR0_BASE_ADDRESS,
                                  ReadArray,
                                  MEMORY_SIZE,
                                  200000,
                                  &NumberRead );

        if (ReturnError != ST_NO_ERROR)
        {
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
        }

        STSPI_Print(( "NumberActuallyRead reported as %d bytes\n",
                       NumberRead ));
        Match = TRUE;
        if ((ReturnError == ST_NO_ERROR) &&
            (NumberRead  == MEMORY_SIZE))
        {
            ErrorCounter = 0;
            for (i = 0; i < NumberRead; i++)
            {
                if (WritArray[i] != ReadArray[i])
                {
                    Match = FALSE;
                    STSPI_Print(( "Mismatch => Write/ReadArray[%2d] = %02X / %02X\n",
                                          i, (U32)WritArray[i], (U32)ReadArray[i] ));
                    ErrorCounter++;
                }
            }

            if (Match)
            {
                STSPI_Print(( "Write data verified with Read of %d bytes\n",
                               NumberRead ));
            }
            else
            {
                STSPI_Print(( "=====> VERIFY ERROR: %u errors out of %u bytes\n",
                                    ErrorCounter, NumberRead ));
            }
        }

        STSPI_Print(( "------------------------------------------------------------\n" ));

    }

    STSPI_Print(( "Calling STSPI_Close()  ..........\n" ));
    ReturnError = STSPI_Close( SPIHandle );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    SPITermParams.ForceTerminate = TRUE;
    STSPI_Print(( "Calling STSPI_Term()  .........\n" ));
    ReturnError = STSPI_Term( "SSC_SPI", &SPITermParams );
    ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

    return ErrorCount;
}

#endif

/****************************************************************************
Name         : LeakTest()

Description  : Checks the memory leak

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR    No errors occurred

****************************************************************************/

int LeakTest( void )
{

    ST_ErrorCode_t ReturnError = ST_NO_ERROR;
    partition_status_t  pstatus;
    U32  PrevMem,AfterMem;
    int  ErrorCount = 0;
    STSPI_TermParams_t  SPITermParams;
    STSPI_InitParams_t  SPIInitParams;

    /* Init Sector0,page 0 */
    SPIInitParams.BaseAddress           = (U32*)SSC_BASE_ADDRESS;
    SPIInitParams.InterruptNumber       = SSC_INTERRUPT;
    SPIInitParams.InterruptLevel        = SSC_INTERRUPT_LEVEL;
    SPIInitParams.ClockFrequency        = ClockSpeed;
    SPIInitParams.BusAccessTimeout      = 10000;
    SPIInitParams.PIOforMTSR.BitMask    = SSC_MRST_BIT;
    SPIInitParams.PIOforMRST.BitMask    = SSC_MTSR_BIT;
    SPIInitParams.PIOforSCL.BitMask     = SSC_SCL_BIT;
    SPIInitParams.MaxHandles            = 4;
    SPIInitParams.DriverPartition       = TEST_PARTITION_1;
    strcpy( SPIInitParams.PIOforMTSR.PortName, PIODevName[SSC_MTSR_DEV] );
    strcpy( SPIInitParams.PIOforMRST.PortName, PIODevName[SSC_MRST_DEV] );
    strcpy( SPIInitParams.PIOforSCL.PortName, PIODevName[SSC_SCL_DEV] );
    SPIInitParams.DefaultParams         = NULL;

    STSPI_Print(( "\n" ));
    STSPI_Print(( "============================================================\n" ));
    STSPI_Print(( "Commencing Leak Test  ..\n" ));
    STSPI_Print(( "============================================================\n" ));

    if (0 == partition_status(TEST_PARTITION_1, &pstatus,(partition_status_flags_t)0))
    {

        PrevMem = pstatus.partition_status_free;

        /* Init */
        {
            /* Setup SPI */
            STSPI_Print(( "Calling STSPI_Init() to allocate memory for driver\n" ));
            ReturnError = STSPI_Init("SSC_SPI",&SPIInitParams);
            ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );

        }

        if (ReturnError == ST_NO_ERROR)
        {

            /* STSPI_Term() */
            {
                SPITermParams.ForceTerminate = FALSE;
                STSPI_Print(( "Calling STSPI_Term() to ensure all memory has been deallocated\n" ));
                ReturnError = STSPI_Term("SSC_SPI",&SPITermParams);
                ErrorReport( &ErrorCount, ReturnError, ST_NO_ERROR );
            }

            if (ReturnError == ST_NO_ERROR)
            {
                if (0 == partition_status(TEST_PARTITION_1, &pstatus,(partition_status_flags_t)0))
                {
                    AfterMem = pstatus.partition_status_free;
                    if (PrevMem != AfterMem)
                    {
                        ErrorCount++;
                    }
                }
                else
                {
                    ErrorCount++;
                }
            }
            else
            {
                ErrorCount++;
            }

        }
        else
        {
            ErrorCount++;
        }
    }
    else
    {
        ErrorCount++;
    }

    STSPI_Print(( "Leak test result is " ));
    STSPI_Print(( "%d errors\n", ErrorCount ));

    return (ErrorCount);

}


