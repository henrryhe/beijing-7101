/*****************************************************************************
File Name   : soak.c

Description : STFDMA soak test source

Copyright (C) 2002 STMicroelectronics

History     :

Reference   :
*****************************************************************************/

/*#define DEBUG 1*/

/* Includes --------------------------------------------------------------- */

#include <string.h>                     /* C libs */
#include <stdio.h>

#include "sttbx.h"
#include "stdevice.h"
#include "stlite.h"                     /* Standard includes */
#include "stddefs.h"
#include "stcommon.h"

#include "stfdma.h"                     /* fdma api header file */

#include "fdmatst.h"

#ifdef DEBUG
#include "debug.h"
#endif

/* Private Constants ------------------------------------------------------ */
#define SK01_DURATION     (STFDMA_SOAK_DURATION)
#define SK02_DURATION     (STFDMA_SOAK_DURATION)

extern partition_t* system_partition;

/* Private Variables ------------------------------------------------------ */
static semaphore_t* pAbortSemap;
static semaphore_t* pTransferCompleteSemap;
static semaphore_t* pSk02Transfer1AbortSemap;
static semaphore_t* pSk02Transfer2AbortSemap;

static STFDMA_TransferId_t GeneralTransferId;
static STFDMA_TransferId_t Sk02TransferId1;
static STFDMA_TransferId_t Sk02TransferId2;
static STOS_Clock_t        WaitTime;

/* Functions -------------------------------------------------------------- */
#define TRANSFER_RUNNING_COUNT_MAX   (1000)
/****************************************************************************
Name         : IsCircularTransferRunning
Description  : Utility to check if circular transfer is running
Parameters   :
Return Value :
****************************************************************************/
static BOOL IsCircularTransferRunning(STFDMA_TransferId_t TransferId)
{
    STFDMA_TransferStatus_t     TransferStatusA, TransferStatusB;
    U32                          i=0;
    U32                         SameNodeAddressCount = 0;
    ST_ErrorCode_t              ErrorCode;

    /* GetTransferStatus, expect no errors */
    VERBOSE_PRINT("Repeatedly calling STFDMA_GetTransferStatus and checking current NodeAddress..\n");

    /* Get transfer status snapshot */
    ErrorCode = STFDMA_GetTransferStatus(TransferId, &TransferStatusA);
    if (ErrorCode != ST_NO_ERROR)
    {
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
    }

    /* Repeatedly get transfer status for node address comparison */
    for (i=0; i!=TRANSFER_RUNNING_COUNT_MAX; i++)
    {
        /* Get another transfer status snapshot */
        STFDMA_GetTransferStatus(TransferId, &TransferStatusB);
        if (ErrorCode != ST_NO_ERROR)
        {
            fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        }

        /* Compare addresses */
        if (TransferStatusA.NodeAddress == TransferStatusB.NodeAddress)
        {
            /* Addresses are the same so increase count.
             * Could be due to polling frequency with FDMA still processing same node
             * or due to transfer not running.
             */
            SameNodeAddressCount++;
        }
        else
        {
            /* Address differ therefore transfer is running */
            SameNodeAddressCount = 0;
        }
    }

    /* Assess if transfer is running.
     * If Count is same as MAX NodeAddress never change therefore transfer is not running.
     */
    if (SameNodeAddressCount == TRANSFER_RUNNING_COUNT_MAX)
    {
        VERBOSE_PRINT("*** ERROR: Circular transfer NOT running\n");
        return FALSE;
    }
    else
    {
        VERBOSE_PRINT("Circular transfer is running.\n");
        return TRUE;
    }

}

/****************************************************************************
Name         : Callback
Description  : Callback function for general functional test usage
Parameters   : As defined by the stfdma API for callback functions.
Return Value :
****************************************************************************/
static void Callback (U32 TransferId, U32 CallbackReason, U32 *CurrentNode_p,
                      U32 NodeBytesRemaining, BOOL Error, void* ApplicationData_p,
                      STOS_Clock_t  InterruptTime)
{

    STFDMA_Node_t *Node_p;

    /* Check bytes remaining has a sensible value */
#if defined (ARCHITECTURE_ST40)
    Node_p = (STFDMA_Node_t *) ST40_NOCACHE_NOTRANSLATE(CurrentNode_p);
#else
    Node_p = (STFDMA_Node_t *) CurrentNode_p;
#endif
    if (NodeBytesRemaining > Node_p->NumberBytes)
    {
        VERBOSE_PRINT_DATA("Callback: *** ERROR: NodeBytesRemaining has illegal value (%d)\n",NodeBytesRemaining);
    }

    if (TransferId == GeneralTransferId)
    {
        /* Expected reasons for callback...*/
        switch (CallbackReason)
        {
        case STFDMA_NOTIFY_TRANSFER_ABORTED:
            STOS_SemaphoreSignal(pAbortSemap);
            break;
        case STFDMA_NOTIFY_TRANSFER_COMPLETE:
            STOS_SemaphoreSignal(pTransferCompleteSemap);
            break;
        default :
            VERBOSE_PRINT("Callback: *** ERROR: Unexpected callback reason.\n");
            break;
        }
    }
    else if ((TransferId == Sk02TransferId1) || (TransferId == Sk02TransferId2))
    {
        /* Expected reasons for callback...*/
        switch (CallbackReason)
        {
        case STFDMA_NOTIFY_TRANSFER_ABORTED:
            if (TransferId == Sk02TransferId1)
            {
                STOS_SemaphoreSignal(pSk02Transfer1AbortSemap);
            }
            else
            {
                STOS_SemaphoreSignal(pSk02Transfer2AbortSemap);
            }
            break;
        default :
            VERBOSE_PRINT("Callback: *** ERROR: Unexpected callback reason.\n");
            break;
        }
    }
    else
    {
        VERBOSE_PRINT("Callback: *** ERROR: Unrecognised TransferId\nn");
    }

    /* Display messgae if error set */
    if (Error)
    {
        VERBOSE_PRINT("Callback: *** Error flag set.\n");
    }

}

/****************************************************************************
Name         : CaseSk01
Description  : Repeated init, transfer to completion, data check, term
               over long period checking all calls are successfull and
               data is transfered correctly.
Parameters   :
Return Value :
****************************************************************************/
static void CaseSk01(void)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node_p = NULL;
    STFDMA_TransferParams_t     TransferParams;
    U32                         DataError = 0;
    STOS_Clock_t                StopTime;


    STTBX_Print(("Checking repeated initialisation, transfer, termination for %d minutes...\n\n",SK01_DURATION));

    /* Calculate test duration in ticks */
    StopTime = STOS_time_plus(time_now(), (fdmatst_GetClocksPerSec() * SK01_DURATION * 60));

    STTBX_Print(("Time now: %u, ",time_now()));
    STTBX_Print(("Stop time: %u\n",StopTime));

    /* Repeat test for duration or until failure */
    while ((STOS_time_after(time_now(),StopTime) == 0) && (fdmatst_IsTestSuccessful() == TRUE))
    {
        /* Initialise the FDMA */
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
        if (ErrorCode != ST_NO_ERROR)
        {
            VERBOSE_PRINT("*** ERROR ***\n");
            STTBX_Print(("Call to STFDMA_Init() failed at %d\n",time_now()));

        }

        ErrorCode = fdmatst_CreateNode(FDMATST_DIM_2D, FDMATST_Y_INC, TRUE,
                                       FDMATST_DIM_2D, FDMATST_Y_INC, TRUE,
                                       FALSE, FALSE,
                                       STFDMA_REQUEST_SIGNAL_NONE,
                                       &Node_p);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (ErrorCode != ST_NO_ERROR)
        {
            VERBOSE_PRINT("*** ERROR ***\n");
            STTBX_Print(("Call to CreateNode failed at %u\n",time_now()));

        }

        /* Start the transfer on any channel */
        TransferParams.BlockingTimeout = 0;
        TransferParams.CallbackFunction = Callback;
        TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        fdmatst_CovertNodeAddressDataToPeripheral(Node_p);
        TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node_p);
        TransferParams.FDMABlock = STFDMA_1;
        ErrorCode = STFDMA_StartTransfer(&TransferParams, &GeneralTransferId);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (ErrorCode != ST_NO_ERROR)
        {
            VERBOSE_PRINT("*** ERROR ***\n");
            STTBX_Print(("Call to STFDMA_StartTransfer() failed at %u\n",time_now()));
        }

        WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
        if (STOS_SemaphoreWaitTimeOut(pTransferCompleteSemap, &WaitTime) != 0)
        {
            /* semaphore timed out....no notification provided..error */
            VERBOSE_PRINT("*** ERROR ***\n");
            STTBX_Print(("Did not get transfer complete notification within time allowed at %u\n",time_now()));
            fdmatst_SetSuccessState(FALSE);
        }

        DataError = fdmatst_CheckData(&Node_p, "Sk01");
        if (DataError != 0)
        {
            VERBOSE_PRINT("*** ERROR ***\n");
            STTBX_Print(("Data check failed DataError == 0x%x\n",DataError));
            STTBX_Print(("Time: %u\n",time_now()));
            fdmatst_SetSuccessState(FALSE);
        }

        /* terminate driver */
        VERBOSE_PRINT("Calling STFDMA_Term, forced. Expect NO_ERROR.\n");
        ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
        fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
        if (ErrorCode != ST_NO_ERROR)
        {
            VERBOSE_PRINT("*** ERROR ***\n");
            STTBX_Print(("Call to STFDMA_Term() failed at %u\n",time_now()));
        }

        /* Clean up */
        VERBOSE_PRINT("Clean up memory.\n");
        fdmatst_DeallocateNodes();

        /* Check test progress...only continue if no failures */
        if (fdmatst_IsTestSuccessful() == FALSE)
        {
#ifdef DEBUG
            debugbreak();
#else
            break;
#endif
        }

    }  /* end while */

    GeneralTransferId = 0;
}

/****************************************************************************
Name         : CaseSk01
Description  : Performs two ping pong transfers over long period checking
               calls and data transfers are successful
Parameters   :
Return Value :
****************************************************************************/
static void CaseSk02(void)
{
    ST_ErrorCode_t              ErrorCode = ST_NO_ERROR;
    STFDMA_InitParams_t         InitParams;
    STFDMA_Node_t               *Node1_p = NULL;
    STFDMA_Node_t               *Node2_p = NULL;
    STFDMA_TransferParams_t     TransferParams;
    STOS_Clock_t                StopTime;
    STOS_Clock_t                TimeCheck;
    U32                         CheckCount;
    U32                         TimeCheckUpdate;
    U32                         DataCheck =0;

    STTBX_Print(("Checking repeated transfers for %d minutes...\n\n",SK02_DURATION));

    /* Initialise the FDMA */
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

    VERBOSE_PRINT("Setting up nodes.\n");

    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, FALSE, FALSE, FALSE, &Node1_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    ErrorCode = fdmatst_SetupCircularTransfer(FALSE, FALSE, FALSE, FALSE, FALSE, &Node2_p);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    /* Start the transfer on any channel */
    TransferParams.BlockingTimeout = 0;
    TransferParams.CallbackFunction = Callback;
    TransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
    fdmatst_CovertNodeAddressDataToPeripheral(Node1_p);
    TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node1_p);
    TransferParams.FDMABlock = STFDMA_1;
    VERBOSE_PRINT("Call STFDMA_StartTransfer for a non-blocking transfer expect NO_ERROR.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &Sk02TransferId1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    fdmatst_CovertNodeAddressDataToPeripheral(Node2_p);
    TransferParams.NodeAddress_p = TRANSLATE_NODE_ADDRESS_TO_PHYS(Node2_p);
    VERBOSE_PRINT("Call STFDMA_StartTransfer for another non-blocking transfer expect NO_ERROR.\n");
    ErrorCode = STFDMA_StartTransfer(&TransferParams, &Sk02TransferId2);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    /* Wait for duration specified */
    StopTime = STOS_time_plus(time_now(), (fdmatst_GetClocksPerSec() * SK02_DURATION * 60));
    TimeCheck = time_now();
    CheckCount = 0;
    /* Set up progress monitor count control */
    if (SK02_DURATION > 30)
    {
        /* Provide notice every 30mins */
        TimeCheckUpdate = 30;
    }
    else if (SK02_DURATION > 5)
    {
        /* Provide notice every 5mins */
        TimeCheckUpdate = 5;
    }
    else
    {
        /* Provide notice every 1mins */
        TimeCheckUpdate = 1;
    }
    STTBX_Print(("Time now: %u, ",time_now()));
    STTBX_Print(("transfers in progress until stop time: %u\n",StopTime));
    while (STOS_time_after(time_now(),StopTime) == 0)
    {
        /* sleep for a minute */
        STOS_TaskDelay(fdmatst_GetClocksPerSec() * 10);
        if ((STOS_time_minus(time_now(),TimeCheck)) >= (fdmatst_GetClocksPerSec() * 60 * TimeCheckUpdate))
        {
            CheckCount++;
            STTBX_Print(("%u mins elapsed\n",(CheckCount * TimeCheckUpdate)));
            TimeCheck = time_now();
        }

        if (IsCircularTransferRunning(Sk02TransferId1) != TRUE)
        {
            STTBX_Print(("*** ERROR: Tranfser Sk02TransferId1 not running!!\n"));
            fdmatst_SetSuccessState(FALSE);
        }

        if (IsCircularTransferRunning(Sk02TransferId2) != TRUE)
        {
            STTBX_Print(("*** ERROR: Tranfser Sk02TransferId2 not running!!\n"));
            fdmatst_SetSuccessState(FALSE);
        }
    }

    /* Abort transfers */
    VERBOSE_PRINT("Aborting both transfers. Expect NO_ERROR\n");
    ErrorCode = STFDMA_AbortTransfer(Sk02TransferId1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("*** Abort call failed (0x%x)\n", ErrorCode));
    }


    ErrorCode = STFDMA_AbortTransfer(Sk02TransferId2);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);
    if (ErrorCode != ST_NO_ERROR)
    {
        STTBX_Print(("*** Abort call failed (0x%x)\n", ErrorCode));
    }

    VERBOSE_PRINT("Waiting for transfers to abort.\n");

    WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
    if (STOS_SemaphoreWaitTimeOut(pSk02Transfer1AbortSemap, &WaitTime) != 0)
    {
        /* semaphore timed out....no notification provided..error */
        STTBX_Print(("*** ERROR: Did not get transfer abort notification within time allowed.\n"));
        fdmatst_SetSuccessState(FALSE);
#ifdef DEBUG
        STTBX_Print(("Time now: %u, ",time_now()));
        debugbreak();
#endif
    }

    WaitTime = (STOS_time_plus(time_now(),(10 * fdmatst_GetClocksPerSec())));
    if (STOS_SemaphoreWaitTimeOut(pSk02Transfer2AbortSemap, &WaitTime) != 0)
    {
        /* semaphore timed out....no notification provided..error */
        STTBX_Print(("*** ERROR: Did not get transfer abort notification within time allowed.\n"));
        fdmatst_SetSuccessState(FALSE);
#ifdef DEBUG
        STTBX_Print(("Time now: %u, ",time_now()));
        debugbreak();
#endif
    }

    /* Check no errors in data transfered */
    VERBOSE_PRINT("Checking data transfered successfully.\n");
    DataCheck = fdmatst_CheckData(&Node1_p, "Sk02");
    if (DataCheck != 0)
    {
        STTBX_Print(("*** ERROR: Data check failed for List 1, node flags %u\n",DataCheck));
        fdmatst_SetSuccessState(FALSE);
#ifdef DEBUG
        STTBX_Print(("Time now: %u, ",time_now()));
        debugbreak();
#endif
    }

    DataCheck = fdmatst_CheckData(&Node2_p, "Sk02");
    if (DataCheck != 0)
    {
        STTBX_Print(("*** ERROR: Data check failed for List 2, node flags %u\n",DataCheck));
        fdmatst_SetSuccessState(FALSE);
#ifdef DEBUG
        STTBX_Print(("Time now: %u, ",time_now()));
        debugbreak();
#endif
    }

    /* terminate driver */
    VERBOSE_PRINT("Calling STFDMA_Term, forced. Expect NO_ERROR.\n");
    ErrorCode = STFDMA_Term(FDMATEST_DEVICENAME, TRUE, STFDMA_1);
    fdmatst_ErrorReport("", ErrorCode, ST_NO_ERROR);

    /* Clean up */
    VERBOSE_PRINT("Clean up memory.\n");
    fdmatst_DeallocateNodes();

    Sk02TransferId1 = 0;
    Sk02TransferId2 = 0;

}

/****************************************************************************
Name         : soak_RunSoakTest
Description  : Entry point for soak test
Parameters   :
Return Value :
****************************************************************************/
void soak_RunSoakTest()
{
    pAbortSemap                 = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pTransferCompleteSemap      = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pSk02Transfer1AbortSemap    = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);
    pSk02Transfer2AbortSemap    = STOS_SemaphoreCreateFifoTimeOut(system_partition, 0);

    STTBX_Print(("\nSoak Test Cases:\n"));
    STTBX_Print(("--------------------------------------------------------\n"));

    fdmatst_ResetSuccessState();

    STTBX_Print(("Sk01 executing:\n\n"));
    CaseSk01();
    fmdatst_TestCaseSummarise();

    STTBX_Print(("Sk02 executing:\n\n"));
    CaseSk02();
    fmdatst_TestCaseSummarise();

    STOS_SemaphoreDelete(pAbortSemap);
    STOS_SemaphoreDelete(pTransferCompleteSemap);
    STOS_SemaphoreDelete(pSk02Transfer1AbortSemap);
    STOS_SemaphoreDelete(pSk02Transfer2AbortSemap);
}

/*eof*/
