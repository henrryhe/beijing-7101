/*****************************************************************************
File Name   : stboot.c

Description : System Boot

              This library is responsible for performing OS20/OS21
              (ST20, ST40, ST200 devices) specific and backend processor
              initialization.

Copyright (C) 1999-2005 STMicroelectronics

History     :

        8/99  First attempt
    07/09/99  Addition of SDRAM initialization code for 5510
              Toolset IO mutex functions now compile-time defined for
              different toolset versions
    13/09/99  API revised - dcache type changed
              Partition initialization removed - now responsibility of
              application
              New API functions GetBackendInfo() and GetRevision().
    14/09/99  Split boot setup into modules; sdram.c, cache.c and
              stboot.c.
    20/09/99  Addition of sdram init. params to API.
              Removed local definition of SDRAM_FREQUENCY.
              Removed ``repeated'' config. of audio clock interface in
              MPEGInit() routine.  This is now done in sdram.c via
              ConfigurePLL().
    21/12/99  Patch release to address PLL already initialized problem
              if SDRAM is initialized in .cfg files.
    08/08/00  Addition of EPLD and FPGA initialization for sti7015.
    03/10/00  Move initialisation into 7015 target directory.
    14/12/00  Remove not backend initialisation (FPGA & EPLD).
              Add InitSDRAM7015 after BackendInit7015.
    10/09/04  Added support for STi7710 and ST20 C1 timer. Added support for
              use with OS21.
    19/11/04  Added support for STi5105.
    10/02/05  Added support for STi7100, STm5700.
    20/04/05  Added support for STi5301, STm8010.
    12/05/05  Changed ST200 specific dcache handling.
    25/10/05  Added support for STi7109.
    18/11/05  Added support for STi5188.
    10/01/06  Added support for STx5525.
    13/04/06  Added support for STx5107.

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/* override makefile forcing of 70xx addresses; this file needs the frontend
  ones instead (INTERRUPT_...) */
#undef REMOVE_GENERIC_ADDRESSES

#ifdef ST_OS20
#include <device.h>
#endif

#include "stlite.h"             /* STLite includes */
#include "stbootcache.h"

#ifdef ST_OS20
#include "debug.h"
#endif

#ifdef ST_OS21
#include "os21debug.h"

#ifdef ARCHITECTURE_ST200
#include <machine/sysconf.h>
#include <machine/rtrecord.h>
#include <machine/bsp.h>
#include <sys/bitfields.h>
#include <machine/ctrlreg.h>
#endif
#endif

#include "stdevice.h"           /* Device includes */

#include "stboot.h"             /* STAPI includes */
#include "stcommon.h"
#include "stsys.h"

#ifdef ARCHITECTURE_ST20        /* Local includes */
#include "sdram.h"

#if defined(ST_7015) || defined (ST_7020)
#include "be_7015.h"            /* specific 7015/7020 backend init */
#endif

#endif /*#ifdef ARCHITECTURE_ST20*/

#include "be_init.h"

/* Private types/constants ------------------------------------------------ */

#ifdef ARCHITECTURE_ST20

/* The setup of mutual exclusion for the debug library depends on the compiler
   version. For V1.7+:
   - the mutual exclusion callback has a 2nd parameter, which is a pointer to
     user data (pre-V1.7, the function had only 1 parameter).
   - the callback function has no static link, & should be marked as
     #pragma ST_nolink. */

#if (__ICC_VERSION_NUMBER__ < 50700)
#define OLD_MUTEX_FN
#endif

#ifndef OLD_MUTEX_FN
typedef struct
{
    int                         *mutex_data_sl;
    void                        *mutex_data_user;
} mutex_data_t;
#endif

/* Interrupt control structure */
typedef struct
{
    char                        *stack_base;
    size_t                      stack_size;
    interrupt_flags_t           flags;
} local_interrupt_control_t;


/* The chip-specific header file may override this definition to modify
 * the trigger mode used for the various interrupt levels on the ILC.
 * The default behaviour is specified here as 'high_level' and will
 * be used otherwise.
 */
#ifndef INTERRUPT_TRIGGER_MODE

/* Workaround for STi5510 silicon */
#if defined(ST_5510)

#define INTERRUPT_TRIGGER_MODE(level)  (((level==VIDEO_INTERRUPT_LEVEL) \
                                      || (level==AUDIO_INTERRUPT_LEVEL)) \
                                     && ((device_id().st.revision) == 0) \
                                     && ((device_id().st.device_code) == 0xCD) \
                                     ? interrupt_trigger_mode_falling : \
                                       interrupt_trigger_mode_high_level)

#elif defined(mb295) || defined(mb290)

#define INTERRUPT_TRIGGER_MODE(level)  ((level == EXTERNAL_1_INTERRUPT_LEVEL) \
                                       ? interrupt_trigger_mode_low_level : \
                                         interrupt_trigger_mode_high_level)

#elif defined(ST_7020) /* db573 7020 STEM board */

#define INTERRUPT_TRIGGER_MODE(level)  ((level == EXTERNAL_0_INTERRUPT_LEVEL) \
                                       ? interrupt_trigger_mode_low_level : \
                                         interrupt_trigger_mode_high_level)
                                         
#elif defined(mb395)

#define INTERRUPT_TRIGGER_MODE(level)  ((level == ATA_INTERRUPT_LEVEL) \
                                        ? interrupt_trigger_mode_low_level : \
                                         interrupt_trigger_mode_high_level)

#else

#define INTERRUPT_TRIGGER_MODE(level)   interrupt_trigger_mode_high_level

#endif
#endif /* INTERRUPT_TRIGGER_MODE */

/* Default interrupt level stack size */
#if defined (STBOOT_DEFAULT_ISR_STACK_SIZE)
#define DEFAULT_STACK_SIZE              STBOOT_DEFAULT_ISR_STACK_SIZE
#else
#define DEFAULT_STACK_SIZE              1024
#endif

/* DeviceType for STi5512 Revs F & G */
#define DEVICE_5512_F                   0x06
#define DEVICE_5512_G                   0x07

/* Private variables ------------------------------------------------------ */

#if (OS20_VERSION_MAJOR < 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR <= 8))
/* This semaphore ensures mutual exclusion during debug output */
static semaphore_t mutex_sem;
#endif

/* Interrupt setup */
static char interrupt_level_0_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_1_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_2_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_3_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_4_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_5_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_6_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_7_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_8_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_9_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_10_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_11_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_12_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_13_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_14_stack[ DEFAULT_STACK_SIZE ];
static char interrupt_level_15_stack[ DEFAULT_STACK_SIZE ];

static local_interrupt_control_t interrupt_control[] =
{
    { interrupt_level_0_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_1_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_2_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_3_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_4_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_5_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_6_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_7_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_8_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_9_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_10_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_11_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_12_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_13_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_14_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority },
    { interrupt_level_15_stack, DEFAULT_STACK_SIZE, interrupt_flags_low_priority }
};


#endif /*#ifdef ARCHITECTURE_ST20*/

/* STBOOT revision number */
static const ST_Revision_t Revision = "STBOOT-REL_2.18.2";

/* Private macros --------------------------------------------------------- */

#define InitialiseStack(_base_, _size_)    { \
    unsigned int _i_; for (_i_=0;_i_<_size_ / sizeof(int);_i_++) \
        ((unsigned int*)_base_)[_i_] = 0xaaaaaaaa;\
}

/* Private function prototypes -------------------------------------------- */

#ifdef ARCHITECTURE_ST20

static ST_ErrorCode_t InitInterrupts(const STBOOT_InitParams_t *InitParams_p);

#if (OS20_VERSION_MAJOR < 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR <= 8))

/* The following routines prevent pre-emption of debug message output */
static void init_debugop_protection(void);


#ifdef OLD_MUTEX_FN
static long int mutex_fn (debugmutexop op);

#else
static long int mutex_fn (debugmutexop op, void *dummy);

static long int mutex_wrapper (debugmutexop op, mutex_data_t* mutex_data);
#pragma ST_nolink(mutex_wrapper)

#endif

#endif /* (OS20_VERSION_MAJOR < 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR <= 8))*/

#endif /*#ifdef ARCHITECTURE_ST20*/

/* API functions ---------------------------------------------------------- */

/*****************************************************************************
Name: STBOOT_Init()
Description:
    Initializes system caches, SDRAM and the OS kernel.

Parameters:
    DeviceName,     device name associated with the boot.
    InitParams_p,   the initialization parameters.

Return Value:
    ST_NO_ERROR                     No Errors during operation.
    STBOOT_ERROR_KERNEL_INIT        Error Initialising the Kernel.
    STBOOT_ERROR_KERNEL_START       Error Starting the Kernel.
    STBOOT_ERROR_INTERRUPT_INIT     Error Initilising Interrupts.
    STBOOT_ERROR_INTERRUPT_ENABLE   Error Enabling Interrupts.
    STBOOT_ERROR_UNKNOWN_BACKEND    The backend type is not recognized.
    STBOOT_ERROR_BACKEND_MISMATCH   The backend type does not match.
    STBOOT_ERROR_INVALID_DCACHE     Invalid dcache map.
    STBOOT_ERROR_SDRAM_INIT         Failed to setup SDRAM.
    STBOOT_ERROR_BSP_FAILURE        Error from toolset BSP.
See Also:
    STBOOT_Term()
*****************************************************************************/

ST_ErrorCode_t STBOOT_Init( const ST_DeviceName_t DeviceName,
                            const STBOOT_InitParams_t *InitParams_p )
{
ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
/************************ FOR OS20 ******************************/
#ifdef ST_OS20

#ifdef ST_5100
S8 Error = 0;
#endif

#if defined(PROCESSOR_C1)
    int c1_timer_initialize(void);
#endif
    
#if !defined(ST_5100)
    /* Initialize instruction cache before starting kernel */
    stboot_InitICache( (CacheReg_t *)InitParams_p->CacheBaseAddress,
                       InitParams_p->ICacheEnabled );

    /* Initialize data cache */
    if ( stboot_InitDCache( (CacheReg_t *)InitParams_p->CacheBaseAddress,
                            InitParams_p->DCacheMap ) )
    {
        return STBOOT_ERROR_INVALID_DCACHE;
    }
    stboot_LockCaches( (CacheReg_t *)InitParams_p->CacheBaseAddress );
#endif

#if !defined (OS20_RUNTIME)
    /* Initialize the kernel */
    if (kernel_initialize() != 0)
    {   
        return STBOOT_ERROR_KERNEL_INIT;
    }
    else if (kernel_start() != 0)       /* Start the kernel */
    {       
        return STBOOT_ERROR_KERNEL_START;
    }
#endif

#if defined(ST_5100) /*cache config for 5100 only, due to dcache bug*/
    
    Error = cache_init_controller((void *)InitParams_p->CacheBaseAddress, cache_map_c2_c200);
    if(Error != 0)
    {
    	return STBOOT_ERROR_BSP_FAILURE;
    }
    
    if(InitParams_p->ICacheEnabled)
    {
    	#ifdef STBOOT_LMI_TOP_REGION_ICACHE_ENABLE
    	Error = cache_config_instruction ((void *)0x00000000, (void *)0xffffffff, cache_config_enable);
    	#else
    	Error = cache_config_instruction ((void *)0xC0000000, (void *)0xC07FFFFF, cache_config_enable);
    	Error |= cache_config_instruction ((void *)0x40000000, (void *)0x7FFFFFFF, cache_config_enable);
    	#endif
        if(Error != 0)
        {
    		return STBOOT_ERROR_BSP_FAILURE;
        }
    
        Error = cache_enable_instruction(); /*typically cache should be enabled AFTER an invalidate call. OS20 takes care of this when using this call*/
        if(Error != 0)
        {
    		return STBOOT_ERROR_BSP_FAILURE;
        }
    }
    
    if ( stboot_InitDCache( (CacheReg_t *)InitParams_p->CacheBaseAddress, InitParams_p->DCacheMap ) )
    {
        return STBOOT_ERROR_INVALID_DCACHE;
    }
#endif

    /* Initialize interrupts and ILC */
    ErrorCode = InitInterrupts(InitParams_p);
    if(ErrorCode != ST_NO_ERROR)
    {
        return ErrorCode;
    }

#if defined(PROCESSOR_C1)
    c1_timer_initialize(); /*present in file st_c1_timer.c*/
    time_ticks_per_sec_set( (clock_t) STCOMMON_ST20TICK_CLOCK ); /*required for GNBvd56186 for new toolsets*/
#endif
    
#if (OS20_VERSION_MAJOR < 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR <= 8))
    /* Debug output semaphore protection */
    init_debugop_protection();
#endif
    
#ifndef mb295
    /* Initialize 55xx MPEG backend */
    ErrorCode = stboot_BackendInit(NULL);
    if ( ErrorCode != ST_NO_ERROR )
    {
        return ErrorCode;
    }
#if !defined(ST_5514) && !defined(ST_5516) && !defined(ST_5517) && !defined(ST_5528) && !defined(ST_5100) && !defined(ST_5101) && !defined(ST_7710) && !defined(ST_5105) && !defined(ST_5700) && !defined(ST_5188) && !defined(ST_5107) /* [ */  /* SMI is initialized in the cfg file */
    /* Initialise SMI SDRAM if size is non zero */
    switch( InitParams_p->MemorySize )
    {
        case STBOOT_DRAM_MEMORY_SIZE_0:
            /* No initialization required */
            /* This is not an error */
            break;

        case STBOOT_DRAM_MEMORY_SIZE_16_MBIT:
        case STBOOT_DRAM_MEMORY_SIZE_32_MBIT:
        case STBOOT_DRAM_MEMORY_SIZE_64_MBIT:
            if(stboot_InitSDRAM(InitParams_p->SDRAMFrequency,
                                InitParams_p->MemorySize) )
            {
                return STBOOT_ERROR_SDRAM_INIT;
            }
            break;
                
        default:
            /* Note: we only range check MemorySize
               if it is required for this platform */
            return ST_ERROR_BAD_PARAMETER;
            break;
    }

#endif /* !ST_5514 !ST_5516 !ST_5517 !ST_5528 !ST_5100 !ST_5101 !ST_7710 !ST_5105 !ST_5700 !ST_5188 !ST_5107 defined ] */
#endif /* !mb295 */

#if defined(mb295) || defined (mb290)
    /* Initialize MPEG backend; 7020 STEM requires reset via I2C,
      so is deferred until STBOOT_Init7020STEM */
      
    ErrorCode = stboot_BackendInit7015(InitParams_p->SDRAMFrequency);
    if ( ErrorCode != ST_NO_ERROR )
    {
        return ErrorCode;
    }
#endif

    return ErrorCode;
#endif /*#ifdef ST_OS20*/

/************************ FOR OS21 ******************************/
#ifdef ST_OS21
    
    /*ICache config for ST200*/
    #ifdef ARCHITECTURE_ST200
    if(InitParams_p->ICacheEnabled == FALSE)
    {
    	return ST_ERROR_FEATURE_NOT_SUPPORTED; /*Icache not to be disabled on ST200 devices*/
    }
    #endif
    
    /*DCache config for ST200*/
    #ifdef ARCHITECTURE_ST200
    #ifdef STBOOT_SPECIFY_HOST_MEMORY_MAP
    ErrorCode = stboot_st200_InitDCache_Specified_Host_Map((STBOOT_InitParams_t *) InitParams_p);
    #else
    ErrorCode = stboot_st200_InitDCache((STBOOT_InitParams_t *) InitParams_p);
    #endif
    if (ErrorCode != ST_NO_ERROR)
    {
        return ErrorCode;
    }
    #endif
    
    /* Initialize & start the OS21 kernel */
    if (kernel_initialize(NULL) != 0)
    {
        return STBOOT_ERROR_KERNEL_INIT;
    }
    else if (kernel_start() != 0)       /* Start the kernel */
    {
        return STBOOT_ERROR_KERNEL_START;
    }
    
    /*ICache config for ST40 */
    #ifdef ARCHITECTURE_ST40
    if(InitParams_p->ICacheEnabled == FALSE)
    {
    	cache_disable_instruction();
    }
    #endif
    
    /*DCache config for ST40 */
    #ifdef ARCHITECTURE_ST40
    if((InitParams_p->DCacheMap == NULL) || (InitParams_p->DCacheEnabled == FALSE))
    {
    	cache_disable_data(1); /*1 means data cache will be flushed before being disabled*/
    }
    #endif
    
    ErrorCode = stboot_BackendInit(NULL);
    if ( ErrorCode != ST_NO_ERROR )
    {
        return ErrorCode;
    }
    
    /*Timeslicing config for OS21, by default gets enabled*/
    if (InitParams_p->TimeslicingEnabled)
    {
    	kernel_timeslice(OS21_TRUE);
    }
    
    #ifdef ARCHITECTURE_ST200
    /*The following calls display the current memory map in a table format, can be enabled for debug*/
    #ifdef STBOOT_DISPLAY_BSP_MEMORY_MAP
    printf("\n\nUser requested dcache configuration retained (post OS21 startup):\n");
    printf("-----------------------------------------------------------------------\n");
    bsp_mmu_dump_TLB_Settings();
    bsp_scu_dump_SCU_Settings();
    printf("-----------------------------------------------------------------------\n\n");
    #endif
    #endif

    return ErrorCode;
#endif /*#ifdef ST_OS21*/
} /* STBOOT_Init() */


/* STBOOT_Init7020STEM - see sti7015/be_7015.c */

/*****************************************************************************
Name: STBOOT_GetBackendInfo()
Description:
    Obtains backend type and cut revision information.
Parameters:
    BackendInfo_p,      pointer to backend information datastructure.
Return Value:
    ST_NO_ERROR,        the operation completed without error.
See Also:
    Global table BackendLUT.
*****************************************************************************/

ST_ErrorCode_t STBOOT_GetBackendInfo(STBOOT_Backend_t *BackendInfo_p)
{
#if defined (ST_OS21)

    /* Assume unrecognised device code */
    BackendInfo_p->DeviceType    = STBOOT_DEVICE_UNKNOWN;
    BackendInfo_p->MajorRevision = STBOOT_REVISION_UNKNOWN;
    BackendInfo_p->MinorRevision = STBOOT_REVISION_UNKNOWN;
#endif

#if defined (ST_OS20)
    {
        int DeviceType;

        /* Use device_id() to obtain info */
        DeviceType = device_id().st.device_code;
    
        /* Workaround for STi5512 - not all revisions
           of 5512 have the same device_code */
        if( (DeviceType == DEVICE_5512_F ) ||
            (DeviceType == DEVICE_5512_G ) )
        {
            DeviceType = (int) STBOOT_DEVICE_5512;
        }

        BackendInfo_p->DeviceType    = (STBOOT_Device_t) DeviceType;
        BackendInfo_p->MajorRevision = device_id().st.revision;
        BackendInfo_p->MinorRevision = STBOOT_REVISION_UNKNOWN;
    }
#endif

    return ST_NO_ERROR;
}

/*****************************************************************************
Name: STBOOT_GetRevision()
Description:
    Returns the driver revision number -- may be called at any time.
Parameters:
    None.
Return Value:
    ST_Revision_t.
See Also:
    Nothing.
*****************************************************************************/

ST_Revision_t STBOOT_GetRevision(void)
{
    return Revision;
} /* STBOOT_GetRevision() */

/*****************************************************************************
Name: STBOOT_Term()
Description:
    Dummy function to comply with ST API
Parameters:
    DeviceName,     device name of boot to terminate.
    TermParams_p,   the termination parameters.
Return Value:
    ST_NO_ERROR,                   No Errors during operation
See Also:
    STBOOT_Init()
*****************************************************************************/

ST_ErrorCode_t STBOOT_Term( const ST_DeviceName_t DeviceName,
                            const STBOOT_TermParams_t *TermParams_p )
{
    return ST_NO_ERROR;
} /* STBOOT_Term() */

/* Debug routines --------------------------------------------------------- */

#ifdef ARCHITECTURE_ST20

#if (OS20_VERSION_MAJOR < 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR <= 8))

/*****************************************************************************
Name: mutex_fn()
Description:
    Function to prevent pre-emption of debug message output.
    As mentioned above, older versions of the compiler had no 2nd parameter.
Parameters:
    op                             mutex operation.
    dummy                          Not used.
Return Value:
    Not used.
See Also:
    init_debugop_protection
*****************************************************************************/

#ifdef OLD_MUTEX_FN
static long int mutex_fn(debugmutexop op)
#else
static long int mutex_fn(debugmutexop op, void *dummy)
#endif
{
    int enables; 
#ifdef C1_CORE 
    const int mask = 0xc000; 
    __asm { ld 0; statusset; st enables; } 
#else 
    const int mask = 255; 
    __asm { ld 0; gintdis; st enables; } 
#endif 

    if (enables != (enables & mask)) { return 0; } 
    if (task_context(NULL, NULL) == task_context_interrupt) { return 0; } 
    
    switch (op)
    {
        case debugmutexop_lock:
            (void) semaphore_wait (&mutex_sem);
            break;
        case debugmutexop_unlock:
            semaphore_signal (&mutex_sem);
            break;
        case debugmutexop_loop:
            task_delay(ST_GetClocksPerSecond()/10000);
            break;
    }
    return 0;
} /* mutex_fn() */

#ifndef OLD_MUTEX_FN
/*****************************************************************************
Name:   mutex_wrapper()
Description:
    Convoluted way to call a function with a static link from one without
    one, without having to put them in different modules. This function has
    no static link. Instead, the static link value is passed in the parameters
    set up in init_debugop_protection(). So we can uses this to call
    mutex_fn() in the correct way. Thus this function just acts as a wrapper
    for mutex_fn().

    This function is not needed with older version of the toolset (see above).

Parameters:
    op                             mutex operation.
    mutex_data
Return Value:
    Not used.
See Also:
    init_debugop_protection
*****************************************************************************/

static long int mutex_wrapper(debugmutexop op, mutex_data_t* mutex_data)
{
    long (*fn)(int*, debugmutexop, void *);
    #pragma ST_nolink(fn)

    fn = (long (*)(int*, debugmutexop, void*))mutex_fn;
    return fn (mutex_data->mutex_data_sl, op, mutex_data->mutex_data_user);
} /* mutex_wrapper() */
#endif

/*****************************************************************************
Name: init_debugop_protection()
Description:
    Function to prevent pre-emption of debug message output
Parameters:
    none
Return Value:
    none
See Also:
    mutex_fn(), mutex_wrapper()
*****************************************************************************/

static void init_debugop_protection(void)
{
    semaphore_init_fifo (&mutex_sem, 1);

#ifdef OLD_MUTEX_FN
    debugsetmutexfn (mutex_fn);
#else
    {
      /* The callback function needs to access static data, but has no
         static link set up. So we need to emulate one, by passing its value
         in the structure mutex_data. This structure also has a field for a
         user data ptr, but this is not used at present. */

      static mutex_data_t mutex_data;
      extern int* _params;

      mutex_data.mutex_data_sl = (int*)_params[1];  /* static link */
      mutex_data.mutex_data_user = NULL;

      debugsetmutexfn((long(*)(debugmutexop, void*))mutex_wrapper, &mutex_data);
    }
#endif
} /* init_debugop_protection() */

#endif /* (OS20_VERSION_MAJOR < 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR <= 8)) */

/*****************************************************************************
Name: InitInterrupts()
Description:
    Function initialise interrupts. Interrupt controller is initialized,
    each interrupt level is then initialised. Upon completion each int
    level is disabled.
Parameters:
    Pointer to IntTriggerMask and IntTriggerTable_p when required
Return Value:
    none
See Also:
    none
*****************************************************************************/

static ST_ErrorCode_t InitInterrupts(const STBOOT_InitParams_t *InitParams_p)
{
    U32     i;
    interrupt_trigger_mode_t trigger_mode;

    /* Initialize interrupts */
    interrupt_init_controller( (void *)INTERRUPT_CONTROLLER_BASE,
                               INTERRUPT_LEVELS,
                               (void *)INTERRUPT_LEVEL_CONTROLLER_BASE,
                               INTERRUPT_NUMBERS,
                               INTERRUPT_INPUTS_OFFSET );

    for (i=0; i < INTERRUPT_LEVELS; i++)
    {
        /* Write known data to all ISR stack areas for debugging */
        InitialiseStack( interrupt_control[i].stack_base,
                         interrupt_control[i].stack_size );

#if defined(STBOOT_CONFIGURABLE_INT_TRIGGER)
        trigger_mode = (((InitParams_p->IntTriggerMask & (1<<i))==0)?
                        (INTERRUPT_TRIGGER_MODE(i)):
                        (InitParams_p->IntTriggerTable_p[i]));
#else
        trigger_mode = INTERRUPT_TRIGGER_MODE(i);
#endif
                
        if (interrupt_init( i,
                            interrupt_control[i].stack_base,
                            interrupt_control[i].stack_size,
                            trigger_mode,
                            interrupt_control[i].flags) )
        {
            return STBOOT_ERROR_INTERRUPT_INIT;
        }

#ifndef STAPI_INTERRUPT_BY_NUMBER /*for GNBvd44968 & GNBvd45179*/
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_5101) || defined(ST_7710) || defined(ST_5105) || defined(ST_5700) || defined(ST_5188) || defined(ST_5107)
        /* Devices with ILC-3 have each interrupt level
           disabled for backwards comptibility */
        if (interrupt_disable (i))
        {
            return STBOOT_ERROR_INTERRUPT_INIT;
        }
#endif
#endif
    }

    /* From DCU 1.8.1 (STLite/OS20 2.07) onwards the method for
       enable/disable of global interrupts changed */
#if (OS20_VERSION_MAJOR > 2) || ((OS20_VERSION_MAJOR == 2) && (OS20_VERSION_MINOR >= 7))

    interrupt_enable_global();                 /* void return */
#else
    if( interrupt_enable(INTERRUPT_GLOBAL_ENABLE) != 0 )
    {
        return STBOOT_ERROR_INTERRUPT_ENABLE;
    }
#endif

#ifndef STAPI_INTERRUPT_BY_NUMBER  /*for GNBvd44968 & GNBvd45179*/
#if defined(ST_5514) || defined(ST_5516) || defined(ST_5517) || defined(ST_5528) || defined(ST_5100) || defined(ST_5101) || defined(ST_7710) || defined(ST_5105) || defined(ST_5700) || defined(ST_5188) || defined(ST_5107)
    /* Devices with ILC-3 have each interrupt number enabled
       for backwards compatibility with earlier ILCs */
    for (i=0; i<INTERRUPT_NUMBERS; i++)
    {
        if (interrupt_enable_number(i) != 0)
        {
            return STBOOT_ERROR_INTERRUPT_ENABLE;
        }
    }
#endif
#endif

    return ST_NO_ERROR;
}

#endif /*#ifdef ARCHITECTURE_ST20*/

/* End of stboot.c */
