/* Includes --------------------------------------------------------------- */

#include <ostime.h>
#include <stdlib.h>        /* GNBvd06085  */
#include <stdio.h>        /* Needed for print if STTBX fails to init */
#include <string.h>

#include "stboot.h"
#include "stclkrv.h"
#include "stcommon.h"
#include "stddefs.h"
#include "stdevice.h"
#include "stlite.h"
#include "stpio.h"
#include "stpwm.h"
#include "sttbx.h"

#include "stsys.h"
#include "clkrvreg.h"


#ifdef CLKRV_SIM
#include "clkrvsim.h"
#endif


#define STCLKRV_HANDLE_NONE           0
#define STCLKRV_HANDLE_OPEN           1
#define CONV90K_TO_27M                300      /* 90kHz to 27MHz conversion */
#define PCR_STALE_SHIFT               24       /* stale shift limit on PCR  */
#define STCLKRV_PCR_VALID_ID          0        /* Offsets for events array  */
#define STCLKRV_PCR_INVALID_ID        1
#define STCLKRV_PCR_GLITCH_ID         2
#define STCLKRV_STATE_PCR_UNKNOWN     0        /* Internal state machine    */
#define STCLKRV_STATE_PCR_OK          1
#define STCLKRV_STATE_PCR_INVALID     2
#define STCLKRV_STATE_PCR_GLITCH      3
#define STCLKRV_DEVICE_TYPE_MIN       STCLKRV_DEVICE_PWMOUT
#define STCLKRV_DEVICE_TYPE_MAX       STCLKRV_DEVICE_FREQSYNTH
#define STC_REBASELINE_TIME           10000000u /* 10secs in usecs units */

#define ENCODER_FREQUENCY             27000000
#define PCR_NO_UPDATE                 0
#define PCR_UPDATE_OK                 1
#define PCR_UPDATE_THIS_LAST_GLITCH   3
#define PCRTIMEFACTOR                 40

/* 5100 */
#define SW_TASK_STACK_SIZE            2048 
#define SD_CLOCK_INDEX                0
#define DEFAULT_SD_FREQUENCY          27000000
#define MAX_NO_IN_FREQ_TABLE          10  

#define STCLKRV_5100_INTERRUPT      PIO_2_INTERRUPT
#define STCLKRV_5100_INTLEVEL       PIO_2_INTERRUPT_LEVEL

/* Driver revision number */
/*static const ST_Revision_t stclkrv_DriverRev = "STCLKRV-REL_5.0.0";*/

typedef enum INIT_Errors_s
{
    NO_ERROR,
    STTST_START_FAILED,
    STCLKRVSIM_INIT_FAILED,
    STEVT_INIT_FAILED,
    STTST_REGISTER_FAILED,
    STTST_INIT_FAILED,
    STTBX_INIT_FAILED,
    STBOOT_FAILED

}INIT_Errors_t;

/* Private Constants -------------------------------------------------- */

/* DVD Transport STPTI should always be defined for Simulator*/
#ifndef DVD_TRANSPORT_STPTI
#define DVD_TRANSPORT_STPTI
#endif

#ifndef CLKRV_SIM
#define CLKRV_SIM
#endif

/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE     (ST20_INTERNAL_MEMORY_SIZE-1200)
#define SYSTEM_PARTITION_SIZE       800000

/* Memory partitions */
#define TEST_PARTITION_1            &the_system_partition
#define TEST_PARTITION_2            &the_internal_partition

#define ENCODER_FREQ                27000000
#define PCM_CLOCK_FREQ              22579200
#define SPDIF_CLOCK_FREQ            74250000

#define EVT_DEVNAME                 "STEVT"
#define EVT_DEVNAME_PCR             "STEVT2"
#define CLKRV_DEVNAME               "STCLKRV"

/* Private Types ---------------------------------------------------------- */

/* Declarations for memory partitions */
static U8               internal_block [INTERNAL_PARTITION_SIZE];
static partition_t      the_internal_partition;
partition_t             *internal_partition = &the_internal_partition;
#pragma ST_section      (internal_block, "internal_section")

static U8               system_block [SYSTEM_PARTITION_SIZE];
static partition_t      the_system_partition;
partition_t             *system_partition = &the_system_partition;
#pragma ST_section      ( system_block,   "system_section")

/* ensures that system_section_noinit and internal_section_noinit
   (which are placed in mb314 config file) are actually created.
   Removes linker warnings.
*/
static U8               system_noinit_block [1];
static partition_t      the_system_noinit_partition;
partition_t             *system_noinit_partition = &the_system_noinit_partition;
#pragma ST_section      ( system_noinit_block, "system_section_noinit")

static U8               internal_noinit_block [1];
static partition_t      the_internal_noinit_partition;
partition_t             *noinit_partition = &the_internal_noinit_partition;
#pragma ST_section      (internal_noinit_block, "internal_section_noinit")

static ST_ErrorCode_t   RetVal;
#pragma ST_section      (RetVal,   "ncache_section")    /* why in non chache section */

/* CLKRV Related */
static STCLKRV_InitParams_t InitParams_s;
static STCLKRV_OpenParams_t OpenParams_s;
static STCLKRV_TermParams_t TermParams_s;
static ST_DeviceName_t      DeviceNam;
static STCLKRV_Handle_t     Handle;

typedef struct
{
    STCLKRV_ExtendedSTC_t       STC;                 /* Given data */
    STCLKRV_STCSource_t         STCReferenceControl; /* Control switch */
} stclkrv_STCBaseline_t;


typedef struct
{
    BOOL Reset;
    S32  PrevDiff;
    U32  PrevPcr;
    S32  OutStandingErr;
    S32  ControlValue;
    S32  *ErrorStore_p;
    S32  ErrorStoreCount;
    U32  Head;
    S32  ErrorSum;
    U32  MaxFrequencyError;
    S32  ResidualAE; /* Residual frequency error */
} stclkrv_Context_t;


typedef struct
{
    BOOL AwaitingPCR;
    BOOL Valid;
    BOOL PCRHandlingActive;
    U32  PcrBaseBit32;
    U32  PcrBaseValue;
    U32  PcrExtension;
    U32  PcrArrivalBaseBit32;
    U32  PcrArrivalBaseValue;
    U32  PcrArrivalExtension;
    U32  TotalValue;      /* PCRBase * CONV90K_TO_27M + PCRExtension (27MHz) */
    U32  Time;
    U32  GlitchCount;
    U32  MachineState;
} stclkrv_ReferencePCR_t;

typedef struct
{
    clock_t     Timer;                   /* Microsecond timer count */
    BOOL        Active;                  /* Control */
} stclkrv_FreerunSTC_t;

U32 DebugCounter = 0;
U32 CounterPCM   = 0;
U32 CounterSPDIF = 0;

#if defined (ST_5100)
#pragma ST_device (DU32)
typedef volatile U32 DU32;
#endif

/* Nominal Freq Look Up Table Structure */
typedef struct stclkrv_Freqtable_s
{
    U32         Frequency;
    S8          SDIV;
    S8          MD;
    U16         IPE;
    U16         Step;

} stclkrv_Freqtable_t;

/* 5100 Clock Context */
typedef struct stclkrv_FS_s
{
    U32         Frequency;
    S8          SDIV;
    S8          MD;
    U16         IPE;
    U32         Step;
    BOOL        Valid;      /* flag for valid frequency */
    U32         PrevCounter;
/*  U32         CurrentCounter;*/
    U32         Ratio;      /* Valid only for PCM and SPDIF/HD clocks */
} stclkrv_FS_t;

/* 5100 Hardware Context */
typedef struct stclkrv_HW_context_s
{
    U32             FS0_Reference;     /* FS0 -- SD Video Frequency count down counter */
    stclkrv_FS_t    Clocks[NO_OF_CLOCKS];
    
} stclkrv_HW_context_t;    

/* Multi-instance link list control block */
struct STCLKRV_ControlBlock_s
{
    struct STCLKRV_ControlBlock_s *Next;    /* Next  in link list */
    ST_DeviceName_t             DeviceName; 
    U32                         OpenCount;  /* OpenCount per instance */
    STCLKRV_InitParams_t        InitPars;   /* Copy of Initialisation params */
    stclkrv_ReferencePCR_t      ActvRefPcr; /* Active pcr reference data */
    stclkrv_Context_t           ActvContext;/* Active context data */
    
#if defined(ST_5100)
    stclkrv_HW_context_t        HWContext;      /* CLKRV HW module data */
    U8                          TaskStack[SW_TASK_STACK_SIZE];
    semaphore_t                 DriftCorrectionSem;
    task_t                      task;
    tdesc_t                     tdesc;
    BOOL                        TaskCreated;
    BOOL                        WaitingToQuit;
#else /* for VCXO based devices */
    STPWM_Handle_t              PwmHandle;	    /* Instance PWM Handle */
    STPWM_OpenParams_t          PwmOpenPars;	/* Instance PWM Open params */
#endif    
    
    STEVT_Handle_t              ClkrvEvtHandle; /* Instance Event handle */
    STEVT_EventID_t             RegisteredEvents[STCLKRV_NB_REGISTERED_EVENTS];
    semaphore_t                 InstanceSemaphore;      /* Provides mutex on private data */
    stclkrv_STCBaseline_t       STCBaseline;    /* STC baseline value and control switch */
    stclkrv_FreerunSTC_t        FreerunSTC;     /* Free running STC control */

#ifdef DVD_TRANSPORT_STPTI
    STEVT_Handle_t              PcrEvtHandle;   /* PCR event for callback from pti */
    STPTI_Slot_t                Slot;           /* STPTI slot handle */
    BOOL                        SlotEnabled;    /* STPTI Slot use enabled */
#endif

};

S32                     PCRIntervalTime = 40             ;   /* in ms */
S32                     Max_Window_Size = STCLKRV_MAX_WINDOW_SIZE   ;
S32                     Drift_Threshold = STCLKRV_PCR_DRIFT_THRES   ;
/* Private Variables ------------------------------------------------------*/

#if defined (ST_5100)

/* Nominal Frequency Table */
stclkrv_Freqtable_t NominalFreqTable[MAX_NO_IN_FREQ_TABLE] = 
{
    /* Freq    sdiv md  ipe    Step  */ 
    {27000000, 16, -17, 0,     5150}, 
    {22579200, 16, -13, 28421, 3602},
    {74250000, 4,   -9, 23832, 9737},
};

#endif


/* Provides mutual exclusion on global data */
static semaphore_t AccessSemaphore;

static stclkrv_Context_t InitContext =
{
    TRUE,       /* Reset */
    0,          /* PrevDiff */
    0,          /* PrevPcr*/
    0,          /* Out Standing Error */
    0,          /* ControlValue */
    NULL,       /* ErrorStore_p; */
    0,          /* ErrorStoreCount */
    0,          /* Head */
    0,          /* ErrorSum */
    (PCRTIMEFACTOR * STCLKRV_PCR_DRIFT_THRES), /* Maximum error before a Glitch event is thrown*/
    0
};

static stclkrv_ReferencePCR_t InitRefPcr =
{
    TRUE,                       /* AwaitingPCR         */
    FALSE,                      /* Valid               */
    TRUE,                       /* PCRHandlingActive   */
    0,                          /* PcrBaseBit32        */
    0,                          /* PcrBaseValue        */
    0,                          /* PcrExtension        */
    0,                          /* PcrArrivalBaseBit32 */
    0,                          /* PcrArrvialBaseValue */
    0,                          /* PcrArrivalExtension */
    0,                          /* Total Value         */
    0,                          /* Time                */
    0,                          /* GlitchCount         */
    STCLKRV_STATE_PCR_UNKNOWN   /* MachineState        */
};



/* control block type def'd */
typedef struct STCLKRV_ControlBlock_s STCLKRV_ControlBlock_t;

/* Initialise ptr to control block list */
static STCLKRV_ControlBlock_t *ClkrvList_p = NULL;


/* Private Macros --------------------------------------------------------- */

/* Enter shared memory protection */
#define EnterCriticalSection()    interrupt_lock();
#define LeaveCriticalSection()    interrupt_unlock();
/* 5100 - Clock Change Functions Prototypes */
/* 5100 - Drift Correction Related Functions Prototypes */                                 

static void Clkrv_DriftCorrectionTask( void *Param);
static void Clkrv_DCOInterruptHandler( void *Param );

/*void EnableDriftCorrection(STCLKRV_ControlBlock_t *TmpClkrvList_p);*/
/*ST_ErrorCode_t SetClockFrequency(STCLKRV_Clock_t Clock, STCLKRV_ControlBlock_t *TmpClkrvList_p);*/

static ST_ErrorCode_t Clkrv_Program_FS_Registers(STCLKRV_ControlBlock_t *TmpClkrvList_p, STCLKRV_Clock_t Clock, U8  Sdiv, S8  Md, U16 Ipe);
static ST_ErrorCode_t Clkrv_ChangeClockFreqBy( STCLKRV_ControlBlock_t *TmpClkrvList_p,STCLKRV_Clock_t Clock, S32 Control );
static ST_ErrorCode_t Clkrv_Reset_5100_HW(STCLKRV_ControlBlock_t *TmpClkrvList_p);
static void Clkrv_Delay (U32 Delay_val);
/* 5100 - Register Read Write Function Prototypes */
static void Clkrvreg_WriteReg(STSYS_DU32 *Base, U32 Reg, U32 Value);
static void Clkrvreg_ReadReg (STSYS_DU32 *Base, U32 Reg, U32 *Value);

static void AddToInstanceList(STCLKRV_ControlBlock_t *NewItem_p);
static BOOL DelFromInstanceList(ST_DeviceName_t DeviceName);
static STCLKRV_ControlBlock_t *FindInstanceByHandle(STCLKRV_Handle_t ClkHandle);
static STCLKRV_ControlBlock_t *FindInstanceByName(const ST_DeviceName_t DeviceName);
static void Dump_Registers();


/****************************************************************************
Name         : main()

Description  : 

Parameters   : 
               
****************************************************************************/

int main( void )
{
   STTBX_InitParams_t      sttbx_InitPars;
   STBOOT_InitParams_t     stboot_InitPars;
   ST_ErrorCode_t          ReturnError,TempError ;
   
    BOOL RetErr             = FALSE;

    STBOOT_DCache_Area_t    Cache[] =
    {
#ifndef DISABLE_DCACHE
        #if defined (mb314) || defined(mb361) || defined(mb382) || defined(mb390)
        
            {(U32 *) CACHEABLE_BASE_ADDRESS, (U32 *) CACHEABLE_STOP_ADDRESS},
        #else
         /* these probably aren't correct, but were used already and appear to work */
            {(U32 *) 0x40080000, (U32 *) 0x4FFFFFFF},
        #endif
#endif
        {NULL, NULL}
    };


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
    stboot_InitPars.DCacheMap                 = Cache;
    stboot_InitPars.MemorySize                = SDRAM_SIZE;

    RetVal = STBOOT_Init( "CLKRVSIM", &stboot_InitPars );
    if( RetVal != ST_NO_ERROR )
    {
        printf("ERROR: STBOOT_Init() returned %d\n", RetVal );
        return RetVal;
    }

    /* Initialize All Partitions */
    partition_init_heap (&the_internal_partition, internal_block,sizeof(internal_block));
    partition_init_heap (&the_system_partition, system_block,sizeof(system_block));
    partition_init_heap (&the_system_noinit_partition, system_noinit_block,
                                                            sizeof(system_noinit_block));
    partition_init_heap (&the_internal_noinit_partition, internal_noinit_block,
                                                            sizeof(internal_noinit_block));
    /* Initialize the ToolBox */
    sttbx_InitPars.SupportedDevices    = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultOutputDevice = STTBX_DEVICE_DCU;
    sttbx_InitPars.DefaultInputDevice  = STTBX_DEVICE_DCU;

    sttbx_InitPars.CPUPartition_p      = TEST_PARTITION_1;

    RetVal = STTBX_Init("TBX", &sttbx_InitPars );
    if( RetVal != ST_NO_ERROR )
    {
        printf( "ERROR: STTBX_Init() returned %d\n", RetVal );
         return RetVal;
    }
   

   /*****************************************/
      /* Clock recovery instance devicename */
    strcpy(DeviceNam, CLKRV_DEVNAME);
    Dump_Registers();
    InitParams_s.DeviceType       = STCLKRV_DEVICE_FREQSYNTH;
    InitParams_s.DCOBaseAddress   = (U32*)(ST5100_CKG_BASE_ADDRESS);
    InitParams_s.InterruptNumber  = ST5100_DCO_CORRECTION_INTERRUPT;
    InitParams_s.InterruptLevel   = 1;

    InitParams_s.PCRMaxGlitch     = STCLKRV_PCR_MAX_GLITCH;
    InitParams_s.PCRDriftThres    = Drift_Threshold;
    InitParams_s.MinSampleThres   = STCLKRV_MIN_SAMPLE_THRESHOLD;
    InitParams_s.MaxWindowSize    = Max_Window_Size;
    InitParams_s.Partition_p      = TEST_PARTITION_2; /* TBD - 1 or 2 */

    strcpy(InitParams_s.PCREvtHandlerName, EVT_DEVNAME_PCR );
    strcpy(InitParams_s.EVTDeviceName, EVT_DEVNAME );
    STTBX_Print(( "Calling STCLKRV_Init()....\n" ));
    ReturnError = STCLKRV_Init( DeviceNam, &InitParams_s);
    if(ReturnError != ST_NO_ERROR)
    {
        return ReturnError;
    }

    /* Open the Clock Recovery Driver */
    
    ReturnError = STCLKRV_Open(DeviceNam, &OpenParams_s, &Handle );
    
    if(ReturnError != ST_NO_ERROR)
    {
        TempError = ReturnError;
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams_s);
       
        return TempError;
    }

    ReturnError = STCLKRV_SetNominalFreq(Handle,STCLKRV_PCM_CLOCK,PCM_CLOCK_FREQ);
   
    if(ReturnError != ST_NO_ERROR)
    {
        TempError = ReturnError;
        ReturnError = STCLKRV_Close(Handle);
       
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams_s);
        return TempError;
    }

    ReturnError = STCLKRV_SetNominalFreq(Handle,STCLKRV_SPDIF_HD_CLOCK,SPDIF_CLOCK_FREQ);
    if(ReturnError != ST_NO_ERROR)
    {
        TempError = ReturnError;
        ReturnError = STCLKRV_Close(Handle);
        ReturnError = STCLKRV_Term(DeviceNam,&TermParams_s);
        return TempError;
    }
    while(1)
    {
        task_delay(1000);
        printf("\nInt Counter = %d, PCM = %d",DebugCounter,CounterPCM);
    }
    
}


/****************************************************************************
Name         : STCLKRV_Init()

Description  : Initializes the Clock Recovery before use.

Parameters   : ST_DeviceType_t non-NUL name needs to be supplied,
               followed by STCLKRV_InitParams_t pointer.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_ALREADY_INITIALIZED    Device already initialized
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid
               ST_ERROR_NO_MEMORY              memory_allocate failed
               ST_ERROR_FEATURE_NOT_SUPPORTED  Legacy PTI error on attempt at MI.
               STCLKRV_ERROR_HANDLER_INSTALL Unable to install internal ISR
               STCLKRV_ERROR_PWM             Error occured in call to STPWM
               STCLKRV_ERROR_EVT_REGISTER    Problem registering events


See Also     : STCLKRV_InitParams_t
               STCLKRV_Term()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Init( const ST_DeviceName_t Name,
                             const STCLKRV_InitParams_t *InitParams )
{

    ST_ErrorCode_t ReturnCode = ST_NO_ERROR;
    U32 i;
    ST_ErrorCode_t RetCode = ST_NO_ERROR;
    STCLKRV_ControlBlock_t *TmpClkrvList_p;
    ST_ErrorCode_t IntReturn = 0;

    /* Perform parameter validity checks */
    if (( Name                == NULL)   ||           /* NULL Name ptr          */
        (InitParams           == NULL)   ||           /* NULL structure ptr     */
        (Name[0]              == '\0' )  ||           /* Device Name undefined  */
        (strlen( Name )       >= ST_MAX_DEVICE_NAME)) /* string too long?       */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Check For Base Address and Device Type */
    if((InitParams->DeviceType != STCLKRV_DEVICE_FREQSYNTH)||(InitParams->DCOBaseAddress == NULL))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* EVT Device Name undefined? */
    if( ( InitParams->EVTDeviceName[0] == '\0' ) ||
        ( strlen( InitParams->EVTDeviceName ) >= ST_MAX_DEVICE_NAME ))
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Memory parition provided ? */
    if (InitParams->Partition_p == NULL)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* Check averaging window parameters */
    if ((InitParams->MaxWindowSize == 0) ||
        (InitParams->MinSampleThres > InitParams->MaxWindowSize))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }

    /* All parameters ok, so start init....
     Create instance list, open and init PWM, register events and callbacks */

    TmpClkrvList_p = (STCLKRV_ControlBlock_t *) memory_allocate(InitParams->Partition_p,
                                                sizeof(STCLKRV_ControlBlock_t));
    if (TmpClkrvList_p == NULL)
    {
         return ST_ERROR_NO_MEMORY;
    }

    /* If no list already created, init global semaphore */
    if (ClkrvList_p == NULL)
    {
        task_lock();    
        semaphore_init_fifo (&AccessSemaphore, 1);
        task_unlock();
    }
    /* Take semap for duration of function */
    
    semaphore_wait(&AccessSemaphore);
    
    /* Semaphore to signal the DRIFT correction TASK from ISR */
  
    /* Create and take instance semap to protect private data write */
    semaphore_init_fifo (&TmpClkrvList_p->InstanceSemaphore, 1);
    semaphore_wait (&TmpClkrvList_p->InstanceSemaphore);

    /* Load init data to instance */

    TmpClkrvList_p->SlotEnabled = FALSE;

    strcpy(TmpClkrvList_p->DeviceName, Name);
    TmpClkrvList_p->OpenCount   = 0;
    TmpClkrvList_p->InitPars    = *InitParams;
    TmpClkrvList_p->ActvContext = InitContext;
    TmpClkrvList_p->ActvContext.MaxFrequencyError = (PCRTIMEFACTOR *
                                                     TmpClkrvList_p->InitPars.PCRDriftThres);
    TmpClkrvList_p->ActvRefPcr  = InitRefPcr;

    /* 5100 specific Initialization */
    if (ReturnCode == ST_NO_ERROR)
    {
        interrupt_lock();
        IntReturn = interrupt_install ( TmpClkrvList_p->InitPars.InterruptNumber,
                                        TmpClkrvList_p->InitPars.InterruptLevel,
                                        Clkrv_DCOInterruptHandler,
                                        TmpClkrvList_p);
        interrupt_unlock();
        if (IntReturn == 0)
        {
            IntReturn=interrupt_enable(TmpClkrvList_p->InitPars.InterruptLevel);
            if(IntReturn != 0)
            {
                interrupt_uninstall(TmpClkrvList_p->InitPars.InterruptNumber,
                                    TmpClkrvList_p->InitPars.InterruptLevel);
                ReturnCode = ST_ERROR_BAD_PARAMETER;
            }
        }
        else
        {   /* Interrupt enable failed */
            ReturnCode = ST_ERROR_INTERRUPT_INSTALL;
        }

        if (IntReturn == 0)
        {
            semaphore_init_fifo (&TmpClkrvList_p->DriftCorrectionSem, 0); 
            TmpClkrvList_p->WaitingToQuit = FALSE;
            TmpClkrvList_p->TaskCreated   = FALSE;
            
            /* drift correction task init */
/*            ReturnCode = task_init(Clkrv_DriftCorrectionTask,
                                   TmpClkrvList_p,
                                   TmpClkrvList_p->TaskStack,
                                   SW_TASK_STACK_SIZE,
                                   &TmpClkrvList_p->task,
                                   &TmpClkrvList_p->tdesc,
                                   MAX_USER_PRIORITY,
                                   "Clkrv_Drift_Task",
                                   0);
*/  
            if (0 == ReturnCode)
            {
                /* Tell Term() that it should try and remove the task */
                TmpClkrvList_p->TaskCreated = TRUE;
                /* setup of STC/SD video frequency to noimnal 27 MHz*/
                RetCode = Clkrv_Reset_5100_HW (TmpClkrvList_p);
                /* SD_Setup(TmpClkrvList_p);*/
            }
            else
            {
                /* Task init failed */
                interrupt_disable(TmpClkrvList_p->InitPars.InterruptLevel);
                interrupt_uninstall(TmpClkrvList_p->InitPars.InterruptNumber,
                                    TmpClkrvList_p->InitPars.InterruptLevel);
                ReturnCode = STCLKRV_ERROR_TASK_FAILURE;
            }
        }        
    }
    
    /* subscription and regsitration ok ? */
    if (ReturnCode == ST_NO_ERROR)
    {
        AddToInstanceList(TmpClkrvList_p);     /* also sets up global ptr */
        semaphore_signal(&(TmpClkrvList_p->InstanceSemaphore));
    }
    else
    {
        /* Init failed, so give up memory */
        semaphore_delete(&TmpClkrvList_p->DriftCorrectionSem);
        semaphore_delete(&TmpClkrvList_p->InstanceSemaphore);
        memory_deallocate(InitParams->Partition_p,TmpClkrvList_p->ActvContext.ErrorStore_p);
        memory_deallocate(InitParams->Partition_p,TmpClkrvList_p);
        semaphore_signal(&AccessSemaphore);
    }
    semaphore_signal(&AccessSemaphore);

    return (ReturnCode);
}



/****************************************************************************
Name         : STCLKRV_Open()

Description  : Opens the Clock Recovery Interface for operation.
               MUST have been preceded by a successful Init call.
               Note that this routine must cater for multi-thread usage.
               The first call actually "opens" the device. Subsequent calls
               just return the same handle as the first call & increment an
               open count.

Parameters   : A STCLKRV_DeviceName_t name (matching name given to Init)
               needs to be supplied, a pointer to STCLKRV_OpenParams_t,
               plus a pointer to STCLKRV_Handle_t for return of the
               access handle.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                   No errors occurred
               ST_ERROR_UNKNOWN_DEVICE       Invalid Name/no prior Init
               ST_ERROR_BAD_PARAMETER        One or more invalid parameters


See Also     : STCLKRV_Handle_t
               STCLKRV_Init()
               STCLKRV_Close()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Open( const ST_DeviceName_t       Name,
                             const STCLKRV_OpenParams_t  *OpenParams,
                             STCLKRV_Handle_t            *Handle )
{

    STCLKRV_ControlBlock_t *TmpClkrvList_p;     /* Temp ptr into instance list */

   /* Perform parameter validity checks */

    if ((Name   == NULL) ||                 /* Name   ptr uninitialised? */
        (Handle == NULL))                   /* Handle ptr uninitialised? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    /* Devuce must already exist in list ie, had prior call to init */
    TmpClkrvList_p = FindInstanceByName(Name);
    if (TmpClkrvList_p == NULL)
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    /* Return ptr to block */
    *Handle =  (STCLKRV_Handle_t )TmpClkrvList_p;

    /* Protect and open status */
    EnterCriticalSection();

    TmpClkrvList_p->OpenCount++;

    LeaveCriticalSection();

    return (ST_NO_ERROR);
}

/****************************************************************************
Name         : STCLKRV_Close()

Description  : Closes the Handle for re-use. Only decrements OpenCount.
               Device closure performed by terminate.

Parameters   : STCLKRV_Handle_t

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         No Open on this Handle

See Also     : STCLKRV_Open()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Close( STCLKRV_Handle_t Handle )
{
    ST_ErrorCode_t RetCode = ST_NO_ERROR;
    STCLKRV_ControlBlock_t *TmpClkrvList_p;

    TmpClkrvList_p = FindInstanceByHandle(Handle);

    if (TmpClkrvList_p  == NULL)
    {
        return( ST_ERROR_INVALID_HANDLE );
    }

    /* Protect, read and  update data */
    EnterCriticalSection();

    if (TmpClkrvList_p->OpenCount != 0)
    {
        TmpClkrvList_p->OpenCount--;
    }
    else
    {
        RetCode = ST_ERROR_INVALID_HANDLE;
    }

    LeaveCriticalSection();

    return (RetCode);
}



/****************************************************************************
Name         : STCLKRV_Term()

Description  : Terminates the named device driver, effectively
               reversing the Initialization call.

               (NB: Using forced terminate may not be completely safe, as it
                is theoretically possible for a task to be waiting on the
                AccessSemaphore, in which case the task gets 'lost'.)

Parameters   : ST_DeviceName_t name matching that supplied with Init,
               STCLKRV_TermParams_t pointer containing ForceTerminate flag.

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_UNKNOWN_DEVICE         Invalid device name
               ST_ERROR_BAD_PARAMETER          One or more parameters invalid
               ST_ERROR_OPEN_HANDLE            Close not called/unsuccessful
               STCLKRV_ERROR_HANDLER_INSTALL   Unable to uninstall internal ISR
               STCLKRV_ERROR_PWM               STPWM_Close() returned an error
               STCLKRV_ERROR_EVT_REGISTER      Problem unregistering events

See Also     : STCLKRV_Init()
               UnregisterEvents()
 ****************************************************************************/

ST_ErrorCode_t STCLKRV_Term( const ST_DeviceName_t Name,
                             const STCLKRV_TermParams_t *TermParams )
{
    STCLKRV_ControlBlock_t *TmpClkrvList_p;
    ST_ErrorCode_t TermRetCode =  ST_NO_ERROR;    /* GNBvd05993 */

    /* Perform parameter validity checks */

    if (( Name       == NULL )   ||             /* NULL Name ptr? */
        ( TermParams == NULL ))                /* NULL structure ptr? */
    {
        return( ST_ERROR_BAD_PARAMETER );
    }

    TmpClkrvList_p = FindInstanceByName(Name);

    if (TmpClkrvList_p == NULL)
    {
        return (ST_ERROR_UNKNOWN_DEVICE);
    }

    if ((!TermParams->ForceTerminate) && (TmpClkrvList_p->OpenCount > 0))
    {
        return (ST_ERROR_OPEN_HANDLE);     /* need to call Close */
    }

    semaphore_wait (&(TmpClkrvList_p->InstanceSemaphore));

     if (TermRetCode == ST_NO_ERROR)
     {
          /* Clean up memory use by instance */
         semaphore_delete (&(TmpClkrvList_p->InstanceSemaphore));
         
         /* Uninstall the interrupt handler */ 
        interrupt_lock();
        interrupt_disable(TmpClkrvList_p->InitPars.InterruptLevel);
        if (interrupt_uninstall( TmpClkrvList_p->InitPars.InterruptNumber,
                                 TmpClkrvList_p->InitPars.InterruptLevel) != 0)
        TermRetCode = ST_ERROR_INTERRUPT_UNINSTALL;
        interrupt_unlock();                 

        /* TBD - Make sure that driftCorrectiontask is done */
        TmpClkrvList_p->WaitingToQuit = TRUE;
        semaphore_signal(&TmpClkrvList_p->DriftCorrectionSem);

        if (TmpClkrvList_p->TaskCreated == TRUE)
        {
            int ret;
            task_t *task_p;
            clock_t timeout = time_plus(time_now(), 
                                        ST_GetClocksPerSecond());

            task_p = &TmpClkrvList_p->task;
            ret = task_wait(&task_p, 1, &timeout); 
            if (ret != -1)
                task_delete(task_p);
            else
                TermRetCode = STCLKRV_ERROR_TASK_FAILURE;
        }
        semaphore_delete (&(TmpClkrvList_p->DriftCorrectionSem));
        semaphore_wait(&AccessSemaphore);
        DelFromInstanceList(TmpClkrvList_p->DeviceName);
        semaphore_signal(&AccessSemaphore);
        
        memory_deallocate(TmpClkrvList_p->InitPars.Partition_p,TmpClkrvList_p);
         
         /* Clean up memory use by list if necessary */
         if (ClkrvList_p == NULL)
         {
             semaphore_delete(&AccessSemaphore);
         }
    }
    else
    {
          semaphore_signal(&(TmpClkrvList_p->InstanceSemaphore));
    }

    return (TermRetCode);
}


/****************************************************************************
Name         : AddToInstanceList

Description  : Adds pre-created intance of clock recovery control block
               to instance linked list.
               Adds to the head of the list.

Parameters   : STCLKRV_ControlBlock_t *newItem_p : pointer to item to add
               (pre-created prior to function call using malloc type command.)

Return Value : None
 ****************************************************************************/
void AddToInstanceList(STCLKRV_ControlBlock_t *NewItem_p)
{


    if (ClkrvList_p == NULL)
    {
        /* If empty list, make newItem last item by NULL */
        NewItem_p->Next = NULL;
    }
    else
    {
        /* Add to start of the list */
        NewItem_p->Next = ClkrvList_p;
    }

    /* Set global ptr new first item */
    ClkrvList_p = NewItem_p;

}

/****************************************************************************
Name         : DelFromInstanceList

Description  : Deletes intance of clock recovery control block from instance
               linked list.

Parameters   : ST_DeviceName_t DeviceName : identifying device to delete from list.
               Does not deallocate the memory used by the deleted control block,
               this operation is required after the call to this function.

Return Value : FALSE is no error, TRUE is error (list is empty, item not found).
 ****************************************************************************/
BOOL DelFromInstanceList(ST_DeviceName_t DeviceName)
{
    STCLKRV_ControlBlock_t *Search_p;
    STCLKRV_ControlBlock_t *Previous_p;

    Search_p = ClkrvList_p;
    Previous_p = ClkrvList_p;

    while (Search_p != NULL)
    {
        /* If device name found...*/
        if (strcmp(Search_p->DeviceName,DeviceName) == 0)
        {
            /* If only item in list..*/
            if (Search_p == ClkrvList_p)
            {
                /* kill global pointer */
                ClkrvList_p = NULL;
            }
            else
            {
                /* link out item to del */
                Previous_p->Next=(STCLKRV_ControlBlock_t *)Search_p->Next;
            }

            return  FALSE;   /* all ok */
        }

        /* Step ptr thru list */
        Previous_p=(STCLKRV_ControlBlock_t *)Search_p;
        Search_p=(STCLKRV_ControlBlock_t *)Search_p->Next;

    }
    return TRUE;   /* Item not found and/or list empty */

}


/****************************************************************************
Name         : FindInstanceByName

Description  : Searches the clock recovery instance linked list for the
               instance with the given name.

Parameters   : const ST_DeviceName_t DeviceName : to identify the instance to
               search for.


Return Value :  STCLKRV_ControlBlock_t *ptr : pointer to instance in the linked
                list, or to NULL if not found.
 ****************************************************************************/
STCLKRV_ControlBlock_t *FindInstanceByName(const ST_DeviceName_t DeviceName)
{

    STCLKRV_ControlBlock_t *Search_p;

    Search_p = ClkrvList_p;

    while (Search_p != NULL)
    {
       if (strcmp(Search_p->DeviceName,DeviceName) == 0)
       {
          return  Search_p;
       }

       Search_p = (STCLKRV_ControlBlock_t *)Search_p->Next;
    }

    return NULL;

}

/****************************************************************************
Name         : FindInstanceByHandle

Description  : Searches the clock recovery instance linked list for the
               instance with the given Handle

Parameters   : Handle


Return Value : STCLKRV_Handle_t clkHandle : Handle to find in Inst list
 ****************************************************************************/
STCLKRV_ControlBlock_t *FindInstanceByHandle( STCLKRV_Handle_t ClkHandle)
{

    STCLKRV_ControlBlock_t *Search_p;

    Search_p = ClkrvList_p;

    while (Search_p != NULL)
    {
        /* Is handle given the same as the address of the instance ie, valid ? */
       if (ClkHandle ==  (STCLKRV_Handle_t) Search_p)
       {
          return  Search_p;
       }

       Search_p = (STCLKRV_ControlBlock_t *)Search_p->Next;

    }

    return NULL;

}

/****************************************************************************
Name         : Clkrv_Reset_5100_HW

Description  : 

Parameters   : 
               
Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle
               STCLKRV_INVALID_FREQUENCY       Invalid Frequency
               STCLKRV_INVALID_SLAVE           Invalid Slave

See Also     : 
****************************************************************************/

static ST_ErrorCode_t Clkrv_Reset_5100_HW(STCLKRV_ControlBlock_t *TmpClkrvList_p)
{
    U16 i = 0;
    STSYS_DU32 *BaseAddress;

    BaseAddress = (STSYS_DU32*)(TmpClkrvList_p->InitPars.DCOBaseAddress);

    /* Program the DCO_MODE_CFG Default Value */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG, DEFAULT_VAL_DCO_MODE_CFG);/* 0x00 */ 
    
#if 0
    /* Program  DCO_CMD To Default Value */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,DEFAULT_VAL_DCO_CMD ); /* 0x01 */
    printf("Writing 1 DCO_CMD(0x164) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_CMD)));
    
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_REF_CNT, DEFAULT_VAL_REF_MAX);
    printf("Loading Val - Ref_Cnt(0x160) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_REF_CNT )));
#else
    /* Now load the Default REF_MAX Value */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_REF_CNT, 5000000);
    printf("Loading Val - Ref_Cnt(0x160) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_REF_CNT )));
    /* Program  DCO_CMD To Default Value */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,DEFAULT_VAL_DCO_CMD ); /* 0x01 */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,0x00 ); 
    printf("Writing 1 DCO_CMD(0x164) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_CMD)));

#endif    
    
    TmpClkrvList_p->HWContext.FS0_Reference = DEFAULT_VAL_REF_MAX ;
    /* Other counters resetting would be taken care by Set Nominal */
/*  Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,0x00 ); 
    printf("Writing 1 DCO_CMD(0x164) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_CMD)));        
*/    
    /* Program  the FS_A to its Default Value */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_FSA_SETUP, 0x04);  
    printf("Loading Val - FSA_Setup(0x010) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_FSA_SETUP )));
    /* Now program the clocks to default frequencies starting with the SD  */
    for(i = 0; i < MAX_NO_IN_FREQ_TABLE; i++)
    {
        if (DEFAULT_SD_FREQUENCY == NominalFreqTable[i].Frequency)
            break;
    }
    
    if(i == MAX_NO_IN_FREQ_TABLE)
    {
        return STCLKRV_ERROR_INVALID_FREQUENCY;
    }
    else
    {
      /*   Clkrv_Program_FS_Registers( TmpClkrvList_p,
                                    STCLKRV_SDVIDEO_CLOCK,
                                    NominalFreqTable[i].SDIV,
                                    NominalFreqTable[i].MD,
                                    NominalFreqTable[i].IPE
                                    );
      */                              
        /* Next Program the DCO_MODE_CFG Default Value */
    /*    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,0x01 );*/
        printf("DCO_Mode - 1 (0x0170) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_MODE_CFG )));
        
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].Frequency  = NominalFreqTable[i].Frequency ;
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].SDIV   = NominalFreqTable[i].SDIV  ;
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].MD     = NominalFreqTable[i].MD ;
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].IPE    = NominalFreqTable[i].IPE ;
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].Step   = NominalFreqTable[i].Step ;
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].Valid  = 1 ;
        
        TmpClkrvList_p->HWContext.Clocks[STCLKRV_SDVIDEO_CLOCK].PrevCounter    = 0 ;
    }
    
    /* Clkrv_Delay(FS_DELAY);*/
    /* TBD - check - Above Instructions should be enough for 10ns delay */
/*     Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,ASSERT_0 );*/

     
     
     printf("Writing 0 DCO_CMD(0x164) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_CMD)));
     printf("\n Dumping Registers after Reset H/W: %d\n",DebugCounter);   
     printf("Int Counter = %d, %d ",DebugCounter, ClkrvList_p->HWContext.Clocks[STCLKRV_PCM_CLOCK].PrevCounter);          
     Dump_Registers();
     
     return ST_NO_ERROR;
}

/****************************************************************************
Name         : STCLKRV_SetNominalFreq

Description  : Sets given Frequency of PCM, SPDIF, HD clock

Parameters   : STCLKRV_Handle_t Handle
               STCLKRV_Clock_t Clock
               U32 Frequency

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle
               STCLKRV_INVALID_FREQUENCY       Invalid Frequency
               STCLKRV_INVALID_SLAVE           Invalid Slave

See Also     : STCLKRV_Enable()
****************************************************************************/
 
ST_ErrorCode_t STCLKRV_SetNominalFreq(STCLKRV_Handle_t Handle,
                                      STCLKRV_Clock_t Clock,
                                      U32 Frequency)
{
    U32 i = 0;
    STCLKRV_ControlBlock_t *TmpClkrvList_p;
    STSYS_DU32 *BaseAddress;

    /* Check handle is valid */
    TmpClkrvList_p = FindInstanceByHandle(Handle);
    
    BaseAddress = TmpClkrvList_p->InitPars.DCOBaseAddress;
    if (TmpClkrvList_p == NULL)
    {
        return (ST_ERROR_INVALID_HANDLE);
    }
    
    if(Clock >= NO_OF_CLOCKS || Clock < STCLKRV_SDVIDEO_CLOCK)
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
        
    for(i = 0; i < MAX_NO_IN_FREQ_TABLE; i++)
    {
        if (Frequency == NominalFreqTable[i].Frequency)
            break;
    }
    
    if(i == MAX_NO_IN_FREQ_TABLE)
    {
        return STCLKRV_ERROR_INVALID_FREQUENCY;
    }
    
    /* TBD - Sem Take here */
    
    /* Protection from interrupt */
    TmpClkrvList_p->HWContext.Clocks[Clock].Valid          = 0 ;
    
    Clkrv_Program_FS_Registers(TmpClkrvList_p,
                               Clock,NominalFreqTable[i].SDIV,
                               NominalFreqTable[i].MD,
                               NominalFreqTable[i].IPE );
                               
    /* Reset Counters - set  DCO_CMD  to 0x01 */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,DEFAULT_VAL_DCO_CMD ); /* 0x01 */
    
    /* TBD - check if required - load the Default REF_MAX Value and reset other counters too */
    /* Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_REF_CNT, TmpClkrvList_p->HWContext.FS0_Reference);*/
    /* Reset the clocks in software, HW should take care of itself */
    TmpClkrvList_p->HWContext.Clocks[STCLKRV_PCM_CLOCK].PrevCounter         = 0 ;
    TmpClkrvList_p->HWContext.Clocks[STCLKRV_SPDIF_HD_CLOCK].PrevCounter    = 0 ;

    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,ASSERT_1 );
    
    /* TBD - synchronization */                                
    TmpClkrvList_p->HWContext.Clocks[Clock].Frequency      = NominalFreqTable[i].Frequency  ;
    TmpClkrvList_p->HWContext.Clocks[Clock].SDIV           = NominalFreqTable[i].SDIV       ;
    TmpClkrvList_p->HWContext.Clocks[Clock].MD             = NominalFreqTable[i].MD         ;
    TmpClkrvList_p->HWContext.Clocks[Clock].IPE            = NominalFreqTable[i].IPE        ;
    TmpClkrvList_p->HWContext.Clocks[Clock].Step           = NominalFreqTable[i].Step       ;
    TmpClkrvList_p->HWContext.Clocks[Clock].Valid          = 1 ;
    TmpClkrvList_p->HWContext.Clocks[Clock].PrevCounter    = 0 ;
/*    TmpClkrvList_p->HWContext.Clocks[Clock].CurrentCounter = 0 ;*/
    
/*    TmpClkrvList_p->HWContext[Clock].Ratio  = Clkrv_GetClockRatio(DEFAULT_SD_FREQUENCY, 
                                                        NominalFreqTable[i].Frequency);  
*/    
    /* Clkrv_Delay(FS_DELAY); */
    /* TBD - check - Above Instructions should be enough for 10ns delay */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,ASSERT_0 );
    
    /* Assuming always drift correction has to be turned on */  
/*    if(TmpClkrvList_p->HWContext.Clocks[STCLKRV_PCM_CLOCK].Valid == 1 &&
       TmpClkrvList_p->HWContext.Clocks[STCLKRV_SPDIF_HD_CLOCK].Valid == 1)
*/
    {
        Clkrvreg_WriteReg(BaseAddress, OFFSET_DCO_CMD, ASSERT_0 );
        TmpClkrvList_p->HWContext.Clocks[Clock].Valid = 1 ;
    }
    
    /* TBD - Sem Give here */
    return ST_NO_ERROR;
}

/****************************************************************************
Name         : Clkrv_ChangeClockFreqBy

Description  : 

Parameters   : 
               

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle
               STCLKRV_INVALID_FREQUENCY       Invalid Frequency
               STCLKRV_INVALID_SLAVE           Invalid Slave

See Also     : STCLKRV_Enable()
****************************************************************************/

static ST_ErrorCode_t Clkrv_ChangeClockFreqBy( STCLKRV_ControlBlock_t *TmpClkrvList_p,STCLKRV_Clock_t Clock, S32 Control )
{
    boolean MDChanged = FALSE;
    U16 ipe;
    S8  md;
    U8  Sdiv;
    U16 Last_ipe;
    U16 ChangePos_ipe;
    S16 ChangeNeg_ipe;
    U16 ChangeBy;
    U32 Step;
    U32 RegValue;
    STSYS_DU32 *BaseAddress;

    BaseAddress = TmpClkrvList_p->InitPars.DCOBaseAddress;
    /*
        <- range  ->
      Min............0............Max
    */
    
    /* DO calculations */
    /* TBD - synchro with Set Nominal */
    ipe  = TmpClkrvList_p->HWContext.Clocks[Clock].IPE;
    md   = TmpClkrvList_p->HWContext.Clocks[Clock].MD;
    Step = TmpClkrvList_p->HWContext.Clocks[Clock].Step;
    Sdiv = TmpClkrvList_p->HWContext.Clocks[Clock].SDIV;
    
    Last_ipe = ipe;
    ChangeBy = abs(Control);
    
    if( Control > 0)
    {
        ChangePos_ipe = (ipe + Control);
        
        if (ChangePos_ipe  > 32767) 
        {
            ipe = (ChangeBy  - (32768 - ipe));
            md--;
            MDChanged = 1;                
        }
        else
            ipe += ChangeBy;
    }
    else
    {
        ChangeNeg_ipe = ipe - ChangeBy;
        if (ChangeNeg_ipe < 0) 
        {
            ipe = (32768 - (ChangeBy - ipe));
            md++;
            MDChanged = 1;
        }
        else
            ipe -= ChangeBy;
    }

    Clkrv_Program_FS_Registers(TmpClkrvList_p,Clock,Sdiv,md,ipe);
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,ASSERT_1 );
    
    /* Update last control value and frequency */ 
    /* TBD - synchronization */
    TmpClkrvList_p->HWContext.Clocks[Clock].Frequency  +=(Control * (S32)Step);
    TmpClkrvList_p->HWContext.Clocks[Clock].MD         =  md     ;
    TmpClkrvList_p->HWContext.Clocks[Clock].IPE        =  ipe    ;
    /* Sdiv should not change */
    /*Clkrv_Delay(FS_DELAY);*/
    /* Check above delay is >10 ns */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,ASSERT_0 );
    
    return ST_NO_ERROR;
}
                                                   
/****************************************************************************
Name         : Clkrv_Program_FS_Registers

Description  : 

Parameters   : 
               

Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle
               STCLKRV_INVALID_FREQUENCY       Invalid Frequency
               STCLKRV_INVALID_SLAVE           Invalid Slave

See Also     : STCLKRV_Enable()
****************************************************************************/

static ST_ErrorCode_t Clkrv_Program_FS_Registers(STCLKRV_ControlBlock_t *TmpClkrvList_p, 
                                                 STCLKRV_Clock_t Clock,
                                                 U8  Sdiv,
                                                 S8  Md,
                                                 U16 Ipe)
{
    U8  temp_sdiv    = 0;
    U32 ClockOffset0 = 0;
    U32 ClockOffset1 = 0;
    U32 RegValue     = 0;
    U16 i            = 0;
    
    STSYS_DU32 *BaseAddress;

    BaseAddress = (STSYS_DU32*)(TmpClkrvList_p->InitPars.DCOBaseAddress);
    
    if(!(((Ipe >= 0 ) && (Ipe < 32768)) &&
       ((Md  <= -1) && (Md  >= -17))     &&
       ((Sdiv == 2)||(Sdiv==4)||(Sdiv==8)||
       (Sdiv==16)||(Sdiv==32)||(Sdiv==64)||
       (Sdiv==128)||(Sdiv==256))))
    {
        return (ST_ERROR_BAD_PARAMETER);
    }
    
    switch(Clock)
    {
        case STCLKRV_SDVIDEO_CLOCK:
            ClockOffset0 = OFFSET_SD_CLK_SETUP0;
            ClockOffset1 = OFFSET_SD_CLK_SETUP1;
            break;
        case STCLKRV_PCM_CLOCK:
            ClockOffset0 = OFFSET_PCM_CLK_SETUP0;
            ClockOffset1 = OFFSET_PCM_CLK_SETUP1;
            break;
        case STCLKRV_SPDIF_HD_CLOCK:
            ClockOffset0 = OFFSET_SPDIF_CLK_SETUP0;
            ClockOffset1 = OFFSET_SPDIF_CLK_SETUP1;
            break;
        default:
            return (ST_ERROR_BAD_PARAMETER);
            break;
    }
    
    /* TBD - check if bit needs to be reset first*/
    /* Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_MODE_CFG,ASSERT_0 );*/
    /* Any delay ???*/
    
    Clkrvreg_ReadReg(BaseAddress, ClockOffset0, (U32*)&RegValue);
    
    /* Update Sdiv */
    if(TmpClkrvList_p->HWContext.Clocks[Clock].SDIV != Sdiv )
    {
        temp_sdiv = Sdiv ;
        while(!((temp_sdiv >>=1) & 0x01))
        {
            i++;
        };
        
        RegValue &= (~SDIV_MASK); /*0xFFFFE3F*/
        RegValue |= (i << 6);
    }
    
    /* Update Md */
    if(TmpClkrvList_p->HWContext.Clocks[Clock].MD  != Md)
    {
        RegValue &= (~MD_MASK); /*0xFFFFFFE0*/
        RegValue |= (32 - abs(Md));
    }
    
    /* Write the Reg value */
    Clkrvreg_WriteReg(BaseAddress, ClockOffset0, RegValue);     /* (RegValue | 0x0400)*/
    
    if(TmpClkrvList_p->HWContext.Clocks[Clock].IPE != Ipe)
    {
        RegValue = Ipe & (IPE_MASK);
        Clkrvreg_WriteReg(BaseAddress, ClockOffset1, RegValue);
    }
    
 /*   printf("\nDumping Registers arfter setting SD");
    
    Dump_Registers();
 */   
    return ST_NO_ERROR;
}

/****************************************************************************
Name         : Clkrv_DCOInterruptHandler

Description  : 

Parameters   : 
               
Return Value : ST_ErrorCode_t specified as
               ST_NO_ERROR                     No errors determined
               ST_ERROR_INVALID_HANDLE         Invalid Handle
               STCLKRV_INVALID_FREQUENCY       Invalid Frequency
               STCLKRV_INVALID_SLAVE           Invalid Slave

See Also     : STCLKRV_Enable()
****************************************************************************/


static void Clkrv_DCOInterruptHandler( void *Param )
{
    U32 RegValue     = 0;
    


    STSYS_DU32 *BaseAddress;
    STCLKRV_ControlBlock_t *TmpClkrvList_p;
    
    DebugCounter++; /* Increment and monitor counter each time int gets invoked */
    
    TmpClkrvList_p = (STCLKRV_ControlBlock_t *)Param;
    BaseAddress    = TmpClkrvList_p->InitPars.DCOBaseAddress;
    
    Clkrvreg_ReadReg(BaseAddress,OFFSET_DCO_CMD,&RegValue );
    
    if(RegValue & 0x02)
    {
       /* STTBX_Print(("***Interrupt Invoked For DriftCorrection*** \n"));    */
    }
    
    /*READ COUNTERS */
    Clkrvreg_ReadReg(BaseAddress, OFFSET_DCO_PCM_CNT, &CounterPCM);
    Clkrvreg_ReadReg(BaseAddress, OFFSET_DCO_SPDIF_HD_CNT, &CounterSPDIF);
    
/*    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_REF_CNT, 5000000*(DebugCounter + 1));*/
/*    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,0x03);*/
/*    Clkrv_Delay(FS_DELAY);   */
    Clkrvreg_WriteReg(BaseAddress,OFFSET_DCO_CMD,ASSERT_0 );
    
 /*   semaphore_signal(&TmpClkrvList_p->DriftCorrectionSem);*/

}

static void Clkrv_DriftCorrectionTask(void *Param)
{
	
	/* Temp ptr into instance list */
    STCLKRV_ControlBlock_t *TmpClkrvList_p;

    TmpClkrvList_p = (STCLKRV_ControlBlock_t *)Param;
 /*   while(1)
    {
        semaphore_wait(&TmpClkrvList_p->DriftCorrectionSem);   
        printf("\nDumping Reg in Task:\n");
        Dump_Registers();
    }
 */   
}

static void Clkrv_Delay (U32 Delay_val)
{
	U32 i=0;
	
	for(i=0; i< Delay_val;i++);
	
}

static void Clkrvreg_WriteReg(STSYS_DU32 *Base, U32 Reg, U32 Value)
{
    U32 *Regaddr;

    Regaddr = (U32 *)((U32)Base + Reg);
    *Regaddr = Value;
}


static void Clkrvreg_ReadReg(STSYS_DU32 *Base, U32 Reg, U32 *Value)
{
    U32 *Regaddr;

    Regaddr = (U32 *)((U32)Base + Reg);
    *Value = *Regaddr;
}

static void Dump_Registers()
{
    U32 i = 0;
    printf("*** DUMPINg REGISTERS ***\n\n");
    printf("DCO_REF_CNT(0x160) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_REF_CNT)));
    printf("DCO_CMD(0x164) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_CMD)));
    printf("DCO_PCM_CNT(0x168) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_PCM_CNT)));
    printf("DCO_SPDIF_HD_CNT(0x16C) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_SPDIF_HD_CNT)));
    printf("DCO_MODE_CFG(0x170) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_DCO_MODE_CFG)));
    
    
    printf("FSA_SETUP(0x010) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_FSA_SETUP)));
    
    printf("SD_CLK_SETUP0(0x014) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_SD_CLK_SETUP0)));
    printf("SD_CLK_SETUP1(0x018) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_SD_CLK_SETUP1)));
    
    printf("PCM_CLK_SETUP0(0x020) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_PCM_CLK_SETUP0)));
    printf("PCM_CLK_SETUP1(0x024) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_PCM_CLK_SETUP1)));
    
    printf("HD_CLK_SETUP0(0x030) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_SPDIF_CLK_SETUP0)));
    printf("HD_CLK_SETUP1(0x034) = %x\n", *((U32*)(ST5100_CKG_BASE_ADDRESS + OFFSET_SPDIF_CLK_SETUP1)));
    
    
/*    
    printf("%x \t %x\n",,(*((U32*)(ST5100_CKG_BASE_ADDRESS + i))));
    for(i=0;i< 372 ;i +=4)
    {
        printf("%x \t %x\n",i,(*((U32*)(ST5100_CKG_BASE_ADDRESS + i))));
    }
  */  
}




/* End of emulator.c */
