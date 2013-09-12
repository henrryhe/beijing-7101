/****************************************************************************

File Name   : stpwmtst.c

Description : Pulse Width Modulation API Test Routines

Copyright (C) 2005, ST Microelectronics

Revision History    :

    [...]
    10Oct03     Updated for 5528 changes
    31Mar04     Updated for 5100 changes
    July05      Updated for 8010 changes
    30Mar06     Updated for 7109/7100 changes
    04Mar06     Updated for 5525 changes
References  :

$ClearCase (VOB: stpwm)

stpwm.fm "PWM API" Reference DVD-API-010 Revision 1.5

 ****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include "stcommon.h"
#include "stdevice.h"
#include "stack.h"
#include "stlite.h"
#include "stddefs.h"
#include  "stos.h"
#if (STPWM_USES_PIO == TRUE)
#include "stpio.h"
#endif

#include "stpwm.h"
#include "stboot.h"
#include "sttbx.h"

#ifdef ST_OS20
#include "debug.h"
#include "stsys.h"
#endif

#define osclock_t  clock_t

/* Private Types ---------------------------------------------------------- */

typedef volatile U8 DU8;

/* Private Constants ------------------------------------------------------ */

#if defined (ST_5100) || defined (ST_8010)
/* 16 bit resolution in chip */
#define MARK_INTER   1280            /* interval between Mark steps */
#else
#define MARK_INTER   5               /* interval between Mark steps */
#endif

#define MARK_DELTA   ( STPWM_MARK_MAX + STPWM_MARK_MIN ) /* mirror difference */
#define TOO_LO_PRE   ( STPWM_PRESCALE_MIN - 1 )   /* too low  prescaler value */
#define TOO_HI_PRE   ( STPWM_PRESCALE_MAX + 1 )   /* too high prescaler value */
#define TOO_LO_MRK   ( STPWM_MARK_MIN - 1 )       /* too low  mark value */
#define TOO_HI_MRK   ( STPWM_MARK_MAX + 1 )       /* too high mark value */
#define MULTI_BITS   ( PIO_BIT_3 | PIO_BIT_4 )    /* multiple PIO bits set */

#define STPWM_HANDLE_NO_0       1    /* PWM channel 0 handle bit */
#define STPWM_HANDLE_NO_1       2    /* PWM channel 1 handle bit */
#define STPWM_HANDLE_NO_2       4    /* PWM channel 2 handle bit */


#define MULTI_HAND   ( STPWM_HANDLE_NO_0 | STPWM_HANDLE_NO_2 )


#ifdef ST_5512

#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_1_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_1_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_1_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_3
#define PWM0_PIO_PORT_NAME              "Port1"

#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_1_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_1_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_1_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_4
#define PWM1_PIO_PORT_NAME              "Port1"

#define PWM2_PIO_PORT_BASE_ADDRESS      PIO_1_BASE_ADDRESS
#define PWM2_PIO_PORT_INTERRUPT         PIO_1_INTERRUPT
#define PWM2_PIO_PORT_INTERRUPT_LEVEL   PIO_1_INTERRUPT_LEVEL
#define PWM2_PIO_BIT                    PIO_BIT_7
#define PWM2_PIO_PORT_NAME              "Port1"

#elif defined (ST_5100)

#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_2_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_2_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_2_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_5
#define PWM0_PIO_PORT_NAME              "Port2"

#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_4_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_4_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_4_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_2
#define PWM1_PIO_PORT_NAME              "Port4"

#elif defined (ST_8010)

#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_2_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_2_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_2_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_3
#define PWM0_PIO_PORT_NAME              "Port2"


#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_2_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_2_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_2_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_6
#define PWM1_PIO_PORT_NAME              "Port2"


#elif defined (ST_7710)

#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_5_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_5_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_5_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_4
#define PWM0_PIO_PORT_NAME              "Port5"


#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_4_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_4_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_4_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_5
#define PWM1_PIO_PORT_NAME              "Port4"

/* Capture pio input pin */
#define PWM_CAPTURE0_PIO_PORT_BASE_ADDRESS  	PIO_3_BASE_ADDRESS
#define PWM_CAPTURE0_PIO_PORT_INTERRUPT     	PIO_3_INTERRUPT
#define PWM_CAPTURE0_PIO_PORT_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL
#define PWM_CAPTURE0_PIO_BIT                    PIO_BIT_7
#define PWM_CAPTURE0_PIO_PORT_NAME              "Port3"

#elif defined (ST_7109)  || defined(ST_7100) || defined(ST_7200)

#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_4_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_4_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_4_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_6
#define PWM0_PIO_PORT_NAME              "Port4"

#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_4_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_4_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_4_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_7
#define PWM1_PIO_PORT_NAME              "Port4"

/* Capture pio input pin */
#define PWM_CAPTURE0_PIO_PORT_BASE_ADDRESS  	PIO_5_BASE_ADDRESS
#define PWM_CAPTURE0_PIO_PORT_INTERRUPT     	PIO_5_INTERRUPT
#define PWM_CAPTURE0_PIO_PORT_INTERRUPT_LEVEL   PIO_5_INTERRUPT_LEVEL
#define PWM_CAPTURE0_PIO_BIT                    PIO_BIT_5
#define PWM_CAPTURE0_PIO_PORT_NAME              "Port5"

#define PWM_CAPTURE1_PIO_PORT_BASE_ADDRESS  	PIO_5_BASE_ADDRESS
#define PWM_CAPTURE1_PIO_PORT_INTERRUPT     	PIO_5_INTERRUPT
#define PWM_CAPTURE1_PIO_PORT_INTERRUPT_LEVEL   PIO_5_INTERRUPT_LEVEL
#define PWM_CAPTURE1_PIO_BIT                    PIO_BIT_7
#define PWM_CAPTURE1_PIO_PORT_NAME              "Port5"

/* Compare Out Pio Pins*/
#define PWM_COMPARE0_PIO_PORT_BASE_ADDRESS  	PIO_5_BASE_ADDRESS
#define PWM_COMPARE0_PIO_PORT_INTERRUPT     	PIO_5_INTERRUPT
#define PWM_COMPARE0_PIO_PORT_INTERRUPT_LEVEL   PIO_5_INTERRUPT_LEVEL
#define PWM_COMPARE0_PIO_BIT                    PIO_BIT_4
#define PWM_COMPARE0_PIO_PORT_NAME              "Port5"

#define PWM_COMPARE1_PIO_PORT_BASE_ADDRESS  	PIO_5_BASE_ADDRESS
#define PWM_COMPARE1_PIO_PORT_INTERRUPT     	PIO_5_INTERRUPT
#define PWM_COMPARE1_PIO_PORT_INTERRUPT_LEVEL   PIO_5_INTERRUPT_LEVEL
#define PWM_COMPARE1_PIO_BIT                    PIO_BIT_6
#define PWM_COMPARE1_PIO_PORT_NAME              "Port5"
#elif defined (ST_5525) 
#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_6_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_6_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_6_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_0
#define PWM0_PIO_PORT_NAME              "Port6"

#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_6_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_6_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_6_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_6
#define PWM1_PIO_PORT_NAME              "Port6"


#else /* 5514, 5516, 5517, 5528 */

#define PWM0_PIO_PORT_BASE_ADDRESS      PIO_2_BASE_ADDRESS
#define PWM0_PIO_PORT_INTERRUPT         PIO_2_INTERRUPT
#define PWM0_PIO_PORT_INTERRUPT_LEVEL   PIO_2_INTERRUPT_LEVEL
#define PWM0_PIO_BIT                    PIO_BIT_7
#define PWM0_PIO_PORT_NAME              "Port2"

#define PWM1_PIO_PORT_BASE_ADDRESS      PIO_3_BASE_ADDRESS
#define PWM1_PIO_PORT_INTERRUPT         PIO_3_INTERRUPT
#define PWM1_PIO_PORT_INTERRUPT_LEVEL   PIO_3_INTERRUPT_LEVEL
#define PWM1_PIO_BIT                    PIO_BIT_7
#define PWM1_PIO_PORT_NAME              "Port3"

#define PWM2_PIO_PORT_BASE_ADDRESS      PIO_4_BASE_ADDRESS
#define PWM2_PIO_PORT_INTERRUPT         PIO_4_INTERRUPT
#define PWM2_PIO_PORT_INTERRUPT_LEVEL   PIO_4_INTERRUPT_LEVEL
#define PWM2_PIO_BIT                    PIO_BIT_7
#define PWM2_PIO_PORT_NAME              "Port4"

#endif

/* "white box" knowledge of driver's returned handles */
#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010) || defined(ST_7109)  || defined(ST_7100) ||\
                                                                                        defined(ST_5525) || defined(ST_7200)
static const U32 stpwm_PWMEnc[] = { STPWM_HANDLE_NO_0,
                                    STPWM_HANDLE_NO_1 };
#else
static const U32 stpwm_PWMEnc[] = { STPWM_HANDLE_NO_0,
                                    STPWM_HANDLE_NO_1,
                                    STPWM_HANDLE_NO_2,};
#endif  /* End ST_5100 */


const U8 HarnessRev[] = "2.0.0";
U32 IntCount=0;
U32 IntCount2= 0;
clock_t time1[1000] = {0};
static semaphore_t *CompareSemaphore_p;
static semaphore_t *CompareSemaphore2_p;

/* Register offset for CKG/DENC registers on STi5508 */
#define CKG_IDDQPAD_C    0xe3         /* CKG_IDDQPAD_C register offset (U8*) */
#define CKG_PWM_MASK     0x30         /* Bitmask for preserving PWM bits */
#define DENC_CFG0        0x00         /* DENC_CFG0 register offset (U8*) */
#define DENC_MASTER_MODE 0x30         /* Bitmask for master mode */

#if !defined(ST_OSLINUX)
/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            100000
#endif

/* Memory partitions */
#ifdef ST_OS20
#define TEST_PARTITION_1        &the_system_partition
#define TEST_PARTITION_2        &the_system_partition

#else
#define TEST_PARTITION_1        system_partition
#define TEST_PARTITION_2        system_partition
#endif


/* Debug output */
#ifdef DebugPrintf
#define DebugPrintf(args)      STTBX_Print(args)
#else
#define DebugPrintf(args)      printf args
#endif



/* Private Variables ------------------------------------------------------ */
#ifdef ST_OS20
/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#pragma ST_section      ( system_block,   "system_section")

#define                 NCACHE_MEMORY_SIZE          0x80000

static unsigned char    ncache_memory [NCACHE_MEMORY_SIZE];
static partition_t      the_ncache_partition;
partition_t             *ncache_partition = &the_ncache_partition;
#pragma ST_section      ( ncache_memory , "ncache_section" )


#ifdef ARCHITECTURE_ST20

/* This is to avoid a linker warning */
static unsigned char    internal_block_noinit[1];
#pragma ST_section      ( internal_block_noinit, "internal_section_noinit")

static unsigned char    system_block_noinit[1];
#pragma ST_section      ( system_block_noinit, "system_section_noinit")

#endif

#if defined(ST_5100) || defined (ST_7710) || defined(ST_5528) || defined(ST_7109)  || defined(ST_7100) ||\
                                                                                      defined(ST_5525) || defined(ST_7200)
static unsigned char    data_section[1];
#pragma ST_section      ( data_section, "data_section")
#endif  /* End ST_5100 */

#elif defined(ST_OS21)

#define                 SYSTEM_MEMORY_SIZE          0x800000
static unsigned char    external_block[SYSTEM_MEMORY_SIZE];
partition_t             *system_partition;

#elif defined(ST_OSLINUX)

ST_Partition_t         *DriverPartition = (ST_Partition_t*)1;
ST_Partition_t         *system_partition = (ST_Partition_t*)1;
#endif


#if (STPWM_USES_PIO == TRUE)

static STPIO_InitParams_t PIOInitPar_s;
static STPIO_TermParams_t PIOTermPar_s;

#else

static DU8 *CkgBase  = (U8*)CKG_BASE_ADDRESS;
static DU8 *DencBase = (U8*)DENC_BASE_ADDRESS;

#endif

static ST_DeviceName_t    DeviceName;

static clock_t            LongDelay;
static clock_t            ShortDelay;

/* Private Macros --------------------------------------------------------- */

/* Private Function prototypes -------------------------------------------- */

void NormalUse( void );
void ErrantUse( void );
void OutputComparetest(void);
void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr );
void NotifyFunction(void);  
void NotifyFunction2(void);             
/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : none

Return Value : int

See Also     : None
 ****************************************************************************/

int main( int argc, char *argv[] )
{

#if !defined(ST_OSLINUX)
#if !defined(DISABLE_TOOLBOX)
    STTBX_InitParams_t      sttbx_InitPars;
#endif

    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    ST_ErrorCode_t retVal;

#ifndef DISABLE_DCACHE
#if defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS
}, /* assumed ok */
        { NULL, NULL }
    };

#elif defined(ST_5514) || defined(ST_5516) || defined(ST_5517)

    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)0x40200000, (U32 *)0x407FFFFF }, /* ok */
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
#endif

#ifdef ST_OS21

    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));

#else /* ifndef ST_OS21 */

#if defined(ST_5100) || defined (ST_7710) || defined(ST_5528) || defined(ST_7109) || defined(ST_7100) ||\
                                                                                     defined(ST_5525) || defined(ST_7200) 
    data_section[0] = 0;
#endif


    /* Create memory partitions */
    partition_init_heap(&the_internal_partition,
                        (U8*)internal_block,
                        sizeof(internal_block));
    partition_init_heap(&the_system_partition,
                        (U8*)system_block,
                        sizeof(system_block));
    partition_init_heap(&the_ncache_partition,
                        ncache_memory,
                        sizeof(ncache_memory));

#ifdef ARCHITECTURE_ST20

    partition_init_heap(&the_ncache_partition,
                        ncache_memory,
                        sizeof(ncache_memory));

    /* Avoids a compiler warning */
    internal_block_noinit[0] = 0;
    system_block_noinit[0] = 0;

#endif

#endif /* ifndef ST_OS21 */

    /* Initialize stboot */
#if defined DISABLE_ICACHE

    stboot_InitPars.ICacheEnabled             = FALSE;

#else

    stboot_InitPars.ICacheEnabled             = TRUE;

#endif
    stboot_InitPars.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitPars.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.CacheBaseAddress          = (U32*)CACHE_BASE_ADDRESS;
    stboot_InitPars.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
#if defined DISABLE_DCACHE

    stboot_InitPars.DCacheMap                 = NULL;

#else

    stboot_InitPars.DCacheMap                 = DCacheMap;

#endif

#ifdef ST_OS21 
    stboot_InitPars.TimeslicingEnabled        = TRUE;
#endif    


    DebugPrintf(("STBoot entry\n"));
    retVal = STBOOT_Init( "PWMTest", &stboot_InitPars );
    DebugPrintf(("STBoot exit\n"));
    if( retVal != ST_NO_ERROR )
    {
        DebugPrintf(( "ERROR: STBOOT_Init() returned %d\n", retVal ));
    }
    else
    {
#if !defined(DISABLE_TOOLBOX)
        retVal = ST_NO_ERROR;

        /* Initialize the toolbox */
        sttbx_InitPars.CPUPartition_p      = system_partition;
        sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

        retVal = STTBX_Init( "tbx", &sttbx_InitPars );

        if( retVal != ST_NO_ERROR )
        {
            printf( "ERROR: STTBX_Init() returned %d\n", retVal );
        }
        else
#endif
        {

            /* Setup delay variables */
            LongDelay  = ST_GetClocksPerSecond()* 15;
            ShortDelay = ST_GetClocksPerSecond() / 10;
#if defined (ST_5100)
            /* Set Pio4[6] in Alt 1 for pwm0 */
            {
                U32 rv;
                rv = *(volatile U32*)(0x20D00014);
                rv |= 0x400;
                rv &= 0xFFFF9FFF;
                *(volatile U32*)(0x20D00014) = rv;
            }
  
#elif defined(ST_7109)  || defined(ST_7100)  || defined(ST_7200)
  /* Set Pio4[6] and Pio4[7] in Alt 1 for pwm0 and pwm1 */
  /*Sysconfig Register 7[0]=0 on reset*/

#elif defined(ST_5525)
           /* Set Pio6[0] and Pio6[6] in Alt 1 for pwm0 and pwm1 in ic_cfg_ctrl3 register*/
            {
                U32 rv;
                rv = *(volatile U32*)(0x1980001c);
                rv |= 0x041;
                *(volatile U32*)(0x1980001c) = rv;
              
            }
#endif      /* End ST_5100 */

            /* In ST40 enable the PWM clock in clock gen block */
#if defined (ARCHITECTURE_ST40) 
            {
                *(volatile U32*)(0xb9163090) = 0xC0DE;
                *(volatile U32*)(0xb916308c) = 0;
                *(volatile U32*)(0xb9163090) = 0x5555;
            }
#endif

            /* Start tests */
#if ! (defined (ST_5100) || defined (ST_7710))         
            OutputComparetest();

#endif            
            NormalUse();
            ErrantUse();

            retVal = (ST_ErrorCode_t) STBOOT_Term( "PWMTest", &stboot_TermPars );
        }
    }

    return retVal;
#endif

#if defined(ST_OSLINUX)

	NormalUse();
	ErrantUse();

       return ST_NO_ERROR;
    
#endif

}

#if defined (ST_7710) || defined(ST_7100)
void DELAY(U32 i)
{
    U32 j;
    for (j=0;j<i;j++);
}
#endif

void NotifyFunction(void)
{
		IntCount++;
	    time1[IntCount] = time_now();
		STOS_SemaphoreSignal(CompareSemaphore_p);

}
void NotifyFunction2(void)
{
		IntCount2++;
		STOS_SemaphoreSignal(CompareSemaphore2_p);

}

/****************************************************************************
Name         : NormalUse()

Description  : Performs all the API interface calls in the correct
               sequence, with legal parameters, exercising the
               three PWM channels supported with a range of MarkValue
               settings with different clock prescaler values.
               The PIO requires initialization prior to PWM access.
               Note that no errors should be raised during this test.
               Else the first error encountered will be returned.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel

See Also     : STLLI_ErrorCode_t
 ****************************************************************************/

void NormalUse( void )
{
    S32             MarkValue;                  /* current Mark setting */
    U32             OtherMark;                  /* opposing value (double test) */
    U32             ReturnMrk;                  /* returned Mark value */
    STPWM_InitParams_t InitParams_s;
    STPWM_OpenParams_t OpenParams_s;
    STPWM_TermParams_t TermParams_s;
    STPWM_Handle_t     Handle[STPWM_CHANNEL_CNT];
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    ST_Revision_t   RevisionStr;
    
#if (STPWM_USES_PIO == FALSE)
    U8              HwRead;
#endif

    DebugPrintf(( "\nHarness Software Revision is %s\n\n", HarnessRev ));

    DebugPrintf(( "==========================================================\n" ));
    DebugPrintf(( "Commencing NormalUse Test Function ....\n" ));
    DebugPrintf(( "==========================================================\n" ));

#if (STPWM_USES_PIO == TRUE)

    /* Initialize the PIO Driver */
    PIOInitPar_s.BaseAddress     = (U32*)PWM0_PIO_PORT_BASE_ADDRESS;
    PIOInitPar_s.InterruptNumber = PWM0_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel  = PWM0_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;

    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM0_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


#if !( defined(ST_8010) ||  defined (ST_7109) || defined(ST_7100) || defined(ST_5525)  || defined(ST_7200))
/* Same Pio Port for both the PWM Outputs*/
#if defined (ST_5100) || defined (ST_7710) 

    PIOInitPar_s.BaseAddress     = (U32*)PWM1_PIO_PORT_BASE_ADDRESS;
    PIOInitPar_s.InterruptNumber = PWM1_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel  = PWM1_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM1_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#else

    if (PWM1_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS)
    {
        PIOInitPar_s.BaseAddress     = (U32*)PWM1_PIO_PORT_BASE_ADDRESS;
        PIOInitPar_s.InterruptNumber = PWM1_PIO_PORT_INTERRUPT;
        PIOInitPar_s.InterruptLevel  = PWM1_PIO_PORT_INTERRUPT_LEVEL;
        PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
        DebugPrintf(( "Calling STPIO_Init() ................\n" ));
        ReturnError = STPIO_Init( PWM1_PIO_PORT_NAME, &PIOInitPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if ((PWM2_PIO_PORT_BASE_ADDRESS != PWM1_PIO_PORT_BASE_ADDRESS) &&
        (PWM2_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS))
    {
        PIOInitPar_s.BaseAddress     = (U32*)PWM2_PIO_PORT_BASE_ADDRESS;
        PIOInitPar_s.InterruptNumber = PWM2_PIO_PORT_INTERRUPT;
        PIOInitPar_s.InterruptLevel  = PWM2_PIO_PORT_INTERRUPT_LEVEL;
        PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
        DebugPrintf(( "Calling STPIO_Init() ................\n" ));
        ReturnError = STPIO_Init( PWM2_PIO_PORT_NAME, &PIOInitPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
#endif  /* End ST_5100 */
#endif  /* End ST_8010 */

#else

    /* Set DENC into master mode to stop it using Hsync/Vsync lines */
    DencBase[DENC_CFG0] = DENC_MASTER_MODE;

    /* Configure outputs as PWM0 and PWM2 not Hsync/Vsync */
    HwRead = CkgBase[CKG_IDDQPAD_C];
    CkgBase[CKG_IDDQPAD_C] = HwRead & ~CKG_PWM_MASK;

#endif

    /* revision string available before PWM Initialized */
    DebugPrintf(( "Calling STPWM_GetRevision() before Init\n" ));
    RevisionStr = STPWM_GetRevision();
    DebugPrintf(( "Software Revision reported as %s\n", RevisionStr ));
    DebugPrintf(( "----------------------------------------------------------\n" ));

    strcpy( (char*)DeviceName, "STPWM" );

    InitParams_s.Device = STPWM_C4;

    InitParams_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;
    
    InitParams_s.C4.InterruptNumber = PWM_CAPTURE_INTERRUPT;
    InitParams_s.C4.InterruptLevel = PWM_INTERRUPT_LEVEL;
#if (STPWM_USES_PIO == TRUE)

    strcpy( (char*)InitParams_s.C4.PIOforPWM0.PortName, PWM0_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM0.BitMask = PWM0_PIO_BIT;
    strcpy( (char*)InitParams_s.C4.PIOforPWM1.PortName, PWM1_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM1.BitMask = PWM1_PIO_BIT;
#if defined(ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109)  || defined(ST_7100)||\
                                                                                         defined(ST_5525) || defined(ST_7200)
/*Two PWM Channels */
    InitParams_s.C4.PIOforPWM2.PortName[0] = '\0';
    InitParams_s.C4.PIOforPWM2.BitMask = 0;
#else
    strcpy( (char*)InitParams_s.C4.PIOforPWM2.PortName, PWM2_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM2.BitMask = PWM2_PIO_BIT;
#endif  /* End ST_5100 */

#else

    InitParams_s.C4.PIOforPWM0.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;

#endif

    /* Initializing with MIN prescale factor */
    DebugPrintf(( "Calling STPWM_Init() with \n" ));
    DebugPrintf(( "PrescaleFactor set to MIN (1) .......\n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Channel 0 open and ramped from MIN to MAX MarkValue */
    /* Start PWM output and then pause for user to set 'scope */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_0;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
    DebugPrintf(( "Calling STPWM_Open() for Channel 0 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_0] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (ST_8010)
    /* Channel 1 open and ramped from MIN to MAX MarkValue */
    /* Start PWM output and then pause for user to set 'scope */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    DebugPrintf(( "Calling STPWM_Open() for Channel 1 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_1] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif


    /* PWM0 is now running, pause for 'scope before ramping */
    DebugPrintf(( "Please place 'Scope/DVM probe on PWM0\n" ));
    DebugPrintf(( "Commencing delay period .............\n" ));
    DebugPrintf(( "----------------------------------------------------------\n" ));


#if !(defined (ST_7710) || defined(ST_7100))
    STOS_TaskDelay( LongDelay );
#else
    DELAY(LongDelay);
#endif    

    /* Note that Open defaults to MIN MarkValue anyway,
       Loop on MarkValue range MIN to MAX with delays   */
    for( MarkValue = STPWM_MARK_MIN;
         MarkValue <= STPWM_MARK_MAX; MarkValue += MARK_INTER )
    {
        DebugPrintf(( "Calling STPWM_SetRatio(),MarkValue %02d\n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_0], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio() for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_0], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (ST_8010)
        DebugPrintf(( "Calling STPWM_SetRatio()(1),MarkValue(1) %02d\n",(STPWM_MARK_MAX - MarkValue) ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1], (U32)(STPWM_MARK_MAX - MarkValue) );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio()(1) for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], &ReturnMrk );
        DebugPrintf(( "MarkValue(1) returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
        STOS_TaskDelay( ShortDelay );
#else
        DELAY(ShortDelay);
#endif    
    }

#if !defined (ST_8010)
    /* Channel 1 open and ramped from MAX to MIN MarkValue */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    DebugPrintf(( "Calling STPWM_Open() for Channel 1 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_1] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Pause for 'scope now that PWM1 is running */
    DebugPrintf(( "Please place 'Scope/DVM probe on PWM1\n" ));
    DebugPrintf(( "Commencing delay period .............\n" ));
    DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
    STOS_TaskDelay( LongDelay );
#else
    DELAY(LongDelay);
#endif    


    /* Note that Open defaults to MIN MarkValue,
       Loop on MarkValue range MAX to MIN with delays   */
    for( MarkValue = STPWM_MARK_MAX;
         MarkValue >= STPWM_MARK_MIN; MarkValue -= MARK_INTER )
    {
        DebugPrintf(( "Calling STPWM_SetRatio(),MarkValue %02d\n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio() for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
    STOS_TaskDelay( ShortDelay );
#else
    DELAY(ShortDelay);
#endif  
    }
#endif /* !8010*/

    /* Closing Channels 0 & 1 */

    DebugPrintf(( "Calling STPWM_Close() for Channel 0 .\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    DebugPrintf(( "Calling STPWM_Close() for Channel 1 .\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    TermParams_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPWM_Term() unforced .......\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


#if defined (ST_8010)
    /* Initializing with MIN prescale factor */
    DebugPrintf(( "Calling STPWM_Init() with \n" ));
    DebugPrintf(( "PrescaleFactor set to MIN (1) .......\n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Channel 0 open and ramped from MIN to MAX MarkValue */
    /* Start PWM output and then pause for user to set 'scope */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_0;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX;
    
    DebugPrintf(( "Calling STPWM_Open() for Channel 0 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_0] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (ST_8010)
    /* Channel 1 open and ramped from MIN to MAX MarkValue */
    /* Start PWM output and then pause for user to set 'scope */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX;
    DebugPrintf(( "Calling STPWM_Open() for Channel 1 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_1] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif


    /* PWM0 is now running, pause for 'scope before ramping */
    DebugPrintf(( "Please place 'Scope/DVM probe on PWM0\n" ));
    DebugPrintf(( "Commencing delay period .............\n" ));
    DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
    STOS_TaskDelay( LongDelay );
#else
    DELAY(LongDelay);
#endif    

    /* Note that Open defaults to MIN MarkValue anyway,
       Loop on MarkValue range MIN to MAX with delays   */
    for( MarkValue = STPWM_MARK_MIN;
         MarkValue <= STPWM_MARK_MAX; MarkValue += MARK_INTER )
    {

        DebugPrintf(( "Calling STPWM_SetRatio(),MarkValue %02d\n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_0], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio() for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_0], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (ST_8010)
        DebugPrintf(( "Calling STPWM_SetRatio()(1),MarkValue(1) %02d\n",(STPWM_MARK_MAX - MarkValue) ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1], (U32)(STPWM_MARK_MAX - MarkValue) );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio()(1) for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], &ReturnMrk );
        DebugPrintf(( "MarkValue(1) returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
        STOS_TaskDelay( ShortDelay );
#else
        DELAY(ShortDelay);
#endif    
    }
  
  /* Closing Channels 0 & 1 */

    DebugPrintf(( "Calling STPWM_Close() for Channel 0 .\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined(ST_8010)
    DebugPrintf(( "Calling STPWM_Close() for Channel 1 .\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif

    TermParams_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPWM_Term() unforced .......\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if defined (ST_8010)

    PIOTermPar_s.ForceTerminate = TRUE;
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM0_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    return;

#endif
#endif/*8010*/


#if (STPWM_USES_PIO == TRUE)

#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109)  || defined(ST_7100) ||\
                                                                                          defined(ST_5525) || defined(ST_7200)
    strcpy( (char*)InitParams_s.C4.PIOforPWM0.PortName, PWM0_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM0.BitMask = PWM0_PIO_BIT;
#else

    InitParams_s.C4.PIOforPWM0.PortName[0] = '\0';
    InitParams_s.C4.PIOforPWM0.BitMask = 0;
    /* InitParams_s.C4.PIOforPWM1 is left unchanged */
    strcpy( (char*)InitParams_s.C4.PIOforPWM2.PortName, PWM2_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM2.BitMask = PWM2_PIO_BIT;
#endif  /* End ST_5100 */

#endif

    /* Initializing with MAX/2 prescale factor */

    DebugPrintf(( "Calling STPWM_Init() with  \n" ));
    DebugPrintf(( "PrescaleFactor set to MAX/2 (8) .....\n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Channel 1 open and ramped from MIN to MAX MarkValue */

    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX/2;
    DebugPrintf(( "Calling STPWM_Open() for Channel 1 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_1] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Note that Open defaults to MIN MarkValue anyway,
       Loop on MarkValue range MIN to MAX with delays   */
    for( MarkValue = STPWM_MARK_MIN;
         MarkValue <= STPWM_MARK_MAX; MarkValue += MARK_INTER )
    {
        DebugPrintf(( "Calling STPWM_SetRatio(),MarkValue %02d\n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio() for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
        STOS_TaskDelay( ShortDelay );
#else
        DELAY(ShortDelay);
#endif  
    }

#if ! (defined(ST_5100) || defined (ST_7710) || defined (ST_8010) || defined (ST_7109) || defined(ST_7100)||\
                                                                                          defined(ST_5525) || defined(ST_7200))                  
    /* Channel 2 open and ramped from MAX to MIN MarkValue */

    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_2;
    OpenParams_s.PWMUsedFor = STPWM_Timer; 
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX/2;
    DebugPrintf(( "Calling STPWM_Open() for Channel 2 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_2] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_2] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Pause for 'scope now that PWM2 is running */
    DebugPrintf(( "Please place 'Scope/DVM probe on PWM2\n" ));
    DebugPrintf(( "Commencing delay period .............\n" ));
    DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
    STOS_TaskDelay( LongDelay );
#else
    DELAY(LongDelay);
#endif 

    /* Note that Open defaults to MIN MarkValue,
       Loop on MarkValue range MAX to MIN with delays   */
    for( MarkValue = STPWM_MARK_MAX;
         MarkValue >= STPWM_MARK_MIN; MarkValue -= MARK_INTER )
    {
        DebugPrintf(( "Calling STPWM_SetRatio(),MarkValue %02d\n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_2], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio() for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_2], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
        STOS_TaskDelay( ShortDelay );
#else
        DELAY(ShortDelay);
#endif        
    }
#endif  /* End ! ST_5100 */

    TermParams_s.ForceTerminate = TRUE;
    DebugPrintf(( "Calling STPWM_Term() forced to also  \n" ));
    DebugPrintf(( "Close Channels 0 & 2 ................\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    DebugPrintf(( "The following test ramps MarkValues  \n" ));


    DebugPrintf(( "on Two Channels simultaneously ....\n" ));

    DebugPrintf(( "Commencing delay period .............\n" ));
    DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
    STOS_TaskDelay( LongDelay );
#else
    DELAY(LongDelay);
#endif


#if (STPWM_USES_PIO == TRUE)

    strcpy( (char*)InitParams_s.C4.PIOforPWM0.PortName, PWM0_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM0.BitMask = PWM0_PIO_BIT;
    InitParams_s.C4.PIOforPWM1.PortName[0] = '\0';
    InitParams_s.C4.PIOforPWM1.BitMask = 0;

#endif

    /* Channel 2 left unchanged */
    /* Initializing */

    DebugPrintf(( "Calling STPWM_Init() with  \n" ));
    DebugPrintf(( "PrescaleFactor set to MAX (16) ......\n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Channels 0 & 2 opened and ramped simultaneously in
       opposite directions between MIN & MAX MarkValues   */

    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_0;
    OpenParams_s.PWMUsedFor = STPWM_Timer;    
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX;
    DebugPrintf(( "Calling STPWM_Open() for Channel 0 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_0] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;    
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX;    
    DebugPrintf(( "Calling STPWM_Open() for Channel 1 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_1] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#else

    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_2;
    OpenParams_s.PWMUsedFor = STPWM_Timer;  
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX;      
    DebugPrintf(( "Calling STPWM_Open() for Channel 2 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_2] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_2] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#endif  /* End ST_5100 */

    /* Loop on MarkValue range MIN to MAX (Channel 0) and
       MAX to MIN (Channel 2) with delays   */
    for( MarkValue = STPWM_MARK_MIN;
         MarkValue <= STPWM_MARK_MAX; MarkValue += MARK_INTER )
    {
        DebugPrintf(( "Calling STPWM_SetRatio(0), Mark = %02d \n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_0], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio(0) to confirm ..\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_0], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        OtherMark = MARK_DELTA - MarkValue; /* mirror value */
        DebugPrintf(( "Calling STPWM_SetRatio(2), Mark = %02d \n", OtherMark ));
        
#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1], OtherMark );
#else
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_2], OtherMark );
#endif
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio(2) to confirm ..\n" ));
        
#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], &ReturnMrk );
#else
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_2], &ReturnMrk );
#endif
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));

#if !(defined (ST_7710) || defined(ST_7100))
        STOS_TaskDelay( ShortDelay );
#else
        DELAY(ShortDelay);
#endif 
    }
    /* Closing Channels 2 & 0 */

    DebugPrintf(( "Calling STPWM_Close() for Channel 2 .\n" ));
#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_1] );
#else
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_2] );
#endif
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    DebugPrintf(( "Calling STPWM_Close() for Channel 0 .\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    TermParams_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPWM_Term() unforced .......\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if (STPWM_USES_PIO == TRUE)

    PIOTermPar_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM0_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !( defined(ST_8010) ||  defined (ST_7109) || defined(ST_7100) || defined(ST_5525) || defined(ST_7200))
/* Same Pio Port for both the PWM Outputs*/
#if defined (ST_5100) || defined (ST_7710) 

    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM1_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#else
    if (PWM1_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS)
    {
        DebugPrintf(( "Calling STPIO_Term() ................\n" ));
        ReturnError = STPIO_Term( PWM1_PIO_PORT_NAME, &PIOTermPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
    if ((PWM2_PIO_PORT_BASE_ADDRESS != PWM1_PIO_PORT_BASE_ADDRESS) &&
        (PWM2_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS))
    {
        DebugPrintf(( "Calling STPIO_Term() ................\n" ));
        ReturnError = STPIO_Term( PWM2_PIO_PORT_NAME, &PIOTermPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
#endif/* End ST_5100 */  
#endif
#endif

    DebugPrintf(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}

#if ! (defined (ST_5100) || defined (ST_7710))
void OutputComparetest( void )
{
    ST_ErrorCode_t     StoredError = ST_NO_ERROR;
    ST_ErrorCode_t     ReturnError;
    ST_Revision_t      RevisionStr;
    STPWM_InitParams_t InitParams_s;
    STPWM_OpenParams_t OpenParams_s;
    STPWM_TermParams_t TermParams_s;
    STPWM_CallbackParam_t  CallbackParam_s;
    STPWM_Handle_t     Handle[STPWM_CHANNEL_CNT];
    osclock_t          clk;
    S32                MarkValue;         /* current Mark setting */
    U32                ReturnMrk; 
    U32                CounterValue;               /* opposing value (double test) */
    U32                i = 10;
#if (STPWM_USES_PIO == FALSE)
    U8              HwRead;
#endif

    DebugPrintf(( "==========================================================\n" ));
    DebugPrintf(( "Commencing OutputCompare Test Function ....\n" ));
    DebugPrintf(( "==========================================================\n" ));

    /* Create timeout semaphore */
    CompareSemaphore_p  = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
    CompareSemaphore2_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0);
#if (STPWM_USES_PIO == TRUE)
    /* Initialize the PIO Driver */
    PIOInitPar_s.BaseAddress     = (U32*)PWM_COMPARE0_PIO_PORT_BASE_ADDRESS; /*5.4*/
    PIOInitPar_s.InterruptNumber = PWM_COMPARE0_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel  = PWM_COMPARE0_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;

    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM_COMPARE0_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
     /* Initialize the PIO Driver */
    PIOInitPar_s.BaseAddress     = (U32*)PWM1_PIO_PORT_BASE_ADDRESS;
    PIOInitPar_s.InterruptNumber = PWM1_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel  = PWM1_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;

    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM1_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#else

    /* Set DENC into master mode to stop it using Hsync/Vsync lines */
    DencBase[DENC_CFG0] = DENC_MASTER_MODE;

    /* Configure outputs as PWM0 and PWM2 not Hsync/Vsync */
    HwRead = CkgBase[CKG_IDDQPAD_C];
    CkgBase[CKG_IDDQPAD_C] = HwRead & ~CKG_PWM_MASK;

#endif

   
    /* revision string available before PWM Initialized */
    DebugPrintf(( "Calling STPWM_GetRevision() before Init\n" ));
    RevisionStr = STPWM_GetRevision();
    DebugPrintf(( "Software Revision reported as %s\n", RevisionStr ));
    DebugPrintf(( "----------------------------------------------------------\n" ));
    strcpy( (char*)DeviceName, "STPWM" );

    InitParams_s.Device = STPWM_C4;
    InitParams_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;
    InitParams_s.C4.InterruptNumber = PWM_CAPTURE_INTERRUPT;
    InitParams_s.C4.InterruptLevel = PWM_INTERRUPT_LEVEL;

#if (STPWM_USES_PIO == TRUE)

    strcpy( (char*)InitParams_s.C4.PIOforPWM0.PortName, PWM_COMPARE0_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM0.BitMask = PWM_COMPARE0_PIO_BIT;
    strcpy( (char*)InitParams_s.C4.PIOforPWM1.PortName, PWM1_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM1.BitMask = PWM1_PIO_BIT;
#if defined(ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109)  || defined(ST_7100)||\
                                                                                         defined(ST_5525) || defined(ST_7200)
    InitParams_s.C4.PIOforPWM2.PortName[0] = '\0';
    InitParams_s.C4.PIOforPWM2.BitMask = 0;
#else
    strcpy( (char*)InitParams_s.C4.PIOforPWM2.PortName, PWM2_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM2.BitMask = PWM2_PIO_BIT;
#endif  /* End ST_5100 */

#else

    InitParams_s.C4.PIOforPWM0.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;
#endif


    /* Initializing with MIN prescale factor */
    DebugPrintf(( "Calling STPWM_Init() with \n" ));
    DebugPrintf(( "PrescaleFactor set to MIN (1) .......\n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Channel 0 open and ramped from MIN to MAX MarkValue */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_0;
    OpenParams_s.PWMUsedFor = STPWM_Compare0;
    OpenParams_s.C4.PrescaleFactor = 0x20;
    DebugPrintf(( "Calling STPWM_Open() for Channel 0 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_0] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    /* Start PWM output and then pause for user to set 'scope */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    OpenParams_s.C4.PrescaleFactor = 0x74;
    DebugPrintf(( "Calling STPWM_Open() for Channel 0 ..\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    DebugPrintf(( "Handle returned as %d\n", Handle[STPWM_CHANNEL_1] ));
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    DebugPrintf(( "programming compare functionality... \n"));
    /*enable the compare 0 intt*/

      
    CallbackParam_s.CallBack =(void*)NotifyFunction;
    CallbackParam_s.CallbackData_p = NULL;


    STPWM_InterruptControl(Handle[STPWM_CHANNEL_0],
                                      STPWM_Compare0,TRUE);
	
    DebugPrintf(( "Calling STPWM_SetCallBack() for Channel 0 ..\n" ));
    ReturnError = STPWM_SetCallback(Handle[STPWM_CHANNEL_0],
                                    &CallbackParam_s,
                                    STPWM_Compare0);
     ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
                        
    CallbackParam_s.CallBack =(void *)NotifyFunction2;
    CallbackParam_s.CallbackData_p = NULL;                                    

    STPWM_InterruptControl(Handle[STPWM_CHANNEL_1],
                                     STPWM_Timer,TRUE);

    DebugPrintf(( "Calling STPWM_SetCallBack() for Channel 1 ..\n" ));                                    
    ReturnError = STPWM_SetCallback(Handle[STPWM_CHANNEL_1],
                                    &CallbackParam_s,
                                    STPWM_Timer);   
   ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


     DebugPrintf(( "Commencing Compare Interrupts ...\n"));
                        
    while(i!= 0)
    {
    	ReturnError =  STPWM_GetCMPCPTCounterValue(Handle[STPWM_CHANNEL_0],
                                                   &CounterValue);
                                               
       CounterValue = CounterValue + 0x2faf09;   
	ReturnError = STPWM_SetCompareValue(Handle[STPWM_CHANNEL_0],CounterValue);

	DebugPrintf(( "Value of int count %d\n",IntCount )); 
	    
	clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
	if (STOS_SemaphoreWaitTimeOut(CompareSemaphore_p, &clk) != 0)
	    {
	    	DebugPrintf(("Timed out waiting for callback"));
	    }
	    else
	    {
	        DebugPrintf(("Received callback!\n"));
	       
	    }
	    i --;
    }

    DebugPrintf(( "ticks per sec %d\n", ST_GetClocksPerSecond() ));
    for(i = 1;i < 10; i++)
	{
		DebugPrintf((" time b/w interrupts %ld\n",time1[i+1]- time1[i]));
	}
	
	STOS_TaskDelay(10000);
    DebugPrintf(( "***************************************************************\n"));

    i = 10;

	MarkValue = STPWM_MARK_MAX/2;
	DebugPrintf(( "Commencing Timer Interrupts \n"));

	while(i != 0)
    {
    	
        DebugPrintf(( "Calling STPWM_SetRatio(),MarkValue %02d\n", MarkValue ));
        ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1], (U32)MarkValue );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        DebugPrintf(( "Calling STPWM_GetRatio() for confirmation\n" ));
        ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], &ReturnMrk );
        DebugPrintf(( "MarkValue returned as %02d\n", ReturnMrk ));
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


        DebugPrintf(( "Commencing delay period .............\n" ));
        DebugPrintf(( "----------------------------------------------------------\n" ));
      		     
	   clk = STOS_time_plus(time_now(), 2*ST_GetClocksPerSecond());
	    if (STOS_SemaphoreWaitTimeOut(CompareSemaphore2_p, &clk) != 0)
	    {
	    	DebugPrintf(("Timed out waiting for callback"));
	    }
	    else
	    {
	        DebugPrintf(("Received callback!\n"));
	       
	    }
	    i --;
#if !(defined (ST_7710) || defined(ST_7100))
        STOS_TaskDelay( ShortDelay );
#else
        DELAY(ShortDelay);
#endif 	    
    }
	

    DebugPrintf(( "Calling STPWM_Close() for Channel 0 .\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    DebugPrintf(( "Calling STPWM_Close() for Channel 1.\n" ));
    ReturnError = STPWM_Close( Handle[STPWM_CHANNEL_1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    TermParams_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPWM_Term() unforced .......\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );


    PIOTermPar_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM_COMPARE0_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    PIOTermPar_s.ForceTerminate = FALSE;    
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM1_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    DebugPrintf(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

}
#endif
/****************************************************************************
Name         : ErrantUse()

Description  : Performs API interface calls, generally mis-sequenced,
               or with illegal parameters.  This is intended to
               exercise the error checking built into the routines.
               Note that errors should be raised during this test,
               but these MUST be the expected ones.  If this is the
               case, an overall pass will be reported.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors occurred
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel


See Also     : STLLI_ErrorCode_t
 ****************************************************************************/
void ErrantUse( void )
{
    U32             ChannelNo;
    U32             ReturnMrk;                  /* returned Mark value */
    STPWM_InitParams_t InitParams_s;
    STPWM_OpenParams_t OpenParams_s;
    STPWM_TermParams_t TermParams_s;
    STPWM_Handle_t     Handle[STPWM_CHANNEL_CNT];

    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;
    U32 MaxPWM_Channel;
#if ! (defined (ST_5100) || defined (ST_7710))    
    U32  ReturnCaptureVal; 
    U32  ReturnCompareVal;     
#endif

    DebugPrintf(( "\n" ));
    DebugPrintf(( "==========================================================\n" ));
    DebugPrintf(( "Commencing ErrantUse Test Function ..\n" ));
    DebugPrintf(( "==========================================================\n" ));

    /* this should still be OK following NormalUse
       call, but is re-initialised anyway, in case
       ErrantUse is called first or in isolation...  */
    strcpy( (char*)DeviceName, "STPWM" );

    /* call Term without prior Init call... */
    TermParams_s.ForceTerminate = TRUE;

#if !defined(ST_OSLINUX)
    DebugPrintf(( "Calling STPWM_Term() forced w/o Init \n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );
#endif

    /* set up Init parameters correctly initially */
    InitParams_s.Device = STPWM_C4;

    InitParams_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;
    InitParams_s.C4.InterruptNumber = PWM_CAPTURE_INTERRUPT;
    InitParams_s.C4.InterruptLevel = PWM_INTERRUPT_LEVEL;

#if (STPWM_USES_PIO == TRUE)

    strcpy( (char*)InitParams_s.C4.PIOforPWM0.PortName, PWM0_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM0.BitMask = PWM0_PIO_BIT;
    strcpy( (char*)InitParams_s.C4.PIOforPWM1.PortName, PWM1_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM1.BitMask = PWM1_PIO_BIT;
    
#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
    InitParams_s.C4.PIOforPWM2.PortName[0] = '\0';
    InitParams_s.C4.PIOforPWM2.BitMask = 0;
#else
    strcpy( (char*)InitParams_s.C4.PIOforPWM2.PortName, PWM2_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM2.BitMask = PWM2_PIO_BIT;
#endif
#else

    InitParams_s.C4.PIOforPWM0.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;

#endif


    /* deliberately spike Init calls with a
       single invalid parameter...          */

    InitParams_s.Device = (STPWM_Device_t)((S32)STPWM_C4 - 1); /* hardware device mismatch */
    DebugPrintf(( "Calling STPWM_Init(), unknown device \n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );
    InitParams_s.Device = STPWM_C4; /* correct device */

    DebugPrintf(( "Calling STPWM_Init(), with NULL Name \n" ));
    ReturnError = STPWM_Init( NULL, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    DebugPrintf(( "Calling STPWM_Init(), with NUL Name .\n" ));
    ReturnError = STPWM_Init( "", &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    DebugPrintf(( "Calling STPWM_Init(), with long Name \n" ));
    ReturnError = STPWM_Init( "TooLongDeviceNam", &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    InitParams_s.C4.BaseAddress = NULL; /* "omitted" h/w address */
    DebugPrintf(( "Calling STPWM_Init(), NULL BaseAddr .\n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    InitParams_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;


#if (STPWM_USES_PIO == TRUE)

    /* PIO PortName has not been validated within
       the PWM driver - this really is PIO's job.
       The BitMask in the same STPIO_PIOBit_t
       structures is however checked, because a
       subscript is formed from the bit set within
       the PWM driver implementation. The fault
       has only been applied to one of the three
       bits (replicated code; white box knowledge) */

    InitParams_s.C4.PIOforPWM0.BitMask = MULTI_BITS; /* more than one bit set */
    DebugPrintf(( "Calling STPWM_Init(), multi PIO bits \n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    InitParams_s.C4.PIOforPWM0.BitMask = PWM0_PIO_BIT;

#endif

    /* STPWM_Init has not been called with success
       yet, so the Open calls OUGHT to fail     */

#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
    MaxPWM_Channel = STPWM_CHANNEL_1;
#else
    MaxPWM_Channel = STPWM_CHANNEL_2;
#endif

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
        OpenParams_s.PWMUsedFor = STPWM_Timer;
        OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
        DebugPrintf(( "Calling STPWM_Open(Chan=%1d) w/o Init \n", ChannelNo ));
        ReturnError = STPWM_Open( DeviceName,
                                  &OpenParams_s,
                                  &Handle[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );
    }
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN - 1; /* too low */
    OpenParams_s.C4.ChannelNumber = ChannelNo;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
      
    /* finally, we make a valid Init call.
       Note that the PIO has NOT been
       Initialized yet - this proves that
       the order of Init calls does not
       concern PWM; it only needs PIO
       Initialized before the STPWM_Open
       call will succeed.                 */
    DebugPrintf(( "Calling STPWM_Init(), with no errors \n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    DebugPrintf(( "Calling STPWM_Open(), Prescale too lo\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    
   
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN - 1; 
    OpenParams_s.C4.ChannelNumber = ChannelNo;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    
    DebugPrintf(( "Calling STPWM_Open(), Prescale too hi\n" ));
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MAX + 1; /* too high */
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN; 

#if (STPWM_USES_PIO == TRUE)

    /* we still haven't called STPIO_Init, so an
       otherwise valid STPWM_Open call OUGHT to
       be trapped by an error return from the
       nested STPIO_Open call...                 */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Timer;
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;

#if !defined(ST_OSLINUX)
    DebugPrintf(( "Calling STPWM_Open(%1d) w/o STPIO_Init\n",
                    OpenParams_s.C4.ChannelNumber ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[OpenParams_s.C4.ChannelNumber] );
    ErrorReport( &StoredError, ReturnError, STPWM_ERROR_PIO );
#endif

    /* Initialize the PIO Driver */

    /* Initialize the PIO Driver */
    PIOInitPar_s.BaseAddress = (U32*)PWM0_PIO_PORT_BASE_ADDRESS;
    PIOInitPar_s.InterruptNumber = PWM0_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel = PWM0_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM0_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !(defined (ST_8010) || defined (ST_7109) || defined(ST_7100) || defined(ST_5525) || defined(ST_7200))
#if defined (ST_5100) || defined (ST_7710)  

    PIOInitPar_s.BaseAddress     = (U32*)PWM1_PIO_PORT_BASE_ADDRESS;
    PIOInitPar_s.InterruptNumber = PWM1_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel  = PWM1_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM1_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#else
    if (PWM1_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS)
    {
        PIOInitPar_s.BaseAddress = (U32*)PWM1_PIO_PORT_BASE_ADDRESS;
        PIOInitPar_s.InterruptNumber = PWM1_PIO_PORT_INTERRUPT;
        PIOInitPar_s.InterruptLevel = PWM1_PIO_PORT_INTERRUPT_LEVEL;
        PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
        DebugPrintf(( "Calling STPIO_Init() ................\n" ));
        ReturnError = STPIO_Init( PWM1_PIO_PORT_NAME, &PIOInitPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    if ((PWM2_PIO_PORT_BASE_ADDRESS != PWM1_PIO_PORT_BASE_ADDRESS) &&
        (PWM2_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS))
    {
        PIOInitPar_s.BaseAddress = (U32*)PWM2_PIO_PORT_BASE_ADDRESS;
        PIOInitPar_s.InterruptNumber = PWM2_PIO_PORT_INTERRUPT;
        PIOInitPar_s.InterruptLevel = PWM2_PIO_PORT_INTERRUPT_LEVEL;
        PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
        DebugPrintf(( "Calling STPIO_Init() ................\n" ));
        ReturnError = STPIO_Init( PWM2_PIO_PORT_NAME, &PIOInitPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
#endif     /* End ST_5100 */
#endif

#endif

    /* call SetRatio with the correct (white box)
       handles, but without valid Open calls.
       Handle should be reported as invalid....  */

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )

    {
        DebugPrintf(( "Calling STPWM_SetRatio(%1d) w/o Open call\n", ChannelNo ));
        ReturnError = STPWM_SetRatio( stpwm_PWMEnc[ChannelNo], STPWM_MARK_MIN );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );
    }

    /* call GetRatio with the correct (white box)
       handles, but without valid Open calls.
       Handle should be reported as invalid....  */

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        DebugPrintf(( "Calling STPWM_GetRatio(%1d) w/o Open call\n", ChannelNo ));
        ReturnError = STPWM_GetRatio( stpwm_PWMEnc[ChannelNo], &ReturnMrk );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );
    }

    /* call Close with the correct (white box)
       handles, but without valid Open calls.
       Handle should be reported as invalid....  */

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        DebugPrintf(( "Calling STPWM_Close(%1d) w/o Open call ..\n", ChannelNo ));
        ReturnError = STPWM_Close( stpwm_PWMEnc[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );
    }

    /* call Open with parameter errors...  */

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
        OpenParams_s.PWMUsedFor = STPWM_Timer;
        OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
        DebugPrintf(( "Calling STPWM_Open(%1d) with NULL Name ..\n", ChannelNo ));
        ReturnError = STPWM_Open( NULL,
                                  &OpenParams_s,
                                  &Handle[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    }

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
        DebugPrintf(( "Calling STPWM_Open(%1d) with wrong Name .\n", ChannelNo ));
        ReturnError = STPWM_Open( "Different",
                                  &OpenParams_s,
                                  &Handle[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_UNKNOWN_DEVICE );
    }

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
        OpenParams_s.PWMUsedFor = STPWM_Timer;
        DebugPrintf(( "Calling STPWM_Open(%1d) with NULL OpenPar\n", ChannelNo ));
        ReturnError = STPWM_Open( DeviceName,
                                  NULL,
                                  &Handle[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    }

    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
        DebugPrintf(( "Calling STPWM_Open(%1d) with NULL &Handle\n", ChannelNo ));
        ReturnError = STPWM_Open( DeviceName,
                                  &OpenParams_s,
                                  NULL );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    }

    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_CNT; /* too high */
    DebugPrintf(( "Calling STPWM_Open(), Chan # too high\n" ));
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    /* clean Open, all channels */
    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
	 DebugPrintf(( "Calling STPWM_Open(%1d) with no errors ..\n", ChannelNo ));
        ReturnError = STPWM_Open( DeviceName,
                                  &OpenParams_s,
                                  &Handle[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }

    /* try a second Open, all channels */
    for( ChannelNo = STPWM_CHANNEL_0;
        ChannelNo <= MaxPWM_Channel; ChannelNo++ )
    {
        OpenParams_s.C4.ChannelNumber = ChannelNo;
        DebugPrintf(( "Calling STPWM_Open(%1d) on Open channel .\n", ChannelNo ));
        ReturnError = STPWM_Open( DeviceName,
                                  &OpenParams_s,
                                  &Handle[ChannelNo] );
        ErrorReport( &StoredError, ReturnError, ST_ERROR_NO_FREE_HANDLES );
    }

    /* call SetRatio with wrong handle */
    DebugPrintf(( "Calling STPWM_SetRatio(), wrong Handle ..\n" ));
    ReturnError = STPWM_SetRatio( MULTI_HAND, STPWM_MARK_MIN );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );

    /* call SetRatio with MarkValue outside valid range */
    DebugPrintf(( "Calling STPWM_SetRatio(), low MarkValue .\n" ));
    ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_0],
                                                STPWM_MARK_MIN - 1 );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    DebugPrintf(( "Calling STPWM_SetRatio(), high MarkValue \n" ));
    ReturnError = STPWM_SetRatio( Handle[STPWM_CHANNEL_1],
                                                STPWM_MARK_MAX + 1 );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    /* call GetRatio with wrong handle */
    DebugPrintf(( "Calling STPWM_GetRatio(), wrong Handle ..\n" ));
    ReturnError = STPWM_GetRatio( MULTI_HAND, &ReturnMrk );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );

    /* call GetRatio with NULL Mark return pointer */
    DebugPrintf(( "Calling STPWM_GetRatio(), NULL Mark ptr. \n" ));
    ReturnError = STPWM_GetRatio( Handle[STPWM_CHANNEL_1], NULL );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );

    /* call Term with Open Handles */
    TermParams_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPWM_Term() with open handle\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_OPEN_HANDLE );

    /* finally a clean Term call to clean up */
    TermParams_s.ForceTerminate = TRUE;
    DebugPrintf(( "Calling STPWM_Term(), forced ........\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if (STPWM_USES_PIO == TRUE)

    PIOTermPar_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM0_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

#if !(defined (ST_8010) || defined (ST_7109) || defined(ST_7100) || defined(ST_5525) || defined(ST_7200))
#if defined (ST_5100) || defined (ST_7710) 
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM1_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
#else

    if (PWM1_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS)
    {
        DebugPrintf(( "Calling STPIO_Term() ................\n" ));
        ReturnError = STPIO_Term( PWM1_PIO_PORT_NAME, &PIOTermPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
    if ((PWM2_PIO_PORT_BASE_ADDRESS != PWM1_PIO_PORT_BASE_ADDRESS) &&
        (PWM2_PIO_PORT_BASE_ADDRESS != PWM0_PIO_PORT_BASE_ADDRESS))
    {
        DebugPrintf(( "Calling STPIO_Term() ................\n" ));
        ReturnError = STPIO_Term( PWM2_PIO_PORT_NAME, &PIOTermPar_s );
        ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    }
#endif  
#endif /* End ST_5100 */
#endif
  
    /* Compare and Capture tests*/
    /* set up Init parameters correctly initially */
#if ! (defined (ST_5100) || defined (ST_7710))    
    PIOInitPar_s.BaseAddress = (U32*)PWM_COMPARE0_PIO_PORT_BASE_ADDRESS;
    PIOInitPar_s.InterruptNumber = PWM_COMPARE0_PIO_PORT_INTERRUPT;
    PIOInitPar_s.InterruptLevel = PWM_COMPARE0_PIO_PORT_INTERRUPT_LEVEL;
    PIOInitPar_s.DriverPartition = TEST_PARTITION_2;
    DebugPrintf(( "Calling STPIO_Init() ................\n" ));
    ReturnError = STPIO_Init( PWM_COMPARE0_PIO_PORT_NAME, &PIOInitPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    InitParams_s.Device = STPWM_C4;
    InitParams_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;
    InitParams_s.C4.InterruptNumber = PWM_CAPTURE_INTERRUPT;
    InitParams_s.C4.InterruptLevel = PWM_INTERRUPT_LEVEL;

#if (STPWM_USES_PIO == TRUE)
    strcpy( (char*)InitParams_s.C4.PIOforPWM0.PortName, PWM_COMPARE0_PIO_PORT_NAME);
    InitParams_s.C4.PIOforPWM0.BitMask = PWM_COMPARE0_PIO_BIT;
    strcpy( (char*)InitParams_s.C4.PIOforPWM1.PortName, PWM_CAPTURE1_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM1.BitMask = PWM_CAPTURE1_PIO_BIT;
    
#if defined (ST_5100) || defined (ST_7710) || defined (ST_8010)  || defined (ST_7109) || defined(ST_7100) ||\
                                                                                         defined(ST_5525) || defined(ST_7200)
    InitParams_s.C4.PIOforPWM2.PortName[0] = '\0';
    InitParams_s.C4.PIOforPWM2.BitMask = 0;
#else
    strcpy( (char*)InitParams_s.C4.PIOforPWM2.PortName, PWM2_PIO_PORT_NAME );
    InitParams_s.C4.PIOforPWM2.BitMask = PWM2_PIO_BIT;
#endif
#else

    InitParams_s.C4.PIOforPWM0.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;
    InitParams_s.C4.PIOforPWM1.BitMask = 0;

#endif

    /* deliberately spike Init calls with a
       single invalid parameter...          */

    InitParams_s.Device = (STPWM_Device_t)((S32)STPWM_C4 - 1); /* hardware device mismatch */
    DebugPrintf(( "Calling STPWM_Init(), unknown device \n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_FEATURE_NOT_SUPPORTED );
    InitParams_s.Device = STPWM_C4; /* correct device */
    
    strcpy( (char*)DeviceName, "STPWM" );
    DebugPrintf(( "Calling STPWM_Init(), with no errors \n" ));
    ReturnError = STPWM_Init( DeviceName, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
        
    DebugPrintf(( "Calling STPWM_Open() for Compare, Prescale too lo\n" ));
    OpenParams_s.C4.PrescaleFactor = STPWM_CPTCMP_PRESCALE_MIN - 1; /* too low */
    OpenParams_s.C4.ChannelNumber = STPWM_CHANNEL_1;
    OpenParams_s.PWMUsedFor = STPWM_Compare1;
    
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    OpenParams_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
    
    DebugPrintf(( "Calling STPWM_Open(), Prescale too hi\n" ));
    OpenParams_s.C4.PrescaleFactor = STPWM_CPTCMP_PRESCALE_MAX + 1; /* too high */
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_0] );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );
    OpenParams_s.C4.PrescaleFactor = STPWM_CPTCMP_PRESCALE_MIN; 
  
    DebugPrintf(( "Calling STPWM_Open(),w/o errors \n" ));
    OpenParams_s.PWMUsedFor = STPWM_Capture1;
    ReturnError = STPWM_Open( DeviceName,
                              &OpenParams_s,
                              &Handle[STPWM_CHANNEL_1] );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    DebugPrintf(( "Calling STPWM_SetCaptureEdge(),with wrong edge value \n" ));
    ReturnError = STPWM_SetCaptureEdge(Handle[STPWM_CHANNEL_1],
    								   STPWM_RisingOrFallingEdge + 1);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_BAD_PARAMETER );                                   

    DebugPrintf(( "Calling STPWM_SetCaptureEdge(),with Correct edge value \n" ));
    ReturnError = STPWM_SetCaptureEdge(Handle[STPWM_CHANNEL_1],
    								   STPWM_RisingOrFallingEdge);
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR ); 
    
    DebugPrintf(( "Calling STPWM_GetCaptureValue() with wrong handle\n" ));
    ReturnError = STPWM_GetCaptureValue(Handle[STPWM_CHANNEL_0],&ReturnCaptureVal);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );                                   

    DebugPrintf(( "Calling STPWM_CounterControl() with wrong handle\n" ));
    ReturnError = STPWM_CounterControl(Handle[STPWM_CHANNEL_0],
                                       STPWM_Capture0,
                                       FALSE);
    ErrorReport( &StoredError, ReturnError, ST_ERROR_INVALID_HANDLE );   
    
    DebugPrintf(( "Calling STPWM_CounterControl() with no errors\n" ));
    ReturnError = STPWM_CounterControl(Handle[STPWM_CHANNEL_1],
                                      		   STPWM_Capture1,
                                       				  FALSE); 
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );                                          				  
    DebugPrintf(( "Calling STPWM_GetCMPCPTCounterValue() with no errors\n" ));
    ReturnError = STPWM_GetCMPCPTCounterValue(Handle[STPWM_CHANNEL_1],
                                             &ReturnCompareVal);  
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );    
                                                                                    				    
    /* call Term with Open Handles */
    TermParams_s.ForceTerminate = FALSE;
    DebugPrintf(( "Calling STPWM_Term() with open handle\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_ERROR_OPEN_HANDLE );

    /* finally a clean Term call to clean up */
    TermParams_s.ForceTerminate = TRUE;
    DebugPrintf(( "Calling STPWM_Term(), forced ........\n" ));
    ReturnError = STPWM_Term( DeviceName, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR ); 
    
    DebugPrintf(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PWM_COMPARE0_PIO_PORT_NAME, &PIOTermPar_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );                       				                                             
#endif
    DebugPrintf(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}

/****************************************************************************
Name         : ErrorReport()

Description  : Outputs the appropriate error message string.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : STLLI_ErrorCode_t
 ****************************************************************************/

void ErrorReport( ST_ErrorCode_t *ErrorStore,
                  ST_ErrorCode_t ErrorGiven,
                  ST_ErrorCode_t ExpectedErr )
{
    switch( ErrorGiven )
    {
        case ST_NO_ERROR:
            DebugPrintf(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            DebugPrintf(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            DebugPrintf(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            DebugPrintf(( "ST_ERROR_INVALID_HANDLE - Handle number not recognized\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            DebugPrintf(( "ST_ERROR_OPEN_HANDLE - unforced Term with handle open\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            DebugPrintf(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            DebugPrintf(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            DebugPrintf(( "ST_ERROR_NO_FREE_HANDLES - Channel already Open\n" ));
            break;

        case STPWM_ERROR_PIO:
            DebugPrintf(( "STPWM_ERROR_PIO - Error occured in the PIO sub component\n" ));
            break;

       default:
            DebugPrintf(( "*** Unrecognised return code ***\n" ));
    }

    DebugPrintf(( "----------------------------------------------------------\n" ));

    /* if ErrorGiven does not match ExpectedErr, update
       ErrorStore only if no previous error was noted    */
    if( ErrorGiven != ExpectedErr )
    {
        if( *ErrorStore == ST_NO_ERROR )
        {
            *ErrorStore = ErrorGiven;
        }
    }
}


/* End of stpwmtst.c module */


