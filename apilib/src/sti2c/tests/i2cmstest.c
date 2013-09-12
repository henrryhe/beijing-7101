/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007
Source file name : i2cmstest.c

I2C tests - master/slave tests.

This program is intended to run on 2 systems, with the Back I2C buses connected
together. For the STi5510 MB231 board, this is done via the Front Panel
connectors on the front of the box.

The program initially polls the other processor for a 1 byte message, which
should contain a special value. This indicates that the other processor has
started running. When it gets this message, the main part of the test is
entered.

The program then enters a loop sending and receiving messages. Each time around
the loop, it sends message of a random length and content to the other
processor, as a master mode transfer, and then attempts to read a message in
response (of random length less than or equal to the message written). The
response message should have a specific content (see below). There is a
variable random delay between loop iterations.

In parallel with this, the program must respond to slave mode requests from
the other processor (which will be trying to do to us what we are doing to it).
These requests are signalled as events, and come in at interrupt level.

The test passes only if all messages are send and received without error,
and the content of all received messages is as expected.

************************************************************************/
#if !defined(ST_OSLINUX)  /* linux changes */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "stlite.h"
#include <time.h>
#include "sttbx.h"
#include "stpio.h"

#ifdef ST_OS20
#include <ostime.h>
#include "debug.h"
#endif

#else
#include <unistd.h>
#define  STTBX_Print(x) printf x
#include "compat.h"
#endif

#include "stos.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stack.h"
#include "stevt.h"
#include "stboot.h"
#include "stcommon.h"
#include "sti2c.h"
/*added for 5100 sysytem register configuration*/
#include "stsys.h"

/* Private types/constants ----------------------------------------------- */
/* Private types/constants ----------------------------------------------- */
#if defined(ST_5100)

#define INTERCONNECT_BASE                      (U32)0x20D00000

#define INTERCONNECT_CONFIG_CONTROL_REG_D       0x04
#define INTERCONNECT_CONFIG_CONTROL_REG_F       0x0C
#define INTERCONNECT_CONFIG_CONTROL_REG_G       0x10
#define INTERCONNECT_CONFIG_CONTROL_REG_H       0x14

#define CONFIG_CONTROL_D (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_D)
#define CONFIG_CONTROL_F (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_F)
#define CONFIG_CONTROL_G (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_G)
#define CONFIG_CONTROL_H (INTERCONNECT_BASE + INTERCONNECT_CONFIG_CONTROL_REG_H)

#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
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

#if defined(ST_5525)

#define INTERCONNECT_BASE              ST5525_IC_BASE_ADDRESS

#define CONFIG_CONTROL_1   (INTERCONNECT_BASE + 0x14)
#define CONFIG_CONTROL_2   (INTERCONNECT_BASE + 0x18)

#endif

#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || \
    defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_5107) 

#define     SSC0_BASE_ADDRESS           SSC_0_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_0_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        SSC_0_INTERRUPT_LEVEL

#define     SSC0_PIO_BASE               PIO_3_BASE_ADDRESS
#define     SSC0_PIO_INTERRUPT          PIO_3_INTERRUPT
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_3_INTERRUPT_LEVEL

#define     SSC1_BASE_ADDRESS           SSC_1_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_1_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        SSC_1_INTERRUPT_LEVEL

#define     SSC1_PIO_BASE               PIO_3_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_3_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_3_INTERRUPT_LEVEL

#elif defined(ST_7710)

#define     SSC0_BASE_ADDRESS           SSC_0_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_0_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        SSC_0_INTERRUPT_LEVEL

#define     SSC0_PIO_BASE               PIO_2_BASE_ADDRESS
#define     SSC0_PIO_INTERRUPT          PIO_2_INTERRUPT
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_2_INTERRUPT_LEVEL

#define     SSC1_BASE_ADDRESS           SSC_2_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_2_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        5

#define     SSC1_PIO_BASE               PIO_4_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_4_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_4_INTERRUPT_LEVEL

#elif defined(ST_7100) || defined(ST_7109) || defined(ST_7200)

#define     SSC0_BASE_ADDRESS           SSC_0_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_0_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        SSC_0_INTERRUPT_LEVEL

#define     SSC0_PIO_BASE               PIO_2_BASE_ADDRESS
#define     SSC0_PIO_INTERRUPT          PIO_2_INTERRUPT
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_2_INTERRUPT_LEVEL

#define     SSC1_BASE_ADDRESS           SSC_1_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_1_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        SSC_1_INTERRUPT_LEVEL

#define     SSC1_PIO_BASE               PIO_3_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_3_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_3_INTERRUPT_LEVEL

#elif defined(ST_5700)

#define     SSC0_BASE_ADDRESS           SSC_0_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_0_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        SSC_0_INTERRUPT_LEVEL

#define     SSC0_PIO_BASE               PIO_1_BASE_ADDRESS
#define     SSC0_PIO_INTERRUPT          PIO_1_INTERRUPT
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_1_INTERRUPT_LEVEL

#define     SSC1_BASE_ADDRESS           SSC_1_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_1_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        SSC_1_INTERRUPT_LEVEL

#define     SSC1_PIO_BASE               PIO_0_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_0_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_0_INTERRUPT_LEVEL

#elif defined(ST_8010)

#define     SSC0_BASE_ADDRESS           SSC_0_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_0_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        SSC_0_INTERRUPT_LEVEL

#define     SSC0_PIO_BASE               PIO_1_BASE_ADDRESS      /* PIO address and pin numbers are not required*/
#define     SSC0_PIO_INTERRUPT          PIO_1_INTERRUPT         /* for SSC0 and SSC1 in case of STi8010, here  */
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_1_INTERRUPT_LEVEL   /* they have been added just for compilation purpose */

#define     SSC1_BASE_ADDRESS           SSC_1_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_1_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        SSC_1_INTERRUPT_LEVEL

#define     SSC1_PIO_BASE               PIO_1_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_1_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_1_INTERRUPT_LEVEL

#elif defined(ST_5188)

#define     SSC0_BASE_ADDRESS           SSC_0_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_0_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        SSC_0_INTERRUPT_LEVEL

#define     SSC0_PIO_BASE               PIO_0_BASE_ADDRESS
#define     SSC0_PIO_INTERRUPT          PIO_0_INTERRUPT
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_0_INTERRUPT_LEVEL

#define     SSC1_BASE_ADDRESS           SSC_1_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_1_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        SSC_1_INTERRUPT_LEVEL

#define     SSC1_PIO_BASE               PIO_0_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_0_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_0_INTERRUPT_LEVEL

#elif defined(ST_5525)

#define     SSC0_BASE_ADDRESS           SSC_2_BASE_ADDRESS
#define     SSC0_INTERRUPT              SSC_2_INTERRUPT
#define     SSC0_INTERRUPT_LEVEL        5

#define     SSC0_PIO_BASE               PIO_2_BASE_ADDRESS
#define     SSC0_PIO_INTERRUPT          PIO_2_INTERRUPT
#define     SSC0_PIO_INTERRUPT_LEVEL    PIO_2_INTERRUPT_LEVEL

#define     SSC1_BASE_ADDRESS           SSC_3_BASE_ADDRESS
#define     SSC1_INTERRUPT              SSC_3_INTERRUPT
#define     SSC1_INTERRUPT_LEVEL        5

#define     SSC1_PIO_BASE               PIO_4_BASE_ADDRESS
#define     SSC1_PIO_INTERRUPT          PIO_4_INTERRUPT
#define     SSC1_PIO_INTERRUPT_LEVEL    PIO_4_INTERRUPT_LEVEL

#else

#define     SSC_0_PIO_BASE              PIO_1_BASE_ADDRESS
#define     SSC_0_PIO_INTERRUPT         PIO_1_INTERRUPT
#define     SSC_0_PIO_INTERRUPT_LEVEL   PIO_1_INTERRUPT_LEVEL

#endif

#if defined(ST_5510) || defined(ST_5512) || defined(ST_TP3)

#define PIO_FOR_SSC0_SDA                PIO_BIT_0
#define PIO_FOR_SSC0_SCL                PIO_BIT_2

#elif defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_5188) \
|| defined(ST_7200)
#define PIO_FOR_SSC0_SDA                PIO_BIT_1
#define PIO_FOR_SSC0_SCL                PIO_BIT_0

#elif defined(ST_5525)
#define PIO_FOR_SSC0_SDA                PIO_BIT_4
#define PIO_FOR_SSC0_SCL                PIO_BIT_5

#else
/* 08, 18, 14, 16, 17, 28, 5105/07, 5103 ... */
#define PIO_FOR_SSC0_SDA                PIO_BIT_0
#define PIO_FOR_SSC0_SCL                PIO_BIT_1

#endif

#if defined (USE_ONE_BOARD)
/*
** This option uses SSC0 and SSC1 on the same board
** But not all boards can support this option
*/
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528)|| \
    defined(ST_5100) || defined(ST_5105) || defined(ST_5301) || defined(ST_5525) || defined(ST_5107)

#define PIO_FOR_SSC1_SDA                PIO_BIT_2
#define PIO_FOR_SSC1_SCL                PIO_BIT_3

#elif defined(ST_5508) || defined(ST_5518)
#error  This board does not support linking two SSCs

#elif defined(ST_7710) || defined(ST_7100) || defined(ST_7109) || defined(ST_7200)
#define PIO_FOR_SSC1_SDA                PIO_BIT_1
#define PIO_FOR_SSC1_SCL                PIO_BIT_0

#elif defined(ST_8010)
#define PIO_FOR_SSC1_SDA                PIO_BIT_5
#define PIO_FOR_SSC1_SCL                PIO_BIT_7

#elif defined(ST_5188)
#define PIO_FOR_SSC1_SDA                PIO_BIT_3
#define PIO_FOR_SSC1_SCL                PIO_BIT_2

#else
#error  The USE_ONE_BOARD option is not supported for this board

#endif
#endif


/* Sizes of partitions */
#if defined(ST_OSLINUX)
#define SYSTEM_PARTITION_SIZE     (ST_Partition_t *)0x1
#define TEST_PARTITION_1          (ST_Partition_t *)0x1
#define TEST_PARTITION_2          (ST_Partition_t *)0x1
ST_Partition_t                    *DriverPartition = (ST_Partition_t*)1;
ST_Partition_t                    *system_partition = (ST_Partition_t*)1;
ST_Partition_t                    *NcachePartition = (ST_Partition_t*)1;

#else

#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            100000
#define TEST_PARTITION_1        &the_system_partition
#define TEST_PARTITION_2        &the_internal_partition

#endif /*ST_OSLINUX*/

#if defined (USE_ONE_BOARD)
#define SLAVE_DEVICE_ID        0x9A     /* SSC1 slave address */
#else
#define SLAVE_DEVICE_ID        0x42     /* SSC0 slave address */
#endif
#define MAGIC_RESPONSE         0x2A
#define MESSAGE_COUNT          1000
#define ERROR_ABORT_THRESHOLD  100
#define MAX_PHASE              10

#define TRACE_PAGE_SIZE        50

/* LUT terminator */
#define ST_ERROR_UNKNOWN            ((U32)-1)

#undef  VERBOSE
#define PRINTV                 if (Verbose) myprintf

typedef ST_ErrorCode_t (*I2cFunc_t) (STI2C_Handle_t, U8*, U32, U32, U32*);

/* Error messages */
typedef struct I2C_ErrorMessage_s
{
    ST_ErrorCode_t Error;
    char ErrorMsg[56];
} I2C_ErrorMessage_t;

#define UNKNOWN_ERROR            ((U32)-1)

static I2C_ErrorMessage_t I2C_ErrorLUT[] =
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
    { UNKNOWN_ERROR, "UNKNOWN ERROR" } /* Terminator */
};

/* Private variables ------------------------------------------------------ */

static U32     ErrorCount [MAX_PHASE]; /* Overall count of errors */
static U32     HwBugCount    = 0;      /* Count of errors caused by h/w bugs */
static U32     MessageNumber = 0;
static U32     TestPhase     = 0;
static BOOL    MessageNumPrinted = FALSE;
static BOOL    MasterFirst       = FALSE;
static BOOL    NoStopTest        = FALSE;

/* Set TRUE to generate extra output for debug etc. */
static BOOL    Verbose      =
#ifdef VERBOSE
                             TRUE;
#else
                             FALSE;
#endif

static U32 TicksPerSec;
/* Only one evt device required, since we do device registration/subscriber */
static ST_DeviceName_t   EVTDevName0 = "EVT0";
static STI2C_Handle_t    MasterHandle, SSC0Handle;
#if defined (USE_ONE_BOARD)
static STI2C_Handle_t    SlaveHandle, SSC1Handle;
#endif

/* Data for slave mode operation */
static U8    RxBuffer [1024];
static U32   TxNotifyCount=0, TxCompleteCount=0, TxByteCount=0;
static U32   RxNotifyCount=0, RxCompleteCount=0, RxByteCount=0;
static U32   MasterRxCount=0, MasterTxCount=0, RxIndex=0, TxIndex=0;

#if !defined(ST_OSLINUX)
#ifdef ST_OS20
/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section_noinit")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#pragma ST_section      ( system_block,   "system_section_noinit")

#pragma ST_section      ( RxBuffer,   "ncache_section")

/* This is to avoid a linker warning */
static unsigned char    internal_block_init[1];
#pragma ST_section      ( internal_block_init, "internal_section")

static unsigned char    system_block_init[1];
#pragma ST_section      ( system_block_init, "system_section")

#if defined(ST_5528)|| defined(ST_5100)|| defined(ST_7710) || defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
static U8               data_section[1];
#pragma ST_section      ( data_section,   "data_section")
#endif

#ifdef UNIFIED_MEMORY

#if defined (ST_5516) || defined(ST_5517)
static unsigned char    ncache2_block[1];
#pragma ST_section      ( ncache2_block, "ncache2_section")
#endif

#endif
#else /* ST_OS21 */
#define                 SYSTEM_MEMORY_SIZE          0x100000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition_p;
#endif /* ST_OS20 */
extern ST_ErrorCode_t STI2C_Reset(STI2C_Handle_t);
#endif/*!(ST_OSLINUX)*/


#if defined(STI2C_DEBUG)
extern U8 TraceBuffer[];
extern U32 TraceBufferPos;
#endif

/* Function prototypes ----------------------------------------------------- */

static void DoTheTests (void);
static void PrintTestResults (void);
void myprintf (char *format, ...);

#ifdef STI2C_DEBUG
extern void cleartrace(void);
extern void dumptrace(char *filename);
#endif
#ifdef STI2C_TRACE
extern volatile U32 I2cIndex;
extern volatile U32 I2cLastIndex;
extern U32 I2cPrintTrace (S32 Start, S32 Number);
extern void I2cClearTrace(void);
#endif

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

    myprintf(("Print trace?\n"));
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

#endif /* STI2C_TRACE */


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
#if defined(ST_5100)||defined(ST_7710)|| defined(ST_5105) || defined(ST_5188) || defined(ST_5525) || defined(ST_5107)
    U32 ReadValue=0xFFFFFFFF;
#endif

#if !defined(ST_OSLINUX)
#if defined(DISABLE_DCACHE)
    STBOOT_DCache_Area_t  Cache[] = { { NULL, NULL } };
#elif defined(CACHEABLE_STOP_ADDRESS)
    /* We now rely on this being present for 5528 and upwards. */
    STBOOT_DCache_Area_t Cache[] =
        {   /* assumed ok */
            { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS },
            { NULL, NULL }
        };
#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)
    STBOOT_DCache_Area_t  Cache[] = { {(U32*)0x40200000, (U32*)0x5FFFFFFF},
                                      { NULL, NULL } };
#else
    STBOOT_DCache_Area_t  Cache[] = { {(U32*)0x40080000, (U32*)0x4FFFFFFF},
                                      { NULL, NULL } };
#endif

#ifdef VERBOSE
    STBOOT_Backend_t      Backend;
#endif

#ifndef STI2C_NO_TBX
    STTBX_InitParams_t    TBX_InitParams;
#endif

#ifdef ST_OS21
    /* Create memory partitions */
    system_partition_p   =  partition_create_heap((U8*)external_block, sizeof(external_block));
#else /* ifndef ST_OS21 */
    partition_init_heap (&the_internal_partition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                         sizeof(system_block));

    /* Avoid compiler warnings */
    internal_block_init[0] = 0;
    system_block_init[0] = 0;

#if defined(ST_5528) || defined(ST_5100)|| defined(ST_7710) || defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    data_section[0] = 0;
#endif

#ifdef UNIFIED_MEMORY

#if defined(ST_5516) || defined(ST_5517)
    ncache2_block[0] = 0;
#endif

#endif

#endif /* ST_OS21 */
#endif/*!ST_OSLINUX*/

#if defined(ST_5100)
    /* Configure Config Control Reg G for selecting ALT0 functions for PIO3<3:2> */
    /* 0000_0000_0000_0000_0000_0000_0000_0000 (INT_CONFIG_CONTROL_G)            */
    /* |||| |||| |||| |||| |||| |||| |||| ||||_____ pio2_altfop_mux0sel<7:0>     */
    /* |||| |||| |||| |||| |||| ||||_______________ pio2_altfop_mux1sel<7:0>     */
    /* |||| |||| |||| ||||_________________________ pio3_altfop_mux0sel<7:0>     */
    /* |||| ||||___________________________________ pio3_altfop_mux1sel<7:0>     */
    /* ========================================================================= */
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, STSYS_ReadRegDev32LE(CONFIG_CONTROL_G) | 0x000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
    /* Configure Config Control Reg H for selecting ALT1 functions for PIO4<3:2> */
    /*                0000_0000_0000_0000_1100 (INT_CONFIG_CONTROL_H)            */
    /*                |||| |||| |||| |||| ||||_____ pio4_altfop_mux0sel<3:0>     */
    /*                |||| |||| |||| ||||__________ Reserved<3:0>                */
    /*                |||| |||| ||||_______________ pio4_altfop_mux1sel<3:0>     */
    /*                |||| ||||____________________ Reserved<3:0>                */
    /* ========================================================================= */
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, STSYS_ReadRegDev32LE(CONFIG_CONTROL_H) |0x0000000f );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
#endif

#if defined(ST_5105) || defined(ST_5188) || defined(ST_5107)
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_G, ReadValue | 0x00000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_G);

    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_H, ReadValue |0x0c000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_H);
#endif

#if defined(ST_7710)
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_1);
    /* Writing to SSC0 & SSC1 Mux Selects Registers in COMMS GLUE */
    /* ========================================================== */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_1,  0x00000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_1);
#endif

#if defined(ST_5525)
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_1);
    /* Writing to SSC2 & SSC3 Mux Selects Registers in COMMS GLUE */
    /* ========================================================== */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_1,  0x00000000 );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_1);
    
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_2);
    /* Writing to SSC2 & SSC3 Mux Selects Registers in COMMS GLUE */
    /* ========================================================== */
    STSYS_WriteRegDev32LE(CONFIG_CONTROL_2,  0x0000000c );
    ReadValue = STSYS_ReadRegDev32LE(CONFIG_CONTROL_2);
    
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

    if ((Code = STBOOT_Init (Name, &InitParams)) != ST_NO_ERROR)
    {
        printf ("ERROR: STBOOT_Init returned code %d\n", Code);
    }
    else
    {
#ifdef VERBOSE
        if ((Code = STBOOT_GetBackendInfo (&Backend)) != ST_NO_ERROR)
        {
            printf ("ERROR: STBOOT_GetBackendInfo returned code %d\n", Code);
        }
        else
        {
            printf ("Backend Info: Devtype: %u  MajorRev: %u  MinorRev: %u\n",
                     Backend.DeviceType, Backend.MajorRevision,
                     Backend.MinorRevision);
        }
#endif

        TicksPerSec = ST_GetClocksPerSecond();
#ifndef STI2C_NO_TBX
#if defined(ST_OS21)
        TBX_InitParams.CPUPartition_p      = system_partition_p;
#else
        TBX_InitParams.CPUPartition_p      = &the_system_partition;
#endif
        TBX_InitParams.SupportedDevices    = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
        TBX_InitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;

        if ((Code = STTBX_Init( "TBX0", &TBX_InitParams )) != ST_NO_ERROR )
        {
            printf ("Error: STTBX_Init = 0x%08X\n", Code );
            return 0;
        }
#endif
#endif/*!ST_OSLINUX*/

        DoTheTests();
        PrintTestResults();
#if !defined(ST_OSLINUX)        
        STBOOT_Term (Name, &TermParams);
    }
#endif    
    return 0;
}

/*---------------------------------------------------------------------------*/
/* Other utility functions for checking various things.                      */
/*---------------------------------------------------------------------------*/

/* Return the error from the lookup table */
static char *I2C_ErrorString(ST_ErrorCode_t Error)
{
    I2C_ErrorMessage_t *mp = NULL;

    /* Search lookup table for error code*/
    mp = I2C_ErrorLUT;
    while (mp->Error != UNKNOWN_ERROR)
    {
        if (mp->Error == Error)
        {
            break;
        }
        mp++;
    }

    /* Return error message string */
    return mp->ErrorMsg;
}

static U32 IncrementErrorCount (void)
{
    ErrorCount [TestPhase]++;
    return ErrorCount [TestPhase];
}

static void PrintMessageNum (void)
{
    if ( (!MessageNumPrinted) && (MessageNumber != 0) )
    {
        myprintf ("\n**** Message %u ****\n", MessageNumber);
        MessageNumPrinted = TRUE;
    }
}


static void PrintError (char *msg, U32 v1, U32 v2)
{
    IncrementErrorCount();
    PrintMessageNum();
    myprintf ("ERROR: ");
    myprintf (msg, v1, v2);
    myprintf ("\n");
}


static BOOL CheckCode (char *msg,
                       ST_ErrorCode_t actualCode, ST_ErrorCode_t expectedCode)
{
    if (actualCode == expectedCode)
    {
        if (Verbose)
        {
             PrintMessageNum();
             myprintf ("%-30s: Got expected code %d\n", msg, expectedCode);
        }
        return TRUE;
    }
    else
    {
        PrintMessageNum();
        myprintf ("%-30s: ERROR: got code %s, expected %s\n", msg,
                  I2C_ErrorString(actualCode),
                  I2C_ErrorString(expectedCode) );
        IncrementErrorCount();
        return FALSE;
    }
}


static BOOL CheckCodeOk (char *msg, ST_ErrorCode_t actualCode)
{
    return CheckCode (msg, actualCode, ST_NO_ERROR);
}


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


static void PrintBuffer (U8 *Buffer, U32 Size, BOOL PrintFlag)
{
    /* Print contents of data buffer in hex, if PrintFlag is TRUE. */
    U32 i;
    char s[8], Line[128];

    if (PrintFlag)
    {
        Line[0] = 0;           /* empty string */
        for (i=0; i<Size; i++)
        {
            sprintf (s, "%02x ", Buffer[i]);
            strcat (Line, s);
            if ( (((i+1) % 16) == 0) || (i==Size-1) )
            {
                myprintf ("%s\n", Line);
                Line[0] = 0;
            }
        }
    }
}


/* Estimate timeout required for an I2C transfer of size TransferSize */
static U32 GetI2cTimeout (U32 TransferSize)
{
    /* TODO: Currently no consideration of clock speed, will need to
     * improve. Don't know where this formula originated from. -- PW
     */
    return (((TransferSize * 10000) / (STI2C_RATE_NORMAL / 40) + 2)*10);
}


static U32 MyRand (U32 Max)
{
    /* Return random number in range 0 .. Max-1 */
    return rand() % Max;
}


static void PrintTestResults (void)
{
    /* Print overall summary of test results */
    U32   TotalErrors=0, j;

    myprintf ("\nSummary of results for each test phase:\n");
    TotalErrors = ErrorCount [0];
    myprintf ("    Initialization - %3u errors\n", ErrorCount [0]);
    for (j=1; j<=TestPhase; j++)
    {
        myprintf ("    Phase %u -        %3u errors\n", j, ErrorCount [j]);
        TotalErrors += ErrorCount [j];
    }

    if (TotalErrors == 0)
    {
        myprintf ("\nOverall test result :  PASS\n");
    }
    else
    {
        myprintf ("\nOverall test result :  FAIL \n");
        myprintf ("A total of %u errors occurred\n", TotalErrors);
        myprintf ("Of these, at least %u were consistent with h/w bugs\n",
                  HwBugCount);
        if (TotalErrors == ErrorCount [4])
        {
            myprintf ("As all the errors occurred during Phase 4, it is likely that all failures\nwere caused by h/w bugs\n");
        }
    }
}

/*--------------------------------------------------------------------------*/
/* Callbacks                                                                */
/*--------------------------------------------------------------------------*/

void TxNotify (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    TxNotifyCount++;
    TxIndex = 0;
}

void RxNotify (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    RxNotifyCount++;
    RxIndex = 0;
}

void TxComplete (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    TxCompleteCount++;
}

void RxComplete (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    RxCompleteCount++;
}

void TxByte (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    if (TxNotifyCount == 1)
    {
        *(U8*)EventData = MAGIC_RESPONSE;   /* No message to reply to */
    }
    else
    {
        if (TxIndex==0)    /* 1st byte of response is last length recvd */
        {
            *(U8*)EventData = (U8) RxIndex;
        }
        else
        {
            *(U8*)EventData = (U8) (RxBuffer [TxIndex-1] + 1);
        }
        TxIndex++;
        TxByteCount++;
    }
}

void RxByte (STEVT_CallReason_t Reason, const ST_DeviceName_t RegistrantName,
               STEVT_EventConstant_t Event, const void *EventData,
               const void *SubscriberData_p)
{
    U8  Data = *(U8*)EventData;

    RxBuffer [RxIndex++] = Data;
    RxByteCount++;
}

/*--------------------------------------------------------------------------*/
/*  Event Handler initialisation                                            */
/*--------------------------------------------------------------------------*/

static BOOL EVT_Init (ST_DeviceName_t EVTDeviceName)
{
    STEVT_InitParams_t      EVTInitParams;

    EVTInitParams.ConnectMaxNum   = 10;
    EVTInitParams.EventMaxNum     = 40;
    EVTInitParams.SubscrMaxNum    = 40;
#if defined(ST_OS21)
    EVTInitParams.MemoryPartition =  system_partition_p;
#else
    EVTInitParams.MemoryPartition = TEST_PARTITION_1;
#endif
    return CheckCodeOk ("Initialising Event Handler",
                         STEVT_Init( EVTDeviceName, &EVTInitParams ) );
}


static void EVT_Open (ST_DeviceName_t EVTDeviceName, STEVT_Handle_t *pHandle)
{
    STEVT_OpenParams_t      EVTOpenParams;
    STEVT_DeviceSubscribeParams_t   TxNotifySub,   RxNotifySub,
                                    TxByteSub, RxByteSub,
                                    TxCompleteSub, RxCompleteSub;

    TxNotifySub.NotifyCallback     = TxNotify;
    TxNotifySub.SubscriberData_p   = NULL;
    RxNotifySub.NotifyCallback     = RxNotify;
    RxNotifySub.SubscriberData_p   = NULL;
    TxByteSub.NotifyCallback       = TxByte;
    TxByteSub.SubscriberData_p     = NULL;
    RxByteSub.NotifyCallback       = RxByte;
    RxByteSub.SubscriberData_p     = NULL;
    TxCompleteSub.NotifyCallback   = TxComplete;
    TxCompleteSub.SubscriberData_p = NULL;
    RxCompleteSub.NotifyCallback   = RxComplete;

    RxCompleteSub.SubscriberData_p = NULL;

    CheckCodeOk ("Open Event handler",
                 STEVT_Open (EVTDevName0, &EVTOpenParams, pHandle) );

    CheckCodeOk ("Subscribe TxNotify",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                   STI2C_TRANSMIT_NOTIFY_EVT,
                                   &TxNotifySub) );

    CheckCodeOk ("Subscribe RxNotify",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                   STI2C_RECEIVE_NOTIFY_EVT,
                                   &RxNotifySub) );

    CheckCodeOk ("Subscribe TxByte",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                   STI2C_TRANSMIT_BYTE_EVT,
                                   &TxByteSub) );

    CheckCodeOk ("Subscribe RxByte",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                   STI2C_RECEIVE_BYTE_EVT,
                                   &RxByteSub) );

    CheckCodeOk ("Subscribe TxComplete",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                   STI2C_TRANSMIT_COMPLETE_EVT,
                                   &TxCompleteSub) );

    CheckCodeOk ("Subscribe RxComplete",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-0",
                                   STI2C_RECEIVE_COMPLETE_EVT,
                                   &RxCompleteSub) );
#if defined (USE_ONE_BOARD)
    CheckCodeOk ("Subscribe TxNotify",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-1",
                                   STI2C_TRANSMIT_NOTIFY_EVT,
                                   &TxNotifySub) );

    CheckCodeOk ("Subscribe RxNotify",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-1",
                                   STI2C_RECEIVE_NOTIFY_EVT,
                                   &RxNotifySub) );

    CheckCodeOk ("Subscribe TxByte",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-1",
                                   STI2C_TRANSMIT_BYTE_EVT,
                                   &TxByteSub) );

    CheckCodeOk ("Subscribe RxByte",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-1",
                                   STI2C_RECEIVE_BYTE_EVT,
                                   &RxByteSub) );

    CheckCodeOk ("Subscribe TxComplete",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-1",
                                   STI2C_TRANSMIT_COMPLETE_EVT,
                                   &TxCompleteSub) );

    CheckCodeOk ("Subscribe RxComplete",
                  STEVT_SubscribeDeviceEvent (*pHandle, "I2C-1",
                                   STI2C_RECEIVE_COMPLETE_EVT,
                                   &RxCompleteSub) );
#endif
}

/*--------------------------------------------------------------------------*/
/*  Test subroutines                                                        */
/*--------------------------------------------------------------------------*/

static BOOL WaitForResponse (void)
{
    /* Synchronize with other processor, by repeatedly reading a single byte
       message from it. When a specific byte is successfully read, it can be
       safely assumed that the other processor is also at a synchronization
       point. This function is called on startup & at the end of each test. */

    ST_ErrorCode_t     Code;
    U32                ActLen, WaitCount=0;
    U8                 Byte;

    myprintf ("Synchronizing with other processor....\n");

    while (1)
    {
        Byte = 0;

        Code = STI2C_Read (MasterHandle, &Byte, 1, 100, &ActLen);
        MasterFirst = (TxNotifyCount == 0);
        myprintf("Code = %s, Length = %d, Data = 0x%02x\n",
                    I2C_ErrorString(Code), ActLen, Byte);
#if defined(STI2C_DEBUG)
        dumptrace(NULL);
#endif
#if defined(STI2C_TRACE)
        if (Code != ST_NO_ERROR)
        {
            QueryTrace();
        }
#endif

        STOS_TaskDelay (TicksPerSec * 2);
        if ( (Code == ST_NO_ERROR) && (Byte == MAGIC_RESPONSE) )
        {
            myprintf ("\n*** Synchronization successful ***\n\n");
            return TRUE;
        }
        else
        {
            myprintf (".");
            PRINTV("%s\n", I2C_ErrorString(Code));
        }

        if (++WaitCount > 30)
        {
            IncrementErrorCount();
            myprintf ("\nERROR: Giving up waiting for response\n");
            return FALSE;
        }
    }

    return FALSE;   /* should never reach here */
}


static BOOL SendMessagesAndCheckResponses (void)
{
    /* Writes messages to the other processor of random size and content,
       then reads and checks a response to each. */

    ST_ErrorCode_t       Code;
    BOOL                 ErrorThisTime;
    U32                  ActLen, WriteLength, ReadLength;
    S32                  j;
    U8                   WriteBuffer[1024], ReadBuffer[1024];
    I2cFunc_t            WriteFunc, ReadFunc;

    myprintf ("Writing & Reading %u messages to other processor...\n",
              MESSAGE_COUNT);

    if (NoStopTest)    /* Read/WriteNoStop test? */
    {
        WriteFunc = (I2cFunc_t) STI2C_WriteNoStop;
        ReadFunc  = STI2C_ReadNoStop;
    }
    else
    {
        WriteFunc = (I2cFunc_t) STI2C_Write;
        ReadFunc  = STI2C_Read;
    }

    for (MessageNumber = 1; MessageNumber <= MESSAGE_COUNT; MessageNumber++)
    {
        MessageNumPrinted = FALSE;
        if (Verbose)
        {
            PrintMessageNum();
        }
        ErrorThisTime = FALSE;
        ActLen = 0;

        /* Setup message of random length & contents & send to other device */
        WriteLength = 1 + MyRand(3);
        for (j=0; j<WriteLength; j++)
        {
            WriteBuffer[j] = (U8) MyRand(256);
            ReadBuffer[j]  = 0;
        }

        PRINTV ("Sending %u bytes\n", WriteLength);
        Code = WriteFunc (MasterHandle, WriteBuffer, WriteLength,
                          GetI2cTimeout (WriteLength), &ActLen);
        if ( CheckCodeOk ("I2C Write", Code ) )
        {
            MasterTxCount += ActLen;
            if (ActLen != WriteLength)
            {
                PrintError ("Write length = %u, actual length = %u",
                            WriteLength, ActLen);
#if defined(STI2C_TRACE)
                QueryTrace();
#endif
            }
            else   /* length written ok */
            {
                /* Now read response of random length & check it.
                   Byte 0 should contain length of last message sent to it.
                   Bytes N should contain value of byte(N-1) + 1.  */

                ActLen = 0;
                ReadLength = 1 + MyRand (WriteLength);
                if (MessageNumber == MESSAGE_COUNT)
                {
                    ReadFunc = STI2C_Read;   /* Last transfer must do a Stop */
                }
                Code = ReadFunc (MasterHandle, ReadBuffer, ReadLength,
                                 GetI2cTimeout (ReadLength), &ActLen);
                if (CheckCodeOk ("I2C Read", Code))
                {
                    MasterRxCount += ActLen;
                    PRINTV ("Read %u bytes\n", ActLen);
                    if (ActLen != ReadLength)
                    {
                        ErrorThisTime = TRUE;
                        PrintError ("Read length = %u, actual length = %u",
                                     ReadLength, ActLen);
#if defined(STI2C_TRACE)
                        QueryTrace();
#endif
                    }
                    /* Check length byte */
                    else if (ReadBuffer[0] != (U8)WriteLength)
                    {
                        ErrorThisTime = TRUE;
                        PrintError ("Byte 0 contains incorrect length! (%u)",
                                     (U32) ReadBuffer[0], 0);
                        if ( ((ReadBuffer[0] & 0xFE) == SLAVE_DEVICE_ID) ||
                             (ReadBuffer[0] == 0xFF) )
                        {
                            HwBugCount++;
                            myprintf ("(Failure consistent with h/w bug)\n");
                        }
#if defined(STI2C_TRACE)
                        QueryTrace();
#endif
                    }
                    else     /* Check other bytes received */
                    {
                        for (j=1; j<ReadLength; j++)
                        {
                            if (ReadBuffer[j] != (U8) (WriteBuffer[j-1]+1))
                            {
                                ErrorThisTime = TRUE;
                                PrintError ("Byte %u of data incorrect", j, 0);
                                if (ReadBuffer[j] == 0xFF)
                                {
                                    HwBugCount++;
                                    myprintf ("(Failure consistent with h/w bug)\n");
                                }
#if defined(STI2C_TRACE)
                                    QueryTrace();
#endif
                                break;
                            }
                        }

                        if (j>=ReadLength)
                        {
                            PRINTV ("*** Message %u matches ***\n",
                                    MessageNumber);
                        }
                    }
                }
#if defined(STI2C_TRACE)
                else
                {
                    QueryTrace();
                }
#endif
            }

            if (Verbose || ErrorThisTime)
            {
                myprintf ("Data written:\n");
                PrintBuffer (WriteBuffer, WriteLength, TRUE);
                myprintf ("Data read:\n");
                PrintBuffer (ReadBuffer, ActLen, TRUE);
            }
        }
#if defined(STI2C_TRACE)
        else
        {
            QueryTrace();
        }
#endif

        /* If 'Bus in Use' error, attempt recovery */
        if ( (Code == STI2C_ERROR_BUS_IN_USE) ||
             (Code == STI2C_ERROR_ADDRESS_ACK) )
        {
            HwBugCount++;
            myprintf ("(Failure consistent with h/w bug, attempting recovery...\n");
            STI2C_Write (MasterHandle, WriteBuffer, 0, 1, &ActLen);
            STOS_TaskDelay (20);
#if !defined(ST_OSLINUX)            
            STI2C_Reset (MasterHandle);
            STOS_TaskDelay (20);
#endif            
            myprintf ("Recovery sequence completed)\n");
        }

        if (ErrorCount [TestPhase] >= ERROR_ABORT_THRESHOLD)
        {
            myprintf ("\n*** Too many errors, aborting ***\n\n");
            return FALSE;
        }

        /* Random delay between iterations */
        STOS_TaskDelay (20 + MyRand(100));
    }

    return TRUE;
}

/* Not currently used on the one-board-only tests */
#if !defined(USE_ONE_BOARD)
static BOOL SendMessageAndCheckBusy (void)
{
    /* Writes messages to the other processor, & check that 'Bus In Use'
       occurs for each. This function is called when the other processor is
       doing read/writes with no Stop, so that the I2C bus should be
       permanently in use. */
    /* NumWrites was originally Count/10, but the other board seems to have
       a habit of finishing its writes faster than we do - this means that
       the transfers succeed and the test fails, when it need not. */

    ST_ErrorCode_t       Code;
    const U32            NumWrites = MESSAGE_COUNT/12;
    U32                  j, ActLen, WriteLength;
    U8                   WriteBuffer[1024];

    STOS_TaskDelay (TicksPerSec*2);
    myprintf ("Writing %u messages to other processor...\n",
              NumWrites);

    for (MessageNumber=1; MessageNumber<=NumWrites; MessageNumber++)
    {
        MessageNumPrinted = FALSE;
        ActLen = 0;

        /* Setup message of random length & contents & send to other device */
        WriteLength = 1 + MyRand(64);
        for (j=0; j<WriteLength; j++)
        {
            WriteBuffer[j] = (U8) MyRand(256);
        }

        PRINTV ("Sending %u bytes\n", WriteLength);

        Code = STI2C_Write (MasterHandle, WriteBuffer, WriteLength,
                            GetI2cTimeout (WriteLength), &ActLen);
        CheckCode ("I2C Write", Code, STI2C_ERROR_BUS_IN_USE );

        if (ErrorCount [TestPhase] >= ERROR_ABORT_THRESHOLD)
        {
            myprintf ("\n*** Too many errors, aborting ***\n\n");
            return FALSE;
        }

        /* Random delay between iterations */
        STOS_TaskDelay (MyRand(100));
    }

    myprintf ("Finished\n");
    return TRUE;
}
#endif


/**************************************************************************
       THIS FUNCTION WILL CALL ALL THE TEST ROUTINES...
**************************************************************************/


static void DoTheTests (void)
{
    /* Declarations ---------------------------------------------------------*/

    ST_DeviceName_t      DevName;
    STI2C_InitParams_t   I2cInitParams;
    STI2C_TermParams_t   I2cTermParams;
#if !defined(ST_OSLINUX)
    STPIO_InitParams_t   PIOInitParams;
#endif    
    STI2C_OpenParams_t   I2cOpenParams;
    STEVT_Handle_t       EVTHandle;
    U32                  j;
    ST_ClockInfo_t       ClockInfo;
#if !defined(USE_ONE_BOARD)
    BOOL                 MasterProcessor;
#endif

    ST_GetClockInfo(&ClockInfo);

    /* Initialisation ------------------------------------------------------*/

    strcpy( DevName, "I2C-0" );
    I2cInitParams.BaseAddress           = (U32*)SSC0_BASE_ADDRESS;
    I2cInitParams.InterruptNumber       = SSC0_INTERRUPT;
    I2cInitParams.InterruptLevel        = SSC0_INTERRUPT_LEVEL;
    I2cInitParams.BaudRate              = STI2C_RATE_NORMAL;
    I2cInitParams.MasterSlave           = STI2C_MASTER_OR_SLAVE;
    I2cInitParams.ClockFrequency        = ClockInfo.CommsBlock;
    I2cInitParams.PIOforSDA.BitMask     = PIO_FOR_SSC0_SDA;
    I2cInitParams.PIOforSCL.BitMask     = PIO_FOR_SSC0_SCL;
    I2cInitParams.MaxHandles            = 4;
    I2cInitParams.GlitchWidth           = 0;
#if defined(ST_OS21)
    I2cInitParams.DriverPartition       = system_partition_p;
#else
    I2cInitParams.DriverPartition       = TEST_PARTITION_1;
#endif

#ifdef TEST_FIFO
    I2cInitParams.FifoEnabled           = TRUE;
#else
    I2cInitParams.FifoEnabled           = FALSE;
#endif
    strcpy( I2cInitParams.PIOforSDA.PortName, "PIODEV_A" );
    strcpy( I2cInitParams.PIOforSCL.PortName, "PIODEV_A" );
    I2cInitParams.SlaveAddress          = SLAVE_DEVICE_ID;
    strcpy( I2cInitParams.EvtHandlerName, EVTDevName0);
    
#if !defined(ST_OSLINUX)
    PIOInitParams.BaseAddress           = (U32*)SSC0_PIO_BASE;
    PIOInitParams.InterruptNumber       = SSC0_PIO_INTERRUPT;
    PIOInitParams.InterruptLevel        = SSC0_PIO_INTERRUPT_LEVEL;
#if defined(ST_OS21)
    PIOInitParams.DriverPartition       = system_partition_p;
#else
    PIOInitParams.DriverPartition       = TEST_PARTITION_1;
#endif
#endif

    I2cOpenParams.BusAccessTimeOut      = 20;
    I2cOpenParams.AddressType           = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.I2cAddress            = SLAVE_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams.BaudRate              = STI2C_RATE_NORMAL;
#endif    

#if defined(USE_ONE_BOARD)
    /* I am device 0x08 */
    I2cInitParams.SlaveAddress          = 0x08;
    /* Talk to the device on address 0x10 */
#if  defined(ST_5100)||defined(ST_7710) || defined(ST_5105)||defined(ST_7100) ||\
defined(ST_5301) || defined(ST_8010)|| defined(ST_7109) || defined(ST_5188) || defined(ST_5107)\
|| defined(ST_7200) 
    /* On mb390 there is some address conflict on 0x10 address*/
    I2cOpenParams.I2cAddress            = 0x42;
#else
    I2cOpenParams.I2cAddress            = 0x10;
#endif

#endif

    I2cTermParams.ForceTerminate        = TRUE;

    /* Program --------------------------------------------------------------*/
#ifdef TEST_FIFO
    myprintf ("\nI2C DRIVER TEST SUITE - MASTER/SLAVE USING FIFO\n\n");
#if (!defined(ST_5100)) && (!defined(ST_7710)) && (!defined(ST_5105)) && (!defined(ST_7100))&& \
    (!defined(ST_5301)) && (!defined(ST_8010)) && (!defined(ST_7109)) && (!defined(ST_5188)) && \
    (!defined(ST_5107)) && (!defined(ST_7200))
    myprintf(("\nFIFO Feature is not supported\n\n"));
    return;
#else
    myprintf ("\nI2C DRIVER TEST SUITE - MASTER/SLAVE\n\n");
#endif
#endif

    srand (STOS_time_now());
    for (j=0; j<MAX_PHASE; j++)
    {
        ErrorCount[j] = 0;
    }
#if !defined(ST_OSLINUX)
    /* Set up the PIO */
    CheckCodeOk ("Initializing PIO", STPIO_Init( "PIODEV_A", &PIOInitParams ));
#endif
    /* Set up Event Handler */
    EVT_Init (EVTDevName0);

    myprintf("SSC initialized at %i baud\n", I2cInitParams.BaudRate);

    /* Initialized SSC0 */
    {
        char TmpString[80];

        myprintf("Initialising SSC0\n");
        sprintf(TmpString, "Initializing %s", DevName);
        CheckCodeOk( TmpString, STI2C_Init (DevName, &I2cInitParams) );
        sprintf(TmpString, "Opening %s", DevName);
        CheckCodeOk( TmpString, STI2C_Open (DevName, &I2cOpenParams,  &SSC0Handle) );
        MasterHandle = SSC0Handle;
    }

#if defined (USE_ONE_BOARD)
    /*
    ** Need to initialize SSC1 as well
    */

    strcpy( DevName, "I2C-1" );
    I2cInitParams.BaseAddress           = (U32*)SSC1_BASE_ADDRESS;
    I2cInitParams.InterruptNumber       = SSC1_INTERRUPT;
    I2cInitParams.InterruptLevel        = SSC1_INTERRUPT_LEVEL;
    I2cInitParams.PIOforSDA.BitMask     = PIO_FOR_SSC1_SDA;
    I2cInitParams.PIOforSCL.BitMask     = PIO_FOR_SSC1_SCL;
    I2cInitParams.SlaveAddress          = SLAVE_DEVICE_ID;
#if !defined(ST_OSLINUX)
    PIOInitParams.BaseAddress           = (U32*)SSC1_PIO_BASE;
    PIOInitParams.InterruptNumber       = SSC1_PIO_INTERRUPT;

    PIOInitParams.InterruptLevel        = SSC1_PIO_INTERRUPT_LEVEL;
#if defined(ST_OS21)
    PIOInitParams.DriverPartition       = system_partition_p;
#else
    PIOInitParams.DriverPartition       = TEST_PARTITION_1;
#endif
#endif
#ifdef TEST_FIFO
    I2cInitParams.FifoEnabled           = TRUE;
#else
    I2cInitParams.FifoEnabled           = FALSE;
#endif
    I2cOpenParams.BusAccessTimeOut      = 20;
    I2cOpenParams.AddressType           = STI2C_ADDRESS_7_BITS;
    I2cOpenParams.I2cAddress            = SLAVE_DEVICE_ID;
#ifdef STI2C_ADAPTIVE_BAUDRATE
    I2cOpenParams.BaudRate              = STI2C_RATE_NORMAL;
#endif  
    /* I am device 0x10 or 0x20 */
#if  defined(ST_5100)||defined(ST_7710)||defined(ST_5105)||defined(ST_7100)||\
defined(ST_5301) || defined(ST_8010)  ||defined(ST_7109) || defined(ST_5188) || \
defined(ST_5107) || defined(ST_7200)
    /* On mb390 there is some address conflict on 0x10 address*/
    I2cInitParams.SlaveAddress           = 0x42;
#else
    I2cInitParams.SlaveAddress           = 0x10;
#endif
    /* Talk to the device on address 0x08 */
    I2cOpenParams.I2cAddress            = 0x08;

    /* Set up the PIO */
    if (SSC1_PIO_BASE != SSC0_PIO_BASE)
    {
#if !defined(ST_OSLINUX)        
        CheckCodeOk ("Initializing PIO", STPIO_Init( "PIODEV_B",
                     &PIOInitParams));
#endif                     
        strcpy( I2cInitParams.PIOforSDA.PortName, "PIODEV_B" );
        strcpy( I2cInitParams.PIOforSCL.PortName, "PIODEV_B" );
    }

    myprintf("Initialising SSC1\n");
    /* Initialize and Open SSC1 */
    {
        char TmpString[80];

        sprintf(TmpString, "Initializing %s", DevName);
        CheckCodeOk( TmpString, STI2C_Init (DevName, &I2cInitParams) );
        sprintf(TmpString, "Opening %s", DevName);
        CheckCodeOk( TmpString, STI2C_Open (DevName, &I2cOpenParams,  &SSC1Handle) );
    }

    SlaveHandle = SSC1Handle;
#endif

    EVT_Open (EVTDevName0, &EVTHandle);

    /* Abort if any errors during initialisation (phase 0) */
    if (ErrorCount[0] > 0)
    {
        return;
    }
    TestPhase = 1;
#if defined(USE_ONE_BOARD)
    {
        /* Swap master/slave - ie start off with SSC1 as master */
        STI2C_Handle_t TmpHandle = MasterHandle;
        MasterHandle = SlaveHandle;
        SlaveHandle = TmpHandle;
    }
#endif

    if (!WaitForResponse())
    {
        return;
    }
    /* Got initial response, start tests */

    myprintf ("******** PHASE 1 : Master writes & reads to passive Slave\n\n");
#if defined (USE_ONE_BOARD)
    myprintf("%s will be master first\n",
             (MasterHandle == SSC0Handle ) ? "SSC0" : "SSC1");

    if (!SendMessagesAndCheckResponses())
    {
        return;
    }
#else
    MasterProcessor = MasterFirst;
    myprintf ("This processor will be %s first\n",
              (MasterProcessor ? "Master" : "Slave") );
    if (MasterProcessor)
    {
        if (!SendMessagesAndCheckResponses())
        {
            return;
        }
    }
    else
    {
        STOS_TaskDelay (TicksPerSec * ((18*MESSAGE_COUNT)/1000) );
    }
#endif

    myprintf ("Test phase 1 complete, %u errors detected\n\n",
              ErrorCount [TestPhase]);
    TxNotifyCount = 0;

#if defined(STI2C_TRACE)
    QueryTrace();
#endif


#if defined (USE_ONE_BOARD)
    {
        /* Swap master/slave */
        STI2C_Handle_t TmpHandle = MasterHandle;
        MasterHandle = SlaveHandle;
        SlaveHandle = TmpHandle;
    }
#endif

    STOS_TaskDelay(ST_GetClocksPerSecond());

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse())
    {
        return;
    }

    myprintf ("******** PHASE 2 : Ditto, Master & Slave switched\n\n");
    TestPhase = 2;
#if defined (USE_ONE_BOARD)
    myprintf("%s will be master now\n",
             (MasterHandle == SSC0Handle ) ? "SSC0" : "SSC1");

    if (!SendMessagesAndCheckResponses())
    {
        return;
    }
#else
    MasterProcessor = (MasterProcessor ? FALSE : TRUE);
    myprintf ("This processor will now be %s\n",
              (MasterProcessor ? "Master" : "Slave") );
    if (MasterProcessor)
    {
        if (!SendMessagesAndCheckResponses())
        {
            return;
        }
    }
    else
    {
        STOS_TaskDelay (TicksPerSec * ((15*MESSAGE_COUNT)/1000) );
    }
#endif
    myprintf ("Test phase 2 complete, %u errors detected\n\n",
              ErrorCount [TestPhase]);
    TxNotifyCount = 0;

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse())
    {
        return;
    }

#if !defined(USE_ONE_BOARD)
    myprintf ("******** PHASE 3 : Bus In Use\n\n");
    TestPhase = 3;
    NoStopTest = TRUE;
    MasterProcessor = (MasterProcessor ? FALSE : TRUE);
    if (MasterProcessor)
    {
        myprintf ("This processor will use 'No Stop' transfers\n");
        if (!SendMessagesAndCheckResponses())
        {
            return;
        }
    }
    else
    {
        myprintf ("This processor will use normal transfers\n");
        if (!SendMessageAndCheckBusy())
        {
            return;
        }
        STOS_TaskDelay (TicksPerSec*15);
    }

    myprintf ("Test phase 3 complete, %u errors detected\n\n",
              ErrorCount [TestPhase]);
    TxNotifyCount = 0;
    NoStopTest = FALSE;

    /* Synchronize with other processor ready for next phase */
    if (!WaitForResponse())
    {
        return;
    }
    myprintf ("******** PHASE 4 : Both processors write & read simultaneously\n\n");
    TestPhase = 4;
    if (!SendMessagesAndCheckResponses())
    {
        return;
    }
    myprintf ("Test phase 4 complete, %u errors detected\n",
              ErrorCount [TestPhase]);
#endif

#if !defined (USE_ONE_BOARD)
    myprintf ("\nTests completed - pausing to allow other side to complete\n");
    STOS_TaskDelay (TicksPerSec * 5);
#endif

    CheckCodeOk ("Terminating I2C", STI2C_Term( "I2C-0", &I2cTermParams ) );
}
/* EOF */

