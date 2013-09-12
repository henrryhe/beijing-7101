/**************************************************************************
File Name   : stmergetst.c

Description : STMERGE Test Routines

Copyright (C) 2005, STMicroelectronics

References  :

****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "stmergetst.h"
#include "stboot.h"

#if defined(STMERGE_DVBCI_TEST)
#include "stpccrd.h"
#endif

#if defined(ST_5301)
#include "..\..\..\target\core\include\bsp\mmu.h"
#elif defined(ST_5525)
#include "..\..\..\target\core\include\bsp.h"
#define TS_FPGA_MODE_REG           0x09A00000
#define TS_EPLD_ENABLE_REG         0x09C00000
void memory_map(void);
#endif

#if defined(STMERGE_USE_TUNER)
#include "tuner.h"
#include "stpio.h"
#include "lists.h"      /* satellite transponder and channel lists */
#endif /* STMERGE_USE_TUNER */

#if defined(USE_DMA_FOR_SWTS)
#if defined(ST_5528)
#include "stgpdma.h"
#else  /* 5100,7710,7100 etc */
#include "stfdma.h"
#endif
#else
#if !defined(ST_OSLINUX)
#include "stsys.h"
#endif
#endif

/* Private Types ---------------------------------------------------------- */
#if defined(STMERGE_USE_TUNER)

#if defined(ST_5100)
#ifndef PIO_4_BASE_ADDRESS
#define PIO_4_BASE_ADDRESS              ST5100_PIO4_BASE_ADDRESS
#endif
#ifndef PIO_4_INTERRUPT
#define PIO_4_INTERRUPT                 ST5100_PIO4_INTERRUPT
#endif
#ifndef PIO_4_INTERRUPT_LEVEL
#define PIO_4_INTERRUPT_LEVEL           5
#endif
#endif /* STMERGE_USE_TUNER */

/* Clock info structure setup here (for global use)  */
ST_ClockInfo_t ST_ClockInfo;

/* I2C setup */
#define FRONT_PIO                3
#define BACK_PIO                 3
#define PIO_FOR_SSC0_SDA         PIO_BIT_0
#define PIO_FOR_SSC0_SCL         PIO_BIT_1
#define PIO_FOR_SSC1_SDA         PIO_BIT_2
#define PIO_FOR_SSC1_SCL         PIO_BIT_3

ST_DeviceName_t I2C_DeviceName[] = { "I2C_BACK", "I2C_FRONT" };
STI2C_Handle_t  I2C_Handle[I2C_DEVICES];

/* PIO Setup */
#if defined(ARCHITECTURE_ST20) || defined(ARCHITECTURE_ST40)
#define PIO_PORTS 5
static STPIO_InitParams_t STPIO_InitParams[PIO_PORTS] =
{
{ (U32*) PIO_0_BASE_ADDRESS, PIO_0_INTERRUPT, PIO_0_INTERRUPT_LEVEL },
{ (U32*) PIO_1_BASE_ADDRESS, PIO_1_INTERRUPT, PIO_1_INTERRUPT_LEVEL },
{ (U32*) PIO_2_BASE_ADDRESS, PIO_2_INTERRUPT, PIO_2_INTERRUPT_LEVEL },
{ (U32*) PIO_3_BASE_ADDRESS, PIO_3_INTERRUPT, PIO_3_INTERRUPT_LEVEL },
{ (U32*) PIO_4_BASE_ADDRESS, PIO_4_INTERRUPT, PIO_4_INTERRUPT_LEVEL },
#if PIO_PORTS > 5
{ (U32*) PIO_5_BASE_ADDRESS, PIO_5_INTERRUPT, PIO_5_INTERRUPT_LEVEL },
#endif
#if PIO_PORTS > 6
{ (U32*) PIO_6_BASE_ADDRESS, PIO_6_INTERRUPT, PIO_6_INTERRUPT_LEVEL },
#endif
#if PIO_PORTS > 7
{ (U32*) PIO_7_BASE_ADDRESS, PIO_7_INTERRUPT, PIO_7_INTERRUPT_LEVEL }
#endif

};
#elif defined(ARCHITECTURE_ST200)
#define PIO_PORTS 5
static STPIO_InitParams_t STPIO_InitParams;

#endif


ST_DeviceName_t PIO_DeviceName[] = {"PIO0","PIO1","PIO2","PIO3","PIO4","PIO5","PIO6","PIO7"};
extern ST_ErrorCode_t EVENT_Setup(ST_DeviceName_t EventHandlerName);
extern ST_ErrorCode_t EVENT_Term(ST_DeviceName_t EventHandlerName);

U32 Pid[3] = {0};

#endif

/* Error messages */
typedef struct stmergetst_ErrorMessage_s
{
    ST_ErrorCode_t Error;
    char ErrorMsg[56];
} stmergetst_ErrorMessage_t;

#define ST_ERROR_UNKNOWN   ((U32)-1)
ST_ErrorCode_t             retVal;


#ifdef ST_OS20
/* Values copied from stpti test harness.h */
#define SYSTEM_PARTITION_SIZE           0x100000
/* Sizes of partitions */
#define INTERNAL_PARTITION_SIZE         (ST20_INTERNAL_MEMORY_SIZE-1200)
#define NCACHE_PARTITION_SIZE           0x40000

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

#ifdef UNIFIED_MEMORY
static unsigned char    ncache2_block[1];
#pragma ST_section      ( ncache2_block, "ncache2_section")
#endif

#pragma ST_section       ( retVal,   "ncache_section")

#elif defined(ST_OS21)/* if defined ST_OS21 */

#define SYSTEM_PARTITION_SIZE     0x100000
#define NCACHE_PARTITION_SIZE     0x10000
static unsigned char              external_block[SYSTEM_PARTITION_SIZE];
ST_Partition_t                    *DriverPartition;
ST_Partition_t                    *system_partition;
static unsigned char              ncache_block[NCACHE_PARTITION_SIZE];
ST_Partition_t                    *NcachePartition;

#elif defined(ST_OSLINUX)/* if defined ST_OS21 */
#define SYSTEM_PARTITION_SIZE     0x1
#define NCACHE_PARTITION_SIZE     0x1
ST_Partition_t                    *DriverPartition = (ST_Partition_t*)1;
ST_Partition_t                    *system_partition = (ST_Partition_t*)1;
ST_Partition_t                    *NcachePartition = (ST_Partition_t*)1;
#endif /* end of OS21 */


/* Harness revision value */
static const ST_Revision_t stmergetst_Rev = "STMERGETST-REL_2.2.1";

static BOOL    stmergetst_Passed;
static stmergetst_TestResult_t stmergetst_TestResults;

/* Private Function prototypes -------------------------------------------- */

char *stmergetst_ErrorString(ST_ErrorCode_t Error);


/* Private Macros --------------------------------------------------------- */

#define stmergetst_PrintDrvError(msg,err)    STTBX_Print(("%s = %s\n", msg,stmergetst_ErrorString(err)))


/* Functions -------------------------------------------------------------- */



/****************************************************************************
Name         : stmergetst_ErrorReport()

Description  : Outputs the appropriate error message string.

Parameters   : ST_ErrorCode_t pointer to an error store, the latest
               ST_ErrorCode_t error code returned, followed by the
               ST_ErrorCode_t expected error code.

Return Value : none

See Also     : ST_ErrorCode_t
 ****************************************************************************/

void stmergetst_ErrorReport( ST_ErrorCode_t *ErrorStore,
                             ST_ErrorCode_t ErrorGiven,
                             ST_ErrorCode_t ExpectedErr )
{
     switch( ErrorGiven )
    {
        case ST_NO_ERROR:
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

        case ST_ERROR_NO_MEMORY:
            STTBX_Print(( "ST_ERROR_NO_MEMORY - Not enough space for dynamic memory allocation\n" ));
            break;

        case ST_ERROR_NO_FREE_HANDLES:
            STTBX_Print(( "ST_ERROR_NO_FREE_HANDLES - Channel already Open\n" ));
            break;

        case ST_ERROR_TIMEOUT:
            STTBX_Print(( "ST_ERROR_TIMEOUT - Timeout occured\n" ));
            break;

        /* stmerge related errors */
        case STMERGE_ERROR_NOT_INITIALIZED:
            STTBX_Print(( "STMERGE_ERROR_NOT_INITIALIZED - Driver in not initialised\n"));
            break;

        case STMERGE_ERROR_BYPASS_MODE:
            STTBX_Print(( "STMERGE_ERROR_BYPASS_MODE - Bypass mode,Requested feature not supported\n"));
            break;

        case STMERGE_ERROR_DEVICE_NOT_SUPPORTED:
            STTBX_Print(( "STMERGE_ERROR_DEVICE_NOT_SUPPORTED - The Specified DeviceType is not supported\n"));
            break;

        case STMERGE_ERROR_PARAMETERS_NOT_SET:
            STTBX_Print(( "STMERGE_ERROR_PARAMETERS_NOT_SET - Parameters are not Set\n"));
            break;

        case STMERGE_ERROR_INVALID_BYTEALIGN:
            STTBX_Print(( "STMERGE_ERROR_INVALID_BYTEALIGN - Byte align. of SOP req. in parallel mode\n"));
            break;

        case STMERGE_ERROR_INVALID_PRIORITY:
            STTBX_Print(( "STMERGE_ERROR_INVALID_PRIORITY - Priority is not valid \n"));
            break;

        case STMERGE_ERROR_ILLEGAL_CONNECTION:
            STTBX_Print(( "STMERGE_ERROR_ILLEGAL_CONNECTION - Invalid Connection\n"));
            break;

        case STMERGE_ERROR_INVALID_COUNTER_RATE:
            STTBX_Print(( "STMERGE_ERROR_INVALID_COUNTER_RATE - Counter Increment rate is out of range\n"));
            break;

        case STMERGE_ERROR_INVALID_COUNTER_VALUE:
            STTBX_Print(( "STMERGE_ERROR_INVALID_COUNTER_VALUE - Counter Value is out of range\n"));
            break;

        case STMERGE_ERROR_INVALID_DESTINATION_ID:
            STTBX_Print(( "STMERGE_ERROR_INVALID_DESTINATION_ID - Destination Object Id is invalid\n"));
            break;

        case STMERGE_ERROR_INVALID_ID:
            STTBX_Print(( "STMERGE_ERROR_INVALID_ID - Object Id is not recognized\n"));
            break;

        case STMERGE_ERROR_INVALID_LENGTH:
            STTBX_Print(( "STMERGE_ERROR_INVALID_LENGTH - Packet length is out of range\n"));
            break;

        case STMERGE_ERROR_INVALID_MODE:
            STTBX_Print(( "STMERGE_ERROR_INVALID_MODE - Requested mode is not supported\n"));
            break;

        case STMERGE_ERROR_INVALID_PACE:
            STTBX_Print(( "STMERGE_ERROR_INVALID_PACE - Pace value out of range\n"));
            break;

        case STMERGE_ERROR_INVALID_PARALLEL:
            STTBX_Print(( "STMERGE_ERROR_INVALID_PARALLEL - TSIN instance can only operate in SERIAL mode\n"));
            break;

        case STMERGE_ERROR_INVALID_SOURCE_ID:
            STTBX_Print(( "STMERGE_ERROR_INVALID_SOURCE_ID - Source Object Id is not a valid Source\n"));
            break;

        case STMERGE_ERROR_INVALID_SYNCDROP:
            STTBX_Print(( "STMERGE_ERROR_INVALID_SYNCDROP - Sync drop is out of range\n"));
            break;

        case STMERGE_ERROR_INVALID_SYNCLOCK:
            STTBX_Print(( "STMERGE_ERROR_INVALID_SYNCLOCK - Sync lock is out of range\n"));
            break;

        case STMERGE_ERROR_INVALID_TAGGING:
            STTBX_Print(( "STMERGE_ERROR_INVALID_TAGGING - Tagging value is invalid\n"));
            break;

        case STMERGE_ERROR_INVALID_TRIGGER:
            STTBX_Print(( "STMERGE_ERROR_INVALID_TRIGGER - Trigger value is invalid\n"));
            break;

        case STMERGE_ERROR_MERGE_MEMORY:
            STTBX_Print(( "STMERGE_ERROR_MERGE_MEMORY - Merge Memory Map is invalid\n"));
            break;

        case STMERGE_ERROR_FEATURE_NOT_SUPPORTED:
            STTBX_Print(( "STMERGE_ERROR_FEATURE_NOT_SUPPORTED - Not build with STMERGE_DEFAULT_PARAMETERS\n" ));
            break;

#if defined(ST_5528) && defined(USE_DMA_FOR_SWTS)
        /* Gpdmda related errors */
        case STGPDMA_ERROR_BUS :
            STTBX_Print(("STGPDMA_ERROR_BUS\n "));
            break;

        case STGPDMA_ERROR_DATA_ALIGNMENT :
            STTBX_Print(("STGPDMA_ERROR_DATA_ALIGNMENT\n "));
            break;

        case STGPDMA_ERROR_DATA_UNIT_SIZE :
            STTBX_Print(("STGPDMA_ERROR_DATA_UNIT_SIZE\n "));
            break;

        case STGPDMA_ERROR_INVALID_CHANNEL :
            STTBX_Print(("STGPDMA_ERROR_INVALID_CHANNEL\n "));
            break;

        case STGPDMA_ERROR_INVALID_EVENT :
            STTBX_Print(("STGPDMA_ERROR_INVALID_EVENT\n "));
            break;

        case STGPDMA_ERROR_INVALID_PROTOCOL :
            STTBX_Print(("STGPDMA_ERROR_INVALID_PROTOCOL\n "));
            break;

        case STGPDMA_ERROR_INVALID_SUBCHANNEL :
            STTBX_Print(("STGPDMA_ERROR_INVALID_SUBCHANNEL\n "));
            break;

        case STGPDMA_ERROR_INVALID_TRANSFERID :
            STTBX_Print(("STGPDMA_ERROR_INVALID_TRANSFERID\n "));
            break;

        case STGPDMA_ERROR_QUEUE_FULL :
            STTBX_Print(("STGPDMA_ERROR_QUEUE_FULL\n "));
            break;

        case STGPDMA_ERROR_SUBCHANNEL_BUSY :
            STTBX_Print(("STGPDMA_ERROR_SUBCHANNEL_BUSY\n "));
            break;

        case STGPDMA_ERROR_TIMING_MODEL :
            STTBX_Print(("STGPDMA_ERROR_TIMING_MODEL\n "));
            break;

        case STGPDMA_ERROR_TRANSFER_ABORTED :
            STTBX_Print(("STGPDMA_ERROR_TRANSFER_ABORTED\n "));
            break;

        case STGPDMA_ERROR_TRANSFER_TYPE :
            STTBX_Print(("STGPDMA_ERROR_TRANSFER_TYPE\n "));
            break;

        case STGPDMA_ERROR_DEVICE_NOT_SUPPORTED :
            STTBX_Print(("STGPDMA_ERROR_DEVICE_NOT_SUPPORTED\n "));
            break;

        case STGPDMA_ERROR_EVENT_REGISTER :
            STTBX_Print(("STGPDMA_ERROR_EVENT_REGISTER\n "));
            break;

        case STGPDMA_ERROR_EVENT_UNREGISTER :
            STTBX_Print(("STGPDMA_ERROR_EVENT_UNREGISTER\n "));
            break;

#elif defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined(ST_5301) || defined(ST_7109) || defined(ST_5525) || defined(ST_5524)
#if 0
        /* FDMA error codes */
        case STFDMA_ERROR_NOT_INITIALIZED:
            STTBX_Print(("STFDMA_ERROR_NOT_INITIALIZED\n"));
            break;

        case STFDMA_ERROR_DEVICE_NOT_SUPPORTED:
            STTBX_Print(("STFDMA_ERROR_DEVICE_NOT_SUPPORTED\n"));
            break;

        case STFDMA_ERROR_NO_CALLBACK_TASK:
            STTBX_Print(("STFDMA_ERROR_NO_CALLBACK_TASK\n"));
            break;

        case STFDMA_ERROR_BLOCKING_TIMEOUT:
            STTBX_Print(("STFDMA_ERROR_BLOCKING_TIMEOUT\n"));
            break;

        case STFDMA_ERROR_CHANNEL_BUSY:
            STTBX_Print(("STFDMA_ERROR_CHANNEL_BUSY\n"));
            break;

        case STFDMA_ERROR_NO_FREE_CHANNELS:
            STTBX_Print(("STFDMA_ERROR_NO_FREE_CHANNELS\n"));
            break;

        case STFDMA_ERROR_ALL_CHANNELS_LOCKED:
            STTBX_Print(("STFDMA_ERROR_ALL_CHANNELS_LOCKED\n"));
            break;

        case STFDMA_ERROR_CHANNEL_NOT_LOCKED:
            STTBX_Print(("STFDMA_ERROR_CHANNEL_NOT_LOCKED\n"));
            break;

        case STFDMA_ERROR_UNKNOWN_CHANNEL_ID:
            STTBX_Print(("STFDMA_ERROR_UNKNOWN_CHANNEL_ID\n"));
            break;

        case STFDMA_ERROR_INVALID_TRANSFER_ID:
            STTBX_Print(("STFDMA_ERROR_INVALID_TRANSFER_ID\n"));
            break;

        case STFDMA_ERROR_TRANSFER_ABORTED:
            STTBX_Print(("STFDMA_ERROR_TRANSFER_ABORTED\n"));
            break;

        case STFDMA_ERROR_TRANSFER_IN_PROGRESS:
            STTBX_Print(("STFDMA_ERROR_TRANSFER_IN_PROGRESS \n"));
            break;

        case STFDMA_ERROR_INVALID_BUFFER:
            STTBX_Print(("STFDMA_ERROR_INVALID_BUFFER \n"));
            break;

        case STFDMA_ERROR_INVALID_CHANNEL:
            STTBX_Print(("STFDMA_ERROR_INVALID_CHANNEL \n"));
            break;

        case STFDMA_ERROR_INVALID_CONTEXT_ID:
            STTBX_Print(("STFDMA_ERROR_INVALID_CONTEXT_ID \n"));
            break;

        case STFDMA_ERROR_INVALID_SC_RANGE:
            STTBX_Print(("STFDMA_ERROR_INVALID_SC_RANGE \n"));
            break;

        case STFDMA_ERROR_NO_FREE_CONTEXTS:
            STTBX_Print(("STFDMA_ERROR_NO_FREE_CONTEXTS \n"));
            break;

#endif /*TBR*/
#endif

#if defined(STMERGE_USE_TUNER)
        /* I2C related error codes */
        case STI2C_ERROR_LINE_STATE:
            STTBX_Print(("STI2C_ERROR_LINE_STATE \n"));
            break;

        case STI2C_ERROR_STATUS:
            STTBX_Print(("STI2C_ERROR_STATUS \n"));
            break;

        case STI2C_ERROR_ADDRESS_ACK:
            STTBX_Print(("STI2C_ERROR_ADDRESS_ACK \n"));
            break;

        case STI2C_ERROR_WRITE_ACK:
            STTBX_Print(("STI2C_ERROR_WRITE_ACK \n"));
            break;

        case STI2C_ERROR_NO_DATA:
            STTBX_Print(("STI2C_ERROR_NO_DATA \n"));
            break;

        case STI2C_ERROR_PIO:
            STTBX_Print(("STI2C_ERROR_PIO \n"));
            break;

        case STI2C_ERROR_BUS_IN_USE:
            STTBX_Print(("STI2C_ERROR_BUS_IN_USE \n"));
            break;

        case STI2C_ERROR_EVT_REGISTER:
            STTBX_Print(("STI2C_ERROR_EVT_REGISTER \n"));
            break;

        /* Tuner related error codes */
        case STTUNER_ERROR_UNSPECIFIED:
            STTBX_Print(("STTUNER_ERROR_UNSPECIFIED \n"));
            break;

        case STTUNER_ERROR_HWFAIL:
            STTBX_Print(("STTUNER_ERROR_HWFAIL \n"));
            break;

        case STTUNER_ERROR_EVT_REGISTER:
            STTBX_Print(("STTUNER_ERROR_EVT_REGISTER \n"));
            break;

        case STTUNER_ERROR_ID:
            STTBX_Print(("STTUNER_ERROR_ID \n"));
            break;

        case STTUNER_ERROR_POINTER:
            STTBX_Print(("STTUNER_ERROR_POINTER \n"));
            break;

        case STTUNER_ERROR_INITSTATE:
            STTBX_Print(("STTUNER_ERROR_INITSTATE \n"));
            break;
#endif

#if defined(STMERGE_DVBCI_TEST)

        /* stpccrd.h */
        case STPCCRD_ERROR_EVT:
            STTBX_Print(("STPCCRD_ERROR_EVT \n"));
            break;

        case STPCCRD_ERROR_BADREAD:
            STTBX_Print(("STPCCRD_ERROR_BADREAD \n"));
            break;

        case STPCCRD_ERROR_BADWRITE:
            STTBX_Print(("STPCCRD_ERROR_BADWRITE \n"));
            break;

        case STPCCRD_ERROR_I2C:
            STTBX_Print(("STPCCRD_ERROR_I2C \n"));
            break;

        case STPCCRD_ERROR_CIS_READ:
            STTBX_Print(("STPCCRD_ERROR_CIS_READ \n"));
            break;

        case STPCCRD_ERROR_NO_MODULE:
            STTBX_Print(("STPCCRD_ERROR_NO_MODULE \n"));
            break;

        case STPCCRD_ERROR_CI_RESET:
            STTBX_Print(("STPCCRD_ERROR_CI_RESET \n"));
            break;

        case STPCCRD_ERROR_MOD_RESET:
            STTBX_Print(("STPCCRD_ERROR_MOD_RESET \n"));
            break;

#endif

        default:
            STTBX_Print(( "*** Unrecognised return code ***\n" ));
            break;
    }

    /* If ErrorGiven does not match ExpectedErr, update
       ErrorStore only if no previous error was noted */

    if (ErrorGiven != ExpectedErr)
    {
        STTBX_Print(("*** ERROR : Unexpected value returned *** =  %x\n", ErrorGiven));
        if (*ErrorStore == ST_NO_ERROR)
        {
            *ErrorStore = ErrorGiven;
            stmergetst_SetSuccessState(FALSE);
        }
    }

}/* stmergetst_ErrorReport */

/****************************************************************************
Name            : stmergetst_TestConclusion

Description     : Conclude test sequence by displaying the given message and
                  incrementing the pass/fail count

Parameters      :

****************************************************************************/

void stmergetst_TestConclusion(char *Msg)
{

    STTBX_Print(("********************************************************************\n"));
    STTBX_Print(("%s\n",Msg));

    if (stmergetst_IsTestSuccessful() == TRUE)
    {
        stmergetst_TestResults.Passed++;
    }
    else
    {
        stmergetst_TestResults.Failed++;
    }

    /* Always display number passed */
    STTBX_Print((">>>> PASSED (%d)\n",stmergetst_TestResults.Passed));

    /* Only display failed if failures occur */
    if (stmergetst_TestResults.Failed !=0)
    {
        STTBX_Print((">>>> FAILED (%d)\n",stmergetst_TestResults.Failed));
    }

    STTBX_Print(("********************************************************************\n"));

}/* stmergetst_TestConclusion */

/****************************************************************************
Name            : stmergetst_SetSuccessState

Description     : Load given value to success control

Parameters      :

****************************************************************************/

void stmergetst_SetSuccessState(BOOL Value)
{
    stmergetst_Passed = Value;
}

/****************************************************************************
Name            : stmergetst_IsTestSuccessful

Description     : Returns current condition of test

Parameters      :

****************************************************************************/

BOOL stmergetst_IsTestSuccessful(void)
{
    return stmergetst_Passed;
}

/****************************************************************************
Name            : stmergetst_ClearTestCount

Description     : Reset Test count

Parameters      :

****************************************************************************/

void stmergetst_ClearTestCount(void)
{
    stmergetst_TestResults.Passed = 0;
    stmergetst_TestResults.Failed = 0;
}

#if !defined(ST_OSLINUX)
#ifndef ST_OS21
/****************************************************************************
Name            : stmergetst_GetSystemPartition

Description     : Returns the system partition

Parameters      :

****************************************************************************/

partition_t *stmergetst_GetSystemPartition(void)
{
    return system_partition;
}

/****************************************************************************
Name            : stmergetst_GetInternalPartition

Description     : Returns the driver partition

Parameters      :

****************************************************************************/

partition_t *stmergetst_GetInternalPartition(void)
{
    return internal_partition;
}

/****************************************************************************
Name            : stmergetst_GetNCachePartition

Description     : Returns the NCache partition

Parameters      :

****************************************************************************/

partition_t *stmergetst_GetNCachePartition(void)
{
    return ncache_partition;
}
#endif
#endif
#if !defined(ST_OSLINUX)
/* -- Private functions -- */
/****************************************************************************
Name         : TBX_Init

Description  : STTBX initialisation funciton

Parameters   :

Return Value : FALSE if all ok.

See Also     :
****************************************************************************/

BOOL TBX_Init(void)
{
    ST_ErrorCode_t Error;
    STTBX_InitParams_t TBXInitParams;

    /* Initialize the toolbox */
    TBXInitParams.SupportedDevices    = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultOutputDevice = STTBX_DEVICE_DCU;
    TBXInitParams.DefaultInputDevice  = STTBX_DEVICE_DCU;
    TBXInitParams.CPUPartition_p      = system_partition;
    Error = STTBX_Init( "STTBX:0", &TBXInitParams );

    if(ST_NO_ERROR != Error)
    {
        printf("STTBX Initialize error\n");
    }

    return Error ? TRUE : FALSE;
}/* TBX_Init */

/****************************************************************************
Name         : BOOT_Init

Description  : STBOOT initialisation funciton

Parameters   :

Return Value : FALSE if all ok.

See Also     :
****************************************************************************/

BOOL BOOT_Init(void)
{

    ST_ErrorCode_t Error;
    STBOOT_InitParams_t     stboot_InitParams;

#ifndef DISABLE_DCACHE
#if defined CACHEABLE_BASE_ADDRESS
    STBOOT_DCache_Area_t DCacheMap[] =
    {
        { (U32 *)CACHEABLE_BASE_ADDRESS, (U32 *)CACHEABLE_STOP_ADDRESS }, /* assumed ok */
        { NULL, NULL }
    };
#elif defined(ST_5528) || defined(ST_5100) || defined(ST_7710) || defined(ST_7100) || defined(ST_7109)
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
#endif
/*  How to do this for 5525 */


    /* Initialize stboot */
#if defined DISABLE_ICACHE
    stboot_InitParams.ICacheEnabled             = FALSE;
#else
    stboot_InitParams.ICacheEnabled             = TRUE;
#endif

#if defined DISABLE_DCACHE
    stboot_InitParams.DCacheMap                 = NULL;
#else
    stboot_InitParams.DCacheMap                 = DCacheMap;
#endif

    stboot_InitParams.BackendType.DeviceType    = STBOOT_DEVICE_UNKNOWN;
    stboot_InitParams.BackendType.MajorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitParams.BackendType.MinorRevision = STBOOT_REVISION_UNKNOWN;
    stboot_InitParams.CacheBaseAddress          = (U32 *)CACHE_BASE_ADDRESS;
    stboot_InitParams.MemorySize                = (STBOOT_DramMemorySize_t)SDRAM_SIZE;
    stboot_InitParams.SDRAMFrequency            = SDRAM_FREQUENCY;
#ifdef ST_OS21
    stboot_InitParams.TimeslicingEnabled        = TRUE;
#endif

    /* Initialize the boot driver */
    Error = STBOOT_Init( "STBOOT:0", &stboot_InitParams );

    if( ST_NO_ERROR != Error)
    {
        /*STTBX is not initialized*/
        printf("BOOT ERROR\n");
    }

    return Error ? TRUE : FALSE;

}/* BOOT_Init */
#endif

/****************************************************************************
Name         : TST_Init

Description  : Initialise the ST Testtool

Parameters   :

Return Value : FALSE if all ok.

See Also     :
****************************************************************************/

BOOL TST_Init(void)
{
    ST_ErrorCode_t Error;
    STTST_InitParams_t STTST_InitParams;

    STTST_InitParams.CPUPartition_p = system_partition;
    STTST_InitParams.NbMaxOfSymbols = 300;

    /* Set input file name array to all zeros */
    memset(STTST_InitParams.InputFileName, 0, sizeof(char) * max_tok_len);

    Error = STTST_Init(&STTST_InitParams);
    if (Error != ST_NO_ERROR)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}/* TST_Init */

#if defined(ST_OSLINUX)
/****************************************************************************
Name         : InitCommands

Description  : Initialise the test commands

****************************************************************************/

BOOL InitCommands(void)
{
    BOOL RetErr = FALSE;

    RetErr |= STTST_RegisterCommand("Normal", stmerge_NormalTest, "STMERGE Normal use test.");
    RetErr |= STTST_RegisterCommand("Errant", stmerge_ErrantTest, "STMERGE Errant use test.");
    RetErr |= STTST_RegisterCommand("ByPass", stmerge_ByPass,     "STMERGE ByPass Scenario.");
    RetErr |= STTST_RegisterCommand("Merge_2_1", stmerge_Merge_2_1_Test, "STMERGE Merge 2:1 test.");
    RetErr |= STTST_RegisterCommand("Merge_3_1",  stmerge_Merge_3_1_Test,      "STMERGE Merge 3:1 test.");
    RetErr |= STTST_RegisterCommand("Disconnect",  stmerge_DisconnectTest,  "STMERGE Disconnect test.");
    RetErr |= STTST_RegisterCommand("DuplicateInput", stmerge_DuplicateInputTest, "STMERGE Duplicate Input Test.");

    RetErr |= STTST_AssignInteger("TSIN0", STMERGE_TSIN_0, TRUE);
    RetErr |= STTST_AssignInteger("TSIN1", STMERGE_TSIN_1, TRUE);
    RetErr |= STTST_AssignInteger("TSIN2", STMERGE_TSIN_2, TRUE);
#if (STMERGE_NUM_TSIN > 3)
    RetErr |= STTST_AssignInteger("TSIN3", STMERGE_TSIN_3, TRUE);
#endif
#if (STMERGE_NUM_TSIN > 4)
    RetErr |= STTST_AssignInteger("TSIN4", STMERGE_TSIN_4, TRUE);
#endif
    RetErr |= STTST_AssignInteger("SWTS0", STMERGE_SWTS_0, TRUE);
#if (NUMBER_OF_SWTS > 1)
    RetErr |= STTST_AssignInteger("SWTS1", STMERGE_SWTS_1, TRUE);
#endif
#if (NUMBER_OF_SWTS > 2)
    RetErr |= STTST_AssignInteger("SWTS2", STMERGE_SWTS_2, TRUE);
#endif
#if (NUMBER_OF_SWTS > 3)
    RetErr |= STTST_AssignInteger("SWTS3", STMERGE_SWTS_3, TRUE);
#endif
    RetErr |= STTST_AssignInteger("PTI0",STMERGE_PTI_0, TRUE);
#if (STMERGE_NUM_PTI > 1)
    RetErr |= STTST_AssignInteger("PTI1",STMERGE_PTI_0, TRUE);
#endif
    return(RetErr);
}/* InitCommands */
#endif

#if defined(ST_OSLINUX)
/*-------------------------------------------------------------------------
 * Function : TST_Setup
 *            Initialise testtool
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
ST_ErrorCode_t TST_Setup(void)
{
    STTST_InitParams_t  TST_InitParams = {0};

    TST_InitParams.CPUPartition_p = system_partition;
    TST_InitParams.NbMaxOfSymbols = 300;
    strcpy( TST_InitParams.InputFileName, "" );

    if ( STTST_Init(&TST_InitParams) == TRUE )
    {
        STTBX_Print(("STTST_Init()=Failed\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    STTBX_Print(("TST_Setup()=%s\n", STTST_GetRevision()));

    /*if ( STTST_SetMode(STTST_INTERACTIVE_MODE) == TRUE )
    {
        STTBX_Print(("STTST_SetMode()=Failed\n"));
        return ST_ERROR_BAD_PARAMETER;
    }
*/
    /*ST_ErrorCode = TST_InitCommand();
    if ( ST_ErrorCode != ST_NO_ERROR )
    {
        STTBX_Print(("TST_InitCommand()=Failed\n"));
        return ST_ErrorCode;
    }*/

    return ST_NO_ERROR;
}

#endif

static stmergetst_ErrorMessage_t stmergetst_ErrorTable[] =
{
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



/****************************************************************************
Name         : stmergetst_ErrorString

Description  : Testtool interactive mode

****************************************************************************/

char *stmergetst_ErrorString(ST_ErrorCode_t Error)
{
    stmergetst_ErrorMessage_t *mp = NULL;

    mp = stmergetst_ErrorTable;
    while (mp->Error != ST_ERROR_UNKNOWN)
    {
        if (mp->Error == Error)
            break;
        mp++;
    }

    return mp->ErrorMsg;

}/* stmergetst_ErrorString */

#if !defined(ST_OSLINUX)
/****************************************************************************
Name         : InitCommands

Description  : Initialise the test commands

****************************************************************************/

BOOL InitCommands(void)
{
    BOOL RetErr = FALSE;

    RetErr |= STTST_RegisterCommand("Normal", stmerge_NormalTest, "STMERGE Normal use test.");
    RetErr |= STTST_RegisterCommand("Merge_2_1", stmerge_Merge_2_1_Test, "STMERGE Merge 2:1 test.");
    RetErr |= STTST_RegisterCommand("Errant", stmerge_ErrantTest, "STMERGE Errant use test.");
    RetErr |= STTST_RegisterCommand("Leak",   stmerge_LeakTest,   "STMERGE Memory leak test.");
    RetErr |= STTST_RegisterCommand("Runall", stmerge_Runall,     "STMERGE Run All tests.");
    RetErr |= STTST_RegisterCommand("ByPass", stmerge_ByPass,     "STMERGE ByPass Scenario.");
    RetErr |= STTST_RegisterCommand("Merge_3_1",  stmerge_Merge_3_1_Test,      "STMERGE Merge 3:1 test.");
    RetErr |= STTST_RegisterCommand("Altout",  stmerge_AltoutTest,"STMERGE Altout test.");
    RetErr |= STTST_RegisterCommand("Disconnect",  stmerge_DisconnectTest,  "STMERGE Disconnect test.");
    RetErr |= STTST_RegisterCommand("DuplicateInput", stmerge_DuplicateInputTest, "STMERGE Duplicate Input Test.");
#ifdef STMERGE_CHANNEL_CHANGE
    RetErr |= STTST_RegisterCommand("Channel", stmerge_ChannelChangeTest, "STMERGE Channel Change Test case.");
#endif
#ifdef STMERGE_INTERRUPT_SUPPORT
    RetErr |= STTST_RegisterCommand("ram_int", stmerge_RAMOverflowIntTest, "STMERGE RAM overflow Test case.");
#endif
#ifdef STMERGE_1394_PRODUCER_CONSUMER
    RetErr |= STTST_RegisterCommand("Test_1394", stmerge_1394_Producer_Consumer, "STMERGE Producer/Consumer Test");
#endif
#if defined(ST_7100) || defined(ST_7109)
    RetErr |= STTST_RegisterCommand("Config", stmerge_ConfigTest, "STMERGE Config Test on 7100 ");
#endif
#ifdef STMERGE_DVBCI_TEST
    RetErr |= STTST_RegisterCommand("DVBCI", stmerge_DVBCITest, "STMERGE DVBCI Test ");
#endif

    RetErr |= STTST_AssignInteger("TSIN0", STMERGE_TSIN_0, TRUE);
    RetErr |= STTST_AssignInteger("TSIN1", STMERGE_TSIN_1, TRUE);
    RetErr |= STTST_AssignInteger("TSIN2", STMERGE_TSIN_2, TRUE);
    RetErr |= STTST_AssignInteger("TSIN3", STMERGE_TSIN_3, TRUE);
    RetErr |= STTST_AssignInteger("TSIN4", STMERGE_TSIN_4, TRUE);
    RetErr |= STTST_AssignInteger("SWTS0", STMERGE_SWTS_0, TRUE);
#if (NUMBER_OF_SWTS > 1)
    RetErr |= STTST_AssignInteger("SWTS1", STMERGE_SWTS_1, TRUE);
#endif
#if (NUMBER_OF_SWTS > 2)
    RetErr |= STTST_AssignInteger("SWTS2", STMERGE_SWTS_2, TRUE);
#endif
#if (NUMBER_OF_SWTS > 3)
    RetErr |= STTST_AssignInteger("SWTS3", STMERGE_SWTS_3, TRUE);
#endif
    RetErr |= STTST_AssignInteger("ALTOUT0",STMERGE_ALTOUT_0, TRUE);
    RetErr |= STTST_AssignInteger("IN1394",STMERGE_1394IN_0, TRUE);
    RetErr |= STTST_AssignInteger("OUT1394",STMERGE_1394OUT_0, TRUE);
    RetErr |= STTST_AssignInteger("PTI0",STMERGE_PTI_0, TRUE);
    RetErr |= STTST_AssignInteger("PTI1",STMERGE_PTI_0, TRUE);

    return(RetErr);
}/* InitCommands */
#endif
#if defined(STMERGE_USE_TUNER)

/*-------------------------------------------------------------------------
 * Function : PIO_Init
 *            Setup PIO ports
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
BOOL PIO_Init( void )
{
    ST_ErrorCode_t Error       = ST_NO_ERROR;
    ST_ErrorCode_t StoredError = ST_NO_ERROR;

#if defined(ARCHITECTURE_ST20) || defined(ARCHITECTURE_ST40)
    int Instance;

    for (Instance = 0; Instance < PIO_PORTS; Instance++)
    {
        STPIO_InitParams[Instance].DriverPartition = system_partition;
        Error = STPIO_Init( PIO_DeviceName[Instance], &STPIO_InitParams[Instance] );
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        if (Error != ST_NO_ERROR)
        {
            return ( TRUE );
        }
    }

#elif defined(ARCHITECTURE_ST200)

    STPIO_InitParams.DriverPartition = system_partition;
    STPIO_InitParams.BaseAddress = (U32 *) PIO_0_BASE_ADDRESS;
    STPIO_InitParams.InterruptNumber = PIO_0_INTERRUPT;
    STPIO_InitParams.InterruptLevel = PIO_0_INTERRUPT_LEVEL;
    Error = STPIO_Init( PIO_DeviceName[0], &STPIO_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPIO_InitParams.BaseAddress = (U32 *) PIO_1_BASE_ADDRESS;
    STPIO_InitParams.InterruptNumber = PIO_1_INTERRUPT;
    STPIO_InitParams.InterruptLevel = PIO_1_INTERRUPT_LEVEL;
    STPIO_InitParams.DriverPartition = system_partition;
    Error = STPIO_Init( PIO_DeviceName[1], &STPIO_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPIO_InitParams.BaseAddress = (U32 *) PIO_2_BASE_ADDRESS;
    STPIO_InitParams.InterruptNumber = PIO_2_INTERRUPT;
    STPIO_InitParams.InterruptLevel = PIO_2_INTERRUPT_LEVEL;
    STPIO_InitParams.DriverPartition = system_partition;
    Error = STPIO_Init( PIO_DeviceName[2], &STPIO_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPIO_InitParams.BaseAddress = (U32 *) PIO_3_BASE_ADDRESS;
    STPIO_InitParams.InterruptNumber = PIO_3_INTERRUPT;
    STPIO_InitParams.InterruptLevel  = PIO_3_INTERRUPT_LEVEL;
    STPIO_InitParams.DriverPartition = system_partition;
    Error = STPIO_Init( PIO_DeviceName[3], &STPIO_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    STPIO_InitParams.BaseAddress = (U32 *) PIO_4_BASE_ADDRESS;
    STPIO_InitParams.InterruptNumber = PIO_4_INTERRUPT;
    STPIO_InitParams.InterruptLevel = PIO_4_INTERRUPT_LEVEL;
    STPIO_InitParams.DriverPartition = system_partition;
    Error = STPIO_Init( PIO_DeviceName[4], &STPIO_InitParams );
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

#endif


    return ( FALSE );
}

/*-------------------------------------------------------------------------
 * Function : PIO_Term
 *            Terminate PIO ports
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/
BOOL PIO_Term( void )
{
    ST_ErrorCode_t Error       = ST_NO_ERROR;
    ST_ErrorCode_t StoredError = ST_NO_ERROR;
    STPIO_TermParams_t PIO_TermParams;
    int Instance;

    for (Instance = 0; Instance < PIO_PORTS; Instance++)
    {

        PIO_TermParams.ForceTerminate = TRUE;
        Error = STPIO_Term(PIO_DeviceName[Instance], &PIO_TermParams);
        stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
        if (Error != ST_NO_ERROR)
        {
            return ( TRUE );
        }
    }

    return ( FALSE );

}/* PIO_Term */

/*-------------------------------------------------------------------------
 * Function : I2C_Init
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/

BOOL I2C_Init(void)
{
    ST_ErrorCode_t     Error       = ST_NO_ERROR;
    ST_ErrorCode_t     StoredError = ST_NO_ERROR;
    STI2C_InitParams_t STI2C_InitParams;

    /* I2C setup before TUNER configurations */
    memset(&STI2C_InitParams, '\0', sizeof(STI2C_InitParams_t) );

    STTBX_Print(("STI2C_GetRevision() = %s",STI2C_GetRevision()));

    /* Back I2C bus */
    STI2C_InitParams.BaseAddress       = (U32 *)SSC_0_BASE_ADDRESS;
    STI2C_InitParams.InterruptNumber   = SSC_0_INTERRUPT;
    STI2C_InitParams.InterruptLevel    = SSC_0_INTERRUPT_LEVEL;
    STI2C_InitParams.PIOforSDA.BitMask = PIO_FOR_SSC0_SDA;
    STI2C_InitParams.PIOforSCL.BitMask = PIO_FOR_SSC0_SCL;
    STI2C_InitParams.MasterSlave       = STI2C_MASTER;
    STI2C_InitParams.BaudRate          = STI2C_RATE_NORMAL;
    STI2C_InitParams.MaxHandles        = 5;
    STI2C_InitParams.ClockFrequency    = ST_ClockInfo.CommsBlock;
    STI2C_InitParams.DriverPartition   = DriverPartition;

    strcpy(STI2C_InitParams.PIOforSDA.PortName, PIO_DeviceName[BACK_PIO]);
    strcpy(STI2C_InitParams.PIOforSCL.PortName, PIO_DeviceName[BACK_PIO]);

    STTBX_Print(("\nSTI2C_Init(%s)= ", I2C_DeviceName[I2C_BACK_BUS] ));
    Error = STI2C_Init(I2C_DeviceName[I2C_BACK_BUS], &STI2C_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);

    if (Error != ST_NO_ERROR)
    {
        return( TRUE );
    }
    else
    {
        STTBX_Print(("ST_NO_ERROR\n"));
    }

    /* Front I2C bus */
    STI2C_InitParams.BaseAddress       = (U32 *) SSC_1_BASE_ADDRESS;
    STI2C_InitParams.InterruptNumber   = SSC_1_INTERRUPT;
    STI2C_InitParams.InterruptLevel    = SSC_1_INTERRUPT_LEVEL;
    STI2C_InitParams.PIOforSDA.BitMask = PIO_FOR_SSC1_SDA;
    STI2C_InitParams.PIOforSCL.BitMask = PIO_FOR_SSC1_SCL;
    STI2C_InitParams.MasterSlave       = STI2C_MASTER;
    STI2C_InitParams.BaudRate          = STI2C_RATE_NORMAL;
    STI2C_InitParams.MaxHandles        = 5;
    STI2C_InitParams.ClockFrequency    = ST_ClockInfo.CommsBlock;
    STI2C_InitParams.DriverPartition   = DriverPartition;

    strcpy(STI2C_InitParams.PIOforSDA.PortName, PIO_DeviceName[FRONT_PIO]);
    strcpy(STI2C_InitParams.PIOforSCL.PortName, PIO_DeviceName[FRONT_PIO]);

    STTBX_Print(("STI2C_Init(%s)=", I2C_DeviceName[I2C_FRONT_BUS] ));
    Error = STI2C_Init(I2C_DeviceName[I2C_FRONT_BUS], &STI2C_InitParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    if (Error != ST_NO_ERROR)
    {
        return( TRUE );
    }
    else
    {
        STTBX_Print(("ST_NO_ERROR\n"));
    }

    return ( FALSE );
}

/*-------------------------------------------------------------------------
 * Function : I2C_Term
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/

BOOL I2C_Term(void)
{
    ST_ErrorCode_t     Error       = ST_NO_ERROR;
    ST_ErrorCode_t     StoredError = ST_NO_ERROR;
    STI2C_TermParams_t I2C_TermParams;

    I2C_TermParams.ForceTerminate = TRUE;
    Error = STI2C_Term(I2C_DeviceName[I2C_BACK_BUS], &I2C_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    if (Error != ST_NO_ERROR)
    {
        return ( TRUE );
    }

    Error = STI2C_Term(I2C_DeviceName[I2C_FRONT_BUS], &I2C_TermParams);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    if (Error != ST_NO_ERROR)
    {
        return ( TRUE );
    }
    else
    {
        return ( FALSE );
    }

}/* I2C_Term */

/*-------------------------------------------------------------------------
 * Function : EVT_Init
 *            Event Init function
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/

BOOL EVT_Init(void)
{
    ST_ErrorCode_t  Error       = ST_NO_ERROR;
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;

    Error = EVENT_Setup(EVENT_HANDLER_NAME);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    if (Error != ST_NO_ERROR)
    {
        return ( TRUE );
    }
    else
    {
        return ( FALSE );
    }

}/* EVT_Init */

/*-------------------------------------------------------------------------
 * Function : EVT_Term
 *            Event Term function
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/

BOOL EVT_Term(void)
{
    ST_ErrorCode_t  Error       = ST_NO_ERROR;
    ST_ErrorCode_t  StoredError = ST_NO_ERROR;

    Error = EVENT_Term(EVENT_HANDLER_NAME);
    stmergetst_ErrorReport(&StoredError,Error,ST_NO_ERROR);
    if (Error != ST_NO_ERROR)
    {
        return ( TRUE );
    }
    else
    {
        return ( FALSE );
    }

}/* EVT_Term */

/*-------------------------------------------------------------------------
 * Function : SelectFrequency
 * Input    : index into transponder list
 * Output   :
 * Return   : Error code
 * ----------------------------------------------------------------------*/

ST_ErrorCode_t SelectFrequency( int transponder )
{
    ST_ErrorCode_t          Error;
    STTUNER_TunerInfo_t     TUNERInfo;
    STTUNER_Scan_t          ScanParams;
    U32                     TunerFrequency;

    ScanParams  = TUNER_Scans[0];  /* defaults for all types */

    /* Set specific parameters */
    TunerFrequency          = LISTS_TransponderList[transponder].Frequency;
    ScanParams.Band         = LISTS_TransponderList[transponder].FrequencyBand;
#if defined(STTUNER_USE_SAT)
    ScanParams.LNBToneState = LISTS_TransponderList[transponder].LNBToneState;
    ScanParams.Polarization = LISTS_TransponderList[transponder].Polarization;
    ScanParams.FECRates     = LISTS_TransponderList[transponder].FECRates;
    ScanParams.SymbolRate   = LISTS_TransponderList[transponder].SymbolRate;
#endif
    ScanParams.Modulation   = LISTS_TransponderList[transponder].Modulation;
    ScanParams.AGC          = LISTS_TransponderList[transponder].AGC;

    Error = TUNER_SetFrequency(TunerFrequency, &ScanParams); /* blocks on semaphore in tuner.c */
    if ( Error == ST_NO_ERROR )
    {
        Error = STTUNER_GetTunerInfo(TUNER_Handle, &TUNERInfo);
        if ( Error == ST_NO_ERROR )
        {
            if ( TUNERInfo.Status != STTUNER_STATUS_LOCKED )
            {
                STTBX_Print(( "TUNERInfo.Status != STTUNER_STATUS_LOCKED\n" ));
                Error = (ST_ErrorCode_t) STTUNER_EV_SCAN_FAILED; /* failed */
            }
        }
    }

    return ( Error );
}

/*-------------------------------------------------------------------------
 * Function : TUNER_Init
 * Input    : None
 * Output   :
 * Return   : Error Code
 * ----------------------------------------------------------------------*/

BOOL TUNER_Init( void )
{
    int  index = 0;
    int  transponder = 0, channel = 0;
    static SERVICE_Mode_t SERVICE_Mode;
    ST_ErrorCode_t     Error       = ST_NO_ERROR;

#if defined(STMERGE_DTV_PACKET)
    SERVICE_Mode = SERVICE_MODE_DTV;
#else
    SERVICE_Mode = SERVICE_MODE_DVB;
#endif

    Error = TUNER_Setup(SERVICE_Mode);
    if (Error != ST_NO_ERROR)
    {
        return( TRUE );

    }
    else
    {
        while (LISTS_ChannelSelection[index] != ENDOFLIST)
        {
            channel     = LISTS_ChannelSelection[index];
            transponder = LISTS_ChannelList[channel].Transponder;

            STTBX_Print(( "Lock to transponder/channel #%d: %s\n", transponder, LISTS_TransponderList[transponder].InfoString ));
            Error = SelectFrequency( transponder );
            if (Error == ST_NO_ERROR)
            {
                Pid[0] = LISTS_ChannelList[channel].AudioPid;
                Pid[1] = LISTS_ChannelList[channel].VideoPid;
                Pid[2] = LISTS_ChannelList[channel].PcrPid;
                break; /* success */

            }
            index++;
        }
    }

    if (Error != ST_NO_ERROR)
    {
        return( TRUE );
    }
    else
    {
        return ( FALSE );
    }

}


#endif

/****************************************************************************
Name         : main

Description  :
****************************************************************************/

int main(void)
{
    BOOL RetErr = TRUE;

#if !defined(ST_OSLINUX)
#ifndef ST_OS21
    partition_init_heap (&TheInternalPartition, internal_block,
                         sizeof(internal_block));
    partition_init_heap (&TheSystemPartition,   system_block,
                         sizeof(system_block));
    partition_init_heap (&TheNcachePartition,   ncache_block,
                         sizeof(ncache_block));

    internal_block_noinit[0] = 0;
    system_block_noinit[0]   = 0;
    data_section[0]          = 0;

#else /* defined ST_OS21 */

    /* Create memory partitions */
    system_partition = partition_create_heap((U8*)external_block, sizeof(external_block));
#ifdef ARCHITECTURE_ST40
    NcachePartition = partition_create_heap((void *)(ST40_NOCACHE_NOTRANSLATE(ncache_block)),
                                             sizeof(ncache_block));
#else
    NcachePartition = partition_create_heap((void *)ncache_block,sizeof(ncache_block));
#endif
    DriverPartition = system_partition;
#endif /* end of ST_OS21 */
#endif /* end of OS_LINUX */

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

#if defined(ST_5525)

    /* Setup the memory map for 5525 */
    memory_map();

    printf("TS_FPGA_MODE_REG=0x%x\n",*((U8*)TS_FPGA_MODE_REG));
    printf("TS_EPLD_ENABLE_REG=0x%x\n",*((U8*)TS_EPLD_ENABLE_REG));

    *((U8*)TS_EPLD_ENABLE_REG) = 0x02; /* TSIN0 HIGH, LED should glow */
    *((U8*)TS_FPGA_MODE_REG)   = 0x02;

    printf("TS_FPGA_MODE_REG=0x%x\n",*((U8*)TS_FPGA_MODE_REG));
    printf("TS_EPLD_ENABLE_REG=0x%x\n",*((U8*)TS_EPLD_ENABLE_REG));

#endif


#if defined(ST_5100) || defined(ST_5301)

#if defined(RUNNING_FROM_STEM)
    /* This poke is needed for routing data from STEM to TSINs on mb390 */
    *((U32*)TS_EPLD_REG) = 0x000B000B;/* TS Mode 8 selection */
#else
    /* This poke is needed for routing data from NIM to TSINs on mb390 */
    *((U32*)TS_EPLD_REG) = 0x00000000;/* TS Mode 0 selection */
#endif
#endif

#if !defined(ST_OSLINUX)
#if defined(ST_7100) || defined(ST_7710) || defined(ST_7109)
    *((U32*)TS_EPLD_REG) = 0x00000000;
#endif
#endif
/*  Do we need to do some EPLD settings for 5525 */

    /* Initialize all drivers */
#if !defined(ST_OSLINUX)
    if (BOOT_Init() || TBX_Init() )
    {
        printf("Problem initializing drivers\n");
        return 0;
    }
#endif
    RetErr = TST_Init();
    if (RetErr == TRUE)
    {
        STTBX_Print((" TST_Init \n"));
    }

    /* Now start the tests */
    STTBX_Print(("********************************************************************\n"));
    STTBX_Print(("                STMERGE Test Harness                    \n"));
    STTBX_Print(("\nLast built %s %s\n", __TIME__, __DATE__));
    STTBX_Print(("Driver Revision: %s\n", STMERGE_GetRevision()));
    STTBX_Print(("Test Harness Revision: %s\n", stmergetst_Rev));
    STTBX_Print(("********************************************************************\n"));

    RetErr = InitCommands();
    if (RetErr == TRUE)
    {
        STTBX_Print(("InitCommands FAILED\n"));
    }

#if defined(STMERGE_USE_TUNER)

    /* Get clock info structure - for global use  */
    (void)ST_GetClockInfo(&ST_ClockInfo);

    RetErr = PIO_Init();
    if (RetErr == TRUE)
    {
        STTBX_Print(("PIO Setup FAILED\n"));
    }

    RetErr = I2C_Init();
    if (RetErr == TRUE)
    {
        STTBX_Print(("I2C Setup FAILED\n"));
    }
    RetErr = EVT_Init();
    if (RetErr == TRUE)
    {
        STTBX_Print(("EVT Setup FAILED\n"));
    }

    RetErr = TUNER_Init();
    if (RetErr == TRUE)
    {
        STTBX_Print(("TUNER Setup FAILED\n"));
    }

#endif /* end of STMERGE_USE_TUNER */

    if (RetErr == FALSE)
    {
        RetErr = STTST_Start();
        if (RetErr == TRUE)
        {
            STTBX_Print(("STTST_Start FAILED\n"));
        }
    }

    /* Terminating all the initialised devices */

#ifdef STMERGE_USE_TUNER

    /* Term I2C */
    STTBX_Print(("Terminating I2C\n"));
    RetErr = I2C_Term();
    if (RetErr == TRUE)
    {
        STTBX_Print(("I2C Term FAILED\n"));
    }

    /* Term Event Handler */
    STTBX_Print(("Terminating EVENT Handler\n"));
    RetErr = EVT_Term();
    if (RetErr == TRUE)
    {
        STTBX_Print(("EVT Term FAILED\n"));
    }

    /* Terminating PIO */
    STTBX_Print(("Terminating PIO\n"));
    RetErr = PIO_Term();
    if (RetErr == TRUE)
    {
        STTBX_Print(("PIO Term FAILED\n"));
    }

    STTBX_Print(("Terminating TUNER\n"));
    RetErr = TUNER_Quit();
    if (RetErr == TRUE)
    {
        STTBX_Print(("TUNER Term FAILED\n"));
    }

#endif /* end of STMERGE_USE_TUNER */

    return 0;

}

#ifdef ST_5525
void memory_map(void)
 {
	int result;
	char problems[255]="";
	unsigned char errors=0;

    printf(" Testtool compiled on %s at %s\n", __DATE__, __TIME__);
    printf("\n Memory mapping...");

/* LMI Configuration Registers */
 result=(int) bsp_mmu_memory_map((void*)0x88000000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x88000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," LMI Configuration Registers\n");}

/* LMI : The 64MB are automatically mapped at Testtool Start-Up as Cached and Locked */
/* Here, We force the last 60MB of LMI to be Uncached (required for FDMA software)*/
 result=(int) bsp_mmu_memory_map((void*)0x80400000,
                                        0x03C00000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_LOCKED|
                                        MAP_OVERRIDE|
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x80400000);
if (result==BSP_FAILURE) {errors++;strcat(problems," Local Memory Interface (LMI)\n");}

/* Peripherals Registers */
 result=(int) bsp_mmu_memory_map((void*)0x18000000,
                                        0x01000000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_LOCKED|
                                        MAP_OVERRIDE|
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x18000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," Peripherals Registers\n");}

/* FMI Configuration Registers -Uncached */
 result=(int) bsp_mmu_memory_map((void*)0x0FFFF000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x0FFFF000);
if (result==BSP_FAILURE) {errors++;strcat(problems," FMI Configuration Registers\n");}

/* FMI Uncached */
/* FMI - Bank 0 -> Flash : 8MB */
 result=(int) bsp_mmu_memory_map((void*)0x50000000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_LOCKED|
                                        MAP_OVERRIDE|
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x00000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," First 4MB of Flash on Bank 0\n");}

 result=(int) bsp_mmu_memory_map((void*)0x50400000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_LOCKED|
                                        MAP_OVERRIDE|
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x00400000);
if (result==BSP_FAILURE) {errors++;strcat(problems," Second 4MB of Flash on Bank 0\n");}

/* FMI - Bank 1 -> Flash : 8MB*/
 result=(int) bsp_mmu_memory_map((void*)0x50800000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_LOCKED|
                                        MAP_OVERRIDE|
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x08000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," First 4MB of Flash on Bank 1\n");}

 result=(int) bsp_mmu_memory_map((void*)0x50C00000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_LOCKED|
                                        MAP_OVERRIDE|
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x08400000);
if (result==BSP_FAILURE) {errors++;strcat(problems," Second 4MB of Flash on Bank 1\n");}

/* FMI - Bank 1 -> LAN1 */
 result=(int) bsp_mmu_memory_map((void*)0x09000000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x09000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," LAN1 on Bank 1\n");}

/* FMI - Bank 1 -> LAN2 */
 result=(int) bsp_mmu_memory_map((void*)0x09400000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x09400000);
if (result==BSP_FAILURE) {errors++;strcat(problems," LAN2 on Bank 1\n");}

/* FMI - Bank 1 -> EPLD */
 result=(int) bsp_mmu_memory_map((void*)0x09800000,
                                        0x00800000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x09800000);
if (result==BSP_FAILURE) {errors++;strcat(problems," EPLD Registers on Bank 1\n");}

 /* FMI - Bank 2 -> STEM0 */
 result=(int) bsp_mmu_memory_map((void*)0x0A000000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x0A000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," STEM0 on Bank 2\n");}

/* FMI - Bank 3 -> STEM1 */
 result=(int) bsp_mmu_memory_map((void*)0x0D000000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x0D000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," STEM1 on Bank 3\n");}

/* FMI - Bank 3 -> FPGAProg */
 result=(int) bsp_mmu_memory_map((void*)0x0F000000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x0F000000);
if (result==BSP_FAILURE) {errors++;strcat(problems," FPGAProg on Bank 3\n");}

/* FMI - Bank 3 -> FPGASRAM */
 result=(int) bsp_mmu_memory_map((void*)0x0F400000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x0F400000);
if (result==BSP_FAILURE) {errors++;strcat(problems," FPGASRAM on Bank 3\n");}

/* FMI - Bank 3 -> ATAPI */
 result=(int) bsp_mmu_memory_map((void*)0x0F800000,
                                        0x00400000,
                                        POLICY_UNCACHEABLE|
                                        PROT_USER_EXECUTE|
                                        PROT_USER_WRITE|
                                        PROT_USER_READ|
                                        PROT_SUPERVISOR_EXECUTE|
                                        PROT_SUPERVISOR_WRITE|
                                        PROT_SUPERVISOR_READ,
                                        MAP_SPARE_RESOURCES,
                                 (void*)0x0F800000);
if (result==BSP_FAILURE) {errors++;strcat(problems," ATAPI on Bank 3\n");}


/* Display info */

    if (errors)
    {
	printf("Failed!\n Failure in memory mapping subsytem ! Mapping error has occured %u time(s).\n",errors);
	printf("\n Modules where mapping memory errors are found : \n%s\n",problems);
    }
    else printf("OK!\n");

#ifdef ENABLE_MEM_DEBUG
	display_memory_map();
#endif

    printf(" Using Kernel Version %s\n\n", kernel_version());


}
#endif
