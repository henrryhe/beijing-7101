/****************************************************************************

File Name   : pio_test.c

Description : BOOT from FLASH test

Copyright (C) 2005, ST Microelectronics

History     :

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
#include "stddefs.h"
#include "stflash.h"
#include "stboot.h"
#include "sttbx.h"
#include "stcommon.h"
#ifdef USE_TESTTOOL
#include "testtool.h"
#endif

#include "uart.h"
#include "trace.h"



enum 
{
    PIO_DEVICE_0 = 0,
    PIO_DEVICE_1,
    PIO_DEVICE_2,
    PIO_DEVICE_3,
    PIO_DEVICE_4,
    PIO_DEVICE_5,
    PIO_DEVICE_6,
    PIO_DEVICE_7,
    PIO_DEVICE_NOT_USED
};

#if defined(ST_5100)
#define PIO_BASE_ADDRESS        PIO_4_BASE_ADDRESS
#define PIO_INTERRUPT           PIO_4_INTERRUPT
#define PIO_INTERRUPT_LEVEL     PIO_4_INTERRUPT_LEVEL
#define PIO 4

#elif defined(ST_7710) || defined(ST_5528)
#define PIO_BASE_ADDRESS        PIO_5_BASE_ADDRESS
#define PIO_INTERRUPT           PIO_5_INTERRUPT
#define PIO_INTERRUPT_LEVEL     PIO_5_INTERRUPT_LEVEL
#define PIO 5

#elif defined(ST_5105)
#define PIO_BASE_ADDRESS        PIO_2_BASE_ADDRESS
#define PIO_INTERRUPT           PIO_2_INTERRUPT
#define PIO_INTERRUPT_LEVEL     PIO_2_INTERRUPT_LEVEL
#define PIO 2
#endif 

ST_DeviceName_t PIO_DeviceName[] = {"PIO0","PIO1","PIO2","PIO3","PIO4","PIO5","PIO6","PIO7"};

/* Semaphores */
#if defined(ST_OS21)
static semaphore_t *CompareSemaphore_p;
#else
static semaphore_t CompareSemaphore;
static semaphore_t *CompareSemaphore_p;
#endif


/* Compare function test */

#if defined(ST_5514)
#define COMPARE_TEST_PIO             PIO_DEVICE_5

#elif defined(ST_5100) || defined(ST_7710) || defined(ST_5105)
#define COMPARE_TEST_PIO             PIO_DEVICE_2

#else
/* ST_5516 || ST_5517 || ST_5528 */
#define COMPARE_TEST_PIO             PIO_DEVICE_4

#endif

#if defined(ST_5514) || defined(ST_5100)
#define COMPARE_TEST_BITMASK    PIO_BIT_4

#elif defined(ST_5105) 
#define COMPARE_TEST_BITMASK    PIO_BIT_0

#elif defined(ST_5510) || defined(ST_5512)
#define COMPARE_TEST_BITMASK    PIO_BIT_7

#elif defined(ST_7710)
#define COMPARE_TEST_BITMASK    PIO_BIT_2

#else
/* ST_5516 || ST_5517 || ST_5528 */
#define COMPARE_TEST_BITMASK    PIO_BIT_3

#endif

#if defined(ST_5528)
#define COMPARE_PIO_BASE_ADDRESS     PIO_4_BASE_ADDRESS
#define COMPARE_PIO_INTERRUPT_LEVEL  PIO_4_INTERRUPT_LEVEL
#define COMPARE_PIO_INTERRUPT        PIO_4_INTERRUPT
#define COMPARE_PIO                  4

#elif defined(ST_5100) || defined(ST_7710) || defined(ST_5105)
#define COMPARE_PIO_BASE_ADDRESS     PIO_2_BASE_ADDRESS
#define COMPARE_PIO_INTERRUPT_LEVEL  PIO_2_INTERRUPT_LEVEL
#define COMPARE_PIO_INTERRUPT        PIO_2_INTERRUPT
#define COMPARE_PIO                  2

#endif


static void CompareNotifyFunction(STPIO_Handle_t Handle,
                                  STPIO_BitMask_t ActiveBits);
static void CompareClearNotifyFunction(STPIO_Handle_t Handle,
                                  STPIO_BitMask_t ActiveBits);

ST_ErrorCode_t CompareTest(void);
/* Private Constants ------------------------------------------------------ */


/* Declarations for memory partitions */
#ifndef ST_OS21

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            0x100000

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

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5528) || defined(ST_5105)
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

static ST_ErrorCode_t       error;

/* Clock info structure setup here (for global use)  */
ST_ClockInfo_t ST_ClockInfo;

/* Suppress linker warning */
#ifdef ARCHITECTURE_ST20
#pragma ST_section      ( error,   "ncache_section")
#endif

const U8 HarnessRev[] = "2.4.1";


/* Private Macros --------------------------------------------------------- */
/* Private Function prototypes -------------------------------------------- */

void os20_main( void *ptr);

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

    STTBX_InitParams_t      sttbx_InitPars;
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    STUART_InitParams_t     stuart_InitParams;
    STUART_Params_t         stuart_Params;
    STPIO_InitParams_t      stpio_InitParams;

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

#if defined(ST_5100) || defined(ST_7710) || defined(ST_5528) || defined(ST_5105)
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


    /* Get clock info structure - for global use  */
    (void) ST_GetClockInfo(&ST_ClockInfo);

    /* Initialise PIO */
    stpio_InitParams.DriverPartition = system_partition;
    stpio_InitParams.BaseAddress      = (U32*)PIO_BASE_ADDRESS;
    stpio_InitParams.InterruptLevel   = PIO_INTERRUPT_LEVEL;
    stpio_InitParams.InterruptNumber  = PIO_INTERRUPT;
   
    /* Initialize the uart */
    stuart_InitParams.DriverPartition = system_partition;
    stuart_InitParams.ClockFrequency  = ST_ClockInfo.CommsBlock;
    stuart_InitParams.SwFIFOEnable    = TRUE;
    stuart_InitParams.FIFOLength      = 256;
    stuart_InitParams.RXD.PortName[0] = '\0';
    stuart_InitParams.TXD.PortName[0] = '\0';
    stuart_InitParams.RTS.PortName[0] = '\0';
    stuart_InitParams.CTS.PortName[0] = '\0';
    
     /* set default values */
     stuart_Params.RxMode.BaudRate       = 115200;
     stuart_Params.RxMode.DataBits       = STUART_8BITS_NO_PARITY;
     stuart_Params.RxMode.StopBits       = STUART_STOP_1_0;
     stuart_Params.RxMode.FlowControl    = STUART_NO_FLOW_CONTROL;
     stuart_Params.RxMode.TermString     = NULL;
     stuart_Params.RxMode.TimeOut        = 1;        /* As short as possible */
     stuart_Params.RxMode.NotifyFunction = NULL;     /* No callback */

     stuart_Params.TxMode.BaudRate       = 115200;
     stuart_Params.TxMode.DataBits       = STUART_8BITS_NO_PARITY;
     stuart_Params.TxMode.StopBits       = STUART_STOP_1_0;
     stuart_Params.TxMode.FlowControl    = STUART_NO_FLOW_CONTROL;
     stuart_Params.TxMode.TermString     = NULL;
     stuart_Params.TxMode.TimeOut        = 0;        /* No time-out */
     stuart_Params.TxMode.NotifyFunction = NULL;     /* No callback */

     stuart_Params.SmartCardModeEnabled  = FALSE;
     stuart_Params.GuardTime             = 0;
     stuart_Params.NACKSignalDisabled    = FALSE;
     stuart_Params.HWFifoDisabled        = FALSE;

     /* Initialize DATA Uart */
     stuart_InitParams.UARTType         = ASC_DEVICE_TYPE;
     stuart_InitParams.DefaultParams    = &stuart_Params;
     stuart_InitParams.BaseAddress      = (U32 *)DATA_UART_BASE_ADDRESS;
     stuart_InitParams.InterruptNumber  = DATA_UART_INTERRUPT;
     stuart_InitParams.InterruptLevel   = DATA_UART_INTERRUPT_LEVEL;

     stuart_InitParams.RXD.BitMask      = DATA_UART_RXD_BIT;
     stuart_InitParams.TXD.BitMask      = DATA_UART_TXD_BIT;
     stuart_InitParams.RTS.BitMask      = DATA_UART_RTS_BIT;
     stuart_InitParams.CTS.BitMask      = DATA_UART_CTS_BIT;

     strcpy(stuart_InitParams.RXD.PortName, PIO_DeviceName[DATA_UART_RXD_DEV]);
     strcpy(stuart_InitParams.TXD.PortName, PIO_DeviceName[DATA_UART_TXD_DEV]);
     strcpy(stuart_InitParams.RTS.PortName, PIO_DeviceName[DATA_UART_RTS_DEV]);
     strcpy(stuart_InitParams.CTS.PortName, PIO_DeviceName[DATA_UART_CTS_DEV]);

    /* Initialize the toolbox */
#ifdef ST_OS21
    sttbx_InitPars.CPUPartition_p      = system_partition;
#else
    sttbx_InitPars.CPUPartition_p      = TEST_PARTITION_1;
#endif

    sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_UART;
    sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_UART;
    sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_UART;
    strcpy(sttbx_InitPars.UartDeviceName, "asc");

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

    printf ("\nStarting BOOT ...\n");
    error = STBOOT_Init( "FLASHTest", &stboot_InitPars );
    if (error != ST_NO_ERROR)
    {
        printf( "ERROR: STBOOT_Init() returned %d\n", error );
    }
    else
    {   
    	
    	printf ("\nStarting PIO ...\n");
        error = STPIO_Init( PIO_DeviceName[PIO], &stpio_InitParams );
        
        printf ("\nStarting UART ...\n");
        error = STUART_Init("asc", &stuart_InitParams);
        if (error != ST_NO_ERROR)
        {
            printf( "ERROR: STUART_Init() returned %d\n", error );
        }
    		
        printf ("\nStarting TBX ...\n");
        error = STTBX_Init( "tbx", &sttbx_InitPars );
        if (error != ST_NO_ERROR)
        {
            printf( "ERROR: STTBX_Init() returned %d\n", error );
        }
        else
        {        	
            if (TraceInit())
            {
                /* Just display an error, but carry on - it is not that serious */
                STFLASH_Print(("TraceInit failed\n"));
            }
           
            STFLASH_Print(("============================================================\n"));
            STFLASH_Print(("                    COMPARE PIO Test                     \n"));
            STFLASH_Print(( "============================================================\n" ));
      
            error = CompareTest();
            if (error == ST_NO_ERROR)
            {
            	STFLASH_Print(("\nTEST PASSED SUCCESSFULLY\n"));
            }
            
            error = STBOOT_Term( "FLASHTest", &stboot_TermPars );
            STFLASH_Print(( "\n============================================================\n" ));
        }
    }
}

static void CompareNotifyFunction(STPIO_Handle_t Handle,
                                  STPIO_BitMask_t ActiveBits)
{
    semaphore_signal(CompareSemaphore_p);
}

static void CompareClearNotifyFunction(STPIO_Handle_t Handle,
                                       STPIO_BitMask_t ActiveBits)
{
    /* Clear the pins, so we shouldn't keep getting interrupts */
    STPIO_Clear(Handle, ActiveBits);
    semaphore_signal(CompareSemaphore_p);

}

ST_ErrorCode_t CompareTest(void)
{
    ST_ErrorCode_t     error = ST_NO_ERROR;
    STPIO_Handle_t     HandleIRQ;
    STPIO_InitParams_t InitParams;
    STPIO_OpenParams_t OpenParams;
    STPIO_Compare_t    setCmpReg;
    STPIO_Status_t     status;
    U8 BitMask;
    U8 Device;
    STPIO_TermParams_t TermParams;
    osclock_t clk;
    
    /* PIO 0 */
    InitParams.BaseAddress     = (U32*)COMPARE_PIO_BASE_ADDRESS;
    InitParams.InterruptLevel  = COMPARE_PIO_INTERRUPT_LEVEL;
    InitParams.InterruptNumber = COMPARE_PIO_INTERRUPT;
    InitParams.DriverPartition = system_partition;

    /* Set test params */
    BitMask = COMPARE_TEST_BITMASK;
    Device  = COMPARE_TEST_PIO;

    /* Create timeout semaphore */
#if defined(ST_OS21)
    CompareSemaphore_p = semaphore_create_fifo(0);
#else
    CompareSemaphore_p = &CompareSemaphore;
    semaphore_init_fifo_timeout(CompareSemaphore_p, 0);
#endif

    /*****************************************
    Init Tests - smartcard
    *****************************************/

    /* Make the call and then print the error value */
    error = STPIO_Init(PIO_DeviceName[COMPARE_PIO], &InitParams);
    STFLASH_Print(("STPIO_Init()\n", error));

    /*****************************************
    Open Tests - smartcard
    *****************************************/

    /* set up for "detect" on smartcard */
    OpenParams.ReservedBits    = BitMask;
    OpenParams.BitConfigure[0] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[1] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[2] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[3] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[4] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[5] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[6] = STPIO_BIT_OUTPUT;
    OpenParams.BitConfigure[7] = STPIO_BIT_OUTPUT;
    OpenParams.IntHandler      = CompareNotifyFunction;

    /* Make the call and then print the error value */
    error = STPIO_Open(PIO_DeviceName[COMPARE_PIO], &OpenParams, &HandleIRQ);
    STFLASH_Print(("STPIO_Open()\n", error));

    /* Obtains status and display */
    error = STPIO_GetStatus(HandleIRQ, &status);
    STFLASH_Print(("STPIO_GetStatus()\n", error));

    /*****************************************
    Compare Tests
    *****************************************/
    STFLASH_Print(("\n+++++++ Compare Test ++++++++++++ \n\n"));
    
    /* Clear compare bit to ensure pattern is zero */
    STFLASH_Print(("Ensure compare bit is cleared....\n"));
    error = STPIO_Clear(HandleIRQ, BitMask);

    /* Set compare tests */
    STFLASH_Print(("Configuring compare pattern for compare bit....\n"));

    setCmpReg.CompareEnable = BitMask;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    STFLASH_Print(("STPIO_SetCompare()\n", error));

    /* Wait for a settling period to ensure no callbacks occur */
    STFLASH_Print(("Wait for a settling period (in case of spurious callbacks)....\n"));
    clk = time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != -1)
    {    
        STFLASH_Print(("Unexpected notify callback\n"));
    }
    STFLASH_Print(("Set the compare bit to invoke the callback....\n"));
    error = STPIO_Set(HandleIRQ, BitMask);
    STFLASH_Print(("STPIO_Set()\n", error));

    clk = time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != 0)
    {
    	STFLASH_Print(("Timed out waiting for callback\n"));
    }
    else
    {
        STFLASH_Print(("Received callback!\n"));
    }

    error = STPIO_Close(HandleIRQ);
    STFLASH_Print(("STPIO_Close\n", error));

    /*****************************************
    CompareClear Tests
    *****************************************/
    STFLASH_Print(("\n+++++++ Compare Clear Test ++++++++++++ \n\n"));
    OpenParams.IntHandler = CompareClearNotifyFunction;
    error = STPIO_Open(PIO_DeviceName[COMPARE_PIO], &OpenParams, &HandleIRQ);
    STFLASH_Print(("STPIO_Open()\n", error));

    /* Check that auto mode still works as it should */
    STFLASH_Print(("Checking that auto mode gives two callbacks\n"));

    /* Set compare tests */
    STFLASH_Print(("Clearing compare enable bits....\n"));

    setCmpReg.CompareEnable = 0;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    STFLASH_Print(("STPIO_SetCompare()\n", error));

    /* Clear compare bit to ensure pattern is zero */
    STFLASH_Print(("Ensure compare bit is cleared....\n"));
    error = STPIO_Clear(HandleIRQ, BitMask);

    /* Do not set compareclear value - should default to auto. If not, that's
     * also a bug...
     */

    /* Set compare */
    STFLASH_Print(("Configuring compare pattern for compare bit....\n"));

    setCmpReg.CompareEnable = BitMask;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    STFLASH_Print(("STPIO_SetCompare()\n", error));

    /* Wait for a settling period to ensure no callbacks occur */
    STFLASH_Print(("Wait for a settling period (in case of spurious callbacks)....\n"));

    clk = time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != -1)
    {
        STFLASH_Print(("Unexpected notify callback\n"));
    }

    STFLASH_Print(("Set the compare bit to invoke the callback....\n"));
    error = STPIO_Set(HandleIRQ, BitMask);
    STFLASH_Print(("STPIO_Set()\n", error));

    clk = time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != 0)
    {
        STFLASH_Print(("Timed out waiting for callback\n"));
    }
    else
    {

        if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != 0)
        {
            STFLASH_Print(("Only received one callback\n"));
        }
        else
        {
            STFLASH_Print(("Received two callbacks\n"));
        }
    }

    /* Check that manual mode works as it should */

    STFLASH_Print(("Checking that manual mode only gives one callback\n"));

    /* Set compare tests */
    STFLASH_Print(("Clearing compare enable bits....\n"));

    setCmpReg.CompareEnable = 0;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    STFLASH_Print(("STPIO_SetCompare()\n", error));

    /* Clear compare bit to ensure pattern is zero */
    STFLASH_Print(("Ensure compare bit is cleared....\n"));
    error = STPIO_Clear(HandleIRQ, BitMask);

    STFLASH_Print(("Setting CompareClear value\n"));
    error = STPIO_SetCompareClear(HandleIRQ, STPIO_COMPARE_CLEAR_MANUAL);
    STFLASH_Print(("STPIO_SetCompareClear()\n", error));

    /* Set compare tests */
    STFLASH_Print(("Configuring compare pattern for compare bit....\n"));

    setCmpReg.CompareEnable = BitMask;
    setCmpReg.ComparePattern = 0;

    error = STPIO_SetCompare(HandleIRQ, &setCmpReg);
    STFLASH_Print(("STPIO_SetCompare()\n", error));

    /* Wait for a settling period to ensure no callbacks occur */
    STFLASH_Print(("Wait for a settling period (in case of spurious callbacks)....\n"));

    clk = time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != -1)
    {
        STFLASH_Print(("Unexpected notify callback\n"));
    }

    STFLASH_Print(("Set the compare bit to invoke the callback....\n"));
    error = STPIO_Set(HandleIRQ, BitMask);
    STFLASH_Print(("STPIO_Set()\n", error));

    clk = time_plus(time_now(), 2*ST_GetClocksPerSecond());

    if (semaphore_wait_timeout(CompareSemaphore_p, &clk) != 0)
    {
        STFLASH_Print(("Timed out waiting for callback\n"));
    }
    else
    {
        if (semaphore_wait_timeout(CompareSemaphore_p, TIMEOUT_IMMEDIATE) != 0)
        {
            STFLASH_Print(("Received *single* callback\n"));
        }
        else
        {
            STFLASH_Print(("Received *two* callbacks!\n"));
        }
    }

    /* Tidy up */
    semaphore_delete(CompareSemaphore_p);

    TermParams.ForceTerminate = TRUE;
    error = STPIO_Term(PIO_DeviceName[COMPARE_PIO], &TermParams);
    STFLASH_Print(("STPIO_Term()\n", error));
    
    return error;

}

