/*****************************************************************************
File Name   : stfdma.c

Description : STFDMA API driver. Driver for FDMA block.

Copyright (C) 2003 STMicroelectronics

*****************************************************************************/
/* Includes --------------------------------------------------------------- */
#define __OS_20TO21_MAP_H /* prevent os_20to21_map.h from being included*/

/*#define STTBX_PRINT 1*/

#include "local.h"
#include "fdmareq.h"                    /* FDMA request signal table */

#ifdef STFDMA_SIM
#include "fdmasim.h"
#endif

/* Private  Constants ------------------------------------------------------ */

/* Callback task constants */
#ifndef STFDMA_CALLBACK_TASK_STACK_SIZE
#if defined(ARCHITECTURE_ST40)

#if defined (ST_OSLINUX)
#define STFDMA_CALLBACK_TASK_STACK_SIZE   (1024 + (16 * 1024))
#else
#define STFDMA_CALLBACK_TASK_STACK_SIZE   (OS21_DEF_MIN_STACK_SIZE + 1024)
#endif

#elif defined(ARCHITECTURE_ST200)
#define STFDMA_CALLBACK_TASK_STACK_SIZE   (1024 + (16 * 1024))
#else
#define STFDMA_CALLBACK_TASK_STACK_SIZE   2*1024
#endif
#endif

#ifndef STFDMA_CALLBACK_TASK_PRIORITY
#if defined (ST_OSLINUX)
#define STFDMA_CALLBACK_TASK_PRIORITY     99
#else
#define STFDMA_CALLBACK_TASK_PRIORITY     MAX_USER_PRIORITY
#endif
#endif

/* Indicator for free channels */
#define NO_FREE_CHANNELS           0xff

/* Indicator for no active transfer */
#define NO_TRANSFER                0x00

/* this might be scalable depending on NumberCallbackTasks */
#define MESSAGE_ARRAY_SIZE         20

/* Channel index extraction mask */
#define TRANSFER_ID_CHANNEL_MASK   0x0000ffff

/* Device index extraction mask */
#define DEVICE_CHANNEL_MASK        0x00ff0000

/* Initialisation progress array size */
#define CREATION_LOG_SIZE          8

/* Semaphore timeout error value */
#define SEMAPHORE_TIMEOUT          -1

/* Offset for thye base address of the SPDIF Context data */

#if defined (ST_7100) || defined(ST_7109) || defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
#define SPDIF_CONTEXT_OFFSET 0X9380
#else
#define SPDIF_CONTEXT_OFFSET 0X45B0
#endif

/* -- Hardware register offsets --
 * The following offsets are taken from the FDMA Interface Specification
 * ADCS #7446886
*/

/*Extern Variables ------------------------------------------------------- */
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
extern unsigned int STFDMA_MaxChannels;
extern unsigned int STFDMA_Mask;
#endif

/* Private Variables ------------------------------------------------------- */

/* First initialisation flag. Set to FALSE */
static BOOL InitAlready = FALSE;

extern fdmareq_RequestConfig_t fdmareq_RequestLines[];

/* Initialisation/termination protection */
static semaphore_t* InitTermSemap;

/* Message queue control block */
/* Global pointer for control block reference */
ControlBlock_t *stfdma_ControlBlock_p[STFDMA_MAX] = {NULL};

#if defined (STFDMA_DEBUG_SUPPORT)
STFDMA_Status_t *stfdma_Status[STFDMA_MAX];
#endif

#if defined (STFDMA_MPX_SUPPORT)
#if defined (STFDMA_MPX_USE_EXT_IRQ3)
U32		* ILCMappedBaseAddr;
U32    *  CFGMappedBaseAddr;
#else
STPIO_Handle_t     PIOHandle;
static BOOL PIOOpened = FALSE;
#endif
#endif

/* Driver revision number */
#ifdef STAPIREF_COMPAT
static const ST_Revision_t Revision = "STFDMA-REL_3.0.0";
#else
static const ST_Revision_t Revision = "STFDMA-FAE-REL_SNAP_2008.01.251";
#endif /* STAPIREF_COMPAT */

/******************************************************************
 *
 * Define the size of each pool.
 *
 * This table is indexed via 'PoolMapping' by the members of 'STFDMA_Pool_t'.
 * An unused pool should be given with a size of 0 (zero).
 */
static int PoolConfiguration[STFDMA_MAX][MAX_POOLS] =
{
	{
#if defined(ST_5517)
	    6, /* DEFAULT */
#elif defined (ST_5188)
	    1, /* PES POOL */
	    1, /* SPDIF POOL */
	    2, /* HIGH BANDWIDTH POOL */
	    1, /* FEI POOL */
	    3 /* DEFAULT POOL */
#elif defined (ST_5162) 
	    1, /* PES POOL */
	    1, /* SPDIF POOL */
	    3, /* HIGH BANDWIDTH POOL */
	   11, /* DEFAULT POOL */
#elif defined(ST_5525)
	    2, /* PES POOL */
	    0, /* SPDIF POOL (NOT USED) */
	    3, /* HIGH BANDWIDTH POOL */
	   11, /* DEFAULT POOL */
#elif defined(ST_498)
	    0, /* PES POOL (NOT USED)*/
	    0, /* SPDIF POOL (NOT USED) */
	    0, /* HIGH BANDWIDTH POOL  */
	   14, /* DEFAULT POOL (keep last 2 channels for MCHI)*/
#elif defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
	    0, /* PES POOL */
	    1, /* SPDIF POOL */
	    1, /* HIGH BANDWIDTH POOL */
	   14, /* DEFAULT POOL */
#else
	    1, /* PES POOL */
	    1, /* SPDIF POOL */
	    3, /* HIGH BANDWIDTH POOL */
	   11, /* DEFAULT POOL */
#endif
	}

#if defined (ST_7200) || defined(ST_7111)  || defined (ST_5525) || defined (ST_7105)
	,{
#if defined(ST_5525)
	    0, /* PES POOL (NOT USED) */
	    1, /* SPDIF POOL */
	    8, /* HIGH BANDWIDTH POOL */
	    7, /* DEFAULT POOL */
#elif defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
	    2, /* PES POOL */
	    0, /* SPDIF POOL */
	    0, /* HIGH BANDWIDTH POOL */
	   14, /* DEFAULT POOL */
#endif
	}
#endif

/*This will be a second instance on 7109 controlling 498 FDMA used only by STPOD*/
#if defined (STFDMA_MPX_SUPPORT)
		/*filler for STFDMA_2*/
	,{
	    0, /* PES POOL */
	    0, /* SPDIF POOL */
	    0, /* HIGH BANDWIDTH POOL (not to be used by STPOD)*/
	    0, /* DEFAULT POOL (last 2 channles only for STPOD)*/
	}
	/*Pool conf for STFDMA_MPX*/
	,{
	    0, /* PES POOL */
	    0, /* SPDIF POOL */
	    14, /* HIGH BANDWIDTH POOL (not to be used by STPOD)*/
	    2, /* DEFAULT POOL (last 2 channles only for STPOD)*/
	}
#endif
};

/******************************************************************
 *
 * Map the STFDMA_Pool_t enumeration to the pools.
 *
 * This table maps the members of 'STFDMA_Pool_t' to 'PoolConfiguration'
 * which gives the size of each pool and 'FreeChannelIndex' in
 * 'stfdma_ControlBlock_p' which give the next free entry in to pool.
 * The number in the table is the index into 'FreeChannelIndex' for
 * the 'STFDMA_Pool_t' entry. e.g.
 *
 *   NextFree = stfdma_ControlBlock_p->FreeChannelIndex[PoolMapping[STFDMA_PES_POOL]];
 */
static int PoolMapping[] =
{
#if defined(ST_5517)

    0, /* STFDMA_DEVICE_DEFAULT_POOL */
    0, /* STFDMA_DEVICE_PES_POOL */
    0, /* STFDMA_DEVICE_SPDIF_POOL */
    0, /* STFDMA_DEVICE_HIGH_BANDWIDTH_POOL */

#else

#if defined (ST_5188)
    4, /* STFDMA_DEVICE_DEFAULT_POOL */
#else
    3, /* STFDMA_DEVICE_DEFAULT_POOL */
#endif

    0, /* STFDMA_DEVICE_PES_POOL */
    1, /* STFDMA_DEVICE_SPDIF_POOL */
    2, /* STFDMA_DEVICE_HIGH_BANDWIDTH_POOL */

#if defined (ST_5188)
    3  /* STFDMA_DEVICE_FEI_POOL */
#endif

#endif
};

/******************************************************************/
#define MAX_POOL (sizeof(PoolMapping)/sizeof(*PoolMapping))

/* Worker macros ----------------------------------------------------------------*/

/* Data structure access macros */
#define ENTER_CRITICAL_SECTION(Device) (STOS_SemaphoreWait(stfdma_ControlBlock_p[Device]->pAccessSemaphore))
#define LEAVE_CRITICAL_SECTION(Device) (STOS_SemaphoreSignal(stfdma_ControlBlock_p[Device]->pAccessSemaphore))

#define IS_TERMINATING(Device)              (stfdma_ControlBlock_p[Device]->Terminate == TRUE)
#define SET_TERMINATING(Device)             (stfdma_ControlBlock_p[Device]->Terminate = TRUE)
#define CLEAR_TERMINATING(Device)           (stfdma_ControlBlock_p[Device]->Terminate = FALSE)

#define INC_TRANSFER_TOTAL(Device)          (stfdma_ControlBlock_p[Device]->TransferTotal++)
#define GET_TRANSFER_TOTAL(Device)          (stfdma_ControlBlock_p[Device]->TransferTotal)

#define GET_TRANSFER_ID(Channel, Device)    (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].TransferId)
#define SET_TRANSFER_ID(Channel, Id, Device)(stfdma_ControlBlock_p[Device]->ChannelTable[Channel].TransferId = Id)
#define CLEAR_TRANSFER_ID(Channel, Device)  (SET_TRANSFER_ID(Channel, NO_TRANSFER, Device))

#define IS_LOCKED(Channel, Device)          (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Locked)
#define SET_LOCKED(Channel, Device)         (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Locked = TRUE)
#define CLEAR_LOCKED(Channel, Device)       (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Locked = FALSE)

#define IS_PAUSED(Channel, Device)          (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Paused)
#define SET_PAUSED(Channel, Device)         (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Paused = TRUE)
#define CLEAR_PAUSED(Channel, Device)       (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].Paused = FALSE)

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
#define GET_GENERIC_DMA(Channel, Device)    (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].GenericDMA)
#define SET_GENERIC_DMA(Channel, Device)    (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].GenericDMA = TRUE)
#define CLEAR_GENERIC_DMA(Channel, Device)  (stfdma_ControlBlock_p[Device]->ChannelTable[Channel].GenericDMA = FALSE)
#endif

/* Private Function Prototypes --------------------------------------------- */
static U32 GetNextFreeChannel(int PoolId, STFDMA_Block_t DeviceNo);

static void GiveUpChannel(U32 Channel, STFDMA_Block_t DeviceNo);

static U32 CreateTransferId(U32 Channel, U8 DeviceNo);

static BOOL IsTransferInProgress(U32 Device);

#ifndef STFDMA_NO_PARAMETER_CHECK
static BOOL IsOutsideRange(U32 ValuePassed, U32 LowerBound, U32 UpperBound);
#endif

static U32 GetChannelFromTransferId( U32 TransferId );
static U32 GetDeviceFromTransferId( U32 TransferId );

static BOOL IsChannelBusy( U32 Channel, STFDMA_Block_t DeviceNo );

static BOOL InitMessages(U32 Size, STFDMA_Block_t DeviceNo);

static void TermMessages(STFDMA_Block_t DeviceNo);

static void SendExitMessage(STFDMA_Block_t DeviceNo);

static int  ReceiveMessage(MessageElement_t * Message, STFDMA_Block_t DeviceNo);

static void CallbackManager(void* pData);

#if defined (STFDMA_MPX_SUPPORT)
#if defined (STFDMA_MPX_USE_EXT_IRQ3)
ST_ErrorCode_t stfdma_RouteInterrupt(void);
#else
ST_ErrorCode_t stfdma_RouteInterrupt(const ST_DeviceName_t DeviceName, U32 PIONumber, U32  PIOBit);
#endif
#endif

/* Function ---------------------------------------------------- */

/****************************************************************************
Name         : STFDMA_Init
Description  : Initialises the FDMA device at the given hardware base address.
               Loads the required FDMA config data from config table,
               initialises and enables the FDMA block.
Parameters   : DeviceName     - Name of device to intialise
               InitParams_p   - Pointer to initialisation parameters
Return Value : ST_NO_ERROR
               ST_ERROR_ALREADY_INITIALISED
               ST_ERROR_BAD_PARAMETER
               ST_ERROR_DEVICE_NOT_SUPPORTED
               ST_ERROR_NO_MEMORY
               ST_ERROR_INTERRUPT_INSTALL
****************************************************************************/
ST_ErrorCode_t STFDMA_Init(const ST_DeviceName_t DeviceName,
                           const STFDMA_InitParams_t *InitParams_p)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 i = 0;
    U32 j = 0;
    U32 k = 0;
    STFDMA_Block_t DeviceNumber = 0;

    BOOL CreationLog[CREATION_LOG_SIZE] = {FALSE};
    STOS_InterruptParam_t  InterruptParam_p;

     /* Intialisation progress flags for use with CreationLog array */
    enum
    {
        CONTROL_BLOCK = 0,
        ACCESS_SEMAPHORE,
        BLOCKING_SEMAPHORE,
        INTERRUPT_INSTALL,
        MESSAGE_SYSTEM,
        STFDMA_CALLBACK_TASK_TABLE,
        STFDMA_CALLBACK_TASK_INIT,
        CONTEXT_BLOCK
    };

#if defined (ST_498)
	U32 *  SharedInterruptStatus = (U32 *)STFDMA_MPX_ECM_SHARED_INT_STATUS;
#endif
#if defined (STFDMA_MPX_SUPPORT)
	U32 *  SharedInterruptStatus = (U32 *)STFDMA_MPX_ESTB_SHARED_INT_STATUS;
#endif    

    STTBX_Print(("Entering STFDMA_Init\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    /* Check parameters  */
    if ((InitParams_p == NULL) ||
        (DeviceName == NULL) ||
        (strlen(DeviceName) > ST_MAX_DEVICE_NAME) ||
        (DeviceName[0] == '\0'))
    {
        STTBX_Print(("ERROR: One or more input parameters invalid\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check init parameters */
    if ((InitParams_p->DriverPartition_p == NULL) ||
        (InitParams_p->NCachePartition_p == NULL) ||
        (InitParams_p->BaseAddress_p == NULL) ||
        (InitParams_p->ClockTicksPerSecond == 0))
    {
        STTBX_Print(("ERROR: One or more init parameters invalid\n"));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check init parameters */
    if (InitParams_p->FDMABlock >= STFDMA_MAX)
    {
        STTBX_Print(("ERROR: One or more init parameters invalid\n"));
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }

#if defined(ST_5517)
    /* Check device type */
    if (IsOutsideRange(InitParams_p->DeviceType,
                       STFDMA_DEVICE_FDMA_1, STFDMA_DEVICE_FDMA_1))
    {
        STTBX_Print(("ERROR: Invalid device type supplied\n"));
        return ST_ERROR_BAD_PARAMETER;
    }
#else
    /* Check device type */
    if (IsOutsideRange(InitParams_p->DeviceType,
                       STFDMA_DEVICE_FDMA_2, STFDMA_DEVICE_FDMA_2))
    {
        STTBX_Print(("ERROR: Invalid device type supplied\n"));
        return ST_ERROR_BAD_PARAMETER;
    }
#endif
#endif

    DeviceNumber = InitParams_p->FDMABlock;

#ifndef STFDMA_NO_USAGE_CHECK
    /* Already intilialised? */
    if (IS_INITIALISED(DeviceNumber) == TRUE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer not NULL\n"));
        return (ST_ERROR_ALREADY_INITIALIZED);
    }
#endif

    /* Check init/term protection semap */
    STTBX_Print(("Create InitTerm protection\n"));

    STOS_TaskLock();

    if (InitAlready == FALSE)
    {
        InitAlready = TRUE;
        /* Initialise so first STOS_SemaphoreWait gets semaphore */
        InitTermSemap =  STOS_SemaphoreCreateFifo(InitParams_p->DriverPartition_p, 1);
    }

    STOS_TaskUnlock();

    /* Protect initialisation sequence */
    STOS_SemaphoreWait(InitTermSemap);

    /* Control block creation */
    if (ErrorCode == ST_NO_ERROR)
    {
        STTBX_Print(("Creating and initialising ControlBlock and associated memory\n"));
        stfdma_ControlBlock_p[DeviceNumber] = (ControlBlock_t *)STOS_MemoryAllocate(InitParams_p->DriverPartition_p, sizeof(ControlBlock_t));

#if defined (STFDMA_DEBUG_SUPPORT)
        stfdma_Status[DeviceNumber] = (STFDMA_Status_t *)STOS_MemoryAllocate(InitParams_p->DriverPartition_p, sizeof(STFDMA_Status_t));
        if (stfdma_Status[DeviceNumber] == NULL)
            STTBX_Print(("ERROR: Status memory alloc failed\n"));
#endif

        CreationLog[CONTROL_BLOCK] = TRUE;

        if (stfdma_ControlBlock_p[DeviceNumber] == NULL)
        {
            STTBX_Print(("ERROR: ControlBlock memory alloc failed\n"));
            CreationLog[CONTROL_BLOCK] = FALSE;
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
        else
        {
#if defined (STFDMA_MPX_SUPPORT)
            stfdma_ControlBlock_p[DeviceNumber]->DeviceType = InitParams_p->DeviceType;
#endif
            /* Control block initialisation */
            strcpy(stfdma_ControlBlock_p[DeviceNumber]->DeviceName, DeviceName);

            /* Initialise so first STOS_SemaphoreWait gets semaphore */
            stfdma_ControlBlock_p[DeviceNumber]->pAccessSemaphore = STOS_SemaphoreCreateFifo(InitParams_p->DriverPartition_p, 1);

            CreationLog[ACCESS_SEMAPHORE]       = TRUE;
            stfdma_ControlBlock_p[DeviceNumber]->DeviceNumber        = DeviceNumber;
            stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p       = (U32*)InitParams_p->BaseAddress_p;
            stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p   = InitParams_p->DriverPartition_p;
            stfdma_ControlBlock_p[DeviceNumber]->NCachePartition_p   = InitParams_p->NCachePartition_p;
            stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks = InitParams_p->NumberCallbackTasks;
            stfdma_ControlBlock_p[DeviceNumber]->InterruptLevel      = InitParams_p->InterruptLevel;
            stfdma_ControlBlock_p[DeviceNumber]->InterruptNumber     = InitParams_p->InterruptNumber;
            stfdma_ControlBlock_p[DeviceNumber]->ClockTicksPerSecond = InitParams_p->ClockTicksPerSecond;
            stfdma_ControlBlock_p[DeviceNumber]->Terminate           = FALSE;
            stfdma_ControlBlock_p[DeviceNumber]->TransferTotal       = 1;  /*Not 0 since NO_TRANSFER indicator is 0 */
            STTBX_Print(("FDMA Base address before translation 0X%08X\n", (U32)(stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p)));            
            stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p = (U32 *)STOS_MapRegisters((void*)InitParams_p->BaseAddress_p, (64 * 1024),"STFDMA");
            if(NULL == stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p)
            {
            	STTBX_Print(("ERROR: STOS_MapRegisters() failed\n"));
            	ErrorCode = STFDMA_ERROR_IO_REMAP_FAILED;
            }
            else
            {
            	STTBX_Print(("FDMA Base address after translation 0X%08X\n", (U32)(stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p)));
            }	

            /* Initialise the channel pools */
            for (j = 0, i = 0; (j < MAX_POOLS) && (i < NUM_CHANNELS); j++)
            {
                if ((0 < PoolConfiguration[DeviceNumber][j]) && (i < NUM_CHANNELS))
                {
                    /* We have some channels tp put into this pool */
                    int Size = PoolConfiguration[DeviceNumber][j];

                    stfdma_ControlBlock_p[DeviceNumber]->FreeChannelIndex[j] = i;

                    for (k = 0; (k < Size) && (i < NUM_CHANNELS); k++, i++)
                    {
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].Pool                = j;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].TransferId          = NO_TRANSFER;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].Locked              = FALSE;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].PacedSignalUsed = STFDMA_REQUEST_SIGNAL_NONE;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].Paused              = FALSE;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].Abort               = FALSE;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].Error               = FALSE;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].Blocking            = FALSE;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].BlockingReason      = 0;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].CallbackFunction_p  = NULL;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].ApplicationData_p   = NULL;
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].NextFreeChannel     = i+1;
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
                        /* Clear the genreric DMA flag */
			            stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].GenericDMA = FALSE;
#endif
                        stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].pBlockingSemaphore = STOS_SemaphoreCreateFifoTimeOut(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, 0);
#if defined (STFDMA_DEBUG_SUPPORT)
                        if(stfdma_Status[DeviceNumber] != NULL)
                        {
                            stfdma_Status[DeviceNumber]->Channel[i].InterruptsTotal = 0;
                            stfdma_Status[DeviceNumber]->Channel[i].InterruptsBlocking = 0;
                            stfdma_Status[DeviceNumber]->Channel[i].InterruptsNonBlocking = 0;
                            stfdma_Status[DeviceNumber]->Channel[i].InterruptsMissed = 0;
                            stfdma_Status[DeviceNumber]->Channel[i].InterruptBlockingSemaphoreReceived = 0;
                            stfdma_Status[DeviceNumber]->Channel[i].Callbacks = 0;
                        }
#endif
                        CreationLog[BLOCKING_SEMAPHORE] = TRUE;
                    }

                    /* The last free channel in a pool cannot reference the next channel */
                    stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i-1].NextFreeChannel = NO_FREE_CHANNELS;
                }
                else
                {
                    /* Empty pool */
                    stfdma_ControlBlock_p[DeviceNumber]->FreeChannelIndex[j] = NO_FREE_CHANNELS;
                }
            }
        }
    }
    /* END: Control block creation */

#if defined (ST_498)
		*SharedInterruptStatus = 0;
		stfdma_SetPIOBitConfiguration(MPX_ECM_PIOx_BITy);
#endif		
#if defined (STFDMA_MPX_SUPPORT)
		if(InitParams_p->FDMABlock == STFDMA_MPX)
		{
			*SharedInterruptStatus = 0;
		    stfdma_ResetPIOBit(MPX_ECM_PIOx_BITy);
#if defined (STFDMA_MPX_USE_EXT_IRQ3)		
			ErrorCode = stfdma_RouteInterrupt();
#endif
			if( ErrorCode != ST_NO_ERROR) 
			{
            CreationLog[CONTROL_BLOCK] = FALSE;
            ErrorCode = ST_ERROR_BAD_PARAMETER;			
			}
		}
#endif

    /* Install interrupt handler */
    if (ErrorCode == ST_NO_ERROR)
    {
#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
		/*No FDMA interrupt handler to be used when PIO interrupt is used for STFDMA_MPX*/
		if(InitParams_p->FDMABlock != STFDMA_MPX)
		{
#endif
        STTBX_Print(("Installing interrupt handler\n"));

        InterruptParam_p.InterruptType = STOS_FAST_IRQ | STOS_SHARED_IRQ;
        if(STOS_SUCCESS != STOS_InterruptInstallConfigurable(InitParams_p->InterruptNumber,
                                                 InitParams_p->InterruptLevel,
                                                 stfdma_InterruptHandler,
                                                 &InterruptParam_p,
                                                 "STFDMA_InterruptHandler",
                                                 (void *)&(stfdma_ControlBlock_p[DeviceNumber]->DeviceNumber)))

        {
            /* Installation failed */
            STTBX_Print(("ERROR: Could not install interrupt handler\n"));
            CreationLog[INTERRUPT_INSTALL] = FALSE;
            ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
        }
        else
            if(STOS_SUCCESS != STOS_InterruptEnable(InitParams_p->InterruptNumber,
                                                    InitParams_p->InterruptLevel))
            {
                /* Enabling failed */
                STTBX_Print(("ERROR: Could not enable interrupt handler\n"));
                CreationLog[INTERRUPT_INSTALL] = FALSE;
                ErrorCode = ST_ERROR_INTERRUPT_INSTALL;
            }
    }

    /* END: Load interrupt handler */
    STTBX_Print(("Loaded interrupt handler\n"));
#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
	}
#endif

#if defined (STFDMA_MPX_SUPPORT)
    if(InitParams_p->FDMABlock != STFDMA_MPX)
    {
#endif
    /* Context is supported on first FDMA */
#if defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
    /* Adding STFDMA_1 as SPDIF which also uses a context is possible only on STFDMA_1 */
    if( (ErrorCode == ST_NO_ERROR) && (DeviceNumber == STFDMA_2 || DeviceNumber == STFDMA_1) )
#else
    if( (ErrorCode == ST_NO_ERROR) && (DeviceNumber == STFDMA_1) )
#endif
    {
        /* Context setup */
        stfdma_InitContexts(stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p, stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, DeviceNumber);
        CreationLog[CONTEXT_BLOCK] = TRUE;
        /* END: Context setup */
    }
#if defined (STFDMA_MPX_SUPPORT)
    }
#endif

    /* Context setting and Message queue creation */

    /* Create message queue and associated semaps */
    if ((ErrorCode == ST_NO_ERROR) && (stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks != 0))
    {
        if (InitMessages(MESSAGE_ARRAY_SIZE, DeviceNumber) == TRUE)
        {
            CreationLog[MESSAGE_SYSTEM] = TRUE;
        }
        else
        {
            STTBX_Print(("ERROR: Message queue intiliatisation failed\n"));
            CreationLog[MESSAGE_SYSTEM] = FALSE;
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
    }
    /* END: Create message queue and associated semaps */

    /* END: Context setting and Message queue creation */

    /* Create table of task Id values */
    if ((ErrorCode == ST_NO_ERROR) && (stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks != 0))
    {
        stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p =
            (TaskData_t *)STOS_MemoryAllocate(InitParams_p->DriverPartition_p,
                                              (sizeof (TaskData_t) * stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks));
        CreationLog[STFDMA_CALLBACK_TASK_TABLE] = TRUE;

        if (stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p == NULL)
        {
            /* Allocation failed, free up used memory and exit */
            STTBX_Print(("ERROR: CallbackTaskTable  memory alloc failed \n"));
            CreationLog[STFDMA_CALLBACK_TASK_TABLE] = FALSE;
            STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p);
            ErrorCode = ST_ERROR_NO_MEMORY;
        }
    }
    /* END: Create table of task Id values */

    /* Launch tasks and store task id */
    if ((ErrorCode == ST_NO_ERROR) && (stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks != 0))
    {
        CreationLog[STFDMA_CALLBACK_TASK_INIT] = TRUE;
        for (i = 0; i < stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks; i++)
        {
            STTBX_Print(("Launching callback task %d \n",i));
            ErrorCode = STOS_TaskCreate((void(*)(void *))CallbackManager,
                                        (void *)DeviceNumber,
                                        stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p,
                                        STFDMA_CALLBACK_TASK_STACK_SIZE,
                                        &(stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[i].TaskStack_p),
                                        stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p,
                                        &(stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[i].pTask),
                                        &(stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[i].TaskDesc),
                                        STFDMA_CALLBACK_TASK_PRIORITY,
                                        "STFDMA_ClbckMgr",
                                        (task_flags_t)0);
            if ( ErrorCode != ST_NO_ERROR )
            {

                U32     j;
                task_t* Task_p;

                STTBX_Print(("ERROR: Initialisation of Task %d failed \n", i));
                SendExitMessage(DeviceNumber);

                /* Delete all tasks */
                for (j = 0; j < i ; j++)
                {
                    Task_p = stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[j].pTask;
                    STOS_TaskWait(&Task_p, TIMEOUT_INFINITY);
                    STOS_TaskDelete(Task_p, stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p,
                                    stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[i].TaskStack_p,
                                    stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p);
                }
                ErrorCode = ST_ERROR_NO_MEMORY;
                CreationLog[STFDMA_CALLBACK_TASK_INIT] = FALSE;
                break;
            }

            STTBX_Print(("Launched callback task %d \n",i));
        }
    }
    /* END: Launch tasks and store task id */

    /* cleanup if error occurred */
    if (ErrorCode != ST_NO_ERROR)
    {
        int j;
        task_t *Task_p;

        for(j=CREATION_LOG_SIZE-1; j>-1; j--)
        {
            if( CreationLog[j] == FALSE)
            {
                continue;
            }
            switch( j )
            {
                case CONTROL_BLOCK:
#if defined (STFDMA_DEBUG_SUPPORT)
                    if(stfdma_Status[DeviceNumber] != NULL)
                    {
                        STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, stfdma_Status[DeviceNumber]);
                        stfdma_Status[DeviceNumber] = NULL;
                    }
#endif
                    STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNumber]);
                    stfdma_ControlBlock_p[DeviceNumber] = NULL;
                    break;

                case ACCESS_SEMAPHORE:
                    STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNumber]->pAccessSemaphore);
                    break;

                case BLOCKING_SEMAPHORE:
                    for (i=0; i < NUM_CHANNELS; i++)
                    {
                        STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNumber]->ChannelTable[i].pBlockingSemaphore);
                    }
                    break;

                case INTERRUPT_INSTALL:
                    STOS_InterruptUninstall(InitParams_p->InterruptNumber,
                                            InitParams_p->InterruptLevel,
                                            (void *)&(stfdma_ControlBlock_p[DeviceNumber]->DeviceNumber));
                    break;

                case MESSAGE_SYSTEM:
                    TermMessages(DeviceNumber);
                    break;

                case STFDMA_CALLBACK_TASK_TABLE:
                    STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p);
                    break;

                case STFDMA_CALLBACK_TASK_INIT:
                    SendExitMessage(DeviceNumber);
                    for (i = 0; i < stfdma_ControlBlock_p[DeviceNumber]->NumberCallbackTasks; i++)
                    {
                        Task_p = stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[i].pTask;
                        STOS_TaskWait(&Task_p, TIMEOUT_INFINITY);
                        STOS_TaskDelete(Task_p, stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p,
                                        stfdma_ControlBlock_p[DeviceNumber]->CallbackTaskTable_p[i].TaskStack_p,
                                        stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p);
                    }
                    break;

                case CONTEXT_BLOCK:
#if defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
                    if (DeviceNumber == STFDMA_2 || DeviceNumber == STFDMA_1)
#else
                    if (DeviceNumber == STFDMA_1)
#endif
                    {
                    		/* Clean up contexts */
                   	 	stfdma_TermContexts(DeviceNumber, stfdma_ControlBlock_p[DeviceNumber]->DriverPartition_p);
                    }
                    break;                    
            }
        }
    }
    /* END: cleanup if error occurred */

    /* Hardware configuration */
#if defined (STFDMA_MPX_SUPPORT)
    if (InitParams_p->FDMABlock != STFDMA_MPX && ErrorCode == ST_NO_ERROR)
#else
    if (ErrorCode == ST_NO_ERROR)
#endif
    {
#if !defined(CONFIG_STM_DMA)
#if !defined(ST_5517)
        /* Disable the FDMA block */
        SET_REG(CLOCKGATE,1, DeviceNumber);
        SET_REG(ENABLE,   0, DeviceNumber);
#endif

#if !defined (STFDMA_TKDMA_LOAD)
        /* Initialise the hardware */
#if defined(ST_5517)
        stfdma_FDMA1Conf(((char*)stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p),
                         ((char*)stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p) +DMEM_OFFSET);
#else
        stfdma_FDMA2Conf(((char*)stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p) +IMEM_OFFSET,
                         ((char*)stfdma_ControlBlock_p[DeviceNumber]->BaseAddress_p) +DMEM_OFFSET, DeviceNumber);
#endif
#endif /*STFDMA_TKDMA_LOAD*/
        /* Configure the request lines in FDMA */
        ErrorCode = stfdma_ReqConf(stfdma_ControlBlock_p[DeviceNumber], DeviceNumber);
    	 if (ErrorCode != ST_NO_ERROR)
    	 {
    	 	STTBX_Print(("ERROR: stfdma_ReqConf() failed\n"));
    	 }

#if defined(ST_5517)
        /* Enable block by writing 1 enable switch */
        SET_REG(ENABLE,1, DeviceNumber);
#else
        /* Reset the mailbox registers */
        SET_REG(STATUS_CLR,  0XFFFFFFFF, DeviceNumber);
        SET_REG(STATUS_MASK, 0XFFFFFFFF, DeviceNumber);
        SET_REG(INTR_CLR,    0XFFFFFFFF, DeviceNumber);
        SET_REG(INTR_MASK,   0XFFFFFFFF, DeviceNumber);

#if defined (ST_498)
		/*For F/W workaround for free running transfers over MPX, clear DREQ_OVERRIDE register (assuming that STFDMA on eCM will handle all transfers over MPX)*/
		 SET_REG(DREQ_OVERRIDE, 0, DeviceNumber);
#endif

        /* Enable the FDMA block */
        SET_REG(SYNCREG,  1, DeviceNumber);
        SET_REG(CLOCKGATE,5, DeviceNumber);
        SET_REG(CLOCKGATE,0, DeviceNumber);
        SET_REG(ENABLE,   1, DeviceNumber);
#endif
#endif /* CONFIG_STM_DMA */
    }
    /* END: FDMA memory initialisation */

    /* Giveup init/term protection */
    STOS_SemaphoreSignal(InitTermSemap);
    STTBX_Print(("STFDMA_Init complete\n"));
    return ErrorCode;
}


/****************************************************************************
Name         : STFDMA_SetRequestLineIncrement
Description  : Set the increment bit  in REQ_CONTROL register. This register
               is programmed at the time of Init, but now increment(on/off) count can
               be reconfigured using this API.
Parameters   : RequestLineNo - Request Line need modification
               On - TRUE/FALSE (INC/STATIC).
               DeviceNo      - FDMA_1/FDMA_2
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER
               STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL
****************************************************************************/
/*Added for ST_498 but can be used across platforms*/
ST_ErrorCode_t STFDMA_SetRequestLineIncrement(U32 RequestLineNo,  BOOL On, STFDMA_Block_t DeviceNo)
{
    U32 ReqControl = 0;
    U32 State = 0;

    STTBX_Print(("STFDMA_SetRequestLineIncrement\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
    if ( (RequestLineNo < 1) || (RequestLineNo > 30) )
    {
        return STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

	 if (On) State = 1;
    ReqControl = GET_REQ_CONT(fdmareq_RequestLines[RequestLineNo].Index,  DeviceNo);
    ReqControl &= ~(1 << 29);     /*  " clear 29 */
    ReqControl |= (State << 29);  /*  " set 29 */
    SET_REQ_CONT(fdmareq_RequestLines[RequestLineNo].Index, ReqControl, DeviceNo);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STFDMA_SetTransferCount
Description  : Set the transfer count in REQ_CONTROL register. This register
               is programmed at the time of Init, but now transfer count can
               be reconfigured using this API.
Parameters   : RequestLineNo - Request Line need modification
               TransferCount - New Transfer count which needs to be updated.
               DeviceNo      - FDMA_1/FDMA_2
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER
               STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL
****************************************************************************/
ST_ErrorCode_t STFDMA_SetTransferCount(U32 RequestLineNo,  U32 TransferCount, STFDMA_Block_t DeviceNo)
{
    U32 ReqControl = 0;

    STTBX_Print(("STFDMA_SetTransferCount\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
    if ( (RequestLineNo < 1) || (RequestLineNo > 30) )
    {
        return STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    ReqControl = GET_REQ_CONT(fdmareq_RequestLines[RequestLineNo].Index,  DeviceNo);
    ReqControl &=0xE0FFFFFF;
    ReqControl |= (U32)((TransferCount-1) & 0x1F) << 24;  														/*  " 28...24 */
  

    SET_REQ_CONT((fdmareq_RequestLines[RequestLineNo].Index), (ReqControl), (DeviceNo));

    return ST_NO_ERROR;
}


/****************************************************************************
Name         : STFDMA_SetRequestLineOpCode
Description  : Set the transfer OPCODE in REQ_CONTROL register. This register
               is programmed at the time of Init, but now request OpCode can
               be reconfigured using this API.
Parameters   : RequestLineNo - Request Line need modification
               OpCode - New request OPCODE which needs to be updated.
               DeviceNo      - FDMA_1/FDMA_2
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER
               STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL
               ST_ERROR_BAD_PARAMETER
****************************************************************************/
ST_ErrorCode_t STFDMA_SetRequestLineOpCode(U32 RequestLineNo,  U8 OpCode, STFDMA_Block_t DeviceNo)
{
    U32 ReqControl = 0;

    STTBX_Print(("STFDMA_SetRequestLineOpCode\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
    if ( (RequestLineNo < 1) || (RequestLineNo > 30) )
    {
        return STFDMA_ERROR_UNKNOWN_REQUEST_SIGNAL;
    }
    switch(OpCode)
    {
        case OPCODE_1:
        case OPCODE_2:
        case OPCODE_4:
        case OPCODE_8:
        case OPCODE_16:				
        case OPCODE_32:
                break;
        default:
                return ST_ERROR_BAD_PARAMETER;			
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    ReqControl = GET_REQ_CONT(fdmareq_RequestLines[RequestLineNo].Index,  DeviceNo);

#if defined(ST_5517)
    ReqControl &=0xFFFFC0FF;
    ReqControl |= (U32)(OpCode & 0x3f) << 9;  		 /*  " 15...9 */
#else
    ReqControl &=0xFFFFFF0F;
    ReqControl |= (U32)(OpCode & 0x0f) <<  4; 	    /*  " 7...4 */
#endif
    SET_REQ_CONT(fdmareq_RequestLines[RequestLineNo].Index, ReqControl, DeviceNo);

    return ST_NO_ERROR;
}



/****************************************************************************
Name         : STFDMA_StartTransfer
Description  : Attempts to start the DMA transfer described in the Node
               structure within TransferParams_p. If the channel (specified
               or from the general pool) is free the Node pointer is written
               to the FDMA block which will then commence the linked list
               transfer (a linked list of one or more nodes), and return a
               transfer id. If no channels are available, an error is returned.
               Notification back to the caller is performed by the callback
               function specified in the TransferParams. The TransferId and
               Event that occured are passed into the callback to identify
               the nature of the event and the transfer it is related to.
Parameters   : TansferParams_p - DMA transfer parameters
               TransferId      - Returned id value unique to the requested transfer.
Return Value : ST_NO_ERROR
               ST_ERROR_BAD_PARAMETER
               STFDMA_ERROR_CHANNEL_BUSY
               STFDMA_ERROR_NO_FREE_CHANNELS
               STFDMA_ERROR_BLOCKING_TIMEOUT
               STFDMA_ERROR_NO_CALLBACK_TASK
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_UNKNOWN_CHANNEL_ID
               STFDMA_ERROR_TRANSFER_ABORTED
               STFDMA_ERROR_REQUEST_SIGNAL_BUSY
****************************************************************************/
#if defined (STFDMA_LEGACY_CODE)
ST_ErrorCode_t STFDMA_StartTransfer(STFDMA_TransferParams_t *TransferParams_p,
                                    STFDMA_TransferId_t *TransferId_p)
#else
ST_ErrorCode_t STFDMA_StartTransfer(STFDMA_TransferParams_t *TransferParams_p,
                                    STFDMA_TransferId_t *TransferId_p,
                                    BOOL IsPaced)
#endif
{
    STFDMA_TransferGenericParams_t Params;

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (TransferParams_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    Params.ChannelId            = TransferParams_p->ChannelId;
    Params.Pool                 = STFDMA_DEFAULT_POOL;
    Params.NodeAddress_p        = (STFDMA_GenericNode_t*)TransferParams_p->NodeAddress_p;
#if !defined (STFDMA_LEGACY_CODE)
    Params.NodeAddress_Vp		= (STFDMA_GenericNode_t*)TransferParams_p->NodeAddress_Vp;
#endif
    Params.BlockingTimeout      = TransferParams_p->BlockingTimeout;
    Params.CallbackFunction     = TransferParams_p->CallbackFunction;
    Params.ApplicationData_p    = TransferParams_p->ApplicationData_p;
    Params.FDMABlock            = TransferParams_p->FDMABlock;

#if defined (STFDMA_LEGACY_CODE)
    return (STFDMA_StartGenericTransfer(&Params, TransferId_p));
#else
    return (STFDMA_StartGenericTransfer(&Params, TransferId_p, IsPaced));
#endif
}


#if defined (STFDMA_LEGACY_CODE)
ST_ErrorCode_t STFDMA_StartGenericTransfer(STFDMA_TransferGenericParams_t *TransferParams_p,
                                    STFDMA_TransferId_t *TransferId_p)
#else
ST_ErrorCode_t STFDMA_StartGenericTransfer(STFDMA_TransferGenericParams_t *TransferParams_p,
                                    STFDMA_TransferId_t *TransferId_p,
                                    BOOL IsPaced)
#endif
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 SelectedChannel;
    U32 DeviceNo = 0;
	STFDMA_Node_t *Node_Vp;	
	
#ifndef STFDMA_NO_PARAMETER_CHECK
    if ((TransferParams_p == NULL) ||
        (TransferId_p     == NULL))
    {
        STTBX_Print(("ERROR: Bad parameters %08X %08X\n", (U32)TransferParams_p, (U32)TransferId_p));
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Check init parameters */
    if (TransferParams_p->FDMABlock >= STFDMA_MAX)
    {
        STTBX_Print(("ERROR: One or more init parameters invalid\n"));
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

    DeviceNo = TransferParams_p->FDMABlock;
#ifndef STFDMA_NO_USAGE_CHECK
    /* Check parameters */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

#ifndef STFDMA_NO_PARAMETER_CHECK
    if ((TransferParams_p->Pool < 0) ||
        (MAX_POOL <= TransferParams_p->Pool))
    {
        STTBX_Print(("ERROR: Bad pool 0 <= %d < %d\n", TransferParams_p->Pool, MAX_POOL));
        return ST_ERROR_BAD_PARAMETER;
    }

    if (TransferParams_p->NodeAddress_p == NULL)
    {
        STTBX_Print(("ERROR: Bad starting node address = %08x\n", TransferParams_p->NodeAddress_p));
        return ST_ERROR_BAD_PARAMETER;
    }
    
#if !defined (STFDMA_LEGACY_CODE)
    if (TransferParams_p->NodeAddress_Vp == NULL)
    {
        STTBX_Print(("ERROR: Bad starting node Virtual address = %08x\n", TransferParams_p->NodeAddress_Vp));
        return ST_ERROR_BAD_PARAMETER;
    }    
#endif
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    if ((stfdma_ControlBlock_p[DeviceNo]->NumberCallbackTasks == 0) &&
        (TransferParams_p->CallbackFunction != NULL))
    {
        return STFDMA_ERROR_NO_CALLBACK_TASK;
    }

    if ((TransferParams_p->CallbackFunction == STFDMA_NO_CALLBACK) &&
        (!IS_LOCKED(TransferParams_p->ChannelId, DeviceNo)))
    {
        return STFDMA_ERROR_CHANNEL_NOT_LOCKED;
    }
#endif

    /* No new transfers should be started while the driver is terminating */
    if (IS_TERMINATING(DeviceNo))
    {
        return STFDMA_ERROR_NOT_INITIALIZED;
    }

#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
	if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX && PIOOpened == FALSE)
	{
		ErrorCode = stfdma_RouteInterrupt(MPX_ESTB_PIO_DEVICE_NAME, STFDMA_MPX_ESTB_PIO_PORT, STFDMA_MPX_ESTB_PIO_BIT);
		if(ErrorCode != ST_NO_ERROR)
		{
			STTBX_Print(("stfdma_RouteInterrupt() Error = %x\n", ErrorCode));
			return ErrorCode;
		}
		PIOOpened = TRUE;
	}
#endif	

    /* Check if channel is specified or obtain channel to use.
     * If a specific channel is given it must have locked by an earlier call
     * to STFDMA_LockChannel().
     * That channel can only be used for this transfer if is not in use.
     * If no particular channel is specified, try to obtain a free channel.
     */

    ENTER_CRITICAL_SECTION(DeviceNo);

    if (TransferParams_p->ChannelId != STFDMA_USE_ANY_CHANNEL)
    {
        if (IS_LOCKED(TransferParams_p->ChannelId, DeviceNo))
        {
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
            STTBX_Print(("GET_GENERIC_DMA A : %d %d \n",TransferParams_p->ChannelId, GET_GENERIC_DMA(TransferParams_p->ChannelId, DeviceNo)));
            if ((IsChannelBusy(TransferParams_p->ChannelId, DeviceNo)) && (!GET_GENERIC_DMA(TransferParams_p->ChannelId, DeviceNo)))
#else
            if (IsChannelBusy(TransferParams_p->ChannelId, DeviceNo))
#endif
            {
		        STTBX_Print(("STFDMA_ERROR_CHANNEL_BUSY : %d\n",TransferParams_p->ChannelId));
		        LEAVE_CRITICAL_SECTION(DeviceNo);
                return STFDMA_ERROR_CHANNEL_BUSY;
            }
            else
            {
                SelectedChannel = TransferParams_p->ChannelId;
            }
        }
        else
        {
            STTBX_Print(("STFDMA_ERROR_CHANNEL_NOT_LOCKED : %d\n",TransferParams_p->ChannelId));
            LEAVE_CRITICAL_SECTION(DeviceNo);
            return STFDMA_ERROR_CHANNEL_NOT_LOCKED;
        }
    }
    else
    {
        /* No channel specified, so get one */
        SelectedChannel = GetNextFreeChannel(PoolMapping[TransferParams_p->Pool], DeviceNo);
        if (SelectedChannel == NO_FREE_CHANNELS)
        {
            STTBX_Print(("STFDMA_ERROR_NO_FREE_CHANNELS : %d\n",SelectedChannel));
            LEAVE_CRITICAL_SECTION(DeviceNo);
            return STFDMA_ERROR_NO_FREE_CHANNELS;
        }
    }

    /* Check if Same DREQ is used for this transfer??
     * If yes return an error STFDMA_ERROR_REQUEST_SIGNAL_BUSY,
     * else start the channel and mark its DREQ as TRUE.
     * when transfer is finished mark its DREQ as FALSE.
     */

#if defined (STFDMA_LEGACY_CODE)
    if(TransferParams_p->Pool != STFDMA_PES_POOL && TransferParams_p->Pool != STFDMA_SPDIF_POOL)
#else
	/*Node checks for various params for paced transfers only*/
    if(TransferParams_p->Pool != STFDMA_PES_POOL && TransferParams_p->Pool != STFDMA_SPDIF_POOL && IsPaced)
#endif
    {
    	U32 channelNumber = 0;
#if defined (STFDMA_LEGACY_CODE)
        Node_Vp = (STFDMA_Node_t*)(TransferParams_p->NodeAddress_p);
#if defined (STFDMA_MPX_SUPPORT)
		if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
		{
			/*Node should reside in 498 shared LMI memory*/
	    	Node_Vp = (STFDMA_Node_t*)GET_MPX_ESTB_SHARED_VADDRESS(Node_Vp);
	    }
	    else
	    {
#endif /*STFDMA_MPX_SUPPORT*/
        
#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    	Node_Vp = (void*)ST40_NOCACHE_NOTRANSLATE(Node_Vp);
#else
    	Node_Vp = (void*)__va((U32)Node_Vp);
#endif
#endif
#if defined (STFDMA_MPX_SUPPORT)
		}
#endif	
#else /*!STFDMA_LEGACY_CODE*/
		Node_Vp = (STFDMA_Node_t*)TransferParams_p->NodeAddress_Vp;			 
/*Additional handling for 7109+498 in 32 bit mode required or will simple mapping do ?*/		
#if defined (STFDMA_MPX_SUPPORT)
		if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
		{
			/*Node should reside in 498 shared LMI memory*/
	    	Node_Vp = (STFDMA_Node_t*)GET_MPX_ESTB_SHARED_VADDRESS(Node_Vp);
	    }
#endif /*STFDMA_MPX_SUPPORT*/		
#endif

#if defined (STFDMA_NAND_SUPPORT)
		/*For NAND channel user should  send a valid memory for ApplicationData_p used to send back page_ptr_error if required*/
		 if(((STFDMA_NANDNode_t *)Node_Vp)->Extended == STFDMA_EXTENDED_NODE && ((STFDMA_NANDNode_t *)Node_Vp)->Type == STFDMA_EXT_NODE_NAND)
		 {
		 	if(NULL == TransferParams_p->ApplicationData_p)
		 	{
				STTBX_Print(("WARNING: ApplicationData_p is NULL, will not be able to send NAND_PAGE_ERROR\n"));         			
			 }
		 }
#endif	
#if defined (STFDMA_MPX_SUPPORT)
/*STFDMA_MPX_SUPPORT_TODO  32 bit port needed here*/
        if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
		{
			 if(((STFDMA_MCHIRxNode_t *)Node_Vp)->Extended == STFDMA_EXTENDED_NODE && ((STFDMA_MCHIRxNode_t *)Node_Vp)->Type == STFDMA_EXT_NODE_MCHI)
			 {
			 	if(NULL == TransferParams_p->ApplicationData_p)
			 	{
	                LEAVE_CRITICAL_SECTION(DeviceNo);
	                return ST_ERROR_BAD_PARAMETER;              			
				 }
				 SET_REG(MCHI_BYTES_ALL_NODES, 0, DeviceNo);	 /*Reset byte transfer count*/
				 /*SET_REG(MCHI_BYTES_CURRENT_NODE, 0, DeviceNo);*/ /*Reset byte transfer count current node*/
			 }
		 }
#endif	

		if (Node_Vp->NodeControl.PaceSignal != STFDMA_REQUEST_SIGNAL_NONE)
		{  
			/*Search in all channels if any one of them is using the given PaceSignal*/
			for(channelNumber = 0; channelNumber < NUM_CHANNELS; channelNumber++)
			{
				/*all ChannelTable[i].PacedSignalUsed == STFDMA_REQUEST_SIGNAL_NONE initially*/
				if(stfdma_ControlBlock_p[DeviceNo]->ChannelTable[channelNumber].PacedSignalUsed == Node_Vp->NodeControl.PaceSignal)
				{
					LEAVE_CRITICAL_SECTION(DeviceNo);
					return STFDMA_ERROR_REQUEST_SIGNAL_BUSY;
				}
			}
			/*If no channel was using this signal then assign this signal to  the SelectedChannel*/
			stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].PacedSignalUsed = Node_Vp->NodeControl.PaceSignal;
#if defined (ST_498)
				/*For F/W workaround for free running transfers over MPX (assuming that STFDMA on eCM will handle all transfers over MPX)*/
				/*Set RequestConf bit for the signal to make destination Incrementing*/
			if (	!(Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS0
					||Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS1
					||Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS2) 
				)
			{  
				ErrorCode = STFDMA_SetRequestLineIncrement(Node_Vp->NodeControl.PaceSignal, TRUE, DeviceNo);
				if(ST_NO_ERROR != ErrorCode)
				{
					return ErrorCode;
				}
				/*Set the bit corresponding to the psuedo request signal in DREQ_OVERRIDE*/
				stfdma_SetDreqOverride(Node_Vp->NodeControl.PaceSignal, TRUE, DeviceNo);
			}	
#endif
			
		}/*VirtNode->NodeControl.PaceSignal != STFDMA_REQUEST_SIGNAL_NONE*/		        
    }

#if !defined(ST_5517)
    if (TransferParams_p->Pool == STFDMA_SPDIF_POOL)
    {
        /* Clear the additional data for this channel */

        ((U32*)((U32)stfdma_ControlBlock_p[DeviceNo]->BaseAddress_p + SPDIF_CONTEXT_OFFSET))[9] = 0;
    }
#endif

    STTBX_Print(("Start transfer channel %d\n", SelectedChannel));

    /* Create transfer reference Id. Do not pass to application until transfer begun. */
    SET_TRANSFER_ID(SelectedChannel, CreateTransferId(SelectedChannel, DeviceNo), DeviceNo);

    LEAVE_CRITICAL_SECTION(DeviceNo);

    /* Update internal structure in readiness for transfer */
    CLEAR_PAUSED(SelectedChannel, DeviceNo);
    CLEAR_ABORTING(SelectedChannel, DeviceNo);
    if (TransferParams_p->CallbackFunction == NULL)
    {
        stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].Blocking = TRUE;
    }
    else
    {
        stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].Blocking = FALSE;
    }

    stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].CallbackFunction_p
        = TransferParams_p->CallbackFunction;

    stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].ApplicationData_p
        = TransferParams_p->ApplicationData_p;

    /* This channel is no longer idle because we are about to start it */
    CLEAR_IDLE(SelectedChannel, DeviceNo);

    STOS_InterruptLock();

    /* Start the transfer */
#if defined(ST_5517)
    SET_CMD_STAT( SelectedChannel, TransferParams_p->NodeAddress_p, DeviceNo );
#else
    SET_CMD_STAT( SelectedChannel, ((((U32)TransferParams_p->NodeAddress_p)&~0X1F) | 1) , DeviceNo);
    SET_REG(INTR_SET, (0X01<<(SelectedChannel*2)), DeviceNo);
#endif

    /* Once the transfer is running, pass back the TransferId to the caller.
     * Providing this value before the transfer has started may be dangerous.

     * MAS: update Transfer ID for Application in interrupt lock, so that a very
     * small transfer should not be completed before providing the transfer ID
     * to the application.
     */
    *TransferId_p = GET_TRANSFER_ID(SelectedChannel, DeviceNo);

    STOS_InterruptUnlock();

#if !defined(ST_5517)
    STTBX_Print(("Request [%08X] %d %08X...\n", GET_REG(INTR, DeviceNo), SelectedChannel, (((U32)TransferParams_p->NodeAddress_p)&~0X1F)));
#endif

    /* For non-blocking transfers exit now */
    if( IS_BLOCKING(SelectedChannel, DeviceNo) == FALSE )
    {
        STTBX_Print(("Non Blocking\n"));
        return ST_NO_ERROR;
    }
    else  /* Its a blocking transfer....*/
    {
        int SemReturnValue = 0;

        STTBX_Print(("Blocking\n"));

        /* Blocking timeout control.
         * If a timeout value is given, use it, Otherwise timeout is infinite.
         */
        if (TransferParams_p->BlockingTimeout != 0)
        {
            STOS_Clock_t time;
            STTBX_Print(("Time out %d\n", TransferParams_p->BlockingTimeout));
            /* Find the time at which timeout occurs.
             * BlockingTimeout value given in milliseconds. Calculation converts from ms to ticks using divide 1000.
             * Max time permitted by calculation is 76mins, min 1ms.
             */

#ifdef PROCESSOR_C1
            time = STOS_time_plus(time_now(), ((stfdma_ControlBlock_p[DeviceNo]->ClockTicksPerSecond / 1000) * TransferParams_p->BlockingTimeout));
#else
            time = STOS_time_plus(time_now(), ((stfdma_ControlBlock_p[DeviceNo]->ClockTicksPerSecond * TransferParams_p->BlockingTimeout) / 1000));
#endif

            STTBX_Print(("now: %d, timeout requested, %d, tickspersec %d, timeout time %d\n",
                         time_now(),
                         TransferParams_p->BlockingTimeout,
                         stfdma_ControlBlock_p[DeviceNo]->ClockTicksPerSecond,
                         time));

            /* Block task by waiting on semaphore */
            SemReturnValue = STOS_SemaphoreWaitTimeOut(GET_BLOCKING_SEM_PTR(SelectedChannel, DeviceNo), &time );
        }
        else
        {
            STTBX_Print(("No Time out\n"));
            /* Block task by waiting on semaphore */
            SemReturnValue = STOS_SemaphoreWaitTimeOut(GET_BLOCKING_SEM_PTR(SelectedChannel, DeviceNo), TIMEOUT_INFINITY);
            STTBX_Print(("StartGenericTransfer - Semaphore returned.\n"));
        }

        /* Timeout means blocking timeout elapsed before transfer completion...*/
        if( SemReturnValue == SEMAPHORE_TIMEOUT )
        {
            STTBX_Print(("Blocking timeout: %u\n", (U32)time_now()));
            /* Regardless of whether the transfer has actually halted the transfer in the time elapsed since
             * the timeout occured, the transfer is aborted.
             * If an interrupt happens (transfer completes) just after the timeout, the subsequent seamphore_wait
             *  will clear the semaphore
             */

            /* Abort the transfer and wait for abort completion */
            STTBX_Print(("Aborting transfer\n"));

            STTBX_Print(("Channel Status is 0x%08X\n",
                GET_REG((CMD_STAT_BASE + ((SelectedChannel) * CMD_STAT_CHAN_OFFSET)), DeviceNo)
                ));
#if defined(ST_5517)
            SET_CMD_STAT(SelectedChannel, NULL, DeviceNo);
#else
            SET_REG(INTR_SET, (0X02<<(SelectedChannel*2)), DeviceNo);
#endif
            STOS_SemaphoreWait(GET_BLOCKING_SEM_PTR(SelectedChannel, DeviceNo));
            STTBX_Print(("Transfer aborted\n"));
            ErrorCode = STFDMA_ERROR_BLOCKING_TIMEOUT;
        }
        else  /* Transfer completion before timeout elapsed */
        {
#if defined (STFDMA_DEBUG_SUPPORT)
            if(stfdma_Status[DeviceNo] != NULL)
                stfdma_Status[DeviceNo]->Channel[SelectedChannel].InterruptBlockingSemaphoreReceived++;
#endif

            STTBX_Print(("Blocking semap signal at time %u\n", (U32)time_now()));
            /* Transfer may have been aborted by external task call to STFDMA_AbortTransfer.
             * If so, need to return required error code.
             */
            if (IS_ABORTING(SelectedChannel, DeviceNo))
            {
                ErrorCode = STFDMA_ERROR_TRANSFER_ABORTED;
            }
            else
            {
                if (IS_ERROR(SelectedChannel, DeviceNo))
                {
#if defined (STFDMA_NAND_SUPPORT)
#if defined (STFDMA_LEGACY_CODE)
				        Node_Vp = (STFDMA_Node_t*)(TransferParams_p->NodeAddress_p);
#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
				    	Node_Vp = (void*)ST40_NOCACHE_NOTRANSLATE(Node_Vp);
#else
				    	Node_Vp = (void*)__va((U32)Node_Vp);
#endif
#endif
#else
						Node_Vp = (STFDMA_Node_t*)TransferParams_p->NodeAddress_Vp;			 
#endif
						if(((STFDMA_NANDNode_t *)Node_Vp)->Extended == STFDMA_EXTENDED_NODE && ((STFDMA_NANDNode_t *)Node_Vp)->Type == STFDMA_EXT_NODE_NAND)
						{
							if(stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].ApplicationData_p != NULL)
							{
								stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].ApplicationData_p = (void*)GET_REG(NAND_PAGE_ERROR, DeviceNo);
							}
						}
#endif
                    ErrorCode = STFDMA_ERROR_TRANSFER_FAILED;
                    CLEAR_ERROR(SelectedChannel, DeviceNo);
                }
                else
                {
                    ErrorCode = ST_NO_ERROR;
                }
            }
        }

        /* Tidy up internal controls */
        CLEAR_TRANSFER_ID(SelectedChannel, DeviceNo);

        /* Give up the channel if not locked.
         * A critical section exists between checking channel is locked and giving
         * the channel back to the free channel pool. Application tasks may change the
         * state of the channel between these calls. Internal state control would
         * become confused if the channel state changes between these lines, so
         * they are semaphore protected.
         */

        STTBX_Print(("Entering critical section\n"));
        ENTER_CRITICAL_SECTION(DeviceNo);
        {
            if( !IS_LOCKED(SelectedChannel, DeviceNo) )
            {
                GiveUpChannel(SelectedChannel, DeviceNo);
            }
        }
        LEAVE_CRITICAL_SECTION(DeviceNo);
        STTBX_Print(("Left critical section\n"));
    }

    /* Trnasfer is completed, aborted, or failed.
     * we need to clear DREQ status.
     */

    if(TransferParams_p->Pool != STFDMA_PES_POOL && TransferParams_p->Pool != STFDMA_SPDIF_POOL)
	{
      stfdma_ControlBlock_p[DeviceNo]->ChannelTable[SelectedChannel].PacedSignalUsed = STFDMA_REQUEST_SIGNAL_NONE;
#if defined (STFDMA_LEGACY_CODE)
        Node_Vp = (STFDMA_Node_t*)(TransferParams_p->NodeAddress_p);
#if defined (STFDMA_MPX_SUPPORT)
		if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
		{
			/*Node should reside in 498 shared LMI memory*/
	    	Node_Vp = (STFDMA_Node_t*)GET_MPX_ESTB_SHARED_VADDRESS(Node_Vp);
	    }
	    else
	    {
#endif /*STFDMA_MPX_SUPPORT*/
        
#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    	Node_Vp = (void*)ST40_NOCACHE_NOTRANSLATE(Node_Vp);
#else
    	Node_Vp = (void*)__va((U32)Node_Vp);
#endif
#endif
#if defined (STFDMA_MPX_SUPPORT)
		}
#endif	
#else /*!STFDMA_LEGACY_CODE*/
		Node_Vp = (STFDMA_Node_t*)TransferParams_p->NodeAddress_Vp;			 
/*Additional handling for 7109+498 in 32 bit mode required or will simple mapping do ?*/		
#if defined (STFDMA_MPX_SUPPORT)
		if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
		{
			/*Node should reside in 498 shared LMI memory*/
	    	Node_Vp = (STFDMA_Node_t*)GET_MPX_ESTB_SHARED_VADDRESS(Node_Vp);
	    }
#endif /*STFDMA_MPX_SUPPORT*/		
#endif
#if defined (ST_498)
		/*For F/W workaround for free running transfers over MPX (assuming that STFDMA on eCM will handle all transfers over MPX)*/
		/*Reset RequestConf bit for the signal to make destination non Incrementing*/
		/*Potential issue: will have to rememebr previous original value (from array used in Init) in case same code to be used in other devices*/
		if (	!(Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS0
				||Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS1
				||Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS2) 
			)
		{ 			
			ErrorCode = STFDMA_SetRequestLineIncrement(Node_Vp->NodeControl.PaceSignal, FALSE, DeviceNo);
			if(ST_NO_ERROR != ErrorCode)
			{
				return ErrorCode;
			}
			/*Reset the bit corresponding to the psuedo request signal in DREQ_OVERRIDE*/
			stfdma_SetDreqOverride(Node_Vp->NodeControl.PaceSignal, FALSE, DeviceNo);
		}
#endif
#if defined (STFDMA_MPX_SUPPORT)						
        	if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
			{
					if(((STFDMA_MCHIRxNode_t *)Node_Vp)->Extended == STFDMA_EXTENDED_NODE && ((STFDMA_MCHIRxNode_t *)Node_Vp)->Type == STFDMA_EXT_NODE_MCHI)
					{
						*((U32 *)TransferParams_p->ApplicationData_p) = (U32)GET_REG(MCHI_BYTES_ALL_NODES, DeviceNo);	                			
					}	
			}		
#endif		              
    }
    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_ResumeTransfer
Description  : Resumes a paused transfer
Parameters   : TransferId_p  - transfer to resume
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INTIIALZED
               STFDMA_ERROR_INVALID_TRANSFER_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_ResumeTransfer(STFDMA_TransferId_t TransferId)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Channel, DeviceNo;

    /* Get Device number from the transfer id */
    DeviceNo = GetDeviceFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    /* Obtain channel number */
    Channel = GetChannelFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    /* Check transfer really is running on the specified channel */
    if ((GET_TRANSFER_ID(Channel, DeviceNo) != TransferId) || (GET_TRANSFER_ID(Channel, DeviceNo) == NO_TRANSFER))
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

    /* If the channel is paused, resume the transfer by loading the current nodes
     * next node pointer into the FDMA block and update internal controls.
     */
    if (IS_PAUSED(Channel, DeviceNo))
    {
        CLEAR_PAUSED(Channel, DeviceNo);
#if defined(ST_5517)
        SET_CMD_STAT(Channel, (stfdma_GetNextNodePtr(Channel, DeviceNo)), DeviceNo);
#else
        {
            U32 Node = GET_CURRENT_NODE_PTR( Channel, DeviceNo);

            STTBX_Print(("STFDMA_ResumeTransfer - Channel = %d node = 0x%08X\n",
                Channel,
                Node));

            SET_CMD_STAT( Channel, Node, DeviceNo);

#if defined (ST_5105) || defined (ST_5100) || defined (ST_5107) || defined (ST_5188) || defined (ST_5107)
           /* Get bytes transfered from FDMA */
           if (GET_BYTE_COUNT(Channel, DeviceNo)!=0)
                 SET_BYTE_COUNT(Channel,0, DeviceNo);
#endif

            SET_REG(INTR_SET, (0X01<<(Channel*2)), DeviceNo);
        }
#endif
    }

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_FlushTransfer
Description  : Flushes the internal buffers and pauses the transfer
Parameters   : TransferId_p  - transfer to resume
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INTIIALZED
               STFDMA_ERROR_INVALID_TRANSFER_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_FlushTransfer(STFDMA_TransferId_t TransferId)
{
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710) || defined(ST_5517)
    return (ST_ERROR_FEATURE_NOT_SUPPORTED);
#else

    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Channel, DeviceNo;

    /* Get Device number from the transfer id */
    DeviceNo = GetDeviceFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    /* Obtain channel number */
    Channel = GetChannelFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    /* Check transfer really is running on the specified channel */
    if ((GET_TRANSFER_ID(Channel, DeviceNo) != TransferId) || (GET_TRANSFER_ID(Channel, DeviceNo) == NO_TRANSFER))
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

    SET_ABORTING(Channel, DeviceNo);
    SET_REG(INTR_SET, (0X03<<(Channel*2)), DeviceNo);

    return ErrorCode;
#endif /* feature supported */
}

/****************************************************************************
Name         : STFDMA_AbortTransfer
Description  : Aborts a running transfer.
Parameters   : TransferId_p  - transfer to abort
Return Value : ST_NO_ERROR
               STFDMA_ERROR_INVALID_TRANSFER_ID
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_CHANNEL_BUSY
****************************************************************************/
ST_ErrorCode_t STFDMA_AbortTransfer(STFDMA_TransferId_t TransferId)
{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Channel, DeviceNo;

    /* Get Device number from the transfer id */
    DeviceNo = GetDeviceFromTransferId(TransferId);
#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check parameters */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    /* Get channel number from the transfer id */
    Channel = GetChannelFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    if( (GET_TRANSFER_ID(Channel, DeviceNo) != TransferId) || (GET_TRANSFER_ID(Channel, DeviceNo) == NO_TRANSFER) )
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

    /* Abort the transfer.
     * Set internal control flag and write NULL to channels node ptr, forcing
     * FDMA to abort the tranfer.
     */
    SET_ABORTING(Channel, DeviceNo);
#if defined(ST_5517)
    STOS_InterruptLock();
    SET_CMD_STAT(Channel, NULL, DeviceNo);
    STOS_InterruptUnlock();
#else
    {
        U32 CommandStatus = GET_REG(INTR, DeviceNo);
    	if ( ((CommandStatus >> (Channel*2)) & 0x3) == 0x02)
    	{
            ErrorCode =  STFDMA_ERROR_CHANNEL_BUSY;
        }
        else
        {
            STOS_InterruptLock();
            SET_REG(INTR_SET, (0X02<<(Channel*2)), DeviceNo);
            STOS_InterruptUnlock();
        }
    }
#endif

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_LockChannel
Description  : Locks a channel and returns a locked channel id
Parameters   : *ChannelId - Pointer to storage location for return channelId
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INITIALIZED
               ST_ERROR_BAD_PARAMETER
               STFDMA_ERROR_NO_FREE_CHANNELS
               STFDMA_ERROR_ALL_CHANNELS_LOCKED
****************************************************************************/
ST_ErrorCode_t STFDMA_LockChannel(STFDMA_ChannelId_t *ChannelId, STFDMA_Block_t DeviceNo)
{
    return (STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL, ChannelId, DeviceNo));
}

ST_ErrorCode_t STFDMA_LockChannelInPool(STFDMA_Pool_t Pool, STFDMA_ChannelId_t *ChannelId, STFDMA_Block_t DeviceNo)
{
    STFDMA_ChannelId_t LocalChannelId;

    STTBX_Print(("STFDMA_LockChannel\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (ChannelId == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
    else if ((Pool < 0) || (MAX_POOL <= Pool))
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    /* Obtain a free channel from the free channel pool */
    LocalChannelId = GetNextFreeChannel(PoolMapping[Pool], DeviceNo);
    if( LocalChannelId == NO_FREE_CHANNELS )
    {
        return STFDMA_ERROR_NO_FREE_CHANNELS;
    }

    /* Flag the channel as locked and return id to application */
    SET_LOCKED(LocalChannelId, DeviceNo);
    *ChannelId = LocalChannelId;

    return ST_NO_ERROR;
}

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,11)
/**************************************************************************
Name         : STFDMA_LockFdmaChannel
Description  : Lock all the channel already locked by 7100_fdma2 DMA driver
Parameters   : ChannelId : a pointer to an array of integer. If the value is 1,
		the corresponding channel is already locked.
****************************************************************************/
ST_ErrorCode_t STFDMA_LockFdmaChannel(int *ChannelId, STFDMA_Block_t DeviceNo)
{
    int Index;

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (ChannelId == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }
#endif

    STFDMA_Mask = 0;
    STFDMA_MaxChannels = NUM_CHANNELS;
    for (Index=0; Index < NUM_CHANNELS; Index ++)
    {
    	if (*ChannelId++ == 1)
    	{
       		SET_LOCKED(Index, DeviceNo);
    		/* decrease the MaxChannels number */
    		STFDMA_MaxChannels --;
    	}
    	else
    	{
    		/* Set the FDMA_Mask */
    		STFDMA_Mask <<= 2;
    		STFDMA_Mask +=3;
    	}
    }

    /* The last free channel in a pool cannot reference the next channel */
    stfdma_ControlBlock_p[DeviceNo]->ChannelTable[STFDMA_MaxChannels - 1].NextFreeChannel = NO_FREE_CHANNELS;

    /*printk("STFDMA_LockFdmaChannel : STFDMA will use %d channels, Mask = 0x%x, ~Mask = 0x%x\n", STFDMA_MaxChannels, STFDMA_Mask, ~STFDMA_Mask);*/
    return ST_NO_ERROR;
}

#else
/**************************************************************************
Name         : STFDMA_LockFdmaChannel
Description  : Lock all the channel already locked by 710x_fdma2 DMA driver
Parameters   :
****************************************************************************/
ST_ErrorCode_t STFDMA_LockFdmaChannel(STFDMA_Block_t DeviceNo)
{
    int Index;

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    STFDMA_Mask = 0;
    STFDMA_MaxChannels = 0;
    for (Index = NUM_CHANNELS - 1; Index > CONFIG_MAX_STM_DMA_CHANNEL_NR; Index--)
    {
        STFDMA_MaxChannels++;
		/* Set the FDMA_Mask */
		STFDMA_Mask <<= 2;
		STFDMA_Mask +=3;
    }

    /*Assuming native has the channels from the default pool only */
    if(CONFIG_MAX_STM_DMA_CHANNEL_NR < NUM_CHANNELS - 1)
        stfdma_ControlBlock_p[DeviceNo]->ChannelTable[NUM_CHANNELS - 1].NextFreeChannel = NO_FREE_CHANNELS;

    for (; Index >= CONFIG_MIN_STM_DMA_CHANNEL_NR; Index--)
    {
        SET_LOCKED(Index, DeviceNo);
		STFDMA_Mask <<= 2;
    }

    for (; Index >= 0; Index--)
    {
        STFDMA_MaxChannels++;
		/* Set the FDMA_Mask */
		STFDMA_Mask <<= 2;
		STFDMA_Mask +=3;
    }

    /*Assuming native has the channels from the default pool only */
    if(CONFIG_MIN_STM_DMA_CHANNEL_NR > 0)
    {
        if(CONFIG_MAX_STM_DMA_CHANNEL_NR < NUM_CHANNELS - 1)
            stfdma_ControlBlock_p[DeviceNo]->ChannelTable[CONFIG_MIN_STM_DMA_CHANNEL_NR - 1].NextFreeChannel = CONFIG_MAX_STM_DMA_CHANNEL_NR + 1;
        else
            stfdma_ControlBlock_p[DeviceNo]->ChannelTable[CONFIG_MIN_STM_DMA_CHANNEL_NR - 1].NextFreeChannel = NO_FREE_CHANNELS;
    }

    return ST_NO_ERROR;
}
#endif

/****************************************************************************
Name         : STFDMA_SetSTLinuxChannel
Description  : Set a channel flag to indicate it as been requested by
	       generic DMA driver and not by a STAPI Application
Parameters   : ChannelId - Channel Id to configure
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INITIALIZED
               STFDMA_UNKNOWN_CHANNEL_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_SetSTLinuxChannel(STFDMA_ChannelId_t ChannelId, STFDMA_Block_t DeviceNo)
{
    STTBX_Print(("STFDMA_SetSTLinuxChannel\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

#ifndef STFDMA_NO_PARAMETER_CHECK
    if ((IsOutsideRange(ChannelId, 0, NUM_CHANNELS - 1)) || (!IS_LOCKED(ChannelId, DeviceNo)))
    {
        return STFDMA_ERROR_UNKNOWN_CHANNEL_ID;
    }
#endif


    /* Set the channel as requested by generic DMA driver.
     * A critical section exists between checking channel is busy and giving
     * the channel back to the free channel pool. Application tasks may change the
     * state of the channel between these calls. Internal state control would
     * become confused if the channel state changes between these lines, so they
     * are semaphore protected.
     */
    ENTER_CRITICAL_SECTION(DeviceNo);
    {
    	/* Set the channel as used by a standard generic linux driver */
    	SET_GENERIC_DMA(ChannelId, DeviceNo);
    	STTBX_Print(("SET_GENERIC_DMA A: %d %d \n",ChannelId,GET_GENERIC_DMA(ChannelId, DeviceNo)));
    }
    LEAVE_CRITICAL_SECTION(DeviceNo);
    return ST_NO_ERROR;
}
#endif

/****************************************************************************
Name         : STFDMA_UnlockChannel
Description  : Unlocks a channel associated with the reference Id
Parameters   : ChannelId - Channel Id to unlock
Return Value : ST_NO_ERROR
               STFDMA_ERROR_NOT_INITIALIZED
               STFDMA_UNKNOWN_CHANNEL_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_UnlockChannel(STFDMA_ChannelId_t ChannelId, STFDMA_Block_t DeviceNo)
{
    STTBX_Print(("STFDMA_UnlockChannel\n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check initialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

#ifndef STFDMA_NO_PARAMETER_CHECK
    if ((IsOutsideRange(ChannelId, 0, NUM_CHANNELS - 1)) || (!IS_LOCKED(ChannelId, DeviceNo)))
    {
        return STFDMA_ERROR_UNKNOWN_CHANNEL_ID;
    }
#endif

    /* Unlock the channel.
     * A critical section exists between checking channel is busy and giving
     * the channel back to the free channel pool. Application tasks may change the
     * state of the channel between these calls. Internal state control would
     * become confused if the channel state changes between these lines, so they
     * are semaphore protected.
     */
    ENTER_CRITICAL_SECTION(DeviceNo);
    {
        CLEAR_LOCKED(ChannelId, DeviceNo);
        if( !IsChannelBusy(ChannelId, DeviceNo) )
        {
            GiveUpChannel(ChannelId, DeviceNo);
        }
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        /* Clear the generic DMA flag */
        CLEAR_GENERIC_DMA(ChannelId, DeviceNo);
#endif

    }
    LEAVE_CRITICAL_SECTION(DeviceNo);

    return ST_NO_ERROR;
}

/****************************************************************************
Name         : STFDMA_GetTransferStatus
Description  : Returns data regarding the current node of the specified
               transfer.
Parameters   : TransferId - transfer to enquire about
               TransferStatus_p - pointer to storage location for status info
Return Value : ST_NO_ERROR
               ST_ERROR_BAD_PARAMETER
               STFDMA_ERROR_NOT_INTIALIZED
               STFDMA_ERROR_INVALID_TRANSFER_ID
****************************************************************************/
ST_ErrorCode_t STFDMA_GetTransferStatus(STFDMA_TransferId_t TransferId,
                                        STFDMA_TransferStatus_t *TransferStatus_p)

{
    ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
    U32 Channel, DeviceNo;
    
    STTBX_Print(("STFDMA_GetTransferStatus   \n"));

    /* Get Device number from the transfer id */
    DeviceNo = GetDeviceFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

    /* Obtain channel number */
    Channel = GetChannelFromTransferId(TransferId);

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (TransferStatus_p == NULL)
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    if (IsOutsideRange(Channel, 0, NUM_CHANNELS - 1))
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }

#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check intialisation */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    /* in STAPI mode, Always check for a valid transfer Id.
       TODO : In the case this function is called by a standard linux driver (using the wrapper),
       it is possible the driver calls linux_wrapper_fdma_get_residue(), calling this function, before
       it launches the dma; as the DMA has not been lauched, the TransfertId is not initialized. */
    if (((GET_TRANSFER_ID(Channel, DeviceNo) != TransferId ) || (GET_TRANSFER_ID(Channel, DeviceNo) == NO_TRANSFER))
            && (!GET_GENERIC_DMA(Channel, DeviceNo)))
    {
        STTBX_Print(("STFDMA_ERROR_INVALID_TRANSFER_ID Channel = 0x%x 0x%x GENERIC_DMA = %d \n",
                GET_TRANSFER_ID(Channel, DeviceNo), TransferId, GET_GENERIC_DMA(Channel, DeviceNo)));

        /* If transferId not initialized, it is the first time this function is called
            for this channel. Don't return an error */

        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#else
    /* Always check for a valid transfer Id */
    if ((GET_TRANSFER_ID(Channel, DeviceNo) != TransferId ) || (GET_TRANSFER_ID(Channel, DeviceNo) == NO_TRANSFER))
    {
        return STFDMA_ERROR_INVALID_TRANSFER_ID;
    }
#endif

    /* Loadup application strucutre with transfer details...*/

    /* Get paused-field value from internal data */
    TransferStatus_p->Paused = IS_PAUSED(Channel, DeviceNo);


    /* Get node ptr from FDMA */
    TransferStatus_p->NodeAddress = GET_CURRENT_NODE_PTR(Channel, DeviceNo);

    /* Get bytes transfered from FDMA */
    TransferStatus_p->NodeBytesRemaining = GET_BYTE_COUNT(Channel, DeviceNo);

    return ErrorCode;
}

/****************************************************************************
Name         : STFDMA_GetRevision
Description  : Obtains the STFMDA driver revision string. This string uniquely
               references a specific FDMA driver code version and a specific
               FDMA block microcode version.
Parameters   : None
Return Value : Revision string.
****************************************************************************/
ST_Revision_t STFDMA_GetRevision(void)
{
    /*STTBX_Print(("STFDMA_GetRevision   \n"));*/

    return Revision;
}

/****************************************************************************
Name         : STFDMA_Term
Description  : Terminates the driver.
               An error will be returned if any transfers are in progress
               when terminate is called unless ForceTerminate is specified.
               ForceTerminate will cause all transfers the abort and the
               driver to terminate.
Parameters   : DeviceName - Name of intitialised driver to terminate
               ForceTerminate - Force flag
               Reserved - For later use (if necessary)
Return Value : ST_NO_ERROR
               ST_ERROR_UNKNOWN_DEVICE
               STFDMA_ERROR_NOT_INITIALIZED
****************************************************************************/
ST_ErrorCode_t STFDMA_Term(const ST_DeviceName_t DeviceName,
                           const BOOL ForceTerminate,
                           STFDMA_Block_t DeviceNo)
{
    U32     i;
    task_t *Task_p;
#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;
#endif

    STTBX_Print(("Entering STFDMA_Term \n"));

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceNo >= STFDMA_MAX)
    {
        return STFDMA_ERROR_UNKNOWN_DEVICE_NUMBER;
    }
#endif

#ifndef STFDMA_NO_USAGE_CHECK
    /* Check parameters */
    if (IS_INITIALISED(DeviceNo) == FALSE)
    {
        STTBX_Print(("ERROR: ControlBlock pointer NULL\n"));
        return STFDMA_ERROR_NOT_INITIALIZED;
    }
#endif

    /* Protect initterm sequence */
    STOS_SemaphoreWait(InitTermSemap);

#ifndef STFDMA_NO_PARAMETER_CHECK
    if (DeviceName == NULL)
    {
    	 STOS_SemaphoreSignal(InitTermSemap);
        return ST_ERROR_BAD_PARAMETER;
    }
    else if (strcmp(stfdma_ControlBlock_p[DeviceNo]->DeviceName,DeviceName) != 0)
    {
        STTBX_Print(("ERROR: DeviceName not recognised\n"));
        STOS_SemaphoreSignal(InitTermSemap);
        return ST_ERROR_UNKNOWN_DEVICE;
    }
#endif

#if defined (STFDMA_MPX_SUPPORT)
	/*IS aborting a transfer initialed by STFDMA_7109 on 498 possible ?*/
    if (stfdma_ControlBlock_p[DeviceNo]->DeviceNumber != STFDMA_MPX )
    {
#endif 

    if (ForceTerminate == FALSE)
    {
        if (IsTransferInProgress(DeviceNo) == TRUE)
        {
            STTBX_Print(("ERROR: Force not set and transfers active\n"));
            STOS_SemaphoreSignal(InitTermSemap);
            return STFDMA_ERROR_TRANSFER_IN_PROGRESS;
        }

        /* Unforced termination commences.
         * Set the exit flag to prevent new transfers.
         */
        SET_TERMINATING(DeviceNo);
    }
    else
    {
        /* Forced termination commmences.
         * Set the exit flag to prevent new transfers and abort any running transfers.
         */
        SET_TERMINATING(DeviceNo);

        if (IsTransferInProgress(DeviceNo) == TRUE)
        {
#if !defined (ST_5517)
            U32 ChannelPauseMask = 0;
#endif

            STTBX_Print(("Aborting running transfers\n"));

#if defined (ST_5517)
            STOS_InterruptLock();
#endif

            /* Set abort flag in all active transfers */
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
            for (i=0; i< STFDMA_MaxChannels; i++)
#else
            for (i=0; i< NUM_CHANNELS; i++)
#endif
            {
                if (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[i].TransferId != 0)
                {

                    SET_ABORTING(i, DeviceNo);

#if defined(ST_5517)
                    SET_CMD_STAT(i, NULL, DeviceNo);
#else
/*                    SET_REG(INTR_SET, (0X02<<(i*2)), DeviceNo);*/
                    ChannelPauseMask |= (0X02<<(i*2));
#endif
                }
            }

#if defined (ST_5517)
            STOS_InterruptUnlock();
#else
            STOS_InterruptLock();
            SET_REG(INTR_SET, ChannelPauseMask, DeviceNo);
            STOS_InterruptUnlock();
#endif

            /* Wait until all transfers aborted.
             * Sit in loop until transfer aborts complete. Ensure loop deschedules.
             */
            STTBX_Print(("Waiting for active transfers to abort\n"));
            while (IsTransferInProgress(DeviceNo) == TRUE)
                STOS_TaskDelay(stfdma_ControlBlock_p[DeviceNo]->ClockTicksPerSecond/10);
        }
    }
#if defined (STFDMA_MPX_SUPPORT)
    }
#endif 

#if !defined(CONFIG_STM_DMA)
#if defined (STFDMA_MPX_SUPPORT)
    if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber != STFDMA_MPX)
    {
#endif
    /* Disable FDMA block */
#if defined(ST_5517)
    SET_REG(ENABLE,0, DeviceNo);
#else
    SET_REG(CLOCKGATE,1, DeviceNo);
    SET_REG(ENABLE,   0, DeviceNo);
#endif
#if defined (STFDMA_MPX_SUPPORT)
    }
#endif
#endif

#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
	if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
	{
		if(PIOOpened)
		{
			ErrorCode = STPIO_Close(PIOHandle);
			if(ErrorCode != ST_NO_ERROR)
			{
				STTBX_Print(("Error STFDMA_Term() : STPIO_Close() = %x\n", ErrorCode));
			}
			else
			{
				PIOOpened = FALSE;
			}
		}	
	}	
	/*No FDMA interrupt handler was installed when PIO interrupt is used for STFDMA_MPX*/
    if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber != STFDMA_MPX)
    {
#endif    
    /* Remove interrupt handler */
    STTBX_Print(("Uninstalling interrupt\n"));
    STOS_InterruptUninstall(stfdma_ControlBlock_p[DeviceNo]->InterruptNumber,
                            stfdma_ControlBlock_p[DeviceNo]->InterruptLevel,
                            (void *)&(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber));
#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
	}
#endif   

    /* Delete resource associated with callback tasks */
    if (stfdma_ControlBlock_p[DeviceNo]->NumberCallbackTasks != 0)
    {
        STTBX_Print(("Terminate callback tasks and message queue\n"));

        /* Send exit message to all tasks with a ReceiveMessage() call. The task
         * MUST NOT make another ReceiveMessage call upon receiving the exit message.
         */
        SendExitMessage(DeviceNo);

        /* Delete all tasks and deallocate all task stack memory allocated */
        for (i = 0; i < stfdma_ControlBlock_p[DeviceNo]->NumberCallbackTasks ; i++)
        {
            Task_p = stfdma_ControlBlock_p[DeviceNo]->CallbackTaskTable_p[i].pTask;
            STOS_TaskWait(&Task_p, TIMEOUT_INFINITY);
            STOS_TaskDelete(Task_p, stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p,
                            stfdma_ControlBlock_p[DeviceNo]->CallbackTaskTable_p[i].TaskStack_p,
                            stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p);
        }

        TermMessages(DeviceNo);
        /* Deallocate task table */
        STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->CallbackTaskTable_p);
    }

    /* Delete semaps */
    STTBX_Print(("Delete semaphores and memory blocks\n"));
    STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->pAccessSemaphore);

    for (i=0; i < NUM_CHANNELS; i++)
    {
        STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->ChannelTable[i].pBlockingSemaphore);
    }

#if defined (STFDMA_DEBUG_SUPPORT)
    if(stfdma_Status[DeviceNo] != NULL)
    {
        STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_Status[DeviceNo]);
        stfdma_Status[DeviceNo] = NULL;
    }
#endif

#if defined (STFDMA_MPX_SUPPORT)
    if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber != STFDMA_MPX)
    {
#endif

    /* Context is supported on first FDMA */
#if defined (ST_7200) || defined (ST_7111) || defined (ST_7105)
    /* Adding STFDMA_1 as SPDIF which also uses a context is possible only on STFDMA_1 */
    if (DeviceNo == STFDMA_2 || DeviceNo == STFDMA_1)
#else
    if (DeviceNo == STFDMA_1)
#endif
    {
        /* Clean up contexts */
        stfdma_TermContexts(DeviceNo, stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p);
    }
#if defined (STFDMA_MPX_SUPPORT)
    }
#endif

     if(NULL != stfdma_ControlBlock_p[DeviceNo]->BaseAddress_p)
     {
	 		STOS_UnmapRegisters((void*)stfdma_ControlBlock_p[DeviceNo]->BaseAddress_p, (64 * 1024));
	 		stfdma_ControlBlock_p[DeviceNo]->BaseAddress_p = NULL;
	  }		
    /* Deallocate control block */
    STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]);

    /* Clean up globals */
    stfdma_ControlBlock_p[DeviceNo] = NULL;

    STOS_SemaphoreSignal(InitTermSemap);
    STTBX_Print(("STFDMA_Term complete\n"));
#if defined (STFDMA_MPX_SUPPORT) && !defined(STFDMA_MPX_USE_EXT_IRQ3)
	return ErrorCode;
#else
    return ST_NO_ERROR;
#endif    
}

/****************************************************************************
Name         : STFDMA_NO_CALLBACK
Description  : This is a stub function to allocate a unique value.
Parameters   : Have no meaning
Return Value :
****************************************************************************/
void  STFDMA_NO_CALLBACK (U32 a, U32 b, U32 *c, U32 d, BOOL e, void *f, STOS_Clock_t g)
{
}

/****************************************************************************
Name         : CallbackManager
Description  : This function will be instantiated several times.  It is
                responsible for decoupling user callbacks from the interrupt handler.
                It massages message data into the format defined by the CallbackFunction
                interface, and updates driver states.
Parameters   :
Return Value :
****************************************************************************/
static void CallbackManager(void* pData)
{
    U32     TransferId;             /* callback function parameters */
    U32     CallbackReason;
    U32     *CurrentNode_p;
    U32     NodeBytesTransfered;
    BOOL    Error = FALSE;
    MessageElement_t Message;
    STFDMA_CallbackFunction_t UserCallbackFunction_p=NULL;
    void *UserApplicationData_p;
    U32   DeviceNo;
    STOS_Clock_t InterruptTime;
    STFDMA_Node_t *Node_Vp;
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    STFDMA_LinuxCallbackFunction_t UserLinuxCallbackFunction_p=NULL;
#endif

    STOS_TaskEnter(pData);

    DeviceNo = (U32)pData;

    while(1)
    {
        /* Wait for notification by interrupt manager.
         * If the exit message is received, task runs to completion.
         */

        ReceiveMessage(&Message, DeviceNo);

		
#if defined (STFDMA_DEBUG_SUPPORT)
        if(stfdma_Status[DeviceNo] != NULL)
            stfdma_Status[DeviceNo]->Channel[Message.ChannelNumber].Callbacks++;
#endif

#if !defined(ST_5517)
        STTBX_Print(("Got a message %d %d %08X %04X\n", Message.InterruptType, Message.ChannelNumber, GET_NODE_STATE(Message.ChannelNumber, DeviceNo), Message.InterruptData));
#endif
        if (Message.InterruptType == EXIT_MESSAGE)
        {
            break;
        }

        /* Load up  variables with the status data provided by message recieved
         * that describes an interrupt recently processed.
         */

        /* Set callback's transferid */
        TransferId = GET_TRANSFER_ID(Message.ChannelNumber, DeviceNo);
        InterruptTime = Message.InterruptTime;

        /* Set callback's error flag if error occured */
        if ((Message.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE) ||
            (Message.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE_PAUSED) ||
            (Message.InterruptType == INTERRUPT_ERROR_LIST_COMPLETE) ||
            (Message.InterruptType == INTERRUPT_ERROR_SCLIST_OVERFLOW) ||
            (Message.InterruptType == INTERRUPT_ERROR_BUFFER_OVERFLOW)
#if defined (STFDMA_NAND_SUPPORT)            
          ||(Message.InterruptType == INTERRUPT_ERROR_NAND_READ)
#endif        
        )
        {
            Error = TRUE;
        }
        else
        {
            Error = FALSE;
        }

        /* Determine callback reason from interrupt.
         * Not a 1 to 1 mapping since error condition is flagged in the dedicated
         * variable "Error".
         */
        if (Message.InterruptType == INTERRUPT_NODE_COMPLETE ||
            Message.InterruptType == INTERRUPT_ERROR_NODE_COMPLETE)
        {
            CallbackReason = STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING;
        }
        else
        {
            if(IS_ABORTING(Message.ChannelNumber, DeviceNo))
            {
                CallbackReason = STFDMA_NOTIFY_TRANSFER_ABORTED;
            }
            else
            {
                switch(Message.InterruptType)
                {
                    case INTERRUPT_NODE_COMPLETE_PAUSED:
                    case INTERRUPT_ERROR_NODE_COMPLETE_PAUSED:
                        CallbackReason = STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED;
                        SET_PAUSED(Message.ChannelNumber, DeviceNo);
                    break;
#if defined (STFDMA_NAND_SUPPORT)  
                    case INTERRUPT_ERROR_NAND_READ:
                        CallbackReason = STFDMA_NOTIFY_NAND_READ_ERROR;
                    break;
#endif                    
                    default:
                    case INTERRUPT_LIST_COMPLETE:
                    case INTERRUPT_ERROR_LIST_COMPLETE:
                        CallbackReason = STFDMA_NOTIFY_TRANSFER_COMPLETE;
                    break;

                }
            }
        }

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        if (GET_GENERIC_DMA(Message.ChannelNumber, DeviceNo))
        {
    		UserLinuxCallbackFunction_p = (STFDMA_LinuxCallbackFunction_t)stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].CallbackFunction_p;
            /* Set callback's current node pointer  */
            CurrentNode_p =  (U32*)((U32)GET_CURRENT_NODE_PTR(Message.ChannelNumber, DeviceNo)) ;
        }
        else
        {
        	UserCallbackFunction_p = (STFDMA_CallbackFunction_t)stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].CallbackFunction_p;
        	/* Set callback's current node pointer  */
        	CurrentNode_p = (U32*)GET_CURRENT_NODE_PTR(Message.ChannelNumber, DeviceNo);
        }
#else
        /* Store data from the channel table, before the channel entry is lost in the subsequent code */
        UserCallbackFunction_p = (STFDMA_CallbackFunction_t)stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].CallbackFunction_p;
        /* Set callback's current node pointer  */
        CurrentNode_p = (U32*)GET_CURRENT_NODE_PTR(Message.ChannelNumber, DeviceNo);
#endif

        UserApplicationData_p =  (void *)stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].ApplicationData_p;
        
#if defined (STFDMA_NAND_SUPPORT)    
		if((CallbackReason == STFDMA_NOTIFY_NAND_READ_ERROR) && (UserApplicationData_p != NULL))
		{
			UserApplicationData_p = (void*)GET_REG(NAND_PAGE_ERROR, DeviceNo);
		}
#endif

        /* Set callback's number of bytes transfered */
        NodeBytesTransfered =  GET_BYTE_COUNT(Message.ChannelNumber, DeviceNo);

        if ( (CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED) ||
             (CallbackReason == STFDMA_NOTIFY_TRANSFER_COMPLETE) 
#if defined (STFDMA_NAND_SUPPORT)  
           ||(CallbackReason == STFDMA_NOTIFY_NAND_READ_ERROR) 
#endif
#if defined(STFDMA_MPX_SUPPORT)
        || (stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX && CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING) 
        || (stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX && CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED)         
#endif
             )
        {
            /* Free the DREQ check flag once the transfer is completed.  */
#if defined (ST_5517)
            if(stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].Pool != STFDMA_PES_POOL && stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].Pool != STFDMA_SPDIF_POOL)
#else
            if(stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].Pool != PoolMapping[STFDMA_PES_POOL] && stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].Pool != PoolMapping[STFDMA_SPDIF_POOL])
#endif
            {			  			   
			     /*Since the transfer is complete reset PacedSignal to NONE for the channel for which the call back was received*/
			     stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Message.ChannelNumber].PacedSignalUsed = STFDMA_REQUEST_SIGNAL_NONE;
				/*Additional handling for 7109+498 in 32 bit mode required or will simple mapping do ?*/					     
#if defined (STFDMA_LEGACY_CODE)
		       Node_Vp = (STFDMA_Node_t*)GET_CURRENT_NODE_PTR(Message.ChannelNumber, DeviceNo);
#if defined (STFDMA_MPX_SUPPORT)
				if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
				{
					/*Node should reside in 498 shared LMI memory*/
			    	Node_Vp = (STFDMA_Node_t*)GET_MPX_ESTB_SHARED_VADDRESS(Node_Vp);
			    }
			    else
				 {
#endif /*STFDMA_MPX_SUPPORT*/
			        
#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
			    	Node_Vp = (void*)ST40_NOCACHE_NOTRANSLATE(Node_Vp);
#else
			    	Node_Vp = (void*)__va((U32)Node_Vp);
#endif
#endif
#if defined (STFDMA_MPX_SUPPORT)
				}
#endif	
#else /*!STFDMA_LEGACY_CODE*/
				Node_Vp = (STFDMA_Node_t*)GET_CURRENT_NODE_PTR(Message.ChannelNumber, DeviceNo);
		/*Additional handling for 7109+498 in 32 bit mode required or will simple mapping do ?*/		
#if defined (STFDMA_MPX_SUPPORT)
				if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
				{
					/*Node should reside in 498 shared LMI memory*/
			    	Node_Vp = (STFDMA_Node_t*)GET_MPX_ESTB_SHARED_VADDRESS(Node_Vp);
			    }
#endif /*STFDMA_MPX_SUPPORT*/		
#endif
#if defined (ST_498)
				/*For F/W workaround for free running transfers over MPX (assuming that STFDMA on eCM will handle all transfers over MPX)*/
				/*Reset RequestConf bit for the signal to make destination non Incrementing*/
				/*Potential issue: will have to rememebr previous original value (from array used in Init) in case same code to be used in other devices*/
				if (	!(Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS0
						||Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS1
						||Node_Vp->NodeControl.PaceSignal == STFDMA_REQUEST_SIGNAL_SWTS2) 
					)
				{ 					
					if(ST_NO_ERROR != STFDMA_SetRequestLineIncrement(Node_Vp->NodeControl.PaceSignal, FALSE, DeviceNo))
					{
						Error = TRUE;
					}
					/*Reset the bit corresponding to the psuedo request signal in DREQ_OVERRIDE*/
					stfdma_SetDreqOverride(Node_Vp->NodeControl.PaceSignal, FALSE, DeviceNo);
				}			
#endif
#if defined (STFDMA_MPX_SUPPORT)						
				/*STFDMA_MPX_SUPPORT_TODO  32 bit port needed here*/
					if(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX)
					{
							if(((STFDMA_MCHIRxNode_t *)Node_Vp)->Extended == STFDMA_EXTENDED_NODE && ((STFDMA_MCHIRxNode_t *)Node_Vp)->Type == STFDMA_EXT_NODE_MCHI)
							{
								*((U32 *)UserApplicationData_p) = (U32)GET_REG(MCHI_BYTES_ALL_NODES, DeviceNo);	   
								SET_REG(MCHI_BYTES_ALL_NODES, 0, DeviceNo); /*Reset*/
							}	
					}		
#endif	                			               
            }

	    /* In case of Generic DMA transfert, the transfert ID must not be cleared */
#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
            if (!GET_GENERIC_DMA(Message.ChannelNumber, DeviceNo))
            {
                CLEAR_TRANSFER_ID(Message.ChannelNumber, DeviceNo);
            }
#else
#if defined (STFDMA_MPX_SUPPORT)
			if(!(stfdma_ControlBlock_p[DeviceNo]->DeviceNumber == STFDMA_MPX && 
				(CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING || CallbackReason == STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED)))
			{
#endif
            CLEAR_TRANSFER_ID(Message.ChannelNumber, DeviceNo);
#if defined (STFDMA_MPX_SUPPORT)
			}
#endif            
#endif

            /* Give up the channel if not locked:  Users can modify the lock status of a channel from another thread,
               so semaphore protection is necessary */
            ENTER_CRITICAL_SECTION(DeviceNo);
            {
                if(!IS_LOCKED(Message.ChannelNumber, DeviceNo))
                {
                    GiveUpChannel(Message.ChannelNumber, DeviceNo);
                }
            }
            LEAVE_CRITICAL_SECTION(DeviceNo);
        }

        if (STFDMA_NO_CALLBACK != UserCallbackFunction_p)
        {
#if defined(STTBX_PRINT)
            if (UserCallbackFunction_p == NULL)
            {
                STTBX_Print(("Illegal Callback routine\n"));
                continue;
            }
#endif

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
            if (GET_GENERIC_DMA(Message.ChannelNumber, DeviceNo))
            {
                UserLinuxCallbackFunction_p( UserApplicationData_p );
            }
            else
            {
                UserCallbackFunction_p( TransferId, CallbackReason, CurrentNode_p, NodeBytesTransfered, Error,
                     UserApplicationData_p, InterruptTime );
            }
#else
            /* Call user's callback function with parameters loaded and application data ptr */
            UserCallbackFunction_p( TransferId, CallbackReason, CurrentNode_p, NodeBytesTransfered, Error,
                     UserApplicationData_p, InterruptTime );
#endif

        }
    } /* end of while 1 loop */

    STOS_TaskExit(pData);
#if defined (ST_OSLINUX)
    return ;
#endif
}

/****************************************************************************
Name         : GetNextFreeChannel
Description  : Obtains free channel index then updates the index.
Parameters   : none
Return Value : -1 : No free channels.
               Channel index.
****************************************************************************/
static U32 GetNextFreeChannel(int PoolId, STFDMA_Block_t DeviceNo)
{
    U32 Channel = NO_FREE_CHANNELS;

    /* Must be thread safe to ensure integrity of internal structures.
     * There is nothing to stop the application calling STFDMA_UnlockChannel
     * from interrupt context. Therefore, interrupt lock must be used to
     * protect the channel usage updates.
     */
    STOS_InterruptLock();
    if (stfdma_ControlBlock_p[DeviceNo]->FreeChannelIndex[PoolId] != NO_FREE_CHANNELS)
    {
        /* Obtain the channel index to return */
        Channel = stfdma_ControlBlock_p[DeviceNo]->FreeChannelIndex[PoolId];

        /* Refresh the next free channel index */
        stfdma_ControlBlock_p[DeviceNo]->FreeChannelIndex[PoolId] =
            stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Channel].NextFreeChannel;
    }
    STOS_InterruptUnlock();

    STTBX_Print(("Channel : %i, FreeChannelIndex : %i\n",Channel,
                 stfdma_ControlBlock_p[DeviceNo]->FreeChannelIndex[PoolId]));

    return Channel;
}

/****************************************************************************
Name         : GiveUpChannel
Description  : Returns the specified channel to the free channel pool.
Parameters   : Channel :
Return Value :

  !! Called from the Callback Manager with limited stack
  !! Don't use STTBX_Print() with out increasing the stack

****************************************************************************/
static void GiveUpChannel(U32 Channel, STFDMA_Block_t DeviceNo)
{
    /* Must be thread safe to ensure integrity of internal structures.
     * There is nothing to stop the application calling STFDMA_UnlockChannel
     * from interrupt context. Therefore, interrupt lock must be used to
     * protect the channel usage updates.
     */
    int PoolId = stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Channel].Pool;

    STOS_InterruptLock();

    /* Add channel to free back into the list of free channels */
    stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Channel].NextFreeChannel =
            stfdma_ControlBlock_p[DeviceNo]->FreeChannelIndex[PoolId];

    /* Make current free channel reference channel to be freed */
    stfdma_ControlBlock_p[DeviceNo]->FreeChannelIndex[PoolId] = Channel;

    STOS_InterruptUnlock();

    return;
}

/****************************************************************************
Name         : CreateTransferId
Description  : Generates a unique transfer id containing the channel id
Parameters   : Channel : Channel index on which transfer is running
Return Value : TransferId value
****************************************************************************/
static U32 CreateTransferId(U32 Channel, U8 DeviceNo)
{
    U32 Id = 0;

    /* Create unique Id containing channel index */
    stfdma_ControlBlock_p[DeviceNo]->TransferTotal++;

    /* When total wraps, ensure set back to 1 */
    if (stfdma_ControlBlock_p[DeviceNo]->TransferTotal == 0)
    {
        stfdma_ControlBlock_p[DeviceNo]->TransferTotal = 1;
    }
    Id = ((stfdma_ControlBlock_p[DeviceNo]->TransferTotal << 24) | DeviceNo << 16 | Channel);
    return Id;
}

/****************************************************************************
Name         : GetChannelFromTransferId
Description  : Converts the given transfer id into a channel index
Parameters   : TransferId
Return Value : Channel index value
****************************************************************************/
static U32 GetChannelFromTransferId( U32 TransferId )
{
    return (TransferId & TRANSFER_ID_CHANNEL_MASK);
}

/****************************************************************************
Name         : GetDeviceFromTransferId
Description  : Converts the given transfer id into a Device index
Parameters   : TransferId
Return Value : Channel index value
****************************************************************************/
static U32 GetDeviceFromTransferId( U32 TransferId )
{
    return ((TransferId & DEVICE_CHANNEL_MASK) >> 16);
}

/****************************************************************************
Name         : IsChannelBusy
Description  : Tests the given channel to see if in use.
Parameters   : Channel index
Return Value : TRUE : Channel is in use
               FALSE : Channel is free
****************************************************************************/
static BOOL IsChannelBusy( U32 Channel, STFDMA_Block_t DeviceNo )
{
    if (GET_TRANSFER_ID(Channel, DeviceNo) == NO_TRANSFER)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

/****************************************************************************
Name         : stfdma_IsNodePauseEnabled
Description  : Obtains channel node ptr from channel index, dereferences into
               main memory and tests the application node condifguration for
               the node pause enable control.
Parameters   : Channel
Return Value : TRUE : Node pause is enabled
               FALSE : Node pause not enabled.
****************************************************************************/
BOOL stfdma_IsNodePauseEnabled(U32 Channel, STFDMA_Block_t DeviceNo)
{

    BOOL NodeCompletePause = FALSE;
    U32		NodeControlWord = 0;
	NodeControlWord = GET_NODE_CONTROL(Channel, DeviceNo);
	if(NodeControlWord & 0x40000000)
	{
		NodeCompletePause = TRUE;
	}	
	else
	{
		NodeCompletePause = FALSE;
	}	
    return NodeCompletePause;
}

/****************************************************************************
Name         : stfdma_GetNextNodePtr
Description  : Obtains channels node ptr, dereferences in to main memory,
               obtains Next node ptr from user node structure.
Parameters   : Channel index
Return Value : Channels next node ptr
****************************************************************************/
U32 stfdma_GetNextNodePtr(U32 Channel, STFDMA_Block_t DeviceNo)
{

	U32 NextPtr = 0 ;
  	NextPtr = GET_NEXT_NODE_PTR(Channel, DeviceNo);
    return NextPtr;
}


/****************************************************************************
Name         : IsTransferInProgress()
Description  : Checks all channel and returns as soon an active transfer is
               found.
Parameters   :
Return Value : TRUE transfers in progress
               FALSE no active transfers
****************************************************************************/
static BOOL IsTransferInProgress(U32 Device)
{
    U32 i =0;

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    for (i=0; i< STFDMA_MaxChannels; i++)
#else
    for (i=0; i< NUM_CHANNELS; i++)
#endif
    {
        if (GET_TRANSFER_ID(i, Device) != 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

#ifndef STFDMA_NO_PARAMETER_CHECK
/****************************************************************************
Name         : IsOutsideRange()
Description  : Checks if U32 formal parameter 1 is outside the range of
               bounds (U32 formal parameters 2 and 3).
Parameters   : Value for checking, followed by the two bounds, preferably
               the lower bound followed by the upper bound.
Return Value : TRUE  if parameter is outside given range
               FALSE if parameter is within  given range
****************************************************************************/
static BOOL IsOutsideRange( U32 ValuePassed,
                            U32 LowerBound,
                            U32 UpperBound )
{
    U32 Temp;

    /* Ensure bounds are the correct way around.
     * If not, bounds require swapping.
     */
    if (UpperBound < LowerBound)
    {
        Temp       = LowerBound;
        LowerBound = UpperBound;
        UpperBound = Temp;
    }

    /* Check within bounds */
    if ((ValuePassed >= LowerBound) &&
        (ValuePassed <= UpperBound))
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}
#endif

/****************************************************************************
Message queue implementation - this is a single instance implementation only
and no checks are performed.  A second attempt to initialise will corrupt the
message queue.
****************************************************************************/

/****************************************************************************
Name         : UpdateIndex
Description  : Decrements/wraps producer or consumer indices
Parameters   : *index: Ptr to producer or consumer indix
Return Value :
****************************************************************************/
void UpdateIndex (S32 * index)
{

    (*index)--;

    /* Need to wrap? */
    if (*index<0)
    {
        /* Set index to end of arry */
        *index = MESSAGE_ARRAY_SIZE-1;
    }
}

/****************************************************************************
Name         : InitMessages
Description  : Initialises the messaging infrastructure
Parameters   : Message - Pointer to the start of the message queue
               Partition - Partition from which to allocate memory
               Size - Number of message elements to allocate as part of the queue
Return Value : TRUE - memory was allocated successfully
               FALSE - memory _allocate failed
****************************************************************************/
static BOOL InitMessages(U32 Size, STFDMA_Block_t DeviceNo)
{
    /* Set up exclusive access semaphore such that first wait succeeds. */

    stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem = STOS_SemaphoreCreateFifo(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, 1);

    STOS_SemaphoreWait(stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem);
    {
        /* Set up message sent semaphore such that first wait fails.*/
        stfdma_ControlBlock_p[DeviceNo]->MessageReadySem = STOS_SemaphoreCreateFifo(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, 0);

        /* Allocate memory for message array */
        stfdma_ControlBlock_p[DeviceNo]->MessageQueue = STOS_MemoryAllocate(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, sizeof(MessageElement_t)*Size);

        /* Memory allocate failed? */
        if (stfdma_ControlBlock_p[DeviceNo]->MessageQueue == NULL)
        {
            /* Clear up semaphores and exit indicating failure */
            STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem);
            STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->MessageReadySem);
            return FALSE;
        }

        /* Initialise variables */
        stfdma_ControlBlock_p[DeviceNo]->ProducerIndex = stfdma_ControlBlock_p[DeviceNo]->ConsumerIndex = 0;
        stfdma_ControlBlock_p[DeviceNo]->QueueFull = FALSE;
        stfdma_ControlBlock_p[DeviceNo]->QueueEmpty = TRUE;
    }

    STOS_SemaphoreSignal(stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem);
    return TRUE;
}

/****************************************************************************
Name         : TermMessages
Description  : Removes message system infrastructure
Parameters   : Message - Pointer to the start of the message queue
               Partition - Partition from which to deallocate memory
Return Value :
****************************************************************************/
static void TermMessages(STFDMA_Block_t DeviceNo)
{
    /* Clear up semaphores */
    STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem);
    STOS_SemaphoreDelete(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->MessageReadySem);

    /* Deallocate memory */
    STOS_MemoryDeallocate(stfdma_ControlBlock_p[DeviceNo]->DriverPartition_p, stfdma_ControlBlock_p[DeviceNo]->MessageQueue);
}

/****************************************************************************
Name         : SendExitMessage
Description  : Sends a termination message to tasks with active ReceiveMessage()
                calls.  This is needed prior to a call to TermMessages to ensure
                that message infrastructure (such as semaphores) are not being used
                during an attempt to delete them.
Parameters   : NumberConsumers - The number of active ReceiveMessage() calls
                                throughout the system.
Return Value :
****************************************************************************/
static void SendExitMessage(STFDMA_Block_t DeviceNo)
{
    U32 i;

    /* Signal a semaphore for each consumer of messages */
    for(i=0; i<stfdma_ControlBlock_p[DeviceNo]->NumberCallbackTasks; i++ )
    {
        MessageElement_t Message;

        Message.InterruptType = EXIT_MESSAGE;
        stfdma_SendMessage(&Message, DeviceNo);
    }
}


/****************************************************************************
Name         : stfdma_SendMessage.  It is assumed that only one thread is at any
                            time attempting to sendmessages.
Description  : Adds a message to the message queue.
Parameters   : Message - the message to send
Return Value :
****************************************************************************/
void stfdma_SendMessage(MessageElement_t * Message, STFDMA_Block_t DeviceNo)
{
    memcpy(&stfdma_ControlBlock_p[DeviceNo]->MessageQueue[stfdma_ControlBlock_p[DeviceNo]->ProducerIndex], Message, sizeof(MessageElement_t));

    UpdateIndex(&stfdma_ControlBlock_p[DeviceNo]->ProducerIndex);
    STOS_SemaphoreSignal(stfdma_ControlBlock_p[DeviceNo]->MessageReadySem);
    return;
}


/****************************************************************************
Name         : ReceiveMessage.  The message queue is seamphore-protected to
                allow multiple reads.
Description  : Acquires a message from the message queue.
Parameters   : Message - a pointer to a
Return Value :

  !! Called from the Callback Manager with limited stack
  !! Don't use STTBX_Print() with out increasing the stack

****************************************************************************/
static int ReceiveMessage(MessageElement_t * Message, STFDMA_Block_t DeviceNo)
{	
    STOS_SemaphoreWait(stfdma_ControlBlock_p[DeviceNo]->MessageReadySem);
    STOS_SemaphoreWait(stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem); /* protect reads from queue */
    memcpy (Message, &stfdma_ControlBlock_p[DeviceNo]->MessageQueue[stfdma_ControlBlock_p[DeviceNo]->ConsumerIndex], sizeof(MessageElement_t));	
    UpdateIndex(&stfdma_ControlBlock_p[DeviceNo]->ConsumerIndex);
    STOS_SemaphoreSignal(stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem);
    return 0;
}


#if defined (ST_498)
/*PIO configuration on ST_498*/
void stfdma_SetPIOBitConfiguration(char PIONumber, char BitNumber)
{
	int         PIOBaseAddress = 0;
	switch (PIONumber)
	{
		case 0 : PIOBaseAddress = PIO_0_BASE_ADDRESS; break;
		case 1 : PIOBaseAddress = PIO_1_BASE_ADDRESS; break;
		case 2 : PIOBaseAddress = PIO_2_BASE_ADDRESS; break;
		case 3 : PIOBaseAddress = PIO_3_BASE_ADDRESS; break;
		case 4 : PIOBaseAddress = PIO_4_BASE_ADDRESS; break;
		default: break;
	}  
	STSYS_WriteRegDev32LE((void*)(PIOBaseAddress+0x28),  (0x1 << BitNumber)); /*PIO_CLEAR_PnC0*/
	STSYS_WriteRegDev32LE((void*)(PIOBaseAddress+0x34),  (0x1 << BitNumber)); /*PIO_SET_PnC1*/
	STSYS_WriteRegDev32LE((void*)(PIOBaseAddress+0x48),  (0x1 << BitNumber)); /*PIO_CLEAR_PnC2*/
}

/*PIO bit set on ST_498*/
void stfdma_SetPIOBit(char PIONumber, char BitNumber)
{
  int   PIOSetAddress;

  switch (PIONumber)
  {
    case 0 : PIOSetAddress = PIO0_OUT_SET; break;
    case 1 : PIOSetAddress = PIO1_OUT_SET; break;
    case 2 : PIOSetAddress = PIO2_OUT_SET; break;
    case 3 : PIOSetAddress = PIO3_OUT_SET; break;
    case 4 : PIOSetAddress = PIO4_OUT_SET; break;
    default: return;
  }
  STSYS_WriteRegDev32LE((void*)(PIOSetAddress), (0x1 << BitNumber));
  return;
}

void  stfdma_SetDreqOverride(U32 RequestLineNo,  BOOL On, STFDMA_Block_t DeviceNo)
{
    U32 DreqControl = 0;
    U32 State = 0;

    STTBX_Print(("stfdma_SetDreqOverride\n"));
	/*Error checks removed as should be handled in previous API calls*/
	 if (On) State = 1;
    DreqControl = GET_REG(DREQ_OVERRIDE, DeviceNo);
    DreqControl &= ~(1 << RequestLineNo);  /*Clear bit at RequestLineNo position*/     
    DreqControl |= (State << RequestLineNo);  /*Set/Reset bit at RequestLineNo position*/     
    SET_REG(DREQ_OVERRIDE, DreqControl, DeviceNo);
}

#endif


#if defined (STFDMA_MPX_SUPPORT)
/*PIO bit reset on eCM from eSTB*/
void stfdma_ResetPIOBit(char PIONumber, char BitNumber)
{
  int         PIOClearAddress;

  switch (PIONumber)
  {
    case 0 : PIOClearAddress = PIO0_OUT_CLEAR; break;
    case 1 : PIOClearAddress = PIO1_OUT_CLEAR; break;
    case 2 : PIOClearAddress = PIO2_OUT_CLEAR; break;
    case 3 : PIOClearAddress = PIO3_OUT_CLEAR; break;
    case 4 : PIOClearAddress = PIO4_OUT_CLEAR; break;
    default: return ;
  }
  STSYS_WriteRegDev32LE((void*)(PIOClearAddress),(0x1 << BitNumber));
  return;
}

#if defined (STFDMA_MPX_USE_EXT_IRQ3)
ST_ErrorCode_t stfdma_RouteInterrupt(void)
{
	U32 Value = 0;

    STTBX_Print(("stfdma_RouteInterrupt start ILC_BASE_ADDRESS = 0x%x\n", ILC_BASE_ADDRESS));
    STTBX_Print(("stfdma_RouteInterrupt start ST7109_CFG_BASE_ADDRESS = 0x%x\n", ST7109_CFG_BASE_ADDRESS));       

	/* 498: Enable FDMA mailbox interrupt */
	Value = STSYS_ReadRegDev32LE(MPX_498_COMMS_ILC_ENABLE0);
	Value |= (1 << MPX_498_ILC3_FDMA_INT_NUMBER);
	STSYS_WriteRegDev32LE(MPX_498_COMMS_ILC_ENABLE0, Value);

	/* 498: Route the FDMA mailbox interrupt to external interrupt #0 */
	STSYS_WriteRegDev32LE(MPX_498_COMMS_ILC_INT_PRIORITY14, 0x8000);
	
	ILCMappedBaseAddr = STOS_MapRegisters(ILC_BASE_ADDRESS, 0x1000, "ILCBase");
	CFGMappedBaseAddr = STOS_MapRegisters(CFG_BASE_ADDRESS, 0x512, "CFGBase");


	/* 7109: Enable external interrupt #3 for wakeup by interrupt generation */
	Value = STSYS_ReadRegDev32LE(ILC3_WAKEUP_EN(ILCMappedBaseAddr));
	Value |= 0x8;
	STSYS_WriteRegDev32LE(ILC3_WAKEUP_EN(ILCMappedBaseAddr), Value);


	/* 7109: Configure the wake-up level for external interrupt #3 as high */
	Value = STSYS_ReadRegDev32LE(ILC3_WAKEUP_ACTIVELEVEL(ILCMappedBaseAddr));
	Value |= 0x8;
	STSYS_WriteRegDev32LE(ILC3_WAKEUP_ACTIVELEVEL(ILCMappedBaseAddr), Value);

	/* 7109: Configure external interrupt #3 to be triggered on a rising edge */
	STSYS_WriteRegDev32LE(ILC3_EXT_MODE(ILCMappedBaseAddr, 3), 0x03);

	/* 7109: Map external interrupt #3 to IRL[3] */
	STSYS_WriteRegDev32LE(ILC3_EXT_PRIORITY(ILCMappedBaseAddr, 3), 0x8006);

	/* 7109: Enable external interrupt #3 */
	Value = STSYS_ReadRegDev32LE(ILC3_SET_EN2(ILCMappedBaseAddr));
	Value |= 0x8;
	STSYS_WriteRegDev32LE(ILC3_SET_EN2(ILCMappedBaseAddr), Value);

	/* 7109: Configure extenal interrupt #3 as input */
	Value = STSYS_ReadRegDev32LE(SYS_CFG10(CFGMappedBaseAddr));
	Value |= 0x8;
	STSYS_WriteRegDev32LE(SYS_CFG10(CFGMappedBaseAddr), Value);

    STTBX_Print(("stfdma_RouteInterrupt end ILCMappedBaseAddr = 0x%x\n", (U32)ILCMappedBaseAddr));	
    STTBX_Print(("stfdma_RouteInterrupt end ILCMappedBaseAddr = 0x%x\n", (U32)CFGMappedBaseAddr));	        
    return ST_NO_ERROR;
}
#else
ST_ErrorCode_t stfdma_RouteInterrupt(const ST_DeviceName_t DeviceName, U32 PIONumber, U32  PIOBit)
{
	STPIO_OpenParams_t OpenParams;
	STPIO_Compare_t    setCmpReg;
	ST_ErrorCode_t ErrorCode = ST_NO_ERROR;

	OpenParams.ReservedBits    = (1 << PIOBit);
	OpenParams.BitConfigure[PIOBit] = STPIO_BIT_INPUT;
	OpenParams.IntHandler = stfdma_PIOInterruptHandler;
	ErrorCode = STPIO_Open(DeviceName, &OpenParams, &PIOHandle);
	if(ErrorCode != ST_NO_ERROR)
	{
		STTBX_Print(("stfdma_RouteInterrupt(): STPIO_Open() = %x\n", ErrorCode));
		return ErrorCode;
	}
	
    setCmpReg.CompareEnable  = (1 << PIOBit);
    setCmpReg.ComparePattern = 0;
    ErrorCode = STPIO_SetCompare(PIOHandle, &setCmpReg);
	if(ErrorCode != ST_NO_ERROR)
	{
		STTBX_Print(("stfdma_RouteInterrupt(): STPIO_SetCompare() = %x\n", ErrorCode));
		return ErrorCode;
	}
    ErrorCode = STPIO_SetCompareClear(PIOHandle, STPIO_COMPARE_CLEAR_MANUAL);
	if(ErrorCode != ST_NO_ERROR)
	{
		STTBX_Print(("stfdma_RouteInterrupt(): STPIO_SetCompareClear() = %x\n", ErrorCode));
		return ErrorCode;
	}	
	return ErrorCode;
	
}
#endif
#endif


#if defined (STFDMA_DEBUG_SUPPORT)
void STFDMA_DisplayStatus (STFDMA_Block_t DeviceNo)
{
    U32 Index = 0;
    int ActualChannel = -1;
    char PoolName[30] = "";

    STTBX_Print(("\n*********************************************************"));
    STTBX_Print(("\n                      FDMA Block %u                      ", DeviceNo));
    STTBX_Print(("\n*********************************************************"));

    STTBX_Print(("\n\nCount (MessageQueue):               %x", semaphore_value(stfdma_ControlBlock_p[DeviceNo]->MessageQueueSem) ));
    STTBX_Print(("\nCount (MessageReady):               %x", semaphore_value(stfdma_ControlBlock_p[DeviceNo]->MessageReadySem) ));

    STTBX_Print(("\n\nProducerIndex (MessageReady):       %d", stfdma_ControlBlock_p[DeviceNo]->ProducerIndex));
    STTBX_Print(("\nConsumerIndex (MessageReady):       %d", stfdma_ControlBlock_p[DeviceNo]->ConsumerIndex));
    STTBX_Print(("\nQueueFull:                          %s", (stfdma_ControlBlock_p[DeviceNo]->QueueFull == TRUE)? "TRUE" : "FALSE"));
    STTBX_Print(("\nQueueEmpty:                         %s", (stfdma_ControlBlock_p[DeviceNo]->QueueEmpty == TRUE)? "TRUE" : "FALSE"));

    STTBX_Print(("\n\nCommand mailbox: Status             0x%x", GET_REG(INTR, DeviceNo) ));
    STTBX_Print(("\nCommand mailbox: Set                0x%x", GET_REG(INTR_SET, DeviceNo) ));
    STTBX_Print(("\nCommand mailbox: Clear              0x%x", GET_REG(INTR_CLR, DeviceNo) ));
    STTBX_Print(("\nCommand mailbox: Mask               0x%x", GET_REG(INTR_MASK, DeviceNo) ));

    STTBX_Print(("\n\nInterrupt mailbox: Status           0x%x", GET_REG(STATUS, DeviceNo) ));
    STTBX_Print(("\nInterrupt mailbox: Set              0x%x", GET_REG(STATUS_SET, DeviceNo) ));
    STTBX_Print(("\nInterrupt mailbox: Clear            0x%x", GET_REG(STATUS_CLR, DeviceNo) ));
    STTBX_Print(("\nInterrupt mailbox: Mask             0x%x", GET_REG(STATUS_MASK, DeviceNo) ));

    for(Index = 0; Index < NUM_CHANNELS; Index++)
    {
        STTBX_Print(("\n\n********************** Channel %u **********************\n", Index));

        for(ActualChannel = 0;
#if defined (ST_5188)
                               ActualChannel < 5;
#else
                               ActualChannel < 4;
#endif
                                                  ActualChannel++)
            if(PoolMapping[ActualChannel] == stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Pool)
                break;

        switch(ActualChannel)
        {
            case STFDMA_DEFAULT_POOL:
                strcpy(PoolName, "STFDMA_DEFAULT_POOL");
                break;
            case STFDMA_PES_POOL:
                strcpy(PoolName, "STFDMA_PES_POOL");
                break;
            case STFDMA_SPDIF_POOL:
                strcpy(PoolName, "STFDMA_SPDIF_POOL");
                break;
            case STFDMA_HIGH_BANDWIDTH_POOL:
                strcpy(PoolName, "STFDMA_HIGH_BANDWIDTH_POOL");
                break;
#if defined (ST_5188)
            case STFDMA_FEI_POOL:
                strcpy(PoolName, "STFDMA_FEI_POOL");
                break;
#endif
        }

        STTBX_Print(("\nPool:                               %s", PoolName));

        STTBX_Print(("\n\nTransferId:                         0x%x", stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].TransferId));
        STTBX_Print(("\nBlocking:                           %s", (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Blocking == TRUE)? "TRUE" : "FALSE"));
        STTBX_Print(("\nLocked:                             %s", (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Locked == TRUE)? "TRUE" : "FALSE"));
        STTBX_Print(("\nPaused:                             %s", (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Paused == TRUE)? "TRUE" : "FALSE"));
        STTBX_Print(("\nIdle:                               %s", (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Idle == TRUE)? "TRUE" : "FALSE"));
        STTBX_Print(("\nAborting:                           %s", (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Abort== TRUE)? "TRUE" : "FALSE"));
        STTBX_Print(("\nError:                              %s", (stfdma_ControlBlock_p[DeviceNo]->ChannelTable[Index].Error == TRUE)? "TRUE" : "FALSE"));

        STTBX_Print(("\n\nTotal interrupts:                   %u", stfdma_Status[DeviceNo]->Channel[Index].InterruptsTotal));
        STTBX_Print(("\nBlocking:                           %u", stfdma_Status[DeviceNo]->Channel[Index].InterruptsBlocking));
        STTBX_Print(("\nNon-blocking:                       %u", stfdma_Status[DeviceNo]->Channel[Index].InterruptsNonBlocking));
        STTBX_Print(("\nMissed:                             %u", stfdma_Status[DeviceNo]->Channel[Index].InterruptsMissed));
        STTBX_Print(("\nSemaphore for blocking received:    %u", stfdma_Status[DeviceNo]->Channel[Index].InterruptBlockingSemaphoreReceived));
        STTBX_Print(("\nCallbacks:                          %u", stfdma_Status[DeviceNo]->Channel[Index].Callbacks));

        STTBX_Print(("\n\nCommand/Status:                     0x%x", GET_CURRENT_NODE_PTR(Index, DeviceNo) ));
        STTBX_Print(("\nBytes remaining:                    %u", GET_BYTE_COUNT(Index, DeviceNo) ));

        STTBX_Print(("\n\n"));
     }
 }
#endif

/* eof */
