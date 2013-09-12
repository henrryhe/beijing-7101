/****************************************************************************

File Name   : stclkrvi.c

Description : Clock Recovery API Interactive Test Routine

Copyright (C) 2003, ST Microelectronics

References  :

$ClearCase (VOB: stclkrv)

stclkrv.fm "Clock Recovery API" Reference DVD-API-072 Revision 4.2

 ****************************************************************************/


/* Includes --------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>        /* Needed for print if STTBX fails to init */
#include <ostime.h>
#include <string.h>

#ifdef DVD_TRANSPORT_STPTI
#include "stpti.h"
#endif

#include "stboot.h"
#include "stclkrv.h"
#include "stcommon.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stevt.h"
#include "stlite.h"
#include "stpio.h"
#include "stpwm.h"
#include "sttbx.h"


/* Private Types ---------------------------------------------------------- */
#if defined (DVD_TRANSPORT_PTI) || defined (DVD_TRANSPORT_LINK)


typedef void (*UpdatedPCR)( BOOL DiscontinuousPCR,
                            U32     PCRBaseBit32,
                            U32     PCRBaseValue,
                            U32     PCRExtension,
                            U32     STCBaseBit32,
                            U32     STCBaseValue,
                            U32     STCExtension );
#else

typedef void (*UpdatedPCR)(STEVT_CallReason_t EventReason,
                           STEVT_EventConstant_t EventOccured,
                           STPTI_EventData_t *PcrData_p);

#endif


/* Private Constants ------------------------------------------------------ */

const U8  HarnessRev[] = "4.0.1";

#ifdef DVD_TRANSPORT_STPTI
#define CLKRV_SLOT_1  1
#endif

/* STACK SIZE FOR TASKS */
#define STACK_SIZE 1024

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE          (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE            100000

/* Memory partitions */
#define TEST_PARTITION_1        &the_system_partition
#define TEST_PARTITION_2        &the_internal_partition

/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      ( internal_block, "internal_section")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#pragma ST_section      ( system_block,   "system_section")

static U8               system_noinit_block [1];
static partition_t      the_system_noinit_partition;
partition_t             *system_noinit_partition = &the_system_noinit_partition;
#pragma ST_section      ( system_noinit_block, "system_section_noinit")

static U8               internal_noinit_block [1];
static partition_t      the_internal_noinit_partition;
partition_t             *noinit_partition = &the_internal_noinit_partition;
#pragma ST_section      ( internal_noinit_block, "internal_section_noinit")

#ifndef STCLKRVT_VCXO_CHANNEL
#if     defined(mb275)
#define STCLKRVT_VCXO_CHANNEL     STPWM_CHANNEL_1
#else
#define STCLKRVT_VCXO_CHANNEL     STPWM_CHANNEL_0
#endif
#endif


#define SMALL_PCR_NUDGE  5              /* small nudge value in Hz */
#define LARGE_PCR_NUDGE  500             /* large nudge value in Hz */
#define CLK_FREQUENCY    27000000       /* 27MHz */
#define CONV90K_TO_27M   300            /* 90kHz to 27MHz conversion */


extern ST_ErrorCode_t STPWM_Init( const ST_DeviceName_t    Name,
                           const STPWM_InitParams_t *InitParams );


/* Function prototypes ---------------------------------------------------- */

void pti_get_packet_arrival_time( U32* ArrivalTime ); /* PTI stub */
void pti_get_extended_packet_arrival_time( U32* ArrivalTimeBaseBit32, /* PTI stub */
                                           U32* ArrivalTimeBaseValue,
                                           U32* ArrivalTimeExtension );
BOOL pti_set_extended_pcr_register_fn( UpdatedPCR ActionPCR );

/* Hidden STCLKRV function */
extern ST_ErrorCode_t STCLKRV_PwmGetRatio( STCLKRV_Handle_t Handle,  
                                           U32 *pMarkValue );


/* Private Function prototypes -------------------------------------------- */
#ifdef DVD_TRANSPORT_STPTI
static void STPTIIntActUse( void );
#else
static void IntActUse( void );
#endif

static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr );

static void STCLKRVI_EventCallback( STEVT_CallReason_t Reason,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData );


/* Private Variables ------------------------------------------------------ */

ST_ErrorCode_t          retVal;
#pragma ST_section      ( retVal,   "ncache_section")

ST_DeviceName_t         PIODevNam;
STPIO_InitParams_t      PIOInitPars_s;
STPIO_TermParams_t      PIOTermPars_s;

ST_DeviceName_t         PWMDevNam;
ST_DeviceName_t         EVTDevNamPcr;
STPWM_InitParams_t      PWMInitPars_s;
STPWM_TermParams_t      PWMTermPars_s;

ST_DeviceName_t         EVTDevNam;
STEVT_InitParams_t      EVTInitPars_s;
STEVT_OpenParams_t      EVTOpenPars_s;
STEVT_TermParams_t      EVTTermPars_s;
STEVT_Handle_t          EVTHandle;


static STEVT_SubscribeParams_t EVTSubscrParams = { STCLKRVI_EventCallback,
                                            NULL,
                                            NULL };
static BOOL                  NewEventRxd;
static STEVT_EventConstant_t EventRxd;

ST_DeviceName_t         DeviceNam;
STCLKRV_InitParams_t    InitParams_s;
STCLKRV_OpenParams_t    OpenParams_s;
STCLKRV_TermParams_t    TermParams_s;
STCLKRV_Handle_t        Handle;
STEVT_EventID_t         CLKRVEventID;

UpdatedPCR              ActionPCR; /* pointer to PTI event function */

#ifdef DVD_TRANSPORT_STPTI
static STEVT_Handle_t          EVTHandlePcr;
#endif





/* Private Macros --------------------------------------------------------- */

/* Functions -------------------------------------------------------------- */

/****************************************************************************
Name         : main()

Description  : Calls the specific test functions

Parameters   : void

Return Value : int

See Also     : None
 ****************************************************************************/

int main( void )
{
    STTBX_InitParams_t      sttbx_InitPars;
    STBOOT_InitParams_t     stboot_InitPars;
    STBOOT_TermParams_t     stboot_TermPars;
    STBOOT_DCache_Area_t    Cache[] =
    {
#ifndef DISABLE_DCACHE
  #if defined (mb314) || defined(mb361) || defined(mb382)
        { (U32 *) CACHEABLE_BASE_ADDRESS, (U32 *) CACHEABLE_STOP_ADDRESS },
  #else
        /* these probably aren't correct,
          but were used already and appear to work */
        { (U32 *) 0x40080000, (U32 *) 0x4FFFFFFF },
  #endif
#endif
        { NULL, NULL }
    };

#ifdef CODETEST
    /* Initialise CodeTest */
    codetest_init();
#endif

    partition_init_heap (&the_internal_partition, internal_block,
                      sizeof(internal_block));
    partition_init_heap (&the_system_partition,   system_block,
                      sizeof(system_block));
    partition_init_heap (&the_system_noinit_partition,   system_noinit_block,
                    sizeof(system_noinit_block));
    partition_init_heap (&the_internal_noinit_partition, internal_noinit_block,
                    sizeof(internal_noinit_block));       

       
#ifndef USE_ORIGINAL  /* Modified for compatiblity to new bom 27-11-00 */

#ifdef DISABLE_ICACHE
    stboot_InitPars.ICacheEnabled             = FALSE;
#else
    stboot_InitPars.ICacheEnabled             = TRUE;
#endif
    stboot_InitPars.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitPars.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitPars.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
    stboot_InitPars.DCacheMap                 = Cache;
    stboot_InitPars.MemorySize                = SDRAM_SIZE;

    retVal = STBOOT_Init( "CLKRVTest", &stboot_InitPars );
    if( retVal != ST_NO_ERROR )
    {
        printf( "ERROR: STBOOT_Init() returned %d\n", retVal );
    }
    else
    { 
       /* Initialize the toolbox */
        sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
        sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;
        sttbx_InitPars.CPUPartition_p = TEST_PARTITION_1;  /* SYSTEM PARTITION  */
         retVal = STTBX_Init( "tbx", &sttbx_InitPars );
        if( retVal != ST_NO_ERROR )
        {
            printf( "ERROR: STTBX_Init() returned %d\n", retVal );
        }
        else
        {
#if defined (DVD_TRANSPORT_PTI) || defined (DVD_TRANSPORT_LINK)
            IntActUse();
#else
            STPTIIntActUse();
#endif
            retVal = STBOOT_Term( "CLKRVTest", &stboot_TermPars );

        }
    }
#else

 
    /* Initialize the toolbox */
       sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
       sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
       sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

       retVal = STTBX_Init( "tbx", &sttbx_InitPars );
       if( retVal != ST_NO_ERROR )
       {
           printf( "ERROR: STTBX_Init() returned %d\n", retVal );
       }
       else
       {
           /* Initialize stboot */
           stboot_InitPars.ICacheEnabled             = TRUE;
           stboot_InitPars.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
           stboot_InitPars.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
           stboot_InitPars.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
           stboot_InitPars.CacheBaseAddress          = (U32*) CACHE_BASE_ADDRESS;
           stboot_InitPars.DCacheMap                 = Cache;
           stboot_InitPars.MemorySize                = SDRAM_SIZE;
           
           retVal = STBOOT_Init( "CLKRVTest", &stboot_InitPars );
           if( retVal != ST_NO_ERROR )
           {
               STTBX_Print(( "ERROR: STBOOT_Init() returned %d\n", retVal ));
           }
           else
           {
               IntActUse();
               
               
               retVal = STBOOT_Term( "CLKRVTest", &stboot_TermPars );
           }
       }
    
#endif /* use */
          

    return retVal;        
}

#ifdef DVD_TRANSPORT_STPTI

/****************************************************************************
Name         : STPTIIntActUse()

Description  : InterActive Use of Clock Recovery to support dynamic changes
               of PCR values from keystrokes.  Effect on STC by PWM changes
               is simulated and displayed.
               Uses STPTI and only tests one instance of clock recovery.         

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_REVISION_NOT_SUPPORTED Version mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STCLKRV_ERROR_PCR_UNAVAILABLE   Valid PCR can't be returned

See Also     : STCLKRV_ErrorCode_t
 ****************************************************************************/

static void STPTIIntActUse( void )
{
    long int RetValue;
    char     KeyBuff;
    U8       KeyCode;
    BOOL     Running    = TRUE;
    BOOL     GlitchAct  = FALSE;

    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;

    U32      PCRFrequency = CLK_FREQUENCY;
    U32      STCFrequency;
    U32      MarkValue;
    BOOL     JumpInPCR = FALSE;
    U32      BaseB32 = 0;
    U32      PCRBase;
    U32      PCRExtd;
    U32      STCBase;
    U32      STCExtd;
    clock_t  CurrTime, PrevTime, ElapTime;
    clock_t  PCRTicks, STCTicks;
    STPTI_EventData_t PcrData;
    STCLKRV_SourceParams_t PCRSrc;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));
    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing STPTIIntActUse Test Function for STPTI....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    STCTicks = time_now();

    /* Initialize the sub-subordinate PIO Driver
       for Port1 only (PWM access enable)        */
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517) || defined (ST_5528)
    /* for Port2 only (PWM access enable)        */
    strcpy( PIODevNam, "Port2" );
    PIOInitPars_s.BaseAddress = (U32*)PIO_2_BASE_ADDRESS;
    PIOInitPars_s.InterruptNumber = PIO_2_INTERRUPT;
    PIOInitPars_s.InterruptLevel = PIO_2_INTERRUPT_LEVEL;
#else
    /*   for Port1 only (PWM access enable)        */    
    strcpy( PIODevNam, "Port1" );
    PIOInitPars_s.BaseAddress = (U32*)PIO_1_BASE_ADDRESS;
    PIOInitPars_s.InterruptNumber = PIO_1_INTERRUPT;
    PIOInitPars_s.InterruptLevel = PIO_1_INTERRUPT_LEVEL;
#endif
    PIOInitPars_s.DriverPartition = TEST_PARTITION_1;
    STTBX_Print(( "Calling STPIO_Init() ................\n" ));    
    ReturnError = STPIO_Init( PIODevNam, &PIOInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    /* Initialize the sub-ordinate PWM Driver
       for Channel 0 only (VCXO fine control) */

    strcpy( PWMDevNam, "STPWM" );
    PWMInitPars_s.Device = STPWM_C4;
    PWMInitPars_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;
    PWMInitPars_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
    strcpy( PWMInitPars_s.C4.PIOforPWM0.PortName, PIODevNam );
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517) || defined (ST_5528)
    PWMInitPars_s.C4.PIOforPWM0.BitMask = PIO_BIT_7;    
#else    
    PWMInitPars_s.C4.PIOforPWM0.BitMask = PIO_BIT_3;
#endif        
    PWMInitPars_s.C4.PIOforPWM1.PortName[0] = '\0';
    PWMInitPars_s.C4.PIOforPWM1.BitMask = 0;
    PWMInitPars_s.C4.PIOforPWM2.PortName[0] = '\0';
    PWMInitPars_s.C4.PIOforPWM2.BitMask = 0;
    STTBX_Print(( "Calling STPWM_Init() for VCXO Channel\n" ));
    ReturnError = STPWM_Init( PWMDevNam, &PWMInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize the sub-ordinate EVT Driver */

    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars_s.EventMaxNum = 3;
    EVTInitPars_s.ConnectMaxNum = 3;
    EVTInitPars_s.SubscrMaxNum = 9;  /*3; modified for new bom */
    EVTInitPars_s.MemoryPartition = TEST_PARTITION_2;
    EVTInitPars_s.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init()\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */

    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars_s, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Subscribe to PCR Events now so that we can get 
       callbacks when events are registered */

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_VALID_EVT) .....\n" ));
    ReturnError = STEVT_Subscribe( EVTHandle, STCLKRV_PCR_VALID_EVT,
                                   &EVTSubscrParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT) ...\n" ));
    ReturnError = STEVT_Subscribe( EVTHandle, STCLKRV_PCR_DISCONTINUITY_EVT, 
                                   &EVTSubscrParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
 
    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_GLITCH_EVT) ....\n" ));
    ReturnError = STEVT_Subscribe( EVTHandle, STCLKRV_PCR_GLITCH_EVT, 
                                   &EVTSubscrParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Setup Event registration for notification of PCR_RECEIVED
       event and callback to ActionPCR */
    strcpy( EVTDevNamPcr, "STEVT2" );
    EVTInitPars_s.EventMaxNum =  3;
    EVTInitPars_s.ConnectMaxNum = 3;
    EVTInitPars_s.SubscrMaxNum = 9;
    EVTInitPars_s.MemoryPartition = TEST_PARTITION_1;
    EVTInitPars_s.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init()\n" ));
    ReturnError = STEVT_Init( EVTDevNamPcr, &EVTInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */

    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNamPcr, &EVTOpenPars_s, &EVTHandlePcr );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
 
    STTBX_Print(("Calling STEVT_Register(PCR_RECIEVED_EVT).....\n"));
    ReturnError = STEVT_Register(EVTHandlePcr, STPTI_EVENT_PCR_RECEIVED_EVT, &CLKRVEventID);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);


    /* Initialize our native Clock Recovery Driver */

    strcpy( DeviceNam, "STCLKRV" );
    strcpy( InitParams_s.PWMDeviceName, PWMDevNam );
    InitParams_s.VCXOChannel = STCLKRVT_VCXO_CHANNEL;
    InitParams_s.InitialMark = STCLKRV_INIT_MARK_VALUE;
    InitParams_s.MinimumMark = STCLKRV_MIN_MARK_VALUE;
    InitParams_s.MaximumMark = STCLKRV_MAX_MARK_VALUE;
    InitParams_s.PCRMaxGlitch = STCLKRV_PCR_MAX_GLITCH;
    InitParams_s.PCRDriftThres = STCLKRV_PCR_DRIFT_THRES;
    InitParams_s.TicksPerMark = STCLKRV_TICKS_PER_MARK;
    InitParams_s.MinSampleThres = STCLKRV_MIN_SAMPLE_THRESHOLD;
    InitParams_s.MaxWindowSize = STCLKRV_MAX_WINDOW_SIZE;            
    InitParams_s.Partition_p = TEST_PARTITION_2;
    InitParams_s.DeviceType = STCLKRV_DEVICE_PWMOUT;
    strcpy( InitParams_s.PCREvtHandlerName, EVTDevNamPcr );
    strcpy( InitParams_s.EVTDeviceName, EVTDevNam );
    STTBX_Print(( "Calling STCLKRV_Init() ..............\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */

    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &OpenParams_s, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Set SlotID value to instance. Required for callback identification
       of relevant instance to action the callback */
    STTBX_Print(( "Calling STCLKRV_SetPCRSource()...\n"));
    PCRSrc.Source_u.STPTI_s.Slot = CLKRV_SLOT_1;
    ReturnError = STCLKRV_SetPCRSource(Handle, &PCRSrc);
    ErrorReport(&StoredError, ReturnError, ST_NO_ERROR);

    PCRTicks = CurrTime = time_now();

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Keys are as follows (case insensitive apart from U & D):\n" ));
    STTBX_Print(( "U - Up PCR frequency         D - Down PCR frequency\n" ));
    STTBX_Print(( "E - Expected PCR jump        J - Jump in PCR (unexpected)\n" ));
    STTBX_Print(( "G - Glitch in PCR clock      Q - Quit InterActive Test\n" ));
    STTBX_Print(( "==========================================================\n" ));

    while( Running )
    {
        ReturnError = STCLKRV_PwmGetRatio( Handle, &MarkValue );
        if (ReturnError != ST_NO_ERROR)
        {
            STTBX_Print(("PWMGetRatio failure\n"));
            Running = FALSE;
        }
        STCFrequency = CLK_FREQUENCY + ( STCLKRV_TICKS_PER_MARK *
                    ( MarkValue - STCLKRV_INIT_MARK_VALUE ) );

        STTBX_Print(( "PCR = %dHz\tSTC = %dHz\tPWM = %3d\t",
                    PCRFrequency, STCFrequency, MarkValue ));
        
        if( NewEventRxd == TRUE )
        {
            if( EventRxd == STCLKRV_PCR_VALID_EVT )
            {
              STTBX_Print(( "Last event = PCR Valid        \t\r" ));
            }
            else if( EventRxd == STCLKRV_PCR_DISCONTINUITY_EVT )
            {
              STTBX_Print(( "Last event = PCR Discontinuity\t\r" ));
            }
            else if( EventRxd == STCLKRV_PCR_GLITCH_EVT )
            {
              STTBX_Print(( "Last event = PCR Glitch       \t\r" ));
            }
        }
        else
        {
          STTBX_Print(( "                              \t\r" ));
        }

        PrevTime = CurrTime;
        CurrTime = time_now();
        ElapTime = time_minus( CurrTime, PrevTime );
        PCRTicks += ( ( PCRFrequency * (ElapTime/200) ) / (ST_GetClocksPerSecond()/200) );
        STCTicks += ( ( STCFrequency * (ElapTime/200) ) / (ST_GetClocksPerSecond()/200) );

        if( GlitchAct )
        {
            /*PCRBase = PCRExtd = 0;*/
            STCBase = STCExtd = 0;
        }
        else
        {
            STCBase = STCTicks / CONV90K_TO_27M;
            STCExtd = STCTicks - ( STCBase * CONV90K_TO_27M );
        }
        
        PCRBase = PCRTicks / CONV90K_TO_27M;
        PCRExtd = PCRTicks - ( PCRBase * CONV90K_TO_27M );

        
        /* Load up callback data structure and triger event */
        PcrData.u.PCREventData.DiscontinuityFlag = JumpInPCR;
        PcrData.u.PCREventData.PCRBase.LSW = PCRBase;
        PcrData.u.PCREventData.PCRExtension =  PCRExtd;      
        PcrData.u.PCREventData.PCRArrivalTime.LSW = STCBase;
        PcrData.u.PCREventData.PCRArrivalTimeExtension = STCExtd;        
        PcrData.u.PCREventData.PCRArrivalTime.Bit32 = BaseB32;
        PcrData.u.PCREventData.PCRBase.Bit32 = BaseB32;
        PcrData.SlotHandle = CLKRV_SLOT_1;    
        /*STTBX_Print(( "Calling notify function \n"));*/
        STEVT_Notify(EVTHandlePcr, CLKRVEventID, &PcrData);


        JumpInPCR = FALSE;
        GlitchAct = FALSE;

        RetValue = STTBX_InputPollChar(( &KeyBuff ));
        /* STTBX_Print(( "RetValue=%ld, KeyBuff=%ld\n", RetValue, KeyBuff )); */

        if( RetValue != 0L )
        {
            KeyCode  = (U8)KeyBuff;
            /* STTBX_Print(( "KeyCode=%d\n", ((U32) KeyCode) )); */

            switch( KeyCode )
            {
                case 'q':
                case 'Q':
                    STTBX_Print(( "\nTerminating Interactive Test\n" ));
                    Running = FALSE;
                    break;

               case 'e': /* JF added */
               case 'E':
                    JumpInPCR = TRUE;
                    PCRTicks = CurrTime;
                    break;

                case 'j':
                case 'J':
                    PCRTicks = CurrTime;
                    break;

                case 'u':
                    PCRFrequency += SMALL_PCR_NUDGE;
                    break;

                case 'U':
                    PCRFrequency += LARGE_PCR_NUDGE;
                    break;

                case 'd':
                    PCRFrequency -= SMALL_PCR_NUDGE;
                    break;

                case 'D':
                    PCRFrequency -= LARGE_PCR_NUDGE;
                    break;

                case 'g':
                case 'G':
                    GlitchAct = TRUE;
                    /* ***NO BREAK*** */

                default:
                    break;
            }
        }
    }

    /* Close Clock Recovery explicitly */

    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */

    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate PWM Driver */

    PWMTermPars_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STPWM_Term() unforced .......\n" ));
    ReturnError = STPWM_Term( PWMDevNam, &PWMTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the sub-subordinate PIO Driver */

    PIOTermPars_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PIODevNam, &PIOTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */

    EVTTermPars_s.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
    
    EVTTermPars_s.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() for EVTDevNamPcr.\n" ));
    ReturnError = STEVT_Term( EVTDevNamPcr, &EVTTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}


#else

/****************************************************************************
Name         : IntActUse()

Description  : InterActive Use of Clock Recovery to support dynamic changes
               of PCR values from keystrokes.  Effect on STC by PWM changes
               is simulated and displayed.

Parameters   : None

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors detected
               ST_ERROR_ALREADY_INITIALIZED    More than one Init call w/o Term
               ST_ERROR_UNKNOWN_DEVICE         Open call w/o prior Init
               ST_ERROR_BAD_PARAMETER          One or more invalid parameters
               ST_ERROR_INVALID_HANDLE         Device handle not open/mismatch
               ST_ERROR_REVISION_NOT_SUPPORTED Version mismatch
               ST_ERROR_FEATURE_NOT_SUPPORTED  Hardware mismatch
               ST_ERROR_NO_FREE_HANDLES        Channel already in use (open)
               ST_ERROR_OPEN_HANDLE            Unforced Term with open channel
               STCLKRV_ERROR_PCR_UNAVAILABLE   Valid PCR can't be returned

See Also     : STCLKRV_ErrorCode_t
 ****************************************************************************/

static void IntActUse( void )
{
    long int RetValue;
    char     KeyBuff;
    U8       KeyCode;
    BOOL     Running    = TRUE;
    BOOL     GlitchAct  = FALSE;

    ST_ErrorCode_t  StoredError = ST_NO_ERROR;
    ST_ErrorCode_t  ReturnError;

    U32      PCRFrequency = CLK_FREQUENCY;
    U32      STCFrequency;
    U32      MarkValue;
    BOOL     JumpInPCR = FALSE;
    U32      BaseB32 = 0;
    U32      PCRBase;
    U32      PCRExtd;
    U32      STCBase;
    U32      STCExtd;
    clock_t  CurrTime, PrevTime, ElapTime;
    clock_t  PCRTicks, STCTicks;

    STTBX_Print(( "\nHarness Software Revision is %s\n\n", HarnessRev ));
    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Commencing IntActUse Test Function for Legacy PTI....\n" ));
    STTBX_Print(( "==========================================================\n" ));

    STCTicks = time_now();

    /* Initialize the sub-subordinate PIO Driver
       for Port1 only (PWM access enable)        */

    /* Initialize the sub-subordinate PIO Driver
       for Port1 only (PWM access enable)        */
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517)
    /* for Port2 only (PWM access enable)        */    
    strcpy( PIODevNam, "Port2" );
    PIOInitPars_s.BaseAddress = (U32*)PIO_2_BASE_ADDRESS;
    PIOInitPars_s.InterruptNumber = PIO_2_INTERRUPT;
    PIOInitPars_s.InterruptLevel = PIO_2_INTERRUPT_LEVEL;
#else
    /*   for Port1 only (PWM access enable)        */    
    strcpy( PIODevNam, "Port1" );
    PIOInitPars_s.BaseAddress = (U32*)PIO_1_BASE_ADDRESS;
    PIOInitPars_s.InterruptNumber = PIO_1_INTERRUPT;
    PIOInitPars_s.InterruptLevel = PIO_1_INTERRUPT_LEVEL;
#endif
    PIOInitPars_s.DriverPartition = TEST_PARTITION_1;
    STTBX_Print(( "Calling STPIO_Init() ................\n" ));    
    ReturnError = STPIO_Init( PIODevNam, &PIOInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );    

    /* Initialize the sub-ordinate PWM Driver
       for Channel 0 only (VCXO fine control) */

    strcpy( PWMDevNam, "STPWM" );
    PWMInitPars_s.Device = STPWM_C4;
    PWMInitPars_s.C4.BaseAddress = (U32*)PWM_BASE_ADDRESS;
    PWMInitPars_s.C4.PrescaleFactor = STPWM_PRESCALE_MIN;
    strcpy( PWMInitPars_s.C4.PIOforPWM0.PortName, PIODevNam );
#if defined (ST_5514) || defined (ST_5516) || defined (ST_5517)
    PWMInitPars_s.C4.PIOforPWM0.BitMask = PIO_BIT_7;    
#else    
    PWMInitPars_s.C4.PIOforPWM0.BitMask = PIO_BIT_3;
#endif    
    PWMInitPars_s.C4.PIOforPWM1.PortName[0] = '\0';
    PWMInitPars_s.C4.PIOforPWM1.BitMask = 0;
    PWMInitPars_s.C4.PIOforPWM2.PortName[0] = '\0';
    PWMInitPars_s.C4.PIOforPWM2.BitMask = 0;
    STTBX_Print(( "Calling STPWM_Init() for VCXO Channel\n" ));
    ReturnError = STPWM_Init( PWMDevNam, &PWMInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize the sub-ordinate EVT Driver */

    strcpy( EVTDevNam, "STEVT" );
    EVTInitPars_s.EventMaxNum = 3;
    EVTInitPars_s.ConnectMaxNum = 2;
    EVTInitPars_s.SubscrMaxNum = 9;  /*3; modified for new bom */
    EVTInitPars_s.MemoryPartition = TEST_PARTITION_2;
    EVTInitPars_s.MemorySizeFlag = STEVT_UNKNOWN_SIZE;
    STTBX_Print(( "Calling STEVT_Init()\n" ));
    ReturnError = STEVT_Init( EVTDevNam, &EVTInitPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Call STEVT_Open() so that we can subscribe to events */

    STTBX_Print(( "Calling STEVT_Open() ................\n" ));
    ReturnError = STEVT_Open( EVTDevNam, &EVTOpenPars_s, &EVTHandle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Subscribe to PCR Events now so that we can get 
       callbacks when events are registered */

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_VALID_EVT) .....\n" ));
    ReturnError = STEVT_Subscribe( EVTHandle, STCLKRV_PCR_VALID_EVT,
                                   &EVTSubscrParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_DISCONTINUITY_EVT) ...\n" ));
    ReturnError = STEVT_Subscribe( EVTHandle, STCLKRV_PCR_DISCONTINUITY_EVT, 
                                   &EVTSubscrParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
 
    STTBX_Print(( "Calling STEVT_Subscribe(STCLKRV_PCR_GLITCH_EVT) ....\n" ));
    ReturnError = STEVT_Subscribe( EVTHandle, STCLKRV_PCR_GLITCH_EVT, 
                                   &EVTSubscrParams );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Initialize our native Clock Recovery Driver */

    strcpy( DeviceNam, "STCLKRV" );
    strcpy( InitParams_s.PWMDeviceName, PWMDevNam );
    InitParams_s.VCXOChannel = STCLKRVT_VCXO_CHANNEL;
    InitParams_s.InitialMark = STCLKRV_INIT_MARK_VALUE;
    InitParams_s.MinimumMark = STCLKRV_MIN_MARK_VALUE;
    InitParams_s.MaximumMark = STCLKRV_MAX_MARK_VALUE;
    InitParams_s.PCRMaxGlitch = STCLKRV_PCR_MAX_GLITCH;
    InitParams_s.PCRDriftThres = STCLKRV_PCR_DRIFT_THRES;
    InitParams_s.TicksPerMark = STCLKRV_TICKS_PER_MARK;
    InitParams_s.MinSampleThres = STCLKRV_MIN_SAMPLE_THRESHOLD;
    InitParams_s.MaxWindowSize = STCLKRV_MAX_WINDOW_SIZE;            
    InitParams_s.Partition_p = TEST_PARTITION_2;
    InitParams_s.DeviceType = STCLKRV_DEVICE_PWMOUT;
    strcpy( InitParams_s.EVTDeviceName, EVTDevNam );
    STTBX_Print(( "Calling STCLKRV_Init() ..............\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &InitParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Open the Clock Recovery Driver */

    STTBX_Print(( "Calling STCLKRV_Open() ..............\n" ));
    ReturnError = STCLKRV_Open( DeviceNam, &OpenParams_s, &Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    PCRTicks = CurrTime = time_now();

    STTBX_Print(( "==========================================================\n" ));
    STTBX_Print(( "Keys are as follows (case insensitive apart from U & D):\n" ));
    STTBX_Print(( "U - Up PCR frequency         D - Down PCR frequency\n" ));
    STTBX_Print(( "E - Expected PCR jump        J - Jump in PCR (unexpected)\n" ));
    STTBX_Print(( "G - Glitch in PCR clock      Q - Quit InterActive Test\n" ));
    STTBX_Print(( "==========================================================\n" ));

    while( Running )
    {
        ReturnError = STCLKRV_PwmGetRatio( Handle, &MarkValue );
        STCFrequency = CLK_FREQUENCY + ( STCLKRV_TICKS_PER_MARK *
                    ( MarkValue - STCLKRV_INIT_MARK_VALUE ) );

        STTBX_Print(( "PCR = %dHz\tSTC = %dHz\tPWM = %3d\t",
                    PCRFrequency, STCFrequency, MarkValue ));
        
        if( NewEventRxd == TRUE )
        {
            if( EventRxd == STCLKRV_PCR_VALID_EVT )
            {
              STTBX_Print(( "Last event = PCR Valid        \t\r" ));
            }
            else if( EventRxd == STCLKRV_PCR_DISCONTINUITY_EVT )
            {
              STTBX_Print(( "Last event = PCR Discontinuity\t\r" ));
            }
            else if( EventRxd == STCLKRV_PCR_GLITCH_EVT )
            {
              STTBX_Print(( "Last event = PCR Glitch       \t\r" ));
            }
        }
        else
        {
          STTBX_Print(( "                              \t\r" ));
        }

        PrevTime = CurrTime;
        CurrTime = time_now();
        ElapTime = time_minus( CurrTime, PrevTime );

        PCRTicks += ( (PCRFrequency * (ElapTime/200)) / (ST_GetClocksPerSecond()/200) );
        STCTicks += ( ( STCFrequency * (ElapTime/200) ) / (ST_GetClocksPerSecond()/200) );

        /* Glitch is due to late arrival of PCR */
        if( GlitchAct )
        {
            /*PCRBase = PCRExtd = 0;*/
            STCBase = STCExtd = 0;
        }
        else
        {
            STCBase = STCTicks / CONV90K_TO_27M;
            STCExtd = STCTicks - ( STCBase * CONV90K_TO_27M );

        }

        PCRBase = PCRTicks / CONV90K_TO_27M;
        PCRExtd = PCRTicks - ( PCRBase * CONV90K_TO_27M );
        (*ActionPCR)( JumpInPCR, BaseB32, PCRBase, PCRExtd,
                                 BaseB32, STCBase, STCExtd );

        JumpInPCR = FALSE;
        GlitchAct = FALSE;

        RetValue = STTBX_InputPollChar(( &KeyBuff ));
        /* STTBX_Print(( "RetValue=%ld, KeyBuff=%ld\n", RetValue, KeyBuff )); */

        if( RetValue != 0L )
        {
            KeyCode  = (U8)KeyBuff;
            /* STTBX_Print(( "KeyCode=%d\n", ((U32) KeyCode) )); */

            switch( KeyCode )
            {
                case 'q':
                case 'Q':
                    STTBX_Print(( "\nTerminating Interactive Test\n" ));
                    Running = FALSE;
                    break;

               case 'e': /* JF added */
               case 'E':
                    JumpInPCR = TRUE;
                    PCRTicks = CurrTime;
                    break;

                case 'j':
                case 'J':
                    PCRTicks = CurrTime;
                    break;

                case 'u':
                    PCRFrequency += SMALL_PCR_NUDGE;
                    break;

                case 'U':
                    PCRFrequency += LARGE_PCR_NUDGE;
                    break;

                case 'd':
                    PCRFrequency -= SMALL_PCR_NUDGE;
                    break;

                case 'D':
                    PCRFrequency -= LARGE_PCR_NUDGE;
                    break;

                case 'g':
                case 'G':
                    GlitchAct = TRUE;
                    /* ***NO BREAK*** */

                default:
                    break;
            }
        }
    }

    /* Close Clock Recovery explicitly */

    STTBX_Print(( "Calling STCLKRV_Close() .............\n" ));
    ReturnError = STCLKRV_Close( Handle );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Unforced Clock Recovery Termination */

    TermParams_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STCLKRV_Term() unforced .....\n" ));
    ReturnError = STCLKRV_Term( DeviceNam, &TermParams_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate PWM Driver */

    PWMTermPars_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STPWM_Term() unforced .......\n" ));
    ReturnError = STPWM_Term( PWMDevNam, &PWMTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the sub-subordinate PIO Driver */

    PIOTermPars_s.ForceTerminate = FALSE;
    STTBX_Print(( "Calling STPIO_Term() ................\n" ));
    ReturnError = STPIO_Term( PIODevNam, &PIOTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    /* Terminate the subordinate EVT Driver */

    EVTTermPars_s.ForceTerminate = TRUE;
    STTBX_Print(( "Calling STEVT_Term() ................\n" ));
    ReturnError = STEVT_Term( EVTDevNam, &EVTTermPars_s );
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );

    STTBX_Print(( "\nOverall test result is " ));
    ReturnError = StoredError;
    ErrorReport( &StoredError, ReturnError, ST_NO_ERROR );
}


/****************************************************************************
Name         : pti_get_packet_arrival_time()

Description  : Stub routine to resolve PTI library reference

Parameters   : *U32 pointer to variable in which value is returned

Return Value : void

See Also     : PTI
 ****************************************************************************/

void pti_get_packet_arrival_time( U32* ArrivalTime )
{
    *ArrivalTime = 0;               /* really meaningful return value */

    return;
}

/****************************************************************************
Name         : pti_get_extended_packet_arrival_time()

Description  : Stub routine to resolve PTI library reference

Parameters   : *U32 pointer to variable in which value is returned

Return Value : void

See Also     : PTI
 ****************************************************************************/

void pti_get_extended_packet_arrival_time( U32* ArrivalTimeBaseBit32,
                                           U32* ArrivalTimeBaseValue,
                                           U32* ArrivalTimeExtension )
{
    *ArrivalTimeBaseBit32 = 0;
    *ArrivalTimeBaseValue = 0;
    *ArrivalTimeExtension = 0;

    return;
}


/****************************************************************************
Name         : pti_set_extended_pcr_register_fn()

Description  : Stub routine to resolve PTI library reference, and to note
               the address of STCLKRV_ActionPCR() function pointer passed.

Parameters   : UpdatedPCR function pointer

Return Value : FALSE (no error)

See Also     : PTI
 ****************************************************************************/

BOOL pti_set_extended_pcr_register_fn( UpdatedPCR CallBack )
{
    ActionPCR = CallBack;

    return FALSE;
}


#endif /* ndef lagacy pti */
/****************************************************************************
Name         : ErrorReport()

Description  : Outputs the appropriate error message string.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : STLLI_ErrorCode_t
 ****************************************************************************/

static void ErrorReport( ST_ErrorCode_t *ErrorStore,
                         ST_ErrorCode_t ErrorGiven,
                         ST_ErrorCode_t ExpectedErr )
{
    switch( ErrorGiven )
    {
        case ST_NO_ERROR:
            STTBX_Print(( "ST_NO_ERROR - Successful Return\n" ));
            break;

        case ST_ERROR_ALREADY_INITIALIZED:
            STTBX_Print(( "ST_ERROR_ALREADY_INITIALIZED - Prior Init call w/o Term\n" ));
            break;

        case ST_ERROR_UNKNOWN_DEVICE:
            STTBX_Print(( "ST_ERROR_UNKNOWN_DEVICE - Init call must precede access\n" ));
            break;

        case ST_ERROR_INVALID_HANDLE:
            STTBX_Print(( "ST_ERROR_INVALID_HANDLE - Handle number not recognized\n" ));
            break;

        case ST_ERROR_OPEN_HANDLE:
            STTBX_Print(( "ST_ERROR_OPEN_HANDLE - unforced Term with handle open\n" ));
            break;

        case ST_ERROR_BAD_PARAMETER:
            STTBX_Print(( "ST_ERROR_BAD_PARAMETER - Parameter(s) out of valid range\n" ));
            break;

        case ST_ERROR_FEATURE_NOT_SUPPORTED:
            STTBX_Print(( "ST_ERROR_FEATURE_NOT_SUPPORTED - Device mismatch\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            STTBX_Print(( "ST_ERROR_NO_FREE_HANDLES - Channel already Open\n" ));
            break;

        case ST_ERROR_NO_MEMORY:
            STTBX_Print(( "ST_ERROR_NO_MEMORY - Not enough space for dynamic memory allocation\n" ));
            break;

        case STCLKRV_ERROR_HANDLER_INSTALL:
            STTBX_Print(( "STCLKRV_ERROR_HANDLER_INSTALL - Action_PCR not (de)installed\n" ));
            break;

        case STCLKRV_ERROR_PCR_UNAVAILABLE:
            STTBX_Print(( "STCLKRV_ERROR_PCR_UNAVAILABLE - PCR can't be returned\n" ));
            break;

        case STCLKRV_ERROR_PWM:
            STTBX_Print(( "STCLKRV_ERROR_PWM - Error occured in sub component PWM\n" ));
            break;
            
        case STCLKRV_ERROR_EVT_REGISTER:
            STTBX_Print(( "STCLKRV_ERROR_EVT_REGISTER - Error occured in sub component EVT\n" ));
            break;

        default:
            STTBX_Print(( "*** Unrecognised return code ***\n" ));
    }

    STTBX_Print(( "----------------------------------------------------------\n" ));

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

/****************************************************************************
Name         : STCLKRVI_EventCallback()

Description  : Callback function for STCLKRV events

Parameters   : STEVT_CallReason_t Reason      Reason for callback
               STEVT_EventConstant Event      Event ID
               const void *EventData          Event associated data

Return Value : None
 ****************************************************************************/
static void STCLKRVI_EventCallback( STEVT_CallReason_t Reason,
                                    STEVT_EventConstant_t Event,
                                    const void *EventData )
{
    NewEventRxd = TRUE;
    EventRxd = Event;
}


/* End of stclkrvt.c module */


