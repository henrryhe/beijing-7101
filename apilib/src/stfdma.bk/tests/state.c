/*****************************************************************************
File Name   : state.c

Description : STFDMA state test case source file

Copyright (C) 2002 STMicroelectronics

History     :

Reference   : STFDMA Component Test Plan
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

/*#define DEBUG               1
#define STFDMA_TEST_VERBOSE 1*/
#define STTBX_PRINT 1   /*Always required on */

#if defined (ST_OSLINUX)
#include <linux/stddef.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/semaphore.h>
#else
#include <string.h>                     /* C libs */
#include "sttbx.h"
#endif

#include "stdevice.h"
#include "stddefs.h"

#include "stfdma.h"                     /* fdma api header file */

#include "fdmatst.h"                    /* test header files */
#include "state.h"

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
extern int Lock_7100_fdma2[NUM_CHANNELS];
#endif

/* Private Variables ------------------------------------------------------ */
static U32 StartTransferCalled=0;
static U32 St02TransferIdAborted=0;

/* TransferIds for cross task usage */
static STFDMA_TransferId_t St02TransferId;
static STFDMA_TransferId_t St03TransferId;
static STFDMA_TransferId_t St04TransferId;
static STFDMA_TransferId_t St05TransferId;

/* General callback semaphores */
static semaphore_t* pAbortSemap;
static semaphore_t* pPausedSemap;
static semaphore_t* pTransferCompleteSemap;
static semaphore_t* pListCompleteSemap;
static semaphore_t* pNodeCompleteSemap;

/* Test case St04 task config */
static tdesc_t *St04TransferTaskDescriptor = NULL;
static U32     *St04TransferTaskStack = NULL;

#if defined (ST_OSLINUX)
static struct task_struct* pSt04TransferTask;
static int  St04TransferTaskFunc(void * Paced);
#else
static task_t* pSt04TransferTask;
static void St04TransferTaskFunc(void * Paced);
#endif

/* Test case St04 semaphore controls */
static semaphore_t* pSt04TestContinue; /* St04 test thread control */
static semaphore_t* pSt04TaskContinue; /* St04 transfer task control */

/* Local constants ------------------------------------------------------ */
#define K (1024)
#define MEG (1024 * K)

#define DEFAULT_RATE (10 * K)

static STOS_Clock_t  WaitTime;

extern partition_t* system_partition;

/* Paced/nonpaced transfer test control */
enum
{
#if defined (STFDMA_NO_PACED_TESTS)
    NON_PACED_TRANSFER = 0,
    TEST_COMPLETE,
    PACED_TRANSFER
#else
    NON_PACED_TRANSFER = 0,
    PACED_TRANSFER,
    TEST_COMPLETE
#endif
};

/* Local Functions -------------------------------------------------------- */

/****************************************************************************
Name         : StGeneralCallback
Description  : Callback function for general state test usage
Parameters   : As defined by the stfdma API for callback functions.
Return Value :
****************************************************************************/
static void Callback
(
    U32     TransferId,
    U32     CallbackReason,
    U32*    CurrentNode_p,
    U32     NodeBytesRemaining,
    BOOL    Error,
    void*   ApplicationData_p,
    STOS_Clock_t InterruptTime
)
{
    STFDMA_Node_t *Node_p;

    if (TransferId == 0)
    {
        VERBOSE_PRINT("Callback: *** ERROR: Invalid transfer id.\n");
        return;
    }

    /* Check bytes remaining has a sensible value */
#if defined (ARCHITECTURE_ST40)
#if defined (ST_OS21)
    Node_p = (STFDMA_Node_t *) ST40_NOCACHE_NOTRANSLATE(CurrentNode_p);
#else
    Node_p = (STFDMA_Node_t*)fdmatst_GetNodeVirtualAddressFromPhysical((dma_addr_t)CurrentNode_p);
#endif
#else
    Node_p = (STFDMA_Node_t *) CurrentNode_p;
#endif

    if (NodeBytesRemaining > Node_p->NumberBytes)
    {
        VERBOSE_PRINT_DATA("Callback: *** ERROR: NodeBytesRemaining has illegal value (%d)\n",NodeBytesRemaining);
    }

    /* Check transferId against expected transfers */
    if (TransferId == St02TransferId)
    {
        /* Expected reasons for callback...*/
        switch (CallbackReason)
        {
            case STFDMA_NOTIFY_TRANSFER_ABORTED:
                VERBOSE_PRINT("Callback: Transfer aborted.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                St02TransferIdAborted++;
                STOS_SemaphoreSignal(pAbortSemap);
                break;

            case STFDMA_NOTIFY_TRANSFER_COMPLETE:
                VERBOSE_PRINT("Callback: Transfer complete.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pTransferCompleteSemap);
                break;

            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
                VERBOSE_PRINT("Callback: Node complete and paused.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pPausedSemap);
                break;

            default :
                VERBOSE_PRINT("Callback: ERROR: Unexpected callback reason.\n");
                break;
        }
    }
    else if (TransferId == St03TransferId)
    {
        /* Expected reasons for callback...*/
        switch (CallbackReason)
        {
            case STFDMA_NOTIFY_TRANSFER_ABORTED:
                VERBOSE_PRINT("Callback: Transfer aborted.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pAbortSemap);
                break;

            case STFDMA_NOTIFY_TRANSFER_COMPLETE:
                VERBOSE_PRINT("Callback: Transfer complete.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pTransferCompleteSemap);
                break;

            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
                VERBOSE_PRINT("Callback: Node complete and paused.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pPausedSemap);
                break;

            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_CONTINUING:
                /* do nothing */
                break;

            default :
                VERBOSE_PRINT_DATA("Callback: ERROR: Unexpected callback reason (0x%x).\n",CallbackReason);
                break;
        }
    }
    else if (TransferId == St05TransferId)
    {
        /* Expected reasons for callback...*/
        switch (CallbackReason)
        {
            case STFDMA_NOTIFY_TRANSFER_ABORTED:
                VERBOSE_PRINT("Callback: Transfer aborted.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pAbortSemap);
                break;

            case STFDMA_NOTIFY_NODE_COMPLETE_DMA_PAUSED:
                VERBOSE_PRINT("Callback: Node complete and paused.\n");
                VERBOSE_PRINT_DATA("Callback: CurrentNode_p: 0x%x\n",CurrentNode_p);
                VERBOSE_PRINT_DATA("Callback: NodeBytesRemaining: %d\n",NodeBytesRemaining);
                STOS_SemaphoreSignal(pPausedSemap);
                break;

            default :
                VERBOSE_PRINT("Callback: ERROR: Unexpected callback reason.\n");
                break;
        }
    }
    else /* Unknown transferId, something wrong....*/
    {
        VERBOSE_PRINT("Callback: ERROR: Unrecognised TransferId\n");
    }

    /* Display messgae if error set */
    if (Error)
    {
        VERBOSE_PRINT("Callback: Error flag set.\n");
    }

    /* Expect no data back */
    if (ApplicationData_p != NULL)
    {
        VERBOSE_PRINT_DATA("Callback: ***ERROR: ApplicationData_p unexpectedly set (0x%x)\n",
                      ApplicationData_p);
        fdmatst_SetSuccessState(FALSE);
    }
}

/****************************************************************************
Name         : CaseSt01
Description  : Testcase St01.
               Driver in "start/end" state.
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt01(int arg)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_TransferId_t         TransferId = 0;
    STFDMA_InitParams_t         InitParams;
    STFDMA_TransferStatus_t     TransferStatus;
    STFDMA_ChannelId_t          ChannelId = 0;
    STFDMA_ContextId_t          ContextId = 0;

    STTBX_Print(("Check 'start' state....\n"));

    /* Driver startup should ensure entry to start condition */
    VERBOSE_PRINT("Check statup condition is correct....\n");

    /* Check illegal calls....*/

    /* Term, expect error unknown device */
    VERBOSE_PRINT("Calling STFDMA_Term, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    fdmatst_ErrorReport("STFDMA_Term(1)", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* StartTransfer, expect error  */
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT("Calling STFDMA_StartTransfer, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &TransferId);
    fdmatst_ErrorReport("STFDMA_StartTransfer()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* AbortTransfer, expect error  */
    VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_AbortTransfer(TransferId);
    fdmatst_ErrorReport("STFDMA_AbortTransfer()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

#if !(defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710) || defined (ST_5517))
    /* FlushTransfer, expect error  */
    VERBOSE_PRINT("Calling STFDMA_FlushTransfer, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_FlushTransfer(TransferId);
    fdmatst_ErrorReport("STFDMA_FlushTransfer()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);
#endif

    /* ResumeTransfer, expect error */
    VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ResumeTransfer(TransferId);
    fdmatst_ErrorReport("STFDMA_ResumeTransfer()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* GetTransferStatus, expect error */
    VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_GetTransferStatus(TransferId, &TransferStatus);
    fdmatst_ErrorReport("STFDMA_GetTransferStatus()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* UnlockChannel, expect error */
    VERBOSE_PRINT("Calling STFDMA_UnlockChannel, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_UnlockChannel(ChannelId, STFDMA_1);
    fdmatst_ErrorReport("STFDMA_UnlockChannel()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_AllocateContext, expect error */
    VERBOSE_PRINT("Calling STFDMA_AllocateContext, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_AllocateContext(&ContextId);
    fdmatst_ErrorReport("STFDMA_AllocateContext()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_DeallocateContext, expect error */
    VERBOSE_PRINT("Calling STFDMA_DeallocateContext, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_DeallocateContext(ContextId);
    fdmatst_ErrorReport("STFDMA_DeallocateContext()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextGetSCList, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextGetSCList, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextGetSCList(ContextId, NULL, NULL, NULL);
    fdmatst_ErrorReport("STFDMA_ContextGetSCList()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextSetSCList, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextSetSCList, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextSetSCList(ContextId, NULL, 0);
    fdmatst_ErrorReport("STFDMA_ContextSetSCList()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextGetSCState, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextGetSCState, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextGetSCState(ContextId, NULL, 0);
    fdmatst_ErrorReport("STFDMA_ContextGetSCState()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextSetSCState, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextSetSCState, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextSetSCState(ContextId, NULL, 0);
    fdmatst_ErrorReport("STFDMA_ContextSetSCState()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextSetESBuffer, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextSetESBuffer, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextSetESBuffer(ContextId, NULL, 0);
    fdmatst_ErrorReport("STFDMA_ContextSetESBuffer()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextSetESReadPtr, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextSetESReadPtr, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextSetESReadPtr(ContextId, NULL);
    fdmatst_ErrorReport("STFDMA_ContextSetESReadPtr()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextSetESWritePtr, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextSetESWritePtr, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextSetESWritePtr(ContextId, NULL);
    fdmatst_ErrorReport("STFDMA_ContextSetESWritePtr()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextGetESReadPtr, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextGetESReadPtr, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextGetESReadPtr(ContextId, NULL);
    fdmatst_ErrorReport("STFDMA_ContextGetESReadPtr()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* STFDMA_ContextGetESWritePtr, expect error */
    VERBOSE_PRINT("Calling STFDMA_ContextGetESWritePtr, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_ContextGetESWritePtr(ContextId, NULL, NULL);
    fdmatst_ErrorReport("STFDMA_ContextGetESWritePtr()", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* Test state exit method 1 (Init)...*/
    VERBOSE_PRINT("\nCheck start exit: Init....\n");

    /* Init, expect no error */
    VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_1;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

    /* Try to repeat state change ...*/
    VERBOSE_PRINT("Check in expected state....\n");

    /* Init, expect error already INITIALIZED */
    VERBOSE_PRINT("Calling STFDMA_Init again, expect ALREADY_INITIALIZED.\n");
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("STFDMA_Init()", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

    /* Test state entry criteria.... */
    VERBOSE_PRINT("\nCheck start entry: Term....\n");

    /* Term, expect no error */
    VERBOSE_PRINT("Calling STFDMA_Term, expect NO_ERROR.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    fdmatst_ErrorReport("STFDMA_Term(2)", ErrorCode, ST_NO_ERROR);

    VERBOSE_PRINT("Check in expected state....\n");

    VERBOSE_PRINT("Calling STFDMA_Term, expect STFDMA_ERROR_NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    fdmatst_ErrorReport("STFDMA_Term(3)", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

    /* nothing to do, already in start condition */
    VERBOSE_PRINT("Clean up done.\n");
}

/****************************************************************************
Name         : CaseSt02
Description  : Testcase St02
               Driver is in ready  condition for paced and non-paced transfers
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt02(int arg)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node_p;
    STFDMA_TransferStatus_t     TransferStatus;
    U8                          TransferType;
    BOOL                        PacedControl;
    U32                         PaceSignal;

    /* Perform test for paced and non paced transfers...*/
    for (TransferType = NON_PACED_TRANSFER; TransferType != TEST_COMPLETE; TransferType++)
    {
        if (TransferType == NON_PACED_TRANSFER)
        {
            STTBX_Print(("Check 'ready' state (non paced transfer)....\n\n"));
            PacedControl = FALSE;
            PaceSignal = STFDMA_REQUEST_SIGNAL_NONE;
        }
        else
        {
            STTBX_Print(("Check 'ready' state (paced transfer)....\n\n"));
            PacedControl = TRUE;
            PaceSignal   = FDMA_TEST_ACTIVE_DREQ;
        }

        /* Test "ready" state entry method 1 (Init call)....  */
        VERBOSE_PRINT("Check state entry: Init call....\n");

        /* Init, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
        InitParams.DeviceType = FDMA_TEST_DEVICE;
        InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
        InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
        InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
        InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
        InitParams.NumberCallbackTasks = 1;
        InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
        InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
        InitParams.FDMABlock = STFDMA_1;
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

        /* Check illegal calls....*/
        VERBOSE_PRINT("Check in expected state....\n");

        /* GetTransferStatus, expect error */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St02TransferId,&TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* AbortTransfer, expect error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

#if !(defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710) || defined (ST_5517))
        /* FlushTransfer, expect error */
        VERBOSE_PRINT("Calling STFDMA_FlushTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_FlushTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);
#endif

        /* ResumeTransfer, expect error*/
        VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_ResumeTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Init, expect already initialised */
        VERBOSE_PRINT("Calling STFDMA_Init, expect ALREADY_INITIALIZED.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

        /* Test "ready" state exit method 1 (unforced term).... */
        VERBOSE_PRINT("\nCheck state exit: Term, unforced....\n");

        /* Term, unforced, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Term (unforced), expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        VERBOSE_PRINT("Check in expected state....\n");

        /* Test not in "ready" with Term call again, expect error */
        VERBOSE_PRINT("Calling STFDMA_Term (unforced), expect NOT_INITIALIZED.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

        /* Test "ready" state entry method 2 (transfer abort).... */
        VERBOSE_PRINT("\nCheck state entry: abort transfer....\n");

        /* Init, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

        /* Setup an endless non-blocking transfer: No pause, no notify  */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, 10 /*DEFAULT_RATE*/);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        /* (Indirectly tests "ready" state exit method 2 (StartTransfer)) */
        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout = 0;
        TransferParams.CallbackFunction = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check not in "ready" state with call to Term, unforced. Expect error */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_Term (unforced), expect TRANSFER_IN_PROGRESS.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_TRANSFER_IN_PROGRESS);

        /* Abort transfer with call to Abort, expect no error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check abort callback recieved */
        VERBOSE_PRINT("Wait for abort notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* Check in "ready" state with illegal calls, expect error */
        VERBOSE_PRINT("Check in expected state....\n");

        /* GetTransferStatus, expect error */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St02TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* AbortTransfer, expect error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* ResumeTransfer, expect error*/
        VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_ResumeTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Init, expect already initialised */
        VERBOSE_PRINT("Calling STFDMA_Init, expect ALREADY_INITIALIZED.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

        /* clean up memory */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        /* Test "ready" state entry method 3 (transfer complete) */
        VERBOSE_PRINT("\nCheck state entry: transfer complete....\n");

        /* Setup node */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                       FALSE, FALSE, PaceSignal, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p,  DEFAULT_RATE );
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        /* Start a short blocking transfer, expect no error */
        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout = 0;  /* No timeout */
        TransferParams.CallbackFunction = NULL;  /* Blocking, dont care */
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* After transfer completion check in "ready" state with illegal calls, expect errors */
        VERBOSE_PRINT("Check in expected state....\n");
        /* GetTransferStatus, expect error */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St02TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* AbortTransfer, expect error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* ResumeTransfer, expect error*/
        VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_ResumeTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Init, expect already initialised */
        VERBOSE_PRINT("Calling STFDMA_Init, expect ALREADY_INITIALIZED.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

        /* clean up memory */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        /* Test "ready" state entry method 4 (blocking timeout) */
        VERBOSE_PRINT("\nCheck state entry: blocking timeout....\n");

        /* Start a blocking transfer, expect no error */
        VERBOSE_PRINT("Setup transfer nodes.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PaceSignal, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout = 1; /* 1ms */
        TransferParams.CallbackFunction = NULL; /* Blocking, don't care */
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for blocking transfer, 1ms timeout. Expect BLOCKING_TIMEOUT.\n");

        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_BLOCKING_TIMEOUT);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* Check in "ready" state with illegal calls, expect errors */
        VERBOSE_PRINT("Check in expected state....\n");

        /* GetTransferStatus, expect error */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St02TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* AbortTransfer, expect error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* ResumeTransfer, expect error*/
        VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_ResumeTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Init, expect already initialised */
        VERBOSE_PRINT("Calling STFDMA_Init, expect ALREADY_INITIALIZED.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

        /* clean up memory */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        /* Test "ready" state exit method 3 (forced term) */
        VERBOSE_PRINT("\nCheck state entry: Term, forced....\n");

        /* Term, forced. Expect no error */
        VERBOSE_PRINT("Calling STFDMA_Term, forced. Expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check not in "ready" state by call to Term again. Expect error */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_Term, expect NOT_INITIALIZED.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

        /* Clean up */
        St02TransferId = 0;

    }/* end paced control loop */
}

#if !defined (ST_5517) && !defined (STFDMA_NO_PACED_TESTS)
/****************************************************************************
Name         : CaseSt99
Description  : Testcase St02
               Simple paced transfer test
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt99(int Flags)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node_p;
    BOOL                        PacedControl;
    U32                         PaceSignal;

    STTBX_Print(("\nCheck 'ready' state (paced transfer)....\n\n"));
    PacedControl = TRUE;
    PaceSignal   = FDMA_TEST_ACTIVE_DREQ;

    /* Init, expect no error */
    VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_1;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

    /* Setup node */
    VERBOSE_PRINT("Setup transfer node.\n");
    if (Flags&2)
    {
        ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PaceSignal, &Node_p);
    }
    else
    {
        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                       FALSE, FALSE, PaceSignal, &Node_p);
    }
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    VERBOSE_PRINT("Init and Start pace request signal generator\n");
    fdmatst_InitPaceRequestGenerator(Node_p,  DEFAULT_RATE );
    fdmatst_StartPaceRequestGenerator();

    /* Start a short blocking transfer, expect no error */
    TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout   = 0;  /* No timeout */
    TransferParams.CallbackFunction  = ((Flags&3) == 1)?(NULL):(Callback);  /* Blocking, dont care */
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, blocking transfer. Expect no error.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &St02TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    if (Flags&2)
    {
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710)
        VERBOSE_PRINT("Abort the transfer.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
#else
        VERBOSE_PRINT("Flush the transfer.\n");
        ErrorCode = STFDMA_FlushTransfer(St02TransferId);
#endif
    }

    if (TransferParams.CallbackFunction == Callback)
    {
        /* Wait for transfer complete notify */
        VERBOSE_PRINT("Wait for transfer complete notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));

        if (Flags&2)
        {
            /* Flushed transfer */

            if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
            {
                /* semaphore timed out....no notification provided..error */
                VERBOSE_PRINT("*** ERROR: Did not get transfer complete notification within time allowed.\n");
                fdmatst_SetSuccessState(FALSE);
            }
        }
        else
        {
            if (STOS_SemaphoreWaitTimeOut(pTransferCompleteSemap, &WaitTime) != 0)
            {
                /* semaphore timed out....no notification provided..error */
                VERBOSE_PRINT("*** ERROR: Did not get transfer complete notification within time allowed.\n");
                fdmatst_SetSuccessState(FALSE);
            }
        }
    }

    /* Halt request generator for paced transfers */
    VERBOSE_PRINT("Terminating pace request signal generator\n");
    fdmatst_TermPaceRequestGenerator();

    /* clean up memory */
    VERBOSE_PRINT("Cleaning memory space.\n");
    fdmatst_DeallocateNodes();

    VERBOSE_PRINT("Calling STFDMA_Term, expect NOT_INITIALIZED.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    /* Clean up */
    St02TransferId = 0;
}
#endif

/****************************************************************************
Name         : CaseSt03
Description  : Testcase St03
               Transfer in progress (non-blocking)
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt03(int arg)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node_p;
    STFDMA_TransferStatus_t     TransferStatus;
    U8                          TransferType;
    BOOL                        PacedControl;
    U32                         PaceSignal;

    /* Perform test for paced and non paced transfers...*/
    for (TransferType = NON_PACED_TRANSFER; TransferType != TEST_COMPLETE; TransferType++)
    {
        if (TransferType == NON_PACED_TRANSFER)
        {
            STTBX_Print(("Check 'in progress, non-blocking' (non-paced)....\n\n"));
            PacedControl = FALSE;
            PaceSignal = STFDMA_REQUEST_SIGNAL_NONE;
        }
        else
        {
            STTBX_Print(("Check 'in progress, non-blocking' (paced)....\n\n"));
            PacedControl = TRUE;
            PaceSignal   = FDMA_TEST_ACTIVE_DREQ;
        }

        /* Test state entry method 1 (StartTransfer)..  */
        VERBOSE_PRINT("Check state entry: Init call....\n");

        /* Init, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
        InitParams.DeviceType          = FDMA_TEST_DEVICE;
        InitParams.DriverPartition_p   = fdmatst_GetSystemPartition();
        InitParams.NCachePartition_p   = fdmatst_GetNCachePartition();
        InitParams.InterruptNumber     = FDMA_INTERRUPT_NUMBER;
        InitParams.InterruptLevel      = FDMA_INTERRUPT_LEVEL;
        InitParams.NumberCallbackTasks = 1;
        InitParams.BaseAddress_p       = fdmatst_GetBaseAddress();
        InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
        InitParams.FDMABlock           = STFDMA_1;
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

        /*=================== Transfer with abort =================================*/

        /* Start endless non-blocking transfer, expect no error */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        /* (Indirectly tests "ready" state exit method 2 (StartTransfer)) */
        TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout   = 0;
        TransferParams.CallbackFunction  = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check in "progress" state using illegal calls, expect errors.. */
        VERBOSE_PRINT("Check in expected state....\n");

        /* Init, expect error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect ALREADY_INITIALIZED.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

        /* Term, expect error */
        VERBOSE_PRINT("Calling STFDMA_Term (unforced), expect TRANSFER_IN_PROGRESS.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_TRANSFER_IN_PROGRESS);

        /* Test state exit method 1 (Abort).... */
        VERBOSE_PRINT("\nCheck state exit: Abort transfer.\n");

        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for abort notification */
        VERBOSE_PRINT("Wait for abort notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        VERBOSE_PRINT("Check in expected state....\n");

        /* Check not in "progress" state with same call, expect error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_AbortTransfer(St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /*=================== Tranfser with channel flush =============================*/

        /* Start endless non-blocking transfer, expect no error */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        /* (Indirectly tests "ready" state exit method 2 (StartTransfer)) */
        TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout   = 0;
        TransferParams.CallbackFunction  = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710)
        /* Test state exit method 1 (Abort).... */
        VERBOSE_PRINT("\nCheck state exit: Abort channel.\n");

        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St03TransferId);
#else
        /* Test state exit method 1 (Flush).... */
        VERBOSE_PRINT("\nCheck state exit: Flush channel.\n");

        VERBOSE_PRINT("Calling STFDMA_FlushTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_FlushTransfer(St03TransferId);
#endif

        fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR, ST_ERROR_FEATURE_NOT_SUPPORTED));

        if (ErrorCode == ST_NO_ERROR)
        {
            /* Wait for abort notification */
            VERBOSE_PRINT("Wait for abort notification.\n");
            WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
            if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
            {
                /* semaphore timed out....no notification provided..error */
                VERBOSE_PRINT("*** ERROR: Did not get pause notification within time allowed.\n");
                fdmatst_SetSuccessState(FALSE);
            }

            VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
            ErrorCode = STFDMA_GetTransferStatus(St03TransferId, &TransferStatus);
            fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);
        }
        else
        {
            VERBOSE_PRINT("Clean up the transfer.\n");
            ErrorCode = STFDMA_AbortTransfer(St03TransferId);
            fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

            /* Wait for abort notification */
            VERBOSE_PRINT("Wait for abort notification.\n");
            WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
            if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
            {
                /* semaphore timed out....no notification provided..error */
                VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
                fdmatst_SetSuccessState(FALSE);
            }
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        VERBOSE_PRINT("Check in expected state....\n");

        /* Check not in "progress" state with same call, expect error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_AbortTransfer(St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /*=================== Tranfser with node complete pause ====================*/

        /* Test state exit method 2 (node complete pause).... */
        VERBOSE_PRINT("\nCheck state exit: Node complete paused.\n");

        /* Start endless non-blocking transfer with pause, expect no error */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(TRUE, TRUE, TRUE, TRUE, PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        /* (Indirectly tests "ready" state exit method 2 (StartTransfer)) */
        TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout   = 0;
        TransferParams.CallbackFunction  = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer with pause. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for pause to occur */
        VERBOSE_PRINT("Wait for pause notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pPausedSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get pause notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "progress" state  with GetTransferStatus, expect no error and pause set */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St03TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != TRUE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be paused, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Test state entry method 2 (ResumeTransfer).... */
        VERBOSE_PRINT("\nCheck state exit: Resume transfer.\n");

        /* ResumeTransfer, expect no error */
        VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_ResumeTransfer(St03TransferId);
#if !defined (ST_OSWINCE)
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check in "progress" state */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
#endif
        ErrorCode = STFDMA_GetTransferStatus(St03TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != FALSE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be running, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Clean up with a force term */
        VERBOSE_PRINT("Calling STFDMA_Term (forced), expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for abort to occur */
        VERBOSE_PRINT("Wait for abort notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* Clean up */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        /*=================== Transfer with node transfers complete =================*/

        /* Test state exit method 3 (transfer complete).... */
        VERBOSE_PRINT("\nCheck state exit: Transfer complete.\n");

        /* Init, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
        InitParams.DeviceType = FDMA_TEST_DEVICE;
        InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
        InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
        InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
        InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
        InitParams.NumberCallbackTasks = 1;
        InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
        InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
        InitParams.FDMABlock = STFDMA_1;
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

        /* Setup a non-blocking transfer. Expect no error */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_0D, FDMATST_Y_STATIC, TRUE,
                                       FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                       FALSE, FALSE, PaceSignal, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout = 0;
        TransferParams.CallbackFunction = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for non-blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for transfer complete notify */
        VERBOSE_PRINT("Wait for transfer complete notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pTransferCompleteSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get transfer complete notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* Check not in "progress" state with illegal calls, expect errors */
        VERBOSE_PRINT("Check in expected state....\n");

        /* Init, expect error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect ALREADY_INITIALIZED.\n");
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_ERROR_ALREADY_INITIALIZED);

        /* Clean up */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        /*=================== Transfer with forced term ====================*/

        /* Test state exit method 4 (Forced term) */
        VERBOSE_PRINT("\nCheck state exit: Term, forced.\n");

        /* Setup an endless non-blocking transfer. Expect no error */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout = 0;
        TransferParams.CallbackFunction = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St03TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check in "progress" state with GetTransferStatus, expect no error */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St03TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.NodeBytesRemaining == 0)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be in progress, but byte count disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Term, forced. Expect no error */
        VERBOSE_PRINT("Calling STFDMA_Term, forced, expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for abort to action */
        VERBOSE_PRINT("Wait for abort notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* Check not in "progress" state with illegal calls */
        VERBOSE_PRINT("Check in expected state....\n");

        /* Term, expect error */
        VERBOSE_PRINT("Calling STFDMA_Term (unforced), expect NOT_INITIALIZED.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, FALSE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

        /* Clean up */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();
        St03TransferId = 0;

    } /* end test loop control for paced/nonpaced. */
}

/****************************************************************************
Name         : St04TransferTaskFunc
Description  : Task for contolling the transfers required by test case St04
Parameters   :
Return Value :
****************************************************************************/
#if defined (ST_OSLINUX)
static int  St04TransferTaskFunc(void * Paced)
#else
static void St04TransferTaskFunc(void * Paced)
#endif
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_Node_t               *Node_p;
    BOOL                        PacedTransfer = FALSE;
    U32                         PaceSignal;

    STOS_TaskEnter(Paced);

    PacedTransfer = (BOOL)Paced;

    /* Paced transfer configuration */
    if (PacedTransfer == TRUE)
    {
        PaceSignal = FDMA_TEST_ACTIVE_DREQ;
    }
    else
    {
        PaceSignal = STFDMA_REQUEST_SIGNAL_NONE;
    }

    VERBOSE_PRINT("\n St04 transfer task running...\n");

    /*=================== Tranfser (unending) ====================*/

    /* Wait for go command */
    VERBOSE_PRINT(" Waiting for GO signal.\n");
    STOS_SemaphoreWait(pSt04TaskContinue);

    VERBOSE_PRINT("... STARTED PART 1\n");

    /* Testing entry method 1 with endless, no node pause transfer */
    /* Also tests exit method 1, when abort is called */
    VERBOSE_PRINT(" Setup transfer node.\n");
    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PaceSignal, &Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* For paced transfer, start the generator */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Init and Start pace request signal generator\n");
        fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
        fdmatst_StartPaceRequestGenerator();
    }
#endif

    TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout   = 0; /* no timeout */
    TransferParams.CallbackFunction  = NULL;  /* Don't care (blocking) */
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT(" Calling STFDMA_StartTransfer for endless, blocking transfer. Expect TRANSFER_ABORTED after Abort call.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &St04TransferId);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_TRANSFER_ABORTED);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* Halt request generator for paced transfers */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Terminating pace request signal generator\n");
        fdmatst_TermPaceRequestGenerator();
    }
#endif

    /* Signal test thread of completion */
    STOS_SemaphoreSignal(pSt04TestContinue);

    /* Clean up */
    VERBOSE_PRINT(" Clean memory space.\n");
    fdmatst_DeallocateNodes();

    /*=================== Tranfser (unending) ====================*/

    /* Wait for go command */
    VERBOSE_PRINT(" Waiting for GO signal.\n");
    STOS_SemaphoreWait(pSt04TaskContinue);

    VERBOSE_PRINT("... STARTED PART 2\n");

    /* Testing entry method 1 with endless, no node pause transfer */
    /* Also tests exit method 1, when abort is called */
    VERBOSE_PRINT(" Setup transfer node.\n");
    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PaceSignal, &Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* For paced transfer, start the generator */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Init and Start pace request signal generator\n");
        fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
        fdmatst_StartPaceRequestGenerator();
    }
#endif

    TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout   = 0;     /* no timeout */
    TransferParams.CallbackFunction  = NULL;  /* Don't care (blocking) */
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT(" Calling STFDMA_StartTransfer for endless, blocking transfer. Expect TRANSFER_ABORTED after Abort call.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &St04TransferId);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_TRANSFER_ABORTED);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* Halt request generator for paced transfers */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Terminating pace request signal generator\n");
        fdmatst_TermPaceRequestGenerator();
    }
#endif

    /* Signal test thread of completion */
    STOS_SemaphoreSignal(pSt04TestContinue);

    /* Clean up */
    VERBOSE_PRINT(" Clean memory space.\n");
    fdmatst_DeallocateNodes();

    /*=================== Tranfser (ending) ====================*/

    /* Wait for next GO command */
    VERBOSE_PRINT(" Waiting for GO signal.\n");
    STOS_SemaphoreWait(pSt04TaskContinue);

    VERBOSE_PRINT("... STARTED PART 3\n");

    /* Testing exit method 2 with no pause transfer to completion */
    VERBOSE_PRINT(" Setup transfer node.\n");
    ErrorCode = fdmatst_CreateNode(FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FDMATST_DIM_1D, FDMATST_Y_INC, TRUE,
                                   FALSE, FALSE, PaceSignal, &Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* For paced transfer, start the generator */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Init and Start pace request signal generator\n");
        fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
        fdmatst_StartPaceRequestGenerator();
    }
#endif

    /* Trnasfer params can stay same as previous */
    TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout   = 0;     /* no timeout */
    TransferParams.CallbackFunction  = NULL;  /* Don't care (blocking) */
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT(" Calling STFDMA_StartTransfer for short, blocking transfer. Expect NO_ERROR.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &St04TransferId);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* Halt request generator for paced transfers */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Terminating pace request signal generator\n");
        fdmatst_TermPaceRequestGenerator();
    }
#endif

    /* Signal test thread of completion */
    VERBOSE_PRINT("Signal the end of the test\n");
    STOS_SemaphoreSignal(pSt04TestContinue);

    /* Clean up */
    VERBOSE_PRINT(" Clean memory space.\n");
    fdmatst_DeallocateNodes();

    /*=================== Tranfser (timeout) ====================*/

    /* Wait for next GO command */
    VERBOSE_PRINT(" Waiting for GO signal.\n");
    STOS_SemaphoreWait(pSt04TaskContinue);

    VERBOSE_PRINT("... STARTED PART 4\n");

    /* Testing state exit method 3 with short timeout */
    VERBOSE_PRINT(" Setup transfer node.\n");
    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PaceSignal, &Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* For paced transfer, start the generator */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Init and Start pace request signal generator\n");
        fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
        fdmatst_StartPaceRequestGenerator();
    }
#endif

    /* Only 1 parameter needs ot change */
    TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout   = 2;     /* 2 ms timeout */
    TransferParams.CallbackFunction  = NULL;  /* Don't care (blocking) */
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT(" Calling STFDMA_StartTransfer for circular, blocking transfer with short timeout. Expect BLOCKING_TIMEOUT.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &St04TransferId);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_BLOCKING_TIMEOUT);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* Halt request generator for paced transfers */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Terminating pace request signal generator\n");
        fdmatst_TermPaceRequestGenerator();
    }
#endif

    /* Signal test thread of completion */
    STOS_SemaphoreSignal(pSt04TestContinue);

    /* Clean up */
    VERBOSE_PRINT(" Clean memory space.\n");
    fdmatst_DeallocateNodes();

    /*=================== Tranfser (unending) ====================*/

    /* Wait for next GO command */
    VERBOSE_PRINT(" Waiting for GO signal.\n");
    STOS_SemaphoreWait(pSt04TaskContinue);

    VERBOSE_PRINT("... STARTED PART 5\n");

    /* Testing state exit method 4 with endless, no pause transfer */
    VERBOSE_PRINT(" Setup transfer node.\n");
    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, PaceSignal, &Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* For paced transfer, start the generator */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Init and Start pace request signal generator\n");
        fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
        fdmatst_StartPaceRequestGenerator();
    }
#endif

    TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout   = 0; /* No timeout */
    TransferParams.CallbackFunction  = NULL;  /* Don't care (blocking) */
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT(" Calling STFDMA_StartTransfer for endless, blocking transfer. Expect TRANSFER_ABORTED after Term call.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &St04TransferId);
    fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_TRANSFER_ABORTED);

#if !defined (STFDMA_NO_PACED_TESTS)
    /* Halt request generator for paced transfers */
    if (PacedTransfer == TRUE)
    {
        VERBOSE_PRINT("Terminating pace request signal generator\n");
        fdmatst_TermPaceRequestGenerator();
    }
#endif

    /* Signal test thread of completion */
    STOS_SemaphoreSignal(pSt04TestContinue);

    /* Clean up */
    VERBOSE_PRINT(" Clean memory space.\n");
    fdmatst_DeallocateNodes();
    St04TransferId = 0;

    /*
    Had to introduce this wait because STOS_TaskCreate() also changes the scheduling policy of this thread
    to SCHED_RR and priority to MAX_USER_PRIORITY due to which the base process (test St04 calling 'forced'
    STFDMA_Term()) never gets a chance to proceed to STOS_TaskWait(). Corresponding signalling call inserted
    in St04 before STOS_TaskWait().
    */
    STOS_SemaphoreWait(pSt04TaskContinue);

    /* Memory clean up */
    VERBOSE_PRINT(" St04 transfer task exit.\n");

    STOS_TaskExit(Paced);
#if defined (ST_OSLINUX)
    return ST_NO_ERROR;
#endif
}

/****************************************************************************
Name         : CaseSt04
Description  : Testcase St04
               Blocking transfer in progress state test.
               Testing occurs in this function in an independant task which
               controls the blocking transfers.
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt04(int arg)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t         InitParams;
    STFDMA_TransferStatus_t     TransferStatus;
    BOOL                        Paced = FALSE;
    U8                          TransferType;


    /* Perform test for paced and non paced transfers...*/
    for (TransferType = NON_PACED_TRANSFER; TransferType != TEST_COMPLETE; TransferType++)
    {
        if (TransferType == NON_PACED_TRANSFER)
        {
            STTBX_Print(("Check 'in progress, blocking' (non-paced)....\n\n"));
            Paced = FALSE;
        }
        else
        {
            STTBX_Print(("Check 'in progress, blocking' (paced)....\n\n"));
            Paced = TRUE;
        }

        /* Reset transfer id before commencing test */
        St04TransferId = 0;

        /* Setup and run a utility task for controlling blocking transfers */
        VERBOSE_PRINT("Setup utility task for controlling blocking transfers.\n");

        STOS_TaskCreate((void(*)(void *))St04TransferTaskFunc,
                        (void *)&Paced,
                        system_partition,
                        (size_t)1024,
                        (void **)&St04TransferTaskStack,
                        system_partition,
                        &pSt04TransferTask,
                        St04TransferTaskDescriptor,
                        MAX_USER_PRIORITY,
                        "St04TransferTask",
                        (task_flags_t)0);

        /* Test "progress" entry method 1 (StartTransfer)..... */
        VERBOSE_PRINT("Check state entry: StartTransfer.....\n");

        /* Init, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
        InitParams.DeviceType = FDMA_TEST_DEVICE;
        InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
        InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
        InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
        InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
        InitParams.NumberCallbackTasks = 1;
        InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
        InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
        InitParams.FDMABlock = STFDMA_1;
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

        /*=================== Start Tranfser ====================*/

        /* signal task to start first transfer, endless, blocking */
        VERBOSE_PRINT("START PART 1 ...\n");
        STOS_SemaphoreSignal(pSt04TaskContinue);

        /* Wait for transferId to become valid */
        VERBOSE_PRINT("Wait for TransferID.\n");
        while (St04TransferId == 0)
            STOS_TaskDelay(fdmatst_GetClocksPerSec()/10);

        /* Test in "progress" state with call to GetTransferStatus */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != FALSE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be running, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Test "progress" state exit method 1 (Abort transfer).... */
        VERBOSE_PRINT("\nCheck state exit: AbortTransfer.....\n");

        /* Abort transfer, expect no error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St04TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for signal from transfer task */
        VERBOSE_PRINT("Wait for abort signal from transfer task.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pSt04TestContinue, &WaitTime))
        {
            VERBOSE_PRINT("*** ERROR: Did not get signal that abort was successful.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "progress" state with calls to GetTransferStatus */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Clear out transferId ready for next test */
        St04TransferId = 0;

        /*=================== Start Tranfser ====================*/

        /* signal task to start first transfer, endless, blocking */
        VERBOSE_PRINT("START PART 2 ...\n");
        STOS_SemaphoreSignal(pSt04TaskContinue);

        /* Wait for transferId to become valid */
        VERBOSE_PRINT("Wait for TransferID.\n");
        while (St04TransferId == 0)
            STOS_TaskDelay(fdmatst_GetClocksPerSec()/10);

        /* Test in "progress" state with call to GetTransferStatus */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != FALSE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be running, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Test "progress" state exit method 1 (Abort transfer).... */
        VERBOSE_PRINT("\nCheck state exit: AbortTransfer.....\n");

        /* Abort transfer, expect no error */
#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710)
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St04TransferId);
#else
        VERBOSE_PRINT("Calling STFDMA_FlushTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_FlushTransfer(St04TransferId);
#endif

        fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

        if (ErrorCode == ST_ERROR_FEATURE_NOT_SUPPORTED)
        {
            /* Abort transfer, expect no error */
            VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
            ErrorCode = STFDMA_AbortTransfer(St04TransferId);
            fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        }

        /* Wait for signal from transfer task */
        VERBOSE_PRINT("Wait for abort signal from transfer task.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pSt04TestContinue, &WaitTime))
        {
            VERBOSE_PRINT("*** ERROR: Did not get signal that abort was successful.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "progress" state with calls to GetTransferStatus */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Clear out transferId ready for next test */
        St04TransferId = 0;

        /*=================== Start Tranfser ====================*/

        /* Test "progress" state exit method 2 (transfer complete)..... */
        VERBOSE_PRINT("\nCheck state exit: TransferComplete.....\n");

        /* signal task to start next transfer, short, blocking */
        VERBOSE_PRINT("START PART 3 ...\n");
        STOS_SemaphoreSignal(pSt04TaskContinue);

        /* Wait for transfer complete signal from  tansfer task */
        VERBOSE_PRINT("Wait for transfer complete signal from transfer task.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pSt04TestContinue, &WaitTime))
        {
            VERBOSE_PRINT("*** ERROR: Did not get signal that transfer was successful.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "progress" state with calls to GetTransferStatus */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Clear out transferId ready for next test */
        St04TransferId = 0;

        /*=================== Start Tranfser ====================*/

        /* Test "progress" state exit method 3 (blocking timeout)..... */
        VERBOSE_PRINT("\nCheck state exit: Blocking timeout....\n");

        /* signal task to start next transfer, endless, blocking with small timeout */
        VERBOSE_PRINT("START PART 4 ...\n");
        STOS_SemaphoreSignal(pSt04TaskContinue);

        /* Wait for timeout signal from tansfer task */
        VERBOSE_PRINT("Wait for blocking timeout signal from transfer task.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pSt04TestContinue, &WaitTime))
        {
            VERBOSE_PRINT("*** ERROR: Did not get signal that blocking timeout was successful.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "progress" state with calls to GetTransferStatus */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

        /* Clear out transferId ready for next test */
        St04TransferId = 0;

        /*=================== Start Tranfser ====================*/

        /* Test "progress" state exit method 4 (forced term).... */
        VERBOSE_PRINT("\nCheck state exit: Term, forced....\n");

        /* signal task to start next transfer, endless, blocking infinite timeout */
        VERBOSE_PRINT("START PART 5 ...\n");
        STOS_SemaphoreSignal(pSt04TaskContinue);

        /* Wait for Id to become valid */
        VERBOSE_PRINT("Wait for TransferID.\n");
        while (St04TransferId == 0)
            STOS_TaskDelay(fdmatst_GetClocksPerSec()/10);

        /* Check in "progress" state with call to GetTransferStatus */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != FALSE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be running, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Term, force, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Term, forced. Expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for abort signal from task */
        VERBOSE_PRINT("Wait for term/abort signal from transfer task.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pSt04TestContinue, &WaitTime))
        {
            VERBOSE_PRINT("*** ERROR: Did not get signal that term/abort was successful.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "progress" state with calls to GetTransferStatus, expect error */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NOT_INITIALIZED.\n");
        ErrorCode = STFDMA_GetTransferStatus(St04TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

        STOS_SemaphoreSignal(pSt04TaskContinue);

        STOS_TaskWait(&pSt04TransferTask, (STOS_Clock_t *)TIMEOUT_INFINITY);
        STOS_TaskDelete(pSt04TransferTask,
                        system_partition,
                        St04TransferTaskStack,
                        system_partition);

    }  /* end of for loop controlling paced/non-paced */
}

/****************************************************************************
Name         : CaseSt05
Description  : Testcase St05
               Transfer Paused
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt05(int arg)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node_p;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_TransferStatus_t     TransferStatus;
    BOOL                        PacedControl;
    U32                         PaceSignal;
    U8                          TransferType;

    /* Perform test for paced and non paced transfers...*/

    for (TransferType = NON_PACED_TRANSFER; TransferType != TEST_COMPLETE; TransferType++)
    {
        if (TransferType == NON_PACED_TRANSFER)
        {
            STTBX_Print(("Check `paused` state (non-paced)....\n\n"));
            PacedControl = FALSE;
            PaceSignal = STFDMA_REQUEST_SIGNAL_NONE;
        }
        else
        {
            STTBX_Print(("Check `paused` state (paced)....\n\n"));
            PacedControl = TRUE;
            PaceSignal   = FDMA_TEST_ACTIVE_DREQ;
        }

        /*=================== Start Tranfser ====================*/

        /* Test "paused" state entry method (only 1)....*/
        VERBOSE_PRINT("Check state entry: node complete paused.....\n");

        /* Init, expect no error */
        VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
        InitParams.DeviceType          = FDMA_TEST_DEVICE;
        InitParams.DriverPartition_p   = fdmatst_GetSystemPartition();
        InitParams.NCachePartition_p   = fdmatst_GetNCachePartition();
        InitParams.InterruptNumber     = FDMA_INTERRUPT_NUMBER;
        InitParams.InterruptLevel      = FDMA_INTERRUPT_LEVEL;
        InitParams.NumberCallbackTasks = 1;
        InitParams.BaseAddress_p       = fdmatst_GetBaseAddress();
        InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
        InitParams.FDMABlock = STFDMA_1;
        ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
        LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

        /* StartTransfer, endless non-blocking with node-complete pause set */
        VERBOSE_PRINT("Setup transfer nodes.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(TRUE, TRUE, TRUE, TRUE,PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        /* (Indirectly tests "ready" state exit method 2 (StartTransfer)) */
        TransferParams.ChannelId         = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p     = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout   = 0;
        TransferParams.CallbackFunction  = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer with pause. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St05TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for paused notification */
        VERBOSE_PRINT("Wait for pause notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pPausedSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get pause notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* GetTransferStatus, expect no error, pause bit set */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St05TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != TRUE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be paused, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Test "paused" state exit method 1 (ResumeTransfer).... */
        VERBOSE_PRINT("\nCheck state exit: ResumeTransfer.....\n");

        /* ResumeTransfer, expect no error */
        VERBOSE_PRINT("Calling STFDMA_ResumeTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_ResumeTransfer(St05TransferId);
#if !defined (ST_OSWINCE)
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check not in paused state */
        VERBOSE_PRINT("Check in expected state....\n");

        /* may not have enough time to perform this check before pause occurs again... */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
#endif
        ErrorCode = STFDMA_GetTransferStatus(St05TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != FALSE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be running, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Test "paused" state exit method 2 (AbortTransfer)...... */
        VERBOSE_PRINT("\nCheck state exit: AbortTransfer.....\n");

        /* Wait for paused notification */
        VERBOSE_PRINT("Wait for pause notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pPausedSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get pause notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check in paused state with GetTransferStatus, expect no error, paused bit set */
        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St05TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != TRUE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be paused, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if defined (ST_5100) || defined (ST_5105) || defined (ST_5107) || defined (ST_5301) || defined (ST_7710)
        /* AbortTransfer, expect no error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St05TransferId);
#else
        /* FlushTransfer, expect no error */
        VERBOSE_PRINT("Calling STFDMA_FlushTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_FlushTransfer(St05TransferId);
#endif

        fdmatst_ErrorReport("", ErrorCode, POST_FDMA_1(ST_NO_ERROR,ST_ERROR_FEATURE_NOT_SUPPORTED));

        if (ErrorCode == ST_NO_ERROR)
        {
            VERBOSE_PRINT("Wait for abort notification.\n");
            WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
            if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
            {
                /* semaphore timed out....no notification provided..error */
                VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
                fdmatst_SetSuccessState(FALSE);
            }
        }
        else
        {
            /* AbortTransfer, expect no error */
            VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
            ErrorCode = STFDMA_AbortTransfer(St05TransferId);
            fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

            VERBOSE_PRINT("Wait for abort notification.\n");
            WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
            if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
            {
                /* semaphore timed out....no notification provided..error */
                VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
                fdmatst_SetSuccessState(FALSE);
            }
        }

        /* Check not in "paused" state with GetTransferStatus call, expect error */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect INVALID_TRANSFER_ID.\n");
        ErrorCode = STFDMA_GetTransferStatus(St05TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_INVALID_TRANSFER_ID);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* clean up memory */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();

        /* Test "paused" state exit method 3 (Term, forced)...... */
        VERBOSE_PRINT("\nCheck state exit: Term, forced.....\n");

        /* StartTransfer, endless non-blocking with node-complete pause set */
        VERBOSE_PRINT("Setup transfer node.\n");
        ErrorCode = fdmatst_SetupCircularTransfer(TRUE, TRUE, TRUE, TRUE, PacedControl, &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if !defined (STFDMA_NO_PACED_TESTS)
        /* For paced transfer, start the generator */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Init and Start pace request signal generator\n");
            fdmatst_InitPaceRequestGenerator(Node_p, DEFAULT_RATE);
            fdmatst_StartPaceRequestGenerator();
        }
#endif

        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.BlockingTimeout = 0;
        TransferParams.CallbackFunction = Callback;
        TransferParams.ApplicationData_p = NULL;
        TransferParams.FDMABlock = STFDMA_1;
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer with pause. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St05TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Wait for paused notification */
        VERBOSE_PRINT("Wait for pause notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pPausedSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get pause notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check in "paused" state with GetTransferStatus call, expect no error */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NO_ERROR.\n");
        ErrorCode = STFDMA_GetTransferStatus(St05TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (TransferStatus.Paused != TRUE)
        {
            VERBOSE_PRINT("*** ERROR: Transfer should be paused, but status disagrees.\n");
            fdmatst_SetSuccessState(FALSE);
        }

#if !defined (STFDMA_NO_PACED_TESTS)
        /* Halt request generator for paced transfers */
        if (TransferType == PACED_TRANSFER)
        {
            VERBOSE_PRINT("Terminating pace request signal generator\n");
            fdmatst_TermPaceRequestGenerator();
        }
#endif

        /* Term, forced. Expect no error */
        VERBOSE_PRINT("Calling STFDMA_Term, forced. Expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        VERBOSE_PRINT("Wait for abort notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pAbortSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
        }

        /* Check not in "paused" state with GetTransferStatus call, expect error */
        VERBOSE_PRINT("Check in expected state....\n");

        VERBOSE_PRINT("Calling STFDMA_GetTransferStatus, expect NOT_INITIALIZED.\n");
        ErrorCode = STFDMA_GetTransferStatus(St05TransferId, &TransferStatus);
        fdmatst_ErrorReport("", ErrorCode, STFDMA_ERROR_NOT_INITIALIZED);

        /* Clean up */
        VERBOSE_PRINT("Cleaning memory space.\n");
        fdmatst_DeallocateNodes();
        St05TransferId = 0;
    } /* end for paced/non-paced control */
}

/****************************************************************************
Name         : CaseSt06
Description  : Testcase St06
               Unalloacted Context
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt06(int arg)
{
    /* Covered by Paramter test P25 */
}

/****************************************************************************
Name         : CaseSt07
Description  : Testcase St07
               Transfer Paused
Parameters   : Allocated Context
Return Value :
****************************************************************************/
static void CaseSt07(int arg)
{
    /* Covered by Paramter test P25 */
}

/****************************************************************************
Name         : CaseSt08
Description  : Testcase St08
               Do Start and Abort transfer to check its stability.
Parameters   :
Return Value :
****************************************************************************/
static void CaseSt08(int arg)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_TransferParams_t     TransferParams;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node_p;
    U32                         Index;

    /* Test "ready" state entry method 1 (Init call)....  */
    VERBOSE_PRINT("Check state entry: Init call....\n");

    /* Init, expect no error */
    VERBOSE_PRINT("Calling STFDMA_Init, expect NO_ERROR.\n");
    InitParams.DeviceType = FDMA_TEST_DEVICE;
    InitParams.DriverPartition_p = fdmatst_GetSystemPartition();
    InitParams.NCachePartition_p = fdmatst_GetNCachePartition();
    InitParams.InterruptNumber = FDMA_INTERRUPT_NUMBER;
    InitParams.InterruptLevel = FDMA_INTERRUPT_LEVEL;
    InitParams.NumberCallbackTasks = 1;
    InitParams.BaseAddress_p = fdmatst_GetBaseAddress();
    InitParams.ClockTicksPerSecond = fdmatst_GetClocksPerSec();
    InitParams.FDMABlock = STFDMA_1;
    ErrorCode = STFDMA_Init(FDMATEST_DEVICENAME, &InitParams);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

#if defined (ST_OSLINUX) && defined (CONFIG_STM_DMA)
    LOCK_FDMA_CHANNELS(&Lock_7100_fdma2[0], STFDMA_1);
#endif

    /* Check illegal calls....*/
    VERBOSE_PRINT("Check in expected state....\n");

    /* Setup an endless non-blocking transfer: No pause, no notify  */
    VERBOSE_PRINT("Setup transfer node.\n");
    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, TRUE, TRUE, FALSE, &Node_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    /* (Indirectly tests "ready" state exit method 2 (StartTransfer)) */
    TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
    TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
    TransferParams.BlockingTimeout = 0;
    TransferParams.CallbackFunction = Callback;
    TransferParams.ApplicationData_p = NULL;
    TransferParams.FDMABlock = STFDMA_1;

    for(Index=0; Index<100; Index++)
    {
        VERBOSE_PRINT("Calling STFDMA_StartTransfer for endless, non-blocking transfer. Expect no error.\n");
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        StartTransferCalled++;
        STOS_TaskDelay(fdmatst_GetClocksPerSec()/10);

        /* Abort transfer with call to Abort, expect no error */
        VERBOSE_PRINT("Calling STFDMA_AbortTransfer, expect NO_ERROR.\n");
        ErrorCode = STFDMA_AbortTransfer(St02TransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

        /* Check abort callback recieved */
        VERBOSE_PRINT("Wait for abort notification.\n");
        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pAbortSemap,&WaitTime  ) != 0) /* TIMEOUT_INFINITY*/
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR: Did not get abort notification within time allowed.\n");
            fdmatst_SetSuccessState(FALSE);
            break;
        }

    	VERBOSE_PRINT_DATA("StartTransfer Called = %d ", StartTransferCalled);
    	VERBOSE_PRINT_DATA("Aborted = %d \n", St02TransferIdAborted);
    }

    /* clean up memory */
    VERBOSE_PRINT("Cleaning memory space.\n");
    fdmatst_DeallocateNodes();

    /* Term, forced. Expect no error */
    VERBOSE_PRINT("Calling STFDMA_Term, forced. Expect NO_ERROR.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    /* Clean up */
    St02TransferId = 0;
}

static struct
{
    U32    Number;
    char  *Title;
    void (*Func)(int);
    int    Arg;
}
TestSet[] =
{
    {1, "St01 executing:\n\n", CaseSt01, 0},
    {2, "St02 executing:\n\n", CaseSt02, 0},
    {3, "St03 executing:\n\n", CaseSt03, 0},
    {4, "St04 executing:\n\n", CaseSt04, 0},
    {5, "St05 executing:\n\n", CaseSt05, 0},
    {6, "St06 executing:\n\n", CaseSt06, 0},
    {7, "St07 executing:\n\n", CaseSt07, 0},
    {8, "St08 executing:\n\n", CaseSt08, 0},
#if !defined(ST_5517) && !defined (STFDMA_NO_PACED_TESTS)
    {97, "St97 executing:\n\n", CaseSt99, 2},  /* Non-blocking - Flush */
    {98, "St98 executing:\n\n", CaseSt99, 1},  /* Blocking     */
    {99, "St99 executing:\n\n", CaseSt99, 0},  /* Non-blocking */
#endif
};

#if defined(DEBUG)
/****************************************************************************
Name         : state_RunTest
Description  : Run a specific test
Parameters   :
Return Value :
****************************************************************************/
static void state_RunTest(U32 Number)
{
    int i;

    for (i = 0; (i < sizeof(TestSet)/sizeof(*TestSet)); i++)
    {
        if (Number == TestSet[i].Number)
        {
            STTBX_Print((TestSet[i].Title));
            TestSet[i].Func(TestSet[i].Arg);
            fmdatst_TestCaseSummarise();
            break;
        }
    }
}
#endif

/* Global Functions ------------------------------------------------------- */
/****************************************************************************
Name         : state_RunStateTest
Description  : Entry point for state test
Parameters   :
Return Value :
****************************************************************************/
void state_RunStateTest(void)
{

    /* Environment setup */
    pAbortSemap = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pPausedSemap = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pTransferCompleteSemap = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pListCompleteSemap = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pNodeCompleteSemap = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pSt04TestContinue = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pSt04TaskContinue = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);

#if defined (ST_OS20)
    St04TransferTaskDescriptor = (tdesc_t *)STOS_MemoryAllocate(system_partition, sizeof(tdesc_t));
#endif

    /* Test execution */
    STTBX_Print(("\nState Test Cases:\n"));
    STTBX_Print(("--------------------------------------------------------\n"));

    fdmatst_ResetSuccessState();

#if defined(DEBUG)
    /* Emulation test set */

    state_RunTest(99);
    state_RunTest(98);
    state_RunTest(97);
    state_RunTest(2);
    state_RunTest(3);
    state_RunTest(4);
    state_RunTest(5);
    state_RunTest(6);
    state_RunTest(7);

#else
    {
        /* Run all the tests listed in the table of tests */

        int i;
        for (i = 0; (i < sizeof(TestSet)/sizeof(*TestSet)); i++)
        {
            STTBX_Print((TestSet[i].Title));
            TestSet[i].Func(TestSet[i].Arg);
            fmdatst_TestCaseSummarise();
        }
    }
#endif

    /* Clean up */
    STOS_SemaphoreDelete(system_partition, pAbortSemap);
    STOS_SemaphoreDelete(system_partition, pPausedSemap);
    STOS_SemaphoreDelete(system_partition, pTransferCompleteSemap);
    STOS_SemaphoreDelete(system_partition, pListCompleteSemap);
    STOS_SemaphoreDelete(system_partition, pNodeCompleteSemap);
    STOS_SemaphoreDelete(system_partition, pSt04TestContinue);
    STOS_SemaphoreDelete(system_partition, pSt04TaskContinue);

#if defined (ST_OS20)
    STOS_MemoryDeallocate(system_partition, St04TransferTaskDescriptor);
#endif
}
/*eof*/
