/*****************************************************************************
File Name   : api.c

Description : UART driver.

Copyright (C) 2001 STMicroelectronics

History     :

    10/10/99  Bug fix to ISR TXHALFEMPTY status bit should not be checked
              unless FIFOs are enabled.

    18/11/99  Re-worked rx error handling code; any error will complete the
              current read.  The offending byte may be received
              with a subsequent read call e.g., parity/framing error.

    29/11/99  Migrated ASC register definitions to separate header file.
              Migrated timer functions to separate module.
              Migrated device access / interrupt handler to separate module.

    02/12/99  New API routine for interrogating actual baudrate (takes into
              account baudrate error as a result of clock division).

              Removed global queue access semaphore - now done with much
              simpler interrupt_lock mechanism.

              All register setup now done via ascregs.h macros.

    07/12/99  Improved error handling - errors that occur without a pending
              read/write in progress are 'latched' and notified to the
              caller on the next read/write allowing corrective
              action to be taken.  This ensures that errors are not
              'missed' by the software.

    09/12/99  Added overridable define for the timer task priority.

    01/03/00  Bug fixed during init code -- by default the txd pin should
              be configured as an output (not bidirectional) otherwise it
              is possible to get spurious activity on the transmit line.

    03/04/00  Calls to pio open assumed that both txd and rxd pins are
              specified.  This may not be the case, however, for example
              on the STi5508 for sc0 and sc1 interfaces -- these only
              use one pio pin for both tx and rx, half duplex.

    23/08/00  Invalid pointer dereference bugs fixed in Open() and Init()
              routines where DefaultParams is set to NULL.

    03/12/01  Added STi5514 additional functionality for SmartCard mode
              using device type ISO7816

    30/09/03  STUART driver has now been made multithreadsafe.InitSemaphore
              & LockSemaphore makes API functions atomic.

    25/03/04  STUART_EnableStuffing & STUART_DisableStuffing APIs have been
              added to provide Stuffing capability.

    29/06/04  Removed the support of #pragma ST_device for C1 core.

    30/08/04  Ported to OS21 & removed the common LockSemaphore from Read &
              Write. Now ReadSemaphore & WriteSemaphore protect Read & Write
              Operations respectively.

    24/12/04  Added multiinstance support(DDTS 35847).

Reference   :

ST API Definition "UART Driver API" DVD-API-22
*****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <string.h>                     /* C libs */
#include <stdio.h>

#include "stlite.h"                     /* Standard includes */
#include "stddefs.h"
#include "stos.h"

#include "stuart.h"                     /* STAPI */
#include "stcommon.h"

#ifdef STUART_DMA_SUPPORT
#include "stfdma.h"
#include "uartdma.h"                    /* DMA support */
#include "stsys.h"
#endif

#include "ascregs.h"                    /* ASC register definitions/macros */
#include "uarttimer.h"                  /* Timer mechanism */
#include "uartdat.h"                    /* Private data */
#include "uartdrv.h"                    /* Private functions / ISR */



/* Private types/constants ------------------------------------------------ */

/* All timer tasks created by the driver have a configurable task
 * priority.  This may be overrided in an external system header file.
 */
#ifndef STUART_TIMER_TASK_PRIORITY
#define STUART_TIMER_TASK_PRIORITY      MAX_USER_PRIORITY
#endif

/* Private Variables ------------------------------------------------------ */

#ifdef STUART_PIOCOMPARE_STATS
U32 UART_PioCompareReceived = 0;
#endif

/* Driver revision number */
static const ST_Revision_t Revision = "STUART-REL_1.8.9A0";

/*****************************************************************************
The UartQueueHead_p points to the head of the queue of UART devices that have
been initialized i.e., STUART_Init() has been called to append them
to the queue.
At system startup, UartQueueHead_p will be NULL.  When the first UART is
initialized, UartQueueHead_p will point to the UART control block for that
UART once STUART_Init() has been called.
*****************************************************************************/

 /* Queue of initialized UARTs */
static UART_ControlBlock_t *UartQueueHead_p = NULL;
static semaphore_t *InitSemaphore_p = NULL;

/* Private Macros --------------------------------------------------------- */

#ifdef MIN                              /* Undefine for this module only */
#undef MIN
#endif

#define MIN(x,y)                    ((x<y)?x:y)


/* Private Functions Prototypes -------------------------------------------- */

/* UART Queue Management */
static void UART_QueueAdd(UART_ControlBlock_t *QueueItem_p);
static void UART_QueueRemove(UART_ControlBlock_t *QueueItem_p);
static BOOL UART_IsInit(const ST_DeviceName_t DeviceName);
static UART_ControlBlock_t *
   UART_GetControlBlockFromName(const ST_DeviceName_t);
static UART_ControlBlock_t *
   UART_GetControlBlockFromHandle(STUART_Handle_t Handle);

/* Parameter setting/checking */
static ST_ErrorCode_t UART_CheckParams(const STUART_InitParams_t* InitParams,
                                       STUART_Params_t* Params);

static ST_ErrorCode_t UART_CheckProtocol(STUART_Device_t DeviceType,
                                         STUART_Protocol_t *Protocol_p);
static ST_ErrorCode_t UART_CheckSmartCardConfig(STUART_Device_t DeviceType,
                                                STUART_Params_t *Params_p);
/* Timer callback functions */
static void UART_ReadCallback(UART_ControlBlock_t *Uart_p);
static void UART_WriteCallback(UART_ControlBlock_t *Uart_p);

/* PIO Compare interrupt handler */
void UART_PioIntHandler(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits);

/* STUART API routines (alphabetical order) ------------------------------- */

/*****************************************************************************
STUART_Abort()
Description:
    Abort any pending IO through a UART device associated with a handle.

Parameters:
    Handle, the device handle.
    Direction, the IO direction to abort (tx/rx/both)
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    the device handle is invalid.
See Also:
    STUART_Read()
    STUART_Write()

*****************************************************************************/


ST_ErrorCode_t STUART_Abort(STUART_Handle_t Handle, STUART_Direction_t Direction)
{
#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif
    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;
    volatile UART_ASCRegs_t *ASCRegs_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    if (Uart_p != NULL)
    {
    
#ifdef STUART_DMA_SUPPORT
        {
            error=UART_DMAAbort(Uart_p, Direction);
            return error;
        }
#endif


        /* Disable interrupts */
        ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
        UART_ReceiverDisable(ASCRegs_p);
        UART_GlobalDisableInts(ASCRegs_p);

        /* Check for an outstanding read operation */
        if (Uart_p->ReadOperation != NULL &&
            (Direction & STUART_DIRECTION_RECEIVE) != 0)
        {
            /* Ensure the read timer is not paused, otherwise we
             * must resume it first.
             */
            if ((Uart_p->UartStatus & UART_PAUSE_RX) != 0)
                stuart_TimerPause(Uart_p->ReadOperation->Timer, FALSE);

            /* Set the status to aborted and signal the timer */
            Uart_p->ReadOperation->Status = STUART_ERROR_ABORT;
            stuart_TimerSignal(Uart_p->ReadOperation->Timer, FALSE);
        }

        /* Check for an outstanding write operation */
        if (Uart_p->WriteOperation != NULL &&
            (Direction & STUART_DIRECTION_TRANSMIT) != 0)
        {
            STPIO_Compare_t         cmp = {0,0};

            /* stop any pio compare happening */
            Uart_p->WriteAwaitingPioCompare = FALSE;
            STPIO_SetCompare(Uart_p->TXDHandle, &cmp);

            /* Ensure the write timer is not paused, otherwise we
             * must resume it first.
             */
            if ((Uart_p->UartStatus & UART_PAUSE_TX) != 0)
                stuart_TimerPause(Uart_p->WriteOperation->Timer, FALSE);

            /* Clean out the transmit fifo (still allows any byte currently in
              the transmit shift register to complete). We know we won't
              clobber data from a previous operation because they
              don't complete until all data has left the FIFO */

            UART_ResetTxFIFO(ASCRegs_p);

            /* Set the status to aborted and signal the timer. From then on
              we mustn't attempt to use WriteOperation */
            Uart_p->WriteOperation->Status = STUART_ERROR_ABORT;
            stuart_TimerSignal(Uart_p->WriteOperation->Timer, FALSE);
        }

        /* Re-enable interrupts */
        UART_ReceiverEnable(ASCRegs_p);
        UART_EnableInts(ASCRegs_p,
                        Uart_p->IntEnable);
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;
} /* STUART_Abort() */

/*****************************************************************************
STUART_Close()
Description:
    Closes a UART device associated with a handle.
    NOTE: If a read or write operation is outstanding, then it will be
    aborted prior to closing.
Parameters:
    Handle, handle as returned by STUART_Open() of device to close.
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.
See Also:
    STUART_Open()
*****************************************************************************/

ST_ErrorCode_t STUART_Close(STUART_Handle_t Handle)
{
#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif
    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;
    volatile UART_ASCRegs_t *ASCRegs_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    if (Uart_p != NULL)
    {

        if (Uart_p->OpenCount == 0)
        {
            /* Abort all activity on the UART */
            STUART_Abort(Handle, STUART_DIRECTION_BOTH);

            /* Nullify the handle to ensure no further API calls can be made
             * with the handle.
             */
            Uart_p->Handle = 0;

            /* Disable the UART i.e., disable interrupts and receiver */
            ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
            UART_GlobalDisableInts(ASCRegs_p);

            /*UART_ResetTxFIFO(ASCRegs_p); - already done in Abort */
            UART_ResetRxFIFO(ASCRegs_p);
            UART_ReceiverDisable(ASCRegs_p);

            /* Clear the open bit from the status flags */
            Uart_p->UartStatus &= ~UART_OPEN;
        }
        else
        {
            Uart_p->OpenCount--;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    return error;
} /* STUART_Close() */

/*****************************************************************************
STUART_DisableStuffing()
Description:
    Stop stuffing operation
Parameters:
    Handle, the device handle.
    Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    the device handle is invalid.
See Also:
    STUART_EnableStuffing()
*****************************************************************************/

ST_ErrorCode_t STUART_DisableStuffing (STUART_Handle_t Handle)
{

    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    if (Uart_p != NULL)
    {

        Uart_p->StuffingEnabled = FALSE;

    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_DisableStuffing() */

/*****************************************************************************
STUART_EnableStuffing()
Description:
    Send stuffing byte to keep a modem connection alive
Parameters:
    Handle, the device handle.
    StuffByte, the byte value to be sent as stuffing
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    the device handle is invalid.
See Also:
    STUART_DisableStuffing()
*****************************************************************************/

ST_ErrorCode_t STUART_EnableStuffing (STUART_Handle_t Handle,U8 StuffByte)
{


    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    if (Uart_p != NULL)
    {
        Uart_p->StuffingEnabled = TRUE;
        Uart_p->StuffingValue   = StuffByte;
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_EnableStuffing() */

/*****************************************************************************
STUART_Flush()
Description:
    Flushes the input buffer.

Parameters:
    Handle, the handle of the UART whose input buffer is to be flushed.
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.
    STUART_ERROR_BUSY,          a read operation is in progress.

See Also:
    STUART_Read()
    STUART_Open()

*****************************************************************************/

ST_ErrorCode_t STUART_Flush(STUART_Handle_t Handle)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;
    volatile UART_ASCRegs_t *ASCRegs_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        /* Ensure a read is not in progress */
        if (Uart_p->ReadOperation == NULL)
        {
            /* Stop receiving to ensure we don't receive any characters
             * during the flush operation.
             */
            ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
            UART_GlobalDisableInts(ASCRegs_p);
            UART_ReceiverDisable(ASCRegs_p);

#ifdef STUART_DMA_SUPPORT
          
                UART_DMAFlushReceive(Uart_p);
           
#endif
            /* Flush software FIFO */
            Uart_p->ReceiveBufferCharsFree = Uart_p->InitParams.FIFOLength;
            Uart_p->ReceivePut_p = Uart_p->ReceiveGet_p =
                                   Uart_p->ReceiveBufferBase;

            /* Flush UART receive FIFO */
            UART_ResetRxFIFO(ASCRegs_p);
            Uart_p->ReceiveError = ST_NO_ERROR;

            /* Re-enable receiver */
            UART_ReceiverEnable(ASCRegs_p);
            UART_EnableInts(ASCRegs_p,
                            Uart_p->IntEnable);
        }
        else
        {
            /* We do not allow a flush whilst a read is in progress */
            error = ST_ERROR_DEVICE_BUSY;

        }

    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_Flush() */

/*****************************************************************************
STUART_GetActualBaudRate()
Description:
    Whenever the baudrate is set via the UART_BaudRate() function, the
    actual baudrate (calculated by considering the error introduced from
    clock division) is set in the UART control block.

    The value is returned to the caller to allow the to compare the
    actual baudrate against the ideal (required) baudrate.

    In some cases, the baudrate error (especially for higher baudrates) may
    be sufficient to cause communications problems.

Parameters:
    Handle,         handle used to access the device.
    ActualBaudRate, pointer to a area for storing the actual baudrate.

Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.

See Also:
    UART_BaudRate()
*****************************************************************************/

ST_ErrorCode_t STUART_GetActualBaudRate(STUART_Handle_t Handle,
                                        U32 *ActualBaudRate
                                       )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        /* Copy the parameters in the control block */
        *ActualBaudRate = Uart_p->ActualBaudRate;
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_GetActualBaudRate() */

/*****************************************************************************
STUART_GetParams()
Description:
    Gets the currently active parameters for the UART associated with the
    handle.

Parameters:
    Handle, handle used to access the device.
    Params, pointer to a parameter structure for storing the parameters in
            use.
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.
See Also:
    STUART_SetParams()
*****************************************************************************/

ST_ErrorCode_t STUART_GetParams(STUART_Handle_t Handle,
                                STUART_Params_t *Params
                               )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        /* Copy the parameters in default params */
        *Params = Uart_p->DefaultParams;
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_GetParams() */

#if STUART_STATISTICS

/*****************************************************************************
STUART_GetStatistics()
Description:
    Gets the current statistics for a UART.

    NOTE: This API call is not part of the official driver API, but may be
    included for debug purposes.

Parameters:
    Handle,     handle used to access the device.
    Stats_p,    pointer to a statistics structure for storing the current
                statistics.
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.
See Also:
    Nothing.
*****************************************************************************/

ST_ErrorCode_t STUART_GetStatistics(STUART_Handle_t Handle,
                                    STUART_Statistics_t *Stats_p)
{
    ST_ErrorCode_t Error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        /* Copy the current statistics for this UART */
        *Stats_p = Uart_p->Statistics;
    }
    else
    {
        /* The handle is invalid */
        Error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return Error;

} /* STUART_GetStatistics() */

#endif /* STUART_STATISTICS */

/*****************************************************************************
STUART_GetRevision()
Description:
    Obtains the revision number of the driver.
Parameters:
    None.
Return Value:
    The revision of the device driver.
See Also:
    Nothing.
*****************************************************************************/

ST_Revision_t STUART_GetRevision(void)
{
    /* Simply return the global revision string */
    return Revision;

} /* STUART_GetRevision() */

/*****************************************************************************
STUART_Init()
Description:
    Initializes the named UART device.

    NOTES: The very first call to this routine will create a semaphore
    for locking access to the UART queue.  The semaphore will be deleted
    during the final call to STUART_Term().

Parameters:
    DeviceName, text string indicating the device name to be used.
    InitParams, parameters to control the initialization of the device.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_NO_MEMORY,             unable to allocate memory.
    ST_ERROR_BAD_PARAMETER,         one or more of the parameters was invalid.
    ST_ERROR_ALREADY_INITIALIZED,   another user has already initialized the
                                    device.
    ST_ERROR_INTERRUPT_INSTALL,     error installing interrupts for the driver's
                                    internal interrupt handler.
    ST_ERROR_FEATURE_NOT_SUPPORTED, the device type is not supported.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name (for PIO).
    ST_ERROR_NO_FREE_HANDLES,       another user has already opened the device,
                                    or the device is in use (for PIO).
See Also:
    STUART_Term()
*****************************************************************************/

ST_ErrorCode_t STUART_Init(const ST_DeviceName_t DeviceName,
                           const STUART_InitParams_t *InitParams
                          )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    partition_t *DriverPartition = NULL;
    BOOL TXRXInit = FALSE;
    BOOL RTSInit = FALSE;
    BOOL CTSInit = FALSE;
    BOOL MemoryInit = FALSE;
    BOOL TimerInit = FALSE;
    BOOL SwFIFOInit = FALSE;
    UART_ControlBlock_t *Uart_p;
    BOOL SemaphoreInit = FALSE;

    STOS_TaskLock();
    if (InitSemaphore_p == NULL)
    {
	InitSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);
    }
    STOS_TaskUnlock();


    /* Check the parameters passed to init. */
    if (strlen(DeviceName) <= ST_MAX_DEVICE_NAME &&
        InitParams->BaseAddress != NULL &&
        InitParams->UARTType >= STUART_NO_HW_FIFO &&
        InitParams->UARTType < STUART_UNKNOWN_DEVICE
       )
    {
        STOS_SemaphoreWait(InitSemaphore_p);


        /* Run through the queue of UARTs to ensure this UART is free */
        if (!UART_IsInit(DeviceName))
        {

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif
            STPIO_OpenParams_t TXDOpenParams, RXDOpenParams;
            STPIO_OpenParams_t RTSOpenParams, CTSOpenParams;
            ST_ErrorCode_t PIOError;
            volatile UART_ASCRegs_t *ASCRegs_p;

            /* Allocate memory for the new queue item and associated UART
             * control block.
             */
            DriverPartition = (partition_t *)InitParams->DriverPartition;

            Uart_p = (UART_ControlBlock_t *)STOS_MemoryAllocate(
                      DriverPartition,
                      sizeof(UART_ControlBlock_t));

            /* Ensure memory allocation was successful */
            if (Uart_p != NULL)
            {
                /* Memory has now been allocated */
                MemoryInit = TRUE;

                /* Fill in the details of our new UART control block */
                Uart_p->Handle    = 0;
                Uart_p->OpenCount = 0;
                strncpy(Uart_p->DeviceName,
                        DeviceName,
                        ST_MAX_DEVICE_NAME);

                Uart_p->InitParams = *InitParams;
                Uart_p->IsHalfDuplex = FALSE; /* default, may change below */

                /* If device uses PIO */
                if (InitParams->RXD.PortName[0] != 0 &&
                    InitParams->TXD.PortName[0] != 0)
                {
                    /* Check whether or not the device has been configured
                     * as half-duplex i.e., TXD and RXD are the same.
                     */
                    if (strncmp(InitParams->RXD.PortName,
                                InitParams->TXD.PortName,
                                ST_MAX_DEVICE_NAME) == 0 &&
                        InitParams->RXD.BitMask == InitParams->TXD.BitMask)
                    {
                        /* This is a half-duplex interface */
                        Uart_p->IsHalfDuplex = TRUE;

                        /* Configure TXD as bidirectional -- don't use RXDHandle */
                        TXDOpenParams.BitConfigure[
                                UART_PIOBitFromBitMask(InitParams->TXD.BitMask)] =
                            STPIO_BIT_ALTERNATE_BIDIRECTIONAL;
                    }
                    else
                    {
                        /* Configure TXD and RXD open parameters -- full duplex */
                        TXDOpenParams.BitConfigure[
                                UART_PIOBitFromBitMask(InitParams->TXD.BitMask)] =
                                STPIO_BIT_ALTERNATE_OUTPUT;
                        RXDOpenParams.BitConfigure[
                                UART_PIOBitFromBitMask(InitParams->RXD.BitMask)] =
                                STPIO_BIT_INPUT;
                    }

                    /* Configure open parameters */
                    TXDOpenParams.ReservedBits = InitParams->TXD.BitMask;
                    RXDOpenParams.ReservedBits = InitParams->RXD.BitMask;
                    TXDOpenParams.IntHandler = UART_PioIntHandler;
                    RXDOpenParams.IntHandler = NULL;

                    /* Open TXD PIO */
                    PIOError = STPIO_Open(InitParams->TXD.PortName,
                                          &TXDOpenParams,
                                          &Uart_p->TXDHandle);

                    /* Ensure TXD open succeeded */
                    if (PIOError == ST_NO_ERROR)
                    {
                        /* Open RXD PIO */
                        if (!Uart_p->IsHalfDuplex)
                        {
                            PIOError = STPIO_Open(InitParams->RXD.PortName,
                                                  &RXDOpenParams,
                                                  &Uart_p->RXDHandle);

                            /* Ensure RXD open succeeded */
                            if (PIOError == ST_NO_ERROR)
                            {
                                TXRXInit = TRUE;
                            }
                            else
                            {
                                /* Tidy up PIO handle */
                                (void)STPIO_Close(Uart_p->TXDHandle);
                                error = STUART_ERROR_PIO;
                            }
                        }
                    }
                    else
                    {
                        error = STUART_ERROR_PIO;
                    }
                }

                /* If the device type supports RTS/CTS then configure
                 * the PIO pins appropriately.
                 */

                if (error == ST_NO_ERROR &&
                    UART_IsRTSCTSSupported(InitParams->UARTType) &&
                    InitParams->RTS.PortName[0] != 0)
                {
                    /* Configure RTS open parameters */
                    RTSOpenParams.BitConfigure[
                            UART_PIOBitFromBitMask(InitParams->RTS.BitMask)
                            ] = STPIO_BIT_ALTERNATE_OUTPUT;
                    RTSOpenParams.ReservedBits = InitParams->RTS.BitMask;
                    RTSOpenParams.IntHandler = NULL;

                    /* Open RTS PIO */
                    PIOError = STPIO_Open(InitParams->RTS.PortName,
                                          &RTSOpenParams,
                                          &Uart_p->RTSHandle);

                    /* Ensure RTS open succeeded */
                    if (PIOError == ST_NO_ERROR)
                    {
                        RTSInit = TRUE;
                    }
                    else
                    {
                        /* The PIO open call failed for some reason */
                        error = STUART_ERROR_PIO;
                    }
                }

                if (error == ST_NO_ERROR &&
                    UART_IsRTSCTSSupported(InitParams->UARTType) &&
                    InitParams->CTS.PortName[0] != 0)
                {
                    /* Configure CTS open parameters */
                    CTSOpenParams.BitConfigure[
                            UART_PIOBitFromBitMask(InitParams->CTS.BitMask)
                            ] = STPIO_BIT_INPUT;
                    CTSOpenParams.ReservedBits = InitParams->CTS.BitMask;
                    CTSOpenParams.IntHandler = NULL;

                    /* Open CTS PIO */
                    PIOError = STPIO_Open(InitParams->CTS.PortName,
                                          &CTSOpenParams,
                                          &Uart_p->CTSHandle);

                    /* Ensure CTS open succeeded */
                    if (PIOError == ST_NO_ERROR)
                    {
                        CTSInit = TRUE;
                    }
                    else
                    {
                        /* The PIO open call failed for some reason */
                        error = STUART_ERROR_PIO;
                    }
                }

                /* Set the thresholds for XON and XOFF to be sent */
                Uart_p->XOFFThreshold = (InitParams->FIFOLength >> 2);
                Uart_p->XONThreshold = (InitParams->FIFOLength >> 2);
                if (Uart_p->XOFFThreshold == 0)
                {
                    Uart_p->XOFFThreshold++;
                }
                if (Uart_p->XONThreshold == 0)
                {
                    Uart_p->XONThreshold++;
                }

                /* Allocate two timers for read and write operations */
                if (error == ST_NO_ERROR)
                {
                    if (stuart_TimerCreate(Uart_p->DeviceName,
						 		  DriverPartition,
  		                                            &Uart_p->ReadTimer,
                 		                              STUART_TIMER_TASK_PRIORITY) ||
                        stuart_TimerCreate(Uart_p->DeviceName,
                        					 DriverPartition,
  	                               	            	 &Uart_p->WriteTimer,
                                           		 STUART_TIMER_TASK_PRIORITY)
                       )
                    {
                        /* There was a problem creating one of the timers */
                        (void)stuart_TimerDelete(&Uart_p->ReadTimer, DriverPartition);
                        (void)stuart_TimerDelete(&Uart_p->WriteTimer, DriverPartition);
                        error = ST_ERROR_NO_MEMORY;
                    }
                    else
                    {
                        TimerInit = TRUE; /* End of timer initialization */
                    }
                }

                /* If a software FIFO is required, then allocate some memory
                 * for the buffer.
                 */
                if ((error == ST_NO_ERROR )&& (InitParams->SwFIFOEnable == TRUE))
                {

#ifdef STUART_DMA_SUPPORT
                    if( InitParams->UARTType == STUART_DMA)
		      {
                        Uart_p->OrigDMAReceiveBufferBase =
                            STOS_MemoryAllocate(InitParams->NCachePartition,
                                            InitParams->FIFOLength + UART_FDMA_ALIGN);
                        Uart_p->DMAReceiveBufferBase
                            = (U8 *)((((U32)Uart_p->OrigDMAReceiveBufferBase ) + UART_FDMA_ALIGN) & ~UART_FDMA_ALIGN);
                        /* Ensure memory allocation was successful */
                        if (Uart_p->DMAReceiveBufferBase == NULL)
                        {
                            /* Memory allocation failure */
                            error = ST_ERROR_NO_MEMORY;
                        }
                        Uart_p->RXDMANodeBytesRemaining = InitParams->FIFOLength;
                    }
#endif

                    Uart_p->ReceiveBufferBase =
                        STOS_MemoryAllocate(DriverPartition,
                                        InitParams->FIFOLength);

                    /* Ensure memory allocation was successful */
                    if (Uart_p->ReceiveBufferBase == NULL)
                    {
                        /* Memory allocation failure */
                        error = ST_ERROR_NO_MEMORY;
                    }
                    else
                    {
                        SwFIFOInit = TRUE; /* End of SW FIFO initialization */
                    }
                }


                if (error == ST_NO_ERROR)
                {

                    /* Set UART status to initialized */
                    Uart_p->UartStatus = UART_INIT;
                    Uart_p->IntEnable = 0;
                    Uart_p->IntEnableRxFlags = 0;
                    Uart_p->IntEnableTxFlags = 0;
                    Uart_p->WriteAwaitingPioCompare = FALSE;

                    /* Default Stuffing is not enabled */
                    Uart_p->StuffingEnabled = FALSE;

                    /* Set 'last protocol' to ensure all protocol parameters
                     * will be updated on first call.
                     */
                    memset(&Uart_p->LastProtocol,
                           -1,
                           sizeof(STUART_Protocol_t));

		      Uart_p->ReadSemaphore_p  = STOS_SemaphoreCreateFifo(NULL,1);
 		      Uart_p->WriteSemaphore_p = STOS_SemaphoreCreateFifo(NULL,1);

                    SemaphoreInit = TRUE;


                    /* UART Register setup - defaults taken from DefaultParams.
                     * Note that these can be overridden during Open().
                     */
                    ASCRegs_p = (UART_ASCRegs_t *)InitParams->BaseAddress;
                    UART_GlobalDisableInts(ASCRegs_p);

                    /* Check that the caller has specified default parameters */
                    if (InitParams->DefaultParams != NULL)
                    {
                        /* Check parameters passed by init */
                        error = UART_CheckParams(InitParams,
                                                 InitParams->DefaultParams);
                        if (error == ST_NO_ERROR)
                        {
                            /* Copy init parameters into the control block */
                            Uart_p->DefaultParams = *InitParams->DefaultParams;
                        }
                    }
                    else
                    {
                        /* Caller has not specified default parameters, so we
                         * select some sensible defaults.
                         */
                        Uart_p->DefaultParams.SmartCardModeEnabled  = FALSE;
                        Uart_p->DefaultParams.GuardTime             = 2;
                        Uart_p->DefaultParams.NACKSignalDisabled    = FALSE;
                        Uart_p->DefaultParams.RxMode.DataBits       = STUART_8BITS_EVEN_PARITY;
                        Uart_p->DefaultParams.HWFifoDisabled        = FALSE;
                        Uart_p->DefaultParams.RxMode.StopBits       = STUART_STOP_1_0;
                        Uart_p->DefaultParams.RxMode.FlowControl    = STUART_NO_FLOW_CONTROL;
                        Uart_p->DefaultParams.RxMode.BaudRate       = 38400;
                        Uart_p->DefaultParams.RxMode.TermString     = NULL;
                        Uart_p->DefaultParams.RxMode.NotifyFunction = NULL;
                        Uart_p->DefaultParams.TxMode.DataBits       = STUART_8BITS_EVEN_PARITY;
                        Uart_p->DefaultParams.TxMode.StopBits       = STUART_STOP_1_0;
                        Uart_p->DefaultParams.TxMode.FlowControl    = STUART_NO_FLOW_CONTROL;
                        Uart_p->DefaultParams.TxMode.BaudRate       = 38400;
                        Uart_p->DefaultParams.TxMode.TermString     = NULL;
                        Uart_p->DefaultParams.TxMode.NotifyFunction = NULL;
                    }

#ifdef STUART_DMA_SUPPORT
					if( InitParams->UARTType == STUART_DMA)
					{
						error = UART_DMAInit(Uart_p);
					}
					
#endif
                    if (error == ST_NO_ERROR)
                    {
                        /* Ensure we get regular timeout interrupts when
                         * operating with FIFOs enabled. This value leads
                         * to an interrupt after a gap of ~1.6 char lengths
                         */
                        UART_SetTimeout(ASCRegs_p, 16);

                        /* Select FIFO operation */
                        switch (InitParams->UARTType)
                        {
                            case STUART_NO_HW_FIFO:
                                /* Disable the hardware FIFO */
                                UART_FifoDisable(ASCRegs_p);
                                Uart_p->HwFIFOSize = 1;
                                break;

                            case STUART_RTSCTS:
                            case STUART_16_BYTE_FIFO:
#ifdef STUART_DMA_SUPPORT								
                            case STUART_DMA:
                                /* Enable the hardware FIFO */
                                UART_FifoEnable(ASCRegs_p);
                                Uart_p->HwFIFOSize = 16;
                                break;
#endif								

                            case STUART_ISO7816:
                                /* HWFifo only optional for ISO7816 device type */
                                if (Uart_p->DefaultParams.HWFifoDisabled == FALSE)
                                {
                                    /* Enable the hardware FIFO */
                                    UART_FifoEnable(ASCRegs_p);
                                    Uart_p->HwFIFOSize = 16;
                                }
                                else
                                {
                                    /* Disable the hardware FIFO */
                                    UART_FifoDisable(ASCRegs_p);
                                    Uart_p->HwFIFOSize = 1;
                                }
                                break;

                            default:
                                break;
                        }

                        /* Clear FIFOs */
                        UART_ResetRxFIFO(ASCRegs_p);
                        UART_ResetTxFIFO(ASCRegs_p);

                        /* Invoke the protocol parameters */
                        UART_SetProtocol(Uart_p,
                                         &Uart_p->DefaultParams.RxMode);

                        /* Enable run to start the UART */
                        UART_RunEnable(ASCRegs_p);

#ifdef STUART_DMA_SUPPORT
                       
                            UART_StartDMARX(Uart_p, Uart_p->DMAReceiveBufferBase, InitParams->FIFOLength);
                       
#endif

#if STUART_STATISTICS
                        /* Clear out UART statistics */
                        Uart_p->Statistics.NumberBytesReceived    = 0;
                        Uart_p->Statistics.NumberBytesTransmitted = 0;
                        Uart_p->Statistics.NumberOverrunErrors    = 0;
                        Uart_p->Statistics.NumberFramingErrors    = 0;
                        Uart_p->Statistics.NumberParityErrors     = 0;

#endif

                        /* Attempt to install the interrupt handler */
                        STOS_InterruptLock();

			  if (STOS_InterruptInstall(InitParams->InterruptNumber,
                                              InitParams->InterruptLevel,
						    STOS_INTERRUPT_CAST(UART_InterruptHandler),
						    "STUART",
						    Uart_p
                                             ) == 0 /* success */)
                       {
   				if (!STOS_InterruptEnable(InitParams->InterruptNumber, InitParams->InterruptLevel))
                            {
                                STOS_InterruptUnlock(); /* Re-enable interrupts */

                                /* Everything is going well, so we can now
                                 * finally append this item to the UART queue.
                                 */

                                UART_QueueAdd(Uart_p);

                                /* Now release the semaphore */
                                STOS_SemaphoreSignal(InitSemaphore_p);

                            }
                            else
                            {
                                STOS_InterruptUnlock(); /* Re-enable interrupts */

                                /* Error enabling interrupts */
                                error = ST_ERROR_BAD_PARAMETER;
                            }
                        }
                        else
                        {
                            STOS_InterruptUnlock(); /* Re-enable interrupts */

                            /* Error installing interrupts */
                            error = ST_ERROR_INTERRUPT_INSTALL;
                        }
                    }
                }
            }
            else
            {
                /* memory allocate failed */
                STOS_SemaphoreSignal(InitSemaphore_p);

            }
        }
        else
        {
            /* The UART is already in use */
            error = ST_ERROR_ALREADY_INITIALIZED;

        }
    }
    else
    {
        /* The device name length is too long */
        error = ST_ERROR_BAD_PARAMETER;
    }

    if (error != ST_NO_ERROR)
    {
        /* Free allocated resources */
        if (SwFIFOInit)                 /* Free SW FIFO */
        {
        
#ifdef STUART_DMA_SUPPORT
            STOS_MemoryDeallocate(InitParams->NCachePartition,
                              Uart_p->OrigDMAReceiveBufferBase);
#endif

            STOS_MemoryDeallocate(DriverPartition,
                              Uart_p->ReceiveBufferBase);
        }

        if (SemaphoreInit)
        {
            STOS_SemaphoreDelete(NULL, Uart_p->ReadSemaphore_p);
            STOS_SemaphoreDelete(NULL, Uart_p->WriteSemaphore_p);
        }

        if (TimerInit)                  /* Free timers */
        {

            (void)stuart_TimerDelete(&Uart_p->ReadTimer, DriverPartition);
            (void)stuart_TimerDelete(&Uart_p->WriteTimer, DriverPartition);
        }

        if (TXRXInit)                    /* Free PIO bits */
        {
            (void)STPIO_Close(Uart_p->TXDHandle);
            if (!Uart_p->IsHalfDuplex)
            {
                (void)STPIO_Close(Uart_p->RXDHandle);
            }
        }

        if (RTSInit)                     /* Free PIO bits */
        {
            (void)STPIO_Close(Uart_p->RTSHandle);
        }

        if (CTSInit)                     /* Free PIO bits */
        {
            (void)STPIO_Close(Uart_p->CTSHandle);
        }

        if (MemoryInit)                 /* Free driver memory */
        {
            STOS_MemoryDeallocate(DriverPartition,Uart_p);
        }

        /* Signal semaphore in case of error condition */
        STOS_SemaphoreSignal(InitSemaphore_p);
    }

    return error;

} /* STUART_Init() */

/*****************************************************************************
STUART_Open()
Description:
    Opens a named UART device and return a handle.
Parameters:
    DeviceName, text string describing the device to be opened.
    OpenParams, pointer to the UART open parameter data structure.
    Handle,     the handle of the opened UART device.
Return Value:
    ST_NO_ERROR,            no error has occurred.
    ST_ERROR_BAD_PARAMETER, one or more of the paramaters was invalid.
    ST_ERROR_UNKNOWN_DEVICE,  invalid device name.
See Also:
    STUART_Close()
    STUART_Init()
    STUART_SetParams()
*****************************************************************************/

ST_ErrorCode_t STUART_Open(const ST_DeviceName_t DeviceName,
                           STUART_OpenParams_t *OpenParams,
                           STUART_Handle_t *Handle
                          )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;
    volatile UART_ASCRegs_t *ASCRegs_p;

    /* Obtain the UART control block associated with the device name */
    Uart_p = UART_GetControlBlockFromName(DeviceName);

    if (Uart_p != NULL)
    {
        ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

        /* Ensure nobody already has this UART open */
        if (Uart_p->Handle == 0)
        {
            /* Caller may choose to override the initialization settings */
            if (OpenParams->DefaultParams != NULL)
            {
                /* check the parameters */
                error = UART_CheckParams(&Uart_p->InitParams,
                                         OpenParams->DefaultParams);
                if (error == ST_NO_ERROR)
                {
                    /* Copy default parameters into the UART control block */
                    Uart_p->DefaultParams = *OpenParams->DefaultParams;
                }
            }

            /* Proceed only if the RX and TX parameters were valid -- note that
             * New parameters are now copied to the local control block.
             */
            if (error != ST_ERROR_BAD_PARAMETER)
            {
                UART_GlobalDisableInts(ASCRegs_p);
                UART_ReceiverDisable(ASCRegs_p);

                /* Apply basic parameters to interrupt masks and SmartCard */
                UART_SetParams(Uart_p);

                /* Check loopback mode */
                if (OpenParams->LoopBackEnabled)
                {
                    /* Enable loopback */
                    UART_LoopBackEnable(ASCRegs_p);
                }
                else
                {
                    /* Disable loopback */
                    UART_LoopBackDisable(ASCRegs_p);
                }

                /* Check FlushIOBuffers */
                if (OpenParams->FlushIOBuffers)
                {
                    /* Flush software FIFO */
                    Uart_p->ReceiveBufferCharsFree = Uart_p->InitParams.FIFOLength;
                    Uart_p->ReceivePut_p = Uart_p->ReceiveGet_p =
                    Uart_p->ReceiveBufferBase;

                    /* Flush hardware FIFOs */
                    UART_ResetTxFIFO(ASCRegs_p);
                    UART_ResetRxFIFO(ASCRegs_p);
                }

                /* When opening a device, we always choose to set parameters
                 * as reflected in RxMode to ensure communications is correct
                 * for unsolicited data received.
                 */
                UART_SetProtocol(Uart_p,
                                 &Uart_p->DefaultParams.RxMode);

                /* Clear down the current read/write operation pointers */
                Uart_p->ReadOperation = NULL;
                Uart_p->WriteOperation = NULL;

                /* Set the handle to a unique non-zero value - using
                 * this particular choice makes it a handy shortcut
                 * for accessing the UART control block from the
                 * handle alone.
                 */

                Uart_p->Handle = (STUART_Handle_t)Uart_p;
                *Handle = Uart_p->Handle;

                /* Clear out the current receive error code */
                Uart_p->ReceiveError = ST_NO_ERROR;

                /* Now set the UART's status to the open flag */
                Uart_p->UartStatus = UART_OPEN;

                /* Enable receiver */
                UART_ReceiverEnable(ASCRegs_p);
               
                /* Enable receive interrupts/errors to begin with */
               {               
                    Uart_p->IntEnable =
                        Uart_p->IntEnableRxFlags | UART_IE_ENABLE_ERRORS;
                    UART_EnableInts(ASCRegs_p,
                                    Uart_p->IntEnable);
                }
            }
        }
        else
        {
            /* The handle is already open ,return the same handle */
            *Handle = Uart_p->Handle;
             Uart_p->OpenCount++;
            /* error = ST_ERROR_NO_FREE_HANDLES; - This error has been commented
             * to provide Multi instance support - DDTS 35847
             */

        }
    }
    else
    {
        /* The named UART does not exist */
        error = ST_ERROR_UNKNOWN_DEVICE;

    }
    /* Common exit point */
    return error;
} /* STUART_Open() */


/*****************************************************************************
STUART_Pause()
Description:
    Pauses all IO operations on the device.

    NOTE: A call to pause may only be followed by either an abort or
          a resume call.

Parameters:
    Handle, A valid handle.
    Direction, the IO direction to pause (tx/rx/both)
    State,  TRUE means pause
            FALSE means resume
Return Value:
    ST_NO_ERROR,                no error has occurred.
    ST_ERROR_INVALID_HANDLE,    the device handle is invalid.
See Also:
    STUART_Abort()
    STUART_Close()
    STUART_SetParams()
*****************************************************************************/

ST_ErrorCode_t STUART_Pause(STUART_Handle_t Handle,
                            STUART_Direction_t Direction
                           )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        volatile UART_ASCRegs_t *ASCRegs_p;
        ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

        /* Flag the UART as paused.  Note that we can not be interrupted
         * whilst doing this otherwise the pause may not take immediate
         * effect and characters may continue to be sent or received.
         */
        UART_GlobalDisableInts(ASCRegs_p);

        /* Pause any outstanding operations */
        if (Uart_p->ReadOperation != NULL &&
            (Uart_p->UartStatus & UART_PAUSE_RX) == 0 &&
            (Direction & STUART_DIRECTION_RECEIVE) != 0)
        {
            stuart_TimerPause(Uart_p->ReadOperation->Timer, TRUE);
            Uart_p->IntEnable &= ~Uart_p->IntEnableRxFlags;
            Uart_p->UartStatus |= UART_PAUSE_RX;
        }

        if (Uart_p->WriteOperation != NULL &&
            (Uart_p->UartStatus & UART_PAUSE_TX) == 0 &&
            (Direction & STUART_DIRECTION_TRANSMIT) != 0)
        {
            stuart_TimerPause(Uart_p->WriteOperation->Timer, TRUE);
            Uart_p->IntEnable &= ~Uart_p->IntEnableTxFlags;
            Uart_p->UartStatus |= UART_PAUSE_TX;
        }
        /* Re-enable interrupts */
        UART_EnableInts(ASCRegs_p,
                        Uart_p->IntEnable);
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_Pause() */

/*****************************************************************************
STUART_PosixRead()
Description:
    Reads a block of data from a UART device.
Parameters:
    Handle,         the device handle to read data from.
    Buffer,         a buffer into which to put the read data.
    MinToRead,      maximum number of bytes to read going to HW FIFO
    MaxToRead,      maximum number of bytes to retrieve from SW FIFO
    NumberRead,     on return this contains the number of bytes that have been
                    read into the buffer.

    STUART_Read maps to STUART_PosixRead with MinToRead = MaxToRead

Return Value:
    ST_NO_ERROR,                no errors.
    STUART_ERROR_ABORT,         the operation was aborted.
    STUART_ERROR_OVERRUN,       an internal read buffer has overrun.
    ST_ERROR_TIMEOUT,           timeout on read operation.
    STUART_ERROR_PARITY,        a parity error has occurred.
    STUART_ERROR_FRAMING,       a framing error has occurred.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.
    ST_ERROR_DEVICE_BUSY,       a read operation is already in progress.

    ST_ERROR_BAD_PARAMETER,     an invalid number of bytes has been
                                specified.
See Also:
    STUART_Abort()
    STUART_Open()
    STUART_Write()
*****************************************************************************/

ST_ErrorCode_t STUART_PosixRead(STUART_Handle_t Handle,
                                U8 *Buffer,
                                U32 MinToRead,
                                U32 MaxToRead,
                                U32 *NumberRead,
                                U32 TimeOut
                                )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    ST_ErrorCode_t           error = ST_NO_ERROR;
    UART_ControlBlock_t     *Uart_p;
    volatile UART_ASCRegs_t *ASCRegs_p;
    BOOL                     Terminated = FALSE;
    UART_Operation_t        *ReadOperation;


    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        /* Make sure that checks are atomic */
        STOS_SemaphoreWait(Uart_p->ReadSemaphore_p);

        /* Ensure that a read is not in progress */
        if (Uart_p->ReadOperation == NULL)
        {
            /*Reset byte count */
            *NumberRead = 0;
            ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

            /* We should first check to see if a call to STUART_SetParams()
             * has been made.  If it has, then the user wishes to change
             * the operational parameters for the UART.
             */
            if ((Uart_p->UartStatus & UART_SETPARAMS) != 0)
            {
                Uart_p->DefaultParams = Uart_p->SetParams;

                /* Disable receiver to avoid spurious recognition of glitches
                   from PIO mode change,re enable after setting params -DDTS 22220 */

                UART_ReceiverDisable(ASCRegs_p);
                UART_SetParams(Uart_p); /* Apply new parameters */
                UART_ReceiverEnable(ASCRegs_p);

                UART_SetProtocol(Uart_p, &Uart_p->DefaultParams.RxMode);

                /* Clear the set params flag to reflect that the new parameters
                 * have now been adopted.
                 */
                Uart_p->UartStatus &= ~UART_SETPARAMS;
            }

            /* Check for invalid number of bytes to write */
            if (UART_Is9BitMode(Uart_p->DefaultParams.RxMode.DataBits) &&
               (((MinToRead & 1) != 0) || ((MaxToRead & 1) != 0)))
            {
                /* In 9-bit data mode, the number of bytes to read must
                 * be an even number - therefore we must reject this
                 * request.
                 */
                STOS_SemaphoreSignal(Uart_p->ReadSemaphore_p);
                return ST_ERROR_BAD_PARAMETER;

            }


            /* Ensure that no errors have occured since the last read operation */
            if (Uart_p->ReceiveError != ST_NO_ERROR)
            {
                error = Uart_p->ReceiveError;
                Uart_p->ReceiveError = ST_NO_ERROR;
                STOS_SemaphoreSignal(Uart_p->ReadSemaphore_p);
                return error;
            }

            /* Clear out the termination string pointer, to start a new
             * series of checks on the term string.
             */
            Uart_p->TermString_p = NULL;

#ifdef STUART_DMA_SUPPORT
	     {
                 error = UART_DMARead(Uart_p,
                                    Buffer,
                                    MinToRead,
                                    MaxToRead,
                                    NumberRead,
                                    TimeOut);
                 STOS_SemaphoreSignal(Uart_p->ReadSemaphore_p);
                 return error;
            }
#endif

            if (MinToRead > 0)
            {
                /* Prepare a ReadOperation incase it is needed. The allocation
                 * strategy for a new read operation could be dynamic, however,
                 * for safety reasons we assign an operation from a
                 * pre-allocated pool.
                 */
                ReadOperation = &Uart_p->OperationPool[0];

                ReadOperation->UserBuffer = Buffer;
                ReadOperation->NumberRemaining = MinToRead;
                ReadOperation->NumberBytes = NumberRead;
                ReadOperation->Timeout = /* 1ms periods */
                   (ST_GetClocksPerSecond()/1000)*TimeOut;
                ReadOperation->NotifyRoutine =
                   Uart_p->DefaultParams.RxMode.NotifyFunction;
                ReadOperation->Timer = &Uart_p->ReadTimer;
                ReadOperation->Status = STUART_ERROR_PENDING;
                /* Grab any data that might be in the software FIFOs */
                if (Uart_p->InitParams.SwFIFOEnable)
                {
                    UART_GetDataFromSwFIFO(Uart_p, Buffer, MinToRead, MaxToRead,
                                           &Terminated, ReadOperation);
                }
                else
                {

                    /* Will have to go to interrupt handler now */
                    Uart_p->ReadOperation = ReadOperation;
                }
            }

            /* A new ReadOperation will have been set up by GetDataFromSwFIFO
               if more bytes were required */
            if (Uart_p->ReadOperation != NULL)
            {
                STOS_InterruptLock();

                /* Enable receive interrupts (must be atomic incase another
                   thread or the ISR is simultaneously setting other flags) */
                Uart_p->IntEnable |= Uart_p->IntEnableRxFlags;
                UART_EnableInts(ASCRegs_p,
                                Uart_p->IntEnable);

                STOS_InterruptUnlock();

                /* Start the timer */
                if (ReadOperation->NotifyRoutine != NULL)
                {
                    /* The caller does not wish to block, but to be notified
                     * via their supplied notification routine.
                     */
                    ReadOperation->ErrorSet_p = NULL;

                    stuart_TimerWait(ReadOperation->Timer,
                                     (void(*)(void *))UART_ReadCallback,
                                     Uart_p,
                                     &ReadOperation->Timeout,
                                     FALSE
                                    );

                }
                else
                {
                    /* The read operation should contain the status to be
                     * be returned to the caller.
                     */
                    ReadOperation->ErrorSet_p = &error;

                    /* The caller is happy to block */
                    stuart_TimerWait(ReadOperation->Timer,
                                     (void(*)(void *))UART_ReadCallback,
                                     Uart_p,
                                     &ReadOperation->Timeout,
                                     TRUE
                                     );
                }
            }
            else
            {
                /* (JF: shouldn't we re-enable read interrupts if the SW FIFO
                  was previously full?) */

                /* Since the request has been satisfied immediately, we may
                 * still have to process a callback.  This is bad news as
                 * we won't be going to the ISR for subsequent bytes.
                 *
                 * We artificially invoke the callback from the timer by
                 * signalling the timer immediately.
                 */

                if (Uart_p->DefaultParams.RxMode.NotifyFunction != NULL)
                {
                    /* The allocation strategy for a new read operation could
                     * be dynamic, however, for safety reasons we assign an
                     * operation from a pre-allocated pool.
                     */
                    ReadOperation = &Uart_p->OperationPool[0];

                    /* First of all setup a new operation for this read */
                    ReadOperation->UserBuffer      = NULL;
                    ReadOperation->NumberRemaining = 0;
                    ReadOperation->NumberBytes     = 0;
                    ReadOperation->Timeout         = 0; /* Forever */
                    ReadOperation->NotifyRoutine   =
                        Uart_p->DefaultParams.RxMode.NotifyFunction;
                    ReadOperation->Timer           = &Uart_p->ReadTimer;
                    ReadOperation->Status          = ST_NO_ERROR;

                    Uart_p->ReadOperation = ReadOperation;

                    /* The caller does not wish to block, but to be notified
                     * via their supplied notification routine.
                     */

                    stuart_TimerWait(ReadOperation->Timer,
                                     (void(*)(void *))UART_ReadCallback,
                                     Uart_p,
                                     &ReadOperation->Timeout,
                                     FALSE
                                     );

                    /* Should invoke the callback immediately */
                    stuart_TimerSignal(Uart_p->ReadOperation->Timer,FALSE);
                }

            }
        }
        else
        {
            /* Can not allow a read to start whilst an operation is already
             * in progress.
             */
            error = ST_ERROR_DEVICE_BUSY;


        }

        /* Signal the semaphore at the end to ensure that only one read Op
           proceed at one time */
        STOS_SemaphoreSignal(Uart_p->ReadSemaphore_p);
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_PosixRead() */

/*****************************************************************************
STUART_Resume()
Description:
    Resumes all IO operations on the device.

Parameters:
    Handle, A valid handle.
    Direction, the IO direction to resume (tx/rx/both)
Return Value:
    ST_NO_ERROR,                no error has occurred.
    ST_ERROR_INVALID_HANDLE,    the device handle is invalid.
See Also:
    STUART_Abort()
    STUART_Close()
    STUART_SetParams()
*****************************************************************************/

ST_ErrorCode_t STUART_Resume(STUART_Handle_t Handle,
                             STUART_Direction_t Direction
                            )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {
        volatile UART_ASCRegs_t *ASCRegs_p;
        ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

        /* Operation must not be interrupted */
        UART_GlobalDisableInts(ASCRegs_p);

        /* Resume any outstanding operations */
        if (Uart_p->ReadOperation != NULL &&
            (Uart_p->UartStatus & UART_PAUSE_RX) != 0 &&
            (Direction & STUART_DIRECTION_RECEIVE) != 0)
        {
            stuart_TimerPause(Uart_p->ReadOperation->Timer, FALSE);
            Uart_p->IntEnable |= Uart_p->IntEnableRxFlags;
            Uart_p->UartStatus &= ~UART_PAUSE_RX;
        }


        if (Uart_p->WriteOperation != NULL &&
            (Uart_p->UartStatus & UART_PAUSE_TX) != 0 &&
            (Direction & STUART_DIRECTION_TRANSMIT) != 0)
        {
            stuart_TimerPause(Uart_p->WriteOperation->Timer, FALSE);
            if (Uart_p->WriteOperation->LastTransferSize ==
                Uart_p->WriteOperation->NumberRemaining)
            {
                /* need to wait for TXEMPTY before marking Write complete */
                Uart_p->IntEnable |= UART_IE_ENABLE_TX_END;
            }
            else
            {
                /* accept any standard interrupt to continue with Write */
                Uart_p->IntEnable |= Uart_p->IntEnableTxFlags;
            }
            Uart_p->UartStatus &= ~UART_PAUSE_TX;
        }

        /* Now renable interrupts as before */
        UART_EnableInts(ASCRegs_p,
                        Uart_p->IntEnable);

    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;

} /* STUART_Resume() */

/*****************************************************************************
STUART_SetParams()
Description:
    Sets the parameters for use by the UART device.

    NOTE: The parameters are copied into the UART control block and do not
    take effect until the next read or write is called.

Parameters:
    Handle, the device handle.
    Params, pointer to a structure containing the communications parameters
            to be used by the UART device.
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_INVALID_HANDLE,    device handle is invalid.
    ST_ERROR_BAD_PARAMETER,     one or more of the parameters was invalid.
    ST_ERROR_FEATURE_NOT_SUPPORTED, the requested feature is not yet
                                    supported.
See Also:
    STUART_GetParams()
    STUART_Read()
    STUART_Write()
*****************************************************************************/

ST_ErrorCode_t STUART_SetParams(STUART_Handle_t Handle,
                                STUART_Params_t *Params
                               )
{
    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);

    /* Ensure the handle as provided a valid UART control block */
    if (Uart_p != NULL)
    {
        /* Check the parameters */
        error = UART_CheckParams(&Uart_p->InitParams, Params);
        if (error == ST_NO_ERROR)
        {
            /* All the parameters check out - flag the new
             * parameters in the UART control block and
             * copy them across.  They will take effect
             * on the next read/write call.
             */
            Uart_p->SetParams = *Params;
            Uart_p->UartStatus |= UART_SETPARAMS;
        }
    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;
    }

    /* Common exit point */
    return error;
} /* STUART_SetParams() */

/*****************************************************************************
STUART_Term()
Description:
    Terminates the UART device.
Parameters:
    DeviceName, text string indicating the device name to terminate.
    TermParams, parameters to control the termination of the device.
Return Value:
    ST_NO_ERROR,                    no errors.
    ST_ERROR_UNKNOWN_DEVICE,        invalid device name.
    ST_ERROR_OPEN_HANDLE,           could not terminate the device.
    ST_ERROR_INVALID_HANDLE,        the PIO handle is invalid.
    ST_ERROR_INTERRUPT_UNINSTALL,   unable to uninstall the interrupt.
See Also:
    STUART_Init()
*****************************************************************************/

ST_ErrorCode_t STUART_Term(const ST_DeviceName_t DeviceName,
                           const STUART_TermParams_t *TermParams
                          )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    volatile UART_ASCRegs_t *ASCRegs_p;
    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;
    partition_t *DriverPartition = NULL;

    if (InitSemaphore_p != NULL)
    {
        STOS_SemaphoreWait(InitSemaphore_p);
    }
    else
    {
        return ST_ERROR_BAD_PARAMETER;
    }

    /* Obtain the UART control block associated with the device name */
    Uart_p = UART_GetControlBlockFromName(DeviceName);

    /* Only continue if we have a control block for this device */
    if (Uart_p != NULL)
    {
        /* This device may be terminated if either:
         * a) the caller wishes to force termination
         * b) the device is closed
         */

        if (Uart_p->Handle == 0 || TermParams->ForceTerminate)
        {

#ifndef ST_OS21
            interrupt_status_t IntStatus;
#endif
            /* Abort any outstanding operations on this device */
            if (Uart_p->Handle != 0)
            {
                (void)STUART_Close(Uart_p->Handle);
            }
#ifdef STUART_DMA_SUPPORT
           
                UART_DMATerm(Uart_p);
            
#endif

	     /* Store driver partition pointer, before deallocating
             * memory.
             */
	     DriverPartition = (partition_t *)Uart_p->InitParams.DriverPartition;

            /* Free up timer resources */
            (void)stuart_TimerDelete(&Uart_p->ReadTimer, DriverPartition);
            (void)stuart_TimerDelete(&Uart_p->WriteTimer, DriverPartition);

            /* Disable run on the UART */
            ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
            UART_RunDisable(ASCRegs_p);

            /* Close any open PIO ports */
            if (Uart_p->InitParams.TXD.PortName[0] != 0 &&
                Uart_p->InitParams.RXD.PortName[0] != 0)
            {
                (void)STPIO_Close(Uart_p->TXDHandle);
                if (!Uart_p->IsHalfDuplex)
                {
                    /* full duplex mode (cf STUART_Init) */
                    (void)STPIO_Close(Uart_p->RXDHandle);
                }
            }

            if (UART_IsRTSCTSSupported(Uart_p->InitParams.UARTType))
            {
                if (Uart_p->InitParams.RTS.PortName[0] != 0)
                {
                    (void)STPIO_Close(Uart_p->RTSHandle);
                }
                if (Uart_p->InitParams.CTS.PortName[0] != 0)
                {
                    (void)STPIO_Close(Uart_p->CTSHandle);
                }
            }

            STOS_InterruptLock();       /* Operation must be atomic */

            /* Remove device from queue */
            UART_QueueRemove(Uart_p);


            /* Now uninstall the interrupt handler */
            if (STOS_InterruptUninstall(Uart_p->InitParams.InterruptNumber,
                                    Uart_p->InitParams.InterruptLevel, Uart_p) != 0)
            {
                /* Unable to remove the handler */
                error = ST_ERROR_INTERRUPT_UNINSTALL;
            }

#ifndef ST_OS21
            /* Obtain number of active handlers still installed */
            if (interrupt_status(Uart_p->InitParams.InterruptLevel,
                                 &IntStatus) == 0)
            {
                /* Disable interrupt level if no handler are installed */
                if (IntStatus.interrupt_numbers == 0)
                {
                    if (STOS_InterruptDisable(Uart_p->InitParams.InterruptNumber, Uart_p->InitParams.InterruptLevel))
                    {
                        /* Unable to obtain interrupt level status */
                        error = ST_ERROR_INTERRUPT_UNINSTALL;
                    }

                }
            }
            else
            {
                /* Unable to obtain interrupt level status */
                error = ST_ERROR_INTERRUPT_UNINSTALL;
            }
#else  /* defined ST_OS21 */

            if (STOS_InterruptDisable(Uart_p->InitParams.InterruptNumber, Uart_p->InitParams.InterruptLevel))
            {
                /* Unable to disable interrupts */
                error = ST_ERROR_INTERRUPT_UNINSTALL;
            }
#endif

            STOS_InterruptUnlock();     /* Re-enable interrupts */

            /* Store driver partition pointer, before deallocating
             * memory.
             */
            DriverPartition =
                (partition_t *)Uart_p->InitParams.DriverPartition;

            /* Free up the software FIFO memory, if allocated */
            if (Uart_p->InitParams.SwFIFOEnable)
            {
                STOS_MemoryDeallocate(DriverPartition,
                                  Uart_p->ReceiveBufferBase);

#ifdef STUART_DMA_SUPPORT				
               {
                     STOS_MemoryDeallocate(Uart_p->InitParams.NCachePartition, Uart_p->OrigDMAReceiveBufferBase);
                }
#endif
            }

            /* Delete read & write semaphores */
            STOS_SemaphoreDelete(NULL, Uart_p->ReadSemaphore_p);
            STOS_SemaphoreDelete(NULL, Uart_p->WriteSemaphore_p);


            /* Free up the control structure for the UART */
            STOS_MemoryDeallocate(DriverPartition,Uart_p);

        }
        else
        {
            /* There is still an outstanding open on this device, but the
             * caller does not wish to force termination.
             */
            error = ST_ERROR_OPEN_HANDLE;

        }

    }
    else
    {
        /* The control block was not retrieved from the queue, therefore the
         * device name must be invalid.
         */
        error = ST_ERROR_UNKNOWN_DEVICE;

    }

    /* If there's an instance, the semaphore must be valid */
    STOS_SemaphoreSignal(InitSemaphore_p);

    /* Common exit point */
    return error;
} /* STUART_Term() */

/*****************************************************************************
STUART_Write()
Description:
    Writes a block of data to a UART device.
Parameters:
    Handle,         handle of the device over which data will be written.
    Buffer,         a buffer holding the data to be sent.
    NumberToWrite,  number of bytes to write to the device.
    NumberWritten,  updated with the number of bytes written.
Return Value:
    ST_NO_ERROR,                no error has occurred.
    ST_ERROR_INVALID_HANDLE,    device handle invalid.
    ST_ERROR_TIMEOUT,           timeout on write operation.
    STUART_ERROR_ABORT,         the operation was aborted.
    ST_ERROR_DEVICE_BUSY,       a write operation is already in progress.
    ST_ERROR_BAD_PARAMETER,     an invalid number of bytes has been
                                specified.
See Also:
    STUART_Open()
    STUART_SetParams()
    STUART_Read()
*****************************************************************************/

ST_ErrorCode_t STUART_Write(STUART_Handle_t Handle,
                            U8 *Buffer,
                            U32 NumberToWrite,
                            U32 *NumberWritten,
                            U32 TimeOut
                           )
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    ST_ErrorCode_t error = ST_NO_ERROR;
    UART_ControlBlock_t *Uart_p;
    UART_Operation_t *WriteOperation;
    volatile UART_ASCRegs_t *ASCRegs_p;

    /* Obtain the UART control block from the passed in handle */
    Uart_p = UART_GetControlBlockFromHandle(Handle);
    /* Ensure the control block is valid */
    if (Uart_p != NULL)
    {

        /* Make sure that checks are atomic */
        STOS_SemaphoreWait(Uart_p->WriteSemaphore_p);

        /* Ensure that no other operation is in progress */
        if (Uart_p->WriteOperation == NULL)
        {

            /* Reset transmit count */
            *NumberWritten = 0;

            /* Disable interrupts whilst we start our transmit */
            ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;
            UART_GlobalDisableInts(ASCRegs_p);

            /* We should first check to see if a call to SetParams
             * has been made.  If it has, then the user wishes to change
             * the operational parameters for the UART.
             */
            if ((Uart_p->UartStatus & UART_SETPARAMS) != 0)
            {
                Uart_p->DefaultParams = Uart_p->SetParams;

                /* Disable receiver to avoid spurious recognition of glitches
                   from PIO mode change & re enable after setting params -DDTS 22220 */
                UART_ReceiverDisable(ASCRegs_p);
                UART_SetParams(Uart_p);
                UART_ReceiverEnable(ASCRegs_p);

                /* UART_SetProtocol call is unconditional below */

                /* Clear the set params flag to reflect that the new parameters
                 * have now been adopted.
                 */
                Uart_p->UartStatus &= ~UART_SETPARAMS;
            }
            /* Check for invalid number of bytes to write */
            if (UART_Is9BitMode(Uart_p->DefaultParams.TxMode.DataBits) &&
               (NumberToWrite & 1) != 0)
            {
                /* In 9-bit data mode, the number of bytes to send must
                 * be an even number - therefore we must reject this
                 * request.
                 */
                STOS_SemaphoreSignal(Uart_p->WriteSemaphore_p);
                return ST_ERROR_BAD_PARAMETER;
            }


            /* Setup communications protocol (TxMode) */
            UART_SetProtocol(Uart_p, &Uart_p->DefaultParams.TxMode);

            /* Reset retries count */
            Uart_p->Retries = Uart_p->DefaultParams.Retries;
            if (UART_IsNACKSupported(Uart_p->InitParams.UARTType))
            {
                UART_SetRetries(ASCRegs_p, Uart_p->Retries);
            }

#ifdef STUART_DMA_SUPPORT
	     
	     {
                error = UART_DMAWrite(Uart_p, Buffer, NumberToWrite, NumberWritten, TimeOut);
                STOS_SemaphoreSignal(Uart_p->WriteSemaphore_p);
                return error;
            }
#endif

            if (NumberToWrite > 0)
            {
                U32 TxSent = 0;

                /* Check the transmit empty status - depending on when the last
                 * transmit was, we may need to recommence the 'chain'.
                 * Otherwise there is data still waiting to be sent. We
                 * can slip it behind the existing data when we get our
                 * next interrupt.
                 *
                 * Note that if we've been stopped by XOFF, we shouldn't even
                 * send the one character.
                 */

                STOS_InterruptLock();

                if (UART_IsTxEmpty(ASCRegs_p) &&
                    ((Uart_p->UartStatus & UART_XOFF_TX) == 0))
                {
                    /* The transmitter is empty, therefore we should write
                     * a character to the transmitter to get the ball rolling
                     * [JF: This is always the case with the new STUART_Write]
                     */
                    if (UART_Is9BitMode(Uart_p->DefaultParams.TxMode.DataBits))
                    {
                        TxSent = UART_Write9BitBlock(Uart_p, Buffer, 2);
                    }
                    else
                    {
                        TxSent = UART_WriteBlock(Uart_p, Buffer, 1);
                    }
                }

                /* The allocation strategy for a new write operation could
                 * be dynamic, however, for safety reasons we assign an
                 * operation from a pre-allocated pool.
                 */
                WriteOperation = &Uart_p->OperationPool[1];

                /* First of all setup a new operation for this write */
                WriteOperation->UserBuffer = Buffer;
                WriteOperation->NumberRemaining = NumberToWrite;
                WriteOperation->NumberBytes = NumberWritten;
                WriteOperation->LastTransferSize = TxSent;
                WriteOperation->Timeout = /* 1ms periods */
                    (ST_GetClocksPerSecond()/1000)*TimeOut;
                WriteOperation->NotifyRoutine =
                    Uart_p->DefaultParams.TxMode.NotifyFunction;
                WriteOperation->Timer = &Uart_p->WriteTimer;
                WriteOperation->Status = STUART_ERROR_PENDING;

                /* Give the write to the ISR; subsequent bytes will be
                 * written via 'WriteOperation'.
                 */

                Uart_p->WriteOperation = WriteOperation;

                STOS_InterruptUnlock();
                /* If we're not paused by XOFF, reenable the transmit
                 * interrupt
                 */
                if ((Uart_p->UartStatus & UART_XOFF_TX) == 0)
                {
                    if (TxSent < NumberToWrite)
                    {
                        /* normal transmit case with further bytes to go */
                        Uart_p->IntEnable |= Uart_p->IntEnableTxFlags;
                    }
                    else
                    {
                        /* where it's the final byte, we only want to know
                          when its fully completed */
                        Uart_p->IntEnable |=
                            (Uart_p->IntEnableTxFlags & ~IE_TXHALFEMPTY);
                    }
                }

                UART_EnableInts(ASCRegs_p,Uart_p->IntEnable);

                /* Start the timer */
                if (WriteOperation->NotifyRoutine != NULL)
                {

                    WriteOperation->ErrorSet_p = NULL;

                    /* The caller does not wish to block, but to be notified
                     * via their supplied notification routine.
                     */

                    stuart_TimerWait(WriteOperation->Timer,
                                     (void(*)(void *))UART_WriteCallback,
                                     Uart_p,
                                     &WriteOperation->Timeout,
                                     FALSE
                                     );
                }
                else
                {

                    WriteOperation->ErrorSet_p = &error;

                    /* The caller is happy to block */
                    stuart_TimerWait(WriteOperation->Timer,
                                     (void(*)(void *))UART_WriteCallback,
                                     Uart_p,
                                     &WriteOperation->Timeout,
                                     TRUE
                                     );

                }
            }
            else
            {
                /* There are no more bytes to write i.e., the request has been
                 * satisfied.  Before leaving, re-enable interrupts. */
                UART_EnableInts(ASCRegs_p,Uart_p->IntEnable);
            }
        }
        else
        {
            /* Can not allow a write to start whilst an operation is already
             * in progress.
             */
            error = ST_ERROR_DEVICE_BUSY;
        }

        /* Signal the semaphore at the end to make sure that only one Write
           proceed at one time */
        STOS_SemaphoreSignal(Uart_p->WriteSemaphore_p);

    }
    else
    {
        /* The handle is invalid */
        error = ST_ERROR_INVALID_HANDLE;

    }
    /* Common exit point */
    return error;
} /* STUART_Write() */

/* Private Functions ------------------------------------------------------ */

/* UART Queue Management */

/*****************************************************************************
UART_QueueAdd()
Description:
    This routine appends an allocated uart control block to the
    uart queue.

    NOTE: The interrupt lock must be held when calling this routine.

Parameters:
    QueueItem_p, the control block to append to the queue.
Return Value:
    Nothing.
See Also:
    UART_QueueRemove()
*****************************************************************************/

static void UART_QueueAdd(UART_ControlBlock_t *QueueItem_p)
{
    UART_ControlBlock_t *qp;

    /* Check the base element is defined, otherwise, append to the end of the
     * linked list.
     */
    if (UartQueueHead_p == NULL)
    {
        /* As this is the first item, no need to append */
        UartQueueHead_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
    else
    {
        /* Iterate along the list until we reach the final item */
        qp = UartQueueHead_p;
        while (qp != NULL && qp->Next_p)
        {
            qp = qp->Next_p;
        }

        /* Append the new item */
        qp->Next_p = QueueItem_p;
        QueueItem_p->Next_p = NULL;
    }
} /* UART_QueueAdd() */

/*****************************************************************************
UART_QueueRemove()
Description:
    This routine removes an allocated uart control block from the
    uart queue.

    NOTE: The interrupt lock must be held when calling this routine.
    Use UART_IsInit() or UART_GetControlBlockFromName() to verify whether
    or not a particular item is in the queue prior to calling this routine.

Parameters:
    QueueItem_p, the control block to remove from the queue.
Return Value:
    Nothing.
See Also:
    UART_QueueAdd()
*****************************************************************************/

static void UART_QueueRemove(UART_ControlBlock_t *QueueItem_p)
{
    UART_ControlBlock_t *this_p, *last_p;

    /* Check the base element is defined, otherwise quit as no items are
     * present on the queue.
     */
    if (UartQueueHead_p != NULL)
    {
        /* Reset pointers to loop start conditions */
        last_p = NULL;
        this_p = UartQueueHead_p;

        /* Iterate over each queue element until we find the required
         * queue item to remove.
         */
        while (this_p != NULL && this_p != QueueItem_p)
        {
            last_p = this_p;
            this_p = this_p->Next_p;
        }

        /* Check we found the queue item */
        if (this_p == QueueItem_p)
        {
            /* Unlink the item from the queue */
            if (last_p != NULL)
            {
                last_p->Next_p = this_p->Next_p;
            }
            else
            {
                /* Recalculate the new head of the queue i.e.,
                 * we have just removed the queue head.
                 */
                UartQueueHead_p = this_p->Next_p;
            }
        }
    }
} /* UART_QueueRemove() */

/*****************************************************************************
UART_IsInit()
Description:
    Runs through the queue of initialized UART devices and checks that
    the UART with the specified device name has not already been added.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    TRUE, the device has already been initialized.
    FALSE, the device is not in the queue and therefore is not initialized.
See Also:
    STUART_Init()
*****************************************************************************/

static BOOL UART_IsInit(const ST_DeviceName_t DeviceName)
{
    BOOL Initialized = FALSE;
    UART_ControlBlock_t *qp = UartQueueHead_p; /* Global queue head pointer */

    while (qp)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->DeviceName, DeviceName) != 0)
        {
            /* Next UART in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The UART is already initialized */
            Initialized = TRUE;
            break;
        }
    }

    /* Boolean return value reflecting whether the device is initialized */
    return Initialized;
} /* UART_IsInit() */

/*****************************************************************************
UART_GetControlBlockFromName()
Description:
    Runs through the queue of initialized UART devices and checks for
    the UART with the specified device name.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    DeviceName, text string indicating the device name to check.
Return Value:
    NULL, end of queue encountered - invalid device name.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    UART_IsInit()
*****************************************************************************/

static UART_ControlBlock_t *
   UART_GetControlBlockFromName(const ST_DeviceName_t DeviceName)
{
    UART_ControlBlock_t *qp = UartQueueHead_p; /* Global queue head pointer */

    while (qp != NULL)
    {
        /* Check the device name for a match with the item in the queue */
        if (strcmp(qp->DeviceName, DeviceName) != 0)
        {
            /* Next UART in the queue */
            qp = qp->Next_p;
        }
        else
        {
            /* The UART entry has been found */
            break;
        }
    }

    /* Return the UART control block (or NULL) to the caller */
    return qp;
} /* UART_GetControlBlockFromName() */

/*****************************************************************************
UART_GetControlBlockFromHandle()
Description:
    Runs through the queue of initialized UART devices and checks for
    the UART with the specified handle.

    NOTE: Must be called with the queue semaphore held.

Parameters:
    Handle, an open handle.
Return Value:
    NULL, end of queue encountered - invalid handle.
    Otherwise, returns a pointer to the control block for the device.
See Also:
    UART_GetControlBlockFromName()
*****************************************************************************/

static UART_ControlBlock_t *
   UART_GetControlBlockFromHandle(STUART_Handle_t Handle)
{
    UART_ControlBlock_t *qp = UartQueueHead_p; /* Global queue head pointer */

    /* Iterate over the uart queue */
    while (qp != NULL)
    {
        /* Check for a matching handle */
        if (Handle == qp->Handle && Handle != 0)
        {
            break;  /* This is a valid handle */
        }

        /* Try the next UART */
        qp = qp->Next_p;
    }

    /* Return the UART control block (or NULL) to the caller */
    return qp;
} /* UART_GetControlBlockFromHandle() */

/*****************************************************************************
UART_ReadCallback()
Description:
    This callback routine is called whenever a read operation's timer
    expires e.g., timeout or signalled.

    It is the responsibility of this function to establish the reason for
    the callback ocurring, tidy up the operation and ensure the caller
    gets the appropriate error code.

    NOTE: This routine nullifies the UART control block's read operation
    pointer to hide it from the ISR.

Parameters:
    param,  pointer to callback's parameters - this should be a pointer
            to a uart control block object.

Return Value:
    Nothing.
See Also:
    UART_Read()
*****************************************************************************/

static void UART_ReadCallback(UART_ControlBlock_t *Uart_p)
{


    UART_Operation_t *ReadOp_p = Uart_p->ReadOperation;

    /* Establish the cause of the callback i.e., we either timed-out or
     * were signalled by an abort, for example.
     * In the case we were signalled, the signaller should have left
     * the status code behind for us.
     */

    if (ReadOp_p->Timer->TimeoutError == TIMER_TIMEDOUT)
    {
        /* The timer timedout, so fill in the read operation's
         * error code.
         */
        ReadOp_p->Status = ST_ERROR_TIMEOUT;
    }

    if (ReadOp_p->ErrorSet_p != NULL)
    {
        *ReadOp_p->ErrorSet_p = ReadOp_p->Status;
    }

    Uart_p->ReadOperation = NULL;       /* ISR no longer knows about us */

    /* Call the notification routine, if there is one */
    if (ReadOp_p->NotifyRoutine != NULL)
    {
        ReadOp_p->NotifyRoutine(Uart_p->Handle,ReadOp_p->Status);
    }


} /* UART_ReadCallback() */

/*****************************************************************************
UART_WriteCallback()
Description:
    This callback routine is called whenever a write operation's timer
    expires e.g., timeout or signalled.

    It is the responsibility of this function to establish the reason for
    the callback ocurring, tidy up the operation and ensure the caller
    gets the appropriate error code.

    NOTE: This routine nullifies the UART control block's write operation
    pointer to hide it from the ISR.

Parameters:
    param,  pointer to callback's parameters - this should be a pointer
            to a uart control block object.

Return Value:
    Nothing.
See Also:
    UART_Write()
*****************************************************************************/

static void UART_WriteCallback(UART_ControlBlock_t *Uart_p)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif


    UART_Operation_t *WriteOp_p = Uart_p->WriteOperation;
    volatile UART_ASCRegs_t *ASCRegs_p = (UART_ASCRegs_t *)Uart_p->InitParams.BaseAddress;

    /* We should remove this operation out of the ISR's way now, as
     * it is effectively complete one way or the other.
     */
    /* JF: it can wait until after we have finished with WriteOp, because the
      InterruptHandler only acts if Uart_p->WriteOperation->Status ==
      STUART_ERROR_PENDING. We need it at the end because another thread can
      enter STUART_Write once Uart_p->WriteOperation = NULL */

    /* Establish the cause of the callback i.e., we either timed-out or
     * were signalled by an abort, for example.
     * In the case we were signalled, the signaller should have left
     * the status code behind for us.
     */
    if (WriteOp_p->Timer->TimeoutError == TIMER_TIMEDOUT)
    {
        /* The timer timedout, so fill in the write operation's
         * error code.
         */
        WriteOp_p->Status = ST_ERROR_TIMEOUT;

        /* Clean out the transmit fifo (still allows any byte currently in the
          transmit shift register to complete). This prevents us waiting
          forever for HW flow control when the cable is unplugged. We know
          no data from a previous operation will be clobbered because they
          don't complete until all data has left the FIFO */

        UART_ResetTxFIFO(ASCRegs_p);
    }

    if (WriteOp_p->ErrorSet_p !=NULL)
    {
        *WriteOp_p->ErrorSet_p = WriteOp_p->Status;
    }

    Uart_p->WriteOperation = NULL; /* ISR no longer knows about us */

    /* Call the notification routine, if there is one. */
    if (WriteOp_p->NotifyRoutine != NULL)
    {
        WriteOp_p->NotifyRoutine(Uart_p->Handle,WriteOp_p->Status);
    }

} /* UART_WriteCallback() */


/******************************************************************************
Function Name : UART_CheckParams
  Description : Check supplied parameters are valid for the UART device
   Parameters :
******************************************************************************/
static ST_ErrorCode_t UART_CheckParams(const STUART_InitParams_t* InitParams,
                                       STUART_Params_t* Params)
{
    ST_ErrorCode_t error;

    error = UART_CheckProtocol(InitParams->UARTType, &Params->RxMode);

    if (error == ST_NO_ERROR)
    {
        error = UART_CheckProtocol(InitParams->UARTType, &Params->TxMode);
    }

    /* Check smart card setup */
    if ((error == ST_NO_ERROR) && (Params->SmartCardModeEnabled))
    {
        /* Smartcard mode requires a PIO pin in order to set bidirectional
           mode, which prevents us holding the line high when the card is
           trying to transmit */

        if (InitParams->TXD.PortName[0] == 0)
        {
            error = ST_ERROR_BAD_PARAMETER;
        }
        else
        {
            error = UART_CheckSmartCardConfig(InitParams->UARTType, Params);
        }
    }

    return (error);
}

/*****************************************************************************
UART_CheckProtocol()
Description:
    Check the protocol intended for use by the UART device.

Parameters:
    DeviceType, UART device type that the device was initialized as.
    Protocol_p, pointer to a structure containing the communications
                parameters to be used by the UART device.
Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_BAD_PARAMETER,     one or more of the parameters was invalid.
See Also:
    UART_SetProtocol()
*****************************************************************************/

static ST_ErrorCode_t UART_CheckProtocol(STUART_Device_t DeviceType,
                                         STUART_Protocol_t *Protocol_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Check Mode */
    if (Protocol_p->DataBits >= STUART_7BITS_ODD_PARITY &&
        Protocol_p->DataBits <= STUART_9BITS_NO_PARITY)
    {
        /* Check stop bits */
        if (Protocol_p->StopBits >= STUART_STOP_0_5 &&
            Protocol_p->StopBits <= STUART_STOP_2_0)
        {
            /* Check flow control */
            switch (Protocol_p->FlowControl)
            {
                case STUART_NO_FLOW_CONTROL: /* Always supported */
                    break;

                case STUART_RTSCTS_FLOW_CONTROL:
                    /* Only supported if device type allows it */
                    if (!UART_IsRTSCTSSupported(DeviceType))
                        error = ST_ERROR_BAD_PARAMETER;/* was FEATURE_NOT_SUPPORTED */
                    break;

                case STUART_SW_FLOW_CONTROL:
#ifdef STUART_DMA_SUPPORT					
                    if (DeviceType == STUART_DMA)
                    {
                        error = ST_ERROR_BAD_PARAMETER;
                    }
#endif					
                    break;

                default:                /* Unknown setting */
                    error = ST_ERROR_BAD_PARAMETER;
                    break;
            }

            if (error == ST_NO_ERROR)
            {
                /* Check baudrate */
                if (UART_BaudRate(NULL,
                                  Protocol_p->BaudRate,
                                  FALSE
                                 ) )
                {
                    /* Baud rate not supported */
                    error = ST_ERROR_BAD_PARAMETER;
                }
            }
        }
        else
        {
            /* Stop bits not supported */
            error = ST_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        /* Mode not supported */
        error = ST_ERROR_BAD_PARAMETER;
    }

    /* Common exit point */
    return error;
} /* UART_CheckProtocol() */


/*****************************************************************************
UART_CheckSmartCardConfig()
Description:
   Checks the smartcard parameters against the device type specified.

Parameters:
    DeviceType, UART device type that the device was initialized as.
    Params_p , pointer to a structure containing the smart card config
               parameters

Return Value:
    ST_NO_ERROR,                no errors.
    ST_ERROR_BAD_PARAMETER,     the value of the parameter is illegal
                                for the device type being used.
*****************************************************************************/
static ST_ErrorCode_t UART_CheckSmartCardConfig(STUART_Device_t DeviceType,
                                                STUART_Params_t *Params_p)
{
    ST_ErrorCode_t error = ST_NO_ERROR;

    /* Check smartcard configuration */
    switch (DeviceType)
    {
        case STUART_ISO7816:
            /* No restrictions for smart card params */
            break;

        case STUART_NO_HW_FIFO:
            /* no break */

        case STUART_RTSCTS:
            /* no break */

        case STUART_16_BYTE_FIFO:
            /* no break */

        default:
            if (Params_p->GuardTime > 255)
            {
                error = ST_ERROR_BAD_PARAMETER;
            }
            break;
    }

    /* Common exit point */
    return error;
} /* UART_CheckSmartCardConfig */


/*****************************************************************************
UART_PIOBitFromBitMask()
Description:
    This routine is used for converting a PIO bitmask to a bit value in
    the range 0-7. E.g., 0x01 => 0, 0x02 => 1, 0x04 => 2 ... 0x80 => 7

Parameters:
    BitMask, a bitmask - should only have a single bit set.
Return Value:
    The bit value of the bit set in the bitmask.
See Also:
    STUART_Init()
*****************************************************************************/
U8 UART_PIOBitFromBitMask(U8 BitMask)
{
    U8 Count = 0;                       /* Initially assume bit zero */

    /* Shift bit along until the bitmask is set to one */
    while (BitMask != 1 && Count < 7)
    {
        BitMask >>= 1;
        Count++;
    }

    return Count;                       /* Should contain the bit [0-7] */
}

/******************************************************************************
Function Name : UART_PIOIntHandler
  Description : Handler for PIO compare interrupt
   Parameters :
******************************************************************************/
void UART_PioIntHandler(STPIO_Handle_t Handle, STPIO_BitMask_t ActiveBits)
{

#if !defined(ARCHITECTURE_ST40) && !defined(PROCESSOR_C1)
#pragma ST_device(ASCRegs_p)
#endif

    STPIO_Compare_t         cmp = {0,0};
    UART_ControlBlock_t*    Uart_p;

    /* unfortunately, the only way we have to identify which uart is involved
       is to trawl through the list looking for a TXDHandle match */

#ifdef STUART_PIOCOMPARE_STATS
    /* use top word to indicate raw interrupts received,
      bottom one to indicate matches found */
    UART_PioCompareReceived += 0x10000;
#endif

    for (Uart_p = UartQueueHead_p; Uart_p != NULL; Uart_p = Uart_p->Next_p)
    {
        if ((Uart_p->TXDHandle == Handle) && Uart_p->WriteAwaitingPioCompare)
        {
            U32 NumberBytes, NumberToWrite = Uart_p->HwFIFOSize;
            volatile UART_ASCRegs_t *ASCRegs_p = (volatile UART_ASCRegs_t *)
                            Uart_p->InitParams.BaseAddress;

            /* current behaviour with SW Flow Control on: this routine will
               transmit regardless; the interrupt handler will fire when that's
               gone out, recognise (Uart_p->UartStatus & UART_XOFF_TX) != 0
               and disable itself. Strictly we should avoid transmitting here
               - is it just sufficient to clear WriteAwaitingPioCompare ?
               Check after GlobalDisableInts to ensure we don't race. */

            /* HOWEVER, no one uses NDS flow control and SW flow control
               simultaneously */

#ifdef STUART_PIOCOMPARE_STATS
            UART_PioCompareReceived++;
#endif

            /* found it: turn off compare and write the bytes */
            (void)STPIO_SetCompare(Handle, &cmp);

            /* need to disable interrupts to ensure interrupt handler doesn't
              cross with us trying to update LastTransferSize */

            UART_GlobalDisableInts(ASCRegs_p);

            Uart_p->WriteAwaitingPioCompare = FALSE;

            if ((ASCRegs_p->ASCnStatus & STATUS_TXHALFEMPTY) != 0 &&
                NumberToWrite > 1)
            {
                NumberToWrite >>= 1;    /* Half empty transmit */
            }

            if (UART_Is9BitMode(Uart_p->DefaultParams.TxMode.DataBits))
            {
                NumberBytes = UART_Write9BitBlock(
                              Uart_p,
                              Uart_p->WriteOperation->UserBuffer,
                              MIN((NumberToWrite << 1),  /* mult 2 */
                              Uart_p->WriteOperation->NumberRemaining)
                                                 );
            }
            else
            {
                NumberBytes = UART_WriteBlock(
                              Uart_p,
                              Uart_p->WriteOperation->UserBuffer,
                              MIN(NumberToWrite,
                              Uart_p->WriteOperation->NumberRemaining)
                                             );
            }

            /* Setup the number of bytes just sent.  We don't decrement it
             * until the next interrupt.
             */
            Uart_p->WriteOperation->LastTransferSize = NumberBytes;

            if (NumberBytes == Uart_p->WriteOperation->NumberRemaining)
            {
                /* we just wrote the last bytes; require full TXEMPTY
                   before accepting write as complete */

                Uart_p->IntEnable |= UART_IE_ENABLE_TX_END;
            }
            else
            {
                /* accept any standard interrupt to continue with Write */
                Uart_p->IntEnable |= Uart_p->IntEnableTxFlags;
            }

            /* Re-trigger the timeout since we've just sent out
             * a new block of data.
             */
            stuart_TimerSignal(Uart_p->WriteOperation->Timer,TRUE);

            UART_EnableInts(ASCRegs_p, Uart_p->IntEnable);
            return;
        }
    }
    /* otherwise not found ! */
}

/* End of api.c */
