/*****************************************************************************
File Name   : uartdma.c

Description : UART driver.

Copyright (C) 2006 STMicroelectronics

History     :


Reference   :

*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <string.h>                     /* C libs */
#include <stdio.h>

#include "stlite.h"                     /* Standard includes */
#include "stddefs.h"
#include "stos.h"

#include "stuart.h"                     /* STAPI */
#include "stcommon.h"

#include "stfdma.h"
#include "stsys.h"
#ifdef ST_7109
#include "sti7109.h"
#else
#include "sti7100.h"
#endif

#include "ascregs.h"                    /* ASC register definitions/macros */
#include "uarttimer.h"                  /* Timer mechanism */
#include "uartdat.h"                    /* Private data */
#include "uartdma.h"                    /* DMA support */



#define STTBX_Print(x) printf x
/*#define UARTDMA_DEBUG*/
/*#define UARTDMA_DBGERR*/
/*#define UARTDMA_DBGDMA*/

/* Private types/constants ------------------------------------------------ */

/* Private Variables ------------------------------------------------------ */

/* Private Macros --------------------------------------------------------- */

#ifdef UARTDMA_DEBUG
#define PrintDebug(x)	STTBX_Print(x)
#else
#define PrintDebug(x)
#endif

#ifdef UARTDMA_DBGERR
#define PrintError(x)	STTBX_Print(x)
#else
#define PrintError(x)
#endif

#ifdef UARTDMA_DBGDMA
#define PrintDebugDMA(x)	STTBX_Print(x)
#else
#define PrintDebugDMA(x)
#endif

#ifdef MIN                              /* Undefine for this module only */
#undef MIN
#endif

#define MIN(x,y)                    ((x<y)?x:y)

#define FDMA_REQ_CONTROL_OFFSET  0x04

#if defined (ST_7100)
#define FDMA_REQ_CONTROL_BASE    0x9780
#elif defined (ST_7109)
#define FDMA_REQ_CONTROL_BASE    0x9180
#endif

#ifndef UART_DMAREAD_TASK_PRIORITY
#define UART_DMAREAD_TASK_PRIORITY      MAX_USER_PRIORITY
#endif
#define UART_DMAREAD_STACK_SIZE         (10*1024)

/* Private Functions Prototypes -------------------------------------------- */

__inline void UART_DMATXFlush(UART_ControlBlock_t *Uart_p,
                            U8 *Buffer,
                            U32 NumberToWrite);
static void UART_DMAReadTask(UART_ControlBlock_t *Uart_p);
static void UART_DMAFlushBufferAndRestart( UART_ControlBlock_t *Uart_p);
static void UART_RXDMACallbackFunction(U32 TransferId,
                            U32 CallbackReason,
                            U32 *CurrentNode_p,
                            U32 NodeBytesRemaining,
                            BOOL Error,
                            void *ApplicationData_p,
                            osclock_t InterruptTime);
static void UART_TXDMACallbackFunction(U32 TransferId,
                            U32 CallbackReason,
                            U32 *CurrentNode_p,
                            U32 NodeBytesRemaining,
                            BOOL Error,
                            void *ApplicationData_p,
                            osclock_t InterruptTime);
__inline ST_ErrorCode_t  UART_RXDMACheckErrors( UART_ControlBlock_t *Uart_p);


/* Functions ******************************************************************/

/*****************************************************************************
UART_DMAInit()
Description:
    Acquies all resources associated with the Uart DMA driver and sets up the
    transfer nodes


Parameters:
    Uart_p,         pointer the the UART control block

Return Value:
    ST_NO_ERROR
    ST_ERROR_NO_MEMORY
See Also:

*****************************************************************************/
ST_ErrorCode_t UART_DMAInit(UART_ControlBlock_t *Uart_p)
{
    volatile UART_ASCRegs_t *ASCRegs_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    PrintDebug(("STUART DMA INIT\n"));

    ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

    /* TX */
    Uart_p->OriginalFDMANode_p[0] = STOS_MemoryAllocate(Uart_p->InitParams.NCachePartition,
                                            sizeof( STFDMA_Node_t ) + UART_FDMA_ALIGN );
    if (Uart_p->OriginalFDMANode_p[0] != NULL)
    {
        Uart_p->TXFDMANode_p = (STFDMA_Node_t *) ((((U32) Uart_p->OriginalFDMANode_p[0] ) +UART_FDMA_ALIGN ) & ~UART_FDMA_ALIGN );
        Uart_p->TXFDMANode_p->Next_p = NULL;
        Uart_p->TXFDMANode_p->DestinationAddress_p = (void *)((U32)&ASCRegs_p->ASCnTxBuffer & ST40_PHYSICAL_ACCESS_MASK);
        Uart_p->TXFDMANode_p->SourceStride = 0;
        Uart_p->TXFDMANode_p->DestinationStride = 0;
        Uart_p->TXFDMANode_p->NodeControl.PaceSignal = Uart_p->InitParams.TXDMARequestSignal;
        Uart_p->TXFDMANode_p->NodeControl.SourceDirection = STFDMA_DIRECTION_INCREMENTING;
        Uart_p->TXFDMANode_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_STATIC;
        Uart_p->TXFDMANode_p->NodeControl.NodeCompleteNotify = FALSE;
        Uart_p->TXFDMANode_p->NodeControl.NodeCompletePause = FALSE;
        Uart_p->TXFDMANode_p->NodeControl.Reserved = 0;

        Uart_p->TXFDMAGenericTransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;
        Uart_p->TXFDMAGenericTransferParams.Pool = STFDMA_DEFAULT_POOL;
        Uart_p->TXFDMAGenericTransferParams.CallbackFunction = UART_TXDMACallbackFunction;
        Uart_p->TXFDMAGenericTransferParams.BlockingTimeout = 0;
        Uart_p->TXFDMAGenericTransferParams.ApplicationData_p = Uart_p;
        Uart_p->TXFDMAGenericTransferParams.NodeAddress_p
            = (STFDMA_GenericNode_t *) ((U32)Uart_p->TXFDMANode_p & ST40_PHYSICAL_ACCESS_MASK);

        /* RX */
        Uart_p->OriginalFDMANode_p[1] = STOS_MemoryAllocate(Uart_p->InitParams.NCachePartition,
                                                sizeof( STFDMA_Node_t ) + UART_FDMA_ALIGN );
        if (Uart_p->OriginalFDMANode_p[1] != NULL)
        {
            Uart_p->RXFDMANode_p = (STFDMA_Node_t *) ((((U32) Uart_p->OriginalFDMANode_p[1] ) +UART_FDMA_ALIGN ) & ~UART_FDMA_ALIGN );
            Uart_p->RXFDMANode_p->Next_p = NULL;
            Uart_p->RXFDMANode_p->SourceAddress_p = (void *)((U32)&ASCRegs_p->ASCnRxBuffer & ST40_PHYSICAL_ACCESS_MASK);
            Uart_p->RXFDMANode_p->SourceStride = 0;
            Uart_p->RXFDMANode_p->DestinationStride = 0;
            Uart_p->RXFDMANode_p->NodeControl.PaceSignal = Uart_p->InitParams.RXDMARequestSignal;
            Uart_p->RXFDMANode_p->NodeControl.SourceDirection = STFDMA_DIRECTION_STATIC;
            Uart_p->RXFDMANode_p->NodeControl.DestinationDirection = STFDMA_DIRECTION_INCREMENTING;
            Uart_p->RXFDMANode_p->NodeControl.NodeCompleteNotify = FALSE;
            Uart_p->RXFDMANode_p->NodeControl.NodeCompletePause = FALSE;
            Uart_p->RXFDMANode_p->NodeControl.Reserved = 0;

            Uart_p->RXFDMAGenericTransferParams.ChannelId = STFDMA_USE_ANY_CHANNEL;  /* set later*/
            Uart_p->RXFDMAGenericTransferParams.Pool = STFDMA_DEFAULT_POOL;
            Uart_p->RXFDMAGenericTransferParams.CallbackFunction = STFDMA_NO_CALLBACK;
            Uart_p->RXFDMAGenericTransferParams.BlockingTimeout = 0;
            Uart_p->RXFDMAGenericTransferParams.ApplicationData_p = Uart_p;
            Uart_p->RXFDMAGenericTransferParams.NodeAddress_p
                = (STFDMA_GenericNode_t *) ((U32)Uart_p->RXFDMANode_p & ST40_PHYSICAL_ACCESS_MASK);

            /* reserve the channels */
            if (STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL,&Uart_p->TXFDMAChannelId, STFDMA_1) == ST_NO_ERROR)
            {
                Uart_p->TXFDMAGenericTransferParams.ChannelId = Uart_p->TXFDMAChannelId;
                if (STFDMA_LockChannelInPool(STFDMA_DEFAULT_POOL,&Uart_p->RXFDMAChannelId, STFDMA_1) == ST_NO_ERROR)
                {
                    Uart_p->RXFDMAGenericTransferParams.ChannelId = Uart_p->RXFDMAChannelId;
#if 1
                    STSYS_WriteRegDev32LE((U32*)(FDMA_BASE_ADDRESS + FDMA_REQ_CONTROL_BASE + Uart_p->InitParams.RXDMARequestSignal*FDMA_REQ_CONTROL_OFFSET),0x03400000);
                    STSYS_WriteRegDev32LE((U32*)(FDMA_BASE_ADDRESS + FDMA_REQ_CONTROL_BASE + Uart_p->InitParams.TXDMARequestSignal*FDMA_REQ_CONTROL_OFFSET),0x03404000);
#endif
                    if ((Uart_p->DMARXSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0)) != NULL)
                    {
                        if ((Uart_p->DMATXSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0)) != NULL)
                        {
                            if ((Uart_p->DMARXRequestSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0)) != NULL)
                            {
                                if ((Uart_p->DMARXCompleteSemaphore_p = STOS_SemaphoreCreateFifoTimeOut(NULL,0)) != NULL)
                                {
					 Error =  STOS_TaskCreate   ((void(*)(void *))UART_DMAReadTask,
									  			Uart_p, 
									  			(partition_t*)(Uart_p->InitParams.NCachePartition),
									  			UART_DMAREAD_STACK_SIZE,
									  			NULL,
									  			Uart_p->InitParams.NCachePartition,
									  			&Uart_p->DMAReadTask,
									  			NULL, 
									  			UART_DMAREAD_TASK_PRIORITY,
									  			"UART_DMARead", 
									  			(task_flags_t) task_flags_no_min_stack_size);

                                     if (Error  == ST_NO_ERROR)
                                    {
                                        return Error;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return ST_ERROR_NO_MEMORY;
}


/*****************************************************************************
UART_DMATerm()
Description:
    Releases all resources associated with the Uart DMA driver


Parameters:
    Uart_p,         pointer the the UART control block

Return Value:
    None
See Also:

*****************************************************************************/
void UART_DMATerm(UART_ControlBlock_t *Uart_p)
{
    /* should be no extant read or write here */
    if (Uart_p->InitParams.SwFIFOEnable)
    {
        STFDMA_AbortTransfer(Uart_p->RXFDMATransferId);
        /* non blocking, STFDMA_AbortTransfer() will return immediatly so wait for it to stop */
        STOS_TaskDelay(ST_GetClocksPerSecond()/100);
    }
    STFDMA_UnlockChannel( Uart_p->TXFDMAChannelId, STFDMA_1);
    STFDMA_UnlockChannel( Uart_p->RXFDMAChannelId, STFDMA_1);

    STOS_MemoryDeallocate(Uart_p->InitParams.NCachePartition, Uart_p->OriginalFDMANode_p[0]);
    STOS_MemoryDeallocate(Uart_p->InitParams.NCachePartition, Uart_p->OriginalFDMANode_p[1]);
    STOS_TaskDelete ( Uart_p->DMAReadTask,
					(partition_t*)Uart_p->InitParams.NCachePartition, 
					NULL,
					(partition_t*)Uart_p->InitParams.NCachePartition);
					
    STOS_SemaphoreDelete(NULL, Uart_p->DMATXSemaphore_p);
    STOS_SemaphoreDelete(NULL, Uart_p->DMARXSemaphore_p);
    STOS_SemaphoreDelete(NULL, Uart_p->DMARXRequestSemaphore_p);
    STOS_SemaphoreDelete(NULL, Uart_p->DMARXCompleteSemaphore_p);
}


/*****************************************************************************
UART_StartDMARX()
Description:
    Starts a DMA read transfer with callback (non blocking)


Parameters:
    Uart_p,         pointer the the UART control block
    Buffer          buffer to receive transfer
    Length          amont to receive

Return Value:
    None
See Also:

*****************************************************************************/
ST_ErrorCode_t UART_StartDMARX(UART_ControlBlock_t *Uart_p, U8 *Buffer, U32 Length)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;

    Uart_p->RXFDMANode_p->NumberBytes = Length;
    Uart_p->RXFDMANode_p->Length = Length;
    Uart_p->RXFDMANode_p->DestinationAddress_p = (void*) ( (U32)Buffer &  ST40_PHYSICAL_ACCESS_MASK);
    Uart_p->RXFDMAGenericTransferParams.CallbackFunction = UART_RXDMACallbackFunction;
    Uart_p->RXFDMAGenericTransferParams.BlockingTimeout = 0;
    Uart_p->RXFDMATransferId = 0;
    Error = STFDMA_StartGenericTransfer(&Uart_p->RXFDMAGenericTransferParams, &Uart_p->RXFDMATransferId);
    PrintDebugDMA(("UART_StartDMARX: STFDMA_StartGenericTransfer Length %d\n", Length));
#ifdef UARTDMA_DBGERR
    if (Error) PrintError(("UART_StartDMARX: STFDMA_StartGenericTransfer Error %X\n", Error));
#endif
    return Error;
}


/*****************************************************************************
UART_DMARead()
Description:
    Reads a block of data via DMA from a UART device.
    Performs a blocking or Non-blocking read.
Parameters:
    Uart_p,         pointer the the UART control block.
                    for.
    Buffer,         a buffer into which to put the read data.
    MinToRead,      maximum number of bytes to read going to HW FIFO
    MaxToRead,      maximum number of bytes to retrieve from SW FIFO
    NumberRead,     on return this contains the number of bytes that have been
                    read into the buffer.


Return Value:
    ST_NO_ERROR,                no errors.
    STUART_ERROR_ABORT,         the operation was aborted.
    STUART_ERROR_OVERRUN,       an internal read buffer has overrun.
    ST_ERROR_TIMEOUT,           timeout on read operation.
    STUART_ERROR_PARITY,        a parity error has occurred.
    STUART_ERROR_FRAMING,       a framing error has occurred.
See Also:

*****************************************************************************/
ST_ErrorCode_t UART_DMARead( UART_ControlBlock_t *Uart_p,
                            U8 *Buffer,
                            U32 MinToRead,
                            U32 MaxToRead,
                            U32 *NumberRead,
                            U32 TimeOut)
{
    U32 BytesAvailable;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    *NumberRead = 0;
    Uart_p->UartStatus &= ~UART_ABORT_RX;

    Uart_p->ReadOperation = &Uart_p->OperationPool[0];
    Uart_p->ReadOperation->UserBuffer = Buffer;
    Uart_p->ReadOperation->NumberRemaining = MaxToRead;
    Uart_p->ReadOperation->NumberBytes = NumberRead;
    Uart_p->ReadOperation->MinToRead = MinToRead;
    Uart_p->ReadOperation->Timeout = TimeOut;

    Uart_p->ReadOperation->NotifyRoutine =
        Uart_p->DefaultParams.RxMode.NotifyFunction;
    Uart_p->ReadOperation->Status = ST_NO_ERROR;

    if (Uart_p->InitParams.SwFIFOEnable)
    {
        /* try SWFifo */
        BytesAvailable = (U32)Uart_p->ReceivePut_p - (U32)Uart_p->ReceiveGet_p;
        if (BytesAvailable)
        {
            *NumberRead = MIN(BytesAvailable, MaxToRead);
            PrintDebug(("UDR: reclaimed S %X a %d n %d\n", (U32)Uart_p->ReceiveGet_p, BytesAvailable, *NumberRead));
            memcpy(Buffer, Uart_p->ReceiveGet_p, *NumberRead);
            Uart_p->ReceiveGet_p += *NumberRead;
        }
        if (*NumberRead < MaxToRead)
        {
            Uart_p->ReadOperation->DMARequested = Uart_p->InitParams.FIFOLength;
            STOS_SemaphoreSignal(Uart_p->DMARXRequestSemaphore_p);

            if (Uart_p->DefaultParams.RxMode.NotifyFunction == NULL)
            {
                STOS_SemaphoreWait(Uart_p->DMARXCompleteSemaphore_p);
                Error = Uart_p->ReadOperation->Status;
                Uart_p->ReadOperation = NULL;
            }
        }
        else Uart_p->ReadOperation = NULL;
    }
    else
    {
        Uart_p->ReadOperation->DMARequested = MaxToRead;
        STOS_SemaphoreSignal(Uart_p->DMARXRequestSemaphore_p);

        if (Uart_p->DefaultParams.RxMode.NotifyFunction == NULL)
        {
            STOS_SemaphoreWait(Uart_p->DMARXCompleteSemaphore_p);
            Error = Uart_p->ReadOperation->Status;
            Uart_p->ReadOperation = NULL;
        }
    }
#if STUART_STATISTICS
    /* Increment bytes transmitted statistics */
    Uart_p->Statistics.NumberBytesReceived += *NumberRead;
#endif
    return Error;
}


/*****************************************************************************
UART_DMAWrite()
Description:
    This routine transmits a block of data through DMA to the ASCnTxBuffer.
    Performs a blocking or non-blocking write.

Parameters:
    Uart_p,         pointer the the UART control block to set the baud rate
                    for.
    Buffer,         a buffer holding the data to be sent.
    NumberToWrite,  number of bytes to write to the device.
    NumberWritten,  updated with the number of bytes written.
Return Value:
    ST_NO_ERROR
    STUART_ERROR_ABORT
See Also:

*****************************************************************************/
ST_ErrorCode_t UART_DMAWrite(UART_ControlBlock_t *Uart_p,
                             U8 *Buffer,
                             U32 NumberToWrite,
                             U32 *NumberWritten,
                             U32 TimeOut)
{

    ST_ErrorCode_t Error = ST_NO_ERROR;

    Uart_p->UartStatus &= ~UART_ABORT_TX;
    if (NumberToWrite < 4)
    {
        UART_DMATXFlush( Uart_p, Buffer,NumberToWrite);
        *NumberWritten =  NumberToWrite;
        if (Uart_p->DefaultParams.TxMode.NotifyFunction)
        {
            Uart_p->DefaultParams.TxMode.NotifyFunction(Uart_p->Handle,ST_NO_ERROR);
        }
        return ST_NO_ERROR;
    }

    Uart_p->WriteOperation = &Uart_p->OperationPool[1];
    Uart_p->WriteOperation->UserBuffer = Buffer;
    Uart_p->WriteOperation->LastTransferSize = NumberToWrite;
    Uart_p->WriteOperation->DMARequested = NumberToWrite;
    Uart_p->WriteOperation->NumberBytes = NumberWritten;


    if (Uart_p->DefaultParams.TxMode.NotifyFunction)
    {
        Uart_p->WriteOperation->NotifyRoutine =
                Uart_p->DefaultParams.TxMode.NotifyFunction;
    }
    else
    {
        Uart_p->WriteOperation->NotifyRoutine = NULL;
    }
    Uart_p->TXFDMANode_p->NumberBytes = NumberToWrite & ~3; /* Can only DMA x 4 bytes */
    Uart_p->TXFDMANode_p->Length = Uart_p->TXFDMANode_p->NumberBytes;
    Uart_p->TXFDMANode_p->SourceAddress_p = (void*) ( (U32)Buffer &  ST40_PHYSICAL_ACCESS_MASK);

    Error = STFDMA_StartGenericTransfer(&Uart_p->TXFDMAGenericTransferParams, &Uart_p->TXFDMATransferId);
    #ifdef UARTDMA_DBGERR
    if (Error) PrintError(("UART_DMAWrite: STFDMA_StartGenericTransfer Error %X\n", Error));
    #endif
    if (Uart_p->WriteOperation->NotifyRoutine == NULL)
    {
        clock_t clk;

        clk = STOS_time_plus(STOS_time_now(), (ST_GetClocksPerSecond()/1000)*TimeOut);
        if (STOS_SemaphoreWaitTimeOut(Uart_p->DMATXSemaphore_p, TimeOut?&clk:TIMEOUT_INFINITY)!=0)
        {
            Uart_p->WriteOperation->DMARequested = 0;

            STFDMA_FlushTransfer(Uart_p->TXFDMATransferId);
            STOS_SemaphoreWait(Uart_p->DMATXSemaphore_p);
        }
        if (Uart_p->UartStatus & UART_ABORT_TX)
        {
            Error = STUART_ERROR_ABORT;
        }
        Uart_p->WriteOperation = NULL;
    }
#if STUART_STATISTICS
    /* Increment bytes transmitted statistics */
    Uart_p->Statistics.NumberBytesTransmitted += *NumberWritten;
#endif
    return Error;
}


/*****************************************************************************
UART_DMAAbort()
Description:
    Abort any pending IO through a UART DMA.

Parameters:
    Uart_p,         pointer the the UART control block.
    Direction,      the IO direction to abort (tx/rx/both)
Return Value:
    ST_NO_ERROR,                no errors.
See Also:

*****************************************************************************/
ST_ErrorCode_t UART_DMAAbort(UART_ControlBlock_t *Uart_p, STUART_Direction_t Direction)
{
    if (Uart_p->WriteOperation != NULL &&
        (Direction & STUART_DIRECTION_RECEIVE) != 0)
    {
        STFDMA_AbortTransfer(Uart_p->RXFDMATransferId);
    }
    if (Uart_p->WriteOperation != NULL &&
            (Direction & STUART_DIRECTION_TRANSMIT) != 0)
    {
        STFDMA_AbortTransfer(Uart_p->TXFDMATransferId);
    }

    return ST_NO_ERROR;
}


/*****************************************************************************
UART_DMAFlushReceive()
Description:
    Flush DME receive buffer when running with SWFifo. Called from STUART_Flush
    which will ensure no read is taking place and handle flushing the SWFifo and
    UART RX Fifo

Parameters:
    Uart_p,         pointer the the UART control block.
Return Value:
    ST_NO_ERROR,                no errors.
See Also:

*****************************************************************************/
ST_ErrorCode_t UART_DMAFlushReceive(UART_ControlBlock_t *Uart_p)
{
    if (Uart_p->InitParams.SwFIFOEnable)
    {
        STFDMA_AbortTransfer(Uart_p->RXFDMATransferId);
        STOS_SemaphoreWait(Uart_p->DMARXSemaphore_p);
    }
    return ST_NO_ERROR;
}


/*****************************************************************************
UART_TXDMACallbackFunction()
Description:
    Called on completion or abort of a DMA write.
    Signals the write routine or calls the users notify function.

Parameters:
    See STFDMA documents
Return Value:
    None
See Also:

*****************************************************************************/
void UART_TXDMACallbackFunction(U32 TransferId,
                            U32 CallbackReason,
                            U32 *CurrentNode_p,
                            U32 NodeBytesRemaining,
                            BOOL CBError,
                            void *ApplicationData_p,
                            osclock_t InterruptTime)
{
    UART_ControlBlock_t *Uart_p = (UART_ControlBlock_t *)ApplicationData_p;
    ST_ErrorCode_t Error = ST_NO_ERROR;

    if (CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED)
    {
        if (Uart_p->WriteOperation->DMARequested)
        {
            /* aborted via STUART_Abort */
            UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

            /* this is what the STUART spec says but no idea how many byte were init it ? */
            UART_ResetTxFIFO(ASCRegs_p);
            Uart_p->UartStatus |= UART_ABORT_TX;
            Error = STUART_ERROR_ABORT;
        }
        *(Uart_p->WriteOperation->NumberBytes) = (Uart_p->WriteOperation->LastTransferSize & ~3) - NodeBytesRemaining;
    }
    else
    {

        UART_DMATXFlush(Uart_p,
                        (U8 *)((U32)Uart_p->WriteOperation->UserBuffer
                                    + (Uart_p->WriteOperation->LastTransferSize & ~3) - NodeBytesRemaining),
                        Uart_p->WriteOperation->LastTransferSize % 4);
        *(Uart_p->WriteOperation->NumberBytes) = Uart_p->WriteOperation->LastTransferSize - NodeBytesRemaining;
    }

    if (Uart_p->WriteOperation->NotifyRoutine != NULL)
    {
        /* non blocking write, notify caller */
        Uart_p->WriteOperation->NotifyRoutine(Uart_p->Handle, Error);
        Uart_p->WriteOperation = NULL;
    }
    else
    {
        STOS_SemaphoreSignal(Uart_p->DMATXSemaphore_p);
    }
}


/*****************************************************************************
UART_DMATXFlush()
Description:
    Flushes data from a buffer into the UART Tx buffer

Parameters:
    Uart_p,         pointer the the UART control block.
    Buffer          buffer containing data.
    NumberToWrite   number of bytes to write
Return Value:
    None
See Also:

*****************************************************************************/
__inline void UART_DMATXFlush(UART_ControlBlock_t *Uart_p,
                              U8 *Buffer,
                              U32 NumberToWrite)
{
    volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
    U32 i;

    for(i=0;i<NumberToWrite;i++)
    {
        ASCRegs_p->ASCnTxBuffer = *Buffer++;
    }
}


/*****************************************************************************
UART_DMAReadTask
Description:
    Task to service DMA reads

Parameters:
    Uart_p,         pointer the the UART control block.
Return Value:
    None
See Also:

*****************************************************************************/
void UART_DMAReadTask(UART_ControlBlock_t *Uart_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    STFDMA_TransferStatus_t TransferStatus;
    clock_t clk;
    clock_t * clk_p;
    BOOL IgnoreAbort;

    PrintDebug(("UART_DMAReadTask started\n"));

    while(1)
    {
        STOS_SemaphoreWaitTimeOut(Uart_p->DMARXRequestSemaphore_p,TIMEOUT_INFINITY);

        if (Uart_p->InitParams.SwFIFOEnable)
        {
            /* stop current transfer, may have already finished */
            IgnoreAbort = FALSE;
            Error = STFDMA_FlushTransfer(Uart_p->RXFDMATransferId);
            PrintDebugDMA(("UART_DMAReadTask: STFDMA_AbortTransfer\n"));
            /* a flush results in abort status seen at call back */
            if (!Error) IgnoreAbort = TRUE;
            #ifdef UARTDMA_DBGERR
            if (Error) PrintError(("UART_DMAReadTask: STFDMA_FlushTransfer Err %X Id %d\n", Error, Uart_p->RXFDMATransferId));
            #endif
            Error = ST_NO_ERROR;

            while (*(Uart_p->ReadOperation->NumberBytes) < Uart_p->ReadOperation->NumberRemaining)
            {
                if (!Uart_p->ReadOperation->Timeout || Uart_p->ReadOperation->NotifyRoutine)
                {
                    clk_p = TIMEOUT_INFINITY;
                }
                else
                {
                    clk = STOS_time_plus(STOS_time_now(), (ST_GetClocksPerSecond()/1000)*Uart_p->ReadOperation->Timeout);
                    clk_p = &clk;
                }
                if (STOS_SemaphoreWaitTimeOut(Uart_p->DMARXSemaphore_p,clk_p)!=0)
                {
                    Error = ST_ERROR_TIMEOUT;
                    /* timeout should only occur on 2nd pass
                     * this flush will result in an abort status which is required
                     * to stop trying to get more on this read
                     */
                    STFDMA_FlushTransfer(Uart_p->RXFDMATransferId);

                    PrintDebugDMA(("UART_DMAReadTask Timeout: STFDMA_FlushTransfer\n"));
                    STOS_SemaphoreWait(Uart_p->DMARXSemaphore_p);
                }
                if (IgnoreAbort) /* is only set on 1st pass */
                {
                    Uart_p->UartStatus &= ~UART_ABORT_RX;
                    IgnoreAbort = FALSE;
                }
                UART_DMAFlushBufferAndRestart(Uart_p);
                if (Error == ST_ERROR_TIMEOUT)
                {
                    PrintDebug(("UDR: Timeout\n"));
                    break;
                }
            }
        }
        else
        {
            UART_StartDMARX(Uart_p, Uart_p->ReadOperation->UserBuffer, Uart_p->ReadOperation->MinToRead);

            if (!Uart_p->ReadOperation->Timeout || Uart_p->ReadOperation->NotifyRoutine)
            {
                clk_p = TIMEOUT_INFINITY;
            }
            else
            {
                clk = STOS_time_plus(STOS_time_now(), (ST_GetClocksPerSecond()/1000)*Uart_p->ReadOperation->Timeout);
                clk_p = &clk;
            }
            if (STOS_SemaphoreWaitTimeOut(Uart_p->DMARXSemaphore_p,clk_p)!=0)
            {
                /* timed out so read what we have */
                Error = STFDMA_FlushTransfer(Uart_p->RXFDMATransferId);

                #ifdef UARTDMA_DBGERR
                if (Error) PrintError(("UART_DMAReadTask: STFDMA_FlushTransfer Err %X Id\n", Error, Uart_p->RXFDMATransferId));
                #endif
                if (!Error)
                {
                    STOS_SemaphoreWait(Uart_p->DMARXSemaphore_p);
                    /* a flush results in abort status seen at call back */
                    Uart_p->UartStatus &= ~UART_ABORT_RX;
                }
               Error = ST_ERROR_TIMEOUT;
            }

            *(Uart_p->ReadOperation->NumberBytes) = Uart_p->ReadOperation->DMARequested -Uart_p->RXDMANodeBytesRemaining;
            if (Uart_p->RXDMANodeBytesRemaining && !(Uart_p->UartStatus & UART_ABORT_RX))
            {
                U32 i;
                U8 *Buffer = Uart_p->ReadOperation->UserBuffer + Uart_p->ReadOperation->DMARequested - Uart_p->RXDMANodeBytesRemaining;
                U32 Residue = MIN(8,Uart_p->RXDMANodeBytesRemaining);
                volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

                for(i=0;i<Residue;i++)
                {
                    if (UART_IsRxFull(ASCRegs_p))
                    {
                        *Buffer++ = ASCRegs_p->ASCnRxBuffer;
                        *(Uart_p->ReadOperation->NumberBytes) += 1;
                    }
                    else break;
                }
            }
        }
        if (Error == TRUE )
        {
            if(Uart_p->UartStatus & UART_ABORT_RX)
            {
               Error = STUART_ERROR_ABORT;
            }
            else
            {
               Error = UART_RXDMACheckErrors(Uart_p);
            }

        }
        if (Uart_p->ReadOperation->NotifyRoutine != NULL)
        {
            /* non blocking read, notify caller */
            Uart_p->ReadOperation->NotifyRoutine(Uart_p->Handle, Error);
            Uart_p->ReadOperation = NULL;
        }
        else
        {
            Uart_p->ReadOperation->Status = Error;
            STOS_SemaphoreSignal(Uart_p->DMARXCompleteSemaphore_p);
        }
    }
}


/*****************************************************************************
UART_DMAFlushBufferAndRestart
Description:
    reads available data from the DMA reeive buffer and starts another receive
    transfer

Parameters:
    Uart_p,         pointer the the UART control block.
Return Value:
    None
See Also:

*****************************************************************************/
void UART_DMAFlushBufferAndRestart( UART_ControlBlock_t *Uart_p)
{
    U32 BytesAvailable, BytesToRead;
    U32 *NumberRead = Uart_p->ReadOperation->NumberBytes;

#if 0
    if ((Uart_p->ReadOperation->DMARequested > 4) &&
        (Uart_p->ReadOperation->DMARequested != Uart_p->InitParams.FIFOLength))
    {
        Uart_p->RXDMANodeBytesRemaining += 4;
    }
#endif
    BytesAvailable = Uart_p->ReadOperation->DMARequested - Uart_p->RXDMANodeBytesRemaining;
    if (Uart_p->RXDMANodeBytesRemaining)
    {
        U32 i;
        U8 *Buffer = Uart_p->DMAReceiveBufferBase + BytesAvailable;
        U32 Residue = MIN(8,Uart_p->RXDMANodeBytesRemaining);
        volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

        for(i=0;i<Residue;i++)
        {
            if (UART_IsRxFull(ASCRegs_p))
            {
                *Buffer++ = ASCRegs_p->ASCnRxBuffer;
                BytesAvailable++;
            }
            else break;
        }
    }

    BytesToRead = MIN((Uart_p->ReadOperation->NumberRemaining - *NumberRead),BytesAvailable);
    memcpy((U8 *)((U32)Uart_p->ReadOperation->UserBuffer + *NumberRead), Uart_p->DMAReceiveBufferBase, BytesToRead);

    *NumberRead += BytesToRead;
    /* used all we have got, only need to satisfy min required */
    Uart_p->ReadOperation->NumberRemaining = Uart_p->ReadOperation->MinToRead;
    if ((*NumberRead < Uart_p->ReadOperation->NumberRemaining) && !(Uart_p->UartStatus & UART_ABORT_RX))
    {
        /* need more, start another transfer */
        U32 ActualRequested;

        Uart_p->ReadOperation->DMARequested
            = MIN((Uart_p->ReadOperation->NumberRemaining - *NumberRead),Uart_p->InitParams.FIFOLength);
#if 0
        if ((Uart_p->ReadOperation->DMARequested > 4) &&
            (Uart_p->ReadOperation->DMARequested != Uart_p->InitParams.FIFOLength))
        {
            ActualRequested = Uart_p->ReadOperation->DMARequested - 4;
        }
        else
#endif
        ActualRequested = Uart_p->ReadOperation->DMARequested;
        UART_StartDMARX(Uart_p, Uart_p->DMAReceiveBufferBase,
                        ActualRequested);
        PrintDebug(("UDF: more %d %d\n", Uart_p->ReadOperation->DMARequested, ActualRequested));
    }
    else
    {
        if (BytesAvailable > BytesToRead)
        {
            /* save unused - SWFifo is empty here and always same size as DMA buffer */
            memcpy(Uart_p->ReceiveBufferBase, (U8 *)((U32)Uart_p->DMAReceiveBufferBase + BytesToRead),
                    BytesAvailable - BytesToRead );
            Uart_p->ReceiveGet_p = Uart_p->ReceiveBufferBase;
            Uart_p->ReceivePut_p = (U8 *)((U32)Uart_p->ReceiveBufferBase + BytesAvailable - BytesToRead);

            PrintDebug(("UDF: saved  %d\n", BytesAvailable - BytesToRead));
        }

        /* start transfer for next read */
        UART_StartDMARX(Uart_p, Uart_p->DMAReceiveBufferBase, Uart_p->InitParams.FIFOLength);
        PrintDebug(("UDF: restart %d\n", Uart_p->InitParams.FIFOLength ));
    }
}


/*****************************************************************************
UART_RXDMACallbackFunction()
Description:
    Called on completion or abort of a DMA read.
    Reads data available and starts another transfor if a SWFifo exists.
    Signals the red routine or calls the users notify function on completion.

Parameters:
    See STFDMA documents
Return Value:
    None
See Also:

*****************************************************************************/
void UART_RXDMACallbackFunction(U32 TransferId,
                            U32 CallbackReason,
                            U32 *CurrentNode_p,
                            U32 NodeBytesRemaining,
                            BOOL Error,
                            void *ApplicationData_p,
                            osclock_t InterruptTime)
{
    UART_ControlBlock_t *Uart_p = (UART_ControlBlock_t *)ApplicationData_p;

    PrintDebugDMA(("UART_RXDMACallbackFunction Remaining %d Reason %d\n", NodeBytesRemaining, CallbackReason));
    if (CallbackReason == STFDMA_NOTIFY_TRANSFER_ABORTED)
    {
        Uart_p->UartStatus |= UART_ABORT_RX;
    }

    Uart_p->RXDMANodeBytesRemaining = NodeBytesRemaining;
    PrintDebug(("CA: rem %d\n",  NodeBytesRemaining ));
    STOS_SemaphoreSignal(Uart_p->DMARXSemaphore_p);
}


/*****************************************************************************
UART_RXDMACheckErrors()
Description:
    Checks the uart status register for receive errors.

Parameters:
    Uart_p,         pointer the the UART control block.
Return Value:
    STUART_ERROR_OVERRUN
    STUART_ERROR_FRAMING
    STUART_ERROR_PARITY
    ST_NO_ERROR
See Also:

*****************************************************************************/
__inline ST_ErrorCode_t UART_RXDMACheckErrors( UART_ControlBlock_t *Uart_p)
{
    UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

#if STUART_STATISTICS
          /* Increment overrun statistics */
        if (ASCRegs_p->ASCnStatus & STATUS_OVERRUNERROR)
        {
             Uart_p->ReceiveError = STUART_ERROR_OVERRUN;
             Uart_p->Statistics.NumberOverrunErrors++;
        }
        if (ASCRegs_p->ASCnStatus & STATUS_FRAMEERROR)
        {
           /* Increment framing error statistics */
             Uart_p->ReceiveError = STUART_ERROR_FRAMING;
             Uart_p->Statistics.NumberFramingErrors++;
        }
        if (ASCRegs_p->ASCnStatus & STATUS_PARITYERROR)
        {
            Uart_p->ReceiveError  = STUART_ERROR_PARITY;
          /* Increment parity error statistics */
            Uart_p->Statistics.NumberParityErrors++;
        }
#else

    if (ASCRegs_p->ASCnStatus & STATUS_OVERRUNERROR)return STUART_ERROR_OVERRUN;
    if (ASCRegs_p->ASCnStatus & STATUS_FRAMEERROR) return STUART_ERROR_FRAMING;
    if (ASCRegs_p->ASCnStatus & STATUS_PARITYERROR) return STUART_ERROR_PARITY;
#endif
    return ST_NO_ERROR;
}
/* End of uartdma.c */
